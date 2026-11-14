/* Dichiarazioni dinamiche nel parser, nei modelli e nella ricerca.
 * I dati oltrepassano sia il vecchio tetto del motore sia quello del
 * checker; i falsi sono smentiti anche dall'interpretazione con parti vuote.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 * cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_symbol_declarations \
 *     tests/engine/test_symbol_declarations.c && /tmp/test_symbol_declarations
 */
#include "../statement_checks.h"

/* La lettura ricorsiva dell'argomento di f introduce il sessantacinquesimo
 * simbolo mentre l'applicazione usa ancora il suo dominio e codominio.
 * La comprensione e vuota, perche nessun elemento appartiene a tutte le
 * parti: la parte vuota lo esclude. L'immagine e quindi vuota per ogni f. */
static int recursive_growth(void) {
    char text[8192];size_t used=(size_t)snprintf(text,sizeof text,"(f \\colon S \\to T");
    for (int i=0;i<63;i++) used+=(size_t)snprintf(text+used,sizeof text-used," \\land %c_{%d} \\subseteq S","ABCDEFG"[i/9],i%9+1);
    snprintf(text+used,sizeof text-used,") \\implies f(\\{x \\in S \\mid \\forall X \\subseteq S\\ (x \\in X)\\})=\\emptyset");
    arena_used=arena_keep;failure=syntax_error=NULL;work_allocation_failed=0;
    cursor=text;next_token();const Sentence *root=abstract_statement();declared_symbols=symbol_count;
    int f=symbol_find('f',0),ok=!failure && !syntax_error && symbol_count==65 && f>=0 && symbols[f].domain[0]==0 && symbols[f].range==1;
    const Sentence *plain=ok?abstract_unfold(root):root;
    for (size_t n=0;n<=2 && ok;n++) {
        Model m={0};
        if (!model_dimensions(&m)) return check_result(0,"recursive model coordinates can be allocated");
        m.dimension[0].size=m.dimension[1].size=n;
        ok=model_allocate(&m);
        if (ok) {
            size_t *values=model_variables();model_steps=0;
            ok=values && book_holds(&m,root,values) && model_holds(&m,plain,values);
        }
        model_clear(&m);
    }
    if (!ok) printf("Recursive parse details: %s\n",syntax_error?syntax_error:"no syntax error");
    return check_result(ok,"recursive parsing can grow declarations while preserving a map's distinct domain and codomain");
}

/* Scrive count parti con nomi distinti, usando soltanto la grammatica
 * preesistente dei pedici a una cifra. La tesi usa l'ultima dichiarazione. */
static void formula_with_parts(char *text,size_t size,int count,int theorem) {
    const char *letters="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    size_t used=(size_t)snprintf(text,size,"(c \\in S");
    for (int i=0;i<count;i++)
        used+=(size_t)snprintf(text+used,size-used," \\land %c_{%d} \\subseteq S",letters[i/9],i%9+1);
    char letter=letters[(count-1)/9];int index=(count-1)%9+1;
    if (theorem) used+=(size_t)snprintf(text+used,size-used," \\land c \\in %c_{%d}",letter,index);
    snprintf(text+used,size-used,") \\implies c \\in %c_{%d}",letter,index);
}

/* Legge il dato come il programma pubblico e conserva tutti i simboli,
 * compresa la costante. Il modello SAT e ricalcolato dal motore e dal
 * checker del certificato pubblico; la parte vuota e un oracolo esplicito. */
static int many_parts(void) {
    char text[32768];formula_with_parts(text,sizeof text,320,0);
    arena_used=arena_keep;failure=syntax_error=NULL;work_allocation_failed=0;
    cursor=text;next_token();const Sentence *root=abstract_statement();declared_symbols=symbol_count;
    int ok=!failure && !syntax_error && symbol_count==321;
    Model m={0};unsigned long long work=0;
    if (!model_dimensions(&m)) return check_result(0,"SAT model coordinates can be allocated");
    m.dimension[0].size=2;
    ok &= model_sat(root,0,&m,&work);
    int last=320,c=symbol_find('c',0);
    ok &= m.symbols==321 && m.table && m.part && c>=0 && m.table[last][m.table[c][0]]==0;
    model_clear(&m);
    ok &= !m.table && !m.part && !m.symbols && !m.bytes;
    int passed=check_result(ok,"320 declared parts retain distinct SAT tables beyond both historical symbol limits");
    Verdict v=prove(text,'n',0,0);
    passed=check_result(v.outcome==REFUTED && v.certificate && symbol_count==321,
                        "the public solver exports an independently accepted countermodel with 321 declared symbols") && passed;
    char item[33000];snprintf(item,sizeof item,"\\[%s\\]",text);
    v=prove_item(item,item+strlen(item));
    passed=check_result(strlen(text)>MAX_FORMULA && v.outcome==REFUTED && v.certificate,
                        "a complete TeX item beyond 4096 characters reaches the solver and retains its last declaration") && passed;
    FILE *stream=tmpfile();int line_ok=stream!=NULL;
    if (stream) {
        fputs(text,stream);fputs("\nc \\in S \\implies c=c\n",stream);rewind(stream);
        char *line=read_input_line(stream),*next=read_input_line(stream);
        line_ok=line && next && strcmp(line,text)==0 && strcmp(next,"c \\in S \\implies c=c")==0;
        free(line);free(next);fclose(stream);
    }
    passed=check_result(line_ok,"stream input reads the entire long formula and leaves the following line intact") && passed;
    formula_with_parts(text,sizeof text,320,1);
    v=prove(text,'n',0,0);
    return check_result(v.outcome==PROVED && v.certificate && declared_symbols==321,
                         "a proof using the last of 320 parts is independently certified without truncating the declaration prefix") && passed;
}

/* I modelli possiedono il numero delle loro tabelle. Cambiare enunciato
 * non deve modificare quanti puntatori il modello precedente libera. */
static int model_ownership(void) {
    Model m={0};
    if (!model_dimensions(&m)) return check_result(0,"owned model coordinates can be allocated");
    m.dimension[0].size=1;
    int ok=model_allocate(&m) && m.symbols==(size_t)symbol_count;
    int saved=symbol_count;symbol_count=1;model_clear(&m);symbol_count=saved;
    size_t limit=model_memory_limit;model_memory_limit=1;
    ok &= !model_allocate(&m) && m.limited && !m.table && !m.part && !m.symbols;
    model_memory_limit=limit;
    ok &= model_dimensions(&m);
    if (ok) m.dimension[0].size=1;
    ok &= model_allocate(&m) && !m.limited;
    model_clear(&m);
    return check_result(ok,"model pointer tables own their lengths and memory exhaustion cannot leave a partial model");
}

/* I vecchi codici 99 e 100 non possono riservare numeri quando i nomi
 * generati superano quel prefisso. Zero esplicito e pedice letterale hanno
 * codici negativi distinti, mentre i numeri positivi restano numeri. */
static int generated_subscripts(void) {
    symbol_count=0;declared_symbols=1;names_given=0;
    int zero=symbol_append((Symbol){'P',ZERO_INDEX,1,{0,0},TRUTH,0,0});
    int part=-1,map=-1,domain[]={0},ok=zero==0;
    for (int i=0;i<220 && ok;i++) {
        part=part_symbol_new('P',0,0);
        ok &= part>=0 && symbols[part].index==i && symbol_find('P',ZERO_INDEX)==zero;
        if (i==99 || i==100) {
            char expected[32];snprintf(expected,sizeof expected,"P_{%d}",i);
            proof_reset();put_symbol(part);ok &= strcmp(proof_text,expected)==0;
        }
    }
    for (int i=1;i<=220 && ok;i++) {
        map=map_symbol_new("varphi",1,domain,0);
        ok &= map>=0 && symbols[map].index==i;
    }
    proof_reset();put_symbol(zero);ok &= strcmp(proof_text,"P_{0}")==0;
    if (map>=0) { proof_reset();put_symbol(map);ok &= strcmp(proof_text,"\\varphi_{220}")==0; }
    return check_result(ok,"440 generated names cross 99 and 100 without aliasing explicit zero or a literal subscript");
}

/* I simboli originali possono avere indici maggiori di SHRT_MAX anche
 * quando il vecchio deposito dei termini contiene pochissimi elementi.
 * Qui si verificano separatamente termine, rinomina e copia dei nomi. */
static int wide_symbol_indices(void) {
    symbol_count=0;int ok=symbol_append((Symbol){'c',0,0,{0,0},0,0,0})==0;
    int high=SHRT_MAX+1;
    for (int i=1;i<=high+1 && ok;i++) ok=symbol_append((Symbol){'P',i,1,{0,0},TRUTH,0,0})==i;
    now=&branch[0];now->grounds=now->knowns=now->clock=0;
    memset(exact,-1,sizeof exact);
    ground_lookup(-1,-1,-1,G_CREATE);
    int c=ground_lookup(0,-1,-1,G_CREATE),term=ground_lookup(high,c,-1,G_CREATE);
    int variable=slot_new('x',0,0);
    Sentence *body=sentence_new(S_HOLDS,NULL,NULL,"");body->left=elem_new(high,0,slot_elem(variable),NULL);
    int at=known_add(body,0,SLOT_VALUES_EMPTY,K_GIVEN,-1),replacement=high+1;
    ok &= term>=0 && ground[term].symbol==high && known_parts(at,&high,1) && now->known[at].part[0]==high;
    const Sentence *renamed=sentence_rename(body,high,replacement);
    ok &= renamed->left->symbol==replacement && sentence_renamed(renamed,body,&high,&replacement,1);
    return check_result(ok,"symbol indices beyond 16 bits survive ground storage, immutable substitutions and structural renaming");
}

/* Gli oracoli sono dati esterni al motore e i risultati pubblici passano
 * per il checker ordinario, senza accedere alle sue strutture interne. */
int main(void) {
    engine_init();int passed=recursive_growth();
    passed=many_parts() && passed;
    passed=model_ownership() && passed;
    passed=generated_subscripts() && passed;
    passed=wide_symbol_indices() && passed;
    return passed?0:1;
}
