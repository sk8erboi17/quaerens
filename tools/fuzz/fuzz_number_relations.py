"""Fuzzing differenziale dei divisori, delle potenze, delle coppie e dei reali
nelle relazioni tra numeri (regole 18w, 18x, 18y di main.c e 17t di parser.c).

Uso: python3 -I fuzz_number_relations.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Genera enunciati di sei famiglie, li fa decidere al programma e confronta
ogni verdetto con un oracolo scritto qui, che non chiama mai il programma e
non svolge nessuna regola del motore:

  - divisori: "per ogni d che divide c, d è tra questi numeri", "esiste un
    divisore di c diverso da questi", "nessun d diverso da 1 divide sia c sia
    c'", "ogni divisore di c divide c'", affermati o negati. L'oracolo prova
    tutti gli interi da -|c| a |c|: un divisore di un numero diverso da zero
    non è più grande di lui in valore assoluto, quindi il conto è esatto;
  - potenze: R = \\{(a, b) \\in A^{2} \\mid \\exists n \\in B\\ (a = k^{n + s} b)\\},
    anche con 2n al posto di n, e le proprietà riflessiva, simmetrica,
    transitiva, o l'appartenenza di una coppia di numeri. L'oracolo calcola
    l'insieme E degli esponenti k^{e} ammessi e decide le proprietà da E: tra
    gli elementi di A ogni rapporto k^{e}, con e intero, si realizza, quindi
    la riflessiva è "0 sta in E", la simmetrica "E = -E", la transitiva
    "E + E sta in E". L'appartenenza di una coppia la calcola dividendo;
  - coppie: R = \\{((x_1, y_1), (x_2, y_2)) \\in (S^{2})^{2} \\mid p = q\\}, con p
    e q polinomi di grado al più due nelle quattro componenti, e le stesse
    proprietà con le lettere delle coppie, o l'appartenenza di due coppie di
    numeri. L'oracolo prova le proprietà sulle coppie di una finestra: un
    controesempio è certo, la sua assenza no;
  - radici: "per ogni x reale, x è razionale oppure x^2 + b x + c non è
    zero", vero solo se il polinomio non ha radici reali irrazionali: lo dice
    il discriminante, che deve essere negativo o un quadrato. Il programma
    prova solo numeri razionali, su cui l'enunciato vale sempre;
  - reali: R = \\{(a, b) \\in (\\mathbb{R}^{+})^{2} \\mid a^{i} b^{j} \\in
    \\mathbb{Q}\\}. L'oracolo prova le proprietà sui reali positivi della forma
    q 2^{u} 3^{v} \\pi^{w}, con u e v multipli di 1/60 e w intero: un tale
    numero è razionale esattamente quando u e v sono interi e w è zero, quindi
    a^{i} b^{j} è razionale quando lo sono i u_a + j u_b e i v_a + j v_b, e i
    w_a + j w_b è zero. Il programma prova solo numeri razionali, su cui
    queste relazioni valgono quasi sempre: qui l'oracolo vede gli irrazionali,
    e una proprietà dimostrata ma smentita da uno di loro è un errore certo.

  - multipli: R = \\{(x, y) \\in S^{2} \\mid \\exists a \\in T\\ (y = c a x)\\}, la
    divisibilità e le sue varianti, con le proprietà di un ordine, il minimo e
    il massimo. L'appartenenza di una coppia l'oracolo la calcola dividendo;
    le proprietà le prova su una finestra, e sa per certo quando 1 è il
    minimo (ogni y / c sta in T) e quando 0 è il massimo.

Un controesempio stampato dal programma viene ricalcolato qui: se non
smentisce la proprietà, e l'enunciato è una sola proprietà "per ogni", è un
errore certo.

Stampa BUG quando il verdetto contraddice un fatto certo, SUSPECT quando
contraddice solo ciò che si vede sulla finestra. I file che scrive vanno nella
cartella di lavoro, mai nel repository.
"""
import itertools
import os
import random
import re
import subprocess
import sys
from fractions import Fraction as Fr

MR, SEED, COUNT, WORK = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
rnd = random.Random(SEED)

SETS = {"N": r"\mathbb{N}", "N0": r"\mathbb{N}_{0}", "Z": r"\mathbb{Z}", "Q+": r"\mathbb{Q}^{+}"}


def in_set(kind, v):
    v = Fr(v)
    if kind in ("N", "N0", "Z") and v.denominator != 1:
        return False
    return {"N": v >= 1, "N0": v >= 0, "Z": True, "Q+": v > 0}[kind]


def divides(d, c):
    return c == 0 if d == 0 else c % d == 0


# --- Divisori --------------------------------------------------------------
class Divisors:
    def __init__(self):
        self.S = rnd.choice(["N", "N", "Z", "N0"])
        self.form = rnd.choice(["list", "other", "common", "chain"])
        self.c = rnd.choice([0] + [v for v in range(-60, 61) if v != 0] * 3)
        self.c2 = rnd.choice([v for v in range(-40, 41) if v != 0])
        self.negated = rnd.random() < 0.3
        sign = -1 if self.S == "Z" and rnd.random() < 0.3 else 1
        true_divisors = [d for d in range(1, abs(self.c) + 1) if self.c and self.c % d == 0]
        listed = set(true_divisors if rnd.random() < 0.5 else rnd.sample(true_divisors, max(0, len(true_divisors) - 1)) if true_divisors else [1])
        if self.S == "Z" and rnd.random() < 0.5:
            listed |= {-d for d in listed}
        if rnd.random() < 0.3:
            listed.add(rnd.randint(1, 12) * sign)
        self.listed = sorted(listed) or [1]

    def candidates(self):
        """Gli unici d che possono dividere c, se c non è zero; None se c è zero."""
        if self.c == 0:
            return None
        return [d for d in range(-abs(self.c), abs(self.c) + 1) if d != 0 and in_set(self.S, d)]

    def truth(self):
        """(vero?, certo?) dell'enunciato non negato."""
        cand, S = self.candidates(), self.S
        if self.form == "list":            # ogni divisore di c sta nell'elenco
            if cand is None:
                return False, True         # infiniti divisori, elenco finito
            return all(d in self.listed for d in cand if divides(d, self.c)), True
        if self.form == "other":           # esiste un divisore fuori dall'elenco
            if cand is None:
                return True, True
            return any(d not in self.listed for d in cand if divides(d, self.c)), True
        if self.form == "common":          # nessun d diverso da 1 divide c e c2
            cand = [d for d in range(-abs(self.c2), abs(self.c2) + 1) if d != 0 and in_set(S, d)]
            return not any(d != 1 and divides(d, self.c) and divides(d, self.c2) for d in cand), True
        if cand is None:                   # ogni divisore di c divide c2
            return False, True             # tutti i d dividono 0, non tutti c2
        return all(divides(d, self.c2) for d in cand if divides(d, self.c)), True

    def tex(self):
        S = SETS[self.S]
        if self.form == "list":
            body = r"\forall d \in %s\ (d \mid %d \implies (%s))" % (S, self.c, r" \lor ".join("d = %d" % v for v in self.listed))
        elif self.form == "other":
            body = r"\exists d \in %s\ (%s \land d \mid %d)" % (S, r" \land ".join(r"d \neq %d" % v for v in self.listed), self.c)
        elif self.form == "common":
            body = r"\forall d \in %s\ (\lnot (d \neq 1 \land d \mid %d \land d \mid %d))" % (S, self.c, self.c2)
        else:
            body = r"\forall d \in %s\ (d \mid %d \implies d \mid %d)" % (S, self.c, self.c2)
        return r"\lnot (%s)" % body if self.negated else body

    def check(self, verdict, detail):
        value, certain = self.truth()
        value = value != self.negated
        if verdict == "P" and not value:
            return [("BUG", "proved, but the statement is false")]
        if verdict == "R" and value:
            return [("BUG", "refuted, but the statement is true")]
        return []


# --- Potenze ---------------------------------------------------------------
class Powers:
    def __init__(self):
        self.A = rnd.choice(["N", "Z", "Q+"])
        self.B = rnd.choice(["Z", "Z", "N", "N0"])
        self.k = rnd.choice([2, 3, 6])
        self.s = rnd.choice([0, 0, 1, -1])
        self.step = rnd.choice([1, 1, 2])
        self.claims = [self.random_claim() for _ in range(rnd.choice([1, 1, 2]))]

    def random_claim(self):
        name = rnd.choice(["refl", "sym", "trans", "pair", "pair"])
        data = None
        if name == "pair":
            a = Fr(self.k) ** rnd.randint(-3, 3) * rnd.choice([1, 1, 5])
            b = rnd.choice([1, 1, 3, 5])
            if self.A != "Q+":
                a = Fr(int(a)) if a.denominator == 1 else Fr(rnd.randint(1, 40))
            data = (a, Fr(b), rnd.random() < 0.3)
        return name, rnd.random() < 0.3, data

    def exponent_ok(self, e):
        """e = step n + s per un n di B?"""
        if (e - self.s) % self.step:
            return False
        return in_set(self.B, (e - self.s) // self.step)

    def related(self, a, b):
        if not (in_set(self.A, a) and in_set(self.A, b)):
            return False
        if b == 0:
            return a == 0
        r = Fr(a) / Fr(b)
        if r <= 0:
            return False
        e = 0
        while r.numerator % self.k == 0 and r != 1:
            r /= self.k
            e += 1
        while r.denominator % self.k == 0 and r != 1:
            r *= self.k
            e -= 1
        return r == 1 and self.exponent_ok(e)

    def truth(self, claim):
        name, _, data = claim
        E = [e for e in range(-30, 31) if self.exponent_ok(e)]
        if name == "pair":
            return self.related(data[0], data[1]) != data[2]
        if name == "refl":
            return self.exponent_ok(0)
        if name == "sym":
            return all(self.exponent_ok(-e) for e in E if abs(e) <= 10)
        return all(self.exponent_ok(e + f) for e in E for f in E if abs(e) <= 10 and abs(f) <= 10)

    def rule_tex(self):
        n = "2n" if self.step == 2 else "n"
        exponent = n if self.s == 0 else "%s %s %d" % (n, "+" if self.s > 0 else "-", abs(self.s))
        return r"\exists n \in %s\ (a = %d^{%s} b)" % (SETS[self.B], self.k, exponent)

    def claim_tex(self, claim):
        name, _, data = claim
        A = SETS[self.A]
        if name == "pair":
            num = lambda v: str(v.numerator) if v.denominator == 1 else r"\frac{%d}{%d}" % (v.numerator, v.denominator)
            return r"(%s, %s) %s R" % (num(data[0]), num(data[1]), r"\notin" if data[2] else r"\in")
        if name == "refl":
            return r"\forall x \in %s\ ((x, x) \in R)" % A
        if name == "sym":
            return r"\forall x, y\ ((x, y) \in R \implies (y, x) \in R)"
        return r"\forall x, y, z\ (((x, y) \in R \land (y, z) \in R) \implies (x, z) \in R)"

    def tex(self):
        A = SETS[self.A]
        pairs = "(%s)^{2}" % A if "^" in A else "%s^{2}" % A
        thesis = " \\land ".join((r"\lnot (%s)" if c[1] else "(%s)") % self.claim_tex(c) for c in self.claims)
        return r"R = \{(a, b) \in %s \mid %s\} \implies (%s)" % (pairs, self.rule_tex(), thesis)

    def falsified_by(self, claim, values):
        name = claim[0]
        R = self.related
        if name == "refl" and "x" in values:
            return in_set(self.A, values["x"]) and not R(values["x"], values["x"])
        if name == "sym" and {"x", "y"} <= set(values):
            return R(values["x"], values["y"]) and not R(values["y"], values["x"])
        if name == "trans" and {"x", "y", "z"} <= set(values):
            x, y, z = values["x"], values["y"], values["z"]
            return R(x, y) and R(y, z) and not R(x, z)
        return False

    def check(self, verdict, detail):
        values = dict((name, Fr(v)) for name, v in re.findall(r"(\w) = (-?\d+(?:/\d+)?)", detail or ""))
        if verdict == "R" and values and len(self.claims) == 1 and not self.claims[0][1] and self.claims[0][0] != "pair":
            if not self.falsified_by(self.claims[0], values):
                return [("BUG", "the counterexample %s does not falsify the claim" % detail)]
        whole = all(self.truth(c) != c[1] for c in self.claims)
        if verdict == "P" and not whole:
            return [("BUG", "proved, but a claim is false")]
        if verdict == "R" and whole:
            return [("BUG", "refuted, but every claim is true")]
        return []


# --- Coppie ----------------------------------------------------------------
class Poly4:
    """Un polinomio nelle quattro componenti, di grado al più due."""

    LETTERS = ("x_{1}", "y_{1}", "x_{2}", "y_{2}")

    def __init__(self):
        monos = [(0, 0, 0, 0)] + [tuple(int(i == j) for j in range(4)) for i in range(4)]
        monos += [tuple(int(j in (i, k)) + int(i == k == j) for j in range(4)) for i in range(4) for k in range(i, 4)]
        self.terms = {}
        for _ in range(rnd.choice([1, 2, 2, 3])):
            m = rnd.choice(monos[:5] if rnd.random() < 0.6 else monos)
            self.terms[m] = self.terms.get(m, 0) + rnd.choice([-2, -1, 1, 1, 2])
        self.terms = {m: c for m, c in self.terms.items() if c}

    def value(self, v):
        total = Fr(0)
        for m, c in self.terms.items():
            term = Fr(c)
            for e, x in zip(m, v):
                term *= Fr(x) ** e
            total += term
        return total

    def tex(self):
        if not self.terms:
            return "0"
        parts = []
        for m, c in sorted(self.terms.items(), reverse=True):
            letters = " ".join(l if e == 1 else "%s^{2}" % l for l, e in zip(self.LETTERS, m) if e)
            size = abs(c)
            parts.append(("-" if c < 0 else "+", (str(size) + (" " if letters else "") if (size != 1 or not letters) else "") + letters))
        text = ("-" if parts[0][0] == "-" else "") + parts[0][1]
        for sign, body in parts[1:]:
            text += " %s %s" % (sign, body)
        return text


class Pairs:
    def __init__(self):
        self.S = rnd.choice(["Z", "Z", "N"])
        self.p, self.q = Poly4(), Poly4()
        self.claim = rnd.choice(["refl", "sym", "trans", "total", "pair", "pair"])
        self.negated = rnd.random() < 0.3
        self.data = tuple(rnd.randint(-2, 3) for _ in range(4)), rnd.random() < 0.3

    def related(self, u, v):
        if not all(in_set(self.S, t) for t in u + v):
            return False
        return self.p.value(u + v) == self.q.value(u + v)

    def window(self):
        values = [t for t in range(-3, 6) if in_set(self.S, t)]
        return list(itertools.product(values, repeat=2))

    def truth(self):
        """(vero?, certo?) della proprietà, non negata."""
        R, w = self.related, self.window()
        if self.claim == "pair":
            u, v = self.data[0][:2], self.data[0][2:]
            return R(u, v) != self.data[1], True
        if self.claim == "refl":
            ok = all(R(u, u) for u in w)
        elif self.claim == "sym":
            ok = all(R(v, u) for u in w for v in w if R(u, v))
        elif self.claim == "trans":
            ok = all(R(u, z) for u in w for v in w if R(u, v) for z in w if R(v, z))
        else:
            ok = all(R(u, v) or R(v, u) for u in w for v in w)
        return ok, not ok

    def falsified_by(self, values):
        R = self.related
        get = lambda name: values.get(name)
        x, y, z = get("x"), get("y"), get("z")
        if self.claim == "refl" and x:
            return not R(x, x)
        if self.claim == "sym" and x and y:
            return R(x, y) and not R(y, x)
        if self.claim == "trans" and x and y and z:
            return R(x, y) and R(y, z) and not R(x, z)
        if self.claim == "total" and x and y:
            return not R(x, y) and not R(y, x)
        return False

    def tex(self):
        S = SETS[self.S]
        if self.claim == "pair":
            u = self.data[0]
            body = r"((%d, %d), (%d, %d)) %s R" % (u + (r"\notin" if self.data[1] else r"\in",))
        elif self.claim == "refl":
            body = r"\forall x \in %s^{2}\ ((x, x) \in R)" % S
        elif self.claim == "sym":
            body = r"\forall x, y\ ((x, y) \in R \implies (y, x) \in R)"
        elif self.claim == "trans":
            body = r"\forall x, y, z\ (((x, y) \in R \land (y, z) \in R) \implies (x, z) \in R)"
        else:
            body = r"\forall x, y \in %s^{2}\ ((x, y) \in R \lor (y, x) \in R)" % S
        if self.negated:
            body = r"\lnot (%s)" % body
        return r"R = \{((x_{1}, y_{1}), (x_{2}, y_{2})) \in (%s^{2})^{2} \mid %s = %s\} \implies (%s)" % (S, self.p.tex(), self.q.tex(), body)

    def check(self, verdict, detail):
        value, certain = self.truth()
        pairs = dict((name, (int(a), int(b))) for name, a, b in re.findall(r"(\w) = \((-?\d+), (-?\d+)\)", detail or ""))
        if verdict == "R" and not self.negated and self.claim != "pair" and pairs:
            if self.falsified_by(pairs):
                return []
            return [("BUG", "the counterexample %s does not falsify the claim" % detail)]
        # Di un "per ogni" è certo solo il controesempio; la certezza passa
        # com'è alla negazione.
        value = value != self.negated
        if verdict == "P" and not value:
            return [("BUG" if certain else "SUSPECT", "proved, but the statement is %s" % ("false" if certain else "not confirmed on the window"))]
        if verdict == "R" and value:
            return [("BUG" if certain else "SUSPECT", "refuted, but the statement is %s" % ("true" if certain else "true on the window"))]
        return []


# --- Reali -----------------------------------------------------------------
class Reals:
    """Un reale positivo q 2^u 3^v \\pi^w è dato dagli esponenti (u, v, w)."""

    def __init__(self):
        self.i = rnd.choice([-2, -1, -1, 1, 1, 2, 3])
        self.j = rnd.choice([-2, -1, -1, 1, 1, 2, 3])
        self.claim = rnd.choice(["refl", "sym", "trans"])
        self.negated = rnd.random() < 0.2
        grid = [Fr(0), Fr(1, 2), Fr(1, 3), Fr(1, 4), Fr(1, 5), Fr(1, 6), Fr(2, 3)]
        self.points = [(u, v, w) for u in grid for v in (Fr(0), Fr(1, 2), Fr(1, 3)) for w in (0, 1)]

    def rational(self, a, b):
        i, j = self.i, self.j
        return (i * a[0] + j * b[0]).denominator == 1 and (i * a[1] + j * b[1]).denominator == 1 and i * a[2] + j * b[2] == 0

    def truth(self):
        R, P = self.rational, self.points
        if self.claim == "refl":
            return all(R(a, a) for a in P)
        if self.claim == "sym":
            return all(R(b, a) for a in P for b in P if R(a, b))
        return all(R(a, c) for a in P for b in P if R(a, b) for c in P if R(b, c))

    def tex(self):
        power = lambda v, e: v if e == 1 else "%s^{%d}" % (v, e)
        rule = r"%s %s \in \mathbb{Q}" % (power("a", self.i), power("b", self.j))
        body = {"refl": r"\forall x \in \mathbb{R}^{+}\ ((x, x) \in R)",
                "sym": r"\forall x, y\ ((x, y) \in R \implies (y, x) \in R)",
                "trans": r"\forall x, y, z\ (((x, y) \in R \land (y, z) \in R) \implies (x, z) \in R)"}[self.claim]
        if self.negated:
            body = r"\lnot (%s)" % body
        return r"R = \{(a, b) \in (\mathbb{R}^{+})^{2} \mid %s\} \implies (%s)" % (rule, body)

    def check(self, verdict, detail):
        # Una proprietà smentita da un punto del gruppo è falsa di sicuro;
        # confermata su tutti i punti, potrebbe ancora cadere su un
        # irrazionale che il gruppo non contiene.
        truth = self.truth()
        value, certain = truth != self.negated, not truth
        if verdict == "R" and not self.negated:
            # Sui razionali a^{i} b^{j} è sempre razionale: un controesempio
            # razionale non esiste.
            return [("BUG", "refuted with a rational counterexample, where the rule always holds")]
        if verdict == "P" and not value:
            return [("BUG" if certain else "SUSPECT", "proved, but the statement is %s" % ("false" if certain else "not confirmed on the test points"))]
        if verdict == "R" and value:
            return [("BUG" if certain else "SUSPECT", "refuted, but the statement is %s" % ("true" if certain else "true on the test points"))]
        return []


class Roots:
    """Per ogni x reale (o reale positivo), x è razionale oppure x^2 + b x + c
    non è zero: vero esattamente quando il polinomio non ha radici reali
    irrazionali (positive, sui reali positivi), cioè quando il discriminante
    d è negativo o un quadrato, oppure, sui positivi, quando la radice più
    grande, (-b + sqrt(d)) / 2, non è positiva: quando b >= 0 e c >= 0."""

    def __init__(self):
        self.b, self.c = rnd.randint(-6, 6), rnd.randint(-9, 9)
        self.positive = rnd.random() < 0.5

    def truth(self):
        d = self.b * self.b - 4 * self.c
        if d < 0 or round(d ** 0.5) ** 2 == d:
            return True
        return self.positive and self.b >= 0 and self.c >= 0

    def tex(self):
        linear = "" if self.b == 0 else " %s %sx" % ("+" if self.b > 0 else "-", "" if abs(self.b) == 1 else abs(self.b))
        constant = "" if self.c == 0 else " %s %d" % ("+" if self.c > 0 else "-", abs(self.c))
        S = r"\mathbb{R}^{+}" if self.positive else r"\mathbb{R}"
        return r"\forall x \in %s\ (x \in \mathbb{Q} \lor x^{2}%s%s \neq 0)" % (S, linear, constant)

    def check(self, verdict, detail):
        if verdict == "P" and not self.truth():
            return [("BUG", "proved, but an irrational root exists")]
        if verdict == "R":
            return [("BUG", "refuted, but no rational number is a counterexample")]
        return []


class Multiples:
    """R = {(x, y) in S^2 | esiste a in T con y = c a x}: la divisibilità e le sue
    varianti, con le proprietà di un ordine e il massimo o il minimo. L'oracolo
    calcola l'appartenenza di una coppia dividendo, e prova le proprietà su una
    finestra: un controesempio è certo, la sua assenza no. Il massimo m, se
    c'è, deve stare in relazione con ogni x della finestra; con S = N_0 lo è 0,
    che è multiplo di tutto."""

    def __init__(self):
        self.S = rnd.choice(["N", "N", "N0", "Z"])
        self.T = rnd.choice(["Z", "Z", "N", "N0"])
        self.c = rnd.choice([1, 1, 2, -1])
        self.claim = rnd.choice(["refl", "antisym", "trans", "total", "min", "nomax", "nomin"])
        self.negated = rnd.random() < 0.3

    def related(self, x, y):
        if not (in_set(self.S, x) and in_set(self.S, y)):
            return False
        if x == 0:
            return y == 0 and any(in_set(self.T, a) for a in range(-1, 2))
        a = Fr(y, self.c * x)
        return in_set(self.T, a)

    def truth(self):
        """(vero?, certo?) della proprietà, non negata."""
        R = self.related
        w = [t for t in range(-6, 13) if in_set(self.S, t)]
        if self.claim == "refl":
            ok = all(R(x, x) for x in w)
        elif self.claim == "antisym":
            ok = all(x == y for x in w for y in w if R(x, y) and R(y, x))
        elif self.claim == "trans":
            ok = all(R(x, z) for x in w for y in w if R(x, y) for z in w if R(y, z))
        elif self.claim == "total":
            ok = all(R(x, y) or R(y, x) for x in w for y in w)
        elif self.claim == "min":
            return self.one_is_bottom(), True
        elif self.claim == "nomax":
            # Nessun m con ogni x in relazione con m: se uno della finestra lo
            # è con tutti, la proprietà non vale di sicuro solo se lo è davvero
            # con tutti, e lo si sa per m = 0 con 0 multiplo di tutto.
            tops = [m for m in w if all(R(x, m) for x in w)]
            if 0 in tops and self.T in ("Z", "N0"):
                return False, True
            return not tops, False
        else:
            if self.one_is_bottom():
                return False, True
            return not any(all(R(m, y) for y in w) for m in w), False
        return ok, not ok

    def one_is_bottom(self):
        """Esatto: (1, y) sta in R per ogni y di S, cioè y / c sta in T per ogni y
        di S. Con c = 1 vuol dire che S sta in T; con c = -1 che -S sta in T, e
        tra questi insiemi succede solo con T = Z, perché -1 non sta né in N né
        in N_0; con c = 2 mai, per y = 1."""
        inside = {("N", "Z"), ("N", "N"), ("N", "N0"), ("N0", "Z"), ("N0", "N0"), ("Z", "Z")}
        if not in_set(self.S, 1):
            return False
        if self.c == 1:
            return (self.S, self.T) in inside
        return self.c == -1 and self.T == "Z"

    def tex(self):
        S, T = SETS[self.S], SETS[self.T]
        coefficient = {1: "", 2: "2", -1: "-"}[self.c]
        rule = r"\exists a \in %s\ (y = %sax)" % (T, coefficient)
        body = {"refl": r"\forall x \in %s\ ((x, x) \in R)" % S,
                "antisym": r"\forall x, y\ (((x, y) \in R \land (y, x) \in R) \implies x = y)",
                "trans": r"\forall x, y, z\ (((x, y) \in R \land (y, z) \in R) \implies (x, z) \in R)",
                "total": r"\forall x, y \in %s\ ((x, y) \in R \lor (y, x) \in R)" % S,
                "min": r"\forall y \in %s\ ((1, y) \in R)" % S,
                "nomax": r"\lnot (\exists m \in %s\ \forall x \in %s\ ((x, m) \in R))" % (S, S),
                "nomin": r"\lnot (\exists m \in %s\ \forall y \in %s\ ((m, y) \in R))" % (S, S)}[self.claim]
        if self.negated:
            body = r"\lnot (%s)" % body
        pairs = "%s^{2}" % S if "_" not in S else "(%s)^{2}" % S
        return r"R = \{(x, y) \in %s \mid %s\} \implies (%s)" % (pairs, rule, body)

    def check(self, verdict, detail):
        value, certain = self.truth()
        value = value != self.negated
        if verdict == "P" and not value:
            return [("BUG" if certain else "SUSPECT", "proved, but the statement is %s" % ("false" if certain else "not confirmed on the window"))]
        if verdict == "R" and value:
            return [("BUG" if certain else "SUSPECT", "refuted, but the statement is %s" % ("true" if certain else "true on the window"))]
        return []


def main():
    families = [Divisors, Divisors, Powers, Powers, Pairs, Pairs, Reals, Roots, Multiples, Multiples]
    cases = [rnd.choice(families)() for _ in range(COUNT)]
    texts = [case.tex() for case in cases]
    path = os.path.join(WORK, "fuzz_number_relations_%d.tex" % SEED)
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
        detail = answer.split("controesempio ", 1)[1] if "controesempio " in answer else None
        key = (type(case).__name__, verdict)
        tally[key] = tally.get(key, 0) + 1
        for severity, message in case.check(verdict, detail):
            print("%s %d [%s]: %s\n      %s" % (severity, number, answer[:60], message, text))
    print("seed %d:" % SEED, ", ".join("%s %s=%d" % (k[0], k[1], v) for k, v in sorted(tally.items())))


main()
