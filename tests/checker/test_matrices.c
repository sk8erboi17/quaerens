/* Test del verificatore sui certificati delle matrici (capitolo 17 di
 * checker.c), da solo: qui il motore non c'entra.
 *
 * Compilazione ed esecuzione, dalla cartella principale del progetto:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/checker checker.c && \
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_matrices tests/checker/test_matrices.c && /tmp/test_matrices
 *
 * Come in test_checker.c, il primo comando fa parte del test: checker.c si
 * compila da solo, e questo file include soltanto checker.c.
 *
 * Il verificatore non fa eliminazioni: somma, moltiplica e confronta. Un
 * determinante, un rango, un'inversa li accetta solo con un testimone, una
 * voce witness che ricontrolla con dei prodotti. I certificati accettati
 * sono quelli che il motore scrive (32f di main.c), copiati qui come dati, o
 * scritti a mano con un testimone diverso ma giusto; gli altri ne sono
 * alterazioni. Ognuno ha la risposta attesa e, se non è accettato, un pezzo
 * del motivo, così un certificato respinto per la ragione sbagliata non
 * passa per buono:
 *
 *   - accettati: i conti diretti (prodotti, somme, multipli, trasposte,
 *     potenze, tracce, numeri); il determinante con il suo testimone, con e
 *     senza scambi di righe, di una matrice singolare, con una colonna senza
 *     perno, di una 5 x 5, e con un altro testimone giusto; il rango da
 *     sopra e da sotto, di matrici rettangolari, della matrice nulla, e con
 *     un altro testimone giusto; l'inversa; più testimoni nell'ordine in cui
 *     l'enunciato li chiede; affermazioni false dichiarate false;
 *   - respinti, per ogni tipo di testimone: un elemento cambiato; una forma
 *     che non è quella chiesta (L non triangolare, o senza 1 sulla
 *     diagonale, U non triangolare) anche quando il prodotto torna; una
 *     permutazione che non lo è, o che è un'altra; un rango dichiarato più
 *     alto o più basso di uno; il testimone di un'altra matrice; un
 *     testimone di un altro tipo, fuori ordine, in più, con un determinante
 *     dentro;
 *   - respinti anche: una verità rovesciata, gli stessi numeri in una
 *     tabella di un'altra forma detti la stessa matrice, ogni operazione non
 *     definita, un certificato scritto male;
 *   - non controllabili: un testimone che manca, una parola o una relazione
 *     che il verificatore non conosce, numeri oltre i 128 bit, matrici o
 *     conti oltre i suoi tetti. Non devono mai diventare un "accettato".
 *
 * Con la voce letters (17d di checker.c) gli elementi sono polinomi, e il
 * certificato dice che due espressioni sono uguali per ogni valore delle
 * lettere. Il determinante si calcola lì con la sua definizione, senza
 * testimoni:
 *
 *   - accettati: lo sviluppo di un determinante 1 x 1, 2 x 2, 3 x 3 e
 *     4 x 4, con una lettera e con due, il polinomio caratteristico scritto
 *     con l'identità, la traccia di una potenza, la trasposta, un prodotto,
 *     un'uguaglianza tra matrici di polinomi;
 *   - respinti: un polinomio con un coefficiente, un grado o un segno
 *     sbagliati, o giusto solo per qualche valore della lettera; le
 *     operazioni non definite; una lettera non dichiarata; un testimone,
 *     che qui non serve; la voce letters scritta male;
 *   - non controllabili: una verità dichiarata falsa (due polinomi diversi
 *     non dicono per quali valori lo sono), una relazione diversa da =, il
 *     rango e l'inversa, una divisione per un polinomio, troppe lettere, un
 *     determinante troppo grande per la sua definizione.
 *
 * I casi respinti e non controllabili sono antiregressione: non si tolgono
 * e non si indeboliscono per far passare una modifica al verificatore.
 */

#include <stdio.h>
#include <string.h>

/* Il verificatore ha il suo main(): lo si include cambiando nome. */
#define main checker_main
#include "../../checker.c"
#undef main

/* Un certificato scritto a mano e ciò che il verificatore deve rispondere. */
typedef struct {
    const char *name;   /* che cosa mette alla prova, in inglese */
    CkAnswer expected;  /* la risposta attesa */
    const char *why;    /* un pezzo del motivo atteso; "" per gli accettati */
    const char *text;   /* il certificato */
} Case;

/* Un certificato con l'enunciato, la verità dichiarata e i testimoni; e una
 * voce witness. NONE è l'elenco vuoto dei testimoni. */
#define CERT(statement, claim, witnesses) "certificate matrices\nstatement " statement "\nclaim " claim "\n" witnesses "end\n"
#define W(text) "witness " text "\n"
#define NONE ""

/* Un certificato con le lettere dichiarate dalla voce letters (17d). */
#define LCERT(letters, statement, claim, witnesses) \
    "certificate matrices\nletters " letters "\nstatement " statement "\nclaim " claim "\n" witnesses "end\n"

/* Le matrici usate più volte. */
#define A "(matrix 2 2 1 2 3 4)"
#define A_INVERSE "(matrix 2 2 -2 1 (/ 3 2) (/ -1 2))"
#define UNIT "(matrix 2 2 1 0 0 1)"
#define SWAP "(matrix 2 2 0 1 1 0)"
#define SHEAR "(matrix 2 2 1 1 0 1)"
#define SINGULAR "(matrix 2 2 1 2 2 4)"
#define WIDE "(matrix 2 3 1 2 3 4 5 6)"
#define TALL "(matrix 3 2 1 2 3 4 5 6)"  /* gli stessi numeri di WIDE, nello stesso ordine, in un'altra forma */
#define NINE "(matrix 3 3 1 2 3 4 5 6 7 8 9)"
#define CYCLE "(matrix 3 3 0 0 1 1 0 0 0 1 0)"
#define NO_PIVOT "(matrix 3 3 0 1 2 0 3 4 0 5 6)"
#define FIVE "(matrix 5 5 2 2 2 2 2 1 1 1 1 5 1 1 1 4 1 1 1 3 1 1 1 2 1 1 1)"
#define STAIRS "(matrix 3 4 0 0 1 2 0 0 0 3 0 0 0 0)"
#define THIRDS "(matrix 3 2 1 -2 -1 2 3 -6)"
#define KK "(matrix 2 2 k 1 1 k)"  /* con una lettera: il suo determinante è k^2 - 1 */

/* I testimoni di quelle matrici, come li scrive il motore. Del determinante:
 * la permutazione, L e U. Del rango: r, C, F, G, H. */
#define LU_A W("lu 1 2 (matrix 2 2 1 0 3 1) (matrix 2 2 1 2 0 -2)")
#define LU_SWAP W("lu 2 1 " UNIT " " UNIT)
#define LU_CYCLE W("lu 2 3 1 (identity 3) (identity 3)")
#define LU_NO_PIVOT W("lu 1 2 3 (matrix 3 3 1 0 0 0 1 0 0 (/ 5 3) 1) (matrix 3 3 0 1 2 0 3 4 0 0 (/ -2 3))")
#define LU_NINE W("lu 1 2 3 (matrix 3 3 1 0 0 4 1 0 7 2 1) (matrix 3 3 1 2 3 0 -3 -6 0 0 0)")
#define LU_FIVE W("lu 1 5 4 3 2 (matrix 5 5 1 0 0 0 0 (/ 1 2) 1 0 0 0 (/ 1 2) 0 1 0 0 (/ 1 2) 0 0 1 0 (/ 1 2) 0 0 0 1) " \
                  "(matrix 5 5 2 2 2 2 2 0 1 0 0 0 0 0 2 0 0 0 0 0 3 0 0 0 0 0 4)")
#define RANK_NINE W("rank 2 (matrix 3 2 1 2 4 5 7 8) (matrix 2 3 1 0 -1 0 1 2) (matrix 2 3 (/ -5 3) (/ 2 3) 0 (/ 4 3) (/ -1 3) 0) (matrix 3 2 1 0 0 1 0 0)")
#define RANK_WIDE W("rank 2 (matrix 2 2 1 2 4 5) (matrix 2 3 1 0 -1 0 1 2) (matrix 2 2 (/ -5 3) (/ 2 3) (/ 4 3) (/ -1 3)) (matrix 3 2 1 0 0 1 0 0)")
#define RANK_THIRDS W("rank 1 (matrix 3 1 1 -1 3) (matrix 1 2 1 -2) (matrix 1 3 1 0 0) (matrix 2 1 1 0)")
#define RANK_STAIRS W("rank 2 (matrix 3 2 1 2 0 3 0 0) (matrix 2 4 0 0 1 0 0 0 0 1) (matrix 2 3 1 (/ -2 3) 0 0 (/ 1 3) 0) (matrix 4 2 0 0 0 0 1 0 0 1)")
#define RANK_A W("rank 2 " A " " UNIT " " A_INVERSE " " UNIT)
#define INVERSE_A W("inverse " A_INVERSE)

static const Case cases[] = {
    /* I conti che il verificatore fa da sé: nessun testimone. */
    {"a product of two matrices", CK_ACCEPTED, "", CERT("(= (* " A " " SWAP ") (matrix 2 2 2 1 4 3))", "true", NONE)},
    {"a product of a 2 x 3 and a 3 x 1 matrix", CK_ACCEPTED, "", CERT("(= (* " WIDE " (matrix 3 1 1 0 1)) (matrix 2 1 4 10))", "true", NONE)},
    {"a sum, a difference and an opposite", CK_ACCEPTED, "", CERT("(= (- (+ " A " " A ") (neg " A ")) (* 3 " A "))", "true", NONE)},
    {"a multiple with the number on the right", CK_ACCEPTED, "", CERT("(= (* " A " (/ 1 2)) (matrix 2 2 (/ 1 2) 1 (/ 3 2) 2))", "true", NONE)},
    {"a transpose", CK_ACCEPTED, "", CERT("(= (transpose " WIDE ") (matrix 3 2 1 4 2 5 3 6))", "true", NONE)},
    {"a square", CK_ACCEPTED, "", CERT("(= (^ " A " 2) (matrix 2 2 7 10 15 22))", "true", NONE)},
    {"the zeroth power is the identity, also of a singular matrix", CK_ACCEPTED, "", CERT("(= (^ " SINGULAR " 0) (identity 2))", "true", NONE)},
    {"a trace", CK_ACCEPTED, "", CERT("(= (trace " A ") 5)", "true", NONE)},
    {"numbers with the four operations and a power", CK_ACCEPTED, "", CERT("(= (- (* (/ 3 4) (^ 2 3)) (+ 1 (neg 2))) 7)", "true", NONE)},
    {"0^0 is 1: the product of no factors", CK_ACCEPTED, "", CERT("(= (^ 0 0) 1)", "true", NONE)},
    {"the zero matrix", CK_ACCEPTED, "", CERT("(= (- " WIDE " " WIDE ") (zero 2 3))", "true", NONE)},
    {"two different matrices are different", CK_ACCEPTED, "", CERT("(neq (* " A " (matrix 2 2 0 1 0 0)) (* (matrix 2 2 0 1 0 0) " A "))", "true", NONE)},
    {"matrices of different shapes are different", CK_ACCEPTED, "", CERT("(neq " WIDE " (transpose " WIDE "))", "true", NONE)},
    {"the same numbers in another shape are another matrix", CK_ACCEPTED, "", CERT("(neq " WIDE " " TALL ")", "true", NONE)},
    {"a row and a column with the same numbers are different", CK_ACCEPTED, "", CERT("(neq (matrix 1 2 1 2) (matrix 2 1 1 2))", "true", NONE)},
    {"a wrong entry, declared false", CK_ACCEPTED, "", CERT("(= (* " A " " SWAP ") (matrix 2 2 2 1 4 4))", "false", NONE)},
    {"equal matrices called different, declared false", CK_ACCEPTED, "", CERT("(neq " A " " A ")", "false", NONE)},
    {"matrices of different shapes called equal, declared false", CK_ACCEPTED, "", CERT("(= " WIDE " (transpose " WIDE "))", "false", NONE)},
    {"the same numbers in another shape called equal, declared false", CK_ACCEPTED, "", CERT("(= " WIDE " " TALL ")", "false", NONE)},

    /* Il determinante, con il suo testimone: la permutazione, L e U. */
    {"a determinant", CK_ACCEPTED, "", CERT("(= (det " A ") -2)", "true", LU_A)},
    {"a determinant that needs a row exchange: the sign of the permutation", CK_ACCEPTED, "", CERT("(= (det " SWAP ") -1)", "true", LU_SWAP)},
    {"a determinant with a cyclic permutation of three rows, which is even", CK_ACCEPTED, "", CERT("(= (det " CYCLE ") 1)", "true", LU_CYCLE)},
    {"a determinant with a column without a pivot: a zero on the diagonal of U", CK_ACCEPTED, "", CERT("(= (det " NO_PIVOT ") 0)", "true", LU_NO_PIVOT)},
    {"the determinant of a singular matrix", CK_ACCEPTED, "", CERT("(= (det " NINE ") 0)", "true", LU_NINE)},
    {"the determinant of a 5 x 5 matrix, with an odd permutation", CK_ACCEPTED, "", CERT("(= (det " FIVE ") 48)", "true", LU_FIVE)},
    {"a determinant with fractions", CK_ACCEPTED, "",
     CERT("(= (det (matrix 2 2 (/ 1 2) (/ 1 3) (/ 1 4) (/ 1 5))) (/ 1 60))", "true", W("lu 1 2 (matrix 2 2 1 0 (/ 1 2) 1) (matrix 2 2 (/ 1 2) (/ 1 3) 0 (/ 1 30))"))},
    {"the determinant of a 1 x 1 matrix", CK_ACCEPTED, "", CERT("(= (det (matrix 1 1 -7)) -7)", "true", W("lu 1 (matrix 1 1 1) (matrix 1 1 -7)"))},
    {"another right witness for the same determinant: the rows exchanged first", CK_ACCEPTED, "",
     CERT("(= (det " A ") -2)", "true", W("lu 2 1 (matrix 2 2 1 0 (/ 1 3) 1) (matrix 2 2 3 4 0 (/ 2 3))"))},
    {"a wrong determinant, declared false", CK_ACCEPTED, "", CERT("(= (det " A ") 2)", "false", LU_A)},
    {"a strict order between a determinant and a trace", CK_ACCEPTED, "", CERT("(< (det " A ") (trace " A "))", "true", LU_A)},

    /* Il rango, con il suo testimone: r, C e F da sopra, G e H da sotto. */
    {"the rank of a singular matrix", CK_ACCEPTED, "", CERT("(= (rank " NINE ") 2)", "true", RANK_NINE)},
    {"the rank of a wide matrix", CK_ACCEPTED, "", CERT("(= (rank " WIDE ") 2)", "true", RANK_WIDE)},
    {"the rank of a tall matrix with proportional rows", CK_ACCEPTED, "", CERT("(= (rank " THIRDS ") 1)", "true", RANK_THIRDS)},
    {"the rank of the zero matrix, without matrices in the witness", CK_ACCEPTED, "", CERT("(= (rank (zero 2 3)) 0)", "true", W("rank 0"))},
    {"a rank with the pivots not on the diagonal", CK_ACCEPTED, "", CERT("(= (rank " STAIRS ") 2)", "true", RANK_STAIRS)},
    {"the rank of an invertible matrix", CK_ACCEPTED, "", CERT("(= (rank " A ") 2)", "true", RANK_A)},
    {"another right witness for the same rank", CK_ACCEPTED, "", CERT("(= (rank " A ") 2)", "true", W("rank 2 " UNIT " " A " " UNIT " " A_INVERSE))},
    {"a wrong rank, declared false", CK_ACCEPTED, "", CERT("(= (rank " NINE ") 3)", "false", RANK_NINE)},
    {"an order that holds with equality", CK_ACCEPTED, "", CERT("(<= (rank " A ") 2)", "true", RANK_A)},
    {"a strict order that fails with equality, declared false", CK_ACCEPTED, "", CERT("(< (rank " A ") 2)", "false", RANK_A)},

    /* L'inversa, con il suo testimone. */
    {"an inverse", CK_ACCEPTED, "", CERT("(= (inv " A ") " A_INVERSE ")", "true", INVERSE_A)},
    {"a matrix times its inverse", CK_ACCEPTED, "", CERT("(= (* " A " (inv " A ")) (identity 2))", "true", INVERSE_A)},
    {"a power of an inverse", CK_ACCEPTED, "", CERT("(= (^ (inv " SHEAR ") 2) (matrix 2 2 1 -2 0 1))", "true", W("inverse (matrix 2 2 1 -1 0 1)"))},

    /* Più testimoni: nell'ordine in cui l'enunciato li chiede, prima quelli
     * dentro e poi quello che li contiene, da sinistra a destra. */
    {"a determinant and a rank, one witness each", CK_ACCEPTED, "", CERT("(= (+ (det " NINE ") (rank " NINE ")) 2)", "true", LU_NINE RANK_NINE)},
    {"the determinant of an inverse, then a rank: three witnesses", CK_ACCEPTED, "",
     CERT("(= (+ (det (inv " A ")) (rank " WIDE ")) (/ 3 2))", "true", INVERSE_A W("lu 1 2 (matrix 2 2 1 0 (/ -3 4) 1) (matrix 2 2 -2 1 0 (/ 1 4))") RANK_WIDE)},
    {"the same matrix twice wants its witness twice", CK_ACCEPTED, "", CERT("(= (* (det " A ") (det " A ")) 4)", "true", LU_A LU_A)},

    /* Sbagliati di poco, o con la verità rovesciata. */
    {"a true product declared false", CK_REJECTED, "la relazione vale", CERT("(= (* " A " " SWAP ") (matrix 2 2 2 1 4 3))", "false", NONE)},
    {"a product with one entry changed", CK_REJECTED, "la relazione non vale", CERT("(= (* " A " " SWAP ") (matrix 2 2 2 1 4 4))", "true", NONE)},
    {"a transpose that is not transposed", CK_REJECTED, "la relazione non vale", CERT("(= (transpose " A ") " A ")", "true", NONE)},
    {"a true inequality declared false", CK_REJECTED, "la relazione vale", CERT("(neq " WIDE " (transpose " WIDE "))", "false", NONE)},
    {"a determinant with the wrong sign, with the right witness", CK_REJECTED, "la relazione non vale", CERT("(= (det " A ") 2)", "true", LU_A)},
    {"a determinant off by one, with the right witness", CK_REJECTED, "la relazione non vale", CERT("(= (det " FIVE ") 49)", "true", LU_FIVE)},
    {"a determinant that forgets the sign of a row exchange", CK_REJECTED, "la relazione non vale", CERT("(= (det " SWAP ") 1)", "true", LU_SWAP)},
    {"a singular matrix with a non-zero determinant", CK_REJECTED, "la relazione non vale", CERT("(neq (det " NINE ") 0)", "true", LU_NINE)},
    {"a rank one too large, with the right witness", CK_REJECTED, "la relazione non vale", CERT("(= (rank " NINE ") 3)", "true", RANK_NINE)},
    {"a rank one too small, with the right witness", CK_REJECTED, "la relazione non vale", CERT("(= (rank " NINE ") 1)", "true", RANK_NINE)},
    {"an order the wrong way round", CK_REJECTED, "la relazione non vale", CERT("(< (trace " A ") (det " A "))", "true", LU_A)},

    /* Le dimensioni fanno parte della matrice: gli stessi numeri, scritti
     * nello stesso ordine, in una tabella di un'altra forma sono un'altra
     * matrice. Un confronto che guardasse solo i numeri accetterebbe questi
     * certificati: la prova del bugiardo lo ha mostrato, quando i soli casi
     * con forme diverse avevano anche numeri diversi. */
    {"the same numbers in a 2 x 3 and a 3 x 2 table called equal", CK_REJECTED, "la relazione non vale", CERT("(= " WIDE " " TALL ")", "true", NONE)},
    {"a row called equal to the column with the same numbers", CK_REJECTED, "la relazione non vale", CERT("(= (matrix 1 2 1 2) (matrix 2 1 1 2))", "true", NONE)},
    {"a 2 x 2 table called equal to a row of four", CK_REJECTED, "la relazione non vale", CERT("(= " A " (matrix 1 4 1 2 3 4))", "true", NONE)},
    {"a row called equal to a longer row that begins the same way", CK_REJECTED, "la relazione non vale", CERT("(= (matrix 1 2 1 2) (matrix 1 3 1 2 3))", "true", NONE)},
    {"a longer row called equal to its beginning", CK_REJECTED, "la relazione non vale", CERT("(= (matrix 1 3 1 2 3) (matrix 1 2 1 2))", "true", NONE)},
    {"the same numbers in another shape called not different", CK_REJECTED, "la relazione vale", CERT("(neq " WIDE " " TALL ")", "false", NONE)},
    {"a product compared with the same numbers in the transposed shape", CK_REJECTED, "la relazione non vale", CERT("(= (* " WIDE " (identity 3)) " TALL ")", "true", NONE)},

    /* Il testimone di un determinante che non torna. In ognuno l'enunciato
     * dichiara proprio ciò che il testimone, creduto, darebbe. */
    {"an entry of U changed", CK_REJECTED, "L U non è la matrice", CERT("(= (det " A ") -3)", "true", W("lu 1 2 (matrix 2 2 1 0 3 1) (matrix 2 2 1 2 0 -3)"))},
    {"an entry of L changed", CK_REJECTED, "L U non è la matrice", CERT("(= (det " A ") -2)", "true", W("lu 1 2 (matrix 2 2 1 0 4 1) (matrix 2 2 1 2 0 -2)"))},
    {"L with an entry above the diagonal", CK_REJECTED, "L triangolare inferiore", CERT("(= (det " A ") -2)", "true", W("lu 1 2 (matrix 2 2 1 1 3 1) (matrix 2 2 1 2 0 -2)"))},
    {"L with an entry above the diagonal, even though L U is the matrix", CK_REJECTED, "L triangolare inferiore",
     CERT("(= (det " A ") 1)", "true", W("lu 1 2 (matrix 2 2 1 1 3 1) (matrix 2 2 1 1 0 1)"))},
    {"L without 1 on the diagonal, even though L U is the matrix", CK_REJECTED, "1 sulla diagonale",
     CERT("(= (det " A ") -1)", "true", W("lu 1 2 (matrix 2 2 2 0 6 1) (matrix 2 2 (/ 1 2) 1 0 -2)"))},
    {"U that is not triangular, even though L U is the matrix", CK_REJECTED, "U triangolare superiore", CERT("(= (det " A ") 4)", "true", W("lu 1 2 " UNIT " " A))},
    {"L and U exchanged: U L is not L U, and L is not upper triangular", CK_REJECTED, "triangolare",
     CERT("(= (det " A ") -2)", "true", W("lu 1 2 (matrix 2 2 1 2 0 -2) (matrix 2 2 1 0 3 1)"))},
    {"a permutation with a row twice", CK_REJECTED, "non dà una permutazione", CERT("(= (det " A ") -2)", "true", W("lu 1 1 (matrix 2 2 1 0 3 1) (matrix 2 2 1 2 0 -2)"))},
    {"a row twice, even though L U is the first row twice: the determinant would be zero", CK_REJECTED, "non dà una permutazione",
     CERT("(= (det " A ") 0)", "true", W("lu 1 1 (matrix 2 2 1 0 1 1) (matrix 2 2 1 2 0 0)"))},
    {"a permutation with a row that does not exist", CK_REJECTED, "non dà una permutazione", CERT("(= (det " A ") -2)", "true", W("lu 1 3 (matrix 2 2 1 0 3 1) (matrix 2 2 1 2 0 -2)"))},
    {"a permutation that starts from zero", CK_REJECTED, "non dà una permutazione", CERT("(= (det " A ") -2)", "true", W("lu 0 1 (matrix 2 2 1 0 3 1) (matrix 2 2 1 2 0 -2)"))},
    {"another permutation with the same L and U: the sign would change", CK_REJECTED, "L U non è la matrice", CERT("(= (det " A ") 2)", "true", W("lu 2 1 (matrix 2 2 1 0 3 1) (matrix 2 2 1 2 0 -2)"))},
    {"the identity permutation for a matrix whose rows were exchanged", CK_REJECTED, "L U non è la matrice", CERT("(= (det " SWAP ") 1)", "true", W("lu 1 2 " UNIT " " UNIT))},
    {"the witness of another matrix of the same size", CK_REJECTED, "L U non è la matrice", CERT("(= (det " SWAP ") -2)", "true", LU_A)},
    {"the witness of a smaller matrix: the permutation is too short", CK_REJECTED, "numero intero atteso", CERT("(= (det " NINE ") -2)", "true", LU_A)},
    {"the witness of a larger matrix: the permutation is too long", CK_REJECTED, "dimensioni", CERT("(= (det " A ") 0)", "true", LU_NINE)},
    {"a permutation with too few rows", CK_REJECTED, "scritto male", CERT("(= (det " A ") -2)", "true", W("lu 1 (matrix 2 2 1 0 3 1) (matrix 2 2 1 2 0 -2)"))},
    {"a witness without U", CK_REJECTED, "scritto male", CERT("(= (det " A ") -2)", "true", W("lu 1 2 (matrix 2 2 1 0 3 1)"))},
    {"a number in place of L", CK_REJECTED, "dimensioni", CERT("(= (det " A ") -2)", "true", W("lu 1 2 1 (matrix 2 2 1 2 0 -2)"))},
    {"the determinant of a matrix that is not square", CK_REJECTED, "non è una matrice quadrata", CERT("(= (det " WIDE ") 0)", "true", LU_A)},
    {"the determinant of a number", CK_REJECTED, "non è una matrice quadrata", CERT("(= (det 3) 3)", "true", NONE)},

    /* Il testimone di un rango che non torna. */
    {"a rank declared one too high: no G and H can pass", CK_REJECTED, "G A H non è l'identità",
     CERT("(= (rank " NINE ") 3)", "true", W("rank 3 " NINE " (identity 3) (identity 3) (identity 3)"))},
    {"a rank declared one too low: no C and F can pass", CK_REJECTED, "C F non è la matrice",
     CERT("(= (rank " NINE ") 1)", "true", W("rank 1 (matrix 3 1 1 4 7) (matrix 1 3 1 2 3) (matrix 1 3 1 0 0) (matrix 3 1 1 0 0)"))},
    {"an entry of C changed", CK_REJECTED, "C F non è la matrice",
     CERT("(= (rank " WIDE ") 2)", "true", W("rank 2 (matrix 2 2 1 2 4 6) (matrix 2 3 1 0 -1 0 1 2) (matrix 2 2 (/ -5 3) (/ 2 3) (/ 4 3) (/ -1 3)) (matrix 3 2 1 0 0 1 0 0)"))},
    {"an entry of F changed", CK_REJECTED, "C F non è la matrice",
     CERT("(= (rank " WIDE ") 2)", "true", W("rank 2 (matrix 2 2 1 2 4 5) (matrix 2 3 1 0 -1 0 1 3) (matrix 2 2 (/ -5 3) (/ 2 3) (/ 4 3) (/ -1 3)) (matrix 3 2 1 0 0 1 0 0)"))},
    {"an entry of G changed", CK_REJECTED, "G A H non è l'identità",
     CERT("(= (rank " WIDE ") 2)", "true", W("rank 2 (matrix 2 2 1 2 4 5) (matrix 2 3 1 0 -1 0 1 2) (matrix 2 2 (/ -5 3) (/ 2 3) (/ 4 3) (/ 1 3)) (matrix 3 2 1 0 0 1 0 0)"))},
    {"H that takes another column", CK_REJECTED, "G A H non è l'identità",
     CERT("(= (rank " WIDE ") 2)", "true", W("rank 2 (matrix 2 2 1 2 4 5) (matrix 2 3 1 0 -1 0 1 2) (matrix 2 2 (/ -5 3) (/ 2 3) (/ 4 3) (/ -1 3)) (matrix 3 2 1 0 0 0 0 1)"))},
    {"only the upper bound: G and H are zero", CK_REJECTED, "G A H non è l'identità",
     CERT("(= (rank " WIDE ") 2)", "true", W("rank 2 (matrix 2 2 1 2 4 5) (matrix 2 3 1 0 -1 0 1 2) (zero 2 2) (zero 3 2)"))},
    {"only the lower bound: C F is another matrix", CK_REJECTED, "C F non è la matrice",
     CERT("(= (rank " WIDE ") 2)", "true", W("rank 2 (identity 2) (matrix 2 3 1 0 0 0 1 0) (matrix 2 2 (/ -5 3) (/ 2 3) (/ 4 3) (/ -1 3)) (matrix 3 2 1 0 0 1 0 0)"))},
    {"rank zero declared for a matrix that is not zero", CK_REJECTED, "non è nulla", CERT("(= (rank " A ") 0)", "true", W("rank 0"))},
    {"matrices after a rank zero", CK_REJECTED, "testo in più", CERT("(= (rank (zero 2 2)) 0)", "true", W("rank 0 " UNIT))},
    {"a rank higher than the number of rows", CK_REJECTED, "non può avere", CERT("(= (rank " WIDE ") 3)", "true", W("rank 3 (identity 3) (identity 3) (identity 3) (identity 3)"))},
    {"a negative rank", CK_REJECTED, "non può avere", CERT("(= (rank " WIDE ") -1)", "true", W("rank -1"))},
    {"C with one column too many", CK_REJECTED, "dimensioni",
     CERT("(= (rank " WIDE ") 2)", "true", W("rank 2 (matrix 2 3 1 2 3 4 5 6) (identity 3) (matrix 2 2 (/ -5 3) (/ 2 3) (/ 4 3) (/ -1 3)) (matrix 3 2 1 0 0 1 0 0)"))},
    {"the witness of another matrix of the same shape", CK_REJECTED, "C F non è la matrice", CERT("(= (rank (matrix 2 3 1 2 3 4 5 7)) 2)", "true", RANK_WIDE)},
    {"the witness of a matrix of another shape", CK_REJECTED, "dimensioni", CERT("(= (rank " NINE ") 2)", "true", RANK_WIDE)},
    {"a rank witness without H", CK_REJECTED, "scritto male",
     CERT("(= (rank " WIDE ") 2)", "true", W("rank 2 (matrix 2 2 1 2 4 5) (matrix 2 3 1 0 -1 0 1 2) (matrix 2 2 (/ -5 3) (/ 2 3) (/ 4 3) (/ -1 3))"))},
    {"the rank of a number", CK_REJECTED, "non è una matrice", CERT("(= (rank 3) 1)", "true", NONE)},

    /* Il testimone di un'inversa che non torna. */
    {"a witness with one entry changed", CK_REJECTED, "il prodotto non è l'identità", CERT("(= (inv " A ") " A_INVERSE ")", "true", W("inverse (matrix 2 2 -2 1 (/ 3 2) (/ 1 2))"))},
    {"the transpose of the inverse as a witness", CK_REJECTED, "il prodotto non è l'identità", CERT("(= (inv " A ") " A_INVERSE ")", "true", W("inverse (matrix 2 2 -2 (/ 3 2) 1 (/ -1 2))"))},
    {"a witness for a singular matrix", CK_REJECTED, "il prodotto non è l'identità", CERT("(= (inv " SINGULAR ") (identity 2))", "true", W("inverse (identity 2)"))},
    {"a wrong witness, even if the statement is declared false", CK_REJECTED, "il prodotto non è l'identità", CERT("(= (inv " SINGULAR ") (identity 2))", "false", W("inverse (identity 2)"))},
    {"the inverse of another matrix", CK_REJECTED, "il prodotto non è l'identità", CERT("(= (inv " SHEAR ") " A_INVERSE ")", "true", INVERSE_A)},
    {"a right inverse of a matrix that is not square", CK_REJECTED, "non è una matrice quadrata", CERT("(= (inv (matrix 1 2 1 0)) (matrix 2 1 1 0))", "true", W("inverse (matrix 2 1 1 0)"))},
    {"a witness of another size", CK_REJECTED, "dimensioni", CERT("(= (inv " A ") (identity 3))", "true", W("inverse (identity 3)"))},
    {"a number as a witness", CK_REJECTED, "dimensioni", CERT("(= (inv " A ") 1)", "true", W("inverse 1"))},

    /* I testimoni e l'enunciato: uno per occorrenza, del tipo giusto,
     * nell'ordine giusto, e nessuno in più. */
    {"a witness of another kind", CK_REJECTED, "non è del tipo", CERT("(= (det " A ") -2)", "true", INVERSE_A)},
    {"a kind of witness that does not exist", CK_REJECTED, "non è del tipo", CERT("(= (det " A ") -2)", "true", W("gauss 1 2"))},
    {"two witnesses in the wrong order", CK_REJECTED, "non è del tipo", CERT("(= (+ (det " NINE ") (rank " NINE ")) 2)", "true", RANK_NINE LU_NINE)},
    {"two witnesses of the same kind in the wrong order", CK_REJECTED, "L U non è la matrice", CERT("(= (+ (det " A ") (det " SWAP ")) -3)", "true", LU_SWAP LU_A)},
    {"a witness that the statement does not use", CK_REJECTED, "non è usato", CERT("(= (det " A ") -2)", "true", LU_A LU_A)},
    {"a witness for a statement without determinants", CK_REJECTED, "non è usato", CERT("(= (trace " A ") 5)", "true", LU_A)},
    {"a determinant inside a witness", CK_REJECTED, "senza determinanti", CERT("(= (inv " A ") " A_INVERSE ")", "true", W("inverse (* (det " A ") " A_INVERSE ")"))},
    {"an inverse inside a witness", CK_REJECTED, "senza determinanti", CERT("(= (det " A ") -2)", "true", W("lu 1 2 (inv (matrix 2 2 1 0 -3 1)) (matrix 2 2 1 2 0 -2)"))},
    {"text after a witness", CK_REJECTED, "testo in più", CERT("(= (det " A ") -2)", "true", W("lu 1 2 (matrix 2 2 1 0 3 1) (matrix 2 2 1 2 0 -2) 7"))},
    {"a witness before the statement", CK_REJECTED, "manca l'enunciato", "certificate matrices\n" LU_A "statement (= (det " A ") -2)\nclaim true\nend\n"},
    {"a witness between the statement and the claim", CK_REJECTED, "manca la voce claim", "certificate matrices\nstatement (= (det " A ") -2)\n" LU_A "claim true\nend\n"},

    /* Operazioni non definite: l'espressione non ha un valore. */
    {"a sum of matrices of different shapes", CK_REJECTED, "dimensioni diverse", CERT("(= (+ " A " " WIDE ") " A ")", "true", NONE)},
    {"a sum of a number and a matrix", CK_REJECTED, "somma di un numero", CERT("(= (+ 1 " A ") " A ")", "false", NONE)},
    {"a product with the wrong shapes", CK_REJECTED, "non compatibili", CERT("(= (* " WIDE " " WIDE ") " WIDE ")", "false", NONE)},
    {"the trace of a matrix that is not square", CK_REJECTED, "non è una matrice quadrata", CERT("(= (trace " WIDE ") 5)", "false", NONE)},
    {"the transpose of a number", CK_REJECTED, "trasposta di un numero", CERT("(= (transpose 3) 3)", "true", NONE)},
    {"a power of a matrix that is not square", CK_REJECTED, "non quadrata", CERT("(= (^ " WIDE " 2) " WIDE ")", "false", NONE)},
    {"a negative exponent", CK_REJECTED, "esponente negativo", CERT("(= (^ " A " -1) " A_INVERSE ")", "true", NONE)},
    {"a division by zero", CK_REJECTED, "divisione per zero", CERT("(= (/ 1 (trace (zero 2 2))) 0)", "false", NONE)},
    {"a division of matrices", CK_REJECTED, "non è tra due numeri", CERT("(= (/ " A " " A ") (identity 2))", "true", NONE)},
    {"a matrix as an element of a matrix", CK_REJECTED, "un elemento", CERT("(= (matrix 1 1 " A ") " A ")", "true", NONE)},
    {"a number compared with a matrix", CK_REJECTED, "non si confrontano", CERT("(= " A " 0)", "false", NONE)},
    {"a 1 x 1 matrix compared with a number", CK_REJECTED, "non si confrontano", CERT("(= (matrix 1 1 5) 5)", "true", NONE)},
    {"an order between matrices", CK_REJECTED, "solo con = e neq", CERT("(< " A " " A ")", "false", NONE)},
    {"a matrix without rows", CK_REJECTED, "almeno una riga", CERT("(= (matrix 0 2) (matrix 0 2))", "true", NONE)},
    {"an identity without rows", CK_REJECTED, "almeno una riga", CERT("(= (identity 0) (identity 0))", "true", NONE)},

    /* Scritti male. */
    {"too few elements", CK_REJECTED, "scritto male", CERT("(= (trace (matrix 2 2 1 2 3)) 0)", "true", NONE)},
    {"too many elements", CK_REJECTED, "scritto male", CERT("(= (trace (matrix 2 2 1 2 3 4 5)) 0)", "false", NONE)},
    {"a missing operand", CK_REJECTED, "scritto male", CERT("(= (trace " A "))", "true", NONE)},
    {"a third operand", CK_REJECTED, "scritto male", CERT("(= (trace " A ") 5 5)", "true", NONE)},
    {"text after the statement", CK_REJECTED, "testo in più dopo l'enunciato", CERT("(= (trace " A ") 5) 5", "true", NONE)},
    {"an exponent that is not written as an integer", CK_REJECTED, "numero intero atteso", CERT("(= (^ " A " (+ 1 1)) " A ")", "true", NONE)},
    {"a name with letters where a number is expected", CK_REJECTED, "scritto male", CERT("(= (trace A) 5)", "true", NONE)},
    {"a claim that is neither true nor false", CK_REJECTED, "true oppure false", CERT("(= (trace " A ") 5)", "maybe", NONE)},
    {"no claim", CK_REJECTED, "manca la voce claim", "certificate matrices\nstatement (= (trace " A ") 5)\nend\n"},
    {"no statement", CK_REJECTED, "manca l'enunciato", "certificate matrices\nclaim true\nend\n"},
    {"no end", CK_REJECTED, "\"end\" mancante", "certificate matrices\nstatement (= (trace " A ") 5)\nclaim true\n"},
    {"an entry after the claim that is not a witness", CK_REJECTED, "\"end\" mancante", "certificate matrices\nstatement (= (trace " A ") 5)\nclaim true\nclaim false\nend\n"},

    /* Con le lettere (17d): un'uguaglianza tra polinomi, che vale per ogni
     * valore delle lettere. Il determinante si calcola con la sua
     * definizione, senza testimoni. */
    {"a determinant with a letter is a polynomial", CK_ACCEPTED, "", LCERT("k", "(= (det " KK ") (- (^ k 2) 1))", "true", NONE)},
    {"a characteristic polynomial: a multiple of the identity taken away", CK_ACCEPTED, "",
     LCERT("s", "(= (det (- (matrix 2 2 2 1 1 2) (* s (identity 2)))) (+ (- (^ s 2) (* 4 s)) 3))", "true", NONE)},
    {"two letters", CK_ACCEPTED, "",
     LCERT("k s", "(= (det (- (matrix 3 3 k 1 0 1 k 1 0 1 k) (* s (identity 3)))) (- (^ (- k s) 3) (* 2 (- k s))))", "true", NONE)},
    {"a 3 x 3 determinant whose terms have both signs", CK_ACCEPTED, "", LCERT("k", "(= (det (matrix 3 3 0 k 1 1 0 k k 1 0)) (+ (^ k 3) 1))", "true", NONE)},
    {"an odd permutation alone: the sign in the definition", CK_ACCEPTED, "", LCERT("k", "(= (det (matrix 2 2 0 k 1 0)) (neg k))", "true", NONE)},
    {"a 4 x 4 determinant", CK_ACCEPTED, "",
     LCERT("t", "(= (det (matrix 4 4 t 1 0 0 1 t 1 0 0 1 t 1 0 0 1 t)) (+ (- (^ t 4) (* 3 (^ t 2))) 1))", "true", NONE)},
    {"a 1 x 1 determinant with a letter", CK_ACCEPTED, "", LCERT("k", "(= (det (matrix 1 1 k)) k)", "true", NONE)},
    {"a determinant that is the zero polynomial", CK_ACCEPTED, "", LCERT("k", "(= (det (matrix 2 2 k k 1 1)) 0)", "true", NONE)},
    {"a trace of a power, with a coefficient that is a fraction", CK_ACCEPTED, "",
     LCERT("k", "(= (trace (^ (matrix 2 2 k 1 0 (/ k 2)) 2)) (* (/ 5 4) (^ k 2)))", "true", NONE)},
    {"the determinant of a transpose, with a letter", CK_ACCEPTED, "",
     LCERT("k", "(= (det (transpose (matrix 2 2 k 1 2 3))) (det (matrix 2 2 k 1 2 3)))", "true", NONE)},
    {"the determinant of a product, on one pair of matrices with a letter", CK_ACCEPTED, "",
     LCERT("k", "(= (det (* (matrix 2 2 k 1 2 3) (matrix 2 2 1 k 0 2))) (* (det (matrix 2 2 k 1 2 3)) (det (matrix 2 2 1 k 0 2))))", "true", NONE)},
    {"an equality between matrices of polynomials", CK_ACCEPTED, "",
     LCERT("k", "(= (^ (matrix 2 2 k 1 0 1) 2) (matrix 2 2 (^ k 2) (+ k 1) 0 1))", "true", NONE)},
    {"the zeroth power of a matrix with a letter", CK_ACCEPTED, "", LCERT("k", "(= (^ " KK " 0) (identity 2))", "true", NONE)},
    {"a letter that the statement does not use", CK_ACCEPTED, "", LCERT("k s", "(= (det " KK ") (- (^ k 2) 1))", "true", NONE)},
    {"numbers only, in a certificate with letters: the definition, without a witness", CK_ACCEPTED, "", LCERT("k", "(= (det " A ") -2)", "true", NONE)},

    /* Con le lettere, sbagliati: un altro polinomio, un'operazione non
     * definita, la voce letters scritta male. */
    {"a polynomial with one coefficient changed", CK_REJECTED, "la relazione non vale", LCERT("k", "(= (det " KK ") (- (^ k 2) 2))", "true", NONE)},
    {"a polynomial with the wrong degree", CK_REJECTED, "la relazione non vale", LCERT("k", "(= (det " KK ") (- (^ k 3) 1))", "true", NONE)},
    {"the sign of an odd permutation forgotten", CK_REJECTED, "la relazione non vale", LCERT("k", "(= (det (matrix 2 2 0 k 1 0)) k)", "true", NONE)},
    {"a term of a 3 x 3 determinant with the wrong sign", CK_REJECTED, "la relazione non vale",
     LCERT("k", "(= (det (matrix 3 3 0 k 1 1 0 k k 1 0)) (- (^ k 3) 1))", "true", NONE)},
    {"the polynomial of another matrix", CK_REJECTED, "la relazione non vale", LCERT("k", "(= (det (matrix 2 2 k 2 1 k)) (- (^ k 2) 1))", "true", NONE)},
    {"a polynomial that is right only for some values of the letter", CK_REJECTED, "la relazione non vale", LCERT("k", "(= (det " KK ") (- k 1))", "true", NONE)},
    {"the same polynomials in a table of another shape", CK_REJECTED, "la relazione non vale", LCERT("k", "(= (matrix 1 2 k 1) (matrix 2 1 k 1))", "true", NONE)},
    {"a polynomial compared with a 1 x 1 matrix", CK_REJECTED, "non si confrontano", LCERT("k", "(= (matrix 1 1 k) k)", "true", NONE)},
    {"a sum of matrices of different shapes, with a letter", CK_REJECTED, "dimensioni diverse", LCERT("k", "(= (+ " KK " (matrix 1 2 k 1)) " KK ")", "true", NONE)},
    {"a sum of a polynomial and a matrix", CK_REJECTED, "somma di un numero", LCERT("k", "(= (+ k " KK ") " KK ")", "true", NONE)},
    {"a product with the wrong shapes, with a letter", CK_REJECTED, "non compatibili", LCERT("k", "(= (* (matrix 1 2 k 1) (matrix 1 2 k 1)) k)", "true", NONE)},
    {"the determinant of a matrix that is not square, with a letter", CK_REJECTED, "non è una matrice quadrata", LCERT("k", "(= (det (matrix 1 2 k 1)) k)", "true", NONE)},
    {"the determinant of a polynomial", CK_REJECTED, "non è una matrice quadrata", LCERT("k", "(= (det k) k)", "true", NONE)},
    {"the trace of a matrix that is not square, with a letter", CK_REJECTED, "non è una matrice quadrata", LCERT("k", "(= (trace (matrix 1 2 k 1)) k)", "true", NONE)},
    {"the transpose of a polynomial", CK_REJECTED, "trasposta di un numero", LCERT("k", "(= (transpose k) k)", "true", NONE)},
    {"a power of a matrix that is not square, with a letter", CK_REJECTED, "non quadrata", LCERT("k", "(= (^ (matrix 1 2 k 1) 2) k)", "true", NONE)},
    {"a negative exponent, with a letter", CK_REJECTED, "esponente negativo", LCERT("k", "(= (^ k -1) k)", "true", NONE)},
    {"a division by the number zero, with a letter", CK_REJECTED, "divisione per zero", LCERT("k", "(= (/ k 0) k)", "true", NONE)},
    {"a division of matrices, with a letter", CK_REJECTED, "non è tra due numeri", LCERT("k", "(= (/ " KK " " KK ") 1)", "true", NONE)},
    {"a matrix as an element of a matrix, with a letter", CK_REJECTED, "un elemento", LCERT("k", "(= (matrix 1 1 " KK ") " KK ")", "true", NONE)},
    {"a matrix without rows is not a polynomial", CK_REJECTED, "almeno una riga", LCERT("k", "(= (matrix 0 0) k)", "true", NONE)},
    {"an identity without rows, with a letter", CK_REJECTED, "almeno una riga", LCERT("k", "(= (identity 0) k)", "true", NONE)},
    {"a letter that is not declared", CK_REJECTED, "espressione attesa", LCERT("k", "(= (det (matrix 2 2 s 1 1 s)) (- (^ s 2) 1))", "true", NONE)},
    {"a letter in a certificate without letters", CK_REJECTED, "espressione attesa", CERT("(= (det " KK ") -1)", "true", NONE)},
    {"a witness in a certificate with letters", CK_REJECTED, "non è usato", LCERT("k", "(= (det " KK ") (- (^ k 2) 1))", "true", LU_A)},
    {"the same letter twice", CK_REJECTED, "lettere singole", LCERT("k k", "(= k k)", "true", NONE)},
    {"a word in place of a letter", CK_REJECTED, "lettere singole", LCERT("kk", "(= 1 1)", "true", NONE)},
    {"a digit in place of a letter", CK_REJECTED, "lettere singole", LCERT("2", "(= 2 2)", "true", NONE)},
    {"no letter after the word letters", CK_REJECTED, "nessuna lettera", "certificate matrices\nletters\nstatement (= 1 1)\nclaim true\nend\n"},
    {"the letters after the statement", CK_REJECTED, "manca la voce claim", "certificate matrices\nstatement (= k k)\nletters k\nclaim true\nend\n"},
    {"text after the statement, with a letter", CK_REJECTED, "testo in più dopo l'enunciato", LCERT("k", "(= k k) k", "true", NONE)},
    {"a missing operand, with a letter", CK_REJECTED, "scritto male", LCERT("k", "(= (det " KK "))", "true", NONE)},

    /* Con le lettere, ciò che il verificatore non tratta: non deve mai
     * diventare un "accettato". */
    {"a claim false with letters: two different polynomials do not say for which values", CK_UNSUPPORTED, "dichiarata vera",
     LCERT("k", "(= (det " KK ") (^ k 2))", "false", NONE)},
    {"a true equality declared false, with letters", CK_UNSUPPORTED, "dichiarata vera", LCERT("k", "(= (det " KK ") (- (^ k 2) 1))", "false", NONE)},
    {"neq with letters", CK_UNSUPPORTED, "solo un'uguaglianza", LCERT("k", "(neq (det " KK ") 0)", "true", NONE)},
    {"an order with letters", CK_UNSUPPORTED, "solo un'uguaglianza", LCERT("k", "(< (det (matrix 2 2 k 1 -1 k)) 0)", "true", NONE)},
    {"a rank with a letter", CK_UNSUPPORTED, "il rango e l'inversa", LCERT("k", "(= (rank " KK ") 2)", "true", NONE)},
    {"an inverse with a letter", CK_UNSUPPORTED, "il rango e l'inversa", LCERT("k", "(= (inv " KK ") " KK ")", "true", NONE)},
    {"the inverse of a matrix of numbers, in a certificate with letters", CK_UNSUPPORTED, "il rango e l'inversa", LCERT("k", "(= (inv " A ") " A_INVERSE ")", "true", INVERSE_A)},
    {"a division by a polynomial", CK_UNSUPPORTED, "divide solo per un numero", LCERT("k", "(= (/ 1 k) (/ 1 k))", "true", NONE)},
    {"more letters than the checker has indeterminates", CK_UNSUPPORTED, "troppe lettere",
     LCERT("a b c d e f g h i j k l m n o p q r s t u v w x y", "(= a a)", "true", NONE)},
    {"an exponent beyond the limit of a polynomial", CK_UNSUPPORTED, "esponenti troppo grandi", LCERT("k", "(= (^ k 200) (^ k 200))", "true", NONE)},
    {"a word that the checker does not know, with a letter", CK_UNSUPPORTED, "operatore che il verificatore non conosce", LCERT("k", "(= (adjugate " KK ") " KK ")", "true", NONE)},

    /* Ciò che il verificatore non conosce, o non riesce a calcolare. Un
     * testimone che manca è tra questi: il verificatore non lo cerca. */
    {"a determinant without its witness", CK_UNSUPPORTED, "manca il testimone", CERT("(= (det " A ") -2)", "true", NONE)},
    {"a rank without its witness", CK_UNSUPPORTED, "manca il testimone", CERT("(= (rank " A ") 2)", "true", NONE)},
    {"an inverse without its witness", CK_UNSUPPORTED, "manca il testimone", CERT("(= (inv " A ") " A_INVERSE ")", "true", NONE)},
    {"the witness written inside the statement, as an operand", CK_UNSUPPORTED, "manca il testimone", CERT("(= (inv " A " " A_INVERSE ") " A_INVERSE ")", "true", NONE)},
    {"two determinants and one witness", CK_UNSUPPORTED, "manca il testimone", CERT("(= (* (det " A ") (det " A ")) 4)", "true", LU_A)},
    {"a word that the checker does not know", CK_UNSUPPORTED, "operatore che il verificatore non conosce", CERT("(= (adjugate " A ") " A ")", "true", NONE)},
    {"a relation that the checker does not know", CK_UNSUPPORTED, "relazione che il verificatore non conosce", CERT("(> (trace " A ") 0)", "true", NONE)},
    {"a power beyond 128 bits", CK_UNSUPPORTED, "troppo grandi", CERT("(= (^ 1000000007 20) 0)", "false", NONE)},
    {"an integer too long to read", CK_UNSUPPORTED, "numero troppo grande", CERT("(= 123456789012345678901234567890 0)", "false", NONE)},
    {"an exponent beyond the limit", CK_UNSUPPORTED, "esponente troppo grande", CERT("(= (^ " A " 1000) " A ")", "false", NONE)},
    {"a matrix with too many rows", CK_UNSUPPORTED, "matrice troppo grande", CERT("(= (identity 5000) (identity 5000))", "true", NONE)},
    {"a matrix beyond the memory of the checker", CK_UNSUPPORTED, "memoria", CERT("(= (identity 3000) (identity 3000))", "true", NONE)},
    {"a product beyond the work limit", CK_UNSUPPORTED, "conto sulle matrici troppo lungo", CERT("(= (^ (identity 400) 2) (identity 400))", "true", NONE)},
};

static int checks, failures;

/* Registra l'esito di un controllo e stampa quelli falliti. */
static void check(int passed, const char *description) {
    checks++;
    if (passed) return;
    failures++;
    printf("  FAILED: %s\n", description);
}

/* Ritorna il nome inglese di una risposta, per i messaggi. */
static const char *answer_name(CkAnswer answer) {
    return answer == CK_ACCEPTED ? "accepted" : answer == CK_REJECTED ? "rejected" : "unsupported";
}

int main(void) {
    static char nested[8 * (CK_MAX_DEPTH + 8) + 128];
    char message[512];
    const char *why;

    /* La tabella: risposta attesa, e il motivo per chi non è accettato. */
    for (size_t i = 0; i < sizeof cases / sizeof *cases; i++) {
        const char *text = cases[i].text;
        CkAnswer answer = ck_check(&text, &why, 0);
        snprintf(message, sizeof message, "\"%s\" should be %s but is %s (%s)", cases[i].name,
                 answer_name(cases[i].expected), answer_name(answer), why);
        check(answer == cases[i].expected, message);
        snprintf(message, sizeof message, "\"%s\" should mention \"%s\" as its reason, not \"%s\"", cases[i].name, cases[i].why, why);
        check(answer != cases[i].expected || strstr(why, cases[i].why) != NULL, message);
    }

    /* Troppe parentesi una dentro l'altra: (- (- ... 1 ...)) con più livelli
     * di quanti il lettore ne segua. Non deve esaurire la pila. */
    size_t length = (size_t)snprintf(nested, sizeof nested, "certificate matrices\nstatement (= ");
    for (int i = 0; i < CK_MAX_DEPTH + 4; i++) length += (size_t)snprintf(nested + length, sizeof nested - length, "(- ");
    length += (size_t)snprintf(nested + length, sizeof nested - length, "1");
    for (int i = 0; i < CK_MAX_DEPTH + 4; i++) length += (size_t)snprintf(nested + length, sizeof nested - length, " 0)");
    snprintf(nested + length, sizeof nested - length, " 1)\nclaim true\nend\n");
    const char *deep = nested;
    check(ck_check(&deep, &why, 0) == CK_UNSUPPORTED && strstr(why, "troppe parentesi") != NULL,
          "an expression nested beyond the limit should be unsupported, not crash");

    /* Due certificati in un testo si leggono uno dopo l'altro: il primo,
     * letto fino in fondo e respinto, lascia il lettore sul secondo, e i
     * testimoni dell'uno non servono all'altro. */
    const char *two = CERT("(= (det " A ") 2)", "true", LU_A) CERT("(= (det " SWAP ") -1)", "true", LU_SWAP);
    check(ck_check(&two, &why, 0) == CK_REJECTED, "the first of two certificates should be rejected");
    check(ck_check(&two, &why, 0) == CK_ACCEPTED, "the second of two certificates should be read next, and accepted");

    /* Un certificato non lascia tracce nel successivo: dopo uno fermato
     * perché il conto è troppo lungo, il conto di uno piccolo riparte da
     * zero; e dopo uno fermato a metà di un testimone, il successivo si
     * legge come un enunciato, non come un testimone. */
    const char *heavy = CERT("(= (^ (identity 400) 2) (identity 400))", "true", NONE), *light = CERT("(= (det " A ") -2)", "true", LU_A);
    check(ck_check(&heavy, &why, 0) == CK_UNSUPPORTED, "a certificate beyond the work limit should be unsupported");
    check(ck_check(&light, &why, 0) == CK_ACCEPTED, "a small certificate after one beyond the work limit should be accepted");
    const char *broken = CERT("(= (inv " A ") " A_INVERSE ")", "true", W("inverse (matrix 2 2 -2 1")), *whole = CERT("(= (det " A ") -2)", "true", LU_A);
    check(ck_check(&broken, &why, 0) == CK_REJECTED, "a certificate that stops inside a witness should be rejected");
    check(ck_check(&whole, &why, 0) == CK_ACCEPTED, "a certificate after one that stopped inside a witness should be accepted");

    /* Le lettere di un certificato non restano al successivo: dopo uno con
     * la voce letters, uno senza non può usare quella lettera. */
    const char *lettered = LCERT("k", "(= (det " KK ") (- (^ k 2) 1))", "true", NONE) CERT("(= (det " KK ") -1)", "true", NONE);
    check(ck_check(&lettered, &why, 0) == CK_ACCEPTED, "a certificate with letters should be accepted");
    check(ck_check(&lettered, &why, 0) == CK_REJECTED && strstr(why, "espressione attesa") != NULL,
          "a certificate without letters after one with letters should not know the letter");

    /* Un determinante con le lettere troppo grande per la sua definizione:
     * una matrice 11 x 11 di tutte k ha 11! termini, tutti da calcolare. Il
     * conto si ferma al suo tetto, o alla memoria, e la risposta è "non so
     * controllarlo": mai un sì, anche se quel determinante vale zero. */
    static char dense[1024];
    length = (size_t)snprintf(dense, sizeof dense, "certificate matrices\nletters k\nstatement (= (det (matrix 11 11");
    for (int i = 0; i < 121; i++) length += (size_t)snprintf(dense + length, sizeof dense - length, " k");
    snprintf(dense + length, sizeof dense - length, ")) 0)\nclaim true\nend\n");
    const char *large = dense;
    check(ck_check(&large, &why, 0) == CK_UNSUPPORTED, "a determinant with letters beyond the work limit should be unsupported");

    /* Lo stesso certificato dà la stessa risposta se lo si controlla due
     * volte. */
    for (int round = 0; round < 2; round++) {
        const char *again = CERT("(= (+ (det " NINE ") (rank " NINE ")) 2)", "true", LU_NINE RANK_NINE);
        check(ck_check(&again, &why, 0) == CK_ACCEPTED, "a certificate checked twice should be accepted twice");
    }

    printf("%s: %d of %d matrix certificate checks passed\n", failures ? "FAIL" : "PASS", checks - failures, checks);
    return failures ? 1 : 0;
}
