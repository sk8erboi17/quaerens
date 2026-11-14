"""Fuzzing differenziale dei quantificatori su parti usati come ipotesi (17u, 28i-28l).

Uso: python3 -I fuzz_part_hypotheses.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Genera enunciati su un'applicazione f di S in T (o di S in sé) in cui un
quantificatore sulle parti, \\forall X, Y \\subseteq S o \\forall Z \\subseteq T\\
\\exists X \\subseteq S, sta dove non vale quanto una dichiarazione: su un
lato di un'equivalenza, a sinistra di un'implicazione, sotto un "esiste".
Le tre forme, con E una proprietà degli elementi (f iniettiva, f
suriettiva, S non vuoto, f(S) = T) e P la proprietà delle parti:

    f \\colon S \\to T \\implies (E \\iff P)
    (f \\colon S \\to T \\land P) \\implies E
    (f \\colon S \\to T \\land E) \\implies P

P confronta, con =, \\subseteq o \\neq, due insiemi fatti di parti legate,
di S, T, \\emptyset, immagini f(A), controimmagini f^{-1}(A), unioni,
intersezioni e differenze; a volte è un'implicazione tra due confronti.
Così nascono le caratterizzazioni dei libri, vere, e le loro varianti
sbagliate, in proporzioni simili.

L'oracolo non usa nulla del programma: calcola ogni insieme come un
frozenset di Python, dalla definizione, e prova ogni modello con al più tre
elementi per insieme, ANCHE VUOTI (il programma non prova mai un insieme
vuoto tra i suoi modelli: è proprio lì che una dimostrazione che inventa un
elemento cadrebbe), ogni applicazione e, per un quantificatore su parti,
ogni sottoinsieme.

  - Un modello in cui l'enunciato è falso è un controesempio certo: se il
    programma ha risposto DIMOSTRATO è un BUG.
  - Se il programma risponde FALSO, l'oracolo rilegge il controesempio
    stampato e vi ricalcola l'enunciato com'è scritto: se lì vale, è un BUG.
  - Gli enunciati dell'elenco TRAPS, su un'applicazione di S in sé, valgono
    in ogni insieme finito e sono falsi su uno infinito (iniettiva implica
    suriettiva, scritto con le parti): se il programma li dimostra è un BUG;
    se li dichiara falsi, anche.

SUSPECT: FALSO con un controesempio che l'oracolo non riesce a rileggere.
"""
import itertools
import os
import random
import re
import subprocess
import sys

MR, SEED, COUNT, WORK = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
rnd = random.Random(SEED)

PARTS_OF = {"S": ["X", "Y"], "T": ["Z", "W"]}

# Proprietà degli elementi: il testo e il calcolo sul modello m (m["s"], m["t"]
# elementi, m["f"] la tabella di f).
PROPERTIES = {
    "injective": ("\\forall a, b \\in S\\ (f(a) = f(b) \\implies a = b)", lambda m: len(set(m["f"])) == m["s"]),
    "surjective": ("\\forall y \\in T\\ \\exists x \\in S\\ (f(x) = y)", lambda m: set(m["f"]) == set(range(m["t"]))),
    "image": ("f(S) = T", lambda m: set(m["f"]) == set(range(m["t"]))),
    "nonempty": ("\\exists x \\in S\\ (x = x)", lambda m: m["s"] > 0),
}

# Enunciati su un'applicazione di S in sé che valgono in ogni insieme finito
# e sono falsi su un insieme infinito (n -> n + 1 e n -> n / 2 sui naturali).
TRAPS = [
    "(f \\colon S \\to S \\land \\forall a, b \\in S\\ (f(a) = f(b) \\implies a = b)) \\implies \\forall Y \\subseteq S\\ (f(f^{-1}(Y)) = Y)",
    "(f \\colon S \\to S \\land \\forall X, Y \\subseteq S\\ (f(X) \\subseteq f(Y) \\implies X \\subseteq Y)) \\implies \\forall y \\in S\\ \\exists x \\in S\\ (f(x) = y)",
    "(f \\colon S \\to S \\land \\forall Y \\subseteq S\\ (f(f^{-1}(Y)) = Y)) \\implies \\forall a, b \\in S\\ (f(a) = f(b) \\implies a = b)",
    "(f \\colon S \\to S \\land \\forall X \\subseteq S\\ (f(S \\setminus X) \\subseteq S \\setminus f(X))) \\implies \\forall Z \\subseteq S\\ \\exists X \\subseteq S\\ (f(X) = Z)",
]


def expression(side, depth, scope):
    """Un insieme di elementi di side ("S" o "T"), con le parti legate in
    scope (un dizionario da insieme a nomi)."""
    leaves = [("var", v) for v in scope.get(side, [])]
    others = [("all", side), ("empty", side)]
    if depth == 0 or rnd.random() < 0.35:
        return rnd.choice(leaves * 4 + others) if leaves else rnd.choice(others)
    kind = rnd.choice(["cup", "cap", "minus", "map", "map", "map"])
    if kind == "map":
        return ("img", expression("S", depth - 1, scope)) if side == "T" else ("pre", expression("T", depth - 1, scope))
    return (kind, expression(side, depth - 1, scope), expression(side, depth - 1, scope))


def tex_expression(e):
    k = e[0]
    if k == "var":
        return e[1]
    if k == "all":
        return e[1]
    if k == "empty":
        return "\\emptyset"
    if k == "img":
        return "f(%s)" % tex_expression(e[1])
    if k == "pre":
        return "f^{-1}(%s)" % tex_expression(e[1])
    op = {"cup": "\\cup", "cap": "\\cap", "minus": "\\setminus"}[k]
    return "(%s %s %s)" % (tex_expression(e[1]), op, tex_expression(e[2]))


def value(e, m, env):
    """Il valore dell'insieme e nel modello m, con le parti legate in env."""
    k = e[0]
    if k == "var":
        return env[e[1]]
    if k == "all":
        return frozenset(range(m["s" if e[1] == "S" else "t"]))
    if k == "empty":
        return frozenset()
    if k == "img":
        return frozenset(m["f"][x] for x in value(e[1], m, env))
    if k == "pre":
        inside = value(e[1], m, env)
        return frozenset(x for x in range(m["s"]) if m["f"][x] in inside)
    a, b = value(e[1], m, env), value(e[2], m, env)
    return a | b if k == "cup" else a & b if k == "cap" else a - b


REL_TEX = {"=": "=", "sub": "\\subseteq", "neq": "\\neq"}


def comparison(scope):
    side = rnd.choice(["S", "T"])
    return (rnd.choice(["=", "=", "sub", "sub", "neq"]), expression(side, 2, scope), expression(side, 2, scope))


def tex_comparison(c):
    return "%s %s %s" % (tex_expression(c[1]), REL_TEX[c[0]], tex_expression(c[2]))


def holds_comparison(c, m, env):
    a, b = value(c[1], m, env), value(c[2], m, env)
    return a == b if c[0] == "=" else a <= b if c[0] == "sub" else a != b


class PartProperty:
    """Un quantificatore su parti: prefix è un elenco di (quantificatore,
    insieme, nomi); body una confronto, o un'implicazione tra due."""

    def __init__(self):
        shape = rnd.choice(["forall", "forall", "forall", "forall_exists", "exists"])
        if shape == "forall":
            side = rnd.choice(["S", "S", "T"])
            names = PARTS_OF[side][:rnd.choice([1, 2])]
            self.prefix = [("forall", side, names)]
        elif shape == "exists":
            side = rnd.choice(["S", "T"])
            self.prefix = [("exists", side, PARTS_OF[side][:1])]
        else:
            outer = rnd.choice(["S", "T"])
            inner = "T" if outer == "S" else "S"
            self.prefix = [("forall", outer, PARTS_OF[outer][:1]), ("exists", inner, PARTS_OF[inner][:1])]
        scope = {}
        for _, side, names in self.prefix:
            scope.setdefault(side, []).extend(names)
        self.body = [comparison(scope)]
        if rnd.random() < 0.4:
            self.body.append(comparison(scope))

    def tex(self):
        text = ""
        for quantifier, side, names in self.prefix:
            text += "\\%s %s \\subseteq %s\\ " % (quantifier, ", ".join(names), side)
        if len(self.body) == 1:
            return text + "(%s)" % tex_comparison(self.body[0])
        return text + "(%s \\implies %s)" % (tex_comparison(self.body[0]), tex_comparison(self.body[1]))

    def holds(self, m):
        def subsets(side):
            n = m["s" if side == "S" else "t"]
            return [frozenset(c) for k in range(n + 1) for c in itertools.combinations(range(n), k)]

        def walk(level, env):
            if level == len(self.prefix):
                first = holds_comparison(self.body[0], m, env)
                return first if len(self.body) == 1 else (not first) or holds_comparison(self.body[1], m, env)
            quantifier, side, names = self.prefix[level]
            choices = itertools.product(subsets(side), repeat=len(names))
            results = (walk(level + 1, dict(env, **dict(zip(names, chosen)))) for chosen in choices)
            return all(results) if quantifier == "forall" else any(results)

        return walk(0, {})


class Case:
    def __init__(self):
        self.trap = None
        if rnd.random() < 0.08:
            self.trap = rnd.choice(TRAPS)
            return
        self.form = rnd.choice(["iff", "iff", "hypothesis", "conclusion"])
        self.property = rnd.choice(list(PROPERTIES))
        self.parts = PartProperty()
        self.into_itself = rnd.random() < 0.2

    def tex(self):
        if self.trap:
            return self.trap
        declaration = "f \\colon S \\to %s" % ("S" if self.into_itself else "T")
        e, p = PROPERTIES[self.property][0], self.parts.tex()
        if self.into_itself:
            e, p = e.replace("T", "S"), p.replace("T", "S")
        if self.form == "iff":
            return "%s \\implies ((%s) \\iff %s)" % (declaration, e, p)
        if self.form == "hypothesis":
            return "(%s \\land (%s)) \\implies (%s)" % (declaration, p, e)
        return "(%s \\land (%s)) \\implies (%s)" % (declaration, e, p)

    def holds(self, m):
        e, p = PROPERTIES[self.property][1](m), self.parts.holds(m)
        if self.form == "iff":
            return e == p
        if self.form == "hypothesis":
            return (not p) or e
        return (not e) or p

    def models(self):
        """Ogni modello con al più tre elementi per insieme, vuoti compresi."""
        for s in range(4):
            for t in ([s] if self.into_itself else range(4)):
                for f in itertools.product(range(t), repeat=s):
                    yield {"s": s, "t": t, "f": list(f)}

    def counterexample(self):
        for m in self.models():
            if not self.holds(m):
                return m
        return None

    def printed(self, text):
        """Il controesempio stampato dal programma, o None."""
        def listed(name, letter):
            found = re.search(r"\$" + name + r" = \\\{(.*?)\\\}", text)
            return None if not found else len(re.findall(letter + r"_\{\d+\}", found.group(1)))

        s = listed("S", "s")
        t = s if self.into_itself else listed("T", "t")
        if s is None or t is None:
            return None
        letter = "s" if self.into_itself else "t"
        f = [None] * s
        for a, b in re.findall(r"f\(s_\{(\d+)\}\) = " + letter + r"_\{(\d+)\}", text):
            if int(a) < s:
                f[int(a)] = int(b)
        return None if None in f else {"s": s, "t": t, "f": f}

    def check(self, verdict, text):
        if self.trap:
            if verdict == "P":
                return [("BUG", "proved, but it is false on an infinite set")]
            if verdict == "R":
                return [("BUG", "refuted, but it holds on every finite set")]
            return []
        if verdict == "P":
            model = self.counterexample()
            if model is not None:
                return [("BUG", "proved, but it fails in %s" % model)]
        if verdict == "R":
            shown = self.printed(text)
            if shown is None:
                return [("SUSPECT", "refuted, but the counterexample printed could not be read back")]
            if self.holds(shown):
                return [("BUG", "the counterexample printed does not falsify the statement as written: %s" % shown)]
        return []


def main():
    cases = [Case() for _ in range(COUNT)]
    texts = [case.tex() for case in cases]
    path = os.path.join(WORK, "fuzz_part_hypotheses_%d.tex" % SEED)
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
        key = ("trap" if case.trap else case.form, verdict)
        tally[key] = tally.get(key, 0) + 1
        for severity, message in case.check(verdict, answer):
            print("%s %d [%s]: %s\n      %s" % (severity, number, answer[:70], message, text))
    print("seed %d:" % SEED, ", ".join("%s %s=%d" % (k[0], k[1], v) for k, v in sorted(tally.items())))


main()
