/* Test del verificatore, da solo, sui modelli di grandezza raggiunta da
 * SAT (28t). I dati sono costruiti qui senza il motore: il ciclo di sette
 * elementi e il gruppo diedrale di ordine sei, con elementi r^i s^j e
 * prodotto (i,j)(k,l)=(i+(-1)^j k mod 3,j+l mod 2). Questa formula dimostra
 * l'associativita; (0,0) e neutro, e ogni elemento ha un inverso. r s e
 * s r sono diversi. Il verificatore ricontrolla ogni proprieta sulle tabelle.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_checker_finite_models \
 *         tests/checker/test_finite_model_certificates.c && /tmp/test_checker_finite_models
 */

#include <stdio.h>
#include <string.h>

#define main checker_main
#include "../../checker.c"
#undef main

/* I dati comuni del gruppo e della permutazione; i parametri delle macro
 * sono proprio le parti che le alterazioni sostituiscono. */
#define GROUP_HEAD "certificate counterexample\nsort S 6\nsymbol op S S S\nsymbol e S\n" \
    "statement (implies (and (and " \
    "(forall x S (forall y S (forall z S (= (op (op x y) z) (op x (op y z)))))) " \
    "(forall x S (and (= (op e x) x) (= (op x e) x)))) " \
    "(forall x S (exists y S (and (= (op x y) e) (= (op y x) e))))) " \
    "(forall a S (forall b S (= (op a b) (op b a)))))\n"
#define GROUP_TAIL " 1 2 0 5 3 4 2 0 1 4 5 3 3 4 5 0 1 2 4 5 3 2 0 1 5 3 4 1 2 0\ntable e 0\nend\n"
#define CYCLE_SEVEN "(f (f (f (f (f (f (f x)))))))"
#define CYCLE_SIX "(f (f (f (f (f (f x))))))"
#define CYCLE_HEAD(power) "certificate example\nsort S 7\nsymbol f S S\n" \
    "statement (forall x S (and (= " power " x) (not (= (f x) x))))\n"

/* Un certificato dato e la risposta richiesta, compresi i dati alterati. */
typedef struct {
    const char *name;   /* Il controllo, in inglese. */
    const char *text;   /* Il certificato completo, posseduto dai dati statici. */
    CkAnswer answer;    /* La risposta del verificatore. */
} ModelCase;

int main(void) {
    static const ModelCase cases[] = {
        {"a noncommutative group of six elements is accepted", GROUP_HEAD "table op 0 1 2 3 4 5" GROUP_TAIL, CK_ACCEPTED},
        {"an altered multiplication cell invalidates the counterexample", GROUP_HEAD "table op 1 1 2 3 4 5" GROUP_TAIL, CK_REJECTED},
        {"a cycle of seven elements is accepted", CYCLE_HEAD(CYCLE_SEVEN) "table f 1 2 3 4 5 6 0\nend\n", CK_ACCEPTED},
        {"a fixed point invalidates the claimed example", CYCLE_HEAD(CYCLE_SEVEN) "table f 0 2 3 4 5 6 0\nend\n", CK_REJECTED},
        {"changing the statement invalidates the same table", CYCLE_HEAD(CYCLE_SIX) "table f 1 2 3 4 5 6 0\nend\n", CK_REJECTED},
        {"a table value outside the set is rejected", CYCLE_HEAD(CYCLE_SEVEN) "table f 1 2 3 4 5 7 0\nend\n", CK_REJECTED},
    };
    int failed = 0;
    for (size_t i = 0; i < sizeof cases / sizeof *cases; i++) {
        const char *text = cases[i].text, *why;
        CkAnswer answer = ck_check(&text, &why);
        int ok = answer == cases[i].answer;
        printf("%s: %s%s%s\n", ok ? "PASS" : "FAIL", cases[i].name, ok ? "" : ": ", ok ? "" : why);
        failed += !ok;
    }
    return failed ? 1 : 0;
}
