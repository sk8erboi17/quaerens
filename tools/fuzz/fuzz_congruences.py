"""Fuzzing differenziale degli enunciati fatti di congruenze con moduli fissati.

Uso: python3 -I fuzz_congruences.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Genera enunciati con una, due o tre variabili intere, quantificate con "per
ogni" o "esiste" su Z, N, N0 o su uno di questi meno un punto, la cui
condizione è una combinazione di

    m \\mid P        m \\nmid P        P \\equiv Q \\pmod{m}
    \\exists k \\in \\mathbb{Z}\\ (P = mk + c)        P \\in \\mathbb{Z}

con P e Q polinomi a coefficienti interi (a volte con un coefficiente
frazionario) e m un numero. Una parte sono richieste di soluzioni,
\\{x \\in \\mathbb{Z} \\mid condizione\\}.

L'oracolo è scritto qui e non chiama mai il programma. Non riduce nulla
modulo m: calcola ogni condizione con i razionali esatti di Python su interi
veri, e per ogni variabile prova due periodi interi consecutivi dell'insieme,
dove il periodo è il minimo comune multiplo dei moduli e dei denominatori.
Poiché una condizione di questo tipo dipende solo dal resto modulo il
periodo, e ogni insieme contiene interi di ogni resto, il conto dell'oracolo
è completo: la sua risposta è certa. Come controllo di sé stesso l'oracolo
rifà il conto su una finestra spostata, e avverte (ORACLE) se le due non
sono d'accordo.

BUG: dimostrato un enunciato falso, dichiarato falso un enunciato vero, un
controesempio che non lo è, oppure soluzioni diverse da quelle vere.
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

SETS = [("Z", None), ("Z", None), ("N", None), ("N0", None), ("Z", 0), ("N", 1), ("Z", 2)]
SET_TEX = {"Z": r"\mathbb{Z}", "N": r"\mathbb{N}", "N0": r"\mathbb{N}_{0}"}
MONOS = [(), ("x",), ("y",), ("z",), ("x", "x"), ("x", "y"), ("y", "y"), ("x", "z"), ("x", "x", "x"), ("x", "y", "y")]


def lcm(a, b):
    return a * b // math.gcd(a, b)


def set_has(s, v):
    kind, hole = s
    return (kind == "Z" or v >= (1 if kind == "N" else 0)) and v != hole


def set_tex(s):
    return SET_TEX[s[0]] + ("" if s[1] is None else r" \setminus \{%d\}" % s[1])


def set_window(s, start, size):
    """size elementi consecutivi dell'insieme, a partire dal primo non minore di start."""
    out, v = [], start
    while len(out) < size:
        if set_has(s, v):
            out.append(v)
        v += 1
    return out


class Case:
    def __init__(self):
        self.solve = rnd.random() < 0.2
        count = 1 if self.solve else rnd.choice([1, 1, 2, 2, 3])
        self.vars = list("xyz")[:count]
        self.sets = [rnd.choice(SETS) for _ in self.vars]
        self.quant = [rnd.choice("AAE") for _ in self.vars]
        # Moduli piccoli quando le variabili sono tante: l'oracolo prova (2 M)^n casi.
        self.moduli = {1: [2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 15, 16, 18, 20, 24, 36], 2: [2, 3, 4, 5, 6, 8, 9, 10, 12], 3: [2, 3, 4, 6]}[count]
        self.base = rnd.choice(self.moduli)
        self.matrix = self.random_matrix(rnd.choice([0, 1, 1, 2]))
        self.whole_period = self.period()

    def modulus(self):
        """Per lo più divisori o multipli di uno stesso numero, così il periodo resta piccolo."""
        friends = [m for m in self.moduli if m % self.base == 0 or self.base % m == 0]
        return rnd.choice(friends)

    def random_poly(self, fraction=False):
        monos = [m for m in MONOS if all(v in self.vars for v in m)]
        poly = {}
        for _ in range(rnd.choice([1, 2, 2, 3])):
            poly[rnd.choice(monos)] = Fr(rnd.choice([-5, -3, -2, -1, 1, 1, 2, 3, 4, 7, 11]))
        if fraction:
            key = rnd.choice(list(poly))
            poly[key] = poly[key] / rnd.choice([2, 3])
        return poly

    def random_matrix(self, depth):
        if depth == 0:
            kind = rnd.choice(["mid", "mid", "nmid", "equiv", "equiv", "exists", "integer"])
            fraction = rnd.random() < 0.1
            if kind == "exists":
                return ("exists", self.random_poly(), self.modulus(), rnd.randint(-3, 9))
            if kind == "integer":
                return ("integer", self.random_poly(True))
            if kind == "equiv":
                return ("equiv", self.random_poly(fraction), self.random_poly(), self.modulus())
            return (kind, self.modulus(), self.random_poly(fraction))
        op = rnd.choice(["and", "or", "imp", "iff", "not"])
        if op == "not":
            return ("not", self.random_matrix(depth - 1))
        return (op, self.random_matrix(depth - 1), self.random_matrix(depth - 1))

    # --- Oracolo -------------------------------------------------------------
    @staticmethod
    def value(poly, env):
        total = Fr(0)
        for mono, c in poly.items():
            term = c
            for v in mono:
                term *= env[v]
            total += term
        return total

    def holds(self, m, env):
        kind = m[0]
        if kind in ("mid", "nmid"):
            return ((self.value(m[2], env) / m[1]).denominator == 1) == (kind == "mid")
        if kind == "equiv":
            return ((self.value(m[1], env) - self.value(m[2], env)) / m[3]).denominator == 1
        if kind == "exists":        # esiste k intero con P = m k + c
            return ((self.value(m[1], env) - m[3]) / m[2]).denominator == 1
        if kind == "integer":
            return self.value(m[1], env).denominator == 1
        if kind == "not":
            return not self.holds(m[1], env)
        a = self.holds(m[1], env)
        if kind == "and":
            return a and self.holds(m[2], env)
        if kind == "or":
            return a or self.holds(m[2], env)
        if kind == "imp":
            return (not a) or self.holds(m[2], env)
        return a == self.holds(m[2], env)

    def period(self, m=None):
        """Un multiplo comune dei moduli e dei denominatori dei coefficienti."""
        m = m or self.matrix
        kind = m[0]
        if kind == "not":
            return self.period(m[1])
        if kind in ("and", "or", "imp", "iff"):
            return lcm(self.period(m[1]), self.period(m[2]))
        if kind in ("mid", "nmid"):
            modulus, polys = m[1], [m[2]]
        elif kind == "equiv":
            modulus, polys = m[3], [m[1], m[2]]
        elif kind == "exists":
            modulus, polys = m[2], [m[1]]
        else:
            modulus, polys = 1, [m[1]]
        for poly in polys:
            for c in poly.values():
                modulus *= c.denominator
        return modulus

    def truth(self, shift, level=0, env=None):
        """Verità dell'enunciato, provando per ogni variabile due periodi a partire da shift."""
        env = env or {}
        if level == len(self.vars):
            return self.holds(self.matrix, env)
        universal = self.quant[level] == "A"
        for v in set_window(self.sets[level], shift, 2 * self.whole_period):
            env[self.vars[level]] = v
            if self.truth(shift, level + 1, env) != universal:
                return not universal
        return universal

    # --- Scrittura -----------------------------------------------------------
    @staticmethod
    def poly_tex(poly):
        terms = []
        for mono, c in sorted(poly.items(), key=lambda t: (-len(t[0]), t[0])):
            letters = "".join(v if mono.count(v) == 1 else "%s^{%d}" % (v, mono.count(v)) for v in sorted(set(mono)))
            size = abs(c)
            if size.denominator != 1:
                body = r"\frac{%d}{%d}" % (size.numerator, size.denominator) + letters
            else:
                body = (str(size.numerator) if (size != 1 or not letters) else "") + letters
            terms.append(("-" if c < 0 else "+", body))
        text = ("-" if terms[0][0] == "-" else "") + terms[0][1]
        for sign, body in terms[1:]:
            text += " %s %s" % (sign, body)
        return text

    def matrix_tex(self, m):
        kind = m[0]
        if kind in ("mid", "nmid"):
            return r"%d \%s %s" % (m[1], kind, self.poly_tex(m[2]))
        if kind == "equiv":
            return r"%s \equiv %s \pmod{%d}" % (self.poly_tex(m[1]), self.poly_tex(m[2]), m[3])
        if kind == "exists":
            tail = "" if m[3] == 0 else " %s %d" % ("+" if m[3] > 0 else "-", abs(m[3]))
            return r"\exists k \in \mathbb{Z}\ (%s = %dk%s)" % (self.poly_tex(m[1]), m[2], tail)
        if kind == "integer":
            return r"%s \in \mathbb{Z}" % self.poly_tex(m[1])
        if kind == "not":
            return r"\lnot (%s)" % self.matrix_tex(m[1])
        op = {"and": r"\land", "or": r"\lor", "imp": r"\implies", "iff": r"\iff"}[kind]
        return "(%s) %s (%s)" % (self.matrix_tex(m[1]), op, self.matrix_tex(m[2]))

    def tex(self):
        if self.solve:
            return r"\{x \in %s \mid %s\}" % (set_tex(self.sets[0]), self.matrix_tex(self.matrix))
        prefix = "".join(r"%s %s \in %s\ " % (r"\forall" if q == "A" else r"\exists", v, set_tex(s))
                         for v, q, s in zip(self.vars, self.quant, self.sets))
        return prefix + "(" + self.matrix_tex(self.matrix) + ")"

    def check_solutions(self, answer):
        """Confronta le soluzioni scritte dal programma con la condizione, intero per intero."""
        classes = [(int(r), int(d)) for r, d in re.findall(r"x \\equiv (-?\d+) \\pmod\{(\d+)\}", answer)]
        everything, nothing = answer.startswith(r"\forall"), answer.startswith(r"\nexists")
        if not classes and not everything and not nothing:
            return [("BUG", "the solutions cannot be read: %s" % answer)]
        span = 2 * self.period()
        for d in [c[1] for c in classes]:
            span = lcm(span, d)
        for v in set_window(self.sets[0], -span, 2 * span):
            said = everything or any((v - r) % d == 0 for r, d in classes)
            if said != self.holds(self.matrix, {"x": v}):
                return [("BUG", "x = %d is %s a solution, but the answer is %s" % (v, "not" if said else "", answer))]
        return []

    def check(self, verdict, text):
        out = []
        if self.solve:
            if verdict == "S":
                return self.check_solutions(text.split(": ", 1)[1])
            return [("BUG", "a solution set was answered with a truth value")] if verdict in ("P", "R") else out
        if verdict not in ("P", "R"):
            return out
        truth = self.truth(-3)
        if truth != self.truth(5):
            out.append(("ORACLE", "the two windows of the oracle disagree"))
        if truth != (verdict == "P"):
            out.append(("BUG", "the statement is %s but the verdict is %s" % (truth, verdict)))
        # Un controesempio di un enunciato di soli "per ogni" si ricalcola.
        if verdict == "R" and "controesempio " in text and set(self.quant) == {"A"}:
            values = dict((name, int(v)) for name, v in re.findall(r"(\w) = (-?\d+)", text.split("controesempio ", 1)[1]))
            if set(values) == set(self.vars):
                inside = all(set_has(s, values[v]) for v, s in zip(self.vars, self.sets))
                if not inside or self.holds(self.matrix, values):
                    out.append(("BUG", "the counterexample does not falsify the statement"))
        return out


def main():
    cases = [Case() for _ in range(COUNT)]
    path = os.path.join(WORK, "fuzz_congruences_%d.tex" % SEED)
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
        verdict = ("P" if text.startswith("DIMOSTRATO") else "R" if text.startswith("FALSO") else "S" if text.startswith("RISOLTO")
                   else "N" if text.startswith("NON DIMOSTRATO") else "U")
        key = ("solve" if case.solve else "%d var" % len(case.vars), verdict)
        tally[key] = tally.get(key, 0) + 1
        if verdict in ("N", "U"):
            print("NOTE %d: %s\n      %s" % (number, text[:100], case.tex()))
        for severity, message in case.check(verdict, text):
            print("%s %d [%s]: %s\n      %s" % (severity, number, text[:60], message, case.tex()))
    print("seed %d:" % SEED, ", ".join("%s %s=%d" % (k[0], k[1], v) for k, v in sorted(tally.items())))


main()
