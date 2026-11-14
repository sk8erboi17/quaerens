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
        ok &= !certificate_broken && ck_check(&text, &why, 0)==CK_ACCEPTED;
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
        ok=!certificate_broken && ck_check(&text, &why, 0)==CK_ACCEPTED;
        if (!ok) printf("Derived name details: %s\n",why);
    }
    model_clear(&m);
    return check_result(ok,"different derived-sort trees have unambiguous names in an independently accepted certificate");
}

/* Le componenti di una coppia, d_{1} e d_{2}, non sono scritte nel testo:
 * un nome sintetico non deve prenderne il posto. Qui tutte le lettere sono
 * usate, così i nomi nuovi hanno un pedice. Il primo enunciato è vero (un
 * insieme è contenuto in sé stesso), il secondo falso (E può essere vuoto):
 * prima il certificato del vero era respinto, e quello del falso legava due
 * volte d_{1}, cioè parlava di un altro enunciato. */
static int pair_component_names(void) {
    const char *head="(f \\colon S \\times S \\to S \\land E \\subseteq S) \\implies "
        "(\\forall a, b, c, g, h, j, k, m, n, p, q, r, s, t, u, v, w, x, y, z \\in S\\ (a = a) \\land "
        "\\forall A, B, C, D, F, G, H, J, K, L, M, N, P, Q, R, T, U, V, W, X, Y, Z \\in S\\ (A = A) \\land "
        "\\forall d \\in S \\times S\\ ";
    char text[1024];
    snprintf(text,sizeof text,"%s(\\{f(d)\\} \\subseteq \\{f(d)\\}))",head);
    int before=unjustified_steps;
    Verdict v=prove(text,'n',0,0);
    int passed=check_result(v.outcome==PROVED && v.certificate && unjustified_steps==before,
                            "a true statement about a pair is proved with an accepted certificate when every letter is in use");
    snprintf(text,sizeof text,"%s(\\{f(d)\\} \\subseteq E))",head);
    v=prove(text,'n',0,0);
    /* Dopo i due quantificatori delle componenti, nessun altro le lega. */
    const char *binders="(forall d1 S (forall d2 S ";
    const char *bound=v.certificate?strstr(v.certificate,binders):NULL,*inside=bound?bound+strlen(binders):NULL;
    return check_result(v.outcome==REFUTED && inside && strstr(inside,"(f d1 d2)") && !strstr(inside,"(forall d1 ") && !strstr(inside,"(forall d2 "),
                        "no synthetic variable rebinds a pair component inside its scope in the certificate") && passed;
}

/* \exists x \in \mathcal{P}(S) non e una dichiarazione in testa: il lettore
 * torna indietro, e con l'insieme delle parti deve dimenticare anche il suo
 * simbolo di struttura. Altrimenti i modelli leggevano un insieme che non
 * c'era piu (lo vedono i sanitizer). Gli oracoli: nessuna parte e diversa
 * da se stessa, la parte vuota esiste sempre, e l'insieme vuoto ha una
 * parte ma nessun elemento. */
static int rolled_back_parts(void) {
    Verdict v=prove("\\exists x \\in \\mathcal{P}(S)\\ (x \\neq x)",'n',0,0);
    int passed=check_result(v.outcome==REFUTED && v.certificate,
                            "no part differs from itself: refuted with a certificate after the header rollback");
    v=prove("\\exists x \\in \\mathcal{P}(S)\\ (x = x)",'n',0,0);
    passed=check_result(v.outcome==PROVED && v.certificate,"some part equals itself: proved with a certificate") && passed;
    v=prove("(\\exists x \\in \\mathcal{P}(S)\\ (x = x)) \\implies \\exists y \\in S\\ (y = y)",'n',0,0);
    passed=check_result(v.outcome==REFUTED && v.certificate && strstr(v.certificate,"sort S 0\n"),
                        "the empty set has a part and no element: certified counterexample") && passed;
    char text[512];size_t used=(size_t)snprintf(text,sizeof text,"\\exists ");
    for (int i=1;i<=16;i++) used+=(size_t)snprintf(text+used,sizeof text-used,"%sS_{%d}",i==1?"":",",i);
    snprintf(text+used,sizeof text-used,"\\ \\exists x \\in \\mathcal{P}(S_{1})\\ (x = x)");
    v=prove(text,'n',0,0);
    return check_result(v.outcome==PROVED && v.certificate && sort_count==16 && strstr(v.certificate,"(existspart x S1 "),
                        "after the rollback sixteen sorts remain, and the quantifier over parts is read as such") && passed;
}

/* Il controesempio in una riga non si tronca: con 130 insiemi supera i
 * 2048 caratteri di una volta, e deve arrivare all'ultimo insieme con le
 * formule chiuse. */
static int whole_counterexample_text(void) {
    char text[8192];size_t used=(size_t)snprintf(text,sizeof text,"(");
    for (int i=1;i<=130;i++)
        used+=(size_t)snprintf(text+used,sizeof text-used,"%sc_{%d} \\in S_{%d}",i==1?"":" \\land ",i,i);
    snprintf(text+used,sizeof text-used," \\land d_{130} \\in S_{130}) \\implies c_{130}=d_{130}");
    Verdict v=prove(text,'n',0,0);
    size_t length=v.detail?strlen(v.detail):0,dollars=0;
    for (size_t i=0;i<length;i++) dollars+=v.detail[i]=='$';
    return check_result(v.outcome==REFUTED && length>2048 && strstr(v.detail,"S_{130} = ") && strstr(v.detail,"d_{130} = ") &&
                        dollars%2==0 && v.detail[length-1]=='$',
                        "the one-line counterexample with 130 sorts is complete, with every formula closed");
}

/* Un numero scritto come elemento e una variabile n_{1} hanno nel
 * certificato due nomi diversi. Prima erano entrambi "n1": sotto
 * \forall n_{1} il numero 1 della condizione diventava la variabile, e il
 * certificato accettato era di un altro enunciato. Gli oracoli: la prima
 * relazione contiene (1, 2) ma non (2, 1), la seconda entrambe. */
static int numeral_names(void) {
    Verdict v=prove("G_{1} = \\{(1, 1), (2, 2), (3, 3), (1, 2)\\} \\implies "
                    "(\\forall n_{1}, y\\ ((n_{1}, y) \\in G_{1} \\implies (y, n_{1}) \\in G_{1}))",'n',0,0);
    int passed=check_result(v.outcome==REFUTED && v.certificate && strstr(v.certificate,"sort Num numbers num1 num2 num3\n") &&
                            strstr(v.certificate,"(= n1 num1)") && !strstr(v.certificate,"(= n1 n1)"),
                            "a variable named n_1 and the numeral 1 keep distinct names in a refuted statement's certificate");
    v=prove("G_{1} = \\{(1, 1), (2, 2), (3, 3), (1, 2), (2, 1)\\} \\implies "
            "(\\forall n_{1}, y\\ ((n_{1}, y) \\in G_{1} \\implies (y, n_{1}) \\in G_{1}))",'n',0,0);
    return check_result(v.outcome==PROVED && v.certificate && strstr(v.certificate,"(= n1 num2)") && !strstr(v.certificate,"(= n1 n1)"),
                        "the same holds in the certificate of the true statement") && passed;
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
    passed=pair_component_names() && passed;
    passed=rolled_back_parts() && passed;
    passed=whole_counterexample_text() && passed;
    passed=numeral_names() && passed;
    return passed?0:1;
}
