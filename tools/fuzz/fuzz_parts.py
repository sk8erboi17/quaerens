"""Fuzzing differenziale delle parti e delle applicazioni prese come oggetti.

Uso: python3 -I fuzz_parts.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Genera enunciati di due famiglie, in cui \\mathcal{P}(S) è un insieme di
oggetti (paragrafo 17v di parser.c, 28m-28o di main.c):

  - una relazione tra le parti di S data da una regola,
    R = \\{(X, Y) \\in \\mathcal{P}(S)^{2} \\mid c\\}, con c una condizione tra X,
    Y e una parte E (X \\cap E = Y \\cap E, X \\subseteq Y, \\forall t \\in X\\
    (t \\in Y), ...), e per tesi una proprietà, affermata o negata:
    riflessiva, simmetrica, transitiva, antisimmetrica;
  - un'applicazione f \\colon S \\to \\mathcal{P}(S) data da una regola, f(x) =
    S \\setminus \\{x\\}, \\{x\\}, E \\cup \\{x\\}, ..., e per tesi iniettiva o
    suriettiva, affermata o negata;
  - una relazione data dall'elenco delle sue coppie di numeri, G_{1} =
    \\{(1, 2), (2, 2)\\}, con la sua composta G, e per tesi simmetrica,
    transitiva, antisimmetrica o G \\subseteq G_{1}: qui l'oracolo calcola con
    le coppie, e la risposta è certa.

Li fa decidere al programma e confronta ogni verdetto con un oracolo scritto
qui, che non chiama mai il programma e non svolge nessuna notazione: per ogni
S con al più WINDOW elementi, vuoto compreso, e per ogni parte E di S, calcola
le parti di S come insiemi di Python, la relazione o l'applicazione dalla sua
definizione, e la proprietà provando ogni parte. Un controesempio trovato così
è certo. La sua assenza no: l'enunciato potrebbe essere falso solo per un S più
grande. Ma i modelli del programma (28m) hanno insiemi di al più tre elementi
quando ci sono le loro parti, dentro la finestra: un FALSO senza controesempio
nella finestra è un errore certo.

Stampa BUG quando il verdetto contraddice un fatto certo: un DIMOSTRATO con un
controesempio, un FALSO senza.
"""
import itertools
import os
import random
import re
import subprocess
import sys

MR, SEED, COUNT, WORK = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
rnd = random.Random(SEED)
WINDOW = 3

# Le condizioni della regola di R: il testo, e il calcolo su X, Y, E, S
# (frozenset di Python).
CONDITIONS = [
    (r"X \cap E = Y \cap E", lambda X, Y, E, S: X & E == Y & E),
    (r"X \cup E = Y \cup E", lambda X, Y, E, S: X | E == Y | E),
    (r"X \setminus E = Y \setminus E", lambda X, Y, E, S: X - E == Y - E),
    (r"X \subseteq Y", lambda X, Y, E, S: X <= Y),
    (r"X \cap Y = \emptyset", lambda X, Y, E, S: not (X & Y)),
    (r"X \cap Y \neq \emptyset", lambda X, Y, E, S: bool(X & Y)),
    (r"X \cup Y = S", lambda X, Y, E, S: X | Y == S),
    (r"E \subseteq X \cup Y", lambda X, Y, E, S: E <= X | Y),
    (r"\forall t \in X\ (t \in Y)", lambda X, Y, E, S: X <= Y),
    (r"\exists t \in X\ (t \in Y)", lambda X, Y, E, S: bool(X & Y)),
    (r"\forall t \in X \cap E\ (t \in Y)", lambda X, Y, E, S: X & E <= Y),
    (r"X = Y \lor X \cap E = \emptyset", lambda X, Y, E, S: X == Y or not (X & E)),
]

# Le proprietà di R: il testo, e il calcolo sull'insieme P delle parti.
def reflexive(R, P):
    return all((x, x) in R for x in P)


def symmetric(R, P):
    return all((y, x) in R for (x, y) in R)


def transitive(R, P):
    return all((x, w) in R for (x, y) in R for (z, w) in R if y == z)


def antisymmetric(R, P):
    return all(x == y for (x, y) in R if (y, x) in R)


PROPERTIES = [
    (r"\forall x \in \mathcal{P}(S)\ ((x, x) \in R)", reflexive),
    (r"\forall x, y\ ((x, y) \in R \implies (y, x) \in R)", symmetric),
    (r"\forall x, y, z\ (((x, y) \in R \land (y, z) \in R) \implies (x, z) \in R)", transitive),
    (r"\forall x, y\ (((x, y) \in R \land (y, x) \in R) \implies x = y)", antisymmetric),
]

# Le regole di f: il testo di f(x), e il calcolo su x, E, S.
RULES = [
    (r"S \setminus \{x\}", lambda x, E, S: S - {x}),
    (r"\{x\}", lambda x, E, S: frozenset({x})),
    (r"\emptyset", lambda x, E, S: frozenset()),
    (r"S", lambda x, E, S: S),
    (r"E", lambda x, E, S: E),
    (r"E \cup \{x\}", lambda x, E, S: E | {x}),
    (r"E \setminus \{x\}", lambda x, E, S: E - {x}),
]


def injective(f, S, P):
    return all(a == b for a in S for b in S if f[a] == f[b])


def surjective(f, S, P):
    return all(any(f[x] == y for x in S) for y in P)


MAP_PROPERTIES = [
    (r"\forall a, b \in S\ (f(a) = f(b) \implies a = b)", injective),
    (r"\forall y \in \mathcal{P}(S)\ \exists x \in S\ (f(x) = y)", surjective),
]


def parts(S):
    items = sorted(S)
    return [frozenset(c) for k in range(len(items) + 1) for c in itertools.combinations(items, k)]


# Le proprietà di una relazione data dall'elenco delle sue coppie di numeri
# (17v), sui soli quantificatori che l'appartenenza obbliga: il testo, e il
# calcolo sull'insieme G delle coppie. "La composta di G con sé è contenuta in
# G" è un altro modo di dire transitiva, scritto con un insieme dato da una
# regola.
LIST_PROPERTIES = [
    (r"\forall x, y\ ((x, y) \in G_{1} \implies (y, x) \in G_{1})", lambda G: all((y, x) in G for (x, y) in G)),
    (r"\forall x, y, z\ (((x, y) \in G_{1} \land (y, z) \in G_{1}) \implies (x, z) \in G_{1})",
     lambda G: all((x, w) in G for (x, y) in G for (z, w) in G if y == z)),
    (r"\forall x, y\ (((x, y) \in G_{1} \land (y, x) \in G_{1}) \implies x = y)", lambda G: all(x == y for (x, y) in G if (y, x) in G)),
    (r"\forall x, y\ ((x, y) \in G \implies (x, y) \in G_{1})",
     lambda G: all((x, w) in G for (x, y) in G for (z, w) in G if y == z)),
]


class Case:
    def __init__(self):
        self.family = rnd.choice(["relation", "map", "list"])
        self.negated = rnd.random() < 0.3
        if self.family == "relation":
            self.condition = rnd.choice(CONDITIONS)
            self.property = rnd.choice(PROPERTIES)
        elif self.family == "map":
            self.rule = rnd.choice(RULES)
            self.property = rnd.choice(MAP_PROPERTIES)
        else:
            numbers = rnd.sample(range(5), rnd.randint(1, 3))
            pairs = [(a, b) for a in numbers for b in numbers]
            self.pairs = rnd.sample(pairs, rnd.randint(1, len(pairs)))
            self.property = rnd.choice(LIST_PROPERTIES)

    def tex(self):
        claim = self.property[0]
        if self.negated:
            claim = r"\lnot (%s)" % claim
        if self.family == "relation":
            return r"(E \subseteq S \land R = \{(X, Y) \in \mathcal{P}(S)^{2} \mid %s\}) \implies (%s)" % (self.condition[0], claim)
        if self.family == "map":
            return r"(f \colon S \to \mathcal{P}(S) \land E \subseteq S \land \forall x \in S\ (f(x) = %s)) \implies (%s)" % (self.rule[0], claim)
        listed = ", ".join("(%d, %d)" % pair for pair in self.pairs)
        composition = r"G = \{(x, z) \mid \exists y\ ((x, y) \in G_{1} \land (y, z) \in G_{1})\}"
        return r"(G_{1} = \{%s\} \land %s) \implies (%s)" % (listed, composition, claim)

    def holds(self, S, E):
        """Il valore della tesi per l'insieme S e la parte E: un conto
        completo, con le parti di S tutte. Per un elenco di coppie S ed E non
        contano: l'enunciato parla solo dei numeri che scrive."""
        if self.family == "list":
            return self.property[1](set(self.pairs)) != self.negated
        P = parts(S)
        if self.family == "relation":
            R = {(X, Y) for X in P for Y in P if self.condition[1](X, Y, E, S)}
            value = self.property[1](R, P)
        else:
            f = {x: frozenset(self.rule[1](x, E, S)) for x in S}
            value = self.property[1](f, S, P)
        return value != self.negated

    def counterexample(self):
        for n in range(WINDOW + 1):
            S = frozenset(range(n))
            for E in parts(S):
                if not self.holds(S, E):
                    return "S = %s, E = %s" % (sorted(S), sorted(E))
        return None


def main():
    cases = [Case() for _ in range(COUNT)]
    texts = [case.tex() for case in cases]
    path = os.path.join(WORK, "fuzz_parts_%d.tex" % SEED)
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
    tally = {}
    for number, (case, text, line) in enumerate(zip(cases, texts, lines), 1):
        answer = line.split(". ", 1)[1]
        verdict = "P" if answer.startswith("DIMOSTRATO") else "R" if answer.startswith("FALSO") else "N" if answer.startswith("NON DIMOSTRATO") else "U"
        tally[(case.family, verdict)] = tally.get((case.family, verdict), 0) + 1
        found = case.counterexample()
        if verdict == "P" and found:
            print("BUG %d [%s]: proved, but false for %s\n      %s" % (number, answer[:60], found, text))
        elif verdict == "R" and not found:
            print("BUG %d [%s]: refuted, but true for every S of at most %d elements\n      %s" % (number, answer[:60], WINDOW, text))
    print("seed %d:" % SEED, ", ".join("%s %s=%d" % (k[0], k[1], v) for k, v in sorted(tally.items())))


main()
