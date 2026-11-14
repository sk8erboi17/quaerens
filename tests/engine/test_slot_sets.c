/* Algebra degli insiemi di variabili, anche oltre le vecchie 128 posizioni.
 * L'oracolo usa sei bit compatti, mentre i dati del motore sono sparsi fino
 * alla posizione 4096. Ogni unione, inclusione e intersezione viene provata
 * per tutte le 4096 coppie di sottoinsiemi. Le copie devono restare immutate.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 * cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_slot_sets \
 *     tests/engine/test_slot_sets.c && /tmp/test_slot_sets
 */
#include "../statement_checks.h"

int main(void) {
    engine_init();
    static const int position[] = {0, 63, 64, 127, 128, 4096};
    SlotSet set[64];
    int all = 1;
    for (unsigned mask = 0; mask < 64; mask++) {
        set[mask] = SLOT_EMPTY;
        for (size_t i = 0; i < sizeof position / sizeof *position; i++)
            if (mask >> i & 1) set[mask] = slot_set_with(set[mask], position[i]);
    }
    for (unsigned a = 0; a < 64; a++) for (unsigned b = 0; b < 64; b++) {
        SlotSet joined = slot_set_union(set[a], set[b]);
        all &= slot_set_includes(set[a], set[b]) == ((a & b) == b);
        all &= slot_set_intersects(set[a], set[b]) == ((a & b) != 0);
        for (size_t i = 0; i < sizeof position / sizeof *position; i++)
            all &= slot_set_contains(joined, position[i]) == (int)((a | b) >> i & 1);
        all &= !slot_set_contains(joined, 4097);
    }
    int passed = check_result(all, "all sparse-set operations agree with the independent six-bit truth table");
    all = 1;
    for (unsigned mask = 0; mask < 64; mask++)
        for (size_t i = 0; i < sizeof position / sizeof *position; i++)
            all &= slot_set_contains(set[mask], position[i]) == (int)(mask >> i & 1);
    passed = check_result(all, "unions leave every stored parent set unchanged") && passed;
    SlotSet parent = slot_set_single(129), child = slot_set_with(parent, 7000);
    passed = check_result(slot_set_contains(parent, 129) && !slot_set_contains(parent, 7000) &&
                          slot_set_contains(child, 129) && slot_set_contains(child, 7000),
                          "extending a shared high tail cannot change a previous scope") && passed;
    SlotSet ordinary = slot_set_with(slot_set_single(0), 127);
    passed = check_result(!ordinary.extra && !ordinary.high,
                          "the existing 128-bit cases need no tail allocation on this platform") && passed;
    passed = check_result(!failure && !work_allocation_failed, "all bitsets are complete and no allocation failed") && passed;
    return passed ? 0 : 1;
}
