/* I pedici numerici dei nomi astratti sono indici interi, non cifre.
 * Gli oracoli sono riflessivita, appartenenza e modelli di uno o due
 * elementi; ogni verdetto positivo o negativo deve avere il certificato.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 * cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_subscript_names \
 *     tests/engine/test_subscript_names.c && /tmp/test_subscript_names
 */
#include "../statement_checks.h"

/* I dati dichiarano l'esito atteso, senza riprodurre il parser o la ricerca.
 * Un errore indipendente di certificazione rende sempre falso il controllo. */
static int certified(const char *text,Outcome wanted,const char *name) {
    int previous=unjustified_steps;
    Verdict v=prove(text,'n',0,0);
    int ok=v.outcome==wanted && v.certificate && unjustified_steps==previous;
    if (!ok) { verdict_print(stdout,&v);putchar('\n'); }
    return check_result(ok,name);
}

/* Tutti i nomi A_i sono distinti. La parte vuota nell'ultima posizione
 * smentisce la tesi, anche oltre i vecchi tetti dei nomi e del testo. */
static int many_numeric_names(void) {
    char text[65536];size_t used=(size_t)snprintf(text,sizeof text,"(c \\in S_{120}");
    for (int i=1;i<=1024;i++)
        used+=(size_t)snprintf(text+used,sizeof text-used," \\land A_{%d} \\subseteq S_{120}",i);
    snprintf(text+used,sizeof text-used,") \\implies c \\in A_{1024}");
    int passed=certified(text,REFUTED,"1024 numeric subscripts remain distinct and the last empty part gives a certified counterexample");
    return check_result(declared_symbols==1025,"numeric subscripts beyond 99 and 100 retain the complete declaration prefix") && passed;
}

/* Il limite del tipo si esercita anche sul prefisso greco piu lungo:
 * varphi seguito dall'indice deve entrare nel linguaggio dei certificati. */
static int index_representation(void) {
    char text[512];
    snprintf(text,sizeof text,"(c \\in S_{%d} \\land \\varphi_{%d} \\colon S_{%d} \\to S_{%d}) \\implies \\varphi_{%d}(c)=\\varphi_{%d}(c)",
             INT_MAX,INT_MAX,INT_MAX,INT_MAX,INT_MAX,INT_MAX);
    int passed=certified(text,PROVED,"the largest representable numeric subscript survives parsing and independent certification of a Greek map name");
    snprintf(text,sizeof text,"\\forall x_{%d} \\in S_{%d}\\ (x_{%d}=x_{%d})",INT_MAX,INT_MAX,INT_MAX,INT_MAX);
    passed=certified(text,PROVED,"bound-variable and sort subscripts can reach the representation limit without truncation") && passed;
    snprintf(text,sizeof text,"\\exists S_{%d} \\neq \\emptyset\\ \\exists f_{%d} \\colon S_{%d} \\to S_{%d}\\ (\\forall x_{%d} \\in S_{%d}\\ (f_{%d}(x_{%d})=x_{%d}))",
             INT_MAX,INT_MAX,INT_MAX,INT_MAX,INT_MAX,INT_MAX,INT_MAX,INT_MAX,INT_MAX);
    passed=certified(text,PROVED,"existential numeric subscripts at the representation limit cannot be decoded as letter tags or overflow") && passed;
    /* Riduzione del dato malformato trovato dal fuzz di robustezza,
     * seme 37507: anche un errore successivo deve evitare il trabocco. */
    snprintf(text,sizeof text,"\\exists S_{%d} \\neq = \\emptyset",INT_MAX);
    Verdict malformed=prove(text,'n',0,0);
    passed=check_result(malformed.outcome==UNSUPPORTED && !malformed.certificate,
                        "a malformed existential header at the largest numeric index is rejected without overflowing") && passed;
    snprintf(text,sizeof text,"\\forall x_{%llu} \\in S\\ (x_{%llu}=x_{%llu})",
             (unsigned long long)INT_MAX+1,(unsigned long long)INT_MAX+1,(unsigned long long)INT_MAX+1);
    Verdict v=prove(text,'n',0,0);
    return check_result(v.outcome==UNSUPPORTED && !v.certificate,"a subscript outside the index representation is rejected without proving a truncated formula") && passed;
}

/* Zero esplicito, nome senza pedice e numeri positivi non sono alias.
 * Le forme senza graffe con piu cifre non sono un singolo pedice LaTeX. */
int main(void) {
    engine_init();
    int passed=certified("\\forall x_{12} \\in S_{120}\\ (x_{12}=x_{12})",PROVED,
                         "a multidigit sort and bound variable preserve reflexivity");
    passed=certified("\\forall x_{12},x_{13} \\in S_{120}\\ (x_{12}=x_{13})",REFUTED,
                     "different multidigit variables are refuted on two elements instead of being aliased") && passed;
    passed=certified("(a \\in S \\land a_{0} \\in S) \\implies a=a_{0}",REFUTED,
                     "explicit zero remains different from a name without a subscript") && passed;
    passed=certified("(c_{12} \\in S_{120} \\land c_{13} \\in S_{120}) \\implies c_{12}=c_{13}",REFUTED,
                     "multidigit constant declarations retain independent values in a certified two-element model") && passed;
    passed=certified("(c \\in S \\land A_{99} \\subseteq S \\land A_{100} \\subseteq S \\land c \\in A_{99}) \\implies c \\in A_{100}",REFUTED,
                     "99 and 100 refer to different parts and cannot alias the internal zero or letter tags") && passed;
    passed=certified("({\\le_{120}} \\subseteq S \\times S) \\implies \\forall x \\in S\\ (x \\le_{120} x \\implies x \\le_{120} x)",PROVED,
                     "a multidigit relation-command subscript is retained by the independent checker") && passed;
    passed=certified("S_{120} \\neq \\emptyset \\implies \\exists x_{12} \\in S_{120}\\ (x_{12}=x_{12})",PROVED,
                     "the nonempty-set notation uses the same multidigit sort as its quantified conclusion") && passed;
    passed=certified("\\exists S_{120} \\neq \\emptyset\\ \\exists f_{12} \\colon S_{120} \\to S_{120}\\ (\\forall x_{12} \\in S_{120}\\ (f_{12}(f_{12}(x_{12}))=x_{12} \\land f_{12}(x_{12})\\neq x_{12}))",PROVED,
                     "an existentially named sort with a numeric subscript admits a certified two-cycle") && passed;
    passed=certified("\\exists S_{12},S_{13} \\neq \\emptyset\\ \\exists f_{12} \\colon S_{12} \\to S_{13}\\ (\\forall x_{12} \\in S_{12}\\ (f_{12}(x_{12})=f_{12}(x_{12})))",PROVED,
                     "comma-separated existential sorts retain their distinct numeric subscripts") && passed;
    Verdict impossible=prove("\\exists S_{120} \\neq \\emptyset\\ (\\forall x_{12} \\in S_{120}\\ (x_{12}\\neq x_{12}))",'n',0,0);
    passed=check_result(impossible.outcome==NOT_PROVED && !impossible.certificate,
                        "an impossible nonempty example cannot be certified by dropping its indexed sort's nonemptiness") && passed;
    const char *invalid[]={"\\forall x_12 \\in S\\ (x_12=x_12)",
        "\\forall x_{-12} \\in S\\ (x_{-12}=x_{-12})"};
    int rejected=1;
    for (size_t i=0;i<sizeof invalid/sizeof *invalid;i++) {
        Verdict v=prove(invalid[i],'n',0,0);
        rejected &= v.outcome==UNSUPPORTED && !v.certificate;
    }
    passed=check_result(rejected,"unbraced multidigit and negative subscripts cannot produce a proof") && passed;
    passed=index_representation() && passed;
    passed=many_numeric_names() && passed;
    return passed?0:1;
}
