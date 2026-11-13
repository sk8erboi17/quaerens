"""Fuzzing differenziale degli integrali (capitolo 30 di main.c, 9h di parser.c).

Uso:

  python3 -I fuzz_integrals.py fuzz <eseguibile> <seme> <quanti> <cartella di lavoro>
      Costruisce a caso delle primitive F da famiglie note (polinomi,
      ln|a x + b| e ln(a x + b), arctan, e^{a x + b}, x^k e^{a x}, seni e
      coseni, e^{a x} sin(b x), 1/(a x + b)^k, radici di a x + b,
      ln(x^2 + p x + q), x ln x, x arctan x, tan, potenze di seni e coseni, e
      funzioni composte come e^{x^2}, sqrt(ln x), arctan(e^x)), con
      coefficienti razionali; ne calcola la derivata f con le regole di
      derivazione di questo file, e scrive in un file .tex della cartella di
      lavoro:
        - enunciati veri \\int f \\, dx = F + c, e varianti false (un
          coefficiente cambiato, un segno, un termine in più, un argomento
          spostato);
        - integrali definiti \\int_a^b f \\, dx = F(b) - F(a) su intervalli
          in cui f è continua, e varianti false del valore;
        - richieste \\int f \\, dx e \\int_a^b f \\, dx, di cui controlla la
          risposta del programma.
      Poi fa decidere il file al programma e confronta i verdetti con
      l'oracolo. Con la variabile d'ambiente FUZZ_NOTES stampa anche le righe
      NOTE.

  python3 -I fuzz_integrals.py improper <eseguibile> <seme> <quanti> <cartella di lavoro>
      Integrali impropri (30k, 30l): potenze (x + s)^{-p} all'infinito e
      (b - x)^{-p} vicino a un estremo, x^m e^{-k x}, 1/(x (ln x)^beta),
      1/(x^2 + 1) su una semiretta o su tutta la retta, ln x da 0, e
      funzioni non definite in un punto interno (1/(x - m)^2, 1/(x - m),
      la radice cubica reale di x - m). Per ognuno: il valore giusto, valori
      sbagliati, "= +\\infty", \\in e \\notin \\mathbb{R}, e la richiesta.
      L'oracolo: il valore in forma chiusa e la convergenza dai teoremi sugli
      esponenti (vedi improper_case()), con il valore ricontrollato da una
      quadratura a doppio esponenziale (tanh-sinh, exp-sinh), divisa nel
      punto interno, a due passi che devono concordare entro 10^-9 tra loro
      ed entro 10^-7 con la forma chiusa: se no la riga ORACLE, e il caso si
      scarta.

L'oracolo non usa il programma per decidere. Ha le sue regole di derivazione
(simboliche, sugli alberi di questo file), il suo calcolo con i numeri in
virgola mobile, la sua quadratura (Gauss-Legendre composta a 20 nodi, con due
suddivisioni che devono concordare) e il suo lettore del LaTeX che il
programma scrive nelle risposte. Vero e falso:
  - una primitiva è giusta per costruzione: f è la derivata di F. L'oracolo
    lo ricontrolla con il rapporto incrementale centrato, e scarta i casi in
    cui non torna;
  - una variante è falsa se in un punto in cui F e f sono definite e lisce
    la derivata della variante (simbolica) e f differiscono oltre 10^-4 della
    loro grandezza; altrimenti viene scartata;
  - un valore è giusto se coincide con la quadratura entro 10^-8, falso se
    ne dista oltre 10^-4; in mezzo il caso si scarta;
  - una risposta del programma è giusta se, in almeno tre punti, il suo
    rapporto incrementale centrato (con due passi diversi, che devono
    concordare) coincide con f entro 10^-5; sbagliata se in un punto ne
    dista oltre 10^-3 con tutti e due i passi.
Una differenza conta solo se supera anche il rumore di arrotondamento del
conto in virgola mobile, stimato da magnitude(): 3 e^{x^2} - 3 e^{x^2} in
x = 6 vale zero, ma i suoi termini sono grandi 10^17, e il valore calcolato
non dice nulla (è successo: l'oracolo ha segnalato come sbagliata una
primitiva giusta).
Le radici e le potenze a esponente frazionario hanno senso solo per basi
positive, come nel programma.

Righe stampate:

  BUG: il programma ha dimostrato un enunciato falso, ha detto FALSO di uno
       vero (o ha dato un controesempio x = p/q in cui F' = f), ha dato una
       risposta sbagliata; oppure ha stampato la riga ATTENZIONE, cioè un suo
       controllo indipendente ha smentito un suo passo.
  SUSPECT: l'oracolo non riesce a controllare una risposta del programma.
  NOTE: un enunciato vero non dimostrato, o una richiesta senza risposta
        (non è un errore).
"""
import math
import os
import random
import re
import subprocess
import sys
from fractions import Fraction as Fr

NOTES = bool(os.environ.get("FUZZ_NOTES"))


class Undefined(Exception):
    pass


class Unsupported(Exception):
    pass


# ---------------------------------------------------------------------------
# Alberi: tuple (tipo, figli...). Tipi: num (Fraction), x, add, sub, mul, div,
# neg, pow (base, esponente Fraction), exp (e^a), ln, lnabs, sin, cos, tan,
# atan, sqrt, root (radicando, indice), abs, pi, e.
# ---------------------------------------------------------------------------

def N(v):
    return ("num", Fr(v))


X = ("x",)


def is_num(t, v=None):
    return t[0] == "num" and (v is None or t[1] == v)


def add(a, b):
    if is_num(a, 0):
        return b
    if is_num(b, 0):
        return a
    if is_num(a) and is_num(b):
        return N(a[1] + b[1])
    return ("add", a, b)


def sub(a, b):
    if is_num(b, 0):
        return a
    if is_num(a) and is_num(b):
        return N(a[1] - b[1])
    return ("sub", a, b)


def mul(a, b):
    if is_num(a, 0) or is_num(b, 0):
        return N(0)
    if is_num(a, 1):
        return b
    if is_num(b, 1):
        return a
    if is_num(a) and is_num(b):
        return N(a[1] * b[1])
    return ("mul", a, b)


def div(a, b):
    if is_num(b, 1):
        return a
    if is_num(a, 0):
        return N(0)
    if is_num(a) and is_num(b):
        return N(a[1] / b[1])
    return ("div", a, b)


def neg(a):
    if is_num(a):
        return N(-a[1])
    return ("neg", a)


def power(a, e):
    e = Fr(e)
    if e == 0:
        return N(1)
    if e == 1:
        return a
    return ("pow", a, e)


def has_x(t):
    if t[0] == "x":
        return True
    return any(has_x(c) for c in t[1:] if isinstance(c, tuple))


def d(t):
    """La derivata di t rispetto a x, con le regole di questo file."""
    k = t[0]
    if not has_x(t):
        return N(0)
    if k == "x":
        return N(1)
    if k == "add":
        return add(d(t[1]), d(t[2]))
    if k == "sub":
        return sub(d(t[1]), d(t[2]))
    if k == "neg":
        return neg(d(t[1]))
    if k == "mul":
        return add(mul(d(t[1]), t[2]), mul(t[1], d(t[2])))
    if k == "div":
        return div(sub(mul(d(t[1]), t[2]), mul(t[1], d(t[2]))), power(t[2], 2))
    if k == "pow":
        return mul(mul(N(t[2]), power(t[1], t[2] - 1)), d(t[1]))
    if k == "exp":
        return mul(t, d(t[1]))
    if k in ("ln", "lnabs"):
        return div(d(t[1]), t[1])
    if k == "sin":
        return mul(("cos", t[1]), d(t[1]))
    if k == "cos":
        return neg(mul(("sin", t[1]), d(t[1])))
    if k == "tan":
        return div(d(t[1]), power(("cos", t[1]), 2))
    if k == "atan":
        return div(d(t[1]), add(N(1), power(t[1], 2)))
    if k == "sqrt":
        return div(d(t[1]), mul(N(2), t))
    if k == "root":
        return div(mul(t, d(t[1])), mul(N(t[2]), t[1]))
    raise Unsupported(k)


def value(t, x):
    """Il valore di t in x, in virgola mobile; Undefined fuori dal dominio."""
    k = t[0]
    try:
        if k == "num":
            return float(t[1])
        if k == "x":
            return x
        if k == "pi":
            return math.pi
        if k == "e":
            return math.e
        if k == "add":
            return value(t[1], x) + value(t[2], x)
        if k == "sub":
            return value(t[1], x) - value(t[2], x)
        if k == "neg":
            return -value(t[1], x)
        if k == "mul":
            return value(t[1], x) * value(t[2], x)
        if k == "div":
            b = value(t[2], x)
            if b == 0:
                raise Undefined()
            return value(t[1], x) / b
        if k == "pow":
            a, e = value(t[1], x), t[2]
            if isinstance(e, tuple):
                ev = value(e, x)
                if a <= 0:
                    raise Undefined()
                return math.exp(ev * math.log(a))
            if e.denominator == 1:
                if a == 0 and e < 0:
                    raise Undefined()
                return a ** int(e)
            if a <= 0:
                raise Undefined()
            return a ** float(e)
        if k == "exp":
            return math.exp(value(t[1], x))
        if k == "ln":
            a = value(t[1], x)
            if a <= 0:
                raise Undefined()
            return math.log(a)
        if k == "lnabs":
            a = value(t[1], x)
            if a == 0:
                raise Undefined()
            return math.log(abs(a))
        if k == "log":
            a, b = value(t[1], x), value(t[2], x)
            if a <= 0 or b <= 0 or b == 1:
                raise Undefined()
            return math.log(a) / math.log(b)
        if k == "abs":
            return abs(value(t[1], x))
        if k == "sin":
            return math.sin(value(t[1], x))
        if k == "cos":
            return math.cos(value(t[1], x))
        if k == "tan":
            c = math.cos(value(t[1], x))
            if c == 0:
                raise Undefined()
            return math.sin(value(t[1], x)) / c
        if k == "cot":
            s = math.sin(value(t[1], x))
            if s == 0:
                raise Undefined()
            return math.cos(value(t[1], x)) / s
        if k == "atan":
            return math.atan(value(t[1], x))
        if k == "sqrt":
            a = value(t[1], x)
            if a < 0:
                raise Undefined()
            return math.sqrt(a)
        if k == "cbrt":
            a = value(t[1], x)
            return math.copysign(abs(a) ** (1.0 / 3), a)
        if k == "root":
            a = value(t[1], x)
            if a < 0 or (a == 0 and False):
                raise Undefined()
            if a == 0:
                return 0.0
            return a ** (1.0 / t[2])
    except (OverflowError, ValueError, ZeroDivisionError):
        raise Undefined()
    raise Unsupported(k)


def finite_value(t, x):
    v = value(t, x)
    if v != v or abs(v) > 1e12:
        raise Undefined()
    return v


def magnitude(t, x):
    """La grandezza dei numeri che il calcolo di t in x attraversa: lo stesso
    conto con le somme fatte sui valori assoluti. L'errore di arrotondamento
    del valore è al più qualche 10^-16 volte questo numero: due termini
    grandi che si cancellano (3 e^{x^2} - 3 e^{x^2}) danno un valore piccolo
    e una grandezza grande, e lì un confronto tra valori non dice nulla."""
    k = t[0]
    if k in ("add", "sub"):
        return magnitude(t[1], x) + magnitude(t[2], x)
    if k == "neg":
        return magnitude(t[1], x)
    if k == "mul":
        return magnitude(t[1], x) * magnitude(t[2], x)
    if k == "div":
        return magnitude(t[1], x) / abs(value(t[2], x))
    return abs(value(t, x))


def noise(t, x):
    """Un margine oltre il quale una differenza dal valore di t in x non è
    rumore di arrotondamento."""
    try:
        return 1e-9 * magnitude(t, x)
    except (Undefined, ZeroDivisionError, OverflowError):
        return float("inf")


# ---------------------------------------------------------------------------
# LaTeX nel lettore del programma (capitolo 9 di parser.c), con le parentesi
# \left( \right) dove servono.
# ---------------------------------------------------------------------------

def frac_tex(q):
    q = Fr(q)
    if q.denominator == 1:
        return str(q.numerator)
    s = "-" if q < 0 else ""
    return "%s\\frac{%d}{%d}" % (s, abs(q.numerator), q.denominator)


PREC = {"add": 1, "sub": 1, "neg": 1, "mul": 3, "div": 5, "pow": 4}


def prec(t):
    if t[0] == "num":
        return 1 if t[1] < 0 or t[1].denominator != 1 else 7
    if t[0] in ("x", "pi", "e"):
        return 7
    return PREC.get(t[0], 5)


def wrap(t, need, bare):
    s = tex(t, bare)
    return "\\left(%s\\right)" % s if prec(t) < need else s


def tex(t, bare=False):
    k = t[0]
    if k == "num":
        return frac_tex(t[1])
    if k == "x":
        return "x"
    if k == "pi":
        return "\\pi"
    if k == "e":
        return "e"
    if k == "add":
        return "%s + %s" % (wrap(t[1], 1, bare), wrap(t[2], 2, bare))
    if k == "sub":
        return "%s - %s" % (wrap(t[1], 1, bare), wrap(t[2], 2, bare))
    if k == "neg":
        return "-%s" % wrap(t[1], 3, bare)
    if k == "mul":
        return "%s \\cdot %s" % (wrap(t[1], 3, bare), wrap(t[2], 4, bare))
    if k == "div":
        return "\\frac{%s}{%s}" % (tex(t[1], bare), tex(t[2], bare))
    if k == "pow":
        e = t[2]
        return "%s^{%s}" % (wrap(t[1], 6, bare), frac_tex(e))
    if k == "exp":
        return "e^{%s}" % tex(t[1], bare)
    if k == "ln":
        return "\\ln\\left(%s\\right)" % tex(t[1], bare)
    if k == "lnabs":
        return ("\\ln|%s|" if bare else "\\ln\\left|%s\\right|") % tex(t[1], bare)
    if k in ("sin", "cos", "tan"):
        a = t[1]
        if bare and a[0] == "mul" and is_num(a[1]) and a[1][1] > 0 and a[1][1].denominator == 1 and a[2] == X:
            return "\\%s %dx" % (k, a[1][1].numerator)
        if bare and a == X:
            return "\\%s x" % k
        return "\\%s\\left(%s\\right)" % (k, tex(a, bare))
    if k == "atan":
        return "\\arctan\\left(%s\\right)" % tex(t[1], bare)
    if k == "sqrt":
        return "\\sqrt{%s}" % tex(t[1], bare)
    if k == "root":
        return "\\sqrt[%d]{%s}" % (t[2], tex(t[1], bare))
    if k == "cbrt":
        return "\\sqrt[3]{%s}" % tex(t[1], bare)
    raise Unsupported(k)


# ---------------------------------------------------------------------------
# Il lettore delle risposte del programma: le scrittura di put_node() e di
# put_rat() del capitolo 13, e quelle dei metodi del capitolo 30.
# ---------------------------------------------------------------------------

def tokenize(text):
    out, i = [], 0
    while i < len(text):
        c = text[i]
        if c.isspace():
            i += 1
        elif c.isdigit():
            j = i
            while j < len(text) and text[j].isdigit():
                j += 1
            out.append(("num", text[i:j]))
            i = j
        elif c.isalpha():
            out.append(("let", c))
            i += 1
        elif c == "\\":
            j = i + 1
            if j < len(text) and not text[j].isalpha():
                if text[j] in ",;!: ":
                    i = j + 1
                    continue
                raise Unsupported("command")
            while j < len(text) and text[j].isalpha():
                j += 1
            name = text[i + 1:j]
            i = j
            if name in ("left", "right"):
                while text[i].isspace():
                    i += 1
                delim = text[i]
                i += 1
                if delim == "|":
                    out.append(("abso" if name == "left" else "absc", "|"))
                elif delim in "([":
                    out.append(("open", "("))
                elif delim in ")]":
                    out.append(("close", ")"))
                else:
                    raise Unsupported("delimiter")
            elif name in ("displaystyle", "textstyle"):
                continue
            else:
                out.append(("cmd", name))
        else:
            if c == "(":
                out.append(("open", c))
            elif c == ")":
                out.append(("close", c))
            elif c == "|":
                out.append(("bar", c))
            else:
                out.append(("ch", c))
            i += 1
    out.append(("end", ""))
    return out


class Reader:
    def __init__(self, text):
        self.t = tokenize(text)
        self.i = 0

    def peek(self):
        return self.t[self.i]

    def take(self):
        tok = self.t[self.i]
        self.i += 1
        return tok

    def expect(self, kind, val=None):
        tok = self.take()
        if tok[0] != kind or (val is not None and tok[1] != val):
            raise Unsupported("expected %s %s, got %s" % (kind, val, tok))
        return tok

    def expr(self):
        k, v = self.peek()
        if (k, v) == ("ch", "-"):
            self.take()
            node = neg(self.term())
        else:
            if (k, v) == ("ch", "+"):
                self.take()
            node = self.term()
        while True:
            k, v = self.peek()
            if (k, v) == ("ch", "+"):
                self.take()
                node = ("add", node, self.term())
            elif (k, v) == ("ch", "-"):
                self.take()
                node = ("sub", node, self.term())
            else:
                return node

    def starts_factor(self):
        k, v = self.peek()
        return k in ("num", "let", "open", "abso") or (k == "cmd" and v in ("frac", "sqrt", "pi", "sin", "cos", "tan", "cot", "arctan", "ln", "log"))

    def term(self):
        node = self.factor()
        while True:
            k, v = self.peek()
            if (k, v) == ("cmd", "cdot"):
                self.take()
                node = ("mul", node, self.factor())
            elif self.starts_factor():
                node = ("mul", node, self.factor())
            else:
                return node

    def group(self):
        if self.peek() == ("ch", "{"):
            self.take()
            e = self.expr()
            self.expect("ch", "}")
            return e
        k, v = self.take()
        if k == "num":
            if len(v) > 1:
                raise Unsupported("digits")
            return N(int(v))
        if k == "let":
            return X if v == "x" else ("e",) if v == "e" else self._letter(v)
        raise Unsupported("group")

    def _letter(self, v):
        raise Unsupported("letter %s" % v)

    def factor(self):
        node = self.primary()
        while self.peek() == ("ch", "^"):
            self.take()
            e = self.group()
            node = power(node, e[1]) if is_num(e) else ("pow", node, e)
        return node

    def primary(self):
        k, v = self.take()
        if k == "num":
            return N(int(v))
        if k == "let":
            if v == "x":
                return X
            if v == "e":
                return ("e",)
            raise Unsupported("letter %s" % v)
        if k == "open":
            e = self.expr()
            self.expect("close")
            return e
        if k == "abso":
            e = self.expr()
            self.expect("absc")
            return ("abs", e)
        if k == "cmd":
            if v == "frac":
                a = self.group()
                b = self.group()
                return ("div", a, b)
            if v == "pi":
                return ("pi",)
            if v == "sqrt":
                if self.peek() == ("ch", "["):
                    self.take()
                    n = int(self.expect("num")[1])
                    self.expect("ch", "]")
                    return ("root", self.group(), n)
                return ("sqrt", self.group())
            if v in ("sin", "cos", "tan", "cot", "arctan", "ln", "log"):
                exponent = None
                base = None
                while self.peek() in (("ch", "^"), ("ch", "_")):
                    which = self.take()[1]
                    g = self.group()
                    if which == "^":
                        exponent = g
                    else:
                        base = g
                if self.peek()[0] == "open":
                    self.take()
                    arg = self.expr()
                    self.expect("close")
                else:
                    arg = self.factor()
                fn = {"arctan": "atan"}.get(v, v)
                if fn == "ln" and arg[0] == "abs":
                    node = ("lnabs", arg[1])
                elif fn == "log":
                    node = ("log", arg, base if base is not None else N(10))
                else:
                    node = (fn, arg)
                if exponent is not None:
                    node = power(node, exponent[1]) if is_num(exponent) else ("pow", node, exponent)
                return node
        raise Unsupported("primary %s %s" % (k, v))


def read_answer(text):
    r = Reader(text)
    e = r.expr()
    if r.peek()[0] != "end":
        raise Unsupported("trailing %s" % (r.peek(),))
    return e


# ---------------------------------------------------------------------------
# Le famiglie delle primitive.
# ---------------------------------------------------------------------------

def rq(rnd, small=False):
    """Un razionale a caso, non nullo."""
    while True:
        q = Fr(rnd.randint(-6, 6), rnd.choice([1, 1, 1, 2, 3, 4]) if not small else 1)
        if q != 0:
            return q


def lin(a, b):
    return add(mul(N(a), X), N(b))


def block(rnd):
    """Un pezzo di primitiva, con il suo coefficiente."""
    a, b = rq(rnd, True), rq(rnd)
    c = rq(rnd)
    kind = rnd.randrange(22)
    if kind == 0:
        body = power(X, rnd.randint(1, 5))
    elif kind == 1:
        body = ("lnabs", lin(a, b))
    elif kind == 2:
        body = ("ln", lin(abs(a), abs(b) + 1))
    elif kind == 3:
        body = ("atan", lin(a, b))
    elif kind == 4:
        body = ("exp", lin(a, b))
    elif kind == 5:
        body = mul(power(X, rnd.randint(1, 3)), ("exp", mul(N(a), X)))
    elif kind == 6:
        body = (rnd.choice(["sin", "cos"]), lin(a, b) if rnd.random() < 0.5 else mul(N(abs(a)), X))
    elif kind == 7:
        body = mul(("exp", mul(N(a), X)), (rnd.choice(["sin", "cos"]), mul(N(rq(rnd, True)), X)))
    elif kind == 8:
        body = power(lin(a, b), -rnd.randint(1, 3))
    elif kind == 9:
        body = ("sqrt", lin(abs(a), abs(b)))
    elif kind == 10:
        body = power(lin(abs(a), abs(b)), Fr(rnd.choice([1, 2, 4, 5]), 3))
    elif kind == 11:
        p = rnd.randint(-3, 3)
        q = Fr(p * p, 4) + rnd.randint(1, 4)
        body = ("ln", add(add(power(X, 2), mul(N(p), X)), N(q)))
    elif kind == 12:
        body = mul(power(X, rnd.randint(1, 2)), ("ln", X))
    elif kind == 13:
        body = mul(X, ("atan", X))
    elif kind == 14:
        body = ("tan", mul(N(abs(a)), X))
    elif kind == 15:
        body = power((rnd.choice(["sin", "cos"]), X), rnd.randint(2, 4))
    elif kind == 16:
        body = ("exp", power(X, 2))
    elif kind == 17:
        body = ("sqrt", ("ln", X))
    elif kind == 18:
        body = ("atan", ("exp", X))
    elif kind == 19:
        body = ("lnabs", ("lnabs", X))
    elif kind == 20:
        body = ("root", X, rnd.choice([3, 4, 6]))
    else:
        body = div(N(1), lin(a, b))
    return mul(N(c), body)


def primitive(rnd):
    F = block(rnd)
    for _ in range(rnd.choice([0, 0, 1, 1, 2])):
        F = add(F, block(rnd))
    return F


POINTS = [0.3, 1.4, -0.9, 2.5, -2.6, 4.5, 0.15, -0.35, 3.7, 6.0, -6.0, 1.7, 0.7, 5.2, -1.6]


def slope(F, x):
    """Il rapporto incrementale centrato di F in x, con due passi."""
    out = []
    for h in (1e-5, 3e-6):
        step = h * (1 + abs(x))
        out.append((finite_value(F, x + step) - finite_value(F, x - step)) / (2 * step))
    return out


def close(a, b, tol):
    return abs(a - b) <= tol * (1 + max(abs(a), abs(b)))


def check_primitive(F, f, points=POINTS):
    """'true' se F' = f (rapporto incrementale) in almeno tre punti e mai
    lontano, 'false' se in un punto è di sicuro diverso, 'unsure' altrimenti;
    con il punto in cui è falso."""
    agree = 0
    for x in points:
        try:
            fx = finite_value(f, x)
            s1, s2 = slope(F, x)
        except Undefined:
            continue
        if not close(s1, s2, 1e-6):
            continue
        if close(s1, fx, 1e-5):
            agree += 1
        elif not close(s1, fx, 1e-3) and not close(s2, fx, 1e-3) and abs(s1 - fx) > noise(f, x) + noise(F, x) / 1e-5:
            return "false", x
    return ("true" if agree >= 3 else "unsure"), None


def symbolic_false(G, f):
    """La variante G è falsa se la sua derivata simbolica è lontana da f in
    un punto in cui tutto è definito."""
    g = d(G)
    seen = 0
    for x in POINTS:
        try:
            a, b = finite_value(g, x), finite_value(f, x)
            finite_value(G, x)
        except Undefined:
            continue
        seen += 1
        if not close(a, b, 1e-4) and abs(a - b) > noise(g, x) + noise(f, x):
            return True
    return False


def variant(F, rnd):
    """Una variante di F: un numero cambiato, un segno, un termine in più."""
    nums = []

    def walk(t, path):
        if t[0] == "num":
            nums.append(path)
        for i, c in enumerate(t[1:], 1):
            if isinstance(c, tuple):
                walk(c, path + (i,))

    def replace(t, path, new):
        if not path:
            return new
        i = path[0]
        return t[:i] + (replace(t[i], path[1:], new),) + t[i + 1:]

    def get(t, path):
        for i in path:
            t = t[i]
        return t

    choice = rnd.randrange(4)
    if choice == 0 and nums:
        p = rnd.choice(nums)
        return replace(F, p, N(get(F, p)[1] + rnd.choice([1, -1])))
    if choice == 1:
        return add(F, rnd.choice([X, mul(N(rq(rnd)), power(X, 2)), ("sin", X), ("lnabs", X)]))
    if choice == 2 and F[0] in ("add", "sub"):
        return (F[0], F[1], neg(F[2]))
    return mul(N(2), F) if rnd.random() < 0.5 else neg(F)


def interval(f, rnd):
    """Un intervallo [a, b], a razionali, in cui f è definita e limitata."""
    for _ in range(20):
        a = Fr(rnd.randint(-40, 40), 10)
        b = a + Fr(rnd.randint(2, 25), 10)
        ok = True
        for k in range(401):
            x = float(a) + (float(b) - float(a)) * k / 400
            try:
                if abs(finite_value(f, x)) > 1e6:
                    ok = False
                    break
            except Undefined:
                ok = False
                break
        if ok:
            return a, b
    return None


GL_X = [-0.9931285991850949, -0.9639719272779138, -0.9122344282513259, -0.8391169718222188, -0.7463319064601508, -0.636053680726515,
        -0.5108670019508271, -0.37370608871541955, -0.2277858511416451, -0.07652652113349734, 0.07652652113349734, 0.2277858511416451,
        0.37370608871541955, 0.5108670019508271, 0.636053680726515, 0.7463319064601508, 0.8391169718222188, 0.9122344282513259,
        0.9639719272779138, 0.9931285991850949]
GL_W = [0.017614007139152118, 0.04060142980038694, 0.06267204833410907, 0.08327674157670475, 0.10193011981724044, 0.11819453196151841,
        0.13168863844917664, 0.14209610931838204, 0.14917298647260374, 0.15275338713072584, 0.15275338713072584, 0.14917298647260374,
        0.14209610931838204, 0.13168863844917664, 0.11819453196151841, 0.10193011981724044, 0.08327674157670475, 0.06267204833410907,
        0.04060142980038694, 0.017614007139152118]


def quadrature(f, a, b, pieces):
    total = 0.0
    width = (b - a) / pieces
    for k in range(pieces):
        lo = a + k * width
        for xi, wi in zip(GL_X, GL_W):
            total += wi * finite_value(f, lo + (xi + 1) * width / 2) * width / 2
    return total


def integral(f, a, b):
    """La quadratura, se due suddivisioni concordano; altrimenti None."""
    try:
        q1, q2 = quadrature(f, float(a), float(b), 16), quadrature(f, float(a), float(b), 40)
    except Undefined:
        return None
    return q2 if close(q1, q2, 1e-11) else None


def subst(t, x_value):
    if t == X:
        return x_value
    return tuple(subst(c, x_value) if isinstance(c, tuple) else c for c in t)


def number(t):
    return finite_value(t, 0.0)


# ---------------------------------------------------------------------------
# I casi, il programma, il confronto.
# ---------------------------------------------------------------------------

def make_case(rnd):
    """Un caso: il testo dell'\\item e ciò che l'oracolo ne sa."""
    F = primitive(rnd)
    f = d(F)
    bare = rnd.random() < 0.3
    truth, _ = check_primitive(F, f)
    if truth != "true":
        return None
    kind = rnd.randrange(6)
    if kind <= 1:
        G, label = F, "true"
        if kind == 1:
            G = variant(F, rnd)
            if not symbolic_false(G, f):
                return None
            label = "false"
        return {"text": "\\int %s \\, dx = %s + c" % (tex(f, bare), tex(G, bare)), "kind": "primitive", "truth": label, "F": G, "f": f}
    if kind == 2:
        return {"text": "\\int %s \\, dx" % tex(f, bare), "kind": "request", "f": f}
    span = interval(f, rnd)
    if span is None:
        return None
    a, b = span
    q = integral(f, a, b)
    if q is None:
        return None
    try:
        exact = sub(subst(F, N(b)), subst(F, N(a)))
        v = number(exact)
    except Undefined:
        return None
    if not close(v, q, 1e-8):
        return None
    bounds = "\\int_{%s}^{%s}" % (frac_tex(a), frac_tex(b))
    if kind == 3:
        return {"text": "%s %s \\, dx = %s" % (bounds, tex(f, bare), tex(exact, bare)), "kind": "definite", "truth": "true", "value": q, "f": f}
    if kind == 4:
        wrong = rnd.choice([add(exact, N(Fr(1, 7))), mul(exact, N(Fr(101, 100))), neg(exact), add(exact, N(1))])
        try:
            wv = number(wrong)
        except Undefined:
            return None
        if close(wv, q, 1e-4):
            return None
        return {"text": "%s %s \\, dx = %s" % (bounds, tex(f, bare), tex(wrong, bare)), "kind": "definite", "truth": "false", "value": q, "f": f}
    return {"text": "%s %s \\, dx" % (bounds, tex(f, bare)), "kind": "drequest", "value": q, "f": f}


def run_engine(exe, path):
    run = subprocess.run([exe, path], capture_output=True, text=True, timeout=7200)
    lines = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
    warning = [l for l in run.stdout.splitlines() if l.startswith("ATTENZIONE")]
    return run, lines, warning


def classify(case, line):
    """Una riga BUG, SUSPECT o NOTE, oppure None."""
    text = line.split(". ", 1)[1]
    proved = text.startswith("DIMOSTRATO")
    refuted = text.startswith("FALSO")
    solved = text.startswith("RISOLTO")
    kind = case["kind"]
    if kind in ("primitive", "definite"):
        if proved and case["truth"] == "false":
            return "BUG: proved, but it is false"
        if refuted and case["truth"] == "true":
            return "BUG: refuted, but it is true"
        if refuted and kind == "primitive":
            point = re.search(r"x = (-?\d+)/(\d+)", text)
            if point and case["truth"] == "false":
                x = int(point.group(1)) / int(point.group(2))
                try:
                    if close(finite_value(d(case["F"]), x), finite_value(case["f"], x), 1e-7):
                        return "BUG: refuted at x = %s, where the derivative is the integrand" % point.group(0)[4:]
                except Undefined:
                    return "BUG: refuted at %s, where a side is not defined" % point.group(0)
        if solved:
            return "BUG: a statement answered as a request"
        if not proved and not refuted and case["truth"] == "true":
            return "NOTE: true, not proved"
        return None
    if refuted or proved:
        return "BUG: a request answered with %s" % text[:20]
    if not solved:
        return "NOTE: request not answered"
    answer = text.split(": ", 1)[1]
    if kind == "request":
        answer = answer.split(" + c")[0] if " + c (" in answer else answer[:-4] if answer.endswith(" + c") else answer
        try:
            G = read_answer(answer)
        except Unsupported as why:
            return "SUSPECT: the oracle cannot read the answer (%s): %s" % (why, answer[:200])
        truth, where = check_primitive(G, case["f"])
        if truth == "false":
            return "BUG: wrong antiderivative (at x = %s): %s" % (where, answer[:300])
        if truth == "unsure":
            return "SUSPECT: the oracle cannot confirm the antiderivative: %s" % answer[:300]
        return None
    try:
        v = number(read_answer(answer))
    except (Unsupported, Undefined) as why:
        return "SUSPECT: the oracle cannot read or compute the value (%s): %s" % (why, answer[:200])
    if close(v, case["value"], 1e-8):
        return None
    if not close(v, case["value"], 1e-4):
        return "BUG: wrong value %s, the quadrature gives %.15g" % (answer[:200], case["value"])
    return "SUSPECT: the value %s and the quadrature %.15g are close but not equal" % (answer[:200], case["value"])


def fuzz(exe, seed, count, work):
    rnd = random.Random(seed)
    cases = []
    while len(cases) < count:
        try:
            case = make_case(rnd)
        except (Undefined, Unsupported, ZeroDivisionError, OverflowError):
            case = None
        if case and len(case["text"]) < 3000:
            cases.append(case)
    os.makedirs(work, exist_ok=True)
    path = os.path.join(work, "fuzz_integrals_%d.tex" % seed)
    with open(path, "w", encoding="utf-8") as out:
        out.write("\\begin{enumerate}\n")
        for case in cases:
            out.write("\\item $\\displaystyle %s$\n" % case["text"])
        out.write("\\end{enumerate}\n")
    run, lines, warning = run_engine(exe, path)
    if len(lines) != len(cases) or run.returncode not in (0, 1):
        print("BUG: the program answered %d of %d items (exit %d)\n%s" % (len(lines), len(cases), run.returncode, run.stderr[-2000:]))
        return
    for w in warning:
        print("BUG: " + w)
    tally = {}
    for index, (case, line) in enumerate(zip(cases, lines), 1):
        problem = classify(case, line)
        key = (case["kind"], case.get("truth", "-"), line.split(". ", 1)[1].split(" ")[0])
        tally[key] = tally.get(key, 0) + 1
        if problem and (NOTES or not problem.startswith("NOTE")):
            print("%s\n   %d [%s]: %s" % (problem, index, line.split(". ", 1)[1][:120], case["text"][:400]))
    print("seed %d: %s" % (seed, ", ".join("%s/%s/%s=%d" % (k + (v,)) for k, v in sorted(tally.items()))))


# ---------------------------------------------------------------------------
# Gli integrali impropri (30k e 30l del programma).
#
# Famiglie con un valore noto in forma chiusa, scritto qui da conti a mano
# (le formule sono in improper_case()), e la convergenza decisa dai teoremi
# dei libri sugli esponenti, non dal programma: \int^{+\infty} (x + s)^{-p}
# converge se e solo se p > 1, \int_{b} (b - x)^{-p} se e solo se p < 1,
# \int^{+\infty} \frac{dx}{x (\ln x)^{\beta}} se e solo se \beta > 1. Ogni
# valore finito è ricontrollato con una quadratura tanh-sinh o exp-sinh
# (doppio esponenziale, che regge le singolarità negli estremi), a due passi
# che devono concordare con il valore chiuso entro 10^-7: se no il caso si
# scarta con una riga ORACLE.
# ---------------------------------------------------------------------------

def de_quadrature(g, a, b, h):
    """\\int_a^b g, con a e b finiti, per la sostituzione tanh-sinh:
    x = (a + b)/2 + (b - a)/2 tanh(pi/2 sinh s). I nodi vicini agli estremi
    si calcolano con la distanza dall'estremo, per non perdere cifre."""
    total, half = 0.0, (b - a) / 2
    k = 0
    while True:
        s = k * h
        done = True
        for sign in ((1,) if k == 0 else (1, -1)):
            u = math.pi / 2 * math.sinh(sign * s)
            if abs(u) > 350:
                continue
            w = half * math.pi / 2 * math.cosh(s) / math.cosh(u) ** 2
            gap = 2 * half / (math.exp(2 * abs(u)) + 1)  # distanza dall'estremo
            x = (b - gap) if u > 0 else (a + gap)
            if gap <= 0 or x <= a or x >= b:
                continue
            try:
                term = w * g(x)
            except (Undefined, ValueError, OverflowError, ZeroDivisionError):
                # Un nodo così vicino all'estremo che f vi esce dai numeri
                # (f ~ (b - x)^{-p}, con p < 1): il suo peso w f, che tende a
                # zero, si trascura; altrove è un errore.
                if gap < 1e-9 * (b - a):
                    continue
                return None
            total += term
            if abs(term) > 1e-18:
                done = False
        if k > 0 and done:
            break
        k += 1
        if k > 4000:
            return None
    return total * h


def de_infinite(g, a, h):
    """\\int_a^{+\\infty} g per exp-sinh: x = a + e^{pi/2 sinh s}."""
    total, k = 0.0, 0
    while True:
        done = True
        for sign in ((1,) if k == 0 else (1, -1)):
            s = sign * k * h
            u = math.pi / 2 * math.sinh(s)
            if u > 700 or u < -700:
                continue
            x = a + math.exp(u)
            if x <= a:
                continue
            w = math.pi / 2 * math.cosh(s) * math.exp(u)
            try:
                term = w * g(x)
            except (Undefined, ValueError, OverflowError, ZeroDivisionError):
                return None
            total += term
            if abs(term) > 1e-18:
                done = False
        if k > 0 and done:
            break
        k += 1
        if k > 4000:
            return None
    return total * h


def numeric_improper(f, a, b, middle=None):
    """La quadratura di f da a a b (a = None per -\\infty, b = None per
    +\\infty), divisa in middle se f lì non è definita, a due passi; None se
    non concordano."""
    if middle is not None:
        left, right = numeric_improper(f, a, middle), numeric_improper(f, middle, b)
        return None if left is None or right is None else left + right

    def g(x):
        # value() senza il tetto di finite_value(): vicino a un estremo
        # singolare (b - x)^{-p} supera 10^{12} con un peso che non è
        # trascurabile.
        v = value(f, x)
        if v != v or v in (float("inf"), float("-inf")):
            raise Undefined()
        return v
    out = []
    for h in (1 / 16, 1 / 32):
        if a is not None and b is not None:
            v = de_quadrature(g, float(a), float(b), h)
        elif a is not None:
            v = de_infinite(g, float(a), h)
        elif b is not None:
            v = de_infinite(lambda y: g(-y), -float(b), h)
        else:
            left, right = de_infinite(lambda y: g(-y), 0.0, h), de_infinite(g, 0.0, h)
            v = None if left is None or right is None else left + right
        if v is None:
            return None
        out.append(v)
    return out[1] if close(out[0], out[1], 1e-9) else None


def improper_case(rnd):
    """Un integrale improprio: (f, a, b, middle, verità, valore[, quadratura]),
    con a o b None per gli infiniti, middle il punto interno in cui f non è
    definita (o None), verità "finite", "plus", "minus" o "neither", il
    valore (un albero) se è finito, e, se serve, la funzione e gli estremi
    su cui fare la quadratura dopo una sostituzione: \\frac{1}{x (\\ln x)^2}
    va all'infinito troppo piano per l'exp-sinh, e con x = e^u diventa
    u^{-2}; (b - x)^{-p} si calcola meglio in u = b - x."""
    c = rq(rnd)
    kind = rnd.randrange(7)
    if kind == 0:
        # \int_a^{+\infty} c (x + s)^{-p}: per p > 1 vale c (a + s)^{1-p} / (p - 1).
        s = rnd.randint(-2, 3)
        a = Fr(rnd.randint(-s + 1, 4))
        p = Fr(rnd.choice([1, 2, 3, 4, 5, 6, 7, 8]), rnd.choice([1, 2, 3]))
        f = mul(N(c), power(add(X, N(s)), -p))
        if p > 1:
            return f, a, None, None, "finite", mul(N(c / (p - 1)), power(N(a + s), 1 - p))
        return f, a, None, None, "plus" if c > 0 else "minus", None
    if kind == 1:
        # \int_a^b c (b - x)^{-p}: per p < 1 vale c (b - a)^{1-p} / (1 - p).
        a = Fr(rnd.randint(-3, 2))
        b = a + rnd.randint(1, 3)
        p = Fr(rnd.choice([1, 1, 2, 3, 4, 5]), rnd.choice([1, 2, 3, 4]))
        f = mul(N(c), power(sub(N(b), X), -p))
        if p < 1:
            # La quadratura in u = b - x, perché b - x calcolato vicino a b
            # perde le cifre che servono.
            return f, a, b, None, "finite", mul(N(c / (1 - p)), power(N(b - a), 1 - p)), (mul(N(c), power(X, -p)), Fr(0), b - a)
        return f, a, b, None, "plus" if c > 0 else "minus", None
    if kind == 2:
        # \int_a^{+\infty} c x^m e^{-k x}, k > 0: c e^{-k a} / k per m = 0,
        # c e^{-k a} (k a + 1) / k^2 per m = 1.
        k = Fr(rnd.choice([1, 2, 3, 1, 1]), rnd.choice([1, 1, 2]))
        a = Fr(rnd.randint(-2, 3))
        m = rnd.choice([0, 1])
        e = ("exp", N(-k * a))
        f = mul(N(c), ("exp", mul(N(-k), X)))
        if m:
            return mul(X, f), a, None, None, "finite", mul(N(c * (k * a + 1) / (k * k)), e)
        return f, a, None, None, "finite", mul(N(c / k), e)
    if kind == 3:
        # \int_a^{+\infty} \frac{c}{x (\ln x)^{\beta}}, a > 1: per \beta > 1
        # vale c (\ln a)^{1 - \beta} / (\beta - 1).
        a = Fr(rnd.choice([2, 3, 5]))
        beta = Fr(rnd.choice([1, 2, 3, 1, 3]), rnd.choice([1, 1, 2]))
        f = div(N(c), mul(X, power(("ln", X), beta)))
        if beta > 1:
            return f, a, None, None, "finite", mul(N(c / (beta - 1)), power(("ln", N(a)), 1 - beta)), (div(N(c), power(X, beta)), math.log(a), None)
        return f, a, None, None, "plus" if c > 0 else "minus", None
    if kind == 4:
        # \int_a^{+\infty} \frac{c}{x^2 + 1} = c (\pi/2 - \arctan a); su tutta
        # la retta c \pi.
        f = div(N(c), add(power(X, 2), N(1)))
        if rnd.random() < 0.4:
            return f, None, None, None, "finite", mul(N(c), ("pi",))
        a = Fr(rnd.randint(-2, 2))
        return f, a, None, None, "finite", mul(N(c), sub(div(("pi",), N(2)), ("atan", N(a))))
    if kind == 5:
        # Un punto m interno in cui f non è definita: c/(x - m)^2 vale
        # \infty con il segno di c; c/(x - m) non converge e non è un
        # infinito; c / \sqrt[3]{x - m} converge, a
        # (3c/2) ((b - m)^{2/3} - (m - a)^{2/3}).
        m = Fr(rnd.randint(-2, 2))
        a, b = m - rnd.randint(1, 3), m + rnd.randint(1, 3)
        which = rnd.randrange(3)
        if which == 0:
            return div(N(c), power(sub(X, N(m)), 2)), a, b, m, "plus" if c > 0 else "minus", None
        if which == 1:
            return div(N(c), sub(X, N(m))), a, b, m, "neither", None
        f = div(N(c), ("cbrt", sub(X, N(m))))
        return f, a, b, m, "finite", mul(N(c * Fr(3, 2)), sub(power(N(b - m), Fr(2, 3)), power(N(m - a), Fr(2, 3))))
    # \int_0^b c \ln x = c (b \ln b - b), b > 0.
    b = Fr(rnd.choice([1, 2, 3, 1]))
    return mul(N(c), ("ln", X)), Fr(0), b, None, "finite", mul(N(c), sub(mul(N(b), ("ln", N(b))), N(b)))


def improper(exe, seed, count, work):
    rnd = random.Random(seed)
    cases = []
    while len(cases) < count:
        try:
            case = improper_case(rnd)
        except (Undefined, Unsupported, ZeroDivisionError):
            continue
        f, a, b, middle, truth, exact = case[:6]
        check = case[6] if len(case) > 6 else (f, a, b)
        lower = "-\\infty" if a is None else frac_tex(a)
        upper = "+\\infty" if b is None else frac_tex(b)
        head = "\\int_{%s}^{%s} %s \\, dx" % (lower, upper, tex(f))
        if truth == "finite":
            try:
                v = finite_value(exact, 0.0)
            except Undefined:
                continue
            q = numeric_improper(check[0], check[1], check[2], middle)
            if q is None or not close(q, v, 1e-7):
                print("ORACLE: the closed form %.15g and the quadrature %s disagree: %s" % (v, q, head))
                continue
            cases.append((head + " = " + tex(exact), "true"))
            cases.append((head + " \\in \\mathbb{R}", "true"))
            for wrong in (add(exact, N(1)), mul(exact, N(2)) if abs(v) > 1e-9 else add(exact, N(Fr(1, 3))), neg(exact) if abs(v) > 1e-9 else N(1)):
                try:
                    if close(finite_value(wrong, 0.0), v, 1e-6):
                        continue
                except Undefined:
                    continue
                cases.append((head + " = " + tex(wrong), "false"))
            cases.append((head + " = +\\infty", "false"))
            cases.append((head + " \\notin \\mathbb{R}", "false"))
            cases.append((head, ("request", "finite", v)))
        else:
            cases.append((head + " \\notin \\mathbb{R}", "true"))
            cases.append((head + " \\in \\mathbb{R}", "false"))
            cases.append((head + " = 0", "false"))
            if truth in ("plus", "minus"):
                cases.append((head + " = %s\\infty" % ("+" if truth == "plus" else "-"), "true"))
                cases.append((head + " = %s\\infty" % ("-" if truth == "plus" else "+"), "false"))
            else:
                cases.append((head + " = +\\infty", "false"))
                cases.append((head + " = -\\infty", "false"))
            cases.append((head, ("request", truth, None)))
    os.makedirs(work, exist_ok=True)
    path = os.path.join(work, "fuzz_improper_%d.tex" % seed)
    with open(path, "w", encoding="utf-8") as out:
        out.write("\\begin{enumerate}\n")
        for text, _ in cases:
            out.write("\\item $\\displaystyle %s$\n" % text)
        out.write("\\end{enumerate}\n")
    run, lines, warning = run_engine(exe, path)
    if len(lines) != len(cases) or run.returncode not in (0, 1):
        print("BUG: the program answered %d of %d items (exit %d)\n%s" % (len(lines), len(cases), run.returncode, run.stderr[-2000:]))
        return
    for w in warning:
        print("BUG: " + w)
    tally = {}
    for index, ((text, expected), line) in enumerate(zip(cases, lines), 1):
        answer = line.split(". ", 1)[1]
        verdict = answer.split(":")[0].split(" ")[0]
        problem = None
        if isinstance(expected, tuple):
            _, truth, v = expected
            key = "request"
            if verdict in ("DIMOSTRATO", "FALSO"):
                problem = "BUG: a request answered with %s" % verdict
            elif verdict == "RISOLTO":
                written = answer.split(": ", 1)[1].split(" (")[0]
                if written in ("+\\infty", "-\\infty"):
                    if truth != ("plus" if written[0] == "+" else "minus"):
                        problem = "BUG: answered %s, the integral is %s" % (written, truth)
                else:
                    try:
                        w = finite_value(read_answer(written), 0.0)
                    except (Unsupported, Undefined) as why:
                        problem = "SUSPECT: the oracle cannot read the answer (%s): %s" % (why, written[:200])
                    else:
                        if truth != "finite" or not close(w, v, 1e-7):
                            problem = "BUG: answered %s, the integral is %s %s" % (written[:200], truth, v)
        else:
            key = expected
            if expected == "true" and verdict == "FALSO":
                problem = "BUG: true, but the program says FALSO"
            if expected == "false" and verdict == "DIMOSTRATO":
                problem = "BUG: false, but the program says DIMOSTRATO"
        tally[(key, verdict)] = tally.get((key, verdict), 0) + 1
        if problem:
            print("%s\n   %d [%s]: %s" % (problem, index, answer[:120], text[:400]))
    print("seed %d: %s" % (seed, ", ".join("%s/%s=%d" % (k + (v,)) for k, v in sorted(tally.items()))))


def main():
    if len(sys.argv) >= 6 and sys.argv[1] == "fuzz":
        fuzz(sys.argv[2], int(sys.argv[3]), int(sys.argv[4]), sys.argv[5])
    elif len(sys.argv) >= 6 and sys.argv[1] == "improper":
        improper(sys.argv[2], int(sys.argv[3]), int(sys.argv[4]), sys.argv[5])
    else:
        print(__doc__)
        sys.exit(2)


main()
