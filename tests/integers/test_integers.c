/* Test di dominio: numeri interi (parità, resti, divisibilità, congruenze).
 *
 * Cinque controlli. I primi quattro sono quelli di tests/statement_checks.h:
 *
 *   1. ogni enunciato di divisibility.tex viene dimostrato, con la sua
 *      dimostrazione scritta;
 *   2. nessun enunciato di false_divisibility.tex viene dimostrato;
 *   3. ogni enunciato di congruences.tex viene dimostrato, con la sua
 *      dimostrazione scritta;
 *   4. nessun enunciato di false_congruences.tex viene dimostrato;
 *   5. ogni richiesta di congruence_solutions.tex viene risolta, e le
 *      soluzioni sono quelle attese, scritte qui sotto.
 *
 * Gli enunciati veri di divisibility.tex chiedono una dimostrazione per casi
 * sul resto di una divisione ("ogni intero è pari o dispari"); quelli falsi
 * valgono in tutti i casi tranne uno, o cambiano l'insieme in modo che un
 * caso resti fuori. Quelli di congruences.tex sono fatti solo di congruenze
 * con moduli fissati, e li decide la regola 18r di main.c, provando tutti i
 * resti: tra i falsi ci sono le soluzioni sbagliate di poco (un resto
 * vicino, un modulo troppo piccolo, una classe dimenticata). I controlli 2 e
 * 4 sono l'antiregressione: impediscono che il motore, per dimostrare di
 * più, cominci ad approvare enunciati falsi. Gli enunciati sono dati: il
 * motore non ne conosce nessuno in anticipo.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_integers \
 *         tests/integers/test_integers.c && /tmp/test_integers
 */

#include "../statement_checks.h"

/* Le soluzioni attese delle richieste di congruence_solutions.tex,
 * nell'ordine del file. Non sono copiate dall'uscita del programma: le prime
 * undici sono le risposte degli esercizi di un libro (congruenze lineari, e
 * un sistema di tre), ricavate a mano con l'inverso del coefficiente e
 * ricontrollate sostituendo; le altre sono state ricavate a mano, guardando
 * i resti uno per uno. La 12 elenca gli interi invertibili modulo 12, che
 * sono quelli primi con 12: il programma li scrive con il periodo più
 * piccolo, 6. */
static const char *const expected_solutions[] = {
    "x \\equiv 16 \\pmod{36}",
    "x \\equiv 3 \\pmod{5}",
    "x \\equiv 7 \\pmod{29}",
    "x \\equiv 2 \\pmod{5}",
    "x \\equiv 3 \\pmod{7}",
    "x \\equiv 3 \\pmod{8}",
    "x \\equiv 2 \\pmod{3}",
    "x \\equiv 5 \\pmod{9}",
    "x \\equiv 2 \\pmod{3}",
    "x \\equiv 4 \\pmod{5}",
    "x \\equiv 53 \\pmod{60}",
    "a \\equiv 1 \\pmod{6} \\lor a \\equiv 5 \\pmod{6}",
    "a \\equiv 1 \\pmod{7} \\lor a \\equiv 2 \\pmod{7} \\lor a \\equiv 3 \\pmod{7} \\lor a \\equiv 4 \\pmod{7} \\lor a \\equiv 5 \\pmod{7} \\lor a \\equiv 6 \\pmod{7}",
    "\\nexists x \\in \\mathbb{Z}",
    "\\forall x \\in \\mathbb{Z}",
    "x \\equiv 1 \\pmod{2}",
    "x \\equiv 0 \\pmod{3} \\lor x \\equiv 2 \\pmod{3}",
    "\\nexists x \\in \\mathbb{Z}",
};

/* Controllo 5: ogni richiesta di path è risolta, e le soluzioni scritte sono
 * expected_solutions[], nell'ordine. Ritorna 1 se il controllo è riuscito.
 *
 * Ha la stessa forma del controllo sulle soluzioni delle disequazioni, in
 * tests/equations_inequalities: resta qui, e non in statement_checks.h,
 * perché ogni dominio ha il suo elenco di risposte attese. */
static int check_solutions(const char *path) {
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }
    const char *position = text, *item, *end;
    int total = 0, right = 0, expected = (int)(sizeof expected_solutions / sizeof *expected_solutions);
    while (next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        total++;
        if (total <= expected && verdict.outcome == SOLVED && strcmp(verdict.detail, expected_solutions[total - 1]) == 0 && verdict.proof) {
            right++;
            continue;
        }
        printf("  request %d: expected %s, got %s\n", total, total <= expected ? expected_solutions[total - 1] : "nothing",
               verdict.outcome == SOLVED ? verdict.detail : "no solution set");
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d solution sets found and equal to the expected ones", right, total);
    return check_result(total == expected && right == total, summary);
}

int main(void) {
    engine_init();

    /* Tutti i controlli vengono eseguiti anche se uno fallisce, così il
     * resoconto è completo. */
    int passed = check_all_proved("tests/integers/divisibility.tex");
    passed = check_none_proved("tests/integers/false_divisibility.tex") && passed;
    passed = check_all_proved("tests/integers/congruences.tex") && passed;
    passed = check_none_proved("tests/integers/false_congruences.tex") && passed;
    passed = check_solutions("tests/integers/congruence_solutions.tex") && passed;

    /* Ogni fatto scritto in una dimostrazione, ogni conto sui resti e ogni
     * soluzione scritta vengono ricontrollati dal programma con un metodo
     * indipendente: nessuno deve essere stato scartato. */
    passed = check_result(unjustified_steps == 0, "every step of the written proofs passed its independent check") && passed;
    return passed ? 0 : 1;
}
