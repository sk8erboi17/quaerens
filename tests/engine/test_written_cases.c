/* Test del motore: il controllo che i casi scritti di una dimostrazione del
 * capitolo 28 siano tutti (28p di main.c).
 *
 * Un caso dimenticato ha tutti i passi giusti e non dimostra nulla; il
 * certificato non se ne accorge quando il caso non chiede istanze, perché il
 * verificatore lo ritrova da sé. Qui si danno al controllo, senza
 * dimostrare nulla, i cammini dei casi di alcune foglie scritte, come le
 * annoterebbe leaf_write(), e si guarda la risposta:
 *
 *   - coprono tutto: una foglia senza casi; i due casi di una disgiunzione;
 *     un caso chiuso subito e l'altro diviso ancora in due;
 *   - non coprono tutto, ed è l'esperimento del motore bugiardo che non
 *     scriveva il caso già chiuso di 28o: un caso solo; un caso diviso
 *     ancora, con uno dei due sotto-casi mancante; due foglie che allo stesso
 *     punto distinguono due disgiunzioni diverse; nessuna foglia.
 *
 * Sono antiregressione: non si tolgono e non si indeboliscono.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_written_cases \
 *         tests/engine/test_written_cases.c && /tmp/test_written_cases
 */

#include "../statement_checks.h"

/* Due condizioni qualsiasi, che fanno da parti di una disgiunzione: il
 * controllo guarda solo quale parte è stata scelta, non che cosa dice. */
static Sentence part_a, part_b, part_c, part_d;

/* Mette in case_path[] le foglie di paths[], ognuna una stringa di passi:
 * "A3" è la parte part_a della disgiunzione di posto 3, e così via; e
 * ritorna la risposta del controllo. */
static int cover(const char *const *paths, int count) {
    case_paths = case_overflow = 0;
    for (int i = 0; i < count; i++) {
        CasePath *p = &case_path[case_paths++];
        p->steps = 0;
        for (const char *c = paths[i]; *c; c += 2) {
            const Sentence *source = *c == 'A' ? &part_a : *c == 'B' ? &part_b : *c == 'C' ? &part_c : &part_d;
            p->step[p->steps++] = (CaseStep){(short)(c[1] - '0'), source, 0};
        }
    }
    return case_paths_complete();
}

int main(void) {
    int passed = 1;
    static const char *const single[] = {""};
    static const char *const two[] = {"A3", "B3"};
    static const char *const nested[] = {"A3", "B3C5", "B3D5"};
    static const char *const one_side[] = {"B3"};
    static const char *const nested_missing[] = {"A3", "B3C5"};
    static const char *const two_splits[] = {"A3", "B4"};

    passed = check_result(cover(single, 1), "a leaf without cases covers everything") && passed;
    passed = check_result(cover(two, 2), "the two cases of a disjunction cover everything") && passed;
    passed = check_result(cover(nested, 3), "a case closed at once and the other split again cover everything") && passed;
    passed = check_result(!cover(one_side, 1), "one case alone, the other not written, is not a proof") && passed;
    passed = check_result(!cover(nested_missing, 2), "a case split again with one of its cases missing is not a proof") && passed;
    passed = check_result(!cover(two_splits, 2), "two leaves splitting two different disjunctions at the same point are not a proof") && passed;
    passed = check_result(!cover(NULL, 0), "no leaf at all is not a proof") && passed;
    return passed ? 0 : 1;
}
