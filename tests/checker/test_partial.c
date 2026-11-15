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
 *     un'uguaglianza né un confronto, troppe lettere di derivazione. Non
 *     devono mai diventare un "accettato".
 *
 * I confronti tra numeri (<, <= e neq: le condizioni del secondo ordine in
 * un punto critico):
 *
 *   - accettati: il determinante dell'hessiana positivo con f_xx positivo,
 *     negativo, nullo (come uguaglianza e come due confronti larghi); il
 *     segno di una potenza di e e di una radice quadrata; pi greco ed e tra
 *     due razionali; un termine senza punto che è un numero;
 *   - respinti: una sella detta minimo, un minimo detto massimo, il
 *     determinante nullo detto positivo o diverso da zero, due numeri
 *     uguali detti diversi, un confronto giusto dopo una formula sbagliata o
 *     in un altro punto, un membro che non è definito;
 *   - non controllabili: una lettera rimasta, un numero di cui il
 *     verificatore non sa leggere il segno (la radice di 2 contro 1, e^2
 *     contro 7, un seno, pi greco contro 3,14159), un valore assoluto, un
 *     termine senza punto che non esiste ovunque.
 *
 * Prima di questi confronti la tesi "(< A 0)" con A = 2xy era il caso "una
 * tesi che non è un'uguaglianza": ora è "un confronto con una lettera
 * rimasta", sempre non controllabile, e il caso di prima ha una tesi che non
 * è nessuna delle due cose.
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

/* Un confronto, con la parola <, <= o neq. */
#define COMPARE(word, l, r) "statement (" word " " l " " r ")\n"

/* Le quattro derivate seconde di of in un punto, con i nomi C, D, E, F, e il
 * determinante della loro matrice. xy è la derivata mista, scritta due volte. */
#define HESSIAN(of, xx, xy, yy, at) TERM_AT("C", of, " x x", xx, at) TERM_AT("D", of, " x y", xy, at) TERM_AT("E", of, " y x", xy, at) TERM_AT("F", of, " y y", yy, at)
#define DET "(- (* C F) (* D E))"

#define CUBIC "(- (+ (^ x 3) (^ y 3)) (* (* 3 x) y))"
#define QUARTIC "(+ (^ x 4) (^ y 4))"
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
    {"a thesis that is neither an equality nor a comparison", CK_UNSUPPORTED, "uguaglianza",
     PARTIAL(TERM("A", "(* (^ x 2) y)", " x", "(* (* 2 x) y)"), "statement (and (= A 0) (= A 0))\n")},

    /* Confronti tra numeri: le condizioni del secondo ordine in un punto. */
    {"the second-order conditions of a minimum", CK_ACCEPTED, "", PARTIAL(HESSIAN(CUBIC, "(* 6 x)", "-3", "(* 6 y)", "x 1 y 1"), COMPARE("<", "0", DET) COMPARE("<", "0", "C"))},
    {"the determinant of a saddle point", CK_ACCEPTED, "", PARTIAL(HESSIAN(CUBIC, "(* 6 x)", "-3", "(* 6 y)", "x 0 y 0"), COMPARE("<", DET, "0"))},
    {"a determinant that is zero, as an equality and as two weak comparisons", CK_ACCEPTED, "",
     PARTIAL(HESSIAN(QUARTIC, "(* 12 (^ x 2))", "0", "(* 12 (^ y 2))", "x 0 y 0"), CLAIM(DET, "0") COMPARE("<=", DET, "0") COMPARE("<=", "0", DET))},
    {"two different numbers", CK_ACCEPTED, "", PARTIAL(HESSIAN(CUBIC, "(* 6 x)", "-3", "(* 6 y)", "x 1 y 1"), COMPARE("neq", DET, "26") COMPARE("<=", DET, "27"))},
    {"the sign of a power of e", CK_ACCEPTED, "",
     PARTIAL(TERM_AT("A", "(exp (+ x y))", " x x", "(exp (+ x y))", "x 1 y 1"), COMPARE("<", "0", "A") COMPARE("<", "(neg A)", "0") COMPARE("<", "0", "(/ (* 3 A) (exp 5))"))},
    {"the sign of a square root", CK_ACCEPTED, "",
     PARTIAL(TERM_AT("A", "(* x y)", " y", "x", "x (root 2 2) y 1"), COMPARE("<", "0", "A") COMPARE("<", "(* -3 (^ A 3))", "0") COMPARE("<", "1", "(^ A 2)"))},
    {"pi between 3 and 22/7", CK_ACCEPTED, "", PARTIAL(TERM_AT("A", "(* x y)", " y", "x", "x (pi) y 1"), COMPARE("<", "3", "A") COMPARE("<", "A", "(/ 22 7)"))},
    {"e between 27/10 and 3", CK_ACCEPTED, "", PARTIAL(TERM_AT("A", "(exp x)", " x", "(exp x)", "x 1"), COMPARE("<", "(/ 27 10)", "A") COMPARE("<=", "A", "3"))},
    {"a term without a point that is a number", CK_ACCEPTED, "", PARTIAL(TERM("A", "(* 3 x)", " x", "3"), COMPARE("<", "0", "A"))},
    {"a fraction with a negative denominator", CK_ACCEPTED, "",
     PARTIAL(TERM_AT("A", "(* x y)", " y", "x", "x (pi) y 1"), COMPARE("<", "(/ 1 (- 3 A))", "0") COMPARE("<", "0", "(/ (neg 1) (- 3 A))"))},
    {"two members with letters whose difference is a number", CK_ACCEPTED, "", PARTIAL("", COMPARE("<", "x", "(+ x 1)"))},

    /* Confronti falsi: il segno si sa, ed è l'altro. */
    {"a saddle point called a minimum", CK_REJECTED, "non vale", PARTIAL(HESSIAN(CUBIC, "(* 6 x)", "-3", "(* 6 y)", "x 0 y 0"), COMPARE("<", "0", DET))},
    {"a minimum called a maximum", CK_REJECTED, "non vale", PARTIAL(HESSIAN(CUBIC, "(* 6 x)", "-3", "(* 6 y)", "x 1 y 1"), COMPARE("<", "0", DET) COMPARE("<", "C", "0"))},
    {"a determinant that is zero called positive", CK_REJECTED, "non vale",
     PARTIAL(HESSIAN(QUARTIC, "(* 12 (^ x 2))", "0", "(* 12 (^ y 2))", "x 0 y 0"), COMPARE("<", "0", DET))},
    {"a determinant that is zero called different from zero", CK_REJECTED, "non vale",
     PARTIAL(HESSIAN(QUARTIC, "(* 12 (^ x 2))", "0", "(* 12 (^ y 2))", "x 0 y 0"), COMPARE("neq", DET, "0"))},
    {"two equal numbers called different", CK_REJECTED, "non vale", PARTIAL(HESSIAN(CUBIC, "(* 6 x)", "-3", "(* 6 y)", "x 1 y 1"), COMPARE("neq", DET, "27"))},
    {"a weak comparison the wrong way", CK_REJECTED, "non vale", PARTIAL(HESSIAN(CUBIC, "(* 6 x)", "-3", "(* 6 y)", "x 1 y 1"), COMPARE("<=", DET, "26"))},
    {"a power of e called negative", CK_REJECTED, "non vale", PARTIAL(TERM_AT("A", "(exp (+ x y))", " x x", "(exp (+ x y))", "x 1 y 1"), COMPARE("<", "A", "0"))},
    {"pi called greater than 22/7", CK_REJECTED, "non vale", PARTIAL(TERM_AT("A", "(* x y)", " y", "x", "x (pi) y 1"), COMPARE("<", "(/ 22 7)", "A"))},
    {"a fraction with a negative denominator called positive", CK_REJECTED, "non vale",
     PARTIAL(TERM_AT("A", "(* x y)", " y", "x", "x (pi) y 1"), COMPARE("<", "0", "(/ 1 (- 3 A))"))},
    {"a right comparison after a wrong formula", CK_REJECTED, "non è la formula",
     PARTIAL(TERM_AT("A", CUBIC, " x x", "(* 7 x)", "x 1 y 1"), COMPARE("<", "0", "A"))},
    {"a comparison that holds only at another point", CK_REJECTED, "non vale", PARTIAL(TERM_AT("A", CUBIC, " x x", "(* 6 x)", "x -1 y 1"), COMPARE("<", "0", "A"))},

    /* Un membro che non è definito: il confronto non afferma nulla di vero. */
    {"a division by a term that is zero", CK_REJECTED, "vale zero", PARTIAL(TERM_AT("A", CUBIC, " x x", "(* 6 x)", "x 0 y 0"), COMPARE("<", "0", "(/ 1 A)"))},
    {"a logarithm of a term that is zero", CK_REJECTED, "vale zero", PARTIAL(TERM_AT("A", CUBIC, " x x", "(* 6 x)", "x 0 y 0"), COMPARE("<", "(ln A)", "0"))},
    {"a logarithm of a negative term", CK_REJECTED, "non è definito", PARTIAL(TERM_AT("A", CUBIC, " x x", "(* 6 x)", "x -1 y 0"), COMPARE("<", "(ln A)", "9"))},

    /* Confronti di cui il verificatore non sa leggere il segno. */
    {"a comparison with a letter left", CK_UNSUPPORTED, "segno", PARTIAL(TERM("A", "(* (^ x 2) y)", " x", "(* (* 2 x) y)"), COMPARE("<", "A", "0"))},
    {"the square root of 2 against 1", CK_UNSUPPORTED, "segno", PARTIAL(TERM_AT("A", "(* x y)", " y", "x", "x (root 2 2) y 1"), COMPARE("<", "1", "A"))},
    {"the square of e against 7", CK_UNSUPPORTED, "segno", PARTIAL(TERM_AT("A", "(exp (+ x y))", " x x", "(exp (+ x y))", "x 1 y 1"), COMPARE("<", "7", "A"))},
    {"the sign of a sine", CK_UNSUPPORTED, "segno", PARTIAL(TERM_AT("A", "(sin x)", "", "(sin x)", "x 1"), COMPARE("<", "0", "A"))},
    {"an absolute value with a comparison", CK_UNSUPPORTED, "valore assoluto", PARTIAL(TERM_AT("A", "(* x y)", " y", "x", "x 2 y 1"), COMPARE("<", "0", "(abs A)"))},
    {"an absolute value with a comparison and no point", CK_UNSUPPORTED, "con un confronto", PARTIAL(TERM("A", "(* 3 x)", " x", "3"), COMPARE("<", "0", "(abs A)"))},

    /* Pi greco contro 3,14159: la differenza è più piccola dell'intervallo
     * razionale che il verificatore ha per pi greco, che contiene lo zero.
     * Il primo confronto è falso e il secondo vero: nessuno dei due si
     * decide, e soprattutto il primo non è accettato. */
    {"pi called less than 3.14159", CK_UNSUPPORTED, "segno", PARTIAL(TERM_AT("A", "(* x y)", " y", "x", "x (pi) y 1"), COMPARE("<", "A", "(/ 314159 100000)"))},
    {"pi called greater than 3.14159", CK_UNSUPPORTED, "segno", PARTIAL(TERM_AT("A", "(* x y)", " y", "x", "x (pi) y 1"), COMPARE("<", "(/ 314159 100000)", "A"))},
    {"a term without a point that does not exist everywhere", CK_UNSUPPORTED, "definiti", PARTIAL(TERM("A", "(- (ln x) (ln x))", " x", "0"), COMPARE("<=", "A", "0"))},
    {"a member with a letter under a logarithm", CK_UNSUPPORTED, "definiti", PARTIAL("", COMPARE("<", "(- (ln x) (ln x))", "1"))},
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
