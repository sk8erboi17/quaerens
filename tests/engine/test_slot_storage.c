/* I metadati delle variabili e le sostituzioni crescono oltre 128 slot.
 * I casi end-to-end hanno quantificatori indipendenti: l'oracolo e la
 * riflessivita, oppure l'uguaglianza di due elementi distinti di {0,1}.
 * Gli snapshot devono sopravvivere alle riallocazioni e i Known devono
 * conservare copie immutabili anche quando nascono nuove variabili.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 * cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_slot_storage \
 *     tests/engine/test_slot_storage.c && /tmp/test_slot_storage
 */
#include "../statement_checks.h"

int main(void) {
    engine_init();
    char formula[16384];
    size_t used = 0;
    for (int i = 0; i < 140; i++)
        used += (size_t)snprintf(formula + used, sizeof formula - used,
                                "%s(\\forall x \\in S\\ (x=x))", i ? " \\land " : "");
    Verdict v = prove(formula, 'n', 0, 0);
    if (v.outcome != PROVED || !v.certificate) {
        printf("Unexpected result for the true statement: ");
        verdict_print(stdout, &v);
        putchar('\n');
    }
    int passed = check_result(v.outcome == PROVED && v.certificate && slot_count >= 140,
                              "a statement with 140 distinct quantifier slots is proved and independently certified");
    snprintf(formula + used, sizeof formula - used,
             " \\land (\\forall x,y \\in S\\ (x=y))");
    v = prove(formula, 'n', 0, 0);
    passed = check_result(v.outcome == REFUTED && v.certificate && slot_count >= 142,
                          "a false variant beyond slot 128 has an independently certified two-element countermodel") && passed;

    /* Anche la profondita supera il vecchio limite. Ogni universale
     * fallisce gia nel singoletto, senza una scansione esponenziale. */
    used = 0;
    for (const char *letter = "abcdefghijklmnopqrst"; *letter; letter++)
        for (int index = 1; index <= 8; index++)
            used += (size_t)snprintf(formula + used, sizeof formula - used,
                                    "\\forall %c_{%d} \\in S\\ ", *letter, index);
    snprintf(formula + used, sizeof formula - used, "(a_{1} \\neq a_{1})");
    v = prove(formula, 'n', 0, 0);
    passed = check_result(v.outcome == REFUTED && v.certificate && slot_count >= 160,
                          "160 nested quantifiers retain a fully checked counterexample") && passed;

    /* Nomi sintetici come dati del test: nessuna formula del linguaggio
     * dipende da questi codici. La lettura torna a un punto gia valido. */
    slot_count = visible_count = 0;
    for (int i = 0; i < 24; i++) {
        int slot = slot_new(10000 + i, i, -1);
        if (slot >= 0) visible_push(slot);
    }
    ReaderMark saved = reader_mark();
    for (int i = 0; i < 24; i++) {
        slot_data[i].letter = -1;
        slot_data[i].index = 0;
        slot_data[i].sort = 0;
        slot_data[i].pair = 23 - i;
    }
    for (int i = 24; i < 700; i++) {
        int slot = slot_new(10000 + i, i, -1);
        if (slot >= 0) visible_push(slot);
    }
    int grew = slot_count == 700 && visible_count == 700 && slot_capacity >= 700 && unfold_capacity >= 700;
    reader_back(&saved);
    int restored = slot_count == 24 && visible_count == 24;
    for (int i = 0; i < 24; i++)
        restored &= slot_data[i].letter == 10000 + i && slot_data[i].index == i &&
                    slot_data[i].sort == -1 && slot_data[i].pair == -1;
    passed = check_result(grew && restored,
                          "reader snapshots restore every field after growing metadata and visibility to 700 slots") && passed;

    SlotValues temporary = slot_values_copy(SLOT_VALUES_EMPTY);
    if (!work_allocation_failed) temporary.data[3] = 7;
    now = &branch[0];
    now->knowns = now->clock = 0;
    Sentence *truth = sentence_new(S_TRUE, NULL, NULL, "");
    int at = known_add(truth, 0, temporary, K_GIVEN, -1);
    if (!work_allocation_failed) temporary.data[3] = 9;
    for (int i = 24; i < 300; i++) slot_new(10000 + i, i, -1);
    SlotValues later = slot_values_copy(at >= 0 ? now->known[at].value : SLOT_VALUES_EMPTY);
    int copied = at >= 0 && !work_allocation_failed && slot_values_get(now->known[at].value, 3) == 7 &&
                 slot_values_get(now->known[at].value, 299) == -1 && slot_values_get(later, 299) == -1;
    if (copied) later.data[299] = 11;
    passed = check_result(copied && slot_values_get(now->known[at].value, 299) == -1 && slot_values_get(later, 299) == 11,
                          "known substitutions are immutable and newly created variables remain unbound in older snapshots") && passed;
    passed = check_result(!failure && !work_allocation_failed && unjustified_steps == 0,
                          "all storage growth preserves independently checked results") && passed;
    certificate_broken = 1;
    Verdict unchecked = {0};
    unchecked.outcome = PROVED;
    unchecked.proof = "incomplete proof";
    unchecked = abstract_certified(unchecked);
    certificate_broken = 0;
    passed = check_result(unchecked.outcome == NOT_PROVED && !unchecked.proof && !unchecked.certificate,
                          "an unsupported or incomplete certificate never leaves a proved verdict") && passed;
    return passed ? 0 : 1;
}
