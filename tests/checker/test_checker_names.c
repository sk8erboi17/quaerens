/* I nomi del certificato crescono col testo. Nomi con lo stesso prefisso
 * lungo devono restare diversi per sort, simboli e variabili legate;
 * l'esaurimento della memoria non permette una copia troncata.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 * cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_checker_names \
 *     tests/checker/test_checker_names.c && /tmp/test_checker_names
 */
#include <stdio.h>
#include <string.h>
#define main checker_main
#include "../../checker.c"
#undef main

/* I nomi sono dati: due code diverse devono identificare due oggetti,
 * anche se tutte le precedenti 511 lettere coincidono. */
static void long_name(char *text,char first,char last) {
    memset(text,'a',512);text[0]=first;text[511]=last;text[512]=0;
}

/* Controlla il certificato senza accedere alle strutture della sua prova.
 * Unsupported rimane distinto da Accepted. */
static int expect(const char *name,const char *text,CkAnswer wanted) {
    const char *cursor=text,*why="";
    CkAnswer found=ck_check(&cursor,&why);int ok=found==wanted;
    printf("%s: %s%s%s\n",ok?"PASS":"FAIL",name,ok?"":": ",ok?"":why);
    return ok;
}

/* Il predicato e vuoto o pieno sul solo elemento. La terza variante usa
 * nella conclusione un altro nome, diverso soltanto nell'ultimo carattere. */
static void model(char *text,size_t size,int altered) {
    char set[513],constant[513],part[513],other[513];
    long_name(set,'S','s');long_name(constant,'c','c');
    long_name(part,'P','p');long_name(other,'P','q');
    snprintf(text,size,"certificate counterexample\nsort %s 1\nsymbol %s %s\nsymbol %s %s bool\n"
             "statement (%s %s)\ntable %s 0\ntable %s %d\nend\n",set,constant,set,part,set,
             altered==2?other:part,constant,constant,part,altered==1);
}

/* Una relazione di uguaglianza fra due variabili universali e falsa su
 * due elementi. Se i nomi venissero troncati diventerebbe x=x, vera. */
static void quantifiers(char *text,size_t size) {
    char set[513],first[513],second[513];
    long_name(set,'S','s');long_name(first,'x','x');long_name(second,'x','y');
    snprintf(text,size,"certificate counterexample\nsort %s 2\n"
             "statement (forall %s %s (forall %s %s (= %s %s)))\nend\n",
             set,first,set,second,set,first,second);
}

/* Il lettore non scrive mai una stringa lunga nel blocco di riserva di
 * ck_alloc quando l'arena e piena; il controllo successivo riparte da zero. */
int main(void) {
    char text[8192];int passed=1;
    model(text,sizeof text,0);
    passed=expect("512-character sort and symbol names independently certify an empty predicate counterexample",text,CK_ACCEPTED) && passed;
    model(text,sizeof text,1);
    passed=expect("filling the long-named predicate invalidates the counterexample",text,CK_REJECTED) && passed;
    model(text,sizeof text,2);
    passed=expect("a different last character cannot resolve to a declared long name",text,CK_REJECTED) && passed;
    quantifiers(text,sizeof text);
    passed=expect("long bound names differing only at their ends remain distinct in a two-element countermodel",text,CK_ACCEPTED) && passed;
    char name[513];long_name(name,'P','p');
    ck_cursor=name;ck_status=CK_ACCEPTED;ck_used=CK_ARENA_BYTES-16;
    const char *copy=NULL;
    int ok=!ck_name(&copy) && ck_status==CK_UNSUPPORTED && !copy;
    printf("%s: exhausted name storage cannot publish a truncated string\n",ok?"PASS":"FAIL");passed &= ok;
    model(text,sizeof text,0);
    passed=expect("a new independent certificate succeeds after name-storage exhaustion",text,CK_ACCEPTED) && passed;
    return passed?0:1;
}
