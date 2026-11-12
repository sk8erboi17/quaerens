"""Fuzzing differenziale delle uguaglianze dimostrate per induzione.

Uso: python3 -I fuzz_induction.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Costruisce enunciati veri (somme e prodotti telescopici, formule chiuse note,
identità con seni, radici, logaritmi, arcotangenti) e ne ricava varianti false
insidiose: errori che si annullano nei primi n, anche oltre i dieci valori in
cui il programma cerca un controesempio; n scambiato con n + 1; estremi
spostati. L'oracolo è numerico e indipendente dal programma: calcola i due
membri per n = 1..16, in modo esatto dove può, e decide da solo se
l'enunciato è vero.

Una parte degli enunciati viene poi resa "non definita più avanti": i due
membri restano uguali finché esistono, ma da un certo n, oltre quelli che il
programma calcola con i numeri, compare una divisione per zero, un logaritmo
di un numero non positivo o una radice di un numero negativo.

BUG: dimostrato un enunciato che l'oracolo trova falso, o che per qualche n
fino a 45 non è definito; oppure "falso per n = w" quando in w i due membri
coincidono.
"""
import math
import os
import random
import re
import subprocess
import sys
from fractions import Fraction as Fr

MR, SEED, COUNT, WORK = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
rnd = random.Random(SEED)

# Con la variabile d'ambiente FUZZ_WRAP ogni enunciato senza parametri viene
# scritto con il quantificatore, "per ogni n in N (...)": così passa dalle
# regole sui quantificatori, che chiamano l'induzione come una loro regola.
WRAP = bool(os.environ.get("FUZZ_WRAP"))


class Undefined(Exception):
    """Un'espressione non è definita: divisione per zero, logaritmo, radice."""


class TooBig(Exception):
    """L'oracolo non sa calcolare: numeri troppo grandi. Non dice nulla."""


def N(v):
    return ("num", v)


n, k, x = ("var", "n"), ("var", "k"), ("var", "x")
add = lambda a, b: ("add", a, b)
sub = lambda a, b: ("sub", a, b)
mul = lambda a, b: ("mul", a, b)
div = lambda a, b: ("div", a, b)
pw = lambda a, e: ("pow", a, e)

PREC = {"add": 1, "sub": 1, "neg": 1, "sum": 1, "prod": 1, "mul": 2, "pow": 3, "powe": 3, "fact": 3}


def tex(e, need=0):
    op = e[0]
    if op == "num":
        text, prec = str(e[1]), (4 if e[1] >= 0 else 1)
    elif op == "var":
        text, prec = e[1], 4
    elif op == "pi":
        text, prec = r"\pi", 4
    elif op in ("add", "sub"):
        text, prec = "%s %s %s" % (tex(e[1], 1), "+" if op == "add" else "-", tex(e[2], 2)), 1
    elif op == "neg":
        text, prec = "-%s" % tex(e[1], 2), 1
    elif op == "mul":
        text, prec = r"%s \cdot %s" % (tex(e[1], 2), tex(e[2], 3)), 2
    elif op == "div":
        text, prec = r"\frac{%s}{%s}" % (tex(e[1]), tex(e[2])), 4
    elif op == "pow":
        text, prec = "%s^{%d}" % (tex(e[1], 4), e[2]), 3
    elif op == "powe":
        text, prec = "%s^{%s}" % (tex(e[1], 4), tex(e[2])), 3
    elif op == "sqrt":
        text, prec = r"\sqrt{%s}" % tex(e[1]), 4
    elif op == "root":
        text, prec = r"\sqrt[%d]{%s}" % (e[2], tex(e[1])), 4
    elif op in ("ln", "arctan", "sin", "cos"):
        text, prec = r"\%s\left(%s\right)" % (op, tex(e[1])), 4
    elif op == "fact":
        text, prec = "%s!" % tex(e[1], 4), 3
    elif op == "binom":
        text, prec = r"\binom{%s}{%s}" % (tex(e[1]), tex(e[2])), 4
    elif op == "fib":
        text, prec = "F_{%s}" % tex(e[1]), 4
    elif op == "harm":
        text, prec = "H_{%s}" % tex(e[1]), 4
    elif op in ("sum", "prod"):
        text, prec = r"\%s_{%s=%s}^{%s} %s" % (op, e[1], tex(e[2]), tex(e[3]), tex(e[4], 2)), 1
    else:
        raise ValueError(op)
    return r"\left(%s\right)" % text if prec < need else text


def whole(v):
    if isinstance(v, Fr) and v.denominator == 1:
        return v.numerator
    raise Undefined


def val(e, env):
    op = e[0]
    if op == "num":
        return Fr(e[1])
    if op == "var":
        return env[e[1]]
    if op == "pi":
        return math.pi
    if op in ("sum", "prod"):
        lo, hi = whole(val(e[2], env)), whole(val(e[3], env))
        total = Fr(0) if op == "sum" else Fr(1)
        inner = dict(env)
        for i in range(lo, hi + 1):
            inner[e[1]] = Fr(i)
            term = val(e[4], inner)
            total = total + term if op == "sum" else total * term
        return total
    a = val(e[1], env)
    try:
        if op == "neg":
            return -a
        if op == "pow":
            return a ** e[2]
        if op == "sqrt":
            if a < 0:
                raise Undefined
            return math.sqrt(a)
        if op == "root":
            if a < 0:
                raise Undefined
            return float(a) ** (1.0 / e[2])
        if op == "ln":
            if a <= 0:
                raise Undefined
            return math.log(a)
        if op == "arctan":
            return math.atan(a)
        if op == "sin":
            return math.sin(a)
        if op == "cos":
            return math.cos(a)
        if op == "fact":
            i = whole(a)
            if i < 0:
                raise Undefined
            if i > 400:
                raise TooBig
            return Fr(math.factorial(i))
        if op == "fib":
            # Per indici negativi vale la stessa ricorrenza, letta all'indietro:
            # F(-m) = (-1)^(m+1) F(m). È la convenzione usuale, e quella del programma.
            i = whole(a)
            if abs(i) > 2000:
                raise TooBig
            p, q = 0, 1
            for _ in range(abs(i)):
                p, q = q, p + q
            return Fr(p if i >= 0 or i % 2 else -p)
        if op == "harm":
            return sum((Fr(1, j) for j in range(1, whole(a) + 1)), Fr(0))
        b = val(e[2], env)
        if op == "add":
            return a + b
        if op == "sub":
            return a - b
        if op == "mul":
            return a * b
        if op == "div":
            if b == 0:
                raise Undefined
            return a / b
        if op == "powe":
            if isinstance(b, Fr) and b.denominator == 1 and isinstance(a, Fr):
                if a == 0 and b < 0:
                    raise Undefined
                return a ** b.numerator
            if a <= 0:
                raise Undefined
            return float(a) ** float(b)
        if op == "binom":
            r = whole(b)
            if r < 0 or r > 40:
                raise Undefined
            out = Fr(1)
            for j in range(1, r + 1):
                out = out * (a - r + j) / j
            return out
    except ZeroDivisionError:
        raise Undefined
    except OverflowError:
        raise TooBig
    raise ValueError(op)


def same(l, r):
    if isinstance(l, Fr) and isinstance(r, Fr):
        return l == r
    l, r = float(l), float(r)
    return abs(l - r) <= 1e-9 * max(1.0, abs(l), abs(r))


def uses(e, name):
    return e == ("var", name) or any(isinstance(c, tuple) and uses(c, name) for c in e[1:])


XS = [0.7, 1.9, -1.3]


def truth(lhs, rhs, first=1, last=16):
    """(vero?, primo n in cui i membri differiscono o None, almeno un n definito?)"""
    xs = XS if uses(lhs, "x") or uses(rhs, "x") else [0.0]
    defined = False
    for i in range(first, last + 1):
        for xv in xs:
            env = {"n": Fr(i), "x": xv}
            try:
                l, r = val(lhs, env), val(rhs, env)
            except (Undefined, TooBig):
                continue
            defined = True
            if not same(l, r):
                return False, i, True
    return True, None, defined


def first_undefined(lhs, rhs, first, last=45):
    """Il primo n in [first, last] in cui l'enunciato non è definito per nessuno
    dei valori di x dell'oracolo, o None. Un n che l'oracolo non sa calcolare
    non conta."""
    xs = XS if uses(lhs, "x") or uses(rhs, "x") else [0.0]
    for i in range(first, last + 1):
        missing = 0
        for xv in xs:
            env = {"n": Fr(i), "x": xv}
            try:
                val(lhs, env), val(rhs, env)
            except Undefined:
                missing += 1
            except TooBig:
                pass
        if missing == len(xs):
            return i
    return None


def break_later(lhs, rhs):
    """Rende l'enunciato non definito da un certo n in poi, oltre i primi undici,
    oppure (varianti innocue) lo cambia senza toglierne la definizione."""
    c = rnd.randint(12, 40)
    kind = rnd.choice(["quotient", "quotient", "square", "double", "log", "root", "sum", "prod", "power", "odd", "plus", "logplus",
                       "mixed", "mixed", "mixedfine", "twopowers"])
    k = ("var", "k")
    if kind == "quotient":
        g = sub(n, N(c))
    elif kind == "square":
        g = sub(pw(n, 2), N(c * c))
    elif kind == "double":
        g = mul(sub(n, N(c)), add(n, N(3)))
    elif kind == "power":
        c = rnd.randint(12, 20)
        g = sub(("powe", N(2), n), N(2 ** c))
    elif kind == "mixed":
        # 2^n - n - (2^c - c): si annulla per n = c, e solo lì da n = 1 in poi.
        c = rnd.randint(12, 20)
        g = sub(sub(("powe", N(2), n), n), N(2 ** c - c))
    elif kind == "mixedfine":
        # 2^n - n e 3^n - 2 n: mai nulli.
        g = rnd.choice([sub(("powe", N(2), n), n), sub(("powe", N(3), n), mul(N(2), n))])
    elif kind == "twopowers":
        # 3^n - 2^n - (3^c - 2^c): si annulla per n = c.
        c = rnd.randint(12, 16)
        g = sub(sub(("powe", N(3), n), ("powe", N(2), n)), N(3 ** c - 2 ** c))
    elif kind == "odd":
        g = sub(mul(N(2), n), N(2 * c + 1))
    elif kind == "plus":
        g = add(pw(n, 2), N(c))
    elif kind == "log":
        e = ("ln", sub(N(c), n))
        return lhs, add(rhs, sub(e, e))
    elif kind == "logplus":
        e = ("ln", add(N(c), n))
        return lhs, add(rhs, sub(e, e))
    elif kind == "root":
        e = ("sqrt", sub(N(c), n))
        return add(lhs, e), add(rhs, e)
    elif kind == "sum":
        e = ("sum", "k", N(1), n, div(sub(k, N(c)), sub(k, N(c))))
        return add(lhs, e), add(rhs, n)
    else:
        e = ("prod", "k", N(1), n, div(sub(k, N(c)), sub(k, N(c))))
        return mul(lhs, e), rhs
    return (mul(div(g, g), lhs), rhs) if rnd.random() < 0.5 else (lhs, mul(rhs, div(g, g)))


def at(e, target):
    """e con n sostituito da target."""
    if e == n:
        return target
    return tuple(at(c, target) if isinstance(c, tuple) else c for c in e)


# --- Funzioni g(indice) da cui nascono le somme telescopiche --------------------
def shifted(e, a):
    return e if a == 0 else add(e, N(a))


def g_family():
    a, c = rnd.choice([0, 1, 1, 2, 3]), rnd.choice([2, 3, 5])
    choices = [
        lambda e: add(mul(N(rnd.choice([1, 2, 3])), pw(e, 2)), e),
        lambda e: pw(e, 3),
        lambda e: div(N(1), shifted(e, a + 1)),
        lambda e: div(e, shifted(e, a + 1)),
        lambda e: ("powe", N(c), e),
        lambda e: mul(e, ("powe", N(c), e)),
        lambda e: mul(pw(e, 2), ("powe", N(2), e)),
        lambda e: ("fact", e),
        lambda e: mul(e, ("fact", e)),
        lambda e: ("fact", shifted(e, 1)),
        lambda e: ("sqrt", shifted(e, a)),
        lambda e: mul(e, ("sqrt", shifted(e, a + 1))),
        lambda e: ("ln", shifted(e, a + 1)),
        lambda e: ("arctan", shifted(e, a)),
        lambda e: ("fib", e),
        lambda e: mul(("fib", e), ("fib", shifted(e, 1))),
        lambda e: ("sin", mul(e, x)),
        lambda e: ("cos", mul(e, x)),
        lambda e: ("binom", shifted(e, a + 1), N(rnd.choice([2, 3]))),
        lambda e: div(N(1), ("fact", e)),
        lambda e: div(("powe", N(c), e), ("fact", e)),
        lambda e: mul(("powe", ("neg", N(1)), e), e),
        lambda e: ("harm", e),
    ]
    first, second = rnd.choice(choices), rnd.choice(choices)
    if rnd.random() < 0.25:
        return lambda e: add(first(e), second(e))
    return first


def h_family():
    a = rnd.choice([1, 1, 2, 3])
    return rnd.choice([
        lambda e: add(e, N(a)),
        lambda e: add(pw(e, 2), N(a)),
        lambda e: ("powe", N(rnd.choice([2, 3])), e),
        lambda e: ("fact", add(e, N(1))),
        lambda e: ("sqrt", add(e, N(a))),
        lambda e: mul(add(e, N(1)), add(e, N(a + 1))),
    ])


def closed_forms():
    a, b, r = rnd.choice([1, 2, 3, 5]), rnd.choice([0, 1, 4]), rnd.choice([2, 3, 4])
    S = lambda body, lo=1, hi=n: ("sum", "k", N(lo), hi, body)
    P = lambda body, lo=1, hi=n: ("prod", "k", N(lo), hi, body)
    n1 = add(n, N(1))
    return [
        (S(add(mul(N(a), k), N(b))), add(div(mul(N(a), mul(n, n1)), N(2)), mul(N(b), n))),
        (S(pw(k, 2)), div(mul(n, mul(n1, add(mul(N(2), n), N(1)))), N(6))),
        (S(pw(k, 3)), pw(div(mul(n, n1), N(2)), 2)),
        (S(("powe", N(r), k)), div(sub(("powe", N(r), n1), N(r)), N(r - 1))),
        (S(("powe", N(r), k), 0), div(sub(("powe", N(r), n1), N(1)), N(r - 1))),
        (S(mul(k, ("powe", N(2), k))), add(mul(sub(n, N(1)), ("powe", N(2), n1)), N(2))),
        (S(div(N(1), mul(k, add(k, N(1))))), div(n, n1)),
        (S(("fib", k)), sub(("fib", add(n, N(2))), N(1))),
        (S(pw(("fib", k), 2)), mul(("fib", n), ("fib", n1))),
        (S(("fib", sub(mul(N(2), k), N(1)))), ("fib", mul(N(2), n))),
        (S(mul(k, ("fact", k))), sub(("fact", n1), N(1))),
        (S(("binom", k, N(2))), ("binom", n1, N(3))),
        (P(add(N(1), div(N(1), k))), n1),
        (P(sub(N(1), div(N(1), add(k, N(1))))), div(N(1), n1)),
        (S(sub(mul(N(2), k), N(1))), pw(n, 2)),
        (S(mul(("powe", ("neg", N(1)), k), k)), div(sub(mul(("powe", ("neg", N(1)), n), add(mul(N(2), n), N(1))), N(1)), N(4))),
        (S(div(N(1), k)), ("harm", n)),
        (("fact", n1), mul(n1, ("fact", n))),
        (("cos", mul(mul(N(2), n), x)), sub(N(1), mul(N(2), pw(("sin", mul(n, x)), 2)))),
        (("sin", mul(mul(N(2), n), x)), mul(N(2), mul(("sin", mul(n, x)), ("cos", mul(n, x))))),
        (add(pw(("sin", mul(n, x)), 2), pw(("cos", mul(n, x)), 2)), N(1)),
        (("sqrt", mul(pw(n, 2), n1)), mul(n, ("sqrt", n1))),
        (("sqrt", add(mul(N(4), n), N(4))), mul(N(2), ("sqrt", n1))),
        (("root", mul(pw(n, 3), n1), 3), mul(n, ("root", n1, 3))),
        (("ln", mul(n, n1)), add(("ln", n), ("ln", n1))),
        (("ln", pw(n, 2)), mul(N(2), ("ln", n))),
        (("ln", div(n1, n)), sub(("ln", n1), ("ln", n))),
        (("arctan", div(N(1), add(add(pw(n, 2), n), N(1)))), sub(("arctan", n1), ("arctan", n))),
        (add(("arctan", n), ("arctan", div(N(1), n))), div(("pi",), N(2))),
        (S(("arctan", div(N(1), add(add(pw(k, 2), k), N(1))))), sub(("arctan", n1), div(("pi",), N(4)))),
        (S(("ln", add(N(1), div(N(1), k)))), ("ln", n1)),
        (S(div(N(1), add(("sqrt", k), ("sqrt", add(k, N(1)))))), sub(("sqrt", n1), N(1))),
    ]


def true_statement():
    shape = rnd.random()
    if shape < 0.4:
        g = g_family()
        if rnd.random() < 0.5:
            return ("sum", "k", N(1), n, sub(g(k), g(sub(k, N(1))))), sub(g(n), g(N(0)))
        return ("sum", "k", N(1), n, sub(g(add(k, N(1))), g(k))), sub(g(add(n, N(1))), g(N(1)))
    if shape < 0.5:
        h = h_family()
        return ("prod", "k", N(1), n, div(h(k), h(sub(k, N(1))))), div(h(n), h(N(0)))
    return rnd.choice(closed_forms())


def falling(m):
    """(n - 1)(n - 2)...(n - m): zero per n = 1..m."""
    e = sub(n, N(1))
    for j in range(2, m + 1):
        e = mul(e, sub(n, N(j)))
    return e


def perturb(lhs, rhs):
    c = N(rnd.choice([1, 2, 3, 7]))
    kind = rnd.choice(["const", "linear", "fall2", "fall3", "fall11", "fall12", "fall13", "scale", "shift", "upper", "parity",
                       "binom", "lower", "swap"])
    if kind == "const":
        return lhs, add(rhs, c)
    if kind == "linear":
        return lhs, add(rhs, mul(c, sub(n, N(1))))
    if kind.startswith("fall"):
        return lhs, add(rhs, mul(c, falling(int(kind[4:]))))
    if kind == "scale":
        return lhs, mul(rhs, add(N(1), mul(c, falling(2))))
    if kind == "shift":
        return lhs, at(rhs, add(n, N(1)))
    if kind == "upper" and lhs[0] in ("sum", "prod"):
        return (lhs[0], lhs[1], lhs[2], rnd.choice([add(n, N(1)), mul(N(2), n), sub(n, N(1))]), lhs[4]), rhs
    if kind == "lower" and lhs[0] in ("sum", "prod"):
        return (lhs[0], lhs[1], N(lhs[2][1] + rnd.choice([1, -1])), lhs[3], lhs[4]), rhs
    if kind == "parity":
        return lhs, add(rhs, mul(c, add(("powe", ("neg", N(1)), n), N(1))))
    if kind == "binom":
        return lhs, add(rhs, mul(c, ("binom", sub(n, N(1)), N(rnd.choice([2, 11, 12])))))
    if kind == "swap":
        return rhs, add(lhs, mul(c, falling(rnd.choice([1, 11]))))
    return lhs, sub(rhs, c)


def main():
    cases = []
    for _ in range(COUNT):
        lhs, rhs = true_statement()
        shape = rnd.random()
        if shape < 0.45:
            lhs, rhs = perturb(lhs, rhs)
        elif shape < 0.75:
            lhs, rhs = break_later(lhs, rhs)
        if WRAP and (uses(lhs, "x") or uses(rhs, "x")):
            continue
        cases.append((lhs, rhs))
    path = os.path.join(WORK, "fuzz_induction_%s%d.tex" % ("wrap_" if WRAP else "", SEED))
    with open(path, "w", encoding="utf-8") as out:
        out.write("\\begin{enumerate}\n")
        for lhs, rhs in cases:
            if WRAP:
                out.write("\\item $\\displaystyle \\forall n \\in \\mathbb{N}\\ \\left(%s = %s\\right)$\n" % (tex(lhs), tex(rhs)))
            else:
                out.write("\\item $\\displaystyle %s = %s$\n" % (tex(lhs), tex(rhs)))
        out.write("\\end{enumerate}\n")
    run = subprocess.run([MR, path], capture_output=True, text=True, timeout=1800)
    lines = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
    if len(lines) != len(cases) or run.returncode not in (0, 1):
        print("BUG: the program answered %d of %d statements (exit %d)\n%s" % (len(lines), len(cases), run.returncode, run.stderr[-2000:]))
        return
    tally = {}
    for number, ((lhs, rhs), line) in enumerate(zip(cases, lines), 1):
        text = line.split(". ", 1)[1]
        verdict = "P" if text.startswith("DIMOSTRATO") else "R" if text.startswith("FALSO") else "N" if text.startswith("NON DIMOSTRATO") else "U"
        ok, where, defined = truth(lhs, rhs)
        gap = first_undefined(lhs, rhs, 1)
        key = ("false" if not ok else "undefined later" if gap else "true", verdict)
        tally[key] = tally.get(key, 0) + 1
        problem = None
        if verdict == "R" and WRAP and not re.search(r"n = (-?\d+)$", text):
            verdict = "r"
        if verdict == "P":
            base = 1 if WRAP else int(re.search(r">= (-?\d+)", text).group(1))
            ok_from_base, where_from_base, _ = truth(lhs, rhs, max(base, 1))
            gap_from_base = first_undefined(lhs, rhs, max(base, 1))
            if not ok_from_base:
                problem = "BUG: proved from n >= %d, but the two sides differ at n = %d" % (base, where_from_base)
            elif gap_from_base:
                problem = "BUG: proved from n >= %d, but the statement is not defined at n = %d" % (base, gap_from_base)
        elif verdict == "R":
            w = int(re.search(r"= (-?\d+)$", text).group(1))
            differs, _, was_defined = truth(lhs, rhs, w, w)
            if differs and was_defined:
                problem = "BUG: refuted at n = %d, but the two sides coincide there" % w
            elif not was_defined:
                problem = "NOTE: refuted at n = %d, where the oracle cannot compute a side" % w
        if verdict == "N" and ok and not gap and not problem:
            problem = "NOTE: true and defined up to n = 45, but not proved"
        if problem:
            print("%s\n   %d [%s]: %s = %s" % (problem, number, text[:70], tex(lhs), tex(rhs)))
    print("seed %d:" % SEED, ", ".join("%s/%s=%d" % (k[0], k[1], v) for k, v in sorted(tally.items())))


main()
