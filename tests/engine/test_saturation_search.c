/* Ricerca per saturazione, con formule come dati e certificati ricontrollati
 * dal verificatore indipendente. I falsi includono insiemi vuoti, scambio
 * illecito dei quantificatori e il tentativo di unificare x con f(x).
 *
 * Compilazione ed esecuzione dalla cartella principale:
 * cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_saturation_search \
 *     tests/engine/test_saturation_search.c && /tmp/test_saturation_search
 */
#include "../statement_checks.h"

/* Un caso dichiara il teorema atteso e una regola che deve essere stata
 * effettivamente usata fra gli antenati del vuoto, non soltanto tentata. */
typedef struct {
    const char *name, *formula;
    int theorem;
    SpRule required;
} SearchCase;

/* Esegue soltanto la nuova ricerca. L'esito positivo richiede il certificato
 * completo, mentre per i dati falsi non deve mai apparire il vuoto. */
static int search_case(const SearchCase *test) {
    arena_used = arena_keep; failure = syntax_error = NULL;
    work_allocation_failed = 0;
    cursor = test->formula; next_token();
    const Sentence *root = abstract_statement();
    declared_symbols = symbol_count;
    SpState state = {0};
    int ready = !syntax_error && !failure && sp_prepare(&state, root, NULL, 0);
    int found = ready && sp_search(&state), ok = test->theorem ? found : !found;
    if (found) {
        unsigned char *needed = calloc((size_t)state.clauses, 1);
        int used = test->required == SP_INPUT;
        if (needed) {
            needed[state.empty] = 1;
            for (int i = state.empty; i >= state.initial; i--) if (needed[i]) {
                SpStep step = state.clause[i].step;
                used |= step.rule == test->required;
                needed[step.p] = 1;
                if (step.rule == SP_RESOLVE || step.rule == SP_SUPER) needed[step.q] = 1;
            }
            free(needed);
        }
        sp_write(&state, root, NULL, 0, 0);
        const char *certificate = certificate_text, *why;
        CkAnswer answer = certificate_broken ? CK_UNSUPPORTED : ck_check(&certificate, &why, 0);
        if (answer != CK_ACCEPTED) printf("Certificate failure: %s\n", certificate_broken ? "incomplete certificate" : why);
        ok &= used && answer == CK_ACCEPTED;
    }
    if (!ok) printf("Search details: ready=%d found=%d clauses=%d terms=%d work=%llu limited=%d unsupported=%d parse=%s\n",
                    ready, found, state.clauses, state.terms, state.work, state.limited, state.unsupported,
                    syntax_error ? syntax_error : "none");
    int passed = check_result(ok, test->name);
    sp_clear(&state);
    return passed;
}

/* Oracoli di struttura: unificazione finita, ordine delle variabili e
 * conservazione del sostituto di una clausola riscritta. L'ultimo caso
 * riproduce una perdita possibile con sussunzione prima della cancellazione. */
static int storage_checks(void) {
    SpState s = {.empty=-1};
    int f = sp_symbol(&s, 1, 0), p = sp_symbol(&s, 1, 1), a = sp_symbol(&s, 0, 0);
    int x = -1, y = -2, ax = sp_term(&s, a, NULL), fx = sp_term(&s, f, &x), fa = sp_term(&s, f, &ax);
    int passed = check_result(sp_unify_start(&s, 2) && !sp_unify(&s, x, fx) && !s.limited,
                              "occurs-check rejects a cyclic finite-term substitution");
    passed = check_result(sp_unify_start(&s, 2) && sp_unify(&s, x, y) && sp_unify(&s, y, ax) &&
                          sp_apply(&s, x, 0, s.sub, s.sub_count, 1) == ax,
                          "a unifier resolves transitive substitution chains") && passed;
    passed = check_result(sp_greater(&s, fx, x) && !sp_greater(&s, fx, y) && !sp_greater(&s, x, fx) &&
                          sp_greater(&s, fa, ax),
                          "KBO respects the variable-occurrence condition and strict subterms") && passed;

    SpLiteral rule = {fx, x, 0};
    int px = sp_term(&s, p, &fx), pa = sp_term(&s, p, &fa);
    SpLiteral old[] = {{px, 0, 0}, {pa, 0, 0}};
    int r = sp_keep(&s, &rule, 1, (SpStep){.rule=SP_INPUT}, -1);
    int q = sp_keep(&s, old, 2, (SpStep){.rule=SP_INPUT}, -1);
    s.initial = s.clauses;
    if (r >= 0) s.clause[r].stage = SP_ACTIVE;
    int changed = r >= 0 && q >= 0 && sp_demodulate(&s, r, q), live = 0;
    for (int i = 0; i < s.clauses; i++) live += i != r && s.clause[i].stage != SP_DELETED;
    passed = check_result(changed && s.clause[q].stage == SP_DELETED && live > 0 && !s.limited,
                          "a rewritten clause cannot discard its replacement by subsuming it and then disappear") && passed;
    sp_clear(&s);

    s.empty = -1;
    f = sp_symbol(&s, 1, 0); a = sp_symbol(&s, 0, 0);
    int b = sp_symbol(&s, 0, 0), bx = sp_term(&s, b, NULL);
    ax = sp_term(&s, a, NULL); fx = sp_term(&s, f, &x);
    int fy = sp_term(&s, f, &y);
    SpLiteral choices[] = {{fx, ax, 0}, {fy, bx, 0}};
    r = sp_keep(&s, choices, 2, (SpStep){.rule=SP_INPUT}, -1);
    s.initial = s.clauses;
    if (r >= 0) sp_unary(&s, r);
    unsigned orders = 0;
    for (int i = s.initial; i < s.clauses; i++)
        if (s.clause[i].step.rule == SP_EQUAL_FACTOR) orders |= 1u << s.clause[i].step.lp;
    passed = check_result(orders == 3 && !s.limited,
                          "equality factoring generates both ordered pairs of distinct positive equalities") && passed;
    sp_clear(&s);

    /* L'intestazione dichiara implicitamente l'appartenenza alle parti
     * prima di f, anche se il testo ha incontrato f per prima. */
    arena_used = arena_keep; failure = syntax_error = NULL;
    cursor = "(f \\colon S \\to T \\land F \\subseteq \\mathcal{P}(T)) \\implies \\forall x \\in S\\ (f(x)=f(x))";
    next_token(); const Sentence *reordered = abstract_statement(); declared_symbols = symbol_count;
    int built = !syntax_error && sp_prepare(&s, reordered, NULL, 0), original = symbol_find('f', 0), implicit = -1;
    for (int i = 0; i < sort_count; i++) if (sort_data[i].kind == SORT_PARTS) implicit = sort_data[i].symbol;
    int mapped = built && original >= 0 && implicit >= 0 && s.input_symbol[implicit] < s.input_symbol[original] &&
                 s.symbol[s.input_symbol[implicit]].predicate && !s.symbol[s.input_symbol[original]].predicate;
    if (mapped && sp_search(&s)) {
        sp_write(&s, reordered, NULL, 0, 0);
        const char *text = certificate_text, *why;
        mapped = ck_check(&text, &why, 0) == CK_ACCEPTED;
    } else mapped = 0;
    passed = check_result(mapped, "implicit sort symbols and explicit functions use the certificate declaration order") && passed;
    sp_clear(&s);

    unsigned long long budget = saturation_work_limit;
    saturation_work_limit = 1;
    arena_used = arena_keep; failure = syntax_error = NULL;
    cursor = "\\forall x \\in S\\ (x=x)"; next_token();
    const Sentence *root = abstract_statement(); declared_symbols = symbol_count;
    int complete = sp_prepare(&s, root, NULL, 0) && sp_search(&s);
    passed = check_result(!complete && s.limited, "work exhaustion cannot produce a positive saturation result") && passed;
    saturation_work_limit = budget; sp_clear(&s);
    int previous = unjustified_steps;
    Verdict mixed = prove("(\\forall x \\in S\\ (x=x)) \\land (0=0)", 'n', 0, 0);
    passed = check_result(mixed.outcome == PROVED && mixed.certificate && unjustified_steps == previous,
                          "a mixed abstract and numeric sort receives no fictitious extensionality axiom") && passed;
    mixed = prove("(\\forall x \\in S\\ (x=x)) \\land (0 \\neq 0)", 'n', 0, 0);
    passed = check_result(mixed.outcome == REFUTED && mixed.certificate && unjustified_steps == previous,
                          "the false numeric variant in a mixed-sort statement is independently refuted") && passed;
    return passed;
}

int main(void) {
    engine_init();
    static const SearchCase cases[] = {
        {"reflexivity closes by equality resolution", "\\forall x \\in S\\ (x=x)", 1, SP_REFLEX},
        {"equality transitivity uses superposition",
         "(a \\in S \\land b \\in S \\land c \\in S \\land a=b \\land b=c) \\implies a=c", 1, SP_SUPER},
        {"function identities propagate inside nested terms",
         "(f \\colon S \\to S \\land \\forall x \\in S\\ (f(x)=x)) \\implies \\forall x \\in S\\ (f(f(x))=x)", 1, SP_SUPER},
        {"predicate excluded middle uses resolution",
         "R \\subseteq S \\implies \\forall x \\in S\\ (x \\in R \\lor x \\notin R)", 1, SP_RESOLVE},
        {"dependent Skolem functions preserve a repeated quantified condition",
         "(R \\subseteq S \\times T \\land \\forall x \\in S\\ \\exists y \\in T\\ ((x,y) \\in R)) \\implies "
         "\\forall x \\in S\\ \\exists y \\in T\\ ((x,y) \\in R)", 1, SP_RESOLVE},
        {"universal reflexivity never invents an element of an empty sort",
         "(\\forall x \\in S\\ (x=x)) \\implies \\exists x \\in S\\ (x=x)", 0, SP_INPUT},
        {"interchanging forall and exists remains unproved",
         "(R \\subseteq S \\times S \\land \\forall x \\in S\\ \\exists y \\in S\\ ((x,y) \\in R)) \\implies "
         "\\exists y \\in S\\ \\forall x \\in S\\ ((x,y) \\in R)", 0, SP_INPUT},
        {"a function without fixed points is not given a fixed point by cyclic unification",
         "(f \\colon S \\to S \\land \\forall x \\in S\\ (f(x) \\neq x)) \\implies \\exists x \\in S\\ (f(x)=x)", 0, SP_INPUT},
    };
    int passed = 1;
    for (size_t i = 0; i < sizeof cases / sizeof *cases; i++) passed = search_case(&cases[i]) && passed;
    passed = storage_checks() && passed;
    return passed ? 0 : 1;
}
