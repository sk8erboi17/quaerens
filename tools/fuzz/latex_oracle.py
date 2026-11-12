"""Un secondo lettore di LaTeX, indipendente dal programma, e due modi di usarlo.

  python3 -I latex_oracle.py syntax <eseguibile> <seme> <quanti> <cartella>
      Genera espressioni scritte in modo compatto e ambiguo (prodotti
      sottintesi, esponenti senza graffe, argomenti senza parentesi), le legge
      con il parser di questo file e chiede al programma di dimostrare
      "espressione = la stessa espressione, tutta tra parentesi". Se il
      programma non la dimostra, i due lettori hanno capito cose diverse.

  python3 -I latex_oracle.py check <eseguibile> <file .tex>...
      Fa decidere al programma gli enunciati dei file e ricontrolla
      numericamente ogni verdetto per induzione: DIMOSTRATO deve valere per i
      primi n dal caso base, FALSO per n = w deve cadere proprio in w.

Il parser segue la grammatica scritta nel commento del capitolo 9 di main.c,
non il suo codice.
"""
import math
import os
import random
import re
import subprocess
import sys
from fractions import Fraction as Fr


class Unsupported(Exception):
    pass


class Undefined(Exception):
    pass


IGNORED = {"displaystyle", "textstyle", "quad", "qquad", "limits"}
FUNCTIONS = {"sin", "cos", "tan", "cot", "arctan", "ln", "log"}


def tokenize(text):
    tokens, i = [], 0
    while i < len(text):
        c = text[i]
        if c.isspace():
            i += 1
        elif c.isdigit():
            j = i
            while j < len(text) and text[j].isdigit():
                j += 1
            tokens.append(("NUM", text[i:j]))
            i = j
        elif c.isalpha():
            tokens.append(("LET", c))
            i += 1
        elif c == "\\":
            i += 1
            if i < len(text) and not text[i].isalpha():
                if text[i] in ",;!: ":
                    i += 1
                    continue
                if text[i] in "{}":
                    tokens.append(("CH", "\\" + text[i]))
                    i += 1
                    continue
                raise Unsupported("command")
            j = i
            while j < len(text) and text[j].isalpha():
                j += 1
            name = text[i:j]
            i = j
            if name in ("left", "right"):
                while i < len(text) and text[i].isspace():
                    i += 1
                d = text[i] if i < len(text) else ""
                if d == "|":
                    tokens.append(("ABSO" if name == "left" else "ABSC", d))
                elif name == "left" and d in "([" and d:
                    tokens.append(("OPEN", d))
                elif name == "right" and d in ")]" and d:
                    tokens.append(("CLOSE", d))
                else:
                    raise Unsupported("delimiter")
                i += 1
            elif name not in IGNORED:
                tokens.append(("CMD", name))
        else:
            tokens.append(("OPEN", c) if c == "(" else ("CLOSE", c) if c == ")" else ("CH", c))
            i += 1
    tokens.append(("END", ""))
    return tokens


class Parser:
    def __init__(self, text):
        self.t, self.i = tokenize(text), 0

    def peek(self):
        return self.t[self.i]

    def take(self):
        self.i += 1
        return self.t[self.i - 1]

    def is_ch(self, c):
        return self.peek() == ("CH", c)

    def is_cmd(self, *names):
        return self.peek()[0] == "CMD" and self.peek()[1] in names

    def expect_ch(self, c):
        if not self.is_ch(c):
            raise Unsupported("expected " + c)
        self.take()

    def group(self):
        if self.is_ch("{"):
            self.take()
            e = self.expr()
            self.expect_ch("}")
            return e
        kind, value = self.peek()
        if kind == "NUM" and len(value) > 1:
            self.t[self.i] = ("NUM", value[1:])
            return ("num", int(value[0]))
        return self.primary()

    def starts_factor(self):
        kind, value = self.peek()
        return kind in ("NUM", "LET", "OPEN", "ABSO") or (kind == "CMD" and value in FUNCTIONS | {
            "frac", "dfrac", "tfrac", "binom", "sqrt", "sum", "prod", "pi"})

    def primary(self):
        kind, value = self.take()
        if kind == "NUM":
            return ("num", int(value))
        if kind == "LET":
            if self.is_ch("_"):
                self.take()
                index = self.group()
                if value == "F":
                    return ("fib", index)
                if value == "H":
                    return ("harm", index)
                raise Unsupported("sequence")
            return ("var", value)
        if kind == "OPEN":
            e = self.expr()
            if self.take()[0] != "CLOSE":
                raise Unsupported("paren")
            return e
        if kind == "ABSO":
            e = self.expr()
            if self.take()[0] != "ABSC":
                raise Unsupported("abs")
            return ("abs", e)
        if kind != "CMD":
            raise Unsupported("primary")
        if value in ("frac", "dfrac", "tfrac"):
            a = self.group()
            return ("div", a, self.group())
        if value == "binom":
            a = self.group()
            return ("binom", a, self.group())
        if value == "sqrt":
            index = ("num", 2)
            if self.is_ch("["):
                self.take()
                index = self.expr()
                self.expect_ch("]")
            return ("rootn", self.group(), index)
        if value in ("sum", "prod"):
            self.expect_ch("_")
            self.expect_ch("{")
            kind2, letter = self.take()
            if kind2 != "LET":
                raise Unsupported("index")
            self.expect_ch("=")
            lo = self.expr()
            self.expect_ch("}")
            self.expect_ch("^")
            hi = self.group()
            return (value, letter, lo, hi, self.term())
        if value in FUNCTIONS:
            exponent = base = None
            while True:
                if self.is_ch("^") and exponent is None:
                    self.take()
                    exponent = self.group()
                elif self.is_ch("_") and value == "log" and base is None:
                    self.take()
                    base = self.group()
                else:
                    break
            if self.peek()[0] == "OPEN":
                self.take()
                argument = self.expr()
                if self.take()[0] != "CLOSE":
                    raise Unsupported("paren")
            else:
                argument = self.factor()
            call = ("log", argument, base) if value == "log" else (value, argument)
            if value == "log" and base is None:
                raise Unsupported("log base")
            return ("powe", call, exponent) if exponent is not None else call
        if value == "pi":
            return ("pi",)
        raise Unsupported("command " + value)

    def factor(self):
        e = self.primary()
        while True:
            if self.is_ch("!"):
                self.take()
                if self.is_ch("!"):
                    raise Unsupported("double factorial")
                e = ("fact", e)
            elif self.is_ch("^"):
                self.take()
                e = ("powe", e, self.group())
            else:
                return e

    def term(self):
        e = self.factor()
        while True:
            if self.is_cmd("cdot", "times"):
                self.take()
                e = ("mul", e, self.factor())
            elif self.is_ch("/"):
                self.take()
                e = ("div", e, self.factor())
            elif self.starts_factor():
                e = ("mul", e, self.factor())
            else:
                return e

    def expr(self):
        if self.is_ch("-"):
            self.take()
            e = ("neg", self.term())
        else:
            if self.is_ch("+"):
                self.take()
            e = self.term()
        while True:
            if self.is_ch("+"):
                self.take()
                e = ("add", e, self.term())
            elif self.is_ch("-"):
                self.take()
                e = ("sub", e, self.term())
            else:
                return e

    def statement(self):
        lhs = self.expr()
        if not self.is_ch("="):
            raise Unsupported("relation")
        self.take()
        rhs = self.expr()
        if self.peek()[0] != "END":
            raise Unsupported("trailing")
        return lhs, rhs


def whole(v):
    if isinstance(v, Fr) and v.denominator == 1:
        return v.numerator
    raise Undefined


def val(e, env):
    op = e[0]
    try:
        if op == "num":
            return Fr(e[1])
        if op == "var":
            return env[e[1]]
        if op == "pi":
            return math.pi
        if op in ("sum", "prod"):
            lo, hi = whole(val(e[2], env)), whole(val(e[3], env))
            if hi - lo > 3000:
                raise Undefined
            total = Fr(0) if op == "sum" else Fr(1)
            inner = dict(env)
            for i in range(lo, hi + 1):
                inner[e[1]] = Fr(i)
                term = val(e[4], inner)
                total = total + term if op == "sum" else total * term
            return total
        a = val(e[1], env)
        if op == "neg":
            return -a
        if op == "abs":
            return abs(a)
        if op == "fact":
            i = whole(a)
            if i < 0 or i > 80:
                raise Undefined
            return Fr(math.factorial(i))
        if op == "fib":
            i = whole(a)
            p, q = 0, 1
            if abs(i) > 300:
                raise Undefined
            for _ in range(abs(i)):
                p, q = q, p + q
            return Fr(p if i >= 0 or i % 2 else -p)   # F(-m) = (-1)^(m+1) F(m)
        if op == "harm":
            i = whole(a)
            if i < 0:
                raise Undefined
            return sum((Fr(1, j) for j in range(1, i + 1)), Fr(0))
        if op in ("sin", "cos", "tan", "cot", "arctan", "ln"):
            a = float(a)
            if op == "sin":
                return math.sin(a)
            if op == "cos":
                return math.cos(a)
            if op == "tan":
                return math.tan(a)
            if op == "cot":
                return math.cos(a) / math.sin(a)
            if op == "arctan":
                return math.atan(a)
            if a <= 0:
                raise Undefined
            return math.log(a)
        b = val(e[2], env)
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
        if op == "log":
            if float(a) <= 0 or float(b) <= 0 or float(b) == 1:
                raise Undefined
            return math.log(float(a)) / math.log(float(b))
        if op == "rootn":
            m = whole(b)
            if m < 2 or a < 0:
                raise Undefined
            return float(a) ** (1.0 / m)
        if op == "powe":
            if isinstance(b, Fr) and b.denominator == 1 and isinstance(a, Fr):
                if abs(b.numerator) > 600 or (a == 0 and b < 0):
                    raise Undefined
                return a ** b.numerator
            if float(a) <= 0:
                raise Undefined
            return float(a) ** float(b)
        if op == "binom":
            r = whole(b)
            if r < 0 or r > 60:
                raise Undefined
            out = Fr(1)
            for j in range(1, r + 1):
                out = out * (a - r + j) / j
            return out
    except (OverflowError, ZeroDivisionError, ValueError, KeyError):
        raise Undefined
    raise ValueError(op)


def same(l, r):
    if isinstance(l, Fr) and isinstance(r, Fr):
        return l == r
    try:
        l, r = float(l), float(r)
    except OverflowError:
        return False
    return abs(l - r) <= 1e-8 * max(1.0, abs(l), abs(r))


def letters(e, bound=()):
    """Le lettere libere di un'espressione."""
    if e[0] == "var":
        return set() if e[1] in bound else {e[1]}
    if e[0] in ("sum", "prod"):
        return letters(e[2], bound) | letters(e[3], bound) | letters(e[4], bound + (e[1],))
    out = set()
    for c in e[1:]:
        if isinstance(c, tuple):
            out |= letters(c, bound)
    return out


def compare(lhs, rhs, variable, first, last):
    """(n in cui i membri differiscono o None, numero di punti in cui sono definiti)."""
    params = sorted((letters(lhs) | letters(rhs)) - {variable})
    defined = 0
    for i in range(first, last + 1):
        for trial in range(3 if params else 1):
            env = dict((p, [1.37, 0.61, 2.23, 3.1, 0.47, 1.9][(k + trial) % 6]) for k, p in enumerate(params))
            env[variable] = Fr(i)
            try:
                l, r = val(lhs, env), val(rhs, env)
            except Undefined:
                continue
            defined += 1
            if not same(l, r):
                return i, defined
    return None, defined


def paren(e):
    """L'espressione scritta senza alcuna ambiguità: ogni operazione tra parentesi."""
    op = e[0]
    P = lambda s: r"\left(%s\right)" % s
    if op == "num":
        return str(e[1])
    if op == "var":
        return e[1]
    if op == "pi":
        return r"\pi"
    if op in ("add", "sub"):
        return P("%s %s %s" % (paren(e[1]), "+" if op == "add" else "-", paren(e[2])))
    if op == "mul":
        return P(r"%s \cdot %s" % (paren(e[1]), paren(e[2])))
    if op == "div":
        return r"\frac{%s}{%s}" % (paren(e[1]), paren(e[2]))
    if op == "neg":
        return P("-%s" % paren(e[1]))
    if op == "powe":
        return P("%s^{%s}" % (P(paren(e[1])), paren(e[2])))
    if op == "fact":
        return P("%s!" % P(paren(e[1])))
    if op == "rootn":
        return r"\sqrt[%s]{%s}" % (paren(e[2]), paren(e[1]))
    if op in ("sin", "cos", "tan", "cot", "arctan", "ln"):
        return r"\%s\left(%s\right)" % (op, paren(e[1]))
    if op == "log":
        return r"\log_{%s}\left(%s\right)" % (paren(e[2]), paren(e[1]))
    if op == "binom":
        return r"\binom{%s}{%s}" % (paren(e[1]), paren(e[2]))
    if op == "fib":
        return "F_{%s}" % paren(e[1])
    if op == "harm":
        return "H_{%s}" % paren(e[1])
    if op == "abs":
        return r"\left|%s\right|" % paren(e[1])
    if op in ("sum", "prod"):
        return P(r"\%s_{%s=%s}^{%s} %s" % (op, e[1], paren(e[2]), paren(e[3]), P(paren(e[4]))))
    raise ValueError(op)


def run_engine(mr, statements, path):
    with open(path, "w", encoding="utf-8") as out:
        out.write("\\begin{enumerate}\n")
        for s in statements:
            out.write("\\item $\\displaystyle %s$\n" % s)
        out.write("\\end{enumerate}\n")
    done = subprocess.run([mr, path], capture_output=True, text=True, errors="replace", timeout=1800)
    lines = [l.split(". ", 1)[1] for l in done.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
    if len(lines) != len(statements):
        raise SystemExit("the program answered %d of %d statements" % (len(lines), len(statements)))
    return lines


# --- Modo "syntax": espressioni compatte e ambigue ------------------------------
def compact(rnd, depth):
    """Una espressione scritta nel modo più compatto, senza badare a come verrà letta."""
    atoms = ["n", "n", "2", "3", "5", "12", "x", "n!", "3!", r"\frac12", r"\frac{n}{2}", r"\frac1n", "(n + 1)", "(n - 2)",
             r"\left(n + 3\right)", "n^2", "n^{2}", "2^n", "x^3", "n^12", "2^{n}3", r"\binom{n}{2}", r"\binom n2", r"\sqrt{4}",
             r"\sqrt 9", "F_n", "F_{n}", "F_n+1", r"\pi", r"\sin x", r"\cos^2 x", r"\sin^{2}(x)", r"\ln n", r"\ln n^2", r"\ln 2n",
             r"\sin 2x", r"\sin x y", r"\sin(x)y", r"\log_2 8", r"\log_{2} n", "H_n", r"\sum_{k=1}^{n} k", r"\sum_{k=1}^n k^2",
             r"\sum_{k=1}^{n} 2 k", r"\sum_{k=1}^{n} k 2 k", r"\prod_{k=1}^{n} 2", r"\sum_{k=1}^{n} \frac1k", "-n", "(-1)^n",
             r"\left(-1\right)^{n} n", "n(n+1)", "2n(n+1)/2", "n/2", "1/2n", "n/2n", "2/n/3", "n^2!", "n!^2", "2n!", "(2n)!", r"x^2^3"]
    if depth == 0:
        return rnd.choice(atoms)
    kind = rnd.random()
    a, b = compact(rnd, depth - 1), compact(rnd, depth - 1)
    if kind < 0.3:
        return "%s %s %s" % (a, rnd.choice(["+", "-", "+", "-", r"\cdot", "/", ""]), b)
    if kind < 0.45:
        return "%s %s" % (a, b)
    if kind < 0.55:
        return "(%s)%s" % (a, rnd.choice(["^2", "^{3}", "!", "", "/2", r" \cdot 3"]))
    if kind < 0.65:
        return r"\frac{%s}{%s}" % (a, b)
    if kind < 0.72:
        return "-%s" % a
    if kind < 0.8:
        return r"\sum_{k=1}^{n} %s" % rnd.choice(["k", "k^2 + 1", "2k - 1", "k k", r"\frac{1}{k}", "k!", "(-1)^k k", "k + " + a])
    if kind < 0.88:
        return "%s^%s" % (a, rnd.choice(["2", "{2}", "3", "{n}", "12"]))
    return "%s - %s + %s" % (a, b, compact(rnd, depth - 1))


def mode_syntax(mr, seed, count, work):
    rnd = random.Random(seed)
    cases = []
    while len(cases) < count:
        s = compact(rnd, rnd.choice([0, 1, 1, 2, 2, 3]))
        try:
            tree = Parser(s + " = 0").statement()[0]
        except Unsupported:
            continue
        cases.append((s, tree))
    lines = run_engine(mr, ["%s = %s" % (s, paren(tree)) for s, tree in cases], os.path.join(work, "syntax_%d.tex" % seed))
    tally = {}
    for (s, tree), text in zip(cases, lines):
        kind = "P" if text.startswith("DIMOSTRATO") else "R" if text.startswith("FALSO") else "N" if text.startswith("NON DIMOSTRATO") else "U"
        tally[kind] = tally.get(kind, 0) + 1
        if kind in ("R", "N"):
            print("MISMATCH [%s]\n   %s\n   = %s" % (text[:70], s, paren(tree)))
        elif kind == "U" and ("simbolo" in text or "manca" in text or "testo inatteso" in text or "incompleta" in text):
            print("REJECTED [%s]\n   %s" % (text[:70], s))
    print("seed %d:" % seed, tally)


# --- Modo "check": ricontrollo numerico dei verdetti ------------------------------
def mode_check(mr, paths):
    for path in paths:
        text = open(path, encoding="utf-8", errors="replace").read()
        statements = [s.strip() for s in re.findall(r"\\item \$\\displaystyle (.*?)\$", text, re.S)]
        statements += [s.strip() for s in re.findall(r"\\item\s*\\\[(.*?)\\\]", text, re.S)]
        lines = run_engine(mr, statements, path + ".check")
        os.remove(path + ".check")
        tally = {"P": 0, "R": 0, "checked": 0, "unreadable": 0}
        for s, verdict in zip(statements, lines):
            proved = re.match(r"DIMOSTRATO per induzione su (\w) >= (-?\d+)", verdict)
            refuted = re.match(r"FALSO: non vale per (\w) = (-?\d+)", verdict)
            if not proved and not refuted:
                continue
            tally["P" if proved else "R"] += 1
            try:
                lhs, rhs = Parser(s).statement()
            except (Unsupported, IndexError):
                tally["unreadable"] += 1
                print("UNREADABLE by the oracle [%s]\n   %s" % (verdict[:50], s[:300]))
                continue
            m = proved or refuted
            variable, at = m.group(1), int(m.group(2))
            if proved:
                where, defined = compare(lhs, rhs, variable, at, at + 9)
                if where is not None:
                    print("BUG: proved from %s >= %d but the sides differ at %d\n   %s" % (variable, at, where, s[:400]))
            else:
                where, defined = compare(lhs, rhs, variable, at, at)
                if where is None and defined:
                    print("BUG: refuted at %s = %d but the sides coincide there\n   %s" % (variable, at, s[:400]))
            tally["checked"] += 1 if defined else 0
        print(os.path.basename(path), tally)


if sys.argv[1] == "syntax":
    mode_syntax(sys.argv[2], int(sys.argv[3]), int(sys.argv[4]), sys.argv[5])
else:
    mode_check(sys.argv[2], sys.argv[3:])
