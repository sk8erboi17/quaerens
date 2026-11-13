r"""Fuzzing differenziale dei limiti (capitolo 29).

Uso: python3 -I fuzz_limits.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Genera funzioni di una lettera y che tende a 0, fatte di polinomi a
coefficienti razionali senza termine noto e delle funzioni elementari che lì
hanno una serie di Taylor a coefficienti razionali (e^u, ln(1 + u), sin u,
cos u, tan u, arctan u, arcsin u, sinh u, cosh u, (1 + u)^{1/2},
(1 + u)^{1/3}), in quozienti costruiti per cancellare i primi termini: per
esempio sin(2y) meno il suo polinomio di Taylor fino a y^3, diviso y^5. Poi
le scrive in x con una sostituzione: y = x (x -> 0, da un lato o dai due),
y = x - a (x -> a), y = 1/x (x -> +\infty, y -> 0^{+}; x -> -\infty,
y -> 0^{-}). Aggiunge le forme esponenziali F^G, con F -> 1 e G che cresce
come una potenza di 1/y. E la famiglia della scala delle crescite (29k e 29l
del programma): quozienti di somme di monomi c x^a (ln x)^b (ln ln x)^g
e^{q x^r} a +\infty, o scritti in 1/x verso 0^{+}, di cui l'oracolo trova il
termine principale con l'ordine lessicografico della scala e lo ricontrolla
con i logaritmi dei termini in x = 10^{40} e 10^{80} (vedi growth()).

L'ORACOLO

Il limite lo calcola questo script, con un'aritmetica delle serie scritta qui
in Python con le frazioni esatte, senza chiedere nulla al programma: la serie
di Laurent di g in y ha un termine principale c y^k, e il limite è c se k = 0,
0 se k > 0, un infinito con il segno di c (e di (-1)^k da sinistra) se k < 0;
dai due lati, con k dispari e negativo, il limite non esiste (e |g| tende a
+\infty). Per una forma esponenziale il limite di G ln F dà e^{L}, +\infty o
0. Ogni termine principale viene poi ricontrollato con i numeri, con il modulo
decimal a due precisioni lontane (240 e 480 cifre: vedi le lezioni nella
classificazione degli esercizi), in y = 10^{-6} e y = 10^{-9}, dove il
rapporto tra g e c y^k deve essere vicino a 1. Se non lo è, l'oracolo ha
sbagliato e il caso si scarta con una riga ORACLE.

GLI ENUNCIATI

Per ogni funzione: il limite giusto (deve essere DIMOSTRATO o NON
DIMOSTRATO, mai FALSO); alcuni sbagliati di poco (un valore più 1 o meno 1/7,
il doppio, il segno cambiato, l'infinito con l'altro segno, "non esiste"
detto di un limite che esiste e viceversa: mai DIMOSTRATI); la richiesta
\lim_{x \to a} f, la cui risposta RISOLTO viene letta qui e confrontata con
il limite dell'oracolo.

Righe stampate: BUG quando il programma dimostra un falso, dichiara falso un
vero, o risolve con un valore sbagliato; SUSPECT quando non si riesce a
leggere la risposta, o il programma scarta un passo della sua dimostrazione
(il controllo con i numeri del capitolo 29 ha contraddetto il conto: un
errore del motore preso in tempo, da guardare); ORACLE quando l'oracolo non
supera il proprio ricontrollo. In fondo il riepilogo dei verdetti.

I file (gli enunciati, l'uscita del programma) vanno nella cartella di lavoro,
fuori dal repository.
"""
import decimal
import os
import random
import re
import subprocess
import sys
from decimal import Decimal as D
from fractions import Fraction as Fr

ORDER = 14  # termini delle serie dell'oracolo: abbastanza per le cancellazioni generate


# ------------------------------------------------------------------ le serie esatte
#
# Una serie è (v, c): la somma dei c[i] y^{v + i} per i < len(c), più un resto
# O(y^{v + len(c)}). Le operazioni sono quelle dei libri di analisi, con le
# frazioni di Python: niente in comune con il programma, che è in C.

def s_trim(s):
    v, c = s
    i = 0
    while i < len(c) and c[i] == 0:
        i += 1
    return (v + i, c[i:])


def s_const(q, order=ORDER):
    return (0, [Fr(q)] + [Fr(0)] * (order - 1))


def s_y(order=ORDER):
    return (1, [Fr(1)] + [Fr(0)] * (order - 1))


def s_end(s):
    return s[0] + len(s[1])


def s_add(a, b, sign=1):
    v = min(a[0], b[0])
    end = min(s_end(a), s_end(b))
    c = []
    for e in range(v, end):
        x = a[1][e - a[0]] if a[0] <= e < s_end(a) else Fr(0)
        y = b[1][e - b[0]] if b[0] <= e < s_end(b) else Fr(0)
        c.append(x + sign * y)
    return (v, c)


def s_mul(a, b):
    a, b = s_trim(a), s_trim(b)
    if not a[1] or not b[1]:
        return (a[0] + b[0], [])
    n = min(len(a[1]), len(b[1]))
    c = [sum(a[1][i] * b[1][k - i] for i in range(k + 1)) for k in range(n)]
    return (a[0] + b[0], c)


def s_scale(a, q):
    return (a[0], [q * x for x in a[1]])


def s_inv(a):
    a = s_trim(a)
    if not a[1]:
        raise ArithmeticError("divisione per una serie senza termini noti")
    c0 = a[1][0]
    n = len(a[1])
    r = [Fr(1) / c0]
    for k in range(1, n):
        r.append(-sum(a[1][i] * r[k - i] for i in range(1, k + 1)) / c0)
    return (-a[0], r)


def s_div(a, b):
    return s_mul(a, s_inv(b))


def s_pow_int(a, n):
    r = s_const(1, len(s_trim(a)[1]) or 1)
    for _ in range(n):
        r = s_mul(r, a)
    return r


def s_compose(coefficients, w):
    """La somma dei coefficients[k] w^k, con w che tende a 0 (esponente > 0)."""
    w = s_trim(w)
    if w[1] and w[0] < 1:
        raise ArithmeticError("l'argomento non tende a 0")
    end = s_end(w) if w[1] else ORDER
    result = (0, [Fr(0)] * end)
    power = s_const(1, end)
    for k, a in enumerate(coefficients):
        if k * max(w[0], 1) >= end:
            break
        result = s_add(result, s_scale(power, a))
        power = s_mul(power, w)
        power = (power[0], power[1][:max(0, end - power[0])])
    return (0, result[1][:end])


def factorial(k):
    r = 1
    for i in range(2, k + 1):
        r *= i
    return r


def taylor(name, n=ORDER + 2):
    """I coefficienti di Taylor in 0 della funzione name (di u, con u -> 0)."""
    if name == "exp":
        return [Fr(1, factorial(k)) for k in range(n)]
    if name == "log1p":
        return [Fr(0)] + [Fr((-1) ** (k + 1), k) for k in range(1, n)]
    if name == "sin":
        return [Fr(0) if k % 2 == 0 else Fr((-1) ** (k // 2), factorial(k)) for k in range(n)]
    if name == "cos":
        return [Fr((-1) ** (k // 2), factorial(k)) if k % 2 == 0 else Fr(0) for k in range(n)]
    if name == "sinh":
        return [Fr(0) if k % 2 == 0 else Fr(1, factorial(k)) for k in range(n)]
    if name == "cosh":
        return [Fr(1, factorial(k)) if k % 2 == 0 else Fr(0) for k in range(n)]
    if name == "atan":
        return [Fr(0) if k % 2 == 0 else Fr((-1) ** (k // 2), k) for k in range(n)]
    if name == "asin":
        # arcsin u = somma di (2j)! / (4^j (j!)^2 (2j + 1)) u^{2j+1}
        return [Fr(0) if k % 2 == 0 else Fr(factorial(k - 1), 4 ** (k // 2) * factorial(k // 2) ** 2 * k) for k in range(n)]
    if name.startswith("root"):
        q = Fr(1, int(name[4:]))
        out, b = [], Fr(1)
        for k in range(n):
            out.append(b)
            b = b * (q - k) / (k + 1)
        return out
    raise ValueError(name)


def tan_series(w):
    return s_div(s_compose(taylor("sin"), w), s_compose(taylor("cos"), w))


# ------------------------------------------------------------------ i numeri
#
# Lo stesso albero calcolato in un y decimale: serve a ricontrollare la serie
# (lezione 1 della classificazione: due precisioni lontane).

def d_exp(x):
    return x.exp()


def d_sin_cos(x):
    # Serie di Taylor: qui x è piccolo o moderato, e la serie converge in fretta.
    eps = D(10) ** (-decimal.getcontext().prec - 5)
    term_s, term_c = x, D(1)
    s, c = term_s, term_c
    k = 1
    while abs(term_s) > eps or abs(term_c) > eps:
        term_c = -term_c * x * x / ((2 * k - 1) * (2 * k))
        term_s = -term_s * x * x / ((2 * k) * (2 * k + 1))
        c += term_c
        s += term_s
        k += 1
        if k > 4000:
            break
    return s, c


def d_atan(x):
    # atan x = atan(x / (1 + sqrt(1 + x^2))) * 2, ripetuto finché x è piccolo
    doublings = 0
    while abs(x) > D("0.1"):
        x = x / (1 + (1 + x * x).sqrt())
        doublings += 1
    eps = D(10) ** (-decimal.getcontext().prec - 5)
    term, total, k = x, x, 1
    while abs(term) > eps:
        term = -term * x * x
        total += term / (2 * k + 1)
        k += 1
    return total * (2 ** doublings)


def d_eval(node, y):
    kind = node[0]
    if kind == "y":
        return y
    if kind == "num":
        return D(node[1].numerator) / D(node[1].denominator)
    if kind in ("add", "sub", "mul", "div"):
        a, b = d_eval(node[1], y), d_eval(node[2], y)
        return a + b if kind == "add" else a - b if kind == "sub" else a * b if kind == "mul" else a / b
    if kind == "pow":
        return d_eval(node[1], y) ** node[2]
    if kind == "fn":
        name, u = node[1], d_eval(node[2], y)
        if name == "exp":
            return d_exp(u)
        if name == "log1p":
            return (1 + u).ln()
        if name in ("sin", "cos", "tan"):
            s, c = d_sin_cos(u)
            return s if name == "sin" else c if name == "cos" else s / c
        if name == "sinh":
            return (d_exp(u) - d_exp(-u)) / 2
        if name == "cosh":
            return (d_exp(u) + d_exp(-u)) / 2
        if name == "atan":
            return d_atan(u)
        if name == "asin":
            return 2 * d_atan(u / (1 + (1 - u * u).sqrt()))
        if name.startswith("root"):
            n = int(name[4:])
            v = 1 + u
            return v.sqrt() if n == 2 else (v.ln() / n).exp()
    if kind == "powf":
        # F^G con F > 0
        return (d_eval(node[2], y) * d_eval(node[1], y).ln()).exp()
    raise ValueError(kind)


# ------------------------------------------------------------------ gli alberi
#
# Nodi: ("y",), ("num", Fraction), ("add"|"sub"|"mul"|"div", a, b),
# ("pow", a, n) con n intero positivo, ("fn", nome, a) dove a tende a 0,
# ("powf", F, G) per F^G.

def series(node):
    kind = node[0]
    if kind == "y":
        return s_y()
    if kind == "num":
        return s_const(node[1])
    if kind == "add":
        return s_add(series(node[1]), series(node[2]))
    if kind == "sub":
        return s_add(series(node[1]), series(node[2]), -1)
    if kind == "mul":
        return s_mul(series(node[1]), series(node[2]))
    if kind == "div":
        return s_div(series(node[1]), series(node[2]))
    if kind == "pow":
        return s_pow_int(series(node[1]), node[2])
    if kind == "fn":
        name, w = node[1], series(node[2])
        if name == "tan":
            return tan_series(w)
        if name.startswith("root"):
            return s_compose(taylor(name), w)
        return s_compose(taylor(name), w)
    raise ValueError(kind)


def render(node, y):
    """Il LaTeX del nodo, con la lettera y scritta come y (un testo)."""
    kind = node[0]
    if kind == "y":
        return y
    if kind == "num":
        q = node[1]
        if q.denominator == 1:
            return "%d" % q.numerator if q >= 0 else "\\left(%d\\right)" % q.numerator
        text = "\\frac{%d}{%d}" % (abs(q.numerator), q.denominator)
        return text if q > 0 else "\\left(-%s\\right)" % text
    if kind in ("add", "sub"):
        return "\\left(%s %s %s\\right)" % (render(node[1], y), "+" if kind == "add" else "-", render(node[2], y))
    if kind == "mul":
        return "%s \\cdot %s" % (render(node[1], y), render(node[2], y))
    if kind == "div":
        return "\\frac{%s}{%s}" % (render(node[1], y), render(node[2], y))
    if kind == "pow":
        return "\\left(%s\\right)^{%d}" % (render(node[1], y), node[2])
    if kind == "powf":
        return "\\left(%s\\right)^{%s}" % (render(node[1], y), render(node[2], y))
    name, u = node[1], render(node[2], y)
    if name == "exp":
        return "e^{%s}" % u
    if name == "log1p":
        return "\\ln\\left(1 + %s\\right)" % u
    if name.startswith("root"):
        n = int(name[4:])
        return "\\sqrt%s{1 + %s}" % ("" if n == 2 else "[%d]" % n, u)
    command = {"sin": "sin", "cos": "cos", "tan": "tan", "sinh": "sinh", "cosh": "cosh", "atan": "arctan", "asin": "arcsin"}[name]
    return "\\%s\\left(%s\\right)" % (command, u)


FUNCTIONS = ["exp", "log1p", "sin", "cos", "tan", "atan", "asin", "sinh", "cosh", "root2", "root3"]
VALUE_AT_ZERO = {"exp": 1, "cos": 1, "cosh": 1, "root2": 1, "root3": 1}


def small(rnd):
    return Fr(rnd.choice([1, 1, 2, 3, -1, -2, 3, 1]), rnd.choice([1, 1, 1, 2, 3]))


def inner(rnd, depth):
    """Un'espressione che tende a 0."""
    if depth == 0 or rnd.random() < 0.4:
        # Un polinomio senza termine noto.
        node = ("mul", ("num", small(rnd)), ("y",))
        if rnd.random() < 0.5:
            node = ("add", node, ("mul", ("num", small(rnd)), ("pow", ("y",), rnd.choice([2, 3]))))
        return node
    name = rnd.choice(FUNCTIONS)
    node = ("fn", name, inner(rnd, depth - 1))
    zero = VALUE_AT_ZERO.get(name, 0)
    return ("sub", node, ("num", Fr(zero))) if zero else node


def polynomial(coefficients, start, stop):
    """Il nodo della somma dei coefficients[k] y^k per k da start a stop - 1."""
    node = None
    for k in range(start, stop):
        q = coefficients[k]
        if q == 0:
            continue
        term = ("mul", ("num", q), ("y",) if k == 1 else ("pow", ("y",), k)) if k else ("num", q)
        node = term if node is None else ("add", node, term)
    return node


def cancelling(rnd):
    """Una funzione che tende a 0, con alcuni dei suoi primi termini tolti."""
    node = inner(rnd, rnd.choice([1, 1, 2]))
    if rnd.random() < 0.6:
        s = s_trim(series(node))
        if s[1]:
            cut = rnd.randint(1, 3)
            coefficients = [Fr(0)] * (s[0] + cut)
            for i in range(min(cut, len(s[1]))):
                coefficients[s[0] + i] = s[1][i]
            correction = polynomial(coefficients, 0, s[0] + cut)
            if correction is not None:
                node = ("sub", node, correction)
    return node


def generate(rnd):
    """Una funzione di y e il modo di avvicinarsi: ritorna (albero, esponenziale)."""
    if rnd.random() < 0.2:
        base = ("add", ("num", Fr(1)), inner(rnd, 1))
        exponent = ("div", ("num", small(rnd)), ("pow", ("y",), rnd.choice([1, 1, 2])) if rnd.random() < 0.5 else ("y",))
        return ("powf", base, exponent), True
    top = cancelling(rnd)
    if rnd.random() < 0.3:
        bottom = cancelling(rnd)
    else:
        bottom = ("pow", ("y",), rnd.randint(1, 5))
    if rnd.random() < 0.15:
        top = ("add", top, ("mul", ("num", small(rnd)), bottom))
    return ("div", top, bottom), False


def logarithmic(rnd):
    """La famiglia della scala dei logaritmi: k y^m (ln y)^j (1 + c y), con
    y -> 0^{+}, scritta con y = x (x -> 0^{+}) o y = 1/x (x -> +\\infty). Il
    limite si sa senza serie: ln y tende a -\\infty più lentamente di ogni
    potenza di 1/y, quindi con m > 0 vince y^m e il limite è 0; con m < 0, o
    con m = 0 e j > 0, è infinito con il segno di k (-1)^j; con m = j = 0 è k.
    Ritorna (testo del limite, limite)."""
    k, c = small(rnd), small(rnd)
    m, j = rnd.choice([-2, -1, 0, 0, 1, 1, 2]), rnd.choice([0, 1, 1, 2, 3])
    if rnd.random() < 0.5:
        where, y = "x \\to 0^{+}", "x"
    else:
        where, y = "x \\to +\\infty", "\\frac{1}{x}"
    text = render(("num", k), y)
    if m:
        power = "\\left(%s\\right)^{%d}" % (y, abs(m))
        text = "\\frac{%s}{%s}" % (text, power) if m < 0 else "%s \\cdot %s" % (text, power)
    if j:
        text += " \\cdot \\left(\\ln\\left(%s\\right)\\right)^{%d}" % (y, j)
    text += " \\cdot %s" % render(("add", ("num", Fr(1)), ("mul", ("num", c), ("y",))), y)
    if m > 0:
        result = ("value", Fr(0))
    elif m < 0 or j > 0:
        result = ("inf", 1 if k * (-1) ** j > 0 else -1)
    else:
        result = ("value", k)
    return "\\lim_{%s} %s" % (where, text), result


# ------------------------------------------------------------------ la scala delle crescite
#
# La famiglia delle crescite (29k, 29l del programma): quozienti di somme di
# monomi della scala
#
#     c x^{a} (ln x)^{b} (ln ln x)^{g} e^{q x^{r}},
#
# con x -> +\infty, oppure scritti in 1/x con x -> 0^{+}. L'oracolo non
# chiede niente al programma, e non ne copia il metodo: il termine
# principale di una somma di monomi diversi è quello più grande nell'ordine
# lessicografico dei libri, prima l'esponenziale (r, poi q), poi la potenza
# (a), poi il logaritmo (b), poi il logaritmo del logaritmo (g), perché
# ognuno di questi fattori batte ogni potenza del successivo. Il limite del
# quoziente è quello del quoziente dei due termini principali. L'ordine viene
# ricontrollato con i numeri: i logaritmi dei termini, calcolati con decimal
# a 120 cifre in x = 10^{40} e x = 10^{80} (lezione 4: la convergenza
# logaritmica non si vede con x piccoli, e qui si guarda solo chi è il più
# grande, non il valore), devono dare lo stesso termine più grande; se no il
# caso si scarta con una riga ORACLE.

GROWTH_EXPONENTIALS = [(0, 0), (1, 1), (1, -1), (2, 1), (Fr(1, 2), 1), (Fr(1, 2), -1), (1, 2), (1, -3)]


def monomial(rnd):
    """Un monomio (c, a, b, g, r, q): c x^a (ln x)^b (ln ln x)^g e^{q x^r}."""
    c = Fr(rnd.choice([1, 2, 3, -1, -2, 5]), rnd.choice([1, 1, 2, 3]))
    a = Fr(rnd.choice([0, 0, 1, 2, 3, -1, -2, 5]), rnd.choice([1, 1, 1, 2]))
    b = rnd.choice([0, 0, 0, 1, 2, -1, 3])
    g = rnd.choice([0, 0, 0, 0, 0, 1, -1]) if rnd.random() < 0.25 else 0
    r, q = rnd.choice(GROWTH_EXPONENTIALS) if rnd.random() < 0.5 else (0, 0)
    return (c, a, Fr(b), Fr(g), Fr(r), Fr(q))


def scale_key(m):
    """L'ordine di grandezza del monomio, senza il coefficiente: prima
    l'esponenziale e^{q x^r} (che cresce più in fretta con r più grande, a
    parità di r con q più grande; e decresce più in fretta con r più grande
    se q < 0), poi la potenza, il logaritmo, il logaritmo del logaritmo."""
    c, a, b, g, r, q = m
    e = (Fr(0), Fr(0)) if q == 0 else ((r, q) if q > 0 else (-r, q))
    return (e, a, b, g)


def dominant(terms):
    """Il termine più grande (oracolo): l'ordine lessicografico della scala."""
    return max(terms, key=scale_key)


def merge(terms):
    """Somma i monomi uguali, e toglie quelli nulli."""
    out = {}
    for c, *rest in terms:
        out[tuple(rest)] = out.get(tuple(rest), Fr(0)) + c
    return [(c,) + k for k, c in out.items() if c != 0]


def log_term(m, x):
    """ln|termine| in x (Decimal), e il segno."""
    c, a, b, g, r, q = m
    lx = x.ln()
    value = D(abs(c.numerator)).ln() - D(c.denominator).ln() + D(a.numerator) / D(a.denominator) * lx
    if b:
        value += D(b.numerator) / D(b.denominator) * lx.ln()
    if g:
        value += D(g.numerator) / D(g.denominator) * lx.ln().ln()
    if q:
        value += D(q.numerator) / D(q.denominator) * (D(r.numerator) / D(r.denominator) * lx).exp()
    return value


def growth_tex(m, y):
    """Il LaTeX del monomio, con x scritto come y (x, o \\frac{1}{x})."""
    c, a, b, g, r, q = m
    parts = []
    if a:
        parts.append("\\left(%s\\right)^{%s}" % (y, value_tex(a)))
    if b:
        parts.append("\\left(\\ln\\left(%s\\right)\\right)^{%s}" % (y, value_tex(b)))
    if g:
        parts.append("\\left(\\ln\\left(\\ln\\left(%s\\right)\\right)\\right)^{%s}" % (y, value_tex(g)))
    if q:
        power = y if r == 1 else "\\left(%s\\right)^{%s}" % (y, value_tex(r))
        parts.append("e^{%s %s}" % (value_tex(q) if abs(q) != 1 else ("-" if q < 0 else ""), power))
    body = " \\cdot ".join(parts)
    if not body:
        return value_tex(c)
    if c == 1:
        return body
    if c == -1:
        return "-" + body
    return "%s \\cdot %s" % (value_tex(c), body)


def growth_sum_tex(terms, y):
    text = growth_tex(terms[0], y)
    for m in terms[1:]:
        t = growth_tex(m, y)
        text += " %s" % (("- " + t[1:]) if t.startswith("-") else ("+ " + t))
    return "\\left(%s\\right)" % text


def growth(rnd):
    """Un limite della famiglia delle crescite: ritorna (testo del limite,
    limite), o None se l'oracolo non conferma l'ordine con i numeri."""
    top = merge([monomial(rnd) for _ in range(rnd.choice([1, 2, 2, 3]))])
    bottom = merge([monomial(rnd) for _ in range(rnd.choice([0, 1, 1, 2]))]) if rnd.random() < 0.7 else []
    if not top:
        return None
    bottom = bottom or [(Fr(1), Fr(0), Fr(0), Fr(0), Fr(0), Fr(0))]

    # I termini principali, ricontrollati con i numeri.
    decimal.getcontext().prec = 120
    for terms in (top, bottom):
        lead = dominant(terms)
        for k in (40, 80):
            x = D(10) ** k
            logs = [(log_term(m, x), m) for m in terms]
            if max(logs, key=lambda p: p[0])[1] != lead:
                print("ORACLE: the scale and the numbers disagree on the largest term at x = 1e%d" % k)
                return None
    n, d = dominant(top), dominant(bottom)
    c = n[0] / d[0]
    if scale_key(n) == scale_key(d):
        result = ("value", c)
    elif scale_key(n) > scale_key(d):
        result = ("inf", 1 if c > 0 else -1)
    else:
        result = ("value", Fr(0))
    if rnd.random() < 0.6:
        where, y = "x \\to +\\infty", "x"
    else:
        where, y = "x \\to 0^{+}", "\\frac{1}{x}"
    text = growth_sum_tex(top, y)
    if bottom != [(Fr(1), Fr(0), Fr(0), Fr(0), Fr(0), Fr(0))]:
        text = "\\frac{%s}{%s}" % (text, growth_sum_tex(bottom, y))
    return "\\lim_{%s} %s" % (where, text), result


# ------------------------------------------------------------------ il limite vero

def leading(node):
    """Il termine principale (c, k) della serie di node, ricontrollato con i numeri."""
    s = s_trim(series(node))
    if not s[1]:
        raise ArithmeticError("serie senza termini noti")
    c, k = s[1][0], s[0]
    for prec in (240, 480):
        decimal.getcontext().prec = prec
        for exponent in (6, 9):
            y = D(10) ** (-exponent)
            ratio = d_eval(node, y) / (D(c.numerator) / D(c.denominator) * y ** k)
            if abs(ratio - 1) > D("0.01"):
                raise AssertionError("the series and the numbers disagree: ratio %s at y = 1e-%d" % (str(ratio)[:12], exponent))
    return c, k


def truth(node, exponential, side):
    """Il limite per y -> 0 dal lato side (+1, -1, 0 per i due lati).
    Ritorna ("value", Fraction), ("exp", Fraction), ("inf", +1/-1), ("none",
    segno se |g| -> +inf con segni opposti)."""
    if exponential:
        c, k = leading(("mul", node[2], ("fn", "log1p", ("sub", node[1], ("num", Fr(1))))))
        if k > 0:
            return ("exp", Fr(0))
        if k == 0:
            return ("exp", c)
        signs = [c * (1 if s > 0 or k % 2 == 0 else -1) for s in ((side,) if side else (1, -1))]
        if all(x > 0 for x in signs):
            return ("inf", 1)
        if all(x < 0 for x in signs):
            return ("exp-zero", 0)
        return ("none", 0)  # da un lato +\infty, dall'altro 0
    c, k = leading(node)
    if k > 0:
        return ("value", Fr(0))
    if k == 0:
        return ("value", c)
    signs = [c if s > 0 or k % 2 == 0 else -c for s in ((side,) if side else (1, -1))]
    if all(x > 0 for x in signs):
        return ("inf", 1)
    if all(x < 0 for x in signs):
        return ("inf", -1)
    return ("none", 1)


def value_tex(q):
    if q.denominator == 1:
        return "%d" % q.numerator
    return "%s\\frac{%d}{%d}" % ("-" if q < 0 else "", abs(q.numerator), q.denominator)


def claims(result):
    """Il valore giusto e alcuni sbagliati di poco, come testi da mettere dopo
    il segno = (None vuol dire "non esiste", con \\nexists davanti)."""
    kind, v = result
    if kind == "value":
        right = value_tex(v)
        wrong = [value_tex(v + 1), value_tex(v - Fr(1, 7)), "+\\infty", None]
        if v != 0:
            wrong += [value_tex(2 * v), value_tex(-v)]
        return [right], wrong
    if kind == "exp":
        right = "1" if v == 0 else "e^{%s}" % value_tex(v)
        wrong = ["e^{%s}" % value_tex(v + 1), "e^{%s}" % value_tex(-v) if v else "e", "0", None]
        return [right], wrong
    if kind == "exp-zero":
        return ["0"], ["1", "+\\infty", None]
    if kind == "inf":
        right = "+\\infty" if v > 0 else "-\\infty"
        return [right, "\\infty"], ["-\\infty" if v > 0 else "+\\infty", "0", None]
    # Non esiste: i due limiti da un lato sono infiniti di segno opposto (v = 1),
    # e allora |g| tende a +\infty, oppure +\infty e 0 (v = 0).
    if v:
        return [None, "\\infty"], ["+\\infty", "-\\infty", "0"]
    return [None], ["+\\infty", "0", "\\infty"]


# ------------------------------------------------------------------ gli enunciati

def place(rnd):
    """Il punto: (testo del pedice, lato dell'oracolo, testo di y in x)."""
    choice = rnd.random()
    if choice < 0.35:
        return "x \\to 0", 0, "x"
    if choice < 0.5:
        return "x \\to 0^{+}", 1, "x"
    if choice < 0.6:
        return "x \\to 0^{-}", -1, "x"
    if choice < 0.75:
        a = rnd.choice([1, 2, -1, 3, -2])
        return "x \\to %d" % a, 0, "\\left(x %s %d\\right)" % ("-" if a > 0 else "+", abs(a))
    if choice < 0.9:
        return "x \\to +\\infty", 1, "\\frac{1}{x}"
    return "x \\to -\\infty", -1, "\\frac{1}{x}"


def answer_value(text):
    """Il valore scritto dal programma dopo il segno =, nella forma in cui lo
    scrive per questi limiti; None se non lo si sa leggere."""
    text = text.strip()
    if text in ("+\\infty", "-\\infty"):
        return ("inf", 1 if text[0] == "+" else -1)
    m = re.fullmatch(r"(-?)(\d+)", text)
    if m:
        return ("value", Fr(int(m.group(2))) * (-1 if m.group(1) else 1))
    m = re.fullmatch(r"(-?)\\frac\{(\d+)\}\{(\d+)\}", text)
    if m:
        return ("value", Fr(int(m.group(2)), int(m.group(3))) * (-1 if m.group(1) else 1))
    m = re.fullmatch(r"e\^\{(.*)\}", text)
    if m:
        inner_value = answer_value(m.group(1))
        return ("exp", inner_value[1]) if inner_value and inner_value[0] == "value" else None
    if text == "e":
        return ("exp", Fr(1))
    return None


def same_answer(written, result):
    kind, v = result
    if written is None:
        return False
    if kind == "exp":
        return (written[0] == "exp" and written[1] == v) or (v == 0 and written == ("value", Fr(1)))
    if kind == "exp-zero":
        return written == ("value", Fr(0))
    return written == result


def main():
    if len(sys.argv) != 5:
        print(__doc__)
        sys.exit(2)
    binary, seed, count, work = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
    os.makedirs(work, exist_ok=True)
    rnd = random.Random(seed)
    cases = []  # (testo, atteso: "true" | "false" | ("request", result))
    for _ in range(count):
        choice = rnd.random()
        if choice < 0.12:
            limit, result = logarithmic(rnd)
        elif choice < 0.4:
            case = growth(rnd)
            if case is None:
                continue
            limit, result = case
        else:
            node, exponential = generate(rnd)
            where, side, y = place(rnd)
            try:
                result = truth(node, exponential, side)
            except AssertionError as problem:
                print("ORACLE: %s\n   %s" % (problem, render(node, "y")))
                continue
            except (ArithmeticError, decimal.DecimalException, ValueError, ZeroDivisionError):
                continue
            limit = "\\lim_{%s} %s" % (where, render(node, y))
        right, wrong = claims(result)
        for text in right:
            cases.append(("\\nexists " + limit if text is None else limit + " = " + text, "true"))
        for text in wrong:
            cases.append(("\\nexists " + limit if text is None else limit + " = " + text, "false"))
        cases.append((limit, ("request", result)))

    path = os.path.join(work, "limits_%d.tex" % seed)
    with open(path, "w") as f:
        f.write("\\begin{enumerate}\n")
        for text, _ in cases:
            f.write("\\item $\\displaystyle %s$\n" % text)
        f.write("\\end{enumerate}\n")
    run = subprocess.run([binary, path], capture_output=True, text=True)
    lines = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
    if len(lines) != len(cases):
        print("BUG: the program answered %d of %d statements (exit %d)\n%s" % (len(lines), len(cases), run.returncode, run.stderr[-2000:]))
        return
    tally = {}
    for number, ((text, expected), line) in enumerate(zip(cases, lines), 1):
        answer = line.split(". ", 1)[1]
        verdict = answer.split(":")[0].split(" ")[0]
        if verdict == "NON":
            verdict = " ".join(answer.split(":")[0].split(" ")[:2])
        if isinstance(expected, tuple):
            kind = "request"
            if verdict == "RISOLTO":
                detail = answer.split(": ", 1)[1].split(" (")[0]
                if detail.startswith("\\nexists"):
                    good = expected[1][0] == "none"
                else:
                    written = answer_value(detail.split(" = ")[-1])
                    if written is None:
                        print("SUSPECT: statement %d, cannot read the answer: %s\n   %s" % (number, detail[-80:], text))
                        good = True
                    else:
                        good = same_answer(written, expected[1])
                if not good:
                    print("BUG: statement %d is solved wrongly: %s (the limit is %s)\n   %s" % (number, answer[:200], expected[1], text))
        else:
            kind = expected
            if expected == "true" and verdict == "FALSO":
                print("BUG: statement %d is true but the program says %s\n   %s" % (number, answer[:120], text))
            if expected == "false" and verdict == "DIMOSTRATO":
                print("BUG: statement %d is false but the program says %s\n   %s" % (number, answer[:120], text))
        tally[(kind, verdict)] = tally.get((kind, verdict), 0) + 1
    if "ATTENZIONE" in run.stdout:
        print("SUSPECT: the program discarded a step of its own proof: %s" % [l for l in run.stdout.splitlines() if "ATTENZIONE" in l])
    print("seed %d:" % seed, ", ".join("%s/%s=%d" % (k[0], k[1], v) for k, v in sorted(tally.items())))


if __name__ == "__main__":
    main()
