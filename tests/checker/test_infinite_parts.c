/* Test di 20g del verificatore con gli insiemi di più componenti e le parti:
 * un controesempio infinito fatto di terne di interi, dimostrato a parti e
 * per casi. Compilato senza il motore.
 *
 * "In un monoide, a b = e e a c = e danno b = c" vale in ogni monoide
 * finito ed è falso: nel quoziente del monoide delle parole su a, b, c con
 * ab = ac = 1 fatto delle terne (n, l, m) (lunghezza della parola su b e
 * c, la sua ultima lettera, le a in fondo) a ha due inversi destri. Il
 * certificato tests/checker/infinite_monoid.cert l'ha scritto Vampire
 * (Math/Driver.cpp, certifyByTriples(), nell'integrazione): 31 parti, 60
 * foglie, l'associatività divisa in 30 casi su 8 atomi. Qui:
 *
 *   - è accettato;
 *   - è respinto senza una foglia (i casi non sono più coperti), con una
 *     parte fuori ordine o che manca, con la sola parte 0 (la lettura per
 *     foglie non si chiede dal testo), con b = c (dove l'enunciato è vero),
 *     con un'operazione cambiata, con un atomo su una lettera che la catena
 *     non lega, con una foglia che ha un segno in più o in meno, con una
 *     terna di cinque componenti, con una define con meno componenti, con
 *     l'interpret delle terne dopo una define che le usa, con un testimone
 *     insieme alle parti;
 *   - una parte senza foglie, e una foglia senza refutazione, sono
 *     respinte.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_infinite_parts \
 *         tests/checker/test_infinite_parts.c && /tmp/test_infinite_parts
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define main checker_main
#include "../../checker.c"
#undef main

static int failures;

static void expect(int condition, const char *what) {
    printf("%s: %s\n", condition ? "PASS" : "FAIL", what);
    if (!condition) failures++;
}

static char *monoid;

static char *slurp(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    rewind(f);
    char *text = malloc((size_t)n + 1);
    if (text && fread(text, 1, (size_t)n, f) != (size_t)n) { free(text); text = NULL; }
    if (text) text[n] = 0;
    fclose(f);
    return text;
}

/* La risposta del verificatore, e il motivo in *why. */
static CkAnswer answer_of(const char *text, const char **why) {
    const char *cursor = text, *reason = "";
    CkAnswer answer = ck_check(&cursor, &reason, 0);
    if (why) *why = reason;
    return answer;
}

/* text con la prima occorrenza di old al posto di new, in un testo nuovo (o NULL). */
static char *replaced(const char *text, const char *old, const char *new) {
    const char *at = strstr(text, old);
    if (!at) return NULL;
    size_t before = (size_t)(at - text), length = strlen(text) - strlen(old) + strlen(new);
    char *out = malloc(length + 1);
    memcpy(out, text, before);
    strcpy(out + before, new);
    strcat(out, at + strlen(old));
    return out;
}

/* Il certificato alterato è respinto (non accettato), e il motivo contiene words, se non è NULL. */
static void refused(char *text, const char *words, const char *what) {
    const char *why = "";
    int ok = text && answer_of(text, &why) != CK_ACCEPTED && (!words || strstr(why, words));
    if (!ok && text) printf("    (%s)\n", why);
    expect(ok, what);
    free(text);
}

/* Il testo da at (incluso) a until (escluso) tolto da text. */
static char *cut(const char *text, const char *at, const char *until) {
    char *out = malloc(strlen(text) + 1);
    memcpy(out, text, (size_t)(at - text));
    strcpy(out + (at - text), until);
    return out;
}

static void test_monoid(void) {
    const char *why = "";
    expect(answer_of(monoid, &why) == CK_ACCEPTED, "il monoide delle terne, a parti e per casi, è accettato");
    if (*why) printf("    (%s)\n", why);

    /* Senza la quinta foglia della parte 18, l'associatività. */
    const char *p18 = strstr(monoid, "\npart 18\n"), *p19 = strstr(monoid, "\npart 19\n");
    const char *leaf = p18;
    for (int i = 0; i < 5 && leaf; i++) leaf = strstr(leaf + 1, "\nleaf ");
    const char *next = leaf ? strstr(leaf + 1, "\nleaf ") : NULL;
    expect(p18 && p19 && leaf && next && next < p19, "la parte 18 ha almeno sei foglie");
    if (leaf && next) refused(cut(monoid, leaf + 1, next + 1), "non coprono", "senza una foglia i casi non sono coperti");

    refused(replaced(monoid, "\npart 19\n", "\npart 20\n"), "fuori ordine", "una parte fuori ordine è respinta");
    const char *last = strstr(monoid, "\npart 30\n");
    if (last) {
        char *text = cut(monoid, last + 1, strstr(last, "\nend\n") + 1);
        refused(text, "mancano parti", "senza l'ultima parte è respinto");
    }
    const char *p1 = strstr(monoid, "\npart 1\n");
    if (p1) refused(cut(monoid, p1 + 1, strstr(monoid, "\nend\n") + 1), "mancano parti", "con la sola parte 0 è respinto");

    refused(replaced(monoid, "define b 1 1 0", "define b 1 0 0"), NULL, "con b = c, dove l'enunciato è vero, è respinto");
    refused(replaced(monoid, "(add (sub (var 2) (var 3)) (var 5))))", "(add (sub (var 2) (var 3)) (var 4))))"), NULL,
            "con un'operazione cambiata è respinto");
    refused(replaced(monoid, "atom (= (var 1001) (var 3002))", "atom (= (var 1777) (var 3002))"), "catena",
            "un atomo su una lettera che la catena non lega è respinto");
    refused(replaced(monoid, "atom (= (var 1001) (var 3002))", "atom (forall (var 1001) (set) (= (var 1001) (var 3002)))"), "catena",
            "un atomo con un quantificatore è respinto");
    const char *signs = strstr(monoid, "\npart 18\n");
    signs = signs ? strstr(signs, "\nleaf ") : NULL;
    if (signs) {
        char line[64];
        size_t n = strcspn(signs + 1, "\n");
        snprintf(line, sizeof line, "%.*s", (int)n, signs + 1);
        char longer[80], shorter[64];
        snprintf(longer, sizeof longer, "%s+", line);
        snprintf(shorter, sizeof shorter, "%.*s", (int)n - 1, line);
        refused(replaced(monoid, line, longer), "un segno per atomo", "una foglia con un segno in più è respinta");
        refused(replaced(monoid, line, shorter), "un segno per atomo", "una foglia con un segno in meno è respinta");
    }
    refused(replaced(monoid, "(tuple 3 ", "(tuple 5 "), "W va da 2 a 4", "una terna di cinque componenti è respinta");
    refused(replaced(monoid, "define e 0 0 0\n", "define e 0 0\n"), NULL, "una define con meno componenti è respinta");

    /* L'interpret delle terne dopo una define che le usa. */
    const char *interpret = strstr(monoid, "interpret S "), *define = strstr(monoid, "define perp ");
    if (interpret && define) {
        size_t a = strcspn(interpret, "\n") + 1, b = strcspn(define, "\n") + 1;
        char *text = malloc(strlen(monoid) + 1);
        size_t at = (size_t)(interpret - monoid);
        memcpy(text, monoid, at);
        memcpy(text + at, define, b);
        memcpy(text + at + b, interpret, a);
        strcpy(text + at + a + b, define + b);
        refused(text, NULL, "l'interpret delle terne dopo la define dell'operazione è respinto");
    }
    refused(replaced(monoid, "define c 1 0 0\n", "define c 1 0 0\nwitness 0 0\n"), "testimoni", "un testimone insieme alle parti non è accettato");
}

static void test_rules(void) {
    static const char *const head =
        "certificate infinite\nsort S\nsymbol e S\nstatement (not (= e e))\n"
        "interpret S (tuple 2 (and (lesseq 0 (var 0)) (lesseq 0 (var 1))))\ndefine e 0 0\n";
    char text[1024];
    const char *why = "";
    snprintf(text, sizeof text, "%spart 0\nend\n", head);
    expect(answer_of(text, &why) != CK_ACCEPTED && strstr(why, "senza foglie"), "una parte senza foglie è respinta");
    snprintf(text, sizeof text, "%spart 0\nleaf\nclausify forward\npart 1\nleaf\nclausify forward\nend\n", head);
    expect(answer_of(text, &why) != CK_ACCEPTED && strstr(why, "clausola vuota"), "una foglia senza refutazione è respinta");
    snprintf(text, sizeof text, "%spart 7\nleaf\nclausify forward\nend\n", head);
    expect(answer_of(text, &why) != CK_ACCEPTED, "una parte che l'affermazione non ha è respinta");
    /* Una define valida con S di una componente, poi S di due. */
    expect(answer_of("certificate infinite\nsort S\nsymbol e S\nstatement (not (= e e))\ndefine e 0\n"
                     "interpret S (tuple 2 (and (lesseq 0 (var 0)) (lesseq 0 (var 1))))\nclausify forward\nend\n", &why) != CK_ACCEPTED &&
               strstr(why, "dopo una define"),
           "l'interpret di un insieme di più componenti dopo una define che lo usa è respinto");
}

int main(void) {
    monoid = slurp("tests/checker/infinite_monoid.cert");
    expect(monoid != NULL, "tests/checker/infinite_monoid.cert si legge (dalla cartella principale)");
    if (monoid) test_monoid();
    test_rules();
    free(monoid);
    if (failures) { printf("%d controlli falliti\n", failures); return 1; }
    printf("test_infinite_parts: tutti i controlli riusciti\n");
    return 0;
}
