/* Selezione SInE e portafoglio di saturazione. Il riferimento della
 * selezione e una chiusura su insiemi di simboli forniti come dati, senza
 * gli indici sparsi del motore. Le prove usano ancora l'enunciato completo
 * nel certificato; il caso falso ha un modello esplicito su un singoletto.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 * cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_saturation_strategies \
 *     tests/engine/test_saturation_strategies.c && /tmp/test_saturation_strategies
 */
#include "../statement_checks.h"

/* Un letterale proposizionale del test: il simbolo e gia dichiarato come
 * predicato senza argomenti. Non introduce un risultato nel programma. */
static SpLiteral atom(SpState *s, int symbol, int negative) {
    return (SpLiteral){sp_term(s, symbol, NULL), 0, negative};
}

/* Chiusura matematica dei trigger, con una tabella booleana di dodici
 * simboli. La scansione esaustiva e indipendente dal grafo sparso di SInE. */
static void selection_oracle(const unsigned *rows, int count, unsigned goal, unsigned tolerance, unsigned char *selected) {
    unsigned frequency[12] = {0};
    for (int i = 0; i < count; i++) for (int j = 0; j < 12; j++) frequency[j] += !!(rows[i] & (1u << j));
    memset(selected, 0, (size_t)count);
    for (int changed = 1; changed;) {
        changed = 0;
        for (int i = 0; i < count; i++) {
            unsigned minimum = (unsigned)-1;
            for (int j = 0; j < 12; j++) if ((rows[i] & (1u << j)) && frequency[j] < minimum) minimum = frequency[j];
            int triggered = !rows[i];
            for (int j = 0; j < 12; j++)
                triggered |= !!(rows[i] & goal & (1u << j)) && frequency[j] <= tolerance * minimum;
            if (triggered && !selected[i]) { selected[i] = 1; goal |= rows[i]; changed = 1; }
        }
    }
}

/* Confronta cento teorie come dati con l'oracolo completo della selezione.
 * Un simbolo tecnico rappresenta le righe senza simboli originali. */
static int random_selections(void) {
    unsigned random = 0x7165u;
    int all = 1;
    for (int round = 0; round < 100 && all; round++) {
        SpState s = {.empty=-1};
        for (int i = 0; i < 13; i++) sp_symbol(&s, 0, 1);
        s.inputs = 12;
        unsigned rows[40];
        for (int i = 0; i < 40; i++) {
            random = random * 1664525u + 1013904223u;
            rows[i] = (random >> 8) & 4095u;
            if (i % 7 == 0) rows[i] = 0;
            SpLiteral literal[12]; int count = 0;
            for (int j = 0; j < 12; j++) if (rows[i] & (1u << j)) literal[count++] = atom(&s, j, 0);
            if (!count) literal[count++] = atom(&s, 12, 0);
            sp_keep(&s, literal, count, (SpStep){.rule=SP_INPUT}, -1);
        }
        s.initial = s.clauses;
        all &= s.initial == 40 && sp_sine_prepare(&s);
        unsigned goals = (random >> 17) & 4095u;
        unsigned char goal[12], expected[40], selected[40];
        for (int j = 0; j < 12; j++) goal[j] = !!(goals & (1u << j));
        for (unsigned tolerance = 1; tolerance <= 2 && all; tolerance++) {
            selection_oracle(rows, 40, goals, tolerance, expected);
            sp_scratch_reset(&s);
            all &= sp_sine_select(&s, goal, tolerance, selected) && memcmp(expected, selected, sizeof selected) == 0;
        }
        sp_clear(&s);
    }
    return check_result(all, "both SInE tolerances agree with an independent fixed-point oracle on 100 theories");
}

/* c e comune, d raro: c non attiva c OR d con tolleranza uno, ma lo fa
 * con due. L'ipotesi e resta estranea e quella tecnica resta disponibile. */
static int tolerance_modes(void) {
    SpState s = {.empty=-1};
    for (int i = 0; i < 6; i++) sp_symbol(&s, 0, 1);
    s.inputs = 5;
    SpLiteral pairs[][2] = {{atom(&s, 0, 0), atom(&s, 1, 0)},
                            {atom(&s, 1, 0), atom(&s, 2, 0)}, {atom(&s, 2, 0), atom(&s, 3, 0)}};
    for (int i = 0; i < 3; i++) sp_keep(&s, pairs[i], 2, (SpStep){.rule=SP_INPUT}, -1);
    SpLiteral independent = atom(&s, 4, 0), technical = atom(&s, 5, 0);
    sp_keep(&s, &independent, 1, (SpStep){.rule=SP_INPUT}, -1);
    sp_keep(&s, &technical, 1, (SpStep){.rule=SP_INPUT}, -1); s.initial = s.clauses;
    unsigned char goal[] = {1, 0, 0, 0, 0}, selected[5], strict[] = {1, 1, 0, 0, 1}, loose[] = {1, 1, 1, 0, 1};
    int ok = sp_sine_prepare(&s) && sp_sine_select(&s, goal, 1, selected) && memcmp(selected, strict, sizeof strict) == 0;
    sp_scratch_reset(&s);
    ok &= sp_sine_select(&s, goal, 2, selected) && memcmp(selected, loose, sizeof loose) == 0;
    sp_clear(&s);
    return check_result(ok, "tolerance two expands rare-symbol triggers while unrelated clauses stay excluded");
}

/* La frequenza conta le clausole: le due occorrenze di f in p(f(f(x)))
 * non rendono f due volte piu comune di p. */
static int symbol_frequency(void) {
    SpState s = {.empty=-1};
    int f = sp_symbol(&s, 1, 0), p = sp_symbol(&s, 1, 1), x = -1;
    s.inputs = 2;
    int fx = sp_term(&s, f, &x), ffx = sp_term(&s, f, &fx);
    SpLiteral literal = {sp_term(&s, p, &ffx), 0, 0};
    sp_keep(&s, &literal, 1, (SpStep){.rule=SP_INPUT}, -1); s.initial = s.clauses;
    int ok = sp_sine_prepare(&s) && s.sine.frequency[f] == 1 && s.sine.frequency[p] == 1 && s.sine.first[1] == 2;
    sp_clear(&s);
    return check_result(ok, "repeated function occurrences count once per clause");
}

/* Una clausola sospesa non puo cancellare una deduzione attiva che sussume:
 * da a OR b e NOT a segue b, anche se un'altra copia iniziale di b e sospesa. */
static int held_subsumption(void) {
    SpState s = {.empty=-1};
    sp_symbol(&s, 0, 1); sp_symbol(&s, 0, 1);
    SpLiteral first[] = {atom(&s, 0, 0), atom(&s, 1, 0)}, second = atom(&s, 0, 1), held = atom(&s, 1, 0);
    sp_keep(&s, first, 2, (SpStep){.rule=SP_INPUT}, -1);
    sp_keep(&s, &second, 1, (SpStep){.rule=SP_INPUT}, -1);
    sp_keep(&s, &held, 1, (SpStep){.rule=SP_INPUT}, -1); s.initial = s.clauses;
    unsigned char selected[] = {1, 1, 0};
    int ok = sp_requeue(&s, selected) == 1;
    sp_resolve(&s, 0, 1);
    ok &= s.clauses == 4 && s.clause[3].stage == SP_PENDING && s.clause[3].count == 1 &&
          s.clause[3].literal[0].left == held.left && !s.clause[3].literal[0].negative;
    sp_clear(&s);
    return check_result(ok, "a held clause cannot discard an available resolution consequence");
}

/* La quota ferma una clausola gia ACTIVE prima di averne finito le
 * inferenze. La ripartenza la rimette PENDING e recupera la contraddizione. */
static int resumed_search(void) {
    SpState s = {.empty=-1};
    sp_symbol(&s, 0, 1); sp_symbol(&s, 0, 1);
    SpLiteral a = atom(&s, 0, 0), rest[] = {atom(&s, 0, 1), atom(&s, 1, 0)}, b = atom(&s, 1, 1);
    sp_keep(&s, &a, 1, (SpStep){.rule=SP_INPUT}, -1);
    sp_keep(&s, rest, 2, (SpStep){.rule=SP_INPUT}, -1);
    sp_keep(&s, &b, 1, (SpStep){.rule=SP_INPUT}, -1); s.initial = s.clauses;
    s.quota = s.work + 5;
    int ok = !sp_search(&s) && s.limited && s.resumable;
    int active = 0;
    for (int i = 0; i < s.clauses; i++) active += s.clause[i].stage == SP_ACTIVE;
    s.limited = s.resumable = 0; s.quota = 0;
    sp_requeue(&s, NULL);
    for (int i = 0; i < s.clauses; i++) ok &= s.clause[i].stage != SP_ACTIVE;
    ok &= active > 0 && sp_search(&s) && s.clause[s.empty].count == 0;
    sp_clear(&s);
    return check_result(ok, "resuming a work slice requeues interrupted active clauses and completes their inferences");
}

/* Una mancanza di memoria o una grammatica non trattata non e una quota
 * locale: riprendere uno stato incompleto non deve inventare i suoi dati. */
static int terminal_limits(void) {
    SpState s = {.bytes=saturation_memory_limit};
    sp_symbol(&s, 0, 1);
    int ok = s.limited && !s.resumable;
    sp_clear(&s);
    s.unsupported = 1;
    ok &= !sp_sine_prepare(&s) && !sp_sine_select(&s, NULL, 1, NULL) && !sp_portfolio(&s, NULL) && !s.resumable;
    sp_clear(&s);
    return check_result(ok, "memory exhaustion and unsupported input cannot be resumed as a work slice");
}

/* Costruisce un problema proposizionale inconsistente con molte ipotesi
 * estranee. L'oracolo e a AND NOT a, indipendente da ogni altro simbolo. */
static void background(SpState *s, int irrelevant) {
    *s = (SpState){.empty=-1};
    for (int i = 0; i <= irrelevant; i++) {
        int symbol = sp_symbol(s, 0, 1);
        SpLiteral literal = atom(s, symbol, 0);
        sp_keep(s, &literal, 1, (SpStep){.rule=SP_INPUT}, -1);
    }
    SpLiteral denied = atom(s, irrelevant, 1);
    sp_keep(s, &denied, 1, (SpStep){.rule=SP_INPUT}, -1);
    s->inputs = s->symbols; s->initial = s->clauses;
}

/* Stesso budget per due strategie sullo stesso dato. Il selettore deve
 * risparmiare lavoro effettivo, senza una scorciatoia per la formula. */
static int large_background(void) {
    SpState s;
    unsigned long long limit = saturation_work_limit;
    background(&s, 320); saturation_work_limit = s.work + 20000;
    int full = sp_search(&s);
    int stopped = !full && s.limited && !s.resumable;
    sp_clear(&s); saturation_work_limit = limit;
    background(&s, 320); saturation_work_limit = s.work + 20000;
    unsigned char goal[321] = {0}, selected[322]; goal[320] = 1;
    int ok = sp_sine_prepare(&s) && sp_sine_select(&s, goal, 1, selected);
    int held = ok ? sp_requeue(&s, selected) : 0;
    ok &= held == 320 && sp_search(&s);
    sp_clear(&s); saturation_work_limit = limit;
    return check_result(stopped && ok, "SInE proves the same contradiction within a budget exhausted by an unrelated background");
}

/* La formula originale resta nel certificato. expected_phase distingue
 * una prova selettiva dalla ripresa con tutte le ipotesi; zero e il falso. */
static int problem(const char *name, const char *formula, int expected_phase) {
    arena_used = arena_keep; failure = syntax_error = NULL; work_allocation_failed = 0;
    cursor = formula; next_token();
    const Sentence *root = abstract_statement(); declared_symbols = symbol_count;
    SpState s = {0};
    int ready = !failure && !syntax_error && sp_prepare(&s, root, NULL, 0);
    int found = ready && sp_portfolio(&s, root);
    int ok = ready && (expected_phase ? found && s.strategy == expected_phase : !found);
    if (found) {
        sp_write(&s, root, NULL, 0, 0);
        const char *text = certificate_text, *why = "incomplete certificate";
        ok &= !certificate_broken && ck_check(&text, &why, 0) == CK_ACCEPTED;
        if (!ok) printf("Certificate details: %s\n", why);
    }
    if (!ok) printf("Portfolio details: ready=%d found=%d strategy=%d work=%llu limited=%d parse=%s\n",
                    ready, found, s.strategy, s.work, s.limited, syntax_error ? syntax_error : "none");
    sp_clear(&s);
    return check_result(ok, name);
}

/* I simboli impliciti precedono f nella traduzione, pur seguendola nel
 * testo. L'insieme iniziale della tesi deve seguire la mappa, non il testo. */
static int goal_symbols(void) {
    arena_used = arena_keep; failure = syntax_error = NULL; work_allocation_failed = 0;
    cursor = "(f \\colon S \\to T \\land F \\subseteq \\mathcal{P}(T)) \\implies \\forall x \\in S\\ (f(x)=f(x))";
    next_token(); const Sentence *root = abstract_statement(); declared_symbols = symbol_count;
    SpState s = {0};
    int ok = !syntax_error && sp_prepare(&s, root, NULL, 0);
    unsigned char *goal = calloc((size_t)s.inputs, 1);
    if (ok && goal) {
        sp_sine_goal(&s, root->b, goal);
        int original = symbol_find('f', 0), selected = 0;
        for (int i = 0; i < s.inputs; i++) selected += goal[i] != 0;
        ok &= original >= 0 && selected == 1 && goal[s.input_symbol[original]];
    } else ok = 0;
    free(goal); sp_clear(&s);
    return check_result(ok, "goal relevance follows the declaration map across implicit sort symbols");
}

/* Esegue selezione, ripartenza e prove certificate; ritorna zero soltanto
 * se le varianti false restano senza refutazione e tutti i controlli passano. */
int main(void) {
    engine_init();
    int passed = random_selections();
    passed = tolerance_modes() && passed;
    passed = symbol_frequency() && passed;
    passed = held_subsumption() && passed;
    passed = resumed_search() && passed;
    passed = terminal_limits() && passed;
    passed = large_background() && passed;
    passed = goal_symbols() && passed;
    passed = problem("a selective proof is checked against the complete original statement",
        "(P \\subseteq S \\land Q \\subseteq S \\land (\\forall x \\in S\\ (x \\in P)) \\land "
        "(\\forall x \\in S\\ (x \\in Q))) \\implies \\forall x \\in S\\ (x \\in P)", 1) && passed;
    passed = problem("the broader second strategy recovers a needed less-rare premise",
        "(c \\in S \\land P \\subseteq S \\land Q \\subseteq S \\land R \\subseteq S \\land E \\subseteq S \\land "
        "c \\in P \\land (c \\in P \\implies c \\in Q) \\land (c \\in Q \\lor c \\in R) \\land c \\in E) \\implies c \\in Q", 2) && passed;
    passed = problem("disconnected inconsistent hypotheses are recovered by the complete final strategy",
        "(P \\subseteq S \\land Q \\subseteq S \\land (\\forall x \\in S\\ (x \\in P)) \\land "
        "(\\exists x \\in S\\ (x \\notin P))) \\implies \\forall x \\in S\\ (x \\in Q)", 3) && passed;
    passed = problem("consistent unrelated premises cannot prove the false conclusion on a singleton",
        "(P \\subseteq S \\land Q \\subseteq S \\land R \\subseteq S \\land (\\forall x \\in S\\ (x \\in P)) \\land "
        "(\\forall x \\in S\\ (x \\in Q))) \\implies \\forall x \\in S\\ (x \\in R)", 0) && passed;
    return passed ? 0 : 1;
}
