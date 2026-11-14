/* Le pile dei quantificatori crescono con il problema. Il caso supera il
 * precedente limite di dodici legami, sia leggendo sia ragionando, e cambia
 * l'ultima variabile nella versione falsa. L'oracolo e x=x, oppure x=x+1,
 * sugli interi: la seconda non vale per nessun valore di x.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 * cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_quantifier_storage \
 *     tests/engine/test_quantifier_storage.c && /tmp/test_quantifier_storage
 */
#include "../statement_checks.h"

int main(void) {
    engine_init();
    char formula[4096];
    size_t used = 0;
    for (const char *name = "abcdefghijklm"; *name; name++)
        used += (size_t)snprintf(formula + used, sizeof formula - used,
                                "\\forall %c \\in \\mathbb{Z}\\ ", *name);
    snprintf(formula + used, sizeof formula - used, "(m = m)");
    Verdict v = prove(formula, 'n', 0, 0);
    int passed = check_result(v.outcome == PROVED && v.proof,
                              "thirteen nested integer quantifiers are parsed and proved") &&
                 check_result(scope_capacity >= 13 && universal_capacity >= 13,
                              "parser and proof stacks both grow beyond twelve bindings");
    snprintf(formula + used, sizeof formula - used, "(m = m + 1)");
    v = prove(formula, 'n', 0, 0);
    passed = check_result(v.outcome == REFUTED && v.detail && strstr(v.detail, "m = 0"),
                          "the false variant is refuted using all thirteen recorded bindings") && passed;

    /* Oltre trentadue variabili: nessuna maschera a una parola puo
     * scegliere quali legami conservare. Un budget piccolo deve fermare
     * il campione esponenziale, non diventare una conferma parziale. */
    used = 0;
    for (const char *name = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"; *name; name++)
        used += (size_t)snprintf(formula + used, sizeof formula - used,
                                "\\forall %c \\in \\mathbb{Z}\\ ", *name);
    unsigned long long saved = verification_work_limit;
    verification_work_limit = 1000;
    snprintf(formula + used, sizeof formula - used, "(Z = Z)");
    v = prove(formula, 'n', 0, 0);
    passed = check_result(v.outcome == NOT_PROVED && v.reason && strstr(v.reason, "budget"),
                          "verification exhaustion is explicit and never confirms a partial sample") && passed;
    snprintf(formula + used, sizeof formula - used, "(Z = Z + 1)");
    v = prove(formula, 'n', 0, 0);
    passed = check_result(v.outcome == REFUTED && v.detail && strlen(v.detail) > 256 && strstr(v.detail, "Z = 0"),
                          "all fifty-two bindings are evaluated and the complete counterexample exceeds the old text capacity") && passed;
    verification_work_limit = saved;

    /* Un solo gruppo separato da virgole usa la stessa pila: nessun
     * deposito temporaneo del lettore puo reintrodurre il vecchio tetto. */
    used = (size_t)snprintf(formula, sizeof formula, "\\forall ");
    for (const char *name = "abcdefghijklm"; *name; name++)
        used += (size_t)snprintf(formula + used, sizeof formula - used, "%s%c", name[0] == 'a' ? "" : ",", *name);
    snprintf(formula + used, sizeof formula - used, " \\in \\mathbb{Z}\\ (m = m)");
    v = prove(formula, 'n', 0, 0);
    passed = check_result(v.outcome == PROVED, "a comma-separated group of thirteen variables is proved") && passed;
    used = (size_t)snprintf(formula, sizeof formula, "\\forall ");
    for (const char *name = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"; *name; name++)
        used += (size_t)snprintf(formula + used, sizeof formula - used, "%s%c", name[0] == 'a' ? "" : ",", *name);
    snprintf(formula + used, sizeof formula - used, " \\in \\mathbb{Z}\\ (Z = Z + 1)");
    v = prove(formula, 'n', 0, 0);
    passed = check_result(v.outcome == REFUTED && v.detail && strstr(v.detail, "Z = 0"),
                          "a comma-separated group of fifty-two variables retains the full counterexample") && passed;

    /* Gli stessi gruppi nella grammatica degli insiemi: quantificatori
     * ordinari, mappe e parti dichiarate nell'intestazione degli esempi. */
    static const char *const abstract_groups[] = {
        "\\forall a,b,c,d,e,f,g,h,i,j,k,l,m \\in S\\ (m \\neq m)",
        "\\exists S \\neq \\emptyset\\ \\exists f,g,h,j,k,l,m,n,p,q,r,s,t \\in S^{S}\\ "
        "(\\forall x \\in S\\ (f(x)=x))",
        "\\exists S \\neq \\emptyset\\ \\exists A,B,C,D,E,F,G,H,I,J,K,L,M \\subseteq S\\ "
        "(\\forall x \\in S\\ (x \\notin M))",
    };
    for (size_t i = 0; i < sizeof abstract_groups / sizeof *abstract_groups; i++) {
        v = prove(abstract_groups[i], 'n', 0, 0);
        passed = check_result(v.outcome == (i ? PROVED : REFUTED) && v.certificate,
                              "a thirteen-name abstract group has the expected independently certified result") && passed;
    }
    passed = check_result(unjustified_steps == 0, "the independent checks confirm the reported outcomes") && passed;
    return passed ? 0 : 1;
}
