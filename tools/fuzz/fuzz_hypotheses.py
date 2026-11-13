"""Fuzzing differenziale delle implicazioni con più ipotesi usate insieme.

Uso: python3 -I fuzz_hypotheses.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Genera enunciati della forma

    \\forall x, y, z \\in A\\ ((H_1 = 0 \\land H_2 = 0) \\implies T = 0)

con una, due o tre ipotesi polinomiali, scritte come uguaglianze tra due
membri. La verità di ognuno è nota per come è costruito, senza chiedere nulla
al programma:

  - veri: T è una combinazione c_1 H_1 + c_2 H_2 delle ipotesi, con
    moltiplicatori polinomiali scelti a caso. Dove le ipotesi valgono, T è
    zero.
  - falsi: le ipotesi sono costruite in modo da annullarsi tutte in un punto
    p scelto a caso nell'insieme, e T è una combinazione delle ipotesi più
    un polinomio che in p non è zero. Il punto p è un controesempio, e
    l'oracolo lo ricalcola con i razionali esatti prima di fidarsene.

BUG: dimostrato un enunciato falso, dichiarato falso un enunciato vero, oppure
un controesempio stampato dal programma che non soddisfa le ipotesi o che
soddisfa la tesi.
"""
import os
import random
import re
import subprocess
import sys
from fractions import Fraction as Fr

MR, SEED, COUNT, WORK = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
rnd = random.Random(SEED)
SETS = {"Z": r"\mathbb{Z}", "Q": r"\mathbb{Q}", "N": r"\mathbb{N}"}


def padd(a, b, scale=1):
    out = dict(a)
    for m, c in b.items():
        out[m] = out.get(m, 0) + scale * c
    return {m: c for m, c in out.items() if c != 0}


def pmul(a, b):
    out = {}
    for m1, c1 in a.items():
        for m2, c2 in b.items():
            m = tuple(sorted(m1 + m2))
            out[m] = out.get(m, 0) + c1 * c2
    return {m: c for m, c in out.items() if c != 0}


def peval(p, env):
    total = Fr(0)
    for mono, c in p.items():
        term = Fr(c)
        for v in mono:
            term *= env[v]
        total += term
    return total


def ptex(p):
    if not p:
        return "0"
    terms = []
    for mono, c in sorted(p.items(), key=lambda t: (-len(t[0]), t[0])):
        letters = "".join(v if mono.count(v) == 1 else "%s^{%d}" % (v, mono.count(v)) for v in sorted(set(mono)))
        size = abs(c)
        terms.append(("-" if c < 0 else "+", (str(size) if (size != 1 or not letters) else "") + letters))
    text = ("-" if terms[0][0] == "-" else "") + terms[0][1]
    for sign, body in terms[1:]:
        text += " %s %s" % (sign, body)
    return text


class Case:
    def __init__(self):
        self.set = rnd.choice(["Z", "Q", "Q", "N"])
        self.vars = list("xyz")[:rnd.choice([2, 2, 3])]
        self.true = rnd.random() < 0.5
        # Il punto in cui le ipotesi si annullano tutte.
        self.point = {v: rnd.randint(1, 4) for v in self.vars}
        self.hyps = [self.vanishing() for _ in range(rnd.choice([1, 2, 2, 3]))]
        self.thesis = {}
        for h in self.hyps:
            self.thesis = padd(self.thesis, pmul(self.small(rnd.choice([0, 0, 1])), h))
        if not self.true:
            extra = self.small(rnd.choice([0, 1]))
            while peval(extra, self.point) == 0:
                extra = padd(extra, {(): 1})
            self.thesis = padd(self.thesis, extra)

    def small(self, degree):
        """Un polinomio a caso di grado al più degree, non nullo."""
        monos = [()] + ([(v,) for v in self.vars] if degree else [])
        p = {}
        for _ in range(rnd.choice([1, 1, 2])):
            p[rnd.choice(monos)] = rnd.choice([-2, -1, 1, 1, 2, 3])
        return p

    def vanishing(self):
        """Un polinomio non nullo che si annulla nel punto: una combinazione delle (v - p_v)."""
        p = {}
        while not p:
            for v in rnd.sample(self.vars, rnd.choice([1, 2])):
                p = padd(p, pmul(self.small(rnd.choice([0, 0, 1])), {(v,): 1, (): -self.point[v]}))
        return p

    @staticmethod
    def equation(p):
        """Scrive p = 0 come uguaglianza tra due membri, spostando a destra una parte dei termini."""
        items = list(p.items())
        rnd.shuffle(items)
        cut = rnd.randint(0, len(items))
        left, right = dict(items[:cut]), {m: -c for m, c in items[cut:]}
        return "%s = %s" % (ptex(left), ptex(right))

    def tex(self):
        hyps = " \\land ".join(self.equation(h) for h in self.hyps)
        return r"\forall %s \in %s\ ((%s) \implies %s)" % (", ".join(self.vars), SETS[self.set], hyps, self.equation(self.thesis))

    def check(self, verdict, text):
        out = []
        if verdict not in ("P", "R"):
            return out
        # L'oracolo ricontrolla la propria costruzione.
        zero = all(peval(h, self.point) == 0 for h in self.hyps)
        if not zero or (peval(self.thesis, self.point) == 0) != self.true:
            return [("ORACLE", "the construction of this case is wrong")]
        if verdict == "P" and not self.true:
            out.append(("BUG", "proved, but the hypotheses hold and the thesis fails at %s" % self.point))
        if verdict == "R":
            if self.true:
                out.append(("BUG", "refuted, but the thesis is a combination of the hypotheses"))
            values = dict((name, Fr(v)) for name, v in re.findall(r"(\w) = (-?\d+(?:/\d+)?)", text.split("controesempio ", 1)[-1]))
            if set(values) == set(self.vars) and (any(peval(h, values) != 0 for h in self.hyps) or peval(self.thesis, values) == 0):
                out.append(("BUG", "the counterexample does not falsify the statement"))
        return out


def main():
    cases = [Case() for _ in range(COUNT)]
    texts = [case.tex() for case in cases]
    path = os.path.join(WORK, "fuzz_hypotheses_%d.tex" % SEED)
    with open(path, "w", encoding="utf-8") as out:
        out.write("\\begin{enumerate}\n")
        for text in texts:
            out.write("\\item $\\displaystyle %s$\n" % text)
        out.write("\\end{enumerate}\n")
    run = subprocess.run([MR, path], capture_output=True, text=True, timeout=3000)
    lines = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
    if len(lines) != len(cases) or run.returncode not in (0, 1):
        print("BUG: the program answered %d of %d statements (exit %d)\n%s" % (len(lines), len(cases), run.returncode, run.stderr[-2000:]))
        return
    tally = {}
    for number, (case, text, line) in enumerate(zip(cases, texts, lines), 1):
        answer = line.split(". ", 1)[1]
        verdict = "P" if answer.startswith("DIMOSTRATO") else "R" if answer.startswith("FALSO") else "N" if answer.startswith("NON DIMOSTRATO") else "U"
        key = ("true" if case.true else "false", verdict)
        tally[key] = tally.get(key, 0) + 1
        for severity, message in case.check(verdict, answer):
            print("%s %d [%s]: %s\n      %s" % (severity, number, answer[:60], message, text))
    print("seed %d:" % SEED, ", ".join("%s %s=%d" % (k[0], k[1], v) for k, v in sorted(tally.items())))


main()
