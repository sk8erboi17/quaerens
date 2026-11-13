r"""Fuzz delle somme con n nel coefficiente binomiale (regola 11n di main.c, il teorema del binomio).

Uso: python3 -I fuzz_binomial.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Genera enunciati \sum_{k=0}^{n} \binom{n}{...} (termine) = (formula), veri e falsi, li scrive in
<cartella di lavoro>/fuzz_binomial_<seme>.tex, li dà al programma e confronta le risposte con un
oracolo indipendente: i due membri calcolati con le frazioni esatte per n = 1, ..., 16, la somma
svolta termine per termine. L'oracolo non usa il teorema del binomio e non chiama mai il programma
per decidere.

Gli enunciati veri lo sono per costruzione (la formula è quella del teorema), e l'oracolo lo
conferma sui primi valori. Quelli falsi hanno la formula cambiata di poco: un 1 in più, una base
diversa, la metà presa per il tutto, oppure un termine in più che vale zero per n = 1, ..., 12 e
quindi supera il caso base e i primi controlli.

Righe stampate:
  BUG      il programma ha dimostrato un enunciato che l'oracolo trova falso, o ha dichiarato
           falso un enunciato vero: è il caso da correggere prima di tutto;
  NOTE     quanti enunciati di ogni tipo il programma non ha deciso, con un esempio;
  l'ultima riga riassume i verdetti per tipo (P dimostrato, R falso, N non dimostrato, U non
  supportato). Il codice di uscita è 1 se c'è almeno un BUG.
"""
import os
import random
import re
import subprocess
import sys
from fractions import Fraction
from math import comb

LAST = 16  # l'oracolo calcola i due membri per n = 1, ..., LAST


def frac_tex(q):
    """Un razionale in LaTeX, tra parentesi se serve per farne la base di una potenza."""
    q = Fraction(q)
    if q.denominator == 1:
        return str(q.numerator) if q >= 0 else "(%d)" % q.numerator
    sign = "-" if q < 0 else ""
    return "\\left(%s\\frac{%d}{%d}\\right)" % (sign, abs(q.numerator), q.denominator)


class Case:
    """Un enunciato: il testo, il tipo, e i due membri come funzioni di n sui razionali."""

    def __init__(self, kind, text, left, right):
        self.kind, self.text, self.left, self.right = kind, text, left, right

    def first_failure(self):
        """Il primo n tra 1 e LAST in cui i due membri differiscono, o None."""
        for n in range(1, LAST + 1):
            if self.left(n) != self.right(n):
                return n
        return None


def true_case(rnd):
    """Un enunciato vero: (tipo, testo della somma, testo della formula, i due membri)."""
    shape = rnd.choice(["all", "all", "two bases", "even", "odd"])
    if shape == "all":
        # \sum \binom{n}{k} c a^k = c (1 + a)^n, con 1 + a diverso da zero.
        a = rnd.choice([Fraction(v) for v in (2, 3, 4, -2, -3, 5)] + [Fraction(1, 2), Fraction(-1, 2), Fraction(2, 3)])
        c = rnd.choice([1, 1, 2, 3])
        left_tex = "\\sum_{k=0}^{n} \\binom{n}{k} %s%s^{k}" % ("" if c == 1 else "%d \\cdot " % c, frac_tex(a))
        right_tex = "%s%s^{n}" % ("" if c == 1 else "%d \\cdot " % c, frac_tex(1 + a))
        return shape, left_tex, right_tex, (lambda n: sum(comb(n, k) * c * a ** k for k in range(n + 1))), (lambda n: c * (1 + a) ** n)
    if shape == "two bases":
        # \sum \binom{n}{k} a^k b^{n-k} = (a + b)^n, con a + b diverso da zero.
        a, b = rnd.choice([(2, 3), (1, 4), (3, -1), (5, -2), (2, 5), (-1, 3)])
        left_tex = "\\sum_{k=0}^{n} \\binom{n}{k} %s^{k} \\cdot %s^{n - k}" % (frac_tex(a), frac_tex(b))
        return shape, left_tex, "%s^{n}" % frac_tex(a + b), (lambda n: sum(comb(n, k) * a ** k * b ** (n - k) for k in range(n + 1))), (lambda n: Fraction(a + b) ** n)
    # Solo i k pari o solo i dispari, con il termine s^{2k}: s è il numero y del teorema.
    s = rnd.choice([2, 3, 4])
    if shape == "even":
        left_tex = "\\sum_{k=0}^{n} \\binom{n}{2k} %d^{k}" % (s * s)
        right_tex = "\\frac{%d^{n} + (%d)^{n}}{2}" % (1 + s, 1 - s)
        return shape, left_tex, right_tex, (lambda n: sum(comb(n, 2 * k) * s ** (2 * k) for k in range(n + 1))), (lambda n: Fraction((1 + s) ** n + (1 - s) ** n, 2))
    left_tex = "\\sum_{k=0}^{n} \\binom{n}{2k + 1} %d^{k}" % (s * s)
    right_tex = "\\frac{%d^{n} - (%d)^{n}}{%d}" % (1 + s, 1 - s, 2 * s)
    return shape, left_tex, right_tex, (lambda n: sum(comb(n, 2 * k + 1) * s ** (2 * k) for k in range(n + 1))), (lambda n: Fraction((1 + s) ** n - (1 - s) ** n, 2 * s))


def make_case(rnd):
    """Un enunciato vero, oppure lo stesso con la formula rovinata."""
    shape, left_tex, right_tex, left, right = true_case(rnd)
    spoil = rnd.choice([None, None, "plus one", "times n", "late"])
    if spoil is None:
        return Case("true/" + shape, "%s = %s" % (left_tex, right_tex), left, right)
    if spoil == "plus one":
        return Case("false/plus one", "%s = %s + 1" % (left_tex, right_tex), left, lambda n: right(n) + 1)
    if spoil == "times n":
        # Giusto per n = 1, sbagliato dopo.
        return Case("false/times n", "%s = n \\cdot \\left(%s\\right)" % (left_tex, right_tex), left, lambda n: n * right(n))
    # Giusto per n = 1, ..., 12: un prodotto che lì vale zero.
    extra = "".join("(n - %d)" % j for j in range(1, 13))
    def late(n):
        product = 1
        for j in range(1, 13):
            product *= n - j
        return right(n) + product
    return Case("false/late", "%s = %s + %s" % (left_tex, right_tex, extra), left, late)


def main():
    program, seed, count, work = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
    rnd = random.Random(seed)
    cases = [make_case(rnd) for _ in range(count)]
    path = os.path.join(work, "fuzz_binomial_%d.tex" % seed)
    with open(path, "w", encoding="utf-8") as out:
        out.write("\\begin{enumerate}\n" + "".join("\\item \\[ %s \\]\n" % c.text for c in cases) + "\\end{enumerate}\n")
    run = subprocess.run([program, path], capture_output=True, text=True, errors="replace", timeout=3600)
    answers = [l.split(". ", 1)[1] for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
    if len(answers) != len(cases):
        sys.exit("the program answered %d of %d statements\n%s" % (len(answers), len(cases), run.stderr[-500:]))

    bugs, tally, undecided = 0, {}, {}
    for number, (case, answer) in enumerate(zip(cases, answers), 1):
        verdict = "P" if answer.startswith("DIMOSTRATO") else "R" if answer.startswith("FALSO") else "N" if answer.startswith("NON DIMOSTRATO") else "U"
        tally[case.kind + "/" + verdict] = tally.get(case.kind + "/" + verdict, 0) + 1
        failure = case.first_failure()
        if case.kind.startswith("true") and failure is not None:
            sys.exit("the generator is wrong: statement %d should be true and fails at n = %d\n   %s" % (number, failure, case.text))
        if case.kind.startswith("false") and failure is None:
            sys.exit("the generator is wrong: statement %d should be false and holds up to n = %d\n   %s" % (number, LAST, case.text))
        if verdict == "P" and failure is not None:
            bugs += 1
            print("BUG: statement %d proved, but the two sides differ at n = %d\n   %s" % (number, failure, case.text))
        elif verdict == "R" and failure is None:
            bugs += 1
            print("BUG: statement %d called false, but it is true by the binomial theorem\n   %s\n   %s" % (number, case.text, answer[:120]))
        elif verdict in "NU":
            undecided.setdefault((case.kind, answer[:90]), []).append(number)
    for (kind, answer), numbers in sorted(undecided.items()):
        print("NOTE: %d statements of %s not decided (%s), first is statement %d" % (len(numbers), kind, answer, numbers[0]))
    print("seed %d: %s" % (seed, ", ".join("%s=%d" % item for item in sorted(tally.items()))))
    return 1 if bugs else 0


if __name__ == "__main__":
    sys.exit(main())
