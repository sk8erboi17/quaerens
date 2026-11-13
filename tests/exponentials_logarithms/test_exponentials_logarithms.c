/* Test di dominio: esponenziali e logaritmi. Equazioni, disequazioni e campi
 * di esistenza in una variabile reale.
 *
 * Sei controlli. I primi due sono quelli di tests/statement_checks.h:
 *
 *   1. ogni enunciato di book_exponentials_logarithms.tex viene dimostrato:
 *      dice che una condizione vale esattamente per i valori stampati come
 *      risultato su un libro di scuola (corretto nell'esercizio 691, dove
 *      il risultato stampato è sbagliato);
 *   2. nessun enunciato di false_exponentials_logarithms.tex viene
 *      dimostrato: sono gli stessi risultati con un dettaglio cambiato, e
 *      il risultato stampato sbagliato;
 *   3. nessun enunciato di book_exponentials_logarithms_beyond.tex viene
 *      dichiarato falso: sono veri, ma il motore non li sa ancora decidere;
 *   4. ogni richiesta di solution_sets_exponentials_logarithms.tex viene
 *      risolta, e le soluzioni sono quelle del libro: scritte allo stesso
 *      modo, oppure in un altro modo che il motore dimostra equivalente;
 *   5. gli enunciati di rule_cases[], uno per ogni regola o errore passato,
 *      ricevono l'esito atteso;
 *   6. almeno 191 dei 207 enunciati falsi vengono dichiarati falsi, non solo
 *      "non dimostrati".
 *
 * Il metodo sotto esame è la tabella dei segni con le lettere degli
 * esponenziali e dei logaritmi (regole 18p-8, 18p-9 e 18p-10 di main.c). I
 * dati vengono dal libro e da un oracolo indipendente
 * (tools/fuzz/fuzz_exponentials_logarithms.py), non dall'uscita del
 * programma: il motore non ne conosce nessuno in anticipo.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_exponentials_logarithms \
 *         tests/exponentials_logarithms/test_exponentials_logarithms.c \
 *         && /tmp/test_exponentials_logarithms
 */

#include "../statement_checks.h"

/* Il risultato atteso di una richiesta di
 * solution_sets_exponentials_logarithms.tex. Invariante: le voci sono
 * nell'ordine delle richieste del file. */
typedef struct {
    int exercise;      /* il numero dell'esercizio sul libro, per i messaggi */
    const char *text;  /* le soluzioni come le stampa il libro: una condizione
                          su x, oppure una delle due frasi "nessun x" e "ogni x" */
    int open;          /* 1 se il motore oggi non risolve la richiesta: può
                          dirlo, ma se la risolve vale la regola di tutte le altre */
} ExpectedSolution;

/* I risultati vengono dal libro. Nell'esercizio 691 il risultato stampato è
 * sbagliato (ammette x negativi, dove la radice non ha un valore, e x = 1,
 * dove il denominatore è zero), come ha trovato l'oracolo indipendente: qui
 * c'è quello corretto. */
static const ExpectedSolution expected_solutions[] = {
    {202, "x = 1", 0},
    {203, "x = 0 \\lor x = 2", 0},
    {204, "x = -\\frac{3}{2} \\lor x = \\frac{1}{2}", 0},
    {205, "x = -1 \\lor x = 0", 0},
    {206, "x = 2", 0},
    {207, "x = 1", 0},
    {208, "\\nexists x \\in \\mathbb{R}", 0},
    {232, "x = -2", 0},
    {233, "x = 0 \\lor x = 1", 0},
    {234, "x = -\\frac{5}{4}", 0},
    {235, "x = -\\frac{1}{5}", 0},
    {236, "x = 1 \\lor x = 5", 0},
    {237, "x = 2", 1},
    {238, "x = 2", 0},
    {239, "x = 1", 0},
    {240, "x = 4", 0},
    {241, "x = 0 \\lor x = \\frac{2}{3}", 0},
    {242, "x = -\\frac{7}{4}", 1},
    {243, "x = 0", 0},
    {244, "x = 1", 0},
    {245, "x = 0", 0},
    {246, "x = -1 \\lor x = -\\frac{1}{2}", 0},
    {247, "x = \\frac{2}{7}", 0},
    {248, "x = \\frac{5}{4}", 0},
    {249, "x = \\frac{1}{2}", 0},
    {250, "x = -1 \\lor x = 1", 0},
    {251, "\\nexists x \\in \\mathbb{R}", 0},
    {311, "x < 0", 0},
    {312, "x < -\\frac{1}{2} \\lor x > 0", 0},
    {313, "x < -\\frac{1}{3}", 0},
    {314, "x < -\\frac{3}{2}", 0},
    {315, "x > 2", 1},
    {316, "x < 2", 0},
    {317, "x > 2", 0},
    {318, "x > -1", 0},
    {319, "x < -1", 0},
    {320, "x > 1", 0},
    {399, "x = 5", 0},
    {400, "x = 16", 0},
    {401, "x = 10", 0},
    {402, "x = \\frac{1}{10} \\lor x = 1", 0},
    {403, "\\nexists x \\in \\mathbb{R}", 0},
    {404, "\\nexists x \\in \\mathbb{R}", 0},
    {405, "x = \\frac{\\sqrt{2}}{2} \\lor x = 8", 0},
    {406, "x = 1", 0},
    {407, "x = \\frac{1}{2\\sqrt{2}} \\lor x = 4", 0},
    {408, "x = 4", 0},
    {667, "0 < x \\le \\frac{\\sqrt{2}}{2} \\lor x \\ge 2", 0},
    {668, "\\ln 6 \\le x < e \\lor x > e", 0},
    {669, "\\frac{\\ln 2}{2\\ln 6} \\le x \\le 2", 0},
    {670, "-2 \\le x < -1 \\lor -1 < x < 0", 1},
    {671, "0 < x < 1", 0},
    {672, "\\frac{1}{2} \\le x < 1 \\lor x > 1", 0},
    {673, "0 < x \\le 1 \\lor 2 < x < 3", 0},
    {674, "0 < x < 4 \\lor x \\ge 100", 1},
    {675, "\\frac{7}{12} < x < 1 \\lor x > 1", 0},
    {676, "-3 < x < 1 \\lor x > 1", 1},
    {677, "x < -4 \\lor -4 < x < 0 \\lor 0 < x < 4 \\lor x > 4", 0},
    {681, "x \\le 0 \\lor x \\ge \\ln 3", 0},
    {682, "-5 < x < -4", 0},
    {683, "x < \\frac{\\log_{10} 3}{\\log_{10} 5} \\lor x > \\frac{\\log_{10} 3}{\\log_{10} 5}", 0},
    {684, "x < 3 \\lor 3 < x < \\frac{9}{2} \\lor \\frac{9}{2} < x < 5", 0},
    {685, "x < -\\frac{\\log_{10} 3}{\\log_{10} 2}", 0},
    {686, "-\\frac{1}{4} < x \\le 0", 0},
    {687, "0 < x \\le \\frac{1}{16}", 0},
    {688, "x > e^{2} - 1", 0},
    {689, "-1 \\le x < 0 \\lor 0 < x \\le 1", 0},
    {690, "x > 3", 0},
    {691, "0 < x < 1 \\lor x > 1", 0},
    {692, "0 < x \\le \\log_{3} 2 \\lor x > 1", 0},
};

/* Un enunciato con l'esito atteso: una regola della tabella, o un errore
 * passato che non deve tornare. */
typedef struct {
    const char *name;       /* che cosa controlla, per i messaggi */
    const char *statement;  /* l'enunciato, in LaTeX */
    Outcome expected;       /* l'esito che il motore deve dare */
    const char *detail;     /* per SOLVED le soluzioni attese; altrimenti NULL */
} RuleCase;

/* I risultati attesi sono ricavati a mano. L'ordine conta per la seconda
 * coppia: l'errore che fissa compariva solo sul secondo enunciato. */
static const RuleCase rule_cases[] = {
    /* Il logaritmo di una potenza. \log x^2 ha un valore anche per x
     * negativo; accanto a \log x, che lì non ne ha, vale 2 \log x.
     * Antiregressione: una versione del motore dava per sempre positivo
     * (\log x^2)^2 + 4\log x, perché trattava i due logaritmi come lettere
     * positive, e così "dimostrava" che l'equazione non ha soluzioni. */
    {"the logarithm of a square has a value for negative numbers too", "\\{x \\in \\mathbb{R} \\mid \\log_{10} x^{2} > 2\\}", SOLVED, "x < -10 \\lor x > 10"},
    {"beside the logarithm of the base it is twice that logarithm", "\\{x \\in \\mathbb{R} \\mid \\left(\\log_{10} x^{2}\\right)^{2} + 4\\log_{10} x = 0\\}", SOLVED,
     "x = \\frac{1}{10} \\lor x = 1"},
    {"that equation has solutions", "\\forall x \\in \\mathbb{R}\\ \\lnot \\left( \\left(\\log_{10} x^{2}\\right)^{2} + 4\\log_{10} x = 0 \\right)", REFUTED, NULL},
    {"the two logarithms in different comparisons stay apart", "\\{x \\in \\mathbb{R} \\mid \\log_{10} x^{2} > 2 \\land \\log_{10} x < 5\\}", SOLVED, "10 < x < 100000"},
    {"the logarithm of a cube beside the logarithm of the base", "\\{x \\in \\mathbb{R} \\mid \\ln x^{3} - \\ln x = 2\\}", SOLVED, "x = e"},

    /* Antiregressione: tabelle non azzerate tra un enunciato e il
     * successivo facevano dimostrare il secondo, che è falso, dopo che il
     * primo era stato letto. */
    {"an inequality with one exponential", "\\{x \\in \\mathbb{R} \\mid \\frac{5^{x + 1} - 1}{5^{x + 1} - 5} < 0\\}", SOLVED, "-1 < x < 0"},
    {"the same inequality has solutions, whatever came before", "\\forall x \\in \\mathbb{R}\\ \\lnot \\left( \\frac{5^{x + 1} - 1}{5^{x + 1} - 5} < 0 \\right)", REFUTED, NULL},

    /* Antiregressione: \log_{5} 2\sqrt{15}, senza parentesi, si rilegge
     * come (\log_{5} 2)\sqrt{15}. */
    {"the argument of a written logarithm is in parentheses", "\\{x \\in \\mathbb{R} \\mid 5^{2x} \\le 60\\}", SOLVED, "x \\le \\log_{5} \\left(2\\sqrt{15}\\right)"},

    /* La radice di un prodotto di esponenziali si estrae. */
    {"the square root of an exponential", "\\{x \\in \\mathbb{R} \\mid \\sqrt{2^{x}} > 4\\}", SOLVED, "x > 4"},
    {"the cube root of a large power", "\\{x \\in \\mathbb{R} \\mid \\sqrt[3]{11^{9x}} = 1331\\}", SOLVED, "x = 1"},
    {"a root that is not a product of exponentials stays a root", "\\{x \\in \\mathbb{R} \\mid \\sqrt{4^{x} + 9} = 5\\}", SOLVED, "x = 2"},

    /* Più esponenziali che sono potenze di uno stesso rapporto: l'equazione
     * "omogenea" dei libri. Con m = (2/3)^x, 2m^2 - 5m + 3 = (2m - 3)(m - 1)
     * si annulla per m = 3/2 e m = 1, cioè per x = -1 e x = 0, ed è negativo
     * tra i due; m^2 - 2m + 1 = (m - 1)^2; m^2 + m - 2 = (m + 2)(m - 1); le
     * radici di m^2 - 3m + 1 non sono razionali, e il motore deve dirlo. */
    {"a polynomial in the ratio of two exponentials", "\\{x \\in \\mathbb{R} \\mid 2 \\cdot 4^{x} - 5 \\cdot 6^{x} + 3 \\cdot 9^{x} = 0\\}", SOLVED, "x = -1 \\lor x = 0"},
    {"the same as an inequality", "\\{x \\in \\mathbb{R} \\mid 2 \\cdot 4^{x} - 5 \\cdot 6^{x} + 3 \\cdot 9^{x} < 0\\}", SOLVED, "-1 < x < 0"},
    {"a double root", "\\{x \\in \\mathbb{R} \\mid 4^{x} - 2 \\cdot 6^{x} + 9^{x} \\le 0\\}", SOLVED, "x = 0"},
    {"a factor that is always positive", "\\{x \\in \\mathbb{R} \\mid 4^{x} + 6^{x} - 2 \\cdot 9^{x} \\ge 0\\}", SOLVED, "x \\le 0"},
    {"no rational root: the logarithm of an irrational number is not written", "\\{x \\in \\mathbb{R} \\mid 4^{x} - 3 \\cdot 6^{x} + 9^{x} = 0\\}", UNSUPPORTED, NULL},

    /* Una lettera a fattor comune tra due logaritmi diversi. */
    {"a common factor between two logarithms", "\\{x \\in \\mathbb{R} \\mid \\ln(3 - x) \\cdot \\left(\\ln(3 - x) - \\ln(2x)\\right) \\ge 0\\}", SOLVED,
     "0 < x \\le 1 \\lor 2 \\le x < 3"},
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

/* Controlli 3 e 6: conta gli enunciati di path per esito. Con forbidden si
 * dice l'esito che nessuno deve ricevere, con wanted e at_least quanti
 * almeno devono ricevere wanted (at_least = 0: nessun minimo). Ritorna 1 se
 * il controllo è riuscito. */
static int check_outcomes(const char *path, Outcome forbidden, Outcome wanted, int at_least) {
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }
    const char *position = text, *item, *end;
    int total = 0, bad = 0, good = 0;
    while (next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        total++;
        if (verdict.outcome == wanted) good++;
        if (verdict.outcome != forbidden) continue;
        bad++;
        printf("  statement %d of %s is %s\n", total, path, outcome_name(forbidden));
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d statements of %s are %s (at least %d wanted), %d are %s (none allowed)",
             good, total, path, outcome_name(wanted), at_least, bad, outcome_name(forbidden));
    return check_result(bad == 0 && good >= at_least, summary);
}

/* Ritorna 1 se le soluzioni written, scritte dal motore, dicono ciò che dice
 * expected: lo stesso testo, oppure una condizione che il motore dimostra
 * equivalente a quella del libro. Le due frasi "nessun x" e "ogni x" non
 * sono condizioni: lì conta solo il testo. */
static int same_solutions(const char *written, const char *expected) {
    static char statement[8192];
    if (strcmp(written, expected) == 0) return 1;
    if (written[0] == '\\' && (strncmp(written, "\\nexists", 8) == 0 || strncmp(written, "\\forall", 7) == 0)) return 0;
    if (strncmp(expected, "\\nexists", 8) == 0 || strncmp(expected, "\\forall", 7) == 0) return 0;
    snprintf(statement, sizeof statement, "\\forall x \\in \\mathbb{R}\\ \\left( \\left( %s \\right) \\iff \\left( %s \\right) \\right)", written, expected);
    return prove(statement, 'n', 0, 0).outcome == PROVED;
}

/* Controllo 4: ogni richiesta di path è risolta, con le soluzioni di
 * expected_solutions[] (same_solutions()); una richiesta segnata open può
 * anche restare senza risposta. Ritorna 1 se il controllo è riuscito. */
static int check_solutions(const char *path) {
    static char written[4096];
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }
    const char *position = text, *item, *end;
    int total = 0, right = 0, unanswered = 0, expected = (int)(sizeof expected_solutions / sizeof *expected_solutions);
    while (next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        const ExpectedSolution *wanted = total < expected ? &expected_solutions[total] : NULL;
        total++;
        if (wanted && wanted->open && verdict.outcome == UNSUPPORTED) { unanswered++; continue; }

        /* Il testo va copiato: dimostrare l'equivalenza riusa la memoria in
         * cui il motore lo ha scritto. */
        if (wanted && verdict.outcome == SOLVED && verdict.proof) {
            snprintf(written, sizeof written, "%s", verdict.detail);
            if (same_solutions(written, wanted->text)) { right++; continue; }
        }
        printf("  request %d (exercise %d): expected %s, got %s %s\n", total, wanted ? wanted->exercise : 0, wanted ? wanted->text : "nothing",
               outcome_name(verdict.outcome), verdict.outcome == SOLVED ? written : "");
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d solution sets found and equal to the expected ones (%d not answered, as allowed)", right, total, unanswered);
    return check_result(total == expected && right + unanswered == total, summary);
}

/* Controllo 5: ogni enunciato di rule_cases[] riceve, nell'ordine, l'esito
 * atteso e, se è una richiesta, le soluzioni attese. Ritorna 1 se il
 * controllo è riuscito. */
static int check_rule_cases(void) {
    char summary[128];
    int total = (int)(sizeof rule_cases / sizeof *rule_cases), right = 0;
    for (int i = 0; i < total; i++) {
        Verdict verdict = prove(rule_cases[i].statement, 'n', 0, 0);
        int same = verdict.outcome == rule_cases[i].expected && (!rule_cases[i].detail || strcmp(verdict.detail, rule_cases[i].detail) == 0);
        if (same) { right++; continue; }
        printf("  %s: expected %s %s, got %s %s\n", rule_cases[i].name, outcome_name(rule_cases[i].expected), rule_cases[i].detail ? rule_cases[i].detail : "",
               outcome_name(verdict.outcome), verdict.outcome == SOLVED ? verdict.detail : "");
    }
    snprintf(summary, sizeof summary, "%d of %d statements on single rules and past errors answered as expected", right, total);
    return check_result(right == total, summary);
}

int main(void) {
    engine_init();

    /* Tutti i controlli vengono eseguiti anche se uno fallisce, così il
     * resoconto è completo. */
    int passed = check_all_proved("tests/exponentials_logarithms/book_exponentials_logarithms.tex");
    passed = check_none_proved("tests/exponentials_logarithms/false_exponentials_logarithms.tex") && passed;
    passed = check_outcomes("tests/exponentials_logarithms/book_exponentials_logarithms_beyond.tex", REFUTED, PROVED, 0) && passed;
    passed = check_solutions("tests/exponentials_logarithms/solution_sets_exponentials_logarithms.tex") && passed;
    passed = check_rule_cases() && passed;
    passed = check_outcomes("tests/exponentials_logarithms/false_exponentials_logarithms.tex", PROVED, REFUTED, 191) && passed;

    /* Ogni riga di una tabella dei segni e ogni soluzione scritta vengono
     * ricontrollate dal programma con un metodo indipendente: nessuna deve
     * essere stata scartata. */
    passed = check_result(unjustified_steps == 0, "every row of the sign tables and every written solution passed its independent check") && passed;
    return passed ? 0 : 1;
}
