"""Fuzzing differenziale degli enunciati su resti e divisibilità.

Uso: python3 -I fuzz_integers.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Genera enunciati della forma

    per ogni x in S esiste y in Z tale che p(x) = m y + c1 oppure ... oppure p(x) = m y + ck

con p polinomio a coefficienti interi, m un modulo tra 2 e 10 e S uno tra Z, N
e N0. L'oracolo è esatto e indipendente dal programma: p(x) modulo m dipende
solo da x modulo m, quindi l'enunciato è vero se e solo se, per ogni resto r
tra 0 e m - 1, p(r) modulo m è uno dei c indicati.

BUG: dimostrato un enunciato falso, dichiarato falso un enunciato vero, oppure
un controesempio x = v in cui p(v) ha invece uno dei resti ammessi.
"""
import os
import random
import re
import subprocess
import sys

MR, SEED, COUNT, WORK = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
rnd = random.Random(SEED)
SETS = [r"\mathbb{Z}", r"\mathbb{N}", r"\mathbb{N}_{0}"]


def poly_tex(c, v):
    parts = []
    for degree in range(len(c) - 1, -1, -1):
        a = c[degree]
        if a == 0:
            continue
        body = "" if degree == 0 else (v if degree == 1 else "%s^{%d}" % (v, degree))
        size = abs(a)
        text = (str(size) if size != 1 or degree == 0 else "") + ((" " if size != 1 and body else "") + body if body else "")
        parts.append(("- " if a < 0 else "+ ") + text)
    if not parts:
        return "0"
    first = parts[0]
    return (("-" + first[2:]) if first.startswith("- ") else first[2:]) + "".join(" " + p for p in parts[1:])


def value(c, x):
    return sum(a * x ** i for i, a in enumerate(c))


def make_case():
    while True:
        m = rnd.choice([2, 2, 3, 3, 4, 5, 6, 6, 7, 8, 9, 10])
        c = [rnd.randint(-3, 3) for _ in range(rnd.choice([2, 3, 3, 4]))]
        if all(a == 0 for a in c[1:]):
            continue
        reached = sorted(set(value(c, r) % m for r in range(m)))
        want_true = rnd.random() < 0.5
        allowed = set(reached)
        if want_true:
            for extra in range(m):
                if rnd.random() < 0.2:
                    allowed.add(extra)
        else:
            allowed.discard(rnd.choice(reached))
            if not allowed:
                continue
        shown = [a + m * rnd.choice([0, 0, 0, 1, -1]) for a in sorted(allowed)]
        rnd.shuffle(shown)
        return c, m, shown, rnd.choice(SETS)


def statement(case):
    c, m, shown, where = case
    left = poly_tex(c, "x")
    forms = []
    for a in shown:
        forms.append("%s = %d y" % (left, m) + ("" if a == 0 else (" + %d" % a if a > 0 else " - %d" % -a)))
    return r"\forall x \in %s\ \exists y \in \mathbb{Z}\ (%s)" % (where, r" \lor ".join(forms))


def truth(case):
    c, m, shown, _ = case
    allowed = set(a % m for a in shown)
    return all(value(c, r) % m in allowed for r in range(m))


def main():
    cases = [make_case() for _ in range(COUNT)]
    path = os.path.join(WORK, "fuzz_integers_%d.tex" % SEED)
    with open(path, "w", encoding="utf-8") as out:
        out.write("\\begin{enumerate}\n")
        for case in cases:
            out.write("\\item $\\displaystyle %s$\n" % statement(case))
        out.write("\\end{enumerate}\n")
    run = subprocess.run([MR, path], capture_output=True, text=True, timeout=1800)
    lines = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
    if len(lines) != len(cases) or run.returncode not in (0, 1):
        print("BUG: the program answered %d of %d statements (exit %d)\n%s" % (len(lines), len(cases), run.returncode, run.stderr[-2000:]))
        return
    if "ATTENZIONE" in run.stdout:
        print("BUG: a written proof step was not confirmed: %s" % [l for l in run.stdout.splitlines() if "ATTENZIONE" in l][0])
    tally = {}
    for number, (case, line) in enumerate(zip(cases, lines), 1):
        text = line.split(". ", 1)[1]
        verdict = "P" if text.startswith("DIMOSTRATO") else "R" if text.startswith("FALSO") else "N" if text.startswith("NON DIMOSTRATO") else "U"
        ok = truth(case)
        small = case[1] <= 6
        key = ("true" if ok else "false", "m<=6" if small else "m>6", verdict)
        tally[key] = tally.get(key, 0) + 1
        problem = None
        if verdict == "P" and not ok:
            problem = "BUG: proved, but some remainder of x gives a remainder that is not listed"
        elif verdict == "R" and ok:
            problem = "BUG: declared false, but every remainder of x gives a listed remainder"
        elif verdict == "R":
            found = re.search(r"controesempio x = (-?\d+)$", text)
            c, m, shown, where = case
            if found and value(c, int(found.group(1))) % m in set(a % m for a in shown):
                problem = "BUG: the counterexample x = %s is not one" % found.group(1)
            if found and (int(found.group(1)) < (1 if where == SETS[1] else 0 if where == SETS[2] else -10 ** 9)):
                problem = "BUG: the counterexample x = %s is outside the set" % found.group(1)
        elif verdict in ("N", "U") and small and ok:
            problem = "NOTE: true with a modulus up to 6, but not proved (%s)" % text[:60]
        if problem:
            print("%s\n   %d: %s" % (problem, number, statement(case)))
    print("seed %d:" % SEED, ", ".join("%s/%s/%s=%d" % (k[0], k[1], k[2], v) for k, v in sorted(tally.items())))


main()
