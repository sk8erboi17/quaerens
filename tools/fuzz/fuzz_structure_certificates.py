"""Fuzzing del verificatore sui certificati degli insiemi qualsiasi (capitolo 11 di checker.c).

Uso: python3 -I fuzz_structure_certificates.py <checker> <seme> <quanti> <cartella di lavoro> <file di certificati>...

I file di certificati sono quelli scritti da mathresearcher --certificates:
se ne prendono i certificati structure, example e counterexample, e se ne
ricavano <quanti> alterati a caso: una riga tolta o ripetuta, un nome al
posto di un altro, forall al posto di exists, un connettivo cambiato, un
valore di una tabella cambiato, il tipo scambiato. Il verificatore li
controlla tutti, e per ognuno che ACCETTA l'oracolo rifà il conto da sé:

  - counterexample, example: rilegge le tabelle e calcola l'enunciato. Se
    nel modello l'enunciato vale (counterexample) o non vale (example), o se
    una tabella non è un'interpretazione, il verificatore ha accettato il
    falso: BUG.
  - structure: prova ogni interpretazione con insiemi di 0, 1, 2 elementi
    (e 3, quando le tabelle sono poche). Se in una l'enunciato è falso, il
    verificatore ha accettato la dimostrazione di un falso: BUG. Gli insiemi
    vuoti sono provati apposta: lì sbaglia chi inventa un elemento.

Un quantificatore su parti (11g), (forallpart X S c) o (existspart X S c),
l'oracolo lo calcola provando ogni sottoinsieme di S, come frozenset, e (X t)
dice che t vi sta. Le alterazioni scambiano anche forallpart ed existspart.

Gli insiemi di parti e di applicazioni (11h), sort P parts S in e sort M maps
S T ap, sono quelli veri: l'oracolo ne calcola da sé la grandezza e la
tabella. L'insieme dei numeri scritti (11i), sort Num numbers n1 n2, ha nei
modelli un elemento per numero, tutti diversi; nelle dimostrazioni è un
insieme qualsiasi con quei nomi, come per il verificatore.

L'oracolo non usa nulla del verificatore: legge la notazione da capo e
calcola con le tabelle, quantificatore per quantificatore. Non trovare un
controesempio piccolo non prova che una dimostrazione accettata sia giusta;
è il limite di questo oracolo.

SUSPECT: un certificato accettato che l'oracolo non sa rileggere.
I file prodotti vanno nella cartella di lavoro, mai nel repository.
"""
import itertools
import os
import random
import re
import subprocess
import sys

CHECKER, SEED, COUNT, WORK = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
rnd = random.Random(SEED)
sys.setrecursionlimit(20000)
MAX_MODELS = 20000  # interpretazioni provate, al più, per una scelta delle grandezze
KINDS = ("structure", "example", "counterexample")


def read_certificates(paths):
    out = []
    for path in paths:
        for block in open(path, encoding="utf-8").read().split("certificate ")[1:]:
            if block.split("\n", 1)[0].strip() in KINDS:
                out.append("certificate " + block.strip() + "\n")
    return out


def tokens(text):
    return re.findall(r"\(|\)|[^\s()]+", text)


def parse(tok, i):
    if tok[i] == "(":
        out, i = [], i + 1
        while tok[i] != ")":
            node, i = parse(tok, i)
            out.append(node)
        return out, i + 1
    return tok[i], i + 1


class Unreadable(Exception):
    pass


DERIVED = {}  # 11h: insieme di parti o di applicazioni -> (tipo, base, valori o None, simbolo)
NUMBERS = {}  # 11i: insieme dei numeri scritti -> i nomi dei numeri, nel loro ordine


def derived_size(name, size):
    """La grandezza dell'insieme delle parti (2^n) o delle applicazioni (m^n)."""
    kind, base, target, _ = DERIVED[name]
    return (2 if kind == "parts" else size[target]) ** size[base]


def derived_table(name, size):
    """La tabella del simbolo di struttura, come la definisce il formato dei
    certificati: la parte numero k ha l'elemento i se il bit i di k è 1;
    l'applicazione numero k manda i nella cifra i di k in base m. Il conto è
    fatto qui da capo, con gli insiemi di Python."""
    kind, base, target, _ = DERIVED[name]
    n, table = size[base], {}
    for k in range(derived_size(name, size)):
        if kind == "parts":
            members = {i for i in range(n) if k >> i & 1}
            for i in range(n):
                table[(i, k)] = int(i in members)
        else:
            m = size[target]
            for i in range(n):
                table[(k, i)] = (k // m ** i) % m
    return table


def read(text):
    """Il tipo, gli insiemi (nome -> grandezza o None), i simboli (nome -> (argomenti, valore)), l'enunciato, le tabelle."""
    lines = text.strip().splitlines()
    kind = lines[0].split()[1]
    sorts, symbols, tables, statement = {}, {}, {}, None
    DERIVED.clear()
    NUMBERS.clear()
    for line in lines[1:]:
        word, _, rest = line.strip().partition(" ")
        parts = rest.split()
        if word == "sort" and len(parts) > 1 and parts[1] in ("parts", "maps"):
            # 11h: sort P parts S in, sort M maps S T ap.
            maps = parts[1] == "maps"
            base, target, symbol = parts[2], parts[3] if maps else None, parts[4 if maps else 3]
            DERIVED[parts[0]] = (parts[1], base, target, symbol)
            sorts[parts[0]] = None
            symbols[symbol] = ([parts[0], base], target) if maps else ([base, parts[0]], "bool")
        elif word == "sort" and len(parts) > 1 and parts[1] == "numbers":
            # 11i: sort Num numbers n1 n2, i numeri scritti, tutti diversi.
            NUMBERS[parts[0]] = parts[2:]
            sorts[parts[0]] = len(parts[2:])
            for name in parts[2:]:
                symbols[name] = ([], parts[0])
        elif word == "sort":
            sorts[parts[0]] = int(parts[1]) if len(parts) > 1 else None
        elif word == "symbol":
            symbols[parts[0]] = (parts[1:-1], parts[-1])
        elif word == "statement":
            statement = parse(tokens(rest), 0)[0]
        elif word == "table":
            tables[parts[0]] = [int(v) for v in parts[1:]]
    if statement is None:
        raise Unreadable("no statement")
    return kind, sorts, symbols, statement, tables


def cells(symbol, symbols, size):
    """Le scelte degli argomenti del simbolo, nell'ordine delle celle: il primo argomento cambia più in fretta."""
    args = symbols[symbol][0]
    return [tuple(reversed(c)) for c in itertools.product(*[range(size[a]) for a in reversed(args)])]


def term(t, env, model):
    if isinstance(t, str):
        if t in env:
            return env[t]
        return model[t][()]
    return model[t[0]][tuple(term(a, env, model) for a in t[1:])]


def holds(c, env, model, size, symbols):
    if c == "true":
        return True
    if isinstance(c, str):
        raise Unreadable("condition %r" % c)
    head = c[0]
    if head == "=":
        return term(c[1], env, model) == term(c[2], env, model)
    if head == "not":
        return not holds(c[1], env, model, size, symbols)
    if head in ("and", "or", "implies", "iff"):
        a, b = holds(c[1], env, model, size, symbols), holds(c[2], env, model, size, symbols)
        return {"and": a and b, "or": a or b, "implies": (not a) or b, "iff": a == b}[head]
    if head in ("forall", "exists"):
        values = (holds(c[3], dict(env, **{c[1]: x}), model, size, symbols) for x in range(size[c[2]]))
        return all(values) if head == "forall" else any(values)
    if head in ("forallpart", "existspart"):
        n = size[c[2]]
        parts = (frozenset(p) for k in range(n + 1) for p in itertools.combinations(range(n), k))
        values = (holds(c[3], dict(env, **{c[1]: p}), model, size, symbols) for p in parts)
        return all(values) if head == "forallpart" else any(values)
    if isinstance(env.get(head), frozenset):
        return term(c[1], env, model) in env[head]
    if head in symbols and symbols[head][1] == "bool":
        return bool(term(c, env, model))
    raise Unreadable("condition %r" % (c,))


def model_from_tables(sorts, symbols, tables):
    """L'interpretazione delle tabelle, o None se non è un'interpretazione."""
    model = {}
    for name in DERIVED:
        sorts[name] = derived_size(name, sorts)
    for name, (args, result) in symbols.items():
        structure = [d for d in DERIVED if DERIVED[d][3] == name]
        if structure:
            if name in tables:
                return None
            model[name] = derived_table(structure[0], sorts)
            continue
        numeral = [n for n in NUMBERS if name in NUMBERS[n]]
        if numeral:
            # 11i: il k-esimo numero è l'elemento k, e non ha tabella.
            if name in tables:
                return None
            model[name] = {(): NUMBERS[numeral[0]].index(name)}
            continue
        keys = cells(name, symbols, sorts)
        values = tables.get(name)
        top = 2 if result == "bool" else sorts[result]
        if values is None or len(values) != len(keys) or any(v < 0 or v >= top for v in values):
            return None
        model[name] = dict(zip(keys, values))
    return model


def counter_model(sorts, symbols, statement):
    """Un'interpretazione con insiemi piccoli in cui l'enunciato è falso, oppure None."""
    names = [name for name in sorts if name not in DERIVED]
    for sizes in itertools.product(range(4), repeat=len(names)):
        size = dict(zip(names, sizes))
        # 11h: gli insiemi di parti e di applicazioni sono quelli veri, con
        # la loro tabella; non si sceglie nulla.
        for name in DERIVED:
            size[name] = derived_size(name, size)
        if any(size[name] > 8 for name in DERIVED):
            continue
        choices, count = [], 1
        for name, (args, result) in symbols.items():
            structure = [d for d in DERIVED if DERIVED[d][3] == name]
            if structure:
                choices.append([derived_table(structure[0], size)])
                continue
            keys = cells(name, symbols, size)
            top = 2 if result == "bool" else size[result]
            count *= top ** len(keys)
            choices.append([dict(zip(keys, values)) for values in itertools.product(range(top), repeat=len(keys))] if count <= MAX_MODELS else [])
        if count > MAX_MODELS or (max(sizes, default=0) == 3 and count > MAX_MODELS // 10):
            continue
        for tables in itertools.product(*choices):
            model = dict(zip(symbols, tables))
            if not holds(statement, {}, model, size, symbols):
                return size, model
    return None


def mutate(text):
    """Il certificato text con una o due alterazioni a caso."""
    lines = text.strip().splitlines()
    for _ in range(rnd.choice([1, 1, 2])):
        head, body = lines[0], lines[1:-1]
        what = rnd.randrange(7)
        pick = rnd.randrange(len(body))
        names = sorted(set(re.findall(r"[A-Za-z][A-Za-z0-9_]*", " ".join(body))) - {"forall", "exists", "forallpart", "existspart", "and", "or", "not", "implies", "iff", "true", "bool"} - set(KINDS) - {"sort", "symbol", "statement", "table", "case", "instance", "witness", "part"})
        if what == 0 and len(body) > 1:
            del body[pick]
        elif what == 1:
            body.insert(pick, body[pick])
        elif what == 2 and names:
            # Un nome al posto di un altro, in un punto solo della riga.
            words = list(re.finditer(r"[A-Za-z][A-Za-z0-9_]*", body[pick]))
            spots = [w for w in words if w.group() in names]
            if spots:
                w = rnd.choice(spots)
                body[pick] = body[pick][:w.start()] + rnd.choice(names) + body[pick][w.end():]
        elif what == 3:
            spots = [m.start() for m in re.finditer(r"\((forall|exists)(part)? ", body[pick])]
            if spots:
                at = rnd.choice(spots) + 1
                word = "exists" if body[pick][at:at + 6] == "forall" else "forall"
                body[pick] = body[pick][:at] + word + body[pick][at + 6:]
        elif what == 4:
            spots = list(re.finditer(r"\((and|or|implies|iff) ", body[pick]))
            if spots:
                m = rnd.choice(spots)
                body[pick] = body[pick][:m.start(1)] + rnd.choice(["and", "or", "implies", "iff"]) + body[pick][m.end(1):]
        elif what == 5:
            tables = [i for i, l in enumerate(body) if l.startswith("table ")]
            if tables:
                i = rnd.choice(tables)
                parts = body[i].split()
                if len(parts) > 2:
                    j = rnd.randrange(2, len(parts))
                    parts[j] = str(rnd.choice([0, 1, 2]))
                    body[i] = " ".join(parts)
        else:
            kind = head.split()[1]
            if kind != "structure":
                head = "certificate " + ("example" if kind == "counterexample" else "counterexample")
        lines = [head] + body + ["end"]
    return "\n".join(lines) + "\n"


def show(node):
    return node if isinstance(node, str) else "(" + " ".join(show(x) for x in node) + ")"


def substitute(c, name, value):
    """La condizione (o il termine) c con value al posto della variabile name, dove non è legata più dentro."""
    if isinstance(c, str):
        return value if c == name else c
    if c[0] in ("forall", "exists", "forallpart", "existspart"):
        return c if c[1] == name else [c[0], c[1], c[2], substitute(c[3], name, value)]
    return [c[0]] + [substitute(x, name, value) for x in c[1:]]


def quantified(c, found):
    """Mette in found le condizioni quantificate di c che non stanno sotto un quantificatore."""
    if isinstance(c, str) or c[0] in ("=",):
        return
    if c[0] in ("forall", "exists", "forallpart", "existspart"):
        if c not in found:
            found.append(c)
        return
    for x in c[1:]:
        quantified(x, found)


def random_term(sort, env, ground, depth):
    """Un termine dell'insieme sort con le variabili di env (nome -> insieme) e i termini chiusi di ground."""
    options = [v for v, s in env.items() if s == sort] + [show(g) for g, s in ground if s == sort and isinstance(g, str)]
    if depth > 0 and rnd.random() < 0.4:
        for name, (args, result) in rnd.sample(list(RANDOM_SYMBOLS.items()), len(RANDOM_SYMBOLS)):
            if result == sort and args:
                inner = [random_term(a, env, ground, depth - 1) for a in args]
                if None not in inner:
                    return [name] + inner
    return rnd.choice(options) if options else None


def random_condition(env, ground, depth, sorts):
    """Una condizione a caso, con le variabili di env."""
    r = rnd.random()
    if depth > 0 and r < 0.35:
        sort, var = rnd.choice(sorts), rnd.choice("xyz")
        return [rnd.choice(["forall", "exists"]), var, sort, random_condition(dict(env, **{var: sort}), ground, depth - 1, sorts)]
    if depth > 0 and r < 0.6:
        word = rnd.choice(["and", "or", "implies", "iff", "not"])
        if word == "not":
            return ["not", random_condition(env, ground, depth - 1, sorts)]
        return [word, random_condition(env, ground, depth - 1, sorts), random_condition(env, ground, depth - 1, sorts)]
    sort = rnd.choice(sorts)
    a, b = random_term(sort, env, ground, 1), random_term(sort, env, ground, 1)
    if a is None or b is None:
        return "true"
    if sort == "S" and rnd.random() < 0.4:
        return ["P", a]
    return ["=", a, b]


RANDOM_SYMBOLS = {}


def random_proof():
    """Un certificato structure a caso: un enunciato piccolo e delle istanze e dei nomi presi a caso tra quelli leciti."""
    sorts = ["S", "T"] if rnd.random() < 0.5 else ["S"]
    RANDOM_SYMBOLS.clear()
    RANDOM_SYMBOLS["f"] = (["S"], rnd.choice(sorts))
    RANDOM_SYMBOLS["P"] = (["S"], "bool")
    if rnd.random() < 0.4:
        RANDOM_SYMBOLS["m"] = (["S", "S"], "S")
    if rnd.random() < 0.4:
        RANDOM_SYMBOLS["u"] = ([], "S")
    ground = [(name, result) for name, (args, result) in RANDOM_SYMBOLS.items() if not args]
    statement = ["implies", random_condition({}, ground, 2, sorts), random_condition({}, ground, 2, sorts)]
    lines = ["certificate structure"] + ["sort %s" % s for s in sorts]
    lines += ["symbol %s %s" % (n, " ".join(a + [r])) for n, (a, r) in RANDOM_SYMBOLS.items()]
    lines += ["statement %s" % show(statement), "case (not %s)" % show(statement)]
    available = []
    quantified(statement, available)
    for step in range(rnd.randrange(1, 6)):
        if not available:
            break
        q = rnd.choice(available)
        if rnd.random() < 0.35:
            name = "w_%d" % step
            body = substitute(q[3], q[1], name)
            ground.append((name, q[2]))
            lines.append("witness %s %s" % (show(q), name))
        else:
            body, terms = q, []
            while not isinstance(body, str) and body[0] == q[0] and (not terms or rnd.random() < 0.5):
                t = random_term(body[2], {}, ground, 1)
                if t is None:
                    break
                terms.append(t)
                body = substitute(body[3], body[1], t)
            if not terms:
                continue
            lines.append("instance %s %s" % (show(q), " ".join(show(t) for t in terms)))
        quantified(body, available)
    return "\n".join(lines + ["end"]) + "\n"


def judge(text):
    """Un problema del certificato accettato text, o None se l'oracolo non ne trova."""
    kind, sorts, symbols, statement, tables = read(text)
    if kind in ("example", "counterexample"):
        model = model_from_tables(sorts, symbols, tables)
        if model is None:
            return "accepted, but its tables are not an interpretation"
        if holds(statement, {}, model, sorts, symbols) != (kind == "example"):
            return "accepted, but in its model the statement is %s" % ("false" if kind == "example" else "true")
        return None
    found = counter_model(sorts, symbols, statement)
    if found:
        return "accepted as a proof, but the statement fails with sets of sizes %s: %s" % (found[0], found[1])
    return None


def main():
    certificates = read_certificates(sys.argv[5:])
    if not certificates:
        print("no structure certificate in the files given")
        return
    mutants = [mutate(rnd.choice(certificates)) if rnd.random() < 0.5 else random_proof() for _ in range(COUNT)]
    path = os.path.join(WORK, "fuzz_structure_certificates_%d.cert" % SEED)
    with open(path, "w", encoding="utf-8") as out:
        out.write("\n".join(mutants))
    run = subprocess.run([CHECKER, path], capture_output=True, text=True, errors="replace", timeout=3000)
    answers = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
    if len(answers) != len(mutants) or run.returncode not in (0, 1):
        print("BUG: the checker answered %d of %d certificates (exit %d)\n%s" % (len(answers), len(mutants), run.returncode, run.stderr[-2000:]))
        return
    stats = {}
    for number, (mutant, answer) in enumerate(zip(mutants, answers), 1):
        verdict = answer.split(". ", 1)[1].split(":")[0]
        stats[verdict] = stats.get(verdict, 0) + 1
        if verdict != "ACCETTATO":
            continue
        try:
            problem = judge(mutant)
        except (Unreadable, KeyError, IndexError, ValueError) as error:
            print("SUSPECT %d: accepted, but the oracle cannot read it (%r)\n%s" % (number, error, mutant))
            continue
        if problem:
            print("BUG %d: %s\n%s" % (number, problem, mutant))
    print("seed %d: %d certificates altered from %d, %s" % (SEED, len(mutants), len(certificates), stats))


main()
