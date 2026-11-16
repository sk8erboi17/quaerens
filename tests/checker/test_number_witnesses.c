/* Test del capitolo 20 del verificatore: gli enunciati sui numeri mostrati
 * veri o falsi da dei testimoni ("certificate numbers"), compilato senza il
 * motore.
 *
 * I certificati giusti devono essere accettati; quelli con un testimone
 * sbagliato, fuori dal suo insieme, mancante o in più, con l'affermazione
 * opposta, o che vorrebbero mostrare con un valore un "per ogni" vero o un
 * "esiste" falso, devono essere respinti. Un quantificatore senza
 * testimone si stabilisce solo con il conto completo di 20e, su una
 * variabile intera: i suoi certificati giusti sono accettati, quelli con
 * l'affermazione opposta e quelli fuori dalla sua portata (numeri reali,
 * due variabili, un divisore che dipende dalla variabile, numeri troppo
 * grandi) no; e un secondo oracolo confronta quel conto, su enunciati a
 * caso in una variabile, con la prova diretta di tutti gli interi tra -400
 * e 400. Poi il primo oracolo: enunciati a caso
 * con le variabili confinate tra -2 e 2, di cui un conto diretto in C
 * stabilisce la verità provando tutti i valori; per ognuno si cercano i
 * testimoni che il verificatore accetta, provandoli tutti, e si controlla
 * che ne accetti qualcuno solo per ciò che è vero ("holds" su un enunciato
 * vero, "fails" su uno falso). L'oracolo calcola per conto suo: non usa
 * nessuna funzione del capitolo 20.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_number_witnesses \
 *         tests/checker/test_number_witnesses.c && /tmp/test_number_witnesses
 */

#include <stdio.h>
#include <string.h>
#define main checker_main
#include "../../checker.c"
#undef main

/* Controlla il certificato text e ritorna la risposta del verificatore. */
static CkAnswer answer(const char *text) {
    const char *cursor = text, *why = "";
    return ck_check(&cursor, &why, 0);
}

/* Conta un errore e lo stampa, se condition è falsa. */
static int expect(int condition, const char *what) {
    if (!condition) printf("FAILED: %s\n", what);
    return !condition;
}

/* "Per ogni x, y interi: se x = k^2 + 7 per qualche k e y = j^2 + 7 per
 * qualche j, allora x = y." Falso con x = 7, y = 8, k = 0, j = 1. Le
 * lettere: 120 x, 121 y, 107 k. */
#define SQUARES \
    "certificate numbers\nkind int\n" \
    "statement (forall (var 120) (set) (forall (var 121) (set) (implies (and " \
    "(exists (var 107) (set) (= (var 120) (add (pow (var 107) 2) 7))) " \
    "(exists (var 107) (set) (= (var 121) (add (pow (var 107) 2) 7)))) (= (var 120) (var 121)))))\n"

static int certificates(void) {
    int errors = 0;

    /* Un "per ogni" smentito da un controesempio, con i testimoni degli
     * "esiste" dell'ipotesi. */
    errors += expect(answer(SQUARES "fails\nwitness 0 7\nwitness 1 8\nwitness 2 0\nwitness 3 1\nend\n") == CK_ACCEPTED,
                     "a counterexample with the witnesses of its hypotheses is accepted");
    errors += expect(answer(SQUARES "fails\nwitness 0 7\nwitness 1 7\nwitness 2 0\nwitness 3 0\nend\n") == CK_REJECTED,
                     "values for which the statement holds are not a counterexample");
    errors += expect(answer(SQUARES "fails\nwitness 0 7\nwitness 1 8\nwitness 2 0\nwitness 3 2\nend\n") == CK_REJECTED,
                     "a wrong witness of a hypothesis leaves the counterexample unproved");
    errors += expect(answer(SQUARES "fails\nwitness 1 8\nwitness 2 0\nwitness 3 1\nend\n") == CK_REJECTED,
                     "a missing witness of the statement's own quantifier leaves the counterexample unproved");
    /* Il testimone di j manca, ma con y = 8 "esiste j con 8 = j^2 + 7" è
     * una sola variabile intera: lo stabilisce il conto di 20e. */
    errors += expect(answer(SQUARES "fails\nwitness 0 7\nwitness 1 8\nwitness 2 0\nend\n") == CK_ACCEPTED,
                     "a witness that the complete count finds by itself may be missing");
    errors += expect(answer(SQUARES "holds\nwitness 0 7\nwitness 1 8\nwitness 2 0\nwitness 3 1\nend\n") == CK_REJECTED,
                     "a false statement is not accepted as holding");
    errors += expect(answer(SQUARES "fails\nwitness 0 7\nwitness 1 8\nwitness 2 0\nwitness 3 1\nwitness 4 0\nend\n") == CK_REJECTED,
                     "a witness for a quantifier that is not there is refused");
    errors += expect(answer(SQUARES "fails\nwitness 0 7\nwitness 0 8\nwitness 2 0\nwitness 3 1\nend\n") == CK_REJECTED,
                     "two witnesses for one quantifier are refused");

    /* Un "per ogni" vero non si mostra con un valore, e un "esiste" falso
     * nemmeno. */
    errors += expect(answer("certificate numbers\nkind int\nstatement (forall (var 120) (set) (= (var 120) (var 120)))\nholds\nwitness 0 3\nend\n") == CK_REJECTED,
                     "a true universal statement is not shown by one value");
    errors += expect(answer("certificate numbers\nkind int\nstatement (exists (var 120) (set) (neq (var 120) (var 120)))\nfails\nwitness 0 3\nend\n") == CK_REJECTED,
                     "a false existential statement is not shown by one value");
    errors += expect(answer("certificate numbers\nkind real\nstatement (forall (var 120) (set) (= (var 120) (var 120)))\nholds\nend\n") == CK_REJECTED,
                     "a true universal statement on real numbers is not accepted without anything");
    errors += expect(answer("certificate numbers\nkind int\nstatement (forall (var 120) (set) (forall (var 121) (set) (= (add (var 120) (var 121)) (add (var 121) (var 120)))))\nholds\nend\n") == CK_REJECTED,
                     "a true universal statement in two variables is not accepted without anything");

    /* Un "esiste" vero, con il suo testimone; i generi di numeri. */
    errors += expect(answer("certificate numbers\nkind int\nstatement (exists (var 120) (set ge 1) (= (mul (var 120) (var 120)) 49))\nholds\nwitness 0 7\nend\n") == CK_ACCEPTED,
                     "a true existential statement with its witness is accepted");
    errors += expect(answer("certificate numbers\nkind int\nstatement (exists (var 120) (set ge 1) (= (mul (var 120) (var 120)) 49))\nholds\nwitness 0 -7\nend\n") == CK_REJECTED,
                     "a witness below the bound of its set is refused");
    errors += expect(answer("certificate numbers\nkind rat\nstatement (exists (var 113) (set gt 0) (= (mul 2 (var 113)) 1))\nholds\nwitness 0 1/2\nend\n") == CK_ACCEPTED,
                     "a rational witness is accepted among rationals");
    errors += expect(answer("certificate numbers\nkind int\nstatement (exists (var 113) (set gt 0) (= (mul 2 (var 113)) 1))\nholds\nwitness 0 1/2\nend\n") == CK_REJECTED,
                     "a rational witness is refused among integers");
    errors += expect(answer("certificate numbers\nkind real\nstatement (forall (var 97) (set) (forall (var 120) (set) (less 0 (add (pow (var 120) 2) (pow (var 97) 2)))))\nfails\nwitness 0 0\nwitness 1 0\nend\n") == CK_ACCEPTED,
                     "a real statement is refuted by rational values");
    errors += expect(answer("certificate numbers\nkind rat\nstatement (forall (var 120) (set except 0/1) (neq (var 120) 0))\nfails\nwitness 0 0\nend\n") == CK_REJECTED,
                     "a witness that is a removed point is refused");
    errors += expect(answer("certificate numbers\nkind rat\nstatement (exists (var 120) (set) (= (div (var 120) 3) (div 1 2)))\nholds\nwitness 0 3/2\nend\n") == CK_ACCEPTED,
                     "a division by a written integer is computed exactly");

    /* La divisibilità: zero divide solo zero. */
    errors += expect(answer("certificate numbers\nkind int\nstatement (divides 0 0)\nholds\nend\n") == CK_ACCEPTED, "zero divides zero");
    errors += expect(answer("certificate numbers\nkind int\nstatement (divides 0 5)\nfails\nend\n") == CK_ACCEPTED, "zero does not divide five");
    errors += expect(answer("certificate numbers\nkind int\nstatement (divides 3 (neg 12))\nholds\nend\n") == CK_ACCEPTED, "three divides minus twelve");
    errors += expect(answer("certificate numbers\nkind int\nstatement (divides 5 12)\nholds\nend\n") == CK_REJECTED, "five does not divide twelve");
    errors += expect(answer("certificate numbers\nkind rat\nstatement (divides 1 2)\nholds\nend\n") != CK_ACCEPTED, "divisibility is among integers only");

    /* Ciò che non è nella notazione, o è fuori posto, non è accettato. */
    errors += expect(answer("certificate numbers\nkind int\nstatement (= (pow 2 17) 131072)\nholds\nend\n") != CK_ACCEPTED, "an exponent above 16 is refused");
    errors += expect(answer("certificate numbers\nkind int\nstatement (= (div 4 2) 2)\nholds\nend\n") != CK_ACCEPTED, "a division among integers is refused");
    errors += expect(answer("certificate numbers\nkind int\nstatement (= (var 120) 0)\nholds\nend\n") != CK_ACCEPTED, "a free variable is refused");
    errors += expect(answer("certificate numbers\nstatement (= 1 1)\nholds\nend\n") != CK_ACCEPTED, "the kind of numbers must be given");
    errors += expect(answer("certificate numbers\nkind int\nstatement (= 1 1)\nend\n") != CK_ACCEPTED, "what is claimed must be given");
    errors += expect(answer("certificate numbers\nkind int\nstatement (= 1 1)\nholds\nholds\nend\n") != CK_ACCEPTED, "what is claimed is given once");

    /* Numeri troppo grandi: "non so controllarlo", mai un sì. */
    errors += expect(answer("certificate numbers\nkind int\nstatement (exists (var 120) (set) (less 0 (pow (pow (pow (var 120) 16) 16) 16)))\nholds\nwitness 0 100000000000\nend\n") != CK_ACCEPTED,
                     "numbers too large are not accepted");
    return errors;
}

/* 20e. Un quantificatore su un intero senza testimone, stabilito dal conto
 * completo: ciò che il conto stabilisce, ciò che smentisce, ciò che non
 * raggiunge. */
#define ONE(set, body) "certificate numbers\nkind int\nstatement (forall (var 120) " set " " body ")\n"
#define SOME(set, body) "certificate numbers\nkind int\nstatement (exists (var 120) " set " " body ")\n"
#define X "(var 120)"
static int counted(void) {
    int errors = 0;

    /* Veri per ogni intero, o falsi per ogni intero: il conto li stabilisce. */
    errors += expect(answer(ONE("(set)", "(= " X " " X ")") "holds\nend\n") == CK_ACCEPTED, "x = x for every integer");
    errors += expect(answer(ONE("(set)", "(divides 2 (mul " X " (add " X " 1)))") "holds\nend\n") == CK_ACCEPTED,
                     "two divides x (x + 1) for every integer");
    errors += expect(answer(ONE("(set)", "(divides 6 (mul (mul " X " (add " X " 1)) (add " X " 2)))") "holds\nend\n") == CK_ACCEPTED,
                     "six divides x (x + 1) (x + 2) for every integer");
    errors += expect(answer(ONE("(set ge 1)", "(less 0 " X ")") "holds\nend\n") == CK_ACCEPTED, "every positive integer is above zero");
    errors += expect(answer(ONE("(set)", "(lesseq " X " (mul " X " " X "))") "holds\nend\n") == CK_ACCEPTED, "x <= x^2 for every integer");
    errors += expect(answer(SOME("(set)", "(= (mul " X " " X ") 2)") "fails\nend\n") == CK_ACCEPTED, "no integer has square two");
    errors += expect(answer(SOME("(set ge 1)", "(= (add " X " 101) 101)") "fails\nend\n") == CK_ACCEPTED, "no positive integer m has m + 101 = 101");
    errors += expect(answer(SOME("(set)", "(= (mul " X " " X ") 49)") "holds\nend\n") == CK_ACCEPTED, "the count finds an integer with square 49");

    /* Radici lontane dall'origine e un insieme che comincia lontano: il
     * conto arriva fin dove il limite delle radici e il periodo dicono. */
    errors += expect(answer(SOME("(set)", "(= (mul (sub " X " 97) (add " X " 83)) 0)") "holds\nend\n") == CK_ACCEPTED,
                     "the count reaches a root at 97");
    errors += expect(answer(ONE("(set)", "(neq (mul (sub " X " 97) (add " X " 83)) 0)") "holds\nend\n") == CK_REJECTED,
                     "a polynomial with roots at 97 and -83 is not always different from zero");
    errors += expect(answer(ONE("(set ge 98)", "(less 0 (mul (sub " X " 97) (add " X " 83)))") "holds\nend\n") == CK_ACCEPTED,
                     "above its largest root the polynomial is positive");
    errors += expect(answer(SOME("(set ge 1000)", "(divides 7 (sub " X " 5))") "holds\nend\n") == CK_ACCEPTED,
                     "the count goes one period beyond the bound of the set");
    errors += expect(answer(SOME("(set ge 1000)", "(and (divides 7 (sub " X " 5)) (less " X " 1006))") "fails\nend\n") == CK_ACCEPTED,
                     "below 1006 no integer from 1000 on is 5 modulo 7");

    /* Le due confutazioni che servivano: fissati i valori trovati da chi ha
     * cercato, resta un quantificatore su un intero. "Per ogni x: se x = k
     * + 100 con k positivo, allora x = m + 101 con m positivo" è falso con
     * x = 101 e k = 1; "per ogni m esiste x positivo con x - m >= 0 e m - x
     * >= 0" è falso con m = 0. */
    errors += expect(answer("certificate numbers\nkind int\nstatement (forall (var 120) (set) (implies (exists (var 107) (set ge 1) (= (var 120) (add (var 107) 100))) "
                            "(exists (var 109) (set ge 1) (= (var 120) (add (var 109) 101)))))\nfails\nwitness 0 101\nwitness 1 1\nend\n") == CK_ACCEPTED,
                     "a counterexample whose last step is a count over one integer is accepted");
    errors += expect(answer("certificate numbers\nkind int\nstatement (forall (var 120) (set) (implies (exists (var 107) (set ge 1) (= (var 120) (add (var 107) 100))) "
                            "(exists (var 109) (set ge 1) (= (var 120) (add (var 109) 101)))))\nfails\nwitness 0 102\nwitness 1 2\nend\n") == CK_REJECTED,
                     "a value for which the statement holds is not a counterexample, and the count says so");
    errors += expect(answer("certificate numbers\nkind int\nstatement (forall (var 109) (set) (exists (var 120) (set ge 1) (and (in (sub (var 120) (var 109)) (set ge 0)) "
                            "(in (sub (var 109) (var 120)) (set ge 0)))))\nfails\nwitness 0 0\nend\n") == CK_ACCEPTED,
                     "a counterexample that needs every positive integer to fail is accepted");
    errors += expect(answer("certificate numbers\nkind int\nstatement (forall (var 109) (set) (exists (var 120) (set ge 1) (and (in (sub (var 120) (var 109)) (set ge 0)) "
                            "(in (sub (var 109) (var 120)) (set ge 0)))))\nfails\nwitness 0 3\nend\n") == CK_REJECTED,
                     "three is not a counterexample: x = 3 is there");

    /* L'affermazione opposta a ciò che il conto trova. */
    errors += expect(answer(ONE("(set)", "(divides 4 (mul " X " (add " X " 1)))") "holds\nend\n") == CK_REJECTED, "four does not divide every x (x + 1)");
    errors += expect(answer(ONE("(set)", "(less " X " (mul " X " " X "))") "holds\nend\n") == CK_REJECTED, "x < x^2 fails at zero and at one");
    errors += expect(answer(SOME("(set)", "(= (mul " X " " X ") 4)") "fails\nend\n") == CK_REJECTED, "an integer with square four exists");
    errors += expect(answer(ONE("(set ge 0)", "(less 0 " X ")") "holds\nend\n") == CK_REJECTED, "zero is not above zero");
    errors += expect(answer(ONE("(set except 0/1)", "(neq " X " 0)") "holds\nend\n") == CK_ACCEPTED, "a removed point is not tried");

    /* Fuori dalla portata del conto: non stabilito, quindi respinto,
     * qualunque cosa si affermi. */
    errors += expect(answer(ONE("(set except 0/1)", "(divides " X " (mul " X " " X "))") "holds\nend\n") != CK_ACCEPTED,
                     "a divisor that depends on the variable is not counted");
    errors += expect(answer(ONE("(set)", "(neq " X " 1000000)") "holds\nend\n") != CK_ACCEPTED, "a window too wide is not counted: not accepted as true");
    errors += expect(answer(ONE("(set)", "(neq " X " 1000000)") "fails\nend\n") != CK_ACCEPTED, "a window too wide is not counted: not accepted as false");
    errors += expect(answer(ONE("(set)", "(neq " X " 1000000)") "fails\nwitness 0 1000000\nend\n") == CK_ACCEPTED, "the same statement with its counterexample written");
    errors += expect(answer("certificate numbers\nkind rat\nstatement (forall (var 120) (set) (= " X " " X "))\nholds\nend\n") != CK_ACCEPTED,
                     "rational numbers are not counted");
    errors += expect(answer(ONE("(set)", "(= (pow (pow (pow " X " 4) 4) 4) (pow (pow (pow " X " 4) 4) 4))") "holds\nend\n") != CK_ACCEPTED,
                     "a degree above the limit is not counted");
    return errors;
}

/* L'ORACOLO DEL CONTO. Enunciati a caso in una sola variabile intera: un
 * quantificatore, un insieme, e due condizioni elementari unite da un
 * connettivo, con coefficienti tra -5 e 5. La verità la stabilisce qui un
 * conto diretto su tutti gli interi tra -400 e 400, molto oltre i punti in
 * cui queste condizioni possono cambiare (le radici stanno entro 6, i
 * periodi entro 60): il verificatore, che ne prova molti meno, deve dire
 * lo stesso, con "holds" e con "fails". */
static unsigned long count_seed = 20261116;
static int count_random(int low, int high) {
    count_seed = count_seed * 1103515245u + 12345u;
    return low + (int)((count_seed >> 8) % (unsigned long)(high - low + 1));
}

/* Una condizione elementare a caso: kind dice la forma, p, q, r i numeri. */
typedef struct { int kind, p, q, r; } CountAtom;

static int count_atom_holds(CountAtom a, long long x) {
    switch (a.kind) {
    case 0: return a.p * x * x + a.q * x + a.r == 0;
    case 1: return a.p * x + a.q < a.r;
    case 2: return a.p * x * x * x + a.q != a.r * x;
    case 3: return a.p * x * x <= a.q * x + a.r;
    case 4: { long long d = a.p < 0 ? -a.p : a.p, v = x * x + a.q * x + a.r; return d == 0 ? v == 0 : v % d == 0; }
    default: { long long d = a.p < 0 ? -a.p : a.p, v = a.q * x + a.r; return d == 0 ? v == 0 : v % d == 0; }
    }
}

/* Scrive l'intero v nella notazione dei certificati: i numeri scritti non
 * sono negativi. */
static int count_number(char *text, size_t size, int v) {
    return v < 0 ? snprintf(text, size, "(neg %d)", -v) : snprintf(text, size, "%d", v);
}

static void count_atom_text(char *text, size_t size, CountAtom a) {
    char p[16], q[16], r[16];
    count_number(p, sizeof p, a.p); count_number(q, sizeof q, a.q); count_number(r, sizeof r, a.r);
    switch (a.kind) {
    case 0: snprintf(text, size, "(= (add (add (mul %s (mul " X " " X ")) (mul %s " X ")) %s) 0)", p, q, r); break;
    case 1: snprintf(text, size, "(less (add (mul %s " X ") %s) %s)", p, q, r); break;
    case 2: snprintf(text, size, "(neq (add (mul %s (pow " X " 3)) %s) (mul %s " X "))", p, q, r); break;
    case 3: snprintf(text, size, "(lesseq (mul %s (pow " X " 2)) (add (mul %s " X ") %s))", p, q, r); break;
    case 4: snprintf(text, size, "(divides %s (add (add (mul " X " " X ") (mul %s " X ")) %s))", p, q, r); break;
    default: snprintf(text, size, "(divides %s (add (mul %s " X ") %s))", p, q, r); break;
    }
}

static int count_oracle(void) {
    static const char *const sets[4] = {"(set)", "(set ge 1)", "(set ge 0)", "(set except 0/1)"};
    static const char *const connectives[4] = {"and", "or", "implies", "iff"};
    int errors = 0, agreed = 0, held = 0;
    for (int trial = 0; trial < 3000; trial++) {
        int every = count_random(0, 1), set = count_random(0, 3), connective = count_random(0, 3);
        CountAtom a = {count_random(0, 5), count_random(-5, 5), count_random(-5, 5), count_random(-5, 5)};
        CountAtom b = {count_random(0, 5), count_random(-5, 5), count_random(-5, 5), count_random(-5, 5)};

        /* La verità, provando tutti gli interi dell'insieme tra -400 e 400. */
        int truth = every;
        for (long long x = -400; x <= 400; x++) {
            if ((set == 1 && x < 1) || (set == 2 && x < 0) || (set == 3 && x == 0)) continue;
            int left = count_atom_holds(a, x), right = count_atom_holds(b, x);
            int body = connective == 0 ? left && right : connective == 1 ? left || right : connective == 2 ? !left || right : left == right;
            if (every) truth = truth && body; else truth = truth || body;
        }

        char left[200], right[200], text[700];
        count_atom_text(left, sizeof left, a); count_atom_text(right, sizeof right, b);
        for (int claim = 0; claim < 2; claim++) {
            snprintf(text, sizeof text, "certificate numbers\nkind int\nstatement (%s " X " %s (%s %s %s))\n%s\nend\n",
                     every ? "forall" : "exists", sets[set], connectives[connective], left, right, claim ? "holds" : "fails");
            int accepted = answer(text) == CK_ACCEPTED;
            if (accepted != (claim == truth)) {
                printf("FAILED: the count %s \"%s\" for a statement that is %s:\n%s", accepted ? "accepted" : "did not accept",
                       claim ? "holds" : "fails", truth ? "true" : "false", text);
                errors++;
            } else agreed++;
        }
        held += truth;
    }
    errors += expect(held > 300 && held < 2700, "the statements of the count oracle are of both kinds");
    printf("count oracle: 3000 statements in one integer variable, %d true, %d answers as the direct count says: %s\n",
           held, agreed, errors ? "FAIL" : "PASS");
    return errors;
}
#undef X

/* L'ORACOLO. Un enunciato a caso ha due quantificatori su variabili intere
 * confinate tra -2 e 2 dal suo stesso testo, e un corpo fatto di due
 * confronti tra polinomi piccoli uniti da un connettivo. shape dice i due
 * quantificatori (bit 0 e 1: 1 = per ogni), il connettivo (bit 2 e 3), e i
 * due confronti (i bit più alti). */
static int atom_holds(int which, int x, int y) {
    switch (which % 6) {
    case 0: return x * x == y + 1;
    case 1: return x + y < 1;
    case 2: return x * y != 2;
    case 3: return 2 * x <= y * y;
    case 4: return x == y;
    default: return (x == 0 ? y == 0 : y % x == 0);
    }
}

static const char *const atom_text[6] = {
    "(= (mul (var 120) (var 120)) (add (var 121) 1))",
    "(less (add (var 120) (var 121)) 1)",
    "(neq (mul (var 120) (var 121)) 2)",
    "(lesseq (mul 2 (var 120)) (pow (var 121) 2))",
    "(= (var 120) (var 121))",
    "(divides (var 120) (var 121))"};

static int body_holds(int shape, int x, int y) {
    int a = atom_holds(shape >> 4, x, y), b = atom_holds(shape >> 7, x, y);
    switch (shape >> 2 & 3) {
    case 0: return a && b;
    case 1: return a || b;
    case 2: return !a || b;
    default: return a == b;
    }
}

/* La verità dell'enunciato, provando tutti i valori tra -2 e 2. */
static int statement_holds(int shape) {
    int outer_all = shape & 1, inner_all = shape >> 1 & 1, outer = outer_all;
    for (int x = -2; x <= 2; x++) {
        int inner = inner_all;
        for (int y = -2; y <= 2; y++) {
            int body = body_holds(shape, x, y);
            if (inner_all) inner = inner && body; else inner = inner || body;
        }
        if (outer_all) outer = outer && inner; else outer = outer || inner;
    }
    return outer;
}

/* Scrive il certificato dell'enunciato shape, con l'affermazione claim e i
 * testimoni dati (use dice quali dei due scrivere). */
static void write_certificate(char *text, size_t size, int shape, int claim, int use, int x, int y) {
    static const char *const connective[4] = {"and", "or", "implies", "iff"};
    static const char *const quantifier[2] = {"exists", "forall"};
    static const char *const guard[2] = {"and", "implies"};
    int outer_all = shape & 1, inner_all = shape >> 1 & 1;
    int length = snprintf(text, size,
        "certificate numbers\nkind int\nstatement (%s (var 120) (set) (%s (and (lesseq (neg 2) (var 120)) (lesseq (var 120) 2)) "
        "(%s (var 121) (set) (%s (and (lesseq (neg 2) (var 121)) (lesseq (var 121) 2)) (%s %s %s)))))\n%s\n",
        quantifier[outer_all], guard[outer_all], quantifier[inner_all], guard[inner_all],
        connective[shape >> 2 & 3], atom_text[(shape >> 4) % 6], atom_text[(shape >> 7) % 6], claim ? "holds" : "fails");
    if (use & 1) length += snprintf(text + length, size - (size_t)length, "witness 0 %d\n", x);
    if (use & 2) length += snprintf(text + length, size - (size_t)length, "witness 1 %d\n", y);
    snprintf(text + length, size - (size_t)length, "end\n");
}

static int oracle(void) {
    int errors = 0, accepted = 0, statements = 0;
    char text[1024];
    for (int shape = 0; shape < 16 * 36; shape++) {
        int truth = statement_holds(shape);
        statements++;
        for (int claim = 0; claim < 2; claim++) for (int use = 0; use < 4; use++)
            for (int x = -3; x <= 3; x++) for (int y = -3; y <= 3; y++) {
                if ((!(use & 1) && x != -3) || (!(use & 2) && y != -3)) continue;
                write_certificate(text, sizeof text, shape, claim, use, x, y);
                if (answer(text) != CK_ACCEPTED) continue;
                accepted++;
                if (claim != truth) {
                    printf("FAILED: accepted \"%s\" for a statement that is %s, shape %d, witnesses %d %d (use %d)\n",
                           claim ? "holds" : "fails", truth ? "true" : "false", shape, x, y, use);
                    errors++;
                }
            }
    }
    errors += expect(accepted > 1000, "the oracle test accepts many certificates: it is not vacuous");
    printf("oracle: %d statements, %d certificates accepted, none for the wrong claim: %s\n", statements, accepted, errors ? "FAIL" : "PASS");
    return errors;
}

int main(void) {
    int errors = certificates() + counted() + count_oracle() + oracle();
    if (errors) { printf("%d checks failed\n", errors); return 1; }
    printf("number statements with witnesses: all checks passed\n");
    return 0;
}
