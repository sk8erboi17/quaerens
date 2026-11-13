"""Fuzzing differenziale degli enunciati su insiemi qualsiasi (capitolo 28).

Uso: python3 -I fuzz_structures.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Genera enunciati su un insieme S qualsiasi, con un'operazione \\perp, due
applicazioni f e g, due elementi u e v, una relazione R e una parte X:

    (dichiarazioni \\land ipotesi) \\implies tesi

Le ipotesi sono proprietà scelte a caso da un elenco (associativa,
commutativa, elemento neutro a sinistra o a destra, iniettiva, suriettiva,
riflessiva, simmetrica, transitiva, parte chiusa, ...), e la tesi è un'altra
proprietà dello stesso elenco, o una loro conseguenza classica. La maggior
parte degli enunciati così ottenuti è falsa, alcuni sono veri.

L'oracolo non usa nessun ragionamento del programma: scrive ogni enunciato
come una funzione Python e lo calcola in ogni modello con pochi elementi,
cioè per ogni scelta delle tabelle dei simboli dichiarati. È una ricerca a
tappeto, scritta qui da capo.

  - Un modello in cui le ipotesi valgono e la tesi no è un controesempio
    certo: se il programma ha risposto DIMOSTRATO è un BUG.
  - Se il programma risponde FALSO, l'oracolo rilegge le tabelle del
    controesempio stampato e vi ricalcola l'enunciato con il proprio
    valutatore: se lì le ipotesi non valgono, o la tesi vale, è un BUG.
  - Alcuni enunciati dell'elenco TRAPS valgono in ogni insieme finito ma
    sono falsi in generale (un'applicazione iniettiva di un insieme in sé è
    suriettiva): nessun modello finito li smentisce, e il programma non deve
    dimostrarli. Se lo fa è un BUG.

SUSPECT: FALSO con un controesempio che l'oracolo non riesce a rileggere: va
guardato a mano.
"""
import itertools
import os
import random
import re
import subprocess
import sys

MR, SEED, COUNT, WORK = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
rnd = random.Random(SEED)
ORACLE_MODELS = 40000  # quanti modelli, al più, l'oracolo prova per una grandezza

# Termini: una lettera (variabile, oppure u, v), ("f", t), ("g", t), ("op", s, t).
# Formule: ("eq", s, t), ("rel", s, t), ("in", t), ("not", A), ("and", A, B),
# ("or", A, B), ("imp", A, B), ("all", lettere, A), ("ex", lettere, A),
# ("allx", lettere, A): per ogni elemento della parte X.


def op(s, t):
    return ("op", s, t)


def f(t):
    return ("f", t)


def g(t):
    return ("g", t)


def eq(s, t):
    return ("eq", s, t)


def rel(s, t):
    return ("rel", s, t)


def imp(a, b):
    return ("imp", a, b)


def every(letters, body):
    return ("all", letters, body)


# Le proprietà tra cui scegliere ipotesi e tesi.
PROPERTIES = {
    "associative": every("abc", eq(op(op("a", "b"), "c"), op("a", op("b", "c")))),
    "commutative": every("ab", eq(op("a", "b"), op("b", "a"))),
    "idempotent": every("a", eq(op("a", "a"), "a")),
    "u left neutral": every("a", eq(op("u", "a"), "a")),
    "u right neutral": every("a", eq(op("a", "u"), "a")),
    "v left neutral": every("a", eq(op("v", "a"), "a")),
    "v right neutral": every("a", eq(op("a", "v"), "a")),
    "left projection": every("ab", eq(op("a", "b"), "a")),
    "u = v": eq("u", "v"),
    "f injective": every("ab", imp(eq(f("a"), f("b")), eq("a", "b"))),
    "g injective": every("ab", imp(eq(g("a"), g("b")), eq("a", "b"))),
    "f surjective": every("b", ("ex", "a", eq(f("a"), "b"))),
    "g surjective": every("b", ("ex", "a", eq(g("a"), "b"))),
    "fg injective": every("ab", imp(eq(f(g("a")), f(g("b"))), eq("a", "b"))),
    "fg surjective": every("b", ("ex", "a", eq(f(g("a")), "b"))),
    "fg identity": every("a", eq(f(g("a")), "a")),
    "gf identity": every("a", eq(g(f("a")), "a")),
    "f involution": every("a", eq(f(f("a")), "a")),
    "f idempotent": every("a", eq(f(f("a")), f("a"))),
    "f identity": every("a", eq(f("a"), "a")),
    "f and g commute": every("a", eq(f(g("a")), g(f("a")))),
    "f homomorphism": every("ab", eq(f(op("a", "b")), op(f("a"), f("b")))),
    "f fixes u": eq(f("u"), "u"),
    "reflexive": every("a", rel("a", "a")),
    "symmetric": every("ab", imp(rel("a", "b"), rel("b", "a"))),
    "transitive": every("abc", imp(("and", rel("a", "b"), rel("b", "c")), rel("a", "c"))),
    "antisymmetric": every("ab", imp(("and", rel("a", "b"), rel("b", "a")), eq("a", "b"))),
    "total": every("ab", ("or", rel("a", "b"), rel("b", "a"))),
    "related implies reflexive": every("ab", imp(rel("a", "b"), rel("a", "a"))),
    "euclidean": every("abc", imp(("and", rel("a", "b"), rel("a", "c")), rel("b", "c"))),
    "f monotone": every("ab", imp(rel("a", "b"), rel(f("a"), f("b")))),
    "ff monotone": every("ab", imp(rel("a", "b"), rel(f(f("a")), f(f("b"))))),
    "X closed under op": ("allx", "ab", ("in", op("a", "b"))),
    "X closed under f": ("allx", "a", ("in", f("a"))),
    "X closed under ff": ("allx", "a", ("in", f(f("a")))),
    "X closed under triple": ("allx", "ab", ("in", op(op("a", "b"), "a"))),
    "u in X": ("in", "u"),
    "f(u) in X": ("in", f("u")),
}

# Veri in ogni insieme finito, falsi in generale: (ipotesi, tesi).
TRAPS = [
    (["f injective"], "f surjective"),
    (["f surjective"], "f injective"),
    (["fg identity"], "gf identity"),
    (["fg identity"], "f injective"),
    (["fg identity"], "g surjective"),
]

SYMBOLS = {"op": "\\perp \\colon S \\times S \\to S", "f": "f \\colon S \\to S", "g": "g \\colon S \\to S",
           "u": "u \\in S", "v": "v \\in S", "rel": "R \\subseteq S \\times S", "in": "X \\subseteq S"}


def used(node, found):
    """Aggiunge a found i simboli dichiarati che compaiono in node."""
    if isinstance(node, str):
        if node in "uv":
            found.add(node)
        return
    if node[0] in ("op", "f", "g", "rel", "in"):
        found.add(node[0])
    if node[0] == "allx":
        found.add("in")
    # Il secondo posto di un quantificatore sono le sue lettere, non un termine.
    for part in node[2:] if node[0] in ("all", "ex", "allx") else node[1:]:
        used(part, found)


def term_tex(t, tight=False):
    if isinstance(t, str):
        return t
    if t[0] == "op":
        text = "%s \\perp %s" % (term_tex(t[1], True), term_tex(t[2], True))
        return "(%s)" % text if tight else text
    return "%s(%s)" % (t[0], term_tex(t[1]))


def tex(node):
    kind = node[0]
    if kind == "eq":
        return "%s = %s" % (term_tex(node[1]), term_tex(node[2]))
    if kind == "rel":
        return "(%s, %s) \\in R" % (term_tex(node[1]), term_tex(node[2]))
    if kind == "in":
        return "%s \\in X" % term_tex(node[1])
    if kind == "not":
        return "\\lnot (%s)" % tex(node[1])
    if kind in ("and", "or", "imp"):
        word = {"and": "\\land", "or": "\\lor", "imp": "\\implies"}[kind]
        return "(%s) %s (%s)" % (tex(node[1]), word, tex(node[2]))
    quantifier = "\\exists" if kind == "ex" else "\\forall"
    return "%s %s \\in %s\\ (%s)" % (quantifier, ", ".join(node[1]), "X" if kind == "allx" else "S", tex(node[2]))


def value(t, model, env):
    """Il valore del termine t nel modello, con le variabili ai valori di env."""
    if isinstance(t, str):
        return env[t] if t in env else model[t]
    if t[0] == "op":
        return model["op"][value(t[1], model, env)][value(t[2], model, env)]
    return model[t[0]][value(t[1], model, env)]


def holds(node, model, env):
    kind = node[0]
    if kind == "eq":
        return value(node[1], model, env) == value(node[2], model, env)
    if kind == "rel":
        return model["rel"][value(node[1], model, env)][value(node[2], model, env)]
    if kind == "in":
        return model["in"][value(node[1], model, env)]
    if kind == "not":
        return not holds(node[1], model, env)
    if kind == "and":
        return holds(node[1], model, env) and holds(node[2], model, env)
    if kind == "or":
        return holds(node[1], model, env) or holds(node[2], model, env)
    if kind == "imp":
        return (not holds(node[1], model, env)) or holds(node[2], model, env)
    for values in itertools.product(range(model["size"]), repeat=len(node[1])):
        inner = dict(env)
        inner.update(zip(node[1], values))
        if kind == "allx" and not all(model["in"][x] for x in values):
            continue
        if holds(node[2], model, inner) != (kind != "ex"):
            return kind == "ex"
    return kind != "ex"


def models(symbols, size):
    """Tutti i modelli con `size` elementi per i simboli dati; None se sono troppi."""
    elements = range(size)
    shapes = {"op": size * size, "f": size, "g": size, "u": 1, "v": 1}
    count = 1
    for s in symbols:
        count *= (2 ** (size * size if s == "rel" else size)) if s in ("rel", "in") else size ** shapes[s]
    if count > ORACLE_MODELS:
        return None
    choices = []
    for s in symbols:
        if s == "op":
            choices.append([[list(t[i * size:(i + 1) * size]) for i in elements] for t in itertools.product(elements, repeat=size * size)])
        elif s in ("f", "g"):
            choices.append([list(t) for t in itertools.product(elements, repeat=size)])
        elif s in ("u", "v"):
            choices.append(list(elements))
        elif s == "rel":
            choices.append([[list(t[i * size:(i + 1) * size]) for i in elements] for t in itertools.product([False, True], repeat=size * size)])
        else:
            choices.append([list(t) for t in itertools.product([False, True], repeat=size)])
    return (dict(zip(symbols, combination), size=size) for combination in itertools.product(*choices))


def printed_model(text, symbols):
    """Il modello che il programma ha stampato come controesempio, o None se non si riesce a rileggerlo."""
    def element(name):
        return int(re.fullmatch(r"s_\{(\d+)\}", name.strip()).group(1))

    try:
        size = len(re.findall(r"s_\{\d+\}", re.search(r"S = \\\{(.*?)\\\}", text).group(1)))
        model = {"size": size}
        for s in symbols:
            if s == "op":
                model[s] = [[None] * size for _ in range(size)]
                for a, b, c in re.findall(r"(s_\{\d+\}) \\perp (s_\{\d+\}) = (s_\{\d+\})", text):
                    model[s][element(a)][element(b)] = element(c)
            elif s in ("f", "g"):
                model[s] = [None] * size
                for a, c in re.findall(s + r"\((s_\{\d+\})\) = (s_\{\d+\})", text):
                    model[s][element(a)] = element(c)
            elif s in ("u", "v"):
                model[s] = element(re.search(s + r" = (s_\{\d+\})", text).group(1))
            elif s == "rel":
                model[s] = [[False] * size for _ in range(size)]
                for a, b in re.findall(r"\((s_\{\d+\}), (s_\{\d+\})\)", re.search(r"R = \\\{(.*?)\\\}", text).group(1)):
                    model[s][element(a)][element(b)] = True
            else:
                model[s] = [False] * size
                for a in re.findall(r"s_\{\d+\}", re.search(r"X = \\\{(.*?)\\\}", text).group(1)):
                    model[s][element(a)] = True
        flat = [x for s in symbols if s in ("op", "f", "g") for row in model[s] for x in (row if isinstance(row, list) else [row])]
        return None if None in flat else model
    except (AttributeError, ValueError, IndexError):
        return None


class Case:
    def __init__(self):
        self.trap = rnd.random() < 0.08
        if self.trap:
            hyps, thesis = rnd.choice(TRAPS)
            self.hyps, self.thesis = list(hyps), thesis
        else:
            names = list(PROPERTIES)
            self.thesis = rnd.choice(names)
            self.hyps = rnd.sample([n for n in names if n != self.thesis], rnd.choice([0, 1, 1, 2, 2, 3]))
        found = set()
        for name in self.hyps + [self.thesis]:
            used(PROPERTIES[name], found)
        self.symbols = [s for s in ("op", "f", "g", "u", "v", "rel", "in") if s in found]

    def tex(self):
        parts = [SYMBOLS[s] for s in self.symbols] + [tex(PROPERTIES[name]) for name in self.hyps]
        parts = ["(%s)" % part if part.startswith(("\\forall", "\\exists")) else part for part in parts]
        thesis = tex(PROPERTIES[self.thesis])
        if not parts:
            return thesis
        return "(%s) \\implies %s" % (" \\land ".join(parts), thesis)

    def counter_model(self):
        """Il più piccolo controesempio, e la grandezza fino a cui la ricerca è stata completa."""
        complete = 0
        for size in (1, 2, 3):
            candidates = models(self.symbols, size)
            if candidates is None:
                break
            for model in candidates:
                if all(holds(PROPERTIES[name], model, {}) for name in self.hyps) and not holds(PROPERTIES[self.thesis], model, {}):
                    return model, complete
            complete = size
        return None, complete

    def check(self, verdict, text):
        model, complete = self.counter_model()
        if self.trap and model:
            return [("ORACLE", "a statement listed as true in every finite set fails in %s" % model)]
        if verdict == "P" and model:
            return [("BUG", "proved, but it fails in %s" % model)]
        if verdict == "P" and self.trap:
            return [("BUG", "proved, but it is false on an infinite set")]
        if verdict == "R":
            if self.trap:
                return [("BUG", "refuted with a finite model, but it holds in every finite set")]
            shown = printed_model(text, self.symbols)
            if shown is None:
                return [("SUSPECT", "refuted, but the counterexample printed could not be read back")]
            if not all(holds(PROPERTIES[name], shown, {}) for name in self.hyps) or holds(PROPERTIES[self.thesis], shown, {}):
                return [("BUG", "the counterexample printed does not falsify the statement: %s" % shown)]
            if not model and shown["size"] <= complete:
                return [("BUG", "refuted with %d elements, but the oracle found no model of that size" % shown["size"])]
        return []


def main():
    cases = [Case() for _ in range(COUNT)]
    texts = [case.tex() for case in cases]
    path = os.path.join(WORK, "fuzz_structures_%d.tex" % SEED)
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
        key = ("trap" if case.trap else "random", verdict)
        tally[key] = tally.get(key, 0) + 1
        for severity, message in case.check(verdict, answer):
            print("%s %d [%s]: %s\n      %s" % (severity, number, answer[:60], message, text))
    print("seed %d:" % SEED, ", ".join("%s %s=%d" % (k[0], k[1], v) for k, v in sorted(tally.items())))


main()
