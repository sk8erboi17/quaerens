"""Fuzzing del verificatore dei certificati, con un oracolo indipendente.

Uso: python3 -I fuzz_checker.py <checker> <seme> <quanti> <cartella di lavoro> <file di certificati>...

Costruisce certificati in tre modi:

  - alterando quelli veri prodotti dal motore (un numero, un operatore, un
    sotto-albero, il fattore, il caso base, una voce generic);
  - scrivendo identità vere per costruzione (somme e prodotti telescopici),
    e poi alterando anche quelle;
  - con fattori scelti per ingannare: 0, 1, il rapporto D(n+1)/D(n);
  - con la voce "defined" su enunciati che smettono di essere definiti più
    avanti: un divisore che si annulla in un n lontano, scritto in molti modi.

Li dà al verificatore e, per ogni certificato ACCETTATO, ricalcola i due
membri con le frazioni esatte di Python, per n dal caso base in su e per
alcuni valori dei parametri: dove l'enunciato è definito (da b fino a n) e le
voci generic non sono zero, i due membri devono coincidere. Se non coincidono
il verificatore ha accettato il falso: BUG. Se il certificato ha la voce
"defined", con quei valori dei parametri l'enunciato deve anche essere
definito per ogni n provato, fino a oltre 200 passi dal caso base.

Segnala anche le identità vere per costruzione che il verificatore respinge
(non è un errore di correttezza, ma è una dimostrazione vera persa).
"""
import os
import random
import re
import subprocess
import sys
from decimal import Decimal, DecimalException, getcontext
from fractions import Fraction

getcontext().prec = 80

CHECKER, SEED, COUNT, WORK = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
rnd = random.Random(SEED)
sys.setrecursionlimit(20000)


# ---------------------------------------------------------------- lettura
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


def show(node):
    return node if isinstance(node, str) else "(" + " ".join(show(x) for x in node) + ")"


def read_certificates(path):
    out = []
    for block in open(path, encoding="utf-8").read().split("certificate induction")[1:]:
        cert = {"variable": "n", "base": 1, "generic": [], "sequence": [], "define": [], "defined": None, "brick": []}
        for line in block.strip().splitlines():
            word, _, rest = line.strip().partition(" ")
            if word == "variable":
                cert["variable"] = rest.strip()
            elif word == "base":
                cert["base"] = int(rest)
            elif word in ("statement", "factor"):
                cert[word] = parse(tokens(rest), 0)[0]
            elif word in ("generic", "brick"):
                cert[word].append(parse(tokens(rest), 0)[0])
            elif word in ("sequence", "define"):
                cert[word].append(rest.strip())
            elif word == "defined":
                tok, i, cert["defined"] = tokens(rest), 0, []
                while i < len(tok):
                    value, j = parse(tok, i + 1)
                    cert["defined"].append((tok[i], value))
                    i = j
        if "statement" in cert and "factor" in cert:
            out.append(cert)
    return out


def write(cert):
    lines = ["certificate induction", "variable %s" % cert["variable"], "base %d" % cert["base"],
             "statement %s" % show(cert["statement"]), "factor %s" % show(cert["factor"])]
    lines += ["generic %s" % show(g) for g in cert["generic"]]
    lines += ["brick %s" % show(b) for b in cert.get("brick", [])]
    if cert.get("defined") is not None:
        lines.append(" ".join(["defined"] + ["%s %s" % (name, show(value)) for name, value in cert["defined"]]))
    lines += ["sequence %s" % s for s in cert["sequence"]] + ["define %s" % s for s in cert["define"]]
    return "\n".join(lines + ["end", ""])


# ---------------------------------------------------------------- oracolo
class Undefined(Exception):
    pass


class Strange(Exception):
    """Qualcosa che il verificatore non avrebbe dovuto accettare affatto."""


# Somme parziali già calcolate e lettere libere dei termini, per il
# certificato in esame: judge() le svuota a ogni certificato.
PARTIAL, FREE = {}, {}

# I numeri dell'oracolo sono frazioni esatte. Un certificato con un logaritmo
# non si può calcolare così: allora EXACT è falso e i numeri sono decimali a
# 80 cifre, confrontati con una tolleranza molto più stretta di qualunque
# errore di formula.
EXACT = True
TINY = Decimal(10) ** -55


def num(value):
    """Un intero o una frazione, nel tipo di numero in uso."""
    if EXACT:
        return Fraction(value)
    value = Fraction(value)
    return Decimal(value.numerator) / Decimal(value.denominator)


def is_zero(value):
    return value == 0 if EXACT else abs(value) < TINY


def same(a, b):
    return a == b if EXACT else abs(a - b) <= TINY * max(1, abs(a), abs(b))


def whole(value):
    """Il valore come intero Python, oppure None se non è intero."""
    if EXACT:
        return value.numerator if value.denominator == 1 else None
    nearest = value.to_integral_value()
    return int(nearest) if abs(value - nearest) < TINY else None


def integer(value, what):
    out = whole(value)
    if out is None:
        raise Strange("non-integer " + what)
    return out


def ev(node, env, seqs):
    if isinstance(node, str):
        if re.fullmatch(r"-?\d+", node):
            return num(int(node))
        if node not in env:
            raise Strange("free letter " + node)
        return env[node]
    op = node[0]
    if op in ("+", "-", "*", "/"):
        a, b = ev(node[1], env, seqs), ev(node[2], env, seqs)
        if op == "+":
            return a + b
        if op == "-":
            return a - b
        if op == "*":
            return a * b
        if is_zero(b):
            raise Undefined()
        return a / b
    if op == "neg":
        return -ev(node[1], env, seqs)
    if op in ("ln", "lnabs"):
        if EXACT:
            raise Strange("logarithm evaluated with exact numbers")
        a = ev(node[1], env, seqs)
        if op == "lnabs":
            a = abs(a)
        if a < TINY:
            raise Undefined()
        return a.ln()
    if op == "^":
        a, e = ev(node[1], env, seqs), integer(ev(node[2], env, seqs), "exponent")
        if is_zero(a) and (e < 0 or not (isinstance(node[2], str) and re.fullmatch(r"-?\d+", node[2]))):
            raise Undefined()
        if abs(e) > 3000:
            raise Undefined()
        if e == 0:
            return num(1)
        return a ** e
    if op in ("sum", "prod"):
        lo, hi = integer(ev(node[2], env, seqs), "bound"), integer(ev(node[3], env, seqs), "bound")
        if hi < lo - 1 or hi - lo > 5000:
            raise Undefined()

        # La stessa somma viene chiesta con estremi via via più alti: si
        # riparte dall'ultima somma parziale invece che da capo. La chiave
        # dice quale somma è: il nodo, l'estremo inferiore e il valore delle
        # lettere libere del termine. Un termine non definito non entra mai
        # nella tabella, perché l'eccezione esce prima.
        free = FREE.get(id(node))
        if free is None:
            free = FREE[id(node)] = sorted(letters(node[4]) - {node[1]})
        key = (id(node), lo, tuple(env.get(name) for name in free))
        start, total = PARTIAL.get(key, (lo - 1, num(1 if op == "prod" else 0)))
        if start > hi:
            start, total = lo - 1, num(1 if op == "prod" else 0)
        inner = dict(env)
        for k in range(start + 1, hi + 1):
            inner[node[1]] = num(k)
            value = ev(node[4], inner, seqs)
            total = total * value if op == "prod" else total + value
        PARTIAL[key] = (hi, total)
        return total
    if op == "fact":
        a = whole(ev(node[1], env, seqs))
        if a is None or a < 0 or a > 3000:
            raise Undefined()
        out = 1
        for j in range(2, a + 1):
            out *= j
        return num(out)
    if op == "binom":
        a, b = ev(node[1], env, seqs), whole(ev(node[2], env, seqs))
        if b is None or b < 0 or b > 3000:
            raise Undefined()
        out = num(1)
        for j in range(1, b + 1):
            out = out * (a - b + j) / j
        return out
    if op == "seq":
        kind, data = seqs[node[1]]
        index = ev(node[2], env, seqs)
        if kind == "define":
            parameter, formula = data
            if letters(formula) - {parameter}:
                raise Strange("define with a free letter")
            return ev(formula, {parameter: index}, seqs)
        coef, init = data
        index = whole(index)
        if index is None or index < 0 or index > 5000:
            raise Undefined()
        window = [num(v) for v in init]
        for _ in range(index):
            window = window[1:] + [sum(c * w for c, w in zip(coef, reversed(window)))]
        return window[0]
    raise Strange("operator " + op)


def letters(node, bound=()):
    """Le lettere libere di un albero."""
    if isinstance(node, str):
        return set() if re.fullmatch(r"-?\d+", node) or node in bound else {node}
    if node[0] in ("sum", "prod"):
        return letters(node[2], bound) | letters(node[3], bound) | letters(node[4], bound + (node[1],))
    if node[0] == "seq":
        return letters(node[2], bound)
    out = set()
    for child in node[1:]:
        out |= letters(child, bound)
    return out


def judge(cert):
    """Ritorna None se il teorema regge sui punti provati, altrimenti il testo del controesempio."""
    global EXACT
    PARTIAL.clear()
    FREE.clear()
    EXACT = "(ln" not in write(cert)
    seqs = {}
    for line in cert["sequence"]:
        parts = line.split()
        order = int(parts[1])
        seqs[parts[0]] = ("recurrence", ([int(x) for x in parts[2:2 + order]], [int(x) for x in parts[2 + order:2 + 2 * order]]))
    for line in cert["define"]:
        name, parameter, formula = line.split(None, 2)
        seqs[name] = ("define", (parameter, parse(tokens(formula), 0)[0]))
    variable = cert["variable"]
    free = letters(cert["statement"]) | set().union(*[letters(g) for g in cert["generic"]] or [set()])
    for kind, data in list(seqs.values()):
        if kind == "define":
            free |= letters(data[1]) - {data[0]}
    parameters = sorted(free - {variable})
    seqs["#parameters"] = set(parameters)
    pool = [Fraction(v) for v in (2, 3, -2, 5, 7, -3, 1, -1, 0, 11)] + [Fraction(1, 2), Fraction(-3, 2), Fraction(5, 3), Fraction(101), Fraction(7, 10)]

    # Con la voce "defined" i parametri nominati hanno il loro valore, e
    # l'enunciato deve essere definito per ogni n: lo si prova molto più in là.
    fixed = {}
    for name, value in cert.get("defined") or []:
        fixed[name] = ev(value, {}, seqs)
    claimed = cert.get("defined") is not None
    for attempt in range(2 if claimed else 6):
        env = {p: num(pool[(attempt + i) % 4] if attempt < 2 else rnd.choice(pool)) for i, p in enumerate(parameters)}
        env.update(fixed)
        try:
            zero = any(is_zero(ev(g, env, seqs)) for g in cert["generic"])
        except Undefined:
            zero = True
        if zero:
            if claimed:
                return "a generic entry is zero or undefined at the point %s" % {p: str(env[p]) for p in parameters}
            continue
        for n in range(cert["base"], cert["base"] + (230 if claimed else 9)):
            env[variable] = num(n)
            try:
                left, right = ev(cert["statement"][1], env, seqs), ev(cert["statement"][2], env, seqs)
            except Undefined:
                if claimed:
                    return "claimed defined, but undefined at n = %d, %s" % (n, {p: str(env[p]) for p in parameters})
                break
            if not same(left, right):
                return "n = %d, %s: %s vs %s" % (n, {p: str(env[p]) for p in parameters}, left, right)
    return None


# ---------------------------------------------------------------- alterazioni
def paths(node, here=()):
    yield here
    if not isinstance(node, str):
        for i, child in enumerate(node[1:], 1):
            if node[0] in ("sum", "prod", "seq") and i == 1:
                continue
            yield from paths(child, here + (i,))


def get(node, path):
    for i in path:
        node = node[i]
    return node


def put(node, path, value):
    if not path:
        return value
    out = list(node)
    out[path[0]] = put(node[path[0]], path[1:], value)
    return out


def substitute(node, name, value):
    if isinstance(node, str):
        return value if node == name else node
    if node[0] in ("sum", "prod") and node[1] == name:
        return [node[0], node[1], substitute(node[2], name, value), substitute(node[3], name, value), node[4]]
    return [node[0]] + [child if (i == 0 and node[0] in ("sum", "prod", "seq")) else substitute(child, name, value) for i, child in enumerate(node[1:])]


def trick(cert):
    """Il fattore D(n+1)/D(n), scritto per esteso."""
    n = cert["variable"]
    left, right = cert["statement"][1], cert["statement"][2]
    now = ["-", left, right]
    return ["/", substitute(now, n, ["+", n, "1"]), now]


def point_for(cert):
    """Una voce "defined" per il certificato: un valore per ogni parametro, a volte per pochi."""
    free = sorted(letters(cert["statement"]) - {cert["variable"]})
    values = ["2", "3", "5", "-2", "1", "0", "-1", ["/", "1", "2"], ["/", "3", "2"], "7", "101"]
    return [(name, rnd.choice(values)) for name in free if rnd.random() < 0.9]


def mutate(cert, pool):
    out = dict(cert, generic=list(cert["generic"]), sequence=list(cert["sequence"]), define=list(cert["define"]),
               brick=list(cert.get("brick", [])))
    if rnd.random() < 0.45:
        out["defined"] = point_for(out)
    elif rnd.random() < 0.3:
        out["defined"] = None
    for _ in range(rnd.choice([1, 1, 1, 2, 3])):
        kind = rnd.random()
        where = "statement" if rnd.random() < 0.75 else "factor"
        tree = out[where]
        all_paths = [p for p in paths(tree) if p]
        if not all_paths:
            all_paths = [()]
        path = rnd.choice(all_paths)
        node = get(tree, path)
        if kind < 0.25:
            numbers = [p for p in all_paths if isinstance(get(tree, p), str) and re.fullmatch(r"-?\d+", get(tree, p))]
            if numbers:
                path = rnd.choice(numbers)
                value = int(get(tree, path))
                out[where] = put(tree, path, str(rnd.choice([value + 1, value - 1, value * 2, 0, -value, value + 2])))
        elif kind < 0.40:
            swaps = {"+": "-", "-": "+", "*": "/", "/": "*", "sum": "prod", "prod": "sum", "^": "*", "neg": "fact", "fact": "neg",
                     "ln": "lnabs", "lnabs": "ln"}
            if not isinstance(node, str) and node[0] in swaps:
                out[where] = put(tree, path, [swaps[node[0]]] + node[1:])
        elif kind < 0.55:
            other = rnd.choice(pool)
            source = other[rnd.choice(["statement", "factor"])]
            out[where] = put(tree, path, get(source, rnd.choice(list(paths(source)))))
        elif kind < 0.65:
            if not isinstance(node, str) and len(node) == 3 and node[0] not in ("seq",):
                out[where] = put(tree, path, [node[0], node[2], node[1]])
        elif kind < 0.75:
            names = sorted(letters(cert["statement"]) | {cert["variable"], "k", "x"})
            leaves = [p for p in all_paths if isinstance(get(tree, p), str)]
            if leaves:
                out[where] = put(tree, rnd.choice(leaves), rnd.choice(names + ["1", "0", "2"]))
        elif kind < 0.82:
            out["base"] = cert["base"] + rnd.choice([-2, -1, 1, 2, 5])
        elif kind < 0.90:
            n = cert["variable"]
            out["factor"] = rnd.choice(["0", "1", "-1", "2", n, ["/", ["+", n, "2"], ["+", n, "1"]], trick(out),
                                        ["/", "1", ["-", out["statement"][1], out["statement"][2]]],
                                        ["/", out["statement"][1], out["statement"][2]]])
        elif kind < 0.93:
            source = out["statement"]
            out["generic"].append(get(source, rnd.choice(list(paths(source)))))
        elif out["sequence"] and rnd.random() < 0.3:
            parts = out["sequence"][0].split()
            i = rnd.randrange(1, len(parts))
            parts[i] = str(int(parts[i]) + rnd.choice([-1, 1]))
            out["sequence"][0] = " ".join(parts)
        else:
            # Una definizione nuova, che può nascondere una lettera, usata al posto di un pezzo dell'enunciato.
            n = cert["variable"]
            formula = rnd.choice(["(* %s m)" % n, "(* k m)", "(+ m x)", "(^ m 2)", "(sum j 1 m (* j %s))" % n, "(sum j 1 m (* j m))",
                                  "(sum %s 1 m %s)" % (n, n), "(/ 1 (+ m 1))", "(seq F m)", "(seq G m)", "(prod j 1 m (+ j 1))", "(^ 2 m)", "(^ x m)"])
            out["define"] = [d for d in out["define"] if not d.startswith("G ")] + ["G %s %s" % (rnd.choice(["m", "m", "m", n, "k"]), formula)]
            if "sequence" in out and not out["sequence"]:
                out["sequence"] = ["F 2 1 1 0 1"]
            tree = out["statement"]
            spots = [p for p in paths(tree) if len(p) >= 2]
            if spots:
                spot = rnd.choice(spots)
                out["statement"] = put(tree, spot, ["seq", "G", get(tree, spot)] if rnd.random() < 0.5 else ["seq", "G", rnd.choice(["k", n, "1", ["+", n, "1"]])])
    return out


# ---------------------------------------------------------------- identità vere
def build_g(index):
    """Un'espressione g(index) fatta dei mattoni che il verificatore conosce."""
    a, b, c = rnd.randint(1, 4), rnd.randint(0, 5), rnd.randint(2, 5)
    shifted = ["+", index, str(b)] if b else index
    bricks = [
        lambda: index,
        lambda: ["^", shifted, str(rnd.randint(2, 4))],
        lambda: ["/", "1", ["+", ["*", str(a), index], str(b + 1)]],
        lambda: ["^", str(c), index],
        lambda: ["^", "x", index],
        lambda: ["^", "-1", index],
        lambda: ["fact", shifted],
        lambda: ["binom", ["+", index, str(b + 2)], str(rnd.randint(1, 4))],
        lambda: ["seq", "F", ["+", index, str(b)]],
        lambda: ["seq", "H", ["+", index, str(b + 1)]],
        lambda: ["/", ["^", str(c), index], ["fact", shifted]],
        lambda: ["^", ["/", "x", str(c)], index],
        lambda: ["^", ["*", str(c), "y"], ["*", "2", index]],
        lambda: ["prod", "j", "1", index, ["+", "j", str(a)]],
        lambda: ["sum", "j", "1", index, ["^", "j", "2"]],
        lambda: ["ln", ["+", index, str(b + 1)]],
        lambda: ["ln", ["*", ["+", index, str(b + 1)], ["+", index, str(b + 2)]]],
        lambda: ["ln", ["/", ["+", index, str(b + 2)], ["+", ["*", "2", index], str(2 * b + 3)]]],
        lambda: ["*", index, ["ln", ["+", index, str(b + 1)]]],
        lambda: ["ln", ["^", str(c), index]],
        lambda: ["lnabs", ["-", index, str(b + 3)]],
        lambda: ["ln", ["fact", ["+", index, "1"]]],
    ]
    g = rnd.choice(bricks)()
    if rnd.random() < 0.45:
        g = [rnd.choice(["*", "*", "+", "/"]), g, rnd.choice(bricks)()]
    if rnd.random() < 0.25:
        g = ["*", rnd.choice(["x", "2", ["+", "x", "1"], ["/", "1", "3"]]), g]
    return g


def telescoping():
    """Un certificato vero per costruzione: somma o prodotto telescopico di g."""
    base = rnd.choice([1, 1, 1, 2, 3, 0])
    shape = build_g("@")
    g = lambda index: substitute(shape, "@", index)
    k, n = "k", "n"
    before = str(base - 1)
    if rnd.random() < 0.65:
        statement = ["=", ["sum", k, str(base), n, ["-", g(k), g(["-", k, "1"])]], ["-", g(n), g(before)]]
        factor = "1"
    else:
        statement = ["=", ["prod", k, str(base), n, ["/", g(k), g(["-", k, "1"])]], ["/", g(n), g(before)]]
        factor = ["/", g(["+", n, "1"]), g(n)]
    if rnd.random() < 0.3:
        statement = ["=", statement[2], statement[1]]
    # I fattori che servono a scomporre gli argomenti dei logaritmi: a volte
    # tutti, a volte nessuno (e allora il verificatore può non riconoscere
    # l'identità, ma non deve accettarne una falsa).
    bricks = [["+", n, str(j)] for j in range(0, 9)] + [["+", ["*", "2", n], str(j)] for j in range(1, 16, 2)]
    cert = {"variable": n, "base": base, "statement": statement, "factor": factor, "generic": [],
            "sequence": ["F 2 1 1 0 1"], "define": ["H m (sum j 1 m (/ 1 j))"], "defined": None,
            "brick": bricks if rnd.random() < 0.8 else []}
    if rnd.random() < 0.6:
        cert["defined"] = point_for(cert)
    return cert


def vanishing():
    """Un enunciato vero dove è definito, con un divisore che si annulla in
    n = r, lontano dal caso base: la voce "defined" è falsa."""
    n, base = "n", rnd.choice([0, 1, 1, 2])
    r = base + rnd.randint(1, 200)
    q = rnd.choice([2, 3])
    disguises = [
        lambda: ["-", n, str(r)],
        lambda: ["-", ["^", n, "2"], str(r * r)],
        lambda: ["*", ["-", n, str(r)], ["+", n, "3"]],
        lambda: ["-", ["*", "2", n], str(2 * r)],
        lambda: ["+", ["-", ["^", n, "3"], ["*", str(r), ["^", n, "2"]]], ["-", n, str(r)]],
        lambda: ["-", ["^", str(q), n], str(q ** min(r, 60))],
        lambda: ["-", ["*", "x", n], ["*", "x", str(r)]],
        lambda: ["-", n, "x"],
        lambda: ["prod", "k", "1", n, ["-", "k", str(r)]],
        lambda: ["sum", "k", "1", "1", ["-", n, str(r)]],
        lambda: ["-", ["fact", n], ["fact", str(min(r, 12))]],
        lambda: ["+", ["-", n, str(r)], ["-", ["^", "-1", n], str((-1) ** r)]],
        lambda: ["*", ["^", "2", n], ["-", n, str(r)]],
        lambda: ["-", ["binom", n, "2"], str(r * (r - 1) // 2)],
    ]
    hidden = [
        lambda: ["sum", "k", str(base), n, ["/", "1", ["-", "k", str(r)]]],
        lambda: ["prod", "k", str(base), n, ["/", ["+", "k", "1"], ["-", "k", str(r)]]],
        lambda: ["sum", "k", str(base), ["*", "2", n], ["/", "k", ["-", "k", str(2 * r - 1)]]],
        lambda: ["sum", "k", str(base), n, ["sum", "j", "1", "k", ["/", "1", ["-", "j", str(r)]]]],
        lambda: ["sum", "k", str(base), n, ["/", "1", ["-", ["^", "2", "k"], str(2 ** min(r, 60))]]],
    ]
    if rnd.random() < 0.6:
        bad = rnd.choice(disguises)()
        piece = ["/", rnd.choice([bad, "1", n]), bad]
    else:
        piece = rnd.choice(hidden)()
    wrap = rnd.choice([lambda t: t, lambda t: ["+", t, n], lambda t: ["*", "2", t], lambda t: ["+", ["sum", "k", "1", n, "k"], t]])
    side = wrap(piece)
    point = [("x", str(r))] if "x" in letters(side) else []
    return {"variable": n, "base": base, "statement": ["=", side, side], "factor": "0", "generic": [],
            "sequence": [], "define": [], "defined": point, "brick": []}


def log_rule():
    """Il logaritmo di un prodotto di potenze, uguagliato a una combinazione
    dei logaritmi dei fattori. Con i coefficienti giusti è vero; con uno
    sbagliato (di segno, o di uno) è falso, e un verificatore che applica
    male una regola dei logaritmi lo accetterebbe.

    Ritorna il certificato e se è vero."""
    n = "n"
    ln = lambda a: ["ln", a]
    factors = [
        ("2", ln("2")), ("3", ln("3")), ("6", ["+", ln("2"), ln("3")]), ("12", ["+", ["*", "2", ln("2")], ln("3")]),
        (["/", "1", "2"], ["neg", ln("2")]), (["/", "3", "4"], ["-", ln("3"), ["*", "2", ln("2")]]),
        (n, ln(n)), (["+", n, "1"], ln(["+", n, "1"])), (["+", ["*", "2", n], "1"], ln(["+", ["*", "2", n], "1"])),
        ("x", ln("x")), (["^", "2", n], ["*", n, ln("2")]), (["^", "6", n], ["*", n, ["+", ln("2"), ln("3")]]),
        (["^", "x", n], ["*", n, ln("x")]), (["fact", n], ln(["fact", n])),
        (["^", ["/", "1", "2"], n], ["neg", ["*", n, ln("2")]]), (["*", "2", "x"], ["+", ln("2"), ln("x")]),
    ]
    # Scritture sbagliate ma plausibili del logaritmo di alcuni fattori.
    wrong = {
        show(["^", "2", n]): [ln("2"), ["+", n, ln("2")], ["*", ln(n), ln("2")], ["^", ln("2"), n]],
        show(["^", "6", n]): [["+", ln("2"), ln("3")], ["*", n, ["*", ln("2"), ln("3")]], ["*", n, ln("5")]],
        show(["^", "x", n]): [ln("x"), ["^", ln("x"), n], ["+", n, ln("x")]],
        show(["fact", n]): [["*", n, ln(n)], ["fact", ln(n)]],
        show(["/", "1", "2"]): [ln("2"), ["/", "1", ln("2")]],
        show(["/", "3", "4"]): [["/", ln("3"), ln("4")], ["-", ln("3"), ln("2")]],
        show(["*", "2", "x"]): [["*", ln("2"), ln("x")], ["*", "2", ln("x")]],
        show("6"): [["*", ln("2"), ln("3")], ln("5")],
        show("12"): [["+", ln("2"), ln("3")], ["*", "2", ["+", ln("2"), ln("3")]]],
        show(["+", n, "1"]): [["+", ln(n), ln("1")], ["+", ln(n), "1"]],
        show(["+", ["*", "2", n], "1"]): [["+", ln("2"), ln(n)], ["*", "2", ln(n)]],
    }
    chosen = rnd.sample(factors, rnd.randint(1, 4))
    exponents = [rnd.choice([-2, -1, 1, 1, 2, 3]) for _ in chosen]
    coefficients = list(exponents)
    logs = [log for _, log in chosen]
    true = rnd.random() < 0.5
    if not true:
        # Una scrittura sbagliata per un fattore che ne ha, oppure un coefficiente sbagliato.
        i = rnd.randrange(len(chosen))
        with_forms = [j for j, (factor, _) in enumerate(chosen) if show(factor) in wrong]
        if with_forms and rnd.random() < 0.7:
            i = rnd.choice(with_forms)
            logs[i] = rnd.choice(wrong[show(chosen[i][0])])
        else:
            coefficients[i] = rnd.choice([-coefficients[i], coefficients[i] + 1, coefficients[i] - 1, 0])
            if coefficients == exponents:
                coefficients[i] += 1
    argument, total = None, None
    for (factor, _), log, e, c in zip(chosen, logs, exponents, coefficients):
        piece = ["^", factor, str(e)]
        argument = piece if argument is None else ["*", argument, piece]
        term = ["*", str(c), log]
        total = term if total is None else ["+", total, term]
    statement = ["=", ln(argument), total] if rnd.random() < 0.5 else ["=", total, ln(argument)]
    cert = {"variable": n, "base": 1, "statement": statement, "factor": "0", "generic": [], "sequence": [], "define": [],
            "defined": None, "brick": [["+", n, "1"], ["+", ["*", "2", n], "1"], n, "x"] if rnd.random() < 0.7 else []}
    return cert, true


def trap():
    """Un certificato falso che passa solo se il verificatore non si accorge
    che il termine di una somma dipende da n.

    Il membro sinistro è la somma di h(k, n) per k fino a n, con la n
    travestita; il destro è la somma di h(j, j). Chi tratta il sinistro come
    una somma qualunque, a cui si aggiunge l'ultimo termine, li trova uguali."""
    n, base = "n", rnd.choice([1, 1, 2])
    if rnd.random() < 0.3:
        # Regole dei logaritmi sbagliate: somiglianti a quelle vere, e false.
        ln = lambda a: ["ln", a]
        m = ["+", n, "1"]
        left, right = rnd.choice([
            (ln(["+", n, "2"]), ["+", ln(n), ln("2")]),
            (ln(["*", "2", n]), ["*", "2", ln(n)]),
            (ln(["^", n, "2"]), ["^", ln(n), "2"]),
            (ln(["*", n, m]), ["*", ln(n), ln(m)]),
            (ln(["/", n, "2"]), ["/", ln(n), ln("2")]),
            (["-", ln(m), ln(n)], ln(["-", m, n])),
            (ln(["^", "2", n]), ["*", "2", ln(n)]),
            (ln(["/", "1", m]), ["/", "1", ln(m)]),
            (["sum", "k", "1", n, ln("k")], ln(["sum", "k", "1", n, "k"])),
            (ln(["fact", n]), ["*", n, ln(n)]),
            (ln(["+", ["^", n, "2"], "1"]), ["*", "2", ln(n)]),
            (ln(["*", "x", n]), ["*", ln("x"), ln(n)]),
            (ln(["^", "x", n]), ["^", ln("x"), n]),
            (ln(["neg", m]), ["neg", ln(m)]),
            (["lnabs", ["-", n, "9"]], ["-", ln(n), ln("9")]),
        ])
        if rnd.random() < 0.5:
            left, right = ["+", left, ["sum", "k", "1", n, "k"]], ["+", right, ["sum", "k", "1", n, "k"]]
        return {"variable": n, "base": base, "statement": ["=", left, right], "factor": rnd.choice(["0", "1"]), "generic": [],
                "sequence": [], "define": [], "defined": None, "brick": [["+", n, str(j)] for j in range(0, 4)]}
    hidden = rnd.choice([
        ("define", ["seq", "G", "k"], "G m (* n m)", lambda j: ["*", j, j]),
        ("define2", ["seq", "G", "k"], "G m (sum i 1 m n)", lambda j: ["*", j, j]),
        ("define3", ["seq", "G", "k"], "G m (+ m (seq F n))", lambda j: ["+", j, ["seq", "F", j]]),
        ("plain", ["*", n, "k"], None, lambda j: ["*", j, j]),
        ("nested", ["sum", "i", "1", "1", ["*", n, "k"]], None, lambda j: ["*", j, j]),
        ("bound", ["*", ["sum", "i", "1", n, "1"], "k"], None, lambda j: ["*", j, j]),
        ("binom", ["*", ["binom", n, "1"], "k"], None, lambda j: ["*", j, j]),
        ("power", ["^", "k", n], None, lambda j: ["^", j, j]),
        ("power2", ["^", n, "k"], None, lambda j: ["^", j, j]),
        ("fib", ["*", ["seq", "F", n], "k"], None, lambda j: ["*", ["seq", "F", j], j]),
        ("fact", ["*", ["fact", n], "k"], None, lambda j: ["*", ["fact", j], j]),
        ("capture", ["sum", "i", "1", "k", ["*", "k", "i"]], None, None),
        ("exp", ["^", "2", ["*", n, "k"]], None, lambda j: ["^", "2", ["*", j, j]]),
        ("seqindex", ["seq", "F", ["+", "k", n]], None, lambda j: ["seq", "F", ["*", "2", j]]),
    ])
    name, body, define, diagonal = hidden
    left = ["sum", "k", str(base), n, body]
    if name == "capture":
        # Somma interna il cui termine dipende dall'indice esterno: a destra una somma diversa.
        right = ["sum", "k", str(base), n, ["*", "k", ["sum", "i", "1", "k", "i"]]]
        left = ["sum", "k", str(base), n, ["+", body, "k"]]
    else:
        right = ["sum", "j", str(base), n, diagonal("j")]
    if rnd.random() < 0.3:
        left, right = ["+", left, "x"], ["+", right, "x"]
    return {"variable": n, "base": base, "statement": ["=", left, right], "factor": "1", "generic": [],
            "sequence": ["F 2 1 1 0 1"], "define": ([define] if define else []), "defined": None, "brick": []}


# ---------------------------------------------------------------- giro
def check(certs, name):
    path = os.path.join(WORK, name)
    with open(path, "w", encoding="utf-8") as out:
        out.write("\n".join(write(c) for c in certs))
    done = subprocess.run([CHECKER, path], capture_output=True, text=True, errors="replace", timeout=600)
    if done.returncode not in (0, 1) or "Sanitizer" in done.stderr or "runtime error" in done.stderr:
        print("CRASH on %s: %s" % (path, done.stderr[-800:]))
        return None
    answers = [l.split(". ", 1)[1] for l in done.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
    if len(answers) != len(certs):
        print("SHAPE: %d answers for %d certificates in %s" % (len(answers), len(certs), path))
        return None
    return answers


def main():
    seeds = []
    for path in sys.argv[5:]:
        seeds += read_certificates(path)
    made = [telescoping() for _ in range(COUNT // 4)]
    certs, origin = [], []
    for cert in made:
        certs.append(cert)
        origin.append("true")
    for _ in range(COUNT // 20):
        certs.append(trap())
        origin.append("trap")
    for _ in range(COUNT // 8):
        certs.append(vanishing())
        origin.append("vanishing")
    for _ in range(COUNT // 6):
        cert, true = log_rule()
        certs.append(cert)
        origin.append("log true" if true else "log false")
    pool = seeds + made
    while len(certs) < COUNT:
        certs.append(mutate(rnd.choice(pool), pool))
        origin.append("mutant")
    stats = {"accepted": 0, "defined accepted": 0, "bug": 0, "strange": 0, "lost": 0, "true accepted": 0, "true unsupported": 0,
             "vanishing rejected": 0, "vanishing unsupported": 0, "log true accepted": 0, "log true not accepted": 0, "log false rejected": 0}
    lost, unsupported = {}, {}
    for start in range(0, len(certs), 500):
        chunk = certs[start:start + 500]
        answers = check(chunk, "checker_%d_%d.cert" % (SEED, start))
        if answers is None:
            stats["bug"] += 1
            continue
        for cert, answer, kind in zip(chunk, answers, origin[start:start + 500]):
            if kind == "log true":
                stats["log true accepted" if answer.startswith("ACCETTATO") else "log true not accepted"] += 1
            if kind == "log false" and answer.startswith("RESPINTO"):
                stats["log false rejected"] += 1
            if kind == "vanishing" and not answer.startswith("ACCETTATO"):
                stats["vanishing rejected" if answer.startswith("RESPINTO") else "vanishing unsupported"] += 1
            if answer.startswith("ACCETTATO"):
                stats["accepted"] += 1
                if cert.get("defined") is not None:
                    stats["defined accepted"] += 1
                if kind == "true":
                    stats["true accepted"] += 1
                try:
                    problem = judge(cert)
                except Strange as strange:
                    stats["strange"] += 1
                    print("STRANGE (%s):\n%s" % (strange, write(cert)))
                    continue
                except (RecursionError, OverflowError, ZeroDivisionError, DecimalException) as error:
                    print("ORACLE ERROR %r:\n%s" % (error, write(cert)))
                    continue
                if problem:
                    stats["bug"] += 1
                    print("BUG: accepted but false at %s\n%s" % (problem, write(cert)))
            elif kind == "true":
                if answer.startswith("RESPINTO"):
                    try:
                        problem = judge(cert)
                    except (Strange, RecursionError):
                        problem = "?"
                    stats["lost"] += 1
                    lost.setdefault(answer, []).append(write(cert))
                else:
                    stats["true unsupported"] += 1
                    unsupported[answer] = unsupported.get(answer, 0) + 1
    for answer, rows in sorted(lost.items(), key=lambda item: -len(item[1]))[:6]:
        print("LOST %d: %s\n%s" % (len(rows), answer, rows[0]))
    if os.environ.get("FUZZ_REASONS"):
        for answer, count in sorted(unsupported.items(), key=lambda item: -item[1]):
            print("TRUE UNSUPPORTED %d: %s" % (count, answer))
    print("seed %d: %d certificates, %s" % (SEED, len(certs), stats))


main()
