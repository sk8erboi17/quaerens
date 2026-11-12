/* =============================================================================
 * plot: figure nel piano scritte in un PDF vettoriale, con la sola libreria C
 * =============================================================================
 *
 * COSA FA
 *
 * Disegna grafici nel piano (curve, rette di riferimento, bande, marcatori,
 * legenda) e li scrive in un file PDF, una figura per pagina. È un matplotlib
 * in miniatura, riscritto in C: stessi nomi, stesso aspetto predefinito, e
 * soltanto ciò che serve a illustrare un enunciato, per esempio il grafico
 * dei due membri di una disequazione con sotto la retta dei numeri su cui è
 * segnato l'insieme delle soluzioni.
 *
 *     PlFigure *figure = pl_figure(6.4, 4.8);          misure in pollici
 *     PlAxes *axes = pl_add_axes(figure, 1);           un pannello
 *     pl_plot(axes, x, y, count, PL_BLUE, 1.5, "f(x)");
 *     pl_axhline(axes, 0, PL_BLACK, 0.8, 0);
 *     pl_legend(axes);
 *
 *     PlPdf *pdf = pl_pdf_open("figure.pdf");
 *     int saved = pl_pdf_savefig(pdf, figure);         una pagina
 *     saved = pl_pdf_close(pdf) && saved;              1 se il file è buono
 *     pl_close(figure);
 *
 * COME È FATTO
 *
 * In due tempi, come matplotlib.
 *
 *   1. Registrare. Le chiamate che descrivono la figura (pl_plot, pl_axvspan,
 *      pl_set_xlim...) non disegnano nulla: copiano i dati e le scritte in
 *      una struttura in memoria (capitolo 2). Chi chiama può riusare o
 *      liberare i suoi array subito dopo.
 *   2. Disegnare. pl_pdf_savefig() percorre la struttura e scrive la pagina
 *      (capitoli da 3 a 8).
 *
 * Disegnare subito, a ogni chiamata, non si potrebbe: i limiti degli assi
 * dipendono da tutti i dati del pannello, l'altezza di un pannello dipende
 * da quanti altri ce ne sono, e una banda va messa sotto le curve anche se è
 * stata chiesta dopo.
 *
 * Il PDF è scritto a mano, un oggetto dopo l'altro, contando i byte: il
 * formato chiede in fondo al file un indice con la posizione di ogni oggetto
 * (capitolo 3). Le pagine sono testo non compresso, e il carattere è
 * Helvetica, uno dei quattordici che ogni lettore di PDF possiede: nel file
 * non c'è nessun carattere incorporato e nessun dato binario.
 *
 * Il file è deterministico: nessuna data, nessun identificatore casuale, i
 * numeri scritti con un formato fisso. Le stesse chiamate danno lo stesso
 * file, byte per byte. I numeri hanno il punto come separatore decimale
 * perché il programma non cambia mai il locale del C: chi lo cambiasse con
 * setlocale() otterrebbe virgole, e un PDF che non si apre.
 *
 * L'ASPETTO
 *
 * È quello predefinito di matplotlib: fondo bianco, cornice nera di 0.8
 * punti, tacche di 3.5 punti verso l'esterno, etichette in Helvetica a 10
 * punti (sotto la tacca e centrate per l'asse x, a sinistra della tacca e
 * allineate a destra per l'asse y), titoli a 12 punti, i quattro colori
 * blu, arancione, verde e rosso della sua tavolozza. Curve, rette e bande
 * sono tagliate alla cornice; i marcatori no, e stanno sopra a tutto. Una
 * sola differenza voluta: le linee finiscono esattamente sull'ultimo punto,
 * senza sporgere di mezzo spessore come in matplotlib.
 *
 * I pannelli di una figura sono impilati dall'alto in basso, larghi quanto
 * la figura meno i margini, con le altezze proporzionali ai pesi. I margini
 * sono fissi, in punti (capitolo 1): non si misura il testo per decidere
 * quanto spazio dargli, e basta finché le etichette dell'asse y sono numeri
 * di poche cifre.
 *
 * GLI ERRORI
 *
 * Le chiamate che registrano non ritornano nulla. Se manca la memoria, o
 * manca un array che doveva esserci, segnano la figura come fallita, e
 * pl_pdf_savefig() ritornerà 0 senza scrivere niente: chi chiama può
 * descrivere tutta la figura e controllare una volta sola, alla fine. Per lo stesso motivo ogni funzione accetta un
 * puntatore nullo al posto della figura, del pannello o del file, e non fa
 * nulla (se ritorna un valore, ritorna NULL o 0).
 *
 * pl_pdf_savefig() ritorna 0 anche per una figura che non si può disegnare:
 * limiti degli assi non finiti, un pannello con peso non positivo, una
 * pagina troppo piccola per i suoi margini. Meglio nessuna pagina che una
 * pagina sbagliata.
 *
 * Nel file non finisce mai un numero non finito: i punti con una coordinata
 * NaN o infinita interrompono la curva, come in matplotlib, ogni coordinata
 * passa da pl_map(), che la tiene entro un limite fisso, e uno spessore
 * negativo o NaN diventa zero, la linea più sottile.
 *
 * CHE COSA MANCA, E PERCHÉ BASTA
 *
 *   - Scale logaritmiche, griglia, tacche minori, nomi degli assi, pannelli
 *     affiancati: nessuna delle figure del programma li usa.
 *   - Testo: solo i caratteri ASCII stampabili, in un solo carattere
 *     tipografico; ogni altro byte diventa '?'. Il carattere '-' è disegnato
 *     come un vero segno meno, più lungo del trattino: in un grafico è
 *     quasi sempre un meno (capitolo 4). Le formule stanno nel documento
 *     LaTeX che include la figura, non nella figura.
 *   - Le etichette automatiche non usano la notazione scientifica: per
 *     intervalli come 0..10^40 sarebbero file di zeri (capitolo 6).
 *   - Trasparenza: una banda è una tinta piena messa sotto il resto, il
 *     colore mescolato col bianco. Su fondo bianco è ciò che si vedrebbe con
 *     l'opacità al 25%, e il file resta leggibile da qualunque programma.
 *   - Compressione: servirebbe zlib, che non è libreria standard. Una curva
 *     di 600 punti pesa circa 10 kB.
 *   - Le rette e le bande di riferimento non allargano i limiti automatici
 *     (in matplotlib sì): i limiti seguono i dati, cioè curve e marcatori.
 *
 * USO
 *
 * Il file non ha un main(): lo include main.c, e si compila anche da solo.
 * Usa soltanto la libreria standard, e nessuna funzione di <math.h>, così
 * non serve mai -lm. Tutti i nomi cominciano con pl_ (Pl per i tipi, PL_
 * per le costanti), per non scontrarsi con quelli del programma. I test
 * stanno in tests/plot/test_plot.c.
 *
 * INDICE DEI CAPITOLI
 *
 *    1. Include, misure, colori
 *    2. La figura in memoria
 *    3. Scrittura nel file PDF
 *    4. Operatori grafici e testo
 *    5. Dai dati alla pagina
 *    6. Tacche
 *    7. Disegno di un pannello
 *    8. Il file: apertura, pagine, chiusura
 */

/* =============================================================================
 * 1. INCLUDE, MISURE, COLORI
 * ============================================================================
 *
 * Tutte le lunghezze sono in punti tipografici, l'unità di un PDF: 72 punti
 * fanno un pollice. Dove non è detto altro, i valori sono quelli predefiniti
 * di matplotlib.
 */

#include <float.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PL_POINTS_PER_INCH 72.0

/* Impaginazione: i margini attorno alla pila dei pannelli, e lo spazio
 * riservato a ciò che sta tra un pannello e l'altro. */
#define PL_MARGIN_LEFT 54.0    /* contiene le etichette dell'asse y */
#define PL_MARGIN_RIGHT 18.0   /* l'ultima etichetta dell'asse x sporge di mezza larghezza */
#define PL_MARGIN_TOP 12.0     /* sopra il primo pannello, o sopra il titolo della figura */
#define PL_MARGIN_BOTTOM 28.0  /* contiene le etichette dell'asse x dell'ultimo pannello */
#define PL_PANEL_GAP 26.0      /* tra due pannelli: le etichette dell'asse x di quello sopra */
#define PL_TITLE_ROOM 20.0     /* in più, sopra un pannello che ha un titolo */
#define PL_SUPTITLE_ROOM 24.0  /* in più, in cima, se la figura ha un titolo */
#define PL_MAX_PAGE 14400.0    /* lato massimo di una pagina: 200 pollici, il limite dei lettori di PDF */

/* Cornice, tacche e scritte. */
#define PL_FRAME_WIDTH 0.8   /* spessore della cornice e delle tacche */
#define PL_TICK_LENGTH 3.5
#define PL_TICK_PAD 3.5      /* tra la punta della tacca e la sua etichetta */
#define PL_LABEL_SIZE 10.0   /* corpo delle etichette e della legenda */
#define PL_TITLE_SIZE 12.0   /* corpo dei titoli */
#define PL_TITLE_PAD 6.0     /* tra la cornice e la base del titolo del pannello */
#define PL_CAP_HEIGHT 0.718  /* altezza delle maiuscole e delle cifre di Helvetica, in parti del corpo */

/* Marcatori, tratteggio, bande. */
#define PL_MARKER_RADIUS 3.0  /* il marcatore 'o' di matplotlib è largo 6 punti */
#define PL_MARKER_EDGE 1.0    /* spessore del suo bordo */
#define PL_DASH_ON 3.7        /* tratto e spazio di una linea tratteggiata, in spessori della linea */
#define PL_DASH_OFF 1.6
#define PL_SPAN_ALPHA 0.25    /* quanto colore c'è nella tinta di una banda */
#define PL_AUTO_MARGIN 0.05   /* margine attorno ai dati nei limiti automatici, in parti dell'intervallo */
#define PL_EXPANDER 0.05      /* di quanto si allarga, per parte, un intervallo ridotto a un valore, in parti del valore */

/* Legenda. */
#define PL_LEGEND_INSET 5.0       /* tra il riquadro della legenda e la cornice */
#define PL_LEGEND_PAD 4.0         /* tra il riquadro e il suo contenuto */
#define PL_LEGEND_SAMPLE 20.0     /* lunghezza del campione di linea */
#define PL_LEGEND_SAMPLE_GAP 8.0  /* tra il campione e la scritta */
#define PL_LEGEND_ROW 15.0        /* da una riga alla successiva */
#define PL_LEGEND_EDGE 0.8        /* grigio del bordo: 0 nero, 1 bianco */

/* Tacche automatiche (capitolo 6). */
#define PL_MAX_INTERVALS 9.0   /* al più dieci tacche */
#define PL_MIN_INTERVALS 3.0   /* anche su un asse cortissimo se ne cerca più d'una */
#define PL_MIN_EXPONENT (-300) /* il passo più piccolo è 10^-300: sotto, 10^k non è più calcolabile */
#define PL_ROUNDING 1e-9       /* tolleranza relativa per gli errori di arrotondamento */
#define PL_LABEL_BYTES 32      /* spazio per un'etichetta automatica */

/* Limiti di sicurezza sulle coordinate scritte nel file. */
#define PL_FAR 15000.0  /* nessuna coordinata supera questo valore, che è oltre PL_MAX_PAGE */
#define PL_SLACK 0.01   /* la precisione con cui si scrivono le coordinate */

/* I colori di una figura: nero, grigio al 50% e i primi quattro della
 * tavolozza predefinita di matplotlib. */
typedef enum { PL_BLACK, PL_GRAY, PL_BLUE, PL_ORANGE, PL_GREEN, PL_RED } PlColor;

/* Rosso, verde e blu di ogni PlColor, tra 0 e 1.
 * Warning: se aggiungi un colore a PlColor, aggiungi qui la sua riga, nella
 * stessa posizione. */
static const double pl_rgb[][3] = {
    {0, 0, 0},
    {0.5, 0.5, 0.5},
    {31 / 255.0, 119 / 255.0, 180 / 255.0},  /* #1f77b4 */
    {255 / 255.0, 127 / 255.0, 14 / 255.0},  /* #ff7f0e */
    {44 / 255.0, 160 / 255.0, 44 / 255.0},   /* #2ca02c */
    {214 / 255.0, 39 / 255.0, 40 / 255.0},   /* #d62728 */
};

#define PL_COLORS (sizeof pl_rgb / sizeof *pl_rgb)  /* quante righe ha la tabella */

/* Una delle due direzioni del piano. Serve da indice: limiti, tacche e
 * scale di un pannello sono coppie, una voce per direzione. */
typedef enum { PL_X, PL_Y } PlDirection;

/* Ritorna 1 se value è un numero finito, 0 se è NaN o un infinito.
 *
 * Sta al posto di isfinite(), che è in <math.h>. Per un numero finito
 * value - value fa zero; infinito meno infinito fa NaN, come NaN meno NaN,
 * e un NaN non è uguale a niente, nemmeno a zero. */
static int pl_is_finite(double value) {
    return value - value == 0;
}

/* =============================================================================
 * 2. LA FIGURA IN MEMORIA
 * ============================================================================
 *
 * Una figura possiede una lista di pannelli, e ogni pannello una lista di
 * elementi da disegnare, più le sue scelte su limiti e tacche. Tutto è in
 * coordinate dei dati: dove finisce sulla pagina si decide al momento di
 * scriverla. Ogni array e ogni scritta ricevuti vengono copiati, e
 * pl_close() libera tutto.
 */

/* I nomi dei tre tipi vengono prima delle definizioni, più sotto ognuna col
 * suo commento, perché le strutture si citano a vicenda. */
typedef struct PlFigure PlFigure;
typedef struct PlAxes PlAxes;
typedef struct PlItem PlItem;

/* Che cosa disegna un elemento di un pannello. */
typedef enum {
    PL_LINE,   /* una spezzata per i punti dati: pl_plot() */
    PL_HLINE,  /* una retta orizzontale da bordo a bordo: pl_axhline() */
    PL_VLINE,  /* una retta verticale da bordo a bordo: pl_axvline() */
    PL_SPAN,   /* una banda verticale: pl_axvspan() */
    PL_MARKER  /* un marcatore tondo: pl_marker() */
} PlKind;

/* Un elemento del disegno: ciò che ha chiesto una chiamata a pl_plot(),
 * pl_axhline(), pl_axvline(), pl_axvspan() o pl_marker(). Gli elementi di un
 * pannello formano una lista nell'ordine delle chiamate, che è l'ordine in
 * cui le linee si coprono.
 * Invariante: x e y hanno count elementi ciascuno, e count è 0 per tutto ciò
 * che non è PL_LINE. */
struct PlItem {
    PlItem *next;   /* l'elemento registrato dopo, o NULL; posseduto */
    PlKind kind;    /* che cosa disegna, e quindi quali dei campi qui sotto contano */
    PlColor color;  /* il colore del tratto; per una banda, quello di cui è la tinta */
    double width;   /* PL_LINE, PL_HLINE, PL_VLINE: spessore del tratto, in punti */
    int dashed;     /* PL_HLINE, PL_VLINE: 1 tratteggiata, 0 continua */
    int filled;     /* PL_MARKER: 1 pieno, 0 vuoto (interno bianco) */
    double a, b;    /* PL_HLINE: y = a. PL_VLINE: x = a. PL_SPAN: da x = a a x = b. PL_MARKER: il punto (a, b) */
    double *x, *y;  /* PL_LINE: le coordinate dei punti; copie possedute */
    int count;      /* PL_LINE: quanti punti */
    char *label;    /* PL_LINE: la voce in legenda, copia posseduta, o NULL */
};

/* I limiti di un asse scelti con pl_set_xlim() o pl_set_ylim(): ai due bordi
 * della cornice stanno i valori low e high. Se set è 0 nessuno li ha scelti,
 * e si ricavano dai dati (capitolo 5). */
typedef struct {
    int set;           /* 1 se i limiti sono stati scelti */
    double low, high;  /* i valori ai due bordi, in coordinate dei dati; contano solo se set è 1 */
} PlLimits;

/* Le tacche di un asse scelte con pl_set_xticks() o pl_set_yticks(). Se set
 * è 0 nessuno le ha scelte, e sono automatiche (capitolo 6); set a 1 con
 * count a 0 vuol dire "nessuna tacca".
 * Invariante: at e label hanno count elementi, posseduti. */
typedef struct {
    int set;       /* 1 se le tacche sono state scelte */
    int count;     /* quante sono */
    double *at;    /* le posizioni, in coordinate dei dati */
    char **label;  /* le etichette; una voce NULL è una tacca senza scritta */
} PlTicks;

/* Un pannello: un riquadro con i suoi due assi e ciò che vi è disegnato (in
 * matplotlib si chiama Axes). limits e ticks hanno come indice PL_X o PL_Y. */
struct PlAxes {
    PlFigure *figure;  /* la figura a cui appartiene, per segnarvi gli errori; non posseduta */
    PlAxes *next;      /* il pannello sotto questo, o NULL; posseduto */
    double weight;     /* la sua parte dell'altezza libera è weight diviso la somma dei pesi */
    char *title;       /* il titolo sopra la cornice, copia posseduta, o NULL */
    PlLimits limits[2];    /* i limiti scelti per l'asse x e per l'asse y */
    PlTicks ticks[2];      /* le tacche scelte per l'asse x e per l'asse y */
    PlItem *first, *last;  /* gli elementi, nell'ordine delle chiamate; posseduti */
    int legend;        /* 1 se è stata chiesta la legenda */
};

/* Una figura: una pagina del PDF, con i suoi pannelli dall'alto in basso.
 * failed è 1 se una chiamata non ha potuto registrare ciò che le era
 * chiesto: da lì in poi la figura è incompleta, e non la si scrive. */
struct PlFigure {
    double width, height;  /* misure della pagina, in punti */
    char *title;           /* il titolo sopra tutti i pannelli, copia posseduta, o NULL */
    PlAxes *first, *last;  /* i pannelli; posseduti */
    int failed;            /* 1 se la figura è incompleta; non torna mai a 0 */
};

/* Ritorna una copia di text, che chi chiama dovrà liberare, o NULL se text è
 * NULL. Se manca la memoria ritorna NULL e segna figure come fallita. */
static char *pl_copy_text(PlFigure *figure, const char *text) {
    if (!text) return NULL;
    size_t bytes = strlen(text) + 1;
    char *copy = malloc(bytes);
    if (copy) memcpy(copy, text, bytes);
    else figure->failed = 1;
    return copy;
}

/* Ritorna una copia dei count numeri in values, che chi chiama dovrà
 * liberare, o NULL se count non è positivo. Se manca la memoria, o se values
 * è NULL pur dovendo contenere dei numeri, ritorna NULL e segna figure come
 * fallita. */
static double *pl_copy_numbers(PlFigure *figure, const double *values, int count) {
    if (count <= 0) return NULL;
    size_t bytes = (size_t)count * sizeof *values;
    double *copy = values ? malloc(bytes) : NULL;
    if (copy) memcpy(copy, values, bytes);
    else figure->failed = 1;
    return copy;
}

/* Crea una figura vuota di width per height pollici (matplotlib usa 6.4 per
 * 4.8). Ritorna NULL se manca la memoria. Va liberata con pl_close(). */
PlFigure *pl_figure(double width, double height) {
    PlFigure *figure = malloc(sizeof *figure);
    if (!figure) return NULL;
    *figure = (PlFigure){.width = width * PL_POINTS_PER_INCH, .height = height * PL_POINTS_PER_INCH};
    return figure;
}

/* Dà alla figura un titolo, scritto sopra tutti i pannelli; text a NULL lo
 * toglie. */
void pl_suptitle(PlFigure *figure, const char *text) {
    if (!figure) return;
    free(figure->title);
    figure->title = pl_copy_text(figure, text);
}

/* Aggiunge alla figura un pannello, sotto quelli che ha già, e lo ritorna.
 * I pannelli sono larghi quanto la figura e si dividono l'altezza in
 * proporzione ai pesi: con pesi 1, 1 e 0.3 il terzo è una striscia sottile.
 * weight deve essere positivo. Ritorna NULL, e segna la figura come fallita,
 * se manca la memoria. Il pannello è della figura: lo libera pl_close(). */
PlAxes *pl_add_axes(PlFigure *figure, double weight) {
    if (!figure) return NULL;
    PlAxes *axes = malloc(sizeof *axes);
    if (!axes) {
        figure->failed = 1;
        return NULL;
    }
    *axes = (PlAxes){.figure = figure, .weight = weight};

    /* In coda alla lista, che va dall'alto in basso. */
    if (figure->last) figure->last->next = axes;
    else figure->first = axes;
    figure->last = axes;
    return axes;
}

/* Dà al pannello un titolo, centrato sopra la cornice; text a NULL lo
 * toglie. */
void pl_set_title(PlAxes *axes, const char *text) {
    if (!axes) return;
    free(axes->title);
    axes->title = pl_copy_text(axes->figure, text);
}

/* Fissa i limiti dell'asse x: al bordo sinistro della cornice sta x = low,
 * al destro x = high. Con low maggiore di high l'asse è rovesciato. Se la
 * funzione non viene chiamata i limiti seguono i dati (capitolo 5). */
void pl_set_xlim(PlAxes *axes, double low, double high) {
    if (axes) axes->limits[PL_X] = (PlLimits){1, low, high};
}

/* Fissa i limiti dell'asse y: al bordo basso sta y = low, all'alto y = high.
 * Per il resto è come pl_set_xlim(). */
void pl_set_ylim(PlAxes *axes, double low, double high) {
    if (axes) axes->limits[PL_Y] = (PlLimits){1, low, high};
}

/* Libera le tacche scelte per un asse e lo riporta alle tacche automatiche. */
static void pl_clear_ticks(PlTicks *ticks) {
    for (int i = 0; i < ticks->count; i++) free(ticks->label[i]);
    free(ticks->label);
    free(ticks->at);
    *ticks = (PlTicks){0};
}

/* Sceglie le tacche dell'asse direction: count posizioni in at, ognuna con
 * la scritta di pari indice in label. È il corpo comune di pl_set_xticks() e
 * pl_set_yticks(), dove sono descritti gli argomenti. */
static void pl_set_ticks(PlAxes *axes, PlDirection direction, const double *at, const char *const *label, int count) {
    if (!axes) return;
    PlTicks *ticks = &axes->ticks[direction];
    pl_clear_ticks(ticks);
    ticks->set = 1;

    /* Le posizioni. Se non ce ne sono, l'asse resta senza tacche. */
    ticks->at = pl_copy_numbers(axes->figure, at, count);
    if (!ticks->at) return;

    /* Le scritte. calloc le mette tutte a NULL: se una copia fallisce a
     * metà, pl_clear_ticks() trova comunque count voci da liberare. */
    ticks->label = calloc((size_t)count, sizeof *ticks->label);
    if (!ticks->label) {
        axes->figure->failed = 1;
        return;
    }
    ticks->count = count;
    for (int i = 0; i < count; i++) ticks->label[i] = pl_copy_text(axes->figure, label ? label[i] : NULL);
}

/* Sceglie le tacche dell'asse x: count posizioni in at, in coordinate dei
 * dati, ognuna con la sua scritta in label. Con count a 0 l'asse non ha
 * tacche. label può essere NULL, o avere voci NULL: tacche senza scritta. Le
 * tacche fuori dai limiti dell'asse non vengono disegnate. Se la funzione
 * non viene chiamata le tacche sono automatiche (capitolo 6). */
void pl_set_xticks(PlAxes *axes, const double *at, const char *const *label, int count) {
    pl_set_ticks(axes, PL_X, at, label, count);
}

/* Sceglie le tacche dell'asse y, come pl_set_xticks() per l'asse x. */
void pl_set_yticks(PlAxes *axes, const double *at, const char *const *label, int count) {
    pl_set_ticks(axes, PL_Y, at, label, count);
}

/* Aggiunge in coda al pannello un elemento di tipo kind e colore color, con
 * tutto il resto a zero, e lo ritorna perché chi chiama lo completi. Ritorna
 * NULL se axes è NULL o se manca la memoria; nel secondo caso segna la
 * figura come fallita. */
static PlItem *pl_add_item(PlAxes *axes, PlKind kind, PlColor color) {
    if (!axes) return NULL;
    PlItem *item = malloc(sizeof *item);
    if (!item) {
        axes->figure->failed = 1;
        return NULL;
    }
    *item = (PlItem){.kind = kind, .color = color};
    if (axes->last) axes->last->next = item;
    else axes->first = item;
    axes->last = item;
    return item;
}

/* Disegna la spezzata che passa per i count punti (x[i], y[i]), con un
 * tratto di width punti (matplotlib usa 1.5). Un punto con una coordinata
 * NaN o infinita non viene disegnato e interrompe la linea: è il modo di non
 * unire i due rami di una curva ai lati di un asintoto. Se label non è NULL
 * la linea compare in legenda con quel nome. */
void pl_plot(PlAxes *axes, const double *x, const double *y, int count, PlColor color, double width, const char *label) {
    PlItem *item = pl_add_item(axes, PL_LINE, color);
    if (!item) return;
    item->width = width;
    item->label = pl_copy_text(axes->figure, label);
    item->x = pl_copy_numbers(axes->figure, x, count);
    item->y = pl_copy_numbers(axes->figure, y, count);

    /* count resta 0 se una delle due copie manca: nessuno leggerà un array
     * che non c'è. */
    if (item->x && item->y) item->count = count;
}

/* Disegna la retta orizzontale di altezza y, da un bordo all'altro del
 * pannello, con un tratto di width punti, tratteggiato se dashed non è 0. */
void pl_axhline(PlAxes *axes, double y, PlColor color, double width, int dashed) {
    PlItem *item = pl_add_item(axes, PL_HLINE, color);
    if (!item) return;
    item->a = y;
    item->width = width;
    item->dashed = dashed;
}

/* Disegna la retta verticale di ascissa x, da un bordo all'altro del
 * pannello, con un tratto di width punti, tratteggiato se dashed non è 0. */
void pl_axvline(PlAxes *axes, double x, PlColor color, double width, int dashed) {
    PlItem *item = pl_add_item(axes, PL_VLINE, color);
    if (!item) return;
    item->a = x;
    item->width = width;
    item->dashed = dashed;
}

/* Colora la banda verticale da x = low a x = high, alta quanto il pannello,
 * con una tinta chiara di color. La banda sta sotto a tutto il resto,
 * comunque siano ordinate le chiamate. Un estremo infinito porta la banda
 * fino al bordo; con un estremo NaN la banda non c'è. */
void pl_axvspan(PlAxes *axes, double low, double high, PlColor color) {
    PlItem *item = pl_add_item(axes, PL_SPAN, color);
    if (!item) return;
    item->a = low;
    item->b = high;
}

/* Disegna nel punto (x, y) un marcatore tondo largo 6 punti, come 'o' in
 * matplotlib: pieno se filled non è 0, altrimenti vuoto, con l'interno
 * bianco e il bordo colorato. I marcatori stanno sopra le linee e la
 * cornice, e non sono tagliati dalla cornice. */
void pl_marker(PlAxes *axes, double x, double y, PlColor color, int filled) {
    PlItem *item = pl_add_item(axes, PL_MARKER, color);
    if (!item) return;
    item->a = x;
    item->b = y;
    item->filled = filled;
}

/* Chiede la legenda: un riquadro nell'angolo in alto a destra del pannello,
 * con una riga per ogni linea che ha un nome. Senza linee con un nome non
 * viene disegnato nulla. */
void pl_legend(PlAxes *axes) {
    if (axes) axes->legend = 1;
}

/* Libera la figura con i suoi pannelli e tutto ciò che contengono. Dopo, né
 * figure né i pannelli ottenuti da pl_add_axes() si possono più usare. */
void pl_close(PlFigure *figure) {
    if (!figure) return;
    while (figure->first) {
        PlAxes *axes = figure->first;
        figure->first = axes->next;
        while (axes->first) {
            PlItem *item = axes->first;
            axes->first = item->next;
            free(item->x);
            free(item->y);
            free(item->label);
            free(item);
        }
        pl_clear_ticks(&axes->ticks[PL_X]);
        pl_clear_ticks(&axes->ticks[PL_Y]);
        free(axes->title);
        free(axes);
    }
    free(figure->title);
    free(figure);
}

/* =============================================================================
 * 3. SCRITTURA NEL FILE PDF
 * ============================================================================
 *
 * Un PDF è un elenco di oggetti numerati, seguito da un indice che dice a
 * quale byte del file comincia ciascuno. Chi lo legge parte dal fondo:
 *
 *     %PDF-1.4                     la versione del formato
 *     4 0 obj                      l'oggetto numero 4; lo 0 è la "generazione",
 *     << /Type /Page ... >>        che cambia solo nei file modificati
 *     endobj
 *     ...gli altri oggetti...
 *     xref                         l'indice (cross-reference table)
 *     0 7                          descrive i numeri da 0 a 6
 *     0000000000 65535 f           lo 0 non è un oggetto: è sempre scritto così
 *     0000000789 00000 n           l'oggetto 1 comincia al byte 789
 *     ...una riga per oggetto...
 *     trailer
 *     << /Size 7 /Root 1 0 R >>    quanti numeri, e qual è l'oggetto radice
 *     startxref
 *     1234                         il byte a cui comincia la parola "xref"
 *     %%EOF
 *
 * Le righe dell'indice sono lunghe esattamente 20 byte (dieci cifre per la
 * posizione, cinque per la generazione, una lettera, e lo spazio prima
 * dell'a capo conta): così chi legge trova la riga di un oggetto con una
 * moltiplicazione, senza scorrere il file. Per questo ogni byte scritto va
 * contato: una posizione sbagliata di uno, e il file non si apre.
 *
 * Gli oggetti sono dizionari, << /Nome valore ... >>, e si citano l'un
 * l'altro per numero: "2 0 R" vuol dire "l'oggetto 2". Dalla radice si
 * scende così:
 *
 *     Catalog  -->  Pages (l'elenco delle pagine)  -->  Page  -->  il contenuto
 *
 * Il contenuto di una pagina è uno "stream": un dizionario con la lunghezza
 * in byte, poi i byte tra le parole stream ed endstream. Sono istruzioni di
 * disegno in un piccolo linguaggio descritto nel capitolo 4. La lunghezza
 * serve a chi legge per saltare lo stream senza interpretarlo. Qui la si
 * conosce solo dopo averlo scritto, e allora al suo posto si mette il
 * riferimento a un oggetto che viene dopo e contiene soltanto quel numero
 * ("/Length 6 0 R"): così la pagina va dritta nel file, senza passare per
 * la memoria.
 *
 * Il carattere. Un PDF di norma porta con sé i caratteri tipografici che
 * usa. Fanno eccezione i quattordici "standard" (Helvetica, Times, Courier,
 * Symbol e le loro varianti), che ogni lettore deve avere di suo: basta
 * nominarli. Per questo l'oggetto del carattere è una riga sola.
 *
 * I numeri degli oggetti sono fissati così:
 *
 *     1  Catalog      2  Pages      3  il carattere Helvetica
 *     e per la pagina p, contando da 0:
 *     4 + 3p  Page      5 + 3p  il suo contenuto      6 + 3p  la lunghezza
 *
 * L'ordine nel file è un altro, ed è libero, perché ci pensa l'indice: prima
 * le pagine, man mano che arrivano, e i primi tre oggetti alla chiusura,
 * quando si sa quante pagine ci sono.
 */

#define PL_CATALOG 1
#define PL_PAGES 2
#define PL_FONT 3
#define PL_FIXED_OBJECTS 3     /* gli oggetti che non dipendono dal numero di pagine */
#define PL_OBJECTS_PER_PAGE 3

/* Un file PDF in corso di scrittura.
 * Invariante: start ha posto per tutti gli oggetti fissi e per quelli delle
 * pagine già scritte. */
typedef struct PlPdf PlPdf;
struct PlPdf {
    FILE *file;   /* aperto in scrittura binaria; chiuso da pl_pdf_close() */
    long offset;  /* i byte scritti finora, cioè la posizione del prossimo */
    long *start;  /* start[n - 1] è la posizione dell'oggetto numero n; posseduto */
    int pages;    /* le pagine scritte finora */
    int failed;   /* 1 se una scrittura è fallita: il file è da buttare */
};

/* Scrive nel file come fprintf(), e conta i byte scritti. Ogni byte del file
 * passa da qui: è ciò che rende giuste le posizioni dell'indice. Un errore
 * di scrittura viene annotato in pdf->failed e riferito da pl_pdf_close().
 * Warning: il compilatore non confronta gli argomenti con il formato. %.2f
 * vuole un double: una costante intera va scritta con il punto, 3.0 e non 3. */
static void pl_put(PlPdf *pdf, const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    int written = vfprintf(pdf->file, format, arguments);
    va_end(arguments);
    if (written < 0) pdf->failed = 1;
    else pdf->offset += written;
}

/* Comincia l'oggetto numero number: ne annota la posizione per l'indice e
 * scrive la riga che lo apre. Chi chiama scrive il resto, fino a "endobj". */
static void pl_begin_object(PlPdf *pdf, int number) {
    pdf->start[number - 1] = pdf->offset;
    pl_put(pdf, "%d 0 obj\n", number);
}

/* =============================================================================
 * 4. OPERATORI GRAFICI E TESTO
 * ============================================================================
 *
 * Il contenuto di una pagina è una fila di istruzioni in notazione
 * postfissa: prima i numeri, poi l'operatore che li usa. L'origine è
 * l'angolo in basso a sinistra, y cresce verso l'alto, l'unità è il punto.
 * Gli operatori usati in questo file:
 *
 *     x y m          comincia un tratto del cammino nel punto (x, y)
 *     x y l          aggiunge un segmento fino a (x, y)
 *     ... x y c      aggiunge una curva di Bézier fino a (x, y); prima, i due
 *                    punti di controllo che la tirano
 *     h              chiude il tratto tornando al suo inizio
 *     x y w h re     aggiunge un rettangolo
 *     S    f    B    traccia il cammino; lo riempie; lo riempie e lo traccia
 *     W n            il cammino diventa il ritaglio: fuori non si disegna più
 *     q ... Q        salva lo stato grafico, e lo ripristina
 *     s w            spessore del tratto
 *     1 j            giunzioni tonde tra i segmenti
 *     [a b] 0 d      tratteggio: a punti di tratto, b di spazio
 *     r g b RG       colore del tratto          r g b rg   del riempimento
 *     t G            grigio del tratto          t g        del riempimento
 *     BT /F1 s Tf x y Td (testo) Tj ET          scrive testo di corpo s in (x, y)
 *
 * Una regola tiene in ordine i colori: all'inizio della pagina tratto e
 * riempimento sono neri e il tratto è continuo, e chi cambia un colore o il
 * tratteggio lo fa tra q e Q. Fuori da quelle coppie, quindi, cornici,
 * tacche e scritte escono nere senza doverlo dire ogni volta. Lo spessore
 * invece lo dichiara chiunque tracci.
 */

/* Sceglie un colore. command è "RG" per il tratto, "rg" per il riempimento.
 * alpha, tra 0 e 1, dice quanto colore c'è: 1 è il colore pieno, 0 è bianco.
 * Un valore di color fuori dall'enum vale nero.
 *
 * Un colore c steso con opacità alpha su un fondo bianco (che vale 1) appare
 * alpha c + (1 - alpha) 1 = 1 - alpha (1 - c), componente per componente: è
 * il conto che si fa qui, al posto della trasparenza. */
static void pl_put_color(PlPdf *pdf, PlColor color, double alpha, const char *command) {
    /* Il confronto è tra numeri senza segno, così respinge anche i valori
     * negativi senza un secondo controllo. */
    const double *rgb = pl_rgb[(unsigned)color < PL_COLORS ? color : PL_BLACK];
    pl_put(pdf, "%.3f %.3f %.3f %s\n", 1 - alpha * (1 - rgb[0]), 1 - alpha * (1 - rgb[1]), 1 - alpha * (1 - rgb[2]), command);
}

/* Prepara un tratto: colore, spessore in punti, e tratteggio se dashed non è
 * 0. Va chiamata tra q e Q. */
static void pl_put_stroke(PlPdf *pdf, PlColor color, double width, int dashed) {
    /* Uno spessore negativo, enorme o NaN non si può scrivere in un PDF. Lo
     * zero per un PDF è la linea più sottile che il dispositivo sa fare. Il
     * primo confronto è scritto al negativo perché fermi anche un NaN, che
     * rende falso ogni confronto. */
    if (!(width > 0)) width = 0;
    if (width > PL_FAR) width = PL_FAR;

    /* Il tratteggio si dichiara sempre, anche per dire che non c'è:
     * altrimenti una linea continua lo erediterebbe da una tratteggiata
     * disegnata prima nello stesso blocco. */
    pl_put_color(pdf, color, 1, "RG");
    pl_put(pdf, "%.2f w\n", width);
    if (dashed) pl_put(pdf, "[%.2f %.2f] 0 d\n", PL_DASH_ON * width, PL_DASH_OFF * width);
    else pl_put(pdf, "[] 0 d\n");
}

/* Il rapporto tra la distanza dei punti di controllo e il raggio, per un
 * quarto di cerchio fatto con una curva di Bézier: (4/3)(sqrt(2) - 1). */
#define PL_CIRCLE_KAPPA 0.5523

/* Aggiunge al cammino un cerchio chiuso di centro (x, y).
 *
 * Il PDF non ha archi: ha le curve di Bézier cubiche, che vanno da un punto
 * a un altro tirate da due punti di controllo. Un quarto di cerchio di
 * raggio r si approssima bene mettendo i controlli sulle tangenti ai due
 * estremi, a distanza kappa r, con kappa come sopra: è il valore per cui la
 * curva passa esattamente per il punto di mezzo dell'arco, e l'errore sul
 * raggio resta sotto lo 0.03%. Quattro quarti, in senso antiorario a partire
 * da destra, fanno il cerchio. */
static void pl_put_circle(PlPdf *pdf, double x, double y, double radius) {
    double k = PL_CIRCLE_KAPPA * radius;
    pl_put(pdf, "%.2f %.2f m\n", x + radius, y);
    pl_put(pdf, "%.2f %.2f %.2f %.2f %.2f %.2f c\n", x + radius, y + k, x + k, y + radius, x, y + radius);
    pl_put(pdf, "%.2f %.2f %.2f %.2f %.2f %.2f c\n", x - k, y + radius, x - radius, y + k, x - radius, y);
    pl_put(pdf, "%.2f %.2f %.2f %.2f %.2f %.2f c\n", x - radius, y - k, x - k, y - radius, x, y - radius);
    pl_put(pdf, "%.2f %.2f %.2f %.2f %.2f %.2f c\nh\n", x + k, y - radius, x + radius, y - k, x + radius, y);
}

/* La larghezza di ogni carattere ASCII stampabile in Helvetica, dal codice
 * 32 (lo spazio) al 126, in millesimi del corpo: una cifra a 10 punti è
 * larga 5.56 punti. Sono le misure pubblicate del carattere (il suo file
 * AFM), che ogni lettore di PDF rispetta anche quando al posto di Helvetica
 * usa un suo sosia.
 *
 * Servono a centrare e ad allineare a destra: il PDF sa soltanto scrivere a
 * partire da un punto, e per finire in un punto bisogna sapere quanto è
 * lunga la scritta. Una riga della tabella copre sedici codici. L'apostrofo
 * (191) e l'accento grave (333) sono quelli diritti della codifica scelta in
 * pl_pdf_close().
 *
 * Il segno meno. Al codice del carattere '-' Helvetica ha il trattino, largo
 * 333: va bene tra due parole, ma davanti a un numero è corto, e accanto a
 * un + (largo 584) si vede. Il carattere possiede anche un vero meno, largo
 * quanto il +, e l'oggetto del carattere (pl_pdf_close()) lo mette al posto
 * del trattino: per questo qui la larghezza di '-' è 584.
 * Warning: le due scelte vanno insieme. Per tornare al trattino si toglie
 * /Differences dall'oggetto del carattere e si rimette 333 qui. */
static const short pl_glyph_width[] = {
    278, 278, 355, 556, 556, 889, 667, 191, 333, 333, 389, 584, 278, 584, 278, 278,   /* da ' ' a '/' */
    556, 556, 556, 556, 556, 556, 556, 556, 556, 556, 278, 278, 584, 584, 584, 556,   /* da '0' a '?' */
    1015, 667, 667, 722, 722, 667, 611, 778, 722, 278, 500, 667, 556, 833, 722, 778,  /* da '@' a 'O' */
    667, 778, 722, 667, 611, 722, 667, 944, 667, 667, 611, 278, 278, 278, 469, 556,   /* da 'P' a '_' */
    333, 556, 556, 500, 556, 556, 278, 556, 556, 222, 222, 500, 222, 833, 556, 556,   /* da '`' ad 'o' */
    556, 556, 333, 500, 278, 556, 500, 722, 500, 500, 500, 334, 260, 334, 584,        /* da 'p' a '~' */
};

/* Una voce mancante farebbe leggere fuori dalla tabella per il carattere '~'. */
_Static_assert(sizeof pl_glyph_width == ('~' - ' ' + 1) * sizeof *pl_glyph_width, "pl_glyph_width deve avere una voce per carattere, da ' ' a '~'");

/* Ritorna c se è un carattere ASCII stampabile, altrimenti '?': è ciò che
 * viene scritto, e misurato, al posto di un byte che non si sa disegnare. */
static int pl_printable(char c) {
    return c >= 32 && c <= 126 ? c : '?';
}

/* Ritorna la larghezza in punti di text scritto in corpo size. */
static double pl_text_width(const char *text, double size) {
    double thousandths = 0;
    for (; *text; text++) thousandths += pl_glyph_width[pl_printable(*text) - 32];
    return thousandths * size / 1000;
}

/* Scrive text in nero, in corpo size, con la linea di base all'altezza y.
 * anchor dice quale punto della scritta sta in x: 0 l'inizio, 0.5 il centro,
 * 1 la fine. Non fa nulla se text è NULL. Va chiamata fuori da q e Q, dove
 * il riempimento è nero. */
static void pl_put_text(PlPdf *pdf, double x, double y, double size, double anchor, const char *text) {
    if (!text) return;
    pl_put(pdf, "BT /F1 %.0f Tf %.2f %.2f Td (", size, x - anchor * pl_text_width(text, size), y);

    /* In un PDF una stringa sta tra parentesi tonde: quelle del testo, e la
     * barra rovesciata che serve a proteggerle, vanno precedute da una
     * barra rovesciata. */
    for (; *text; text++) {
        int c = pl_printable(*text);
        if (c == '(' || c == ')' || c == '\\') pl_put(pdf, "\\");
        pl_put(pdf, "%c", c);
    }
    pl_put(pdf, ") Tj ET\n");
}

/* =============================================================================
 * 5. DAI DATI ALLA PAGINA
 * ============================================================================
 *
 * Ogni asse di un pannello è una scala: un intervallo di valori steso su un
 * tratto di pagina. Qui si decide l'intervallo (i limiti scelti, o quelli
 * ricavati dai dati) e si converte un valore in una posizione.
 */

/* Una scala: l'intervallo di dati da low a high steso sul tratto di pagina
 * lungo length punti che comincia a start. low sta in start, high in
 * start + length.
 * Invariante, dopo pl_limits(): low e high sono finiti e diversi; low può
 * essere il maggiore, e allora l'asse è rovesciato. length è positivo. */
typedef struct {
    double low, high;      /* i valori ai due capi, in coordinate dei dati */
    double start, length;  /* dove comincia e quanto è lungo il tratto, in punti */
} PlScale;

/* Il posto di un pannello sulla pagina e la parte di piano che mostra: una
 * scala per direzione, con indice PL_X o PL_Y. La cornice è il rettangolo
 * che le due scale coprono. */
typedef struct {
    PlScale scale[2];
} PlFrame;

/* Ritorna la posizione sulla pagina, in punti, del valore value.
 *
 * Il risultato è sempre un numero finito tra -PL_FAR e PL_FAR, anche se
 * value è enorme, infinito o NaN (un NaN finisce a -PL_FAR). Vicino a un
 * asintoto una curva ha punti con ordinate come 10^15, che scritte nel file
 * supererebbero ciò che un lettore di PDF accetta. Portarle a PL_FAR non
 * cambia il disegno: il limite è oltre il bordo di qualunque pagina, quindi
 * il punto resta fuori dalla cornice e dalla stessa parte, e il segmento
 * che lo raggiunge esce dalla cornice tra gli stessi due campioni di prima.
 * Su una pagina ordinaria, trenta volte più piccola di PL_FAR, il punto di
 * uscita si sposta di meno di un trentesimo della distanza tra i campioni. */
static double pl_map(const PlScale *scale, double value) {
    double at = scale->start + (value - scale->low) / (scale->high - scale->low) * scale->length;

    /* Scritto al negativo perché fermi anche un NaN. */
    if (!(at > -PL_FAR)) at = -PL_FAR;
    if (at > PL_FAR) at = PL_FAR;
    return at;
}

/* Allarga l'intervallo da *low a *high quanto basta a contenere il punto
 * (x, y), guardato lungo direction. Un punto con una coordinata non finita
 * non verrà disegnato, e non conta. */
static void pl_cover(double x, double y, PlDirection direction, double *low, double *high) {
    double value = direction == PL_X ? x : y;
    if (!pl_is_finite(x) || !pl_is_finite(y)) return;
    if (value < *low) *low = value;
    if (value > *high) *high = value;
}

/* Mette in scale->low e scale->high i limiti dell'asse direction del
 * pannello. Ritorna 1 se sono due numeri finiti e diversi, cioè se l'asse si
 * può disegnare; altrimenti 0.
 *
 * Le regole sono quelle di matplotlib:
 *
 *   5a. Senza limiti scelti si prende l'intervallo coperto dai dati, cioè
 *       dai punti delle linee e dai marcatori. Senza dati, da 0 a 1.
 *   5b. Un intervallo ridotto a un solo valore v (una retta orizzontale, o
 *       limiti scelti uguali) non ha estensione, e lo si allarga del 5% di
 *       |v| per parte; di 0.05 se v è zero.
 *   5c. Ai limiti ricavati dai dati si aggiunge per parte un margine del 5%
 *       dell'intervallo, così la curva non tocca la cornice. */
static int pl_limits(const PlAxes *axes, PlDirection direction, PlScale *scale) {
    const PlLimits *chosen = &axes->limits[direction];
    scale->low = chosen->low;
    scale->high = chosen->high;

    /* 5a. Si parte da un intervallo rovesciato, che il primo punto raddrizza:
     * se alla fine è ancora rovesciato, di punti non ce n'erano. */
    if (!chosen->set) {
        scale->low = DBL_MAX;
        scale->high = -DBL_MAX;
        for (const PlItem *item = axes->first; item; item = item->next) {
            if (item->kind == PL_MARKER) pl_cover(item->a, item->b, direction, &scale->low, &scale->high);
            for (int i = 0; i < item->count; i++) pl_cover(item->x[i], item->y[i], direction, &scale->low, &scale->high);
        }
        if (scale->low > scale->high) {
            scale->low = 0;
            scale->high = 1;
            return 1;
        }
    }

    /* 5b. */
    if (scale->low == scale->high) {
        double size = scale->low < 0 ? -scale->low : scale->low;
        double pad = PL_EXPANDER * (size > 0 ? size : 1);
        scale->low -= pad;
        scale->high += pad;
    }

    /* 5c. */
    if (!chosen->set) {
        double pad = PL_AUTO_MARGIN * (scale->high - scale->low);
        scale->low -= pad;
        scale->high += pad;
    }

    /* La differenza è finita solo se lo sono tutti e due i limiti, e copre
     * anche il caso di due limiti finiti ma troppo lontani per un double. */
    return scale->low != scale->high && pl_is_finite(scale->high - scale->low);
}

/* =============================================================================
 * 6. TACCHE
 * ============================================================================
 *
 * Le tacche di un asse sono quelle scelte da chi chiama, oppure sono
 * automatiche: multipli di un passo "bello", scelto come farebbe una persona
 * che gradua un righello.
 *
 * Un passo è bello se è 1, 2 o 5 volte una potenza di dieci: 0.02, 0.5, 1,
 * 20, 5000. Sono i soli passi i cui multipli si leggono a colpo d'occhio e
 * tra i quali cade sempre lo zero. Si cerca il più piccolo che non faccia
 * troppe tacche:
 *
 *   6a. Quanti intervalli al massimo. Le etichette non devono toccarsi: si
 *       conta un intervallo ogni tre altezze di carattere sull'asse x (le
 *       scritte sono affiancate) e ogni due sull'asse y (sono impilate), e
 *       comunque non più di 9 né meno di 3. È la regola di matplotlib.
 *   6b. Il passo grezzo è la lunghezza dell'intervallo divisa per quel
 *       massimo: un passo più piccolo farebbe troppe tacche. Lo si scrive
 *       come m 10^k con m tra 1 e 10, cercando k a forza di moltiplicazioni
 *       (non c'è il logaritmo: <math.h> non si usa).
 *   6c. Si arrotonda m in su al primo tra 1, 2, 5, 10. Il passo è quello.
 *   6d. Le tacche sono i multipli interi del passo che cadono nei limiti.
 *
 * Esempi, con al più 9 intervalli. Da 0 a 1: grezzo 0.111, passo 0.2, tacche
 * 0, 0.2, ... 1. Da -3 a 7: grezzo 1.11, passo 2, tacche -2, 0, 2, 4, 6.
 *
 * Tra un passo bello e il successivo c'è al più un fattore 2.5, quindi gli
 * intervalli sono più dei due quinti del massimo: su un asse lungo le tacche
 * vanno da 3 a 10, di solito da 5 a 9, e anche sul più corto ce n'è almeno
 * una.
 *
 * Le etichette hanno le cifre decimali che servono al passo (una per 0.5,
 * due per 0.02), meno gli zeri finali: -2, -1.5, 0, 0.5.
 */

/* Disegna sull'asse direction la tacca del valore value, con la scritta
 * label se non è NULL. Per l'asse x la tacca scende dal bordo basso della
 * cornice e la scritta le sta sotto, centrata; per l'asse y la tacca esce
 * dal bordo sinistro e la scritta le sta a sinistra, allineata a destra e
 * centrata in altezza. Una tacca fuori dalla cornice non viene disegnata. */
static void pl_draw_tick(PlPdf *pdf, const PlFrame *frame, PlDirection direction, double value, const char *label) {
    const PlScale *scale = &frame->scale[direction];
    double at = pl_map(scale, value);

    /* Il bordo da cui la tacca esce: quello basso per l'asse x, quello
     * sinistro per l'asse y. In entrambi i casi è l'inizio dell'altra scala. */
    double edge = frame->scale[direction == PL_X ? PL_Y : PL_X].start;
    double tip = edge - PL_TICK_LENGTH, beyond = tip - PL_TICK_PAD;  /* la punta della tacca, e dove comincia la scritta */

    /* Il confronto si fa sulla pagina, con la tolleranza di scrittura, e non
     * sui dati: una tacca automatica che per arrotondamento cade un soffio
     * fuori dal limite deve restare. Scritto al negativo per i NaN. */
    if (!(at >= scale->start - PL_SLACK && at <= scale->start + scale->length + PL_SLACK)) return;

    if (direction == PL_X) {
        pl_put(pdf, "%.2f %.2f m %.2f %.2f l S\n", at, edge, at, tip);
        pl_put_text(pdf, at, beyond - PL_LABEL_SIZE * PL_CAP_HEIGHT, PL_LABEL_SIZE, 0.5, label);
    } else {
        pl_put(pdf, "%.2f %.2f m %.2f %.2f l S\n", edge, at, tip, at);
        pl_put_text(pdf, beyond, at - PL_LABEL_SIZE * PL_CAP_HEIGHT / 2, PL_LABEL_SIZE, 1, label);
    }
}

/* Ritorna value per dieci alla exponent.
 *
 * Con un esponente negativo divide per 10^-exponent invece di moltiplicare
 * per 0.1 più volte: 0.1 non è esatto in binario e l'errore crescerebbe a
 * ogni passo, mentre 10^n è esatto fino a n = 22 e la divisione arrotonda
 * una volta sola. Così 3 per 10^-1 dà il double più vicino a 0.3, quello che
 * stampato dà "0.3". Oltre 10^308 la potenza è infinita: il risultato è
 * infinito, o zero se l'esponente è negativo. */
static double pl_times_pow10(double value, int exponent) {
    double power = 1;
    for (int i = exponent < 0 ? -exponent : exponent; i > 0; i--) power *= 10;
    return exponent < 0 ? value / power : value * power;
}

/* Ritorna il più grande intero non maggiore di value, che deve stare
 * largamente in un long long (in valore assoluto sotto 9 10^18). Sta al
 * posto di floor(), che è in <math.h>. */
static long long pl_floor(double value) {
    /* La conversione tronca verso lo zero: per i negativi non interi
     * arrotonda in su, e va corretta. */
    long long whole = (long long)value;
    return (double)whole > value ? whole - 1 : whole;
}

/* Disegna le tacche dell'asse direction con le loro etichette: quelle scelte
 * da chi chiama, se ci sono, altrimenti quelle automatiche. I casi da 6a a
 * 6d sono descritti in testa al capitolo. */
static void pl_draw_ticks(PlPdf *pdf, const PlAxes *axes, const PlFrame *frame, PlDirection direction) {
    static const int nice[] = {1, 2, 5, 10};
    const PlTicks *ticks = &axes->ticks[direction];
    const PlScale *scale = &frame->scale[direction];

    /* Tacche scelte: si disegnano quelle, e non altre. */
    if (ticks->set) {
        for (int i = 0; i < ticks->count; i++) pl_draw_tick(pdf, frame, direction, ticks->at[i], ticks->label[i]);
        return;
    }

    /* 6a. I limiti in ordine crescente (l'asse può essere rovesciato) e il
     * massimo numero di intervalli. */
    double low = scale->low < scale->high ? scale->low : scale->high;
    double high = scale->low < scale->high ? scale->high : scale->low;
    double most = scale->length / (PL_LABEL_SIZE * (direction == PL_X ? 3 : 2));
    if (most > PL_MAX_INTERVALS) most = PL_MAX_INTERVALS;
    if (most < PL_MIN_INTERVALS) most = PL_MIN_INTERVALS;

    /* 6b. Dopo i due cicli 10^exponent <= raw < 10^(exponent + 1). Il primo
     * si ferma comunque a PL_MIN_EXPONENT, il secondo quando la potenza
     * diventa infinita. */
    double raw = (high - low) / most;
    int exponent = 0;
    while (exponent > PL_MIN_EXPONENT && pl_times_pow10(1, exponent) > raw) exponent--;
    while (pl_times_pow10(1, exponent + 1) <= raw) exponent++;

    /* 6c. La tolleranza fa sì che un intervallo come 0.6 diviso in 3 dia il
     * passo 0.2 anche quando il quoziente esce 0.2000000000000001. Il 10
     * chiude l'elenco: è l'1 della decina successiva. */
    double mantissa = pl_times_pow10(raw, -exponent);
    int choice = 0;
    while (nice[choice] < 10 && mantissa > nice[choice] * (1 + PL_ROUNDING)) choice++;
    double step = pl_times_pow10(nice[choice], exponent);

    /* 6d. Il primo e l'ultimo multiplo nei limiti; la tolleranza tiene le
     * tacche che cadono proprio su un limite. Il soffitto di v è l'opposto
     * del pavimento di -v.
     *
     * I due indici stanno in un long long: due double diversi distano almeno
     * 2^-53 volte il maggiore, quindi un limite diviso per l'intervallo è al
     * più 2^53, e diviso per il passo al più dieci volte tanto, circa 10^17.
     * E sono vicini: tra il primo e l'ultimo ci sono al più most intervalli. */
    long long first = -pl_floor(PL_ROUNDING - low / step), last = pl_floor(high / step + PL_ROUNDING);
    for (long long index = first; index <= last; index++) {
        /* Il valore si ricava dall'indice, non sommando il passo: così ogni
         * tacca è il double più vicino al suo valore decimale, zero compreso. */
        double value = pl_times_pow10((double)index * nice[choice], exponent);
        char label[PL_LABEL_BYTES];
        int length = snprintf(label, sizeof label, "%.*f", exponent < 0 ? -exponent : 0, value);

        /* Un'etichetta che non entra viene omessa, non troncata: un numero
         * senza le ultime cifre è un altro numero. Alle altre si tolgono gli
         * zeri in fondo alla parte decimale, e il punto se non resta altro;
         * solo se un punto c'è, perché gli zeri di "5000" sono cifre. */
        int fits = length < (int)sizeof label;
        if (fits && exponent < 0) {
            while (label[length - 1] == '0') length--;
            if (label[length - 1] == '.') length--;
            label[length] = 0;
        }
        pl_draw_tick(pdf, frame, direction, value, fits ? label : NULL);
    }
}

/* =============================================================================
 * 7. DISEGNO DI UN PANNELLO
 * ============================================================================
 *
 * Un pannello si disegna a strati, dal fondo alla superficie:
 *
 *   7a. dentro la cornice, col ritaglio: le bande, poi le linee;
 *   7b. la cornice, le tacche con le etichette, il titolo;
 *   7c. i marcatori, senza ritaglio: un marcatore sul bordo resta intero;
 *   7d. la legenda.
 */

/* Aggiunge al cammino la spezzata di line e la traccia. Un punto con una
 * coordinata non finita non viene scritto, e il punto dopo comincia un
 * tratto nuovo. Il tratto (colore, spessore) deve essere già stato scelto. */
static void pl_put_polyline(PlPdf *pdf, const PlItem *line, const PlFrame *frame) {
    int joined = 0;  /* 1 se il punto precedente è stato scritto: il prossimo si unisce a lui */
    int any = 0;     /* 1 se è stato scritto almeno un punto */
    for (int i = 0; i < line->count; i++) {
        if (!pl_is_finite(line->x[i]) || !pl_is_finite(line->y[i])) {
            joined = 0;
            continue;
        }
        pl_put(pdf, "%.2f %.2f %s\n", pl_map(&frame->scale[PL_X], line->x[i]), pl_map(&frame->scale[PL_Y], line->y[i]), joined ? "l" : "m");
        joined = any = 1;
    }

    /* Tracciare un cammino vuoto in un PDF è un errore. */
    if (any) pl_put(pdf, "S\n");
}

/* Disegna la legenda del pannello: un riquadro bianco nell'angolo in alto a
 * destra della cornice, con una riga per ogni linea che ha un nome: un
 * campione del tratto e il nome. Non disegna nulla se nessuna linea ha un
 * nome. */
static void pl_draw_legend(PlPdf *pdf, const PlAxes *axes, const PlFrame *frame) {
    /* Quante righe, e quanto è lunga la scritta più lunga. */
    int rows = 0;
    double widest = 0;
    for (const PlItem *item = axes->first; item; item = item->next) {
        if (!item->label) continue;
        double length = pl_text_width(item->label, PL_LABEL_SIZE);
        if (length > widest) widest = length;
        rows++;
    }
    if (!rows) return;

    /* Il riquadro: largo quanto campione, spazio e scritta più lunga; alto
     * quanto le righe, senza lo spazio dopo l'ultima. */
    double width = 2 * PL_LEGEND_PAD + PL_LEGEND_SAMPLE + PL_LEGEND_SAMPLE_GAP + widest;
    double height = 2 * PL_LEGEND_PAD + PL_LABEL_SIZE + (rows - 1) * PL_LEGEND_ROW;
    double left = frame->scale[PL_X].start + frame->scale[PL_X].length - PL_LEGEND_INSET - width;
    double top = frame->scale[PL_Y].start + frame->scale[PL_Y].length - PL_LEGEND_INSET;
    pl_put(pdf, "q\n1 g\n%.2f G\n%.2f w\n%.2f %.2f %.2f %.2f re B\nQ\n", PL_LEGEND_EDGE, PL_FRAME_WIDTH, left, top - height, width, height);

    /* Le righe. middle è l'altezza a metà della riga: lì passa il campione,
     * e la scritta ha la linea di base mezza altezza di maiuscola più sotto. */
    double middle = top - PL_LEGEND_PAD - PL_LABEL_SIZE / 2;
    double sample = left + PL_LEGEND_PAD;
    for (const PlItem *item = axes->first; item; item = item->next) {
        if (!item->label) continue;
        pl_put(pdf, "q\n");
        pl_put_stroke(pdf, item->color, item->width, 0);
        pl_put(pdf, "%.2f %.2f m %.2f %.2f l S\nQ\n", sample, middle, sample + PL_LEGEND_SAMPLE, middle);
        pl_put_text(pdf, sample + PL_LEGEND_SAMPLE + PL_LEGEND_SAMPLE_GAP, middle - PL_LABEL_SIZE * PL_CAP_HEIGHT / 2, PL_LABEL_SIZE, 0, item->label);
        middle -= PL_LEGEND_ROW;
    }
}

/* Disegna il pannello axes nella cornice frame, le cui scale devono avere
 * già i limiti. Gli strati da 7a a 7d sono descritti in testa al capitolo. */
static void pl_draw_axes(PlPdf *pdf, const PlAxes *axes, const PlFrame *frame) {
    const PlScale *across = &frame->scale[PL_X], *up = &frame->scale[PL_Y];
    double left = across->start, right = left + across->length;
    double bottom = up->start, top = bottom + up->length;

    /* 7a. Il rettangolo della cornice diventa il ritaglio: fino a "Q" ciò
     * che ne esce non viene disegnato, e una curva che sale verso un
     * asintoto è tagliata netta al bordo. */
    pl_put(pdf, "q\n%.2f %.2f %.2f %.2f re W n\n", left, bottom, across->length, up->length);

    /* Le bande per prime, così stanno sotto a tutto qualunque sia l'ordine
     * delle chiamate. Un estremo NaN, diverso da se stesso, toglie la banda. */
    for (const PlItem *item = axes->first; item; item = item->next) {
        if (item->kind != PL_SPAN || item->a != item->a || item->b != item->b) continue;
        double from = pl_map(across, item->a);
        pl_put_color(pdf, item->color, PL_SPAN_ALPHA, "rg");
        pl_put(pdf, "%.2f %.2f %.2f %.2f re f\n", from, bottom, pl_map(across, item->b) - from, up->length);
    }

    /* Poi le linee, nell'ordine delle chiamate: l'ultima copre le altre. */
    for (const PlItem *item = axes->first; item; item = item->next) {
        if (item->kind == PL_LINE) {
            pl_put_stroke(pdf, item->color, item->width, 0);
            pl_put_polyline(pdf, item, frame);
        } else if (item->kind == PL_HLINE) {
            double y = pl_map(up, item->a);
            pl_put_stroke(pdf, item->color, item->width, item->dashed);
            pl_put(pdf, "%.2f %.2f m %.2f %.2f l S\n", left, y, right, y);
        } else if (item->kind == PL_VLINE) {
            double x = pl_map(across, item->a);
            pl_put_stroke(pdf, item->color, item->width, item->dashed);
            pl_put(pdf, "%.2f %.2f m %.2f %.2f l S\n", x, bottom, x, top);
        }
    }
    pl_put(pdf, "Q\n");

    /* 7b. In nero, col colore iniziale della pagina. Lo spessore dichiarato
     * per la cornice vale anche per le tacche. */
    pl_put(pdf, "%.2f w\n%.2f %.2f %.2f %.2f re S\n", PL_FRAME_WIDTH, left, bottom, across->length, up->length);
    pl_draw_ticks(pdf, axes, frame, PL_X);
    pl_draw_ticks(pdf, axes, frame, PL_Y);
    pl_put_text(pdf, (left + right) / 2, top + PL_TITLE_PAD, PL_TITLE_SIZE, 0.5, axes->title);

    /* 7c. Un cerchio riempito e bordato: l'interno del colore del bordo se
     * il marcatore è pieno, bianco se è vuoto. */
    for (const PlItem *item = axes->first; item; item = item->next) {
        if (item->kind != PL_MARKER) continue;
        pl_put(pdf, "q\n%.2f w\n", PL_MARKER_EDGE);
        pl_put_color(pdf, item->color, 1, "RG");
        pl_put_color(pdf, item->color, item->filled ? 1 : 0, "rg");
        pl_put_circle(pdf, pl_map(across, item->a), pl_map(up, item->b), PL_MARKER_RADIUS);
        pl_put(pdf, "B\nQ\n");
    }

    /* 7d. */
    if (axes->legend) pl_draw_legend(pdf, axes, frame);
}

/* =============================================================================
 * 8. IL FILE: APERTURA, PAGINE, CHIUSURA
 * ============================================================================
 *
 * Le tre funzioni che fanno un file: pl_pdf_open() scrive l'intestazione,
 * pl_pdf_savefig() una pagina con i suoi tre oggetti, pl_pdf_close() gli
 * oggetti comuni, l'indice e la coda. La forma del file è descritta in testa
 * al capitolo 3.
 */

/* Decide se la figura si può disegnare, e con quali misure. Ritorna 1 e
 * mette in *room l'altezza in punti che i pannelli si dividono, tolti i
 * margini e gli spazi fissi, e in *weights la somma dei loro pesi. Ritorna 0
 * se un pannello ha il peso non positivo o un asse senza limiti validi, o se
 * la pagina è più piccola dei suoi margini, o più grande di PL_MAX_PAGE. */
static int pl_measure(const PlFigure *figure, double *room, double *weights) {
    *room = figure->height - PL_MARGIN_TOP - PL_MARGIN_BOTTOM - (figure->title ? PL_SUPTITLE_ROOM : 0);
    *weights = 0;
    for (const PlAxes *axes = figure->first; axes; axes = axes->next) {
        PlScale scale;
        if (!(axes->weight > 0) || !pl_limits(axes, PL_X, &scale) || !pl_limits(axes, PL_Y, &scale)) return 0;
        *weights += axes->weight;
        *room -= (axes->title ? PL_TITLE_ROOM : 0) + (axes->next ? PL_PANEL_GAP : 0);
    }

    /* I confronti sono falsi anche per una misura NaN; quelli con
     * PL_MAX_PAGE escludono l'infinito. */
    return figure->width - PL_MARGIN_LEFT - PL_MARGIN_RIGHT > 0 && figure->width <= PL_MAX_PAGE && *room > 0 &&
           figure->height <= PL_MAX_PAGE && pl_is_finite(*weights);
}

/* Scrive il contenuto della pagina di figure: il fondo, il titolo, e i
 * pannelli dall'alto in basso. room e weights sono quelli di pl_measure(),
 * che deve aver ritornato 1. */
static void pl_draw_figure(PlPdf *pdf, const PlFigure *figure, double room, double weights) {
    double left = PL_MARGIN_LEFT, width = figure->width - PL_MARGIN_LEFT - PL_MARGIN_RIGHT;
    double top = figure->height - PL_MARGIN_TOP;

    /* Il fondo bianco, su cui sono calcolate le tinte delle bande, e le
     * giunzioni tonde, che valgono per tutta la pagina. Le estremità delle
     * linee restano quelle iniziali, tronche: un tratto spesso finisce sul
     * suo ultimo punto, com'è giusto per un intervallo sulla retta dei
     * numeri. */
    pl_put(pdf, "q\n1 g\n0 0 %.2f %.2f re f\nQ\n1 j\n", figure->width, figure->height);

    /* Il titolo della figura, centrato sui pannelli come i loro titoli. */
    if (figure->title) {
        pl_put_text(pdf, left + width / 2, top - PL_TITLE_SIZE * PL_CAP_HEIGHT, PL_TITLE_SIZE, 0.5, figure->title);
        top -= PL_SUPTITLE_ROOM;
    }

    /* I pannelli. top è l'altezza a cui comincia lo spazio del prossimo:
     * prima il suo titolo, se c'è, poi la cornice, alta la sua parte di
     * room. Che i limiti siano validi l'ha già stabilito pl_measure(): qui
     * di pl_limits() serve solo ciò che scrive nella scala. */
    for (const PlAxes *axes = figure->first; axes; axes = axes->next) {
        PlFrame frame;
        if (axes->title) top -= PL_TITLE_ROOM;
        frame.scale[PL_X].start = left;
        frame.scale[PL_X].length = width;
        frame.scale[PL_Y].length = room * axes->weight / weights;
        frame.scale[PL_Y].start = top - frame.scale[PL_Y].length;
        pl_limits(axes, PL_X, &frame.scale[PL_X]);
        pl_limits(axes, PL_Y, &frame.scale[PL_Y]);
        pl_draw_axes(pdf, axes, &frame);
        top = frame.scale[PL_Y].start - PL_PANEL_GAP;
    }
}

/* Comincia un file PDF in path, sostituendo quello che c'era. Ritorna NULL
 * se il file non si può creare o se manca la memoria. Il file va chiuso con
 * pl_pdf_close(), che è anche ciò che lo rende un PDF completo. */
PlPdf *pl_pdf_open(const char *path) {
    /* Il file si apre per ultimo: se manca la memoria non resta sul disco
     * un file vuoto. In modalità binaria, perché un a capo deve essere un
     * byte solo su ogni sistema, o le posizioni dell'indice sarebbero
     * sbagliate. */
    PlPdf *pdf = malloc(sizeof *pdf);
    long *start = malloc(PL_FIXED_OBJECTS * sizeof *start);
    FILE *file = pdf && start && path ? fopen(path, "wb") : NULL;
    if (!file) {
        free(start);
        free(pdf);
        return NULL;
    }
    *pdf = (PlPdf){.file = file, .start = start};
    pl_put(pdf, "%%PDF-1.4\n");
    return pdf;
}

/* Aggiunge al file una pagina con la figura, grande quanto la figura.
 * Ritorna 1 se la pagina è stata scritta, 0 altrimenti: se la figura è
 * fallita (una chiamata che la descriveva non ha potuto registrare ciò che
 * le era chiesto) o non si può disegnare (vedi pl_measure()), se manca la
 * memoria, o se una scrittura nel file è fallita, ora o prima. Nei primi tre
 * casi nel file non viene scritto nulla, e resta buono per altre pagine. La
 * figura non viene modificata: la si può salvare ancora, o cambiare e
 * salvare di nuovo. */
int pl_pdf_savefig(PlPdf *pdf, const PlFigure *figure) {
    double room, weights;
    if (!pdf || pdf->failed || !figure || figure->failed || !pl_measure(figure, &room, &weights)) return 0;

    /* Posto per le posizioni dei tre oggetti della pagina. */
    int page = PL_FIXED_OBJECTS + PL_OBJECTS_PER_PAGE * pdf->pages + 1, content = page + 1, length = page + 2;
    long *start = realloc(pdf->start, (size_t)length * sizeof *start);
    if (!start) return 0;
    pdf->start = start;

    /* La pagina: le sue misure, il carattere che usa sotto il nome F1, e
     * dove sta il contenuto. */
    pl_begin_object(pdf, page);
    pl_put(pdf, "<< /Type /Page /Parent %d 0 R /MediaBox [0 0 %.2f %.2f] /Resources << /Font << /F1 %d 0 R >> >> /Contents %d 0 R >>\nendobj\n",
           PL_PAGES, figure->width, figure->height, PL_FONT, content);

    /* Il contenuto. La lunghezza è la differenza tra i byte contati prima e
     * dopo; l'a capo che precede "endstream" non ne fa parte. */
    pl_begin_object(pdf, content);
    pl_put(pdf, "<< /Length %d 0 R >>\nstream\n", length);
    long begin = pdf->offset;
    pl_draw_figure(pdf, figure, room, weights);
    long bytes = pdf->offset - begin;
    pl_put(pdf, "\nendstream\nendobj\n");

    /* La lunghezza, ora che la si conosce. */
    pl_begin_object(pdf, length);
    pl_put(pdf, "%ld\nendobj\n", bytes);

    pdf->pages++;
    return !pdf->failed;
}

/* Completa il file con gli oggetti comuni, l'indice e la coda, lo chiude e
 * libera pdf, che non si può più usare. Ritorna 1 se il file è un PDF
 * completo con almeno una pagina, 0 altrimenti.
 *
 * Un file senza pagine viene scritto lo stesso, ben formato, ma la funzione
 * ritorna 0: un elenco di pagine vuoto è ammesso dalla grammatica, però
 * molti lettori lo rifiutano, e non è un documento da consegnare. Quando la
 * funzione ritorna 0 il file resta sul disco: toglierlo spetta a chi chiama. */
int pl_pdf_close(PlPdf *pdf) {
    if (!pdf) return 0;

    /* Gli oggetti comuni, ora che si sa quante pagine ci sono. */
    pl_begin_object(pdf, PL_CATALOG);
    pl_put(pdf, "<< /Type /Catalog /Pages %d 0 R >>\nendobj\n", PL_PAGES);
    pl_begin_object(pdf, PL_PAGES);
    pl_put(pdf, "<< /Type /Pages /Kids [");
    for (int i = 0; i < pdf->pages; i++) pl_put(pdf, "%s%d 0 R", i ? " " : "", PL_FIXED_OBJECTS + PL_OBJECTS_PER_PAGE * i + 1);
    pl_put(pdf, "] /Count %d >>\nendobj\n", pdf->pages);

    /* Il carattere. WinAnsiEncoding fa corrispondere i codici da 32 a 126
     * ai caratteri ASCII, apostrofo e accento grave compresi (senza,
     * Helvetica al loro posto ha due virgolette curve). /Differences cambia
     * una sola voce: al codice 45, quello di '-', mette il segno meno.
     * Warning: la larghezza di '-' in pl_glyph_width è quella del meno. */
    pl_begin_object(pdf, PL_FONT);
    pl_put(pdf, "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding "
                "<< /Type /Encoding /BaseEncoding /WinAnsiEncoding /Differences [45 /minus] >> >>\nendobj\n");

    /* L'indice: la riga dello 0, poi una riga di 20 byte per oggetto, in
     * ordine di numero. */
    int objects = PL_FIXED_OBJECTS + PL_OBJECTS_PER_PAGE * pdf->pages;
    long xref = pdf->offset;
    pl_put(pdf, "xref\n0 %d\n0000000000 65535 f \n", objects + 1);
    for (int i = 0; i < objects; i++) pl_put(pdf, "%010ld 00000 n \n", pdf->start[i]);
    pl_put(pdf, "trailer\n<< /Size %d /Root %d 0 R >>\nstartxref\n%ld\n%%%%EOF\n", objects + 1, PL_CATALOG, xref);

    /* fclose() va chiamata comunque, ed è lei a scrivere davvero gli ultimi
     * byte: un disco pieno può farsi vivo solo qui. */
    int complete = !pdf->failed && !ferror(pdf->file) && pdf->pages > 0;
    if (fclose(pdf->file) != 0) complete = 0;
    free(pdf->start);
    free(pdf);
    return complete;
}
