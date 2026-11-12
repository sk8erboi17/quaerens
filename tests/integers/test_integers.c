/* Test di dominio: numeri interi (parità, resti, divisibilità).
 *
 * Due controlli, descritti in tests/statement_checks.h:
 *
 *   1. ogni enunciato di divisibility.tex viene dimostrato, con la sua
 *      dimostrazione scritta;
 *   2. nessun enunciato di false_divisibility.tex viene dimostrato.
 *
 * Gli enunciati veri chiedono una dimostrazione per casi sul resto di una
 * divisione ("ogni intero è pari o dispari"); quelli falsi valgono in tutti
 * i casi tranne uno, o cambiano l'insieme in modo che un caso resti fuori.
 * Il controllo 2 è l'antiregressione: impedisce che il motore, per
 * dimostrare di più, cominci ad approvare enunciati falsi. Gli enunciati sono
 * dati: il motore non ne conosce nessuno in anticipo.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_integers \
 *         tests/integers/test_integers.c && /tmp/test_integers
 */

#include "../statement_checks.h"

int main(void) {
    engine_init();

    /* Tutti i controlli vengono eseguiti anche se uno fallisce, così il
     * resoconto è completo. */
    int passed = check_all_proved("tests/integers/divisibility.tex");
    passed = check_none_proved("tests/integers/false_divisibility.tex") && passed;

    /* Ogni fatto scritto in una dimostrazione viene ricontrollato dal
     * programma con un metodo indipendente: nessuno deve essere stato
     * scartato. */
    passed = check_result(unjustified_steps == 0, "every step of the written proofs passed its independent check") && passed;
    return passed ? 0 : 1;
}
