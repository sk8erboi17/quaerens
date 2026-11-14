/* I singoli passi di 11m sono controllati senza il motore. Oltre alle
 * alterazioni si prova la validita semantica sulle 4096 interpretazioni di
 * sei simboli su {0,1}: due costanti, tre funzioni e un predicato binario.
 * Il controllo semantico interpreta i genitori e la conclusione, senza
 * chiamare il codice che applica sostituzioni o ricostruisce il passo.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 * cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_saturation_steps \
 *     tests/checker/test_saturation_steps.c && /tmp/test_saturation_steps
 */
#include <stdio.h>
#include <string.h>
#define main checker_main
#include "../../checker.c"
#undef main

/* Prepara soltanto i simboli dei dati, con gli indici usati nei passi. */
static void prepare(void) {
    ck_r_clear(); ck_used = 0; ck_status = CK_ACCEPTED; ck_reason = "";
    static const int arity[] = {0, 0, 1, 2, 1, 1};
    for (int i = 0; i < 6; i++) ck_r_symbol(arity[i], i == 3);
}

/* Legge una clausola come dato, poi ne conserva la normalizzazione. */
static void parent(const char *text) {
    ck_cursor = text;
    ck_r_add(ck_r_read_clause());
}

/* Valore diretto di un termine nell'interpretazione. model contiene le
 * tabelle in codifica binaria, vars l'assegnazione alle variabili. */
static int value(int t, unsigned vars, const int *model) {
    if (t < 0) return (int)(vars >> (-t - 1) & 1);
    CkRTerm n = ck_r.term[t];
    int cell = 0;
    for (int i = 0; i < n.count; i++) cell |= value(ck_r.argument[n.first + i], vars, model) << i;
    return model[n.symbol] >> cell & 1;
}

/* Ogni variabile della clausola e universale. Il test usa al piu due nomi
 * in questa parte esaustiva; il caso da cento nomi ha un controllo separato. */
static int holds(CkRClause c, const int *model) {
    for (unsigned vars = 0; vars < (1u << c.variables); vars++) {
        int some = 0;
        for (int i = 0; i < c.count; i++) {
            CkRLiteral l = c.literal[i];
            int truth = !l.left ? 1 : l.right ? value(l.left, vars, model) == value(l.right, vars, model)
                                                     : value(l.left, vars, model);
            some |= truth != l.negative;
        }
        if (!some) return 0;
    }
    return 1;
}

/* Tutte le interpretazioni finite dei dati devono rispettare l'inferenza. */
static int semantic_check(int parents) {
    int all = 1;
    for (unsigned code = 0; code < 4096; code++) {
        int model[6] = {(int)(code & 1), (int)(code >> 1 & 1), (int)(code >> 2 & 3),
                        (int)(code >> 4 & 15), (int)(code >> 8 & 3), (int)(code >> 10 & 3)};
        int premises = 1;
        for (int i = 0; i < parents; i++) premises &= holds(ck_r.clause[i], model);
        if (premises && !holds(ck_r.clause[parents], model)) all = 0;
    }
    return all;
}

/* Un passo corretto deve superare anche l'oracolo e lasciare immutati i
 * genitori. Un passo alterato deve essere respinto, non solo ignorato. */
static int step_case(const char *name, const char *a, const char *b, const char *step, CkAnswer wanted) {
    prepare(); parent(a); if (b) parent(b);
    int parents = ck_r.clauses;
    CkRLiteral saved[2][4];
    for (int i = 0; i < parents; i++) memcpy(saved[i], ck_r.clause[i].literal, (size_t)ck_r.clause[i].count * sizeof saved[i][0]);
    ck_cursor = step; ck_r_step();
    int ok = ck_status == wanted;
    if (wanted == CK_ACCEPTED) {
        ok &= ck_r.clauses == parents + 1 && semantic_check(parents);
        for (int i = 0; i < parents; i++) ok &= memcmp(saved[i], ck_r.clause[i].literal, (size_t)ck_r.clause[i].count * sizeof saved[i][0]) == 0;
    }
    printf("%s: %s%s%s\n", ok ? "PASS" : "FAIL", name, ok ? "" : ": ", ok ? "" : ck_reason);
    return ok;
}

int main(void) {
    static const char *eq = "(clause (eq (f 2 (v 0)) (v 0)))";
    static const char *ne = "(clause (ne (f 2 (f 0)) (f 1)))";
    static const char *two = "(clause (eq (f 2 (v 0)) (f 0)) (eq (f 2 (v 1)) (f 1)))";
    static const char *positive = "(clause (pos (f 3 (v 0) (v 0))) (pos (f 3 (v 1) (v 1))))";
    int passed = step_case("superposition preserves every two-element interpretation", eq, ne,
        "super 0 0 0 1 0 0 0 (subst (0 (f 0))) (clause (ne (f 0) (f 1)))", CK_ACCEPTED);
    passed = step_case("an altered superposition position is rejected", eq, ne,
        "super 0 0 0 1 0 0 1 (subst (0 (f 0))) (clause (ne (f 0) (f 1)))", CK_REJECTED) && passed;
    passed = step_case("an altered substitution does not identify different constants", eq, ne,
        "super 0 0 0 1 0 0 0 (subst (0 (f 1))) (clause (ne (f 0) (f 1)))", CK_REJECTED) && passed;
    passed = step_case("equality factoring preserves every two-element interpretation", two, NULL,
        "equalfactor 0 0 0 1 0 (subst (0 (v 1))) (clause (eq (f 2 (v 0)) (f 1)) (ne (f 0) (f 1)))", CK_ACCEPTED) && passed;
    passed = step_case("equality factoring cannot omit its residual disequality", two, NULL,
        "equalfactor 0 0 0 1 0 (subst (0 (v 1))) (clause (eq (f 2 (v 0)) (f 1)))", CK_REJECTED) && passed;
    passed = step_case("equality factoring also preserves the opposite ordered pair", two, NULL,
        "equalfactor 0 1 0 0 0 (subst (1 (v 0))) (clause (eq (f 2 (v 0)) (f 0)) (ne (f 1) (f 0)))", CK_ACCEPTED) && passed;
    passed = step_case("predicate factoring preserves every two-element interpretation", positive, NULL,
        "factor 0 0 1 (subst (0 (v 1))) (clause (pos (f 3 (v 0) (v 0))))", CK_ACCEPTED) && passed;
    static const char *complex = "(clause (eq (f 2 (f 4 (v 0))) (f 5 (v 0))))";
    static const char *target = "(clause (pos (f 3 (v 0) (f 2 (v 0)))))";
    passed = step_case("superposition positions are counted after substitution", complex, target,
        "super 0 0 0 1 0 0 3 (subst (64 (f 4 (v 0)))) (clause (pos (f 3 (f 4 (v 0)) (f 5 (v 0)))))", CK_ACCEPTED) && passed;
    passed = step_case("using the old position after variable expansion is rejected", complex, target,
        "super 0 0 0 1 0 0 2 (subst (64 (f 4 (v 0)))) (clause (pos (f 3 (f 4 (v 0)) (f 5 (v 0)))))", CK_REJECTED) && passed;

    /* Il secondo genitore comincia al nome 100, non al prefisso storico
     * 64. Il controllo rifiuta l'identificazione tacita dell'ultimo nome. */
    prepare(); int predicate = ck_r_symbol(100, 1), args[100];
    for (int i = 0; i < 100; i++) args[i] = -i - 1;
    CkRLiteral yes = {ck_r_term(predicate, args), 0, 0}, no = ck_r_not(yes);
    ck_r_add((CkRClause){.count=1, .literal=&yes}); ck_r_add((CkRClause){.count=1, .literal=&no});
    char step[8192]; size_t used = (size_t)snprintf(step, sizeof step, "resolve 0 0 1 0 (subst");
    for (int i = 0; i < 100; i++) used += (size_t)snprintf(step + used, sizeof step - used, " (%d (v %d))", i, 100 + i);
    snprintf(step + used, sizeof step - used, ") (clause)");
    ck_cursor = step; ck_r_step();
    int ok = ck_status == CK_ACCEPTED && ck_r.clause[2].count == 0;
    printf("%s: two parents with 100 variables and 100-argument predicates remain disjoint\n", ok ? "PASS" : "FAIL");
    passed &= ok;
    char *last = strstr(step, " (99 (v 199))");
    if (last) snprintf(last, sizeof step - (size_t)(last - step), ") (clause)");
    ck_cursor = step; ck_r_step();
    ok = last && ck_status == CK_REJECTED;
    printf("%s: leaving the final variables unbound cannot close the large resolution step\n", ok ? "PASS" : "FAIL");
    passed &= ok;
    ck_r_clear();
    return passed ? 0 : 1;
}
