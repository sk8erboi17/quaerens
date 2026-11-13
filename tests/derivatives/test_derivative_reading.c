/* Test di dominio: come si legge il segno di derivata (paragrafo 9d e
 * starts_with_derivative() di parser.c, capitolo 27 di main.c).
 *
 * Tre controlli; i primi due sono descritti in tests/statement_checks.h:
 *
 *   1. ogni enunciato di derivative_sign_reading.tex viene dimostrato: sono
 *      derivate vere scritte come nei libri, con \displaystyle davanti, la
 *      funzione tra \left( e \right) e argomenti come \sin 2x;
 *   2. nessun enunciato di false_derivative_sign_reading.tex viene
 *      dimostrato: sono derivate false che, lette come frazione delle
 *      lettere d e x, sarebbero uguaglianze vere. È l'antiregressione contro
 *      l'errore che le dimostrava. Quelle con il segno in mezzo
 *      all'espressione oggi si leggono come derivate (27j) e risultano non
 *      dimostrate; prima erano rifiutate dal lettore;
 *   3. alcuni enunciati ricevono proprio il verdetto atteso.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_derivative_reading \
 *         tests/derivatives/test_derivative_reading.c && /tmp/test_derivative_reading
 */

#include "../statement_checks.h"

/* Un enunciato con il verdetto atteso. */
typedef struct {
    const char *formula;
    Outcome outcome;    /* il verdetto atteso */
} Expected;

static const Expected expected[] = {
    /* Con \displaystyle davanti il segno è ancora una derivata, e la
     * derivata sbagliata è smentita, non dimostrata come algebra. */
    {"\\displaystyle \\frac{d}{dx} x^{2} = x", REFUTED},
    {"\\displaystyle \\frac{d}{dx} 5 = \\frac{5}{x}", REFUTED},
    {"\\displaystyle \\frac{d}{dx} x^{2} = 2x", PROVED},

    /* In mezzo a un'espressione il segno di derivata è la derivata del
     * term che lo segue (9d, 27j): qui x^2 + 2x = x^2 + x, che è falso
     * tranne in x = 0. Prima il lettore rifiutava il segno, e il verdetto
     * era NON SUPPORTATO; ora l'enunciato si legge come scritto e non si
     * dimostra. Che non si dimostri è ciò che conta: letto come frazione
     * delle lettere d e x sarebbe vero. L'induzione del capitolo 13, che lo
     * riceve perché non ha quantificatori, non dice FALSO per un enunciato
     * senza n: la x è un parametro. */
    {"x^{2} + \\frac{d}{dx} x^{2} = x^{2} + x", NOT_PROVED},
    {"\\frac{d}{dx} = \\frac{1}{x}", UNSUPPORTED},

    /* Con i quantificatori la tabella dei segni lo decide: l'insieme dei
     * numeri in cui la falsa uguaglianza vale è il solo 0. */
    {"\\forall x \\in \\mathbb{R}\\ \\left(x^{2} + \\frac{d}{dx}\\left( x^{2} \\right) = x^{2} + x\\right)", REFUTED},
    {"\\forall x \\in \\mathbb{R}\\ \\left(x^{2} + \\frac{d}{dx}\\left( x^{2} \\right) = x^{2} + 2x\\right)", PROVED},

    /* Dentro un'espressione l'ordine di una derivata è un numero. */
    {"\\forall x \\in \\mathbb{R}\\ \\left(\\frac{d^{n}}{dx^{n}}\\left( x \\right) = 1\\right)", UNSUPPORTED},

    /* \sin 2x è sin(2x); una frazione con d sopra e un numero sotto resta
     * una frazione. */
    {"\\frac{d}{dx} \\sin 2x = 2\\cos 2x", PROVED},
    {"\\frac{d}{dx} \\sin 2x = \\cos 2x", REFUTED},
    {"\\frac{d}{2} \\cdot 2 = d", PROVED},
};

/* Controllo 3: i verdetti attesi, uno per uno. */
static int check_expected(void) {
    char summary[128];
    int count = (int)(sizeof expected / sizeof *expected), passed = 0;
    for (int i = 0; i < count; i++) {
        const Expected *e = &expected[i];
        Verdict verdict = prove(e->formula, 'n', 0, 0);
        if (verdict.outcome == e->outcome) { passed++; continue; }
        printf("  unexpected verdict: %s\n    got: ", e->formula);
        verdict_print(stdout, &verdict);
        putchar('\n');
    }
    snprintf(summary, sizeof summary, "%d of %d expected verdicts on the reading of the derivative sign", passed, count);
    return check_result(passed == count, summary);
}

int main(void) {
    engine_init();

    /* Tutti i controlli vengono eseguiti anche se uno fallisce, così il
     * resoconto è completo. */
    int passed = check_all_proved("tests/derivatives/derivative_sign_reading.tex");
    passed = check_none_proved("tests/derivatives/false_derivative_sign_reading.tex") && passed;
    passed = check_expected() && passed;
    passed = check_result(unjustified_steps == 0, "every step of the written proofs passed its independent check") && passed;
    return passed ? 0 : 1;
}
