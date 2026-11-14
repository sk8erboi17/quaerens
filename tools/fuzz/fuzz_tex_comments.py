"""Fuzzing dei commenti LaTeX intorno agli enunciati (parser.c, capitolo 21).

Uso: python3 -I fuzz_tex_comments.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Genera uguaglianze fra interi e somme finite, vere o false, e aggiunge note
prima e dopo la formula. Nelle note mette anche falsi delimitatori di
formule ed elenchi. L'oracolo calcola gli interi e le somme con Python,
senza chiamare il motore per stabilire la verita. Ogni formula deve ricevere
un verdetto, le vere DIMOSTRATO e le false FALSO; un frammento di commento
non deve diventare un enunciato in piu. Tutti i file prodotti stanno nella
cartella di lavoro. Un BUG fa terminare lo script con codice 1.
"""

import os
import random
import re
import subprocess
import sys


def main():
    if len(sys.argv) != 5:
        print(__doc__)
        return 2
    program, seed, count, work = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
    rng = random.Random(seed)
    os.makedirs(work, exist_ok=True)
    notes = (
        "source note",
        r"\item $\displaystyle 0 = 1$",
        r"\end{enumerate}",
        r"$a = 0$ with $a > 0$",
        r"\] $ \itemize",
    )
    cases = []
    for _ in range(count):
        a, b = rng.randint(-20, 20), rng.randint(-20, 20)
        if rng.randrange(2):
            terms = [a * k + b for k in range(1, rng.randint(2, 12))]
            value = sum(terms)
            expression = r"\sum_{k=1}^{%d} (%d k + (%d))" % (len(terms), a, b)
        else:
            value = a * a + b
            expression = "(%d)^2 + (%d)" % (a, b)
        claimed = value + rng.choice((0, 0, -2, -1, 1, 2))
        formula = "%s = %d" % (expression, claimed)
        rendered = (r"$\displaystyle %s$" if rng.randrange(2) else r"\[ %s \]") % formula
        leading = "".join("%% %s\n" % rng.choice(notes) for _ in range(rng.randrange(3)))
        trailing = "".join(" %% %s\n" % rng.choice(notes) for _ in range(rng.randint(1, 3)))
        cases.append((leading + rendered + trailing, value == claimed))

    # Le note nel documento devono essere ignorate anche prima del primo item.
    path = os.path.join(work, "comments_%d.tex" % seed)
    with open(path, "w", encoding="utf-8") as out:
        out.write(r"\documentclass{article}" + "\n" + r"\usepackage{amsmath,amssymb}" + "\n")
        out.write(r"\begin{document}" + "\n% " + notes[1] + "\n" + r"\begin{enumerate}" + "\n")
        for text, _ in cases:
            out.write("\\item " + text + "\n")
        out.write(r"\end{enumerate}" + "\n" + r"\end{document}" + "\n")
    run = subprocess.run([program, path], capture_output=True, text=True, timeout=180)
    with open(os.path.join(work, "results_%d.log" % seed), "w", encoding="utf-8") as out:
        out.write(run.stdout + run.stderr)
    lines = [line for line in run.stdout.splitlines() if re.match(r"\s*\d+\. ", line)]
    if run.returncode not in (0, 1) or len(lines) != len(cases) or "ATTENZIONE" in run.stdout:
        print("BUG: expected %d verdicts, got %d (exit %d)" % (len(cases), len(lines), run.returncode))
        return 1
    bugs = 0
    for i, ((text, truth), line) in enumerate(zip(cases, lines), 1):
        expected = "DIMOSTRATO" if truth else "FALSO"
        verdict = line.split(". ", 1)[1]
        if not verdict.startswith(expected):
            bugs += 1
            print("BUG %d: expected %s, got %s\n%s" % (i, expected, verdict, text))
    print("seed %d: %d statements, %d BUG" % (seed, len(cases), bugs))
    return 1 if bugs else 0


if __name__ == "__main__":
    sys.exit(main())
