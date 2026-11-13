"""Fuzzing differenziale delle relazioni date da una regola.

Uso: python3 -I fuzz_relations.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Genera enunciati della forma

    R = \\{(x, y) \\in A \\times B \\mid c\\} \\implies tesi

con c un'equazione tra polinomi, un'appartenenza come x - y \\in \\mathbb{Z},
un "esiste k" o una disgiunzione, e la tesi una congiunzione di proprietà,
ognuna affermata o negata: riflessiva, simmetrica, transitiva, antisimmetrica,
totale, "è il grafico di un'applicazione" (con \\exists!), l'appartenenza di
una coppia di numeri o di una coppia di espressioni. Li fa decidere al
programma e confronta ogni verdetto con un oracolo scritto qui, che non
chiama mai il programma e non svolge nessuna definizione: per sapere se una
coppia di numeri sta in R calcola la condizione c su quei due numeri, dopo
aver guardato se stanno in A e in B.

  - Una proprietà "per ogni" si prova su una finestra di numeri: un
    controesempio è certo, la sua assenza no. I quantificatori che l'enunciato
    scrive senza insieme qui variano su tutti i razionali della finestra: è la
    lettura alla lettera, quella che il programma deve rispettare.
  - L'appartenenza di una coppia di numeri è un conto, quindi è certa.
  - "Per ogni x esiste un solo y": per ogni x della finestra gli y si contano
    in modo esatto, con le radici razionali dell'equazione in y.
  - Un controesempio stampato dal programma viene ricalcolato qui, con la
    definizione: se smentisce una delle proprietà affermate il verdetto FALSO
    è confermato, e se la tesi è fatta solo di "per ogni" e non ne smentisce
    nessuna è un errore certo.

Le lettere degli enunciati sono scelte apposta per scontrarsi con quelle
della regola (la k di "esiste k" compare anche come variabile della tesi) e
con i nomi degli insiemi numerici (una variabile che si chiama Z, accanto a
\\mathbb{Z}): una sostituzione che catturasse una lettera, o che cambiasse
il nome di un insieme, darebbe un verdetto sbagliato. Il secondo caso è
successo: con le lettere di prima questo script non lo vedeva.
Una parte degli enunciati ha un quantificatore senza insieme che la
condizione non obbliga: il programma deve rifiutarli.

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
    divisors = lambda n: [d for d in range(1, abs(n) + 1) if n % d == 0]
    for p in divisors(c[0]):
        for q in divisors(c[-1]):
            for sign in (1, -1):
                r = Fr(sign * p, q)
                if sum(k * r ** i for i, k in enumerate(c)) == 0:
                    roots.add(r)
    return roots


class Poly:
    """Un polinomio in due lettere, di grado al più due in ciascuna: {(i, j): coefficiente}."""

    def __init__(self, terms):
        self.terms = {m: Fr(c) for m, c in terms.items() if c != 0}

    @staticmethod
    def random(quadratic):
        monos = [(0, 0), (1, 0), (0, 1)] + ([(2, 0), (0, 2), (1, 1)] if quadratic else [])
        terms = {}
        for _ in range(rnd.choice([1, 2, 2, 3])):
            terms[rnd.choice(monos)] = rnd.choice([-3, -2, -1, 1, 1, 1, 2, 3, 4])
        return Poly(terms)

    def value(self, x, y):
        return sum(c * x ** i * y ** j for (i, j), c in self.terms.items())

    def minus(self, other):
        terms = dict(self.terms)
        for m, c in other.terms.items():
            terms[m] = terms.get(m, 0) - c
        return Poly(terms)

    def in_second(self, x):
        """I coefficienti, dal grado 0, del polinomio nella seconda lettera quando la prima vale x."""
        c = [Fr(0)] * 3
        for (i, j), k in self.terms.items():
            c[j] += k * x ** i
        return c

    def tex(self, x, y):
        if not self.terms:
            return "0"
        parts = []
        for (i, j), c in sorted(self.terms.items(), key=lambda t: (-(t[0][0] + t[0][1]), -t[0][0])):
            letters = "".join(v if e == 1 else "%s^{%d}" % (v, e) for v, e in ((x, i), (y, j)) if e)
            size = abs(c)
            parts.append(("-" if c < 0 else "+", (str(size) if (size != 1 or not letters) else "") + letters))
        text = ("-" if parts[0][0] == "-" else "") + parts[0][1]
        for sign, body in parts[1:]:
            text += " %s %s" % (sign, body)
        return text


class Case:
    def __init__(self):
        self.A = rnd.choice(["N", "N0", "Z", "Z", "Q", "Q+"])
        self.B = self.A if rnd.random() < 0.7 else rnd.choice(["N", "N0", "Z", "Q"])
        # Anche lettere che sono nomi di insiemi numerici: la Z della regola non è la Z di \mathbb{Z}.
        self.x, self.y = rnd.choice([("x", "y"), ("a", "b"), ("u", "v"), ("Z", "y"), ("N", "Q")])
        self.name = rnd.choice("RGS")
        self.kind = rnd.choice(["eq", "eq", "eq", "in", "ex", "or"])
        quadratic = rnd.random() < 0.4
        self.p, self.q = Poly.random(quadratic), Poly.random(quadratic)
        self.p2, self.q2 = Poly.random(False), Poly.random(False)
        self.target = rnd.choice(["N0", "Z", "N"])
        self.modulus = rnd.choice([2, 3, 4])
        self.multiple = rnd.random() < 0.5
        self.claims = [self.random_claim() for _ in range(rnd.choice([1, 1, 2, 2, 3]))]
        self.unguarded = rnd.random() < 0.06

    def random_claim(self):
        name = rnd.choice(["refl", "irrefl", "sym", "trans", "antisym", "total", "func", "pair", "pair", "shift"])
        if name == "func" and self.kind != "eq":
            name = "sym"
        data = None
        if name == "pair":
            data = (rnd.randint(-3, 4), rnd.randint(-3, 4), rnd.random() < 0.3)
        if name == "shift":
            data = (rnd.randint(-2, 2), rnd.choice([1, 1, 2, -1]), rnd.randint(-2, 2), rnd.choice([1, 2, -1]))
        # Lettere scelte per scontrarsi con quelle della regola.
        letters = rnd.choice([("x", "y", "z"), ("k", "y", "x"), ("y", "x", "k"), ("b", "a", "k"), ("m", "k", "n"), ("N", "Z", "Q"), ("Z", "N", "k")])
        return name, rnd.random() < 0.4, data, letters

    # --- Oracolo: la relazione calcolata dalla sua definizione -------------
    def condition(self, x, y):
        if self.kind == "eq":
            return self.p.value(x, y) == self.q.value(x, y)
        if self.kind == "or":
            return self.p.value(x, y) == self.q.value(x, y) or self.p2.value(x, y) == self.q2.value(x, y)
        if self.kind == "in":
            return in_set(self.target, self.p.value(x, y))
        if self.multiple:                      # esiste k intero con y = k x
            return y == 0 if x == 0 else (y / x).denominator == 1
        return ((x - y) / self.modulus).denominator == 1   # esiste k intero con x - y = m k

    def related(self, x, y):
        return in_set(self.A, x) and in_set(self.B, y) and self.condition(x, y)

    def images(self, x):
        """Esatto, per una condizione che è un'equazione: gli y di B con (x, y) nella relazione."""
        roots = rational_roots(self.p.minus(self.q).in_second(x))
        return None if roots is None else [r for r in roots if in_set(self.B, r)]

    def truth(self, claim):
        """Ritorna (vero?, certo?) per la proprietà, non negata."""
        name, _, data, _ = claim
        everything = sorted(set(window("Q", 3)) | set(window("Z", 9)))
        R = self.related
        if name == "pair":
            return R(Fr(data[0]), Fr(data[1])) != data[2], True
        if name == "refl":
            return all(R(x, x) for x in window(self.A, 8)), False
        if name == "irrefl":
            return not any(R(x, x) for x in everything), False
        if name == "sym":
            return all(R(y, x) for x in everything for y in everything if R(x, y)), False
        if name == "antisym":
            return all(x == y for x in everything for y in everything if R(x, y) and R(y, x)), False
        if name == "trans":
            pairs = [(x, y) for x in everything for y in everything if R(x, y)]
            return all(R(x, z) for x, y in pairs for z in everything if R(y, z)), False
        if name == "total":
            w = window(self.A, 5)
            return all(R(x, y) or R(y, x) for x in w for y in w), False
        if name == "shift":
            return all(R(data[0] + data[1] * t, data[2] + data[3] * t) for t in window(self.A, 8)), False
        for x in window(self.A, 8):        # func
            found = self.images(x)
            if found is None or len(found) != 1:
                return False, True
        return True, False

    def falsified_by(self, claim, values):
        """Esatto: i numeri di values, uno per lettera, smentiscono il "per ogni" di claim?"""
        name, _, data, letters = claim
        R, inA = self.related, lambda t: in_set(self.A, t)
        if name in ("pair", "func") or any(l not in values for l in letters[:{"refl": 1, "irrefl": 1, "shift": 1, "trans": 3}.get(name, 2)]):
            return False
        u, v, w = (values.get(l) for l in letters)
        if name == "refl":
            return inA(u) and not R(u, u)
        if name == "irrefl":
            return R(u, u)
        if name == "sym":
            return R(u, v) and not R(v, u)
        if name == "antisym":
            return R(u, v) and R(v, u) and u != v
        if name == "trans":
            return R(u, v) and R(v, w) and not R(u, w)
        if name == "total":
            return inA(u) and inA(v) and not R(u, v) and not R(v, u)
        return inA(u) and not R(data[0] + data[1] * u, data[2] + data[3] * u)   # shift

    # --- Scrittura ---------------------------------------------------------
    def rule_tex(self):
        x, y = self.x, self.y
        if self.kind == "eq":
            return "%s = %s" % (self.p.tex(x, y), self.q.tex(x, y))
        if self.kind == "or":
            return r"%s = %s \lor %s = %s" % (self.p.tex(x, y), self.q.tex(x, y), self.p2.tex(x, y), self.q2.tex(x, y))
        if self.kind == "in":
            return r"%s \in %s" % (self.p.tex(x, y), SETS[self.target])
        if self.multiple:
            return r"\exists k \in \mathbb{Z}\ (%s = k%s)" % (y, x)
        return r"\exists k \in \mathbb{Z}\ (%s - %s = %dk)" % (x, y, self.modulus)

    def claim_tex(self, claim):
        name, _, data, (u, v, w) = claim
        A, B, R = SETS[self.A], SETS[self.B], self.name
        if name == "pair":
            return r"(%d, %d) %s %s" % (data[0], data[1], r"\notin" if data[2] else r"\in", R)
        if name == "refl":
            return r"\forall %s \in %s\ ((%s, %s) \in %s)" % (u, A, u, u, R)
        if name == "irrefl":
            return r"\forall %s\ ((%s, %s) \notin %s)" % (u, u, u, R)
        if name == "sym":
            return r"\forall %s, %s\ ((%s, %s) \in %s \implies (%s, %s) \in %s)" % (u, v, u, v, R, v, u, R)
        if name == "antisym":
            return r"\forall %s, %s\ (((%s, %s) \in %s \land (%s, %s) \in %s) \implies %s = %s)" % (u, v, u, v, R, v, u, R, u, v)
        if name == "trans":
            return r"\forall %s, %s, %s\ (((%s, %s) \in %s \land (%s, %s) \in %s) \implies (%s, %s) \in %s)" % (u, v, w, u, v, R, v, w, R, u, w, R)
        if name == "total":
            return r"\forall %s, %s \in %s\ ((%s, %s) \in %s \lor (%s, %s) \in %s)" % (u, v, A, u, v, R, v, u, R)
        if name == "shift":
            linear = lambda c, m: ("%d%s" % (m, u) if m not in (1, -1) else ("-" if m < 0 else "") + u) + ("" if c == 0 else " %s %d" % ("+" if c > 0 else "-", abs(c)))
            return r"\forall %s \in %s\ ((%s, %s) \in %s)" % (u, A, linear(data[0], data[1]), linear(data[2], data[3]), R)
        return r"\forall %s \in %s\ \exists! %s \in %s\ ((%s, %s) \in %s)" % (u, A, v, B, u, v, R)

    def tex(self):
        A, B = SETS[self.A], SETS[self.B]
        # Il quadrato solo dove non c'è già un apice: \mathbb{Q}^{+}^{2} non è LaTeX.
        pairs = "%s^{2}" % A if (A == B and "^" not in A and rnd.random() < 0.5) else r"%s \times %s" % (A, B)
        if self.unguarded:
            thesis = rnd.choice([r"\forall x, y\ ((x, y) \in %s \lor (y, x) \in %s)", r"\forall x\ ((x, x) \in %s \land x = x)",
                                 r"\exists x, y\ ((x, y) \in %s \lor x = y)", r"\exists x\ ((x, x) \notin %s)"]).replace("%s", self.name)
        else:
            thesis = " \\land ".join((r"\lnot (%s)" if c[1] else "(%s)") % self.claim_tex(c) for c in self.claims)
        return r"%s = \{(%s, %s) \in %s \mid %s\} \implies (%s)" % (self.name, self.x, self.y, pairs, self.rule_tex(), thesis)

    def check(self, verdict, detail):
        out = []
        if verdict not in ("P", "R"):
            return out
        if self.unguarded:
            return [("BUG", "a quantifier without a set, not forced by the condition, was decided instead of being refused")]

        # Un controesempio stampato dal programma si ricalcola qui: se smentisce
        # davvero una delle proprietà affermate, il verdetto FALSO è giusto.
        values = dict((name, Fr(v)) for name, v in re.findall(r"(\w) = (-?\d+(?:/\d+)?)", detail or ""))
        if verdict == "R" and values:
            if any(not c[1] and self.falsified_by(c, values) for c in self.claims):
                return out
            if all(c[0] not in ("pair", "func") and not c[1] for c in self.claims):
                return [("BUG", "the counterexample %s does not falsify any claim" % detail)]
        values = []
        for claim in self.claims:
            value, certain = self.truth(claim)
            # Di un "per ogni" è certo solo il controesempio.
            values.append((claim, value != claim[1], certain or not value))
        if verdict == "P":
            for claim, value, certain in values:
                if not value:
                    out.append(("BUG" if certain else "SUSPECT", "proved, but %s%s is %s" % ("not " if claim[1] else "", claim[0], "false" if certain else "not confirmed on the window")))
        elif all(value for _, value, _ in values):
            certain = all(c for _, _, c in values)
            out.append(("BUG" if certain else "SUSPECT", "refuted, but every claim is true%s" % ("" if certain else " on the window")))
        return out


def main():
    cases = [Case() for _ in range(COUNT)]
    texts = [case.tex() for case in cases]
    path = os.path.join(WORK, "fuzz_relations_%d.tex" % SEED)
    with open(path, "w", encoding="utf-8") as out:
        out.write("\\begin{enumerate}\n")
        for text in texts:
            out.write("\\item $\\displaystyle %s$\n" % text)
        out.write("\\end{enumerate}\n")
    run = subprocess.run([MR, path], capture_output=True, text=True, timeout=1800)
    lines = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
    if len(lines) != len(cases) or run.returncode not in (0, 1):
        print("BUG: the program answered %d of %d statements (exit %d)\n%s" % (len(lines), len(cases), run.returncode, run.stderr[-2000:]))
        return
    tally = {}
    for number, (case, text, line) in enumerate(zip(cases, texts, lines), 1):
        answer = line.split(". ", 1)[1]
        verdict = "P" if answer.startswith("DIMOSTRATO") else "R" if answer.startswith("FALSO") else "N" if answer.startswith("NON DIMOSTRATO") else "U"
        detail = answer.split("controesempio ", 1)[1] if "controesempio " in answer else None
        key = ("unguarded" if case.unguarded else case.kind, verdict)
        tally[key] = tally.get(key, 0) + 1
        for severity, message in case.check(verdict, detail):
            print("%s %d [%s]: %s\n      %s" % (severity, number, answer[:60], message, text))
    print("seed %d:" % SEED, ", ".join("%s %s=%d" % (k[0], k[1], v) for k, v in sorted(tally.items())))


main()
