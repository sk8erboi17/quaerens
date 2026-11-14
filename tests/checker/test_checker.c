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
 *   - accettato: un'identità vera con un fattore giusto e, se c'è la voce
 *     "defined", definita per ogni n;
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

    /* La voce "defined": l'enunciato è anche definito per ogni n. */
    {"defined: no division at all", CK_ACCEPTED, "",
     HEAD GAUSS "factor 1\ndefined\nend\n"},
    {"defined: denominators k (k + 1) inside the terms of a sum", CK_ACCEPTED, "",
     HEAD "statement (= (sum k 1 n (/ 1 (* k (+ k 1)))) (/ n (+ n 1)))\nfactor 1\ndefined\nend\n"},
    {"defined: a factorial in a denominator", CK_ACCEPTED, "",
     HEAD "statement (= (sum k 1 n (/ k (fact (+ k 1)))) (- 1 (/ 1 (fact (+ n 1)))))\nfactor 1\ndefined\nend\n"},
    {"defined: a product with a rational factor", CK_ACCEPTED, "",
     HEAD "statement (= (prod k 1 n (/ (+ k 1) k)) (+ n 1))\nfactor (/ (+ n 2) (+ n 1))\ndefined\nend\n"},
    {"defined: a parameter fixed away from the pole", CK_ACCEPTED, "",
     "certificate induction\nvariable n\nbase 0\n"
     "statement (= (sum k 0 n (^ x k)) (/ (- (^ x (+ n 1)) 1) (- x 1)))\nfactor 1\ndefined x 2\nend\n"},
    {"defined: a parameter fixed at a fraction", CK_ACCEPTED, "",
     "certificate induction\nvariable n\nbase 0\n"
     "statement (= (sum k 0 n (^ x k)) (/ (- (^ x (+ n 1)) 1) (- x 1)))\nfactor 1\ndefined x (/ 1 2)\nend\n"},
    {"defined: 2^n + n in a denominator, terms of one sign", CK_ACCEPTED, "",
     HEAD "statement (= (/ (+ (^ 2 n) n) (+ (^ 2 n) n)) 1)\nfactor 1\ndefined\nend\n"},
    {"defined: 2^n - 4096 in a denominator, from n = 13 on", CK_ACCEPTED, "",
     "certificate induction\nvariable n\nbase 13\nstatement (= (/ (- (^ 2 n) 4096) (- (^ 2 n) 4096)) 1)\nfactor 1\ndefined\nend\n"},
    {"defined: n + (-1)^n in a denominator, from n = 2 on", CK_ACCEPTED, "",
     "certificate induction\nvariable n\nbase 2\nstatement (= (/ 1 (+ n (^ -1 n))) (/ 1 (+ n (^ -1 n))))\nfactor 0\ndefined\nend\n"},
    {"defined: (n - 50)/(n - 50) from n = 51 on", CK_ACCEPTED, "",
     "certificate induction\nvariable n\nbase 51\nstatement (= (/ (- n 50) (- n 50)) 1)\nfactor 1\ndefined\nend\n"},

    /* La voce "defined" quando è falsa: il verificatore dice dove. */
    {"defined, but (n - 50)/(n - 50) has no value at n = 50", CK_REJECTED, "n = 50",
     HEAD "statement (= (/ (- n 50) (- n 50)) 1)\nfactor 1\ndefined\nend\n"},
    {"defined, but a term of the sum divides by zero at k = 3", CK_REJECTED, "n = 3",
     HEAD "statement (= (sum k 1 n (/ 1 (- k 3))) (sum k 1 n (/ 1 (- k 3))))\nfactor 0\ndefined\nend\n"},
    {"defined, but 2^n - 4096 vanishes at n = 12", CK_REJECTED, "n = 12",
     HEAD "statement (= (/ (- (^ 2 n) 4096) (- (^ 2 n) 4096)) 1)\nfactor 1\ndefined\nend\n"},
    {"defined, but a product in a denominator has a zero factor at k = 4", CK_REJECTED, "n = 4",
     HEAD "statement (= (/ 1 (prod k 1 n (- k 4))) (/ 1 (prod k 1 n (- k 4))))\nfactor 0\ndefined\nend\n"},
    {"defined, but n^2 - 1681 vanishes at n = 41", CK_REJECTED, "n = 41",
     HEAD "statement (= (/ n (- (^ n 2) 1681)) (/ n (- (^ n 2) 1681)))\nfactor 0\ndefined\nend\n"},
    {"defined, but n - 7 + (-1)^n + 1 vanishes at the odd n = 7", CK_REJECTED, "n = 7",
     HEAD "statement (= (/ 1 (+ (- n 6) (^ -1 n))) (/ 1 (+ (- n 6) (^ -1 n))))\nfactor 0\ndefined\nend\n"},
    {"defined, with the parameter fixed exactly at the pole", CK_REJECTED, "divide per zero",
     "certificate induction\nvariable n\nbase 0\n"
     "statement (= (sum k 0 n (^ x k)) (/ (- (^ x (+ n 1)) 1) (- x 1)))\nfactor 1\ndefined x 1\nend\n"},
    {"defined, with the parameter fixed where n - x vanishes later", CK_REJECTED, "n = 30",
     HEAD "statement (= (/ 1 (- n x)) (/ 1 (- n x)))\nfactor 0\ndefined x 30\nend\n"},
    {"undefined at the base case itself", CK_REJECTED, "caso base",
     HEAD "statement (= (/ 1 (+ n (^ -1 n))) (/ 1 (+ n (^ -1 n))))\nfactor 0\nend\n"},
    {"defined gives a value to the induction variable", CK_REJECTED, "variabile di induzione",
     HEAD GAUSS "factor 1\ndefined n 3\nend\n"},
    {"defined gives a parameter a value that is not a number", CK_REJECTED, "non è un numero",
     HEAD GAUSS "factor 1\ndefined x y\nend\n"},
    {"defined twice", CK_REJECTED, "due volte",
     HEAD GAUSS "factor 1\ndefined\ndefined\nend\n"},

    /* La voce "defined" quando il verificatore non sa decidere: non è un sì. */
    {"defined: a sum in a denominator", CK_UNSUPPORTED, "non so stabilire",
     HEAD HARMONIC "statement (= (/ 1 (seq H n)) (/ 1 (seq H n)))\nfactor 0\ndefined\nend\n"},
    {"defined: 3^n - 2^n in a denominator", CK_UNSUPPORTED, "non so stabilire",
     HEAD "statement (= (/ 1 (- (^ 3 n) (^ 2 n))) (/ 1 (- (^ 3 n) (^ 2 n))))\nfactor 0\ndefined\nend\n"},
    {"defined: a parameter in a denominator left without a value", CK_UNSUPPORTED, "non so stabilire",
     "certificate induction\nvariable n\nbase 0\n"
     "statement (= (sum k 0 n (^ x k)) (/ (- (^ x (+ n 1)) 1) (- x 1)))\nfactor 1\ndefined\nend\n"},
    {"defined: a Fibonacci number in a denominator", CK_UNSUPPORTED, "non so stabilire",
     HEAD FIBONACCI "statement (= (/ 1 (seq F n)) (/ 1 (seq F n)))\nfactor 0\ndefined\nend\n"},
    {"defined: a generic entry that is zero at the given point", CK_UNSUPPORTED, "generic",
     HEAD "statement (= (* x (prod k 1 n (/ (+ k 1) k))) (* x (+ n 1)))\n"
     "factor (/ (* x (+ n 2)) (* x (+ n 1)))\ngeneric x\ndefined x 0\nend\n"},

    /* Logaritmi: le regole vere. I fattori da riconoscere li suggeriscono
     * le voci "brick". */
    {"logarithm: telescoping sum of ln(1 + 1/k)", CK_ACCEPTED, "",
     HEAD "statement (= (sum k 1 n (ln (+ 1 (/ 1 k)))) (ln (+ n 1)))\nfactor 1\nbrick (+ n 1)\nbrick (+ n 2)\nend\n"},
    {"logarithm: sum of ln(1 - 1/k^2), with n (n + 2) to be split", CK_ACCEPTED, "",
     "certificate induction\nvariable n\nbase 2\n"
     "statement (= (sum k 2 n (ln (- 1 (/ 1 (^ k 2))))) (- (ln (+ n 1)) (+ (ln n) (ln 2))))\nfactor 1\n"
     "brick (+ n 1)\nbrick (+ n 2)\nend\n"},
    {"logarithm of a product is the sum of the logarithms", CK_ACCEPTED, "",
     HEAD "statement (= (ln (* x n)) (+ (ln x) (ln n)))\nfactor 0\nend\n"},
    {"logarithm of a power: ln(n^3) = 3 ln n", CK_ACCEPTED, "",
     HEAD "statement (= (ln (^ n 3)) (* 3 (ln n)))\nfactor 0\nend\n"},
    {"logarithm of a quotient: ln(n/2) = ln n - ln 2", CK_ACCEPTED, "",
     HEAD "statement (= (ln (/ n 2)) (- (ln n) (ln 2)))\nfactor 0\nend\n"},
    {"logarithm of a number through its prime factors: ln 12 = 2 ln 2 + ln 3", CK_ACCEPTED, "",
     HEAD "statement (= (ln 12) (+ (* 2 (ln 2)) (ln 3)))\nfactor 0\nend\n"},
    {"logarithm of 2^n is n ln 2", CK_ACCEPTED, "",
     HEAD "statement (= (ln (^ 2 n)) (* n (ln 2)))\nfactor 0\nend\n"},
    {"logarithm of x^n is n ln x", CK_ACCEPTED, "",
     HEAD "statement (= (ln (^ x n)) (* n (ln x)))\nfactor 0\nend\n"},
    {"logarithm of n^2 + 3n + 2, split with the suggested factors", CK_ACCEPTED, "",
     HEAD "statement (= (ln (+ (^ n 2) (+ (* 3 n) 2))) (+ (ln (+ n 1)) (ln (+ n 2))))\nfactor 0\nbrick (+ n 1)\nbrick (+ n 2)\nend\n"},
    {"logarithm of a product over k is the sum of the logarithms", CK_ACCEPTED, "",
     HEAD "statement (= (ln (prod k 1 n k)) (sum k 1 n (ln k)))\nfactor 1\nbrick (+ n 1)\nend\n"},
    {"a suggested factor that divides nothing has no effect", CK_ACCEPTED, "",
     HEAD "statement (= (ln (* x n)) (+ (ln x) (ln n)))\nfactor 0\nbrick (+ n 77)\nend\n"},

    /* Logaritmi: regole che somigliano a quelle vere, e sono false. */
    {"false: ln(n + 2) = ln n + ln 2", CK_REJECTED, "caso base",
     HEAD "statement (= (ln (+ n 2)) (+ (ln n) (ln 2)))\nfactor 0\nend\n"},
    {"false: ln(2n) = 2 ln n", CK_REJECTED, "caso base",
     HEAD "statement (= (ln (* 2 n)) (* 2 (ln n)))\nfactor 0\nend\n"},
    {"false: ln(n^2) = (ln n)^2", CK_REJECTED, "passo induttivo",
     HEAD "statement (= (ln (^ n 2)) (^ (ln n) 2))\nfactor 0\nend\n"},
    {"false: ln(n (n + 1)) = ln n ln(n + 1)", CK_REJECTED, "caso base",
     HEAD "statement (= (ln (* n (+ n 1))) (* (ln n) (ln (+ n 1))))\nfactor 0\nend\n"},
    {"false: ln(n/2) = ln n / ln 2", CK_REJECTED, "caso base",
     HEAD "statement (= (ln (/ n 2)) (/ (ln n) (ln 2)))\nfactor 0\nend\n"},
    {"false: ln(n/2) = ln n + ln 2, the sign of the denominator", CK_REJECTED, "caso base",
     HEAD "statement (= (ln (/ n 2)) (+ (ln n) (ln 2)))\nfactor 0\nend\n"},
    {"false: ln(2^n) = ln 2, true only at the base case", CK_REJECTED, "passo induttivo",
     HEAD "statement (= (ln (^ 2 n)) (ln 2))\nfactor 0\nend\n"},
    {"false: ln(x^n) = n + ln x", CK_REJECTED, "caso base",
     HEAD "statement (= (ln (^ x n)) (+ n (ln x)))\nfactor 0\nend\n"},
    {"false: ln(x^2) = ln x, the exponent forgotten", CK_REJECTED, "caso base",
     HEAD "statement (= (ln (^ x 2)) (ln x))\nfactor 0\nend\n"},
    {"false: the sum of the logarithms is the logarithm of the sum", CK_REJECTED, "passo induttivo",
     HEAD "statement (= (sum k 1 n (ln k)) (ln (sum k 1 n k)))\nfactor 1\nend\n"},
    {"false: ln(n!) = n ln n", CK_REJECTED, "passo induttivo",
     HEAD "statement (= (ln (fact n)) (* n (ln n)))\nfactor 0\nend\n"},
    {"false: ln(n^2 + 3n + 2) = ln(n + 1) + ln(n + 3), with misleading suggestions", CK_REJECTED, "caso base",
     HEAD "statement (= (ln (+ (^ n 2) (+ (* 3 n) 2))) (+ (ln (+ n 1)) (ln (+ n 3))))\nfactor 0\nbrick (+ n 1)\nbrick (+ n 3)\nend\n"},
    {"logarithm of an expression that is always zero", CK_REJECTED, "logaritmo",
     HEAD "statement (= (ln (- n n)) 0)\nfactor 0\nend\n"},
    {"a suggested factor that is a constant", CK_REJECTED, "brick",
     HEAD "statement (= (ln n) (ln n))\nfactor 0\nbrick 2\nend\n"},

    /* Logaritmi: ciò che il verificatore non sa stabilire. */
    {"a power of a logarithm with n in the exponent", CK_UNSUPPORTED, "soli parametri",
     HEAD "statement (= (ln (^ x n)) (^ (ln x) n))\nfactor 0\nend\n"},
    {"true, but not recognised without the suggested factors", CK_REJECTED, "passo induttivo",
     HEAD "statement (= (ln (+ (^ n 2) (+ (* 3 n) 2))) (+ (ln (+ n 1)) (ln (+ n 2))))\nfactor 0\nend\n"},
    {"defined: the sign of the argument of a logarithm", CK_UNSUPPORTED, "logaritmo",
     HEAD "statement (= (sum k 1 n (ln (+ 1 (/ 1 k)))) (ln (+ n 1)))\nfactor 1\nbrick (+ n 1)\nbrick (+ n 2)\ndefined\nend\n"},

    /* Enunciati sugli insiemi: il verificatore li decide da capo. */
    {"sets: a difference of a difference", CK_ACCEPTED, "",
     "certificate sets\nstatement (= (diff (diff S T) V) (diff S (union T V)))\nend\n"},
    {"sets: an equivalence between two inclusions", CK_ACCEPTED, "",
     "certificate sets\nstatement (iff (subset (inter S T) V) (subset S (union V (diff S T))))\nend\n"},
    {"sets: symmetric differences of four sets", CK_ACCEPTED, "",
     "certificate sets\nstatement (= (symdiff (symdiff S T) (symdiff V W)) (symdiff (symdiff S V) (symdiff T W)))\nend\n"},
    {"sets: a product distributes over a union", CK_ACCEPTED, "",
     "certificate sets\nstatement (= (times S (union T V)) (union (times S T) (times S V)))\nend\n"},
    {"sets: an empty product has an empty factor", CK_ACCEPTED, "",
     "certificate sets\nstatement (implies (= (times S T) (empty)) (or (= S (empty)) (= T (empty))))\nend\n"},
    {"sets: the power set of an intersection", CK_ACCEPTED, "",
     "certificate sets\nstatement (= (inter (power S) (power T)) (power (inter S T)))\nend\n"},
    {"sets: the union of two power sets is inside the power set of the union", CK_ACCEPTED, "",
     "certificate sets\nstatement (subset (union (power S) (power T)) (power (union S T)))\nend\n"},
    {"sets: inclusion of sets and of their power sets", CK_ACCEPTED, "",
     "certificate sets\nstatement (iff (subset S T) (subset (power S) (power T)))\nend\n"},
    {"sets: the power set of the empty set", CK_ACCEPTED, "",
     "certificate sets\nstatement (= (power (empty)) (onlyempty))\nend\n"},
    {"sets: false, a union is not its first set", CK_REJECTED, "S = {}, T = {2}",
     "certificate sets\nstatement (= (union S T) S)\nend\n"},
    {"sets: false, a product does not commute", CK_REJECTED, "falso",
     "certificate sets\nstatement (= (times S T) (times T S))\nend\n"},
    {"sets: false, equal products do not give equal sets when one is empty", CK_REJECTED, "falso",
     "certificate sets\nstatement (implies (= (times S T) (times T S)) (= S T))\nend\n"},
    {"sets: false, the power set of a union is larger than the union of the power sets", CK_REJECTED, "falso",
     "certificate sets\nstatement (= (union (power S) (power T)) (power (union S T)))\nend\n"},
    {"sets: false, the power set of the empty set is not empty", CK_REJECTED, "falso",
     "certificate sets\nstatement (= (power (empty)) (empty))\nend\n"},
    {"sets: false, the difference is not symmetric", CK_REJECTED, "falso",
     "certificate sets\nstatement (= (diff S T) (diff T S))\nend\n"},
    {"sets: objects of different types compared", CK_UNSUPPORTED, "tipi diversi",
     "certificate sets\nstatement (= (power S) (times S T))\nend\n"},
    {"sets: a power set of a power set", CK_UNSUPPORTED, "tipi diversi",
     "certificate sets\nstatement (= (power (power S)) (power (power S)))\nend\n"},
    {"sets: five sets", CK_UNSUPPORTED, "quattro insiemi",
     "certificate sets\nstatement (= (union (union S T) (union V W)) (union X X))\nend\n"},
    {"sets: a word of the induction certificates", CK_UNSUPPORTED, "tipi diversi",
     "certificate sets\nstatement (= (+ S T) S)\nend\n"},
    {"sets: an expression where a statement is expected", CK_UNSUPPORTED, "tipi diversi",
     "certificate sets\nstatement (union S T)\nend\n"},
    {"sets: missing end", CK_REJECTED, "scritto male",
     "certificate sets\nstatement (= S S)\n"},
    {"a word of the set statements in an induction certificate", CK_UNSUPPORTED, "insiemi",
     HEAD "statement (= (union n n) n)\nfactor 0\nend\n"},

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
    return ck_check(&text, why, 0);
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
    check(ck_check(&two, &why, 0) == CK_ACCEPTED, "the first of two certificates should be accepted");
    check(ck_check(&two, &why, 0) == CK_REJECTED, "the second of two certificates should be read next, and rejected");
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
