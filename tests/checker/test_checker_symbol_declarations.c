/* Deposito indipendente delle dichiarazioni, oltre 256 simboli, con
 * modelli veri e alterati, prove FO e nomi introdotti nei rami.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 * cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_checker_symbol_declarations \
 *     tests/checker/test_checker_symbol_declarations.c && /tmp/test_checker_symbol_declarations
 */
#include <stdio.h>
#include <string.h>
#define main checker_main
#include "../../checker.c"
#undef main

/* Controlla un certificato fornito come dato e riporta una descrizione
 * inglese. Una risposta non controllabile non vale come accettazione. */
static int expect(const char *name,const char *text,CkAnswer wanted) {
    const char *cursor=text,*why="";CkAnswer found=ck_check(&cursor, &why, 0);
    int ok=found==wanted;
    printf("%s: %s%s%s\n",ok?"PASS":"FAIL",name,ok?"":": ",ok?"":why);
    return ok;
}

/* Scrive un controesempio su un solo elemento: ogni P_i e vuota, percio
 * P_count(c) e falso. altered cambia o omette soltanto l'ultima tabella. */
static void model(char *text,size_t size,int count,int altered) {
    size_t used=(size_t)snprintf(text,size,"certificate counterexample\nsort S 1\nsymbol c S\n");
    for (int i=1;i<=count;i++) used+=(size_t)snprintf(text+used,size-used,"symbol P%d S bool\n",i);
    used+=(size_t)snprintf(text+used,size-used,"statement (P%d c)\ntable c 0\n",count);
    for (int i=1;i<=count;i++) if (i<count || altered!=2)
        used+=(size_t)snprintf(text+used,size-used,"table P%d %d\n",i,i==count?altered:0);
    snprintf(text+used,size-used,"end\n");
}

/* Una refutazione di NOT(P(c)=>P(c)) usa soltanto l'ultimo predicato.
 * L'indice del nome tecnico segue tutte le dichiarazioni originali.
 * Una conclusione diversa rende invece errati i passi gia scritti. */
static void proof(char *text,size_t size,int count,int altered) {
    size_t used=(size_t)snprintf(text,size,"certificate saturation\nsort S\nsymbol c S\n");
    for (int i=1;i<=count;i++) used+=(size_t)snprintf(text+used,size-used,"symbol P%d S bool\n",i);
    used+=(size_t)snprintf(text+used,size-used,"statement (implies (P%d c) (P%d c))\nclausify\n",count,count-altered);
    snprintf(text+used,size-used,
        "step resolve 4 0 1 0 (subst) (clause (pos (f %d (f 0))))\n"
        "step resolve 4 0 2 0 (subst) (clause (neg (f %d (f 0))))\n"
        "step resolve 5 0 6 0 (subst) (clause)\nend\n",count,count);
}

/* Un witness di biiezione introduce due nomi a cavallo della capacita
 * corrente. I riferimenti gia letti devono sopravvivere alla crescita. */
static void witnesses(char *text,size_t size) {
    size_t used=(size_t)snprintf(text,size,"certificate structure\nsort S\nsort T\n");
    for (int i=0;i<254;i++) used+=(size_t)snprintf(text+used,size-used,"symbol P%d S bool\n",i);
    const char *claim="(implies (and (equipotent S T) (exists s S (= s s))) (exists t T (= t t)))";
    snprintf(text+used,size-used,"statement %s\ncase (not %s)\n"
        "witness (exists s S (= s s)) s_1\n"
        "witness (equipotent S T) varphi_1 psi_2\n"
        "instance (exists t T (= t t)) (varphi_1 s_1)\nend\n",claim,claim);
}

/* Le tavole con un valore alterato o mancante restano respinte anche
 * oltre il vecchio limite. Un errore di risorse non contamina il controllo
 * successivo; ck_check libera il deposito a entrambe le uscite. */
int main(void) {
    char text[65536];int passed=1;
    model(text,sizeof text,512,0);
    passed=expect("512 predicates with empty tables independently refute membership in the last predicate",text,CK_ACCEPTED) && passed;
    model(text,sizeof text,512,1);
    passed=expect("changing the last predicate to contain the element invalidates the counterexample",text,CK_REJECTED) && passed;
    model(text,sizeof text,512,2);
    passed=expect("omitting the last table remains rejected after growing past 256 declarations",text,CK_REJECTED) && passed;
    proof(text,sizeof text,512,0);
    passed=expect("resolution after 513 declarations uses the correct original and technical symbol indices",text,CK_ACCEPTED) && passed;
    proof(text,sizeof text,512,1);
    passed=expect("altering the statement does not turn an unchanged resolution proof into a proof of a false implication",text,CK_REJECTED) && passed;
    witnesses(text,sizeof text);
    passed=expect("two bijection witnesses can cross a declaration storage boundary without losing their names",text,CK_ACCEPTED) && passed;
    ck_status=CK_ACCEPTED;
    int ok=!ck_symbols_reserve((size_t)INT_MAX+1) && ck_status==CK_UNSUPPORTED;
    ck_structure_clear();
    printf("%s: declaration size overflow remains unsupported and releases its storage\n",ok?"PASS":"FAIL");passed &= ok;
    proof(text,sizeof text,512,0);
    passed=expect("a complete proof succeeds after unsupported symbol storage has been cleared",text,CK_ACCEPTED) && passed;
    return passed?0:1;
}
