"""Fuzzing differenziale delle operazioni binarie dichiarate con una regola.

Uso: python3 -I fuzz_operations.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Genera enunciati della forma

    (\\ast \\colon A \\times A \\to A \\land \\forall a, b \\in A\\ (a \\ast b = p(a, b))) \\implies tesi

con p un polinomio di secondo grado a coefficienti piccoli (a volte diviso per
2 o per 3) e la tesi una congiunzione di proprietà, ognuna affermata o negata:
associativa, commutativa, idempotente, con elemento neutro a sinistra o a
destra, regolare a sinistra. Li fa decidere al programma e confronta ogni
verdetto con un oracolo scritto qui, che non chiama mai il programma:

  - un'identità tra polinomi su un insieme infinito vale se e solo se vale su
    una griglia con più punti del grado, per ogni variabile: associatività,
    commutatività e idempotenza si decidono così, in modo esatto;
  - "esiste e tale che e * a = a per ogni a" vuol dire che i coefficienti di
    p(e, a) - a, come polinomio in a, sono tutti zero: sono condizioni su e di
    grado al più due, e le loro radici razionali si trovano tutte;
  - la regolarità si prova su una finestra di numeri: un controesempio è certo,
    la sua assenza no;
  - che la regola abbia valori nell'insieme si prova sulla finestra: un valore
    fuori è certo, e allora il programma non deve né dimostrare né smentire.

Una parte degli enunciati è scritta apposta in modo ambiguo (a * b * c, oppure
a + b * c, senza parentesi): il programma deve rifiutarli, non sceglierne una
lettura.

Stampa BUG quando il verdetto contraddice un fatto certo, SUSPECT quando
contraddice solo ciò che si vede sulla finestra.
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

SETS = {"N": r"\mathbb{N}", "N0": r"\mathbb{N}_{0}", "Z": r"\mathbb{Z}", "Q": r"\mathbb{Q}", "Q+": r"\mathbb{Q}^{+}"}
SYMBOLS = ["ast", "star", "circ", "bullet", "diamond", "perp", "oplus", "otimes", "odot"]


def in_set(kind, v):
    v = Fr(v)
    if kind in ("N", "N0", "Z") and v.denominator != 1:
        return False
    return {"N": v >= 1, "N0": v >= 0, "Z": True, "Q": True, "Q+": v > 0}[kind]


def window(kind, size):
    """Elementi dell'insieme: interi in [-size, size], razionali con denominatore fino a 3."""
    dens = (1,) if kind in ("N", "N0", "Z") else (1, 2, 3)
    return sorted({Fr(p, q) for q in dens for p in range(-size * q, size * q + 1) if in_set(kind, Fr(p, q))})


def rational_roots(c):
    """Radici razionali del polinomio di coefficienti c (dal grado 0); None se è il polinomio nullo."""
    c = [Fr(k) for k in c]
    while c and c[-1] == 0:
        c.pop()
    if not c:
        return None
    scale = 1
    for k in c:
        scale = scale * k.denominator // math.gcd(scale, k.denominator)
    c = [int(k * scale) for k in c]
    roots = set()
    while len(c) > 1 and c[0] == 0:
        roots.add(Fr(0))
        c = c[1:]
    if len(c) == 1:
        return roots

    def divisors(n):
        n = abs(n)
        return [d for d in range(1, n + 1) if n % d == 0]

    for p in divisors(c[0]):
        for q in divisors(c[-1]):
            for sign in (1, -1):
                r = Fr(sign * p, q)
                if sum(k * r ** i for i, k in enumerate(c)) == 0:
                    roots.add(r)
    return roots


def term_tex(coef, mono):
    """Un termine con il suo segno: ('+' o '-', testo senza segno)."""
    size = abs(coef)
    if size.denominator != 1:
        body = r"\frac{%d}{%d}" % (size.numerator, size.denominator) + mono
    else:
        body = (str(size.numerator) if (size != 1 or not mono) else "") + mono
    return ("-" if coef < 0 else "+", body)


class Case:
    """Un'operazione su un insieme, con una tesi fatta di proprietà."""
    MONOS = ["", "a", "b", "ab", "a^{2}", "b^{2}"]

    def __init__(self):
        self.A = rnd.choice(["N", "N0", "Z", "Z", "Q", "Q", "Q+"])
        self.sym = rnd.choice(SYMBOLS)
        small = lambda: rnd.choice([-2, -1, -1, 0, 0, 0, 1, 1, 1, 2, 3, 4])
        self.k = [Fr(small()) for _ in range(6)]
        if rnd.random() < 0.6:
            self.k[4] = self.k[5] = Fr(0)
        if rnd.random() < 0.15:
            d = rnd.choice([2, 3])
            self.k = [c / d for c in self.k]
        if not any(self.k[1:]):
            self.k[1] = Fr(1)
        names = ["assoc", "comm", "idem", "lneutral", "rneutral", "lreg"]
        self.claims = [(rnd.choice(names), rnd.random() < 0.4) for _ in range(rnd.choice([1, 1, 2, 2, 3]))]
        self.trap = rnd.choice(["chain", "mixed"]) if rnd.random() < 0.08 else None

    # --- Oracolo -----------------------------------------------------------
    def op(self, a, b):
        k = self.k
        return k[0] + k[1] * a + k[2] * b + k[3] * a * b + k[4] * a * a + k[5] * b * b

    def not_closed(self):
        w = window(self.A, 8)
        for a in w:
            for b in w:
                if not in_set(self.A, self.op(a, b)):
                    return a, b
        return None

    def neutral(self, left):
        """Esatto: esiste e in A con e * a = a (left) o a * e = a per ogni a di A."""
        k = self.k
        # p(e, a) - a come polinomio in a: i tre coefficienti, polinomi in e.
        if left:
            square, linear, constant = k[5], (k[2] - 1, k[3]), (k[0], k[1], k[4])
        else:
            square, linear, constant = k[4], (k[1] - 1, k[3]), (k[0], k[2], k[5])
        if square != 0:
            return False
        candidates = None
        for poly in (list(linear), list(constant)):
            roots = rational_roots(poly)
            if roots is None:
                continue
            candidates = roots if candidates is None else candidates & roots
        if candidates is None:
            return True  # ogni e va bene, e A non è vuoto
        return any(in_set(self.A, e) for e in candidates)

    def truth(self, name):
        """Ritorna (vero?, certo?) per la proprietà name, non negata."""
        grid = window(self.A, 8)[:7]
        if name == "assoc":
            return all(self.op(self.op(a, b), c) == self.op(a, self.op(b, c)) for a in grid for b in grid for c in grid), True
        if name == "comm":
            return all(self.op(a, b) == self.op(b, a) for a in grid for b in grid), True
        if name == "idem":
            return all(self.op(a, a) == a for a in grid), True
        if name in ("lneutral", "rneutral"):
            return self.neutral(name == "lneutral"), True
        w = window(self.A, 6)
        for a in w:
            seen = {}
            for x in w:
                v = self.op(a, x)
                if v in seen:
                    return False, True
                seen[v] = x
        return True, False

    # --- Scrittura ---------------------------------------------------------
    def rule_tex(self):
        terms = [term_tex(c, m) for c, m in zip(self.k, self.MONOS) if c != 0]
        text = ("-" if terms[0][0] == "-" else "") + terms[0][1]
        for sign, body in terms[1:]:
            text += " %s %s" % (sign, body)
        return text

    def claim_tex(self, name):
        A, o = SETS[self.A], "\\" + self.sym
        return {
            "assoc": r"\forall a, b, c \in %s\ ((a %s b) %s c = a %s (b %s c))" % (A, o, o, o, o),
            "comm": r"\forall a, b \in %s\ (a %s b = b %s a)" % (A, o, o),
            "idem": r"\forall a \in %s\ (a %s a = a)" % (A, o),
            "lneutral": r"\exists e \in %s\ \forall a \in %s\ (e %s a = a)" % (A, A, o),
            "rneutral": r"\exists e \in %s\ \forall a \in %s\ (a %s e = a)" % (A, A, o),
            "lreg": r"\forall a, x, y \in %s\ (a %s x = a %s y \implies x = y)" % (A, o, o),
        }[name]

    def tex(self):
        A, o = SETS[self.A], "\\" + self.sym
        if self.trap == "chain":
            thesis = r"\forall a, b, c \in %s\ (a %s b %s c = a %s (b %s c))" % (A, o, o, o, o)
        elif self.trap == "mixed":
            thesis = r"\forall a, b, c \in %s\ (a + b %s c = a + (b %s c))" % (A, o, o)
        else:
            thesis = " \\land ".join((r"\lnot (%s)" if neg else "(%s)") % self.claim_tex(name) for name, neg in self.claims)
        return r"(%s \colon %s \times %s \to %s \land \forall a, b \in %s\ (a %s b = %s)) \implies (%s)" % (
            o, A, A, A, A, o, self.rule_tex(), thesis)

    def check(self, verdict):
        out = []
        decided = verdict in ("P", "R")
        if self.trap:
            if decided:
                out.append(("BUG", "an ambiguous writing was decided instead of being refused"))
            return out
        bad = self.not_closed()
        if bad is not None:
            if decided:
                out.append(("BUG", "the rule leaves the set at (a, b) = (%s, %s) but the verdict is %s" % (bad[0], bad[1], verdict)))
            return out
        if not decided:
            return out
        values = []
        for name, neg in self.claims:
            value, certain = self.truth(name)
            values.append((name, neg, value != neg, certain))
        if verdict == "P":
            for name, neg, value, certain in values:
                if not value:
                    out.append(("BUG" if certain else "SUSPECT", "proved, but %s%s is %s" % ("not " if neg else "", name, "false" if certain else "not confirmed on the window")))
        elif all(value for _, _, value, _ in values):
            certain = all(c for _, _, _, c in values)
            out.append(("BUG" if certain else "SUSPECT", "refuted, but every claim is true%s" % ("" if certain else " on the window")))
        return out


def main():
    cases = [Case() for _ in range(COUNT)]
    path = os.path.join(WORK, "fuzz_operations_%d.tex" % SEED)
    with open(path, "w", encoding="utf-8") as out:
        out.write("\\begin{enumerate}\n")
        for case in cases:
            out.write("\\item $\\displaystyle %s$\n" % case.tex())
        out.write("\\end{enumerate}\n")
    run = subprocess.run([MR, path], capture_output=True, text=True, timeout=1800)
    lines = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
    if len(lines) != len(cases) or run.returncode not in (0, 1):
        print("BUG: the program answered %d of %d statements (exit %d)\n%s" % (len(lines), len(cases), run.returncode, run.stderr[-2000:]))
        return
    tally = {}
    for number, (case, line) in enumerate(zip(cases, lines), 1):
        text = line.split(". ", 1)[1]
        verdict = "P" if text.startswith("DIMOSTRATO") else "R" if text.startswith("FALSO") else "N" if text.startswith("NON DIMOSTRATO") else "U"
        key = ("trap" if case.trap else "op", verdict)
        tally[key] = tally.get(key, 0) + 1
        for severity, message in case.check(verdict):
            print("%s %d [%s]: %s\n      %s" % (severity, number, text[:60], message, case.tex()))
    print("seed %d:" % SEED, ", ".join("%s %s=%d" % (k[0], k[1], v) for k, v in sorted(tally.items())))


main()
