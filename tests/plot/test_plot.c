/* Test della libreria dei grafici, plot.c, da sola: qui il motore non c'entra.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_plot tests/plot/test_plot.c && /tmp/test_plot
 *
 * La compilazione fa parte del test: plot.c deve compilarsi da solo e senza
 * avvisi, perché main.c lo include così com'è. Per questo il file include
 * soltanto plot.c, e -Werror trasforma ogni avviso in un errore.
 *
 * I PDF vengono scritti nella cartella indicata da TMPDIR (o in /tmp), mai
 * nel repository, poi riletti byte per byte e cancellati. I controlli sono di
 * tre tipi.
 *
 *   1. La forma del file. L'indice in fondo a un PDF dà la posizione di ogni
 *      oggetto: structure_problem() verifica che ogni posizione cada proprio
 *      sull'oggetto giusto, e che ogni contenuto di pagina sia lungo quanto
 *      dichiara. È il controllo che scopre quasi ogni errore di scrittura:
 *      un byte contato male sposta tutto ciò che segue.
 *   2. Il disegno. Le pagine sono testo, e si può cercarvi ciò che una
 *      chiamata deve aver prodotto: un'etichetta, il numero di tratti di una
 *      curva interrotta da un NaN, i rettangoli di ritaglio, una coordinata.
 *      Le coordinate attese sono calcolate dai margini di plot.c, non
 *      scritte a mano: i margini possono cambiare, le proporzioni no.
 *   3. Gli errori. Una figura che non si può disegnare, o descritta mentre
 *      la memoria finisce, non deve mai diventare una pagina. Per provarlo
 *      le allocazioni di plot.c passano da qui, e si possono far fallire.
 *
 * Questi test non guardano la figura: che l'aspetto sia quello voluto si
 * controlla a occhio, trasformando un PDF in immagine.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Quante allocazioni possono ancora riuscire prima che la memoria "finisca";
 * un numero negativo vuol dire che non finisce mai. Una volta finita resta
 * finita, come succede davvero. allocation_refused dice se almeno una
 * richiesta è stata respinta. */
static long allocations_left = -1;
static int allocation_refused;

/* Ritorna 1 se la prossima allocazione può riuscire, e la conta. */
static int allocation_allowed(void) {
    if (allocations_left < 0) return 1;
    if (allocations_left == 0) {
        allocation_refused = 1;
        return 0;
    }
    allocations_left--;
    return 1;
}

/* Le tre funzioni di allocazione che plot.c userà al posto di quelle vere. */
static void *test_malloc(size_t size) {
    return allocation_allowed() ? malloc(size) : NULL;
}

static void *test_calloc(size_t count, size_t size) {
    return allocation_allowed() ? calloc(count, size) : NULL;
}

static void *test_realloc(void *block, size_t size) {
    return allocation_allowed() ? realloc(block, size) : NULL;
}

/* plot.c viene incluso con le allocazioni deviate sulle funzioni qui sopra.
 * <stdlib.h> è già stato incluso, quindi le macro non toccano le sue
 * dichiarazioni; dopo l'inclusione il test torna a usare quelle vere. */
#define malloc(size) test_malloc(size)
#define calloc(count, size) test_calloc(count, size)
#define realloc(block, size) test_realloc(block, size)
#include "../../plot.c"
#undef malloc
#undef calloc
#undef realloc

static int checks, failures;

/* Registra l'esito di un controllo e lo stampa. */
static void check(int passed, const char *description) {
    checks++;
    if (!passed) failures++;
    printf("%s: %s\n", passed ? "PASS" : "FAIL", description);
}

/* Il contenuto di un file riletto in memoria. bytes ha size byte seguiti da
 * uno zero, ed è posseduto; è NULL se il file non si è potuto leggere. */
typedef struct {
    char *bytes;
    long size;
} Content;

/* Mette in path il nome di un file temporaneo, fuori dal repository. */
static void temporary_path(char *path, size_t size, const char *name) {
    const char *folder = getenv("TMPDIR");
    snprintf(path, size, "%s/mathresearcher_test_plot_%s.pdf", folder && *folder ? folder : "/tmp", name);
}

/* Rilegge il file path. */
static Content read_content(const char *path) {
    Content content = {NULL, 0};
    FILE *file = fopen(path, "rb");
    if (!file) return content;
    if (fseek(file, 0, SEEK_END) == 0 && (content.size = ftell(file)) >= 0) {
        rewind(file);
        content.bytes = malloc((size_t)content.size + 1);
    }
    if (content.bytes && fread(content.bytes, 1, (size_t)content.size, file) == (size_t)content.size) {
        content.bytes[content.size] = 0;
    } else {
        free(content.bytes);
        content.bytes = NULL;
    }
    fclose(file);
    return content;
}

/* Ritorna quante volte needle compare in text, senza sovrapposizioni; -1 se
 * text è NULL, cioè se il file non si è potuto rileggere. */
static int count_of(const char *text, const char *needle) {
    int count = 0;
    if (!text) return -1;
    for (const char *at = text; (at = strstr(at, needle)) != NULL; at += strlen(needle)) count++;
    return count;
}

/* Ritorna 1 se nel contenuto compare la scritta data, composta come con
 * printf(). */
static int has(const Content *content, const char *format, ...) {
    char needle[512];
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(needle, sizeof needle, format, arguments);
    va_end(arguments);
    return content->bytes && strstr(content->bytes, needle) != NULL;
}

/* Ritorna 1 se text comincia con la scritta data, composta come con
 * printf(). */
static int starts_with(const char *text, const char *format, ...) {
    char prefix[512];
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(prefix, sizeof prefix, format, arguments);
    va_end(arguments);
    return strncmp(text, prefix, strlen(prefix)) == 0;
}

/* Il massimo numero di oggetti di un PDF che structure_problem() sa
 * controllare: bastano per una decina di pagine. */
#define MAX_OBJECTS 40

/* Controlla la forma di un PDF di pages pagine scritto da plot.c. Ritorna
 * NULL se è tutto a posto, altrimenti il primo problema trovato, in inglese.
 *
 * Segue la strada di un lettore di PDF: dal fondo del file all'indice,
 * dall'indice a ogni oggetto, dalla radice alle pagine e ai loro contenuti.
 * I numeri degli oggetti sono quelli fissati in plot.c: 1 la radice, 2
 * l'elenco delle pagine, 3 il carattere, poi tre oggetti per pagina (la
 * pagina, il contenuto, la lunghezza del contenuto). */
static const char *structure_problem(const Content *pdf, int pages) {
    const char *text = pdf->bytes;
    int objects = 3 + 3 * pages;
    long position[MAX_OBJECTS + 1];
    if (!text) return "the file cannot be read";
    if (objects > MAX_OBJECTS) return "too many pages for this test";

    /* L'inizio, la fine, e nessun byte zero in mezzo: il file è testo. */
    if ((long)strlen(text) != pdf->size) return "the file contains a zero byte";
    if (!starts_with(text, "%%PDF-1.4\n")) return "the file does not start with %PDF-1.4";
    if (pdf->size < 6 || strcmp(text + pdf->size - 6, "%%EOF\n") != 0) return "the file does not end with %%EOF";

    /* L'ultimo startxref dà la posizione della parola xref. */
    const char *startxref = NULL;
    for (const char *at = text; (at = strstr(at, "startxref\n")) != NULL; at++) startxref = at;
    if (!startxref) return "there is no startxref";
    long xref = strtol(startxref + strlen("startxref\n"), NULL, 10);
    if (xref <= 0 || xref >= pdf->size || !starts_with(text + xref, "xref\n")) return "startxref does not point at the xref keyword";

    /* L'indice: l'intestazione, la riga dell'oggetto 0, poi una riga di 20
     * byte per oggetto, fatta di dieci cifre e di una coda fissa. */
    if (!starts_with(text + xref, "xref\n0 %d\n0000000000 65535 f \n", objects + 1)) return "the xref table does not start as expected";
    const char *entry = strchr(strchr(text + xref, '\n') + 1, '\n') + 1 + 20;
    for (int number = 1; number <= objects; number++, entry += 20) {
        if (strspn(entry, "0123456789") != 10 || !starts_with(entry + 10, " 00000 n \n")) return "an xref entry is not 20 bytes in the standard form";
        position[number] = strtol(entry, NULL, 10);

        /* Il controllo principale: alla posizione scritta nell'indice
         * comincia proprio l'oggetto con quel numero. */
        if (position[number] >= xref || !starts_with(text + position[number], "%d 0 obj\n", number)) return "an xref offset does not point at its object";
    }

    /* La coda, fino all'ultimo byte del file. */
    char trailer[128];
    snprintf(trailer, sizeof trailer, "trailer\n<< /Size %d /Root 1 0 R >>\nstartxref\n%ld\n%%%%EOF\n", objects + 1, xref);
    if (strcmp(entry, trailer) != 0) return "the trailer is not as expected";

    /* Nessun oggetto fuori dall'indice. */
    if (count_of(text, " 0 obj\n") != objects || count_of(text, "endobj\n") != objects) return "the number of objects differs from the xref table";

    /* La radice, l'elenco delle pagine con /Count, il carattere. */
    char kids[512] = "", page_tree[640];
    for (int page = 0; page < pages; page++) snprintf(kids + strlen(kids), sizeof kids - strlen(kids), "%s%d 0 R", page ? " " : "", 4 + 3 * page);
    snprintf(page_tree, sizeof page_tree, "2 0 obj\n<< /Type /Pages /Kids [%s] /Count %d >>\nendobj\n", kids, pages);
    if (!starts_with(text + position[1], "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n")) return "the catalog is not as expected";
    if (!starts_with(text + position[2], "%s", page_tree)) return "the page tree does not list the pages, or /Count is wrong";
    if (!starts_with(text + position[3], "3 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica ")) return "the font is not as expected";
    if (count_of(text, "/Type /Page ") != pages) return "the number of page objects differs from the number of saved figures";
    if (count_of(text, "\nstream\n") != pages || count_of(text, "endstream") != pages) return "the number of streams differs from the number of pages";

    /* Ogni pagina: l'oggetto pagina, il contenuto, e la lunghezza dichiarata
     * confrontata con i byte che ci sono davvero tra stream ed endstream. */
    for (int page = 0; page < pages; page++) {
        int number = 4 + 3 * page;
        char opening[64], header[32];
        if (!starts_with(text + position[number], "%d 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 ", number)) return "a page object is not as expected";
        snprintf(opening, sizeof opening, "%d 0 obj\n<< /Length %d 0 R >>\nstream\n", number + 1, number + 2);
        if (!starts_with(text + position[number + 1], "%s", opening)) return "a content stream does not start as expected";

        /* La lunghezza sta in un oggetto a parte, subito dopo la riga che
         * lo apre (la posizione l'ha già controllata l'indice). */
        snprintf(header, sizeof header, "%d 0 obj\n", number + 2);
        long length = strtol(text + position[number + 2] + strlen(header), NULL, 10);
        const char *data = text + position[number + 1] + strlen(opening);
        const char *end = strstr(data, "\nendstream\nendobj\n");
        if (!end || end - data != length) return "a /Length differs from the real number of bytes of its stream";
    }
    return NULL;
}

/* Scrive le figure date, una per pagina, in un file temporaneo, lo rilegge,
 * lo cancella e ne ritorna il contenuto, che chi chiama libera. Controlla
 * che ogni pagina sia stata salvata e che la forma del file sia giusta: è un
 * controllo solo, con una riga nel resoconto. name distingue il file e
 * compare nei messaggi. Le figure restano di chi chiama. */
static Content render(const char *name, PlFigure *const *figures, int pages) {
    char path[1024], message[256];
    temporary_path(path, sizeof path, name);
    PlPdf *pdf = pl_pdf_open(path);
    int saved = pdf != NULL;
    for (int page = 0; page < pages; page++) saved = pl_pdf_savefig(pdf, figures[page]) && saved;
    saved = pl_pdf_close(pdf) && saved;
    Content content = read_content(path);
    remove(path);

    const char *problem = saved ? structure_problem(&content, pages) : "a figure was not saved, or the file was not closed correctly";
    snprintf(message, sizeof message, "%s: saved, and the PDF structure is sound (%s)", name, problem ? problem : "xref offsets, page tree, stream lengths");
    check(problem == NULL, message);
    return content;
}

/* Come render() per una figura sola, che viene anche liberata. */
static Content render_one(const char *name, PlFigure *figure) {
    Content content = render(name, &figure, 1);
    pl_close(figure);
    return content;
}

/* La cornice dell'unico pannello di una figura di 6.4 per 4.8 pollici senza
 * titoli, in punti: è la figura di quasi tutti i controlli sulle coordinate. */
#define FRAME_LEFT PL_MARGIN_LEFT
#define FRAME_BOTTOM PL_MARGIN_BOTTOM
#define FRAME_WIDTH (6.4 * 72 - PL_MARGIN_LEFT - PL_MARGIN_RIGHT)
#define FRAME_HEIGHT (4.8 * 72 - PL_MARGIN_TOP - PL_MARGIN_BOTTOM)

/* Ritorna una figura di 6.4 per 4.8 pollici con un solo pannello, messo in
 * *axes, con i limiti dati e senza tacche: nella pagina resta solo ciò che
 * il test vi disegna. */
static PlFigure *bare_figure(PlAxes **axes, double xlow, double xhigh, double ylow, double yhigh) {
    PlFigure *figure = pl_figure(6.4, 4.8);
    *axes = pl_add_axes(figure, 1);
    pl_set_xlim(*axes, xlow, xhigh);
    pl_set_ylim(*axes, ylow, yhigh);
    pl_set_xticks(*axes, NULL, NULL, 0);
    pl_set_yticks(*axes, NULL, NULL, 0);
    return figure;
}

/* Ritorna una figura vicina all'uso vero: il grafico di (x - 2)/(x + 3) con
 * il suo asintoto e le bande dove è positiva, un secondo pannello con due
 * linee, e in fondo una striscia sottile usata come retta dei numeri.
 * Sopporta che la memoria finisca a metà: plot.c accetta i puntatori nulli. */
static PlFigure *number_line_figure(void) {
    enum { SAMPLES = 200 };
    double x[SAMPLES + 1], y[SAMPLES + 1], other[SAMPLES + 1];
    double zero = 0, nan = zero / zero;
    int count = 0;

    /* I campioni, con un NaN sull'asintoto perché i due rami restino divisi. */
    for (int i = 0; i < SAMPLES; i++) {
        double value = -8 + 15.0 * i / (SAMPLES - 1);
        if (count > 0 && x[count - 1] < -3 && value > -3) {
            x[count] = -3;
            y[count++] = nan;
        }
        x[count] = value;
        y[count++] = (value - 2) / (value + 3);
    }
    for (int i = 0; i < count; i++) other[i] = 4 - x[i] * x[i] / 8;

    PlFigure *figure = pl_figure(6.4, 5.5);
    pl_suptitle(figure, "A rational inequality");

    PlAxes *curve = pl_add_axes(figure, 1);
    pl_set_title(curve, "Sign of the left side");
    pl_set_xlim(curve, -8, 7);
    pl_set_ylim(curve, -10, 10);
    pl_plot(curve, x, y, count, PL_BLUE, 1.5, "left side");
    pl_axhline(curve, 0, PL_BLACK, 0.8, 0);
    pl_axvline(curve, -3, PL_GRAY, 0.8, 1);
    pl_axvspan(curve, -8, -3, PL_GREEN);
    pl_axvspan(curve, 2, 7, PL_GREEN);
    pl_marker(curve, 2, 0, PL_GREEN, 1);
    pl_legend(curve);

    PlAxes *pair = pl_add_axes(figure, 1);
    pl_plot(pair, x, other, count, PL_ORANGE, 1.5, "a parabola");
    pl_plot(pair, x, x, count, PL_RED, 1.5, NULL);
    pl_legend(pair);

    PlAxes *line = pl_add_axes(figure, 0.3);
    double at[] = {-3, 2.5}, segment[] = {-8, -3}, level[] = {0, 0};
    const char *const label[] = {"-3", "5/2"};
    pl_set_xlim(line, -8, 7);
    pl_set_ylim(line, -1, 1);
    pl_set_yticks(line, NULL, NULL, 0);
    pl_set_xticks(line, at, label, 2);
    pl_plot(line, segment, level, 2, PL_GREEN, 4, NULL);
    pl_marker(line, -3, 0, PL_GREEN, 0);
    return figure;
}

/* La figura d'uso: forma del file, file identici da chiamate identiche, e
 * ciò che ogni chiamata deve aver lasciato nella pagina. */
static void test_whole_figure(void) {
    PlFigure *figure = number_line_figure();
    Content first = render("whole figure", &figure, 1);
    pl_close(figure);
    Content second = render_one("whole figure again", number_line_figure());
    check(first.bytes && second.bytes && first.size == second.size && memcmp(first.bytes, second.bytes, (size_t)first.size) == 0,
          "two identical sequences of calls give byte-identical files");

    check(count_of(first.bytes, " re W n\n") == 3, "a figure with three panels has three clipping rectangles");
    check(has(&first, "(-3) Tj") && has(&first, "(5/2) Tj"), "explicit tick labels appear as PDF strings");
    check(has(&first, "(A rational inequality) Tj") && has(&first, "(Sign of the left side) Tj"), "the figure title and the panel title are written");
    check(has(&first, "(left side) Tj") && has(&first, "(a parabola) Tj"), "the legend names the labelled lines");
    check(has(&first, "[%.2f %.2f] 0 d", PL_DASH_ON * 0.8, PL_DASH_OFF * 0.8), "a dashed line sets a dash pattern proportional to its width");
    check(has(&first, "0.793 0.907 0.793 rg"), "a band is filled with the colour mixed with white, as alpha 0.25 on white");
    check(has(&first, "1.000 1.000 1.000 rg") && has(&first, "0.173 0.627 0.173 rg"), "a hollow marker has a white face, a filled one a coloured face");
    check(count_of(first.bytes, " c\n") == 8, "each of the two markers is a circle made of four curves");
    check(!has(&first, "nan") && !has(&first, "inf"), "no NaN or infinity is ever written into the page");
    free(first.bytes);
    free(second.bytes);
}

/* Più pagine di misure diverse nello stesso file. */
static void test_pages_of_different_sizes(void) {
    PlAxes *axes;
    PlFigure *figures[3] = {pl_figure(6.4, 4.8), pl_figure(4, 3), pl_figure(8, 2)};
    for (int i = 0; i < 3; i++) pl_add_axes(figures[i], 1);
    Content pdf = render("three pages", figures, 3);
    for (int i = 0; i < 3; i++) pl_close(figures[i]);

    const char *first = pdf.bytes ? strstr(pdf.bytes, "/MediaBox [0 0 460.80 345.60]") : NULL;
    const char *second = first ? strstr(first, "/MediaBox [0 0 288.00 216.00]") : NULL;
    const char *third = second ? strstr(second, "/MediaBox [0 0 576.00 144.00]") : NULL;
    check(third != NULL && count_of(pdf.bytes, "/MediaBox") == 3, "each page has the /MediaBox of its figure, in points, in the order of saving");
    free(pdf.bytes);

    /* La stessa figura salvata due volte: due pagine uguali, e la figura
     * resta buona. */
    PlFigure *twice[2];
    twice[0] = twice[1] = bare_figure(&axes, 0, 1, 0, 1);
    pdf = render("one figure saved twice", twice, 2);
    pl_close(twice[0]);
    free(pdf.bytes);
}

/* Una curva con un punto non finito è divisa in tratti separati. */
static void test_broken_lines(void) {
    double zero = 0, nan = zero / zero, infinity = 1 / zero;
    double x[] = {0, 1, 2, 3, 4};
    PlAxes *axes;

    /* Un NaN in mezzo: due tratti da due punti, cioè due "m" e due "l". */
    double with_nan[] = {0, 1, nan, 1, 0};
    PlFigure *figure = bare_figure(&axes, 0, 4, 0, 1);
    pl_plot(axes, x, with_nan, 5, PL_BLUE, 1.5, NULL);
    Content pdf = render_one("line with a NaN", figure);
    check(count_of(pdf.bytes, " m\n") == 2 && count_of(pdf.bytes, " l\n") == 2, "a NaN in the data splits the line into two separate subpaths");
    check(!has(&pdf, "nan"), "the NaN itself is not written");
    free(pdf.bytes);

    /* Lo stesso con un infinito, e nelle ascisse. */
    double with_infinity[] = {0, 1, -infinity, 3, 4};
    figure = bare_figure(&axes, 0, 4, 0, 1);
    pl_plot(axes, with_infinity, x, 5, PL_BLUE, 1.5, NULL);
    pdf = render_one("line with an infinity", figure);
    check(count_of(pdf.bytes, " m\n") == 2 && count_of(pdf.bytes, " l\n") == 2, "an infinite abscissa splits the line as a NaN does");
    check(!has(&pdf, "inf"), "the infinity itself is not written");
    free(pdf.bytes);

    /* Un valore enorme resta nella linea, ma con una coordinata limitata. */
    double huge[] = {0, 1, 1e300, 1, 0};
    figure = bare_figure(&axes, 0, 4, 0, 1);
    pl_plot(axes, x, huge, 5, PL_BLUE, 1.5, NULL);
    pdf = render_one("line with a huge value", figure);
    check(count_of(pdf.bytes, " m\n") == 1 && count_of(pdf.bytes, " l\n") == 4, "a huge finite value does not split the line");
    check(has(&pdf, " %.2f l\n", PL_FAR), "a huge ordinate is written as a bounded coordinate");
    free(pdf.bytes);

    /* Numeri che un PDF non può contenere, passati dove non sono dati: uno
     * spessore, una posizione, gli estremi di una banda. */
    figure = bare_figure(&axes, 0, 4, 0, 1);
    pl_axhline(axes, nan, PL_RED, nan, 1);
    pl_axvline(axes, infinity, PL_RED, -1, 0);
    pl_axvspan(axes, nan, 2, PL_GREEN);
    pl_axvspan(axes, -infinity, 2, PL_GREEN);
    pl_marker(axes, nan, infinity, PL_RED, 1);
    pl_plot(axes, x, x, 5, (PlColor)42, infinity, NULL);
    pdf = render_one("non-finite arguments", figure);
    check(!has(&pdf, "nan") && !has(&pdf, "inf"), "non-finite widths and positions never reach the file");
    check(count_of(pdf.bytes, " re f\n") == 2, "a band with a NaN end is not drawn (the other fill is the page background)");
    check(has(&pdf, "%.2f %.2f %.2f %.2f re f\n", -PL_FAR, FRAME_BOTTOM, FRAME_LEFT + FRAME_WIDTH / 2 + PL_FAR, FRAME_HEIGHT),
          "a band with an infinite end starts beyond the frame, at a bounded coordinate");
    check(has(&pdf, "0.000 0.000 0.000 RG"), "a colour outside the enum is drawn in black");
    free(pdf.bytes);
}

/* Ritorna il contenuto della pagina di una figura senza dati con i limiti
 * dati e le tacche automatiche, alta height pollici. */
static Content automatic_ticks(const char *name, double xlow, double xhigh, double ylow, double yhigh, double height) {
    PlFigure *figure = pl_figure(6.4, height);
    PlAxes *axes = pl_add_axes(figure, 1);
    pl_set_xlim(axes, xlow, xhigh);
    pl_set_ylim(axes, ylow, yhigh);
    return render_one(name, figure);
}

/* Chiude l'elenco delle scritte passato a labels_are(). Il tipo va detto: tra
 * gli argomenti variabili un NULL nudo potrebbe non essere un puntatore. */
#define END ((const char *)NULL)

/* Ritorna 1 se le scritte della pagina sono esattamente quelle elencate
 * (l'elenco finisce con END): ognuna tante volte quante compare nell'elenco,
 * e nessun'altra. */
static int labels_are(const Content *pdf, ...) {
    const char *expected[32];
    int count = 0, matching = 1;
    va_list arguments;
    va_start(arguments, pdf);
    while ((expected[count] = va_arg(arguments, const char *)) != NULL) count++;
    va_end(arguments);

    for (int i = 0; i < count; i++) {
        char needle[64];
        int repeated = 0;
        for (int j = 0; j < count; j++) repeated += strcmp(expected[i], expected[j]) == 0;
        snprintf(needle, sizeof needle, "(%s) Tj", expected[i]);
        if (count_of(pdf->bytes, needle) != repeated) matching = 0;
    }
    return pdf->bytes && matching && count_of(pdf->bytes, " Tj") == count;
}

/* Le tacche: automatiche con i loro passi "belli", scelte, e assenti. */
static void test_ticks(void) {
    Content pdf = automatic_ticks("ticks 0..1 and -3..7", 0, 1, -3, 7, 4.8);
    check(labels_are(&pdf, "0", "0.2", "0.4", "0.6", "0.8", "1", "-2", "0", "2", "4", "6", END),
          "automatic ticks for 0..1 are 0, 0.2, ... 1 and for -3..7 are -2, 0, 2, 4, 6");
    free(pdf.bytes);

    pdf = automatic_ticks("ticks -2..0.5 and 0..25000", -2, 0.5, 0, 25000, 4.8);
    check(labels_are(&pdf, "-2", "-1.5", "-1", "-0.5", "0", "0.5", "0", "5000", "10000", "15000", "20000", "25000", END),
          "labels drop useless decimals (-2, -1.5, ... 0.5) but keep the zeros of 5000");
    free(pdf.bytes);

    pdf = automatic_ticks("ticks 0..0.003 and 1..0", 0, 0.003, 1, 0, 4.8);
    check(labels_are(&pdf, "0", "0.0005", "0.001", "0.0015", "0.002", "0.0025", "0.003", "0", "0.2", "0.4", "0.6", "0.8", "1", END),
          "a small range gets a small step, and a reversed axis the same ticks as a straight one");
    free(pdf.bytes);

    pdf = automatic_ticks("ticks 0.2..0.8 and -1e6..1e6", 0.2, 0.8, -1e6, 1e6, 4.8);
    check(labels_are(&pdf, "0.2", "0.3", "0.4", "0.5", "0.6", "0.7", "0.8", "-1000000", "-500000", "0", "500000", "1000000", END),
          "ticks falling exactly on the limits are kept, also when the limits are not exact in binary");
    free(pdf.bytes);

    /* I limiti qui sotto non sono esatti in binario, e i quozienti che
     * decidono il passo e le tacche estreme escono un soffio più grandi o
     * più piccoli del dovuto: senza tolleranza si perderebbe la tacca -5.8, e
     * il passo da -6 a -5.1 sarebbe 0.2 invece di 0.1. */
    pdf = automatic_ticks("ticks -5.8..-5.6 and -6..-5.1", -5.8, -5.6, -6, -5.1, 4.8);
    check(labels_are(&pdf, "-5.8", "-5.75", "-5.7", "-5.65", "-5.6", "-6", "-5.9", "-5.8", "-5.7", "-5.6", "-5.5", "-5.4", "-5.3", "-5.2", "-5.1", END),
          "rounding errors in the limits neither lose the ticks on the limits nor change the step");
    free(pdf.bytes);

    /* Da 0 a 10^40 le tacche sono 0, 2 10^39, ... 10^40: numeri di quaranta
     * cifre, che non entrano in un'etichetta. Restano le sei tacche, e la
     * sola scritta dello zero: un numero troncato sarebbe un altro numero. */
    pdf = automatic_ticks("ticks 0..1e40", 0, 1e40, 0, 1, 4.8);
    check(labels_are(&pdf, "0", "0", "0.2", "0.4", "0.6", "0.8", "1", END) && count_of(pdf.bytes, " l S\n") == 12,
          "a label too long to be written is left out, not truncated, and its tick is still drawn");
    free(pdf.bytes);

    /* Su un asse corto le etichette sarebbero una sull'altra: meno tacche. */
    pdf = automatic_ticks("ticks on a short axis", 0, 1, 0, 9, 1.2);
    check(labels_are(&pdf, "0", "0.2", "0.4", "0.6", "0.8", "1", "0", "5", END), "a short axis gets fewer ticks than a long one (0 and 5 instead of 0, 1, ... 9)");
    free(pdf.bytes);

    /* Senza limiti e senza dati un asse va da 0 a 1. */
    PlFigure *figure = pl_figure(6.4, 4.8);
    PlAxes *axes = pl_add_axes(figure, 1);
    pl_set_yticks(axes, NULL, NULL, 0);
    pdf = render_one("ticks of an empty panel", figure);
    check(labels_are(&pdf, "0", "0.2", "0.4", "0.6", "0.8", "1", END), "a panel with no data and no limits spans 0..1");
    free(pdf.bytes);

    /* count == 0: né tacche né etichette. Restano solo i rettangoli. */
    pdf = render_one("no ticks", bare_figure(&axes, 0, 1, 0, 1));
    check(count_of(pdf.bytes, " Tj") == 0 && count_of(pdf.bytes, " l S\n") == 0, "count == 0 produces no ticks and no tick labels");
    free(pdf.bytes);

    /* Tacche scelte: solo quelle, senza quelle fuori dai limiti, e una
     * tacca senza scritta se l'etichetta è NULL. */
    double at[] = {-3, 2.5, 99, 5};
    const char *const label[] = {"-3", "5/2", "far away", NULL};
    figure = bare_figure(&axes, -8, 7, 0, 1);
    pl_set_xticks(axes, at, label, 4);
    pdf = render_one("explicit ticks", figure);
    check(labels_are(&pdf, "-3", "5/2", END), "explicit ticks replace the automatic ones, and a tick outside the limits is not drawn");
    check(count_of(pdf.bytes, " l S\n") == 3, "a tick with a NULL label is a mark without text");

    /* La posizione delle scritte: quella dell'asse x è centrata sotto la
     * tacca. "-3" è largo un meno (584) più una cifra (556), a 10 punti. */
    double tick = FRAME_LEFT + FRAME_WIDTH * 5 / 15, width = (584 + 556) * PL_LABEL_SIZE / 1000;
    double baseline = FRAME_BOTTOM - PL_TICK_LENGTH - PL_TICK_PAD - PL_LABEL_SIZE * PL_CAP_HEIGHT;
    check(has(&pdf, "BT /F1 10 Tf %.2f %.2f Td (-3) Tj ET", tick - width / 2, baseline), "an x tick label is centred under its tick, using the Helvetica widths");
    free(pdf.bytes);

    /* Quella dell'asse y finisce a sinistra della tacca. */
    double level[] = {-3, 2.5};
    figure = bare_figure(&axes, 0, 1, -8, 7);
    pl_set_yticks(axes, level, label, 2);
    pdf = render_one("explicit ticks on the y axis", figure);
    check(has(&pdf, "BT /F1 10 Tf %.2f %.2f Td (-3) Tj ET", FRAME_LEFT - PL_TICK_LENGTH - PL_TICK_PAD - width,
              FRAME_BOTTOM + FRAME_HEIGHT * 5 / 15 - PL_LABEL_SIZE * PL_CAP_HEIGHT / 2),
          "a y tick label is right-aligned to the left of its tick");
    free(pdf.bytes);

    /* Scegliere le tacche due volte: vale l'ultima. */
    figure = bare_figure(&axes, -8, 7, 0, 1);
    pl_set_xticks(axes, at, label, 4);
    pl_set_xticks(axes, at, label, 1);
    pdf = render_one("ticks set twice", figure);
    check(labels_are(&pdf, "-3", END), "setting the ticks again replaces the previous ones");
    free(pdf.bytes);
}

/* Le scritte: caratteri da proteggere e caratteri che non si sanno scrivere. */
static void test_text(void) {
    PlAxes *axes;
    PlFigure *figure = bare_figure(&axes, 0, 1, 0, 1);
    pl_set_title(axes, "f(x) = a\\b");
    pl_suptitle(figure, "caf\xc3\xa9 a\tb");
    Content pdf = render_one("text", figure);
    check(has(&pdf, "(f\\(x\\) = a\\\\b) Tj"), "parentheses and backslashes in a title are escaped");
    check(has(&pdf, "(caf?? a?b) Tj"), "bytes that are not printable ASCII are replaced by question marks");
    check(has(&pdf, "BT /F1 12 Tf %.2f ", FRAME_LEFT + FRAME_WIDTH / 2 - (278 + 333 + 500 + 333 + 278 + 584 + 278 + 556 + 278 + 556) * 12.0 / 1000 / 2),
          "a title is centred over its panel, using the Helvetica widths");
    free(pdf.bytes);

    /* Un titolo cambiato o tolto: resta l'ultima scelta. */
    figure = bare_figure(&axes, 0, 1, 0, 1);
    pl_set_title(axes, "first");
    pl_set_title(axes, "second");
    pl_suptitle(figure, "removed");
    pl_suptitle(figure, NULL);
    pdf = render_one("text set twice", figure);
    check(labels_are(&pdf, "second", END), "setting a title again replaces it, and NULL removes it");
    free(pdf.bytes);
}

/* I limiti automatici, e il fatto che i dati vengono copiati. */
static void test_limits(void) {
    PlAxes *axes;

    /* Dati da 0 a 10: limiti da -0.5 a 10.5, e il primo punto a un
     * ventiduesimo della cornice. Dopo la chiamata gli array vengono
     * sporcati: nella pagina devono esserci i valori di prima. */
    double x[] = {0, 10}, y[] = {0, 10};
    char name[] = "copied";
    PlFigure *figure = pl_figure(6.4, 4.8);
    axes = pl_add_axes(figure, 1);
    pl_set_xticks(axes, NULL, NULL, 0);
    pl_set_yticks(axes, NULL, NULL, 0);
    pl_plot(axes, x, y, 2, PL_BLUE, 1.5, name);
    pl_legend(axes);
    x[0] = y[0] = x[1] = y[1] = 12345;
    name[0] = 'X';
    Content pdf = render_one("automatic limits", figure);
    check(has(&pdf, "%.2f %.2f m\n", FRAME_LEFT + FRAME_WIDTH / 22, FRAME_BOTTOM + FRAME_HEIGHT / 22) &&
              has(&pdf, "%.2f %.2f l\n", FRAME_LEFT + FRAME_WIDTH * 21 / 22, FRAME_BOTTOM + FRAME_HEIGHT * 21 / 22),
          "automatic limits leave a 5% margin around the data");
    check(has(&pdf, "(copied) Tj"), "data and strings are copied: the caller may change its arrays right after the call");
    free(pdf.bytes);

    /* Una linea orizzontale non ha estensione in y: i limiti si allargano
     * attorno al suo valore, e la linea sta a metà altezza. Con y = 4 vanno
     * da 3.78 a 4.22: la prima tacca è 3.8, l'ultima 4.2. */
    double across[] = {0, 10}, flat[] = {4, 4};
    figure = pl_figure(6.4, 4.8);
    axes = pl_add_axes(figure, 1);
    pl_plot(axes, across, flat, 2, PL_BLUE, 1.5, NULL);
    pl_set_xticks(axes, NULL, NULL, 0);
    pdf = render_one("limits of a constant", figure);
    check(has(&pdf, " %.2f m\n", FRAME_BOTTOM + FRAME_HEIGHT / 2), "a constant line is drawn at mid height");
    check(labels_are(&pdf, "3.8", "3.85", "3.9", "3.95", "4", "4.05", "4.1", "4.15", "4.2", END), "the limits of a constant 4 are 3.78..4.22, as in matplotlib");
    free(pdf.bytes);

    /* Un marcatore è un dato: da solo, sta al centro del pannello. */
    figure = pl_figure(6.4, 4.8);
    axes = pl_add_axes(figure, 1);
    pl_marker(axes, 5, -5, PL_RED, 1);
    pl_set_xticks(axes, NULL, NULL, 0);
    pl_set_yticks(axes, NULL, NULL, 0);
    pdf = render_one("limits of a marker", figure);
    check(has(&pdf, "%.2f %.2f m\n", FRAME_LEFT + FRAME_WIDTH / 2 + PL_MARKER_RADIUS, FRAME_BOTTOM + FRAME_HEIGHT / 2), "a marker counts as data for the automatic limits");
    free(pdf.bytes);
}

/* Ritorna la posizione di needle nel contenuto, o -1 se non c'è. */
static long position_of(const Content *content, const char *needle) {
    const char *at = content->bytes ? strstr(content->bytes, needle) : NULL;
    return at ? at - content->bytes : -1;
}

/* L'ordine degli strati: le bande sotto, i marcatori sopra, comunque siano
 * ordinate le chiamate. */
static void test_layers(void) {
    PlAxes *axes;
    double x[] = {0, 1}, y[] = {0, 1};
    PlFigure *figure = bare_figure(&axes, 0, 1, 0, 1);
    pl_marker(axes, 0.5, 0.5, PL_RED, 1);
    pl_plot(axes, x, y, 2, PL_BLUE, 1.5, "line");
    pl_axvspan(axes, 0, 0.5, PL_ORANGE);
    pl_legend(axes);
    Content pdf = render_one("layers", figure);

    long clip = position_of(&pdf, " re W n\n"), band = position_of(&pdf, "1.000 0.875 0.764 rg"), line = position_of(&pdf, " l\n");
    long unclip = position_of(&pdf, "\nQ\n0.80 w\n"), frame = position_of(&pdf, " re S\n"), marker = position_of(&pdf, " c\n");
    long legend = position_of(&pdf, "(line) Tj");
    check(clip > 0 && clip < band && band < line, "a band is drawn under the lines even when it is requested after them");
    check(line < unclip && unclip < frame, "lines are drawn inside the clipping rectangle, the frame after it is released");
    check(frame < marker && marker < legend, "markers are drawn above the frame and outside the clipping, the legend last");
    free(pdf.bytes);

    /* La legenda senza linee con un nome, o non chiesta, non c'è. */
    figure = bare_figure(&axes, 0, 1, 0, 1);
    pl_plot(axes, x, y, 2, PL_BLUE, 1.5, NULL);
    pl_legend(axes);
    PlAxes *second = pl_add_axes(figure, 1);
    pl_set_xticks(second, NULL, NULL, 0);
    pl_set_yticks(second, NULL, NULL, 0);
    pl_plot(second, x, y, 2, PL_BLUE, 1.5, "not shown");
    pdf = render_one("no legend", figure);
    check(count_of(pdf.bytes, " Tj") == 0 && count_of(pdf.bytes, " re B\n") == 0, "no legend is drawn without labelled lines, or when it is not requested");
    free(pdf.bytes);
}

/* Ritorna 1 se la figura viene rifiutata da pl_pdf_savefig(); la libera. */
static int is_refused(PlPdf *pdf, PlFigure *figure) {
    int refused = !pl_pdf_savefig(pdf, figure);
    pl_close(figure);
    return refused;
}

/* Ciò che non si può fare non lascia tracce: né un file né una pagina. */
static void test_failures(void) {
    char path[1024];
    double zero = 0, nan = zero / zero, infinity = 1 / zero;
    PlAxes *axes;
    PlFigure *figure;

    check(pl_pdf_open("/no-such-folder-for-mathresearcher/plot.pdf") == NULL, "pl_pdf_open on an impossible path returns NULL");
    check(pl_pdf_open(NULL) == NULL, "pl_pdf_open without a path returns NULL");

    /* Le figure che non si possono disegnare vengono rifiutate, e nel file
     * non resta nulla di loro: alla fine c'è la sola pagina buona. */
    temporary_path(path, sizeof path, "failures");
    PlPdf *pdf = pl_pdf_open(path);
    check(pdf != NULL && !pl_pdf_savefig(pdf, NULL), "saving a NULL figure fails");
    figure = bare_figure(&axes, 0, 1, 0, 1);
    check(!pl_pdf_savefig(NULL, figure), "saving into a NULL file fails");
    pl_close(figure);

    figure = bare_figure(&axes, nan, 1, 0, 1);
    check(is_refused(pdf, figure), "a figure with a NaN limit is not saved");
    figure = bare_figure(&axes, 0, 1, -infinity, infinity);
    check(is_refused(pdf, figure), "a figure with infinite limits is not saved");
    figure = bare_figure(&axes, -1e308, 1e308, 0, 1);
    check(is_refused(pdf, figure), "a figure whose limits are too far apart for a double is not saved");
    figure = pl_figure(6.4, 4.8);
    pl_add_axes(figure, 1);
    pl_add_axes(figure, 0);
    check(is_refused(pdf, figure), "a figure with a panel of weight zero is not saved");
    figure = pl_figure(6.4, 4.8);
    pl_add_axes(figure, nan);
    check(is_refused(pdf, figure), "a figure with a panel of weight NaN is not saved");
    figure = pl_figure(0.5, 0.5);
    pl_add_axes(figure, 1);
    check(is_refused(pdf, figure), "a figure smaller than its margins is not saved");
    figure = pl_figure(nan, 4.8);
    check(is_refused(pdf, figure), "a figure with a NaN size is not saved");
    figure = pl_figure(6.4, infinity);
    check(is_refused(pdf, figure), "a figure with an infinite size is not saved");

    /* Un array che manca, con un numero di punti positivo. */
    double points[] = {0, 1};
    figure = bare_figure(&axes, 0, 1, 0, 1);
    pl_plot(axes, NULL, points, 2, PL_BLUE, 1.5, NULL);
    check(is_refused(pdf, figure), "a figure with a line whose x array is missing is not saved");
    figure = bare_figure(&axes, 0, 1, 0, 1);
    pl_set_xticks(axes, NULL, NULL, 2);
    check(is_refused(pdf, figure), "a figure with ticks whose positions are missing is not saved");

    /* Limiti uguali non sono un errore: si allargano, come in matplotlib. */
    figure = bare_figure(&axes, 2, 2, 0, 0);
    check(!is_refused(pdf, figure), "equal limits are widened, not refused");
    check(pl_pdf_close(pdf), "after refused figures the file is still closed correctly");
    Content content = read_content(path);
    remove(path);
    const char *problem = structure_problem(&content, 1);
    check(problem == NULL, problem ? problem : "refused figures leave nothing in the file: it has exactly the one good page");
    free(content.bytes);

    /* Un file senza pagine: ben formato, ma pl_pdf_close() dice di no. */
    temporary_path(path, sizeof path, "no_pages");
    pdf = pl_pdf_open(path);
    check(pdf != NULL && !pl_pdf_close(pdf), "closing a file with no pages returns 0");
    content = read_content(path);
    remove(path);
    problem = structure_problem(&content, 0);
    check(problem == NULL, problem ? problem : "a file with no pages is still well formed");
    free(content.bytes);
    check(!pl_pdf_close(NULL), "closing a NULL file returns 0");

    /* Ogni funzione accetta un puntatore nullo e non fa nulla. */
    double value[] = {0, 1};
    const char *const label[] = {"a", "b"};
    check(pl_add_axes(NULL, 1) == NULL, "adding a panel to a NULL figure returns NULL");
    pl_suptitle(NULL, "title");
    pl_set_title(NULL, "title");
    pl_set_xlim(NULL, 0, 1);
    pl_set_ylim(NULL, 0, 1);
    pl_set_xticks(NULL, value, label, 2);
    pl_set_yticks(NULL, value, label, 2);
    pl_plot(NULL, value, value, 2, PL_BLUE, 1.5, "label");
    pl_axhline(NULL, 0, PL_BLACK, 1, 0);
    pl_axvline(NULL, 0, PL_BLACK, 1, 1);
    pl_axvspan(NULL, 0, 1, PL_GREEN);
    pl_marker(NULL, 0, 0, PL_RED, 1);
    pl_legend(NULL);
    pl_close(NULL);
    check(1, "every recording function accepts a NULL panel or figure");
}

/* Scrive figure in un file temporaneo, che poi cancella. Ritorna 1 se la
 * pagina è stata salvata e il file chiuso senza errori. */
static int can_be_saved(const PlFigure *figure) {
    char path[1024];
    temporary_path(path, sizeof path, "memory");
    PlPdf *pdf = pl_pdf_open(path);
    int saved = pl_pdf_savefig(pdf, figure);
    saved = pl_pdf_close(pdf) && saved;
    remove(path);
    return saved;
}

/* La memoria che finisce. La figura d'uso viene descritta più volte, facendo
 * fallire ogni volta un'allocazione più avanti: finché una è fallita, la
 * figura non deve diventare una pagina; quando nessuna fallisce, sì. */
static void test_out_of_memory(void) {
    int consistent = 1, exhausted = 0;
    for (long budget = 0;; budget++) {
        allocations_left = budget;
        allocation_refused = 0;
        PlFigure *figure = number_line_figure();

        /* Da qui la memoria c'è di nuovo: se la pagina non viene scritta è
         * perché la figura ricorda di essere incompleta. */
        allocations_left = -1;
        if (can_be_saved(figure) == allocation_refused) consistent = 0;
        pl_close(figure);
        if (!allocation_refused) break;
        exhausted++;
    }
    check(consistent, "a figure described while memory runs out is never saved, whichever allocation fails");
    check(exhausted > 20, "the out-of-memory test really made the allocations of the figure fail, one after the other");

    /* Aprire un file senza memoria: NULL, e nessun file lasciato sul disco. */
    char path[1024];
    temporary_path(path, sizeof path, "memory_open");
    remove(path);
    int opened = 0;
    for (long budget = 0; budget < 2; budget++) {
        allocations_left = budget;
        if (pl_pdf_open(path) != NULL) opened = 1;
    }
    allocations_left = -1;
    FILE *left = fopen(path, "rb");
    check(!opened && !left, "pl_pdf_open without memory returns NULL and leaves no file behind");
    if (left) fclose(left);

    /* Salvare senza memoria: la pagina è rifiutata per intero, e il file
     * resta buono per salvarla quando la memoria torna. */
    PlAxes *axes;
    PlFigure *figure = bare_figure(&axes, 0, 1, 0, 1);
    PlPdf *pdf = pl_pdf_open(path);
    allocations_left = 0;
    int refused = !pl_pdf_savefig(pdf, figure);
    allocations_left = -1;
    int saved = pl_pdf_savefig(pdf, figure);
    saved = pl_pdf_close(pdf) && saved;
    Content content = read_content(path);
    remove(path);
    pl_close(figure);
    check(refused && saved && structure_problem(&content, 1) == NULL, "a page refused for lack of memory leaves the file intact for the next one");
    free(content.bytes);
}

int main(void) {
    /* Tutti i gruppi vengono eseguiti anche se uno fallisce, così il
     * resoconto è completo. */
    test_whole_figure();
    test_pages_of_different_sizes();
    test_broken_lines();
    test_ticks();
    test_text();
    test_limits();
    test_layers();
    test_failures();
    test_out_of_memory();

    printf("%s: %d of %d plot checks passed\n", failures ? "FAIL" : "PASS", checks - failures, checks);
    return failures ? 1 : 0;
}
