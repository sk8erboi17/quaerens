"""Fuzzing differenziale degli enunciati sulle successioni definite per ricorrenza.

Uso: python3 -I fuzz_recurrences.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Costruisce enunciati "valori iniziali e regola, allora tesi" di due tipi:

  - una formula per il termine n-esimo: ricorrenze di primo e di secondo
    ordine a coefficienti interi, somme di polinomi, a_{n+1} = a_n / (1 + c a_n),
    regole con n dentro come a_{n+1} = (n + c) a_n;
  - stime sui termini: radici annidate che crescono verso un limite, medie
    che convergono, il metodo di Erone per le radici quadrate.

Da quelli veri ricava varianti insidiose: un valore iniziale o un
coefficiente della regola ritoccato, una formula giusta solo nei primi 2, 3,
7 o 13 termini, n scambiato con n + 1, una stima stretta di pochissimo, il
verso di una monotonia rovesciato, un punto di partenza dall'altra parte del
limite. Metà degli enunciati è scritta a parole ("Let ... Then ..."), l'altra
metà in formula, con \\implies.

L'oracolo non usa il programma: calcola i termini uno dopo l'altro dalla
regola, con le frazioni esatte dove la regola è razionale e con 300 cifre
decimali dove c'è una radice, e li confronta con la tesi.

BUG: dimostrato un enunciato che l'oracolo trova falso o non definito in un
indice calcolato; oppure "falso per n = w" quando in w la tesi vale.
SUSPECT: l'oracolo non riesce a decidere un confronto.
"""
import os
import random
import re
import subprocess
import sys
from decimal import Decimal, getcontext
from fractions import Fraction as Fr

MR, SEED, COUNT, WORK = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
rnd = random.Random(SEED)
getcontext().prec = 300

EXACT_TERMS = 40    # termini calcolati con le frazioni
ROOT_TERMS = 60     # termini calcolati con i decimali, quando c'è una radice
TOO_LONG = 12000    # bit oltre i quali una frazione non viene più calcolata


class Undefined(Exception):
    """Un termine non è definito: divisione per zero, radice di un negativo."""


class Unsure(Exception):
    """L'oracolo non sa decidere: numeri troppo lunghi o troppo vicini."""


def N(v):
    return ("num", v)


n = ("var", "n")
add = lambda a, b: ("add", a, b)
sub = lambda a, b: ("sub", a, b)
mul = lambda a, b: ("mul", a, b)
div = lambda a, b: ("div", a, b)
pw = lambda a, e: ("pow", a, e)
term = lambda j: ("term", j)          # a_{n + j}


def tex(e, need=0):
    op = e[0]
    if op == "num":
        text, prec = str(e[1]), (4 if e[1] >= 0 else 1)
    elif op == "var":
        text, prec = e[1], 4
    elif op == "term":
        text, prec = "a_{n%s}" % ("" if e[1] == 0 else " %s %d" % ("+" if e[1] > 0 else "-", abs(e[1]))), 4
    elif op in ("add", "sub"):
        text, prec = "%s %s %s" % (tex(e[1], 1), "+" if op == "add" else "-", tex(e[2], 2)), 1
    elif op == "mul":
        text, prec = r"%s \cdot %s" % (tex(e[1], 2), tex(e[2], 3)), 2
    elif op == "div":
        text, prec = r"\frac{%s}{%s}" % (tex(e[1]), tex(e[2])), 4
    elif op == "pow":
        text, prec = "%s^{%d}" % (tex(e[1], 5 if e[1][0] in ("div", "term") else 4), e[2]), 3
    elif op == "powe":
        text, prec = "%s^{%s}" % (tex(e[1], 5 if e[1][0] == "div" else 4), tex(e[2])), 3
    elif op == "sqrt":
        text, prec = r"\sqrt{%s}" % tex(e[1]), 4
    elif op == "fact":
        text, prec = "%s!" % tex(e[1], 4), 3
    else:
        raise ValueError(op)
    return r"\left(%s\right)" % text if prec < need else text


def dec(v):
    """v in decimali: è il solo punto in cui l'oracolo lascia le frazioni."""
    return v if isinstance(v, Decimal) else Decimal(v.numerator) / Decimal(v.denominator)


def value(e, at, terms):
    """Il valore di e con n = at; terms[m] è il termine di indice m. Una
    frazione finché non c'è una radice, poi un decimale."""
    op = e[0]
    if op == "num":
        return Fr(e[1])
    if op == "var":
        return Fr(at)
    if op == "term":
        if at + e[1] not in terms:
            raise Undefined
        return terms[at + e[1]]
    a = value(e[1], at, terms)
    if op == "pow":
        if e[2] < 0 and a == 0:
            raise Undefined
        return a ** e[2]
    if op == "sqrt":
        if a < 0:
            raise Undefined
        return dec(a).sqrt()
    if op == "fact":
        if isinstance(a, Decimal) or a.denominator != 1 or a < 0:
            raise Undefined
        out = 1
        for i in range(2, int(a) + 1):
            out *= i
        return Fr(out)
    b = value(e[2], at, terms)
    if isinstance(a, Decimal) != isinstance(b, Decimal):
        a, b = dec(a), dec(b)
    if op == "add":
        return a + b
    if op == "sub":
        return a - b
    if op == "mul":
        return a * b
    if op == "div":
        if b == 0:
            raise Undefined
        return a / b
    if op == "powe":
        if isinstance(b, Decimal) or b.denominator != 1 or abs(b) > 400:
            raise Unsure
        if b < 0 and a == 0:
            raise Undefined
        return a ** int(b)
    raise ValueError(op)


def compare(a, rel, b):
    """Il confronto a rel b. Tra frazioni è esatto; con i decimali due
    numeri troppo vicini non vengono giudicati."""
    if isinstance(a, Decimal) or isinstance(b, Decimal):
        a, b = dec(a), dec(b)
        if abs(a - b) <= (abs(a) + abs(b) + 1) * Decimal(10) ** -200:
            raise Unsure
    return {"=": a == b, "<": a < b, ">": a > b, r"\le": a <= b, r"\ge": a >= b}[rel]


def too_long(v):
    return isinstance(v, Fr) and v.numerator.bit_length() + v.denominator.bit_length() > TOO_LONG


def oracle(case):
    """('true', None), ('false', m), ('undefined', m), ('unsure', m) o
    ('ill-posed', None): calcola i termini dalla regola, dal primo indice in
    su, e controlla le tesi in ogni indice dal caso base."""
    first, inits, shift, rule, claims, base = case["first"], case["inits"], case["shift"], case["rule"], case["claims"], case["base"]
    if case.get("ill"):
        return "ill-posed", None
    terms = {first + j: Fr(v) for j, v in enumerate(inits)}
    rooted = "sqrt" in tex(rule)
    count = ROOT_TERMS if rooted else EXACT_TERMS
    reach = max(max(spread(c[0]) + spread(c[2])) for c in claims)
    try:
        for m in range(first + len(inits), first + count + reach + 1):
            terms[m] = value(rule, m - shift, terms)
            if too_long(terms[m]):
                count = m - first - reach - 1
                break
    except Undefined:
        return "undefined", m
    except Unsure:
        return "unsure", m
    for m in range(base, first + count):
        for lhs, rel, rhs in claims:
            try:
                if not compare(value(lhs, m, terms), rel, value(rhs, m, terms)):
                    return "false", m
            except Undefined:
                return "undefined", m
            except Unsure:
                return "unsure", m
    return "true", None


def spread(e):
    """Gli spostamenti j dei termini a_{n + j} che compaiono in e."""
    if e[0] == "term":
        return [e[1]]
    return [j for c in e[1:] if isinstance(c, tuple) for j in spread(c)] or [0]


def falling(first, count):
    e = sub(n, N(first))
    for j in range(first + 1, first + count):
        e = mul(e, sub(n, N(j)))
    return e


def closed_case():
    """Una successione con la formula del termine n-esimo, vera."""
    kind = rnd.choice(["affine", "affine", "roots", "double", "poly", "rational", "factorial", "half"])
    c, s = rnd.choice([2, 3, -2, 4]), rnd.randint(-3, 5)
    if kind == "affine":
        f = rnd.randint(-3, 3)
        return dict(first=0, inits=[s], shift=1, rule=add(mul(N(c), term(0)), N(f * (1 - c))),
                    claims=[(term(0), "=", add(mul(N(s - f), ("powe", N(c), n)), N(f)))])
    if kind == "roots":
        r1, r2 = rnd.sample([1, 2, 3, -1, -2, 5], 2)
        al, be = rnd.randint(1, 3), rnd.randint(-2, 3)
        return dict(first=0, inits=[al + be, al * r1 + be * r2], shift=2,
                    rule=sub(mul(N(r1 + r2), term(1)), mul(N(r1 * r2), term(0))),
                    claims=[(term(0), "=", add(mul(N(al), ("powe", N(r1), n)), mul(N(be), ("powe", N(r2), n))))])
    if kind == "double":
        r, al, be = rnd.choice([2, 3, -2]), rnd.randint(1, 3), rnd.randint(-2, 3)
        return dict(first=0, inits=[al, (al + be) * r], shift=2, rule=sub(mul(N(2 * r), term(1)), mul(N(r * r), term(0))),
                    claims=[(term(0), "=", mul(add(N(al), mul(N(be), n)), ("powe", N(r), n)))])
    if kind == "poly":
        p, q, r = rnd.randint(1, 3), rnd.randint(-3, 3), rnd.randint(-3, 3)
        first = rnd.choice([0, 1])
        # a_n = p n^2 + q n + r, quindi a_{n+1} - a_n = 2 p n + p + q.
        return dict(first=first, inits=[p * first * first + q * first + r], shift=1,
                    rule=add(term(0), add(mul(N(2 * p), n), N(p + q))),
                    claims=[(term(0), "=", add(add(mul(N(p), pw(n, 2)), mul(N(q), n)), N(r)))])
    if kind == "rational":
        s, c = rnd.randint(1, 4), rnd.randint(1, 3)
        return dict(first=1, inits=[Fr(1, s)], shift=1, rule=div(term(0), add(N(1), mul(N(c), term(0)))),
                    claims=[(term(0), "=", div(N(1), add(N(s), mul(N(c), sub(n, N(1))))))])
    if kind == "factorial":
        return dict(first=0, inits=[1], shift=1, rule=mul(add(n, N(1)), term(0)), claims=[(term(0), "=", ("fact", n))])
    f = rnd.randint(-2, 4)
    return dict(first=0, inits=[s], shift=1, rule=add(div(term(0), N(2)), div(N(f), N(2))),
                claims=[(term(0), "=", add(N(f), div(N(s - f), ("powe", N(2), n))))])


def bound_case():
    """Una successione con stime vere sui termini."""
    kind = rnd.choice(["nested", "nested", "mean up", "mean down", "heron", "square", "inverse"])
    if kind == "nested":
        limit = rnd.choice([2, 3, 4, 5])
        return dict(first=0, inits=[rnd.choice([0, 1])], shift=1, rule=("sqrt", add(N(limit * limit - limit), term(0))),
                    claims=[(term(0), "<", N(limit)), (term(0), "<", term(1))], limit=limit)
    if kind.startswith("mean"):
        limit = rnd.randint(1, 5)
        start = limit - rnd.randint(1, 3) if kind == "mean up" else limit + rnd.randint(1, 3)
        rel = "<" if kind == "mean up" else ">"
        return dict(first=0, inits=[start], shift=1, rule=div(add(term(0), N(limit)), N(2)),
                    claims=[(term(0), rel, N(limit)), (term(0), rel, term(1))], limit=limit)
    if kind == "heron":
        c = rnd.choice([2, 3, 5])
        return dict(first=0, inits=[rnd.choice([2, 3])], shift=1, rule=div(add(term(0), div(N(c), term(0))), N(2)),
                    claims=[(pw(term(0), 2), r"\ge", N(c)), (term(1), r"\le", term(0))], limit=None)
    if kind == "square":
        return dict(first=0, inits=[Fr(1, rnd.choice([2, 3]))], shift=1, rule=pw(term(0), 2),
                    claims=[(term(0), ">", N(0)), (term(0), "<", N(1)), (term(1), "<", term(0))], limit=None)
    return dict(first=0, inits=[1], shift=1, rule=div(N(1), add(N(1), term(0))),
                claims=[(term(0), ">", N(0)), (term(0), r"\le", N(1))], limit=None)


FLIP = {"<": ">", ">": "<", r"\le": r"\ge", r"\ge": r"\le", "=": "="}


def shift_n(e):
    """e con n + 1 al posto di n."""
    if e == n:
        return add(n, N(1))
    if e[0] in ("num", "var", "term"):
        return e
    return (e[0],) + tuple(shift_n(c) if isinstance(c, tuple) else c for c in e[1:])


def perturb(case):
    """Una variante che può essere falsa, o non definire la successione."""
    case = dict(case, inits=list(case["inits"]), claims=list(case["claims"]))
    closed = case["claims"][0][1] == "="
    kind = rnd.choice(["init", "rule", "hidden", "hidden", "next", "early", "since", "drop"] if closed else
                      ["tight", "tight", "flip", "start", "strict", "rule", "early"])
    lhs, rel, rhs = case["claims"][0]
    if kind == "init":
        case["inits"][rnd.randrange(len(case["inits"]))] += rnd.choice([1, -1, 2])
    elif kind == "rule":
        case["rule"] = add(case["rule"], N(rnd.choice([1, -1]))) if rnd.random() < 0.5 else mul(N(2), case["rule"])
    elif kind == "hidden":
        case["claims"][0] = (lhs, rel, add(rhs, mul(N(rnd.choice([1, 3])), falling(case["first"], rnd.choice([2, 3, 7, 13])))))
    elif kind == "next":
        case["claims"][0] = (lhs, rel, shift_n(rhs))
    elif kind == "early":
        case["base"] = case["first"] - 1
    elif kind == "since":
        case["since"] = case["first"] - (case["shift"] - len(case["inits"])) + rnd.choice([1, 2])
        case["ill"] = True
    elif kind == "drop" and len(case["inits"]) > 1:
        case["inits"].pop()
        case["ill"] = True
    elif kind == "tight":
        if case.get("limit") is None:
            return case
        gap = Fr(1, rnd.choice([10, 1000, 10 ** 6, 10 ** 12]))
        bound = Fr(case["limit"]) - gap if rel == "<" else Fr(case["limit"]) + gap
        case["claims"][0] = (lhs, rel, div(N(bound.numerator), N(bound.denominator)))
    elif kind == "flip":
        i = rnd.randrange(len(case["claims"]))
        case["claims"][i] = (case["claims"][i][0], FLIP[case["claims"][i][1]], case["claims"][i][2])
    elif kind == "start":
        case["inits"][0] = case["inits"][0] + rnd.choice([3, 6, -4])
    elif kind == "strict":
        i = rnd.randrange(len(case["claims"]))
        swap = {"<": r"\le", ">": r"\ge", r"\le": "<", r"\ge": ">"}
        case["claims"][i] = (case["claims"][i][0], swap[case["claims"][i][1]], case["claims"][i][2])
    return case


def number(v):
    v = Fr(v)
    return str(v.numerator) if v.denominator == 1 else r"\frac{%d}{%d}" % (v.numerator, v.denominator)


def write(case, words):
    """L'\\item dell'enunciato: a parole oppure in formula."""
    first, shift = case["first"], case["shift"]
    low = shift - len(case["inits"]) if not case.get("ill") else min(spread(case["rule"]))
    givens = ["a_{%d} = %s" % (first + j, number(v)) for j, v in enumerate(case["inits"])]
    givens.append("a_{n%s} = %s" % ("" if shift == 0 else " + %d" % shift, tex(case["rule"])))
    claims = ["%s %s %s" % (tex(l), r, tex(h)) for l, r, h in case["claims"]]
    since = case.get("since")
    if words:
        text = "Let " + ", ".join("$%s$" % g for g in givens[:-1]) + " and $%s$" % givens[-1]
        text += (" for $n\\ge %d$" % since if since is not None else "") + ". Then "
        text += " and ".join("$%s$" % c for c in claims)
        return "\\item %s for every integer $n\\ge %d$.\n" % (text, case["base"])
    if since is not None:
        givens.append("n \\ge %d" % since)
    return "\\item \\[ %s \\implies %s \\] for every integer $n\\ge %d$.\n" % (", ".join(givens), " \\land ".join(claims), case["base"])


def main():
    cases = []
    for _ in range(COUNT):
        case = closed_case() if rnd.random() < 0.55 else bound_case()
        case["base"] = case["first"]
        if rnd.random() < 0.65:
            case = perturb(case)
        cases.append((case, rnd.random() < 0.5))
    path = os.path.join(WORK, "fuzz_recurrences_%d.tex" % SEED)
    with open(path, "w", encoding="utf-8") as out:
        out.write("\\begin{enumerate}\n")
        for case, words in cases:
            out.write(write(case, words))
        out.write("\\end{enumerate}\n")
    run = subprocess.run([MR, path], capture_output=True, text=True, timeout=3600)
    lines = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
    if len(lines) != len(cases) or run.returncode not in (0, 1):
        print("BUG: the program answered %d of %d statements (exit %d)\n%s" % (len(lines), len(cases), run.returncode, run.stderr[-2000:]))
        return
    tally = {}
    for index, ((case, words), line) in enumerate(zip(cases, lines), 1):
        text = line.split(". ", 1)[1]
        verdict = "P" if text.startswith("DIMOSTRATO") else "R" if text.startswith("FALSO") else "N" if text.startswith("NON DIMOSTRATO") else "U"
        truth, where = oracle(case)
        if case["base"] < case["first"] and truth != "ill-posed":
            truth = "ill-posed"
        tally[(truth, verdict)] = tally.get((truth, verdict), 0) + 1
        problem = None
        if truth == "unsure":
            problem = "SUSPECT: the oracle cannot decide index %d" % where
        elif verdict == "P" and truth != "true":
            problem = "BUG: proved, but the statement is %s%s" % (truth, "" if where is None else " at index %d" % where)
        elif verdict == "R":
            w = int(re.search(r"= (-?\d+)$", text).group(1))
            if truth == "true" or truth == "ill-posed" or (truth == "false" and w < where):
                problem = "BUG: refuted at n = %d, but the oracle finds the claims true there" % w
        elif verdict in "NU" and truth == "true":
            problem = "NOTE: true wherever the oracle computed it, but not proved"
        if problem:
            print("%s\n   %d [%s]: %s" % (problem, index, text[:70], write(case, words).strip()[:400]))
    print("seed %d:" % SEED, ", ".join("%s/%s=%d" % (k[0], k[1], v) for k, v in sorted(tally.items())))


main()
