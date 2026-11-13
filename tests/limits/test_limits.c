/* Test di dominio: limiti (capitolo 29 di main.c).
 *
 * Sette controlli. I primi tre sono quelli di tests/statement_checks.h:
 *
 *   1. ogni enunciato di book_limits.tex viene dimostrato: dice che un limite
 *      ha il valore stampato come risultato in una raccolta di esercizi
 *      (corretto nell'unico esercizio in cui il risultato stampato è
 *      sbagliato);
 *   2. nessun enunciato di false_limits.tex viene dimostrato: sono gli stessi
 *      limiti con un numero cambiato di uno, un segno cambiato, un infinito
 *      con il segno sbagliato, un "non esiste" sbagliato, e il risultato
 *      stampato sbagliato;
 *   3. nessun enunciato di book_limits_beyond.tex viene dichiarato falso:
 *      sono veri, ma il motore non li sa ancora decidere;
 *   4. almeno 247 dei 260 enunciati falsi vengono dichiarati falsi, non solo
 *      "non dimostrati": se una modifica facesse scendere questo numero il
 *      motore saprebbe riconoscere meno errori di oggi;
 *   5. ogni richiesta di limit_requests.tex viene risolta, e la risposta è
 *      quella del libro: scritta allo stesso modo, oppure in un altro modo
 *      che il motore dimostra uguale;
 *   6. ogni enunciato di book_limits.tex, tolto il valore, diventa una
 *      richiesta che il motore risolve, e la risposta riletta come enunciato
 *      è dimostrata;
 *   7. gli enunciati di special_cases[] (la lettura, le convenzioni, gli
 *      errori trovati) ricevono l'esito atteso.
 *
 * Il metodo sotto esame sono le serie troncate del capitolo 29, con il
 * controllo con i numeri (29i) e la risposta riletta (29j). I dati vengono da
 * photos/dati_esercizi/limiti.jsonl, confermati da un oracolo numerico
 * indipendente, e da tools/fuzz/fuzz_limits.py, non dall'uscita del
 * programma: il motore non ne conosce nessuno in anticipo.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_limits \
 *         tests/limits/test_limits.c && /tmp/test_limits
 */

#include "../statement_checks.h"

/* Il risultato atteso di una richiesta di limit_requests.tex.
 * Invariante: le voci sono nell'ordine delle richieste del file. */
typedef struct {
    const char *exercise;  /* il numero dell'esercizio nella raccolta, per i messaggi */
    const char *answer;    /* la risposta attesa, come enunciato: il limite con il
                              suo valore, oppure \nexists davanti */
    int open;              /* 1 se il motore oggi non risolve la richiesta: può
                              dirlo, ma se la risolve vale la regola di tutte le altre */
} ExpectedLimit;

/* Le risposte vengono dalla raccolta di esercizi (photos/dati_esercizi), con
 * \log scritto \ln: lì \log è il logaritmo naturale, qui è in base 10.
 * Dove il libro scrive \infty senza segno per un limite i cui due limiti da
 * un lato sono +\infty e -\infty, la risposta del motore è che il limite non
 * esiste. */
static const ExpectedLimit expected_limits[] = {
    {"siena-A1", "\\lim_{x \\to 1} \\frac{\\arctan x}{\\ln(1 + x^{2})} = \\frac{\\pi}{4\\ln 2}", 0},
    {"siena-A3", "\\lim_{x \\to e} \\ln\\left(\\ln(2x - e)\\right) = 0", 0},
    {"siena-B6", "\\lim_{x \\to 0^{+}} (\\cot x - \\ln x) = +\\infty", 0},
    {"siena-B7", "\\lim_{x \\to 0^{+}} \\left(\\cos x + \\arctan\\frac{1}{x}\\right) = 1 + \\frac{\\pi}{2}", 0},
    {"siena-C3", "\\lim_{x \\to -\\infty} \\frac{x^{4} + 2}{2x^{2} - x - 1} = +\\infty", 0},
    {"sassetti-1.1", "\\lim_{x \\to 0} \\left(\\frac{\\cos x}{\\sin x} - \\frac{\\exp x}{x}\\right) = -1", 0},
    {"sassetti-1.4", "\\lim_{x \\to 2^{+}} \\frac{\\sqrt{x - 2} + \\sqrt{x - 1} - 1}{x^{2} - 4} = +\\infty", 0},
    {"sassetti-1.6", "\\lim_{x \\to \\frac{\\pi}{2}} \\left(\\frac{\\pi}{2} - x\\right)^{\\cos x} = 1", 0},
    {"sassetti-1.8", "\\lim_{x \\to e} \\left(\\frac{1}{\\ln x} - 1\\right)^{x - e} = 1", 0},
    {"sassetti-2.1", "\\lim_{x \\to 0} \\frac{\\ln(1 + x^{2}) - \\sin(x^{2})}{\\cos 2x + \\exp(2x^{2}) - 2} = -\\frac{3}{16}", 0},
    {"sassetti-2.6", "\\lim_{x \\to 0} \\frac{\\sqrt{1 - x^{2}} - \\cos x + x^{4}}{x^{2} - \\sin^{2} x} = \\frac{5}{2}", 0},
    {"units-3a", "\\lim_{x \\to 1} \\frac{x^{2} - 1}{x^{3} - 1} = \\frac{2}{3}", 0},
    {"units-3d", "\\lim_{x \\to +\\infty} \\frac{x^{5/2} - 2x\\sqrt{x} + 1}{2x^{5/2} - 1} = \\frac{1}{2}", 0},
    {"units-4d", "\\lim_{x \\to -\\infty} \\left(\\sqrt{4 + x^{2}} + x\\right) = 0", 0},
    {"units-9f", "\\lim_{x \\to -\\infty} x\\left(3^{1/x} - 1\\right) = \\ln 3", 0},
    {"units-9l", "\\lim_{x \\to 0} \\frac{2^{x} - 3^{x}}{x} = -\\ln\\frac{3}{2}", 0},
    {"units-10l", "\\lim_{x \\to \\pi} \\frac{\\cos x + 1}{\\cos(3x) + 1} = \\frac{1}{9}", 0},
    {"units-11a", "\\lim_{x \\to +\\infty} \\left(1 + \\frac{1}{x}\\right)^{2x} = e^{2}", 0},
    {"units-11b", "\\lim_{x \\to +\\infty} \\left(1 + \\frac{1}{x}\\right)^{x^{2}} = +\\infty", 0},
    {"units-11d", "\\lim_{x \\to 0^{+}} x^{x} = 1", 0},
    {"unitsvolti-1f", "\\lim_{x \\to 2} \\frac{2x^{3} - 5x^{2} - 4x + 12}{x^{4} - 4x^{3} + 5x^{2} - 4x + 4} = \\frac{7}{5}", 0},
    {"unitsvolti-1q", "\\lim_{x \\to -1} \\frac{x + 1}{\\sqrt[4]{x + 17} - 2} = 32", 0},
    {"unitsvolti-1r", "\\lim_{x \\to 0} \\frac{\\arccos x - \\frac{\\pi}{2}}{x} = -1", 0},
    {"zuccher4-2.12", "\\lim_{x \\to +\\infty} \\sqrt{2x^{2} - 1} - \\sqrt{2x^{2} - x - 1} = 1/(2\\sqrt{2})", 0},
    {"zuccher4-2.16", "\\nexists \\lim_{x \\to 0} \\frac{1}{x}", 0},
    {"zuccher4-2.17", "\\nexists \\lim_{x \\to 0} e^{\\frac{1}{x}}", 0},
    {"zuccher4-2.23", "\\lim_{x \\to +\\infty} \\left(\\frac{x - 3}{x + 4}\\right)^{\\frac{x^{2} - 1}{2x}} = 1/\\sqrt{e^{7}}", 0},
    {"zuccher4-2.29", "\\nexists \\lim_{x \\to 0} \\left|x\\right|^{1/x}", 0},
    {"zuccher9-1.5", "\\lim_{x \\to +\\infty} \\frac{x + \\sin x}{x + \\cos x} = 1", 0},
    {"zuccher9-2.7", "\\lim_{x \\to 0} \\frac{e^{x^{2}} - 1 - x^{2} + \\ln(1 + x^{3})}{\\sin x - x} = -6", 0},
    {"zuccher9-2.9", "\\lim_{x \\to 0} \\left(\\frac{\\sin x}{x}\\right)^{1/x^{2}} = e^{-1/6}", 0},
    {"scaglia-2", "\\lim_{x \\to 0} \\frac{e^{-x} + \\ln\\left(\\frac{1 + x}{e}\\right)}{2(\\cosh x - 1)\\sinh x} = \\frac{1}{6}", 0},
    {"scaglia-6", "\\lim_{x \\to 0} \\frac{(6\\cosh\\sqrt{x} - 6\\cos\\sqrt{x})\\tan(x^{3})}{15\\sin^{2} x - 15(\\arcsin x)^{2}} = -\\frac{3}{5}", 0},
    {"scaglia-9", "\\lim_{x \\to +\\infty} \\frac{\\frac{\\pi}{2} - \\arctan x}{x - x^{2}\\ln\\left(1 + \\frac{1}{x}\\right) - \\frac{1}{2}} = -3", 0},
    {"scaglia-10", "\\lim_{n \\to +\\infty} \\frac{e^{2/n^{3}} - 1}{\\sin\\left(\\frac{1}{n}\\right) - \\frac{1}{n}}\\left[\\ln\\left(\\left(1 + \\frac{1}{4n}\\right)^{n}\\right)\\right] = -3", 0},
    {"miranda-6.0.11ix", "\\lim_{x \\to 0} \\frac{1 - \\cos\\sqrt{\\left|x\\right|}}{\\left|x\\right|} = \\frac{1}{2}", 0},
    {"miranda-6.0.13i", "\\lim_{x \\to 0} \\frac{5^{x} - 1}{3^{x} - 1} = \\log_{3} 5", 0},
    {"miranda-6.0.13xi", "\\lim_{x \\to 0} (1 + \\sin x)^{1/x} = e", 0},
    {"trento6-24.2", "\\lim_{n \\to +\\infty} \\left(\\frac{1 + n}{3 + n}\\right)^{n^{2}} = 0", 0},
    {"trento6-24.3", "\\lim_{x \\to +\\infty} \\left(\\frac{1}{3x}\\right)^{\\frac{1}{\\ln(2x)}} = e^{-1}", 0},
    {"trento8-8.1b", "\\lim_{x \\to 0^{+}} \\frac{\\ln(\\ln(x + 1))}{\\ln x} = 1", 0},
    {"trento8-8.3c", "\\lim_{x \\to 0} \\frac{x - \\sin^{2}\\sqrt{x} - \\sin^{2} x}{x^{2}} = -\\frac{2}{3}", 0},
    {"units-3c", "\\nexists \\lim_{x \\to 0} \\frac{3x^{2} - x}{x^{5} + 2x^{2}}", 0},
    {"virdis-E15a", "\\nexists \\lim_{x \\to 0} \\frac{1}{3^{\\frac{1}{x}} + 1}", 0},
    {"units-12e", "\\lim_{x \\to +\\infty} 2^{-x}x^{x} = +\\infty", 1},
    {"miranda-6.0.13iii", "\\nexists \\lim_{x \\to 0} \\frac{e^{1/x} + 3}{e^{1/x} + 1}", 1},
};

/* Un enunciato con l'esito atteso, per il controllo 7. */
typedef struct {
    const char *name;       /* che cosa controlla, per i messaggi */
    const char *statement;  /* l'enunciato, in LaTeX */
    Outcome expected;       /* l'esito che il motore deve dare */
} LimitCase;

/* Gli esiti attesi sono ricavati a mano, uno per uno, con la definizione di
 * limite e le convenzioni scritte in testa al capitolo 29. */
static const LimitCase special_cases[] = {
    /* La lettura (9i). */
    {"\\limits and \\rightarrow are read", "\\lim\\limits_{x \\rightarrow 0} \\frac{\\sin x}{x} = 1", PROVED},
    {"a side written without braces", "\\lim_{x \\to 0^+} \\sqrt{x} = 0", PROVED},
    {"\\infty without a sign is refused as a point of a real variable", "\\lim_{x \\to \\infty} \\frac{1}{x} = 0", UNSUPPORTED},
    {"\\infty without a sign is +\\infty for the index of a sequence", "\\lim_{n \\to \\infty} \\frac{n}{n + 1} = 1", PROVED},
    {"a sequence has a limit only at +\\infty", "\\lim_{n \\to 0} n = 0", UNSUPPORTED},
    {"an infinite point has no side", "\\lim_{x \\to +\\infty^{+}} \\frac{1}{x} = 0", UNSUPPORTED},
    {"a letter other than the variable is a parameter, not treated", "\\lim_{x \\to 0} \\frac{\\sin(a x)}{x} = a", UNSUPPORTED},
    {"the variable cannot be e", "\\lim_{e \\to 0} e = 0", UNSUPPORTED},
    {"the point cannot contain the variable", "\\lim_{x \\to x} x = 0", UNSUPPORTED},
    {"\\exp, \\sinh, \\cosh are read by their definition", "\\lim_{x \\to 0} \\frac{\\exp x - \\cosh x - \\sinh x}{x} = 0", PROVED},

    /* Il campo di esistenza (29h). */
    {"only the right side is in the domain of a square root", "\\lim_{x \\to 0} \\sqrt{x} = 0", PROVED},
    {"only the left side", "\\lim_{x \\to 0} \\sqrt{-x} = 0", PROVED},
    {"no side is in the domain: the limit has no meaning", "\\lim_{x \\to 0} \\sqrt{-x^{2}} = 0", NOT_PROVED},
    {"a power with a non-integer exponent wants a positive base", "\\lim_{x \\to 0} x^{\\frac{1}{3}} = 0", PROVED},
    {"f^g is e^{g ln f}: x^{1/x} has no left side at 0", "\\lim_{x \\to 0} x^{\\frac{1}{x}} = 0", PROVED},

    /* Limiti infiniti e limiti che non esistono. */
    {"two infinite one-sided limits of opposite sign: the limit does not exist", "\\lim_{x \\to 0} \\frac{1}{x} = +\\infty", REFUTED},
    {"but its absolute value tends to +\\infty", "\\lim_{x \\to 0} \\frac{1}{x} = \\infty", PROVED},
    {"an infinite limit of one sign", "\\lim_{x \\to 0} \\frac{1}{x^{2}} = +\\infty", PROVED},
    {"an infinite limit is not unsigned-false", "\\lim_{x \\to 0} \\frac{1}{x^{2}} = \\infty", PROVED},
    {"a limit that exists is not said not to exist", "\\nexists \\lim_{x \\to 0} \\frac{1}{x^{2}}", REFUTED},
    {"one-sided limits 1 and -1", "\\nexists \\lim_{x \\to 0} \\frac{\\sin x}{\\left|x\\right|}", PROVED},
    {"an oscillation is never refuted from its expansion alone", "\\lim_{x \\to 0} \\sin\\frac{1}{x} = 0", NOT_PROVED},
    {"a bounded factor times an infinitesimal", "\\lim_{x \\to 0} x \\sin\\frac{1}{x} = 0", PROVED},
    {"a bounded term beside an infinite one", "\\lim_{x \\to +\\infty} \\sqrt{x} + \\cos x = +\\infty", PROVED},
    {"faster than any power: x e^{-x}", "\\lim_{x \\to +\\infty} x e^{-x} = 0", PROVED},
    {"an infinite form that the growth scale would need", "\\lim_{x \\to +\\infty} \\frac{e^{x}}{x} = +\\infty", NOT_PROVED},

    /* La scala dei logaritmi (29d). */
    {"x ln x", "\\lim_{x \\to 0^{+}} x \\ln x = 0", PROVED},
    {"x ln x is not below 0 by a constant", "\\lim_{x \\to 0^{+}} x \\ln x = -1", REFUTED},
    {"ln x / x at +\\infty", "\\lim_{x \\to +\\infty} \\frac{\\ln x}{x} = 0", PROVED},
    {"x^{1/x} at +\\infty", "\\lim_{x \\to +\\infty} x^{\\frac{1}{x}} = 1", PROVED},

    /* Le costanti: uno zero nascosto si riconosce con il test esatto, e un
     * numero che il test non sa decidere ferma il conto. */
    {"ln 4 - 2 ln 2 is zero", "\\lim_{x \\to 0} \\frac{\\ln 4 - 2\\ln 2}{x} = 0", PROVED},
    {"cos(pi/3) - 1/2 is zero, but the algebra does not see it", "\\lim_{x \\to 0} \\frac{\\cos\\frac{\\pi}{3} - \\frac{1}{2}}{x} = 0", NOT_PROVED},
    {"a constant that is close to the limit but not it", "\\lim_{x \\to 0} \\frac{\\sin x}{x} = \\frac{10001}{10000}", REFUTED},

    /* Antiregressione: senza riscrivere nel denominatore degli esponenti la
     * costante di uno sviluppo, 1/(2 x^{5/2} - 1) a +\infty diventava
     * t^5/2 invece di t^{5/2}/2, e il limite di x^{5/2}/(2 x^{5/2} - 1)
     * risultava 0. Il controllo con i numeri lo aveva fermato. */
    {"the denominator of the exponents survives a constant expansion", "\\lim_{x \\to +\\infty} \\frac{x^{5/2}}{2x^{5/2} - 1} = \\frac{1}{2}", PROVED},
    {"and the wrong value it used to give is false", "\\lim_{x \\to +\\infty} \\frac{x^{5/2}}{2x^{5/2} - 1} = 0", REFUTED},
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

/* Controllo 4: conta gli enunciati di path per esito. Nessuno deve ricevere
 * forbidden; almeno at_least devono ricevere wanted. Ritorna 1 se il
 * controllo è riuscito. */
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

/* Ritorna 1 se la risposta written, scritta dal motore, dice ciò che dice
 * expected: lo stesso testo, oppure un enunciato che il motore dimostra, con
 * l'enunciato atteso dimostrato anche lui. Il limite è uno solo: se tutti e
 * due sono dimostrati, il valore scritto è quello atteso. */
static int same_limit(const char *written, const char *expected) {
    static char copy[MAX_FORMULA];
    if (strcmp(written, expected) == 0) return 1;
    snprintf(copy, sizeof copy, "%s", written);
    return prove(expected, 'n', 0, 0).outcome == PROVED && prove(copy, 'n', 0, 0).outcome == PROVED;
}

/* Controllo 5: ogni richiesta di path è risolta con la risposta di
 * expected_limits[]; una richiesta segnata open può anche restare senza
 * risposta, ma non con una risposta sbagliata. Ritorna 1 se il controllo è
 * riuscito. */
static int check_requests(const char *path) {
    static char written[MAX_FORMULA];
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }
    const char *position = text, *item, *end;
    int total = 0, right = 0, unanswered = 0, expected = (int)(sizeof expected_limits / sizeof *expected_limits);
    while (next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        const ExpectedLimit *wanted = total < expected ? &expected_limits[total] : NULL;
        total++;
        if (wanted && wanted->open && verdict.outcome != SOLVED) { unanswered++; continue; }

        /* Il testo va copiato: dimostrare l'uguaglianza riusa la memoria in
         * cui il motore lo ha scritto. */
        if (wanted && verdict.outcome == SOLVED && verdict.proof) {
            snprintf(written, sizeof written, "%s", verdict.detail);
            if (same_limit(written, wanted->answer)) { right++; continue; }
        }
        printf("  request %d (exercise %s): expected %s, got %s %s\n", total, wanted ? wanted->exercise : "?", wanted ? wanted->answer : "nothing",
               outcome_name(verdict.outcome), verdict.outcome == SOLVED ? written : verdict.reason ? verdict.reason : "");
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d limits computed and equal to the expected ones (%d not answered, as allowed)", right, total, unanswered);
    return check_result(total == expected && right + unanswered == total, summary);
}

/* Controllo 6: ogni enunciato di path, tolto ciò che dice del limite,
 * diventa una richiesta; la risposta del motore, riletta come enunciato,
 * deve essere dimostrata. Ritorna 1 se il controllo è riuscito. */
static int check_book_requests(const char *path) {
    static char request[MAX_FORMULA], written[MAX_FORMULA];
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }
    const char *position = text, *item, *end;
    int total = 0, right = 0;
    while (next_item(&position, &item, &end)) {
        Item statement;
        LimitStatement limit;
        total++;
        if (read_item(item, end, &statement) || !parse_limit_statement(statement.formula, &limit)) {
            printf("  statement %d of %s is not a limit\n", total, path);
            continue;
        }
        snprintf(request, sizeof request, "%.*s", (int)(limit.to - limit.from), limit.from);
        Verdict verdict = prove(request, 'n', 0, 0);
        if (verdict.outcome == SOLVED) {
            snprintf(written, sizeof written, "%s", verdict.detail);
            if (prove(written, 'n', 0, 0).outcome == PROVED) { right++; continue; }
        }
        printf("  statement %d of %s: the request %s is answered %s %s\n", total, path, request, outcome_name(verdict.outcome),
               verdict.outcome == SOLVED ? written : verdict.reason ? verdict.reason : "");
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d statements of %s, asked as requests, are answered with a limit that is then proved", right, total, path);
    return check_result(total > 0 && right == total, summary);
}

/* Controllo 7: ogni enunciato di special_cases[] riceve l'esito atteso.
 * Ritorna 1 se il controllo è riuscito. */
static int check_special_cases(void) {
    char summary[128];
    int total = (int)(sizeof special_cases / sizeof *special_cases), right = 0;
    for (int i = 0; i < total; i++) {
        Verdict verdict = prove(special_cases[i].statement, 'n', 0, 0);
        if (verdict.outcome == special_cases[i].expected) { right++; continue; }
        printf("  %s: expected %s, got %s (%s)\n", special_cases[i].name, outcome_name(special_cases[i].expected), outcome_name(verdict.outcome),
               verdict.reason ? verdict.reason : verdict.detail ? verdict.detail : "");
    }
    snprintf(summary, sizeof summary, "%d of %d statements on reading, domains, infinities and constants answered as expected", right, total);
    return check_result(right == total, summary);
}

int main(void) {
    engine_init();

    /* Tutti i controlli vengono eseguiti anche se uno fallisce, così il
     * resoconto è completo. */
    int passed = check_all_proved("tests/limits/book_limits.tex");
    passed = check_none_proved("tests/limits/false_limits.tex") && passed;
    passed = check_none_refuted("tests/limits/book_limits_beyond.tex") && passed;
    passed = check_outcomes("tests/limits/false_limits.tex", PROVED, REFUTED, 247) && passed;
    passed = check_requests("tests/limits/limit_requests.tex") && passed;
    passed = check_book_requests("tests/limits/book_limits.tex") && passed;
    passed = check_special_cases() && passed;

    /* Ogni limite calcolato passa il controllo con i numeri, e ogni valore
     * scritto viene riletto: nessuno deve essere stato scartato. */
    passed = check_result(unjustified_steps == 0, "every computed limit passed its check with numbers, and every written value was read back") && passed;
    return passed ? 0 : 1;
}
