/* Test delle costanti nei polinomi per esponenziali (30e): il fattore
 * costante e il monomio successivo devono restare separati nel LaTeX.
 * Prima \pi seguito da x diventava il comando inesistente \pix: la
 * primitiva giusta non si rileggeva, e l'integrale restava senza risposta.
 *
 * Il terzo conto e dato dalle primitive indicate nei dati: si derivano con
 * (x^m)' = m x^{m-1}, (e^x)' = e^x, (sin x)' = cos x. Le varianti false
 * cambiano un fattore, non una costante additiva che resterebbe lecita.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_constant_factors \
 *         tests/integrals/test_constant_factors.c && /tmp/test_constant_factors
 */

#include "../statement_checks.h"

/* Una richiesta di integrale, con un risultato vero ricavato a mano e una
 * variante falsa. Le stringhe sono dati, mai regole del motore. */
typedef struct {
    const char *request;  /* L'integrale da calcolare. */
    const char *answer;   /* Il valore o una primitiva, con + c se indefinito. */
    const char *wrong;    /* Un valore o una primitiva con un fattore errato. */
} ConstantCase;

int main(void) {
    static const ConstantCase cases[] = {
        {"\\int_0^1 \\pi x\\,dx", "\\frac{\\pi}{2}", "\\pi"},
        {"\\int \\pi x\\,dx", "\\frac{\\pi x^2}{2} + c", "\\pi x^2 + c"},
        {"\\int \\pi x^3\\,dx", "\\frac{\\pi x^4}{4} + c", "\\frac{\\pi x^4}{3} + c"},
        {"\\int \\pi e^x\\,dx", "\\pi e^x + c", "2\\pi e^x + c"},
        {"\\int \\pi x e^x\\,dx", "\\pi(x - 1)e^x + c", "\\pi(x + 1)e^x + c"},
        {"\\int \\pi \\cos x\\,dx", "\\pi\\sin x + c", "2\\pi\\sin x + c"},
        {"\\int e x\\,dx", "\\frac{e x^2}{2} + c", "e x^2 + c"},
        {"\\int \\sqrt{2} x\\,dx", "\\frac{\\sqrt{2} x^2}{2} + c", "\\sqrt{2} x^2 + c"},
    };
    int passed = 1;
    engine_init();
    for (size_t i = 0; i < sizeof cases / sizeof *cases; i++) {
        char statement[MAX_FORMULA], answer[MAX_FORMULA];
        Verdict v = prove(cases[i].request, 'n', 0, 0);
        int solved = v.outcome == SOLVED && v.detail;
        passed = check_result(solved, "a constant factor does not prevent finding and rereading an antiderivative") && passed;
        if (solved) {
            snprintf(answer, sizeof answer, "%s", v.detail);
            snprintf(statement, sizeof statement, "%s = %s", cases[i].request, answer);
            v = prove(statement, 'n', 0, 0);
            passed = check_result(v.outcome == PROVED && v.certificate, "the written answer is proved with an accepted independent certificate") && passed;
        }
        snprintf(statement, sizeof statement, "%s = %s", cases[i].request, cases[i].answer);
        v = prove(statement, 'n', 0, 0);
        passed = check_result(v.outcome == PROVED && v.certificate, "the independently derived result is certified") && passed;
        snprintf(statement, sizeof statement, "%s = %s", cases[i].request, cases[i].wrong);
        v = prove(statement, 'n', 0, 0);
        passed = check_result(v.outcome == REFUTED, "a wrong constant factor is refuted") && passed;
    }
    passed = check_result(unjustified_steps == 0, "every primitive is reread and passes its independent checks") && passed;
    return passed ? 0 : 1;
}
