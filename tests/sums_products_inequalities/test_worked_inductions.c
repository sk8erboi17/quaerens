/* Test di dominio: la spiegazione di un'uguaglianza dimostrata per induzione
 * (paragrafo 40b di main.c, e capitolo 41 di output.c per il testo).
 *
 * Che gli enunciati siano dimostrati lo controlla già
 * test_simple_inductions.c. Qui si controlla ciò che il motore registra per
 * spiegarli: i primi valori dei due membri, il caso base, e il passo svolto
 * come un conto. I valori attesi sono scritti qui sotto e sono stati
 * calcolati a mano; le righe attese del passo sono formule, confrontate con
 * quelle registrate per valore (il test di annullamento del capitolo 12),
 * non per come sono scritte.
 *
 *   1. Ogni enunciato di worked_inductions.tex ha la sua spiegazione, con
 *      i primi sei valori dei due membri, quelli attesi, uguali riga per
 *      riga, e il caso base che è la prima riga.
 *   2. Il passo: dove un membro è una somma fino a n e l'altro una formula
 *      che si scrive come polinomio, c'è la catena attesa (ultimo termine
 *      staccato, ipotesi, conti) e i due membri arrivano alla stessa
 *      scrittura. Dove la forma è un'altra (un prodotto che dà una frazione
 *      non ridotta, una somma dentro un'espressione) il passo non si
 *      registra, e la spiegazione lo dice.
 *   3. Il bugiardo: un enunciato falso dato per dimostrato (vero per n = 1,
 *      falso da n = 2) non si spiega, e il passo conta tra i non
 *      giustificati. Un enunciato con un parametro, i cui valori non sono
 *      numeri, resta senza spiegazione, con il suo verdetto.
 *   4. Nessun passo ha fallito il controllo (unjustified_steps a zero).
 *   5. Il documento: ogni voce comincia con ciò che si afferma e la tabella
 *      dei primi valori, ha il suo grafico, il passo svolto, e in fondo la
 *      dimostrazione di sempre; due scritture danno gli stessi file; senza
 *      le spiegazioni non c'è nessun grafico, come prima.
 *
 * I file vengono scritti nella cartella indicata da TMPDIR, o in /tmp: mai
 * nel repository. Il test non compila il documento in PDF.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_worked_inductions \
 *         tests/sums_products_inequalities/test_worked_inductions.c \
 *         && /tmp/test_worked_inductions
 */

#include "../statement_checks.h"

static int checks, failures;

/* Registra l'esito di un controllo sull'enunciato numero item (0: nessuno
 * in particolare); se è fallito lo stampa. */
static void check(int passed, int item, const char *what) {
    checks++;
    if (passed) return;
    failures++;
    if (item) printf("  failed, statement %d: %s\n", item, what);
    else printf("  failed: %s\n", what);
}

/* Ciò che ci si aspetta dalla spiegazione di un enunciato di
 * worked_inductions.tex: i valori dei due membri per n = 1, ..., 6, come
 * testo LaTeX, e le righe della catena del membro che è una somma. Con
 * split = NULL il passo non deve essere registrato. */
typedef struct {
    const char *value[EXPLAIN_VALUES];  /* i primi valori, uguali per i due membri */
    const char *split;                  /* la somma fino a n + 1 con l'ultimo termine staccato */
    const char *assumed;                /* la stessa, con l'altro membro al posto della somma */
    const char *expanded;               /* i conti fatti: la forma a cui arrivano i due membri */
    int side;                           /* quale membro è la somma: 1 il primo, 2 il secondo */
} Expected;

/* I conti. 1: 1, 1+2, 1+2+3, ...; il passo: n(n+1)/2 + (n+1) =
 * (n^2 + 3n + 2)/2. 2: i quadrati; n^2 + (2(n+1) - 1) = n^2 + 2n + 1.
 * 3: 1, 5, 14, 30, 55, 91; n(n+1)(2n+1)/6 + (n+1)^2 =
 * (2n^3 + 9n^2 + 13n + 6)/6. 4: 2, 6, 14, 30, 62, 126;
 * 2^(n+1) - 2 + 2^(n+1) = 4 * 2^n - 2. 5: come 2, con i membri scambiati.
 * 6: (1 + 1/1)(1 + 1/2)... = 2, 3, 4, ...: il prodotto fino a n + 1 è
 * (n + 1)(1 + 1/(n + 1)), che il motore tiene come frazione non ridotta e
 * non scrive come n + 2: niente catena. 7: 1/2, 2/3, 3/4, ...: una
 * frazione, niente catena. 8: 2, 6, 12, 20, 30, 42; il primo membro non è
 * una somma sola: niente catena. */
static const Expected expected[] = {
    {{"1", "3", "6", "10", "15", "21"}, "\\sum_{k=1}^{n} k + (n + 1)", "\\frac{n(n+1)}{2} + (n + 1)", "\\frac{n^{2} + 3n + 2}{2}", 1},
    {{"1", "4", "9", "16", "25", "36"}, "\\sum_{k=1}^{n} \\left(2k - 1\\right) + (2(n + 1) - 1)", "n^{2} + (2(n + 1) - 1)", "n^{2} + 2n + 1", 1},
    {{"1", "5", "14", "30", "55", "91"}, "\\sum_{k=1}^{n} k^{2} + (n + 1)^{2}", "\\frac{n(n+1)(2n+1)}{6} + (n + 1)^{2}", "\\frac{2n^{3} + 9n^{2} + 13n + 6}{6}", 1},
    {{"2", "6", "14", "30", "62", "126"}, "\\sum_{k=1}^{n} 2^{k} + 2^{n + 1}", "2^{n + 1} - 2 + 2^{n + 1}", "4 \\cdot 2^{n} - 2", 1},
    {{"1", "4", "9", "16", "25", "36"}, "\\sum_{k=1}^{n} \\left(2k - 1\\right) + (2(n + 1) - 1)", "n^{2} + (2(n + 1) - 1)", "n^{2} + 2n + 1", 2},
    {{"2", "3", "4", "5", "6", "7"}, NULL, NULL, NULL, 0},
    {{"\\frac{1}{2}", "\\frac{2}{3}", "\\frac{3}{4}", "\\frac{4}{5}", "\\frac{5}{6}", "\\frac{6}{7}"}, NULL, NULL, NULL, 0},
    {{"2", "6", "12", "20", "30", "42"}, NULL, NULL, NULL, 0},
};

/* Ritorna 1 se le espressioni scritte in a e in b hanno lo stesso valore
 * per ogni n, per il test di annullamento del capitolo 12: una somma fino a
 * n è la stessa lettera in tutte e due. */
static int same_value(const char *a, const char *b) {
    Scratch mark = scratch_mark();
    failure = syntax_error = NULL;
    env_count = 0;
    Rat gap = rat_add(eval(parse_expression(a)), eval(parse_expression(b)), 1);
    int same = !syntax_error && !failure && (gap.num.count == 0 || poly_vanishes(gap.num));
    failure = syntax_error = NULL;
    scratch_drop(mark);
    return same;
}

/* Ritorna il passo numero nth (da 0) del tipo kind che riguarda il membro
 * group (0: qualunque) nella spiegazione in corso, o NULL se non c'è. */
static const ExplainStep *step_of(ExplainKind kind, int group, int nth) {
    for (int i = 0; i < explanation.steps; i++)
        if (explanation.step[i].kind == kind && (!group || explanation.step[i].group == group) && nth-- == 0) return &explanation.step[i];
    return NULL;
}

/* Controlli 1 e 2 sugli enunciati di path. */
static void check_steps(const char *path) {
    char *text = read_file(path);
    const char *position = text, *item, *end;
    int number = 0;
    check(text != NULL, 0, "the file of the statements can be read");
    if (!text) return;
    while (next_item(&position, &item, &end)) {
        if (++number > (int)(sizeof expected / sizeof *expected)) break;
        const Expected *want = &expected[number - 1];
        Verdict verdict = prove_item(item, end);
        check(verdict.outcome == PROVED && verdict.variable == 'n' && verdict.base == 1, number, "the statement is proved by induction from n = 1");
        check(explanation.steps > 0 && explanation.request == EXPLAIN_INDUCTION, number, "the statement has an explanation");
        if (!explanation.steps) continue;
        check(explanation.step[0].kind == EXPLAIN_ASK && strcmp(explanation.step[0].text[1], "1") == 0 &&
              explanation.step[explanation.steps - 1].kind == EXPLAIN_ANSWER, number, "the explanation opens with the claim and its base, and closes with the conclusion");

        /* 1. I primi valori, e il caso base. */
        for (int row = 0; row < EXPLAIN_VALUES; row++) {
            const ExplainStep *value = step_of(EXPLAIN_VALUE, 0, row);
            char n[8];
            snprintf(n, sizeof n, "%d", row + 1);
            check(value && strcmp(value->text[0], n) == 0 && strcmp(value->text[1], want->value[row]) == 0 && strcmp(value->text[2], want->value[row]) == 0,
                  number, "a row of the first values has the expected value on both sides");
        }
        check(step_of(EXPLAIN_VALUE, 0, EXPLAIN_VALUES) == NULL, number, "there are six rows of first values");
        const ExplainStep *base = step_of(EXPLAIN_BASE, 0, 0);
        check(base && strcmp(base->text[2], want->value[0]) == 0 && strcmp(base->text[3], want->value[0]) == 0 && same_value(base->text[0], want->value[0]) &&
              same_value(base->text[1], want->value[0]), number, "the base case is the first row, with the two sides written for n = 1");
        check(step_of(EXPLAIN_HYPOTHESIS, 0, 0) != NULL, number, "the inductive hypothesis is recorded");

        /* 2. Il passo. */
        const ExplainStep *answer = step_of(EXPLAIN_ANSWER, 0, 0);
        if (!want->split) {
            check(!step_of(EXPLAIN_TARGET, 0, 0) && !step_of(EXPLAIN_REWRITE, 0, 0) && answer && answer->count == 0, number,
                  "the step is not recorded, and the conclusion says so");
            continue;
        }
        const ExplainStep *split = step_of(EXPLAIN_REWRITE, want->side, 0), *assumed = step_of(EXPLAIN_REWRITE, want->side, 1);
        const ExplainStep *expanded = step_of(EXPLAIN_REWRITE, want->side, 2), *other = step_of(EXPLAIN_REWRITE, 3 - want->side, 0);
        check(answer && answer->count == 1 && step_of(EXPLAIN_TARGET, 1, 0) && step_of(EXPLAIN_TARGET, 2, 0), number, "the step is recorded for both sides");
        check(split && split->flag == REWRITE_SPLIT_SUM && same_value(split->text[0], want->split), number, "the last term of the sum is split off");
        check(assumed && assumed->flag == REWRITE_HYPOTHESIS && same_value(assumed->text[0], want->assumed), number, "the hypothesis puts the other side in place of the sum");
        check(expanded && expanded->flag == REWRITE_EXPAND && same_value(expanded->text[0], want->expanded), number, "the operations are carried out");
        check(other && other->flag == REWRITE_EXPAND && expanded && strcmp(other->text[0], expanded->text[0]) == 0 && !step_of(EXPLAIN_REWRITE, 3 - want->side, 1),
              number, "the other side, with its operations carried out, is written in the same way");
    }
    check(number == (int)(sizeof expected / sizeof *expected), 0, "the file has one statement for each expected explanation");
    free(text);
}

/* Controllo 3: un enunciato falso dato per dimostrato, e uno con un
 * parametro. */
static void check_liars(void) {
    /* Vero per n = 1 (1 = 1 + 1 - 1), falso da n = 2 (3 contro 4). Il
     * motore lo dichiara falso; qui gli si dà per buono un verdetto
     * rovesciato, come farebbe un errore del capitolo 13. */
    const char *liar = "\\sum_{k=1}^{n} k = \\frac{n(n+1)}{2} + n - 1";
    Verdict verdict = prove(liar, 'n', 0, 0);
    check(verdict.outcome == REFUTED && explanation.steps == 0, 0, "a false statement is refuted, and has no explanation");
    Verdict forged = {PROVED, 'n', 1, 0, NULL, NULL, "forged", NULL, NULL, NULL};
    int before = unjustified_steps;
    explain_induction(liar, &forged);
    check(explanation.steps == 0 && unjustified_steps == before + 1, 0, "a statement wrongly given as proved is not explained: a row of its first values does not agree");
    unjustified_steps = before;

    /* La stessa chiamata con un enunciato vero spiega. */
    const char *honest = "\\sum_{k=1}^{n} k = \\frac{n(n+1)}{2}";
    verdict = prove(honest, 'n', 0, 0);
    explain_induction(honest, &verdict);
    check(verdict.outcome == PROVED && explanation.steps > 0 && unjustified_steps == before, 0, "the same call on a true statement explains it");

    /* Un parametro: i valori non sono numeri. */
    verdict = prove("\\sum_{k=1}^{n} x^{k} = \\frac{x^{n+1} - x}{x - 1}", 'n', 0, 0);
    check(verdict.outcome == PROVED && verdict.proof && explanation.steps == 0 && unjustified_steps == before, 0,
          "a statement with a parameter keeps its proof, without an explanation");
}

/* Ritorna quante volte needle compare in text. */
static int count(const char *text, const char *needle) {
    int found = 0;
    for (const char *at = strstr(text, needle); at; at = strstr(at + 1, needle)) found++;
    return found;
}

/* Scrive in tex_path il documento degli enunciati di statements, senza
 * compilarlo. Ritorna 1 se il documento e i suoi grafici sono stati scritti
 * senza problemi. */
static int write_document(const char *statements, const char *tex_path) {
    Document document;
    Report report;
    if (document_open(&document, tex_path)) return 0;
    document_section(&document, statements);
    int read = prove_file(statements, &report, NULL, &document, NULL);
    return document_close(&document) == NULL && read;
}

/* Controllo 5: il documento degli enunciati di path. */
static void check_document(const char *path) {
    const char *folder = getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp";
    char first[1024], second[1024], plain[1024], first_plots[1040], second_plots[1040], plain_plots[1040];
    int statements = (int)(sizeof expected / sizeof *expected);
    snprintf(first, sizeof first, "%s/mathresearcher_test_inductions_a.tex", folder);
    snprintf(second, sizeof second, "%s/mathresearcher_test_inductions_b.tex", folder);
    snprintf(plain, sizeof plain, "%s/mathresearcher_test_inductions_c.tex", folder);
    snprintf(first_plots, sizeof first_plots, "%s/mathresearcher_test_inductions_a-plots.pdf", folder);
    snprintf(second_plots, sizeof second_plots, "%s/mathresearcher_test_inductions_b-plots.pdf", folder);
    snprintf(plain_plots, sizeof plain_plots, "%s/mathresearcher_test_inductions_c-plots.pdf", folder);
    remove(plain_plots);

    figures_wanted = explanations_wanted = 1;
    check(write_document(path, first), 0, "the document is written without problems");
    char *tex = read_file(first), *plots = read_file(first_plots);
    check(tex != NULL && plots != NULL, 0, "the document and the PDF of its graphs exist");
    if (tex && plots) {
        check(count(tex, "\\textbf{Worked solution.}") == statements, 0, "every item opens with the worked solution");
        check(strstr(tex, "\\emph{What is claimed.} That for every integer $n \\ge 1$\n\\[ \\fitmath{\\sum_{k=1}^{n} k = \\frac{n \\cdot \\left(n + 1\\right)}{2}} \\]") != NULL, 0,
              "the worked solution starts from what is claimed");
        check(strstr(tex, "n & \\text{left side} & \\text{right side} \\\\ \\hline\n") != NULL && strstr(tex, "}3 & 6 & 6 \\\\\n") != NULL, 0,
              "the first values are written as a table");
        check(count(tex, "\\includegraphics") == statements && count(plots, "/Type /Page /Parent") == statements, 0, "every item has the graph of its first values, one page each");
        check(strstr(tex, "\\end{array} \\]") < strstr(tex, "\\includegraphics") && strstr(tex, "\\includegraphics") < strstr(tex, "\\emph{Step 1. The idea: induction.}"),
              0, "the graph comes after the table and before the proof is explained");
        check(count(tex, "{\\small \\textbf{Graph.} The rows of the table: a dot for each value of the left side") == statements, 0, "each graph says how to read it");
        check(strstr(tex, "\\emph{Step 2. Base case.} For $n = 1$ the left side is $\\sum_{k=1}^{1} k = 1$") != NULL, 0, "the base case is written with the sides for n = 1");
        check(strstr(tex, " ={} & \\sum_{k=1}^{n} k + \\left(n + 1\\right) && \\text{(split off the last term of the sum)} \\\\\n") != NULL &&
              strstr(tex, " && \\text{(inductive hypothesis)} \\\\\n") != NULL, 0, "the inductive step is written as a chain, each line with its reason");
        check(count(tex, "Both sides become the same expression, so the claim is true for $n + 1$.") == 5, 0, "the five statements with a worked step say that the two sides meet");
        check(count(tex, "The inductive step is carried out in the proof on record below.") == 3, 0, "the three without it point to the proof on record");
        check(count(tex, "{\\small \\textbf{The proof on record.} \\textbf{Proof.} By induction on $n$.") == statements, 0, "the proof of always stays, at the end of each item");
    }

    /* Le stesse chiamate, gli stessi file. */
    check(write_document(path, second), 0, "the document is written a second time");
    char *again = read_file(second), *plots_again = read_file(second_plots);
    int same = again && tex && strlen(again) == strlen(tex), names = 0;
    for (size_t i = 0; same && tex[i]; i++) {
        if (tex[i] == again[i]) continue;
        same = tex[i] == 'a' && again[i] == 'b';
        names++;
    }
    check(same && names == statements, 0, "the second document is the first, character by character, but for the name of the PDF it includes");
    check(plots_again && plots && strlen(plots) == strlen(plots_again) && strcmp(plots, plots_again) == 0, 0, "the PDF of the graphs is the same, byte for byte");
    free(tex);
    free(plots);
    free(again);
    free(plots_again);

    /* Senza le spiegazioni: nessuno svolgimento e nessun grafico, come
     * prima che esistessero. */
    explanations_wanted = 0;
    check(write_document(path, plain), 0, "the document is written without explanations");
    char *bare = read_file(plain);
    FILE *leftover = fopen(plain_plots, "rb");
    check(bare && count(bare, "Worked solution") == 0 && count(bare, "\\includegraphics") == 0 && leftover == NULL &&
          count(bare, "$\n\n\\textbf{Proof.} By induction on $n$.") == statements, 0, "without explanations each item is the proof alone, and no graph is drawn");
    if (leftover) fclose(leftover);
    free(bare);
    figures_wanted = 0;
}

int main(void) {
    engine_init();
    explanations_wanted = 1;
    check_steps("tests/sums_products_inequalities/worked_inductions.tex");
    check_liars();
    check(unjustified_steps == 0, 0, "no recorded step failed its independent check, apart from the forged one");
    check_document("tests/sums_products_inequalities/worked_inductions.tex");
    check(unjustified_steps == 0, 0, "no step failed its independent check while the documents were written");
    printf("%s: %d of %d checks on worked inductions passed\n", failures ? "FAIL" : "PASS", checks - failures, checks);
    return failures ? 1 : 0;
}
