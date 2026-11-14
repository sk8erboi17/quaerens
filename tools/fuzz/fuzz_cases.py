"""Fuzzing differenziale delle applicazioni definite per casi (17s, 18v).

Uso: python3 -I fuzz_cases.py <eseguibile> <seme> <quanti> <cartella di lavoro> [tutti]

Con "tutti" stampa anche, come NOTE, gli enunciati ben definiti che il
programma non ha deciso: non sono errori, ma dicono dove le regole non
arrivano.

Genera applicazioni tra insiemi numerici definite per casi, applicazioni a
valori in coppie, f(x) = (P(x), Q(x)), formule chiuse per le iterate f^{n}
di f(x) = a x + b (17s, 26c), applicazioni a n + b con i parametri a, b e
lettere legate che li nascondono, e applicazioni lineari di \\mathbb{C} in
\\mathbb{C} (17s, 18v-8, 18v-11). Le prime sono definite sul resto
(2 | n, 3 \\nmid n), su un lato di un numero (n \\ge c, x < c), su un punto
(x = c, x \\neq c), sull'essere un intero (x \\in Z), oppure con un valore
assoluto (f(x) = a x + b |x - c| + d), con una tesi su iniettività e
suriettività. Una parte dei casi è sbagliata apposta: casi che si
sovrappongono con valori diversi, che lasciano fuori dei punti, o un valore
che cade fuori dal codominio. Li fa decidere al programma e confronta ogni
verdetto con un oracolo scritto qui, indipendente dal programma:

  - f calcolata con le frazioni di Python, caso per caso, e la definizione
    controllata su una finestra: un caso vale, i casi che valgono danno lo
    stesso numero, e il numero sta nel codominio;
  - collisioni f(a) = f(b) cercate sulla finestra;
  - controimmagini calcolate in modo esatto: per ogni caso le radici
    razionali di "valore del caso = y", tenute se soddisfano la condizione
    del caso e stanno nel dominio; per una coppia, le radici comuni delle
    due componenti;
  - le iterate applicando f n volte, e confrontandole con la formula per i
    primi n e su una finestra di x;
  - per a n + b: iniettiva se e solo se a non è 0, suriettiva se e solo se a
    è 1 o -1, confrontate con la condizione della tesi per a da -6 a 6;
  - per le applicazioni lineari di C: non iniettiva e non suriettiva
    esattamente quando il determinante è zero;
  - un controesempio si rilegge e si ricontrolla: anche irrazionale, come
    b = 1 + 3\\sqrt{3}, con i conti esatti in Q(sqrt(r)) (Surd).

Stampa BUG quando il verdetto contraddice un fatto certo, SUSPECT quando
contraddice solo ciò che si vede sulla finestra, o quando un controesempio
non si sa rileggere.
"""
import math
import os
import random
import re
import subprocess
import sys
from fractions import Fraction as Fr

MR, SEED, COUNT, WORK = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
VERBOSE = len(sys.argv) > 5 and sys.argv[5] == "tutti"
rnd = random.Random(SEED)


def ftex(v):
    v = Fr(v)
    if v.denominator == 1:
        return str(v.numerator)
    return ("-" if v < 0 else "") + r"\frac{%d}{%d}" % (abs(v.numerator), v.denominator)


class NS:
    """Un insieme numerico: N, N0, Z, Q, R, R+. Dei reali si guardano i
    razionali, e i punti irrazionali dove servono si trattano a parte
    (RealRoot)."""
    BASE = {"N": r"\mathbb{N}", "N0": r"\mathbb{N}_{0}", "Z": r"\mathbb{Z}", "Q": r"\mathbb{Q}",
            "R": r"\mathbb{R}", "R+": r"\mathbb{R}^{+}"}

    def __init__(self, kind):
        self.kind = kind

    def integers(self):
        return self.kind in ("N", "N0", "Z")

    def reals(self):
        return self.kind in ("R", "R+")

    def has(self, v):
        if isinstance(v, RealRoot):
            return self.reals() and (self.kind == "R" or v.compare(0) > 0)
        if isinstance(v, Surd):
            return self.reals() and (self.kind == "R" or v > 0)
        v = Fr(v)
        if self.integers() and v.denominator != 1:
            return False
        return not ((self.kind == "N" and v < 1) or (self.kind == "N0" and v < 0) or (self.kind == "R+" and v <= 0))

    def tex(self):
        return self.BASE[self.kind]

    def window(self, size):
        if self.integers():
            return [Fr(i) for i in range(-size, size + 1) if self.has(i)]
        # Anche qui solo gli elementi dell'insieme: senza il filtro la finestra
        # di R+ conteneva i negativi, e un valore come -10, che non sta nel
        # codominio, risultava "mancato" da un'applicazione suriettiva su R+.
        return sorted({Fr(p, q) for q in range(1, 7) for p in range(-size * q // 3, size * q // 3 + 1)
                       if self.has(Fr(p, q))})


class RealRoot:
    """Una radice irrazionale (-b + s sqrt(D)) / (2a) di a x^2 + b x + c, con D
    il discriminante, positivo e non un quadrato: un numero reale esatto, di
    cui si sa dire il segno della differenza con un razionale."""

    def __init__(self, a, b, d, sign):
        self.a, self.b, self.d, self.sign = Fr(a), Fr(b), Fr(d), sign

    def compare(self, t):
        """Il segno di questa radice meno t: quello di (u + s sqrt(D)) / (2a),
        con u = -b - 2 a t."""
        u = -self.b - 2 * self.a * Fr(t)
        if self.sign > 0:
            top = 1 if u >= 0 else (1 if self.d > u * u else -1)
        else:
            top = -1 if u <= 0 else (1 if u * u > self.d else -1)
        return top if self.a > 0 else -top


class Surd:
    """Un numero p + q sqrt(r), con p e q razionali e r un intero positivo
    senza fattori quadrati: serve a rileggere un controesempio irrazionale,
    come b = 1 + 3\\sqrt{3}, che la tabella dei segni trova tra le radici
    (18t). Somme, prodotti e confronti sono esatti."""

    def __init__(self, p, q=0, r=1):
        self.p, self.q, self.r = Fr(p), Fr(q), r if q else 1

    @staticmethod
    def of(v):
        return v if isinstance(v, Surd) else Surd(v)

    def same_field(self, other):
        if self.q and other.q and self.r != other.r:
            raise ValueError("two different square roots")
        return self.r if self.q else other.r

    def __add__(self, other):
        o = Surd.of(other)
        return Surd(self.p + o.p, self.q + o.q, self.same_field(o))

    __radd__ = __add__

    def __neg__(self):
        return Surd(-self.p, -self.q, self.r)

    def __sub__(self, other):
        return self + (-Surd.of(other))

    def __rsub__(self, other):
        return Surd.of(other) - self

    def __mul__(self, other):
        o = Surd.of(other)
        r = self.same_field(o)
        return Surd(self.p * o.p + self.q * o.q * r, self.p * o.q + self.q * o.p, r)

    __rmul__ = __mul__

    def sign(self):
        """Il segno esatto: p e q sqrt(r) con lo stesso segno, o il confronto
        tra p^2 e q^2 r."""
        sp, sq = (self.p > 0) - (self.p < 0), (self.q > 0) - (self.q < 0)
        if sq == 0 or sp == sq:
            return sp or sq
        if sp == 0:
            return sq
        big = self.p * self.p - self.q * self.q * self.r
        return sp if big > 0 else (sq if big < 0 else 0)

    def __eq__(self, other):
        return isinstance(other, (int, Fr, Surd)) and (self - other).sign() == 0

    def __ne__(self, other):
        return not self == other

    def __lt__(self, other):
        return (self - other).sign() < 0

    def __le__(self, other):
        return (self - other).sign() <= 0

    def __gt__(self, other):
        return (self - other).sign() > 0

    def __ge__(self, other):
        return (self - other).sign() >= 0

    def __hash__(self):
        return hash((self.p, self.q, self.r))

    def __repr__(self):
        return "%s + %s sqrt(%d)" % (self.p, self.q, self.r)


def read_value(text):
    """Il numero scritto in text dal programma, in un controesempio: un
    razionale (Fr), oppure p + q\\sqrt{r} (Surd), anche dentro \\frac{...}{d}.
    None se la scrittura è un'altra: chi chiama allora non sa ricontrollare."""
    text = text.replace(" ", "")
    m = re.fullmatch(r"(-?)\\frac\{(.+)\}\{(\d+)\}", text)
    if m and "frac" not in m.group(2):
        inner = read_value(m.group(2))
        if inner is None:
            return None
        value = Surd.of(inner) * Fr(1, int(m.group(3)))
        return -value if m.group(1) else value
    terms = list(re.finditer(r"([+-]?)(\d*(?:/\d+)?)(\\sqrt\{(\d+)\})?", text))
    if "".join(t.group(0) for t in terms) != text:
        return None
    value, used = Surd(0), False
    for t in terms:
        if not t.group(0):
            continue
        if not t.group(2) and not t.group(3):
            return None
        size = Fr(t.group(2)) if t.group(2) else Fr(1)
        if t.group(1) == "-":
            size = -size
        value = value + (Surd(0, size, int(t.group(4))) if t.group(3) else Surd(size))
        used = True
    if not used:
        return None
    return value.p if value.q == 0 else value


def read_choices(detail):
    """Le scelte di un controesempio, "a = -4, b = 1 + 3\\sqrt{3}", lettera
    per lettera, con i valori letti da read_value()."""
    out = {}
    # Il controesempio finisce al punto e virgola del verdetto, e prima della
    # nota tra parentesi su come si sono lette le coppie, (z = c + id).
    detail = re.sub(r"\s+\(\w = .*$", "", (detail or "").split(";")[0])
    for part in re.split(r",\s*(?=\w = )", detail):
        m = re.match(r"\s*(\w) = (.+?)\s*$", part)
        if m:
            out[m.group(1)] = read_value(m.group(2))
    return out


def is_square(v):
    v = Fr(v)
    if v < 0:
        return False
    n, d = math.isqrt(v.numerator), math.isqrt(v.denominator)
    return n * n == v.numerator and d * d == v.denominator


def real_roots(c):
    """Le radici reali di un polinomio di grado al più 2: razionali, o RealRoot;
    None se è il polinomio nullo."""
    c = [Fr(k) for k in c]
    while c and c[-1] == 0:
        c.pop()
    if not c:
        return None
    if len(c) <= 2:
        return [] if len(c) == 1 else [-c[0] / c[1]]
    a, b, k = c[2], c[1], c[0]
    d = b * b - 4 * a * k
    if d < 0:
        return []
    if is_square(d):
        r = Fr(math.isqrt(d.numerator), math.isqrt(d.denominator))
        return sorted({(-b + r) / (2 * a), (-b - r) / (2 * a)})
    return [RealRoot(a, b, d, 1), RealRoot(a, b, d, -1)]


# --- Polinomi in una variabile, come liste di coefficienti razionali -----------
def ptex(c, x):
    terms = []
    for k in range(len(c) - 1, -1, -1):
        if c[k] == 0:
            continue
        size = abs(Fr(c[k]))
        mono = "" if k == 0 else (x if k == 1 else "%s^{%d}" % (x, k))
        if size.denominator != 1:
            body = r"\frac{%d}{%d}%s" % (size.numerator, size.denominator, mono)
        else:
            body = (str(size.numerator) if (size != 1 or k == 0) else "") + mono
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
    """Radici razionali; None se il polinomio è nullo."""
    c = [Fr(k) for k in c]
    while c and c[-1] == 0:
        c.pop()
    if not c:
        return None
    scale = 1
    for k in c:
        scale = scale * k.denominator // math.gcd(scale, k.denominator)
    c = [int(k * scale) for k in c]
    roots = set()
    while len(c) > 1 and c[0] == 0:
        roots.add(Fr(0))
        c = c[1:]
    if len(c) == 1:
        return roots

    def divisors(n):
        n = abs(n)
        return {d for d in range(1, int(n ** 0.5) + 1) if n % d == 0} | {n // d for d in range(1, int(n ** 0.5) + 1) if n % d == 0}

    for p in divisors(c[0]):
        for q in divisors(c[-1]):
            for sign in (1, -1):
                if peval(c, Fr(sign * p, q)) == 0:
                    roots.add(Fr(sign * p, q))
    return roots


# --- Condizioni dei casi --------------------------------------------------------
class Cond:
    """Una condizione sulla variabile: ('div', m), ('ndiv', m), ('ge', c), ('le', c),
    ('lt', c), ('gt', c), ('eq', c), ('ne', c), ('int',), ('nint',)."""

    def __init__(self, kind, value=None):
        self.kind, self.value = kind, value

    def holds(self, x):
        k, c = self.kind, self.value
        if isinstance(x, RealRoot):
            # Un irrazionale: solo confronti, e mai uguale a un razionale.
            if k in ("eq", "ne"):
                return k == "ne"
            side = x.compare(c)
            return {"ge": side >= 0, "le": side <= 0, "lt": side < 0, "gt": side > 0}[k]
        if isinstance(x, Surd):
            # Un irrazionale p + q sqrt(r): non è un intero, né un multiplo.
            if k in ("div", "ndiv", "int", "nint"):
                return k in ("ndiv", "nint")
            return {"ge": x >= c, "le": x <= c, "lt": x < c, "gt": x > c, "eq": x == c, "ne": x != c}[k]
        if k in ("div", "ndiv"):
            divides = (Fr(x) / c).denominator == 1
            return divides == (k == "div")
        if k in ("int", "nint"):
            return (Fr(x).denominator == 1) == (k == "int")
        return {"ge": x >= c, "le": x <= c, "lt": x < c, "gt": x > c, "eq": x == c, "ne": x != c}[k]

    def tex(self, x):
        k, c = self.kind, self.value
        if k == "div":
            return r"%d \mid %s" % (c, x)
        if k == "ndiv":
            return rnd.choice([r"%d \nmid %s" % (c, x), r"\lnot (%d \mid %s)" % (c, x)])
        if k == "int":
            return r"%s \in \mathbb{Z}" % x
        if k == "nint":
            return r"%s \notin \mathbb{Z}" % x
        op = {"ge": r"\ge", "le": r"\le", "lt": "<", "gt": ">", "eq": "=", "ne": r"\neq"}[k]
        return "%s %s %s" % (x, op, ftex(c))


def condition_pair(domain, broken):
    """Due condizioni che dividono il dominio; con broken, spesso sbagliate apposta."""
    integers = domain.integers()
    families = ["order", "order", "point"] if domain.reals() else ["div", "order", "point"] + ([] if integers else ["int", "int"])
    family = rnd.choice(families)
    c = rnd.choice([-2, -1, 0, 1, 2, 3])
    if family == "div":
        m = rnd.choice([2, 2, 3])
        pair = [Cond("div", m), Cond("ndiv", m)]
    elif family == "int":
        pair = [Cond("int"), Cond("nint")]
    elif family == "point":
        pair = [Cond("ne", Fr(c)), Cond("eq", Fr(c))]
    elif integers:
        pair = [Cond("ge", Fr(c)), Cond("le", Fr(c - 1))]
    else:
        pair = rnd.choice([[Cond("ge", Fr(c)), Cond("lt", Fr(c))], [Cond("gt", Fr(c)), Cond("le", Fr(c))]])
    if broken and family == "order":
        # Un lato spostato di uno: i casi si sovrappongono, o lasciano un buco.
        second = pair[1]
        pair[1] = Cond(second.kind, second.value + rnd.choice([-1, 1]))
    if rnd.random() < 0.5:
        pair.reverse()
    return pair


class CaseMap:
    """f: A -> B definita per casi, o con un valore assoluto, e una tesi."""

    def __init__(self):
        self.A = NS(rnd.choice(["N", "N0", "Z", "Z", "Q", "Q", "R", "R"]))
        self.B = NS(rnd.choice(["R", "R", "R+"]) if self.A.reals() else rnd.choice(["N", "N0", "Z", "Z", "Q"]))
        self.x = rnd.choice("nxm")
        broken = rnd.random() < 0.25
        small = lambda: rnd.choice([-3, -2, -1, -1, 0, 1, 1, 1, 2, 3])
        self.absolute = rnd.random() < 0.25
        if self.absolute:
            # f(x) = a x + b |x - c| + d, letta come due casi.
            a, b, c, d = small(), rnd.choice([-2, -1, 1, 1, 2, 3]), rnd.choice([-1, 0, 0, 1, 2]), small()
            self.abs_data = (a, b, c, d)
            self.cases = [(Cond("ge", Fr(c)), [d - b * c, a + b]), (Cond("lt", Fr(c)), [d + b * c, a - b])]
        else:
            pair = condition_pair(self.A, broken)
            self.cases = []
            for cond in pair:
                if rnd.random() < 0.8:
                    poly = [small(), rnd.choice([-3, -2, -1, 1, 1, 2, 3])]
                else:
                    poly = [small(), small(), rnd.choice([-1, 1])]
                if rnd.random() < 0.2 and cond.kind == "div":
                    poly = [Fr(k, cond.value) for k in poly]
                self.cases.append((cond, poly))
        # Spesso un codominio in cui la definizione regge.
        if rnd.random() < 0.7:
            for _ in range(30):
                if self.ill_defined() is None:
                    break
                self.B = NS(rnd.choice(["R", "R+"]) if self.A.reals() else rnd.choice(["N", "N0", "Z", "Q"]))
        self.claim = rnd.choice(["inj", "ninj", "surj", "nsurj", "inj&surj", "inj&nsurj", "ninj&surj", "ninj&nsurj"])

    # --- Il valore -------------------------------------------------------------
    def values(self, x):
        return [peval(poly, x) for cond, poly in self.cases if cond.holds(x)]

    def f(self, x):
        v = self.values(x)
        return v[0] if v and all(w == v[0] for w in v) else None

    def ill_defined(self):
        for x in self.A.window(40):
            v = self.values(x)
            if not v or any(w != v[0] for w in v) or not self.B.has(v[0]):
                return x
        return None

    # --- Il testo --------------------------------------------------------------
    def tex(self):
        A, B, x = self.A.tex(), self.B.tex(), self.x
        if self.absolute:
            a, b, c, d = self.abs_data
            inner = x if c == 0 else "%s %s %d" % (x, "-" if c > 0 else "+", abs(c))
            rule = r"\forall %s \in %s\ (f(%s) = %s)" % (x, A, x, ptex([d, a], x) + " %s %s|%s|" % (
                "+" if b > 0 else "-", "" if abs(b) == 1 else str(abs(b)), inner))
            rule = rule.replace("= 0 + ", "= ").replace("= 0 - ", "= -")
        else:
            parts = [r"(%s \implies f(%s) = %s)" % (cond.tex(x), x, ptex(poly, x)) for cond, poly in self.cases]
            rule = r"\forall %s \in %s\ (%s)" % (x, A, r" \land ".join(parts))
        claims = {
            "inj": r"\forall a, b \in %s\ (f(a) = f(b) \implies a = b)" % A,
            "surj": r"\forall y \in %s\ \exists z \in %s\ (f(z) = y)" % (B, A),
        }
        claims["ninj"] = r"\lnot (%s)" % claims["inj"]
        claims["nsurj"] = r"\lnot (%s)" % claims["surj"]
        thesis = " \\land ".join("(%s)" % claims[p] for p in self.claim.split("&"))
        return r"(f \colon %s \to %s \land %s) \implies (%s)" % (A, B, rule, thesis)

    # --- Oracolo ---------------------------------------------------------------
    def collision(self):
        """Due elementi con la stessa immagine: per ogni x della finestra, le
        controimmagini esatte di f(x) diverse da x."""
        for x in self.A.window(30 if self.A.integers() or self.A.reals() else 9):
            others = [r for r in self.preimages(self.f(x)) if r != x]
            if others:
                return x, others[0]
        return None

    def preimages(self, y):
        """Le controimmagini di y, esatte: per ogni caso le radici del suo valore
        meno y. Un caso il cui valore è la costante y le ha tutte: quelle della
        finestra in cui vale la condizione (le costanti delle condizioni sono
        piccole, e la finestra le contiene)."""
        found = []
        for cond, poly in self.cases:
            shifted = [poly[0] - y] + list(poly[1:])
            roots = real_roots(shifted) if self.A.reals() else rational_roots(shifted)
            if roots is None:
                roots = [x for x in self.A.window(40) if cond.holds(x)]
            for r in roots:
                if not (self.A.has(r) and cond.holds(r)):
                    continue
                # Una radice irrazionale sta in un caso solo dei due di una
                # coppia complementare, quindi lì f vale y; una razionale si
                # ricontrolla calcolando.
                if isinstance(r, RealRoot) or self.f(r) == y:
                    found.append(r)
        return found

    def missed(self):
        for y in self.B.window(30 if self.B.integers() or self.B.reals() else 9):
            if not self.preimages(y):
                return y
        return None

    def check(self, verdict, detail):
        out = []
        decided = verdict in ("P", "R")
        bad = self.ill_defined()
        if bad is not None:
            if decided:
                out.append(("BUG", "the cases do not define a map at %s = %s, but the verdict is %s" % (self.x, bad, verdict)))
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
        values = read_choices(detail)
        if any(values.get(k, 0) is None for k in "aby"):
            return [("SUSPECT", "a value of the counterexample cannot be read back: %s" % detail)]
        if "a" in values and "b" in values and "inj" in parts:
            a, b = values["a"], values["b"]
            if not (self.A.has(a) and self.A.has(b) and a != b and self.f(a) == self.f(b)):
                out.append(("BUG", "refuted injectivity with a=%s, b=%s, which is not a collision" % (a, b)))
        elif "y" in values and "surj" in parts:
            y = values["y"]
            if isinstance(y, Surd):
                return [("SUSPECT", "an irrational missed value, y = %s, is not checked" % detail)]
            if not self.B.has(y) or self.preimages(y):
                out.append(("BUG", "refuted surjectivity with y=%s, which has a preimage or is outside B" % y))
        else:
            # Senza numeri: la tesi è una congiunzione, ed è falsa se lo è una
            # parte. Il verdetto è sbagliato se ogni parte è vera: di sicuro
            # per "non iniettiva" con una collisione e "non suriettiva" con un
            # valore mancato, solo sulla finestra per le altre due.
            truth = {"ninj": collision is not None, "nsurj": missed is not None,
                     "inj": collision is None, "surj": missed is None}
            if all(truth[p] for p in parts):
                certain = all(p in ("ninj", "nsurj") for p in parts)
                out.append(("BUG" if certain else "SUSPECT",
                            "refuted, but every part holds (collision=%s, missed=%s)" % (collision, missed)))
        return out


class PairMap:
    """f: A -> B x C, f(x) = (P(x), Q(x)), con una tesi su iniettività e suriettività."""

    def __init__(self):
        self.A = NS(rnd.choice(["N", "N0", "Z", "Z", "Q"]))
        self.B = [NS(rnd.choice(["N", "N0", "Z", "Z", "Q"])) for _ in range(2)]
        self.x = rnd.choice("nxm")
        small = lambda: rnd.choice([-3, -2, -1, 0, 0, 1, 1, 2, 3])
        self.polys = []
        for _ in range(2):
            degree = rnd.choice([0, 1, 1, 2, 2, 3])
            self.polys.append([small() for _ in range(degree)] + [rnd.choice([-2, -1, 1, 1, 2])])
        if rnd.random() < 0.7:
            for _ in range(30):
                if self.ill_defined() is None:
                    break
                self.B = [NS(rnd.choice(["N", "N0", "Z", "Q"])) for _ in range(2)]
        self.square = self.B[0].kind == self.B[1].kind and rnd.random() < 0.6
        self.claim = rnd.choice(["inj", "ninj", "surj", "nsurj", "inj&surj", "inj&nsurj", "ninj&surj", "ninj&nsurj"])

    def f(self, x):
        return tuple(peval(p, x) for p in self.polys)

    def ill_defined(self):
        for x in self.A.window(40):
            if not all(b.has(v) for b, v in zip(self.B, self.f(x))):
                return x
        return None

    def codomain_tex(self):
        if self.square:
            return self.B[0].tex() + "^{2}"
        return r"%s \times %s" % (self.B[0].tex(), self.B[1].tex())

    def tex(self):
        A, B, x = self.A.tex(), self.codomain_tex(), self.x
        rule = r"\forall %s \in %s\ (f(%s) = (%s, %s))" % (x, A, x, ptex(self.polys[0], x), ptex(self.polys[1], x))
        claims = {
            "inj": r"\forall a, b \in %s\ (f(a) = f(b) \implies a = b)" % A,
            "surj": r"\forall y \in %s\ \exists z \in %s\ (f(z) = y)" % (B, A),
        }
        claims["ninj"] = r"\lnot (%s)" % claims["inj"]
        claims["nsurj"] = r"\lnot (%s)" % claims["surj"]
        thesis = " \\land ".join("(%s)" % claims[p] for p in self.claim.split("&"))
        return r"(f \colon %s \to %s \land %s) \implies (%s)" % (A, B, rule, thesis)

    def preimages(self, y):
        """Le controimmagini esatte della coppia y: le radici comuni delle due
        componenti, cercate tra quelle della prima che non è costante."""
        roots = [None, None]
        for k in range(2):
            roots[k] = rational_roots([self.polys[k][0] - y[k]] + list(self.polys[k][1:]))
        if roots[0] is None and roots[1] is None:
            return [x for x in self.A.window(40)]
        candidates = roots[0] if roots[0] is not None else roots[1]
        return [r for r in candidates if self.A.has(r) and self.f(r) == tuple(y)]

    def collision(self):
        for x in self.A.window(30 if self.A.integers() else 9):
            others = [r for r in self.preimages(self.f(x)) if r != x]
            if others:
                return x, others[0]
        return None

    def missed(self):
        size = 12 if all(b.integers() for b in self.B) else 5
        for y0 in self.B[0].window(size):
            for y1 in self.B[1].window(size):
                if not self.preimages((y0, y1)):
                    return (y0, y1)
        return None

    def check(self, verdict, detail):
        out = []
        decided = verdict in ("P", "R")
        bad = self.ill_defined()
        if bad is not None:
            if decided:
                out.append(("BUG", "the rule is not a map at %s = %s, but the verdict is %s" % (self.x, bad, verdict)))
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
                    out.append(("BUG", "proved surjective but %s has no preimage" % (missed,)))
                if p == "ninj" and not collision:
                    out.append(("SUSPECT", "proved not injective but no collision in the window"))
                if p == "nsurj" and missed is None:
                    out.append(("SUSPECT", "proved not surjective but every pair of the window has a preimage"))
            return out
        values = dict((name, Fr(v)) for name, v in re.findall(r"(\w) = (-?\d+(?:/\d+)?)", detail or ""))
        # Una lettera che sta per una coppia compare nel controesempio con
        # la coppia, y = (1, -2).
        pair = re.search(r"\by = \((-?\d+(?:/\d+)?), (-?\d+(?:/\d+)?)\)", detail or "")
        if "a" in values and "b" in values and "inj" in parts:
            a, b = values["a"], values["b"]
            if not (self.A.has(a) and self.A.has(b) and a != b and self.f(a) == self.f(b)):
                out.append(("BUG", "refuted injectivity with a=%s, b=%s, which is not a collision" % (a, b)))
        elif pair and "surj" in parts:
            y = (Fr(pair.group(1)), Fr(pair.group(2)))
            if not (self.B[0].has(y[0]) and self.B[1].has(y[1])) or self.preimages(y):
                out.append(("BUG", "refuted surjectivity with y=%s, which has a preimage or is outside B" % (y,)))
        else:
            truth = {"ninj": collision is not None, "nsurj": missed is not None,
                     "inj": collision is None, "surj": missed is None}
            if all(truth[p] for p in parts):
                certain = all(p in ("ninj", "nsurj") for p in parts)
                out.append(("BUG" if certain else "SUSPECT",
                            "refuted, but every part holds (collision=%s, missed=%s)" % (collision, missed)))
        return out


class IterateMap:
    """f(x) = a x + b, con f^{1} = f e f^{n+1} = f \\circ f^{n}, e una formula
    chiusa per f^{n}(x): quella giusta o una sbagliata di poco."""

    def __init__(self):
        self.A = NS(rnd.choice(["Z", "Q"]))
        self.a = rnd.choice([-3, -2, -1, 1, 2, 3])
        self.b = rnd.choice([-3, -2, -1, 0, 1, 2, 3])
        self.wrong = rnd.choice([None, None, "b", "n3", "shift"])

    def value(self, n, x):
        """f^{n}(x), calcolata applicando f n volte."""
        for _ in range(n):
            x = self.a * x + self.b
        return x

    def claimed(self, n, x):
        a, b = Fr(self.a), Fr(self.b)
        bb = b + 1 if self.wrong == "b" else b
        m = n + 1 if self.wrong == "shift" else n
        if a == 1:
            v = x + m * bb
        else:
            v = a ** m * x + bb * (a ** m - 1) / (a - 1)
        if self.wrong == "n3":
            v += n * (n - 1) * (n - 2)
        return v

    def tex(self):
        a, b = self.a, self.b
        bb = b + 1 if self.wrong == "b" else b
        m = "n + 1" if self.wrong == "shift" else "n"
        power = ("(%d)" % a if a < 0 else str(a)) + "^{%s}" % m
        if a == 1:
            closed = "x %s %d(%s)" % ("-" if bb < 0 else "+", abs(bb), m)
        else:
            closed = r"%s x + \frac{%d(%s - 1)}{%d}" % (power, bb, power, a - 1)
        if self.wrong == "n3":
            closed += " + n(n - 1)(n - 2)"
        A = self.A.tex()
        return (r"(f \colon %s \to %s \land \forall x \in %s\ (f(x) = %s) \land f^{1} = f \land \forall n \in \mathbb{N}\ "
                r"(f^{n + 1} = f \circ f^{n})) \implies (\forall n \in \mathbb{N}\ \forall x \in %s\ (f^{n}(x) = %s))"
                % (A, A, A, ptex([b, a], "x"), A, closed))

    def check(self, verdict, detail):
        mismatch = None
        for n in range(1, 8):
            for x in self.A.window(4):
                if self.value(n, Fr(x)) != self.claimed(n, Fr(x)):
                    mismatch = (n, x)
                    break
            if mismatch:
                break
        if verdict == "P" and mismatch:
            return [("BUG", "proved, but f^%d(%s) differs from the formula" % mismatch)]
        if verdict == "R" and not mismatch:
            return [("SUSPECT", "refuted, but the formula holds on the window")]
        return []


class ParamMap:
    """f(n) = a n + b su Z, con i parametri a, b dichiarati insieme, e una tesi
    "iniettiva (o suriettiva) se e solo se cond(a)", scritta con le lettere a e
    b legate che nascondono i parametri. Iniettiva vuol dire a diverso da 0,
    suriettiva a = 1 oppure a = -1: la tesi vale se cond(a) dice lo stesso
    per ogni intero a."""

    CONDS = [("a \\neq 0", lambda a: a != 0), ("a \\neq 1", lambda a: a != 1), ("(a = 1 \\lor a = -1)", lambda a: a in (1, -1)),
             ("a = 1", lambda a: a == 1), ("(a \\neq 0 \\land a \\neq 2)", lambda a: a not in (0, 2)), ("a = -1", lambda a: a == -1)]

    def __init__(self):
        self.surj = rnd.random() < 0.5
        self.cond = rnd.choice(self.CONDS)

    def truth(self, a):
        return a in (1, -1) if self.surj else a != 0

    def tex(self):
        prop = (r"\forall y \in \mathbb{Z}\ \exists x \in \mathbb{Z}\ (f(x) = y)" if self.surj
                else r"\forall a, b \in \mathbb{Z}\ (f(a) = f(b) \implies a = b)")
        return (r"(f \colon \mathbb{Z} \to \mathbb{Z} \land (a, b \in \mathbb{Z} \land \forall n \in \mathbb{Z}\ (f(n) = an + b))) \implies ((%s) \iff %s)"
                % (prop, self.cond[0]))

    def check(self, verdict, detail):
        wrong = [a for a in range(-6, 7) if self.cond[1](a) != self.truth(a)]
        if verdict == "P" and wrong:
            return [("BUG", "proved, but for a = %d the two sides differ" % wrong[0])]
        if verdict == "R":
            values = dict((name, Fr(v)) for name, v in re.findall(r"(\w) = (-?\d+(?:/\d+)?)", detail or ""))
            if "a" in values and values["a"].denominator == 1 and int(values["a"]) not in wrong:
                return [("BUG", "refuted with a = %s, where the two sides agree" % values["a"])]
            if not wrong:
                return [("BUG", "refuted, but the two sides agree for every a")]
        return []


class ComplexMap:
    """f(a + ib) = (p a + q b + r) + i (s a + t b + u), da C in C, e la tesi
    "non iniettiva" o "non suriettiva": tutte e due valgono esattamente quando
    il determinante p t - q s è zero."""

    def __init__(self):
        small = lambda: rnd.choice([-2, -1, 0, 0, 1, 1, 2])
        self.c = [small() for _ in range(6)]
        if rnd.random() < 0.5:
            # Spesso il determinante zero: righe proporzionali.
            k = rnd.choice([-2, -1, 1, 2])
            self.c[3], self.c[4] = k * self.c[0], k * self.c[1]
        self.claim = rnd.choice(["ninj", "nsurj"])

    def det(self):
        p, q, r, s_, t, u = self.c
        return p * t - q * s_

    def tex(self):
        p, q, r, s_, t, u = self.c
        def lin(x, y, z):
            text = ptex([z, x], "a").replace("a", "@")
            body = []
            for coef, letter in ((x, "a"), (y, "b")):
                if coef:
                    body.append(("-" if coef < 0 else "+", ("" if abs(coef) == 1 else str(abs(coef))) + letter))
            if z or not body:
                body.append(("-" if z < 0 else "+", str(abs(z))))
            out = ("-" if body[0][0] == "-" else "") + body[0][1]
            for sign, b in body[1:]:
                out += " %s %s" % (sign, b)
            return out
        value = "(%s) + i(%s)" % (lin(p, q, r), lin(s_, t, u))
        claim = (r"\lnot (\forall a, b \in \mathbb{C}\ (f(a) = f(b) \implies a = b))" if self.claim == "ninj"
                 else r"\lnot (\forall y \in \mathbb{C}\ \exists x \in \mathbb{C}\ (f(x) = y))")
        return r"(f \colon \mathbb{C} \to \mathbb{C} \land \forall a, b \in \mathbb{R}\ (f(a + ib) = %s)) \implies (%s)" % (value, claim)

    def check(self, verdict, detail):
        true = self.det() == 0
        if verdict == "P" and not true:
            return [("BUG", "proved %s, but the determinant is %d" % (self.claim, self.det()))]
        if verdict == "R" and true:
            return [("BUG", "refuted %s, but the determinant is zero" % self.claim)]
        return []


def main():
    cases = []
    for _ in range(COUNT):
        r = rnd.random()
        cases.append(CaseMap() if r < 0.55 else PairMap() if r < 0.7 else IterateMap() if r < 0.8 else ParamMap() if r < 0.9 else ComplexMap())
    path = os.path.join(WORK, "fuzz_cases_%d.tex" % SEED)
    with open(path, "w", encoding="utf-8") as out:
        out.write("\\begin{enumerate}\n")
        for case in cases:
            out.write("\\item $\\displaystyle %s$\n" % case.tex())
        out.write("\\end{enumerate}\n")
    run = subprocess.run([MR, path], capture_output=True, text=True, timeout=3600)
    lines = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
    if len(lines) != len(cases) or run.returncode not in (0, 1):
        print("BUG: the program answered %d of %d statements (exit %d)\n%s" % (len(lines), len(cases), run.returncode, run.stderr[-2000:]))
        return
    tally = {}
    for number, (case, line) in enumerate(zip(cases, lines), 1):
        text = line.split(". ", 1)[1]
        verdict = "P" if text.startswith("DIMOSTRATO") else "R" if text.startswith("FALSO") else "N" if text.startswith("NON DIMOSTRATO") else "U"
        detail = text.split("controesempio ", 1)[1] if "controesempio " in text else None
        tally[verdict] = tally.get(verdict, 0) + 1
        if verdict == "U" or (VERBOSE and verdict == "N" and case.ill_defined() is None):
            print("NOTE %d: %s\n      %s" % (number, text, case.tex()))
        for severity, message in case.check(verdict, detail):
            print("%s %d [%s]: %s\n      %s" % (severity, number, text[:60], message, case.tex()))
    print("seed %d:" % SEED, ", ".join("%s=%d" % (k, v) for k, v in sorted(tally.items())))


main()
