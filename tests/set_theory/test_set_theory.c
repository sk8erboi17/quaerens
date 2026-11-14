/* Test di dominio: teoria degli insiemi.
 *
 * Diciannove controlli, descritti in tests/statement_checks.h, e due scritti
 * qui (16 e 17):
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
 *  14. ogni enunciato di number_relations.tex viene dimostrato: divisori,
 *      potenze e coppie nelle relazioni tra numeri (18w e seguenti di
 *      main.c, 17t di parser.c);
 *  15. nessun enunciato di false_number_relations.tex viene dimostrato;
 *  16. e almeno 48 di loro sono dichiarati falsi (check_refuted_at_least()).
 *      Il controllo 15 da solo non basta: una regola rotta apposta, che
 *      dimenticava i divisori negativi (18w), "dimostrava" due di quegli
 *      enunciati, e la verifica indipendente del capitolo 19 li fermava con
 *      un NON DIMOSTRATO, che il controllo 15 accetta;
 *  17. il controesempio di una relazione tra coppie si scrive con le
 *      componenti al loro posto (check_counterexample()). Una copia del
 *      lettore che scambiava le componenti delle lettere che stanno per una
 *      coppia (17t) dava ancora i verdetti giusti, perché lo scambio era lo
 *      stesso dappertutto, ma controesempi sbagliati: lo ha visto solo
 *      fuzz_number_relations.py, e questo controllo lo fissa;
 *  18. ogni enunciato di part_hypotheses.tex viene dimostrato: un'ipotesi su
 *      ogni parte di un insieme, o l'esistenza di una parte (17u di
 *      parser.c, 28i-28l di main.c);
 *  19. nessun enunciato di false_part_hypotheses.tex viene dimostrato;
 *  20. ogni enunciato di parts_and_maps.tex viene dimostrato: le parti e le
 *      applicazioni come oggetti, gli ordini scritti tra gli elementi, le
 *      relazioni date dall'elenco delle loro coppie (17v, 28m-28o);
 *  21. nessun enunciato di false_parts_and_maps.tex viene dimostrato.
 *
 * Inoltre i verdetti del capitolo 28 portano un certificato che il
 * verificatore accetta (check_structure_certificates()), e ogni passo delle
 * dimostrazioni scritte supera il suo controllo indipendente.
 *
 * I controlli da 7 a 13, il 15, il 16, il 19 e il 21 sono
 * l'antiregressione: impediscono che il motore cominci ad approvare
 * enunciati falsi. Qui non si usano le varianti false ricavate dagli
 * enunciati veri, perché aggiungere 1 a un membro ha senso solo per le
 * uguaglianze tra numeri. Gli enunciati sono dati: il motore non ne conosce
 * nessuno in anticipo.
 *
 * I controlli 3, 5 e da 6 a 21 stanno qui, e non in
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

/* Controllo 16: almeno at_least enunciati di path, un file di enunciati
 * falsi, vengono dichiarati FALSO, e nessuno dimostrato. Lo stesso controllo
 * di test_integrals.c, ripetuto qui perché i test non dipendano l'uno
 * dall'altro. */
static int check_refuted_at_least(const char *path, int at_least) {
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }
    const char *position = text, *item, *end;
    int total = 0, refuted = 0, proved = 0;
    while (next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        total++;
        if (verdict.outcome == REFUTED) refuted++;
        if (verdict.outcome == PROVED) proved++;
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d statements of %s are REFUTED (at least %d wanted), %d are PROVED (none allowed)",
             refuted, total, path, at_least, proved);
    return check_result(refuted >= at_least && proved == 0, summary);
}

/* Controllo 17: l'enunciato text è dichiarato FALSO con il controesempio
 * expected, scritto proprio così. */
static int check_counterexample(const char *text, const char *expected) {
    char summary[512];
    Verdict verdict = prove(text, 'n', 0, 0);
    const char *found = verdict.outcome == REFUTED && verdict.detail ? verdict.detail : "missing";
    snprintf(summary, sizeof summary, "the counterexample to the symmetry of x_{1} = y_{2} between pairs is %s (expected %s)", found, expected);
    return check_result(strcmp(found, expected) == 0, summary);
}

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
    passed = check_all_proved("tests/set_theory/number_relations.tex") && passed;
    passed = check_none_proved("tests/set_theory/false_number_relations.tex") && passed;
    passed = check_refuted_at_least("tests/set_theory/false_number_relations.tex", 48) && passed;
    passed = check_counterexample("R = \\{((x_{1}, y_{1}), (x_{2}, y_{2})) \\in (\\mathbb{Z}^{2})^{2} \\mid x_{1} = y_{2}\\} \\implies (\\forall x, y\\ ((x, y) \\in R \\implies (y, x) \\in R))",
                                  "x = (0, 0), y = (1, 0)") && passed;
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

    /* Controlli 20 e 21: le parti e le applicazioni come oggetti (17v,
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
