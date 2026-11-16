/* Test di 19z: il controesempio di un enunciato con quantificatori sui
 * numeri come certificato numbers.
 *
 * Quando il motore smentisce un "per ogni" trovando dei numeri, scrive
 * l'enunciato, appena letto, nella notazione dei certificati numbers e quei
 * numeri come testimoni; il verdetto dice "certificato verificato" solo se
 * il verificatore lo accetta. Qui:
 *
 *   - controesempi tra interi e tra razionali, con uno o due numeri, e uno
 *     con un "esiste" più interno su un solo intero, che il conto completo
 *     del verificatore (20e) stabilisce: tutti con il certificato, che il
 *     verificatore accetta da capo;
 *   - lo stesso certificato con un testimone cambiato in un numero che non
 *     smentisce l'enunciato: respinto;
 *   - un controesempio con un "esiste" più interno sui razionali, che
 *     nessun numero stabilisce ("per ogni x razionale esiste y con y^2 = x",
 *     smentito in x = -1): senza certificato, ed è giusto;
 *   - un enunciato vero non diventa falso: resta dimostrato.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_counterexample_certificates \
 *         tests/engine/test_counterexample_certificates.c && /tmp/test_counterexample_certificates
 */

#define main mathresearcher_main
#include "../../main.c"
#undef main

static int failures;

static void expect(int condition, const char *what) {
    if (!condition) { printf("FALLITO: %s\n", what); failures++; }
}

static Verdict decide(const char *item) { return prove_item(item, item + strlen(item)); }

/* Il certificato del verdetto, accettato da capo dal verificatore? */
static int accepted(const char *certificate) {
    const char *cursor = certificate, *why = "";
    return ck_check(&cursor, &why, 0) == CK_ACCEPTED;
}

int main(void) {
    engine_init();
    static const char *const certified[] = {
        "$\\displaystyle \\forall a, b \\in \\mathbb{Z}\\ (ab = 0 \\implies a = 0)$",
        "$\\displaystyle \\forall a \\in \\mathbb{Z}\\ (a^{2} \\neq 4)$",
        "$\\displaystyle \\forall n \\in \\mathbb{N}\\ \\exists m \\in \\mathbb{N}\\ (m + 1 = n)$",
        "$\\displaystyle \\forall a, b \\in \\mathbb{Q}\\ (a + b = ab \\implies a = 0)$",
    };
    for (size_t i = 0; i < sizeof certified / sizeof *certified; i++) {
        Verdict verdict = decide(certified[i]);
        char what[256];
        snprintf(what, sizeof what, "controesempio certificato: %s", certified[i]);
        expect(verdict.outcome == REFUTED && verdict.detail && verdict.certificate, what);
        if (verdict.certificate) {
            expect(strstr(verdict.certificate, "certificate numbers\n") == verdict.certificate && strstr(verdict.certificate, "\nfails\n") &&
                   strstr(verdict.certificate, "\nwitness "), "il certificato è numbers, con i testimoni");
            expect(accepted(verdict.certificate), "il verificatore accetta il certificato da capo");
        }
    }

    /* Un testimone cambiato: a = 1, b = 0 smentisce "ab = 0 implica a = 0",
     * a = 0, b = 0 no. */
    Verdict verdict = decide(certified[0]);
    if (verdict.certificate) {
        char altered[4096];
        snprintf(altered, sizeof altered, "%s", verdict.certificate);
        char *witness = strstr(altered, "witness 0 1\n");
        expect(witness != NULL, "il testimone di a è 1");
        if (witness) {
            witness[10] = '0';
            expect(!accepted(altered), "con a = 0 il certificato è respinto");
        }
    }

    /* Un "esiste" sui razionali più interno, che nessun numero stabilisce. */
    verdict = decide("$\\displaystyle \\forall x \\in \\mathbb{Q}\\ \\exists y \\in \\mathbb{Q}\\ (y^{2} = x)$");
    expect(verdict.outcome == REFUTED && !verdict.certificate, "un esiste sui razionali che nessun numero stabilisce resta senza certificato");

    /* Un enunciato vero resta dimostrato. */
    verdict = decide("$\\displaystyle \\forall a, b \\in \\mathbb{Z}\\ (a + b = b + a)$");
    expect(verdict.outcome == PROVED, "un enunciato vero resta dimostrato");

    if (failures) { printf("%d controlli falliti\n", failures); return 1; }
    printf("test_counterexample_certificates: tutti i controlli riusciti\n");
    return 0;
}
