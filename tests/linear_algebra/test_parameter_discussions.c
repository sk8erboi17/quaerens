/* Test di dominio: ranghi e sistemi lineari con un parametro, discussi per
 * casi (paragrafi 33f-33h di main.c, 17d e 17f di checker.c).
 *
 * Sette controlli. Il primo è quello di tests/statement_checks.h:
 *
 *   1. ogni enunciato di parameter_discussions.tex viene dimostrato, con la
 *      sua dimostrazione scritta: sono i casi delle discussioni stampate
 *      come risultato in due raccolte di esercizi (per quali valori del
 *      parametro il rango è quello, o il sistema è impossibile; quali sono
 *      le soluzioni dove il parametro soddisfa una condizione);
 *   2. ognuno di quei verdetti porta i certificati, e il verificatore,
 *      rileggendoli qui uno per uno, li accetta; tra loro ce n'è almeno uno
 *      del caso generico (con le voci letters e where) e, alterato, non è
 *      più accettato: con la verità rovesciata, con un numero
 *      dell'enunciato cambiato, con un testimone tolto, e con la voce where
 *      che dichiara "dappertutto" al posto del suo polinomio;
 *   3. ogni enunciato di false_parameter_discussions.tex viene dichiarato
 *      FALSO, non solo "non dimostrato": sono gli stessi risultati sbagliati
 *      di poco (un rango sbagliato di uno, un valore speciale spostato,
 *      dimenticato o di troppo, un caso dimenticato, una componente di una
 *      soluzione), con i certificati e gli stessi controlli del punto 2;
 *   4. ogni richiesta di parameter_requests.tex viene risolta, e la risposta
 *      è quella attesa: l'elenco dei casi, o la condizione sul parametro;
 *   5. gli enunciati di special_cases[] (ciò che con un parametro non si
 *      tratta ancora, con il suo motivo; il fatto senza il parametro; un
 *      insieme scritto che in un valore non è definito, o che non lo è in
 *      nessuno) ricevono l'esito atteso;
 *   6. un enunciato troppo lungo viene rifiutato, non letto a metà;
 *   7. nessun passo scartato dai controlli indipendenti.
 *
 * Il metodo sotto esame è la discussione di 33h: il caso generico trovato
 * con i polinomi nel parametro e certificato dove un polinomio non si
 * annulla, i valori speciali come sue radici razionali, il conto sui numeri
 * in ognuno, e la condizione sul parametro messa al posto di ogni fatto. I
 * dati vengono dalle raccolte elencate in parameter_discussions_about.tex e
 * sono stati ricalcolati da un oracolo indipendente
 * (tools/fuzz/fuzz_linear_algebra.py parameter-data e parameter-answers),
 * non dall'uscita del programma. I certificati scritti a mano con la voce
 * where sono in tests/checker/test_systems.c.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_parameter_discussions \
 *         tests/linear_algebra/test_parameter_discussions.c && /tmp/test_parameter_discussions
 */

#include "../statement_checks.h"

/* Le risposte attese delle richieste di parameter_requests.tex, nell'ordine
 * del file. Un caso è il valore, poi &, poi la condizione sul parametro; i
 * valori speciali vengono prima, in ordine crescente, e il caso generico
 * per ultimo. Di un rango si elencano solo i valori speciali in cui non è
 * quello generico. Ogni risposta è stata confrontata con il risultato
 * stampato nella raccolta da un oracolo indipendente
 * (tools/fuzz/fuzz_linear_algebra.py parameter-answers). */
static const char *const expected_answers[] = {
    "\\begin{cases} 1 & t = 0 \\\\ 3 & t \\neq 0 \\end{cases}",
    "\\begin{cases} 2 & t = 0 \\\\ 3 & t \\neq 0 \\end{cases}",
    "\\begin{cases} 1 & t = 1 \\\\ 2 & t \\neq 1 \\end{cases}",
    "\\begin{cases} 2 & t = 1 \\\\ 3 & t \\neq 1 \\end{cases}",
    "\\begin{cases} 2 & k = 5 \\\\ 3 & k \\neq 5 \\end{cases}",
    "\\begin{cases} 1 & k = -1 \\\\ 1 & k = 1 \\\\ 2 & k \\neq -1 \\land k \\neq 1 \\end{cases}",
    "1",
    "k = 5",
    "k = 5",
    "k = -1",
    "k = \\frac{9}{2}",
    "\\begin{cases} \\{(\\frac{5}{4}, 0, \\frac{3}{4}) + t (-2, 1, 0) \\mid t \\in \\mathbb{R}\\} & k = 5 \\\\ \\emptyset & k = 9 \\\\ "
    "\\{(\\frac{-5k + 23}{k - 9}, \\frac{k^{2} + 6k - 58}{2k - 18}, \\frac{-2k + 7}{k - 9})\\} & k \\neq 5 \\land k \\neq 9 \\end{cases}",
    "\\begin{cases} \\emptyset & u = 2 \\\\ \\{(\\frac{-u}{u - 2}, \\frac{u^{2} - u}{u - 2}, \\frac{-u}{u - 2})\\} & u \\neq 2 \\end{cases}",
    "\\begin{cases} \\{(\\frac{2}{3}, -\\frac{11}{3}, -\\frac{10}{3})\\} & k = 1 \\\\ \\emptyset & k \\neq 1 \\end{cases}",
    "\\begin{cases} \\{(-1, 0, 0) + t (1, -1, 1) \\mid t \\in \\mathbb{R}\\} & a = -2 \\\\ \\{(2, 0, 0) + t (1, 1, 0) + s (-1, 0, 1) \\mid t, s \\in \\mathbb{R}\\} & a = 1 \\\\ "
    "\\{(1, a, 2)\\} & a \\neq -2 \\land a \\neq 1 \\end{cases}",
    "\\begin{cases} \\{(1, -1, 0) + t (1, -2, 1) \\mid t \\in \\mathbb{R}\\} & a = 0 \\\\ \\{(1, 0, 0) + t (0, -1, 1) \\mid t \\in \\mathbb{R}\\} & a = 1 \\\\ "
    "\\{(a, 1, -1)\\} & a \\neq 0 \\land a \\neq 1 \\end{cases}",
    "\\begin{cases} \\emptyset & h = -2 \\\\ \\{(1, 0, 0) + t (-1, 1, 0) + s (-1, 0, 1) \\mid t, s \\in \\mathbb{R}\\} & h = 1 \\\\ "
    "\\{(\\frac{-2}{h + 2}, \\frac{h}{h + 2}, \\frac{2h + 2}{h + 2})\\} & h \\neq -2 \\land h \\neq 1 \\end{cases}",
    "\\begin{cases} \\emptyset & a = -2 \\\\ \\emptyset & a = 0 \\\\ \\{(\\frac{1}{2}, \\frac{1}{4}, 0) + t (-1, \\frac{5}{2}, 1) \\mid t \\in \\mathbb{R}\\} & a = 2 \\\\ "
    "\\{(\\frac{1}{a + 2}, \\frac{2a + 3}{a^{2} + 2a}, \\frac{1}{a + 2})\\} & a \\neq -2 \\land a \\neq 0 \\land a \\neq 2 \\end{cases}",
};

/* Un enunciato scritto apposta per mettere alla prova una convenzione o un
 * limite, e l'esito che deve ricevere. */
typedef struct {
    const char *name;       /* che cosa mette alla prova, in inglese */
    const char *statement;  /* l'enunciato, in LaTeX */
    Outcome expected;       /* l'esito atteso */
    const char *piece;      /* un pezzo del motivo per NON SUPPORTATO e NON
                               DIMOSTRATO, del controesempio per FALSO; la
                               risposta intera per RISOLTO; "" se non c'è
                               nulla da guardare */
} SpecialCase;

/* I pezzi usati più volte: una matrice e due sistemi con il parametro k.
 * KK ha rango 1 per k = 1 e k = -1; SYSTEM è x + ky = 1, kx + y = 1, che è
 * impossibile per k = -1 e ha una retta di soluzioni per k = 1; HALF è
 * kx + 3y + 5z = 0, x + 6y + 10z = 0, che per k diverso da 1/2 ha le
 * soluzioni (0, -5t, 3t). */
#define KK "\\begin{pmatrix} k & 1 \\\\ 1 & k \\end{pmatrix}"
#define ALL "\\forall k \\in \\mathbb{R}\\ "
#define SYSTEM "\\{(x, y) \\in \\mathbb{R}^{2} \\mid x + ky = 1 \\land kx + y = 1\\}"
#define HALF "\\{(x, y, z) \\in \\mathbb{R}^{3} \\mid kx + 3y + 5z = 0 \\land x + 6y + 10z = 0\\}"

static const SpecialCase special_cases[] = {
    /* Ciò che la discussione tratta, nelle sue forme. */
    {"a rank that is the same for every value of the parameter", ALL "\\left( \\operatorname{rank} \\begin{pmatrix} k & 1 \\\\ 2k & 2 \\end{pmatrix} = 1 \\right)", PROVED, ""},
    {"a rank compared with an order", ALL "\\left( \\operatorname{rank} " KK " \\ge 1 \\right)", PROVED, ""},
    {"a rank that is never zero", "\\exists k \\in \\mathbb{R}\\ \\left( \\operatorname{rank} " KK " = 0 \\right)", REFUTED, ""},
    {"the values of the parameter for which a rank drops", "\\{k \\in \\mathbb{R} \\mid \\operatorname{rank} " KK " < 2\\}", SOLVED, "k = -1 \\lor k = 1"},
    {"a rank written rk, with a matrix that has a name", "A = " KK " \\implies \\{k \\in \\mathbb{R} \\mid \\mathrm{rk} A = 1\\}", SOLVED, "k = -1 \\lor k = 1"},
    {"a rank and a determinant in the same statement", "A = \\begin{pmatrix} 2 & 1 \\\\ 1 & 2 \\end{pmatrix} \\implies \\{s \\in \\mathbb{R} \\mid \\operatorname{rank}(A - sI) < 2 \\land \\det(A - sI) = 0\\}",
     SOLVED, "s = 1 \\lor s = 3"},
    {"a system that is impossible for one value", "\\{k \\in \\mathbb{R} \\mid " SYSTEM " = \\emptyset\\}", SOLVED, "k = -1"},
    {"a system with solutions for some value", "\\exists k \\in \\mathbb{R}\\ \\left( " SYSTEM " \\neq \\emptyset \\right)", PROVED, ""},
    {"the solutions written with the parameter in a denominator", ALL "\\left( \\left( k \\neq 1 \\land k \\neq -1 \\right) \\implies " SYSTEM " = \\{(\\frac{1}{k + 1}, \\frac{1}{k + 1})\\} \\right)", PROVED, ""},
    {"the same solutions written with another denominator", ALL "\\left( \\left( k \\neq 1 \\land k \\neq -1 \\right) \\implies " SYSTEM " = \\{(\\frac{k - 1}{k^{2} - 1}, \\frac{2}{2k + 2})\\} \\right)", PROVED, ""},
    {"a formula claimed where it is not defined: there the fact does not hold", ALL "\\left( k \\neq 1 \\implies " SYSTEM " = \\{(\\frac{1}{k + 1}, \\frac{1}{k + 1})\\} \\right)", REFUTED, "k = -1"},
    {"a formula claimed where the solutions are more", ALL "\\left( k \\neq -1 \\implies " SYSTEM " = \\{(\\frac{1}{k + 1}, \\frac{1}{k + 1})\\} \\right)", REFUTED, "k = 1"},
    {"a line of solutions written with the parameter in the direction", ALL "\\left( \\left( k \\neq \\frac{1}{2} \\land k \\neq 0 \\right) \\implies " HALF " = \\{t (0, -5k, 3k) \\mid t \\in \\mathbb{R}\\} \\right)", PROVED, ""},
    {"the same where the written direction vanishes: there it is one point", ALL "\\left( k \\neq \\frac{1}{2} \\implies " HALF " = \\{t (0, -5k, 3k) \\mid t \\in \\mathbb{R}\\} \\right)", REFUTED, "k = 0"},
    {"a fact without the parameter inside a statement with one", ALL "\\left( \\{(x, y) \\in \\mathbb{R}^{2} \\mid x + y = 1 \\land x - y = 3\\} = \\{(2, -1)\\} \\right)", PROVED, ""},
    {"a false fact without the parameter", ALL "\\left( \\{(x, y) \\in \\mathbb{R}^{2} \\mid x + y = 1\\} = \\{(1, 0)\\} \\right)", REFUTED, ""},
    {"a request for a rank that does not change", ALL "\\operatorname{rank} \\begin{pmatrix} 1 & 2 \\\\ 2 & 4 \\\\ 3 & 6 \\end{pmatrix}", SOLVED, "1"},
    {"a request for a system that does not change", ALL "\\{(x, y) \\in \\mathbb{R}^{2} \\mid x + y = 1 \\land x - y = 3\\}", SOLVED, "\\{(2, -1)\\}"},
    {"a request with the parameter in the right-hand sides only", ALL "\\{(x, y) \\in \\mathbb{R}^{2} \\mid x + y = k \\land x - y = 3k\\}", SOLVED, "\\{(2k, -k)\\}"},
    {"a request with a vector as the unknown", ALL "\\{x \\in \\mathbb{R}^{2} \\mid " KK " x = \\begin{pmatrix} 1 \\\\ 1 \\end{pmatrix}\\}", SOLVED,
     "\\begin{cases} \\emptyset & k = -1 \\\\ \\{(1, 0) + t (-1, 1) \\mid t \\in \\mathbb{R}\\} & k = 1 \\\\ \\{(\\frac{1}{k + 1}, \\frac{1}{k + 1})\\} & k \\neq -1 \\land k \\neq 1 \\end{cases}"},

    /* Ciò che con un parametro non si tratta ancora: mai una risposta. */
    {"a special value that is irrational", ALL "\\left( \\operatorname{rank} \\begin{pmatrix} k & 2 \\\\ 1 & k \\end{pmatrix} \\ge 1 \\right)", UNSUPPORTED, "irrazionale"},
    {"a system that has solutions for two irrational values only",
     ALL "\\left( \\{(x, y) \\in \\mathbb{R}^{2} \\mid 2x + 3y = k \\land x + 5y = 2 \\land 3x + ky = 1\\} \\neq \\emptyset \\iff 2k^{2} - 38k + 50 = 0 \\right)", UNSUPPORTED, "irrazionale"},
    {"two parameters", "\\forall h \\in \\mathbb{R}\\ \\forall k \\in \\mathbb{R}\\ \\left( \\operatorname{rank} \\begin{pmatrix} h & 1 \\\\ 1 & k \\end{pmatrix} \\ge 1 \\right)", UNSUPPORTED, "un solo parametro"},
    {"a written set with fewer vectors than the free unknowns, in the generic case", ALL "\\left( k \\neq \\frac{1}{2} \\implies " HALF " = \\{(0, 0, 0)\\} \\right)", UNSUPPORTED,
     "non è ancora trattato"},
    {"a vector and its membership, with a parameter", ALL "\\left( (\\frac{1}{k + 1}, \\frac{1}{k + 1}) \\in " SYSTEM " \\right)", UNSUPPORTED, "si confronta con un insieme scritto"},
    {"a rank on the right of the comparison", ALL "\\left( 1 \\le \\operatorname{rank} " KK " \\right)", UNSUPPORTED, "a sinistra di un confronto"},
    {"a rank compared with the parameter", ALL "\\left( \\operatorname{rank} " KK " = k \\right)", UNSUPPORTED, "si confronta con un numero scritto"},
    {"the parameter in a denominator of the system", ALL "\\{(x, y) \\in \\mathbb{R}^{2} \\mid \\frac{x}{k} + y = 1 \\land x - y = 3\\}", UNSUPPORTED, "polinomi nel parametro"},
    {"a root of the parameter in a matrix", ALL "\\left( \\operatorname{rank} \\begin{pmatrix} \\sqrt{k} & 1 \\\\ 1 & k \\end{pmatrix} \\ge 1 \\right)", UNSUPPORTED, "polinomi nel parametro"},
    {"the inverse of a matrix with a parameter", ALL "\\left( \\left( k \\neq 1 \\land k \\neq -1 \\right) \\implies " KK "^{-1} = \\frac{1}{k^{2} - 1} \\begin{pmatrix} k & -1 \\\\ -1 & k \\end{pmatrix} \\right)",
     UNSUPPORTED, ""},
    {"a set of solutions alone inside a statement", ALL "\\left( k = 1 \\implies " SYSTEM " \\right)", UNSUPPORTED, "si confronta con un insieme scritto"},

    /* Anti-regressione: un divisore che è zero per ogni valore del parametro.
     * L'insieme scritto non è definito in nessun valore; prima della
     * correzione il motore cercava senza fine un valore in cui lo fosse. */
    {"a written set divided by zero for every value of the parameter", ALL "\\left( " SYSTEM " = \\{(\\frac{1}{k - k}, 2)\\} \\right)", UNSUPPORTED, "zero per ogni valore del parametro"},
    {"the same with a negative power", ALL "\\left( " SYSTEM " = \\{(\\frac{k}{2}, (k - k)^{-1})\\} \\right)", UNSUPPORTED, "zero per ogni valore del parametro"},
    {"a capital letter is a matrix, not a parameter", "\\forall T \\in \\mathbb{R}\\ \\operatorname{rank} \\begin{pmatrix} T & 1 \\\\ 1 & T \\end{pmatrix}", UNSUPPORTED, ""},
    {"a parameter in the integers", "\\forall k \\in \\mathbb{Z}\\ \\left( " SYSTEM " \\neq \\emptyset \\right)", UNSUPPORTED, ""},
};

/* Il nome di un esito, per i messaggi. */
static const char *outcome_name(Outcome outcome) {
    static const char *const names[] = {[PROVED] = "PROVED", [REFUTED] = "REFUTED", [NOT_PROVED] = "NOT_PROVED",
                                        [UNSUPPORTED] = "UNSUPPORTED", [SOLVED] = "SOLVED"};
    return names[outcome];
}

/* Ritorna la risposta del verificatore al certificato text. */
static CkAnswer checker_answer(const char *text) {
    const char *why;
    return ck_check(&text, &why, 0);
}

/* Altera il certificato one, del caso generico di una discussione (con le
 * voci letters e where), che il verificatore accetta, e conta le
 * alterazioni che accetta ancora: deve essere zero. Aggiunge a *tried quante
 * ne ha provate. Ogni alterazione rende il certificato sbagliato o non
 * controllabile, qualunque sia quello giusto di partenza:
 *
 *   - la verità rovesciata: l'enunciato accettato come vero non può essere
 *     accettato come falso, e viceversa;
 *   - l'ultima cifra della voce statement cambiata, in un enunciato
 *     dichiarato vero: è il rango dichiarato, oppure un coefficiente di una
 *     matrice dell'enunciato, e i testimoni sono quelli dell'enunciato di
 *     prima;
 *   - una voce witness tolta: i testimoni non bastano più;
 *   - la voce where sostituita da "where 1", cioè "per ogni valore": i
 *     testimoni dividono per il polinomio di prima, che ora non è più
 *     dichiarato diverso da zero. Solo se quel polinomio non era un numero. */
static int altered_generic_accepted(const char *one, int *tried) {
    static char altered[CERTIFICATE_BYTES];
    const char *where = strstr(one, "\nwhere "), *statement = strstr(one, "\nstatement "), *claim = strstr(one, "\nclaim "), *witness = strstr(one, "\nwitness ");
    int accepted = 0;
    if (!where || !statement || !claim) return 1;

    /* La verità rovesciata. */
    int was_true = strncmp(claim + 7, "true", 4) == 0;
    snprintf(altered, sizeof altered, "%.*s\nclaim %s%s", (int)(claim - one), one, was_true ? "false" : "true", claim + 7 + (was_true ? 4 : 5));
    (*tried)++;
    accepted += checker_answer(altered) == CK_ACCEPTED;

    /* L'ultima cifra dell'enunciato, se era dichiarato vero: un enunciato
     * dichiarato falso può restare falso anche con un numero cambiato. */
    if (was_true) {
        strcpy(altered, one);
        char *last = altered + (claim - one);
        while (*last < '0' || *last > '9') last--;
        *last = *last == '9' ? '8' : (char)(*last + 1);
        (*tried)++;
        accepted += checker_answer(altered) == CK_ACCEPTED;
    }

    /* Un testimone tolto. */
    if (witness) {
        snprintf(altered, sizeof altered, "%.*s%s", (int)(witness - one), one, strchr(witness + 1, '\n'));
        (*tried)++;
        accepted += checker_answer(altered) == CK_ACCEPTED;
    }

    /* "Per ogni valore" al posto del polinomio dichiarato, se ha la lettera. */
    int number = 1;
    for (const char *p = where + 7; p < statement; p++) number = number && !isalpha((unsigned char)*p);
    if (!number && witness) {
        snprintf(altered, sizeof altered, "%.*s\nwhere 1%s", (int)(where - one), one, statement);
        (*tried)++;
        accepted += checker_answer(altered) == CK_ACCEPTED;
    }
    return accepted;
}

/* Controlli 2 e 3: ogni enunciato di path riceve l'esito expected (PROVED o
 * REFUTED) con dei certificati che il verificatore, riletti qui uno per uno,
 * accetta; tra loro ce n'è uno del caso generico, e nessuno di quelli,
 * alterato, è più accettato. Ritorna 1 se il controllo è riuscito. */
static int check_certified(const char *path, Outcome expected) {
    static char copy[CERTIFICATE_BYTES], one[CERTIFICATE_BYTES];
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }
    const char *position = text, *item, *end;
    int total = 0, right = 0, certificates = 0, altered = 0;
    while (next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        total++;
        if (verdict.outcome != expected || !verdict.certificate || strlen(verdict.certificate) >= sizeof copy) {
            printf("  statement %d of %s: expected %s with its certificates, got %s (%s)\n", total, path, outcome_name(expected),
                   outcome_name(verdict.outcome), verdict.reason ? verdict.reason : verdict.certificate ? "certificates too long for the test" : "no certificate");
            continue;
        }
        strcpy(copy, verdict.certificate);

        /* I certificati, uno per uno: ognuno va da "certificate" al suo
         * "end". */
        int accepted = 1, generic = 0, slipped = 0;
        for (const char *start = strstr(copy, "certificate "); start && accepted;) {
            const char *stop = strstr(start, "\nend\n");
            if (!stop) { accepted = 0; break; }
            size_t length = (size_t)(stop + 5 - start);
            memcpy(one, start, length);
            one[length] = 0;
            accepted = checker_answer(one) == CK_ACCEPTED;
            certificates++;
            if (accepted && strncmp(one, "certificate matrices\nletters ", 29) == 0) {
                generic++;
                slipped += altered_generic_accepted(one, &altered);
            }
            start = strstr(stop + 5, "certificate ");
        }
        if (accepted && generic && !slipped) { right++; continue; }
        printf("  statement %d of %s: its certificates are %s, %d of them are of a generic case, and %d altered ones are still accepted\n", total, path,
               accepted ? "accepted" : "not all accepted", generic, slipped);
    }
    free(text);
    snprintf(summary, sizeof summary,
             "%d of %d statements of %s are %s with certificates that the checker accepts one by one (%d certificates), "
             "and no longer accepts when one of a generic case is altered (%d alterations)",
             right, total, path, expected == PROVED ? "proved" : "declared false", certificates, altered);
    return check_result(total > 0 && right == total && altered > 0, summary);
}

/* Controllo 4: ogni richiesta di path è risolta, con la risposta attesa di
 * expected_answers[], nello stesso ordine. Ritorna 1 se il controllo è
 * riuscito. */
static int check_requests(const char *path) {
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }
    const char *position = text, *item, *end;
    int expected = (int)(sizeof expected_answers / sizeof *expected_answers), total = 0, right = 0;
    while (next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        total++;
        if (total <= expected && verdict.outcome == SOLVED && verdict.certificate && strcmp(verdict.detail, expected_answers[total - 1]) == 0) { right++; continue; }
        printf("  request %d of %s: expected %s, got %s %s\n", total, path, total <= expected ? expected_answers[total - 1] : "(no expected answer)",
               outcome_name(verdict.outcome), verdict.outcome == SOLVED ? verdict.detail : verdict.reason ? verdict.reason : "");
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d requests of %s solved with the expected cases, each with verified certificates", right, total, path);
    return check_result(total == expected && right == total, summary);
}

/* Controllo 5: ogni enunciato di special_cases[] riceve l'esito atteso, e il
 * testo che lo accompagna contiene il pezzo atteso. Ritorna 1 se il
 * controllo è riuscito. */
static int check_special_cases(void) {
    char summary[128];
    int total = (int)(sizeof special_cases / sizeof *special_cases), right = 0;
    for (int i = 0; i < total; i++) {
        const SpecialCase *c = &special_cases[i];
        Verdict verdict = prove(c->statement, 'n', 0, 0);
        const char *said = verdict.outcome == REFUTED || verdict.outcome == SOLVED ? verdict.detail : verdict.reason;

        /* Un esito deciso vuole i certificati; una risposta è confrontata
         * per intero, gli altri testi per un pezzo. */
        int decided = verdict.outcome == PROVED || verdict.outcome == REFUTED || verdict.outcome == SOLVED;
        int text_right = !c->piece[0] || (said && (verdict.outcome == SOLVED ? strcmp(said, c->piece) == 0 : strstr(said, c->piece) != NULL));
        if (verdict.outcome == c->expected && text_right && (!decided || verdict.certificate)) { right++; continue; }
        printf("  %s: expected %s (%s), got %s (%s)\n", c->name, outcome_name(c->expected), c->piece, outcome_name(verdict.outcome), said ? said : "");
    }
    snprintf(summary, sizeof summary, "%d of %d statements on what a discussion with a parameter treats and does not treat answered as expected", right, total);
    return check_result(right == total, summary);
}

/* Controllo 6: un enunciato con un parametro più lungo di MAX_FORMULA viene
 * rifiutato intero. Lo si costruisce ripetendo molte volte lo stesso fatto,
 * che è vero. Ritorna 1 se il controllo è riuscito. */
static int check_too_long(void) {
    static char statement[2 * MAX_FORMULA];
    size_t length = (size_t)snprintf(statement, sizeof statement, "%s\\left( ", ALL);
    while (length < MAX_FORMULA + 64) length += (size_t)snprintf(statement + length, sizeof statement - length, "\\operatorname{rank} %s \\ge 1 \\land ", KK);
    snprintf(statement + length, sizeof statement - length, "\\operatorname{rank} %s \\ge 1 \\right)", KK);
    Verdict verdict = prove(statement, 'n', 0, 0);
    return check_result(verdict.outcome == UNSUPPORTED && verdict.reason && strstr(verdict.reason, "troppo lungo"),
                        "a statement with a parameter longer than the limit is refused whole, not read in part");
}

int main(void) {
    engine_init();

    /* Tutti i controlli vengono eseguiti anche se uno fallisce, così il
     * resoconto è completo. */
    int passed = check_all_proved("tests/linear_algebra/parameter_discussions.tex");
    passed = check_certified("tests/linear_algebra/parameter_discussions.tex", PROVED) && passed;
    passed = check_certified("tests/linear_algebra/false_parameter_discussions.tex", REFUTED) && passed;
    passed = check_requests("tests/linear_algebra/parameter_requests.tex") && passed;
    passed = check_special_cases() && passed;
    passed = check_too_long() && passed;

    /* Ogni lettura ha superato il suo ricontrollo, ogni testimone i suoi
     * prodotti, ogni risposta la sua rilettura, e il verificatore non ha
     * respinto nessun certificato. */
    passed = check_result(unjustified_steps == 0, "no reading failed its check, no witness failed its products, no answer failed its reading, no certificate was rejected") && passed;
    return passed ? 0 : 1;
}
