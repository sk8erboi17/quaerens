/* Test dei modelli SAT (28t) attraverso l'interfaccia del motore. La
 * ricerca deve costruire le tabelle, senza conoscere gruppi o cicli.
 *
 * Terzo conto sui dati. Le permutazioni di tre oggetti formano un gruppo
 * non commutativo di sei elementi; i gruppi di ordine minore sono abeliani.
 * Una permutazione con f^p=id e senza punti fissi, per p primo, puo essere
 * un ciclo di p elementi. Sono esempi di esistenza indipendenti dal SAT;
 * ogni tabella trovata viene poi ricalcolata dal checker da solo.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_finite_models \
 *         tests/algebraic_structures/test_finite_models.c && /tmp/test_finite_models
 */

#include "../statement_checks.h"

int main(void) {
    static const char *const cases[] = {
        "(\\perp \\colon S \\times S \\to S \\land e \\in S \\land "
        "\\forall x,y,z \\in S\\ ((x \\perp y) \\perp z = x \\perp (y \\perp z)) \\land "
        "\\forall x \\in S\\ (e \\perp x = x \\land x \\perp e = x) \\land "
        "\\forall x \\in S\\ \\exists y \\in S\\ (x \\perp y = e \\land y \\perp x = e)) "
        "\\implies \\forall a,b \\in S\\ (a \\perp b = b \\perp a)",
        "\\exists S \\neq \\emptyset\\ \\exists f \\colon S \\to S\\ ("
        "\\forall x \\in S\\ (f(f(f(f(f(x))))) = x \\land f(x) \\neq x))",
        "\\exists S \\neq \\emptyset\\ \\exists f \\colon S \\to S\\ ("
        "\\forall x \\in S\\ (f(f(f(f(f(f(f(x))))))) = x \\land f(x) \\neq x))",
    };
    int passed = 1;
    engine_init();
    for (size_t i = 0; i < sizeof cases / sizeof *cases; i++) {
        /* I cicli primi obbligano cardinalita oltre il vecchio tetto di otto.
     * L'oracolo e un ciclo di lunghezza p, non una risposta del SAT. */
    for (int p = 11; p <= 13; p += 2) {
        char iterate[128] = "x", next[128], formula[1024];
        for (int i = 0; i < p; i++) {
            snprintf(next, sizeof next, "f(%s)", iterate);
            strcpy(iterate, next);
        }
        snprintf(formula, sizeof formula, "\\exists S \\neq \\emptyset\\ \\exists f \\colon S \\to S\\ "
                 "(\\forall x \\in S\\ (%s = x \\land f(x) \\neq x))", iterate);
        Verdict large = prove(formula, 'n', 0, 0);
        char size[32]; snprintf(size, sizeof size, "sort S %d\n", p);
        passed = check_result(large.outcome == PROVED && large.certificate && strstr(large.certificate, size),
                              "a certified prime cycle is found beyond eight elements") && passed;
    }
    Verdict v = prove(cases[i], 'n', 0, 0);
        int decided = v.outcome == (i ? PROVED : REFUTED);
        if (!decided) { verdict_print(stdout, &v); putchar('\n'); }
        passed = check_result(decided, i ? "a fixed-point-free permutation of prime order has a certified finite example"
                                        : "not every group is commutative: a larger finite counterexample is found") && passed;
        passed = check_result(v.certificate != NULL, "the independent checker accepts the complete model") && passed;
    }
    Verdict v = prove("\\forall f \\in S^{S}\\ ((\\forall a,b \\in S\\ (f(a) = f(b) \\implies a = b)) "
                      "\\implies \\forall y \\in S\\ \\exists x \\in S\\ (f(x) = y))", 'n', 0, 0);
    passed = check_result(v.outcome != PROVED, "truth in every finite model never proves a false statement about infinite sets") && passed;
    passed = check_result(unjustified_steps == 0, "all produced models pass their independent checks") && passed;
    return passed ? 0 : 1;
}
