/* Insiemi dinamici: le dichiarazioni oltrepassano sei tipi, i bit di una
 * parola e i vecchi intervalli dei nomi interni. Gli oracoli sono due
 * costanti distinte, tautologie e parti dell'insieme vuoto; ogni esito
 * pubblico e ricontrollato dal checker ordinario.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 * cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_sort_storage \
 *     tests/engine/test_sort_storage.c && /tmp/test_sort_storage
 */
#include "../statement_checks.h"

/* Una costante in ogni insieme forza cardinalita almeno uno. Per
 * smentire c_N=d_N servono esattamente due elementi nell'ultimo insieme. */
static int many_constants(void) {
    char text[32768];size_t used=(size_t)snprintf(text,sizeof text,"(");
    for (int i=1;i<=160;i++)
        used+=(size_t)snprintf(text+used,sizeof text-used,"%sc_{%d} \\in S_{%d}",i==1?"":" \\land ",i,i);
    used+=(size_t)snprintf(text+used,sizeof text-used," \\land d_{160} \\in S_{160}");
    snprintf(text+used,sizeof text-used,") \\implies c_{160}=d_{160}");
    Verdict v=prove(text,'n',0,0);
    int passed=check_result(v.outcome==REFUTED && v.certificate && sort_count==160 &&
                             strstr(v.certificate,"sort S160 2\n"),
                             "160 nonempty sorts retain distinct types and a certified two-element countermodel in the last sort");
    snprintf(text+used,sizeof text-used," \\land c_{160}=d_{160}) \\implies c_{160}=d_{160}");
    v=prove(text,'n',0,0);
    passed=check_result(v.outcome==PROVED && v.certificate && sort_count==160 &&
                         strstr(v.certificate,"certificate saturation\n"),
                         "a tautology with 160 sorts is certified through saturation beyond the legacy case-mask width") && passed;
    snprintf(text+used,sizeof text-used,") \\implies S_{129} \\sim S_{160}");
    v=prove(text,'n',0,0);
    return check_result(v.outcome==REFUTED && v.certificate && sort_count==160,
                         "a public equipotence counterexample preserves sort identifiers beyond four-bit packing") && passed;
}

/* Gli esistenziali introducono i tipi come dati. Ogni condizione di non
 * vuotezza deve restare presente, anche oltre 128 indici e nove pedici. */
static int many_nonempty_sorts(void) {
    char text[32768];size_t used=(size_t)snprintf(text,sizeof text,"\\exists ");
    for (int i=1;i<=512;i++) used+=(size_t)snprintf(text+used,sizeof text-used,"%sS_{%d}",i==1?"":",",i);
    used+=(size_t)snprintf(text+used,sizeof text-used," \\neq \\emptyset\\ ");
    snprintf(text+used,sizeof text-used,"(\\forall x \\in S_{512}\\ (x=x))");
    Verdict v=prove(text,'n',0,0);
    int passed=check_result(v.outcome==PROVED && v.certificate && sort_count==512 && slot_count>=513,
                            "512 existential sorts preserve every nonemptiness witness and produce a certified example");
    int large=0;
    for (int i=0;i<slot_count;i++) large |= slot_data[i].index>9;
    passed=check_result(large,"synthetic witness names grow beyond the old single-digit pool") && passed;
    snprintf(text+used,sizeof text-used,"(\\forall x \\in S_{512}\\ (x\\neq x))");
    unsigned long long saved=model_work_limit;model_work_limit=20000;
    v=prove(text,'n',0,0);model_work_limit=saved;
    return check_result(v.outcome==NOT_PROVED && !v.certificate,
                         "an impossible example cannot discard the nonemptiness of its 512th sort") && passed;
}

/* Tutte le lettere sono dati del testo, con alcuni nomi gia indicizzati.
 * L'oracolo controlla direttamente catture e ripetizioni dei nomi nuovi. */
static int fresh_name_hygiene(void) {
    const char *text="a b c d f g h j k m n p q r s t u v w x y z A B C D F G H J K L M N P Q R S T U V W X Y Z a_{1} b_{12} x_{2147483647}";
    failure=syntax_error=NULL;work_allocation_failed=0;unfolding_reset(text);
    FreshName seen[1000];int ok=!failure && !syntax_error;
    for (int i=0;i<1000 && ok;i++) {
        seen[i].letter=fresh_name(&seen[i].index);
        ok=seen[i].index>0 && !(seen[i].letter=='a' && seen[i].index==1) &&
           !(seen[i].letter=='b' && seen[i].index==12) &&
           !(seen[i].letter=='x' && seen[i].index==2147483647);
        for (int j=0;j<i && ok;j++) ok=seen[i].letter!=seen[j].letter || seen[i].index!=seen[j].index;
    }
    return check_result(ok && !syntax_error,"1000 generated names remain fresh when every available letter and selected numeric subscripts occur in the source");
}

/* Ogni coppia (a,b) con a+b<=total determina una composizione in tre
 * coordinate: la terza e total-a-b. Le coppie sono (t+1)(t+2)/2. */
static int compositions(void) {
    sort_count=3;symbol_count=0;
    for (int i=0;i<3;i++) sort_data[i].kind=SORT_PLAIN;
    Model m={0};int ok=model_dimensions(&m);
    for (size_t total=0;total<=8 && ok;total++) {
        unsigned char seen[9][9]={{0}};size_t count=0;
        model_composition_start(&m,total);
        do {
            size_t a=m.dimension[0].extra,b=m.dimension[1].extra,c=m.dimension[2].extra;
            ok=a<=total && b<=total-a && c==total-a-b && !seen[a][b];
            if (!ok) break;
            seen[a][b]=1;count++;
        } while (model_composition_next(&m));
        ok &= count==(total+1)*(total+2)/2;
    }
    model_clear(&m);
    return check_result(ok,"iterative dimension enumeration visits every small composition once without a power-coded counter");
}

/* Mille costruzioni indipendenti attraverserebbero NUMERAL_NAMES con il
 * vecchio codice STRUCTURE_NAMES+i. Parti dell'insieme vuoto hanno un solo
 * elemento; il numero scritto ha anch'esso una tabella di una cella. */
static int structural_names(void) {
    arena_used=arena_keep;failure=syntax_error=NULL;work_allocation_failed=0;
    sort_count=symbol_count=definition_count=visible_count=slot_count=0;
    int ok=1;
    for (int i=1;i<=1002 && ok;i++) {
        int base=sort_index_sub('S',i),part=sort_derived(SORT_PARTS,base,-1);
        int symbol=sort_data[part].symbol;
        ok=!syntax_error && symbol>=0 && symbols[symbol].name<0 && symbol_structure(symbol)==part;
    }
    int number=numeral_symbol(7);
    ok &= !syntax_error && number>=0 && symbols[number].name==NUMERAL_NAMES+7 && sort_count==2005;
    Model m={0};ok &= model_dimensions(&m) && model_derived_sizes(&m) && model_allocate(&m);
    if (ok) {
        model_structure(&m);
        ok=m.table[number][0]==0;
        Sentence truth={.kind=S_TRUE};declared_symbols=symbol_count;
        cert_abstract_head("example",&truth,&m);cert_add("end\n");
        const char *text=certificate_text,*why="";
        ok &= !certificate_broken && ck_check(&text,&why)==CK_ACCEPTED;
        if (!ok) printf("Structural certificate details: %s\n",why);
    }
    model_clear(&m);
    return check_result(ok,"2005 sorts keep structural symbols separate from numerals and describe independently checked powersets");
}

/* Due alberi diversi avevano lo stesso nome concatenato MPPS:
 * mappe da parti(P) a S e mappe da P a parti(S). Le definizioni restano
 * distinte anche nel certificato, oltre che nei dati del motore. */
static int derived_certificate_names(void) {
    sort_count=symbol_count=definition_count=visible_count=slot_count=0;
    failure=syntax_error=NULL;work_allocation_failed=0;arena_used=arena_keep;
    int p=sort_index('P'),s=sort_index('S');
    int pp=sort_derived(SORT_PARTS,p,-1),ps=sort_derived(SORT_PARTS,s,-1);
    int first=sort_derived(SORT_MAPS,pp,s),second=sort_derived(SORT_MAPS,p,ps);
    Model m={0};int ok=first!=second && model_dimensions(&m) && model_derived_sizes(&m) && model_allocate(&m);
    if (ok) {
        model_structure(&m);declared_symbols=symbol_count;
        Sentence truth={.kind=S_TRUE};
        cert_abstract_head("example",&truth,&m);cert_add("end\n");
        const char *text=certificate_text,*why="";
        ok=!certificate_broken && ck_check(&text,&why)==CK_ACCEPTED;
        if (!ok) printf("Derived name details: %s\n",why);
    }
    model_clear(&m);
    return check_result(ok,"different derived-sort trees have unambiguous names in an independently accepted certificate");
}

/* I casi sono dati del test; il motore non conosce numeri o nomi di questa
 * famiglia. Le due varianti false non possono diventare dimostrate. */
int main(void) {
    engine_init();int passed=many_constants();
    passed=many_nonempty_sorts() && passed;
    passed=fresh_name_hygiene() && passed;
    passed=compositions() && passed;
    passed=structural_names() && passed;
    passed=derived_certificate_names() && passed;
    return passed?0:1;
}
