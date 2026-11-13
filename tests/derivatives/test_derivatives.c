/* Test di dominio: derivate n-esime e di ordine fisso (capitolo 27 di
 * main.c, e il segno di derivata letto dal capitolo 21 di parser.c).
 *
 * Tre controlli; i primi due sono descritti in tests/statement_checks.h:
 *
 *   1. ogni enunciato di nth_derivatives.tex viene dimostrato, con la
 *      dimostrazione scritta;
 *   2. nessun enunciato di false_nth_derivatives.tex viene dimostrato: è
 *      l'antiregressione contro le false dimostrazioni;
 *   3. alcuni enunciati ricevono proprio il verdetto atteso.
 *
 * Gli enunciati sono dati: il motore non ne conosce nessuno in anticipo.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_derivatives \
 *         tests/derivatives/test_derivatives.c && /tmp/test_derivatives
 */

#include "../statement_checks.h"

/* Un enunciato con il verdetto atteso. */
typedef struct {
    const char *formula;
    int has_base;       /* 1 se il caso base è `base` */
    long long base;
    Outcome outcome;    /* il verdetto atteso */
    long long witness;  /* REFUTED per induzione: l'ordine n atteso */
} Expected;

static const Expected expected[] = {
    {"\\frac{d^{n}}{dx^{n}} \\sin x = \\sin\\left(x + \\frac{n\\pi}{2}\\right)", 1, 0, PROVED, 0},
    {"\\frac{d^{n}}{dx^{n}} \\sin x = \\sin\\left(x + \\frac{n\\pi}{3}\\right)", 1, 0, REFUTED, 1},
    {"\\frac{d^{n}}{dx^{n}} \\ln x = \\frac{(-1)^{n-1}(n-1)!}{x^{n}}", 1, 1, PROVED, 0},
    {"\\frac{d^{n}}{dx^{n}} \\ln x = \\frac{(-1)^{n-1} n!}{x^{n}}", 1, 1, REFUTED, 2},
    {"\\frac{d^{n}}{dx^{n}} e^{2x} = 2^{n} e^{2x} + n(n-1)(n-2)(n-3)(n-4)", 1, 0, NOT_PROVED, 0},
    {"\\frac{d^{n}}{dx^{n}} x^{n} = n!\\,x", 1, 1, UNSUPPORTED, 0},

    /* Il segno di Leibniz è una derivata, non una frazione di lettere. */
    {"\\frac{d}{dx} 5 = \\frac{5}{x}", 0, 0, REFUTED, 1},
    {"\\frac{d}{dx} x^{2} = 2x", 0, 0, PROVED, 0},
    {"\\frac{d^{2}}{dx^{2}} x^{3} = 3x^{2}", 0, 0, REFUTED, 2},

    /* Ciò che non è scritto come una derivata resta algebra. */
    {"\\frac{d}{dx} = \\frac{1}{x}", 0, 0, PROVED, 0},
};

/* Controllo 3: i verdetti attesi, uno per uno. */
static int check_expected(void) {
    char summary[128];
    int count = (int)(sizeof expected / sizeof *expected), passed = 0;
    for (int i = 0; i < count; i++) {
        const Expected *e = &expected[i];
        Verdict verdict = prove(e->formula, 'n', e->has_base, e->base);
        if (verdict.outcome == e->outcome && (e->outcome != REFUTED || verdict.witness == e->witness) &&
            (e->outcome != PROVED || (verdict.proof && verdict.proof[0]))) { passed++; continue; }
        printf("  unexpected verdict: %s\n    got: ", e->formula);
        verdict_print(stdout, &verdict);
        putchar('\n');
    }
    snprintf(summary, sizeof summary, "%d of %d expected verdicts on derivatives", passed, count);
    return check_result(passed == count, summary);
}

int main(void) {
    engine_init();

    /* Tutti i controlli vengono eseguiti anche se uno fallisce, così il
     * resoconto è completo. */
    int passed = check_all_proved("tests/derivatives/nth_derivatives.tex");
    passed = check_none_proved("tests/derivatives/false_nth_derivatives.tex") && passed;
    passed = check_expected() && passed;

    /* Nessun passo delle dimostrazioni scritte deve aver fallito il suo
     * controllo indipendente. */
    passed = check_result(unjustified_steps == 0, "every step of the written proofs passed its independent check") && passed;
    return passed ? 0 : 1;
}
