"""Fuzzing differenziale degli enunciati sulle derivate (capitolo 27 di main.c).

Uso:

  python3 -I fuzz_derivatives.py fuzz <eseguibile> <seme> <quanti> <cartella di lavoro>
      Costruisce enunciati veri da famiglie note (e^{ax+b} e c^{ax+b}, seno e
      coseno di ax+b, (ax+b)^r con r intero o razionale, anche con \\prod
      nella formula, ln(ax+b) e log_c, x e^{ax}, x^2 e^{ax}, x sin, sin cos,
      polinomi e somme con \\sum, un parametro a, somme di due famiglie,
      e^x sin x che il programma non sa dimostrare, e funzioni composte a
      caso con un ordine fisso e la derivata calcolata qui), e ne ricava
      varianti false: n spostato di uno, un segno, un fattore, un numero
      cambiato, un termine dimenticato o aggiunto, un termine che si annulla
      nei primi n, n pi/3 al posto di n pi/2, e la "derivata" f / x^k che
      sarebbe vera algebra se \\frac{d}{dx} fosse la frazione delle lettere d
      e x (\\frac{d}{dx} x^{2} = x). Li scrive in un file .tex nella
      cartella di lavoro, in tre forme (\\[ ... \\], $\\dfrac{d}{dx} ...$,
      $\\displaystyle \\frac{d}{dx} ...$) e a volte con gli argomenti senza
      parentesi come nei libri (\\sin 2x, \\cos^{2} 3x), li fa decidere al
      programma e confronta i verdetti con l'oracolo. Con la variabile
      d'ambiente FUZZ_NOTES stampa anche le righe NOTE; con FUZZ_SIMPLE
      scrive tutto nella sola forma \\[ ... \\] con le parentesi, per
      separare gli errori del lettore da quelli del motore.
      Le funzioni composte comprendono arcoseno, arcocoseno, arcocotangente,
      valore assoluto (anche con le sole sbarre), u^v con x nella base e
      nell'esponente, logaritmi con x nella base, radici cubiche di numeri
      negativi. Una parte degli enunciati di ordine fisso diventa una
      richiesta, senza il segno di uguale: il programma risponde con la
      derivata scritta, che qui si rilegge e si confronta con la derivata
      vera nei punti di prova.

  python3 -I fuzz_derivatives.py studies <eseguibile> <seme> <quanti> <cartella di lavoro>
      Studi di funzione: costruisce richieste \\{x \\in \\mathbb{R} \\mid
      \\frac{d^{k}}{dx^{k}}\\left( f \\right) > 0\\} (o <, \\ge, \\le, = 0) per
      funzioni di famiglie che la tabella dei segni tratta, e controlla le
      soluzioni scritte dal programma: il segno della derivata vera, con le
      serie di Taylor di questo file, su una griglia fitta di punti e negli
      estremi scritti. Poi, dalle soluzioni confermate, costruisce enunciati
      \\forall x\\ (... \\iff soluzioni), veri, e varianti con un estremo
      spostato o compreso, o un pezzo tolto, e confronta i verdetti con
      l'oracolo.

  python3 -I fuzz_derivatives.py check <eseguibile> <file .tex>...
      Rilegge con il lettore di questo file ogni enunciato sulle derivate dei
      file (gli \\item che non lo sono vengono saltati), dice se è vero
      secondo l'oracolo, e lo confronta con il verdetto del programma e con
      il nome del file: in un file false_*.tex gli enunciati devono essere
      falsi, negli altri veri. Legge anche gli studi di funzione scritti con
      i quantificatori, \\forall x \\in \\mathbb{R}\\ (\\frac{d}{dx}(f) > 0
      \\iff ...), che l'oracolo decide sulla griglia e negli estremi.

L'oracolo non usa il programma. Legge il LaTeX con un lettore suo, scritto
sulla grammatica del capitolo 9 di parser.c e non sul suo codice, e calcola
le derivate di ordine k = n0, ..., n0 + 6 (o l'ordine fisso) con le serie di
Taylor troncate, in aritmetica decimale a 80 cifre, in dieci punti x e con tre
scelte dei parametri. Le serie sono scritte qui in un altro modo rispetto a
main.c: le potenze con la ricorrenza di u p' = r u' p, la tangente con
t' = (1 + t^2) u', il logaritmo e l'arcotangente come integrale di una serie.
La formula si calcola negli stessi punti, con n = k. Due numeri che
differiscono oltre 10^-25 della loro grandezza sono diversi; sotto 10^-40
sono uguali; in mezzo l'oracolo non decide. L'arcoseno è l'integrale della
serie di u' / sqrt(1 - u^2) (main.c usa invece cos(y) y' = u'), arcocoseno e
arcocotangente sono pi/2 meno arcoseno e arcotangente, |u|' = u' |u| / u.

Negli studi di funzione la verità di una condizione si guarda su una
griglia di 601 punti tra -12 e 12, negli estremi scritti, e nei punti
speciali che l'oracolo trova da sé con 120 cifre: gli zeri delle derivate e
delle differenze tra i membri, i poli, i bordi del dominio (zeros_of()).
Dove una derivata non si sa se esiste (x|x| in 0), o un numero è troppo
piccolo per dire se è zero, l'oracolo non decide.

Righe stampate:

  BUG: il programma ha dimostrato un enunciato che l'oracolo trova falso,
       oppure ha detto "falso per n = w" (o "controesempio x = p/q") quando
       lì, nei punti e con i parametri che il programma usa, la formula è
       giusta; oppure ha scritto una derivata, o delle soluzioni, che in un
       punto sono di sicuro sbagliate. Anche la riga ATTENZIONE del
       programma, che conta i passi scartati dai suoi controlli: una regola
       di derive() smentita dalla serie di Taylor è un errore del programma,
       anche se il verdetto resta prudente.
  SUSPECT: l'oracolo non riesce a decidere un confronto che serve.
  NOTE: un enunciato vero non dimostrato, o che l'oracolo non decide, e che
        il programma non dimostra (non è un errore).
  DATA: in modalità check, un enunciato di un file di soli veri che l'oracolo
        trova falso, o il contrario.
"""
import math
import os
import random
import re
import subprocess
import sys
from decimal import Decimal as D, getcontext
from fractions import Fraction as Fr

getcontext().prec = 80
SAME = D(10) ** -40         # sotto: uguali
DIFFERENT = D(10) ** -25    # sopra: diversi
WINDOW = 6                  # ordini controllati oltre il caso base
MAX_UNROLL = 4096           # termini di una somma svolta

NOTES = bool(os.environ.get("FUZZ_NOTES"))
SIMPLE = bool(os.environ.get("FUZZ_SIMPLE"))


class Undefined(Exception):
    """In quel punto un'espressione non è definita o non è derivabile."""


class Unsupported(Exception):
    """Il lettore o l'oracolo non sanno trattare questa scrittura."""


class NotAnalytic(Undefined):
    """La serie di un pezzo non c'è perché lì il pezzo non è derivabile, come
    |u| o una radice dove u = 0: la funzione intera, come x|x| in 0, può
    esserlo lo stesso."""


class Unsure(Exception):
    """L'oracolo non sa se in quel punto la derivata esiste."""


# =============================================================================
# 1. I numeri reali con 80 cifre: pi greco, seno, coseno, arcotangente.
#    exp, ln e la radice quadrata li dà il modulo decimal. Le cifre sono
#    quelle del contesto in uso: 80 di solito, FINE nei punti speciali degli
#    studi di funzione (capitolo 10 di questo file).
# =============================================================================

def series_eps():
    """Sotto questa grandezza un termine di una serie non conta più."""
    return D(10) ** -(getcontext().prec + 5)


def tiny():
    """Sotto questa grandezza un numero calcolato si prende per zero, quando
    serve sapere se una divisione, un logaritmo o una radice hanno senso: un
    numero che vicino a un polo vale 10^-120 con 120 cifre è lo zero del
    polo, non un numero piccolo."""
    return D(10) ** -(getcontext().prec - 10)


def _atan_small(y):
    """La serie y - y^3/3 + y^5/5 - ..., per |y| piccolo."""
    total, power, y2, k = y, y, y * y, 1
    while True:
        power = -power * y2
        k += 2
        term = power / k
        if abs(term) < series_eps():
            return total
        total += term


def _pi():
    """Pi greco con 150 cifre, perché basti anche ai conti con FINE cifre."""
    previous = getcontext().prec
    getcontext().prec = 150
    value = 16 * _atan_small(D(1) / 5) - 4 * _atan_small(D(1) / 239)
    getcontext().prec = previous
    return value


PI = _pi()


def d_atan(v):
    """atan(v) = 2 atan(v / (1 + sqrt(1 + v^2))), finché l'argomento è piccolo."""
    doublings = 0
    while abs(v) > D("0.1"):
        v = v / (1 + (1 + v * v).sqrt())
        doublings += 1
    return _atan_small(v) * (2 ** doublings)


def d_sin_cos(v):
    """Seno e coseno: si toglie il multiplo di 2 pi più vicino, poi le serie."""
    turns = (v / (2 * PI)).to_integral_value()
    r = v - turns * 2 * PI
    s, c, term, k = D(0), D(0), D(1), 0
    while True:
        if k % 4 == 0:
            c += term
        elif k % 4 == 1:
            s += term
        elif k % 4 == 2:
            c -= term
        else:
            s -= term
        k += 1
        term = term * r / k
        if abs(term) < series_eps() and k > 4:
            return s, c


def d_of(q):
    return D(q.numerator) / D(q.denominator)


# =============================================================================
# 2. Le espressioni: alberi di tuple.
#
#   ("num", Fraction)  ("var", lettera)  ("e",)  ("pi",)
#   ("add", a, b) ("sub", a, b) ("mul", a, b) ("div", a, b) ("neg", a) ("pow", a, b)
#   ("fn", nome, a) con nome tra sin cos tan cot ln arctan arcsin arccos arccot
#   ("log", base, a)  ("root", m, a)  ("fact", a)  ("abs", a)
#   ("sum", indice, da, a, corpo)  ("prod", indice, da, a, corpo)
#   ("deriv", lettera, k, f): il segno di derivata dentro un'espressione
# =============================================================================

def N(v):
    return ("num", Fr(v))


X, NV = ("var", "x"), ("var", "n")
E, PIT = ("e",), ("pi",)
add = lambda a, b: ("add", a, b)
sub = lambda a, b: ("sub", a, b)
mul = lambda a, b: ("mul", a, b)
div = lambda a, b: ("div", a, b)
pw = lambda a, b: ("pow", a, b)
neg = lambda a: ("neg", a)
fn = lambda name, a: ("fn", name, a)
exp_ = lambda a: ("pow", E, a)
fact = lambda a: ("fact", a)


def letters(t, found=None):
    """Le lettere libere di t."""
    found = set() if found is None else found
    if t[0] == "var":
        found.add(t[1])
    elif t[0] == "deriv":
        letters(t[3], found)
    elif t[0] in ("sum", "prod"):
        letters(t[2], found)
        letters(t[3], found)
        inner = letters(t[4])
        inner.discard(t[1])
        found |= inner
    else:
        for part in t[1:]:
            if isinstance(part, tuple):
                letters(part, found)
    return found


def subst(t, name, value):
    """t con la lettera libera name sostituita da value."""
    if t[0] == "var":
        return value if t[1] == name else t
    if t[0] in ("sum", "prod"):
        body = t[4] if t[1] == name else subst(t[4], name, value)
        return (t[0], t[1], subst(t[2], name, value), subst(t[3], name, value), body)
    return tuple(subst(p, name, value) if isinstance(p, tuple) else p for p in t)


# =============================================================================
# 3. Il lettore di LaTeX, indipendente da parser.c.
# =============================================================================

FUNCTIONS = {"sin", "cos", "tan", "cot", "ln", "arctan", "arcsin", "arccos", "arccot"}
SKIPPED = {"left", "right", "displaystyle", "textstyle", "quad", "qquad", "bigl", "bigr", "Bigl", "Bigr"}


def tokenize(text):
    out, i = [], 0
    while i < len(text):
        c = text[i]
        if c.isspace():
            i += 1
        elif c == "\\":
            j = i + 1
            if j < len(text) and not text[j].isalpha():
                if text[j] not in ",;!: ":
                    out.append(("chr", text[j]))
                i = j + 1
                continue
            while j < len(text) and text[j].isalpha():
                j += 1
            name = text[i + 1:j]
            if name in ("left", "right") and text[j:j + 1] == "|":
                # Il valore assoluto: \left| apre e \right| chiude.
                out.append(("chr", "|<" if name == "left" else "|>"))
                j += 1
            elif name in ("left", "right") and text[j:j + 1] in "[]" and text[j:j + 1]:
                # Le parentesi quadre sono parentesi tonde.
                out.append(("chr", "(" if name == "left" else ")"))
                j += 1
            elif name not in SKIPPED:
                out.append(("cmd", name))
            i = j
        elif c.isdigit():
            j = i
            while j < len(text) and text[j].isdigit():
                j += 1
            out.append(("num", text[i:j]))
            i = j
        elif c.isalpha():
            out.append(("let", c))
            i += 1
        else:
            out.append(("chr", c))
            i += 1
    out.append(("end", ""))
    return out


class Reader:
    """Discesa ricorsiva sulla grammatica del capitolo 9: due fattori accostati
    si moltiplicano, l'argomento di una funzione senza parentesi è un solo
    fattore, il corpo di una somma è il term che la segue, un esponente senza
    graffe è un solo token. Il segno di derivata deriva il term che lo segue,
    anche dentro un'espressione; una sbarra |, dentro un valore assoluto
    aperto con una sbarra, lo chiude e non comincia un fattore; \\log senza
    base è in base 10."""

    def __init__(self, text):
        self.t = tokenize(text)
        self.i = 0
        self.bars = 0

    def peek(self):
        return self.t[self.i]

    def take(self):
        token = self.t[self.i]
        self.i += 1
        return token

    def is_chr(self, c):
        return self.peek() == ("chr", c)

    def expect(self, c):
        if not self.is_chr(c):
            raise Unsupported("expected %r, found %r" % (c, self.peek()))
        self.take()

    def leibniz_ahead(self):
        """Vero se dal token corrente, un \\frac, comincia un segno di derivata."""
        ahead = self.t[self.i + 1:self.i + 4]
        if len(ahead) < 3 or ahead[0] != ("chr", "{") or ahead[1] != ("let", "d"):
            return False
        return ahead[2] in (("chr", "}"), ("chr", "^"))

    def derivative(self, request=False):
        """\\frac{d^n}{dx^n} term = expr: ritorna (x, ordine, funzione, formula).
        Con request il segno di uguale può mancare, e la formula è None."""
        x, order = self.leibniz()
        function = self.term()
        if request and self.peek()[0] == "end":
            return x, order, function, None
        self.expect("=")
        formula = self.expr()
        if self.peek()[0] != "end":
            raise Unsupported("text after the formula")
        return x, order, function, formula

    def leibniz(self):
        """Il segno \\frac{d^n}{dx^n}: ritorna (x, ordine)."""
        if self.take() not in (("cmd", "frac"), ("cmd", "dfrac"), ("cmd", "tfrac")):
            raise Unsupported("not a derivative")
        self.expect("{")
        if self.take() != ("let", "d"):
            raise Unsupported("not a derivative")
        above = self.exponent()
        self.expect("}")
        self.expect("{")
        if self.take() != ("let", "d"):
            raise Unsupported("not a derivative")
        kind, var = self.take()
        if kind != "let":
            raise Unsupported("not a derivative")
        below = self.exponent()
        self.expect("}")
        if above != below:
            raise Unsupported("different orders above and below")
        if above is None:
            order = 1
        elif above[0] == "var":
            order = above[1]
        elif above[0] == "num" and above[1].denominator == 1 and above[1] >= 1:
            order = int(above[1])
        else:
            raise Unsupported("order")
        return var, order

    def exponent(self):
        """Un esponente ^group, se c'è; altrimenti None."""
        if not self.is_chr("^"):
            return None
        self.take()
        return self.group()

    def expr(self):
        if self.is_chr("-"):
            self.take()
            left = neg(self.term())
        else:
            if self.is_chr("+"):
                self.take()
            left = self.term()
        while self.is_chr("+") or self.is_chr("-"):
            op = self.take()[1]
            right = self.term()
            left = add(left, right) if op == "+" else sub(left, right)
        return left

    def starts_factor(self):
        kind, value = self.peek()
        if kind in ("num", "let"):
            return True
        if kind == "chr":
            return value in ("(", "|<") or (value == "|" and self.bars == 0)
        return kind == "cmd" and (value in FUNCTIONS or value in ("frac", "dfrac", "tfrac", "sqrt", "pi", "sum", "prod", "log", "binom"))

    def term(self):
        left = self.factor()
        while True:
            if self.is_chr("/"):
                self.take()
                left = div(left, self.factor())
            elif self.peek() in (("cmd", "cdot"), ("cmd", "times")):
                self.take()
                left = mul(left, self.factor())
            elif self.starts_factor():
                left = mul(left, self.factor())
            else:
                return left

    def factor(self):
        base = self.primary()
        while True:
            if self.is_chr("^"):
                self.take()
                base = pw(base, self.group())
            elif self.is_chr("!"):
                self.take()
                if self.is_chr("!"):
                    raise Unsupported("double factorial")
                base = fact(base)
            else:
                return base

    def group(self):
        if self.is_chr("{"):
            self.take()
            inside = self.expr()
            self.expect("}")
            return inside
        kind, value = self.take()
        if kind == "num":
            # Come in TeX: x^12 è x^1 seguito da 2.
            if len(value) > 1:
                self.i -= 1
                self.t[self.i] = ("num", value[1:])
                return N(int(value[0]))
            return N(int(value))
        if kind == "let":
            return ("var", value) if value != "e" else E
        if (kind, value) == ("cmd", "pi"):
            return PIT
        raise Unsupported("group %r" % value)

    def argument(self):
        """L'argomento di una funzione: tra parentesi; oppure un numero
        seguito da lettere, che è tutto argomento, come nei libri
        (\\sin 2x è sin(2x), \\cos^{2} 3x è cos(3x)^2); altrimenti un fattore."""
        if self.is_chr("("):
            self.take()
            inside = self.expr()
            self.expect(")")
            return inside
        inside = self.factor()
        if inside[0] == "num":
            while self.peek()[0] == "let":
                inside = mul(inside, self.factor())
        return inside

    def primary(self):
        kind, value = self.take()
        if kind == "num":
            return N(int(value))
        if kind == "let":
            if self.is_chr("_"):
                raise Unsupported("subscript")
            return E if value == "e" else ("var", value)
        if kind == "chr" and value == "(":
            inside = self.expr()
            self.expect(")")
            return inside
        if kind == "chr" and value == "|<":
            inside = self.expr()
            self.expect("|>")
            return ("abs", inside)
        if kind == "chr" and value == "|":
            self.bars += 1
            inside = self.expr()
            self.bars -= 1
            self.expect("|")
            return ("abs", inside)
        if kind == "cmd":
            if value == "pi":
                return PIT
            if value in ("frac", "dfrac", "tfrac"):
                self.i -= 1
                if self.leibniz_ahead():
                    var, order = self.leibniz()
                    if not isinstance(order, int):
                        raise Unsupported("order of a derivative inside an expression")
                    return ("deriv", var, order, self.term())
                self.i += 1
                top = self.group()
                return div(top, self.group())
            if value == "binom":
                top = self.group()
                return ("binom", top, self.group())
            if value == "sqrt":
                index = 2
                if self.is_chr("["):
                    self.take()
                    index_tree = self.expr()
                    self.expect("]")
                    if index_tree[0] != "num" or index_tree[1].denominator != 1 or index_tree[1] < 2:
                        raise Unsupported("root index")
                    index = int(index_tree[1])
                return ("root", index, self.group())
            if value in FUNCTIONS or value == "log":
                base = None
                if value == "log":
                    base = N(10)
                    if self.is_chr("_"):
                        self.take()
                        base = self.group()
                power = None
                if self.is_chr("^"):
                    self.take()
                    power = self.group()
                inner = self.argument()
                call = ("log", base, inner) if value == "log" else fn(value, inner)
                return pw(call, power) if power is not None else call
            if value in ("sum", "prod"):
                self.expect("_")
                self.expect("{")
                kind2, index = self.take()
                if kind2 != "let":
                    raise Unsupported("index")
                self.expect("=")
                low = self.expr()
                self.expect("}")
                self.expect("^")
                high = self.group()
                return (value, index, low, high, self.term())
        raise Unsupported("token %r" % value)


def read_items(path):
    """Gli \\item di un file .tex: per ognuno (numero, testo)."""
    text = open(path, encoding="utf-8").read()
    start = text.find("\\begin{enumerate}")
    end = text.rfind("\\end{enumerate}")
    body = text[start + len("\\begin{enumerate}"):end if end >= 0 else len(text)]
    parts = re.split(r"\\item\b", body)[1:]
    return [(i + 1, p.strip()) for i, p in enumerate(parts)]


def read_statement(item):
    """Dall'\\item l'enunciato: la formula con il segno di derivata (in \\[ \\] o
    tra $) e il caso base "n \\ge n0" scritto dopo. None se non è una derivata."""
    pieces = re.findall(r"\\\[(.*?)\\\]|\$(.*?)\$", item, re.S)
    for math_text in (a or b for a, b in pieces):
        if not re.match(r"\s*(\\(display|text)style\s*)?\\[dt]?frac\s*\{\s*d", math_text):
            continue
        var, order, f, g = Reader(math_text).derivative(request=True)
        base = 0
        if isinstance(order, str):
            rest = item.split(math_text, 1)[1]
            found = re.search(r"%s\s*\\ge\s*\{?(-?\d+)" % order, rest)
            if found:
                base = int(found.group(1))
        return {"x": var, "order": order, "f": f, "g": g, "base": base, "request": g is None}
    return None


# =============================================================================
# 4. Le serie di Taylor troncate, con i decimali.
#    Una serie è una lista [c_0, ..., c_K], con c_k = f^(k)(x0) / k!.
# =============================================================================

def t_const(c, K):
    return [c] + [D(0)] * K


def t_add(a, b):
    return [p + q for p, q in zip(a, b)]


def t_neg(a):
    return [-p for p in a]


def t_mul(a, b):
    return [sum((a[i] * b[k - i] for i in range(k + 1)), D(0)) for k in range(len(a))]


def t_div(a, b):
    if abs(b[0]) < tiny():
        raise Undefined("division by zero")
    q = []
    for k in range(len(a)):
        q.append((a[k] - sum((b[i] * q[k - i] for i in range(1, k + 1)), D(0))) / b[0])
    return q


def t_deriv(a):
    """La serie della derivata, un grado in meno."""
    return [(k + 1) * a[k + 1] for k in range(len(a) - 1)]


def t_integ(q, c0):
    """La serie con c_0 dato e derivata q."""
    return [c0] + [q[k - 1] / k for k in range(1, len(q) + 1)]


def t_exp(a):
    e = [a[0].exp()]
    for k in range(1, len(a)):
        e.append(sum((i * a[i] * e[k - i] for i in range(1, k + 1)), D(0)) / k)
    return e


def t_ln(a):
    # Un argomento più piccolo di tiny() è lo zero del bordo del dominio.
    if a[0] <= tiny():
        raise Undefined("logarithm of a non-positive number")
    if len(a) == 1:
        return [a[0].ln()]
    return t_integ(t_div(t_deriv(a), a[:-1]), a[0].ln())


def t_sin_cos(a):
    s0, c0 = d_sin_cos(a[0])
    s, c = [s0], [c0]
    for k in range(1, len(a)):
        s.append(sum((i * a[i] * c[k - i] for i in range(1, k + 1)), D(0)) / k)
        c.append(-sum((i * a[i] * s[k - i] for i in range(1, k + 1)), D(0)) / k)
    return s, c


def t_tan(a, sign):
    """tan u (sign = 1) da t' = (1 + t^2) u', cot u (sign = -1) da c' = -(1 + c^2) u'."""
    s0, c0 = d_sin_cos(a[0])
    if abs(c0 if sign > 0 else s0) < tiny():
        raise Undefined("tangent at a pole")
    t = [s0 / c0 if sign > 0 else c0 / s0]
    for k in range(1, len(a)):
        w = lambda j: (1 if j == 0 else 0) + sum((t[l] * t[j - l] for l in range(j + 1)), D(0))
        t.append(sign * sum((i * a[i] * w(k - i) for i in range(1, k + 1)), D(0)) / k)
    return t


def t_atan(a):
    if len(a) == 1:
        return [d_atan(a[0])]
    one_plus = t_add(t_const(D(1), len(a) - 1), t_mul(a, a))
    return t_integ(t_div(t_deriv(a), one_plus[:-1]), d_atan(a[0]))


def t_asin(a):
    """arcsin u come integrale di u' / sqrt(1 - u^2), con il valore
    arctan(u_0 / sqrt(1 - u_0^2)) in x0; fuori da |u_0| < 1 non è
    derivabile."""
    if abs(a[0]) >= 1 - tiny():
        raise Undefined("arcsine outside (-1, 1)")
    y0 = d_atan(a[0] / (1 - a[0] * a[0]).sqrt())
    if len(a) == 1:
        return [y0]
    root = t_pow_const(t_add(t_const(D(1), len(a) - 1), t_neg(t_mul(a, a))), Fr(1, 2))
    return t_integ(t_div(t_deriv(a), root[:-1]), y0)


def t_deriv_order(a, k):
    """La serie della derivata k-esima, k gradi in meno."""
    for _ in range(k):
        a = t_deriv(a)
    return a


def t_pow_const(a, r):
    """a^r con r razionale costante: prodotti per r intero, altrimenti la
    ricorrenza di a p' = r a' p, che vuole a_0 diverso da zero."""
    K = len(a) - 1
    if r.denominator == 1 and 0 <= r <= 64:
        p = t_const(D(1), K)
        for _ in range(int(r)):
            p = t_mul(p, a)
        return p
    if r.denominator == 1 and r < 0:
        return t_div(t_const(D(1), K), t_pow_const(a, -r))
    if abs(a[0]) < tiny() and r > 0:
        # La base è zero: il valore c'è, la serie no.
        if K == 0:
            return [D(0)]
        raise NotAnalytic("power of zero")
    if a[0] < 0 and r.denominator % 2 == 1:
        sign = -1 if r.numerator % 2 else 1
        return [sign * c for c in t_pow_const(t_neg(a), r)]
    if a[0] <= 0:
        raise Undefined("power of a non-positive number")
    p = [(d_of(r) * a[0].ln()).exp()]
    rd = d_of(r)
    for k in range(1, K + 1):
        p.append(sum((((rd + 1) * i - k) * a[i] * p[k - i] for i in range(1, k + 1)), D(0)) / (k * a[0]))
    return p


def exact(t, env):
    """Il valore esatto di t se è un razionale che si calcola con le frazioni:
    niente x, e, pi, funzioni. Altrimenti None."""
    op = t[0]
    if op == "num":
        return t[1]
    if op == "var":
        return env.get(t[1])
    if op in ("add", "sub", "mul", "div"):
        a, b = exact(t[1], env), exact(t[2], env)
        if a is None or b is None:
            return None
        if op == "div":
            if b == 0:
                raise Undefined("division by zero")
            return a / b
        return a + b if op == "add" else a - b if op == "sub" else a * b
    if op == "neg":
        a = exact(t[1], env)
        return None if a is None else -a
    if op == "pow":
        a, b = exact(t[1], env), exact(t[2], env)
        if a is None or b is None or b.denominator != 1 or abs(b) > 400:
            return None
        if a == 0 and b < 0:
            raise Undefined("zero to a negative power")
        return a ** int(b)
    if op == "fact":
        a = exact(t[1], env)
        if a is None or a.denominator != 1:
            return None
        if a < 0:
            raise Undefined("factorial of a negative number")
        return Fr(math.factorial(int(a)))
    if op == "binom":
        a, b = exact(t[1], env), exact(t[2], env)
        if a is None or b is None or a.denominator != 1 or b.denominator != 1:
            return None
        return Fr(math.comb(int(a), int(b))) if 0 <= b <= a else Fr(0)
    if op == "abs":
        a = exact(t[1], env)
        return None if a is None else abs(a)
    if op in ("sum", "prod"):
        low, high = exact(t[2], env), exact(t[3], env)
        if low is None or high is None or low.denominator != 1 or high.denominator != 1:
            return None
        total = Fr(1 if op == "prod" else 0)
        for i in range(int(low), int(high) + 1):
            v = exact(t[4], dict(env, **{t[1]: Fr(i)}))
            if v is None:
                return None
            total = total * v if op == "prod" else total + v
        return total
    return None


def taylor(t, x, x0, K, env):
    """La serie di Taylor di t in x = x0 (un Decimal), fino al grado K; le
    altre lettere hanno i valori razionali di env."""
    value = exact(t, env) if x not in letters(t) else None
    if value is not None:
        return t_const(d_of(value), K)
    op = t[0]
    if op == "var":
        if t[1] == x:
            return [x0] + ([D(1)] if K else []) + [D(0)] * (K - 1)
        raise Unsupported("letter %s without a value" % t[1])
    if op == "e":
        return t_const(D(1).exp(), K)
    if op == "pi":
        return t_const(PI, K)
    rec = lambda u: taylor(u, x, x0, K, dict(env))
    if op == "add":
        return t_add(rec(t[1]), rec(t[2]))
    if op == "sub":
        return t_add(rec(t[1]), t_neg(rec(t[2])))
    if op == "neg":
        return t_neg(rec(t[1]))
    if op == "mul":
        return t_mul(rec(t[1]), rec(t[2]))
    if op == "div":
        return t_div(rec(t[1]), rec(t[2]))
    if op == "pow":
        r = exact(t[2], env) if x not in letters(t[2]) else None
        if r is not None:
            return t_pow_const(rec(t[1]), r)
        return t_exp(t_mul(rec(t[2]), t_ln(rec(t[1]))))
    if op == "fn":
        a = rec(t[2])
        name = t[1]
        if name in ("sin", "cos"):
            s, c = t_sin_cos(a)
            return s if name == "sin" else c
        if name == "tan":
            return t_tan(a, 1)
        if name == "cot":
            return t_tan(a, -1)
        if name == "ln":
            return t_ln(a)
        if name == "arctan":
            return t_atan(a)
        if name == "arccot":
            return t_add(t_const(PI / 2, K), t_neg(t_atan(a)))
        if name == "arcsin":
            return t_asin(a)
        if name == "arccos":
            return t_add(t_const(PI / 2, K), t_neg(t_asin(a)))
    if op == "deriv":
        if t[1] != x:
            raise Unsupported("a derivative with respect to another letter")
        try:
            return t_deriv_order(taylor(t[3], x, x0, K + t[2], dict(env)), t[2])
        except NotAnalytic:
            # Un pezzo non è derivabile lì. Se la funzione non è definita da
            # una delle due parti, la derivata non c'è; altrimenti non si sa.
            delta = D(10) ** -30
            for side in (x0 - delta, x0 + delta):
                try:
                    taylor(t[3], x, side, 0, dict(env))
                except Undefined:
                    raise Undefined("derivative at an edge of the domain")
            raise Unsure("derivative at a point where a piece is not differentiable")
    if op == "log":
        return t_div(t_ln(rec(t[2])), t_ln(rec(t[1])))
    if op == "root":
        return t_pow_const(rec(t[2]), Fr(1, t[1]))
    if op == "abs":
        a = rec(t[1])
        if abs(a[0]) < tiny():
            if K == 0:
                return [D(0)]
            raise NotAnalytic("absolute value at zero")
        return a if a[0] > 0 else t_neg(a)
    if op in ("sum", "prod"):
        low, high = exact(t[2], env), exact(t[3], env)
        if low is None or high is None or low.denominator != 1 or high.denominator != 1:
            raise Unsupported("bounds of a sum")
        if high < low - 1 or high - low >= MAX_UNROLL:
            raise Undefined("sum with a bad upper bound")
        total = t_const(D(1 if op == "prod" else 0), K)
        for i in range(int(low), int(high) + 1):
            term = taylor(t[4], x, x0, K, dict(env, **{t[1]: Fr(i)}))
            total = t_mul(total, term) if op == "prod" else t_add(total, term)
        return total
    raise Unsupported("operation %s" % op)


# =============================================================================
# 5. L'oracolo.
# =============================================================================

ENGINE_POINTS = [Fr(3, 10), Fr(7, 5), Fr(-9, 10), Fr(13, 5), Fr(-11, 5), Fr(23, 4)]
POINTS = ENGINE_POINTS + [Fr(1, 7), Fr(2, 3), Fr(5, 2), Fr(-1, 3), Fr(-7, 4), Fr(11, 5), Fr(-5, 2)]


def engine_values(names):
    """I valori che il programma dà alle altre lettere (sample_tree(), capitolo
    27): servono solo a ricontrollare un suo controesempio."""
    return {c: Fr(3 + ord(c) % 7, 2 + ord(c) % 5) for c in names}


def parameters(st):
    """Le lettere dell'enunciato che non sono né x né l'ordine."""
    return (letters(st["f"]) | letters(st["g"])) - {st["x"], st["order"] if isinstance(st["order"], str) else None}


def parameter_sets(names):
    """Tre scelte dei valori delle lettere: quella del programma e due altre."""
    choices = [Fr(1, 3), Fr(5, 2), Fr(-3, 2), Fr(7, 3), Fr(-2, 5), Fr(4, 3)]
    sets = [engine_values(names)]
    for shift in (1, 3):
        sets.append({c: choices[(i + shift) % len(choices)] for i, c in enumerate(sorted(names))})
    return sets


def compare(a, b):
    """1 uguali, -1 diversi, 0 non si sa."""
    gap, scale = abs(a - b), 1 + abs(b)
    if gap <= SAME * scale:
        return 1
    if gap > DIFFERENT * scale:
        return -1
    return 0


def judge(st, orders, points=POINTS, sets=None):
    """Confronta la derivata vera con la formula, per ogni ordine in orders,
    ogni punto e ogni scelta dei parametri. Ritorna un dizionario
    {k: lista di (esito, punto, parametri)}, con esito come compare()."""
    x, order = st["x"], st["order"]
    sets = parameter_sets(parameters(st)) if sets is None else sets
    results = {k: [] for k in orders}
    top = max(orders)
    # Se la funzione contiene la lettera dell'ordine (x^n), la sua serie
    # cambia con k e si calcola per ogni k.
    moving = isinstance(order, str) and order in letters(st["f"])
    for values in sets:
        for point in points:
            series = None
            for k in orders:
                env = dict(values)
                if isinstance(order, str):
                    env[order] = Fr(k)
                try:
                    if series is None or moving:
                        series = taylor(st["f"], x, d_of(point), top, env)
                    claimed = taylor(st["g"], x, d_of(point), 0, env)[0]
                except (Undefined, ArithmeticError):
                    # Non definita, o un numero troppo grande per i decimali
                    # (una potenza (x^2 + 1)^u con u grande): il punto si salta.
                    series = None
                    continue
                true_value = series[k] * math.factorial(k)
                results[k].append((compare(true_value, claimed), point, values))
    return results


def verdict_of(st):
    """("true", None), ("false", k) con il più piccolo k sbagliato, oppure
    ("unsure", k)."""
    orders = [st["order"]] if isinstance(st["order"], int) else list(range(st["base"], st["base"] + WINDOW + 1))
    results = judge(st, orders)
    for k in orders:
        if any(r[0] < 0 for r in results[k]):
            return "false", k
    for k in orders:
        if not results[k] or any(r[0] == 0 for r in results[k]):
            return "unsure", k
    return "true", None


def witness_holds(st, k, point=None):
    """1 se nei punti del programma, con i suoi parametri, la formula è di
    sicuro sbagliata per l'ordine k; 0 se vi è giusta ovunque si calcola; None
    se non si sa."""
    results = judge(st, [k], [point] if point is not None else ENGINE_POINTS, [engine_values(parameters(st))])[k]
    if any(r[0] < 0 for r in results):
        return 1
    if results and all(r[0] > 0 for r in results):
        return 0
    return None


# =============================================================================
# 6. La scrittura in LaTeX.
# =============================================================================

def number_tex(q):
    """Un razionale in LaTeX, con la precedenza come tex()."""
    if q.denominator == 1:
        return str(q.numerator), (3 if q >= 0 else 1)
    body = r"\frac{%d}{%d}" % (abs(q.numerator), q.denominator)
    return (body, 3) if q > 0 else ("-" + body, 1)


def paren(s):
    """s tra parentesi."""
    return r"\left(%s\right)" % s


def bare_argument(t):
    """Il testo dell'argomento t di una funzione scritto senza parentesi, come
    nei libri, se t è x oppure un intero c >= 2 per x (\\sin 2x); altrimenti
    None."""
    if t == X:
        return "x"
    if t[0] == "mul" and t[2] == X and t[1][0] == "num" and t[1][1].denominator == 1 and t[1][1] >= 2:
        return "%dx" % t[1][1]
    return None


def tex(t, bare=False):
    """(testo, precedenza): 1 somma, 2 prodotto, 3 atomo. Con bare gli
    argomenti semplici delle funzioni sono scritti senza parentesi, e i
    fattori di un prodotto sono separati da \\cdot: senza, \\sin 2x x si
    leggerebbe sin(2x x)."""
    op = t[0]
    rec = lambda u: tex(u, bare)
    if op == "num":
        return number_tex(t[1])
    if op == "var":
        return t[1], 3
    if op == "e":
        return "e", 3
    if op == "pi":
        return r"\pi", 3
    if op in ("add", "sub"):
        a, _ = rec(t[1])
        b, pb = rec(t[2])
        if b.startswith("-") or (op == "sub" and pb < 2):
            b = paren(b)
        return "%s %s %s" % (a, "+" if op == "add" else "-", b), 1
    if op == "neg":
        a, pa = rec(t[1])
        return "-" + (paren(a) if pa < 2 or a.startswith("-") else a), 1
    if op == "mul":
        a, pa = rec(t[1])
        b, pb = rec(t[2])
        a = paren(a) if pa < 2 else a
        b = paren(b) if pb < 2 or b.startswith("-") else b
        # Due numeri accostati si leggerebbero come uno solo.
        joiner = r" \cdot " if b[0].isdigit() or bare else " "
        return a + joiner + b, 2
    if op == "div":
        return r"\frac{%s}{%s}" % (rec(t[1])[0], rec(t[2])[0]), 3
    if op == "pow":
        if bare and t[1][0] == "fn" and bare_argument(t[1][2]):
            return "\\%s^{%s} %s" % (t[1][1], rec(t[2])[0], bare_argument(t[1][2])), 3
        a, pa = rec(t[1])
        if pa < 3 or t[1][0] in ("pow", "div", "fn", "log", "root", "fact") or (t[1][0] == "num" and t[1][1].denominator != 1):
            a = paren(a)
        return "%s^{%s}" % (a, rec(t[2])[0]), 3
    if op == "fn":
        if bare and bare_argument(t[2]):
            return "\\%s %s" % (t[1], bare_argument(t[2])), 3
        return "\\%s%s" % (t[1], paren(rec(t[2])[0])), 3
    if op == "log":
        return r"\log_{%s}%s" % (rec(t[1])[0], paren(rec(t[2])[0])), 3
    if op == "root":
        inner = rec(t[2])[0]
        return (r"\sqrt{%s}" % inner if t[1] == 2 else r"\sqrt[%d]{%s}" % (t[1], inner)), 3
    if op == "fact":
        a, pa = rec(t[1])
        return (a if t[1][0] in ("var", "num") and pa == 3 else paren(a)) + "!", 3
    if op == "abs":
        inside = rec(t[1])[0]
        # Come nei libri, a volte con le sole sbarre: solo se dentro non ce
        # ne sono altre, che renderebbero la scrittura ambigua.
        if bare and "|" not in inside:
            return "|%s|" % inside, 3
        return r"\left|%s\right|" % inside, 3
    if op in ("sum", "prod"):
        body, pb = rec(t[4])
        return r"\%s_{%s=%s}^{%s} %s" % (op, t[1], rec(t[2])[0], rec(t[3])[0], paren(body) if pb < 3 else body), 1
    raise ValueError(op)


def statement_tex(st):
    """L'\\item dell'enunciato, in una delle forme dei libri e dei test:
    st["style"] è "bracket" (\\[ ... \\]), "inline" ($\\dfrac{d}{dx} ...$) o
    "display" ($\\displaystyle \\frac{d}{dx} ...$); con st["bare"] gli
    argomenti semplici sono senza parentesi. La funzione è sempre tra
    \\left( e \\right)."""
    order, style, bare = st["order"], st.get("style", "bracket"), st.get("bare", False)
    sign = r"\frac{d}{dx}" if order == 1 else r"\frac{d^{%s}}{dx^{%s}}" % (order, order)
    if style == "inline":
        sign = sign.replace(r"\frac", r"\dfrac", 1)
    formula = "%s %s" % (sign, paren(tex(st["f"], bare)[0]))
    if not st.get("request"):
        formula += " = " + tex(st["g"], bare)[0]
    math = r"\[ %s \]" % formula if style == "bracket" else r"$\displaystyle %s$" % formula if style == "display" else "$%s$" % formula
    if isinstance(order, int):
        return r"\item %s" % math
    return r"\item %s for every integer $%s \ge %d$." % (math, order, st["base"])


# =============================================================================
# 7. Le famiglie di enunciati veri.
# =============================================================================

def lin(a, b):
    """a x + b."""
    ax = X if a == 1 else neg(X) if a == -1 else mul(N(a), X)
    if b == 0:
        return ax
    return add(ax, N(b)) if b > 0 else sub(ax, N(-b))


def power_n(a, e):
    """a^e, con a razionale o un albero."""
    base = a if isinstance(a, tuple) else N(a)
    if base == N(1):
        return N(1)
    return pw(base, e)


def times(c, t):
    """c t, senza scrivere un fattore 1."""
    return t if c == N(1) or c == 1 else mul(c if isinstance(c, tuple) else N(c), t)


def nm(k):
    """n + k come albero."""
    return NV if k == 0 else add(NV, N(k)) if k > 0 else sub(NV, N(-k))


def falling(m):
    """n (n - 1) ... (n - m + 1)."""
    t = NV
    for j in range(1, m):
        t = mul(t, ("sub", NV, N(j)))
    return t


def family(rnd):
    """Una famiglia vera: (f, g(n), il più piccolo n0 per cui vale)."""
    A = [1, -1, 2, -2, 3, Fr(1, 2), Fr(-1, 2), Fr(3, 2)]
    a, b = rnd.choice(A), rnd.choice([0, 1, -1, 2, Fr(1, 3), 3])
    u = lin(a, b)
    kind = rnd.choice(["exp", "exp", "base", "sin", "cos", "recip", "recip", "rational", "half", "ln", "log",
                       "xexp", "x2exp", "xsin", "poly", "sincos", "param", "hard"])
    if kind == "exp":
        return exp_(u), times(power_n(a, NV), exp_(u)), 0
    if kind == "base":
        c = rnd.choice([2, 3, 5, Fr(1, 2)])
        cu = pw(N(c), u)
        return cu, times(power_n(a, NV), mul(pw(fn("ln", N(c)), NV), cu)), 0
    if kind in ("sin", "cos"):
        c = rnd.choice([1, 1, 2, -3])
        shifted = add(u, div(mul(NV, PIT), N(2)))
        return times(c, fn(kind, u)), times(c, times(power_n(a, NV), fn(kind, shifted))), 0
    if kind == "recip":
        m = rnd.choice([1, 1, 2, 3])
        b = rnd.choice([1, 2, 3, -2])
        u = lin(a, b)
        f = div(N(1), u) if m == 1 else div(N(1), pw(u, N(m)))
        top = mul(pw(N(-1), NV), fact(nm(m - 1)))
        if m > 2:
            top = div(top, N(math.factorial(m - 1)))
        top = mul(top, power_n(a, NV)) if a != 1 else top
        return f, div(top, pw(u, nm(m))), 0
    if kind == "rational":
        r = rnd.choice([Fr(1, 3), Fr(3, 2), Fr(-3, 2), Fr(2, 3), Fr(-1, 2)])
        a, b = rnd.choice([1, 2, -1]), rnd.choice([2, 3, 4])
        u = lin(a, b)
        coefficient = ("prod", "j", N(0), nm(-1), sub(N(r), ("var", "j")))
        return pw(u, N(r)), mul(times(power_n(a, NV), coefficient), pw(u, sub(N(r), NV))), 0
    if kind == "half":
        a, b = rnd.choice([1, 2, -1]), rnd.choice([1, 2, 3])
        u = lin(a, b)
        coefficient = div(mul(pw(N(-1), nm(-1)), fact(sub(mul(N(2), NV), N(2)))), mul(pw(N(2), sub(mul(N(2), NV), N(1))), fact(nm(-1))))
        return ("root", 2, u), mul(times(power_n(a, NV), coefficient), pw(u, sub(N(Fr(1, 2)), NV))), 1
    if kind in ("ln", "log"):
        a, b = rnd.choice([1, 2, -1, 3]), rnd.choice([1, 2, 3])
        u = lin(a, b)
        top = times(power_n(a, NV), mul(pw(N(-1), nm(-1)), fact(nm(-1))))
        if kind == "ln":
            return fn("ln", u), div(top, pw(u, NV)), 1
        c = rnd.choice([2, 3, 10])
        return ("log", N(c), u), div(top, mul(pw(u, NV), fn("ln", N(c)))), 1
    if kind == "xexp":
        a = rnd.choice([1, 2, -1, 3, Fr(1, 2)])
        return mul(X, exp_(lin(a, 0))), mul(add(times(power_n(a, NV), X), times(power_n(a, nm(-1)), NV)), exp_(lin(a, 0))), 0
    if kind == "x2exp":
        a = rnd.choice([1, 2, -1])
        inner = add(add(times(power_n(a, NV), pw(X, N(2))), mul(N(2), times(power_n(a, nm(-1)), mul(NV, X)))),
                    times(power_n(a, nm(-2)), mul(NV, sub(NV, N(1)))))
        return mul(pw(X, N(2)), exp_(lin(a, 0))), mul(inner, exp_(lin(a, 0))), 0
    if kind == "xsin":
        name = rnd.choice(["sin", "cos"])
        a = rnd.choice([1, 2, 3])
        g_n = lambda k: times(power_n(a, nm(k)), fn(name, add(lin(a, 0), div(mul(nm(k), PIT), N(2)))))
        return mul(X, fn(name, lin(a, 0))), add(mul(X, g_n(0)), mul(NV, g_n(-1))), 0
    if kind == "poly":
        m = rnd.choice([1, 2, 3])
        if rnd.random() < 0.3:
            return ("sum", "j", N(0), N(m), pw(X, ("var", "j"))), N(0), m + 1
        return add(pw(X, N(m)), N(rnd.choice([0, 5]))) if m > 1 else lin(rnd.choice([2, 3]), 1), N(0), m + 1
    if kind == "sincos":
        a = rnd.choice([1, 2])
        return mul(fn("sin", lin(a, 0)), fn("cos", lin(a, 0))), \
            mul(mul(pw(N(2), nm(-1)), power_n(a, NV)), fn("sin", add(lin(2 * a, 0), div(mul(NV, PIT), N(2))))), 0
    if kind == "param":
        A_ = ("var", "a")
        if rnd.random() < 0.5:
            return exp_(mul(A_, X)), mul(pw(A_, NV), exp_(mul(A_, X))), 0
        return fn("sin", mul(A_, X)), mul(pw(A_, NV), fn("sin", add(mul(A_, X), div(mul(NV, PIT), N(2))))), 0
    # Vera ma fuori dal metodo del programma: non deve mai diventare FALSO.
    return mul(exp_(X), fn("sin", X)), mul(mul(pw(N(2), div(NV, N(2))), exp_(X)), fn("sin", add(X, div(mul(NV, PIT), N(4))))), 0


def true_statement(rnd):
    """Un enunciato vero di ordine n: una famiglia, o la somma di due."""
    f, g, n0 = family(rnd)
    if rnd.random() < 0.3:
        f2, g2, m2 = family(rnd)
        f, g, n0 = add(f, f2), add(g, g2), max(n0, m2)
    return {"x": "x", "order": "n", "f": f, "g": g, "base": min(4, n0 + rnd.choice([0, 0, 0, 1, 2]))}


# Le derivate di ordine fisso: una funzione composta a caso e la sua derivata,
# calcolata qui con le regole scritte in un altro modo da quelle di main.c.

def has_x(t):
    return "x" in letters(t)


def simple_mul(a, b):
    """a b, senza scrivere i fattori 0 e 1."""
    if a == N(0) or b == N(0):
        return N(0)
    if a == N(1):
        return b
    if b == N(1):
        return a
    return mul(a, b)


def simple_add(a, b):
    """a + b, senza scrivere un addendo 0."""
    if a == N(0):
        return b
    if b == N(0):
        return a
    return add(a, b)


def sym_d(t):
    """La derivata di t rispetto a x, come albero."""
    if not has_x(t):
        return N(0)
    op = t[0]
    if op == "var":
        return N(1)
    if op == "add":
        return simple_add(sym_d(t[1]), sym_d(t[2]))
    if op == "sub":
        return sub(sym_d(t[1]), sym_d(t[2])) if has_x(t[1]) else neg(sym_d(t[2]))
    if op == "neg":
        return neg(sym_d(t[1]))
    if op == "mul":
        return simple_add(simple_mul(sym_d(t[1]), t[2]), simple_mul(t[1], sym_d(t[2])))
    if op == "div":
        return div(sub(simple_mul(sym_d(t[1]), t[2]), simple_mul(t[1], sym_d(t[2]))), pw(t[2], N(2)))
    u, du = (t[2], sym_d(t[2])) if op in ("fn", "log", "root") else (None, None)
    if op == "pow":
        base, e = t[1], t[2]
        if not has_x(e):
            return simple_mul(simple_mul(e, pw(base, sub(e, N(1)))), sym_d(base))
        if base == E:
            return simple_mul(t, sym_d(e))
        if not has_x(base):
            return simple_mul(simple_mul(t, fn("ln", base)), sym_d(e))
        # u^v = e^{v ln u}: la derivata di v ln u, per u^v.
        return mul(t, sym_d(mul(e, fn("ln", base))))
    if op == "fn":
        name = t[1]
        root = ("root", 2, sub(N(1), pw(u, N(2))))
        outer = {"sin": lambda: fn("cos", u), "cos": lambda: neg(fn("sin", u)),
                 "tan": lambda: add(N(1), pw(fn("tan", u), N(2))), "cot": lambda: neg(add(N(1), pw(fn("cot", u), N(2)))),
                 "ln": lambda: div(N(1), u), "arctan": lambda: div(N(1), add(N(1), pw(u, N(2)))),
                 "arccot": lambda: neg(div(N(1), add(N(1), pw(u, N(2))))),
                 "arcsin": lambda: div(N(1), root), "arccos": lambda: neg(div(N(1), root))}[name]()
        return simple_mul(outer, du)
    if op == "abs":
        # |u|' = u' |u| / u: il segno di u scritto al contrario di main.c.
        return simple_mul(sym_d(t[1]), div(t, t[1]))
    if op == "log":
        if has_x(t[1]):
            return sym_d(div(fn("ln", t[2]), fn("ln", t[1])))
        return simple_mul(div(N(1), mul(u, fn("ln", t[1]))), du)
    if op == "root":
        m = t[1]
        return simple_mul(div(N(1), mul(N(m), pw(t, N(m - 1)) if m > 2 else t)), du)
    raise Unsupported("derivative of %s" % op)


def composite(rnd, depth):
    """Una funzione di x composta a caso, che dove la si prova è di solito
    definita."""
    if depth == 0:
        return rnd.choice([X, X, lin(rnd.choice([2, -1, 3]), rnd.choice([1, 2])), lin(rnd.choice([2, 3]), 0), pw(X, N(2))])
    u = composite(rnd, depth - 1)
    kind = rnd.choice(["sin", "cos", "tan", "cot", "ln", "arctan", "sqrt", "cbrt", "exp", "base", "log", "pow", "inv",
                       "add", "mul", "div", "arcsin", "arccos", "arccot", "abs", "tower", "logx", "cbrtneg"])
    if kind in ("sin", "cos", "tan", "cot", "arctan", "arccot"):
        return fn(kind, u)
    if kind in ("arcsin", "arccos"):
        # u / (u^2 + 2) sta tra -1/(2 sqrt 2) e 1/(2 sqrt 2): sempre definito.
        return fn(kind, div(u, add(pw(u, N(2)), N(2))))
    if kind == "abs":
        return ("abs", u)
    if kind == "tower":
        # Una base positiva, x nella base e nell'esponente.
        return pw(add(pw(X, N(2)), N(1)), u)
    if kind == "logx":
        return ("log", add(pw(X, N(2)), N(2)), add(pw(u, N(2)), N(1)))
    if kind == "cbrtneg":
        # La radice cubica reale, anche di un numero negativo.
        return ("root", 3, sub(u, N(2)))
    if kind == "ln":
        return fn("ln", add(pw(u, N(2)), N(1)))
    if kind == "sqrt":
        return ("root", 2, add(pw(u, N(2)), N(2)))
    if kind == "cbrt":
        return ("root", 3, add(u, N(3)))
    if kind == "exp":
        return exp_(u)
    if kind == "base":
        return pw(N(rnd.choice([2, 3])), u)
    if kind == "log":
        return ("log", N(rnd.choice([2, 10])), add(pw(u, N(2)), N(1)))
    if kind == "pow":
        return pw(u, N(rnd.choice([2, 3, Fr(1, 2), -2])))
    if kind == "inv":
        return div(N(1), add(pw(u, N(2)), N(1)))
    v = composite(rnd, 0)
    if kind == "add":
        return add(u, v)
    if kind == "mul":
        return mul(u, v)
    return div(u, add(pw(v, N(2)), N(1)))


def fixed_statement(rnd):
    """Un enunciato di ordine fisso: da una famiglia con n = k, oppure una
    funzione composta e la sua derivata."""
    if rnd.random() < 0.4:
        f, g, n0 = family(rnd)
        k = rnd.randint(max(1, n0), 4)
        return {"x": "x", "order": k, "f": f, "g": subst(g, "n", N(k)), "base": k}
    if rnd.random() < 0.15:
        # La trappola del segno di Leibniz: se \frac{d}{dx} si leggesse come
        # la frazione delle lettere d e x, cioè 1/x, l'enunciato sarebbe
        # vera algebra; come derivata è falso (salvo d/dx x = 1).
        f = rnd.choice([N(rnd.choice([1, 5, -2])), X, pw(X, N(rnd.randint(2, 5))), mul(N(3), pw(X, N(2))), fn("sin", X), exp_(X)])
        k = rnd.choice([1, 1, 2])
        return {"x": "x", "order": k, "f": f, "g": over_x(f, N(k)), "base": k}
    if rnd.random() < 0.15:
        # Una somma con \sum: la derivata termine per termine, scritta con i
        # fattoriali, j! / (j - k)! x^{j - k}.
        J, m, k, c = ("var", "j"), rnd.randint(2, 5), rnd.randint(1, 3), rnd.choice([1, 2])
        body = mul(N(c), pw(X, J)) if c > 1 else pw(X, J)
        g_body = mul(times(c, div(fact(J), fact(sub(J, N(k))))), pw(X, sub(J, N(k))))
        return {"x": "x", "order": k, "f": ("sum", "j", N(0), N(m), body), "g": ("sum", "j", N(k), N(m), g_body), "base": k}
    while True:
        k = 1 if rnd.random() < 0.75 else 2
        f = composite(rnd, rnd.choice([1, 1, 2]) if k == 1 else 1)
        g = f
        for _ in range(k):
            g = sym_d(g)
        if len(tex(g)[0]) < 1500:
            return {"x": "x", "order": k, "f": f, "g": g, "base": k}


# =============================================================================
# 8. Le varianti false.
# =============================================================================

def numbers_in(t, path=()):
    """I percorsi dei numeri di t."""
    found = []
    if t[0] == "num":
        found.append(path)
    for i, part in enumerate(t[1:], 1):
        if isinstance(part, tuple):
            found += numbers_in(part, path + (i,))
    return found


def replace_at(t, path, value):
    if not path:
        return value
    i = path[0]
    return t[:i] + (replace_at(t[i], path[1:], value),) + t[i + 1:]


def get_at(t, path):
    for i in path:
        t = t[i]
    return t


def over_x(f, k):
    """f / x^k, con k un albero (un numero o n): ciò che \\frac{d^k}{dx^k} f
    vale come algebra, se d e x sono lettere. Per f = c x^m si scrive come
    nei libri, c x^{m - k} (x^{m - 1} per k = 1)."""
    c, m = (f[1], f[2]) if f[0] == "mul" and f[1][0] == "num" else (None, f)
    if m[0] == "pow" and m[1] == X and m[2][0] == "num":
        exponent = N(m[2][1] - k[1]) if k[0] == "num" else sub(m[2], k)
        power = N(1) if exponent == N(0) else X if exponent == N(1) else pw(X, exponent)
        return power if c is None else mul(c, power)
    return div(f, X if k == N(1) else pw(X, k))


def variant(st, rnd):
    """Una variante di st che di solito è falsa; l'oracolo decide se lo è."""
    st = dict(st)
    g = st["g"]
    induction = isinstance(st["order"], str)
    kinds = ["sign", "factor", "number", "plus", "algebra"] + (["shift", "shift", "hidden", "pi", "term"] if induction else ["term"])
    kind = rnd.choice(kinds)
    if kind == "algebra":
        st["g"] = over_x(st["f"], NV if induction else N(st["order"]))
    elif kind == "shift":
        st["g"] = subst(g, "n", nm(rnd.choice([1, -1])))
    elif kind == "sign":
        st["g"] = neg(g) if rnd.random() < 0.5 or not induction else mul(pw(N(-1), NV), g)
    elif kind == "factor":
        st["g"] = mul(N(rnd.choice([2, 3, Fr(1, 2)])), g) if rnd.random() < 0.6 or not induction else mul(NV, g)
    elif kind == "plus":
        st["g"] = add(g, N(1)) if rnd.random() < 0.5 else add(g, X)
    elif kind == "number":
        paths = numbers_in(g)
        if paths:
            path = rnd.choice(paths)
            st["g"] = replace_at(g, path, N(get_at(g, path)[1] + rnd.choice([1, -1, 2])))
    elif kind == "hidden":
        m = rnd.randint(1, 5)
        st["g"] = add(g, mul(N(rnd.choice([1, 2, -1])), falling(m)))
    elif kind == "pi":
        st["g"] = replace_pi(g)
    elif kind == "term":
        if g[0] in ("add", "sub"):
            st["g"] = g[1] if rnd.random() < 0.5 else g[2]
        else:
            st["g"] = add(g, mul(N(2), g))
    return st


def replace_pi(t):
    """n pi / 2 diventa n pi / 3."""
    if t[0] == "div" and t[2] == N(2) and t[1][0] == "mul" and t[1][2] == PIT:
        return div(t[1], N(3))
    return tuple(replace_pi(p) if isinstance(p, tuple) else p for p in t)


# =============================================================================
# 9. Il confronto con il programma.
# =============================================================================

def run_engine(exe, path):
    """Fa decidere al programma il file path: l'esito, le righe dei verdetti
    e le righe ATTENZIONE."""
    run = subprocess.run([exe, path], capture_output=True, text=True, timeout=3600)
    lines = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
    warning = [l for l in run.stdout.splitlines() if l.startswith("ATTENZIONE")]
    return run, lines, warning


def short(text):
    """Il verdetto senza il numero dell'\\item."""
    return text.split(". ", 1)[1] if ". " in text else text


def classify_request(st, line, label):
    """Come classify(), per una richiesta: la derivata scritta dal programma
    (S) si rilegge e si confronta con la derivata vera nei punti di prova.
    Una richiesta senza risposta (N, U) non è un errore."""
    text = short(line)
    if not text.startswith("RISOLTO"):
        verdict = "N" if text.startswith("NON DIMOSTRATO") else "U"
        return verdict, "request", "NOTE: request not answered\n   %s [%s]: %s" % (label, text[:90], st["text"][:600])
    answer = text.split(": ", 1)[1].rsplit(" (dove", 1)[0]
    reader = Reader(answer)
    written = reader.expr()
    if reader.peek()[0] != "end":
        raise Unsupported("text after the answer")
    results = judge(dict(st, g=written), [st["order"]])[st["order"]]
    problem = None
    if any(r[0] < 0 for r in results):
        bad = [r for r in results if r[0] < 0][0]
        problem = "BUG: the derivative written is wrong at x = %s" % bad[1]
    elif not any(r[0] > 0 for r in results):
        problem = "SUSPECT: the oracle cannot confirm the derivative written at any point"
    return "S", "request", (None if problem is None else "%s\n   %s [%s]: %s" % (problem, label, text[:200], st["text"][:600]))


def classify(st, line, label):
    """Il verdetto del programma (P, R, N o U), l'esito dell'oracolo, e una
    riga BUG, SUSPECT o NOTE, oppure None. st["text"] è l'\\item scritto."""
    if st.get("request"):
        return classify_request(st, line, label)
    text = short(line)
    verdict = "P" if text.startswith("DIMOSTRATO") else "R" if text.startswith("FALSO") else "N" if text.startswith("NON DIMOSTRATO") else "U"
    truth, where = verdict_of(st)
    problem = None
    if verdict == "P" and truth == "false":
        problem = "BUG: proved, but the oracle finds it false for n = %s" % where
    elif verdict == "P" and truth == "unsure":
        problem = "SUSPECT: proved, and the oracle cannot decide order %s" % where
    elif verdict == "R":
        found = re.search(r"per n = (-?\d+)", text)
        point = re.search(r"x = (-?\d+)/(\d+)", text)
        k = int(found.group(1)) if found else st["order"]
        at = Fr(int(point.group(1)), int(point.group(2))) if point and not found else None
        holds = witness_holds(st, k, at)
        if truth == "true" or holds == 0:
            problem = "BUG: refuted for order %s%s, but the oracle finds the formula right there" % (k, "" if at is None else " at x = %s" % at)
        elif holds is None:
            problem = "SUSPECT: refuted for order %s, and the oracle cannot confirm it" % k
    elif verdict in "NU" and truth == "true":
        problem = "NOTE: true, not proved"
    elif verdict in "NU" and truth == "unsure":
        problem = "NOTE: not proved, and the oracle cannot decide order %s" % where
    return verdict, truth, (None if problem is None else "%s\n   %s [%s]: %s" % (problem, label, text[:90], st["text"][:600]))


def fuzz(exe, seed, count, work):
    """La modalità fuzz: count enunciati dal seme seed."""
    rnd = random.Random(seed)
    cases = []
    while len(cases) < count:
        st = true_statement(rnd) if rnd.random() < 0.6 else fixed_statement(rnd)
        # Una richiesta, senza il risultato: si controlla la risposta.
        if isinstance(st["order"], int) and rnd.random() < 0.35:
            st["request"] = True
        elif rnd.random() < 0.55:
            st = variant(st, rnd)
        # La forma scritta: le tre dei test, e gli argomenti senza parentesi.
        style, bare = rnd.choice(["bracket", "inline", "display"]), rnd.random() < 0.4
        st["style"], st["bare"] = ("bracket", False) if SIMPLE else (style, bare)
        text = statement_tex(st)
        # Il lettore di questo file deve rileggere ciò che è stato scritto:
        # l'oracolo lavora sull'albero riletto, non su quello costruito.
        try:
            back = read_statement(text[len(r"\item"):])
        except Unsupported:
            back = None
        if back is None or len(text) > 3500:
            continue
        back["text"] = text
        cases.append(back)
    path = os.path.join(work, "fuzz_derivatives_%d.tex" % seed)
    with open(path, "w", encoding="utf-8") as out:
        out.write("\\begin{enumerate}\n")
        for st in cases:
            out.write(st["text"] + "\n")
        out.write("\\end{enumerate}\n")
    run, lines, warning = run_engine(exe, path)
    if len(lines) != len(cases) or run.returncode not in (0, 1):
        print("BUG: the program answered %d of %d statements (exit %d)\n%s" % (len(lines), len(cases), run.returncode, run.stderr[-2000:]))
        return
    for w in warning:
        print("BUG: " + w)
    tally = {}
    for index, (st, line) in enumerate(zip(cases, lines), 1):
        try:
            verdict, truth, problem = classify(st, line, str(index))
        except Unsupported as why:
            print("SUSPECT: the oracle cannot compute statement %d (%s): %s" % (index, why, st["text"][:300]))
            continue
        tally[(truth, verdict)] = tally.get((truth, verdict), 0) + 1
        if problem and (NOTES or not problem.startswith("NOTE")):
            print(problem)
    notes = sum(v for (t, verdict), v in tally.items() if t == "true" and verdict in "NU")
    print("seed %d: %s; %d true statements not proved" % (seed, ", ".join("%s/%s=%d" % (k[0], k[1], v) for k, v in sorted(tally.items())), notes))



def check(exe, paths):
    """La modalità check: gli enunciati sulle derivate dei file paths."""
    for path in paths:
        items = read_items(path)
        run, lines, warning = run_engine(exe, path)
        false_file = os.path.basename(path).startswith("false_")
        print("%s: %d items" % (path, len(items)))
        for w in warning:
            print("BUG: " + w)
        if len(lines) != len(items):
            print("BUG: the program answered %d of %d items" % (len(lines), len(items)))
            continue
        tally = {}
        for (number, item), line in zip(items, lines):
            try:
                st = read_statement(item)
                if st is None and "\\mathbb" in item and "frac{d" in item:
                    key = check_study(item, line, "%s:%d" % (os.path.basename(path), number), false_file)
                    tally[key] = tally.get(key, 0) + 1
                    continue
            except Unsupported as why:
                tally["unread"] = tally.get("unread", 0) + 1
                print("  %d: the oracle cannot read it (%s) [%s]" % (number, why, short(line)[:60]))
                continue
            if st is None:
                continue
            st["text"] = r"\item " + " ".join(item.split())
            try:
                verdict, truth, problem = classify(st, line, "%s:%d" % (os.path.basename(path), number))
            except Unsupported as why:
                print("  %d: the oracle cannot compute it (%s) [%s]" % (number, why, short(line)[:60]))
                continue
            tally[(truth, verdict)] = tally.get((truth, verdict), 0) + 1
            if problem:
                print(problem)
            if truth == "true" and false_file or truth == "false" and not false_file:
                print("DATA: item %d of %s is %s for the oracle" % (number, path, truth))
        print("  " + ", ".join("%s=%d" % ("/".join(k) if isinstance(k, tuple) else k, v) for k, v in sorted(tally.items(), key=str)))


# =============================================================================
# 10. Le condizioni: studi di funzione e insiemi di soluzioni.
#
#   ("cmp", [e0, e1, ...], [r1, ...])  una catena e0 r1 e1 r2 e2 ..., con le
#                                      relazioni tra "<", "le", "=", "ne"
#                                      (> e \ge si leggono scambiando i membri)
#   ("and", a, b) ("or", a, b) ("not", a) ("iff", a, b) ("implies", a, b)
#   ("forall" | "exists", lettera, insieme, corpo), con insieme
#        ("R", punti tolti) oppure ("Z",)
#   ("const", vero o falso)
#
# Un confronto vale in un punto solo se i suoi membri vi hanno un valore, come
# nella tabella dei segni; la derivata ha un valore solo dove la funzione è
# derivabile, cioè dove la sua serie di Taylor si calcola.
# =============================================================================

RELATIONS = {"<": ("<", False), ">": ("<", True), "le": ("le", False), "leq": ("le", False), "ge": ("le", True),
             "geq": ("le", True), "=": ("=", False), "neq": ("ne", False), "ne": ("ne", False)}


class LogicReader(Reader):
    """Il lettore delle condizioni, sopra quello delle espressioni."""

    def relation(self):
        kind, value = self.peek()
        if (kind == "chr" and value in "<>=") or (kind == "cmd" and value in RELATIONS):
            self.take()
            return RELATIONS[value]
        return None

    def numbers(self):
        """\\mathbb{R} [\\setminus \\{a, b\\}] oppure \\mathbb{Z}."""
        if self.take() != ("cmd", "mathbb"):
            raise Unsupported("set")
        self.expect("{")
        kind, name = self.take()
        self.expect("}")
        holes = []
        if self.peek() == ("cmd", "setminus"):
            self.take()
            self.expect("{")
            holes.append(self.expr())
            while self.is_chr(","):
                self.take()
                holes.append(self.expr())
            self.expect("}")
        return ("Z",) if name == "Z" else ("R", holes)

    def formula(self):
        left = self.implication()
        while self.peek() == ("cmd", "iff"):
            self.take()
            left = ("iff", left, self.implication())
        return left

    def implication(self):
        left = self.disjunction()
        if self.peek() == ("cmd", "implies"):
            self.take()
            return ("implies", left, self.implication())
        return left

    def disjunction(self):
        left = self.conjunction()
        while self.peek() == ("cmd", "lor"):
            self.take()
            left = ("or", left, self.conjunction())
        return left

    def conjunction(self):
        left = self.unary()
        while self.peek() == ("cmd", "land"):
            self.take()
            left = ("and", left, self.unary())
        return left

    def unary(self):
        kind, value = self.peek()
        if (kind, value) == ("cmd", "lnot"):
            self.take()
            return ("not", self.unary())
        if kind == "cmd" and value in ("forall", "exists", "nexists"):
            self.take()
            _, letter = self.take()
            if self.take() != ("cmd", "in"):
                raise Unsupported("quantifier without \\in")
            numbers = self.numbers()
            if self.peek()[0] == "end" or self.is_chr(")"):
                # \nexists x \in R, \forall x \in R: le risposte "nessun x" e
                # "ogni x" del programma.
                return ("const", value == "forall")
            body = self.unary()
            node = ("exists" if value != "forall" else "forall", letter, numbers, body)
            return ("not", node) if value == "nexists" else node
        if self.is_chr("("):
            # Una parentesi apre un confronto, come (x - 1)^{2} > 0, oppure una
            # condizione: si prova la prima lettura, e se non riesce la seconda.
            start = self.i
            try:
                return self.chain()
            except Unsupported:
                self.i, self.bars = start, 0
            self.take()
            inside = self.formula()
            self.expect(")")
            return inside
        return self.chain()

    def chain(self):
        sides, relations = [self.expr()], []
        while True:
            relation = self.relation()
            if relation is None:
                break
            relations.append(relation)
            sides.append(self.expr())
        if not relations:
            raise Unsupported("a condition without a relation")
        return ("cmp", sides, relations)


def read_condition(text):
    reader = LogicReader(text)
    try:
        formula = reader.formula()
    except IndexError:
        raise Unsupported("the condition ends too early")
    if reader.peek()[0] != "end":
        raise Unsupported("text after the condition: %r" % (reader.peek(),))
    return formula


def check_study(item, line, label, false_file):
    """La modalità check per un enunciato con quantificatori sulle derivate:
    la sua verità per l'oracolo, contro il verdetto e il nome del file. Ritorna
    (esito dell'oracolo, verdetto), dopo aver stampato i problemi."""
    pieces = re.findall(r"\\\[(.*?)\\\]|\$(.*?)\$", item, re.S)
    math_text = next((a or b for a, b in pieces), "")
    truth, where = statement_truth(read_condition(math_text))
    text = short(line)
    verdict = "P" if text.startswith("DIMOSTRATO") else "R" if text.startswith("FALSO") else "N" if text.startswith("NON DIMOSTRATO") else "U"
    if verdict == "P" and truth != "true":
        print("BUG: proved, but the oracle finds it %s at x = %s\n   %s: %s" % (truth, str(where)[:24], label, math_text[:300]))
    elif verdict == "R" and truth == "true":
        print("BUG: refuted, but the oracle finds it true\n   %s: %s" % (label, math_text[:300]))
    if truth == "true" and false_file or truth == "false" and not false_file:
        print("DATA: %s is %s for the oracle" % (label, truth))
    if truth == "unsure":
        print("SUSPECT: the oracle cannot decide %s at x = %s" % (label, str(where)[:24]))
    return truth, verdict


def value_at(t, x0, env):
    """Il valore di t con x = x0 (un Decimal) e le altre lettere in env; None
    se lì non ne ha uno."""
    try:
        return taylor(t, "x", x0, 0, env)[0]
    except Undefined:
        return None


def side_of(left, right, x0, env):
    """Il segno di left - right in x0, due alberi: 1, 0, -1; None se non si
    sa; "undefined" se un membro lì non ha un valore.

    Con 80 cifre una differenza sotto 10^-40 può essere uno zero vero, che
    gli arrotondamenti lasciano a 10^-79, oppure un numero piccolo ma non
    nullo, come e^{-225}. Le due cose si separano rifacendo il conto con
    FINE cifre: lo zero scende a 10^-119, il numero resta com'è. Con FINE
    cifre una differenza sotto 10^-100 è zero."""
    a, b = value_at(left, x0, env), value_at(right, x0, env)
    if a is None or b is None:
        return "undefined"
    gap, scale = a - b, 1 + abs(a) + abs(b)
    if abs(gap) > DIFFERENT * scale:
        return 1 if gap > 0 else -1
    if getcontext().prec >= FINE:
        return 0 if abs(gap) <= D(10) ** -100 * scale else 1 if gap > 0 else -1
    if abs(gap) > SAME * scale:
        return None
    previous = getcontext().prec
    getcontext().prec = FINE
    try:
        return side_of(left, right, x0, env)
    finally:
        getcontext().prec = previous


def cmp_holds(side, relation):
    """True o False per una relation tra due membri con il segno side."""
    return {"<": side < 0, "le": side <= 0, "=": side == 0, "ne": side != 0}[relation]


def holds(f, x0, env):
    """La verità di f con x = x0: True, False, oppure None se l'oracolo non
    sa decidere un confronto che conta."""
    op = f[0]
    if op == "const":
        return f[1]
    if op == "cmp":
        result = True
        for i, (relation, swapped) in enumerate(f[2]):
            left, right = (f[1][i + 1], f[1][i]) if swapped else (f[1][i], f[1][i + 1])
            try:
                side = side_of(left, right, x0, env)
            except (ArithmeticError, Unsure):
                # Un numero troppo grande per i decimali, o una derivata che
                # non si sa se esiste: non si sa.
                side = None
            if side == "undefined":
                return False
            if side is None:
                result = None
            elif not cmp_holds(side, relation):
                return False
        return result
    if op == "not":
        r = holds(f[1], x0, env)
        return None if r is None else not r
    if op in ("and", "or", "iff", "implies"):
        a, b = holds(f[1], x0, env), holds(f[2], x0, env)
        if op == "implies":
            a = None if a is None else not a
            op = "or"
        if op == "and":
            return False if a is False or b is False else None if a is None or b is None else True
        if op == "or":
            return True if a is True or b is True else None if a is None or b is None else False
        return None if a is None or b is None else a == b
    if op in ("exists", "forall") and f[2] == ("Z",):
        # Gli interi k che contano per x0 tra -12 e 12: la griglia non va oltre.
        seen = []
        for k in range(-80, 81):
            seen.append(holds(f[3], x0, dict(env, **{f[1]: Fr(k)})))
        if op == "exists":
            return True if True in seen else None if None in seen else False
        return False if False in seen else None if None in seen else True
    raise Unsupported("condition %s" % op)


GRID = [Fr(i, 25) + Fr(1, 997) for i in range(-300, 301)]
FINE = 120  # cifre con cui si cercano e si provano i punti speciali


def watched(t, found):
    """Mette in found le sotto-espressioni di t i cui zeri sono i punti in
    cui la verità di un confronto può cambiare senza che la griglia lo veda:
    le derivate stesse, i denominatori, gli argomenti dei logaritmi, i
    radicandi, gli argomenti dei valori assoluti, il coseno sotto una
    tangente, 1 - u^2 sotto un arcoseno, la base di una potenza con x
    all'esponente."""
    if not isinstance(t, tuple) or "x" not in letters(t):
        return
    op = t[0]
    if op == "sub" and t[2] != N(0) and any(part[0] == "deriv" for part in t[1:] if isinstance(part, tuple)):
        # La differenza tra i membri di un confronto con una derivata.
        found.append(t)
    if op == "deriv":
        found.append(t)
        watched(t[3], found)
        return
    if op == "div":
        found.append(t[2])
    elif op == "fn" and t[1] in ("ln",):
        found.append(t[2])
    elif op == "fn" and t[1] in ("tan", "cot"):
        found.append(fn("cos" if t[1] == "tan" else "sin", t[2]))
    elif op == "fn" and t[1] in ("arcsin", "arccos"):
        found.append(sub(N(1), pw(t[2], N(2))))
    elif op in ("log", "root"):
        found.append(t[2])
    elif op == "abs":
        found.append(t[1])
    elif op == "pow" and "x" in letters(t[2]):
        found.append(t[1])
    for part in t[1:]:
        watched(part, found)


def nearby_rational(x0, s):
    """Un razionale semplice vicino a x0 (denominatore fino a 64) in cui s è
    zero o non ha un valore, se c'è: i punti esatti delle tabelle sono di
    solito così."""
    for q in range(1, 65):
        p = (x0 * q).to_integral_value()
        if abs(p / q - x0) > D(10) ** -20:
            continue
        candidate = D(int(p)) / q
        v = probe(s, candidate)
        if v is None or v == 0:
            return candidate
    return None


def probe(s, p):
    """Il valore di s in p per la ricerca dei punti speciali: None anche dove
    non si sa se c'è, che per la ricerca è un bordo come un altro."""
    try:
        return value_at(s, p, {})
    except (ArithmeticError, Unsure):
        return None


def zeros_of(s):
    """I punti tra -12 e 12 in cui s si annulla, cambia segno, sfiora lo zero
    o smette di avere un valore: trovati sulla griglia e stretti per
    bisezione, o per ricerca del minimo di |s|, con FINE cifre."""
    points = []
    grid = [d_of(p) for p in GRID]
    values = [probe(s, p) for p in grid]
    previous = getcontext().prec
    getcontext().prec = FINE
    try:
        for i in range(len(grid) - 1):
            a, b, va, vb = grid[i], grid[i + 1], values[i], values[i + 1]
            if (va is None) != (vb is None) or (va is not None and va * vb < 0):
                # Un cambio di segno, o il bordo del dominio: bisezione.
                inside_a = va is not None
                sign_a = va > 0 if inside_a else None
                for _ in range(420):
                    m = (a + b) / 2
                    vm = probe(s, m)
                    if (vm is not None) == inside_a and (vm is None or (vm > 0) == sign_a):
                        a = m
                    else:
                        b = m
                # Sul bordo del dominio si prende l'estremo dalla parte in cui
                # non c'è un valore: con le cifre finite il punto trovato è
                # già "uguale" al bordo scritto, e deve dire come il bordo.
                if (va is None) != (vb is None):
                    points.append(b if inside_a else a)
                else:
                    points.append((a + b) / 2)
            elif 0 < i and va is not None and values[i - 1] is not None and vb is not None and abs(va) <= abs(values[i - 1]) and abs(va) <= abs(vb) \
                    and values[i - 1] * va > 0 and va * vb > 0:
                # Un minimo di |s| tra due punti dello stesso segno: forse uno
                # zero che non cambia segno. Uno che cambia segno lo trova la
                # bisezione, con più cifre: un punto stretto solo fino a
                # 10^-44, accanto a un polo, avrebbe ancora un valore, ma
                # sarebbe già "uguale" all'estremo scritto.
                low, high = grid[i - 1], b
                for _ in range(300):
                    m1, m2 = low + (high - low) / 3, high - (high - low) / 3
                    v1, v2 = probe(s, m1), probe(s, m2)
                    if v1 is None or v2 is None:
                        break
                    if abs(v1) < abs(v2):
                        high = m2
                    else:
                        low = m1
                # Solo se è davvero uno zero: un minimo che non lo è, come
                # quello di 1 / sqrt(1 - x^2) in 0, non ha niente di speciale.
                bottom = probe(s, (low + high) / 2)
                if bottom is not None and abs(bottom) < D(10) ** -50:
                    points.append((low + high) / 2)
        # Un punto che è un razionale semplice si prende esatto, con tutte
        # le cifre: 2/3 con 80 cifre non è 2/3 per un conto con 120.
        points = [r if r is not None else p for p, r in ((p, nearby_rational(p, s)) for p in points)]
    except ArithmeticError:
        pass
    finally:
        getcontext().prec = previous
    return points


def constants_in(f):
    """I membri senza lettere dei confronti di f: gli estremi scritti, come
    alberi, perché si calcolino con le cifre che servono."""
    found = []
    if f[0] == "cmp":
        for side in f[1]:
            if not letters(side):
                found.append(side)
    elif f[0] in ("and", "or", "iff", "implies"):
        found = constants_in(f[1]) + constants_in(f[2])
    elif f[0] == "not":
        found = constants_in(f[1])
    elif f[0] in ("forall", "exists"):
        found = constants_in(f[3])
    return found


def sides_in(f, found):
    """Mette in found i membri dei confronti di f, e le differenze tra due
    membri vicini di una catena: dove si annulla una differenza come
    f'(x) - 1, la verità del confronto cambia."""
    if f[0] == "cmp":
        found += f[1]
        found += [sub(a, b) for a, b in zip(f[1], f[1][1:]) if "x" in letters(a) or "x" in letters(b)]
    elif f[0] in ("and", "or", "iff", "implies"):
        sides_in(f[1], found)
        sides_in(f[2], found)
    elif f[0] == "not":
        sides_in(f[1], found)
    elif f[0] in ("forall", "exists"):
        sides_in(f[3], found)


def truth_on_grid(body, extra=()):
    """La verità di "per ogni x reale, body": ("true", None), ("false", x0)
    con un punto in cui body non vale, oppure ("unsure", x0). Si guarda la
    griglia; gli estremi scritti e due punti vicinissimi a ognuno di loro; e
    i punti speciali delle espressioni (zeros_of()): dove una derivata si
    annulla senza cambiare segno, o una funzione non ha un valore, la griglia
    non passa, ma la verità di un confronto può cambiare proprio lì."""
    points, special = [d_of(p) for p in GRID], []
    previous = getcontext().prec
    for tree in constants_in(body):
        c = value_at(tree, D(0), {})
        if c is None:
            continue
        points += [c - D(10) ** -6, c + D(10) ** -6, c - D(10) ** -3, c + D(10) ** -3]
        # L'estremo stesso con tutte le cifre: può essere anche un polo, o
        # uno zero della derivata, e lì le due cose vanno viste insieme.
        getcontext().prec = FINE
        fine = value_at(tree, D(0), {})
        getcontext().prec = previous
        if fine is not None:
            special.append(fine)
    for c in extra:
        points += [c, c - D(10) ** -6, c + D(10) ** -6]
    sides, subtrees = [], []
    sides_in(body, sides)
    for side in sides:
        watched(side, subtrees)
    seen = set()
    for s in subtrees:
        if repr(s) not in seen:
            seen.add(repr(s))
            special += zeros_of(s)
    unsure = None
    for fine, group in ((False, points), (True, special)):
        previous = getcontext().prec
        if fine:
            getcontext().prec = FINE
        try:
            for x0 in group:
                r = holds(body, x0, {})
                if r is False:
                    return "false", x0
                if r is None and unsure is None:
                    unsure = x0
        finally:
            getcontext().prec = previous
    return ("unsure", unsure) if unsure is not None else ("true", None)


def statement_truth(f):
    """La verità di un enunciato \\forall x \\in R (...) (anche con punti
    tolti), o di \\lnot di uno così."""
    if f[0] == "not":
        truth, where = statement_truth(f[1])
        return {"true": "false", "false": "true"}.get(truth, truth), where
    if f[0] != "forall" or f[1] != "x" or f[2][0] != "R":
        raise Unsupported("not a statement for every real x")
    body = f[3]
    for hole in f[2][1]:
        body = ("implies", ("cmp", [X, hole], [("ne", False)]), body)
    return truth_on_grid(body)


# =============================================================================
# 11. Gli studi di funzione.
# =============================================================================

def study_function(rnd):
    """Una funzione di una famiglia che la tabella dei segni tratta."""
    c = lambda: rnd.choice([1, 2, 3, -1, -2, Fr(1, 2)])
    r = lambda: rnd.choice([-3, -2, -1, 0, 1, 2, 3, Fr(1, 2), Fr(-3, 2)])
    kind = rnd.choice(["roots", "poly", "rational", "rational2", "exp", "exp2", "log", "xlog", "sqrt", "atan", "trig", "trig2", "cbrt"])
    if kind == "roots":
        f = N(c())
        for _ in range(rnd.choice([2, 3, 3, 4])):
            f = mul(f, pw(lin(1, -r()), N(rnd.choice([1, 1, 2]))))
        return f
    if kind == "poly":
        return add(add(pw(X, N(rnd.choice([3, 4]))), mul(N(rnd.randint(-4, 4)), pw(X, N(2)))), lin(rnd.randint(-6, 6), rnd.randint(-3, 3)))
    if kind == "rational":
        return div(add(pw(X, N(2)), lin(rnd.randint(-3, 3), rnd.randint(-4, 4))), lin(1, rnd.randint(-3, 3)))
    if kind == "rational2":
        return div(lin(1, rnd.randint(-3, 3)), add(pw(X, N(2)), N(rnd.randint(1, 4))))
    if kind == "exp":
        return mul(pw(X, N(rnd.choice([1, 2, 3]))), exp_(lin(rnd.choice([1, -1, 2, -2]), 0)))
    if kind == "exp2":
        return mul(add(pw(X, N(2)), lin(rnd.randint(-3, 3), rnd.randint(-3, 3))), exp_(lin(rnd.choice([1, -1]), 0)))
    if kind == "log":
        return fn("ln", add(pw(X, N(2)), lin(rnd.randint(-3, 3), rnd.randint(-4, 3))))
    if kind == "xlog":
        return mul(pw(X, N(rnd.choice([1, 2]))), fn("ln", X)) if rnd.random() < 0.6 else pw(fn("ln", X), N(2))
    if kind == "sqrt":
        return rnd.choice([mul(lin(1, rnd.randint(-2, 2)), ("root", 2, lin(1, rnd.randint(-2, 3)))),
                           div(X, ("root", 2, add(pw(X, N(2)), N(1)))), mul(X, ("root", 2, lin(-1, rnd.randint(1, 4))))])
    if kind == "atan":
        return fn("arctan", add(pw(X, N(2)), lin(rnd.randint(-3, 3), 0)))
    if kind == "trig":
        return rnd.choice([fn("sin", lin(rnd.choice([1, 2, 3]), 0)), add(fn("sin", X), fn("cos", X)), mul(fn("sin", X), fn("cos", X)),
                           pw(fn("sin", X), N(2))])
    if kind == "trig2":
        return add(fn("cos", lin(2, 0)), mul(N(rnd.choice([1, 2, -2])), fn("sin", X)))
    return ("root", 3, lin(1, rnd.randint(-2, 2)))


RELATION_TEX = {">": ">", "<": "<", "ge": r"\ge", "le": r"\le", "=": "="}


def study_request(rnd):
    """Una richiesta \\{x \\in R \\mid f^{(k)} rel 0\\}: (testo, condizione)."""
    f = study_function(rnd)
    k = 1 if rnd.random() < 0.7 else 2
    relation = rnd.choice([">", ">", "<", "ge", "le", "="])
    sign = r"\frac{d}{dx}" if k == 1 else r"\frac{d^{2}}{dx^{2}}"
    condition = r"%s\left( %s \right) %s 0" % (sign, tex(f)[0], RELATION_TEX[relation])
    return condition


def answer_of(line):
    """Le soluzioni scritte in una riga RISOLTO: (testo, periodiche), oppure
    None se la riga non è una risposta o usa nomi di radici."""
    text = short(line)
    if not text.startswith("RISOLTO"):
        return None
    answer = text.split(": ", 1)[1]
    periodic = answer.endswith(" (k intero)")
    answer = re.sub(r" \((k intero|negli estremi[^)]*|radici[^)]*)\)$", "", answer)
    if "r_{" in answer:
        return None
    return answer, periodic


def wrapped(answer, periodic):
    """Le soluzioni come condizione da rileggere: con k, sotto \\exists k."""
    return r"\exists k \in \mathbb{Z}\ \left( %s \right)" % answer if periodic else answer


def mutated(answer, rnd):
    """Le soluzioni con un dettaglio cambiato: un estremo compreso o escluso,
    un numero spostato, un pezzo tolto."""
    choice = rnd.choice(["closed", "number", "piece"])
    if choice == "closed":
        swaps = [(m.start(), m.group()) for m in re.finditer(r" (<|>|\\le|\\ge) ", answer)]
        if swaps:
            at, old = rnd.choice(swaps)
            new = {" < ": r" \le ", " > ": r" \ge ", r" \le ": " < ", r" \ge ": " > "}[old]
            return answer[:at] + new + answer[at + len(old):]
    if choice == "number":
        numbers = list(re.finditer(r"(?<![{\w^])-?\d+(?![}\w])", answer))
        if numbers:
            m = rnd.choice(numbers)
            return answer[:m.start()] + str(int(m.group()) + rnd.choice([1, -1])) + answer[m.end():]
    pieces = answer.split(r" \lor ")
    if len(pieces) > 1:
        pieces.pop(rnd.randrange(len(pieces)))
        return r" \lor ".join(pieces)
    return None


def study_problem(kind, line, condition, answer_text, label):
    """Controlla un verdetto dello studio: una riga BUG o SUSPECT, o None."""
    return "%s\n   %s [%s]: %s ; %s" % (kind, label, short(line)[:160], condition[:300], answer_text[:300])


def studies(exe, seed, count, work):
    """La modalità studies: count richieste dal seme seed, poi gli enunciati."""
    rnd = random.Random(seed)
    conditions = []
    while len(conditions) < count:
        condition = study_request(rnd)
        try:
            read_condition(condition)
        except Unsupported:
            continue
        conditions.append(condition)
    path = os.path.join(work, "fuzz_studies_%d.tex" % seed)
    with open(path, "w", encoding="utf-8") as out:
        out.write("\\begin{enumerate}\n")
        for condition in conditions:
            out.write("\\item $\\displaystyle \\{x \\in \\mathbb{R} \\mid %s\\}$\n" % condition)
        out.write("\\end{enumerate}\n")
    run, lines, warning = run_engine(exe, path)
    if len(lines) != len(conditions):
        print("BUG: the program answered %d of %d requests (exit %d)" % (len(lines), len(conditions), run.returncode))
        return
    for w in warning:
        print("BUG: " + w)

    # Fase 1: le soluzioni scritte, contro il segno vero della derivata.
    tally, confirmed = {}, []
    for index, (condition, line) in enumerate(zip(conditions, lines), 1):
        found = answer_of(line)
        if found is None:
            tally["unanswered"] = tally.get("unanswered", 0) + 1
            continue
        answer, periodic = found
        try:
            formula = read_condition("(%s) \\iff (%s)" % (condition, wrapped(answer, periodic)))
            truth, where = truth_on_grid(formula)
        except Unsupported as why:
            print("SUSPECT: the oracle cannot read the answer of request %d (%s): %s" % (index, why, answer[:200]))
            continue
        tally["answer " + truth] = tally.get("answer " + truth, 0) + 1
        if truth == "false":
            print(study_problem("BUG: the solutions written are wrong at x = %s" % str(where)[:24], line, condition, answer, "request %d" % index))
        elif truth == "unsure":
            print(study_problem("SUSPECT: the oracle cannot decide the solutions at x = %s" % str(where)[:24], line, condition, answer, "request %d" % index))
        else:
            confirmed.append((condition, answer, periodic))

    # Fase 2: le soluzioni confermate come enunciati, e le loro varianti.
    cases = []
    for condition, answer, periodic in confirmed:
        cases.append((condition, answer, periodic))
        other = mutated(answer, rnd)
        if other and other != answer:
            cases.append((condition, other, periodic))
    statements = [r"\forall x \in \mathbb{R}\ \left( \left( %s \right) \iff \left( %s \right) \right)" % (c, wrapped(a, p)) for c, a, p in cases]
    path = os.path.join(work, "fuzz_studies_%d_statements.tex" % seed)
    with open(path, "w", encoding="utf-8") as out:
        out.write("\\begin{enumerate}\n")
        for s in statements:
            out.write("\\item $\\displaystyle %s$\n" % s)
        out.write("\\end{enumerate}\n")
    run, lines, warning = run_engine(exe, path)
    if len(lines) != len(statements):
        print("BUG: the program answered %d of %d statements (exit %d)" % (len(lines), len(statements), run.returncode))
        return
    for w in warning:
        print("BUG: " + w)
    for index, (statement, line) in enumerate(zip(statements, lines), 1):
        text = short(line)
        verdict = "P" if text.startswith("DIMOSTRATO") else "R" if text.startswith("FALSO") else "N"
        try:
            truth, where = statement_truth(read_condition(statement))
        except Unsupported as why:
            print("SUSPECT: the oracle cannot read statement %d (%s)" % (index, why))
            continue
        tally["%s/%s" % (truth, verdict)] = tally.get("%s/%s" % (truth, verdict), 0) + 1
        if verdict == "P" and truth != "true":
            print(study_problem("BUG: proved, but the oracle finds it %s at x = %s" % (truth, str(where)[:24]), line, statement, "", "statement %d" % index))
        elif verdict == "R" and truth == "true":
            print(study_problem("BUG: refuted, but the oracle finds it true", line, statement, "", "statement %d" % index))
        elif verdict == "R":
            # Il controesempio del programma: lì i due membri devono differire.
            # Si calcola con FINE cifre: è spesso una radice, come 2 + sqrt 3,
            # dove la derivata con 80 cifre non è ancora zero.
            point = re.search(r"controesempio x = (.*?)( \(.*\))?$", text)
            body = read_condition(statement)[3]
            previous = getcontext().prec
            getcontext().prec = FINE
            try:
                at = value_at(Reader(point.group(1)).expr(), D(0), {}) if point else None
                seen = holds(body, at, {}) if at is not None else None
            except Unsupported:
                seen = None
            finally:
                getcontext().prec = previous
            if seen is True:
                print(study_problem("BUG: refuted at x = %s, but the oracle finds the statement true there" % point.group(1), line, statement, "", "statement %d" % index))
            elif seen is None and truth != "false":
                print(study_problem("SUSPECT: refuted, and the oracle cannot confirm the counterexample", line, statement, "", "statement %d" % index))
    print("seed %d: %s" % (seed, ", ".join("%s=%d" % (k, v) for k, v in sorted(tally.items()))))


def main():
    if len(sys.argv) >= 6 and sys.argv[1] == "fuzz":
        fuzz(sys.argv[2], int(sys.argv[3]), int(sys.argv[4]), sys.argv[5])
    elif len(sys.argv) >= 6 and sys.argv[1] == "studies":
        studies(sys.argv[2], int(sys.argv[3]), int(sys.argv[4]), sys.argv[5])
    elif len(sys.argv) >= 4 and sys.argv[1] == "check":
        check(sys.argv[2], sys.argv[3:])
    else:
        print(__doc__)
        sys.exit(2)


main()
