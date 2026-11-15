r"""Fuzzing differenziale dell'algebra lineare sulle matrici a elementi razionali
(capitolo 31 di parser.c, capitolo 32 di main.c, capitolo 17 di checker.c).

Uso:
    python3 -I fuzz_linear_algebra.py <eseguibile> <seme> <quanti> <cartella di lavoro> [<checker>]
    python3 -I fuzz_linear_algebra.py letters <eseguibile> <seme> <quanti> <cartella di lavoro> [<checker>]
    python3 -I fuzz_linear_algebra.py data <file .tex>...
    python3 -I fuzz_linear_algebra.py answers <file .tex di richieste> <test .c con le soluzioni attese>
    python3 -I fuzz_linear_algebra.py systems <eseguibile> <seme> <quanti> <cartella di lavoro> [<checker>]
    python3 -I fuzz_linear_algebra.py systems-data <file .tex>...
    python3 -I fuzz_linear_algebra.py systems-answers <file .tex di richieste> <test .c con le risposte attese>
    python3 -I fuzz_linear_algebra.py parameter <eseguibile> <seme> <quanti> <cartella di lavoro> [<checker>]
    python3 -I fuzz_linear_algebra.py parameter-odd <eseguibile> <seme> <quanti> <cartella di lavoro> [<checker>]
    python3 -I fuzz_linear_algebra.py parameter-data <file .tex>...
    python3 -I fuzz_linear_algebra.py parameter-answers <file .tex di richieste> <test .c con le risposte attese>

I tre modi systems sono per i sistemi lineari (paragrafo 31s di parser.c, capitolo 33 di
main.c, paragrafo 17f di checker.c): vedi I SISTEMI LINEARI, piu sotto. I tre modi
parameter sono per i ranghi e i sistemi con un parametro, discussi per casi (33f-33h di
main.c, la voce where di 17d di checker.c): vedi CON UN PARAMETRO: LA DISCUSSIONE.

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
prodotto. In piu la richiesta dei valori della lettera per cui un rango e 2,
che il programma discute per casi (33h): la giudica l'oracolo del parametro,
piu sotto, con i minori. E cio che con una lettera non si tratta (inversa,
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
stesso polinomio. I certificati di un rango discusso per casi hanno anche la
voce where, e un testimone: quelli si controllano sui numeri, in molti valori
della lettera (vedi CON UN PARAMETRO: LA DISCUSSIONE).

I SISTEMI LINEARI

Gli enunciati (modo systems): l'insieme delle soluzioni di un sistema, scritto con le
incognite dichiarate (anche con il pedice), con le equazioni unite da \land, da virgole
o in cases, oppure con un vettore per incognita, A x = B, anche con i nomi; da solo (una
richiesta), uguale o diverso da un insieme scritto (vuoto, un punto, una soluzione
particolare piu i parametri, o una tupla con i parametri dentro; giusto, scritto con
un'altra soluzione particolare e altri vettori, con un vettore di troppo, oppure
sbagliato di poco: una componente, un vettore, un parametro dimenticato o in piu), e un
vettore con la sua appartenenza. In piu cio che non e un sistema lineare (un prodotto di
incognite, un vettore della lunghezza sbagliata, una lettera non dichiarata) e gli
enunciati rovinati, che il programma non deve decidere.

I sistemi si costruiscono in modo da conoscerne le soluzioni senza risolverli (la classe
SystemGenerator). Ma la verita di ogni enunciato la stabilisce l'oracolo rileggendo il
testo: legge le equazioni come polinomi nelle incognite, ne prende i coefficienti, e
decide con i ranghi, calcolati con i minori. Il sistema ha soluzioni quando il rango
della matrice dei coefficienti e quello della matrice completa (Rouche-Capelli);
l'insieme scritto p + U t e quello delle soluzioni quando A p = b, A U = 0 e il rango di
U e il numero delle incognite meno il rango di A (teorema della dimensione). Il
programma usa l'eliminazione di Gauss, il verificatore solo prodotti: i due teoremi di
qui non li usa nessuno dei due. La risposta di una richiesta si rilegge qui, e deve
essere l'insieme delle soluzioni con i vettori indipendenti.

I certificati alterati: la verita, la relazione, un numero, una dimensione, le parole
solutions, affine e nothing, e nelle voci witness un numero, il tipo, la voce tolta,
ripetuta, presa da un altro certificato, azzerata, o sostituita con il punto scritto.
Per ognuno che il verificatore accetta l'oracolo pretende che l'enunciato valga, o non
valga, come dichiarato (con i ranghi) e che i testimoni siano quelli che il caso chiede
e tornino (con i suoi prodotti).

I modi systems-data e systems-answers sono il terzo conto sui dati dei libri e sulle
risposte attese del test in C, senza il programma.

CON UN PARAMETRO: LA DISCUSSIONE

Gli enunciati (modo parameter): un rango o un sistema con il parametro k in una riga o in
una colonna (cosi i valori speciali sono razionali; ogni tanto in due posti qualsiasi),
come richiesta da discutere per casi, come richiesta dei valori del parametro, e dentro
un "per ogni" o un "esiste", anche con "se e solo se" e la condizione giusta, calcolata
dall'oracolo, oppure sbagliata di poco (un valore spostato, tolto o aggiunto). In un
secondo tempo, dai casi che il programma ha scritto per i sistemi: "se vale la
condizione, le soluzioni sono queste", com'e scritto e sbagliato di poco (un numero delle
soluzioni cambiato, una parte della condizione tolta).

L'oracolo non discute come il programma (un minore scelto, la sua aggiunta, i polinomi
interpolati). Legge i coefficienti come frazioni di polinomi in k e raccoglie tutti i
polinomi da cui la verita di un fatto puo dipendere: i denominatori, tutti i minori (di
ogni ordine), i residui A p - b e A U. Fuori dalle loro radici la verita e una sola, e la
calcola in un punto; in ogni radice razionale la calcola li, con i ranghi presi dai
minori. Il fatto diventa una condizione su k e l'enunciato che resta lo decide la tabella
dell'oracolo con una lettera: e un conto completo, tranne quando un polinomio ha una
radice irrazionale, e allora l'oracolo non decide. I casi scritti dal programma si
rileggono qui: ogni caso deve dare un enunciato vero, e le condizioni coprire tutti i k.

I certificati con le voci letters e where, del programma e alterati, si controllano sui
numeri: in molti valori del parametro che non annullano il polinomio dichiarato la lettera
diventa quel numero, e il certificato deve essere uno giusto tra quelli sui numeri. Qui un
si vuol dire "in quei valori".

Il modo parameter-odd scrive enunciati strani (insiemi scritti con divisori nulli,
frazioni di frazioni, potenze negative, radici; un terzo rovinato): guarda prima che il
programma finisca su ognuno (un conto che non finisce e un BUG), poi confronta i
verdetti con lo stesso oracolo. Molti sono fuori da cio che il programma tratta, apposta:
li i MISS sono attesi.

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
        """Cio che il capitolo 32 con una lettera non tratta. Il primo, un rango confrontato
        con un numero, il programma lo discute per casi (33h), e lo giudica l'oracolo del
        parametro; gli altri non deve deciderli."""
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


def lettered_judge(text):
    """Che cosa dice l'oracolo di un enunciato del modo letters. Cio che letter_judge() non
    legge perche nomina un rango passa all'oracolo del parametro (parameter_judge()), che
    mette al posto del rango la condizione sulla lettera trovata con i minori; se nemmeno
    lui lo legge, resta non letto."""
    expected = letter_judge(text)
    if expected[0] == "unreadable" and "rank" in text:
        again = parameter_judge(text)
        if again[0] != "unreadable":
            return again
    return expected


def lettered_cert_judge(cert):
    """Il giudizio dell'oracolo su un certificato del modo letters: quello completo di
    cert_judge(), che sviluppa i polinomi. Un certificato con la voce where (un rango
    discusso per casi) cert_judge() non lo conosce: per quello vale il conto in molti
    valori della lettera di sampled_cert_judge()."""
    lines = [l for l in cert.strip().split("\n") if l.strip()]
    if len(lines) > 2 and lines[2].split()[:1] == ["where"]:
        return sampled_cert_judge(cert)
    return cert_judge(cert)


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
        cases.append((text, lettered_judge(text) if "\\mathbb" in text else judge(text)))
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
                    good = lettered_cert_judge(cert)
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
                    good = lettered_cert_judge(variant)
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


# ------------------------------------------------------------ i sistemi lineari
#
# Un enunciato su un sistema dice qualcosa dell'insieme delle soluzioni di A x = b. Il
# programma lo decide con l'eliminazione di Gauss e lo fa controllare con dei prodotti;
# qui non c'e ne l'una ne gli altri. L'oracolo legge le equazioni come polinomi nelle
# incognite (la classe Poly) e ne prende i coefficienti, e poi decide con i ranghi,
# calcolati con i minori (rank()):
#
#   - il sistema ha soluzioni quando il rango di A e quello di A con accanto b (teorema di
#     Rouche-Capelli);
#   - l'insieme scritto p + U t e quello delle soluzioni quando A p = b, A U = 0 e il rango
#     di U e n meno il rango di A: i vettori di U stanno nel nucleo di A, che ha
#     dimensione n meno il rango (teorema della dimensione), e ne generano una parte della
#     stessa dimensione, cioe tutto.
#
# Sono due teoremi che ne il motore ne il verificatore usano.

class SystemReader(Reader):
    """Un lettore del paragrafo 31s, scritto senza guardare quello del programma. Le
    minuscole sono lettere (incognite e parametri), anche con un pedice numerico."""

    def __init__(self, text):
        Reader.__init__(self, text)
        self.letters = True
        self.vector = None  # (lettera, n) mentre si legge la condizione A x = B

    def primary(self):
        t = self.peek()
        if t is not None and len(t) == 1 and t.islower():
            name = self.letter()
            if self.vector and name == self.vector[0]:
                # Il vettore incognito: una colonna di lettere nuove.
                return ("mat", tuple((("var", "%s#%d" % (name, i)),) for i in range(self.vector[1])))
            return ("var", name)
        return Reader.primary(self)

    def letter(self):
        t = self.take()
        if not (len(t) == 1 and t.islower()):
            raise Unreadable("a letter was expected, found %r" % t)
        if self.peek() == "_":
            self.take()
            index = self.group()
            if index[0] != "num":
                raise Unreadable("subscript")
            return "%s_%d" % (t, int(index[1]))
        return t

    def reals(self):
        for expected in ("\\mathbb", "{", "R", "}"):
            self.take(expected)

    def equation(self):
        left = self.sum()
        if self.peek() == "&":
            self.take()
        self.take("=")
        if self.peek() == "&":
            self.take()
        return left, self.sum()

    def solutions(self):
        """Ritorna (n, sistema): sistema e ("equations", incognite, coppie di alberi) oppure
        ("vector", incognite, primo membro, secondo membro)."""
        self.take("\\{")
        vector = self.peek() != "("
        if vector:
            names = [self.letter()]
        else:
            self.take("(")
            names = [self.letter()]
            while self.peek() == ",":
                self.take()
                names.append(self.letter())
            self.take(")")
        self.take("\\in")
        self.reals()
        self.take("^")
        size = self.group()
        if size[0] != "num":
            raise Unreadable("exponent")
        n = int(size[1])
        if len(set(names)) != len(names) or (not vector and len(names) != n) or n < 2 or n > 12 or (vector and "_" in names[0]):
            raise Unreadable("unknowns")
        if self.take() not in ("\\mid", ":"):
            raise Unreadable("\\mid")
        if vector:
            self.vector = (names[0], n)
            left = self.sum()
            self.take("=")
            right = self.sum()
            self.vector = None
            system = ("vector", ["%s#%d" % (names[0], i) for i in range(n)], left, right)
        elif self.peek() == "\\begin":
            self.take()
            if self.word() != "cases":
                raise Unreadable("environment")
            pairs = []
            while True:
                pairs.append(self.equation())
                if self.peek() == "\\\\":
                    self.take()
                    if self.peek() == "\\end":
                        break
                    continue
                break
            self.take("\\end")
            if self.word() != "cases":
                raise Unreadable("environment")
            system = ("equations", names, pairs)
        else:
            pairs = [self.equation()]
            while self.peek() in ("\\land", "\\wedge", ","):
                self.take()
                pairs.append(self.equation())
            system = ("equations", names, pairs)
        self.take("\\}")
        return n, system

    def tuple_here(self):
        """Il token corrente apre una tupla: dopo la prima espressione c'e una virgola. La
        lettura di prova puo spezzare un numero in due token (un esponente senza graffe):
        si rimette com'era anche l'elenco dei token."""
        if self.peek() != "(":
            return False
        tokens, mark = list(self.tok), self.pos
        try:
            self.take("(")
            self.sum()
            return self.peek() == ","
        except Unreadable:
            return False
        finally:
            self.tok, self.pos = tokens, mark

    def vector_sum(self):
        """Un vettore scritto come somma di tuple, ognuna con i suoi fattori davanti:
        ritorna gli alberi delle componenti."""
        components = None
        negative = False
        if self.peek() in ("+", "-"):
            negative = self.take() == "-"
        while True:
            factor = None
            while not self.tuple_here():
                if not self.starts_factor():
                    raise Unreadable("a vector was expected")
                piece = self.factor()
                factor = piece if factor is None else ("mul", factor, piece)
                if self.peek() == "\\cdot":
                    self.take()
            self.take("(")
            entries = [self.sum()]
            while self.peek() == ",":
                self.take()
                entries.append(self.sum())
            self.take(")")
            if factor is not None:
                entries = [("mul", factor, e) for e in entries]
            if components is None:
                components = [("neg", e) if negative else e for e in entries]
            elif len(entries) != len(components):
                raise Unreadable("vectors of different lengths")
            else:
                components = [("sub" if negative else "add", c, e) for c, e in zip(components, entries)]
            if self.peek() not in ("+", "-"):
                return components
            negative = self.take() == "-"

    def written(self):
        """Un insieme scritto: ("empty",) oppure ("set", componenti, parametri)."""
        if self.peek() in ("\\emptyset", "\\varnothing"):
            self.take()
            return ("empty",)
        self.take("\\{")
        components = self.vector_sum()
        parameters = []
        if self.peek() in ("\\mid", ":"):
            self.take()
            parameters.append(self.letter())
            while self.peek() == ",":
                self.take()
                parameters.append(self.letter())
            self.take("\\in")
            self.reals()
            if len(set(parameters)) != len(parameters):
                raise Unreadable("a parameter twice")
        self.take("\\}")
        return ("set", components, parameters)

    def definitions(self):
        """Le definizioni di matrici davanti all'enunciato: A = ..., B = ... \\implies."""
        while True:
            name = self.take()
            if not (len(name) == 1 and name.isupper()) or name in self.names:
                raise Unreadable("definition of %r" % name)
            self.take("=")
            self.names[name] = self.sum()
            if self.take() == "\\implies":
                break
            if self.tok[self.pos - 1] != ",":
                raise Unreadable("after a definition")

    def fact(self):
        """Ritorna (n, sistema, relazione o None, insieme scritto o None)."""
        if "\\implies" in self.tok:
            self.definitions()
        if self.peek() == "\\{":
            n, system = self.solutions()
            if self.peek() is None:
                return n, system, None, None
            relation = {"=": "=", "\\neq": "neq", "\\ne": "neq"}.get(self.take())
            if relation is None:
                raise Unreadable("relation")
            other = self.written()
        else:
            components = self.vector_sum()
            relation = {"\\in": "in", "\\notin": "notin"}.get(self.take())
            if relation is None:
                raise Unreadable("relation")
            n, system = self.solutions()
            other = ("set", components, [])
        if self.peek() is not None:
            raise Unreadable("text after the statement: %r" % self.peek())
        return n, system, relation, other


def affine_parts(v, names):
    """I coefficienti delle lettere names e il termine noto di un valore, che deve essere
    di primo grado in quelle lettere e non contenerne altre."""
    if is_matrix(v) or is_unit(v):
        raise Undefined("a matrix where a number is expected")
    coefficients, constant = {name: Fr(0) for name in names}, Fr(0)
    for monomial, c in Poly.of(v).terms.items():
        if monomial == ():
            constant = c
        elif len(monomial) == 1 and monomial[0][1] == 1 and monomial[0][0] in coefficients:
            coefficients[monomial[0][0]] = c
        else:
            raise Unreadable("not of the first degree in its letters")
    return [coefficients[name] for name in names], constant


def system_matrices(system, value=value, affine_parts=affine_parts):
    """La matrice dei coefficienti, come tupla di righe, e la lista dei termini noti. Con
    value e affine_parts quelli dei numeri; con un parametro, rvalue() e
    affine_fractions()."""
    if system[0] == "equations":
        rows, rhs = [], []
        for left, right in system[2]:
            l, r = value(left), value(right)
            if is_matrix(l) or is_matrix(r) or is_unit(l) or is_unit(r):
                raise Undefined("a matrix in an equation between numbers")
            coefficients, constant = affine_parts(add(l, r, -1), system[1])
            rows.append(tuple(coefficients))
            rhs.append(-constant)
        return tuple(rows), rhs
    # A x = B: la differenza dei due membri, elemento per elemento. Lo 0 da solo e la
    # matrice nulla.
    left, right = value(system[2]), value(system[3])
    if is_zero_literal(system[3]) and is_matrix(left):
        difference = left
    elif is_zero_literal(system[2]) and is_matrix(right):
        difference = mul(Fr(-1), right)
    else:
        if is_unit(left) and is_matrix(right):
            left = identity(shape(right)[0], left[1])
        if is_unit(right) and is_matrix(left):
            right = identity(shape(left)[0], right[1])
        if not is_matrix(left) or not is_matrix(right):
            raise Undefined("the two sides of A x = B are matrices")
        difference = add(left, right, -1)
    rows, rhs = [], []
    for row in difference:
        for entry in row:
            coefficients, constant = affine_parts(entry, system[1])
            rows.append(tuple(coefficients))
            rhs.append(-constant)
    return tuple(rows), rhs


def system_solvable(a, b):
    """Rouche-Capelli: il rango di A e quello di A con accanto b."""
    return rank(a) == rank(tuple(row + (c,) for row, c in zip(a, b)))


def written_parts(other, n, value=value, affine_parts=affine_parts):
    """Il punto e i vettori (per righe: n righe, una colonna per parametro) di un insieme
    scritto con n componenti."""
    components, parameters = other[1], other[2]
    if len(components) != n:
        raise Undefined("a written vector of the wrong length")
    point, directions = [], []
    for tree in components:
        coefficients, constant = affine_parts(value(tree), parameters)
        point.append(constant)
        directions.append(tuple(coefficients))
    return point, directions


def system_holds(a, b, n, relation, other):
    """La verita dell'enunciato: relation tra l'insieme delle soluzioni di a x = b e
    l'insieme scritto other, o tra un vettore e quell'insieme."""
    if other[0] == "empty":
        equal = not system_solvable(a, b)
    else:
        point, directions = written_parts(other, n)
        equal = all(sum((x * y for x, y in zip(row, point)), Fr(0)) == c for row, c in zip(a, b))
        if relation in ("=", "neq") and equal:
            f = len(other[2])
            inside = all(sum((row[j] * directions[j][q] for j in range(n)), Fr(0)) == 0 for row in a for q in range(f))
            equal = inside and (rank(tuple(directions)) if f else 0) == n - rank(a)
    return equal if relation in ("=", "in") else not equal


def system_judge(text):
    """Che cosa dice l'oracolo dell'enunciato text su un sistema: ("true",), ("false",),
    ("request", (a, b, n)), ("undefined", perche), ("unreadable", perche), ("skip", perche)."""
    determinants[0] = 0
    try:
        n, system, relation, other = SystemReader(text).fact()
    except (Unreadable, IndexError) as problem:
        return ("unreadable", str(problem))
    try:
        a, b = system_matrices(system)
        if len(a[0]) != n:
            raise Undefined("unknowns")
        if relation is None:
            return ("request", (a, b, n))
        return ("true",) if system_holds(a, b, n, relation, other) else ("false",)
    except Undefined as problem:
        return ("undefined", str(problem))
    except Unreadable as problem:
        return ("unreadable", str(problem))
    except TooBig as problem:
        return ("skip", str(problem))


def system_answer_right(request, written):
    """Dice se l'insieme written, com'e scritto dal programma o atteso da un test, e
    l'insieme delle soluzioni della richiesta, e se i suoi parametri sono liberi: i vettori
    che moltiplicano sono indipendenti."""
    a, b, n = request
    reader = SystemReader(written)
    other = reader.written()
    if reader.peek() is not None:
        raise Unreadable("text after the written set")
    if not system_holds(a, b, n, "=", other):
        return False
    if other[0] == "empty" or not other[2]:
        return True
    return rank(tuple(written_parts(other, n)[1])) == len(other[2])


def s_term(c, body):
    """Un addendo c body, con il segno: (segno, testo senza segno)."""
    c = Fr(c)
    magnitude = abs(c)
    if not body:
        return c < 0, tex_number(magnitude)
    if magnitude == 1:
        return c < 0, body
    return c < 0, "%s %s" % (tex_number(magnitude), body) if magnitude.denominator != 1 else "%d%s" % (magnitude.numerator, body)


def s_sum(terms):
    """La somma degli addendi (coefficiente, testo): "0" se non ce ne sono."""
    text = ""
    for c, body in terms:
        if c == 0:
            continue
        negative, piece = s_term(c, body)
        text += ("-" if negative else "") + piece if not text else (" - " if negative else " + ") + piece
    return text or "0"


class SystemGenerator:
    """Sistemi di cui si conoscono le soluzioni per costruzione, senza risolverli: A = M R
    con R gia ridotta (un 1 per riga nelle colonne dei perni, zeri negli altri perni) e M
    con le colonne indipendenti. Il nucleo di A e quello di R, e una sua base si legge da
    R: per ogni colonna libera j, il vettore con 1 nel posto j e l'opposto degli elementi
    di R nei posti dei perni."""

    def __init__(self, rnd):
        self.rnd = rnd

    def number(self):
        rnd = self.rnd
        return Fr(rnd.randint(-4, 4)) if rnd.random() < 0.8 else Fr(rnd.randint(-5, 5), rnd.randint(2, 4))

    def build(self):
        rnd = self.rnd
        n = rnd.choice([2, 2, 3, 3, 3, 4, 4, 5])
        m = rnd.randint(1, min(5, n + 2))
        r = rnd.randint(0 if rnd.random() < 0.05 else 1, min(m, n))
        pivots = sorted(rnd.sample(range(n), r))
        free = [j for j in range(n) if j not in pivots]
        reduced = [[Fr(0)] * n for _ in range(r)]
        for i, p in enumerate(pivots):
            reduced[i][p] = Fr(1)
            for j in free:
                reduced[i][j] = Fr(rnd.randint(-3, 3))
        for _ in range(60):
            left = [[Fr(rnd.randint(-3, 3)) for _ in range(r)] for _ in range(m)]
            if r == 0 or rank(tuple(map(tuple, left))) == r:
                break
        else:
            return None
        a = [[sum((left[i][k] * reduced[k][j] for k in range(r)), Fr(0)) for j in range(n)] for i in range(m)]
        kernel = []
        for j in free:
            v = [Fr(0)] * n
            v[j] = Fr(1)
            for i, p in enumerate(pivots):
                v[p] = -reduced[i][j]
            kernel.append(v)
        x0 = [self.number() for _ in range(n)]
        b = [sum((a[i][j] * x0[j] for j in range(n)), Fr(0)) for i in range(m)]
        solvable = True
        if r < m and rnd.random() < 0.3:
            # Termini noti fuori dallo spazio delle colonne: M con accanto w ha un rango in piu.
            for _ in range(20):
                w = [Fr(rnd.randint(-3, 3)) for _ in range(m)]
                if rank(tuple(tuple(left[i]) + (w[i],) for i in range(m))) == r + 1:
                    b = [x + y for x, y in zip(b, w)]
                    solvable = False
                    break
        return {"n": n, "m": m, "a": a, "b": b, "x0": x0, "kernel": kernel, "solvable": solvable, "pivots": pivots}

    def names(self, n):
        rnd = self.rnd
        style = rnd.random()
        if style < 0.45 and n <= 5:
            pool = list("xyzwt"[:n]) if rnd.random() < 0.7 else rnd.sample(list("xyzwtuv"), n)
        elif style < 0.75:
            letter = rnd.choice("xyab")
            pool = ["%s_%d" % (letter, i + 1) for i in range(n)]
        else:
            pool = rnd.sample(list("abcdfghpqruvwxyz"), n)
        return pool

    @staticmethod
    def show(name):
        return name if "_" not in name else "%s_{%s}" % tuple(name.split("_"))

    def equation(self, row, c, names):
        """L'equazione riga . incognite = c, con qualche addendo portato dall'altra parte."""
        rnd = self.rnd
        left, right = [], []
        for coefficient, name in zip(row, names):
            if coefficient == 0 and rnd.random() < 0.9:
                continue
            if rnd.random() < 0.15:
                right.append((-coefficient, self.show(name)))
            else:
                left.append((coefficient, self.show(name)))
        if rnd.random() < 0.2:
            left.append((-c, ""))
        else:
            right.append((c, ""))
        rnd.shuffle(left)
        return "%s = %s" % (s_sum(left), s_sum(right))

    def solutions_text(self, s, names):
        """L'insieme delle soluzioni, scritto in uno dei modi di 31s; e le definizioni da
        mettere davanti all'enunciato, se ce ne sono."""
        rnd = self.rnd
        n = s["n"]
        if rnd.random() < 0.18:
            # A x = B.
            a_text = tex_value(tuple(map(tuple, s["a"])), rnd)
            b_text = tex_value(tuple((x,) for x in s["b"]), rnd)
            letter = rnd.choice("xv")
            if all(x == 0 for x in s["b"]) and rnd.random() < 0.5:
                b_text = "0"
            if rnd.random() < 0.4:
                return "A = %s%s \\implies " % (a_text, "" if b_text == "0" else ", B = " + b_text), \
                       "\\{%s \\in \\mathbb{R}^{%d} \\mid A %s = %s\\}" % (letter, n, letter, "0" if b_text == "0" else "B")
            return "", "\\{%s \\in \\mathbb{R}^{%d} \\mid %s %s = %s\\}" % (letter, n, a_text, letter, b_text)
        equations = [self.equation(row, c, names) for row, c in zip(s["a"], s["b"])]
        style = rnd.random()
        if style < 0.4:
            condition = "\\begin{cases} %s \\end{cases}" % " \\\\ ".join(equations)
        else:
            condition = (" \\land " if style < 0.8 else ", ").join(equations)
        return "", "\\{(%s) \\in \\mathbb{R}^{%d} \\mid %s\\}" % (", ".join(self.show(x) for x in names), n, condition)

    def written_text(self, point, directions):
        """L'insieme scritto point + somma dei parametri per i vettori directions, in una
        delle due scritture: somma di tuple, oppure una tupla con i parametri dentro."""
        rnd = self.rnd
        letters = rnd.sample(list("tsuvpqr"), len(directions))
        tuple_of = lambda v: "(%s)" % ", ".join(tex_number(x) for x in v)
        if not directions:
            return "\\{%s\\}" % tuple_of(point)
        if rnd.random() < 0.5:
            pieces = [] if all(x == 0 for x in point) and rnd.random() < 0.7 else [tuple_of(point)]
            pieces += ["%s %s" % (letter, tuple_of(v)) for letter, v in zip(letters, directions)]
            body = " + ".join(pieces)
        else:
            body = "(%s)" % ", ".join(s_sum([(point[i], "")] + [(v[i], letter) for letter, v in zip(letters, directions)]) for i in range(len(point)))
        return "\\{%s \\mid %s \\in \\mathbb{R}\\}" % (body, ", ".join(letters))

    def another_writing(self, s):
        """Un'altra soluzione particolare e altri vettori per lo stesso insieme: x0 piu una
        combinazione del nucleo, e combinazioni indipendenti dei vettori del nucleo."""
        rnd = self.rnd
        kernel, f, n = s["kernel"], len(s["kernel"]), s["n"]
        shift = [Fr(rnd.randint(-2, 2)) for _ in range(f)]
        point = [s["x0"][i] + sum((shift[q] * kernel[q][i] for q in range(f)), Fr(0)) for i in range(n)]
        for _ in range(40):
            mix = [[Fr(rnd.randint(-2, 2)) for _ in range(f)] for _ in range(f)]
            if f == 0 or det(tuple(map(tuple, mix))) != 0:
                break
        else:
            mix = [[Fr(i == j) for j in range(f)] for i in range(f)]
        directions = [[sum((mix[q][k] * kernel[k][i] for k in range(f)), Fr(0)) for i in range(n)] for q in range(f)]
        if f and rnd.random() < 0.12:
            # Un vettore di troppo, combinazione degli altri: l'insieme non cambia.
            weights = [Fr(rnd.randint(-1, 2)) for _ in directions]
            directions.append([sum((w * v[i] for w, v in zip(weights, directions)), Fr(0)) for i in range(n)])
        return point, directions

    def statement(self):
        """Ritorna (testo, vero atteso dal generatore oppure None se non lo sa)."""
        rnd = self.rnd
        s = self.build()
        if s is None:
            return None
        n, names = s["n"], self.names(s["n"])
        prefix, solutions = self.solutions_text(s, names)
        kind = rnd.random()
        point, directions = self.another_writing(s)
        if kind < 0.22:
            return prefix + solutions, None
        if kind < 0.42:
            # Vero: le soluzioni, o l'insieme vuoto.
            if s["solvable"]:
                return "%s%s = %s" % (prefix, solutions, self.written_text(point, directions)), True
            return "%s%s = %s" % (prefix, solutions, rnd.choice(["\\emptyset", "\\varnothing"])), True
        if kind < 0.66:
            # Sbagliato di poco. Che sia falso lo decide l'oracolo: un numero cambiato puo
            # lasciare lo stesso insieme.
            choice = rnd.random()
            if not s["solvable"]:
                text = rnd.choice(["\\neq \\emptyset", "= " + self.written_text(s["x0"], [])])
            elif choice < 0.3:
                i = rnd.randrange(n)
                point[i] += rnd.choice([1, -1, Fr(1, 2)])
                text = "= " + self.written_text(point, directions)
            elif choice < 0.5 and directions:
                v = rnd.choice(directions)
                v[rnd.randrange(n)] += rnd.choice([1, -1])
                text = "= " + self.written_text(point, directions)
            elif choice < 0.7 and directions:
                directions.pop(rnd.randrange(len(directions)))
                text = "= " + self.written_text(point, directions)
            elif choice < 0.85 and len(directions) < 6:
                directions.append([Fr(i == rnd.choice(s["pivots"] or [0])) for i in range(n)])
                text = "= " + self.written_text(point, directions)
            else:
                text = "= \\emptyset"
            return "%s%s %s" % (prefix, solutions, text), None
        if kind < 0.76:
            # Con \neq: falso con l'insieme giusto, vero con uno sbagliato.
            if not s["solvable"]:
                return "%s%s \\neq %s" % (prefix, solutions, self.written_text(s["x0"], [])), True
            if rnd.random() < 0.5:
                return "%s%s \\neq %s" % (prefix, solutions, self.written_text(point, directions)), False
            point[rnd.randrange(n)] += 1
            return "%s%s \\neq %s" % (prefix, solutions, self.written_text(point, directions)), None
        if kind < 0.92:
            # Un vettore e la sua appartenenza.
            if rnd.random() < 0.5:
                point[rnd.randrange(n)] += rnd.choice([1, -1])
            vector = "(%s)" % ", ".join(tex_number(x) for x in point)
            if rnd.random() < 0.2:
                vector = "%s + (%s)" % (vector, ", ".join("0" for _ in point))
            return "%s%s %s %s" % (prefix, vector, rnd.choice(["\\in", "\\notin"]), solutions), None
        # Fuori dal linguaggio: non lineare, un vettore troppo lungo, una lettera non dichiarata.
        choice = rnd.random()
        if choice < 0.4 and "\\mid A" not in solutions and "pmatrix" not in solutions and "bmatrix" not in solutions:
            first = self.show(names[0])
            return prefix + solutions.replace(" = ", " + %s %s = " % (first, self.show(names[-1])), 1), None
        if choice < 0.7:
            return "%s%s = \\{(%s)\\}" % (prefix, solutions, ", ".join(tex_number(x) for x in point + [Fr(0)])), None
        return "%s%s = \\{(%s)\\}" % (prefix, solutions, ", ".join(["k"] + [tex_number(x) for x in point[1:]])), None


def system_cert_judge(text):
    """Ritorna True se il certificato di un enunciato su un sistema dice il vero e i suoi
    testimoni sono quelli che devono essere; False se dice il falso o se un testimone
    manca, avanza o non torna; solleva Unreadable se non e scritto nella notazione. La
    verita la stabiliscono i ranghi (system_holds()); i testimoni si ricontrollano qui con
    i prodotti dell'oracolo."""
    determinants[0] = 0
    cert_letters[0] = None
    lines = [l for l in text.strip().split("\n") if l.strip()]
    if len(lines) < 4 or lines[0].split() != ["certificate", "matrices"] or lines[-1].strip() != "end":
        raise Unreadable("entries")
    head, claim = lines[1].split(None, 1), lines[2].split()
    if len(head) != 2 or head[0] != "statement" or len(claim) != 2 or claim[0] != "claim" or claim[1] not in ("true", "false"):
        raise Unreadable("entries")
    witnesses = []
    for line in lines[3:-1]:
        entry = sexp("(" + line + ")")
        if len(entry) < 3 or entry[0] != "witness" or entry[1] not in ("combination", "solution", "span"):
            raise Unreadable("witness entry")
        witnesses.append(entry[1:])
    node = sexp(head[1])
    if not isinstance(node, list) or len(node) != 3 or node[0] not in ("=", "neq", "in", "notin"):
        raise Unreadable("relation")
    relation = node[0]
    member = relation in ("in", "notin")
    system, other = (node[2], node[1]) if member else (node[1], node[2])
    if not isinstance(system, list) or len(system) != 3 or system[0] != "solutions":
        raise Unreadable("solutions")
    try:
        a, b = cert_value(system[1], None), cert_value(system[2], None)
        if not is_matrix(a) or not is_matrix(b) or shape(b) != (len(a), 1):
            return False
        m, n = shape(a)
        point = directions = None
        nothing = other == ["nothing"]
        if member:
            point = cert_value(other, None)
        elif not nothing:
            if not isinstance(other, list) or other[0] != "affine" or len(other) not in (2, 3):
                raise Unreadable("written set")
            point = cert_value(other[1], None)
            directions = cert_value(other[2], None) if len(other) == 3 else None
        if point is not None and (not is_matrix(point) or shape(point) != (n, 1)):
            return False
        if directions is not None and (not is_matrix(directions) or shape(directions)[0] != n):
            return False
        matrices = [[cert_value(x, None) for x in w[1:]] for w in witnesses]
    except Undefined:
        return False
    if any(not is_matrix(x) for w in matrices for x in w):
        return False
    kinds = [w[0] for w in witnesses]
    rhs = [row[0] for row in b]
    zero = lambda rows, cols: tuple(tuple(Fr(0) for _ in range(cols)) for _ in range(rows))
    solves = lambda z: shape(z) == (n, 1) and mat_mul(a, z) == b

    # La verita, con i ranghi; poi i testimoni, con i prodotti.
    if nothing:
        equal = not system_solvable(a, rhs)
        if equal:
            right = kinds == ["combination"] and len(matrices[0]) == 1 and shape(matrices[0][0]) == (1, m) and \
                mat_mul(matrices[0][0], a) == zero(1, n) and mat_mul(matrices[0][0], b) != zero(1, 1)
        else:
            right = kinds == ["solution"] and len(matrices[0]) == 1 and solves(matrices[0][0])
    else:
        column = [row[0] for row in point]
        at_point = mat_mul(a, point) == b
        if member:
            equal, right = at_point, not kinds
        else:
            f = shape(directions)[1] if directions is not None else 0
            inside = at_point and (not f or mat_mul(a, directions) == zero(m, f))
            equal = inside and (rank(directions) if f else 0) == n - rank(a)
            if not inside:
                right = not kinds
            elif equal:
                right = kinds == ["span"] and len(matrices[0]) == (2 if f else 1) and shape(matrices[0][0]) == (n, m)
                if right:
                    total = mat_mul(matrices[0][0], a)
                    if f:
                        right = shape(matrices[0][1]) == (f, n)
                        total = add(total, mat_mul(directions, matrices[0][1]), 1) if right else total
                    right = right and total == identity(n)
            else:
                right = kinds == (["solution", "combination"] if f else ["solution"]) and all(len(w) == 1 for w in matrices) and solves(matrices[0][0])
                if right and f:
                    y = matrices[1][0]
                    right = shape(y) == (1, n) and mat_mul(y, directions) == zero(1, f) and \
                        mat_mul(y, add(matrices[0][0], point, -1)) != zero(1, 1)
                elif right:
                    right = [row[0] for row in matrices[0][0]] != column
    truth = equal if relation in ("=", "in") else not equal
    return right and truth == (claim[1] == "true")


def mutate_system(cert, rnd, pool):
    """Un certificato di un sistema alterato in un punto: la verita, la relazione, un
    numero, una dimensione, l'insieme scritto, un testimone (un numero, il tipo, la voce
    tolta, ripetuta, scambiata con quella di un altro certificato, azzerata, o sostituita
    con il punto scritto)."""
    lines = cert.rstrip("\n").split("\n")
    witnesses = [i for i, l in enumerate(lines) if l.startswith("witness ")]
    choice = rnd.random()
    if witnesses and choice < 0.35:
        where = rnd.choice(witnesses)
        how = rnd.random()
        tokens = re.findall(r"\(|\)|[^\s()]+", lines[where])
        numbers = [i for i, t in enumerate(tokens) if re.fullmatch(r"-?\d+", t) and tokens[i - 1] != "matrix" and tokens[i - 2] != "matrix"]
        if how < 0.4 and numbers:
            i = rnd.choice(numbers)
            tokens[i] = str(int(tokens[i]) + rnd.choice([1, -1, 2]))
            lines[where] = join(tokens)
        elif how < 0.5:
            lines[where] = lines[where].replace(tokens[1], rnd.choice(["combination", "solution", "span"]), 1)
        elif how < 0.6:
            del lines[where]
        elif how < 0.68:
            lines.insert(where, lines[where])
        elif how < 0.8 and pool:
            lines[where] = rnd.choice(pool)
        elif how < 0.9:
            # Azzerato: ogni matrice diventa la matrice nulla della stessa forma.
            lines[where] = re.sub(r"\(matrix (\d+) (\d+)[^()]*(?:\([^()]*\)[^()]*)*\)", r"(zero \1 \2)", lines[where])
        else:
            # Il punto scritto al posto del testimone.
            found = re.search(r"\(affine (\(matrix \d+ \d+[^()]*(?:\([^()]*\)[^()]*)*\))", lines[1])
            if found:
                lines[where] = "witness %s %s" % (tokens[1], found.group(1))
        return "\n".join(lines) + "\n"
    tokens = re.findall(r"\(|\)|\n|[^\s()]+", cert)
    places = [i for i, t in enumerate(tokens) if t not in ("\n", "certificate", "matrices", "statement", "claim", "end", "witness")]
    choice = rnd.random()
    if choice < 0.15:
        tokens = [("false" if t == "true" else "true") if t in ("true", "false") else t for t in tokens]
    elif choice < 0.3:
        relations = [i for i, t in enumerate(tokens) if i and tokens[i - 1] == "(" and t in ("=", "neq", "in", "notin") and i < 6]
        if relations:
            tokens[relations[0]] = rnd.choice(["=", "neq"] if tokens[relations[0]] in ("=", "neq") else ["in", "notin"])
    elif choice < 0.7:
        numbers = [i for i in places if re.fullmatch(r"-?\d+", tokens[i])]
        if numbers:
            i = rnd.choice(numbers)
            v = int(tokens[i])
            tokens[i] = str(rnd.choice([v + 1, v - 1, -v, 0, 2 * v, v + 7]))
    elif choice < 0.8:
        tables = [i for i in places if tokens[i] == "matrix" and i + 2 < len(tokens) and tokens[i + 1].isdigit() and tokens[i + 2].isdigit()]
        if tables:
            i = rnd.choice(tables)
            rows, cols = int(tokens[i + 1]), int(tokens[i + 2])
            tokens[i + 1], tokens[i + 2] = (str(cols), str(rows)) if rows != cols and rnd.random() < 0.6 else ("1", str(rows * cols))
    elif choice < 0.9:
        words = [i for i in places if tokens[i] in ("solutions", "affine", "nothing")]
        if words:
            i = rnd.choice(words)
            tokens[i] = rnd.choice(["solutions", "affine", "nothing"])
    elif places:
        i = rnd.choice(places)
        if rnd.random() < 0.5:
            del tokens[i]
        else:
            tokens.insert(i, tokens[i])
    out = join(tokens)
    return out if out.endswith("\n") else out + "\n"


def systems_mode(binary, seed, count, work, checker):
    """Il fuzz degli enunciati sui sistemi lineari: vedi "I SISTEMI LINEARI" in testa al file."""
    rnd = random.Random(seed)
    gen = SystemGenerator(rnd)
    os.makedirs(work, exist_ok=True)
    cases = []
    while len(cases) < count:
        made = gen.statement()
        if made is None or len(made[0]) > 3500:
            continue
        text, intended = made
        if rnd.random() < 0.05:
            text, intended = damage(text, rnd), None
            if len(text) > 3500:
                continue
        expected = system_judge(text)
        if intended is not None and expected[0] != ("true" if intended else "false"):
            print("ORACLE: the reader says %s where the generator built a %s statement\n   %s" % (expected, intended, text))
            continue
        cases.append((text, expected))
    tex_path = os.path.join(work, "systems_%d.tex" % seed)
    cert_path = os.path.join(work, "systems_%d.cert" % seed)
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
        outcome = (answer.split()[0] + (" " + answer.split()[1] if answer.startswith("NON") else "")).rstrip(":")
        tally[(expected[0], outcome)] = tally.get((expected[0], outcome), 0) + 1
        decided = outcome in ("DIMOSTRATO", "FALSO", "RISOLTO")
        if decided and "certificato verificato" not in answer:
            print("BUG: statement %d is decided without a verified certificate: %s\n   %s" % (number, answer[:120], text))
            bugs += 1
        if expected[0] in ("unreadable", "skip"):
            if decided and expected[0] == "unreadable":
                print("SUSPECT: statement %d is not a statement for the reader of this script (%s) but the program says %s\n   %s" % (number, expected[1], answer[:120], text))
                suspects += 1
        elif expected[0] == "undefined":
            if decided:
                print("BUG: statement %d has no meaning but the program says %s\n   %s" % (number, answer[:120], text))
                bugs += 1
        elif expected[0] == "request":
            if outcome == "RISOLTO":
                try:
                    if not system_answer_right(expected[1], answer.split(": ", 1)[1]):
                        print("BUG: statement %d is solved wrongly: %s\n   %s" % (number, answer[:300], text))
                        bugs += 1
                except (Unreadable, IndexError, Undefined) as problem:
                    print("SUSPECT: statement %d, cannot read the answer (%s): %s\n   %s" % (number, problem, answer[-200:], text))
                    suspects += 1
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
    accepted_wrong = mutated = accepted = 0
    if checker and os.path.exists(cert_path):
        certs = [c.lstrip("\n") + "end\n" for c in open(cert_path, encoding="utf-8").read().split("end\n") if c.strip()]
        run = subprocess.run([checker, cert_path], capture_output=True, text=True)
        answers = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
        for cert, answer in zip(certs, answers):
            try:
                good = system_cert_judge(cert)
            except (Unreadable, IndexError, TooBig) as problem:
                print("SUSPECT: the oracle cannot read a certificate of the program (%s)\n%s" % (problem, cert))
                suspects += 1
                continue
            if "ACCETTATO" not in answer or not good:
                print("BUG: a certificate of the program is %s by the checker and %s for the oracle\n%s" % (answer.split(". ", 1)[1][:80], good, cert))
                bugs += 1
        if len(answers) != len(certs):
            print("BUG: the checker answered %d of %d certificates" % (len(answers), len(certs)))
            bugs += 1
        variants = []
        pool = [line for cert in certs for line in cert.split("\n") if line.startswith("witness ")]
        while certs and len(variants) < count:
            variant = mutate_system(rnd.choice(certs), rnd, pool)
            if variant not in certs:
                variants.append(variant)
        mutated_path = os.path.join(work, "systems_%d_mutated.cert" % seed)
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
                    good = system_cert_judge(variant)
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

    print("Summary (seed %d): %d statements on linear systems" % (seed, len(cases)))
    for key in sorted(tally):
        print("   %-10s %-16s %d" % (key[0], key[1], tally[key]))
    if checker:
        print("   certificates altered: %d, accepted by the checker: %d, of which wrong: %d" % (mutated, accepted, accepted_wrong))
    print("BUG %d, SUSPECT %d, MISS %d" % (bugs, suspects, misses))
    sys.exit(1 if bugs else 0)


def systems_data_mode(paths):
    """Il terzo conto sui file di dati dei sistemi: che cosa ne dice l'oracolo."""
    words = {"true": "VERO", "false": "FALSO", "undefined": "NON DEFINITO", "unreadable": "NON LETTO", "request": "RICHIESTA", "skip": "NON DECISO DALL'ORACOLO"}
    for path in paths:
        print(path)
        counts = {}
        for number, text in enumerate(items_of(path), 1):
            verdict = system_judge(text)
            counts[words[verdict[0]]] = counts.get(words[verdict[0]], 0) + 1
            print("%4d. %s%s" % (number, words[verdict[0]], ": " + verdict[1] if verdict[0] in ("undefined", "unreadable", "skip") else ""))
        print("   " + ", ".join("%s %d" % item for item in sorted(counts.items())))


def systems_answers_mode(tex_path, c_path):
    """Le risposte attese delle richieste di un file di dati sui sistemi, scritte nel test
    in C in expected_answers[]: l'oracolo dice se ognuna e l'insieme delle soluzioni della
    sua richiesta, con i parametri liberi."""
    source = open(c_path, encoding="utf-8").read()
    block = re.search(r"expected_answers\[\] = \{\n(.*?)\n\};", source, re.S)
    answers = [m.group(1).replace("\\\\", "\\") for m in re.finditer(r'^\s*"(.*)",\s*$', block.group(1), re.M)] if block else []
    requests = items_of(tex_path)
    print("%s: %d requests, %d expected answers" % (tex_path, len(requests), len(answers)))
    wrong = 0
    for number, (text, answer) in enumerate(zip(requests, answers), 1):
        verdict = system_judge(text)
        try:
            if verdict[0] != "request":
                outcome = "NON E UNA RICHIESTA PER L'ORACOLO (%s)" % (verdict[1] if len(verdict) > 1 else verdict[0])
            else:
                outcome = "CONFERMATA" if system_answer_right(verdict[1], answer) else "SBAGLIATA"
        except (Unreadable, IndexError, Undefined, TooBig) as problem:
            outcome = "NON DECISA DALL'ORACOLO (%s)" % problem
        wrong += outcome != "CONFERMATA"
        print("%4d. %s: %s" % (number, outcome, answer))
    wrong += abs(len(requests) - len(answers))
    print("%d not confirmed" % wrong)
    sys.exit(1 if wrong else 0)


# ------------------------------------------- con un parametro: la discussione
#
# Un fatto con un parametro k (un rango confrontato con un numero, un insieme di soluzioni
# confrontato con un insieme scritto) vale per certi valori di k. Il programma lo discute
# con un minore scelto, la sua aggiunta e l'interpolazione; qui si fa in un altro modo. I
# coefficienti si leggono come frazioni di polinomi in k (la classe RatFn), e la verita del
# fatto dipende solo da quali, tra questi polinomi, si annullano: i denominatori, tutti i
# minori della matrice (per un sistema, della matrice completa e dei vettori scritti), i
# residui A p - b e A U. Un rango scende sotto un ordine solo dove i minori di quell'ordine
# si annullano tutti, cioe nelle radici del loro massimo comun divisore; un residuo si
# annulla, se non e nullo, nelle radici comuni alle sue componenti. Fuori da queste radici
# e da quelle dei denominatori la verita e una sola, e la si calcola in un punto qualsiasi
# (i ranghi con i minori, come sopra); in ogni radice razionale la si calcola li. Se una
# di quelle radici e irrazionale l'oracolo non decide. Il fatto diventa cosi una condizione su k, e l'enunciato che resta
# lo decide la tabella dell'oracolo con una lettera (letter_judge()): e completo, non a
# campione.

class RatFn:
    """Una frazione di polinomi (Poly), non ridotta: si somma e si moltiplica anche con i
    numeri, cosi i prodotti di matrici e lo sviluppo di Laplace valgono senza cambiare."""

    def __init__(self, num, den=1):
        self.num, self.den = Poly.of(num), Poly.of(den)

    @staticmethod
    def of(x):
        return x if isinstance(x, RatFn) else RatFn(x)

    def __add__(self, other):
        other = RatFn.of(other)
        return RatFn(self.num * other.den + other.num * self.den, self.den * other.den)

    __radd__ = __add__

    def __neg__(self):
        return RatFn(-self.num, self.den)

    def __sub__(self, other):
        return self + (-RatFn.of(other))

    def __rsub__(self, other):
        return RatFn.of(other) + (-self)

    def __mul__(self, other):
        other = RatFn.of(other)
        return RatFn(self.num * other.num, self.den * other.den)

    __rmul__ = __mul__

    def __truediv__(self, other):
        other = RatFn.of(other)
        if other.num == 0:
            raise Undefined("division by zero")
        return RatFn(self.num * other.den, self.den * other.num)

    def __pow__(self, k):
        out = RatFn(1)
        for _ in range(abs(k)):
            out = out * self
        return out if k >= 0 else RatFn(1) / out

    def __eq__(self, other):
        other = RatFn.of(other)
        return self.num * other.den == other.num * self.den

    def __ne__(self, other):
        return not self == other

    __hash__ = None

    def at(self, name, v):
        """Il numero che la frazione vale con v al posto della lettera name, la sola che ha."""
        num, den = self.num.at(name, v), self.den.at(name, v)
        if not num.is_number() or not den.is_number():
            raise Unreadable("a letter without a value")
        if den.number() == 0:
            raise Undefined("a denominator vanishes")
        return num.number() / den.number()


def rvalue(t):
    """Il valore di un albero con le lettere: una frazione di polinomi, o una matrice di
    frazioni. Il rango e l'inversa di una matrice con le lettere non sono un valore."""
    kind = t[0]
    if kind == "num":
        return t[1]
    if kind == "var":
        return RatFn(Poly.letter(t[1]))
    if kind == "mat":
        rows = tuple(tuple(rvalue(e) for e in row) for row in t[1])
        if any(is_matrix(e) or is_unit(e) for row in rows for e in row):
            raise Undefined("matrix inside a matrix")
        return rows
    if kind == "I":
        return identity(t[1]) if t[1] else ("unit", Fr(1))
    if kind == "neg":
        return mul(Fr(-1), rvalue(t[1]))
    if kind in ("add", "sub"):
        return add(rvalue(t[1]), rvalue(t[2]), 1 if kind == "add" else -1)
    if kind == "mul":
        return mul(rvalue(t[1]), rvalue(t[2]))
    if kind == "div":
        a, b = rvalue(t[1]), rvalue(t[2])
        if is_matrix(a) or is_matrix(b) or is_unit(a) or is_unit(b):
            raise Undefined("division")
        return RatFn.of(a) / b
    if kind == "pow":
        a = rvalue(t[1])
        if is_matrix(a) or is_unit(a):
            if t[2] < 0:
                raise Unreadable("the inverse of a matrix with a parameter")
            return power(a, t[2])
        return RatFn.of(a) ** t[2]
    a = rvalue(t[1])
    if not is_matrix(a):
        raise Undefined("%s of something that is not a written matrix" % kind)
    if kind == "T":
        return transpose(a)
    if kind == "det":
        return det(a)
    if kind == "tr" and shape(a)[0] == shape(a)[1]:
        return sum((a[i][i] for i in range(len(a))), Fr(0))
    raise Unreadable("%s inside an expression with a parameter" % kind)


def affine_fractions(v, names):
    """Come affine_parts(), con i coefficienti che sono frazioni di polinomi nelle lettere
    che non sono in names: le lettere di names non stanno a denominatore, e a numeratore
    compaiono al primo grado, una per monomio."""
    if is_matrix(v) or is_unit(v):
        raise Undefined("a matrix where a number is expected")
    v = RatFn.of(v)
    if any(name in names for name in v.den.letters()):
        raise Unreadable("an unknown in a denominator")
    coefficients, constant = {name: Poly() for name in names}, Poly()
    for monomial, c in v.num.terms.items():
        mine = [(n, e) for n, e in monomial if n in coefficients]
        rest = Poly({tuple((n, e) for n, e in monomial if n not in coefficients): c})
        if not mine:
            constant = constant + rest
        elif len(mine) == 1 and mine[0][1] == 1:
            coefficients[mine[0][0]] = coefficients[mine[0][0]] + rest
        else:
            raise Unreadable("not of the first degree in its letters")
    return [RatFn(coefficients[name], v.den) for name in names], RatFn(constant, v.den)


def common_roots(polys, k):
    """Le radici reali comuni ai polinomi non nulli di polys, nella lettera k, se sono
    tutte razionali: sono le radici del loro massimo comun divisore. Solleva Skip se ce
    n'e una irrazionale. Senza polinomi non nulli non ce ne sono da guardare."""
    coefficients = None
    for p in polys:
        c = u_trim(Poly.of(p).coefficients(k))
        if c:
            coefficients = c if coefficients is None else u_trim(u_gcd(coefficients, c))
    if coefficients is None or len(coefficients) < 2:
        return []
    roots, rest = rational_roots(coefficients)
    rest = u_trim(rest)
    if len(rest) > 1:
        # Senza radici multiple: diviso per il massimo comun divisore con la derivata.
        derivative = u_trim([c * i for i, c in enumerate(rest)][1:])
        common = u_gcd(rest, derivative) if len(derivative) > 0 else [Fr(1)]
        if isolate(u_quo(rest, common) if len(common) > 1 else rest):
            raise Skip("an irrational special value of the parameter")
    return roots


def minors_by_order(m):
    """Per ogni ordine, i numeratori dei determinanti di tutte le sottomatrici quadrate di
    m di quell'ordine: il rango di m scende sotto l'ordine solo dove si annullano tutti."""
    rows, cols = shape(m)
    for size in range(1, min(rows, cols) + 1):
        yield [RatFn.of(det(tuple(tuple(m[i][j] for j in cs) for i in rs))).num
               for rs in itertools.combinations(range(rows), size) for cs in itertools.combinations(range(cols), size)]


class ParameterFact:
    """Un fatto con il parametro k, letto dai token [start, end) di un enunciato: i suoi
    coefficienti come frazioni di polinomi in k, e la sua verita in un valore di k."""

    def __init__(self, k, start, end, kind, relation):
        self.k, self.start, self.end, self.kind, self.relation = k, start, end, kind, relation
        self.matrices = {}  # nome -> matrice di frazioni (tupla di righe); None se non c'e

    def groups(self):
        """Liste di polinomi in k: la verita del fatto puo cambiare solo dove i polinomi
        non nulli di una lista si annullano tutti insieme. Un denominatore da solo (li un
        coefficiente non e definito); i minori di uno stesso ordine (li scende un rango);
        le componenti di un residuo (li il punto scritto, o un vettore, diventa una
        soluzione o smette di esserlo)."""
        found = [[RatFn.of(x).den] for m in self.matrices.values() for row in m or () for x in row]
        if self.kind == "rank":
            found += list(minors_by_order(self.matrices["m"]))
        else:
            a, b, p, u = (self.matrices[name] for name in ("a", "b", "p", "u"))
            found += list(minors_by_order(a)) + list(minors_by_order(tuple(row + c for row, c in zip(a, b))))
            if p is not None:
                found.append([RatFn.of(row[0]).num for row in add(mat_mul(a, p), b, -1)])
            if u is not None:
                found.append([RatFn.of(x).num for row in mat_mul(a, u) for x in row])
                found += list(minors_by_order(u))
        return found

    def at(self, c):
        """La verita del fatto con k = c. Un coefficiente non definito: il fatto non vale."""
        try:
            numbers = {name: m and tuple(tuple(RatFn.of(x).at(self.k, c) for x in row) for row in m) for name, m in self.matrices.items()}
        except Undefined:
            return False
        if self.kind == "rank":
            return holds(Fr(rank(numbers["m"])), self.number, self.relation)
        a, b, p, u = (numbers[name] for name in ("a", "b", "p", "u"))
        n, rhs = len(a[0]), [row[0] for row in b]
        if p is None:
            equal = not system_solvable(a, rhs)
        else:
            equal = mat_mul(a, p) == b and (u is None or all(x == 0 for row in mat_mul(a, u) for x in row)) and \
                (rank(u) if u is not None else 0) == n - rank(a)
        return equal if self.relation == "=" else not equal

    def table(self):
        """(valori speciali razionali in ordine, verita in ognuno, verita negli altri)."""
        special = sorted({r for group in self.groups() for r in common_roots(group, self.k)})
        generic = next(Fr(v) for v in itertools.count(1) if Fr(v) not in special)
        return special, [self.at(c) for c in special], self.at(generic)

    def condition(self):
        """La condizione su k che dice dove il fatto vale, in LaTeX."""
        special, truths, generic = self.table()
        k = self.k
        pieces = ["%s = %s" % (k, tex_number(c)) for c, t in zip(special, truths) if t]
        if generic:
            away = ["%s \\neq %s" % (k, tex_number(c)) for c, t in zip(special, truths) if not t]
            pieces.append("\\left( %s \\right)" % " \\land ".join(away) if away else "0 = 0")
        return "\\left( %s \\right)" % (" \\lor ".join(pieces) if pieces else "0 = 1")


def parameter_letter(text):
    """La lettera che un quantificatore, o un insieme \\{k \\in \\mathbb{R} \\mid ...\\},
    mette in tutto \\mathbb{R}: None se non c'e o se sono piu di una."""
    found = set(re.findall(r"(?:\\forall|\\exists|\\\{)\s*([a-z])\s*\\in\s*\\mathbb\s*\{\s*R\s*\}(?!\s*\^)", text))
    return found.pop() if len(found) == 1 else None


def parameter_facts(reader, k):
    """I fatti di un enunciato con il parametro k, dal token corrente di reader in poi:
    una lista di ParameterFact. Un fatto senza relazione ha relation uguale a None."""
    facts, i = [], reader.pos
    while i < len(reader.tok):
        t, saved = reader.tok[i], list(reader.tok)
        reader.pos = i
        try:
            if t == "\\{" and reader.tok[i - 1] != "=":
                n, system = reader.solutions()
                relation = {"=": "=", "\\neq": "neq", "\\ne": "neq"}.get(reader.peek())
                other = None
                if relation is not None:
                    reader.take()
                    other = reader.written()
                fact = ParameterFact(k, i, reader.pos, "system", relation)
                a, b = system_matrices(system, rvalue, affine_fractions)
                if len(a[0]) != n:
                    raise Undefined("unknowns")
                fact.matrices = {"a": a, "b": tuple((c,) for c in b), "p": None, "u": None}
                if other is not None and other[0] == "set":
                    point, directions = written_parts(other, n, rvalue, affine_fractions)
                    fact.matrices["p"] = tuple((c,) for c in point)
                    fact.matrices["u"] = tuple(directions) if other[2] else None
                facts.append(fact)
                i = reader.pos
                continue
            if t in ("\\operatorname", "\\mathrm", "\\text"):
                tree = reader.primary()
                if tree[0] == "rank":
                    relation = RELATIONS.get(reader.peek())
                    number = None
                    if relation is not None:
                        reader.take()
                        number = settled(value(reader.sum()))
                        if is_matrix(number) or isinstance(number, Poly):
                            raise Unreadable("a rank is compared with a number")
                    fact = ParameterFact(k, i, reader.pos, "rank", relation)
                    fact.number = number
                    fact.matrices = {"m": rvalue(tree[1])}
                    if not is_matrix(fact.matrices["m"]):
                        raise Undefined("the rank of a number")
                    facts.append(fact)
                    i = reader.pos
                    continue
        except Unreadable:
            pass
        reader.tok = saved
        i += 1
    return facts


def parameter_read(text):
    """Legge un enunciato con un parametro: ritorna (lettore, lettera, fatti, dove
    comincia l'enunciato dopo le definizioni)."""
    k = parameter_letter(text)
    if k is None:
        raise Unreadable("one real parameter is expected")
    reader = SystemReader(text)
    if len(reader.tok) > 1 and len(reader.tok[0]) == 1 and reader.tok[0].isupper() and reader.tok[1] == "=":
        reader.definitions()
    start = reader.pos
    return reader, k, parameter_facts(reader, k), start


def parameter_judge(text):
    """Che cosa dice l'oracolo di un enunciato con un parametro: cio che dice
    letter_judge() dell'enunciato con ogni fatto sostituito dalla sua condizione sul
    parametro, oppure ("cases", (prefisso, testo della richiesta, lettera)) per una
    richiesta da discutere per casi."""
    determinants[0] = 0
    try:
        reader, k, facts, start = parameter_read(text)
        tokens = reader.tok
        asked = [f for f in facts if f.relation is None]
        if asked:
            if len(facts) == 1 and facts[0].end == len(tokens) and tokens[start:facts[0].start] == ["\\forall", k, "\\in", "\\mathbb", "{", "R", "}"]:
                return ("cases", (" ".join(tokens[:start]), " ".join(tokens[facts[0].start:]), k))
            raise Unreadable("a fact without a relation inside a statement")
        pieces, at = [], 0
        for fact in facts:
            pieces += tokens[at:fact.start] + [fact.condition()]
            at = fact.end
        return letter_judge(" ".join(pieces + tokens[at:]))
    except (Unreadable, IndexError) as problem:
        return ("unreadable", str(problem))
    except Undefined as problem:
        return ("undefined", str(problem))
    except (Skip, TooBig) as problem:
        return ("skip", str(problem))


def cases_right(request, answer):
    """Dice se i casi answer, scritti dal programma o attesi da un test, rispondono alla
    richiesta request (cio che parameter_judge() ritorna con "cases"): ogni caso "valore &
    condizione" deve dare un enunciato vero, "per ogni k, se vale la condizione la
    richiesta vale quel valore", e le condizioni devono coprire tutti i valori di k."""
    prefix, asked, k = request
    text = answer.strip()
    if text.startswith("\\begin{cases}"):
        rows = [row.rsplit(" & ", 1) for row in text[len("\\begin{cases}"):-len("\\end{cases}")].split(" \\\\ ")]
        if any(len(row) != 2 for row in rows):
            raise Unreadable("a case without its condition")
    else:
        rows = [[text, "0 = 0"]]
    every = "\\forall %s \\in \\mathbb{R}\\ \\left( %s \\right)" % (k, " \\lor ".join("\\left( %s \\right)" % c.strip() for _, c in rows))
    if letter_judge(every) != ("true",):
        return False
    for value, condition in rows:
        statement = "%s \\forall %s \\in \\mathbb{R}\\ \\left( \\left( %s \\right) \\implies %s = %s \\right)" % (prefix, k, condition.strip(), asked, value.strip())
        verdict = parameter_judge(statement)
        if verdict[0] in ("skip", "unreadable", "undefined"):
            raise Skip("a case that the oracle does not decide: %s" % (verdict[1:],))
        if verdict != ("true",):
            return False
    return True


PARAMETER_SAMPLES = [Fr(v) for v in range(-6, 7)] + [Fr(1, 2), Fr(-1, 2), Fr(3, 2), Fr(-5, 3), Fr(7, 4), Fr(11), Fr(-13)]


def sampled_cert_judge(cert):
    """Un certificato di una discussione, con o senza lettere, controllato sui numeri: con
    le lettere, in ogni valore di PARAMETER_SAMPLES che non annulla il polinomio della voce
    where la lettera diventa quel numero e il certificato deve essere uno giusto tra quelli
    sui numeri (cert_judge(), system_cert_judge()). Non e una dimostrazione: e il conto in
    molti valori. Ritorna None per i certificati di un altro tipo."""
    lines = [l for l in cert.strip().split("\n") if l.strip()]
    if not lines or lines[0].split() != ["certificate", "matrices"]:
        return None
    judge = system_cert_judge if "(solutions" in cert else cert_judge
    if lines[1].split()[:1] != ["letters"]:
        return judge(cert)
    letters = lines[1].split()[1:]
    where = "1"
    body = lines[2:]
    if body and body[0].split()[:1] == ["where"]:
        where, body = body[0].split(None, 1)[1], body[1:]
    if len(letters) != 1:
        raise Unreadable("letters")

    def with_number(text, v):
        number = str(v.numerator) if v.denominator == 1 else "(/ %d %d)" % (v.numerator, v.denominator)
        return join([number if t == letters[0] else t for t in re.findall(r"\(|\)|[^\s()]+", text)])

    checked = 0
    for v in PARAMETER_SAMPLES:
        try:
            if cert_value(sexp(with_number(where, v)), None) == 0:
                continue
        except Undefined:
            return False
        checked += 1
        if not judge("certificate matrices\n" + "\n".join(with_number(l, v) for l in body) + "\n"):
            return False
    return checked > 0


class ParameterGenerator:
    """Matrici e sistemi con il parametro k in una riga o in una colonna sola: i minori
    sono allora di primo grado in k, e i valori speciali razionali. Ogni tanto il parametro
    sta in due posti qualsiasi, e i valori speciali possono essere irrazionali: li
    l'oracolo non decide, e il programma non deve decidere a sproposito."""

    def __init__(self, rnd):
        self.rnd = rnd

    def matrix(self, m, n):
        rnd = self.rnd
        where = rnd.random()
        line = rnd.randrange(m) if where < 0.45 else rnd.randrange(n)
        spots = {(rnd.randrange(m), rnd.randrange(n)) for _ in range(2)}
        rows = []
        for i in range(m):
            row = []
            for j in range(n):
                inside = (i == line) if where < 0.45 else (j == line) if where < 0.9 else (i, j) in spots
                row.append((Fr(rnd.randint(-3, 3)), Fr(rnd.choice([1, 1, -1, 2, -2, 3])) if inside and rnd.random() < 0.7 else Fr(0)))
            rows.append(row)
        return rows

    @staticmethod
    def entry(c):
        return s_sum([(c[1], "k"), (c[0], "")])

    def rank_text(self):
        rnd = self.rnd
        m, n = rnd.randint(2, 3), rnd.randint(2, 4)
        rows = self.matrix(m, n)
        return "\\operatorname{rank} \\begin{pmatrix} %s \\end{pmatrix}" % " \\\\ ".join(" & ".join(self.entry(c) for c in row) for row in rows), min(m, n)

    def system_text(self):
        rnd = self.rnd
        m, n = rnd.randint(2, 3), rnd.randint(2, 3)
        rows = self.matrix(m, n + 1)
        names = list("xyz"[:n])
        equations = []
        for row in rows:
            left = ""
            for c, x in zip(row, names):
                if c == (0, 0):
                    continue
                if c[0] and c[1]:
                    piece, negative = "(%s)%s" % (self.entry(c), x), False
                else:
                    negative, piece = s_term(c[1] or c[0], ("k" if c[1] else "") + x)
                left += ("-" if negative else "") + piece if not left else (" - " if negative else " + ") + piece
            equations.append("%s = %s" % (left or "0", self.entry(row[n])))
        return "\\{(%s) \\in \\mathbb{R}^{%d} \\mid %s\\}" % (", ".join(names), n, " \\land ".join(equations))

    def condition(self, fact_text, wrong):
        """Una condizione su k da mettere accanto a un fatto: quella vera, calcolata
        dall'oracolo, oppure la stessa con un valore spostato, tolto o aggiunto."""
        rnd = self.rnd
        reader, k, facts, _ = parameter_read("\\forall k \\in \\mathbb{R}\\ \\left( %s \\right)" % fact_text)
        special, truths, generic = facts[0].table()
        if wrong:
            if special and rnd.random() < 0.6:
                i = rnd.randrange(len(special))
                if rnd.random() < 0.5:
                    special[i] += rnd.choice([1, -1])
                else:
                    truths[i] = not truths[i]
            else:
                special, truths = special + [Fr(rnd.randint(-5, 5)) + Fr(1, 7)], truths + [not generic]
        pieces = ["k = %s" % tex_number(c) for c, t in zip(special, truths) if t]
        if generic:
            away = ["k \\neq %s" % tex_number(c) for c, t in zip(special, truths) if not t]
            pieces.append("\\left( %s \\right)" % " \\land ".join(away) if away else "0 = 0")
        return " \\lor ".join(pieces) if pieces else "0 = 1"

    def statement(self):
        rnd = self.rnd
        kind = rnd.random()
        try:
            if kind < 0.5:
                fact, top = self.rank_text()
                r = rnd.randint(0, top)
                relation = rnd.choice(["=", "=", "<", "\\ge", "\\neq"])
                shape_ = rnd.random()
                if shape_ < 0.25:
                    return "\\forall k \\in \\mathbb{R}\\ %s" % fact
                if shape_ < 0.45:
                    return "\\{k \\in \\mathbb{R} \\mid %s %s %d\\}" % (fact, relation, r)
                if shape_ < 0.85:
                    wrong = rnd.random() < 0.5
                    return "\\forall k \\in \\mathbb{R}\\ \\left( %s %s %d \\iff \\left( %s \\right) \\right)" % (fact, relation, r, self.condition("%s %s %d" % (fact, relation, r), wrong))
                return "%s k \\in \\mathbb{R}\\ \\left( %s %s %d \\right)" % (rnd.choice(["\\forall", "\\exists"]), fact, relation, r)
            fact = self.system_text()
            relation = rnd.choice(["=", "\\neq"])
            shape_ = rnd.random()
            if shape_ < 0.35:
                return "\\forall k \\in \\mathbb{R}\\ %s" % fact
            if shape_ < 0.5:
                return "\\{k \\in \\mathbb{R} \\mid %s %s \\emptyset\\}" % (fact, relation)
            if shape_ < 0.85:
                wrong = rnd.random() < 0.5
                return "\\forall k \\in \\mathbb{R}\\ \\left( %s %s \\emptyset \\iff \\left( %s \\right) \\right)" % (fact, relation, self.condition("%s %s \\emptyset" % (fact, relation), wrong))
            return "%s k \\in \\mathbb{R}\\ \\left( %s %s \\emptyset \\right)" % (rnd.choice(["\\forall", "\\exists"]), fact, relation)
        except (Skip, TooBig, Unreadable, Undefined):
            return None


def perturbed(text, rnd):
    """Il testo con un numero intero cambiato di uno: una risposta sbagliata di poco."""
    places = [m for m in re.finditer(r"(?<![\w{}^_])\d+", text)]
    if not places:
        return text
    m = rnd.choice(places)
    return text[:m.start()] + str(int(m.group()) + 1) + text[m.end():]


def parameter_run(binary, checker, cases, work, name, rnd, count):
    """Da gli enunciati cases, coppie (testo, verdetto dell'oracolo), al programma, e
    confronta; poi fa ricontrollare i certificati al verificatore e ne altera count.
    Ritorna (errori, sospetti, mancati, conteggi, risposte)."""
    tex_path, cert_path = os.path.join(work, name + ".tex"), os.path.join(work, name + ".cert")
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
    tally, answers = {}, []
    if len(lines) != len(cases):
        print("BUG: the program answered %d of %d statements (exit %d)\n%s" % (len(lines), len(cases), run.returncode, run.stderr[-2000:]))
        bugs += 1
    for number, (line, (text, expected)) in enumerate(zip(lines, cases), 1):
        answer = line.split(". ", 1)[1]
        outcome = (answer.split()[0] + (" " + answer.split()[1] if answer.startswith("NON") else "")).rstrip(":")
        decided = outcome in ("DIMOSTRATO", "FALSO", "RISOLTO")
        written = answer.split(": ", 1)[1] if outcome == "RISOLTO" and ": " in answer else None
        answers.append(written)
        kind = expected[0]
        if decided and "certificato verificato" not in answer and "ha i suoi certificati, verificati" not in answer:
            print("BUG: statement %d is decided and no certificate is mentioned: %s\n   %s" % (number, answer[:160], text))
            bugs += 1
        if kind in ("request", "cases"):
            if outcome == "RISOLTO":
                try:
                    right = cases_right(expected[1], written) if kind == "cases" else same_solutions(expected[1], written)
                    if not right:
                        print("BUG: statement %d is solved wrongly: %s\n   %s" % (number, answer[:400], text))
                        bugs += 1
                except (Skip, TooBig):
                    kind += ", not checked"
                except (Unreadable, IndexError, ValueError, Undefined) as problem:
                    print("SUSPECT: statement %d, cannot read the answer (%s): %s\n   %s" % (number, problem, answer[-300:], text))
                    suspects += 1
            elif decided:
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
        elif kind == "undefined" and decided:
            print("BUG: statement %d has no meaning but the program says %s\n   %s" % (number, answer[:160], text))
            bugs += 1
        elif kind == "unreadable" and decided:
            print("SUSPECT: statement %d is not a statement for the reader of this script (%s) but the program says %s\n   %s" % (number, expected[1], answer[:160], text))
            suspects += 1
        tally[(kind, outcome)] = tally.get((kind, outcome), 0) + 1
    if "ATTENZIONE" in run.stdout:
        print("SUSPECT: the program discarded a step of its own proof")
        suspects += 1

    # I certificati: quelli del programma, poi quelli alterati.
    accepted = wrong = mutated = 0
    if checker and os.path.exists(cert_path):
        certs = [c.lstrip("\n") + "end\n" for c in open(cert_path, encoding="utf-8").read().split("end\n") if c.strip()]
        run = subprocess.run([checker, cert_path], capture_output=True, text=True)
        replies = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
        if len(replies) != len(certs):
            print("BUG: the checker answered %d of %d certificates" % (len(replies), len(certs)))
            bugs += 1
        for cert, reply in zip(certs, replies):
            try:
                good = sampled_cert_judge(cert)
            except (Unreadable, IndexError, TooBig) as problem:
                print("SUSPECT: the oracle cannot read a certificate of the program (%s)\n%s" % (problem, cert))
                suspects += 1
                continue
            if "ACCETTATO" not in reply or good is False:
                print("BUG: a certificate of the program is %s by the checker and %s for the oracle\n%s" % (reply.split(". ", 1)[1][:80], good, cert))
                bugs += 1
        ours = [c for c in certs if c.startswith("certificate matrices")]
        pool = [line for cert in ours for line in cert.split("\n") if line.startswith("witness ")]
        variants = []
        for _ in range(20 * count):
            if not ours or len(variants) == count:
                break
            variant = mutate_system(rnd.choice(ours), rnd, pool)
            if variant not in certs:
                variants.append(variant)
        mutated_path = os.path.join(work, name + "_mutated.cert")
        with open(mutated_path, "w", encoding="utf-8") as out:
            out.write("\n".join(variants))
        run = subprocess.run([checker, mutated_path], capture_output=True, text=True)
        replies = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
        mutated = len(variants)
        if len(replies) != len(variants):
            print("SUSPECT: the checker answered %d of %d altered certificates (some no longer begin as one)" % (len(replies), len(variants)))
            suspects += 1
        else:
            for variant, reply in zip(variants, replies):
                if "ACCETTATO" not in reply:
                    continue
                accepted += 1
                try:
                    good = sampled_cert_judge(variant)
                except TooBig:
                    continue
                except (Unreadable, IndexError, RecursionError) as problem:
                    good = False
                    print("the oracle cannot read it: %s" % problem)
                if good is False:
                    print("BUG: the checker accepted a certificate that does not say the truth\n%s" % variant)
                    bugs += 1
                    wrong += 1
    print("   %s: %d statements; certificates altered: %d, accepted by the checker: %d, of which wrong: %d" % (name, len(cases), mutated, accepted, wrong))
    return bugs, suspects, misses, tally, answers


def parameter_mode(binary, seed, count, work, checker):
    """Il fuzz delle discussioni con un parametro: vedi "CON UN PARAMETRO: LA DISCUSSIONE"
    in testa al file. In due tempi: prima ranghi e sistemi; poi, dai casi che il programma
    ha scritto per i sistemi, gli enunciati "se vale la condizione, le soluzioni sono
    queste", giusti e sbagliati di poco."""
    rnd = random.Random(seed)
    gen = ParameterGenerator(rnd)
    os.makedirs(work, exist_ok=True)
    cases = []
    while len(cases) < count:
        text = gen.statement()
        if text is None or len(text) > 3000:
            continue
        cases.append((text, parameter_judge(text)))
    totals = list(parameter_run(binary, checker, cases, work, "parameter_%d" % seed, rnd, count))

    # Il secondo tempo.
    derived = []
    for (text, expected), answer in zip(cases, totals[4]):
        if expected[0] != "cases" or answer is None or "\\{" not in text:
            continue
        prefix, asked, k = expected[1]
        rows = [row.rsplit(" & ", 1) for row in answer[len("\\begin{cases}"):-len("\\end{cases}")].split(" \\\\ ")] if answer.startswith("\\begin{cases}") else [[answer, "0 = 0"]]
        for value, condition in rows:
            how = rnd.random()
            if how < 0.4:
                value = perturbed(value, rnd)
            elif how < 0.6 and " \\land " in condition:
                condition = condition.split(" \\land ", 1)[1]
            elif how < 0.7:
                condition = "0 = 0"
            statement = "\\forall %s \\in \\mathbb{R}\\ \\left( \\left( %s \\right) \\implies %s = %s \\right)" % (k, condition.strip(), asked, value.strip())
            derived.append((statement, parameter_judge(statement)))
    derived = derived[:count]
    if derived:
        second = parameter_run(binary, checker, derived, work, "parameter_%d_cases" % seed, rnd, len(derived))
        for i in range(3):
            totals[i] += second[i]
        for key, v in second[3].items():
            totals[3][key] = totals[3].get(key, 0) + v
    print("Summary (seed %d): %d statements with a parameter, %d on the cases written by the program" % (seed, len(cases), len(derived)))
    for key in sorted(totals[3]):
        print("   %-22s %-16s %d" % (key[0], key[1], totals[3][key]))
    print("BUG %d, SUSPECT %d, MISS %d" % (totals[0], totals[1], totals[2]))
    sys.exit(1 if totals[0] else 0)


# ------------------------------------------- con un parametro: enunciati strani
#
# Il generatore qui sopra scrive enunciati come quelli dei libri. Un errore del motore si
# e visto solo con un insieme scritto strano: \{(\frac{1}{k - k}, 2)\}, con un divisore
# che e zero per ogni valore del parametro, su cui il programma non finiva. Il modo
# parameter-odd scrive enunciati cosi: insiemi scritti e matrici con il parametro in
# pezzi presi da una lista che comprende divisori nulli, frazioni di frazioni, potenze
# negative, radici, gradi alti, numeri al limite del calcolo esatto; e un terzo degli
# enunciati rovinato. Prima guarda che il programma finisca; poi confronta ogni verdetto
# con l'oracolo del parametro. Molti di questi enunciati sono fuori da cio che il
# programma tratta, apposta: i MISS qui sono attesi, e dicono solo quanti.
#
# Nell'ipotesi di un "se ... allora" vanno solo i pezzi che sono polinomi (ODD_PLAIN).
# Li c'e un confronto della tabella dei segni, non un fatto, e dove un suo membro non e
# definito il programma segue la convenzione di quella tabella (il confronto non vale):
# il lettore di questo script non la conosce, e direbbe "senza senso" o "non letto".
ODD_PIECES = ["k", "k - 1", "k + 1", "2k", "k^{2} - 1", "k^{2}", "k - k", "0", "1", "3", "k^{2} - k^{2}", "(k - 1)(k + 1)", "k^{3} - 2k", "\\frac{k}{2}",
              "\\frac{1}{k}", "\\sqrt{k}", "k^{-1}", "(k - k)^{-1}", "(k + 1)^{-2}", "\\frac{1}{k - k}", "k^{40}", "k^{2} + 1", "-k", "\\frac{k - 1}{k + 1}",
              "\\frac{\\frac{1}{k}}{k - 1}", "999999999989k", "\\frac{k}{999999999989}", "\\frac{1000000007}{999999999989} k^{2}", "k + 4611686018427387903",
              "\\frac{k}{1000000007} + \\frac{1}{998244353}"]
ODD_PLAIN = [piece for piece in ODD_PIECES if "\\" not in piece and "^{-" not in piece]
ODD_SYSTEMS = ["\\{(x, y) \\in \\mathbb{R}^{2} \\mid x + ky = 1 \\land kx + y = 1\\}",
               "\\{(x, y) \\in \\mathbb{R}^{2} \\mid x + y = k \\land x - y = 0\\}",
               "\\{(x, y, z) \\in \\mathbb{R}^{3} \\mid kx + 3y + 5z = 0 \\land x + 6y + 10z = 0\\}",
               "\\{(x, y, z) \\in \\mathbb{R}^{3} \\mid kx + y + z = 1 \\land x + ky + z = 1 \\land x + y + kz = 1\\}",
               "\\{(x, y) \\in \\mathbb{R}^{2} \\mid 0 = 0\\}",
               "\\{(x, y) \\in \\mathbb{R}^{2} \\mid (k - k)x + y = 1\\}",
               "\\{(x, y) \\in \\mathbb{R}^{2} \\mid k^{2}x + ky = k \\land kx + y = 1\\}"]


class OddGenerator:
    """Enunciati con il parametro k scritti in modo strano: un sistema di ODD_SYSTEMS
    confrontato con un insieme scritto che ha pezzi di ODD_PIECES nelle componenti, o un
    rango di una matrice fatta di quei pezzi; ogni tanto un enunciato di
    ParameterGenerator; un terzo rovinato."""

    def __init__(self, rnd):
        self.rnd = rnd

    def component(self):
        rnd, c = self.rnd, self.rnd.random()
        if c < 0.5:
            return rnd.choice(ODD_PIECES)
        if c < 0.8:
            return "\\frac{%s}{%s}" % (rnd.choice(ODD_PIECES), rnd.choice(ODD_PIECES))
        return "%s %s %s" % (rnd.choice(ODD_PIECES), rnd.choice("+-"), rnd.choice(ODD_PIECES))

    def written(self, n):
        c = self.rnd.random()
        if c < 0.15:
            return "\\emptyset"
        point = "(%s)" % ", ".join(self.component() for _ in range(n))
        if c < 0.55:
            return "\\{%s\\}" % point
        if c < 0.8:
            return "\\{%s + t (%s) \\mid t \\in \\mathbb{R}\\}" % (point, ", ".join(self.component() for _ in range(n)))
        return "\\{t (%s) + s (%s) \\mid t, s \\in \\mathbb{R}\\}" % (", ".join(self.component() for _ in range(n)), ", ".join(self.component() for _ in range(n)))

    def statement(self):
        rnd, c = self.rnd, self.rnd.random()
        if c < 0.3:
            text = ParameterGenerator(rnd).statement()
        elif c < 0.8:
            system = rnd.choice(ODD_SYSTEMS)
            fact = "%s %s %s" % (system, rnd.choice(["=", "=", "\\neq"]), self.written(3 if "^{3}" in system else 2))
            shape_ = rnd.random()
            if shape_ < 0.4:
                text = "\\forall k \\in \\mathbb{R}\\ \\left( %s \\right)" % fact
            elif shape_ < 0.6:
                text = "\\{k \\in \\mathbb{R} \\mid %s\\}" % fact
            elif shape_ < 0.8:
                text = "\\forall k \\in \\mathbb{R}\\ \\left( %s \\neq %s \\implies %s \\right)" % (rnd.choice(ODD_PLAIN), rnd.choice(["0", "1", "k"]), fact)
            else:
                text = "\\exists k \\in \\mathbb{R}\\ \\left( %s \\right)" % fact
        else:
            m = "\\begin{pmatrix} %s & %s \\\\ %s & %s \\end{pmatrix}" % tuple(self.component() for _ in range(4))
            text = rnd.choice(["\\forall k \\in \\mathbb{R}\\ \\operatorname{rank} %s", "\\{k \\in \\mathbb{R} \\mid \\operatorname{rank} %s = 1\\}",
                               "\\forall k \\in \\mathbb{R}\\ \\left( \\operatorname{rank} %s \\ge 1 \\right)", "\\forall k \\in \\mathbb{R}\\ \\{x \\in \\mathbb{R}^{2} \\mid %s x = 0\\}"]) % m
        if text is not None and rnd.random() < 0.3:
            text = damage(text, rnd)
        return text


def endless(binary, texts, work, name):
    """Gli enunciati di texts su cui il programma non finisce entro un minuto, o e fermato
    da un segnale. Li prova tutti insieme; solo se qualcosa non va, uno alla volta."""
    def stuck(some, seconds):
        path = os.path.join(work, name + "_ends.tex")
        with open(path, "w", encoding="utf-8") as out:
            out.write("\\begin{document}\n\\begin{enumerate}\n")
            for text in some:
                out.write("\\item $\\displaystyle %s$\n" % text)
            out.write("\\end{enumerate}\n\\end{document}\n")
        try:
            return subprocess.run([binary, path], capture_output=True, text=True, timeout=seconds).returncode not in (0, 1)
        except subprocess.TimeoutExpired:
            return True

    if not stuck(texts, 60 + 5 * len(texts)):
        return []
    return [text for text in texts if stuck([text], 60)]


def parameter_odd_mode(binary, seed, count, work, checker):
    """Il fuzz degli enunciati strani con un parametro: vedi il commento qui sopra."""
    rnd = random.Random(seed)
    gen = OddGenerator(rnd)
    os.makedirs(work, exist_ok=True)
    cases = []
    while len(cases) < count:
        text = gen.statement()
        if text is None or len(text) > 3000 or "\n" in text:
            continue
        cases.append((text, parameter_judge(text)))
    never = endless(binary, [text for text, _ in cases], work, "odd_%d" % seed)
    for text in never:
        print("BUG: the program does not end, or is stopped by a signal\n   %s" % text)
    if never:
        print("BUG %d, SUSPECT 0, MISS 0" % len(never))
        sys.exit(1)
    bugs, suspects, misses, tally, _ = parameter_run(binary, checker, cases, work, "odd_%d" % seed, rnd, count)
    print("Summary (seed %d): %d odd statements with a parameter" % (seed, len(cases)))
    for key in sorted(tally):
        print("   %-22s %-16s %d" % (key[0], key[1], tally[key]))
    print("BUG %d, SUSPECT %d, MISS %d" % (bugs, suspects, misses))
    sys.exit(1 if bugs else 0)


def parameter_data_mode(paths):
    """Il terzo conto sui file di dati con un parametro: che cosa ne dice l'oracolo."""
    words = {"true": "VERO", "false": "FALSO", "undefined": "NON DEFINITO", "unreadable": "NON LETTO", "request": "RICHIESTA", "cases": "RICHIESTA PER CASI",
             "skip": "NON DECISO DALL'ORACOLO"}
    for path in paths:
        print(path)
        counts = {}
        for number, text in enumerate(items_of(path), 1):
            verdict = parameter_judge(text)
            counts[words[verdict[0]]] = counts.get(words[verdict[0]], 0) + 1
            print("%4d. %s%s" % (number, words[verdict[0]], ": " + str(verdict[1]) if verdict[0] in ("undefined", "unreadable", "skip") else ""))
        print("   " + ", ".join("%s %d" % item for item in sorted(counts.items())))


def parameter_answers_mode(tex_path, c_path):
    """Le risposte attese delle richieste con un parametro, scritte nel test in C in
    expected_answers[] (anche su due righe): l'oracolo dice se ognuna risponde alla sua
    richiesta."""
    source = open(c_path, encoding="utf-8").read()
    block = re.search(r"expected_answers\[\] = \{\n(.*?)\n\};", source, re.S)
    answers = []
    for entry in re.split(r",\s*\n", block.group(1)) if block else []:
        pieces = re.findall(r'"((?:[^"\\]|\\.)*)"', entry)
        if pieces:
            answers.append("".join(pieces).replace("\\\\", "\\"))
    requests = items_of(tex_path)
    print("%s: %d requests, %d expected answers" % (tex_path, len(requests), len(answers)))
    wrong = 0
    for number, (text, answer) in enumerate(zip(requests, answers), 1):
        verdict = parameter_judge(text)
        try:
            if verdict[0] == "cases":
                outcome = "CONFERMATA" if cases_right(verdict[1], answer) else "SBAGLIATA"
            elif verdict[0] == "request":
                outcome = "CONFERMATA" if same_solutions(verdict[1], answer) else "SBAGLIATA"
            else:
                outcome = "NON E UNA RICHIESTA PER L'ORACOLO (%s)" % (verdict[1] if len(verdict) > 1 else verdict[0])
        except (Unreadable, IndexError, Undefined, Skip, TooBig) as problem:
            outcome = "NON DECISA DALL'ORACOLO (%s)" % problem
        wrong += outcome != "CONFERMATA"
        print("%4d. %s: %s" % (number, outcome, answer[:150]))
    wrong += abs(len(requests) - len(answers))
    print("%d not confirmed" % wrong)
    sys.exit(1 if wrong else 0)


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
    if len(sys.argv) >= 3 and sys.argv[1] == "parameter-data":
        parameter_data_mode(sys.argv[2:])
        return
    if len(sys.argv) == 4 and sys.argv[1] == "parameter-answers":
        parameter_answers_mode(sys.argv[2], sys.argv[3])
        return
    if len(sys.argv) in (6, 7) and sys.argv[1] == "parameter":
        parameter_mode(sys.argv[2], int(sys.argv[3]), int(sys.argv[4]), sys.argv[5], sys.argv[6] if len(sys.argv) == 7 else None)
        return
    if len(sys.argv) in (6, 7) and sys.argv[1] == "parameter-odd":
        parameter_odd_mode(sys.argv[2], int(sys.argv[3]), int(sys.argv[4]), sys.argv[5], sys.argv[6] if len(sys.argv) == 7 else None)
        return
    if len(sys.argv) >= 3 and sys.argv[1] == "systems-data":
        systems_data_mode(sys.argv[2:])
        return
    if len(sys.argv) == 4 and sys.argv[1] == "systems-answers":
        systems_answers_mode(sys.argv[2], sys.argv[3])
        return
    if len(sys.argv) in (6, 7) and sys.argv[1] == "systems":
        systems_mode(sys.argv[2], int(sys.argv[3]), int(sys.argv[4]), sys.argv[5], sys.argv[6] if len(sys.argv) == 7 else None)
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
