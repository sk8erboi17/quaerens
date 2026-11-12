"""Fuzzing differenziale degli enunciati con quantificatori.

Uso: python3 -I fuzz_maps.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Genera enunciati a caso (applicazioni tra insiemi numerici con le quattro
proprietà iniettiva / suriettiva e negazioni; formule quantificate qualsiasi),
li fa decidere al programma e confronta ogni verdetto con un oracolo scritto
qui, indipendente dal programma:

  - collisioni f(a) = f(b) cercate su una finestra di numeri;
  - controimmagini calcolate in modo esatto, con le radici razionali;
  - formule qualsiasi valutate per forza bruta su finestre di numeri.

Stampa BUG quando il verdetto contraddice un fatto certo, SUSPECT quando
contraddice solo ciò che si vede sulla finestra.
"""
import os
import random
import re
import subprocess
import sys
from fractions import Fraction as Fr

MR, SEED, COUNT, WORK = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
rnd = random.Random(SEED)


def ftex(v):
    v = Fr(v)
    if v.denominator == 1:
        return str(v.numerator)
    return ("-" if v < 0 else "") + r"\frac{%d}{%d}" % (abs(v.numerator), v.denominator)


class NS:
    """Un insieme numerico: N, N0, Z, Q, Q+ meno alcuni punti."""
    BASE = {"N": r"\mathbb{N}", "N0": r"\mathbb{N}_{0}", "Z": r"\mathbb{Z}", "Q": r"\mathbb{Q}", "Q+": r"\mathbb{Q}^{+}"}

    def __init__(self, kind, holes=()):
        self.kind, self.holes = kind, tuple(holes)

    def base_has(self, v):
        v = Fr(v)
        if self.kind in ("N", "N0", "Z") and v.denominator != 1:
            return False
        if self.kind == "N" and v < 1:
            return False
        if self.kind == "N0" and v < 0:
            return False
        if self.kind == "Q+" and v <= 0:
            return False
        return True

    def has(self, v):
        return self.base_has(v) and Fr(v) not in self.holes

    def tex(self):
        text = self.BASE[self.kind]
        if self.holes:
            text += r" \setminus \{" + ", ".join(ftex(h) for h in self.holes) + r"\}"
        return text

    def window(self, size):
        """Elementi dell'insieme: interi in [-size, size], razionali di altezza fino a size."""
        if self.kind in ("N", "N0", "Z"):
            return [Fr(i) for i in range(-size, size + 1) if self.has(i)]
        out = set()
        for q in range(1, size + 1):
            for p in range(-size, size + 1):
                if self.has(Fr(p, q)):
                    out.add(Fr(p, q))
        return sorted(out)


def random_set():
    s = NS(rnd.choice(["N", "N0", "Z", "Z", "Q", "Q", "Q+"]))
    if rnd.random() < 0.3:
        pool = [Fr(k) for k in (-2, -1, 0, 1, 2, 3)] + [Fr(1, 2), Fr(-1, 2), Fr(3, 2)]
        pool = [h for h in pool if s.base_has(h)]
        rnd.shuffle(pool)
        s = NS(s.kind, pool[:rnd.choice([1, 1, 2])])
    return s


# --- Polinomi in una variabile, come liste di coefficienti interi ---------------
def ptex(c, x):
    terms = []
    for k in range(len(c) - 1, -1, -1):
        if c[k] == 0:
            continue
        mono = "" if k == 0 else (x if k == 1 else "%s^{%d}" % (x, k))
        size = abs(c[k])
        body = (str(size) if (size != 1 or k == 0) else "") + mono
        terms.append(("-" if c[k] < 0 else "+", body))
    if not terms:
        return "0"
    text = ("-" if terms[0][0] == "-" else "") + terms[0][1]
    for sign, body in terms[1:]:
        text += " %s %s" % (sign, body)
    return text


def peval(c, x):
    value = Fr(0)
    for k in reversed(c):
        value = value * x + k
    return value


def rational_roots(c):
    """Radici razionali di un polinomio a coefficienti razionali; None se è il polinomio nullo."""
    c = [Fr(k) for k in c]
    while c and c[-1] == 0:
        c.pop()
    if not c:
        return None
    scale = 1
    for k in c:
        scale = scale * k.denominator // __import__("math").gcd(scale, k.denominator)
    c = [int(k * scale) for k in c]
    roots = set()
    while len(c) > 1 and c[0] == 0:
        roots.add(Fr(0))
        c = c[1:]
    if len(c) == 1:
        return roots

    def divisors(n):
        n = abs(n)
        return [d for d in range(1, int(n ** 0.5) + 1) if n % d == 0] + [n // d for d in range(1, int(n ** 0.5) + 1) if n % d == 0]

    for p in divisors(c[0]):
        for q in divisors(c[-1]):
            for sign in (1, -1):
                r = Fr(sign * p, q)
                if peval(c, r) == 0:
                    roots.add(r)
    return roots


class MapCase:
    """f: A -> B, f(x) = P(x)/Q(x), con una tesi su iniettività e suriettività."""

    def __init__(self):
        self.A, self.B = random_set(), random_set()
        self.x = rnd.choice("nxtm")
        small = lambda: rnd.choice([-3, -2, -1, -1, 0, 0, 1, 1, 1, 2, 3, 4, 5])
        shape = rnd.random()
        if shape < 0.45:
            self.P = [small() for _ in range(rnd.choice([2, 2, 3, 3, 4]))]
            self.Q = [rnd.choice([1, 1, 1, 2, 2, 3, 4, 6])]
        elif shape < 0.8:
            self.P, self.Q = [small(), small()], [small(), small()]
        else:
            self.P = [small() for _ in range(rnd.choice([2, 3]))]
            self.Q = [small() for _ in range(rnd.choice([2, 3]))]
        if not any(self.Q):
            self.Q = [1]

        # Spesso si sceglie il codominio in modo che la regola definisca davvero
        # un'applicazione: altrimenti quasi tutti i casi si fermano lì.
        if rnd.random() < 0.8:
            for _ in range(40):
                if self.ill_defined() is None:
                    break
                self.B = random_set() if rnd.random() < 0.7 else NS("Q")
        self.claim = rnd.choice(["inj", "ninj", "surj", "nsurj", "inj&surj", "inj&nsurj", "ninj&surj", "ninj&nsurj"])

    def f(self, x):
        den = peval(self.Q, x)
        return None if den == 0 else peval(self.P, x) / den

    def body_tex(self):
        if self.Q == [1]:
            return ptex(self.P, self.x)
        return r"\frac{%s}{%s}" % (ptex(self.P, self.x), ptex(self.Q, self.x))

    def tex(self):
        A, B = self.A.tex(), self.B.tex()
        parts = {
            "inj": r"\forall a, b \in %s\ (f(a) = f(b) \implies a = b)" % A,
            "surj": r"\forall y \in %s\ \exists x \in %s\ (f(x) = y)" % (B, A),
        }
        parts["ninj"] = r"\lnot (%s)" % parts["inj"]
        parts["nsurj"] = r"\lnot (%s)" % parts["surj"]
        thesis = " \\land ".join("(%s)" % parts[p] for p in self.claim.split("&"))
        return r"(f \colon %s \to %s \land \forall %s \in %s\ (f(%s) = %s)) \implies (%s)" % (A, B, self.x, A, self.x, self.body_tex(), thesis)

    # --- Oracolo ---------------------------------------------------------------
    def ill_defined(self):
        for x in self.A.window(40):
            v = self.f(x)
            if v is None or not self.B.has(v):
                return x
        return None

    def collision(self):
        seen = {}
        for x in self.A.window(60 if self.A.kind in ("N", "N0", "Z") else 12):
            v = self.f(x)
            if v in seen:
                return seen[v], x
            seen[v] = x
        return None

    def preimages(self, y):
        """Tutte le controimmagini di y in A (esatto), o 'all' se ogni x lo è."""
        width = max(len(self.P), len(self.Q))
        c = [(self.P[k] if k < len(self.P) else 0) - y * (self.Q[k] if k < len(self.Q) else 0) for k in range(width)]
        roots = rational_roots(c)
        if roots is None:
            return "all"
        return [r for r in roots if self.A.has(r) and peval(self.Q, r) != 0]

    def missed(self):
        for y in self.B.window(30 if self.B.kind in ("N", "N0", "Z") else 9):
            if not self.preimages(y):
                return y
        return None

    def check(self, verdict, detail):
        """Ritorna una lista di (gravità, messaggio)."""
        out = []
        decided = verdict in ("P", "R")
        bad = self.ill_defined()
        if bad is not None:
            if decided:
                out.append(("BUG", "rule is not a map at %s=%s but verdict is %s" % (self.x, bad, verdict)))
            return out
        if not decided:
            return out
        collision, missed = self.collision(), self.missed()
        parts = self.claim.split("&")
        if verdict == "P":
            for p in parts:
                if p == "inj" and collision:
                    out.append(("BUG", "proved injective but f(%s) = f(%s)" % collision))
                if p == "surj" and missed is not None:
                    out.append(("BUG", "proved surjective but %s has no preimage" % missed))
                if p == "ninj" and not collision:
                    out.append(("SUSPECT", "proved not injective but no collision in the window"))
                if p == "nsurj" and missed is None:
                    out.append(("SUSPECT", "proved not surjective but every y of the window has a preimage"))
            return out

        # Verdetto FALSO: i numeri riportati dicono quale parte è stata smentita.
        values = dict((name, Fr(v)) for name, v in re.findall(r"(\w) = (-?\d+(?:/\d+)?)", detail or ""))
        if "a" in values and "b" in values and "inj" in parts:
            a, b = values["a"], values["b"]
            if not (self.A.has(a) and self.A.has(b) and a != b and self.f(a) == self.f(b)):
                out.append(("BUG", "refuted injectivity with a=%s, b=%s, which is not a collision" % (a, b)))
        elif "y" in values and "surj" in parts:
            y = values["y"]
            if not self.B.has(y) or self.preimages(y):
                out.append(("BUG", "refuted surjectivity with y=%s, which has a preimage or is outside B" % y))
        else:
            # Nessun numero: è stata dimostrata la proprietà negata da una parte "n...".
            possible = [p for p in parts if p in ("ninj", "nsurj")]
            if not possible:
                out.append(("BUG", "refuted without a counterexample: %r" % detail))
            elif all((p == "ninj" and collision) or (p == "nsurj" and missed is not None) for p in possible):
                out.append(("BUG", "refuted a negation that is true (collision=%s, missed=%s)" % (collision, missed)))
        return out


# --- Formule quantificate qualsiasi ---------------------------------------------
MONOS = [(), ("x",), ("y",), ("z",), ("x", "x"), ("x", "y"), ("y", "y"), ("x", "z"), ("y", "z"), ("x", "x", "x")]


class GenericCase:
    def __init__(self):
        count = rnd.choice([1, 2, 2, 2, 3])
        self.vars = list("xyz")[:count]
        self.quant = [rnd.choice("AE") for _ in self.vars]
        self.sets = [random_set() for _ in self.vars]
        self.matrix = self.random_matrix(rnd.choice([0, 1, 1, 2]))

    def random_poly(self):
        monos = [m for m in MONOS if all(v in self.vars for v in m)]
        poly = {}
        for _ in range(rnd.choice([1, 2, 2, 3])):
            poly[rnd.choice(monos)] = rnd.choice([-3, -2, -1, 1, 1, 2, 3, 4])
        return poly

    def random_matrix(self, depth):
        if depth == 0:
            return (rnd.choice(["=", "=", "!="]), self.random_poly(), self.random_poly())
        op = rnd.choice(["and", "or", "imp", "iff", "not"])
        if op == "not":
            return ("not", self.random_matrix(depth - 1))
        return (op, self.random_matrix(depth - 1), self.random_matrix(depth - 1))

    @staticmethod
    def poly_tex(poly):
        terms = []
        for mono, c in sorted(poly.items(), key=lambda t: (-len(t[0]), t[0])):
            letters = ""
            for v in sorted(set(mono)):
                k = mono.count(v)
                letters += v if k == 1 else "%s^{%d}" % (v, k)
            size = abs(c)
            terms.append(("-" if c < 0 else "+", (str(size) if (size != 1 or not letters) else "") + letters))
        text = ("-" if terms[0][0] == "-" else "") + terms[0][1]
        for sign, body in terms[1:]:
            text += " %s %s" % (sign, body)
        return text

    def matrix_tex(self, m):
        if m[0] in ("=", "!="):
            return "%s %s %s" % (self.poly_tex(m[1]), "=" if m[0] == "=" else r"\neq", self.poly_tex(m[2]))
        if m[0] == "not":
            return r"\lnot (%s)" % self.matrix_tex(m[1])
        op = {"and": r"\land", "or": r"\lor", "imp": r"\implies", "iff": r"\iff"}[m[0]]
        return "(%s) %s (%s)" % (self.matrix_tex(m[1]), op, self.matrix_tex(m[2]))

    def tex(self):
        prefix = "".join(r"%s %s \in %s\ " % (r"\forall" if q == "A" else r"\exists", v, s.tex())
                         for v, q, s in zip(self.vars, self.quant, self.sets))
        return prefix + "(" + self.matrix_tex(self.matrix) + ")"

    def holds(self, m, env):
        if m[0] in ("=", "!="):
            def value(poly):
                total = Fr(0)
                for mono, c in poly.items():
                    term = Fr(c)
                    for v in mono:
                        term *= env[v]
                    total += term
                return total
            return (value(m[1]) == value(m[2])) == (m[0] == "=")
        if m[0] == "not":
            return not self.holds(m[1], env)
        a = self.holds(m[1], env)
        if m[0] == "and":
            return a and self.holds(m[2], env)
        if m[0] == "or":
            return a or self.holds(m[2], env)
        if m[0] == "imp":
            return (not a) or self.holds(m[2], env)
        return a == self.holds(m[2], env)

    def window_truth(self, claim, level=0, env=None):
        """Verità sulla finestra dell'enunciato (claim True) o della sua negazione (claim False).

        Le finestre sono larghe per le variabili che, in ciò che si vuole
        confermare, sono esistenziali, e strette per le universali: così un
        "falso" contraddice davvero il verdetto del programma."""
        env = env or {}
        if level == len(self.vars):
            return self.holds(self.matrix, env) == claim
        s = self.sets[level]
        universal = (self.quant[level] == "A") == claim
        integers = s.kind in ("N", "N0", "Z")
        window = s.window((7 if integers else 3) if universal else (60 if integers else 10))
        for v in window:
            env[self.vars[level]] = v
            result = self.window_truth(claim, level + 1, env)
            if result != universal:
                return result
        return universal

    def check(self, verdict, detail):
        out = []
        if verdict not in ("P", "R"):
            return out
        claim = verdict == "P"
        kinds = set(self.quant)
        if not self.window_truth(claim):
            certain = (claim and kinds == {"A"}) or (not claim and kinds == {"E"})
            out.append(("BUG" if certain else "SUSPECT", "verdict %s is not confirmed on the window" % verdict))
        if verdict == "R" and kinds == {"A"}:
            values = dict((name, Fr(v)) for name, v in re.findall(r"(\w) = (-?\d+(?:/\d+)?)", detail or ""))
            if set(values) == set(self.vars):
                inside = all(s.has(values[v]) for v, s in zip(self.vars, self.sets))
                if not inside or self.holds(self.matrix, values):
                    out.append(("BUG", "counterexample %s does not falsify the statement" % detail))
        return out


def main():
    cases = [MapCase() if rnd.random() < 0.6 else GenericCase() for _ in range(COUNT)]
    path = os.path.join(WORK, "fuzz_maps_%d.tex" % SEED)
    with open(path, "w", encoding="utf-8") as out:
        out.write("\\begin{enumerate}\n")
        for case in cases:
            out.write("\\item $\\displaystyle %s$\n" % case.tex())
        out.write("\\end{enumerate}\n")
    run = subprocess.run([MR, path], capture_output=True, text=True, timeout=1200)
    lines = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
    if len(lines) != len(cases) or run.returncode not in (0, 1):
        print("BUG: the program answered %d of %d statements (exit %d)\n%s" % (len(lines), len(cases), run.returncode, run.stderr[-2000:]))
        return
    tally = {}
    for number, (case, line) in enumerate(zip(cases, lines), 1):
        text = line.split(". ", 1)[1]
        verdict = "P" if text.startswith("DIMOSTRATO") else "R" if text.startswith("FALSO") else "N" if text.startswith("NON DIMOSTRATO") else "U"
        detail = text.split("controesempio ", 1)[1] if "controesempio " in text else None
        key = (type(case).__name__, verdict)
        tally[key] = tally.get(key, 0) + 1
        if verdict == "U" and "memoria" in text:
            print("NOTE %d: %s\n      %s" % (number, text, case.tex()))
        for severity, message in case.check(verdict, detail):
            print("%s %d [%s]: %s\n      %s" % (severity, number, text[:60], message, case.tex()))
    print("seed %d:" % SEED, ", ".join("%s %s=%d" % (k[0][:3], k[1], v) for k, v in sorted(tally.items())))


main()
