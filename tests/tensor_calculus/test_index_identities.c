/* Test delle identità con gli indici del calcolo tensoriale (capitoli 37 e
 * 39 di parser.c, capitolo 38 di main.c, capitolo 19 di checker.c): delta di
 * Kronecker, simbolo di Levi-Civita, componenti di tensori qualsiasi, con la
 * convenzione di Einstein, in dimensione fissa, anche con una metrica
 * diagonale dichiarata tra le ipotesi, con tensori dati per componenti, con
 * indici greci e con indici che cominciano da 0.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_index_identities \
 *         tests/tensor_calculus/test_index_identities.c && /tmp/test_index_identities
 *
 * I dati sono in nove file accanto a questo, ognuno con la sua nota
 * (*_about.tex):
 *
 *   - index_identities.tex: identità vere, prese dagli esercizi svolti e
 *     confermate con un terzo conto (tools/fuzz/fuzz_tensors.py, modo data).
 *     Devono essere tutte dimostrate, con il certificato accettato;
 *   - false_index_identities.tex: le stesse con un segno, due indici, un
 *     fattore o la dimensione cambiati. Qui "non dimostrata" non basta: il
 *     conto è completo, quindi devono essere tutte dichiarate false, con il
 *     certificato del controesempio accettato;
 *   - index_notation_beyond.tex: enunciati che il lettore non accetta.
 *     Devono essere tutti NON SUPPORTATO, con un motivo: né dimostrati né
 *     smentiti;
 *   - metric_identities.tex e false_metric_identities.tex: identità con una
 *     metrica dichiarata tra le ipotesi (capitolo 39 di parser.c), vere e
 *     false di poco, con gli stessi controlli dei primi due file;
 *   - given_components.tex e false_given_components.tex: enunciati che tra
 *     le ipotesi danno un tensore per componenti, con una tabella o con un
 *     elenco (39d), veri e falsi di poco: una componente cambiata, un segno
 *     della metrica, un indice alzato letto come abbassato, gli indici da 0
 *     a 3 letti come da 1 a 4;
 *   - index_ranges.tex e false_index_ranges.tex: identità con gli indici
 *     greci o con i valori che cominciano da 0 (37d, 37f, 39e), vere e false
 *     di poco.
 *
 * I certificati alterati sono antiregressione per il verificatore: quello di
 * un'identità vera con "fails" al posto di "holds", quello di una falsa con
 * "holds" al posto di "fails" o con un valore scritto cambiato, non devono
 * essere accettati, e per il motivo giusto.
 */

#include "../statement_checks.h"

/* Scrive in out, capiente size, il certificato certificate di un'identità
 * vera con la voce "holds" sostituita da "fails" nel punto in cui ogni
 * indice libero di free vale 1. Ritorna 0 se il testo non ha la forma
 * attesa o non ci sta. */
static int alter_true(const char *certificate, IndexSet free, char *out, size_t size) {
    const char *claim = strstr(certificate, "\nholds\n");
    if (!claim) return 0;
    int used = snprintf(out, size, "%.*s\nfails", (int)(claim - certificate), certificate);
    for (int slot = 0; slot < INDEX_LETTERS && used > 0 && (size_t)used < size; slot++)
        if (free >> slot & 1) used += snprintf(out + used, size - (size_t)used, " %c 1", index_letter(slot));
    if (used <= 0 || (size_t)used >= size) return 0;
    return snprintf(out + used, size - (size_t)used, "\nend\n") < (int)(size - (size_t)used);
}

/* Scrive in out il certificato certificate di un'identità falsa, alterato:
 * con kind 0 la voce "fails" e i valori scritti sono sostituiti da "holds";
 * con kind 1 al valore scritto del primo membro è aggiunto 1. Ritorna 0 se
 * il testo non ha la forma attesa o non ci sta. */
static int alter_false(const char *certificate, int kind, char *out, size_t size) {
    const char *claim = strstr(certificate, "\nfails"), *left = strstr(certificate, "\nleft "), *right = strstr(certificate, "\nright ");
    if (!claim || !left || !right) return 0;
    int used = kind == 0 ? snprintf(out, size, "%.*s\nholds\nend\n", (int)(claim - certificate), certificate)
                         : snprintf(out, size, "%.*s\nleft (+ 1 %.*s)%s", (int)(left - certificate), certificate,
                                    (int)(right - left - 6), left + 6, right);
    return used > 0 && (size_t)used < size;
}

/* Ritorna la risposta del verificatore sul certificato text, e mette in
 * *why il motivo. */
static CkAnswer reread(const char *text, const char **why) {
    *why = "";
    return ck_check(&text, why, 0);
}

/* Controlla il file path. Con truth 1 ogni enunciato deve essere
 * dimostrato, con truth 0 dichiarato falso; in tutti e due i casi con un
 * certificato che il verificatore, rileggendolo, accetta, e i cui alterati
 * non sono accettati per il motivo giusto. expected è il numero degli
 * enunciati del file, perché il controllo non passi a vuoto. */
static int check_certified(const char *path, int truth, int expected) {
    static char altered[2 * CERTIFICATE_BYTES], kept[CERTIFICATE_BYTES];
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }
    const char *position = text, *item, *end, *why;
    int total = 0, decided = 0, certified = 0, alterations = 0, stopped = 0;
    while (next_item(&position, &item, &end)) {
        Item statement;
        total++;
        if (read_item(item, end, &statement)) { printf("  statement %d cannot be read\n", total); continue; }
        Verdict verdict = prove(statement.formula, statement.variable, statement.has_base, statement.base);
        if (verdict.outcome != (truth ? PROVED : REFUTED)) {
            printf("  statement %d should be %s: ", total, truth ? "proved" : "refuted");
            verdict_print(stdout, &verdict);
            putchar('\n');
            continue;
        }
        decided++;
        if (!verdict.certificate || strlen(verdict.certificate) >= sizeof kept || reread(verdict.certificate, &why) != CK_ACCEPTED) {
            printf("  statement %d has no certificate the checker accepts\n", total);
            continue;
        }
        certified++;

        /* Il certificato va copiato: rileggere l'enunciato per sapere i
         * suoi indici liberi riusa i depositi del motore. */
        strcpy(kept, verdict.certificate);
        IndexStatement read;
        arena_used = arena_keep;
        failure = NULL;
        int reread_ok = strstr(statement.formula, "\\implies") ? parse_index_hypotheses(statement.formula, &read) : parse_index_statement(statement.formula, &read);
        if (!reread_ok) { printf("  statement %d cannot be read again\n", total); continue; }
        for (int kind = 0; kind < (truth ? 1 : 2); kind++) {
            const char *wanted = truth ? "sono uguali" : kind == 0 ? "sono diversi" : "valore scritto";
            alterations++;
            if (!(truth ? alter_true(kept, read.free, altered, sizeof altered) : alter_false(kept, kind, altered, sizeof altered))) {
                printf("  statement %d: cannot build the altered certificate\n", total);
                continue;
            }
            if (reread(altered, &why) == CK_REJECTED && strstr(why, wanted)) { stopped++; continue; }
            printf("  statement %d: the altered certificate is not rejected as expected (%s):\n%s", total, why, altered);
        }
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d statements %s, %d with a certificate the checker accepts, in %s", decided, total,
             truth ? "proved" : "refuted", certified, path);
    int passed = check_result(total == expected && decided == total && certified == total, summary);
    snprintf(summary, sizeof summary, "%d of %d altered certificates rejected for the right reason, for %s", stopped, alterations, path);
    return check_result(alterations > 0 && stopped == alterations, summary) && passed;
}

/* Controlla che ogni enunciato del file path sia NON SUPPORTATO. */
static int check_all_unsupported(const char *path, int expected) {
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }
    const char *position = text, *item, *end;
    int total = 0, unsupported = 0;
    while (next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        total++;
        if (verdict.outcome == UNSUPPORTED && verdict.reason && verdict.reason[0]) { unsupported++; continue; }
        printf("  statement %d should be unsupported, with a reason: ", total);
        verdict_print(stdout, &verdict);
        putchar('\n');
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d statements outside the language are unsupported, each with its reason, in %s", unsupported, total, path);
    return check_result(total == expected && unsupported == total, summary);
}

/* Un enunciato, il verdetto atteso e un pezzo del testo che lo accompagna:
 * il motivo per NOT_PROVED e UNSUPPORTED, il controesempio per REFUTED, la
 * dimostrazione per PROVED. Le stringhe sono dati, mai regole del motore. */
typedef struct {
    const char *statement;
    Outcome outcome;
    const char *piece;
    const char *name;  /* che cosa mette alla prova, in inglese */
} Case;

int main(void) {
    static const Case cases[] = {
        /* La dimensione decide: la stessa identità con la delta è vera in
         * tre dimensioni e falsa in due, e viceversa. */
        {"\\forall i, k \\in \\{1, 2, 3\\}\\ (\\delta_{ik}\\delta_{ll} - \\delta_{il}\\delta_{lk} = 2\\delta_{ik})", PROVED, "dimension $3$",
         "an identity that holds only in three dimensions is proved there"},
        {"\\forall i, k \\in \\{1, 2\\}\\ (\\delta_{ik}\\delta_{ll} - \\delta_{il}\\delta_{lk} = 2\\delta_{ik})", REFUTED, "il primo membro vale 1, il secondo 2",
         "the same identity is refuted in two dimensions, with the two values"},
        {"\\varepsilon_{ijk}\\varepsilon_{imn} = \\delta_{jm}\\delta_{kn} - \\delta_{jn}\\delta_{km}", PROVED, "$81$ choices",
         "the proof counts the 81 values of the four free indices"},
        {"\\varepsilon_{ijk}\\varepsilon_{ijk} = 6", PROVED, "Their common value is $6$",
         "a statement without free indices shows the common value"},

        /* Il controesempio: il punto e i due valori, anche quando sono
         * polinomi nelle componenti. */
        {"\\varepsilon_{ijk} = \\varepsilon_{jik}", REFUTED, "i = 3, j = 2, k = 1: il primo membro vale -1, il secondo 1",
         "the counterexample names the point and the two values"},
        {"\\varepsilon_{ijk} a_{j} b_{k} = 0", REFUTED, "i = 1: il primo membro vale a_{2} b_{3} - a_{3} b_{2}, il secondo 0",
         "the values of a counterexample with tensors are polynomials in the components"},
        {"\\frac{1}{3}\\varepsilon_{123} a_{j} a_{j} = \\frac{1}{2} a_{k} a_{k}", REFUTED, "\\frac{1}{3} a_{1} a_{1} + \\frac{1}{3} a_{2} a_{2} + \\frac{1}{3} a_{3} a_{3}",
         "a square is written as a repeated factor, with a rational coefficient"},
        {"\\varepsilon_{ijk}\\varepsilon_{ijk} = 3", REFUTED, "senza indici liberi: il primo membro vale 6, il secondo 3",
         "a false statement without free indices gives the two values"},

        /* Lo 0 da solo è il tensore nullo; la parentesi dopo i quantificatori
         * può aprire solo il primo fattore. */
        {"\\varepsilon_{ijk} a_{j} a_{k} = 0", PROVED, "The letter $a$ denotes an arbitrary tensor", "zero alone stands for the zero tensor"},
        {"0 = \\varepsilon_{ijk} a_{j} a_{k}", PROVED, "The letter $a$ denotes an arbitrary tensor", "zero alone on the left stands for the zero tensor"},
        {"\\varepsilon_{ijk} a_{j} b_{k} = -\\varepsilon_{ijk} b_{j} a_{k}", PROVED, "The letters $a$, $b$ denote arbitrary tensors",
         "the proof names the arbitrary tensors of the statement"},
        {"\\varepsilon_{ijk} a_{j} a_{k} + 0 = 0 + 0", UNSUPPORTED, "indici liberi diversi", "a sum of zeros is not the zero tensor"},
        {"\\forall i \\in \\{1, 2, 3\\}\\ (\\delta_{ij} a_{j} = 1)", UNSUPPORTED, "indici liberi diversi", "a number that is not zero is not the zero tensor"},
        {"\\forall i \\in \\{1, 2, 3\\}\\ (a_{i} + b_{i})\\varepsilon_{123} = a_{i} + b_{i}", PROVED, "free index is $i$",
         "after the quantifiers a parenthesis may open only the first factor"},

        /* Un enunciato che comincia con \lim o con il segno di derivata
         * resta al suo capitolo, con il suo motivo. */
        {"\\lim_{\\varepsilon \\to 0} \\varepsilon^{2} = 0", UNSUPPORTED, "un limite si scrive", "a limit over a Greek letter is still refused by the limit reader"},
        {"\\forall j \\in \\{1, 2, 3\\}\\ (a_{i}\\delta_{ij} = a)", UNSUPPORTED, "compare con 1 e con 0 indici",
         "the reason of a wrapped identity is the one of the reading that went further"},

        /* Il segno di somma dichiara la dimensione, e non aggiunge somme. */
        {"\\sum_{i=1}^{3} \\delta_{ii} = 3", PROVED, "dimension $3$", "a written summation sign declares the dimension"},
        {"\\sum_{i=1}^{5} \\delta_{ii} = 5", PROVED, "dimension $5$", "the dimension is not limited to the ones of the exercises"},
        {"\\sum_{i=1}^{3}\\sum_{j=1}^{3} \\delta_{ij}\\delta_{ij} = 3", PROVED, "dimension $3$", "two summation signs on a product sum each index once"},
        {"\\sum_{i=1}^{3} \\delta_{jj} a_{i} = 3 a_{i}", UNSUPPORTED, "non compare due volte", "a summation sign on an index written once is refused"},

        /* La metrica non si sottintende: \varepsilon con gli indici in alto
         * e in basso, senza una metrica dichiarata, è rifiutato; con la
         * metrica euclidea dichiarata è 6, e con una lorentziana il segno
         * lo decide la metrica. */
        {"\\varepsilon^{ijk}\\varepsilon_{ijk} = 6", UNSUPPORTED, "determinante della metrica", "epsilon with upper and lower indices needs a declared metric"},
        {"\\varepsilon^{ijkl}\\varepsilon_{ijkl} = -24", UNSUPPORTED, "va dichiarata tra le ipotesi", "no signature is chosen for the writer"},
        {"g_{ij} = \\operatorname{diag}(1, 1, 1) \\land \\varepsilon_{123} = 1 \\implies \\varepsilon^{ijk}\\varepsilon_{ijk} = 6", PROVED, "declared metric",
         "with the Euclidean metric declared the contraction is 6"},
        {"g_{ij} = \\operatorname{diag}(-1, 1, 1, 1) \\land \\varepsilon_{1234} = 1 \\implies \\varepsilon^{ijkl}\\varepsilon_{ijkl} = -24", PROVED, "$g_{ii} = -1, 1, 1, 1$",
         "with a Lorentzian metric declared the contraction is -24, and the proof states the metric"},
        {"g_{ij} = \\operatorname{diag}(-1, 1, 1, 1) \\land \\varepsilon_{1234} = 1 \\implies \\varepsilon^{ijkl}\\varepsilon_{ijkl} = 24", REFUTED, "il primo membro vale -24, il secondo 24",
         "the Euclidean value is refuted under a Lorentzian metric"},
        {"g_{ij} = \\mathrm{diag}(1, 1, 1, 1) \\land \\varepsilon_{1234} = 1 \\implies \\varepsilon^{ijkl}\\varepsilon_{ijkl} = 24", PROVED, "declared metric",
         "the same value is proved under the Euclidean metric"},
        {"g_{ij} = \\operatorname{diag}(1, -1, -1, -1) \\land \\varepsilon^{1234} = 1 \\implies \\varepsilon_{1234} = -1", PROVED, "indices above",
         "the declared value of epsilon is the one with the indices in the declared position"},
        {"g^{ij} = \\operatorname{diag}(\\frac{1}{2}, \\frac{1}{3}) \\implies g_{11} + g_{22} = 5", PROVED, "$g_{ii} = 2, 3$",
         "a metric declared with upper indices is the inverse of the one with lower indices"},
        /* I valori di un controesempio sono polinomi nelle componenti
         * dichiarate, e sono scritti con gli indici dove l'enunciato li
         * dichiara: A è dichiarato con l'indice in alto, e A_{1} al posto
         * di A^{1} sarebbe la componente abbassata, un altro numero. */
        {"g_{ij} = \\operatorname{diag}(2, 3) \\implies \\forall i \\in \\{1, 2\\}\\ (g_{ij} A^{j} = 2 A_{i})", REFUTED, "i = 1: il primo membro vale 2 A^{1}, il secondo 4 A^{1}",
         "a counterexample with a metric shows the components in the declared position"},
        {"g_{ij} = \\operatorname{diag}(-1, 1, 1, 1) \\implies A^{i} B_{i} = A_{1} B_{1} + A_{2} B_{2} + A_{3} B_{3} + A_{4} B_{4}", REFUTED,
         "il primo membro vale A^{1} B_{1} + A^{2} B_{2} + A^{3} B_{3} + A^{4} B_{4}, il secondo -A^{1} B_{1} + A^{2} B_{2} + A^{3} B_{3} + A^{4} B_{4}",
         "a contraction is not the sum of the products of the lowered components under a Lorentzian metric"},
        {"g_{ij} = \\operatorname{diag}(-1, 2) \\implies \\forall i, j \\in \\{1, 2\\}\\ (S_{i}{}^{j} = S^{j}{}_{i})", REFUTED,
         "i = 2, j = 1: il primo membro vale S_{2}{}^{1}, il secondo -2 S_{1}{}^{2}", "a component with mixed positions is shown with its mixed positions"},
        {"g_{ij} = \\operatorname{diag}(2, 3) \\implies M^{ij} u_{i} v_{j} = \\frac{1}{4} M_{11} u_{1} v_{1} + \\frac{1}{6} M_{12} u_{1} v_{2} + "
         "\\frac{1}{6} M_{21} u_{2} v_{1} + \\frac{1}{9} M_{22} u_{2} v_{2}", PROVED, "declared metric",
         "a side written component by component, with every index lowered, is proved"},
        {"g_{ij} = \\operatorname{diag}(1, 1) \\implies a_{i} a^{i} = a^{1} a^{1} + a^{2} a^{2}", UNSUPPORTED, "può essere una potenza",
         "a numeric upper index on a tensor is refused in a statement also with a metric"},

        /* Le lettere nuove dello svolgimento. Nel secondo addendo del primo
         * membro A^{j} prende una lettera nuova dentro la parentesi, e B_{j},
         * che la moltiplica, deve prenderne un'altra: la stessa lettera
         * comparirebbe quattro volte in un prodotto. */
        {"g_{ij} = \\operatorname{diag}(2, 3) \\implies A_{i} B^{i} + (A^{j} + C^{j}) B_{j} = 2 A_{i} B^{i} + C^{i} B_{i}", PROVED, "declared metric",
         "a moved index inside a bracket and one in the factor that multiplies it are unfolded on two letters"},
        {"g_{ij} = \\operatorname{diag}(2, 3) \\implies A_{i} B^{i} + (A^{j} + C^{j}) B_{j} = 2 A_{i} B^{i} + 2 C^{i} B_{i}", REFUTED, "il secondo",
         "the same statement with a wrong coefficient is refuted"},

        /* Che cosa manca, detto nel motivo. */
        {"g_{ij} = \\operatorname{diag}(-1, 1, 1, 1) \\implies \\varepsilon^{ijkl}\\varepsilon_{ijkl} = -24", UNSUPPORTED, "quanto vale \\varepsilon",
         "a declared metric without the value of epsilon is refused, and the reason says what is missing"},
        {"\\varepsilon_{1234} = 1 \\implies \\varepsilon^{ijkl}\\varepsilon_{ijkl} = -24", UNSUPPORTED, "manca la metrica", "hypotheses without a metric are refused"},
        {"g_{ij} = \\operatorname{diag}(-1, 1, 1, 1) \\implies A_{i} B_{i} = B_{i} A_{i}", UNSUPPORTED, "nella stessa posizione",
         "with a metric two lower indices are not a contraction"},
        {"g_{ij} = \\operatorname{diag}(-1, 1, 1, 1) \\implies \\delta_{ij} A^{j} = A_{i}", UNSUPPORTED, "sarebbe la metrica",
         "with a metric the Kronecker delta has one upper and one lower index"},
        {"g_{ij} = \\operatorname{diag}(-1, 1, 1) \\land \\varepsilon_{1234} = 1 \\implies \\varepsilon^{ijkl}\\varepsilon_{ijkl} = -24", UNSUPPORTED, "ma la metrica ha 3",
         "the declared epsilon and the metric must have the same dimension"},
        {"g_{ij} = \\operatorname{diag}(-1, 1, 1, 1) \\implies a_{i} b^{i} = b_{i} a^{i}", PROVED, "declared metric",
         "an identity without delta or epsilon is recognised by its declared metric"},

        /* I tensori dati per componenti (39d). La dimostrazione dice quali
         * sono, e gli altri restano qualsiasi; un controesempio mostra i
         * valori con le componenti dei tensori qualsiasi. */
        {"g_{ij} = \\operatorname{diag}(2, 3) \\land T_{ij} = \\begin{pmatrix} 1 & 2 \\\\ 3 & 4 \\end{pmatrix} \\implies T^{i}{}_{i} = \\frac{11}{6}", PROVED,
         "The components of $T$ are the ones that the statement declares", "the proof names the tensors given by their components"},
        {"g_{ij} = \\operatorname{diag}(2, 3) \\land T_{ij} = \\begin{pmatrix} 1 & 2 \\\\ 3 & 4 \\end{pmatrix} \\implies T^{i}{}_{i} = 5", REFUTED,
         "il primo membro vale \\frac{11}{6}, il secondo 5", "the Euclidean trace of a given table is refuted under the declared metric"},
        {"g_{ij} = \\operatorname{diag}(2, 3) \\land T_{ij} = \\begin{pmatrix} 1 & 2 \\\\ 3 & 4 \\end{pmatrix} \\implies \\forall i, j \\in \\{1, 2\\}\\ (T^{i}{}_{j} = T_{j}{}^{i})", REFUTED,
         "i = 2, j = 1: il primo membro vale 1, il secondo \\frac{2}{3}", "raising the first index of a given table is not raising the second"},
        {"A_{i} = (a, 2 b, a b - \\frac{1}{2}) \\implies \\varepsilon_{ijk} A_{j} A_{k} = 0", PROVED, "The letters $a$, $b$ denote arbitrary tensors",
         "the letters in the elements of a list stay arbitrary"},
        {"A_{i} = (a, 2 b, a b - \\frac{1}{2}) \\implies \\delta_{ij} A_{i} A_{j} = a a + 4 b b + a a b b - a b", REFUTED, "\\frac{1}{4} - a b + a a + a a b b + 4 b b",
         "the value of a side with given components is a polynomial in the letters of the elements"},
        {"F_{ij} = \\begin{pmatrix} 1 & 2 \\\\ 3 & 4 \\end{pmatrix} \\implies \\delta_{ij} F_{ij} = 5", PROVED, "dimension $2$",
         "a table declares the dimension, and can be given without a metric"},
        {"F^{ij} = \\begin{pmatrix} 1 & 2 \\\\ 3 & 4 \\end{pmatrix} \\implies \\delta_{11} F^{21} = 2", REFUTED, "il primo membro vale 3, il secondo 2",
         "the first index of a table is the row: a table read by columns is refuted"},

        /* Che cosa manca a un'ipotesi con una tabella, detto nel motivo. */
        {"g_{ij} = \\operatorname{diag}(1, 1) \\land F = \\begin{pmatrix} 1 & 2 \\\\ 3 & 4 \\end{pmatrix} \\implies F^{i}{}_{i} = 5", UNSUPPORTED, "non dice se le componenti",
         "a table without the positions of its indices is refused"},
        {"g_{ij} = \\operatorname{diag}(1, 1) \\land F_{ij} = \\begin{pmatrix} 1 & F_{11} \\\\ 3 & 4 \\end{pmatrix} \\implies F^{i}{}_{i} = 5", UNSUPPORTED, "potrebbero contraddirsi",
         "a table that names itself is refused"},
        {"g_{ij} = \\operatorname{diag}(1, 1) \\land F_{ij} = \\begin{pmatrix} 1 & A_{1} \\\\ 3 & 4 \\end{pmatrix} \\land A_{i} = (1, 2) \\implies F^{i}{}_{i} = 5", UNSUPPORTED, "prima di nominarlo",
         "a tensor used in a table cannot be given afterwards"},
        {"g_{ij} = \\operatorname{diag}(1, 1) \\land F_{ij} = \\begin{pmatrix} 1 2 \\\\ 3 & 4 \\end{pmatrix} \\implies F^{i}{}_{i} = 5", UNSUPPORTED, "due numeri uno dopo l'altro",
         "two numbers without a separator in a table are refused, not multiplied"},
        {"g_{ij} = \\operatorname{diag}(1, 1) \\land F_{ij} = \\begin{pmatrix} 1 & 2 & 3 \\\\ 3 & 4 & 5 \\\\ 1 & 1 & 1 \\end{pmatrix} \\implies F^{i}{}_{i} = 5", UNSUPPORTED, "due dimensioni diverse",
         "a table and a metric of different sizes are refused"},
        {"F_{ij} = \\begin{pmatrix} 1 & 2 \\\\ 3 & 4 \\end{pmatrix} \\implies \\delta_{ij} F^{ij} = 5", UNSUPPORTED, "serve una metrica",
         "without a metric a given tensor keeps the positions of its hypothesis"},
        {"A^{i} = (1, 2) \\implies \\delta_{11} A^{2} = 2", UNSUPPORTED, "può essere una potenza",
         "a numeric upper index on a given vector is still refused"},

        /* Gli indici greci e i valori che cominciano da 0 (37d, 37f, 39e): il
         * controesempio nomina gli indici come l'enunciato e conta i valori
         * dalla sua origine, anche negli indici delle componenti. */
        {"\\sum_{\\mu=0}^{3} \\delta_{\\mu\\mu} = 4", PROVED, "from $0$ to $3$", "a summation sign from 0 declares where the values start"},
        {"\\forall \\mu, \\nu \\in \\{0, 1, 2, 3\\}\\ (\\delta_{\\mu\\nu} = \\delta_{\\nu 0}\\delta_{\\mu 1})", REFUTED,
         "\\mu = 0, \\nu = 0: il primo membro vale 1, il secondo 0", "a counterexample names the Greek indices and counts their values from 0"},
        {"\\forall \\alpha \\in \\{0, 1, 2\\}\\ (\\varepsilon_{\\alpha\\beta\\gamma} a_{\\beta} b_{\\gamma} = 0)", REFUTED,
         "\\alpha = 0: il primo membro vale a_{1} b_{2} - a_{2} b_{1}, il secondo 0", "the components of a counterexample are numbered from 0 when the indices are"},
        {"i \\in \\{0, 1\\} \\land A_{i} = (a, 2 b) \\implies \\forall i \\in \\{0, 1\\}\\ (\\delta_{ij} A_{j} = A_{1}\\delta_{i0})", REFUTED,
         "i = 0: il primo membro vale a, il secondo 2 b", "the first element of a list is the component 0 when the indices start from 0"},
        {"\\mu \\in \\{0, 1, 2, 3\\} \\land g_{\\mu\\nu} = \\operatorname{diag}(1, -1, -1, -1) \\implies A^{\\mu} B_{\\mu} = A_{0} B_{0} - A_{1} B_{1} - A_{2} B_{2} - A_{3} B_{3}", PROVED,
         "for $i = 0, \\dots, 3$", "with indices from 0 to 3 the first number of the metric is the component 0"},
        {"\\mu \\in \\{0, 1, 2, 3\\} \\land g_{\\mu\\nu} = \\operatorname{diag}(1, -1, -1, -1) \\implies A^{\\mu} B_{\\mu} = -A_{0} B_{0} - A_{1} B_{1} - A_{2} B_{2} + A_{3} B_{3}", REFUTED,
         "il primo membro vale A^{0} B_{0} + A^{1} B_{1} + A^{2} B_{2} + A^{3} B_{3}", "the time index read as the last one is refuted"},
        {"g_{\\mu\\nu} = \\operatorname{diag}(1, -1, -1, -1) \\implies A^{\\mu} B_{\\mu} = A_{\\mu} B^{\\mu}", UNSUPPORTED, "va dichiarato da dove cominciano",
         "Greek indices without declared values are refused"},
        {"g_{ij} = \\operatorname{diag}(1, -1) \\implies A^{i} B_{i} = A_{0} B_{0} - A_{1} B_{1}", UNSUPPORTED, "lo deve dichiarare l'enunciato",
         "the index 0 without a declaration is refused"},
        {"\\mu \\in \\{0, 1, 2, 3\\} \\land g_{\\mu\\nu} = \\operatorname{diag}(1, -1, -1, -1) \\implies A^{\\mu} B_{\\mu} = A_{1} B_{1} - A_{2} B_{2} - A_{3} B_{3} - A_{4} B_{4}", UNSUPPORTED,
         "l'indice numerico 4 è fuori dai valori da 0 a 3", "an index from 1 to 4 under values declared from 0 to 3 is refused"},
        {"\\sum_{\\mu=0}^{3} \\sum_{\\nu=1}^{4} \\delta_{\\mu\\nu}\\delta_{\\mu\\nu} = 4", UNSUPPORTED, "a partire da 0 e a partire da 1",
         "two declarations that start from different values are refused"},
        {"\\varepsilon_{\\alpha\\beta\\gamma\\delta}\\varepsilon_{\\alpha\\beta\\gamma\\delta} = 24", UNSUPPORTED, "come indici non si usano",
         "delta is a symbol of the language, not an index"},

        /* Un enunciato sulle matrici senza notazione con gli indici resta del
         * capitolo 32, anche ora che una tabella può stare tra le ipotesi di
         * un'identità con gli indici. */
        {"\\begin{pmatrix} 1 & 2 \\\\ 3 & 4 \\end{pmatrix} \\begin{pmatrix} 0 & 1 \\\\ 1 & 0 \\end{pmatrix} = \\begin{pmatrix} 2 & 1 \\\\ 4 & 3 \\end{pmatrix}", PROVED, "",
         "a statement on matrices without index notation is still read by chapter 32"},

        /* Gli enunciati che gli altri linguaggi leggevano restano loro: una
         * lettera greca dichiarata con \colon è un'applicazione. */
        {"\\delta \\colon S \\to T \\implies \\forall x \\in S\\ (\\delta(x) \\in T)", PROVED, "", "a declared map named delta is still read by chapter 28"},
        {"\\delta_{1} \\colon S \\to T \\implies \\forall x \\in S\\ (\\delta_{1}(x) \\in T)", PROVED, "",
         "a declared map named delta with a numeric subscript is still read by chapter 28"},
        {"\\forall x \\in \\{1, 2, 3\\}\\ (x = 1 \\lor x = 2 \\lor x = 3)", PROVED, "", "a quantifier over listed numbers without index symbols is still read by chapter 28"},
    };
    char message[512];
    int passed = 1;
    engine_init();

    /* I file di dati. */
    passed = check_all_proved("tests/tensor_calculus/index_identities.tex") && passed;
    passed = check_certified("tests/tensor_calculus/index_identities.tex", 1, 77) && passed;
    passed = check_certified("tests/tensor_calculus/false_index_identities.tex", 0, 46) && passed;
    passed = check_all_unsupported("tests/tensor_calculus/index_notation_beyond.tex", 77) && passed;
    passed = check_all_proved("tests/tensor_calculus/metric_identities.tex") && passed;
    passed = check_certified("tests/tensor_calculus/metric_identities.tex", 1, 35) && passed;
    passed = check_certified("tests/tensor_calculus/false_metric_identities.tex", 0, 18) && passed;
    passed = check_all_proved("tests/tensor_calculus/given_components.tex") && passed;
    passed = check_certified("tests/tensor_calculus/given_components.tex", 1, 45) && passed;
    passed = check_certified("tests/tensor_calculus/false_given_components.tex", 0, 36) && passed;
    passed = check_all_proved("tests/tensor_calculus/index_ranges.tex") && passed;
    passed = check_certified("tests/tensor_calculus/index_ranges.tex", 1, 25) && passed;
    passed = check_certified("tests/tensor_calculus/false_index_ranges.tex", 0, 16) && passed;

    /* I casi scritti qui sopra. */
    for (size_t i = 0; i < sizeof cases / sizeof *cases; i++) {
        Verdict v = prove(cases[i].statement, 'n', 0, 0);
        const char *text = v.outcome == PROVED ? v.proof : v.outcome == REFUTED ? v.detail : v.reason;
        int certified = (v.outcome != PROVED && v.outcome != REFUTED) || v.certificate != NULL;
        snprintf(message, sizeof message, "%s", cases[i].name);
        passed = check_result(v.outcome == cases[i].outcome && text && strstr(text, cases[i].piece) && certified, message) && passed;
    }

    /* La forma del certificato, che è anche documentazione: l'enunciato con
     * l'operatore davanti, la dimensione, ciò che si afferma. */
    Verdict v = prove("\\varepsilon_{ijk}\\varepsilon_{imn} = \\delta_{jm}\\delta_{kn} - \\delta_{jn}\\delta_{km}", 'n', 0, 0);
    passed = check_result(v.certificate && strcmp(v.certificate, "certificate indices\ndimension 3\n"
                          "statement (= (* (eps i j k) (eps i m n)) (- (* (delta j m) (delta k n)) (* (delta j n) (delta k m))))\nholds\nend\n") == 0,
                          "the certificate of a proved identity is the statement, the dimension and the claim") && passed;
    v = prove("g_{ij} = \\operatorname{diag}(-1, 1) \\implies \\forall i \\in \\{1, 2\\}\\ (g_{ij} A^{j} = A_{i})", 'n', 0, 0);
    passed = check_result(v.certificate && strcmp(v.certificate, "certificate indices\ndimension 2\nmetric -1 1\n"
                          "statement (= (* (metric i j) (t A j)) (* (metric i a) (t A a)))\nholds\nend\n") == 0,
                          "the certificate of an identity with a metric is the unfolded identity: a lowered index is a contraction with the metric") && passed;
    /* Le componenti date vanno nel certificato come dati, una voce given per
     * tensore, per righe; i valori degli indici vi vanno sempre da 1 a d,
     * anche quando l'enunciato li conta da 0 (A_{1} è (t A 2), e il punto
     * i = 0 è "fails i 1"). */
    v = prove("g_{ij} = \\operatorname{diag}(2, 3) \\land T_{ij} = \\begin{pmatrix} 1 & 2 \\\\ 3 & 4 \\end{pmatrix} \\implies T^{i}{}_{i} = \\frac{11}{6}", 'n', 0, 0);
    passed = check_result(v.certificate && strcmp(v.certificate, "certificate indices\ndimension 2\nmetric 2 3\ngiven T 2 1 2 3 4\n"
                          "statement (= (* (inverse i a) (t T a i)) (frac 11 6))\nholds\nend\n") == 0,
                          "the certificate of a statement with a given table carries its components, by rows") && passed;
    v = prove("i \\in \\{0, 1\\} \\land A_{i} = (a, 2 b) \\implies \\forall i \\in \\{0, 1\\}\\ (\\delta_{ij} A_{j} = A_{1}\\delta_{i0})", 'n', 0, 0);
    passed = check_result(v.certificate && strcmp(v.certificate, "certificate indices\ndimension 2\ngiven A 1 (t a) (* 2 (t b))\n"
                          "statement (= (* (delta i j) (t A j)) (* (t A 2) (delta i 1)))\nfails i 1\nleft (* 1 (t a))\nright (* 2 (t b))\nend\n") == 0,
                          "in a certificate the values of the indices go from 1 to d also when the statement counts them from 0") && passed;
    v = prove("\\varepsilon_{ijk} a_{j} b_{k} = \\frac{1}{2} a_{i}", 'n', 0, 0);
    passed = check_result(v.certificate && strcmp(v.certificate, "certificate indices\ndimension 3\n"
                          "statement (= (* (* (eps i j k) (t a j)) (t b k)) (* (frac 1 2) (t a i)))\nfails i 1\n"
                          "left (+ (* (* 1 (t a 2)) (t b 3)) (* (* -1 (t a 3)) (t b 2)))\nright (* (frac 1 2) (t a 1))\nend\n") == 0,
                          "the certificate of a refuted identity carries the point and the two values") && passed;

    /* Tre matrici in tre dimensioni sono 27 componenti: più delle 24
     * indeterminate dei polinomi comuni del verificatore, che per questo
     * capitolo ha polinomi suoi. Prima il verdetto era NON DIMOSTRATO. */
    v = prove("\\varepsilon_{123} A_{ij} B_{jk} C_{ki} = B_{jk} C_{ki} A_{ij}", 'n', 0, 0);
    passed = check_result(v.outcome == PROVED && v.certificate, "an identity with 27 different components is proved and certified") && passed;
    v = prove("\\varepsilon_{123} A_{ij} B_{jk} C_{ki} = A_{ij} C_{jk} B_{ki}", 'n', 0, 0);
    passed = check_result(v.outcome == REFUTED && v.certificate, "a false identity with 27 different components is refuted and certified") && passed;
    v = prove("\\varepsilon_{1234} A_{ij} B_{jk} C_{kl} = A_{ij} (B_{jk} C_{kl})", 'n', 0, 0);
    passed = check_result(v.outcome == PROVED && v.certificate, "an identity with 48 different components is proved and certified") && passed;

    /* Limiti dichiarati, e onesti: un conto più lungo del budget, o un
     * prodotto di più componenti diverse di quante ne tiene un monomio del
     * motore (nove scalari), non danno mai un verdetto senza certificato. */
    long long budget = model_step_limit;
    model_step_limit = 10;
    v = prove("\\varepsilon_{ijk}\\varepsilon_{imn} = \\delta_{jm}\\delta_{kn} - \\delta_{jn}\\delta_{km}", 'n', 0, 0);
    passed = check_result(v.outcome == NOT_PROVED && v.reason && strstr(v.reason, "budget"), "a count longer than the work budget ends as not proved") && passed;

    /* Il motore finisce il conto e il verificatore no. Otto delta in cerchio:
     * il motore salta gli addendi nulli (ogni delta fissa l'indice che le
     * resta) e fa poche decine di passi; il verificatore somma tutto, quattro
     * valori per otto indici, e con due volte e mezza il budget del motore
     * non arriva in fondo. Il conto del motore è giusto, ma senza la
     * conferma indipendente non c'è verdetto. */
    model_step_limit = 1000;
    v = prove("\\sum_{i=1}^{4} \\delta_{ij}\\delta_{jk}\\delta_{kl}\\delta_{lm}\\delta_{mn}\\delta_{np}\\delta_{pq}\\delta_{qi} = 4", 'n', 0, 0);
    passed = check_result(v.outcome == NOT_PROVED && v.reason && strstr(v.reason, "non sa controllare il certificato") && !v.certificate,
                          "an identity the engine decides and the checker cannot recheck within its budget is not proved") && passed;
    model_step_limit = budget;
    v = prove("\\sum_{i=1}^{4} \\delta_{ij}\\delta_{jk}\\delta_{kl}\\delta_{lm}\\delta_{mn}\\delta_{np}\\delta_{pq}\\delta_{qi} = 4", 'n', 0, 0);
    passed = check_result(v.outcome == PROVED && v.certificate, "the same identity is proved and certified with the default budget") && passed;
    v = prove("\\varepsilon_{123} a b c d e f g h p = \\varepsilon_{123} p h g f e d c b a", 'n', 0, 0);
    passed = check_result(v.outcome == NOT_PROVED && v.reason && strstr(v.reason, "troppi fattori") && !v.certificate,
                          "a true identity beyond the limits of the engine is not proved, and says why") && passed;
    v = prove("\\varepsilon_{123} a b c d e f g h p = 2\\varepsilon_{123} p h g f e d c b a", 'n', 0, 0);
    passed = check_result(v.outcome == NOT_PROVED && !v.certificate, "a false identity beyond the limits of the engine is not refuted") && passed;

    /* Nessun passo scritto ha fallito il suo controllo: né una rilettura dei
     * valori, né un certificato respinto. */
    passed = check_result(unjustified_steps == 0, "every written value was reread and no certificate was rejected") && passed;
    return passed ? 0 : 1;
}
