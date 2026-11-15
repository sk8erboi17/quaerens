/* Test del verificatore sui certificati delle identità con gli indici
 * (capitolo 19 di checker.c), da solo: qui il motore non c'entra.
 *
 * Compilazione ed esecuzione, dalla cartella principale del progetto:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/checker checker.c && \
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_index_certificates \
 *         tests/checker/test_index_certificates.c && /tmp/test_index_certificates
 *
 * Il primo comando fa parte del test: checker.c deve compilarsi da solo e
 * senza avvisi. Per lo stesso motivo questo file include soltanto checker.c.
 *
 * I certificati sono scritti a mano, e ognuno ha la risposta attesa e, se
 * non è accettato, un pezzo del motivo: un certificato respinto per la
 * ragione sbagliata non proverebbe nulla. I casi respinti e non
 * controllabili sono antiregressione: non si tolgono e non si indeboliscono
 * per far passare una modifica al verificatore.
 */

#include <stdio.h>
#include <string.h>

/* Il verificatore è un programma con il suo main(): lo si include cambiando
 * nome a quel main. */
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

/* Le voci comuni, e gli enunciati usati più volte. */
#define HEAD "certificate indices\ndimension 3\n"
#define CONTRACTION "statement (= (* (eps i j k) (eps i m n)) (- (* (delta j m) (delta k n)) (* (delta j n) (delta k m))))\n"
#define ODD "statement (= (eps i j k) (eps j i k))\n"
#define CROSS "statement (= (* (* (eps i j k) (t a j)) (t b k)) 0)\n"

static const Case cases[] = {
    /* Identità vere, affermate vere. */
    {"contraction of two Levi-Civita symbols", CK_ACCEPTED, "", HEAD CONTRACTION "holds\nend\n"},
    {"entries in a different order", CK_ACCEPTED, "", "certificate indices\nholds\n" CONTRACTION "dimension 3\nend\n"},
    {"trace of the Kronecker delta in three dimensions", CK_ACCEPTED, "", HEAD "statement (= (delta i i) 3)\nholds\nend\n"},
    {"trace of the Kronecker delta in one dimension", CK_ACCEPTED, "", "certificate indices\ndimension 1\nstatement (= (delta i i) 1)\nholds\nend\n"},
    {"full contraction in two dimensions", CK_ACCEPTED, "", "certificate indices\ndimension 2\nstatement (= (* (eps i j) (eps i j)) 2)\nholds\nend\n"},
    {"full contraction in four dimensions", CK_ACCEPTED, "",
     "certificate indices\ndimension 4\nstatement (= (* (eps i j k l) (eps i j k l)) 24)\nholds\nend\n"},
    {"a rational coefficient", CK_ACCEPTED, "", HEAD "statement (= (* (frac 1 2) (* (eps i j k) (eps i j k))) 3)\nholds\nend\n"},
    {"numeric indices", CK_ACCEPTED, "", HEAD "statement (= (eps 2 3 1) 1)\nholds\nend\n"},
    {"a negated symbol", CK_ACCEPTED, "", HEAD "statement (= (eps i j k) (neg (eps j i k)))\nholds\nend\n"},
    {"the delta substitutes an index of an arbitrary tensor", CK_ACCEPTED, "", HEAD "statement (= (* (delta i j) (t a j)) (t a i))\nholds\nend\n"},
    {"double cross product of arbitrary vectors", CK_ACCEPTED, "",
     HEAD "statement (= (* (* (* (* (eps i j k) (eps k l m)) (t a j)) (t b l)) (t c m)) "
          "(- (* (t b i) (* (t a m) (t c m))) (* (t c i) (* (t a l) (t b l)))))\nholds\nend\n"},
    {"zero alone is the zero tensor", CK_ACCEPTED, "", HEAD "statement (= (* (* (eps i j k) (t a j)) (t a k)) 0)\nholds\nend\n"},
    {"zero alone as an addend", CK_ACCEPTED, "", HEAD "statement (= (+ (* (delta i j) (t a j)) 0) (t a i))\nholds\nend\n"},
    {"a scalar, a tensor without indices", CK_ACCEPTED, "", HEAD "statement (= (* (t c) (* (delta i j) (t a j))) (* (t a i) (t c)))\nholds\nend\n"},
    {"an index repeated inside one symbol is summed there", CK_ACCEPTED, "", HEAD "statement (= (+ (t T i i) (delta j j)) (+ 3 (t T k k)))\nholds\nend\n"},
    {"the same letter may be summed in two addends", CK_ACCEPTED, "", HEAD "statement (= (+ (* (t a i) (t b i)) (* (t c i) (t d i))) (+ (* (t c j) (t d j)) (* (t b j) (t a j))))\nholds\nend\n"},
    {"a tensor name and an index may be the same letter", CK_ACCEPTED, "", HEAD "statement (= (* (delta i j) (t i j)) (t i i))\nholds\nend\n"},
    {"a component with a two-digit index", CK_ACCEPTED, "", "certificate indices\ndimension 12\nstatement (= (* (delta i 12) (t a i)) (t a 12))\nholds\nend\n"},

    /* Con la voce metric: la metrica diagonale e la sua inversa. */
    {"the metric contracted with its inverse is the dimension", CK_ACCEPTED, "",
     "certificate indices\ndimension 4\nmetric -1 1 1 1\nstatement (= (* (metric i j) (inverse i j)) 4)\nholds\nend\n"},
    {"the metric times its inverse is the delta", CK_ACCEPTED, "",
     "certificate indices\ndimension 2\nmetric 2 (frac 1 3)\nstatement (= (* (inverse i j) (metric j k)) (delta i k))\nholds\nend\n"},
    {"the Levi-Civita symbol raised with a Lorentzian metric changes sign", CK_ACCEPTED, "",
     "certificate indices\ndimension 2\nmetric -1 1\nstatement (= (* (* (inverse j b) (* (inverse i a) (eps a b))) (eps i j)) -2)\nholds\nend\n"},
    {"the entry metric may come after the statement", CK_ACCEPTED, "",
     "certificate indices\ndimension 2\nstatement (= (* (metric i j) (t A j)) (* (metric i a) (t A a)))\nmetric -1 1\nholds\nend\n"},
    {"the values of the metric with numeric indices", CK_ACCEPTED, "",
     "certificate indices\ndimension 2\nmetric 2 3\nstatement (= (+ (metric 1 1) (* 6 (inverse 2 2))) 4)\nholds\nend\n"},
    {"the Lorentzian sign with the Euclidean value", CK_REJECTED, "sono diversi",
     "certificate indices\ndimension 2\nmetric -1 1\nstatement (= (* (* (inverse j b) (* (inverse i a) (eps a b))) (eps i j)) 2)\nholds\nend\n"},
    {"the metric taken for its inverse", CK_REJECTED, "sono diversi",
     "certificate indices\ndimension 2\nmetric 2 3\nstatement (= (* (metric i j) (metric j k)) (delta i k))\nholds\nend\n"},
    {"the metric is diagonal: an entry off the diagonal is zero", CK_REJECTED, "sono diversi",
     "certificate indices\ndimension 2\nmetric 2 3\nstatement (= (metric 1 2) 2)\nholds\nend\n"},
    {"a different metric gives a different trace", CK_REJECTED, "sono diversi",
     "certificate indices\ndimension 2\nmetric 2 3\nstatement (= (metric i i) 6)\nholds\nend\n"},
    {"the metric without its entry", CK_REJECTED, "senza la voce metric", HEAD "statement (= (* (metric i j) (inverse i j)) 3)\nholds\nend\n"},
    {"a metric with a zero on the diagonal", CK_REJECTED, "diversi da zero",
     "certificate indices\ndimension 2\nmetric 1 0\nstatement (= (metric i i) 1)\nholds\nend\n"},
    {"a metric with fewer entries than the dimension", CK_REJECTED, "tanti numeri quante sono le dimensioni",
     "certificate indices\ndimension 3\nmetric 1 1\nstatement (= (metric i i) 2)\nholds\nend\n"},
    {"a metric with fewer entries is not completed with ones", CK_REJECTED, "tanti numeri quante sono le dimensioni",
     "certificate indices\ndimension 3\nmetric 1 1\nstatement (= (metric i i) 3)\nholds\nend\n"},
    {"a metric with more entries than the dimension", CK_REJECTED, "tanti numeri quante sono le dimensioni",
     "certificate indices\ndimension 2\nmetric 1 1 5\nstatement (= (metric i i) 2)\nholds\nend\n"},
    {"a metric with no entries", CK_REJECTED, "tanti numeri quante sono le dimensioni",
     "certificate indices\ndimension 2\nmetric\nstatement (= (delta i i) 2)\nholds\nend\n"},
    {"a zero on the diagonal is refused even where the certificate does not use it", CK_REJECTED, "diversi da zero",
     "certificate indices\ndimension 2\nmetric 0 1\nstatement (= (metric 2 2) 1)\nholds\nend\n"},
    {"a metric with three indices", CK_REJECTED, "due indici", "certificate indices\ndimension 2\nmetric 1 1\nstatement (= (metric i i i) 2)\nholds\nend\n"},
    {"two entries metric", CK_REJECTED, "due volte", "certificate indices\ndimension 2\nmetric 1 1\nmetric 1 1\nstatement (= (metric i i) 2)\nholds\nend\n"},
    {"an entry metric with a letter", CK_REJECTED, "voce metric", "certificate indices\ndimension 2\nmetric 1 (t a)\nstatement (= (metric i i) 2)\nholds\nend\n"},

    /* Identità false, affermate false nel punto giusto. */
    {"a refutation with the two values", CK_ACCEPTED, "", HEAD ODD "fails i 1 j 2 k 3\nleft 1\nright -1\nend\n"},
    {"a refutation without the two values", CK_ACCEPTED, "", HEAD ODD "fails i 1 j 2 k 3\nend\n"},
    {"a refutation without free indices", CK_ACCEPTED, "", HEAD "statement (= (* (eps i j k) (eps i j k)) 3)\nfails\nleft 6\nright 3\nend\n"},
    {"a refutation whose values are polynomials in the components", CK_ACCEPTED, "",
     HEAD CROSS "fails i 1\nleft (- (* (t a 2) (t b 3)) (* (t a 3) (t b 2)))\nright 0\nend\n"},
    {"the written value may be any expression with that value", CK_ACCEPTED, "",
     HEAD CROSS "fails i 1\nleft (+ (* (t b 3) (t a 2)) (neg (* (t b 2) (t a 3))))\nend\n"},

    /* Identità false affermate vere: un segno, un fattore, la dimensione. */
    {"a wrong sign", CK_REJECTED, "sono diversi",
     HEAD "statement (= (* (eps i j k) (eps i m n)) (+ (* (delta j m) (delta k n)) (* (delta j n) (delta k m))))\nholds\nend\n"},
    {"two exchanged indices", CK_REJECTED, "sono diversi",
     HEAD "statement (= (* (eps i j k) (eps i m n)) (- (* (delta j n) (delta k m)) (* (delta j m) (delta k n))))\nholds\nend\n"},
    {"a wrong factor", CK_REJECTED, "sono diversi", HEAD "statement (= (* (eps i j k) (eps i j k)) 3)\nholds\nend\n"},
    {"the trace of the delta in the wrong dimension", CK_REJECTED, "sono diversi", "certificate indices\ndimension 2\nstatement (= (delta i i) 3)\nholds\nend\n"},
    {"an odd permutation taken as even", CK_REJECTED, "sono diversi", HEAD ODD "holds\nend\n"},
    {"two different arbitrary tensors taken as equal", CK_REJECTED, "sono diversi", HEAD "statement (= (* (t a i) (t b i)) (* (t a i) (t a i)))\nholds\nend\n"},
    {"a commutative cross product", CK_REJECTED, "sono diversi",
     HEAD "statement (= (* (* (eps i j k) (t a j)) (t b k)) (* (* (eps i j k) (t b j)) (t a k)))\nholds\nend\n"},
    {"an identity that fails only in the last point", CK_REJECTED, "sono diversi", HEAD "statement (= (* (delta i 3) (delta j 3)) 0)\nholds\nend\n"},

    /* Identità vere affermate false, e punti o valori sbagliati. */
    {"a true identity claimed to fail", CK_REJECTED, "sono uguali", HEAD CONTRACTION "fails j 1 k 2 m 1 n 2\nend\n"},
    {"a false identity refuted in a point where it holds", CK_REJECTED, "sono uguali", HEAD ODD "fails i 1 j 1 k 2\nend\n"},
    {"a wrong written value of the left side", CK_REJECTED, "valore scritto", HEAD ODD "fails i 1 j 2 k 3\nleft -1\nright -1\nend\n"},
    {"a wrong written value of the right side", CK_REJECTED, "valore scritto", HEAD ODD "fails i 1 j 2 k 3\nleft 1\nright 1\nend\n"},
    {"a written polynomial with a wrong sign", CK_REJECTED, "valore scritto",
     HEAD CROSS "fails i 1\nleft (+ (* (t a 2) (t b 3)) (* (t a 3) (t b 2)))\nright 0\nend\n"},
    {"a refutation that leaves a free index without a value", CK_REJECTED, "ogni indice libero", HEAD ODD "fails i 1 j 2\nend\n"},
    {"a refutation that gives a value to a summed index", CK_REJECTED, "ogni indice libero", HEAD "statement (= (delta i i) 2)\nfails i 1\nend\n"},
    {"a point outside the values of the indices", CK_REJECTED, "fuori da 1", HEAD ODD "fails i 1 j 2 k 4\nend\n"},
    {"written values with a free index", CK_REJECTED, "indici liberi", HEAD CROSS "fails i 1\nleft (t a i)\nend\n"},
    {"written values on a claim that the identity holds", CK_REJECTED, "accompagnano fails", HEAD CONTRACTION "holds\nleft 1\nend\n"},

    /* La convenzione degli indici e la forma dei simboli. */
    {"an index three times in a product", CK_REJECTED, "più di due volte", HEAD "statement (= (* (* (t a i) (t b i)) (t c i)) 0)\nholds\nend\n"},
    {"an index summed in a factor and used in the other", CK_REJECTED, "più di due volte",
     HEAD "statement (= (* (* (t a i) (t b i)) (* (t c i) (t d i))) (* (* (t a i) (t b i)) (* (t c j) (t d j))))\nholds\nend\n"},
    {"an index three times in one symbol", CK_REJECTED, "più di due volte", HEAD "statement (= (eps i i i) 0)\nholds\nend\n"},
    {"different free indices in the two sides", CK_REJECTED, "indici liberi diversi", HEAD "statement (= (t a i) (t a j))\nholds\nend\n"},
    {"different free indices in two addends", CK_REJECTED, "indici liberi diversi", HEAD "statement (= (+ (t a i) (t a j)) 0)\nholds\nend\n"},
    {"a sum of zeros is not the zero tensor", CK_REJECTED, "indici liberi diversi", HEAD "statement (= (* (* (eps i j k) (t a j)) (t a k)) (+ 0 0))\nholds\nend\n"},
    {"a number that is not zero is not the zero tensor", CK_REJECTED, "indici liberi diversi", HEAD "statement (= (* (delta i j) (t a j)) 1)\nholds\nend\n"},
    {"a number that is not zero is not the zero tensor, as an addend", CK_REJECTED, "indici liberi diversi", HEAD "statement (= (+ (t a i) 2) (t a i))\nholds\nend\n"},
    {"a Kronecker delta with three indices", CK_REJECTED, "due indici", HEAD "statement (= (delta i j k) 0)\nholds\nend\n"},
    {"a Levi-Civita symbol with the wrong number of indices", CK_REJECTED, "tanti indici", HEAD "statement (= (eps i j) (eps i j))\nholds\nend\n"},
    {"a tensor with two different ranks", CK_REJECTED, "numeri diversi di indici", HEAD "statement (= (* (t a i) (t a i)) (* (t a) (t a)))\nholds\nend\n"},
    {"a numeric index beyond the dimension", CK_REJECTED, "fuori dai valori", HEAD "statement (= (eps 1 2 4) 0)\nholds\nend\n"},
    {"a numeric index equal to zero", CK_REJECTED, "intero positivo", HEAD "statement (= (delta 0 1) 0)\nholds\nend\n"},

    /* Certificati scritti male. */
    {"no claim", CK_REJECTED, "incompleto", HEAD CONTRACTION "end\n"},
    {"no dimension", CK_REJECTED, "incompleto", "certificate indices\n" CONTRACTION "holds\nend\n"},
    {"no statement", CK_REJECTED, "incompleto", HEAD "holds\nend\n"},
    {"dimension zero", CK_REJECTED, "intero positivo", "certificate indices\ndimension 0\nstatement (= (delta i i) 0)\nholds\nend\n"},
    {"two claims", CK_REJECTED, "due volte", HEAD CONTRACTION "holds\nfails j 1 k 1 m 1 n 1\nend\n"},
    {"two statements", CK_REJECTED, "due volte", HEAD CONTRACTION CONTRACTION "holds\nend\n"},
    {"an unknown entry", CK_REJECTED, "voce sconosciuta", HEAD CONTRACTION "holds\nsignature 1 1 1\nend\n"},
    {"no end", CK_REJECTED, "voce sconosciuta", HEAD CONTRACTION "holds\n"},
    {"a statement that is not closed", CK_REJECTED, "parentesi chiusa", HEAD "statement (= (delta i i) 3\nholds\nend\n"},
    {"a tensor name of two letters", CK_REJECTED, "nome di un tensore", HEAD "statement (= (t ab i) (t ab i))\nholds\nend\n"},
    {"an index glued to a letter", CK_REJECTED, "un indice", HEAD "statement (= (eps 1 2 3a) 1)\nholds\nend\n"},
    {"a fraction with a zero denominator", CK_REJECTED, "frazione", HEAD "statement (= (* (frac 1 0) (delta i i)) 3)\nholds\nend\n"},

    /* Ciò che il verificatore non conosce o non sa finire: mai accettato. */
    {"an operator of another chapter", CK_UNSUPPORTED, "operatore", HEAD "statement (= (sin (delta i i)) 0)\nholds\nend\n"},
    {"a relation that is not an equality", CK_UNSUPPORTED, "uguaglianza", HEAD "statement (< (delta i i) 4)\nholds\nend\n"},
    {"a product of two polynomials with more terms than the checker multiplies", CK_UNSUPPORTED, "polinomi troppo grandi",
     "certificate indices\ndimension 300\nstatement (= (* (* (t a i) (t b i)) (* (t c j) (t d j))) 0)\nholds\nend\n"},

    /* Più componenti diverse di quante indeterminate hanno i polinomi del
     * capitolo 4 (24): i polinomi del capitolo 19 non hanno quel tetto. */
    {"the trace of a product of three matrices is cyclic (27 components)", CK_ACCEPTED, "",
     HEAD "statement (= (* (* (t A i j) (t B j k)) (t C k i)) (* (* (t B j k) (t C k i)) (t A i j)))\nholds\nend\n"},
    {"the product of three matrices is associative in four dimensions (48 components)", CK_ACCEPTED, "",
     "certificate indices\ndimension 4\nstatement (= (* (* (t A i j) (t B j k)) (t C k l)) (* (t A i j) (* (t B j k) (t C k l))))\nholds\nend\n"},
    {"the antisymmetric part of a tensor with three indices (27 components)", CK_ACCEPTED, "",
     HEAD "statement (= (* (eps i j k) (t T i j k)) (- (+ (+ (t T 1 2 3) (t T 2 3 1)) (t T 3 1 2)) (+ (+ (t T 1 3 2) (t T 2 1 3)) (t T 3 2 1))))\nholds\nend\n"},
    {"the trace of a product of three matrices is not symmetric (27 components)", CK_REJECTED, "sono diversi",
     HEAD "statement (= (* (* (t A i j) (t B j k)) (t C k i)) (* (* (t A i j) (t C j k)) (t B k i)))\nholds\nend\n"},
    {"a refutation with 27 components", CK_ACCEPTED, "",
     HEAD "statement (= (* (* (t A i j) (t B j k)) (t C k i)) (* (* (t A i j) (t C j k)) (t B k i)))\nfails\nend\n"},
    {"a wrong written value with 27 components", CK_REJECTED, "valore scritto",
     HEAD "statement (= (* (* (t A i j) (t B j k)) (t C k i)) (* (* (t A i j) (t C j k)) (t B k i)))\nfails\nleft 0\nend\n"},
    {"a missing term among 48 components", CK_REJECTED, "sono diversi",
     "certificate indices\ndimension 4\nstatement (= (* (* (t A i j) (t B j k)) (t C k l)) (- (* (t A i j) (* (t B j k) (t C k l))) (* (* (t A i 1) (t B 1 1)) (t C 1 l))))\nholds\nend\n"},
    {"the same component written twice is a square, not two unknowns", CK_REJECTED, "sono diversi",
     HEAD "statement (= (* (t a 1) (t a 1)) (* (t a 1) (t a 2)))\nholds\nend\n"},
    {"the factors of a product commute", CK_ACCEPTED, "", HEAD "statement (= (* (* (t a 3) (t b 1)) (t a 1)) (* (t a 1) (* (t a 3) (t b 1))))\nholds\nend\n"},
    {"tensors with the same indices and different names are different", CK_REJECTED, "sono diversi", HEAD "statement (= (t a i) (t b i))\nholds\nend\n"},
    {"components of one tensor with different indices are different", CK_REJECTED, "sono diversi", HEAD "statement (= (t T i j) (t T j i))\nholds\nend\n"},
    {"a dimension too large to try every value", CK_UNSUPPORTED, "lavoro di verifica",
     "certificate indices\ndimension 1000000000000\nstatement (= (delta i j) (delta j i))\nholds\nend\n"},
};

static int checks, failures;

/* Annota l'esito di un controllo, stampando solo quelli falliti. */
static void check(int passed, const char *message) {
    checks++;
    if (passed) return;
    failures++;
    printf("  FAILED: %s\n", message);
}

/* Ritorna il nome inglese di una risposta. */
static const char *answer_name(CkAnswer answer) {
    return answer == CK_ACCEPTED ? "accepted" : answer == CK_REJECTED ? "rejected" : "unsupported";
}

/* Ritorna la risposta del verificatore sul certificato text con il budget
 * steps, e mette in *why il motivo. */
static CkAnswer answer_of(const char *text, long long steps, const char **why) {
    *why = "";
    return ck_check(&text, why, steps);
}

int main(void) {
    char message[512];
    const char *why;

    /* La tabella: risposta attesa, e il motivo per chi non è accettato. */
    for (size_t i = 0; i < sizeof cases / sizeof *cases; i++) {
        CkAnswer answer = answer_of(cases[i].text, 0, &why);
        snprintf(message, sizeof message, "\"%s\" should be %s but is %s (%s)", cases[i].name, answer_name(cases[i].expected), answer_name(answer), why);
        check(answer == cases[i].expected, message);
        snprintf(message, sizeof message, "\"%s\" should mention \"%s\" as its reason, not \"%s\"", cases[i].name, cases[i].why, why);
        check(answer != cases[i].expected || strstr(why, cases[i].why) != NULL, message);
    }

    /* Il budget di lavoro dice solo quanto si è disposti a calcolare: lo
     * stesso certificato, giusto, è "non so controllarlo" con un budget
     * piccolo e accettato con uno che basta; uno sbagliato non diventa mai
     * accettato. */
    check(answer_of(HEAD CONTRACTION "holds\nend\n", 50, &why) == CK_UNSUPPORTED && strstr(why, "lavoro di verifica"),
          "a true certificate should be unsupported, not accepted, when the work budget is too small");
    check(answer_of(HEAD CONTRACTION "holds\nend\n", 100000, &why) == CK_ACCEPTED, "the same certificate should be accepted with a larger budget");
    check(answer_of(HEAD ODD "holds\nend\n", 3, &why) != CK_ACCEPTED, "a false certificate should not be accepted with a small budget");
    check(answer_of(HEAD ODD "holds\nend\n", 1000000000, &why) == CK_REJECTED, "a false certificate should be rejected with a large budget");

    /* Una somma lunga non finisce la memoria: sei indici sommati in
     * dimensione 6 sono 46656 addendi, e resta solo il totale. */
    check(answer_of("certificate indices\ndimension 6\nstatement (= (* (eps i j k l m n) (eps i j k l m n)) 720)\nholds\nend\n", 0, &why) == CK_ACCEPTED,
          "a sum of 46656 terms should be computed within the memory of the checker");

    /* Una risposta non lascia tracce nella successiva: il rango di un
     * tensore vale per un certificato solo. */
    answer_of(HEAD "statement (= (t a i) (t a i))\nholds\nend\n", 0, &why);
    check(answer_of(HEAD "statement (= (t a i j) (t a i j))\nholds\nend\n", 0, &why) == CK_ACCEPTED,
          "the rank of a tensor in an earlier certificate should not be remembered by the next one");
    answer_of(HEAD ODD "holds\nend\n", 0, &why);
    check(answer_of(HEAD CONTRACTION "holds\nend\n", 0, &why) == CK_ACCEPTED, "a rejection should not affect the next certificate");

    /* Parentesi una dentro l'altra oltre il limite di lettura: non
     * controllabile, senza esaurire la pila. */
    static char deep[3 * (CK_MAX_DEPTH + 8) * 6 + 256];
    size_t length = (size_t)snprintf(deep, sizeof deep, HEAD "statement (= ");
    for (int i = 0; i < CK_MAX_DEPTH + 4; i++) length += (size_t)snprintf(deep + length, sizeof deep - length, "(neg ");
    length += (size_t)snprintf(deep + length, sizeof deep - length, "1");
    for (int i = 0; i < CK_MAX_DEPTH + 4; i++) length += (size_t)snprintf(deep + length, sizeof deep - length, ")");
    snprintf(deep + length, sizeof deep - length, " 1)\nholds\nend\n");
    check(answer_of(deep, 0, &why) == CK_UNSUPPORTED && strstr(why, "troppe parentesi"), "too many nested parentheses should be unsupported");

    /* Il numero delle componenti moltiplicate in un termine ha un limite: al
     * limite il conto si fa, oltre la risposta è "non so controllarlo". Lo
     * scalare c moltiplicato per sé stesso 16 e 17 volte. */
    for (int degree = CK_IX_MAX_DEGREE; degree <= CK_IX_MAX_DEGREE + 1; degree++) {
        static char power[4096];
        length = (size_t)snprintf(power, sizeof power, HEAD "statement (= ");
        for (int side = 0; side < 2; side++) {
            for (int i = 1; i < degree; i++) length += (size_t)snprintf(power + length, sizeof power - length, "(* ");
            length += (size_t)snprintf(power + length, sizeof power - length, "(t c)");
            for (int i = 1; i < degree; i++) length += (size_t)snprintf(power + length, sizeof power - length, " (t c))");
            length += (size_t)snprintf(power + length, sizeof power - length, side ? ")\nholds\nend\n" : " ");
        }
        CkAnswer answer = answer_of(power, 0, &why);
        check(degree == CK_IX_MAX_DEGREE ? answer == CK_ACCEPTED : answer == CK_UNSUPPORTED && strstr(why, "troppe componenti moltiplicate") != NULL,
              "a product of 16 components should be computed, one of 17 should be unsupported");
    }

    /* ck_check() avanza fino a dopo "end": due certificati in un testo solo
     * si leggono uno dopo l'altro. */
    const char *two = HEAD CONTRACTION "holds\nend\n" HEAD ODD "holds\nend\n";
    check(ck_check(&two, &why, 0) == CK_ACCEPTED, "the first of two certificates should be accepted");
    check(ck_check(&two, &why, 0) == CK_REJECTED, "the second of two certificates should be read next, and rejected");

    printf("%s: %d of %d checks on index certificates passed\n", failures ? "FAIL" : "PASS", checks - failures, checks);
    return failures ? 1 : 0;
}
