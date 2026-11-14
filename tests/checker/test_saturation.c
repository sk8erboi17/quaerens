/* Test del kernel dei certificati di saturazione (11m), compilato senza
 * il motore. I certificati manuali includono passi e sostituzioni alterati.
 * Il secondo controllo confronta la clausificazione con un oracolo finito:
 * tutte le parti di un universo di due elementi, tutti i predicati binari,
 * tutti i testimoni di Skolem e tutte le interpretazioni dei nomi di Tseitin.
 * L'oracolo valuta direttamente forall/exists, senza usare la traduzione.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_saturation \
 *         tests/checker/test_saturation.c && /tmp/test_saturation
 */

#include <stdio.h>
#include <string.h>
#define main checker_main
#include "../../checker.c"
#undef main

/* Una prova di riflessivita. Le clausole 1 e 3 danno k!=k, poi il vuoto.
 * Il simbolo 0 e il dominio S, 1 il testimone k, 2 il nome della congiunzione. */
#define REFLEX_HEAD "certificate saturation\nsort S\nstatement (forall x S (= x x))\nclausify\n"
#define REFLEX_FIRST "step resolve 1 0 3 0 (subst) (clause (ne (f 1) (f 1)))\n"
#define REFLEX_LAST "step reflex 4 0 (subst) (clause)\nend\n"

/* Con un elemento a dichiarato, exists x:S x=x e vero. La prima clausola
 * e D_S(a); i due genitori successivi hanno variabili distinte, 0 e 64. */
#define EXISTS_HEAD "certificate saturation\nsort S\nsymbol a S\nstatement (exists x S (= x x))\nclausify\n"
#define EXISTS_RESULT "(clause (neg (f 1 (f 0))) (ne (f 0) (f 0)))\n"
#define EXISTS_END "step reflex 5 1 (subst) (clause (neg (f 1 (f 0))))\n" \
    "step resolve 6 0 0 0 (subst) (clause)\nend\n"

/* Un certificato completo e la risposta richiesta. text appartiene ai dati
 * statici; why e una parte del motivo richiesta per i casi respinti. */
typedef struct {
    const char *name, *text, *why;
    CkAnswer answer;
} ProofCase;

/* Controlla i certificati completi, compresi gli attacchi agli indici e alle
 * variabili standardizzate separatamente nei due genitori. */
static int certificates(void) {
    static const ProofCase cases[] = {
        {"truth clausifies to an immediate contradiction", "certificate saturation\nstatement true\nclausify\nend\n", "", CK_ACCEPTED},
        {"falsehood has no empty-clause proof", "certificate saturation\nstatement (not true)\nclausify\nend\n", "clausola vuota", CK_REJECTED},
        {"reflexivity is proved by explicit resolution and equality resolution", REFLEX_HEAD REFLEX_FIRST REFLEX_LAST, "", CK_ACCEPTED},
        {"changing a literal sign invalidates a written step", REFLEX_HEAD "step resolve 1 0 3 0 (subst) (clause (eq (f 1) (f 1)))\n" REFLEX_LAST, "non segue", CK_REJECTED},
        {"dropping a derived literal is not a resolution step", REFLEX_HEAD "step resolve 1 0 3 0 (subst) (clause)\nend\n", "non e il risultato", CK_REJECTED},
        {"a missing parent is rejected", REFLEX_HEAD "step resolve 80 0 3 0 (subst) (clause)\nend\n", "fuori limite", CK_REJECTED},
        {"a declared element justifies nonemptiness", EXISTS_HEAD "step resolve 3 0 4 0 (subst (0 (f 0)) (64 (f 0))) " EXISTS_RESULT EXISTS_END, "", CK_ACCEPTED},
        {"identical variable names in different parents are not identified", EXISTS_HEAD "step resolve 3 0 4 0 (subst (0 (f 0))) " EXISTS_RESULT EXISTS_END, "complementari", CK_REJECTED},
        {"a variable cannot be substituted twice", EXISTS_HEAD "step resolve 3 0 4 0 (subst (0 (f 0)) (0 (f 0))) " EXISTS_RESULT EXISTS_END, "due volte", CK_REJECTED},
        {"a domain predicate is not a term for substitution", EXISTS_HEAD "step resolve 3 0 4 0 (subst (0 (f 1 (f 0)))) " EXISTS_RESULT EXISTS_END, "posto sbagliato", CK_REJECTED},
        {"without a declared element an empty sort remains possible", "certificate saturation\nsort S\nstatement (exists x S (= x x))\nclausify\nend\n", "clausola vuota", CK_REJECTED},
        {"a theorem does not become true by declaring an unused empty sort", "certificate saturation\nsort S\nsort T\nstatement (implies (exists x S (= x x)) (exists y T (= y y)))\nclausify\nend\n", "clausola vuota", CK_REJECTED},
        {"a proof must contain clausification", "certificate saturation\nstatement true\nend\n", "clausola vuota", CK_REJECTED},
    };
    int passed = 1;
    for (size_t i = 0; i < sizeof cases / sizeof *cases; i++) {
        const char *text = cases[i].text, *why;
        CkAnswer answer = ck_check(&text, &why);
        int ok = answer == cases[i].answer && (!cases[i].why[0] || strstr(why, cases[i].why));
        printf("%s: %s%s%s\n", ok ? "PASS" : "FAIL", cases[i].name, ok ? "" : ": ", ok ? "" : why);
        passed &= ok;
    }
    return passed;
}

/* Valuta un termine su {0,1}. skolem contiene i valori della funzione nuova
 * (una cifra per argomento); le variabili hanno i valori nei bit di vars. */
static int term_value(int t, int vars, int skolem) {
    if (t < 0) return vars >> (-t - 1) & 1;
    const CkRTerm n = ck_r.term[t];
    int argument = n.count ? term_value(ck_r.argument[n.first], vars, skolem) : 0;
    return skolem >> argument & 1;
}

/* Valuta un letterale: il simbolo 0 e R, 1 e 2 i domini; quelli nuovi,
 * esclusa la funzione di Skolem 3, hanno una tavola di due bit in gates. */
static int literal_value(CkRLiteral l, int vars, int skolem, unsigned gates, int S, int T, int R) {
    int value = 1;
    if (l.right) value = term_value(l.left, vars, skolem) == term_value(l.right, vars, skolem);
    else if (l.left) {
        const CkRTerm n = ck_r.term[l.left];
        int a = term_value(ck_r.argument[n.first], vars, skolem);
        if (n.symbol == 0) {
            int b = term_value(ck_r.argument[n.first + 1], vars, skolem);
            value = R >> (a + 2 * b) & 1;
        } else if (n.symbol == 1) value = S >> a & 1;
        else if (n.symbol == 2) value = T >> a & 1;
        else value = gates >> (2 * (n.symbol - 4) + a) & 1;
    }
    return value != l.negative;
}

/* Decide le clausole della traduzione provando tutte le interpretazioni
 * ausiliarie. Ogni clausola e valutata per tutti i valori delle variabili. */
static int clauses_have_model(int S, int T, int R, int function) {
    unsigned gate_count = (unsigned)(ck_r.symbols - 4), possibilities = 1u << (2 * gate_count);
    for (int skolem = 0; skolem < (function ? 4 : 2); skolem++)
        for (unsigned gates = 0; gates < possibilities; gates++) {
            int all = 1;
            for (int c = 0; c < ck_r.clauses && all; c++)
                for (int vars = 0; vars < 4 && all; vars++) {
                    int some = 0;
                    for (int i = 0; i < ck_r.clause[c].count; i++) some |= literal_value(ck_r.clause[c].literal[i], vars, skolem, gates, S, T, R);
                    all = some;
                }
            if (all) return 1;
        }
    return 0;
}

/* L'oracolo degli enunciati, direttamente dalla definizione: alternate=0
 * significa forall x:S exists y:T R(x,y), alternate=1 exists x:S forall
 * y:T R(x,y). Le parti vuote sono incluse in tutti e due i quantificatori. */
static int quantified_truth(int S, int T, int R, int alternate, int body) {
    int result = !alternate;
    for (int x = 0; x < 2; x++) if (S >> x & 1) {
        int inner = alternate;
        for (int y = 0; y < 2; y++) if (T >> y & 1) {
            int a = R >> (x + 2 * y) & 1, b = !a;
            int value = body == 0 ? a : body == 1 ? a || b : !a || b;
            inner = alternate ? inner && value : inner || value;
        }
        result = alternate ? result || inner : result && inner;
    }
    return result;
}

/* Confronta la trasformazione con l'oracolo sui 256 modelli di ogni schema.
 * Gli alberi sono dati costruiti nel test, non prodotti dal motore. */
static int clausification(void) {
    int all = 1;
    for (int alternate = 0; alternate < 2; alternate++) for (int body = 0; body < 3; body++) {
        ck_status = CK_ACCEPTED;
        ck_used = ck_depth = 0;
        ck_sort_count = 2;
        ck_symbol_count = ck_declared = 1;
        memset(ck_sorts, 0, sizeof ck_sorts);
        memset(ck_symbols, 0, sizeof ck_symbols);
        ck_symbols[0].arity = 2; ck_symbols[0].arg[1] = 1; ck_symbols[0].result = CK_BOOL;
        const CkNode *x = ck_make(CK_VAR, 1, NULL, NULL), *y = ck_make(CK_VAR, 0, NULL, NULL);
        const CkNode *a = ck_make(CK_EQ, 0, ck_make(CK_APP, 0, x, y), NULL);
        const CkNode *b = ck_make(CK_NOT, 0, a, NULL);
        const CkNode *inner = body ? ck_make(body == 1 ? CK_OR : CK_IMPLIES, 0, a, b) : a;
        inner = ck_make(alternate ? CK_FORALL : CK_EXISTS, 1, inner, NULL);
        const CkNode *root = ck_make(alternate ? CK_EXISTS : CK_FORALL, 0, inner, NULL);
        ck_r_prepare(root, NULL, 0);
        if (ck_status != CK_ACCEPTED) { printf("FAIL: clausification was unsupported\n"); return 0; }
        for (int S = 0; S < 4; S++) for (int T = 0; T < 4; T++) for (int R = 0; R < 16; R++) {
            int truth = quantified_truth(S, T, R, alternate, body);
            if (clauses_have_model(S, T, R, alternate) == truth) {
                printf("FAIL: schema %d/%d, domains %d/%d, relation %d\n", alternate, body, S, T, R);
                all = 0;
            }
        }
    }
    printf("%s: clausification agrees with all 1536 finite interpretations, including empty sorts and dependent Skolem functions\n", all ? "PASS" : "FAIL");
    return all;
}

int main(void) {
    int passed = certificates();
    passed = clausification() && passed;
    return passed ? 0 : 1;
}
