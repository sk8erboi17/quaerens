/* Test del verificatore sui certificati delle tabelle dei segni (capitolo 12
 * di checker.c), da solo: qui il motore non c'entra.
 *
 * Compilazione ed esecuzione, dalla cartella principale del progetto:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/checker checker.c && \
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_signs tests/checker/test_signs.c && /tmp/test_signs
 *
 * Come in test_checker.c, il primo comando fa parte del test: checker.c si
 * compila da solo, e questo file include soltanto checker.c.
 *
 * I certificati accettati sono quelli che il motore scrive (regola 18p-15 di
 * main.c), copiati qui come dati; gli altri ne sono alterazioni. Ognuno ha
 * la risposta attesa e, se non è accettato, un pezzo del motivo, così un
 * certificato respinto per la ragione sbagliata (una parentesi dimenticata
 * nel test) non passa per buono:
 *
 *   - accettati: soluzioni con estremi razionali e con radici quadrate, con
 *     valori assoluti, con un punto tolto, nessuna soluzione e tutti i
 *     numeri; "per ogni" ed "esiste", veri e falsi, nei reali e nei
 *     razionali; un fattore che riunisce due polinomi presenti con esponenti
 *     diversi; una condizione che non ha valore in nessun punto;
 *   - respinti: un intervallo con due radici, o con la radice di un altro
 *     fattore; una radice mancante; un campione fuori dal suo intervallo; un
 *     fattore cambiato; la condizione cambiata; la verità di una riga
 *     rovesciata; soluzioni scritte con < al posto di \le, o con un estremo
 *     spostato; nei razionali, una radice razionale scritta come irrazionale;
 *     un'affermazione che la tabella non dà;
 *   - non controllabili: un logaritmo, un parametro, numeri troppo grandi.
 *     Non devono mai diventare un "accettato".
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

/* Le soluzioni di (x - 2)/(x + 3) >= 0: x < -3 oppure x >= 2. */
#define QUOTIENT_HEAD "certificate signs\nvariable x\ndomain reals\ncondition (<= 0 (/ (- x 2) (+ x 3)))\nclaim solve\n"
#define QUOTIENT_SOLUTION "solution (or (< x (neg 3)) (<= 2 x))\n"
#define QUOTIENT_FACTORS "factor (+ x 3)\nfactor (+ x -2)\n"
#define QUOTIENT_ROWS "interval -5 true\npoint -3 false\ninterval -2 false\npoint 2 true\ninterval 4 true\nend\n"

/* Le soluzioni di x^2 < 2, tra -sqrt(2) e sqrt(2): due radici irrazionali. */
#define SQUARE_HEAD "certificate signs\nvariable x\ndomain reals\ncondition (< (^ x 2) 2)\nclaim solve\n" \
    "solution (and (< (neg (sqrt 2)) x) (< x (sqrt 2)))\nfactor (+ (^ x 2) -2)\n"
#define SQUARE_ROWS "interval -3 false\nroot 1 (/ -95 64) (/ -45 32) false\ninterval -1 true\nroot 1 (/ 45 32) (/ 95 64) false\ninterval 3 false\nend\n"

/* x^2 - 2 \neq 0: falso nelle due radici. */
#define NOT_TWO(claim) "certificate signs\nvariable x\ndomain reals\ncondition (neq (- (^ x 2) 2) 0)\nclaim " claim "\nfactor (+ (^ x 2) -2)\n"
#define NOT_TWO_ROWS "interval -3 true\nroot 1 (/ -95 64) (/ -45 32) false\ninterval -1 true\nroot 1 (/ 45 32) (/ 95 64) false\ninterval 3 true\nend\n"

/* Due valori assoluti: |2x + 3| - |3x - 2| < 4 per x < 3/5 o x > 1. */
#define ABSOLUTE "certificate signs\nvariable x\ndomain reals\ncondition (< (- (abs (+ (* 2 x) 3)) (abs (- (* 3 x) 2))) 4)\nclaim solve\n" \
    "solution (or (< x (/ 3 5)) (< 1 x))\nfactor (+ (* 2 x) 3)\nfactor (+ x 1)\nfactor (+ (* 5 x) -3)\nfactor (+ (* 3 x) -2)\nfactor (+ x -1)\nfactor (+ x -9)\n" \
    "interval -3 true\npoint (/ -3 2) true\ninterval (/ -4 3) true\npoint -1 true\ninterval 0 true\npoint (/ 3 5) false\ninterval (/ 5 8) false\n" \
    "point (/ 2 3) false\ninterval (/ 3 4) false\npoint 1 false\ninterval 2 true\npoint 9 true\ninterval 11 true\nend\n"

/* (x + 1)^4 (x^5 + 1)(x^6 + x^4 - x^2 - 1) >= 0 se e solo se x >= 1 o x = -1. Il
 * terzo fattore è (x^2 + 1)(x^4 - x^3 + x^2 - x + 1), e la condizione
 * contiene x^2 + 1 al quadrato: resta un pezzo che nessun fattore divide
 * (12b). */
#define PRODUCT "certificate signs\nvariable x\ndomain reals\n" \
    "condition (iff (<= 0 (* (* (^ (+ x 1) 4) (+ (^ x 5) 1)) (- (- (+ (^ x 6) (^ x 4)) (^ x 2)) 1))) (or (<= 1 x) (= x (neg 1))))\n" \
    "claim forall true\nfactor (+ x 1)\nfactor (+ x -1)\n" \
    "factor (+ (^ x 6) (+ (* -1 (^ x 5)) (+ (* 2 (^ x 4)) (+ (* -2 (^ x 3)) (+ (* 2 (^ x 2)) (+ (* -1 x) 1))))))\n" \
    "interval -3 true\npoint -1 true\ninterval 0 true\npoint 1 true\ninterval 3 true\nend\n"

/* (x^3 - 2)^2 (x^3 - 3) \neq 0 è falso nelle radici cubiche di 2 e di 3, che
 * hanno lo stesso fattore x^6 - 5x^3 + 6: in quella di 2 si annulla solo una
 * parte del fattore, e lo dice il massimo comun divisore (12b). */
#define CUBES "certificate signs\nvariable x\ndomain reals\ncondition (neq (* (^ (- (^ x 3) 2) 2) (- (^ x 3) 3)) 0)\nclaim forall false\n" \
    "factor (+ (^ x 6) (+ (* -5 (^ x 3)) 6))\n" \
    "interval -1 true\nroot 1 (/ 5 4) (/ 85 64) false\ninterval (/ 4 3) true\nroot 1 (/ 45 32) (/ 95 64) false\ninterval 3 true\nend\n"

/* Nei razionali x^2 < 2 o x^2 > 2 vale sempre: le radici sono irrazionali. */
#define RATIONALS(factor, low, high) "certificate signs\nvariable x\ndomain rationals\ncondition (or (< (^ x 2) " factor ") (< " factor " (^ x 2)))\n" \
    "claim forall true\nfactor (+ (^ x 2) -" factor ")\ninterval -3 true\nroot 1 (neg " high ") (neg " low ") false\ninterval -1 true\n" \
    "root 1 " low " " high " false\ninterval 3 true\nend\n"

/* Non ogni razionale positivo ha (5x - 3)(x + 1) > 0: l'insieme è nella
 * condizione. */
#define POSITIVE "certificate signs\nvariable x\ndomain rationals\ncondition (implies (< 0 x) (< 0 (* (- (* 5 x) 3) (+ x 1))))\nclaim forall false\n" \
    "factor (+ x 1)\nfactor x\nfactor (+ (* 5 x) -3)\n" \
    "interval -3 true\npoint -1 true\ninterval (/ -1 2) true\npoint 0 true\ninterval (/ 1 2) false\npoint (/ 3 5) false\ninterval 2 true\nend\n"

static const Case cases[] = {
    /* Certificati scritti dal motore. */
    {"solutions with rational endpoints", CK_ACCEPTED, "", QUOTIENT_HEAD QUOTIENT_SOLUTION QUOTIENT_FACTORS QUOTIENT_ROWS},
    {"solutions with square roots as endpoints", CK_ACCEPTED, "", SQUARE_HEAD SQUARE_ROWS},
    {"solutions of a condition with two absolute values", CK_ACCEPTED, "", ABSOLUTE},
    {"x^2 - 2 \\neq 0 is false at an irrational root", CK_ACCEPTED, "", NOT_TWO("forall false") NOT_TWO_ROWS},
    {"a factor joining two polynomials with different exponents", CK_ACCEPTED, "", PRODUCT},
    {"a condition that vanishes at a root of part of a factor", CK_ACCEPTED, "", CUBES},
    {"among the rationals the square of x is never 2", CK_ACCEPTED, "", RATIONALS("2", "(/ 45 32)", "(/ 95 64)")},
    {"not every positive rational satisfies the condition", CK_ACCEPTED, "", POSITIVE},
    {"solutions with a point taken away", CK_ACCEPTED, "",
     "certificate signs\nvariable x\ndomain reals\ncondition (and (neq x 1) (<= (^ x 2) 4))\nclaim solve\n"
     "solution (or (and (<= (neg 2) x) (< x 1)) (and (< 1 x) (<= x 2)))\nfactor (+ x 2)\nfactor (+ x -1)\nfactor (+ x -2)\n"
     "interval -4 false\npoint -2 true\ninterval -1 true\npoint 1 false\ninterval (/ 3 2) true\npoint 2 true\ninterval 4 false\nend\n"},
    {"no solution", CK_ACCEPTED, "",
     "certificate signs\nvariable x\ndomain reals\ncondition (< (+ (^ x 2) 1) 0)\nclaim solve\nsolution none\nfactor (+ (^ x 2) 1)\ninterval 0 false\nend\n"},
    {"every real number is a solution", CK_ACCEPTED, "",
     "certificate signs\nvariable x\ndomain reals\ncondition (< 0 (+ (^ x 2) 1))\nclaim solve\nsolution all\nfactor (+ (^ x 2) 1)\ninterval 0 true\nend\n"},
    {"a negative exponent is a division", CK_ACCEPTED, "",
     "certificate signs\nvariable x\ndomain reals\ncondition (< (^ x (neg 2)) 1)\nclaim solve\nsolution (or (< x (neg 1)) (< 1 x))\n"
     "factor (+ x 1)\nfactor x\nfactor (+ x -1)\ninterval -3 true\npoint -1 false\ninterval (/ -1 2) false\npoint 0 false\n"
     "interval (/ 1 2) false\npoint 1 false\ninterval 3 true\nend\n"},
    {"a condition with no value anywhere", CK_ACCEPTED, "",
     "certificate signs\nvariable x\ndomain reals\ncondition (neq (/ 1 (- x x)) 5)\nclaim solve\nsolution none\ninterval 0 false\nend\n"},
    {"some x has x^3 - 2x < 0", CK_ACCEPTED, "",
     "certificate signs\nvariable x\ndomain reals\ncondition (< (- (^ x 3) (* 2 x)) 0)\nclaim exists true\nfactor x\nfactor (+ (^ x 2) -2)\n"
     "interval -3 true\nroot 2 (/ -95 64) (/ -45 32) false\ninterval -1 false\npoint 0 false\ninterval 1 true\n"
     "root 2 (/ 45 32) (/ 95 64) false\ninterval 3 false\nend\n"},

    /* Le radici: isolate, tutte, e nessun campione fuori posto. */
    {"a root interval that holds both roots", CK_REJECTED, "una sola radice",
     SQUARE_HEAD "interval -3 false\nroot 1 -2 2 false\ninterval 3 false\nend\n"},
    {"a root interval that holds the root of another factor", CK_REJECTED, "di un altro fattore",
     "certificate signs\nvariable x\ndomain reals\ncondition (< (- (^ x 3) (* 2 x)) 0)\nclaim exists true\nfactor x\nfactor (+ (^ x 2) -2)\n"
     "interval -3 true\nroot 2 -2 (/ 1 2) false\ninterval 1 true\nroot 2 (/ 45 32) (/ 95 64) false\ninterval 3 false\nend\n"},
    {"a missing root of a factor", CK_REJECTED, "manca una radice", NOT_TWO("forall true") "interval 0 true\nend\n"},
    {"a root of the condition that is no factor's", CK_REJECTED, "non è un punto della tabella",
     "certificate signs\nvariable x\ndomain reals\ncondition (< (* (- x 1) (- x 2)) 0)\nclaim exists false\nfactor (- x 1)\n"
     "interval 0 false\npoint 1 false\ninterval 3 false\nend\n"},
    {"a sample outside its interval", CK_REJECTED, "campione",
     QUOTIENT_HEAD QUOTIENT_SOLUTION QUOTIENT_FACTORS "interval -5 true\npoint -3 false\ninterval -4 false\npoint 2 true\ninterval 4 true\nend\n"},
    {"a root interval with its ends in the wrong order", CK_REJECTED, "ordine sbagliato",
     SQUARE_HEAD "interval -3 false\nroot 1 (/ -45 32) (/ -95 64) false\ninterval -1 true\nroot 1 (/ 45 32) (/ 95 64) false\ninterval 3 false\nend\n"},

    /* Bugie che solo un controllo ferma: ognuno di questi certificati
     * afferma il falso, e ogni altro controllo lo lascerebbe passare.
     * x^2 < 2 non ha soluzioni se una radice si dichiara dove non è, o se il
     * campione tra le due radici si prende fuori; x^2 \neq 4 in tutti i
     * razionali se le radici 2 e -2 si dichiarano irrazionali. */
    {"no solution of x^2 < 2, with a root declared where there is none", CK_REJECTED, "una sola radice",
     "certificate signs\nvariable x\ndomain reals\ncondition (< (^ x 2) 2)\nclaim exists false\nfactor (+ (^ x 2) -2)\n"
     "interval -3 false\nroot 1 (/ -3 2) (/ -45 32) false\ninterval 2 false\nroot 1 (/ 5 2) 3 false\ninterval 4 false\nend\n"},
    {"no solution of x^2 < 2, with the sample between the roots taken outside", CK_REJECTED, "campione",
     "certificate signs\nvariable x\ndomain reals\ncondition (< (^ x 2) 2)\nclaim exists false\nfactor (+ (^ x 2) -2)\n"
     "interval -3 false\nroot 1 (/ -95 64) (/ -45 32) false\ninterval 5 false\nroot 1 (/ 45 32) (/ 95 64) false\ninterval 6 false\nend\n"},
    {"x^2 \\neq 4 for every rational, with the rational roots written as irrational", CK_REJECTED, "razionale",
     "certificate signs\nvariable x\ndomain rationals\ncondition (neq (^ x 2) 4)\nclaim forall true\nfactor (+ (^ x 2) -4)\n"
     "interval -3 true\nroot 1 (/ -5 2) (/ -3 2) false\ninterval -1 true\nroot 1 (/ 3 2) (/ 5 2) false\ninterval 3 true\nend\n"},
    {"a written solution with an endpoint that no sample sees", CK_REJECTED, "non è un punto della tabella",
     QUOTIENT_HEAD "solution (or (< x (neg 3)) (<= 0 x))\n" QUOTIENT_FACTORS QUOTIENT_ROWS},

    /* Un dato cambiato. */
    {"a factor changed", CK_REJECTED, "manca una radice",
     QUOTIENT_HEAD QUOTIENT_SOLUTION "factor (+ x 3)\nfactor (+ x -3)\n" QUOTIENT_ROWS},
    {"the condition changed: strict instead of not strict", CK_REJECTED, "verità dichiarata",
     "certificate signs\nvariable x\ndomain reals\ncondition (< 0 (/ (- x 2) (+ x 3)))\nclaim solve\n" QUOTIENT_SOLUTION QUOTIENT_FACTORS QUOTIENT_ROWS},
    {"the truth of a row flipped", CK_REJECTED, "verità dichiarata",
     QUOTIENT_HEAD QUOTIENT_SOLUTION QUOTIENT_FACTORS "interval -5 true\npoint -3 false\ninterval -2 false\npoint 2 true\ninterval 4 false\nend\n"},
    {"the truth at an irrational root flipped", CK_REJECTED, "verità dichiarata",
     NOT_TWO("forall false") "interval -3 true\nroot 1 (/ -95 64) (/ -45 32) true\ninterval -1 true\nroot 1 (/ 45 32) (/ 95 64) false\ninterval 3 true\nend\n"},

    /* Le soluzioni scritte. */
    {"a written solution with < in place of \\le", CK_REJECTED, "non dà ciò che",
     QUOTIENT_HEAD "solution (or (< x (neg 3)) (< 2 x))\n" QUOTIENT_FACTORS QUOTIENT_ROWS},
    {"a written solution with \\le in place of <", CK_REJECTED, "non dà ciò che",
     QUOTIENT_HEAD "solution (or (<= x (neg 3)) (<= 2 x))\n" QUOTIENT_FACTORS QUOTIENT_ROWS},
    {"a written solution with an endpoint moved", CK_REJECTED, "non è un punto della tabella",
     QUOTIENT_HEAD "solution (or (< x (neg 3)) (<= 3 x))\n" QUOTIENT_FACTORS QUOTIENT_ROWS},
    {"a written solution with the other square root", CK_REJECTED, "non è un punto della tabella",
     "certificate signs\nvariable x\ndomain reals\ncondition (< (^ x 2) 2)\nclaim solve\n"
     "solution (and (< (neg (sqrt 3)) x) (< x (sqrt 3)))\nfactor (+ (^ x 2) -2)\n" SQUARE_ROWS},
    {"a written solution with a piece forgotten", CK_REJECTED, "non dà ciò che",
     QUOTIENT_HEAD "solution (< x (neg 3))\n" QUOTIENT_FACTORS QUOTIENT_ROWS},
    {"no solution, written for a condition that has some", CK_REJECTED, "non dà ciò che",
     QUOTIENT_HEAD "solution none\n" QUOTIENT_FACTORS QUOTIENT_ROWS},

    /* Che cosa si afferma, e dove. */
    {"for every x, where the table has a false row", CK_REJECTED, "non dà ciò che", NOT_TWO("forall true") NOT_TWO_ROWS},
    {"no x, where the table has a true row", CK_REJECTED, "non dà ciò che",
     "certificate signs\nvariable x\ndomain reals\ncondition (< (- (^ x 3) (* 2 x)) 0)\nclaim exists false\nfactor x\nfactor (+ (^ x 2) -2)\n"
     "interval -3 true\nroot 2 (/ -95 64) (/ -45 32) false\ninterval -1 false\npoint 0 false\ninterval 1 true\n"
     "root 2 (/ 45 32) (/ 95 64) false\ninterval 3 false\nend\n"},
    {"among the rationals, a rational root written as irrational", CK_REJECTED, "razionale",
     RATIONALS("4", "(/ 3 2)", "(/ 5 2)")},
    {"solutions among the rationals", CK_REJECTED, "solve vuole i reali",
     "certificate signs\nvariable x\ndomain rationals\ncondition (< (^ x 2) 2)\nclaim solve\nsolution none\nend\n"},
    {"a table that ends with a point", CK_REJECTED, "finire con un intervallo",
     QUOTIENT_HEAD QUOTIENT_SOLUTION QUOTIENT_FACTORS "interval -5 true\npoint -3 false\nend\n"},
    {"a factor that is a number", CK_REJECTED, "un numero",
     QUOTIENT_HEAD QUOTIENT_SOLUTION "factor 7\n" QUOTIENT_FACTORS QUOTIENT_ROWS},

    /* Ciò che il verificatore non conosce. */
    {"a logarithm in the condition", CK_UNSUPPORTED, "non conosce",
     "certificate signs\nvariable x\ndomain reals\ncondition (< (ln x) 0)\nclaim exists true\ninterval 0 true\nend\n"},
    {"a second letter, a parameter", CK_UNSUPPORTED, "non conosce",
     "certificate signs\nvariable x\ndomain reals\ncondition (< x a)\nclaim exists true\ninterval 0 true\nend\n"},
    {"a square root in the condition", CK_UNSUPPORTED, "non conosce",
     "certificate signs\nvariable x\ndomain reals\ncondition (< (sqrt x) 2)\nclaim exists true\ninterval 0 true\nend\n"},
    {"numbers too large for the checker", CK_UNSUPPORTED, "troppo grandi",
     "certificate signs\nvariable x\ndomain reals\ncondition (< (^ x 20) 0)\nclaim exists false\nfactor x\n"
     "interval -1 false\npoint 0 false\ninterval 99999999999999999 false\nend\n"},
    {"too many absolute values", CK_UNSUPPORTED, "troppi valori assoluti",
     "certificate signs\nvariable x\ndomain reals\ncondition (< (+ (+ (abs x) (abs (- x 1))) (+ (+ (abs (- x 2)) (abs (- x 3))) (abs (- x 4)))) 0)\n"
     "claim exists false\ninterval 0 false\nend\n"},
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
        CkAnswer answer = ck_check(&text, &why);
        snprintf(message, sizeof message, "\"%s\" should be %s but is %s (%s)", cases[i].name,
                 answer_name(cases[i].expected), answer_name(answer), why);
        check(answer == cases[i].expected, message);
        snprintf(message, sizeof message, "\"%s\" should mention \"%s\" as its reason, not \"%s\"", cases[i].name, cases[i].why, why);
        check(answer != cases[i].expected || strstr(why, cases[i].why) != NULL, message);
    }

    /* Un certificato non lascia tracce nel successivo, e due certificati in
     * un testo si leggono uno dopo l'altro. */
    const char *two = QUOTIENT_HEAD "solution (< x (neg 3))\n" QUOTIENT_FACTORS QUOTIENT_ROWS SQUARE_HEAD SQUARE_ROWS;
    check(ck_check(&two, &why) == CK_REJECTED, "the first of two certificates should be rejected");
    check(ck_check(&two, &why) == CK_ACCEPTED, "the second of two certificates should be read next, and accepted");

    printf("%s: %d of %d sign table certificate checks passed\n", failures ? "FAIL" : "PASS", checks - failures, checks);
    return failures ? 1 : 0;
}
