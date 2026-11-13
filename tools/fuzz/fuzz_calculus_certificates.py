"""Fuzzing del verificatore sui certificati delle derivate, delle primitive e degli integrali (capitolo 13 di checker.c).

Uso: python3 -I fuzz_calculus_certificates.py <checker> <seme> <quanti> <cartella di lavoro> <file di certificati>...

I file di certificati sono quelli scritti da mathresearcher --certificates: se
ne prendono i certificati derivative, antiderivative e integral, e se ne
ricavano <quanti> alterati a caso, con da una a tre alterazioni ciascuno: un
numero, un operatore, una funzione al posto di un'altra, un termine tolto o
aggiunto, la variabile spostata, l'indice di una radice, l'ordine o il caso
base, un estremo, gli estremi scambiati, il valore; e due alterazioni
"plausibili": x al posto di 2x da tutte e due le parti, e per un integrale
un altro intervallo con il valore F(b') - F(a') scritto apposta, che solo il
controllo dell'intervallo può fermare. Il verificatore li
controlla tutti, e per ognuno che ACCETTA l'oracolo decide da sé se ciò che il
certificato afferma è vero.

L'oracolo. Non deriva nulla con le regole: calcola le espressioni con i
numeri decimali di Python a molte cifre (exp, ln e le radici della libreria
decimal; pi greco con la formula di Machin, seno e coseno con la serie di
Taylor, l'arcotangente dimezzando l'angolo), e
  - per una derivata di ordine k confronta G(x) con la differenza centrale
    di ordine k di F, con passo h = 10^-30 e 40 + 30 k cifre (l'errore del
    troncamento è circa h^2, quello degli arrotondamenti 10^-40), in punti
    razionali a caso in cui F è definita in tutto lo schema e G in x; per una
    derivata n-esima con n dal caso base a tre ordini dopo;
  - per una primitiva fa lo stesso con k = 1;
  - per un integrale controlla che f e F siano definite in 101 punti di
    [a, b], estremi compresi, e nei multipli di 1/12 che vi cadono, che
    F' = f in cinque punti, e confronta il valore con la formula di
    Gauss-Legendre a 10 nodi su 64 e su 128 parti.
Le altre lettere (i parametri) prendono valori razionali a caso.

BUG: il verificatore ha accettato un certificato che afferma il falso (o un
certificato del motore, non alterato, che l'oracolo smentisce).
SUSPECT: un certificato accettato che l'oracolo non sa decidere (numeri troppo
vicini per dire, una quadratura che non converge).
VACUOUS (solo contati): un certificato accettato in cui, nei punti provati, le
due espressioni non sono mai definite insieme, come ln(-x) e ln x: il teorema
non dice nulla, ed è vero.
I file prodotti vanno nella cartella di lavoro, mai nel repository.
"""
import os
import random
import re
import subprocess
import sys
from decimal import Decimal, localcontext
from fractions import Fraction

CHECKER, SEED, COUNT, WORK = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
rnd = random.Random(SEED)
sys.setrecursionlimit(20000)


class Unknown(Exception):
    pass


class Undefined(Exception):
    pass


class Vacuous(Exception):
    """Nessun punto provato in cui le due espressioni siano definite insieme: il teorema non dice nulla, ed è vero."""
    pass


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
    return parse(tokens(text), 0)[0]


def show(node):
    return node if isinstance(node, str) else "(" + " ".join(show(x) for x in node) + ")"


VOICES = ["variable", "order", "lower", "upper", "function", "derivative", "integrand", "primitive", "value"]


def read_certificate(block):
    """Le voci di un certificato, come dizionario; brick e end restano fuori."""
    lines = block.strip().split("\n")
    kind = lines[0].split()[1]
    voices = {"kind": kind}
    for line in lines[1:]:
        word = line.split(" ", 1)[0]
        if word in VOICES:
            voices[word] = line.split(" ", 1)[1]
    return voices


def write_certificate(v):
    out = ["certificate " + v["kind"], "variable " + v["variable"]]
    if v["kind"] == "derivative":
        out += ["order " + v["order"], "function " + v["function"], "derivative " + v["derivative"]]
    else:
        if v["kind"] == "integral":
            out += ["lower " + v["lower"], "upper " + v["upper"]]
        out += ["integrand " + v["integrand"], "primitive " + v["primitive"]]
        if v["kind"] == "integral":
            out.append("value " + v["value"])
    return "\n".join(out + ["end"]) + "\n"


def read_certificates(paths):
    out = []
    for path in paths:
        text = open(path, encoding="utf-8").read()
        for block in re.split(r"(?=^certificate )", text, flags=re.M):
            head = block.split("\n", 1)[0].strip()
            if head in ("certificate derivative", "certificate antiderivative", "certificate integral"):
                out.append(read_certificate(block))
    return out


# ---------------------------------------------------------------- i numeri
PI_CACHE = {}


def pi_value():
    """pi con la formula di Machin, pi/4 = 4 arctan(1/5) - arctan(1/239), una volta per precisione."""
    from decimal import getcontext
    precision = getcontext().prec
    if precision not in PI_CACHE:
        PI_CACHE[precision] = machin_pi()
    return +PI_CACHE[precision]


def machin_pi():
    def arctan_inverse(n):
        x, total, k, sign = Decimal(1) / n, Decimal(0), 1, 1
        term = x
        n2 = n * n
        while True:
            piece = term / k
            if piece == 0:
                break
            total += sign * piece
            term /= n2
            k += 2
            sign = -sign
        return total
    with localcontext() as c:
        c.prec += 10
        value = 4 * (4 * arctan_inverse(5) - arctan_inverse(239))
    return +value


def sin_cos(x):
    """Seno e coseno: x ridotto in [-pi, pi], poi la serie di Taylor."""
    with localcontext() as c:
        c.prec += 20
        two_pi = 2 * pi_value()
        x = x - two_pi * (x / two_pi).to_integral_value()
        s, co, term, k = Decimal(0), Decimal(0), Decimal(1), 0
        while True:
            if k % 2 == 0:
                piece = term * (1 if k % 4 == 0 else -1)
                co += piece
            else:
                piece = term * (1 if k % 4 == 1 else -1)
                s += piece
            if abs(term) < Decimal(10) ** (-(c.prec + 5)) and k > 4:
                break
            k += 1
            term = term * x / k
            if k > 10000:
                raise Unknown("serie troppo lunga")
    return +s, +co


def arctan(x):
    """arctan x = 2 arctan(x / (1 + sqrt(1 + x^2))), finché x è piccolo; poi la serie."""
    with localcontext() as c:
        c.prec += 20
        halvings = 0
        while abs(x) > Decimal("0.1"):
            x = x / (1 + (1 + x * x).sqrt())
            halvings += 1
        total, term, k, x2 = Decimal(0), x, 1, x * x
        while term != 0 and abs(term) > Decimal(10) ** (-(c.prec + 5)):
            total += term / k if (k // 2) % 2 == 0 else -term / k
            term *= x2
            k += 2
        result = total * (2 ** halvings)
    return +result


def is_integer_value(d):
    return d == d.to_integral_value()


def mentions(node, letter):
    if isinstance(node, str):
        return node == letter
    return any(mentions(child, letter) for child in node[1:])


def evaluate(node, env, x, regular):
    """Il valore di node con le lettere di env; regular chiede anche radicandi e argomenti dei valori assoluti diversi da zero."""
    if isinstance(node, str):
        if re.fullmatch(r"-?\d+", node):
            return Decimal(int(node))
        if node in env:
            return env[node]
        raise Unknown("lettera senza valore: " + node)
    op, args = node[0], node[1:]
    if op == "pi":
        return pi_value()
    if op in ("sum", "prod"):
        index, low, high, body = args[0], evaluate(args[1], env, x, regular), evaluate(args[2], env, x, regular), args[3]
        if not (is_integer_value(low) and is_integer_value(high)):
            raise Undefined()
        total = Decimal(0 if op == "sum" else 1)
        for k in range(int(low), int(high) + 1):
            inner = dict(env)
            inner[index] = Decimal(k)
            value = evaluate(body, inner, x, regular)
            total = total + value if op == "sum" else total * value
        return total
    vals = [evaluate(a, env, x, regular) for a in args]
    if op == "+":
        return vals[0] + vals[1]
    if op == "-":
        return vals[0] - vals[1]
    if op == "*":
        return vals[0] * vals[1]
    if op == "neg":
        return -vals[0]
    if op == "/":
        if vals[1] == 0:
            raise Undefined()
        return vals[0] / vals[1]
    if op == "^":
        a, b = vals
        if not mentions(args[1], x) and is_integer_value(b):
            if b < 0 and a == 0:
                raise Undefined()
            if abs(b) > 600:
                raise Unknown("esponente troppo grande")
            return a ** int(b)
        if a <= 0:
            raise Undefined()
        return (b * a.ln()).exp()
    if op == "exp":
        if vals[0] > 10000:
            raise Unknown("esponenziale troppo grande")
        return vals[0].exp()
    if op == "ln":
        if vals[0] <= 0:
            raise Undefined()
        return vals[0].ln()
    if op == "lnabs":
        if vals[0] == 0:
            raise Undefined()
        return abs(vals[0]).ln()
    if op == "abs":
        if regular and vals[0] == 0:
            raise Undefined()
        return abs(vals[0])
    if op in ("sin", "cos"):
        s, c = sin_cos(vals[0])
        return s if op == "sin" else c
    if op == "atan":
        return arctan(vals[0])
    if op == "root":
        m, a = int(args[0]), vals[1]
        if a < 0 and m % 2 == 0:
            raise Undefined()
        if a == 0:
            if regular:
                raise Undefined()
            return Decimal(0)
        magnitude = (abs(a).ln() / m).exp()
        return magnitude if a > 0 else -magnitude
    if op == "fact":
        if not is_integer_value(vals[0]) or vals[0] < 0 or vals[0] > 300:
            raise Undefined()
        result = Decimal(1)
        for k in range(2, int(vals[0]) + 1):
            result *= k
        return result
    raise Unknown("parola sconosciuta all'oracolo: " + op)


def binomial(k, j):
    result = 1
    for i in range(j):
        result = result * (k - i) // (i + 1)
    return result


def derivative_at(function, env, x, point, k):
    """La differenza centrale di ordine k di function in point: errore circa h^2."""
    h = Decimal(10) ** -30
    total = Decimal(0)
    for j in range(k + 1):
        inner = dict(env)
        inner[x] = point + (Decimal(k) / 2 - j) * h
        total += (-1) ** j * binomial(k, j) * evaluate(function, inner, x, True)
    return total / h ** k


def compare(a, b):
    """1 se a e b coincidono, -1 se sono di sicuro diversi, 0 se non si sa dire."""
    size = 1 + max(abs(a), abs(b))
    gap = abs(a - b)
    if gap <= Decimal(10) ** -22 * size:
        return 1
    if gap > Decimal(10) ** -8 * size:
        return -1
    return 0


def random_point():
    return Decimal(rnd.randint(-60, 60)) / rnd.choice([7, 9, 10, 11, 13])


def parameters(v):
    letters = set(re.findall(r"(?<![a-z])([a-z])(?![a-z])", " ".join(str(v.get(k, "")) for k in ("function", "derivative", "integrand", "primitive", "value", "lower", "upper"))))
    letters.discard(v["variable"])
    order = v.get("order", "").split()
    if len(order) == 3:
        letters.discard(order[0])
    return sorted(letters)


def judge_derivative_like(v, function, claim, orders, letter):
    """Vero se in ogni punto valido F^(k) = G; Unknown se non c'è nessun punto da guardare."""
    x = v["variable"]
    seen = defined = 0
    for _ in range(2):
        env = {p: Decimal(rnd.randint(-9, 9) or 1) / rnd.choice([2, 3, 5]) for p in parameters(v)}
        for k, n in orders:
            if letter:
                env[letter] = Decimal(n)
            for _ in range(40):
                point = random_point()
                with localcontext() as c:
                    c.prec = 40 + 30 * k
                    try:
                        inner = dict(env)
                        inner[x] = point
                        expected = evaluate(claim, inner, x, False)
                        found = derivative_at(function, env, x, point, k)
                    except Undefined:
                        continue
                    except (ArithmeticError, OverflowError):
                        continue
                    defined += 1
                    agreement = compare(found, expected)
                if agreement < 0:
                    return False, "%s = %s, %s" % (x, point, "ordine %d" % k)
                seen += agreement > 0
                if agreement > 0:
                    break
    if not defined:
        raise Vacuous("nessun punto in cui le due espressioni siano definite insieme")
    if not seen:
        raise Unknown("i numeri non bastano a confrontare le due espressioni")
    return True, ""


GAUSS = None


def gauss_nodes():
    """I nodi e i pesi di Gauss-Legendre a 10 punti su [-1, 1], con Newton sul polinomio di Legendre."""
    global GAUSS
    if GAUSS:
        return GAUSS
    import math
    n, nodes = 10, []
    with localcontext() as c:
        c.prec = 60
        for i in range(1, n + 1):
            t = Decimal(math.cos(math.pi * (i - 0.25) / (n + 0.5)))
            for _ in range(100):
                p0, p1 = Decimal(1), t
                for k in range(2, n + 1):
                    p0, p1 = p1, ((2 * k - 1) * t * p1 - (k - 1) * p0) / k
                dp = n * (t * p1 - p0) / (t * t - 1)
                step = p1 / dp
                t -= step
                if abs(step) < Decimal(10) ** -55:
                    break
            nodes.append((t, 2 / ((1 - t * t) * dp * dp)))
    GAUSS = nodes
    return nodes


def quadrature(f, env, x, a, b, parts):
    total = Decimal(0)
    width = (b - a) / parts
    for i in range(parts):
        middle = a + width * (Decimal(i) + Decimal("0.5"))
        for t, w in gauss_nodes():
            inner = dict(env)
            inner[x] = middle + width / 2 * t
            total += w * evaluate(f, inner, x, False)
    return total * width / 2


def judge_integral(v):
    x = v["variable"]
    f, F = tree(v["integrand"]), tree(v["primitive"])
    with localcontext() as c:
        c.prec = 40
        try:
            a, b = evaluate(tree(v["lower"]), {}, x, False), evaluate(tree(v["upper"]), {}, x, False)
            value = evaluate(tree(v["value"]), {}, x, False)
        except Undefined:
            return False, "un estremo o il valore non è un numero"
        low, high = min(a, b), max(a, b)
        # 101 punti in fila, e i multipli di 1/12 tra gli estremi, dove
        # cadono di solito i poli delle funzioni dei test.
        grid = [low + (high - low) * i / 100 for i in range(101)]
        grid += [Decimal(k) / 12 for k in range(int(low * 12) - 1, int(high * 12) + 2) if low <= Decimal(k) / 12 <= high]
        for point in grid:
            try:
                evaluate(f, {x: point}, x, False)
                evaluate(F, {x: point}, x, True)
            except Undefined:
                return False, "f o F non è definita in %s = %s, dentro l'intervallo" % (x, point)
    for i in range(5):
        point = low + (high - low) * Decimal(rnd.randint(1, 99)) / 100
        with localcontext() as c:
            c.prec = 70
            try:
                if compare(derivative_at(F, {}, x, point, 1), evaluate(f, {x: point}, x, False)) < 0:
                    return False, "F' diversa da f in %s = %s" % (x, point)
            except Undefined:
                pass
    with localcontext() as c:
        c.prec = 40
        coarse, fine = quadrature(f, {}, x, a, b, 64), quadrature(f, {}, x, a, b, 128)
        size = 1 + abs(fine)
        if abs(coarse - fine) > Decimal(10) ** -12 * size:
            raise Unknown("la quadratura non converge abbastanza")
        gap = abs(value - fine)
        if gap > Decimal(10) ** -6 * size:
            return False, "il valore %s non è l'integrale %s" % (value, fine)
        if gap > Decimal(10) ** -10 * size:
            raise Unknown("valore e quadratura troppo vicini per dire")
    return True, ""


def judge(v):
    """(vero, perché): ciò che il certificato v afferma, deciso dall'oracolo."""
    if v["kind"] == "integral":
        return judge_integral(v)
    if v["kind"] == "antiderivative":
        return judge_derivative_like(v, tree(v["primitive"]), tree(v["integrand"]), [(1, 0)], None)
    order = v["order"].split()
    if len(order) == 1:
        k = int(order[0])
        return judge_derivative_like(v, tree(v["function"]), tree(v["derivative"]), [(k, 0)], None)
    letter, base = order[0], int(order[2])
    orders = [(n, n) for n in range(base, min(base + 4, 9))]
    return judge_derivative_like(v, tree(v["function"]), tree(v["derivative"]), orders, letter)


# ---------------------------------------------------------------- le alterazioni
SWAPS = {"+": ["-", "*"], "-": ["+", "/"], "*": ["+", "/"], "/": ["*", "-"], "sin": ["cos"], "cos": ["sin"], "exp": ["ln", "sin"],
         "ln": ["lnabs", "exp"], "lnabs": ["ln"], "atan": ["sin", "exp"]}


def subtrees(node, path=()):
    yield path, node
    if not isinstance(node, str):
        for i, child in enumerate(node[1:], 1):
            yield from subtrees(child, path + (i,))


def replace(node, path, new):
    if not path:
        return new
    copy = list(node)
    copy[path[0]] = replace(node[path[0]], path[1:], new)
    return copy


def alter_tree(node, x):
    places = list(subtrees(node))
    path, sub = rnd.choice(places)
    choice = rnd.random()
    if isinstance(sub, str) and re.fullmatch(r"-?\d+", sub):
        k = int(sub)
        return replace(node, path, str(rnd.choice([k + 1, k - 1, -k, 2 * k, k + 2])))
    if isinstance(sub, str):
        return replace(node, path, rnd.choice([["+", sub, "1"], ["*", "2", sub], ["neg", sub], x if sub != x else "1"]))
    if sub[0] == "root" and choice < 0.5:
        return replace(node, path, ["root", str(int(sub[1]) + rnd.choice([1, -1 if int(sub[1]) > 2 else 1])), sub[2]])
    if sub[0] in SWAPS and choice < 0.4:
        return replace(node, path, [rnd.choice(SWAPS[sub[0]])] + sub[1:])
    if len(sub) == 3 and sub[0] in "+-*/" and choice < 0.7:
        return replace(node, path, sub[rnd.choice([1, 2])])
    return replace(node, path, rnd.choice([["+", sub, "1"], ["*", "2", sub], ["neg", sub], ["+", sub, x], ["*", sub, x]]))


def substitute(node, x, new):
    if isinstance(node, str):
        return new if node == x else node
    return [node[0]] + [substitute(child, x, new) for child in node[1:]]


BOUNDS = ["-3", "-2", "-1", "0", "1", "2", "3", "4", "5", "(/ 1 2)", "(/ -1 2)", "(/ 7 2)"]


def alter(v):
    v = dict(v)
    x = v["variable"]
    choices = ["integrand", "primitive", "both"] if v["kind"] != "derivative" else ["function", "derivative", "order", "both"]
    if v["kind"] == "integral":
        choices += ["value", "lower", "upper", "swap", "move", "move"]
    what = rnd.choice(choices)
    if what == "both":
        # x -> 2x da tutte e due le parti: la derivata di F(2x) è 2 F'(2x).
        for key in ("function", "derivative", "integrand", "primitive"):
            if key in v:
                v[key] = show(substitute(tree(v[key]), x, ["*", "2", x]))
    elif what == "move":
        # Un altro intervallo, con il valore F(b') - F(a') scritto apposta:
        # lo ferma solo il controllo che f e F siano definite su [a', b'].
        v["lower"], v["upper"] = rnd.sample(BOUNDS, 2)
        F = tree(v["primitive"])
        v["value"] = show(["-", substitute(F, x, tree(v["upper"])), substitute(F, x, tree(v["lower"]))])
    elif what == "order":
        words = v["order"].split()
        if len(words) == 1:
            words[0] = str(max(0, int(words[0]) + rnd.choice([1, -1])))
        else:
            words[2] = str(max(0, int(words[2]) + rnd.choice([1, -1])))
        v["order"] = " ".join(words)
    elif what == "swap":
        v["lower"], v["upper"] = v["upper"], v["lower"]
    else:
        v[what] = show(alter_tree(tree(v[what]), x))
    return v


# ---------------------------------------------------------------- il giro
def run_checker(texts, name):
    path = os.path.join(WORK, name)
    with open(path, "w", encoding="utf-8") as out:
        for t in texts:
            out.write(t + "\n")
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
        print("no derivative, antiderivative or integral certificates in the given files")
        return 1
    os.makedirs(WORK, exist_ok=True)
    bugs = suspects = vacuous = 0

    # I certificati del motore: accettati, e veri per l'oracolo.
    texts = [write_certificate(v) for v in originals]
    for i, (v, answer) in enumerate(zip(originals, run_checker(texts, "originals.cert"))):
        try:
            truth, why = judge(v)
        except Vacuous:
            vacuous += 1
            continue
        except Unknown as e:
            suspects += 1
            print("SUSPECT: original %d cannot be judged (%s)" % (i + 1, e))
            continue
        if not truth:
            bugs += 1
            print("BUG: original certificate %d claims something false (%s; checker: %s):\n%s" % (i + 1, why, answer, texts[i]))

    # Le alterazioni.
    mutants = []
    for _ in range(COUNT):
        v = rnd.choice(originals)
        for _ in range(rnd.choice([1, 1, 2, 3])):
            v = alter(v)
        mutants.append(v)
    texts = [write_certificate(v) for v in mutants]
    tally = {}
    for v, text, answer in zip(mutants, texts, run_checker(texts, "mutants_%d.cert" % SEED)):
        tally[answer] = tally.get(answer, 0) + 1
        if answer != "ACCETTATO":
            continue
        try:
            truth, why = judge(v)
        except Vacuous:
            vacuous += 1
            continue
        except (Unknown, ArithmeticError, ValueError, IndexError, KeyError) as e:
            suspects += 1
            print("SUSPECT: accepted certificate cannot be judged (%s):\n%s" % (e, text))
            continue
        if not truth:
            bugs += 1
            print("BUG: the checker accepted a certificate that claims something false (%s):\n%s" % (why, text))
    print("%d originals, %d mutants: %s; %d BUG, %d SUSPECT, %d VACUOUS" % (len(originals), len(mutants), tally, bugs, suspects, vacuous))
    return 1 if bugs else 0


if __name__ == "__main__":
    sys.exit(main())
