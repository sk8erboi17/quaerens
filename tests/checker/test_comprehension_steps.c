/* Comprensioni nei certificati di saturazione, senza collegare il motore.
 * L'oracolo interpreta direttamente gli enunciati su tutte le parti di
 * {0,1}, poi cerca esaustivamente un'estensione delle tavole ausiliarie
 * che soddisfi le clausole. Un controesempio originale deve sopravvivere
 * a ogni istanza valida; una comprensione non puo inventare una prova.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 * cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_comprehension_steps \
 *     tests/checker/test_comprehension_steps.c && /tmp/test_comprehension_steps
 */
#include <stdio.h>
#include <string.h>
#define main checker_main
#include "../../checker.c"
#undef main

/* Un'interpretazione indipendente sull'universo {0,1}: domain e la
 * maschera di S, constant il valore di c quando dichiarato, Q/R tavole
 * unarie di due bit. auxiliary contiene tutte le tavole dei simboli
 * tecnici; shift, preso in prestito, ne delimita i bit per simbolo. */
typedef struct {
    unsigned domain, constant, Q, R, auxiliary;
    int has_constant;
    const unsigned *shift;
} Interpretation;

/* Registra un risultato e una descrizione inglese. */
static int check(int ok,const char *name) {
    printf("%s: %s%s%s\n",ok?"PASS":"FAIL",name,ok?"":": ",ok?"":ck_reason);
    return ok;
}

/* Prepara solo le dichiarazioni c:S (opzionale), Q,R:S->bool, poi legge
 * l'enunciato come dato. Il risultato appartiene all'arena del checker. */
static const CkNode *prepare(const char *text,int constant) {
    ck_r_clear(); ck_structure_clear(); ck_used=ck_depth=ck_nodes=0;
    ck_status=CK_ACCEPTED; ck_reason="";
    ck_sort_count=1; ck_symbol_count=ck_declared=2+constant;
    if (!ck_symbols_reserve((size_t)ck_symbol_count)) return NULL;
    memset(ck_sorts,0,sizeof ck_sorts); memset(ck_symbols,0,(size_t)ck_symbol_count*sizeof *ck_symbols);
    ck_sorts[0].name="S";
    if (constant) { ck_symbols[0].name="c"; ck_symbols[0].result=0; }
    for (int i=0;i<2;i++) {
        CkSymbol *s=&ck_symbols[constant+i];
        s->name=i?"R":"Q"; s->arity=1; s->arg[0]=0; s->result=CK_BOOL;
    }
    ck_cursor=text; const CkNode *root=ck_read_condition();
    if (ck_status==CK_ACCEPTED) ck_r_prepare(root,NULL,0);
    return root;
}

/* Valuta un termine dell'enunciato, senza usare la traduzione in clausole.
 * env contiene elementi o maschere di parti, nell'ordine dei quantificatori. */
static unsigned source_term(const CkNode *n,const Interpretation *m,const unsigned *env,int depth) {
    if (n->kind==CK_VAR) return env[depth-1-(int)n->value];
    if (n->kind==CK_PARTVAR) return env[depth-1-(int)n->value] >> source_term(n->a,m,env,depth) & 1u;
    if (m->has_constant && !n->value) return m->constant;
    unsigned table=n->value==m->has_constant?m->Q:m->R;
    return table >> source_term(n->a,m,env,depth) & 1u;
}

/* Definizione diretta dei connettivi e dei quantificatori, inclusi tutti
 * i sottoinsiemi del dominio e il dominio vuoto. env ha spazio per i
 * dodici quantificatori al massimo dei dati di questo test. */
static int source_truth(const CkNode *n,const Interpretation *m,unsigned *env,int depth) {
    switch (n->kind) {
    case CK_TRUE:return 1;
    case CK_EQ:return n->b?source_term(n->a,m,env,depth)==source_term(n->b,m,env,depth):!!source_term(n->a,m,env,depth);
    case CK_NOT:return !source_truth(n->a,m,env,depth);
    case CK_AND:return source_truth(n->a,m,env,depth) && source_truth(n->b,m,env,depth);
    case CK_OR:return source_truth(n->a,m,env,depth) || source_truth(n->b,m,env,depth);
    case CK_IMPLIES:return !source_truth(n->a,m,env,depth) || source_truth(n->b,m,env,depth);
    case CK_IFF:return source_truth(n->a,m,env,depth)==source_truth(n->b,m,env,depth);
    default:break;
    }
    int every=n->kind==CK_FORALL || n->kind==CK_FORALLPART;
    int parts=n->kind==CK_FORALLPART || n->kind==CK_EXISTSPART;
    for (unsigned v=0;v<(parts?4u:2u);v++) {
        if (parts?(v & ~m->domain):!(m->domain>>v&1u)) continue;
        env[depth]=v;
        if (source_truth(n->a,m,env,depth+1)!=every) return !every;
    }
    return every;
}

/* Un termine delle clausole: le variabili ricevono i bit di vars. Tutte
 * le funzioni ausiliarie sono provate con ogni tavola a valori in {0,1}. */
static unsigned clause_term(int t,const Interpretation *m,unsigned vars) {
    if (t<0) return vars>>(-t-1)&1u;
    CkRTerm n=ck_r.term[t]; unsigned at=0;
    for (int i=0;i<n.count;i++) at |= clause_term(ck_r.argument[n.first+i],m,vars)<<i;
    if (n.symbol<ck_declared) {
        if (m->has_constant && !n.symbol) return m->constant;
        return (n.symbol==m->has_constant?m->Q:m->R)>>at&1u;
    }
    if (n.symbol==ck_declared) return m->domain>>at&1u;
    return m->auxiliary>>(m->shift[n.symbol]+at)&1u;
}

/* Cerca un'estensione delle tavole, senza inferenze ne unificazione.
 * I dati mantengono il totale sotto 22 bit e le clausole sotto 12 variabili;
 * superarli e un errore del test, non un caso saltato. */
static int extension(Interpretation *m,unsigned bits) {
    if (bits>22) return 0;
    for (unsigned code=0;code<(1u<<bits);code++) {
        m->auxiliary=code; int all=1;
        for (int j=0;j<ck_r.clauses && all;j++) {
            CkRClause c=ck_r.clause[j];
            if (c.variables>12) return 0;
            for (unsigned vars=0;vars<(1u<<c.variables) && all;vars++) {
                int some=0;
                for (int i=0;i<c.count;i++) {
                    CkRLiteral l=c.literal[i];
                    int value=!l.left?1:l.right?clause_term(l.left,m,vars)==clause_term(l.right,m,vars):!!clause_term(l.left,m,vars);
                    some |= value!=l.negative;
                }
                all=some;
            }
        }
        if (all) return 1;
    }
    return 0;
}

/* Un dato semantico: statement e l'enunciato originale, first/second le
 * istanze richieste. constant dichiara c; tutti i testi restano statici. */
typedef struct { const char *name,*statement,*first,*second; int constant; } SemanticCase;

/* Ogni controesempio finito dell'enunciato deve estendersi alle clausole
 * prima e dopo le comprensioni, compresi contesti e parti annidati. */
static int semantic_cases(void) {
    static const SemanticCase cases[]={
        {"a universal part admits the full-domain comprehension",
         "(implies (forallpart X S (or (X c) (Q c))) (R c))","0 (clause (eq (v 0) (v 0)))",NULL,1},
        {"a universal part admits the empty comprehension without assuming the property",
         "(implies (forallpart X S (or (X c) (Q c))) (R c))","0 (clause (ne (v 0) (v 0)))",NULL,1},
        {"first-order parameters are preserved outside a part binder",
         "(implies (forall y S (forallpart X S (implies (X y) (Q y)))) (forall z S (R z)))",
         "0 (clause (pos (f 1 (v 0))))",NULL,1},
        {"nested part binders retain the outer comprehension",
         "(implies (forallpart X S (forallpart Y S (or (or (X c) (Y c)) (Q c)))) (R c))",
         "0 (clause (ne (v 0) (v 0)))","1 (clause (eq (v 0) (f 0)))",1},
        {"the empty sort remains an original countermodel after comprehension",
         "(implies (forallpart X S (forall y S (X y))) (exists y S (= y y)))",
         "0 (clause (ne (v 0) (v 0)))",NULL,0},
        {"a negated existential part is universally instantiated with the correct polarity",
         "(existspart X S (forall y S (and (X y) (Q y))))","0 (clause (eq (v 0) (v 0)))",NULL,0},
        {"a tautological closure premise cannot imply an arbitrary property",
         "(implies (forallpart X S (forall y S (implies (X y) (X y)))) (Q c))",
         "0 (clause (ne (v 0) (v 0)))",NULL,1},
    };
    int passed=1;
    for (int forward=0;forward<2;forward++) for (size_t k=0;k<sizeof cases/sizeof *cases;k++) {
        const SemanticCase *c=&cases[k]; const CkNode *root=prepare(c->statement,c->constant);
        if (forward) ck_r_prepare_mode(root,NULL,0,1);
        ck_cursor=c->first; ck_r_comprehend();
        if (c->second && ck_status==CK_ACCEPTED) { ck_cursor=c->second; ck_r_comprehend(); }
        int ok=ck_status==CK_ACCEPTED, models=0;
        unsigned *shift=calloc((size_t)ck_r.symbols,sizeof *shift),bits=0;
        if (!shift) ok=0;
        if (ok) for (int i=ck_declared+1;i<ck_r.symbols;i++) {
            if (ck_r.symbol[i].arity>4) { ok=0;break; }
            shift[i]=bits; bits+=1u<<ck_r.symbol[i].arity;
        }
        for (unsigned domain=0;domain<4 && ok;domain++) for (unsigned a=0;a<(c->constant?2u:1u) && ok;a++) {
            if (c->constant && !(domain>>a&1u)) continue;
            for (unsigned Q=0;Q<4 && ok;Q++) for (unsigned R=0;R<4 && ok;R++) {
                Interpretation m={domain,a,Q,R,0,c->constant,shift}; unsigned env[12];
                if (!source_truth(root,&m,env,0)) { models++;ok=extension(&m,bits); }
            }
        }
        if (!ok) printf("Semantic details: case=%zu models=%d auxiliary_bits=%u\n",k,models,bits);
        if (forward) printf("Forward CNF: ");
        passed=check(ok && models,c->name) && passed; free(shift);
    }
    return passed;
}

/* Una prova completa usa la parte vuota contro forall X X(c). Un'istanza
 * non produce invece alcuna prova di un esistenziale falso o del falso
 * principio che i passi ricorsivi da soli sostituiscano il caso base. */
static int certificates(void) {
    static const char *proof="certificate saturation\nsort S\nsymbol c S\n"
        "statement (implies (forallpart X S (X c)) (not true))\nclausify\n"
        "comprehend 0 (clause (ne (v 0) (v 0)))\n"
        "step resolve 1 0 5 0 (subst) (clause (pos (f 3)))\n"
        "step resolve 6 0 3 0 (subst) (clause (ne (f 0) (f 0)))\n"
        "step reflex 7 0 (subst) (clause)\nend\n";
    const char *text=proof,*why; int passed=check(ck_check(&text,&why)==CK_ACCEPTED,"a complete refutation using an explicit empty-part comprehension is accepted");
    text="certificate saturation\nsort S\nsymbol c S\nsymbol Q S bool\n"
        "statement (implies (forallpart X S (forall y S (implies (X y) (X y)))) (Q c))\n"
        "clausify\ncomprehend 0 (clause (ne (v 0) (v 0)))\nend\n";
    passed=check(ck_check(&text,&why)==CK_REJECTED,"instantiating a tautological premise does not produce a proof of a false conclusion") && passed;
    static const char *const invalid[]={
        "0 (clause (pos (f 1 (v 0))) (neg (f 2 (v 0))))",
        "0 (clause (eq (v 0) (v 1)))",
        "0 (clause (pos (f 1 (v 7))))",
        "2 (clause (eq (v 0) (v 0)))",
    };
    for (size_t i=0;i<sizeof invalid/sizeof *invalid;i++) {
        prepare("(implies (forallpart X S (or (X c) (Q c))) (R c))",1);
        ck_cursor=invalid[i]; ck_r_comprehend();
        passed=check(ck_status==CK_REJECTED,"an altered comprehension index, literal count or free-variable context is rejected") && passed;
    }
    prepare("(implies (existspart X S (forall y S (X y))) (Q c))",1);
    ck_cursor="0 (clause (ne (v 0) (v 0)))";ck_r_comprehend();
    passed=check(ck_status==CK_REJECTED,"an existential part cannot be replaced by an arbitrary universal instance") && passed;
    prepare("(forallpart X S (forall y S (X y)))",0);
    ck_cursor="0 (clause (ne (v 0) (v 0)))";ck_r_comprehend();
    passed=check(ck_status==CK_REJECTED,"a negated universal part cannot use the universal-instantiation rule") && passed;
    prepare("(implies (forallpart X S (or (X c) (Q c))) (R c))",1);
    ck_cursor="(clause (pos (f 1 (v 0))))";ck_r_define_component();
    int first=ck_r.component[0].symbol,between=ck_r.symbols;
    ck_cursor="0 (clause (eq (v 0) (v 0)))";ck_r_comprehend();
    ck_cursor="(clause (pos (f 2 (v 0))))";ck_r_define_component();
    int second=ck_r.component[1].symbol;
    int tags=ck_status==CK_ACCEPTED && ck_r.components==2 && second>between && ck_r.symbol[first].component==1 && ck_r.symbol[second].component==2;
    for (int i=between;i<ck_r.component[1].symbol;i++) tags &= ck_r.symbol[i].component==0;
    passed=check(tags,"AVATAR symbols keep their roles across interleaved comprehension definitions") && passed;
    if (!tags) return 0;
    /* Un simbolo tecnico fra due guardie deve essere astratto, mentre
     * entrambe le guardie originali devono sopravvivere alla partizione. */
    char command[256];
    snprintf(command,sizeof command,"(clause (pos (f %d)) (pos (f %d)) (neg (f %d)))",between,first,second);
    ck_cursor=command;int parent=ck_r.clauses;ck_r_add(ck_r_read_clause());
    snprintf(command,sizeof command,"(clause (pos (f %d)))",between);
    ck_cursor=command;ck_r_define_component();
    int third=ck_r.component[2].symbol;
    snprintf(command,sizeof command,"split %d (groups (2 0 0)) (clause (pos (f %d)) (pos (f %d)) (neg (f %d)))",parent,third,first,second);
    ck_cursor=command;ck_r_step();
    passed=check(ck_status==CK_ACCEPTED,"a technical symbol between AVATAR names remains ordinary and both guards survive the split") && passed;
    snprintf(command,sizeof command,"split %d (groups (2 0 0)) (clause (pos (f %d)) (pos (f %d)))",parent,third,first);
    ck_cursor=command;ck_r_step();
    passed=check(ck_status==CK_REJECTED,"a split cannot drop the guard introduced after a comprehension") && passed;
    prepare("(implies (forallpart X S (Q c)) (R c))",1);
    ck_used=CK_ARENA_BYTES;ck_cursor="0 (clause (eq (v 0) (v 0)))";ck_r_comprehend();
    passed=check(ck_status==CK_UNSUPPORTED,"an allocation failure while reading a comprehension is not accepted") && passed;
    text=proof;
    passed=check(ck_check(&text,&why)==CK_ACCEPTED,"a complete proof succeeds after a failed comprehension has been cleared") && passed;
    return passed;
}

/* Esegue gli oracoli e i certificati, poi libera anche i contesti nuovi. */
int main(void) {
    int passed=semantic_cases();
    passed=certificates() && passed;
    ck_r_clear();ck_structure_clear();
    return passed?0:1;
}
