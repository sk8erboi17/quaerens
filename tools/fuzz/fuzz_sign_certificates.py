"""Fuzzing del verificatore sui certificati delle tabelle dei segni (capitolo 12 di checker.c).

Uso: python3 -I fuzz_sign_certificates.py <checker> <seme> <quanti> <cartella di lavoro> <file di certificati>...

I file di certificati sono quelli scritti da mathresearcher --certificates: se
ne prendono i certificati signs, e se ne ricavano <quanti> alterati a caso, con
da una a tre alterazioni ciascuno: la verità di una riga, un numero di una
riga, una riga tolta o aggiunta, un fattore cambiato o tolto, il fattore di una
radice, l'affermazione, l'insieme, un confronto o un numero della condizione,
un confronto o un estremo delle soluzioni scritte. Il verificatore li controlla
tutti, e per ognuno che ACCETTA l'oracolo decide da sé ciò che il certificato
afferma, guardando solo la condizione, l'insieme e l'affermazione: dei fattori
e delle righe non usa nulla.

L'oracolo. Calcola i polinomi della condizione (ogni scelta dei segni dei
valori assoluti, i divisori, gli argomenti dei valori assoluti) e, per le
soluzioni scritte, quelli dei loro estremi a + b sqrt(k): x - a e
(x - a)^2 - b^2 k. Trova tutte le loro radici reali: quelle razionali con il
teorema delle radici razionali, le altre isolandole con Sturm sul prodotto
senza radici multiple. Poi calcola la condizione in un razionale di ogni
intervallo e in ogni radice: nei razionali con le frazioni, nelle radici
irrazionali dal segno dei polinomi, che è zero se il massimo comun divisore con
il polinomio della radice ha una radice nel suo intervallo, altrimenti quello
in un estremo dell'intervallo, stretto finché il polinomio non vi ha radici.
È un conto diverso da quello del verificatore, che invece si fida dei fattori
dati e li prova con le divisioni.

BUG: il verificatore ha accettato un certificato che afferma il falso (o
l'oracolo non è d'accordo con un certificato del motore non alterato).
SUSPECT: un certificato accettato che l'oracolo non sa decidere (numeri troppo
grandi per le sue prove dei divisori, una notazione che non legge).
I file prodotti vanno nella cartella di lavoro, mai nel repository.
"""
import math
import os
import random
import re
import subprocess
import sys
from fractions import Fraction

CHECKER, SEED, COUNT, WORK = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
rnd = random.Random(SEED)
sys.setrecursionlimit(20000)
MAX_DIVISOR_SCAN = 10 ** 7  # oltre, la ricerca dei divisori si ferma: SUSPECT


class Unknown(Exception):
    pass


# ---------------------------------------------------------------- lettura
def tokens(text):
    return re.findall(r"\(|\)|[^\s()]+", text)


def parse(tok, i):
    if tok[i] == "(":
        out, i = [], i + 1
        while tok[i] != ")":
            node, i = parse(tok, i)
            out.append(node)
        return out, i + 1
    return tok[i], i + 1


def tree(text):
    node, used = parse(tokens(text), 0)
    return node


def show(node):
    return node if isinstance(node, str) else "(" + " ".join(show(x) for x in node) + ")"


def read_certificates(paths):
    out = []
    for path in paths:
        for block in open(path, encoding="utf-8").read().split("certificate ")[1:]:
            if block.split("\n", 1)[0].strip() == "signs":
                out.append("certificate " + block.strip() + "\n")
    return out


def entries(text):
    """Le righe del certificato, come elenco di (parola, resto)."""
    out = []
    for line in text.strip().splitlines()[1:]:
        word, _, rest = line.strip().partition(" ")
        out.append((word, rest))
    return out


# ---------------------------------------------------------------- polinomi
# Un polinomio è una lista di Fraction, dal grado 0; [] è il polinomio nullo.
def trim(p):
    p = list(p)
    while p and p[-1] == 0:
        p.pop()
    return p


def padd(p, q):
    n = max(len(p), len(q))
    return trim([(p[i] if i < len(p) else 0) + (q[i] if i < len(q) else 0) for i in range(n)])


def pneg(p):
    return [-c for c in p]


def pmul(p, q):
    if not p or not q:
        return []
    out = [Fraction(0)] * (len(p) + len(q) - 1)
    for i, a in enumerate(p):
        for j, b in enumerate(q):
            out[i + j] += a * b
    return trim(out)


def pdivmod(a, b):
    a, q = list(a), [Fraction(0)] * max(len(a) - len(b) + 1, 1)
    while len(a) >= len(b) and a:
        k, c = len(a) - len(b), a[-1] / b[-1]
        q[k] = c
        for i, x in enumerate(b):
            a[i + k] -= c * x
        a = trim(a)
    return trim(q), a


def pgcd(a, b):
    while b:
        a, b = b, pdivmod(a, b)[1]
    return [c / a[-1] for c in a] if a else a


def deriv(p):
    return trim([i * p[i] for i in range(1, len(p))])


def ev(p, x):
    v = Fraction(0)
    for c in reversed(p):
        v = v * x + c
    return v


def sign(v):
    return (v > 0) - (v < 0)


def squarefree(p):
    return pdivmod(p, pgcd(p, deriv(p)))[0]


def chain(p):
    out = [p, deriv(p)]
    while out[-1]:
        r = pdivmod(out[-2], out[-1])[1]
        if not r:
            break
        out.append(pneg(r))
    return [q for q in out if q]


def variations(ch, x, side=0):
    signs = []
    for q in ch:
        s = sign(ev(q, x)) if x is not None else sign(q[-1]) * (-1 if side < 0 and (len(q) - 1) % 2 else 1)
        if s:
            signs.append(s)
    return sum(1 for a, b in zip(signs, signs[1:]) if a != b)


def roots_between(p, lo, hi):
    """Radici reali distinte di p tra lo e hi (None: infinito), che non sono sue radici."""
    ch = chain(p)
    return variations(ch, lo, -1) - variations(ch, hi, 1)


def divisors(n):
    n = abs(int(n))
    if n > MAX_DIVISOR_SCAN ** 2:
        raise Unknown("number too large for the divisor scan: %d" % n)
    small, d = [], 1
    while d * d <= n:
        if n % d == 0:
            small.append(d)
            small.append(n // d)
        d += 1
    return sorted(set(small))


def integer(p):
    """p moltiplicato per il comune denominatore: coefficienti interi."""
    den = 1
    for c in p:
        den = den * c.denominator // gcd_int(den, c.denominator)
    return [int(c * den) for c in p]


def gcd_int(a, b):
    while b:
        a, b = b, a % b
    return abs(a)


def rational_roots(p):
    """Le radici razionali di p, non nullo: p/q con p | a_0 e q | a_d, dopo aver tolto le x comuni."""
    c = integer(p)
    out = set()
    while c and c[0] == 0:
        out.add(Fraction(0))
        c = c[1:]
    if len(c) <= 1:
        return out
    for num in divisors(c[0]):
        for den in divisors(c[-1]):
            for s in (1, -1):
                r = Fraction(s * num, den)
                if ev(p, r) == 0:
                    out.add(r)
    return out


# ---------------------------------------------------------------- i punti
class Point:
    """Un punto della retta: un razionale (exact), oppure la sola radice di q (senza radici razionali) tra lo e hi."""

    def __init__(self, lo, hi, q=None):
        self.lo, self.hi, self.q = lo, hi, q

    def refine(self):
        mid = (self.lo + self.hi) / 2
        if sign(ev(self.q, mid)) == sign(ev(self.q, self.lo)):
            self.lo = mid
        else:
            self.hi = mid

    def sign_of(self, p):
        if not p:
            return 0
        if self.q is None:
            return sign(ev(p, self.lo))
        g = pgcd(p, self.q)
        if len(g) > 1 and roots_between(g, self.lo, self.hi) == 1:
            return 0
        for _ in range(400):
            if ev(p, self.lo) != 0 and ev(p, self.hi) != 0 and roots_between(p, self.lo, self.hi) == 0:
                return sign(ev(p, self.lo))
            self.refine()
        raise Unknown("cannot separate a root")


def points_of(polys):
    """I punti in ordine: tutte le radici reali dei polinomi dati, ognuna una volta."""
    rational, product = set(), [Fraction(1)]
    for p in polys:
        if len(p) < 2:
            continue
        rational |= rational_roots(p)
        s = squarefree(p)
        g = pgcd(product, s)
        product = pmul(product, pdivmod(s, g)[0])
    for r in rational:
        product = pdivmod(product, [-r, Fraction(1)])[0] if ev(product, r) == 0 else product
    points = [Point(r, r) for r in rational]
    if len(product) > 1:
        bound = 1 + max(abs(c / product[-1]) for c in product[:-1])
        stack = [(-bound, bound)]
        while stack:
            lo, hi = stack.pop()
            n = roots_between(product, lo, hi)
            if n == 0:
                continue
            if n == 1:
                points.append(Point(lo, hi, product))
                continue
            mid = (lo + hi) / 2
            stack += [(lo, mid), (mid, hi)]
    # Separati: si stringono gli intervalli finché non si toccano.
    for _ in range(200):
        points.sort(key=lambda p: p.lo)
        touching = [(a, b) for a, b in zip(points, points[1:]) if a.hi >= b.lo]
        if not touching:
            return points
        for a, b in touching:
            for p in (a, b):
                if p.q is not None:
                    p.refine()
    raise Unknown("points do not separate")


# ---------------------------------------------------------------- la condizione
CONNECTIVES = ("and", "or", "implies", "iff", "not")
COMPARISONS = ("=", "neq", "<", "<=")


def number(tok):
    return Fraction(int(tok))


def value_at(node, x):
    """Il valore dell'espressione in un razionale; None dove una divisione ha il divisore nullo."""
    if isinstance(node, str):
        return x if node.isalpha() else number(node)
    head = node[0]
    args = [value_at(a, x) for a in node[1:]] if head != "^" else [value_at(node[1], x)]
    if any(a is None for a in args):
        return None
    if head == "+":
        return args[0] + args[1]
    if head == "-":
        return args[0] - args[1]
    if head == "*":
        return args[0] * args[1]
    if head == "/":
        return None if args[1] == 0 else args[0] / args[1]
    if head == "neg":
        return -args[0]
    if head == "abs":
        return abs(args[0])
    if head == "^":
        k = exponent(node[2])
        if k < 0 and args[0] == 0:
            return None
        return args[0] ** k
    raise Unknown("operator %r" % head)


def exponent(node):
    if isinstance(node, str):
        return int(node)
    if node[0] == "neg" and isinstance(node[1], str):
        return -int(node[1])
    raise Unknown("exponent %r" % (node,))


def fractions(node, collect):
    """Tutti i valori dell'espressione come frazioni (num, den) di polinomi, uno per ogni scelta dei segni dei valori
    assoluti; None per una scelta in cui si divide per la frazione nulla. Mette in collect i divisori e numeratori e
    denominatori degli argomenti dei valori assoluti."""
    X = [Fraction(0), Fraction(1)]
    if isinstance(node, str):
        return [(X if node.isalpha() else [number(node)] if number(node) else [], [Fraction(1)])]
    head = node[0]
    if head in ("neg", "abs"):
        out = []
        for v in fractions(node[1], collect):
            if v is None:
                out.append(None)
                continue
            collect += [v[0], v[1]] if head == "abs" else []
            out.append((pneg(v[0]), v[1]))
            if head == "abs":
                out.append(v)
        return out
    if head == "^":
        k, out = exponent(node[2]), []
        for v in fractions(node[1], collect):
            if v is None or (k < 0 and not v[0]):
                out.append(None)
                continue
            num, den = (v[1], v[0]) if k < 0 else v
            if k < 0:
                collect.append(v[0])
            a, b = [Fraction(1)], [Fraction(1)]
            for _ in range(abs(k)):
                a, b = pmul(a, num), pmul(b, den)
            out.append((a, b))
        return out
    out = []
    for u in fractions(node[1], collect):
        for v in fractions(node[2], collect):
            if u is None or v is None:
                out.append(None)
            elif head in ("+", "-"):
                w = v[0] if head == "+" else pneg(v[0])
                out.append((padd(pmul(u[0], v[1]), pmul(w, u[1])), pmul(u[1], v[1])))
            elif head == "*":
                out.append((pmul(u[0], v[0]), pmul(u[1], v[1])))
            elif head == "/":
                if not v[0]:
                    out.append(None)
                else:
                    collect.append(v[0])
                    out.append((pmul(u[0], v[1]), pmul(u[1], v[0])))
            else:
                raise Unknown("operator %r" % head)
    return out


def at_point(node, point):
    """Il valore dell'espressione nel punto irrazionale, come frazione (num, den) e il suo essere definita."""
    X = [Fraction(0), Fraction(1)]
    if isinstance(node, str):
        return (X if node.isalpha() else [number(node)] if number(node) else [], [Fraction(1)]), True
    head = node[0]
    if head in ("neg", "abs"):
        v, ok = at_point(node[1], point)
        flip = head == "neg" or point.sign_of(v[0]) * point.sign_of(v[1]) < 0
        return ((pneg(v[0]) if flip else v[0], v[1]), ok)
    if head == "^":
        k = exponent(node[2])
        v, ok = at_point(node[1], point)
        if k < 0:
            ok = ok and point.sign_of(v[0]) != 0
            v = (v[1], v[0])
        a, b = [Fraction(1)], [Fraction(1)]
        for _ in range(abs(k)):
            a, b = pmul(a, v[0]), pmul(b, v[1])
        return (a, b), ok
    u, ok1 = at_point(node[1], point)
    v, ok2 = at_point(node[2], point)
    ok = ok1 and ok2
    if head in ("+", "-"):
        w = v[0] if head == "+" else pneg(v[0])
        return (padd(pmul(u[0], v[1]), pmul(w, u[1])), pmul(u[1], v[1])), ok
    if head == "*":
        return (pmul(u[0], v[0]), pmul(u[1], v[1])), ok
    if head == "/":
        ok = ok and point.sign_of(v[0]) != 0
        return (pmul(u[0], v[1]), pmul(u[1], v[0]) if v[0] else [Fraction(1)]), ok
    raise Unknown("operator %r" % head)


def compared(op, s):
    return {"=": s == 0, "neq": s != 0, "<": s < 0, "<=": s <= 0}[op]


def holds(c, where):
    """La verità della condizione c in where: un razionale, oppure un Point irrazionale."""
    if isinstance(c, str):
        raise Unknown("condition %r" % c)
    head = c[0]
    if head in CONNECTIVES:
        a = holds(c[1], where)
        if head == "not":
            return not a
        b = holds(c[2], where)
        return {"and": a and b, "or": a or b, "implies": (not a) or b, "iff": a == b}[head]
    if head not in COMPARISONS:
        raise Unknown("condition %r" % head)
    if not isinstance(where, Point):
        a, b = value_at(c[1], where), value_at(c[2], where)
        return a is not None and b is not None and compared(head, sign(a - b))
    (ln, ld), ok1 = at_point(c[1], where)
    (rn, rd), ok2 = at_point(c[2], where)
    if not (ok1 and ok2):
        return False
    return compared(head, where.sign_of(padd(pmul(ln, rd), pneg(pmul(rn, ld)))) * where.sign_of(ld) * where.sign_of(rd))


def condition_polys(c, out):
    """I polinomi da cui dipende la verità della condizione."""
    if c[0] in CONNECTIVES:
        for a in c[1:]:
            condition_polys(a, out)
        return
    collect = []
    for l in fractions(c[1], collect):
        for r in fractions(c[2], collect):
            if l is not None and r is not None:
                out += [padd(pmul(l[0], r[1]), pneg(pmul(r[0], l[1]))), l[1], r[1]]
    out += collect


# ---------------------------------------------------------------- le soluzioni scritte
def surd(node):
    """Il numero come (a, b, k): a + b sqrt(k)."""
    if isinstance(node, str):
        return (number(node), Fraction(0), 0)
    head = node[0]
    if head == "sqrt":
        n = int(node[1])
        if n < 0:
            raise Unknown("square root of a negative number")
        r = math.isqrt(n)
        return (Fraction(r), Fraction(0), 0) if r * r == n else (Fraction(0), Fraction(1), n)
    if head == "neg":
        a, b, k = surd(node[1])
        return (-a, -b, k)
    x = surd(node[1])
    if head == "^":
        result, e = (Fraction(1), Fraction(0), 0), exponent(node[2])
        if e < 0:
            x = inverse(x)
        for _ in range(abs(e)):
            result = smul(result, x)
        return result
    y = surd(node[2])
    if head == "+":
        return sadd(x, y)
    if head == "-":
        return sadd(x, (-y[0], -y[1], y[2]))
    if head == "*":
        return smul(x, y)
    if head == "/":
        return smul(x, inverse(y))
    raise Unknown("operator %r" % head)


def common(x, y):
    if x[2] and y[2] and x[2] != y[2]:
        raise Unknown("two different square roots")
    return x[2] or y[2]


def sadd(x, y):
    return (x[0] + y[0], x[1] + y[1], common(x, y))


def smul(x, y):
    k = common(x, y)
    return (x[0] * y[0] + k * x[1] * y[1], x[0] * y[1] + x[1] * y[0], k)


def inverse(x):
    norm = x[0] * x[0] - x[2] * x[1] * x[1]
    if norm == 0:
        raise Unknown("division by zero in the written solutions")
    return (x[0] / norm, -x[1] / norm, x[2])


def endpoint_polys(w, out):
    if w[0] in CONNECTIVES:
        for a in w[1:]:
            endpoint_polys(a, out)
        return
    a, b, k = surd(w[2] if isinstance(w[1], str) and w[1].isalpha() else w[1])
    out.append([-a, Fraction(1)])
    out.append(padd(pmul([-a, Fraction(1)], [-a, Fraction(1)]), [-b * b * k]))


def side_of(where, e):
    """Il segno di x - e, con e = a + b sqrt(k): da u = x - a e da u^2 - b^2 k."""
    a, b, k = e
    u = [-a, Fraction(1)]
    sq = padd(pmul(u, u), [-b * b * k])
    su = where.sign_of(u) if isinstance(where, Point) else sign(where - a)
    if b == 0:
        return su
    ss = where.sign_of(sq) if isinstance(where, Point) else sign(ev(sq, where))
    if b > 0:
        return -1 if su < 0 or ss < 0 else 0 if ss == 0 else 1
    return 1 if su > 0 or ss < 0 else 0 if ss == 0 else -1


def written(w, where):
    head = w[0]
    if head in CONNECTIVES:
        a = written(w[1], where)
        if head == "not":
            return not a
        b = written(w[2], where)
        return {"and": a and b, "or": a or b, "implies": (not a) or b, "iff": a == b}[head]
    left = isinstance(w[1], str) and w[1].isalpha()
    s = side_of(where, surd(w[2] if left else w[1]))
    return compared(head, s if left else -s)


# ---------------------------------------------------------------- l'oracolo
def judge(text):
    """True se ciò che il certificato afferma è vero, False se è falso; Unknown se non si sa dire."""
    e = dict(entries(text))
    domain, claim = e["domain"], e["claim"].split()
    condition = tree(e["condition"])
    solution = e.get("solution")
    polys = []
    condition_polys(condition, polys)
    w = None
    if claim[0] == "solve":
        if domain != "reals":
            raise Unknown("solve outside the reals")
        if solution not in ("all", "none"):
            w = tree(solution)
            endpoint_polys(w, polys)
    pts = points_of([p for p in polys if p])
    regions = []
    for i, p in enumerate(pts):
        lo = pts[i - 1].hi if i else p.lo - 2
        regions.append((lo + p.lo) / 2)
        regions.append(p.lo if p.q is None else p)
    regions.append(pts[-1].hi + 1 if pts else Fraction(0))
    truths = []
    for where in regions:
        rational = not isinstance(where, Point)
        if domain == "rationals" and not rational:
            continue
        c = holds(condition, where)
        if claim[0] == "solve":
            truths.append(c == (written(w, where) if w is not None else solution == "all"))
        else:
            truths.append(c)
    if claim[0] == "solve":
        return all(truths)
    if claim[0] == "forall":
        return all(truths) == (claim[1] == "true")
    return any(truths) == (claim[1] == "true")


# ---------------------------------------------------------------- le alterazioni
def numbers_in(node, out, path=()):
    if isinstance(node, str):
        if re.fullmatch(r"-?\d+", node):
            out.append(path)
        return
    for i, a in enumerate(node[1:], 1):
        numbers_in(a, out, path + (i,))


def replace(node, path, new):
    if not path:
        return new
    node = list(node)
    node[path[0]] = replace(node[path[0]], path[1:], new)
    return node


def get(node, path):
    for i in path:
        node = node[i]
    return node


def comparisons_in(node, out, path=()):
    if isinstance(node, str):
        return
    if node[0] in COMPARISONS:
        out.append(path)
    if node[0] in CONNECTIVES:
        for i, a in enumerate(node[1:], 1):
            comparisons_in(a, out, path + (i,))


def nudge(tok):
    """Un numero vicino, o un altro, nella notazione."""
    v = Fraction(int(tok)) if re.fullmatch(r"-?\d+", tok) else None
    choices = [str(int(v) + d) for d in (-2, -1, 1, 2)] + [str(-int(v)), "0"]
    return rnd.choice(choices)


def alter_tree(node, comparisons):
    if comparisons and rnd.random() < 0.5:
        paths = []
        comparisons_in(node, paths)
        if paths:
            path = rnd.choice(paths)
            op = get(node, path)
            other = rnd.choice([o for o in COMPARISONS if o != op[0]])
            return replace(node, path, [other] + op[1:])
    paths = []
    numbers_in(node, paths)
    if not paths:
        return node
    path = rnd.choice(paths)
    return replace(node, path, nudge(get(node, path)))


def rational_text(v):
    return str(v.numerator) if v.denominator == 1 else "(/ %d %d)" % (v.numerator, v.denominator)


def row_value(line, k=1):
    """Il numero k (da 1) di una riga della tabella; Unknown se un'alterazione precedente l'ha reso senza valore."""
    v = value_at(tree("(" + line + ")")[k], Fraction(0))
    if v is None:
        raise Unknown("a row number with no value")
    return v


def inside_values(lines, rows):
    """Numeri dentro gli intervalli della tabella, ma non i campioni: a metà tra un campione e un punto vicino."""
    out = []
    for n, i in enumerate(rows):
        if not lines[i].startswith("interval"):
            continue
        s = row_value(lines[i])
        if n > 0:
            before = lines[rows[n - 1]]
            out.append((s + row_value(before, 3 if before.startswith("root") else 1)) / 2)
        if n + 1 < len(rows):
            after = lines[rows[n + 1]]
            out.append((s + row_value(after, 2 if after.startswith("root") else 1)) / 2)
    return out


def alter(text):
    lines = text.strip().splitlines()
    rows = [i for i, l in enumerate(lines) if l.split(" ", 1)[0] in ("interval", "point", "root")]
    factors = [i for i, l in enumerate(lines) if l.startswith("factor ")]
    kind = rnd.choice(["truth", "truth", "number", "number", "drop", "insert", "factor", "factor", "root", "claim",
                       "domain", "condition", "condition", "solution", "solution", "move", "move", "sample", "endpoint"])
    if kind == "move":
        # L'intervallo di una radice spostato: può non contenerla più.
        roots = [i for i in rows if lines[i].startswith("root")]
        if roots:
            i = rnd.choice(roots)
            words = tree("(" + lines[i] + ")")
            shift = Fraction(rnd.choice([-1, 1]) * rnd.choice([1, 2, 4, 8, 16]), rnd.choice([8, 16, 64]))
            lo, hi = row_value(lines[i], 2), row_value(lines[i], 3)
            lines[i] = "root %s %s %s %s" % (words[1], rational_text(lo + shift), rational_text(hi + shift), words[4])
        return "\n".join(lines) + "\n"
    if kind == "sample":
        # Il campione di un intervallo preso da un altro.
        intervals = [i for i in rows if lines[i].startswith("interval")]
        if len(intervals) > 1:
            i, j = rnd.sample(intervals, 2)
            value = lines[j].split(" ", 1)[1].rsplit(" ", 1)[0]
            lines[i] = "interval %s %s" % (value, lines[i].rsplit(" ", 1)[1])
        return "\n".join(lines) + "\n"
    if kind == "endpoint":
        # Un estremo delle soluzioni scritte spostato dentro un intervallo, dove nessun campione lo vede.
        found = [i for i, l in enumerate(lines) if l.startswith("solution ") and l[9:] not in ("all", "none")]
        values = inside_values(lines, rows)
        if found and values:
            i = found[0]
            node, paths = tree(lines[i][9:]), []
            comparisons_in(node, paths)
            path = rnd.choice(paths)
            op = get(node, path)
            side = 2 if isinstance(op[1], str) and op[1].isalpha() else 1
            lines[i] = "solution " + show(replace(node, path + (side,), tree(rational_text(rnd.choice(values)))))
        return "\n".join(lines) + "\n"
    if kind == "truth" and rows:
        i = rnd.choice(rows)
        words = lines[i].split(" ")
        words[-1] = "false" if words[-1] == "true" else "true"
        lines[i] = " ".join(words)
    elif kind == "number" and rows:
        i = rnd.choice(rows)
        word, rest = lines[i].split(" ", 1)
        node = tree("(" + rest + ")")
        paths = []
        numbers_in(["row"] + node, paths)
        if paths:
            path = rnd.choice(paths)
            node = replace(["row"] + node, path, nudge(get(["row"] + node, path)))[1:]
            lines[i] = word + " " + " ".join(show(a) for a in node)
    elif kind == "drop" and len(rows) > 1:
        # Un punto con l'intervallo che lo segue, o un intervallo con il punto che lo precede.
        i = rnd.choice(rows[1:])
        start = i if lines[i].startswith(("point", "root")) else i - 1
        del lines[start:start + 2]
    elif kind == "insert" and rows:
        i = rnd.choice(rows)
        if lines[i].startswith("interval"):
            v = Fraction(rnd.randint(-40, 40), rnd.choice([1, 2, 3, 4, 8]))
            lines[i + 1:i + 1] = ["point %s %s" % (rational_text(v), rnd.choice(["true", "false"])),
                                  "interval %s %s" % (rational_text(v + Fraction(1, 64)), rnd.choice(["true", "false"]))]
    elif kind == "factor" and factors:
        i = rnd.choice(factors)
        if rnd.random() < 0.3:
            del lines[i]
        else:
            lines[i] = "factor " + show(alter_tree(tree(lines[i][7:]), False))
    elif kind == "root":
        roots = [i for i in rows if lines[i].startswith("root")]
        if roots:
            i = rnd.choice(roots)
            words = lines[i].split(" ")
            words[1] = str(max(1, int(words[1]) + rnd.choice([-1, 1])))
            lines[i] = " ".join(words)
    elif kind == "claim":
        i = next(i for i, l in enumerate(lines) if l.startswith("claim"))
        if lines[i] != "claim solve":
            lines[i] = rnd.choice(["claim forall true", "claim forall false", "claim exists true", "claim exists false"])
    elif kind == "domain":
        i = next(i for i, l in enumerate(lines) if l.startswith("domain"))
        lines[i] = "domain rationals" if lines[i] == "domain reals" else "domain reals"
    elif kind == "condition":
        i = next(i for i, l in enumerate(lines) if l.startswith("condition"))
        lines[i] = "condition " + show(alter_tree(tree(lines[i][10:]), True))
    elif kind == "solution":
        found = [i for i, l in enumerate(lines) if l.startswith("solution ")]
        if found:
            i = found[0]
            rest = lines[i][9:]
            if rest in ("all", "none") or rnd.random() < 0.15:
                lines[i] = "solution " + rnd.choice(["all", "none"])
            else:
                lines[i] = "solution " + show(alter_tree(tree(rest), True))
    return "\n".join(lines) + "\n"


# ---------------------------------------------------------------- il giro
def run_checker(texts, name):
    path = os.path.join(WORK, name)
    with open(path, "w", encoding="utf-8") as out:
        for t in texts:
            out.write(t + "\n")
    result = subprocess.run([CHECKER, path], capture_output=True, text=True)
    answers = {}
    for line in result.stdout.splitlines():
        m = re.match(r"\s*(\d+)\. (ACCETTATO|RESPINTO|NON CONTROLLABILE)", line)
        if m:
            answers[int(m.group(1)) - 1] = m.group(2)
    return [answers.get(i, "?") for i in range(len(texts))]


def main():
    originals = read_certificates(sys.argv[5:])
    if not originals:
        print("no sign certificates in the given files")
        return 1
    os.makedirs(WORK, exist_ok=True)
    bugs = suspects = 0

    # I certificati del motore: accettati, e veri per l'oracolo.
    for i, (text, answer) in enumerate(zip(originals, run_checker(originals, "originals.cert"))):
        try:
            truth = judge(text)
        except Unknown as e:
            suspects += 1
            print("SUSPECT: original %d cannot be judged (%s)" % (i + 1, e))
            continue
        if not truth:
            bugs += 1
            print("BUG: original certificate %d claims something false (checker: %s):\n%s" % (i + 1, answer, text))

    # Le alterazioni.
    mutants = []
    for _ in range(COUNT):
        text = rnd.choice(originals)
        for _ in range(rnd.choice([1, 1, 2, 3])):
            try:
                text = alter(text)
            except (Unknown, ValueError, IndexError):
                pass  # un'alterazione che non si applica al testo già alterato: lo lascia com'è
        mutants.append(text)
    answers = run_checker(mutants, "mutants_%d.cert" % SEED)
    tally = {}
    for text, answer in zip(mutants, answers):
        tally[answer] = tally.get(answer, 0) + 1
        if answer != "ACCETTATO":
            continue
        try:
            truth = judge(text)
        except (Unknown, ZeroDivisionError, KeyError, IndexError, ValueError) as e:
            suspects += 1
            print("SUSPECT: accepted certificate cannot be judged (%s):\n%s" % (e, text))
            continue
        if not truth:
            bugs += 1
            print("BUG: the checker accepted a certificate that claims something false:\n%s" % text)
    print("%d originals, %d mutants: %s; %d BUG, %d SUSPECT" % (len(originals), len(mutants), tally, bugs, suspects))
    return 1 if bugs else 0


if __name__ == "__main__":
    sys.exit(main())
