/* Test di dominio: strutture algebriche (operazioni e relazioni date da una
 * regola o su un insieme qualsiasi, congruenze).
 *
 * Dieci controlli, descritti in tests/statement_checks.h:
 *
 *   1. ogni enunciato di operations.tex viene dimostrato, con la sua
 *      dimostrazione scritta;
 *   2. nessun enunciato di false_operations.tex viene dimostrato;
 *   3. ogni enunciato di book_structures.tex viene dimostrato: sono le
 *      affermazioni che gli esercizi di un libro universitario chiedono di
 *      provare, scritte per esteso con i quantificatori;
 *   4. nessun enunciato di book_structures_beyond.tex viene dichiarato
 *      falso: vengono dagli stessi esercizi, ma il motore non li sa ancora
 *      decidere, o non ne sa leggere la notazione;
 *   5. ogni enunciato di witnesses.tex viene dimostrato: ipotesi "esiste
 *      k" usate con il loro testimone, congruenze modulo una lettera
 *      (regola 18u);
 *   6. nessun enunciato di false_witnesses.tex viene dimostrato;
 *   7. ogni enunciato di abstract_structures.tex viene dimostrato: un
 *      insieme qualsiasi con un'operazione, delle applicazioni, una
 *      relazione di cui si conoscono solo le ipotesi (capitolo 28);
 *   8. nessun enunciato di false_abstract_structures.tex viene dimostrato;
 *   9. ogni enunciato di rare_witnesses.tex viene dimostrato: ipotesi
 *      "esiste k" che pochi numeri piccoli soddisfano, o nessuno, come
 *      2x = k^2 + 14; la verifica del capitolo 19 deve trovare valori che
 *      le soddisfano e ricontrollare il valore scelto dalla dimostrazione;
 *  10. nessun enunciato di false_rare_witnesses.tex viene dimostrato: i
 *      loro controesempi stanno dove i primi elementi degli insiemi non
 *      arrivano, e una verifica che provasse solo quelli confermerebbe una
 *      dimostrazione sbagliata.
 *
 * Poi i certificati del capitolo 28 (paragrafo 28f), in questo file perché
 * riguardano questi enunciati; il verificatore da solo è provato in
 * tests/checker/test_structures.c:
 *
 *   9. ogni verdetto del capitolo 28 di abstract_structures.tex,
 *      false_abstract_structures.tex e book_structures.tex, DIMOSTRATO o
 *      FALSO, porta un certificato che il verificatore, rileggendolo,
 *      accetta. Antiregressione: tolte le voci instance e witness, una
 *      dimostrazione non è più accettata, e un controesempio presentato
 *      come esempio (o il contrario) nemmeno;
 *  10. il testo di due certificati, fissato: è ciò che un lettore confronta
 *      con l'enunciato e ciò che il programma checker deve saper rileggere;
 *  11. i controesempi certificati vanno anche nel file dei certificati
 *      (--certificates), ma non si contano tra i dimostrati.
 *
 * Gli enunciati dichiarano un'operazione con i suoi insiemi e la sua regola,
 * e ne affermano le proprietà scritte per esteso: associativa, commutativa,
 * con elemento neutro, regolare. Il motore non conosce queste parole: vede
 * quantificatori e uguaglianze, e la regola al posto dell'operazione. I
 * controlli 2, 6 e 8 sono l'antiregressione: una proprietà affermata
 * dell'operazione sbagliata, la negazione di una proprietà vera, una regola
 * che esce dall'insieme; un'ipotesi tolta, un modulo cambiato; due elementi
 * neutri dallo stesso lato, un enunciato vero solo sugli insiemi finiti o
 * solo su quelli non vuoti. Gli enunciati sono dati: il motore non ne
 * conosce nessuno in anticipo.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_algebraic_structures \
 *         tests/algebraic_structures/test_algebraic_structures.c && /tmp/test_algebraic_structures
 */

#include "../statement_checks.h"

/* Il controllo 9, check_structure_certificates(), sta in
 * tests/statement_checks.h: lo usa anche tests/set_theory/test_set_theory.c. */

/* Controllo 11: i controesempi di path scritti da prove_file() nel file dei
 * certificati, riletti, sono tutti accettati, e sono `expected`; tra i
 * dimostrati non si contano. */
static int check_counterexamples_written(const char *path, int expected) {
    static char written[1 << 20];
    char summary[512];
    Report report;
    FILE *memory = tmpfile();
    if (!memory || !prove_file(path, &report, NULL, NULL, memory)) {
        snprintf(summary, sizeof summary, "cannot prove %s into a temporary file", path);
        return check_result(0, summary);
    }
    rewind(memory);
    size_t length = fread(written, 1, sizeof written - 1, memory);
    fclose(memory);
    written[length] = 0;
    int accepted = 0, others = 0;
    for (const char *cursor = written; (cursor = strstr(cursor, "certificate")) != NULL;) {
        const char *why;
        if (ck_check(&cursor, &why) == CK_ACCEPTED) accepted++; else others++;
    }
    snprintf(summary, sizeof summary, "%d counterexamples written for %s, %d accepted when read back, %d counted as proved",
             accepted + others, path, accepted, report.certified);
    return check_result(accepted == expected && others == 0 && report.certified == 0, summary);
}

int main(void) {
    engine_init();

    /* Tutti i controlli vengono eseguiti anche se uno fallisce, così il
     * resoconto è completo. */
    int passed = check_all_proved("tests/algebraic_structures/operations.tex");
    passed = check_none_proved("tests/algebraic_structures/false_operations.tex") && passed;
    passed = check_all_proved("tests/algebraic_structures/book_structures.tex") && passed;
    passed = check_none_refuted("tests/algebraic_structures/book_structures_beyond.tex") && passed;
    passed = check_all_proved("tests/algebraic_structures/witnesses.tex") && passed;
    passed = check_none_proved("tests/algebraic_structures/false_witnesses.tex") && passed;
    passed = check_all_proved("tests/algebraic_structures/abstract_structures.tex") && passed;
    passed = check_none_proved("tests/algebraic_structures/false_abstract_structures.tex") && passed;
    passed = check_all_proved("tests/algebraic_structures/rare_witnesses.tex") && passed;
    passed = check_none_proved("tests/algebraic_structures/false_rare_witnesses.tex") && passed;

    /* 9. I certificati del capitolo 28. */
    passed = check_structure_certificates("tests/algebraic_structures/abstract_structures.tex", 24) && passed;
    passed = check_structure_certificates("tests/algebraic_structures/false_abstract_structures.tex", 19) && passed;
    passed = check_structure_certificates("tests/algebraic_structures/book_structures.tex", 15) && passed;

    /* 10. Due testi fissati: una dimostrazione, con un caso e due istanze;
     * un controesempio, con le sue tabelle. */
    Verdict neutral = prove("(\\perp \\colon S \\times S \\to S \\land u \\in S \\land v \\in S \\land \\forall x \\in S\\ (u \\perp x = x) \\land "
                            "\\forall x \\in S\\ (x \\perp v = x)) \\implies u = v", 'n', 0, 0);
    const char *proof = "certificate structure\nsort S\nsymbol perp S S S\nsymbol u S\nsymbol v S\n"
                        "statement (implies (and (and (and (and true true) true) (forall x S (= (perp u x) x))) (forall x S (= (perp x v) x))) (= u v))\n"
                        "case (not (= u v))\ninstance (forall x S (= (perp u x) x)) v\ninstance (forall x S (= (perp x v) x)) u\nend\n";
    passed = check_result(neutral.certificate && strcmp(neutral.certificate, proof) == 0,
                          "the certificate that a left and a right neutral element coincide has the expected text") && passed;
    Verdict member = prove("(u \\in S \\land X \\subseteq S) \\implies u \\in X", 'n', 0, 0);
    const char *model = "certificate counterexample\nsort S 1\nsymbol u S\nsymbol X S bool\n"
                        "statement (implies (and true true) (X u))\ntable u 0\ntable X 0\nend\n";
    passed = check_result(member.outcome == REFUTED && member.certificate && strcmp(member.certificate, model) == 0,
                          "the certificate of the counterexample to u in X has the expected text") && passed;

    /* La riga del verdetto dice che il controesempio è certificato. */
    char line[512];
    FILE *memory = tmpfile();
    if (memory) {
        verdict_print(memory, &member);
        rewind(memory);
        line[fread(line, 1, sizeof line - 1, memory)] = 0;
        fclose(memory);
    }
    passed = check_result(memory && strcmp(line, "FALSO: controesempio $S = \\{s_{0}\\}$; $u = s_{0}$; $X = \\{\\} = \\emptyset$; certificato verificato") == 0,
                          "the verdict line of a certified counterexample says so") && passed;

    /* 11. I controesempi nel file dei certificati. */
    /* Tre controesempi in piu usano domini vuoti; tutti vengono scritti
     * e riletti, senza aumentare il numero degli enunciati dimostrati. */
    passed = check_counterexamples_written("tests/algebraic_structures/false_abstract_structures.tex", 19) && passed;

    /* Ogni fatto scritto in una dimostrazione viene ricontrollato dal
     * programma con un metodo indipendente: nessuno deve essere stato
     * scartato. */
    passed = check_result(unjustified_steps == 0, "every step of the written proofs passed its independent check") && passed;
    return passed ? 0 : 1;
}
