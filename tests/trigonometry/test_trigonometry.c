/* Test di dominio: goniometria. Equazioni, disequazioni e sistemi in una
 * variabile reale con seno, coseno, tangente e cotangente.
 *
 * Sei controlli. I primi due sono quelli di tests/statement_checks.h:
 *
 *   1. ogni enunciato di book_trigonometry.tex viene dimostrato: dice che
 *      una condizione vale esattamente per i valori stampati come risultato
 *      su un libro di scuola (corretto nei cinque esercizi in cui il
 *      risultato stampato è sbagliato);
 *   2. nessun enunciato di false_trigonometry.tex viene dimostrato: sono gli
 *      stessi risultati con un dettaglio cambiato (un periodo, un angolo, un
 *      estremo compreso o escluso, una famiglia dimenticata), e i cinque
 *      risultati stampati sbagliati;
 *   3. nessun enunciato di book_trigonometry_beyond.tex viene dichiarato
 *      falso: sono veri, ma il motore non li sa ancora decidere;
 *   4. ogni richiesta di solution_sets_trigonometry.tex viene risolta, e le
 *      soluzioni sono quelle del libro: scritte allo stesso modo, oppure in
 *      un altro modo che il motore dimostra equivalente;
 *   5. gli enunciati di window_cases[], sulle condizioni ristrette a un
 *      intervallo e sulle famiglie x = a + kT, ricevono l'esito atteso;
 *   6. almeno 239 dei 283 enunciati falsi vengono dichiarati falsi, non
 *      solo "non dimostrati": se una modifica facesse scendere questo numero
 *      il motore saprebbe riconoscere meno errori di oggi.
 *
 * Il metodo sotto esame è la tabella dei segni nella lettera t = tan(u/2)
 * (regole 18p-11, 18p-12, 18p-13 e 18p-14 di main.c). I dati vengono dal
 * libro e da un oracolo indipendente (tools/fuzz/fuzz_trigonometry.py), non
 * dall'uscita del programma: il motore non ne conosce nessuno in anticipo.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_trigonometry \
 *         tests/trigonometry/test_trigonometry.c && /tmp/test_trigonometry
 */

#include "../statement_checks.h"

/* Il risultato atteso di una richiesta di solution_sets_trigonometry.tex.
 * Invariante: le voci sono nell'ordine delle richieste del file. */
typedef struct {
    int exercise;         /* il numero dell'esercizio sul libro, per i messaggi */
    const char *text;     /* le soluzioni come le stampa il libro */
    const char *formula;  /* le stesse come condizione su x, con il suo
                             quantificatore su k: serve a dimostrare che una
                             scrittura diversa dice la stessa cosa. NULL se
                             non c'è una condizione da confrontare ("nessuna
                             soluzione"), o se il confronto non è alla portata
                             del motore: allora conta solo il testo */
    int open;             /* 1 se il motore oggi non risolve la richiesta:
                             può dirlo, ma se la risolve vale la regola di
                             tutte le altre */
} ExpectedSolution;

/* I risultati vengono dal libro. In cinque esercizi (407, 408, 60, 62 e, tra
 * quelli con un parametro, 747) il risultato stampato è sbagliato, come ha
 * trovato l'oracolo indipendente: qui c'è quello corretto. Negli esercizi 413
 * e 428 il libro usa l'arcocoseno, che la grammatica non ha: il testo atteso
 * è lo stesso angolo scritto con la tangente dell'angolo metà, ricavato a
 * mano (se cos a = 1/4 allora tan(a/2) = sqrt(3/5) = sqrt(15)/5, e
 * 2a = 4 arctan(sqrt(15)/5); se cos a = -1/3 allora tan(a/2) = sqrt(2)). */
static const ExpectedSolution expected_solutions[] = {
    {298, "x = \\frac{2}{3}\\pi + k\\pi \\lor x = \\frac{5}{6}\\pi + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\frac{2}{3}\\pi + k\\pi \\lor x = \\frac{5}{6}\\pi + k\\pi \\right)", 0},
    {299, "x = \\frac{\\pi}{8} + k\\pi \\lor x = \\frac{3}{4}\\pi + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\frac{\\pi}{8} + k\\pi \\lor x = \\frac{3}{4}\\pi + k\\pi \\right)", 0},
    {300, "x = \\frac{\\pi}{4} + k\\pi \\lor x = \\frac{\\pi}{3} + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\frac{\\pi}{4} + k\\pi \\lor x = \\frac{\\pi}{3} + k\\pi \\right)", 0},
    {301, "x = k\\pi \\lor x = \\frac{3}{4}\\pi + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = k\\pi \\lor x = \\frac{3}{4}\\pi + k\\pi \\right)", 0},
    {302, "x = \\arctan\\left(2\\right) + k\\pi \\lor x = \\frac{\\pi}{2} + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\arctan\\left(2\\right) + k\\pi \\lor x = \\frac{\\pi}{2} + k\\pi \\right)", 0},
    {303, "x = \\arctan\\left(-\\frac{1}{2}\\right) + k\\pi \\lor x = \\frac{\\pi}{4} + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\arctan\\left(-\\frac{1}{2}\\right) + k\\pi \\lor x = \\frac{\\pi}{4} + k\\pi \\right)", 0},
    {304, "x = k\\pi \\lor x = \\frac{\\pi}{4} + \\frac{k\\pi}{2}",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = k\\pi \\lor x = \\frac{\\pi}{4} + \\frac{k\\pi}{2} \\right)", 0},
    {305, "x = \\frac{\\pi}{4} + \\frac{k\\pi}{2}",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\frac{\\pi}{4} + \\frac{k\\pi}{2} \\right)", 0},
    {306, "x = -\\arctan\\left(2\\right) + k\\pi \\lor x = \\frac{\\pi}{4} + \\frac{k\\pi}{2} \\lor x = \\arctan\\left(2\\right) + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = -\\arctan\\left(2\\right) + k\\pi \\lor x = \\frac{\\pi}{4} + \\frac{k\\pi}{2} \\lor x = \\arctan\\left(2\\right) + k\\pi \\right)", 0},
    {307, "x = -\\frac{\\pi}{3} + k\\pi \\lor x = \\frac{\\pi}{3} + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = -\\frac{\\pi}{3} + k\\pi \\lor x = \\frac{\\pi}{3} + k\\pi \\right)", 0},
    {308, "x = \\arctan\\left(\\frac{1}{2}\\right) + k\\pi \\lor x = \\frac{\\pi}{4} + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\arctan\\left(\\frac{1}{2}\\right) + k\\pi \\lor x = \\frac{\\pi}{4} + k\\pi \\right)", 0},
    {395, "x = k\\pi \\lor x = \\frac{\\pi}{6} + \\frac{k\\pi}{3}",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = k\\pi \\lor x = \\frac{\\pi}{6} + \\frac{k\\pi}{3} \\right)", 0},
    {396, "x = -\\frac{\\pi}{3} + 2k\\pi \\lor x = \\frac{\\pi}{3} + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = -\\frac{\\pi}{3} + 2k\\pi \\lor x = \\frac{\\pi}{3} + 2k\\pi \\right)", 0},
    {397, "x \\neq \\frac{\\pi}{2} + k\\pi",
     "\\forall k \\in \\mathbb{Z}\\ \\left( x \\neq \\frac{\\pi}{2} + k\\pi \\right)", 0},
    {398, "x = \\frac{\\pi}{6} + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\frac{\\pi}{6} + k\\pi \\right)", 0},
    {399, "x = \\frac{\\pi}{6} + 2k\\pi \\lor x = \\frac{5}{6}\\pi + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\frac{\\pi}{6} + 2k\\pi \\lor x = \\frac{5}{6}\\pi + 2k\\pi \\right)", 0},
    {400, "x = -\\frac{\\pi}{3} + k\\pi \\lor x = k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = -\\frac{\\pi}{3} + k\\pi \\lor x = k\\pi \\right)", 0},
    {401, "x = k\\pi \\lor x = \\frac{\\pi}{2} + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = k\\pi \\lor x = \\frac{\\pi}{2} + k\\pi \\right)", 0},
    {402, "x = \\frac{\\pi}{24} + \\frac{k\\pi}{2} \\lor x = \\frac{5}{24}\\pi + \\frac{k\\pi}{2} \\lor x = \\frac{\\pi}{4} + \\frac{k\\pi}{2}",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\frac{\\pi}{24} + \\frac{k\\pi}{2} \\lor x = \\frac{5}{24}\\pi + \\frac{k\\pi}{2} \\lor x = \\frac{\\pi}{4} + \\frac{k\\pi}{2} \\right)", 1},
    {403, "x = -\\frac{\\pi}{4} + k\\pi \\lor x = \\frac{\\pi}{2} + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = -\\frac{\\pi}{4} + k\\pi \\lor x = \\frac{\\pi}{2} + k\\pi \\right)", 0},
    {404, "x = \\frac{\\pi}{4} + k\\pi \\lor x = \\frac{7}{6}\\pi + 2k\\pi \\lor x = \\frac{11}{6}\\pi + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\frac{\\pi}{4} + k\\pi \\lor x = \\frac{7}{6}\\pi + 2k\\pi \\lor x = \\frac{11}{6}\\pi + 2k\\pi \\right)", 0},
    {405, "x = 2k\\pi \\lor x = \\frac{\\pi}{2} + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = 2k\\pi \\lor x = \\frac{\\pi}{2} + k\\pi \\right)", 0},
    {406, "x = \\frac{7}{18}\\pi + \\frac{2k\\pi}{3} \\lor x = \\frac{11}{18}\\pi + \\frac{2k\\pi}{3}",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\frac{7}{18}\\pi + \\frac{2k\\pi}{3} \\lor x = \\frac{11}{18}\\pi + \\frac{2k\\pi}{3} \\right)", 0},
    {407, "x = \\frac{7}{6}\\pi + 2k\\pi \\lor x = \\frac{11}{6}\\pi + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\frac{7}{6}\\pi + 2k\\pi \\lor x = \\frac{11}{6}\\pi + 2k\\pi \\right)", 1},
    {408, "x = \\frac{\\pi}{16} + \\frac{k\\pi}{4} \\lor x = \\frac{\\pi}{8} + \\frac{k\\pi}{2} \\lor x = \\frac{\\pi}{2} + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\frac{\\pi}{16} + \\frac{k\\pi}{4} \\lor x = \\frac{\\pi}{8} + \\frac{k\\pi}{2} \\lor x = \\frac{\\pi}{2} + k\\pi \\right)", 1},
    {409, "x = \\frac{3}{20}\\pi + \\frac{k\\pi}{5} \\lor x = \\frac{\\pi}{2} + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\frac{3}{20}\\pi + \\frac{k\\pi}{5} \\lor x = \\frac{\\pi}{2} + k\\pi \\right)", 0},
    {410, "x = -\\frac{\\pi}{4} + k\\pi \\lor x = k\\pi \\lor x = \\frac{\\pi}{4} + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = -\\frac{\\pi}{4} + k\\pi \\lor x = k\\pi \\lor x = \\frac{\\pi}{4} + k\\pi \\right)", 0},
    {411, "x = -\\frac{2}{3}\\pi + 2k\\pi \\lor x = \\frac{2}{3}\\pi + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = -\\frac{2}{3}\\pi + 2k\\pi \\lor x = \\frac{2}{3}\\pi + 2k\\pi \\right)", 0},
    {412, "x = -\\frac{5}{6}\\pi + 2k\\pi \\lor x = -\\frac{\\pi}{6} + 2k\\pi \\lor x = \\frac{\\pi}{2} + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = -\\frac{5}{6}\\pi + 2k\\pi \\lor x = -\\frac{\\pi}{6} + 2k\\pi \\lor x = \\frac{\\pi}{2} + 2k\\pi \\right)", 0},
    {413, "x = 4\\arctan\\left(-\\frac{\\sqrt{15}}{5}\\right) + 4k\\pi \\lor x = 4\\arctan\\left(\\frac{\\sqrt{15}}{5}\\right) + 4k\\pi \\lor x = \\pi + 2k\\pi",
     NULL, 0},
    {414, "x = \\frac{\\pi}{2} + k\\pi \\lor x = \\frac{3}{4}\\pi + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\frac{\\pi}{2} + k\\pi \\lor x = \\frac{3}{4}\\pi + k\\pi \\right)", 0},
    {415, "x = \\frac{2}{3}\\pi + 2k\\pi \\lor x = \\frac{3}{4}\\pi + k\\pi \\lor x = \\frac{5}{6}\\pi + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\frac{2}{3}\\pi + 2k\\pi \\lor x = \\frac{3}{4}\\pi + k\\pi \\lor x = \\frac{5}{6}\\pi + 2k\\pi \\right)", 0},
    {416, "x = \\frac{3}{4}\\pi + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\frac{3}{4}\\pi + k\\pi \\right)", 0},
    {417, "x = k\\pi \\lor x = \\frac{\\pi}{10} + \\frac{k\\pi}{5} \\lor x = \\frac{\\pi}{6} + \\frac{k\\pi}{3}",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = k\\pi \\lor x = \\frac{\\pi}{10} + \\frac{k\\pi}{5} \\lor x = \\frac{\\pi}{6} + \\frac{k\\pi}{3} \\right)", 0},
    {418, "x = \\frac{\\pi}{32} + \\frac{k\\pi}{4} \\lor x = \\frac{\\pi}{16} + \\frac{k\\pi}{8} \\lor x = \\frac{3}{32}\\pi + \\frac{k\\pi}{4}",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\frac{\\pi}{32} + \\frac{k\\pi}{4} \\lor x = \\frac{\\pi}{16} + \\frac{k\\pi}{8} \\lor x = \\frac{3}{32}\\pi + \\frac{k\\pi}{4} \\right)", 0},
    {419, "x = \\frac{\\pi}{3} + 2k\\pi \\lor x = \\frac{5}{3}\\pi + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\frac{\\pi}{3} + 2k\\pi \\lor x = \\frac{5}{3}\\pi + 2k\\pi \\right)", 0},
    {420, "x = -\\frac{4}{3}\\pi + 4k\\pi \\lor x = \\frac{\\pi}{2} + k\\pi \\lor x = \\frac{4}{3}\\pi + 4k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = -\\frac{4}{3}\\pi + 4k\\pi \\lor x = \\frac{\\pi}{2} + k\\pi \\lor x = \\frac{4}{3}\\pi + 4k\\pi \\right)", 0},
    {421, "x = \\frac{\\pi}{3} + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\frac{\\pi}{3} + k\\pi \\right)", 0},
    {422, "\\nexists x \\in \\mathbb{R}",
     NULL, 0},
    {425, "x = 2k\\pi \\lor x = \\frac{\\pi}{2} + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = 2k\\pi \\lor x = \\frac{\\pi}{2} + 2k\\pi \\right)", 0},
    {426, "x = \\frac{\\pi}{2} + k\\pi \\lor x = \\pi + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\frac{\\pi}{2} + k\\pi \\lor x = \\pi + 2k\\pi \\right)", 0},
    {427, "x = \\arctan\\left(3\\right) + k\\pi \\lor x = \\frac{3}{4}\\pi + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\arctan\\left(3\\right) + k\\pi \\lor x = \\frac{3}{4}\\pi + k\\pi \\right)", 0},
    {428, "x = 2\\arctan\\left(-\\sqrt{2}\\right) + 2k\\pi \\lor x = 2k\\pi \\lor x = 2\\arctan\\left(\\sqrt{2}\\right) + 2k\\pi",
     NULL, 0},
    {429, "x = \\frac{\\pi}{4} + \\frac{k\\pi}{2}",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\frac{\\pi}{4} + \\frac{k\\pi}{2} \\right)", 0},
    {430, "x = -\\frac{2}{3}\\pi + 2k\\pi \\lor x = -\\frac{\\pi}{3} + 2k\\pi \\lor x = \\frac{\\pi}{3} + 2k\\pi \\lor x = \\frac{\\pi}{2} + k\\pi \\lor x = \\frac{2}{3}\\pi + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = -\\frac{2}{3}\\pi + 2k\\pi \\lor x = -\\frac{\\pi}{3} + 2k\\pi \\lor x = \\frac{\\pi}{3} + 2k\\pi \\lor x = \\frac{\\pi}{2} + k\\pi \\lor x = \\frac{2}{3}\\pi + 2k\\pi \\right)", 0},
    {694, "\\frac{5}{6}\\pi + 2k\\pi \\le x < \\frac{11}{6}\\pi + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( \\frac{5}{6}\\pi + 2k\\pi \\le x < \\frac{11}{6}\\pi + 2k\\pi \\right)", 0},
    {695, "-\\frac{\\pi}{4} + 2k\\pi \\le x \\le \\frac{\\pi}{4} + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( -\\frac{\\pi}{4} + 2k\\pi \\le x \\le \\frac{\\pi}{4} + 2k\\pi \\right)", 0},
    {696, "\\frac{5}{6}\\pi + 2k\\pi \\le x \\le \\frac{7}{4}\\pi + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( \\frac{5}{6}\\pi + 2k\\pi \\le x \\le \\frac{7}{4}\\pi + 2k\\pi \\right)", 0},
    {697, "\\nexists x \\in \\mathbb{R}",
     NULL, 0},
    {698, "-\\frac{\\pi}{6} + k\\pi \\le x \\le \\frac{\\pi}{6} + k\\pi \\land x \\neq k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( -\\frac{\\pi}{6} + k\\pi \\le x \\le \\frac{\\pi}{6} + k\\pi \\land x \\neq k\\pi \\right)", 0},
    {699, "x = 0 \\lor \\frac{\\pi}{6} \\le x \\le \\frac{\\pi}{3} \\lor \\frac{5}{3}\\pi \\le x \\le 2\\pi",
     "\\left( x = 0 \\lor \\frac{\\pi}{6} \\le x \\le \\frac{\\pi}{3} \\lor \\frac{5}{3}\\pi \\le x \\le 2\\pi \\right)", 0},
    {700, "-\\frac{\\pi}{6} + 2k\\pi \\le x \\le \\frac{\\pi}{3} + 2k\\pi \\lor \\frac{4}{3}\\pi + 2k\\pi \\le x \\le \\frac{3}{2}\\pi + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( -\\frac{\\pi}{6} + 2k\\pi \\le x \\le \\frac{\\pi}{3} + 2k\\pi \\lor \\frac{4}{3}\\pi + 2k\\pi \\le x \\le \\frac{3}{2}\\pi + 2k\\pi \\right)", 0},
    {701, "x = 0 \\lor \\frac{2}{3}\\pi \\le x \\le \\frac{3}{4}\\pi \\lor \\frac{7}{4}\\pi \\le x \\le 2\\pi",
     "\\left( x = 0 \\lor \\frac{2}{3}\\pi \\le x \\le \\frac{3}{4}\\pi \\lor \\frac{7}{4}\\pi \\le x \\le 2\\pi \\right)", 0},
    {702, "-\\frac{\\pi}{6} + 2k\\pi \\le x \\le \\frac{\\pi}{3} + 2k\\pi \\lor x = \\frac{2}{3}\\pi + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( -\\frac{\\pi}{6} + 2k\\pi \\le x \\le \\frac{\\pi}{3} + 2k\\pi \\lor x = \\frac{2}{3}\\pi + 2k\\pi \\right)", 0},
    {703, "\\frac{\\pi}{6} + 2k\\pi \\le x < \\frac{\\pi}{2} + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( \\frac{\\pi}{6} + 2k\\pi \\le x < \\frac{\\pi}{2} + 2k\\pi \\right)", 0},
    {704, "k\\pi \\le x < \\frac{\\pi}{4} + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( k\\pi \\le x < \\frac{\\pi}{4} + k\\pi \\right)", 0},
    {705, "\\frac{\\pi}{3} + 2k\\pi \\le x < \\frac{\\pi}{2} + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( \\frac{\\pi}{3} + 2k\\pi \\le x < \\frac{\\pi}{2} + 2k\\pi \\right)", 0},
    {706, "x = \\frac{3}{2}\\pi + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\frac{3}{2}\\pi + 2k\\pi \\right)", 0},
    {707, "x = \\frac{3}{4}\\pi + 2k\\pi \\lor \\frac{7}{4}\\pi + 2k\\pi \\le x < 2\\pi + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\frac{3}{4}\\pi + 2k\\pi \\lor \\frac{7}{4}\\pi + 2k\\pi \\le x < 2\\pi + 2k\\pi \\right)", 0},
    {708, "\\frac{\\pi}{4} + k\\pi \\le x \\le \\frac{3}{4}\\pi + k\\pi \\land x \\neq \\frac{\\pi}{2} + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( \\frac{\\pi}{4} + k\\pi \\le x \\le \\frac{3}{4}\\pi + k\\pi \\land x \\neq \\frac{\\pi}{2} + k\\pi \\right)", 0},
    {709, "-\\frac{\\pi}{4} + k\\pi < x < \\frac{\\pi}{4} + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( -\\frac{\\pi}{4} + k\\pi < x < \\frac{\\pi}{4} + k\\pi \\right)", 0},
    {710, "\\nexists x \\in \\mathbb{R}",
     NULL, 0},
    {711, "\\frac{\\pi}{6} + 2k\\pi < x < \\frac{\\pi}{2} + 2k\\pi \\lor \\frac{11}{6}\\pi + 2k\\pi < x < 2\\pi + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( \\frac{\\pi}{6} + 2k\\pi < x < \\frac{\\pi}{2} + 2k\\pi \\lor \\frac{11}{6}\\pi + 2k\\pi < x < 2\\pi + 2k\\pi \\right)", 0},
    {712, "k\\pi < x < \\frac{\\pi}{3} + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( k\\pi < x < \\frac{\\pi}{3} + k\\pi \\right)", 0},
    {713, "\\frac{\\pi}{2} + k\\pi < x < \\pi + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( \\frac{\\pi}{2} + k\\pi < x < \\pi + k\\pi \\right)", 0},
    {714, "\\frac{5}{6}\\pi < x < \\frac{5}{3}\\pi",
     "\\left( \\frac{5}{6}\\pi < x < \\frac{5}{3}\\pi \\right)", 0},
    {715, "x \\neq \\frac{3}{4}\\pi + k\\pi",
     "\\forall k \\in \\mathbb{Z}\\ \\left( x \\neq \\frac{3}{4}\\pi + k\\pi \\right)", 0},
    {716, "2k\\pi < x \\le \\frac{\\pi}{3} + 2k\\pi \\lor \\pi + 2k\\pi < x < \\frac{3}{2}\\pi + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( 2k\\pi < x \\le \\frac{\\pi}{3} + 2k\\pi \\lor \\pi + 2k\\pi < x < \\frac{3}{2}\\pi + 2k\\pi \\right)", 0},
    {50, "\\frac{7}{6}\\pi + 2k\\pi < x < \\frac{11}{6}\\pi + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( \\frac{7}{6}\\pi + 2k\\pi < x < \\frac{11}{6}\\pi + 2k\\pi \\right)", 0},
    {51, "0 \\le x < \\frac{\\pi}{4} \\lor \\frac{3}{4}\\pi < x \\le 2\\pi",
     "\\left( 0 \\le x < \\frac{\\pi}{4} \\lor \\frac{3}{4}\\pi < x \\le 2\\pi \\right)", 0},
    {52, "\\left(0 \\le x < \\frac{\\pi}{2} \\lor \\frac{4}{3}\\pi < x < \\frac{3}{2}\\pi \\lor \\frac{5}{3}\\pi \\le x \\le 2\\pi\\right) \\land x \\neq \\frac{\\pi}{3}",
     "\\left( \\left(0 \\le x < \\frac{\\pi}{2} \\lor \\frac{4}{3}\\pi < x < \\frac{3}{2}\\pi \\lor \\frac{5}{3}\\pi \\le x \\le 2\\pi\\right) \\land x \\neq \\frac{\\pi}{3} \\right)", 0},
    {53, "k\\pi \\le x < \\frac{\\pi}{2} + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( k\\pi \\le x < \\frac{\\pi}{2} + k\\pi \\right)", 0},
    {54, "\\frac{\\pi}{3} + k\\pi < x < \\frac{2}{3}\\pi + k\\pi \\land x \\neq \\frac{\\pi}{2} + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( \\frac{\\pi}{3} + k\\pi < x < \\frac{2}{3}\\pi + k\\pi \\land x \\neq \\frac{\\pi}{2} + k\\pi \\right)", 0},
    {55, "k\\pi < x \\le \\frac{\\pi}{6} + k\\pi \\lor \\frac{\\pi}{3} + k\\pi \\le x \\le \\frac{\\pi}{2} + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( k\\pi < x \\le \\frac{\\pi}{6} + k\\pi \\lor \\frac{\\pi}{3} + k\\pi \\le x \\le \\frac{\\pi}{2} + k\\pi \\right)", 0},
    {56, "0 \\le x \\le \\frac{\\pi}{4} \\lor \\frac{\\pi}{3} \\le x \\le \\frac{3}{4}\\pi \\lor \\frac{5}{4}\\pi \\le x \\le \\frac{5}{3}\\pi \\lor \\frac{7}{4}\\pi \\le x \\le 2\\pi",
     "\\left( 0 \\le x \\le \\frac{\\pi}{4} \\lor \\frac{\\pi}{3} \\le x \\le \\frac{3}{4}\\pi \\lor \\frac{5}{4}\\pi \\le x \\le \\frac{5}{3}\\pi \\lor \\frac{7}{4}\\pi \\le x \\le 2\\pi \\right)", 0},
    {57, "k\\pi \\le x < \\frac{\\pi}{4} + k\\pi \\lor \\frac{\\pi}{2} + k\\pi < x < \\frac{3}{4}\\pi + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( k\\pi \\le x < \\frac{\\pi}{4} + k\\pi \\lor \\frac{\\pi}{2} + k\\pi < x < \\frac{3}{4}\\pi + k\\pi \\right)", 0},
    {58, "k\\pi < x \\le \\frac{\\pi}{3} + k\\pi \\lor \\frac{\\pi}{2} + k\\pi < x \\le \\frac{3}{4}\\pi + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( k\\pi < x \\le \\frac{\\pi}{3} + k\\pi \\lor \\frac{\\pi}{2} + k\\pi < x \\le \\frac{3}{4}\\pi + k\\pi \\right)", 0},
    {59, "0 \\le x < \\frac{\\pi}{3} \\lor \\frac{\\pi}{2} \\le x \\le \\pi \\lor \\frac{4}{3}\\pi < x \\le \\frac{3}{2}\\pi \\lor x = 2\\pi",
     "\\left( 0 \\le x < \\frac{\\pi}{3} \\lor \\frac{\\pi}{2} \\le x \\le \\pi \\lor \\frac{4}{3}\\pi < x \\le \\frac{3}{2}\\pi \\lor x = 2\\pi \\right)", 0},
    {60, "\\left(x = 0 \\lor \\frac{\\pi}{6} < x \\le \\frac{\\pi}{4} \\lor \\frac{5}{6}\\pi < x \\le \\pi \\lor \\frac{5}{4}\\pi \\le x \\le 2\\pi\\right) \\land x \\neq \\frac{3}{2}\\pi",
     "\\left( \\left(x = 0 \\lor \\frac{\\pi}{6} < x \\le \\frac{\\pi}{4} \\lor \\frac{5}{6}\\pi < x \\le \\pi \\lor \\frac{5}{4}\\pi \\le x \\le 2\\pi\\right) \\land x \\neq \\frac{3}{2}\\pi \\right)", 0},
    {61, "k\\pi < x < \\frac{\\pi}{3} + k\\pi \\lor \\frac{\\pi}{2} + k\\pi < x < \\frac{2}{3}\\pi + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( k\\pi < x < \\frac{\\pi}{3} + k\\pi \\lor \\frac{\\pi}{2} + k\\pi < x < \\frac{2}{3}\\pi + k\\pi \\right)", 0},
    {62, "\\frac{k\\pi}{2} < x < \\frac{\\pi}{4} + \\frac{k\\pi}{2}",
     "\\exists k \\in \\mathbb{Z}\\ \\left( \\frac{k\\pi}{2} < x < \\frac{\\pi}{4} + \\frac{k\\pi}{2} \\right)", 0},
    {63, "2k\\pi < x \\le \\frac{\\pi}{4} + 2k\\pi \\lor \\frac{\\pi}{2} + 2k\\pi < x < \\pi + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( 2k\\pi < x \\le \\frac{\\pi}{4} + 2k\\pi \\lor \\frac{\\pi}{2} + 2k\\pi < x < \\pi + 2k\\pi \\right)", 0},
    {64, "\\frac{2}{3}\\pi + 2k\\pi < x < \\frac{4}{3}\\pi + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( \\frac{2}{3}\\pi + 2k\\pi < x < \\frac{4}{3}\\pi + 2k\\pi \\right)", 0},
    {65, "x = \\frac{\\pi}{2} + 2k\\pi \\lor \\pi + 2k\\pi \\le x \\le 2\\pi + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( x = \\frac{\\pi}{2} + 2k\\pi \\lor \\pi + 2k\\pi \\le x \\le 2\\pi + 2k\\pi \\right)", 0},
    {66, "\\frac{\\pi}{4} + k\\pi < x < \\frac{3}{4}\\pi + k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( \\frac{\\pi}{4} + k\\pi < x < \\frac{3}{4}\\pi + k\\pi \\right)", 0},
    {67, "-\\frac{\\pi}{6} + 2k\\pi \\le x \\le \\frac{\\pi}{3} + 2k\\pi \\lor x = \\pi + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( -\\frac{\\pi}{6} + 2k\\pi \\le x \\le \\frac{\\pi}{3} + 2k\\pi \\lor x = \\pi + 2k\\pi \\right)", 0},
    {68, "\\frac{\\pi}{6} + 2k\\pi \\le x \\le \\frac{\\pi}{2} + 2k\\pi \\lor x = \\frac{7}{6}\\pi + 2k\\pi",
     "\\exists k \\in \\mathbb{Z}\\ \\left( \\frac{\\pi}{6} + 2k\\pi \\le x \\le \\frac{\\pi}{2} + 2k\\pi \\lor x = \\frac{7}{6}\\pi + 2k\\pi \\right)", 0},
};

/* Un enunciato con l'esito atteso: una condizione ristretta a un intervallo
 * (18p-14) o una famiglia di soluzioni (18p-13). */
typedef struct {
    const char *name;       /* che cosa controlla, per i messaggi */
    const char *statement;  /* l'enunciato, in LaTeX */
    Outcome expected;       /* l'esito che il motore deve dare */
    const char *detail;     /* per SOLVED le soluzioni attese; altrimenti NULL */
} WindowCase;

/* I risultati attesi sono ricavati a mano: sulla circonferenza goniometrica
 * si leggono gli archi in cui la condizione vale, e si tagliano con
 * l'intervallo. */
static const WindowCase window_cases[] = {
    /* Una condizione periodica ristretta a un intervallo ha un numero
     * finito di pezzi, senza k. */
    {"an inequality on one turn", "\\{x \\in \\mathbb{R} \\mid 0 \\le x \\le 2\\pi \\land 2\\sin x > 1\\}", SOLVED, "\\frac{\\pi}{6} < x < \\frac{5}{6}\\pi"},
    {"two pieces at the ends of the interval", "\\{x \\in \\mathbb{R} \\mid 0 \\le x \\le 2\\pi \\land \\cos x \\ge 0\\}", SOLVED,
     "0 \\le x \\le \\frac{\\pi}{2} \\lor \\frac{3}{2}\\pi \\le x \\le 2\\pi"},
    {"an interval across the point where the tangent of the half angle has no value", "\\{x \\in \\mathbb{R} \\mid 0 \\le x \\le 2\\pi \\land \\cos x \\le 0\\}", SOLVED,
     "\\frac{\\pi}{2} \\le x \\le \\frac{3}{2}\\pi"},
    {"an isolated solution at that point", "\\{x \\in \\mathbb{R} \\mid 0 < x < 2\\pi \\land \\sin x = 0\\}", SOLVED, "x = \\pi"},
    {"an interval longer than one period", "\\{x \\in \\mathbb{R} \\mid -\\pi \\le x \\le 3\\pi \\land \\tan x \\ge 1\\}", SOLVED,
     "-\\frac{3}{4}\\pi \\le x < -\\frac{\\pi}{2} \\lor \\frac{\\pi}{4} \\le x < \\frac{\\pi}{2} \\lor \\frac{5}{4}\\pi \\le x < \\frac{3}{2}\\pi \\lor \\frac{9}{4}\\pi \\le x < \\frac{5}{2}\\pi"},
    {"a double angle on half a turn", "\\{x \\in \\mathbb{R} \\mid 0 \\le x \\le \\pi \\land \\sin\\left(2x\\right) \\ge 0\\}", SOLVED, "0 \\le x \\le \\frac{\\pi}{2} \\lor x = \\pi"},
    {"a half angle on two turns", "\\{x \\in \\mathbb{R} \\mid 0 \\le x \\le 4\\pi \\land 2\\cos\\left(\\frac{x}{2}\\right) < 1\\}", SOLVED, "\\frac{2}{3}\\pi < x < \\frac{10}{3}\\pi"},

    /* Con un estremo solo le soluzioni non sono né limitate né periodiche:
     * il motore deve dirlo, non scriverne un pezzo. */
    {"solutions on a half line are not written", "\\{x \\in \\mathbb{R} \\mid x \\ge 0 \\land \\sin x > 0\\}", UNSUPPORTED, NULL},

    /* Gli stessi risultati come enunciati, giusti e sbagliati di poco. */
    {"the result of an inequality on one turn", "\\forall x \\in \\mathbb{R}\\ \\left( 0 \\le x \\le 2\\pi \\land 2\\sin x > 1 \\iff \\frac{\\pi}{6} < x < \\frac{5}{6}\\pi \\right)", PROVED, NULL},
    {"an endpoint included by mistake", "\\forall x \\in \\mathbb{R}\\ \\left( 0 \\le x \\le 2\\pi \\land 2\\sin x > 1 \\iff \\frac{\\pi}{6} \\le x < \\frac{5}{6}\\pi \\right)", REFUTED, NULL},
    {"the end of the interval excluded by mistake",
     "\\forall x \\in \\mathbb{R}\\ \\left( 0 \\le x \\le 2\\pi \\land \\cos x \\ge 0 \\iff 0 \\le x \\le \\frac{\\pi}{2} \\lor \\frac{3}{2}\\pi \\le x < 2\\pi \\right)", REFUTED, NULL},

    /* Le famiglie: un intervallo più corto di mezzo periodo, uno più lungo,
     * uno lungo mezzo periodo e uno lungo un periodo intero. */
    {"an interval shorter than half a period", "\\forall x \\in \\mathbb{R}\\ \\left( 2\\sin x > 1 \\iff \\exists k \\in \\mathbb{Z}\\ \\left( \\frac{\\pi}{6} + 2k\\pi < x < \\frac{5}{6}\\pi + 2k\\pi \\right) \\right)", PROVED, NULL},
    {"an interval longer than half a period", "\\forall x \\in \\mathbb{R}\\ \\left( 2\\cos x < 1 \\iff \\exists k \\in \\mathbb{Z}\\ \\left( \\frac{\\pi}{3} + 2k\\pi < x < \\frac{5}{3}\\pi + 2k\\pi \\right) \\right)", PROVED, NULL},
    {"the same with an endpoint included", "\\forall x \\in \\mathbb{R}\\ \\left( 2\\cos x < 1 \\iff \\exists k \\in \\mathbb{Z}\\ \\left( \\frac{\\pi}{3} + 2k\\pi < x \\le \\frac{5}{3}\\pi + 2k\\pi \\right) \\right)", REFUTED, NULL},
    {"an interval of half a period", "\\forall x \\in \\mathbb{R}\\ \\left( \\sin x > 0 \\iff \\exists k \\in \\mathbb{Z}\\ \\left( 2k\\pi < x < \\pi + 2k\\pi \\right) \\right)", PROVED, NULL},
    {"the same for a weak inequality", "\\forall x \\in \\mathbb{R}\\ \\left( \\sin x \\ge 0 \\iff \\exists k \\in \\mathbb{Z}\\ \\left( 2k\\pi < x < \\pi + 2k\\pi \\right) \\right)", REFUTED, NULL},
    {"an open interval of a whole period leaves out one point",
     "\\forall x \\in \\mathbb{R}\\ \\left( \\cos\\left(\\frac{x}{2} - \\frac{\\pi}{6}\\right) < 1 \\iff \\exists k \\in \\mathbb{Z}\\ \\left( \\frac{\\pi}{3} + 4k\\pi < x < \\frac{13}{3}\\pi + 4k\\pi \\right) \\right)", PROVED, NULL},
    /* Antiregressione: con un estremo compreso l'intervallo lungo un periodo
     * copre tutto, e il controllo con i numeri una volta dava il punto per
     * escluso, fermandosi all'intervallo precedente. */
    {"a half-open interval of a whole period leaves out nothing",
     "\\forall x \\in \\mathbb{R}\\ \\left( \\cos\\left(\\frac{x}{2} - \\frac{\\pi}{6}\\right) < 1 \\iff \\exists k \\in \\mathbb{Z}\\ \\left( \\frac{\\pi}{3} + 4k\\pi \\le x < \\frac{13}{3}\\pi + 4k\\pi \\right) \\right)", REFUTED, NULL},
    {"an integer k is read only as the index of a family", "\\forall x \\in \\mathbb{R}\\ \\left( x > 0 \\iff \\exists k \\in \\mathbb{Z}\\ \\left( x = k \\right) \\right)", UNSUPPORTED, NULL},
};

/* Nome di un esito, per i messaggi. */
static const char *outcome_name(Outcome outcome) {
    switch (outcome) {
    case PROVED:      return "PROVED";
    case REFUTED:     return "REFUTED";
    case NOT_PROVED:  return "NOT_PROVED";
    case UNSUPPORTED: return "UNSUPPORTED";
    case SOLVED:      return "SOLVED";
    }
    return "?";
}

/* Controlli 3 e 6: conta gli enunciati di path per esito. Con forbidden si
 * dice l'esito che nessuno deve ricevere, con wanted e at_least quanti
 * almeno devono ricevere wanted (at_least = 0: nessun minimo). Ritorna 1 se
 * il controllo è riuscito. */
static int check_outcomes(const char *path, Outcome forbidden, Outcome wanted, int at_least) {
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }
    const char *position = text, *item, *end;
    int total = 0, bad = 0, good = 0;
    while (next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        total++;
        if (verdict.outcome == wanted) good++;
        if (verdict.outcome != forbidden) continue;
        bad++;
        printf("  statement %d of %s is %s\n", total, path, outcome_name(forbidden));
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d statements of %s are %s (at least %d wanted), %d are %s (none allowed)",
             good, total, path, outcome_name(wanted), at_least, bad, outcome_name(forbidden));
    return check_result(bad == 0 && good >= at_least, summary);
}

/* Ritorna 1 se le soluzioni written, scritte dal motore, dicono ciò che dice
 * expected: lo stesso testo, oppure un testo che il motore dimostra
 * equivalente alla condizione expected->formula. Una k nelle soluzioni
 * scritte è "per qualche intero k". */
static int same_solutions(const char *written, const ExpectedSolution *expected) {
    static char statement[8192];
    if (strcmp(written, expected->text) == 0) return 1;
    if (!expected->formula) return 0;
    int family = strstr(written, "k\\pi") != NULL;
    snprintf(statement, sizeof statement, "\\forall x \\in \\mathbb{R}\\ \\left( \\left( %s%s%s \\right) \\iff \\left( %s \\right) \\right)",
             family ? "\\exists k \\in \\mathbb{Z}\\ \\left( " : "", written, family ? " \\right)" : "", expected->formula);
    return prove(statement, 'n', 0, 0).outcome == PROVED;
}

/* Controllo 4: ogni richiesta di path è risolta, con le soluzioni di
 * expected_solutions[] (same_solutions()); una richiesta segnata open può
 * anche restare senza risposta. Ritorna 1 se il controllo è riuscito. */
static int check_solutions(const char *path) {
    static char written[4096];
    char summary[512];
    char *text = read_file(path);
    if (!text) {
        snprintf(summary, sizeof summary, "cannot read %s", path);
        return check_result(0, summary);
    }
    const char *position = text, *item, *end;
    int total = 0, right = 0, unanswered = 0, expected = (int)(sizeof expected_solutions / sizeof *expected_solutions);
    while (next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        const ExpectedSolution *wanted = total < expected ? &expected_solutions[total] : NULL;
        total++;
        if (wanted && wanted->open && verdict.outcome == UNSUPPORTED) { unanswered++; continue; }

        /* Il testo va copiato: dimostrare l'equivalenza riusa la memoria in
         * cui il motore lo ha scritto. */
        if (wanted && verdict.outcome == SOLVED && verdict.proof) {
            snprintf(written, sizeof written, "%s", verdict.detail);
            if (same_solutions(written, wanted)) { right++; continue; }
        }
        printf("  request %d (exercise %d): expected %s, got %s %s\n", total, wanted ? wanted->exercise : 0, wanted ? wanted->text : "nothing",
               outcome_name(verdict.outcome), verdict.outcome == SOLVED ? written : "");
    }
    free(text);
    snprintf(summary, sizeof summary, "%d of %d solution sets found and equal to the expected ones (%d not answered, as allowed)", right, total, unanswered);
    return check_result(total == expected && right + unanswered == total, summary);
}

/* Controllo 5: ogni enunciato di window_cases[] riceve l'esito atteso e, se
 * è una richiesta, le soluzioni attese. Ritorna 1 se il controllo è
 * riuscito. */
static int check_window_cases(void) {
    char summary[128];
    int total = (int)(sizeof window_cases / sizeof *window_cases), right = 0;
    for (int i = 0; i < total; i++) {
        Verdict verdict = prove(window_cases[i].statement, 'n', 0, 0);
        int same = verdict.outcome == window_cases[i].expected && (!window_cases[i].detail || strcmp(verdict.detail, window_cases[i].detail) == 0);
        if (same) { right++; continue; }
        printf("  %s: expected %s %s, got %s %s\n", window_cases[i].name, outcome_name(window_cases[i].expected), window_cases[i].detail ? window_cases[i].detail : "",
               outcome_name(verdict.outcome), verdict.outcome == SOLVED ? verdict.detail : "");
    }
    snprintf(summary, sizeof summary, "%d of %d statements on restricted conditions and families answered as expected", right, total);
    return check_result(right == total, summary);
}

int main(void) {
    engine_init();

    /* Tutti i controlli vengono eseguiti anche se uno fallisce, così il
     * resoconto è completo. */
    int passed = check_all_proved("tests/trigonometry/book_trigonometry.tex");
    passed = check_none_proved("tests/trigonometry/false_trigonometry.tex") && passed;
    passed = check_outcomes("tests/trigonometry/book_trigonometry_beyond.tex", REFUTED, PROVED, 0) && passed;
    passed = check_solutions("tests/trigonometry/solution_sets_trigonometry.tex") && passed;
    passed = check_window_cases() && passed;
    passed = check_outcomes("tests/trigonometry/false_trigonometry.tex", PROVED, REFUTED, 239) && passed;

    /* Ogni riga di una tabella dei segni e ogni soluzione scritta vengono
     * ricontrollate dal programma con un metodo indipendente: nessuna deve
     * essere stata scartata. */
    passed = check_result(unjustified_steps == 0, "every row of the sign tables and every written solution passed its independent check") && passed;
    return passed ? 0 : 1;
}
