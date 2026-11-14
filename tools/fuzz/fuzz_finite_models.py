"""Fuzzing dei modelli finiti piu grandi della ricerca esaustiva (28t).

Uso: python3 -I fuzz_finite_models.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Genera funzioni f con f^a=id e vincoli sui punti fissi delle iterate. L'oracolo
usa la decomposizione in cicli: f^a=id rende f una permutazione e le lunghezze
dei cicli dividono a. Un ciclo di lunghezza d ha punti fissi per f^b se e solo
se d divide b. Si scelgono a<=17 per coprire anche cardinalita oltre otto;
questo e un intervallo dei dati di fuzz, non un limite del motore. Le frasi sono sia richieste di esempi non vuoti sia
implicazioni universali, con tesi vere e false. Non si usano SAT o tabelle
del motore per decidere la verita. I certificati scritti dal motore restano
nella cartella di lavoro per il controllo con checker da solo.

Alcune implicazioni hanno da 65 a 320 parti dichiarate ma non usate. Ogni
insieme ammette queste parti, anche quello vuoto: l'oracolo sui cicli non
cambia. Servono a esercitare dichiarazioni e tabelle senza i vecchi tetti
di 64 e 256 simboli. I nomi alternano pedici a una cifra e numeri oltre 99;
anche insiemi, mappe e variabili possono ricevere pedici numerici grandi.
Una rinomina iniettiva non modifica l'oracolo sui cicli.

BUG: un falso DIMOSTRATO, un vero FALSO, oppure un errore del controllo
indipendente. NOTE: un esempio possibile o un controesempio non trovato.
"""

import os
import random
import re
import subprocess
import sys


def iterate(order):
    return "f(" * order + "x" + ")" * order


def main():
    if len(sys.argv) != 5:
        print(__doc__)
        return 2
    program, seed, count, work = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
    rng = random.Random(seed)
    os.makedirs(work, exist_ok=True)
    cases = []
    for _ in range(count):
        order = rng.randint(1, 17)
        forbidden = rng.sample(range(1, 19), rng.randint(1, 3))
        if rng.randrange(2):
            possible = any(order % d == 0 and all(b % d for b in forbidden) for d in range(1, order + 1))
            condition = " \\land ".join([iterate(order) + " = x"] + [iterate(b) + r" \neq x" for b in forbidden])
            formula = (r"\exists S \neq \emptyset\ \exists f \colon S \to S\ (\forall x \in S\ (%s))" % condition)
            cases.append((formula, possible, True))
        else:
            power = forbidden[0]
            formula = (r"(f \colon S \to S \land \forall x \in S\ (%s = x))"
                       r" \implies \forall x \in S\ (%s = x)" % (iterate(order), iterate(power)))
            if rng.randrange(4) == 0:
                extra = rng.choice([65, 257, 320])
                letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
                numeric = rng.randrange(2)
                declarations = r" \land ".join("%s_{%d} \\subseteq S" %
                    ("A" if numeric else letters[i // 9], i + 1 if numeric else i % 9 + 1) for i in range(extra))
                formula = formula.replace(r"(f \colon S \to S \land ", "(f \\colon S \\to S \\land " + declarations + r" \land ", 1)
            cases.append((formula, power % order == 0, False))

        # Una rinomina iniettiva cambia soltanto i nomi, non i cicli.
        if rng.randrange(3)==0:
            formula, truth, example=cases[-1]
            index=rng.choice([12,99,100,1024,2147483647])
            for name in ["S","f","x"]:
                formula=re.sub(r"\b"+name+r"\b",name+"_{%d}" % index,formula)
            cases[-1]=(formula,truth,example)

    path = os.path.join(work, "finite_models_%d.tex" % seed)
    certificate_path = os.path.join(work, "finite_models_%d.cert" % seed)
    with open(path, "w", encoding="utf-8") as out:
        out.write(r"\documentclass{article}" + "\n" + r"\usepackage{amsmath,amssymb}" + "\n")
        out.write(r"\begin{document}\begin{enumerate}" + "\n")
        for formula, _, _ in cases:
            out.write("\\item $\\displaystyle %s$\n" % formula)
        out.write(r"\end{enumerate}\end{document}" + "\n")
    run = subprocess.run([program, "--certificates", certificate_path, path], capture_output=True, text=True, timeout=600)
    with open(os.path.join(work, "results_%d.log" % seed), "w", encoding="utf-8") as out:
        out.write(run.stdout + run.stderr)
    lines = [line for line in run.stdout.splitlines() if re.match(r"\s*\d+\. ", line)]
    if run.returncode not in (0, 1) or len(lines) != len(cases) or "ATTENZIONE" in run.stdout:
        print("BUG: expected %d verdicts, got %d (exit %d)" % (len(cases), len(lines), run.returncode))
        return 1
    bugs = notes = 0
    for i, ((formula, truth, example), line) in enumerate(zip(cases, lines), 1):
        verdict = line.split(". ", 1)[1]
        wrong = (verdict.startswith("DIMOSTRATO") and not truth) or (verdict.startswith("FALSO") and truth)
        if wrong:
            bugs += 1
            print("BUG %d: %s\n%s" % (i, verdict, formula))
        elif (example and truth and not verdict.startswith("DIMOSTRATO")) or (not example and not truth and not verdict.startswith("FALSO")):
            notes += 1
            print("NOTE %d: a finite model was not found: %s" % (i, verdict))
    print("seed %d: %d statements, %d BUG, %d NOTE" % (seed, len(cases), bugs, notes))
    return 1 if bugs else 0


if __name__ == "__main__":
    sys.exit(main())
