/* Test del motore: i fatti del dominio (11q in main.c) e il test di
 * annullamento delle uguaglianze che valgono "dove i due membri sono
 * definiti" (12d).
 *
 * Controlla il meccanismo senza riferirsi a un esercizio:
 *
 *   1. il test con i fatti e quello senza: \sqrt[3]{x} e (\sqrt[6]{x})^2
 *      sono lo stesso numero dove la radice sesta è definita, e
 *      vanishes_where_defined() lo vede; poly_vanishes() da solo no, perché
 *      la radice cubica reale di -1 c'è e la sesta no. \sqrt{x^2} = x
 *      resta non dimostrata anche con i fatti;
 *   2. da dove vengono i fatti: una radice di indice pari, una potenza a
 *      esponente non intero, un logaritmo, sì; una radice di indice dispari,
 *      che è definita per ogni base, e il logaritmo di un valore assoluto,
 *      no. E la funzione di cui si afferma la derivata, o la primitiva, anche
 *      se non è un membro dell'uguaglianza che si calcola: dove la derivata
 *      esiste la funzione è definita;
 *   3. i quattro usi (11q-1 ... 11q-4), uno per uno;
 *   4. i fatti non escono dal test: dopo vanishes_where_defined() e dopo un
 *      enunciato intero sono spenti e dimenticati;
 *   5. uguaglianze vere solo dove i due membri sono definiti, derivate e
 *      primitive, vengono dimostrate;
 *   6. uguaglianze false, che diventerebbero vere se un fatto valesse fuori
 *      dal dominio, non vengono mai dimostrate. Alcune sono false solo per
 *      x < -3, dove nessun punto di prova dei controlli con i numeri arriva
 *      (27h, 30b): lì a fermarle è solo il test di annullamento, e un fatto
 *      usato dove non vale le farebbe passare. E gli enunciati con
 *      \forall x \in \mathbb{R}, che parlano anche dei punti in cui un
 *      membro non è definito;
 *   7. un'uguaglianza vera che il motore non sa ancora decidere (il prodotto
 *      di due radici di fattori negativi) non viene dichiarata falsa;
 *   8. nessun passo smentito dai controlli indipendenti.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_domain_facts \
 *         tests/engine/test_domain_facts.c && /tmp/test_domain_facts
 */

/* Il programma ha il proprio main(), in main.c: lo si include cambiando
 * nome a quel main, così il test può chiamarne le funzioni interne. */
#define main mathresearcher_main
#include "../../main.c"
#undef main

static int checks, failures;

/* Conta un controllo e, se non è riuscito, lo stampa. */
static void check(int passed, const char *what) {
    checks++;
    if (passed) return;
    failures++;
    printf("FAIL: %s\n", what);
}

/* Come sta il test di annullamento su un'uguaglianza tra due espressioni. */
typedef struct {
    int plain;          /* 1 se L - R si riduce a zero senza i fatti */
    int where_defined;  /* 1 se si riduce a zero con i fatti (12d) */
    int facts_left;     /* fatti rimasti dopo il test: devono essere zero */
} ZeroTest;

/* Legge left e right come espressioni in x, con la memoria e le tabelle
 * ripartite da zero come all'inizio di un enunciato, e ne fa i due test.
 * Un testo che non si legge dà due "no". */
static ZeroTest zero_tests(const char *left, const char *right) {
    static char text[2][MAX_FORMULA];
    ZeroTest result = {0, 0, 0};
    arena_used = arena_keep;
    atom_count = base_poly_count = atan_count = 0;
    env_count = point_count = abs_count = 0;
    induction_var = 'n';
    failure = syntax_error = NULL;
    snprintf(text[0], sizeof text[0], "%s", left);
    snprintf(text[1], sizeof text[1], "%s", right);
    Node *lhs = parse_expression(text[0]), *rhs = parse_expression(text[1]);
    if (syntax_error || failure) { syntax_error = failure = NULL; return result; }
    Statement s = {lhs, rhs, REL_EQ};
    Rat gap = difference(&s, rat_int(0));
    result.plain = !failure && poly_vanishes(gap.num);
    failure = NULL;
    result.where_defined = vanishes_where_defined(&s, NULL, rat_int(0)) && !failure;
    result.facts_left = domain_on + domain_fact_count;
    failure = NULL;
    return result;
}

/* 1, 2, 3, 4. Il meccanismo, su uguaglianze tra due espressioni. */
static void test_mechanism(void) {
    /* 1. Con e senza fatti. */
    ZeroTest t = zero_tests("\\sqrt[3]{x}", "\\left(\\sqrt[6]{x}\\right)^{2}");
    check(!t.plain, "without the domain the real cube root and the square of the sixth root differ");
    check(t.where_defined, "where the sixth root is defined they are the same number");
    t = zero_tests("\\sqrt{x^{2}}", "x");
    check(!t.plain && !t.where_defined, "\\sqrt{x^2} = x is not proved, not even where both sides are defined");
    t = zero_tests("\\sqrt[6]{x^{2}}", "\\sqrt[3]{x}");
    check(!t.where_defined, "the radicand x^2 >= 0 says nothing on the sign of x: \\sqrt[6]{x^2} = \\sqrt[3]{x} is not proved");

    /* 2. Da dove vengono i fatti. La radice cubica, definita per ogni base,
     * non dice che x + 3 >= 0; la radice quadrata, la potenza 1/3 e il
     * logaritmo sì. */
    t = zero_tests("\\sqrt{\\left(x + 3\\right)^{2}} \\cdot \\sqrt[3]{x + 3}", "\\left(x + 3\\right)\\sqrt[3]{x + 3}");
    check(!t.where_defined, "an odd root gives no fact: \\sqrt{(x + 3)^2} is not x + 3 next to \\sqrt[3]{x + 3}");
    t = zero_tests("\\sqrt{\\left(x + 3\\right)^{2}} \\cdot \\sqrt{x + 3}", "\\left(x + 3\\right)\\sqrt{x + 3}");
    check(t.where_defined, "a square root gives the fact x + 3 >= 0, and with it \\sqrt{(x + 3)^2} = x + 3");
    t = zero_tests("\\sqrt{\\left(x + 3\\right)^{2}} \\cdot \\left(x + 3\\right)^{\\frac{1}{3}}", "\\left(x + 3\\right)^{\\frac{4}{3}}");
    check(t.where_defined, "a power with exponent 1/3 is defined only for a base >= 0, and gives the fact");
    t = zero_tests("\\sqrt{\\left(x + 3\\right)^{2}} + \\ln\\left(x + 3\\right)", "x + 3 + \\ln\\left(x + 3\\right)");
    check(t.where_defined, "a logarithm without absolute value gives the fact x + 3 > 0");
    t = zero_tests("\\sqrt{\\left(x + 3\\right)^{2}} + \\ln\\left|x + 3\\right|", "x + 3 + \\ln\\left|x + 3\\right|");
    check(!t.where_defined, "a logarithm of an absolute value gives no fact");

    /* 3. I quattro usi. */
    t = zero_tests("\\sqrt[3]{x} \\cdot \\sqrt{x}", "\\left(\\sqrt[6]{x}\\right)^{5}");
    check(t.where_defined, "11q-1: with x >= 0 the cube root, the square root and the sixth root are powers of x");
    t = zero_tests("\\sqrt{x}\\sqrt{x - 1}", "\\sqrt{x^{2} - x}");
    check(!t.plain && t.where_defined, "11q-2: the root of a product of two facts is the product of the roots");
    t = zero_tests("\\sqrt{\\frac{x}{x - 1}}", "\\frac{\\sqrt{x}}{\\sqrt{x - 1}}");
    check(t.where_defined, "11q-2: the root of a fraction whose two terms are facts");
    t = zero_tests("|x| + \\sqrt{x}", "x + \\sqrt{x}");
    check(!t.plain && t.where_defined, "11q-3: |x| = x where x >= 0 is a fact");
    t = zero_tests("|x| + \\sqrt{-x}", "-x + \\sqrt{-x}");
    check(t.where_defined, "11q-3: |x| = -x where -x >= 0 is a fact");
    t = zero_tests("|x| + \\sqrt{-x}", "x + \\sqrt{-x}");
    check(!t.where_defined, "11q-3: |x| = x is not proved where -x >= 0");
    t = zero_tests("\\sqrt{1 - \\left(\\sqrt{1 - x}\\right)^{2}}", "\\sqrt{x}");
    check(!t.plain && t.where_defined, "11q-4: the radicand loses the square of the root it contains");

    /* 4. Niente resta acceso. */
    check(t.facts_left == 0, "after the zero test the facts are off and forgotten");
}

/* Ritorna il verdetto di prove() sulla formula, che deve stare in un
 * buffer modificabile come quelli letti da un file. */
static Verdict verdict_of(const char *formula) {
    static char text[MAX_FORMULA];
    snprintf(text, sizeof text, "%s", formula);
    return prove(text, 'n', 0, 0);
}

/* 5. Vere dove i due membri sono definiti, e solo lì: dimostrate. Accanto a
 * ognuna, l'uso dei fatti che serve. */
static const char *const true_claims[] = {
    /* 11q-1: x >= 0 viene dalla radice sesta o dalla radice quadrata. */
    "\\frac{d}{dx}\\left(\\sqrt[3]{x}\\right) = \\frac{1}{3\\left(\\sqrt[6]{x}\\right)^{4}}",
    "\\frac{d}{dx}\\left(\\left(\\sqrt[6]{x}\\right)^{2}\\right) = \\frac{\\sqrt[3]{x}}{3x}",
    "\\int \\frac{1}{\\sqrt[3]{x}} \\, dx = \\frac{3}{2}\\left(\\sqrt[6]{x}\\right)^{4} + c",
    "\\int \\frac{\\sqrt[3]{x}}{\\sqrt{x}} \\, dx = \\frac{6}{5}\\left(\\sqrt[6]{x}\\right)^{5} + c",
    "\\int \\frac{\\sqrt{x}}{\\sqrt[3]{x}} \\, dx = \\frac{6}{7}\\left(\\sqrt[6]{x}\\right)^{7} + c",
    /* 11q-2: prodotti e frazioni, quadrati con il segno di un caso. */
    "\\frac{d}{dx}\\left(\\sqrt{x}\\sqrt{x + 1}\\right) = \\frac{2x + 1}{2\\sqrt{x^{2} + x}}",
    "\\frac{d}{dx}\\left(\\sqrt{\\frac{x}{x + 1}}\\right) = \\frac{1}{2\\sqrt{x}\\left(x + 1\\right)^{\\frac{3}{2}}}",
    "\\frac{d}{dx}\\left(\\sqrt{x^{2}}\\right) = \\frac{x}{|x|}",
    "\\frac{d}{dx}\\left(\\sqrt{x^{2} + 2x + 1}\\right) = \\frac{x + 1}{|x + 1|}",
    "\\frac{d}{dx}\\left(\\sqrt{1 - \\cos^{2} x}\\right) = \\frac{\\sin x \\cos x}{|\\sin x|}",
    /* 11q-3: |x| = x dove x >= 0. */
    "\\int \\frac{|x|}{\\sqrt{x}} \\, dx = \\frac{2}{3}\\left(\\sqrt{x}\\right)^{3} + c",
    /* x + 3 >= 0 da una potenza 1/3 e da una radice quadrata. */
    "\\frac{d}{dx}\\left(\\left(x + 3\\right)^{\\frac{1}{3}} \\cdot \\sqrt{\\left(x + 3\\right)^{2}}\\right) = \\frac{4}{3}\\left(x + 3\\right)^{\\frac{1}{3}}",
    "\\frac{d}{dx}\\left(\\sqrt{x + 3} \\cdot \\sqrt{\\left(x + 3\\right)^{2}}\\right) = \\frac{3}{2}\\sqrt{x + 3}",
    /* x + 3 > 0 dal logaritmo della funzione derivata, e x + 8 > 0 da
     * quello della primitiva: nella derivata il logaritmo non c'è più. */
    "\\frac{d}{dx}\\left(\\ln\\left(x + 3\\right) + \\sqrt{\\left(x + 3\\right)^{2}}\\right) = \\frac{1}{x + 3} + 1",
    "\\int \\left(\\frac{1}{x + 8} + \\frac{\\sqrt{\\left(x + 8\\right)^{2}}}{x + 8}\\right) dx = \\ln\\left(x + 8\\right) + x + c",
};

/* 6. False: nessuna deve essere dimostrata. */
static const char *const false_claims[] = {
    /* False solo per x < -3, oltre i punti di prova: le ferma solo il test
     * di annullamento. Vengono subito dopo enunciati con il fatto
     * x + 3 >= 0: un fatto rimasto da un enunciato precedente le farebbe
     * passare. */
    "\\frac{d}{dx}\\left(\\sqrt{\\left(x + 3\\right)^{2}}\\right) = 1",
    "\\frac{d}{dx}\\left(\\sqrt{\\left(x + 3\\right)^{2}} \\cdot \\sqrt[3]{x + 3}\\right) = \\frac{4}{3}\\sqrt[3]{x + 3}",
    "\\frac{d}{dx}\\left(\\sqrt{\\left(x + 3\\right)^{2}} + \\left|x + 3\\right|\\right) = 2",
    "\\int \\frac{\\sqrt{\\left(x + 8\\right)^{2}}}{x + 8} \\, dx = x + c",
    "\\int \\sqrt[3]{x + 8} \\cdot \\frac{\\sqrt{\\left(x + 8\\right)^{2}}}{x + 8} \\, dx = \\frac{3}{4}\\left(x + 8\\right)\\sqrt[3]{x + 8} + c",
    /* Come le due vere qui sopra, ma con il valore assoluto nel
     * logaritmo: la funzione e la primitiva sono definite anche per
     * x < -3 e x < -8, dove la formula è sbagliata. */
    "\\frac{d}{dx}\\left(\\ln\\left|x + 3\\right| + \\sqrt{\\left(x + 3\\right)^{2}}\\right) = \\frac{1}{x + 3} + 1",
    "\\int \\left(\\frac{1}{x + 8} + \\frac{\\sqrt{\\left(x + 8\\right)^{2}}}{x + 8}\\right) dx = \\ln\\left|x + 8\\right| + x + c",
    /* False anche nei punti di prova. */
    "\\frac{d}{dx}\\left(\\sqrt{x^{2}}\\right) = 1",
    "\\int \\frac{\\sqrt{x^{2}}}{x} \\, dx = x + c",
    "\\frac{d}{dx}\\left(\\sqrt[3]{x}\\right) = \\frac{\\sqrt[3]{x}}{3\\sqrt{x^{2}}}",
    "\\frac{d}{dx}\\left(\\sqrt[3]{x}\\sqrt[3]{x}\\right) = \\frac{2}{3\\sqrt[3]{x}} + \\frac{\\sqrt{x^{2}}}{x} - 1",
    "\\int \\frac{1}{\\sqrt[3]{x}} \\, dx = \\frac{3}{2}\\sqrt[3]{x^{2}} \\cdot \\frac{\\sqrt{x^{2}}}{x} + c",
    "\\int \\frac{\\sqrt[3]{x}}{x} \\, dx = 3\\sqrt[3]{x} \\cdot \\frac{\\sqrt{x^{2}}}{x} + c",
    "\\frac{d}{dx}\\left(\\sqrt[3]{x^{2}}\\right) = \\frac{2}{3\\sqrt[6]{x^{2}}}",
    "\\frac{d}{dx}\\left(\\sqrt{1 - \\cos^{2} x}\\right) = \\cos x",
    "\\frac{d}{dx}\\left(\\sqrt{x^{2} + 2x + 1}\\right) = 1",
    "\\frac{d}{dx}\\left(x^{2}\\right) = 2\\sqrt{-x}\\sqrt{-x}",
    "\\frac{d}{dx}\\left(\\sqrt{-x}\\sqrt{-1 - x}\\right) = -\\frac{2x + 1}{2\\sqrt{x^{2} + x}}",
    /* I due membri non sono mai definiti insieme: x <= -1 per il primo,
     * x >= 0 per il secondo. Vera "a vuoto", ma non c'è un punto in cui i
     * numeri la confermino, e DIMOSTRATO lo chiede. */
    "\\frac{d}{dx}\\left(\\sqrt{-x}\\sqrt{-1 - x}\\right) = \\frac{2x + 1}{2\\sqrt{x}\\sqrt{x + 1}}",
    /* Con il quantificatore l'enunciato parla di ogni numero reale. */
    "\\forall x \\in \\mathbb{R}\\ \\sqrt[3]{x} = \\left(\\sqrt[6]{x}\\right)^{2}",
    "\\forall x \\in \\mathbb{R}\\ \\sqrt{x^{2}} = x",
    "\\forall x \\in \\mathbb{R} \\setminus \\{0\\}\\ \\ln\\left(x^{2}\\right) = 2\\ln x",
};

/* 7. Vere dove i due membri sono definiti, ma il motore non le decide
 * ancora: non devono essere dichiarate false. \sqrt{x^2 + x} è
 * \sqrt{-x} \sqrt{-1 - x} dove x <= -1, ma per vederlo il resto x^2 + x
 * andrebbe diviso per il fatto -x (vedi "Che cosa manca" in 11q). */
static const char *const undecided_claims[] = {
    "\\frac{d}{dx}\\left(\\sqrt{-x}\\sqrt{-1 - x}\\right) = \\frac{2x + 1}{2\\sqrt{x^{2} + x}}",
};

/* 5, 6, 7. Gli enunciati, con il verdetto chiesto. */
static void test_statements(void) {
    char what[MAX_FORMULA + 64];
    for (size_t i = 0; i < sizeof true_claims / sizeof *true_claims; i++) {
        Verdict v = verdict_of(true_claims[i]);
        snprintf(what, sizeof what, "proved where both sides are defined: %s", true_claims[i]);
        check(v.outcome == PROVED, what);
        check(domain_on == 0 && domain_fact_count == 0, "after a statement the facts are off and forgotten");
    }
    for (size_t i = 0; i < sizeof false_claims / sizeof *false_claims; i++) {
        Verdict v = verdict_of(false_claims[i]);
        snprintf(what, sizeof what, "not proved: %s", false_claims[i]);
        check(v.outcome != PROVED, what);
    }
    for (size_t i = 0; i < sizeof undecided_claims / sizeof *undecided_claims; i++) {
        Verdict v = verdict_of(undecided_claims[i]);
        snprintf(what, sizeof what, "not called false: %s", undecided_claims[i]);
        check(v.outcome != REFUTED, what);
    }
}

int main(void) {
    engine_init();
    test_mechanism();
    test_statements();

    /* 8. Ogni derivata e ogni primitiva è stata ricontrollata con i numeri:
     * nessuna smentita. */
    check(unjustified_steps == 0, "every step passed its independent check");
    printf("%s: %d of %d checks passed\n", failures ? "FAIL" : "PASS", checks - failures, checks);
    return failures ? 1 : 0;
}
