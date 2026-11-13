/* Test di dominio: teoria degli insiemi.
 *
 * Otto controlli, descritti in tests/statement_checks.h:
 *
 *   1. ogni enunciato di set_operations.tex viene dimostrato, con la sua
 *      dimostrazione scritta;
 *   2. lo stesso per cartesian_products.tex;
 *   3. lo stesso per relations_by_rule.tex;
 *   4. lo stesso per set_quantifiers.tex;
 *   5. nessun enunciato di false_set_statements.tex viene dimostrato;
 *   6. nessun enunciato di false_map_statements.tex viene dimostrato;
 *   7. nessun enunciato di false_relation_statements.tex viene dimostrato;
 *   8. nessun enunciato di false_set_quantifiers.tex viene dimostrato.
 *
 * I controlli da 5 a 8 sono l'antiregressione: impediscono che il motore
 * cominci ad approvare enunciati falsi. Qui non si usano le varianti false
 * ricavate dagli enunciati veri, perché aggiungere 1 a un membro ha senso solo
 * per le uguaglianze tra numeri. Gli enunciati sono dati: il motore non ne
 * conosce nessuno in anticipo.
 *
 * I controlli 3, 6 e 7 stanno qui, e non in test_maps_and_relations.c, perché
 * questo test deve passare sempre: l'altro elenca enunciati ancora da
 * dimostrare, e fallisce finché non lo sono tutti.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_set_theory \
 *         tests/set_theory/test_set_theory.c && /tmp/test_set_theory
 */

#include "../statement_checks.h"

int main(void) {
    engine_init();

    /* Tutti i controlli vengono eseguiti anche se uno fallisce, così il
     * resoconto è completo. */
    int passed = check_all_proved("tests/set_theory/set_operations.tex");
    passed = check_all_proved("tests/set_theory/cartesian_products.tex") && passed;
    passed = check_all_proved("tests/set_theory/relations_by_rule.tex") && passed;
    passed = check_all_proved("tests/set_theory/set_quantifiers.tex") && passed;
    passed = check_none_proved("tests/set_theory/false_set_statements.tex") && passed;
    passed = check_none_proved("tests/set_theory/false_map_statements.tex") && passed;
    passed = check_none_proved("tests/set_theory/false_relation_statements.tex") && passed;
    passed = check_none_proved("tests/set_theory/false_set_quantifiers.tex") && passed;

    /* Ogni fatto scritto in una dimostrazione viene ricontrollato dal
     * programma con un metodo indipendente: nessuno deve essere stato
     * scartato. */
    passed = check_result(unjustified_steps == 0, "every step of the written proofs passed its independent check") && passed;
    return passed ? 0 : 1;
}
