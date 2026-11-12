r"""Goniometria: controllo dei dati degli esercizi e fuzzing differenziale.

Uso:
    python3 -I fuzz_trigonometry.py check-data <exercises.jsonl>
    python3 -I fuzz_trigonometry.py fuzz <eseguibile> <seme> <quanti> <cartella di lavoro>

Equazioni, disequazioni e sistemi goniometrici in una variabile reale, con il
risultato scritto come sui libri: famiglie x = a + 2k\pi e intervalli
a + 2k\pi \le x \le b + 2k\pi, con k intero legato da \exists k \in \mathbb{Z}.
Lo script non chiede mai al programma che cosa è vero.

L'ORACOLO NUMERICO (serve a entrambi i modi)

Un lettore del LaTeX scritto qui, che segue la grammatica nei commenti dei
capitoli 9 e 17 di parser.c e non il suo codice, e un calcolatore con errore
garantito: ogni numero è un intervallo [lo, hi] di decimali a 100 cifre che
contiene il valore vero, perché ogni operazione arrotonda lo verso il basso e
hi verso l'alto. Pi greco viene dalla formula di Machin, seno e coseno dalla
serie di Taylor dopo aver ridotto l'angolo, con il resto della serie aggiunto
all'intervallo. Un segno è quindi certo ogni volta che l'intervallo non
contiene lo zero. Quando lo contiene ed è più stretto di 10^-45, il numero è
dichiarato zero: è l'unica decisione non dimostrata, ed è quella che fa dire
"i due membri coincidono a meno di 10^-45".

Dove può, il calcolatore resta esatto: un multiplo razionale di pi greco più
un razionale è tenuto come coppia di frazioni (a + b pi), e lì il segno si
decide senza arrotondare (pi è irrazionale: a + b pi = 0 solo se a = b = 0).
Così "x = \frac{\pi}{6} + 2k\pi" in un punto della griglia è deciso
esattamente, e la tangente non ha valore proprio dove il coseno è zero.

Convenzione dei libri, e del programma: un confronto è falso dove uno dei suoi
membri non ha valore (tangente con coseno nullo, divisore nullo, radice di un
numero negativo).

"Esiste k intero" si decide risolvendo: ogni confronto del risultato è lineare
in k, quindi la sua verità cambia in un solo valore reale di k; basta provare
gli interi vicini a quei valori (il perché è scritto in integer_quantifier).

CHECK-DATA

Per ogni esercizio del file confronta la condizione con il risultato stampato
su tre periodi: in tutti i multipli di pi/720, in ogni angolo scritto nel
risultato e nei punti a distanza 10^-3, 10^-9 e 10^-20 da esso, e negli zeri
delle funzioni da cui dipende la verità (differenza dei due membri di ogni
confronto, divisori, coseno sotto una tangente, seno sotto una cotangente,
radicandi): quelli che cadono tra due punti di prova vengono cercati per
bisezione dove la funzione cambia segno. Ricontrolla il periodo dichiarato,
conferma che ogni variante falsa è falsa nel suo testimone e, per gli esercizi
con un parametro, conta le soluzioni per alcuni valori del parametro.

Limite dichiarato: uno zero di molteplicità pari che non cade su un punto di
prova non fa cambiare segno, e questo controllo non lo vede. Negli esercizi di
un libro gli zeri sono angoli notevoli, cioè punti della griglia.

Righe stampate: CONFIRMED (il risultato del libro regge in tutti i punti),
NOT CONFIRMED (non regge: i punti sono elencati; se il file lo dichiara già
con "book_confirmed": false, viene controllato il risultato corretto), DATA
ERROR (il file contraddice se stesso: una variante falsa che non è falsa nel
testimone, un periodo sbagliato), SUSPECT (un segno non deciso), NOTE.

FUZZ

Genera enunciati la cui verità è nota per costruzione e li dà al programma.
Segnala BUG se il programma dimostra un enunciato falso, dichiara falso un
enunciato vero, o scrive soluzioni diverse; NOTE se non decide; SUSPECT se le
soluzioni scritte non si riescono a leggere, o se i due conti di questo
script (vedi sotto) non sono d'accordo tra loro.

Perché ogni generatore conosce la verità:

(i)   Elementari: sin, cos o tan di ax + b confrontato con il valore della
      stessa funzione in un angolo notevole t. Le soluzioni sono lette sulla
      circonferenza goniometrica: sin u = sin t in u = t e u = pi - t (più
      2k pi), sin u > sin t sull'arco tra i due; cos u = cos t in u = +-t,
      cos u > cos t sull'arco da -t a t; tan u = tan t in u = t (più k pi),
      tan u > tan t da t al punto pi/2, dove la tangente non ha valore. Da
      qui si sa, per ogni multiplo razionale di pi greco, se la funzione
      supera, uguaglia o sta sotto la costante, con conti tra frazioni.
(ii)  Prodotti di tali fattori confrontati con zero: il segno del prodotto è
      il prodotto dei segni, e il confronto è falso dove un fattore non ha
      valore. Per "= 0" è l'unione delle famiglie, tolti i punti in cui una
      tangente non esiste.
(iii) Sistemi di due disequazioni elementari con 0 <= x <= 2 pi, a = 1 e
      b multiplo di pi/12. Qui la verità si calcola a forza bruta: in ogni
      multiplo di pi/12 tra 0 e 2 pi e nel punto medio di ogni arco tra due
      multipli consecutivi. È un conto esatto. Ogni funzione in gioco è
      continua e monotona su ciascun arco (i suoi massimi, minimi e punti
      senza valore sono multipli di pi/12), agli estremi dell'arco assume
      due valori consecutivi dell'elenco finito dei valori negli angoli
      notevoli, e la costante è un valore dello stesso elenco: non può stare
      strettamente tra i due, quindi funzione meno costante non si annulla
      dentro l'arco e ha lì un segno solo, quello del punto medio. Agli
      estremi i numeri confrontati stanno in un elenco finito di valori che
      distano più di 10^-2 l'uno dall'altro: un intervallo largo 10^-90 che
      contiene lo zero vuol dire uguaglianza. Ci sono anche gli stessi
      sistemi senza la restrizione, con il risultato periodico.
(iv)  Varianti false di tutti: un pezzo tolto, 2k pi al posto di k pi o
      viceversa, un angolo spostato su un angolo notevole vicino, < al posto
      di <=, un punto senza tangente incluso. La variante non è falsa per
      ipotesi: il suo insieme viene confrontato con quello vero in ogni
      estremo e in ogni arco, con frazioni esatte.

Ogni condizione generata è controllata due volte prima di usarla: l'insieme
costruito sulla circonferenza goniometrica deve coincidere, in ogni estremo e
nel punto medio di ogni arco, con ciò che il calcolatore a intervalli trova
leggendo il testo LaTeX dell'enunciato. Sono due conti indipendenti; se non
coincidono il caso è scartato con una riga SUSPECT, ed è un errore di questo
script.
"""
import json
import os
import random
import re
import subprocess
import sys
from decimal import Context, Decimal, ROUND_CEILING, ROUND_FLOOR, ROUND_HALF_EVEN
from fractions import Fraction
from math import factorial, isqrt


class Undefined(Exception):
    """Un'espressione non ha valore nel punto."""


class Undecided(Exception):
    """La precisione non basta a decidere un segno: il punto va guardato a mano."""


class Unsupported(Exception):
    """Il testo usa qualcosa che questo lettore non conosce."""


# ---------------------------------------------------------------- numeri con errore garantito
ZERO, ONE, TWO = Decimal(0), Decimal(1), Decimal(2)
TINY = ONE.scaleb(-45)
HALF = Fraction(1, 2)
DIGITS, MORE_DIGITS = 100, 300


class Num:
    """Un numero reale noto con errore garantito.

    Se ex = (a, b) il numero è esattamente a + b pi, con a e b frazioni, e lo e hi si calcolano
    solo quando servono (digits ricorda con quante cifre, per rifarli se la precisione cambia).
    Altrimenti ex è None e il numero sta tra i decimali lo e hi."""

    __slots__ = ("ex", "lo", "hi", "digits")

    def __init__(self, ex=None, lo=None, hi=None):
        self.ex, self.lo, self.hi, self.digits = ex, lo, hi, None


def exact(a, b=0):
    """Il numero a + b pi."""
    return Num((Fraction(a), Fraction(b)))


def frac_bounds(q):
    """Due decimali che racchiudono la frazione q."""
    n, d = Decimal(q.numerator), Decimal(q.denominator)
    return DOWN.divide(n, d), UP.divide(n, d)


def mul_bounds(xl, xh, yl, yh):
    """Il prodotto di due intervalli: il più piccolo e il più grande dei quattro prodotti degli estremi."""
    if xl >= 0 and yl >= 0:
        return DOWN.multiply(xl, yl), UP.multiply(xh, yh)
    pairs = ((xl, yl), (xl, yh), (xh, yl), (xh, yh))
    return min(DOWN.multiply(p, q) for p, q in pairs), max(UP.multiply(p, q) for p, q in pairs)


def machin_pi(digits):
    """Due decimali che racchiudono pi greco, dalla formula di Machin
    pi = 16 arctan(1/5) - 4 arctan(1/239), con arctan(1/n) = 1/n - 1/(3 n^3) + 1/(5 n^5) - ...
    La serie è a segni alterni con termini decrescenti: una somma che finisce con un termine
    negativo sta sotto il valore vero, e aggiungendo il termine dopo si passa sopra. I conti
    sono tra frazioni, quindi esatti."""
    def arctan_inverse(n, terms):
        total, power = Fraction(0), Fraction(1, n)
        for j in range(terms):  # terms è pari: l'ultimo termine sommato è negativo
            total += power / (2 * j + 1) * (-1 if j % 2 else 1)
            power /= n * n
        return total, total + power / (2 * terms + 1)

    low5, high5 = arctan_inverse(5, 2 * (digits * 2 // 5 + 4))        # 5^-(2j+1) scende di 1.4 cifre per termine
    low239, high239 = arctan_inverse(239, 2 * (digits // 9 + 4))      # 239^-(2j+1) di 4.7
    return frac_bounds(16 * low5 - 4 * high239)[0], frac_bounds(16 * high5 - 4 * low239)[1]


SETTINGS = {}
NUMERIC_ZEROS = [0]  # quante volte un numero è stato dichiarato zero perché più piccolo di 10^-45


def set_precision(digits):
    """Sceglie con quante cifre si fanno i conti: i due contesti (per difetto e per eccesso), pi
    greco, quanti termini delle serie servono perché il resto stia sotto l'ultima cifra, e le
    tabelle dei valori già calcolati, che valgono per una precisione sola.

    Si lavora a 100 cifre; at_point() passa a 300 nei rari punti in cui a 100 un segno non si
    decide, per esempio quando due termini enormi si cancellano accanto a un punto senza valore."""
    global PREC, DOWN, UP, PI_LO, PI_HI, PI_LO_FR, PI_HI_FR, TERMS, SIN_REST, COS_REST, ATAN_TERMS, ATAN_REST, SIN_PI, ATAN, TRIG
    if digits not in SETTINGS:
        PREC, DOWN, UP = digits, Context(prec=digits, rounding=ROUND_FLOOR), Context(prec=digits, rounding=ROUND_CEILING)
        pi_lo, pi_hi = machin_pi(digits)
        assert str(pi_lo).startswith("3.14159265358979323846264338327950288419716939937510") and pi_hi - pi_lo < ONE.scaleb(3 - digits)
        terms = next(t for t in range(1, 10 ** 4) if factorial(2 * t + 2) > 10 ** (digits + 8))
        atan_terms = int((digits + 5) / 2.6) + 1  # dopo i dimezzamenti v < 0.05 = 10^-1.3: v^(2n+3) < 10^-(digits+5)
        SETTINGS[digits] = (digits, DOWN, UP, pi_lo, pi_hi, Fraction(pi_lo), Fraction(pi_hi), terms,
                            UP.divide(ONE, Decimal(factorial(2 * terms + 3))), UP.divide(ONE, Decimal(factorial(2 * terms + 2))),
                            atan_terms, ONE.scaleb(-digits - 5), {}, {}, {})
    PREC, DOWN, UP, PI_LO, PI_HI, PI_LO_FR, PI_HI_FR, TERMS, SIN_REST, COS_REST, ATAN_TERMS, ATAN_REST, SIN_PI, ATAN, TRIG = SETTINGS[digits]


def bounds(v):
    """I due decimali che racchiudono v."""
    if v.ex is not None and v.digits != PREC:
        a, b = v.ex
        lo, hi = frac_bounds(a)
        if b:
            plo, phi = mul_bounds(*frac_bounds(b), PI_LO, PI_HI)
            lo, hi = DOWN.add(lo, plo), UP.add(hi, phi)
        v.lo, v.hi, v.digits = lo, hi, PREC
    return v.lo, v.hi


def sign(v):
    """Il segno di v: -1, 0 o 1. Solleva Undecided se l'intervallo contiene lo zero ed è troppo largo."""
    if v.ex is not None:
        a, b = v.ex
        if b == 0:
            return (a > 0) - (a < 0)
        if a == 0:
            return (b > 0) - (b < 0)
        # a + b pi = b (pi - q) con q = -a/b razionale: non è mai zero, e il segno di pi - q si
        # legge confrontando q con i due razionali che racchiudono pi.
        q = -a / b
        if q < PI_LO_FR:
            return 1 if b > 0 else -1
        if q > PI_HI_FR:
            return -1 if b > 0 else 1
        raise Undecided
    if v.lo > 0:
        return 1
    if v.hi < 0:
        return -1
    if v.lo >= -TINY and v.hi <= TINY:
        NUMERIC_ZEROS[0] += 1
        return 0
    raise Undecided


def add(x, y):
    if x.ex is not None and y.ex is not None:
        return Num((x.ex[0] + y.ex[0], x.ex[1] + y.ex[1]))
    (xl, xh), (yl, yh) = bounds(x), bounds(y)
    return Num(None, DOWN.add(xl, yl), UP.add(xh, yh))


def neg(x):
    if x.ex is not None:
        return Num((-x.ex[0], -x.ex[1]))
    return Num(None, x.hi.copy_negate(), x.lo.copy_negate())


def sub(x, y):
    return add(x, neg(y))


def mul(x, y):
    # (a + b pi)(c + d pi) resta della stessa forma solo se b o d è zero.
    if x.ex is not None and y.ex is not None and (x.ex[1] == 0 or y.ex[1] == 0):
        (a, b), (c, d) = x.ex, y.ex
        return Num((a * c, a * d + b * c))
    return Num(None, *mul_bounds(*bounds(x), *bounds(y)))


def div(x, y):
    """x / y; solleva Undefined se y è zero."""
    if sign(y) == 0:
        raise Undefined
    if x.ex is not None and y.ex is not None and y.ex[1] == 0:
        return Num((x.ex[0] / y.ex[0], x.ex[1] / y.ex[0]))
    yl, yh = bounds(y)
    if yl <= 0 <= yh:
        raise Undecided
    return Num(None, *mul_bounds(*bounds(x), DOWN.divide(ONE, yh), UP.divide(ONE, yl)))


def sqrt(x):
    """La radice quadrata; solleva Undefined se x è negativo."""
    s = sign(x)
    if s < 0:
        raise Undefined
    if s == 0:
        return exact(0)
    if x.ex is not None and x.ex[1] == 0:
        p, q = x.ex[0].numerator, x.ex[0].denominator
        if isqrt(p) ** 2 == p and isqrt(q) ** 2 == q:
            return exact(Fraction(isqrt(p), isqrt(q)))
    lo, hi = bounds(x)
    # La radice di un decimale è arrotondata sempre al più vicino, qualunque sia il contesto:
    # il decimale prima e quello dopo la racchiudono di sicuro.
    return Num(None, max(ZERO, DOWN.next_minus(DOWN.sqrt(lo))), UP.next_plus(UP.sqrt(hi)))


def absolute(x):
    s = sign(x)
    return x if s > 0 else exact(0) if s == 0 else neg(x)


# ---------------------------------------------------------------- seno, coseno, arcotangente
def taylor(lo, hi, cosine):
    """Due decimali che racchiudono sin r (o cos r) per ogni r in [lo, hi], con |r| <= 1.

    sin r = r (1 - r^2/(2 3) (1 - r^2/(4 5) (1 - ...))) e cos r = 1 - r^2/(1 2) (1 - r^2/(3 4) (1 - ...)),
    troncate dopo TERMS fattori. Tutte le derivate di seno e coseno stanno tra -1 e 1, quindi il
    resto di Lagrange dopo il termine di grado n è al più |r|^(n+1)/(n+1)!, cioè al più 1/(n+1)!
    quando |r| <= 1: TERMS è scelto in set_precision() perché sia meno di 10^-(cifre + 8)."""
    if max(lo.copy_abs(), hi.copy_abs()) > 1:
        raise Undecided
    r2 = mul_bounds(lo, hi, lo, hi)
    r2 = (max(r2[0], ZERO), r2[1])  # un quadrato non è negativo
    acc = (ONE, ONE)
    for j in range(TERMS, 0, -1):
        d = Decimal((2 * j - 1) * (2 * j) if cosine else (2 * j) * (2 * j + 1))
        t = mul_bounds(DOWN.divide(r2[0], d), UP.divide(r2[1], d), acc[0], acc[1])
        acc = (DOWN.subtract(ONE, t[1]), UP.subtract(ONE, t[0]))
    if not cosine:
        acc = mul_bounds(lo, hi, acc[0], acc[1])
    rest = COS_REST if cosine else SIN_REST
    return DOWN.subtract(acc[0], rest), UP.add(acc[1], rest)


def sin_pi(r):
    """sin(r pi) per una frazione r.

    Il teorema di Niven dice che i soli valori razionali sono 0, +-1/2 e +-1: quelli sono resi
    esatti, così "2 sin x = 1" in pi/6 e "cos x = 0" in pi/2 si decidono senza soglie."""
    r %= 2
    v = SIN_PI.get(r)
    if v is None:
        t, s = (r - 1, -1) if r > 1 else (r, 1)   # sin((1 + t) pi) = -sin(t pi)
        if t > HALF:
            t = 1 - t                              # sin((1 - t) pi) = sin(t pi)
        if t == 0:
            v = exact(0)
        elif t == Fraction(1, 6):
            v = exact(Fraction(s, 2))
        elif t == HALF:
            v = exact(s)
        else:
            # Sotto pi/4 la serie del seno, sopra quella del coseno dell'angolo complementare.
            lo, hi = bounds(exact(0, t if t <= Fraction(1, 4) else HALF - t))
            w = Num(None, *taylor(lo, hi, t > Fraction(1, 4)))
            v = w if s > 0 else neg(w)
        SIN_PI[r] = v
    return v


def sin_cos(x):
    """La coppia (sin x, cos x)."""
    if x.ex is not None and x.ex[0] == 0:
        return sin_pi(x.ex[1]), sin_pi(x.ex[1] + HALF)

    # Lo stesso angolo compare più volte in una condizione (\sin x, \cos x, \tan x): la coppia si
    # calcola una volta. La tabella è svuotata quando cresce troppo.
    lo, hi = bounds(x)
    key = x.ex if x.ex is not None else (lo, hi)
    pair = TRIG.get(key)
    if pair is None:
        # Riduzione: x = n pi/2 + r con |r| <= pi/4 o poco più; (lo + hi)/pi è il punto medio diviso pi/2.
        n = int(DOWN.divide(DOWN.add(lo, hi), PI_LO).to_integral_value(rounding=ROUND_HALF_EVEN))
        rlo, rhi = bounds(sub(x, exact(0, Fraction(n, 2))))
        s, c = Num(None, *taylor(rlo, rhi, False)), Num(None, *taylor(rlo, rhi, True))
        if len(TRIG) > 50000:
            TRIG.clear()
        pair = TRIG[key] = ((s, c), (c, neg(s)), (neg(s), neg(c)), (neg(c), s))[n % 4]
    return pair


def tangent(x, cotangent=False):
    """tan x, o cot x; div solleva Undefined dove il divisore (coseno o seno) è zero."""
    s, c = sin_cos(x)
    return div(c, s) if cotangent else div(s, c)


HALVINGS = 4


def atan_bounds(t):
    """Due decimali che racchiudono arctan t, per un decimale t.

    Per t > 1 si usa arctan t = pi/2 - arctan(1/t). Per 0 <= t <= 1 si dimezza l'angolo quattro
    volte con arctan v = 2 arctan(v / (1 + sqrt(1 + v^2))), che porta v sotto tan(pi/64) < 0.05, e
    poi si somma v - v^3/3 + v^5/5 - ... : a segni alterni con termini decrescenti, il resto dopo
    il termine di grado 2n+1 è al più v^(2n+3); n = ATAN_TERMS lo porta sotto 10^-(cifre + 5)."""
    if t < 0:
        lo, hi = atan_bounds(t.copy_negate())
        return hi.copy_negate(), lo.copy_negate()
    if t not in ATAN:
        if t > 1:
            small, large = atan_bounds(DOWN.divide(ONE, t)), atan_bounds(UP.divide(ONE, t))
            ATAN[t] = DOWN.subtract(DOWN.divide(PI_LO, TWO), large[1]), UP.subtract(UP.divide(PI_HI, TWO), small[0])
        else:
            lo = hi = t
            for _ in range(HALVINGS):
                # v / (1 + sqrt(1 + v^2)) cresce con v: per difetto in lo, per eccesso in hi.
                lo = DOWN.divide(lo, UP.add(ONE, UP.next_plus(UP.sqrt(UP.add(ONE, UP.multiply(lo, lo))))))
                hi = UP.divide(hi, DOWN.add(ONE, DOWN.next_minus(DOWN.sqrt(DOWN.add(ONE, DOWN.multiply(hi, hi))))))
            if hi > Decimal("0.05"):
                raise Undecided
            v2 = (DOWN.multiply(lo, lo), UP.multiply(hi, hi))
            acc = (ZERO, ZERO)
            for j in range(ATAN_TERMS, -1, -1):
                t2 = mul_bounds(v2[0], v2[1], acc[0], acc[1])
                d = Decimal(2 * j + 1)
                acc = (DOWN.subtract(DOWN.divide(ONE, d), t2[1]), UP.subtract(UP.divide(ONE, d), t2[0]))
            acc = mul_bounds(lo, hi, acc[0], acc[1])
            scale = Decimal(2 ** HALVINGS)
            ATAN[t] = DOWN.multiply(DOWN.subtract(acc[0], ATAN_REST), scale), UP.multiply(UP.add(acc[1], ATAN_REST), scale)
    return ATAN[t]


def arctan(x):
    if x.ex is not None and x.ex == (0, 0):
        return exact(0)
    lo, hi = bounds(x)
    return Num(None, atan_bounds(lo)[0], atan_bounds(hi)[1])  # arctan cresce


def at_point(function, p):
    """function(p). Se a 100 cifre un segno non si decide e p è noto esattamente, rifà il conto a 300."""
    try:
        return function(p)
    except Undecided:
        if p.ex is None:
            raise
    set_precision(MORE_DIGITS)
    try:
        return function(Num(p.ex))
    finally:
        set_precision(DIGITS)


set_precision(DIGITS)


def arccos(x):
    """arccos x = 2 arctan sqrt((1 - x)/(1 + x)) per -1 < x <= 1 (formula di bisezione), pi per x = -1."""
    above, below = sub(exact(1), x), add(exact(1), x)
    if sign(above) < 0 or sign(below) < 0:
        raise Undefined
    if sign(below) == 0:
        return exact(0, 1)
    return mul(exact(2), arctan(sqrt(div(above, below))))


# ---------------------------------------------------------------- lettore del LaTeX
# La grammatica è quella scritta nei commenti dei capitoli 9 e 17 di parser.c, limitata a ciò che
# serve qui. In più legge \arccos, che il programma oggi non conosce.
IGNORED = ("displaystyle", "textstyle", "quad", "qquad", "limits")
FUNCTIONS = ("sin", "cos", "tan", "cot", "arctan", "arccos")
RELATIONS = {"=": "=", "neq": "!=", "ne": "!=", "<": "<", ">": ">", "le": "<=", "leq": "<=", "ge": ">=", "geq": ">="}
COMPARE = {"=": lambda s: s == 0, "!=": lambda s: s != 0, "<": lambda s: s < 0, "<=": lambda s: s <= 0,
           ">": lambda s: s > 0, ">=": lambda s: s >= 0}


def tokenize(text):
    """La lista dei token (tipo, testo). \\left( e ( sono lo stesso token; \\left| e \\right| restano distinti."""
    tokens, i = [], 0
    while i < len(text):
        c = text[i]
        if c.isspace():
            i += 1
        elif c.isdigit():
            j = i
            while j < len(text) and text[j].isdigit():
                j += 1
            tokens.append(("num", text[i:j]))
            i = j
        elif c.isalpha():
            tokens.append(("let", c))
            i += 1
        elif c == "\\":
            j = i + 1
            if j < len(text) and not text[j].isalpha():
                if text[j] in ",;!: ":
                    i = j + 1
                    continue
                if text[j] in "{}\\":
                    tokens.append(("esc", text[j]))
                    i = j + 1
                    continue
                raise Unsupported("command \\%s" % text[j])
            while j < len(text) and text[j].isalpha():
                j += 1
            name, i = text[i + 1:j], j
            if name in ("left", "right"):
                while i < len(text) and text[i].isspace():
                    i += 1
                d = text[i:i + 1]
                if d == "|":
                    tokens.append(("abs(" if name == "left" else "abs)", d))
                elif name == "left" and d in ("(", "["):
                    tokens.append(("(", d))
                elif name == "right" and d in (")", "]"):
                    tokens.append((")", d))
                else:
                    raise Unsupported("delimiter after \\%s" % name)
                i += 1
            elif name not in IGNORED:
                tokens.append(("cmd", name))
        else:
            tokens.append(("(", c) if c == "(" else (")", c) if c == ")" else ("ch", c))
            i += 1
    tokens.append(("end", ""))
    return tokens


class Reader:
    """Discesa ricorsiva: un metodo per regola della grammatica.

    Espressioni: ("num", n), ("var", lettera), ("pi",), ("neg", a), ("+", a, b), ("-", a, b),
    ("*", a, b), ("/", a, b), ("^", a, n), ("abs", a), ("sqrt", a), ("fn", nome, a).
    Condizioni: ("cmp", relazione, a, b), ("not", p), ("and", p, q), ("or", p, q),
    ("implies", p, q), ("iff", p, q), ("forall" | "exists", lettera, insieme, p),
    ("solve", lettera, insieme, p)."""

    def __init__(self, text):
        self.t, self.i = tokenize(text), 0

    def at(self, kind, value=None):
        k, v = self.t[self.i]
        return k == kind and (value is None or v == value)

    def command(self, *names):
        k, v = self.t[self.i]
        return k == "cmd" and v in names

    def take(self):
        self.i += 1
        return self.t[self.i - 1]

    def expect(self, kind, value=None):
        if not self.at(kind, value):
            raise Unsupported("expected %s, found %s" % (value or kind, self.t[self.i][1] or "the end"))
        return self.take()

    # group := '{' expr '}' | un solo token
    def group(self):
        if self.at("ch", "{"):
            self.take()
            e = self.expr()
            self.expect("ch", "}")
            return e
        # Senza graffe TeX prende una cifra sola: x^12 è x^1 seguito da 2.
        if self.at("num") and len(self.t[self.i][1]) > 1:
            digits = self.t[self.i][1]
            self.t[self.i:self.i + 1] = [("num", digits[0]), ("num", digits[1:])]
        return self.primary()

    # funzione ['^' group] argomento, con argomento := '(' expr ')' | factor
    def function(self):
        name, exponent = self.take()[1], None
        if self.at("ch", "^"):
            self.take()
            exponent = self.group()
        if self.at("("):
            self.take()
            argument = self.expr()
            self.expect(")")
        else:
            argument = self.factor()
        call = ("fn", name, argument)
        return call if exponent is None else ("^", call, exponent)

    def primary(self):
        kind, value = self.t[self.i]
        if kind == "num":
            self.take()
            return ("num", int(value))
        if kind == "let":
            self.take()
            if self.at("ch", "_"):
                raise Unsupported("subscript")
            return ("var", value)
        if kind == "(":
            self.take()
            e = self.expr()
            self.expect(")")
            return e
        if kind == "abs(":
            self.take()
            e = self.expr()
            self.expect("abs)")
            return ("abs", e)
        if self.command("frac", "dfrac", "tfrac"):
            self.take()
            above = self.group()
            return ("/", above, self.group())
        if self.command("sqrt"):
            self.take()
            if self.at("ch", "["):
                raise Unsupported("root with an index")
            return ("sqrt", self.group())
        if self.command(*FUNCTIONS):
            return self.function()
        if self.command("pi"):
            self.take()
            return ("pi",)
        raise Unsupported("unexpected %s" % (("\\" if kind == "cmd" else "") + value or "end of text"))

    # factor := primary { '^' group }
    def factor(self):
        e = self.primary()
        while self.at("ch", "^"):
            self.take()
            e = ("^", e, self.group())
        if self.at("ch", "!"):
            raise Unsupported("factorial")
        return e

    def starts_factor(self):
        kind, value = self.t[self.i]
        return kind in ("num", "let", "(", "abs(") or (kind == "cmd" and value in FUNCTIONS + ("frac", "dfrac", "tfrac", "sqrt", "pi"))

    # term := factor { [\cdot | '/'] factor }: due fattori accostati si moltiplicano
    def term(self):
        e = self.factor()
        while True:
            if self.command("cdot", "times"):
                self.take()
                e = ("*", e, self.factor())
            elif self.at("ch", "/"):
                self.take()
                e = ("/", e, self.factor())
            elif self.starts_factor():
                e = ("*", e, self.factor())
            else:
                return e

    # expr := ['-' | '+'] term { ('+' | '-') term }
    def expr(self):
        if self.at("ch", "-"):
            self.take()
            e = ("neg", self.term())
        else:
            if self.at("ch", "+"):
                self.take()
            e = self.term()
        while self.at("ch", "+") or self.at("ch", "-"):
            op = self.take()[1]
            e = (op, e, self.term())
        return e

    def relation(self):
        kind, value = self.t[self.i]
        if (kind == "ch" and value in ("=", "<", ">")) or (kind == "cmd" and value in RELATIONS):
            self.take()
            return RELATIONS[value]
        return None

    def numbers(self):
        self.expect("cmd", "mathbb")
        self.expect("ch", "{")
        letter = self.expect("let")[1]
        self.expect("ch", "}")
        if letter not in ("Z", "R") or self.at("ch", "^") or self.at("ch", "_") or self.command("setminus"):
            raise Unsupported("number set")
        return letter

    def cases_word(self, word):
        self.expect("cmd", word)
        self.expect("ch", "{")
        for letter in "cases":
            self.expect("let", letter)
        self.expect("ch", "}")

    def unary(self):
        # Negazione.
        if self.command("lnot", "neg"):
            self.take()
            return ("not", self.unary())

        # Quantificatore: \forall a, b è \forall a \forall b.
        if self.command("forall", "exists"):
            kind = self.take()[1]
            names = [self.expect("let")[1]]
            while self.at("ch", ","):
                self.take()
                names.append(self.expect("let")[1])
            self.expect("cmd", "in")
            where = self.numbers()
            body = self.unary()
            for name in reversed(names):
                body = (kind, name, where, body)
            return body

        # Sistema: la congiunzione delle righe.
        if self.command("begin"):
            self.cases_word("begin")
            node = self.formula()
            while self.at("esc", "\\"):
                self.take()
                if self.command("end"):
                    break
                node = ("and", node, self.formula())
            self.cases_word("end")
            return node

        # Confronto, anche in catena: a < b \le c dice a < b e b \le c. Se non riesce si torna
        # indietro, e una parentesi deve aprire una condizione.
        start = self.i
        try:
            left, chain = self.expr(), None
            while True:
                relation = self.relation()
                if relation is None:
                    break
                right = self.expr()
                node = ("cmp", relation, left, right)
                chain = node if chain is None else ("and", chain, node)
                left = right
            if chain is not None:
                return chain
            problem = Unsupported("a relation is expected after an expression")
        except Unsupported as error:
            problem = error
        self.i = start
        if self.at("("):
            self.take()
            inner = self.formula()
            self.expect(")")
            return inner
        raise problem

    def conjunction(self):
        node = self.unary()
        while self.command("land", "wedge"):
            self.take()
            node = ("and", node, self.unary())
        return node

    def disjunction(self):
        node = self.conjunction()
        while self.command("lor", "vee"):
            self.take()
            node = ("or", node, self.conjunction())
        return node

    def implication(self):
        node = self.disjunction()
        if self.command("implies", "Rightarrow", "Longrightarrow"):
            self.take()
            node = ("implies", node, self.implication())
        return node

    def formula(self):
        node = self.implication()
        while self.command("iff", "Leftrightarrow", "Longleftrightarrow"):
            self.take()
            node = ("iff", node, self.implication())
        return node

    # statement := formula | \{ lettera \in insieme (\mid | ':') formula \}
    def statement(self):
        if self.at("esc", "{"):
            self.take()
            name = self.expect("let")[1]
            self.expect("cmd", "in")
            where = self.numbers()
            if not (self.command("mid") or self.at("ch", ":")):
                raise Unsupported("\\mid is missing")
            self.take()
            node = ("solve", name, where, self.formula())
            self.expect("esc", "}")
        else:
            node = self.formula()
        self.expect("end")
        return node


def read_statement(text):
    return Reader(text).statement()


def read_expression(text):
    reader = Reader(text)
    e = reader.expr()
    reader.expect("end")
    return e


# ---------------------------------------------------------------- valore e verità
PI_NUM = exact(0, 1)


def value(e, env):
    """Il valore dell'espressione e, con le lettere prese da env. Solleva Undefined dove non ha valore."""
    op = e[0]
    if op == "num":
        return exact(e[1])
    if op == "var":
        if e[1] not in env:
            raise Unsupported("the letter %s has no value" % e[1])
        return env[e[1]]
    if op == "pi":
        return PI_NUM
    if op == "neg":
        return neg(value(e[1], env))
    if op == "abs":
        return absolute(value(e[1], env))
    if op == "sqrt":
        return sqrt(value(e[1], env))
    if op == "fn":
        x, name = value(e[2], env), e[1]
        if name == "sin":
            return sin_cos(x)[0]
        if name == "cos":
            return sin_cos(x)[1]
        if name in ("tan", "cot"):
            return tangent(x, name == "cot")
        return arctan(x) if name == "arctan" else arccos(x)
    if op == "^":
        n = value(e[2], env)
        if n.ex is None or n.ex[1] != 0 or n.ex[0].denominator != 1 or not 0 <= n.ex[0] <= 64:
            raise Unsupported("the exponent is not a small natural number")
        base, out = value(e[1], env), exact(1)
        for _ in range(int(n.ex[0])):
            out = mul(out, base)
        return out
    a, b = value(e[1], env), value(e[2], env)
    return add(a, b) if op == "+" else sub(a, b) if op == "-" else mul(a, b) if op == "*" else div(a, b)


def atoms(f):
    """I confronti che compaiono nella condizione f."""
    if f[0] == "cmp":
        return [f]
    return [a for part in f[1:] if isinstance(part, tuple) for a in atoms(part)]


def mentions(e, name):
    """Vero se la lettera name compare nell'albero e."""
    return e[:2] == ("var", name) or any(mentions(part, name) for part in e[1:] if isinstance(part, tuple))


def subterms(e):
    """L'espressione e e tutte le espressioni di cui è fatta."""
    yield e
    for part in e[1:]:
        if isinstance(part, tuple):
            yield from subterms(part)


ONLY_LETTER, REMEMBERED, QUANTIFIED_ATOMS = {}, {}, {}


def side_value(e, env, cache):
    """Il valore di un membro di un confronto. Sotto un quantificatore sugli interi (cache è la
    coppia lettera, valore) i membri che contengono solo la lettera quantificata, come
    \\frac{\\pi}{6} + 2k\\pi, vengono chiesti per gli stessi pochi k in migliaia di punti: si
    ricordano. Le tabelle tengono vivo l'albero, così il suo id resta suo."""
    if cache is None:
        return value(e, env)
    only = ONLY_LETTER.get((id(e), cache[0]))
    if only is None:
        only = ONLY_LETTER[(id(e), cache[0])] = (all(t[0] != "var" or t[1] == cache[0] for t in subterms(e)), e)
    if not only[0]:
        return value(e, env)
    v = REMEMBERED.get((id(e), cache))
    if v is None:
        v = REMEMBERED[(id(e), cache)] = value(e, env)
    return v


def integer_quantifier(f, env):
    """Decide "esiste k intero" (o "per ogni k intero") risolvendo, senza provare k a tappeto.

    Ogni confronto del corpo che contiene k ha la forma D(k) ? 0 con D(k) = D(0) + k (D(1) - D(0)),
    lineare in k (lo si verifica in k = 2). La sua verità, al variare di k tra i reali, cambia solo
    nella radice r = -D(0) / (D(1) - D(0)). Le radici di tutti i confronti dividono la retta in
    punti e intervalli aperti su ciascuno dei quali il corpo è sempre vero o sempre falso. Basta
    quindi un intero per ogni pezzo che ne contiene: ogni radice intera, l'intero subito sotto e
    quello subito sopra ogni radice. Stanno tutti tra m - 1 e m + 2, dove m è la parte intera
    dell'estremo inferiore dell'intervallo che racchiude r."""
    kind, name, _, body = f
    candidates = set()
    inner = [dict(env, **{name: exact(k)}) for k in (0, 1, 2)]
    try:
        for atom in QUANTIFIED_ATOMS.setdefault(id(f), (f, [a for a in atoms(body) if mentions(a, name)]))[1]:
            d0, d1, d2 = (sub(side_value(atom[2], inner[k], (name, k)), side_value(atom[3], inner[k], (name, k))) for k in (0, 1, 2))
            slope = sub(d1, d0)
            if sign(sub(sub(d2, d1), slope)) != 0:
                raise Unsupported("a comparison is not linear in %s" % name)
            if sign(slope) != 0:
                m = int(bounds(div(neg(d0), slope))[0].to_integral_value(rounding=ROUND_FLOOR))
                candidates.update((m - 1, m, m + 1, m + 2))
    except Undefined:
        raise Unsupported("a side has no value under a quantifier") from None
    results = (holds(body, dict(env, **{name: exact(k)}), (name, k)) for k in sorted(candidates or {0}))
    return any(results) if kind == "exists" else all(results)


def holds(f, env, cache=None):
    """La verità della condizione f. Un confronto è falso dove uno dei due membri non ha valore.
    cache serve solo a side_value()."""
    op = f[0]
    if op == "cmp":
        try:
            return COMPARE[f[1]](sign(sub(side_value(f[2], env, cache), side_value(f[3], env, cache))))
        except Undefined:
            return False
    if op == "not":
        return not holds(f[1], env, cache)
    if op == "and":
        return holds(f[1], env, cache) and holds(f[2], env, cache)
    if op == "or":
        return holds(f[1], env, cache) or holds(f[2], env, cache)
    if op == "implies":
        return not holds(f[1], env, cache) or holds(f[2], env, cache)
    if op == "iff":
        return holds(f[1], env, cache) == holds(f[2], env, cache)
    if op in ("forall", "exists") and f[2] == "Z":
        return integer_quantifier(f, env)
    raise Unsupported("cannot evaluate %s here" % op)


FALSE = ("cmp", "=", ("num", 0), ("num", 1))
TRUE = ("cmp", "=", ("num", 0), ("num", 0))


# ---------------------------------------------------------------- scrittura in LaTeX
def frac_tex(q):
    if q.denominator == 1:
        return str(q.numerator)
    return "%s\\frac{%d}{%d}" % ("-" if q < 0 else "", abs(q.numerator), q.denominator)


def angle_tex(q):
    """Il multiplo q di pi greco come lo scrive un libro: \\frac{\\pi}{6}, \\frac{5}{6}\\pi, -\\frac{\\pi}{4}, 2\\pi, 0."""
    if q == 0:
        return "0"
    n, d, minus = abs(q.numerator), q.denominator, "-" if q < 0 else ""
    if d == 1:
        return "%s%s\\pi" % (minus, "" if n == 1 else n)
    return "%s\\frac{\\pi}{%d}" % (minus, d) if n == 1 else "%s\\frac{%d}{%d}\\pi" % (minus, n, d)


def period_tex(p):
    """Il periodo p pi moltiplicato per k: 2k\\pi, k\\pi, \\frac{k\\pi}{2}, \\frac{2k\\pi}{3}."""
    n = "" if p.numerator == 1 else str(p.numerator)
    return "%sk\\pi" % n if p.denominator == 1 else "\\frac{%sk\\pi}{%d}" % (n, p.denominator)


def family_tex(angle, period):
    """angle + periodo k; angle è una frazione (multiplo di pi greco) o un testo LaTeX già pronto."""
    text = angle if isinstance(angle, str) else angle_tex(angle)
    if period is None:
        return text
    return period_tex(period) if text == "0" else "%s + %s" % (text, period_tex(period))


def piece_tex(piece, period):
    """Un pezzo (lo, compreso, hi, compreso): un punto se lo e hi coincidono, altrimenti un intervallo."""
    lo, lo_closed, hi, hi_closed = piece
    if lo == hi:
        return "x = %s" % family_tex(lo, period)
    return "%s %s x %s %s" % (family_tex(lo, period), "\\le" if lo_closed else "<", "\\le" if hi_closed else "<", family_tex(hi, period))


def describe(p):
    """Un punto di prova come testo LaTeX (quando è a + b pi) e in cifre."""
    lo, hi = bounds(p)
    digits = format(DOWN.divide(DOWN.add(lo, hi), TWO), ".30f")
    if p.ex is None:
        return None, digits
    a, b = p.ex
    if a == 0:
        return angle_tex(b), digits
    if b == 0:
        return frac_tex(a), digits
    if abs(a).numerator != 1:
        return None, digits
    return "%s %s \\frac{1}{%d}" % (angle_tex(b), "+" if a > 0 else "-", abs(a).denominator), digits


# ---------------------------------------------------------------- punti di prova
OFFSETS = (Fraction(1, 10 ** 3), Fraction(1, 10 ** 9), Fraction(1, 10 ** 20))
WIDTH = ONE.scaleb(-60)


def near(p):
    """I sei punti accanto a p, a distanza 10^-3, 10^-9 e 10^-20 da una parte e dall'altra."""
    return [q for d in OFFSETS for q in (add(p, exact(d)), sub(p, exact(d)))]


def family_values(e, env, name, low, high):
    """I valori dell'espressione e, lineare nella lettera intera name, che cadono tra low e high."""
    if not mentions(e, name):
        return [value(e, env)]
    e0, e1 = value(e, dict(env, **{name: exact(0)})), value(e, dict(env, **{name: exact(1)}))
    slope = sub(e1, e0)
    if sign(slope) == 0:
        return [e0]
    ends = sorted(int(bounds(div(sub(limit, e0), slope))[0].to_integral_value(rounding=ROUND_FLOOR)) for limit in (low, high))
    found = [value(e, dict(env, **{name: exact(k)})) for k in range(ends[0] - 1, ends[1] + 3)]
    return [v for v in found if sign(sub(v, low)) >= 0 and sign(sub(high, v)) >= 0]


def printed_angles(f, var, low, high, names=()):
    """Gli angoli scritti nella condizione f: ciò che è confrontato con la sola lettera var, per ogni
    valore degli interi legati da un quantificatore che lo fa cadere tra low e high."""
    if f[0] in ("forall", "exists"):
        return printed_angles(f[3], var, low, high, names + (f[1],))
    if f[0] != "cmp":
        return [p for part in f[1:] if isinstance(part, tuple) for p in printed_angles(part, var, low, high, names)]
    out = []
    for side, other in ((f[2], f[3]), (f[3], f[2])):
        if side == ("var", var) and not mentions(other, var):
            used = [n for n in names if mentions(other, n)]
            if len(used) > 1:
                raise Unsupported("an angle with two integer letters")
            out += family_values(other, {}, used[0], low, high) if used else [value(other, {})]
    return out


def critical_functions(f):
    """Le espressioni i cui zeri sono i soli punti in cui la verità della condizione f può cambiare:
    la differenza dei due membri di ogni confronto e, dentro i membri, ogni divisore, il coseno
    dell'argomento di una tangente, il seno dell'argomento di una cotangente, ogni radicando."""
    found = []
    for atom in atoms(f):
        found.append(("-", atom[2], atom[3]))
        for side in atom[2:]:
            for t in subterms(side):
                if t[0] == "/":
                    found.append(t[2])
                elif t[0] == "fn" and t[1] in ("tan", "cot"):
                    found.append(("fn", "cos" if t[1] == "tan" else "sin", t[2]))
                elif t[0] == "sqrt":
                    found.append(t[1])
    return list(dict.fromkeys(found))


def sign_at(g, p, var):
    """Il segno dell'espressione g nel punto p; None dove g non ha valore."""
    try:
        return at_point(lambda q: sign(value(g, {var: q})), p)
    except Undefined:
        return None


def locate(g, a, b, sign_a, var):
    """Tra i decimali a < b, dove g ha segni opposti, stringe per bisezione fino a 10^-60 e ritorna
    un numero che racchiude il punto in cui g cambia segno: uno zero, o un punto senza valore."""
    while UP.subtract(b, a) > WIDTH:
        m = DOWN.divide(DOWN.add(a, b), TWO)
        s = sign_at(g, Num(None, m, m), var)
        if s is None or s == 0:
            return Num(None, m, m)
        a, b = (m, b) if s == sign_a else (a, m)
    return Num(None, a, b)


def sample_points(cond, result, var, low, high, den):
    """I punti in cui confrontare cond e result: vedi CHECK-DATA nel commento in testa.

    low e high sono frazioni (multipli di pi greco). Ritorna i punti in ordine crescente e quanti
    zeri sono stati trovati per bisezione tra due punti."""
    points = {}

    def put(p):
        points.setdefault(p.ex if p.ex is not None else (p.lo, p.hi), p)

    # 1. La griglia, poi gli angoli stampati con i loro vicini.
    for j in range(-(-low * den // 1), high * den // 1 + 1):
        put(exact(0, Fraction(int(j), den)))
    for p in printed_angles(result, var, exact(0, low), exact(0, high)):
        put(p)
        for q in near(p):
            put(q)

    # 2. Gli zeri delle funzioni critiche: accanto a quelli che cadono su un punto si aggiungono i
    #    vicini; quelli che cadono tra due punti si cercano dove il segno cambia.
    ordered = sorted(points.values(), key=lambda p: bounds(p)[0])
    located = 0
    for g in critical_functions(cond):
        signs = [sign_at(g, p, var) for p in ordered]
        for i, s in enumerate(signs):
            if s == 0:
                for q in near(ordered[i]):
                    put(q)
            if i and s and signs[i - 1] and s != signs[i - 1]:
                zero = locate(g, bounds(ordered[i - 1])[1], bounds(ordered[i])[0], signs[i - 1], var)
                located += 1
                put(zero)
                for q in near(zero):
                    put(q)
    return sorted(points.values(), key=lambda p: bounds(p)[0]), located


def compare(cond, result, var, points):
    """I punti in cui cond e result hanno verità diverse, come (punto, verità di cond, verità di
    result); quelli in cui un segno non è stato deciso; e la verità di cond in ogni punto deciso,
    come lista di coppie (punto, verità)."""
    differ, undecided, truths = [], [], []
    for p in points:
        try:
            c, r = at_point(lambda q: (holds(cond, {var: q}), holds(result, {var: q})), p)
        except Undecided:
            undecided.append(p)
            continue
        truths.append((p, c))
        if c != r:
            differ.append((p, c, r))
    return differ, undecided, truths


def simplest(differ):
    """Tra i punti in cui due condizioni differiscono, i più semplici da scrivere per primi: multipli
    di pi greco con denominatore piccolo, poi gli altri esatti, poi quelli noti solo in cifre."""
    def rank(item):
        p = item[0]
        if p.ex is None:
            return (2, 0, abs(float(bounds(p)[0])))
        a, b = p.ex
        return (0 if a == 0 else 1, b.denominator, abs(float(b)))
    return sorted(differ, key=rank)


def point_text(item):
    tex, digits = describe(item[0])
    return "x = %s (%s): condition %s, result %s" % (tex or "?", digits[:24], str(item[1]).lower(), str(item[2]).lower())


# ---------------------------------------------------------------- periodo
PERIOD_PROBES = [exact(0, Fraction(131 * j + 7, 997)) for j in range(12)]


def same_side(side, var, p, q):
    """Vero se l'espressione side ha lo stesso valore in p e in q, o non ha valore in nessuno dei due."""
    values = []
    for point in (p, q):
        try:
            values.append(value(side, {var: point}))
        except Undefined:
            values.append(None)
    if values[0] is None or values[1] is None:
        return values[0] is values[1]
    return sign(sub(values[0], values[1])) == 0


def form_period(cond, var):
    """Il più piccolo T = 4 pi / n tale che ogni membro di ogni confronto di cond, come funzione, abbia
    periodo T; None se nemmeno 4 pi lo è. Ritorna T in unità di pi greco.

    Il controllo è numerico, in dodici punti senza nulla di speciale: due funzioni di questo tipo
    che coincidono lì coincidono ovunque, salvo un caso che non capita per sbaglio. Se una
    funzione continua non costante ha periodo 4 pi, il suo periodo minimo è 4 pi / n per un
    intero n, e il periodo comune a più funzioni è ancora di questa forma: provare n da 64 a 1
    trova quindi il minimo."""
    sides = [side for atom in atoms(cond) for side in atom[2:]]
    for n in range(64, 0, -1):
        shift = exact(0, Fraction(4, n))
        if all(same_side(side, var, p, add(p, shift)) for p in PERIOD_PROBES for side in sides):
            return Fraction(4, n)
    return None


def solution_period(cond, var, period, truths):
    """Il più piccolo T = period / m (m da 1 a 16) tale che la verità di cond in p e in p + T sia la
    stessa in tutti i punti di prova; None se cond è sempre vera o sempre falsa su quei punti.
    truths è la lista di coppie (punto, verità di cond) che compare() ha già calcolato.

    Basta provare i divisori interi di period: se l'insieme delle soluzioni non è vuoto né tutto,
    i suoi periodi sono i multipli di un periodo minimo, e period è uno di essi."""
    if all(t for _, t in truths) or not any(t for _, t in truths):
        return None
    for m in range(16, 1, -1):
        shift = exact(0, period / m)
        try:
            if all(at_point(lambda q: holds(cond, {var: q}), add(p, shift)) == t for p, t in truths):
                return period / m
        except Undecided:
            continue
    return period


# ---------------------------------------------------------------- check-data
class DataError(Exception):
    """Il file degli esercizi contraddice se stesso."""


def fraction_text(q):
    return None if q is None else str(q)


def split_check(text):
    """Dall'enunciato \\forall x \\in \\mathbb{R} (condizione \\iff risultato) ritorna la lettera, la
    condizione e il risultato. \\forall x \\lnot (condizione) ha per risultato il falso, e
    \\forall x (condizione) il vero."""
    f = read_statement(text)
    if f[0] != "forall" or f[2] != "R":
        raise DataError("a check must start with \\forall x \\in \\mathbb{R}")
    body = f[3]
    if body[0] == "iff":
        return f[1], body[1], body[2]
    return (f[1], body[1], FALSE) if body[0] == "not" else (f[1], body, TRUE)


def witness_point(witness):
    """Il punto scritto in un testimone, dopo aver controllato che le cifre dicano lo stesso numero."""
    letter = "x" if "x" in witness else "k"
    p = value(read_expression(witness[letter]), {})
    lo, hi = bounds(p)
    shown = Decimal(witness["decimal"])
    if (DOWN.subtract(lo, shown)).copy_abs() > Decimal("1e-12"):
        raise DataError("the witness %s is not %s" % (witness[letter], witness["decimal"]))
    return p


def check_solving(ex):
    """Un esercizio con condizione e risultato: equazione, disequazione, sistema, intersezioni."""
    lines = []
    solve = read_statement(ex["solve"])
    var, cond, result = split_check(ex["check"])
    if solve[0] != "solve" or solve[1] != var or solve[3] != cond:
        raise DataError("solve and check do not have the same condition")

    # Il risultato atteso deve essere quello dentro il check, senza il quantificatore.
    inner = result[3] if result[0] in ("exists", "forall") else result
    if ex["expected"] == "\\nexists x \\in \\mathbb{R}":
        if result is not FALSE:
            raise DataError("expected says no solutions, check does not")
    elif read_statement(ex["expected"]) != inner:
        raise DataError("expected is not the result written in check")

    # Il periodo dichiarato, da confrontare con quello calcolato sui membri della condizione.
    free = read_statement(ex["condition"])
    restricted = ex.get("restriction") is not None
    if (("and", read_statement(ex["restriction"]), free) if restricted else free) != cond:
        raise DataError("condition and restriction do not make the condition of solve")
    period = form_period(free, var)
    if fraction_text(period) != ex["period"]:
        raise DataError("period is %s, the sides of the condition have period %s" % (ex["period"], fraction_text(period)))

    # I punti: tre periodi, o da -2 pi a 4 pi se il periodo è più corto.
    span = max(period or 2, Fraction(2))
    confirmed = ex.get("book_confirmed", True)
    checks = [("printed", ex["check"])] + ([] if confirmed else [("corrected", ex["corrected_check"])])
    if "check_in_grammar" in ex:
        checks.append(("grammar-only", ex["check_in_grammar"]))
    status = "CONFIRMED"
    for label, text in checks:
        _, c, r = split_check(text)
        if c != cond:
            raise DataError("the %s check has another condition" % label)
        points, located = sample_points(cond, r, var, -span, 2 * span, 720)
        differ, undecided, truths = compare(cond, r, var, points)
        if undecided:
            lines.append("SUSPECT %s: %d points not decided, first x = %s" % (ex["number"], len(undecided), describe(undecided[0])[1][:24]))
        summary = "%d points (%d zeros found between grid points)" % (len(points), located)
        if label == "printed" and confirmed:
            if differ:
                status = "NOT CONFIRMED"
                lines.append("NOT CONFIRMED %s (%s): the printed result differs from the condition in %d of %s; simplest: %s"
                             % (ex["number"], ex["kind"], len(differ), summary, "; ".join(point_text(d) for d in simplest(differ)[:4])))
            else:
                lines.append("CONFIRMED %s (%s): %s" % (ex["number"], ex["kind"], summary))
        elif label == "printed":
            status = "NOT CONFIRMED"
            if not differ:
                raise DataError("book_confirmed is false but the printed result agrees everywhere")
            recorded = witness_point(ex["disagreement"])
            if holds(c, {var: recorded}) == holds(r, {var: recorded}):
                raise DataError("the recorded disagreement is not one")
            lines.append("NOT CONFIRMED %s (%s), as the file says: the printed result differs in %d of %s; simplest: %s"
                         % (ex["number"], ex["kind"], len(differ), summary, "; ".join(point_text(d) for d in simplest(differ)[:4])))
        elif differ:
            raise DataError("the %s result differs too: %s" % (label, point_text(simplest(differ)[0])))
        else:
            lines.append("   %s %s result: agrees in all %s" % (ex["number"], label, summary))

        # Il periodo dell'insieme delle soluzioni, sul risultato che regge.
        if not differ and not restricted and period is not None and label != "grammar-only":
            found = solution_period(cond, var, period, truths)
            if fraction_text(found) != ex.get("solution_period"):
                raise DataError("solution_period is %s, found %s" % (ex.get("solution_period"), fraction_text(found)))

    # Le varianti false: nel testimone i due lati devono avere verità diverse.
    for variant in ex["false_checks"]:
        _, c, r = split_check(variant["check"])
        p = witness_point(variant["witness"])
        if (c == cond) != variant.get("same_condition", True):
            raise DataError("a false variant has another condition, or says so and has not")
        if holds(c, {var: p}) == holds(r, {var: p}):
            raise DataError("a false variant holds at its witness: %s" % variant["check"])
    lines.append("   %s: %d false variants differ at their witnesses; period %s, solutions repeat every %s"
                 % (ex["number"], len(ex["false_checks"]), ex["period"], ex.get("solution_period")))

    # L'intersezione con l'asse y: un'uguaglianza tra numeri.
    if "y_axis" in ex and not holds(read_statement(ex["y_axis"]["statement"]), {}):
        raise DataError("the intersection with the y axis is wrong")
    return status, lines


def count_roots(f, env, var, low, low_closed, high, high_closed, steps=1024):
    """Quante soluzioni distinte ha f = 0 tra low e high, e quante di esse sono doppie.

    Si guarda il segno di f in steps + 1 punti equidistanti. Uno zero su un punto conta (agli
    estremi solo se l'estremo è compreso); un cambio di segno tra due punti vicini è una
    soluzione. Dove tre punti vicini hanno lo stesso segno ma la parabola per essi arriva vicino
    allo zero, f potrebbe toccare l'asse (soluzione doppia) o attraversarlo due volte: lì si
    cerca il minimo di |f| stringendo l'intervallo in tre parti, e si guarda il segno di f nel
    minimo. È un conto numerico, non una dimostrazione: assume che f abbia al più un massimo o
    minimo tra tre punti vicini."""
    width = sub(high, low)
    xs = [add(low, mul(exact(Fraction(i, steps)), width)) for i in range(steps + 1)]

    def at(x):
        try:
            return value(f, dict(env, **{var: x}))
        except Undefined:
            return None

    values = [at(x) for x in xs]
    signs = [None if v is None else sign(v) for v in values]
    count = doubles = 0
    for i, s in enumerate(signs):
        if s == 0:
            count += low_closed if i == 0 else high_closed if i == steps else 1
        if i and s and signs[i - 1] and s != signs[i - 1]:
            count += 1
    refined = []
    for i in range(1, steps):
        if signs[i] and signs[i - 1] == signs[i] == signs[i + 1]:
            y0, y1, y2 = (float(bounds(values[j])[0]) * signs[i] for j in (i - 1, i, i + 1))
            bend = y0 - 2 * y1 + y2
            if bend <= 0 or abs(y2 - y0) > 2 * bend or y1 - (y2 - y0) ** 2 / (8 * bend) > 1e-6:
                continue
            a, b = bounds(xs[i - 1])[0], bounds(xs[i + 1])[1]
            if any(a <= done <= b for done in refined):
                continue

            def height(x):
                v = at(Num(None, x, x))
                return bounds(v)[0] * signs[i], v

            while UP.subtract(b, a) > ONE.scaleb(-25):
                third = DOWN.divide(DOWN.subtract(b, a), Decimal(3))
                m1, m2 = DOWN.add(a, third), DOWN.subtract(b, third)
                a, b = (a, m2) if height(m1)[0] < height(m2)[0] else (m1, b)
            refined.append(a)
            lowest = height(a)[0]
            if lowest < -TINY:
                count += 2
            elif lowest <= TINY:
                count += 1
                doubles += 1
    return count, doubles


def check_counting(ex):
    """Un esercizio con un parametro: quante soluzioni nell'intervallo, al variare del parametro."""
    lines = []
    param = ex["parameter"]
    equation = read_statement(ex["equation"])
    interval = read_statement(ex["interval"])
    f = ("-", equation[2], equation[3])
    (_, rel_low, low, _), (_, rel_high, _, high) = interval[1], interval[2]
    low, high = value(low, {}), value(high, {})
    rows = [(row["solutions"], read_statement(row["parameter"])) for row in ex["counts"]]

    # Il check proposto: esiste una soluzione se e solo se il parametro sta in uno dei casi stampati.
    def split(text):
        s = read_statement(text)
        if s[0] != "forall" or s[1] != param or s[3][0] != "iff" or s[3][1] != ("exists", "x", "R", ("and", interval, equation)):
            raise DataError("the check of a parametric exercise must be \\forall %s (\\exists x (interval \\land equation) \\iff ...)" % param)
        return s[3][2]

    union = split(ex["check"])

    # I valori del parametro da provare: gli estremi dei casi, i punti accanto, i punti medi, lontano.
    ends = sorted((p for _, cond in rows for p in printed_angles(cond, param, None, None)), key=lambda p: bounds(p)[0])
    tried = list(ends)
    for p in ends:
        tried += [add(p, exact(Fraction(s, 10 ** e))) for s in (1, -1) for e in (3, 6)]
    tried += [mul(add(p, q), exact(HALF)) for p, q in zip(ends, ends[1:])]
    tried += [add(ends[0], exact(-d)) for d in (1, 10)] + [add(ends[-1], exact(d)) for d in (1, 10)]

    def counted(k):
        return count_roots(f, {param: k}, "x", low, rel_low == "<=", high, rel_high == "<=")

    wrong = coincident = 0
    for k in tried:
        distinct, doubles = counted(k)
        claims = [n for n, cond in rows if holds(cond, {param: k})]
        if len(claims) > 1:
            raise DataError("two printed cases overlap")
        claimed = claims[0] if claims else 0
        where = "%s = %s" % (param, describe(k)[1][:14])
        if distinct == claimed:
            pass
        elif distinct + doubles == claimed:
            coincident += 1
            lines.append("NOTE %s: at %s there are %d distinct solutions, %d of them double: the book counts coincident solutions twice and prints %d"
                         % (ex["number"], where, distinct, doubles, claimed))
        else:
            wrong += 1
            lines.append("NOT CONFIRMED %s (%s): at %s the equation has %d distinct solutions, the book prints %d"
                         % (ex["number"], ex["kind"], where, distinct, claimed))
        if (claimed > 0) != holds(union, {param: k}):
            raise DataError("the check does not say what the printed cases say at %s" % where)
    if (wrong == 0) != ex.get("book_confirmed", True):
        raise DataError("book_confirmed is %s but %d parameter values disagree" % (ex.get("book_confirmed", True), wrong))
    if not wrong:
        lines.append("CONFIRMED %s (%s): solutions counted for %d values of %s%s"
                     % (ex["number"], ex["kind"], len(tried), param, ", with coincident solutions counted twice" if coincident else ""))

    # Il check corretto, se c'è, deve reggere; le varianti false devono cadere nel testimone.
    if "corrected_check" in ex:
        fixed = split(ex["corrected_check"])
        for k in tried:
            if (counted(k)[0] > 0) != holds(fixed, {param: k}):
                raise DataError("the corrected check fails at %s = %s" % (param, describe(k)[1][:14]))
    for variant in ex["false_checks"]:
        k = witness_point(variant["witness"])
        if (counted(k)[0] > 0) == holds(split(variant["check"]), {param: k}):
            raise DataError("a false variant holds at its witness: %s" % variant["check"])
    lines.append("   %s: %d false variants differ at their witnesses" % (ex["number"], len(ex["false_checks"])))
    return ("CONFIRMED" if not wrong else "NOT CONFIRMED"), lines


def check_data(path):
    tally, failed = {}, False
    with open(path, encoding="utf-8") as source:
        exercises = [json.loads(line) for line in source if line.strip()]
    for ex in exercises:
        try:
            if ex["kind"] == "count_by_parameter":
                status, lines = check_counting(ex)
            elif ex.get("check"):
                status, lines = check_solving(ex)
            else:
                status, lines = "NOT CHECKED", ["NOTE %s (%s): nothing to check: %s" % (ex["number"], ex["kind"], ex["notes"][:110])]
        except (DataError, Unsupported, Undecided, KeyError) as error:
            status, lines = "DATA ERROR", ["DATA ERROR %s: %s %s" % (ex.get("number"), type(error).__name__, error)]
        failed = failed or status == "DATA ERROR" or any(line.startswith("SUSPECT") for line in lines)
        tally[status] = tally.get(status, 0) + 1
        print("\n".join(lines), flush=True)
    print("%d exercises: %s; %d signs decided by the 10^-45 rule"
          % (len(exercises), ", ".join("%s %d" % item for item in sorted(tally.items())), NUMERIC_ZEROS[0]))
    return 1 if failed else 0
