"""Fuzzing differenziale del numero degli elementi di un insieme confrontato
con un numero scritto, |T| = 3 (17x di parser.c, svolto da 17q e ricontrollato
da 28h di main.c).

Uso: python3 -I fuzz_cardinality.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Genera enunciati di due famiglie, fatti di confronti |E| = n, \\neq, \\le,
\\ge, <, > con n da 0 a 4, anche negati, uniti in ipotesi e tesi:

  - GRANDEZZE: E è un insieme qualsiasi S o T, un insieme di parti
    \\mathcal{P}(S), o l'insieme delle coppie \\{(x, y) \\mid x \\in S \\land
    y \\in T\\}; tra le ipotesi può esserci un'applicazione f da S a T, anche
    iniettiva, suriettiva o tutte e due.
  - PARTI: X e Y sono parti di S, ed E è X, Y, la loro unione, intersezione
    o differenza, o S; tra le ipotesi può esserci X \\subseteq Y, X \\cap Y =
    \\emptyset, X = Y o X = S.

Metà degli enunciati nasce da un fatto vero (2^a parti, a b coppie, una
somma per un'unione disgiunta, il verso di un'iniezione) a cui, una volta su
due, si sposta un numero di uno o si toglie un'ipotesi; l'altra metà è presa
a caso.

Li fa decidere al programma e confronta ogni verdetto con un oracolo scritto
qui, che non chiama mai il programma e non svolge nulla: fa i conti con i
numeri degli elementi.

  - GRANDEZZE: l'enunciato dipende solo da s = |S| e t = |T|. |\\mathcal{P}(S)|
    è 2^s, le coppie sono s t; un'applicazione da S a T c'è se t > 0 o s = 0,
    una iniettiva se s <= t, una suriettiva se t <= s (e t > 0 o s = 0).
    L'oracolo prova ogni s e t da 0 a N + 2, con N il numero più grande
    scritto.
  - PARTI: l'enunciato dipende solo da quanti elementi stanno in X e non in
    Y, in tutti e due, in Y e non in X, in nessuno dei due. L'oracolo prova
    ogni scelta dei quattro numeri da 0 a N + 1.

Perché la finestra basta. Ogni confronto è con un numero scritto, al più N:
un insieme con più di N elementi, anche infinito, dà a ogni confronto lo
stesso valore di uno con N + 1 (2^s > s, e s t > N se s > N e t >= 1); e tra
due insiemi più grandi di N conta solo quale dei due lo è di più, che N + 1 e
N + 2 rappresentano nei tre casi. Quindi un enunciato senza controesempio
nella finestra vale per tutti gli insiemi, finiti o no, e uno con un
controesempio è falso: l'oracolo decide sempre, e nei due versi.

Stampa BUG quando il verdetto contraddice l'oracolo: un DIMOSTRATO con un
controesempio, un FALSO senza. Stampa SUSPECT per un NON SUPPORTATO: sono
enunciati che 17x dice di leggere, e va guardato a mano. Alla fine il conto
dei verdetti, e quanti enunciati veri non sono stati dimostrati e quanti
falsi non smentiti (non è un errore: è ciò che i budget non hanno raggiunto).
"""
import itertools
import os
import random
import re
import subprocess
import sys

MR, SEED, COUNT, WORK = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
rnd = random.Random(SEED)
TOP = 4
COMPARE = {
    "=": lambda size, n: size == n,
    r"\neq": lambda size, n: size != n,
    r"\le": lambda size, n: size <= n,
    r"\ge": lambda size, n: size >= n,
    "<": lambda size, n: size < n,
    ">": lambda size, n: size > n,
}
INJECTIVE = r"\forall x, y \in S\ (f(x) = f(y) \implies x = y)"
SURJECTIVE = r"\forall y \in T\ \exists x \in S\ (f(x) = y)"

# Gli insiemi di cui si contano gli elementi: come si scrivono, e quanti
# elementi hanno. Per GRANDEZZE il conto dipende da (s, t); per PARTI dalle
# quattro regioni (solo X, tutti e due, solo Y, nessuno).
SIZES = {
    "S": ("S", lambda s, t: s),
    "T": ("T", lambda s, t: t),
    "PS": (r"\mathcal{P}(S)", lambda s, t: 2 ** s),
    "ST": (r"\{(x, y) \mid x \in S \land y \in T\}", lambda s, t: s * t),
}
PARTS = {
    "X": ("X", lambda a, b, c, d: a + b),
    "Y": ("Y", lambda a, b, c, d: b + c),
    "XuY": (r"X \cup Y", lambda a, b, c, d: a + b + c),
    "XnY": (r"X \cap Y", lambda a, b, c, d: b),
    "X-Y": (r"X \setminus Y", lambda a, b, c, d: a),
    "Y-X": (r"Y \setminus X", lambda a, b, c, d: c),
    "S": ("S", lambda a, b, c, d: a + b + c + d),
}
RELATIONS = {
    "sub": (r"X \subseteq Y", lambda a, b, c, d: a == 0),
    "disjoint": (r"X \cap Y = \emptyset", lambda a, b, c, d: b == 0),
    "same": ("X = Y", lambda a, b, c, d: a == 0 and c == 0),
    "all": ("X = S", lambda a, b, c, d: c == 0 and d == 0),
}


def count(name, relation, number, negated=False):
    """Un confronto: ("count", insieme, confronto, numero, negato)."""
    return ("count", name, relation, number, negated)


class Case:
    """Un enunciato: una famiglia, le ipotesi (un elenco di confronti e di
    relazioni), come sono unite le tesi, e le tesi."""

    def __init__(self):
        self.sizes = rnd.random() < 0.5
        self.table = SIZES if self.sizes else PARTS
        self.map = None       # GRANDEZZE: None, "any", "injective", "surjective", "bijective"
        self.hyps = []
        self.join = None
        self.theses = []
        if rnd.random() < 0.5:
            self.derived()
        else:
            self.random()

    def atom(self):
        return count(rnd.choice(sorted(self.table)), rnd.choice(sorted(COMPARE)), rnd.randint(0, TOP), rnd.random() < 0.15)

    def random(self):
        if self.sizes and rnd.random() < 0.5:
            self.map = rnd.choice(["any", "injective", "surjective", "bijective"])
        for _ in range(rnd.randint(0, 3)):
            self.hyps.append(self.atom())
        if not self.sizes and rnd.random() < 0.4:
            self.hyps.append(("relation", rnd.choice(sorted(RELATIONS))))
        self.theses = [self.atom() for _ in range(rnd.choice([1, 1, 1, 2]))]
        if len(self.theses) == 2:
            self.join = rnd.choice([r"\land", r"\lor"])

    def derived(self):
        """Un fatto vero, e una volta su due una sua variante sbagliata."""
        a, b = rnd.randint(0, 2), rnd.randint(0, 2)
        if self.sizes:
            kind = rnd.randint(0, 6)
            if kind == 0:
                self.hyps, self.theses = [count("S", "=", a)], [count("PS", "=", 2 ** a)]
            elif kind == 1:
                self.hyps, self.theses = [count("S", "=", a), count("T", "=", b)], [count("ST", "=", a * b)]
            elif kind == 2:
                self.map, self.hyps, self.theses = "injective", [count("T", r"\le", b)], [count("S", r"\le", b)]
            elif kind == 3:
                self.map, self.hyps, self.theses = "injective", [count("S", r"\ge", a)], [count("T", r"\ge", a)]
            elif kind == 4:
                self.map, self.hyps, self.theses = "surjective", [count("S", r"\le", a)], [count("T", r"\le", a)]
            elif kind == 5:
                self.map, self.hyps, self.theses = "bijective", [count("S", "=", a)], [count("T", "=", a)]
            else:
                self.hyps, self.theses = [count("S", r"\le", a)], [count("PS", r"\le", 2 ** a)]
        else:
            kind = rnd.randint(0, 6)
            if kind == 0:
                self.hyps, self.theses = [count("X", r"\le", a), count("Y", r"\le", b)], [count("XuY", r"\le", a + b)]
            elif kind == 1:
                self.hyps = [count("X", "=", a), count("Y", "=", b), ("relation", "disjoint")]
                self.theses = [count("XuY", "=", a + b)]
            elif kind == 2:
                self.hyps, self.theses = [("relation", "sub"), count("Y", r"\le", b)], [count("X", r"\le", b)]
            elif kind == 3:
                self.hyps, self.theses = [("relation", "sub"), count("X", r"\ge", a)], [count("Y", r"\ge", a)]
            elif kind == 4:
                low = min(a, b)
                self.hyps, self.theses = [count("X", "=", max(a, b)), count("XnY", "=", low)], [count("X-Y", "=", max(a, b) - low)]
            elif kind == 5:
                self.hyps, self.theses = [count("S", r"\le", a)], [count("XuY", r"\le", a)]
            else:
                self.hyps, self.theses = [count("XuY", r"\le", a)], [count("XnY", r"\le", a)]
        if rnd.random() < 0.5:
            self.spoil()

    def spoil(self):
        """Sposta di uno il numero di una tesi, o toglie un'ipotesi, o
        l'applicazione."""
        choice = rnd.randint(0, 2)
        if choice == 0 or (choice == 1 and not self.hyps) or (choice == 2 and not self.map):
            _, name, relation, number, negated = self.theses[0]
            self.theses[0] = count(name, relation, max(0, number + rnd.choice([-1, 1])), negated)
        elif choice == 1:
            self.hyps.pop(rnd.randrange(len(self.hyps)))
        else:
            self.map = "any"

    def atom_tex(self, atom):
        if atom[0] == "relation":
            return RELATIONS[atom[1]][0]
        _, name, relation, number, negated = atom
        body = "|%s| %s %d" % (self.table[name][0], relation, number)
        return r"\lnot (%s)" % body if negated else body

    def tex(self):
        hyps = []
        if self.sizes and self.map:
            hyps.append(r"f \colon S \to T")
            if self.map in ("injective", "bijective"):
                hyps.append(INJECTIVE)
            if self.map in ("surjective", "bijective"):
                hyps.append(SURJECTIVE)
        if not self.sizes:
            hyps += [r"X \subseteq S", r"Y \subseteq S"]
        hyps += [self.atom_tex(h) for h in self.hyps]
        if self.join:
            thesis = (" %s " % self.join).join("(%s)" % self.atom_tex(t) for t in self.theses)
        else:
            thesis = self.atom_tex(self.theses[0])
        if not hyps:
            return thesis
        return r"(%s) \implies (%s)" % (r" \land ".join(hyps), thesis)

    def value(self, atom, point):
        if atom[0] == "relation":
            return RELATIONS[atom[1]][1](*point)
        _, name, relation, number, negated = atom
        return COMPARE[relation](self.table[name][1](*point), number) != negated

    def possible(self, point):
        """GRANDEZZE: 1 se con s e t elementi c'è un'applicazione del tipo
        chiesto dalle ipotesi."""
        if not self.sizes or not self.map:
            return True
        s, t = point
        exists = t > 0 or s == 0
        if self.map == "any":
            return exists
        if self.map == "injective":
            return s <= t
        if self.map == "surjective":
            return t <= s and exists
        return s == t

    def counterexample(self):
        """Il primo punto della finestra in cui le ipotesi valgono e la tesi
        no, scritto; None se non ce n'è, e allora l'enunciato vale sempre."""
        top = max([a[3] for a in self.hyps + self.theses if a[0] == "count"] + [0])
        window = range(top + 3) if self.sizes else range(top + 2)
        for point in itertools.product(window, repeat=2 if self.sizes else 4):
            if not self.possible(point) or not all(self.value(h, point) for h in self.hyps):
                continue
            values = [self.value(t, point) for t in self.theses]
            if not (any(values) if self.join == r"\lor" else all(values)):
                names = ("|S|", "|T|") if self.sizes else ("only X", "both", "only Y", "neither")
                return ", ".join("%s = %d" % pair for pair in zip(names, point))
        return None


def main():
    cases = [Case() for _ in range(COUNT)]
    texts = [case.tex() for case in cases]
    path = os.path.join(WORK, "fuzz_cardinality_%d.tex" % SEED)
    with open(path, "w", encoding="utf-8") as out:
        out.write("\\begin{enumerate}\n")
        for text in texts:
            out.write("\\item $\\displaystyle %s$\n" % text)
        out.write("\\end{enumerate}\n")
    run = subprocess.run([MR, path], capture_output=True, text=True, timeout=7200)
    lines = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
    if len(lines) != len(cases) or run.returncode not in (0, 1):
        print("BUG: the program answered %d of %d statements (exit %d)\n%s" % (len(lines), len(cases), run.returncode, run.stderr[-2000:]))
        return
    tally, true_missed, false_missed, true_total = {}, 0, 0, 0
    for number, (case, text, line) in enumerate(zip(cases, texts, lines), 1):
        answer = line.split(". ", 1)[1]
        verdict = "P" if answer.startswith("DIMOSTRATO") else "R" if answer.startswith("FALSO") else "N" if answer.startswith("NON DIMOSTRATO") else "U"
        tally[verdict] = tally.get(verdict, 0) + 1
        found = case.counterexample()
        true_total += found is None
        if verdict == "P" and found:
            print("BUG %d [%s]: proved, but false for %s\n      %s" % (number, answer[:60], found, text))
        elif verdict == "R" and not found:
            print("BUG %d [%s]: refuted, but true for every number of elements\n      %s" % (number, answer[:60], text))
        elif verdict == "U":
            print("SUSPECT %d [%s]: not read\n      %s" % (number, answer[:90], text))
        elif verdict == "N" and found:
            false_missed += 1
        elif verdict == "N":
            true_missed += 1
    print("seed %d: %s; true for the oracle: %d of %d; true and not proved: %d; false and not refuted: %d"
          % (SEED, ", ".join("%s=%d" % kv for kv in sorted(tally.items())), true_total, len(cases), true_missed, false_missed))


main()
