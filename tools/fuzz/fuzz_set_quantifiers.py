"""Fuzzing differenziale degli enunciati sugli insiemi con più quantificatori.

Uso: python3 -I fuzz_set_quantifiers.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Genera enunciati su uno o due insiemi liberi, S e T, con da uno a tre
quantificatori "esiste" o "per ogni" su insiemi X, Y, Z, uno dentro l'altro,
davanti a una combinazione di uguaglianze e inclusioni. Ogni insieme
quantificato ha un limite, \\subseteq S oppure \\subseteq S \\cup T: così ciò
che sta fuori dagli insiemi liberi non conta, e un universo finito dice tutto.

L'oracolo è scritto qui e non chiama mai il programma: calcola davvero ogni
espressione, con gli insiemi di Python, per tutti gli insiemi liberi dentro
un universo di tre o quattro elementi, e per ogni quantificatore prova tutti
i sottoinsiemi del suo limite. Non conta gli elementi delle regioni e non sa
nulla di soglie.

  - Un controesempio trovato dall'oracolo è certo: se il programma ha
    dimostrato l'enunciato è un BUG.
  - Un controesempio stampato dal programma viene ricalcolato qui, sugli
    insiemi che il programma indica: se lì l'enunciato vale è un BUG.
  - Se il programma dichiara falso un enunciato senza che l'oracolo trovi un
    controesempio, e il suo è confermato, il controesempio ha più elementi
    dell'universo dell'oracolo: è corretto, e viene solo contato.
"""
import itertools
import os
import random
import re
import subprocess
import sys

MR, SEED, COUNT, WORK = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
rnd = random.Random(SEED)


def subsets(s):
    items = sorted(s)
    return [frozenset(c) for k in range(len(items) + 1) for c in itertools.combinations(items, k)]


class Case:
    def __init__(self):
        self.free = ["S"] if rnd.random() < 0.6 else ["S", "T"]
        self.bound = list("XYZ")[:rnd.choice([1, 2, 2, 2, 3, 3])]
        if len(self.free) + len(self.bound) > 4:
            self.bound = self.bound[:2]
        self.quant = [rnd.choice("AE") for _ in self.bound]
        self.limit = [rnd.choice([("S",), ("S",), ("S", "T")] if len(self.free) == 2 else [("S",)]) for _ in self.bound]
        self.matrix = self.random_formula(rnd.choice([1, 2, 2, 3]))

    def random_set(self, depth):
        letters = self.free + self.bound
        if depth == 0 or rnd.random() < 0.45:
            return rnd.choice(letters + letters + ["0"])
        return (rnd.choice(["cup", "cap", "minus"]), self.random_set(depth - 1), self.random_set(depth - 1))

    def random_formula(self, depth):
        if depth == 0:
            return (rnd.choice(["=", "sub", "neq"]), self.random_set(rnd.choice([0, 1, 1, 2])), self.random_set(rnd.choice([0, 1])))
        op = rnd.choice(["and", "and", "or", "imp", "iff", "not"])
        if op == "not":
            return ("not", self.random_formula(depth - 1))
        return (op, self.random_formula(depth - 1), self.random_formula(depth - 1))

    # --- Oracolo -------------------------------------------------------------
    def value(self, e, env):
        if isinstance(e, str):
            return frozenset() if e == "0" else env[e]
        a, b = self.value(e[1], env), self.value(e[2], env)
        return a | b if e[0] == "cup" else a & b if e[0] == "cap" else a - b

    def holds(self, f, env):
        if f[0] == "=":
            return self.value(f[1], env) == self.value(f[2], env)
        if f[0] == "neq":
            return self.value(f[1], env) != self.value(f[2], env)
        if f[0] == "sub":
            return self.value(f[1], env) <= self.value(f[2], env)
        if f[0] == "not":
            return not self.holds(f[1], env)
        a = self.holds(f[1], env)
        if f[0] == "and":
            return a and self.holds(f[2], env)
        if f[0] == "or":
            return a or self.holds(f[2], env)
        if f[0] == "imp":
            return (not a) or self.holds(f[2], env)
        return a == self.holds(f[2], env)

    def truth(self, env, level=0):
        """Verità dell'enunciato con gli insiemi liberi di env: ogni quantificatore prova tutti i sottoinsiemi del suo limite."""
        if level == len(self.bound):
            return self.holds(self.matrix, env)
        universal = self.quant[level] == "A"
        whole = frozenset().union(*(env[l] for l in self.limit[level]))
        for candidate in subsets(whole):
            env[self.bound[level]] = candidate
            if self.truth(env, level + 1) != universal:
                return not universal
        return universal

    def counterexample(self):
        """Una scelta degli insiemi liberi, dentro un universo piccolo, in cui l'enunciato è falso; None se non c'è."""
        universe = subsets(range(4 if len(self.free) == 1 else 3))
        for choice in itertools.product(universe, repeat=len(self.free)):
            if not self.truth(dict(zip(self.free, choice))):
                return choice
        return None

    # --- Scrittura -----------------------------------------------------------
    def set_tex(self, e):
        if isinstance(e, str):
            return r"\emptyset" if e == "0" else e
        op = {"cup": r"\cup", "cap": r"\cap", "minus": r"\setminus"}[e[0]]
        return "(%s %s %s)" % (self.set_tex(e[1]), op, self.set_tex(e[2]))

    def formula_tex(self, f):
        if f[0] in ("=", "neq", "sub"):
            rel = {"=": "=", "neq": r"\neq", "sub": r"\subseteq"}[f[0]]
            return "%s %s %s" % (self.set_tex(f[1]), rel, self.set_tex(f[2]))
        if f[0] == "not":
            return r"\lnot (%s)" % self.formula_tex(f[1])
        op = {"and": r"\land", "or": r"\lor", "imp": r"\implies", "iff": r"\iff"}[f[0]]
        return "(%s) %s (%s)" % (self.formula_tex(f[1]), op, self.formula_tex(f[2]))

    def tex(self):
        prefix = ""
        for letter, q, limit in zip(self.bound, self.quant, self.limit):
            where = " \\cup ".join(limit)
            # Le due scritture del limite: \subseteq A, oppure \in \mathcal{P}(A).
            bound = r"\in \mathcal{P}(%s)" % where if rnd.random() < 0.3 else r"\subseteq %s" % where
            prefix += r"%s %s %s\ " % (r"\forall" if q == "A" else r"\exists", letter, bound)
        return prefix + "(" + self.formula_tex(self.matrix) + ")"

    def check(self, verdict, text):
        if verdict not in ("P", "R"):
            return []
        found = self.counterexample()
        if verdict == "P":
            if found is not None:
                shown = ", ".join("%s = %s" % (l, sorted(v)) for l, v in zip(self.free, found))
                return [("BUG", "proved, but the statement fails for %s" % shown)]
            return []

        # FALSO: gli insiemi stampati dal programma, ricalcolati qui.
        sets = dict((name, frozenset(int(x) for x in re.findall(r"\d+", body)))
                    for name, body in re.findall(r"(\w) = \{([^}]*)\}", text))
        if not sets or not set(sets) <= set(self.free):
            return [("BUG", "refuted without the sets of a counterexample: %s" % text)]
        for letter in self.free:
            sets.setdefault(letter, frozenset())  # una lettera che l'enunciato non usa: va bene qualunque insieme
        if self.truth(dict(sets)):
            return [("BUG", "the counterexample does not falsify the statement: %s" % text)]
        return [] if found is not None else [("LARGE", "the counterexample is larger than the universe of the oracle: %s" % text)]


def main():
    cases = [Case() for _ in range(COUNT)]
    texts = [case.tex() for case in cases]
    path = os.path.join(WORK, "fuzz_set_quantifiers_%d.tex" % SEED)
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
        key = ("%d quantifiers" % len(case.bound), verdict)
        tally[key] = tally.get(key, 0) + 1
        if verdict in ("N", "U"):
            print("NOTE %d: %s\n      %s" % (number, answer[:110], text))
        for severity, message in case.check(verdict, answer):
            print("%s %d [%s]: %s\n      %s" % (severity, number, answer[:60], message, text))
    print("seed %d:" % SEED, ", ".join("%s %s=%d" % (k[0], k[1], v) for k, v in sorted(tally.items())))


main()
