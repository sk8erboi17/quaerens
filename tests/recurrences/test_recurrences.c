/* Test di dominio: successioni definite per ricorrenza (capitolo 26 di
 * main.c, e la lettura a parole del capitolo 21 di parser.c).
 *
 * Cinque controlli; i primi tre sono descritti in tests/statement_checks.h:
 *
 *   1. ogni enunciato di sequences_by_recurrence.tex viene dimostrato, con
 *      la dimostrazione scritta;
 *   2. nessuna variante falsa delle formule di quel file viene dimostrata:
 *      la formula più 1, e la formula più n - n0, che è giusta nel primo
 *      termine e sbagliata dopo;
 *   3. nessun enunciato di false_sequences_by_recurrence.tex viene
 *      dimostrato: è l'antiregressione contro le false dimostrazioni;
 *   4. alcuni enunciati, dati in formula, ricevono il verdetto atteso: FALSO
 *      con l'indice giusto, oppure DIMOSTRATO con una frase attesa nella
 *      dimostrazione;
 *   5. il lettore trasforma un \item scritto a parole nella formula attesa,
 *      e dichiara ciò che non ha letto.
 *
 * Gli enunciati sono dati: il motore non ne conosce nessuno in anticipo.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_recurrences \
 *         tests/recurrences/test_recurrences.c && /tmp/test_recurrences
 */

#include "../statement_checks.h"

/* Un enunciato in formula con il verdetto atteso. */
typedef struct {
    const char *formula;
    Outcome outcome;    /* il verdetto atteso */
    long long witness;  /* REFUTED: l'indice atteso come controesempio */
    const char *text;   /* PROVED: un testo che la dimostrazione deve contenere */
} Expected;

static const Expected expected[] = {
    /* Formule sbagliate: nel primo valore, nella regola, dopo i primi termini. */
    {"a_{0} = 2, a_{n + 1} = 2 a_{n} + 1 \\implies a_{n} = 2^{n + 1} - 1", REFUTED, 0, NULL},
    {"a_{0} = 1, a_{n + 1} = 3 a_{n} + 1 \\implies a_{n} = 2^{n + 1} - 1", REFUTED, 1, NULL},
    {"a_{0} = 1, a_{n + 1} = 2 a_{n} + 1 \\implies a_{n} = 2^{n + 1} - 1 + n(n - 1)(n - 2)", REFUTED, 3, NULL},
    {"a_{0} = 1, a_{n + 1} = 2 a_{n} + 1 \\implies a_{n} = 2^{n + 1} - 1 + \\prod_{j=0}^{7} (n - j)", NOT_PROVED, 0, NULL},
    {"a_{0} = 1, a_{n + 1} = 2 a_{n} + 1 \\implies a_{n} = 2^{n + 1} - 1 + \\prod_{j=0}^{13} (n - j)", NOT_PROVED, 0, NULL},
    {"a_{0} = 2, a_{1} = 6, a_{n + 2} = 5 a_{n + 1} - 6 a_{n} \\implies a_{n} = 2^{n} + 3^{n}", REFUTED, 1, NULL},
    {"a_{0} = 0, a_{1} = \\sin x, a_{n + 1} = 2 \\cos x\\, a_{n} - a_{n - 1} \\implies a_{n} = n \\sin x", NOT_PROVED, 0, NULL},

    /* Stime false: subito, dopo qualche termine, oltre i termini calcolati. */
    {"a_{0} = 0, a_{n + 1} = \\sqrt{6 + a_{n}} \\implies a_{n} < 2", REFUTED, 1, NULL},
    {"a_{0} = 0, a_{n + 1} = \\sqrt{6 + a_{n}} \\implies a_{n} < \\frac{299}{100}", REFUTED, 4, NULL},
    {"a_{0} = 0, a_{n + 1} = \\sqrt{6 + a_{n}} \\implies a_{n} < \\frac{29999}{10000}", NOT_PROVED, 0, NULL},
    {"a_{0} = 0, a_{n + 1} = \\sqrt{6 + a_{n}} \\implies a_{n} > a_{n + 1}", REFUTED, 0, NULL},
    {"a_{0} = 1, a_{n + 1} = 2 a_{n} \\implies a_{n} < 1000", NOT_PROVED, 0, NULL},
    {"a_{0} = 2, a_{n + 1} = \\sqrt{a_{n} - 3} \\implies a_{n} \\ge 0", NOT_PROVED, 0, NULL},

    /* Ipotesi che non definiscono la successione, o tesi fuori dai due tipi. */
    {"a_{1} = 1, a_{n + 2} = a_{n + 1} + a_{n} \\implies a_{n} = F_{n}", UNSUPPORTED, 0, NULL},
    {"a_{0} = 0, a_{1} = 1, a_{n + 1} = a_{n} + a_{n - 1}, n \\ge 3 \\implies a_{n} = F_{n}", UNSUPPORTED, 0, NULL},
    {"a_{0} = 1, a_{n + 1} = 2 a_{n}, a_{n + 1} = 3 a_{n} \\implies a_{n} = 2^{n}", UNSUPPORTED, 0, NULL},
    {"a_{0} = 1, a_{n + 1} = a\\, a_{n} \\implies a_{n} > 0", UNSUPPORTED, 0, NULL},
    {"a_{0} = 1, a_{n + 1} = a_{n} + \\frac{1}{n + 1} \\implies a_{n} < 3", UNSUPPORTED, 0, NULL},
    {"a_{0} = 1, a_{n + 1} = a_{n}^{2} \\implies \\ln a_{n} = 2^{n} \\ln a_{0}", UNSUPPORTED, 0, NULL},

    /* Veri: che cosa c'è nella dimostrazione. */
    {"a_{0} = 1, a_{n + 1} = 2 a_{n} + 1 \\implies a_{n} = 2^{n + 1} - 1", PROVED, 0, "the rule becomes $2^{n + 1 + 1} - 1 = 2 \\cdot \\left(2^{n + 1} - 1\\right) + 1$"},
    {"a_{0} = 1, a_{n + 1} = 2 a_{n} + 1 \\implies a_{n} = 2^{n + 1} - 1", PROVED, 0, "\\emph{First value}, $n = 0$"},
    {"a_{0} = 2, a_{1} = 5, a_{n + 2} = 5 a_{n + 1} - 6 a_{n} \\implies 2^{n} + 3^{n} = a_{n}", PROVED, 0, "The terms from $a_{2}$ to $a_{7}$"},
    {"a_{0} = 0, a_{n + 1} = \\sqrt{6 + a_{n}} \\implies a_{n} < 3 \\land a_{n} < a_{n + 1}", PROVED, 0, "Here $I(a)$ is $a < 3 \\land a \\ge 0$"},
    {"a_{0} = 2, a_{n + 1} = \\frac{a_{n}}{2} + \\frac{1}{a_{n}} \\implies a_{n}^{2} \\ge 2 \\land a_{n + 1} \\le a_{n}", PROVED, 0, "\\land a > 0$"},
    {"a_{0} = 1, a_{n + 1} = \\frac{a_{n}}{2} + 1 \\implies a_{n} < 2", PROVED, 0, "Here $I(a)$ is $a < 2$."},
};

/* Controllo 4: i verdetti attesi, uno per uno. */
static int check_expected(void) {
    char summary[128];
    int count = (int)(sizeof expected / sizeof *expected), passed = 0;
    for (int i = 0; i < count; i++) {
        const Expected *e = &expected[i];
        Verdict verdict = prove(e->formula, 'n', 0, 0);
        const char *problem = NULL;
        if (verdict.outcome != e->outcome) problem = "unexpected verdict";
        else if (e->outcome == REFUTED && verdict.witness != e->witness) problem = "refuted at an unexpected index";
        else if (e->outcome == PROVED && (!verdict.proof || !strstr(verdict.proof, e->text))) problem = "the written proof lacks an expected sentence";
        else if (e->outcome == PROVED && (verdict.certificate || !verdict.reason)) problem = "the verdict must say why there is no certificate";
        if (!problem) { passed++; continue; }
        printf("  %s: %s\n    got: ", problem, e->formula);
        verdict_print(stdout, &verdict);
        putchar('\n');
    }
    snprintf(summary, sizeof summary, "%d of %d expected verdicts on sequences", passed, count);
    return check_result(passed == count, summary);
}

/* Un \item scritto a parole e ciò che il lettore ne deve ricavare. */
typedef struct {
    const char *item;
    const char *formula;  /* la formula attesa, o NULL se l'\item va rifiutato */
    int conditional;      /* 1 se una parte del testo non è stata letta */
    int has_base;
    long long base;
} Reading;

static const Reading readings[] = {
    {" Sia $a_0=0$, $a_1=\\sin x$ e $a_{n+1}=2\\cos x\\,a_n-a_{n-1}$ per $n\\ge1$. Allora $a_n=\\sin(nx)$ per ogni $n\\ge0$.",
     "a_0=0, a_1=\\sin x, a_{n+1}=2\\cos x\\,a_n-a_{n-1}, n \\ge 1 \\implies a_n=\\sin(nx)", 0, 1, 0},
    {" Let $a_0=4$ and $a_{n+1}=\\frac{a_n}{2}+1$. Then $a_n>2$ and $a_{n+1}<a_n$ for every integer $n\\ge0$.",
     "a_0=4, a_{n+1}=\\frac{a_n}{2}+1 \\implies a_n>2 \\land a_{n+1}<a_n", 0, 1, 0},
    {" Con $a_0=0$, $a_{n+1}=\\sqrt{2+a_n}$, vale $a_n<2$ per ogni $n\\ge0$.",
     "a_0=0, a_{n+1}=\\sqrt{2+a_n} \\implies a_n<2", 0, 1, 0},
    {" Sia $0\\le x\\le\\pi$, $a_0=\\cos x$ e $a_{n+1}=\\sqrt{\\dfrac{1+a_n}{2}}$. Allora $a_n=\\cos\\left(\\dfrac{x}{2^n}\\right)$ per ogni $n\\ge0$.",
     "a_0=\\cos x, a_{n+1}=\\sqrt{\\dfrac{1+a_n}{2}} \\implies a_n=\\cos\\left(\\dfrac{x}{2^n}\\right)", 1, 1, 0},
    {" Sia $a_0>0$ e $a_{n+1}=a_n^2$. Allora $\\ln a_n=2^n\\ln a_0$ per ogni $n\\ge0$.",
     "a_{n+1}=a_n^2 \\implies \\ln a_n=2^n\\ln a_0", 1, 1, 0},
    {" Let $a_0=1$ and $a_{n+1}=2a_n$. Then $a_n=2^n$.", "a_0=1, a_{n+1}=2a_n \\implies a_n=2^n", 0, 0, 0},
    {" Let $a_0=1$ and $a_{n+1}=2a_n$. Then $a_n=2^n$ if $n$ is even.", "a_0=1, a_{n+1}=2a_n \\implies a_n=2^n", 1, 0, 0},

    /* Non sono enunciati di questa forma: restano prosa. */
    {" Per ogni $n\\ge0$ il numero $\\left(1+\\sqrt3\\right)^n+\\left(1-\\sqrt3\\right)^n$ è un intero pari.", NULL, 0, 0, 0},
    {" Sia $a_0=1$ e $a_{n+1}=2a_n$.", NULL, 0, 0, 0},
    {" Sia $a_0=1$ e $a_{n+1}=2a_n$. Allora esiste $n$ con $a_n>100$.", NULL, 0, 0, 0},
    {" Let $x>0$. Then $x^2>0$.", NULL, 0, 0, 0},
};

/* Controllo 5: la lettura degli \item scritti a parole. */
static int check_readings(void) {
    char summary[128];
    int count = (int)(sizeof readings / sizeof *readings), passed = 0;
    for (int i = 0; i < count; i++) {
        const Reading *r = &readings[i];
        Item item;
        const char *problem = read_item(r->item, r->item + strlen(r->item), &item);
        int fine = r->formula ? !problem && strcmp(item.formula, r->formula) == 0 && item.conditional == r->conditional &&
                                item.has_base == r->has_base && (!r->has_base || item.base == r->base)
                              : problem != NULL;
        if (fine) { passed++; continue; }
        printf("  item read in an unexpected way:%s\n    got: %s\n", r->item, problem ? problem : item.formula);
    }
    snprintf(summary, sizeof summary, "%d of %d items in words read as expected", passed, count);
    return check_result(passed == count, summary);
}

int main(void) {
    engine_init();

    /* Tutti i controlli vengono eseguiti anche se uno fallisce, così il
     * resoconto è completo. */
    int passed = check_all_proved("tests/recurrences/sequences_by_recurrence.tex");
    passed = check_false_variants("tests/recurrences/sequences_by_recurrence.tex") && passed;
    passed = check_none_proved("tests/recurrences/false_sequences_by_recurrence.tex") && passed;
    passed = check_expected() && passed;
    passed = check_readings() && passed;

    /* Nessun passo delle dimostrazioni scritte deve aver fallito il suo
     * controllo indipendente. */
    passed = check_result(unjustified_steps == 0, "every step of the written proofs passed its independent check") && passed;
    return passed ? 0 : 1;
}
