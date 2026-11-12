/* Test di dominio: equazioni e disequazioni in una variabile reale.
 *
 * Cinque controlli. I primi tre sono quelli di tests/statement_checks.h:
 *
 *   1. ogni enunciato di book_inequalities.tex (esercizi di un libro, con il
 *      risultato stampato) e di sign_table.tex (dal fuzz, con il suo oracolo
 *      indipendente) viene dimostrato, con la dimostrazione scritta;
 *   2. nessun enunciato di false_inequalities.tex viene dimostrato;
 *   3. anche gli enunciati di book_inequalities_with_radicals.tex, i cui
 *      risultati contengono radici quadrate, vengono dimostrati;
 *   4. ogni richiesta di solution_sets.tex viene risolta, e le soluzioni
 *      sono quelle attese, scritte qui sotto;
 *   5. gli enunciati in cui conta l'insieme su cui varia la variabile
 *      ricevono la risposta giusta: ciò che è vero sui reali può essere falso
 *      sui razionali o sugli interi, e viceversa.
 *
 * Il metodo sotto esame è la tabella dei segni (regola 18p di main.c). I
 * controlli 2 e 5 sono l'antiregressione: impediscono che, per risolvere un
 * esercizio in più, il motore cominci ad approvare risultati sbagliati di
 * poco (un estremo compreso al posto di uno escluso, un punto in cui un
 * membro non è definito). Gli enunciati sono dati: il motore non ne conosce
 * nessuno in anticipo.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_equations_inequalities \
 *         tests/equations_inequalities/test_equations_inequalities.c \
 *         && /tmp/test_equations_inequalities
 */

#include "../statement_checks.h"

/* Le soluzioni attese delle richieste di solution_sets.tex, nell'ordine del
 * file, scritte come le scrive un libro. Non sono copiate dall'uscita del
 * programma: la 2, la 6, la 8 e la 12 sono i risultati stampati sul libro
 * degli esercizi 626, 345, 480 e 675; le altre sono state ricavate a mano.
 * Nella 7 r_{1} è l'unica radice reale di x^3 - x - 1, che non ha una
 * scrittura con i radicali quadratici; nella 18 r_{1} e r_{2} sono le radici
 * cubiche di 2 e di 3. La 17 chiede di scomporre x^4 - 5x^2 + 6 in due
 * fattori di secondo grado (18p-6). Le ultime sei sono i risultati stampati
 * sul libro degli esercizi 353, 480, 508, 631, 686 e 688, quelli con una
 * radice quadrata: il motore li deve trovare da sé, scritti allo stesso modo. */
static const char *const expected_solutions[] = {
    "x < -3 \\lor x \\ge 2",
    "x < \\frac{3}{5} \\lor x > 1",
    "-\\sqrt{2} < x < \\sqrt{2}",
    "\\nexists x \\in \\mathbb{R}",
    "\\forall x \\in \\mathbb{R}",
    "x \\le -\\frac{1}{2} \\lor x = -\\frac{1}{4} \\lor x \\ge 4",
    "x > r_{1}",
    "-\\sqrt{6} < x < 0",
    "x = -\\frac{\\sqrt{6}}{3} \\lor 1 - \\sqrt{2} \\le x \\le 1 + \\sqrt{2}",
    "0 < x \\le 2",
    "-2 \\le x < 1 \\lor 1 < x \\le 2",
    "-1 \\le x < 2 \\lor 2 < x < 3 \\lor 3 < x \\le 5",
    "x = \\frac{2 - \\sqrt{6}}{2} \\lor x = \\frac{2 + \\sqrt{6}}{2}",
    "-1 \\le x < 0 \\lor x \\ge 1",
    "x = -3 \\lor x = 3",
    "-3 \\le x \\le -1 \\lor 1 \\le x \\le 3",
    "-\\sqrt{3} \\le x \\le -\\sqrt{2} \\lor \\sqrt{2} \\le x \\le \\sqrt{3}",
    "r_{1} < x < r_{2}",
    "-2 < x < 1 - \\sqrt{2} \\lor 1 < x < 1 + \\sqrt{2}",
    "-\\sqrt{6} < x < 0",
    "x \\le -\\sqrt{3} \\lor x \\ge \\sqrt{3}",
    "\\sqrt{3} \\le x \\le 3",
    "2 - \\sqrt{6} \\le x \\le \\sqrt{2}",
    "2 - \\sqrt{6} < x < 2 + \\sqrt{6}",
};

/* Un enunciato in cui conta l'insieme su cui varia la variabile, o la forma
 * della richiesta, con l'esito atteso. */
typedef struct {
    const char *name;       /* che cosa controlla, per i messaggi */
    const char *statement;  /* l'enunciato, in LaTeX */
    Outcome expected;       /* l'esito che il motore deve dare */
} SetCase;

static const SetCase set_cases[] = {
    /* La radice di 2 non è un razionale: lo stesso enunciato cambia verità
     * passando dai reali ai razionali. */
    {"a positive real number has square 2", "\\exists x \\in \\mathbb{R}\\ (x^{2} = 2 \\land x > 0)", PROVED},
    {"no positive rational number has square 2", "\\exists x \\in \\mathbb{Q}\\ (x^{2} = 2 \\land x > 0)", REFUTED},
    {"on the rationals a square is never 2", "\\forall x \\in \\mathbb{Q}\\ (x^{2} < 2 \\iff x^{2} \\le 2)", PROVED},
    {"on the reals a square can be 2", "\\forall x \\in \\mathbb{R}\\ (x^{2} < 2 \\iff x^{2} \\le 2)", REFUTED},

    /* Gli interi non sono un intervallo: tra 0 e 1 c'è un reale ma nessun
     * intero. La tabella dei segni non li tratta, e deve dirlo invece di
     * rispondere come se fossero reali. */
    {"an order statement on the integers is not decided as if on the reals", "\\exists x \\in \\mathbb{Z}\\ (0 < 2x < 2)", UNSUPPORTED},
    {"the same statement on the reals is true", "\\exists x \\in \\mathbb{R}\\ (0 < 2x < 2)", PROVED},

    /* L'insieme ristretto: lo zero e i negativi non contano in R+. */
    {"on the positive reals x + 1/x is at least 2", "\\forall x \\in \\mathbb{R}^{+}\\ \\left(x + \\frac{1}{x} \\ge 2\\right)", PROVED},
    {"on all the reals it is not", "\\forall x \\in \\mathbb{R}\\ \\left(x + \\frac{1}{x} \\ge 2\\right)", REFUTED},
    {"a removed point is not a counterexample", "\\forall x \\in \\mathbb{R} \\setminus \\{0\\}\\ \\left(\\frac{x}{x} = 1\\right)", PROVED},

    /* Una richiesta di soluzioni sta solo in cima, e solo sui reali. */
    {"solutions are searched only in the reals", "\\{x \\in \\mathbb{Q} \\mid x^{2} \\le 4\\}", UNSUPPORTED},
    {"a solution set is not a condition", "\\forall y \\in \\mathbb{R}\\ (\\{x \\in \\mathbb{R} \\mid x < y\\})", UNSUPPORTED},
    {"a square root has no value on the negative numbers", "\\{x \\in \\mathbb{R} \\mid \\sqrt{x} < 2\\}", SOLVED},
    {"the square root of a square is not the number itself", "\\forall x \\in \\mathbb{R}\\ (\\sqrt{x^{2}} = x)", REFUTED},
    {"it is its absolute value", "\\forall x \\in \\mathbb{R}\\ (\\sqrt{x^{2}} = \\left|x\\right|)", PROVED},
    {"the square of a square root is the number only where the root exists", "\\forall x \\in \\mathbb{R}\\ ((\\sqrt{x})^{2} = x)", REFUTED},
    {"a condition with a logarithm is beyond the sign table", "\\{x \\in \\mathbb{R} \\mid \\ln x < 2\\}", UNSUPPORTED},
};

/* Nome di un esito, per i messaggi. */
static const char *outcome_name(Outcome outcome) {
    switch (outcome) {
    case PROVED:      return "PROVED";
    case REFUTED:     return "REFUTED";
    case NOT_PROVED:  return "NOT_PROVED";
    case UNSUPPORTED: return "UNSUPPORTED";
    case SOLVED:      return "SOLVED";
    }
    return "?";
}

/* Controllo 4: ogni richiesta di path è risolta, e le soluzioni scritte sono
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
        if (total <= expected && verdict.outcome == SOLVED && strcmp(verdict.detail, expected_solutions[total - 1]) == 0 && verdict.proof) {
            right++;
            continue;
        }
        printf("  request %d: expected %s, got %s %s\n", total, total <= expected ? expected_solutions[total - 1] : "nothing",
               outcome_name(verdict.outcome), verdict.outcome == SOLVED ? verdict.detail : "");
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d solution sets found and equal to the expected ones", right, total);
    return check_result(total == expected && right == total, summary);
}

/* Controllo 5: ogni enunciato di set_cases[] riceve l'esito atteso. Ritorna
 * 1 se il controllo è riuscito. */
static int check_set_cases(void) {
    char summary[128];
    int total = (int)(sizeof set_cases / sizeof *set_cases), right = 0;
    for (int i = 0; i < total; i++) {
        Verdict verdict = prove(set_cases[i].statement, 'n', 0, 0);
        if (verdict.outcome == set_cases[i].expected) { right++; continue; }
        printf("  %s: expected %s, got %s\n", set_cases[i].name, outcome_name(set_cases[i].expected), outcome_name(verdict.outcome));
    }
    snprintf(summary, sizeof summary, "%d of %d statements about the set of the variable answered as expected", right, total);
    return check_result(right == total, summary);
}

int main(void) {
    engine_init();

    /* Tutti i controlli vengono eseguiti anche se uno fallisce, così il
     * resoconto è completo. */
    int passed = check_all_proved("tests/equations_inequalities/book_inequalities.tex");
    passed = check_all_proved("tests/equations_inequalities/sign_table.tex") && passed;
    passed = check_none_proved("tests/equations_inequalities/false_inequalities.tex") && passed;
    passed = check_all_proved("tests/equations_inequalities/book_inequalities_with_radicals.tex") && passed;
    passed = check_solutions("tests/equations_inequalities/solution_sets.tex") && passed;
    passed = check_set_cases() && passed;

    /* Ogni riga di una tabella dei segni e ogni soluzione scritta vengono
     * ricontrollate dal programma con un metodo indipendente: nessuna deve
     * essere stata scartata. */
    passed = check_result(unjustified_steps == 0, "every row of the sign tables and every written solution passed its independent check") && passed;
    return passed ? 0 : 1;
}
