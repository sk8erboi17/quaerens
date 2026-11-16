/* Test del capitolo 20f del verificatore: un enunciato sugli interi
 * dimostrato per saturazione dalle loro leggi (certificate arithmetic).
 * Compilato senza il motore.
 *
 * Ciò che 20f aggiunge ai passi di 11m sono tre cose: la traduzione
 * dell'enunciato, le leggi degli interi e il passo compute. Se una fosse
 * sbagliata, una refutazione potrebbe certificare il falso. Qui:
 *
 *   - un certificato vero, scritto da Vampire sulle clausole del
 *     verificatore ("(ab - 2)(a - b) = 0 e (bc - 2)(b - c) = 0 danno
 *     (ac - 2)(a - c) = 0" tra gli interi), è accettato; gli stessi passi
 *     con l'enunciato negato, con un numero cambiato, senza l'ultimo
 *     passo, con un fatto falso tra i compute, per i razionali, o con
 *     compute in un certificato saturation, no;
 *   - un oracolo ricalcola le clausole di compute con la sua aritmetica:
 *     il verificatore deve accettare esattamente quelle vere, e rifiutare
 *     variabili, simboli di Skolem e il predicato del dominio;
 *   - le leggi sono vere su tutte le scelte di interi da -4 a 4, valutate
 *     da un valutatore di questo file;
 *   - la traduzione dice ciò che dice l'enunciato: su enunciati casuali,
 *     con ogni variabile confinata a pochi valori, il valore dell'albero
 *     tradotto, calcolato da questo file tra gli interi, è quello che un
 *     oracolo dà all'enunciato contando. L'oracolo legge l'albero che lui
 *     stesso ha generato, non il testo e non la traduzione.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_arithmetic_laws \
 *         tests/checker/test_arithmetic_laws.c && /tmp/test_arithmetic_laws
 */

#include <stdio.h>
#include <string.h>
#define main checker_main
#include "../../checker.c"
#undef main

static int failures;

static void expect(int condition, const char *what) {
    if (!condition) { printf("FALLITO: %s\n", what); failures++; }
}

/* La risposta del verificatore a un testo con un solo certificato. */
static CkAnswer answer_of(const char *text, const char **why) {
    const char *cursor = text;
    return ck_check(&cursor, why, 0);
}

/* "Per ogni a, b, c interi: (ab - 2)(a - b) = 0 e (bc - 2)(b - c) = 0
 * implicano (ac - 2)(a - c) = 0", con la refutazione che Vampire ha trovato
 * sulle clausole del verificatore e che Proof/Replay ha scritto nei passi di
 * 11m. */
static const char *const TRUE_CERTIFICATE =
    "certificate arithmetic\n"
    "kind int\n"
    "statement (forall (var 97) (set) (forall (var 98) (set) (forall (var 99) (set) (implies (and (= (mul (sub (mul (var 97) (var 98)) 2) (sub (var 97) (var 98))) 0) (= (mul (sub (mul (var 98) (var 99)) 2) (sub (var 98) (var 99))) 0)) (= (mul (sub (mul (var 97) (var 99)) 2) (sub (var 97) (var 99))) 0)))))\n"
    "clausify forward\n"
    "step compute (clause (ne (f 4) (f 5)))\n"
    "step compute (clause (pos (f 3 (f 4) (f 5))))\n"
    "step compute (clause (neg (f 3 (f 5) (f 4))))\n"
    "step compute (clause (ne (f 4) (f 6)))\n"
    "step compute (clause (pos (f 3 (f 4) (f 6))))\n"
    "step compute (clause (neg (f 3 (f 6) (f 4))))\n"
    "step compute (clause (ne (f 5) (f 6)))\n"
    "step compute (clause (pos (f 3 (f 5) (f 6))))\n"
    "step compute (clause (neg (f 3 (f 6) (f 5))))\n"
    "step compute (clause (eq (f 0 (f 5) (f 5)) (f 6)))\n"
    "step resolve 21 0 20 0 (subst (0 (v 64)) (1 (v 65))) (clause (ne (f 0 (v 0) (f 1 (v 1))) (f 4)) (eq (v 0) (v 1)))\n"
    "step resolve 19 0 18 0 (subst (0 (v 64)) (1 (v 65)) (2 (v 66))) (clause (pos (f 15 (v 0) (v 1) (v 2))) (eq (v 1) (v 2)))\n"
    "step resolve 16 0 15 0 (subst (0 (v 64)) (1 (v 65))) (clause (ne (f 2 (v 0) (v 1)) (f 4)) (pos (f 13 (v 0) (v 1))))\n"
    "step resolve 4 0 3 0 (subst) (clause (ne (f 2 (f 0 (f 2 (f 8) (f 10)) (f 1 (f 6))) (f 0 (f 8) (f 1 (f 10)))) (f 4)))\n"
    "step resolve 4 0 2 0 (subst) (clause (pos (f 11)))\n"
    "step super 9 0 0 1 1 0 0 (subst (0 (f 0 (f 2 (f 9) (f 10)) (f 1 (f 6)))) (1 (f 0 (f 9) (f 1 (f 10))))) (clause (neg (f 11)) (eq (f 2 (f 0 (f 9) (f 1 (f 10))) (f 0 (f 2 (f 9) (f 10)) (f 1 (f 6)))) (f 4)))\n"
    "step super 9 0 0 0 1 0 0 (subst (0 (f 0 (f 2 (f 8) (f 9)) (f 1 (f 6)))) (1 (f 0 (f 8) (f 1 (f 9))))) (clause (neg (f 11)) (eq (f 2 (f 0 (f 8) (f 1 (f 9))) (f 0 (f 2 (f 8) (f 9)) (f 1 (f 6)))) (f 4)))\n"
    "step super 9 0 0 49 0 0 0 (subst (0 (f 0 (f 2 (f 8) (f 10)) (f 1 (f 6)))) (1 (f 0 (f 8) (f 1 (f 10))))) (clause (ne (f 2 (f 0 (f 8) (f 1 (f 10))) (f 0 (f 2 (f 8) (f 10)) (f 1 (f 6)))) (f 4)))\n"
    "step resolve 50 0 51 0 (subst) (clause (eq (f 2 (f 0 (f 9) (f 1 (f 10))) (f 0 (f 2 (f 9) (f 10)) (f 1 (f 6)))) (f 4)))\n"
    "step resolve 50 0 52 0 (subst) (clause (eq (f 2 (f 0 (f 8) (f 1 (f 9))) (f 0 (f 2 (f 8) (f 9)) (f 1 (f 6)))) (f 4)))\n"
    "step super 5 0 0 53 0 0 5 (subst (0 (f 2 (f 8) (f 10))) (1 (f 1 (f 6)))) (clause (ne (f 2 (f 0 (f 8) (f 1 (f 10))) (f 0 (f 1 (f 6)) (f 2 (f 8) (f 10)))) (f 4)))\n"
    "step super 5 0 0 54 0 0 5 (subst (0 (f 2 (f 9) (f 10))) (1 (f 1 (f 6)))) (clause (eq (f 2 (f 0 (f 9) (f 1 (f 10))) (f 0 (f 1 (f 6)) (f 2 (f 9) (f 10)))) (f 4)))\n"
    "step super 5 0 0 55 0 0 5 (subst (0 (f 2 (f 8) (f 9))) (1 (f 1 (f 6)))) (clause (eq (f 2 (f 0 (f 8) (f 1 (f 9))) (f 0 (f 1 (f 6)) (f 2 (f 8) (f 9)))) (f 4)))\n"
    "step super 9 0 0 13 0 0 0 (subst (0 (v 64)) (1 (f 4))) (clause (eq (f 2 (f 4) (v 0)) (f 4)))\n"
    "step super 5 0 0 46 0 0 0 (subst (0 (v 64)) (1 (f 1 (v 65)))) (clause (ne (f 0 (f 1 (v 0)) (v 1)) (f 4)) (eq (v 1) (v 0)))\n"
    "step super 57 0 0 48 0 0 0 (subst (64 (f 0 (f 9) (f 1 (f 10)))) (65 (f 0 (f 1 (f 6)) (f 2 (f 9) (f 10))))) (clause (ne (f 4) (f 4)) (pos (f 13 (f 0 (f 9) (f 1 (f 10))) (f 0 (f 1 (f 6)) (f 2 (f 9) (f 10))))))\n"
    "step reflex 61 0 (subst) (clause (pos (f 13 (f 0 (f 9) (f 1 (f 10))) (f 0 (f 1 (f 6)) (f 2 (f 9) (f 10))))))\n"
    "step super 58 0 0 48 0 0 0 (subst (64 (f 0 (f 8) (f 1 (f 9)))) (65 (f 0 (f 1 (f 6)) (f 2 (f 8) (f 9))))) (clause (ne (f 4) (f 4)) (pos (f 13 (f 0 (f 8) (f 1 (f 9))) (f 0 (f 1 (f 6)) (f 2 (f 8) (f 9))))))\n"
    "step reflex 63 0 (subst) (clause (pos (f 13 (f 0 (f 8) (f 1 (f 9))) (f 0 (f 1 (f 6)) (f 2 (f 8) (f 9))))))\n"
    "step resolve 47 0 17 0 (subst (0 (v 64)) (1 (v 65)) (2 (v 66))) (clause (eq (v 0) (v 1)) (ne (f 2 (v 2) (v 0)) (f 2 (v 2) (v 1))) (eq (v 2) (f 4)))\n"
    "step resolve 14 0 62 0 (subst (0 (f 0 (f 9) (f 1 (f 10)))) (1 (f 0 (f 1 (f 6)) (f 2 (f 9) (f 10))))) (clause (eq (f 0 (f 9) (f 1 (f 10))) (f 4)) (eq (f 0 (f 1 (f 6)) (f 2 (f 9) (f 10))) (f 4)))\n"
    "component (clause (eq (f 0 (f 9) (f 1 (f 10))) (f 4)))\n"
    "complement 0\n"
    "step resolve 68 0 66 0 (subst) (clause (pos (f 26)) (eq (f 0 (f 1 (f 6)) (f 2 (f 9) (f 10))) (f 4)))\n"
    "component (clause (eq (f 0 (f 1 (f 6)) (f 2 (f 9) (f 10))) (f 4)))\n"
    "complement 1\n"
    "step resolve 71 0 69 1 (subst) (clause (pos (f 27)) (pos (f 26)))\n"
    "step super 67 0 0 46 0 0 0 (subst (64 (f 9)) (65 (f 10))) (clause (neg (f 26)) (ne (f 4) (f 4)) (eq (f 9) (f 10)))\n"
    "step reflex 73 1 (subst) (clause (neg (f 26)) (eq (f 9) (f 10)))\n"
    "step super 74 1 1 56 0 0 4 (subst) (clause (neg (f 26)) (ne (f 2 (f 0 (f 8) (f 1 (f 9))) (f 0 (f 1 (f 6)) (f 2 (f 8) (f 10)))) (f 4)))\n"
    "step super 74 1 1 75 1 0 10 (subst) (clause (neg (f 26)) (ne (f 2 (f 0 (f 8) (f 1 (f 9))) (f 0 (f 1 (f 6)) (f 2 (f 8) (f 9)))) (f 4)))\n"
    "step resolve 58 0 76 1 (subst) (clause (neg (f 26)))\n"
    "step resolve 14 0 64 0 (subst (0 (f 0 (f 8) (f 1 (f 9)))) (1 (f 0 (f 1 (f 6)) (f 2 (f 8) (f 9))))) (clause (eq (f 0 (f 8) (f 1 (f 9))) (f 4)) (eq (f 0 (f 1 (f 6)) (f 2 (f 8) (f 9))) (f 4)))\n"
    "component (clause (eq (f 0 (f 8) (f 1 (f 9))) (f 4)))\n"
    "complement 2\n"
    "step resolve 80 0 78 0 (subst) (clause (pos (f 28)) (eq (f 0 (f 1 (f 6)) (f 2 (f 8) (f 9))) (f 4)))\n"
    "component (clause (eq (f 0 (f 1 (f 6)) (f 2 (f 8) (f 9))) (f 4)))\n"
    "complement 3\n"
    "step resolve 83 0 81 1 (subst) (clause (pos (f 29)) (pos (f 28)))\n"
    "step super 79 0 0 46 0 0 0 (subst (64 (f 8)) (65 (f 9))) (clause (neg (f 28)) (ne (f 4) (f 4)) (eq (f 8) (f 9)))\n"
    "step reflex 85 1 (subst) (clause (neg (f 28)) (eq (f 8) (f 9)))\n"
    "step super 86 1 1 57 0 0 2 (subst) (clause (neg (f 28)) (eq (f 2 (f 0 (f 8) (f 1 (f 10))) (f 0 (f 1 (f 6)) (f 2 (f 9) (f 10)))) (f 4)))\n"
    "step super 86 1 1 87 1 0 9 (subst) (clause (neg (f 28)) (eq (f 2 (f 0 (f 8) (f 1 (f 10))) (f 0 (f 1 (f 6)) (f 2 (f 8) (f 10)))) (f 4)))\n"
    "step resolve 56 0 88 1 (subst) (clause (neg (f 28)))\n"
    "step super 9 0 0 65 1 0 0 (subst (0 (v 66)) (1 (v 64))) (clause (eq (v 0) (v 1)) (ne (f 2 (v 0) (v 2)) (f 2 (v 2) (v 1))) (eq (v 2) (f 4)))\n"
    "step super 70 0 0 60 0 0 0 (subst (64 (f 6)) (65 (f 2 (f 9) (f 10)))) (clause (neg (f 27)) (ne (f 4) (f 4)) (eq (f 2 (f 9) (f 10)) (f 6)))\n"
    "step reflex 91 1 (subst) (clause (neg (f 27)) (eq (f 2 (f 9) (f 10)) (f 6)))\n"
    "step super 82 0 0 60 0 0 0 (subst (64 (f 6)) (65 (f 2 (f 8) (f 9)))) (clause (neg (f 29)) (ne (f 4) (f 4)) (eq (f 2 (f 8) (f 9)) (f 6)))\n"
    "step reflex 93 1 (subst) (clause (neg (f 29)) (eq (f 2 (f 8) (f 9)) (f 6)))\n"
    "step super 92 1 0 90 1 1 0 (subst (65 (f 10)) (66 (f 9))) (clause (neg (f 27)) (eq (v 0) (f 10)) (ne (f 2 (v 0) (f 9)) (f 6)) (eq (f 9) (f 4)))\n"
    "component (clause (eq (f 9) (f 4)))\n"
    "complement 4\n"
    "step resolve 97 0 95 3 (subst) (clause (pos (f 30)) (neg (f 27)) (eq (v 0) (f 10)) (ne (f 2 (v 0) (f 9)) (f 6)))\n"
    "component (clause (eq (v 0) (f 10)) (ne (f 2 (v 0) (f 9)) (f 6)))\n"
    "step split 98 (groups (5 0 2 3)) (clause (pos (f 31)) (pos (f 30)) (neg (f 27)))\n"
    "step super 96 0 0 94 1 0 2 (subst) (clause (neg (f 30)) (neg (f 29)) (eq (f 2 (f 8) (f 4)) (f 6)))\n"
    "step super 13 0 0 101 2 0 0 (subst (0 (f 8))) (clause (neg (f 30)) (neg (f 29)) (eq (f 4) (f 6)))\n"
    "step resolve 39 0 102 2 (subst) (clause (neg (f 30)) (neg (f 29)))\n"
    "step super 94 1 0 99 1 0 0 (subst (64 (f 8))) (clause (neg (f 29)) (eq (f 8) (f 10)) (ne (f 6) (f 6)) (neg (f 31)))\n"
    "step reflex 104 2 (subst) (clause (neg (f 29)) (eq (f 8) (f 10)) (neg (f 31)))\n"
    "step super 105 1 1 56 0 0 4 (subst) (clause (neg (f 29)) (neg (f 31)) (ne (f 2 (f 0 (f 8) (f 1 (f 8))) (f 0 (f 1 (f 6)) (f 2 (f 8) (f 10)))) (f 4)))\n"
    "step super 105 1 1 106 2 0 10 (subst) (clause (neg (f 29)) (neg (f 31)) (ne (f 2 (f 0 (f 8) (f 1 (f 8))) (f 0 (f 1 (f 6)) (f 2 (f 8) (f 8)))) (f 4)))\n"
    "step super 8 0 0 107 2 0 1 (subst (0 (f 8))) (clause (neg (f 29)) (neg (f 31)) (ne (f 2 (f 4) (f 0 (f 1 (f 6)) (f 2 (f 8) (f 8)))) (f 4)))\n"
    "step resolve 59 0 108 2 (subst (0 (f 0 (f 1 (f 6)) (f 2 (f 8) (f 8))))) (clause (neg (f 29)) (neg (f 31)))\n"
    "step resolve 72 1 77 0 (subst) (clause (pos (f 27)))\n"
    "step resolve 110 0 100 2 (subst) (clause (pos (f 31)) (pos (f 30)))\n"
    "step resolve 111 0 109 1 (subst) (clause (pos (f 30)) (neg (f 29)))\n"
    "step resolve 112 0 103 0 (subst) (clause (neg (f 29)))\n"
    "step resolve 84 0 113 0 (subst) (clause (pos (f 28)))\n"
    "step resolve 114 0 89 0 (subst) (clause)\n"
    "end\n"
    ;

/* Il testo t con la prima occorrenza di from sostituita da to. */
static char *replaced(const char *t, const char *from, const char *to) {
    static char out[1 << 16];
    const char *at = strstr(t, from);
    if (!at) { snprintf(out, sizeof out, "%s", t); return out; }
    snprintf(out, sizeof out, "%.*s%s%s", (int)(at - t), t, to, at + strlen(from));
    return out;
}

/* Il certificato vero e le sue alterazioni. */
static void test_certificates(void) {
    const char *why;
    char text[1 << 16];
    expect(answer_of(TRUE_CERTIFICATE, &why) == CK_ACCEPTED, "il certificato vero è accettato");

    /* L'enunciato negato: le clausole di partenza sono altre. */
    const char *statement = strstr(TRUE_CERTIFICATE, "statement ") + strlen("statement ");
    const char *line_end = strchr(statement, '\n');
    snprintf(text, sizeof text, "%.*s(not %.*s)%s", (int)(statement - TRUE_CERTIFICATE), TRUE_CERTIFICATE,
             (int)(line_end - statement), statement, line_end);
    expect(answer_of(text, &why) != CK_ACCEPTED, "gli stessi passi non dimostrano la negazione");

    /* Un numero cambiato: con ab = 3 l'enunciato è falso (a = 3, b = 1, c = 2). */
    expect(answer_of(replaced(TRUE_CERTIFICATE, "(var 98)) 2)", "(var 98)) 3)"), &why) != CK_ACCEPTED,
           "con un numero cambiato non è accettato");

    /* Senza l'ultimo passo, che dà la clausola vuota. */
    snprintf(text, sizeof text, "%s", TRUE_CERTIFICATE);
    char *last = strstr(text, "\nend\n");
    char *previous = last;
    while (previous > text && previous[-1] != '\n') previous--;
    memmove(previous, last + 1, strlen(last + 1) + 1);
    expect(answer_of(text, &why) != CK_ACCEPTED, "senza l'ultimo passo non è accettato");

    /* 0 = 2 tra i compute: falso. */
    expect(answer_of(replaced(TRUE_CERTIFICATE, "compute (clause (ne (f 4) (f 6)))", "compute (clause (eq (f 4) (f 6)))"), &why) == CK_REJECTED,
           "un fatto falso tra i compute è respinto");

    /* I razionali non hanno le loro leggi. */
    expect(answer_of(replaced(TRUE_CERTIFICATE, "kind int", "kind rat"), &why) != CK_ACCEPTED, "kind rat non è accettato");

    /* compute non è un passo di saturation. */
    expect(answer_of("certificate saturation\nsort S\nsymbol c S\nstatement (= c c)\nclausify\n"
                     "step compute (clause (eq (f 0) (f 0)))\nend\n", &why) == CK_REJECTED,
           "compute in un certificato saturation è respinto");
}

/* Un valore tra gli interi per le clausole di compute: l'aritmetica di
 * questo file, sui long long, con termini piccoli. */
typedef struct { int symbol; int a, b; } TestTerm;   /* symbol 0 add, 1 neg, 2 mul, >= 4 numero */
static TestTerm terms[64];
static int term_count;
static const long long NUMERALS[] = {0, 1, 2, 5};   /* i numeri dell'enunciato qui sotto, in ordine */

static int random_term(unsigned *seed, int depth) {
    *seed = *seed * 1103515245u + 12345u;
    int kind = (int)(*seed >> 16) % (depth > 2 ? 1 : 4);
    TestTerm t = {0, 0, 0};
    if (kind == 0) { *seed = *seed * 1103515245u + 12345u; t.symbol = 4 + (int)((*seed >> 16) % 4); }
    else if (kind == 1) { t.symbol = 1; t.a = random_term(seed, depth + 1); }
    else { t.symbol = kind == 2 ? 0 : 2; t.a = random_term(seed, depth + 1); t.b = random_term(seed, depth + 1); }
    terms[term_count] = t;
    return term_count++;
}

static long long term_value(int t) {
    TestTerm x = terms[t];
    if (x.symbol >= 4) return NUMERALS[x.symbol - 4];
    if (x.symbol == 1) return -term_value(x.a);
    return x.symbol == 0 ? term_value(x.a) + term_value(x.b) : term_value(x.a) * term_value(x.b);
}

static int term_text(int t, char *out, size_t size) {
    TestTerm x = terms[t];
    if (x.symbol >= 4) return snprintf(out, size, "(f %d)", x.symbol);
    int n = snprintf(out, size, "(f %d ", x.symbol);
    n += term_text(x.a, out + n, size - (size_t)n);
    if (x.symbol != 1) { n += snprintf(out + n, size - (size_t)n, " "); n += term_text(x.b, out + n, size - (size_t)n); }
    return n + snprintf(out + n, size - (size_t)n, ")");
}

/* Il verificatore legge un certificato arithmetic fino a end: ritorna il
 * motivo, "" se tutto ciò che c'era era giusto. */
static const char *read_beginning(const char *text) {
    ck_begin(text, 0);
    ck_word("certificate");
    ck_word("arithmetic");
    ck_a_read();
    const char *why = ck_status == CK_ACCEPTED ? "" : ck_reason;
    ck_finish();
    return why;
}

#define COMPUTE_HEAD "certificate arithmetic\nkind int\nstatement (forall (var 97) (set ge -5) (neq (var 97) (add 2 1)))\nclausify\n"

static void test_compute(void) {
    unsigned seed = 7;
    int accepted = 0, refused = 0;
    for (int round = 0; round < 3000; round++) {
        char clause[4096], text[8192];
        int n = snprintf(clause, sizeof clause, "(clause");
        int literals = 1 + round % 3, holds = 0;
        term_count = 0;
        for (int i = 0; i < literals; i++) {
            seed = seed * 1103515245u + 12345u;
            int kind = (int)(seed >> 16) % 4;
            int a = random_term(&seed, 0), b = random_term(&seed, 0);
            long long x = term_value(a), y = term_value(b);
            static const char *const words[] = {"eq", "ne", "pos (f 3", "neg (f 3"};
            int truth = kind == 0 ? x == y : kind == 1 ? x != y : kind == 2 ? x < y : !(x < y);
            holds |= truth;
            n += snprintf(clause + n, sizeof clause - (size_t)n, " (%s ", words[kind]);
            n += term_text(a, clause + n, sizeof clause - (size_t)n);
            n += snprintf(clause + n, sizeof clause - (size_t)n, " ");
            n += term_text(b, clause + n, sizeof clause - (size_t)n);
            n += snprintf(clause + n, sizeof clause - (size_t)n, kind >= 2 ? "))" : ")");
        }
        snprintf(clause + n, sizeof clause - (size_t)n, ")");
        snprintf(text, sizeof text, COMPUTE_HEAD "step compute %s\nend\n", clause);
        const char *why = read_beginning(text);
        if (holds) { accepted++; expect(!*why, "una clausola vera di compute è accettata"); }
        else { refused++; expect(strcmp(why, "la clausola di compute è falsa tra gli interi") == 0, "una clausola falsa di compute è respinta"); }
        if (failures) { printf("  %s\n  motivo: %s\n", clause, why); return; }
    }
    expect(accepted > 500 && refused > 500, "le clausole vere e quelle false sono tante");

    /* Ciò che non è un conto. Qui i simboli sono add 0, neg 1, mul 2,
     * less 3, i numeri 0 1 2 5 da 4 a 7, il dominio 8 e poi Skolem. */
    expect(*read_beginning(COMPUTE_HEAD "step compute (clause (eq (v 0) (v 0)))\nend\n") != 0, "una variabile in compute è respinta");
    expect(*read_beginning(COMPUTE_HEAD "step compute (clause (pos (f 8 (f 4))))\nend\n") != 0, "il predicato del dominio in compute è respinto");
    expect(*read_beginning(COMPUTE_HEAD "step compute (clause (eq (f 9) (f 9)))\nend\n") != 0, "un simbolo di Skolem in compute è respinto");
    expect(!*read_beginning(COMPUTE_HEAD "step compute (clause (true))\nend\n"), "la clausola (true) è accettata");
    expect(*read_beginning(COMPUTE_HEAD "step compute (clause (false))\nend\n") != 0, "la clausola (false) è respinta");
    expect(*read_beginning(COMPUTE_HEAD "step compute (clause)\nend\n") != 0, "la clausola vuota non si calcola");
}

/* Il valore tra gli interi di un albero di 11 scritto da 20f: le variabili
 * legate hanno i valori di bound[], la più interna per ultima. */
static long long bound[CK_A_DEPTH + 8];
static int bounds;
#define WINDOW 5

static long long tree_value(const CkNode *t) {
    if (t->kind == CK_VAR) return bound[bounds - 1 - t->value];
    switch (t->value) {
    case CK_A_ADD: return tree_value(t->a) + tree_value(t->b);
    case CK_A_NEG: return -tree_value(t->a);
    case CK_A_MUL: return tree_value(t->a) * tree_value(t->b);
    default: return ck_a_numeral[t->value - CK_A_NUMERALS];
    }
}

/* Se il termine t, letto sotto depth quantificatori, nomina la variabile del più interno. */
static int mentions_innermost(const CkNode *t, int depth) {
    if (!t) return 0;
    if (t->kind == CK_VAR) return t->value == depth;
    return mentions_innermost(t->a, depth) || mentions_innermost(t->b, depth);
}

static int tree_holds(const CkNode *c) {
    switch (c->kind) {
    case CK_TRUE: return 1;
    case CK_EQ:
        if (!c->b) return tree_value(c->a->a) < tree_value(c->a->b);   /* less, il solo predicato */
        return tree_value(c->a) == tree_value(c->b);
    case CK_NOT: return !tree_holds(c->a);
    case CK_AND: return tree_holds(c->a) && tree_holds(c->b);
    case CK_OR: return tree_holds(c->a) || tree_holds(c->b);
    case CK_IMPLIES: return !tree_holds(c->a) || tree_holds(c->b);
    case CK_IFF: return tree_holds(c->a) == tree_holds(c->b);
    case CK_FORALL: case CK_EXISTS: {
        /* "esiste k: v = u k", con u e v che non nominano k, è la
         * divisibilità, su tutti gli interi: si calcola. */
        const CkNode *b = c->a;
        if (c->kind == CK_EXISTS && b->kind == CK_EQ && b->b && b->b->kind == CK_APP && b->b->value == CK_A_MUL &&
            b->b->b->kind == CK_VAR && b->b->b->value == 0 && !mentions_innermost(b->a, 0) && !mentions_innermost(b->b->a, 0)) {
            bound[bounds++] = 0;
            long long u = tree_value(b->b->a), v = tree_value(b->a);
            bounds--;
            return u == 0 ? v == 0 : v % u == 0;
        }
        /* Gli altri quantificatori hanno una variabile confinata dentro
         * [-3, 3]: provare da -WINDOW a WINDOW basta. */
        int every = c->kind == CK_FORALL;
        for (long long x = -WINDOW; x <= WINDOW; x++) {
            bound[bounds++] = x;
            int h = tree_holds(c->a);
            bounds--;
            if (h != every) return !every;
        }
        return every;
    }
    default:
        printf("nodo inatteso %d\n", c->kind);
        failures++;
        return 0;
    }
}

/* Le leggi: vere per ogni scelta di x, y, z da -4 a 4. */
static void test_laws(void) {
    const CkNode *laws[CK_A_LAWS];
    ck_begin("(forall (var 97) (set) (= (var 97) (var 97)))", 0);
    ck_n_kind = CK_N_INT;
    const CkNNode *read = ck_n_condition(0);
    expect(ck_a_translate(read, laws) != NULL, "le leggi si leggono");
    for (int i = 0; i < CK_A_LAWS && ck_status == CK_ACCEPTED; i++) {
        bounds = 0;
        if (!tree_holds(laws[i])) { printf("FALLITO: la legge %d non vale tra gli interi: %s\n", i, ck_a_laws[i]); failures++; }
    }
    ck_finish();
}

/* Un enunciato casuale nella notazione di 20, come albero di questo file:
 * l'oracolo lo valuta contando, e lo stesso albero si scrive come testo per
 * il verificatore. Ogni quantificatore confina la sua variabile tra due
 * numeri di [-3, 3] e ha uno degli insiemi di SETS. */
typedef struct { const char *text; int bounded, strict; long long low; int holes; long long hole[2], den[2]; } TestSet;
static const TestSet SETS[] = {
    {"(set)", 0, 0, 0, 0, {0, 0}, {1, 1}},
    {"(set ge 0)", 1, 0, 0, 0, {0, 0}, {1, 1}},
    {"(set ge 1)", 1, 0, 1, 0, {0, 0}, {1, 1}},
    {"(set except 0)", 0, 0, 0, 1, {0, 0}, {1, 1}},
    {"(set gt -2 except 1/2 except 2)", 1, 1, -2, 2, {1, 2}, {2, 1}},
    {"(set ge -1 except -1)", 1, 0, -1, 1, {-1, 0}, {1, 1}},
};
#define SET_COUNT ((int)(sizeof SETS / sizeof *SETS))

static int test_member(long long x, int set) {
    const TestSet *s = &SETS[set];
    if (s->bounded && (s->strict ? x <= s->low : x < s->low)) return 0;
    for (int i = 0; i < s->holes; i++) if (s->den[i] == 1 && x == s->hole[i]) return 0;
    return 1;
}

typedef enum {
    G_NUM, G_VAR, G_ADD, G_SUB, G_MUL, G_NEG, G_POW,
    G_EQ, G_NEQ, G_LESS, G_LESSEQ, G_DIVIDES, G_IN, G_AND, G_OR, G_IMPLIES, G_IFF, G_NOT, G_FORALL, G_EXISTS
} GKind;
typedef struct GNode { GKind kind; long long value; int set, low, high; const struct GNode *a, *b; } GNode;
static GNode pool[1 << 16];
static int pooled;

static unsigned rng = 12345;
static int pick(int n) { rng = rng * 1103515245u + 12345u; return (int)((rng >> 16) % (unsigned)n); }

static GNode *gnode(GKind kind) { GNode *n = &pool[pooled++]; memset(n, 0, sizeof *n); n->kind = kind; return n; }

static int scope[16], scoped;   /* le lettere legate nel punto in cui si genera */

static const GNode *gen_term(int depth) {
    int kind = pick(depth < 2 ? 7 : 2);
    GNode *n;
    if (kind == 0) { n = gnode(G_NUM); n->value = pick(5); return n; }
    if (kind == 1 || kind == 6) {
        n = gnode(kind == 1 ? G_VAR : G_POW);
        GNode *v = kind == 1 ? n : gnode(G_VAR);
        v->value = scope[pick(scoped)];
        if (kind == 6) { n->a = v; n->value = 2 + pick(2); }
        return n;
    }
    static const GKind ops[] = {G_ADD, G_SUB, G_MUL, G_NEG};
    n = gnode(ops[kind - 2]);
    n->a = gen_term(depth + 1);
    if (n->kind != G_NEG) n->b = gen_term(depth + 1);
    return n;
}

static const GNode *gen_condition(int depth) {
    int kind = pick(depth < 3 ? 6 : 1);
    GNode *n;
    if (kind == 0) {
        static const GKind atoms[] = {G_EQ, G_NEQ, G_LESS, G_LESSEQ, G_DIVIDES, G_IN};
        n = gnode(atoms[pick(6)]);
        n->a = gen_term(0);
        if (n->kind == G_IN) n->set = pick(SET_COUNT); else n->b = gen_term(0);
        return n;
    }
    if (kind <= 2) {
        static const GKind connectives[] = {G_AND, G_OR, G_IMPLIES, G_IFF};
        n = gnode(connectives[pick(4)]);
        n->a = gen_condition(depth + 1);
        n->b = gen_condition(depth + 1);
        return n;
    }
    if (kind == 3) { n = gnode(G_NOT); n->a = gen_condition(depth + 1); return n; }
    n = gnode(pick(2) ? G_FORALL : G_EXISTS);
    /* A volte la lettera di un quantificatore esterno: conta il più interno. */
    n->value = scoped && pick(4) == 0 ? scope[pick(scoped)] : 97 + pooled % 20;
    n->set = pick(SET_COUNT);
    n->low = -pick(4);
    n->high = pick(4);
    scope[scoped++] = (int)n->value;
    n->a = gen_condition(depth + 1);
    scoped--;
    return n;
}

/* L'oracolo. */
static long long env_letter[32], env_value[32];
static int envs;

static long long oracle_term(const GNode *n) {
    switch (n->kind) {
    case G_NUM: return n->value;
    case G_VAR:
        for (int i = envs - 1; i >= 0; i--) if (env_letter[i] == n->value) return env_value[i];
        failures++;
        return 0;
    case G_ADD: return oracle_term(n->a) + oracle_term(n->b);
    case G_SUB: return oracle_term(n->a) - oracle_term(n->b);
    case G_MUL: return oracle_term(n->a) * oracle_term(n->b);
    case G_NEG: return -oracle_term(n->a);
    default: { long long x = oracle_term(n->a), r = 1; for (long long i = 0; i < n->value; i++) r *= x; return r; }
    }
}

static int oracle(const GNode *n) {
    long long x, y;
    switch (n->kind) {
    case G_EQ: return oracle_term(n->a) == oracle_term(n->b);
    case G_NEQ: return oracle_term(n->a) != oracle_term(n->b);
    case G_LESS: return oracle_term(n->a) < oracle_term(n->b);
    case G_LESSEQ: return oracle_term(n->a) <= oracle_term(n->b);
    case G_DIVIDES: x = oracle_term(n->a); y = oracle_term(n->b); return x == 0 ? y == 0 : y % x == 0;
    case G_IN: return test_member(oracle_term(n->a), n->set);
    case G_AND: return oracle(n->a) && oracle(n->b);
    case G_OR: return oracle(n->a) || oracle(n->b);
    case G_IMPLIES: return !oracle(n->a) || oracle(n->b);
    case G_IFF: return oracle(n->a) == oracle(n->b);
    case G_NOT: return !oracle(n->a);
    default: {
        int every = n->kind == G_FORALL;
        for (long long v = n->low; v <= n->high; v++) {
            if (!test_member(v, n->set)) continue;
            env_letter[envs] = n->value;
            env_value[envs++] = v;
            int h = oracle(n->a);
            envs--;
            if (h != every) return !every;
        }
        return every;
    }
    }
}

/* Il testo per il verificatore. */
static char written[1 << 16];
static size_t wrote;
static void put(const char *s) { size_t n = strlen(s); if (wrote + n < sizeof written) { memcpy(written + wrote, s, n); wrote += n; } }
static void put_number(long long v) { char b[32]; snprintf(b, sizeof b, v < 0 ? "(neg %lld)" : "%lld", v < 0 ? -v : v); put(b); }
static void put_var(long long letter) { char b[32]; snprintf(b, sizeof b, "(var %lld)", letter); put(b); }

static void write_node(const GNode *n) {
    static const char *const words[] = {
        [G_ADD] = "add", [G_SUB] = "sub", [G_MUL] = "mul", [G_NEG] = "neg", [G_POW] = "pow",
        [G_EQ] = "=", [G_NEQ] = "neq", [G_LESS] = "less", [G_LESSEQ] = "lesseq", [G_DIVIDES] = "divides", [G_IN] = "in",
        [G_AND] = "and", [G_OR] = "or", [G_IMPLIES] = "implies", [G_IFF] = "iff", [G_NOT] = "not",
        [G_FORALL] = "forall", [G_EXISTS] = "exists"};
    char b[32];
    if (n->kind == G_NUM) { snprintf(b, sizeof b, "%lld", n->value); put(b); return; }
    if (n->kind == G_VAR) { put_var(n->value); return; }
    put("("); put(words[n->kind]); put(" ");
    if (n->kind == G_FORALL || n->kind == G_EXISTS) {
        /* Il confine: low <= x <= high, prima del corpo. */
        put_var(n->value); put(" "); put(SETS[n->set].text);
        put(n->kind == G_FORALL ? " (implies (and (lesseq " : " (and (and (lesseq ");
        put_number(n->low); put(" "); put_var(n->value); put(") (lesseq "); put_var(n->value); put(" "); put_number(n->high); put(")) ");
        write_node(n->a);
        put("))");
        return;
    }
    write_node(n->a);
    if (n->kind == G_POW) { snprintf(b, sizeof b, " %lld", n->value); put(b); }
    else if (n->kind == G_IN) { put(" "); put(SETS[n->set].text); }
    else if (n->b) { put(" "); write_node(n->b); }
    put(")");
}

static void test_translation(void) {
    const CkNode *laws[CK_A_LAWS];
    int truths[2] = {0, 0};
    for (int round = 0; round < 4000 && !failures; round++) {
        pooled = scoped = envs = 0;
        scope[scoped++] = 120;
        GNode *root = gnode(pick(2) ? G_FORALL : G_EXISTS);
        root->value = 120; root->set = pick(SET_COUNT); root->low = -pick(4); root->high = pick(4);
        root->a = gen_condition(1);
        scoped = 0;
        int truth = oracle(root);
        wrote = 0;
        write_node(root);
        written[wrote] = 0;

        ck_begin(written, 0);
        ck_n_kind = CK_N_INT;
        ck_n_quantifiers = ck_n_bound = 0;
        const CkNNode *read = ck_n_condition(0);
        const CkNode *translated = ck_status == CK_ACCEPTED ? ck_a_translate(read, laws) : NULL;
        if (!translated) { printf("FALLITO: non tradotto (%s): %s\n", ck_reason, written); failures++; ck_finish(); break; }
        bounds = 0;
        int computed = tree_holds(translated);
        ck_finish();
        truths[truth]++;
        if (computed != truth) { printf("FALLITO: la traduzione vale %d, l'enunciato %d: %s\n", computed, truth, written); failures++; }
    }
    expect(truths[0] > 500 && truths[1] > 500, "enunciati veri e falsi sono tanti");
    printf("traduzione: %d enunciati veri, %d falsi\n", truths[1], truths[0]);
}

/* Un enunciato vero a vuoto ("se y = y implica 4 < 0, allora ..."), da
 * fuzz_numbers.py, seme 44: la clausola vuota è già tra quelle che il
 * verificatore fa dall'enunciato (un letterale t \neq t), e il certificato
 * che Vampire scrive non ha passi oltre i fatti compute. Prima il
 * verificatore guardava solo l'ultima clausola, e lo respingeva. */
static const char *const VACUOUS_CERTIFICATE =
    "certificate arithmetic\n"
    "kind int\n"
    "statement (forall (var 120) (set ge 0) (implies (and (lesseq (neg 2) (var 120)) (lesseq (var 120) 2)) (forall (var 121) (set except 0/1) (implies (and (lesseq (neg 3) (var 121)) (lesseq (var 121) 3)) (implies (implies (= (var 121) (var 121)) (less 4 0)) (implies (neq (pow (var 121) 3) (pow (var 121) 3)) (divides (add (mul 0 4) 2) (add 2 2))))))))\n"
    "clausify forward\n"
    "step compute (clause (ne (f 4) (f 5)))\n"
    "step compute (clause (pos (f 3 (f 4) (f 5))))\n"
    "step compute (clause (neg (f 3 (f 5) (f 4))))\n"
    "step compute (clause (ne (f 4) (f 6)))\n"
    "step compute (clause (pos (f 3 (f 4) (f 6))))\n"
    "step compute (clause (neg (f 3 (f 6) (f 4))))\n"
    "step compute (clause (ne (f 4) (f 7)))\n"
    "step compute (clause (pos (f 3 (f 4) (f 7))))\n"
    "step compute (clause (neg (f 3 (f 7) (f 4))))\n"
    "step compute (clause (ne (f 4) (f 8)))\n"
    "step compute (clause (pos (f 3 (f 4) (f 8))))\n"
    "step compute (clause (neg (f 3 (f 8) (f 4))))\n"
    "step compute (clause (ne (f 5) (f 6)))\n"
    "step compute (clause (pos (f 3 (f 5) (f 6))))\n"
    "step compute (clause (neg (f 3 (f 6) (f 5))))\n"
    "step compute (clause (ne (f 5) (f 7)))\n"
    "step compute (clause (pos (f 3 (f 5) (f 7))))\n"
    "step compute (clause (neg (f 3 (f 7) (f 5))))\n"
    "step compute (clause (ne (f 5) (f 8)))\n"
    "step compute (clause (pos (f 3 (f 5) (f 8))))\n"
    "step compute (clause (neg (f 3 (f 8) (f 5))))\n"
    "step compute (clause (ne (f 6) (f 7)))\n"
    "step compute (clause (pos (f 3 (f 6) (f 7))))\n"
    "step compute (clause (neg (f 3 (f 7) (f 6))))\n"
    "step compute (clause (ne (f 6) (f 8)))\n"
    "step compute (clause (pos (f 3 (f 6) (f 8))))\n"
    "step compute (clause (neg (f 3 (f 8) (f 6))))\n"
    "step compute (clause (ne (f 7) (f 8)))\n"
    "step compute (clause (pos (f 3 (f 7) (f 8))))\n"
    "step compute (clause (neg (f 3 (f 8) (f 7))))\n"
    "step compute (clause (eq (f 0 (f 5) (f 5)) (f 6)))\n"
    "step compute (clause (eq (f 0 (f 5) (f 6)) (f 7)))\n"
    "step compute (clause (eq (f 0 (f 5) (f 7)) (f 8)))\n"
    "step compute (clause (eq (f 0 (f 6) (f 6)) (f 8)))\n"
    "step compute (clause (eq (f 2 (f 6) (f 6)) (f 8)))\n"
    "end\n"
    ;

static void test_vacuous(void) {
    const char *why = "";
    expect(answer_of(VACUOUS_CERTIFICATE, &why) == CK_ACCEPTED, "un enunciato vero già nella sua traduzione è accettato senza passi");
    /* replaced() scrive in un deposito suo: si copia a ogni passo. */
    static char a[1 << 16], b[1 << 16];
    snprintf(a, sizeof a, "%s", replaced(VACUOUS_CERTIFICATE, "(less 4 0)", "(less 0 4)"));
    snprintf(b, sizeof b, "%s", replaced(a, "(neq (pow (var 121) 3) (pow (var 121) 3))", "(= (var 121) (var 121))"));
    snprintf(a, sizeof a, "%s", replaced(b, "(divides (add (mul 0 4) 2) (add 2 2))", "(divides 3 (add 2 2))"));
    expect(strstr(a, "(divides 3 (add 2 2))") && answer_of(a, &why) != CK_ACCEPTED, "reso falso (3 non divide 4), senza passi non è accettato");
}

int main(void) {
    test_certificates();
    test_vacuous();
    test_compute();
    test_laws();
    test_translation();
    if (failures) { printf("%d controlli falliti\n", failures); return 1; }
    printf("test_arithmetic_laws: tutti i controlli riusciti\n");
    return 0;
}
