"""Fuzzing differenziale dell'equipotenza, A \\sim B (17w di parser.c, 28q di
main.c, 11j di checker.c).

Uso: python3 -I fuzz_equipotence.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Genera enunciati fatti di equipotenze tra insiemi costruiti da qualche
lettera (S, T, V, W) con \\mathcal{P}, gli insiemi di applicazioni T^{S} e i
prodotti di due: delle ipotesi, anche negate, e una tesi, anche negata o
fatta di due equipotenze. Molte tesi nascono da un lato a cui si applicano
trasformazioni che danno un insieme equipotente (le componenti di un
prodotto scambiate, una lettera al posto di una equipotente per ipotesi),
altre da trasformazioni che di solito non lo danno (una lettera cambiata, un
fattore in più, base ed esponente scambiati).

Li fa decidere al programma e confronta ogni verdetto con due oracoli scritti
qui, che non chiamano mai il programma:

  - FINITO: per ogni scelta del numero degli elementi delle lettere, da 0 a
    WINDOW, calcola il numero degli elementi di ogni lato (2^n per le parti,
    m^n per le applicazioni da n a m elementi, con 0^0 = 1, il prodotto per un
    prodotto) e il valore dell'enunciato: tra insiemi finiti c'è una biiezione
    se e solo se hanno tanti elementi quanti. Un controesempio trovato così è
    certo; i modelli del programma hanno insiemi di al più 4 elementi, dentro
    la finestra.
  - SIMBOLICO, per gli enunciati con sole ipotesi A \\sim B tra lettere e una
    tesi che è una congiunzione di equipotenze: riduce ogni lato a una forma
    normale con le leggi delle potenze dei cardinali, che valgono per insiemi
    qualsiasi, infiniti o vuoti, con biiezioni scritte senza scegliere nulla:
    il prodotto è commutativo e associativo, (a^b)^c = a^{b c}, (a b)^c = a^c
    b^c, \\mathcal{P}(a) = 2^a; e ogni lettera è sostituita dalla prima della
    sua classe nelle ipotesi. Due forme uguali vogliono dire che l'enunciato
    vale per ogni scelta degli insiemi. Il ragionamento non è quello del
    programma: lui costruisce le biiezioni con la grammatica di 28q, che non
    conosce l'associatività né (a^b)^c = a^{bc}.

Stampa BUG quando il verdetto contraddice un fatto certo: un DIMOSTRATO con un
controesempio finito, un FALSO senza controesempio nella finestra. Stampa
SUSPECT per un DIMOSTRATO che l'oracolo simbolico non conferma: va guardato a
mano. Alla fine il conto dei verdetti, e quanti enunciati veri per l'oracolo
simbolico il programma non ha dimostrato (non è un errore: la grammatica è
più piccola delle leggi).
"""
import itertools
import os
import random
import re
import subprocess
import sys

MR, SEED, COUNT, WORK = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
rnd = random.Random(SEED)
WINDOW = 4


# Un insieme è un albero: ("L", lettera), ("P", a), ("M", base, esponente)
# per base^{esponente}, ("X", a, b) per il prodotto a \times b (solo in un lato).
def tex(e, inside=False):
    kind = e[0]
    if kind == "L":
        return e[1]
    if kind == "P":
        return r"\mathcal{P}(%s)" % tex(e[1])
    if kind == "M":
        base = tex(e[1])
        if e[1][0] == "M":
            base = "(%s)" % base
        return "%s^{%s}" % (base, tex(e[2]))
    return r"%s \times %s" % (tex(e[1]), tex(e[2]))


def size(e, n):
    """Il numero degli elementi di e, con n[lettera] elementi per lettera."""
    kind = e[0]
    if kind == "L":
        return n[e[1]]
    if kind == "P":
        return 2 ** size(e[1], n)
    if kind == "M":
        return size(e[1], n) ** size(e[2], n)
    return size(e[1], n) * size(e[2], n)


def normal(e, rep):
    """La forma normale di e: una tupla ordinata di fattori (base, esponente),
    con base una lettera (la prima della sua classe) o "2", ed esponente a sua
    volta una forma normale; () è 1."""
    kind = e[0]
    if kind == "L":
        return ((rep[e[1]], ()),)
    if kind == "P":
        return (("2", normal(e[1], rep)),)
    if kind == "M":
        exponent = normal(e[2], rep)
        return tuple(sorted((b, tuple(sorted(x + exponent))) for b, x in normal(e[1], rep)))
    return tuple(sorted(normal(e[1], rep) + normal(e[2], rep)))


def factor(letters):
    roll = rnd.random()
    a = ("L", rnd.choice(letters))
    if roll < 0.45:
        return a
    if roll < 0.7:
        return ("P", a)
    if roll < 0.9:
        return ("M", a, ("L", rnd.choice(letters)))
    if roll < 0.95:
        return ("P", ("P", a))
    return ("M", ("M", a, ("L", rnd.choice(letters))), ("L", rnd.choice(letters)))


def side(letters):
    a = factor(letters)
    return ("X", a, factor(letters)) if rnd.random() < 0.35 else a


def rename(e, change):
    if e[0] == "L":
        return ("L", change.get(e[1], e[1]))
    return (e[0],) + tuple(rename(x, change) for x in e[1:])


def variant(e, letters, pairs, faithful):
    """Un lato da confrontare con e: equipotente se faithful (componenti
    scambiate, lettere sostituite con altre equipotenti per ipotesi), di
    solito no altrimenti."""
    if faithful:
        change = {}
        for a, b in pairs:
            if rnd.random() < 0.7:
                change[a] = b
        out = rename(e, change)
        if out[0] == "X" and rnd.random() < 0.5:
            out = ("X", out[2], out[1])
        return out
    roll = rnd.random()
    if roll < 0.3:
        return rename(e, {rnd.choice(letters): rnd.choice(letters)})
    if roll < 0.5 and e[0] != "X":
        return ("X", e, factor(letters))
    if roll < 0.7 and e[0] == "M":
        return ("M", e[2], e[1])
    if roll < 0.85:
        return ("P", e) if e[0] != "X" else e[1]
    return side(letters)


class Case:
    def __init__(self):
        self.letters = rnd.sample(["S", "T", "V", "W"], rnd.randint(1, 3))
        # Le ipotesi: equipotenze tra lettere (pairs), qualche volta tra lati
        # o negate, e qualche "non vuoto".
        self.hyps = []  # (negata, lato, lato)
        self.pairs = []
        for _ in range(rnd.randint(0, 2)):
            if len(self.letters) > 1 and rnd.random() < 0.75:
                a, b = rnd.sample(self.letters, 2)
                self.pairs.append((a, b))
                self.hyps.append((False, ("L", a), ("L", b)))
            else:
                self.hyps.append((rnd.random() < 0.4, side(self.letters), side(self.letters)))
        self.nonempty = [x for x in self.letters if rnd.random() < 0.2]
        left = side(self.letters)
        theses = [(rnd.random() < 0.15, left, variant(left, self.letters, self.pairs, rnd.random() < 0.6))]
        self.join = None
        if rnd.random() < 0.15:
            self.join = rnd.choice([r"\land", r"\lor"])
            other = side(self.letters)
            theses.append((False, other, variant(other, self.letters, self.pairs, rnd.random() < 0.6)))
        self.theses = theses

    @staticmethod
    def atom(negated, a, b):
        body = r"%s \sim %s" % (tex(a), tex(b))
        return r"\lnot (%s)" % body if negated else body

    def tex(self):
        hyps = [r"%s \neq \emptyset" % x for x in self.nonempty] + [self.atom(*h) for h in self.hyps]
        thesis = (" %s " % self.join).join("(%s)" % self.atom(*t) for t in self.theses) if self.join else self.atom(*self.theses[0])
        if not hyps:
            return thesis
        return r"(%s) \implies (%s)" % (r" \land ".join(hyps), thesis)

    def holds(self, n):
        for x in self.nonempty:
            if n[x] == 0:
                return True
        for negated, a, b in self.hyps:
            if (size(a, n) == size(b, n)) == negated:
                return True
        values = [(size(a, n) == size(b, n)) != negated for negated, a, b in self.theses]
        return all(values) if self.join != r"\lor" else any(values)

    def counterexample(self):
        for sizes in itertools.product(range(WINDOW + 1), repeat=len(self.letters)):
            n = dict(zip(self.letters, sizes))
            if not self.holds(n):
                return ", ".join("|%s| = %d" % item for item in sorted(n.items()))
        return None

    def symbolic(self):
        """1 se l'oracolo simbolico dice che l'enunciato vale per ogni scelta
        degli insiemi; None se non sa dirlo. Usa le sole ipotesi A \\sim B tra
        lettere (lasciare fuori un'ipotesi rende la tesi più difficile, mai più
        facile); un'ipotesi negata tra due lati con la stessa forma normale è
        falsa per ogni scelta, e allora l'enunciato vale comunque. Di una tesi
        negata non sa dire nulla."""
        rep = {x: x for x in self.letters}

        def find(x):
            while rep[x] != x:
                x = rep[x]
            return x

        for a, b in self.pairs:
            ra, rb = find(a), find(b)
            if ra != rb:
                rep[max(ra, rb)] = min(ra, rb)
        rep = {x: find(x) for x in self.letters}
        if any(negated and normal(a, rep) == normal(b, rep) for negated, a, b in self.hyps):
            return 1
        sure = [not negated and normal(a, rep) == normal(b, rep) for negated, a, b in self.theses]
        return 1 if (any(sure) if self.join == r"\lor" else all(sure)) else None


def main():
    cases = [Case() for _ in range(COUNT)]
    texts = [case.tex() for case in cases]
    path = os.path.join(WORK, "fuzz_equipotence_%d.tex" % SEED)
    with open(path, "w", encoding="utf-8") as out:
        out.write("\\begin{enumerate}\n")
        for text in texts:
            out.write("\\item $\\displaystyle %s$\n" % text)
        out.write("\\end{enumerate}\n")
    run = subprocess.run([MR, path], capture_output=True, text=True, timeout=3600)
    lines = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
    if len(lines) != len(cases) or run.returncode not in (0, 1):
        print("BUG: the program answered %d of %d statements (exit %d)\n%s" % (len(lines), len(cases), run.returncode, run.stderr[-2000:]))
        return
    tally, missed = {}, 0
    for number, (case, text, line) in enumerate(zip(cases, texts, lines), 1):
        answer = line.split(". ", 1)[1]
        verdict = "P" if answer.startswith("DIMOSTRATO") else "R" if answer.startswith("FALSO") else "N" if answer.startswith("NON DIMOSTRATO") else "U"
        tally[verdict] = tally.get(verdict, 0) + 1
        found, sure = case.counterexample(), case.symbolic()
        if verdict == "P" and found:
            print("BUG %d [%s]: proved, but false for %s\n      %s" % (number, answer[:60], found, text))
        elif verdict == "R" and not found:
            print("BUG %d [%s]: refuted, but true for every choice of at most %d elements\n      %s" % (number, answer[:60], WINDOW, text))
        elif verdict == "P" and not sure:
            print("SUSPECT %d [%s]: proved, and the symbolic oracle cannot confirm it\n      %s" % (number, answer[:60], text))
        if sure and found:
            print("BUG %d: the two oracles disagree (an oracle is wrong)\n      %s" % (number, text))
        if sure and verdict != "P":
            missed += 1
    print("seed %d: %s; true for the symbolic oracle and not proved: %d" % (SEED, ", ".join("%s=%d" % kv for kv in sorted(tally.items())), missed))


main()
