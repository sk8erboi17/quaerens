r"""Radici quadrate e parametri: controllo dei dati presi dal libro e fuzzing differenziale.

Uso:
    python3 -I fuzz_radicals_parameters.py check-data <exercises.jsonl>
    python3 -I fuzz_radicals_parameters.py fuzz <eseguibile> <seme> <quanti> <cartella di lavoro>

A COSA SERVE

Il programma impara due cose nuove: le radici quadrate dentro un confronto (di
costanti e di espressioni in x: equazioni e disequazioni irrazionali) e un
parametro reale accanto alla variabile. Questo script è il secondo parere su
entrambe. Non chiama mai il programma per sapere che cosa è vero.

  check-data  controlla i DATI, senza il programma. Ogni riga del file è un
      esercizio del libro: la condizione, il risultato stampato, l'enunciato
      "per ogni x: condizione <=> risultato" e alcune varianti false con un
      testimone. Lo script calcola condizione e risultato in molti punti e
      segnala ogni punto in cui sono diversi; poi verifica che ogni variante
      falsa sia davvero diversa nel suo testimone.

  fuzz  genera enunciati di cui la verità è nota PER COSTRUZIONE, li dà al
      programma e confronta la risposta.

L'ORACOLO

Un lettore suo del LaTeX (la grammatica è quella scritta nei commenti dei
capitoli 9 e 17 di parser.c, ridotta a ciò che serve qui) e un calcolatore
che sostituisce i numeri nell'enunciato com'è scritto, senza portarlo in
un'altra forma. I numeri sono esatti finché si può:

  - i razionali sono frazioni;
  - la radice di un razionale è q sqrt(n) con n senza fattori quadrati, e le
    somme q_1 sqrt(n_1) + ... + q_k sqrt(n_k) si sommano, moltiplicano e
    dividono restando di questa forma. Radici di interi senza quadrati
    distinti sono linearmente indipendenti sui razionali: una somma così è
    zero solo se lo sono tutti i coefficienti, quindi uguaglianza e segno
    sono decisi senza errore (vedi la classe Surd);
  - dove una scrittura esatta manca (la radice di un numero che è già una
    somma di radici, o di un razionale troppo grande per scomporlo in
    fattori) il numero diventa un intervallo con estremi `decimal` a 80
    cifre, arrotondati verso l'esterno: l'intervallo contiene sempre il
    numero vero. Se l'intervallo di una differenza non contiene lo zero il
    segno è certo; se lo contiene (i due membri coincidono per almeno 40
    cifre) il confronto è trattato come un'uguaglianza, ed è un pareggio:
    check-data segnala il punto come SUSPECT, a meno che lì condizione e
    risultato siano entrambi veri, cioè che il punto sia una soluzione che
    il risultato dichiara; fuzz non genera un caso la cui verità dipende da
    un pareggio, e segnala SUSPECT se dipende da un pareggio il confronto
    con le soluzioni scritte dal programma.

Un confronto è falso dove uno dei suoi membri non ha valore (divisione per
zero, radice di un numero negativo): è la convenzione dei libri.

"Per ogni x" e "esiste x" dentro una condizione (esercizi che chiedono i
valori del parametro) sono decisi solo per un confronto tra polinomi di grado
al più 2 in x, completando il quadrato: vedi signs_taken().

DOVE GUARDA check-data

  - una griglia di razionali: i dodicesimi tra -40 e 40, poi gli interi fino
    a 200 e le potenze di 10 fino a un milione; con un parametro, per lui i
    dodicesimi tra -6 e 6 e qualche valore lontano, per la variabile i mezzi
    tra -12 e 12;
  - i punti speciali: gli estremi scritti nel risultato, gli zeri dei
    radicandi e dei divisori, le radici dei confronti che sono polinomi di
    grado al più 2 (con la formula risolutiva), i valori del parametro scritti
    nei casi; e accanto a ognuno i punti a distanza 1/1000 e 1/10^7, uno tra
    due consecutivi, uno oltre ciascun estremo;
  - per un'equazione irrazionale, anche i cambi di segno: se la differenza dei
    due membri è continua tra due punti consecutivi e vi cambia segno, lì in
    mezzo c'è una soluzione (teorema dei valori intermedi), e il risultato
    deve dichiararla.

Che cosa conferma, e che cosa no. Con un parametro, per ogni valore provato
del parametro il controllo in x è completo quando i confronti sono polinomi
di grado al più 2: la verità può cambiare solo nelle radici, che sono tra i
punti provati. Sul parametro resta un campione. Per le equazioni irrazionali
conferma che ogni soluzione stampata lo è (conto esatto), che nessun altro
punto provato lo è, e che i due membri non si incrociano altrove; non esclude
una soluzione non stampata in cui i due membri si toccano senza incrociarsi
tra due punti della griglia.

PERCHÉ OGNI GENERATORE DI fuzz CONOSCE LA VERITÀ

In tutti e tre il metodo è lo stesso: il generatore sceglie lui i punti in
cui la verità della condizione può cambiare, quindi li conosce tutti; in
ognuno, e in un razionale di ogni intervallo tra due consecutivi, la
condizione è calcolata con il conto esatto. Ciò che cambia è il motivo per
cui quei punti sono tutti.

  (i-a) sqrt(a x + b) ? c x + d. Scelti c, d e due razionali r_1, r_2, il
      generatore pone a x + b = (c x + d)^2 - c^2 (x - r_1)(x - r_2), che è
      di primo grado. Dove la radice esiste, g(x) = sqrt(a x + b) - (c x + d)
      è continua, e g = 0 implica a x + b = (c x + d)^2, cioè x = r_1 o
      x = r_2. Quindi g non cambia segno tra due punti consecutivi di
      {r_1, r_2, -b/a}, e fuori dal campo di esistenza ogni confronto è
      falso. r_i è una radice estranea quando c r_i + d < 0.
  (i-b) sqrt(u_1) +- sqrt(u_2) ? k, con u_i = a_i x + b_i e a_1 != a_2.
      Scelti x_0 e due interi m, n, il generatore pone u_1(x_0) = m^2,
      u_2(x_0) = n^2, k = m +- n. Da sqrt(u_1) +- sqrt(u_2) = k segue,
      isolando e elevando al quadrato due volte,
      (k^2 + u_1 - u_2)^2 = 4 k^2 u_1: un'equazione di secondo grado a
      coefficienti razionali di cui x_0 è radice, quindi l'altra radice x_1
      è razionale e si ricava dalla somma delle radici. Ogni soluzione è x_0
      oppure x_1; i punti sono {x_0, x_1, -b_1/a_1, -b_2/a_2}.
  (i-c) l sqrt(u_1) ? sqrt(u_2), anche scritta sqrt(u_2)/sqrt(u_1) ? l.
      L'uguaglianza implica l^2 u_1 = u_2, di primo grado, con la sola radice
      x_0 scelta dal generatore (l^2 a_1 != a_2); i punti sono
      {x_0, -b_1/a_1, -b_2/a_2}.
  (ii) costanti con radicali. La condizione è un prodotto o un quoziente di
      fattori di cui il generatore ha scelto le radici: razionali,
      (p +- k sqrt(d))/q per il fattore q^2 x^2 - 2 p q x + p^2 - k^2 d,
      p + k sqrt(d) per il fattore x - p - k sqrt(d). Un prodotto cambia
      segno solo dove un fattore si annulla.
  (iii) (x - r_1)(x - r_2) ? 0 con r_i = m_i a + n_i. Il prodotto è negativo
      esattamente tra le due radici; quale delle due è la minore lo decide il
      segno di r_1 - r_2 = (m_1 - m_2)(a - a*), con a* = (n_2 - n_1)/(m_1 -
      m_2): da qui l'elenco dei casi a < a*, a = a*, a > a*.

Gli enunciati veri sono "per ogni x: condizione <=> soluzioni" con le
soluzioni così trovate. Quelli falsi nascono da un piccolo errore (un estremo
spostato, < al posto di <=, un pezzo o un caso dimenticato, una radice
estranea aggiunta, l'ordine delle radici non scambiato): un enunciato è
dichiarato falso solo se il conto esatto trova un punto in cui i due lati
sono diversi. Le richieste di soluzioni, \{x \in \mathbb{R} \mid c\} e, con
un parametro, \forall a \in \mathbb{R}\ \{x \in \mathbb{R} \mid c\}, non
affermano nulla: la risposta del programma viene letta come una condizione e
confrontata con c in tutti i punti speciali, compresi gli estremi che il
programma ha scritto, e tra di essi.

LE RIGHE STAMPATE

  BUG      fuzz: il programma ha dimostrato un falso, smentito un vero o
           scritto soluzioni sbagliate. check-data: il dato è rotto (non si
           legge, è incoerente, una variante falsa non è diversa nel suo
           testimone).
  DIFFER   check-data: in quel punto condizione e risultato stampato sono
           diversi. Sbaglia il libro, o la trascrizione.
  SUSPECT  un confronto troppo vicino per essere deciso, o soluzioni scritte
           dal programma che questo lettore non sa leggere: da guardare a mano.
  NOTE     fuzz: il programma non ha deciso. check-data: un esercizio che non
           si è potuto scrivere, o un controllo che non si è potuto fare.
"""
import json
import math
import os
import random
import re
import subprocess
import sys
from decimal import ROUND_CEILING, ROUND_FLOOR, Context, Decimal
from fractions import Fraction
from functools import cmp_to_key, lru_cache


class Undefined(Exception):
    """Un'espressione non ha valore: una divisione per zero, la radice di un numero negativo."""


class Unreadable(Exception):
    """Il testo usa una notazione che questo lettore non conosce, o chiede un conto che l'oracolo non sa fare."""


class NotPolynomial(Exception):
    """L'espressione non è un polinomio nella variabile richiesta."""


# ---------------------------------------------------------------- numeri
# Un numero reale ha tre scritture, dalla più precisa alla meno: una Fraction, un Surd (somma
# esatta di radici quadrate), un Box (un intervallo che lo contiene). Le operazioni scelgono la
# più precisa che possono, e un Box non torna mai esatto.
DIGITS = 80                                             # cifre degli estremi di un Box
DOWN = Context(prec=DIGITS, rounding=ROUND_FLOOR)       # i conti arrotondati verso il basso
UP = Context(prec=DIGITS, rounding=ROUND_CEILING)       # e verso l'alto
TIE = Decimal(10) ** -40                                # un Box a cavallo dello zero più stretto di così è un pareggio
TRIAL = 100000                                          # fin dove square_part() prova i divisori

# I Box a cavallo dello zero di cui sign() ha dovuto dire il segno, dall'ultimo azzeramento. Chi
# calcola una formula in un punto la svuota prima e la guarda dopo: vedi judge().
ties = []


class Surd:
    """Il numero q_1 sqrt(n_1) + ... + q_k sqrt(n_k). terms porta ogni n_i (intero positivo senza
    fattori quadrati; 1 è la parte razionale) nel suo q_i (Fraction non nulla). Almeno un n_i è
    maggiore di 1: un razionale resta una Fraction, e lo garantisce surd().

    Perché i conti sono esatti. Le radici quadrate di interi positivi senza fattori quadrati e
    distinti sono linearmente indipendenti sui razionali (Besicovitch, 1940): la somma è zero solo
    se tutti i q_i sono zero. Quindi un Surd non è mai zero, due Surd sono uguali solo se hanno gli
    stessi termini, e il segno si trova stringendo un intervallo finché non esclude lo zero: prima
    o poi lo esclude."""

    def __init__(self, terms):
        self.terms, self.known = terms, None

    def bounds(self, digits):
        """Due razionali lo <= self <= hi, con ogni radice presa a digits cifre dopo la virgola."""
        scale, lo, hi = 10 ** digits, Fraction(0), Fraction(0)
        for n, q in self.terms.items():
            # r <= scale sqrt(n) < r + 1, e la disuguaglianza di sinistra è un'uguaglianza solo per n = 1.
            r = math.isqrt(n * scale * scale)
            a, b = q * r / scale, q * (r + (n > 1)) / scale
            lo, hi = lo + min(a, b), hi + max(a, b)
        return lo, hi

    def sign(self):
        """Il segno, esatto: +1 o -1."""
        digits = 20
        while self.known is None:
            lo, hi = self.bounds(digits)
            self.known = 1 if lo > 0 else -1 if hi < 0 else None
            digits *= 2
        return self.known

    def __add__(self, other):
        if isinstance(other, Box):
            return box(self) + other
        terms = dict(self.terms)
        for n, q in parts(other).items():
            terms[n] = terms.get(n, 0) + q
        return surd(terms)

    __radd__ = __add__

    def __neg__(self):
        return Surd({n: -q for n, q in self.terms.items()})

    def __sub__(self, other):
        return self + (-other)

    def __rsub__(self, other):
        return (-self) + other

    def __mul__(self, other):
        if isinstance(other, Box):
            return box(self) * other
        terms = {}
        for n, q in self.terms.items():
            for m, r in parts(other).items():
                # sqrt(n) sqrt(m) = g sqrt(n/g m/g) con g il massimo comun divisore: n/g e m/g sono
                # primi tra loro e senza quadrati, quindi lo è anche il loro prodotto.
                g = math.gcd(n, m)
                k = (n // g) * (m // g)
                terms[k] = terms.get(k, 0) + q * r * g
        return surd(terms)

    __rmul__ = __mul__

    def __truediv__(self, other):
        return box(self) / other if isinstance(other, Box) else self * inverse(other)

    def __rtruediv__(self, other):
        return inverse(self) * other


def surd(terms):
    """Il numero con questi termini, nella scrittura più semplice: una Fraction se non ha radici."""
    terms = {n: q for n, q in terms.items() if q}
    return Surd(terms) if any(n > 1 for n in terms) else terms.get(1, Fraction(0))


def parts(v):
    """I termini di un razionale o di un Surd, nella forma di Surd.terms."""
    return v.terms if isinstance(v, Surd) else {1: Fraction(v)} if v else {}


def least_factor(n):
    """Il più piccolo divisore primo dell'intero n > 1."""
    f = 2
    while f * f <= n:
        if n % f == 0:
            return f
        f += 1
    return n


def inverse(v):
    """1/v, esatto, per un razionale o un Surd (che non è mai zero; per un razionale controlla chi chiama).

    Si razionalizza un primo alla volta. Preso un primo p che divide qualche n_i, v = A + B sqrt(p)
    con A e B senza sqrt(p): in A i termini con p che non divide n_i, in B quelli con p che lo
    divide, scritti con n_i / p. Allora 1/v = (A - B sqrt(p)) / (A^2 - p B^2), e il divisore non
    contiene più p: dopo tanti passi quanti sono i primi è un razionale. Non è zero, perché è il
    prodotto di v per il suo coniugato, che è zero solo se lo è v."""
    if not isinstance(v, Surd):
        return 1 / Fraction(v)
    p = least_factor(max(v.terms))
    plain = surd({n: q for n, q in v.terms.items() if n % p})
    twisted = surd({n // p: q for n, q in v.terms.items() if n % p == 0})
    return (plain - twisted * Surd({p: Fraction(1)})) * inverse(plain * plain - p * twisted * twisted)


class Box:
    """Un numero reale noto per approssimazione: sta tra lo e hi (Decimal), estremi compresi.

    Ogni operazione arrotonda lo verso il basso e hi verso l'alto, quindi l'intervallo del
    risultato contiene il risultato vero. Dei Decimal si usano solo le operazioni che prendono il
    contesto come argomento: il meno unario e abs() arrotonderebbero con quello predefinito."""

    def __init__(self, lo, hi):
        self.lo, self.hi = lo, hi

    def __add__(self, other):
        other = box(other)
        return Box(DOWN.add(self.lo, other.lo), UP.add(self.hi, other.hi))

    __radd__ = __add__

    def __neg__(self):
        return Box(self.hi.copy_negate(), self.lo.copy_negate())

    def __sub__(self, other):
        return self + (-box(other))

    def __rsub__(self, other):
        return (-self) + other

    def __mul__(self, other):
        other = box(other)
        pairs = [(a, b) for a in (self.lo, self.hi) for b in (other.lo, other.hi)]
        return Box(min(DOWN.multiply(a, b) for a, b in pairs), max(UP.multiply(a, b) for a, b in pairs))

    __rmul__ = __mul__

    def __truediv__(self, other):
        # Chi chiama ha già escluso che il divisore sia zero: lo e hi hanno lo stesso segno, e
        # 1/x è decrescente su ciascuna delle due semirette.
        other = box(other)
        return self * Box(DOWN.divide(1, other.hi), UP.divide(1, other.lo))

    def __rtruediv__(self, other):
        return box(other) / self

    def magnitude(self):
        """Il valore assoluto, senza chiedere il segno: un intervallo a cavallo dello zero resta lecito."""
        if self.lo >= 0:
            return self
        if self.hi <= 0:
            return -self
        return Box(Decimal(0), max(self.lo.copy_negate(), self.hi))


def box(v):
    """Un Box che contiene v."""
    if isinstance(v, Box):
        return v
    lo, hi = v.bounds(DIGITS) if isinstance(v, Surd) else (Fraction(v), Fraction(v))
    return Box(DOWN.divide(lo.numerator, lo.denominator), UP.divide(hi.numerator, hi.denominator))


def sign(v):
    """Il segno di v: -1, 0 o +1. Per un Box che contiene lo zero risponde 0 e lo annota in ties:
    è un pareggio, cioè v è zero oppure più piccolo di TIE. Se il Box è più largo di TIE il conto
    ha perso troppe cifre per dire anche solo questo, e solleva Unreadable."""
    if isinstance(v, Surd):
        return v.sign()
    if isinstance(v, Box):
        if v.lo > 0 or v.hi < 0:
            return 1 if v.lo > 0 else -1
        if UP.subtract(v.hi, v.lo) >= TIE:
            raise Unreadable("%d digits are not enough to compare two numbers" % DIGITS)
        ties.append(v)
        return 0
    return (v > 0) - (v < 0)


@lru_cache(maxsize=None)
def square_part(m):
    """Scrive l'intero positivo m come s^2 n con n senza fattori quadrati e ritorna (s, n); None se
    non ne è certo.

    Toglie i fattori fino a TRIAL. Ciò che resta non ha divisori fino a f: se è più piccolo di
    f^2 è 1 oppure un primo, e il conto è finito. Se è un quadrato perfetto va tutto in s.
    Altrimenti potrebbe nascondere il quadrato di un primo grande, e non si può dire."""
    s, n, f = 1, 1, 2
    while f * f <= m and f <= TRIAL:
        while m % (f * f) == 0:
            m //= f * f
            s *= f
        if m % f == 0:
            m //= f
            n *= f
        f += 1
    if f * f > m:
        return s, n * m
    r = math.isqrt(m)
    return (s * r, n) if r * r == m else None


def root(v):
    """La radice quadrata di v; solleva Undefined se v è negativo. È esatta per un razionale di cui
    square_part() sa scrivere la radice; altrimenti è un Box."""
    s = sign(v)
    if s < 0:
        raise Undefined
    if s == 0:
        return Fraction(0)
    if isinstance(v, Fraction):
        # sqrt(p/d) = sqrt(p d) / d
        split = square_part(v.numerator * v.denominator)
        if split:
            return surd({split[1]: Fraction(split[0], v.denominator)})

    # La radice di decimal è arrotondata al numero rappresentabile più vicino: quello prima e
    # quello dopo racchiudono la radice vera.
    b = box(v)
    low = DOWN.sqrt(max(b.lo, Decimal(0))).next_minus(DOWN)
    return Box(max(low, Decimal(0)), UP.sqrt(b.hi).next_plus(UP))


def near(v):
    """Un razionale vicinissimo a v: serve a proporre punti, mai a decidere."""
    if isinstance(v, Surd):
        lo, hi = v.bounds(40)
        return (lo + hi) / 2
    return Fraction(v.lo) if isinstance(v, Box) else v


def between(lo, hi):
    """Un razionale strettamente tra lo e hi, con il denominatore più piccolo che trova tra le
    potenze di 2; None se i due numeri sono troppo vicini."""
    middle, den = (near(lo) + near(hi)) / 2, 1
    while den < 2 ** 90:
        candidate = Fraction(round(middle * den), den)
        if sign(candidate - lo) > 0 and sign(hi - candidate) > 0:
            return candidate
        den *= 2
    return None


def order(points):
    """I numeri di points, distinti e in ordine crescente (confronti esatti)."""
    distinct = []
    for p in sorted(points, key=cmp_to_key(lambda u, v: sign(u - v))):
        if not distinct or sign(p - distinct[-1]) != 0:
            distinct.append(p)
        elif isinstance(distinct[-1], Box):
            distinct[-1] = p   # dello stesso punto si tiene la scrittura esatta, se una c'è
    return distinct


def samples(points):
    """I punti in cui provare, in ordine crescente, quando la verità può cambiare solo in points:
    ognuno di essi, un razionale tra due consecutivi e uno oltre ciascun estremo."""
    marks = order(points)
    if not marks:
        return [Fraction(0)]
    out = [Fraction(math.floor(near(marks[0]))) - 1]
    for i, mark in enumerate(marks):
        out.append(mark)
        middle = between(mark, marks[i + 1]) if i + 1 < len(marks) else Fraction(math.ceil(near(mark))) + 1
        if middle is not None:
            out.append(middle)
    return out


# ---------------------------------------------------------------- lettura del LaTeX
# Un'espressione è una tupla: ("num", Fraction), ("var", lettera), ("neg", a), ("+", a, b),
# ("-", a, b), ("*", a, b), ("/", a, b), ("^", a, intero), ("abs", a), ("sqrt", a).
# Una condizione è ("cmp", relazione, a, b), ("and", p, q), ("or", p, q), ("not", p),
# ("iff", p, q), ("forall", lettera, p), ("exists", lettera, p), ("true",), ("false",). In cima a
# un enunciato può esserci ("solve", lettera, p), la richiesta di soluzioni, anche sotto un
# ("forall", parametro, ...).
#
# Della grammatica di parser.c manca ciò che qui non compare mai: \implies, i sistemi, gli altri
# insiemi numerici, le funzioni. Chi li incontra solleva Unreadable, e non tira a indovinare.
RELATION = {"=": "=", "<": "<", ">": ">", "neq": "\\neq", "ne": "\\neq", "le": "\\le", "leq": "\\le", "ge": "\\ge", "geq": "\\ge"}


def tokens(text):
    """Divide il testo in token (tipo, testo). Gli spazi di LaTeX non ne producono; \\left e
    \\right si fondono con il delimitatore che segue."""
    out, i = [], 0
    while i < len(text):
        c = text[i]
        if c.isspace():
            i += 1
        elif c.isdecimal():
            j = i
            while j < len(text) and text[j].isdecimal():
                j += 1
            out.append(("num", text[i:j]))
            i = j
        elif c.isalpha():
            out.append(("letter", c))
            i += 1
        elif c == "\\":
            j = i + 1
            while j < len(text) and text[j].isalpha():
                j += 1
            name = text[i + 1:j]
            if not name:
                # \{ e \} sono le graffe di un insieme; \, \; \! \: e "\ " sono spazi.
                if text[j:j + 1] in ("{", "}"):
                    out.append(("brace", text[j]))
                elif text[j:j + 1] not in (" ", ",", ";", "!", ":"):
                    raise Unreadable("unknown command")
                i = j + 1
            elif name in ("left", "right"):
                while j < len(text) and text[j].isspace():
                    j += 1
                kind = {"(": "open", "[": "open", ")": "close", "]": "close", "|": "abs" + name}.get(text[j:j + 1])
                if kind is None or kind == ("close" if name == "left" else "open"):
                    raise Unreadable("unknown delimiter after \\%s" % name)
                out.append((kind, text[j]))
                i = j + 1
            else:
                out.append(("command", name))
                i = j
        else:
            out.append(("open" if c == "(" else "close" if c == ")" else "char", c))
            i += 1
    out.append(("end", ""))
    return out


class Reader:
    """Lettore a discesa ricorsiva: una funzione per regola della grammatica, scritta nel suo commento."""

    def __init__(self, text):
        self.tokens, self.i = tokens(text), 0

    def at(self, kind, *texts):
        token = self.tokens[self.i]
        return token[0] == kind and (not texts or token[1] in texts)

    def take(self):
        self.i += 1
        return self.tokens[self.i - 1][1]

    def expect(self, kind, *texts):
        if not self.at(kind, *texts):
            raise Unreadable("expected %s, found %s" % (" ".join(texts) or kind, self.tokens[self.i][1] or "the end"))
        return self.take()

    def group(self):
        """group := '{' expr '}' | un solo token. Un numero di più cifre senza graffe per TeX è una
        cifra sola: qui non serve, e non lo si legge."""
        if self.at("char", "{"):
            self.take()
            inner = self.expr()
            self.expect("char", "}")
            return inner
        if self.at("num") and len(self.tokens[self.i][1]) > 1:
            raise Unreadable("a number of several digits without braces")
        return self.primary()

    def primary(self):
        """primary := numero | lettera | '(' expr ')' | \\left| expr \\right| | \\frac group group | \\sqrt group"""
        if self.at("num"):
            return ("num", Fraction(int(self.take())))
        if self.at("letter"):
            name = self.take()
            if self.at("char", "_"):
                raise Unreadable("a letter with a subscript")
            return ("var", name)
        if self.at("open"):
            self.take()
            inner = self.expr()
            self.expect("close")
            return inner
        if self.at("absleft"):
            self.take()
            inner = self.expr()
            self.expect("absright")
            return ("abs", inner)
        if self.at("command", "frac", "dfrac", "tfrac"):
            self.take()
            above = self.group()
            return ("/", above, self.group())
        if self.at("command", "sqrt"):
            self.take()
            if self.at("char", "["):
                raise Unreadable("a root with an index")
            return ("sqrt", self.group())
        raise Unreadable("an expression cannot start with %s" % (self.tokens[self.i][1] or "the end"))

    def factor(self):
        """factor := primary { '^' group }, con l'esponente intero non negativo scritto in cifre."""
        node = self.primary()
        while self.at("char", "^"):
            self.take()
            exponent = self.group()
            if exponent[0] != "num":
                raise Unreadable("an exponent that is not a number")
            node = ("^", node, int(exponent[1]))
        if self.at("char", "!"):
            raise Unreadable("a factorial")
        return node

    def term(self):
        """term := factor { [\\cdot | '/'] factor }: due fattori accostati si moltiplicano."""
        node = self.factor()
        while True:
            if self.at("command", "cdot", "times"):
                self.take()
                node = ("*", node, self.factor())
            elif self.at("char", "/"):
                self.take()
                node = ("/", node, self.factor())
            elif self.at("num") or self.at("letter") or self.at("open") or self.at("absleft") or self.at("command", "frac", "dfrac", "tfrac", "sqrt"):
                node = ("*", node, self.factor())
            else:
                return node

    def expr(self):
        """expr := ['-' | '+'] term { ('+' | '-') term }: il meno davanti prende solo il primo termine."""
        if self.at("char", "-"):
            self.take()
            node = ("neg", self.term())
        else:
            if self.at("char", "+"):
                self.take()
            node = self.term()
        while self.at("char", "+", "-"):
            node = (self.take(), node, self.term())
        return node

    def bound(self):
        """lettera \\in \\mathbb{R}: ritorna la lettera. Gli altri insiemi qui non servono."""
        name = self.expect("letter")
        self.expect("command", "in")
        self.expect("command", "mathbb")
        self.expect("char", "{")
        self.expect("letter", "R")
        self.expect("char", "}")
        if self.at("char", "^", "_") or self.at("command", "setminus"):
            raise Unreadable("a set of numbers other than \\mathbb{R}")
        return name

    def unary(self):
        """unary := \\lnot unary | (\\forall | \\exists) lettera \\in \\mathbb{R} unary
                  | expr relation expr { relation expr } | '(' formula ')'

        In più, le due scritture con cui si dice che le soluzioni sono tutti i numeri o nessuno:
        \\forall x \\in \\mathbb{R} e \\nexists x \\in \\mathbb{R} senza niente dopo."""
        if self.at("command", "lnot", "neg"):
            self.take()
            return ("not", self.unary())
        if self.at("command", "forall", "exists", "nexists"):
            word = self.take()
            name = self.bound()
            if self.at("end") or self.at("close") or self.at("command", "lor", "land"):
                if word == "exists":
                    raise Unreadable("a quantifier without a condition")
                return ("true",) if word == "forall" else ("false",)
            if word == "nexists":
                raise Unreadable("\\nexists followed by a condition")
            return (word, name, self.unary())

        # Un confronto può cominciare con una parentesi, come (x + 1)^{2} < 4: prima si prova a
        # leggere una catena di confronti; se non riesce si torna indietro, e la parentesi deve
        # aprire una formula.
        start = self.i
        try:
            left, chain = self.expr(), None
            while (self.at("char", "=", "<", ">") or self.at("command", "neq", "ne", "le", "leq", "ge", "geq")):
                relation = RELATION[self.take()]
                right = self.expr()
                link = ("cmp", relation, left, right)
                chain = link if chain is None else ("and", chain, link)   # a < b \le c dice a < b e b \le c
                left = right
            if chain:
                return chain
            problem = Unreadable("after an expression a relation is needed")
        except Unreadable as error:
            problem = error
        self.i = start
        if not self.at("open"):
            raise problem
        self.take()
        inner = self.formula()
        self.expect("close")
        return inner

    def conjunction(self):
        """conjunction := unary { \\land unary }"""
        node = self.unary()
        while self.at("command", "land", "wedge"):
            self.take()
            node = ("and", node, self.unary())
        return node

    def disjunction(self):
        """disjunction := conjunction { \\lor conjunction }"""
        node = self.conjunction()
        while self.at("command", "lor", "vee"):
            self.take()
            node = ("or", node, self.conjunction())
        return node

    def formula(self):
        """formula := disjunction { \\iff disjunction }"""
        node = self.disjunction()
        while self.at("command", "iff", "Leftrightarrow", "Longleftrightarrow"):
            self.take()
            node = ("iff", node, self.disjunction())
        return node

    def solve(self):
        """solve := \\{ lettera \\in \\mathbb{R} (\\mid | ':') formula \\}"""
        self.expect("brace", "{")
        name = self.bound()
        if not (self.at("command", "mid") or self.at("char", ":")):
            raise Unreadable("\\mid is missing")
        self.take()
        inner = self.formula()
        self.expect("brace", "}")
        return ("solve", name, inner)

    def statement(self):
        """statement := formula | solve | \\forall lettera \\in \\mathbb{R} solve

        La terza forma è la richiesta di soluzioni con un parametro: per ogni valore del
        parametro, risolvi nella variabile."""
        start = self.i
        if self.at("command", "forall"):
            self.take()
            name = self.bound()
            if self.at("brace", "{"):
                return ("forall", name, self.solve())
            self.i = start
        return self.solve() if self.at("brace", "{") else self.formula()


def read(text, rule="statement"):
    """Legge tutto text con la regola rule della grammatica e ne ritorna l'albero."""
    reader = Reader(text)
    node = getattr(reader, rule)()
    if not reader.at("end"):
        raise Unreadable("unexpected %s" % reader.tokens[reader.i][1])
    return node


# ---------------------------------------------------------------- calcolo
RELATIONS = {"<": lambda s: s < 0, "\\le": lambda s: s <= 0, ">": lambda s: s > 0, "\\ge": lambda s: s >= 0,
             "=": lambda s: s == 0, "\\neq": lambda s: s != 0}


def value(e, env):
    """Il valore dell'espressione e quando le lettere valgono env. Solleva Undefined dove e non ha
    valore, KeyError per una lettera che env non ha."""
    op = e[0]
    if op == "num":
        return e[1]
    if op == "var":
        return env[e[1]]
    if op == "neg":
        return -value(e[1], env)
    if op == "abs":
        v = value(e[1], env)
        return v.magnitude() if isinstance(v, Box) else -v if sign(v) < 0 else v
    if op == "sqrt":
        return root(value(e[1], env))
    if op == "^":
        v, out = value(e[1], env), Fraction(1)
        for _ in range(e[2]):
            out = out * v
        return out
    a, b = value(e[1], env), value(e[2], env)
    if op == "+":
        return a + b
    if op == "-":
        return a - b
    if op == "*":
        return a * b
    if sign(b) == 0:
        raise Undefined
    return a / b


def mentions(e, name):
    """Dice se nell'espressione e compare la lettera name."""
    return e == ("var", name) or any(isinstance(part, tuple) and mentions(part, name) for part in e[1:])


def polynomial(e, name, env):
    """I coefficienti [c_0, c_1, ...] di e vista come polinomio nella lettera name; le altre lettere
    valgono env. Solleva NotPolynomial se e non lo è (name sotto una radice, un valore assoluto o
    in un divisore), Undefined o KeyError se una parte senza name non si può calcolare."""
    if not mentions(e, name):
        return [value(e, env)]
    op = e[0]
    if op == "var":
        return [Fraction(0), Fraction(1)]
    if op == "neg":
        return [-c for c in polynomial(e[1], name, env)]
    if op == "^":
        base, out = polynomial(e[1], name, env), [Fraction(1)]
        for _ in range(e[2]):
            out = [sum((out[i] * base[k - i] for i in range(len(out)) if 0 <= k - i < len(base)), Fraction(0))
                   for k in range(len(out) + len(base) - 1)]
        return out
    if op in ("+", "-", "*"):
        a, b = polynomial(e[1], name, env), polynomial(e[2], name, env)
        if op == "*":
            return [sum((a[i] * b[k - i] for i in range(len(a)) if 0 <= k - i < len(b)), Fraction(0))
                    for k in range(len(a) + len(b) - 1)]
        size = max(len(a), len(b))
        a, b = a + [Fraction(0)] * (size - len(a)), b + [Fraction(0)] * (size - len(b))
        return [x + y if op == "+" else x - y for x, y in zip(a, b)]
    if op == "/" and not mentions(e[2], name):
        divisor = value(e[2], env)
        if sign(divisor) == 0:
            raise Undefined
        return [c / divisor for c in polynomial(e[1], name, env)]
    raise NotPolynomial


def trimmed(p):
    """Il polinomio p senza i coefficienti nulli di grado più alto: l'ultimo che resta non è zero."""
    p = list(p)
    while p and sign(p[-1]) == 0:
        p.pop()
    return p


def roots(p):
    """Le radici reali del polinomio p, se il grado è al più 2; None se è più alto. Il polinomio
    nullo e le costanti non ne hanno: il loro segno non cambia."""
    p = trimmed(p)
    if len(p) <= 1:
        return []
    if len(p) == 2:
        return [-p[0] / p[1]]
    if len(p) > 3:
        return None
    c, b, a = p
    discriminant = b * b - 4 * a * c
    if sign(discriminant) < 0:
        return []
    r = root(discriminant)
    return [(-b - r) / (2 * a), (-b + r) / (2 * a)]


def signs_taken(p):
    """L'insieme dei segni (-1, 0, +1) che il polinomio p, di grado al più 2, assume sui reali.

    Grado 0: il segno della costante. Grado 1: tutti e tre. Grado 2: completando il quadrato,
    a x^2 + b x + c = a (x + b/(2a))^2 + m con m = c - b^2/(4a); per a > 0 i valori assunti sono
    tutti e soli quelli da m in su (da m in giù per a < 0). Quindi il segno di a c'è sempre, lo
    zero c'è quando m è zero o ha il segno opposto ad a, e il segno opposto solo in questo
    secondo caso."""
    p = trimmed(p)
    if len(p) <= 1:
        return {sign(p[0]) if p else 0}
    if len(p) == 2:
        return {-1, 0, 1}
    if len(p) > 3:
        raise Unreadable("a quantified comparison of degree above 2")
    c, b, a = p
    lead, turn = sign(a), sign(c - b * b / (4 * a))
    return {lead} | ({0} if turn != lead else set()) | ({-lead} if turn == -lead else set())


def holds(f, env):
    """La verità della condizione f quando le lettere valgono env. Un confronto è falso dove uno
    dei suoi membri non ha valore."""
    op = f[0]
    if op == "cmp":
        try:
            return RELATIONS[f[1]](sign(value(f[2], env) - value(f[3], env)))
        except Undefined:
            return False
    if op == "not":
        return not holds(f[1], env)
    if op == "and":
        return holds(f[1], env) and holds(f[2], env)
    if op == "or":
        return holds(f[1], env) or holds(f[2], env)
    if op == "iff":
        return holds(f[1], env) == holds(f[2], env)
    if op in ("true", "false"):
        return op == "true"
    if op in ("forall", "exists") and f[2][0] == "cmp":
        # Il confronto dice qualcosa del segno di un polinomio in f[1]: "per ogni" chiede che lo
        # dicano tutti i segni che il polinomio assume, "esiste" che lo dica almeno uno.
        try:
            taken = signs_taken(polynomial(("-", f[2][2], f[2][3]), f[1], env))
        except NotPolynomial:
            raise Unreadable("a quantified comparison that is not a polynomial") from None
        except Undefined:
            return False
        return (all if op == "forall" else any)(RELATIONS[f[2][1]](s) for s in taken)
    raise Unreadable("a condition the oracle cannot decide: %s" % op)


def judge(f, env):
    """La verità di f in env, e se è certa: non lo è quando un confronto è stato deciso con un
    pareggio, cioè senza poter separare i due membri."""
    del ties[:]
    answer = holds(f, env)
    return answer, not ties


def surely(f, env):
    """La verità di f in env, solo se è certa: altrimenti solleva Unreadable."""
    answer, certain = judge(f, env)
    if not certain:
        raise Unreadable("a comparison too close to call")
    return answer


# ---------------------------------------------------------------- dove la verità può cambiare
def zeros(e, name, env, out):
    """Aggiunge a out i valori di name in cui l'espressione e vale zero. Ritorna True se li ha
    trovati tutti: sa farlo per ciò che è zero solo dove lo è una sua parte (radice, valore
    assoluto, potenza, prodotto, numeratore di una frazione) e per un polinomio di grado al più 2.
    Un prodotto si guarda fattore per fattore prima che come polinomio: le radici di (x - sqrt(2))
    (x - sqrt(3)) sono così esatte, mentre il discriminante del prodotto sviluppato non è un razionale."""
    if e[0] in ("sqrt", "abs", "neg", "^", "/"):
        return zeros(e[1], name, env, out)
    if e[0] == "*":
        return zeros(e[1], name, env, out) & zeros(e[2], name, env, out)
    try:
        found = roots(polynomial(e, name, env))
    except NotPolynomial:
        return False
    out.extend(found or [])
    return found is not None


def singular(e, name, env, out):
    """Aggiunge a out i valori di name in cui e può smettere di avere un valore: gli zeri dei
    radicandi e dei divisori. Ritorna True se li ha trovati tutti. Tra due di questi punti
    consecutivi e ha un valore dappertutto o da nessuna parte, e dove lo ha è continua."""
    known = True
    if e[0] in ("sqrt", "/"):
        known = zeros(e[-1], name, env, out)
    for part in e[1:]:
        if isinstance(part, tuple):
            known = singular(part, name, env, out) and known
    return known


def critical(f, name, env, out):
    """Aggiunge a out i valori di name in cui la verità di f può cambiare, per quanto si sanno
    trovare: dove i due membri di un confronto coincidono e dove smettono di avere un valore.
    Ritorna True se sono tutti. Un confronto che dipende da una lettera che env non ha, o che non
    ha valore per nessun name, non ne aggiunge."""
    if f[0] == "cmp":
        difference = f[2] if f[3] == ("num", 0) else ("-", f[2], f[3])   # contro lo zero si vedono i fattori
        try:
            meet = zeros(difference, name, env, out)
            return singular(difference, name, env, out) and meet
        except (Undefined, KeyError):
            return True
    if f[0] in ("forall", "exists"):
        return False
    known = True
    for part in f[1:]:
        known = critical(part, name, env, out) and known
    return known


# I punti in più di check-data: accanto a ogni punto speciale, e tre griglie, fitte vicino allo zero
# e rade lontano. GRID per una lettera sola; con un parametro GRID_PARAMETER per lui e GRID_UNDER
# per la variabile, meno fitta perché lì le radici dei confronti sono già tra i punti speciali.
BESIDE = (Fraction(1, 1000), Fraction(-1, 1000), Fraction(1, 10 ** 7), Fraction(-1, 10 ** 7))
GRID = ([Fraction(k, 12) for k in range(-480, 481)]
        + [Fraction(side * k) for side in (1, -1) for k in list(range(41, 201)) + [10 ** 3, 10 ** 4, 10 ** 5, 10 ** 6]])
GRID_PARAMETER = ([Fraction(k, 12) for k in range(-72, 73)]
                  + [Fraction(side * k) for side in (1, -1) for k in list(range(7, 21)) + [100, 1000]])
GRID_UNDER = [Fraction(k, 2) for k in range(-24, 25)]


def line(f, name, env, hints, grid):
    """I valori di name in cui provare f, in ordine crescente: i punti in cui la sua verità può
    cambiare, quelli di hints, e ciò che samples() vi aggiunge. Con una griglia (check-data) anche
    i punti accanto a ciascuno e la griglia."""
    points = list(hints)
    critical(f, name, env, points)
    if grid:
        points += [p + d for p in points for d in BESIDE]
    return order(samples(points) + grid) if grid else samples(points)


def sweep(f, names, hints, dense):
    """Genera gli assegnamenti {lettera: valore} in cui provare f. names sono una lettera, oppure
    il parametro e la variabile: per ogni valore del parametro i punti della variabile sono
    cercati da capo. hints porta ogni lettera nei punti che chi chiama sa essere speciali."""
    if len(names) == 1:
        for x in line(f, names[0], {}, hints.get(names[0], []), GRID if dense else []):
            yield {names[0]: x}
        return
    parameter, variable = names
    for a in line(f, parameter, {}, hints.get(parameter, []), GRID_PARAMETER if dense else []):
        for x in line(f, variable, {parameter: a}, hints.get(variable, []), GRID_UNDER if dense else []):
            yield {parameter: a, variable: x}


# ---------------------------------------------------------------- scrittura
def number(q):
    """Un razionale in LaTeX: 3, -3, \\frac{1}{2}, -\\frac{1}{2}."""
    q = Fraction(q)
    if q.denominator == 1:
        return str(q.numerator)
    return "%s\\frac{%d}{%d}" % ("-" if q < 0 else "", abs(q.numerator), q.denominator)


def total(terms):
    """La somma dei termini (coefficiente, lettere): [(2, "ax"), (-1, "")] dà 2ax - 1. I termini
    con coefficiente zero sono omessi, e così il coefficiente 1 davanti a delle lettere."""
    out = ""
    for q, letters in terms:
        q = Fraction(q)
        if q == 0:
            continue
        body = letters if abs(q) == 1 and letters else number(abs(q)) + letters
        out += (" - " if q < 0 else " + ") + body if out else ("-" if q < 0 else "") + body
    return out or "0"


def endpoint(v):
    """Un estremo in LaTeX: un razionale, oppure (p + k sqrt(d)) / q scritto come sui libri
    (1 - \\sqrt{2}, \\frac{2 - \\sqrt{6}}{2}, -\\frac{\\sqrt{6}}{3}). None se non ha questa forma."""
    if not isinstance(v, Surd):
        return None if isinstance(v, Box) else number(v)
    radicands = [n for n in v.terms if n > 1]
    if len(radicands) != 1:
        return None
    rational_part, coefficient = v.terms.get(1, Fraction(0)), v.terms[radicands[0]]
    q = math.lcm(rational_part.denominator, coefficient.denominator)
    p, k, surd_text = rational_part * q, coefficient * q, "\\sqrt{%d}" % radicands[0]
    if q == 1:
        return total([(p, ""), (k, surd_text)])
    if p == 0:
        return "%s\\frac{%s}{%d}" % ("-" if k < 0 else "", total([(abs(k), surd_text)]), q)
    return "\\frac{%s}{%d}" % (total([(p, ""), (k, surd_text)]), q)


def show(env):
    """Un punto, per i messaggi: a = -1, x = 3/4."""
    def one(v):
        if isinstance(v, Fraction):
            return str(v)
        return "%s (about %.12g)" % (endpoint(v) or "a number without an exact writing", float(near(v)))
    return ", ".join("%s = %s" % (name, one(v)) for name, v in env.items())


# ---------------------------------------------------------------- check-data
def split_check(statement):
    """Di un enunciato \\forall ... (CONDIZIONE \\iff RISULTATO) ritorna (lettere, condizione, risultato)."""
    names = []
    while statement[0] == "forall":
        names.append(statement[1])
        statement = statement[2]
    if statement[0] != "iff" or not 1 <= len(names) <= 2:
        raise Unreadable("a check must be \\forall ... (CONDITION \\iff RESULT), with one or two letters")
    return names, statement[1], statement[2]


def split_solve(statement):
    """Di una richiesta di soluzioni ritorna (lettere, condizione)."""
    names = []
    if statement[0] == "forall":
        names.append(statement[1])
        statement = statement[2]
    if statement[0] != "solve":
        raise Unreadable("a solve request must be \\{x \\in \\mathbb{R} \\mid CONDITION\\}")
    return names + [statement[1]], statement[2]


def unlisted_crossings(condition, result, name, points):
    """Per un'equazione a = b in una lettera: genera le coppie di punti consecutivi di points tra
    cui c'è una soluzione che il risultato non dichiara.

    È il teorema dei valori intermedi. points è in ordine e contiene tutti i punti in cui a - b
    può smettere di avere un valore (lo controlla chi chiama, con singular()): tra due punti
    consecutivi in cui ha un valore, a - b è quindi continua. Se nei due ha segni opposti, in
    mezzo si annulla. E il risultato, che cambia solo nei suoi estremi, anch'essi tra i punti, lì
    in mezzo ha una verità sola: se in un punto interno è falso, quella soluzione manca."""
    difference, previous = ("-", condition[2], condition[3]), None
    for x in points:
        try:
            now = sign(value(difference, {name: x}))
        except Undefined:
            previous = None
            continue
        if previous and previous[1] * now < 0:
            middle = between(previous[0], x)
            if middle is not None and not holds(result, {name: middle}):
                yield previous[0], x
        previous = (x, now)


def examine(exercise):
    """Controlla un esercizio e stampa ciò che trova. Ritorna "confirmed", "not confirmed", "broken"
    (il dato non si può usare) oppure "not expressed" (manca l'enunciato)."""
    label = "exercise %s" % exercise["number"]
    if not exercise.get("check"):
        print("NOTE: %s is not expressed: %s" % (label, exercise.get("notes", "")))
        return "not expressed"

    # Le quattro scritture devono dire la stessa cosa: la richiesta e il risultato atteso sono i
    # due lati dell'enunciato.
    try:
        names, condition, result = split_check(read(exercise["check"]))
        if split_solve(read(exercise["solve"])) != (names, condition):
            raise Unreadable("solve and check do not have the same letters and condition")
        if read(exercise["expected"], "formula") != result:
            raise Unreadable("expected is not the right side of check")
    except Unreadable as error:
        print("BUG: %s cannot be used: %s" % (label, error))
        return "broken"

    # Condizione e risultato, punto per punto.
    outcome, count, both = "confirmed", 0, ("iff", condition, result)
    try:
        envs = list(sweep(both, names, {}, True))
        for env in envs:
            (c, certain_c), (r, certain_r) = judge(condition, env), judge(result, env)
            count += 1
            if c != r:
                print("DIFFER: %s at %s: the condition is %s, the result is %s" % (label, show(env), str(c).lower(), str(r).lower()))
                outcome = "not confirmed"
            elif not (certain_c and certain_r) and not (c and r):
                print("SUSPECT: %s at %s: a comparison is too close to call" % (label, show(env)))

        # Le soluzioni non dichiarate di un'equazione in una lettera.
        crossing = ""
        if len(names) == 1 and condition[0] == "cmp" and condition[1] == "=":
            if singular(("-", condition[2], condition[3]), names[0], {}, []) and critical(result, names[0], {}, []):
                crossing = ", no unlisted crossing"
                for low, high in unlisted_crossings(condition, result, names[0], [env[names[0]] for env in envs]):
                    print("DIFFER: %s: the two sides cross between %s and %s, where the result lists no solution" % (
                        label, show({names[0]: low}), show({names[0]: high})))
                    outcome = "not confirmed"
            else:
                print("NOTE: %s: the crossings of the two sides were not searched" % label)
    except (Unreadable, KeyError) as error:
        print("BUG: %s cannot be evaluated: %s" % (label, error))
        return "broken"

    # Le varianti false: stessa condizione, e nel testimone i due lati devono essere diversi, con certezza.
    wrong = 0
    for position, variant in enumerate(exercise["false_checks"], 1):
        try:
            other_names, other_condition, other_result = split_check(read(variant["check"]))
            if (other_names, other_condition) != (names, condition):
                raise Unreadable("it changes the letters or the condition, not only the result")
            env = {name: value(read(text, "expr"), {}) for name, text in variant["witness"].items()}
            if sorted(env) != sorted(names):
                raise Unreadable("the witness does not give a value to each letter")
            (c, certain_c), (r, certain_r) = judge(condition, env), judge(other_result, env)
            if c == r or not (certain_c and certain_r):
                raise Unreadable("at its witness %s condition and result do not differ" % show(env))
            wrong += 1
        except (Unreadable, Undefined, KeyError) as error:
            print("BUG: %s, false check %d: %s" % (label, position, error))
            outcome = "broken"
    if outcome == "confirmed":
        print("OK: %s (%s): condition and result agree at %d points%s; %d false variants differ at their witnesses" % (
            label, exercise["kind"], count, crossing, wrong))
    return outcome


def check_data(path):
    """Il modo check-data. Ritorna il codice di uscita: 0 se ogni esercizio scritto è confermato."""
    tally, failed = {}, False
    with open(path, encoding="utf-8") as lines:
        for row in lines:
            if not row.strip():
                continue
            exercise = json.loads(row)
            outcome = examine(exercise)
            tally[(exercise["kind"], outcome)] = tally.get((exercise["kind"], outcome), 0) + 1
            failed = failed or outcome in ("not confirmed", "broken")
    for kind in sorted({k for k, _ in tally}):
        print("%s: %s" % (kind, ", ".join("%d %s" % (v, o) for (k, o), v in sorted(tally.items()) if k == kind)))
    return 1 if failed else 0


# ---------------------------------------------------------------- generazione: una variabile
rnd = random.Random()   # fuzz() gli dà il seme richiesto


def rational():
    return Fraction(rnd.randint(-6, 6), rnd.choice([1, 1, 1, 2, 3]))


def nonzero(limit):
    return rnd.choice([k for k in range(-limit, limit + 1) if k])


def linear(a, b):
    """a x + b in LaTeX; a volte con il termine noto davanti, come 12 - x."""
    if b > 0 and a < 0 and rnd.random() < 0.5:
        return total([(b, ""), (a, "x")])
    return total([(a, "x"), (b, "")])


def relation():
    """Una relazione a caso: soprattutto equazioni, come negli esercizi dei libri."""
    return rnd.choice(["=", "=", "=", "<", "\\le", ">", "\\ge", "\\neq"])


def root_against_line():
    """(i-a) sqrt(a x + b) ? c x + d. Ritorna (condizione in LaTeX, punti in cui la verità può
    cambiare, radici estranee dell'equazione elevata al quadrato)."""
    c, d, r1, r2 = rnd.choice([1, 1, 2, -1, -2, 3]), Fraction(rnd.randint(-6, 6)), rational(), rational()
    a, b = 2 * c * d + c * c * (r1 + r2), d * d - c * c * r1 * r2
    if a == 0:
        raise Unreadable("the radicand does not depend on x")

    # L'identità su cui poggia il generatore: due polinomi di grado 2 uguali in tre punti sono uguali.
    assert all(a * t + b - (c * t + d) ** 2 == -c * c * (t - r1) * (t - r2) for t in (0, 1, 2))
    surd_text, layout = "\\sqrt{%s}" % linear(a, b), rnd.random()
    if layout < 0.5:
        text = "%s %s %s" % (surd_text, relation(), linear(c, d))
    elif layout < 0.75:
        text = "%s %s %s" % (linear(c, d), relation(), surd_text)
    else:
        text = "%s %s %s %s %s" % (surd_text, "-" if c > 0 else "+", total([(abs(c), "x")]), relation(), number(d))
    return text, [r1, r2, -b / a], [r for r in {r1, r2} if c * r + d < 0]


def two_roots():
    """(i-b) sqrt(a_1 x + b_1) +- sqrt(a_2 x + b_2) ? k. Ritorna come root_against_line()."""
    x0, m, n, s = rational(), rnd.randint(0, 5), rnd.randint(0, 5), rnd.choice([1, 1, -1])
    a1, a2, k = nonzero(4), nonzero(4), m + s * n
    if a1 == a2 or k == 0:
        raise Unreadable("a degenerate choice")
    b1, b2 = m * m - a1 * x0, n * n - a2 * x0

    # (k^2 + u_1 - u_2)^2 - 4 k^2 u_1 = q2 x^2 + q1 x + q0, con la radice x0: l'altra è la somma
    # delle radici, -q1/q2, meno x0.
    e = k * k + b1 - b2
    q2, q1, q0 = (a1 - a2) ** 2, 2 * (a1 - a2) * e - 4 * k * k * a1, e * e - 4 * k * k * b1
    x1 = -q1 / q2 - x0
    assert all(q2 * t * t + q1 * t + q0 == 0 for t in (x0, x1))
    first, second, sum_sign = "\\sqrt{%s}" % linear(a1, b1), "\\sqrt{%s}" % linear(a2, b2), "+" if s > 0 else "-"
    equation = "%s %s %s = %d" % (first, sum_sign, second, k)
    if rnd.random() < 0.6:
        text = "%s %s %s %s %d" % (first, sum_sign, second, relation(), k)
    else:
        text = "%s %s %d %s %s" % (first, relation(), k, "-" if s > 0 else "+", second)
    extraneous = [t for t in {x0, x1} if not surely(read(equation, "formula"), {"x": t})]
    return text, [x0, x1, -b1 / a1, -b2 / a2], extraneous


def root_against_root():
    """(i-c) l sqrt(a_1 x + b_1) ? sqrt(a_2 x + b_2), o lo stesso confronto scritto come quoziente.
    Ritorna come root_against_line()."""
    scale, a1, a2, x0, b1 = rnd.choice([1, 1, 2, 3]), nonzero(4), nonzero(4), rational(), Fraction(rnd.randint(-6, 6))
    if scale * scale * a1 == a2:
        raise Unreadable("a degenerate choice")
    b2 = scale * scale * (a1 * x0 + b1) - a2 * x0
    first, second = "\\sqrt{%s}" % linear(a1, b1), "\\sqrt{%s}" % linear(a2, b2)
    if rnd.random() < 0.6:
        text = "%s %s %s" % (total([(scale, first)]), relation(), second)
        equation = "%s = %s" % (total([(scale, first)]), second)
    else:
        text = "\\frac{%s}{%s} %s %d" % (second, first, relation(), scale)
        equation = "\\frac{%s}{%s} = %d" % (second, first, scale)
    extraneous = [] if surely(read(equation, "formula"), {"x": x0}) else [x0]
    return text, [x0, -b1 / a1, -b2 / a2], extraneous


def radical_constants():
    """(ii) Un prodotto o un quoziente di fattori con radici scelte, alcune con un radicale.
    Ritorna come root_against_line(): qui non ci sono radici estranee."""
    found, factors = [], []
    for _ in range(rnd.choice([1, 1, 2, 2, 3])):
        d, kind = rnd.choice([2, 3, 5, 6, 7]), rnd.random()
        surd_text, unit = "\\sqrt{%d}" % d, Surd({d: Fraction(1)})
        if kind < 0.25:
            r = rational()
            found.append(r)
            factors.append(total([(1, "x"), (-r, "")]))
        elif kind < 0.6:
            # (q x - p)^2 - k^2 d, che si annulla in (p +- k sqrt(d)) / q
            p, k, q = rnd.randint(-3, 3), rnd.choice([1, 1, 2, 3]), rnd.choice([1, 1, 2])
            found += [Fraction(p, q) + Fraction(side * k, q) * unit for side in (1, -1)]
            factors.append(total([(q * q, "x^{2}"), (-2 * p * q, "x"), (p * p - k * k * d, "")]))
        elif kind < 0.85:
            p, k = rnd.choice([0, 0, 1, -2]), rnd.choice([1, 1, 2, -1, -2])
            found.append(p + k * unit)
            factors.append(total([(1, "x"), (-p, ""), (-k, surd_text)]))
        else:
            factors.append(total([(1, "x^{2}"), (rnd.randint(1, 5), "")]))
    if not any(isinstance(r, Surd) for r in found):
        raise Unreadable("no radical this time")
    if len(factors) == 1:
        text = factors[0]
    elif rnd.random() < 0.6:
        text = "".join("(%s)" % factor for factor in factors)
    else:
        text = "\\frac{%s}{%s}" % ("".join("(%s)" % factor for factor in factors[:-1]) if len(factors) > 2 else factors[0], factors[-1])
    return "%s %s 0" % (text, rnd.choice(["<", "\\le", ">", "\\ge", "<", ">", "=", "\\neq"])), found, []


def solution(condition, name, points):
    """Le soluzioni di condition come lista di pezzi (basso, compreso, alto, compreso), con None
    per un estremo all'infinito, quando la verità può cambiare solo nei punti di points."""
    marks = order(points)
    row = samples(marks)
    if len(row) != 2 * len(marks) + 1:
        raise Unreadable("two endpoints too close to each other")

    # row alterna un punto dentro un intervallo (posti pari) e un punto speciale (posti dispari).
    # Le righe vere consecutive fanno un pezzo: la prima e l'ultima dicono se gli estremi sono compresi.
    pieces, current = [], None
    for i, x in enumerate(row):
        mark = i % 2 == 1
        low = x if mark else row[i - 1] if i else None
        high = x if mark else row[i + 1] if i + 1 < len(row) else None
        if not surely(condition, {name: x}):
            if current:
                pieces.append(tuple(current))
            current = None
        elif current:
            current[2:] = [high, mark]
        else:
            current = [low, mark, high, mark]
    if current:
        pieces.append(tuple(current))
    return pieces


def pieces_text(pieces):
    """I pezzi come condizione su x, uniti da \\lor. None se un estremo non si sa scrivere."""
    out = []
    for low, low_closed, high, high_closed in pieces:
        low_text, high_text = "" if low is None else endpoint(low), "" if high is None else endpoint(high)
        if low_text is None or high_text is None:
            return None
        if low is not None and high is not None and sign(high - low) == 0:
            out.append("x = %s" % low_text)
        elif low is None:
            out.append("x %s %s" % ("\\le" if high_closed else "<", high_text))
        elif high is None:
            out.append("x %s %s" % ("\\ge" if low_closed else ">", low_text))
        else:
            out.append("%s %s x %s %s" % (low_text, "\\le" if low_closed else "<", "\\le" if high_closed else "<", high_text))
    return " \\lor ".join(out)


def claim(text, pieces):
    """L'enunciato "la condizione text ha queste soluzioni", nelle tre forme dei libri: nessuna
    soluzione, tutti i numeri, oppure condizione <=> soluzioni."""
    if not pieces:
        return "\\forall x \\in \\mathbb{R}\\ \\lnot \\left( %s \\right)" % text
    if pieces == [(None, False, None, False)]:
        return "\\forall x \\in \\mathbb{R}\\ \\left( %s \\right)" % text
    written = pieces_text(pieces)
    if written is None:
        raise Unreadable("an endpoint that cannot be written")
    return "\\forall x \\in \\mathbb{R}\\ \\left( %s \\iff \\left( %s \\right) \\right)" % (text, written)


def shifted(point):
    """Un estremo sbagliato di poco: spostato, o con il radicale cambiato di segno o arrotondato."""
    if not isinstance(point, Surd) or rnd.random() < 0.3:
        return point + rnd.choice([Fraction(1, 2), Fraction(-1, 2), 1, -1, Fraction(1, 7)])
    if rnd.random() < 0.5:
        return Surd({n: q if n == 1 else -q for n, q in point.terms.items()})
    return Fraction(round(near(point) * 10), 10)


def spoil(pieces, extraneous):
    """Un insieme di soluzioni sbagliato di poco: una radice estranea aggiunta, un pezzo
    dimenticato, un estremo aperto o chiuso al contrario, un estremo spostato. None se non trova
    come sbagliarlo. Se l'errore conta davvero lo stabilisce poi il conto, non questa funzione."""
    pieces, kind = [list(p) for p in pieces], rnd.random()
    if extraneous and kind < 0.35:
        r = rnd.choice(extraneous)
        pieces.append([r, True, r, True])
    elif kind < 0.5:
        if not pieces:
            return None
        del pieces[rnd.randrange(len(pieces))]
    else:
        ends = [(i, k) for i, p in enumerate(pieces) for k in (0, 2) if p[k] is not None]
        if not ends:
            return None
        i, k = rnd.choice(ends)
        low, high = pieces[i][0], pieces[i][2]
        single = low is not None and high is not None and sign(high - low) == 0
        if single:
            pieces[i][0] = pieces[i][2] = shifted(low)
        elif kind < 0.75:
            pieces[i][k + 1] = not pieces[i][k + 1]
        else:
            pieces[i][k] = shifted(pieces[i][k])
            low, high = pieces[i][0], pieces[i][2]
            if low is not None and high is not None and sign(high - low) <= 0:
                return None
    return [tuple(p) for p in pieces]


def one_variable(text, points, extraneous):
    """Di una condizione in x fa un caso (enunciato, verità, dati): una richiesta di soluzioni
    (verità None, e dati per controllare la risposta), un enunciato vero, oppure uno sbagliato di
    poco, la cui verità è calcolata in tutti i punti in cui può cambiare."""
    condition, hints = read(text, "formula"), {"x": points}
    shape = rnd.random()
    if shape < 0.3:
        return "\\{x \\in \\mathbb{R} \\mid %s\\}" % text, None, (condition, ["x"], hints)
    pieces = solution(condition, "x", points)
    if shape >= 0.6:
        pieces = spoil(pieces, extraneous)
        if pieces is None:
            raise Unreadable("nothing to spoil")
    statement = claim(text, pieces)
    body = read(statement)[2]
    truth = all([surely(body, env) for env in sweep(body, ["x"], hints, False)])
    assert truth or shape >= 0.6, "a statement true by construction is false: %s" % statement
    return statement, truth, None


# ---------------------------------------------------------------- generazione: un parametro
FLIPPED = {"<": "\\le", "\\le": "<", ">": "\\ge", "\\ge": ">"}
BETWEEN = {"<": "%s < x < %s", "\\le": "%s \\le x \\le %s", ">": "x < %s \\lor x > %s", "\\ge": "x \\le %s \\lor x \\ge %s"}


def case_list(a, rel, first, second, flaw):
    """Le soluzioni di (x - r_1)(x - r_2) rel 0, con r_i = m_i a + n_i dati come coppie (m_i, n_i),
    scritte come elenco di casi sul parametro a. flaw, se non è None, le sbaglia di poco:
    "endpoint" (un estremo), "strict" (< al posto di <= o viceversa), "order" (le radici non
    scambiate), "boundary" (il valore del parametro che separa i casi), "middle" (il caso in cui
    le radici coincidono), "drop" (un caso dimenticato). None se quell'errore qui non si può fare."""
    (m1, n1), (m2, n2) = first, second
    e1, e2 = total([(m1, a), (n1 + (flaw == "endpoint"), "")]), total([(m2, a), (n2, "")])
    if m1 == m2:
        # Le radici non si scambiano mai: un caso solo.
        if flaw in ("boundary", "middle", "drop"):
            return None
        low, high = (e1, e2) if (n1 < n2) != (flaw == "order") else (e2, e1)
        return BETWEEN[FLIPPED[rel] if flaw == "strict" else rel] % (low, high)

    # r_1 - r_2 = (m_1 - m_2)(a - star): sotto star la minore è r_1 quando m_1 > m_2.
    star = Fraction(n2 - n1, m1 - m2)
    low, high = (e1, e2) if m1 > m2 else (e2, e1)
    written, meet = number(star + (flaw == "boundary")), number(m1 * star + n1)
    middle = {"<": None, "\\le": "x = %s", ">": "x \\neq %s", "\\ge": ""}[rel]
    if flaw == "middle":
        middle = {"<": "x = %s", "\\le": None, ">": "", "\\ge": "x \\neq %s"}[rel]
    cases = [("%s < %s" % (a, written), BETWEEN[FLIPPED[rel] if flaw == "strict" else rel] % (low, high)),
             ("%s = %s" % (a, written), middle and middle % meet),
             ("%s > %s" % (a, written), BETWEEN[rel] % ((low, high) if flaw == "order" else (high, low)))]
    if flaw == "drop":
        del cases[rnd.choice([0, 2])]

    # Un caso senza soluzioni (None) non si scrive; uno in cui vanno bene tutti gli x ("") è la
    # sola condizione sul parametro.
    out = []
    for when, then in cases:
        if then is not None:
            out.append("(%s \\land %s)" % (when, "(%s)" % then if "\\lor" in then else then) if then else when)
    return " \\lor ".join(out)


def parametric():
    """(iii) (x - r_1)(x - r_2) ? 0 con r_i = m_i a + n_i, scritto sviluppato o in fattori. Ritorna
    un caso (enunciato, verità, dati) come one_variable()."""
    a, rel = rnd.choice("akmbt"), rnd.choice(["<", "\\le", ">", "\\ge"])
    first, second = (rnd.randint(-3, 3), rnd.randint(-4, 4)), (rnd.randint(-3, 3), rnd.randint(-4, 4))
    (m1, n1), (m2, n2) = first, second
    if first == second or m1 == m2 == 0:
        raise Unreadable("a degenerate choice")
    if rnd.random() < 0.6:
        text = "%s %s 0" % (total([(1, "x^{2}"), (-(m1 + m2), a + "x"), (-(n1 + n2), "x"), (m1 * m2, a + "^{2}"),
                                    (m1 * n2 + m2 * n1, a), (n1 * n2, "")]), rel)
    else:
        text = "(%s)(%s) %s 0" % (total([(1, "x"), (-m1, a), (-n1, "")]), total([(1, "x"), (-m2, a), (-n2, "")]), rel)

    # I valori del parametro da provare: quello in cui le radici coincidono, se c'è, e qualche altro.
    star = Fraction(n2 - n1, m1 - m2) if m1 != m2 else rational()
    hints = {a: [star, star - 2, star + 2, rational(), rational()]}
    shape = rnd.random()
    if shape < 0.3:
        return "\\forall %s \\in \\mathbb{R}\\ \\{x \\in \\mathbb{R} \\mid %s\\}" % (a, text), None, (read(text, "formula"), [a, "x"], hints)
    flaw = rnd.choice(["endpoint", "strict", "order", "boundary", "middle", "drop"]) if shape >= 0.6 else None
    result = case_list(a, rel, first, second, flaw)
    if result is None:
        raise Unreadable("this flaw does not apply")
    statement = "\\forall %s \\in \\mathbb{R}\\ \\forall x \\in \\mathbb{R}\\ \\left( %s \\iff \\left( %s \\right) \\right)" % (a, text, result)
    body = read(statement)[2][2]
    truth = all([surely(body, env) for env in sweep(body, [a, "x"], hints, False)])
    assert truth or flaw, "a statement true by construction is false: %s" % statement
    if truth and flaw:
        raise Unreadable("no point found where the flawed result differs")
    return statement, truth, None


# ---------------------------------------------------------------- fuzz
FAMILIES = [("irrational", lambda: one_variable(*root_against_line())), ("irrational", lambda: one_variable(*two_roots())),
            ("irrational", lambda: one_variable(*root_against_root())), ("radicals", lambda: one_variable(*radical_constants())),
            ("parametric", parametric), ("parametric", parametric)]


def check_answer(answer, data):
    """Confronta le soluzioni scritte dal programma con la condizione, in tutti i punti in cui
    l'una o le altre possono cambiare. Ritorna ("ok" | "bug" | "suspect", dove)."""
    condition, names, hints = data
    try:
        written = read(answer.split(": ", 1)[1].split(" (estremi irrazionali")[0], "formula")
        both, doubt = ("iff", condition, written), None
        for env in sweep(both, names, hints, False):
            agree, certain = judge(both, env)
            if not certain:
                doubt = env
            elif not agree:
                return "bug", "at %s" % show(env)
        return ("suspect", "too close to call at %s" % show(doubt)) if doubt else ("ok", "")
    except (Unreadable, KeyError, IndexError) as error:
        return "suspect", "cannot read the written solutions (%s)" % error


def fuzz(program, seed, count, work):
    """Il modo fuzz: genera count casi con il seme seed, li fa decidere a program e confronta."""
    rnd.seed(seed)
    cases = []
    while len(cases) < count:
        family, make = rnd.choice(FAMILIES)
        try:
            cases.append((family,) + make())
        except Unreadable:
            pass   # una scelta degenere, o un punto in cui il conto non è certo: se ne genera un altro
    path = os.path.join(work, "fuzz_radicals_parameters_%d.tex" % seed)
    with open(path, "w", encoding="utf-8") as out:
        out.write("\\begin{enumerate}\n")
        for _, text, _, _ in cases:
            out.write("\\item \\[ %s \\]\n" % text)
        out.write("\\end{enumerate}\n")
    run = subprocess.run([program, path], capture_output=True, text=True, errors="replace", timeout=1800)
    lines = [line for line in run.stdout.splitlines() if re.match(r"\s*\d+\. ", line)]
    if len(lines) != len(cases) or run.returncode not in (0, 1) or "Sanitizer" in run.stderr or "runtime error" in run.stderr:
        print("BUG: the program answered %d of %d statements (exit %d)\n%s" % (len(lines), len(cases), run.returncode, run.stderr[-2000:]))
        return
    tally = {}
    for position, ((family, text, truth, data), row) in enumerate(zip(cases, lines), 1):
        answer = row.split(". ", 1)[1]
        verdict = ("P" if answer.startswith("DIMOSTRATO") else "R" if answer.startswith("FALSO") else "S" if answer.startswith("RISOLTO")
                   else "N" if answer.startswith("NON DIMOSTRATO") else "U")
        kind = "solve" if truth is None else "true" if truth else "false"
        tally[(family, kind, verdict)] = tally.get((family, kind, verdict), 0) + 1
        if truth is None and verdict == "S":
            outcome, where = check_answer(answer, data)
            if outcome == "bug":
                print("BUG: statement %d, the written solutions are wrong %s: %s\n   %s" % (position, where, answer[:300], text))
            elif outcome == "suspect":
                print("SUSPECT: statement %d, %s: %s\n   %s" % (position, where, answer[:300], text))
        elif (truth is None) != (verdict == "S") and verdict in ("P", "R", "S"):
            print("BUG: statement %d got the wrong kind of answer: %s\n   %s" % (position, answer[:90], text))
        elif (verdict == "P" and not truth) or (verdict == "R" and truth):
            print("BUG: statement %d is %s but the program says %s\n   %s" % (position, "true" if truth else "false", answer[:90], text))
        elif verdict in ("N", "U"):
            print("NOTE: statement %d not decided (%s)\n   %s" % (position, answer[:100], text[:300]))
    if "ATTENZIONE" in run.stdout:
        print("BUG: the program discarded a step of its own proof: %s" % [line for line in run.stdout.splitlines() if "ATTENZIONE" in line])
    print("seed %d:" % seed, ", ".join("%s:%s/%s=%d" % (k + (v,)) for k, v in sorted(tally.items())))


if len(sys.argv) == 3 and sys.argv[1] == "check-data":
    sys.exit(check_data(sys.argv[2]))
elif len(sys.argv) == 6 and sys.argv[1] == "fuzz":
    fuzz(sys.argv[2], int(sys.argv[3]), int(sys.argv[4]), sys.argv[5])
else:
    sys.exit(__doc__)
