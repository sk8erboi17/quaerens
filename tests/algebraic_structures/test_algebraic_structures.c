/* Test di dominio: strutture algebriche (operazioni e relazioni date da una
 * regola o su un insieme qualsiasi, congruenze).
 *
 * Otto controlli, descritti in tests/statement_checks.h:
 *
 *   1. ogni enunciato di operations.tex viene dimostrato, con la sua
 *      dimostrazione scritta;
 *   2. nessun enunciato di false_operations.tex viene dimostrato;
 *   3. ogni enunciato di book_structures.tex viene dimostrato: sono le
 *      affermazioni che gli esercizi di un libro universitario chiedono di
 *      provare, scritte per esteso con i quantificatori;
 *   4. nessun enunciato di book_structures_beyond.tex viene dichiarato
 *      falso: vengono dagli stessi esercizi, ma il motore non li sa ancora
 *      decidere, o non ne sa leggere la notazione;
 *   5. ogni enunciato di witnesses.tex viene dimostrato: ipotesi "esiste
 *      k" usate con il loro testimone, congruenze modulo una lettera
 *      (regola 18u);
 *   6. nessun enunciato di false_witnesses.tex viene dimostrato;
 *   7. ogni enunciato di abstract_structures.tex viene dimostrato: un
 *      insieme qualsiasi con un'operazione, delle applicazioni, una
 *      relazione di cui si conoscono solo le ipotesi (capitolo 28);
 *   8. nessun enunciato di false_abstract_structures.tex viene dimostrato.
 *
 * Gli enunciati dichiarano un'operazione con i suoi insiemi e la sua regola,
 * e ne affermano le proprietà scritte per esteso: associativa, commutativa,
 * con elemento neutro, regolare. Il motore non conosce queste parole: vede
 * quantificatori e uguaglianze, e la regola al posto dell'operazione. I
 * controlli 2, 6 e 8 sono l'antiregressione: una proprietà affermata
 * dell'operazione sbagliata, la negazione di una proprietà vera, una regola
 * che esce dall'insieme; un'ipotesi tolta, un modulo cambiato; due elementi
 * neutri dallo stesso lato, un enunciato vero solo sugli insiemi finiti o
 * solo su quelli non vuoti. Gli enunciati sono dati: il motore non ne
 * conosce nessuno in anticipo.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_algebraic_structures \
 *         tests/algebraic_structures/test_algebraic_structures.c && /tmp/test_algebraic_structures
 */

#include "../statement_checks.h"

int main(void) {
    engine_init();

    /* Tutti i controlli vengono eseguiti anche se uno fallisce, così il
     * resoconto è completo. */
    int passed = check_all_proved("tests/algebraic_structures/operations.tex");
    passed = check_none_proved("tests/algebraic_structures/false_operations.tex") && passed;
    passed = check_all_proved("tests/algebraic_structures/book_structures.tex") && passed;
    passed = check_none_refuted("tests/algebraic_structures/book_structures_beyond.tex") && passed;
    passed = check_all_proved("tests/algebraic_structures/witnesses.tex") && passed;
    passed = check_none_proved("tests/algebraic_structures/false_witnesses.tex") && passed;
    passed = check_all_proved("tests/algebraic_structures/abstract_structures.tex") && passed;
    passed = check_none_proved("tests/algebraic_structures/false_abstract_structures.tex") && passed;

    /* Ogni fatto scritto in una dimostrazione viene ricontrollato dal
     * programma con un metodo indipendente: nessuno deve essere stato
     * scartato. */
    passed = check_result(unjustified_steps == 0, "every step of the written proofs passed its independent check") && passed;
    return passed ? 0 : 1;
}
