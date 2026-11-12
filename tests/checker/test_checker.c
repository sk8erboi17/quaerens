/* Test del verificatore dei certificati, da solo: qui il motore non c'entra.
 *
 * Compilazione ed esecuzione, dalla cartella principale del progetto:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/checker checker.c && \
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_checker tests/checker/test_checker.c && /tmp/test_checker
 *
 * Il primo comando fa parte del test: checker.c deve compilarsi da solo e
 * senza avvisi, perché chi controlla una dimostrazione non deve avere
 * bisogno del programma che l'ha prodotta. Per lo stesso motivo questo file
 * include soltanto checker.c.
 *
 * I certificati sono scritti a mano, e ognuno ha la risposta attesa. Le
 * risposte possibili sono tre, e il test le distingue:
 *
 *   - accettato: un'identità vera con un fattore giusto;
 *   - respinto: un controllo fallisce. Di ogni caso è indicato anche un pezzo
 *     del motivo, perché un certificato respinto per la ragione sbagliata
 *     (per esempio una parentesi dimenticata nel test) non proverebbe nulla;
 *   - non controllabile: il certificato chiede qualcosa che il verificatore
 *     non sa trattare. Non deve mai diventare un "accettato".
 *
 * I casi respinti e non controllabili sono antiregressione: non si tolgono e
 * non si indeboliscono per far passare una modifica al verificatore.
 */

#include <stdio.h>
#include <string.h>

/* Il verificatore è un programma con il suo main(): lo si include cambiando
 * nome a quel main, così il test può chiamare sia ck_check() sia il
 * programma intero. */
#define main checker_main
#include "../../checker.c"
#undef main

/* Un certificato scritto a mano e ciò che il verificatore deve rispondere. */
typedef struct {
    const char *name;    /* che cosa mette alla prova, in inglese */
    CkAnswer expected;   /* la risposta attesa */
    const char *why;     /* un pezzo del motivo atteso; "" per gli accettati */
    const char *text;    /* il certificato */
} Case;

/* Le voci comuni a quasi tutti i certificati. */
#define HEAD "certificate induction\nvariable n\nbase 1\n"
#define GAUSS "statement (= (sum k 1 n k) (/ (* n (+ n 1)) 2))\n"
#define FIBONACCI "sequence F 2 1 1 0 1\n"
#define HARMONIC "define H m (sum j 1 m (/ 1 j))\n"

static const Case cases[] = {
    /* Identità vere, con il fattore giusto. */
    {"sum of the first n integers", CK_ACCEPTED, "",
     HEAD GAUSS "factor 1\nend\n"},
    {"entries in a different order", CK_ACCEPTED, "",
     "certificate induction\nfactor 1\n" GAUSS "base 1\nvariable n\nend\n"},
    {"telescoping product, with a rational factor", CK_ACCEPTED, "",
     HEAD "statement (= (prod k 1 n (/ (+ k 1) k)) (+ n 1))\nfactor (/ (+ n 2) (+ n 1))\nend\n"},
    {"geometric sum with a numeric base", CK_ACCEPTED, "",
     "certificate induction\nvariable n\nbase 0\nstatement (= (sum k 0 n (^ 2 k)) (- (^ 2 (+ n 1)) 1))\nfactor 1\nend\n"},
    {"geometric sum with a parameter as base", CK_ACCEPTED, "",
     "certificate induction\nvariable n\nbase 0\n"
     "statement (= (sum k 0 n (^ x k)) (/ (- (^ x (+ n 1)) 1) (- x 1)))\nfactor 1\nend\n"},
    {"power of a number times a parameter, split into 2^n and x^n", CK_ACCEPTED, "",
     "certificate induction\nvariable n\nbase 0\n"
     "statement (= (sum k 0 n (^ (* 2 x) k)) (/ (- (* (^ 2 (+ n 1)) (^ x (+ n 1))) 1) (- (* 2 x) 1)))\nfactor 1\nend\n"},
    {"4^n recognised as the square of 2^n", CK_ACCEPTED, "",
     HEAD "statement (= (^ 4 n) (* (^ 2 n) (^ 2 n)))\nfactor 0\nend\n"},
    {"alternating sum, with (-1)^n", CK_ACCEPTED, "",
     HEAD "statement (= (sum k 1 n (* (^ -1 k) k)) (/ (- (* (^ -1 n) (+ (* 2 n) 1)) 1) 4))\nfactor 1\nend\n"},
    {"(-1)^n in the denominator of the factor", CK_ACCEPTED, "",
     "certificate induction\nvariable n\nbase 0\n"
     "statement (= (prod k 0 n (/ (^ -1 k) (^ -1 (- k 1)))) (/ (^ -1 n) (^ -1 -1)))\nfactor (/ (^ -1 (+ n 1)) (^ -1 n))\nend\n"},
    {"Fibonacci numbers, defined by their recurrence", CK_ACCEPTED, "",
     HEAD FIBONACCI "statement (= (sum k 1 n (seq F k)) (- (seq F (+ n 2)) 1))\nfactor 1\nend\n"},
    {"harmonic numbers, defined by a formula", CK_ACCEPTED, "",
     HEAD HARMONIC "statement (= (sum k 1 n (seq H k)) (- (* (+ n 1) (seq H n)) n))\nfactor 1\nend\n"},
    {"sum of k k!, with the factorial", CK_ACCEPTED, "",
     HEAD "statement (= (sum k 1 n (* k (fact k))) (- (fact (+ n 1)) 1))\nfactor 1\nend\n"},
    {"binomials with a constant lower index", CK_ACCEPTED, "",
     HEAD "statement (= (sum k 1 n (binom k 2)) (binom (+ n 1) 3))\nfactor 1\nend\n"},
    {"central binomial as factorials: no hypothesis needed, factor 0", CK_ACCEPTED, "",
     HEAD "statement (= (binom (* 2 n) n) (/ (fact (* 2 n)) (* (fact n) (fact n))))\nfactor 0\nend\n"},
    {"sum up to 2n, two new terms at each step", CK_ACCEPTED, "",
     HEAD "statement (= (sum k 1 (* 2 n) k) (* n (+ (* 2 n) 1)))\nfactor 1\nend\n"},
    {"base case later than the first term", CK_ACCEPTED, "",
     "certificate induction\nvariable n\nbase 2\n" GAUSS "factor 1\nend\n"},
    {"an empty sum at the base case", CK_ACCEPTED, "",
     "certificate induction\nvariable n\nbase 0\n" GAUSS "factor 1\nend\n"},
    {"undefined at n = 50, certified where it is defined", CK_ACCEPTED, "",
     HEAD "statement (= (/ (- n 50) (- n 50)) 1)\nfactor 1\nend\n"},
    {"a declared generic hypothesis on a parameter", CK_ACCEPTED, "",
     HEAD "statement (= (* x (prod k 1 n (/ (+ k 1) k))) (* x (+ n 1)))\n"
     "factor (/ (* x (+ n 2)) (* x (+ n 1)))\ngeneric x\nend\n"},

    /* Enunciati falsi: nessun fattore li può far accettare. */
    {"wrong closed form, false at the base case", CK_REJECTED, "caso base",
     HEAD "statement (= (sum k 1 n k) (/ (* n (+ n 2)) 2))\nfactor 1\nend\n"},
    {"one more than the true closed form", CK_REJECTED, "caso base",
     HEAD "statement (= (sum k 1 n k) (+ (/ (* n (+ n 1)) 2) 1))\nfactor 1\nend\n"},
    {"right at the base case, wrong afterwards", CK_REJECTED, "passo induttivo",
     HEAD "statement (= (sum k 1 n k) n)\nfactor 1\nend\n"},
    {"right at the base case, wrong afterwards, with the true form plus n - 1", CK_REJECTED, "passo induttivo",
     HEAD "statement (= (sum k 1 n k) (+ (/ (* n (+ n 1)) 2) (- n 1)))\nfactor 1\nend\n"},
    {"false statement with the factor D(n+1)/D(n) written out", CK_REJECTED, "denominatore",
     HEAD "statement (= (sum k 1 n k) n)\n"
     "factor (/ (- (sum k 1 (+ n 1) k) (+ n 1)) (- (sum k 1 n k) n))\nend\n"},
    {"false statement with a factor that vanishes for odd n in the denominator", CK_REJECTED, "passo induttivo",
     HEAD "statement (= (sum k 1 n k) n)\nfactor (/ 1 (+ 1 (^ -1 n)))\nend\n"},

    /* Enunciati veri con un certificato sbagliato. */
    {"true statement, wrong factor", CK_REJECTED, "passo induttivo",
     HEAD GAUSS "factor 2\nend\n"},
    {"true statement, factor D(n+1)/D(n): its denominator is the hypothesis itself", CK_REJECTED, "denominatore",
     HEAD GAUSS "factor (/ (- (+ (sum k 1 n k) (+ n 1)) (/ (* (+ n 1) (+ n 2)) 2)) (- (sum k 1 n k) (/ (* n (+ n 1)) 2)))\nend\n"},
    {"factor with a parameter in the denominator and no generic entry", CK_REJECTED, "denominatore",
     HEAD "statement (= (* x (prod k 1 n (/ (+ k 1) k))) (* x (+ n 1)))\n"
     "factor (/ (* x (+ n 2)) (* x (+ n 1)))\nend\n"},
    {"factor whose denominator is zero for every odd n", CK_REJECTED, "denominatore",
     HEAD "statement (= n n)\nfactor (/ 1 (+ 1 (^ -1 n)))\nend\n"},
    {"generic entry that mentions n", CK_REJECTED, "generic",
     HEAD GAUSS "factor (/ 1 (- n 5))\ngeneric (- n 5)\nend\n"},
    {"generic entry that is always zero", CK_REJECTED, "generic",
     HEAD GAUSS "factor 1\ngeneric (- x x)\nend\n"},
    {"statement that divides by an expression that is always zero", CK_REJECTED, "sempre zero",
     HEAD "statement (= (/ n (- n n)) 1)\nfactor 1\nend\n"},

    /* Certificati scritti male. */
    {"missing end", CK_REJECTED, "scritto male",
     HEAD GAUSS "factor 1\n"},
    {"missing factor", CK_REJECTED, "incompleto",
     HEAD GAUSS "end\n"},
    {"unknown entry", CK_REJECTED, "scritto male",
     HEAD GAUSS "factor 1\nassume (- n 1)\nend\n"},
    {"two statements in one certificate", CK_REJECTED, "due volte",
     HEAD "statement (= n n)\n" GAUSS "factor 1\nend\n"},
    {"two factors in one certificate", CK_REJECTED, "due volte",
     HEAD GAUSS "factor 2\nfactor 1\nend\n"},
    {"the same sequence defined twice", CK_REJECTED, "due volte",
     HEAD FIBONACCI "sequence F 2 1 1 2 1\n" GAUSS "factor 1\nend\n"},
    {"a formula given where a recurrence is announced", CK_UNSUPPORTED, "ordine di ricorrenza",
     HEAD "sequence H m (sum j 1 m (/ 1 j))\n" GAUSS "factor 1\nend\n"},
    {"a recurrence given where a formula is announced", CK_REJECTED, "lettera attesa",
     HEAD "define F 2 1 1 0 1\n" GAUSS "factor 1\nend\n"},
    {"unbalanced parentheses", CK_REJECTED, "scritto male",
     HEAD "statement (= (sum k 1 n k) (/ (* n (+ n 1)) 2)\nfactor 1\nend\n"},
    {"a name of more than one letter", CK_REJECTED, "scritto male",
     HEAD "statement (= (sum k 1 n k) (/ (* n (+ nn 1)) 2))\nfactor 1\nend\n"},

    /* Certificati che il verificatore non sa controllare. */
    {"sum whose term depends on n", CK_UNSUPPORTED, "dipende dalla variabile di induzione",
     HEAD "statement (= (sum k 1 n (* n k)) (sum j 1 n (* j j)))\nfactor 1\nend\n"},
    {"sum whose term depends on n through a nested sum", CK_UNSUPPORTED, "dipende dalla variabile di induzione",
     HEAD "statement (= (sum k 1 n (* (sum i 1 n 1) k)) (sum j 1 n (* j j)))\nfactor 1\nend\n"},
    {"definition that hides n: once accepted, and false", CK_UNSUPPORTED, "define",
     HEAD "define G m (* n m)\nstatement (= (sum k 1 n (seq G k)) (/ (* (* n (+ n 1)) (+ (* 2 n) 1)) 6))\nfactor 1\nend\n"},
    {"definition that names a letter other than its parameter", CK_UNSUPPORTED, "define",
     HEAD "define G m (* k m)\nstatement (= (sum k 1 n (seq G k)) (sum j 1 n (* j j)))\nfactor 1\nend\n"},
    {"definition whose parameter is the induction variable", CK_UNSUPPORTED, "define",
     HEAD "define G n (* n n)\nstatement (= (sum k 1 n (seq G k)) (sum j 1 n (* j j)))\nfactor 1\nend\n"},
    {"inner sum whose term depends on the outer index", CK_UNSUPPORTED, "altro indice",
     HEAD "statement (= (sum k 1 n (sum i 1 k (* k i))) (sum k 1 n (* k (sum i 1 k i))))\nfactor 1\nend\n"},
    {"sum whose lower bound depends on n", CK_UNSUPPORTED, "estremo inferiore",
     HEAD "statement (= (sum k n (* 2 n) 1) (+ n 1))\nfactor 1\nend\n"},
    {"sum that starts after its last term at the base case", CK_UNSUPPORTED, "estremi",
     HEAD "statement (= (sum k 3 n k) (- (/ (* n (+ n 1)) 2) 3))\nfactor 1\nend\n"},
    {"factor that names a sum with no meaning at the base case", CK_UNSUPPORTED, "ultimo termine",
     HEAD GAUSS "factor (/ (sum k 5 n k) (sum k 5 n k))\nend\n"},
    {"sequence with a negative index at the base case", CK_UNSUPPORTED, "successione",
     HEAD FIBONACCI "statement (= (seq F (- n 2)) (- (seq F n) (seq F (- n 1))))\nfactor 0\nend\n"},
    {"power with n both in the base and in the exponent", CK_UNSUPPORTED, "soli parametri",
     HEAD "statement (= (^ (+ n 1) n) (^ (+ n 1) n))\nfactor 0\nend\n"},
    {"zero to a power that depends on n", CK_UNSUPPORTED, "zero elevato",
     HEAD "statement (= (^ 0 n) 0)\nfactor 0\nend\n"},
    {"exponent that is not an integer", CK_UNSUPPORTED, "esponente",
     HEAD "statement (= (^ x (/ n 2)) (^ x (/ n 2)))\nfactor 0\nend\n"},
    {"an operator the checker does not know", CK_UNSUPPORTED, "operatore",
     HEAD "statement (= (sin n) (sin n))\nfactor 0\nend\n"},
    {"an inequality", CK_UNSUPPORTED, "uguaglianza",
     HEAD "statement (<= n (* 2 n))\nfactor 0\nend\n"},
    {"a sequence the certificate does not define", CK_UNSUPPORTED, "non definisce",
     HEAD "statement (= (seq F n) (seq F n))\nfactor 0\nend\n"},
    {"numbers beyond 128 bits", CK_UNSUPPORTED, "troppo grand",
     HEAD "statement (= (* n (^ 10 40)) (* n (^ 10 40)))\nfactor 0\nend\n"},
    {"a base case too large to work with", CK_UNSUPPORTED, "caso base",
     "certificate induction\nvariable n\nbase 100000000000\n" GAUSS "factor 1\nend\n"},
};

static int checks, failures;

/* Registra l'esito di un controllo e stampa quelli falliti. */
static void check(int passed, const char *description) {
    checks++;
    if (passed) return;
    failures++;
    printf("  FAILED: %s\n", description);
}

/* Ritorna la risposta del verificatore sul certificato text; mette in *why
 * il motivo. */
static CkAnswer answer_of(const char *text, const char **why) {
    return ck_check(&text, why);
}

/* Ritorna il nome inglese di una risposta, per i messaggi. */
static const char *answer_name(CkAnswer answer) {
    return answer == CK_ACCEPTED ? "accepted" : answer == CK_REJECTED ? "rejected" : "unsupported";
}

int main(void) {
    char message[512];

    /* La tabella: risposta attesa, e il motivo per chi non è accettato. */
    for (size_t i = 0; i < sizeof cases / sizeof *cases; i++) {
        const char *why;
        CkAnswer answer = answer_of(cases[i].text, &why);
        snprintf(message, sizeof message, "\"%s\" should be %s but is %s (%s)", cases[i].name,
                 answer_name(cases[i].expected), answer_name(answer), why);
        check(answer == cases[i].expected, message);
        snprintf(message, sizeof message, "\"%s\" should mention \"%s\" as its reason, not \"%s\"", cases[i].name, cases[i].why, why);
        check(answer != cases[i].expected || strstr(why, cases[i].why) != NULL, message);
    }

    /* Una risposta non lascia tracce nella successiva: dopo un certificato
     * con le sue successioni e le sue somme, uno che non le definisce resta
     * non controllabile, e uno giusto resta accettato. */
    const char *why;
    answer_of(HEAD FIBONACCI "statement (= (sum k 1 n (seq F k)) (- (seq F (+ n 2)) 1))\nfactor 1\nend\n", &why);
    check(answer_of(HEAD "statement (= (seq F n) (seq F n))\nfactor 0\nend\n", &why) == CK_UNSUPPORTED,
          "a sequence defined by an earlier certificate should not be known to the next one");
    answer_of(HEAD "statement (= (sum k 1 n k) n)\nfactor 1\nend\n", &why);
    check(answer_of(HEAD GAUSS "factor 1\nend\n", &why) == CK_ACCEPTED, "a rejection should not affect the next certificate");

    /* ck_check() avanza fino a dopo "end": due certificati in un testo solo
     * si leggono uno dopo l'altro. */
    const char *two = HEAD GAUSS "factor 1\nend\n" HEAD "statement (= (sum k 1 n k) n)\nfactor 1\nend\n";
    check(ck_check(&two, &why) == CK_ACCEPTED, "the first of two certificates should be accepted");
    check(ck_check(&two, &why) == CK_REJECTED, "the second of two certificates should be read next, and rejected");
    while (*two == '\n') two++;
    check(*two == 0, "after two certificates the text should be finished");

    /* Il programma intero: un file che non esiste è un errore d'uso, non
     * un "nessun certificato respinto". */
    char *arguments[] = {"checker", "tests/checker/no_such_file.cert", NULL};
    check(checker_main(2, arguments) == 2, "the checker program should exit with 2 on a missing file");
    check(checker_main(1, arguments) == 2, "the checker program should exit with 2 without a file");

    printf("%s: %d of %d checker checks passed\n", failures ? "FAIL" : "PASS", checks - failures, checks);
    return failures ? 1 : 0;
}
