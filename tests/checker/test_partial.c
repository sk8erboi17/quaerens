/* Test del verificatore sui certificati delle derivate parziali (capitolo 18
 * di checker.c), da solo: qui il motore non c'entra.
 *
 * Compilazione ed esecuzione, dalla cartella principale del progetto:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/checker checker.c && \
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_partial tests/checker/test_partial.c && /tmp/test_partial
 *
 * Come in test_checker.c, il primo comando fa parte del test: checker.c si
 * compila da solo, e questo file include soltanto checker.c.
 *
 * I certificati accettati sono quelli che il motore scrive (35e di main.c),
 * o della stessa forma; gli altri ne sono alterazioni. Ognuno ha la risposta
 * attesa e, se non è accettato, un pezzo del motivo, così un certificato
 * respinto per la ragione sbagliata non passa per buono:
 *
 *   - accettati: derivate prime, seconde e miste di polinomi, frazioni,
 *     esponenziali, logaritmi, seni, radici; un termine che ne nomina uno
 *     precedente; valori in un punto razionale, in un punto con pi greco, e
 *     con una condizione di esistenza che contiene pi greco; un termine di
 *     ordine zero (la funzione, o il suo valore); più tesi; una tesi con più
 *     termini (l'equazione di Laplace);
 *   - respinti: una formula sbagliata, la lettera di derivazione cambiata,
 *     l'ordine delle lettere cambiato dove conta, una tesi sbagliata, un
 *     valore sbagliato, un punto in cui la funzione non è definita (un
 *     logaritmo di un numero negativo, una divisione per zero), un nome di
 *     termine usato come lettera o prima della sua voce, certificati scritti
 *     male;
 *   - non controllabili: un punto in cui un radicando vale zero, una
 *     relazione che il verificatore non conosce, un valore assoluto con un
 *     punto, una coordinata che è un parametro, una tesi che non è
 *     un'uguaglianza, troppe lettere di derivazione. Non devono mai
 *     diventare un "accettato".
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

/* Un termine senza punto e uno con il punto, e un certificato con un solo
 * termine A e una sola tesi. */
#define TERM(name, of, by, is) "term " name "\nof " of "\nby" by "\nis " is "\n"
#define TERM_AT(name, of, by, is, at) "term " name "\nof " of "\nby" by "\nis " is "\nat " at "\n"
#define PARTIAL(terms, claims) "certificate partial\n" terms claims "end\n"
#define CLAIM(l, r) "statement (= " l " " r ")\n"

#define CUBIC "(- (+ (^ x 3) (^ y 3)) (* (* 3 x) y))"
#define LOG "(ln (+ (^ x 2) (^ y 2)))"
#define LOG_XX "(/ (- (* 2 (^ y 2)) (* 2 (^ x 2))) (^ (+ (^ x 2) (^ y 2)) 2))"
#define LOG_YY "(/ (- (* 2 (^ x 2)) (* 2 (^ y 2))) (^ (+ (^ x 2) (^ y 2)) 2))"

static const Case cases[] = {
    /* Formule. */
    {"a first derivative of a polynomial", CK_ACCEPTED, "", PARTIAL(TERM("A", "(* (^ x 2) y)", " x", "(* (* 2 x) y)"), CLAIM("A", "(* (* 2 x) y)"))},
    {"a mixed derivative, in both orders", CK_ACCEPTED, "",
     PARTIAL(TERM("A", "(* (^ x 2) (^ y 3))", " x y", "(* (* 6 x) (^ y 2))") TERM("B", "(* (^ x 2) (^ y 3))", " y x", "(* (* 6 x) (^ y 2))"), CLAIM("A", "B"))},
    {"the mixed derivative of e^{xy}", CK_ACCEPTED, "",
     PARTIAL(TERM("A", "(exp (* x y))", " x y", "(+ (exp (* x y)) (* (* x y) (exp (* x y))))"), CLAIM("A", "(* (+ 1 (* x y)) (exp (* x y)))"))},
    {"the Laplace equation for ln(x^2 + y^2)", CK_ACCEPTED, "", PARTIAL(TERM("A", LOG, " x x", LOG_XX) TERM("B", LOG, " y y", LOG_YY), CLAIM("(+ A B)", "0"))},
    {"a derivative with a parameter", CK_ACCEPTED, "", PARTIAL(TERM("A", "(* a (sin (* b x)))", " x", "(* (* a b) (cos (* b x)))"), CLAIM("A", "(* (* a b) (cos (* b x)))"))},
    {"a derivative with respect to a letter that is not there", CK_ACCEPTED, "", PARTIAL(TERM("A", "(* (^ x 2) y)", " z", "0"), CLAIM("A", "0"))},
    {"a square root", CK_ACCEPTED, "",
     PARTIAL(TERM("A", "(root 2 (+ (^ x 2) (^ y 2)))", " y", "(/ y (root 2 (+ (^ x 2) (^ y 2))))"), CLAIM("A", "(/ y (root 2 (+ (^ x 2) (^ y 2))))"))},
    {"a third derivative in three letters", CK_ACCEPTED, "", PARTIAL(TERM("A", "(+ (* (* x y) z) (* (exp x) (sin z)))", " z y x", "1"), CLAIM("A", "1"))},
    {"a term of order zero", CK_ACCEPTED, "", PARTIAL(TERM("A", "(* (^ x 2) y)", "", "(* y (^ x 2))"), CLAIM("(* 2 A)", "(* (* 2 y) (^ x 2))"))},
    {"a term that names an earlier term", CK_ACCEPTED, "",
     PARTIAL(TERM("A", "(* (^ x 2) y)", " x", "(* (* 2 x) y)") TERM("B", "(* A y)", " y", "(* (* 4 x) y)"), CLAIM("B", "(* (* 4 x) y)"))},
    {"terms without a thesis", CK_ACCEPTED, "", PARTIAL(TERM("A", "(* (^ x 2) y)", " x", "(* (* 2 x) y)"), "")},
    {"a thesis without terms", CK_ACCEPTED, "", PARTIAL("", CLAIM("(* 2 (+ x y))", "(+ (* 2 x) (* 2 y))"))},

    /* Punti. */
    {"a gradient and a Hessian matrix at a critical point", CK_ACCEPTED, "",
     PARTIAL(TERM_AT("A", CUBIC, " x", "(- (* 3 (^ x 2)) (* 3 y))", "x 1 y 1") TERM_AT("B", CUBIC, " y", "(- (* 3 (^ y 2)) (* 3 x))", "x 1 y 1")
             TERM_AT("C", CUBIC, " x x", "(* 6 x)", "x 1 y 1") TERM_AT("D", CUBIC, " x y", "-3", "x 1 y 1"),
             CLAIM("A", "0") CLAIM("B", "0") CLAIM("C", "6") CLAIM("D", "(neg 3)"))},
    {"a value at a point with fractions", CK_ACCEPTED, "",
     PARTIAL(TERM_AT("A", "(neg (ln (- (- 1 x) y)))", " x", "(/ 1 (- (- 1 x) y))", "x (/ 1 3) y (/ 1 3)"), CLAIM("A", "3"))},
    {"a logarithm of a product of two negative numbers", CK_ACCEPTED, "", PARTIAL(TERM_AT("A", "(ln (* x y))", " x", "(/ 1 x)", "x -1 y -1"), CLAIM("A", "(neg 1)"))},
    {"a point with pi", CK_ACCEPTED, "", PARTIAL(TERM_AT("A", "(* x (sin y))", " y", "(* x (cos y))", "x 1 y (/ (pi) 2)"), CLAIM("A", "0"))},
    {"a condition of existence with pi", CK_ACCEPTED, "",
     PARTIAL(TERM_AT("A", "(ln (- x 3))", " x", "(/ 1 (- x 3))", "x (pi)"), CLAIM("A", "(/ 1 (- (pi) 3))"))},
    {"the value of a function at a point", CK_ACCEPTED, "", PARTIAL(TERM_AT("A", "(ln (+ x y))", "", "(ln (+ x y))", "x 1 y 1"), CLAIM("A", "(ln 2)"))},
    {"a point for some letters only", CK_ACCEPTED, "", PARTIAL(TERM_AT("A", "(* (^ x 2) y)", " x", "(* (* 2 x) y)", "x 3"), CLAIM("A", "(* 6 y)"))},
    {"a point term inside a later function", CK_ACCEPTED, "",
     PARTIAL(TERM_AT("A", "(* (^ x 2) y)", " x", "(* (* 2 x) y)", "x 1 y 2") TERM("B", "(* A (^ x 2))", " x", "(* 8 x)"), CLAIM("B", "(* 8 x)"))},

    /* Formule e tesi sbagliate. */
    {"a wrong formula", CK_REJECTED, "non è la formula", PARTIAL(TERM("A", "(* (^ x 2) y)", " x", "(* 2 x)"), CLAIM("A", "(* 2 x)"))},
    {"the letter of differentiation changed", CK_REJECTED, "non è la formula", PARTIAL(TERM("A", "(* (^ x 2) y)", " y", "(* (* 2 x) y)"), CLAIM("A", "(* (* 2 x) y)"))},
    {"one differentiation dropped", CK_REJECTED, "non è la formula",
     PARTIAL(TERM("A", "(* (^ x 2) (^ y 3))", " x", "(* (* 6 x) (^ y 2))"), CLAIM("A", "(* (* 6 x) (^ y 2))"))},
    {"a right formula and a wrong thesis", CK_REJECTED, "due membri", PARTIAL(TERM("A", "(* (^ x 2) y)", " x", "(* (* 2 x) y)"), CLAIM("A", "(* (* 3 x) y)"))},
    {"a wrong second thesis", CK_REJECTED, "due membri",
     PARTIAL(TERM("A", "(* (^ x 2) y)", " x", "(* (* 2 x) y)"), CLAIM("A", "(* (* 2 x) y)") CLAIM("A", "(* (* 2 y) y)"))},
    {"the Laplace equation with a wrong sign", CK_REJECTED, "due membri", PARTIAL(TERM("A", LOG, " x x", LOG_XX) TERM("B", LOG, " y y", LOG_YY), CLAIM("(- A B)", "0"))},
    {"a wrong formula for a later term", CK_REJECTED, "non è la formula",
     PARTIAL(TERM("A", "(* (^ x 2) y)", " x", "(* (* 2 x) y)") TERM("B", "(* A y)", " y", "(* (* 2 x) y)"), CLAIM("B", "(* (* 2 x) y)"))},
    {"a wrong value at a point", CK_REJECTED, "due membri", PARTIAL(TERM_AT("A", "(* (^ x 2) y)", " x", "(* (* 2 x) y)", "x 1 y 2"), CLAIM("A", "5"))},
    {"a wrong value at a point with pi", CK_REJECTED, "due membri", PARTIAL(TERM_AT("A", "(* x (sin y))", " y", "(* x (cos y))", "x 1 y (/ (pi) 2)"), CLAIM("A", "1"))},
    {"a wrong thesis without terms", CK_REJECTED, "due membri", PARTIAL("", CLAIM("1", "2"))},

    /* Punti in cui la funzione non c'è. */
    {"a logarithm of a negative number at the point", CK_REJECTED, "in un punto", PARTIAL(TERM_AT("A", "(ln (* x y))", " x", "(/ 1 x)", "x -1 y 1"), CLAIM("A", "(neg 1)"))},
    {"a division by zero at the point", CK_REJECTED, "in un punto",
     PARTIAL(TERM_AT("A", "(/ 1 (- x y))", " x", "(neg (/ 1 (^ (- x y) 2)))", "x 1 y 1"), CLAIM("A", "0"))},
    {"a formula that is not defined at the point", CK_REJECTED, "in un punto", PARTIAL(TERM_AT("A", "(^ x 2)", " x", "(/ (* 2 (^ x 2)) x)", "x 0"), CLAIM("A", "0"))},
    {"a logarithm that is negative because of pi", CK_REJECTED, "in un punto",
     PARTIAL(TERM_AT("A", "(ln (- 3 x))", " x", "(/ 1 (- x 3))", "x (pi)"), CLAIM("A", "(/ 1 (- (pi) 3))"))},
    {"an earlier term that does not exist at the point", CK_REJECTED, "in un punto",
     PARTIAL(TERM("A", "(ln x)", " x", "(/ 1 x)") TERM_AT("B", "(* A y)", " y", "(/ 1 x)", "x -1 y 2"), CLAIM("B", "(neg 1)"))},

    /* I nomi. */
    {"a term named in its own formula", CK_REJECTED, "nome di un termine", PARTIAL(TERM("A", "(* (^ x 2) y)", " x", "(* (* 2 A) y)"), CLAIM("A", "0"))},
    {"a term named like a letter of its function", CK_REJECTED, "nome di un termine", PARTIAL(TERM("x", "(* (^ x 2) y)", " x", "(* (* 2 x) y)"), CLAIM("x", "0"))},
    {"a term named before its entry", CK_REJECTED, "nome di un termine",
     PARTIAL(TERM("A", "(* B y)", " y", "0") TERM("B", "(* (^ x 2) y)", " x", "(* (* 2 x) y)"), CLAIM("A", "0"))},
    {"two terms with the same name", CK_REJECTED, "nome di un termine",
     PARTIAL(TERM("A", "(* (^ x 2) y)", " x", "(* (* 2 x) y)") TERM("A", "(* (^ x 2) y)", " y", "(^ x 2)"), CLAIM("A", "(^ x 2)"))},
    {"a term named like a letter of differentiation", CK_REJECTED, "nome di un termine",
     PARTIAL(TERM("y", "(^ x 2)", " x", "(* 2 x)") TERM("B", "(^ x 2)", " y", "0"), CLAIM("B", "0"))},
    {"a term named like a letter of a point", CK_REJECTED, "nome di un termine",
     PARTIAL(TERM("y", "(^ x 2)", " x", "(* 2 x)") TERM_AT("B", "(^ x 2)", " x", "(* 2 x)", "x 1 y 2"), CLAIM("B", "2"))},

    /* Certificati scritti male. */
    {"no formula", CK_REJECTED, "scritto male", "certificate partial\nterm A\nof (* (^ x 2) y)\nby x\nstatement (= A 0)\nend\n"},
    {"no letters line", CK_REJECTED, "scritto male", "certificate partial\nterm A\nof (* (^ x 2) y)\nis (* (* 2 x) y)\nstatement (= A 0)\nend\n"},
    {"a number among the letters", CK_REJECTED, "scritto male", "certificate partial\nterm A\nof (* (^ x 2) y)\nby x 2\nis (* (* 2 x) y)\nend\n"},
    {"a point without coordinates", CK_REJECTED, "senza coordinate", "certificate partial\nterm A\nof (* (^ x 2) y)\nby x\nis (* (* 2 x) y)\nat\nend\n"},
    {"no end", CK_REJECTED, "scritto male", "certificate partial\nterm A\nof (* (^ x 2) y)\nby x\nis (* (* 2 x) y)\nstatement (= A (* (* 2 x) y))\n"},
    {"nothing stated", CK_REJECTED, "non afferma nulla", "certificate partial\nend\n"},
    {"a long name", CK_REJECTED, "nome di un termine", "certificate partial\nterm AB\nof x\nby x\nis 1\nend\n"},
    {"a point with two values for one letter", CK_REJECTED, "due valori", PARTIAL(TERM_AT("A", "(* (^ x 2) y)", " x", "(* (* 2 x) y)", "x 1 x 2"), CLAIM("A", "(* 4 y)"))},

    /* Ciò che il verificatore non sa controllare. */
    {"a radicand that is zero at the point", CK_UNSUPPORTED, "in un punto",
     PARTIAL(TERM_AT("A", "(root 2 (- (+ (^ x 2) (^ y 2)) 1))", " x", "(/ x (root 2 (- (+ (^ x 2) (^ y 2)) 1)))", "x 1 y 0"), CLAIM("A", "1"))},
    {"a wrong mixed derivative of e^{xy}", CK_UNSUPPORTED, "relazioni",
     PARTIAL(TERM("A", "(exp (* x y))", " x y", "(+ (exp (* x y)) (* x (exp (* x y))))"), CLAIM("A", "(+ (exp (* x y)) (* x (exp (* x y))))"))},
    {"an absolute value with a point", CK_UNSUPPORTED, "valore assoluto", PARTIAL(TERM_AT("A", "(* (abs x) y)", " y", "(abs x)", "x -2 y 1"), CLAIM("A", "2"))},
    {"a coordinate that is a parameter", CK_UNSUPPORTED, "in un punto", PARTIAL(TERM_AT("A", "(ln (* x y))", " x", "(/ 1 x)", "x a y 1"), CLAIM("A", "(/ 1 a)"))},
    {"a thesis that is not an equality", CK_UNSUPPORTED, "uguaglianza", PARTIAL(TERM("A", "(* (^ x 2) y)", " x", "(* (* 2 x) y)"), "statement (< A 0)\n")},
    {"nine letters of differentiation", CK_UNSUPPORTED, "ordine", PARTIAL(TERM("A", "(^ x 9)", " x x x x x x x x x", "362880"), CLAIM("A", "362880"))},
    {"an operator the verifier does not know", CK_UNSUPPORTED, "operatore", PARTIAL(TERM("A", "(sinh x)", " x", "(cosh x)"), CLAIM("A", "(cosh x)"))},
};

static int checks, failures;

/* Annota l'esito di un controllo e stampa il messaggio se è fallito. */
static void check(int passed, const char *message) {
    checks++;
    if (passed) return;
    failures++;
    printf("  %s\n", message);
}

/* Ritorna il nome di una risposta, per i messaggi. */
static const char *answer_name(CkAnswer answer) {
    return answer == CK_ACCEPTED ? "ACCEPTED" : answer == CK_REJECTED ? "REJECTED" : "UNSUPPORTED";
}

int main(void) {
    char message[640];
    const char *why;

    /* La tabella: risposta attesa, e il motivo per chi non è accettato. */
    for (size_t i = 0; i < sizeof cases / sizeof *cases; i++) {
        const char *text = cases[i].text;
        CkAnswer answer = ck_check(&text, &why, 0);
        snprintf(message, sizeof message, "\"%s\" should be %s but is %s (%s)", cases[i].name, answer_name(cases[i].expected), answer_name(answer), why);
        check(answer == cases[i].expected, message);
        snprintf(message, sizeof message, "\"%s\" should mention \"%s\" as its reason, not \"%s\"", cases[i].name, cases[i].why, why);
        check(answer != cases[i].expected || strstr(why, cases[i].why) != NULL, message);
    }

    /* Un certificato non lascia tracce nel successivo: dopo uno con un punto
     * e uno respinto, il terzo riparte da capo, e viceversa. */
    const char *three = PARTIAL(TERM_AT("A", "(* (^ x 2) y)", " x", "(* (* 2 x) y)", "x 1 y 2"), CLAIM("A", "4"))
                        PARTIAL(TERM("A", "(* (^ x 2) y)", " x", "(* 2 x)"), CLAIM("A", "(* 2 x)"))
                        PARTIAL(TERM("A", "(* (^ x 2) y)", " x", "(* (* 2 x) y)"), CLAIM("A", "(* (* 2 x) y)"))
                        "certificate derivative\nvariable x\norder 1\nfunction (* x (exp x))\nderivative (* (+ x 1) (exp x))\nend\n";
    check(ck_check(&three, &why, 0) == CK_ACCEPTED, "the first of four certificates should be accepted");
    check(ck_check(&three, &why, 0) == CK_REJECTED, "the second of four certificates should be rejected");
    check(ck_check(&three, &why, 0) == CK_ACCEPTED, "the third of four certificates should be read next, and accepted");
    check(ck_check(&three, &why, 0) == CK_ACCEPTED, "a derivative certificate after three partial ones should be accepted");

    printf("%s: %d of %d partial derivative certificate checks passed\n", failures ? "FAIL" : "PASS", checks - failures, checks);
    return failures ? 1 : 0;
}
