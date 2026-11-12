"""Fuzzing differenziale degli enunciati sugli insiemi.

Uso: python3 -I fuzz_sets.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Genera enunciati ben tipati a caso su due o tre insiemi (unione, intersezione,
differenza, differenza simmetrica, insieme delle parti, prodotto cartesiano,
relazioni, connettivi, "esiste un insieme") e confronta il verdetto del
programma con un oracolo indipendente, che calcola davvero ogni espressione
come insieme Python su tutti i modelli piccoli:

  - ogni regione del diagramma di Venn vuota, oppure con uno o due elementi;
  - per "esiste V", V varia su tutti i sottoinsiemi dell'universo, che contiene
    anche due elementi fuori da ogni insieme.

Un controesempio trovato dall'oracolo è certo. Quello stampato dal programma
viene ricalcolato qui.

Con la variabile d'ambiente FUZZ_FOUR=1 gli enunciati sono uguaglianze e
inclusioni su quattro insiemi; lì l'oracolo prova 1500 modelli a caso invece
di elencarli tutti.
"""
import itertools
import os
import random
import re
import subprocess
import sys

MR, SEED, COUNT, WORK = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
FOUR = os.environ.get("FUZZ_FOUR") == "1"
rnd = random.Random(SEED)
EMPTY = frozenset()


def powerset(a):
    items = sorted(a, key=repr)
    return frozenset(frozenset(c) for r in range(len(items) + 1) for c in itertools.combinations(items, r))


# Un'espressione è una tupla (operatore, figli...). Tipi: 'S' insieme di
# elementi, 'F' collezione di insiemi, 'P' insieme di coppie.
def gen(kind, depth, names):
    if kind == "S":
        if depth == 0 or rnd.random() < 0.25:
            return ("empty",) if rnd.random() < 0.08 else ("var", rnd.choice(names))
        return (rnd.choice(["cup", "cap", "minus", "minus", "tri"]), gen("S", depth - 1, names), gen("S", depth - 1, names))
    if kind == "F":
        if depth == 0 or rnd.random() < 0.45:
            return ("only_empty",) if rnd.random() < 0.15 else ("power", gen("S", max(depth - 1, 0), names))
        return (rnd.choice(["cup", "cap", "minus"]), gen("F", depth - 1, names), gen("F", depth - 1, names))
    if depth == 0 or rnd.random() < 0.5:
        return ("times", gen("S", max(depth - 1, 0), names), gen("S", max(depth - 1, 0), names))
    return (rnd.choice(["cup", "cap", "minus"]), gen("P", depth - 1, names), gen("P", depth - 1, names))


def gen_relation(names, depth):
    kind = "S" if len(names) == 4 else rnd.choice(["S", "S", "S", "S", "F", "F", "P", "P"])
    return (rnd.choice(["eq", "eq", "sub", "sub", "sup", "neq"]), gen(kind, depth, names), gen(kind, depth, names))


def gen_formula(names, depth, connectives):
    if connectives == 0:
        return gen_relation(names, depth)
    op = rnd.choice(["and", "or", "imp", "iff", "iff", "not"])
    if op == "not":
        return ("not", gen_formula(names, depth, connectives - 1))
    return (op, gen_formula(names, depth, connectives - 1), gen_formula(names, depth, connectives - 1))


BIN = {"cup": r"\cup", "cap": r"\cap", "minus": r"\setminus", "tri": r"\triangle", "times": r"\times",
       "eq": "=", "sub": r"\subseteq", "sup": r"\supseteq", "neq": r"\neq",
       "and": r"\land", "or": r"\lor", "imp": r"\implies", "iff": r"\iff"}


def tex(e):
    op = e[0]
    if op == "var":
        return e[1]
    if op == "empty":
        return r"\emptyset"
    if op == "only_empty":
        return r"\{\emptyset\}"
    if op == "power":
        return r"\mathcal{P}(%s)" % tex(e[1])
    if op == "not":
        return r"\lnot (%s)" % tex(e[1])
    if op == "exists":
        return r"\exists %s \left( %s \right)" % (e[1], tex(e[2]))
    return "(%s %s %s)" % (tex(e[1]), BIN[op], tex(e[2]))


def value(e, env):
    op = e[0]
    if op == "var":
        return env[e[1]]
    if op in ("empty",):
        return EMPTY
    if op == "only_empty":
        return frozenset([EMPTY])
    if op == "power":
        return powerset(value(e[1], env))
    a = value(e[1], env)
    if op == "not":
        return not a
    if op == "exists":
        raise ValueError
    b = value(e[2], env)
    if op == "cup":
        return a | b
    if op == "cap":
        return a & b
    if op == "minus":
        return a - b
    if op == "tri":
        return a ^ b
    if op == "times":
        return frozenset((x, y) for x in a for y in b)
    if op == "eq":
        return a == b
    if op == "sub":
        return a <= b
    if op == "sup":
        return a >= b
    if op == "neq":
        return a != b
    if op == "and":
        return a and b
    if op == "or":
        return a or b
    if op == "imp":
        return (not a) or b
    return a == b


def holds(e, env, universe):
    """Verità di una formula; 'exists' fa variare la lettera sui sottoinsiemi di universe."""
    op = e[0]
    if op == "exists":
        for subset in powerset(universe):
            env2 = dict(env)
            env2[e[1]] = subset
            if holds(e[2], env2, universe):
                return True
        return False
    if op == "not":
        return not holds(e[1], env, universe)
    if op in ("and", "or", "imp", "iff"):
        a = holds(e[1], env, universe)
        if op == "and":
            return a and holds(e[2], env, universe)
        if op == "or":
            return a or holds(e[2], env, universe)
        if op == "imp":
            return (not a) or holds(e[2], env, universe)
        return a == holds(e[2], env, universe)
    return value(e, env)


def has(e, what):
    return e[0] == what or any(isinstance(c, tuple) and has(c, what) for c in e[1:])


def models(names, sizes, extras):
    """Tutti i modelli: ogni regione (non esterna) con un numero di elementi preso da sizes."""
    regions = range(1, 1 << len(names))
    for choice in itertools.product(sizes, repeat=len(regions)):
        env = dict((n, set()) for n in names)
        element = 0
        for region, size in zip(regions, choice):
            for _ in range(size):
                element += 1
                for i, n in enumerate(names):
                    if (region >> i) & 1:
                        env[n].add(element)
        universe = frozenset(range(1, element + 1 + extras))
        yield dict((n, frozenset(s)) for n, s in env.items()), universe


def counterexample(e, names):
    """Un modello in cui la formula è falsa, oppure None."""
    exists = has(e, "exists")
    heavy = has(e, "power") or has(e, "times")
    if len(names) == 4:
        # Quattro insiemi: troppi modelli per elencarli tutti, se ne provano 1500 a caso.
        sample = random.Random(repr(e))
        for _ in range(1500):
            env = dict((n, set()) for n in names)
            for region in range(1, 16):
                if sample.random() < 0.5:
                    for i, n in enumerate(names):
                        if (region >> i) & 1:
                            env[n].add(region)
            env = dict((n, frozenset(s)) for n, s in env.items())
            if not holds(e, env, frozenset(range(1, 16))):
                return env
        return None
    if exists:
        sizes, extras = (0, 2) if not heavy else (0, 1), 2 if not heavy else 1
    elif len(names) == 2:
        sizes, extras = (0, 1, 2), 0
    else:
        sizes, extras = ((0, 1, 2) if not heavy else (0, 1)), 0
    for env, universe in models(names, sizes, extras):
        if not holds(e, env, universe):
            return env
    return None


def make_case():
    """Un enunciato a caso, con probabilità simile di essere vero o falso."""
    want_true = rnd.random() < 0.5
    for _ in range(4000):
        shape = rnd.random()
        if shape < (0.0 if FOUR else 0.2):
            names = ["S", "T"]
            inner = gen_formula(names + ["V"], rnd.choice([1, 1, 2]), rnd.choice([0, 1, 1]))
            quantified = ("exists", "V", inner)
            other = gen_formula(names, rnd.choice([1, 2]), 0)
            e = rnd.choice([quantified, ("iff", other, quantified), ("iff", quantified, other),
                            ("imp", quantified, other), ("imp", other, quantified)])
        else:
            names = ["S", "T", "V", "W"] if FOUR else ["S", "T"] if rnd.random() < 0.35 else ["S", "T", "V"]
            e = gen_formula(names, rnd.choice([1, 2, 2, 3]), rnd.choice([0, 0, 1, 1, 2]))
        used = [n for n in names if has(e, n) or any(c == ("var", n) for c in walk(e))]
        if len(used) < 2:
            continue
        bad = counterexample(e, names)
        if (bad is None) == want_true:
            return e, names, bad
    return e, names, bad


def walk(e):
    yield e
    for c in e[1:]:
        if isinstance(c, tuple):
            for d in walk(c):
                yield d


def parse_model(detail, names):
    env = dict((n, frozenset()) for n in names)
    for name, body in re.findall(r"(\w) = \{([^}]*)\}", detail):
        env[name] = frozenset(int(x) for x in body.replace(" ", "").split(",") if x)
    return env


def main():
    cases = [make_case() for _ in range(COUNT)]
    path = os.path.join(WORK, "fuzz_sets_%d.tex" % SEED)
    with open(path, "w", encoding="utf-8") as out:
        out.write("\\begin{enumerate}\n")
        for e, names, bad in cases:
            text = tex(e)
            out.write("\\item $\\displaystyle %s$\n" % (text[1:-1] if text.startswith("(") and e[0] != "not" else text))
        out.write("\\end{enumerate}\n")
    run = subprocess.run([MR, path], capture_output=True, text=True, timeout=1200)
    lines = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
    if len(lines) != len(cases) or run.returncode not in (0, 1):
        print("BUG: the program answered %d of %d statements (exit %d)\n%s" % (len(lines), len(cases), run.returncode, run.stderr[-2000:]))
        return
    tally = {}
    for number, ((e, names, bad), line) in enumerate(zip(cases, lines), 1):
        text = line.split(". ", 1)[1]
        verdict = "P" if text.startswith("DIMOSTRATO") else "R" if text.startswith("FALSO") else "N" if text.startswith("NON DIMOSTRATO") else "U"
        key = ("valid" if bad is None else "invalid", verdict)
        tally[key] = tally.get(key, 0) + 1
        problem = None
        if verdict == "P" and bad is not None:
            problem = "BUG: proved, but it fails for %s" % dict((k, sorted(v)) for k, v in bad.items())
        elif verdict == "R":
            detail = text.split("controesempio ", 1)[1] if "controesempio " in text else ""
            env = parse_model(detail, names)
            universe = frozenset().union(*env.values()) | frozenset([98, 99])
            if holds(e, env, universe):
                problem = "BUG: refuted, but the statement holds for the counterexample %s" % detail
            elif bad is None:
                problem = "NOTE: refuted with a valid counterexample that the oracle's models missed: %s" % detail
        elif verdict in ("N", "U"):
            problem = "NOTE: not decided (%s), oracle says %s" % (text[:90], "valid" if bad is None else "invalid")
        if problem:
            print("%s\n   %d: %s" % (problem, number, tex(e)))
    print("seed %d:" % SEED, ", ".join("%s/%s=%d" % (k[0], k[1], v) for k, v in sorted(tally.items())))


main()
