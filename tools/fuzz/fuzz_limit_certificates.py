r"""Fuzzing del verificatore sui certificati dei limiti (capitolo 14 di checker.c).

Uso: python3 -I fuzz_limit_certificates.py <checker> <seme> <quanti> <cartella di lavoro> <file di certificati>...

I file di certificati sono quelli scritti da mathresearcher --certificates: se
ne prendono i certificati limit, e se ne ricavano <quanti> alterati a caso,
con da una a tre alterazioni ciascuno: la verità rovesciata; il valore
affermato cambiato (più o meno 1, il doppio, il segno, un'altra costante, e
numeri che non sono reali, come ln(-e), 1/(1 - 1), la radice di un
negativo); il tipo di affermazione; la sostituzione o l'ordine di un lato; il
punto o il suo lato; un intero o una funzione dentro la funzione; un lato tolto
o ripetuto. Il verificatore li controlla tutti, e per ognuno che ACCETTA
l'oracolo decide da sé se ciò che il certificato afferma è vero, guardando solo
la funzione, il punto e l'affermazione: delle sostituzioni e degli ordini non
usa nulla.

L'oracolo. Calcola la funzione con il modulo decimal, a due precisioni lontane
(80 e 160 cifre; un punto in cui le due non sono d'accordo non si usa), nei
punti x = a + 10^{-k}, a - 10^{-k}, oppure x = 10^{k}, -10^{k}, per k da 1 a
48. Il dominio è quello naturale, come lo definisce il teorema del capitolo 14:
ln di un positivo, divisori non nulli, radici di indice pari di un non
negativo, potenze con esponente non intero o non costante di una base
positiva, arcsin e arccos tra -1 e 1. Un numero che esce dal contesto, anche
verso lo zero (e^{-10^{12}}), fa scartare il punto: uno zero che viene da un
arrotondamento non dice che la funzione lì non è definita (era un falso BUG:
1/3^{1/x} sembrava senza valori da sinistra di 0). Da ciò che vede negli ultimi punti
calcolabili di ogni lato giudica l'affermazione: vera, falsa, oppure non la
sa giudicare (una convergenza lenta, come quella di 1/ln(1/t), valori che
escono dai numeri). Un valore o un punto che non si calcolano (ln(-e)) non
sono numeri: un certificato che li contiene non dice nulla di vero. Non usa
serie, né nulla del programma.

BUG: il verificatore ha accettato un certificato che l'oracolo giudica falso
con sicurezza (o un certificato del motore non alterato è giudicato falso).
SUSPECT: un certificato accettato che l'oracolo non sa giudicare: va guardato
a mano. I file prodotti vanno nella cartella di lavoro, mai nel repository.
"""
import decimal
import os
import random
import re
import subprocess
import sys
from decimal import Decimal as D
from fractions import Fraction

CHECKER, SEED, COUNT, WORK = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
rnd = random.Random(SEED)
sys.setrecursionlimit(20000)
PRECISIONS = (80, 160)
EXPONENTS = (1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48)


class Unknown(Exception):
    pass


class Undefined(Exception):
    """Il punto non sta nel dominio naturale della funzione."""


# ---------------------------------------------------------------- lettura
def tokens(text):
    return re.findall(r"\(|\)|[^\s()]+", text)


def parse(tok, i):
    if tok[i] == "(":
        out, i = [], i + 1
        while tok[i] != ")":
            node, i = parse(tok, i)
            out.append(node)
        return out, i + 1
    return tok[i], i + 1


def tree(text):
    node, used = parse(tokens(text), 0)
    return node


def show(node):
    return node if isinstance(node, str) else "(" + " ".join(show(x) for x in node) + ")"


def read_certificates(paths):
    out = []
    for path in paths:
        for block in open(path, encoding="utf-8").read().split("certificate ")[1:]:
            if block.split("\n", 1)[0].strip() == "limit":
                out.append("certificate " + block.strip() + "\n")
    return out


def fields(text):
    """Le voci del certificato: variabile, punto, lato del punto, funzione,
    affermazione (tipo, valore, verità)."""
    lines = text.strip().splitlines()
    out = {"sides": []}
    for line in lines[1:]:
        word, _, rest = line.strip().partition(" ")
        if word == "variable":
            out["x"] = rest.strip()
        elif word == "point":
            rest = rest.strip()
            out["side"] = 0
            if rest in ("+infinity", "-infinity"):
                out["infinity"], out["point"] = (1 if rest[0] == "+" else -1), None
                continue
            out["infinity"] = 0
            for name, sign in ((" right", 1), (" left", -1)):
                if rest.endswith(name):
                    rest, out["side"] = rest[: -len(name)], sign
            out["point"] = tree(rest)
        elif word == "function":
            out["function"] = tree(rest)
        elif word == "claim":
            kind, _, rest = rest.strip().partition(" ")
            value, _, truth = rest.rpartition(" ")
            out["claim"] = kind
            out["value"] = tree(value) if kind == "value" else None
            out["truth"] = truth == "true"
        elif word == "side":
            out["sides"].append(rest.split())
    return out


# ---------------------------------------------------------------- i numeri
PI_CACHE = {}


def pi():
    """pi con la formula di Machin, alla precisione del contesto (calcolato una
    volta per precisione)."""
    ctx = decimal.getcontext()
    if ctx.prec in PI_CACHE:
        return +PI_CACHE[ctx.prec]
    prec = ctx.prec
    ctx.prec += 10

    def arctan_inverse(n):
        # Fino a un termine più piccolo della precisione: aspettare che sia
        # zero vorrebbe dire scendere fino a Emin.
        total, k, term, eps = D(0), 0, D(1) / n, D(10) ** (-ctx.prec - 5)
        while term > eps:
            total += term / (2 * k + 1) if k % 2 == 0 else -term / (2 * k + 1)
            term = term / (n * n)
            k += 1
        return total

    value = 16 * arctan_inverse(5) - 4 * arctan_inverse(239)
    ctx.prec = prec
    PI_CACHE[prec] = +value
    return +value


def d_sin_cos(x):
    # Riduzione modulo 2 pi, con le cifre che servono per un x grande.
    ctx = decimal.getcontext()
    extra = max(0, x.adjusted()) + 20
    ctx.prec += extra
    two_pi = 2 * pi()
    x = x - (x / two_pi).to_integral_value() * two_pi
    eps = D(10) ** (-ctx.prec - 5)
    term_s, term_c = x, D(1)
    s, c, k = term_s, term_c, 1
    while abs(term_s) > eps or abs(term_c) > eps:
        term_c = -term_c * x * x / ((2 * k - 1) * (2 * k))
        term_s = -term_s * x * x / ((2 * k) * (2 * k + 1))
        c += term_c
        s += term_s
        k += 1
        if k > 5000:
            raise Unknown("seno che non converge")
    ctx.prec -= extra
    return +s, +c


def d_atan(x):
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


def exact(node):
    """Il valore razionale esatto di un albero senza variabile fatto di interi
    e di + - * / neg, oppure None."""
    if isinstance(node, str):
        return Fraction(int(node)) if re.fullmatch(r"-?\d+", node) else None
    if node[0] in ("+", "-", "*", "/"):
        a, b = exact(node[1]), exact(node[2])
        if a is None or b is None or (node[0] == "/" and b == 0):
            return None
        return a + b if node[0] == "+" else a - b if node[0] == "-" else a * b if node[0] == "*" else a / b
    if node[0] == "neg":
        a = exact(node[1])
        return None if a is None else -a
    return None


def mentions(node, x):
    return node == x if isinstance(node, str) else any(mentions(child, x) for child in node[1:])


def real_power(a, q):
    """a^q per q razionale non intero: a > 0, oppure 0 con q > 0."""
    if a > 0:
        return (D(q.numerator) / D(q.denominator) * a.ln()).exp()
    if a == 0 and q > 0:
        return D(0)
    raise Undefined()


def ev(node, x, at):
    """Il valore dell'albero con la variabile x che vale at (Decimal)."""
    if isinstance(node, str):
        if node == x:
            return at
        if re.fullmatch(r"-?\d+", node):
            return D(int(node))
        raise Unknown("una lettera che non è la variabile: " + node)
    op = node[0]
    if op == "pi":
        return pi()
    args = node[1:]
    if op in ("+", "-", "*", "/"):
        a, b = ev(args[0], x, at), ev(args[1], x, at)
        if op == "/":
            if b == 0:
                raise Undefined()
            return a / b
        return a + b if op == "+" else a - b if op == "-" else a * b
    if op == "neg":
        return -ev(args[0], x, at)
    if op == "^":
        a = ev(args[0], x, at)
        q = None if mentions(args[1], x) else exact(args[1])
        if q is not None and q.denominator == 1:
            if a == 0 and q < 0:
                raise Undefined()
            if abs(q) > 1000:
                raise Unknown("esponente grande")
            return a ** int(q)
        if q is not None:
            return real_power(a, q)
        b = ev(args[1], x, at)
        if a <= 0:
            raise Undefined()
        return (b * a.ln()).exp()
    if op == "root":
        k = exact(args[0])  # (root m a): l'indice prima
        if k is None or k.denominator != 1 or k < 2:
            raise Unknown("indice di una radice")
        a = ev(args[1], x, at)
        if a < 0 and k % 2 == 0:
            raise Undefined()
        r = real_power(abs(a), Fraction(1, int(k)))
        return -r if a < 0 else r
    a = ev(args[0], x, at)
    if op == "exp":
        return a.exp()
    if op in ("ln", "lnabs"):
        if op == "lnabs":
            a = abs(a)
        if a <= 0:
            raise Undefined()
        return a.ln()
    if op == "abs":
        return abs(a)
    if op in ("sin", "cos", "tan", "cot"):
        s, c = d_sin_cos(a)
        if op == "sin":
            return s
        if op == "cos":
            return c
        below = c if op == "tan" else s
        if below == 0:
            raise Undefined()
        return (s if op == "tan" else c) / below
    if op == "atan":
        return d_atan(a)
    if op in ("arcsin", "arccos"):
        if abs(a) > 1:
            raise Undefined()
        value = 2 * d_atan(a / (1 + (1 - a * a).sqrt()))
        return value if op == "arcsin" else pi() / 2 - value
    raise Unknown("parola sconosciuta: " + op)


def value_at(node, x, at_text):
    """Il valore in due precisioni; None fuori dal dominio; Unknown se le due
    non sono d'accordo o se i numeri escono dal contesto. Un punto fuori dal
    dominio in tutte e due si ricontrolla a 400 cifre: uno zero può venire da
    una cancellazione più profonda di 160 cifre (cos 2x + e^{2x^2} - 2 in
    x = 10^{-48}), e allora non dice nulla del dominio."""
    results = []
    for prec in PRECISIONS + (400,):
        if prec == 400 and results != [None, None]:
            break
        ctx = decimal.Context(prec=prec, Emax=10 ** 9, Emin=-10 ** 9, traps=[decimal.Overflow, decimal.Underflow, decimal.InvalidOperation, decimal.DivisionByZero])
        decimal.setcontext(ctx)
        try:
            results.append(ev(node, x, D(at_text) if isinstance(at_text, str) else at_text()))
        except Undefined:
            results.append(None)
        except (decimal.Overflow, decimal.Underflow, decimal.InvalidOperation, decimal.DivisionByZero, OverflowError):
            raise Unknown("numeri fuori dal contesto")
    if len(results) == 3 and results[2] is not None:
        raise Unknown("uno zero che viene da una cancellazione")
    a, b = results[:2]
    if (a is None) != (b is None):
        raise Unknown("le due precisioni non sono d'accordo sul dominio")
    if a is None:
        return None
    decimal.setcontext(decimal.Context(prec=60, Emax=10 ** 9, Emin=-10 ** 9))
    if abs(a - b) > D(10) ** -30 * (1 + abs(b)):
        raise Unknown("le due precisioni non sono d'accordo")
    return b


def constant(node):
    """Il valore di un numero del certificato (il punto, il valore): None se
    non è un numero reale."""
    try:
        return value_at(node, "\0", "0")
    except Unknown:
        return "unknown"


# ---------------------------------------------------------------- l'oracolo
def side_values(f, x, point, toward):
    """I valori di f negli ultimi punti calcolabili da un lato: toward è +1 o
    -1 per i lati di un punto finito, +2 o -2 per +infinito e -infinito.
    Ritorna (valori, nessun valore negli ultimi tre punti)."""
    values, undefined = [], []
    for k in EXPONENTS:
        def at(k=k):
            if abs(toward) == 2:
                return D(10) ** k * (1 if toward > 0 else -1)
            return point + (D(10) ** -k) * toward
        try:
            v = value_at(f, x, at)
        except Unknown:
            continue
        undefined.append(v is None)
        if v is not None:
            values.append(v)
    nowhere = len(undefined) >= 3 and all(undefined[-3:])
    if not nowhere and undefined and any(undefined[-3:]):
        raise Unknown("il dominio cambia vicino al punto")
    return values, nowhere


def behaviour(values):
    """Dove vanno gli ultimi valori: ('finite', v, err), con il limite tra v -
    err e v + err, ('plus',), ('minus',), o Unknown. Prudente: un valore
    grande che cresce in fretta è un infinito; valori le cui differenze
    scendono almeno di un fattore 10 da un punto al successivo vanno a un
    numero, che dista dall'ultimo al più il doppio dell'ultima differenza
    (una serie geometrica); il resto non si giudica."""
    if len(values) < 3:
        raise Unknown("troppo pochi punti calcolabili")
    a, b, c = values[-3:]
    if abs(c) > D(10) ** 12 and abs(c) > 100 * abs(b) and (b > 0) == (c > 0):
        return ("plus",) if c > 0 else ("minus",)
    near, far = abs(c - b), abs(b - a)
    if near <= D(10) ** -4 * (1 + abs(c)) and (near * 10 <= far or near <= D(10) ** -25 * (1 + abs(c))):
        return ("finite", c, 2 * near + D(10) ** -25 * (1 + abs(c)))
    raise Unknown("i valori non si fermano e non crescono abbastanza")


def compare(a, b, err):
    """1 se a e b sono uguali entro err, 0 se sono lontani molto più di err,
    Unknown altrimenti."""
    gap = abs(a - b)
    if gap <= err:
        return 1
    if gap > 1000 * err + D(10) ** -9 * (1 + abs(b)):
        return 0
    raise Unknown("un limite è vicino a un altro numero ma non uguale")


def judge(text):
    """La verità di ciò che il certificato afferma, secondo l'oracolo: True,
    False, oppure Unknown."""
    c = fields(text)
    x, f = c["x"], c["function"]
    point = None
    if not c["infinity"]:
        point = constant(c["point"])
        if point is None:
            return False  # un punto che non è un numero
        if point == "unknown":
            raise Unknown("il punto non si calcola")
    value = None
    if c["claim"] == "value":
        value = constant(c["value"])
        if value is None:
            return False  # un valore che non è un numero
        if value == "unknown":
            raise Unknown("il valore non si calcola")
    towards = [2 * c["infinity"]] if c["infinity"] else [c["side"]] if c["side"] else [1, -1]
    sides = []
    for toward in towards:
        values, nowhere = side_values(f, x, point, toward)
        sides.append(None if nowhere else behaviour(values))
    defined = [s for s in sides if s is not None]
    if not defined:
        return False  # il limite non ha senso

    def same(a, b):
        if a[0] != b[0]:
            return False
        return a[0] != "finite" or compare(a[1], b[1], a[2] + b[2]) == 1

    kind = c["claim"]
    if kind == "value":
        holds = all(s[0] == "finite" and compare(s[1], value, s[2]) == 1 for s in defined)
    elif kind in ("plus", "minus"):
        holds = all(s[0] == kind for s in defined)
    elif kind == "unsigned":
        holds = all(s[0] != "finite" for s in defined)
    else:
        holds = len(defined) == 2 and not same(defined[0], defined[1])
    return holds == c["truth"]


# ---------------------------------------------------------------- le alterazioni
NON_REAL = ["(ln (neg (exp 1)))", "(ln (* -1 (exp 1)))", "(/ 1 (- 1 1))", "(root 2 -2)", "(ln 0)", "(^ -4 (/ 1 2))", "(arcsin 2)", "(tan (/ (pi) 2))"]
CONSTANTS = ["0", "1", "-1", "(exp 1)", "(pi)", "(ln 2)", "(/ 1 2)", "(exp 2)", "(root 2 2)", "(/ (pi) 2)", "(exp (/ -1 6))"]
SUBSTITUTIONS = ["a+t", "a-t", "1/t", "-1/t", "e^(1/t)", "-e^(1/t)", "e^(-1/t)", "-e^(-1/t)"]
WORDS = [("sin", "cos"), ("cos", "sin"), ("tan", "cot"), ("cot", "tan"), ("exp", "ln"), ("ln", "exp"), ("atan", "sin"), ("arcsin", "arccos"),
         ("arccos", "arcsin"), ("abs", "neg")]


def replace_line(text, word, new_line):
    lines = text.splitlines()
    for i, line in enumerate(lines):
        if line.startswith(word + " "):
            lines[i] = new_line
            return "\n".join(lines) + "\n"
    raise ValueError(word)


def line_of(text, word):
    for line in text.splitlines():
        if line.startswith(word + " "):
            return line[len(word) + 1:]
    raise ValueError(word)


def mutate_tree(node):
    """Un albero con un intero cambiato di uno, o una parola cambiata."""
    paths = []

    def walk(n, path):
        paths.append(path)
        if isinstance(n, list):
            for i, child in enumerate(n[1:], 1):
                walk(child, path + [i])

    walk(node, [])
    path = rnd.choice(paths)

    def rebuild(n, path):
        if not path:
            if isinstance(n, str):
                if re.fullmatch(r"-?\d+", n):
                    return str(int(n) + rnd.choice([-1, 1, 2]))
                return ["neg", n]
            for old, new in WORDS:
                if n[0] == old:
                    return [new] + n[1:]
            return ["neg", n]
        copy = list(n)
        copy[path[0]] = rebuild(n[path[0]], path[1:])
        return copy

    return rebuild(node, path)


def alter(text):
    choice = rnd.randrange(10)
    claim = line_of(text, "claim")
    kind, _, rest = claim.partition(" ")
    value, _, truth = rest.rpartition(" ")
    flipped = "claim %s%s %s" % (kind, " " + value if value else "", "false" if truth == "true" else "true")
    if choice == 0:
        return replace_line(text, "claim", flipped)
    if choice == 1 and kind == "value":
        v = value
        new = rnd.choice(["(+ %s 1)" % v, "(- %s 1)" % v, "(* 2 %s)" % v, "(neg %s)" % v, "(+ %s (/ 1 7))" % v, rnd.choice(CONSTANTS), rnd.choice(NON_REAL)])
        return replace_line(text, "claim", "claim value %s %s" % (new, truth))
    if choice == 2:
        new = rnd.choice(["plus", "minus", "unsigned", "none", "value " + rnd.choice(CONSTANTS + NON_REAL)])
        return replace_line(text, "claim", "claim %s %s" % (new, rnd.choice([truth, "true", "false"])))
    if choice == 3:
        lines = text.splitlines()
        sides = [i for i, line in enumerate(lines) if line.startswith("side ")]
        i = rnd.choice(sides)
        word, side, substitution, order = lines[i].split()
        if rnd.random() < 0.5:
            substitution = rnd.choice(SUBSTITUTIONS)
        else:
            order = str(rnd.choice([1, 2, 3, int(order) // 2 or 1, int(order) * 2]))
        lines[i] = " ".join([word, side, substitution, order])
        return "\n".join(lines) + "\n"
    if choice == 4:
        point = line_of(text, "point")
        if point.endswith(" right") or point.endswith(" left"):
            base, _, side = point.rpartition(" ")
            new = base + (" left" if side == "right" else " right")
            if rnd.random() < 0.5:
                text = text.replace("side %s " % side, "side %s " % ("left" if side == "right" else "right"))
            return replace_line(text, "point", "point " + new)
        if point in ("+infinity", "-infinity"):
            return replace_line(text, "point", "point " + ("-infinity" if point[0] == "+" else "+infinity"))
        new = rnd.choice(["(+ %s 1)" % point, "(neg %s)" % point, rnd.choice(CONSTANTS), rnd.choice(NON_REAL)])
        return replace_line(text, "point", "point " + new)
    if choice in (5, 6):
        return replace_line(text, "function", "function " + show(mutate_tree(tree(line_of(text, "function")))))
    if choice == 7:
        lines = text.splitlines()
        sides = [i for i, line in enumerate(lines) if line.startswith("side ")]
        i = rnd.choice(sides)
        if rnd.random() < 0.5 and len(sides) > 1:
            del lines[i]
        else:
            lines.insert(i, lines[i])
        return "\n".join(lines) + "\n"
    if choice == 8 and kind == "value":
        # Un valore vicino, che solo un confronto esatto distingue.
        return replace_line(text, "claim", "claim value (+ %s (/ 1 1000000)) %s" % (value, truth))
    return replace_line(text, "claim", flipped)


# ---------------------------------------------------------------- il verificatore
def run_checker(texts, name):
    path = os.path.join(WORK, name)
    with open(path, "w", encoding="utf-8") as out:
        out.write("".join(texts))
    result = subprocess.run([CHECKER, path], capture_output=True, text=True)
    answers = {}
    for line in result.stdout.splitlines():
        m = re.match(r"\s*(\d+)\. (ACCETTATO|RESPINTO|NON CONTROLLABILE)", line)
        if m:
            answers[int(m.group(1)) - 1] = m.group(2)
    return [answers.get(i, "?") for i in range(len(texts))]


def main():
    originals = read_certificates(sys.argv[5:])
    if not originals:
        print("no limit certificates in the given files")
        return 1
    os.makedirs(WORK, exist_ok=True)
    bugs = suspects = 0

    # I certificati del motore: accettati, e veri per l'oracolo.
    for i, (text, answer) in enumerate(zip(originals, run_checker(originals, "originals.cert"))):
        try:
            truth = judge(text)
        except (Unknown, ValueError, KeyError) as e:
            suspects += 1
            print("SUSPECT: original %d cannot be judged (%s)" % (i + 1, e))
            continue
        if not truth or answer != "ACCETTATO":
            bugs += 1
            print("BUG: original certificate %d is judged %s (checker: %s):\n%s" % (i + 1, truth, answer, text))

    # Le alterazioni.
    mutants = []
    for _ in range(COUNT):
        text = rnd.choice(originals)
        for _ in range(rnd.choice([1, 1, 2, 3])):
            try:
                text = alter(text)
            except (ValueError, IndexError):
                pass  # un'alterazione che non si applica al testo già alterato: lo lascia com'è
        mutants.append(text)
    answers = run_checker(mutants, "mutants_%d.cert" % SEED)
    tally = {}
    for text, answer in zip(mutants, answers):
        tally[answer] = tally.get(answer, 0) + 1
        if answer != "ACCETTATO":
            continue
        try:
            truth = judge(text)
        except (Unknown, ValueError, KeyError, IndexError) as e:
            suspects += 1
            print("SUSPECT: accepted certificate cannot be judged (%s):\n%s" % (e, text))
            continue
        if not truth:
            bugs += 1
            print("BUG: the checker accepted a certificate that claims something false:\n%s" % text)
    print("%d originals, %d mutants: %s; %d BUG, %d SUSPECT" % (len(originals), len(mutants), tally, bugs, suspects))
    return 1 if bugs else 0


if __name__ == "__main__":
    sys.exit(main())
