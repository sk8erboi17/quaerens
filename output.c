/* =============================================================================
 * output.c: la generazione del documento finale
 * =============================================================================
 *
 * COSA FA
 *
 * Scrive il risultato del programma come documento: per ogni enunciato letto
 * la formula, poi la dimostrazione o le soluzioni che il motore ha trovato,
 * oppure il verdetto quando una dimostrazione non c'è, e sotto il grafico
 * quando il motore ne ha fatto uno. Il documento è un file LaTeX, con il
 * preambolo dei file in tests/; su richiesta lo fa compilare in PDF.
 *
 * Qui non si decide nulla e non si calcola nulla: si riporta ciò che il
 * motore ha già stabilito e ricontrollato. Se scrivere un risultato
 * richiedesse un conto, quel conto va fatto nel motore, dove viene
 * verificato, e consegnato qui come testo.
 *
 * COME È FATTO
 *
 * Due file, scritti insieme. Il documento .tex, una riga alla volta, con
 * fprintf. E il PDF dei grafici, che ha lo stesso nome con "-plots.pdf" in
 * fondo e una pagina per grafico: lo scrive plot.c, e il documento include
 * ogni pagina con \includegraphics. Il PDF dei grafici si apre al primo
 * grafico, così un documento senza grafici non lascia un file vuoto accanto.
 *
 * PERCHÉ IL PDF FINALE PASSA DA LATEX
 *
 * I grafici sono linee e poche scritte, e plot.c li scrive in PDF da solo.
 * Il documento invece è fatto di formule: comporle (frazioni, radici, indici,
 * allineamenti, andate a capo) è il mestiere di un compilatore LaTeX, e
 * riscriverne uno non sarebbe codice minimale. Per questo il PDF finale si
 * ottiene chiamando un compilatore già installato, l'unica cosa esterna su
 * cui il programma si appoggia; senza, resta il documento .tex, completo.
 *
 * COME STA NEL PROGRAMMA
 *
 * È un volume dello stesso libro di main.c, che lo include dopo il suo
 * capitolo 20. Usa ciò che main.c ha già presentato: il verdetto (capitolo
 * 13) e la riga che lo riassume, verdict_print() (capitolo 20). Di plot.c usa
 * l'apertura del PDF e l'aggiunta di una pagina. Il capitolo ha il numero
 * 22: la numerazione è una sola per main.c, parser.c e output.c.
 *
 * INDICE DEI CAPITOLI
 *
 *   22. Il documento finale
 */

/* =============================================================================
 * 22. IL DOCUMENTO FINALE
 * ============================================================================
 *
 * L'ordine delle chiamate: document_open(), poi per ogni file di enunciati
 * document_section(), document_list() che apre l'elenco, un document_item()
 * per enunciato, document_list() che lo chiude; alla fine document_close().
 */

/* Il documento finale in corso di scrittura, con il PDF dei suoi grafici. */
typedef struct {
    FILE *tex;              /* il documento LaTeX, aperto in scrittura */
    char tex_path[1024];    /* il suo percorso */
    int want_pdf;           /* 1 se alla fine va compilato in PDF */
    PlPdf *plots;           /* il PDF dei grafici; NULL finché non ce n'è uno */
    char plots_path[1040];  /* il suo percorso: quello del documento, con
                               "-plots.pdf" al posto dell'estensione */
    int plots_pages;        /* pagine scritte finora nel PDF dei grafici */
    int plots_failed;       /* 1 se un grafico non si è potuto scrivere */
} Document;

/* Scrive su out il testo, sostituendo i caratteri che in LaTeX hanno un
 * significato speciale: serve per riportare in un documento un nome di file o
 * un messaggio del programma, che possono contenere barre e graffe. */
static void put_escaped(FILE *out, const char *text) {
    for (; *text; text++) {
        if (*text == '\\') fputs("\\textbackslash{}", out);
        else if (*text == '^') fputs("\\^{}", out);
        else if (*text == '~') fputs("\\~{}", out);
        else if (strchr("{}_$%&#", *text)) fprintf(out, "\\%c", *text);
        else fputc(*text, out);
    }
}

/* Scrive su out, come testo LaTeX, la riga del verdetto. */
static void put_plain_text(FILE *out, const Verdict *verdict) {
    char line[512];
    FILE *memory = tmpfile();
    if (!memory) return;
    verdict_print(memory, verdict);
    rewind(memory);
    size_t length = fread(line, 1, sizeof line - 1, memory);
    line[length] = 0;
    fclose(memory);
    put_escaped(out, line);
}

/* Compila in PDF il documento LaTeX tex_path, mettendo il PDF nella stessa
 * cartella. Ritorna 1 se riesce.
 *
 * È l'unico punto in cui il programma si appoggia a qualcosa di esterno: un
 * compilatore LaTeX già installato, chiamato con system(). Si provano in
 * ordine i comandi di compilers[]; il documento .tex resta comunque, e chi
 * non ha un compilatore lo può usare altrove.
 *
 * Il compilatore lavora dentro la cartella del documento. Così il PDF dei
 * grafici, che il documento nomina senza cartella, viene trovato da tutti e
 * due i compilatori: uno cerca i file accanto al documento, l'altro nella
 * cartella da cui è stato lanciato. */
static int compile_pdf(const char *tex_path) {
    static const char *const compilers[] = {
        "cd '%s' && tectonic '%s' > /dev/null 2>&1",
        "cd '%s' && pdflatex -interaction=batchmode -halt-on-error '%s' > /dev/null 2>&1",
    };
    char directory[1024], command[4096];

    /* I percorsi vanno tra apici nel comando: uno che ne contiene non si può
     * passare in sicurezza. */
    if (strchr(tex_path, '\'') || strlen(tex_path) >= sizeof directory) return 0;
    strcpy(directory, tex_path);
    char *slash = strrchr(directory, '/');
    const char *name = slash ? tex_path + (slash - directory) + 1 : tex_path;
    if (slash) *slash = 0; else strcpy(directory, ".");

    for (size_t i = 0; i < sizeof compilers / sizeof *compilers; i++) {
        snprintf(command, sizeof command, compilers[i], directory, name);
        if (system(command) == 0) return 1;
    }
    return 0;
}

/* Comincia il documento *document nel file path e ne scrive il preambolo,
 * che è quello dei file in tests/ più il pacchetto per includere i grafici.
 * Con un nome che finisce in .pdf scrive il documento LaTeX con lo stesso
 * nome e l'estensione .tex, e document_close() poi lo compila. Ritorna NULL
 * se riesce, altrimenti il motivo, in italiano. */
static const char *document_open(Document *document, const char *path) {
    memset(document, 0, sizeof *document);
    size_t length = strlen(path);
    if (length < 5 || length >= sizeof document->tex_path) return "nome del documento non valido";
    strcpy(document->tex_path, path);
    document->want_pdf = strcmp(path + length - 4, ".pdf") == 0;
    if (document->want_pdf) strcpy(document->tex_path + length - 4, ".tex");
    strcpy(document->plots_path, document->tex_path);
    strcpy(document->plots_path + length - 4, "-plots.pdf");
    document->tex = fopen(document->tex_path, "w");
    if (!document->tex) return "impossibile scrivere il documento";
    fprintf(document->tex, "\\documentclass[11pt,a4paper]{article}\n\\usepackage[T1]{fontenc}\n"
                           "\\usepackage[utf8]{inputenc}\n\\usepackage[english]{babel}\n"
                           "\\usepackage{amsmath,amssymb}\n\\usepackage{graphicx}\n\\usepackage[margin=2cm]{geometry}\n"
                           "\\setlength{\\parindent}{0pt}\n\\setlength{\\emergencystretch}{3em}\n"
                           "\\allowdisplaybreaks\n\n\\title{Proofs}\n\\author{}\n\\date{}\n"
                           "\\begin{document}\n\\maketitle\n");
    return NULL;
}

/* Scrive il titolo di una parte del documento: il nome del file di enunciati
 * che sta per essere letto. */
static void document_section(Document *document, const char *title) {
    fprintf(document->tex, "\n\\section*{");
    put_escaped(document->tex, title);
    fprintf(document->tex, "}\n");
}

/* Apre (opening = 1) o chiude (opening = 0) l'elenco degli enunciati di un
 * file. */
static void document_list(Document *document, int opening) {
    fprintf(document->tex, opening ? "\\begin{enumerate}\n" : "\\end{enumerate}\n");
}

/* Scrive una voce dell'elenco: l'enunciato com'è scritto nel file (formula,
 * o NULL se l'\item non ha una formula leggibile), poi la dimostrazione del
 * verdetto, o in sua mancanza il verdetto stesso, e infine il grafico plot,
 * se non è NULL, con due righe che dicono come leggerlo.
 *
 * Il grafico diventa una pagina del PDF dei grafici, e la voce la include
 * per numero di pagina. Se il PDF non si può scrivere la voce resta senza
 * grafico, e document_close() lo segnala. */
static void document_item(Document *document, const char *formula, const Verdict *verdict, const PlFigure *plot) {
    FILE *tex = document->tex;
    fprintf(tex, "\\item ");

    /* La formula letta da "$\displaystyle ...$" comincia già con quel
     * comando: non lo si scrive due volte. */
    if (formula) fprintf(tex, strncmp(formula, "\\displaystyle", 13) == 0 ? "$%s$\n\n" : "$\\displaystyle %s$\n\n", formula);
    if (verdict->proof) {
        fprintf(tex, "%s\n\n", verdict->proof);
    } else {
        fprintf(tex, "\\textbf{No proof.} ");
        put_plain_text(tex, verdict);
        fprintf(tex, "\n\n");
    }
    if (!plot) return;

    /* La pagina del grafico, e la sua inclusione. Il nome è senza cartella:
     * i due file stanno insieme (vedi compile_pdf()). */
    if (!document->plots) document->plots = pl_pdf_open(document->plots_path);
    if (!document->plots || !pl_pdf_savefig(document->plots, plot)) { document->plots_failed = 1; return; }
    const char *slash = strrchr(document->plots_path, '/');
    fprintf(tex, "\\begin{center}\n\\includegraphics[page=%d,width=0.8\\textwidth]{%s}\n\\end{center}\n"
                 "{\\small \\textbf{Graph.} Each panel shows the two sides of one comparison, or one side and the axis when the other is $0$. "
                 "Green bands: where that comparison holds. Gray bands and dashed lines: where a side has no value. "
                 "Bottom line: where the whole condition holds.}\n\n",
            ++document->plots_pages, slash ? slash + 1 : document->plots_path);
}

/* Finisce il documento: chiude il file LaTeX e il PDF dei grafici e, se era
 * stato chiesto un PDF, lo fa compilare. Ritorna NULL se tutto è riuscito,
 * altrimenti che cosa non è riuscito, in italiano: il testo vale fino alla
 * chiamata successiva. */
static const char *document_close(Document *document) {
    static char problem[1200];
    problem[0] = '\0';
    fprintf(document->tex, "\\end{document}\n");
    if (fclose(document->tex) != 0) snprintf(problem, sizeof problem, "il documento %s non è stato scritto per intero", document->tex_path);
    if ((document->plots && !pl_pdf_close(document->plots)) || document->plots_failed)
        snprintf(problem, sizeof problem, "il PDF dei grafici %s non è stato scritto per intero", document->plots_path);
    else if (!problem[0] && document->want_pdf && !compile_pdf(document->tex_path))
        snprintf(problem, sizeof problem, "nessun compilatore LaTeX ha prodotto il PDF; resta il documento %s", document->tex_path);
    return problem[0] ? problem : NULL;
}
