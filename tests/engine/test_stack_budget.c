/* La pila di lavoro e il budget dei passi dei modelli.
 *
 * Da quando un enunciato può superare MAX_FORMULA caratteri, un albero può
 * avere più livelli di quanti la pila ne regga alle funzioni ricorsive che
 * lo visitano: 20.000 negazioni una dentro l'altra, o 100.000 ipotesi unite
 * una dopo l'altra, fermavano il programma invece di dare un esito. Qui si
 * controlla che ogni forma di annidamento dia "pila di lavoro esaurita", che
 * gli altri linguaggi rifiutino un enunciato lungo, che le catene di ipotesi
 * non costino livelli (sono unite a metà), e che un controesempio che non si
 * riesce a ricalcolare entro il budget dei passi lo dica, senza diventare
 * un FALSO né un silenzio.
 *
 * Gli enunciati sono generati: l'oracolo è la loro costruzione (un numero
 * pari di negazioni di un enunciato vero, due costanti che nulla obbliga a
 * coincidere, un'involuzione che non è l'identità).
 *
 * Compilazione ed esecuzione dalla cartella principale:
 * cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_stack_budget \
 *     tests/engine/test_stack_budget.c && /tmp/test_stack_budget
 */
#include "../statement_checks.h"

/* Un testo in costruzione: text ha size byte, di cui used scritti. */
typedef struct {
    char *text;
    size_t size, used;
} Built;

/* Ritorna un testo vuoto con posto per size caratteri. */
static Built built_new(size_t size) {
    Built d = {malloc(size + 1), size + 1, 0};
    if (!d.text) { printf("FAIL: out of memory building a test statement\n"); exit(1); }
    d.text[0] = 0;
    return d;
}

/* Aggiunge piece in fondo a d, che deve avere posto. */
static void built_add(Built *d, const char *piece) {
    size_t length = strlen(piece);
    if (length >= d->size - d->used) { printf("FAIL: a test statement is longer than the room made for it\n"); exit(1); }
    memcpy(d->text + d->used, piece, length + 1);
    d->used += length;
}

/* Ritorna, in memoria da liberare, head seguito da count copie di piece
 * (separate da between, se non è NULL) e poi da tail. */
static char *repeated(const char *head, const char *piece, const char *between, int count, const char *tail) {
    size_t step = strlen(piece) + (between ? strlen(between) : 0);
    Built d = built_new(strlen(head) + step * (size_t)count + strlen(tail));
    built_add(&d, head);
    for (int i = 0; i < count; i++) {
        if (i && between) built_add(&d, between);
        built_add(&d, piece);
    }
    built_add(&d, tail);
    return d.text;
}

/* Ritorna, in memoria da liberare, head, poi depth copie di open, core,
 * depth copie di close, e tail: core annidato depth volte. */
static char *nested(const char *head, const char *open, int depth, const char *core, const char *close, const char *tail) {
    Built d = built_new(strlen(head) + (strlen(open) + strlen(close)) * (size_t)depth + strlen(core) + strlen(tail));
    built_add(&d, head);
    for (int i = 0; i < depth; i++) built_add(&d, open);
    built_add(&d, core);
    for (int i = 0; i < depth; i++) built_add(&d, close);
    built_add(&d, tail);
    return d.text;
}

/* Come repeated(), con il numero della copia, da 1, al posto dell'unico %d
 * di piece. */
static char *numbered(const char *head, const char *piece, const char *between, int count, const char *tail) {
    Built d = built_new(strlen(head) + (strlen(piece) + 12 + strlen(between)) * (size_t)count + strlen(tail));
    built_add(&d, head);
    for (int i = 1; i <= count; i++) {
        char one[128];
        snprintf(one, sizeof one, piece, i);
        if (i > 1) built_add(&d, between);
        built_add(&d, one);
    }
    built_add(&d, tail);
    return d.text;
}

/* Dimostra text, lo libera, e ritorna 1 se l'esito è "pila di lavoro
 * esaurita" senza dimostrazione né certificato. */
static int exhausts_stack(char *text) {
    Verdict v = prove(text, 'n', 0, 0);
    free(text);
    return v.outcome == NOT_PROVED && v.reason == STACK_EXHAUSTED && !v.proof && !v.certificate && !v.detail;
}

/* 1. Ogni forma di annidamento del linguaggio degli insiemi qualsiasi, a
 * una profondità che la pila non regge: l'esito c'è, ed è quello. */
static int deep_abstract_statements(void) {
    enum { DEPTH = 100000 };
    int passed = check_result(exhausts_stack(repeated("", "\\lnot ", NULL, DEPTH, "(\\forall x \\in S\\ (x = x))")),
                              "100000 nested negations end with an exhausted stack budget, not a crash");
    passed = check_result(exhausts_stack(nested("\\forall x \\in S\\ ", "(", DEPTH, "x = x", ")", "")),
                          "100000 nested parentheses end with an exhausted stack budget") && passed;
    passed = check_result(exhausts_stack(nested("(f \\colon S \\to S) \\implies \\forall x \\in S\\ (", "f(", DEPTH, "x", ")", " = x)")),
                          "100000 nested applications end with an exhausted stack budget") && passed;
    passed = check_result(exhausts_stack(numbered("", "\\forall x_{%d} \\in S\\ ", "", DEPTH, "(x_{1} = x_{1})")),
                          "100000 nested quantifiers end with an exhausted stack budget") && passed;
    passed = check_result(exhausts_stack(repeated("\\forall x \\in S\\ (", "x = x", " \\implies ", 3 * DEPTH, ")")),
                          "300000 nested implications end with an exhausted stack budget") && passed;
    passed = check_result(exhausts_stack(repeated("\\forall x \\in S\\ (", "x = x", " \\iff ", DEPTH, ")")),
                          "a chain of 100000 equivalences is counted against the stack budget") && passed;

    /* Gli elenchi dopo un quantificatore: un livello per lettera, senza che
     * la lettura scenda. Non erano contati, e fermavano il programma. */
    passed = check_result(exhausts_stack(numbered("\\forall ", "x_{%d}", ", ", 60000, " \\in S\\ (x_{1} = x_{1})")),
                          "a quantifier over 60000 listed letters is counted against the stack budget") && passed;
    passed = check_result(exhausts_stack(numbered("(\\exists ", "X_{%d}", ", ", DEPTH, " \\subseteq S\\ (X_{1} = X_{1})) \\implies \\forall x \\in S\\ (x = x)")),
                          "a quantifier over 100000 listed parts is counted against the stack budget") && passed;
    passed = check_result(exhausts_stack(numbered("\\exists ", "S_{%d}", ", ", 60000, " \\neq \\emptyset\\ \\exists x \\in S_{1}\\ (x = x)")),
                          "60000 nonempty sorts in an example header are counted against the stack budget") && passed;
    return passed;
}

/* 2. Gli altri linguaggi non crescono con l'enunciato: uno lungo lo
 * rifiutano, com'era quando era il lettore degli \item a fermarsi. */
static int long_statements_elsewhere(void) {
    enum { DEPTH = 20000 };
    struct { char *text; const char *what; } cases[] = {
        {nested("", "(", DEPTH, "n", ")", " = n"), "numeric parentheses"},
        {nested("", "\\sqrt{", DEPTH, "n", "}", " = n"), "nested square roots"},
        {repeated("", "n", " + ", DEPTH, " = n"), "a numeric sum"},
        {repeated("\\forall x \\in \\mathbb{N}\\ (", "x = x", " \\land ", DEPTH, ")"), "a conjunction over the naturals"},
        {nested("", "(", DEPTH, "S", ")", " \\subseteq S"), "parentheses around a set"},
        {nested("", "\\mathcal{P}(", DEPTH, "S", ")", " = S"), "nested power sets"},
        {nested("\\lim_{x \\to 0} ", "\\sin(", DEPTH, "x", ")", " = 0"), "a limit"},
        {nested("\\frac{d}{dx} ", "\\sin(", DEPTH, "x", ")", " = 1"), "a derivative"},
        {repeated("\\int ", "x", " + ", DEPTH, "\\,dx = x"), "an integral"},
    };
    /* Un'applicazione tra parti (17r) la decide il capitolo 15: corta sì,
     * e con la tesi ripetuta fino a superare la lunghezza no. Il motivo è la
     * lunghezza, non l'errore del lettore del capitolo 28 a cui tocca dopo. */
    const char *maps = "(f \\colon \\mathcal{P}(S) \\to \\mathcal{P}(S) \\land (S \\neq \\emptyset \\land E \\subseteq S \\land "
                       "\\forall X \\in \\mathcal{P}(S)\\ (f(X) = X \\cap E))) \\implies (";
    const char *claim = "(((\\forall a, b \\in \\mathcal{P}(S)\\ (f(a) = f(b) \\implies a = b)) \\iff E = S))";
    char *once = repeated(maps, claim, " \\land ", 1, ")"), *often = repeated(maps, claim, " \\land ", 60, ")");
    Verdict v = prove(once, 'n', 0, 0);
    int passed = check_result(strlen(once) < MAX_FORMULA && v.outcome == PROVED, "a short statement about a map between parts is proved");
    v = prove(often, 'n', 0, 0);
    passed = check_result(strlen(often) >= MAX_FORMULA && v.outcome == UNSUPPORTED && v.reason && strcmp(v.reason, "enunciato troppo lungo") == 0,
                          "the same statement with its claim repeated 60 times is refused as too long") && passed;
    free(once);
    free(often);
    for (size_t i = 0; i < sizeof cases / sizeof *cases; i++) {
        char summary[160];
        v = prove(cases[i].text, 'n', 0, 0);
        snprintf(summary, sizeof summary, "%s longer than a statement of its language is refused as too long", cases[i].what);
        passed = check_result(v.outcome == UNSUPPORTED && v.reason && strcmp(v.reason, "enunciato troppo lungo") == 0 && !v.certificate, summary) && passed;
        free(cases[i].text);
    }
    return passed;
}

/* Ritorna quante parentesi sono aperte, al più, una dentro l'altra nella
 * riga "statement" del certificato. */
static int statement_nesting(const char *certificate) {
    const char *line = strstr(certificate, "\nstatement ");
    int depth = 0, deepest = 0;
    for (const char *p = line ? line + 1 : ""; *p && *p != '\n'; p++) {
        if (*p == '(' && ++depth > deepest) deepest = depth;
        if (*p == ')') depth--;
    }
    return deepest;
}

/* 3. Le ipotesi non costano livelli. Tremila costanti dichiarate e una tesi
 * che nulla impone: due elementi la smentiscono. Poi più ipotesi di quante
 * parentesi una dentro l'altra il verificatore legga (4000): unite una dopo
 * l'altra davano un certificato che non si poteva controllare. */
static int long_hypothesis_chains(void) {
    char *text = numbered("(", "c_{%d} \\in S", " \\land ", 3000, ") \\implies c_{1} = c_{2}");
    Verdict v = prove(text, 'n', 0, 0);
    free(text);
    int passed = check_result(v.outcome == REFUTED && v.certificate && strstr(v.certificate, "sort S 2\n"),
                              "3000 hypotheses leave a certified two-element counterexample");
    passed = check_result(v.certificate && statement_nesting(v.certificate) <= 16,
                          "the certificate nests 3000 hypotheses in at most 16 levels, joined in halves") && passed;

    text = repeated("(c \\in S \\land d \\in S \\land ", "c = c", " \\land ", 4500, ") \\implies c = d");
    v = prove(text, 'n', 0, 0);
    free(text);
    passed = check_result(v.outcome == REFUTED && v.certificate && statement_nesting(v.certificate) <= 17,
                          "4502 hypotheses, more than the checker's nesting limit, still give a checked counterexample") && passed;

    /* Dentro 15 parentesi il lettore prova tre letture e torna indietro due
     * volte per parentesi: le 1100 unioni vanno contate una volta sola.
     * Contate a ogni rilettura sembravano troppe. L'unione di X con sé
     * stessa è X, non S: lo smentisce X vuoto in S non vuoto. */
    char *unions = repeated("", "X", " \\cup ", 1100, " = S");
    text = nested("(X \\subseteq S \\land c \\in S) \\implies ", "(", 15, unions, ")", "");
    v = prove(text, 'n', 0, 0);
    free(unions);
    free(text);
    return check_result(v.outcome == REFUTED && v.certificate,
                        "1100 unions inside 15 parentheses are counted once, and the false statement is refuted") && passed;
}

/* 4. La guardia riguarda solo gli enunciati lunghi: uno corto e molto
 * annidato si decide come sempre. Seicento negazioni, un numero pari, di
 * un enunciato vero lo lasciano vero. Dopo un enunciato che esaurisce la
 * pila, il successivo riparte da capo. */
static int short_statements_and_recovery(void) {
    char *text = repeated("", "\\lnot ", NULL, 600, "(\\forall x \\in S\\ (x = x))");
    int fits = strlen(text) < MAX_FORMULA;
    Verdict v = prove(text, 'n', 0, 0);
    free(text);
    int passed = check_result(fits && v.outcome == PROVED && v.certificate,
                              "600 nested negations in a short statement are still proved with a certificate");
    passed = check_result(exhausts_stack(repeated("", "\\lnot ", NULL, 50000, "(\\forall x \\in S\\ (x = x))")) && stack_exhausted,
                          "a statement that is too deep sets the sticky stack flag") && passed;
    v = prove("\\forall x \\in S\\ (x = x)", 'n', 0, 0);
    passed = check_result(v.outcome == PROVED && v.certificate && !stack_exhausted,
                          "the statement after an exhausted one is proved from a clean state") && passed;

    /* Anche uno lungo: il conto delle catene dell'enunciato prima non deve
     * arrivare ai suoi primi token. */
    passed = check_result(exhausts_stack(repeated("\\forall x \\in S\\ (", "x = x", " \\iff ", 100000, ")")),
                          "a long chain exhausts the budget again") && passed;
    text = numbered("(", "c_{%d} \\in S", " \\land ", 3000, ") \\implies c_{1} = c_{2}");
    v = prove(text, 'n', 0, 0);
    free(text);
    return check_result(v.outcome == REFUTED && v.certificate,
                        "a long statement after an exhausted one starts with a clean chain count") && passed;
}

/* 5. Il budget dei passi. Un'involuzione non è per forza l'identità: lo
 * smentisce lo scambio di due elementi. Ricalcolare l'enunciato su quel
 * modello chiede tre passi dei quantificatori: con un budget di due il
 * modello trovato non si conferma, e il verdetto deve dirlo; con il budget
 * consueto è un controesempio certificato. */
static int model_step_budget(void) {
    const char *text = "(f \\colon S \\to S \\land \\forall x \\in S\\ (f(f(x)) = x)) \\implies \\forall x \\in S\\ (f(x) = x)";
    long long saved = model_step_limit;
    model_step_limit = 2;
    Verdict v = prove(text, 'n', 0, 0);
    model_step_limit = saved;
    int passed = check_result(v.outcome == NOT_PROVED && v.reason && v.reason == model_unconfirmed && strstr(v.reason, "modello candidato") &&
                              !v.certificate && !v.detail,
                              "a model that cannot be recomputed within the step budget is reported as such, not as a counterexample");
    v = prove(text, 'n', 0, 0);
    passed = check_result(v.outcome == REFUTED && v.certificate && strstr(v.certificate, "sort S 2\n"),
                          "with the usual step budget the same statement has a certified two-element counterexample") && passed;

    /* Il verificatore riceve due volte e mezza il budget del motore: con
     * quello consueto, i cento milioni che erano la sua costante. */
    passed = check_result(checker_steps() == 100000000LL, "the checker's usual step budget is 2.5 times the engine's") && passed;
    model_step_limit = 1000000000LL;
    long long wide = checker_steps();
    model_step_limit = LLONG_MAX - 1;
    long long widest = checker_steps();
    model_step_limit = saved;
    return check_result(wide == 2500000000LL && widest == LLONG_MAX - 1,
                        "a larger engine budget widens the checker's, without overflow at the largest one") && passed;
}

/* 6. L'arena piena. Lo svolgimento di \\exists! raddoppia l'enunciato a
 * ogni livello: 22 uno dentro l'altro, in 500 caratteri, riempiono la
 * memoria di lavoro; un milione di congiunzioni la riempiono leggendo. Il
 * nodo di riserva era congiunto con sé stesso, e chi visitava l'albero non
 * finiva più: ora l'esito è "memoria di lavoro esaurita". */
static int full_arena(void) {
    char *text = numbered("", "\\exists! x_{%d} \\in S\\ ", "", 22, "(x_{1} = x_{1})");
    int fits = strlen(text) < MAX_FORMULA;
    Verdict v = prove(text, 'n', 0, 0);
    free(text);
    int passed = check_result(fits && v.outcome != PROVED && v.outcome != REFUTED && v.reason == OUT_OF_MEMORY && !v.certificate,
                              "22 nested unique-existence quantifiers end with exhausted memory, not a crash");
    text = repeated("\\forall x \\in S\\ (", "x = x", " \\land ", 1000000, ")");
    v = prove(text, 'n', 0, 0);
    free(text);
    passed = check_result(v.outcome != PROVED && v.outcome != REFUTED && v.reason == OUT_OF_MEMORY && !v.certificate,
                          "a million conjunctions end with exhausted memory, not a crash") && passed;
    v = prove("\\forall x \\in S\\ (x = x)", 'n', 0, 0);
    return check_result(v.outcome == PROVED && v.certificate, "the statement after a full arena is proved from a clean state") && passed;
}

int main(void) {
    engine_init();

    /* Tutti i controlli vengono eseguiti anche se uno fallisce, così il
     * resoconto è completo. */
    int passed = deep_abstract_statements();
    passed = long_statements_elsewhere() && passed;
    passed = long_hypothesis_chains() && passed;
    passed = short_statements_and_recovery() && passed;
    passed = model_step_budget() && passed;
    passed = full_arena() && passed;
    printf(passed ? "All stack and step budget checks passed.\n" : "Some stack or step budget checks FAILED.\n");
    return passed ? 0 : 1;
}
