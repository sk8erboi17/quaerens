/* Test del lettore dei documenti: i commenti LaTeX intorno agli enunciati
 * non diventano condizioni e non introducono elementi dell'elenco.
 * Le condizioni vere sulla riga seguente devono invece restare lette.
 * I commenti dentro una formula restano fuori dalla grammatica matematica:
 * non si deve scambiare una parte della formula per l'enunciato intero.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_tex_comments \
 *         tests/engine/test_tex_comments.c && /tmp/test_tex_comments
 */

#include "../statement_checks.h"

/* Legge e decide un elemento senza richiedere un file temporaneo. Il testo
 * deve essere terminato da zero; il verdetto ha la durata usuale di prove(). */
static Verdict decide_item(const char *text) {
    return prove_item(text, text + strlen(text));
}

int main(void) {
    int passed = 1;
    engine_init();

    /* Le note prima e dopo la formula non nascondono un controesempio. */
    Verdict v = decide_item("% source note\n $\\displaystyle 1 = 2$ % $a = 0$\n");
    passed = check_result(v.outcome == REFUTED, "comments before and after a false statement do not hide its counterexample") && passed;
    v = decide_item("% source note\r\n \\[1 = 1\\] % source note");
    passed = check_result(v.outcome == PROVED && v.certificate, "comments with CRLF and no final newline preserve a certified proof") && passed;

    /* Una nota non cancella quello che segue sull'altra riga. */
    v = decide_item("$\\displaystyle 1 = 2$ % source note\n with $a > 0$");
    passed = check_result(v.outcome == NOT_PROVED, "a condition after a comment is still unread and prevents a refutation") && passed;
    v = decide_item("$\\displaystyle 1 = 1$ % source note\n $0 = 1$");
    passed = check_result(v.outcome == UNSUPPORTED, "a second equality after a comment prevents a partial proof") && passed;
    v = decide_item("$\\displaystyle 1 % $ fake delimiter\n = 2$");
    passed = check_result(v.outcome != PROVED, "a delimiter inside a formula comment never creates a proof of a fragment") && passed;

    /* Il documento contiene solo due elementi reali; gli altri delimitatori
     * stanno nelle note, nei comandi piu lunghi o dopo una barra protetta. */
    const char *document =
        "% \\item $\\displaystyle 0 = 1$\n"
        "\\begin{enumerate}\n\\itemize \\\\item\n"
        "\\item % source note\n $\\displaystyle 1 = 1$\n"
        "% \\item $\\displaystyle 0 = 1$ \\end{enumerate}\n"
        "\\item $\\displaystyle 1 = 2$ % source note\n"
        "\\end{enumerate}\n";
    const char *position = document, *item, *end;
    int count = 0;
    while (next_item(&position, &item, &end)) {
        v = prove_item(item, end);
        count++;
        passed = check_result(v.outcome == (count == 1 ? PROVED : REFUTED), "only real list items are decided, with their complete statements") && passed;
    }
    passed = check_result(count == 2, "comments, escaped backslashes and longer commands do not create list items") && passed;

    /* Una percentuale protetta non apre un commento; dopo due barre invece
     * la percentuale e libera, come dopo un a capo esplicito di LaTeX. */
    const char *escaped = "\\% \\item";
    passed = check_result(tex_find(escaped, escaped + strlen(escaped), "\\item") == escaped + 3,
                          "an escaped percent sign does not hide the next command") && passed;
    const char *linebreak = "\\\\% \\item\n\\item";
    passed = check_result(tex_find(linebreak, linebreak + strlen(linebreak), "\\item") == linebreak + 10,
                          "a percent sign after an escaped backslash starts a comment") && passed;
    passed = check_result(unjustified_steps == 0, "every emitted proof passes its independent checks") && passed;
    return passed ? 0 : 1;
}
