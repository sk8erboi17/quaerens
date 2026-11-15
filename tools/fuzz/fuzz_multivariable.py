"""Fuzzing differenziale degli enunciati sulle funzioni di più variabili (capitolo 34 di parser.c, 35 e 36 di main.c, 18 di checker.c).

Uso:

  python3 -I fuzz_multivariable.py fuzz <eseguibile> <seme> <quanti> <cartella di lavoro>
      Costruisce funzioni a caso di due o tre variabili (polinomi, frazioni,
      esponenziali, logaritmi, seni, coseni, radici, arcotangenti, e loro
      somme e prodotti), le deriva con le regole scritte qui, e ne ricava
      enunciati: una derivata parziale, anche mista, scritta come frazione
      con \\partial, con il pedice o con il segno davanti all'espressione; un
      gradiente; un'hessiana; gli stessi in un punto razionale (per le
      funzioni razionali, con i valori esatti); l'uguaglianza delle derivate
      miste; e richieste senza il segno di uguale. Da ogni enunciato vero
      ricava varianti alterate: un numero, un segno, un termine tolto, le
      variabili scambiate, una derivata in meno, il punto o un valore
      cambiato. La definitezza dell'hessiana con le scritture \\succ,
      \\prec, \\succeq e \\preceq, di forme quadratiche scelte in modo che
      capitino le matrici definite, quelle solo semidefinite, le indefinite
      e quelle con i minori di testa tutti nulli. In più, per i punti critici (35g e capitolo 36): le
      condizioni del secondo ordine in un punto (il gradiente nullo, il
      segno del determinante dell'hessiana, di f_xx e, con tre variabili,
      del minore di lato 2 scritto come tabella), con i segni veri o con uno
      cambiato; confronti tra numeri, anche con il determinante di una
      tabella; e la richiesta dei punti critici di funzioni costruite in
      modo da conoscerli tutti. Li scrive in un file .tex nella cartella di
      lavoro, li fa decidere al programma e confronta i verdetti con
      l'oracolo.

  python3 -I fuzz_multivariable.py check <eseguibile> <file .tex>...
      Rilegge con il lettore di questo file ogni enunciato dei file, dice se
      è vero secondo l'oracolo, e lo confronta con il verdetto del programma
      e con il nome del file: in un file false_*.tex gli enunciati devono
      essere falsi, negli altri veri. Di una richiesta controlla la risposta.

  python3 -I fuzz_multivariable.py certificates <checker> <seme> <quanti> <cartella di lavoro> <file .cert>...
      Dai certificati "partial" scritti da mathresearcher --certificates
      ricava <quanti> certificati alterati (un numero, un operatore, una
      lettera di derivazione, una coordinata, un termine di una somma, i due
      membri di una tesi scambiati o la sua relazione cambiata) e li fa
      controllare al verificatore. Di ognuno che viene ACCETTATO l'oracolo
      decide da sé se ciò che afferma è vero.

L'oracolo non usa il programma. Legge il LaTeX e i certificati con lettori
suoi, scritti sulla grammatica dei commenti (capitoli 9 e 34 di parser.c, 6 e
18 di checker.c) e non sul loro codice; deriva con regole sue, sull'albero; e
confronta i due membri di ogni tesi in 24 punti razionali a caso in cui le
funzioni sono regolari: con le frazioni esatte di Python quando l'enunciato
ha solo operazioni razionali (e radici di quadrati perfetti), altrimenti in
virgola mobile, dove due numeri che differiscono meno di 10^-9 della loro
grandezza sono uguali, più di 10^-6 sono diversi, e in mezzo l'oracolo non
decide. Un enunciato su un punto in cui la funzione non è definita è falso;
dove è definita ma non regolare (un radicando o l'argomento di un valore
assoluto che vale zero) la derivata può esistere, e l'oracolo non decide;
e non decide dove il conto esce dai float, che non vuol dire "non definita".
Una funzione definita con la derivata di un'altra esiste solo dove esiste
quell'altra, e così un termine di un certificato che ne nomina uno senza
punto.

Un confronto tra due numeri (<, >, \\le, \\ge, \\neq) è il segno di una
differenza: esatto con le frazioni, e con i float deciso solo quando i due
numeri sono di sicuro diversi. Un determinante si calcola con l'eliminazione
di Gauss, non con lo sviluppo lungo una riga che fa il programma.

La definitezza di un'hessiana (H_f(P) \\succ 0 e le altre tre scritture) non
si decide con i minori, come fa il programma, ma con i coefficienti del
polinomio caratteristico, calcolati con le tracce delle potenze della
matrice (definite()): né un determinante né un minore.

La risposta alla richiesta dei punti critici si confronta con il gradiente
punto per punto, nei due versi (dove dice "critico" il gradiente deve essere
nullo, e dove il gradiente è nullo deve dirlo): su una griglia di razionali,
nei punti che la risposta elenca, in qualche punto di ogni retta "x = r" che
la risposta dichiara e, in modalità fuzz, nei punti critici che la funzione
ha per come è stata costruita. Una ricerca con il metodo di
Newton cerca punti critici che la risposta non dice: uno trovato così è
approssimato, e dà una riga SUSPECT, non BUG. Non è una dimostrazione che
l'elenco sia completo.

Righe stampate:

  BUG: il programma ha dimostrato (o risolto con una risposta sbagliata) un
       enunciato che l'oracolo trova falso, o ha dichiarato falso un
       enunciato vero; il verificatore ha accettato un certificato che
       afferma il falso; o il programma ha stampato la riga ATTENZIONE, che
       conta i passi scartati dai suoi controlli.
  SUSPECT: l'oracolo non riesce a decidere un enunciato su cui il programma
       si è pronunciato, o un certificato accettato.
  DATA: in modalità check, un enunciato di un file di veri che l'oracolo
        trova falso, o il contrario.
  VACUOUS (solo contati): un certificato accettato i cui termini, nei punti
        provati, non esistono mai (la radice di 1 - 1, ln(-x^2)): il teorema
        non dice nulla, ed è vero.
  NOTE (solo con la variabile d'ambiente FUZZ_NOTES): un enunciato vero non
        dimostrato, o uno falso non smentito. Non è un errore.

I file prodotti vanno nella cartella di lavoro, mai nel repository.
"""
import math
import os
import random
import re
import subprocess
import sys
from fractions import Fraction as Fr

sys.setrecursionlimit(20000)


class Undefined(Exception):
    """Un'espressione senza valore in un punto."""


class Irregular(Undefined):
    """Un'espressione che in un punto ha un valore ma non è regolare: un
    radicando, o l'argomento di un valore assoluto, che vale zero; l'argomento
    di un arcoseno o di un arcocoseno che vale 1 o -1. Lì le regole non danno
    la derivata, che può esistere lo stesso."""


class Overflow(Undefined):
    """Un conto uscito dai float (e^{x^4} con x = 9): l'espressione lì può
    essere definita, ma l'oracolo non sa quanto vale. Chi sceglie i punti a
    caso ne prova un altro, come per Undefined; chi guarda un punto dato non
    decide."""


class Unreadable(Exception):
    """Un testo che il lettore di questo file non tratta."""


# ----------------------------------------------------------------------------
# 1. Alberi: ("num", q), ("var", c), ("e",), ("pi",), ("add" | "sub" | "mul" |
#    "div" | "pow", a, b), ("neg", a), ("fn", nome, a), ("root", n, a),
#    ("op", k) per l'operando numero k di un enunciato.
# ----------------------------------------------------------------------------

def num(q):
    return ("num", Fr(q))


ZERO, ONE = num(0), num(1)


def is_num(e, q=None):
    return e[0] == "num" and (q is None or e[1] == q)


def add(a, b):
    return b if is_num(a, 0) else a if is_num(b, 0) else ("add", a, b)


def sub(a, b):
    return a if is_num(b, 0) else ("neg", b) if is_num(a, 0) else ("sub", a, b)


def mul(a, b):
    if is_num(a, 0) or is_num(b, 0):
        return ZERO
    return b if is_num(a, 1) else a if is_num(b, 1) else ("mul", a, b)


def div(a, b):
    return ZERO if is_num(a, 0) else ("div", a, b)


def fn(name, a):
    return ("fn", name, a)


def uses(e, v):
    return e == ("var", v) or any(isinstance(c, tuple) and uses(c, v) for c in e[1:])


def diff(e, v):
    """La derivata di e rispetto alla lettera v: le regole, scritte qui."""
    op = e[0]
    if not uses(e, v):
        return ZERO
    if op == "var":
        return ONE
    if op == "neg":
        return ("neg", diff(e[1], v))
    if op in ("add", "sub"):
        return (add if op == "add" else sub)(diff(e[1], v), diff(e[2], v))
    if op == "mul":
        return add(mul(diff(e[1], v), e[2]), mul(e[1], diff(e[2], v)))
    if op == "div":
        return div(sub(mul(diff(e[1], v), e[2]), mul(e[1], diff(e[2], v))), ("pow", e[2], num(2)))
    if op == "pow":
        a, b = e[1], e[2]
        if is_num(b) and b[1].denominator == 1:
            # (a^n)' = n a^(n-1) a', con a^0 scritto 1 e a^1 scritto a.
            lower = ONE if b[1] == 1 else a if b[1] == 2 else ("pow", a, num(b[1] - 1))
            return mul(mul(b, lower), diff(a, v))
        # a^b = exp(b ln a): a^b (b' ln a + b a' / a), con a > 0.
        return mul(e, add(mul(diff(b, v), fn("ln", a)), div(mul(b, diff(a, v)), a)))
    if op == "root":
        return div(mul(e, diff(e[2], v)), mul(num(e[1]), e[2]))
    a, da = e[2], diff(e[2], v)
    name = e[1]
    if name == "exp":
        return mul(e, da)
    if name == "ln":
        return div(da, a)
    if name == "sin":
        return mul(fn("cos", a), da)
    if name == "cos":
        return ("neg", mul(fn("sin", a), da))
    if name == "tan":
        return div(da, ("pow", fn("cos", a), num(2)))
    if name == "cot":
        return ("neg", div(da, ("pow", fn("sin", a), num(2))))
    if name == "atan":
        return div(da, add(ONE, ("pow", a, num(2))))
    if name in ("asin", "acos"):
        slope = div(da, ("root", 2, sub(ONE, ("pow", a, num(2)))))
        return slope if name == "asin" else ("neg", slope)
    if name == "abs":
        return div(mul(da, a), e)
    raise Unreadable("derivata di " + name)


def subst(e, mapping):
    """e con ogni variabile di mapping sostituita dal suo albero, tutte insieme."""
    if e[0] == "var":
        return mapping.get(e[1], e)
    return tuple(subst(c, mapping) if isinstance(c, tuple) else c for c in e)


def exact_root(q, n):
    """La radice n-esima esatta del razionale q >= 0, o None."""
    def integer_root(m):
        r = round(m ** (1.0 / n))
        for candidate in (r - 1, r, r + 1):
            if candidate >= 0 and candidate ** n == m:
                return candidate
        return None
    top, bottom = integer_root(q.numerator), integer_root(q.denominator)
    return None if top is None or bottom is None else Fr(top, bottom)


def determinant(values, n):
    """Il determinante della matrice n x n dei numeri values, scritti per
    righe. Eliminazione di Gauss con scambio di righe: esatta sulle frazioni,
    e un conto diverso dallo sviluppo lungo una riga che fa il programma."""
    rows = [list(values[r * n:(r + 1) * n]) for r in range(n)]
    result = Fr(1)
    for k in range(n):
        pivot = max(range(k, n), key=lambda r: abs(rows[r][k]))
        if rows[pivot][k] == 0:
            return Fr(0) if all(isinstance(v, Fr) for v in values) else 0.0
        if pivot != k:
            rows[k], rows[pivot] = rows[pivot], rows[k]
            result = -result
        result = result * rows[k][k]
        for r in range(k + 1, n):
            factor = rows[r][k] / rows[k][k]
            for c in range(k, n):
                rows[r][c] = rows[r][c] - factor * rows[k][c]
    return result


def val(e, env):
    """Il valore di e, esatto (Fraction) quando si può, altrimenti float.
    Solleva Undefined dove e non è definita (un divisore nullo, un logaritmo
    non positivo, un radicando negativo, una base negativa con un esponente
    non intero, un arcoseno fuori da [-1, 1]), e Irregular dove è definita ma
    non regolare (un radicando, una base così o l'argomento di un valore
    assoluto che vale zero, un arcoseno in 1 o in -1)."""
    op = e[0]
    try:
        if op == "num":
            return e[1]
        if op == "var":
            return env[e[1]]
        if op == "e":
            return math.e
        if op == "pi":
            return math.pi
        if op == "neg":
            return -val(e[1], env)
        if op == "det":
            return determinant([val(c, env) for c in e[2:]], e[1])
        if op == "root":
            a = val(e[2], env)
            if a == 0:
                raise Irregular
            if e[1] % 2 and a < 0:
                return -float(-a) ** (1.0 / e[1])
            if a < 0:
                raise Undefined
            root = exact_root(a, e[1]) if isinstance(a, Fr) else None
            return root if root is not None else float(a) ** (1.0 / e[1])
        if op == "fn":
            a, name = val(e[2], env), e[1]
            if name == "abs":
                if a == 0:
                    raise Irregular
                return abs(a)
            # I valori che sono razionali restano esatti: e^0, ln 1, e in zero
            # il seno, la tangente, l'arcotangente, l'arcoseno e il coseno.
            # Così il segno di una derivata di e^{g} in un punto in cui g = 0
            # si legge da una frazione, non da un float.
            if isinstance(a, Fr) and a == (1 if name == "ln" else 0) and name in ("exp", "ln", "sin", "tan", "atan", "asin", "cos"):
                return Fr(1 if name in ("exp", "cos") else 0)
            a = float(a)
            if name == "exp":
                return math.exp(a)
            if name == "ln":
                if a <= 0:
                    raise Undefined
                return math.log(a)
            if name in ("asin", "acos") and abs(a) >= 1:
                # In 1 e in -1 l'arcoseno e l'arcocoseno hanno un valore, ma non
                # sono regolari, come una radice nello zero: lì non si decide.
                # Fuori non ci sono.
                raise Irregular if abs(a) == 1 else Undefined
            if name in ("tan", "cot") and abs(math.cos(a) if name == "tan" else math.sin(a)) < 1e-12:
                raise Undefined
            return {"sin": math.sin, "cos": math.cos, "tan": math.tan, "cot": lambda t: math.cos(t) / math.sin(t), "atan": math.atan,
                    "asin": math.asin, "acos": math.acos}[name](a)
        a, b = val(e[1], env), val(e[2], env)
        if op == "add":
            return a + b
        if op == "sub":
            return a - b
        if op == "mul":
            # Zero per un numero qualsiasi è zero, esatto: i due fattori sono
            # già stati calcolati, quindi sono definiti. Così e^{g} (2x - 1) in
            # x = 1/2 vale la frazione 0, non il float 0.0, e il confronto con
            # zero si decide.
            if (isinstance(a, Fr) and a == 0) or (isinstance(b, Fr) and b == 0):
                return Fr(0)
            return a * b
        if op == "div":
            if b == 0:
                raise Undefined
            return a / b
        if op == "pow":
            if isinstance(b, Fr) and b.denominator == 1:
                if b == 0:
                    return Fr(1)        # a^0 è 1 anche per a = 0, come per il programma
                if a == 0 and b < 0:
                    raise Undefined
                return a ** int(b) if abs(b) < 400 else float(a) ** int(b)
            if a == 0:
                raise Irregular
            if a < 0:
                raise Undefined
            return float(a) ** float(b)
    except OverflowError:
        raise Overflow
    except (ZeroDivisionError, ValueError, KeyError):
        raise Undefined
    raise Unreadable(op)


def finite(value):
    """False per un float infinito o non numero: un conto uscito dai float."""
    return isinstance(value, Fr) or not (value != value or math.isinf(value))


def same(left, right):
    """True se i due numeri sono uguali, False se diversi, None se non si sa."""
    if isinstance(left, Fr) and isinstance(right, Fr):
        return left == right
    try:
        left, right = float(left), float(right)
    except OverflowError:
        return None
    if not finite(left) or not finite(right):
        return None
    gap, size = abs(left - right), max(1.0, abs(left), abs(right))
    return True if gap <= 1e-9 * size else False if gap > 1e-6 * size else None


def compare(left, right, relation):
    """True se vale "left relation right" tra due numeri, False se non vale,
    None se non si sa. Con le frazioni il segno della differenza è esatto.
    Con i float si sa solo quando i due numeri sono di sicuro diversi: due
    float uguali entro 10^-9 possono essere lo stesso numero o no, e un
    confronto diverso dall'uguaglianza non si decide."""
    equal = same(left, right)
    if relation == "=":
        return equal
    if isinstance(left, Fr) and isinstance(right, Fr):
        side = (left > right) - (left < right)
    elif equal is not False:
        return None
    else:
        side = 1 if float(left) > float(right) else -1
    return {"!=": side != 0, "<": side < 0, "<=": side <= 0, ">": side > 0, ">=": side >= 0}[relation]


def definite(values, n, kind):
    """True, False o None: la matrice simmetrica di lato n con gli elementi
    values, per righe, è definita positiva ("succ"), definita negativa
    ("prec"), semidefinita positiva ("succeq") o negativa ("preceq").

    Il programma lo decide con i minori principali (il criterio di
    Sylvester). Qui, apposta, con un altro conto: i coefficienti del polinomio
    caratteristico. Una matrice simmetrica reale ha tutti gli autovalori
    reali, e

        det(t I - A) = t^n + c_1 t^(n-1) + ... + c_n,   c_k = (-1)^k e_k,

    dove e_k è la somma dei prodotti a k a k degli autovalori. Se gli e_k
    sono tutti positivi il polinomio non ha radici minori o uguali a zero
    (in t = -s, con s >= 0, vale (-1)^n (s^n + e_1 s^(n-1) + ... + e_n), che
    non è zero): gli autovalori sono tutti positivi, e viceversa. Con e_k >=
    0 non ha radici negative: semidefinita. Per le negative lo stesso con -A,
    i cui e_k sono (-1)^k e_k, cioè i c_k.

    I c_k si calcolano senza determinanti, con il metodo di Faddeev e
    LeVerrier: M_1 = I, c_k = -traccia(A M_k) / k, M_(k+1) = A M_k + c_k I.
    Con le frazioni il conto è esatto; con i float un coefficiente troppo
    vicino a zero non si decide. Una matrice che non risulta simmetrica non
    si decide (l'hessiana di una funzione regolare lo è sempre)."""
    a = [list(values[r * n:(r + 1) * n]) for r in range(n)]
    for r in range(n):
        for c in range(r):
            if same(a[r][c], a[c][r]) is not True:
                return None
    m = [[Fr(int(r == c)) for c in range(n)] for r in range(n)]
    wanted = []
    for k in range(1, n + 1):
        am = [[sum(a[r][t] * m[t][c] for t in range(n)) for c in range(n)] for r in range(n)]
        c_k = -sum(am[r][r] for r in range(n)) / k
        m = [[am[r][c] + (c_k if r == c else 0) for c in range(n)] for r in range(n)]
        wanted.append((-1) ** k * c_k if kind in ("succ", "succeq") else c_k)
    answers = [compare(w, Fr(0), ">" if kind in ("succ", "prec") else ">=") for w in wanted]
    return False if False in answers else None if None in answers else True


def letters(e, out=None):
    out = set() if out is None else out
    if e[0] == "var":
        out.add(e[1])
    for c in e[1:]:
        if isinstance(c, tuple):
            letters(c, out)
    return out


# ----------------------------------------------------------------------------
# 2. Scrivere in LaTeX, con tutte le parentesi e i prodotti scritti.
# ----------------------------------------------------------------------------

def tex(e):
    op = e[0]
    if op == "num":
        q = e[1]
        body = str(abs(q.numerator)) if q.denominator == 1 else "\\frac{%d}{%d}" % (abs(q.numerator), q.denominator)
        return body if q >= 0 else "\\left(-%s\\right)" % body
    if op == "var":
        return e[1]
    if op == "e":
        return "e"
    if op == "pi":
        return "\\pi"
    if op == "neg":
        return "\\left(-%s\\right)" % wrapped(e[1])
    if op == "add":
        return "%s + %s" % (tex(e[1]), tex(e[2]))
    if op == "sub":
        return "%s - %s" % (tex(e[1]), wrapped(e[2]))
    if op == "mul":
        return "%s \\cdot %s" % (wrapped(e[1]), wrapped(e[2]))
    if op == "div":
        return "\\frac{%s}{%s}" % (tex(e[1]), tex(e[2]))
    if op == "pow":
        base = tex(e[1]) if e[1][0] == "var" else "\\left(%s\\right)" % tex(e[1])
        return "%s^{%s}" % (base, tex(e[2]))
    if op == "root":
        return ("\\sqrt{%s}" if e[1] == 2 else "\\sqrt[%d]{%%s}" % e[1]) % tex(e[2])
    if e[1] == "exp":
        return "e^{%s}" % tex(e[2])
    if e[1] == "abs":
        return "\\left|%s\\right|" % tex(e[2])
    name = {"atan": "arctan", "asin": "arcsin", "acos": "arccos"}.get(e[1], e[1])
    return "\\%s\\left(%s\\right)" % (name, tex(e[2]))


def wrapped(e):
    return "\\left(%s\\right)" % tex(e) if e[0] in ("add", "sub") else tex(e)


# ----------------------------------------------------------------------------
# 3. Leggere il LaTeX: le espressioni del capitolo 9, e gli enunciati del 34.
# ----------------------------------------------------------------------------

FUNCTIONS = {"sin": "sin", "cos": "cos", "tan": "tan", "cot": "cot", "arctan": "atan", "arcsin": "asin", "arccos": "acos", "ln": "ln"}
SKIPPED = {"displaystyle", "textstyle", "quad", "qquad", "dfrac", "tfrac"}
RELATIONS = {"neq": "!=", "ne": "!=", "le": "<=", "leq": "<=", "ge": ">=", "geq": ">="}
DEFINITE = ("succ", "prec", "succeq", "preceq")     # definita positiva, negativa, semidefinita positiva, negativa


def tokens(text):
    out, i = [], 0
    text = text.replace("\\dfrac", "\\frac").replace("\\tfrac", "\\frac")
    while i < len(text):
        c = text[i]
        if c.isspace():
            i += 1
        elif c.isdigit():
            j = i
            while j < len(text) and text[j].isdigit():
                j += 1
            out.append(("NUM", text[i:j]))
            i = j
        elif c.isalpha():
            out.append(("LET", c))
            i += 1
        elif c == "\\":
            if text[i + 1:i + 2] == "\\":
                out.append(("ROW", ""))
                i += 2
                continue
            if text[i + 1:i + 2] in ("{", "}"):
                out.append(("SET", text[i + 1]))      # \{ e \}: le graffe di un insieme
                i += 2
                continue
            if not text[i + 1:i + 2].isalpha():
                i += 2      # \, \; \! e simili: spazi
                continue
            j = i + 1
            while j < len(text) and text[j].isalpha():
                j += 1
            name, i = text[i + 1:j], j
            if name in ("left", "right"):
                while text[i].isspace():
                    i += 1
                out.append(("OPEN" if text[i] in "([" else "CLOSE" if text[i] in ")]" else "BAR", text[i]))
                i += 1
            elif name in ("begin", "end"):
                close = text.index("}", i)
                out.append((name.upper(), text[i + 1:close]))
                i = close + 1
            elif name not in SKIPPED:
                out.append(("CMD", name))
        else:
            out.append(("OPEN", c) if c == "(" else ("CLOSE", c) if c == ")" else ("BAR", c) if c == "|" else ("CH", c))
            i += 1
    out.append(("END", ""))
    return out


class Reader:
    """Discesa ricorsiva sulla grammatica dei commenti dei capitoli 9 e 34."""

    def __init__(self, text):
        self.t, self.i = tokens(text), 0
        self.functions, self.operands, self.bars = {}, [], 0

    def peek(self):
        return self.t[self.i]

    def take(self):
        self.i += 1
        return self.t[self.i - 1]

    def at(self, kind, value=None):
        return self.peek()[0] == kind and (value is None or self.peek()[1] == value)

    def expect(self, kind, value=None):
        if not self.at(kind, value):
            raise Unreadable("atteso %s %s, trovato %s" % (kind, value, self.peek()))
        return self.take()

    def group(self):
        if self.at("CH", "{"):
            self.take()
            e = self.expr()
            self.expect("CH", "}")
            return e
        if self.at("NUM") and len(self.peek()[1]) > 1:      # \frac12: una cifra per volta
            digits = self.peek()[1]
            self.t[self.i] = ("NUM", digits[1:])
            return num(int(digits[0]))
        return self.primary()

    def starts_factor(self):
        kind, value = self.peek()
        if kind == "BAR":
            return self.bars == 0
        return kind in ("NUM", "LET", "OPEN") or (kind == "CMD" and (value in FUNCTIONS or value in ("frac", "sqrt", "pi", "log")))

    def point(self, f):
        """Dopo una funzione dichiarata o una sua derivata: None, o le coordinate."""
        if not self.at("OPEN"):
            return None
        self.take()
        coordinates = []
        for k in range(len(f[0])):
            if k:
                self.expect("CH", ",")
            coordinates.append(self.expr())
        self.expect("CLOSE")
        return None if coordinates == [("var", v) for v in f[0]] else coordinates

    def operand(self, name, body, by, point):
        self.operands.append((name, body, by, point))
        return ("op", len(self.operands) - 1)

    def partial(self):
        """\\frac{\\partial^k f}{\\partial x \\partial y} [punto], o lo stesso segno davanti a un fattore."""
        self.expect("CH", "{")
        self.expect("CMD", "partial")
        order = 1
        if self.at("CH", "^"):
            self.take()
            order = int(val(self.group(), {}))
        name = self.take()[1] if self.at("LET") else None
        self.expect("CH", "}")
        self.expect("CH", "{")
        written = []
        while self.at("CMD", "partial"):
            self.take()
            letter, times = self.expect("LET")[1], 1
            if self.at("CH", "^"):
                self.take()
                times = int(val(self.group(), {}))
            written += [letter] * times
        self.expect("CH", "}")
        if len(written) != order or (name is not None and name not in self.functions):
            raise Unreadable("segno di derivata parziale")
        by = written[::-1]      # si deriva per prima rispetto all'ultima lettera scritta
        if name is None:
            # La funzione è un solo fattore; un altro fattore accanto è una scrittura ambigua.
            body = self.factor()
            if self.starts_factor() or self.at("CMD", "cdot") or self.at("CMD", "times") or self.at("CH", "/"):
                raise Unreadable("un fattore accanto alla funzione di un segno di derivata parziale")
            return self.operand(None, body, by, None)
        return self.operand(name, None, by, self.point(self.functions[name]))

    def hessian_ahead(self):
        """True se da qui comincia H_{f} o H_f, con H che non è una funzione dichiarata."""
        return self.at("LET", "H") and "H" not in self.functions and self.t[self.i + 1] == ("CH", "_")

    def hessian(self):
        """H_{f} o H_f [punto]: ritorna il nome di f e le derivate seconde, per righe."""
        self.i += 2
        braces = self.at("CH", "{")
        if braces:
            self.take()
        name = self.expect("LET")[1]
        if braces:
            self.expect("CH", "}")
        f = self.functions[name]
        where = self.point(f)
        return name, [self.operand(name, None, [r, c], where) for r in f[0] for c in f[0]]

    def table(self):
        """Dopo \\begin{...}: gli elementi per righe fino a \\end{...}; ritorna (elementi, righe, colonne)."""
        rows = [[]]
        while True:
            rows[-1].append(self.expr())
            if self.at("CH", "&"):
                self.take()
            elif self.at("ROW"):
                self.take()
                if self.at("END"):
                    break
                rows.append([])
            else:
                break
        self.expect("END")
        if any(len(r) != len(rows[0]) for r in rows):
            raise Unreadable("righe di una tabella")
        return [e for r in rows for e in r], len(rows), len(rows[0])

    def det(self, entries, rows, columns):
        """Il determinante di una tabella quadrata: ("det", lato, elementi...)."""
        if rows != columns or rows > 4:
            raise Unreadable("determinante di una tabella non quadrata, o di lato oltre 4")
        return ("det", rows) + tuple(entries)

    def primary(self):
        kind, value = self.take()
        if kind == "NUM":
            return num(int(value))
        if kind == "BEGIN":
            if value != "vmatrix":
                raise Unreadable("una tabella dentro un'espressione")
            return self.det(*self.table())
        if kind == "CMD" and value == "det":
            if self.hessian_ahead():
                name, entries = self.hessian()
                return self.det(entries, len(self.functions[name][0]), len(self.functions[name][0]))
            kind, value = self.take()
            if kind != "BEGIN" or value not in ("pmatrix", "bmatrix", "matrix"):
                raise Unreadable("\\det seguito da altro")
            return self.det(*self.table())
        if kind == "LET":
            if value in self.functions:
                by = []
                if self.at("CH", "_"):
                    self.take()
                    if self.at("CH", "{"):
                        self.take()
                        while self.at("LET"):
                            by.append(self.take()[1])
                        self.expect("CH", "}")
                    else:
                        by.append(self.expect("LET")[1])
                return self.operand(value, None, by, self.point(self.functions[value]))
            return ("e",) if value == "e" else ("var", value)
        if kind == "OPEN":
            e = self.expr()
            self.expect("CLOSE")
            return e
        if kind == "BAR":
            self.bars += 1
            e = self.expr()
            self.bars -= 1
            self.expect("BAR")
            return fn("abs", e)
        if kind != "CMD":
            raise Unreadable("inizio di espressione: %s %s" % (kind, value))
        if value == "frac":
            if self.t[self.i + 1] == ("CMD", "partial"):
                return self.partial()
            a = self.group()
            return ("div", a, self.group())
        if value == "sqrt":
            index = 2
            if self.at("CH", "["):
                self.take()
                index = int(val(self.expr(), {}))
                self.expect("CH", "]")
            return ("root", index, self.group())
        if value == "pi":
            return ("pi",)
        if value in FUNCTIONS or value == "log":
            exponent = base = None
            while self.at("CH", "^") or (value == "log" and self.at("CH", "_")):
                if self.take()[1] == "^":
                    exponent = self.group()
                else:
                    base = self.group()
            if self.at("OPEN"):
                self.take()
                argument = self.expr()
                self.expect("CLOSE")
            else:
                # Un fattore solo; ma un numero seguito da lettere è il loro prodotto.
                number = self.at("NUM")
                argument = self.factor()
                while number and self.at("LET") and self.peek()[1] not in self.functions:
                    argument = ("mul", argument, self.factor())
            call = fn(FUNCTIONS[value], argument) if value != "log" else ("div", fn("ln", argument), fn("ln", base))
            return call if exponent is None else ("pow", call, exponent)
        raise Unreadable("comando " + value)

    def factor(self):
        e = self.primary()
        while self.at("CH", "^"):
            self.take()
            exponent = self.group()
            e = fn("exp", exponent) if e == ("e",) else ("pow", e, exponent)
        return e

    def term(self):
        e = self.factor()
        while True:
            if self.at("CMD", "cdot") or self.at("CMD", "times"):
                self.take()
                e = ("mul", e, self.factor())
            elif self.at("CH", "/"):
                self.take()
                e = ("div", e, self.factor())
            elif self.starts_factor():
                e = ("mul", e, self.factor())
            else:
                return e

    def expr(self):
        if self.at("CH", "-"):
            self.take()
            e = ("neg", self.term())
        else:
            if self.at("CH", "+"):
                self.take()
            e = self.term()
        while self.at("CH", "+") or self.at("CH", "-"):
            e = ("add" if self.take()[1] == "+" else "sub", e, self.term())
        return e

    def components(self, shape, count, columns):
        """Il secondo membro di un gradiente (una lista, o 0) o di un'hessiana (una matrice)."""
        if shape == "gradient" and not self.at("OPEN"):
            zero = self.expr()
            if not is_num(zero, 0):
                raise Unreadable("gradiente uguale a un'espressione")
            return [ZERO] * count
        self.expect("OPEN") if shape == "gradient" else self.expect("BEGIN")
        out = []
        for k in range(count):
            if k % columns:
                self.expect("CH", "," if shape == "gradient" else "&")
            elif k:
                self.expect("ROW")
            out.append(self.expr())
        if shape == "gradient":
            self.expect("CLOSE")
        else:
            if self.at("ROW"):
                self.take()
            self.expect("END")
        return out

    def relation(self):
        """Il segno tra i due membri di una tesi, se c'è: "=", "!=", "<", "<=", ">", ">="."""
        if self.at("CH", "=") or self.at("CH", "<") or self.at("CH", ">"):
            return self.take()[1]
        if self.at("CMD") and self.peek()[1] in RELATIONS:
            return RELATIONS[self.take()[1]]
        return None

    def critical(self):
        """\\{(x, y) \\mid \\nabla f(x, y) = (0, 0)\\}: ritorna le componenti del gradiente di f."""
        self.expect("SET", "{")
        self.expect("OPEN")
        unknowns = [self.expect("LET")[1]]
        while self.at("CH", ","):
            self.take()
            unknowns.append(self.expect("LET")[1])
        self.expect("CLOSE")
        self.expect("CMD", "mid")
        self.expect("CMD", "nabla")
        name = self.expect("LET")[1]
        f = self.functions[name]
        if unknowns != f[0] or self.point(f) is not None:
            raise Unreadable("le incognite non sono le variabili della funzione")
        self.expect("CH", "=")
        zeros = self.components("gradient", len(unknowns), len(unknowns))
        if any(not is_num(z, 0) for z in zeros):
            raise Unreadable("gradiente uguale a qualcosa che non è zero")
        self.expect("SET", "}")
        return [self.operand(name, None, [v], None) for v in unknowns]

    def statement(self):
        """Ritorna (richiesta, tesi): le tesi sono terne (primo membro, secondo
        membro o None, relazione). La richiesta è None, "value", "gradient",
        "hessian" o "critical" (i punti critici: le tesi sono le componenti
        del gradiente). La definitezza di un'hessiana è una tesi sola: il
        primo membro è ("hess", lato, elementi per righe...), il secondo zero,
        la relazione una di DEFINITE."""
        # Le definizioni, se dopo c'è \implies.
        if ("CMD", "implies") in self.t:
            while True:
                name = self.expect("LET")[1]
                self.expect("OPEN")
                variables = [self.expect("LET")[1]]
                while self.at("CH", ","):
                    self.take()
                    variables.append(self.expect("LET")[1])
                self.expect("CLOSE")
                self.expect("CH", "=")
                body = self.expr()
                self.functions[name] = (variables, body)
                if not self.at("CH", ","):
                    break
                self.take()
            self.expect("CMD", "implies")
        claims, request = [], None
        if self.at("SET", "{"):
            claims, request = [(l, None, "=") for l in self.critical()], "critical"
        while request != "critical":
            shape, left = "value", None
            if self.at("CMD", "nabla"):
                self.take()
                shape = "gradient"
                name = self.expect("LET")[1]
                f = self.functions[name]
                where, n = self.point(f), len(f[0])
                left = [self.operand(name, None, [v], where) for v in f[0]]
            elif self.hessian_ahead():
                shape = "hessian"
                name, left = self.hessian()
                n = len(self.functions[name][0])
                if self.at("CMD") and self.peek()[1] in DEFINITE:
                    kind = self.take()[1]
                    if not is_num(self.expr(), 0):
                        raise Unreadable("la definitezza rispetto a qualcosa che non è zero")
                    claims.append((("hess", n) + tuple(left), ZERO, kind))
                    if not self.at("CMD", "land"):
                        break
                    self.take()
                    continue
            else:
                left, n = [self.expr()], 1
            sign = self.relation()
            if sign is None:
                request = shape
                claims += [(l, None, "=") for l in left]
                break
            if shape != "value" and sign != "=":
                raise Unreadable("un gradiente o un'hessiana in un confronto")
            right = [self.expr()] if shape == "value" else self.components(shape, len(left), n)
            claims += [(l, r, sign) for l, r in zip(left, right)]
            if not self.at("CMD", "land"):
                break
            self.take()
        self.expect("END")
        return request, claims


# ----------------------------------------------------------------------------
# 4. L'oracolo: gli operandi, poi le tesi nei punti a caso.
# ----------------------------------------------------------------------------

def named(e, out):
    """Aggiunge a out gli operandi che compaiono in e."""
    if e[0] == "op":
        out.add(e[1])
    for c in e[1:]:
        if isinstance(c, tuple):
            named(c, out)
    return out


def resolve(reader):
    """Per ogni operando, nell'ordine: (funzione nel punto, valore), alberi
    senza operandi; e dove l'operando esiste, come elenco di alberi che devono
    essere tutti regolari: la sua funzione nel suo punto e, per ogni operando
    che la sua funzione o il suo punto nominano, gli alberi di quello con il
    punto di questo al posto delle variabili. Con g = ln x e f = g_x + x, la
    funzione f è 1/x + x solo dove g esiste: f in (-1, 0) non c'è, anche se
    lì 1/x + x ha un valore."""
    values, domains = [], []

    def placed(e):
        if e[0] == "op":
            return values[e[1]][1]
        return tuple(placed(c) if isinstance(c, tuple) else c for c in e)

    for name, body, by, where in reader.operands:
        variables, body = reader.functions[name] if name is not None else ([], body)
        inner = named(body, set())
        for coordinate in where or []:
            named(coordinate, inner)
        body = placed(body)
        derivative = body
        for letter in by:
            derivative = diff(derivative, letter)
        mapping = dict(zip(variables, [placed(c) for c in where])) if where else {}
        values.append((subst(body, mapping), subst(derivative, mapping)))
        domains.append([values[-1][0]] + [subst(t, mapping) for j in sorted(inner) for t in domains[j]])
    return values, placed, domains


def sample(rnd, names):
    """Un punto razionale a caso: metà delle volte con tutte le coordinate piccole,
    perché i domini come 1 - x - y > 0 con x, y > 0 non restino senza punti."""
    low, high = rnd.choice([(1, 7), (8, 40)])
    return {name: Fr(rnd.randint(-9, 9), rnd.randint(low, high)) for name in names}


# In quanti punti a caso l'oracolo guarda una tesi prima di dirla vera. Erano
# 8, e non bastavano: g(u, v) = sqrt(u^2) v con la tesi g_u = v è falsa solo
# per u < 0, e otto punti tutti con u > 0 (o con v = 0) sono capitati 24
# volte su 3000 semi provati. Così l'oracolo diceva "vero" di un enunciato che il
# programma, con ragione, dichiarava falso (una riga BUG sul seme 65004); e
# allo stesso modo avrebbe potuto non vedere un falso dimostrato.
POINTS = 24


def truth(reader, claims, rnd):
    """True, False o None per un enunciato già letto, con tutte le tesi complete.
    Ogni tesi è affermata dove sono regolari le funzioni dei suoi operandi, e
    solo loro: si guarda in punti a caso scelti così, una tesi alla volta."""
    values, placed, domains = resolve(reader)
    unsure = False
    for l, r, relation in claims:
        mine = sorted(named(l, named(r, set())))
        exist = [t for k in mine for t in domains[k]] + [values[k][1] for k in mine]
        left, right = placed(l), placed(r)
        names = sorted(set().union(letters(left), letters(right), *[letters(t) for t in exist]))
        found = 0
        for attempt in range(400):
            env = sample(rnd, names)
            try:
                for tree in exist:
                    val(tree, env)
                pair = [val(c, env) for c in left[2:]] if relation in DEFINITE else [val(left, env), val(right, env)]
            except Undefined as problem:
                if not names:
                    # Un punto scritto in cui una funzione non è definita: falso. Dove
                    # è solo non regolare la derivata può esistere, e dove il conto
                    # esce dai float non si sa: l'oracolo non decide.
                    return None if isinstance(problem, (Irregular, Overflow)) else False
                continue
            if names and not all(finite(v) for v in pair):
                continue        # un punto in cui i float non bastano (e^{3 x^2 y^2 z} con x = 9): se ne prova un altro
            found += 1
            holds = definite(pair, left[1], relation) if relation in DEFINITE else compare(pair[0], pair[1], relation)
            if holds is False:
                return False
            unsure = unsure or holds is None
            if found == POINTS or not names:
                break
        unsure = unsure or found < (2 if names else 1)
    return None if unsure else True


def answer_wrong(reader, claims, answer, rnd):
    """True se la risposta a una richiesta è di sicuro sbagliata, False se è confermata, None se non si sa."""
    shape = "hessian" if "pmatrix" in answer else "gradient" if answer.strip().startswith("\\left(") and "," in answer else "value"
    written = Reader(answer)
    try:
        parts = [written.expr()] if shape == "value" else written.components(shape, len(claims), int(round(len(claims) ** 0.5)) if shape == "hessian" else len(claims))
        written.expect("END")
    except (Unreadable, Undefined, IndexError, ValueError):
        # Una lista di una sola componente, o una scrittura che questo lettore non tratta.
        return None
    if len(parts) != len(claims):
        return None
    verdict = truth(reader, [(claim[0], part, "=") for claim, part in zip(claims, parts)], rnd)
    return None if verdict is None else not verdict


# I punti in cui la risposta sui punti critici viene confrontata con il
# gradiente: una griglia di razionali piccoli, la stessa per ogni richiesta.
GRID = sorted({Fr(n, d) for d in (1, 2, 3) for n in range(-4 * d, 4 * d + 1)})

# I valori dell'incognita libera in cui si guarda una retta di punti critici
# che la risposta dichiara (critical_wrong()).
LINE = [Fr(-3, 2), Fr(0), Fr(1, 3), Fr(2)]


def condition(reader):
    """Legge la condizione che il programma scrive per dire dove il gradiente
    è nullo: confronti tra espressioni, uniti da \\land e \\lor, con le
    parentesi. Ritorna ("or" | "and", a, b) o ("cmp", relazione, a, b)."""
    def atom():
        if reader.at("OPEN"):
            mark = reader.i
            reader.take()
            try:
                inner = disjunction()
                reader.expect("CLOSE")
                return inner
            except Unreadable:
                reader.i = mark     # era la parentesi di un'espressione
        left = reader.expr()
        sign = reader.relation()
        if sign is None:
            raise Unreadable("condizione senza relazione")
        middle = reader.expr()
        first = ("cmp", sign, left, middle)
        again = reader.relation()       # una catena, 0 < x < 1: i due confronti insieme
        return first if again is None else ("and", first, ("cmp", again, middle, reader.expr()))

    def conjunction():
        e = atom()
        while reader.at("CMD", "land"):
            reader.take()
            e = ("and", e, atom())
        return e

    def disjunction():
        e = conjunction()
        while reader.at("CMD", "lor"):
            reader.take()
            e = ("or", e, conjunction())
        return e

    e = disjunction()
    reader.expect("END")
    return e


def satisfied(c, env):
    """True, False o None: la condizione c nel punto env. Un'uguaglianza tra
    float vale se la differenza è sotto 10^-7, non vale sopra 10^-4."""
    if c[0] in ("and", "or"):
        a, b = satisfied(c[1], env), satisfied(c[2], env)
        decisive = c[0] == "or"
        return decisive if a is decisive or b is decisive else None if a is None or b is None else not decisive
    try:
        left, right = val(c[2], env), val(c[3], env)
    except Overflow:
        return None
    except Undefined:
        return False
    if isinstance(left, Fr) and isinstance(right, Fr):
        return compare(left, right, c[1])
    gap = float(left) - float(right)
    if abs(gap) <= 1e-7:
        return {"=": True, "<=": True, ">=": True}.get(c[1])
    if abs(gap) <= 1e-4:
        return None
    return {"=": False, "!=": True, "<": gap < 0, "<=": gap < 0, ">": gap > 0, ">=": gap > 0}[c[1]]


def listed(c, unknowns):
    """I punti che la condizione c elenca, come dizionari lettera -> albero, se
    c è fatta di uguaglianze "incognita = numero" unite da e e da o; None se
    non lo è."""
    if c[0] == "cmp":
        if c[1] == "=" and c[2][0] == "var" and c[2][1] in unknowns and not letters(c[3]):
            return [{c[2][1]: c[3]}]
        return None
    a, b = listed(c[1], unknowns), listed(c[2], unknowns)
    if a is None or b is None:
        return None
    if c[0] == "or":
        return a + b
    return [dict(p, **q) for p in a for q in b if not set(p) & set(q)]


def critical_wrong(reader, claims, answer, known, rnd):
    """Controlla la risposta alla richiesta dei punti critici: True se è di
    sicuro sbagliata, False se nessun punto provato la smentisce, None se non
    si sa. Il confronto è punto per punto, nei due versi: dove la risposta
    dice "critico" il gradiente deve essere nullo (e la funzione regolare), e
    dove il gradiente è nullo la risposta deve dirlo. I punti: la griglia
    GRID, i punti che la risposta elenca, e quelli di known (i punti critici
    noti per costruzione, in modalità fuzz). Poi una ricerca con il metodo
    di Newton da partenze a caso: un punto trovato così che la risposta non
    spiega lascia la risposta non confermata (None), non sbagliata. Non è
    una dimostrazione di completezza: un punto critico irrazionale lontano
    dalle partenze sfugge."""
    values, _, domains = resolve(reader)
    name = reader.operands[claims[0][0][1]][0]
    unknowns = reader.functions[name][0]
    gradient = [values[claim[0][1]][1] for claim in claims]
    exist = [t for claim in claims for t in domains[claim[0][1]]]
    if len(unknowns) != 2:
        return None
    x, y = unknowns
    points, lines = [], []
    if answer == "\\emptyset":
        said = lambda env: False
    elif answer == "\\mathbb{R}^{2}":
        said = lambda env: True
    else:
        c = condition(Reader(answer))
        said = lambda env: satisfied(c, env)
        every = listed(c, unknowns) or []
        points = [{k: val(v, {}) for k, v in p.items()} for p in every if set(p) == set(unknowns)]

        # Una retta di punti critici, "x = r" senza nulla sull'altra incognita:
        # la risposta la dice critica tutta, e il programma non la rilegge
        # punto per punto. Qui la si guarda in qualche punto: una retta fuori
        # dal dominio di un logaritmo si vede così, anche con r irrazionale,
        # che la griglia non tocca.
        for p in every:
            if len(p) == 1:
                (fixed, tree), = p.items()
                lines += [{fixed: val(tree, {}), (y if fixed == x else x): t} for t in LINE]

    def critical(env):
        """True, False o None: la funzione è regolare e il gradiente è nullo in env."""
        try:
            for tree in exist:
                val(tree, env)
            parts = [val(g, env) for g in gradient]
        except Overflow:
            return None
        except Undefined:
            return False
        zero = [same(p, Fr(0)) if isinstance(p, Fr) else (True if abs(p) <= 1e-9 else False if abs(p) > 1e-6 else None) for p in parts]
        return False if False in zero else None if None in zero else True

    tests = [{x: p, y: q} for p in GRID for q in GRID] + points + lines + [dict(zip(unknowns, p)) for p in known]
    unsure = False
    for env in tests:
        truly, claimed = critical(env), said(env)
        if truly is None or claimed is None:
            unsure = True
        elif truly != claimed:
            return True

    # La ricerca con Newton: il gradiente e la sua jacobiana, dalle regole di
    # qui. Un punto trovato così è approssimato, e dove la jacobiana degenera
    # (x^4 + y^4 nell'origine) la ricerca si ferma prima di arrivare: se la
    # risposta non lo dice critico, e non elenca un punto lì vicino, non è
    # un errore certo ma un caso da guardare.
    jacobian = [[diff(g, v) for v in unknowns] for g in gradient]
    for _ in range(60):
        px, py = rnd.uniform(-5, 5), rnd.uniform(-5, 5)
        try:
            for _ in range(60):
                env = {x: px, y: py}
                g = [float(val(t, env)) for t in gradient]
                (a, b), (c2, d) = [[float(val(t, env)) for t in row] for row in jacobian]
                det = a * d - b * c2
                if abs(det) < 1e-14:
                    break
                px, py = px - (d * g[0] - b * g[1]) / det, py - (a * g[1] - c2 * g[0]) / det
            env = {x: px, y: py}
            if abs(px) > 50 or abs(py) > 50 or any(abs(float(val(t, env))) > 1e-11 for t in gradient):
                continue
            # Critico vuol dire anche che lì la funzione c'è: la formula del
            # gradiente di ln u ha un valore, e può annullarsi, anche dove u è
            # negativo. Un punto così non è un punto critico.
            for tree in exist:
                val(tree, env)
        except (Undefined, OverflowError, ZeroDivisionError):
            continue
        near = any(abs(float(p[x]) - px) + abs(float(p[y]) - py) < 1e-2 for p in points)
        if said(env) is True or near:
            continue
        # Vicino a una retta di punti critici il gradiente è piccolissimo
        # anche fuori dalla retta, e la ricerca si ferma a un passo da lei: il
        # punto trovato è spiegato se, arrotondato a qualche decimale, dà un
        # punto in cui la risposta dice "critico" e il gradiente è nullo
        # esattamente.
        explained = False
        for digits in (6, 5, 4, 3, 2):
            rounded = {x: Fr(round(px * 10 ** digits), 10 ** digits), y: Fr(round(py * 10 ** digits), 10 ** digits)}
            explained = explained or (said(rounded) is True and critical(rounded) is True)
        unsure = unsure or not explained
    return None if unsure else False


# ----------------------------------------------------------------------------
# 5. Il programma.
# ----------------------------------------------------------------------------

PREAMBLE = "\\documentclass{article}\n\\usepackage{amsmath,amssymb}\n\\begin{document}\n\\begin{enumerate}\n"

# In modalità fuzz: per una richiesta dei punti critici costruita in modo da
# conoscerli, l'enunciato e l'elenco dei suoi punti critici (coppie di
# frazioni). Lo riempie generate(), lo legge judge().
KNOWN = {}


def run_program(program, statements, path):
    with open(path, "w") as out:
        out.write(PREAMBLE + "".join("\\item $\\displaystyle %s$\n" % s for s in statements) + "\\end{enumerate}\n\\end{document}\n")
    result = subprocess.run([program, path], capture_output=True, text=True)
    verdicts = {}
    for line in result.stdout.splitlines():
        m = re.match(r"\s*(\d+)\. (.*)", line)
        if m:
            verdicts[int(m.group(1))] = m.group(2)
    return verdicts, "ATTENZIONE" in result.stdout, result.returncode


def items_of(path):
    text = open(path).read()
    return [m.group(1).strip() for m in re.finditer(r"\\item \$\\displaystyle (.*?)\$\s*(?=\\item|\\end\{enumerate\})", text, re.S)]


def judge(statements, expected, verdicts, warned, rnd, label):
    """Confronta i verdetti con l'oracolo; expected[i] è True/False per i file di dati, o None."""
    counts = {"proved": 0, "refuted": 0, "solved": 0, "true undecided": 0, "false undecided": 0, "both undecided": 0, "unread": 0, "BUG": 0, "SUSPECT": 0,
              "DATA": 0}
    notes = os.environ.get("FUZZ_NOTES")
    if warned:
        counts["BUG"] += 1
        print("BUG %s: il programma ha stampato ATTENZIONE (passi scartati dai controlli)" % label)
    for index, statement in enumerate(statements, 1):
        verdict = verdicts.get(index, "")
        try:
            reader = Reader(statement)
            request, claims = reader.statement()
            oracle = None if request else truth(reader, claims, rnd)
        except (Unreadable, Undefined, IndexError, ValueError, KeyError) as problem:
            counts["unread"] += 1
            if not verdict.startswith("NON SUPPORTATO") and notes:
                print("NOTE %s %d: l'oracolo non legge l'enunciato (%s): %s" % (label, index, problem, statement))
            if verdict.startswith(("DIMOSTRATO", "FALSO", "RISOLTO")) and expected[index - 1] is not None:
                counts["SUSPECT"] += 1
                print("SUSPECT %s %d: verdetto senza oracolo: %s | %s" % (label, index, verdict, statement))
            continue
        if request:
            if verdict.startswith("RISOLTO"):
                counts["solved"] += 1
                try:
                    if request == "critical":
                        # La risposta sta dopo il primo ": ", e prima della nota tra parentesi.
                        answer = verdict.split(": ", 1)[1]
                        for note in (" (condizioni del secondo ordine", " (i punti critici non sono", " (i punti in cui la funzione non"):
                            answer = answer.split(note, 1)[0]
                        wrong = critical_wrong(reader, claims, answer.strip(), KNOWN.get(statement, []), rnd)
                    else:
                        wrong = answer_wrong(reader, claims, verdict.split(": ", 1)[1].rsplit(" (dove", 1)[0], rnd)
                except (Unreadable, Undefined, IndexError, ValueError, KeyError):
                    wrong = None
                if wrong:
                    counts["BUG"] += 1
                    print("BUG %s %d: risposta sbagliata: %s | %s" % (label, index, verdict, statement))
                elif wrong is None:
                    counts["SUSPECT"] += 1
                    print("SUSPECT %s %d: risposta non confermata: %s | %s" % (label, index, verdict, statement))
            elif notes:
                print("NOTE %s %d: richiesta senza risposta: %s | %s" % (label, index, verdict, statement))
            continue
        if expected[index - 1] is not None and oracle is not None and oracle != expected[index - 1]:
            counts["DATA"] += 1
            print("DATA %s %d: l'oracolo lo trova %s: %s" % (label, index, "vero" if oracle else "falso", statement))
        proved, refuted = verdict.startswith("DIMOSTRATO"), verdict.startswith("FALSO")
        if (proved and oracle is False) or (refuted and oracle is True):
            counts["BUG"] += 1
            print("BUG %s %d: oracolo %s, programma: %s | %s" % (label, index, "vero" if oracle else "falso", verdict, statement))
        elif (proved or refuted) and oracle is None:
            counts["SUSPECT"] += 1
            print("SUSPECT %s %d: l'oracolo non decide; programma: %s | %s" % (label, index, verdict, statement))
        elif proved or refuted:
            counts["proved" if proved else "refuted"] += 1
        else:
            counts["true undecided" if oracle else "false undecided" if oracle is False else "both undecided"] += 1
            if notes:
                print("NOTE %s %d: oracolo %s, programma: %s | %s" % (label, index, oracle, verdict, statement))
    return counts


def mode_check(program, paths, work=None):
    rnd, total = random.Random(1), {}
    for path in paths:
        statements = items_of(path)
        copy = os.path.join(work or os.environ.get("TMPDIR", "/tmp"), "check_" + os.path.basename(path))
        verdicts, warned, _ = run_program(program, statements, copy)
        expected = [not os.path.basename(path).startswith("false_")] * len(statements)
        counts = judge(statements, expected, verdicts, warned, rnd, os.path.basename(path))
        print("%s: %d enunciati; %s" % (path, len(statements), ", ".join("%d %s" % (v, k) for k, v in counts.items() if v)))
        for key, value in counts.items():
            total[key] = total.get(key, 0) + value
    print("Totale: " + ", ".join("%d %s" % (v, k) for k, v in total.items()))
    return 1 if total.get("BUG") or total.get("DATA") else 0


# ----------------------------------------------------------------------------
# 6. Generare funzioni ed enunciati.
# ----------------------------------------------------------------------------

def polynomial(rnd, variables, terms, degree):
    e = ZERO
    for _ in range(terms):
        term = num(rnd.choice([-4, -3, -2, -1, 1, 2, 3, 4]))
        for v in variables:
            power = rnd.choice([0, 0, 1, 1, 2, degree])
            if power:
                term = mul(term, ("var", v) if power == 1 else ("pow", ("var", v), num(power)))
        e = add(e, term)
    return e if e != ZERO else ("var", variables[0])


def function(rnd, variables, depth=1):
    """Ritorna (albero, razionale): razionale è True se ha solo operazioni razionali."""
    small = polynomial(rnd, variables, rnd.randint(1, 2), 2)
    kind = rnd.choice(["poly", "poly", "ratio", "exp", "ln", "sin", "cos", "sqrt", "atan", "sum", "product"] if depth else ["poly", "ratio"])
    if kind == "poly":
        return polynomial(rnd, variables, rnd.randint(2, 4), 3), True
    if kind == "ratio":
        return ("div", polynomial(rnd, variables, 2, 2), add(polynomial(rnd, variables, 2, 2), num(rnd.randint(1, 3)))), True
    if kind in ("sum", "product"):
        (a, exact_a), (b, exact_b) = function(rnd, variables, 0), function(rnd, variables, depth - 1 if rnd.random() < 0.5 else 0)
        if rnd.random() < 0.5:
            b, exact_b = fn(rnd.choice(["exp", "sin", "cos"]), small), False
        return ("add" if kind == "sum" else "mul", a, b), exact_a and exact_b
    if kind == "sqrt":
        return ("root", 2, add(("pow", small, num(2)), num(rnd.randint(1, 5)))), False
    if kind == "ln":
        return fn("ln", add(("pow", small, num(2)), num(rnd.randint(1, 5)))), False
    return fn(kind, small), False


def mutate(rnd, e):
    """Un albero vicino a e: un numero cambiato, un segno, un termine tolto, due lettere scambiate."""
    paths = []

    def walk(node, path):
        paths.append((path, node))
        for k, child in enumerate(node[1:], 1):
            if isinstance(child, tuple):
                walk(child, path + (k,))

    def rebuilt(node, path, new):
        if not path:
            return new
        return node[:path[0]] + (rebuilt(node[path[0]], path[1:], new),) + node[path[0] + 1:]

    walk(e, ())
    kind = rnd.choice(["number", "number", "sign", "drop", "swap"])
    numbers = [(p, n) for p, n in paths if n[0] == "num"]
    sums = [(p, n) for p, n in paths if n[0] in ("add", "sub")]
    if kind == "number" and numbers:
        path, node = rnd.choice(numbers)
        return rebuilt(e, path, num(node[1] + rnd.choice([-1, 1])))
    if kind == "drop" and sums:
        path, node = rnd.choice(sums)
        return rebuilt(e, path, node[rnd.choice([1, 2])])
    if kind == "swap":
        names = sorted(letters(e))
        if len(names) >= 2:
            a, b = rnd.sample(names, 2)
            return subst(e, {a: ("var", b), b: ("var", a)})
    path, node = rnd.choice(paths)
    return rebuilt(e, path, ("neg", node))


def operand_tex(rnd, name, body, by):
    """Una derivata scritta in uno dei tre modi del capitolo 34; ritorna (testo, serve la definizione)."""
    order, style = len(by), rnd.choice(["fraction", "subscript", "sign"])
    if style == "subscript":
        return "%s_{%s}" % (name, "".join(by)), True
    below = " ".join("\\partial " + v for v in by[::-1])
    top = "\\partial" if order == 1 else "\\partial^{%d}" % order
    if style == "fraction":
        return "\\frac{%s %s}{%s}" % (top, name, below), True
    return "\\frac{%s}{%s}\\left( %s \\right)" % (top, below, tex(body)), False


def centred(rnd, variables, point):
    """Una funzione che ha in point un punto critico, quasi sempre: una forma
    quadratica negli scarti dal punto, più un termine di terzo grado; una
    volta su cinque anche un termine di primo grado, e allora il punto non è
    critico; una volta su cinque dentro un esponenziale, che non cambia i
    punti critici."""
    shifts = [("var", v) if q == 0 else ("sub", ("var", v), num(q)) for v, q in zip(variables, point)]
    body = ZERO
    for i, a in enumerate(shifts):
        for b in shifts[i:]:
            body = add(body, mul(num(rnd.choice([-3, -2, -1, 0, 1, 1, 2, 3])), mul(a, b)))
    body = add(body, mul(num(rnd.choice([-2, -1, 1, 2])), mul(mul(rnd.choice(shifts), rnd.choice(shifts)), rnd.choice(shifts))))
    if rnd.random() < 0.2:
        body = add(body, mul(num(rnd.choice([-1, 1, 2])), rnd.choice(shifts)))
    return fn("exp", body) if rnd.random() < 0.2 else add(body, num(rnd.randint(-3, 3)))


def quadratic_matrix(rnd, n):
    """Una matrice simmetrica di lato n a elementi interi, scelta in modo che
    capitino i casi che contano per la definitezza: B^T B (semidefinita
    positiva, definita se B è invertibile), la stessa con una riga di B
    azzerata o ripetuta (semidefinita con il determinante nullo), una
    diagonale con degli zeri in testa (i minori di testa tutti nulli, e la
    matrice non semidefinita), una qualsiasi (di solito indefinita); e una
    volta su due l'opposta."""
    style = rnd.choice(["gram", "gram", "deficient", "diagonal", "any"])
    if style == "any":
        a = [[0] * n for _ in range(n)]
        for r in range(n):
            for c in range(r, n):
                a[r][c] = a[c][r] = rnd.randint(-2, 2)
    elif style == "diagonal":
        entries = [rnd.choice([0, 0, 1, 2, -1]) for _ in range(n)]
        entries.sort(key=lambda v: (v != 0, rnd.random()))      # gli zeri davanti
        a = [[entries[r] if r == c else 0 for c in range(n)] for r in range(n)]
    else:
        b = [[rnd.randint(-2, 2) for _ in range(n)] for _ in range(n)]
        if style == "deficient":
            b[rnd.randrange(n)] = rnd.choice([[0] * n, b[0][:]])
        a = [[sum(b[t][r] * b[t][c] for t in range(n)) for c in range(n)] for r in range(n)]
    sign = rnd.choice([1, -1])
    return [[sign * v for v in row] for row in a]


def polynomial_in(coefficients, u):
    """L'albero della somma dei coefficients[i] per u^i."""
    e = ZERO
    for power, c in enumerate(coefficients):
        if c:
            e = add(e, mul(num(c), ONE if power == 0 else u if power == 1 else ("pow", u, num(power))))
    return e


def primitive_with_roots(rnd):
    """Ritorna (coefficienti di A, radici reali di A'): A' è un numero per un
    prodotto di fattori di primo grado con le radici scelte, a volte per
    t^2 + 1, che non ne aggiunge; il grado di A' è al più 3."""
    roots = [Fr(rnd.randint(-4, 4), rnd.choice([1, 1, 2])) for _ in range(rnd.randint(1, 3))]
    factors = [[-r, Fr(1)] for r in roots]
    if len(roots) == 1 and rnd.random() < 0.5:
        factors.append([Fr(1), Fr(0), Fr(1)])
    slope = [Fr(rnd.choice([-2, -1, 1, 2, 3]))]
    for factor in factors:
        slope = [sum(slope[i] * factor[k - i] for i in range(len(slope)) if 0 <= k - i < len(factor)) for k in range(len(slope) + len(factor) - 1)]
    return [Fr(rnd.randint(-2, 2))] + [c / (k + 1) for k, c in enumerate(slope)], roots


def critical_case(rnd):
    """Una funzione di x e y di cui si conoscono tutti i punti critici, per
    come è costruita: ritorna (albero, elenco dei punti critici, oppure dei
    punti noti se sono infiniti)."""
    x, y = ("var", "x"), ("var", "y")
    kind = rnd.choice(["separate", "separate", "mixed", "mixed", "cubic", "quadratic", "line", "none", "all", "log", "log"])
    if kind == "log":
        # Un logaritmo: la sua derivata ha un valore anche dove lui non c'è, e i
        # punti critici vanno cercati solo dove la funzione è definita. Qui non
        # si conoscono in anticipo: decide il confronto punto per punto. Una
        # volta su quattro manca il termine in y: allora i punti critici
        # stanno su rette, che il programma non rilegge una per una, ed è lì
        # che una retta fuori dal dominio non veniva fermata da nulla.
        a, b = rnd.choice([-2, -1, 1, 2]), rnd.randint(-2, 2)
        inner = add(mul(num(a), x), num(b)) if rnd.random() < 0.6 else add(add(mul(num(a), x), y), num(b))
        body = add(mul(num(rnd.choice([-2, -1, 1, 2])), fn("ln", inner)), polynomial_in([Fr(0), Fr(rnd.randint(-3, 3)), Fr(rnd.randint(-2, 2), 2)], x))
        along = rnd.choice([-1, 1, 2, 0])
        return (add(body, mul(num(along), ("pow", y, num(2)))) if along else body), []
    if kind in ("separate", "mixed"):
        # A(u) + B(v), con u e v due combinazioni indipendenti di x e y: il
        # gradiente è A'(u) per il gradiente di u più B'(v) per quello di v, e
        # si annulla esattamente dove A'(u) = 0 e B'(v) = 0.
        (first, us), (second, vs) = primitive_with_roots(rnd), primitive_with_roots(rnd)
        a, b, c, d = (1, 0, 0, 1) if kind == "separate" else rnd.choice([(1, 1, 1, -1), (2, 1, 1, 1), (1, 2, 0, 1), (1, -1, 2, 1), (3, 1, 1, 0)])
        u, v = add(mul(num(a), x), mul(num(b), y)), add(mul(num(c), x), mul(num(d), y))
        det = a * d - b * c
        return add(polynomial_in(first, u), polynomial_in(second, v)), [((d * r - b * s) / det, (a * s - c * r) / det) for r in set(us) for s in set(vs)]
    if kind == "cubic":
        # x^3 + y^3 - 3 a x y: y = x^2 / a e x = y^2 / a, quindi x = 0 o x = a.
        a = Fr(rnd.choice([-3, -2, -1, 1, 2, 3]), rnd.choice([1, 1, 2]))
        return sub(add(("pow", x, num(3)), ("pow", y, num(3))), mul(num(3 * a), mul(x, y))), [(Fr(0), Fr(0)), (a, a)]
    if kind == "quadratic":
        # A x^2 + B x y + C y^2 + D x + E y con 4 A C - B^2 diverso da zero: un punto solo.
        while True:
            A, B, C, D, E = [rnd.randint(-3, 3) for _ in range(5)]
            if 4 * A * C - B * B:
                break
        body = add(add(mul(num(A), ("pow", x, num(2))), mul(num(B), mul(x, y))), add(mul(num(C), ("pow", y, num(2))), add(mul(num(D), x), mul(num(E), y))))
        return body, [(Fr(-2 * C * D + B * E, 4 * A * C - B * B), Fr(-2 * A * E + B * D, 4 * A * C - B * B))]
    if kind == "line":
        # (a x + b y + c)^2: critici i punti della retta a x + b y + c = 0.
        a, b, c = rnd.choice([-2, -1, 1, 2]), rnd.choice([-2, -1, 1, 2]), rnd.randint(-2, 2)
        body = ("pow", add(add(mul(num(a), x), mul(num(b), y)), num(c)), num(2))
        return body, [(Fr(t), Fr(-c - a * t, b)) for t in range(-3, 4)]
    if kind == "none":
        return add(mul(num(rnd.choice([-2, 1, 3])), x), ("pow", y, num(rnd.choice([1, 3])))) if rnd.random() < 0.5 else add(x, mul(x, ("pow", y, num(2)))), []
    return num(rnd.randint(-3, 3)), [(Fr(p), Fr(q)) for p in range(-2, 3) for q in range(-2, 3)]


def generate(rnd, count):
    statements = []
    while len(statements) < count:
        variables = rnd.choice([["x", "y"], ["x", "y"], ["x", "y", "z"], ["u", "v"], ["x", "t"]])
        body, rational = function(rnd, variables)
        head = "f(%s) = %s \\implies " % (", ".join(variables), tex(body))
        by = [rnd.choice(variables) for _ in range(rnd.choice([1, 1, 2, 2, 3]))]
        derivative = body
        for letter in by:
            derivative = diff(derivative, letter)
        kind = rnd.choice(["partial", "partial", "partial", "gradient", "hessian", "point", "point", "schwarz", "request", "request", "two",
                           "second", "second", "second", "compare", "compare", "critical", "critical", "nested", "nested", "definite", "definite"])
        point = [Fr(rnd.randint(-4, 4), rnd.randint(1, 3)) for _ in variables]
        where = "\\left( %s \\right)" % ", ".join(tex(num(q)) for q in point)
        env = dict(zip(variables, point))

        def at_point(e):
            # Un valore esatto nel punto, o None se lì non c'è o non è razionale.
            try:
                val(body, env)
                value = val(e, env)
            except Undefined:
                return None
            return value if isinstance(value, Fr) else None

        if kind == "partial":
            text, declared = operand_tex(rnd, "f", body, by)
            right = derivative if rnd.random() < 0.45 else mutate(rnd, derivative)
            statements.append((head if declared else "") + "%s = %s" % (text, tex(right)))
        elif kind == "gradient":
            parts = [diff(body, v) for v in variables]
            if rnd.random() < 0.55:
                k = rnd.randrange(len(parts))
                parts[k] = mutate(rnd, parts[k])
            statements.append(head + "\\nabla f = \\left( %s \\right)" % ", ".join(tex(p) for p in parts))
        elif kind == "hessian" and len(variables) == 2:
            parts = [diff(diff(body, r), c) for r in variables for c in variables]
            if rnd.random() < 0.55:
                k = rnd.randrange(4)
                parts[k] = mutate(rnd, parts[k])
            statements.append(head + "H_{f} = \\begin{pmatrix} %s & %s \\\\ %s & %s \\end{pmatrix}" % tuple(tex(p) for p in parts))
        elif kind == "point" and rational:
            shape = rnd.choice(["partial", "gradient", "hessian"]) if len(variables) == 2 else rnd.choice(["partial", "gradient"])
            trees = {"partial": [derivative], "gradient": [diff(body, v) for v in variables],
                     "hessian": [diff(diff(body, r), c) for r in variables for c in variables]}[shape]
            numbers = [at_point(t) for t in trees]
            if None in numbers:
                numbers = [Fr(rnd.randint(-3, 3)) for _ in trees]       # un punto fuori dal dominio: l'enunciato è falso
            elif rnd.random() < 0.5:
                k = rnd.randrange(len(numbers))
                numbers[k] += rnd.choice([-1, 1, Fr(1, 2)])
            written = [tex(num(q)) for q in numbers]
            if shape == "partial":
                text, declared = operand_tex(rnd, "f", body, by)
                if declared:
                    statements.append(head + "%s%s = %s" % (text, where, written[0]))
            elif shape == "gradient":
                statements.append(head + "\\nabla f%s = \\left( %s \\right)" % (where, ", ".join(written)))
            else:
                statements.append(head + "H_{f}%s = \\begin{pmatrix} %s & %s \\\\ %s & %s \\end{pmatrix}" % ((where,) + tuple(written)))
        elif kind == "two":
            # Due funzioni con domini diversi, una tesi per ciascuna: la seconda è
            # vera solo dove la sua funzione ha un segno, e lì l'altra può non esserci.
            x, y = variables[0], variables[1]
            narrow = rnd.choice([fn("ln", ("var", x)), ("root", 2, ("var", x)), fn("ln", ("neg", ("var", x))), polynomial(rnd, variables, 2, 2)])
            wide = rnd.choice([mul(("root", 2, ("pow", ("var", x), num(2))), ("var", y)), mul(fn("abs", ("var", x)), ("var", y)), body])
            stated = rnd.choice([diff(wide, x), ("var", y), ("neg", ("var", y))])
            statements.append("f(%s) = %s, g(%s) = %s \\implies f_{%s} = %s \\land g_{%s} = %s" % (
                ", ".join(variables), tex(narrow), ", ".join(variables), tex(wide), x, tex(diff(narrow, x)), x, tex(stated)))
        elif kind == "schwarz" and len(by) >= 2:
            other = by[:]
            rnd.shuffle(other)
            if rnd.random() < 0.3:
                other[0] = rnd.choice(variables)
            statements.append(head + "f_{%s} = f_{%s}" % ("".join(by), "".join(other)))
        elif kind == "request":
            form = rnd.choice(["partial", "gradient", "hessian", "at"])
            if form == "partial":
                text, declared = operand_tex(rnd, "f", body, by)
                statements.append((head if declared else "") + text)
            elif form == "gradient":
                statements.append(head + "\\nabla f")
            elif form == "hessian":
                statements.append(head + "H_{f}")
            elif rational:
                statements.append(head + rnd.choice(["\\nabla f", "H_{f}"]) + where)
        elif kind == "second":
            # Le condizioni del secondo ordine in un punto: il gradiente nullo, il
            # segno del determinante dell'hessiana e di f_xx, e con tre variabili
            # quello del minore di nord-ovest di lato 2, scritto come tabella. I
            # segni sono quelli veri; una volta su due una tesi ne prende un altro.
            variables = rnd.choice([["x", "y"], ["x", "y"], ["x", "y", "z"]])
            point = [Fr(rnd.randint(-3, 3), rnd.choice([1, 1, 2])) for _ in variables]
            body = centred(rnd, variables, point)
            head = "f(%s) = %s \\implies " % (", ".join(variables), tex(body))
            where = "\\left( %s \\right)" % ", ".join(tex(num(q)) for q in point)
            env, n = dict(zip(variables, point)), len(variables)
            second = [val(diff(diff(body, r), c), env) for r in variables for c in variables]
            sign = lambda value: ">" if value > 0 else "<" if value < 0 else "="
            entry = lambda r, c: "f_{%s%s}%s" % (variables[r], variables[c], where)
            claims = ["\\nabla f%s = %s" % (where, rnd.choice(["0", "\\left( %s \\right)" % ", ".join("0" for _ in variables)])),
                      "\\det H_{f}%s %s 0" % (where, sign(determinant(second, n))), "%s %s 0" % (entry(0, 0), sign(second[0]))]
            if n == 3:
                minor = determinant([second[0], second[1], second[3], second[4]], 2)
                claims.append("\\det \\begin{pmatrix} %s & %s \\\\ %s & %s \\end{pmatrix} %s 0" % (entry(0, 0), entry(0, 1), entry(1, 0), entry(1, 1), sign(minor)))
            if rnd.random() < 0.5:
                k = rnd.randrange(1, len(claims))
                claims[k] = claims[k].rsplit(" ", 2)[0] + " %s 0" % rnd.choice([">", "<", "=", "\\le", "\\ge", "\\neq"])
            statements.append(head + " \\land ".join(claims))
        elif kind == "compare" and rational:
            # Un confronto tra due numeri: valori e derivate in un punto, a volte
            # dentro il determinante di una tabella, contro un numero vicino al
            # valore vero (anche uguale: lì < è falso e \le è vero) o contro
            # un'altra derivata.
            x, y = variables[0], variables[1]
            terms = ["f%s" % where, "f_{%s}%s" % (x, where), "f_{%s%s}%s" % (x, y, where), "f_{%s%s}%s" % (y, y, where), "\\det H_{f}%s" % where]
            left = rnd.choice(terms)
            if rnd.random() < 0.4:
                table = "%s & %d \\\\ %d & %s" % (rnd.choice(terms), rnd.randint(-2, 2), rnd.randint(-2, 2), rnd.choice(terms))
                left = rnd.choice(["\\det \\begin{pmatrix} %s \\end{pmatrix}", "\\begin{vmatrix} %s \\end{vmatrix}", "\\det \\begin{bmatrix} %s \\end{bmatrix}"]) % table
            right = rnd.choice(terms)
            try:
                reader = Reader(head + left)
                _, asked = reader.statement()
                _, placed, _ = resolve(reader)
                value = val(placed(asked[0][0]), {})
                if isinstance(value, Fr) and rnd.random() < 0.7:
                    right = tex(num(value + rnd.choice([0, 0, 1, -1, Fr(1, 2)])))
            except (Unreadable, Undefined, KeyError):
                pass
            statements.append(head + "%s %s %s" % (left, rnd.choice(["<", ">", "\\le", "\\ge", "\\neq", "\\leq", "\\geq", "\\ne"]), right))
        elif kind == "nested":
            # Una funzione definita con una derivata di un'altra, g, che esiste in
            # meno punti della sua derivata: fuori dal dominio di g la funzione
            # non c'è, anche se la formula lì ha un valore. Il dominio di g è
            # quello di un logaritmo, di una radice, di un arcoseno o di un
            # arcocoseno, o manca un punto per un divisore; la derivata è rispetto
            # a x, a y o mista; e il punto è scelto in modo che cada fuori dal
            # dominio più o meno una volta su due. La tesi scrive il valore che
            # la formula ha nel punto (o quel valore più 1), con il segno di
            # uguale o con un confronto che quel valore rende vero: è vera solo
            # dove g è regolare, e lo decide l'oracolo. È la combinazione in cui
            # il programma dimostrava un falso (35b, partial_regular_at()).
            x, y = ("var", "x"), ("var", "y")
            inner = rnd.choice([add(fn("ln", x), ("pow", y, num(2))), add(fn("ln", x), div(ONE, y)), add(fn("ln", x), fn("asin", y)),
                                add(mul(x, fn("ln", x)), ("root", 2, y)), add(div(("pow", x, num(2)), y), fn("ln", add(x, y))),
                                add(mul(("root", 2, x), y), ("pow", x, num(2))), add(fn("acos", x), mul(x, ("pow", y, num(2)))),
                                add(fn("asin", y), mul(("pow", x, num(2)), y)), mul(fn("ln", sub(x, y)), x)])
            wrt = rnd.choice(["x", "x", "y", "xy"])
            head = "g(x, y) = %s, f(x, y) = g_{%s} + %s \\implies " % (tex(inner), wrt, tex(polynomial(rnd, ["x", "y"], 2, 2)))
            spot = [Fr(rnd.choice([-2, -1, 1, 2, 3]), rnd.choice([1, 1, 2])), Fr(rnd.choice([-2, -1, 0, 1, 1, 2]), rnd.choice([1, 2]))]
            target = rnd.choice(["f", "f_{x}", "f_{y}", "f_{xy}"]) + "\\left( %s \\right)" % ", ".join(tex(num(q)) for q in spot)
            value = Fr(0)
            try:
                reader = Reader(head + target)
                _, asked = reader.statement()
                _, placed, _ = resolve(reader)
                found = val(placed(asked[0][0]), {})
                value = found if isinstance(found, Fr) else value
            except (Unreadable, Undefined, KeyError):
                pass
            statements.append(head + "%s %s %s" % (target, rnd.choice(["=", "=", "=", "\\le", "\\ge"]), tex(num(value + rnd.choice([0, 0, 0, 1])))))
        elif kind == "definite":
            # La definitezza dell'hessiana, con le quattro scritture. La funzione
            # è una forma quadratica negli scarti da un punto, con l'hessiana
            # doppia della matrice di quadratic_matrix(); una volta su tre ha in
            # più un termine di terzo grado, che nel punto non la cambia. Quale
            # scrittura sia vera lo decide l'oracolo, con un conto che non usa i
            # minori.
            variables = ["x", "y", "z", "w"][:rnd.choice([2, 2, 3, 3, 4])]
            n = len(variables)
            matrix = quadratic_matrix(rnd, n)
            point = [Fr(rnd.randint(-2, 2)) for _ in variables]
            shifts = [("var", v) if q == 0 else ("sub", ("var", v), num(q)) for v, q in zip(variables, point)]
            body = ZERO
            for r in range(n):
                for c in range(r, n):
                    if matrix[r][c]:
                        body = add(body, mul(num(matrix[r][c] * (1 if r == c else 2)), mul(shifts[r], shifts[c])))
            cubic = rnd.random() < 0.33
            if cubic:
                body = add(body, mul(num(rnd.choice([-2, -1, 1, 2])), mul(mul(rnd.choice(shifts), rnd.choice(shifts)), rnd.choice(shifts))))
            if body == ZERO:
                body = num(rnd.randint(1, 3))
            where = "\\left( %s \\right)" % ", ".join(tex(num(q)) for q in point) if cubic or rnd.random() < 0.7 else ""
            critical = "\\nabla f%s = 0 \\land " % where if where and rnd.random() < 0.3 else ""
            statements.append("f(%s) = %s \\implies %sH_{f}%s \\%s 0" % (", ".join(variables), tex(body), critical, where, rnd.choice(DEFINITE)))
        elif kind == "critical":
            # I punti critici di una funzione di cui si conoscono, per come è costruita.
            body, known = critical_case(rnd)
            statement = "f(x, y) = %s \\implies \\{(x, y) \\mid \\nabla f%s = %s\\}" % (tex(body), rnd.choice(["", "(x, y)"]), rnd.choice(["0", "(0, 0)"]))
            KNOWN[statement] = known
            statements.append(statement)
    return statements


def mode_fuzz(program, seed, count, work):
    rnd = random.Random(seed)
    statements = generate(rnd, count)
    verdicts, warned, _ = run_program(program, statements, os.path.join(work, "fuzz_multivariable_%d.tex" % seed))
    counts = judge(statements, [None] * len(statements), verdicts, warned, rnd, "seme %d" % seed)
    print("seme %d: %d enunciati; %s" % (seed, len(statements), ", ".join("%d %s" % (v, k) for k, v in counts.items())))
    return 1 if counts["BUG"] else 0


# ----------------------------------------------------------------------------
# 7. I certificati: leggere la notazione di checker.c, alterare, decidere.
# ----------------------------------------------------------------------------

WORDS = {"+": "add", "-": "sub", "*": "mul", "/": "div", "^": "pow"}
CALLS = {"exp": "exp", "ln": "ln", "lnabs": "ln", "sin": "sin", "cos": "cos", "atan": "atan", "abs": "abs"}


def sexpr(text):
    """Legge un'espressione della notazione dei certificati; ritorna (albero grezzo, resto)."""
    text = text.lstrip(" ")
    if text[0] != "(":
        m = re.match(r"-?\d+|[A-Za-z]", text)
        if not m:
            raise Unreadable("certificato: " + text[:20])
        return m.group(0), text[m.end():]
    word, rest = re.match(r"\((\S+?)(?=[\s()])(.*)", text, re.S).groups()
    items = [word]
    while True:
        rest = rest.lstrip(" ")
        if rest[0] == ")":
            return items, rest[1:]
        item, rest = sexpr(rest)
        items.append(item)


def tree_of(raw):
    if isinstance(raw, str):
        return num(int(raw)) if raw.lstrip("-").isdigit() else ("var", raw)
    word, arguments = raw[0], [tree_of(a) for a in raw[1:]]
    if word in WORDS:
        return (WORDS[word],) + tuple(arguments)
    if word == "neg":
        return ("neg", arguments[0])
    if word == "pi":
        return ("pi",)
    if word == "root":
        return ("root", int(arguments[0][1]), arguments[1])
    if word == "lnabs":
        return fn("ln", fn("abs", arguments[0]))
    if word in CALLS:
        return fn(CALLS[word], arguments[0])
    raise Unreadable("parola " + word)


def certificate_truth(text, rnd):
    """True, False o None per ciò che afferma un certificato partial."""
    terms, claims, current = [], [], None
    for line in text.strip().splitlines()[1:]:
        word, _, rest = line.partition(" ")
        if word == "term":
            current = {"name": rest.strip(), "by": [], "at": {}}
            terms.append(current)
        elif word in ("of", "is"):
            current[word] = tree_of(sexpr(rest)[0])
        elif word == "by":
            current["by"] = rest.split()
        elif word == "at":
            while rest.strip():
                letter, rest = rest.lstrip()[0], rest.lstrip()[1:]
                raw, rest = sexpr(rest)
                current["at"][letter] = tree_of(raw)
        elif word == "statement":
            raw = sexpr(rest)[0]
            if raw[0] not in ("=", "<", "<=", "neq"):
                raise Unreadable("tesi")
            claims.append((tree_of(raw[1]), tree_of(raw[2]), "!=" if raw[0] == "neq" else raw[0]))
    # Ogni termine: dove esiste, la derivata dell'oracolo, la formula, il
    # punto; e il valore che il nome prende. "Dove esiste" è un elenco di
    # alberi che devono essere tutti regolari: la sua funzione e, per ogni
    # termine senza punto che la sua funzione o il suo punto nominano, gli
    # alberi di quello. Con A la derivata di ln x e B = A + x, il termine B
    # c'è solo dove c'è ln x, anche se 1/x + x ha un valore altrove. Un
    # termine con il punto è invece un numero: che esista lo dice il suo
    # controllo, e chi lo nomina non deve chiedere altro.
    values, checks = {}, []
    for t in terms:
        function = subst(t["of"], values)
        derivative = function
        for letter in t["by"]:
            derivative = diff(derivative, letter)
        point = {k: subst(v, values) for k, v in t["at"].items()}
        inner = [j for j, u in enumerate(terms[:len(checks)]) if not u["at"] and (uses(t["of"], u["name"]) or any(uses(v, u["name"]) for v in t["at"].values()))]
        checks.append(([function] + [tree for j in inner for tree in checks[j][0]], derivative, t["is"], point))
        values[t["name"]] = subst(derivative, point) if point else derivative
    sides = [(subst(l, values), subst(r, values), relation) for l, r, relation in claims]

    # Per ogni tesi, i termini senza punto che nomina: di quelli un confronto
    # afferma l'esistenza per ogni valore delle lettere.
    mentioned = [[k for k, t in enumerate(terms) if not t["at"] and (uses(l, t["name"]) or uses(r, t["name"]))] for l, r, _ in claims]
    names = sorted(set().union(*[letters(a) | letters(b) for a, b, _ in sides],
                               *[set().union(*[letters(tree) for tree in trees]) | letters(d) | letters(g) for trees, d, g, _ in checks]) or {"x"})
    found, unsure = 0, False
    for attempt in range(400):
        env = sample(rnd, names)
        # Un confronto afferma anche, per ogni valore delle lettere, che i
        # termini senza punto che nomina esistono e che i suoi due membri
        # sono definiti: se in questo punto non è così il certificato afferma
        # il falso (dove qualcosa è solo non regolare, o il conto esce dai
        # float, l'oracolo non decide).
        for (left, right, relation), named in zip(sides, mentioned):
            if relation == "=":
                continue
            try:
                for k in named:
                    for tree in checks[k][0]:
                        val(tree, env)
                    val(checks[k][2], env)
                val(left, env)
                val(right, env)
            except (Irregular, Overflow):
                unsure = True
            except Undefined:
                return False
        try:
            pairs = []
            for trees, derivative, formula, point in checks:
                # Nel punto il termine deve esistere: la funzione regolare,
                # con quelle dei termini senza punto che nomina, e la formula
                # definita. Se il punto è fatto di soli numeri e non è così,
                # il certificato afferma il falso (o, dove la funzione è solo
                # non regolare o il conto esce dai float, qualcosa che
                # l'oracolo non decide).
                if point:
                    try:
                        fixed = dict(env)
                        fixed.update({k: val(v, env) for k, v in point.items()})
                        for tree in trees:
                            val(tree, fixed)
                        val(formula, fixed)
                    except Undefined as problem:
                        if not any(letters(v) for v in point.values()) and set(point) >= set().union(letters(formula), *[letters(tree) for tree in trees]):
                            return None if isinstance(problem, (Irregular, Overflow)) else False
                        raise
                # L'identità, dove la funzione è regolare e la formula definita.
                for tree in trees:
                    val(tree, env)
                pairs.append((val(derivative, env), val(formula, env), "="))
            pairs += [(val(l, env), val(r, env), relation) for l, r, relation in sides]
        except Undefined:
            continue
        if not all(finite(a) and finite(b) for a, b, _ in pairs):
            continue        # un punto in cui i float non bastano: se ne prova un altro
        found += 1
        for left, right, relation in pairs:
            true = compare(left, right, relation)
            if true is False:
                return False
            unsure = unsure or true is None
        if found == POINTS:
            break
    if found == 0 and not unsure:
        return "vacuous"        # in nessun punto provato i termini esistono: il teorema non dice nulla
    return None if unsure or found < 2 else True


def dump(raw):
    """Riscrive un albero grezzo di sexpr() nella notazione dei certificati."""
    return raw if isinstance(raw, str) else "(" + " ".join(dump(r) for r in raw) + ")"


def altered(rnd, text):
    """Il certificato con una o due alterazioni."""
    for _ in range(rnd.choice([1, 1, 2])):
        kind = rnd.choice(["number", "number", "operator", "letter", "drop", "relation"])
        if kind == "relation":
            # Una tesi con i due membri scambiati, o con un'altra relazione.
            lines = text.split("\n")
            rows = [k for k, line in enumerate(lines) if line.startswith("statement (")]
            if rows:
                k = rnd.choice(rows)
                raw = sexpr(lines[k][len("statement "):])[0]
                if rnd.random() < 0.5:
                    raw = [raw[0], raw[2], raw[1]]
                else:
                    raw = [rnd.choice([w for w in ("=", "<", "<=", "neq") if w != raw[0]]), raw[1], raw[2]]
                lines[k] = "statement " + dump(raw)
                text = "\n".join(lines)
            continue
        if kind == "number":
            places = [m for m in re.finditer(r"(?<![\w.])-?\d+", text)]
            if places:
                m = rnd.choice(places)
                text = text[:m.start()] + str(int(m.group(0)) + rnd.choice([-1, 1, 2])) + text[m.end():]
        elif kind == "operator":
            places = [m for m in re.finditer(r"\((\+|-|\*|sin|cos) ", text)]
            if places:
                m = rnd.choice(places)
                swap = {"+": "-", "-": "+", "*": "+", "sin": "cos", "cos": "sin"}[m.group(1)]
                text = text[:m.start(1)] + swap + text[m.end(1):]
        elif kind == "letter":
            lines = text.split("\n")
            rows = [k for k, line in enumerate(lines) if line.startswith(("by ", "at "))]
            if rows:
                k = rnd.choice(rows)
                found = [m for m in re.finditer(r"(?<= )[a-z](?= |$)", lines[k])]
                if found:
                    m = rnd.choice(found)
                    lines[k] = lines[k][:m.start()] + rnd.choice([c for c in "xyzt" if c != m.group(0)]) + lines[k][m.end():]
                text = "\n".join(lines)
        else:
            m = re.search(r"\((\+|-) (\([^()]*\)|\S+) ", text)
            if m:
                text = text[:m.start()] + m.group(2) + " " + text[m.end():].replace(")", "", 1)
    return text


def mode_certificates(checker, seed, count, work, paths):
    rnd = random.Random(seed)
    originals = []
    for path in paths:
        originals += [c for c in re.findall(r"certificate partial\n.*?\nend\n", open(path).read(), re.S)]
    if not originals:
        print("nessun certificato partial nei file")
        return 1
    batch = originals + [altered(rnd, rnd.choice(originals)) for _ in range(count)]
    path = os.path.join(work, "fuzz_partial_%d.cert" % seed)
    open(path, "w").write("\n".join(batch))
    result = subprocess.run([checker, path], capture_output=True, text=True)
    answers = {int(m.group(1)): m.group(2) for m in re.finditer(r"^\s*(\d+)\. (\S+)", result.stdout, re.M)}
    counts = {"accettati": 0, "respinti": 0, "non controllabili": 0, "accettati e veri": 0, "VACUOUS": 0, "BUG": 0, "SUSPECT": 0}
    for index, text in enumerate(batch, 1):
        answer = answers.get(index, "?")
        counts["accettati" if answer == "ACCETTATO" else "respinti" if answer == "RESPINTO:" else "non controllabili"] += 1
        if index <= len(originals) and answer != "ACCETTATO":
            counts["BUG"] += 1
            print("BUG seme %d: un certificato del motore non è accettato (%s):\n%s" % (seed, answer, text))
        if answer != "ACCETTATO":
            continue
        try:
            verdict = certificate_truth(text, rnd)
        except (Unreadable, Undefined, IndexError, ValueError, KeyError, AttributeError, TypeError):
            verdict = None
        if verdict is False:
            counts["BUG"] += 1
            print("BUG seme %d: accettato un certificato che afferma il falso:\n%s" % (seed, text))
        elif verdict == "vacuous":
            counts["VACUOUS"] += 1
        elif verdict is None:
            counts["SUSPECT"] += 1
            print("SUSPECT seme %d: accettato un certificato che l'oracolo non decide:\n%s" % (seed, text))
        else:
            counts["accettati e veri"] += 1
    print("seme %d: %d certificati (%d del motore); %s" % (seed, len(batch), len(originals), ", ".join("%d %s" % (v, k) for k, v in counts.items())))
    return 1 if counts["BUG"] else 0


if __name__ == "__main__":
    if len(sys.argv) >= 6 and sys.argv[1] == "fuzz":
        sys.exit(mode_fuzz(sys.argv[2], int(sys.argv[3]), int(sys.argv[4]), sys.argv[5]))
    if len(sys.argv) >= 4 and sys.argv[1] == "check":
        sys.exit(mode_check(sys.argv[2], sys.argv[3:]))
    if len(sys.argv) >= 7 and sys.argv[1] == "certificates":
        sys.exit(mode_certificates(sys.argv[2], int(sys.argv[3]), int(sys.argv[4]), sys.argv[5], sys.argv[6:]))
    print(__doc__)
    sys.exit(2)
