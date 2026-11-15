/* Test di dominio: lo svolgimento di una derivata, una regola alla volta
 * (regola 27n di main.c, e capitolo 41 di output.c per il testo).
 *
 * La risposta di queste richieste la controlla già test_derivatives.c. Qui
 * si controlla la strada: per ogni richiesta di worked_derivatives.tex il
 * motore deve aver registrato una riga per ogni pezzo dell'espressione,
 * dal più interno, con la derivata del pezzo e la regola usata. Le righe
 * attese sono scritte qui sotto come formule, ricavate a mano, e si
 * confrontano con quelle registrate per valore (il test di annullamento del
 * capitolo 12), non per come sono scritte: così il test non dipende dalla
 * forma in cui il motore scrive una derivata.
 *
 *   1. Ogni richiesta ha la sua spiegazione, con le righe attese: lo stesso
 *      pezzo, la stessa derivata, la stessa regola, nello stesso ordine.
 *   2. Il controllo di ogni riga è vivo: una riga giusta è accettata, una
 *      con la derivata sbagliata (un fattore dimenticato, un segno, un
 *      addendo in meno) è respinta dal verificatore.
 *   3. Ciò che non si sa spiegare resta senza spiegazione, con la sua
 *      risposta: una derivata seconda, e l'arcoseno, che il verificatore
 *      non conosce.
 *   4. Nessuna riga ha fallito il controllo (unjustified_steps a zero).
 *   5. Il documento: ogni voce comincia con lo svolgimento, ha le righe con
 *      la loro regola e conserva in fondo la soluzione di sempre.
 *
 * I file vengono scritti nella cartella indicata da TMPDIR, o in /tmp: mai
 * nel repository. Il test non compila il documento in PDF.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_worked_derivatives \
 *         tests/derivatives/test_worked_derivatives.c \
 *         && /tmp/test_worked_derivatives
 */

#include "../statement_checks.h"

static int checks, failures;

/* Registra l'esito di un controllo sulla richiesta numero item (0: nessuna
 * in particolare); se è fallito lo stampa. */
static void check(int passed, int item, const char *what) {
    checks++;
    if (passed) return;
    failures++;
    if (item) printf("  failed, request %d: %s\n", item, what);
    else printf("  failed: %s\n", what);
}

/* Una riga attesa: il pezzo, la sua derivata, la regola, e per
 * RULE_FUNCTION la funzione. piece = NULL chiude l'elenco di una richiesta. */
typedef struct {
    const char *piece, *derivative;
    DerivativeRule rule;
    Function function;
} Line;

/* Le righe attese, richiesta per richiesta, nell'ordine in cui i pezzi si
 * incontrano andando dal più interno al più esterno, da sinistra. */
static const Line expected[][5] = {
    /* x^2 tan x: (x^2)' = 2x, (tan x)' = 1/cos^2 x, poi il prodotto. */
    {{"x^{2}", "2x", RULE_POWER, 0},
     {"\\tan x", "\\frac{1}{\\cos^{2} x}", RULE_FUNCTION, FN_TAN},
     {"x^{2} \\tan x", "2x \\tan x + \\frac{x^{2}}{\\cos^{2} x}", RULE_PRODUCT, 0},
     {NULL, NULL, 0, 0}},
    /* (x^2 - 3)/(x + 2): numeratore 2x, denominatore 1, poi il quoziente
     * (2x (x + 2) - (x^2 - 3)) / (x + 2)^2. */
    {{"x^{2}", "2x", RULE_POWER, 0},
     {"x^{2} - 3", "2x", RULE_SUM, 0},
     {"x + 2", "1", RULE_SUM, 0},
     {"\\frac{x^{2} - 3}{x + 2}", "\\frac{2x(x + 2) - (x^{2} - 3)}{(x + 2)^{2}}", RULE_QUOTIENT, 0},
     {NULL, NULL, 0, 0}},
    /* sin(3x): (3x)' = 3, poi la catena: 3 cos(3x). */
    {{"3x", "3", RULE_FACTOR, 0},
     {"\\sin(3x)", "3\\cos(3x)", RULE_FUNCTION, FN_SIN},
     {NULL, NULL, 0, 0}},
    /* e^x sin x: (e^x)' = e^x, (sin x)' = cos x, poi il prodotto. */
    {{"e^{x}", "e^{x}", RULE_EXPONENTIAL, 0},
     {"\\sin x", "\\cos x", RULE_FUNCTION, FN_SIN},
     {"e^{x} \\sin x", "e^{x} \\sin x + e^{x} \\cos x", RULE_PRODUCT, 0},
     {NULL, NULL, 0, 0}},
    /* sqrt(x^2 + 1): (x^2 + 1)' = 2x, poi la catena: 2x / (2 sqrt(x^2 + 1)). */
    {{"x^{2}", "2x", RULE_POWER, 0},
     {"x^{2} + 1", "2x", RULE_SUM, 0},
     {"\\sqrt{x^{2} + 1}", "\\frac{x}{\\sqrt{x^{2} + 1}}", RULE_FUNCTION, FN_ROOT},
     {NULL, NULL, 0, 0}},
    /* (2x + 1)^3: (2x)' = 2, (2x + 1)' = 2, poi la potenza con la catena:
     * 3 (2x + 1)^2 per 2. */
    {{"2x", "2", RULE_FACTOR, 0},
     {"2x + 1", "2", RULE_SUM, 0},
     {"(2x + 1)^{3}", "6(2x + 1)^{2}", RULE_POWER, 0},
     {NULL, NULL, 0, 0}},
};

/* Ritorna 1 se le espressioni scritte in a e in b hanno lo stesso valore
 * dove sono definite, per il test di annullamento del capitolo 12. */
static int same_value(const char *a, const char *b) {
    Scratch mark = scratch_mark();
    failure = syntax_error = NULL;
    env_count = 0;
    Rat gap = rat_add(eval(parse_expression(a)), eval(parse_expression(b)), 1);
    int same = !syntax_error && !failure && (gap.num.count == 0 || poly_vanishes(gap.num));
    failure = syntax_error = NULL;
    scratch_drop(mark);
    return same;
}

/* Controllo 1 sulle richieste di path. */
static void check_lines(const char *path) {
    char *text = read_file(path);
    const char *position = text, *item, *end;
    int number = 0;
    check(text != NULL, 0, "the file of the requests can be read");
    if (!text) return;
    while (next_item(&position, &item, &end)) {
        if (++number > (int)(sizeof expected / sizeof *expected)) break;
        const Line *want = expected[number - 1];
        Verdict verdict = prove_item(item, end);
        check(verdict.outcome == SOLVED, number, "the derivative is found");
        check(explanation.steps > 0 && explanation.request == EXPLAIN_DERIVATIVE, number, "the request has an explanation");
        if (!explanation.steps) continue;
        check(explanation.step[0].kind == EXPLAIN_ASK && explanation.step[explanation.steps - 1].kind == EXPLAIN_ANSWER, number,
              "the explanation opens with what is asked and closes with the answer");

        /* Le righe, una per una. */
        int lines = 0;
        while (want[lines].piece) lines++;
        check(explanation.steps == lines + 2, number, "the explanation has one line for each expected piece");
        for (int l = 0; l < lines && l + 1 < explanation.steps - 1; l++) {
            const ExplainStep *step = &explanation.step[l + 1];
            check(step->kind == EXPLAIN_DERIVE && same_value(step->text[0], want[l].piece), number, "a line is about the expected piece");
            check(same_value(step->text[1], want[l].derivative), number, "a line gives the expected derivative of its piece");
            check(step->flag == (int)want[l].rule && (want[l].rule != RULE_FUNCTION || step->count == (int)want[l].function), number, "a line names the expected rule");
        }

        /* L'ultima riga è tutta l'espressione, e la sua derivata è la
         * risposta del verdetto. */
        const ExplainStep *last = &explanation.step[explanation.steps - 2], *answer = &explanation.step[explanation.steps - 1];
        check(strcmp(last->text[0], explanation.step[0].text[0]) == 0 && strcmp(last->text[1], verdict.detail) == 0 && strcmp(answer->text[0], verdict.detail) == 0,
              number, "the last line is the whole expression, and its derivative is the answer of the verdict");
    }
    check(number == (int)(sizeof expected / sizeof *expected), 0, "the file has one request for each expected explanation");
    free(text);
}

/* Ritorna 1 se il verificatore accetta la riga "la derivata di function è
 * derivative", e controlla che un rifiuto sia annotato come errore. */
static int line_accepted(const char *function, const char *derivative) {
    explain_wrong = explain_broken = 0;
    int accepted = explain_derivative_line(function, derivative, 'x');
    check(accepted ? !explain_wrong && !explain_broken : explain_wrong && !explain_broken, 0, "a refused line is recorded as a wrong step, an accepted one as nothing");
    return accepted;
}

/* Controllo 2: righe giuste e righe false, date al controllo di ogni riga. */
static void check_liars(void) {
    Scratch mark = scratch_mark();
    failure = NULL;
    check(line_accepted("x^{2}", "2x"), 0, "the derivative of x^2 is 2x");
    check(!line_accepted("x^{2}", "x"), 0, "x is refused as the derivative of x^2: a factor forgotten");
    check(!line_accepted("x^{2}", "2x^{2}"), 0, "2x^2 is refused as the derivative of x^2: the exponent not lowered");
    check(line_accepted("\\sin\\left(x\\right)", "\\cos\\left(x\\right)"), 0, "the derivative of sin x is cos x");
    check(!line_accepted("\\sin\\left(x\\right)", "-\\cos\\left(x\\right)"), 0, "-cos x is refused as the derivative of sin x: a sign");
    check(line_accepted("x^{2} \\cdot \\tan\\left(x\\right)", "2x \\cdot \\tan\\left(x\\right) + \\frac{x^{2}}{\\cos^{2}\\left(x\\right)}"), 0, "the product rule on x^2 tan x is accepted");
    check(!line_accepted("x^{2} \\cdot \\tan\\left(x\\right)", "2x \\cdot \\tan\\left(x\\right)"), 0, "a product rule with one term missing is refused");
    check(!line_accepted("x^{2} \\cdot \\tan\\left(x\\right)", "\\frac{2x}{\\cos^{2}\\left(x\\right)}"), 0, "the product of the two derivatives is refused");
    check(!line_accepted("\\sin\\left(3x\\right)", "\\cos\\left(3x\\right)"), 0, "a chain rule without the inner derivative is refused");
    check(!line_accepted("\\frac{x + 1}{x - 1}", "\\frac{x + 1 - \\left(x - 1\\right)}{\\left(x - 1\\right)^{2}}"), 0, "a quotient rule with the two terms exchanged is refused");
    check(!line_accepted("x^{2}", "2x +"), 0, "a line that cannot be read back is refused");
    failure = NULL;
    scratch_drop(mark);
}

/* Controllo 3: ciò che resta senza spiegazione. */
static void check_unexplained(void) {
    static const char *const outside[] = {
        "\\frac{d^{2}}{dx^{2}}\\left( x^{3} - 2x \\right)",
        "\\frac{d}{dx}\\left( \\arcsin x \\right)",
    };
    for (size_t i = 0; i < sizeof outside / sizeof *outside; i++) {
        Verdict verdict = prove("\\frac{d}{dx}\\left( x^{2} \\tan x \\right)", 'n', 0, 0);
        check(verdict.outcome == SOLVED && explanation.steps > 0, 0, "a first derivative the checker knows is explained");
        verdict = prove(outside[i], 'n', 0, 0);
        check(verdict.outcome == SOLVED && verdict.proof && explanation.steps == 0, 0, "a request outside the layer keeps its answer, without an explanation");
    }
}

/* Ritorna quante volte needle compare in text. */
static int count(const char *text, const char *needle) {
    int found = 0;
    for (const char *at = strstr(text, needle); at; at = strstr(at + 1, needle)) found++;
    return found;
}

/* Controllo 5: il documento delle richieste di path. */
static void check_document(const char *path) {
    const char *folder = getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp";
    char name[1024];
    Document document;
    Report report;
    int requests = (int)(sizeof expected / sizeof *expected);
    snprintf(name, sizeof name, "%s/mathresearcher_test_worked_derivatives.tex", folder);
    check(document_open(&document, name) == NULL, 0, "the document can be opened");
    document_section(&document, path);
    int read = prove_file(path, &report, NULL, &document, NULL);
    check(document_close(&document) == NULL && read, 0, "the document is written without problems");
    char *tex = read_file(name);
    check(tex != NULL, 0, "the document exists");
    if (!tex) return;
    check(count(tex, "\\textbf{Worked solution.}") == requests, 0, "every item opens with the worked solution");
    check(strstr(tex, "\\emph{What is asked.} The derivative with respect to $x$ of\n\\[ \\fitmath{x^{2} \\cdot \\tan\\left(x\\right)} \\]") != NULL, 0,
          "the worked solution starts from what is asked");
    check(strstr(tex, "\\item $\\displaystyle \\frac{d}{dx}\\left(x^{2}\\right) = 2x$\\quad power rule: $(u^{c})' = c\\,u^{c-1}u'$, with $u = x$ and $u' = 1$.") != NULL, 0,
          "a line is written with its rule");
    check(strstr(tex, "\\quad product rule: $(u\\,v)' = u'v + u\\,v'$.") != NULL, 0, "the product rule is named");
    check(strstr(tex, "\\quad $(\\sin u)' = \\cos u\\cdot u'$, with $u$ the inner expression (chain rule).") != NULL, 0, "the chain rule is named where the inside is not the variable");
    check(count(tex, "{\\small \\textbf{The proof on record.} \\textbf{Solution.} By the rules of differentiation") == requests, 0, "the solution of always stays, at the end of each item");
    check(count(tex, "\\includegraphics") == 0, 0, "a derivative has no graph yet");
    free(tex);
}

int main(void) {
    engine_init();
    explanations_wanted = 1;
    check_lines("tests/derivatives/worked_derivatives.tex");
    check_liars();
    check_unexplained();
    check(unjustified_steps == 0, 0, "no recorded line failed its independent check");
    check_document("tests/derivatives/worked_derivatives.tex");
    check(unjustified_steps == 0, 0, "no line failed its independent check while the document was written");
    printf("%s: %d of %d checks on worked derivatives passed\n", failures ? "FAIL" : "PASS", checks - failures, checks);
    return failures ? 1 : 0;
}
