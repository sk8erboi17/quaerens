/* Test di dominio: algebra lineare sulle matrici a elementi razionali, e sui
 * determinanti con una lettera (capitolo 31 di parser.c, capitolo 32 di
 * main.c, capitolo 17 di checker.c).
 *
 * Tredici controlli. Il primo, il sesto e il nono sono quelli di
 * tests/statement_checks.h:
 *
 *   1. ogni enunciato di book_matrices.tex viene dimostrato, con la sua
 *      dimostrazione scritta: sono prodotti, determinanti, ranghi e inverse
 *      con il risultato stampato in una raccolta di esercizi;
 *   2. ognuno di quei verdetti porta un certificato che il verificatore,
 *      rileggendolo qui, accetta; lo stesso certificato con la verità
 *      rovesciata (claim true al posto di claim false, o viceversa) lo
 *      respinge; e non lo accetta più con un testimone tolto o alterato (un
 *      numero dell'inversa o di U, il rango dichiarato più alto o più basso
 *      di uno, un elemento di C);
 *   3. ogni enunciato di false_book_matrices.tex viene dichiarato FALSO, non
 *      solo "non dimostrato": sono gli stessi risultati sbagliati di poco
 *      (un elemento, un segno, un rango o un determinante cambiati di uno),
 *      e un conto esatto li deve smentire tutti, con il certificato;
 *   4. ogni enunciato di book_matrices.tex con = al posto di \neq, o \neq al
 *      posto di =, viene dichiarato FALSO: è la negazione di un enunciato
 *      vero, scritta senza cambiare i numeri;
 *   5. ogni richiesta di matrix_requests.tex viene risolta, e la risposta è
 *      quella del libro, scritta nello stesso modo;
 *   6. nessun enunciato di matrices_beyond.tex viene dichiarato falso: sono
 *      veri, ma fuori dal linguaggio (radici, \pi, una lettera senza il suo
 *      insieme; con una lettera, il rango, l'inversa, un confronto tra
 *      matrici), e nessuno deve nemmeno risultare dimostrato;
 *   7. gli enunciati di special_cases[] (le convenzioni di lettura, la
 *      dimensione di I, lo 0 come matrice nulla, le operazioni non definite,
 *      i limiti del conto, ciò che con una lettera non si tratta ancora)
 *      ricevono l'esito atteso, con il motivo atteso;
 *   8. un enunciato troppo lungo viene rifiutato, non letto a metà;
 *   9. ogni enunciato di letter_determinants.tex viene dimostrato, con la
 *      sua dimostrazione scritta: determinanti e tracce con una lettera
 *      negli elementi, e la lettera nel suo insieme (per quali valori la
 *      matrice è singolare, quali sono gli autovalori, quale polinomio è il
 *      determinante);
 *  10. di quei verdetti, 35 portano i certificati, quello dell'enunciato che
 *      resta e quelli degli sviluppi, e il verificatore li accetta uno per
 *      uno e respinge ogni sviluppo con il polinomio alterato; gli altri 3
 *      (due con radici nel risultato, uno con due lettere) dicono che è
 *      certificato solo lo sviluppo;
 *  11. ogni enunciato di false_letter_determinants.tex viene dichiarato
 *      FALSO: gli stessi con il risultato sbagliato di poco (un valore
 *      spostato, dimenticato o in più, un coefficiente, un estremo compreso
 *      al posto di uno escluso), con i certificati in 33 casi su 34;
 *  12. ogni richiesta di letter_solutions.tex viene risolta con le soluzioni
 *      attese, e quelle soluzioni, rilette come enunciato "condizione se e
 *      solo se soluzioni", vengono dimostrate;
 *  13. nessun passo scartato dai controlli indipendenti: né i prodotti che
 *      ricontrollano i testimoni di un'inversa, di un determinante e di un
 *      rango, né la rilettura di una risposta, né il verificatore.
 *
 * Il metodo sotto esame è il conto esatto del capitolo 32: l'eliminazione
 * di Gauss sui razionali e i testimoni che lascia, la dimensione
 * dell'identità ricavata dal contesto, la risposta di una richiesta riletta
 * come enunciato; e, con una lettera, il determinante trovato come
 * polinomio dai suoi valori in abbastanza punti (32g) e consegnato alla
 * tabella dei segni. I dati vengono dalle raccolte elencate in
 * book_matrices_about.tex e in letter_determinants_about.tex e sono stati
 * ricalcolati da un oracolo indipendente (tools/fuzz/fuzz_linear_algebra.py
 * data e answers), non dall'uscita del programma: il motore non ne conosce
 * nessuno in anticipo. I certificati scritti a mano, giusti e sbagliati,
 * sono in tests/checker/test_matrices.c.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_linear_algebra \
 *         tests/linear_algebra/test_linear_algebra.c && /tmp/test_linear_algebra
 */

#include "../statement_checks.h"

/* Le risposte attese delle richieste di matrix_requests.tex, nell'ordine
 * del file: i risultati stampati nelle raccolte, scritti come li scrive il
 * motore (una matrice tra parentesi tonde, le frazioni ridotte con il segno
 * davanti). L'inversa del numero 22, che il libro scrive come un centesimo
 * di una matrice di interi, è qui con le frazioni sviluppate: che le due
 * scritture siano la stessa matrice lo dice l'enunciato 39 di
 * book_matrices.tex. */
static const char *const expected_answers[] = {
    "\\begin{pmatrix} -2 & 4 & \\frac{1}{2} \\\\ -1 & 2 & \\frac{1}{2} \\\\ 2 & -3 & -\\frac{1}{2} \\end{pmatrix}",
    "\\begin{pmatrix} 0 & -2 \\\\ -3 & -1 \\end{pmatrix}",
    "1",
    "3",
    "2",
    "2",
    "1",
    "2",
    "\\begin{pmatrix} \\frac{9}{4} & -\\frac{1}{4} & -1 \\\\ -\\frac{1}{8} & \\frac{1}{8} & 0 \\\\ -\\frac{9}{8} & \\frac{1}{8} & 1 \\end{pmatrix}",
    "\\begin{pmatrix} 3 & -6 \\\\ -5 & 10 \\\\ 7 & -14 \\end{pmatrix}",
    "\\begin{pmatrix} 3 & 10 & 4 \\\\ 11 & 82 & 12 \\end{pmatrix}",
    "48",
    "420",
    "2",
    "\\begin{pmatrix} 0 & 1 & 2 \\\\ \\frac{1}{2} & -\\frac{1}{2} & -1 \\\\ -\\frac{1}{2} & \\frac{3}{2} & 2 \\end{pmatrix}",
    "-3",
    "\\begin{pmatrix} \\frac{1}{3} & -\\frac{1}{3} & \\frac{2}{3} \\\\ \\frac{2}{3} & -\\frac{2}{3} & \\frac{1}{3} \\\\ \\frac{4}{3} & -\\frac{7}{3} & \\frac{5}{3} \\end{pmatrix}",
    "39",
    "93",
    "99",
    "160",
    "\\begin{pmatrix} \\frac{1}{20} & -\\frac{11}{100} \\\\ \\frac{1}{10} & -\\frac{1}{50} \\end{pmatrix}",
    "\\begin{pmatrix} 2 & -1 \\\\ -6 & 4 \\end{pmatrix}",
    "6",
};

/* Le soluzioni attese delle richieste di letter_solutions.tex, nell'ordine
 * del file, scritte come le scrive il programma. Sono i risultati stampati
 * nelle raccolte, dove ci sono, e tutte sono state confermate dall'oracolo
 * (tools/fuzz/fuzz_linear_algebra.py answers), che non usa il programma:
 * vedi letter_solutions_about.tex.
 *
 * Warning: l'oracolo le legge da questo file, tra la riga che apre
 * expected_solutions[] e la sua graffa chiusa: una stringa per riga. */
static const char *const expected_solutions[] = {
    "h = 9",
    "h < 9 \\lor h > 9",
    "h = 0",
    "h = 1",
    "h = -1 \\lor h = \\frac{1}{5}",
    "h < -1 \\lor -1 < h < \\frac{1}{5} \\lor h > \\frac{1}{5}",
    "\\forall k \\in \\mathbb{R}",
    "a = -1 \\lor a = 1",
    "a = -4 \\lor a = 4",
    "t = 0 \\lor t = \\frac{1}{2}",
    "s = 0 \\lor s = 1 \\lor s = 2",
    "s = -1 \\lor s = 1 \\lor s = 2",
    "\\nexists s \\in \\mathbb{R}",
    "s = 50 \\lor s = 200",
    "s = \\frac{92 - 3\\sqrt{545}}{6} \\lor s = \\frac{92 + 3\\sqrt{545}}{6}",
    "s = \\frac{58 - 2\\sqrt{202}}{3} \\lor s = \\frac{58 + 2\\sqrt{202}}{3} \\lor s = \\frac{116}{3}",
    "-1 < k < 1",
    "k \\le -2 \\lor k \\ge 2",
    "\\forall k \\in \\mathbb{R}",
    "x = 1 \\lor x = 2",
    "t = \\frac{-1 - \\sqrt{5}}{2} \\lor t = \\frac{1 - \\sqrt{5}}{2} \\lor t = \\frac{-1 + \\sqrt{5}}{2} \\lor t = \\frac{1 + \\sqrt{5}}{2}",
    "-2 < k < 1",
    "s = a - 1 \\lor s = a + 1",
};

/* Un enunciato con l'esito atteso, per il controllo 7. */
typedef struct {
    const char *name;       /* che cosa controlla, per i messaggi */
    const char *statement;  /* l'enunciato, in LaTeX */
    Outcome expected;       /* l'esito atteso */
    const char *piece;      /* un pezzo del testo atteso: del motivo per NON SUPPORTATO e
                               NON DIMOSTRATO, della nota per FALSO, della risposta per
                               RISOLTO; "" se non c'è nulla da guardare */
} SpecialCase;

/* Le matrici usate più volte nei casi qui sotto. */
#define M22 "\\begin{pmatrix} 1 & 2 \\\\ 3 & 4 \\end{pmatrix}"
#define M23 "\\begin{pmatrix} 1 & 2 & 3 \\\\ 4 & 5 & 6 \\end{pmatrix}"
#define SWAP "\\begin{pmatrix} 0 & 1 \\\\ 1 & 0 \\end{pmatrix}"
#define SHEAR "\\begin{pmatrix} 1 & 1 \\\\ 0 & 1 \\end{pmatrix}"
#define SINGULAR "\\begin{pmatrix} 1 & 2 \\\\ 2 & 4 \\end{pmatrix}"
#define KK "\\begin{pmatrix} k & 1 \\\\ 1 & k \\end{pmatrix}"

static const SpecialCase special_cases[] = {
    /* Le convenzioni di lettura (capitolo 31). */
    {"an exponent without braces is one digit: A^12 is A^1 times 2", SHEAR "^12 = \\begin{pmatrix} 2 & 2 \\\\ 0 & 2 \\end{pmatrix}", PROVED, ""},
    {"and it is not the twelfth power", SHEAR "^12 = \\begin{pmatrix} 1 & 12 \\\\ 0 & 1 \\end{pmatrix}", REFUTED, "riga 1 e colonna 1"},
    {"the twelfth power wants braces", SHEAR "^{12} = \\begin{pmatrix} 1 & 12 \\\\ 0 & 1 \\end{pmatrix}", PROVED, ""},
    {"the argument of det is one factor: det A B is (det A) B", "A = " M22 ", B = " SWAP " \\implies \\det A B = \\begin{pmatrix} 0 & -2 \\\\ -2 & 0 \\end{pmatrix}", PROVED, ""},
    {"so det A B is not a number", "A = " M22 ", B = " SWAP " \\implies \\det A B = 2", UNSUPPORTED, "non si confrontano"},
    {"the determinant of a product wants parentheses", "A = " M22 ", B = " SWAP " \\implies \\det(A B) = 2", PROVED, ""},
    {"the factor of det takes its exponents: the determinant of the inverse", "\\det " M22 "^{-1} = -\\frac{1}{2}", PROVED, ""},
    {"a minus in front applies to the whole power", "-" SWAP "^{2} = -I", PROVED, ""},
    {"and a power of the opposite is another thing", "(-" SWAP ")^{2} = I", PROVED, ""},
    {"vmatrix is the determinant", "\\begin{vmatrix} 1 & 2 \\\\ 3 & 4 \\end{vmatrix} = -2", PROVED, ""},
    {"array with its columns, inside brackets, and bmatrix", "\\left[ \\begin{array}{rr} 1 & 2 \\\\ 3 & 4 \\end{array} \\right]^{T} = \\begin{bmatrix} 1 & 3 \\\\ 2 & 4 \\end{bmatrix}", PROVED, ""},
    {"a line break after the last row", "\\begin{pmatrix} 1 & 2 \\\\ 3 & 4 \\\\ \\end{pmatrix} = " M22, PROVED, ""},
    {"the transpose written with \\top, \\mathrm{T} and t", M23 "^{\\top} = " M23 "^{\\mathrm{T}}", PROVED, ""},
    {"the transpose written without braces", M23 "^T = " M23 "^t", PROVED, ""},
    {"the rank written rk and rg, the trace", "\\mathrm{rk} " M23 " + \\operatorname{rg} " SINGULAR " = \\operatorname{tr} " M22 " - 2", PROVED, ""},
    {"a matrix can be named I", "I = " M22 " \\implies I = " M22, PROVED, ""},
    {"elements written with arithmetic", "\\begin{pmatrix} 2^{3} & \\frac{6}{4} \\\\ 1 - 3 & 2 \\cdot 5 \\end{pmatrix} = \\begin{pmatrix} 8 & \\frac{3}{2} \\\\ -2 & 10 \\end{pmatrix}", PROVED, ""},
    {"a negative exponent in an element", "\\begin{pmatrix} 2^{-2} \\end{pmatrix} = \\begin{pmatrix} \\frac{1}{4} \\end{pmatrix}", PROVED, ""},
    {"0^0 is 1, as in the rest of the program", "\\begin{pmatrix} 0^{0} \\end{pmatrix} = \\begin{pmatrix} 1 \\end{pmatrix}", PROVED, ""},

    /* Numeri e matrici (32d). */
    {"a multiple, with the number on the left", "\\frac{1}{2} " M22 " = \\begin{pmatrix} \\frac{1}{2} & 1 \\\\ \\frac{3}{2} & 2 \\end{pmatrix}", PROVED, ""},
    {"a multiple, with the number on the right", M22 " \\cdot 2 = \\begin{pmatrix} 2 & 4 \\\\ 6 & 8 \\end{pmatrix}", PROVED, ""},
    {"a number in parentheses, raised to a power", "(1 + 2)^{2} " M22 " = 9 " M22, PROVED, ""},
    {"the zeroth power of the zero matrix is the identity", "\\begin{pmatrix} 0 & 0 \\\\ 0 & 0 \\end{pmatrix}^{0} = I", PROVED, ""},
    {"a negative power is a power of the inverse", SHEAR "^{-2} = \\begin{pmatrix} 1 & -2 \\\\ 0 & 1 \\end{pmatrix}", PROVED, ""},
    {"the inverse of a 1 x 1 matrix", "\\begin{pmatrix} 3 \\end{pmatrix}^{-1} = \\begin{pmatrix} \\frac{1}{3} \\end{pmatrix}", PROVED, ""},
    {"numbers are compared with an order", "\\det " M22 " < \\operatorname{tr} " M22, PROVED, ""},
    {"and the order can be wrong", "\\det " M22 " \\ge \\operatorname{tr} " M22, REFUTED, "il primo membro vale -2, il secondo 5"},
    {"less or equal holds with equality", "\\operatorname{rank} " M22 " \\le 2", PROVED, ""},
    {"a strict inequality fails with equality", "\\operatorname{rank} " M22 " > 2", REFUTED, ""},

    /* Uguaglianza tra matrici. */
    {"matrices of different shapes are different", M23 " = " M23 "^{T}", REFUTED, "2 x 3"},
    {"and saying so is true", M23 " \\neq " M23 "^{T}", PROVED, ""},
    {"a row is not a column", "\\begin{pmatrix} 1 & 2 \\end{pmatrix} = \\begin{pmatrix} 1 \\\\ 2 \\end{pmatrix}", REFUTED, "1 x 2"},
    {"the same numbers in another shape are another matrix", M23 " = \\begin{pmatrix} 1 & 2 \\\\ 3 & 4 \\\\ 5 & 6 \\end{pmatrix}", REFUTED, "2 x 3"},
    {"and they are different", M23 " \\neq \\begin{pmatrix} 1 & 2 \\\\ 3 & 4 \\\\ 5 & 6 \\end{pmatrix}", PROVED, ""},
    {"a row is not a longer row that begins the same way", "\\begin{pmatrix} 1 & 2 \\end{pmatrix} = \\begin{pmatrix} 1 & 2 & 3 \\end{pmatrix}", REFUTED, "1 x 2"},
    {"nor is the longer row its beginning", "\\begin{pmatrix} 1 & 2 & 3 \\end{pmatrix} = \\begin{pmatrix} 1 & 2 \\end{pmatrix}", REFUTED, "1 x 3"},
    {"the place of the first difference is reported", M22 " = \\begin{pmatrix} 1 & 2 \\\\ 3 & 5 \\end{pmatrix}", REFUTED, "riga 2 e colonna 2"},

    /* La dimensione di I (32d). */
    {"I with a subscript", "I_{2} " M22 " = " M22, PROVED, ""},
    {"I with a subscript of one digit without braces", "I_2 " M22 " = " M22, PROVED, ""},
    {"I on the right of a 2 x 3 matrix is 3 x 3", M23 " I = " M23, PROVED, ""},
    {"I on both sides of a 2 x 3 matrix has two sizes", "I " M23 " I = " M23, PROVED, ""},
    {"a sum of identities takes the size from the factor beside it", "(I + I) " M23 " = 2 " M23, PROVED, ""},
    {"a multiple of I in a sum", "A = " M22 " \\implies A^{2} - 5A - 2I = 0", PROVED, ""},
    {"I first in a sum", "3I - " SWAP " = \\begin{pmatrix} 3 & -1 \\\\ -1 & 3 \\end{pmatrix}", PROVED, ""},
    {"I alone as a member", M22 " " M22 "^{-1} = I", PROVED, ""},
    {"I alone as the first member", "I = " SWAP "^{2}", PROVED, ""},
    {"a power and a transpose of I", "(2I)^{3} + I^{T} = \\begin{pmatrix} 9 & 0 \\\\ 0 & 9 \\end{pmatrix}", PROVED, ""},
    {"no identity equals a matrix that is not square", M23 " = I", REFUTED, "2 x 3"},
    {"and saying they differ is true", M23 " \\neq I", PROVED, ""},
    {"I added to a matrix that is not square", M23 " + I = " M23, UNSUPPORTED, "non hanno le stesse dimensioni"},
    {"the size of I inside a determinant is not given by what is outside", "\\det I = \\det " SWAP "^{2}", UNSUPPORTED, "la dimensione di I"},
    {"a definition must stand alone: B = I has no size", "B = I \\implies B " M22 " = " M22, UNSUPPORTED, "la dimensione di I"},
    {"a definition that fixes the size of its I", "A = " M22 ", B = A - I \\implies B = \\begin{pmatrix} 0 & 2 \\\\ 3 & 3 \\end{pmatrix}", PROVED, ""},
    {"a subscript that is not a number", "I_{n} " M22 " = " M22, UNSUPPORTED, "pedice"},
    {"a subscript that is zero", "I_{0} " M22 " = " M22, UNSUPPORTED, "pedice"},

    /* Lo 0 da solo è la matrice nulla. */
    {"0 beside a matrix is the zero matrix", M22 " - " M22 " = 0", PROVED, ""},
    {"also on the left", "0 = " M22 " - " M22, PROVED, ""},
    {"also for a matrix that is not square", "\\begin{pmatrix} 0 \\\\ 0 \\end{pmatrix} \\begin{pmatrix} 1 & 2 & 3 \\end{pmatrix} = 0", PROVED, ""},
    {"a matrix that is not zero", M22 " = 0", REFUTED, "riga 1 e colonna 1"},
    {"and saying so", M22 " \\neq 0", PROVED, ""},
    {"another number beside a matrix is a mistake", M22 " = 1", UNSUPPORTED, "non si confrontano"},
    {"0 written as an expression is a number", M22 " = 1 - 1", UNSUPPORTED, "non si confrontano"},
    {"a 1 x 1 matrix is not a number", "\\begin{pmatrix} 1 & 2 \\end{pmatrix} \\begin{pmatrix} 3 \\\\ 4 \\end{pmatrix} = 11", UNSUPPORTED, "non si confrontano"},

    /* Operazioni non definite: l'enunciato non ha senso, e non è falso. */
    {"a sum of different shapes", M22 " + " M23 " = " M22, UNSUPPORTED, "non hanno le stesse dimensioni"},
    {"a product with the wrong shapes", M23 " " M23 " = " M23, UNSUPPORTED, "le colonne della prima"},
    {"the determinant of a matrix that is not square", "\\det " M23 " = 0", UNSUPPORTED, "quadrata"},
    {"the trace of a matrix that is not square", "\\operatorname{tr} " M23 " = 5", UNSUPPORTED, "quadrata"},
    {"the inverse of a singular matrix", SINGULAR "^{-1} = " SINGULAR, UNSUPPORTED, "singolare"},
    {"and it is not different from anything either", SINGULAR "^{-1} \\neq " SINGULAR, UNSUPPORTED, "singolare"},
    {"the inverse of a matrix that is not square", M23 "^{-1} = " M23, UNSUPPORTED, "quadrata"},
    {"a power of a matrix that is not square", M23 "^{2} = " M23, UNSUPPORTED, "quadrata"},
    {"a number plus a matrix", "2 + " M22 " = " M22, UNSUPPORTED, "non si sommano"},
    {"an order between matrices", M22 " < " M22, UNSUPPORTED, "solo con ="},
    {"the transpose of a number", "(\\det " M22 ")^{T} = -2", UNSUPPORTED, "trasposta"},
    {"the determinant of a number", "\\det 2 " M22 " = 0", UNSUPPORTED, "non di un numero"},
    {"a division by zero in an element", "\\begin{pmatrix} \\frac{1}{0} \\end{pmatrix} = \\begin{pmatrix} 1 \\end{pmatrix}", UNSUPPORTED, "divisione per zero"},
    {"the inverse of the number zero", "(\\det " SINGULAR ")^{-1} " M22 " = " M22, UNSUPPORTED, "divisione per zero"},
    {"an undefined definition spoils a statement that does not use it", "A = " SINGULAR "^{-1} \\implies " M22 " = " M22, UNSUPPORTED, "singolare"},
    {"a request for something undefined", SINGULAR "^{-1}", UNSUPPORTED, "singolare"},

    /* Fuori dal linguaggio, o scritto male. */
    {"a letter as an element, in a statement that does not say where the letter lives", "\\begin{pmatrix} 1 & x \\\\ 3 & 4 \\end{pmatrix} = " M22, UNSUPPORTED,
     "una lettera solo se l'enunciato dice dove sta"},
    {"a determinant with a letter and no set for the letter, even if it is an identity", "\\det \\begin{pmatrix} k & 1 \\\\ 1 & k \\end{pmatrix} = k^{2} - 1", UNSUPPORTED,
     "una lettera solo se l'enunciato dice dove sta"},

    /* Con una lettera che ha il suo insieme (32g): che cosa non si tratta
     * ancora, con il suo motivo. */
    {"the rank of a matrix with a letter", "\\{k \\in \\mathbb{R} \\mid \\operatorname{rank} " KK " = 2\\}", UNSUPPORTED, "la discussione per casi"},
    {"the inverse of a matrix with a letter", "\\{k \\in \\mathbb{R} \\mid \\det " KK "^{-1} = 1\\}", UNSUPPORTED, "la discussione per casi"},
    {"the inverse of a matrix of numbers beside a letter", "\\{s \\in \\mathbb{R} \\mid \\det\\left( " M22 "^{-1} - sI \\right) = 0\\}", UNSUPPORTED, "la discussione per casi"},
    {"an equality between matrices with a letter", "\\{k \\in \\mathbb{R} \\mid " KK " = " SWAP "\\}", UNSUPPORTED, "solo dentro un determinante o una traccia"},
    {"an entry divided by the letter", "\\{k \\in \\mathbb{R} \\mid \\det \\begin{pmatrix} \\frac{1}{k} & 1 \\\\ 1 & k \\end{pmatrix} = 0\\}", UNSUPPORTED, "niente divisioni per una lettera"},
    {"a negative power of the letter", "\\{k \\in \\mathbb{R} \\mid \\det \\begin{pmatrix} k^{-1} & 1 \\\\ 1 & k \\end{pmatrix} = 0\\}", UNSUPPORTED, "niente divisioni per una lettera"},
    {"a root of the letter in an entry", "\\{k \\in \\mathbb{R} \\mid \\det \\begin{pmatrix} \\sqrt{k} & 1 \\\\ 1 & k \\end{pmatrix} = 0\\}", UNSUPPORTED, "radici, funzioni"},
    {"a capital letter among the entries is not one of the letters", "\\{h \\in \\mathbb{R} \\mid \\operatorname{tr} \\begin{pmatrix} 0 & I \\\\ h & 1 \\end{pmatrix} = 0\\}", UNSUPPORTED, "nomi di matrici"},
    {"the name of a matrix among the entries is not a number: a block matrix is not read as a 2 x 2 one",
     "A = " M22 " \\implies \\{k \\in \\mathbb{R} \\mid \\det \\begin{pmatrix} A & 0 \\\\ 0 & k \\end{pmatrix} = 0\\}", UNSUPPORTED, "nomi di matrici"},
    {"four letters in one determinant", "\\forall a \\in \\mathbb{R}\\ \\forall b \\in \\mathbb{R}\\ \\forall c \\in \\mathbb{R}\\ \\forall d \\in \\mathbb{R}\\ \\left( \\det \\begin{pmatrix} a & b \\\\ c & d \\end{pmatrix} = ad - bc \\right)",
     UNSUPPORTED, "troppe lettere diverse"},
    {"the determinant of a matrix that is not square, with a letter", "\\{k \\in \\mathbb{R} \\mid \\det \\begin{pmatrix} k & 1 & 2 \\\\ 1 & k & 3 \\end{pmatrix} = 0\\}", UNSUPPORTED, "solo per una matrice quadrata"},
    {"an identity alone in a determinant has no size, with a letter too", "\\{s \\in \\mathbb{R} \\mid \\det\\left( sI \\right) = \\det " M22 "\\}", UNSUPPORTED, "la dimensione di I non è fissata"},
    {"a degree beyond the limit", "\\{k \\in \\mathbb{R} \\mid \\det \\begin{pmatrix} k^{30} & 1 \\\\ 1 & k \\end{pmatrix} = 0\\}", UNSUPPORTED, "grado troppo alto"},
    {"a capital letter that is not defined, with a letter", "B = " M22 " \\implies \\{s \\in \\mathbb{R} \\mid \\det\\left( A - sI \\right) = 0\\}", UNSUPPORTED, "va definita prima di \\implies"},
    {"two numbers without & in a matrix with a letter", "\\{k \\in \\mathbb{R} \\mid \\det \\begin{pmatrix} k 1 \\\\ 1 2 \\end{pmatrix} = 0\\}", UNSUPPORTED, "due numeri scritti uno dopo l'altro"},
    {"a determinant without letters in a statement with a letter", "\\{x \\in \\mathbb{R} \\mid x \\det " M22 " = 4\\}", SOLVED, "x = -2"},
    {"the exponent belongs to the argument of det, with a letter too", "\\{k \\in \\mathbb{R} \\mid \\det " KK "^{2} \\le 9\\}", SOLVED, "-2 \\le k \\le 2"},
    {"a definition that uses a letter", "A = " KK " \\implies \\{k \\in \\mathbb{R} \\mid \\det\\left( A - I \\right) = 0\\}", SOLVED, "k = 0 \\lor k = 2"},
    {"a statement with a letter and a matrix only in a definition that it does not use", "A = " KK " \\implies \\forall k \\in \\mathbb{R}\\ \\left( k^{2} \\ge 0 \\right)", PROVED, ""},
    {"after statements that gave values to k, a k without its set is still a letter", "\\det " KK " = 0", UNSUPPORTED, "una lettera solo se l'enunciato dice dove sta"},
    {"a square root as an element, even of a square", "\\begin{pmatrix} \\sqrt{4} \\end{pmatrix} = \\begin{pmatrix} 2 \\end{pmatrix}", UNSUPPORTED, "radici"},
    {"a matrix without a definition", "A = " M22 " \\implies A = B", UNSUPPORTED, "va definita"},
    {"a matrix defined twice", "A = " M22 ", A = " SWAP " \\implies A = A", UNSUPPORTED, "due volte"},
    {"a matrix that names itself", "A = A + " M22 " \\implies A = A", UNSUPPORTED, "va definita"},
    {"rows of different lengths", "\\begin{pmatrix} 1 & 2 \\\\ 3 \\end{pmatrix} = " M22, UNSUPPORTED, "stesso numero di elementi"},
    {"an environment closed by another", "\\begin{pmatrix} 1 & 2 \\\\ 3 & 4 \\end{bmatrix} = " M22, UNSUPPORTED, "stesso ambiente"},
    {"a decimal point", "\\begin{pmatrix} 1 & 0.5 \\\\ 0 & 1 \\end{pmatrix} = " M22, UNSUPPORTED, ""},
    {"two elements without & are not a product", "\\begin{pmatrix} 1 2 \\\\ 3 4 \\end{pmatrix} = \\begin{pmatrix} 2 \\\\ 12 \\end{pmatrix}", UNSUPPORTED, "due numeri"},
    {"nor are two numbers side by side in front of a matrix", "2 3 " M22 " = 6 " M22, UNSUPPORTED, "due numeri"},
    {"a product of numbers is written with a dot", "2 \\cdot 3 " M22 " = 6 " M22, PROVED, ""},
    {"the cross sign is not a product here", M22 " \\times " M22 " = " M22, UNSUPPORTED, "\\times"},
    {"text after the statement", M22 " = " M22 " \\land " M22 " = " M22, UNSUPPORTED, "testo inatteso"},
    {"a quantifier makes a determinant with a letter a statement of 32g", "\\forall k \\in \\mathbb{R}\\ \\det " KK " = k^{2} - 1", PROVED, ""},
    {"and a wrong one is false there", "\\forall k \\in \\mathbb{R}\\ \\det " KK " = k^{2} + 1", REFUTED, ""},
    {"a quantifier in front of matrices without letters is not a statement of this chapter", "\\forall k \\in \\mathbb{R}\\ " M22 " = " M22, UNSUPPORTED,
     "solo dentro un determinante o una traccia"},
    {"an integral sign in front", "\\int " M22 " \\, dx = " M22, UNSUPPORTED, ""},
    {"an exponent that is a letter", M22 "^{n} = " M22, UNSUPPORTED, "esponente"},
    {"a negative exponent without braces", M22 "^-1 = " M22, UNSUPPORTED, "esponente"},

    /* I limiti del conto: fermarsi, non rispondere a caso. */
    {"an exponent beyond the limit", M22 "^{1000} = I", UNSUPPORTED, "esponente costante troppo grande"},
    {"numbers beyond the exact arithmetic", "\\begin{pmatrix} 99999999999 & 2 \\\\ 3 & 99999999999 \\end{pmatrix}^{9} = I", UNSUPPORTED, "numeri troppo grandi"},
    {"a computation beyond the budget", "\\det I_{1500} = \\det \\begin{pmatrix} 1 \\end{pmatrix}", UNSUPPORTED, "troppo lungo"},

    /* Le richieste (32g). */
    {"a request for a product", M22 " " SWAP, SOLVED, "\\begin{pmatrix} 2 & 1 \\\\ 4 & 3 \\end{pmatrix}"},
    {"a request for a rank", "\\operatorname{rank} " SINGULAR, SOLVED, "1"},
    {"a request for a negative fraction", "\\det " M22 "^{-1}", SOLVED, "-\\frac{1}{2}"},
    {"a request with names", "A = " M22 " \\implies A^{T} A", SOLVED, "\\begin{pmatrix} 10 & 14 \\\\ 14 & 20 \\end{pmatrix}"},
    {"a request with an identity", M22 " - 2I", SOLVED, "\\begin{pmatrix} -1 & 2 \\\\ 3 & 2 \\end{pmatrix}"},
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

/* Cambia l'ultima cifra del numero intero che finisce in *last: un altro
 * numero, con le stesse cifre davanti e senza riporti. */
static void change_digit(char *last) {
    *last = *last == '9' ? '8' : (char)(*last + 1);
}

/* Altera uno per volta i testimoni del certificato text, che il verificatore
 * accetta, e conta le alterazioni che accetta ancora: deve essere zero.
 * Aggiunge a *tried quante ne ha provate. Ogni alterazione è scelta perché
 * NESSUN certificato giusto la sopporti (un testimone non è unico: di una
 * matrice singolare ci sono più L, e più G):
 *
 *   - una voce tolta: i testimoni non bastano più, o non sono più al loro
 *     posto. Il verificatore può rispondere "non so controllarlo", mai sì;
 *   - inverse W, l'ultimo numero cambiato: l'ultima colonna di A W cambia di
 *     un multiplo non nullo dell'ultima colonna di A, che non è nulla perché
 *     A è invertibile;
 *   - lu p L U, l'ultimo numero cambiato: è l'ultimo elemento della diagonale
 *     di U, e nell'ultimo elemento di L U entra moltiplicato per l'1 che L ha
 *     sulla diagonale;
 *   - rank r, r aumentato o diminuito di uno: C, F, G e H non hanno più le
 *     dimensioni che r chiede, e rank 1 senza matrici o rank 0 con matrici
 *     non si leggono;
 *   - rank r C F G H, il primo elemento di C cambiato: la prima riga di C F
 *     cambia di un multiplo non nullo di una riga di F, e le righe di F non
 *     sono nulle perché F ha rango r (G C F H è l'identità r x r). */
static int altered_witnesses_accepted(const char *text, int *tried) {
    static char altered[CERTIFICATE_BYTES + 16];
    int accepted = 0;
    for (const char *line = strstr(text, "\nwitness "); line; line = strstr(line + 1, "\nwitness ")) {
        size_t start = (size_t)(line + 1 - text), end = (size_t)(strchr(line + 1, '\n') - text);

        /* La voce tolta. */
        snprintf(altered, sizeof altered, "%.*s%s", (int)start, text, text + end + 1);
        (*tried)++;
        if (checker_answer(altered) == CK_ACCEPTED) accepted++;

        if (strncmp(text + start, "witness rank ", 13) != 0) {
            /* L'ultimo numero di un'inversa o di U: l'ultima cifra della
             * riga. */
            size_t last = end;
            while (text[last] < '0' || text[last] > '9') last--;
            strcpy(altered, text);
            change_digit(altered + last);
            (*tried)++;
            if (checker_answer(altered) != CK_REJECTED) accepted++;
            continue;
        }

        /* Il rango dichiarato, più alto e più basso di uno. */
        char *after;
        long rank = strtol(text + start + 13, &after, 10);
        for (long other = rank - 1; other <= rank + 1; other += 2) {
            if (other < 0) continue;
            snprintf(altered, sizeof altered, "%.*s%ld%s", (int)(start + 13), text, other, after);
            (*tried)++;
            if (checker_answer(altered) != CK_REJECTED) accepted++;
        }

        /* Il primo elemento di C: dopo "(matrix righe colonne", la prima
         * cifra che segue è l'inizio del suo numeratore. */
        if (rank == 0) continue;
        strcpy(altered, text);
        char *digit = altered + (after - text) + strlen(" (matrix ");
        for (int number = 0; number < 2; number++) digit = strchr(digit, ' ') + 1;
        while (*digit < '0' || *digit > '9') digit++;
        while (digit[1] >= '0' && digit[1] <= '9') digit++;
        change_digit(digit);
        (*tried)++;
        if (checker_answer(altered) != CK_REJECTED) accepted++;
    }
    return accepted;
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
        int slipped = straight == CK_ACCEPTED ? altered_witnesses_accepted(copy, &altered) : 0;
        if (straight == CK_ACCEPTED && reversed == CK_REJECTED && !slipped) { right++; continue; }
        printf("  statement %d of %s: the certificate is %s, with the claim reversed it is %s, and %d altered witnesses are accepted\n", total, path,
               straight == CK_ACCEPTED ? "accepted" : "not accepted", reversed == CK_REJECTED ? "rejected" : "not rejected", slipped);
    }
    free(text);
    snprintf(summary, sizeof summary,
             "%d of %d statements of %s are %s with a certificate that the checker accepts, rejects with the claim reversed, "
             "and no longer accepts with a witness removed or altered (%d alterations)",
             right, total, path, expected == PROVED ? "proved" : "declared false", altered);
    return check_result(total > 0 && right == total && altered > 0, summary);
}

/* Controllo 4: di ogni enunciato di path la cui relazione è = oppure \neq
 * si scrive la negazione, con l'altra delle due, e la si fa decidere: deve
 * essere dichiarata falsa. La relazione è l'ultimo = o \neq della formula:
 * quelli delle definizioni vengono prima di \implies, e dentro le matrici
 * non ce ne sono. Ritorna 1 se il controllo è riuscito. */
static int check_negations(const char *path) {
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
        if (read_item(item, end, &statement)) continue;
        const char *formula = statement.formula, *equal = strrchr(formula, '='), *different = NULL;
        for (const char *p = strstr(formula, "\\neq"); p; p = strstr(p + 1, "\\neq")) different = p;

        /* L'ultima delle due è la relazione; un enunciato con < o > non ha
         * una negazione che si scriva così, e si salta. */
        if (strchr(formula, '<') || strchr(formula, '>') || (!equal && !different)) continue;
        if (different && (!equal || different > equal))
            snprintf(negated, sizeof negated, "%.*s=%s", (int)(different - formula), formula, different + 4);
        else
            snprintf(negated, sizeof negated, "%.*s\\neq%s", (int)(equal - formula), formula, equal + 1);
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
    snprintf(summary, sizeof summary, "%d of %d requests of %s solved with the expected value, each with a verified certificate", right, total, path);
    return check_result(total == expected && right == total, summary);
}

/* Controlli 10 e 11: ogni enunciato di path, con una lettera, riceve l'esito
 * expected. Se il verdetto porta i certificati, il verificatore li accetta
 * uno per uno; tra loro c'è quello di almeno uno sviluppo (la voce letters),
 * e ogni sviluppo con l'ultimo numero del suo enunciato cambiato è respinto:
 * quel numero sta nel polinomio, che così è un altro. Se non li porta, il
 * motivo dice che lo sviluppo è certificato e l'enunciato che ne risulta
 * no. I verdetti con i certificati devono essere certified. Ritorna 1 se il
 * controllo è riuscito. */
static int check_letter_verdicts(const char *path, Outcome expected, int certified) {
    static char copy[CERTIFICATE_BYTES], one[CERTIFICATE_BYTES];
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }
    const char *position = text, *item, *end;
    int total = 0, right = 0, with_certificates = 0, expansions = 0;
    while (next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        total++;
        if (verdict.outcome != expected) {
            printf("  statement %d of %s: expected %s, got %s (%s)\n", total, path, outcome_name(expected), outcome_name(verdict.outcome), verdict.reason ? verdict.reason : "");
            continue;
        }
        if (!verdict.certificate) {
            if (verdict.reason && strstr(verdict.reason, "lo sviluppo di determinanti e tracce ha il suo certificato")) right++;
            else printf("  statement %d of %s has no certificate, and does not say that the expansion has one\n", total, path);
            continue;
        }
        if (strlen(verdict.certificate) >= sizeof copy) {
            printf("  statement %d of %s: certificates too long for the test\n", total, path);
            continue;
        }
        strcpy(copy, verdict.certificate);

        /* I certificati, uno per uno: ognuno va da "certificate" al suo
         * "end". */
        int accepted = 1, letters = 0, slipped = 0;
        for (const char *start = strstr(copy, "certificate "); start && accepted;) {
            const char *stop = strstr(start, "\nend\n");
            if (!stop) { accepted = 0; break; }
            size_t length = (size_t)(stop + 5 - start);
            memcpy(one, start, length);
            one[length] = 0;
            accepted = checker_answer(one) == CK_ACCEPTED;
            char *statement = strstr(one, "\nstatement ");
            if (strncmp(one, "certificate matrices\nletters ", 29) == 0 && statement) {
                char *last = strchr(statement + 1, '\n');
                while (*last < '0' || *last > '9') last--;
                change_digit(last);
                letters++;
                if (checker_answer(one) != CK_REJECTED) slipped++;
            }
            start = strstr(stop + 5, "certificate ");
        }
        with_certificates++;
        expansions += letters;
        if (accepted && !slipped) { right++; continue; }
        printf("  statement %d of %s: its certificates are %s, and %d altered expansions are not rejected\n", total, path,
               accepted ? "accepted" : "not all accepted", slipped);
    }
    free(text);
    snprintf(summary, sizeof summary,
             "%d of %d statements of %s are %s; %d carry certificates that the checker accepts one by one, with %d expansions rejected "
             "once their polynomial is altered; the others say that only the expansion is certified",
             right, total, path, expected == PROVED ? "proved" : "declared false", with_certificates, expansions);
    return check_result(total > 0 && right == total && with_certificates == certified && expansions > 0, summary);
}

/* Controllo 12: ogni richiesta di path, con una lettera, è risolta con le
 * soluzioni attese di expected_solutions[], nello stesso ordine; e le
 * soluzioni, rilette come enunciato, sono dimostrate:
 *
 *     \{x \in \mathbb{R} \mid condizione\}     con le soluzioni S diventa
 *     \forall x \in \mathbb{R}\ (condizione \iff (S)),
 *
 * oppure \forall x \in \mathbb{R}\ (condizione) se S è tutto \mathbb{R}, e
 * \forall x \in \mathbb{R}\ \lnot (condizione) se non ci sono soluzioni. Ciò
 * che sta prima di \{, le definizioni o il quantificatore su un parametro,
 * resta com'è. Ritorna 1 se il controllo è riuscito. */
static int check_letter_solutions(const char *path) {
    static char reread[4 * MAX_FORMULA];
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }
    const char *position = text, *item, *end;
    int expected = (int)(sizeof expected_solutions / sizeof *expected_solutions), total = 0, right = 0;
    while (next_item(&position, &item, &end)) {
        Item statement;
        total++;
        if (total > expected || read_item(item, end, &statement)) {
            printf("  request %d of %s cannot be read, or has no expected solutions\n", total, path);
            continue;
        }
        const char *solution = expected_solutions[total - 1];
        Verdict verdict = prove(statement.formula, 'n', 0, 0);
        if (verdict.outcome != SOLVED || strcmp(verdict.detail, solution) != 0) {
            printf("  request %d of %s: expected %s, got %s %s\n", total, path, solution, outcome_name(verdict.outcome),
                   verdict.outcome == SOLVED ? verdict.detail : verdict.reason ? verdict.reason : "");
            continue;
        }

        /* Le soluzioni rilette: \{ diventa \forall, \mid apre la parentesi,
         * l'ultimo \} la chiude. */
        const char *formula = statement.formula, *open = strstr(formula, "\\{"), *bar = open ? strstr(open, "\\mid") : NULL, *close = NULL;
        for (const char *p = bar; p && (p = strstr(p, "\\}")) != NULL; p += 2) close = p;
        if (!close) {
            printf("  request %d of %s is not written as a set of solutions\n", total, path);
            continue;
        }
        int all = strncmp(solution, "\\forall", 7) == 0, none = strncmp(solution, "\\nexists", 8) == 0;
        int written = snprintf(reread, sizeof reread, "%.*s\\forall %.*s\\ %s\\left( %.*s", (int)(open - formula), formula, (int)(bar - open - 2), open + 2,
                               none ? "\\lnot " : "", (int)(close - bar - 4), bar + 4);
        if (all || none) snprintf(reread + written, sizeof reread - (size_t)written, " \\right)");
        else snprintf(reread + written, sizeof reread - (size_t)written, " \\iff \\left( %s \\right) \\right)", solution);
        Verdict again = prove(reread, 'n', 0, 0);
        if (again.outcome == PROVED) { right++; continue; }
        printf("  request %d of %s: its solutions, read again as %s, are %s\n", total, path, reread, outcome_name(again.outcome));
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d requests of %s solved with the expected solutions, which are proved when read again as a statement", right, total, path);
    return check_result(total == expected && right == total, summary);
}

/* Controllo 6, seconda parte: nessun enunciato di path, che è fuori dal
 * linguaggio, risulta deciso. Ritorna 1 se il controllo è riuscito. */
static int check_none_decided(const char *path) {
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }
    const char *position = text, *item, *end;
    int total = 0, decided = 0;
    while (next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        total++;
        if (verdict.outcome != UNSUPPORTED && verdict.outcome != NOT_PROVED) {
            decided++;
            printf("  statement %d of %s is beyond the language but was answered %s\n", total, path, outcome_name(verdict.outcome));
        }
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d statements of %s, beyond the language, are left undecided", total - decided, total, path);
    return check_result(total > 0 && decided == 0, summary);
}

/* Controllo 7: ogni enunciato di special_cases[] riceve l'esito atteso, e il
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
    snprintf(summary, sizeof summary, "%d of %d statements on reading, shapes, identities and limits answered as expected", right, total);
    return check_result(right == total, summary);
}

/* Controllo 8: un enunciato sulle matrici più lungo di MAX_FORMULA viene
 * rifiutato intero, con il suo motivo. Lo si costruisce sommando molte volte
 * la stessa matrice: è vero, e non deve essere né letto a metà né deciso.
 * Ritorna 1 se il controllo è riuscito. */
static int check_too_long(void) {
    static char statement[2 * MAX_FORMULA];
    size_t length = 0;
    while (length < MAX_FORMULA + 64) length += (size_t)snprintf(statement + length, sizeof statement - length, "%s + ", M22);
    snprintf(statement + length, sizeof statement - length, "%s \\neq 0", M22);
    Verdict verdict = prove(statement, 'n', 0, 0);
    return check_result(verdict.outcome == UNSUPPORTED && verdict.reason && strstr(verdict.reason, "troppo lungo"),
                        "a statement longer than the limit is refused whole, not read in part");
}

int main(void) {
    engine_init();

    /* Tutti i controlli vengono eseguiti anche se uno fallisce, così il
     * resoconto è completo. */
    int passed = check_all_proved("tests/linear_algebra/book_matrices.tex");
    passed = check_certified("tests/linear_algebra/book_matrices.tex", PROVED) && passed;
    passed = check_certified("tests/linear_algebra/false_book_matrices.tex", REFUTED) && passed;
    passed = check_negations("tests/linear_algebra/book_matrices.tex") && passed;
    passed = check_requests("tests/linear_algebra/matrix_requests.tex") && passed;
    passed = check_none_refuted("tests/linear_algebra/matrices_beyond.tex") && passed;
    passed = check_none_decided("tests/linear_algebra/matrices_beyond.tex") && passed;
    passed = check_special_cases() && passed;
    passed = check_too_long() && passed;

    /* Con una lettera. */
    passed = check_all_proved("tests/linear_algebra/letter_determinants.tex") && passed;
    passed = check_letter_verdicts("tests/linear_algebra/letter_determinants.tex", PROVED, 35) && passed;
    passed = check_letter_verdicts("tests/linear_algebra/false_letter_determinants.tex", REFUTED, 33) && passed;
    passed = check_letter_solutions("tests/linear_algebra/letter_solutions.tex") && passed;

    /* Ogni testimone ha superato i suoi prodotti, ogni risposta la sua
     * rilettura, e il verificatore non ha respinto nessun certificato. */
    passed = check_result(unjustified_steps == 0, "no witness failed its products, no answer failed its reading, no certificate was rejected") && passed;
    return passed ? 0 : 1;
}
