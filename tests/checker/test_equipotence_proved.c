/* Test della regola 11m-f del verificatore: un'equipotenza dimostrata, in
 * un certificato di saturazione, esibendo la biiezione con una riga biject.
 * Compilato senza il motore.
 *
 * Tre cose. I certificati giusti (due refutazioni vere, trovate da un
 * dimostratore del primo ordine sulle clausole del verificatore) devono
 * essere accettati: "f e g si disfano a vicenda, quindi S e T sono
 * equipotenti", e "f è iniettiva e suriettiva, quindi S e T sono
 * equipotenti", dove l'inversa è la funzione di Skolem dell'ipotesi di
 * suriettività. I tentativi di far dimostrare alla regola più di ciò che
 * dà devono essere respinti: gli stessi passi senza una delle ipotesi, la
 * biiezione scritta al contrario, un termine con due variabili, la sola
 * riga biject senza i passi che mostrano che è una biiezione. E un oracolo
 * finito ricontrolla la regola: su un universo di tre elementi prova tutte
 * le coppie di sottoinsiemi A e B e tutte le coppie di funzioni t e u, e
 * verifica che le quattro clausole della riga impongano la proposizione
 * esattamente quando t e u si disfano a vicenda tra A e B, e che allora A
 * e B abbiano lo stesso numero di elementi. L'oracolo applica le funzioni
 * e conta gli elementi: non usa la traduzione in clausole.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_equipotence_proved \
 *         tests/checker/test_equipotence_proved.c && /tmp/test_equipotence_proved
 */

#include <stdio.h>
#include <string.h>
#define main checker_main
#include "../../checker.c"
#undef main

/* "Se g(f(x)) = x per ogni x di S e f(g(y)) = y per ogni y di T, allora S
 * e T sono equipotenti." Le righe biject propongono tre coppie; la prova
 * usa la prima, f e g. */
#define INVERSE_HEAD \
    "certificate saturation\n" \
    "sort S\n" \
    "sort T\n" \
    "symbol f S T\n" \
    "symbol g T S\n"
#define INVERSE_STATEMENT \
    "statement (implies (and (and (and true true) (forall x S (= (g (f x)) x))) (forall y T (= (f (g y)) y))) (equipotent S T))\n"
#define INVERSE_BIJECTIONS \
    "clausify\n" \
    "biject 0 (f 0 (v 0)) (f 1 (v 0))\n" \
    "biject 0 (f 0 (v 0)) (f 9 (v 0))\n" \
    "biject 0 (f 8 (v 0)) (f 1 (v 0))\n"
#define INVERSE_STEPS \
    "component (clause (pos (f 7)))\n" \
    "complement 0\n" \
    "step resolve 32 0 22 0 (subst) (clause (pos (f 17)) (neg (f 3 (f 0 (f 11)))) (ne (f 1 (f 0 (f 11))) (f 11)) (neg (f 2 (f 1 (f 12)))) (ne (f 0 (f 1 (f 12))) (f 12)))\n" \
    "component (clause (pos (f 3 (f 0 (f 11)))))\n" \
    "complement 1\n" \
    "step resolve 34 0 33 1 (subst) (clause (neg (f 18)) (pos (f 17)) (ne (f 1 (f 0 (f 11))) (f 11)) (neg (f 2 (f 1 (f 12)))) (ne (f 0 (f 1 (f 12))) (f 12)))\n" \
    "component (clause (eq (f 1 (f 0 (f 11))) (f 11)))\n" \
    "complement 2\n" \
    "step resolve 37 0 36 2 (subst) (clause (neg (f 19)) (neg (f 18)) (pos (f 17)) (neg (f 2 (f 1 (f 12)))) (ne (f 0 (f 1 (f 12))) (f 12)))\n" \
    "component (clause (pos (f 2 (f 1 (f 12)))))\n" \
    "complement 3\n" \
    "step resolve 40 0 39 3 (subst) (clause (neg (f 20)) (neg (f 19)) (neg (f 18)) (pos (f 17)) (ne (f 0 (f 1 (f 12))) (f 12)))\n" \
    "component (clause (eq (f 0 (f 1 (f 12))) (f 12)))\n" \
    "complement 4\n" \
    "step resolve 43 0 42 4 (subst) (clause (neg (f 21)) (neg (f 20)) (neg (f 19)) (neg (f 18)) (pos (f 17)))\n" \
    "step resolve 32 0 21 0 (subst) (clause (pos (f 17)) (neg (f 3 (f 0 (f 11)))) (ne (f 1 (f 0 (f 11))) (f 11)) (pos (f 3 (f 12))))\n" \
    "step resolve 34 0 46 1 (subst) (clause (neg (f 18)) (pos (f 17)) (ne (f 1 (f 0 (f 11))) (f 11)) (pos (f 3 (f 12))))\n" \
    "step resolve 37 0 47 2 (subst) (clause (neg (f 19)) (neg (f 18)) (pos (f 17)) (pos (f 3 (f 12))))\n" \
    "component (clause (pos (f 3 (f 12))))\n" \
    "complement 5\n" \
    "step resolve 50 0 48 3 (subst) (clause (pos (f 22)) (neg (f 19)) (neg (f 18)) (pos (f 17)))\n" \
    "step resolve 32 0 20 0 (subst) (clause (pos (f 17)) (pos (f 2 (f 11))) (neg (f 2 (f 1 (f 12)))) (ne (f 0 (f 1 (f 12))) (f 12)))\n" \
    "component (clause (pos (f 2 (f 11))))\n" \
    "complement 6\n" \
    "step resolve 54 0 52 1 (subst) (clause (pos (f 23)) (pos (f 17)) (neg (f 2 (f 1 (f 12)))) (ne (f 0 (f 1 (f 12))) (f 12)))\n" \
    "step resolve 40 0 55 2 (subst) (clause (neg (f 20)) (pos (f 23)) (pos (f 17)) (ne (f 0 (f 1 (f 12))) (f 12)))\n" \
    "step resolve 43 0 56 3 (subst) (clause (neg (f 21)) (neg (f 20)) (pos (f 23)) (pos (f 17)))\n" \
    "step resolve 32 0 19 0 (subst) (clause (pos (f 17)) (pos (f 2 (f 11))) (pos (f 3 (f 12))))\n" \
    "step resolve 54 0 58 1 (subst) (clause (pos (f 23)) (pos (f 17)) (pos (f 3 (f 12))))\n" \
    "step resolve 50 0 59 2 (subst) (clause (pos (f 22)) (pos (f 23)) (pos (f 17)))\n" \
    "step resolve 18 0 16 0 (subst (0 (v 64)) (1 (v 65))) (clause (neg (f 7)))\n" \
    "step resolve 18 0 15 0 (subst (0 (v 64)) (1 (v 65))) (clause (pos (f 6 (v 0) (v 1))))\n" \
    "step resolve 31 0 61 0 (subst) (clause (neg (f 17)))\n" \
    "step resolve 62 0 8 0 (subst (0 (v 64)) (1 (v 65))) (clause (pos (f 4 (v 0))))\n" \
    "step resolve 62 0 9 0 (subst (0 (v 64)) (1 (v 65))) (clause (pos (f 5 (v 0))))\n" \
    "step resolve 64 0 4 0 (subst (0 (v 64))) (clause (neg (f 2 (v 0))) (eq (f 1 (f 0 (v 0))) (v 0)))\n" \
    "step resolve 65 0 7 0 (subst (0 (v 64))) (clause (neg (f 3 (v 0))) (eq (f 0 (f 1 (v 0))) (v 0)))\n" \
    "step resolve 49 0 67 0 (subst (64 (f 12))) (clause (neg (f 22)) (eq (f 0 (f 1 (f 12))) (f 12)))\n" \
    "step resolve 44 0 68 1 (subst) (clause (pos (f 21)) (neg (f 22)))\n" \
    "step resolve 1 0 41 0 (subst (0 (f 12))) (clause (neg (f 3 (f 12))) (pos (f 20)))\n" \
    "step resolve 49 0 70 0 (subst) (clause (neg (f 22)) (pos (f 20)))\n" \
    "step resolve 66 0 53 0 (subst (0 (f 11))) (clause (eq (f 1 (f 0 (f 11))) (f 11)) (neg (f 23)))\n" \
    "step resolve 38 0 72 0 (subst) (clause (pos (f 19)) (neg (f 23)))\n" \
    "step resolve 0 0 35 0 (subst (0 (f 11))) (clause (neg (f 2 (f 11))) (pos (f 18)))\n" \
    "step resolve 53 0 74 0 (subst) (clause (neg (f 23)) (pos (f 18)))\n" \
    "step resolve 71 1 45 1 (subst) (clause (neg (f 22)) (neg (f 21)) (neg (f 19)) (neg (f 18)) (pos (f 17)))\n" \
    "step resolve 69 0 76 1 (subst) (clause (neg (f 22)) (neg (f 19)) (neg (f 18)) (pos (f 17)))\n" \
    "step resolve 51 0 77 0 (subst) (clause (neg (f 19)) (neg (f 18)) (pos (f 17)))\n" \
    "step resolve 73 0 78 0 (subst) (clause (neg (f 23)) (neg (f 18)) (pos (f 17)))\n" \
    "step resolve 75 1 79 1 (subst) (clause (neg (f 23)) (pos (f 17)))\n" \
    "step resolve 71 1 57 1 (subst) (clause (neg (f 22)) (neg (f 21)) (pos (f 23)) (pos (f 17)))\n" \
    "step resolve 69 0 81 1 (subst) (clause (neg (f 22)) (pos (f 23)) (pos (f 17)))\n" \
    "step resolve 60 0 82 0 (subst) (clause (pos (f 23)) (pos (f 17)))\n" \
    "step resolve 83 0 80 0 (subst) (clause (pos (f 17)))\n" \
    "step resolve 84 0 63 0 (subst) (clause)\n" \
    "end\n"

/* "Se f è iniettiva e ogni y di T è f di un x di S, allora S e T sono
 * equipotenti." La biiezione proposta è f con la funzione di Skolem
 * dell'ipotesi di suriettività, il simbolo 6. */
#define BIJECTIVE_HEAD \
    "certificate saturation\n" \
    "sort S\n" \
    "sort T\n" \
    "symbol f S T\n"
#define BIJECTIVE_STATEMENT \
    "statement (implies (and (and true (forall a S (forall b S (implies (= (f a) (f b)) (= a b))))) (forall y T (exists x S (= (f x) y)))) (equipotent S T))\n"
#define BIJECTIVE_STEPS \
    "clausify\n" \
    "biject 0 (f 0 (v 0)) (f 6 (v 0))\n" \
    "biject 0 (f 0 (v 0)) (f 12 (v 0))\n" \
    "biject 0 (f 11 (v 0)) (f 6 (v 0))\n" \
    "component (clause (pos (f 10)))\n" \
    "complement 0\n" \
    "step resolve 40 0 30 0 (subst) (clause (pos (f 20)) (neg (f 2 (f 0 (f 14)))) (ne (f 6 (f 0 (f 14))) (f 14)) (neg (f 1 (f 6 (f 15)))) (ne (f 0 (f 6 (f 15))) (f 15)))\n" \
    "component (clause (pos (f 2 (f 0 (f 14)))))\n" \
    "complement 1\n" \
    "step resolve 42 0 41 1 (subst) (clause (neg (f 21)) (pos (f 20)) (ne (f 6 (f 0 (f 14))) (f 14)) (neg (f 1 (f 6 (f 15)))) (ne (f 0 (f 6 (f 15))) (f 15)))\n" \
    "component (clause (eq (f 6 (f 0 (f 14))) (f 14)))\n" \
    "complement 2\n" \
    "step resolve 45 0 44 2 (subst) (clause (neg (f 22)) (neg (f 21)) (pos (f 20)) (neg (f 1 (f 6 (f 15)))) (ne (f 0 (f 6 (f 15))) (f 15)))\n" \
    "component (clause (pos (f 1 (f 6 (f 15)))))\n" \
    "complement 3\n" \
    "step resolve 48 0 47 3 (subst) (clause (neg (f 23)) (neg (f 22)) (neg (f 21)) (pos (f 20)) (ne (f 0 (f 6 (f 15))) (f 15)))\n" \
    "component (clause (eq (f 0 (f 6 (f 15))) (f 15)))\n" \
    "complement 4\n" \
    "step resolve 51 0 50 4 (subst) (clause (neg (f 24)) (neg (f 23)) (neg (f 22)) (neg (f 21)) (pos (f 20)))\n" \
    "step resolve 40 0 29 0 (subst) (clause (pos (f 20)) (neg (f 2 (f 0 (f 14)))) (ne (f 6 (f 0 (f 14))) (f 14)) (pos (f 2 (f 15))))\n" \
    "step resolve 42 0 54 1 (subst) (clause (neg (f 21)) (pos (f 20)) (ne (f 6 (f 0 (f 14))) (f 14)) (pos (f 2 (f 15))))\n" \
    "step resolve 45 0 55 2 (subst) (clause (neg (f 22)) (neg (f 21)) (pos (f 20)) (pos (f 2 (f 15))))\n" \
    "component (clause (pos (f 2 (f 15))))\n" \
    "complement 5\n" \
    "step resolve 58 0 56 3 (subst) (clause (pos (f 25)) (neg (f 22)) (neg (f 21)) (pos (f 20)))\n" \
    "step resolve 40 0 28 0 (subst) (clause (pos (f 20)) (pos (f 1 (f 14))) (neg (f 1 (f 6 (f 15)))) (ne (f 0 (f 6 (f 15))) (f 15)))\n" \
    "component (clause (pos (f 1 (f 14))))\n" \
    "complement 6\n" \
    "step resolve 62 0 60 1 (subst) (clause (pos (f 26)) (pos (f 20)) (neg (f 1 (f 6 (f 15)))) (ne (f 0 (f 6 (f 15))) (f 15)))\n" \
    "step resolve 48 0 63 2 (subst) (clause (neg (f 23)) (pos (f 26)) (pos (f 20)) (ne (f 0 (f 6 (f 15))) (f 15)))\n" \
    "step resolve 51 0 64 3 (subst) (clause (neg (f 24)) (neg (f 23)) (pos (f 26)) (pos (f 20)))\n" \
    "step resolve 40 0 27 0 (subst) (clause (pos (f 20)) (pos (f 1 (f 14))) (pos (f 2 (f 15))))\n" \
    "step resolve 62 0 66 1 (subst) (clause (pos (f 26)) (pos (f 20)) (pos (f 2 (f 15))))\n" \
    "step resolve 58 0 67 2 (subst) (clause (pos (f 25)) (pos (f 26)) (pos (f 20)))\n" \
    "step resolve 26 0 24 0 (subst (0 (v 64)) (1 (v 65)) (2 (v 66))) (clause (neg (f 10)))\n" \
    "step resolve 26 0 23 0 (subst (0 (v 64)) (1 (v 65)) (2 (v 66))) (clause (pos (f 9 (v 0) (v 1) (v 2))))\n" \
    "step resolve 39 0 69 0 (subst) (clause (neg (f 20)))\n" \
    "step resolve 70 0 17 0 (subst (0 (v 64)) (1 (v 65)) (2 (v 66))) (clause (pos (f 8 (v 0))))\n" \
    "step resolve 15 0 72 0 (subst (0 (v 64))) (clause (neg (f 2 (v 0))) (pos (f 7 (v 0))))\n" \
    "step resolve 57 0 73 0 (subst (64 (f 15))) (clause (neg (f 25)) (pos (f 7 (f 15))))\n" \
    "step resolve 70 0 16 0 (subst (0 (v 64)) (1 (v 65)) (2 (v 66))) (clause (pos (f 5 (v 0) (v 1))))\n" \
    "step resolve 11 0 74 1 (subst (0 (f 15))) (clause (eq (f 0 (f 6 (f 15))) (f 15)) (neg (f 25)))\n" \
    "step resolve 52 0 76 0 (subst) (clause (pos (f 24)) (neg (f 25)))\n" \
    "step resolve 10 1 49 0 (subst (0 (f 15))) (clause (neg (f 7 (f 15))) (pos (f 23)))\n" \
    "step resolve 74 1 78 0 (subst) (clause (neg (f 25)) (pos (f 23)))\n" \
    "step resolve 0 0 43 0 (subst (0 (f 14))) (clause (neg (f 1 (f 14))) (pos (f 21)))\n" \
    "step resolve 61 0 80 0 (subst) (clause (neg (f 26)) (pos (f 21)))\n" \
    "step resolve 73 0 42 0 (subst (0 (f 0 (f 14)))) (clause (pos (f 7 (f 0 (f 14)))) (neg (f 21)))\n" \
    "step resolve 11 0 82 0 (subst (0 (f 0 (f 14)))) (clause (eq (f 0 (f 6 (f 0 (f 14)))) (f 0 (f 14))) (neg (f 21)))\n" \
    "step resolve 75 0 9 0 (subst (0 (v 64)) (1 (v 65))) (clause (neg (f 1 (v 0))) (pos (f 4 (v 0) (v 1))))\n" \
    "step resolve 6 0 84 1 (subst (0 (v 64)) (1 (v 65))) (clause (neg (f 1 (v 0))) (pos (f 3 (v 1) (v 0))) (neg (f 1 (v 1))))\n" \
    "step super 83 0 0 3 1 0 0 (subst (64 (f 6 (f 0 (f 14))))) (clause (neg (f 21)) (neg (f 3 (f 6 (f 0 (f 14))) (v 0))) (ne (f 0 (f 14)) (f 0 (v 0))) (eq (f 6 (f 0 (f 14))) (v 0)))\n" \
    "step reflex 86 2 (subst (0 (f 14))) (clause (neg (f 21)) (neg (f 3 (f 6 (f 0 (f 14))) (f 14))) (eq (f 6 (f 0 (f 14))) (f 14)))\n" \
    "step resolve 46 0 87 2 (subst) (clause (pos (f 22)) (neg (f 21)) (neg (f 3 (f 6 (f 0 (f 14))) (f 14))))\n" \
    "step resolve 85 1 88 2 (subst (0 (f 14)) (1 (f 6 (f 0 (f 14))))) (clause (neg (f 1 (f 14))) (neg (f 1 (f 6 (f 0 (f 14))))) (pos (f 22)) (neg (f 21)))\n" \
    "step resolve 61 0 89 0 (subst) (clause (neg (f 26)) (neg (f 1 (f 6 (f 0 (f 14))))) (pos (f 22)) (neg (f 21)))\n" \
    "step resolve 10 1 90 1 (subst (0 (f 0 (f 14)))) (clause (neg (f 7 (f 0 (f 14)))) (neg (f 26)) (pos (f 22)) (neg (f 21)))\n" \
    "step resolve 82 0 91 0 (subst) (clause (neg (f 21)) (neg (f 26)) (pos (f 22)))\n" \
    "step resolve 79 1 53 1 (subst) (clause (neg (f 25)) (neg (f 24)) (neg (f 22)) (neg (f 21)) (pos (f 20)))\n" \
    "step resolve 77 0 93 1 (subst) (clause (neg (f 25)) (neg (f 22)) (neg (f 21)) (pos (f 20)))\n" \
    "step resolve 59 0 94 0 (subst) (clause (neg (f 22)) (neg (f 21)) (pos (f 20)))\n" \
    "step resolve 92 2 95 0 (subst) (clause (neg (f 21)) (neg (f 26)) (pos (f 20)))\n" \
    "step resolve 81 1 96 0 (subst) (clause (neg (f 26)) (pos (f 20)))\n" \
    "step resolve 79 1 65 1 (subst) (clause (neg (f 25)) (neg (f 24)) (pos (f 26)) (pos (f 20)))\n" \
    "step resolve 77 0 98 1 (subst) (clause (neg (f 25)) (pos (f 26)) (pos (f 20)))\n" \
    "step resolve 68 0 99 0 (subst) (clause (pos (f 26)) (pos (f 20)))\n" \
    "step resolve 100 0 97 0 (subst) (clause (pos (f 20)))\n" \
    "step resolve 101 0 71 0 (subst) (clause)\n" \
    "end\n"

/* Controlla il certificato text e ritorna la risposta del verificatore. */
static CkAnswer answer(const char *text) {
    const char *cursor = text, *why = "";
    return ck_check(&cursor, &why, 0);
}

/* Conta un errore e lo stampa, se condition è falsa. */
static int expect(int condition, const char *what) {
    if (!condition) printf("FAILED: %s\n", what);
    return !condition;
}

/* I certificati: quelli giusti accettati, quelli alterati no. */
static int certificates(void) {
    int errors = 0;
    errors += expect(answer(INVERSE_HEAD INVERSE_STATEMENT INVERSE_BIJECTIONS INVERSE_STEPS) == CK_ACCEPTED,
                     "two maps that undo each other prove the equipotence");
    errors += expect(answer(BIJECTIVE_HEAD BIJECTIVE_STATEMENT BIJECTIVE_STEPS) == CK_ACCEPTED,
                     "an injective and surjective map proves the equipotence, with the Skolem function as its inverse");

    /* Gli stessi passi con un'ipotesi in meno: un enunciato falso. */
    errors += expect(answer(INVERSE_HEAD "statement (implies (and (and (and true true) (forall x S (= (g (f x)) x))) true) (equipotent S T))\n"
                            INVERSE_BIJECTIONS INVERSE_STEPS) != CK_ACCEPTED,
                     "a left inverse alone does not prove the equipotence");
    errors += expect(answer(INVERSE_HEAD "statement (implies (and (and (and true true) true) (forall y T (= (f (g y)) y))) (equipotent S T))\n"
                            INVERSE_BIJECTIONS INVERSE_STEPS) != CK_ACCEPTED,
                     "a right inverse alone does not prove the equipotence");
    errors += expect(answer(BIJECTIVE_HEAD "statement (implies (and (and true (forall a S (forall b S (implies (= (f a) (f b)) (= a b))))) true) (equipotent S T))\n"
                            BIJECTIVE_STEPS) != CK_ACCEPTED,
                     "an injective map alone does not prove the equipotence");
    errors += expect(answer(BIJECTIVE_HEAD "statement (implies (and (and true true) (forall y T (exists x S (= (f x) y)))) (equipotent S T))\n"
                            BIJECTIVE_STEPS) != CK_ACCEPTED,
                     "a surjective map alone does not prove the equipotence");

    /* La riga biject alterata, con i passi di prima. */
    errors += expect(answer(INVERSE_HEAD INVERSE_STATEMENT "clausify\nbiject 0 (f 1 (v 0)) (f 0 (v 0))\nbiject 0 (f 0 (v 0)) (f 9 (v 0))\n"
                            "biject 0 (f 8 (v 0)) (f 1 (v 0))\n" INVERSE_STEPS) != CK_ACCEPTED,
                     "the two maps written the other way round are not the bijection the steps use");
    errors += expect(answer(INVERSE_HEAD INVERSE_STATEMENT "clausify\nbiject 1 (f 0 (v 0)) (f 1 (v 0))\n" INVERSE_STEPS) != CK_ACCEPTED,
                     "an equipotence that the statement does not have is rejected");
    errors += expect(answer(INVERSE_HEAD INVERSE_STATEMENT "clausify\nbiject 0 (f 0 (v 1)) (f 1 (v 0))\n" INVERSE_STEPS) != CK_ACCEPTED,
                     "a map written with a second variable is rejected");
    errors += expect(answer(INVERSE_HEAD INVERSE_STATEMENT "clausify\nbiject 0 (f 2 (v 0)) (f 1 (v 0))\n" INVERSE_STEPS) != CK_ACCEPTED,
                     "a predicate is not a map");

    /* La sola riga, senza i passi: proporre una biiezione non la dimostra. */
    errors += expect(answer(INVERSE_HEAD INVERSE_STATEMENT "clausify\nbiject 0 (f 0 (v 0)) (f 1 (v 0))\nend\n") != CK_ACCEPTED,
                     "a bijection that is only offered proves nothing");
    errors += expect(answer("certificate saturation\nsort S\nsort T\nsymbol f S T\nsymbol g T S\nstatement (equipotent S T)\nclausify\n"
                            "biject 0 (f 0 (v 0)) (f 1 (v 0))\nend\n") != CK_ACCEPTED,
                     "two maps of which nothing is known do not prove the equipotence");
    return errors;
}

/* Prepara il verificatore con i due insiemi S e T, senza simboli
 * dichiarati, clausifica (equipotent S T) e aggiunge la riga
 * "biject 0 t u" con t e u le due funzioni dell'equipotenza stessa, i
 * simboli 3 e 4: due simboli di funzione qualsiasi, che l'oracolo
 * interpreta in tutti i modi. Ritorna 0 se non ci riesce. */
static int prepare(void) {
    static const char *const names[2] = {"S", "T"};
    ck_status = CK_ACCEPTED;
    ck_used = ck_depth = 0;
    ck_sides_count = 0;
    ck_sort_count = 2;
    ck_symbol_count = ck_declared = 0;
    if (!ck_sorts_reserve((size_t)ck_sort_count)) return 0;
    memset(ck_sorts, 0, (size_t)ck_sort_count * sizeof *ck_sorts);
    for (int i = 0; i < 2; i++) { ck_sorts[i].name = names[i]; ck_sorts[i].size = (size_t)-1; ck_sorts[i].base = ck_sorts[i].target = ck_sorts[i].symbol = -1; }
    ck_cursor = "(equipotent S T)";
    const CkNode *root = ck_read_condition();
    if (ck_status != CK_ACCEPTED) return 0;
    ck_r_prepare(root, NULL, 0);
    if (ck_status != CK_ACCEPTED) return 0;
    ck_cursor = "0 (f 3 (v 0)) (f 4 (v 0))";
    ck_r_biject();
    return ck_status == CK_ACCEPTED;
}

/* L'interpretazione provata dall'oracolo su {0, 1, 2}: i due sottoinsiemi
 * come bit, la proposizione, le due funzioni, e i valori delle due
 * costanti nuove della riga biject. */
static int part_s, part_t, proposition, there[3], back[3], first_constant, second_constant;

/* Il valore del termine chiuso t. I simboli: 3 e 4 le due funzioni, 5 e 6
 * le due costanti. */
static int term_value(int t) {
    CkRTerm n = ck_r.term[t];
    if (n.symbol == 5) return first_constant;
    if (n.symbol == 6) return second_constant;
    int argument = term_value(ck_r.argument[n.first]);
    return n.symbol == 3 ? there[argument] : back[argument];
}

/* La verità del letterale chiuso l: 0 e 1 sono i domini di S e di T, 2 la
 * proposizione dell'equipotenza. */
static int literal_holds(CkRLiteral l) {
    int truth;
    if (l.right) truth = term_value(l.left) == term_value(l.right);
    else {
        CkRTerm n = ck_r.term[l.left];
        truth = n.symbol == 2 ? proposition : (n.symbol == 0 ? part_s : part_t) >> term_value(ck_r.argument[n.first]) & 1;
    }
    return truth != l.negative;
}

/* Ritorna 1 se le quattro clausole della riga biject, le ultime, valgono. */
static int rule_holds(void) {
    for (int i = ck_r.clauses - 4; i < ck_r.clauses; i++) {
        CkRClause c = ck_r.clause[i];
        int holds = 0;
        for (int j = 0; j < c.count; j++) holds = holds || literal_holds(c.literal[j]);
        if (!holds) return 0;
    }
    return 1;
}

/* L'oracolo. Per ogni coppia di sottoinsiemi A e B di {0, 1, 2} e ogni
 * coppia di funzioni t e u:
 *   - con la proposizione vera le quattro clausole valgono, qualunque
 *     valore abbiano le due costanti;
 *   - con la proposizione falsa c'è un valore delle due costanti che le
 *     rende vere esattamente quando t e u NON si disfano a vicenda tra A
 *     e B.
 * Insieme dicono che la riga impone la proposizione solo davanti a una
 * biiezione, e che dove la proposizione ha il valore dell'equipotenza le
 * clausole hanno sempre un modello (la regola non dimostra il falso),
 * purché una biiezione ci sia solo tra insiemi equipotenti: l'ultimo
 * controllo, che l'oracolo fa contando. */
static int oracle(void) {
    int errors = 0;
    errors += expect(prepare(), "the statement is clausified and the bijection offered");

    /* Simboli: 0 e 1 i domini, 2 la proposizione, 3 e 4 le due funzioni,
     * 5 e 6 le costanti nuove. Clausole: le quattro di 11m-e, l'enunciato
     * negato, le quattro della riga. */
    errors += expect(ck_r.symbols == 7 && ck_r.clauses == 9, "the line gives two constants and four clauses");
    if (errors) return errors;
    static const int count[8] = {0, 1, 1, 2, 1, 2, 2, 3};
    int always = 1, exactly = 1, counted = 1;
    for (part_s = 0; part_s < 8; part_s++) for (part_t = 0; part_t < 8; part_t++)
        for (int f = 0; f < 27; f++) for (int g = 0; g < 27; g++) {
            there[0] = f % 3; there[1] = f / 3 % 3; there[2] = f / 9;
            back[0] = g % 3; back[1] = g / 3 % 3; back[2] = g / 9;
            int inverse = 1, some = 0;
            for (int x = 0; x < 3; x++) {
                if (part_s >> x & 1) inverse = inverse && (part_t >> there[x] & 1) && back[there[x]] == x;
                if (part_t >> x & 1) inverse = inverse && (part_s >> back[x] & 1) && there[back[x]] == x;
            }
            for (first_constant = 0; first_constant < 3; first_constant++)
                for (second_constant = 0; second_constant < 3; second_constant++) {
                    proposition = 1;
                    always = always && rule_holds();
                    proposition = 0;
                    some = some || rule_holds();
                }
            exactly = exactly && some == !inverse;
            counted = counted && (!inverse || count[part_s] == count[part_t]);
        }
    errors += expect(always, "with the proposition true the clauses of the line hold whatever the constants are");
    errors += expect(exactly, "with the proposition false the clauses have a model exactly when the two maps do not undo each other");
    errors += expect(counted, "two maps that undo each other join sets with as many elements");
    return errors;
}

int main(void) {
    int errors = certificates() + oracle();
    if (errors) { printf("%d checks failed\n", errors); return 1; }
    printf("equipotence proved in saturation certificates: all checks passed\n");
    return 0;
}
