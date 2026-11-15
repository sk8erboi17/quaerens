/* Test di dominio: i sistemi lineari a coefficienti razionali (paragrafo 31s
 * di parser.c, capitolo 33 di main.c, paragrafo 17f di checker.c).
 *
 * Nove controlli. Il primo è quello di tests/statement_checks.h:
 *
 *   1. ogni enunciato di book_systems.tex viene dimostrato, con la sua
 *      dimostrazione scritta: sono gli insiemi delle soluzioni stampati come
 *      risultato in una raccolta di esercizi, i sistemi impossibili, i
 *      vettori che sono una soluzione;
 *   2. ognuno di quei verdetti porta un certificato che il verificatore,
 *      rileggendolo qui, accetta; lo stesso certificato con la verità
 *      rovesciata lo respinge; e non lo accetta più con un testimone tolto
 *      o alterato (la combinazione delle equazioni e le matrici G e T
 *      azzerate, una componente di una soluzione cambiata);
 *   3. ogni enunciato di false_book_systems.tex viene dichiarato FALSO, non
 *      solo "non dimostrato": sono gli stessi risultati sbagliati di poco
 *      (una componente, un segno, un parametro dimenticato o di troppo, un
 *      sistema possibile detto impossibile e viceversa), e il conto li deve
 *      smentire tutti, con il certificato e gli stessi controlli del punto 2;
 *   4. ogni enunciato di book_systems.tex con la relazione rovesciata (=
 *      al posto di \neq, \in al posto di \notin, e viceversa) viene
 *      dichiarato FALSO: è la negazione di un enunciato vero, scritta senza
 *      cambiare i numeri;
 *   5. ogni richiesta di system_requests.tex viene risolta, e la risposta è
 *      quella attesa, scritta nello stesso modo: una soluzione particolare
 *      più i parametri liberi;
 *   6. gli enunciati di special_cases[] (le convenzioni di lettura, le
 *      equazioni che non sono lineari, i vettori della lunghezza sbagliata,
 *      il sistema scritto con un vettore per incognita, i limiti del conto)
 *      ricevono l'esito atteso, con il motivo atteso;
 *   7. un enunciato troppo lungo viene rifiutato, non letto a metà;
 *   8. gli enunciati di altri domini che scrivono \mathbb{R} con un
 *      esponente (la definizione di una relazione tra numeri reali, una
 *      disequazione sui reali positivi) restano dei loro capitoli;
 *   9. nessun passo scartato dai controlli indipendenti: né la rilettura dei
 *      coefficienti, né i prodotti che ricontrollano i testimoni, né la
 *      rilettura di una risposta, né il verificatore.
 *
 * Il metodo sotto esame è quello del capitolo 33: i coefficienti letti da
 * ogni equazione come da una forma di primo grado, l'eliminazione di Gauss
 * e i testimoni che lascia, la risposta di una richiesta riletta come
 * enunciato. I dati vengono dalle raccolte elencate in
 * book_systems_about.tex e sono stati ricalcolati da un oracolo indipendente
 * (tools/fuzz/fuzz_linear_algebra.py systems-data e systems-answers), non
 * dall'uscita del programma: il motore non ne conosce nessuno in anticipo. I
 * certificati scritti a mano, giusti e sbagliati, sono in
 * tests/checker/test_systems.c.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_linear_systems \
 *         tests/linear_algebra/test_linear_systems.c && /tmp/test_linear_systems
 */

#include "../statement_checks.h"

/* Le risposte attese delle richieste di system_requests.tex, nell'ordine
 * del file. Le incognite libere sono quelle senza perno nella forma ridotta,
 * cioè le ultime che le equazioni non fissano: la soluzione particolare le
 * ha a zero, e ogni vettore ne ha una a 1. Ogni risposta è stata confrontata
 * con il risultato stampato nella raccolta da un oracolo indipendente
 * (tools/fuzz/fuzz_linear_algebra.py systems-answers). */
static const char *const expected_answers[] = {
    "\\{(\\frac{5}{2}, 0)\\}",
    "\\{(\\frac{3}{7}, \\frac{26}{7})\\}",
    "\\emptyset",
    "\\{t (\\frac{1}{3}, 1) \\mid t \\in \\mathbb{R}\\}",
    "\\{(\\frac{4}{5}, 0, \\frac{1}{5}) + t (-1, 1, 0) \\mid t \\in \\mathbb{R}\\}",
    "\\emptyset",
    "\\{(-\\frac{1}{2}, \\frac{3}{2})\\}",
    "\\{(\\frac{5}{2}, 0, 0) + t (-\\frac{3}{2}, 1, 0) \\mid t \\in \\mathbb{R}\\}",
    "\\{t (\\frac{1}{3}, \\frac{4}{9}, \\frac{2}{3}, 1) \\mid t \\in \\mathbb{R}\\}",
    "\\{t (-\\frac{1}{2}, -\\frac{1}{2}, 1) \\mid t \\in \\mathbb{R}\\}",
    "\\{t (\\frac{3}{4}, \\frac{3}{8}, -\\frac{23}{8}, 1) \\mid t \\in \\mathbb{R}\\}",
    "\\{(-\\frac{5}{4}, -\\frac{9}{8}, \\frac{61}{8}, 0) + t (\\frac{3}{4}, \\frac{3}{8}, -\\frac{23}{8}, 1) \\mid t \\in \\mathbb{R}\\}",
    "\\{(\\frac{1}{3}, 0, -\\frac{2}{3}) + t (-1, 1, 0) \\mid t \\in \\mathbb{R}\\}",
    "\\emptyset",
    "\\{(0, -9, 0, 0) + t (\\frac{3}{4}, \\frac{1}{2}, 1, 0) + s (\\frac{1}{4}, -\\frac{7}{2}, 0, 1) \\mid t, s \\in \\mathbb{R}\\}",
    "\\{s (1, 1, 0, 0) + u (-2, 0, 0, 1) \\mid s, u \\in \\mathbb{R}\\}",
    "\\{(\\frac{2}{3}, -\\frac{11}{3}, -\\frac{10}{3})\\}",
    "\\{(2, 0, 0) + t (1, 1, 0) + s (-1, 0, 1) \\mid t, s \\in \\mathbb{R}\\}",
    "\\{(\\frac{5}{2}, 0)\\}",
    "\\{t (-2, 1) \\mid t \\in \\mathbb{R}\\}",
};

/* Un enunciato scritto apposta per mettere alla prova una convenzione o un
 * limite, e l'esito che deve ricevere. */
typedef struct {
    const char *name;       /* che cosa mette alla prova, in inglese */
    const char *statement;  /* l'enunciato, in LaTeX */
    Outcome expected;       /* l'esito atteso */
    const char *piece;      /* un pezzo del motivo per NON SUPPORTATO e NON
                               DIMOSTRATO, della nota per FALSO; la risposta
                               intera per RISOLTO; "" se non c'è nulla da
                               guardare */
} SpecialCase;

/* I pezzi usati più volte nei casi qui sotto: un sistema con una soluzione,
 * (2, -1), e uno con una retta di soluzioni, (t, 3t). */
#define XY "\\{(x, y) \\in \\mathbb{R}^{2} \\mid "
#define XYZ "\\{(x, y, z) \\in \\mathbb{R}^{3} \\mid "
#define ONE XY "x + y = 1 \\land x - y = 3\\}"
#define LINE XY "3x - y = 0\\}"
#define M22 "\\begin{pmatrix} 1 & 1 \\\\ 1 & -1 \\end{pmatrix}"
#define COL "\\begin{pmatrix} 1 \\\\ 3 \\end{pmatrix}"

static const SpecialCase special_cases[] = {
    /* Come si scrivono le equazioni (31s). */
    {"equations joined by commas", XY "x + y = 1, x - y = 3\\}", SOLVED, "\\{(2, -1)\\}"},
    {"equations joined by \\wedge", XY "x + y = 1 \\wedge x - y = 3\\}", SOLVED, "\\{(2, -1)\\}"},
    {"equations in cases, aligned with & and with a line break after the last", XY "\\begin{cases} x + y &= 1 \\\\ x - y &= 3 \\\\ \\end{cases}\\}", SOLVED, "\\{(2, -1)\\}"},
    {"a colon in place of \\mid", "\\{(x, y) \\in \\mathbb{R}^{2} : x + y = 1 \\land x - y = 3\\}", SOLVED, "\\{(2, -1)\\}"},
    {"the exponent of R without braces", "\\{(x, y) \\in \\mathbb{R}^2 \\mid x + y = 1 \\land x - y = 3\\}", SOLVED, "\\{(2, -1)\\}"},
    {"unknowns with subscripts without braces", "\\{(x_1, x_2) \\in \\mathbb{R}^{2} \\mid x_1 + x_2 = 1 \\land x_1 - x_2 = 3\\}", SOLVED, "\\{(2, -1)\\}"},
    {"the same letter with two subscripts, and without one", "\\{(a, a_{1}, a_{2}) \\in \\mathbb{R}^{3} \\mid a = 1 \\land a_{1} = 2 \\land a_{2} = a + a_{1}\\}", SOLVED, "\\{(1, 2, 3)\\}"},
    {"the order of the unknowns is the declared one", "\\{(y, x) \\in \\mathbb{R}^{2} \\mid x + y = 1 \\land x - y = 3\\}", SOLVED, "\\{(-1, 2)\\}"},
    {"unknowns on both sides, and a constant on the left", XY "x + y - 1 = 0 \\land 2x = x + y + 3\\}", SOLVED, "\\{(2, -1)\\}"},
    {"coefficients written with arithmetic", XY "\\frac{x}{2} + 2^{2} y = -3 \\land \\frac{1}{3} (x - y) = 1\\}", SOLVED, "\\{(2, -1)\\}"},
    {"a number that multiplies a sum of unknowns on the right", XY "(x + y) \\cdot 2 = 2 \\land x - y = 3\\}", SOLVED, "\\{(2, -1)\\}"},
    {"an unknown multiplied by zero", XY "x + y = 1 \\land x - y + 0 \\cdot x = 3\\}", SOLVED, "\\{(2, -1)\\}"},
    {"an unknown that the equations do not mention is free", XYZ "x + y = 1 \\land x - y = 3\\}", SOLVED, "\\{(2, -1, 0) + t (0, 0, 1) \\mid t \\in \\mathbb{R}\\}"},
    {"no condition on any unknown", XY "0 = 0\\}", SOLVED, "\\{t (1, 0) + s (0, 1) \\mid t, s \\in \\mathbb{R}\\}"},
    {"an equation that no vector satisfies", XY "0 = 1\\}", SOLVED, "\\emptyset"},
    {"an equation repeated", XY "x + y = 1 \\land x + y = 1 \\land x - y = 3\\}", SOLVED, "\\{(2, -1)\\}"},
    {"the letters of the answer avoid the unknowns", "\\{(t, s) \\in \\mathbb{R}^{2} \\mid 3t - s = 0\\}", SOLVED, "\\{u (\\frac{1}{3}, 1) \\mid u \\in \\mathbb{R}\\}"},

    /* Ciò che non è un sistema lineare, o non si legge. */
    {"a product of two unknowns", XY "x y = 1 \\land x - y = 3\\}", UNSUPPORTED, "primo grado"},
    {"the square of an unknown", XY "x^{2} + y = 1\\}", UNSUPPORTED, "primo grado"},
    {"an unknown in a denominator", XY "\\frac{1}{x} + y = 1\\}", UNSUPPORTED, "primo grado"},
    {"the square root of an unknown", XY "\\sqrt{x} + y = 1\\}", UNSUPPORTED, "primo grado"},
    {"a function of an unknown", XY "\\sin x + y = 1\\}", UNSUPPORTED, "primo grado"},
    {"a letter that is not an unknown", XY "x + k y = 1 \\land x - y = 3\\}", UNSUPPORTED, "una lettera solo se"},
    {"a square root among the coefficients", XY "\\sqrt{2} x + y = 1\\}", UNSUPPORTED, "radici"},
    {"an inequality in a system", XY "x - y \\ge 0 \\land 2x + y - 3 < 0\\}", UNSUPPORTED, ""},
    {"an unknown declared twice", "\\{(x, x) \\in \\mathbb{R}^{2} \\mid x = 1\\}", UNSUPPORTED, ""},
    {"more unknowns than the exponent says", "\\{(x, y, z) \\in \\mathbb{R}^{2} \\mid x + y = 1\\}", UNSUPPORTED, ""},
    {"one unknown is an equation, not a system", "\\{(x) \\in \\mathbb{R}^{1} \\mid 2x = 4\\}", UNSUPPORTED, ""},
    {"two sets of solutions compared", ONE " = " ONE, UNSUPPORTED, "un insieme scritto va chiuso"},
    {"the written set in front", "\\{(2, -1)\\} = " ONE, UNSUPPORTED, ""},
    {"text after the statement", ONE " = \\{(2, -1)\\} \\land 1 = 1", UNSUPPORTED, "testo inatteso"},
    {"a set of solutions that is not closed", XY "x + y = 1", UNSUPPORTED, "va chiuso"},
    {"another environment in place of cases", XY "\\begin{aligned} x + y = 1 \\\\ x - y = 3 \\end{aligned}\\}", UNSUPPORTED, "cases"},
    {"numbers beyond the exact arithmetic", XY "99999999999 x + 3y = 1 \\land 7x + 99999999999 y = 1 \\land 99999999999 x + 99999999998 y = 5\\}", UNSUPPORTED, "numeri troppo grandi"},

    /* L'insieme scritto. */
    {"a written set with the right vectors in another form", LINE " = \\{(1, 3) + t (2, 6) \\mid t \\in \\mathbb{R}\\}", PROVED, ""},
    {"a written set with a parameter too many", LINE " = \\{s (1, 3) + t (2, 6) \\mid s, t \\in \\mathbb{R}\\}", PROVED, ""},
    {"a written set with a vector subtracted, and a number in front", LINE " = \\{(1, 3) - 2t (\\frac{1}{2}, \\frac{3}{2}) \\mid t \\in \\mathbb{R}\\}", PROVED, ""},
    {"a written set with a fraction of the parameter in front of the vector", LINE " = \\{\\frac{t}{2} \\cdot (2, 6) \\mid t \\in \\mathbb{R}\\}", PROVED, ""},
    {"a written set with the parameters inside the components", XYZ "x + y + z = 1\\} = \\{(1 - s - t, s, t) \\mid s, t \\in \\mathbb{R}\\}", PROVED, ""},
    {"a parameter with the name of an unknown is another letter", "\\{(x, t) \\in \\mathbb{R}^{2} \\mid x + t = 1\\} = \\{(1 - t, t) \\mid t \\in \\mathbb{R}\\}", PROVED, ""},
    {"a parameter with a subscript", LINE " = \\{(t_{1}, 3 t_{1}) \\mid t_{1} \\in \\mathbb{R}\\}", PROVED, ""},
    {"a parameter that the written vectors do not use", ONE " = \\{(2, -1) \\mid t \\in \\mathbb{R}\\}", PROVED, ""},
    {"the empty set written \\varnothing", XY "0 = 1\\} = \\varnothing", PROVED, ""},
    {"a point of the line is not the line", LINE " = \\{(1, 3)\\}", REFUTED, "che l'insieme scritto non contiene"},
    {"the line is not one of its points", LINE " \\neq \\{(1, 3)\\}", PROVED, ""},
    {"a set that contains the solutions and more", ONE " = \\{(2, -1) + t (1, 0) \\mid t \\in \\mathbb{R}\\}", REFUTED, "non risolve il sistema omogeneo"},
    {"a written vector with a component too many", ONE " = \\{(2, -1, 0)\\}", UNSUPPORTED, "tante componenti"},
    {"a written vector with a component too few", XYZ "x + y + z = 1\\} = \\{(1, 0)\\}", UNSUPPORTED, "tante componenti"},
    {"vectors of different lengths in one sum", ONE " = \\{(2, -1) + t (1, 0, 0) \\mid t \\in \\mathbb{R}\\}", UNSUPPORTED, "stesso numero di componenti"},
    {"a letter of the written set that is not declared as a parameter", LINE " = \\{(t, 3t)\\}", UNSUPPORTED, "una lettera solo se"},
    {"an unknown of the system inside the written set", LINE " = \\{(x, 3x)\\}", UNSUPPORTED, "una lettera solo se"},
    {"the square of a parameter", LINE " = \\{(t^{2}, 3t^{2}) \\mid t \\in \\mathbb{R}\\}", UNSUPPORTED, "primo grado"},
    {"a product of two parameters", LINE " = \\{s t (1, 3) \\mid s, t \\in \\mathbb{R}\\}", UNSUPPORTED, "primo grado"},
    {"a parameter declared twice", LINE " = \\{(t, 3t) \\mid t, t \\in \\mathbb{R}\\}", UNSUPPORTED, "due volte"},
    {"a parameter in another set", LINE " = \\{(t, 3t) \\mid t \\in \\mathbb{Q}\\}", UNSUPPORTED, "\\mathbb{R}"},
    {"a number in parentheses is not a vector", ONE " = \\{(2)\\}", UNSUPPORTED, "un vettore si scrive tra parentesi"},

    /* Un vettore e la sua appartenenza. */
    {"a vector written as a sum", "(1, 0) + (1, -1) \\in " ONE, PROVED, ""},
    {"a vector written with a number in front", "2 (1, -\\frac{1}{2}) \\in " ONE, PROVED, ""},
    {"a vector that satisfies the first equation only", "(1, 0) \\in " ONE, REFUTED, "l'equazione numero 2"},
    {"and saying so is true", "(1, 0) \\notin " ONE, PROVED, ""},
    {"a vector of the wrong length is not compared", "(2, -1, 0) \\in " ONE, UNSUPPORTED, "tante componenti"},
    {"a vector with a letter", "(t, 3t) \\in " LINE, UNSUPPORTED, "una lettera solo se"},

    /* Il sistema scritto con un vettore per incognita, A x = B. */
    {"a system written with a matrix", "\\{x \\in \\mathbb{R}^{2} \\mid " M22 " x = " COL "\\}", SOLVED, "\\{(2, -1)\\}"},
    {"the unknown on the right", "\\{x \\in \\mathbb{R}^{2} \\mid " COL " = " M22 " x\\}", SOLVED, "\\{(2, -1)\\}"},
    {"the unknown on both sides", "\\{x \\in \\mathbb{R}^{2} \\mid " M22 " x + x = " COL " + x\\}", SOLVED, "\\{(2, -1)\\}"},
    {"a number in front of the product", "\\{x \\in \\mathbb{R}^{2} \\mid 2 " M22 " x = 2 " COL "\\}", SOLVED, "\\{(2, -1)\\}"},
    {"the vector multiplied by a number", "\\{x \\in \\mathbb{R}^{2} \\mid 3 x = " COL "\\}", SOLVED, "\\{(\\frac{1}{3}, 1)\\}"},
    {"an identity without its size beside the unknown: the eigenvectors of an eigenvalue",
     "\\{x \\in \\mathbb{R}^{2} \\mid (\\begin{pmatrix} 2 & 1 \\\\ 1 & 2 \\end{pmatrix} - I) x = 0\\}", SOLVED, "\\{t (-1, 1) \\mid t \\in \\mathbb{R}\\}"},
    {"the identity alone takes its size from the unknown", "\\{x \\in \\mathbb{R}^{2} \\mid I x = " COL "\\}", SOLVED, "\\{(1, 3)\\}"},
    {"0 alone is the zero column", "\\{x \\in \\mathbb{R}^{2} \\mid " M22 " x = 0\\}", SOLVED, "\\{(0, 0)\\}"},
    {"an inverse and a determinant among the coefficients", "\\{x \\in \\mathbb{R}^{2} \\mid " M22 "^{-1} x = \\det " M22 " \\cdot " COL "\\}", SOLVED, "\\{(-8, 4)\\}"},
    {"a matrix with a name", "A = " M22 ", B = " COL " \\implies \\{x \\in \\mathbb{R}^{2} \\mid A x = B\\} = \\{(2, -1)\\}", PROVED, ""},
    {"a matrix with three columns for a vector of two components", "\\{x \\in \\mathbb{R}^{2} \\mid \\begin{pmatrix} 1 & 1 & 1 \\end{pmatrix} x = \\begin{pmatrix} 1 \\end{pmatrix}\\}", UNSUPPORTED, "prodotto di due matrici non definito"},
    {"two sides of different shapes", "\\{x \\in \\mathbb{R}^{2} \\mid " M22 " x = \\begin{pmatrix} 1 & 3 \\end{pmatrix}\\}", UNSUPPORTED, "stesse dimensioni"},
    {"the unknown on the left of the matrix: a column times a matrix", "\\{x \\in \\mathbb{R}^{2} \\mid x " M22 " = " COL "\\}", UNSUPPORTED, "prodotto di due matrici non definito"},
    {"the unknown twice in a product", "\\{x \\in \\mathbb{R}^{2} \\mid x^{T} " M22 " x = \\begin{pmatrix} 1 \\end{pmatrix}\\}", UNSUPPORTED, ""},
    {"the unknown divided by a number", "\\{x \\in \\mathbb{R}^{2} \\mid " M22 " \\frac{x}{2} = " COL "\\}", UNSUPPORTED, "va scritto da solo"},
    {"the unknown as an entry of a matrix", "\\{x \\in \\mathbb{R}^{2} \\mid \\begin{pmatrix} x & 1 \\\\ 1 & 1 \\end{pmatrix} = " M22 "\\}", UNSUPPORTED, "primo grado"},
    {"a vector unknown compared with a number", "\\{x \\in \\mathbb{R}^{2} \\mid x = 1\\}", UNSUPPORTED, ""},
    {"equations between numbers with a vector unknown", "\\{x \\in \\mathbb{R}^{2} \\mid 2 x = 4 \\land 3 x = 6\\}", UNSUPPORTED, ""},
    {"a vector unknown with a subscript", "\\{x_{1} \\in \\mathbb{R}^{2} \\mid " M22 " x_{1} = " COL "\\}", UNSUPPORTED, ""},
    {"a definition that uses the unknown", "A = x " M22 " \\implies \\{x \\in \\mathbb{R}^{2} \\mid A x = " COL "\\}", UNSUPPORTED, ""},
    {"a matrix that is not defined", "\\{x \\in \\mathbb{R}^{2} \\mid A x = " COL "\\}", UNSUPPORTED, "va definita"},
};

/* Il nome di un esito, per i messaggi. */
static const char *outcome_name(Outcome outcome) {
    static const char *const names[] = {[PROVED] = "PROVED", [REFUTED] = "REFUTED", [NOT_PROVED] = "NOT_PROVED",
                                        [UNSUPPORTED] = "UNSUPPORTED", [SOLVED] = "SOLVED"};
    return names[outcome];
}

/* Mette in out, di size byte, il certificato text con la verità rovesciata:
 * "claim true" al posto di "claim false", o viceversa. Ritorna 1 se la voce
 * c'era e il testo ci sta. */
static int flip_claim(const char *text, char *out, size_t size) {
    const char *claim = strstr(text, "\nclaim ");
    if (!claim || strlen(text) + 2 > size) return 0;
    int was_true = strncmp(claim + 7, "true", 4) == 0;
    snprintf(out, size, "%.*s\nclaim %s%s", (int)(claim - text), text, was_true ? "false" : "true", claim + 7 + (was_true ? 4 : 5));
    return 1;
}

/* Ritorna la risposta del verificatore al certificato text. */
static CkAnswer checker_answer(const char *text) {
    const char *why;
    return ck_check(&text, &why, 0);
}

/* Altera uno per volta i testimoni del certificato text, che il verificatore
 * accetta, e conta le alterazioni che non vanno come devono: deve essere
 * zero. Aggiunge a *tried quante ne ha provate. Ogni alterazione è scelta
 * perché NESSUN certificato giusto la sopporti (un testimone non è unico):
 *
 *   - una voce tolta: i testimoni non bastano più. Il verificatore può
 *     rispondere "non so controllarlo", mai sì;
 *   - combination y, con y azzerata: y b, o y (z - p), deve essere diverso
 *     da zero, e con y nulla è zero. Respinto;
 *   - span G T, con G e T azzerate: G A + U T deve essere l'identità, e con
 *     G e T nulle è la matrice nulla. Respinto;
 *   - solution z, con l'ultimo numero cambiato: A z cambia di un multiplo
 *     non nullo dell'ultima colonna di A, che in questi dati non è nulla
 *     perché ogni incognita compare in un'equazione. Respinto. */
static int altered_witnesses_wrong(const char *text, int *tried) {
    static char altered[CERTIFICATE_BYTES + 64];
    int wrong = 0;
    for (const char *line = strstr(text, "\nwitness "); line; line = strstr(line + 1, "\nwitness ")) {
        size_t start = (size_t)(line + 1 - text), end = (size_t)(strchr(line + 1, '\n') - text);
        int rows, cols, more_rows, more_cols, read;

        /* La voce tolta. */
        snprintf(altered, sizeof altered, "%.*s%s", (int)start, text, text + end + 1);
        (*tried)++;
        if (checker_answer(altered) == CK_ACCEPTED) wrong++;

        if (strncmp(text + start, "witness solution ", 17) == 0) {
            /* L'ultimo numero della soluzione: l'ultima cifra della riga. */
            size_t last = end;
            while (text[last] < '0' || text[last] > '9') last--;
            strcpy(altered, text);
            altered[last] = altered[last] == '9' ? '8' : (char)(altered[last] + 1);
        } else if (sscanf(text + start, "witness combination (matrix %d %d", &rows, &cols) == 2) {
            snprintf(altered, sizeof altered, "%.*switness combination (zero %d %d)%s", (int)start, text, rows, cols, text + end);
        } else if (sscanf(text + start, "witness span (matrix %d %d", &rows, &cols) == 2) {
            /* La seconda matrice, se c'è, comincia dopo la parentesi che
             * chiude la prima: le matrici di un testimone sono tabelle di
             * numeri, con parentesi solo attorno alle frazioni. */
            const char *second = text + start + strlen("witness span (");
            for (int depth = 1; depth > 0; second++) depth += *second == '(' ? 1 : *second == ')' ? -1 : 0;
            read = sscanf(second, " (matrix %d %d", &more_rows, &more_cols);
            if (read == 2) snprintf(altered, sizeof altered, "%.*switness span (zero %d %d) (zero %d %d)%s", (int)start, text, rows, cols, more_rows, more_cols, text + end);
            else snprintf(altered, sizeof altered, "%.*switness span (zero %d %d)%s", (int)start, text, rows, cols, text + end);
        } else {
            /* Un tipo di testimone che questo test non conosce. */
            wrong++;
            continue;
        }
        (*tried)++;
        if (checker_answer(altered) != CK_REJECTED) wrong++;
    }
    return wrong;
}

/* Controlli 2 e 3: ogni enunciato di path riceve l'esito expected (PROVED o
 * REFUTED) con un certificato che il verificatore, riletto qui, accetta; che
 * con la verità rovesciata respinge; e che con un testimone alterato non
 * accetta più. Ritorna 1 se il controllo è riuscito. */
static int check_certified(const char *path, Outcome expected) {
    static char copy[CERTIFICATE_BYTES], flipped[CERTIFICATE_BYTES];
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }
    const char *position = text, *item, *end;
    int total = 0, right = 0, altered = 0;
    while (next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        total++;
        if (verdict.outcome != expected || !verdict.certificate || strlen(verdict.certificate) >= sizeof copy) {
            printf("  statement %d of %s: expected %s with a certificate, got %s (%s)\n", total, path, outcome_name(expected),
                   outcome_name(verdict.outcome), verdict.reason ? verdict.reason : verdict.certificate ? "certificate too long for the test" : "no certificate");
            continue;
        }

        /* Il certificato va copiato: il verificatore lavora sul testo che
         * gli si dà, e quello del verdetto vale fino al verdetto dopo. */
        strcpy(copy, verdict.certificate);
        CkAnswer straight = checker_answer(copy);
        CkAnswer reversed = flip_claim(copy, flipped, sizeof flipped) ? checker_answer(flipped) : CK_ACCEPTED;
        int slipped = straight == CK_ACCEPTED ? altered_witnesses_wrong(copy, &altered) : 0;
        if (straight == CK_ACCEPTED && reversed == CK_REJECTED && !slipped) { right++; continue; }
        printf("  statement %d of %s: the certificate is %s, with the claim reversed it is %s, and %d altered witnesses are not refused as they should\n", total, path,
               straight == CK_ACCEPTED ? "accepted" : "not accepted", reversed == CK_REJECTED ? "rejected" : "not rejected", slipped);
    }
    free(text);
    snprintf(summary, sizeof summary,
             "%d of %d statements of %s are %s with a certificate that the checker accepts, rejects with the claim reversed, "
             "and no longer accepts with a witness removed or altered (%d alterations)",
             right, total, path, expected == PROVED ? "proved" : "declared false", altered);
    return check_result(total > 0 && right == total && altered > 0, summary);
}

/* Controllo 4: di ogni enunciato di path si scrive la negazione, con l'altra
 * relazione della sua coppia (= e \neq, \in e \notin), e la si fa decidere:
 * deve essere dichiarata falsa. Dov'è la relazione lo dice il lettore: sta
 * subito dopo l'insieme delle soluzioni, oppure subito prima. Ritorna 1 se
 * il controllo è riuscito. */
static int check_negations(const char *path) {
    static const char *const written[] = {[LINEAR_EQ] = "=", [LINEAR_NEQ] = "\\neq", [LINEAR_IN] = "\\in", [LINEAR_NOTIN] = "\\notin"};
    static const LinearRelation opposite[] = {[LINEAR_EQ] = LINEAR_NEQ, [LINEAR_NEQ] = LINEAR_EQ, [LINEAR_IN] = LINEAR_NOTIN, [LINEAR_NOTIN] = LINEAR_IN};
    static char negated[2 * MAX_FORMULA];
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }
    const char *position = text, *item, *end;
    int total = 0, right = 0;
    while (next_item(&position, &item, &end)) {
        Item statement;
        LinearStatement st;
        MatrixStatement names;
        if (read_item(item, end, &statement)) continue;
        const char *formula = statement.formula;
        arena_used = arena_keep;
        if (!parse_linear_statement(formula, &st, &names) || st.relation == LINEAR_ASKED) continue;

        /* La relazione scritta: dopo l'insieme, o come ultima cosa prima. */
        const char *relation = NULL, *word = written[st.relation];
        if (st.relation == LINEAR_EQ || st.relation == LINEAR_NEQ) relation = strstr(st.set_end, word);
        else for (const char *p = strstr(formula, word); p && p < st.set_start; p = strstr(p + 1, word)) relation = p;
        if (!relation) continue;
        snprintf(negated, sizeof negated, "%.*s%s%s", (int)(relation - formula), formula, written[opposite[st.relation]], relation + strlen(word));
        total++;
        Verdict verdict = prove(negated, 'n', 0, 0);
        if (verdict.outcome == REFUTED && verdict.certificate) { right++; continue; }
        printf("  the negation %s is %s\n", negated, outcome_name(verdict.outcome));
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d negations of the statements of %s are declared false, with a certificate", right, total, path);
    return check_result(total > 0 && right == total, summary);
}

/* Controllo 5: ogni richiesta di path è risolta, con la risposta attesa di
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
    snprintf(summary, sizeof summary, "%d of %d requests of %s solved with the expected set, each with a verified certificate", right, total, path);
    return check_result(total == expected && right == total, summary);
}

/* Controllo 6: ogni enunciato di special_cases[] riceve l'esito atteso, e il
 * testo che lo accompagna contiene il pezzo atteso. Ritorna 1 se il
 * controllo è riuscito. */
static int check_special_cases(void) {
    char summary[128];
    int total = (int)(sizeof special_cases / sizeof *special_cases), right = 0;
    for (int i = 0; i < total; i++) {
        const SpecialCase *c = &special_cases[i];
        Verdict verdict = prove(c->statement, 'n', 0, 0);
        const char *said = verdict.outcome == REFUTED ? verdict.note : verdict.outcome == SOLVED ? verdict.detail : verdict.reason;

        /* Un esito deciso vuole il certificato; una risposta è confrontata
         * per intero, gli altri testi per un pezzo. */
        int decided = verdict.outcome == PROVED || verdict.outcome == REFUTED || verdict.outcome == SOLVED;
        int text_right = !c->piece[0] || (said && (verdict.outcome == SOLVED ? strcmp(said, c->piece) == 0 : strstr(said, c->piece) != NULL));
        if (verdict.outcome == c->expected && text_right && (!decided || verdict.certificate)) { right++; continue; }
        printf("  %s: expected %s (%s), got %s (%s)\n", c->name, outcome_name(c->expected), c->piece, outcome_name(verdict.outcome), said ? said : "");
    }
    snprintf(summary, sizeof summary, "%d of %d statements on reading, shapes and limits of linear systems answered as expected", right, total);
    return check_result(right == total, summary);
}

/* Controllo 7: un enunciato su un sistema più lungo di MAX_FORMULA viene
 * rifiutato intero, con il suo motivo. Lo si costruisce ripetendo molte
 * volte la stessa equazione: è vero, e non deve essere né letto a metà né
 * deciso. Ritorna 1 se il controllo è riuscito. */
static int check_too_long(void) {
    static char statement[2 * MAX_FORMULA];
    size_t length = (size_t)snprintf(statement, sizeof statement, "%s", XY);
    while (length < MAX_FORMULA + 64) length += (size_t)snprintf(statement + length, sizeof statement - length, "x + y = 1 \\land ");
    snprintf(statement + length, sizeof statement - length, "x - y = 3\\} = \\{(2, -1)\\}");
    Verdict verdict = prove(statement, 'n', 0, 0);
    return check_result(verdict.outcome == UNSUPPORTED && verdict.reason && strstr(verdict.reason, "troppo lungo"),
                        "a statement on a system longer than the limit is refused whole, not read in part");
}

/* Controllo 8: gli enunciati di altri domini che scrivono \mathbb{R} con un
 * esponente restano dei loro capitoli. Antiregressione: la prima versione
 * del riconoscimento dei sistemi (uses_linear_systems()) si prendeva la
 * definizione di una relazione tra numeri reali, R = \{(a, b) \in
 * \mathbb{R}^{2} \mid ...\}, e un enunciato vero dei test sugli insiemi
 * risultava non supportato. Ritorna 1 se il controllo è riuscito. */
static int check_other_domains(void) {
    static const struct { const char *statement; Outcome expected; } cases[] = {
        {"R = \\{(a, b) \\in \\mathbb{R}^{2} \\mid a - b \\in \\mathbb{Q}\\} \\implies (\\forall x \\in \\mathbb{R}\\ ((x, x) \\in R))", PROVED},
        /* Falso, e il suo capitolo si ferma sulla divisione per zero: conta
         * che il motivo sia il suo, non quello di un sistema. */
        {"R = \\{(a, b) \\in \\mathbb{R}^{2} \\mid a b^{-1} \\in \\mathbb{Q}\\} \\implies (\\forall x \\in \\mathbb{R}\\ ((x, x) \\in R))", UNSUPPORTED},
        {"\\{x \\in \\mathbb{R}^{+} \\mid x^{2} \\le 4\\}", SOLVED},
    };
    char summary[160];
    int total = (int)(sizeof cases / sizeof *cases), right = 0;
    for (int i = 0; i < total; i++) {
        Verdict verdict = prove(cases[i].statement, 'n', 0, 0);
        if (verdict.outcome == cases[i].expected && verdict.method != LINEAR_METHOD) { right++; continue; }
        printf("  %s: expected %s from its own chapter, got %s (%s)\n", cases[i].statement, outcome_name(cases[i].expected), outcome_name(verdict.outcome),
               verdict.reason ? verdict.reason : "");
    }
    snprintf(summary, sizeof summary, "%d of %d statements of other domains that write R with an exponent are left to their chapters", right, total);
    return check_result(right == total, summary);
}

int main(void) {
    engine_init();

    /* Tutti i controlli vengono eseguiti anche se uno fallisce, così il
     * resoconto è completo. */
    int passed = check_all_proved("tests/linear_algebra/book_systems.tex");
    passed = check_certified("tests/linear_algebra/book_systems.tex", PROVED) && passed;
    passed = check_certified("tests/linear_algebra/false_book_systems.tex", REFUTED) && passed;
    passed = check_negations("tests/linear_algebra/book_systems.tex") && passed;
    passed = check_requests("tests/linear_algebra/system_requests.tex") && passed;
    passed = check_special_cases() && passed;
    passed = check_too_long() && passed;
    passed = check_other_domains() && passed;

    /* Ogni lettura ha superato il suo ricontrollo, ogni testimone i suoi
     * prodotti, ogni risposta la sua rilettura, e il verificatore non ha
     * respinto nessun certificato. */
    passed = check_result(unjustified_steps == 0, "no reading failed its check, no witness failed its products, no answer failed its reading, no certificate was rejected") && passed;
    return passed ? 0 : 1;
}
