/* Test della voce "recurrence" del certificato di induzione: una
 * successione definita per ricorrenza e la sua formula chiusa (26a di
 * main.c). Compilato senza il motore.
 *
 * "a_0 = 2, a_1 = 5, a_{n+2} = 5 a_{n+1} - 6 a_n, allora a_n = 2^n + 3^n":
 * il certificato porta la ricorrenza (i valori iniziali e la regola) e la
 * formula C, e il verificatore controlla da sé i valori iniziali e
 * l'identità C(n + 2) = 5 C(n + 1) - 6 C(n), che scrive sostituendo C nella
 * regola. Qui:
 *
 *   - gli undici certificati che il motore scrive per
 *     tests/recurrences/sequences_by_recurrence.tex sono accettati (fra
 *     questi Fibonacci, con la voce sequence, e i numeri armonici, con
 *     define);
 *   - sono respinti con un valore iniziale sbagliato, con la regola o la
 *     formula cambiate, con un termine fuori dalla finestra della regola,
 *     con il caso base che non è m0, con un valore iniziale in meno, con la
 *     formula che nomina la successione, con il nome della successione
 *     uguale alla variabile o a quello di una voce sequence;
 *   - Fibonacci con i valori iniziali 1 e 1, dove la formula F_n è falsa,
 *     è respinto.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_recurrence_certificates \
 *         tests/checker/test_recurrence_certificates.c && /tmp/test_recurrence_certificates
 */

#include <stdio.h>
#include <string.h>
#define main checker_main
#include "../../checker.c"
#undef main

static int failures;

static void expect(int condition, const char *what) {
    printf("%s: %s\n", condition ? "PASS" : "FAIL", what);
    if (!condition) failures++;
}

static CkAnswer answer_of(const char *text, const char **why) {
    const char *cursor = text, *reason = "";
    CkAnswer answer = ck_check(&cursor, &reason, 0);
    if (why) *why = reason;
    return answer;
}

/* text con la prima occorrenza di old al posto di new, in un deposito suo. */
static const char *replaced(const char *text, const char *old, const char *new) {
    static char out[1 << 14];
    const char *at = strstr(text, old);
    if (!at) return "";
    snprintf(out, sizeof out, "%.*s%s%s", (int)(at - text), text, new, at + strlen(old));
    return out;
}

/* Scritti dal motore (main.c, recurrence_certificate()). */
static const char *const WRITTEN[] = {
    "certificate induction\n"
    "variable n\n"
    "base 0\n"
    "recurrence a 0 1 1\n"
    "initial 1\n"
    "rule (+ (* 2 (seq a n)) 1)\n"
    "statement (= (seq a n) (- (^ 2 (+ n 1)) 1))\n"
    "factor 0\n"
    "defined\n"
    "end\n",
    "certificate induction\n"
    "variable n\n"
    "base 0\n"
    "recurrence a 0 1 1\n"
    "initial 5\n"
    "rule (- (* 3 (seq a n)) 4)\n"
    "statement (= (seq a n) (+ (^ 3 (+ n 1)) 2))\n"
    "factor 0\n"
    "defined\n"
    "end\n",
    "certificate induction\n"
    "variable n\n"
    "base 1\n"
    "recurrence a 1 1 1\n"
    "initial 1\n"
    "rule (+ (+ (seq a n) (* 2 n)) 1)\n"
    "statement (= (seq a n) (^ n 2))\n"
    "factor 0\n"
    "defined\n"
    "end\n",
    "certificate induction\n"
    "variable n\n"
    "base 0\n"
    "recurrence a 0 2 2\n"
    "initial 2\n"
    "initial 5\n"
    "rule (- (* 5 (seq a (+ n 1))) (* 6 (seq a n)))\n"
    "statement (= (seq a n) (+ (^ 2 n) (^ 3 n)))\n"
    "factor 0\n"
    "defined\n"
    "end\n",
    "certificate induction\n"
    "variable n\n"
    "base 0\n"
    "recurrence a 0 2 2\n"
    "initial 1\n"
    "initial 4\n"
    "rule (- (* 4 (seq a (+ n 1))) (* 4 (seq a n)))\n"
    "statement (= (seq a n) (* (+ n 1) (^ 2 n)))\n"
    "factor 0\n"
    "defined\n"
    "end\n",
    "certificate induction\n"
    "variable n\n"
    "base 0\n"
    "recurrence a 0 2 2\n"
    "initial 0\n"
    "initial 1\n"
    "rule (+ (seq a (+ n 1)) (seq a n))\n"
    "statement (= (seq a n) (seq F n))\n"
    "factor 0\n"
    "defined\n"
    "sequence F 2 1 1 0 1\n"
    "end\n",
    "certificate induction\n"
    "variable n\n"
    "base 1\n"
    "recurrence a 1 1 1\n"
    "initial 1\n"
    "rule (/ (seq a n) (+ 1 (seq a n)))\n"
    "statement (= (seq a n) (/ 1 n))\n"
    "factor 0\n"
    "defined\n"
    "end\n",
    "certificate induction\n"
    "variable n\n"
    "base 1\n"
    "recurrence a 1 1 1\n"
    "initial 2\n"
    "rule (/ (* (+ n 1) (seq a n)) n)\n"
    "statement (= (seq a n) (* 2 n))\n"
    "factor 0\n"
    "defined\n"
    "end\n",
    "certificate induction\n"
    "variable n\n"
    "base 0\n"
    "recurrence a 0 1 1\n"
    "initial 1\n"
    "rule (* (+ n 1) (seq a n))\n"
    "statement (= (seq a n) (fact n))\n"
    "factor 0\n"
    "defined\n"
    "end\n",
    "certificate induction\n"
    "variable n\n"
    "base 1\n"
    "recurrence a 1 1 1\n"
    "initial 1\n"
    "rule (+ (seq a n) (/ 1 (+ n 1)))\n"
    "statement (= (seq a n) (seq H n))\n"
    "factor 0\n"
    "defined\n"
    "define H m (sum j 1 m (/ 1 j))\n"
    "end\n",
    "certificate induction\n"
    "variable n\n"
    "base 0\n"
    "recurrence a 0 1 1\n"
    "initial 3\n"
    "rule (- (* 2 (seq a n)) 1)\n"
    "statement (= (seq a n) (+ (^ 2 (+ n 1)) 1))\n"
    "factor 0\n"
    "defined\n"
    "end\n",
};

/* Il certificato con old al posto di new è respinto, e il motivo contiene words (se non NULL). */
static void refused(const char *base, const char *old, const char *new, const char *words, const char *what) {
    const char *why = "", *text = replaced(base, old, new);
    int ok = *text && answer_of(text, &why) != CK_ACCEPTED && (!words || strstr(why, words));
    if (!ok) printf("    (%s)\n", *text ? why : "alterazione non applicata");
    expect(ok, what);
}

int main(void) {
    int count = (int)(sizeof WRITTEN / sizeof *WRITTEN), accepted = 0;
    for (int i = 0; i < count; i++) {
        const char *why = "";
        if (answer_of(WRITTEN[i], &why) == CK_ACCEPTED) accepted++;
        else printf("    respinto: %s\n%s", why, WRITTEN[i]);
    }
    expect(count == 11 && accepted == count, "gli undici certificati scritti dal motore sono accettati");

    const char *two = NULL, *fibonacci = NULL;
    for (int i = 0; i < count; i++) {
        if (strstr(WRITTEN[i], "(+ (^ 2 n) (^ 3 n))")) two = WRITTEN[i];
        if (strstr(WRITTEN[i], "(seq F n)")) fibonacci = WRITTEN[i];
    }
    expect(two && fibonacci, "fra quelli scritti ci sono 2^n + 3^n e Fibonacci");
    if (!two || !fibonacci) return 1;
    refused(two, "initial 5", "initial 6", "valore iniziale", "un valore iniziale sbagliato è respinto");
    refused(two, "(* 6 (seq a n))", "(* 5 (seq a n))", NULL, "la regola cambiata è respinta");
    refused(two, "(+ (^ 2 n) (^ 3 n))", "(+ (^ 2 n) (^ 3 (+ n 1)))", NULL, "la formula cambiata è respinta");
    refused(two, "(seq a (+ n 1))", "(seq a (+ n 2))", "che precedono", "un termine fuori dalla finestra della regola è respinto");
    refused(two, "base 0", "base 1", "m0", "un caso base che non è m0 è respinto");
    refused(two, "initial 5\n", "", "d voci initial", "un valore iniziale in meno è respinto");
    refused(two, "(+ (^ 2 n) (^ 3 n))", "(+ (^ 2 n) (seq a 0))", "nomina la successione", "la formula che nomina la successione è respinta");
    refused(two, "recurrence a 0 2 2", "recurrence n 0 2 2", NULL, "la successione con il nome della variabile è respinta");
    refused(fibonacci, "recurrence a 0 2 2", "recurrence F 0 2 2", NULL, "la successione con il nome di una voce sequence è respinta");
    refused(fibonacci, "initial 0", "initial 1", "valore iniziale", "Fibonacci con i valori iniziali 1 e 1 non è F_n: respinto");

    if (failures) { printf("%d controlli falliti\n", failures); return 1; }
    printf("test_recurrence_certificates: tutti i controlli riusciti\n");
    return 0;
}
