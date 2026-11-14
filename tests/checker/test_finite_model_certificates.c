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
        {"a powerset cannot name itself as its base or reuse a previous model's size",
         "certificate example\nsort P parts P e\nstatement true\nend\n", CK_REJECTED},
        {"a map sort cannot use itself as its domain",
         "certificate example\nsort T 1\nsort M maps M T ev\nstatement true\nend\n", CK_REJECTED},
        {"a map sort cannot use itself as its codomain",
         "certificate example\nsort S 0\nsort M maps S M ev\nstatement true\nend\n", CK_REJECTED},
        {"a powerset of an earlier powerset remains a valid finite interpretation",
         "certificate example\nsort S 0\nsort P parts S e\nsort Q parts P r\n"
         "statement (exists X P (forall x S (not (e x X))))\nend\n", CK_ACCEPTED},
    };
    int failed = 0;
    for (size_t i = 0; i < sizeof cases / sizeof *cases; i++) {
        const char *text = cases[i].text, *why;
        CkAnswer answer = ck_check(&text, &why, 0);
        int ok = answer == cases[i].answer;
        printf("%s: %s%s%s\n", ok ? "PASS" : "FAIL", cases[i].name, ok ? "" : ": ", ok ? "" : why);
        failed += !ok;
    }

    /* 257 elementi richiedono indici oltre un byte. La tabella identita e
     * costruita direttamente: l'oracolo e la definizione f(i)=i. */
    char large[8192];
    size_t used = (size_t)snprintf(large, sizeof large,
        "certificate example\nsort S 257\nsymbol f S S\nstatement (forall x S (= (f x) x))\ntable f");
    for (size_t i = 0; i < 257; i++) used += (size_t)snprintf(large + used, sizeof large - used, " %zu", i);
    snprintf(large + used, sizeof large - used, "\nend\n");
    const char *text = large, *why;
    int ok = ck_check(&text, &why, 0) == CK_ACCEPTED;
    printf("%s: a 257-element identity table is checked exactly\n", ok ? "PASS" : "FAIL");
    failed += !ok;
    char *last = strstr(large, " 256\n");
    if (last) memcpy(last + 1, "255", 3);
    text = large;
    ok = last && ck_check(&text, &why, 0) == CK_REJECTED;
    printf("%s: changing the value beyond 255 invalidates the example\n", ok ? "PASS" : "FAIL");
    failed += !ok;

    /* Un testimone e la parte vuota: basta il primo sottoinsieme, senza
     * calcolare 2^257 o spostare un bit oltre la larghezza di un intero. */
    text = "certificate example\nsort S 257\nstatement (existspart X S (forall x S (not (X x))))\nend\n";
    ok = ck_check(&text, &why, 0) == CK_ACCEPTED;
    printf("%s: bound subsets have no machine-word cardinality limit\n", ok ? "PASS" : "FAIL");
    failed += !ok;
    text = "certificate counterexample\nsort S 257\nstatement (forallpart X S (forall x S (X x)))\nend\n";
    ok = ck_check(&text, &why, 0) == CK_ACCEPTED;
    printf("%s: a large empty subset refutes the claimed universal condition\n", ok ? "PASS" : "FAIL");
    failed += !ok;
    text = "certificate counterexample\nsort S 0\nstatement (and (exists x S (= x x)) (forall x S (= x x)))\nend\n";
    ok = ck_check(&text, &why, 0) == CK_ACCEPTED;
    printf("%s: explicit nonemptiness is false in an empty interpretation\n", ok ? "PASS" : "FAIL");
    failed += !ok;
    text = "certificate example\nsort S 0\nstatement (and (exists x S (= x x)) (forall x S (= x x)))\nend\n";
    ok = ck_check(&text, &why, 0) == CK_REJECTED;
    printf("%s: an empty example cannot satisfy explicit nonemptiness\n", ok ? "PASS" : "FAIL");
    failed += !ok;

    /* Il budget del ricalcolo, terzo argomento di ck_check(). Tre "per
     * ogni" annidati su 30 elementi provano 30 + 900 + 27000 = 27930
     * elementi: con quel budget il conto arriva in fondo, con uno in meno
     * no, e la risposta è "non so controllarlo", non un rifiuto. */
    const char *nested = "certificate example\nsort S 30\nstatement (forall a S (forall b S (forall c S (= a a))))\nend\n";
    text = nested;
    ok = ck_check(&text, &why, 27930) == CK_ACCEPTED;
    printf("%s: a model is checked when the step budget is exactly its cost\n", ok ? "PASS" : "FAIL");
    failed += !ok;
    text = nested;
    ok = ck_check(&text, &why, 27929) == CK_UNSUPPORTED;
    printf("%s: one step less leaves the certificate unchecked, not rejected\n", ok ? "PASS" : "FAIL");
    failed += !ok;
    text = nested;
    ok = ck_check(&text, &why, 0) == CK_ACCEPTED;
    printf("%s: a zero budget asks for the usual one, also after a smaller one\n", ok ? "PASS" : "FAIL");
    failed += !ok;

    /* Oltre il budget consueto: quattro "per ogni" su 101 elementi provano
     * più di cento milioni di elementi. Prima nessuno poteva far controllare
     * questo modello; ora chi chiama dà il budget. Un budget largo non fa
     * accettare nulla in più: lo stesso modello con la condizione negata
     * resta respinto. */
    const char *wide = "certificate example\nsort S 101\nstatement (forall a S (forall b S (forall c S (forall d S (= a a)))))\nend\n";
    text = wide;
    ok = ck_check(&text, &why, 0) == CK_UNSUPPORTED;
    printf("%s: a model beyond the usual step budget is not checked by default\n", ok ? "PASS" : "FAIL");
    failed += !ok;
    text = wide;
    ok = ck_check(&text, &why, 200000000LL) == CK_ACCEPTED;
    printf("%s: the same model is accepted with a larger step budget\n", ok ? "PASS" : "FAIL");
    failed += !ok;
    text = "certificate example\nsort S 101\nstatement (forall a S (forall b S (forall c S (forall d S (not (= a a))))))\nend\n";
    ok = ck_check(&text, &why, 200000000LL) == CK_REJECTED;
    printf("%s: a larger step budget does not accept a false example\n", ok ? "PASS" : "FAIL");
    failed += !ok;
    return failed ? 1 : 0;
}
