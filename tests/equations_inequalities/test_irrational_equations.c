/* Test di dominio: equazioni irrazionali, cioè con l'incognita sotto una o
 * più radici quadrate. Fa parte del dominio delle equazioni e disequazioni
 * in una variabile reale, come test_equations_inequalities.c.
 *
 * Quattro controlli. I primi due sono quelli di tests/statement_checks.h:
 *
 *   1. ogni enunciato di book_irrational_equations.tex viene dimostrato:
 *      dice che un'equazione vale esattamente per i valori stampati come
 *      risultato su un libro di scuola;
 *   2. nessun enunciato di false_irrational_equations.tex viene dimostrato:
 *      sono gli stessi risultati con una soluzione dimenticata, spostata, o
 *      con in più una radice dell'equazione elevata al quadrato che non
 *      soddisfa quella di partenza;
 *   3. ogni richiesta di irrational_equations_to_solve.tex viene risolta, e
 *      le soluzioni sono quelle stampate sul libro, scritte allo stesso modo;
 *   4. tutti i 30 enunciati falsi vengono dichiarati falsi, non solo "non
 *      dimostrati".
 *
 * Il metodo sotto esame è la tabella dei segni con le radici quadrate
 * (regola 18p-7 di main.c): ogni radice diventa una lettera, e il segno di
 * P + Q s si legge da quelli di P, di Q e di P^2 - Q^2 R. È il metodo
 * generale, non "elevare al quadrato e scartare le radici estranee": le
 * radici estranee qui non nascono. I dati vengono dal libro; un oracolo
 * indipendente (tools/fuzz/fuzz_radicals_parameters.py) ha confermato i
 * risultati e trovato per ogni enunciato falso il punto in cui sbaglia.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_irrational_equations \
 *         tests/equations_inequalities/test_irrational_equations.c \
 *         && /tmp/test_irrational_equations
 */

#include "../statement_checks.h"

/* Il risultato atteso di una richiesta di irrational_equations_to_solve.tex:
 * il numero dell'esercizio sul libro e le soluzioni come le stampa il libro.
 * Invariante: le voci sono nell'ordine delle richieste del file. */
typedef struct {
    int exercise;
    const char *text;
} ExpectedSolution;

static const ExpectedSolution expected_solutions[] = {
    {755, "x = 3 \\lor x = 9"},
    {756, "x = 1"},
    {757, "x = 5"},
    {758, "x = 1"},
    {759, "x = 4"},
    {760, "x = -\\frac{12}{5} \\lor x = 0"},
    {761, "x = -1"},
    {762, "x = 1"},
    {763, "x = 5"},
    {764, "x = 3 \\lor x = 4"},
};

/* Controllo 3: ogni richiesta di path è risolta, e le soluzioni scritte sono
 * expected_solutions[], nell'ordine. Ritorna 1 se il controllo è riuscito. */
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
        if (total <= expected && verdict.outcome == SOLVED && strcmp(verdict.detail, expected_solutions[total - 1].text) == 0 && verdict.proof) {
            right++;
            continue;
        }
        printf("  request %d (exercise %d): expected %s, got %s\n", total, total <= expected ? expected_solutions[total - 1].exercise : 0,
               total <= expected ? expected_solutions[total - 1].text : "nothing", verdict.outcome == SOLVED ? verdict.detail : "no solutions written");
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d solution sets found and equal to the expected ones", right, total);
    return check_result(total == expected && right == total, summary);
}

/* Controllo 4: ogni enunciato di path, che è falso, viene dichiarato falso.
 * Ritorna 1 se il controllo è riuscito. */
static int check_all_refuted(const char *path) {
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }
    const char *position = text, *item, *end;
    int total = 0, refuted = 0;
    while (next_item(&position, &item, &end)) {
        total++;
        if (prove_item(item, end).outcome == REFUTED) refuted++;
        else printf("  statement %d of %s is not refuted\n", total, path);
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d false statements refuted in %s", refuted, total, path);
    return check_result(refuted == total, summary);
}

int main(void) {
    engine_init();

    /* Tutti i controlli vengono eseguiti anche se uno fallisce, così il
     * resoconto è completo. */
    int passed = check_all_proved("tests/equations_inequalities/book_irrational_equations.tex");
    passed = check_none_proved("tests/equations_inequalities/false_irrational_equations.tex") && passed;
    passed = check_solutions("tests/equations_inequalities/irrational_equations_to_solve.tex") && passed;
    passed = check_all_refuted("tests/equations_inequalities/false_irrational_equations.tex") && passed;

    /* Ogni riga di una tabella dei segni e ogni soluzione scritta vengono
     * ricontrollate dal programma con un metodo indipendente: nessuna deve
     * essere stata scartata. */
    passed = check_result(unjustified_steps == 0, "every row of the sign tables and every written solution passed its independent check") && passed;
    return passed ? 0 : 1;
}
