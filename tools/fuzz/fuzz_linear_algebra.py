r"""Fuzzing differenziale dell'algebra lineare sulle matrici a elementi razionali
(capitolo 31 di parser.c, capitolo 32 di main.c, capitolo 17 di checker.c).

Uso:
    python3 -I fuzz_linear_algebra.py <eseguibile> <seme> <quanti> <cartella di lavoro> [<checker>]
    python3 -I fuzz_linear_algebra.py letters <eseguibile> <seme> <quanti> <cartella di lavoro> [<checker>]
    python3 -I fuzz_linear_algebra.py data <file .tex>...
    python3 -I fuzz_linear_algebra.py answers <file .tex di richieste> <test .c con le soluzioni attese>

Nel primo modo genera <quanti> enunciati a caso su matrici scritte per esteso
(somme, prodotti, multipli, trasposte, potenze anche negative, determinanti,
ranghi, tracce, l'identita con e senza pedice, lo 0 come matrice nulla, matrici
con un nome), li da al programma e confronta ogni verdetto con l'oracolo. Se
c'e anche <checker>, il verificatore compilato da solo, gli fa ricontrollare i
certificati scritti dal programma e poi <quanti> certificati alterati.

Nel secondo modo fa lo stesso con gli enunciati che hanno una lettera negli
elementi e la lettera in \mathbb{R}: vedi CON UNA LETTERA, piu sotto.

Nel terzo modo non usa il programma: legge gli enunciati dei file di dati di
tests/linear_algebra/ con il suo lettore e stampa per ognuno che cosa ne dice
l'oracolo (VERO, FALSO, NON DEFINITO, il valore di una richiesta, RICHIESTA
per una richiesta di soluzioni). E il terzo conto sui risultati presi dai
libri: i dati non si controllano con il programma. Nel quarto modo, sempre
senza il programma, dice se le soluzioni attese scritte nel test in C sono
quelle delle richieste del file di dati.

L'ORACOLO

Non chiama mai il programma per decidere, e calcola per un'altra strada: con
le frazioni esatte di Python, il determinante con lo sviluppo di Laplace
lungo la prima riga, il rango come il piu grande ordine di un minore non
nullo, l'inversa con i complementi algebrici divisi per il determinante. Nel
programma gli stessi numeri vengono dall'eliminazione di Gauss, e il
verificatore li ricava dai testimoni che l'eliminazione lascia: qui non c'e
nessuna eliminazione e nessun testimone decide un valore. Sono conti che
crescono come il fattoriale: oltre trecentomila determinanti per un
enunciato l'oracolo si dichiara incapace (SUSPECT), e gli enunciati generati
non ci arrivano.

L'identita senza pedice e, per l'oracolo, un oggetto a se: "c volte
l'identita, della dimensione che serve" (la coppia ("unit", c)). Sommata a
una matrice quadrata o moltiplicata per una matrice qualsiasi prende la
dimensione giusta; dove nessuna matrice gliela da l'enunciato non e definito.
Il programma fa in un altro modo (porta la dimensione dal contesto verso il
basso): i due devono trovarsi d'accordo.

GLI ENUNCIATI

    uguaglianza vera      espressione = il suo valore, scritto per esteso
    uguaglianza falsa     lo stesso con un elemento cambiato di poco, un segno,
                          due elementi scambiati, la trasposta, una riga in meno
    \neq                  vero con il valore cambiato, falso con quello giusto
    confronti tra numeri  determinanti, ranghi e tracce, con <, >, \le, \ge
    = 0                   espressione - valore = 0, e la variante falsa
    non definito          dimensioni che non tornano, inversa di una singolare
    richiesta             l'espressione da sola: la risposta RISOLTO viene
                          letta qui e confrontata con il valore dell'oracolo
    rovinato              uno dei precedenti con un token tolto, ripetuto,
                          scambiato con il vicino o sostituito: lo giudica il
                          lettore di questo script, e se per lui non e un
                          enunciato il programma non deve deciderlo

I CERTIFICATI ALTERATI

Dai certificati del programma se ne ricavano altri cambiando una cosa: la
verita dichiarata, la relazione, un numero, una dimensione, la forma di una
tabella lasciandone i numeri (le due dimensioni scambiate, o tutto su una
riga), una parola, un operando, un numero della matrice di cui un testimone
parla; e nelle voci witness un numero, il rango dichiarato (uno in piu o in
meno), la permutazione (due righe scambiate, una riga due volte, una che non
c'e), due matrici scambiate (L con U, C con F), il tipo, la voce tolta,
ripetuta, scambiata con un'altra, o sostituita con il testimone di un'altra
matrice. Un numero cambiato a caso rompe quasi sempre un prodotto, e non
mette alla prova i controlli sulla forma: per quelli si costruiscono
testimoni falsi che superano tutti i controlli tranne uno (forge()): "rank
0" per una matrice non nulla, L U con U non triangolare, L U con un
elemento di L sopra la diagonale, una riga ripetuta al posto di una
permutazione.

Per ognuno che il verificatore ACCETTA l'oracolo rilegge il certificato con
un suo lettore della notazione e pretende due cose. La prima: l'enunciato
vale, o non vale, come il certificato dichiara, con i valori calcolati qui
(Laplace, minori, complementi algebrici) e non presi dai testimoni. La
seconda: ogni det, rank e inv ha il suo testimone, del tipo giusto, e il
testimone e quello che deve essere (una permutazione vera, con il segno
contato dai cicli; L e U della loro forma con L U uguale alle righe
permutate; C F uguale alla matrice e G A H uguale all'identita; l'inversa
uguale a quella dei complementi algebrici), e nessuno avanza. Se una delle
due manca e un BUG. E un BUG anche un testimone che supera i suoi prodotti
ma da un numero diverso da quello dell'oracolo: sarebbe falso uno dei
teoremi su cui il verificatore si appoggia.

CON UNA LETTERA

Gli enunciati: una richiesta di soluzioni \{k \in \mathbb{R} \mid S rel c\},
un "per ogni" o un "esiste" su S rel c, un'identita "S = polinomio" giusta o
con un coefficiente cambiato, un "S = 0 se e solo se k = ... \lor k = ..."
con le radici vere oppure con una tolta, spostata o in piu; S e un
determinante o una traccia di una matrice con la lettera negli elementi,
anche det(A - sI) con A di numeri e un nome, la trasposta, un quadrato, un
prodotto. In piu cio che con una lettera non si tratta (rango, inversa,
confronto tra matrici, divisione per la lettera, nessun insieme per la
lettera), che il programma non deve decidere.

L'oracolo sviluppa i determinanti con Laplace su polinomi suoi (la classe
Poly): il programma li trova dai valori in una griglia di punti, il
verificatore con la formula di Leibniz. La condizione, che e un confronto tra
polinomi nella lettera, la decide poi con una tabella: separa le radici con
il teorema di Sturm, in frazioni esatte, e guarda la condizione in ogni
radice e in un punto di ogni intervallo. Le soluzioni scritte dal programma
(k = a, a < k < b, anche con una radice quadrata negli estremi) le rilegge
come una condizione, e le confronta con quella dell'enunciato negli stessi
punti: il confronto e completo, non a campione. Dove il programma scrive una
radice senza radicali (r_{1}) il confronto non si fa, e la richiesta e
contata tra quelle non controllate. Con un parametro (due lettere) l'oracolo
decide le identita tra polinomi; il resto lo prova solo in alcuni valori del
parametro, e allora un si non e una conferma.

I certificati degli sviluppi (la voce letters) alterati: un numero, una
parola, la verita, la relazione, un pezzo tolto o ripetuto, una lettera al
posto di un numero o un numero al posto di una lettera. Per ognuno che il
verificatore accetta l'oracolo sviluppa i due membri e pretende che siano lo
stesso polinomio.

Righe stampate: BUG quando il programma dimostra un falso, dichiara falso un
vero, decide un enunciato non definito, risolve con un valore sbagliato, o
quando il verificatore accetta un certificato che non dice il vero o respinge
uno del programma; SUSPECT quando una risposta non si riesce a leggere, il
programma segnala un passo scartato, o decide un enunciato rovinato che il
lettore di questo script non legge (i due lettori non sono d'accordo su che
cosa c'e scritto: va guardato a mano); MISS quando il programma non decide un
enunciato che l'oracolo decide (numeri troppo grandi: non e un errore, ma va
contato). In fondo il riepilogo.

I file (gli enunciati, i certificati, l'uscita del programma) vanno nella
cartella di lavoro, fuori dal repository.
"""
import itertools
import math
import os
import random
import re
import subprocess
import sys
from fractions import Fraction as Fr


class Undefined(Exception):
    """L'espressione non ha un valore: un'operazione non e definita."""


class Unreadable(Exception):
    """Il testo non segue la notazione che questo lettore conosce."""


class TooBig(Exception):
    """Una matrice troppo grande per i conti dell'oracolo."""


# Quanti determinanti, contando quelli dentro gli sviluppi, l'oracolo calcola per un
# enunciato o un certificato. E un tetto sul lavoro e non sulla grandezza: l'identita
# 11 x 11 si sviluppa in undici passi, una 9 x 9 piena in piu di trecentomila.
MAX_DETERMINANTS = 300000
determinants = [0]


# ------------------------------------------------------------ i polinomi
class Poly:
    """Un polinomio in piu lettere a coefficienti razionali, per gli enunciati con una
    lettera: un dizionario dal monomio, la tupla ordinata delle coppie (lettera, esponente),
    al coefficiente, senza coefficienti nulli. Si somma e si moltiplica anche con i numeri,
    cosi le funzioni dell'aritmetica qui sotto (prodotti di matrici, sviluppo di Laplace)
    valgono per i polinomi senza cambiare."""

    def __init__(self, terms=None):
        self.terms = {m: c for m, c in (terms or {}).items() if c != 0}

    @staticmethod
    def of(x):
        return x if isinstance(x, Poly) else Poly({(): Fr(x)})

    @staticmethod
    def letter(name):
        return Poly({((name, 1),): Fr(1)})

    def __add__(self, other):
        out = dict(self.terms)
        for m, c in Poly.of(other).terms.items():
            out[m] = out.get(m, Fr(0)) + c
        return Poly(out)

    __radd__ = __add__

    def __neg__(self):
        return Poly({m: -c for m, c in self.terms.items()})

    def __sub__(self, other):
        return self + (-Poly.of(other))

    def __rsub__(self, other):
        return Poly.of(other) + (-self)

    def __mul__(self, other):
        out = {}
        for m1, c1 in self.terms.items():
            for m2, c2 in Poly.of(other).terms.items():
                exps = dict(m1)
                for name, e in m2:
                    exps[name] = exps.get(name, 0) + e
                m = tuple(sorted(exps.items()))
                out[m] = out.get(m, Fr(0)) + c1 * c2
        return Poly(out)

    __rmul__ = __mul__

    def __pow__(self, k):
        out = Poly.of(1)
        for _ in range(k):
            out = out * self
        return out

    def __truediv__(self, other):
        other = Poly.of(other)
        if not other.is_number() or other == 0:
            raise Unreadable("division by a letter, or by zero")
        return self * (1 / other.number())

    def __eq__(self, other):
        return self.terms == Poly.of(other).terms

    def __ne__(self, other):
        return not self == other

    __hash__ = None

    def is_number(self):
        return all(m == () for m in self.terms)

    def number(self):
        return self.terms.get((), Fr(0))

    def letters(self):
        return sorted({name for m in self.terms for name, _ in m})

    def at(self, name, v):
        """Il polinomio con il numero v al posto della lettera name."""
        out = Poly()
        for m, c in self.terms.items():
            rest = tuple((n, e) for n, e in m if n != name)
            out = out + Poly({rest: c * Fr(v) ** dict(m).get(name, 0)})
        return out

    def coefficients(self, name):
        """I coefficienti, dal termine noto, del polinomio nella sola lettera name."""
        if any(n != name for n in self.letters()):
            raise Unreadable("more than one letter")
        out = [Fr(0)] * (1 + max([dict(m).get(name, 0) for m in self.terms] or [0]))
        for m, c in self.terms.items():
            out[dict(m).get(name, 0)] = c
        return out


def settled(v):
    """Un polinomio senza lettere torna a essere il suo numero; ogni altro valore resta."""
    if isinstance(v, Poly) and v.is_number():
        return v.number()
    if is_matrix(v):
        return tuple(tuple(settled(x) for x in row) for row in v)
    return v


def has_letters(v):
    if is_matrix(v):
        return any(isinstance(x, Poly) for row in v for x in row)
    return isinstance(v, Poly)


# ------------------------------------------------------------ l'aritmetica
def is_matrix(v):
    return isinstance(v, tuple) and v and isinstance(v[0], tuple)


def is_unit(v):
    return isinstance(v, tuple) and v and v[0] == "unit"


def shape(m):
    return len(m), len(m[0])


def identity(n, c=Fr(1)):
    return tuple(tuple(c if i == j else Fr(0) for j in range(n)) for i in range(n))


def transpose(m):
    return tuple(zip(*m))


def mat_mul(a, b):
    if shape(a)[1] != shape(b)[0]:
        raise Undefined("product")
    cols = list(zip(*b))
    return tuple(tuple(sum((x * y for x, y in zip(row, col)), Fr(0)) for col in cols) for row in a)


def det(m):
    """Sviluppo di Laplace lungo la prima riga: la somma degli elementi della riga, ognuno
    per il determinante di cio che resta tolte la sua riga e la sua colonna, a segni alterni."""
    n, c = shape(m)
    if n != c:
        raise Undefined("determinant of a non-square matrix")
    determinants[0] += 1
    if determinants[0] > MAX_DETERMINANTS:
        raise TooBig("determinant of a %d x %d matrix" % (n, n))
    if n == 1:
        return m[0][0]
    total = Fr(0)
    for j in range(n):
        if m[0][j] != 0:
            minor = tuple(tuple(row[s] for s in range(n) if s != j) for row in m[1:])
            total += (-1 if j % 2 else 1) * m[0][j] * det(minor)
    return total


def rank(m):
    """Il piu grande ordine di un minore non nullo."""
    rows, cols = shape(m)
    # Il conto e quello dei minori da provare: una riga di dodici numeri ne ha dodici, e si fa.
    minors = sum(math.comb(rows, k) * math.comb(cols, k) for k in range(1, min(rows, cols) + 1))
    if minors > MAX_DETERMINANTS:
        raise TooBig("rank of a %d x %d matrix" % (rows, cols))
    for k in range(min(rows, cols), 0, -1):
        for rs in itertools.combinations(range(rows), k):
            for cs in itertools.combinations(range(cols), k):
                if det(tuple(tuple(m[i][j] for j in cs) for i in rs)) != 0:
                    return k
    return 0


def inverse(m):
    """I complementi algebrici, trasposti, divisi per il determinante."""
    n, c = shape(m)
    if n != c:
        raise Undefined("inverse of a non-square matrix")
    d = det(m)
    if d == 0:
        raise Undefined("inverse of a singular matrix")
    if n == 1:
        return ((1 / d,),)
    def cofactor(i, j):
        minor = tuple(tuple(m[r][s] for s in range(n) if s != j) for r in range(n) if r != i)
        return (-1 if (i + j) % 2 else 1) * det(minor)
    return tuple(tuple(cofactor(j, i) / d for j in range(n)) for i in range(n))


def fix(v, n):
    """Un'identita senza dimensione, accanto a una matrice di lato n, diventa una matrice."""
    return identity(n, v[1]) if is_unit(v) else v


def add(a, b, sign):
    if is_unit(a) and is_unit(b):
        return ("unit", a[1] + sign * b[1])
    if is_unit(a) or is_unit(b):
        other = b if is_unit(a) else a
        if not is_matrix(other) or shape(other)[0] != shape(other)[1]:
            raise Undefined("identity added to something that is not a square matrix")
        a, b = fix(a, shape(other)[0]), fix(b, shape(other)[0])
    if is_matrix(a) != is_matrix(b):
        raise Undefined("number plus matrix")
    if not is_matrix(a):
        return a + sign * b
    if shape(a) != shape(b):
        raise Undefined("sum of different shapes")
    return tuple(tuple(x + sign * y for x, y in zip(r, s)) for r, s in zip(a, b))


def mul(a, b):
    if is_unit(a) and is_unit(b):
        return ("unit", a[1] * b[1])
    if is_unit(a):
        a = fix(a, shape(b)[0]) if is_matrix(b) else ("unit", a[1] * b)
        if is_unit(a):
            return a
    if is_unit(b):
        b = fix(b, shape(a)[1]) if is_matrix(a) else ("unit", a * b[1])
        if is_unit(b):
            return b
    if is_matrix(a) and is_matrix(b):
        return mat_mul(a, b)
    if is_matrix(a) or is_matrix(b):
        m, c = (a, b) if is_matrix(a) else (b, a)
        return tuple(tuple(c * x for x in row) for row in m)
    return a * b


def power(a, k):
    if is_unit(a):
        if k < 0 and a[1] == 0:
            raise Undefined("inverse of zero")
        return ("unit", a[1] ** k)
    if isinstance(a, Poly):
        if k < 0:
            raise Unreadable("a negative power of a letter")
        return a ** k
    if not is_matrix(a):
        if k < 0 and a == 0:
            raise Undefined("division by zero")
        return Fr(a) ** k
    if shape(a)[0] != shape(a)[1]:
        raise Undefined("power of a non-square matrix")
    if k < 0 and has_letters(a):
        raise Unreadable("the inverse of a matrix with a letter")
    base = inverse(a) if k < 0 else a
    result = identity(shape(a)[0])
    for _ in range(abs(k)):
        result = mat_mul(result, base)
    return result


def value(t):
    """Il valore di un albero: una frazione, una matrice, o ("unit", c)."""
    kind = t[0]
    if kind == "num":
        return t[1]
    if kind == "var":
        return Poly.letter(t[1])
    if kind == "mat":
        rows = tuple(tuple(value(e) for e in row) for row in t[1])
        if any(is_matrix(e) or is_unit(e) for row in rows for e in row):
            raise Undefined("matrix inside a matrix")
        return rows
    if kind == "I":
        return identity(t[1]) if t[1] else ("unit", Fr(1))
    if kind == "neg":
        return mul(Fr(-1), value(t[1]))
    if kind in ("add", "sub"):
        return add(value(t[1]), value(t[2]), 1 if kind == "add" else -1)
    if kind == "mul":
        return mul(value(t[1]), value(t[2]))
    if kind == "div":
        a, b = value(t[1]), settled(value(t[2]))
        if is_matrix(a) or is_matrix(b) or is_unit(a) or is_unit(b) or b == 0:
            raise Undefined("division")
        if isinstance(b, Poly):
            raise Unreadable("division by a letter")
        return a / b
    if kind == "pow":
        return power(value(t[1]), t[2])
    a = value(t[1])
    if not is_matrix(a):
        raise Undefined("%s of something that is not a written matrix" % kind)
    if kind == "T":
        return transpose(a)
    if kind == "det":
        return det(a)
    if kind == "rank":
        if has_letters(a):
            raise Unreadable("the rank of a matrix with a letter")
        return Fr(rank(a))
    if kind == "tr":
        if shape(a)[0] != shape(a)[1]:
            raise Undefined("trace of a non-square matrix")
        return sum((a[i][i] for i in range(len(a))), Fr(0))
    raise Unreadable(kind)


def holds(left, right, relation, zero_left=False, zero_right=False):
    """La verita di "left relation right"; zero_* dice se quel membro e lo 0 scritto da solo."""
    if is_unit(left) and is_unit(right):
        raise Undefined("identity compared with identity")
    if zero_right and is_matrix(left):
        right = tuple(tuple(Fr(0) for _ in row) for row in left)
    if zero_left and is_matrix(right):
        left = tuple(tuple(Fr(0) for _ in row) for row in right)
    if is_unit(left) or is_unit(right):
        other = right if is_unit(left) else left
        if not is_matrix(other):
            raise Undefined("identity compared with a number")
        if shape(other)[0] != shape(other)[1]:
            # Nessuna identita e una matrice non quadrata.
            if relation not in ("=", "neq"):
                raise Undefined("order between matrices")
            return relation == "neq"
        left, right = fix(left, shape(other)[0]), fix(right, shape(other)[0])
    if is_matrix(left) != is_matrix(right):
        raise Undefined("number compared with matrix")
    if is_matrix(left):
        if relation not in ("=", "neq"):
            raise Undefined("order between matrices")
        return (left == right) == (relation == "=")
    return {"=": left == right, "neq": left != right, "<": left < right, "<=": left <= right,
            ">": left > right, ">=": left >= right}[relation]


# ------------------------------------------------------- il lettore del LaTeX
TOKEN = re.compile(r"\\\\|\\[a-zA-Z]+|\\[{},;!: ]|\d+|[A-Za-z]|\S")
LAYOUT = {"\\displaystyle", "\\textstyle", "\\,", "\\;", "\\!", "\\:", "\\ ", "\\quad", "\\qquad"}
ENVIRONMENTS = ("pmatrix", "bmatrix", "matrix", "array", "vmatrix")
FUNCTIONS = {"rank": "rank", "rk": "rank", "rg": "rank", "tr": "tr", "det": "det"}
RELATIONS = {"=": "=", "\\neq": "neq", "\\ne": "neq", "<": "<", ">": ">", "\\le": "<=", "\\leq": "<=", "\\ge": ">=", "\\geq": ">="}


class Reader:
    """Un lettore del linguaggio del capitolo 31, scritto senza guardare quello del programma."""

    def __init__(self, text):
        # Due numeri separati solo da spazi non sono ne un prodotto ne due elementi: non si leggono.
        if re.search(r"\d\s+\d", text):
            raise Unreadable("two numbers side by side")
        every = TOKEN.findall(text)
        # Subito dopo \left e \right, senza nemmeno uno spazio scritto (\ , \quad) in mezzo.
        for i, t in enumerate(every[:-1]):
            if t in ("\\left", "\\right") and every[i + 1] in LAYOUT:
                raise Unreadable("no delimiter after %s" % t)
        raw = [t for t in every if t not in LAYOUT]
        self.tok, i = [], 0
        while i < len(raw):
            # \left e \right spariscono: resta il delimitatore; le quadre valgono le tonde.
            # In LaTeX vogliono subito dopo un delimitatore: \left \left( non e nulla.
            if raw[i] in ("\\left", "\\right"):
                allowed = ("(", "[") if raw[i] == "\\left" else (")", "]")
                if i + 1 == len(raw) or raw[i + 1] not in allowed:
                    raise Unreadable("no delimiter after %s" % raw[i])
                i += 1
                continue
            self.tok.append({"[": "(", "]": ")"}.get(raw[i], raw[i]) if i and raw[i - 1] in ("\\left", "\\right") else raw[i])
            i += 1
        self.pos = 0
        self.names = {}
        self.letters = False  # negli enunciati con una lettera: le minuscole sono lettere, e c'e \sqrt

    def peek(self):
        return self.tok[self.pos] if self.pos < len(self.tok) else None

    def take(self, expected=None):
        t = self.peek()
        if t is None or (expected is not None and t != expected):
            raise Unreadable("expected %r, found %r" % (expected, t))
        self.pos += 1
        return t

    def word(self):
        self.take("{")
        letters = ""
        while self.peek() != "}":
            letters += self.take()
        self.take("}")
        return letters

    def statement(self):
        """Ritorna (membro sinistro, relazione o None, membro destro o None)."""
        if "\\implies" in self.tok:
            while True:
                name = self.take()
                if not (len(name) == 1 and name.isupper()) or name in self.names:
                    raise Unreadable("definition of %r" % name)
                self.take("=")
                tree = self.sum()
                self.names[name] = tree
                if self.take() == "\\implies":
                    break
                if self.tok[self.pos - 1] != ",":
                    raise Unreadable("after a definition")
        left = self.sum()
        if self.peek() is None:
            return left, None, None
        relation = RELATIONS.get(self.take())
        if relation is None:
            raise Unreadable("relation")
        right = self.sum()
        if self.peek() is not None:
            raise Unreadable("text after the statement: %r" % self.peek())
        return left, relation, right

    def sum(self):
        if self.peek() == "-":
            self.take()
            tree = ("neg", self.term())
        else:
            if self.peek() == "+":
                self.take()
            tree = self.term()
        while self.peek() in ("+", "-"):
            tree = ("add" if self.take() == "+" else "sub", tree, self.term())
        return tree

    def starts_factor(self):
        t = self.peek()
        return t is not None and (t in ("\\begin", "(", "\\det", "\\operatorname", "\\mathrm", "\\text", "\\frac", "\\dfrac", "\\tfrac")
                                  or t.isdigit() or (len(t) == 1 and t.isalpha()) or (self.letters and t == "\\sqrt"))

    def term(self):
        tree = self.factor()
        while True:
            if self.peek() == "\\cdot":
                self.take()
                tree = ("mul", tree, self.factor())
            elif self.peek() == "/":
                self.take()
                tree = ("div", tree, self.factor())
            elif self.starts_factor():
                tree = ("mul", tree, self.factor())
            else:
                return tree

    def factor(self):
        tree = self.primary()
        while self.peek() == "^":
            self.take()
            braces = self.peek() == "{"
            if braces:
                self.take()
            t = self.peek()
            if t in ("T", "t", "\\top"):
                self.take()
                tree = ("T", tree)
            elif braces and t in ("\\mathrm", "\\mathsf"):
                self.take()
                if self.word() not in ("T", "t"):
                    raise Unreadable("exponent")
                tree = ("T", tree)
            else:
                negative = braces and t == "-"
                if negative:
                    self.take()
                digits = self.take()
                if not digits.isdigit():
                    raise Unreadable("exponent")
                if not braces and len(digits) > 1:
                    # Senza graffe TeX prende una cifra sola: il resto e un altro fattore.
                    self.tok.insert(self.pos, digits[1:])
                    digits = digits[0]
                tree = ("pow", tree, -int(digits) if negative else int(digits))
            if braces:
                self.take("}")
        return tree

    def group(self):
        if self.peek() == "{":
            self.take()
            tree = self.sum()
            self.take("}")
            return tree
        t = self.take()
        if not t.isdigit():
            raise Unreadable("group")
        if len(t) > 1:
            self.tok.insert(self.pos, t[1:])
        return ("num", Fr(int(t[0])))

    def primary(self):
        t = self.take()
        if t == "\\begin":
            name = self.word()
            if name not in ENVIRONMENTS:
                raise Unreadable("environment %r" % name)
            if name == "array":
                self.word_columns()
            rows, row = [], []
            while True:
                row.append(self.sum())
                if self.peek() == "&":
                    self.take()
                    continue
                rows.append(row)
                row = []
                if self.peek() == "\\\\":
                    self.take()
                    if self.peek() == "\\end":
                        break
                    continue
                break
            self.take("\\end")
            if self.word() != name or len({len(r) for r in rows}) != 1:
                raise Unreadable("matrix")
            tree = ("mat", tuple(tuple(r) for r in rows))
            return ("det", tree) if name == "vmatrix" else tree
        if t == "(":
            tree = self.sum()
            self.take(")")
            return tree
        if t == "\\det":
            return ("det", self.factor())
        if t in ("\\operatorname", "\\mathrm", "\\text"):
            name = FUNCTIONS.get(self.word())
            if name is None:
                raise Unreadable("function")
            return (name, self.factor())
        if t in ("\\frac", "\\dfrac", "\\tfrac"):
            above = self.group()
            return ("div", above, self.group())
        if t.isdigit():
            return ("num", Fr(int(t)))
        if len(t) == 1 and t.isupper():
            if t in self.names:
                return self.names[t]
            if t != "I":
                raise Unreadable("undefined matrix %r" % t)
            if self.peek() == "_":
                self.take()
                size = self.group()
                if size[0] != "num" or size[1] < 1:
                    raise Unreadable("identity size")
                return ("I", int(size[1]))
            return ("I", 0)
        if self.letters and len(t) == 1 and t.islower():
            return ("var", t)
        if self.letters and t == "\\sqrt":
            return ("sqrt", self.group())
        raise Unreadable("unexpected %r" % t)

    def word_columns(self):
        self.take("{")
        while self.take() != "}":
            pass


def is_zero_literal(tree):
    return tree == ("num", Fr(0))


def judge(text):
    """Che cosa dice l'oracolo dell'enunciato text: ("true",), ("false",), ("undefined", perche),
    ("value", v) per una richiesta, ("unreadable", perche)."""
    determinants[0] = 0
    try:
        left, relation, right = Reader(text).statement()
    except (Unreadable, IndexError) as problem:
        return ("unreadable", str(problem))
    try:
        if relation is None:
            v = value(left)
            if is_unit(v):
                raise Undefined("identity without a size")
            return ("value", v)
        truth = holds(value(left), value(right), relation, is_zero_literal(left), is_zero_literal(right))
        return ("true",) if truth else ("false",)
    except Undefined as problem:
        return ("undefined", str(problem))


# ---------------------------------------------------------------- la scrittura
def tex_number(q):
    q = Fr(q)
    if q.denominator == 1:
        return str(q.numerator)
    return "%s\\frac{%d}{%d}" % ("-" if q < 0 else "", abs(q.numerator), q.denominator)


def tex_value(v, rnd=None):
    if not is_matrix(v):
        return tex_number(v)
    env = rnd.choice(["pmatrix", "pmatrix", "bmatrix"]) if rnd else "pmatrix"
    return "\\begin{%s} %s \\end{%s}" % (env, " \\\\ ".join(" & ".join(tex_number(x) for x in row) for row in v), env)


def tex(t, rnd, parent=None):
    """Scrive l'albero t in LaTeX, con le varianti di notazione scelte a caso."""
    kind = t[0]
    if kind == "num":
        s = tex_number(t[1])
        return "(%s)" % s if t[1] < 0 and parent in ("mul", "pow", "fn", "neg", "right", "sub-right") else s
    if kind == "mat":
        env = rnd.choice(["pmatrix", "pmatrix", "bmatrix", "array"])
        body = " \\\\ ".join(" & ".join(tex(e, rnd) for e in row) for row in t[1])
        if env == "array":
            return "\\left( \\begin{array}{%s} %s \\end{array} \\right)" % ("c" * len(t[1][0]), body)
        return "\\begin{%s} %s \\end{%s}" % (env, body, env)
    if kind == "name":
        return t[1]
    if kind == "I":
        return "I_{%d}" % t[1] if t[2] else "I"
    if kind == "neg":
        s = "-" + tex(t[1], rnd, "neg")
        return "(%s)" % s if parent and parent != "left" else s
    if kind in ("add", "sub"):
        s = "%s %s %s" % (tex(t[1], rnd, "left"), "+" if kind == "add" else "-", tex(t[2], rnd, "sub-right" if kind == "sub" else "right"))
        return "\\left( %s \\right)" % s if parent in ("mul", "pow", "fn", "neg", "right", "sub-right") else s
    if kind == "mul":
        left, right = tex(t[1], rnd, "mul"), tex(t[2], rnd, "mul")
        # Due numeri accostati non si leggono come prodotto: tra loro ci vuole il punto.
        dot = left[-1].isdigit() and right[0].isdigit()
        s = "%s%s%s" % (left, " \\cdot " if dot else rnd.choice([" ", " \\cdot "]), right)
        return "\\left( %s \\right)" % s if parent in ("pow", "fn") else s
    if kind == "pow":
        return "%s^{%d}" % (tex(t[1], rnd, "pow"), t[2])
    if kind == "T":
        return "%s^%s" % (tex(t[1], rnd, "pow"), rnd.choice(["{T}", "T", "{\\top}", "{\\mathrm{T}}", "{t}"]))
    name = {"det": ["\\det"], "rank": ["\\operatorname{rank}", "\\mathrm{rk}", "\\operatorname{rg}"],
            "tr": ["\\operatorname{tr}", "\\mathrm{tr}"]}[kind]
    if kind == "det" and t[1][0] == "mat" and rnd.random() < 0.3:
        return "\\begin{vmatrix} %s \\end{vmatrix}" % " \\\\ ".join(" & ".join(tex(e, rnd) for e in row) for row in t[1][1])
    s = "%s %s" % (rnd.choice(name), tex(t[1], rnd, "fn"))
    return "\\left( %s \\right)" % s if parent in ("pow",) else s


def plain(t):
    """Lo stesso albero per value(): i nomi sostituiti, le identita come ("I", n) o ("I", 0)."""
    kind = t[0]
    if kind == "num":
        return t
    if kind == "mat":
        return ("mat", tuple(tuple(plain(e) for e in row) for row in t[1]))
    if kind == "name":
        return plain(t[2])
    if kind == "I":
        return ("I", t[1] if t[2] else 0)
    if kind == "pow":
        return ("pow", plain(t[1]), t[2])
    return (kind,) + tuple(plain(x) for x in t[1:])


# ---------------------------------------------------------------- il generatore
class Generator:
    def __init__(self, rnd):
        self.rnd = rnd

    def number(self):
        r = self.rnd
        if r.random() < 0.75:
            return Fr(r.randint(-5, 5))
        return Fr(r.randint(-7, 7), r.choice([2, 3, 4, 5]))

    def literal(self, rows, cols, low_rank=False):
        r = self.rnd
        m = [[self.number() for _ in range(cols)] for _ in range(rows)]
        if low_rank and rows > 1:
            # Una riga combinazione delle altre: ranghi piu bassi e determinanti nulli.
            target, a, b = r.randrange(rows), r.randint(-2, 2), r.randint(-2, 2)
            others = [i for i in range(rows) if i != target]
            first, second = others[0], others[-1]
            m[target] = [a * m[first][j] + b * m[second][j] for j in range(cols)]
        return ("mat", tuple(tuple(("num", x) for x in row) for row in m))

    def matrix(self, depth, rows, cols):
        """Un'espressione che vale una matrice rows x cols."""
        r = self.rnd
        if depth <= 0 or r.random() < 0.3:
            if rows == cols and r.random() < 0.12:
                return ("I", rows, True)
            return self.literal(rows, cols, low_rank=r.random() < 0.3)
        choice = r.random()
        if choice < 0.2:
            return (r.choice(["add", "sub"]), self.matrix(depth - 1, rows, cols), self.matrix(depth - 1, rows, cols))
        if choice < 0.45:
            inner = r.randint(1, 3)
            return ("mul", self.matrix(depth - 1, rows, inner), self.matrix(depth - 1, inner, cols))
        if choice < 0.58:
            pair = (self.scalar(depth - 1), self.matrix(depth - 1, rows, cols))
            return ("mul",) + (pair if r.random() < 0.7 else pair[::-1])
        if choice < 0.7:
            return ("T", self.matrix(depth - 1, cols, rows))
        if choice < 0.78:
            return ("neg", self.matrix(depth - 1, rows, cols))
        if rows == cols and choice < 0.93:
            return ("pow", self.matrix(depth - 1, rows, rows), r.choice([0, 2, 2, 3, -1, -1, -2]))
        if rows == cols and r.random() < 0.6:
            # L'identita senza pedice, dove una matrice accanto ne fissa la dimensione.
            unit = ("I", rows, False)
            if r.random() < 0.5:
                unit = ("mul", ("num", self.number()), unit)
            other = self.matrix(depth - 1, rows, cols)
            return (r.choice(["add", "sub", "mul"]),) + ((other, unit) if r.random() < 0.5 else (unit, other))
        return self.literal(rows, cols)

    def scalar(self, depth):
        """Un'espressione che vale un numero."""
        r = self.rnd
        if depth <= 0 or r.random() < 0.45:
            return ("num", self.number())
        choice = r.random()
        n = r.randint(1, 4)
        if choice < 0.35:
            return ("det", self.matrix(depth - 1, n, n))
        if choice < 0.6:
            return ("rank", self.matrix(depth - 1, r.randint(1, 4), r.randint(1, 4)))
        if choice < 0.75:
            return ("tr", self.matrix(depth - 1, n, n))
        if choice < 0.9:
            return (r.choice(["add", "sub", "mul"]), self.scalar(depth - 1), self.scalar(depth - 1))
        return ("pow", self.scalar(depth - 1), r.choice([0, 2, 3, -1]))

    def broken(self, depth):
        """Un'espressione in cui un'operazione non e definita."""
        r = self.rnd
        rows, cols = r.randint(1, 3), r.randint(1, 3)
        choice = r.random()
        if choice < 0.3:
            return ("add", self.matrix(depth, rows, cols), self.matrix(depth, rows + 1, cols))
        if choice < 0.6:
            return ("mul", self.matrix(depth, rows, cols), self.matrix(depth, cols + 1, rows))
        if choice < 0.8:
            return ("det", self.matrix(depth, rows, rows + 1))
        return ("pow", self.literal(max(rows, 2), max(rows, 2), low_rank=True), -1)


def wrong(v, rnd):
    """Un valore diverso da v, di poco. Ritorna None se non ne trova uno."""
    if not is_matrix(v):
        return v + rnd.choice([1, -1, Fr(1, 2), 2 * (-v) if v else 3])
    rows, cols = shape(v)
    m = [list(row) for row in v]
    choice = rnd.random()
    i, j = rnd.randrange(rows), rnd.randrange(cols)
    if choice < 0.45:
        m[i][j] += rnd.choice([1, -1, Fr(1, 2), Fr(-1, 3)])
    elif choice < 0.6:
        m[i][j] = -m[i][j]
    elif choice < 0.75:
        k, l = rnd.randrange(rows), rnd.randrange(cols)
        m[i][j], m[k][l] = m[k][l], m[i][j]
    elif choice < 0.88:
        m = [list(row) for row in transpose(v)]
    elif rows > 1:
        m = m[:-1]
    result = tuple(tuple(row) for row in m)
    return result if result != v else None


def name_parts(tree, rnd):
    """Da un nome a qualche matrice scritta dell'albero: ritorna (albero con i nomi, definizioni)."""
    definitions, letters = [], [c for c in "ABCDEFGHJKLMNPQRS"]
    def walk(t):
        if t[0] == "mat" and letters and rnd.random() < 0.6:
            name = letters.pop(0)
            definitions.append((name, t))
            return ("name", name, t)
        if t[0] in ("num", "mat", "I", "name"):
            return t
        if t[0] == "pow":
            return ("pow", walk(t[1]), t[2])
        return (t[0],) + tuple(walk(x) for x in t[1:])
    return walk(tree), definitions


def make_case(gen, rnd):
    """Ritorna (testo dell'enunciato, atteso): atteso e ("true",), ("false",), ("undefined",)
    oppure ("value", v)."""
    depth = rnd.choice([1, 1, 2, 2, 3])
    kind = rnd.random()
    if kind < 0.08:
        tree = gen.broken(depth - 1)
        text = tex(tree, rnd)
        if rnd.random() < 0.5:
            text += " = " + tex_value(identity(2), rnd)
        try:
            value(plain(tree))
        except Undefined:
            return text, ("undefined",)
        return None
    scalar = rnd.random() < 0.35
    tree = gen.scalar(depth) if scalar else gen.matrix(depth, rnd.randint(1, 4), rnd.randint(1, 4))
    definitions = []
    if rnd.random() < 0.3:
        tree, definitions = name_parts(tree, rnd)
    prefix = ", ".join("%s = %s" % (n, tex(d, rnd)) for n, d in definitions)
    prefix = prefix + " \\implies " if definitions else ""
    try:
        v = value(plain(tree))
    except Undefined:
        return prefix + tex(tree, rnd) + " = " + tex_value(identity(2), rnd), ("undefined",)
    if is_unit(v):
        return None
    text = prefix + tex(tree, rnd)
    if "\\begin" not in text:
        # Senza una matrice scritta l'enunciato non e di questo capitolo.
        return None
    if kind < 0.2:
        return text, ("value", v)
    if scalar and kind < 0.4:
        other = gen.scalar(depth - 1)
        try:
            w = value(plain(other))
        except Undefined:
            return None
        relation = rnd.choice(["<", ">", "\\le", "\\geq", "=", "\\neq"])
        truth = holds(v, w, RELATIONS[relation])
        return "%s %s %s" % (text, relation, tex(other, rnd)), ("true",) if truth else ("false",)
    bad = wrong(v, rnd)
    if kind < 0.6 or bad is None:
        if is_matrix(v) and rnd.random() < 0.15:
            return "%s - %s = 0" % (text, tex_value(v, rnd)), ("true",)
        return "%s = %s" % (text, tex_value(v, rnd)), ("true",)
    if kind < 0.85:
        if is_matrix(v) and shape(bad) == shape(v) and rnd.random() < 0.15:
            return "%s - %s = 0" % (text, tex_value(bad, rnd)), ("false",)
        return "%s = %s" % (text, tex_value(bad, rnd)), ("false",)
    if kind < 0.93:
        return "%s \\neq %s" % (text, tex_value(bad, rnd)), ("true",)
    return "%s \\ne %s" % (text, tex_value(v, rnd)), ("false",)


def damage(text, rnd):
    """Il testo di un enunciato con un token tolto, ripetuto, scambiato o sostituito."""
    # I pezzi sono i token e gli spazi tra loro: cosi il resto del testo resta com'e scritto.
    pieces = re.findall(r"\s+|" + TOKEN.pattern, text)
    places = [k for k, piece in enumerate(pieces) if not piece.isspace()]
    at = rnd.randrange(len(places))
    i = places[at]
    choice = rnd.random()
    if choice < 0.3:
        del pieces[i]
    elif choice < 0.5:
        pieces.insert(i, pieces[i] + " ")
    elif choice < 0.7 and at + 1 < len(places):
        j = places[at + 1]
        pieces[i], pieces[j] = pieces[j], pieces[i]
    else:
        pieces[i] = rnd.choice(["&", "\\\\", "^", "{", "}", "(", ")", "-", "+", "=", "I", "0", "2", "T", "\\cdot ", "\\det ", "\\neq ", "_"])
    return "".join(pieces)


# ------------------------------------------------------- i certificati
def sexp(text):
    tokens = re.findall(r"\(|\)|[^\s()]+", text)
    def parse(i):
        if tokens[i] == "(":
            out, i = [], i + 1
            while tokens[i] != ")":
                node, i = parse(i)
                out.append(node)
            return out, i + 1
        if tokens[i] == ")":
            raise Unreadable("unbalanced")
        return tokens[i], i + 1
    node, used = parse(0)
    if used != len(tokens):
        raise Unreadable("text after the expression")
    return node


def integer(token):
    if not isinstance(token, str) or not re.fullmatch(r"-?\d+", token):
        raise Unreadable("integer expected")
    return int(token)


class Witnesses:
    """Le voci witness di un certificato, nell'ordine in cui sono scritte: ogni det, rank e
    inv dell'enunciato prende la prossima."""

    def __init__(self, entries):
        self.entries, self.used = entries, 0

    def take(self, kind):
        if self.used == len(self.entries):
            raise Undefined("a witness is missing")
        entry = self.entries[self.used]
        self.used += 1
        if entry[0] != kind:
            raise Undefined("a witness of another kind")
        return entry[1:]


# Quante volte un testimone ha superato i suoi prodotti e l'oracolo, per la sua strada, ha
# trovato un altro numero. Non deve succedere mai: sarebbe falso uno dei teoremi su cui il
# verificatore si appoggia, o sbagliato l'oracolo.
contradictions = [0]


def contradiction(what, a, found, own):
    contradictions[0] += 1
    print("BUG: the witness of a %s passes its products and gives %s, but the oracle computes %s for %s" % (what, found, own, a))


def witness_matrix(node, rows, cols):
    """Una matrice di un testimone: scritta senza det, rank e inv, e delle dimensioni date."""
    m = cert_value(node, None)
    if not is_matrix(m) or shape(m) != (rows, cols):
        raise Undefined("a matrix of a witness has the wrong shape")
    return m


def permutation_sign(p):
    """Il segno di una permutazione di 0..n-1 dai suoi cicli: un ciclo di lunghezza k vale
    k - 1 scambi. (Il verificatore conta le inversioni: e un'altra strada.)"""
    seen, sign = set(), 1
    for start in range(len(p)):
        length, i = 0, start
        while i not in seen:
            seen.add(i)
            i = p[i]
            length += 1
        if length and length % 2 == 0:
            sign = -sign
    return sign


def witnessed_inverse(a, entry):
    """witness inverse W: W deve essere l'inversa di a, che l'oracolo calcola con i
    complementi algebrici."""
    if shape(a)[0] != shape(a)[1]:
        raise Undefined("inverse of a non-square matrix")
    if len(entry) != 1:
        raise Unreadable("an inverse witness is one matrix")
    w = witness_matrix(entry[0], *shape(a))
    if mat_mul(a, w) != identity(len(a)):
        raise Undefined("the witness is not the inverse")
    own = inverse(a)
    if own != w:
        contradiction("inverse", a, w, own)
        raise Undefined("contradiction")
    return w


def witnessed_det(a, entry):
    """witness lu p1 ... pn L U: p una permutazione, L triangolare inferiore con 1 sulla
    diagonale, U triangolare superiore, e le righe di a nell'ordine p uguali a L U. Il
    valore e quello dello sviluppo di Laplace, che deve coincidere con segno(p) per la
    diagonale di U."""
    n, c = shape(a)
    if n != c:
        raise Undefined("determinant of a non-square matrix")
    if len(entry) != n + 2:
        raise Unreadable("an lu witness is n integers and two matrices")
    p = [integer(t) - 1 for t in entry[:n]]
    if sorted(p) != list(range(n)):
        raise Undefined("not a permutation")
    low, up = witness_matrix(entry[n], n, n), witness_matrix(entry[n + 1], n, n)
    for i in range(n):
        for j in range(n):
            if (j > i and low[i][j] != 0) or (j == i and low[i][j] != 1) or (j < i and up[i][j] != 0):
                raise Undefined("L or U has not its shape")
    if mat_mul(low, up) != tuple(a[i] for i in p):
        raise Undefined("L U is not the matrix with its rows permuted")
    found = Fr(permutation_sign(p))
    for i in range(n):
        found *= up[i][i]
    own = det(a)
    if own != found:
        contradiction("determinant", a, found, own)
        raise Undefined("contradiction")
    return own


def witnessed_rank(a, entry):
    """witness rank r C F G H: a = C F e G a H = I, con le dimensioni che r chiede; witness
    rank 0: a e nulla. Il valore e il piu grande ordine di un minore non nullo, che deve
    coincidere con r."""
    rows, cols = shape(a)
    if not entry:
        raise Unreadable("a rank witness begins with the rank")
    r = integer(entry[0])
    if r < 0 or r > min(rows, cols):
        raise Undefined("a rank that the matrix cannot have")
    if r == 0:
        if len(entry) != 1:
            raise Unreadable("rank 0 has no matrices")
        if any(x != 0 for row in a for x in row):
            raise Undefined("rank 0 of a matrix that is not zero")
    else:
        if len(entry) != 5:
            raise Unreadable("a rank witness is the rank and four matrices")
        c, f = witness_matrix(entry[1], rows, r), witness_matrix(entry[2], r, cols)
        g, h = witness_matrix(entry[3], r, rows), witness_matrix(entry[4], cols, r)
        if mat_mul(c, f) != a:
            raise Undefined("C F is not the matrix")
        if mat_mul(mat_mul(g, a), h) != identity(r):
            raise Undefined("G A H is not the identity")
    own = rank(a)
    if own != r:
        contradiction("rank", a, r, own)
        raise Undefined("contradiction")
    return Fr(r)


# Le lettere dichiarate dal certificato che si sta rileggendo (la voce letters), o None.
cert_letters = [None]


def cert_value(node, witnesses):
    """Il valore di un'espressione della notazione del capitolo 17, ricalcolato qui.
    witnesses sono le voci witness ancora da usare; None dentro un testimone, dove det,
    rank e inv non si possono scrivere. Con le lettere dichiarate una lettera vale se
    stessa, il determinante e lo sviluppo di Laplace sui polinomi, e non ci sono ne
    testimoni, ne ranghi, ne inverse."""
    letters = cert_letters[0]
    if isinstance(node, str):
        if letters is not None and node in letters:
            return Poly.letter(node)
        return Fr(integer(node))
    if not node or not isinstance(node[0], str):
        raise Unreadable("operator expected")
    op, args = node[0], node[1:]
    def arity(n):
        if len(args) != n:
            raise Unreadable("wrong number of operands of %s" % op)
    def number(x):
        if is_matrix(x):
            raise Undefined("a number was expected")
        return x
    def matrix(x):
        if not is_matrix(x):
            raise Undefined("a matrix was expected")
        return x
    if op == "matrix":
        rows, cols = integer(args[0]), integer(args[1])
        if rows < 1 or cols < 1 or len(args) != 2 + rows * cols:
            raise Unreadable("matrix size")
        flat = [number(cert_value(a, witnesses)) for a in args[2:]]
        return tuple(tuple(flat[i * cols + j] for j in range(cols)) for i in range(rows))
    if op == "identity":
        arity(1)
        if integer(args[0]) < 1:
            raise Unreadable("identity size")
        return identity(integer(args[0]))
    if op == "zero":
        arity(2)
        if integer(args[0]) < 1 or integer(args[1]) < 1:
            raise Unreadable("zero size")
        return tuple(tuple(Fr(0) for _ in range(integer(args[1]))) for _ in range(integer(args[0])))
    if op == "^":
        arity(2)
        if integer(args[1]) < 0:
            raise Unreadable("negative exponent")
        return power(cert_value(args[0], witnesses), integer(args[1]))
    if op == "neg":
        arity(1)
        return mul(Fr(-1), cert_value(args[0], witnesses))
    if op in ("+", "-"):
        arity(2)
        a = cert_value(args[0], witnesses)
        return add(a, cert_value(args[1], witnesses), 1 if op == "+" else -1)
    if op == "*":
        arity(2)
        a = cert_value(args[0], witnesses)
        return mul(a, cert_value(args[1], witnesses))
    if op == "/":
        arity(2)
        a = number(cert_value(args[0], witnesses))
        b = settled(number(cert_value(args[1], witnesses)))
        if isinstance(b, Poly):
            raise Unreadable("division by a letter")
        if b == 0:
            raise Undefined("division by zero")
        return a / b
    if op not in ("transpose", "trace", "det", "rank", "inv"):
        raise Unreadable("unknown word %s" % op)
    arity(1)
    a = matrix(cert_value(args[0], witnesses))
    if op == "transpose":
        return transpose(a)
    if op == "trace":
        if shape(a)[0] != shape(a)[1]:
            raise Undefined("trace of a non-square matrix")
        return sum((a[i][i] for i in range(len(a))), Fr(0))
    if letters is not None:
        if op != "det":
            raise Unreadable("%s in a certificate with letters" % op)
        return det(a)

    # Le tre parole che vogliono un testimone: prima l'operando, che ha gia preso i suoi,
    # poi la prossima voce.
    if witnesses is None:
        raise Unreadable("%s inside a witness" % op)
    if op == "det":
        if shape(a)[0] != shape(a)[1]:
            raise Undefined("determinant of a non-square matrix")
        return witnessed_det(a, witnesses.take("lu"))
    if op == "rank":
        return witnessed_rank(a, witnesses.take("rank"))
    if shape(a)[0] != shape(a)[1]:
        raise Undefined("inverse of a non-square matrix")
    return witnessed_inverse(a, witnesses.take("inverse"))


def cert_judge(text):
    """Ritorna True se il certificato dice il vero e ogni suo testimone e quello che deve
    essere; False se dice il falso, parla di un'espressione senza valore, o ha un testimone
    che manca, che avanza o che non torna; solleva Unreadable se non e scritto nella
    notazione."""
    determinants[0] = 0
    cert_letters[0] = None
    lines = [l for l in text.strip().split("\n") if l.strip()]
    if len(lines) < 4 or lines[0].split() != ["certificate", "matrices"] or lines[-1].strip() != "end":
        raise Unreadable("entries")
    if lines[1].split()[:1] == ["letters"]:
        return cert_judge_letters(lines)
    head, claim = lines[1].split(None, 1), lines[2].split()
    if len(head) != 2 or head[0] != "statement" or len(claim) != 2 or claim[0] != "claim" or claim[1] not in ("true", "false"):
        raise Unreadable("entries")
    entries = []
    for line in lines[3:-1]:
        entry = sexp("(" + line + ")")
        if len(entry) < 2 or entry[0] != "witness" or entry[1] not in ("inverse", "lu", "rank"):
            raise Unreadable("witness entry")
        entries.append(entry[1:])
    node = sexp(head[1])
    if not isinstance(node, list) or len(node) != 3 or node[0] not in ("=", "neq", "<", "<="):
        raise Unreadable("relation")
    witnesses = Witnesses(entries)
    try:
        left = cert_value(node[1], witnesses)
        truth = holds(left, cert_value(node[2], witnesses), node[0])
    except Undefined:
        return False
    return witnesses.used == len(entries) and truth == (claim[1] == "true")


def cert_judge_letters(lines):
    """Un certificato con la voce letters: True se dichiara vera un'uguaglianza e i due
    membri, sviluppati qui, sono lo stesso polinomio (o matrici con gli stessi polinomi);
    False se non lo sono, se un'operazione non e definita, se dichiara altro o ha dei
    testimoni; solleva Unreadable se non e scritto nella notazione."""
    letters = lines[1].split()[1:]
    if not letters or len(set(letters)) != len(letters) or any(len(x) != 1 or not x.isalpha() for x in letters) or len(lines) < 5:
        raise Unreadable("letters")
    head, claim = lines[2].split(None, 1), lines[3].split()
    if len(head) != 2 or head[0] != "statement" or claim != ["claim", "true"] or len(lines) != 5:
        return False
    node = sexp(head[1])
    if not isinstance(node, list) or len(node) != 3 or node[0] != "=":
        return False
    cert_letters[0] = letters
    try:
        left, right = cert_value(node[1], None), cert_value(node[2], None)
    except Undefined:
        return False
    finally:
        cert_letters[0] = None
    if is_matrix(left) != is_matrix(right):
        return False
    if not is_matrix(left):
        return Poly.of(left) == Poly.of(right)
    return shape(left) == shape(right) and all(Poly.of(x) == Poly.of(y) for r, s in zip(left, right) for x, y in zip(r, s))


def join(tokens):
    """Riscrive i token di un certificato, o di una sua riga, con gli spazi al loro posto."""
    out = ""
    for t in tokens:
        out += t if t in (")", "\n") or not out or out[-1] in ("(", "\n") else " " + t
    return out


def groups(tokens):
    """Gli intervalli [inizio, fine) delle parentesi piu esterne di una riga a token."""
    found, depth, start = [], 0, 0
    for i, t in enumerate(tokens):
        if t == "(":
            if depth == 0:
                start = i
            depth += 1
        elif t == ")":
            depth -= 1
            if depth == 0:
                found.append((start, i + 1))
    return found


def cert_matrix(m):
    """Scrive una matrice di frazioni nella notazione dei certificati."""
    numbers = [str(x.numerator) if x.denominator == 1 else "(/ %d %d)" % (x.numerator, x.denominator) for row in m for x in row]
    return "(matrix %d %d %s)" % (len(m), len(m[0]), " ".join(numbers))


def forge(line, rnd):
    """Un testimone falso costruito apposta, che supera tutti i controlli del verificatore
    tranne uno: cambiare un numero a caso non basta a provare i controlli sulla forma,
    perché rompe quasi sempre anche un prodotto. Ritorna la voce nuova, o None.

      - rank r ...: "rank 0", che vale solo per la matrice nulla;
      - lu p L U, con B = L U le righe permutate: "lu p I B", dove il prodotto torna ma B
        non e triangolare; oppure la prima riga di B ripetuta n volte, "lu q ... q L' U'"
        con L' che ha 1 sulla diagonale e nella prima colonna e U' che ha solo la prima
        riga: il prodotto torna, ma q ... q non e una permutazione; oppure L' = L E e
        U' = E^-1 U, con E l'identita piu c nel posto (j, n): il prodotto torna, U' resta
        triangolare con la stessa diagonale, L' ha ancora 1 sulla diagonale (se l'elemento
        (n, j) di L e zero) ma ha c sopra la diagonale."""
    entry = sexp("(" + line + ")")[1:]
    if entry[0] == "rank":
        return None if entry[1] == "0" else "witness rank 0"
    if entry[0] != "lu":
        return None
    n = len(entry) - 3
    try:
        low, up = cert_value(entry[-2], None), cert_value(entry[-1], None)
        b = mat_mul(low, up)
    except (Undefined, Unreadable):
        return None
    order, choice = " ".join(entry[1:n + 1]), rnd.random()
    free = [j for j in range(n - 1) if low[n - 1][j] == 0]
    if n >= 2 and choice < 0.35 and free:
        j, c = rnd.choice(free), Fr(rnd.choice([1, -1, 2]))
        low = tuple(tuple(x + c * row[j] if k == n - 1 else x for k, x in enumerate(row)) for row in low)
        up = tuple(tuple(x - c * y for x, y in zip(row, up[n - 1])) if i == j else row for i, row in enumerate(up))
        return "witness lu %s %s %s" % (order, cert_matrix(low), cert_matrix(up))
    if n < 2 or choice < 0.7:
        return "witness lu %s %s %s" % (order, cert_matrix(identity(n)), cert_matrix(b))
    low = tuple(tuple(Fr(1) if j in (0, i) else Fr(0) for j in range(n)) for i in range(n))
    up = (b[0],) + tuple(tuple(Fr(0) for _ in range(n)) for _ in range(n - 1))
    return "witness lu %s %s %s" % (" ".join([entry[1]] * n), cert_matrix(low), cert_matrix(up))


def mutate_witness(lines, where, rnd, pool):
    """Altera la voce witness lines[where]: un numero, il rango dichiarato, la permutazione,
    due matrici scambiate, il tipo; oppure la toglie, la ripete, la scambia con un'altra, o
    mette al suo posto il testimone di un'altra matrice (uno di pool)."""
    tokens = re.findall(r"\(|\)|[^\s()]+", lines[where])
    kind, how = tokens[1], rnd.random()
    numbers = [i for i in range(2, len(tokens)) if re.fullmatch(r"-?\d+", tokens[i])]
    leading = [i for i in range(2, len(tokens)) if "(" not in tokens[2:i + 1]]
    blocks = groups(tokens)
    others = [i for i, l in enumerate(lines) if l.startswith("witness ") and i != where]
    if how < 0.3 and numbers:
        i = rnd.choice(numbers)
        n = int(tokens[i])
        tokens[i] = str(rnd.choice([n + 1, n - 1, -n, 0, 2 * n, n + 7]))
    elif how < 0.42 and kind == "rank" and leading:
        tokens[2] = str(int(tokens[2]) + rnd.choice([1, -1]))
    elif how < 0.42 and kind == "lu" and len(leading) > 1:
        # La permutazione: due righe scambiate, una riga due volte, una che non c'e.
        i, j = rnd.sample(leading, 2)
        way = rnd.random()
        if way < 0.5:
            tokens[i], tokens[j] = tokens[j], tokens[i]
        elif way < 0.8:
            tokens[i] = tokens[j]
        else:
            tokens[i] = str(rnd.choice([0, len(leading) + 1]))
    elif how < 0.52 and len(blocks) > 1:
        # Due matrici scambiate: L con U, C con F, G con H.
        (a, b), (c, d) = sorted(rnd.sample(blocks, 2))
        tokens = tokens[:a] + tokens[c:d] + tokens[b:c] + tokens[a:b] + tokens[d:]
    elif how < 0.6:
        tokens[1] = rnd.choice(["inverse", "lu", "rank"])
    elif how < 0.7:
        del lines[where]
        return
    elif how < 0.76:
        lines.insert(where, lines[where])
        return
    elif how < 0.84 and others:
        other = rnd.choice(others)
        lines[where], lines[other] = lines[other], lines[where]
        return
    elif pool:
        lines[where] = rnd.choice(pool)
        return
    lines[where] = join(tokens)


def mutate(cert, rnd, pool):
    """Un certificato alterato in un punto. pool sono le voci witness di tutti i
    certificati del programma: i testimoni di altre matrici."""
    lines = cert.rstrip("\n").split("\n")
    witnesses = [i for i, l in enumerate(lines) if l.startswith("witness ")]
    choice = rnd.random()
    if witnesses and choice < 0.12:
        where = rnd.choice(witnesses)
        forged = forge(lines[where], rnd)
        if forged:
            lines[where] = forged
            return "\n".join(lines) + "\n"
    if witnesses and choice < 0.4:
        mutate_witness(lines, rnd.choice(witnesses), rnd, pool)
        return "\n".join(lines) + "\n"
    tokens = re.findall(r"\(|\)|\n|[^\s()]+", cert)
    places = [i for i, t in enumerate(tokens) if t not in ("\n", "certificate", "matrices", "statement", "claim", "end", "witness")]
    witnessed = [i for i, t in enumerate(tokens) if t in ("det", "rank", "inv") and tokens[i - 1] == "("]
    choice = rnd.random()
    if witnessed and rnd.random() < 0.25:
        # Un numero cambiato nella matrice di cui un testimone parla: il testimone resta
        # quello di prima, e solo i suoi prodotti possono accorgersene.
        start = rnd.choice(witnessed) + 1
        depth, end = 0, start
        while end < len(tokens):
            depth += tokens[end] == "("
            depth -= tokens[end] == ")"
            end += 1
            if depth == 0:
                break
        numbers = [i for i in range(start, end) if re.fullmatch(r"-?\d+", tokens[i]) and tokens[i - 1] != "matrix" and tokens[i - 2] != "matrix"]
        if numbers:
            i = rnd.choice(numbers)
            tokens[i] = str(int(tokens[i]) + rnd.choice([1, -1, 2]))
    elif choice < 0.15:
        tokens = [("false" if t == "true" else "true") if t in ("true", "false") else t for t in tokens]
    elif choice < 0.3:
        relations = [i for i, t in enumerate(tokens) if i and tokens[i - 1] == "(" and t in ("=", "neq", "<", "<=") and i < 6]
        if relations:
            tokens[relations[0]] = rnd.choice(["=", "neq", "<", "<="])
    elif choice < 0.7:
        numbers = [i for i in places if re.fullmatch(r"-?\d+", tokens[i])]
        if numbers:
            i = rnd.choice(numbers)
            n = int(tokens[i])
            tokens[i] = str(rnd.choice([n + 1, n - 1, -n, 0, 2 * n, n + 7]))
    elif choice < 0.78:
        # La stessa tabella con un'altra forma: le due dimensioni scambiate, o una riga sola.
        tables = [i for i in places if tokens[i] == "matrix" and i + 2 < len(tokens) and tokens[i + 1].isdigit() and tokens[i + 2].isdigit()]
        if tables:
            i = rnd.choice(tables)
            rows, cols = int(tokens[i + 1]), int(tokens[i + 2])
            tokens[i + 1], tokens[i + 2] = (str(cols), str(rows)) if rows != cols and rnd.random() < 0.6 else ("1", str(rows * cols))
    elif choice < 0.88:
        words = [i for i in places if tokens[i] in ("det", "rank", "trace", "transpose", "+", "-", "*", "neg", "inv", "identity", "zero", "^", "/")]
        if words:
            i = rnd.choice(words)
            tokens[i] = rnd.choice(["det", "rank", "trace", "transpose", "+", "-", "*", "neg", "inv"])
    elif places:
        i = rnd.choice(places)
        if rnd.random() < 0.5:
            del tokens[i]
        else:
            tokens.insert(i, tokens[i])
    out = join(tokens)
    return out if out.endswith("\n") else out + "\n"


# ------------------------------------------------- gli enunciati con una lettera
#
# Un enunciato con una lettera e un quantificatore, o una richiesta di soluzioni, su una
# condizione fatta di confronti tra polinomi nella lettera: i determinanti e le tracce si
# sviluppano qui con Laplace sui polinomi (det(), con gli elementi Poly). La condizione si
# decide poi con una tabella: tra due radici consecutive dei polinomi che vi compaiono
# ogni confronto ha un valore solo, quindi basta guardarla in ogni radice e in un punto di
# ogni intervallo. Le radici si separano con il teorema di Sturm, con frazioni esatte: non
# ci sono numeri in virgola mobile.

class Skip(Exception):
    """L'enunciato e nel linguaggio, ma questo oracolo non sa deciderlo."""


def u_trim(p):
    while p and p[-1] == 0:
        p = p[:-1]
    return p


def u_eval(p, x):
    total = Fr(0)
    for c in reversed(p):
        total = total * x + c
    return total


def u_rem(a, b):
    """Il resto della divisione di a per b, polinomi come liste dal termine noto."""
    a = list(a)
    while len(a) >= len(b):
        q = a[-1] / b[-1]
        for i in range(len(b)):
            a[len(a) - len(b) + i] -= q * b[i]
        a = u_trim(a[:-1])
    return u_trim(a)


def u_gcd(a, b):
    while b:
        a, b = b, u_rem(a, b)
    return a


def u_quo(a, b):
    """Il quoziente esatto di a per b."""
    a, out = list(a), [Fr(0)] * (len(a) - len(b) + 1)
    while len(a) >= len(b):
        q = a[-1] / b[-1]
        out[len(a) - len(b)] = q
        for i in range(len(b)):
            a[len(a) - len(b) + i] -= q * b[i]
        a = u_trim(a[:-1])
    return out


def u_mul(a, b):
    out = [Fr(0)] * (len(a) + len(b) - 1)
    for i, x in enumerate(a):
        for j, y in enumerate(b):
            out[i + j] += x * y
    return out


def sturm(p):
    """La catena di Sturm di p: p, la derivata, e poi ogni resto cambiato di segno."""
    chain = [p, u_trim([i * c for i, c in enumerate(p)][1:])]
    while chain[-1]:
        chain.append([-c for c in u_rem(chain[-2], chain[-1])])
    return chain[:-1]


def changes(chain, x):
    """I cambi di segno della catena in x; x = None e +infinito, x = "-" e -infinito."""
    signs = []
    for q in chain:
        if x is None or x == "-":
            v = q[-1] * (-1 if x == "-" and len(q) % 2 == 0 else 1)
        else:
            v = u_eval(q, x)
        if v != 0:
            signs.append(v > 0)
    return sum(1 for a, b in zip(signs, signs[1:]) if a != b)


def roots_between(chain, lo, hi):
    """Quante radici reali distinte ha il primo polinomio della catena in (lo, hi] (teorema
    di Sturm); lo e hi non sono radici, oppure sono -infinito e +infinito."""
    return changes(chain, lo) - changes(chain, hi)


def isolate(p):
    """Le radici reali distinte del polinomio p, senza radici multiple, in ordine: ognuna
    come (lo, hi), con lo = hi se e razionale e trovata esatta, altrimenti un intervallo
    con gli estremi razionali che non sono radici e che ne contiene una sola."""
    chain = sturm(p)
    bound = 1 + max(abs(c / p[-1]) for c in p)  # limite di Cauchy: ogni radice sta in (-bound, bound)
    found, todo = [], [(-bound, bound)]
    while todo:
        lo, hi = todo.pop()
        count = roots_between(chain, lo, hi)
        if count == 0:
            continue
        mid = (lo + hi) / 2
        if u_eval(p, mid) == 0:
            # Una radice razionale: la si toglie con un intorno senza altre radici.
            found.append((mid, mid))
            width = (hi - lo) / 4
            while roots_between(chain, mid - width, mid + width) != 1 or u_eval(p, mid - width) == 0:
                width /= 2
            todo += [(lo, mid - width), (mid + width, hi)]
        elif count == 1:
            found.append((lo, hi))
        else:
            todo += [(lo, mid), (mid, hi)]
    return sorted(found)


class Surd:
    """Un numero a + b sqrt(d), con a e b razionali e d intero positivo senza quadrati: le
    costanti con una radice quadrata che compaiono nelle soluzioni scritte."""

    def __init__(self, a, b=Fr(0), d=1):
        self.a, self.b, self.d = Fr(a), Fr(b), d if b != 0 else 1

    @staticmethod
    def of(x):
        return x if isinstance(x, Surd) else Surd(x)

    @staticmethod
    def root(q):
        """La radice quadrata del razionale non negativo q."""
        if q < 0:
            raise Unreadable("the square root of a negative number")
        n, out, f = q.numerator * q.denominator, 1, 2
        while f * f <= n:
            while n % (f * f) == 0:
                n //= f * f
                out *= f
            f += 1
        return Surd(Fr(out, q.denominator)) if n == 1 else Surd(0, Fr(out, q.denominator), n)

    def same(self, other):
        if self.b != 0 and other.b != 0 and self.d != other.d:
            raise Skip("two different square roots in one number")
        return self.d if self.b != 0 else other.d

    def __add__(self, other):
        other = Surd.of(other)
        return Surd(self.a + other.a, self.b + other.b, self.same(other))

    def __neg__(self):
        return Surd(-self.a, -self.b, self.d)

    def __sub__(self, other):
        return self + (-Surd.of(other))

    def __mul__(self, other):
        other = Surd.of(other)
        d = self.same(other)
        return Surd(self.a * other.a + self.b * other.b * d, self.a * other.b + self.b * other.a, d)

    def inverse(self):
        norm = self.a * self.a - self.b * self.b * self.d
        if norm == 0:
            raise Undefined("division by zero")
        return Surd(self.a / norm, -self.b / norm, self.d)

    def sign(self):
        """Il segno di a + b sqrt(d): se a e b tirano da parti opposte decide chi e piu
        grande, e lo dicono i quadrati."""
        sa, sb = (self.a > 0) - (self.a < 0), (self.b > 0) - (self.b < 0)
        if sa == sb or sb == 0 or sa == 0:
            return sa or sb
        return sb if self.b * self.b * self.d > self.a * self.a else sa


def surd_value(tree, env):
    """Il valore di un'espressione senza la variabile: un numero di Surd. env da i valori
    delle altre lettere."""
    kind = tree[0]
    if kind == "num":
        return Surd(tree[1])
    if kind == "var":
        if tree[1] not in env:
            raise Unreadable("a letter in a constant")
        return Surd(env[tree[1]])
    if kind == "sqrt":
        inner = surd_value(tree[1], env)
        if inner.b != 0:
            raise Skip("a root inside a root")
        return Surd.root(inner.a)
    if kind == "neg":
        return -surd_value(tree[1], env)
    if kind in ("add", "sub"):
        a, b = surd_value(tree[1], env), surd_value(tree[2], env)
        return a + b if kind == "add" else a - b
    if kind == "mul":
        return surd_value(tree[1], env) * surd_value(tree[2], env)
    if kind == "div":
        return surd_value(tree[1], env) * surd_value(tree[2], env).inverse()
    if kind == "pow" and tree[2] >= 0:
        out = Surd(1)
        for _ in range(tree[2]):
            out = out * surd_value(tree[1], env)
        return out
    raise Unreadable("a constant with %s" % kind)


def mentions(tree, what):
    """Ritorna 1 se nell'albero di un'espressione c'e un nodo del tipo what, o la lettera what."""
    if not isinstance(tree, tuple):
        return False
    if tree[0] == what or (tree[0] == "var" and tree[1] == what):
        return True
    if tree[0] == "mat":
        return any(mentions(e, what) for row in tree[1] for e in row)
    return any(mentions(t, what) for t in tree[1:])


LOGIC_RELATIONS = {"=": "=", "\\neq": "neq", "\\ne": "neq", "<": "<", ">": ">", "\\le": "<=", "\\leq": "<=", "\\ge": ">=", "\\geq": ">="}


class LetterReader:
    """Il lettore degli enunciati con una lettera: i quantificatori e i connettivi qui, le
    espressioni con il Reader delle matrici, che legge anche le lettere.

        enunciato := [definizioni \\implies] quantificato
        quantificato := (\\forall | \\exists) x \\in \\mathbb{R} quantificato
                      | \\{ x \\in \\mathbb{R} \\mid formula \\} | formula
        formula := disgiunzione [(\\iff | \\implies) disgiunzione]
        disgiunzione := congiunzione { \\lor congiunzione }
        congiunzione := parte { \\land parte }
        parte := (\\lnot | \\neg) parte | '(' formula ')' | espressione relazione espressione [relazione espressione]
    """

    def __init__(self, text):
        self.r = Reader(text)
        self.r.letters = True

    def statement(self):
        r = self.r
        if r.peek() is not None and len(r.peek()) == 1 and r.peek().isupper():
            while True:
                name = r.take()
                if not (len(name) == 1 and name.isupper()) or name in r.names:
                    raise Unreadable("definition of %r" % name)
                r.take("=")
                r.names[name] = r.sum()
                if r.take() == "\\implies":
                    break
                if r.tok[r.pos - 1] != ",":
                    raise Unreadable("after a definition")
        tree = self.quantified()
        if r.peek() is not None:
            raise Unreadable("text after the statement: %r" % r.peek())
        return tree

    def binder(self):
        r = self.r
        name = r.take()
        if not (len(name) == 1 and name.islower()):
            raise Unreadable("the letter of a quantifier")
        for expected in ("\\in", "\\mathbb", "{", "R", "}"):
            r.take(expected)
        return name

    def quantified(self):
        r = self.r
        if r.peek() in ("\\forall", "\\exists"):
            kind = r.take()[1:]
            name = self.binder()
            return (kind, name, self.quantified())
        if r.peek() == "\\{":
            r.take()
            name = self.binder()
            r.take("\\mid")
            body = self.formula()
            r.take("\\}")
            return ("solve", name, body)
        return self.formula()

    def formula(self):
        left = self.disjunction()
        if self.r.peek() in ("\\iff", "\\implies"):
            return (self.r.take()[1:], left, self.disjunction())
        return left

    def disjunction(self):
        tree = self.conjunction()
        while self.r.peek() == "\\lor":
            self.r.take()
            tree = ("or", tree, self.conjunction())
        return tree

    def conjunction(self):
        tree = self.part()
        while self.r.peek() == "\\land":
            self.r.take()
            tree = ("and", tree, self.part())
        return tree

    def part(self):
        r = self.r
        if r.peek() in ("\\lnot", "\\neg"):
            r.take()
            return ("not", self.part())
        if r.peek() == "(":
            # Una formula tra parentesi, se dopo la parentesi chiusa non continua
            # un'espressione; altrimenti la parentesi apriva un'espressione.
            start = r.pos
            try:
                r.take()
                inner = self.formula()
                r.take(")")
                if r.peek() not in LOGIC_RELATIONS and r.peek() != "^" and not r.starts_factor() and r.peek() not in ("+", "-", "\\cdot", "/"):
                    return inner
            except Unreadable:
                pass
            r.pos = start
        first = r.sum()
        relation = LOGIC_RELATIONS.get(r.take())
        if relation is None:
            raise Unreadable("relation")
        second = r.sum()
        tree = ("rel", relation, first, second)
        if r.peek() in LOGIC_RELATIONS:
            tree = ("and", tree, ("rel", LOGIC_RELATIONS[r.take()], second, r.sum()))
        return tree


def atoms_of(f, x, env, out):
    """Raccoglie in out i confronti della formula f, ognuno ridotto a un polinomio nella
    lettera x o a una costante con una radice: ("poly", relazione, p) vuol dire
    "p(x) relazione 0", ("surd", relazione, c) vuol dire "x relazione c". Ritorna la formula
    con, al posto di ogni confronto, il suo numero in out."""
    if f[0] == "rel":
        _, relation, left, right = f
        if mentions(left, "sqrt") or mentions(right, "sqrt"):
            # Una costante con una radice: dall'altra parte deve esserci la lettera sola.
            if left == ("var", x) and not mentions(right, x):
                out.append(("surd", relation, surd_value(right, env)))
            elif right == ("var", x) and not mentions(left, x):
                flipped = {"<": ">", ">": "<", "<=": ">=", ">=": "<="}.get(relation, relation)
                out.append(("surd", flipped, surd_value(left, env)))
            else:
                raise Skip("a root that is not alone beside the letter")
        else:
            a, b = settled(value(left)), settled(value(right))
            if is_matrix(a) or is_matrix(b) or is_unit(a) or is_unit(b):
                raise Unreadable("a matrix compared in a statement with a letter")
            p = Poly.of(a) - Poly.of(b)
            for name, v in env.items():
                p = p.at(name, v)
            out.append(("poly", relation, u_trim(p.coefficients(x))))
        return ("atom", len(out) - 1)
    if f[0] in ("forall", "exists", "solve"):
        raise Skip("a quantifier inside a formula")
    return (f[0],) + tuple(atoms_of(g, x, env, out) for g in f[1:])


def related(sign, relation):
    return {"=": sign == 0, "neq": sign != 0, "<": sign < 0, "<=": sign <= 0, ">": sign > 0, ">=": sign >= 0}[relation]


def table(f, x, env):
    """La verita della formula f nella lettera x, lungo tutta la retta: la lista dei suoi
    valori nelle radici e in un punto di ogni intervallo tra due radici, in ordine."""
    atoms = []
    shape_ = atoms_of(f, x, env, atoms)

    # Il polinomio che ha per radici tutti i punti dove un confronto puo cambiare: il
    # prodotto dei polinomi dei confronti e dei polinomi minimi delle costanti con una
    # radice, (x - a)^2 - b^2 d, senza fattori ripetuti.
    product = [Fr(1)]
    for kind, _, what in atoms:
        if kind == "poly" and len(what) > 1:
            product = u_mul(product, what)
        elif kind == "surd":
            product = u_mul(product, [what.a * what.a - what.b * what.b * what.d, -2 * what.a, Fr(1)] if what.b != 0 else [-what.a, Fr(1)])
    if len(product) > 1:
        product = u_quo(product, u_gcd(product, u_trim([i * c for i, c in enumerate(product)][1:])))
    roots = isolate(product) if len(product) > 1 else []

    def sign_at_root(p, k):
        """Il segno del polinomio p, uno dei fattori del prodotto, nella radice numero k.
        Le radici di p sono radici del prodotto, e l'intervallo ne contiene una sola: p vi
        si annulla se ha una radice li dentro, e altrimenti ha in tutto l'intervallo il
        segno che ha nel suo estremo."""
        lo, hi = roots[k]
        if lo != hi:
            common = u_gcd(product, p)
            if len(common) > 1 and roots_between(sturm(common), lo, hi) > 0:
                return 0
        v = u_eval(p, lo)
        return (v > 0) - (v < 0)

    def is_root(c):
        total = Surd(0)
        for coefficient in reversed(product):
            total = total * c + coefficient
        return total.a == 0 and total.b == 0

    def compare_root(c, k):
        """Il segno di (radice numero k) - c, con c una costante di Surd. Se c sta
        nell'intervallo della radice ed e una radice del prodotto, e lei; altrimenti
        l'intervallo si dimezza, tenendo la meta con la radice, finche c ne resta fuori."""
        lo, hi = roots[k]
        if lo == hi:
            return (Surd(lo) - c).sign()
        chain, same = sturm(product), is_root(c)
        while True:
            if (Surd(lo) - c).sign() >= 0:
                return 1
            if (Surd(hi) - c).sign() <= 0:
                return -1
            if same:
                return 0
            mid = (lo + hi) / 2
            if u_eval(product, mid) == 0:
                return (Surd(mid) - c).sign()
            lo, hi = (lo, mid) if roots_between(chain, lo, mid) > 0 else (mid, hi)

    def truth(shape_, place, k):
        """La formula nel punto: la radice numero k (place = "root") o il razionale k."""
        if shape_[0] == "atom":
            kind, relation, what = atoms[shape_[1]]
            if kind == "poly":
                v = u_eval(what, k if place == "point" else 0)
                sign = sign_at_root(what, k) if place == "root" and len(what) > 1 else (v > 0) - (v < 0)
                return related(sign, relation)
            return related(compare_root(what, k) if place == "root" else (Surd(k) - what).sign(), relation)
        values = [truth(g, place, k) for g in shape_[1:]]
        if shape_[0] == "not":
            return not values[0]
        if shape_[0] == "and":
            return values[0] and values[1]
        if shape_[0] == "or":
            return values[0] or values[1]
        if shape_[0] == "iff":
            return values[0] == values[1]
        if shape_[0] == "implies":
            return (not values[0]) or values[1]
        raise Unreadable("connective %s" % shape_[0])

    # In ordine: un punto prima della prima radice, poi ogni radice e un punto dopo di lei.
    # Un punto tra due radici e la media tra la fine dell'intervallo dell'una e l'inizio di
    # quello dell'altra: tra le due non ci sono radici, e gli estremi non lo sono.
    samples = [Fr(0)] if not roots else [roots[0][0] - 1] + [(roots[k][1] + roots[k + 1][0]) / 2 for k in range(len(roots) - 1)] + [roots[-1][1] + 1]
    out = [truth(shape_, "point", samples[0])]
    for k in range(len(roots)):
        out += [truth(shape_, "root", k), truth(shape_, "point", samples[k + 1])]
    return out


PARAMETER_VALUES = [Fr(v) for v in (-3, -2, -1, 0, 1, 2, 3)] + [Fr(1, 2), Fr(-3, 2), Fr(7, 3), Fr(-5, 4), Fr(10), Fr(-9)]


def decide(tree, env=None):
    """La verita di un enunciato con una lettera, senza richieste: True, False, oppure
    solleva Skip. Con due lettere una dentro l'altra decide solo cio che e certo: un
    "per ogni" esterno e falso se lo e in un valore provato; se vale in tutti i valori
    provati non si sa."""
    env = env or {}
    kind = tree[0]
    if kind in ("forall", "exists"):
        _, x, body = tree
        if body[0] in ("forall", "exists", "solve"):
            # Un'identita tra polinomi in piu lettere si decide da sola.
            inner = body
            while inner[0] == "forall":
                inner = inner[2]
            if kind == "forall" and inner[0] == "rel" and inner[1] == "=" and not mentions(inner, "sqrt"):
                difference = Poly.of(settled(value(inner[2]))) - Poly.of(settled(value(inner[3])))
                for name, v in env.items():
                    difference = difference.at(name, v)
                return difference == 0
            results = [decide(body, dict(env, **{x: v})) for v in PARAMETER_VALUES]
            if kind == "forall" and not all(results):
                return False
            if kind == "exists" and any(results):
                return True
            raise Skip("two letters: true at every value tried, which proves nothing" if kind == "forall" else "two letters")
        values = table(body, x, env)
        return all(values) if kind == "forall" else any(values)
    if kind == "solve":
        raise Skip("a request has no truth")
    raise Unreadable("a statement with a letter wants its quantifier")


def letter_judge(text):
    """Che cosa dice l'oracolo di un enunciato con una lettera: ("true",), ("false",),
    ("request", albero) per una richiesta di soluzioni, ("skip", perche), ("undefined",
    perche), ("unreadable", perche)."""
    determinants[0] = 0
    try:
        tree = LetterReader(text).statement()
        inner = tree[2] if tree[0] == "forall" and tree[2][0] == "solve" else tree
        if inner[0] == "solve":
            # Che si legga fino in fondo: la condizione nella sua lettera, con il parametro,
            # se c'e, a zero.
            table(inner[2], inner[1], {} if inner is tree else {tree[1]: Fr(0)})
            return ("request", tree)
        return ("true",) if decide(tree) else ("false",)
    except (Unreadable, IndexError) as problem:
        return ("unreadable", str(problem))
    except Undefined as problem:
        return ("undefined", str(problem))
    except (Skip, TooBig) as problem:
        return ("skip", str(problem))


def same_solutions(request, solutions):
    """Ritorna True se le soluzioni scritte in solutions sono quelle della richiesta
    request (l'albero di una richiesta, anche sotto un "per ogni" sul parametro); False se
    non lo sono; solleva Skip se non si sa. Con un parametro si prova nei valori di
    PARAMETER_VALUES: un no e certo, un si vuol dire solo "in quei valori"."""
    envs = [{}]
    if request[0] == "forall":
        envs = [{request[1]: v} for v in PARAMETER_VALUES]
        request = request[2]
    _, x, condition = request
    text = solutions.strip()
    if text.startswith("\\forall"):
        written = None, True
    elif text.startswith("\\nexists"):
        written = None, False
    else:
        reader = LetterReader(text)
        written = reader.formula(), None
        if reader.r.peek() is not None:
            raise Unreadable("text after the solutions: %r" % reader.r.peek())
    for env in envs:
        if written[0] is None:
            values = table(condition, x, env)
            if any(v != written[1] for v in values):
                return False
        elif not all(table(("iff", condition, written[0]), x, env)):
            return False
    return True


# ------------------------------------------------- il fuzz con una lettera
def poly_tex(p, x):
    """Scrive in LaTeX il polinomio p (lista dal termine noto) nella lettera x."""
    out = ""
    for e in range(len(p) - 1, -1, -1):
        c = p[e]
        if c == 0:
            continue
        body = "" if e == 0 else x if e == 1 else "%s^{%d}" % (x, e)
        size = tex_number(abs(c)) if abs(c) != 1 or not body else ""
        out += ("-" if c < 0 else "") if not out else (" - " if c < 0 else " + ")
        out += size + (" " if size and body else "") + body
    return out or "0"


def rational_roots(p):
    """Le radici razionali di p, e cio che resta di p tolti i loro fattori: per il teorema
    delle radici razionali, con i coefficienti resi interi, sono tra le frazioni che hanno
    per numeratore un divisore del termine noto e per denominatore uno del primo."""
    roots = []
    while len(p) > 1 and p[0] == 0:
        roots.append(Fr(0))
        p = p[1:]
    scale = 1
    for c in p:
        scale = scale * c.denominator // math.gcd(scale, c.denominator)
    ints = [int(c * scale) for c in p]
    if len(p) > 1 and abs(ints[0]) <= 10 ** 6 and abs(ints[-1]) <= 10 ** 6:
        divisors = lambda n: [d for d in range(1, abs(n) + 1) if n % d == 0] if abs(n) <= 5000 else [1, abs(n)]
        for num in divisors(ints[0]):
            for den in divisors(ints[-1]):
                for candidate in (Fr(num, den), Fr(-num, den)):
                    while len(p) > 1 and u_eval(p, candidate) == 0:
                        roots.append(candidate)
                        p = u_quo(p, [-candidate, Fr(1)])
    return sorted(set(roots)), p


class LetterGenerator:
    """Enunciati con una lettera: un determinante, o una traccia, di una matrice con la
    lettera negli elementi, dentro una richiesta di soluzioni, un "per ogni", un "esiste".
    Scrive il testo; che cosa vale lo decide poi l'oracolo rileggendolo."""

    def __init__(self, rnd):
        self.rnd = rnd

    def entry(self, x):
        r, c = self.rnd, self.rnd.random()
        if c < 0.45:
            return str(r.randint(-4, 4))
        if c < 0.6:
            return x
        if c < 0.74:
            return "%s %s %d" % (x, r.choice("+-"), r.randint(1, 4))
        if c < 0.84:
            return r.choice(["-%s", "2%s", "-3%s", "\\frac{%s}{2}", "\\frac{2}{3} %s"]) % x
        if c < 0.92:
            return "%d - %s" % (r.randint(-3, 3), x)
        return r.choice(["%s^{2}", "%s^{2} - 1", "2 - %s^{2}"]) % x

    def numbers(self, n):
        r = self.rnd
        return "\\begin{pmatrix} %s \\end{pmatrix}" % " \\\\ ".join(" & ".join(str(r.randint(-4, 4)) for _ in range(n)) for _ in range(n))

    def matrix(self, x, n):
        r = self.rnd
        rows = [[self.entry(x) for _ in range(n)] for _ in range(n)]
        if n >= 3:
            # Qualche zero in piu: determinanti di grado piu basso, con radici piu semplici.
            for _ in range(r.randint(0, n)):
                rows[r.randrange(n)][r.randrange(n)] = "0"
        env = r.choice(["pmatrix", "pmatrix", "bmatrix"])
        return "\\begin{%s} %s \\end{%s}" % (env, " \\\\ ".join(" & ".join(row) for row in rows), env)

    def scalar(self, x):
        """Ritorna (definizioni, espressione): un determinante o una traccia nella lettera x."""
        r, c = self.rnd, self.rnd.random()
        n = r.choice([2, 2, 2, 3, 3, 4])
        if c < 0.4:
            return "", "\\det %s" % self.matrix(x, n)
        if c < 0.5:
            return "", "\\begin{vmatrix}%s\\end{vmatrix}" % self.matrix(x, n)[15:-13]
        if c < 0.75:
            # Gli autovalori di una matrice di numeri.
            n = r.choice([2, 2, 3, 3])
            if r.random() < 0.5:
                return "A = %s \\implies " % self.numbers(n), "\\det\\left( A - %sI \\right)" % x
            return "", "\\det\\left( %s - %s I_{%d} \\right)" % (self.numbers(n), x, n)
        if c < 0.82:
            return "", "\\operatorname{tr} %s" % self.matrix(x, r.choice([2, 3]))
        if c < 0.88:
            return "", "\\det %s^{%s}" % (self.matrix(x, 2), r.choice(["T", "2"]))
        if c < 0.94:
            return "", "\\det\\left( %s %s \\right)" % (self.matrix(x, 2), self.matrix(x, 2))
        return "", "%d \\det %s + %s" % (r.randint(2, 3), self.matrix(x, 2), x)

    def beyond(self, x):
        """Cio che con una lettera non si tratta: il programma non deve deciderlo."""
        r = self.rnd
        m = self.matrix(x, 2)
        return r.choice([
            "\\{%s \\in \\mathbb{R} \\mid \\operatorname{rank} %s = 2\\}" % (x, m),
            "\\{%s \\in \\mathbb{R} \\mid \\det %s^{-1} = 1\\}" % (x, m),
            "\\forall %s \\in \\mathbb{R}\\ \\left( %s = %s \\right)" % (x, m, m),
            "\\{%s \\in \\mathbb{R} \\mid \\det \\begin{pmatrix} \\frac{1}{%s} & 1 \\\\ 2 & %s \\end{pmatrix} = 0\\}" % (x, x, x),
            "\\{%s \\in \\mathbb{R} \\mid \\operatorname{tr} \\begin{pmatrix} I & 1 \\\\ 2 & %s \\end{pmatrix} = 0\\}" % (x, x),
            "\\det %s = 0" % m,
        ])

    def statement(self):
        r = self.rnd
        x = r.choice("khstxa")
        if r.random() < 0.06:
            return self.beyond(x)
        names, s = self.scalar(x)
        relation = r.choice(["=", "=", "=", "\\neq", "<", ">", "\\le", "\\ge"])
        c = r.choice([0, 0, 0, 0, 1, -2, 3])
        kind = r.random()
        if kind < 0.3:
            return "%s\\{%s \\in \\mathbb{R} \\mid %s %s %d\\}" % (names, x, s, relation, c)
        if kind < 0.42:
            return "%s%s %s \\in \\mathbb{R}\\ \\left( %s %s %d \\right)" % (names, r.choice(["\\forall", "\\forall", "\\exists"]), x, s, relation, c)

        # Gli enunciati veri o falsi per costruzione vogliono il polinomio: lo sviluppa
        # l'oracolo, rileggendo l'espressione appena scritta.
        try:
            reader = LetterReader(names + s + " = 0")
            reader.statement()
        except (Unreadable, IndexError):
            return None
        try:
            reader = LetterReader(names + "\\forall %s \\in \\mathbb{R}\\ \\left( %s = 0 \\right)" % (x, s))
            p = u_trim(Poly.of(settled(value(reader.statement()[2][2]))).coefficients(x))
        except (Unreadable, Undefined, IndexError, TooBig):
            return None
        if kind < 0.64:
            # Il polinomio, giusto oppure con un coefficiente cambiato.
            if r.random() < 0.45:
                p = list(p) or [Fr(0)]
                i = r.randrange(len(p))
                p[i] += r.choice([1, -1, 2])
                p = u_trim(p)
            return "%s\\forall %s \\in \\mathbb{R}\\ \\left( %s = %s \\right)" % (names, x, s, poly_tex(p, x))

        # "Si annulla se e solo se": con le radici vere, se sono tutte razionali, oppure
        # con una tolta, una spostata, una in piu.
        if not p:
            return "%s\\forall %s \\in \\mathbb{R}\\ \\left( %s = 0 \\right)" % (names, x, s)
        roots, rest = rational_roots(p)
        if len(rest) > 1 and roots_between(sturm(u_quo(rest, u_gcd(rest, u_trim([i * q for i, q in enumerate(rest)][1:])))), "-", None) > 0:
            return "%s\\{%s \\in \\mathbb{R} \\mid %s = 0\\}" % (names, x, s)
        if not roots:
            return "%s\\forall %s \\in \\mathbb{R}\\ \\left( %s %s 0 \\right)" % (names, x, s, r.choice(["\\neq", "\\neq", "=", ">"]))
        change = r.random()
        if change < 0.2 and len(roots) > 1:
            roots = roots[:-1] if r.random() < 0.5 else roots[1:]
        elif change < 0.35:
            i = r.randrange(len(roots))
            roots[i] += r.choice([1, -1, Fr(1, 2)])
        elif change < 0.45:
            roots.append(roots[-1] + r.randint(1, 3))
        written = " \\lor ".join("%s = %s" % (x, tex_number(q)) for q in roots)
        return "%s\\forall %s \\in \\mathbb{R}\\ \\left( %s = 0 \\iff \\left( %s \\right) \\right)" % (names, x, s, written)


def mutate_letters(cert, rnd):
    """Un certificato con le lettere alterato in un punto dell'enunciato: una lettera al
    posto di un numero, un numero al posto di una lettera, o un'altra lettera."""
    lines = cert.rstrip("\n").split("\n")
    declared = lines[1].split()[1:]
    tokens = re.findall(r"\(|\)|[^\s()]+", lines[2])
    spots = [i for i, t in enumerate(tokens) if i and (t in declared or re.fullmatch(r"-?\d+", t)) and tokens[i - 1] != "matrix" and tokens[i - 2] != "matrix"]
    if not spots:
        return cert
    i = rnd.choice(spots)
    tokens[i] = rnd.choice([t for t in declared + ["0", "1", "2", "-1"] if t != tokens[i]])
    lines[2] = join(tokens)
    return "\n".join(lines) + "\n"


def letters_mode(binary, seed, count, work, checker):
    """Il fuzz degli enunciati con una lettera: vedi "CON UNA LETTERA" in testa al file."""
    rnd = random.Random(seed)
    gen = LetterGenerator(rnd)
    os.makedirs(work, exist_ok=True)
    cases = []
    while len(cases) < count:
        text = gen.statement()
        if text is None or len(text) > 3500:
            continue
        if rnd.random() < 0.06:
            text = damage(text, rnd)
            if not re.search(r"\\begin\s*\{(pmatrix|bmatrix|matrix|array|vmatrix)\}", text) or len(text) > 3500:
                continue
        # Senza \mathbb l'enunciato e uno di quelli sui numeri: una lettera non vi si legge.
        cases.append((text, letter_judge(text) if "\\mathbb" in text else judge(text)))
    tex_path = os.path.join(work, "letters_%d.tex" % seed)
    cert_path = os.path.join(work, "letters_%d.cert" % seed)
    with open(tex_path, "w", encoding="utf-8") as out:
        out.write("\\begin{document}\n\\begin{enumerate}\n")
        for text, _ in cases:
            out.write("\\item $\\displaystyle %s$\n" % text)
        out.write("\\end{enumerate}\n\\end{document}\n")
    if os.path.exists(cert_path):
        os.remove(cert_path)
    run = subprocess.run([binary, "--certificates", cert_path, tex_path], capture_output=True, text=True)
    lines = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
    bugs = suspects = misses = skipped = 0
    tally = {}
    if len(lines) != len(cases):
        print("BUG: the program answered %d of %d statements (exit %d)\n%s" % (len(lines), len(cases), run.returncode, run.stderr[-2000:]))
        bugs += 1
    for number, (line, (text, expected)) in enumerate(zip(lines, cases), 1):
        answer = line.split(". ", 1)[1]
        outcome = (answer.split()[0] + (" " + answer.split()[1] if answer.startswith("NON") else "")).rstrip(":")
        decided = outcome in ("DIMOSTRATO", "FALSO", "RISOLTO")
        if decided and "certificato verificato" not in answer and "ha il suo certificato, verificato" not in answer:
            print("BUG: statement %d is decided and no certificate is mentioned: %s\n   %s" % (number, answer[:160], text))
            bugs += 1
        kind = expected[0]
        if kind == "request" and outcome == "RISOLTO":
            # Le soluzioni scritte, rilette qui e confrontate con la condizione.
            written = answer.split(": ", 1)[1] if ": " in answer else ""
            try:
                if "r_{" in written:
                    raise Skip("roots without a writing with radicals")
                try:
                    same = same_solutions(expected[1], written)
                except Unreadable:
                    same = same_solutions(expected[1], written[:written.rindex(" (")])
                if not same:
                    print("BUG: statement %d is solved wrongly: %s\n   %s" % (number, answer[:300], text))
                    bugs += 1
            except (Skip, TooBig):
                kind = "request, not checked"
                skipped += 1
            except (Unreadable, IndexError, ValueError, Undefined) as problem:
                print("SUSPECT: statement %d, cannot read the solutions (%s): %s\n   %s" % (number, problem, answer[-200:], text))
                suspects += 1
        elif kind == "request":
            if decided:
                print("BUG: statement %d is a request but the program says %s\n   %s" % (number, answer[:160], text))
                bugs += 1
            else:
                print("MISS: statement %d is not solved: %s\n   %s" % (number, answer[:160], text))
                misses += 1
        elif kind in ("true", "false"):
            right = "DIMOSTRATO" if kind == "true" else "FALSO"
            if decided and outcome != right:
                print("BUG: statement %d is %s but the program says %s\n   %s" % (number, kind, answer[:160], text))
                bugs += 1
            elif not decided:
                print("MISS: statement %d is %s and the program does not decide it: %s\n   %s" % (number, kind, answer[:160], text))
                misses += 1
        elif kind == "skip":
            skipped += 1
        elif decided:
            print("SUSPECT: statement %d is %s for the oracle (%s) but the program says %s\n   %s" % (number, kind, expected[1], answer[:160], text))
            suspects += 1
        tally[(kind, outcome)] = tally.get((kind, outcome), 0) + 1
    if "ATTENZIONE" in run.stdout:
        print("SUSPECT: the program discarded a step of its own proof: %s" % [l for l in run.stdout.splitlines() if "ATTENZIONE" in l])
        suspects += 1

    # I certificati: tutti accettati dal verificatore, e quelli delle matrici riletti
    # dall'oracolo; poi quelli degli sviluppi, alterati.
    mutated = accepted = accepted_wrong = 0
    if checker and os.path.exists(cert_path):
        certs = [c.lstrip("\n") + "end\n" for c in open(cert_path, encoding="utf-8").read().split("end\n") if c.strip()]
        run = subprocess.run([checker, cert_path], capture_output=True, text=True)
        answers = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
        if len(answers) != len(certs):
            print("BUG: the checker answered %d of %d certificates" % (len(answers), len(certs)))
            bugs += 1
        own = []
        for cert, answer in zip(certs, answers):
            good = True
            if cert.startswith("certificate matrices"):
                own.append(cert)
                try:
                    good = cert_judge(cert)
                except (Unreadable, IndexError, TooBig) as problem:
                    print("SUSPECT: the oracle cannot read a certificate of the program (%s)\n%s" % (problem, cert))
                    suspects += 1
                    continue
            if "ACCETTATO" not in answer or not good:
                print("BUG: a certificate of the program is %s by the checker and %s for the oracle\n%s" % (answer.split(". ", 1)[1][:80], good, cert))
                bugs += 1
        variants = []
        lettered = [c for c in own if c.split("\n")[1].startswith("letters")]
        while lettered and len(variants) < count:
            cert = rnd.choice(lettered)
            variant = mutate_letters(cert, rnd) if rnd.random() < 0.4 else mutate(cert, rnd, [])
            if variant not in own:
                variants.append(variant)
        mutated_path = os.path.join(work, "letters_%d_mutated.cert" % seed)
        with open(mutated_path, "w", encoding="utf-8") as out:
            out.write("\n".join(variants))
        run = subprocess.run([checker, mutated_path], capture_output=True, text=True)
        answers = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
        mutated = len(variants)
        if len(answers) != len(variants):
            print("SUSPECT: the checker answered %d of %d altered certificates (some no longer begin as one)" % (len(answers), len(variants)))
            suspects += 1
        else:
            for variant, answer in zip(variants, answers):
                if "ACCETTATO" not in answer:
                    continue
                accepted += 1
                try:
                    good = cert_judge(variant)
                except TooBig as problem:
                    print("SUSPECT: the checker accepted a certificate too large for the oracle (%s)\n%s" % (problem, variant))
                    suspects += 1
                    continue
                except (Unreadable, IndexError, RecursionError) as problem:
                    good = False
                if not good:
                    print("BUG: the checker accepted a certificate with letters that does not say the truth\n%s" % variant)
                    bugs += 1
                    accepted_wrong += 1

    print("Summary (seed %d): %d statements with a letter, %d not checked by the oracle" % (seed, len(cases), skipped))
    for key in sorted(tally):
        print("   %-22s %-16s %d" % (key[0], key[1], tally[key]))
    if checker:
        print("   certificates altered: %d, accepted by the checker: %d, of which wrong: %d" % (mutated, accepted, accepted_wrong))
    bugs += contradictions[0]
    print("BUG %d, SUSPECT %d, MISS %d" % (bugs, suspects, misses))
    sys.exit(1 if bugs else 0)


# ---------------------------------------------------------------- i due modi
def items_of(path):
    text = open(path, encoding="utf-8").read()
    return [m.group(1).strip() for m in re.finditer(r"\\item\s+\$\\displaystyle(.*?)\$\s*(?=\\item|\\end\{enumerate\})", text, re.S)]


def data_mode(paths):
    for path in paths:
        print(path)
        for number, text in enumerate(items_of(path), 1):
            verdict = letter_judge(text) if "\\mathbb" in text else judge(text)
            words = {"true": "VERO", "false": "FALSO", "undefined": "NON DEFINITO", "unreadable": "NON LETTO", "value": "VALORE",
                     "request": "RICHIESTA", "skip": "NON DECISO DALL'ORACOLO"}
            detail = tex_value(verdict[1]) if verdict[0] == "value" else "" if verdict[0] == "request" else verdict[1] if len(verdict) > 1 else ""
            print("%4d. %s%s" % (number, words[verdict[0]], ": " + detail if detail else ""))


def answers_mode(tex_path, c_path):
    """Le soluzioni attese delle richieste di un file di dati, scritte nel test in C tra la
    riga che apre expected_solutions[] e la sua graffa chiusa: l'oracolo dice se sono
    quelle della richiesta."""
    source = open(c_path, encoding="utf-8").read()
    block = re.search(r"expected_solutions\[\] = \{\n(.*?)\n\};", source, re.S)
    answers = [m.group(1).replace("\\\\", "\\") for m in re.finditer(r'^\s*"(.*)",\s*$', block.group(1), re.M)] if block else []
    requests = items_of(tex_path)
    print("%s: %d requests, %d expected solutions" % (tex_path, len(requests), len(answers)))
    wrong = 0
    for number, (text, answer) in enumerate(zip(requests, answers), 1):
        verdict = letter_judge(text)
        try:
            if verdict[0] != "request":
                outcome = "NON E UNA RICHIESTA PER L'ORACOLO (%s)" % (verdict[1] if len(verdict) > 1 else verdict[0])
            elif not same_solutions(verdict[1], answer):
                outcome = "SBAGLIATA"
            else:
                outcome = "CONFERMATA" if verdict[1][0] == "solve" else "CONFERMATA IN %d VALORI DEL PARAMETRO" % len(PARAMETER_VALUES)
        except (Unreadable, IndexError, Undefined, Skip, TooBig) as problem:
            outcome = "NON DECISA DALL'ORACOLO (%s)" % problem
        wrong += not outcome.startswith("CONFERMATA")
        print("%4d. %s: %s" % (number, outcome, answer))
    wrong += abs(len(requests) - len(answers))
    print("%d not confirmed" % wrong)
    sys.exit(1 if wrong else 0)


def main():
    if len(sys.argv) >= 3 and sys.argv[1] == "data":
        data_mode(sys.argv[2:])
        return
    if len(sys.argv) == 4 and sys.argv[1] == "answers":
        answers_mode(sys.argv[2], sys.argv[3])
        return
    if len(sys.argv) in (6, 7) and sys.argv[1] == "letters":
        letters_mode(sys.argv[2], int(sys.argv[3]), int(sys.argv[4]), sys.argv[5], sys.argv[6] if len(sys.argv) == 7 else None)
        return
    if len(sys.argv) not in (5, 6):
        print(__doc__)
        sys.exit(2)
    binary, seed, count, work = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
    checker = sys.argv[5] if len(sys.argv) == 6 else None
    rnd = random.Random(seed)
    gen = Generator(rnd)
    os.makedirs(work, exist_ok=True)

    # Gli enunciati, ognuno riletto anche dal lettore di questo script: se il lettore e
    # l'albero generato non sono d'accordo, e l'oracolo a essere incoerente.
    cases = []
    while len(cases) < count:
        case = make_case(gen, rnd)
        if case is None or len(case[0]) > 3500:
            continue
        text, expected = case
        again = judge(text)
        if again[0] != expected[0] or (expected[0] == "value" and again[1] != expected[1]):
            print("ORACLE: the reader and the generator disagree (%s against %s)\n   %s" % (again[:1], expected[:1], text))
            continue
        if rnd.random() < 0.15:
            # Rovinato: vale cio che ne dice il lettore di questo script.
            text = damage(text, rnd)
            if not re.search(r"\\begin\s*\{(pmatrix|bmatrix|matrix|array|vmatrix)\}", text):
                continue
            case = (text, judge(text))
        cases.append(case)
    tex_path = os.path.join(work, "linear_algebra_%d.tex" % seed)
    cert_path = os.path.join(work, "linear_algebra_%d.cert" % seed)
    with open(tex_path, "w", encoding="utf-8") as out:
        out.write("\\begin{document}\n\\begin{enumerate}\n")
        for text, _ in cases:
            out.write("\\item $\\displaystyle %s$\n" % text)
        out.write("\\end{enumerate}\n\\end{document}\n")
    if os.path.exists(cert_path):
        os.remove(cert_path)
    run = subprocess.run([binary, "--certificates", cert_path, tex_path], capture_output=True, text=True)
    lines = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
    bugs = suspects = misses = 0
    tally = {}
    if len(lines) != len(cases):
        print("BUG: the program answered %d of %d statements (exit %d)\n%s" % (len(lines), len(cases), run.returncode, run.stderr[-2000:]))
        bugs += 1
    for number, (line, (text, expected)) in enumerate(zip(lines, cases), 1):
        answer = line.split(". ", 1)[1]
        outcome = answer.split()[0] + (" " + answer.split()[1] if answer.startswith("NON") else "")
        outcome = outcome.rstrip(":")
        tally[(expected[0], outcome)] = tally.get((expected[0], outcome), 0) + 1
        decided = outcome in ("DIMOSTRATO", "FALSO", "RISOLTO")
        if decided and "certificato verificato" not in answer:
            print("BUG: statement %d is decided without a verified certificate: %s\n   %s" % (number, answer[:120], text))
            bugs += 1
        if expected[0] == "unreadable":
            if decided:
                print("SUSPECT: statement %d is not a statement for the reader of this script (%s) but the program says %s\n   %s" % (number, expected[1], answer[:120], text))
                suspects += 1
        elif expected[0] == "undefined":
            if decided:
                print("BUG: statement %d has no meaning but the program says %s\n   %s" % (number, answer[:120], text))
                bugs += 1
        elif expected[0] == "value":
            if outcome == "RISOLTO":
                found = judge(answer.split(": ", 1)[1]) if ": " in answer else ("unreadable",)
                if found[0] != "value":
                    print("SUSPECT: statement %d, cannot read the answer: %s\n   %s" % (number, answer[-120:], text))
                    suspects += 1
                elif found[1] != expected[1]:
                    print("BUG: statement %d is solved wrongly: %s (the value is %s)\n   %s" % (number, answer[:200], tex_value(expected[1]), text))
                    bugs += 1
            elif decided:
                print("BUG: statement %d is a request but the program says %s\n   %s" % (number, answer[:120], text))
                bugs += 1
            else:
                print("MISS: statement %d is not solved: %s\n   %s" % (number, answer[:120], text))
                misses += 1
        else:
            right = "DIMOSTRATO" if expected[0] == "true" else "FALSO"
            if decided and outcome != right:
                print("BUG: statement %d is %s but the program says %s\n   %s" % (number, expected[0], answer[:120], text))
                bugs += 1
            elif not decided:
                print("MISS: statement %d is %s and the program does not decide it: %s\n   %s" % (number, expected[0], answer[:120], text))
                misses += 1
    if "ATTENZIONE" in run.stdout:
        print("SUSPECT: the program discarded a step of its own proof: %s" % [l for l in run.stdout.splitlines() if "ATTENZIONE" in l])
        suspects += 1

    # I certificati: quelli del programma, poi quelli alterati.
    accepted_wrong = rejected_right = mutated = accepted = 0
    if checker and os.path.exists(cert_path):
        certs = [c + "end\n" for c in open(cert_path, encoding="utf-8").read().split("end\n") if c.strip()]
        certs = [c.lstrip("\n") for c in certs]
        run = subprocess.run([checker, cert_path], capture_output=True, text=True)
        answers = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
        for cert, answer in zip(certs, answers):
            try:
                good = cert_judge(cert)
            except (Unreadable, IndexError, TooBig) as problem:
                print("SUSPECT: the oracle cannot read a certificate of the program (%s)\n%s" % (problem, cert))
                suspects += 1
                continue
            if "ACCETTATO" not in answer or not good:
                print("BUG: a certificate of the program is %s by the checker and %s for the oracle\n%s" % (answer.split(". ", 1)[1][:80], good, cert))
                bugs += 1
                rejected_right += 1
        if len(answers) != len(certs):
            print("BUG: the checker answered %d of %d certificates" % (len(answers), len(certs)))
            bugs += 1
        variants = []
        pool = [line for cert in certs for line in cert.split("\n") if line.startswith("witness ")]
        while certs and len(variants) < count:
            variant = mutate(rnd.choice(certs), rnd, pool)
            if variant not in certs:
                variants.append(variant)
        mutated_path = os.path.join(work, "linear_algebra_%d_mutated.cert" % seed)
        with open(mutated_path, "w", encoding="utf-8") as out:
            out.write("\n".join(variants))
        run = subprocess.run([checker, mutated_path], capture_output=True, text=True)
        answers = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
        mutated = len(variants)
        if len(answers) != len(variants):
            print("SUSPECT: the checker answered %d of %d altered certificates (some no longer begin as one)" % (len(answers), len(variants)))
            suspects += 1
        else:
            for variant, answer in zip(variants, answers):
                if "ACCETTATO" not in answer:
                    continue
                accepted += 1
                try:
                    good = cert_judge(variant)
                except TooBig as problem:
                    print("SUSPECT: the checker accepted a certificate too large for the oracle (%s)\n%s" % (problem, variant))
                    suspects += 1
                    continue
                except (Unreadable, IndexError, RecursionError) as problem:
                    print("BUG: the checker accepted a certificate that the oracle cannot read (%s)\n%s" % (problem, variant))
                    bugs += 1
                    accepted_wrong += 1
                    continue
                if not good:
                    print("BUG: the checker accepted a certificate that does not say the truth\n%s" % variant)
                    bugs += 1
                    accepted_wrong += 1

    print("Summary (seed %d): %d statements" % (seed, len(cases)))
    for key in sorted(tally):
        print("   %-10s %-16s %d" % (key[0], key[1], tally[key]))
    if checker:
        print("   certificates altered: %d, accepted by the checker: %d, of which wrong: %d" % (mutated, accepted, accepted_wrong))
    bugs += contradictions[0]
    print("BUG %d, SUSPECT %d, MISS %d" % (bugs, suspects, misses))
    sys.exit(1 if bugs else 0)


if __name__ == "__main__":
    main()
