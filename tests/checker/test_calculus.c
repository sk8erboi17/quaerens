/* Test del verificatore sui certificati delle derivate, delle primitive e
 * degli integrali definiti (capitolo 13 di checker.c), da solo: qui il motore
 * non c'entra.
 *
 * Compilazione ed esecuzione, dalla cartella principale del progetto:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/checker checker.c && \
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_calculus tests/checker/test_calculus.c && /tmp/test_calculus
 *
 * Come in test_checker.c, il primo comando fa parte del test: checker.c si
 * compila da solo, e questo file include soltanto checker.c.
 *
 * I certificati accettati sono quelli che il motore scrive (27m e 30cert di
 * main.c), o della stessa forma; gli altri ne sono alterazioni. Ognuno ha la
 * risposta attesa e, se non è accettato, un pezzo del motivo, così un
 * certificato respinto per la ragione sbagliata non passa per buono:
 *
 *   - accettati: derivate di funzioni razionali, esponenziali, logaritmi,
 *     seni e coseni (anche con l'angolo doppio e la metà dell'angolo),
 *     radici di indici diversi dello stesso radicando, valori assoluti, una
 *     potenza x^x; derivate n-esime; primitive con ln|x| e con un valore
 *     assoluto dentro il logaritmo; integrali definiti con estremi
 *     razionali, con pi greco e con e, anche con gli estremi scambiati;
 *   - respinti: una derivata sbagliata, un termine mancante, una primitiva
 *     giusta a meno del segno, un integrale attraverso un polo, un logaritmo
 *     di un numero negativo su tutto l'intervallo, un valore sbagliato (con
 *     un razionale, con i logaritmi, con pi greco), la funzione cambiata, una
 *     formula n-esima sbagliata nel passo o nel caso base, certificati
 *     scritti male;
 *   - non controllabili: identità che chiedono relazioni che il verificatore
 *     non conosce (la radice di x^2), un valore assoluto o una tangente in un
 *     integrale definito, parole che non conosce, ordini troppo alti, una
 *     voce brick con una radice, le parole di queste funzioni in un
 *     certificato di induzione. Non devono mai diventare un "accettato".
 *
 * I casi respinti e non controllabili sono antiregressione: non si tolgono
 * e non si indeboliscono per far passare una modifica al verificatore.
 */

#include <stdio.h>
#include <string.h>

/* Il verificatore ha il suo main(): lo si include cambiando nome. */
#define main checker_main
#include "../../checker.c"
#undef main

/* Un certificato scritto a mano e ciò che il verificatore deve rispondere. */
typedef struct {
    const char *name;   /* che cosa mette alla prova, in inglese */
    CkAnswer expected;  /* la risposta attesa */
    const char *why;    /* un pezzo del motivo atteso; "" per gli accettati */
    const char *text;   /* il certificato */
} Case;

#define DERIVATIVE(order, f, g) "certificate derivative\nvariable x\norder " order "\nfunction " f "\nderivative " g "\nend\n"
#define PRIMITIVE(f, F) "certificate antiderivative\nvariable x\nintegrand " f "\nprimitive " F "\nend\n"
#define INTEGRAL(a, b, f, F, v) "certificate integral\nvariable x\nlower " a "\nupper " b "\nintegrand " f "\nprimitive " F "\nvalue " v "\nend\n"

/* La funzione razionale dell'esercizio 2 di 30d: (2x - 1)/(x^2 - 9x + 20),
 * con primitiva 9 ln|x - 5| - 7 ln|x - 4|. */
#define QUOTIENT "(/ (- (* 2 x) 1) (+ (- (^ x 2) (* 9 x)) 20))"
#define QUOTIENT_PRIMITIVE "(- (* 9 (lnabs (- x 5))) (* 7 (lnabs (- x 4))))"

static const Case cases[] = {
    /* Derivate di ordine fisso. */
    {"the derivative of x^2 e^x", CK_ACCEPTED, "", DERIVATIVE("1", "(* (^ x 2) (exp x))", "(* (exp x) (+ (^ x 2) (* 2 x)))")},
    {"the second derivative of a rational function", CK_ACCEPTED, "",
     DERIVATIVE("2", "(/ (+ x 1) (+ (^ x 2) 1))", "(/ (- (- (+ (* 2 (^ x 3)) (* 6 (^ x 2))) (* 6 x)) 2) (^ (+ (^ x 2) 1) 3))")},
    {"the derivative of the logarithm of a quotient", CK_ACCEPTED, "",
     DERIVATIVE("1", "(ln (/ (- (^ x 2) 9) (- 1 (* 4 x))))", "(/ (- (+ (* -4 (^ x 2)) (* 2 x)) 36) (* (- (^ x 2) 9) (- 1 (* 4 x))))")},
    {"a logarithm in base 2/3, and log_{2/3} e", CK_ACCEPTED, "",
     DERIVATIVE("1", "(* (^ x 4) (/ (ln x) (ln (/ 2 3))))",
                "(* (^ x 3) (+ (/ (ln (^ x 4)) (ln (/ 2 3))) (/ (ln (exp 1)) (ln (/ 2 3)))))")},
    {"x^2 tan x, with the double angle", CK_ACCEPTED, "",
     DERIVATIVE("1", "(* (^ x 2) (/ (sin x) (cos x)))", "(/ (* x (+ (sin (* 2 x)) x)) (^ (cos x) 2))")},
    {"the fourth derivative of sin 4x", CK_ACCEPTED, "", DERIVATIVE("4", "(sin (* 4 x))", "(* 256 (sin (* 4 x)))")},
    {"tan(x/2), with the half angle", CK_ACCEPTED, "", DERIVATIVE("1", "(/ (sin (/ x 2)) (cos (/ x 2)))", "(/ 1 (+ 1 (cos x)))")},
    {"ln cot(1/x), with the angle 2/x", CK_ACCEPTED, "",
     DERIVATIVE("1", "(ln (/ (cos (/ 1 x)) (sin (/ 1 x))))", "(/ 2 (* (^ x 2) (sin (/ 2 x))))")},
    {"2^x", CK_ACCEPTED, "", DERIVATIVE("1", "(^ 2 x)", "(* (ln 2) (^ 2 x))")},
    {"x^x, as e^{x ln x}", CK_ACCEPTED, "", DERIVATIVE("1", "(^ x x)", "(* (^ x x) (+ (ln x) 1))")},
    {"arctan of a rational function", CK_ACCEPTED, "",
     DERIVATIVE("1", "(atan (/ (- (* 3 x) (^ x 3)) (- 1 (* 3 (^ x 2)))))", "(/ 3 (+ 1 (^ x 2)))")},
    {"the square root", CK_ACCEPTED, "", DERIVATIVE("1", "(root 2 x)", "(/ 1 (* 2 (root 2 x)))")},
    {"the second derivative of the cube root, with the root of x^5", CK_ACCEPTED, "",
     DERIVATIVE("2", "(root 3 x)", "(neg (/ 2 (* 9 (root 3 (^ x 5)))))")},
    {"a power with a fractional exponent", CK_ACCEPTED, "", DERIVATIVE("1", "(^ x (/ 3 2))", "(* (/ 3 2) (root 2 x))")},
    {"x|x|, with the sign of x", CK_ACCEPTED, "", DERIVATIVE("2", "(* x (abs x))", "(* 2 (/ (abs x) x))")},
    {"e^{sin x}, twice", CK_ACCEPTED, "", DERIVATIVE("2", "(exp (sin x))", "(* (- (^ (cos x) 2) (sin x)) (exp (sin x)))")},

    /* Derivate n-esime. */
    {"the n-th derivative of 1/(1 + x)", CK_ACCEPTED, "",
     DERIVATIVE("n base 0", "(/ 1 (+ 1 x))", "(/ (* (^ -1 n) (fact n)) (^ (+ 1 x) (+ n 1)))")},
    {"the n-th derivative of e^{-2x}", CK_ACCEPTED, "", DERIVATIVE("n base 0", "(exp (* -2 x))", "(* (* (^ -1 n) (^ 2 n)) (exp (* -2 x)))")},
    {"the n-th derivative of sin x", CK_ACCEPTED, "", DERIVATIVE("n base 0", "(sin x)", "(sin (+ x (/ (* n (pi)) 2)))")},
    {"the n-th derivative of 3^x", CK_ACCEPTED, "", DERIVATIVE("n base 0", "(^ 3 x)", "(* (^ (ln 3) n) (^ 3 x))")},
    {"the n-th derivative of ln x, from n = 1", CK_ACCEPTED, "",
     DERIVATIVE("n base 1", "(ln x)", "(/ (* (^ -1 (- n 1)) (fact (- n 1))) (^ x n))")},

    /* Primitive. */
    {"ln|x| is an antiderivative of 1/x", CK_ACCEPTED, "", PRIMITIVE("(/ 1 x)", "(lnabs x)")},
    {"partial fractions", CK_ACCEPTED, "", PRIMITIVE(QUOTIENT, QUOTIENT_PRIMITIVE)},
    {"a logarithm of a quotient with an absolute value", CK_ACCEPTED, "",
     PRIMITIVE("(/ 1 (* (^ x 3) (+ 1 (^ x 2))))", "(- (ln (/ (root 2 (+ 1 (^ x 2))) (abs x))) (/ 1 (* 2 (^ x 2))))")},
    {"square, cube and sixth roots of x together", CK_ACCEPTED, "",
     PRIMITIVE("(/ 1 (+ (root 2 x) (root 3 x)))",
               "(- (+ (- (* 2 (root 2 x)) (* 3 (root 3 x))) (* 6 (root 6 x))) (* 6 (ln (+ (root 6 x) 1))))")},
    {"t = tan(x/2)", CK_ACCEPTED, "",
     PRIMITIVE("(/ 1 (- (* 5 (cos x)) 3))",
               "(- (* (/ 1 4) (lnabs (+ (* 2 (/ (sin (/ x 2)) (cos (/ x 2)))) 1))) (* (/ 1 4) (lnabs (- (* 2 (/ (sin (/ x 2)) (cos (/ x 2)))) 1))))")},
    {"e^x sin x, by parts twice", CK_ACCEPTED, "",
     PRIMITIVE("(* (exp x) (sin x))", "(* (/ 1 2) (* (exp x) (- (sin x) (cos x))))")},

    /* Integrali definiti. */
    {"the integral of x^2 from 0 to 1", CK_ACCEPTED, "", INTEGRAL("0", "1", "(^ x 2)", "(/ (^ x 3) 3)", "(/ 1 3)")},
    {"the same with the bounds exchanged", CK_ACCEPTED, "", INTEGRAL("1", "0", "(^ x 2)", "(/ (^ x 3) 3)", "(neg (/ 1 3))")},
    {"a rational function between its poles, with logarithms", CK_ACCEPTED, "",
     INTEGRAL("2", "3", QUOTIENT, QUOTIENT_PRIMITIVE, "(- (* 16 (ln 2)) (* 9 (ln 3)))")},
    {"sin^2 from 0 to 2 pi", CK_ACCEPTED, "",
     INTEGRAL("0", "(* 2 (pi))", "(^ (sin x) 2)", "(- (/ x 2) (/ (sin (* 2 x)) 4))", "(pi)")},
    {"arctan, and the value pi/4", CK_ACCEPTED, "", INTEGRAL("0", "1", "(/ 1 (+ (^ x 2) 1))", "(atan x)", "(/ (pi) 4)")},
    {"from 1 to e", CK_ACCEPTED, "", INTEGRAL("1", "(exp 1)", "(/ 1 x)", "(lnabs x)", "(ln (exp 1))")},
    {"x e^{-3x} from -1 to 0", CK_ACCEPTED, "",
     INTEGRAL("-1", "0", "(* x (exp (* -3 x)))", "(neg (* (/ (exp (* -3 x)) 9) (+ (* 3 x) 1)))", "(- (neg (/ 1 9)) (* (/ 2 9) (exp 3)))")},

    /* Respinti: ciò che il certificato afferma è falso, o è scritto male. */
    {"a wrong derivative", CK_REJECTED, "non è quella del certificato", DERIVATIVE("1", "(^ x 2)", "(* 3 x)")},
    {"a wrong derivative with an exponential", CK_REJECTED, "non è quella del certificato",
     DERIVATIVE("1", "(* (^ x 2) (exp x))", "(* (exp x) (+ (^ x 2) (* 3 x)))")},
    {"a missing term", CK_REJECTED, "non è quella del certificato", DERIVATIVE("1", "(+ (^ x 2) (sin x))", "(* 2 x)")},
    {"x|x| with a wrong first derivative: never accepted, not rejected", CK_UNSUPPORTED, "scelta dei segni",
     DERIVATIVE("1", "(* x (abs x))", "(abs x)")},
    {"a function that is not the one differentiated", CK_REJECTED, "non è quella del certificato",
     DERIVATIVE("1", "(* (^ x 3) (exp x))", "(* (exp x) (+ (^ x 2) (* 2 x)))")},
    {"the wrong order", CK_REJECTED, "non è quella del certificato", DERIVATIVE("3", "(sin (* 4 x))", "(* 256 (sin (* 4 x)))")},
    {"an antiderivative up to the sign", CK_REJECTED, "non è la funzione integranda", PRIMITIVE("(/ 1 x)", "(neg (lnabs x))")},
    {"an antiderivative with the coefficients exchanged", CK_REJECTED, "non è la funzione integranda",
     PRIMITIVE(QUOTIENT, "(- (* 7 (lnabs (- x 5))) (* 5 (lnabs (- x 4))))")},
    {"an integral across a pole", CK_REJECTED, "non è definita", INTEGRAL("-1", "1", "(/ 1 (^ x 2))", "(neg (/ 1 x))", "-2")},
    {"an integral up to a pole", CK_REJECTED, "non è definita", INTEGRAL("0", "1", "(/ 1 x)", "(lnabs x)", "0")},
    {"a logarithm of a negative number on all the interval", CK_REJECTED, "non è definita",
     INTEGRAL("2", "3", "(/ 1 (- x 4))", "(ln (- x 4))", "(neg (ln 2))")},
    {"a wrong rational value", CK_REJECTED, "F(b) - F(a)", INTEGRAL("0", "1", "(^ x 2)", "(/ (^ x 3) 3)", "(/ 1 2)")},
    {"a wrong value with logarithms", CK_REJECTED, "F(b) - F(a)", INTEGRAL("2", "3", QUOTIENT, QUOTIENT_PRIMITIVE, "(- (* 16 (ln 2)) (* 8 (ln 3)))")},
    {"a wrong value with pi", CK_REJECTED, "F(b) - F(a)", INTEGRAL("0", "1", "(/ 1 (+ (^ x 2) 1))", "(atan x)", "(/ (pi) 3)")},
    {"the n-th derivative right only in the base case", CK_REJECTED, "n + 1",
     DERIVATIVE("n base 0", "(/ 1 (+ 1 x))", "(/ (* (^ -1 n) (* (fact n) (fact n))) (^ (+ 1 x) (+ n 1)))")},
    {"the n-th derivative wrong only in the base case", CK_REJECTED, "caso base",
     DERIVATIVE("n base 0", "(exp (* 2 x))", "(* 2 (* (^ 2 n) (exp (* 2 x))))")},
    {"the order letter in the function", CK_REJECTED, "lettera dell'ordine", DERIVATIVE("n base 0", "(^ x n)", "(^ x n)")},
    {"a missing voice", CK_REJECTED, "manca", "certificate derivative\nvariable x\norder 1\nderivative (* 2 x)\nend\n"},
    {"the variable in a bound", CK_REJECTED, "contengono la variabile", INTEGRAL("0", "x", "(^ x 2)", "(/ (^ x 3) 3)", "(/ (^ x 3) 3)")},

    /* Il valore e gli estremi devono essere numeri reali. Il primo l'ha
     * trovato fuzz_calculus_certificates.py: per l'algebra del verificatore
     * ln(-e) e ln(e) hanno la stessa scrittura, e il valore vero è 1. */
    {"a value that is the logarithm of a negative number", CK_REJECTED, "non è un numero reale",
     INTEGRAL("1", "(exp 1)", "(/ 1 x)", "(lnabs x)", "(ln (neg (exp 1)))")},
    {"a value that is the logarithm of a negative rational", CK_REJECTED, "non è un numero reale",
     INTEGRAL("0", "1", "(^ x 2)", "(/ (^ x 3) 3)", "(+ (/ 1 3) (ln (neg 1)))")},
    {"a value divided by zero", CK_REJECTED, "non è un numero reale", INTEGRAL("0", "1", "(^ x 2)", "(/ (^ x 3) 3)", "(/ 1 (- 3 3))")},
    {"a bound that is the logarithm of a negative number", CK_REJECTED, "non è un numero reale",
     INTEGRAL("1", "(ln (neg (exp 1)))", "(^ x 2)", "(/ (^ x 3) 3)", "0")},
    {"the same integral with the value written as ln e", CK_ACCEPTED, "", INTEGRAL("1", "(exp 1)", "(/ 1 x)", "(lnabs x)", "(ln (exp 1))")},

    /* Non controllabili: non devono mai diventare un sì. */
    {"the square root of x^2, which is |x|", CK_UNSUPPORTED, "non so stabilire che la differenza è zero",
     DERIVATIVE("1", "(root 2 (^ x 2))", "(/ x (abs x))")},
    {"a choice of signs that happens nowhere: |x| = -x where the square root of x is defined", CK_UNSUPPORTED, "scelta dei segni",
     PRIMITIVE("(/ (abs x) (root 2 x))", "(* (/ 2 3) (^ (root 2 x) 3))")},
    {"an absolute value in a definite integral", CK_UNSUPPORTED, "valore assoluto", INTEGRAL("0", "3", "(abs (- x 1))", "(/ (^ x 2) 2)", "(/ 5 2)")},
    {"a tangent in a definite integral", CK_UNSUPPORTED, "non è un polinomio in x",
     INTEGRAL("0", "1", "(/ (sin x) (cos x))", "(neg (ln (cos x)))", "(neg (ln (cos 1)))")},
    {"a word the checker does not know", CK_UNSUPPORTED, "operatore", DERIVATIVE("1", "(asin x)", "(/ 1 (root 2 (- 1 (^ x 2))))")},
    {"an order too high", CK_UNSUPPORTED, "ordine", DERIVATIVE("9", "(exp x)", "(exp x)")},
    {"a bound the checker cannot place", CK_UNSUPPORTED, "estremo",
     INTEGRAL("1", "(exp (/ 3 2))", "(/ 1 x)", "(lnabs x)", "(/ 3 2)")},
    {"an exponential in an induction certificate", CK_UNSUPPORTED, "operatore",
     "certificate induction\nvariable n\nbase 1\nstatement (= (exp n) (exp n))\nfactor 0\nend\n"},
    {"an absolute value in an induction certificate", CK_UNSUPPORTED, "induzione",
     "certificate induction\nvariable n\nbase 1\nstatement (= (ln (abs n)) (ln n))\nfactor 1\nend\n"},
    {"a brick with a root", CK_UNSUPPORTED, "brick",
     "certificate derivative\nvariable x\norder 1\nfunction (root 3 x)\nderivative (/ 1 (* 3 (root 3 (^ x 2))))\nbrick (+ (root 2 x) 1)\nend\n"},
};

static int checks, failures;

/* Registra l'esito di un controllo e stampa quelli falliti. */
static void check(int passed, const char *description) {
    checks++;
    if (passed) return;
    failures++;
    printf("  FAILED: %s\n", description);
}

/* Ritorna il nome inglese di una risposta, per i messaggi. */
static const char *answer_name(CkAnswer answer) {
    return answer == CK_ACCEPTED ? "accepted" : answer == CK_REJECTED ? "rejected" : "unsupported";
}

int main(void) {
    char message[512];
    const char *why;

    /* La tabella: risposta attesa, e il motivo per chi non è accettato. */
    for (size_t i = 0; i < sizeof cases / sizeof *cases; i++) {
        const char *text = cases[i].text;
        CkAnswer answer = ck_check(&text, &why, 0);
        snprintf(message, sizeof message, "\"%s\" should be %s but is %s (%s)", cases[i].name,
                 answer_name(cases[i].expected), answer_name(answer), why);
        check(answer == cases[i].expected, message);
        snprintf(message, sizeof message, "\"%s\" should mention \"%s\" as its reason, not \"%s\"", cases[i].name, cases[i].why, why);
        check(answer != cases[i].expected || strstr(why, cases[i].why) != NULL, message);
    }

    /* Un certificato non lascia tracce nel successivo: dopo uno che ha
     * imparato l'unità 2 e le radici sesta, uno senza riparte da capo. */
    const char *two = PRIMITIVE("(/ 1 (+ (root 2 x) (root 3 x)))",
                                "(- (+ (- (* 2 (root 2 x)) (* 3 (root 3 x))) (* 6 (root 6 x))) (* 6 (ln (+ (root 6 x) 1))))")
                      DERIVATIVE("2", "(root 3 x)", "(neg (/ 2 (* 9 (root 3 (^ x 5)))))");
    check(ck_check(&two, &why, 0) == CK_ACCEPTED, "the first of two certificates should be accepted");
    check(ck_check(&two, &why, 0) == CK_ACCEPTED, "the second of two certificates should be read next, and accepted");

    printf("%s: %d of %d derivative, antiderivative and integral certificate checks passed\n", failures ? "FAIL" : "PASS", checks - failures, checks);
    return failures ? 1 : 0;
}
