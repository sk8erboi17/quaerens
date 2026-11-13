"""Fuzzing differenziale della notazione dei libri sugli insiemi (17p, 17q, 28h).

Uso: python3 -I fuzz_book_notation.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Genera enunciati scritti come nei libri su due insiemi qualsiasi S e T, con
un'applicazione f di S in T, parti X, Y di S e Z, W di T, e una relazione R
su S:

    (f \\colon S \\to T \\land R \\subseteq S \\times S \\land ipotesi)
        \\implies \\forall X, Y \\subseteq S\\ \\forall Z, W \\subseteq T\\ (tesi)

La tesi confronta, con =, \\subseteq, \\subsetneq o \\neq, due insiemi fatti
di immagini f(E) e controimmagini f^{-1}(E), unioni, intersezioni e
differenze; oppure due relazioni fatte di R, della sua inversa
\\{(y, x) \\mid (x, y) \\in E\\}, di composizioni \\{(x, z) \\mid \\exists y\\
((x, y) \\in E \\land (y, z) \\in F)\\}, della diagonale \\{(x, x) \\mid x \\in
S\\}, di unioni e intersezioni. Le ipotesi, scelte a caso: f iniettiva, f
suriettiva (scritta f(S) = T), R riflessiva, simmetrica, transitiva (scritta
come inclusione della composizione in R). Così vengono enunciati veri e
falsi, in proporzioni simili.

L'oracolo non usa nulla del programma: scrive ogni insieme come un insieme
di Python, calcolato dalla definizione (un'immagine è l'insieme dei valori,
una composizione l'insieme delle coppie che si concatenano), e prova ogni
modello con pochi elementi: ogni applicazione, ogni relazione, ogni parte.

  - Un modello in cui le ipotesi valgono e la tesi no è un controesempio
    certo: se il programma ha risposto DIMOSTRATO è un BUG.
  - Se il programma risponde FALSO, l'oracolo rilegge il controesempio
    stampato e vi ricalcola l'enunciato com'è scritto: se lì le ipotesi non
    valgono, o la tesi vale, è un BUG. È il controllo che serve contro uno
    svolgimento sbagliato: il programma ragiona sull'enunciato svolto, e
    l'oracolo su quello scritto.
  - Alcuni enunciati dell'elenco TRAPS, su un'applicazione di S in sé,
    valgono in ogni insieme finito ma sono falsi in generale (iniettiva
    implica suriettiva): il programma non deve dimostrarli. Se lo fa è un
    BUG.

SUSPECT: FALSO con un controesempio che l'oracolo non riesce a rileggere, o
DIMOSTRATO su un enunciato che l'oracolo non ha potuto provare per intero
fino a tre elementi: va guardato a mano.
"""
import itertools
import os
import random
import re
import subprocess
import sys

MR, SEED, COUNT, WORK = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
rnd = random.Random(SEED)
ORACLE_MODELS = 60000  # modelli provati al più per una scelta delle grandezze

# Insiemi di elementi di S: ("X",), ("Y",), ("S",), ("empty",), ("pre", e) con
# e di T, ("cup"|"cap"|"minus", a, b). Di T: ("Z",), ("W",), ("T",), ("empty",),
# ("img", e) con e di S, e le stesse operazioni. Relazioni su S: ("R",),
# ("inv", e), ("comp", e, g), ("diag",), ("cupr"|"capr", e, g).


def part(side, depth):
    """Un insieme di elementi di S (side "S") o di T, a caso."""
    leaves = [("X",), ("Y",), ("S",), ("empty",)] if side == "S" else [("Z",), ("W",), ("T",), ("empty",)]
    if depth == 0 or rnd.random() < 0.3:
        return rnd.choice(leaves[:2] * 3 + leaves[2:])
    kind = rnd.choice(["cup", "cap", "minus", "map", "map"])
    if kind == "map":
        return ("pre", part("T", depth - 1)) if side == "S" else ("img", part("S", depth - 1))
    return (kind, part(side, depth - 1), part(side, depth - 1))


def relation(depth):
    """Una relazione su S, a caso."""
    if depth == 0 or rnd.random() < 0.3:
        return rnd.choice([("R",), ("R",), ("R",), ("diag",)])
    kind = rnd.choice(["inv", "comp", "comp", "cupr", "capr"])
    if kind == "inv":
        return ("inv", relation(depth - 1))
    return (kind, relation(depth - 1), relation(depth - 1))


def tex_part(e):
    k = e[0]
    if k in ("X", "Y", "Z", "W", "S", "T"):
        return k
    if k == "empty":
        return "\\emptyset"
    if k == "pre":
        return "f^{-1}(%s)" % tex_part(e[1])
    if k == "img":
        return "f(%s)" % tex_part(e[1])
    op = {"cup": "\\cup", "cap": "\\cap", "minus": "\\setminus"}[k]
    return "(%s %s %s)" % (tex_part(e[1]), op, tex_part(e[2]))


def tex_relation(e):
    k = e[0]
    if k == "R":
        return "R"
    if k == "diag":
        return "\\{(x, x) \\mid x \\in S\\}"
    if k == "inv":
        return "\\{(y, x) \\mid (x, y) \\in %s\\}" % tex_relation(e[1])
    if k == "comp":
        return "\\{(x, z) \\mid \\exists y\\ ((x, y) \\in %s \\land (y, z) \\in %s)\\}" % (tex_relation(e[1]), tex_relation(e[2]))
    return "(%s %s %s)" % (tex_relation(e[1]), "\\cup" if k == "cupr" else "\\cap", tex_relation(e[2]))


def value_part(e, m):
    """Il valore dell'insieme e nel modello m: un frozenset di elementi."""
    k = e[0]
    if k in ("X", "Y", "Z", "W"):
        return m[k]
    if k == "S":
        return frozenset(range(m["s"]))
    if k == "T":
        return frozenset(range(m["t"]))
    if k == "empty":
        return frozenset()
    if k == "pre":
        inside = value_part(e[1], m)
        return frozenset(x for x in range(m["s"]) if m["f"][x] in inside)
    if k == "img":
        return frozenset(m["f"][x] for x in value_part(e[1], m))
    a, b = value_part(e[1], m), value_part(e[2], m)
    return a | b if k == "cup" else a & b if k == "cap" else a - b


def value_relation(e, m):
    """Il valore della relazione e nel modello m: un frozenset di coppie."""
    k = e[0]
    if k == "R":
        return m["R"]
    if k == "diag":
        return frozenset((x, x) for x in range(m["s"]))
    if k == "inv":
        return frozenset((y, x) for x, y in value_relation(e[1], m))
    a = value_relation(e[1], m)
    b = value_relation(e[2], m)
    if k == "comp":
        return frozenset((x, z) for x, y in a for w, z in b if y == w)
    return a | b if k == "cupr" else a & b


def compare(relation_name, a, b):
    if relation_name == "=":
        return a == b
    if relation_name == "neq":
        return a != b
    if relation_name == "sub":
        return a <= b
    return a <= b and a != b


TEX_REL = {"=": "=", "neq": "\\neq", "sub": "\\subseteq", "psub": "\\subsetneq"}

HYPOTHESES = {
    "injective": ("\\forall a, b \\in S\\ (f(a) = f(b) \\implies a = b)", lambda m: len(set(m["f"])) == m["s"]),
    "surjective": ("f(S) = T", lambda m: set(m["f"]) == set(range(m["t"]))),
    "reflexive": ("\\forall x \\in S\\ ((x, x) \\in R)", lambda m: all((x, x) in m["R"] for x in range(m["s"]))),
    "symmetric": ("\\{(y, x) \\mid (x, y) \\in R\\} \\subseteq R", lambda m: all((y, x) in m["R"] for x, y in m["R"])),
    "transitive": ("\\{(x, z) \\mid \\exists y\\ ((x, y) \\in R \\land (y, z) \\in R)\\} \\subseteq R",
                   lambda m: all((x, z) in m["R"] for x, y in m["R"] for w, z in m["R"] if y == w)),
}

# Enunciati su un'applicazione di S in sé che valgono in ogni insieme finito
# e sono falsi su un insieme infinito: nessun modello finito li smentisce.
TRAPS = [
    "(f \\colon S \\to S \\land \\forall a, b \\in S\\ (f(a) = f(b) \\implies a = b)) \\implies (f(S) = S)",
    "(f \\colon S \\to S \\land f(S) = S) \\implies (\\forall a, b \\in S\\ (f(a) = f(b) \\implies a = b))",
    "(f \\colon S \\to S \\land \\forall a, b \\in S\\ (f(a) = f(b) \\implies a = b) \\land u \\in S) \\implies (u \\in f(S))",
    "(f \\colon S \\to S \\land \\forall a, b \\in S\\ (f(a) = f(b) \\implies a = b)) \\implies (S \\subseteq f(S))",
]


def used(e, found):
    """Aggiunge a found i nomi delle parti e delle relazioni che e usa."""
    if e[0] in ("X", "Y", "Z", "W", "R"):
        found.add(e[0])
    for sub in e[1:]:
        used(sub, found)


class Case:
    def __init__(self):
        self.trap = None
        if rnd.random() < 0.06:
            self.trap = rnd.choice(TRAPS)
            return
        self.kind = rnd.choice(["parts", "parts", "relations"])
        relation_name = rnd.choice(["=", "=", "sub", "psub", "neq"])
        if self.kind == "parts":
            side = rnd.choice(["S", "T"])
            self.left, self.right = part(side, 2), part(side, 2)
        else:
            self.left, self.right = relation(2), relation(2)
        self.relation = relation_name
        found = set()
        used(self.left, found)
        used(self.right, found)
        self.parts = [p for p in ("X", "Y", "Z", "W") if p in found]
        self.has_r = "R" in found or self.kind == "relations"
        names = ["injective", "surjective"] + (["reflexive", "symmetric", "transitive"] if self.has_r else [])
        self.hyps = rnd.sample(names, rnd.choice([0, 0, 1, 1, 2]))
        self.has_r = self.has_r or any(h in ("reflexive", "symmetric", "transitive") for h in self.hyps)

    def tex(self):
        if self.trap:
            return self.trap
        parts = ["f \\colon S \\to T"] + (["R \\subseteq S \\times S"] if self.has_r else []) + [HYPOTHESES[h][0] for h in self.hyps]
        if self.kind == "parts":
            body = "%s %s %s" % (tex_part(self.left), TEX_REL[self.relation], tex_part(self.right))
        else:
            body = "%s %s %s" % (tex_relation(self.left), TEX_REL[self.relation], tex_relation(self.right))
        prefix = ""
        of_s, of_t = [p for p in self.parts if p in "XY"], [p for p in self.parts if p in "ZW"]
        if of_s:
            prefix += "\\forall %s \\subseteq S\\ " % ", ".join(of_s)
        if of_t:
            prefix += "\\forall %s \\subseteq T\\ " % ", ".join(of_t)
        return "(%s) \\implies %s(%s)" % (" \\land ".join(parts), prefix, body)

    def holds(self, m):
        """1 se nel modello m le ipotesi valgono e la tesi vale, 0 se le
        ipotesi valgono e la tesi no, None se le ipotesi non valgono."""
        if not all(HYPOTHESES[h][1](m) for h in self.hyps):
            return None
        value = value_part if self.kind == "parts" else value_relation
        return compare(self.relation, value(self.left, m), value(self.right, m))

    def models(self, s, t):
        """Tutti i modelli con s elementi in S e t in T; None se sono troppi."""
        subsets_s = [frozenset(c) for k in range(s + 1) for c in itertools.combinations(range(s), k)]
        subsets_t = [frozenset(c) for k in range(t + 1) for c in itertools.combinations(range(t), k)]
        pairs = [(x, y) for x in range(s) for y in range(s)]
        relations = [frozenset(c) for k in range(len(pairs) + 1) for c in itertools.combinations(pairs, k)] if self.has_r else [frozenset()]
        choices = [list(itertools.product(range(t), repeat=s)), relations]
        names = ["f", "R"]
        for p in self.parts:
            choices.append(subsets_s if p in "XY" else subsets_t)
            names.append(p)
        count = 1
        for c in choices:
            count *= len(c)
        if count > ORACLE_MODELS:
            return None
        return (dict(zip(names, combination), s=s, t=t) for combination in itertools.product(*choices))

    def search(self):
        """Il primo controesempio, e se la ricerca fino a tre elementi è stata completa."""
        complete = True
        for s in (1, 2, 3):
            for t in (1, 2, 3):
                candidates = self.models(s, t)
                if candidates is None:
                    complete = False
                    continue
                for m in candidates:
                    if self.holds(m) == 0:
                        return m, complete
        return None, complete

    def printed(self, text):
        """Il controesempio stampato dal programma, o None se non si riesce a rileggerlo."""
        def element(name, letter):
            return int(re.fullmatch(letter + r"_\{(\d+)\}", name.strip()).group(1))

        def listed(name, letter):
            found = re.search(name + r" = \\\{(.*?)\\\}", text)
            if not found:
                return None
            return frozenset(element(x, letter) for x in re.findall(letter + r"_\{\d+\}", found.group(1)))

        try:
            s = len(listed("S", "s"))
            t = len(listed("T", "t"))
            m = {"s": s, "t": t, "f": [None] * s, "R": frozenset()}
            for a, b in re.findall(r"f\((s_\{\d+\})\) = (t_\{\d+\})", text):
                m["f"][element(a, "s")] = element(b, "t")
            if self.has_r:
                found = re.search(r"R = \\\{(.*?)\\\}", text)
                m["R"] = frozenset((element(a, "s"), element(b, "s")) for a, b in re.findall(r"\((s_\{\d+\}), (s_\{\d+\})\)", found.group(1)))
            for p in self.parts:
                m[p] = listed(p, "s" if p in "XY" else "t")
                if m[p] is None:
                    return None
            return None if None in m["f"] else m
        except (AttributeError, ValueError, TypeError):
            return None

    def check(self, verdict, text):
        if self.trap:
            if verdict == "P":
                return [("BUG", "proved, but it is false on an infinite set")]
            if verdict == "R":
                return [("BUG", "refuted, but it holds on every finite set")]
            return []
        model, complete = self.search()
        if verdict == "P" and model:
            return [("BUG", "proved, but it fails in %s" % model)]
        if verdict == "P" and not complete:
            return [("SUSPECT", "proved, and the oracle could not try every model up to three elements")]
        if verdict == "R":
            shown = self.printed(text)
            if shown is None:
                return [("SUSPECT", "refuted, but the counterexample printed could not be read back")]
            if self.holds(shown) != 0:
                return [("BUG", "the counterexample printed does not falsify the statement as written: %s" % shown)]
        return []


def main():
    cases = [Case() for _ in range(COUNT)]
    texts = [case.tex() for case in cases]
    path = os.path.join(WORK, "fuzz_book_notation_%d.tex" % SEED)
    with open(path, "w", encoding="utf-8") as out:
        out.write("\\begin{enumerate}\n")
        for text in texts:
            out.write("\\item $\\displaystyle %s$\n" % text)
        out.write("\\end{enumerate}\n")
    run = subprocess.run([MR, path], capture_output=True, text=True, timeout=6000)
    lines = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
    if len(lines) != len(cases) or run.returncode not in (0, 1):
        print("BUG: the program answered %d of %d statements (exit %d)\n%s" % (len(lines), len(cases), run.returncode, run.stderr[-2000:]))
        return
    if "ATTENZIONE" in run.stdout:
        print("SUSPECT: the program reports steps that failed their independent check:\n%s" % run.stdout.splitlines()[-1])
    tally = {}
    for number, (case, text, line) in enumerate(zip(cases, texts, lines), 1):
        answer = line.split(". ", 1)[1]
        verdict = "P" if answer.startswith("DIMOSTRATO") else "R" if answer.startswith("FALSO") else "N" if answer.startswith("NON DIMOSTRATO") else "U"
        key = ("trap" if case.trap else case.kind, verdict)
        tally[key] = tally.get(key, 0) + 1
        for severity, message in case.check(verdict, answer):
            print("%s %d [%s]: %s\n      %s" % (severity, number, answer[:70], message, text))
    print("seed %d:" % SEED, ", ".join("%s %s=%d" % (k[0], k[1], v) for k, v in sorted(tally.items())))


main()
