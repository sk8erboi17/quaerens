/* Test delle due opzioni che chiedono il documento finale, dalla riga di
 * comando: --proofs, con gli svolgimenti (capitolo 40 di main.c), e
 * --plain-proofs, senza.
 *
 * Il documento asciutto deve essere quello che il programma scriveva prima
 * che esistessero gli svolgimenti. Non basta che "non contenga lo
 * svolgimento": deve essere lo stesso file. Per questo il confronto è con un
 * documento di riferimento, plain_document_expected.tex, scritto dal
 * programma di prima (il commit ffd029d di main) sugli stessi dati,
 * document_statements.tex, e tenuto qui com'è uscito.
 *
 *   1. Con --plain-proofs il documento è il riferimento, byte per byte, con
 *      una sola differenza dichiarata: la riga del preambolo che definisce
 *      \arccot, aggiunta dopo perché senza di lei un documento che lo usa
 *      non si compila. Il test la toglie, una volta, e pretende che ci sia.
 *   2. Il PDF dei grafici è quello di prima: stessa lunghezza e stessa
 *      somma di controllo (un PDF non sta nel repository: di quello di prima
 *      qui ci sono i due numeri).
 *   3. Con --proofs gli svolgimenti ci sono, e sotto ognuno c'è ancora la
 *      dimostrazione del documento asciutto.
 *   4. Le due opzioni non si danno insieme, né una due volte: il programma
 *      lo dice ed esce con 1.
 *
 * Il test chiama main() del programma, con le sue opzioni: è lì che si
 * decide quale documento scrivere. Per questo nel suo output compaiono
 * anche le righe dei verdetti.
 *
 * Quando il testo di una dimostrazione cambia di proposito il riferimento
 * va rifatto, e si rifà con il programma stesso, dalla cartella principale:
 *
 *     mathresearcher --plain-proofs /tmp/mathresearcher_test_plain.tex \
 *         tests/output/document_statements.tex
 *
 * togliendo dal file scritto la riga di \arccot; lunghezza e somma di
 * controllo del PDF dei grafici le stampa questo test quando non tornano.
 *
 * I file vengono scritti nella cartella indicata da TMPDIR, o in /tmp: mai
 * nel repository. Il test non compila il documento in PDF.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_plain_document \
 *         tests/output/test_plain_document.c && /tmp/test_plain_document
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

/* Il PDF dei grafici che il programma di prima scriveva per
 * document_statements.tex: quanti byte, e la loro somma di controllo
 * (file_sum()). */
#define OLD_PLOTS_BYTES 46044ULL
#define OLD_PLOTS_SUM 9888178031069191667ULL

/* Mette in *length la lunghezza del file path e ritorna la sua somma di
 * controllo: FNV-1a a 64 bit, cioè per ogni byte "xor, poi per un numero
 * primo fisso". Due file diversi della stessa lunghezza hanno la stessa
 * somma con probabilità trascurabile. Ritorna 0, con *length a 0, se il
 * file non si apre. */
static unsigned long long file_sum(const char *path, unsigned long long *length) {
    FILE *in = fopen(path, "rb");
    unsigned long long sum = 14695981039346656037ULL;
    int byte;
    *length = 0;
    if (!in) return 0;
    while ((byte = fgetc(in)) != EOF) {
        sum = (sum ^ (unsigned char)byte) * 1099511628211ULL;
        ++*length;
    }
    fclose(in);
    return sum;
}

/* Chiama main() del programma con l'opzione option, il documento document e
 * il file di enunciati, e ne ritorna il codice di uscita. second, se non è
 * NULL, è una seconda opzione con il suo documento, messa dopo la prima. */
static int run(const char *option, const char *document, const char *second, const char *second_document) {
    char words[6][1100];
    char *argv[7];
    int argc = 0;
    snprintf(words[argc++], sizeof words[0], "mathresearcher");
    snprintf(words[argc++], sizeof words[0], "%s", option);
    snprintf(words[argc++], sizeof words[0], "%s", document);
    if (second) {
        snprintf(words[argc++], sizeof words[0], "%s", second);
        snprintf(words[argc++], sizeof words[0], "%s", second_document);
    }
    snprintf(words[argc++], sizeof words[0], "tests/output/document_statements.tex");
    for (int i = 0; i < argc; i++) argv[i] = words[i];
    argv[argc] = NULL;
    int code = mathresearcher_main(argc, argv);

    /* main() chiude l'ultimo grafico senza azzerare il puntatore, perché
     * dopo il programma finisce: qui invece si ricomincia. */
    figure = NULL;
    return code;
}

int main(void) {
    static const char arccot[] = "\\DeclareMathOperator{\\arccot}{arccot}\n";
    const char *folder = getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp";
    char plain[1024], plain_plots[1040], worked[1024], spare[1024], other[1024];
    snprintf(plain, sizeof plain, "%s/mathresearcher_test_plain.tex", folder);
    snprintf(plain_plots, sizeof plain_plots, "%s/mathresearcher_test_plain-plots.pdf", folder);
    snprintf(worked, sizeof worked, "%s/mathresearcher_test_worked.tex", folder);
    snprintf(spare, sizeof spare, "%s/mathresearcher_test_spare_a.tex", folder);
    snprintf(other, sizeof other, "%s/mathresearcher_test_spare_b.tex", folder);
    remove(plain_plots);
    remove(other);

    /* 1. Il documento asciutto, confrontato con il riferimento. Due dei
     * cinque enunciati non sono dimostrati (uno è falso, uno non si legge):
     * il programma esce con 1. */
    check(run("--plain-proofs", plain, NULL, NULL) == 1, "the program writes the plain document and reports that not every statement is proved");
    char *written = read_file(plain), *expected = read_file("tests/output/plain_document_expected.tex");
    check(written != NULL && expected != NULL, "the plain document and the reference document can be read");
    if (written && expected) {
        char *line = strstr(written, arccot);
        check(line != NULL, "the plain document has the preamble line that defines \\arccot");
        if (line) memmove(line, line + strlen(arccot), strlen(line + strlen(arccot)) + 1);
        size_t same = 0;
        while (written[same] && written[same] == expected[same]) same++;
        if (written[same] || expected[same]) printf("  the plain document differs from the reference at byte %zu\n", same);
        check(!written[same] && !expected[same], "but for that line, the plain document is the document of before, byte for byte");
        check(count(written, "Worked solution") == 0 && count(written, "The proof on record") == 0, "the plain document has no worked solution");
    }

    /* 2. I grafici. */
    unsigned long long length, sum = file_sum(plain_plots, &length);
    if (length != OLD_PLOTS_BYTES || sum != OLD_PLOTS_SUM) printf("  the PDF of the graphs has %llu bytes and checksum %lluULL\n", length, sum);
    check(length == OLD_PLOTS_BYTES && sum == OLD_PLOTS_SUM, "the PDF of the graphs of the plain document is the one of before");

    /* 3. Il documento con gli svolgimenti: ci sono, e sotto ognuno resta la
     * dimostrazione, quella del documento asciutto. */
    check(run("--proofs", worked, NULL, NULL) == 1, "the program writes the document with the worked solutions");
    char *full = read_file(worked);
    check(full != NULL, "the document with the worked solutions can be read");
    if (full && written) {
        int solutions = count(full, "\\textbf{Worked solution.}");
        check(solutions >= 2, "with --proofs the statements the engine can explain open with a worked solution");
        check(count(full, "{\\small \\textbf{The proof on record.} ") == solutions, "and each worked solution is followed by the proof on record");
        check(strstr(full, "Hence the condition holds exactly for $x < -3 \\lor x \\ge 2$. $\\Box$\\par}") != NULL &&
              strstr(written, "Hence the condition holds exactly for $x < -3 \\lor x \\ge 2$. $\\Box$\n") != NULL, "the proof on record is the proof of the plain document");
        check(count(full, "\\item $\\displaystyle") == count(written, "\\item $\\displaystyle"), "the two documents have the same items");
        check(strstr(full, "\\textbf{No proof.} NON SUPPORTATO") != NULL, "a statement without a proof is reported as in the plain document");
    }
    free(written);
    free(expected);
    free(full);

    /* 4. Un documento solo. */
    check(run("--proofs", spare, "--plain-proofs", other) == 1, "the two options together are refused");
    check(run("--plain-proofs", spare, "--plain-proofs", other) == 1, "the plain document asked twice is refused");
    FILE *second = fopen(other, "r");
    check(second == NULL, "and the second document is not written");
    if (second) fclose(second);

    check(unjustified_steps == 0, "no recorded step failed its independent check");
    printf("%s: %d of %d checks on the plain document passed\n", failures ? "FAIL" : "PASS", checks - failures, checks);
    return failures ? 1 : 0;
}
