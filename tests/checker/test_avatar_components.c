/* Componenti di AVATAR controllate dal verificatore compilato da solo.
 * L'oracolo prova tutti i predicati unari e il valore di una costante su
 * {0,1}. Interpreta ogni nuova proposizione come la chiusura universale
 * della sua chiave e controlla che ogni modello del genitore si estenda
 * alle definizioni e alla clausola ottenuta dallo split. Nessun SAT.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 * cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_avatar_components \
 *     tests/checker/test_avatar_components.c && /tmp/test_avatar_components
 */
#include <stdio.h>
#include <string.h>
#define main checker_main
#include "../../checker.c"
#undef main

/* Ricomincia con P,Q predicati unari e c costante: simboli 0,1,2. */
static void prepare(void) {
    ck_r_clear(); ck_used = 0; ck_status = CK_ACCEPTED; ck_reason = "";
    ck_r_symbol(1, 1); ck_r_symbol(1, 1); ck_r_symbol(0, 0);
}

/* Legge una clausola come dato del test, senza inventare una deduzione. */
static void parent(const char *text) {
    ck_cursor = text; ck_r_add(ck_r_read_clause());
}

/* Esegue la definizione richiesta e ritorna la risposta del kernel. */
static CkAnswer component(const char *text) {
    ck_cursor = text; ck_r_define_component(); return ck_status;
}

/* Valuta direttamente un termine: vars e l'assegnazione binaria, model
 * contiene le due tavole dei predicati e la costante c, labels le nuove
 * proposizioni. La funzione non applica sostituzioni ne regole del kernel. */
static int value(int t, unsigned vars, const int *model, const int *labels) {
    if (t < 0) return (int)(vars >> (-t - 1) & 1u);
    CkRTerm n = ck_r.term[t];
    if (n.symbol < 2) return model[n.symbol] >> value(ck_r.argument[n.first], vars, model, labels) & 1;
    return n.symbol == 2 ? model[2] : labels[n.symbol - ck_r.component_first];
}

/* Una clausola e vera se lo e per tutte le assegnazioni alle sue variabili.
 * Nei dati semantici di questo test ci sono al piu due variabili. */
static int holds(CkRClause c, const int *model, const int *labels) {
    for (unsigned vars = 0; vars < (1u << c.variables); vars++) {
        int some = 0;
        for (int i = 0; i < c.count; i++) {
            CkRLiteral l = c.literal[i];
            int truth = !l.left ? 1 : l.right ? value(l.left, vars, model, labels) == value(l.right, vars, model, labels)
                                                      : value(l.left, vars, model, labels);
            some |= truth != l.negative;
        }
        if (!some) return 0;
    }
    return 1;
}

/* Le 32 interpretazioni indipendenti dei dati devono ammettere le
 * proposizioni appena definite e ogni conseguenza del genitore indicato.
 * first e l'indice della prima clausola nuova da ricontrollare. */
static int semantic_check(int source, int first) {
    for (int code = 0; code < 32; code++) {
        int model[3] = {code & 3, code >> 2 & 3, code >> 4 & 1}, labels[3] = {0};
        for (int i = 0; i < ck_r.components; i++) labels[i] = holds(ck_r.component[i].key, model, labels);
        if (!holds(ck_r.clause[source], model, labels)) continue;
        for (int i = first; i < ck_r.clauses; i++)
            if (i != source && !holds(ck_r.clause[i], model, labels)) return 0;
    }
    return 1;
}

/* Registra un controllo del test con descrizione e diagnostica in inglese. */
static int check(int ok, const char *name) {
    printf("%s: %s%s%s\n", ok ? "PASS" : "FAIL", name, ok ? "" : ": ", ok ? "" : ck_reason);
    return ok;
}

/* Dati con due componenti a variabili disgiunte; shared riproduce invece
 * l'errore matematico P(x) OR Q(x) -> (forall P) OR (forall Q). */
static void unary_parts(int shared) {
    prepare();
    parent(shared ? "(clause (pos (f 0 (v 0))) (pos (f 1 (v 0))))"
                  : "(clause (pos (f 0 (v 0))) (pos (f 1 (v 1))))");
    component("(clause (pos (f 0 (v 0))))");
    component("(clause (pos (f 1 (v 0))))");
}

/* Un genitore condizionato usa una terza proposizione, P(c), come guardia.
 * I suoi gruppi non proposizionali sono ancora P(x) e Q(y). */
static void guarded_parent(void) {
    unary_parts(0);
    component("(clause (pos (f 0 (f 2))))");
    parent("(clause (pos (f 0 (v 0))) (pos (f 1 (v 1))) (neg (f 5)))");
}

/* Prova il passo corretto e le alterazioni della partizione o del risultato.
 * Ogni caso riparte, cosi un rifiuto non nasconde controlli successivi. */
static int partitions(void) {
    /* Un caso fornisce descrizione e passo come dati; shared indica che i
     * letterali usano la stessa variabile, expected la risposta richiesta. */
    static const struct {
        const char *name;  /* Descrizione inglese del controllo. */
        const char *step;  /* Testo del passo, senza memoria posseduta. */
        int shared;       /* Uno se la partizione e volutamente non disgiunta. */
        CkAnswer expected;/* Risposta richiesta al verificatore. */
    } cases[] = {
        {"disjoint universal components preserve every two-element model",
         "split 0 (groups (0 0 0) (1 0 1)) (clause (pos (f 3)) (pos (f 4)))", 0, CK_ACCEPTED},
        {"shared variables cannot be split into independent universal statements",
         "split 0 (groups (0 0 0) (1 0 1)) (clause (pos (f 3)) (pos (f 4)))", 1, CK_REJECTED},
        {"a partition cannot omit the second literal",
         "split 0 (groups (0 0 0)) (clause (pos (f 3)))", 0, CK_REJECTED},
        {"a partition cannot reuse one literal twice",
         "split 0 (groups (0 0 0) (0 0 0) (1 0 1)) (clause (pos (f 3)) (pos (f 4)))", 0, CK_REJECTED},
        {"a component without an index is rejected",
         "split 0 (groups (0 0) (1 0 1)) (clause (pos (f 3)) (pos (f 4)))", 0, CK_REJECTED},
        {"an altered propositional sign is rejected",
         "split 0 (groups (0 0 0) (1 0 1)) (clause (neg (f 3)) (pos (f 4)))", 0, CK_REJECTED},
        {"complementing a universal component is rejected",
         "split 0 (groups (0 1 0) (1 0 1)) (clause (neg (f 3)) (pos (f 4)))", 0, CK_REJECTED},
        {"a group must match the component it names",
         "split 0 (groups (1 0 0) (0 0 1)) (clause (pos (f 4)) (pos (f 3)))", 0, CK_REJECTED},
        {"an absent component is rejected before accessing its definition",
         "split 0 (groups (3 0 0) (1 0 1)) (clause (pos (f 3)) (pos (f 4)))", 0, CK_REJECTED},
    };
    int passed = 1;
    for (size_t i = 0; i < sizeof cases / sizeof *cases; i++) {
        unary_parts(cases[i].shared);
        ck_cursor = cases[i].step; ck_r_step();
        int ok = ck_status == cases[i].expected;
        if (cases[i].expected == CK_ACCEPTED) ok &= semantic_check(0, 1);
        passed = check(ok, cases[i].name) && passed;
    }
    return passed;
}

/* I guardiani proposizionali del genitore restano condizioni dello split.
 * Il caso ground puo usare il segno negativo della stessa astrazione. */
static int guards_and_complements(void) {
    guarded_parent();
    ck_cursor = "split 4 (groups (0 0 0) (1 0 1)) (clause (pos (f 3)) (pos (f 4)) (neg (f 5)))"; ck_r_step();
    /* La clausola zero e solo lo scaffolding del primo dato: l'oracolo
     * assume il genitore condizionato e controlla le definizioni da uno. */
    int ok = ck_status == CK_ACCEPTED && semantic_check(4, 1);
    int passed = check(ok, "a conditional split retains the parent's propositional guard");
    guarded_parent();
    ck_cursor = "split 4 (groups (0 0 0) (1 0 1)) (clause (pos (f 3)) (pos (f 4)))"; ck_r_step();
    passed = check(ck_status == CK_REJECTED, "a conditional split cannot omit its guard from the written result") && passed;

    prepare(); parent("(clause (neg (f 0 (f 2))) (pos (f 1 (v 0))))");
    component("(clause (pos (f 0 (f 2))))"); component("(clause (pos (f 1 (v 0))))");
    ck_cursor = "0"; ck_r_complement_component();
    ck_cursor = "split 0 (groups (0 1 0) (1 0 1)) (clause (neg (f 3)) (pos (f 4)))"; ck_r_step();
    passed = check(ck_status == CK_ACCEPTED && semantic_check(0, 1), "a negative ground atom shares the complement of its positive abstraction") && passed;

    unary_parts(0); ck_cursor = "0"; ck_r_complement_component();
    passed = check(ck_status == CK_REJECTED, "a universal component cannot produce a universally negated clause") && passed;
    unary_parts(0); component("(clause (pos (f 3)))");
    passed = check(ck_status == CK_REJECTED, "a component key cannot refer to a previously introduced proposition") && passed;
    prepare(); component("(clause (pos (f 0 (v 7))))");
    passed = check(ck_status == CK_REJECTED, "component variables must use the canonical first-occurrence names") && passed;
    prepare(); ck_used = CK_ARENA_BYTES;
    component("(clause (pos (f 0 (v 0))))");
    passed = check(ck_status == CK_UNSUPPORTED, "an incomplete component allocation is not accepted") && passed;
    prepare(); component("(clause (pos (f 0 (v 0))))");
    passed = check(ck_status == CK_ACCEPTED, "a new component check succeeds after an allocation failure was cleared") && passed;
    return passed;
}

/* La prova completa usa una componente per k=k, il suo complemento e lo
 * split della clausola k!=k, poi risolve le due proposizioni opposte.
 * I dati negativi mantengono un enunciato falso senza clausola vuota. */
static int complete_certificates(void) {
    static const char *const proof =
        "certificate saturation\nsort S\nstatement (forall x S (= x x))\nclausify\n"
        "component (clause (eq (f 1) (f 1)))\ncomplement 0\n"
        "step resolve 1 0 3 0 (subst) (clause (ne (f 1) (f 1)))\n"
        "step split 6 (groups (0 1 0)) (clause (neg (f 3)))\n"
        "step reflex 5 0 (subst) (clause (pos (f 3)))\n"
        "step resolve 7 0 8 0 (subst) (clause)\nend\n";
    const char *text = proof, *why;
    int passed = check(ck_check(&text, &why) == CK_ACCEPTED, "a complete proof with component definitions and splitting is independently accepted");
    text = "certificate saturation\nstatement (not true)\nclausify\ncomponent (clause (eq (v 0) (v 0)))\nend\n";
    passed = check(ck_check(&text, &why) == CK_REJECTED, "a conservative component definition does not prove falsehood") && passed;
    text = "certificate saturation\nsort S\nstatement (exists x S (= x x))\nclausify\ncomponent (clause (pos (f 0 (v 0))))\ncomplement 0\nend\n";
    passed = check(ck_check(&text, &why) == CK_REJECTED, "component syntax cannot invent an element of an empty sort") && passed;
    text = "certificate counterexample\nsort S 2\nsymbol P S bool\nstatement (or (forall x S (P x)) (forall x S (not (P x))))\ntable P 1 0\nend\n";
    passed = check(ck_check(&text, &why) == CK_ACCEPTED, "a nonuniform predicate refutes the disjunction of the two universal closures") && passed;
    text = "certificate saturation\nsort S\nsymbol P S bool\nstatement (or (forall x S (P x)) (forall x S (not (P x))))\nclausify\ncomponent (clause (pos (f 0 (v 0))))\ncomplement 0\nend\n";
    passed = check(ck_check(&text, &why) == CK_REJECTED, "the false uniformity theorem cannot use an illegal universal complement") && passed;
    return passed;
}

/* Esegue i controlli finiti e i certificati completi, poi libera i depositi.
 * Ritorna zero soltanto se ogni alterazione matematica e stata respinta. */
int main(void) {
    int passed = partitions();
    passed = guards_and_complements() && passed;
    passed = complete_certificates() && passed;
    ck_r_clear();
    return passed ? 0 : 1;
}
