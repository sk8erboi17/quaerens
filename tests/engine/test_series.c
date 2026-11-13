/* Test del motore: l'aritmetica delle serie troncate del capitolo 29 di
 * main.c, che calcola i limiti.
 *
 * Ogni controllo calcola lo sviluppo di un'espressione vicino a un punto,
 * dal lato destro (x = a + t, oppure x = 1/t a +\infty), e confronta i
 * coefficienti esatti con quelli delle serie di Taylor dei libri, scritti
 * qui a mano: e^t = 1 + t + t^2/2 + ..., (1 + t)^{1/2} = 1 + t/2 - t^2/8 +
 * t^3/16 - ..., ln(1 + t) = t - t^2/2 + t^3/3 - ... . Poi il resto: uno
 * sviluppo troncato non deve mai dirsi esatto, e una funzione piatta come
 * e^{-1/t} resta piatta. Infine i casi in cui la risposta giusta è "non si
 * sa" o "non ha valori": l'algebra dei limiti davanti a +\infty - \infty, un
 * divisore identicamente nullo, un logaritmo di numeri negativi, esponenti
 * troppo grandi.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_series \
 *         tests/engine/test_series.c && /tmp/test_series
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

/* Prepara un limite della variabile x: al punto razionale point da destra,
 * oppure a +\infty se infinity è 1, con la precisione goal. */
static void at(Frac point, int infinity, int goal) {
    arena_used = arena_keep;
    atom_count = base_poly_count = atan_count = 0;
    failure = NULL;
    lim_x = 'x';
    lim_infinity = infinity;
    lim_side = 1;
    lim_goal = goal;
    lim_ell = atom_var(VAR_LOG_SCALE);
    lim_point = rat_frac(point);
}

/* Il valore dell'espressione text vicino al punto preparato da at(). */
static Value value_of(const char *text) {
    Node *tree = parse_expression(text);
    if (syntax_error) return value_failed(V_UNKNOWN, syntax_error);
    return limit_eval(tree);
}

/* Ritorna 1 se il coefficiente di t^{e/den} della serie s è il razionale q
 * (e se s quel coefficiente lo conosce: e < s.end). */
static int coefficient_is(Series s, int e, Frac q) {
    Num value;
    Rat c = e >= s.low && e < s.low + s.count ? s.coef[e - s.low] : rat_int(0);
    if (e >= s.end) return 0;
    if (c.num.count == 0) return q.num == 0;
    return rat_number(c, &value) && value.im.num == 0 && frac_cmp(value.re, q) == 0;
}

/* Ritorna 1 se v è una serie con il denominatore den, che conosce almeno i
 * coefficienti di esponente da 0 a count - 1 (in unità 1/den), e questi sono
 * expected[]. */
static int series_is(Value v, int den, const Frac *expected, int count) {
    if (v.kind != V_SERIES || v.series.den != den) return 0;
    for (int e = 0; e < count; e++)
        if (!coefficient_is(v.series, e, expected[e])) return 0;
    return 1;
}

int main(void) {
    engine_init();
    Frac zero = {0, 1};

    /* Le serie di Taylor dei libri, in t -> 0^{+}. */
    at(zero, 0, 8);
    Frac exp_series[] = {{1, 1}, {1, 1}, {1, 2}, {1, 6}, {1, 24}, {1, 120}};
    check(series_is(value_of("e^{x}"), 1, exp_series, 6), "e^t = 1 + t + t^2/2 + t^3/6 + ...");
    Frac root_series[] = {{1, 1}, {1, 2}, {-1, 8}, {1, 16}, {-5, 128}};
    check(series_is(value_of("\\sqrt{1 + x}"), 1, root_series, 5), "(1 + t)^{1/2} = 1 + t/2 - t^2/8 + t^3/16 - 5t^4/128 + ...");
    Frac log_series[] = {{0, 1}, {1, 1}, {-1, 2}, {1, 3}, {-1, 4}};
    check(series_is(value_of("\\ln(1 + x)"), 1, log_series, 5), "ln(1 + t) = t - t^2/2 + t^3/3 - t^4/4 + ...");
    Frac geometric[] = {{1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}};
    check(series_is(value_of("\\frac{1}{1 - x}"), 1, geometric, 6), "1/(1 - t) = 1 + t + t^2 + ...");
    Frac sinc[] = {{1, 1}, {0, 1}, {-1, 6}, {0, 1}, {1, 120}};
    check(series_is(value_of("\\frac{\\sin x}{x}"), 1, sinc, 5), "sin(t)/t = 1 - t^2/6 + t^4/120 + ...");
    Frac tangent[] = {{0, 1}, {1, 1}, {0, 1}, {1, 3}, {0, 1}, {2, 15}};
    check(series_is(value_of("\\tan x"), 1, tangent, 6), "tan t = t + t^3/3 + 2t^5/15 + ...");
    Frac arcsine[] = {{0, 1}, {1, 1}, {0, 1}, {1, 6}, {0, 1}, {3, 40}};
    check(series_is(value_of("\\arcsin x"), 1, arcsine, 6), "arcsin t = t + t^3/6 + 3t^5/40 + ..., through its definition with arctan");
    Frac one[] = {{1, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}};
    check(series_is(value_of("e^{x} e^{-x}"), 1, one, 7), "e^t e^{-t} = 1, with every known coefficient after the first zero");
    check(series_is(value_of("\\cosh^{2} x - \\sinh^{2} x"), 1, one, 7), "cosh^2 t - sinh^2 t = 1");

    /* Le radici: esponenti frazionari (serie di Puiseux). */
    Value root = value_of("\\sqrt{x}");
    check(root.kind == V_SERIES && root.series.den == 2 && root.series.low == 1 && root.series.count == 1 && root.series.exact,
          "sqrt(t) is exactly t^{1/2}");
    Value cube = (lim_side = -1, value_of("\\sqrt[3]{x}"));
    Frac minus_one = {-1, 1};
    check(cube.kind == V_SERIES && cube.series.den == 3 && cube.series.low == 1 && coefficient_is(cube.series, 1, minus_one),
          "a cube root from the left: x = -t gives -t^{1/3}");
    Value even = value_of("\\sqrt{x}");
    check(even.kind == V_UNDEFINED, "a square root from the left of 0 has no value");
    lim_side = 1;

    /* A +\infty, x = 1/t. Antiregressione: la costante di uno sviluppo va
     * scritta nel denominatore degli esponenti della serie, altrimenti
     * 1/(2 t^{-5/2} - 1) diventava t^5/2 invece di t^{5/2}/2. */
    at(zero, 1, 8);
    Value high = value_of("\\frac{1}{2x^{5/2} - 1}");
    Frac half = {1, 2};
    check(high.kind == V_SERIES && high.series.den == 2 && high.series.low == 5 && coefficient_is(high.series, 5, half),
          "1/(2x^{5/2} - 1) at +infinity is t^{5/2}/2 + ...");
    Value ratio = value_of("\\frac{x^{5/2}}{2x^{5/2} - 1}");
    check(ratio.kind == V_SERIES && coefficient_is(ratio.series, 0, half), "x^{5/2}/(2x^{5/2} - 1) at +infinity tends to 1/2");

    /* La scala dei logaritmi: x ln x da destra di 0 è -L t. */
    at(zero, 0, 8);
    Value xlogx = value_of("x \\ln x");
    Lead lead = xlogx.kind == V_SERIES && xlogx.series.count ? coefficient_lead(xlogx.series.coef[0]) : (Lead){0};
    check(xlogx.kind == V_SERIES && xlogx.series.low == 1 && lead.known && !lead.zero && lead.side < 0 && frac_cmp(lead.degree, frac_int(1)) == 0,
          "x ln x is -L t, with L = ln(1/t)");
    Value limit = value_limit(xlogx);
    check(limit.kind == V_LIMIT && !limit.infinite && limit.value.num.count == 0, "and its limit is 0");
    Rat ell = rat_atom(lim_ell), top = rat_add(rat_add(rat_mul(rat_int(2), rat_mul(ell, ell)), ell, 1), rat_int(1), 0);
    lead = coefficient_lead(rat_div(top, rat_add(ell, rat_int(3), 0)));
    Num number;
    check(lead.known && lead.side > 0 && frac_cmp(lead.degree, frac_int(1)) == 0 && rat_number(lead.value, &number) && frac_cmp(number.re, frac_int(2)) == 0,
          "(2L^2 - L + 1)/(L + 3) behaves as 2L");

    /* Le costanti in forma unica. */
    Rat value, other;
    lead = coefficient_lead(rat_add(logarithm(rat_int(4)), rat_mul(rat_int(2), logarithm(rat_int(2))), 1));
    check(lead.known && lead.zero, "ln 4 - 2 ln 2 is zero, by the exact test");
    check(constant_exp(rat_mul(rat_frac((Frac){3, 2}), logarithm(rat_int(2))), &value) &&
          rat_add(value, fractional_power(rat_int(8), (Frac){1, 2}, 0), 1).num.count == 0, "e^{(3/2) ln 2} is the square root of 8");
    Rat root_e = (Rat){poly_term((Term){num_int(1), mono_atom(atom_var('e'), (Frac){1, 2})}), poly_const(num_int(1))};
    check(constant_ln(rat_mul(root_e, fractional_power(rat_int(2), (Frac){1, 2}, 0)), &value) &&
          rat_add(value, rat_mul(rat_frac(half), rat_add(rat_int(1), logarithm(rat_int(2)), 0)), 1).num.count == 0,
          "ln(e^{1/2} 2^{1/2}) = (1 + ln 2)/2");
    check(constant_power(rat_pow_int(rat_atom(atom_var('e')), 7), (Frac){-1, 2}, &other) && constant_exp(rat_frac((Frac){-7, 2}), &value) &&
          rat_add(value, other, 1).num.count == 0, "(e^7)^{-1/2} = e^{-7/2}");

    /* Il resto: uno sviluppo troncato non è esatto, una funzione piatta
     * resta piatta, e sommarle una costante non la rende esatta. */
    Value truncated = value_of("e^{x}");
    check(truncated.kind == V_SERIES && !truncated.series.exact && truncated.series.end >= 8, "the expansion of e^t is not exact, and reaches t^8");
    Value flat = value_of("e^{-\\frac{1}{x}}");
    check(flat.kind == V_SERIES && flat.series.count == 0 && flat.series.end == SERIES_FLAT && !flat.series.exact && flat.positive,
          "e^{-1/t} is smaller than every power of t, and positive");
    Value still = value_of("\\frac{e^{-\\frac{1}{x}}}{x^{5}}");
    check(still.kind == V_SERIES && still.series.count == 0 && still.series.end == SERIES_FLAT, "and so is e^{-1/t} / t^5");
    Value shifted = value_of("\\cos\\left(e^{-\\frac{1}{x}}\\right) - 1");
    check(shifted.kind == V_SERIES && !shifted.series.exact && value_binary(N_DIV, value_of("1"), shifted).kind != V_UNDEFINED,
          "cos(e^{-1/t}) - 1 is not exactly zero: dividing by it is not a division by zero");
    Value big = value_of("e^{\\frac{1}{x}}");
    check(big.kind == V_LIMIT && big.infinite > 0, "e^{1/t} has no expansion, only its limit +infinity");

    /* Ciò che non ha valori, e ciò che non si sa. */
    check(value_of("\\frac{1}{x - x}").kind == V_UNDEFINED, "a division by a function that is exactly zero has no value");
    lim_side = -1;
    check(value_of("\\ln x").kind == V_UNDEFINED, "the logarithm on the left of 0 has no value");
    lim_side = 1;
    check(value_of("e^{\\frac{1}{x}} - e^{\\frac{2}{x}}").kind == V_UNKNOWN, "+infinity minus +infinity is not decided by the algebra of limits");
    check(value_of("\\frac{e^{\\frac{1}{x}}}{e^{\\frac{2}{x}}}").kind == V_UNKNOWN, "infinity over infinity is not decided either");
    check(value_limit(value_of("\\sin\\frac{1}{x}")).kind == V_UNKNOWN, "sin(1/t) is only bounded: no limit");
    check(value_limit(value_of("x \\sin\\frac{1}{x}")).kind == V_LIMIT, "but t sin(1/t) tends to 0");
    failure = NULL;
    Value huge = value_of("\\left(\\left(x^{512}\\right)^{512}\\right)^{512}");
    check(huge.kind == V_UNKNOWN || failure, "exponents beyond the bound make the computation fail, not wrap around");
    failure = NULL;

    char summary[96];
    snprintf(summary, sizeof summary, "%d of %d series checks passed", checks - failures, checks);
    printf("%s: %s\n", failures ? "FAIL" : "PASS", summary);
    return failures ? 1 : 0;
}
