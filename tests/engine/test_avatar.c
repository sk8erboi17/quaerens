/* AVATAR: scelta dei rami SAT e replay delle risoluzioni, con un oracolo
 * esaustivo indipendente per le CNF proposizionali. Le formule FO sono
 * dati del test e le prove passano soltanto attraverso ck_check.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 * cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_avatar \
 *     tests/engine/test_avatar.c && /tmp/test_avatar
 */
#include "../statement_checks.h"

/* Un atomo del test, eventualmente con un argomento variabile. I simboli
 * sono dichiarati dal chiamante; negative sceglie soltanto il segno. */
static SpLiteral literal(SpState *s, int symbol, int argument, int negative) {
    return (SpLiteral){sp_term(s, symbol, argument ? &argument : NULL), 0, negative};
}

/* Valuta la CNF data usando bit per le proposizioni, senza clausificazione,
 * SAT, sostituzioni o metodi del motore. Zero termina ogni riga. */
static int cnf_holds(const int rows[][4], int count, unsigned assignment) {
    for (int i = 0; i < count; i++) {
        int some = 0;
        for (int j = 0; j < 4 && rows[i][j]; j++) {
            int l = rows[i][j], v = l < 0 ? -l : l;
            some |= (int)(assignment >> (v - 1) & 1u) != (l < 0);
        }
        if (!some) return 0;
    }
    return 1;
}

/* Costruisce le clausole come dati. L'oracolo resta sulla matrice originale,
 * non sulle clausole normalizzate o sulle componenti introdotte dal motore. */
static void cnf_state(SpState *s, const int rows[][4], int count, int variables) {
    *s = (SpState){.empty=-1};
    for (int i = 0; i < variables; i++) sp_symbol(s, 0, 1);
    for (int i = 0; i < count; i++) {
        SpLiteral out[4]; int used = 0;
        for (int j = 0; j < 4 && rows[i][j]; j++) {
            int l = rows[i][j];
            out[used++] = literal(s, (l < 0 ? -l : l) - 1, 0, l < 0);
        }
        sp_keep(s, out, used, (SpStep){.rule=SP_INPUT}, -1);
    }
    s->initial = s->clauses; s->avatar = s->avatar_dirty = 1;
}

/* Cento teorie casuali con tre variabili vengono decise enumerando gli
 * otto vettori booleani. Un SAT erroneo o una falsa refutazione falliscono
 * il confronto; il primo caso richiede un vero apprendimento CDCL. */
static int propositional_oracle(void) {
    unsigned random = 0x53ac13u; int all = 1, learned = 0, sat = 0, unsat = 0;
    for (int round = 0; round < 100 && all; round++) {
        int rows[16][4] = {{1,2,0,0},{1,-2,0,0},{-1,2,0,0},{-1,-2,0,0}};
        int count = round ? 4 + round % 13 : 4;
        if (round) for (int i = 0; i < count; i++) {
            for (int j = 0; j < 3; j++) {
                random = random * 1664525u + 1013904223u;
                int v = (int)(random >> 12) % 3 + 1;
                rows[i][j] = random & 1u ? v : -v;
            }
            rows[i][3] = 0;
        }
        int models = 0;
        for (unsigned assignment = 0; assignment < 8; assignment++) models += cnf_holds(rows, count, assignment);
        SpState s; cnf_state(&s, rows, count, 3);
        int found = sp_search(&s);
        all &= !s.limited && found == !models;
        sat += models != 0; unsat += !models;
        for (size_t i = 0; i < s.sat.clauses; i++) if (s.sat_proof[i] && s.clause[s.sat_proof[i] - 1].step.rule == SP_RESOLVE) learned++;
        if (!all) printf("CNF details: round=%d models=%d found=%d limited=%d work=%llu\n",round,models,found,s.limited,s.work);
        sp_clear(&s);
    }
    return check_result(all && learned && sat && unsat, "100 SAT-guided searches agree with exhaustive Boolean truth tables and replay learned clauses");
}

/* Lo split rispetta le connessioni fra variabili, anche quando una
 * variabile collega indirettamente due letterali attraverso un terzo. */
static int variable_components(void) {
    SpState s = {.empty=-1, .avatar=1};
    int p = sp_symbol(&s, 1, 1), q = sp_symbol(&s, 1, 1);
    SpLiteral shared[] = {literal(&s,p,-1,0),literal(&s,q,-1,0)};
    sp_keep(&s, shared, 2, (SpStep){.rule=SP_INPUT}, -1); sp_split(&s,0);
    int ok = !s.limited && s.components == 0 && s.clauses == 1 && s.clause[0].stage != SP_DELETED;
    sp_clear(&s);
    s = (SpState){.empty=-1,.avatar=1};
    p = sp_symbol(&s,1,1); q = sp_symbol(&s,1,1);
    SpLiteral disjoint[] = {literal(&s,p,-1,0),literal(&s,q,-2,1)};
    sp_keep(&s,disjoint,2,(SpStep){.rule=SP_INPUT},-1); sp_split(&s,0);
    ok &= !s.limited && s.components == 2 && s.clause[0].stage == SP_DELETED;
    ok &= s.component[0].key.variables == 1 && s.component[1].key.variables == 1 && s.component[1].key.literal[0].negative;
    ok &= s.component[0].reverse == -1 && s.component[1].reverse == -1;
    sp_clear(&s);
    s = (SpState){.empty=-1,.avatar=1};
    p=sp_symbol(&s,1,1); q=sp_symbol(&s,1,1); int r=sp_symbol(&s,2,1), args[]={-1,-2};
    SpLiteral connected[]={literal(&s,p,-1,0),literal(&s,q,-2,0),{sp_term(&s,r,args),0,0}};
    sp_keep(&s,connected,3,(SpStep){.rule=SP_INPUT},-1); sp_split(&s,0);
    ok &= !s.limited && s.components==0 && s.clause[0].stage!=SP_DELETED;
    sp_clear(&s);
    return check_result(ok,"shared variables stay connected and negative universal components do not acquire illegal complements");
}

/* Regressione: una quota durante la partizione non puo usare un nodo
 * interno della union-find come un gruppo gia numerato. Ogni interruzione
 * conserva il genitore; una partizione completata trova proprio due gruppi. */
static int split_work_slices(void) {
    int ok=1, stopped=0, complete=0;
    for (unsigned cut=1;cut<=500 && ok;cut++) {
        SpState s={.empty=-1,.avatar=1};
        int p=sp_symbol(&s,1,1),q=sp_symbol(&s,1,1),r=sp_symbol(&s,1,1);
        SpLiteral data[]={literal(&s,p,-1,0),literal(&s,q,-1,0),literal(&s,r,-2,0)};
        sp_keep(&s,data,3,(SpStep){.rule=SP_INPUT},-1);
        s.quota=s.work+cut; sp_split(&s,0);
        if (s.limited) { stopped++; ok &= s.resumable && s.clause[0].stage!=SP_DELETED && s.empty<0; }
        else { complete++; ok &= s.components==2 && s.clause[0].stage==SP_DELETED; }
        sp_clear(&s);
    }
    return check_result(ok && stopped && complete,"500 work slices cannot turn a partially visited union-find node into an invalid component index");
}

/* Il segno di una guardia e parte della clausola completa: il ramo la
 * richiede quando e falsa. Un cambiamento del modello non riscrive la
 * formula e riattiva tutte le clausole ora necessarie. */
static int guarded_branches(void) {
    SpState s = {.empty=-1,.avatar=1};
    int p=sp_symbol(&s,1,1), c=sp_symbol(&s,0,0), constant=sp_term(&s,c,NULL);
    SpLiteral keyliteral=literal(&s,p,constant,0);
    SpClause key=sp_normal(&s,&keyliteral,1); int id=sp_component(&s,key);
    SpLiteral guarded[]={literal(&s,p,-1,0),{s.component[id].atom,0,1}};
    int clause=sp_keep(&s,guarded,2,(SpStep){.rule=SP_INPUT},-1);
    s.component[id].value=-1; sp_requeue(&s,NULL);
    int ok=clause>=0 && s.clause[clause].stage==SP_LOCKED;
    s.component[id].value=1; sp_requeue(&s,NULL);
    ok &= s.clause[clause].stage==SP_PENDING && s.clause[clause].literal[1].negative;
    sp_clear(&s);
    return check_result(ok,"changing the SAT branch requeues the full guarded clauses without dropping assumptions");
}

/* Un modello SAT consistente non consente di ricostruire il vuoto.
 * La stessa funzione usata dopo UNSAT deve fallire senza un conflitto. */
static int untrusted_unsat(void) {
    static const int rows[][4]={{1,2,0,0},{-1,2,0,0}};
    SpState s; cnf_state(&s,rows,2,2);
    int ready=sp_avatar_update(&s);
    int ok=ready && s.sat_models==1 && s.empty<0 && !sp_avatar_replay(&s) && s.empty<0 && !s.limited;
    sp_clear(&s);
    return check_result(ok,"a claimed UNSAT result cannot prove anything when unit propagation has no conflict");
}

/* Controlla il limite congiunto dei due motori, non due quote indipendenti
 * che permetterebbero di allocare il doppio della memoria dichiarata. */
static int shared_memory(void) {
    SpState s={.empty=-1,.avatar=1,.avatar_dirty=1};
    size_t limit=saturation_memory_limit;
    s.bytes=limit-1;
    unsigned long long before=0;
    int ready=sp_sat_begin(&s,&before);
    if (ready) { sat_variable(&s.sat); sp_sat_end(&s,before); }
    int ok=s.limited && !s.resumable && s.bytes+s.sat.bytes<=limit;
    s.bytes=0; sp_clear(&s);
    return check_result(ok,"SAT allocations respect the saturation memory budget and exhaustion is not a resumable proof");
}

/* Il costo atomico di un'operazione puo non entrare nella quota locale
 * rimasta, anche senza consumarla tutta. Deve restare possibile ripartire
 * con il ciclo ordinario entro il budget globale. */
static int local_work_slice(void) {
    SpState s={.empty=-1};
    s.quota=s.work+8;
    unsigned long long before=0;
    int ready=sp_sat_begin(&s,&before);
    if (ready) { sat_spend(&s.sat,100); sp_sat_end(&s,before); }
    int ok=ready && s.limited && s.resumable && s.work<s.quota && s.empty<0;
    sp_clear(&s);
    return check_result(ok,"an atomic SAT operation that exceeds a local work slice remains resumable");
}

/* Centosessanta proposizioni sono dati del test: numero di componenti,
 * firma e vettori del SAT crescono senza una maschera a 128 bit. */
static int many_components(void) {
    SpState s={.empty=-1};
    for (int i=0;i<160;i++) {
        int symbol=sp_symbol(&s,0,1); SpLiteral l=literal(&s,symbol,0,0);
        sp_keep(&s,&l,1,(SpStep){.rule=SP_INPUT},-1);
    }
    s.initial=s.clauses; s.avatar=s.avatar_dirty=1;
    int ok=sp_avatar_update(&s) && s.components==160 && s.sat.variables==161 && s.sat_models==1;
    for (int i=0;i<s.components;i++) ok &= s.component[i].value==1;
    ok &= s.bytes+s.sat.bytes<=saturation_memory_limit;
    sp_clear(&s);
    return check_result(ok,"160 components share a checked SAT model with dynamically sized storage");
}

/* Cerca una quota che interrompa una definizione gia registrata, anziche
 * dipendere dal numero esatto di visite di una versione del motore.
 * Il certificato della ripartenza deve introdurre anche quel nome fresco
 * e mantenere corretti tutti gli indici delle clausole successive. */
static int partial_definition(const char *formula) {
    arena_used=arena_keep; failure=syntax_error=NULL; work_allocation_failed=0;
    cursor=formula; next_token(); const Sentence *root=abstract_statement(); declared_symbols=symbol_count;
    SpState s={0}; int partial=0;
    for (unsigned cut=1;cut<=256 && !partial;cut++) {
        sp_clear(&s);
        if (syntax_error || !sp_prepare(&s,root,NULL,0)) break;
        s.avatar=s.avatar_dirty=1; s.quota=s.work+cut; sp_search(&s);
        for (int i=0;i<s.components;i++) partial |= s.component[i].forward<0 ||
            (!s.component[i].key.variables && s.component[i].key.count==1 && s.component[i].reverse<0);
    }
    int ok=partial && s.limited && s.resumable;
    if (ok) {
        s.limited=s.resumable=0; s.quota=0; s.avatar=0; sat_clear(&s.sat);
        sp_requeue(&s,NULL); ok=sp_search(&s);
        if (ok) {
            sp_write(&s,root,NULL,0,0); const char *text=certificate_text,*why="incomplete certificate";
            ok=!s.limited && !certificate_broken && ck_check(&text,&why)==CK_ACCEPTED;
            if (!ok) printf("Partial definition certificate: %s\n",why);
        }
    }
    sp_clear(&s);
    return check_result(ok,"a quota inside component construction cannot invalidate the fallback certificate");
}

/* Cerca direttamente con AVATAR, o interrompe una quota e riprende senza
 * SAT. Ogni refutazione viene riscritta con tutte le definizioni prima
 * dei passi e ricontrollata dal solo ingresso pubblico del checker. */
static int problem(const char *name,const char *formula,int theorem,int branches,int fallback) {
    arena_used=arena_keep; failure=syntax_error=NULL; work_allocation_failed=0;
    cursor=formula; next_token(); const Sentence *root=abstract_statement(); declared_symbols=symbol_count;
    SpState s={0};
    int ready=!syntax_error && !failure && sp_prepare(&s,root,NULL,0);
    s.avatar=s.avatar_dirty=1;
    if (fallback) s.quota=s.work+50;
    int found=ready && sp_search(&s);
    int interrupted=0;
    if (fallback) {
        interrupted=!found && s.limited && s.resumable;
        s.limited=s.resumable=0; s.quota=0; s.avatar=0;
        sat_clear(&s.sat); sp_requeue(&s,NULL); found=sp_search(&s);
    }
    int ok=ready && (theorem ? found : !found) && (!branches || s.sat_models>=2) && (!fallback || interrupted);
    if (found) {
        sp_write(&s,root,NULL,0,0);
        const char *text=certificate_text,*why="incomplete certificate";
        ok &= !s.limited && !certificate_broken && ck_check(&text,&why)==CK_ACCEPTED;
        if (!ok) printf("Certificate details: %s\n",why);
    }
    if (!ok) printf("AVATAR details: ready=%d found=%d models=%u components=%d clauses=%d work=%llu limited=%d parse=%s\n",
                    ready,found,s.sat_models,s.components,s.clauses,s.work,s.limited,syntax_error?syntax_error:"none");
    sp_clear(&s); return check_result(ok,name);
}

/* Esegue oracoli, varianti false e prove certificate, senza affidare mai
 * l'esito positivo al solo valore restituito da sat_solve. */
int main(void) {
    engine_init();
    int passed=propositional_oracle();
    passed=variable_components() && passed;
    passed=split_work_slices() && passed;
    passed=guarded_branches() && passed;
    passed=untrusted_unsat() && passed;
    passed=shared_memory() && passed;
    passed=local_work_slice() && passed;
    passed=many_components() && passed;
    const char *equality="(a \\in S \\land b \\in S \\land c \\in S \\land d \\in S \\land "
        "(a=b \\lor a=c) \\land b=d \\land c=d) \\implies a=d";
    passed=partial_definition(equality) && passed;
    passed=problem("conditional equality conflicts exclude SAT branches and yield an independently checked proof",equality,1,1,0) && passed;
    passed=problem("a paused AVATAR attempt resumes as ordinary saturation with a valid remapped certificate",equality,1,0,1) && passed;
    passed=problem("Boolean case analysis exports component definitions and reconstructed resolution steps",
        "(P \\subseteq S \\land Q \\subseteq S \\land R \\subseteq S \\land c \\in S \\land "
        "(c \\in P \\lor c \\in Q) \\land (c \\in P \\lor c \\notin Q) \\land "
        "(c \\notin P \\lor c \\in Q) \\land (c \\notin P \\lor c \\notin Q)) \\implies c \\in R",1,0,0) && passed;
    passed=problem("the neutral-element theorem also has a checked AVATAR proof in addition to its historical instance proof",
        "(\\perp \\colon S \\times S \\to S \\land u \\in S \\land v \\in S \\land "
        "\\forall x \\in S\\ (u \\perp x=x) \\land \\forall x \\in S\\ (x \\perp v=x)) \\implies u=v",1,0,0) && passed;
    passed=problem("a nonuniform predicate still refutes the false universal disjunction",
        "(P \\subseteq S) \\implies ((\\forall x \\in S\\ (x \\in P)) \\lor (\\forall x \\in S\\ (x \\notin P)))",0,0,0) && passed;
    passed=problem("empty sorts cannot supply an existential witness through component splitting",
        "\\exists x \\in S\\ (x=x)",0,0,0) && passed;
    return passed?0:1;
}
