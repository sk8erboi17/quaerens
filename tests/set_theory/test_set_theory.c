/* Test di dominio: teoria degli insiemi.
 *
 * Diciassette controlli, descritti in tests/statement_checks.h:
 *
 *   1. ogni enunciato di set_operations.tex viene dimostrato, con la sua
 *      dimostrazione scritta;
 *   2. lo stesso per cartesian_products.tex;
 *   3. lo stesso per relations_by_rule.tex;
 *   4. lo stesso per set_quantifiers.tex;
 *   5. lo stesso per book_notation.tex: immagini, controimmagini e
 *      relazioni scritte come insiemi di coppie (17p e 17q di parser.c);
 *   6. lo stesso per maps_by_cases.tex: applicazioni definite per casi e
 *      con valori assoluti (17s di parser.c, 18v di main.c);
 *   7. nessun enunciato di false_set_statements.tex viene dimostrato;
 *   8. nessun enunciato di false_map_statements.tex viene dimostrato;
 *   9. nessun enunciato di false_relation_statements.tex viene dimostrato;
 *  10. nessun enunciato di false_set_quantifiers.tex viene dimostrato;
 *  11. nessun enunciato di false_book_relations.tex viene dimostrato;
 *  12. nessun enunciato di false_book_maps.tex viene dimostrato;
 *  13. nessun enunciato di false_maps_by_cases.tex viene dimostrato;
 *  14. ogni enunciato di part_hypotheses.tex viene dimostrato: un'ipotesi su
 *      ogni parte di un insieme, o l'esistenza di una parte (17u di
 *      parser.c, 28i-28l di main.c);
 *  15. nessun enunciato di false_part_hypotheses.tex viene dimostrato;
 *  16. ogni enunciato di parts_and_maps.tex viene dimostrato: le parti e le
 *      applicazioni come oggetti, gli ordini scritti tra gli elementi, le
 *      relazioni date dall'elenco delle loro coppie (17v, 28m-28o);
 *  17. nessun enunciato di false_parts_and_maps.tex viene dimostrato.
 *
 * I controlli da 7 a 13, il 15 e il 17 sono l'antiregressione: impediscono che il motore
 * cominci ad approvare enunciati falsi. Qui non si usano le varianti false
 * ricavate dagli enunciati veri, perché aggiungere 1 a un membro ha senso solo
 * per le uguaglianze tra numeri. Gli enunciati sono dati: il motore non ne
 * conosce nessuno in anticipo.
 *
 * I controlli 3, 5 e da 6 a 17 stanno qui, e non in
 * test_maps_and_relations.c, perché questo test deve passare sempre: l'altro
 * elenca enunciati ancora da dimostrare, e fallisce finché non lo sono
 * tutti.
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
    passed = check_all_proved("tests/set_theory/book_notation.tex") && passed;
    passed = check_all_proved("tests/set_theory/maps_by_cases.tex") && passed;
    passed = check_none_proved("tests/set_theory/false_set_statements.tex") && passed;
    passed = check_none_proved("tests/set_theory/false_map_statements.tex") && passed;
    passed = check_none_proved("tests/set_theory/false_relation_statements.tex") && passed;
    passed = check_none_proved("tests/set_theory/false_set_quantifiers.tex") && passed;
    passed = check_none_proved("tests/set_theory/false_book_relations.tex") && passed;
    passed = check_none_proved("tests/set_theory/false_book_maps.tex") && passed;
    passed = check_none_proved("tests/set_theory/false_maps_by_cases.tex") && passed;
    passed = check_all_proved("tests/set_theory/part_hypotheses.tex") && passed;
    passed = check_none_proved("tests/set_theory/false_part_hypotheses.tex") && passed;

    /* I verdetti sulla notazione dei libri vengono dal capitolo 28: ognuno
     * porta un certificato che il verificatore accetta, e alterato no
     * (controllo 5). I numeri sono quanti verdetti di quel capitolo ci si
     * aspetta almeno: tutti gli enunciati veri, e i falsi che un modello
     * finito smentisce. */
    passed = check_structure_certificates("tests/set_theory/book_notation.tex", 23) && passed;
    passed = check_structure_certificates("tests/set_theory/false_book_relations.tex", 15) && passed;
    passed = check_structure_certificates("tests/set_theory/false_book_maps.tex", 16) && passed;
    passed = check_structure_certificates("tests/set_theory/part_hypotheses.tex", 21) && passed;
    passed = check_structure_certificates("tests/set_theory/false_part_hypotheses.tex", 19) && passed;

    /* Controlli 16 e 17: le parti e le applicazioni come oggetti (17v,
     * 28m-28o), con i loro certificati (11h di checker.c). */
    passed = check_all_proved("tests/set_theory/parts_and_maps.tex") && passed;
    passed = check_none_proved("tests/set_theory/false_parts_and_maps.tex") && passed;
    passed = check_structure_certificates("tests/set_theory/parts_and_maps.tex", 16) && passed;
    passed = check_structure_certificates("tests/set_theory/false_parts_and_maps.tex", 16) && passed;

    /* Ogni fatto scritto in una dimostrazione viene ricontrollato dal
     * programma con un metodo indipendente: nessuno deve essere stato
     * scartato. */
    passed = check_result(unjustified_steps == 0, "every step of the written proofs passed its independent check") && passed;
    return passed ? 0 : 1;
}
