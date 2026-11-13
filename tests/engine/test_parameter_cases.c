/* Test del motore: i pezzi della regola 18t di main.c, i casi su un parametro.
 *
 * La regola decide un enunciato con un parametro e una variabile dividendo la
 * retta del parametro in celle, e che le celle siano quelle giuste dipende da
 * pochi conti esatti. Qui si provano quelli, uno per uno, con risultati noti
 * dall'algebra e non dall'uscita del programma:
 *
 *   - il coefficiente principale di un sottorisultante, psc_k: il risultante
 *     di due polinomi di primo grado, il discriminante del trinomio e della
 *     cubica, e la proprietà per cui serve, cioè che il primo k con psc_k
 *     diverso da zero è il grado del massimo comun divisore;
 *   - la radice quadrata esatta di un polinomio;
 *   - la sostituzione di un numero al parametro, e la semplificazione dei
 *     confronti che così diventano confronti tra numeri;
 *   - la cella in cui cade un numero, anche accanto a un valore critico
 *     irrazionale;
 *   - i limiti dichiarati: un enunciato oltre il grado trattato, o con un
 *     valore critico senza scrittura, riceve "non trattato" con il suo motivo.
 *
 * Gli enunciati interi, con i risultati dei libri e le loro varianti
 * sbagliate, sono in tests/equations_inequalities.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_parameter_cases \
 *         tests/engine/test_parameter_cases.c && /tmp/test_parameter_cases
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

/* Ritorna il valore di text, un polinomio scritto in LaTeX: le lettere
 * restano lettere. Serve a scrivere i polinomi di prova e i risultati attesi
 * come li scrive un libro, 4c - b^{2}. */
static Poly poly_of(const char *text) {
    env_count = 0;
    return eval(parse_expression(text)).num;
}

/* Mette in part[], che ha MAX_PARAM_DEGREE + 1 posti, i coefficienti del
 * polinomio text nella lettera x, e ne ritorna il grado. */
static int split_in_x(const char *text, Poly *part) {
    return poly_split(poly_of(text), atom_var('x'), part, MAX_PARAM_DEGREE);
}

/* Ritorna il più piccolo k per cui psc_k dei due polinomi in x, scritti in
 * LaTeX, non è il polinomio nullo; il più piccolo dei due gradi se sono
 * tutti nulli. */
static int first_subresultant(const char *first, const char *second) {
    Poly f[MAX_PARAM_DEGREE + 1], g[MAX_PARAM_DEGREE + 1];
    int m = split_in_x(first, f), n = split_in_x(second, g), k = 0;
    while (k < (m < n ? m : n) && subresultant_lead(f, m, g, n, k).count == 0) k++;
    return k;
}

/* Controlla che la radice quadrata esatta del polinomio text nella lettera a
 * sia expected; expected NULL vuol dire che text non è un quadrato, e allora
 * uni_square_root() deve dire di no, oppure dare un polinomio il cui
 * quadrato non è text: è il suo contratto, chi la chiama rimoltiplica. */
static void square_root_is(const char *text, const char *expected) {
    char message[200];
    Uni p, root, wanted;
    int shift, a = atom_var('a');
    uni_from_poly(poly_of(text), a, &p, &shift);
    int found = uni_square_root(&p, &root);
    Poly again = found ? poly_from_uni(&root, a, frac_int(1)) : poly_const(num_int(0));
    int exact = found && poly_equal(poly_mul(again, again), poly_of(text));
    if (expected) {
        uni_from_poly(poly_of(expected), a, &wanted, &shift);
        snprintf(message, sizeof message, "the square root of %s is %s", text, expected);
        check(exact && uni_equal(&root, &wanted), message);
    } else {
        snprintf(message, sizeof message, "%s is not the square of a polynomial", text);
        check(!exact, message);
    }
}

/* Ritorna quanti confronti ha la condizione f. */
static int comparisons_in(const Formula *f) {
    if (!f) return 0;
    if (f->kind == F_EQUAL || f->kind == F_DISTINCT || f->kind == F_LESS || f->kind == F_LESS_EQ) return 1;
    return comparisons_in(f->a) + comparisons_in(f->b);
}

/* Legge text come condizione in a e x, mette value al posto di a e toglie i
 * confronti tra numeri. Ritorna -1 se resta la costante falsa, -2 se resta
 * la vera, altrimenti il numero dei confronti rimasti; -3 se qualcosa non
 * riesce. */
static int reduced(const char *text, long long value) {
    Verdict unused = {UNSUPPORTED, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL};
    syntax_error = NULL;
    cursor = text;
    next_token();
    Formula *condition = logic_formula();
    if (syntax_error || token.kind != T_END) return -3;
    Formula *left = closed_reduce(formula_with(condition, 'a', node_rational((Wide){value, 1})), 'x', 'a', &unused);
    if (!left) return -3;
    if (left == &closed_constant[0] || left == &closed_constant[1]) return left == &closed_constant[1] ? -2 : -1;
    return formula_uses_letter(left, 'a') ? -3 : comparisons_in(left);
}

/* Controlla che l'enunciato statement riceva l'esito expected e, se reason
 * non è NULL, proprio quel motivo. */
static void answered(const char *statement, Outcome expected, const char *reason, const char *what) {
    Verdict verdict = prove(statement, 'n', 0, 0);
    check(verdict.outcome == expected && (!reason || (verdict.reason && strcmp(verdict.reason, reason) == 0)), what);
}

int main(void) {
    Poly f[MAX_PARAM_DEGREE + 1], g[MAX_PARAM_DEGREE + 1];
    engine_init();
    arena_used = arena_keep;
    atom_count = base_poly_count = atan_count = 0;
    failure = NULL;

    /* psc_0 è il risultante. Di x - p e x - q è p - q; di un polinomio e
     * della sua derivata è il discriminante, a meno del segno e del
     * coefficiente direttivo: per x^2 + b x + c è 4c - b^2, per
     * x^3 + p x + q è 4p^3 + 27q^2. */
    int m = split_in_x("x - p", f), n = split_in_x("x - q", g);
    check(poly_equal(subresultant_lead(f, m, g, n, 0), poly_of("p - q")), "the resultant of x - p and x - q is p - q");
    m = split_in_x("x^{2} + bx + c", f);
    n = split_in_x("2x + b", g);
    check(poly_equal(subresultant_lead(f, m, g, n, 0), poly_of("4c - b^{2}")), "the resultant of x^2 + bx + c and its derivative is 4c - b^2");
    m = split_in_x("x^{3} + px + q", f);
    n = split_in_x("3x^{2} + p", g);
    check(poly_equal(subresultant_lead(f, m, g, n, 0), poly_of("4p^{3} + 27q^{2}")), "the resultant of x^3 + px + q and its derivative is 4p^3 + 27q^2");

    /* Due polinomi con la radice 1 in comune, qualunque siano p e q: psc_0 è
     * il polinomio nullo, e psc_1 è p - q, che si annulla solo quando hanno
     * in comune anche l'altra. */
    m = split_in_x("(x - 1)(x - p)", f);
    n = split_in_x("(x - 1)(x - q)", g);
    check(subresultant_lead(f, m, g, n, 0).count == 0, "psc_0 of two polynomials with a common root is the zero polynomial");
    check(poly_equal(subresultant_lead(f, m, g, n, 1), poly_of("p - q")), "psc_1 of (x - 1)(x - p) and (x - 1)(x - q) is p - q");

    /* La proprietà per cui psc_k serve: il primo k con psc_k diverso da zero
     * è il grado del massimo comun divisore. */
    check(first_subresultant("(x - 1)(x - 2)(x - 3)", "(x + 4)(x + 5)(x + 6)") == 0, "no common root: the first non-zero psc is psc_0");
    check(first_subresultant("(x - 1)(x - 2)(x - 3)", "(x - 3)(x + 5)(x + 6)") == 1, "one common root: the first non-zero psc is psc_1");
    check(first_subresultant("(x - 1)(x - 2)(x - 3)", "(x - 2)(x - 3)(x + 6)") == 2, "two common roots: the first non-zero psc is psc_2");
    check(first_subresultant("(x - 1)(x - 2)(x - 3)(x - 4)", "(x - 2)(x - 3)(x - 4)") == 3, "a divisor of degree three: all the psc below three are zero");
    check(first_subresultant("(x - 1)^{2}(x + 2)", "2(x - 1)(x + 2) + (x - 1)^{2}") == 1, "a double root is a common root of the polynomial and of its derivative");
    check(first_subresultant("x^{4} + ax^{2} + 1", "4x^{3} + 2ax") == 0, "with a letter in the coefficients the discriminant is not the zero polynomial");

    /* Oltre MAX_MINOR righe il determinante non si calcola, e lo si dice. */
    m = split_in_x("x^{4} + x + a", f);
    n = split_in_x("x^{4} + ax + 1", g);
    failure = NULL;
    subresultant_lead(f, m, g, n, 0);
    check(failure == NULL, "the resultant of two polynomials of degree four is computed");
    m = split_in_x("x^{4} + x + a", f);
    Poly wide[6] = {f[0], f[1], f[2], f[3], f[4], f[4]};
    subresultant_lead(wide, 5, g, n, 0);
    check(failure == PARAM_DEGREE, "a determinant with more than eight rows is refused");
    failure = NULL;

    /* La radice quadrata esatta di un polinomio. */
    square_root_is("a^{2} + 2a + 1", "a + 1");
    square_root_is("4a^{2} - 12a + 9", "2a - 3");
    square_root_is("a^{4} - 2a^{3} + 3a^{2} - 2a + 1", "a^{2} - a + 1");
    square_root_is("9", "3");
    square_root_is("a^{2} + 1", NULL);
    square_root_is("a^{2} + 2a + 5", NULL);
    square_root_is("2a^{2}", NULL);
    square_root_is("a^{3} + a", NULL);
    square_root_is("4 - a^{2}", NULL);

    /* La sostituzione: il parametro sparisce dalla copia, non dall'originale. */
    syntax_error = NULL;
    cursor = "\\forall x \\in \\mathbb{R}\\ (x^{2} - 4ax \\le a)";
    next_token();
    Formula *statement = logic_formula(), *copy = formula_with(statement, 'a', node_rational((Wide){-3, 2}));
    check(!syntax_error && !formula_uses_letter(copy, 'a') && formula_uses_letter(copy, 'x'), "after the substitution the parameter is gone and the variable is still there");
    check(formula_uses_letter(statement, 'a'), "the substitution works on a copy");

    /* Tolti i confronti tra numeri resta il solo caso del parametro che
     * vale: dei due casi di un elenco, quello giusto. */
    check(reduced("(a < 0 \\land x > 1) \\lor (a \\ge 0 \\land 2 < x < 3)", -1) == 1, "with a = -1 only the case a < 0 is left, with its one comparison");
    check(reduced("(a < 0 \\land x > 1) \\lor (a \\ge 0 \\land 2 < x < 3)", 2) == 2, "with a = 2 only the case a >= 0 is left, with its two comparisons");
    check(reduced("(a < 0 \\land x > 1) \\lor a = 5", 5) == -2, "a case that holds for every x leaves the constant true");
    check(reduced("a < 0 \\land x > 1", 3) == -1, "a case that does not apply leaves the constant false");
    check(reduced("\\lnot (a < 0) \\implies x > a", 3) == 1, "a true premise leaves the conclusion");
    check(reduced("a < 0 \\implies x > a", 3) == -2, "a false premise leaves the constant true");
    check(reduced("x > 1 \\iff a > 0", -1) == 1, "an equivalence with a false side leaves the negation of the other");
    check(reduced("\\frac{1}{a} > 0 \\lor x > 1", 0) == 1, "a comparison with no value counts as false");

    /* Le celle: per x^2 - 2kx + 7 > 0 i valori critici sono le due radici di
     * 7, e un razionale vicinissimo a una di loro cade dalla parte giusta. */
    Verdict verdict = prove("\\{k \\in \\mathbb{R} \\mid \\forall x \\in \\mathbb{R}\\ (x^{2} - 2kx + 7 > 0)\\}", 'n', 0, 0);
    check(verdict.outcome == SOLVED && param_points == 2 && param_cells == 5, "the discriminant of x^2 - 2kx + 7 gives two critical values and five cells");
    check(!param_point[0].at.exact && !param_point[1].at.exact, "both critical values are irrational");
    check(param_cell_of((Wide){-3, 1}) == 0 && param_cell_of((Wide){0, 1}) == 2 && param_cell_of((Wide){3, 1}) == 4, "-3, 0 and 3 fall in the three intervals");
    check(param_cell_of((Wide){264575131, 100000000}) == 2, "2.64575131 is below the square root of 7");
    check(param_cell_of((Wide){264575132, 100000000}) == 4, "2.64575132 is above the square root of 7");
    check(param_cell_of((Wide){-264575131, 100000000}) == 2 && param_cell_of((Wide){-264575132, 100000000}) == 0, "and the same on the negative side");
    check(verdict.proof && strstr(verdict.proof, "Cases on $k$") != NULL, "the written proof lists the cases on the parameter");

    /* I limiti dichiarati, ognuno con il suo motivo. */
    answered("\\forall a \\in \\mathbb{R}\\ \\forall x \\in \\mathbb{R}\\ (x^{5} + ax + 1 > 0 \\lor x < a)", UNSUPPORTED, PARAM_DEGREE,
             "degree five in the variable is beyond the rule, and the verdict says so");
    answered("\\{a \\in \\mathbb{R} \\mid \\exists x \\in \\mathbb{R}\\ (x^{3} + ax + 1 = 0 \\land x > 0)\\}", UNSUPPORTED, PARAM_ROOT,
             "a critical value that is a cube root is beyond the rule, and the verdict says so");
    answered("\\forall a \\in \\mathbb{R}\\ \\{x \\in \\mathbb{R} \\mid (x - a)(x - 2a)(x + 1) > 0\\}", UNSUPPORTED, PARAM_WRITE,
             "endpoints that are roots of a cubic in the variable cannot be written, and the verdict says so");
    answered("\\forall a \\in \\mathbb{R}\\ \\forall x \\in \\mathbb{R}\\ (\\left|x\\right| + a^{2} \\ge 0)", UNSUPPORTED, PARAM_SHAPE,
             "an absolute value of the variable beside a parameter is beyond the rule, and the verdict says so");
    answered("\\forall a \\in \\mathbb{R}\\ \\forall b \\in \\mathbb{R}\\ \\forall x \\in \\mathbb{R}\\ (x^{2} + a^{2} + b^{2} \\ge 0)", UNSUPPORTED, PARAM_FORM,
             "three real letters are beyond the rule, and the verdict says so");
    check(unjustified_steps == 0, "no case on a parameter was contradicted by its independent checks");

    printf("%s: %d of %d parameter checks passed\n", failures ? "FAIL" : "PASS", checks - failures, checks);
    return failures ? 1 : 0;
}
