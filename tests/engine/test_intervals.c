/* Test del motore: i numeri reali con l'errore garantito (capitolo 24 di
 * main.c).
 *
 * Ogni funzione sugli intervalli deve dare un intervallo che contiene il
 * valore vero, e abbastanza stretto da servire. I valori veri sono costanti
 * note, scritte qui con più cifre di quante un double ne tenga: il
 * compilatore le arrotonda al double più vicino, che dista dal vero meno di
 * 10^-15 volte il suo valore, mentre gli intervalli sono larghi almeno cento
 * volte tanto; per questo "il double sta dentro l'intervallo" è un controllo
 * valido. Un intervallo che non contenesse il valore vero farebbe ordinare
 * male due punti della tabella dei segni: è l'errore che questo test deve
 * impedire.
 *
 * Controlla anche i casi in cui la risposta giusta è "non so": una divisione
 * per un intervallo che contiene lo zero, il logaritmo e la radice di un
 * intervallo che tocca i numeri negativi.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_intervals \
 *         tests/engine/test_intervals.c && /tmp/test_intervals
 */

/* Il programma ha il proprio main(), in main.c: lo si include cambiando
 * nome a quel main, così il test può chiamarne le funzioni interne. */
#define main mathresearcher_main
#include "../../main.c"
#undef main

static int checks, failures;

/* Registra l'esito di un controllo; se è fallito lo stampa. */
static void check(int passed, const char *what) {
    checks++;
    if (passed) return;
    failures++;
    printf("  failed: %s\n", what);
}

/* Controlla che r contenga truth e sia largo meno di width volte il suo
 * valore assoluto (o meno di width, per un valore vicino a zero). */
static void encloses(const char *what, Range r, double truth, double width) {
    double size = truth < 0 ? -truth : truth;
    char message[160];
    snprintf(message, sizeof message, "%s: [%.17g, %.17g] must contain %.17g and be narrow", what, r.low, r.high, truth);
    check(range_known(r) && r.low < truth && truth < r.high && r.high - r.low < width * (size > 1 ? size : 1), message);
}

int main(void) {
    Range sine, cosine, zero_inside = {-1, 2}, negative = {-3, -2}, positive = {2, 3}, touching = {0, 1};
    int side = 0, undefined = 0;

    /* Le costanti. */
    encloses("ln 2", range_ln2(), 0.69314718055994530941723, 1e-11);
    encloses("pi", range_pi(), 3.14159265358979323846264, 1e-11);
    encloses("e", range_exp_at(1), 2.71828182845904523536028, 1e-11);

    /* Le funzioni, anche lontano da dove le serie convergono in fretta. */
    encloses("ln 10", range_ln(range_ratio(10, 1)), 2.30258509299404568401799, 1e-11);
    encloses("ln 1/1000", range_ln(range_ratio(1, 1000)), -6.90775527898213705205397, 1e-11);
    encloses("ln 123456789", range_ln(range_ratio(123456789, 1)), 18.6314017661680180331939, 1e-11);
    encloses("exp -20", range_exp_at(-20), 2.06115362243855782796594e-09, 1e-11);
    encloses("exp 50", range_exp_at(50), 5.18470552858707246408745e+21, 1e-11);
    encloses("exp 1/3", range_exp(range_ratio(1, 3)), 1.39561242508608952862812, 1e-11);
    encloses("sqrt 2", range_sqrt(range_ratio(2, 1)), 1.41421356237309504880168, 1e-11);
    encloses("sqrt 1/1000000", range_sqrt(range_ratio(1, 1000000)), 0.001, 1e-11);
    encloses("atan 1", range_atan(range_ratio(1, 1)), 0.78539816339744830961566, 1e-11);
    encloses("atan 1000000", range_atan(range_ratio(1000000, 1)), 1.57079532679489661923132, 1e-11);
    encloses("atan -2", range_atan(range_ratio(-2, 1)), -1.10714871779409050301706, 1e-11);
    range_sin_cos(range_ratio(1, 1), &sine, &cosine);
    encloses("sin 1", sine, 0.84147098480789650665250, 1e-11);
    encloses("cos 1", cosine, 0.54030230586813971740093, 1e-11);
    range_sin_cos(range_ratio(100, 1), &sine, &cosine);
    encloses("sin 100", sine, -0.50636564110975879365655, 1e-9);
    encloses("cos 100", cosine, 0.86231887228768393410193, 1e-9);
    range_sin_cos(range_ratio(-7, 2), &sine, &cosine);
    encloses("sin -7/2", sine, 0.35078322768961984812037, 1e-11);
    encloses("cos -7/2", cosine, -0.93645668729079633770001, 1e-11);

    /* Le quattro operazioni, su intervalli larghi. */
    Range product = range_mul(zero_inside, negative), quotient = range_div(positive, negative);
    check(product.low <= -6 && product.high >= 3, "[-1, 2] * [-3, -2] contains [-6, 3]");
    check(quotient.low <= -1.5 && quotient.high >= -2.0 / 3, "[2, 3] / [-3, -2] contains [-3/2, -2/3]");
    check(range_side(range_sub(positive, negative), &side) && side == 1, "[2, 3] - [-3, -2] is certainly positive");
    check(!range_side(zero_inside, &side), "the sign of [-1, 2] is not known");

    /* "Non so". */
    check(!range_known(range_inv(zero_inside)), "1 / [-1, 2] is not known");
    check(!range_known(range_ln(touching)), "ln [0, 1] is not known");
    check(!range_known(range_sqrt(zero_inside)), "sqrt [-1, 2] is not known");
    check(!range_known(range_add(range_none(), positive)), "an unknown operand gives an unknown result");
    check(!range_known(range_exp_at(1000)), "exp 1000 is too large to be known");

    /* Il valore di un'espressione: (x^2 + 1) / |x - 3| in x = 1/2 vale 1/2. */
    cursor = "\\frac{x^{2} + 1}{\\left|x - 3\\right|}";
    syntax_error = NULL;
    next_token();
    const Node *expression = parse_expr();
    check(syntax_error == NULL, "the test expression is read");
    encloses("an expression at x = 1/2", node_range(expression, 'x', range_ratio(1, 2), &undefined), 0.5, 1e-11);
    check(!undefined, "and it has a value there");
    cursor = "\\ln\\left(x - 3\\right)";
    next_token();
    expression = parse_expr();
    node_range(expression, 'x', range_ratio(1, 2), &undefined);
    check(undefined, "the logarithm of a negative number certainly has no value");

    printf("%s: %d of %d interval checks passed\n", failures ? "FAIL" : "PASS", checks - failures, checks);
    return failures ? 1 : 0;
}
