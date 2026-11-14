/* Test di dominio: teoria degli insiemi, parte su applicazioni e relazioni.
 *
 * Un controllo, descritto in tests/statement_checks.h: ogni enunciato dei sei
 * file elencati in main() viene dimostrato, con la sua dimostrazione scritta.
 * Sono corrispondenze, applicazioni, relazioni di equivalenza e d'ordine,
 * insiemi finiti e infiniti. Gli enunciati sono dati: il motore non ne conosce
 * nessuno in anticipo.
 *
 * Oggi il motore dimostra tutti i 16 enunciati di correspondences.tex, 47
 * dei 54 di maps.tex, 23 dei 28 di equivalence_relations.tex, 9 dei 14 di
 * order_relations.tex, 1 dei 7 di finite_sets.tex e 4 dei 17 di
 * infinite_sets.tex: il test fallisce, e indica il lavoro ancora da fare.
 * Gli enunciati dimostrati sono quelli sulle applicazioni tra insiemi di
 * numeri date da una formula, anche per casi, con un valore assoluto, con
 * dei parametri o a valori in coppie (17s, 18v), anche tra numeri reali o
 * complessi, con la definizione svolta (18v-8, 18v-11), sulle loro iterate
 * (26c), e
 * sulle relazioni tra numeri date da una regola, anche con i divisori, le
 * potenze, i numeri reali senza ordine, le coppie e i numeri complessi a +
 * ib (capitoli 17-18, 17t, 18w-18z), e quelli su insiemi qualsiasi
 * scritti come nei libri: immagini e
 * controimmagini, relazioni come insiemi di coppie, inverse e
 * composizioni, coppie, relazioni e applicazioni tra le parti di un insieme
 * (17p-17r di parser.c, capitoli 28 e 15), quelli con un'ipotesi su tutte
 * le parti di un insieme, come le caratterizzazioni delle applicazioni
 * iniettive e suriettive con le immagini e le controimmagini (17u,
 * 28i-28l), e quelli che prendono \mathcal{P}(S) e T^{S} come insiemi di
 * oggetti, con le famiglie di parti e le relazioni d'ordine scritte tra gli
 * elementi, e le relazioni date dall'elenco delle loro coppie di numeri
 * (17v, 28m-28o), comprese le dimostrazioni più lunghe sulle famiglie di
 * parti (una partizione comune, la controimmagine e l'immagine di una
 * partizione), sulla composizione di due relazioni di equivalenza e sugli
 * ordini tra parti, tra applicazioni e tra classi, le cui istanze sceglie
 * un modello candidato (28r), e l'ordine punto per punto su S^{S}, che
 * chiede le applicazioni con due valori (28s); e le equipotenze, con una
 * biiezione costruita (17w, 28q). Mancano soprattutto le applicazioni da
 * costruire con una regola qualsiasi, il quoziente, il numero di elementi e
 * gli insiemi infiniti, il massimo comun divisore e i numeri primi, le
 * applicazioni di due argomenti che non vengono da \mathbb{C}: il verdetto
 * dice quale.
 *
 * Ciò che deve restare vero da subito ha un test a parte,
 * test_set_theory.c: lì stanno gli enunciati sugli insiemi già dimostrati,
 * altri della stessa notazione dei libri, e gli enunciati falsi, anche
 * quelli sulle applicazioni. Il file to_determine.tex non è tra i
 * controllati: contiene domande, non enunciati.
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
