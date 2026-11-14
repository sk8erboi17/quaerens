/* Test di dominio: le derivate delle raccolte di esercizi (capitolo 27 di
 * main.c): calcolarle, confrontarle con il risultato stampato, e usarle
 * dentro un'espressione per lo studio di una funzione.
 *
 * I controlli; quelli su un file intero sono descritti in
 * tests/statement_checks.h:
 *
 *   1. ogni enunciato di book_derivatives.tex viene dimostrato: la derivata
 *      di una funzione è il risultato stampato in una raccolta (corretto
 *      dove è sbagliato);
 *   2. nessun enunciato di false_book_derivatives.tex viene dimostrato, e
 *      tutti vengono dichiarati falsi: sono i risultati con un numero
 *      cambiato di uno o un segno rovesciato, e i quattro risultati stampati
 *      sbagliati;
 *   3. nessun enunciato di book_derivatives_beyond.tex viene dichiarato
 *      falso: sono veri, ma la forma stampata chiede identità che il conto
 *      esatto non applica;
 *   4. le richieste: per ogni enunciato di quei due file con l'ordine scritto
 *      come numero, la derivata chiesta senza il risultato (27k) deve avere
 *      la risposta (per il secondo file può anche mancare), e la risposta
 *      deve avere, in almeno un punto e senza
 *      smentite, lo stesso valore del risultato stampato. Il confronto si fa
 *      con gli intervalli del capitolo 24, che non passano né da derive() né
 *      dalla serie di Taylor; in nessuna risposta deve comparire ciò che un
 *      libro non scriverebbe (1 \cdot, + 0, due segni meno di fila);
 *   5. alcune richieste di derivative_requests.tex hanno proprio il testo
 *      atteso;
 *   6. ogni enunciato di function_studies.tex viene dimostrato: dove la
 *      derivata prima o seconda di una funzione è positiva, negativa o zero
 *      (27j, con la tabella dei segni);
 *   7. nessun enunciato di false_function_studies.tex viene dimostrato, e
 *      tutti vengono dichiarati falsi: un estremo spostato, un estremo
 *      compreso che non è una soluzione, un intervallo che passa per un
 *      punto in cui la funzione non ha valore, i risultati stampati
 *      sbagliati;
 *   8. nessun enunciato di function_studies_beyond.tex viene dichiarato
 *      falso;
 *   9. ogni richiesta di study_requests.tex riceve le soluzioni attese, o il
 *      verdetto atteso quando la derivata non si decide in un punto (27l);
 *  10. alcuni enunciati ricevono proprio il verdetto atteso;
 *  11. nei passaggi scritti un coefficiente ln(3) resta separato dai fattori
 *      successivi, anche con un caso base diverso da zero, e le formule
 *      alterate restano smentite (regressione dal fuzz, seme 18005).
 *
 * I dati vengono dalle raccolte di esercizi, e i risultati sono stati
 * confermati da un oracolo indipendente (vedi i file _about.tex); il
 * motore non ne conosce nessuno in anticipo.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_book_derivatives \
 *         tests/derivatives/test_book_derivatives.c && /tmp/test_book_derivatives
 */

#include "../statement_checks.h"

/* Ritorna il nome di un esito, per i messaggi. */
static const char *outcome_name(Outcome outcome) {
    switch (outcome) {
    case PROVED: return "PROVED";
    case REFUTED: return "REFUTED";
    case NOT_PROVED: return "NOT_PROVED";
    case UNSUPPORTED: return "UNSUPPORTED";
    case SOLVED: return "SOLVED";
    }
    return "?";
}

/* Conta gli enunciati di path per esito: nessuno deve ricevere forbidden, e
 * almeno at_least devono ricevere wanted. Ritorna 1 se il controllo è
 * riuscito. */
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

/* I punti in cui due formule vengono confrontate: sparsi, positivi e
 * negativi, diversi da quelli del motore (derivative_points[]). */
static const Frac comparison_points[] = {{-37, 10}, {-21, 10}, {-13, 10}, {-7, 10}, {-3, 10}, {2, 5}, {9, 10}, {7, 5}, {11, 5}, {33, 10}, {9, 2}, {61, 10}};

/* Ritorna 1 se i testi a e b, letti dal lettore del programma, hanno nella
 * variabile x lo stesso valore in almeno uno dei punti di prova, e in nessuno
 * hanno di sicuro valori diversi. I valori vengono dagli intervalli
 * (node_range()), con il confronto del motore (numbers_agree()). */
static int same_values(const char *a, const char *b) {
    static char copy[2][MAX_FORMULA];
    Node *tree[2];
    snprintf(copy[0], sizeof copy[0], "%s", a);
    snprintf(copy[1], sizeof copy[1], "%s", b);
    for (int i = 0; i < 2; i++) {
        tree[i] = parse_expression(copy[i]);
        if (syntax_error || failure) { syntax_error = failure = NULL; return 0; }
    }
    int agreed = 0;
    for (size_t p = 0; p < sizeof comparison_points / sizeof *comparison_points; p++) {
        Range at = range_ratio(comparison_points[p].num, comparison_points[p].den);
        int undefined = 0;
        int agreement = numbers_agree(node_range(tree[0], 'x', at, &undefined), node_range(tree[1], 'x', at, &undefined));
        if (agreement < 0) return 0;
        agreed += agreement > 0;
    }
    return agreed > 0;
}

/* Se formula è un enunciato \frac{d^{k}}{dx^{k}} f = g con k scritto come
 * numero, mette in request il testo fino al segno di uguale, cioè la
 * richiesta, e in *answer il testo dopo, e ritorna 1. Per trovare il segno
 * di uguale si usa il lettore del programma, come in make_variant(). */
static int split_derivative(const char *formula, char *request, size_t size, const char **answer) {
    int variable, order;
    long long times;
    arena_used = arena_keep;
    failure = syntax_error = NULL;
    cursor = formula;
    next_token();
    if (!is_command("frac") && !is_command("dfrac") && !is_command("tfrac")) return 0;
    parse_leibniz(&variable, &order, &times);
    parse_term();
    if (syntax_error || failure || order || !is_char('=')) { syntax_error = failure = NULL; return 0; }
    int written = snprintf(request, size, "%.*s", (int)(token.start - formula), formula);
    *answer = token.start + 1;
    return written > 0 && (size_t)written < size;
}

/* Ritorna 1 se il testo di una risposta contiene ciò che un libro non
 * scriverebbe dopo aver applicato le regole. */
static int untidy(const char *text) {
    static const char *const noise[] = {"1 \\cdot", "\\cdot 1 ", "+ 0", "0 +", "- -", "+ -", "^{1}", "--"};
    for (size_t i = 0; i < sizeof noise / sizeof *noise; i++)
        if (strstr(text, noise[i])) return 1;
    return 0;
}

/* Controllo 4: le richieste ricavate dagli enunciati di path. Mette in
 * *count quante sono e ritorna quante hanno la risposta giusta e pulita; con
 * optional diverso da zero una richiesta può anche restare senza risposta
 * (NON DIMOSTRATO o NON SUPPORTATO), e allora conta come giusta. */
static int requests_from(const char *path, int optional, int *count) {
    static char request[MAX_FORMULA], answer_text[MAX_FORMULA], written[MAX_FORMULA];
    char *text = read_file(path);
    if (!text) { printf("  cannot read %s\n", path); return 0; }
    const char *position = text, *item, *end, *answer;
    int right = 0, number = 0;
    while (next_item(&position, &item, &end)) {
        Item statement;
        number++;
        if (read_item(item, end, &statement) || !split_derivative(statement.formula, request, sizeof request, &answer)) continue;
        snprintf(answer_text, sizeof answer_text, "%s", answer);
        (*count)++;
        Verdict verdict = prove(request, 'n', 0, 0);
        if (optional && (verdict.outcome == NOT_PROVED || verdict.outcome == UNSUPPORTED)) { right++; continue; }
        if (verdict.outcome == SOLVED && verdict.proof) {
            /* Il testo va copiato: rileggerlo riusa la memoria del motore. */
            snprintf(written, sizeof written, "%s", verdict.detail);
            if (same_values(written, answer_text) && !untidy(written)) { right++; continue; }
        }
        printf("  request from statement %d of %s: %s %s\n    expected the value of %s\n", number, path, outcome_name(verdict.outcome),
               verdict.outcome == SOLVED ? written : (verdict.reason ? verdict.reason : ""), answer_text);
    }
    free(text);
    return right;
}

static int check_requests(void) {
    char summary[256];
    int count = 0, right = requests_from("tests/derivatives/book_derivatives.tex", 0, &count);
    right += requests_from("tests/derivatives/book_derivatives_beyond.tex", 1, &count);
    snprintf(summary, sizeof summary, "%d of %d derivatives asked without their result are computed, written tidily, and equal to the printed result "
             "(those of the statements not decided yet may stay unanswered)", right, count);
    return check_result(count > 0 && right == count, summary);
}

/* Controllo 5: il testo atteso di alcune risposte. */
typedef struct {
    const char *request;
    const char *answer;  /* la risposta, carattere per carattere */
} ExpectedAnswer;

static const ExpectedAnswer expected_answers[] = {
    {"\\frac{d}{dx}\\left( x^{2} \\tan x \\right)", "2x \\cdot \\tan\\left(x\\right) + \\frac{x^{2}}{\\cos^{2}\\left(x\\right)}"},
    {"\\frac{d}{dx}\\left( \\arcsin x \\right)", "\\frac{1}{\\sqrt{1 - x^{2}}}"},
    {"\\frac{d^{2}}{dx^{2}}\\left( x^{3} - 2x \\right)", "6x"},
    {"\\frac{d}{dx}\\left( \\ln|x| \\right)", "\\frac{1}{x}"},
    {"\\frac{d}{dx}\\left( |x - 1| \\right)", "\\frac{x - 1}{\\left|x - 1\\right|}"},
    {"\\frac{d}{dx}\\left( \\sqrt{x^{2} + 1} \\right)", "\\frac{\\sqrt{x^{2} + 1} \\cdot x}{x^{2} + 1}"},
    {"\\frac{d}{dx}\\left( \\sqrt[3]{x} \\right)", "\\frac{\\sqrt[3]{x}}{3x}"},
    /* Un segno di derivata dentro la funzione si espande prima (27j); la
     * sua condizione di esistenza, 0 per qualcosa definito ovunque, non si
     * scrive. */
    {"\\frac{d}{dx}\\left( \\frac{d}{dx}\\left( x^{3} \\right) \\right)", "6x"},
};

static int check_answers(void) {
    char summary[128];
    int count = (int)(sizeof expected_answers / sizeof *expected_answers), right = 0;
    for (int i = 0; i < count; i++) {
        Verdict verdict = prove(expected_answers[i].request, 'n', 0, 0);
        if (verdict.outcome == SOLVED && strcmp(verdict.detail, expected_answers[i].answer) == 0) { right++; continue; }
        printf("  %s: expected %s, got %s %s\n", expected_answers[i].request, expected_answers[i].answer, outcome_name(verdict.outcome),
               verdict.outcome == SOLVED ? verdict.detail : verdict.reason);
    }
    snprintf(summary, sizeof summary, "%d of %d derivatives written as expected", right, count);
    return check_result(right == count, summary);
}

/* Controllo 9: le soluzioni attese delle richieste di study_requests.tex,
 * nell'ordine del file. Invariante: una voce per richiesta. */
typedef struct {
    Outcome outcome;
    const char *solutions;  /* SOLVED: le soluzioni, come le scrive il motore */
    const char *note;       /* la nota attesa, o NULL se non deve essercene una */
} ExpectedStudy;

static const ExpectedStudy expected_studies[] = {
    {SOLVED, "x < -3 \\lor x > -1", NULL},
    {SOLVED, "x < -2 \\lor 0 < x < \\frac{1}{2}", NULL},
    {SOLVED, "x < -1 - \\sqrt{2} \\lor x > -1 + \\sqrt{2}", NULL},
    {SOLVED, "-1 < x < 1", NULL},
    {SOLVED, "x < -3 - \\sqrt{3} \\lor -3 + \\sqrt{3} < x < 0", NULL},
    {SOLVED, "-1 < x < -\\frac{2}{3}", NULL},
    {SOLVED, "x = \\frac{\\pi}{6} + \\frac{k\\pi}{3}", "k intero"},
    {SOLVED, "0 \\le x < \\frac{\\pi}{3} \\lor \\frac{5}{3}\\pi < x \\le 2\\pi", NULL},
    {SOLVED, "0 < x < e^{-3}", NULL},
    /* ln(x^2 - 1) è definito solo per |x| > 1: dove -1 < x < 0 la formula
     * della derivata, 2x / (x^2 - 1), è positiva, ma lì la funzione non c'è. */
    {SOLVED, "x > 1", NULL},
    {SOLVED, "\\nexists x \\in \\mathbb{R}", NULL},
    /* In 0 la radice è definita, ma non a sinistra: lì non c'è la derivata. */
    {SOLVED, "x > 0", "negli estremi del dominio la derivata, che chiede la funzione definita da tutte e due le parti, non c'è"},
    {SOLVED, "x > -1", NULL},
    {SOLVED, "-1 < x < 0 \\lor 0 < x < 1", "negli estremi del dominio la derivata, che chiede la funzione definita da tutte e due le parti, non c'è"},
    /* x|x| è derivabile anche in 0, ma le regole lì non danno la derivata:
     * la risposta non si dà (27l). */
    {NOT_PROVED, NULL, NULL},
    {SOLVED, "x < \\frac{1}{6}", NULL},
};

static int check_studies(void) {
    char summary[256];
    const char *path = "tests/derivatives/study_requests.tex";
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }
    const char *position = text, *item, *end;
    int total = 0, right = 0, expected = (int)(sizeof expected_studies / sizeof *expected_studies);
    while (next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        const ExpectedStudy *wanted = total < expected ? &expected_studies[total] : NULL;
        total++;
        int same = wanted && verdict.outcome == wanted->outcome;
        if (same && wanted->solutions) same = verdict.detail && strcmp(verdict.detail, wanted->solutions) == 0;
        if (same && verdict.outcome == SOLVED) same = wanted->note ? verdict.note && strcmp(verdict.note, wanted->note) == 0 : !verdict.note;
        if (same) { right++; continue; }
        printf("  request %d: expected %s %s, got %s %s\n", total, wanted ? outcome_name(wanted->outcome) : "nothing", wanted && wanted->solutions ? wanted->solutions : "",
               outcome_name(verdict.outcome), verdict.outcome == SOLVED ? verdict.detail : (verdict.reason ? verdict.reason : ""));
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d requests on studies of functions answered as expected", right, total);
    return check_result(total == expected && right == total, summary);
}

/* Controllo 10: verdetti attesi. */
typedef struct {
    const char *formula;
    Outcome outcome;
    const char *reason_part;  /* un pezzo che il motivo, o la nota, deve contenere; NULL se non conta */
} Expected;

static const Expected expected[] = {
    /* u^v con x nella base e nell'esponente: il verdetto dice come lo si
     * intende. */
    {"\\frac{d}{dx}\\left( x^{x} \\right) = x^{x}(\\ln x + 1)", PROVED, "e^{v ln u}"},
    {"\\frac{d}{dx}\\left( x^{x} \\right) = x^{x}\\ln x", REFUTED, NULL},
    /* Le regole nuove, giuste e sbagliate. */
    {"\\frac{d}{dx}\\left( \\arccos\\left(\\frac{x}{2}\\right) \\right) = -\\frac{1}{\\sqrt{4 - x^{2}}}", PROVED, NULL},
    {"\\frac{d}{dx}\\left( \\arccot x \\right) = \\frac{1}{1 + x^{2}}", REFUTED, NULL},
    {"\\frac{d}{dx}\\left( \\log_{x} 2 \\right) = -\\frac{\\ln 2}{x \\ln^{2} x}", PROVED, NULL},
    {"\\frac{d}{dx}\\left( \\log_{e} x \\right) = \\frac{1}{x}", PROVED, NULL},
    {"\\frac{d}{dx}\\left( \\left|x^{2} - 1\\right| \\right) = \\frac{2x (x^{2} - 1)}{\\left|x^{2} - 1\\right|}", PROVED, NULL},
    {"\\frac{d}{dx}\\left( \\left|x^{2} - 1\\right| \\right) = 2x", REFUTED, NULL},
    {"\\frac{d^{2}}{dx^{2}}\\left( \\sqrt[3]{x} \\right) = -\\frac{2}{9\\sqrt[3]{x^{5}}}", PROVED, NULL},
    {"\\frac{d}{dx}\\left( \\sqrt[3]{x^{2}} \\right) = \\frac{2}{3\\sqrt[3]{\\left|x\\right|}}", REFUTED, NULL},
    /* Le richieste che non si fanno. */
    {"\\frac{d^{n}}{dx^{n}}\\left( x^{2} \\right)", UNSUPPORTED, NULL},
    {"\\frac{d}{dx}\\left( x^{2} \\right) + 1", UNSUPPORTED, NULL},
    {"\\frac{d^{5}}{dx^{5}}\\left( x^{6} \\right)", UNSUPPORTED, NULL},
    {"\\frac{d}{dx}\\left( x! \\right)", UNSUPPORTED, NULL},
    /* Il segno dentro un'espressione, dove non ha senso: in un enunciato
     * sugli insiemi, con l'ordine n, con un parametro. */
    {"\\frac{d}{dx}\\left( x \\right) = 1 \\implies x = x", UNSUPPORTED, NULL},
    {"\\forall a \\in \\mathbb{R}\\ \\{x \\in \\mathbb{R} \\mid \\frac{d}{dx}\\left( a x^{2} \\right) > 0\\}", UNSUPPORTED, NULL},
    /* Dove la funzione è definita e le regole non danno la derivata, non si
     * decide: x|x| è derivabile in 0, |x| no. */
    {"\\forall x \\in \\mathbb{R}\\ \\left(\\frac{d}{dx}\\left( x|x| \\right) = 2|x|\\right)", NOT_PROVED, "x = 0"},
    {"\\forall x \\in \\mathbb{R}\\ \\left(\\frac{d}{dx}\\left( |x| \\right) > 0 \\iff x > 0\\right)", NOT_PROVED, "x = 0"},
};

static int check_expected(void) {
    char summary[128];
    int count = (int)(sizeof expected / sizeof *expected), passed = 0;
    for (int i = 0; i < count; i++) {
        const Expected *e = &expected[i];
        Verdict verdict = prove(e->formula, 'n', 0, 0);
        int good = verdict.outcome == e->outcome;
        if (good && e->reason_part) good = (verdict.reason && strstr(verdict.reason, e->reason_part)) || (verdict.note && strstr(verdict.note, e->reason_part));
        if (good) { passed++; continue; }
        printf("  unexpected verdict: %s\n    got: ", e->formula);
        verdict_print(stdout, &verdict);
        putchar('\n');
    }
    snprintf(summary, sizeof summary, "%d of %d expected verdicts on derivatives and studies", passed, count);
    return check_result(passed == count, summary);
}

/* Antiregressione dal fuzz, seme 18005: nei conti del passo induttivo una
 * costante ln(3) precedeva e^x, e il testo senza parentesi cambiava argomento
 * al logaritmo. La derivata n-esima di log_3(x+1) si ricava derivando 1/(x+1)
 * n-1 volte e dividendo per ln(3); per x e^x e (x+n)e^x, per Leibniz.
 * Si verificano la rilettura di ogni riga, il certificato e una formula
 * falsa con un termine e^x in piu. */
static int check_written_logarithm_products(void) {
    const char *correct = "\\frac{d^{n}}{dx^{n}}(\\log_{3}(x+1) + x e^x) = "
                          "\\frac{(-1)^{n-1}(n-1)!}{(x+1)^n \\ln(3)} + (x+n)e^x";
    const char *wrong = "\\frac{d^{n}}{dx^{n}}(\\log_{3}(x+1) + x e^x) = "
                        "\\frac{(-1)^{n-1}(n-1)!}{(x+1)^n \\ln(3)} + (x+n+1)e^x";
    int passed = 1;
    for (int base = 1; base <= 4; base++) {
        int before = unjustified_steps;
        Verdict v = prove(correct, 'n', 1, base);
        passed = check_result(v.outcome == PROVED && v.certificate && unjustified_steps == before,
                              "a logarithmic coefficient in a written derivative proof is reread correctly and certified") && passed;
        v = prove(wrong, 'n', 1, base);
        passed = check_result(v.outcome == REFUTED, "an extra exponential term in the claimed derivative is refuted") && passed;
    }
    return passed;
}

int main(void) {
    engine_init();

    /* Tutti i controlli vengono eseguiti anche se uno fallisce, così il
     * resoconto è completo. */
    int passed = check_all_proved("tests/derivatives/book_derivatives.tex");
    passed = check_outcomes("tests/derivatives/false_book_derivatives.tex", PROVED, REFUTED, 91) && passed;
    passed = check_none_refuted("tests/derivatives/book_derivatives_beyond.tex") && passed;
    passed = check_requests() && passed;
    passed = check_answers() && passed;
    passed = check_all_proved("tests/derivatives/function_studies.tex") && passed;
    passed = check_outcomes("tests/derivatives/false_function_studies.tex", PROVED, REFUTED, 31) && passed;
    passed = check_none_refuted("tests/derivatives/function_studies_beyond.tex") && passed;
    passed = check_studies() && passed;
    passed = check_expected() && passed;
    passed = check_written_logarithm_products() && passed;

    /* Ogni derivata, riga della tabella dei segni e formula scritta viene
     * ricontrollata dal programma con un metodo indipendente: nessuna deve
     * essere stata scartata. */
    passed = check_result(unjustified_steps == 0, "every derivative, row of a sign table and written formula passed its independent check") && passed;
    return passed ? 0 : 1;
}
