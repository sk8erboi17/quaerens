/* Test del documento finale (output.c) e dei grafici che il motore vi mette.
 *
 * Fa scrivere il documento degli enunciati di document_statements.tex e poi
 * lo rilegge, insieme al PDF dei grafici. Controlla che:
 *
 *   1. ogni enunciato abbia la sua voce, con la dimostrazione, le soluzioni
 *      o il verdetto;
 *   2. gli enunciati decisi con la tabella dei segni abbiano il grafico, gli
 *      altri no, e che ogni grafico incluso sia una pagina del PDF dei
 *      grafici, nello stesso ordine;
 *   3. le stesse chiamate diano gli stessi file, byte per byte;
 *   4. senza un documento i grafici non vengano costruiti, e un documento
 *      senza grafici non lasci accanto a sé un PDF vuoto;
 *   5. i nomi dei file siano quelli promessi, e un nome non valido sia
 *      respinto.
 *
 * I file vengono scritti nella cartella indicata da TMPDIR, o in /tmp: mai
 * nel repository. Il test non compila il documento in PDF: quello richiede
 * un compilatore LaTeX, che non è una dipendenza dei test.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_output \
 *         tests/output/test_output.c && /tmp/test_output
 */

#include "../statement_checks.h"

static int checks, failures;

/* Registra l'esito di un controllo; se è fallito lo stampa. */
static void check(int passed, const char *what) {
    checks++;
    if (passed) return;
    failures++;
    printf("  failed: %s\n", what);
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

int main(void) {
    engine_init();
    const char *folder = getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp";
    char first[1024], second[1024], first_plots[1040], second_plots[1040], plain[1024], plain_plots[1040];
    snprintf(first, sizeof first, "%s/mathresearcher_test_output_a.tex", folder);
    snprintf(first_plots, sizeof first_plots, "%s/mathresearcher_test_output_a-plots.pdf", folder);
    snprintf(second, sizeof second, "%s/mathresearcher_test_output_b.tex", folder);
    snprintf(second_plots, sizeof second_plots, "%s/mathresearcher_test_output_b-plots.pdf", folder);
    snprintf(plain, sizeof plain, "%s/mathresearcher_test_output_c.tex", folder);
    snprintf(plain_plots, sizeof plain_plots, "%s/mathresearcher_test_output_c-plots.pdf", folder);
    remove(first_plots);
    remove(second_plots);
    remove(plain_plots);

    /* Senza un documento: nessun grafico viene costruito. */
    Verdict verdict = prove("\\{x \\in \\mathbb{R} \\mid x^{2} < 4\\}", 'n', 0, 0);
    check(verdict.outcome == SOLVED && figure == NULL, "without a document no graph is built");

    /* 1 e 2. Il documento, riletto. */
    figures_wanted = 1;
    check(write_document("tests/output/document_statements.tex", first), "the document is written without problems");
    char *tex = read_file(first), *plots = read_file(first_plots);
    check(tex != NULL && plots != NULL, "the document and the PDF of its graphs exist");
    if (tex && plots) {
        check(strstr(tex, "\\usepackage{graphicx}") != NULL, "the preamble loads the package that includes the graphs");
        check(count(tex, "\\item $\\displaystyle") == 5, "every statement has its item");
        check(strstr(tex, "\\textbf{Proof.} By induction on $n$.") != NULL, "the induction has its written proof");
        check(strstr(tex, "Hence the condition holds exactly for $x < -3 \\lor x \\ge 2$") != NULL, "the solutions are written in the document");
        check(strstr(tex, "So the statement is false.") != NULL, "the false statement is reported as false");
        check(count(tex, "\\textbf{No proof.}") == 1 && strstr(tex, "NON SUPPORTATO") != NULL, "the statement that cannot be read has its verdict in place of a proof");
        check(count(tex, "\\includegraphics") == 3, "the three statements decided by a sign table have a graph, the others none");
        check(strstr(tex, "\\includegraphics[page=1,width=0.8\\textwidth]{mathresearcher_test_output_a-plots.pdf}") != NULL,
              "a graph is included by page number, from the file next to the document");
        check(strstr(tex, "[page=2,") && strstr(tex, "[page=3,") && !strstr(tex, "[page=4,"), "the pages are numbered in the order of the statements");
        check(strstr(tex, "[page=1,") < strstr(tex, "[page=2,") && strstr(tex, "[page=2,") < strstr(tex, "[page=3,"), "and appear in that order");
        check(strncmp(plots, "%PDF-", 5) == 0, "the file of the graphs is a PDF");
        check(count(plots, "/Type /Page /Parent") == 3 && strstr(plots, "/Count 3 ") != NULL, "it has one page for each included graph");
        check(strstr(tex, "\\end{enumerate}\n\\end{document}\n") != NULL, "the document is closed");
    }

    /* 3. Le stesse chiamate, gli stessi file. I due documenti differiscono
     * solo nel nome del PDF che includono, che ha la stessa lunghezza. */
    check(write_document("tests/output/document_statements.tex", second), "the document is written a second time");
    char *again = read_file(second), *plots_again = read_file(second_plots);
    check(again && tex && strlen(again) == strlen(tex), "the second document has the same length as the first");
    check(plots_again && plots && strlen(plots) == strlen(plots_again) && strcmp(plots, plots_again) == 0, "the PDF of the graphs is the same, byte for byte");
    free(tex);
    free(plots);
    free(again);
    free(plots_again);

    /* 4. Un documento senza grafici non lascia un PDF vuoto. */
    check(write_document("tests/integers/divisibility.tex", plain), "a document without graphs is written");
    FILE *leftover = fopen(plain_plots, "rb");
    check(leftover == NULL, "and no PDF of graphs is created next to it");
    if (leftover) fclose(leftover);

    /* 5. I nomi. Con .pdf il documento è il .tex con lo stesso nome; qui non
     * lo si compila, perché il test non deve dipendere da un compilatore
     * LaTeX installato. */
    Document document;
    char wanted[1024];
    snprintf(wanted, sizeof wanted, "%s/mathresearcher_test_output_d.pdf", folder);
    check(document_open(&document, wanted) == NULL, "a document can be asked as a PDF");
    check(document.want_pdf && strcmp(document.tex_path + strlen(document.tex_path) - 6, "_d.tex") == 0 &&
          strcmp(document.plots_path + strlen(document.plots_path) - 12, "_d-plots.pdf") == 0, "its LaTeX source and its graphs take their names from it");
    document.want_pdf = 0;
    check(document_close(&document) == NULL, "and it is closed without problems");
    check(document_open(&document, "x") != NULL, "a name too short to have an extension is refused");

    printf("%s: %d of %d output checks passed\n", failures ? "FAIL" : "PASS", checks - failures, checks);
    return failures ? 1 : 0;
}
