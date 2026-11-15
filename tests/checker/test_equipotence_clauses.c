/* Test della regola 11m-e del verificatore: l'equipotenza tra le clausole
 * di un certificato di saturazione, compilato senza il motore.
 *
 * Tre cose. I certificati giusti (due refutazioni vere, trovate da un
 * dimostratore del primo ordine sulle clausole del verificatore) devono
 * essere accettati. I tentativi di usare la regola per dimostrare più di
 * ciò che dà devono essere respinti: l'equipotenza stessa, la simmetria, un
 * enunciato a cui è stata tolta l'ipotesi, una tesi cambiata. E un oracolo
 * finito ricontrolla la regola: su un universo di tre elementi prova tutte
 * le coppie di sottoinsiemi A e B e tutte le coppie di funzioni, e verifica
 * che le quattro clausole, con la proposizione vera, abbiano un modello
 * esattamente quando A e B hanno lo stesso numero di elementi. L'oracolo
 * conta gli elementi: non usa la traduzione in clausole.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_equipotence_clauses \
 *         tests/checker/test_equipotence_clauses.c && /tmp/test_equipotence_clauses
 */

#include <stdio.h>
#include <string.h>
#define main checker_main
#include "../../checker.c"
#undef main

/* "Se S e T sono equipotenti e a è in S, allora T ha un elemento." */
#define NONEMPTY_HEAD "certificate saturation\nsort S\nsort T\nsymbol a S\n"
#define NONEMPTY_STATEMENT "statement (implies (and (equipotent S T) true) (exists y T (= y y)))\n"
#define NONEMPTY_STEPS \
    "clausify\n" \
    "step reflex 7 2 (subst) (clause (neg (f 6 (v 0))) (neg (f 2 (v 0))))\n" \
    "step resolve 11 0 9 0 (subst (0 (v 64))) (clause (pos (f 6 (v 0))))\n" \
    "step resolve 11 0 8 0 (subst (0 (v 64))) (clause (pos (f 3)))\n" \
    "component (clause (pos (f 3)))\n" \
    "complement 0\n" \
    "step resolve 15 0 1 0 (subst) (clause (neg (f 8)) (neg (f 1 (v 0))) (pos (f 2 (f 4 (v 0)))))\n" \
    "component (clause (neg (f 1 (v 0))) (pos (f 2 (f 4 (v 0)))))\n" \
    "step split 17 (groups (1 0 1 2)) (clause (pos (f 9)) (neg (f 8)))\n" \
    "step resolve 16 0 14 0 (subst) (clause (pos (f 8)))\n" \
    "step resolve 13 0 12 0 (subst (0 (v 64))) (clause (neg (f 2 (v 0))))\n" \
    "step resolve 21 0 18 1 (subst (0 (f 4 (v 64)))) (clause (neg (f 1 (v 0))) (neg (f 9)))\n" \
    "step resolve 0 0 22 0 (subst (64 (f 0))) (clause (neg (f 9)))\n" \
    "step resolve 19 0 23 0 (subst) (clause (neg (f 8)))\n" \
    "step resolve 20 0 24 0 (subst) (clause)\n" \
    "end\n"

/* "Se S e T sono equipotenti e S ha al più un elemento, due elementi di T
 * sono uguali." Usa tutte e quattro le clausole della regola. */
#define UNIQUE \
    "certificate saturation\nsort S\nsort T\nsymbol a T\nsymbol b T\n" \
    "statement (implies (and (and (and (equipotent S T) (forall x S (forall y S (= x y)))) true) true) (= a b))\n" \
    "clausify\n" \
    "step resolve 18 0 16 0 (subst (0 (v 64)) (1 (v 65))) (clause (ne (f 0) (f 1)))\n" \
    "step resolve 18 0 15 0 (subst (0 (v 64)) (1 (v 65))) (clause (pos (f 9 (v 0) (v 1))))\n" \
    "component (clause (pos (f 4)))\n" \
    "complement 0\n" \
    "step resolve 22 0 12 1 (subst) (clause (pos (f 11)) (neg (f 9 (v 0) (v 1))))\n" \
    "component (clause (neg (f 9 (v 0) (v 1))))\n" \
    "step split 23 (groups (1 0 1)) (clause (pos (f 12)) (pos (f 11)))\n" \
    "step resolve 21 0 5 0 (subst) (clause (neg (f 11)) (neg (f 3 (v 0))) (eq (f 5 (f 6 (v 0))) (v 0)))\n" \
    "component (clause (neg (f 3 (v 0))) (eq (f 5 (f 6 (v 0))) (v 0)))\n" \
    "step split 26 (groups (2 0 1 2)) (clause (pos (f 13)) (neg (f 11)))\n" \
    "step resolve 21 0 4 0 (subst) (clause (neg (f 11)) (neg (f 3 (v 0))) (pos (f 2 (f 6 (v 0)))))\n" \
    "component (clause (neg (f 3 (v 0))) (pos (f 2 (f 6 (v 0)))))\n" \
    "step split 29 (groups (3 0 1 2)) (clause (pos (f 14)) (neg (f 11)))\n" \
    "step resolve 20 0 24 0 (subst (0 (v 64)) (1 (v 65))) (clause (neg (f 12)))\n" \
    "step resolve 20 0 13 0 (subst (0 (v 64)) (1 (v 65))) (clause (pos (f 8 (v 0) (v 1))))\n" \
    "step resolve 0 0 27 0 (subst (64 (f 0))) (clause (eq (f 5 (f 6 (f 0))) (f 0)) (neg (f 13)))\n" \
    "step resolve 1 0 27 0 (subst (64 (f 1))) (clause (eq (f 5 (f 6 (f 1))) (f 1)) (neg (f 13)))\n" \
    "step resolve 33 0 11 0 (subst (0 (v 64)) (1 (v 65))) (clause (neg (f 2 (v 0))) (pos (f 7 (v 0) (v 1))))\n" \
    "step resolve 8 0 36 1 (subst (0 (v 64)) (1 (v 65))) (clause (neg (f 2 (v 0))) (eq (v 1) (v 0)) (neg (f 2 (v 1))))\n" \
    "step resolve 30 1 37 0 (subst (64 (f 6 (v 0)))) (clause (neg (f 3 (v 0))) (neg (f 14)) (eq (v 1) (f 6 (v 0))) (neg (f 2 (v 1))))\n" \
    "step resolve 0 0 38 0 (subst (64 (f 0))) (clause (neg (f 14)) (eq (v 0) (f 6 (f 0))) (neg (f 2 (v 0))))\n" \
    "step resolve 30 1 39 2 (subst (64 (f 6 (v 0)))) (clause (neg (f 3 (v 0))) (neg (f 14)) (eq (f 6 (v 0)) (f 6 (f 0))))\n" \
    "step resolve 1 0 40 0 (subst (64 (f 1))) (clause (neg (f 14)) (eq (f 6 (f 1)) (f 6 (f 0))))\n" \
    "step super 41 1 0 35 0 0 1 (subst) (clause (neg (f 14)) (eq (f 5 (f 6 (f 0))) (f 1)) (neg (f 13)))\n" \
    "step super 34 0 0 42 1 0 0 (subst) (clause (neg (f 13)) (neg (f 14)) (eq (f 0) (f 1)))\n" \
    "step resolve 19 0 43 2 (subst) (clause (neg (f 13)) (neg (f 14)))\n" \
    "step resolve 31 0 44 1 (subst) (clause (neg (f 11)) (neg (f 13)))\n" \
    "step resolve 28 0 45 1 (subst) (clause (neg (f 11)))\n" \
    "step resolve 25 1 46 0 (subst) (clause (pos (f 12)))\n" \
    "step resolve 47 0 32 0 (subst) (clause)\n" \
    "end\n"

/* Controlla il certificato text e ritorna la risposta del verificatore. */
static CkAnswer answer(const char *text) {
    const char *cursor = text, *why = "";
    return ck_check(&cursor, &why, 0);
}

/* Conta un errore e lo stampa, se condition è falsa. */
static int expect(int condition, const char *what) {
    if (!condition) printf("FAILED: %s\n", what);
    return !condition;
}

/* I certificati: quelli giusti accettati, quelli alterati no. */
static int certificates(void) {
    int errors = 0;
    errors += expect(answer(NONEMPTY_HEAD NONEMPTY_STATEMENT NONEMPTY_STEPS) == CK_ACCEPTED,
                     "a refutation that uses an equipotence hypothesis is accepted");
    errors += expect(answer(UNIQUE) == CK_ACCEPTED, "a refutation that uses the bijection in both directions is accepted");

    /* Gli stessi passi per un altro enunciato. Senza l'ipotesi di
     * equipotenza l'enunciato è falso (S con un elemento, T vuoto). */
    errors += expect(answer(NONEMPTY_HEAD "statement (implies (and true true) (exists y T (= y y)))\n" NONEMPTY_STEPS) != CK_ACCEPTED,
                     "the same steps without the equipotence hypothesis are not accepted");
    errors += expect(answer(NONEMPTY_HEAD "statement (implies (and (equipotent S T) true) (exists y T (not (= y y))))\n" NONEMPTY_STEPS) != CK_ACCEPTED,
                     "the same steps for a false claim are not accepted");
    errors += expect(answer(NONEMPTY_HEAD "statement (implies (and (not (equipotent S T)) true) (exists y T (= y y)))\n" NONEMPTY_STEPS) != CK_ACCEPTED,
                     "the same steps with the equipotence denied are not accepted");
    errors += expect(answer(NONEMPTY_HEAD "statement (implies (and (equipotent T S) true) (exists y T (= y y)))\n" NONEMPTY_STEPS) != CK_ACCEPTED,
                     "the same steps with the two sets exchanged are not accepted");

    /* L'equipotenza non si dimostra: nessuna refutazione senza passi. */
    errors += expect(answer("certificate saturation\nsort S\nstatement (equipotent S S)\nclausify\nend\n") != CK_ACCEPTED,
                     "an equipotence is not proved by the rule alone");
    errors += expect(answer("certificate saturation\nsort S\nsort T\nstatement (implies (equipotent S T) (equipotent T S))\nclausify\nend\n") != CK_ACCEPTED,
                     "symmetry is not proved by the rule alone");

    /* I prodotti di insiemi non sono trattati: la risposta è "non so". */
    errors += expect(answer("certificate saturation\nsort S\nsort T\nstatement (implies (equipotent (S T) T) true)\nclausify\nend\n") != CK_ACCEPTED,
                     "an equipotence with a product of sets is not accepted in a saturation");
    return errors;
}

/* Prepara il verificatore con i due insiemi S e T, senza simboli
 * dichiarati, e clausifica la condizione scritta in text come enunciato di
 * un certificato di saturazione. Ritorna 0 se non ci riesce. Dopo, ck_r ha
 * i simboli e le clausole: 0 e 1 sono i domini di S e di T. */
static int clausify(const char *text) {
    static const char *const names[2] = {"S", "T"};
    ck_status = CK_ACCEPTED;
    ck_used = ck_depth = 0;
    ck_sides_count = 0;
    ck_sort_count = 2;
    ck_symbol_count = ck_declared = 0;
    if (!ck_sorts_reserve((size_t)ck_sort_count)) return 0;
    memset(ck_sorts, 0, (size_t)ck_sort_count * sizeof *ck_sorts);
    for (int i = 0; i < 2; i++) { ck_sorts[i].name = names[i]; ck_sorts[i].size = (size_t)-1; ck_sorts[i].base = ck_sorts[i].target = ck_sorts[i].symbol = -1; }
    ck_cursor = text;
    const CkNode *root = ck_read_condition();
    if (ck_status != CK_ACCEPTED) return 0;
    ck_r_prepare(root, NULL, 0);
    return ck_status == CK_ACCEPTED;
}

/* Due scritture della stessa equipotenza hanno ognuna la sua proposizione
 * e le sue due funzioni: "S ~ T implica S ~ T" non si chiude da solo, e
 * senza passi non è accettato. Lo sa dimostrare 11c, con i testimoni. */
static int two_names(void) {
    int errors = 0;
    errors += expect(answer("certificate saturation\nsort S\nsort T\nstatement (implies (equipotent S T) (equipotent S T))\nclausify\nend\n") != CK_ACCEPTED,
                     "an equipotence is not derived from another writing of itself without steps");
    errors += expect(clausify("(implies (equipotent S T) (equipotent S T))"), "the statement is clausified");
    int maps = 0, propositions = 0;
    for (int i = 2; i < ck_r.symbols; i++) {
        maps += ck_r.symbol[i].arity == 1 && !ck_r.symbol[i].predicate;
        propositions += ck_r.symbol[i].arity == 0 && ck_r.symbol[i].predicate;
    }
    errors += expect(maps == 4 && propositions >= 2, "each equipotence has its own proposition and its own two maps");
    return errors;
}

/* L'interpretazione provata dall'oracolo su {0, 1, 2}: i due sottoinsiemi
 * come bit, la proposizione, le due funzioni. */
static int part_s, part_t, proposition, there[3], back[3];

/* Il valore del termine t quando la variabile vale x. I simboli sono quelli
 * della clausificazione di (equipotent S T): 3 e 4 le due funzioni. */
static int term_value(int t, int x) {
    if (t < 0) return x;
    CkRTerm n = ck_r.term[t];
    int argument = n.count ? term_value(ck_r.argument[n.first], x) : 0;
    return n.symbol == 3 ? there[argument] : back[argument];
}

/* La verità del letterale l quando la variabile vale x: 0 e 1 sono i
 * domini di S e di T, 2 la proposizione dell'equipotenza. */
static int literal_holds(CkRLiteral l, int x) {
    int truth;
    if (l.right) truth = term_value(l.left, x) == term_value(l.right, x);
    else {
        CkRTerm n = ck_r.term[l.left];
        int argument = n.count ? term_value(ck_r.argument[n.first], x) : 0;
        truth = n.symbol == 0 ? part_s >> argument & 1 : n.symbol == 1 ? part_t >> argument & 1 : proposition;
    }
    return truth != l.negative;
}

/* Ritorna 1 se le prime quattro clausole, quelle della regola, valgono
 * per ogni valore della variabile. */
static int rule_holds(void) {
    for (int i = 0; i < 4; i++)
        for (int x = 0; x < 3; x++) {
            CkRClause c = ck_r.clause[i];
            int holds = 0;
            for (int j = 0; j < c.count; j++) holds = holds || literal_holds(c.literal[j], x);
            if (!holds) return 0;
        }
    return 1;
}

/* L'oracolo. Per ogni coppia di sottoinsiemi A e B di {0, 1, 2}:
 *   - con la proposizione falsa le clausole valgono sempre (non impongono
 *     nulla dove l'equipotenza non vale);
 *   - con la proposizione vera hanno un modello, cioè due funzioni che le
 *     soddisfano, esattamente quando A e B hanno tanti elementi quanti.
 * Il primo punto e il "se" del secondo dicono che ogni modello
 * dell'enunciato si estende alle clausole (la regola non dimostra il
 * falso); il "solo se" dice che le clausole dicono davvero l'equipotenza. */
static int oracle(void) {
    int errors = 0;
    errors += expect(clausify("(equipotent S T)"), "the statement is clausified");

    /* Simboli: 0 e 1 i domini, 2 la proposizione, 3 e 4 le due funzioni.
     * Clausole: le quattro della regola, poi l'enunciato negato. */
    errors += expect(ck_r.symbols == 5 && ck_r.clauses == 5, "the rule gives one proposition, two maps and four clauses");
    if (errors) return errors;
    static const int count[8] = {0, 1, 1, 2, 1, 2, 2, 3};
    for (part_s = 0; part_s < 8; part_s++) for (part_t = 0; part_t < 8; part_t++) {
        int model = 0, free = 1;
        for (int f = 0; f < 27; f++) for (int g = 0; g < 27; g++) {
            there[0] = f % 3; there[1] = f / 3 % 3; there[2] = f / 9;
            back[0] = g % 3; back[1] = g / 3 % 3; back[2] = g / 9;
            proposition = 0;
            free = free && rule_holds();
            proposition = 1;
            model = model || rule_holds();
        }
        errors += expect(free, "with the proposition false the clauses of the rule hold whatever the maps are");
        errors += expect(model == (count[part_s] == count[part_t]),
                         "with the proposition true the clauses have a model exactly when the two sets have as many elements");
    }
    return errors;
}

int main(void) {
    int errors = certificates() + two_names() + oracle();
    if (errors) { printf("%d checks failed\n", errors); return 1; }
    printf("equipotence in saturation certificates: all checks passed\n");
    return 0;
}
