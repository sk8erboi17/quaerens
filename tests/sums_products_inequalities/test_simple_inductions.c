/* Test di dominio: somme, prodotti, disuguaglianze.
 *
 * Tre controlli, descritti in tests/statement_checks.h:
 *
 *   1. ogni enunciato di simple_inductions.tex viene dimostrato per induzione;
 *   2. nessuna variante falsa di quegli enunciati viene dimostrata;
 *   3. nessun enunciato di false_simple_inductions.tex viene dimostrato.
 *
 * I controlli 1 e 2 vengono ripetuti su finite_sums_and_products.tex, e il
 * controllo 3 su altri due file:
 *
 *   - false_hidden_errors.tex: enunciati falsi i cui due membri coincidono
 *     per i primi undici valori di n o più, cioè oltre quelli in cui il
 *     motore cerca un controesempio;
 *   - false_undefined_later.tex: enunciati i cui due membri sono uguali
 *     finché esistono, ma smettono di essere definiti da un certo n in poi.
 *
 * Infine le stesse formule scritte con il quantificatore, "per ogni n in N":
 * quelle vere di quantified_inductions.tex vanno dimostrate (controllo 1),
 * quelle di false_quantified_inductions.tex no (controllo 3). Passano dalle
 * regole sui quantificatori, che chiamano l'induzione come una loro regola.
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
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_simple_inductions \
 *         tests/sums_products_inequalities/test_simple_inductions.c \
 *         && /tmp/test_simple_inductions
 */

#include "../statement_checks.h"

int main(int argc, char **argv) {
    const char *truths = argc > 1 ? argv[1] : "tests/sums_products_inequalities/simple_inductions.tex";
    const char *falsehoods = argc > 2 ? argv[2] : "tests/sums_products_inequalities/false_simple_inductions.tex";
    engine_init();

    /* Tutti e tre i controlli vengono eseguiti anche se uno fallisce, così il
     * resoconto è completo. */
    int passed = check_all_proved(truths);
    passed = check_false_variants(truths) && passed;
    passed = check_none_proved(falsehoods) && passed;

    /* Un secondo file di enunciati veri dello stesso dominio, con gli stessi
     * due controlli. */
    const char *more = "tests/sums_products_inequalities/finite_sums_and_products.tex";
    passed = check_all_proved(more) && passed;
    passed = check_false_variants(more) && passed;

    /* Gli enunciati falsi più insidiosi: solo il passo induttivo li ferma. */
    passed = check_none_proved("tests/sums_products_inequalities/false_hidden_errors.tex") && passed;

    /* Quelli che l'algebra approva e i primi n anche: li ferma solo il
     * controllo che ogni espressione sia definita per ogni n. */
    passed = check_none_proved("tests/sums_products_inequalities/false_undefined_later.tex") && passed;

    /* Le formule scritte con il quantificatore: l'induzione come regola
     * degli enunciati con quantificatori. */
    passed = check_all_proved("tests/sums_products_inequalities/quantified_inductions.tex") && passed;
    passed = check_none_proved("tests/sums_products_inequalities/false_quantified_inductions.tex") && passed;

    /* Nessun passo delle dimostrazioni scritte deve aver fallito il suo
     * controllo indipendente. */
    passed = check_result(unjustified_steps == 0, "every step of the written proofs passed its independent check") && passed;
    return passed ? 0 : 1;
}
