/* Test di dominio: lo svolgimento passo per passo delle equazioni e delle
 * disequazioni razionali in una variabile reale (capitolo 40 di main.c, e
 * capitolo 41 di output.c per il testo).
 *
 * Il verdetto di questi enunciati lo controlla già
 * test_equations_inequalities.c. Qui si controlla la strada: per ogni
 * enunciato di worked_solutions.tex il motore deve aver registrato una
 * spiegazione, e i suoi passi devono essere quelli che si scrivono alla
 * lavagna. I passi attesi sono scritti qui sotto, e sono stati ricavati a
 * mano (i conti sono nel commento di expected[]), non copiati da ciò che il
 * motore scrive.
 *
 *   1. Ogni enunciato ha la sua spiegazione, e il verdetto è quello atteso.
 *   2. La catena del primo confronto: tutto a un membro, una sola frazione,
 *      i fattori. Un passo atteso deve esserci con quel testo; uno non
 *      atteso non deve esserci (non si registra un passo che non cambia
 *      nulla).
 *   3. I punti della tabella, la riga dei segni della frazione e l'ultima
 *      riga, quella da cui si legge la risposta, casella per casella.
 *   4. La risposta: le soluzioni, oppure la colonna che decide.
 *   5. I valori di prova del primo enunciato, con i due membri calcolati.
 *   6. Nessun passo ha fallito il controllo indipendente
 *      (unjustified_steps resta a zero).
 *   7. Il documento: ogni voce comincia con lo svolgimento, ha la catena, la
 *      tabella e i valori di prova, e conserva in fondo la dimostrazione di
 *      sempre; i grafici sono uno per voce; due scritture danno gli stessi
 *      file, byte per byte; senza le spiegazioni il documento è quello di
 *      prima.
 *
 * I file vengono scritti nella cartella indicata da TMPDIR, o in /tmp: mai
 * nel repository. Il test non compila il documento in PDF.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_worked_solutions \
 *         tests/equations_inequalities/test_worked_solutions.c \
 *         && /tmp/test_worked_solutions
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
 * worked_solutions.tex. I testi sono LaTeX, come li scrive il motore; NULL
 * in move, combine o factor vuol dire che quel passo non ci deve essere.
 * Le caselle si scrivono con un carattere per colonna: + - 0 e n ("nessun
 * valore") per una riga di segni, T e F per una riga di verità. */
typedef struct {
    Outcome outcome;
    const char *move, *combine, *factor;  /* la catena del primo confronto */
    const char *points;                   /* i punti della tabella, in ordine */
    const char *label, *signs;            /* la riga dei segni della frazione:
                                             la sua formula e le sue caselle */
    const char *truth;                    /* le caselle dell'ultima riga */
    const char *answer;                   /* il testo della risposta, o NULL */
    int count, flag;                      /* i campi del passo della risposta */
} Expected;

/* I passi attesi, nell'ordine degli enunciati del file. I conti:
 *
 *   1. (x - 2)/(x + 3) - x = (x - 2 - x^2 - 3x)/(x + 3) = (-x^2 - 2x - 2)/(x + 3),
 *      e x^2 + 2x + 2 ha discriminante 4 - 8 < 0: mai zero, positivo. Per
 *      x < -3 la frazione è -(+)/(-) > 0; per x > -3 è negativa.
 *   2. x^2 - 5x + 6 = (x - 2)(x - 3): negativo solo tra 2 e 3.
 *   3. 1/x - x = (1 - x^2)/x = -(x + 1)(x - 1)/x. Segni, da sinistra:
 *      -(-)(-)/(-) = +, poi 0 in -1, poi -, niente in 0, poi +, 0 in 1, -.
 *   4. x^2 - 2 non si scompone tra i razionali: si annulla in -+sqrt(2).
 *   5. 1/x > 0 vale per x > 0, x >= 0 anche in 0: lì i due non concordano.
 *   6. x^2 + 1 > 0 sempre: nessun punto, una colonna sola.
 *   7. x^2 = 2 vale nei due punti -+sqrt(2): il primo basta per "esiste".
 *   8. x^2 + 1 < 0 mai.
 *   9. Tra i positivi, x^2 - 4 = (x + 2)(x - 2) <= 0 fino a 2 compreso.
 *  10. (x^2 - 1)/(x - 1) = (x + 1)(x - 1)/(x - 1): in 1 non ha valore, e
 *      x > -1 lì è vero. Segni: (-)(-)/(-) = -, 0 in -1, (+)(-)/(-) = +,
 *      niente in 1, +.
 *  11. 2x^2 - 7x - 4 = (2x + 1)(x - 4), e (4x + 1)^2 non cambia segno: il
 *      prodotto è + prima di -1/2, - fino a 4 (0 in -1/4), + dopo.
 *  12. (1 + x^2)/(3x) <= 0 per x < 0; x < (x + 2)(3 - x) è x^2 - 6 < 0;
 *      insieme, tra -sqrt(6) e 0. */
static const Expected expected[] = {
    {SOLVED, "\\frac{x - 2}{x + 3} - x", "\\frac{-x^{2} - 2x - 2}{x + 3}", "\\frac{-\\left(x^{2} + 2x + 2\\right)}{x + 3}",
     "-3", "\\frac{-\\left(x^{2} + 2x + 2\\right)}{x + 3}", "+n-", "TFF", "x < -3", 0, 0},
    {SOLVED, NULL, NULL, "\\left(x - 2\\right)\\left(x - 3\\right)",
     "2,\\ 3", "\\left(x - 2\\right)\\left(x - 3\\right)", "+0-0+", "FFTFF", "2 < x < 3", 0, 0},
    {SOLVED, "\\frac{1}{x} - x", "\\frac{-x^{2} + 1}{x}", "\\frac{-\\left(x + 1\\right)\\left(x - 1\\right)}{x}",
     "-1,\\ 0,\\ 1", "\\frac{-\\left(x + 1\\right)\\left(x - 1\\right)}{x}", "+0-n+0-", "FTTFFTT", "-1 \\le x < 0 \\lor x \\ge 1", 0, 0},
    {SOLVED, "x^{2} - 2", NULL, NULL,
     "-\\sqrt{2},\\ \\sqrt{2}", "x^{2} - 2", "+0-0+", "FFTFF", "-\\sqrt{2} < x < \\sqrt{2}", 0, 0},
    {REFUTED, NULL, NULL, NULL,
     "0", "\\frac{1}{x}", "-n+", "TFT", "x = 0", 1, 0},
    {PROVED, NULL, NULL, NULL,
     "", "x^{2} + 1", "+", "T", NULL, 0, 1},
    {PROVED, "x^{2} - 2", NULL, NULL,
     "-\\sqrt{2},\\ \\sqrt{2}", "x^{2} - 2", "+0-0+", "FTFTF", "x = -\\sqrt{2}", 1, 1},
    {SOLVED, NULL, NULL, NULL,
     "", "x^{2} + 1", "+", "F", "\\nexists x \\in \\mathbb{R}", 2, 0},
    {SOLVED, "x^{2} - 4", NULL, "\\left(x + 2\\right)\\left(x - 2\\right)",
     "2", "\\left(x + 2\\right)\\left(x - 2\\right)", "-0+", "TTF", "0 < x \\le 2", 0, 0},
    {REFUTED, NULL, NULL, "\\frac{\\left(x + 1\\right)\\left(x - 1\\right)}{x - 1}",
     "-1,\\ 1", "\\frac{\\left(x + 1\\right)\\left(x - 1\\right)}{x - 1}", "-0+n+", "TTTFT", "x = 1", 1, 0},
    {SOLVED, NULL, NULL, "\\left(2x + 1\\right)\\left(4x + 1\\right)^{2}\\left(x - 4\\right)",
     "-\\frac{1}{2},\\ -\\frac{1}{4},\\ 4", "\\left(2x + 1\\right)\\left(4x + 1\\right)^{2}\\left(x - 4\\right)", "+0-0-0+", "TTFTFTT",
     "x \\le -\\frac{1}{2} \\lor x = -\\frac{1}{4} \\lor x \\ge 4", 0, 0},
    {SOLVED, NULL, NULL, NULL,
     "-\\sqrt{6},\\ 0,\\ \\sqrt{6}", "\\frac{1 + x^{2}}{3x}", "---n+++", "FFTFFFF", "-\\sqrt{6} < x < 0", 0, 0},
};

/* Ritorna il primo passo del tipo kind che riguarda il confronto group
 * (0: qualunque) nella spiegazione in corso, o NULL se non c'è. */
static const ExplainStep *step_of(ExplainKind kind, int group) {
    for (int i = 0; i < explanation.steps; i++)
        if (explanation.step[i].kind == kind && (!group || explanation.step[i].group == group)) return &explanation.step[i];
    return NULL;
}

/* Ritorna 1 se il passo del tipo kind del primo confronto è quello atteso:
 * con il testo text, oppure assente se text è NULL. */
static int step_is(ExplainKind kind, const char *text) {
    const ExplainStep *step = step_of(kind, 1);
    return text ? step && strcmp(step->text[0], text) == 0 : step == NULL;
}

/* Scrive in out, che ha MAX_EXPLAIN_COLUMNS + 1 posti, le caselle della
 * riga line della tabella, un carattere per colonna. */
static void cells_of(const ExplainLine *line, char *out) {
    int truth = line->kind != EXPLAIN_LINE_FACTOR && line->kind != EXPLAIN_LINE_VALUE;
    for (int column = 0; column < explanation.columns; column++)
        out[column] = truth ? (line->cell[column] ? 'T' : 'F') : "-0+n"[line->cell[column] + 1];
    out[explanation.columns] = 0;
}

/* Ritorna 1 se la tabella ha una riga di segni con la formula label e le
 * caselle signs. */
static int signs_are(const char *label, const char *signs) {
    char cells[MAX_EXPLAIN_COLUMNS + 1];
    for (int l = 0; l < explanation.lines; l++) {
        const ExplainLine *line = &explanation.line[l];
        if ((line->kind != EXPLAIN_LINE_FACTOR && line->kind != EXPLAIN_LINE_VALUE) || strcmp(line->label, label) != 0) continue;
        cells_of(line, cells);
        return strcmp(cells, signs) == 0;
    }
    return 0;
}

/* Controlli da 1 a 5 sugli enunciati di path. */
static void check_steps(const char *path) {
    char *text = read_file(path);
    const char *position = text, *item, *end;
    int number = 0;
    check(text != NULL, 0, "the file of the statements can be read");
    if (!text) return;
    while (next_item(&position, &item, &end)) {
        const Expected *want = &expected[number];
        if (++number > (int)(sizeof expected / sizeof *expected)) break;
        Verdict verdict = prove_item(item, end);

        /* 1. */
        check(verdict.outcome == want->outcome, number, "the verdict is the expected one");
        check(explanation.steps > 0, number, "the statement has an explanation");
        if (!explanation.steps) continue;
        check(explanation.step[0].kind == EXPLAIN_ASK && explanation.step[explanation.steps - 1].kind == EXPLAIN_ANSWER, number,
              "the explanation opens with what is asked and closes with the answer");

        /* 2. */
        check(step_is(EXPLAIN_MOVE, want->move), number, "the step that moves everything to one side is the expected one");
        check(step_is(EXPLAIN_COMBINE, want->combine), number, "the step that writes one fraction is the expected one");
        check(step_is(EXPLAIN_FACTOR, want->factor), number, "the step that splits into factors is the expected one");

        /* 3. */
        char cells[MAX_EXPLAIN_COLUMNS + 1];
        const ExplainStep *points = step_of(EXPLAIN_POINTS, 0);
        check(points && strcmp(points->text[0], want->points) == 0, number, "the points of the table are the expected ones, in order");
        check((int)strlen(want->truth) == explanation.columns, number, "the table has one column for each point and each interval");
        check(signs_are(want->label, want->signs), number, "the line of the fraction has the expected signs");
        cells_of(&explanation.line[explanation.lines - 1], cells);
        check(strcmp(cells, want->truth) == 0, number, "the last line of the table has the expected truth in each column");

        /* 4. */
        const ExplainStep *answer = step_of(EXPLAIN_ANSWER, 0);
        check(answer && answer->count == want->count && answer->flag == want->flag, number, "the answer is of the expected kind");
        check(answer && (want->answer ? answer->text[0] && strcmp(answer->text[0], want->answer) == 0 : answer->text[0] == NULL), number,
              "the answer is the expected one");

        /* 5. Il primo enunciato: in x = -5 i membri sono 7/2 e -5, in
         * x = -1 sono -3/2 e -1. */
        if (number != 1) continue;
        const ExplainStep *test = step_of(EXPLAIN_TEST, 1);
        check(test && strcmp(test->text[0], "-5") == 0 && strcmp(test->text[1], "\\frac{7}{2}") == 0 && strcmp(test->text[2], "-5") == 0 &&
              strcmp(test->text[3], "\\ge") == 0 && test->flag == 1, number, "the first test value has the two sides computed, and the comparison true");
        check(test && test + 1 < explanation.step + explanation.steps && test[1].kind == EXPLAIN_TEST && strcmp(test[1].text[0], "-1") == 0 &&
              strcmp(test[1].text[1], "-\\frac{3}{2}") == 0 && strcmp(test[1].text[2], "-1") == 0 && test[1].flag == 0, number,
              "the second test value has the two sides computed, and the comparison false");
    }
    check(number == (int)(sizeof expected / sizeof *expected), 0, "the file has one statement for each expected explanation");
    free(text);
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

/* Controllo 7: il documento degli enunciati di path. */
static void check_document(const char *path) {
    const char *folder = getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp";
    char first[1024], second[1024], plain[1024], first_plots[1040], second_plots[1040];
    int statements = (int)(sizeof expected / sizeof *expected);
    snprintf(first, sizeof first, "%s/mathresearcher_test_worked_a.tex", folder);
    snprintf(second, sizeof second, "%s/mathresearcher_test_worked_b.tex", folder);
    snprintf(plain, sizeof plain, "%s/mathresearcher_test_worked_c.tex", folder);
    snprintf(first_plots, sizeof first_plots, "%s/mathresearcher_test_worked_a-plots.pdf", folder);
    snprintf(second_plots, sizeof second_plots, "%s/mathresearcher_test_worked_b-plots.pdf", folder);

    /* Il documento, riletto. */
    figures_wanted = explanations_wanted = 1;
    check(write_document(path, first), 0, "the document is written without problems");
    char *tex = read_file(first), *plots = read_file(first_plots);
    check(tex != NULL && plots != NULL, 0, "the document and the PDF of its graphs exist");
    if (tex && plots) {
        check(count(tex, "\\item $\\displaystyle") == statements, 0, "every statement has its item");
        check(count(tex, "\\textbf{Worked solution.}") == statements, 0, "every item opens with the worked solution");
        check(count(tex, "\\includegraphics") == statements && count(plots, "/Type /Page /Parent") == statements, 0, "every item has its graph, one page each");
        check(strstr(tex, "\\emph{What is asked.} Find every $x$ in $\\mathbb{R}$ for which this condition is true:\n\\[ \\fitmath{\\frac{x - 2}{x + 3} \\ge x} \\]") != NULL,
              0, "the worked solution starts from what is asked");
        check(strstr(tex, "\\emph{What is asked.}") < strstr(tex, "\\includegraphics") && strstr(tex, "\\includegraphics") < strstr(tex, "\\emph{Step 1."),
              0, "the graph comes after what is asked and before the steps");
        check(strstr(tex, " \\iff\\quad & \\frac{-x^{2} - 2x - 2}{x + 3} \\ge 0 && \\text{(bring everything to one fraction, with a common denominator)}") != NULL,
              0, "a step of the chain is written with its reason");
        check(strstr(tex, "\\item $x^{2} + 2x + 2$ is never zero: it is positive for every $x$.") != NULL, 0, "a factor that is never zero is said so, with its sign");
        check(strstr(tex, "\\item $x^{2} - 2$ is zero only for $x = -\\sqrt{2}$ and for $x = \\sqrt{2}$.") != NULL, 0, "irrational zeros are written with radicals");
        check(strstr(tex, "\\displaystyle \\frac{-\\left(x^{2} + 2x + 2\\right)}{x + 3} & + & \\nexists & - \\\\") != NULL, 0, "the sign table is written as a table");
        check(strstr(tex, "\\item $x = -5$: the left side is $\\frac{7}{2}$, the right side is $-5$, and $\\frac{7}{2} \\ge -5$ is true.") != NULL,
              0, "a test value is written with the two sides");
        check(strstr(tex, "in the column $x = 0$: there the claim fails, so the statement is false.") != NULL, 0, "a false statement names the column that refutes it");
        check(strstr(tex, "{\\small \\textbf{The proof on record.} \\textbf{Sign table.}") != NULL &&
              strstr(tex, "Hence the condition holds exactly for $x < -3$. $\\Box$\\par}") != NULL, 0, "the proof of always stays, at the end of the item");
        check(strstr(tex, "\\end{enumerate}\n\\end{document}\n") != NULL, 0, "the document is closed");
    }

    /* Le stesse chiamate, gli stessi file. I due documenti differiscono
     * solo nel nome del PDF che includono, che ha la stessa lunghezza. */
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

    /* Senza le spiegazioni il documento è quello di prima. */
    explanations_wanted = 0;
    check(write_document(path, plain), 0, "the document is written without explanations");
    char *bare = read_file(plain);
    check(bare && count(bare, "Worked solution") == 0 && count(bare, "The proof on record") == 0 && count(bare, "\\includegraphics") == statements &&
          strstr(bare, "$\n\n\\textbf{Sign table.} Each comparison counts as false") != NULL, 0, "without explanations each item is the proof and the graph, as before");
    free(bare);
    figures_wanted = 0;
}

int main(void) {
    engine_init();
    explanations_wanted = 1;
    check_steps("tests/equations_inequalities/worked_solutions.tex");
    check(unjustified_steps == 0, 0, "no recorded step failed its independent check");
    check_document("tests/equations_inequalities/worked_solutions.tex");
    check(unjustified_steps == 0, 0, "no step failed its independent check while the documents were written");
    printf("%s: %d of %d checks on worked solutions passed\n", failures ? "FAIL" : "PASS", checks - failures, checks);
    return failures ? 1 : 0;
}
