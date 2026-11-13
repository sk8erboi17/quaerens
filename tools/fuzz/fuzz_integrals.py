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


def main():
    if len(sys.argv) >= 6 and sys.argv[1] == "fuzz":
        fuzz(sys.argv[2], int(sys.argv[3]), int(sys.argv[4]), sys.argv[5])
    else:
        print(__doc__)
        sys.exit(2)


main()
