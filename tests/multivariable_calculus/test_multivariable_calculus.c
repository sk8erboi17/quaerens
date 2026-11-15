/* Test di dominio: le funzioni di più variabili (capitolo 34 di parser.c,
 * capitolo 35 di main.c, capitolo 18 di checker.c): derivate parziali,
 * gradiente e hessiana, come formule o in un punto, le uguaglianze che le
 * derivate soddisfano, e le richieste senza il segno di uguale.
 *
 * I controlli; quelli su un file intero sono descritti in
 * tests/statement_checks.h:
 *
 *   1. ogni enunciato di partial_derivatives.tex viene dimostrato, con la
 *      dimostrazione scritta;
 *   2. quasi tutti portano un certificato che il verificatore ha accettato:
 *      quelli che non lo portano sono contati, e non devono aumentare;
 *   3. ogni certificato, alterato in una formula (la derivata più 1) o in
 *      una tesi (un membro più 1), non è più accettato;
 *   4. nessun enunciato di false_partial_derivatives.tex viene dimostrato, e
 *      tutti vengono dichiarati falsi: sono enunciati veri con un
 *      coefficiente, un segno, la variabile di derivazione, un elemento di
 *      una matrice o una coordinata cambiati;
 *   5. antiregressione: da ogni enunciato vero che finisce con
 *      un'espressione si ricava quello con "+ 1" in fondo, che è falso e
 *      deve essere dichiarato tale;
 *   6. nessun enunciato di partial_derivatives_beyond.tex viene dichiarato
 *      falso: sono veri, ma chiedono ciò che il motore non sa ancora fare;
 *   7. le richieste: ogni enunciato vero con una sola tesi, tagliato al
 *      segno di uguale, è una richiesta, e la risposta scritta dal motore
 *      deve avere, componente per componente, lo stesso valore del risultato
 *      stampato. Il confronto si fa con gli intervalli del capitolo 24 in
 *      punti che non sono quelli del motore, senza passare da derive() né
 *      dalle serie di Taylor;
 *   8. ogni richiesta di partial_requests.tex riceve la risposta, con il
 *      certificato, e alcune hanno proprio il testo atteso;
 *   9. alcuni enunciati ricevono proprio il verdetto atteso, con il motivo:
 *      un punto in cui la funzione non è definita, le scritture che il
 *      lettore non accetta;
 *  10. il lettore rispetta le convenzioni del capitolo 34: l'ordine delle
 *      derivate miste nella frazione e nel pedice, le componenti di un
 *      gradiente e di un'hessiana;
 *  11. il controllo con le serie di Taylor lungo le rette (35b), da solo,
 *      senza derive(): una derivata mista scritta a mano deve essere
 *      confermata, e una sbagliata smentita;
 *  12. nessun passo di una dimostrazione scritta è stato scartato dai
 *      controlli indipendenti.
 *
 * Gli enunciati sono dati: il motore non ne conosce nessuno in anticipo. Che
 * siano veri e falsi l'ha controllato anche l'oracolo indipendente di
 * tools/fuzz/fuzz_multivariable.py (modalità check).
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_multivariable_calculus \
 *         tests/multivariable_calculus/test_multivariable_calculus.c && /tmp/test_multivariable_calculus
 */

#include "../statement_checks.h"

#define TRUE_FILE "tests/multivariable_calculus/partial_derivatives.tex"
#define BOOK_FILE "tests/multivariable_calculus/book_partial_derivatives.tex"
#define FALSE_FILE "tests/multivariable_calculus/false_partial_derivatives.tex"
#define FALSE_BOOK_FILE "tests/multivariable_calculus/false_book_partial_derivatives.tex"
#define BEYOND_FILE "tests/multivariable_calculus/partial_derivatives_beyond.tex"
#define REQUEST_FILE "tests/multivariable_calculus/partial_requests.tex"

/* Quanti enunciati restano senza certificato. In TRUE_FILE: x^y, la cui
 * derivata rispetto a x chiede al verificatore una relazione tra potenze
 * che non conosce. In BOOK_FILE: le derivate seconde di x^y, per lo stesso
 * motivo, e un gradiente in un punto con la radice di 3 tra le coordinate,
 * dove il verificatore non sa stabilire il segno di una condizione. */
#define UNCERTIFIED 1
#define BOOK_UNCERTIFIED 2

/* Ritorna il nome di un esito, per i messaggi. */
static const char *outcome_name(Outcome outcome) {
    switch (outcome) {
    case PROVED: return "PROVED";
    case REFUTED: return "REFUTED";
    case NOT_PROVED: return "NOT_PROVED";
    case UNSUPPORTED: return "UNSUPPORTED";
    case SOLVED: return "SOLVED";
    }
    return "?";
}

/* Ritorna l'enunciato del prossimo \item di *position, come lo vede il
 * motore, o NULL quando gli \item sono finiti o uno non si legge. */
static const char *next_formula(const char **position) {
    static Item statement;
    const char *item, *end;
    if (!next_item(position, &item, &end) || read_item(item, end, &statement)) return NULL;
    return statement.formula;
}

/* Controlli 2 e 3 su path, un file di enunciati veri: i verdetti DIMOSTRATO
 * senza certificato sono al più uncertified, e ogni certificato, alterato,
 * non è più accettato. Le alterazioni: la prima formula "is" più 1, e il
 * primo membro dell'ultima tesi più 1. */
static int check_certificates(const char *path, int uncertified) {
    static char altered[2][CERTIFICATE_BYTES];
    char summary[512];
    char *text = read_file(path);
    const char *position = text, *formula, *why;
    int total = 0, certified = 0, accepted = 0, alterations = 0;
    while (text && (formula = next_formula(&position)) != NULL) {
        Verdict verdict = prove(formula, 'n', 0, 0);
        total++;
        if (verdict.outcome != PROVED || !verdict.certificate) continue;
        certified++;
        const char *certificate = verdict.certificate, *is = strstr(certificate, "\nis "), *statement = NULL;
        for (const char *next = strstr(certificate, "\nstatement (= "); next; next = strstr(next + 1, "\nstatement (= ")) statement = next;
        if (!is || !statement) { printf("  statement %d: the certificate has no formula or no thesis\n", total); accepted++; continue; }

        /* La formula: "is G" diventa "is (+ 1 G)". */
        const char *line = strchr(is + 1, '\n');
        snprintf(altered[0], sizeof altered[0], "%.*s\nis (+ 1 %.*s)%s", (int)(is - certificate), certificate, (int)(line - is - 4), is + 4, line);

        /* La tesi: "(= L R)" diventa "(= (+ 1 L) R)". L finisce dove le sue
         * parentesi si chiudono, o al primo spazio se è una parola sola. */
        const char *left = statement + strlen("\nstatement (= "), *stop = left;
        for (int depth = 0; *stop && (depth > 0 || (*stop != ' ' && *stop != ')')); stop++) depth += *stop == '(' ? 1 : *stop == ')' ? -1 : 0;
        snprintf(altered[1], sizeof altered[1], "%.*s(+ 1 %.*s)%s", (int)(left - certificate), certificate, (int)(stop - left), left, stop);
        for (int i = 0; i < 2; i++) {
            const char *cursor_text = altered[i];
            alterations++;
            if (ck_check(&cursor_text, &why, 0) != CK_ACCEPTED) continue;
            accepted++;
            printf("  statement %d: the altered certificate %d is accepted:\n%s\n", total, i + 1, altered[i]);
        }
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d statements of %s carry an accepted certificate (at most %d may lack it)", certified, total, path, uncertified);
    int passed = check_result(total > 0 && total - certified <= uncertified, summary);
    snprintf(summary, sizeof summary, "%d of %d altered certificates of %s are not accepted", alterations - accepted, alterations, path);
    return check_result(alterations > 0 && accepted == 0, summary) && passed;
}

/* Conta gli enunciati di path per esito: nessuno deve ricevere forbidden, e
 * tutti devono ricevere wanted, con un certificato accettato se certified
 * è 1. Ritorna 1 se il controllo è riuscito. */
static int check_outcomes(const char *path, Outcome forbidden, Outcome wanted, int certified) {
    char summary[512];
    char *text = read_file(path);
    const char *position = text, *item, *end;
    int total = 0, bad = 0, good = 0;
    while (text && next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        total++;
        if (verdict.outcome == wanted && (!certified || verdict.certificate)) { good++; continue; }
        if (verdict.outcome == forbidden) bad++;
        printf("  statement %d of %s is %s: ", total, path, outcome_name(verdict.outcome));
        verdict_print(stdout, &verdict);
        putchar('\n');
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d statements of %s are %s%s, %d are %s (none allowed)", good, total, path, outcome_name(wanted),
             certified ? " with a certificate" : "", bad, outcome_name(forbidden));
    return check_result(total > 0 && bad == 0 && good == total, summary);
}

/* Controllo 5: da ogni enunciato di path si ricava quello con " + 1" in
 * fondo. Se il lettore lo legge, il suo ultimo secondo membro è cresciuto di
 * 1, e l'enunciato, che era vero, è falso: deve essere dichiarato tale. Un
 * enunciato che finisce con una lista o una matrice non si legge più, e non
 * conta. at_least è quante varianti devono essere state lette. */
static int check_plus_one(const char *path, int at_least) {
    static char variant[2 * MAX_FORMULA];
    char summary[512];
    char *text = read_file(path);
    const char *position = text, *formula;
    int total = 0, read = 0, refuted = 0;
    while (text && (formula = next_formula(&position)) != NULL) {
        total++;
        snprintf(variant, sizeof variant, "%s + 1", formula);
        Verdict verdict = prove(variant, 'n', 0, 0);
        if (verdict.outcome == UNSUPPORTED) continue;
        read++;
        if (verdict.outcome == REFUTED) { refuted++; continue; }
        printf("  statement %d with \"+ 1\" appended is %s, not REFUTED\n", total, outcome_name(verdict.outcome));
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d false variants \"+ 1\" of %s are called false (at least %d wanted)", refuted, read, path, at_least);
    return check_result(read >= at_least && refuted == read, summary);
}

/* I punti in cui due risposte vengono confrontate: dodici, diversi da quelli
 * del motore (partial_points[]). Nei primi sei la lettera letter prende un
 * valore piccolo e positivo, diverso da lettera a lettera, così anche
 * ln(1 - x - y) è definita in qualcuno; negli altri un valore tra 1/2 e 5,
 * negli ultimi tre a volte negativo. */
#define COMPARISON_POINTS 12
static Node *comparison_value(int letter, int p) {
    long long top = 1 + (3 * letter + p) % 5, bottom = p < 6 ? 7 + p + letter % 3 : 1 + (letter + p) % 2;
    return node_new(N_DIV, node_num(p >= 9 && (letter + p) % 3 == 0 ? -top : top), node_num(bottom));
}

/* Confronta gli alberi a e b, senza operandi, nei punti di confronto:
 * ritorna -1 se in uno hanno valori di sicuro diversi, 1 se in almeno uno
 * hanno lo stesso valore, 0 se in nessuno si calcolano tutti e due. I valori
 * vengono dagli intervalli (sample_value()), con il confronto del motore
 * (numbers_agree()). */
static int same_values(const Node *a, const Node *b) {
    int agreed = 0;
    for (int p = 0; p < COMPARISON_POINTS; p++) {
        Node *tree[2] = {(Node *)a, (Node *)b};
        for (int i = 0; i < 2; i++) {
            for (int letter = 'A'; letter <= 'z'; letter++)
                if (isalpha(letter) && letter != 'e' && node_has_variable(tree[i], letter)) tree[i] = node_replace(tree[i], letter, comparison_value(letter, p));
            tree[i] = node_fold(tree[i], &(int){1});
        }
        int agreement = numbers_agree(sample_value(tree[0], VAR_LINE, range_none()), sample_value(tree[1], VAR_LINE, range_none()));
        if (agreement < 0) return -1;
        agreed += agreement > 0;
    }
    return agreed > 0;
}

/* Controllo 7 su path: vedi la testa del file. at_least è quante richieste
 * devono avere avuto la risposta. Il segno di uguale della tesi è il primo
 * dopo \implies (o il primo, senza definizioni): nei dati il primo membro
 * non ne contiene. Si saltano gli enunciati con più tesi, e quelli il cui
 * secondo membro contiene una derivata. */
static int check_requests(const char *path, int at_least) {
    static char request[MAX_FORMULA], completed[4 * MAX_FORMULA];
    static PartialStatement stated, answered;
    char summary[512];
    char *text = read_file(path);
    const char *position = text, *formula;
    int total = 0, asked = 0, solved = 0, wrong = 0, unknown = 0;
    while (text && (formula = next_formula(&position)) != NULL) {
        total++;
        const char *thesis = strstr(formula, "\\implies"), *equal = strchr(thesis ? thesis : formula, '=');
        if (!equal || strstr(equal, "\\land")) continue;
        snprintf(request, sizeof request, "%.*s", (int)(equal - formula), formula);
        asked++;
        Verdict verdict = prove(request, 'n', 0, 0);
        if (verdict.outcome != SOLVED) continue;
        solved++;
        snprintf(completed, sizeof completed, "%s = %s", request, verdict.detail);

        /* I due enunciati, quello stampato e quello con la risposta, letti
         * dal lettore del programma: stesse tesi, da confrontare una a una.
         * Un secondo membro stampato con dentro una derivata non si calcola
         * qui: quella richiesta non conta. */
        arena_used = arena_keep;
        failure = NULL;
        int readable = parse_partial_statement(formula, &stated) && parse_partial_statement(completed, &answered) && stated.claims == answered.claims;
        int same = readable ? 1 : -1, skipped = 0;
        for (int c = 0; same > 0 && c < stated.claims; c++) {
            skipped = partial_pending(stated.claim[c].rhs);
            if (skipped) break;
            same = same_values(stated.claim[c].rhs, answered.claim[c].rhs);
        }
        syntax_error = failure = NULL;
        if (skipped) { asked--; solved--; continue; }
        if (same > 0) continue;
        if (same < 0) wrong++; else unknown++;
        printf("  statement %d: the answer %s %s\n", total, verdict.detail,
               same < 0 ? "does not have the value of the printed result" : "cannot be compared with the printed result at any comparison point");
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d requests made from %s are answered (at least %d wanted), %d answers differ from the printed result, %d "
             "could not be compared", solved, asked, path, at_least, wrong, unknown);
    return check_result(solved >= at_least && wrong == 0 && unknown == 0, summary);
}

/* Una richiesta e il testo atteso della risposta. */
typedef struct {
    const char *request, *answer;
} Written;

static const Written written[] = {
    {"f(x, y) = x^{2} y \\implies \\nabla f", "\\left( 2x \\cdot y, x^{2} \\right)"},
    {"f(x, y) = x^{2} y \\implies H_{f}", "\\begin{pmatrix} 2y & 2x \\\\ 2x & 0 \\end{pmatrix}"},
    {"f(x, y) = x^{2} y \\implies f_{xy}", "2x"},
    {"f(x, y) = x^{2} y \\implies \\nabla f(1, 2)", "\\left( 4, 1 \\right)"},
    {"f(x, y) = x^{3} + y^{3} - 3xy \\implies H_{f}(1, 1)", "\\begin{pmatrix} 6 & -3 \\\\ -3 & 6 \\end{pmatrix}"},
    {"\\frac{\\partial}{\\partial y}\\left( x^{2} y \\right)", "x^{2}"},
    {"f(x, y, z) = xyz \\implies \\frac{\\partial^{3} f}{\\partial x \\partial y \\partial z}", "1"},
};

/* Un enunciato con il verdetto atteso e, se non è dimostrato, un pezzo del
 * motivo. */
typedef struct {
    const char *formula;
    Outcome outcome;
    const char *reason;  /* un pezzo del motivo atteso, o NULL */
} Expected;

static const Expected expected[] = {
    /* Un punto in cui la funzione non è definita, o non è regolare: la
     * formula della derivata lì ha un valore, ma la derivata non esiste. */
    {"f(x, y) = \\ln(xy) \\implies f_{x}(-1, 1) = -1", NOT_PROVED, "nel punto scritto"},
    {"f(x, y) = x \\ln y \\implies f_{y}(2, 0) = 1", NOT_PROVED, "nel punto scritto"},
    {"f(x, y) = \\frac{1}{x - y} \\implies f_{x}(1, 1) = 0", NOT_PROVED, "nel punto scritto"},
    {"f(x, y) = \\sqrt{x^{2} + y^{2}} \\implies \\nabla f(0, 0) = (0, 0)", NOT_PROVED, "nel punto scritto"},
    {"f(x, y) = \\ln(xy) \\implies \\nabla f(-1, 1)", NOT_PROVED, "nel punto scritto"},

    /* Lo stesso enunciato, vero in un punto in cui la funzione è definita. */
    {"f(x, y) = \\ln(xy) \\implies f_{x}(-1, -1) = -1", PROVED, NULL},

    /* Una funzione definita in nessuno dei punti di prova del capitolo 27
     * (sample_tree() fissa y = 5/3), e qui sì. */
    {"f(x, y) = \\ln(1 - x - y) \\implies f_{y} = \\frac{1}{x + y - 1}", PROVED, NULL},

    /* Un parametro non è una variabile: la derivata rispetto a lui è quella
     * di un'altra lettera, e rispetto a una lettera che non c'è è zero. */
    {"f(x, y) = a x^{2} y \\implies \\frac{\\partial f}{\\partial a} = x^{2} y", PROVED, NULL},
    {"f(x, y) = x^{2} y \\implies \\frac{\\partial f}{\\partial z} = 0", PROVED, NULL},
    {"f(x, y) = a x^{2} y \\implies \\frac{\\partial f}{\\partial a} = 2xy", REFUTED, NULL},

    /* Antiregressione: una tesi si guarda dove sono regolari le sue
     * funzioni, non dove lo sono anche quelle di un'altra tesi. La seconda
     * tesi è falsa per x < 0, dove \ln x non c'è: guardando le due tesi nei
     * soli punti comuni, e con i fatti del dominio di tutte e due, il motore
     * l'aveva dimostrata (senza certificato). Con le due funzioni nella
     * stessa tesi, invece, il dominio è quello comune, e l'enunciato è vero. */
    {"f(x, y) = \\ln x, g(x, y) = \\sqrt{x^{2}} y \\implies f_{x} = \\frac{1}{x} \\land g_{x} = y", REFUTED, NULL},
    {"f(x, y) = \\ln x, g(x, y) = |x| y \\implies f_{x} = \\frac{1}{x} \\land g_{x} = y", REFUTED, NULL},
    {"g(x, y) = \\sqrt{x^{2}} y \\implies g_{x} = y", REFUTED, NULL},
    {"f(x, y) = \\ln x, g(x, y) = \\sqrt{x^{2}} y \\implies f_{x} g_{x} = \\frac{y}{x}", PROVED, NULL},

    /* I numeri da soli non dimostrano: questo secondo membro differisce
     * da quello giusto meno di quanto gli intervalli sappiano vedere, in
     * ogni punto di prova, e lo ferma solo l'algebra esatta. */
    {"f(x, y) = x^{2} y \\implies f_{x} = 2xy + \\frac{1}{10^{30}}", NOT_PROVED, "non si riduce a zero"},

    /* Come frazione di lettere, \partial f / \partial x sarebbe f / x. */
    {"f(x, y) = x^{2} \\implies \\frac{\\partial f}{\\partial x} = x", REFUTED, NULL},
    {"\\frac{\\partial}{\\partial x}\\left( x^{2} \\right) = x", REFUTED, NULL},

    /* Antiregressione: un fattore accanto alla funzione del segno senza
     * funzione sopra. Letto dentro la derivata, come fa \frac{d}{dx} (9d),
     * il primo enunciato risultava DIMOSTRATO, ed è falso per chi legge il
     * prodotto della derivata per x + 1, che vale 2x^2 + 2x. La scrittura
     * ambigua non si legge; quella con il fattore davanti sì. */
    {"\\frac{\\partial}{\\partial x}\\left( x^{2} \\right) \\cdot \\left( x + 1 \\right) = 3x^{2} + 2x", UNSUPPORTED, "un solo fattore"},
    {"\\frac{\\partial}{\\partial x}\\left( x^{2} \\right) \\left( x + 1 \\right) = 3x^{2} + 2x", UNSUPPORTED, "un solo fattore"},
    {"\\frac{\\partial}{\\partial x} x^{2} y = 2xy", UNSUPPORTED, "un solo fattore"},
    {"\\left( x + 1 \\right) \\frac{\\partial}{\\partial x}\\left( x^{2} \\right) = 2x^{2} + 2x", PROVED, NULL},
    {"\\left( x + 1 \\right) \\frac{\\partial}{\\partial x}\\left( x^{2} \\right) = 3x^{2} + 2x", REFUTED, NULL},
    {"f(x, y) = x^{2} \\implies \\frac{\\partial f}{\\partial x} \\cdot \\left( x + 1 \\right) = 2x^{2} + 2x", PROVED, NULL},
    {"\\frac{\\partial}{\\partial x}\\left( x + y \\right)^{2} = 2x + 2y", PROVED, NULL},

    /* Le scritture che il lettore non accetta, con il loro motivo. */
    {"\\frac{\\partial f}{\\partial x} = 2x", UNSUPPORTED, "non è dichiarata"},
    {"f(x, y) = x^{2} y \\implies \\frac{\\partial^{2} f}{\\partial x} = 2y", UNSUPPORTED, "stesso ordine sopra e sotto"},
    {"f(x, y) = x^{2} y \\implies \\frac{\\partial^{5} f}{\\partial x^{5}} = 0", UNSUPPORTED, "al più 4"},
    {"f(x, y) = x^{2} y \\implies f_{x}(1) = 2", UNSUPPORTED, "un valore per ogni sua variabile"},
    {"f(x, y) = x^{2} y \\implies f_{x}(y, x) = 2xy", UNSUPPORTED, "non possono contenere le variabili"},
    {"f(x, y) = x^{2} y \\implies \\nabla f = (2xy, x^{2}, 0)", UNSUPPORTED, "una componente per variabile"},
    {"f(x, y) = x^{2} y \\implies \\nabla f = 2xy", UNSUPPORTED, "una componente per variabile"},
    {"f(x, y) = x^{2} y \\implies \\nabla f + 1 = (2xy, x^{2})", UNSUPPORTED, "testo inatteso"},
    {"f(x, y) = x^{2} y \\implies 2 \\nabla f = (4xy, 2x^{2})", UNSUPPORTED, "testo inatteso"},
    {"f(x, y) = x^{2} y \\implies 1 + \\nabla f = (2xy, x^{2})", UNSUPPORTED, "da soli al primo membro"},
    {"f(x, y) = x^{2} y \\implies H_{f} = (2y, 2x, 2x, 0)", UNSUPPORTED, "una matrice"},
    {"f(x, y) = x^{2} y \\implies H_{f} = \\begin{pmatrix} 2y & 2x \\end{pmatrix}", UNSUPPORTED, "una matrice"},
    {"f(x, e) = x^{2} e \\implies f_{x} = 2xe", UNSUPPORTED, "numero di Nepero"},
    {"f(x, x) = x^{2} \\implies f_{x} = 2x", UNSUPPORTED, "lettere diverse"},
    {"f(x, y) = x^{2} y, f(x, y) = x \\implies f_{x} = 1", UNSUPPORTED, "nome già usato"},
    {"f(x, y) = x^{2} y \\implies f_{x} \\land f_{y}", UNSUPPORTED, "deve essere la sola tesi"},
    {"f(x, y) = x^{2} y \\implies f_{x} = 2xy \\land f_{y}", UNSUPPORTED, "deve essere la sola tesi"},
    {"f(x, y) = x^{2} y \\implies f_{x} \\ge 2xy", UNSUPPORTED, "testo inatteso"},
    {"f(x, y) = x^{2} y", UNSUPPORTED, NULL},
    {"f(x, y) = x! y \\implies f_{x} = y", UNSUPPORTED, "regole di derivazione"},
};

/* Controllo 10: una scrittura e come il lettore deve averla letta: quante
 * tesi, e per il primo operando della prima funzione le lettere rispetto a
 * cui si deriva, nell'ordine in cui si deriva. */
typedef struct {
    const char *formula;
    int claims;
    const char *by;     /* le lettere dell'operando numero operand, nell'ordine */
    int operand;
} Reading;

static const Reading readings[] = {
    {"f(x, y) = xy \\implies \\frac{\\partial^{2} f}{\\partial x \\partial y} = 1", 1, "yx", 0},
    {"f(x, y) = xy \\implies f_{xy} = 1", 1, "xy", 0},
    {"f(x, y) = xy \\implies \\frac{\\partial^{3} f}{\\partial x \\partial y^{2}} = 0", 1, "yyx", 0},
    {"f(x, y) = xy \\implies f_x = y", 1, "x", 0},
    {"f(x, y) = xy \\implies \\nabla f = (y, x)", 2, "y", 1},
    {"f(x, y) = xy \\implies H_{f} = \\begin{pmatrix} 0 & 1 \\\\ 1 & 0 \\end{pmatrix}", 4, "xy", 1},
    {"f(x, y) = xy \\implies H_f = \\begin{pmatrix} 0 & 1 \\\\ 1 & 0 \\end{pmatrix}", 4, "yx", 2},
    {"f(x, y, z) = xyz \\implies \\nabla f(1, 2, 3) = 0", 3, "z", 2},
    {"\\dfrac{\\partial^{2}}{\\partial x \\partial y}\\left( xy \\right) = 1", 1, "yx", 0},
};

/* Controllo 11: una funzione, le lettere rispetto a cui si deriva, e la
 * derivata scritta a mano: giusta, e sbagliata. */
typedef struct {
    const char *function, *by, *right, *wrong;
} Mixed;

static const Mixed mixed[] = {
    {"x^{3} y^{2} + \\sin(xy)", "xy", "6x^{2} y + \\cos(xy) - xy \\sin(xy)", "6x^{2} y + \\cos(xy) + xy \\sin(xy)"},
    {"\\ln(x^{2} + y^{2})", "xy", "-\\frac{4xy}{(x^{2} + y^{2})^{2}}", "-\\frac{2xy}{(x^{2} + y^{2})^{2}}"},
    {"e^{xy}", "xyy", "2x e^{xy} + x^{2} y e^{xy}", "x e^{xy} + x^{2} y e^{xy}"},
    {"\\frac{x}{x + y}", "xxy", "\\frac{4y - 2x}{(x + y)^{4}}", "\\frac{2x - 4y}{(x + y)^{4}}"},
    {"x y z + x^{2} z^{2}", "xzx", "4z", "4x"},
    {"\\sqrt{x^{2} + y^{2}}", "yx", "-\\frac{xy}{(x^{2} + y^{2}) \\sqrt{x^{2} + y^{2}}}", "\\frac{xy}{(x^{2} + y^{2}) \\sqrt{x^{2} + y^{2}}}"},
};

/* Ritorna 1 se la derivata di function rispetto alle lettere di by, letta
 * dalle serie di Taylor lungo le rette (partial_taylor()), coincide con
 * formula in almeno un punto di prova e non ne è di sicuro diversa in
 * nessuno; -1 se in un punto ne è di sicuro diversa; 0 se non si sa. Non
 * chiama derive(). */
static int taylor_agrees(const char *function, const char *by, const char *formula) {
    static PartialStatement st;
    static char text[MAX_FORMULA];
    arena_used = arena_keep;
    failure = NULL;
    snprintf(text, sizeof text, "f(x, y, z) = %s \\implies f_{%s} = %s", function, by, formula);
    if (!parse_partial_statement(text, &st) || failure) { syntax_error = failure = NULL; return 0; }
    partial_letters(&st);
    int agreed = 0;
    for (int p = 0; p < PARTIAL_POINTS; p++) {
        int agreement = numbers_agree(partial_taylor(&st.operand[0], &st.function[0], st.function[0].body, NULL, p), partial_range(st.claim[0].rhs, p));
        if (agreement < 0) return -1;
        agreed += agreement > 0;
    }
    return agreed > 0;
}

int main(void) {
    char summary[512];
    int passed = 1;

    /* Controlli da 1 a 8. */
    passed &= check_all_proved(TRUE_FILE);
    passed &= check_all_proved(BOOK_FILE);
    passed &= check_certificates(TRUE_FILE, UNCERTIFIED);
    passed &= check_certificates(BOOK_FILE, BOOK_UNCERTIFIED);
    passed &= check_outcomes(FALSE_FILE, PROVED, REFUTED, 0);
    passed &= check_outcomes(FALSE_BOOK_FILE, PROVED, REFUTED, 0);
    passed &= check_plus_one(TRUE_FILE, 30);
    passed &= check_plus_one(BOOK_FILE, 20);
    passed &= check_none_refuted(BEYOND_FILE);
    passed &= check_requests(TRUE_FILE, 12);
    passed &= check_requests(BOOK_FILE, 12);
    passed &= check_outcomes(REQUEST_FILE, PROVED, SOLVED, 1);
    int exact = 0, total = (int)(sizeof written / sizeof *written);
    for (int i = 0; i < total; i++) {
        Verdict verdict = prove(written[i].request, 'n', 0, 0);
        if (verdict.outcome == SOLVED && verdict.certificate && strcmp(verdict.detail, written[i].answer) == 0) { exact++; continue; }
        printf("  %s: expected %s, got ", written[i].request, written[i].answer);
        verdict_print(stdout, &verdict);
        putchar('\n');
    }
    snprintf(summary, sizeof summary, "%d of %d requests answered as expected, with a certificate", exact, total);
    passed &= check_result(exact == total, summary);

    /* Controllo 9. */
    int right = 0;
    total = (int)(sizeof expected / sizeof *expected);
    for (int i = 0; i < total; i++) {
        Verdict verdict = prove(expected[i].formula, 'n', 0, 0);
        int reason = !expected[i].reason || (verdict.reason && strstr(verdict.reason, expected[i].reason));
        if (verdict.outcome == expected[i].outcome && reason) { right++; continue; }
        printf("  %s: expected %s%s%s, got ", expected[i].formula, outcome_name(expected[i].outcome), expected[i].reason ? " mentioning " : "",
               expected[i].reason ? expected[i].reason : "");
        verdict_print(stdout, &verdict);
        putchar('\n');
    }
    snprintf(summary, sizeof summary, "%d of %d statements get the expected verdict and reason", right, total);
    passed &= check_result(right == total, summary);

    /* Controllo 10. */
    static PartialStatement st;
    right = 0;
    total = (int)(sizeof readings / sizeof *readings);
    for (int i = 0; i < total; i++) {
        arena_used = arena_keep;
        failure = NULL;
        int read = parse_partial_statement(readings[i].formula, &st) && st.claims == readings[i].claims && readings[i].operand < st.operands;
        const PartialOperand *operand = &st.operand[readings[i].operand];
        read = read && operand->order == (int)strlen(readings[i].by);
        for (int j = 0; read && j < operand->order; j++) read = operand->by[j] == readings[i].by[j];
        if (read) { right++; continue; }
        printf("  %s is not read as expected (%s)\n", readings[i].formula, syntax_error ? syntax_error : "another order or count");
    }
    syntax_error = failure = NULL;
    snprintf(summary, sizeof summary, "%d of %d notations read with the expected order of differentiation and components", right, total);
    passed &= check_result(right == total, summary);

    /* Controllo 11. */
    right = 0;
    total = (int)(sizeof mixed / sizeof *mixed);
    for (int i = 0; i < total; i++) {
        int confirmed = taylor_agrees(mixed[i].function, mixed[i].by, mixed[i].right), refuted = taylor_agrees(mixed[i].function, mixed[i].by, mixed[i].wrong);
        if (confirmed == 1 && refuted == -1) { right++; continue; }
        printf("  f_{%s} of %s: the right formula gives %d (1 wanted), the wrong one %d (-1 wanted)\n", mixed[i].by, mixed[i].function, confirmed, refuted);
    }
    snprintf(summary, sizeof summary, "%d of %d mixed derivatives confirmed, and their wrong versions contradicted, by the Taylor series along lines alone", right,
             total);
    passed &= check_result(right == total, summary);

    /* Controllo 12. */
    snprintf(summary, sizeof summary, "every step of the written proofs passed its independent check (%d discarded)", unjustified_steps);
    passed &= check_result(unjustified_steps == 0, summary);
    return passed ? 0 : 1;
}
