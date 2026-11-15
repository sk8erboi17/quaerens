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
 * I punti critici e le condizioni del secondo ordine (35g, capitolo 36):
 *
 *  13. ogni enunciato di critical_points.tex viene dimostrato, con la
 *      dimostrazione scritta, e con il certificato (controlli 1 e 2);
 *  14. ogni certificato, alterato, non è più accettato (controllo 3), e in
 *      più ogni confronto, sostituito dalla sua negazione (a < b diventa
 *      b <= a), viene respinto: non basta che non sia accettato;
 *  15. nessun enunciato di false_critical_points.tex viene dimostrato, e
 *      tutti vengono dichiarati falsi: una sella detta minimo, un minimo
 *      detto massimo, un punto che non è critico, il determinante nullo
 *      spacciato per positivo, e dodici risultati stampati sbagliati;
 *  16. il determinante svolto dal lettore (partial_minor()) contro un altro
 *      conto, l'eliminazione senza frazioni scritta qui: di tabelle di
 *      numeri a caso di lato da 1 a 4 l'enunciato con il valore giusto è
 *      dimostrato, e quello con il valore più 1 è dichiarato falso;
 *  17. ogni richiesta di critical_requests.tex riceve la risposta, senza
 *      certificato e con il motivo che lo dice, e alcune hanno proprio la
 *      condizione e i confronti del secondo ordine attesi (quelli dei libri);
 *  18. i punti critici attesi, senza derive() e senza la tabella dei segni:
 *      in ognuno il gradiente letto dalle sole serie di Taylor lungo le
 *      rette è nullo, e nel punto spostato di 1 non lo è;
 *  19. una derivata scritta due volte è un operando solo;
 *  20. gli estremi vincolati, che non hanno codice loro: ogni enunciato di
 *      constrained_extrema.tex (il vincolo, il gradiente della lagrangiana
 *      nullo, il determinante dell'hessiano orlato) viene dimostrato, quasi
 *      tutti con il certificato, i certificati alterati non sono accettati e
 *      i confronti negati sono respinti; nessun enunciato di
 *      false_constrained_extrema.tex viene dimostrato, e tutti vengono
 *      dichiarati falsi.
 *
 * Gli enunciati sono dati: il motore non ne conosce nessuno in anticipo. Che
 * siano veri e falsi l'ha controllato anche l'oracolo indipendente di
 * tools/fuzz/fuzz_multivariable.py (modalità check), che per le richieste
 * confronta la risposta con il gradiente punto per punto.
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
#define CRITICAL_FILE "tests/multivariable_calculus/critical_points.tex"
#define FALSE_CRITICAL_FILE "tests/multivariable_calculus/false_critical_points.tex"
#define CRITICAL_REQUEST_FILE "tests/multivariable_calculus/critical_requests.tex"
#define CONSTRAINED_FILE "tests/multivariable_calculus/constrained_extrema.tex"
#define FALSE_CONSTRAINED_FILE "tests/multivariable_calculus/false_constrained_extrema.tex"

/* Quanti enunciati restano senza certificato. In TRUE_FILE: x^y, la cui
 * derivata rispetto a x chiede al verificatore una relazione tra potenze
 * che non conosce. In BOOK_FILE: le derivate seconde di x^y, per lo stesso
 * motivo, e un gradiente in un punto con la radice di 3 tra le coordinate,
 * dove il verificatore non sa stabilire il segno di una condizione. In
 * CRITICAL_FILE nessuno. In CONSTRAINED_FILE: un punto e un moltiplicatore
 * con tre radici quadrate diverse (di 6, di 3/2 e di 2/3), tra cui il
 * verificatore non conosce relazioni. */
#define UNCERTIFIED 1
#define BOOK_UNCERTIFIED 2
#define CRITICAL_UNCERTIFIED 0
#define CONSTRAINED_UNCERTIFIED 1

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

/* Ritorna la fine dell'espressione che comincia in text, nella notazione dei
 * certificati: dove le sue parentesi si chiudono, o al primo spazio se è una
 * parola sola. */
static const char *expression_end(const char *text) {
    const char *stop = text;
    for (int depth = 0; *stop && (depth > 0 || (*stop != ' ' && *stop != ')')); stop++) depth += *stop == '(' ? 1 : *stop == ')' ? -1 : 0;
    return stop;
}

/* Controlli 2 e 3 su path, un file di enunciati veri: i verdetti DIMOSTRATO
 * senza certificato sono al più uncertified, e ogni certificato, alterato,
 * non è più accettato. Le alterazioni: la prima formula "is" più 1, e il
 * primo membro dell'ultima tesi con il segno di uguale più 1 (se ce n'è
 * una: un certificato può avere solo confronti, e li guarda il controllo
 * 14). */
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
        if (!is && !statement) { printf("  statement %d: the certificate has no formula and no thesis with an equal sign\n", total); accepted++; continue; }

        /* La formula: "is G" diventa "is (+ 1 G)". Un enunciato senza
         * derivate (il determinante di una tabella di numeri) non ne ha. */
        const char *line = is ? strchr(is + 1, '\n') : NULL;
        if (is) snprintf(altered[0], sizeof altered[0], "%.*s\nis (+ 1 %.*s)%s", (int)(is - certificate), certificate, (int)(line - is - 4), is + 4, line);

        /* La tesi: "(= L R)" diventa "(= (+ 1 L) R)". */
        if (statement) {
            const char *left = statement + strlen("\nstatement (= "), *stop = expression_end(left);
            snprintf(altered[1], sizeof altered[1], "%.*s(+ 1 %.*s)%s", (int)(left - certificate), certificate, (int)(stop - left), left, stop);
        }
        for (int i = 0; i < 2; i++) {
            const char *cursor_text = altered[i];
            if (i == 0 ? !is : !statement) continue;
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

/* Controllo 14 su path, un file di enunciati veri: in ogni certificato ogni
 * confronto, uno alla volta, viene sostituito dalla sua negazione, e il
 * verificatore deve respingere il certificato. "(< L R)" diventa
 * "(<= R L)", "(<= L R)" diventa "(< R L)", "(neq L R)" diventa "(= L R)":
 * ognuna è falsa esattamente quando l'originale è vera. Non basta "non
 * controllabile": il verificatore, che ha letto il segno per accettare
 * l'originale, lo legge anche qui. at_least è quanti confronti devono
 * essere stati provati. */
static int check_negated_comparisons(const char *path, int at_least) {
    static const struct { const char *word, *negated; int swap; } negation[] = {{"<= ", "<", 1}, {"< ", "<=", 1}, {"neq ", "=", 0}};
    static char altered[CERTIFICATE_BYTES];
    char summary[512];
    char *text = read_file(path);
    const char *position = text, *formula, *why;
    int total = 0, tried = 0, rejected = 0;
    while (text && (formula = next_formula(&position)) != NULL) {
        Verdict verdict = prove(formula, 'n', 0, 0);
        total++;
        if (verdict.outcome != PROVED || !verdict.certificate) continue;
        const char *certificate = verdict.certificate;
        for (const char *line = strstr(certificate, "\nstatement ("); line; line = strstr(line + 1, "\nstatement (")) {
            const char *word = line + strlen("\nstatement (");
            for (size_t k = 0; k < sizeof negation / sizeof *negation; k++) {
                size_t length = strlen(negation[k].word);
                if (strncmp(word, negation[k].word, length) != 0) continue;

                /* I due membri, e ciò che segue la tesi. */
                const char *left = word + length, *middle = expression_end(left), *right = middle + 1, *end = expression_end(right);
                const char *first = negation[k].swap ? right : left, *second = negation[k].swap ? left : right;
                int first_length = (int)(negation[k].swap ? end - right : middle - left), second_length = (int)(negation[k].swap ? middle - left : end - right);
                snprintf(altered, sizeof altered, "%.*s%s %.*s %.*s%s", (int)(word - certificate), certificate, negation[k].negated, first_length, first,
                         second_length, second, end);
                const char *cursor_text = altered;
                tried++;
                if (ck_check(&cursor_text, &why, 0) == CK_REJECTED) rejected++;
                else printf("  statement %d: a certificate with a comparison replaced by its negation is not rejected:\n%s\n", total, altered);
                break;
            }
        }
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d certificates of %s with a comparison replaced by its negation are rejected (at least %d wanted)", rejected, tried,
             path, at_least);
    return check_result(tried >= at_least && rejected == tried, summary);
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
    {"f(x, y) = x^{2} y", UNSUPPORTED, NULL},
    {"f(x, y) = x! y \\implies f_{x} = y", UNSUPPORTED, "regole di derivazione"},

    /* Antiregressione: una funzione definita con la derivata di un'altra
     * esiste solo dove esiste quell'altra. Con g = ln x + arcsin y la
     * derivata g_x = 1/x ha un valore anche per x < 0, dove g non c'è: la
     * prima tesi risultava DIMOSTRATO (senza certificato: l'arcoseno non ha
     * la sua parola, e il verificatore non poteva fermarla), anche sul
     * programma di prima. In (-1, 0) la funzione f non esiste. Lo stesso con
     * 1/y in y = 0. Dentro il dominio di g le stesse tesi sono vere, e con
     * altre lettere (g_u) il punto di f non tocca g. */
    {"g(x, y) = \\ln x + \\arcsin y, f(x, y) = g_{x} + x + y^{2} \\implies f_{x}(-1, 0) = 0", NOT_PROVED, "nel punto scritto"},
    {"g(x, y) = \\ln x + \\arcsin y, f(x, y) = g_{x} + x + y^{2} \\implies f(-1, 0) = -2", NOT_PROVED, "nel punto scritto"},
    {"g(x, y) = \\ln x + \\arcsin y, f(x, y) = g_{x} + x + y^{2} \\implies f_{xx}(-1, 0) < 0", NOT_PROVED, "nel punto scritto"},
    {"g(x, y) = \\ln x, f(x, y) = g_{x} + x + y^{2} \\implies f_{x}(-1, 0) = 0", NOT_PROVED, "nel punto scritto"},
    {"g(x, y) = \\ln x + \\frac{1}{y}, f(x, y) = g_{x} + x + y^{2} \\implies f_{x}(1, 0) = 0", NOT_PROVED, "nel punto scritto"},
    {"g(x, y) = \\ln x + \\arcsin y, f(x, y) = g_{x} + x + y^{2} \\implies f_{x}(1, 0) = 0", PROVED, NULL},
    {"g(x, y) = \\ln x + \\frac{1}{y}, f(x, y) = g_{x} + x + y^{2} \\implies f_{x}(1, 2) = 0", PROVED, NULL},
    {"g(u, v) = \\ln u + \\arcsin v, f(x, y) = g_{u} + x + y^{2} \\implies f_{x}(-1, 0) = 1", PROVED, NULL},
    {"g(x, y) = \\ln x + \\arcsin y, f(x, y) = g_{x} + x + y^{2} \\implies \\{(x, y) \\mid \\nabla f = 0\\}", UNSUPPORTED, "derivata di un'altra"},
    {"g(x, y) = \\ln x, f(x, y) = g(x, y) + \\frac{x^{2}}{2} + 2x \\implies \\{(x, y) \\mid \\nabla f = 0\\}", SOLVED, NULL},

    /* Antiregressione: una potenza con l'esponente non intero vuole la base
     * positiva, per il motore come per il verificatore. Con la base nulla
     * il motore dimostrava e il verificatore respingeva il certificato: il
     * verdetto era NON DIMOSTRATO, ma con un passo scartato (anche prima
     * della tappa 2, per il primo enunciato). Ora il motore non ci arriva, e
     * l'ultimo controllo di questo file, che vuole zero passi scartati, lo
     * verifica. Con la base positiva le stesse scritture sono dimostrate. */
    {"f(x, y) = 0^{\\frac{1}{2}} + x \\implies f_{x}(1, 1) = 1", NOT_PROVED, "nel punto scritto"},
    {"f(x, y) = y^{\\frac{3}{2}} + x \\implies f_{x}(1, 0) = 1", NOT_PROVED, "nel punto scritto"},
    {"f(x, y) = y^{\\frac{3}{2}} x \\implies f_{x}(1, 0) \\ge 0", NOT_PROVED, "nel punto scritto"},
    {"f(x, y) = x^{2} y \\implies (f_{y}(0, 1))^{\\frac{1}{2}} \\ge 0", NOT_PROVED, "base positiva"},
    {"f(x, y) = x^{2} y \\implies f_{x}(0, 1) \\ge 0^{\\frac{1}{2}}", NOT_PROVED, "base positiva"},
    {"f(x, y) = x^{2} y \\implies f_{x}(1, 1) > 0^{\\frac{1}{2}}", NOT_PROVED, "base positiva"},
    {"f(x, y) = x^{2} + y^{2} + 0^{\\frac{1}{2}} \\implies \\det H_{f} > 0", NOT_PROVED, "base positiva"},
    {"f(x, y) = y^{\\frac{3}{2}} + x \\implies f_{y}(1, 4) = 3", PROVED, NULL},
    {"f(x, y) = y^{\\frac{3}{2}} x \\implies f_{x}(1, 4) > 7", PROVED, NULL},
    {"f(x, y) = x^{2} y \\implies f_{x}(1, 1) \\ge (f_{x}(2, 1))^{\\frac{1}{2}}", PROVED, NULL},
    {"f(x, y) = x^{2} + y^{2} + 4^{\\frac{1}{2}} \\implies \\det H_{f} > 0", PROVED, NULL},
    {"f(x, y) = x^{y} \\implies f_{x}(2, 3) = 12", PROVED, NULL},

    /* Un confronto è trattato solo tra numeri: con una lettera rimasta, la
     * variabile o un parametro, è una disequazione, e qui non si decide.
     * (Prima della tappa 2 il lettore non leggeva \ge, e il motivo di
     * questo enunciato era "testo inatteso": l'esito non è cambiato.) */
    {"f(x, y) = x^{2} y \\implies f_{x} \\ge 2xy", UNSUPPORTED, "solo tra numeri"},
    {"f(x, y) = x^{2} y \\implies f_{xx} > 0", UNSUPPORTED, "solo tra numeri"},
    {"f(x, y) = x^{3} + y^{2} \\implies \\det H_{f} > 0", UNSUPPORTED, "solo tra numeri"},

    /* Senza punto, ma con le derivate seconde costanti: i due membri sono
     * numeri lo stesso, e il confronto vale in ogni punto. */
    {"f(x, y) = x^{2} + y^{2} \\implies \\det H_{f} > 0", PROVED, NULL},
    {"f(x, y) = x^{2} + y^{2} \\implies \\det H_{f} < 0", REFUTED, NULL},
    {"f(x, y) = a x^{2} + y^{2} \\implies f_{xx}(1, 1) > 0", UNSUPPORTED, "solo tra numeri"},

    /* Un gradiente o un'hessiana sono liste: si dicono uguali, non diversi
     * o maggiori. Che una componente non sia zero si scrive con quella. */
    {"f(x, y) = x^{2} y \\implies \\nabla f(1, 1) \\neq (0, 0)", UNSUPPORTED, "solo con il segno di uguale"},
    {"f(x, y) = x^{2} y \\implies H_{f}(1, 1) > 0", UNSUPPORTED, "solo con il segno di uguale"},
    {"f(x, y) = x^{2} y \\implies f_{x}(1, 1) \\neq 0", PROVED, NULL},

    /* Un punto in cui la funzione non è regolare: il segno non si afferma. */
    {"f(x, y) = \\sqrt{x^{2} + y^{2}} \\implies f_{xx}(0, 0) > 0", NOT_PROVED, "nel punto scritto"},
    {"f(x, y) = \\ln(xy) \\implies \\det H_{f}(-1, 1) > 0", NOT_PROVED, "nel punto scritto"},

    /* I numeri da soli non dimostrano: questi secondi membri differiscono
     * dal valore vero, 2, meno di quanto gli intervalli sappiano vedere. Il
     * primo confronto è vero e il secondo falso: nessuno dei due è deciso, e
     * con il valore esatto l'algebra dice che il confronto stretto è falso. */
    {"f(x, y) = x^{2} y \\implies f_{xx}(1, 1) > 2 - \\frac{1}{10^{30}}", NOT_PROVED, "non so stabilire il segno"},
    {"f(x, y) = x^{2} y \\implies f_{xx}(1, 1) > 2 + \\frac{1}{10^{30}}", NOT_PROVED, "non so stabilire il segno"},
    {"f(x, y) = x^{2} y \\implies f_{xx}(1, 1) > 2", REFUTED, NULL},
    {"f(x, y) = x^{2} y \\implies f_{xx}(1, 1) \\ge 2", PROVED, NULL},

    /* Il determinante: dell'hessiana di una funzione dichiarata, o di una
     * tabella quadrata di lato al più 4; un numero che lo moltiplica vuole
     * \cdot, come per ogni comando che il capitolo 9 non conosce. */
    {"f(x, y) = x^{3} + y^{3} - 3xy \\implies 2 \\cdot \\det H_{f}(1, 1) = 54", PROVED, NULL},
    {"f(x, y) = x^{3} + y^{3} - 3xy \\implies 2 \\det H_{f}(1, 1) = 54", UNSUPPORTED, "testo inatteso"},
    {"f(x, y) = x^{3} + y^{3} - 3xy \\implies \\det H_{g}(1, 1) = 27", UNSUPPORTED, "dopo \\det"},
    {"f(x, y) = x^{3} + y^{3} - 3xy \\implies \\det f_{xx}(1, 1) = 6", UNSUPPORTED, "dopo \\det"},
    {"f(x, y) = x \\implies \\det \\begin{pmatrix} 1 & 2 & 3 \\\\ 4 & 5 & 6 \\end{pmatrix} = 0", UNSUPPORTED, "quadrata"},
    {"f(x, y) = x \\implies \\det \\begin{pmatrix} 1 & 0 & 0 & 0 & 0 \\\\ 0 & 1 & 0 & 0 & 0 \\\\ 0 & 0 & 1 & 0 & 0 \\\\ 0 & 0 & 0 & 1 & 0 \\\\ 0 & 0 & 0 & 0 & 1 \\end{pmatrix} = 1",
     UNSUPPORTED, "al più 4"},
    {"f(x, y) = x \\implies \\begin{pmatrix} 1 & 2 \\\\ 3 & 4 \\end{pmatrix} = -2", UNSUPPORTED, "tra due sbarre"},
    {"f(x, y) = x \\implies \\det \\begin{vmatrix} 1 & 2 \\\\ 3 & 4 \\end{vmatrix} = -2", UNSUPPORTED, "dopo \\det"},
    {"f(x, y) = x \\implies \\det \\begin{pmatrix} 1 & 2 \\\\ 3 \\end{pmatrix} = -2", UNSUPPORTED, "stesso numero di elementi"},
    {"f(x, y) = x^{2} y \\implies \\det H_{f}(1, 1) > 0 \\land \\det H_{f}", UNSUPPORTED, "deve essere la sola tesi"},

    /* La richiesta dei punti critici: le scritture che il lettore non
     * accetta, e ciò che il capitolo 36 non tratta, con il motivo. */
    {"f(x, y) = x^{2} + y^{2} \\implies \\{(y, x) \\mid \\nabla f(x, y) = (0, 0)\\}", UNSUPPORTED, "nel loro ordine"},
    {"f(x, y) = x^{2} + y^{2} \\implies \\{(x, y) \\mid \\nabla f(1, 2) = (0, 0)\\}", UNSUPPORTED, "nel loro ordine"},
    {"f(x, y) = x^{2} + y^{2} \\implies \\{(x, y) \\mid \\nabla f(x, y) = (0, 1)\\}", UNSUPPORTED, "uguale a zero"},
    {"f(x, y) = x^{2} + y^{2} \\implies \\{(x, y) \\mid \\nabla g(x, y) = (0, 0)\\}", UNSUPPORTED, "funzione dichiarata"},
    {"f(x, y) = x^{2} + y^{2} \\implies \\{(x, y) \\mid f_{x} = 0\\}", UNSUPPORTED, "uguale a zero"},
    {"f(x, y) = x^{2} + y^{2} \\implies \\{(x, y) \\mid \\nabla f(x, y) = (0, 0)\\} \\land f_{x} = 2x", UNSUPPORTED, "testo inatteso"},
    {"f(x, y) = x^{2} + y^{2} \\implies \\{(x, y) \\mid \\nabla f(x, y) = (0, 0)", UNSUPPORTED, "uguale a zero"},
    {"f(x, y, z) = x^{2} + y^{2} + z^{2} \\implies \\{(x, y, z) \\mid \\nabla f(x, y, z) = (0, 0, 0)\\}", UNSUPPORTED, "due variabili"},
    {"f(x, y) = a x^{2} + y^{2} \\implies \\{(x, y) \\mid \\nabla f(x, y) = (0, 0)\\}", UNSUPPORTED, "senza parametri"},
    {"f(x, y) = (x^{2} + 2y^{2}) e^{-x^{2} - y^{2}} \\implies \\{(x, y) \\mid \\nabla f(x, y) = (0, 0)\\}", UNSUPPORTED, "non è risolto"},
    {"f(x, y) = \\ln(x^{2} + y^{2} + 1) \\implies \\{(x, y) \\mid \\nabla f(x, y) = (0, 0)\\}", SOLVED, NULL},
    {"f(x, y) = \\sqrt{x^{2} + y^{2}} \\implies \\{(x, y) \\mid \\nabla f(x, y) = (0, 0)\\}", UNSUPPORTED, "non è risolto"},
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

/* Controllo 16. Ritorna il determinante della matrice di interi m, di lato
 * size, che il conto rovina: l'eliminazione senza frazioni di Bareiss, in
 * cui ogni divisione è esatta. È un altro conto dallo sviluppo lungo la
 * prima riga che fa il lettore (partial_minor()), e non ne usa nulla. */
static long long elimination_determinant(long long m[4][4], int size) {
    long long previous = 1, sign = 1;
    for (int k = 0; k + 1 < size; k++) {
        /* Un perno nullo si scambia con una riga sotto, e il segno cambia. */
        int row = k;
        while (row < size && m[row][k] == 0) row++;
        if (row == size) return 0;
        for (int j = 0; row != k && j < size; j++) { long long swap = m[k][j]; m[k][j] = m[row][j]; m[row][j] = swap; }
        if (row != k) sign = -sign;
        for (int i = k + 1; i < size; i++)
            for (int j = k + 1; j < size; j++) m[i][j] = (m[k][k] * m[i][j] - m[i][k] * m[k][j]) / previous;
        previous = m[k][k];
    }
    return sign * m[size - 1][size - 1];
}

/* Controllo 16: tabelle di numeri a caso (sempre le stesse: il generatore
 * parte da un seme fisso), di lato da 1 a 4, scritte con \det e, quelle di
 * lato 2, anche tra due sbarre. Con il determinante calcolato qui
 * l'enunciato deve essere dimostrato; con quel numero più 1, dichiarato
 * falso. */
static int check_determinants(void) {
    static char formula[2][MAX_FORMULA];
    char summary[512];
    unsigned state = 20261115u;
    int total = 0, right = 0;
    for (int round = 0; round < 40; round++) {
        long long m[4][4];
        int size = 1 + round % 4, bars = size == 2 && round % 8 == 1, length = 0;
        length += snprintf(formula[0], sizeof formula[0], "f(x, y) = x \\implies %s", bars ? "\\begin{vmatrix} " : "\\det \\begin{pmatrix} ");
        for (int i = 0; i < size; i++)
            for (int j = 0; j < size; j++) {
                state = state * 1103515245u + 12345u;
                m[i][j] = (long long)(state >> 16) % 11 - 5;
                length += snprintf(formula[0] + length, sizeof formula[0] - (size_t)length, "%s%lld", j ? " & " : i ? " \\\\ " : "", m[i][j]);
            }
        long long value = elimination_determinant(m, size);
        snprintf(formula[1], sizeof formula[1], "%s %s = %lld", formula[0], bars ? "\\end{vmatrix}" : "\\end{pmatrix}", value + 1);
        snprintf(formula[0] + length, sizeof formula[0] - (size_t)length, " %s = %lld", bars ? "\\end{vmatrix}" : "\\end{pmatrix}", value);
        Outcome good = prove(formula[0], 'n', 0, 0).outcome, bad = prove(formula[1], 'n', 0, 0).outcome;
        total++;
        if (good == PROVED && bad == REFUTED) { right++; continue; }
        printf("  %s is %s (PROVED wanted), and with the value plus 1 it is %s (REFUTED wanted)\n", formula[0], outcome_name(good), outcome_name(bad));
    }
    snprintf(summary, sizeof summary, "%d of %d determinants of tables of numbers agree with fraction-free elimination, and are called false with the value plus 1",
             right, total);
    return check_result(right == total, summary);
}

/* Una richiesta dei punti critici con ciò che il libro stampa: la funzione,
 * la condizione attesa (come la scrive il motore: per casi sul parametro), i
 * confronti del secondo ordine attesi nella nota, e i punti critici, come
 * coordinate in LaTeX separate da ';' (per il controllo 18), o NULL se non
 * sono un elenco finito. */
typedef struct {
    const char *function, *answer, *second, *points;
} BookCritical;

static const BookCritical book_critical[] = {
    /* Magni 14.33. */
    {"x^{3} + y^{3} - 3xy", "(y = 0 \\land x = 0) \\lor (y = 1 \\land x = 1)", "(0, 0): \\det H_{f} < 0; (1, 1): \\det H_{f} > 0 \\land f_{xx} > 0", "0, 0; 1, 1"},
    /* Lancelotti 1.8. */
    {"\\frac{1}{3}(x^{3} + y^{3}) - 16x - 25y", "(y = -5 \\land (x = -4 \\lor x = 4)) \\lor (y = 5 \\land (x = -4 \\lor x = 4))",
     "(-4, -5): \\det H_{f} > 0 \\land f_{xx} < 0; (4, -5): \\det H_{f} < 0; (-4, 5): \\det H_{f} < 0; (4, 5): \\det H_{f} > 0 \\land f_{xx} > 0", "-4, -5; 4, -5; -4, 5; 4, 5"},
    /* Lancelotti 1.1. */
    {"\\frac{x}{1 + x^{2} + y^{2}}", "y = 0 \\land (x = -1 \\lor x = 1)", "(-1, 0): \\det H_{f} > 0 \\land f_{xx} > 0; (1, 0): \\det H_{f} > 0 \\land f_{xx} < 0", "-1, 0; 1, 0"},
    /* Roma Tre 7. */
    {"2x^{2} y + 2x y^{2} - x^{2} y^{2} - 4xy", "(y = 0 \\land (x = 0 \\lor x = 2)) \\lor (y = 1 \\land x = 1) \\lor (y = 2 \\land (x = 0 \\lor x = 2))",
     "(1, 1): \\det H_{f} > 0 \\land f_{xx} > 0", "0, 0; 2, 0; 1, 1; 0, 2; 2, 2"},
    /* Habala 1. */
    {"2x^{3} + 9x y^{2} + 15x^{2} + 27y^{2}", "(y = -2 \\land x = -3) \\lor (y = 0 \\land (x = -5 \\lor x = 0)) \\lor (y = 2 \\land x = -3)",
     "(-5, 0): \\det H_{f} > 0 \\land f_{xx} < 0; (0, 0): \\det H_{f} > 0 \\land f_{xx} > 0", "-3, -2; -5, 0; 0, 0; -3, 2"},
    /* Ferrara 1(ii): il libro elenca solo i due estremi; le due selle ci sono. */
    {"x^{3} + x y^{2} - 2x^{2} + 4xy", "(y = -4 \\land x = 0) \\lor (y = -2 \\land (x = -\\frac{2}{3} \\lor x = 2)) \\lor (y = 0 \\land x = 0)",
     "(0, -4): \\det H_{f} < 0; (-\\frac{2}{3}, -2): \\det H_{f} > 0 \\land f_{xx} < 0; (2, -2): \\det H_{f} > 0 \\land f_{xx} > 0; (0, 0): \\det H_{f} < 0",
     "0, -4; -\\frac{2}{3}, -2; 2, -2; 0, 0"},
    /* Esparza 2(c): il libro dimentica (-2, 2). */
    {"(y - 2) x^{2} - y^{2}", "(y = 0 \\land x = 0) \\lor (y = 2 \\land (x = -2 \\lor x = 2))",
     "(0, 0): \\det H_{f} > 0 \\land f_{xx} < 0; (-2, 2): \\det H_{f} < 0; (2, 2): \\det H_{f} < 0", "0, 0; -2, 2; 2, 2"},
    /* Leaci: un solo punto critico. */
    {"\\frac{2(x + y) - 1}{x^{2} + y^{2}}", "y = \\frac{1}{2} \\land x = \\frac{1}{2}", "(\\frac{1}{2}, \\frac{1}{2}): \\det H_{f} > 0 \\land f_{xx} < 0", "\\frac{1}{2}, \\frac{1}{2}"},
    /* Perfetti 6.7: nell'origine il determinante è nullo. */
    {"(x + y)^{2} - x^{4} - y^{4}", "(y = -1 \\land x = -1) \\lor (y = 0 \\land x = 0) \\lor (y = 1 \\land x = 1)",
     "(-1, -1): \\det H_{f} > 0 \\land f_{xx} < 0; (0, 0): \\det H_{f} = 0; (1, 1): \\det H_{f} > 0 \\land f_{xx} < 0", "-1, -1; 0, 0; 1, 1"},
    /* Boschetti: nove punti. */
    {"2(x^{2} + y^{2}) - (x^{4} + y^{4}) + 3",
     "(y = -1 \\land (x = -1 \\lor x = 0 \\lor x = 1)) \\lor (y = 0 \\land (x = -1 \\lor x = 0 \\lor x = 1)) \\lor (y = 1 \\land (x = -1 \\lor x = 0 \\lor x = 1))",
     "(0, 0): \\det H_{f} > 0 \\land f_{xx} > 0", "-1, -1; 0, -1; 1, -1; -1, 0; 0, 0; 1, 0; -1, 1; 0, 1; 1, 1"},
    /* Bonanno: coordinate con una radice. */
    {"x^{4} + 2x^{2} y - y^{3}", "(y = -\\frac{2}{3} \\land (x = -\\frac{\\sqrt{6}}{3} \\lor x = \\frac{\\sqrt{6}}{3})) \\lor (y = 0 \\land x = 0)",
     "(0, 0): \\det H_{f} = 0", "-\\frac{\\sqrt{6}}{3}, -\\frac{2}{3}; \\frac{\\sqrt{6}}{3}, -\\frac{2}{3}; 0, 0"},
    /* Eleuteri 5.1.2. */
    {"x^{4} + y^{4} - 2(x - y)^{2}", "(y = -\\sqrt{2} \\land x = \\sqrt{2}) \\lor (y = 0 \\land x = 0) \\lor (y = \\sqrt{2} \\land x = -\\sqrt{2})",
     "(\\sqrt{2}, -\\sqrt{2}): \\det H_{f} > 0 \\land f_{xx} > 0; (0, 0): \\det H_{f} = 0", "\\sqrt{2}, -\\sqrt{2}; 0, 0; -\\sqrt{2}, \\sqrt{2}"},
    /* Roma Tre 9: un solo punto, e il test non decide. */
    {"x^{4} - 4x^{2} y + y^{2}", "y = 0 \\land x = 0", "(0, 0): \\det H_{f} = 0", "0, 0"},
    /* Eleuteri 5.1.11: i due assi e un punto. */
    {"x^{3} y^{2} - x^{4} y^{2} - x^{3} y^{3}",
     "(x < 0 \\land y = 0) \\lor x = 0 \\lor (0 < x < \\frac{1}{2} \\land y = 0) \\lor (x = \\frac{1}{2} \\land (y = 0 \\lor y = \\frac{1}{3})) \\lor (x > \\frac{1}{2} \\land y = 0)",
     "non sono un elenco finito", NULL},
    /* Lancelotti 1.7: i due assi. */
    {"x^{3} y^{2} + 1", "(y < 0 \\land x = 0) \\lor y = 0 \\lor (y > 0 \\land x = 0)", "non sono un elenco finito", NULL},
    /* Un valore assoluto: dove il suo argomento vale zero la funzione non è
     * regolare, e ha un minimo in (1, 0) che non è un punto critico. La
     * risposta "nessun punto critico" deve dire che quei punti non li ha
     * guardati. */
    {"x^{3} - 3x + |y|", "\\emptyset", "non è regolare", NULL},
    /* Un logaritmo: la sua derivata ha un valore anche dove lui non c'è.
     * Vitali 5 e Carriero: l'altro punto in cui il gradiente si annulla,
     * (-3/2, -1/2) e (1, -2), è fuori dal dominio, come dicono i libri. */
    {"x^{2} + 3y^{2} - 3\\ln(x + y + 1)", "y = \\frac{1}{4} \\land x = \\frac{3}{4}", "(\\frac{3}{4}, \\frac{1}{4}): \\det H_{f} > 0 \\land f_{xx} > 0",
     "\\frac{3}{4}, \\frac{1}{4}"},
    {"\\frac{2}{y^{2}} + \\ln(xy) + \\frac{1}{x}", "y = 2 \\land x = 1", "(1, 2): \\det H_{f} > 0 \\land f_{xx} > 0", "1, 2"},
    /* Antiregressione: un RISOLTO falso. La derivata 1/x + x + 2 vale zero
     * in x = -1, dove ln x non esiste: il sistema del solo gradiente dava la
     * retta x = -1 come insieme dei punti critici, e con y^2 in più il punto
     * (-1, 0), fermato solo dalla rilettura. La funzione non ha punti
     * critici. */
    {"\\ln x + \\frac{x^{2}}{2} + 2x", "\\emptyset", NULL, NULL},
    {"\\ln x + \\frac{x^{2}}{2} + 2x + y^{2}", "\\emptyset", NULL, NULL},
    /* Lo stesso con il valore assoluto dentro il logaritmo: in x = -1 la
     * funzione c'è, e il punto è critico. */
    {"\\ln|x| + \\frac{x^{2}}{2} + 2x + y^{2}", "y = 0 \\land x = -1", "(-1, 0): \\det H_{f} = 0", "-1, 0"},
    /* Nessun punto critico, e tutti. */
    {"x + 2y", "\\emptyset", NULL, NULL},
    {"7", "\\mathbb{R}^{2}", NULL, NULL},
};

/* Ritorna 1 se nel punto di coordinate at (in LaTeX, separate da una
 * virgola) le due derivate prime di function, lette dalle sole serie di
 * Taylor lungo le rette (partial_taylor()), sono zero; -1 se una delle due
 * è di sicuro diversa da zero; 0 se non si sa. Non chiama derive(). */
static int gradient_vanishes(const char *function, const char *at) {
    static PartialStatement st;
    static char text[MAX_FORMULA];
    int vanishes = 1;
    arena_used = arena_keep;
    failure = NULL;
    snprintf(text, sizeof text, "f(x, y) = %s \\implies f_{x}\\left( %s \\right) = 0 \\land f_{y}\\left( %s \\right) = 0", function, at, at);
    if (!parse_partial_statement(text, &st) || failure || st.operands != 2) { syntax_error = failure = NULL; return 0; }
    partial_letters(&st);
    for (int k = 0; k < 2 && vanishes > 0; k++) {
        int agreement = numbers_agree(partial_taylor(&st.operand[k], &st.function[0], st.function[0].body, st.operand[k].point, 0), range_ratio(0, 1));
        vanishes = agreement < 0 ? -1 : agreement > 0;
    }
    failure = NULL;
    return vanishes;
}

/* Controlli 17 e 18 sulle richieste di book_critical[]. */
static int check_critical_requests(void) {
    static char request[MAX_FORMULA], moved[256];
    char summary[512];
    int total = (int)(sizeof book_critical / sizeof *book_critical), exact = 0, points = 0, vanishing = 0, moved_away = 0;
    for (int i = 0; i < total; i++) {
        const BookCritical *c = &book_critical[i];
        snprintf(request, sizeof request, "f(x, y) = %s \\implies \\{(x, y) \\mid \\nabla f(x, y) = (0, 0)\\}", c->function);
        Verdict verdict = prove(request, 'n', 0, 0);
        int honest = !verdict.certificate && verdict.reason && strstr(verdict.reason, "non ha certificato");
        int second = c->second ? verdict.note && strstr(verdict.note, c->second) != NULL : !verdict.note;
        if (verdict.outcome == SOLVED && honest && second && strcmp(verdict.detail, c->answer) == 0) exact++;
        else {
            printf("  %s: expected %s%s%s, without a certificate, got ", request, c->answer, c->second ? " with a note mentioning " : "", c->second ? c->second : "");
            verdict_print(stdout, &verdict);
            putchar('\n');
        }

        /* Controllo 18: ogni punto atteso, e lo stesso con la x aumentata
         * di un settimo, che per queste funzioni non è critico. */
        for (const char *at = c->points; at; at = strchr(at, ';') ? strchr(at, ';') + 1 : NULL) {
            char one[128];
            snprintf(one, sizeof one, "%.*s", (int)strcspn(at, ";"), at);
            const char *comma = strchr(one, ',');
            snprintf(moved, sizeof moved, "\\frac{1}{7} + \\left( %.*s \\right)%s", (int)(comma - one), one, comma);
            points++;
            int here = gradient_vanishes(c->function, one), away = gradient_vanishes(c->function, moved);
            vanishing += here == 1;
            moved_away += away == -1;
            if (here != 1 || away != -1) printf("  %s at (%s): the Taylor series give %d (1 wanted), and at (%s) they give %d (-1 wanted)\n", c->function, one, here, moved, away);
        }
    }
    snprintf(summary, sizeof summary, "%d of %d requests for critical points answered as the books do, without a certificate and saying so", exact, total);
    int passed = check_result(exact == total, summary);
    snprintf(summary, sizeof summary, "%d of %d expected critical points have a vanishing gradient by the Taylor series alone, and %d of %d moved points do not",
             vanishing, points, moved_away, points);
    return check_result(points > 0 && vanishing == points && moved_away == points, summary) && passed;
}

/* Controllo 19: quanti operandi ha letto il lettore in formula; -1 se non
 * la legge. */
static int operands_read(const char *formula) {
    static PartialStatement st;
    arena_used = arena_keep;
    failure = NULL;
    int count = parse_partial_statement(formula, &st) && !failure ? st.operands : -1;
    syntax_error = failure = NULL;
    return count;
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

    /* Controlli da 13 a 18: i punti critici. */
    passed &= check_all_proved(CRITICAL_FILE);
    passed &= check_certificates(CRITICAL_FILE, CRITICAL_UNCERTIFIED);
    passed &= check_negated_comparisons(CRITICAL_FILE, 200);
    passed &= check_outcomes(FALSE_CRITICAL_FILE, PROVED, REFUTED, 0);
    passed &= check_determinants();
    passed &= check_outcomes(CRITICAL_REQUEST_FILE, PROVED, SOLVED, 0);
    passed &= check_critical_requests();

    /* Controllo 19: f_xx(1, 1) da sola e dentro il determinante è un
     * operando solo, come le quattro derivate seconde scritte due volte; in
     * due punti diversi sono operandi diversi. */
    int once = operands_read("f(x, y) = x^{2} y \\implies f_{xx}(1, 1) + \\det H_{f}(1, 1) = 0"),
        twice = operands_read("f(x, y) = x^{2} y \\implies \\det H_{f}(1, 1) = \\det H_{f}(1, 1)"),
        apart = operands_read("f(x, y) = x^{2} y \\implies f_{xx}(1, 1) = f_{xx}(1, 2)");
    snprintf(summary, sizeof summary, "a derivative written twice is read as one operand (%d, %d and %d operands; 4, 4 and 2 wanted)", once, twice, apart);
    passed &= check_result(once == 4 && twice == 4 && apart == 2, summary);

    /* Controllo 20: gli estremi vincolati. */
    passed &= check_all_proved(CONSTRAINED_FILE);
    passed &= check_certificates(CONSTRAINED_FILE, CONSTRAINED_UNCERTIFIED);
    passed &= check_negated_comparisons(CONSTRAINED_FILE, 8);
    passed &= check_outcomes(FALSE_CONSTRAINED_FILE, PROVED, REFUTED, 0);

    /* Controllo 12. */
    snprintf(summary, sizeof summary, "every step of the written proofs passed its independent check (%d discarded)", unjustified_steps);
    passed &= check_result(unjustified_steps == 0, summary);
    return passed ? 0 : 1;
}
