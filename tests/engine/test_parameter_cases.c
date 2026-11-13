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
 *     valore critico senza scrittura, riceve "non trattato" con il suo motivo;
 *   - il ricontrollo della proiezione (18t-h): il determinante di Bareiss e
 *     il valore omogeneo su conti noti; poi lo stato lasciato da un enunciato
 *     deciso, che il ricontrollo conferma, guastato un pezzo per volta come
 *     farebbe un motore sbagliato (un coefficiente cambiato, un psc_k
 *     dichiarato nullo o non nullo a torto, un valore critico tolto, la radice
 *     sbagliata, un intervallo senza la sua radice, un valore scelto fuori
 *     dalla sua cella): ogni guasto deve essere visto, e un conto che esce
 *     dai 128 bit non deve confermare nulla.
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

/* Lo stato che 18t-h ricontrolla, salvato da state_save() e rimesso da
 * state_restore(): i controlli sui guasti lo rovinano, uno alla volta. */
static ProjectionItem saved_item[MAX_PROJECTION];
static ParamPoint saved_point[MAX_PARAM_POINTS];
static ParamCell saved_cell[2 * MAX_PARAM_POINTS + 1];
static int saved_items, saved_points, saved_cells;

/* Copia lo stato di 18t-h nelle variabili saved_. */
static void state_save(void) {
    memcpy(saved_item, param_item, sizeof param_item);
    memcpy(saved_point, param_point, sizeof param_point);
    memcpy(saved_cell, param_cell, sizeof param_cell);
    saved_items = param_items;
    saved_points = param_points;
    saved_cells = param_cells;
}

/* Rimette lo stato di 18t-h com'era all'ultimo state_save(). */
static void state_restore(void) {
    memcpy(param_item, saved_item, sizeof param_item);
    memcpy(param_point, saved_point, sizeof param_point);
    memcpy(param_cell, saved_cell, sizeof param_cell);
    param_items = saved_items;
    param_points = saved_points;
    param_cells = saved_cells;
}

/* Ritorna il posto in param_item[] del polinomio della proiezione con questa
 * provenienza (come in ProjectionItem), -1 se non è registrato. */
static int item_of(int kind, int first, int second, int k) {
    for (int i = 0; i < param_items; i++)
        if (param_item[i].kind == kind && param_item[i].first == first && param_item[i].second == second && param_item[i].k == k) return i;
    return -1;
}

/* Riempie le prime size righe di m con la matrice di Vandermonde dei nodi
 * node[0..size-1]: la riga r è 1, node[r], node[r]^2, ... Il suo determinante
 * è il prodotto delle differenze node[j] - node[i] con i < j. */
static void vandermonde(__int128 m[MAX_MINOR][MAX_MINOR], const long long *node, int size) {
    for (int r = 0; r < size; r++) {
        __int128 power = 1;
        for (int c = 0; c < size; c++, power *= node[r]) m[r][c] = power;
    }
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

    /* 18t-h. Il determinante di Bareiss su matrici note: Vandermonde dei
     * nodi 0, 1, 2, 3, 5, che vale (1 2 3 5)(1 2 4)(1 3)(2) = 1440; un perno
     * nullo, che va scambiato; una matrice singolare; numeri che escono dai
     * 128 bit, e il conto lo deve dire. */
    __int128 matrix[MAX_MINOR][MAX_MINOR];
    const long long nodes[5] = {0, 1, 2, 3, 5};
    vandermonde(matrix, nodes, 5);
    check(bareiss(matrix, 5) == 1440, "the Vandermonde determinant of 0, 1, 2, 3, 5 is 1440");
    matrix[0][0] = 0, matrix[0][1] = 2, matrix[1][0] = 3, matrix[1][1] = 5;
    check(bareiss(matrix, 2) == -6, "a zero pivot is swapped with the row below it, and the sign changes");
    const long long singular[3][3] = {{1, 2, 3}, {2, 4, 6}, {1, 0, 1}};
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 3; c++) matrix[r][c] = singular[r][c];
    check(bareiss(matrix, 3) == 0, "a matrix with two proportional rows has determinant zero");
    matrix[0][0] = matrix[1][1] = (__int128)1 << 70, matrix[0][1] = matrix[1][0] = 1;
    failure = NULL;
    bareiss(matrix, 2);
    check(failure == SIGN_LIMIT, "a determinant beyond 128 bits is refused, not computed wrong");
    failure = NULL;

    /* Il valore omogeneo: con p = 1 + 2a + 3a^2, 2^2 p(1/2) = 11 e
     * 2^3 p(1/2) = 22. */
    Uni p = {2, {1, 2, 3}};
    check(uni_homogeneous(&p, 2, (Frac){1, 2}) == 11 && uni_homogeneous(&p, 3, (Frac){1, 2}) == 22, "q^d p(n/q) is computed without fractions");

    /* Lo stato di una richiesta risolta: il discriminante di x^2 - 2kx + 7,
     * proporzionale a k^2 - 7, con i valori critici irrazionali -sqrt(7) e
     * sqrt(7). Il ricontrollo lo conferma; ogni guasto lo deve fermare. */
    verdict = prove("\\{k \\in \\mathbb{R} \\mid \\forall x \\in \\mathbb{R}\\ (x^{2} - 2kx + 7 > 0)\\}", 'n', 0, 0);
    int k = atom_var('k'), lead = item_of(1, 0, -1, 0), discriminant = item_of(2, 0, -1, 0);
    check(verdict.outcome == SOLVED && projection_sound(k) == 1, "the projection of x^2 - 2kx + 7 passes its independent check");
    check(lead >= 0 && discriminant >= 0 && param_item[discriminant].value.degree == 2, "its leading coefficient and its discriminant, of degree two in k, are recorded");
    state_save();
    param_item[discriminant].value.c[0] += 1;
    check(projection_sound(k) == 0, "a discriminant with a wrong coefficient is caught");
    state_restore();
    param_item[discriminant].value.degree = -1;
    check(projection_sound(k) == 0, "a discriminant recorded as the zero polynomial is caught");
    state_restore();
    uni_negate(&param_item[discriminant].value);
    check(projection_sound(k) == 0, "a discriminant recorded with the opposite sign is caught");
    state_restore();
    param_item[discriminant].k = 1;
    check(projection_sound(k) == 0, "a discriminant recorded under another k counts as missing, and is caught");
    state_restore();
    param_points = 1, param_cells = 3;
    check(projection_sound(k) == 0, "a critical value left out of the cells is caught");
    state_restore();
    param_point[0].smaller = 0;
    check(projection_sound(k) == 0, "the wrong root of the quadratic at an irrational critical value is caught");
    state_restore();
    param_point[0].at.low = (Wide){-2, 1}, param_point[0].at.high = (Wide){-19, 10};
    check(projection_sound(k) == 0, "an isolating interval that does not contain its root is caught");
    state_restore();
    param_cell[2].sample = (Wide){3, 1};
    check(projection_sound(k) == 0, "a value chosen for a cell outside that cell is caught");
    state_restore();
    /* Coefficienti di 2^126, con il segno del termine noto registrato: nel
     * primo punto, k = 0, il rapporto torna, e già in k = 1 la somma esce dai
     * 128 bit. */
    __int128 huge = param_item[discriminant].value.c[0] < 0 ? -((__int128)1 << 126) : (__int128)1 << 126;
    for (int i = 0; i <= 2; i++) param_item[discriminant].value.c[i] = huge;
    check(projection_sound(k) == -1, "a recheck that runs beyond 128 bits does not confirm anything");
    int steps = unjustified_steps;
    Verdict unused = {UNSUPPORTED, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL};
    check(!projection_confirmed(k, &unused) && unused.outcome == NOT_PROVED && strstr(unused.reason, "non riesco a ricontrollare la proiezione") && unjustified_steps == steps,
          "then the verdict is not proved, says why, and does not count as an error of the engine");
    state_restore();
    param_item[discriminant].value.c[0] += 1;
    check(!projection_confirmed(k, &unused) && unused.outcome == NOT_PROVED && unjustified_steps == steps + 1, "a projection that does not agree is an error of the engine, and is counted");
    unjustified_steps = steps;
    state_restore();
    check(projection_sound(k) == 1, "restored, the projection passes its check again");

    /* Una coppia: x - a e x - 2a - 30 hanno una radice in comune solo per
     * a = -30, fuori dalla griglia di 18t-g. Il loro risultante è registrato
     * e il valore critico è tra le celle; tolto, il ricontrollo se ne
     * accorge. */
    verdict = prove("\\forall a \\in \\mathbb{R}\\ \\{x \\in \\mathbb{R} \\mid \\frac{x - a}{x - 2a - 30} \\le 0\\}", 'n', 0, 0);
    int a = atom_var('a'), pair = item_of(3, 0, 1, 0);
    check(verdict.outcome == SOLVED && pair >= 0 && param_item[pair].value.degree == 1 && param_points == 1 && param_point[0].at.exact &&
              wide_cmp(param_point[0].at.low, (Wide){-30, 1}) == 0 && projection_sound(a) == 1,
          "the resultant of x - a and x - 2a - 30 gives the critical value -30, and the projection passes its check");
    state_save();
    param_points = 0, param_cells = 1;
    check(projection_sound(a) == 0, "a critical value far beyond the grid, left out, is caught");
    state_restore();

    /* Un psc_k che è il polinomio nullo: f = (x - 2a - 30)(x^2 + 1) si
     * annulla in x = 2a + 30 per ogni a, quindi il risultante di f e di
     * x - 2a - 30 è zero, ed è registrato così. Dichiararlo non nullo è un
     * errore come il contrario. */
    verdict = prove("\\forall a \\in \\mathbb{R}\\ \\forall x \\in \\mathbb{R}\\ (x^{3} - 2ax^{2} - 30x^{2} + x - 2a - 30 > 0 \\iff x > 2a + 30)", 'n', 0, 0);
    pair = item_of(3, 0, 1, 0);
    check(verdict.outcome == PROVED && pair >= 0 && param_item[pair].value.degree < 0 && projection_sound(a) == 1,
          "the resultant of (x - 2a - 30)(x^2 + 1) and x - 2a - 30 is recorded as the zero polynomial, and the projection passes its check");
    state_save();
    param_item[pair].value = (Uni){0, {1}};
    check(projection_sound(a) == 0, "a psc that is the zero polynomial, recorded as a number, is caught");
    state_restore();

    /* Quarto grado: il discriminante di x^4 - 2ax^2 + 2a + 120, che ha i
     * valori critici -60 e 12, è un determinante di sette righe. */
    verdict = prove("\\{a \\in \\mathbb{R} \\mid \\forall x \\in \\mathbb{R}\\ (x^{4} - 2ax^{2} + 2a + 120 > 0)\\}", 'n', 0, 0);
    discriminant = item_of(2, 0, -1, 0);
    check(verdict.outcome == SOLVED && verdict.detail && strcmp(verdict.detail, "-60 < a < 12") == 0 && discriminant >= 0 && projection_sound(a) == 1,
          "the values of a for which x^4 - 2ax^2 + 2a + 120 > 0 for every x are -60 < a < 12, and the projection passes its check");
    state_save();
    param_item[discriminant].value.degree = -1;
    check(projection_sound(a) == 0, "a seven-row discriminant recorded as the zero polynomial is caught");
    state_restore();
    check(unjustified_steps == 0, "no case on a parameter was contradicted by its independent checks");

    printf("%s: %d of %d parameter checks passed\n", failures ? "FAIL" : "PASS", checks - failures, checks);
    return failures ? 1 : 0;
}
