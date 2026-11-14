/* Istanze di comprensione durante la saturazione. Le formule sono dati,
 * incluse le ipotesi del principio induttivo: non vengono inventate dal
 * motore. Le prove complete passano per il checker indipendente; le
 * varianti senza base, passo o generazione hanno modelli espliciti su {0,1}.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 * cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_comprehension_search \
 *     tests/engine/test_comprehension_search.c && /tmp/test_comprehension_search
 */
#include "../statement_checks.h"

/* Le dichiarazioni e le ipotesi restano testi del test. Nessuna stringa
 * di questa famiglia e presente nel codice della ricerca. */
#define DECLARATIONS "z \\in S \\land s \\colon S \\to S \\land f \\colon S \\to S"
#define GENERATION "(\\forall X \\subseteq S\\ ((z \\in X \\land " \
    "\\forall y \\in S\\ (y \\in X \\implies s(y) \\in X)) \\implies \\forall y \\in S\\ (y \\in X)))"
#define BASE "f(z)=z"
#define STEP "(\\forall y \\in S\\ (f(s(y))=s(f(y))))"
#define CLAIM "\\forall y \\in S\\ (f(y)=y)"
#define INDUCTION "(" DECLARATIONS " \\land " GENERATION " \\land " BASE " \\land " STEP ") \\implies " CLAIM

/* Legge e svolge l'input come il percorso pubblico, ma lascia la ricerca
 * dei modelli al suo test separato. L'albero vive nell'arena del programma. */
static const Sentence *read_problem(const char *text) {
    arena_used=arena_keep;failure=syntax_error=NULL;work_allocation_failed=0;
    cursor=text;next_token();const Sentence *root=abstract_statement();declared_symbols=symbol_count;
    if (!syntax_error && sentence_has_book_notation(root)) root=abstract_unfold(root);
    return root;
}

/* Conta soltanto le istanze tra gli antenati del vuoto. Un'istanza emessa
 * ma inutile non basta a dimostrare che il test ha esercitato la regola. */
static int used_instances(const SpState *s) {
    unsigned char *needed=calloc((size_t)s->clauses,1);int count=0;
    if (!needed) return 0;
    needed[s->empty]=1;
    for (int i=s->empty;i>=s->initial;i--) if (needed[i]) {
        SpStep step=s->clause[i].step;
        if (step.rule==SP_COMPREHEND) { count++;continue; }
        if (step.rule==SP_COMPONENT || step.rule==SP_COMPLEMENT) continue;
        needed[step.p]=1;
        if (step.rule==SP_RESOLVE || step.rule==SP_SUPER) needed[step.q]=1;
    }
    free(needed);return count;
}

/* Cerca con il solo ciclo finale, oppure con tutto il portafoglio. Il
 * certificato deve usare un'istanza effettiva e superare ck_check. Per un
 * dato falso non puo apparire il vuoto, nemmeno dopo una quota esaurita. */
static int problem(const char *name,const char *formula,int theorem,int portfolio) {
    const Sentence *root=read_problem(formula);SpState s={0};
    int ready=!failure && !syntax_error && sp_supported_formula(root) && sp_prepare(&s,root,NULL,0);
    s.comprehension=1;
    int found=ready && (portfolio?sp_portfolio(&s,root):sp_search(&s));
    int ok=ready && (theorem?found:s.empty<0);
    if (found) {
        int used=used_instances(&s);
        sp_write(&s,root,NULL,0,0);
        const char *text=certificate_text,*why="incomplete certificate";
        ok &= theorem && (!s.schemas || used>0) && !s.limited && !certificate_broken && ck_check(&text,&why)==CK_ACCEPTED;
        if (!ok) printf("Certificate details: %s; used=%d\n",why,used);
    }
    if (!ok) printf("Search details: ready=%d found=%d schemas=%d expansions=%d clauses=%d work=%llu limited=%d unsupported=%d parse=%s\n",
                    ready,found,s.schemas,s.expansions,s.clauses,s.work,s.limited,s.unsupported,syntax_error?syntax_error:"none");
    if (!ok) for (int i=0;i<s.expansions;i++) {
        proof_reset();SpClause p={.count=1,.literal=&s.expansion[i].property};sp_put_clause(&s,p);
        printf("Expansion %d, schema %d: %s\n",i,s.expansion[i].schema,proof_text);
    }
    sp_clear(&s);return check_result(ok,name);
}

/* Oracolo indipendente su {0,1}: prova tutti i sottoinsiemi nella
 * definizione di generazione e valuta direttamente base, passo e tesi. */
static int finite_countermodels(void) {
    static const int successor[3][2]={{1,0},{1,0},{0,1}};
    static const int function[3][2]={{1,0},{0,0},{0,0}};
    int all=1;
    for (int row=0;row<3;row++) {
        int generated=1,base=function[row][0]==0,step=1,claim=1;
        for (int X=0;X<4;X++) {
            int closed=1;
            for (int x=0;x<2;x++) if ((X>>x&1) && !(X>>successor[row][x]&1)) closed=0;
            if ((X&1) && closed && X!=3) generated=0;
        }
        for (int x=0;x<2;x++) {
            step &= function[row][successor[row][x]]==successor[row][function[row][x]];
            claim &= function[row][x]==x;
        }
        all &= !claim && (row==0?!base && generated && step:row==1?base && generated && !step:base && !generated && step);
    }
    return check_result(all,"two-element interpretations independently refute omission of the base, step or generation premise");
}

/* Le istanze uguali vengono condivise, mentre i corpi annidati hanno
 * contesti nuovi. Il controllo usa un dato semplice, senza cercare prove. */
static int duplicate_instances(void) {
    const Sentence *root=read_problem("(c \\in S \\land \\forall X \\subseteq S\\ (c \\in X)) \\implies c\\ne c");
    SpState s={0};int ready=!syntax_error && sp_prepare(&s,root,NULL,0);
    SpLiteral empty={-1,-1,1};
    int ok=ready && sp_comprehend(&s,0,empty);
    int clauses=s.clauses,symbols_count=s.symbols;
    ok &= sp_comprehend(&s,0,empty) && s.expansions==1 && s.clauses==clauses && s.symbols==symbols_count;
    sp_clear(&s);return check_result(ok,"repeated schema-property pairs do not duplicate clauses or technical symbols");
}

/* Una quota che interrompe la traduzione di un corpo rende il gruppo
 * incompleto e terminale: riprendere o esportare quel prefisso non puo
 * trasformarlo in un comando completo nel certificato. */
static int partial_expansion(void) {
    const Sentence *root=read_problem(INDUCTION);SpState s={0};int partial=0;
    unsigned long long limit=saturation_work_limit;
    for (unsigned cut=1;cut<=200 && !partial;cut++) {
        saturation_work_limit=limit;
        sp_clear(&s);if (!sp_prepare(&s,root,NULL,0)) break;
        saturation_work_limit=s.work+cut;sp_comprehend(&s,0,(SpLiteral){-1,-1,0});
        partial=s.expansions && !s.expansion[0].complete;
    }
    saturation_work_limit=limit;
    int ok=partial && s.limited && !s.resumable;
    if (ok) {
        s.limited=0;s.quota=0;
        ok &= !sp_comprehend(&s,0,(SpLiteral){-1,-1,0}) && s.limited && !s.resumable;
        s.limited=0;s.quota=0;s.comprehension=1;
        ok &= !sp_search(&s) && s.limited && s.empty<0;
        s.limited=0;sp_write(&s,root,NULL,0,0);ok &= s.limited;
    }
    sp_clear(&s);return check_result(ok,"an interrupted comprehension cannot be resumed or exported as a complete proof step");
}

/* Una quota locale puo terminare fra due comandi, mai dentro il corpo di
 * uno stesso comprehend. Il budget globale resta quello della ricerca. */
static int atomic_local_expansion(void) {
    const Sentence *root=read_problem(INDUCTION);SpState s={0};int finished=0;
    for (unsigned cut=1;cut<=200 && !finished;cut++) {
        sp_clear(&s);if (!sp_prepare(&s,root,NULL,0)) break;
        s.quota=s.work+cut;
        int complete=sp_comprehend(&s,0,(SpLiteral){-1,-1,0});
        finished=complete && s.expansions==1 && s.work>s.quota;
    }
    int ok=finished && s.expansion[0].complete && !s.limited;
    if (ok) ok &= !sp_visit(&s) && s.limited && s.resumable;
    sp_clear(&s);return check_result(ok,"a local strategy slice ends after a complete comprehension command and remains resumable");
}

/* Una quota fra i due membri di v0=v0 non deve far leggere term[-1]
 * come se il prefisso incompleto fosse un predicato. Il vecchio percorso
 * fallisce con ASan anche prima che inizi la ricerca delle istanze. */
static int normalization_slices(void) {
    int ok=1,stopped=0,complete=0;
    for (int length=1;length<=2;length++) for (int negative=0;negative<2;negative++) for (unsigned cut=1;cut<=50;cut++) {
        SpState s={.empty=-1,.quota=cut};SpLiteral equality[]={{-1,-1,negative},{-2,-2,negative}};
        SpClause prefix=sp_normal(&s,equality,length);
        if (s.limited) {
            stopped++;ok &= prefix.count<length && (!prefix.count || prefix.literal);
            sp_keep(&s,prefix.literal,prefix.count,(SpStep){.rule=SP_INPUT},-1);
            ok &= s.clauses==0 && s.empty<0;
        } else { complete++;ok &= prefix.count==length && prefix.literal[0].right==-1 && s.empty<0; }
        sp_clear(&s);
    }
    return check_result(ok && stopped && complete,"200 normalization slices retain a readable prefix and cannot reinterpret an unfinished equality or publish an empty clause");
}

/* Valuta una CNF booleana direttamente dal deposito, senza fare inferenze.
 * I dati di questi due oracoli usano soltanto tre predicati nullari. */
static int boolean_clauses(const SpState *s,unsigned assignment,int first) {
    for (int i=first;i<s->clauses;i++) if (s->clause[i].stage!=SP_DELETED) {
        SpClause c=s->clause[i];int some=0;
        for (int j=0;j<c.count;j++) {
            SpLiteral l=c.literal[j];
            if (l.right || l.left<=0 || s->term[l.left].symbol>=3) return 0;
            some |= (int)(assignment>>s->term[l.left].symbol&1u)!=l.negative;
        }
        if (!some) return 0;
    }
    return 1;
}

/* Cento CNF casuali si confrontano con la tabella esistenziale di q.
 * L'oracolo legge la matrice originale, non i risolventi dell'algoritmo. */
static int boolean_elimination(void) {
    unsigned random=0xc06137u;int ok=1,sat=0,unsat=0;
    for (int round=0;round<100 && ok;round++) {
        SpState s={.empty=-1};int rows[8][3],atoms[3],count=4+round%5;
        for (int i=0;i<3;i++) atoms[i]=sp_term(&s,sp_symbol(&s,0,1),NULL);
        for (int i=0;i<count;i++) {
            SpLiteral out[3];
            for (int j=0;j<3;j++) {
                random=random*1664525u+1013904223u;
                int variable=(int)(random>>8)%3,negative=random&1u;
                rows[i][j]=(negative?-1:1)*(variable+1);
                out[j]=(SpLiteral){atoms[variable],0,negative};
            }
            sp_keep(&s,out,3,(SpStep){.rule=SP_INPUT},-1);
        }
        s.initial=s.clauses;s.inputs=2;
        ok &= sp_reserve(&s,(void **)&s.expansion,&s.expansion_cap,1,sizeof *s.expansion);
        if (ok) {
            s.expansions=1;s.expansion[0]=(SpExpansion){.complete=1,.symbol_first=2,.symbol_past=3};
            sp_eliminate_comprehensions(&s);
            for (unsigned original=0;original<4 && ok;original++) {
                int exists=0;
                for (unsigned q=0;q<2;q++) {
                    unsigned assignment=original|(q<<2);int all=1;
                    for (int i=0;i<count;i++) {
                        int some=0;
                        for (int j=0;j<3;j++) {
                            int l=rows[i][j],v=abs(l)-1;
                            some |= (int)(assignment>>v&1u)!=(l<0);
                        }
                        all &= some;
                    }
                    exists |= all;
                }
                sat += exists; unsat += !exists;
                ok &= !s.limited && boolean_clauses(&s,original,0)==exists && boolean_clauses(&s,original|4u,0)==exists;
            }
        }
        sp_clear(&s);
    }
    return check_result(ok && sat && unsat,"100 Boolean eliminations preserve exactly the existential truth table of the removed technical name");
}

/* Una unita condizionata non e un fatto assoluto. Il risolvente mantiene
 * la guardia e puo sostituire il bersaglio solo se questi gia la contiene. */
static int conditional_units(void) {
    SpState s={.empty=-1,.avatar=1};
    int p=sp_term(&s,sp_symbol(&s,0,1),NULL),q=sp_term(&s,sp_symbol(&s,0,1),NULL);
    SpLiteral atom={p,0,0};SpClause key=sp_normal(&s,&atom,1);
    int component=sp_component(&s,key);s.component[component].value=1;
    SpLiteral guard={s.component[component].atom,0,1};
    SpLiteral target[]={{p,0,1},{q,0,0},guard};
    int first=sp_keep(&s,target,2,(SpStep){.rule=SP_INPUT},-1),past=s.clauses;
    int reduced=sp_unit_resolve(&s,first,s.component[component].forward);
    int ok=!reduced && s.clause[first].stage!=SP_DELETED && s.clauses>past;
    int original=first;
    for (unsigned assignment=0;assignment<8 && ok;assignment++) {
        int P=assignment&1u,Q=assignment>>1&1u,B=assignment>>2&1u;
        int premise=P==B && (!P || Q);
        if (premise) ok &= boolean_clauses(&s,assignment,original);
    }
    int guarded=sp_keep(&s,target,3,(SpStep){.rule=SP_INPUT},-1);
    ok &= sp_unit_resolve(&s,guarded,s.component[component].forward) && s.clause[guarded].stage==SP_DELETED;
    sp_clear(&s);
    return check_result(ok,"conditional unit propagation retains its guard and cannot replace an unconditional target");
}

/* Prove e falsi differiscono soltanto nelle ipotesi fornite come dati. */
int main(void) {
    engine_init();
    /* Il costo comprende anche le nuove clausificazioni e i rami SAT:
     * il test dichiara il proprio budget, senza cambiare il default CLI. */
    saturation_work_limit=20000000;
    int passed=finite_countermodels();
    passed=boolean_elimination() && passed;
    passed=conditional_units() && passed;
    passed=duplicate_instances() && passed;
    passed=partial_expansion() && passed;
    passed=atomic_local_expansion() && passed;
    passed=normalization_slices() && passed;
    passed=problem("the empty-part instance refutes a universal membership assumption",
        "(c \\in S \\land \\forall X \\subseteq S\\ (c \\in X)) \\implies c\\ne c",1,0) && passed;
    passed=problem("a nested universal part retains its outer comprehension in the certificate",
        "(P \\subseteq S \\land \\forall X \\subseteq S\\ \\forall Y \\subseteq S\\ "
        "((\\forall x \\in S\\ (x \\in X)) \\lor (\\forall x \\in S\\ (x \\in Y)))) "
        "\\implies \\forall x \\in S\\ (x \\in P)",1,1) && passed;
    passed=problem("a full-part instance proves existence without inventing a domain element",
        "(P \\subseteq S) \\implies (\\exists X \\subseteq S\\ (\\forall y \\in S\\ (y \\in X)))",1,0) && passed;
    passed=problem("the first-order closure of fixed points follows from the recursive equation",
        "(" DECLARATIONS " \\land " STEP ") \\implies \\forall y \\in S\\ (f(y)=y \\implies f(s(y))=s(y))",1,0) && passed;
    passed=problem("induction follows from the explicit generation principle, base and recursive step",INDUCTION,1,0) && passed;
    passed=problem("the portfolio preserves AVATAR names before dynamically generated comprehension instances",INDUCTION,1,1) && passed;
    passed=problem("recursive compatibility without a base does not imply identity",
        "(" DECLARATIONS " \\land " GENERATION " \\land " STEP ") \\implies " CLAIM,0,0) && passed;
    passed=problem("a base without a recursive step does not imply identity",
        "(" DECLARATIONS " \\land " GENERATION " \\land " BASE ") \\implies " CLAIM,0,0) && passed;
    passed=problem("base and recursive compatibility on an arbitrary set do not imply identity",
        "(" DECLARATIONS " \\land " BASE " \\land " STEP ") \\implies " CLAIM,0,0) && passed;
    passed=problem("a tautological closure premise cannot prove an arbitrary predicate",
        "(P \\subseteq S \\land c \\in S \\land \\forall X \\subseteq S\\ "
        "(\\forall x \\in S\\ (x \\in X \\implies x \\in X))) \\implies c \\in P",0,0) && passed;
    return passed?0:1;
}
