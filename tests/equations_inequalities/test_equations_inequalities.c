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
 * Con un parametro reale accanto alla variabile (regola 18t) i controlli sono
 * altri sei:
 *
 *   6. ogni enunciato di book_inequalities_with_parameter.tex (gli esercizi
 *      del libro con il parametro, con il risultato stampato) e di
 *      parameter_statements.tex (scritti a mano) viene dimostrato;
 *   7. ogni enunciato di false_inequalities_with_parameter.tex viene
 *      dichiarato falso: non basta che non sia dimostrato;
 *   8. nessun enunciato di false_inequalities_with_parameter_beyond_reach.tex,
 *      falsi che la regola non tratta, viene dimostrato;
 *   9. ogni richiesta di solution_sets_with_parameter.tex viene risolta, e le
 *      soluzioni scritte dicono ciò che dicono quelle attese, scritte qui
 *      sotto: il testo può essere diverso, quindi le due si confrontano come
 *      condizioni, calcolandole in tutti i punti di una griglia di numeri;
 *  10. gli enunciati in cui conta la forma della richiesta, o l'insieme di una
 *      delle due lettere, ricevono la risposta giusta;
 *  11. gli enunciati di parameter_statements_beyond_grid.tex, i cui valori
 *      critici del parametro sono lontani o irrazionali isolati, o vengono da
 *      un polinomio di terzo o quarto grado in x, vengono dimostrati; quelli
 *      di false_inequalities_with_parameter_beyond_grid.tex, le stesse
 *      sbagliate di poco, vengono dichiarati falsi. Sono i casi che la griglia
 *      di 18t-g non vede e che il ricontrollo della proiezione (18t-h) deve
 *      reggere.
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
    {"a logarithm has a value only on the positive numbers", "\\{x \\in \\mathbb{R} \\mid \\ln x < 2\\}", SOLVED},
    /* La variabile sia all'esponente sia fuori: non c'è un metodo, e va
     * detto. Antiregressione: una regola sui "due termini positivi" ha
     * risolto la prima con "ogni x", che è falso (x = -1), e avrebbe
     * dimostrato la seconda. */
    {"the variable both in an exponent and outside it is beyond the sign table", "\\{x \\in \\mathbb{R} \\mid x + 2^{x} > 0\\}", UNSUPPORTED},
    {"x + 2^x is not always positive", "\\forall x \\in \\mathbb{R}\\ (x + 2^{x} > 0)", UNSUPPORTED},
    {"nor is x 2^x + 1", "\\forall x \\in \\mathbb{R}\\ (x \\cdot 2^{x} + 1 > 0)", UNSUPPORTED},
    {"a sum of two exponentials is positive", "\\forall x \\in \\mathbb{R}\\ (2^{x} + 3^{x} > 0)", PROVED},
    {"a difference of two exponentials changes sign once", "\\forall x \\in \\mathbb{R}\\ (2^{x} - 3^{x} > 0 \\iff x < 0)", PROVED},
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

/* Le soluzioni attese di una richiesta con un parametro. */
typedef struct {
    int parameter;          /* la lettera del parametro */
    int variable;           /* la lettera della variabile; 0 se la richiesta è
                               dei valori del parametro, \{k \in \mathbb{R} \mid ...\} */
    const char *solutions;  /* le soluzioni, come condizione in LaTeX; oppure
                               una delle due frasi "nessun valore" e "ogni
                               valore", che non sono condizioni */
} ParameterSolution;

/* Le soluzioni attese delle richieste di solution_sets_with_parameter.tex,
 * nell'ordine del file. Le prime 19 sono i risultati stampati sul libro
 * degli esercizi da 215 a 233; le altre sono state ricavate a mano. Nessuna è
 * copiata dall'uscita del programma, che può scriverle in un altro modo: la
 * 10 ha sul libro un valore assoluto dove il programma scrive due casi, e la
 * 13 un x \neq -2 dove il programma scrive x < -2 \lor x > -2. Per questo il
 * confronto non è tra testi (vedi same_on_grid()). */
static const ParameterSolution parameter_solutions[] = {
    {'a', 'x', "(a < 0 \\land 4a \\le x \\le 0) \\lor (a = 0 \\land x = 0) \\lor (a > 0 \\land 0 \\le x \\le 4a)"},
    {'a', 'x', "(a < 0 \\land (x \\le \\frac{2}{a} \\lor x \\ge -\\frac{5}{a})) \\lor (a > 0 \\land (x \\le -\\frac{5}{a} \\lor x \\ge \\frac{2}{a}))"},
    {'a', 'x', "(a < 0 \\land 2a \\le x \\le -4a) \\lor (a = 0 \\land x = 0) \\lor (a > 0 \\land -4a \\le x \\le 2a)"},
    {'a', 'x', "(a < 0 \\land \\frac{1}{2a} < x < -\\frac{3}{a}) \\lor a = 0 \\lor (a > 0 \\land -\\frac{3}{a} < x < \\frac{1}{2a})"},
    {'a', 'x', "(a < 0 \\land (x < \\frac{1}{a} \\lor x > -a)) \\lor (a = 0 \\land x > 0) \\lor (a > 0 \\land -a < x < \\frac{1}{a})"},
    {'b', 'x', "(b < -1 \\land 2 < x < \\frac{2(b - 1)}{b + 1}) \\lor (b = -1 \\land x > 2) \\lor (b > -1 \\land (x < \\frac{2(b - 1)}{b + 1} \\lor x > 2))"},
    {'a', 'x', "(a < 0 \\land \\frac{2a}{3} \\le x \\le -a) \\lor (a = 0 \\land x = 0) \\lor (a > 0 \\land -a \\le x \\le \\frac{2a}{3})"},
    {'a', 'x', "(a < 5 \\land a - 1 < x < 4) \\lor (a > 5 \\land 4 < x < a - 1)"},
    {'a', 'x', "(a < 2 \\land (x \\le \\frac{a - 1}{2} \\lor x \\ge \\frac{1}{2})) \\lor a = 2 \\lor (a > 2 \\land (x \\le \\frac{1}{2} \\lor x \\ge \\frac{a - 1}{2}))"},
    {'a', 'x', "(a \\neq \\frac{1}{2} \\land (x \\le -\\left|2a - 1\\right| \\lor x \\ge \\left|2a - 1\\right|)) \\lor a = \\frac{1}{2}"},
    {'a', 'x', "(a < 0 \\land a \\le x \\le -\\frac{3}{a}) \\lor (a = 0 \\land x \\ge 0) \\lor (a > 0 \\land (x \\le -\\frac{3}{a} \\lor x \\ge a))"},
    {'k', 'x', "-2 < k < 2 \\land 2 - \\sqrt{4 - k^{2}} < x < 2 + \\sqrt{4 - k^{2}}"},
    {'k', 'x', "(k < -3 \\land (x < k + 1 \\lor x > -k - 5)) \\lor (k = -3 \\land x \\neq -2) \\lor (k > -3 \\land (x < -k - 5 \\lor x > k + 1))"},
    {'k', 'x', "(k < 2 \\land (x \\le \\frac{k}{2} \\lor x \\ge 1)) \\lor k = 2 \\lor (k > 2 \\land (x \\le 1 \\lor x \\ge \\frac{k}{2}))"},
    {'a', 'x', "a \\neq 0 \\lor (a = 0 \\land x \\neq 0)"},
    {'a', 'x', "(a < 1 \\land a < x < 1) \\lor (a > 1 \\land 1 < x < a)"},
    {'a', 'x', "(a < -2 \\land a \\le x \\le -2) \\lor (a = -2 \\land x = -2) \\lor (a > -2 \\land -2 \\le x \\le a)"},
    {'k', 0, "-\\sqrt{7} < k < \\sqrt{7}"},
    {'b', 0, "-\\frac{4}{9} < b < 0"},
    {'a', 'x', "a > 0 \\land -\\sqrt{a} < x < \\sqrt{a}"},
    {'m', 'x', "(m < 0 \\land x < \\frac{1}{m}) \\lor (m > 0 \\land x > \\frac{1}{m})"},
    {'a', 'x', "a < x < a + 1"},
    {'a', 'x', "\\nexists x \\in \\mathbb{R}"},
    {'a', 'x', "\\forall x \\in \\mathbb{R}"},
    {'k', 'x', "(k \\le -1 \\land k - \\sqrt{k^{2} - 1} \\le x \\le k + \\sqrt{k^{2} - 1}) \\lor (k \\ge 1 \\land k - \\sqrt{k^{2} - 1} \\le x \\le k + \\sqrt{k^{2} - 1})"},
    {'a', 'x', "a > -1 \\land a < x < 2a + 1"},
    {'a', 'x', "(a < 1 \\land (x < a \\lor 1 < x < 3)) \\lor (a = 1 \\land x \\neq 1 \\land x < 3) \\lor (1 < a < 3 \\land (x < 1 \\lor a < x < 3)) \\lor (a \\ge 3 \\land x < 1)"},
    {'a', 'x', "(a < 0 \\land (x \\le a \\lor x > -a)) \\lor (a = 0 \\land x \\neq 0) \\lor (a > 0 \\land (x < -a \\lor x \\ge a))"},
    {'a', 0, "a > -2"},
    {'k', 0, "k \\le -2 \\lor k \\ge 2"},
    {'a', 0, "a < -1"},
    {'m', 0, "m < 1"},
    {'a', 0, "\\nexists a \\in \\mathbb{R}"},
    {'a', 0, "\\forall a \\in \\mathbb{R}"},
    {'k', 0, "k > 4"},
};

/* Gli enunciati con un parametro in cui conta la forma della richiesta o
 * l'insieme di una delle due lettere, con l'esito atteso. Una richiesta con
 * un parametro è \forall a \in \mathbb{R}\ \{x \in \mathbb{R} \mid ...\}, e
 * nient'altro: ogni altra scrittura deve ricevere un "non trattato", non una
 * risposta calcolata come se fosse quella. */
static const SetCase parameter_cases[] = {
    {"a request with a real parameter in front is solved", "\\forall a \\in \\mathbb{R}\\ \\{x \\in \\mathbb{R} \\mid x^{2} - 4ax \\le 0\\}", SOLVED},
    {"the parameter of a request must be real, not rational", "\\forall a \\in \\mathbb{Q}\\ \\{x \\in \\mathbb{R} \\mid x > a\\}", UNSUPPORTED},
    {"nor a natural number", "\\forall n \\in \\mathbb{N}\\ \\{x \\in \\mathbb{R} \\mid x > n\\}", UNSUPPORTED},
    {"with a parameter the solutions are searched in the reals", "\\forall a \\in \\mathbb{R}\\ \\{x \\in \\mathbb{Q} \\mid x > a\\}", UNSUPPORTED},
    {"only one quantifier may stand before a request", "\\forall a \\in \\mathbb{R}\\ \\forall b \\in \\mathbb{R}\\ \\{x \\in \\mathbb{R} \\mid x > a + b\\}", UNSUPPORTED},
    {"and it must be a universal one", "\\exists a \\in \\mathbb{R}\\ \\{x \\in \\mathbb{R} \\mid x > a\\}", UNSUPPORTED},
    {"a request with a parameter cannot be negated", "\\lnot \\forall a \\in \\mathbb{R}\\ \\{x \\in \\mathbb{R} \\mid x > a\\}", UNSUPPORTED},
    {"the values of a parameter are searched only in the reals", "\\{k \\in \\mathbb{Q} \\mid \\forall x \\in \\mathbb{R}\\ (x^{2} + k > 0)\\}", UNSUPPORTED},

    /* Tre lettere reali, o una variabile che non varia su tutti i reali:
     * enunciati veri, che la regola non tratta e non deve fingere di
     * trattare. */
    {"three real letters are beyond the rule", "\\forall a \\in \\mathbb{R}\\ \\forall b \\in \\mathbb{R}\\ \\forall x \\in \\mathbb{R}\\ (x^{2} + a^{2} + b^{2} \\ge 0)", UNSUPPORTED},
    {"so is a variable on the positive reals under a parameter", "\\forall a \\in \\mathbb{R}\\ \\forall x \\in \\mathbb{R}^{+}\\ (x + a^{2} > 0)", UNSUPPORTED},

    /* Ciò che è vero per ogni x reale può non esserlo dove un'espressione
     * non ha valore per un valore del parametro. */
    {"every real number is a real number", "\\forall a \\in \\mathbb{R}\\ \\exists x \\in \\mathbb{R}\\ (x = a)", PROVED},
    {"dividing by the parameter fails where it is zero", "\\forall a \\in \\mathbb{R}\\ \\forall x \\in \\mathbb{R}\\ \\left(\\frac{x}{a} \\cdot a = x\\right)", REFUTED},
    {"and holds elsewhere", "\\forall a \\in \\mathbb{R}\\ \\forall x \\in \\mathbb{R}\\ \\left(a \\neq 0 \\implies \\frac{x}{a} \\cdot a = x\\right)", PROVED},
    {"a condition on the parameter alone is part of the request", "\\forall a \\in \\mathbb{R}\\ \\{x \\in \\mathbb{R} \\mid x > a \\land a > 0\\}", SOLVED},
    {"the values of a parameter for a negated statement", "\\{b \\in \\mathbb{R} \\mid \\lnot \\exists x \\in \\mathbb{R}\\ (x^{2} + 3bx - b \\le 0)\\}", SOLVED},
};

/* Controllo 7: ogni enunciato di path, un file di formule sbagliate, deve
 * essere dichiarato falso. È più di check_none_proved(): un "non so" qui non
 * basta, perché questi enunciati la regola li tratta. Ritorna 1 se il
 * controllo è riuscito. */
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
        Verdict verdict = prove_item(item, end);
        total++;
        if (verdict.outcome == REFUTED) { refuted++; continue; }
        printf("  statement %d is false but was not refuted: ", total);
        verdict_print(stdout, &verdict);
        putchar('\n');
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d false statements refuted in %s", refuted, total, path);
    return check_result(total > 0 && refuted == total, summary);
}

/* Legge text come condizione, con il lettore del programma, e la ritorna;
 * NULL se non è una condizione. I nodi stanno nell'arena del programma:
 * valgono fino all'enunciato successivo. */
static Formula *read_condition(const char *text) {
    syntax_error = NULL;
    cursor = text;
    next_token();
    Formula *condition = logic_formula();
    if (!syntax_error && token.kind == T_END) return condition;
    syntax_error = NULL;
    return NULL;
}

/* Quanti quarti, da una parte e dall'altra dello zero, prende la griglia di
 * same_on_grid() per il parametro e per la variabile: il parametro da -6 a 6,
 * la variabile da -12 a 12. */
enum { GRID_PARAMETER = 24, GRID_VARIABLE = 48 };

/* Ritorna 1 se le due condizioni written ed expected, nel parametro
 * `parameter` e nella variabile `variable` (0 se c'è solo il parametro),
 * hanno la stessa verità in ogni punto di una griglia di numeri razionali:
 * tutti i quarti, per il parametro e per la variabile. Mette in *compared il
 * numero dei punti in cui il confronto si è potuto fare; se in un punto le
 * due sono diverse ritorna 0, con il punto scritto in where, di size byte.
 *
 * È un confronto tra condizioni, non tra testi, e non usa nulla della
 * regola sotto esame: in ogni punto le due lettere sono numeri, e ogni
 * confronto si calcola con i numeri (sign_holds_at()). I quarti toccano gli
 * estremi scritti negli esercizi (4a, \frac{2}{a}, \frac{a - 1}{2}, ...) per
 * molti valori del parametro, ed è lì che un < al posto di un \le si vede.
 * Dove il conto di un punto esce dai limiti del programma il punto si
 * salta. Non è una dimostrazione dell'equivalenza: quella, per i risultati
 * del libro, è il controllo 6. */
static int same_on_grid(const Formula *written, const Formula *expected, int parameter, int variable, int *compared, char *where, size_t size) {
    *compared = 0;
    for (int i = -GRID_PARAMETER; i <= GRID_PARAMETER; i++) {
        Scratch mark = scratch_mark();
        Frac a = frac_make(i, 4);
        const Formula *left = written, *right = expected;

        /* Con due lettere il parametro si sostituisce, e resta una
         * condizione nella variabile. */
        if (variable) {
            Node *value = node_rational((Wide){a.num, a.den});
            left = formula_with(written, parameter, value);
            right = formula_with(expected, parameter, value);
        }
        for (int j = variable ? -GRID_VARIABLE : 0; j <= (variable ? GRID_VARIABLE : 0); j++) {
            Frac x = variable ? frac_make(j, 4) : a;
            failure = NULL;
            int first = sign_holds_at(left, variable ? variable : parameter, x);
            int second = failure ? 0 : sign_holds_at(right, variable ? variable : parameter, x);
            if (failure) { failure = NULL; continue; }
            (*compared)++;
            if (first == second) continue;
            if (variable) snprintf(where, size, "%c = %lld/%lld, %c = %lld/%lld", parameter, a.num, a.den, variable, x.num, x.den);
            else snprintf(where, size, "%c = %lld/%lld", parameter, a.num, a.den);
            scratch_drop(mark);
            return 0;
        }
        scratch_drop(mark);
    }
    return 1;
}

/* Controllo 9: ogni richiesta di path è risolta, con la dimostrazione
 * scritta, e le soluzioni dicono ciò che dicono parameter_solutions[],
 * nell'ordine. Una soluzione attesa che non è una condizione ("nessun x",
 * "ogni x") si confronta come testo; le altre con same_on_grid(), che deve
 * aver potuto confrontare almeno metà dei punti. Ritorna 1 se il controllo è
 * riuscito. */
static int check_parameter_solutions(const char *path) {
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }
    const char *position = text, *item, *end;
    int total = 0, right = 0, expected = (int)(sizeof parameter_solutions / sizeof *parameter_solutions);
    while (next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        const ParameterSolution *wanted = total < expected ? &parameter_solutions[total] : NULL;
        char where[128] = "";
        int compared = 0, same = 0;
        total++;
        if (!wanted || verdict.outcome != SOLVED || !verdict.proof) {
            printf("  request %d: expected %s, got %s\n", total, wanted ? wanted->solutions : "nothing", outcome_name(verdict.outcome));
            continue;
        }

        /* La soluzione attesa si legge per seconda: il lettore non tocca il
         * testo del verdetto, e i nodi delle due stanno nell'arena fino
         * alla richiesta successiva. */
        const Formula *written = read_condition(verdict.detail), *asked = read_condition(wanted->solutions);
        if (!asked) {
            same = strcmp(verdict.detail, wanted->solutions) == 0;
        } else if (written) {
            int points = (2 * GRID_PARAMETER + 1) * (wanted->variable ? 2 * GRID_VARIABLE + 1 : 1);
            same = same_on_grid(written, asked, wanted->parameter, wanted->variable, &compared, where, sizeof where) && 2 * compared >= points;
        }
        if (same) { right++; continue; }
        printf("  request %d: expected %s, got %s (%s%s, %d points compared)\n", total, wanted->solutions, verdict.detail,
               where[0] ? "they differ at " : "no difference found", where, compared);
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d solution sets with a parameter found and equal to the expected ones on a grid of numbers", right, total);
    return check_result(total == expected && right == total, summary);
}

/* Controllo 10: ogni enunciato di parameter_cases[] riceve l'esito atteso.
 * Ritorna 1 se il controllo è riuscito. */
static int check_parameter_cases(void) {
    char summary[128];
    int total = (int)(sizeof parameter_cases / sizeof *parameter_cases), right = 0;
    for (int i = 0; i < total; i++) {
        Verdict verdict = prove(parameter_cases[i].statement, 'n', 0, 0);
        if (verdict.outcome == parameter_cases[i].expected) { right++; continue; }
        printf("  %s: expected %s, got %s\n", parameter_cases[i].name, outcome_name(parameter_cases[i].expected), outcome_name(verdict.outcome));
    }
    snprintf(summary, sizeof summary, "%d of %d statements about the shape of a request with a parameter answered as expected", right, total);
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

    /* Con un parametro (regola 18t). */
    passed = check_all_proved("tests/equations_inequalities/book_inequalities_with_parameter.tex") && passed;
    passed = check_all_proved("tests/equations_inequalities/parameter_statements.tex") && passed;
    passed = check_none_proved("tests/equations_inequalities/false_inequalities_with_parameter.tex") && passed;
    passed = check_all_refuted("tests/equations_inequalities/false_inequalities_with_parameter.tex") && passed;
    passed = check_none_proved("tests/equations_inequalities/false_inequalities_with_parameter_beyond_reach.tex") && passed;
    passed = check_parameter_solutions("tests/equations_inequalities/solution_sets_with_parameter.tex") && passed;
    passed = check_parameter_cases() && passed;
    passed = check_all_proved("tests/equations_inequalities/parameter_statements_beyond_grid.tex") && passed;
    passed = check_none_proved("tests/equations_inequalities/false_inequalities_with_parameter_beyond_grid.tex") && passed;
    passed = check_all_refuted("tests/equations_inequalities/false_inequalities_with_parameter_beyond_grid.tex") && passed;

    /* Ogni riga di una tabella dei segni e ogni soluzione scritta vengono
     * ricontrollate dal programma con un metodo indipendente: nessuna deve
     * essere stata scartata. */
    passed = check_result(unjustified_steps == 0, "every row of the sign tables and every written solution passed its independent check") && passed;
    return passed ? 0 : 1;
}
