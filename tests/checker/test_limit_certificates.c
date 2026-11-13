/* Test del verificatore sui certificati dei limiti (capitolo 14 di
 * checker.c), da solo: qui il motore non c'entra.
 *
 * Compilazione ed esecuzione, dalla cartella principale del progetto:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/checker checker.c && \
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_limit_certificates tests/checker/test_limit_certificates.c && /tmp/test_limit_certificates
 *
 * Come in test_checker.c, il primo comando fa parte del test: checker.c si
 * compila da solo, e questo file include soltanto checker.c.
 *
 * I certificati accettati sono quelli che il motore scrive (29n di main.c),
 * copiati qui come dati; gli altri ne sono alterazioni. Ognuno ha la
 * risposta attesa e, se non è accettato, un pezzo del motivo, così un
 * certificato respinto per la ragione sbagliata non passa per buono:
 *
 *   - accettati: limiti in un punto, da un lato, all'infinito; con pi, e, i
 *     logaritmi dei primi e le radici nei coefficienti; con la lettera L
 *     (x^x); con il fattore e^{G} della scala delle crescite e salendo di un
 *     piano (x = e^{1/t}); lati senza valori; limiti che non esistono;
 *     affermazioni false, smentite con le tre regole del segno (14b);
 *   - respinti: un valore sbagliato, un segno sbagliato all'infinito, un
 *     lato scambiato, un ordine che taglia il termine principale, una
 *     funzione cambiata, una sostituzione sbagliata, lati mancanti o
 *     nell'ordine sbagliato, un "non esiste" detto di un limite che esiste,
 *     un limite senza senso, un'affermazione falsa dichiarata vera e una
 *     vera dichiarata falsa; un valore o un punto che non sono numeri
 *     reali, come ln(-e), 1/(1 - 1), la radice di un negativo;
 *   - non controllabili: x x^{\sin(1/x)}, che il motore dimostrava per
 *     errore; seno e logaritmo di costanti che il verificatore non sa
 *     scrivere; un parametro; confronti tra radici di basi diverse. Non
 *     devono mai diventare un "accettato".
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

/* Un limite in 0 da tutte e due le parti, con la funzione e l'affermazione. */
#define AT_ZERO(function, claim) "certificate limit\nvariable x\npoint 0\nfunction " function "\nclaim " claim "\nside right a+t 4\nside left a-t 4\nend\n"

/* (1 - cos x)/x^2 in 0. */
#define COSINE "(/ (- 1 (cos x)) (^ x 2))"

/* (1 + 1/x)^{2x} a +infinito. */
#define POWER(claim) "certificate limit\nvariable x\npoint +infinity\nfunction (^ (+ 1 (/ 1 x)) (* 2 x))\nclaim " claim "\nside infinity 1/t 4\nend\n"

/* e^x / x^5 a +infinito: il fattore e^{G} (14g). */
#define GROWTH(claim, side) "certificate limit\nvariable x\npoint +infinity\nfunction (/ (exp x) (^ x 5))\nclaim " claim "\nside infinity " side " 4\nend\n"

/* 1/x in 0, da un lato. */
#define RECIPROCAL(point, claim, side) "certificate limit\nvariable x\npoint " point "\nfunction (/ 1 x)\nclaim " claim "\n" side "end\n"

static const Case cases[] = {
    /* Certificati scritti dal motore. */
    {"(1 - cos x)/x^2 tends to 1/2", CK_ACCEPTED, "", AT_ZERO(COSINE, "value (/ 1 2) true")},
    {"arctan x / ln(1 + x^2) at 1 is pi/(4 ln 2)", CK_ACCEPTED, "",
     "certificate limit\nvariable x\npoint 1\nfunction (/ (atan x) (ln (+ 1 (^ x 2))))\nclaim value (/ (pi) (* 4 (ln 2))) true\nside right a+t 4\nside left a-t 4\nend\n"},
    {"and it is not pi/(4 ln 3): a combination of logarithms (14b-2)", CK_ACCEPTED, "",
     "certificate limit\nvariable x\npoint 1\nfunction (/ (atan x) (ln (+ 1 (^ x 2))))\nclaim value (/ (pi) (* 4 (ln 3))) false\nside right a+t 4\nside left a-t 4\nend\n"},
    {"(1 + 1/x)^{2x} tends to e^2", CK_ACCEPTED, "", POWER("value (exp 2) true")},
    {"and not to e^3: one base, Sturm (14b-3)", CK_ACCEPTED, "", POWER("value (exp 3) false")},
    {"x^x from the right of 0, with the letter L", CK_ACCEPTED, "",
     "certificate limit\nvariable x\npoint 0 right\nfunction (^ x x)\nclaim value 1 true\nside right a+t 4\nend\n"},
    {"(2^x - 3^x)/x is -ln(3/2)", CK_ACCEPTED, "", AT_ZERO("(/ (- (^ 2 x) (^ 3 x)) x)", "value (neg (ln (/ 3 2))) true")},
    {"and not -ln(3/3)", CK_ACCEPTED, "", AT_ZERO("(/ (- (^ 2 x) (^ 3 x)) x)", "value (neg (ln (/ 3 3))) false")},
    {"e^x / x^5 is not -infinity: the scale of growth", CK_ACCEPTED, "", GROWTH("minus false", "1/t")},
    {"e^x / x^5 tends to +infinity", CK_ACCEPTED, "", GROWTH("plus true", "1/t")},
    {"ln x / ln ln x, moving up with x = e^{1/t}", CK_ACCEPTED, "",
     "certificate limit\nvariable x\npoint +infinity\nfunction (/ (ln x) (ln (ln x)))\nclaim plus true\nside infinity e^(1/t) 4\nend\n"},
    {"1/cos x from the left of pi/2: cos(pi/2) is exactly 0", CK_ACCEPTED, "",
     "certificate limit\nvariable x\npoint (/ (pi) 2) left\nfunction (/ 1 (cos x))\nclaim plus true\nside left a-t 4\nend\n"},
    {"1/cos x has no limit at pi/2", CK_ACCEPTED, "",
     "certificate limit\nvariable x\npoint (/ (pi) 2)\nfunction (/ 1 (cos x))\nclaim none true\nside right a+t 4\nside left a-t 4\nend\n"},
    {"(arccos x - pi/2)/x is -1", CK_ACCEPTED, "", AT_ZERO("(/ (- (arccos x) (/ (pi) 2)) x)", "value (neg 1) true")},
    {"1/(3^{1/x} + 1) has one-sided limits 0 and 1", CK_ACCEPTED, "", AT_ZERO("(/ 1 (+ (^ 3 (/ 1 x)) 1))", "none true")},
    {"1/x has no limit at 0", CK_ACCEPTED, "", RECIPROCAL("0", "none true", "side right a+t 4\nside left a-t 4\n")},
    {"but |1/x| tends to +infinity", CK_ACCEPTED, "", RECIPROCAL("0", "unsigned true", "side right a+t 4\nside left a-t 4\n")},
    {"1/x from the left of 0 is -infinity", CK_ACCEPTED, "", RECIPROCAL("0 left", "minus true", "side left a-t 4\n")},
    {"sin x / x at +infinity is 0", CK_ACCEPTED, "",
     "certificate limit\nvariable x\npoint +infinity\nfunction (/ (sin x) x)\nclaim value 0 true\nside infinity 1/t 4\nend\n"},
    {"sin x / x at 0 is not 10001/10000", CK_ACCEPTED, "", AT_ZERO("(/ (sin x) x)", "value (/ 10001 10000) false")},
    {"sin x / x at 0 is not pi/3: Machin's bounds for pi", CK_ACCEPTED, "", AT_ZERO("(/ (sin x) x)", "value (/ (pi) 3) false")},
    {"(x - 1)/sqrt(2x^2 - 1) at +infinity is not 1: one base, sqrt 2", CK_ACCEPTED, "",
     "certificate limit\nvariable x\npoint +infinity\nfunction (/ (- x 1) (root 2 (- (* 2 (^ x 2)) 1)))\nclaim value 1 false\nside infinity 1/t 4\nend\n"},
    {"(sin x / x)^{1/x^2} is not e^{-1/7}: two roots of e", CK_ACCEPTED, "", AT_ZERO("(^ (/ (sin x) x) (/ 1 (^ x 2)))", "value (exp (/ -1 7)) false")},
    {"sqrt x at 0: the left side has no values", CK_ACCEPTED, "", AT_ZERO("(root 2 x)", "value 0 true")},
    {"an odd root of a negative number", CK_ACCEPTED, "", AT_ZERO("(root 3 x)", "value 0 true")},
    {"x sin(1/x): a bounded factor times t", CK_ACCEPTED, "", AT_ZERO("(* x (sin (/ 1 x)))", "value 0 true")},
    {"sin x / |x| has one-sided limits 1 and -1", CK_ACCEPTED, "", AT_ZERO("(/ (sin x) (abs x))", "none true")},

    /* Un valore, un segno, un lato. */
    {"a wrong value", CK_REJECTED, "non danno", AT_ZERO(COSINE, "value (/ 1 3) true")},
    {"a true value declared false", CK_REJECTED, "non danno", AT_ZERO(COSINE, "value (/ 1 2) false")},
    {"e^2, declared false", CK_REJECTED, "non danno", POWER("value (exp 2) false")},
    {"a false value declared true", CK_REJECTED, "non danno", AT_ZERO("(/ (sin x) x)", "value (/ 10001 10000) true")},
    {"the wrong sign at +infinity", CK_REJECTED, "non danno", GROWTH("minus true", "1/t")},
    {"the scale of growth read upside down", CK_REJECTED, "non danno",
     "certificate limit\nvariable x\npoint +infinity\nfunction (/ x (exp x))\nclaim plus true\nside infinity 1/t 4\nend\n"},
    {"a one-sided limit with the side swapped", CK_REJECTED, "non danno", RECIPROCAL("0 left", "plus true", "side left a-t 4\n")},
    {"a limit that exists said not to", CK_REJECTED, "non danno", AT_ZERO("(/ 1 (^ x 2))", "none true")},
    {"a value true from one side only", CK_REJECTED, "non danno", AT_ZERO("(/ (sin x) (abs x))", "value 1 true")},
    {"|f| tends to +infinity, said of a finite limit", CK_REJECTED, "non danno", AT_ZERO(COSINE, "unsigned true")},
    {"a function changed: the limit does not exist any more", CK_REJECTED, "non danno", AT_ZERO("(/ (- 1 (cos x)) (^ x 3))", "value (/ 1 2) true")},
    {"no side has values: the limit has no meaning", CK_REJECTED, "nessun lato", AT_ZERO("(root 2 (neg (^ x 2)))", "value 0 true")},

    /* L'ordine e le sostituzioni. */
    {"an order that cuts the leading term", CK_REJECTED, "termine principale",
     "certificate limit\nvariable x\npoint 0\nfunction (/ (- x (sin x)) (^ x 3))\nclaim value (/ 1 6) true\nside right a+t 2\nside left a-t 2\nend\n"},
    {"order 0", CK_REJECTED, "ordine", "certificate limit\nvariable x\npoint 0\nfunction " COSINE "\nclaim value (/ 1 2) true\nside right a+t 0\nside left a-t 0\nend\n"},
    {"the right side with x = a - t", CK_REJECTED, "sostituzione", RECIPROCAL("0 right", "plus true", "side right a-t 4\n")},
    {"-infinity with x = 1/t", CK_REJECTED, "sostituzione",
     "certificate limit\nvariable x\npoint -infinity\nfunction (/ 1 x)\nclaim value 0 true\nside infinity 1/t 4\nend\n"},
    {"moving up at a point that is not 0", CK_REJECTED, "sostituzione",
     "certificate limit\nvariable x\npoint 1 right\nfunction (ln x)\nclaim value 0 true\nside right e^(-1/t) 4\nend\n"},
    {"-infinity with x = e^{1/t}", CK_REJECTED, "sostituzione",
     "certificate limit\nvariable x\npoint -infinity\nfunction (exp x)\nclaim value 0 true\nside infinity e^(1/t) 4\nend\n"},
    {"and with x = -e^{1/t}, as it should be", CK_ACCEPTED, "",
     "certificate limit\nvariable x\npoint -infinity\nfunction (exp x)\nclaim value 0 true\nside infinity -e^(1/t) 4\nend\n"},
    {"a two-sided limit with one side only", CK_REJECTED, "lati", RECIPROCAL("0", "none true", "side right a+t 4\n")},
    {"the two sides in the wrong order", CK_REJECTED, "lati", RECIPROCAL("0", "none true", "side left a-t 4\nside right a+t 4\n")},
    {"a side for a point that has none", CK_REJECTED, "lati", RECIPROCAL("0 right", "plus true", "side left a-t 4\n")},

    /* Numeri che non sono numeri reali: un valore o un punto che non si
     * possono calcolare non sono mai uguali al limite, come ln(-e), che un
     * logaritmo spezzato nei fattori scriverebbe come ln e. */
    {"a claimed value ln(-e)", CK_REJECTED, "non ha un valore", AT_ZERO("(/ (sin x) x)", "value (ln (neg (exp 1))) true")},
    {"a claimed value ln(-1 e)", CK_REJECTED, "non ha un valore", AT_ZERO("(/ (sin x) x)", "value (ln (* -1 (exp 1))) true")},
    {"a claimed value 1/(1 - 1)", CK_REJECTED, "non ha un valore", AT_ZERO("(/ (sin x) x)", "value (/ 1 (- 1 1)) true")},
    {"a claimed value sqrt(-2)", CK_REJECTED, "non ha un valore", AT_ZERO("(/ (sin x) x)", "value (root 2 -2) true")},
    {"a claimed value ln 0", CK_REJECTED, "non ha un valore", AT_ZERO("(/ (sin x) x)", "value (ln 0) true")},
    {"a claimed value (-4)^{1/2}", CK_REJECTED, "non ha un valore", AT_ZERO("(/ (sin x) x)", "value (^ -4 (/ 1 2)) true")},
    {"a claimed value 1 + 0 ln(-1)", CK_REJECTED, "non ha un valore", AT_ZERO("(/ (sin x) x)", "value (+ 1 (* 0 (ln -1))) true")},
    {"a claimed value tan(pi/2)", CK_REJECTED, "non ha un valore", AT_ZERO("(/ (sin x) x)", "value (tan (/ (pi) 2)) true")},
    {"a claimed value arcsin 2", CK_REJECTED, "non ha un valore", AT_ZERO("(/ (sin x) x)", "value (arcsin 2) true")},
    {"a point ln(-1)", CK_REJECTED, "non ha un valore",
     "certificate limit\nvariable x\npoint (ln -1)\nfunction x\nclaim value 0 true\nside right a+t 4\nside left a-t 4\nend\n"},

    /* Ciò che il verificatore non sa controllare. */
    {"x x^{sin(1/x)}: e^{O(1)} is not bounded when O(1) hides L", CK_UNSUPPORTED, "esponenziale",
     "certificate limit\nvariable x\npoint 0 right\nfunction (* x (^ x (sin (/ 1 x))))\nclaim value 0 true\nside right a+t 4\nend\n"},
    {"the sine of 1", CK_UNSUPPORTED, "seno e coseno",
     "certificate limit\nvariable x\npoint 1\nfunction (sin x)\nclaim value (sin 1) true\nside right a+t 4\nside left a-t 4\nend\n"},
    {"the logarithm of pi - 3", CK_UNSUPPORTED, "logaritmo", AT_ZERO("(/ (sin x) x)", "value (ln (- (pi) 3)) true")},
    {"a parameter", CK_UNSUPPORTED, "lettera", AT_ZERO("(/ (sin (* a x)) x)", "value 1 true")},
    {"a root whose index is not a written integer", CK_UNSUPPORTED, "indice", AT_ZERO("(root (+ 1 1) (+ 1 x))", "value 1 true")},
    {"square roots of different bases compared", CK_UNSUPPORTED, "confrontare",
     "certificate limit\nvariable x\npoint 0 right\nfunction (/ (ln x) (root 2 (+ 1 (* 2 (^ (ln x) 2)))))\nclaim value (neg (root 2 (/ 1 3))) false\nside right e^(-1/t) 4\nend\n"},
    {"an exponential of a function of L with no expansion", CK_UNSUPPORTED, "esponenziale",
     "certificate limit\nvariable x\npoint +infinity\nfunction (^ (/ 1 (* 3 x)) (/ 1 (ln (* 2 x))))\nclaim value (exp -1) true\nside infinity 1/t 4\nend\n"},
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

    /* Un certificato non lascia tracce nel successivo: dopo uno con pi, e e
     * la lettera L, uno di soli razionali; e due certificati in un testo si
     * leggono uno dopo l'altro. */
    const char *two = POWER("value (exp 3) true") AT_ZERO(COSINE, "value (/ 1 2) true");
    check(ck_check(&two, &why) == CK_REJECTED, "the first of two certificates should be rejected");
    check(ck_check(&two, &why) == CK_ACCEPTED, "the second of two certificates should be read next, and accepted");

    printf("%s: %d of %d limit certificate checks passed\n", failures ? "FAIL" : "PASS", checks - failures, checks);
    return failures ? 1 : 0;
}
