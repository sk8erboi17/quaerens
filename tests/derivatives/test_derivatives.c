/* Test di dominio: derivate n-esime e di ordine fisso (capitolo 27 di
 * main.c, e il segno di derivata letto dal capitolo 21 di parser.c).
 *
 * Quattro controlli; i primi due sono descritti in tests/statement_checks.h:
 *
 *   1. ogni enunciato di nth_derivatives.tex viene dimostrato, con la
 *      dimostrazione scritta;
 *   2. nessun enunciato di false_nth_derivatives.tex viene dimostrato: è
 *      l'antiregressione contro le false dimostrazioni;
 *   3. alcuni enunciati ricevono proprio il verdetto atteso;
 *   4. il controllo con la serie di Taylor (27h), da solo, senza derive():
 *      una funzione e le sue derivate scritte a mano devono essere
 *      confermate nei punti dove la funzione è definita, e solo lì; una
 *      derivata scritta sbagliata, come la darebbe una regola rotta di
 *      derive(), deve essere smentita.
 *
 * Gli enunciati sono dati: il motore non ne conosce nessuno in anticipo.
 * Che gli enunciati dei due file .tex siano veri e falsi l'ha controllato
 * anche l'oracolo indipendente di tools/fuzz/fuzz_derivatives.py (modalità
 * check).
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

    /* Con la serie di Taylor le somme e i prodotti si calcolano con i
     * numeri: la regola 27g si controlla, e un errore diventa FALSO. Prima
     * erano tutti NON DIMOSTRATO. */
    {"\\frac{d}{dx} \\sum_{k=0}^{3} x^{k} = 1 + 2x + 3x^{2}", 0, 0, PROVED, 0},
    {"\\frac{d}{dx} \\sum_{k=0}^{3} x^{k} = 1 + 2x + 3x^{3}", 0, 0, REFUTED, 1},
    {"\\frac{d^{n}}{dx^{n}} \\sum_{j=0}^{3} x^{j} = 0", 1, 4, PROVED, 0},
    {"\\frac{d^{n}}{dx^{n}} \\sum_{j=0}^{3} x^{j} = 0", 1, 3, REFUTED, 3},
    {"\\frac{d^{n}}{dx^{n}} (1+x)^{\\frac{1}{2}} = \\left(\\prod_{j=0}^{n-1} \\left(\\frac{1}{2} - j\\right)\\right) (1+x)^{\\frac{1}{2} - n}", 1, 0, PROVED, 0},
    {"\\frac{d^{n}}{dx^{n}} (1+x)^{\\frac{1}{2}} = \\left(\\prod_{j=0}^{n} \\left(\\frac{1}{2} - j\\right)\\right) (1+x)^{\\frac{1}{2} - n}", 1, 0, REFUTED, 0},

    /* Un errore che si annulla per n <= 4, oltre gli ordini confrontati uno
     * per uno: lo ferma il passo induttivo, non diventa FALSO. */
    {"\\frac{d^{n}}{dx^{n}} \\left(x e^{-x}\\right) = (-1)^{n} (x - n) e^{-x} + n(n-1)(n-2)(n-3)(n-4)", 1, 0, NOT_PROVED, 0},

    /* Trovato da fuzz_derivatives.py: la derivata di ordine 4, non
     * semplificata, è più lunga di quanto reread_equal() sa rileggere, e la
     * rilettura mancata contava come un passo sbagliato. Ora la formula non
     * si scrive; il controllo finale (nessun passo scartato) lo sorveglia. */
    {"\\frac{d^{n}}{dx^{n}} \\frac{1}{\\left(\\frac{1}{2} x - 2\\right)^{2}} = \\frac{(-1)^{n} (n + 1)! \\left(\\frac{1}{2}\\right)^{n}}{\\left(\\frac{1}{2} x - 2\\right)^{n + 2}}", 1, 4, PROVED, 0},

    /* Il segno di Leibniz è una derivata, non una frazione di lettere. */
    {"\\frac{d}{dx} 5 = \\frac{5}{x}", 0, 0, REFUTED, 1},
    {"\\frac{d}{dx} x^{2} = 2x", 0, 0, PROVED, 0},
    {"\\frac{d^{2}}{dx^{2}} x^{3} = 3x^{2}", 0, 0, REFUTED, 2},

    /* Il segno di derivata senza una funzione non si legge: come frazione
     * delle lettere d e x è la porta da cui passavano le false derivate
     * (vedi tests/derivatives/test_derivative_reading.c). */
    {"\\frac{d}{dx} = \\frac{1}{x}", 0, 0, UNSUPPORTED, 0},
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

/* Una funzione e le sue derivate di ordine 1, 2, ..., scritte a mano, per il
 * controllo 4. Ognuna sbagliata imita una regola rotta di derive(). */
typedef struct {
    const char *chain[MAX_JET + 2];  /* f, f', f'', ...; l'elenco finisce con NULL */
    int wrong;                       /* 1 se una delle derivate scritte è sbagliata */
    int agree;                       /* per quelle giuste: i punti di prova (bit p) in cui
                                        ogni derivata deve essere confermata */
} HandChain;

/* I punti di prova sono 3/10, 7/5 e -9/10: 7 vuol dire tutti e tre, 3 solo
 * i due positivi, dove sono definiti ln x, la radice di x, x^x. */
static const HandChain chains[] = {
    /* Giuste: fino all'ordine più alto, e per ogni funzione che la serie
     * di Taylor tratta, anche dove derive() non arriva (x^x, |x - 2|). */
    {{"e^{x}", "e^{x}", "e^{x}", "e^{x}", "e^{x}", "e^{x}", "e^{x}", "e^{x}", NULL}, 0, 7},
    {{"\\sin x", "\\cos x", "-\\sin x", "-\\cos x", "\\sin x", "\\cos x", "-\\sin x", "-\\cos x", NULL}, 0, 7},
    {{"\\ln(1 + x)", "\\frac{1}{1 + x}", "-\\frac{1}{(1 + x)^{2}}", "\\frac{2}{(1 + x)^{3}}", "-\\frac{6}{(1 + x)^{4}}", "\\frac{24}{(1 + x)^{5}}", NULL}, 0, 7},
    {{"\\ln x", "\\frac{1}{x}", "-\\frac{1}{x^{2}}", NULL}, 0, 3},
    {{"\\tan x", "1 + \\tan^{2} x", "2 \\tan x (1 + \\tan^{2} x)", NULL}, 0, 7},
    {{"\\cot x", "-1 - \\cot^{2} x", "2 \\cot x (1 + \\cot^{2} x)", NULL}, 0, 7},
    {{"\\arctan x", "\\frac{1}{1 + x^{2}}", "-\\frac{2x}{(1 + x^{2})^{2}}", NULL}, 0, 7},
    {{"\\sqrt{x}", "\\frac{1}{2\\sqrt{x}}", "-\\frac{1}{4 x \\sqrt{x}}", NULL}, 0, 3},
    {{"\\sqrt[3]{x + 2}", "\\frac{1}{3 (x + 2)^{\\frac{2}{3}}}", "-\\frac{2}{9 (x + 2)^{\\frac{5}{3}}}", NULL}, 0, 7},
    {{"\\log_{2} x", "\\frac{1}{x \\ln 2}", "-\\frac{1}{x^{2} \\ln 2}", NULL}, 0, 3},
    {{"2^{x}", "2^{x} \\ln 2", "2^{x} (\\ln 2)^{2}", NULL}, 0, 7},
    {{"x^{x}", "x^{x} (\\ln x + 1)", NULL}, 0, 3},
    {{"\\sum_{j=0}^{4} x^{j}", "\\sum_{j=1}^{4} j x^{j - 1}", "\\sum_{j=2}^{4} j (j - 1) x^{j - 2}", NULL}, 0, 7},
    {{"\\prod_{j=1}^{3} (x + j)", "(x + 2)(x + 3) + (x + 1)(x + 3) + (x + 1)(x + 2)", NULL}, 0, 7},
    {{"\\left|x - 2\\right| x", "2 - 2x", NULL}, 0, 7},
    {{"x e^{-2x}", "(1 - 2x) e^{-2x}", "(4x - 4) e^{-2x}", NULL}, 0, 7},
    {{"\\frac{1}{1 - x}", "\\frac{1}{(1 - x)^{2}}", "\\frac{2}{(1 - x)^{3}}", NULL}, 0, 7},

    /* Sbagliate: le regole rotte dell'esperimento del bugiardo, e due
     * errori che compaiono solo all'ordine 3. */
    {{"\\tan x", "\\frac{1}{\\cos x}", NULL}, 1, 0},
    {{"\\cos x", "\\sin x", NULL}, 1, 0},
    {{"\\ln(2x + 1)", "\\frac{1}{2x + 1}", NULL}, 1, 0},
    {{"\\frac{x}{1 + x^{2}}", "\\frac{1 + 3x^{2}}{(1 + x^{2})^{2}}", NULL}, 1, 0},
    {{"x^{5}", "5 x^{6}", NULL}, 1, 0},
    {{"e^{3x}", "e^{3x}", NULL}, 1, 0},
    {{"x \\sin x", "\\sin x", NULL}, 1, 0},
    {{"\\arctan x", "\\frac{1}{1 - x^{2}}", NULL}, 1, 0},
    {{"\\sqrt{x}", "\\frac{\\sqrt{x}}{x}", NULL}, 1, 0},
    {{"3^{x}", "3^{x}", NULL}, 1, 0},
    {{"\\sin(2x)", "\\cos(2x)", NULL}, 1, 0},
    {{"\\cot x", "\\frac{1}{\\sin^{2} x}", NULL}, 1, 0},
    {{"\\log_{3} x", "\\frac{1}{x}", NULL}, 1, 0},
    {{"x^{2} - x", "2x + 1", NULL}, 1, 0},
    {{"5 x^{3}", "3 x^{2}", NULL}, 1, 0},
    {{"\\sum_{j=0}^{3} x^{j}", "\\sum_{j=0}^{3} x^{j}", NULL}, 1, 0},
    {{"\\sin(x^{2})", "\\cos(x^{2})", NULL}, 1, 0},
    {{"-\\cos x", "-\\sin x", NULL}, 1, 0},
    {{"e^{2x}", "2 e^{2x}", "4 e^{2x}", "6 e^{2x}", NULL}, 1, 0},
    {{"\\sin x", "\\cos x", "-\\sin x", "\\cos x", NULL}, 1, 0},
};

/* Controllo 4: derivatives_check() su ogni catena. Una giusta non deve mai
 * essere smentita, e ogni sua derivata deve essere confermata proprio nei
 * punti attesi: dove la funzione non è definita la serie di Taylor deve
 * dire "non so", anche se la derivata scritta lì si calcola (1/x in
 * x = -9/10). Una sbagliata deve essere smentita. */
static int check_taylor(void) {
    char summary[128];
    int count = (int)(sizeof chains / sizeof *chains), passed = 0;
    for (int i = 0; i < count; i++) {
        const HandChain *c = &chains[i];
        Node *sampled[MAX_JET + 1];
        int order = -1, readable = 1;
        arena_used = arena_keep;
        failure = NULL;
        for (int k = 0; c->chain[k] && k <= MAX_JET; k++) {
            Node *tree = parse_expression(c->chain[k]);
            readable = readable && !syntax_error;
            sampled[k] = sample_tree(tree, 'x', 0, 0);
            order = k;
        }
        DerivativeCheck check;
        derivatives_check(&check, sampled, order, 'x');
        int good = readable && !failure && check.wrong == c->wrong;
        for (int k = 1; good && !c->wrong && k <= order; k++) good = check.agree[k] == c->agree;
        if (good) { passed++; continue; }
        printf("  unexpected numeric check of the derivatives of %s: %s", c->chain[0], readable ? "" : "unreadable, ");
        printf("wrong = %d, confirmed at", check.wrong);
        for (int k = 1; k <= order; k++) printf(" %d", check.agree[k]);
        putchar('\n');
    }
    snprintf(summary, sizeof summary, "%d of %d chains of derivatives checked as expected against their Taylor series", passed, count);
    return check_result(passed == count, summary);
}

int main(void) {
    engine_init();

    /* Tutti i controlli vengono eseguiti anche se uno fallisce, così il
     * resoconto è completo. */
    int passed = check_all_proved("tests/derivatives/nth_derivatives.tex");
    passed = check_none_proved("tests/derivatives/false_nth_derivatives.tex") && passed;
    passed = check_expected() && passed;
    passed = check_taylor() && passed;

    /* Nessun passo delle dimostrazioni scritte deve aver fallito il suo
     * controllo indipendente. */
    passed = check_result(unjustified_steps == 0, "every step of the written proofs passed its independent check") && passed;
    return passed ? 0 : 1;
}
