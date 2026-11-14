"""Fuzz del primo ordine monadico senza uguaglianza, con oracolo completo.

Uso: python3 -I fuzz_saturation.py <eseguibile> <seme> <quanti> <cartella di lavoro>

Con k predicati unari, ogni elemento ha uno dei 2^k tipi booleani. Senza
uguaglianza o predicati binari, la verita di una formula dipende solo da
quali tipi sono abitati: tutte le occorrenze di uno stesso tipo sono
indistinguibili. Un'interpretazione, anche infinita, equivale quindi a
una con un rappresentante per tipo abitato. Provare tutti i sottoinsiemi
dei tipi, compreso quello vuoto, decide esattamente questo frammento.
L'oracolo interpreta direttamente l'albero con quantificatori, senza
clausificazione, Skolem, SAT, risoluzione o funzioni del programma.

Le formule sono dati casuali, comprese identita logiche e alterazioni che
possono essere false. BUG indica una risposta contraria all'oracolo, un
certificato respinto dal controllo interno o un'uscita anomala. NOTE indica
solo una formula non decisa dal programma entro le proprie risorse.
"""
import os
import random
import re
import subprocess
import sys


def evaluate(node, elements, env):
    op = node[0]
    if op == "atom":
        return bool(env[node[2]] >> node[1] & 1)
    if op == "not":
        return not evaluate(node[1], elements, env)
    if op in ("forall", "exists"):
        every = op == "forall"
        for element in elements:
            inner = dict(env)
            inner[node[1]] = element
            if evaluate(node[2], elements, inner) != every:
                return not every
        return every
    a = evaluate(node[1], elements, env)
    if op == "and":
        return a and evaluate(node[2], elements, env)
    if op == "or":
        return a or evaluate(node[2], elements, env)
    return not a or evaluate(node[2], elements, env)


def valid(node, predicates):
    types = 1 << predicates
    for inhabited in range(1 << types):
        elements = [t for t in range(types) if inhabited >> t & 1]
        if not evaluate(node, elements, {}):
            return False
    return True


def render(node):
    op = node[0]
    if op == "atom":
        return "%s \\in %s" % (node[2], "PQR"[node[1]])
    if op == "not":
        return r"\lnot (" + render(node[1]) + ")"
    if op in ("forall", "exists"):
        return "\\%s %s \\in S\\ (%s)" % (op, node[1], render(node[2]))
    word = {"and": r"\land", "or": r"\lor", "implies": r"\implies"}[op]
    return "(%s %s %s)" % (render(node[1]), word, render(node[2]))


def formula(rng, predicates):
    names = "xyzw"[:rng.randint(1, 4)]

    def body(depth):
        if not depth or rng.randrange(3) == 0:
            atom = ("atom", rng.randrange(predicates), rng.choice(names))
            return ("not", atom) if rng.randrange(2) else atom
        return (rng.choice(["and", "or", "implies"]), body(depth - 1), body(depth - 1))

    node = body(rng.randint(1, 3))
    for name in reversed(names):
        node = (rng.choice(["forall", "exists"]), name, node)
    shape = rng.randrange(4)
    if shape == 0:
        return ("implies", node, node)
    if shape == 1:
        return ("or", node, ("not", node))
    return node


def main():
    if len(sys.argv) != 5:
        print(__doc__)
        return 2
    program, seed, count, work = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
    rng = random.Random(seed)
    os.makedirs(work, exist_ok=True)
    cases = []
    for _ in range(count):
        predicates = rng.choice([1, 2, 2, 3])
        node = formula(rng, predicates)
        declarations = r" \land ".join("%s \\subseteq S" % p for p in "PQR"[:predicates])
        cases.append(("(%s) \\implies (%s)" % (declarations, render(node)), valid(node, predicates)))
    path = os.path.join(work, "saturation_%d.tex" % seed)
    certificate = os.path.join(work, "saturation_%d.cert" % seed)
    with open(path, "w", encoding="utf-8") as out:
        out.write("\\documentclass{article}\n\\usepackage{amsmath,amssymb}\n"
                  "\\title{Monadic first-order statements}\n\\begin{document}\n\\maketitle\n\\begin{enumerate}\n")
        for text, _ in cases:
            out.write("\\item $\\displaystyle %s$\n" % text)
        out.write("\\end{enumerate}\n\\end{document}\n")
    result = subprocess.run([program, "--model-work", "20000", "--certificates", certificate, path],
                            capture_output=True, text=True, timeout=900)
    with open(os.path.join(work, "results_%d.log" % seed), "w", encoding="utf-8") as out:
        out.write(result.stdout + result.stderr)
    lines = [s for s in result.stdout.splitlines() if re.match(r"\s*\d+\. ", s)]
    if result.returncode not in (0, 1) or len(lines) != count or "ATTENZIONE" in result.stdout:
        print("BUG: expected %d verdicts, got %d (exit %d)" % (count, len(lines), result.returncode))
        return 1
    bugs = unknown = saturation = 0
    for i, ((text, truth), answer) in enumerate(zip(cases, lines), 1):
        proved, refuted = "DIMOSTRATO" in answer and "NON DIMOSTRATO" not in answer, "FALSO" in answer
        saturation += "per saturazione" in answer
        unknown += not proved and not refuted
        if (proved and not truth) or (refuted and truth):
            print("BUG %d: oracle=%s, answer=%s\n%s" % (i, truth, answer.strip(), text))
            bugs += 1
    print("seed %d: %d statements, %d saturation proofs, %d BUG, %d NOTE" % (seed, count, saturation, bugs, unknown))
    return int(bugs != 0)


if __name__ == "__main__":
    sys.exit(main())
