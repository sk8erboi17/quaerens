/* Un errore di crescita non deve produrre un verdetto positivo, nemmeno
 * se una strategia interna cancella failure per provarne un'altra. La
 * sostituzione di realloc appartiene al test, non al programma.
 * Si verifica anche che i valori visibili dell'ambiente restino agli stessi
 * indirizzi mentre crescono sia i blocchi sia il loro indice.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 * cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_allocation_failures \
 *     tests/engine/test_allocation_failures.c && /tmp/test_allocation_failures
 */
#include <stdlib.h>

static int reject_reallocation;
static int reallocations_left=-1;

/* Simula soltanto il fallimento richiesto dal test; normalmente usa la
 * vera realloc della libreria standard, senza modificare i dati. */
static void *controlled_realloc(void *data, size_t bytes) {
    if (reject_reallocation || reallocations_left==0) return NULL;
    if (reallocations_left>0) reallocations_left--;
    return realloc(data, bytes);
}

#define realloc controlled_realloc
#include "../statement_checks.h"
#undef realloc

/* I due depositi del parser possono crescere separatamente. Nessuno dei
 * due fallimenti deve pubblicare una dichiarazione priva del proprio ruolo;
 * la ripartenza riusa l'eventuale primo deposito gia cresciuto. */
static int symbol_growth_failures(void) {
    int ok=1;
    for (int available=0;available<2;available++) {
        free(symbols);free(symbol_bound);symbols=NULL;symbol_bound=NULL;
        symbol_capacity=symbol_bound_capacity=0;symbol_count=0;
        failure=syntax_error=NULL;work_allocation_failed=0;
        reallocations_left=available;
        int id=symbol_append((Symbol){'c',0,0,{0,0},0,0,0});
        ok &= id<0 && !symbol_count && work_allocation_failed && syntax_error;
        reallocations_left=-1;failure=syntax_error=NULL;work_allocation_failed=0;
        id=symbol_append((Symbol){'c',0,0,{0,0},0,0,0});
        ok &= id==0 && symbol_count==1 && symbol_bound[0]==PART_FREE && symbols[0].name=='c';
    }
    int passed=check_result(ok,"failure of either declaration vector leaves no partial symbol and permits a clean retry");
    Verdict v=prove("c \\in S \\implies c=c",'n',0,0);
    return check_result(v.outcome==PROVED && v.certificate && !work_allocation_failed,
                         "an independent abstract statement can be certified after declaration allocation failures") && passed;
}

/* Il testo lungo non puo tornare a essere un prefisso corto in caso di
 * memoria finita. Item e lettura da stream devono fallire senza formula
 * utilizzabile; una nuova lettura indipendente puo invece riuscire. */
static int input_growth_failures(void) {
    const char *text="\\[c \\in S \\implies c=c\\]";
    reject_reallocation=1;
    Verdict v=prove_item(text,text+strlen(text));
    int ok=v.outcome==NOT_PROVED && !v.certificate && v.reason==OUT_OF_MEMORY;
    reject_reallocation=0;
    v=prove_item(text,text+strlen(text));
    ok &= v.outcome==PROVED && v.certificate && !work_allocation_failed;
    FILE *stream=tmpfile();
    if (stream) {
        fputs("c \\in S \\implies c=c\n",stream);rewind(stream);
        reject_reallocation=1;char *line=read_input_line(stream);reject_reallocation=0;
        ok &= !line && failure==OUT_OF_MEMORY;free(line);fclose(stream);
    } else ok=0;
    failure=syntax_error=NULL;work_allocation_failed=0;
    return check_result(ok,"failed formula or stream growth cannot publish a truncated statement and permits a later certified retry");
}

int main(void) {
    engine_init();
    const char *truth = "\\forall a \\in \\mathbb{Z}\\ (a = a)";
    reject_reallocation = 1;
    Verdict v = prove(truth, 'n', 0, 0);
    int passed = check_result(v.outcome == NOT_PROVED && !v.certificate && v.reason == OUT_OF_MEMORY,
                              "a failed parser-stack allocation produces no proof or counterexample");
    passed = check_result(work_allocation_failed, "allocation failure remains recorded after internal fallbacks") && passed;
    reject_reallocation = 0;
    v = prove(truth, 'n', 0, 0);
    passed = check_result(v.outcome == PROVED && !work_allocation_failed,
                          "a later independent statement starts cleanly and can be proved") && passed;

    /* I nomi numerici sintetici sono dati della prova di memoria: la
     * valutazione non deve interpretarli come lettere di una formula. */
    env_count = 0;
    passed=symbol_growth_failures() && passed;
    passed=input_growth_failures() && passed;
    passed = check_result(env_push('a', rat_int(7)), "the first binding is stored") && passed;
    const Rat *remembered = env_lookup('a');
    int complete = 1;
    for (int i = 0; i < 1100; i++) if (!env_push(1000 + i, rat_int(i))) { complete = 0; break; }
    Num value;
    passed = check_result(complete && remembered == env_lookup('a') && rat_number(*remembered, &value) &&
                          num_equal(value, num_int(7)),
                          "binding addresses and values survive growth beyond sixteen separately allocated blocks") && passed;
    env_count = 0;
    return passed ? 0 : 1;
}
