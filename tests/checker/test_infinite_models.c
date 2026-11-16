/* Test di 20g del verificatore: un controesempio infinito, un'interpretazione
 * sugli interi (certificate infinite). Compilato senza il motore.
 *
 * Ciò che 20g aggiunge a 20f è l'affermazione: le chiusure e la negazione
 * dell'enunciato tradotto nell'interpretazione, con i cases espansi e i
 * testimoni sostituiti. Se fosse sbagliata, un enunciato vero in
 * un'interpretazione vi risulterebbe falso. Qui:
 *
 *   - due certificati scritti da Vampire ("una f iniettiva di S in sé è
 *     suriettiva", con S i naturali e f(x) = x + 1; "una f suriettiva è
 *     iniettiva", con f(x) = x - 1 per x > 0 e f(0) = 0, e due testimoni)
 *     sono accettati, e respinti con un'altra interpretazione (f
 *     l'identità, dove l'enunciato è vero; f(x) = x - 1, che manda 0 fuori
 *     da S), con l'enunciato negato, senza l'ultimo passo, con un testimone
 *     sbagliato;
 *   - le regole delle voci: un testimone per un quantificatore che non vale
 *     come un esiste, una definizione con una lettera in più o con un
 *     quantificatore, un insieme senza interpret, cases fuori da define;
 *   - un oracolo per l'affermazione: per enunciati e interpretazioni a caso,
 *     il valore dell'affermazione, calcolato da questo file con ogni
 *     quantificatore ristretto agli interi da -4 a 4, è quello di "le
 *     chiusure valgono e l'enunciato no", che questo file calcola da sé
 *     sull'enunciato e con l'interpretazione scritta in C, con la stessa
 *     restrizione; e con un testimone in quella finestra l'affermazione
 *     vera resta vera senza (un testimone la rende solo più forte).
 *
 * Compilazione ed esecuzione dalla cartella principale:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_infinite_models \
 *         tests/checker/test_infinite_models.c && /tmp/test_infinite_models
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

static CkAnswer answer_of(const char *text) {
    const char *cursor = text, *why = "";
    return ck_check(&cursor, &why, 0);
}

/* Il testo t con la prima occorrenza di from sostituita da to. */
static char *replaced(const char *t, const char *from, const char *to) {
    static char out[1 << 17];
    const char *at = strstr(t, from);
    if (!at) { snprintf(out, sizeof out, "%s", t); return out; }
    snprintf(out, sizeof out, "%.*s%s%s", (int)(at - t), t, to, at + strlen(from));
    return out;
}

/* "Una f iniettiva di S in sé è suriettiva": falso con S = N_0 e f(x) = x + 1. */
static const char *const INJECTIVE =
    "certificate infinite\n"
    "sort S\n"
    "symbol f S S\n"
    "statement (implies (forall a S (forall b S (implies (= (f a) (f b)) (= a b)))) (forall y S (exists x S (= (f x) y))))\n"
    "interpret S (set ge 0)\n"
    "define f (add (var 0) 1)\n"
    "witness 3 0\n"
    "clausify forward\n"
    "step compute (clause (ne (f 4) (f 5)))\n"
    "step compute (clause (pos (f 3 (f 4) (f 5))))\n"
    "step compute (clause (neg (f 3 (f 5) (f 4))))\n"
    "component (clause (pos (f 18)))\n"
    "complement 0\n"
    "step resolve 48 0 12 0 (subst) (clause (neg (f 32)) (pos (f 8)) (pos (f 17)))\n"
    "component (clause (pos (f 8)))\n"
    "complement 1\n"
    "step resolve 52 0 50 1 (subst) (clause (pos (f 33)) (neg (f 32)) (pos (f 17)))\n"
    "component (clause (pos (f 17)))\n"
    "complement 2\n"
    "step resolve 55 0 53 2 (subst) (clause (pos (f 34)) (pos (f 33)) (neg (f 32)))\n"
    "step resolve 54 0 11 0 (subst) (clause (neg (f 34)) (pos (f 13)) (pos (f 16)))\n"
    "component (clause (pos (f 13)))\n"
    "complement 3\n"
    "step resolve 59 0 57 1 (subst) (clause (pos (f 35)) (neg (f 34)) (pos (f 16)))\n"
    "component (clause (pos (f 16)))\n"
    "complement 4\n"
    "step resolve 62 0 60 2 (subst) (clause (pos (f 36)) (pos (f 35)) (neg (f 34)))\n"
    "step resolve 61 0 10 0 (subst) (clause (neg (f 36)) (pos (f 3 (f 4) (f 4))) (pos (f 15)))\n"
    "component (clause (pos (f 3 (f 4) (f 4))))\n"
    "complement 5\n"
    "step resolve 66 0 64 1 (subst) (clause (pos (f 37)) (neg (f 36)) (pos (f 15)))\n"
    "component (clause (pos (f 15)))\n"
    "complement 6\n"
    "step resolve 69 0 67 2 (subst) (clause (pos (f 38)) (pos (f 37)) (neg (f 36)))\n"
    "step resolve 68 0 9 0 (subst) (clause (neg (f 38)) (eq (f 0 (f 14) (f 5)) (f 4)))\n"
    "component (clause (eq (f 0 (f 14) (f 5)) (f 4)))\n"
    "complement 7\n"
    "step resolve 73 0 71 1 (subst) (clause (pos (f 39)) (neg (f 38)))\n"
    "step resolve 68 0 8 0 (subst) (clause (neg (f 38)) (neg (f 3 (f 14) (f 4))))\n"
    "component (clause (pos (f 3 (f 14) (f 4))))\n"
    "complement 8\n"
    "step resolve 76 0 75 1 (subst) (clause (neg (f 40)) (neg (f 38)))\n"
    "step resolve 58 0 7 0 (subst) (clause (neg (f 35)) (pos (f 12)))\n"
    "component (clause (pos (f 12)))\n"
    "complement 9\n"
    "step resolve 81 0 79 1 (subst) (clause (pos (f 41)) (neg (f 35)))\n"
    "step resolve 80 0 5 0 (subst) (clause (neg (f 41)) (pos (f 11)))\n"
    "component (clause (pos (f 11)))\n"
    "complement 10\n"
    "step resolve 85 0 83 1 (subst) (clause (pos (f 42)) (neg (f 41)))\n"
    "step resolve 84 0 3 0 (subst) (clause (neg (f 42)) (ne (f 9) (f 10)))\n"
    "component (clause (eq (f 9) (f 10)))\n"
    "complement 11\n"
    "step resolve 88 0 87 1 (subst) (clause (neg (f 43)) (neg (f 42)))\n"
    "step resolve 84 0 2 0 (subst) (clause (neg (f 42)) (eq (f 0 (f 9) (f 5)) (f 0 (f 10) (f 5))))\n"
    "component (clause (eq (f 0 (f 9) (f 5)) (f 0 (f 10) (f 5))))\n"
    "complement 12\n"
    "step resolve 93 0 91 1 (subst) (clause (pos (f 44)) (neg (f 42)))\n"
    "step resolve 51 0 1 0 (subst) (clause (neg (f 33)) (pos (f 3 (f 0 (f 7) (f 5)) (f 4))))\n"
    "component (clause (pos (f 3 (f 0 (f 7) (f 5)) (f 4))))\n"
    "complement 13\n"
    "step resolve 97 0 95 1 (subst) (clause (pos (f 45)) (neg (f 33)))\n"
    "step resolve 51 0 0 0 (subst) (clause (neg (f 33)) (neg (f 3 (f 7) (f 4))))\n"
    "component (clause (pos (f 3 (f 7) (f 4))))\n"
    "complement 14\n"
    "step resolve 100 0 99 1 (subst) (clause (neg (f 46)) (neg (f 33)))\n"
    "step resolve 49 0 13 0 (subst) (clause (pos (f 32)))\n"
    "step super 16 0 0 14 0 0 0 (subst (0 (v 64)) (65 (f 4))) (clause (eq (v 0) (f 0 (f 4) (v 0))))\n"
    "step super 14 0 0 96 0 0 1 (subst (0 (f 7)) (1 (f 5))) (clause (pos (f 3 (f 0 (f 5) (f 7)) (f 4))) (neg (f 45)))\n"
    "step resolve 37 0 36 0 (subst (0 (v 64)) (1 (v 65))) (clause (pos (f 3 (v 0) (v 1))) (pos (f 26 (v 0) (v 1))))\n"
    "step resolve 35 0 106 1 (subst (0 (v 64)) (1 (v 65))) (clause (eq (v 0) (v 1)) (pos (f 3 (v 1) (v 0))) (pos (f 3 (v 0) (v 1))))\n"
    "step resolve 101 0 107 1 (subst (64 (f 4)) (65 (f 7))) (clause (pos (f 46)) (eq (f 4) (f 7)) (pos (f 3 (f 4) (f 7))))\n"
    "component (clause (eq (f 4) (f 7)))\n"
    "complement 15\n"
    "step resolve 110 0 108 1 (subst) (clause (pos (f 47)) (pos (f 46)) (pos (f 3 (f 4) (f 7))))\n"
    "component (clause (pos (f 3 (f 4) (f 7))))\n"
    "complement 16\n"
    "step resolve 113 0 111 2 (subst) (clause (pos (f 48)) (pos (f 47)) (pos (f 46)))\n"
    "step super 109 0 1 105 0 0 3 (subst) (clause (neg (f 47)) (pos (f 3 (f 0 (f 5) (f 4)) (f 4))) (neg (f 45)))\n"
    "step super 16 0 0 115 1 0 1 (subst (0 (f 5))) (clause (neg (f 47)) (pos (f 3 (f 5) (f 4))) (neg (f 45)))\n"
    "step resolve 47 0 116 1 (subst) (clause (neg (f 47)) (neg (f 45)))\n"
    "step super 17 0 0 15 0 0 1 (subst (0 (v 64)) (65 (f 1 (v 64)))) (clause (eq (f 0 (f 4) (v 0)) (f 0 (v 1) (f 0 (f 1 (v 1)) (v 0)))))\n"
    "step super 104 0 1 118 0 0 0 (subst (0 (v 64))) (clause (eq (v 0) (f 0 (v 1) (f 0 (f 1 (v 1)) (v 0)))))\n"
    "step super 17 0 0 119 0 1 2 (subst (0 (f 1 (v 65))) (64 (f 1 (f 1 (v 65))))) (clause (eq (f 1 (f 1 (v 0))) (f 0 (v 0) (f 4))))\n"
    "step super 16 0 0 120 0 1 0 (subst (0 (v 64))) (clause (eq (f 1 (f 1 (v 0))) (v 0)))\n"
    "step super 121 0 0 119 0 1 4 (subst (65 (f 1 (v 0)))) (clause (eq (v 0) (f 0 (f 1 (v 1)) (f 0 (v 1) (v 0)))))\n"
    "step resolve 34 0 33 0 (subst (0 (v 64)) (1 (v 65)) (2 (v 66))) (clause (pos (f 24 (v 0) (v 1) (v 2))) (pos (f 3 (v 0) (v 2))))\n"
    "step resolve 32 0 123 0 (subst (0 (v 64)) (1 (v 65)) (2 (v 66))) (clause (neg (f 3 (v 0) (v 1))) (neg (f 3 (v 1) (v 2))) (pos (f 3 (v 0) (v 2))))\n"
    "step resolve 105 0 124 0 (subst (64 (f 0 (f 5) (f 7))) (65 (f 4))) (clause (neg (f 45)) (neg (f 3 (f 4) (v 0))) (pos (f 3 (f 0 (f 5) (f 7)) (v 0))))\n"
    "step resolve 112 0 125 1 (subst (64 (f 7))) (clause (neg (f 48)) (neg (f 45)) (pos (f 3 (f 0 (f 5) (f 7)) (f 7))))\n"
    "step resolve 124 0 126 2 (subst (0 (f 0 (f 5) (f 7))) (1 (f 7))) (clause (neg (f 3 (f 7) (v 0))) (pos (f 3 (f 0 (f 5) (f 7)) (v 0))) (neg (f 48)) (neg (f 45)))\n"
    "step resolve 39 0 38 0 (subst (0 (v 64)) (1 (v 65)) (2 (v 66))) (clause (neg (f 3 (v 0) (v 1))) (pos (f 3 (f 0 (v 0) (v 2)) (f 0 (v 1) (v 2)))))\n"
    "step resolve 46 0 128 0 (subst (64 (f 4)) (65 (f 5))) (clause (pos (f 3 (f 0 (f 4) (v 0)) (f 0 (f 5) (v 0)))))\n"
    "step super 104 0 1 129 0 0 1 (subst (0 (v 64))) (clause (pos (f 3 (v 0) (f 0 (f 5) (v 0)))))\n"
    "step resolve 127 0 130 0 (subst (0 (f 0 (f 5) (f 7))) (64 (f 7))) (clause (pos (f 3 (f 0 (f 5) (f 7)) (f 0 (f 5) (f 7)))) (neg (f 48)) (neg (f 45)))\n"
    "step resolve 31 0 131 0 (subst (0 (f 0 (f 5) (f 7)))) (clause (neg (f 48)) (neg (f 45)))\n"
    "step super 14 0 0 72 0 0 0 (subst (0 (f 14)) (1 (f 5))) (clause (eq (f 0 (f 5) (f 14)) (f 4)) (neg (f 39)))\n"
    "step super 133 0 0 130 0 0 2 (subst (64 (f 14))) (clause (neg (f 39)) (pos (f 3 (f 14) (f 4))))\n"
    "step resolve 77 0 134 1 (subst) (clause (pos (f 40)) (neg (f 39)))\n"
    "step resolve 31 0 65 0 (subst (0 (f 4))) (clause (neg (f 37)))\n"
    "step super 14 0 0 92 0 1 0 (subst (0 (f 10)) (1 (f 5))) (clause (eq (f 0 (f 9) (f 5)) (f 0 (f 5) (f 10))) (neg (f 44)))\n"
    "step super 14 0 0 137 0 0 0 (subst (0 (f 9)) (1 (f 5))) (clause (eq (f 0 (f 5) (f 9)) (f 0 (f 5) (f 10))) (neg (f 44)))\n"
    "step super 138 0 1 122 0 1 3 (subst (64 (f 10)) (65 (f 5))) (clause (neg (f 44)) (eq (f 10) (f 0 (f 1 (f 5)) (f 0 (f 5) (f 9)))))\n"
    "step super 122 0 1 139 1 1 0 (subst (0 (f 9)) (1 (f 5))) (clause (neg (f 44)) (eq (f 10) (f 9)))\n"
    "step resolve 89 0 140 1 (subst) (clause (pos (f 43)) (neg (f 44)))\n"
    "step resolve 141 0 90 0 (subst) (clause (neg (f 44)) (neg (f 42)))\n"
    "step resolve 94 0 142 0 (subst) (clause (neg (f 42)))\n"
    "step resolve 114 1 117 0 (subst) (clause (pos (f 48)) (pos (f 46)) (neg (f 45)))\n"
    "step resolve 144 0 132 0 (subst) (clause (pos (f 46)) (neg (f 45)))\n"
    "step resolve 98 0 145 1 (subst) (clause (neg (f 33)) (pos (f 46)))\n"
    "step resolve 146 1 102 0 (subst) (clause (neg (f 33)))\n"
    "step resolve 135 0 78 0 (subst) (clause (neg (f 39)) (neg (f 38)))\n"
    "step resolve 74 0 148 0 (subst) (clause (neg (f 38)))\n"
    "step resolve 70 0 149 0 (subst) (clause (pos (f 37)) (neg (f 36)))\n"
    "step resolve 63 0 150 1 (subst) (clause (pos (f 35)) (neg (f 34)) (pos (f 37)))\n"
    "step resolve 56 0 151 1 (subst) (clause (pos (f 33)) (neg (f 32)) (pos (f 35)) (pos (f 37)))\n"
    "step resolve 152 0 147 0 (subst) (clause (neg (f 32)) (pos (f 35)) (pos (f 37)))\n"
    "step resolve 153 1 82 1 (subst) (clause (neg (f 32)) (pos (f 37)) (pos (f 41)))\n"
    "step resolve 154 2 86 1 (subst) (clause (neg (f 32)) (pos (f 37)) (pos (f 42)))\n"
    "step resolve 155 2 143 0 (subst) (clause (neg (f 32)) (pos (f 37)))\n"
    "step resolve 156 1 136 0 (subst) (clause (neg (f 32)))\n"
    "step resolve 103 0 157 0 (subst) (clause)\n"
    "end\n"
    ;

/* "Una f suriettiva di S in sé è iniettiva": falso con f(x) = x - 1 per x > 0,
 * f(0) = 0; i testimoni sono 0 e 1, dove f vale 0. */
static const char *const SURJECTIVE =
    "certificate infinite\n"
    "sort S\n"
    "symbol f S S\n"
    "statement (implies (forall y S (exists x S (= (f x) y))) (forall a S (forall b S (implies (= (f a) (f b)) (= a b)))))\n"
    "interpret S (set ge 0)\n"
    "define f (cases (less 0 (var 0)) (sub (var 0) 1) 0)\n"
    "witness 3 0\n"
    "witness 4 1\n"
    "clausify forward\n"
    "step compute (clause (ne (f 4) (f 5)))\n"
    "step compute (clause (pos (f 3 (f 4) (f 5))))\n"
    "step compute (clause (neg (f 3 (f 5) (f 4))))\n"
    "component (clause (pos (f 11)))\n"
    "complement 0\n"
    "step resolve 65 0 28 1 (subst) (clause (pos (f 44)) (neg (f 30 (v 0))) (pos (f 29 (v 0))))\n"
    "component (clause (neg (f 30 (v 0))) (pos (f 29 (v 0))))\n"
    "step split 66 (groups (1 0 1 2)) (clause (pos (f 45)) (pos (f 44)))\n"
    "component (clause (pos (f 28)))\n"
    "complement 2\n"
    "step resolve 70 0 27 2 (subst) (clause (pos (f 46)) (neg (f 29 (v 0))) (pos (f 17 (v 0))))\n"
    "component (clause (neg (f 29 (v 0))) (pos (f 17 (v 0))))\n"
    "step split 71 (groups (3 0 1 2)) (clause (pos (f 47)) (pos (f 46)))\n"
    "step resolve 69 0 26 0 (subst) (clause (neg (f 46)) (pos (f 3 (f 4) (f 4))) (pos (f 27)))\n"
    "component (clause (pos (f 3 (f 4) (f 4))))\n"
    "complement 4\n"
    "step resolve 76 0 74 1 (subst) (clause (pos (f 48)) (neg (f 46)) (pos (f 27)))\n"
    "component (clause (pos (f 27)))\n"
    "complement 5\n"
    "step resolve 79 0 77 2 (subst) (clause (pos (f 49)) (pos (f 48)) (neg (f 46)))\n"
    "step resolve 78 0 25 0 (subst) (clause (neg (f 49)) (pos (f 3 (f 5) (f 4))) (pos (f 26)))\n"
    "component (clause (pos (f 3 (f 5) (f 4))))\n"
    "complement 6\n"
    "step resolve 83 0 81 1 (subst) (clause (pos (f 50)) (neg (f 49)) (pos (f 26)))\n"
    "component (clause (pos (f 26)))\n"
    "complement 7\n"
    "step resolve 86 0 84 2 (subst) (clause (pos (f 51)) (pos (f 50)) (neg (f 49)))\n"
    "step resolve 85 0 24 0 (subst) (clause (neg (f 51)) (pos (f 25)) (eq (f 4) (f 5)))\n"
    "component (clause (pos (f 25)))\n"
    "complement 8\n"
    "step resolve 90 0 88 1 (subst) (clause (pos (f 52)) (neg (f 51)) (eq (f 4) (f 5)))\n"
    "component (clause (eq (f 4) (f 5)))\n"
    "complement 9\n"
    "step resolve 93 0 91 2 (subst) (clause (pos (f 53)) (pos (f 52)) (neg (f 51)))\n"
    "step resolve 89 0 23 0 (subst) (clause (neg (f 52)) (pos (f 24)))\n"
    "component (clause (pos (f 24)))\n"
    "complement 10\n"
    "step resolve 97 0 95 1 (subst) (clause (pos (f 54)) (neg (f 52)))\n"
    "step resolve 96 0 21 0 (subst) (clause (neg (f 54)) (pos (f 3 (f 4) (f 4))) (pos (f 23)))\n"
    "step resolve 76 0 99 1 (subst) (clause (pos (f 48)) (neg (f 54)) (pos (f 23)))\n"
    "component (clause (pos (f 23)))\n"
    "complement 11\n"
    "step resolve 102 0 100 2 (subst) (clause (pos (f 55)) (pos (f 48)) (neg (f 54)))\n"
    "step resolve 101 0 19 0 (subst) (clause (neg (f 55)) (pos (f 22)))\n"
    "component (clause (pos (f 22)))\n"
    "complement 12\n"
    "step resolve 106 0 104 1 (subst) (clause (pos (f 56)) (neg (f 55)))\n"
    "step resolve 105 0 18 0 (subst) (clause (neg (f 56)) (neg (f 3 (f 4) (f 5))) (ne (f 4) (f 0 (f 5) (f 1 (f 5)))))\n"
    "component (clause (pos (f 3 (f 4) (f 5))))\n"
    "complement 13\n"
    "step resolve 109 0 108 1 (subst) (clause (neg (f 57)) (neg (f 56)) (ne (f 4) (f 0 (f 5) (f 1 (f 5)))))\n"
    "component (clause (eq (f 4) (f 0 (f 5) (f 1 (f 5)))))\n"
    "complement 14\n"
    "step resolve 112 0 111 2 (subst) (clause (neg (f 58)) (neg (f 57)) (neg (f 56)))\n"
    "component (clause (pos (f 3 (f 12) (f 4))))\n"
    "complement 15\n"
    "step resolve 115 0 11 1 (subst) (clause (neg (f 59)) (neg (f 17 (v 0))))\n"
    "component (clause (neg (f 17 (v 0))))\n"
    "step split 117 (groups (16 0 1)) (clause (pos (f 60)) (neg (f 59)))\n"
    "step resolve 64 0 5 0 (subst) (clause (neg (f 44)) (pos (f 10)))\n"
    "component (clause (pos (f 10)))\n"
    "complement 17\n"
    "step resolve 122 0 120 1 (subst) (clause (pos (f 61)) (neg (f 44)))\n"
    "step resolve 121 0 3 0 (subst) (clause (neg (f 61)) (pos (f 9)))\n"
    "component (clause (pos (f 9)))\n"
    "complement 18\n"
    "step resolve 126 0 124 1 (subst) (clause (pos (f 62)) (neg (f 61)))\n"
    "step resolve 121 0 2 0 (subst) (clause (neg (f 61)) (pos (f 8)))\n"
    "component (clause (pos (f 8)))\n"
    "complement 19\n"
    "step resolve 130 0 128 1 (subst) (clause (pos (f 63)) (neg (f 61)))\n"
    "step resolve 125 0 1 0 (subst) (clause (neg (f 62)) (pos (f 3 (f 4) (f 7))) (pos (f 3 (f 4) (f 4))))\n"
    "component (clause (pos (f 3 (f 4) (f 7))))\n"
    "complement 20\n"
    "step resolve 134 0 132 1 (subst) (clause (pos (f 64)) (neg (f 62)) (pos (f 3 (f 4) (f 4))))\n"
    "step resolve 76 0 135 2 (subst) (clause (pos (f 48)) (pos (f 64)) (neg (f 62)))\n"
    "step resolve 129 0 0 0 (subst) (clause (neg (f 63)) (neg (f 3 (f 4) (f 7))) (pos (f 3 (f 0 (f 7) (f 1 (f 5))) (f 4))))\n"
    "step resolve 133 0 137 1 (subst) (clause (neg (f 64)) (neg (f 63)) (pos (f 3 (f 0 (f 7) (f 1 (f 5))) (f 4))))\n"
    "component (clause (pos (f 3 (f 0 (f 7) (f 1 (f 5))) (f 4))))\n"
    "complement 21\n"
    "step resolve 140 0 138 2 (subst) (clause (pos (f 65)) (neg (f 64)) (neg (f 63)))\n"
    "step resolve 92 0 61 0 (subst) (clause (neg (f 53)))\n"
    "step resolve 110 0 62 0 (subst) (clause (pos (f 57)))\n"
    "step resolve 82 0 63 0 (subst) (clause (neg (f 50)))\n"
    "step resolve 29 0 67 0 (subst (0 (v 64))) (clause (pos (f 29 (v 0))) (neg (f 45)))\n"
    "step resolve 145 0 72 0 (subst (0 (v 64))) (clause (neg (f 45)) (pos (f 17 (v 0))) (neg (f 47)))\n"
    "step resolve 12 0 146 1 (subst (0 (v 64))) (clause (pos (f 16 (v 0))) (neg (f 45)) (neg (f 47)))\n"
    "step resolve 147 0 10 0 (subst (0 (v 64))) (clause (neg (f 45)) (neg (f 47)) (pos (f 3 (v 0) (f 4))) (pos (f 15 (v 0))))\n"
    "step resolve 83 0 148 2 (subst (64 (f 5))) (clause (pos (f 50)) (neg (f 45)) (neg (f 47)) (pos (f 15 (f 5))))\n"
    "step super 32 0 0 30 0 0 0 (subst (0 (v 64)) (65 (f 4))) (clause (eq (v 0) (f 0 (f 4) (v 0))))\n"
    "step resolve 8 0 149 3 (subst (0 (f 5))) (clause (pos (f 13 (f 5))) (pos (f 50)) (neg (f 45)) (neg (f 47)))\n"
    "step resolve 53 0 52 0 (subst (0 (v 64)) (1 (v 65))) (clause (pos (f 3 (v 0) (v 1))) (pos (f 38 (v 0) (v 1))))\n"
    "step resolve 51 0 152 1 (subst (0 (v 64)) (1 (v 65))) (clause (eq (v 0) (v 1)) (pos (f 3 (v 1) (v 0))) (pos (f 3 (v 0) (v 1))))\n"
    "step resolve 116 0 153 1 (subst (64 (f 4)) (65 (f 12))) (clause (pos (f 59)) (eq (f 4) (f 12)) (pos (f 3 (f 4) (f 12))))\n"
    "component (clause (eq (f 4) (f 12)))\n"
    "complement 22\n"
    "step resolve 156 0 154 1 (subst) (clause (pos (f 66)) (pos (f 59)) (pos (f 3 (f 4) (f 12))))\n"
    "step resolve 109 0 6 1 (subst (64 (f 5))) (clause (neg (f 57)) (neg (f 13 (f 5))) (ne (f 0 (f 5) (f 1 (f 5))) (f 12)))\n"
    "step resolve 151 0 158 1 (subst) (clause (pos (f 50)) (neg (f 45)) (neg (f 47)) (neg (f 57)) (ne (f 0 (f 5) (f 1 (f 5))) (f 12)))\n"
    "step super 33 0 0 159 4 0 0 (subst (0 (f 5))) (clause (pos (f 50)) (neg (f 45)) (neg (f 47)) (neg (f 57)) (ne (f 4) (f 12)))\n"
    "step super 33 0 0 31 0 0 1 (subst (0 (v 64)) (65 (f 1 (v 64)))) (clause (eq (f 0 (f 4) (v 0)) (f 0 (v 1) (f 0 (f 1 (v 1)) (v 0)))))\n"
    "step super 150 0 1 161 0 0 0 (subst (0 (v 64))) (clause (eq (v 0) (f 0 (v 1) (f 0 (f 1 (v 1)) (v 0)))))\n"
    "step super 33 0 0 162 0 1 2 (subst (0 (f 1 (v 65))) (64 (f 1 (f 1 (v 65))))) (clause (eq (f 1 (f 1 (v 0))) (f 0 (v 0) (f 4))))\n"
    "step super 32 0 0 163 0 1 0 (subst (0 (v 64))) (clause (eq (f 1 (f 1 (v 0))) (v 0)))\n"
    "step super 164 0 0 162 0 1 4 (subst (65 (f 1 (v 0)))) (clause (eq (v 0) (f 0 (f 1 (v 1)) (f 0 (v 1) (v 0)))))\n"
    "step resolve 50 0 49 0 (subst (0 (v 64)) (1 (v 65)) (2 (v 66))) (clause (pos (f 36 (v 0) (v 1) (v 2))) (pos (f 3 (v 0) (v 2))))\n"
    "step resolve 48 0 166 0 (subst (0 (v 64)) (1 (v 65)) (2 (v 66))) (clause (neg (f 3 (v 0) (v 1))) (neg (f 3 (v 1) (v 2))) (pos (f 3 (v 0) (v 2))))\n"
    "step resolve 60 0 59 0 (subst (0 (v 64)) (1 (v 65))) (clause (neg (f 3 (v 0) (v 1))) (neg (f 3 (v 1) (f 0 (v 0) (f 5)))))\n"
    "step super 30 0 0 168 1 0 2 (subst (0 (v 64)) (1 (f 5))) (clause (neg (f 3 (v 0) (v 1))) (neg (f 3 (v 1) (f 0 (f 5) (v 0)))))\n"
    "step super 33 0 0 169 1 0 2 (subst (0 (f 5)) (64 (f 1 (f 5)))) (clause (neg (f 3 (f 1 (f 5)) (v 0))) (neg (f 3 (v 0) (f 4))))\n"
    "step resolve 109 0 167 0 (subst (64 (f 4)) (65 (f 5))) (clause (neg (f 57)) (neg (f 3 (f 5) (v 0))) (pos (f 3 (f 4) (v 0))))\n"
    "step resolve 55 0 54 0 (subst (0 (v 64)) (1 (v 65)) (2 (v 66))) (clause (neg (f 3 (v 0) (v 1))) (pos (f 3 (f 0 (v 0) (v 2)) (f 0 (v 1) (v 2)))))\n"
    "step resolve 109 0 172 0 (subst (64 (f 4)) (65 (f 5))) (clause (neg (f 57)) (pos (f 3 (f 0 (f 4) (v 0)) (f 0 (f 5) (v 0)))))\n"
    "step super 150 0 1 173 1 0 1 (subst (0 (v 64))) (clause (neg (f 57)) (pos (f 3 (v 0) (f 0 (f 5) (v 0)))))\n"
    "step super 33 0 0 174 1 0 3 (subst (0 (f 5)) (64 (f 1 (f 5)))) (clause (neg (f 57)) (pos (f 3 (f 1 (f 5)) (f 4))))\n"
    "step resolve 167 0 175 1 (subst (0 (f 1 (f 5))) (1 (f 4))) (clause (neg (f 3 (f 4) (v 0))) (pos (f 3 (f 1 (f 5)) (v 0))) (neg (f 57)))\n"
    "step resolve 146 1 118 0 (subst (0 (v 64))) (clause (neg (f 45)) (neg (f 47)) (neg (f 60)))\n"
    "step resolve 155 0 160 4 (subst) (clause (neg (f 66)) (pos (f 50)) (neg (f 45)) (neg (f 47)) (neg (f 57)))\n"
    "step resolve 172 0 133 0 (subst (0 (f 4)) (1 (f 7))) (clause (pos (f 3 (f 0 (f 4) (v 0)) (f 0 (f 7) (v 0)))) (neg (f 64)))\n"
    "step super 150 0 1 179 0 0 1 (subst (0 (v 64))) (clause (pos (f 3 (v 0) (f 0 (f 7) (v 0)))) (neg (f 64)))\n"
    "step resolve 170 0 180 0 (subst (0 (f 0 (f 7) (f 1 (f 5)))) (64 (f 1 (f 5)))) (clause (neg (f 3 (f 0 (f 7) (f 1 (f 5))) (f 4))) (neg (f 64)))\n"
    "step resolve 139 0 181 0 (subst) (clause (neg (f 65)) (neg (f 64)))\n"
    "step resolve 47 0 75 0 (subst (0 (f 4))) (clause (neg (f 48)))\n"
    "step resolve 33 0 113 0 (subst (0 (f 5))) (clause (pos (f 58)))\n"
    "component (clause (pos (f 3 (f 4) (f 12))))\n"
    "complement 23\n"
    "step resolve 186 0 157 2 (subst) (clause (pos (f 67)) (pos (f 66)) (pos (f 59)))\n"
    "step resolve 147 0 10 0 (subst (0 (v 64))) (clause (neg (f 45)) (neg (f 47)) (pos (f 3 (v 0) (f 4))) (pos (f 15 (v 0))))\n"
    "step resolve 172 0 185 0 (subst (0 (f 4)) (1 (f 12))) (clause (pos (f 3 (f 0 (f 4) (v 0)) (f 0 (f 12) (v 0)))) (neg (f 67)))\n"
    "step super 150 0 1 189 0 0 1 (subst (0 (v 64))) (clause (pos (f 3 (v 0) (f 0 (f 12) (v 0)))) (neg (f 67)))\n"
    "step resolve 171 1 190 0 (subst (0 (f 0 (f 12) (f 5))) (64 (f 5))) (clause (neg (f 57)) (pos (f 3 (f 4) (f 0 (f 12) (f 5)))) (neg (f 67)))\n"
    "step super 30 0 0 191 1 0 2 (subst (0 (f 12)) (1 (f 5))) (clause (neg (f 57)) (pos (f 3 (f 4) (f 0 (f 5) (f 12)))) (neg (f 67)))\n"
    "step resolve 6 1 192 1 (subst (0 (f 0 (f 5) (f 12)))) (clause (neg (f 13 (f 0 (f 5) (f 12)))) (ne (f 0 (f 0 (f 5) (f 12)) (f 1 (f 5))) (f 12)) (neg (f 57)) (neg (f 67)))\n"
    "step super 30 0 0 193 1 0 0 (subst (0 (f 0 (f 5) (f 12))) (1 (f 1 (f 5)))) (clause (neg (f 13 (f 0 (f 5) (f 12)))) (ne (f 0 (f 1 (f 5)) (f 0 (f 5) (f 12))) (f 12)) (neg (f 57)) (neg (f 67)))\n"
    "step resolve 165 0 194 1 (subst (0 (f 12)) (1 (f 5))) (clause (neg (f 13 (f 0 (f 5) (f 12)))) (neg (f 57)) (neg (f 67)))\n"
    "step resolve 192 1 176 0 (subst (64 (f 0 (f 5) (f 12)))) (clause (neg (f 57)) (neg (f 67)) (pos (f 3 (f 1 (f 5)) (f 0 (f 5) (f 12)))))\n"
    "step resolve 170 0 196 2 (subst (0 (f 0 (f 5) (f 12)))) (clause (neg (f 3 (f 0 (f 5) (f 12)) (f 4))) (neg (f 57)) (neg (f 67)))\n"
    "step resolve 188 2 197 0 (subst (0 (f 0 (f 5) (f 12)))) (clause (neg (f 45)) (neg (f 47)) (pos (f 15 (f 0 (f 5) (f 12)))) (neg (f 57)) (neg (f 67)))\n"
    "step resolve 8 0 198 2 (subst (0 (f 0 (f 5) (f 12)))) (clause (pos (f 13 (f 0 (f 5) (f 12)))) (neg (f 45)) (neg (f 47)) (neg (f 57)) (neg (f 67)))\n"
    "step resolve 195 0 199 0 (subst) (clause (neg (f 57)) (neg (f 67)) (neg (f 45)) (neg (f 47)))\n"
    "step resolve 187 2 119 1 (subst) (clause (pos (f 67)) (pos (f 66)) (pos (f 60)))\n"
    "step resolve 201 2 177 2 (subst) (clause (pos (f 67)) (pos (f 66)) (neg (f 45)) (neg (f 47)))\n"
    "step resolve 202 1 178 0 (subst) (clause (pos (f 67)) (neg (f 45)) (neg (f 47)) (pos (f 50)) (neg (f 57)))\n"
    "step resolve 203 0 200 1 (subst) (clause (neg (f 45)) (neg (f 47)) (pos (f 50)) (neg (f 57)))\n"
    "step resolve 131 0 141 2 (subst) (clause (neg (f 61)) (pos (f 65)) (neg (f 64)))\n"
    "step resolve 205 1 182 0 (subst) (clause (neg (f 61)) (neg (f 64)))\n"
    "step resolve 136 1 206 1 (subst) (clause (pos (f 48)) (neg (f 62)) (neg (f 61)))\n"
    "step resolve 127 0 207 1 (subst) (clause (neg (f 61)) (pos (f 48)))\n"
    "step resolve 123 0 208 0 (subst) (clause (neg (f 44)) (pos (f 48)))\n"
    "step resolve 68 1 209 0 (subst) (clause (pos (f 45)) (pos (f 48)))\n"
    "step resolve 210 0 204 0 (subst) (clause (pos (f 48)) (neg (f 47)) (pos (f 50)) (neg (f 57)))\n"
    "step resolve 73 0 211 1 (subst) (clause (pos (f 46)) (pos (f 48)) (pos (f 50)) (neg (f 57)))\n"
    "step resolve 212 0 80 2 (subst) (clause (pos (f 48)) (pos (f 50)) (neg (f 57)) (pos (f 49)))\n"
    "step resolve 213 3 87 2 (subst) (clause (pos (f 48)) (pos (f 50)) (neg (f 57)) (pos (f 51)))\n"
    "step resolve 214 3 94 2 (subst) (clause (pos (f 48)) (pos (f 50)) (neg (f 57)) (pos (f 53)) (pos (f 52)))\n"
    "step resolve 215 4 98 1 (subst) (clause (pos (f 48)) (pos (f 50)) (neg (f 57)) (pos (f 53)) (pos (f 54)))\n"
    "step resolve 216 4 103 2 (subst) (clause (pos (f 48)) (pos (f 50)) (neg (f 57)) (pos (f 53)) (pos (f 55)))\n"
    "step resolve 217 4 107 1 (subst) (clause (pos (f 48)) (pos (f 50)) (neg (f 57)) (pos (f 53)) (pos (f 56)))\n"
    "step resolve 218 4 114 2 (subst) (clause (pos (f 48)) (pos (f 50)) (neg (f 57)) (pos (f 53)) (neg (f 58)))\n"
    "step resolve 184 0 219 4 (subst) (clause (pos (f 48)) (pos (f 50)) (neg (f 57)) (pos (f 53)))\n"
    "step resolve 220 0 183 0 (subst) (clause (pos (f 50)) (neg (f 57)) (pos (f 53)))\n"
    "step resolve 221 2 142 0 (subst) (clause (pos (f 50)) (neg (f 57)))\n"
    "step resolve 222 0 144 0 (subst) (clause (neg (f 57)))\n"
    "step resolve 143 0 223 0 (subst) (clause)\n"
    "end\n"
    ;

static void test_certificates(void) {
    char text[1 << 17];
    expect(answer_of(INJECTIVE) == CK_ACCEPTED, "il controesempio x + 1 è accettato");
    expect(answer_of(SURJECTIVE) == CK_ACCEPTED, "il controesempio per casi è accettato");
    expect(answer_of(replaced(INJECTIVE, "define f (add (var 0) 1)", "define f (var 0)")) != CK_ACCEPTED,
           "con f l'identità, dove l'enunciato è vero, non è accettato");
    expect(answer_of(replaced(INJECTIVE, "define f (add (var 0) 1)", "define f (sub (var 0) 1)")) != CK_ACCEPTED,
           "con f(x) = x - 1, che manda 0 fuori da S, non è accettato");
    expect(answer_of(replaced(SURJECTIVE, "witness 4 1", "witness 4 0")) != CK_ACCEPTED,
           "con due testimoni uguali non è accettato");

    const char *statement = strstr(INJECTIVE, "statement ") + strlen("statement ");
    const char *line_end = strchr(statement, '\n');
    snprintf(text, sizeof text, "%.*s(not %.*s)%s", (int)(statement - INJECTIVE), INJECTIVE,
             (int)(line_end - statement), statement, line_end);
    expect(answer_of(text) != CK_ACCEPTED, "con l'enunciato negato non è accettato");

    snprintf(text, sizeof text, "%s", INJECTIVE);
    char *last = strstr(text, "\nend\n"), *previous = last;
    while (previous > text && previous[-1] != '\n') previous--;
    memmove(previous, last + 1, strlen(last + 1) + 1);
    expect(answer_of(text) != CK_ACCEPTED, "senza l'ultimo passo non è accettato");
}

/* Il motivo per cui il verificatore ferma un certificato infinite letto fino
 * a end, o "" se tutto ciò che c'era era giusto (manca solo la
 * refutazione). */
static const char *reason_of(const char *text) {
    const char *cursor = text, *why = "";
    CkAnswer answer = ck_check(&cursor, &why, 0);
    if (answer == CK_ACCEPTED) return "accettato";
    return strcmp(why, "la refutazione non termina con la clausola vuota") == 0 ? "" : why;
}

#define HEAD "certificate infinite\nsort S\nsymbol f S S\nsymbol P S bool\n" \
             "statement (forall x S (exists y S (= (f y) x)))\ninterpret S (set ge 0)\n"

static void test_rules(void) {
    expect(!*reason_of(HEAD "define f (add (var 0) 1)\ndefine P (= (var 0) 0)\nclausify\nend\n"), "un certificato senza passi si legge fino a end");
    /* Il quantificatore 0 è la chiusura di f, un per ogni in positivo; il 1 è il per ogni dell'enunciato, negato; il 2 l'esiste, negato. */
    expect(!*reason_of(HEAD "define f (add (var 0) 1)\ndefine P (= (var 0) 0)\nwitness 1 0\nclausify\nend\n"), "un testimone per un per ogni negato");
    expect(*reason_of(HEAD "define f (add (var 0) 1)\ndefine P (= (var 0) 0)\nwitness 0 0\nclausify\nend\n") != 0, "un testimone per un per ogni in positivo è respinto");
    expect(*reason_of(HEAD "define f (add (var 0) 1)\ndefine P (= (var 0) 0)\nwitness 2 0\nclausify\nend\n") != 0, "un testimone per un esiste negato è respinto");
    expect(*reason_of(HEAD "define f (add (var 0) 1)\ndefine P (= (var 0) 0)\nwitness 7 0\nclausify\nend\n") != 0, "un testimone per un quantificatore che non c'è è respinto");
    expect(*reason_of(HEAD "define f (add (var 0) (var 1))\ndefine P (= (var 0) 0)\nclausify\nend\n") != 0, "una definizione con una lettera in più è respinta");
    expect(*reason_of(HEAD "define f (add (var 0) 1)\ndefine P (exists (var 5) (set) (= (var 5) (var 0)))\nclausify\nend\n") != 0, "una definizione con un quantificatore è respinta");
    expect(*reason_of(HEAD "define f (add (var 0) 1)\nclausify\nend\n") != 0, "un simbolo senza define è respinto");
    expect(*reason_of("certificate infinite\nsort S\nsort T\nsymbol f S T\nstatement (forall x S (= (f x) (f x)))\ninterpret S (set)\ndefine f 0\nclausify\nend\n") != 0, "un insieme senza interpret è respinto");
    expect(*reason_of("certificate arithmetic\nkind int\nstatement (= (cases (less 0 1) 1 0) 1)\nclausify\nend\n") != 0, "cases fuori da define è respinto");
}

/* L'oracolo. Un enunciato di 11 su S, con f: S -> S, c in S, P parte e R
 * relazione, come albero di questo file; l'interpretazione in C. */
typedef enum { T_VAR, T_C, T_F, C_EQ, C_P, C_R, C_NOT, C_AND, C_OR, C_IMPLIES, C_IFF, C_FORALL, C_EXISTS, C_TRUE } TKind;
typedef struct TNode { TKind kind; int depth; const struct TNode *a, *b; } TNode;   /* depth: per T_VAR, la profondità del quantificatore */
static TNode tpool[1 << 15];
static int tpooled;
static unsigned rng = 2026;
static int pick(int n) { rng = rng * 1103515245u + 12345u; return (int)((rng >> 16) % (unsigned)n); }
static TNode *tnode(TKind kind) { TNode *n = &tpool[tpooled++]; memset(n, 0, sizeof *n); n->kind = kind; return n; }

static const TNode *gen_term(int bound, int depth) {
    int k = pick(depth > 1 ? 2 : 3);
    if (k == 0 && bound > 0) { TNode *n = tnode(T_VAR); n->depth = pick(bound); return n; }
    if (k <= 1) return tnode(T_C);
    TNode *n = tnode(T_F); n->a = gen_term(bound, depth + 1); return n;
}
static const TNode *gen_cond(int bound, int depth) {
    int k = pick(depth < 3 ? 9 : 3);
    TNode *n;
    switch (k) {
    case 0: n = tnode(C_EQ); n->a = gen_term(bound, 0); n->b = gen_term(bound, 0); return n;
    case 1: n = tnode(C_P); n->a = gen_term(bound, 0); return n;
    case 2: n = tnode(C_R); n->a = gen_term(bound, 0); n->b = gen_term(bound, 0); return n;
    case 3: n = tnode(C_NOT); n->a = gen_cond(bound, depth + 1); return n;
    case 4: case 5: { static const TKind ks[] = {C_AND, C_OR, C_IMPLIES, C_IFF}; n = tnode(ks[pick(4)]); n->a = gen_cond(bound, depth + 1); n->b = gen_cond(bound, depth + 1); return n; }
    default: n = tnode(pick(2) ? C_FORALL : C_EXISTS); n->a = gen_cond(bound + 1, depth + 1); return n;
    }
}

/* Le interpretazioni: il testo della voce, e lo stesso in C. */
typedef struct { const char *text; long long (*value)(long long); } FDef;
static long long f_succ(long long x) { return x + 1; }
static long long f_pred(long long x) { return x > 0 ? x - 1 : 0; }
static long long f_double(long long x) { return 2 * x; }
static long long f_fold(long long x) { return x < 2 ? x + 2 : x - 2; }
static long long f_square(long long x) { return x * x - 1; }
static const FDef FS[] = {
    {"(add (var 0) 1)", f_succ}, {"(cases (less 0 (var 0)) (sub (var 0) 1) 0)", f_pred}, {"(mul 2 (var 0))", f_double},
    {"(cases (less (var 0) 2) (add (var 0) 2) (sub (var 0) 2))", f_fold}, {"(sub (pow (var 0) 2) 1)", f_square}};
typedef struct { const char *text; int (*holds)(long long); } PDef;
static int p_zero(long long x) { return x == 0; }
static int p_positive(long long x) { return x > 0; }
static int p_even(long long x) { return x % 2 == 0; }
static const PDef PS[] = {{"(= (var 0) 0)", p_zero}, {"(less 0 (var 0))", p_positive}, {"(divides 2 (var 0))", p_even}};
typedef struct { const char *text; int (*holds)(long long, long long); } RDef;
static int r_less(long long x, long long y) { return x < y; }
static int r_sum(long long x, long long y) { return x + y == 2; }
static int r_cases(long long x, long long y) { return y == (x > 0 ? x : 0); }
static const RDef RS[] = {{"(less (var 0) (var 1))", r_less}, {"(= (add (var 0) (var 1)) 2)", r_sum},
                          {"(= (cases (less 0 (var 0)) (var 0) 0) (var 1))", NULL}, {"(lesseq (var 0) (var 1))", NULL}};
typedef struct { const char *text; long long low; int bounded; long long hole; int holed; } SDef;
static const SDef SETS[] = {{"(set ge 0)", 0, 1, 0, 0}, {"(set)", 0, 0, 0, 0}, {"(set ge -1 except 1)", -1, 1, 1, 1}};
static const long long CS[] = {0, 1, -2};

static const FDef *f_now; static const PDef *p_now; static int r_now; static const SDef *s_now; static long long c_now;
static int in_s(long long x) { return (!s_now->bounded || x >= s_now->low) && !(s_now->holed && x == s_now->hole); }
static int r_holds(long long x, long long y) { return r_now == 3 ? x <= y : r_now == 2 ? r_cases(x, y) : RS[r_now].holds(x, y); }

#define WINDOW 4
static long long env[64];
static long long t_value(const TNode *t, int bound) {
    if (t->kind == T_VAR) return env[t->depth];
    if (t->kind == T_C) return c_now;
    return f_now->value(t_value(t->a, bound));
    (void)bound;
}
static int t_holds(const TNode *c, int bound) {
    switch (c->kind) {
    case C_EQ: return t_value(c->a, bound) == t_value(c->b, bound);
    case C_P: return p_now->holds(t_value(c->a, bound));
    case C_R: return r_holds(t_value(c->a, bound), t_value(c->b, bound));
    case C_NOT: return !t_holds(c->a, bound);
    case C_AND: return t_holds(c->a, bound) && t_holds(c->b, bound);
    case C_OR: return t_holds(c->a, bound) || t_holds(c->b, bound);
    case C_IMPLIES: return !t_holds(c->a, bound) || t_holds(c->b, bound);
    case C_IFF: return t_holds(c->a, bound) == t_holds(c->b, bound);
    case C_TRUE: return 1;
    default: {
        int every = c->kind == C_FORALL;
        for (long long x = -WINDOW; x <= WINDOW; x++) {
            if (!in_s(x)) continue;
            env[bound] = x;
            if (t_holds(c->a, bound + 1) != every) return !every;
        }
        return every;
    }
    }
}

/* L'enunciato come testo di 11: la variabile a profondità k si chiama vk. */
static char out[1 << 16];
static size_t wrote;
static void put(const char *s) { size_t n = strlen(s); if (wrote + n < sizeof out) { memcpy(out + wrote, s, n); wrote += n; } }
static void put_term(const TNode *t) {
    char b[16];
    if (t->kind == T_VAR) { snprintf(b, sizeof b, "v%d", t->depth); put(b); return; }
    if (t->kind == T_C) { put("c"); return; }
    put("(f "); put_term(t->a); put(")");
}
static void put_cond(const TNode *c, int bound) {
    static const char *const words[] = {[C_NOT] = "not", [C_AND] = "and", [C_OR] = "or", [C_IMPLIES] = "implies", [C_IFF] = "iff"};
    char b[32];
    switch (c->kind) {
    case C_EQ: put("(= "); put_term(c->a); put(" "); put_term(c->b); put(")"); return;
    case C_P: put("(P "); put_term(c->a); put(")"); return;
    case C_R: put("(R "); put_term(c->a); put(" "); put_term(c->b); put(")"); return;
    case C_TRUE: put("true"); return;
    case C_FORALL: case C_EXISTS:
        snprintf(b, sizeof b, "(%s v%d S ", c->kind == C_FORALL ? "forall" : "exists", bound); put(b);
        put_cond(c->a, bound + 1); put(")"); return;
    default:
        put("("); put(words[c->kind]); put(" "); put_cond(c->a, bound);
        if (c->kind != C_NOT) { put(" "); put_cond(c->b, bound); }
        put(")");
    }
}

/* Il valore di un'affermazione di 20g, con i quantificatori ristretti alla finestra. */
static long long nl[64], nv[64];
static int nn;
static int n_member(long long x, const CkNSet *set) {
    if (set->bounded && (set->strict ? x <= set->low : x < set->low)) return 0;
    for (int i = 0; i < set->holes; i++) if (set->hole[i].den == 1 && x == (long long)set->hole[i].num) return 0;
    return 1;
}
static int n_holds(const CkNNode *n);
static long long n_value(const CkNNode *t) {
    long long x, y, r;
    switch (t->kind) {
    case CK_N_NUMBER: return t->value;
    case CK_N_VAR: for (int i = nn - 1; i >= 0; i--) if (nl[i] == t->value) return nv[i]; failures++; printf("lettera libera\n"); return 0;
    case CK_N_ADD: return n_value(t->a) + n_value(t->b);
    case CK_N_SUB: return n_value(t->a) - n_value(t->b);
    case CK_N_MUL: return n_value(t->a) * n_value(t->b);
    case CK_N_NEG: return -n_value(t->a);
    case CK_N_POW: x = n_value(t->a); r = 1; for (long long i = 0; i < t->value; i++) r *= x; return r;
    case CK_N_CASES: return n_holds(t->c) ? n_value(t->a) : n_value(t->b);
    default: failures++; printf("espressione inattesa %d\n", t->kind); (void)y; return 0;
    }
}
static int n_holds(const CkNNode *n) {
    long long x, y;
    switch (n->kind) {
    case CK_N_EQ: return n_value(n->a) == n_value(n->b);
    case CK_N_NEQ: return n_value(n->a) != n_value(n->b);
    case CK_N_LESS: return n_value(n->a) < n_value(n->b);
    case CK_N_LESSEQ: return n_value(n->a) <= n_value(n->b);
    case CK_N_IN: return n_member(n_value(n->a), &n->set);
    case CK_N_DIVIDES: x = n_value(n->a); y = n_value(n->b); return x == 0 ? y == 0 : y % x == 0;
    case CK_N_NOT: return !n_holds(n->a);
    case CK_N_AND: return n_holds(n->a) && n_holds(n->b);
    case CK_N_OR: return n_holds(n->a) || n_holds(n->b);
    case CK_N_IMPLIES: return !n_holds(n->a) || n_holds(n->b);
    case CK_N_IFF: return n_holds(n->a) == n_holds(n->b);
    case CK_N_FORALL: case CK_N_EXISTS: {
        int every = n->kind == CK_N_FORALL;
        for (long long v = -WINDOW; v <= WINDOW; v++) {
            if (!n_member(v, &n->set)) continue;
            nl[nn] = n->value; nv[nn++] = v;
            int h = n_holds(n->a);
            nn--;
            if (h != every) return !every;
        }
        return every;
    }
    default: failures++; printf("condizione inattesa %d\n", n->kind); return 0;
    }
}

/* L'affermazione del verificatore per l'enunciato e l'interpretazione
 * correnti, con il testimone witness (o nessuno se witness è NULL); NULL
 * se la respinge. */
static const CkNNode *claim_of(const char *witness) {
    char text[1 << 17];
    char c[8];
    snprintf(c, sizeof c, c_now < 0 ? "(neg %lld)" : "%lld", c_now < 0 ? -c_now : c_now);
    snprintf(text, sizeof text,
             "certificate infinite\nsort S\nsymbol f S S\nsymbol c S\nsymbol P S bool\nsymbol R S S bool\nstatement %.*s\n"
             "interpret S %s\ndefine f %s\ndefine c %s\ndefine P %s\ndefine R %s\n%s%sclausify\nend\n",
             (int)wrote, out, s_now->text, f_now->text, c, p_now->text, RS[r_now].text,
             witness ? witness : "", witness ? "\n" : "");
    ck_begin(text, 0);
    ck_word("certificate");
    ck_word("infinite");
    ck_g_read();
    return ck_status == CK_ACCEPTED ? ck_g_claim : NULL;
}

static void test_claims(void) {
    int agreed = 0, values[2] = {0, 0}, witnessed = 0;
    for (int round = 0; round < 3000 && failures == 0; round++) {
        tpooled = 0;
        const TNode *statement = gen_cond(0, 0);
        wrote = 0;
        put_cond(statement, 0);
        f_now = &FS[pick(5)]; p_now = &PS[pick(3)]; r_now = pick(4); s_now = &SETS[pick(3)]; c_now = CS[pick(3)];

        /* Le chiusure sulla finestra: f manda S in S, c sta in S. */
        int closed = in_s(c_now);
        for (long long x = -WINDOW; x <= WINDOW && closed; x++) if (in_s(x) && !in_s(f_now->value(x))) closed = 0;
        int expected = closed && !t_holds(statement, 0);

        const CkNNode *claim = claim_of(NULL);
        if (!claim) { printf("FALLITO: affermazione non scritta (%s): %.*s\n", ck_reason, (int)wrote, out); failures++; ck_finish(); break; }
        nn = 0;
        int got = n_holds(claim);
        ck_finish();
        values[expected]++;
        if (got != expected) { printf("FALLITO: l'affermazione vale %d, l'oracolo dice %d: %.*s con f %s\n", got, expected, (int)wrote, out, f_now->text); failures++; break; }
        agreed++;

        /* Un testimone a caso, nella finestra: se il verificatore lo prende,
         * l'affermazione con il testimone implica quella senza. */
        char witness[32];
        snprintf(witness, sizeof witness, "witness %d %d", pick(6), pick(2 * WINDOW + 1) - WINDOW);
        const CkNNode *strong = claim_of(witness);
        if (strong) {
            nn = 0;
            if (n_holds(strong) && !expected) { printf("FALLITO: con %s l'affermazione falsa diventa vera: %.*s\n", witness, (int)wrote, out); failures++; }
            witnessed++;
        }
        ck_finish();
    }
    expect(values[0] > 300 && values[1] > 300, "affermazioni vere e false sono tante");
    expect(witnessed > 100, "i testimoni accettati sono tanti");
    printf("affermazioni: %d d'accordo con l'oracolo (%d vere, %d false), %d con un testimone accettato\n", agreed, values[1], values[0], witnessed);
}

int main(void) {
    test_certificates();
    test_rules();
    test_claims();
    if (failures) { printf("%d controlli falliti\n", failures); return 1; }
    printf("test_infinite_models: tutti i controlli riusciti\n");
    return 0;
}
