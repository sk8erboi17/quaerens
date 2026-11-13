"""Fuzzing differenziale delle ipotesi "esiste k" e dei moduli che sono lettere (regola 18u).

Uso: python3 -I fuzz_witnesses.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Genera enunciati della forma

    \\forall n \\in \\mathbb{N}\\ \\forall a, b, c, d \\in \\mathbb{Z}\\ ((H_1 \\land H_2) \\implies T)

dove ipotesi e tesi dicono che un polinomio è un multiplo intero di un altro,
oppure che è un numero intero, scritte in uno dei modi che il programma
legge: a \\equiv b \\pmod{n}, n \\mid a - b, \\exists k \\in \\mathbb{Z}\\ (a - b = kn),
x - y \\in \\mathbb{Z}. Una volta su tre le ipotesi e la tesi usano tutte la
stessa lettera per il loro testimone. Le famiglie:

  - congruenze modulo una lettera: da a \\equiv b e c \\equiv d segue
    P(a, c) \\equiv P(b, d) per ogni polinomio P a coefficienti interi; le
    varianti sbagliate cambiano un coefficiente, il modulo, o tolgono
    un'ipotesi;
  - catene di divisibilità: m \\mid n e n \\mid a danno m \\mid a, e non il
    contrario;
  - "la differenza è un intero" tra razionali: compatibile con la somma, non
    con il prodotto;
  - la cancellazione con l'identità di Bézout: da ac \\equiv bc \\pmod{m} e
    uc + vm = 1 segue a \\equiv b \\pmod{m}; senza la seconda ipotesi no.

L'oracolo non usa nessun ragionamento del programma: dà alle lettere tutti i
valori di una finestra di numeri e decide ogni condizione con una divisione
esatta tra razionali. Un valore che soddisfa le ipotesi e non la tesi è un
controesempio certo. Gli enunciati costruiti per essere veri lo sono per un
teorema (le congruenze rispettano somma e prodotto), e la finestra lo
ricontrolla. Gli altri, nel riepilogo "other", sono varianti sbagliate di
poco: quasi tutte false, ma qualcuna resta vera (un coefficiente e il modulo
raddoppiati insieme), e di loro si sa solo ciò che dice la finestra.

BUG: dimostrato un enunciato di cui la finestra ha un controesempio, oppure
dichiarato falso un enunciato vero per costruzione. SUSPECT: dichiarato falso
un enunciato di cui la finestra non trova controesempi, senza che sia vero
per costruzione: va guardato a mano.
"""
import itertools
import os
import random
import re
import subprocess
import sys
from fractions import Fraction as Fr

MR, SEED, COUNT, WORK = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
rnd = random.Random(SEED)
INTEGERS = [0, 1, -1, 2, -2, 3, -3, 4, 5, 6]
NATURALS = [1, 2, 3, 4, 5, 6]
RATIONALS = [Fr(0), Fr(1), Fr(-1), Fr(1, 2), Fr(-1, 2), Fr(3, 2), Fr(2), Fr(1, 3), Fr(-2, 3)]
WITNESS_LETTERS = "kjhqrst"


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


def pvar(name):
    return {(name,): 1}


def pnum(value):
    return {(): value} if value else {}


def psubst(p, name, by):
    """Il polinomio p con la lettera name sostituita dal polinomio by."""
    out = {}
    for mono, c in p.items():
        term = {(): c}
        for v in mono:
            term = pmul(term, by if v == name else pvar(v))
        out = padd(out, term)
    return out


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


def factor_tex(p):
    """p scritto come fattore di un prodotto: tra parentesi se non è una lettera sola."""
    return ptex(p) if (len(p) == 1 and list(p.values())[0] == 1 and len(list(p)[0]) == 1) else "\\left(%s\\right)" % ptex(p)


def split_sides(p):
    """Due polinomi left, right con left - right = p: una parte dei termini va a destra."""
    items = list(p.items())
    rnd.shuffle(items)
    cut = rnd.randint(1, len(items)) if items else 0
    return dict(items[:cut]), {m: -c for m, c in items[cut:]}


class Condition:
    """'multiple': dividend è divisor per un intero. 'integer': dividend è un intero."""

    def __init__(self, kind, dividend, divisor=None):
        self.kind, self.dividend, self.divisor = kind, dividend, divisor

    def holds(self, env):
        value = peval(self.dividend, env)
        if self.kind == "integer":
            return value.denominator == 1
        by = peval(self.divisor, env)
        return value == 0 if by == 0 else (value / by).denominator == 1

    def tex(self, letters, integers_only):
        """Una delle scritture che il programma legge. letters è la riserva di lettere per i testimoni."""
        left, right = split_sides(self.dividend)
        if self.kind == "integer":
            if rnd.random() < 0.5:
                return "%s \\in \\mathbb{Z}" % ptex(self.dividend)
            return "\\exists %s \\in \\mathbb{Z}\\ (%s = %s)" % (letters[0], ptex(self.dividend), letters.pop(0))
        form = rnd.choice(["equiv", "mid", "exists", "exists"]) if integers_only else "exists"
        if form == "equiv":
            return "%s \\equiv %s \\pmod{%s}" % (ptex(left), ptex(right), ptex(self.divisor))
        if form == "mid":
            return "%s \\mid %s" % (ptex(self.divisor), ptex(self.dividend))
        letter = letters.pop(0)
        product = letter + factor_tex(self.divisor) if rnd.random() < 0.5 else "%s \\cdot %s" % (letter, factor_tex(self.divisor))
        return "\\exists %s \\in \\mathbb{Z}\\ (%s = %s)" % (letter, ptex(self.dividend) if rnd.random() < 0.5 else ptex(padd(left, right, -1)), product)


class Case:
    """Un enunciato: insiemi delle lettere, ipotesi, tesi, e se è vero per costruzione."""

    def __init__(self):
        getattr(self, rnd.choice(["congruence", "congruence", "chain", "difference", "bezout"]))()

    def small(self, names, degree):
        """Un polinomio a caso, non costante, a coefficienti interi nelle lettere names."""
        p = {}
        while not any(p):
            p = {}
            for _ in range(rnd.choice([1, 2, 2, 3])):
                mono = tuple(sorted(rnd.choice(names) for _ in range(rnd.randint(1, degree))))
                p[mono] = rnd.choice([-2, -1, 1, 1, 2, 3])
            if rnd.random() < 0.3:
                p[()] = rnd.choice([-1, 1, 2])
        return p

    def congruence(self):
        self.sets = [("n", "N"), ("a", "Z"), ("b", "Z"), ("c", "Z"), ("d", "Z")]
        modulus = pvar("n")
        p = self.small(["a", "c"], rnd.choice([1, 1, 2]))
        image = psubst(psubst(p, "a", pvar("b")), "c", pvar("d"))
        self.hyps = [Condition("multiple", padd(pvar("a"), pvar("b"), -1), modulus), Condition("multiple", padd(pvar("c"), pvar("d"), -1), modulus)]
        self.true = rnd.random() < 0.5
        if not self.true:
            wrong = rnd.choice(["coefficient", "modulus", "dropped"])
            if wrong == "coefficient":
                image = padd(image, rnd.choice([pnum(1), pvar("b"), pvar("d"), pnum(-1)]))
            elif wrong == "modulus":
                modulus = rnd.choice([padd(pvar("n"), pnum(1)), pmul(pnum(2), pvar("n"))])
            else:
                used = [h for h, name in zip(self.hyps, "ac") if any(name in mono for mono in p)]
                self.hyps.remove(rnd.choice(used))
        self.thesis = Condition("multiple", padd(p, image, -1), modulus)

    def chain(self):
        self.sets = [("m", "N"), ("n", "N"), ("a", "Z"), ("b", "Z")]
        self.true = rnd.random() < 0.5
        combination = padd(pmul(pnum(rnd.choice([1, 2, -1])), pvar("a")), pmul(pnum(rnd.choice([1, 3, -2])), pvar("b")))
        self.hyps = [Condition("multiple", pvar("n"), pvar("m")), Condition("multiple", pvar("a"), pvar("n")), Condition("multiple", pvar("b"), pvar("n"))]
        self.thesis = Condition("multiple", combination, pvar("m"))
        if not self.true:
            if rnd.random() < 0.5:
                self.hyps[0] = Condition("multiple", pvar("m"), pvar("n"))
            else:
                self.hyps = [self.hyps[1], Condition("multiple", pvar("b"), pvar("m"))]
                self.thesis = Condition("multiple", combination, pvar("n"))

    def difference(self):
        self.sets = [("x", "Q"), ("y", "Q"), ("z", "Q"), ("w", "Q")]
        self.true = rnd.random() < 0.5
        self.hyps = [Condition("integer", padd(pvar("x"), pvar("y"), -1)), Condition("integer", padd(pvar("z"), pvar("w"), -1))]
        u, v = rnd.choice([1, 2, -1]), rnd.choice([1, -1, 3])
        linear = padd(pmul(pnum(u), padd(pvar("x"), pvar("y"), -1)), pmul(pnum(v), padd(pvar("z"), pvar("w"), -1)))
        if self.true:
            self.thesis = Condition("integer", linear)
        else:
            wrong = rnd.choice([padd(pmul(pvar("x"), pvar("z")), pmul(pvar("y"), pvar("w")), -1), padd(pvar("x"), pvar("y")), padd(linear, pvar("z"))])
            self.thesis = Condition("integer", wrong)

    def bezout(self):
        self.sets = [("m", "N"), ("a", "Z"), ("b", "Z"), ("c", "Z"), ("u", "Z"), ("v", "Z")]
        self.true = rnd.random() < 0.5
        product = pmul(padd(pvar("a"), pvar("b"), -1), pvar("c"))
        self.hyps = [Condition("multiple", product, pvar("m"))]
        identity = padd(padd(pmul(pvar("u"), pvar("c")), pmul(pvar("v"), pvar("m"))), pnum(-1))
        # L'identità di Bézout come ipotesi: uc + vm - 1 è zero per un intero qualsiasi.
        self.equation = identity if self.true else None
        self.thesis = Condition("multiple", padd(pvar("a"), pvar("b"), -1), pvar("m"))

    def tex(self):
        # Una volta su tre tutti i testimoni hanno la stessa lettera, come nei libri.
        letters = ["k"] * len(WITNESS_LETTERS) if rnd.random() < 0.33 else list(WITNESS_LETTERS)
        integers_only = all(kind != "Q" for _, kind in self.sets)
        parts = [h.tex(letters, integers_only) for h in self.hyps]
        if getattr(self, "equation", None):
            left, right = split_sides(self.equation)
            parts.append("%s = %s" % (ptex(left), ptex(right)))
        rnd.shuffle(parts)
        parts = ["(%s)" % part if part.startswith("\\exists") else part for part in parts]
        thesis = self.thesis.tex(letters, integers_only)
        names = {"N": "\\mathbb{N}", "Z": "\\mathbb{Z}", "Q": "\\mathbb{Q}"}
        prefix = ""
        for kind in ("N", "Z", "Q"):
            group = [name for name, k in self.sets if k == kind]
            if group:
                prefix += "\\forall %s \\in %s\\ " % (", ".join(group), names[kind])
        hyps = parts[0] if len(parts) == 1 else "(%s)" % " \\land ".join(parts)
        return "%s(%s \\implies %s)" % (prefix, hyps, thesis)

    def fails_at(self, env):
        if not all(h.holds(env) for h in self.hyps):
            return False
        if getattr(self, "equation", None) and peval(self.equation, env) != 0:
            return False
        return not self.thesis.holds(env)

    def counterexample(self):
        """Il primo punto della finestra in cui le ipotesi valgono e la tesi no, o None."""
        windows = {"N": NATURALS, "Z": INTEGERS, "Q": RATIONALS}
        ranges = [windows[kind][:5 if len(self.sets) > 5 else 7] for _, kind in self.sets]
        for values in itertools.product(*ranges):
            env = {name: Fr(value) for (name, _), value in zip(self.sets, values)}
            if self.fails_at(env):
                return env
        return None

    def check(self, verdict, text):
        found = self.counterexample()
        if self.true and found:
            return [("ORACLE", "built to be true, but it fails at %s" % found)]
        if verdict == "P" and found:
            return [("BUG", "proved, but it fails at %s" % {k: str(v) for k, v in found.items()})]
        if verdict != "R":
            return []
        if self.true:
            return [("BUG", "refuted, but it is true by construction")]
        out = []
        values = dict((name, Fr(v)) for name, v in re.findall(r"(\w) = (-?\d+(?:/\d+)?)", text.split("controesempio ", 1)[-1]))
        if set(values) == set(name for name, _ in self.sets) and not self.fails_at(values):
            out.append(("BUG", "the counterexample printed does not falsify the statement"))
        if not found:
            out.append(("SUSPECT", "refuted, and the window has no counterexample"))
        return out


def main():
    cases = [Case() for _ in range(COUNT)]
    texts = [case.tex() for case in cases]
    path = os.path.join(WORK, "fuzz_witnesses_%d.tex" % SEED)
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
        key = ("true" if case.true else "other", verdict)
        tally[key] = tally.get(key, 0) + 1
        for severity, message in case.check(verdict, answer):
            print("%s %d [%s]: %s\n      %s" % (severity, number, answer[:60], message, text))
    print("seed %d:" % SEED, ", ".join("%s %s=%d" % (k[0], k[1], v) for k, v in sorted(tally.items())))


main()
