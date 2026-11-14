/* Test del risolutore proposizionale (28t), senza enunciati matematici.
 * L'oracolo prova tutte le assegnazioni, indipendentemente dalle clausole
 * osservate e dall'apprendimento. Si provano tutte le combinazioni delle
 * otto clausole non tautologiche su due variabili, poi 600 formule casuali
 * su sette variabili. Anche un esito SAT deve soddisfare ogni clausola
 * originale, non soltanto quelle rimaste osservate.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_sat \
 *         tests/engine/test_sat.c && /tmp/test_sat
 */

#include "../statement_checks.h"

/* Una formula proposizionale di test: count clausole, ognuna di length[i]
 * letterali in literal[i]. Le variabili dei dati partono da 2, per lasciare
 * al risolutore la sua costante vera 1. Nessun puntatore e posseduto. */
typedef struct {
    int count;
    int length[48];
    SatLiteral literal[48][3];
} SatCase;

/* Decide la formula con la tabella di verita, senza usare il risolutore. */
static int exhaustive(const SatCase *c, int variables) {
    for (unsigned bits = 0; bits < (1u << variables); bits++) {
        int holds = 1;
        for (int i = 0; i < c->count && holds; i++) {
            int any = 0;
            for (int j = 0; j < c->length[i]; j++) {
                SatLiteral l = c->literal[i][j];
                int value = bits >> (sat_index(l) - 2) & 1;
                any |= l > 0 ? value : !value;
            }
            holds = any;
        }
        if (holds) return 1;
    }
    return 0;
}

/* Confronta il risolutore con l'oracolo e controlla l'assegnazione estratta.
 * Lo stato viene azzerato per ogni caso, come per una nuova ricerca. */
static int agrees(SatState *s, const SatCase *c, int variables) {
    sat_clear(s);
    for (int i = 0; i <= variables; i++) sat_variable(s);
    sat_assign(s, 1, 0);
    for (int i = 0; i < c->count; i++) sat_clause(s, c->literal[i], c->length[i]);
    int answer = sat_solve(s), expected = exhaustive(c, variables);
    if (answer != (expected ? 1 : -1)) return 0;
    if (answer != 1) return 1;
    for (int i = 0; i < c->count; i++) {
        int any = 0;
        for (int j = 0; j < c->length[i]; j++) {
            SatLiteral l = c->literal[i][j];
            int value = s->var[sat_index(l)].value;
            any |= l > 0 ? value == 1 : value == -1;
        }
        if (!any) return 0;
    }
    return 1;
}

/* Generatore deterministico dei soli dati di test, senza effetti sul motore. */
static unsigned next_random(unsigned *state) {
    *state = *state * 1664525u + 1013904223u;
    return *state;
}

int main(void) {
    static const int choices[8][2] = {{2, 0}, {-2, 0}, {3, 0}, {-3, 0}, {2, 3}, {2, -3}, {-2, 3}, {-2, -3}};
    SatState *s = calloc(1, sizeof *s);
    int passed = check_result(s != NULL, "the SAT test state can be allocated");
    if (!s) return 1;
    int all = 1;
    for (unsigned mask = 0; mask < 256; mask++) {
        SatCase c = {0};
        for (int i = 0; i < 8; i++) {
            if (!(mask >> i & 1)) continue;
            c.length[c.count] = choices[i][1] ? 2 : 1;
            c.literal[c.count][0] = choices[i][0];
            c.literal[c.count++][1] = choices[i][1];
        }
        all &= agrees(s, &c, 2);
    }
    passed = check_result(all, "all 256 two-variable clause sets agree with their complete truth tables") && passed;

    /* Formule piu grandi richiedono decisioni, conflitti e ritorni a livelli
     * precedenti; l'oracolo resta il conto completo su 128 assegnazioni. */
    unsigned state = 17041;
    all = 1;
    for (int trial = 0; trial < 600; trial++) {
        SatCase c = {0};
        c.count = 12 + (int)(next_random(&state) % 36);
        for (int i = 0; i < c.count; i++) {
            c.length[i] = 2 + (int)(next_random(&state) % 2);
            for (int j = 0; j < c.length[i]; j++) {
                int v = 2 + (int)(next_random(&state) % 7);
                c.literal[i][j] = next_random(&state) >> 16 & 1 ? v : -v;
            }
        }
        if (!agrees(s, &c, 7)) { all = 0; printf("FAIL: random SAT case %d\n", trial); break; }
    }
    passed = check_result(all, "600 larger clause sets agree with exhaustive enumeration and every returned assignment satisfies the original clauses") && passed;

    /* Un limite e una clausola vuota hanno risposte diverse. */
    sat_clear(s);
    s->broken = 1;
    passed = check_result(sat_solve(s) == 0, "a resource limit is unknown, not UNSAT or SAT") && passed;
    sat_clear(s);
    sat_clause(s, NULL, 0);
    passed = check_result(sat_solve(s) == -1, "an empty clause is UNSAT") && passed;

    /* Oltrepassa tutti i vecchi depositi fissi. Le clausole sono dati
     * soddisfacibili da v2=true; non serve cercare 70.000 decisioni. */
    sat_clear(s);
    for (size_t i = 0; i < 70000 && !s->broken; i++) sat_variable(s);
    SatLiteral long_clause[] = {2, 3, 4, 5, 6, 7, 8, 9};
    for (size_t i = 0; i < 270000 && !s->broken; i++) sat_clause(s, long_clause, 8);
    for (size_t i = 0; i < 33000 && !s->broken; i++) sat_constant(s, 33000, i);
    passed = check_result(!s->broken && s->variables == 70000 && s->clauses == 270000 &&
                          s->literals == 2160000 && s->terms == 33000,
                          "variables, clauses, literals and cached terms grow beyond every old fixed capacity") && passed;
    size_t repeated = sat_constant(s, 33000, 32999);
    passed = check_result(sat_at(s, repeated, 32999) == 1 && sat_at(s, repeated, 256) == -1,
                          "large constants remain exact after repeated hash and vector growth") && passed;
    sat_clear(s);
    free(s);
    return passed ? 0 : 1;
}
