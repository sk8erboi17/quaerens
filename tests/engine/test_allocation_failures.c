/* Un errore di crescita non deve produrre un verdetto positivo, nemmeno
 * se una strategia interna cancella failure per provarne un'altra. La
 * sostituzione di realloc appartiene al test, non al programma.
 * Si verifica anche che i valori visibili dell'ambiente restino agli stessi
 * indirizzi mentre crescono sia i blocchi sia il loro indice.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 * cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_allocation_failures \
 *     tests/engine/test_allocation_failures.c && /tmp/test_allocation_failures
 */
#include <stdlib.h>

static int reject_reallocation;

/* Simula soltanto il fallimento richiesto dal test; normalmente usa la
 * vera realloc della libreria standard, senza modificare i dati. */
static void *controlled_realloc(void *data, size_t bytes) {
    return reject_reallocation ? NULL : realloc(data, bytes);
}

#define realloc controlled_realloc
#include "../statement_checks.h"
#undef realloc

int main(void) {
    engine_init();
    const char *truth = "\\forall a \\in \\mathbb{Z}\\ (a = a)";
    reject_reallocation = 1;
    Verdict v = prove(truth, 'n', 0, 0);
    int passed = check_result(v.outcome == NOT_PROVED && !v.certificate && v.reason == OUT_OF_MEMORY,
                              "a failed parser-stack allocation produces no proof or counterexample");
    passed = check_result(work_allocation_failed, "allocation failure remains recorded after internal fallbacks") && passed;
    reject_reallocation = 0;
    v = prove(truth, 'n', 0, 0);
    passed = check_result(v.outcome == PROVED && !work_allocation_failed,
                          "a later independent statement starts cleanly and can be proved") && passed;

    /* I nomi numerici sintetici sono dati della prova di memoria: la
     * valutazione non deve interpretarli come lettere di una formula. */
    env_count = 0;
    passed = check_result(env_push('a', rat_int(7)), "the first binding is stored") && passed;
    const Rat *remembered = env_lookup('a');
    int complete = 1;
    for (int i = 0; i < 1100; i++) if (!env_push(1000 + i, rat_int(i))) { complete = 0; break; }
    Num value;
    passed = check_result(complete && remembered == env_lookup('a') && rat_number(*remembered, &value) &&
                          num_equal(value, num_int(7)),
                          "binding addresses and values survive growth beyond sixteen separately allocated blocks") && passed;
    env_count = 0;
    return passed ? 0 : 1;
}
