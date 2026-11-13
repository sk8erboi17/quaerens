/* Test di dominio: integrali. Primitive da verificare e da trovare,
 * integrali definiti da verificare e da calcolare (capitolo 30 di main.c,
 * paragrafo 9j di parser.c).
 *
 * Nove controlli. I primi quattro sono quelli di tests/statement_checks.h:
 *
 *   1. ogni enunciato di book_integrals.tex viene dimostrato: una primitiva
 *      o un valore stampati su un libro (corretti nei due esercizi in cui il
 *      risultato stampato è sbagliato);
 *   2. nessun enunciato di false_integrals.tex viene dimostrato: gli stessi
 *      risultati con un intero cambiato di uno, un segno, un termine in più,
 *      e i due risultati stampati sbagliati;
 *   3. nessun enunciato di book_integrals_beyond.tex viene dichiarato falso:
 *      sono veri, ma il motore non li sa ancora decidere;
 *   4. ogni enunciato di integral_conventions.tex viene dimostrato, e nessuno
 *      di false_integral_conventions.tex: dove può stare il dx, ln con e
 *      senza valore assoluto, gli estremi rovesciati, il teorema
 *      fondamentale usato attraverso un punto in cui la funzione non è
 *      definita;
 *   5. almeno 118 dei 148 enunciati di false_integrals.tex vengono dichiarati
 *      falsi, non solo "non dimostrati": se il numero scendesse il motore
 *      saprebbe riconoscere meno errori di oggi;
 *   6. le richieste di integral_requests.tex: ogni risposta del motore,
 *      riscritta come enunciato, viene dimostrata, e differisce dal
 *      risultato del libro per una costante (per un integrale definito: è
 *      lo stesso numero); almeno 57 delle 80 hanno risposta, e nessuna è un
 *      verdetto diverso da RISOLTO o "non so". Erano 58: una primitiva che
 *      mescola \sqrt[3]{x} e \sqrt[6]{x} non si verifica più da quando
 *      \sqrt[3]{x} è la radice cubica reale (11o; vedi
 *      book_integrals_beyond_about.tex);
 *   7. il lettore degli integrali (9j): che cosa accetta e che cosa rifiuta;
 *   8. le funzioni razionali (30d) prese da sole: ogni ramo dell'algoritmo,
 *      con la primitiva che scrive verificata come ogni altra, e un
 *      denominatore con un fattore irriducibile di terzo grado, che deve
 *      essere rifiutato;
 *   9. nessun passo smentito dai controlli indipendenti (unjustified_steps).
 *
 * I dati vengono da photos/dati_esercizi/integrali.jsonl (i libri, e un
 * oracolo numerico indipendente) e, per i file delle convenzioni, da conti a
 * mano; il motore non ne conosce nessuno in anticipo.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_integrals \
 *         tests/integrals/test_integrals.c && /tmp/test_integrals
 */

#include "../statement_checks.h"

/* Il risultato del libro per una richiesta di integral_requests.tex.
 * Invariante: le voci sono nell'ordine delle richieste del file. */
typedef struct {
    const char *answer;  /* la primitiva (senza + c) o il valore stampati */
    int definite;        /* 1 se la richiesta è un integrale definito */
    int comparable;      /* 0 se il motore non sa confrontare la sua risposta con
                            questa (arcoseno, seno iperbolico, +\infty) */
} Expected;

/* I risultati di photos/dati_esercizi/integrali.jsonl, con \ln al posto di
 * \log e \arctan al posto di \operatorname{arctg}. Sono i risultati
 * corretti: in firenze-5d (voce 15) e ferrara-R7 (voce 25) il libro sbaglia
 * un coefficiente. */
static const Expected expected[] = {
    {"\\ln|x| + \\frac{9}{4x^{4}}", 0, 1},
    {"\\tan x - x", 0, 1},
    {"2\\sqrt{\\ln x}", 0, 1},
    {"\\frac{1}{4}e^{2x^{2} - 1}", 0, 1},
    {"\\frac{1}{2}\\arctan x^{2}", 0, 1},
    {"\\frac{1}{3}x^{3}\\left(\\ln x - \\frac{1}{3}\\right)", 0, 1},
    {"\\frac{1}{3}x\\sinh 3x - \\frac{1}{9}\\cosh 3x", 0, 0},
    {"\\frac{1}{2}\\left(\\frac{x\\sin 2x}{2} + \\frac{\\cos 2x}{4} + \\frac{x^{2}}{2}\\right)", 0, 1},
    {"\\ln|x - 2| - \\frac{4}{x - 2}", 0, 1},
    {"\\ln|x + 2| + \\frac{1}{2}\\ln\\left(x^{2} + 2x + 6\\right) - \\frac{4}{\\sqrt{5}}\\arctan\\frac{x + 1}{\\sqrt{5}}", 0, 1},
    {"2\\ln\\left|\\frac{x - 1}{x - 2}\\right| - \\frac{5}{x - 2}", 0, 1},
    {"\\frac{x^{3}}{3} + 2x - \\ln|x - 2| + 2\\ln|x - 3|", 0, 1},
    {"\\frac{1}{8}\\frac{3x^{3} + 5x}{\\left(x^{2} + 1\\right)^{2}} + \\frac{3}{8}\\arctan x", 0, 1},
    {"2\\ln\\left(e^{x} - 2\\right) - \\ln\\left(e^{2x} + e^{x} + 1\\right)", 0, 1},
    {"6\\sqrt[6]{x} + \\ln x - 3\\ln\\left(1 + \\sqrt[3]{x}\\right) - 6\\arctan\\sqrt[6]{x}", 0, 1},
    {"\\frac{1}{2}\\ln\\left|\\frac{\\tan(x/2)}{\\tan(x/2) - 2}\\right|", 0, 1},
    {"\\frac{x}{2}\\sqrt{x^{2} - 1} - \\frac{1}{2}\\operatorname{arcosh} x", 0, 0},
    {"\\arcsin x + \\sqrt{1 - x^{2}}", 0, 0},
    {"4\\sqrt[4]{x} + 4\\ln\\left|\\sqrt[4]{x} - 1\\right|", 0, 1},
    {"2\\sqrt{e^{x} - 1} - 2\\arctan\\left(\\sqrt{e^{x} - 1}\\right)", 0, 1},
    {"\\arcsin\\left(e^{x}\\right)", 0, 0},
    {"-2\\arctan\\left(2\\cos x - 2\\right)", 0, 1},
    {"\\frac{x^{2}}{2}\\arctan\\left(1 + 16x\\right) + \\frac{1}{512}\\ln\\left(1 + \\left(1 + 16x\\right)^{2}\\right) - \\frac{x}{32}", 0, 1},
    {"3\\ln|x + 2| - 2\\ln|x - 4|", 0, 1},
    {"x - 4\\ln\\left(x^{2} + 2x + 4\\right) + \\frac{8\\sqrt{3}}{3}\\arctan\\left(\\frac{\\sqrt{3}(x + 1)}{3}\\right)", 0, 1},
    {"\\frac{-1}{2(2x + 3)}", 0, 1},
    {"2\\arctan\\sqrt{x}", 0, 1},
    {"\\frac{3}{64}(2x + 1)^{\\frac{8}{3}} - \\frac{3}{20}(2x + 1)^{\\frac{5}{3}} + \\frac{3}{16}(2x + 1)^{\\frac{2}{3}}", 0, 1},
    {"\\arctan(\\ln x + 2)", 0, 1},
    {"e^{x}\\left(x^{3} - 3x^{2} + 6x - 6\\right)", 0, 1},
    {"\\frac{1}{2}e^{x}(\\sin x - \\cos x)", 0, 1},
    {"-\\frac{5}{6}", 1, 1},
    {"-5 - \\frac{\\pi}{2}", 1, 1},
    {"\\ln 2 - 2 + \\frac{\\pi}{2}", 1, 1},
    {"-\\frac{1}{9} - \\frac{2}{9}e^{3}", 1, 1},
    {"16\\ln 2 - 9\\ln 3", 1, 1},
    {"2\\ln 3 - \\frac{22}{3}", 1, 1},
    {"\\ln\\frac{(x - 1)^{2}}{x^{2} + x + 1} - \\frac{4}{\\sqrt{3}}\\arctan\\left(\\frac{2x + 1}{\\sqrt{3}}\\right)", 0, 1},
    {"-\\frac{1}{x} + \\ln\\frac{\\sqrt{x^{2} + x + 1}}{|x|} - \\frac{1}{\\sqrt{3}}\\arctan\\left(\\frac{2x + 1}{\\sqrt{3}}\\right)", 0, 1},
    {"\\frac{1}{4}\\ln\\frac{|x + 1|}{\\sqrt{x^{2} + 1}} + \\frac{1}{2}\\arctan x + \\frac{x + 1}{4\\left(x^{2} + 1\\right)}", 0, 1},
    {"\\frac{x + 2}{4\\left((x + 2)^{2} + 1\\right)^{2}} + \\frac{3}{4}\\left[\\frac{x + 2}{2\\left((x + 2)^{2} + 1\\right)} + \\frac{1}{2}\\arctan(x + 2)\\right]", 0, 1},
    {"x\\arctan\\sqrt{x} - \\sqrt{x} + \\arctan\\sqrt{x}", 0, 1},
    {"\\frac{2}{\\ln 2}\\left[\\sqrt{2^{x} - 1} - \\arctan\\sqrt{2^{x} - 1}\\right]", 0, 1},
    {"\\frac{x}{2}\\left[\\sin(\\ln x) + \\cos(\\ln x)\\right]", 0, 1},
    {"\\frac{1}{2}x - \\ln|e^{x} - 1| + \\frac{1}{2}\\ln|e^{x} - 2|", 0, 1},
    {"\\pi", 1, 1},
    {"\\frac{5}{2}", 1, 1},
    {"4", 1, 1},
    {"2", 1, 1},
    {"\\frac{\\pi}{2}", 1, 1},
    {"\\frac{1}{7}\\cos^{7} x - \\frac{1}{5}\\cos^{5} x", 0, 1},
    {"\\frac{1}{32}\\sin 4x + \\frac{1}{4}\\sin 2x + \\frac{3}{8}x", 0, 1},
    {"-\\tan x + \\frac{1}{3}\\tan^{3} x + x", 0, 1},
    {"\\frac{1}{4}\\ln\\left|2\\tan\\frac{x}{2} + 1\\right| - \\frac{1}{4}\\ln\\left|2\\tan\\frac{x}{2} - 1\\right|", 0, 1},
    {"\\frac{1}{2}\\cos x - \\frac{1}{10}\\cos 5x", 0, 1},
    {"\\frac{1}{2}\\ln\\left(x + \\sqrt{x^{2} + 1}\\right) + \\frac{1}{2}x\\sqrt{x^{2} + 1}", 0, 1},
    {"\\frac{1}{2}x\\sqrt{x^{2} - 16} - 8\\ln\\left(\\sqrt{x^{2} - 16} + x\\right)", 0, 1},
    {"\\frac{1}{2}\\left(\\ln\\left|\\sqrt{4 - x^{2}} - 2\\right| - \\ln|x|\\right)", 0, 1},
    {"\\tan x - \\cot x", 0, 1},
    {"-\\frac{1}{3}x^{2}\\left(2 - x^{2}\\right)^{\\frac{3}{2}} - \\frac{2}{15}\\left(2 - x^{2}\\right)^{\\frac{5}{2}}", 0, 1},
    {"2\\sqrt{x} - 3\\sqrt[3]{x} + 6\\sqrt[6]{x} - 6\\ln\\left(\\sqrt[6]{x} + 1\\right)", 0, 1},
    {"\\ln\\frac{\\sqrt{1 + x^{2}}}{|x|} - \\frac{1}{2x^{2}}", 0, 1},
    {"\\ln\\frac{x^{2}}{\\sqrt{x^{2} + 2x + 5}} - \\frac{13}{2}\\arctan\\frac{x + 1}{2}", 0, 1},
    {"-\\frac{1}{x^{2} + x\\sqrt{x^{2} + 4}}", 0, 1},
    {"\\arcsin\\frac{x - 1}{\\sqrt{2}}", 0, 0},
    {"6\\pi - 6", 1, 1},
    {"\\frac{\\sqrt{3}}{6}\\pi - 2\\ln 2", 1, 1},
    {"-\\sqrt{2} - 2\\ln\\left(1 - \\frac{\\sqrt{2}}{2}\\right)", 1, 1},
    {"\\frac{1}{2}", 1, 1},
    {"\\frac{\\pi^{2}}{28}", 1, 1},
    {"3\\left(\\frac{-\\arctan x}{x} + \\ln\\frac{|x|}{\\sqrt{1 + x^{2}}}\\right)", 0, 1},
    {"3 \\cdot \\left[\\frac{\\pi}{4} - \\ln\\frac{1}{\\sqrt{2}}\\right]", 1, 1},
    {"1", 1, 1},
    {"\\frac{3}{2}2^{2/3}", 1, 1},
    {"\\frac{\\pi}{4}", 1, 1},
    {"+\\infty", 1, 0},
    {"\\pi", 1, 1},
    {"+\\infty", 1, 0},
    {"\\frac{5}{34}\\left(e^{10\\pi} - 1\\right)", 1, 1},
    {"\\frac{1}{10}\\left(1 - e^{8\\pi}\\right)", 1, 1},
};
enum { EXPECTED = sizeof expected / sizeof *expected };

/* Ritorna 1 se il motore dimostra l'enunciato text. */
static int verdict_proved(const char *text) {
    Verdict verdict = prove(text, 'n', 0, 0);
    return verdict.outcome == PROVED;
}

/* Controllo 5: almeno `at_least` enunciati di path vengono dichiarati falsi,
 * e nessuno dimostrato. */
static int check_refuted_at_least(const char *path, int at_least) {
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }
    const char *position = text, *item, *end;
    int total = 0, refuted = 0, proved = 0;
    while (next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        total++;
        if (verdict.outcome == REFUTED) refuted++;
        if (verdict.outcome == PROVED) proved++;
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d statements of %s are REFUTED (at least %d wanted), %d are PROVED (none allowed)",
             refuted, total, path, at_least, proved);
    return check_result(refuted >= at_least && proved == 0, summary);
}

/* Controllo 6: le richieste e i risultati del libro. */
static int check_requests(const char *path, int at_least) {
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }
    const char *position = text, *item, *end;
    int total = 0, solved = 0, wrong = 0;
    while (next_item(&position, &item, &end)) {
        static char request[MAX_FORMULA], answer[2 * MAX_FORMULA], statement[4 * MAX_FORMULA];
        Item read;
        total++;
        if (total > EXPECTED || read_item(item, end, &read)) { wrong++; printf("  request %d cannot be read\n", total); continue; }
        snprintf(request, sizeof request, "%s", read.formula);
        Verdict verdict = prove(request, 'n', 0, 0);
        if (verdict.outcome == NOT_PROVED || verdict.outcome == UNSUPPORTED) continue;
        if (verdict.outcome != SOLVED || !verdict.detail) {
            wrong++;
            printf("  request %d: unexpected verdict ", total);
            verdict_print(stdout, &verdict);
            putchar('\n');
            continue;
        }
        solved++;
        snprintf(answer, sizeof answer, "%s", verdict.detail);
        const Expected *e = &expected[total - 1];

        /* La risposta, riscritta come enunciato, va dimostrata. */
        snprintf(statement, sizeof statement, "%s = %s", request, answer);
        if (!verdict_proved(statement)) { wrong++; printf("  request %d: the answer is not proved: %s\n", total, statement); continue; }
        if (!e->comparable) continue;

        /* Il libro: la differenza tra le due primitive ha derivata nulla, o
         * i due numeri sono uguali. Per una primitiva la risposta finisce
         * con + c, che resta fuori dalla differenza. */
        if (e->definite) {
            snprintf(statement, sizeof statement, "\\int_{0}^{1} 0 \\, dx = \\left(%s\\right) - \\left(%s\\right)", answer, e->answer);
        } else {
            size_t length = strlen(answer);
            if (length > 4 && strcmp(answer + length - 4, " + c") == 0) answer[length - 4] = 0;
            snprintf(statement, sizeof statement, "\\int 0 \\, dx = \\left(%s\\right) - \\left(%s\\right) + c", answer, e->answer);
        }
        if (!verdict_proved(statement)) { wrong++; printf("  request %d: the answer and the book do not agree: %s\n", total, statement); }
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d requests of %s answered (at least %d wanted), each proved and equal to the book's result", solved, total, path, at_least);
    return check_result(total == EXPECTED && solved >= at_least && wrong == 0, summary);
}

/* Un caso del lettore degli integrali: il testo, se deve essere accettato,
 * e allora la variabile, se è definito e se c'è la risposta. */
typedef struct {
    const char *text;
    int accepted;
    int variable;
    int definite;
    int answered;
} ReaderCase;

static const ReaderCase reader_cases[] = {
    {"\\int x \\, dx = \\frac{x^{2}}{2} + c", 1, 'x', 0, 1},
    {"\\int \\frac{dx}{x}", 1, 'x', 0, 0},
    {"\\int \\frac{t \\, dt}{t^{2} + 1}", 1, 't', 0, 0},
    {"\\int_{0}^{1} x \\, dx = \\frac{1}{2}", 1, 'x', 1, 1},
    {"\\int^{1}_{0} x \\, dx", 1, 'x', 1, 0},
    {"\\int_0^1 x\\,dx", 1, 'x', 1, 0},
    {"\\int x^{2} + 1 \\, dx", 1, 'x', 0, 0},
    {"\\int \\sin 2x \\, dx", 1, 'x', 0, 0},
    {"\\int dx = x + C", 1, 'x', 0, 1},
    {"\\int \\ln|x| \\, dx", 1, 'x', 0, 0},
    {"\\int x \\, dx", 1, 'x', 0, 0},
    {"\\int x \\, dx + 1", 0, 0, 0, 0},
    {"\\int x", 0, 0, 0, 0},
    {"\\int x \\, dx \\, dx", 0, 0, 0, 0},
    {"\\int \\sin\\left(x \\, dx\\right)", 0, 0, 0, 0},
    {"\\int_{0} x \\, dx", 0, 0, 0, 0},
    {"\\int x \\, dx = \\frac{x^{2}}{2}", 0, 0, 0, 0},
    {"\\int x \\, dx = \\frac{x^{2}}{2} + c + 1", 0, 0, 0, 0},
    {"\\int |x \\, dx", 0, 0, 0, 0},
};

/* Controllo 7: il lettore degli integrali (9j). */
static int check_reader(void) {
    int passed = 0, total = (int)(sizeof reader_cases / sizeof *reader_cases);
    for (int i = 0; i < total; i++) {
        const ReaderCase *c = &reader_cases[i];
        IntegralStatement s;
        arena_used = arena_keep;
        failure = NULL;
        int accepted = parse_integral_statement(c->text, &s);
        syntax_error = NULL;
        int right = accepted == c->accepted && (!accepted || (s.variable == c->variable && !!s.lower == c->definite && !!s.answer == c->answered));
        if (right) passed++;
        else printf("  reader case %d not as expected: %s\n", i + 1, c->text);
    }
    char summary[128];
    snprintf(summary, sizeof summary, "%d of %d readings of integrals as expected", passed, total);
    return check_result(passed == total, summary);
}

/* Una funzione razionale per il controllo 8: numeratore e denominatore come
 * coefficienti interi da x^0 in su, e se la primitiva deve essere trovata. */
typedef struct {
    const char *what;
    int num_degree, den_degree;
    long long num[9], den[9];
    int found;
} RationalCase;

static const RationalCase rational_cases[] = {
    {"distinct rational roots, 1/(x^3 - x)", 0, 3, {1}, {0, -1, 0, 1}, 1},
    {"a root of multiplicity three, (x^4 + 1)/(x^2 (x - 1)^3)", 4, 5, {1, 0, 0, 0, 1}, {0, 0, -1, 3, -3, 1}, 1},
    {"an irreducible quadratic with negative discriminant, 1/(x^2 + x + 1)", 0, 2, {1}, {1, 1, 1}, 1},
    {"an irreducible quadratic with positive discriminant, 1/(x^2 - 2)", 0, 2, {1}, {-2, 0, 1}, 1},
    {"two quadratic factors found by Kronecker, 1/(x^4 + 5x^2 + 4)", 0, 4, {1}, {4, 0, 5, 0, 1}, 1},
    {"a repeated quadratic factor, x/(x^2 + 4)^2", 1, 4, {0, 1}, {16, 0, 8, 0, 1}, 1},
    {"a polynomial quotient, (x^3 + 1)/(x - 2)", 3, 1, {1, 0, 0, 1}, {-2, 1}, 1},
    {"x^{m-1} R(x^m), x^3/(x^8 + 1)", 3, 8, {0, 0, 0, 1}, {1, 0, 0, 0, 0, 0, 0, 0, 1}, 1},
    {"an irreducible cubic, 1/(x^3 + 2)", 0, 3, {1}, {2, 0, 0, 1}, 0},
};

/* Controllo 8: le funzioni razionali da sole. La primitiva scritta da
 * rational_any() si rilegge e si verifica con primitive_check(), come ogni
 * altra; quando non deve essere trovata, il metodo deve rinunciare. */
static int check_rational(void) {
    int passed = 0, total = (int)(sizeof rational_cases / sizeof *rational_cases);
    for (int i = 0; i < total; i++) {
        const RationalCase *c = &rational_cases[i];
        char text[ANSWER_BYTES], integrand[ANSWER_BYTES] = "\\frac{";
        QPoly num = qpoly_zero(), den = qpoly_zero();
        arena_used = arena_keep;
        atom_count = base_poly_count = atan_count = 0;
        failure = NULL;
        induction_var = 0;
        for (int k = 0; k <= c->num_degree; k++) num.c[k] = frac_int(c->num[k]);
        for (int k = 0; k <= c->den_degree; k++) den.c[k] = frac_int(c->den[k]);
        num.degree = c->num_degree;
        den.degree = c->den_degree;
        qpoly_trim(&num);
        qpoly_trim(&den);
        int found = rational_any(num, den, 'x', text, sizeof text, 0);
        failure = NULL;
        if (found != c->found) { printf("  rational case %d (%s): found = %d\n", i + 1, c->what, found); continue; }
        if (!found) { passed++; continue; }

        /* La primitiva trovata, riletta e verificata. */
        qpoly_latex(num, 'x', integrand, sizeof integrand);
        text_add(integrand, sizeof integrand, "}{");
        qpoly_latex(den, 'x', integrand, sizeof integrand);
        text_add(integrand, sizeof integrand, "}");
        Node *f = text_tree(integrand), *F = text_tree(text);
        PrimitiveCheck check = f && F ? primitive_check(f, F, 'x') : (PrimitiveCheck){NOT_PROVED, "unreadable", {0, 1}, 0, {0, 1}, NULL};
        if (check.outcome == PROVED) passed++;
        else printf("  rational case %d (%s): %s is not verified (%s)\n", i + 1, c->what, text, check.reason ? check.reason : "");
    }
    char summary[160];
    snprintf(summary, sizeof summary, "%d of %d rational functions integrated, or refused, as expected", passed, total);
    return check_result(passed == total, summary);
}

int main(void) {
    engine_init();
    int ok = 1;
    ok &= check_all_proved("tests/integrals/book_integrals.tex");
    ok &= check_none_proved("tests/integrals/false_integrals.tex");
    ok &= check_none_refuted("tests/integrals/book_integrals_beyond.tex");
    ok &= check_all_proved("tests/integrals/integral_conventions.tex");
    ok &= check_none_proved("tests/integrals/false_integral_conventions.tex");
    ok &= check_refuted_at_least("tests/integrals/false_integrals.tex", 118);
    ok &= check_requests("tests/integrals/integral_requests.tex", 57);
    ok &= check_reader();
    ok &= check_rational();
    ok &= check_result(unjustified_steps == 0, "every step of the written proofs passed its independent check");
    return ok ? 0 : 1;
}
