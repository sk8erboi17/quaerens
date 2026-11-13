/* Test di dominio: disuguaglianze dimostrate per induzione (capitolo 25 di
 * main.c).
 *
 * Quattro controlli; i primi due sono descritti in tests/statement_checks.h:
 *
 *   1. ogni enunciato di induction_inequalities.tex viene dimostrato, con la
 *      dimostrazione scritta;
 *   2. nessun enunciato di false_induction_inequalities.tex viene
 *      dimostrato: è l'antiregressione contro le false dimostrazioni;
 *   3. alcuni enunciati falsi ricevono proprio il verdetto FALSO, con l'n in
 *      cui non valgono, e uno vero su tre ha il passo che parte dall'n
 *      giusto;
 *   4. un verdetto DIMOSTRATO dichiara che il certificato manca e che il
 *      passo è deciso dalla tabella dei segni, e la dimostrazione scritta
 *      contiene la tabella.
 *
 * Gli enunciati sono dati: il motore non ne conosce nessuno in anticipo.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_induction_inequalities \
 *         tests/sums_products_inequalities/test_induction_inequalities.c \
 *         && /tmp/test_induction_inequalities
 */

#include "../statement_checks.h"

/* Un enunciato con il verdetto atteso. */
typedef struct {
    const char *formula;
    int has_base;       /* 1 se il caso base è `base`, 0 se lo sceglie il motore */
    long long base;
    Outcome outcome;    /* il verdetto atteso */
    long long witness;  /* REFUTED: l'n atteso come controesempio */
    const char *text;   /* PROVED: un testo che la dimostrazione deve contenere, o NULL */
} Expected;

static const Expected expected[] = {
    /* Falsi in un solo n, o veri solo da un n più grande di quello detto. */
    {"2^{n} \\ge n^{2}", 0, 0, REFUTED, 3, NULL},
    {"2^{n} > n^{2}", 1, 4, REFUTED, 4, NULL},
    {"3^{n} > n^{3}", 0, 0, REFUTED, 3, NULL},
    {"n! \\ge 3^{n}", 1, 6, REFUTED, 6, NULL},
    {"2^{n} \\ge n^{3}", 1, 9, REFUTED, 9, NULL},
    {"\\sum_{k=1}^{n} \\frac{1}{k^{2}} \\le \\frac{5}{3} - \\frac{1}{n}", 1, 3, REFUTED, 3, NULL},
    {"\\prod_{k=1}^{n} \\frac{2k - 1}{2k} \\ge \\frac{1}{\\sqrt{3n + 1}}", 0, 0, REFUTED, 2, NULL},
    {"\\sum_{k=1}^{n} \\frac{1}{\\sqrt{k}} > \\sqrt{n}", 0, 0, REFUTED, 1, NULL},

    /* Falsi oltre i valori calcolati, o non definiti più avanti: non c'è un
     * controesempio da mostrare, ma non vanno dimostrati. */
    {"n^{3} + 2000 \\ge 2^{n}", 0, 0, NOT_PROVED, 0, NULL},
    {"\\left(n - 13\\right)\\left(n - 15\\right) \\ge 0", 0, 0, NOT_PROVED, 0, NULL},
    {"\\sum_{k=1}^{n} \\frac{1}{k^{2}} \\le \\frac{8}{5}", 0, 0, NOT_PROVED, 0, NULL},
    {"\\frac{n - 50}{n - 50} + 2^{n} > n + 1", 0, 0, NOT_PROVED, 0, NULL},

    /* Veri, ma non con questo metodo: il secondo membro non cresce con n, o
     * la somma compare al quadrato. Non dimostrati, e mai falsi. */
    {"\\sum_{k=1}^{n} \\frac{1}{k^{2}} < 2", 0, 0, NOT_PROVED, 0, NULL},
    {"\\left(\\sum_{k=1}^{n} \\sqrt{k}\\right)^{2} \\le \\frac{n^{2}\\left(n + 1\\right)}{2}", 0, 0, NOT_PROVED, 0, NULL},

    /* Con un parametro il passo ha due variabili: fuori portata. */
    {"\\left(1 + x\\right)^{n} \\ge 1 + n x", 0, 0, UNSUPPORTED, 0, NULL},

    /* Veri: da dove parte il passo, e che cosa c'è nella dimostrazione. */
    {"3^{n} \\ge n^{3}", 0, 0, PROVED, 0, "Assume the statement for some $n \\ge 3$"},
    {"3^{n} \\ge 2n^{2} + 1", 1, 0, PROVED, 0, "Assume the statement for some $n \\ge 1$"},
    {"\\left(n - 3\\right)\\left(n - 4\\right) \\ge 0", 0, 0, PROVED, 0, "For every $n \\ge 3$ we prove the statement for $n + 1$"},
    {"2^{n} \\ge n^{3}", 1, 10, PROVED, 0, "\\emph{Base case}, $n = 10$"},
    {"n! > 2^{n}", 1, 4, PROVED, 0, "The factor $2^{n}$ is positive"},
    {"\\sum_{k=1}^{n} \\frac{1}{\\sqrt{k}} \\ge \\sqrt{n}", 0, 0, PROVED, 0, "\\textbf{Sign table.}"},
    {"\\sum_{k=1}^{n} \\frac{1}{\\sqrt{k}} \\ge \\sqrt{n}", 0, 0, PROVED, 0, "D_{n + 1} = D_{n} + \\left("},
    {"\\prod_{k=1}^{n} \\frac{2k - 1}{2k} \\ge \\frac{1}{2\\sqrt{n}}", 0, 0, PROVED, 0, "so the factor of $D_{n + 1}$ is not zero"},
};

/* Controlli 3 e 4: i verdetti attesi, uno per uno. */
static int check_expected(void) {
    char summary[128];
    int count = (int)(sizeof expected / sizeof *expected), passed = 0;
    for (int i = 0; i < count; i++) {
        const Expected *e = &expected[i];
        Verdict verdict = prove(e->formula, 'n', e->has_base, e->base);
        const char *problem = NULL;
        if (verdict.outcome != e->outcome) problem = "unexpected verdict";
        else if (e->outcome == REFUTED && verdict.witness != e->witness) problem = "refuted at an unexpected value of n";
        else if (e->outcome == PROVED && (!verdict.proof || !strstr(verdict.proof, e->text))) problem = "the written proof lacks an expected sentence";
        else if (e->outcome == PROVED && verdict.certificate) problem = "an inequality cannot have a certificate yet";
        else if (e->outcome == PROVED && (!verdict.reason || !strstr(verdict.reason, "tabella dei segni"))) problem = "the verdict does not say that the step was decided by the sign table";
        if (!problem) { passed++; continue; }
        printf("  %s: %s\n    got: ", problem, e->formula);
        verdict_print(stdout, &verdict);
        putchar('\n');
    }
    snprintf(summary, sizeof summary, "%d of %d expected verdicts on inequalities", passed, count);
    return check_result(passed == count, summary);
}

int main(void) {
    engine_init();

    /* Tutti i controlli vengono eseguiti anche se uno fallisce, così il
     * resoconto è completo. */
    int passed = check_all_proved("tests/sums_products_inequalities/induction_inequalities.tex");
    passed = check_none_proved("tests/sums_products_inequalities/false_induction_inequalities.tex") && passed;
    passed = check_expected() && passed;

    /* Nessun passo delle dimostrazioni scritte deve aver fallito il suo
     * controllo indipendente. */
    passed = check_result(unjustified_steps == 0, "every step of the written proofs passed its independent check") && passed;
    return passed ? 0 : 1;
}
