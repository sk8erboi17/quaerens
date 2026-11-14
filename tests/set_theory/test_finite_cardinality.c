/* Test di dominio: teoria degli insiemi, il numero degli elementi di un
 * insieme confrontato con un numero scritto, |T| = 3 (17x di parser.c).
 *
 * I controlli, sui file di dati di questa cartella e su enunciati scritti
 * qui:
 *
 *   1. ogni enunciato di finite_cardinality.tex viene dimostrato, con la sua
 *      dimostrazione scritta;
 *   2. nessun enunciato di false_finite_cardinality.tex viene dimostrato;
 *   3. e almeno 44 di loro sono dichiarati falsi;
 *   4. i verdetti dei due file portano un certificato che il verificatore
 *      accetta, e alterato no (check_structure_certificates());
 *   5. "esattamente k": per k da 0 a 5, l'unico controesempio di |S| \neq k
 *      ha proprio k elementi, e l'esempio chiesto da \exists S (|S| = k)
 *      anche;
 *   6. ciò che 17x non legge dà NON SUPPORTATO con il suo motivo: un numero
 *      di elementi confrontato con una lettera, con l'infinito, con un altro
 *      numero di elementi, con un conto, o in un enunciato con un insieme di
 *      numeri;
 *   7. un numero scritto troppo grande per essere svolto dà NON DIMOSTRATO
 *      per la memoria di lavoro, mai un sì o un no, e l'enunciato dopo si
 *      decide come sempre;
 *   8. le definizioni: |S| = 0 \iff S = \emptyset e le altre tre scritte in
 *      check_definitions() sono dimostrate, con il certificato.
 *
 * I controlli 2 e 3 sono l'antiregressione: le varianti false sono
 * sbagliate di poco (un limite spostato di uno, un confronto scambiato,
 * un'ipotesi tolta), e uno svolgimento sbagliato di uno ne renderebbe vera
 * qualcuna. Il 5 controlla lo svolgimento dall'esterno, sui modelli che il
 * programma consegna. Gli enunciati sono dati: il motore non ne conosce
 * nessuno in anticipo.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_finite_cardinality \
 *         tests/set_theory/test_finite_cardinality.c && /tmp/test_finite_cardinality
 */

#include "../statement_checks.h"

/* Controllo 3: almeno at_least enunciati di path, un file di enunciati
 * falsi, vengono dichiarati FALSO, e nessuno dimostrato. Lo stesso controllo
 * di test_set_theory.c, ripetuto qui perché i test non dipendano l'uno
 * dall'altro. */
static int check_refuted_at_least(const char *path, int at_least) {
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
        if (verdict.outcome == REFUTED) refuted++;
        if (verdict.outcome == PROVED) proved++;
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d statements of %s are REFUTED (at least %d wanted), %d are PROVED (none allowed)",
             refuted, total, path, at_least, proved);
    return check_result(refuted >= at_least && proved == 0, summary);
}

/* Ritorna quante volte needle compare in text, fino a stop escluso (o fino
 * alla fine, se stop è NULL). */
static int occurrences(const char *text, const char *needle, const char *stop) {
    int count = 0;
    for (const char *at = strstr(text, needle); at && (!stop || at < stop); at = strstr(at + 1, needle)) count++;
    return count;
}

/* Controllo 5: per k da 0 a 5, |S| \neq k è falso con un controesempio di
 * proprio k elementi (gli elementi di S si chiamano s_{0}, s_{1}, ...), e
 * \exists S (|S| = k) è dimostrato con un esempio di proprio k elementi. */
static int check_exact_counts(void) {
    char text[64], summary[256];
    int passed = 1;
    for (int k = 0; k <= 5; k++) {
        snprintf(text, sizeof text, "|S| \\neq %d", k);
        Verdict verdict = prove(text, 'n', 0, 0);
        int elements = verdict.outcome == REFUTED && verdict.detail ? occurrences(verdict.detail, "s_{", NULL) : -1;
        if (elements != k) { printf("  %s: a counterexample of %d elements (expected %d)\n", text, elements, k); passed = 0; }

        snprintf(text, sizeof text, "\\exists S\\ (|S| = %d)", k);
        verdict = prove(text, 'n', 0, 0);
        const char *stop = verdict.proof ? strstr(verdict.proof, "Computing") : NULL;
        elements = verdict.outcome == PROVED && stop ? occurrences(verdict.proof, "s_{", stop) : -1;
        if (elements != k) { printf("  %s: an example of %d elements (expected %d)\n", text, elements, k); passed = 0; }
    }
    snprintf(summary, sizeof summary, "the counterexample of |S| \\neq k and the example of |S| = k have exactly k elements, for k from 0 to 5");
    return check_result(passed, summary);
}

/* Controllo 6: ognuno degli enunciati dà NON SUPPORTATO con il motivo di
 * 17x (count_reason). */
static int check_unread_counts(void) {
    static const char *const statements[] = {
        "|S| = n",
        "|S| = \\infty",
        "|S| < \\infty",
        "|S| = |T|",
        "|\\mathcal{P}(S)| = 2^{n}",
        "|S| = 2n",
        "|S| = 3 + 1",
        "|S| = 3 \\land |T| = m",
        "X \\subseteq S \\implies |X| \\le |S|",
        "n = |S| \\implies |S| \\ge 0",
        "\\forall n \\in \\mathbb{N}\\ (|S| = n \\implies |\\mathcal{P}(S)| = 2^{n})",
        "|\\{d \\in \\mathbb{N} \\mid d \\le 3\\}| = 3",
    };
    int passed = 1, count = (int)(sizeof statements / sizeof statements[0]);
    for (int i = 0; i < count; i++) {
        Verdict verdict = prove(statements[i], 'n', 0, 0);
        if (verdict.outcome == UNSUPPORTED && verdict.reason && strcmp(verdict.reason, count_reason) == 0) continue;
        printf("  %s: outcome %d, reason %s\n", statements[i], (int)verdict.outcome, verdict.reason ? verdict.reason : "none");
        passed = 0;
    }
    char summary[256];
    snprintf(summary, sizeof summary, "%d numbers of elements not compared with a written number are UNSUPPORTED, with the reason of 17x", count);
    return check_result(passed, summary);
}

/* Controllo 7: un numero che non si può svolgere nella memoria di lavoro dà
 * NON DIMOSTRATO con quel motivo, e l'enunciato dopo è dimostrato. */
static int check_large_counts(void) {
    static const char *const statements[] = {
        "|S| = 100000",
        "|S| \\ge 100000 \\implies |S| \\ge 2",
        "X \\subseteq S \\implies |X| \\le 3000000000",
        "|S| \\neq 2147483647",
    };
    int passed = 1, count = (int)(sizeof statements / sizeof statements[0]);
    for (int i = 0; i < count; i++) {
        Verdict verdict = prove(statements[i], 'n', 0, 0);
        if (verdict.outcome != NOT_PROVED || !verdict.reason || strcmp(verdict.reason, OUT_OF_MEMORY) != 0) {
            printf("  %s: outcome %d, reason %s\n", statements[i], (int)verdict.outcome, verdict.reason ? verdict.reason : "none");
            passed = 0;
        }
        Verdict after = prove("|S| = 0 \\iff S = \\emptyset", 'n', 0, 0);
        if (after.outcome != PROVED) { printf("  after %s the next statement is not proved\n", statements[i]); passed = 0; }
    }

    /* Un numero che non sta nemmeno in un long long non è un numero che il
     * lettore sappia leggere: NON SUPPORTATO. */
    Verdict huge = prove("|S| = 99999999999999999999", 'n', 0, 0);
    if (huge.outcome != UNSUPPORTED) { printf("  a number beyond long long: outcome %d\n", (int)huge.outcome); passed = 0; }
    return check_result(passed, "a written number too large to unfold is NOT PROVED for the work memory, and the next statement is proved");
}

/* Controllo 8: i quattro enunciati che dicono che cosa vuol dire avere
 * zero, al più uno e almeno due elementi sono dimostrati, con un certificato
 * accettato.
 *
 * Stanno qui e non in finite_cardinality.tex perché, svolti, hanno i due
 * membri uguali a meno dei nomi delle variabili: il verificatore li
 * riconosce da solo, anche senza le istanze scritte dal motore, e il
 * controllo 4 chiede invece che senza di quelle un certificato non passi. */
static int check_definitions(void) {
    static const char *const statements[] = {
        "|S| = 0 \\iff S = \\emptyset",
        "\\left| S \\right| = 0 \\iff S = \\emptyset",
        "|S| \\le 1 \\iff \\forall x, y \\in S\\ (x = y)",
        "|S| \\ge 2 \\iff \\exists x, y \\in S\\ (x \\neq y)",
    };
    int passed = 1, count = (int)(sizeof statements / sizeof statements[0]);
    for (int i = 0; i < count; i++) {
        Verdict verdict = prove(statements[i], 'n', 0, 0);
        const char *why = "", *reread = verdict.certificate;
        if (verdict.outcome == PROVED && reread && ck_check(&reread, &why, 0) == CK_ACCEPTED) continue;
        printf("  %s: outcome %d (%s)\n", statements[i], (int)verdict.outcome, why);
        passed = 0;
    }
    return check_result(passed, "what zero, at most one and at least two elements mean is proved, with an accepted certificate");
}

int main(void) {
    engine_init();

    /* Tutti i controlli vengono eseguiti anche se uno fallisce, così il
     * resoconto è completo. */
    int passed = check_all_proved("tests/set_theory/finite_cardinality.tex");
    passed = check_none_proved("tests/set_theory/false_finite_cardinality.tex") && passed;
    passed = check_refuted_at_least("tests/set_theory/false_finite_cardinality.tex", 44) && passed;
    passed = check_structure_certificates("tests/set_theory/finite_cardinality.tex", 55) && passed;
    passed = check_structure_certificates("tests/set_theory/false_finite_cardinality.tex", 44) && passed;
    passed = check_exact_counts() && passed;
    passed = check_unread_counts() && passed;
    passed = check_large_counts() && passed;
    passed = check_definitions() && passed;

    /* Ogni fatto scritto in una dimostrazione viene ricontrollato dal
     * programma con un metodo indipendente: nessuno deve essere stato
     * scartato. */
    passed = check_result(unjustified_steps == 0, "every step of the written proofs passed its independent check") && passed;
    return passed ? 0 : 1;
}
