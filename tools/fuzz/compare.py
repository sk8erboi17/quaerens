"""Confronto tra due versioni del programma sugli stessi file di enunciati.

Uso: python3 -I compare.py <vecchio eseguibile> <nuovo eseguibile> <file .tex>...

Stampa ogni enunciato il cui verdetto cambia tra le due versioni, raggruppando
per tipo di cambiamento, e segnala i passi di dimostrazione non confermati.
"""
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

OLD, NEW, FILES = sys.argv[1], sys.argv[2], sys.argv[3:]


def run(binary, path):
    try:
        done = subprocess.run([binary, path], capture_output=True, text=True, errors="replace", timeout=900)
    except subprocess.TimeoutExpired:
        return None, "timeout"
    return [l for l in done.stdout.splitlines() if re.match(r"\s*\d+\. ", l)], done.stdout + done.stderr


def kind(line):
    text = line.split(". ", 1)[1]
    for word in ("DIMOSTRATO", "FALSO", "NON DIMOSTRATO", "NON SUPPORTATO"):
        if text.startswith(word):
            return word
    return "?"


def statements(path):
    text = open(path, encoding="utf-8", errors="replace").read()
    return re.findall(r"\\item (.*)", text)


def one(path):
    return path, run(OLD, path), run(NEW, path)


changes, total, warnings = {}, 0, 0
with ThreadPoolExecutor(8) as pool:
    for path, (old, _), (new, raw) in pool.map(one, FILES):
        if "ATTENZIONE" in raw or "Sanitizer" in raw or "runtime error" in raw:
            warnings += 1
            print("WARNING in %s: %s" % (path, [l for l in raw.splitlines() if "ATTENZIONE" in l or "ERROR" in l or "runtime error" in l][:3]))
        if old is None or new is None or len(old) != len(new):
            print("DIFFERENT SHAPE: %s (%s vs %s)" % (path, old and len(old), new and len(new)))
            continue
        items = statements(path)
        for i, (a, b) in enumerate(zip(old, new)):
            total += 1
            ta, tb = a.split(". ", 1)[1], b.split(". ", 1)[1]
            if ta.split(" (ignorato")[0] != tb.split(" (ignorato")[0]:
                key = "%s -> %s" % (kind(a), kind(b))
                changes.setdefault(key, []).append((path.split("/")[-1], i + 1, ta[:110], tb[:110], items[i][:200] if i < len(items) else ""))
print("%d statements compared, %d files with warnings" % (total, warnings))
for key, rows in sorted(changes.items()):
    print("== %s: %d" % (key, len(rows)))
    for row in rows[:int(12)]:
        print("   %s #%d\n      old: %s\n      new: %s\n      %s" % row)
