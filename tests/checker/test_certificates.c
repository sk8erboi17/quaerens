/* Test dei certificati che il motore scrive: motore e verificatore insieme.
 *
 * Compilazione ed esecuzione, dalla cartella principale del progetto:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_certificates tests/checker/test_certificates.c && /tmp/test_certificates
 *
 * Il verificatore da solo, con certificati scritti a mano, è in
 * test_checker.c. Qui si controlla il passaggio di mano:
 *
 *   1. ogni dimostrazione per induzione dei file di somme e prodotti porta
 *      un certificato, e quel testo, riletto, è accettato dal verificatore;
 *   2. antiregressione: da ogni certificato vero se ne ricavano tre
 *      sbagliati, e nessuno deve essere accettato:
 *
 *          L = R + 1          un enunciato diverso, falso già nel caso base;
 *          L = R + n - n0     un enunciato diverso, vero solo nel caso base;
 *          fattore + 1        l'enunciato giusto con il passo alterato;
 *
 *   3. una dimostrazione senza certificato dice perché manca;
 *   4. i certificati scritti su file sono quelli verificati, uno per uno;
 *   5. ogni certificato dei due file dice anche che l'enunciato è definito
 *      per ogni n, perché lì il verificatore lo sa ricontrollare;
 *   6. le tabelle dei segni (regola 18p-15 di main.c): nei file delle
 *      equazioni e delle disequazioni ogni verdetto della tabella ha il
 *      certificato, almeno quanti se ne contano oggi, oppure dice perché
 *      manca; ogni certificato riletto è accettato, e da ognuno se ne
 *      ricavano tre sbagliati, nessuno dei quali deve essere accettato:
 *
 *          l'ultima riga con la verità rovesciata;
 *          l'affermazione rovesciata, o le soluzioni "nessuna" ("tutte");
 *          il primo punto tolto, con l'intervallo che lo segue;
 *
 *   7. le derivate, le primitive e gli integrali definiti (27m e 30cert di
 *      main.c): nei file delle derivate e degli integrali ogni verdetto
 *      DIMOSTRATO o RISOLTO ha il certificato, almeno quanti se ne contano
 *      oggi, oppure dice perché manca; ogni certificato riletto è accettato,
 *      e nessuna delle sue alterazioni lo è:
 *
 *          la derivata affermata, o la funzione integranda, più 1;
 *          la funzione, o la primitiva, più e^x;
 *          il valore di un integrale più 1;
 *          l'ordine di una derivata più 1.
 *
 *      Il numero di alterazioni respinte, e non solo non controllabili, si
 *      riferisce; e alcune righe di verdetto sono fissate.
 *
 *   8. i limiti (29n di main.c): nei file di tests/limits ogni verdetto del
 *      capitolo 29 ha il certificato, almeno quanti se ne contano oggi,
 *      oppure dice perché manca; ogni certificato riletto è accettato, e da
 *      ognuno se ne ricavano tre che affermano il falso, nessuno dei quali
 *      deve essere accettato:
 *
 *          la verità di ciò che si afferma rovesciata;
 *          un'affermazione vera cambiata (il valore più 1, l'infinito con
 *          l'altro segno, il valore 0 per un limite infinito o che non
 *          esiste);
 *          la sostituzione del primo lato scambiata con quella dell'altro.
 *
 * I certificati sbagliati non si tolgono e non si indeboliscono per far
 * passare una modifica: se uno viene accettato, si corregge il verificatore.
 */

#include <stdio.h>
#include <string.h>
#include "../statement_checks.h"

/* Ritorna la fine dell'espressione che comincia in from, nella notazione dei
 * certificati: una parola, oppure una parentesi con tutto ciò che contiene. */
static const char *expression_end(const char *from) {
    int depth = 0;
    do {
        if (*from == '(') depth++;
        if (*from == ')') depth--;
        from++;
    } while (depth > 0 || (*from && *from != ' ' && *from != ')' && *from != '\n'));
    return from;
}

/* Scrive in out (di size byte) il certificato certificate alterato:
 *
 *   kind 0: al secondo membro dell'enunciato si aggiunge 1;
 *   kind 1: al secondo membro si aggiunge variable - base;
 *   kind 2: al fattore si aggiunge 1.
 *
 * Ritorna 0 se il testo non ha la forma attesa. */
static int alter(const char *certificate, int kind, int variable, long long base, char *out, size_t size) {
    const char *marker = kind == 2 ? "\nfactor " : "\nstatement (= ";
    const char *from = strstr(certificate, marker);
    if (!from) return 0;
    from += strlen(marker);
    if (kind != 2) from = expression_end(from) + 1;
    const char *to = expression_end(from);

    /* Ciò che precede, l'espressione con l'aggiunta, ciò che segue. */
    char added[64];
    if (kind == 1) snprintf(added, sizeof added, "(- %c %lld)", variable, base);
    else strcpy(added, "1");
    int written = snprintf(out, size, "%.*s(+ %.*s %s)%s", (int)(from - certificate), certificate, (int)(to - from), from, added, to);
    return written > 0 && (size_t)written < size;
}

/* Controlli 1 e 2 sul file path. */
static int check_certificates(const char *path) {
    static char altered[2 * CERTIFICATE_BYTES];
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }

    const char *position = text, *item, *end;
    int total = 0, certified = 0, defined = 0, wrong = 0, wrong_accepted = 0;
    while (next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        total++;
        if (verdict.outcome != PROVED || !verdict.certificate) {
            printf("  statement %d has no certificate: ", total);
            verdict_print(stdout, &verdict);
            putchar('\n');
            continue;
        }

        /* 1. Il testo consegnato è quello che il verificatore accetta. */
        const char *why, *reread = verdict.certificate;
        if (ck_check(&reread, &why) != CK_ACCEPTED) {
            printf("  statement %d: its certificate is not accepted when read again (%s)\n", total, why);
            continue;
        }
        certified++;
        if (strstr(verdict.certificate, "\ndefined")) defined++;

        /* 2. Le tre alterazioni. */
        for (int kind = 0; kind < 3; kind++) {
            if (!alter(verdict.certificate, kind, verdict.variable, verdict.base, altered, sizeof altered)) {
                printf("  statement %d: cannot build altered certificate %d\n", total, kind);
                wrong_accepted++;
                continue;
            }
            const char *cursor = altered;
            wrong++;
            if (ck_check(&cursor, &why) != CK_ACCEPTED) continue;
            wrong_accepted++;
            printf("  statement %d: altered certificate %d is accepted:\n%s", total, kind, altered);
        }
    }
    free(text);

    snprintf(summary, sizeof summary, "%d of %d proofs carry a certificate the checker accepts, in %s", certified, total, path);
    int passed = check_result(total > 0 && certified == total, summary);
    snprintf(summary, sizeof summary, "%d of %d certificates also say the statement is defined for every n, in %s", defined, total, path);
    passed &= check_result(defined == total, summary);
    snprintf(summary, sizeof summary, "%d of %d altered certificates not accepted for %s", wrong - wrong_accepted, wrong, path);
    return check_result(wrong == 3 * certified && wrong_accepted == 0, summary) && passed;
}

/* Controllo 3 sul file path: ogni enunciato dimostrato ha il certificato,
 * oppure il motivo per cui manca. `expected` è il numero minimo di
 * certificati attesi: il file contiene anche enunciati che la notazione dei
 * certificati sa dire. */
static int check_missing_explained(const char *path, int expected) {
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }

    const char *position = text, *item, *end;
    int proved = 0, certified = 0, silent = 0;
    while (next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        if (verdict.outcome != PROVED) continue;
        proved++;
        if (verdict.certificate) certified++;
        else if (!verdict.reason || !verdict.reason[0]) silent++;
    }
    free(text);

    snprintf(summary, sizeof summary, "%d proofs in %s: %d with a certificate, the other %d say why it is missing",
             proved, path, certified, proved - certified - silent);
    return check_result(proved > 0 && silent == 0 && certified >= expected, summary);
}

/* Controllo 4 sul file path: i certificati scritti da prove_file() sono
 * tanti quanti il riepilogo ne conta, e riletti dal file sono tutti
 * accettati. */
static int check_written(const char *path) {
    static char written[1 << 20];
    char summary[512];
    Report report;
    FILE *memory = tmpfile();
    if (!memory || !prove_file(path, &report, NULL, NULL, memory)) {
        snprintf(summary, sizeof summary, "cannot prove %s into a temporary file", path);
        return check_result(0, summary);
    }
    rewind(memory);
    size_t length = fread(written, 1, sizeof written - 1, memory);
    fclose(memory);
    written[length] = 0;

    /* Un certificato dopo l'altro, come fa il programma checker. */
    int accepted = 0, others = 0;
    for (const char *cursor = written; (cursor = strstr(cursor, "certificate")) != NULL;) {
        const char *why;
        if (ck_check(&cursor, &why) == CK_ACCEPTED) accepted++; else others++;
    }
    snprintf(summary, sizeof summary, "%d certificates written for %s, %d accepted when read back, %d counted by the report",
             accepted + others, path, accepted, report.certified);
    return check_result(accepted > 0 && others == 0 && accepted == report.certified, summary);
}

/* Il file path di enunciati sugli insiemi: ognuno è dimostrato, almeno
 * `expected` hanno un certificato, e ogni certificato riletto è accettato. */
static int check_sets(const char *path, int expected) {
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }
    const char *position = text, *item, *end;
    int total = 0, certified = 0, broken = 0;
    while (next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        const char *why, *reread = verdict.certificate;
        total++;
        if (verdict.outcome != PROVED) broken++;
        else if (reread && ck_check(&reread, &why) == CK_ACCEPTED) certified++;
        else if (reread) broken++;
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d statements about sets carry a certificate the checker accepts, in %s", certified, total, path);
    return check_result(total > 0 && broken == 0 && certified >= expected, summary);
}

/* Scrive in out (di size byte) il certificato di una tabella dei segni
 * alterato:
 *
 *   kind 0: l'ultima riga con la verità rovesciata;
 *   kind 1: l'affermazione rovesciata (forall true diventa forall false, e
 *           così via), oppure le soluzioni none al posto di quelle scritte, o
 *           all al posto di none;
 *   kind 2: tolti il primo punto della tabella e l'intervallo che lo segue.
 *
 * Ritorna 0 se il testo non ha la forma attesa, o se per kind 2 la tabella
 * non ha punti. */
static int alter_signs(const char *certificate, int kind, char *out, size_t size) {
    const char *from, *to;
    const char *replacement = "";
    if (kind == 0) {
        /* La parola prima di "\nend\n". */
        to = strstr(certificate, "\nend\n");
        if (!to) return 0;
        for (from = to; from > certificate && from[-1] != ' '; from--) {}
        replacement = strncmp(from, "true", 4) == 0 ? "false" : "true";
    } else if (kind == 1) {
        static const char *const claims[][2] = {{"claim forall true\n", "claim forall false\n"}, {"claim forall false\n", "claim forall true\n"},
                                                {"claim exists true\n", "claim exists false\n"}, {"claim exists false\n", "claim exists true\n"}};
        from = NULL;
        for (int i = 0; i < 4 && !from; i++)
            if ((from = strstr(certificate, claims[i][0])) != NULL) { to = from + strlen(claims[i][0]); replacement = claims[i][1]; }
        if (!from) {
            /* Le soluzioni: tutta la riga dopo "solution ". */
            if (!(from = strstr(certificate, "\nsolution "))) return 0;
            from += 10;
            to = strchr(from, '\n');
            replacement = strncmp(from, "none\n", 5) == 0 ? "all" : "none";
        }
    } else {
        /* La riga del primo punto e quella dopo. */
        const char *point = strstr(certificate, "\npoint "), *root = strstr(certificate, "\nroot ");
        from = !point ? root : !root ? point : point < root ? point : root;
        if (!from || !(to = strchr(from + 1, '\n')) || !(to = strchr(to + 1, '\n'))) return 0;
    }
    int written = snprintf(out, size, "%.*s%s%s", (int)(from - certificate), certificate, replacement, to);
    return written > 0 && (size_t)written < size;
}

/* Controllo 6 sul file path: ogni verdetto della tabella dei segni ha un
 * certificato che, riletto, è accettato, oppure il motivo per cui manca;
 * i certificati sono almeno `expected`, e nessuna delle loro alterazioni
 * (alter_signs()) è accettata. */
static int check_sign_tables(const char *path, int expected) {
    static char altered[2 * CERTIFICATE_BYTES];
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }

    const char *position = text, *item, *end;
    int verdicts = 0, certified = 0, silent = 0, wrong = 0, wrong_accepted = 0;
    while (next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        if (!verdict.method || strcmp(verdict.method, "con la tabella dei segni") != 0) continue;
        verdicts++;
        if (!verdict.certificate) {
            if (!verdict.reason || !verdict.reason[0]) silent++;
            continue;
        }

        /* Il testo consegnato è quello che il verificatore accetta. */
        const char *why, *reread = verdict.certificate;
        if (ck_check(&reread, &why) != CK_ACCEPTED) {
            printf("  statement %d: its certificate is not accepted when read again (%s)\n", verdicts, why);
            silent++;
            continue;
        }
        certified++;

        /* Le tre alterazioni. Una tabella senza punti non ha la terza. */
        for (int kind = 0; kind < 3; kind++) {
            if (!alter_signs(verdict.certificate, kind, altered, sizeof altered)) {
                if (kind == 2 && !strstr(verdict.certificate, "\npoint ") && !strstr(verdict.certificate, "\nroot ")) continue;
                printf("  statement %d: cannot build altered certificate %d\n", verdicts, kind);
                wrong_accepted++;
                continue;
            }
            const char *cursor = altered;
            wrong++;
            if (ck_check(&cursor, &why) != CK_ACCEPTED) continue;
            wrong_accepted++;
            printf("  statement %d: altered certificate %d is accepted:\n%s", verdicts, kind, altered);
        }
    }
    free(text);

    snprintf(summary, sizeof summary, "%d of %d sign table verdicts carry a certificate the checker accepts (at least %d expected), the others say why not, in %s",
             certified, verdicts, expected, path);
    int passed = check_result(verdicts > 0 && silent == 0 && certified >= expected, summary);
    snprintf(summary, sizeof summary, "%d of %d altered sign table certificates not accepted for %s", wrong - wrong_accepted, wrong, path);
    return check_result(wrong_accepted == 0, summary) && passed;
}

/* Scrive in out (di size byte) il certificato di una derivata, di una
 * primitiva o di un integrale (27m e 30cert di main.c) alterato:
 *
 *   kind 0: alla derivata affermata, o alla funzione integranda, si aggiunge 1;
 *   kind 1: alla funzione, o alla primitiva, si aggiunge e^x, che cambia ogni
 *           sua derivata;
 *   kind 2: al valore di un integrale si aggiunge 1;
 *   kind 3: l'ordine di una derivata di ordine fisso sale di uno.
 *
 * Ritorna 0 se il certificato non ha la voce da alterare. */
static int alter_calculus(const char *certificate, int kind, char *out, size_t size) {
    static const char *const markers[][2] = {{"\nderivative ", "\nintegrand "}, {"\nfunction ", "\nprimitive "}, {"\nvalue ", "\nvalue "}, {"\norder ", "\norder "}};
    const char *variable = strstr(certificate, "\nvariable "), *from = strstr(certificate, markers[kind][0]);
    if (!from) from = strstr(certificate, markers[kind][1]);
    if (!variable || !from) return 0;
    from = strchr(from + 1, ' ') + 1;
    const char *to = expression_end(from);
    char added[64];
    int written;
    if (kind == 3) {
        long long order;
        if (sscanf(from, "%lld", &order) != 1 || to - from > 6 || !isdigit((unsigned char)*from)) return 0;
        written = snprintf(out, size, "%.*s%lld%s", (int)(from - certificate), certificate, order + 1, to);
    } else {
        if (kind == 1) snprintf(added, sizeof added, "(exp %c)", variable[10]);
        else strcpy(added, "1");
        written = snprintf(out, size, "%.*s(+ %.*s %s)%s", (int)(from - certificate), certificate, (int)(to - from), from, added, to);
    }
    return written > 0 && (size_t)written < size;
}

/* Controllo 7 sul file path: ogni verdetto DIMOSTRATO o RISOLTO dei capitoli
 * 27 e 30 ha un certificato che, riletto, è accettato, oppure il motivo per
 * cui manca; i certificati sono almeno `expected`, e nessuna delle loro
 * alterazioni (alter_calculus()) è accettata. Quante sono respinte, e non
 * solo non controllabili, si riferisce. */
static int check_calculus(const char *path, int expected) {
    static char altered[2 * CERTIFICATE_BYTES];
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }

    const char *position = text, *item, *end;
    int verdicts = 0, certified = 0, silent = 0, wrong = 0, rejected = 0, wrong_accepted = 0;
    while (next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        if (verdict.outcome != PROVED && verdict.outcome != SOLVED) continue;
        verdicts++;
        if (!verdict.certificate) {
            if (!verdict.reason || !verdict.reason[0]) {
                printf("  statement %d: no certificate, and no reason\n", verdicts);
                silent++;
            }
            continue;
        }
        if (strncmp(verdict.certificate, "certificate derivative", 22) != 0 && strncmp(verdict.certificate, "certificate antiderivative", 26) != 0 &&
            strncmp(verdict.certificate, "certificate integral", 20) != 0) {
            printf("  statement %d: a certificate of another kind\n", verdicts);
            silent++;
            continue;
        }

        /* Il testo consegnato è quello che il verificatore accetta. */
        const char *why, *reread = verdict.certificate;
        if (ck_check(&reread, &why) != CK_ACCEPTED) {
            printf("  statement %d: its certificate is not accepted when read again (%s)\n", verdicts, why);
            silent++;
            continue;
        }
        certified++;

        /* Le alterazioni che il certificato ha. */
        for (int kind = 0; kind < 4; kind++) {
            if (!alter_calculus(verdict.certificate, kind, altered, sizeof altered)) continue;
            const char *cursor = altered;
            wrong++;
            CkAnswer answer = ck_check(&cursor, &why);
            rejected += answer == CK_REJECTED;
            if (answer != CK_ACCEPTED) continue;
            wrong_accepted++;
            printf("  statement %d: altered certificate %d is accepted:\n%s", verdicts, kind, altered);
        }
    }
    free(text);

    snprintf(summary, sizeof summary, "%d of %d derivative and integral verdicts carry a certificate the checker accepts (at least %d expected), the others say why not, in %s",
             certified, verdicts, expected, path);
    int passed = check_result(verdicts > 0 && silent == 0 && certified >= expected, summary);
    snprintf(summary, sizeof summary, "%d of %d altered derivative and integral certificates not accepted (%d rejected, the others not controllable) for %s",
             wrong - wrong_accepted, wrong, rejected, path);
    return check_result(wrong_accepted == 0, summary) && passed;
}

/* Scrive in out (di size byte) il certificato di un limite alterato, in modo
 * che affermi il falso:
 *
 *   kind 0: la verità di ciò che si afferma rovesciata;
 *   kind 1: un'affermazione vera cambiata: value V diventa value V + 1, plus
 *           e minus si scambiano, unsigned diventa value 0, none diventa
 *           value 0 (un limite che non esiste non vale 0); per
 *           un'affermazione falsa, la sostituzione del primo lato scambiata;
 *   kind 2: la sostituzione del primo lato scambiata (a + t con a - t, 1/t
 *           con -1/t, e^(1/t) con -e^(1/t), e^(-1/t) con -e^(-1/t)), che non
 *           porta più quel lato del punto in t -> 0+.
 *
 * Ritorna 0 se il testo non ha la forma attesa. */
static int alter_limit(const char *certificate, int kind, char *out, size_t size) {
    static const char *const swaps[][2] = {{" a+t ", " a-t "}, {" a-t ", " a+t "}, {" -1/t ", " 1/t "}, {" 1/t ", " -1/t "},
                                           {" -e^(1/t) ", " e^(1/t) "}, {" e^(1/t) ", " -e^(1/t) "}, {" -e^(-1/t) ", " e^(-1/t) "}, {" e^(-1/t) ", " -e^(-1/t) "}};
    const char *claim = strstr(certificate, "\nclaim "), *side = strstr(certificate, "\nside "), *from = NULL, *to = NULL;
    const char *replacement = "";
    char value[CERTIFICATE_BYTES];
    if (!claim || !side) return 0;
    const char *line_end = strchr(claim + 1, '\n');
    int truth = line_end && line_end - claim > 5 && strncmp(line_end - 5, " true", 5) == 0;
    if (kind == 1 && truth) {
        /* Tutta la riga claim, riscritta. */
        from = claim + 7;
        to = line_end;
        if (strncmp(from, "value ", 6) == 0) {
            snprintf(value, sizeof value, "value (+ %.*s 1) true", (int)(to - 5 - (from + 6)), from + 6);
            replacement = value;
        } else {
            replacement = strncmp(from, "plus", 4) == 0 ? "minus true" : strncmp(from, "minus", 5) == 0 ? "plus true" : "value 0 true";
        }
    } else if (kind == 0) {
        from = line_end - (truth ? 4 : 5);
        to = line_end;
        replacement = truth ? "false" : "true";
    } else {
        const char *line = strchr(side + 1, '\n');
        for (size_t i = 0; i < sizeof swaps / sizeof *swaps && !from; i++) {
            const char *at = strstr(side, swaps[i][0]);
            if (at && line && at < line) { from = at; to = at + strlen(swaps[i][0]); replacement = swaps[i][1]; }
        }
        if (!from) return 0;
    }
    int written = snprintf(out, size, "%.*s%s%s", (int)(from - certificate), certificate, replacement, to);
    return written > 0 && (size_t)written < size;
}

/* Controllo 8 sul file path: ogni verdetto dei limiti (capitolo 29) ha un
 * certificato che, riletto, è accettato, oppure il motivo per cui manca; i
 * certificati sono almeno `expected`, e nessuna delle loro alterazioni
 * (alter_limit()) è accettata. */
static int check_limits(const char *path, int expected) {
    static char altered[2 * CERTIFICATE_BYTES];
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }

    const char *position = text, *item, *end;
    int verdicts = 0, certified = 0, silent = 0, wrong = 0, wrong_accepted = 0;
    while (next_item(&position, &item, &end)) {
        /* Un verdetto del capitolo 29: dimostrato, falso o risolto. */
        Verdict verdict = prove_item(item, end);
        if (verdict.outcome != PROVED && verdict.outcome != REFUTED && verdict.outcome != SOLVED) continue;
        verdicts++;
        if (!verdict.certificate) {
            if (!verdict.reason || !verdict.reason[0]) silent++;
            continue;
        }

        /* Il testo consegnato è quello che il verificatore accetta. */
        const char *why, *reread = verdict.certificate;
        if (ck_check(&reread, &why) != CK_ACCEPTED) {
            printf("  statement %d: its certificate is not accepted when read again (%s)\n", verdicts, why);
            silent++;
            continue;
        }
        certified++;

        /* Le tre alterazioni. */
        for (int kind = 0; kind < 3; kind++) {
            if (!alter_limit(verdict.certificate, kind, altered, sizeof altered)) {
                printf("  statement %d: cannot build altered certificate %d\n", verdicts, kind);
                wrong_accepted++;
                continue;
            }
            const char *cursor = altered;
            wrong++;
            if (ck_check(&cursor, &why) != CK_ACCEPTED) continue;
            wrong_accepted++;
            printf("  statement %d: altered certificate %d is accepted:\n%s", verdicts, kind, altered);
        }
    }
    free(text);

    snprintf(summary, sizeof summary, "%d of %d limit verdicts carry a certificate the checker accepts (at least %d expected), the others say why not, in %s",
             certified, verdicts, expected, path);
    int passed = check_result(verdicts > 0 && silent == 0 && certified >= expected, summary);
    snprintf(summary, sizeof summary, "%d of %d altered limit certificates not accepted for %s", wrong - wrong_accepted, wrong, path);
    return check_result(wrong_accepted == 0, summary) && passed;
}

int main(void) {
    engine_init();
    int passed = 1;
    passed &= check_certificates("tests/sums_products_inequalities/simple_inductions.tex");
    passed &= check_certificates("tests/sums_products_inequalities/finite_sums_and_products.tex");
    passed &= check_missing_explained("tests/logs_trig_powers_roots/logs_trig_powers_roots_inductions.tex", 23);
    passed &= check_written("tests/sums_products_inequalities/simple_inductions.tex");

    /* Il testo di un certificato, fissato: è ciò che un lettore confronta
     * con l'enunciato, e ciò che il programma checker deve saper rileggere. */
    Verdict gauss = prove("\\sum_{k=1}^{n} k = \\frac{n\\left(n + 1\\right)}{2}", 'n', 0, 0);
    const char *expected = "certificate induction\nvariable n\nbase 1\n"
                           "statement (= (sum k 1 n k) (/ (* n (+ n 1)) 2))\nfactor 1\ndefined\nend\n";
    passed &= check_result(gauss.certificate && strcmp(gauss.certificate, expected) == 0,
                           "the certificate of the sum of the first n integers has the expected text");

    /* Un prodotto: il fattore non è 1, e le successioni usate sono definite
     * nel certificato. */
    Verdict product = prove("\\prod_{k=1}^{n} \\frac{k + 1}{k} = n + 1", 'n', 0, 0);
    passed &= check_result(product.certificate && strstr(product.certificate, "\nfactor (/ ") != NULL,
                           "the certificate of a telescoping product has a rational factor");
    Verdict fibonacci = prove("\\sum_{k=1}^{n} F_{k} = F_{n + 2} - 1", 'n', 0, 0);
    passed &= check_result(fibonacci.certificate && strstr(fibonacci.certificate, "\nsequence F 2 1 1 0 1\n") != NULL,
                           "the certificate of a Fibonacci sum defines the sequence it uses");
    /* La voce "defined" porta il punto in cui il motore ha stabilito che
     * l'enunciato è definito per ogni n; dove il verificatore non sa
     * ricontrollarlo resta il certificato della sola uguaglianza. */
    Verdict geometric = prove("\\sum_{k=0}^{n} x^{k} = \\frac{x^{n + 1} - 1}{x - 1}", 'n', 0, 0);
    passed &= check_result(geometric.certificate && strstr(geometric.certificate, "\ndefined x ") != NULL,
                           "the certificate of a sum with a parameter says where it is defined for every n");
    Verdict two_powers = prove("\\sum_{k=1}^{n} \\frac{1}{3^{k} - 2^{k}} = \\sum_{k=1}^{n} \\frac{1}{3^{k} - 2^{k}}", 'n', 0, 0);
    passed &= check_result(two_powers.outcome == PROVED && two_powers.certificate && strstr(two_powers.certificate, "\ndefined") == NULL,
                           "where the checker cannot recheck that a divisor never vanishes, the certificate covers the equality only");

    /* Un logaritmo: il certificato porta i fattori in cui il motore ha
     * scomposto gli argomenti. */
    Verdict logarithm = prove("\\sum_{k=1}^{n} \\ln\\left(1 + \\frac{1}{k}\\right) = \\ln\\left(n + 1\\right)", 'n', 0, 0);
    passed &= check_result(logarithm.certificate && strstr(logarithm.certificate, "(ln ") && strstr(logarithm.certificate, "\nbrick "),
                           "the certificate of a sum of logarithms suggests the factors of their arguments");

    Verdict harmonic = prove("\\sum_{k=1}^{n} H_{k} = \\left(n + 1\\right) H_{n} - n", 'n', 0, 0);
    passed &= check_result(harmonic.certificate && strstr(harmonic.certificate, "\ndefine H m (sum j 1 m (/ 1 j))\n") != NULL,
                           "the certificate of a sum of harmonic numbers defines them by their formula");

    /* Gli altri metodi non hanno un certificato, e il verdetto lo dice. */
    /* Insiemi: il certificato è l'enunciato, che il verificatore decide da
     * capo. "Esiste un insieme" non è nella sua notazione. */
    Verdict sets = prove("(S \\setminus T) \\setminus V = S \\setminus (T \\cup V)", 'n', 0, 0);
    passed &= check_result(sets.outcome == PROVED && sets.certificate &&
                           strcmp(sets.certificate, "certificate sets\nstatement (= (diff (diff S T) V) (diff S (union T V)))\nend\n") == 0,
                           "the certificate of a statement about sets is the statement, in the checker's notation");
    Verdict chosen = prove("\\exists V \\left( S \\cup V = S \\right)", 'n', 0, 0);
    passed &= check_result(chosen.outcome == PROVED && !chosen.certificate, "a statement with \"there is a set\" has no certificate");
    passed &= check_sets("tests/set_theory/set_operations.tex", 23);
    passed &= check_sets("tests/set_theory/cartesian_products.tex", 8);
    Verdict quantified = prove("\\forall n \\in \\mathbb{N}\\ \\left(\\sum_{k=1}^{n} k = \\frac{n\\left(n + 1\\right)}{2}\\right)", 'n', 0, 0);
    passed &= check_result(quantified.outcome == PROVED && !quantified.certificate, "a proof with quantifiers has no certificate of its own");

    /* 6. Le tabelle dei segni. I file con le radici quadrate nelle
     * condizioni non hanno certificati, ma ogni verdetto dice perché. */
    passed &= check_sign_tables("tests/equations_inequalities/book_inequalities.tex", 62);
    passed &= check_sign_tables("tests/equations_inequalities/false_inequalities.tex", 45);
    passed &= check_sign_tables("tests/equations_inequalities/sign_table.tex", 30);
    passed &= check_sign_tables("tests/equations_inequalities/solution_sets.tex", 22);
    passed &= check_sign_tables("tests/equations_inequalities/book_irrational_equations.tex", 0);

    /* Il testo di un certificato della tabella, fissato, e la riga del
     * verdetto: per un insieme di soluzioni il certificato si dice prima
     * dei due punti, dopo i quali vengono le soluzioni. */
    Verdict quotient = prove("\\{x \\in \\mathbb{R} \\mid \\frac{x - 2}{x + 3} \\ge 0\\}", 'n', 0, 0);
    const char *table = "certificate signs\nvariable x\ndomain reals\ncondition (<= 0 (/ (- x 2) (+ x 3)))\nclaim solve\n"
                        "solution (or (< x (neg 3)) (<= 2 x))\nfactor (+ x 3)\nfactor (+ x -2)\n"
                        "interval -5 true\npoint -3 false\ninterval -2 false\npoint 2 true\ninterval 4 true\nend\n";
    passed &= check_result(quotient.outcome == SOLVED && quotient.certificate && strcmp(quotient.certificate, table) == 0,
                           "the certificate of the solutions of (x - 2)/(x + 3) >= 0 has the expected text");
    static const char *const lines[][2] = {
        {"\\{x \\in \\mathbb{R} \\mid \\frac{x - 2}{x + 3} \\ge 0\\}", "RISOLTO con la tabella dei segni, certificato verificato: x < -3 \\lor x \\ge 2"},
        {"\\forall x \\in \\mathbb{R}\\ (x^{2} - 2 \\neq 0)", "FALSO: controesempio x = -\\sqrt{2}; certificato verificato"},
        {"\\exists x \\in \\mathbb{R}\\ (x^{3} - 2x < 0)", "DIMOSTRATO con la tabella dei segni; certificato verificato"},
        {"\\forall x \\in \\mathbb{R}\\ (\\sqrt{x^{2}} \\ge 0)",
         "DIMOSTRATO con la tabella dei segni; senza certificato (la notazione dei certificati della tabella dei segni ha solo frazioni di "
         "polinomi e valori assoluti, non radici, esponenziali, logaritmi o funzioni goniometriche)"},
    };
    for (size_t i = 0; i < sizeof lines / sizeof *lines; i++) {
        char line[512] = "";
        Verdict verdict = prove(lines[i][0], 'n', 0, 0);
        FILE *memory = tmpfile();
        if (memory) {
            verdict_print(memory, &verdict);
            rewind(memory);
            line[fread(line, 1, sizeof line - 1, memory)] = 0;
            fclose(memory);
        }
        passed &= check_result(strcmp(line, lines[i][1]) == 0, lines[i][1]);
    }

    /* 7. Le derivate, le primitive e gli integrali definiti. */
    passed &= check_calculus("tests/derivatives/book_derivatives.tex", 44);
    passed &= check_calculus("tests/derivatives/nth_derivatives.tex", 44);
    passed &= check_calculus("tests/derivatives/derivative_sign_reading.tex", 9);
    passed &= check_calculus("tests/derivatives/derivative_requests.tex", 11);
    passed &= check_calculus("tests/integrals/book_integrals.tex", 59);
    passed &= check_calculus("tests/integrals/integral_conventions.tex", 16);
    passed &= check_calculus("tests/integrals/integral_requests.tex", 55);

    /* Il testo di un certificato di derivata, fissato, e le righe dei
     * verdetti: il certificato c'è, oppure manca e si dice perché. */
    Verdict derivative = prove("\\frac{d}{dx}\\left( x^{2} e^{x} \\right) = e^{x}\\left(x^{2} + 2x\\right)", 'n', 0, 0);
    passed &= check_result(derivative.certificate &&
                           strcmp(derivative.certificate, "certificate derivative\nvariable x\norder 1\nfunction (* (^ x 2) (exp x))\n"
                                                          "derivative (* (exp x) (+ (^ x 2) (* 2 x)))\nend\n") == 0,
                           "the certificate of the derivative of x^2 e^x has the expected text");
    static const char *const calculus[][2] = {
        {"\\frac{d}{dx}\\left( \\ln x \\right) = \\frac{1}{x}", "DIMOSTRATO con le regole di derivazione; certificato verificato"},
        {"\\frac{d^{n}}{dx^{n}} e^{-2x} = (-1)^{n} 2^{n} e^{-2x}", "DIMOSTRATO per induzione su n >= 0; certificato verificato: uguali dove sono definiti"},
        {"\\frac{d}{dx}\\left( \\sqrt{x} \\right)",
         "RISOLTO con le regole di derivazione, certificato verificato: \\frac{\\sqrt{x}}{2x} (dove la funzione e la derivata sono "
         "definite; il certificato non copre i punti in cui la base di una radice o di una potenza con l'esponente non intero, o l'argomento "
         "di un valore assoluto, vale zero)"},
        {"\\frac{d}{dx}\\left( \\arcsin x \\right) = \\frac{1}{\\sqrt{1 - x^{2}}}",
         "DIMOSTRATO con le regole di derivazione; senza certificato (la notazione dei certificati delle derivate non ha arcoseno, arcocoseno e "
         "le successioni, o il testo è troppo lungo)"},
        {"\\int \\frac{1}{x} \\, dx = \\ln|x| + c",
         "DIMOSTRATO con le regole di derivazione, su ogni intervallo in cui primitiva e funzione integranda sono definite; certificato verificato"},
        {"\\int_{0}^{1} x^{2} \\, dx", "RISOLTO con il teorema fondamentale del calcolo, certificato verificato: \\frac{1}{3}"},
        {"\\int_{0}^{3} |x - 1| \\, dx",
         "RISOLTO con il teorema fondamentale del calcolo, senza certificato (il certificato di un integrale ha una primitiva sola, e qui un "
         "valore assoluto divide l'intervallo in parti): \\frac{5}{2}"},
    };
    for (size_t i = 0; i < sizeof calculus / sizeof *calculus; i++) {
        char line[768] = "";
        Verdict verdict = prove(calculus[i][0], 'n', 0, 0);
        FILE *memory = tmpfile();
        if (memory) {
            verdict_print(memory, &verdict);
            rewind(memory);
            line[fread(line, 1, sizeof line - 1, memory)] = 0;
            fclose(memory);
        }
        if (strcmp(line, calculus[i][1]) != 0) printf("  got: %s\n", line);
        passed &= check_result(strcmp(line, calculus[i][1]) == 0, calculus[i][1]);
    }

    /* 8. I limiti (29n di main.c). I numeri sono quelli di oggi: dei 593
     * verdetti dei cinque file ne restano senza certificato 6, che dicono
     * perché (un esponenziale di una funzione di L, che il motore tratta con
     * l'algebra dei limiti; un confronto tra radici di basi diverse). */
    passed &= check_limits("tests/limits/book_limits.tex", 155);
    passed &= check_limits("tests/limits/false_limits.tex", 252);
    passed &= check_limits("tests/limits/limit_requests.tex", 45);
    passed &= check_limits("tests/limits/growth_scale_limits.tex", 34);
    passed &= check_limits("tests/limits/false_growth_scale_limits.tex", 101);

    /* Il testo di un certificato di un limite, fissato, e le righe dei
     * verdetti. */
    Verdict cosine = prove("\\lim_{x \\to 0} \\frac{1 - \\cos x}{x^{2}} = \\frac{1}{2}", 'n', 0, 0);
    const char *limit = "certificate limit\nvariable x\npoint 0\nfunction (/ (- 1 (cos x)) (^ x 2))\nclaim value (/ 1 2) true\n"
                        "side right a+t 4\nside left a-t 4\nend\n";
    passed &= check_result(cosine.outcome == PROVED && cosine.certificate && strcmp(cosine.certificate, limit) == 0,
                           "the certificate of the limit of (1 - cos x)/x^2 has the expected text");
    static const char *const limit_lines[][2] = {
        {"\\lim_{x \\to 0} \\frac{1 - \\cos x}{x^{2}} = \\frac{1}{2}", "DIMOSTRATO con le serie troncate; certificato verificato"},
        {"\\lim_{x \\to 0} \\frac{1}{x} = +\\infty",
         "FALSO: è dimostrata la sua negazione (il limite non esiste: i limiti da destra e da sinistra sono diversi); certificato verificato"},
        {"\\lim_{x \\to +\\infty} \\frac{\\ln x}{\\ln \\ln x}", "RISOLTO con le serie troncate, certificato verificato: \\lim_{x \\to +\\infty} \\frac{\\ln x}{\\ln \\ln x} = +\\infty"},
        {"\\lim_{x \\to +\\infty} \\left(\\frac{1}{3x}\\right)^{\\frac{1}{\\ln(2x)}} = e^{-1}",
         "DIMOSTRATO con le serie troncate; senza certificato (il verificatore non sa controllarlo, l'esponenziale di una funzione di L che il "
         "verificatore non sa scrivere)"},
    };
    for (size_t i = 0; i < sizeof limit_lines / sizeof *limit_lines; i++) {
        char line[512] = "";
        Verdict verdict = prove(limit_lines[i][0], 'n', 0, 0);
        FILE *memory = tmpfile();
        if (memory) {
            verdict_print(memory, &verdict);
            rewind(memory);
            line[fread(line, 1, sizeof line - 1, memory)] = 0;
            fclose(memory);
        }
        passed &= check_result(strcmp(line, limit_lines[i][1]) == 0, limit_lines[i][1]);
    }

    passed &= check_result(unjustified_steps == 0, "no certificate was rejected and every proof step passed its independent check");
    return passed ? 0 : 1;
}
