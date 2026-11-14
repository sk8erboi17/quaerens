/* Quantificatori e rami oltre le vecchie dimensioni fisse del verificatore.
 * I certificati sono costruiti senza il motore: su un insieme singoletto
 * tutti gli elementi coincidono, su {0,1} la stessa universale e falsa.
 * Le alterazioni devono restare respinte, anche dopo una crescita fallita.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 * cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_dynamic_contexts \
 *     tests/checker/test_dynamic_contexts.c && /tmp/test_dynamic_contexts
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int reject_growth;

/* Il guasto appartiene al test: il verificatore deve rispondere che non
 * puo controllare, poi tornare a funzionare sul certificato successivo. */
static void *controlled_realloc(void *data, size_t bytes) {
    return reject_growth ? NULL : realloc(data, bytes);
}

#define realloc controlled_realloc
#define main checker_main
#include "../../checker.c"
#undef main
#undef realloc

/* Controlla la risposta e stampa una descrizione inglese, col motivo del
 * verificatore soltanto se la risposta differisce da quella attesa. */
static int expect(const char *text, CkAnswer wanted, const char *description) {
    const char *why;
    CkAnswer answer = ck_check(&text, &why);
    int passed = answer == wanted;
    printf("%s: %s%s%s\n", passed ? "PASS" : "FAIL", description, passed ? "" : ": ", passed ? "" : why);
    return passed;
}

/* Scrive una formula con depth quantificatori, ciascuno con nome distinto.
 * I dati del test stanno in buffer abbastanza grandi per il solo caso
 * costruito; nessun deposito del verificatore dipende da questi numeri. */
static void nested(char *text, size_t bytes, int depth, int parts, int size, const char *kind) {
    size_t used = (size_t)snprintf(text, bytes, "certificate %s\nsort S %d\nstatement ", kind, size);
    for (int i = 0; i < depth; i++)
        used += (size_t)snprintf(text + used, bytes - used, "(%s x%d S ", parts ? "forallpart" : "forall", i);
    used += (size_t)snprintf(text + used, bytes - used, parts ? "(forall t S (x0 t))" : "(= x0 x%d)", depth - 1);
    for (int i = 0; i < depth; i++) text[used++] = ')';
    snprintf(text + used, bytes - used, "\nend\n");
}

int main(void) {
    char text[16384];
    nested(text, sizeof text, 160, 0, 1, "example");
    int passed = expect(text, CK_ACCEPTED, "160 nested element bindings are checked on a singleton");
    nested(text, sizeof text, 160, 0, 2, "example");
    passed = expect(text, CK_REJECTED, "the same universal equality is rejected on two distinct elements") && passed;
    nested(text, sizeof text, 160, 0, 2, "counterexample");
    passed = expect(text, CK_ACCEPTED, "the two-element interpretation independently certifies a counterexample") && passed;
    nested(text, sizeof text, 160, 1, 0, "example");
    passed = expect(text, CK_ACCEPTED, "160 nested subset bindings are checked on the empty base set") && passed;
    nested(text, sizeof text, 160, 1, 1, "example");
    passed = expect(text, CK_REJECTED, "the empty subset invalidates the altered nonempty interpretation") && passed;

    size_t used = (size_t)snprintf(text, sizeof text, "certificate structure\nstatement true\n");
    for (int i = 0; i < 160; i++) used += (size_t)snprintf(text + used, sizeof text - used, "case true\n");
    snprintf(text + used, sizeof text - used, "end\n");
    passed = expect(text, CK_ACCEPTED, "160 proof branches preserve the complete case-coverage check") && passed;
    used = (size_t)snprintf(text, sizeof text, "certificate structure\nstatement (not true)\n");
    for (int i = 0; i < 160; i++) used += (size_t)snprintf(text + used, sizeof text - used, "case true\n");
    snprintf(text + used, sizeof text - used, "end\n");
    passed = expect(text, CK_REJECTED, "changing the certified statement still invalidates the proof") && passed;

    nested(text, sizeof text, 160, 0, 1, "example");
    reject_growth = 1;
    passed = expect(text, CK_UNSUPPORTED, "failed context allocation is never accepted") && passed;
    reject_growth = 0;
    passed = expect(text, CK_ACCEPTED, "a new certificate starts cleanly after allocation failure") && passed;
    return passed ? 0 : 1;
}
