r"""Fuzzing differenziale degli enunciati con una variabile reale.

Uso: python3 -I fuzz_reals.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Genera equazioni, disequazioni e sistemi in una variabile reale (polinomi e
frazioni scritti come prodotti di fattori, valori assoluti) insieme al loro
insieme di soluzioni, e ne fa enunciati della forma

    per ogni x reale: condizione <=> soluzioni

veri per costruzione. Poi ne ricava di falsi: un estremo spostato, un < al
posto di un <=, un pezzo dimenticato. Aggiunge enunciati con "esiste", sui
razionali, sui reali positivi, con una negazione davanti. Infine chiede al
programma di risolvere, con \{x \in \mathbb{R} \mid condizione\}: le soluzioni
che il programma scrive vengono lette qui e confrontate con quelle
dell'oracolo in ogni radice e in ogni intervallo.

L'oracolo è indipendente dal programma e non usa né il teorema di Sturm né
massimi comuni divisori. Conosce le radici di ogni fattore, perché le ha
scelte lui: sono numeri razionali, oppure (p +- k sqrt(d))/q. Tra due radici
consecutive nessuna espressione cambia segno, quindi calcola l'enunciato in
ogni radice e in un punto di ogni intervallo, con le frazioni esatte; nelle
radici irrazionali con i numeri della forma a + b sqrt(d), esatti anche loro.
Un'espressione non definita in un punto rende falso il confronto che la
contiene: è la convenzione dei libri, e del programma.

Segnala BUG quando il programma dimostra un enunciato falso, dichiara falso
un enunciato vero, o scrive soluzioni diverse da quelle dell'oracolo.
SUSPECT quando non riesce a leggere le soluzioni scritte dal programma.
"""
import os
import random
import re
import subprocess
import sys
from fractions import Fraction

MR, SEED, COUNT, WORK = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
rnd = random.Random(SEED)


# ---------------------------------------------------------------- numeri a + b sqrt(d)
class Quad:
    """Il numero a + b sqrt(d), con a e b razionali e d intero non quadrato."""

    def __init__(self, a, b=0, d=2):
        self.a, self.b, self.d = Fraction(a), Fraction(b), d

    def lift(self, other):
        return other if isinstance(other, Quad) else Quad(other, 0, self.d)

    def __add__(self, other):
        other = self.lift(other)
        return Quad(self.a + other.a, self.b + other.b, self.d)

    __radd__ = __add__

    def __neg__(self):
        return Quad(-self.a, -self.b, self.d)

    def __sub__(self, other):
        return self + (-self.lift(other))

    def __rsub__(self, other):
        return self.lift(other) - self

    def __mul__(self, other):
        other = self.lift(other)
        return Quad(self.a * other.a + self.b * other.b * self.d, self.a * other.b + self.b * other.a, self.d)

    __rmul__ = __mul__

    def __truediv__(self, other):
        other = self.lift(other)
        norm = other.a * other.a - other.b * other.b * self.d
        if norm == 0:
            raise ZeroDivisionError
        return self * Quad(other.a / norm, -other.b / norm, self.d)

    def __rtruediv__(self, other):
        return self.lift(other) / self

    def sign(self):
        """Il segno di a + b sqrt(d): se a e b hanno segni opposti decide il confronto tra a^2 e d b^2."""
        sa, sb = (self.a > 0) - (self.a < 0), (self.b > 0) - (self.b < 0)
        if sb == 0 or sa == sb:
            return sa
        if sa == 0:
            return sb
        gap = self.a * self.a - self.d * self.b * self.b
        return sa * ((gap > 0) - (gap < 0))


def sign(value):
    return value.sign() if isinstance(value, Quad) else (value > 0) - (value < 0)


# ---------------------------------------------------------------- espressioni
# Un'espressione è una tupla: ("x",), ("num", Fraction), ("+", a, b), ("-", a, b),
# ("*", a, b), ("/", a, b), ("^", a, k), ("abs", a), ("neg", a).
def value(e, x):
    op = e[0]
    if op == "x":
        return x
    if op == "num":
        return e[1]
    if op == "neg":
        return -value(e[1], x)
    if op == "abs":
        v = value(e[1], x)
        return -v if sign(v) < 0 else v
    if op == "^":
        v, out = value(e[1], x), Fraction(1)
        for _ in range(e[2]):
            out = out * v
        return out
    a, b = value(e[1], x), value(e[2], x)
    if op == "+":
        return a + b
    if op == "-":
        return a - b
    if op == "*":
        return a * b
    if sign(b) == 0:
        raise ZeroDivisionError
    return a / b


def frac_tex(q):
    if q.denominator == 1:
        return str(q.numerator)
    return "%s\\frac{%d}{%d}" % ("-" if q < 0 else "", abs(q.numerator), q.denominator)


def tex(e, top=True):
    op = e[0]
    if op == "x":
        return "x"
    if op == "num":
        return frac_tex(e[1]) if top or e[1] >= 0 else "\\left(%s\\right)" % frac_tex(e[1])
    if op == "neg":
        return "-%s" % tex(e[1], False)
    if op == "abs":
        return "\\left|%s\\right|" % tex(e[1])
    if op == "^":
        return "%s^{%d}" % (tex(e[1], False), e[2])
    if op == "/":
        return "\\frac{%s}{%s}" % (tex(e[1]), tex(e[2]))
    if op == "*":
        return "%s \\cdot %s" % (tex(e[1], False), tex(e[2], False))
    inner = "%s %s %s" % (tex(e[1]), op, tex(e[2], False) if op == "-" else tex(e[2]))
    return inner if top else "\\left(%s\\right)" % inner


# Una condizione è ("cmp", relazione, a, b), ("and", p, q), ("or", p, q), ("not", p), ("iff", p, q).
RELATIONS = {"<": lambda s: s < 0, "\\le": lambda s: s <= 0, ">": lambda s: s > 0, "\\ge": lambda s: s >= 0,
             "=": lambda s: s == 0, "\\neq": lambda s: s != 0}


def holds(c, x):
    op = c[0]
    if op == "cmp":
        try:
            return RELATIONS[c[1]](sign(value(c[2], x) - value(c[3], x)))
        except ZeroDivisionError:
            return False
    if op == "not":
        return not holds(c[1], x)
    if op == "and":
        return holds(c[1], x) and holds(c[2], x)
    if op == "or":
        return holds(c[1], x) or holds(c[2], x)
    return holds(c[1], x) == holds(c[2], x)


def ctex(c, top=True):
    op = c[0]
    if op == "cmp":
        return "%s %s %s" % (tex(c[2]), c[1], tex(c[3]))
    if op == "not":
        return "\\lnot \\left(%s\\right)" % ctex(c[1])
    if op == "cases":
        return "\\begin{cases} %s \\end{cases}" % " \\\\ ".join(ctex(p) for p in c[1:])
    word = {"and": "\\land", "or": "\\lor", "iff": "\\iff"}[op]
    inner = "%s %s %s" % (ctex(c[1], False), word, ctex(c[2], False))
    return inner if top else "\\left(%s\\right)" % inner


def holds_all(c, x):
    return all(holds(p, x) for p in c[1:]) if c[0] == "cases" else holds(c, x)


# ---------------------------------------------------------------- generazione
def num(v):
    return ("num", Fraction(v))


X = ("x",)


def rational():
    return Fraction(rnd.randint(-6, 6), rnd.choice([1, 1, 1, 2, 3]))


def plus(e, v):
    """e + v, scritto e - |v| quando v è negativo: "+ -3" non è una scrittura da libro."""
    return ("+", e, num(v)) if v >= 0 else ("-", e, num(-v))


def factor(roots, d):
    """Un fattore e le sue radici: (x - r), (a x + b), x^2 + c, x^2 - d, oppure
    q^2 x^2 - 2 p q x + p^2 - k^2 d, che ha le radici (p +- k sqrt(d)) / q."""
    kind = rnd.random()
    if kind < 0.5:
        r = rational()
        roots.append(r)
        return plus(X, -r)
    if kind < 0.7:
        a, b = rnd.choice([2, 3, -1, -2, 5]), rnd.randint(-7, 7)
        roots.append(Fraction(-b, a))
        return plus(("*", num(a), X), b)
    if kind < 0.8:
        return ("+", ("^", X, 2), num(rnd.randint(1, 5)))
    if kind < 0.9:
        roots.append(Quad(0, 1, d))
        roots.append(Quad(0, -1, d))
        return ("-", ("^", X, 2), num(d))
    p, k, q = rnd.randint(-3, 3), rnd.choice([1, 1, 2, 3]), rnd.choice([1, 1, 2])
    roots.append(Quad(Fraction(p, q), Fraction(k, q), d))
    roots.append(Quad(Fraction(p, q), Fraction(-k, q), d))
    square = ("^", X, 2) if q == 1 else ("*", num(q * q), ("^", X, 2))
    return plus(plus(square, 0) if p == 0 else ("-" if p > 0 else "+", square, ("*", num(abs(2 * p * q)), X)), p * p - k * k * d)


def product(roots, d, count):
    e = None
    for _ in range(count):
        f = factor(roots, d)
        if rnd.random() < 0.25:
            f = ("^", f, rnd.choice([2, 3]))
        e = f if e is None else ("*", e, f)
    return e


def inequality(roots, d):
    """Una condizione in x, con le radici di tutto ciò che vi compare."""
    relation = rnd.choice(["<", "\\le", ">", "\\ge", "<", ">", "=", "\\neq"])
    shape = rnd.random()
    if shape < 0.35:
        return ("cmp", relation, product(roots, d, rnd.randint(1, 3)), num(0))
    if shape < 0.7:
        return ("cmp", relation, ("/", product(roots, d, rnd.randint(1, 2)), product(roots, d, rnd.randint(1, 2))), num(0))
    if shape < 0.85:
        # |a x + b| ? c: le radici sono dove a x + b vale 0, c e -c.
        a, b, c = rnd.choice([1, 2, 3]), rnd.randint(-5, 5), rnd.randint(0, 6)
        for v in (0, c, -c):
            roots.append(Fraction(v - b, a))
        return ("cmp", relation, ("abs", plus(("*", num(a), X), b)), num(c))
    # |x - p| ? |x - q|: cambia in p, in q e a metà strada.
    p, q = rational(), rational()
    roots.extend([p, q, (p + q) / 2])
    return ("cmp", relation, ("abs", plus(X, -p)), ("abs", plus(X, -q)))


def as_float(v):
    return float(v.a) + float(v.b) * v.d ** 0.5 if isinstance(v, Quad) else float(v)


def before(u, v):
    """u < v, esatto, per razionali e numeri con la stessa radice."""
    return sign(v - u) > 0


def cells(roots):
    """I punti in cui provare: ogni radice, e un razionale in ogni intervallo."""
    distinct = []
    for r in roots:
        if not any(sign(r - s) == 0 for s in distinct):
            distinct.append(r)
    distinct.sort(key=as_float)
    for i in range(len(distinct) - 1):
        assert before(distinct[i], distinct[i + 1])
    if not distinct:
        return [(Fraction(0), None, None)]
    out = [(Fraction(int(as_float(distinct[0])) - 2), None, distinct[0])]
    for i, r in enumerate(distinct):
        out.append((r, r, r))
        if i + 1 < len(distinct):
            lo, hi = as_float(r), as_float(distinct[i + 1])
            middle, den = None, 2
            while middle is None:
                candidate = Fraction(round((lo + hi) / 2 * den), den)
                if before(r, candidate) and before(candidate, distinct[i + 1]):
                    middle = candidate
                den *= 2
            out.append((middle, r, distinct[i + 1]))
    out.append((Fraction(int(as_float(distinct[-1])) + 2), distinct[-1], None))
    return out


def side(relation, point, d):
    """La condizione "x relation point", per un razionale o per +-sqrt(d), scritta senza radicali.
    Per le altre radici irrazionali ritorna None: non c'è una scrittura senza radicali altrettanto semplice."""
    if not isinstance(point, Quad):
        return ("cmp", relation, X, num(point))
    if point.a != 0 or abs(point.b) != 1:
        return None
    square, zero, positive = ("^", X, 2), num(0), point.b > 0
    if relation == "=":
        return ("and", ("cmp", "=", square, num(d)), ("cmp", ">" if positive else "<", X, zero))
    strict = relation in ("<", ">")
    above = relation in (">", "\\ge")
    if positive:
        if above:
            return ("and", ("cmp", ">", X, zero), ("cmp", ">" if strict else "\\ge", square, num(d)))
        return ("or", ("cmp", "\\le", X, zero), ("cmp", "<" if strict else "\\le", square, num(d)))
    if above:
        return ("or", ("cmp", "\\ge", X, zero), ("cmp", "<" if strict else "\\le", square, num(d)))
    return ("and", ("cmp", "<", X, zero), ("cmp", ">" if strict else "\\ge", square, num(d)))


def solution(condition, roots, d):
    """L'insieme delle soluzioni come condizione: unione di punti e intervalli. Ritorna anche i pezzi."""
    table = [(sample, lo, hi, holds_all(condition, sample)) for sample, lo, hi in cells(roots)]
    pieces, i = [], 0
    while i < len(table):
        if not table[i][3]:
            i += 1
            continue
        j = i
        while j + 1 < len(table) and table[j + 1][3]:
            j += 1
        # Le righe da i a j sono vere: il primo e l'ultimo dicono se gli estremi sono compresi.
        first, last = table[i], table[j]
        low = None if first[1] is None else (first[1], first[1] is first[2])
        high = None if last[2] is None else (last[2], last[1] is last[2])
        pieces.append((low, high))
        i = j + 1
    return pieces


def piece_condition(piece, d):
    low, high = piece
    if low and high and sign(low[0] - high[0]) == 0:
        return side("=", low[0], d)
    parts = []
    if low:
        parts.append(side("\\ge" if low[1] else ">", low[0], d))
    if high:
        parts.append(side("\\le" if high[1] else "<", high[0], d))
    if None in parts:
        return None
    if not parts:
        return ("cmp", "=", num(0), num(0))
    return parts[0] if len(parts) == 1 else ("and", parts[0], parts[1])


def pieces_condition(pieces, d):
    if not pieces:
        return ("cmp", "=", num(0), num(1))
    parts = [piece_condition(p, d) for p in pieces]
    if None in parts:
        return None
    out = parts[0]
    for p in parts[1:]:
        out = ("or", out, p)
    return out


def spoil(pieces, roots):
    """Un insieme di soluzioni sbagliato di poco: ritorna None se non trova come sbagliarlo."""
    pieces = [list(p) for p in pieces]
    kind = rnd.random()
    if kind < 0.3 and pieces:
        del pieces[rnd.randrange(len(pieces))]
        return [tuple(p) for p in pieces]
    candidates = [(i, k) for i, p in enumerate(pieces) for k in (0, 1) if p[k] and not isinstance(p[k][0], Quad)]
    if not candidates:
        return None
    i, k = rnd.choice(candidates)
    point, closed = pieces[i][k]
    if kind < 0.65:
        pieces[i][k] = (point, not closed)
    else:
        moved = point + rnd.choice([Fraction(1, 2), Fraction(-1, 2), 1, -1, Fraction(1, 7)])
        pieces[i][k] = (moved, closed)
        roots.append(moved)
    low, high = pieces[i]
    if low and high and not before(low[0], high[0]) and not (sign(low[0] - high[0]) == 0 and low[1] and high[1]):
        return None
    return [tuple(p) for p in pieces]


def make_case():
    """Ritorna (testo LaTeX, verità, dati): verità è True o False per un enunciato, None per una
    richiesta di soluzioni, e allora dati = (condizione, radici, d) serve a controllare la risposta."""
    d = rnd.choice([2, 3, 5, 7])
    roots = []
    count = rnd.choice([1, 1, 1, 2, 2, 3])
    parts = [inequality(roots, d) for _ in range(count)]
    condition = parts[0] if count == 1 else ("cases",) + tuple(parts)
    pieces = solution(condition, roots, d)
    shape = rnd.random()
    if shape < 0.3:
        return "\\{x \\in \\mathbb{R} \\mid %s\\}" % ctex(condition), None, (condition, roots, d)
    if shape < 0.55:
        result = pieces_condition(pieces, d)
        if result is None:
            return make_case()
        return "\\forall x \\in \\mathbb{R}\\ \\left( %s \\iff \\left( %s \\right) \\right)" % (ctex(condition), ctex(result)), True, None
    if shape < 0.8:
        wrong = spoil(pieces, roots)
        result = pieces_condition(wrong, d) if wrong is not None else None
        if result is None:
            return make_case()
        truth = all(holds_all(condition, s) == holds(result, s) for s, _, _ in cells(roots))
        return "\\forall x \\in \\mathbb{R}\\ \\left( %s \\iff \\left( %s \\right) \\right)" % (ctex(condition), ctex(result)), truth, None
    # Enunciati senza "se e solo se": per ogni / esiste, su R, Q o R+, con o senza negazione.
    where = rnd.choice(["R", "R", "Q", "R+", "Q+"])
    domain = {"R": "\\mathbb{R}", "Q": "\\mathbb{Q}", "R+": "\\mathbb{R}^{+}", "Q+": "\\mathbb{Q}^{+}"}[where]
    table = cells(roots + [Fraction(0)])
    usable = [(s, lo, hi) for s, lo, hi in table
              if ("+" not in where or sign(s) > 0) and ("Q" not in where or not isinstance(s, Quad))]
    truths = [holds_all(condition, s) for s, _, _ in usable]
    every = rnd.random() < 0.5
    truth = all(truths) if every else any(truths)
    text = "\\%s x \\in %s\\ \\left( %s \\right)" % ("forall" if every else "exists", domain, ctex(condition))
    if rnd.random() < 0.3:
        text, truth = "\\lnot " + text, not truth
    return text, truth, None


# ---------------------------------------------------------------- lettura delle soluzioni del programma
def read_number(text, d, names):
    """Un estremo scritto dal programma: un razionale, (p +- k sqrt(d)) / q, o un nome r_{k} che sta in names.
    None se non lo si sa leggere."""
    text = text.strip()
    if text in names:
        return names[text]
    negative = text.startswith("-\\frac")
    if negative:
        text = text[1:]
    m = re.fullmatch(r"\\frac\{(.*)\}\{(\d+)\}", text)
    top, bottom = (m.group(1), int(m.group(2))) if m else (text, 1)
    m = re.fullmatch(r"(?:(-?\d+) ([+-]) )?(-?)(\d*)\\sqrt\{(\d+)\}", top)
    if m:
        if int(m.group(5)) != d:
            return None
        k = int(m.group(4) or 1) * (-1 if m.group(2) == "-" or m.group(3) == "-" else 1)
        value = Quad(Fraction(int(m.group(1) or 0), bottom), Fraction(k, bottom), d)
    elif re.fullmatch(r"-?\d+", top):
        value = Fraction(int(top), bottom)
    else:
        return None
    return -value if negative else value


def read_solution(text, d, names):
    """Le soluzioni scritte dal programma, come lista di pezzi (basso, compreso, alto, compreso); None se illeggibili."""
    if text == "\\nexists x \\in \\mathbb{R}":
        return []
    pieces = []
    for part in text.split(" \\lor "):
        if part == "\\forall x \\in \\mathbb{R}":
            pieces.append((None, False, None, False))
            continue
        m = re.fullmatch(r"(?:(.+?) (<|\\le) )?x (<|\\le|>|\\ge|=) (.+)", part)
        if not m:
            return None
        right = read_number(m.group(4), d, names)
        left = read_number(m.group(1), d, names) if m.group(1) else None
        if right is None or (m.group(1) and (left is None or m.group(3) not in ("<", "\\le"))):
            return None
        if m.group(1):
            pieces.append((left, m.group(2) == "\\le", right, m.group(3) == "\\le"))
        elif m.group(3) == "=":
            pieces.append((right, True, right, True))
        elif m.group(3) in ("<", "\\le"):
            pieces.append((None, False, right, m.group(3) == "\\le"))
        else:
            pieces.append((right, m.group(3) == "\\ge", None, False))
    return pieces


def in_pieces(pieces, x):
    for low, low_closed, high, high_closed in pieces:
        above = low is None or sign(x - low) > 0 or (low_closed and sign(x - low) == 0)
        below = high is None or sign(high - x) > 0 or (high_closed and sign(high - x) == 0)
        if above and below:
            return True
    return False


def check_solution(answer, data):
    """Confronta le soluzioni scritte dal programma con la condizione, in ogni radice e in ogni
    intervallo, contando tra le radici anche gli estremi che il programma ha scritto.
    Ritorna "ok", "bug" oppure "suspect"."""
    condition, roots, d = data
    text = answer.split(": ", 1)[1].split(" (estremi irrazionali")[0]

    # Una radice senza scrittura ha un nome, e il programma dice tra quali numeri sta: l'oracolo, che
    # conosce tutte le radici, deve trovarne lì una sola, ed è quella.
    names = {}
    for m in re.finditer(r"(r_\{\d+\}) è la radice di .*? tra (-?\d+(?:/\d+)?) e (-?\d+(?:/\d+)?)", answer):
        low, high = Fraction(m.group(2)), Fraction(m.group(3))
        inside = [r for r, lo, hi in cells(roots) if lo is hi and lo is not None and sign(r - low) > 0 and sign(high - r) > 0]
        if len(inside) != 1:
            return "bug"
        names[m.group(1)] = inside[0]
    pieces = read_solution(text, d, names)
    if pieces is None:
        return "suspect"
    ends = [e for low, _, high, _ in pieces for e in (low, high) if e is not None]
    for sample, _, _ in cells(roots + ends):
        if holds_all(condition, sample) != in_pieces(pieces, sample):
            return "bug"
    return "ok"


def main():
    cases = [make_case() for _ in range(COUNT)]
    path = os.path.join(WORK, "fuzz_reals_%d.tex" % SEED)
    with open(path, "w", encoding="utf-8") as out:
        out.write("\\begin{enumerate}\n")
        for text, truth, data in cases:
            out.write("\\item \\[ %s \\]\n" % text)
        out.write("\\end{enumerate}\n")
    run = subprocess.run([MR, path], capture_output=True, text=True, errors="replace", timeout=1800)
    lines = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
    if len(lines) != len(cases) or run.returncode not in (0, 1) or "Sanitizer" in run.stderr or "runtime error" in run.stderr:
        print("BUG: the program answered %d of %d statements (exit %d)\n%s" % (len(lines), len(cases), run.returncode, run.stderr[-2000:]))
        return
    tally = {}
    for number, ((text, truth, data), line) in enumerate(zip(cases, lines), 1):
        answer = line.split(". ", 1)[1]
        verdict = ("P" if answer.startswith("DIMOSTRATO") else "R" if answer.startswith("FALSO") else "S" if answer.startswith("RISOLTO")
                   else "N" if answer.startswith("NON DIMOSTRATO") else "U")
        kind = "solve" if truth is None else "true" if truth else "false"
        tally[(kind, verdict)] = tally.get((kind, verdict), 0) + 1
        if truth is None and verdict == "S":
            outcome = check_solution(answer, data)
            if outcome == "bug":
                print("BUG: statement %d, the written solutions are wrong: %s\n   %s" % (number, answer[:200], text))
            elif outcome == "suspect":
                print("SUSPECT: statement %d, cannot read the written solutions: %s\n   %s" % (number, answer[:200], text))
        elif (truth is None) != (verdict == "S") and verdict in ("P", "R", "S"):
            print("BUG: statement %d got the wrong kind of answer: %s\n   %s" % (number, answer[:90], text))
        elif (verdict == "P" and not truth) or (verdict == "R" and truth):
            print("BUG: statement %d is %s but the program says %s\n   %s" % (number, "true" if truth else "false", answer[:90], text))
        elif verdict in ("N", "U"):
            print("NOTE: statement %d not decided (%s)\n   %s" % (number, answer[:100], text[:300]))
    if "ATTENZIONE" in run.stdout:
        print("BUG: the program discarded a step of its own proof: %s" % [l for l in run.stdout.splitlines() if "ATTENZIONE" in l])
    print("seed %d:" % SEED, ", ".join("%s/%s=%d" % (k[0], k[1], v) for k, v in sorted(tally.items())))


main()
