/* Test di dominio: teoria degli insiemi, parte su applicazioni e relazioni.
 *
 * Un controllo, descritto in tests/statement_checks.h: ogni enunciato dei sei
 * file elencati in main() viene dimostrato, con la sua dimostrazione scritta.
 * Sono corrispondenze, applicazioni, relazioni di equivalenza e d'ordine,
 * insiemi finiti e infiniti. Gli enunciati sono dati: il motore non ne conosce
 * nessuno in anticipo.
 *
 * Oggi il motore dimostra una parte di maps.tex, le applicazioni tra insiemi
 * di numeri date da una sola formula, e nulla degli altri file: il test
 * fallisce, e indica il lavoro ancora da fare. Ciò che deve restare vero da
 * subito ha un test a parte, test_set_theory.c: lì stanno gli enunciati sugli
 * insiemi già dimostrati e gli enunciati falsi, anche quelli sulle
 * applicazioni. Il file to_determine.tex non è tra i controllati: contiene
 * domande, non enunciati.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_maps_and_relations \
 *         tests/set_theory/test_maps_and_relations.c && /tmp/test_maps_and_relations
 */

#include "../statement_checks.h"

int main(void) {
    static const char *const files[] = {
        "tests/set_theory/correspondences.tex",
        "tests/set_theory/maps.tex",
        "tests/set_theory/equivalence_relations.tex",
        "tests/set_theory/order_relations.tex",
        "tests/set_theory/finite_sets.tex",
        "tests/set_theory/infinite_sets.tex",
    };
    engine_init();

    /* Tutti i file vengono controllati anche se uno fallisce, così il
     * resoconto è completo. */
    int passed = 1;
    for (size_t i = 0; i < sizeof files / sizeof *files; i++)
        passed = check_all_proved(files[i]) && passed;

    /* Ogni fatto scritto in una dimostrazione viene ricontrollato dal
     * programma con un metodo indipendente: nessuno deve essere stato
     * scartato. */
    passed = check_result(unjustified_steps == 0, "every step of the written proofs passed its independent check") && passed;
    return passed ? 0 : 1;
}
