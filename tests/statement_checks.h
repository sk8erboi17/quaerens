/* Controlli condivisi dai test di dominio.
 *
 * Ogni dominio ha due file di dati: enunciati veri, che il motore deve
 * dimostrare, ed enunciati falsi, che non deve dimostrare mai. I controlli
 * sono gli stessi per tutti i domini, quindi stanno qui una volta sola; i test
 * di dominio dicono solo quali file leggere. Questo non è un test: non ha un
 * main() e non contiene enunciati.
 *
 * I controlli sono cinque.
 *
 *   1. check_all_proved(): ogni enunciato del file è dimostrato, e il motore
 *      ne ha scritto la dimostrazione.
 *
 *   2. check_false_variants(): antiregressione. Da ogni uguaglianza L = R del
 *      file si ricavano due enunciati che non possono essere veri insieme a
 *      quello di partenza, e nessuno dei due deve risultare dimostrato:
 *
 *          L = R + 1          sbagliato già nel caso base;
 *          L = R + n - n0     giusto nel caso base n = n0, sbagliato dopo.
 *
 *      Il secondo è il più insidioso: supera il caso base, e solo il passo
 *      induttivo può fermarlo. Se una modifica al motore lo rendesse più
 *      permissivo pur di dimostrare un enunciato in più, queste varianti
 *      comincerebbero a passare e il test lo segnalerebbe.
 *
 *   3. check_none_proved(): antiregressione. Nessun enunciato di un file di
 *      formule sbagliate è dimostrato.
 *
 *   4. check_none_refuted(): l'altro verso. Nessun enunciato di un file di
 *      enunciati veri che il motore non sa ancora decidere è dichiarato
 *      falso: può rispondere che non lo sa, non che è falso.
 *
 *   5. check_structure_certificates(): ogni verdetto del capitolo 28 del
 *      file, DIMOSTRATO o FALSO, porta un certificato che il verificatore,
 *      rileggendolo, accetta; e lo stesso certificato alterato (senza le
 *      voci instance e witness, o con il modello dichiarato del tipo
 *      opposto) non è accettato.
 *
 * Ogni funzione stampa i casi che non vanno e una riga finale PASS o FAIL, e
 * ritorna 1 se il controllo è riuscito. Sono `static inline` perché un test può
 * usarne solo alcune senza che il compilatore segnali le altre come inutilizzate.
 */

/* Il programma ha il proprio main(), in main.c: lo si include cambiando
 * nome a quel main, così i test possono chiamarne le funzioni interne. */
#define main mathresearcher_main
#include "../main.c"
#undef main

/* Stampa l'esito di un controllo e lo ritorna: 1 se riuscito. */
static inline int check_result(int passed, const char *summary) {
    printf("%s: %s\n", passed ? "PASS" : "FAIL", summary);
    return passed;
}

/* Controllo 1: ogni enunciato di path deve essere dimostrato, con la sua
 * dimostrazione scritta. */
static inline int check_all_proved(const char *path) {
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }

    /* Un enunciato alla volta; si stampano solo quelli non dimostrati. */
    const char *position = text, *item, *end;
    int total = 0, proved = 0, unwritten = 0;
    while (next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        total++;
        if (verdict.outcome == PROVED) {
            /* Un enunciato dimostrato senza dimostrazione scritta non basta:
             * il motore deve anche dire perché. */
            proved++;
            if (verdict.proof && verdict.proof[0]) continue;
            unwritten++;
            printf("  statement %d is proved but has no written proof\n", total);
            continue;
        }
        printf("  statement %d is not proved: ", total);
        verdict_print(stdout, &verdict);
        putchar('\n');
    }
    free(text);

    snprintf(summary, sizeof summary, "%d of %d statements proved, each with a written proof, in %s", proved - unwritten, total, path);
    return check_result(total > 0 && proved == total && unwritten == 0, summary);
}

/* Scrive in out la variante di formula con addend aggiunto al membro destro:
 * da "L = R" a "L = \left(R\right) addend". Ritorna 1 se riesce, 0 se formula
 * non è un'uguaglianza che il parser sa leggere o se out è troppo piccolo.
 *
 * Per trovare dove finisce il membro sinistro si usa il parser del programma:
 * cercare il carattere '=' non basterebbe, perché compare anche nel pedice di
 * \sum. Dopo aver letto un'espressione, il token corrente è la relazione. */
static inline int make_variant(const char *formula, const char *addend, char *out, size_t size) {
    /* Un enunciato su una successione definita per ricorrenza ha le ipotesi
     * davanti a \implies: la variante riguarda la tesi, e solo se è una
     * sola, perché di più tesi non si sa quale uguaglianza alterare. */
    const char *thesis = strstr(formula, "\\implies");
    thesis = thesis ? thesis + strlen("\\implies") : formula;
    if (thesis != formula && strstr(thesis, "\\land")) return 0;

    arena_used = arena_keep;
    failure = NULL;
    syntax_error = NULL;
    cursor = thesis;
    next_token();
    parse_expr();
    if (syntax_error || failure || !is_char('=')) return 0;
    const char *relation = token.start;
    int written = snprintf(out, size, "%.*s = \\left(%s\\right) %s",
                           (int)(relation - formula), formula, relation + 1, addend);
    return written > 0 && (size_t)written < size;
}

/* Controllo 2: nessuna variante falsa degli enunciati di path deve essere
 * dimostrata. Gli enunciati che non sono uguaglianze vengono saltati:
 * aggiungere 1 a un membro di una disuguaglianza non la rende falsa. */
static inline int check_false_variants(const char *path) {
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }

    const char *position = text, *item, *end;
    int number = 0, variants = 0, proved = 0;
    while (next_item(&position, &item, &end)) {
        Item statement;
        number++;
        if (read_item(item, end, &statement)) continue;

        /* L'enunciato di partenza dice da quale n0 parte l'induzione. */
        Verdict original = prove(statement.formula, statement.variable, statement.has_base, statement.base);
        char addend[2][64];
        snprintf(addend[0], sizeof addend[0], "+ 1");
        snprintf(addend[1], sizeof addend[1], "+ %c - \\left(%lld\\right)", original.variable, original.base);

        /* Le due varianti, con lo stesso caso base dell'originale. */
        for (int v = 0; v < 2; v++) {
            char variant[2 * MAX_FORMULA];
            if (!make_variant(statement.formula, addend[v], variant, sizeof variant)) continue;
            Verdict verdict = prove(variant, original.variable, 1, original.base);
            variants++;
            if (verdict.outcome != PROVED) continue;
            proved++;
            printf("  statement %d: a false variant was proved: %s\n", number, variant);
        }
    }
    free(text);

    snprintf(summary, sizeof summary, "%d of %d false variants rejected for %s", variants - proved, variants, path);
    return check_result(variants > 0 && proved == 0, summary);
}

/* Controllo 3: nessun enunciato di path, un file di formule sbagliate, deve
 * essere dimostrato. */
static inline int check_none_proved(const char *path) {
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }

    const char *position = text, *item, *end;
    int total = 0, proved = 0;
    while (next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        total++;
        if (verdict.outcome != PROVED) continue;
        proved++;
        printf("  statement %d is false but was proved\n", total);
    }
    free(text);

    snprintf(summary, sizeof summary, "%d of %d false statements rejected in %s", total - proved, total, path);
    return check_result(total > 0 && proved == 0, summary);
}

/* Controllo 4: nessun enunciato di path, un file di enunciati veri che il
 * motore può non saper decidere, deve essere dichiarato falso. Quanti ne
 * dimostra viene solo riferito: quando uno diventa dimostrabile va spostato
 * nel file di quelli che devono esserlo. */
static inline int check_none_refuted(const char *path) {
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
        if (verdict.outcome == PROVED) proved++;
        if (verdict.outcome != REFUTED) continue;
        refuted++;
        printf("  statement %d is true but was called false\n", total);
    }
    free(text);

    snprintf(summary, sizeof summary, "none of the %d true statements of %s is called false (%d are proved)", total, path, proved);
    return check_result(total > 0 && refuted == 0, summary);
}

/* Ritorna 1 se il verdetto viene dal capitolo 28: il metodo è uno dei suoi
 * due. */
static inline int from_chapter_28(const Verdict *verdict) {
    return (verdict->outcome == PROVED || verdict->outcome == REFUTED) && verdict->method &&
           (strcmp(verdict->method, "con istanze delle ipotesi e uguali al posto di uguali") == 0 ||
            strcmp(verdict->method, "con un esempio") == 0);
}

/* Scrive in out il certificato certificate alterato: una dimostrazione
 * senza le sue righe instance e witness; un modello con il tipo scambiato,
 * counterexample al posto di example o il contrario. Ritorna 0 se il testo
 * non ha la forma attesa o se l'alterazione non cambia nulla. */
static inline int alter_structure(const char *certificate, char *out, size_t size) {
    static const char *const kinds[][2] = {{"certificate counterexample\n", "certificate example\n"},
                                          {"certificate example\n", "certificate counterexample\n"}};
    for (int i = 0; i < 2; i++) {
        size_t length = strlen(kinds[i][0]);
        if (strncmp(certificate, kinds[i][0], length) != 0) continue;
        return snprintf(out, size, "%s%s", kinds[i][1], certificate + length) < (int)size;
    }
    if (strncmp(certificate, "certificate structure\n", 22) != 0) return 0;

    /* Riga per riga, saltando le voci della dimostrazione. */
    size_t used = 0, dropped = 0;
    for (const char *line = certificate; *line;) {
        const char *next = strchr(line, '\n');
        size_t length = next ? (size_t)(next - line) + 1 : strlen(line);
        if (strncmp(line, "instance ", 9) == 0 || strncmp(line, "witness ", 8) == 0) dropped++;
        else if (used + length < size) { memcpy(out + used, line, length); used += length; }
        else return 0;
        line += length;
    }
    out[used] = 0;
    return dropped > 0;
}

/* Controllo 5 sul file path: ogni verdetto del capitolo 28 ha un certificato
 * accettato, e la sua alterazione no. `expected` è quanti verdetti del
 * capitolo 28 ci si aspetta almeno, perché il controllo non passi a vuoto.
 *
 * Una dimostrazione da cui si tolgono le istanze e i nomi non deve essere
 * più accettata: se lo fosse, il verificatore avrebbe deciso da solo, e il
 * controllo non direbbe nulla delle voci che il motore ha scritto. */
static inline int check_structure_certificates(const char *path, int expected) {
    static char altered[2 * CERTIFICATE_BYTES];
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }
    const char *position = text, *item, *end;
    int number = 0, verdicts = 0, certified = 0, wrong = 0, wrong_accepted = 0;
    while (next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        number++;
        if (!from_chapter_28(&verdict)) continue;
        verdicts++;
        const char *why = "", *reread = verdict.certificate;
        if (!reread || ck_check(&reread, &why) != CK_ACCEPTED) {
            printf("  statement %d has no certificate the checker accepts (%s): ", number, why);
            verdict_print(stdout, &verdict);
            putchar('\n');
            continue;
        }
        certified++;
        if (!alter_structure(verdict.certificate, altered, sizeof altered)) {
            printf("  statement %d: cannot build the altered certificate\n", number);
            wrong_accepted++;
            continue;
        }
        const char *cursor = altered;
        wrong++;
        if (ck_check(&cursor, &why) != CK_ACCEPTED) continue;
        wrong_accepted++;
        printf("  statement %d: the altered certificate is accepted:\n%s", number, altered);
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d verdicts of chapter 28 carry a certificate the checker accepts, in %s", certified, verdicts, path);
    int passed = check_result(verdicts >= expected && certified == verdicts, summary);
    snprintf(summary, sizeof summary, "%d of %d altered certificates not accepted for %s", wrong - wrong_accepted, wrong, path);
    return check_result(wrong == certified && wrong_accepted == 0, summary) && passed;
}
