/* Il checker legge insiemi dinamici indipendentemente dal motore. I
 * modelli espliciti hanno oracoli di cardinalita e tabelle elementari;
 * prodotti con indici oltre quattro bit non possono cambiare significato.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 * cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_sort_declarations \
 *     tests/checker/test_sort_declarations.c && /tmp/test_sort_declarations
 */
#include <stdio.h>
#include <string.h>
#define main checker_main
#include "../../checker.c"
#undef main

/* Controlla soltanto attraverso l'ingresso indipendente del checker. */
static int expect(const char *name,const char *text,CkAnswer wanted) {
    const char *cursor=text,*why="";CkAnswer answer=ck_check(&cursor,&why);
    int ok=answer==wanted;
    printf("%s: %s%s%s\n",ok?"PASS":"FAIL",name,ok?"":": ",ok?"":why);
    return ok;
}

/* Ogni insieme ha un elemento. P e vuota o piena sull'ultimo insieme;
 * nessun vecchio indice booleano puo diventare uno di questi 160 tipi. */
static void predicate_model(char *text,size_t size,int full) {
    size_t used=(size_t)snprintf(text,size,"certificate counterexample\n");
    for (int i=1;i<=160;i++) used+=(size_t)snprintf(text+used,size-used,"sort S%d 1\n",i);
    snprintf(text+used,size-used,"symbol c S160\nsymbol P S160 bool\nstatement (P c)\ntable c 0\ntable P %d\nend\n",full);
}

/* I due prodotti hanno cardinalita 2*3 e 1*6, o 1*5 nella variante falsa.
 * Gli altri insiemi sono vuoti: troncamenti degli indici darebbero un conto
 * diverso. Non si usano le routine del motore per stabilire questi numeri. */
static void product_model(char *text,size_t size,int altered) {
    size_t used=(size_t)snprintf(text,size,"certificate example\n");
    for (int i=1;i<=40;i++) {
        int cardinal=i==17?2:i==34?3:i==33?1:i==18?(altered?5:6):0;
        used+=(size_t)snprintf(text+used,size-used,"sort S%d %d\n",i,cardinal);
    }
    snprintf(text+used,size-used,"statement (equipotent (S17 S34) (S33 S18))\nend\n");
}

/* Il formato a clausole verifica true senza enumerare le configurazioni
 * vuoto/non vuoto del vecchio formato per casi. */
static void large_refutation(char *text,size_t size,int legacy,int sorts) {
    size_t used=(size_t)snprintf(text,size,"certificate %s\n",legacy?"structure":"saturation");
    for (int i=1;i<=sorts;i++) used+=(size_t)snprintf(text+used,size-used,"sort S%d\n",i);
    snprintf(text+used,size-used,legacy?"statement true\nend\n":"statement true\nclausify forward\nend\n");
}

/* I certificati alterati mantengono il tipo di controllo richiesto. Una
 * risposta unsupported non viene mai scambiata con un'accettazione. */
int main(void) {
    char text[32768];int passed=1;
    predicate_model(text,sizeof text,0);
    passed=expect("160 finite sorts remain separate from the boolean type and certify an empty predicate counterexample",text,CK_ACCEPTED) && passed;
    predicate_model(text,sizeof text,1);
    passed=expect("filling the last predicate invalidates the same many-sort counterexample",text,CK_REJECTED) && passed;
    product_model(text,sizeof text,0);
    passed=expect("products over sorts beyond four-bit indices have their exact cardinalities",text,CK_ACCEPTED) && passed;
    product_model(text,sizeof text,1);
    passed=expect("changing one wide-index product cardinality invalidates the claimed equipotence",text,CK_REJECTED) && passed;
    large_refutation(text,sizeof text,0,160);
    passed=expect("a saturation proof with 160 sorts has no legacy case-mask declaration ceiling",text,CK_ACCEPTED) && passed;
    large_refutation(text,sizeof text,1,160);
    passed=expect("the legacy case format reports its own mask representation limit without claiming a partial proof",text,CK_UNSUPPORTED) && passed;
    large_refutation(text,sizeof text,1,31);
    passed=expect("representable legacy case counts still report exhaustion before an unmanageable enumeration",text,CK_UNSUPPORTED) && passed;

    /* Le chiavi devono conservare gli identificatori e riusare l'indice
     * per due occorrenze sintatticamente uguali di una stessa equipotenza. */
    ck_status=CK_ACCEPTED;
    int a[2][2]={{16,33},{32,17}},b[2][2]={{16,34},{32,17}};
    long long first=ck_sides_code(a),same=ck_sides_code(a),different=ck_sides_code(b);
    int ok=first==same && first!=different && ck_side_sort(first,0,0)==16 && ck_side_sort(first,0,1)==33;
    printf("%s: canonical equipotence sides preserve complete identifiers without nibble packing\n",ok?"PASS":"FAIL");passed &= ok;
    ck_structure_clear();ck_status=CK_ACCEPTED;
    ok=!ck_sorts_reserve((size_t)INT_MAX+1) && ck_status==CK_UNSUPPORTED;
    printf("%s: an unrepresentable sort count is unsupported rather than truncated\n",ok?"PASS":"FAIL");passed &= ok;
    predicate_model(text,sizeof text,0);
    passed=expect("a new certificate succeeds after sort-storage exhaustion",text,CK_ACCEPTED) && passed;
    return passed?0:1;
}
