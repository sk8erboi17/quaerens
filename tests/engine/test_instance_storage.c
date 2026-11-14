/* I gruppi di variabili e parti della ricerca per istanze non si fermano
 * a dodici. Le prove end-to-end disabilitano la saturazione tramite il suo
 * budget, cosi il metodo nuovo non puo nascondere un difetto del precedente.
 * Gli oracoli sono l'impossibilita di x!=x e l'esistenza della parte vuota;
 * le varianti false hanno un modello con un solo elemento.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 * cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_instance_storage \
 *     tests/engine/test_instance_storage.c && /tmp/test_instance_storage
 */
#include "../statement_checks.h"

/* Produce una prova con un'unica istanza su count variabili: un insieme
 * abitato non puo soddisfare per tutti gli elementi x!=x. */
static int element_group(int count) {
    char formula[16384];
    size_t used = (size_t)snprintf(formula, sizeof formula, "(c \\in S \\land ");
    const char *letters = "abcdefghijklmnopqrst";
    for (int i = 0; i < count; i++)
        used += (size_t)snprintf(formula + used, sizeof formula - used, "\\forall %c_{%d} \\in S\\ ", letters[i / 8], i % 8 + 1);
    char last = letters[(count - 1) / 8]; int subscript = (count - 1) % 8 + 1;
    snprintf(formula + used, sizeof formula - used, "(%c_{%d} \\neq %c_{%d})) \\implies c \\neq c", last, subscript, last, subscript);
    Verdict v = prove(formula, 'n', 0, 0);
    if (v.outcome != PROVED) { verdict_print(stdout, &v); putchar('\n'); }
    int width = 0;
    for (int i = 0; now && i < now->knowns; i++) if (now->known[i].origin == K_INSTANCE && now->known[i].parent >= 0) {
        const Sentence *q = now->known[now->known[i].parent].source;
        int variables = 0;
        while (q->kind == S_FORALL) { variables++; q = q->a; }
        if (variables > width) width = variables;
    }
    int passed = check_result(v.outcome == PROVED && v.certificate && width == count &&
                              strcmp(v.method, "con istanze delle ipotesi e uguali al posto di uguali") == 0,
                              "160 element variables are instantiated together with an independently accepted certificate");
    snprintf(formula + used, sizeof formula - used, "(%c_{%d} = %c_{%d})) \\implies c \\neq c", last, subscript, last, subscript);
    v = prove(formula, 'n', 0, 0);
    return check_result(v.outcome == REFUTED && v.certificate,
                         "the true universal hypothesis cannot prove a false disequality") && passed;
}

/* Produce una prova che sceglie count parti insieme. La prima puo essere
 * vuota: richiedere che contenga l'elemento c e un'ipotesi impossibile. */
static int part_group(int count) {
    char formula[16384];
    size_t used = (size_t)snprintf(formula, sizeof formula, "(c \\in S \\land P \\subseteq S \\land ");
    for (int i = 0; i < count; i++)
        used += (size_t)snprintf(formula + used, sizeof formula - used, "\\forall %c_{%d} \\subseteq S\\ ", "XYZAB"[i / 8], i % 8 + 1);
    snprintf(formula + used, sizeof formula - used, "(c \\in X_{1})) \\implies c \\neq c");
    Verdict v = prove(formula, 'n', 0, 0);
    if (v.outcome != PROVED) { verdict_print(stdout, &v); putchar('\n'); }
    int width = 0;
    for (int i = 0; now && i < now->knowns; i++) if (now->known[i].parts > width) width = now->known[i].parts;
    int passed = check_result(v.outcome == PROVED && v.certificate && width == count,
                              "40 quantified parts are chosen and certified in a single instance");
    used = (size_t)snprintf(formula, sizeof formula, "(c \\in S \\land P \\subseteq S \\land ");
    for (int i = 0; i < count; i++)
        used += (size_t)snprintf(formula + used, sizeof formula - used, "\\exists %c_{%d} \\subseteq S\\ ", "XYZAB"[i / 8], i % 8 + 1);
    snprintf(formula + used, sizeof formula - used, "(c \\in B_{8})) \\implies c \\neq c");
    v = prove(formula, 'n', 0, 0);
    return check_result(v.outcome == REFUTED && v.certificate,
                         "a witnessed family of 40 parts cannot prove a false disequality") && passed;
}

/* Controlla copie e rinomina direttamente, comprese alterazioni che non
 * devono piu essere nascoste dalla lunghezza fissa di dodici celle. */
static int part_snapshots(void) {
    char formula[8192];
    size_t used = (size_t)snprintf(formula, sizeof formula, "(P \\subseteq S \\land Q \\subseteq T \\land ");
    for (int i = 0; i < 40; i++)
        used += (size_t)snprintf(formula + used, sizeof formula - used, "\\forall %c_{%d} \\subseteq S\\ ", "XYZAB"[i / 8], i % 8 + 1);
    snprintf(formula + used, sizeof formula - used, "(\\forall x \\in S\\ (x \\in B_{8}))) \\implies \\forall x \\in S\\ (x=x)");
    arena_used = arena_keep; failure = syntax_error = NULL; work_allocation_failed = exhausted = 0;
    cursor = formula; next_token();
    const Sentence *root = abstract_statement();
    if (!root || syntax_error || failure) return check_result(0, "the quantified part test parses");
    const Sentence *prefix = root->kind == S_IMPLIES && root->a && root->a->kind == S_AND ? root->a->b : NULL;
    const Sentence *body = prefix;
    int width = 0;
    for (const Sentence *node = prefix; node && node->kind == S_FORALL_PART; node = node->a) width++;
    if (width != 40) return check_result(0, "the quantified part test retains 40 binders in its hypothesis");
    declared_symbols = symbol_count;
    now = &branch[0]; now->knowns = now->clock = 0;
    int from[40], chosen[40];
    int p = symbol_find('P', 0), q = symbol_find('Q', 0);
    for (int i = 0; i < 40; i++, body = body->a) { from[i] = body->part; chosen[i] = p; }
    const Sentence *instance = body;
    for (int i = 0; i < 40; i++) instance = sentence_rename(instance, from[i], chosen[i]);
    int parent = known_add(prefix, 0, SLOT_VALUES_EMPTY, K_GIVEN, -1);
    int at = known_add(instance, 0, SLOT_VALUES_EMPTY, K_INSTANCE, parent);
    int saved = at >= 0 && known_parts(at, chosen, 40);
    int passed = check_result(saved && part_instance_derived(&now->known[at], &now->known[parent]),
                              "the structural checker validates all 40 part substitutions");
    if (!saved) return 0;
    chosen[39] = q;
    passed = check_result(now->known[at].part[39] == p,
                          "a saved part substitution is immutable after its input changes") && passed;
    now->known[at].parts--;
    passed = check_result(!part_instance_derived(&now->known[at], &now->known[parent]),
                          "omitting the last substituted part is rejected") && passed;
    now->known[at].parts++;
    saved = known_parts(at, chosen, 40);
    passed = check_result(saved && !part_instance_derived(&now->known[at], &now->known[parent]),
                          "a wrong-sort substitution after the twelfth part is rejected") && passed;

    /* Un quantificatore interno nasconde X_1 ma non B_8. La rinomina
     * simultanea deve rispettare lo stesso confine della copia costruttiva. */
    chosen[39] = p;
    Elem *x = slot_elem(body->slot);
    Sentence *a = sentence_new(S_HOLDS, NULL, NULL, NULL), *b = sentence_new(S_HOLDS, NULL, NULL, NULL);
    a->left = elem_new(from[0], 0, x, NULL); b->left = elem_new(from[39], 0, x, NULL);
    Sentence *nested = sentence_new(S_FORALL_PART, sentence_new(S_AND, a, b, NULL), NULL, NULL);
    nested->part = from[0];
    instance = nested;
    for (int i = 0; i < 40; i++) instance = sentence_rename(instance, from[i], chosen[i]);
    passed = check_result(sentence_renamed(instance, nested, from, chosen, 40),
                          "a nested binder shadows one name in a 40-part renaming without truncation") && passed;
    Sentence *captured = sentence_new(S_HOLDS, NULL, NULL, NULL);
    captured->left = elem_new(p, 0, x, NULL);
    Sentence *wrong = sentence_new(S_FORALL_PART, sentence_new(S_AND, captured, captured, NULL), NULL, NULL);
    wrong->part = from[0];
    passed = check_result(!sentence_renamed(wrong, nested, from, chosen, 40),
                          "capturing a shadowed part name is rejected") && passed;

    const int *previous = now->known[at].part;
    size_t memory = arena_used; arena_used = ARENA_BYTES;
    int stored = known_parts(at, chosen, 40);
    int failed = !stored && exhausted && work_allocation_failed && now->known[at].part == previous;
    arena_used = memory; failure = NULL; work_allocation_failed = exhausted = 0;
    return check_result(failed, "allocation failure cannot replace a saved part substitution with partial data") && passed;
}

/* Con molti quantificatori il numero di tuple puo superare anche il tipo
 * intero del contatore. Il budget si decide senza calcolare quel prodotto;
 * un dominio finale vuoto continua a non produrre istanze. */
static int tuple_budget(void) {
    int slots[160];
    for (int i = 0; i < 160; i++) slots[i] = slot_new(10000 + i, 0, 0);
    if (work_allocation_failed || syntax_error) return check_result(0, "the tuple budget test allocates its variables");
    Rule rule = {.known=0, .slot=slots, .slots=160, .body=now->known[0].source};
    candidate_size[0] = candidate_size[1] = 2;
    int before = now->knowns;
    int made = instance_model(&rule);
    slot_data[slots[159]].sort = 1; candidate_size[1] = 0;
    made += instance_model(&rule);
    return check_result(!made && now->knowns == before && !failure && !work_allocation_failed,
                         "a 2^160 tuple space and an empty final domain cannot overflow or add an unchecked instance");
}

/* Esegue le regressioni con il budget della saturazione esaurito in
 * partenza. Ritorna zero soltanto se prove, falsi e depositi sono corretti. */
int main(void) {
    engine_init();
    saturation_work_limit = 1;
    model_work_limit = 20000;
    int passed = element_group(160);
    passed = part_group(40) && passed;
    passed = part_snapshots() && passed;
    passed = tuple_budget() && passed;
    passed = check_result(unjustified_steps == 0, "every produced proof passed its independent check") && passed;
    return passed ? 0 : 1;
}
