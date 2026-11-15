/* Test del capitolo 20 del verificatore: gli enunciati sui numeri mostrati
 * veri o falsi da dei testimoni ("certificate numbers"), compilato senza il
 * motore.
 *
 * I certificati giusti devono essere accettati; quelli con un testimone
 * sbagliato, fuori dal suo insieme, mancante o in più, con l'affermazione
 * opposta, o che vorrebbero mostrare con un valore un "per ogni" vero o un
 * "esiste" falso, devono essere respinti. Poi un oracolo: enunciati a caso
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
    errors += expect(answer(SQUARES "fails\nwitness 0 7\nwitness 1 8\nwitness 2 0\nend\n") == CK_REJECTED,
                     "a missing witness leaves the counterexample unproved");
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
    errors += expect(answer("certificate numbers\nkind int\nstatement (forall (var 120) (set) (= (var 120) (var 120)))\nholds\nend\n") == CK_REJECTED,
                     "a true universal statement is not accepted without anything");

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
    int errors = certificates() + oracle();
    if (errors) { printf("%d checks failed\n", errors); return 1; }
    printf("number statements with witnesses: all checks passed\n");
    return 0;
}
