"""Fuzzing di robustezza: enunciati rovinati apposta e casi estremi.

Uso: python3 -I fuzz_crash.py <eseguibile con i sanitizer> <seme> <quanti> <cartella di lavoro> <file .tex>...

Prende gli enunciati dei file dati, li altera (tagli, ripetizioni, simboli
inseriti a caso), aggiunge casi estremi costruiti apposta (annidamenti
profondi, catene lunghissime) e fa girare il programma a blocchi. Segnala:

  - un errore dei sanitizer o un'uscita anomala;
  - un blocco che non finisce in tempo;
  - un enunciato che da solo impiega più di qualche secondo.

Non giudica i verdetti: un enunciato alterato è un altro enunciato.
"""
import os
import random
import re
import subprocess
import sys
import time

MR, SEED, COUNT, WORK = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
rnd = random.Random(SEED)
TOKENS = ["(", ")", "{", "}", "\\{", "\\}", "\\left(", "\\right)", "\\left|", "\\right|", "^", "_", "=", "+", "-", "!", "/", ",",
          "\\frac", "\\sqrt", "\\sum", "\\prod", "\\binom", "\\ln", "\\log", "\\sin", "\\arctan", "\\pi", "\\cdot",
          "\\forall", "\\exists", "\\in", "\\mathbb{N}", "\\mathbb{Z}", "\\mathbb{Q}", "\\mathbb", "\\setminus", "\\colon", "\\to",
          "\\land", "\\lor", "\\implies", "\\iff", "\\lnot", "\\neq", "\\cup", "\\cap", "\\times", "\\mathcal{P}", "\\emptyset",
          "\\subseteq", "\\triangle", "0", "1", "2", "7", "99999999999999999999", "n", "k", "x", "f", "S", "T", "F_", "\\", "$", " ",
          "\\mathbb{R}", "\\mathbb{R}^{+}", "<", ">", "\\le", "\\ge", "\\mid", ":", "\\begin{cases}", "\\end{cases}", "\\\\", "|"]


def corpus(paths):
    out = []
    for path in paths:
        text = open(path, encoding="utf-8").read()
        out += re.findall(r"\\item \$\\displaystyle (.*?)\$", text, re.S)
        out += re.findall(r"\\item\s*\\\[(.*?)\\\]", text, re.S)
    return [s.strip() for s in out if s.strip()]


def mutate(s):
    for _ in range(rnd.choice([1, 1, 2, 3, 5])):
        kind = rnd.random()
        i = rnd.randrange(len(s) + 1)
        j = min(len(s), i + rnd.choice([1, 1, 2, 5, 20]))
        if kind < 0.3:
            s = s[:i] + s[j:]
        elif kind < 0.5:
            s = s[:i] + s[i:j] * rnd.choice([2, 3]) + s[j:]
        elif kind < 0.9:
            s = s[:i] + " " + rnd.choice(TOKENS) + " " + s[i:]
        else:
            a = rnd.randrange(len(s) + 1)
            s = s[:min(i, a)] + s[max(i, a):] + s[min(i, a):max(i, a)]
        if not s:
            s = "x"
    return s.replace("$", " ")[:4000]


def extremes():
    N = r"\mathbb{N}"
    deep = lambda d: "(" * d + "n" + ")" * d
    cases = [
        deep(1500) + " = n",
        deep(900) + "^{2} = n^{2}",
        r"\frac{" * 600 + "n" + "}{2}" * 600 + " = n",
        r"\sqrt{" * 400 + "n" + "}" * 400 + " = n",
        "n" + "^{2}" * 500 + " = n",
        "2" + "^{n" * 300 + "}" * 300 + " = n",
        " + ".join(["n"] * 1300) + " = 1300 n",
        r" \cdot ".join(["n"] * 900) + " = n^{900}",
        r"\sum_{k=1}^{n} " * 60 + "1 = n",
        r"\sum_{k=1}^{n} " + " + ".join(["k^{%d}" % (i % 9) for i in range(200)]) + " = n",
        r"\prod_{k=1}^{n} \frac{k + 1}{k} " * 40 + " = n + 1",
        r"\sum_{k=1}^{100000000} k = n",
        r"\sum_{k=1}^{n} k^{400} = n",
        r"\binom{n}{300} = n",
        r"\binom{n}{" + deep(300) + "} = 1",
        "n!" + "!" * 50 + " = n",
        r"\left(n + 1\right)^{511} = \left(n + 1\right)^{511}",
        r"\left(n + x + y + z\right)^{60} = \left(n + x + y + z\right)^{60}",
        r"\left(a + b + c + d + e + g + h + m\right)^{12} = 1",
        r"\sin\left(" * 200 + "x" + r"\right)" * 200 + " = x",
        r"\ln\left(" * 200 + "n" + r"\right)" * 200 + " = n",
        r"\arctan\left(" * 100 + "n" + r"\right)" * 100 + " = n",
        r"F_{" * 300 + "n" + "}" * 300 + " = n",
        r"\sqrt[" + "9" * 30 + "]{n} = n",
        r"n^{" + "9" * 18 + "} = n",
        " \\cup ".join(["S", "T", "V", "W"] * 300) + " = S",
        r"\mathcal{P}(" * 400 + "S" + ")" * 400 + " = S",
        "(" * 1200 + "S" + ")" * 1200 + r" \subseteq S",
        r"\lnot " * 1500 + "(S = S)",
        r"\exists V " * 300 + "(V = S)",
        " \\land ".join(["(S \\subseteq T)"] * 500),
        " \\iff ".join(["S = T"] * 400),
        " \\implies ".join(["S \\subseteq T"] * 800),
        r"\forall x \in %s\ " % N + "(" * 1200 + "x = x" + ")" * 1200,
        r"\lnot " * 1500 + r"\forall x \in %s\ (x = x)" % N,
        "".join(r"\forall %s \in %s\ " % (v, N) for v in "abcdeghm") + "(a = a)",
        "".join(r"\exists %s \in %s\ " % (v, N) for v in "abcdeghm") + "(a + b + c + d + e + g + h + m = 999983)",
        "".join(r"\forall %s \in %s\ \exists %s \in %s\ " % (u, N, v, N) for u, v in ["ab", "cd", "eg", "hm"]) + "(b = a + c + e + h \\land d = c \\land g = e \\land m = h)",
        r"\forall x \in %s\ (" % N + " \\land ".join(["x = x"] * 600) + ")",
        r"\forall x \in %s\ (" % N + " \\lor ".join(["x \\neq %d" % i for i in range(300)]) + ")",
        r"\forall x \in %s\ (" % N + " \\iff ".join(["x = 1"] * 30) + ")",
        r"\forall x \in %s \setminus \{" % N + ", ".join(str(i) for i in range(1, 60)) + r"\}\ (x = x)",
        r"\forall x \in %s" % N + r" \setminus \{1\}" * 300 + r"\ (x = x)",
        r"\forall x \in \mathbb{Q}\ (x^{16} + " + " + ".join("%d x^{%d}" % (10 ** 8 + i, i) for i in range(1, 16)) + r" + 720720 \neq 0)",
        r"\forall x \in \mathbb{Q}\ (720720 x^{16} \neq 367567200)",
        r"\exists x \in \mathbb{Q}\ (" + " + ".join(["x"] * 800) + " = 1)",
        r"(f \colon %s \to %s \land \forall n \in %s\ (f(n) = %s)) \implies \forall a \in %s\ (f(a) = f(a))" % (N, N, N, deep(800), N),
        r"(f \colon %s \to %s \land \forall n \in %s\ (f(n) = n^{2})) \implies \forall a \in %s\ (%s = f(a))" % (N, N, N, N, "f(" * 1 + "a" + ")"),
        "".join(r"(%s \colon %s \to %s \land \forall n \in %s\ (%s(n) = n + 1)) \land " % (c, N, N, N, c) for c in "fghpq")[:-7] + r" \implies \forall a \in %s\ (f(a) = g(a))" % N,
        r"\forall x \in \mathbb{Q}\ \exists y \in \mathbb{Q}\ (" + " \\land ".join(["y = x + %d - %d" % (i, i) for i in range(200)]) + ")",
        "",
        "=",
        "\\",
        "{",
        "}" * 50,
        r"\mathbb",
        r"\forall",
        r"\frac{1}{0} = \frac{1}{0}",
        r"\frac{n}{n - n} = 1",
        r"\sum_{k=5}^{n} 1 = n - 4",
        r"\sum_{k=-5}^{n} 1 = n + 6",
        r"\sum_{k=1}^{-n} 1 = 0",
        r"\sum_{k=1}^{n - 100} 1 = n - 100",
        r"\sum_{k=1}^{1000000 n} 1 = 1000000 n",
        r"\sum_{k=1}^{n^{2}} 1 = n^{2}",
        r"\prod_{k=1}^{n} 0 = 0",
        r"\prod_{k=0}^{n} k = 0",
        r"\sum_{n=1}^{n} n = n",
        r"\sum_{k=1}^{k} k = k",
        r"\sum_{k=1}^{n} \sum_{k=1}^{k} 1 = n",
    ]
    return cases


def run(statements, name, timeout):
    path = os.path.join(WORK, name)
    with open(path, "w", encoding="utf-8") as out:
        out.write("\\begin{enumerate}\n")
        for s in statements:
            out.write("\\item $\\displaystyle %s$\n" % s)
        out.write("\\end{enumerate}\n")
    start = time.time()
    try:
        done = subprocess.run([MR, path], capture_output=True, text=True, errors="replace", timeout=timeout)
    except subprocess.TimeoutExpired:
        return "timeout", time.time() - start, ""
    bad = done.returncode not in (0, 1) or "Sanitizer" in done.stderr or "runtime error" in done.stderr
    answered = len([l for l in done.stdout.splitlines() if re.match(r"\s*\d+\. ", l)])
    if not bad and answered != len(statements):
        return "answered %d of %d" % (answered, len(statements)), time.time() - start, done.stdout[-300:]
    return ("crash" if bad else "ok"), time.time() - start, done.stderr[-1500:]


def narrow(statements, label):
    """Ritrova, uno per uno, gli enunciati che fanno fallire o rallentano un blocco."""
    for s in statements:
        status, elapsed, detail = run([s], "single.tex", 60)
        if status != "ok" or elapsed > 5:
            print("%s: %s in %.1fs\n   %s\n   %s" % (label, status.upper(), elapsed, s[:300], detail.strip()[-600:]))


def main():
    base = corpus(sys.argv[5:])
    statements = extremes() if SEED == 0 else []
    while len(statements) < COUNT:
        statements.append(mutate(rnd.choice(base)))
    problems = 0
    for start in range(0, len(statements), 100):
        chunk = statements[start:start + 100]
        status, elapsed, detail = run(chunk, "crash_%d_%d.tex" % (SEED, start), 300)
        if status != "ok" or elapsed > 40:
            problems += 1
            print("block %d: %s in %.1fs" % (start, status, elapsed))
            narrow(chunk, "seed %d" % SEED)
    print("seed %d: %d statements, %d problem blocks" % (SEED, len(statements), problems))


main()
