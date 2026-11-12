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
 *   4. i certificati scritti su file sono quelli verificati, uno per uno.
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
    int total = 0, certified = 0, wrong = 0, wrong_accepted = 0;
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

int main(void) {
    engine_init();
    int passed = 1;
    passed &= check_certificates("tests/sums_products_inequalities/simple_inductions.tex");
    passed &= check_certificates("tests/sums_products_inequalities/finite_sums_and_products.tex");
    passed &= check_missing_explained("tests/logs_trig_powers_roots/logs_trig_powers_roots_inductions.tex", 4);
    passed &= check_written("tests/sums_products_inequalities/simple_inductions.tex");

    /* Il testo di un certificato, fissato: è ciò che un lettore confronta
     * con l'enunciato, e ciò che il programma checker deve saper rileggere. */
    Verdict gauss = prove("\\sum_{k=1}^{n} k = \\frac{n\\left(n + 1\\right)}{2}", 'n', 0, 0);
    const char *expected = "certificate induction\nvariable n\nbase 1\n"
                           "statement (= (sum k 1 n k) (/ (* n (+ n 1)) 2))\nfactor 1\nend\n";
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
    Verdict harmonic = prove("\\sum_{k=1}^{n} H_{k} = \\left(n + 1\\right) H_{n} - n", 'n', 0, 0);
    passed &= check_result(harmonic.certificate && strstr(harmonic.certificate, "\ndefine H m (sum j 1 m (/ 1 j))\n") != NULL,
                           "the certificate of a sum of harmonic numbers defines them by their formula");

    /* Gli altri metodi non hanno un certificato, e il verdetto lo dice. */
    Verdict sets = prove("S \\cup T = T \\cup S", 'n', 0, 0);
    passed &= check_result(sets.outcome == PROVED && !sets.certificate, "a proof about sets has no certificate");
    Verdict quantified = prove("\\forall n \\in \\mathbb{N}\\ \\left(\\sum_{k=1}^{n} k = \\frac{n\\left(n + 1\\right)}{2}\\right)", 'n', 0, 0);
    passed &= check_result(quantified.outcome == PROVED && !quantified.certificate, "a proof with quantifiers has no certificate of its own");

    passed &= check_result(unjustified_steps == 0, "no certificate was rejected and every proof step passed its independent check");
    return passed ? 0 : 1;
}
