/* Test del motore: la verifica indipendente della regola 18u (capitolo 19).
 *
 * Controlla i tre pezzi che rendono quella verifica qualcosa di più di una
 * formalità, senza riferirsi a un esercizio:
 *
 *   1. i valori da provare (sample_equations(), sample_candidates(),
 *      check_sample()): da un'ipotesi "esiste k con x = k^2 + 7" la verifica
 *      ricava x = 7, 8, 11, 16, ..., scegliendo k; da "n divide a - b"
 *      ricava b = a - q n; tra i razionali da provare ce n'è sempre uno che
 *      non è intero;
 *   2. il controllo del valore scelto da 18u-3 (witness_records[],
 *      records_doubt()): il valore vero regge, uno alterato apposta no;
 *   3. il controllo della copia rinominata di 18u-1 (formula_renamed()): la
 *      stessa condizione con un'altra lettera passa; un insieme cambiato, un
 *      corpo non rinominato, una lettera nuova che ne cattura un'altra, un
 *      coefficiente cambiato no;
 *
 * e la regola "Quante prove servono": una dimostrazione le cui ipotesi non
 * valgono in nessun valore del campione non viene confermata, anche se
 * l'enunciato è vero; una le cui ipotesi valgono in un punto solo sì.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_witness_checks \
 *         tests/engine/test_witness_checks.c && /tmp/test_witness_checks
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

/* Legge text come una condizione con quantificatori, con la memoria e le
 * tabelle ripartite da zero come all'inizio di un enunciato. Le lettere non
 * quantificate restano libere: qui servono così. Ritorna NULL se il testo
 * non si legge. */
static Formula *read_condition(const char *text) {
    arena_used = arena_keep;
    atom_count = base_poly_count = atan_count = 0;
    env_count = map_count = 0;
    failure = syntax_error = NULL;
    cursor = text;
    next_token();
    Formula *f = logic_formula();
    return syntax_error || failure || token.kind != T_END ? NULL : f;
}

/* Ritorna 1 se value[0..count) contiene il numero intero n. */
static int has_value(const Frac *value, int count, long long n) {
    for (int i = 0; i < count; i++)
        if (frac_cmp(value[i], frac_int(n)) == 0) return 1;
    return 0;
}

/* 1. I valori da provare. */
static void test_candidates(void) {
    Wanted wanted[MAX_EQUATIONS];
    Frac value[SAMPLE_SIZE];

    /* Un'ipotesi con il suo testimone: si sceglie k e si calcola x. */
    Formula *square = read_condition("\\exists k \\in \\mathbb{Z}\\ (x = k^{2} + 7)");
    int count = square ? sample_equations(square, 0, NULL, wanted, 0) : 0;
    check(count == 1 && wanted[0].witness == 'k', "the equality under an \"exists k\" is kept, with k as its witness");
    int values = count ? sample_candidates('x', &all_integers, wanted, count, SAMPLE_SIZE, NULL, 0, value) : 0;
    check(values >= 4 && has_value(value, values, 7) && has_value(value, values, 8) && has_value(value, values, 11) &&
          has_value(value, values, 16), "x = k^2 + 7 gives the values 7, 8, 11, 16 to try");
    check(!has_value(value, values, 0) && !has_value(value, values, 9), "no value that is not of the form k^2 + 7 is proposed");

    /* La stessa condizione da rendere falsa (è un'ipotesi negata, cioè un
     * "per ogni k"): non dà valori. */
    count = square ? sample_equations(square, 1, NULL, wanted, 0) : -1;
    check(count == 0, "an equality that has to fail gives no value to try");

    /* Una divisibilità, letta con la sua definizione: b = a - q n. La
     * lettura riparte dall'arena vuota, quindi `square` non vale più. */
    Formula *divides = read_condition("n \\mid a - b");
    count = divides ? sample_equations(divides, 0, NULL, wanted, 0) : 0;
    check(count == 1 && wanted[0].set == &all_integers, "a divisibility is read as an equality with an integer quotient");
    env_push('n', rat_int(3));
    env_push('a', rat_int(5));
    values = count ? sample_candidates('b', &all_integers, wanted, count, SAMPLE_SIZE, NULL, 0, value) : 0;
    env_count = 0;
    check(has_value(value, values, 5) && has_value(value, values, 2) && has_value(value, values, 8) && has_value(value, values, 20),
          "3 | 5 - b gives b = 5, 2, 8, ..., 20");
    check(!has_value(value, values, 4), "3 | 5 - b does not give b = 4");

    /* Il verificatore deve attraversare anche un gruppo con piu di dodici
     * testimoni. Solo m compare nell'equazione: gli altri non cambiano
     * l'insieme dei valori, perche gli interi sono non vuoti. */
    Formula *deep = read_condition("\\exists a,b,c,d,e,f,g,h,i,j,k,l,m \\in \\mathbb{Z}\\ (x = m^{2} + 7)");
    count = deep ? sample_equations(deep, 0, NULL, wanted, 0) : 0;
    check(count == 1 && wanted[0].witness == 'm', "the independent verifier keeps a witness beyond twelve binders");
    values = count ? sample_candidates('x', &all_integers, wanted, count, SAMPLE_SIZE, NULL, 0, value) : 0;
    check(has_value(value, values, 7) && has_value(value, values, 8), "the deep witness still proposes exact square-plus-seven values");
    deep = read_condition("\\exists a,b,c,d,e,f,g,h,i,j,k,l,m \\in \\mathbb{Z}\\ (x = m^{2} + l^{2} + 7)");
    count = deep ? sample_equations(deep, 0, NULL, wanted, 0) : -1;
    check(count == 0, "a deep condition with two witnesses is not misread as one with a single witness");
}

/* 2. Il valore scelto da 18u-3, ricontrollato con i numeri. */
static void test_records(void) {
    /* 2x = k^2 + 14 vuole x >= 7: nessuno dei primi razionali la soddisfa.
     * La dimostrazione sceglie m = (k^2 + 14)^2. */
    Verdict verdict = prove("\\forall x \\in \\mathbb{Q}\\ ((\\exists k \\in \\mathbb{Z}\\ (2x = k^{2} + 14)) \\implies "
                            "\\exists m \\in \\mathbb{Z}\\ (4x^{2} = m))", 'n', 0, 0);
    check(verdict.outcome == PROVED, "a hypothesis that no small number satisfies: the statement is proved");
    /* Scegliendo k tra i primi interi, x = (k^2 + 14)/2 prende sette valori
     * diversi (k^2 = 0, 1, 4, ..., 36): ne bastano quattro per dire che la
     * verifica non si è fermata a uno. Con i soli primi razionali erano zero. */
    check(sample_theses >= 4, "the check of the statement found several values that satisfy the hypothesis");
    check(record_count == 1 && records_doubt() == NULL, "the value chosen for m is recorded and holds on the numbers");

    /* Lo stesso valore alterato: più uno, poi cambiato di segno. La verifica
     * dell'enunciato com'è scritto non se ne accorgerebbe, perché
     * l'enunciato resta vero: questa sì. */
    if (record_count == 1) {
        Rat chosen = witness_records[0].value[0];
        witness_records[0].value[0] = rat_add(chosen, rat_int(1), 0);
        check(records_doubt() != NULL, "the chosen value plus one is rejected");
        witness_records[0].value[0] = rat_mul(chosen, rat_int(-1));
        check(records_doubt() != NULL, "the chosen value with the wrong sign is rejected");
        witness_records[0].value[0] = chosen;
        check(records_doubt() == NULL, "the chosen value, put back, holds again");
    }

    /* Due ipotesi scritte con la stessa lettera k: sono due numeri, e i
     * testimoni ricevono un valore ciascuno prima delle altre variabili. */
    verdict = prove("\\forall x, y \\in \\mathbb{Q}\\ ((\\exists k \\in \\mathbb{Z}\\ (2x = k^{2} + 14) \\land "
                    "\\exists k \\in \\mathbb{Z}\\ (2y = k^{2} + 14)) \\implies \\exists m \\in \\mathbb{Z}\\ (4xy = m))", 'n', 0, 0);
    check(verdict.outcome == PROVED && record_count == 1 && witness_records[0].generics == 4, "two hypotheses with the same letter are named apart");
    if (record_count == 1) {
        int first = witness_records[0].generic[0].name, second = witness_records[0].generic[1].name;
        check(first != second && first != 'x' && first != 'y' && second != 'x' && second != 'y',
              "the two witnesses have different letters, and get their values before x and y");
    }
}

/* Un campione di razionali contiene sempre un numero che non è intero. Ogni
 * lettura riparte dall'arena vuota: la condizione letta prima non vale più. */
static void test_rational_sample(void) {
    Frac value[SAMPLE_SIZE + 1];
    Formula *rationals = read_condition("\\forall x \\in \\mathbb{Q}\\ (x = x)");
    int count = rationals ? check_sample(&rationals->numbers, value, 5) : 0;
    check(count == 6 && frac_cmp(value[5], frac_make(1, 2)) == 0, "five rationals to try are 0, 1, -1, 2, -2 and also 1/2");
    Formula *integers = read_condition("\\forall x \\in \\mathbb{Z}\\ (x = x)");
    count = integers ? check_sample(&integers->numbers, value, 5) : 0;
    check(count == 5, "five integers to try are just five");
}

/* 3. La copia rinominata deve dire la stessa cosa. */
static void test_renamed(void) {
    static const struct {
        const char *copy;
        int expected;
        const char *what;
    } copies[] = {
        {"\\exists h \\in \\mathbb{Z}\\ (x = 2h)", 1, "the same condition with h for k is accepted"},
        {"\\exists h \\in \\mathbb{N}\\ (x = 2h)", 0, "a copy with another set is rejected"},
        {"\\exists h \\in \\mathbb{Z}\\ (x = 2k)", 0, "a copy whose body still says k is rejected"},
        {"\\exists h \\in \\mathbb{Z}\\ (x = 3h)", 0, "a copy with another coefficient is rejected"},
        {"\\exists h \\in \\mathbb{Z}\\ (y = 2h)", 0, "a copy with another free letter is rejected"},
    };
    const int from[] = {'k'}, to[] = {'h'};
    for (size_t i = 0; i < sizeof copies / sizeof *copies; i++) {
        /* Le due letture nella stessa memoria: la seconda non riparte. */
        Formula *original = read_condition("\\exists k \\in \\mathbb{Z}\\ (x = 2k)");
        cursor = copies[i].copy;
        next_token();
        Formula *copy = original ? logic_formula() : NULL;
        check(copy && !syntax_error && formula_renamed(original, copy, from, to, 1) == copies[i].expected, copies[i].what);
    }

    /* La lettera nuova non deve comparire già: h prenderebbe il posto di
     * due variabili diverse. */
    Formula *original = read_condition("\\exists k \\in \\mathbb{Z}\\ (h = 2k)");
    cursor = "\\exists h \\in \\mathbb{Z}\\ (h = 2h)";
    next_token();
    Formula *copy = original ? logic_formula() : NULL;
    check(copy && !formula_renamed(original, copy, from, to, 1), "a new letter that captures another one is rejected");
}

/* La regola "Quante prove servono". */
static void test_too_few(void) {
    /* x^2 = 2k^2 + 3 non ha soluzioni (x^2 sarebbe 3 o 5 modulo 8), quindi
     * l'enunciato è vero; ma nessun valore del campione soddisfa l'ipotesi,
     * e una verifica che non ha mai trovato vera l'ipotesi non ha
     * controllato nulla. */
    Verdict verdict = prove("\\forall x \\in \\mathbb{Q}\\ ((\\exists k \\in \\mathbb{Z}\\ (x^{2} = 2k^{2} + 3)) \\implies "
                            "\\exists m \\in \\mathbb{Z}\\ (2m = x^{2} + 1))", 'n', 0, 0);
    check(verdict.outcome == NOT_PROVED && verdict.reason && strstr(verdict.reason, "non ha controllato la tesi"),
          "a proof whose hypothesis no sample satisfies is not confirmed");

    /* Ipotesi che valgono in un punto solo, x = y = 1: una prova basta,
     * perché è tutta la verifica possibile. */
    verdict = prove("G = \\{(u, v) \\in \\mathbb{Q} \\times \\mathbb{N} \\mid -3u = -3\\} \\implies "
                    "((\\forall x, y\\ (((x, y) \\in G \\land (y, x) \\in G) \\implies x = y)))", 'n', 0, 0);
    check(verdict.outcome == PROVED && sample_theses == 1, "hypotheses that hold at one point only are checked there, and that is enough");
}

int main(void) {
    engine_init();
    test_candidates();
    test_rational_sample();
    test_records();
    test_renamed();
    test_too_few();
    check(unjustified_steps == 0, "every step of the written proofs passed its independent check");
    if (failures) printf("FAIL: %d of %d witness check tests failed\n", failures, checks);
    else printf("PASS: %d of %d witness check tests passed\n", checks, checks);
    return failures ? 1 : 0;
}
