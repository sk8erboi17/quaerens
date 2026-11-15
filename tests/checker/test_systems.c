/* Test del verificatore sui certificati dei sistemi lineari e della voce
 * where (17d e 17f di checker.c), da solo: qui il motore non c'entra.
 *
 * Compilazione ed esecuzione, dalla cartella principale del progetto:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/checker checker.c && \
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_systems tests/checker/test_systems.c && /tmp/test_systems
 *
 * Come in test_checker.c, il primo comando fa parte del test: checker.c si
 * compila da solo, e questo file include soltanto checker.c.
 *
 * Un enunciato su un sistema dice qualcosa dell'insieme delle soluzioni di
 * A x = b: che un vettore ci sta, che è vuoto, che è p + U t. Il
 * verificatore non risolve il sistema: moltiplica e confronta, e ciò che il
 * motore ha trovato eliminando lo riceve come testimone. I certificati
 * accettati sono quelli che il motore scrive (33d di main.c), copiati qui
 * come dati, o scritti a mano con un testimone diverso ma giusto; gli altri
 * ne sono alterazioni. Ognuno ha la risposta attesa e, se non è accettato,
 * un pezzo del motivo:
 *
 *   - accettati: un vettore che è una soluzione, e uno che non lo è detto
 *     tale; il sistema impossibile, con la combinazione delle equazioni che
 *     dà 0 = c; il sistema con una soluzione sola; le soluzioni con uno e
 *     con più parametri, anche scritte con un'altra soluzione particolare,
 *     con altri vettori, con vettori di troppo; un altro testimone giusto;
 *     gli enunciati falsi dichiarati falsi, nei loro quattro modi;
 *   - respinti: un numero di un testimone cambiato; una combinazione che non
 *     annulla le equazioni, o che dà 0 = 0; una "soluzione" che non lo è;
 *     un insieme scritto troppo piccolo, o troppo grande, detto uguale; una
 *     soluzione "in più" che sta nell'insieme scritto; la verità rovesciata;
 *     le dimensioni che non tornano; un testimone in più, o con un
 *     determinante dentro; un certificato scritto male;
 *   - non controllabili: un testimone che manca o di un tipo sconosciuto,
 *     un insieme scritto per primo, numeri oltre i 128 bit.
 *
 * Con la voce where un certificato con le lettere parla solo dei valori che
 * non annullano un polinomio dichiarato, e lì può dividere per quel
 * polinomio, e trattare il rango e l'inversa con i loro testimoni:
 *
 *   - accettati: la soluzione di un sistema con un parametro, scritta con il
 *     parametro a denominatore; il sistema impossibile fuori da un valore;
 *     il rango e l'inversa di una matrice con una lettera; un insieme
 *     scritto diverso dalle soluzioni in ogni punto;
 *   - respinti: il polinomio nullo come voce where, che non direbbe nulla;
 *     la voce where senza lettere, o scritta male; i testimoni alterati; un
 *     rango più alto o più basso di uno;
 *   - non controllabili: una divisione per un polinomio che non divide una
 *     potenza di quello dichiarato; un'uguaglianza, o una differenza, che
 *     vale solo per qualche valore della lettera; il polinomio dichiarato
 *     sbagliato; il rango e l'inversa senza la voce where.
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

/* Un certificato con l'enunciato, la verità dichiarata e i testimoni; una
 * voce witness; NONE è l'elenco vuoto dei testimoni. LCERT ha in più le
 * lettere, WCERT le lettere e la voce where. */
#define CERT(statement, claim, witnesses) "certificate matrices\nstatement " statement "\nclaim " claim "\n" witnesses "end\n"
#define LCERT(letters, statement, claim, witnesses) \
    "certificate matrices\nletters " letters "\nstatement " statement "\nclaim " claim "\n" witnesses "end\n"
#define WCERT(letters, where, statement, claim, witnesses) \
    "certificate matrices\nletters " letters "\nwhere " where "\nstatement " statement "\nclaim " claim "\n" witnesses "end\n"
#define W(text) "witness " text "\n"
#define NONE ""

/* I sistemi usati più volte, come (solutions A b).
 *
 *   ONE:    x + y = 1, x - y = 3: la sola soluzione (2, -1);
 *   LINE:   x + y + z = 1, 3x + 3y - 2z = 2: una retta;
 *   PLANE:  x + y + z = 1: un piano;
 *   OVER:   2x + 3y = 5, 2x + 4y = 5, y = 0: tre equazioni, una soluzione;
 *   NONE3:  2x + 3y = 5, 2x + 4y = 5, 6x + 9y = 7: nessuna soluzione;
 *   SAME2:  3x - y = 0, -6x + 2y = 0: la retta y = 3x. */
#define ONE "(solutions (matrix 2 2 1 1 1 -1) (matrix 2 1 1 3))"
#define LINE "(solutions (matrix 2 3 1 1 1 3 3 -2) (matrix 2 1 1 2))"
#define PLANE "(solutions (matrix 1 3 1 1 1) (matrix 1 1 1))"
#define OVER "(solutions (matrix 3 2 2 3 2 4 0 1) (matrix 3 1 5 5 0))"
#define NONE3 "(solutions (matrix 3 2 2 3 2 4 6 9) (matrix 3 1 5 5 7))"
#define SAME2 "(solutions (matrix 2 2 3 -1 -6 2) (matrix 2 1 0 0))"

/* I loro insiemi scritti e i loro testimoni, come li scrive il motore. */
#define ONE_SET "(affine (matrix 2 1 2 -1))"
#define ONE_SPAN W("span (matrix 2 2 (/ 1 2) (/ 1 2) (/ 1 2) (/ -1 2))")
#define LINE_SET "(affine (matrix 3 1 (/ 4 5) 0 (/ 1 5)) (matrix 3 1 -1 1 0))"
#define LINE_SPAN W("span (matrix 3 2 (/ 2 5) (/ 1 5) 0 0 (/ 3 5) (/ -1 5)) (matrix 1 3 0 1 0)")
#define LINE_BOOK "(affine (matrix 3 1 0 (/ 4 5) (/ 1 5)) (matrix 3 1 1 -1 0))"  /* (t, 4/5 - t, 1/5) */
#define LINE_BOOK_SPAN W("span (matrix 3 2 (/ 2 5) (/ 1 5) 0 0 (/ 3 5) (/ -1 5)) (matrix 1 3 0 -1 0)")
#define PLANE_THREE "(affine (matrix 3 1 1 0 0) (matrix 3 3 1 0 1 -1 1 0 0 -1 -1))"  /* tre vettori per un piano */
#define PLANE_THREE_SPAN W("span (matrix 3 1 1 0 0) (matrix 3 3 0 -1 -1 0 0 -1 0 0 0)")
#define PLANE_LINE "(affine (matrix 3 1 1 0 0) (matrix 3 1 1 -1 0))"  /* una retta del piano */
#define OVER_SET "(affine (matrix 2 1 (/ 5 2) 0))"
#define OVER_SPAN W("span (matrix 2 3 2 (/ -3 2) 0 -1 1 0)")
#define NONE3_ROW W("combination (matrix 1 3 -3 0 1)")

/* Con una lettera. KK x = (1, 1) con KK = (k 1; 1 k): per k diverso da 1 e
 * da -1 la sola soluzione è (1/(k + 1), 1/(k + 1)); D è il determinante. */
#define KK "(matrix 2 2 k 1 1 k)"
#define D "(- (^ k 2) 1)"
#define KK_SYSTEM "(solutions " KK " (matrix 2 1 1 1))"
#define KK_SET "(affine (matrix 2 1 (/ 1 (+ k 1)) (/ 1 (+ k 1))))"
#define KK_INVERSE "(* (/ 1 " D ") (matrix 2 2 k -1 -1 k))"
#define KK_RANK W("rank 2 " KK " (identity 2) " KK_INVERSE " (identity 2)")

/* x + y = 1: una retta, e i vettori (k, 1 - k) ci stanno per ogni k. */
#define SUM1 "(solutions (matrix 1 2 1 1) (matrix 1 1 1))"

/* x + y = 1, x + y = k: impossibile per k diverso da 1. */
#define TWICE "(solutions (matrix 2 2 1 1 1 1) (matrix 2 1 1 k))"
#define TWICE_ROW W("combination (matrix 1 2 1 -1)")

static const Case cases[] = {
    /* (M) Un vettore e la sua appartenenza: nessun testimone. */
    {"a vector that is a solution", CK_ACCEPTED, "", CERT("(in (matrix 2 1 2 -1) " ONE ")", "true", NONE)},
    {"a vector that is not a solution, said to be one", CK_ACCEPTED, "", CERT("(in (matrix 2 1 1 1) " ONE ")", "false", NONE)},
    {"a vector that is not a solution, said not to be one", CK_ACCEPTED, "", CERT("(notin (matrix 2 1 1 1) " ONE ")", "true", NONE)},
    {"a solution said not to be one, declared false", CK_ACCEPTED, "", CERT("(notin (matrix 2 1 2 -1) " ONE ")", "false", NONE)},
    {"a point of a line of solutions", CK_ACCEPTED, "", CERT("(in (matrix 3 1 0 (/ 4 5) (/ 1 5)) " LINE ")", "true", NONE)},
    {"a vector that is not a solution, declared one", CK_REJECTED, "dichiara vera", CERT("(in (matrix 2 1 1 1) " ONE ")", "true", NONE)},
    {"a solution declared not to be one", CK_REJECTED, "dichiara falsa", CERT("(in (matrix 2 1 2 -1) " ONE ")", "false", NONE)},
    {"a vector that satisfies only the first equation", CK_REJECTED, "dichiara vera", CERT("(in (matrix 2 1 1 0) " ONE ")", "true", NONE)},
    {"a witness with a membership, which needs none", CK_REJECTED, "non è usato", CERT("(in (matrix 2 1 2 -1) " ONE ")", "true", ONE_SPAN)},

    /* (V) e (N) L'insieme vuoto. */
    {"an impossible system, with the combination that gives 0 = c", CK_ACCEPTED, "", CERT("(= " NONE3 " (nothing))", "true", NONE3_ROW)},
    {"an impossible system said to have solutions, declared false", CK_ACCEPTED, "", CERT("(neq " NONE3 " (nothing))", "false", NONE3_ROW)},
    {"another right combination for the same system", CK_ACCEPTED, "", CERT("(= " NONE3 " (nothing))", "true", W("combination (matrix 1 3 6 0 -2)"))},
    {"a system with solutions said to be impossible, declared false", CK_ACCEPTED, "", CERT("(= " SAME2 " (nothing))", "false", W("solution (matrix 2 1 0 0)"))},
    {"a system with solutions said to have some", CK_ACCEPTED, "", CERT("(neq " SAME2 " (nothing))", "true", W("solution (matrix 2 1 1 3)"))},
    {"0 = 1 has no solutions", CK_ACCEPTED, "", CERT("(= (solutions (matrix 1 3 0 0 0) (matrix 1 1 1)) (nothing))", "true", W("combination (matrix 1 1 1)"))},
    {"a combination that does not cancel the unknowns", CK_REJECTED, "y A non è zero", CERT("(= " NONE3 " (nothing))", "true", W("combination (matrix 1 3 -3 0 2)"))},
    {"a combination that gives 0 = 0", CK_REJECTED, "y b è zero", CERT("(= " NONE3 " (nothing))", "true", W("combination (matrix 1 3 0 0 0)"))},
    {"a combination that gives 0 = 0 for a system with solutions", CK_REJECTED, "y b è zero", CERT("(= " SAME2 " (nothing))", "true", W("combination (matrix 1 2 2 1)"))},
    {"a solution that is not one, for a system said to have some", CK_REJECTED, "A z non è b", CERT("(neq " NONE3 " (nothing))", "true", W("solution (matrix 2 1 1 1)"))},
    {"an impossible system declared to have solutions", CK_REJECTED, "dichiara falsa", CERT("(= " NONE3 " (nothing))", "false", NONE3_ROW)},
    {"a system with solutions declared impossible", CK_REJECTED, "dichiara vera", CERT("(= " SAME2 " (nothing))", "true", W("solution (matrix 2 1 0 0)"))},
    {"a combination of the wrong length", CK_REJECTED, "dimensioni", CERT("(= " NONE3 " (nothing))", "true", W("combination (matrix 1 2 -3 1)"))},
    {"a combination written as a column", CK_REJECTED, "dimensioni", CERT("(= " NONE3 " (nothing))", "true", W("combination (matrix 3 1 -3 0 1)"))},
    {"an impossible system without its witness", CK_UNSUPPORTED, "manca il testimone", CERT("(= " NONE3 " (nothing))", "true", NONE)},
    {"an impossible system with the witness of a set of solutions", CK_UNSUPPORTED, "manca il testimone", CERT("(= " NONE3 " (nothing))", "true", ONE_SPAN)},

    /* (U) Le soluzioni sono quelle scritte. */
    {"one solution", CK_ACCEPTED, "", CERT("(= " ONE " " ONE_SET ")", "true", ONE_SPAN)},
    {"one solution of three equations", CK_ACCEPTED, "", CERT("(= " OVER " " OVER_SET ")", "true", OVER_SPAN)},
    {"another right witness: a left inverse is not unique", CK_ACCEPTED, "", CERT("(= " OVER " " OVER_SET ")", "true", W("span (matrix 2 3 3 (/ -5 2) 1 -1 1 0)"))},
    {"a line of solutions", CK_ACCEPTED, "", CERT("(= " LINE " " LINE_SET ")", "true", LINE_SPAN)},
    {"the same line with another point and the opposite direction", CK_ACCEPTED, "", CERT("(= " LINE " " LINE_BOOK ")", "true", LINE_BOOK_SPAN)},
    {"a plane written with three vectors", CK_ACCEPTED, "", CERT("(= " PLANE " " PLANE_THREE ")", "true", PLANE_THREE_SPAN)},
    {"every vector is a solution of 0 = 0", CK_ACCEPTED, "",
     CERT("(= (solutions (matrix 1 3 0 0 0) (matrix 1 1 0)) (affine (matrix 3 1 0 0 0) (identity 3)))", "true", W("span (zero 3 1) (identity 3)"))},
    {"the right set said to be different, declared false", CK_ACCEPTED, "", CERT("(neq " LINE " " LINE_SET ")", "false", LINE_SPAN)},
    {"an element of G changed", CK_REJECTED, "G A + U T non è l'identità", CERT("(= " ONE " " ONE_SET ")", "true", W("span (matrix 2 2 (/ 1 2) (/ 1 2) (/ 1 2) (/ -1 3))"))},
    {"an element of T changed", CK_REJECTED, "G A + U T non è l'identità",
     CERT("(= " LINE " " LINE_SET ")", "true", W("span (matrix 3 2 (/ 2 5) (/ 1 5) 0 0 (/ 3 5) (/ -1 5)) (matrix 1 3 0 2 0)"))},
    {"the witness of another parametrization of the same line", CK_REJECTED, "G A + U T non è l'identità", CERT("(= " LINE " " LINE_SET ")", "true", LINE_BOOK_SPAN)},
    {"T missing where the written set has directions", CK_REJECTED, "espressione attesa",
     CERT("(= " LINE " " LINE_SET ")", "true", W("span (matrix 3 2 (/ 2 5) (/ 1 5) 0 0 (/ 3 5) (/ -1 5))"))},
    {"T given where the written set is one point", CK_REJECTED, "testo in più dopo un testimone", CERT("(= " ONE " " ONE_SET ")", "true", W("span (matrix 2 2 (/ 1 2) (/ 1 2) (/ 1 2) (/ -1 2)) (matrix 1 2 0 0)"))},
    {"a line of the plane said to be the whole plane: no G and T can pass", CK_REJECTED, "G A + U T non è l'identità",
     CERT("(= " PLANE " " PLANE_LINE ")", "true", W("span (matrix 3 1 1 0 0) (matrix 1 3 0 -1 0)"))},
    {"one point of a line said to be all the solutions", CK_REJECTED, "G A + U T non è l'identità",
     CERT("(= " LINE " (affine (matrix 3 1 (/ 4 5) 0 (/ 1 5))))", "true", W("span (matrix 3 2 (/ 2 5) (/ 1 5) 0 0 (/ 3 5) (/ -1 5))"))},
    {"a point that is not a solution, with the right witness of the system", CK_REJECTED, "dichiara vera", CERT("(= " ONE " (affine (matrix 2 1 2 1)))", "true", NONE)},
    {"a set with a wrong component of the point", CK_REJECTED, "dichiara vera", CERT("(= " LINE " (affine (matrix 3 1 (/ 4 5) 0 (/ 2 5)) (matrix 3 1 -1 1 0)))", "true", NONE)},
    {"a set with a direction that is not a solution of the homogeneous system", CK_REJECTED, "dichiara vera",
     CERT("(= " LINE " (affine (matrix 3 1 (/ 4 5) 0 (/ 1 5)) (matrix 3 1 -1 1 1)))", "true", NONE)},
    {"the right set declared wrong", CK_REJECTED, "dichiara falsa", CERT("(= " LINE " " LINE_SET ")", "false", LINE_SPAN)},
    {"the right set without its witness", CK_UNSUPPORTED, "manca il testimone", CERT("(= " LINE " " LINE_SET ")", "true", NONE)},
    {"G with the wrong shape", CK_REJECTED, "dimensioni", CERT("(= " ONE " " ONE_SET ")", "true", W("span (matrix 2 3 1 0 0 0 1 0)"))},
    {"a determinant inside a witness", CK_REJECTED, "senza determinanti", CERT("(= " ONE " " ONE_SET ")", "true", W("span (* (det (identity 2)) (matrix 2 2 (/ 1 2) (/ 1 2) (/ 1 2) (/ -1 2)))"))},
    {"a witness that is not used", CK_REJECTED, "non è usato", CERT("(= " ONE " " ONE_SET ")", "true", ONE_SPAN ONE_SPAN)},

    /* (D) Le soluzioni non sono quelle scritte. */
    {"a point that is not a solution: no witness is needed", CK_ACCEPTED, "", CERT("(= " ONE " (affine (matrix 2 1 2 1)))", "false", NONE)},
    {"a wrong component of the point of a line", CK_ACCEPTED, "", CERT("(= " LINE " (affine (matrix 3 1 (/ 4 5) 0 (/ 2 5)) (matrix 3 1 -1 1 0)))", "false", NONE)},
    {"a direction that is not a solution of the homogeneous system", CK_ACCEPTED, "",
     CERT("(neq " LINE " (affine (matrix 3 1 (/ 4 5) 0 (/ 1 5)) (matrix 3 1 -1 1 1)))", "true", NONE)},
    {"a line of the plane is not the plane: a solution outside, and the row that shows it", CK_ACCEPTED, "",
     CERT("(= " PLANE " " PLANE_LINE ")", "false", W("solution (matrix 3 1 0 0 1)") W("combination (matrix 1 3 1 1 0)"))},
    {"one point of a line is not the line: another solution", CK_ACCEPTED, "",
     CERT("(= " SAME2 " (affine (matrix 2 1 0 0)))", "false", W("solution (matrix 2 1 (/ 1 3) 1)"))},
    {"a witness with a point that is not a solution, where none is needed", CK_REJECTED, "non è usato",
     CERT("(= " ONE " (affine (matrix 2 1 2 1)))", "false", W("solution (matrix 2 1 2 -1)"))},
    {"a solution outside that is not a solution", CK_REJECTED, "A z non è b",
     CERT("(= " PLANE " " PLANE_LINE ")", "false", W("solution (matrix 3 1 0 0 2)") W("combination (matrix 1 3 1 1 0)"))},
    {"a solution outside that is inside: the row does not separate it", CK_REJECTED, "y (z - p) è zero",
     CERT("(= " PLANE " " PLANE_LINE ")", "false", W("solution (matrix 3 1 2 -1 0)") W("combination (matrix 1 3 1 1 0)"))},
    {"a row that is not orthogonal to the written direction", CK_REJECTED, "y U non è zero",
     CERT("(= " PLANE " " PLANE_LINE ")", "false", W("solution (matrix 3 1 0 0 1)") W("combination (matrix 1 3 1 0 0)"))},
    {"another solution that is the written point", CK_REJECTED, "è il punto scritto", CERT("(= " SAME2 " (affine (matrix 2 1 0 0)))", "false", W("solution (matrix 2 1 0 0)"))},
    {"the right line declared wrong with a solution of the line", CK_REJECTED, "y (z - p) è zero",
     CERT("(= " LINE " " LINE_SET ")", "false", W("solution (matrix 3 1 (/ -1 5) 1 (/ 1 5))") W("combination (matrix 1 3 1 1 0)"))},
    {"a solution outside without the row that shows it", CK_UNSUPPORTED, "manca il testimone", CERT("(= " PLANE " " PLANE_LINE ")", "false", W("solution (matrix 3 1 0 0 1)"))},
    {"a wrong set declared right, with the witnesses of its falsity", CK_REJECTED, "dichiara vera",
     CERT("(= " PLANE " " PLANE_LINE ")", "true", W("solution (matrix 3 1 0 0 1)") W("combination (matrix 1 3 1 1 0)"))},

    /* Le dimensioni, e ciò che è scritto male. */
    {"a right-hand side with two columns", CK_REJECTED, "una colonna b", CERT("(= (solutions (matrix 2 2 1 1 1 -1) (matrix 2 2 1 3 1 3)) " ONE_SET ")", "true", ONE_SPAN)},
    {"a right-hand side with the wrong number of rows", CK_REJECTED, "una colonna b", CERT("(= (solutions (matrix 2 2 1 1 1 -1) (matrix 3 1 1 3 0)) " ONE_SET ")", "true", ONE_SPAN)},
    {"a written point with three components for two unknowns", CK_REJECTED, "tante componenti", CERT("(= " ONE " (affine (matrix 3 1 2 -1 0)))", "false", NONE)},
    {"a written point that is a row", CK_REJECTED, "tante componenti", CERT("(in (matrix 1 2 2 -1) " ONE ")", "true", NONE)},
    {"written directions with the wrong number of rows", CK_REJECTED, "tante componenti", CERT("(= " ONE " (affine (matrix 2 1 2 -1) (matrix 3 1 0 0 0)))", "false", NONE)},
    {"a number in place of the matrix of the system", CK_REJECTED, "una matrice A", CERT("(= (solutions 2 (matrix 1 1 4)) (nothing))", "false", NONE)},
    {"a written set in front of the system", CK_UNSUPPORTED, "scritto per primo", CERT("(= " ONE_SET " " ONE ")", "true", ONE_SPAN)},
    {"two systems compared", CK_UNSUPPORTED, "affine o nothing", CERT("(= " ONE " " ONE ")", "true", NONE)},
    {"a relation between sets that the checker does not know", CK_UNSUPPORTED, "relazione che il verificatore non conosce", CERT("(subset " ONE " " ONE_SET ")", "true", NONE)},
    {"an order between sets", CK_UNSUPPORTED, "relazione che il verificatore non conosce", CERT("(< " ONE " " ONE_SET ")", "true", NONE)},
    {"text after the statement", CK_REJECTED, "testo in più dopo l'enunciato", CERT("(= " ONE " " ONE_SET ") 1", "true", ONE_SPAN)},
    {"a missing parenthesis in the written set", CK_REJECTED, "scritto male", CERT("(= " ONE " (affine (matrix 2 1 2 -1))", "true", ONE_SPAN)},
    {"nothing with an operand", CK_REJECTED, "scritto male", CERT("(= " ONE " (nothing 2))", "false", W("solution (matrix 2 1 2 -1)"))},
    {"a rank inside the matrix of a system, without its witness", CK_UNSUPPORTED, "il rango e l'inversa", CERT("(in (matrix 1 1 1) (solutions (matrix 1 1 (rank (identity 2))) (matrix 1 1 2)))", "false", NONE)},
    {"a number beyond 128 bits", CK_UNSUPPORTED, "troppo grandi", CERT("(in (matrix 2 1 (^ 1000000007 20) 0) " ONE ")", "false", NONE)},

    /* Con una lettera e la voce where (17d): la soluzione scritta con il
     * parametro a denominatore, dove il determinante non si annulla. */
    {"the solution of a system with a letter, where the determinant is not zero", CK_ACCEPTED, "", WCERT("k", D, "(= " KK_SYSTEM " " KK_SET ")", "true", W("span " KK_INVERSE))},
    {"the same solution as a member", CK_ACCEPTED, "", WCERT("k", D, "(in (matrix 2 1 (/ 1 (+ k 1)) (/ 1 (+ k 1))) " KK_SYSTEM ")", "true", NONE)},
    {"the same solution written with the common denominator", CK_ACCEPTED, "",
     WCERT("k", D, "(= " KK_SYSTEM " (affine (* (/ 1 " D ") (matrix 2 1 (- k 1) (- k 1)))))", "true", W("span " KK_INVERSE))},
    {"a power of the declared polynomial as a denominator", CK_ACCEPTED, "",
     WCERT("k", "(- k 1)", "(in (matrix 2 1 (/ 1 (^ (- k 1) 2)) 0) (solutions (matrix 1 2 (^ (- k 1) 2) k) (matrix 1 1 1)))", "true", NONE)},
    {"a constant multiple of a divisor as a denominator", CK_ACCEPTED, "",
     WCERT("k", D, "(in (matrix 2 1 (/ 3 (* 3 (+ k 1))) (/ 3 (* 3 (+ k 1)))) " KK_SYSTEM ")", "true", NONE)},
    {"an impossible system where a polynomial is not zero", CK_ACCEPTED, "", WCERT("k", "(- k 1)", "(= " TWICE " (nothing))", "true", TWICE_ROW)},
    {"a written point that is a solution for no value where the polynomial is not zero", CK_ACCEPTED, "",
     WCERT("k", "k", "(= (solutions (matrix 1 2 1 1) (matrix 1 1 k)) (affine (matrix 2 1 0 0) (matrix 2 1 1 -1)))", "false", NONE)},
    {"where 1: every value of the letter", CK_ACCEPTED, "", WCERT("k", "1", "(in (matrix 2 1 k (- 1 k)) " SUM1 ")", "true", NONE)},
    {"an element of the witness changed, with a letter", CK_REJECTED, "G A + U T non è l'identità",
     WCERT("k", D, "(= " KK_SYSTEM " " KK_SET ")", "true", W("span (* (/ 1 " D ") (matrix 2 2 k -1 -1 1))"))},
    {"the witness without its denominator", CK_REJECTED, "G A + U T non è l'identità", WCERT("k", D, "(= " KK_SYSTEM " " KK_SET ")", "true", W("span (matrix 2 2 k -1 -1 k)"))},
    {"a wrong component of the solution: the second equation fails by 2 for every value", CK_ACCEPTED, "",
     WCERT("k", D, "(= " KK_SYSTEM " (affine (matrix 2 1 (/ 1 (- k 1)) (/ 1 (+ k 1)))))", "false", NONE)},
    {"the same wrong solution declared right", CK_REJECTED, "dichiara vera",
     WCERT("k", D, "(= " KK_SYSTEM " (affine (matrix 2 1 (/ 1 (- k 1)) (/ 1 (+ k 1)))))", "true", NONE)},
    {"a written set that is the solutions for one value of the letter only, declared right", CK_UNSUPPORTED, "solo per qualche valore",
     LCERT("k", "(= " SUM1 " (affine (matrix 2 1 k 0) (matrix 2 1 1 -1)))", "true", W("span (matrix 2 1 1 0) (matrix 1 2 0 -1)"))},
    {"and it cannot be declared wrong either", CK_UNSUPPORTED, "solo per qualche valore",
     LCERT("k", "(= " SUM1 " (affine (matrix 2 1 k 0) (matrix 2 1 1 -1)))", "false", NONE)},
    {"the solution without the voice where: it divides by a polynomial", CK_UNSUPPORTED, "divide solo per un numero", LCERT("k", "(= " KK_SYSTEM " " KK_SET ")", "true", W("span " KK_INVERSE))},
    {"a denominator that does not divide the declared polynomial", CK_UNSUPPORTED, "divide solo per un numero",
     WCERT("k", "(- k 1)", "(= " KK_SYSTEM " " KK_SET ")", "true", W("span " KK_INVERSE))},
    {"an impossible system without the voice where: 0 = 1 - k is not 0 = c for every k", CK_UNSUPPORTED, "solo per qualche valore", LCERT("k", "(= " TWICE " (nothing))", "true", TWICE_ROW)},
    {"an impossible system with the wrong polynomial declared", CK_UNSUPPORTED, "solo per qualche valore", WCERT("k", "(+ k 1)", "(= " TWICE " (nothing))", "true", TWICE_ROW)},
    {"the zero polynomial as the voice where: it would say nothing", CK_REJECTED, "polinomio nullo", WCERT("k", "(- k k)", "(= " TWICE " (nothing))", "true", TWICE_ROW)},
    {"the number zero as the voice where", CK_REJECTED, "polinomio nullo", WCERT("k", "0", "(in (matrix 2 1 5 5) " KK_SYSTEM ")", "true", NONE)},
    {"the voice where without letters", CK_REJECTED, "vuole prima la voce letters",
     "certificate matrices\nwhere 2\nstatement (in (matrix 2 1 2 -1) " ONE ")\nclaim true\nend\n"},
    {"the voice where after the statement", CK_REJECTED, "manca la voce claim",
     "certificate matrices\nletters k\nstatement (in (matrix 2 1 2 -1) " ONE ")\nwhere k\nclaim true\nend\n"},
    {"a matrix as the voice where", CK_REJECTED, "scrive un polinomio", WCERT("k", "(matrix 1 1 k)", "(in (matrix 2 1 2 -1) " ONE ")", "true", NONE)},
    {"two polynomials in the voice where", CK_REJECTED, "scrive un polinomio", WCERT("k", "k k", "(in (matrix 2 1 2 -1) " ONE ")", "true", NONE)},
    {"a division by the letter inside the voice where", CK_UNSUPPORTED, "divide solo per un numero", WCERT("k", "(/ 1 k)", "(in (matrix 2 1 2 -1) " ONE ")", "true", NONE)},
    {"a member that is a solution for one value of the letter only", CK_UNSUPPORTED, "solo per qualche valore", LCERT("k", "(in (matrix 2 1 k 0) " SUM1 ")", "true", NONE)},

    /* Il rango e l'inversa con una lettera, dove il determinante non si
     * annulla: gli stessi testimoni dei numeri. */
    {"the rank of a matrix with a letter, where its determinant is not zero", CK_ACCEPTED, "", WCERT("k", D, "(= (rank " KK ") 2)", "true", KK_RANK)},
    {"the rank of a matrix that has it for every value of the letter", CK_ACCEPTED, "",
     WCERT("k", "1", "(= (rank (matrix 2 2 1 k 2 (* 2 k))) 1)", "true", W("rank 1 (matrix 2 1 1 2) (matrix 1 2 1 k) (matrix 1 2 1 0) (matrix 2 1 1 0)"))},
    {"the rank of the zero matrix, with a letter", CK_ACCEPTED, "", WCERT("k", "1", "(= (rank (matrix 2 2 (- k k) 0 0 0)) 0)", "true", W("rank 0"))},
    {"the inverse of a matrix with a letter", CK_ACCEPTED, "", WCERT("k", D, "(= (inv " KK ") " KK_INVERSE ")", "true", W("inverse " KK_INVERSE))},
    {"the inverse written entry by entry", CK_ACCEPTED, "",
     WCERT("k", D, "(= (inv " KK ") (matrix 2 2 (/ k " D ") (/ -1 " D ") (/ -1 " D ") (/ k " D ")))", "true", W("inverse " KK_INVERSE))},
    {"a rank one higher than the true one, with a letter", CK_REJECTED, "dimensioni",
     WCERT("k", "1", "(= (rank (matrix 2 2 1 k 2 (* 2 k))) 2)", "true", W("rank 2 (matrix 2 1 1 2) (matrix 1 2 1 k) (matrix 1 2 1 0) (matrix 2 1 1 0)"))},
    {"a rank one lower than the true one, with a letter", CK_REJECTED, "C F non è la matrice",
     WCERT("k", D, "(= (rank " KK ") 1)", "true", W("rank 1 (matrix 2 1 k 1) (matrix 1 2 1 0) (matrix 1 2 1 0) (matrix 2 1 1 0)"))},
    {"the generic rank claimed without its denominator", CK_REJECTED, "G A H non è l'identità", WCERT("k", D, "(= (rank " KK ") 2)", "true", W("rank 2 " KK " (identity 2) (matrix 2 2 k -1 -1 k) (identity 2)"))},
    {"rank zero for a matrix that is zero for one value of the letter only", CK_REJECTED, "non è nulla", WCERT("k", "1", "(= (rank (matrix 1 1 k)) 0)", "true", W("rank 0"))},
    {"the right rank compared with the wrong number", CK_REJECTED, "dichiara vera", WCERT("k", D, "(= (rank " KK ") 1)", "true", KK_RANK)},
    {"a wrong inverse, with a letter", CK_REJECTED, "il prodotto non è l'identità", WCERT("k", D, "(= (inv " KK ") " KK_INVERSE ")", "true", W("inverse (* (/ 1 " D ") (matrix 2 2 k 1 1 k))"))},
    {"the right inverse compared with another matrix", CK_REJECTED, "dichiara vera", WCERT("k", D, "(= (inv " KK ") (* (/ 1 " D ") (matrix 2 2 k -1 -1 1)))", "true", W("inverse " KK_INVERSE))},
    {"a rank with a letter and no witness", CK_UNSUPPORTED, "manca il testimone", WCERT("k", D, "(= (rank " KK ") 2)", "true", NONE)},
    {"a rank with a letter and no voice where", CK_UNSUPPORTED, "il rango e l'inversa", LCERT("k", "(= (rank " KK ") 2)", "true", KK_RANK)},
    {"a claim false with letters outside a system is still not treated", CK_UNSUPPORTED, "dichiarata vera", WCERT("k", D, "(= (rank " KK ") 1)", "false", KK_RANK)},
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
    char message[640];
    const char *why;

    /* La tabella: risposta attesa, e il motivo per chi non è accettato. */
    for (size_t i = 0; i < sizeof cases / sizeof *cases; i++) {
        const char *text = cases[i].text;
        CkAnswer answer = ck_check(&text, &why, 0);
        snprintf(message, sizeof message, "\"%s\" should be %s but is %s (%s)", cases[i].name, answer_name(cases[i].expected), answer_name(answer), why);
        check(answer == cases[i].expected, message);
        snprintf(message, sizeof message, "\"%s\" should mention \"%s\" as its reason, not \"%s\"", cases[i].name, cases[i].why, why);
        check(answer != cases[i].expected || strstr(why, cases[i].why) != NULL, message);
    }

    /* La voce where di un certificato non resta al successivo: dopo uno che
     * divide per il polinomio dichiarato, lo stesso senza la voce non può
     * più farlo. */
    const char *two = WCERT("k", D, "(in (matrix 2 1 (/ 1 (+ k 1)) (/ 1 (+ k 1))) " KK_SYSTEM ")", "true", NONE)
                      LCERT("k", "(in (matrix 2 1 (/ 1 (+ k 1)) (/ 1 (+ k 1))) " KK_SYSTEM ")", "true", NONE);
    check(ck_check(&two, &why, 0) == CK_ACCEPTED, "a certificate with the voice where should be accepted");
    check(ck_check(&two, &why, 0) == CK_UNSUPPORTED && strstr(why, "divide solo per un numero") != NULL,
          "a certificate without the voice where, after one with it, should not divide by a polynomial");

    /* Dopo un certificato su un sistema, uno sulle matrici si legge come
     * prima; e i testimoni dell'uno non servono all'altro. */
    const char *mixed = CERT("(= " ONE " " ONE_SET ")", "true", ONE_SPAN) CERT("(= (rank (matrix 2 2 1 2 2 4)) 1)", "true", NONE);
    check(ck_check(&mixed, &why, 0) == CK_ACCEPTED, "a certificate on a system should be accepted");
    check(ck_check(&mixed, &why, 0) == CK_UNSUPPORTED && strstr(why, "manca il testimone") != NULL,
          "a rank after a certificate on a system should still want its own witness");

    /* Lo stesso certificato dà la stessa risposta se lo si controlla due
     * volte. */
    for (int round = 0; round < 2; round++) {
        const char *again = WCERT("k", D, "(= " KK_SYSTEM " " KK_SET ")", "true", W("span " KK_INVERSE));
        check(ck_check(&again, &why, 0) == CK_ACCEPTED, "a certificate checked twice should be accepted twice");
    }

    printf("%s: %d of %d system certificate checks passed\n", failures ? "FAIL" : "PASS", checks - failures, checks);
    return failures ? 1 : 0;
}
