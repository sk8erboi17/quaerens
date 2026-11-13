"""Fuzzing differenziale delle disuguaglianze dimostrate per induzione.

Uso: python3 -I fuzz_inequalities.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Costruisce disuguaglianze nella variabile intera n: somme confrontate con una
stima (di 1/sqrt(k), di sqrt(k), di 1/k^2, di potenze di k), prodotti
confrontati con una radice, potenze e fattoriali confrontati con polinomi e
tra loro, binomiali centrali, disuguaglianze nella sola n con polinomi e
radici. Da quelle vere ricava varianti insidiose: il verso rovesciato, una
costante ritoccata di poco, il caso base anticipato (vale solo da un n più
grande), il segno stretto dove c'è un'uguaglianza, un termine che è zero
nei primi undici valori e conta dopo, un buco in un solo n lontano.

L'oracolo è indipendente dal programma e non approssima: calcola i due
membri per n = n0, ..., n0 + 59, e poi in qualche n più lontano, come
combinazioni a coefficienti razionali di radici quadrate di interi senza
quadrati (classe Surd), che sono zero solo se lo è ogni coefficiente. Solo
il segno di una combinazione non nulla è deciso con i decimali, a molte
cifre, e se il valore è troppo vicino a zero l'oracolo dice che non sa.

BUG: dimostrato un enunciato che per qualche n calcolato non vale o non è
definito; oppure "falso per n = w" quando in w vale.
SUSPECT: l'oracolo non è riuscito a decidere un segno.
"""
import os
import random
import re
import subprocess
import sys
from decimal import Decimal, getcontext
from fractions import Fraction as Fr

MR, SEED, COUNT, WORK = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
rnd = random.Random(SEED)

SPAN = 60                   # valori consecutivi di n calcolati dal caso base
FAR = (90, 150, 400)        # e alcuni più lontani, se il conto non è troppo lungo


class Undefined(Exception):
    """Un'espressione non è definita: divisione per zero, radice di un negativo."""


class Unsure(Exception):
    """L'oracolo non sa calcolare o decidere: non dice nulla sull'enunciato."""


def squarefree(m):
    """m = s^2 t con t senza quadrati: ritorna (s, t)."""
    s, t, p = 1, 1, 2
    while p * p <= m:
        while m % (p * p) == 0:
            m //= p * p
            s *= p
        if m % p == 0:
            m //= p
            t *= p
        p += 1
    return s, t * m


class Surd:
    """Un numero reale della forma q1 sqrt(d1) + ... + qk sqrt(dk), con i qi
    razionali non nulli e i di interi positivi senza quadrati, tutti diversi
    (d = 1 è la parte razionale). Le radici di interi senza quadrati diversi
    sono linearmente indipendenti sui razionali: il numero è zero se e solo
    se non ha termini."""

    def __init__(self, terms=None):
        self.t = {d: q for d, q in (terms or {}).items() if q != 0}

    @staticmethod
    def of(q):
        return Surd({1: Fr(q)})

    def __add__(self, other):
        out = dict(self.t)
        for d, q in other.t.items():
            out[d] = out.get(d, 0) + q
        return Surd(out)

    def __neg__(self):
        return Surd({d: -q for d, q in self.t.items()})

    def __sub__(self, other):
        return self + (-other)

    def __mul__(self, other):
        out = {}
        for d, q in self.t.items():
            for e, r in other.t.items():
                s, t = squarefree(d * e)
                out[t] = out.get(t, 0) + q * r * s
        return Surd(out)

    def inverse(self):
        """1 / (q sqrt(d)) = sqrt(d) / (q d). Un denominatore con più
        termini l'oracolo non lo tratta."""
        if not self.t:
            raise Undefined
        if len(self.t) != 1:
            raise Unsure
        (d, q), = self.t.items()
        return Surd({d: 1 / (q * d)})

    def rational(self):
        if not self.t:
            return Fr(0)
        if list(self.t) != [1]:
            raise Unsure
        return self.t[1]

    def sqrt(self):
        """La radice di un razionale p/q >= 0 è sqrt(p q) / q."""
        v = self.rational()
        if v < 0:
            raise Undefined
        if v == 0:
            return Surd()
        s, t = squarefree(v.numerator * v.denominator)
        return Surd({t: Fr(s, v.denominator)})

    def sign(self):
        """Il segno: esatto per lo zero e per un termine solo; altrimenti con
        i decimali, a tante cifre quante ne servono per i coefficienti più
        cinquanta, e rinunciando se il valore è troppo vicino a zero."""
        if not self.t:
            return 0
        if len(self.t) == 1:
            return 1 if next(iter(self.t.values())) > 0 else -1
        size = max(len(str(q.numerator)) + len(str(q.denominator)) for q in self.t.values())
        getcontext().prec = size + 80
        total = sum(Decimal(q.numerator) / Decimal(q.denominator) * Decimal(d).sqrt() for d, q in self.t.items())
        biggest = max(abs(Decimal(q.numerator) / Decimal(q.denominator)) * Decimal(d).sqrt() for d, q in self.t.items())
        if abs(total) < biggest * Decimal(10) ** -40:
            raise Unsure
        return 1 if total > 0 else -1


def N(v):
    return ("num", v)


n, k = ("var", "n"), ("var", "k")
add = lambda a, b: ("add", a, b)
sub = lambda a, b: ("sub", a, b)
mul = lambda a, b: ("mul", a, b)
div = lambda a, b: ("div", a, b)
pw = lambda a, e: ("pow", a, e)
sq = lambda a: ("sqrt", a)
S = lambda body, lo=1: ("sum", "k", N(lo), n, body)
P = lambda body, lo=1: ("prod", "k", N(lo), n, body)
n1 = add(n, N(1))


def tex(e, need=0):
    op = e[0]
    if op == "num":
        text, prec = str(e[1]), (4 if e[1] >= 0 else 1)
    elif op == "var":
        text, prec = e[1], 4
    elif op in ("add", "sub"):
        text, prec = "%s %s %s" % (tex(e[1], 1), "+" if op == "add" else "-", tex(e[2], 2)), 1
    elif op == "mul":
        text, prec = r"%s \cdot %s" % (tex(e[1], 2), tex(e[2], 3)), 2
    elif op == "div":
        text, prec = r"\frac{%s}{%s}" % (tex(e[1]), tex(e[2])), 4
    elif op in ("pow", "powe"):
        # Una frazione come base va tra parentesi: \frac{3}{2}^{n} non si legge.
        base = tex(e[1], 5 if e[1][0] == "div" else 4)
        text, prec = "%s^{%s}" % (base, e[2] if op == "pow" else tex(e[2])), 3
    elif op == "sqrt":
        text, prec = r"\sqrt{%s}" % tex(e[1]), 4
    elif op == "fact":
        text, prec = "%s!" % tex(e[1], 4), 3
    elif op == "binom":
        text, prec = r"\binom{%s}{%s}" % (tex(e[1]), tex(e[2])), 4
    elif op in ("sum", "prod"):
        text, prec = r"\%s_{%s=%s}^{%s} %s" % (op, e[1], tex(e[2]), tex(e[3]), tex(e[4], 2)), 1
    else:
        raise ValueError(op)
    return r"\left(%s\right)" % text if prec < need else text


def whole(v):
    q = v.rational()
    if q.denominator != 1:
        raise Unsure
    return q.numerator


def factorial(m):
    if m < 0:
        raise Undefined
    out = 1
    for i in range(2, m + 1):
        out *= i
    return out


def val(e, env):
    """Il valore esatto di e, con le variabili di env."""
    op = e[0]
    if op == "num":
        return Surd.of(e[1])
    if op == "var":
        return Surd.of(env[e[1]])
    if op in ("sum", "prod"):
        lo, hi = whole(val(e[2], env)), whole(val(e[3], env))
        if hi < lo - 1:
            raise Undefined
        total = Surd.of(0 if op == "sum" else 1)
        inner = dict(env)
        for i in range(lo, hi + 1):
            inner[e[1]] = i
            term = val(e[4], inner)
            total = total + term if op == "sum" else total * term
        return total
    a = val(e[1], env)
    if op == "pow" or op == "powe":
        m = e[2] if op == "pow" else whole(val(e[2], env))
        if abs(m) > 4000:
            raise Unsure
        base, out = (a.inverse() if m < 0 else a), Surd.of(1)
        for _ in range(abs(m)):
            out = out * base
        return out
    if op == "sqrt":
        return a.sqrt()
    if op == "fact":
        return Surd.of(factorial(whole(a)))
    b = val(e[2], env)
    if op == "add":
        return a + b
    if op == "sub":
        return a - b
    if op == "mul":
        return a * b
    if op == "div":
        return a * b.inverse()
    if op == "binom":
        top, low = whole(a), whole(b)
        if low < 0 or top < low:
            raise Undefined
        return Surd.of(factorial(top) // (factorial(low) * factorial(top - low)))
    raise ValueError(op)


def holds(lhs, rel, rhs, at):
    """La disuguaglianza in n = at: True o False; solleva Undefined o Unsure."""
    side = (val(lhs, {"n": at}) - val(rhs, {"n": at})).sign()
    return {r"\ge": side >= 0, r"\le": side <= 0, ">": side > 0, "<": side < 0}[rel]


def uses_bigop(e):
    return isinstance(e, tuple) and (e[0] in ("sum", "prod") or any(uses_bigop(c) for c in e[1:]))


def verdict_of_oracle(lhs, rel, rhs, base):
    """('true', None), ('false', n) con il primo n in cui non vale,
    ('undefined', n), oppure ('unsure', n). Calcola n = base, ..., base +
    SPAN - 1 e poi i valori di FAR, questi solo se non ci sono somme o
    prodotti da svolgere (che lì costerebbero troppo)."""
    far = [] if uses_bigop(lhs) or uses_bigop(rhs) else [base + f for f in FAR]
    for at in list(range(base, base + SPAN)) + far:
        try:
            if not holds(lhs, rel, rhs, at):
                return "false", at
        except Undefined:
            return "undefined", at
        except Unsure:
            return "unsure", at
    return "true", None


def falling(first, count):
    """(n - first)(n - first - 1)...: zero per `count` valori consecutivi di n."""
    e = sub(n, N(first))
    for j in range(first + 1, first + count):
        e = mul(e, sub(n, N(j)))
    return e


def true_statement():
    """Una disuguaglianza vera per ogni n dal caso base: (lhs, rel, rhs, base)."""
    c = rnd.randint(1, 4)
    family = [
        # Somme e stime: il termine nuovo si confronta con la crescita della stima.
        (S(div(N(1), sq(k))), r"\ge", sq(n), 1),
        (S(div(N(1), sq(k))), r"\ge", mul(N(2), sub(sq(n1), N(1))), 1),
        (S(div(N(1), sq(k))), r"\le", sub(mul(N(2), sq(n)), N(1)), 1),
        (S(sq(k)), r"\ge", div(mul(mul(N(2), n), sq(n)), N(3)), 1),
        (S(sq(k)), r"\le", div(mul(mul(N(2), n1), sq(n1)), N(3)), 1),
        (S(div(N(1), mul(k, sq(k)))), r"\le", sub(N(3), div(N(2), sq(n))), 1),
        (S(div(N(1), pw(k, 2))), r"\le", sub(N(2), div(N(1), n)), 1),
        (S(div(N(1), pw(k, 2))), r"\le", sub(div(N(7), N(4)), div(N(1), n)), 2),
        (S(div(N(1), pw(k, 3))), r"\le", sub(div(N(3), N(2)), div(N(1), mul(N(2), pw(n, 2)))), 1),
        (S(pw(k, c)), r"\ge", div(pw(n, c + 1), N(c + 1)), 1),
        (S(pw(k, c)), r"\le", div(pw(n1, c + 1), N(c + 1)), 1),
        (S(pw(k, c)), r"\le", pw(n, c + 1), 1),
        (S(mul(k, ("fact", k))), "<", ("fact", n1), 1),
        (S(("fact", k)), r"\le", mul(N(2), ("fact", n)), 1),
        # Prodotti: il fattore nuovo si confronta con il rapporto delle stime.
        (P(div(sub(mul(N(2), k), N(1)), mul(N(2), k))), r"\ge", div(N(1), mul(N(2), sq(n))), 1),
        (P(div(sub(mul(N(2), k), N(1)), mul(N(2), k))), r"\le", div(N(1), sq(add(mul(N(3), n), N(1)))), 1),
        (P(div(sub(mul(N(2), k), N(1)), mul(N(2), k))), "<", div(N(1), sq(add(mul(N(2), n), N(1)))), 1),
        (P(add(N(1), div(N(1), pw(k, 2)))), r"\ge", sub(N(2), div(N(1), n)), 1),
        # Potenze, fattoriali, binomiali.
        (("powe", N(2), n), r"\ge", add(n, N(1)), 0),
        (("powe", N(2), n), ">", n, 0),
        (("powe", N(2), n), r"\ge", pw(n, 2), 4),
        (("powe", N(2), n), ">", pw(n, 2), 5),
        (("powe", N(2), n), r"\ge", pw(n, 3), 10),
        (("powe", N(3), n), r"\ge", pw(n, 3), 1),
        (("powe", N(3), n), r"\ge", add(mul(N(2), pw(n, 2)), N(1)), 1),
        (("powe", N(c + 1), n), r"\ge", add(mul(N(c), n), N(1)), 0),
        (("powe", div(N(c + 2), N(2)), n), r"\ge", add(N(1), div(mul(N(c), n), N(2))), 0),
        (("powe", N(5), n), r"\ge", mul(N(2), ("powe", N(3), n)), 2),
        (("fact", n), r"\ge", ("powe", N(2), sub(n, N(1))), 1),
        (("fact", n), ">", ("powe", N(2), n), 4),
        (("fact", n), r"\ge", ("powe", N(3), n), 7),
        (("fact", mul(N(2), n)), r"\ge", mul(("powe", N(2), n), ("fact", n)), 1),
        (("binom", mul(N(2), n), n), r"\le", ("powe", N(4), n), 0),
        (("binom", mul(N(2), n), n), r"\ge", div(("powe", N(4), n), mul(N(2), n)), 1),
        (("binom", mul(N(2), n), n), r"\ge", div(("powe", N(4), n), add(mul(N(2), n), N(1))), 0),
        # Nella sola n: l'ipotesi induttiva non serve.
        (pw(n, 2), r"\ge", sub(mul(N(2), n), N(1)), 1),
        (add(pw(n, 3), N(2)), r"\ge", mul(N(3), n), 1),
        (sub(sq(n1), sq(n)), r"\le", div(N(1), mul(N(2), sq(n))), 1),
        (sub(sq(n1), sq(n)), ">", div(N(1), mul(N(2), sq(n1))), 1),
        (mul(sub(n, N(c)), sub(n, N(c + 1))), r"\ge", N(0), 1),
    ]
    return rnd.choice(family)


FLIP = {r"\ge": r"\le", r"\le": r"\ge", ">": "<", "<": ">"}
STRICT = {r"\ge": ">", r"\le": "<", ">": r"\ge", "<": r"\le"}


def perturb(lhs, rel, rhs, base):
    """Una variante che può essere falsa, o falsa solo in parte."""
    kind = rnd.choice(["flip", "strict", "early", "early", "late", "const", "scale", "hidden", "hidden", "hole", "gap", "shift"])
    up = rel in (r"\ge", ">")
    c = rnd.choice([1, 2, 3])
    if kind == "flip":
        return lhs, FLIP[rel], rhs, base
    if kind == "strict":
        return lhs, STRICT[rel], rhs, base
    if kind == "early":
        return lhs, rel, rhs, base - rnd.choice([1, 2, 3])
    if kind == "late":
        return lhs, rel, rhs, base + rnd.choice([1, 2, 5])
    if kind == "const":
        # Il membro minore cresce di poco, o di molto.
        extra = div(N(1), N(rnd.choice([1, 10, 1000])))
        return (lhs, rel, add(rhs, extra), base) if up else (add(lhs, extra), rel, rhs, base)
    if kind == "scale":
        factor = div(N(rnd.choice([11, 101, 3])), N(rnd.choice([10, 100, 2])))
        return (lhs, rel, mul(factor, rhs), base) if up else (mul(factor, lhs), rel, rhs, base)
    if kind == "hidden":
        # Un termine nullo nei primi undici valori o più, positivo dopo.
        extra = mul(N(c), falling(base, rnd.choice([11, 12, 14])))
        return (lhs, rel, add(rhs, extra), base) if up else (add(lhs, extra), rel, rhs, base)
    if kind == "hole":
        # Un termine negativo in un solo n, lontano dal caso base.
        far = base + rnd.choice([12, 13, 20])
        extra = mul(N(1000), mul(sub(n, N(far - 1)), sub(n, N(far + 1))))
        return (add(lhs, extra), rel, rhs, base) if up else (lhs, rel, add(rhs, extra), base)
    if kind == "gap":
        # Vero finché è definito, ma in un n lontano c'è una divisione per
        # zero, oppure da lì in poi la radice di un numero negativo.
        far = base + rnd.choice([11, 12, 30])
        hollow = rnd.choice([sub(div(sub(n, N(far)), sub(n, N(far))), N(1)), mul(N(0), sq(sub(N(far), n)))])
        return add(lhs, hollow), rel, rhs, base
    return lhs, rel, at_next(rhs), base


def at_next(e):
    """e con n + 1 al posto di n, fuori dagli estremi di somme e prodotti."""
    if e == n:
        return n1
    if e[0] in ("num", "var"):
        return e
    if e[0] in ("sum", "prod"):
        return e
    return (e[0],) + tuple(at_next(c) if isinstance(c, tuple) else c for c in e[1:])


def main():
    cases = []
    for _ in range(COUNT):
        case = true_statement()
        if rnd.random() < 0.7:
            case = perturb(*case)
        cases.append(case)
    path = os.path.join(WORK, "fuzz_inequalities_%d.tex" % SEED)
    with open(path, "w", encoding="utf-8") as out:
        out.write("\\begin{enumerate}\n")
        for lhs, rel, rhs, base in cases:
            out.write("\\item $\\displaystyle %s %s %s$ for every integer $n\\ge %d$.\n" % (tex(lhs), rel, tex(rhs), base))
        out.write("\\end{enumerate}\n")
    run = subprocess.run([MR, path], capture_output=True, text=True, timeout=3600)
    lines = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
    if len(lines) != len(cases) or run.returncode not in (0, 1):
        print("BUG: the program answered %d of %d statements (exit %d)\n%s" % (len(lines), len(cases), run.returncode, run.stderr[-2000:]))
        return
    tally = {}
    for number, ((lhs, rel, rhs, base), line) in enumerate(zip(cases, lines), 1):
        text = line.split(". ", 1)[1]
        verdict = "P" if text.startswith("DIMOSTRATO") else "R" if text.startswith("FALSO") else "N" if text.startswith("NON DIMOSTRATO") else "U"
        truth, where = verdict_of_oracle(lhs, rel, rhs, base)
        tally[(truth, verdict)] = tally.get((truth, verdict), 0) + 1
        problem = None
        if truth == "unsure":
            problem = "SUSPECT: the oracle cannot decide n = %d" % where
        elif verdict == "P" and truth != "true":
            problem = "BUG: proved, but at n = %d the statement is %s" % (where, "false" if truth == "false" else "not defined")
        elif verdict == "P" and int(re.search(r">= (-?\d+)", text).group(1)) != base:
            problem = "BUG: proved from another base case than the stated n >= %d" % base
        elif verdict == "R":
            w = int(re.search(r"= (-?\d+)$", text).group(1))
            try:
                if w < base or holds(lhs, rel, rhs, w):
                    problem = "BUG: refuted at n = %d, but the statement holds there (or n is below the base case)" % w
            except Undefined:
                problem = None
            except Unsure:
                problem = "SUSPECT: refuted at n = %d, where the oracle cannot decide" % w
        if verdict in "NU" and truth == "true" and not problem:
            problem = "NOTE: holds wherever the oracle computed it, but not proved"
        if problem:
            print("%s\n   %d [%s]: %s %s %s, n >= %d" % (problem, number, text[:70], tex(lhs), rel, tex(rhs), base))
    print("seed %d:" % SEED, ", ".join("%s/%s=%d" % (k[0], k[1], v) for k, v in sorted(tally.items())))


main()
