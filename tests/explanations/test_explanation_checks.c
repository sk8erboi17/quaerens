/* Test dello strato di spiegazione in generale (capitolo 40 di main.c): non
 * di un tipo di esercizio, ma delle garanzie che lo strato deve dare per
 * tutti.
 *
 * Il principio è che ogni passaggio mostrato è un fatto ricontrollato, e
 * che un passaggio che non regge non si mostra. Un controllo che accetta
 * tutto non controlla nulla: per questo qui i controlli vengono messi alla
 * prova con dei passaggi falsi, come i certificati sbagliati in
 * tests/checker/.
 *
 *   1. I controlli di 40a, uno alla volta. Una forma della catena giusta è
 *      accettata; una con il segno di un fattore cambiato, con un termine
 *      sbagliato, con una parentesi dimenticata è respinta. Un valore
 *      sostituito in un'espressione dà il segno vero, anche se è un numero
 *      con una radice, e dice quando l'espressione lì non ha un valore.
 *   2. Il bugiardo dentro il motore. Decisa una tabella dei segni, la si
 *      altera prima che la spiegazione venga registrata: una riga con i
 *      segni rovesciati, una radice messa nell'intervallo dell'altra, una
 *      risposta diversa da quella della tabella. In ogni caso la
 *      spiegazione non deve esserci, e unjustified_steps deve salire. La
 *      stessa tabella, non alterata, viene spiegata.
 *   3. Lo strato non decide. Per ogni enunciato di alcuni file di dati, di
 *      domini diversi, il verdetto con le spiegazioni accese è quello con
 *      le spiegazioni spente: esito, soluzioni, dimostrazione, certificato,
 *      motivo e nota, carattere per carattere. Tra i file ci sono le
 *      derivate, la cui spiegazione usa il deposito dei certificati prima
 *      che vi si scriva quello del verdetto (27n), e le induzioni, la cui
 *      spiegazione rilegge l'enunciato dopo il verdetto (40b).
 *   4. Ciò che lo strato non sa spiegare resta senza spiegazione, con il
 *      suo verdetto: valori assoluti, radici, esponenziali, funzioni
 *      goniometriche, una radice senza scrittura con i radicali, una
 *      negazione davanti, un parametro.
 *   5. La spiegazione di un enunciato non resta attaccata al successivo.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_explanation_checks \
 *         tests/explanations/test_explanation_checks.c \
 *         && /tmp/test_explanation_checks
 */

#include "../statement_checks.h"

static int checks, failures;

/* Registra l'esito di un controllo; se è fallito lo stampa. */
static void check(int passed, const char *what) {
    checks++;
    if (passed) return;
    failures++;
    printf("  failed: %s\n", what);
}

/* Ritorna il valore dell'espressione scritta in text, con le lettere
 * libere: ciò con cui explain_equals() confronta una forma scritta. */
static Rat value_of(const char *text) {
    env_count = 0;
    return eval(parse_expression(text));
}

/* Ritorna 1 se explain_equals() accetta text come scrittura di expected, e
 * controlla che un rifiuto sia annotato come errore (explain_wrong) e non
 * come limite del programma. */
static int accepted(const char *text, Rat expected) {
    explain_wrong = explain_broken = 0;
    int same = explain_equals(text, expected);
    check(same ? !explain_wrong && !explain_broken : explain_wrong && !explain_broken, "a refused form is recorded as a wrong step, an accepted one as nothing");
    return same;
}

/* Controllo 1: i controlli di 40a, con forme giuste e forme false. */
static void check_controls(void) {
    Scratch mark = scratch_mark();
    failure = NULL;

    /* La catena di (x - 2)/(x + 3) >= x: la differenza dei membri è
     * (-x^2 - 2x - 2)/(x + 3) = -(x^2 + 2x + 2)/(x + 3). */
    Rat gap = value_of("\\frac{x - 2}{x + 3} - x");
    check(accepted("\\frac{-x^{2} - 2x - 2}{x + 3}", gap), "the difference written as one fraction is accepted");
    check(accepted("\\frac{-\\left(x^{2} + 2x + 2\\right)}{x + 3}", gap), "the fraction with its factors is accepted");
    check(!accepted("\\frac{\\left(x^{2} + 2x + 2\\right)}{x + 3}", gap), "a factor with its sign changed is refused");
    check(!accepted("\\frac{-x^{2} - 2x + 2}{x + 3}", gap), "a numerator with one sign wrong is refused");
    check(!accepted("\\frac{-x^{2} - 2x - 2}{x - 3}", gap), "a denominator with one sign wrong is refused");
    check(!accepted("\\frac{-\\left(x^{2} + 2x + 2\\right)}{x + 3} + 1", gap), "a form that differs by a constant is refused");

    /* x^2 - 5x + 6 = (x - 2)(x - 3). */
    Rat product = value_of("x^{2} - 5x + 6");
    check(accepted("\\left(x - 2\\right)\\left(x - 3\\right)", product), "a factorization is accepted");
    check(!accepted("\\left(x + 2\\right)\\left(x - 3\\right)", product), "a factorization with a wrong root is refused");
    check(!accepted("x - 2\\left(x - 3\\right)", product), "a factorization with a parenthesis forgotten is refused");
    check(!accepted("\\left(x - 2\\right)\\left(x - 3\\right)^{2}", product), "a factorization with a wrong exponent is refused");
    check(!accepted("\\left(x - 2\\right)\\left(x - 3", product), "a form that cannot be read back is refused");

    /* Un valore sostituito: il segno, e dove non c'è un valore. */
    int side = 5, defined = 5;
    explain_wrong = explain_broken = 0;
    check(explain_sign_at(parse_expression("x - 2"), 'x', rat_int(2), &side, &defined) && defined && side == 0, "x - 2 is zero at 2");
    check(explain_sign_at(parse_expression("x - 2"), 'x', rat_int(3), &side, &defined) && defined && side == 1, "x - 2 is positive at 3");
    check(explain_sign_at(parse_expression("\\frac{x - 2}{x + 3}"), 'x', rat_int(-5), &side, &defined) && defined && side == 1, "(x - 2)/(x + 3) is positive at -5");
    check(explain_sign_at(parse_expression("\\frac{x - 2}{x + 3}"), 'x', rat_int(-3), &side, &defined) && !defined, "(x - 2)/(x + 3) has no value at -3");
    check(explain_sign_at(parse_expression("x^{2} - 2"), 'x', value_of("-\\sqrt{2}"), &side, &defined) && defined && side == 0, "x^2 - 2 is zero at -sqrt(2), exactly");
    check(explain_sign_at(parse_expression("x^{2} - 2"), 'x', value_of("\\frac{3}{2}"), &side, &defined) && defined && side == 1, "x^2 - 2 is positive at 3/2");
    check(explain_sign_at(parse_expression("x - 1"), 'x', value_of("\\frac{1 - \\sqrt{5}}{2}"), &side, &defined) && defined && side == -1, "x - 1 is negative at (1 - sqrt(5))/2");
    check(explain_sign_at(parse_expression("\\frac{1}{x^{2} - x - 1}"), 'x', value_of("\\frac{1 - \\sqrt{5}}{2}"), &side, &defined) && !defined,
          "1/(x^2 - x - 1) has no value at (1 - sqrt(5))/2, a root of the denominator");
    check(!explain_wrong && !explain_broken && !failure, "these computations raise neither an error nor a limit");
    failure = NULL;
    scratch_drop(mark);
}

/* Decide formula con la tabella dei segni e ne ritorna la tesi, con il
 * verdetto in *verdict, lasciando la tabella com'è dopo la decisione: ciò
 * che decide_quantified() fa prima di chiamare explain_signs(), che qui
 * resta da chiamare. Serve ad alterare la tabella tra le due cose. */
static const Formula *decided(const char *formula, Verdict *verdict) {
    proof_reset();
    arena_used = arena_keep;
    atom_count = base_poly_count = atan_count = 0;
    failure = NULL;
    env_count = 0;
    map_count = statement_parameter_count = 0;
    syntax_error = NULL;
    cursor = formula;
    next_token();
    const Formula *thesis = logic_statement();
    *verdict = (Verdict){UNSUPPORTED, 0, 0, 0, NULL, NULL, NULL, "con le regole sui quantificatori", NULL, NULL};
    decide_by_signs(thesis, verdict);
    sign_certify(thesis, verdict);
    return thesis;
}

/* Ritorna 1 se, dopo explain_signs() su thesis e *verdict, la spiegazione
 * non c'è e il contatore dei passi non giustificati è salito di uno. */
static int refused(const Formula *thesis, const Verdict *verdict) {
    int before = unjustified_steps;
    explain_signs(thesis, verdict);
    int caught = explanation.steps == 0 && unjustified_steps == before + 1;
    unjustified_steps = before;
    return caught;
}

/* Controllo 2: la tabella alterata prima della spiegazione. */
static void check_liars(void) {
    const char *fraction = "\\{x \\in \\mathbb{R} \\mid \\frac{x - 2}{x + 3} \\ge x\\}", *square = "\\{x \\in \\mathbb{R} \\mid x^{2} < 2\\}";
    Verdict verdict;

    /* La tabella com'è: spiegata, e il contatore fermo. */
    const Formula *thesis = decided(fraction, &verdict);
    int before = unjustified_steps;
    check(verdict.outcome == SOLVED && strcmp(verdict.detail, "x < -3") == 0, "the helper decides the statement as the engine does");
    explain_signs(thesis, &verdict);
    check(explanation.steps > 0 && unjustified_steps == before, "the table as the engine left it is explained");

    /* Una riga con un segno rovesciato: quello del primo polinomio che,
     * rovesciato, cambia ciò che la tabella dice per x < -3. La
     * sostituzione di x = -5 dice un'altra cosa. (Rovesciarli tutti non
     * cambierebbe nulla: il segno di una frazione è il prodotto di due.) */
    thesis = decided(fraction, &verdict);
    int was = sign_truth(thesis->a, sign_row[0].sign), turned = 0;
    for (int i = 0; i < sign_polys && !turned; i++) {
        sign_row[0].sign[i] = -sign_row[0].sign[i];
        turned = sign_truth(thesis->a, sign_row[0].sign) != was;
        if (!turned) sign_row[0].sign[i] = -sign_row[0].sign[i];
    }
    check(turned, "reversing the sign of one polynomial changes what the first row says");
    check(refused(thesis, &verdict), "a table with a wrong sign in one row is not explained, and the step is counted");

    /* Una risposta che non è quella dell'ultima riga della tabella. */
    thesis = decided(fraction, &verdict);
    verdict.detail = "\\nexists x \\in \\mathbb{R}";
    check(refused(thesis, &verdict), "an answer that the table does not give is not explained, and the step is counted");

    /* Un verdetto rovesciato: "per ogni x, x^2 + 1 > 0" dato per falso. */
    thesis = decided("\\forall x \\in \\mathbb{R}\\ (x^{2} + 1 > 0)", &verdict);
    check(verdict.outcome == PROVED, "the helper proves a true statement");
    verdict.outcome = REFUTED;
    check(refused(thesis, &verdict), "a verdict that the table does not give is not explained, and the step is counted");

    /* Una radice nell'intervallo dell'altra: -sqrt(2) dichiarata tra 1 e
     * 2. La scrittura con i radicali, riletta, non sta lì. */
    thesis = decided(square, &verdict);
    check(verdict.outcome == SOLVED && sign_points_count == 2 && !sign_point[0].exact, "the helper finds the two irrational points");
    sign_point[0].low = (Wide){1, 1};
    sign_point[0].high = (Wide){2, 1};
    check(refused(thesis, &verdict), "a root written with the wrong sign before the radical is not explained, and the step is counted");

    /* E la stessa tabella, non alterata, viene spiegata. */
    thesis = decided(square, &verdict);
    before = unjustified_steps;
    explain_signs(thesis, &verdict);
    check(explanation.steps > 0 && unjustified_steps == before, "the same table, untouched, is explained");
}

/* Una copia di ciò che un verdetto dice, da confrontare dopo che i suoi
 * testi non valgono più. */
typedef struct {
    Outcome outcome;
    char *text[5];  /* detail, proof, certificate, reason, note; NULL se assente */
} Kept;

/* Ritorna una copia di text, o NULL se text è NULL. */
static char *copy_of(const char *text) {
    char *copy = text ? malloc(strlen(text) + 1) : NULL;
    if (copy) strcpy(copy, text);
    return copy;
}

/* Ritorna la copia del verdetto verdict. */
static Kept keep(const Verdict *verdict) {
    return (Kept){verdict->outcome, {copy_of(verdict->detail), copy_of(verdict->proof), copy_of(verdict->certificate), copy_of(verdict->reason), copy_of(verdict->note)}};
}

/* Ritorna 1 se il verdetto verdict dice ciò che dice la copia kept, che
 * viene liberata. */
static int kept_same(Kept kept, const Verdict *verdict) {
    const char *now[5] = {verdict->detail, verdict->proof, verdict->certificate, verdict->reason, verdict->note};
    int same = kept.outcome == verdict->outcome;
    for (int i = 0; i < 5; i++) {
        if ((kept.text[i] == NULL) != (now[i] == NULL) || (now[i] && strcmp(kept.text[i], now[i]) != 0)) same = 0;
        free(kept.text[i]);
    }
    return same;
}

/* Controllo 3 su un file: ogni enunciato di path ha lo stesso verdetto con
 * le spiegazioni spente e accese. Aggiunge a *explained quanti ne sono
 * stati spiegati. */
static void check_same_verdicts(const char *path, int *explained) {
    char summary[512];
    char *text = read_file(path);
    const char *position = text, *item, *end;
    int total = 0, same = 0;
    while (text && next_item(&position, &item, &end)) {
        explanations_wanted = 0;
        Verdict plain = prove_item(item, end);
        Kept kept = keep(&plain);
        explanations_wanted = 1;
        Verdict again = prove_item(item, end);
        total++;
        *explained += explanation.steps > 0;
        if (kept_same(kept, &again)) same++;
        else printf("  statement %d of %s has a different verdict when explanations are wanted\n", total, path);
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d verdicts are the same with and without explanations, in %s", same, total, path);
    check(total > 0 && same == total, summary);
}

/* Controlli 4 e 5: ciò che lo strato non spiega, e la spiegazione che non
 * resta attaccata all'enunciato successivo. */
static void check_unexplained(void) {
    static const char *const outside[] = {
        "\\{x \\in \\mathbb{R} \\mid \\left|x\\right| = 3\\}",
        "\\{x \\in \\mathbb{R} \\mid \\sqrt{x + 3} > x\\}",
        "\\{x \\in \\mathbb{R} \\mid 2^{x} > 8\\}",
        "\\{x \\in \\mathbb{R} \\mid \\sin x > \\frac{1}{2}\\}",
        "\\{x \\in \\mathbb{R} \\mid x^{3} - x - 1 > 0\\}",
        "\\lnot \\forall x \\in \\mathbb{R}\\ (x^{2} > 0)",
        "\\forall a \\in \\mathbb{R}\\ \\{x \\in \\mathbb{R} \\mid a x > 1\\}",

        /* Un determinante con una lettera (32g): la tabella dei segni decide
         * l'enunciato che resta dopo lo sviluppo, e lo svolgimento
         * spiegherebbe quello al posto di ciò che è scritto. */
        "\\{h \\in \\mathbb{R} \\mid \\det \\begin{pmatrix} h & 3 \\\\ 3 & 1 \\end{pmatrix} = 0\\}",
        "A = \\begin{pmatrix} 2 & 1 \\\\ 1 & 2 \\end{pmatrix} \\implies \\{s \\in \\mathbb{R} \\mid \\det(A - sI) = 0\\}",
    };
    const char *inside = "\\{x \\in \\mathbb{R} \\mid x^{2} - 5x + 6 < 0\\}";
    explanations_wanted = 1;
    for (size_t i = 0; i < sizeof outside / sizeof *outside; i++) {
        /* Prima un enunciato che si spiega: così, se la spiegazione
         * restasse attaccata, qui si vedrebbe. */
        Verdict verdict = prove(inside, 'n', 0, 0);
        check(verdict.outcome == SOLVED && explanation.steps > 0, "a rational inequality is explained");
        verdict = prove(outside[i], 'n', 0, 0);
        if (verdict.outcome != SOLVED && verdict.outcome != PROVED) printf("  statement outside the layer number %d is not decided\n", (int)i + 1);
        check((verdict.outcome == SOLVED || verdict.outcome == PROVED) && verdict.proof && explanation.steps == 0,
              "a statement outside the layer keeps its verdict and its proof, without an explanation");
    }

    /* Un enunciato di un'altra famiglia, che nessuno spiega: sugli insiemi. */
    prove(inside, 'n', 0, 0);
    Verdict sets = prove("S \\cup T = T \\cup S", 'n', 0, 0);
    check(sets.outcome == PROVED && explanation.steps == 0, "the explanation of a statement does not stay attached to the next one");
}

int main(void) {
    static const char *const files[] = {
        "tests/equations_inequalities/solution_sets.tex",
        "tests/equations_inequalities/sign_table.tex",
        "tests/equations_inequalities/book_inequalities.tex",
        "tests/equations_inequalities/false_inequalities.tex",
        "tests/equations_inequalities/book_inequalities_with_radicals.tex",
        "tests/equations_inequalities/solution_sets_with_parameter.tex",
        "tests/exponentials_logarithms/solution_sets_exponentials_logarithms.tex",
        "tests/trigonometry/solution_sets_trigonometry.tex",
        "tests/derivatives/study_requests.tex",
        "tests/derivatives/derivative_requests.tex",
        "tests/derivatives/book_derivatives.tex",
        "tests/sums_products_inequalities/induction_inequalities.tex",
        "tests/sums_products_inequalities/simple_inductions.tex",
        "tests/sums_products_inequalities/finite_sums_and_products.tex",
        "tests/logs_trig_powers_roots/logs_trig_powers_roots_inductions.tex",
        "tests/integers/divisibility.tex",
    };
    int explained = 0;
    engine_init();
    check_controls();
    check_liars();
    for (size_t i = 0; i < sizeof files / sizeof *files; i++) check_same_verdicts(files[i], &explained);
    check(explained >= 200, "the files of rational equations and inequalities, of derivatives and of inductions have their explanations");
    check_unexplained();
    check(unjustified_steps == 0, "no recorded step failed its independent check, apart from the altered ones");
    printf("%s: %d of %d checks on the explanation layer passed\n", failures ? "FAIL" : "PASS", checks - failures, checks);
    return failures ? 1 : 0;
}
