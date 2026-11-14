"""Fuzzing differenziale delle dimostrazioni con le istanze scelte da un modello.

Uso: python3 -I fuzz_model_instances.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Genera enunciati delle tre famiglie le cui dimostrazioni chiedono le istanze
del modello candidato (paragrafo 28r di main.c), scritti come nei libri:

  - famiglie di parti costruite da partizioni: la controimmagine
    \\{f^{-1}(Y) \\mid Y \\in F\\} (con o senza il filtro f^{-1}(Y) \\neq
    \\emptyset), l'immagine \\{f(X) \\mid X \\in F\\} (con f iniettiva,
    suriettiva, o nessuna delle due), la partizione comune \\{X \\cap Y \\mid X
    \\in F_{1} \\land Y \\in F_{2}\\} (con o senza il filtro X \\cap Y \\neq
    \\emptyset); delle ipotesi "\\emptyset \\notin F", "disgiunti", "coprono"
    se ne tiene una scelta a caso, e la tesi è una delle stesse proprietà, o
    "ogni pezzo sta in un pezzo di F_{1}", o "è F_{1}", affermata o negata;
  - un ordine tra le parti (non vuote, o tutte) di un insieme con una
    relazione \\le di cui si suppone una scelta di riflessiva, antisimmetrica,
    transitiva: X R Y se X = Y (o no) oppure Q x \\in X\\ Q' y \\in Y\\ (x \\le
    y), con Q e Q' "per ogni" o "esiste" e i due lati scambiati o no; la tesi
    è riflessiva, antisimmetrica, transitiva o totale, affermata o negata;
  - la composizione G di due relazioni R_{1}, R_{2} su S, e H quella
    nell'altro ordine, con una scelta di proprietà di R_{1} e R_{2}; la tesi
    è G riflessiva, simmetrica, transitiva, G = H, G = R_{1}, o le
    implicazioni del libro tra queste, affermata o negata.

Li fa decidere al programma e confronta ogni verdetto con un oracolo scritto
qui, che non chiama mai il programma e non svolge nessuna notazione: per ogni
scelta degli insiemi con al più WINDOW elementi, vuoti compresi, e di
applicazioni, famiglie e relazioni, calcola l'enunciato con gli insiemi di
Python, dalle definizioni. Un controesempio trovato così è certo. La sua
assenza no; ma i modelli del programma (28a, 28m) hanno insiemi di al più tre
elementi quando ci sono le loro parti, dentro la finestra: un FALSO senza
controesempio è allora un errore certo. Nella terza famiglia non ci sono
parti, il programma prova anche insiemi di quattro elementi, e un FALSO senza
controesempio nella finestra è solo SUSPECT, da guardare a mano.

Stampa BUG quando il verdetto contraddice un fatto certo: un DIMOSTRATO con un
controesempio, un FALSO senza (fuori dalla terza famiglia).
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


def parts(S):
    items = sorted(S)
    return [frozenset(c) for k in range(len(items) + 1) for c in itertools.combinations(items, k)]


def families(S):
    """Tutte le famiglie di parti di S, come frozenset di frozenset."""
    P = parts(S)
    return [frozenset(c) for k in range(len(P) + 1) for c in itertools.combinations(P, k)]


def maps(S, T):
    S, T = sorted(S), sorted(T)
    for values in itertools.product(T, repeat=len(S)):
        yield dict(zip(S, values))


def relations(S):
    pairs = [(a, b) for a in sorted(S) for b in sorted(S)]
    for bits in range(1 << len(pairs)):
        yield frozenset(p for i, p in enumerate(pairs) if bits >> i & 1)


# --- Le proprietà di una famiglia G di parti di U ---------------------------

def no_empty(G, U):
    return frozenset() not in G


def disjoint(G, U):
    return all(X == Y or not (X & Y) for X in G for Y in G)


def covers(G, U):
    return frozenset().union(*G) == U if G else not U


FAMILY_HYPOTHESES = [
    (r"\emptyset \notin %s", no_empty),
    (r"\forall X, Y \in %s\ (X \neq Y \implies X \cap Y = \emptyset)", disjoint),
    (r"\bigcup_{X \in %s} X = %s", covers),
]


def family_tex(name, universe, kept):
    """Le ipotesi kept (indici in FAMILY_HYPOTHESES) sulla famiglia name di
    parti di universe."""
    pieces = [r"%s \subseteq \mathcal{P}(%s)" % (name, universe)]
    for i in kept:
        text = FAMILY_HYPOTHESES[i][0]
        pieces.append(text % (name, universe) if i == 2 else text % name)
    return "(" + r" \land ".join(pieces) + ")"


def family_holds(G, U, kept):
    return all(FAMILY_HYPOTHESES[i][1](G, U) for i in kept)


class FamilyCase:
    """Una famiglia costruita da partizioni, e una sua proprietà."""

    def __init__(self):
        self.kind = rnd.choice(["preimage", "image", "refinement"])
        self.negated = rnd.random() < 0.25
        self.filtered = rnd.random() < 0.6
        self.kept = [i for i in range(3) if rnd.random() < 0.8]
        self.kept2 = [i for i in range(3) if rnd.random() < 0.8]
        self.injective = rnd.random() < 0.5
        self.surjective = rnd.random() < 0.4
        self.nonempty = rnd.random() < 0.5
        claims = ["no_empty", "disjoint", "covers", "points"]
        if self.kind == "refinement":
            claims += ["inside", "equal"]
        if self.kind == "image":
            self.target = rnd.choice(["T", "f(S)"])
        self.claim = rnd.choice(claims)

    def name(self):
        return "F" if self.kind == "refinement" else "F^{*}"

    def universe(self):
        return self.target if self.kind == "image" else "S"

    def tex(self):
        G, U = self.name(), self.universe()
        hyp = []
        if self.nonempty:
            hyp.append(r"S \neq \emptyset")
        if self.kind == "preimage":
            hyp.append(r"f \colon S \to T")
            hyp.append(family_tex("F", "T", self.kept))
            rule = r"f^{-1}(Y) \mid Y \in F" + (r" \land f^{-1}(Y) \neq \emptyset" if self.filtered else "")
            hyp.append(r"F^{*} = \{%s\}" % rule)
        elif self.kind == "image":
            hyp.append(r"f \colon S \to T")
            if self.injective:
                hyp.append(r"\forall a, b \in S\ (f(a) = f(b) \implies a = b)")
            if self.surjective:
                hyp.append(r"\forall y \in T\ \exists x \in S\ (f(x) = y)")
            hyp.append(family_tex("F", "S", self.kept))
            hyp.append(r"F^{*} = \{f(X) \mid X \in F\}")
        else:
            hyp.append(family_tex("F_{1}", "S", self.kept))
            hyp.append(family_tex("F_{2}", "S", self.kept2))
            rule = r"X \cap Y \mid X \in F_{1} \land Y \in F_{2}" + (r" \land X \cap Y \neq \emptyset" if self.filtered else "")
            hyp.append(r"F = \{%s\}" % rule)
        claim = {
            "no_empty": r"\emptyset \notin %s" % G,
            "disjoint": r"\forall X, Y \in %s\ (X \neq Y \implies X \cap Y = \emptyset)" % G,
            "covers": r"\bigcup_{X \in %s} X = %s" % (G, U),
            "points": r"\forall a \in S\ \exists X \in %s\ (a \in X)" % G,
            "inside": r"\forall Z \in F\ \exists X \in F_{1}\ (Z \subseteq X)",
            "equal": r"F = F_{1}",
        }[self.claim]
        if self.kind == "image" and self.claim == "points":
            claim = r"\forall a \in S\ \exists X \in F^{*}\ (f(a) \in X)"
        if self.negated:
            claim = r"\lnot (%s)" % claim
        return r"(%s) \implies (%s)" % (r" \land ".join(hyp), claim)

    def value(self, S, T, f, F, F2):
        """Il valore della tesi, None se le ipotesi non valgono."""
        if self.nonempty and not S:
            return None
        if self.kind == "preimage":
            if not family_holds(F, T, self.kept):
                return None
            G = set()
            for Y in F:
                pre = frozenset(x for x in S if f[x] in Y)
                if pre or not self.filtered:
                    G.add(pre)
            U = S
        elif self.kind == "image":
            if self.injective and len(set(f.values())) != len(S):
                return None
            if self.surjective and set(f.values()) != set(T):
                return None
            if not family_holds(F, S, self.kept):
                return None
            G = {frozenset(f[x] for x in X) for X in F}
            U = frozenset(T) if self.target == "T" else frozenset(f.values())
        else:
            if not family_holds(F, S, self.kept) or not family_holds(F2, S, self.kept2):
                return None
            G = {X & Y for X in F for Y in F2 if (X & Y) or not self.filtered}
            U = S
        G = frozenset(G)
        result = {
            "no_empty": lambda: no_empty(G, U),
            "disjoint": lambda: disjoint(G, U),
            "covers": lambda: covers(G, U),
            "points": lambda: all(any((f[a] if self.kind == "image" else a) in X for X in G) for a in S),
            "inside": lambda: all(any(Z <= X for X in F) for Z in G),
            "equal": lambda: G == F,
        }[self.claim]()
        return result != self.negated

    def counterexample(self):
        for n in range(WINDOW + 1):
            S = frozenset(range(n))
            if self.kind == "refinement":
                fams = families(S)
                for F in fams:
                    if not family_holds(F, S, self.kept):
                        continue
                    for F2 in fams:
                        if self.value(S, frozenset(), {}, F, F2) is False:
                            return "S = %s, F1 = %s, F2 = %s" % (sorted(S), show(F), show(F2))
                continue
            for m in range(WINDOW + 1):
                T = frozenset(range(10, 10 + m))
                fams = families(T if self.kind == "preimage" else S)
                for f in maps(S, T):
                    for F in fams:
                        if self.value(S, T, f, F, None) is False:
                            return "S = %s, T = %s, f = %s, F = %s" % (sorted(S), sorted(T), f, show(F))
        return None


def show(F):
    return "{" + ", ".join(str(sorted(X)) for X in sorted(F, key=sorted)) + "}"


# --- Un ordine tra le parti --------------------------------------------------

ORDER_HYPOTHESES = [
    (r"\forall x \in S\ ((x, x) \in \le)", lambda L, S: all((x, x) in L for x in S)),
    (r"\forall x, y\ (((x, y) \in \le \land (y, x) \in \le) \implies x = y)", lambda L, S: all(x == y for (x, y) in L if (y, x) in L)),
    (r"\forall x, y, z\ (((x, y) \in \le \land (y, z) \in \le) \implies (x, z) \in \le)",
     lambda L, S: all((x, w) in L for (x, y) in L for (z, w) in L if y == z)),
]


class OrderCase:
    """Un ordine tra le parti di S costruito da \\le, e una sua proprietà."""

    def __init__(self):
        self.negated = rnd.random() < 0.25
        self.kept = [0] + [i for i in (1, 2) if rnd.random() < 0.85]
        self.nonempty_parts = rnd.random() < 0.7
        self.equal = rnd.random() < 0.8
        self.quantifiers = (rnd.choice(["forall", "exists"]), rnd.choice(["forall", "exists"]))
        self.swapped = rnd.random() < 0.3
        self.claim = rnd.choice(["reflexive", "antisymmetric", "transitive", "total"])

    def tex(self):
        hyp = [r"{\le} \subseteq S \times S", "(" + r" \land ".join(ORDER_HYPOTHESES[i][0] for i in self.kept) + ")"]
        A = "A" if self.nonempty_parts else r"\mathcal{P}(S)"
        if self.nonempty_parts:
            hyp.append(r"A = \mathcal{P}(S) \setminus \{\emptyset\}")
        compare = r"y \le x" if self.swapped else r"x \le y"
        rule = r"\%s x \in X\ \%s y \in Y\ (%s)" % (self.quantifiers[0], self.quantifiers[1], compare)
        if self.equal:
            rule = "X = Y \\lor " + rule
        hyp.append(r"R = \{(X, Y) \in %s^{2} \mid %s\}" % (A, rule))
        claim = {
            "reflexive": r"\forall x \in %s\ ((x, x) \in R)" % A,
            "antisymmetric": r"\forall x, y\ (((x, y) \in R \land (y, x) \in R) \implies x = y)",
            "transitive": r"\forall x, y, z\ (((x, y) \in R \land (y, z) \in R) \implies (x, z) \in R)",
            "total": r"\forall x, y \in %s\ ((x, y) \in R \lor (y, x) \in R)" % A,
        }[self.claim]
        if self.negated:
            claim = r"\lnot (%s)" % claim
        return r"(%s) \implies (%s)" % (r" \land ".join(hyp), claim)

    def value(self, S, L):
        if not all(ORDER_HYPOTHESES[i][1](L, S) for i in self.kept):
            return None
        A = [X for X in parts(S) if X or not self.nonempty_parts]

        def below(x, y):
            return ((y, x) if self.swapped else (x, y)) in L

        def related(X, Y):
            if self.equal and X == Y:
                return True
            outer = all if self.quantifiers[0] == "forall" else any
            inner = all if self.quantifiers[1] == "forall" else any
            return outer(inner(below(x, y) for y in Y) for x in X)

        R = {(X, Y) for X in A for Y in A if related(X, Y)}
        result = {
            "reflexive": lambda: all((X, X) in R for X in A),
            "antisymmetric": lambda: all(X == Y for (X, Y) in R if (Y, X) in R),
            "transitive": lambda: all((X, W) in R for (X, Y) in R for (Z, W) in R if Y == Z),
            "total": lambda: all((X, Y) in R or (Y, X) in R for X in A for Y in A),
        }[self.claim]()
        return result != self.negated

    def counterexample(self):
        for n in range(WINDOW + 1):
            S = frozenset(range(n))
            for L in relations(S):
                if self.value(S, L) is False:
                    return "S = %s, <= = %s" % (sorted(S), sorted(L))
        return None


# --- La composizione di due relazioni ----------------------------------------

def rel_props(name):
    return [
        (r"\forall x \in S\ ((x, x) \in %s)" % name, lambda R, S: all((x, x) in R for x in S)),
        (r"\forall x, y\ ((x, y) \in %s \implies (y, x) \in %s)" % (name, name), lambda R, S: all((y, x) in R for (x, y) in R)),
        (r"\forall x, y, z\ (((x, y) \in %s \land (y, z) \in %s) \implies (x, z) \in %s)" % (name, name, name),
         lambda R, S: all((x, w) in R for (x, y) in R for (z, w) in R if y == z)),
    ]


def compose(A, B):
    return frozenset((x, w) for (x, y) in A for (z, w) in B if y == z)


class CompositionCase:
    """La composizione di due relazioni su S, e una sua proprietà."""

    CLAIMS = {
        "reflexive": r"\forall x \in S\ ((x, x) \in G)",
        "symmetric": r"\forall x, y\ ((x, y) \in G \implies (y, x) \in G)",
        "transitive": r"\forall x, y, z\ (((x, y) \in G \land (y, z) \in G) \implies (x, z) \in G)",
        "commute": r"G = H",
        "first": r"G = R_{1}",
        "sym_commute": r"\forall x, y\ ((x, y) \in G \implies (y, x) \in G) \implies G = H",
        "commute_trans": r"G = H \implies \forall x, y, z\ (((x, y) \in G \land (y, z) \in G) \implies (x, z) \in G)",
        "commute_sym": r"G = H \implies \forall x, y\ ((x, y) \in G \implies (y, x) \in G)",
    }

    def __init__(self):
        self.negated = rnd.random() < 0.25
        self.kept1 = [i for i in range(3) if rnd.random() < 0.85]
        self.kept2 = [i for i in range(3) if rnd.random() < 0.85]
        if not self.kept1:
            self.kept1 = [0]
        if not self.kept2:
            self.kept2 = [1]
        self.claim = rnd.choice(sorted(self.CLAIMS))

    def tex(self):
        p1, p2 = rel_props("R_{1}"), rel_props("R_{2}")
        hyp = [r"S \neq \emptyset", r"R_{1} \subseteq S \times S", r"R_{2} \subseteq S \times S",
               "(" + r" \land ".join(p1[i][0] for i in self.kept1) + ")",
               "(" + r" \land ".join(p2[i][0] for i in self.kept2) + ")",
               r"G = \{(x, z) \mid \exists y\ ((x, y) \in R_{1} \land (y, z) \in R_{2})\}",
               r"H = \{(x, z) \mid \exists y\ ((x, y) \in R_{2} \land (y, z) \in R_{1})\}"]
        claim = self.CLAIMS[self.claim]
        claim = r"\lnot (%s)" % claim if self.negated else "(%s)" % claim
        return r"(%s) \implies %s" % (r" \land ".join(hyp), claim)

    def value(self, S, R1, R2):
        p = rel_props("")
        if not S or not all(p[i][1](R1, S) for i in self.kept1) or not all(p[i][1](R2, S) for i in self.kept2):
            return None
        G, H = compose(R1, R2), compose(R2, R1)
        sym = lambda R: all((y, x) in R for (x, y) in R)
        trans = lambda R: all((x, w) in R for (x, y) in R for (z, w) in R if y == z)
        result = {
            "reflexive": lambda: all((x, x) in G for x in S),
            "symmetric": lambda: sym(G),
            "transitive": lambda: trans(G),
            "commute": lambda: G == H,
            "first": lambda: G == R1,
            "sym_commute": lambda: not sym(G) or G == H,
            "commute_trans": lambda: G != H or trans(G),
            "commute_sym": lambda: G != H or sym(G),
        }[self.claim]()
        return result != self.negated

    def counterexample(self):
        p = rel_props("")
        for n in range(1, WINDOW + 1):
            S = frozenset(range(n))
            first = [R for R in relations(S) if all(p[i][1](R, S) for i in self.kept1)]
            second = [R for R in relations(S) if all(p[i][1](R, S) for i in self.kept2)]
            for R1 in first:
                for R2 in second:
                    if self.value(S, R1, R2) is False:
                        return "S = %s, R1 = %s, R2 = %s" % (sorted(S), sorted(R1), sorted(R2))
        return None


def main():
    makers = [FamilyCase, OrderCase, CompositionCase]
    cases = [rnd.choice(makers)() for _ in range(COUNT)]
    texts = [case.tex() for case in cases]
    path = os.path.join(WORK, "fuzz_model_instances_%d.tex" % SEED)
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
    if "ATTENZIONE" in run.stdout:
        print("BUG: the program reports steps that failed their independent check")
    tally = {}
    for number, (case, text, line) in enumerate(zip(cases, texts, lines), 1):
        answer = line.split(". ", 1)[1]
        verdict = "P" if answer.startswith("DIMOSTRATO") else "R" if answer.startswith("FALSO") else "N" if answer.startswith("NON DIMOSTRATO") else "U"
        family = type(case).__name__
        tally[(family, verdict)] = tally.get((family, verdict), 0) + 1
        if verdict not in ("P", "R"):
            continue
        found = case.counterexample()
        if verdict == "P" and found:
            print("BUG %d [%s]: proved, but false for %s\n      %s" % (number, answer[:60], found, text))
        elif verdict == "R" and not found:
            label = "SUSPECT" if isinstance(case, CompositionCase) else "BUG"
            print("%s %d [%s]: refuted, but true for every choice with at most %d elements\n      %s" % (label, number, answer[:90], WINDOW, text))
    print("seed %d:" % SEED, ", ".join("%s %s=%d" % (k[0], k[1], v) for k, v in sorted(tally.items())))


main()
