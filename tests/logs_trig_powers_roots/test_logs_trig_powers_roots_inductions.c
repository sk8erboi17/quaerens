/* Test di dominio: logaritmi, trigonometria, potenze e radici.
 *
 * Tre controlli, descritti in tests/statement_checks.h:
 *
 *   1. ogni enunciato di logs_trig_powers_roots_inductions.tex viene
 *      dimostrato per induzione;
 *   2. nessuna variante falsa di quegli enunciati viene dimostrata;
 *   3. nessun enunciato di false_logs_trig_powers_roots_inductions.tex viene
 *      dimostrato.
 *
 * Il controllo 3 viene ripetuto su altri tre file:
 *
 *   - false_undefined_later_logs_roots.tex: enunciati i cui due membri sono
 *     uguali finché esistono, ma smettono di essere definiti da un certo n
 *     in poi (un logaritmo, una radice, una divisione);
 *   - false_division_by_hidden_zero.tex: enunciati che dividono per un
 *     numero che è zero per un'identità, e quindi non sono mai definiti;
 *   - false_log_of_negative_sine_or_cosine.tex: enunciati con il logaritmo
 *     di un seno o di un coseno che non è positivo.
 *
 * I controlli 2 e 3 sono l'antiregressione: impediscono che il motore, per
 * dimostrare di più, cominci ad approvare enunciati falsi. Gli enunciati sono
 * dati: il motore non ne conosce nessuno in anticipo.
 *
 * Si possono passare altri due file come argomenti: prima quello degli
 * enunciati veri, poi quello dei falsi.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_logs_trig_powers_roots \
 *         tests/logs_trig_powers_roots/test_logs_trig_powers_roots_inductions.c \
 *         && /tmp/test_logs_trig_powers_roots
 */

#include "../statement_checks.h"

int main(int argc, char **argv) {
    const char *truths = argc > 1 ? argv[1] : "tests/logs_trig_powers_roots/logs_trig_powers_roots_inductions.tex";
    const char *falsehoods = argc > 2 ? argv[2] : "tests/logs_trig_powers_roots/false_logs_trig_powers_roots_inductions.tex";
    engine_init();

    /* Tutti e tre i controlli vengono eseguiti anche se uno fallisce, così il
     * resoconto è completo. */
    int passed = check_all_proved(truths);
    passed = check_false_variants(truths) && passed;
    passed = check_none_proved(falsehoods) && passed;

    /* Quelli che l'algebra approva e i primi n anche: li ferma solo il
     * controllo che ogni espressione sia definita per ogni n. */
    passed = check_none_proved("tests/logs_trig_powers_roots/false_undefined_later_logs_roots.tex") && passed;

    /* Quelli che dividono per un numero che è zero solo grazie a un'identità
     * (2 cos(pi/3) - 1): li ferma solo il rifiuto di prendere per "diverso
     * da zero" ciò che semplicemente non si semplifica. */
    passed = check_none_proved("tests/logs_trig_powers_roots/false_division_by_hidden_zero.tex") && passed;

    /* Quelli con il logaritmo di un seno o di un coseno negativo: li ferma
     * solo il segno giusto, preso dal quadrante dell'angolo. */
    passed = check_none_proved("tests/logs_trig_powers_roots/false_log_of_negative_sine_or_cosine.tex") && passed;
    return passed ? 0 : 1;
}
