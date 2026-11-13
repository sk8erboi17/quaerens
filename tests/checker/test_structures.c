/* Test del verificatore sui certificati degli enunciati su insiemi
 * qualsiasi (capitolo 11 di checker.c), da solo: qui il motore non c'entra.
 *
 * Compilazione ed esecuzione, dalla cartella principale del progetto:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/checker checker.c && \
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_structures tests/checker/test_structures.c && /tmp/test_structures
 *
 * Come in test_checker.c, il primo comando fa parte del test: checker.c si
 * compila da solo, e questo file include soltanto checker.c.
 *
 * I certificati sono scritti a mano; quelli di dimostrazioni e modelli veri
 * sono stati presi da ciò che il motore scrive, gli altri ne sono
 * alterazioni. Ognuno ha la risposta attesa e, se non è accettato, un pezzo
 * del motivo, così un certificato respinto per la ragione sbagliata (una
 * parentesi dimenticata nel test) non passa per buono:
 *
 *   - accettati: dimostrazioni per assurdo con istanze e nomi (11c-11f), un
 *     controesempio e un esempio calcolati sul modello (11a, 11b), anche con
 *     un insieme vuoto;
 *   - respinti: un'istanza mancante, su un termine dell'insieme sbagliato o
 *     su un elemento mai nominato, di una condizione che non c'è; un nome
 *     già usato; un caso dimenticato; l'enunciato cambiato; una cella della
 *     tabella cambiata; e le dimostrazioni che inventano un elemento di un
 *     insieme che potrebbe essere vuoto, il falso "vale anche per gli
 *     insiemi vuoti";
 *   - non controllabili: modelli o enunciati troppo grandi. Non devono mai
 *     diventare un "accettato".
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

/* Un'operazione con un elemento neutro a sinistra u e uno a destra v: sono
 * lo stesso elemento. */
#define NEUTRAL_HEAD "sort S\nsymbol perp S S S\nsymbol u S\nsymbol v S\n"
#define LEFT "(forall x S (= (perp u x) x))"
#define RIGHT "(forall x S (= (perp x v) x))"
#define NEUTRAL "statement (implies (and (and (and (and true true) true) " LEFT ") " RIGHT ") (= u v))\n"

/* g dopo f è l'identità: f è iniettiva e g suriettiva. Due affermazioni,
 * due casi. */
#define INVERSE_HEAD "certificate structure\nsort S\nsort T\nsymbol f S T\nsymbol g T S\n" \
    "statement (implies (and (and true true) (forall x S (= (g (f x)) x))) " \
    "(and (forall a S (forall b S (implies (= (f a) (f b)) (= a b)))) (forall y S (exists x T (= (g x) y)))))\n"
#define INJECTIVE "(forall a S (forall b S (implies (= (f a) (f b)) (= a b))))"
#define SURJECTIVE "(forall y S (exists x T (= (g x) y)))"
#define INVERSE_FIRST "case (not " INJECTIVE ")\nwitness " INJECTIVE " a_1\n" \
    "witness (forall b S (implies (= (f a_1) (f b)) (= a_1 b))) b_2\n" \
    "instance (forall x S (= (g (f x)) x)) a_1\ninstance (forall x S (= (g (f x)) x)) b_2\n"
#define INVERSE_SECOND "case (not " SURJECTIVE ")\nwitness " SURJECTIVE " y_1\n" \
    "instance (forall x S (= (g (f x)) x)) y_1\ninstance (exists x T (= (g x) y_1)) (f y_1)\n"

/* L'associatività vale anche per quattro elementi. */
#define ASSOCIATIVE "(forall x S (forall y S (forall z S (= (perp (perp x y) z) (perp x (perp y z))))))"
#define FOUR "(forall a S (forall b S (forall c S (forall d S (= (perp (perp (perp a b) c) d) (perp a (perp b (perp c d))))))))"
#define FOUR_HEAD "certificate structure\nsort S\nsymbol perp S S S\nstatement (implies " ASSOCIATIVE " " FOUR ")\ncase (not " FOUR ")\n"
#define FOUR_NAMES(second) "witness " FOUR " a_1\n" \
    "witness (forall b S (forall c S (forall d S (= (perp (perp (perp a_1 b) c) d) (perp a_1 (perp b (perp c d))))))) " second "\n" \
    "witness (forall c S (forall d S (= (perp (perp (perp a_1 b_1) c) d) (perp a_1 (perp b_1 (perp c d)))))) c_1\n" \
    "witness (forall d S (= (perp (perp (perp a_1 b_1) c_1) d) (perp a_1 (perp b_1 (perp c_1 d))))) d_1\n"
#define FOUR_STEPS "instance " ASSOCIATIVE " (perp a_1 b_1) c_1 d_1\ninstance " ASSOCIATIVE " a_1 b_1 (perp c_1 d_1)\n"

/* In un gruppo si semplifica a sinistra: serve l'inverso di a, che ha un
 * nome dato da witness. */
#define GROUP_INVERSE "(forall x S (exists y S (and (= (perp x y) e) (= (perp y x) e))))"
#define GROUP_NEUTRAL "(forall x S (and (= (perp e x) x) (= (perp x e) x)))"
#define CANCEL "(forall a S (forall x S (forall y S (implies (= (perp a x) (perp a y)) (= x y)))))"
#define GROUP(name) "certificate structure\nsort S\nsymbol perp S S S\nsymbol e S\n" \
    "statement (implies (and (and (and (and true " ASSOCIATIVE ") true) " GROUP_NEUTRAL ") " GROUP_INVERSE ") " CANCEL ")\n" \
    "case (not " CANCEL ")\nwitness " CANCEL " " name "\n" \
    "witness (forall x S (forall y S (implies (= (perp a_1 x) (perp a_1 y)) (= x y)))) x_2\n" \
    "witness (forall y S (implies (= (perp a_1 x_2) (perp a_1 y)) (= x_2 y))) y_3\n" \
    "instance " GROUP_NEUTRAL " x_2\ninstance " GROUP_NEUTRAL " y_3\ninstance " GROUP_INVERSE " a_1\n" \
    "witness (exists y S (and (= (perp a_1 y) e) (= (perp y a_1) e))) y_4\n" \
    "instance " ASSOCIATIVE " y_4 a_1 x_2\ninstance " ASSOCIATIVE " y_4 a_1 y_3\nend\n"

/* Una proiezione pi da S per T in S è suriettiva se T non è vuoto. */
#define PROJECTION "(forall x S (forall y T (= (pi x y) x)))"
#define REACHED "(forall z S (exists x S (exists y T (= (pi x y) z))))"
#define PROJECTION_HEAD "certificate structure\nsort T\nsort S\nsymbol pi S T S\n"

/* Due elementi neutri a sinistra, u e v: un modello in cui sono diversi. */
#define LEFT_LEFT "statement (implies (and (and (and (and true true) true) " LEFT ") (forall x S (= (perp v x) x))) (= u v))\n"
#define MODEL_HEAD "sort S 2\nsymbol perp S S S\nsymbol u S\nsymbol v S\n"

static const Case cases[] = {
    /* Dimostrazioni per assurdo, scritte dal motore. */
    {"a left and a right neutral element coincide", CK_ACCEPTED, "",
     "certificate structure\n" NEUTRAL_HEAD NEUTRAL "case (not (= u v))\ninstance " LEFT " v\ninstance " RIGHT " u\nend\n"},
    {"the same proof with its two instances in the other order", CK_ACCEPTED, "",
     "certificate structure\n" NEUTRAL_HEAD NEUTRAL "case (not (= u v))\ninstance " RIGHT " u\ninstance " LEFT " v\nend\n"},
    {"associativity for four elements: four names and two instances", CK_ACCEPTED, "",
     FOUR_HEAD FOUR_NAMES("b_1") FOUR_STEPS "end\n"},
    {"f injective and g surjective: two claims, two cases", CK_ACCEPTED, "",
     INVERSE_HEAD INVERSE_FIRST INVERSE_SECOND "end\n"},
    {"cancellation in a group, with a name for the inverse", CK_ACCEPTED, "", GROUP("a_1")},
    {"the composition of two injective maps, instances on applied terms", CK_ACCEPTED, "",
     "certificate structure\nsort S\nsymbol f S S\nsymbol g S S\n"
     "statement (implies (and (forall a S (forall b S (implies (= (f a) (f b)) (= a b)))) (forall a S (forall b S (implies (= (g a) (g b)) (= a b))))) "
     "(forall a S (forall b S (implies (= (f (g a)) (f (g b))) (= a b)))))\n"
     "case (not (forall a S (forall b S (implies (= (f (g a)) (f (g b))) (= a b)))))\n"
     "witness (forall a S (forall b S (implies (= (f (g a)) (f (g b))) (= a b)))) a_1\n"
     "witness (forall b S (implies (= (f (g a_1)) (f (g b))) (= a_1 b))) b_2\n"
     "instance (forall a S (forall b S (implies (= (f a) (f b)) (= a b)))) (g b_2) (g a_1)\n"
     "instance (forall a S (forall b S (implies (= (g a) (g b)) (= a b)))) b_2 a_1\nend\n"},
    {"symmetric and transitive: membership in a relation, with congruence", CK_ACCEPTED, "",
     "certificate structure\nsort S\nsymbol R S S bool\n"
     "statement (implies true (implies (and (forall x S (forall y S (implies (R x y) (R y x)))) "
     "(forall x S (forall y S (forall z S (implies (and (R x y) (R y z)) (R x z)))))) (forall x S (forall y S (implies (R x y) (R x x))))))\n"
     "case (not (implies (and (forall x S (forall y S (implies (R x y) (R y x)))) "
     "(forall x S (forall y S (forall z S (implies (and (R x y) (R y z)) (R x z)))))) (forall x S (forall y S (implies (R x y) (R x x))))))\n"
     "witness (forall x S (forall y S (implies (R x y) (R x x)))) x_1\nwitness (forall y S (implies (R x_1 y) (R x_1 x_1))) y_2\n"
     "instance (forall x S (forall y S (implies (R x y) (R y x)))) x_1 y_2\n"
     "instance (forall x S (forall y S (forall z S (implies (and (R x y) (R y z)) (R x z))))) x_1 y_2 x_1\nend\n"},
    {"a non-empty part closed under f: an existential hypothesis gives the element", CK_ACCEPTED, "",
     "certificate structure\nsort S\nsymbol f S S\nsymbol X S bool\n"
     "statement (implies (and (and (and true true) (exists x S (X x))) (forall x S (implies (X x) (X (f x))))) (exists y S (and (X y) (X (f (f y))))))\n"
     "case (not (exists y S (and (X y) (X (f (f y))))))\nwitness (exists x S (X x)) x_1\n"
     "instance (forall x S (implies (X x) (X (f x)))) x_1\ninstance (exists y S (and (X y) (X (f (f y))))) x_1\n"
     "instance (forall x S (implies (X x) (X (f x)))) (f x_1)\nend\n"},
    {"a projection reaches every element when T is not empty", CK_ACCEPTED, "",
     PROJECTION_HEAD "statement (implies (and (and (exists t T (= t t)) true) " PROJECTION ") " REACHED ")\n"
     "case (not " REACHED ")\nwitness " REACHED " z_2\nwitness (exists t T (= t t)) t_1\n"
     "instance " PROJECTION " z_2 t_1\ninstance (exists x S (exists y T (= (pi x y) z_2))) z_2 t_1\nend\n"},
    {"a property of every element, with a name for the supposed exception", CK_ACCEPTED, "",
     "certificate structure\nsort S\nsymbol f S S\nstatement (forall x S (= (f x) (f x)))\n"
     "case (not (forall x S (= (f x) (f x))))\nwitness (forall x S (= (f x) (f x))) x_1\nend\n"},

    /* Modelli. */
    {"two left neutral elements: a counterexample with two elements", CK_ACCEPTED, "",
     "certificate counterexample\n" MODEL_HEAD LEFT_LEFT "table perp 0 0 1 1\ntable u 1\ntable v 0\nend\n"},
    {"an injective map that moves an element: an example", CK_ACCEPTED, "",
     "certificate example\nsort S 2\nsymbol f S S\n"
     "statement (and (forall a S (forall b S (implies (= (f a) (f b)) (= a b)))) (exists x S (not (= (f x) x))))\ntable f 1 0\nend\n"},
    {"a counterexample with two sets", CK_ACCEPTED, "",
     "certificate counterexample\nsort S 1\nsort T 2\nsymbol f S T\nsymbol g T S\n"
     "statement (implies (and (and true true) (forall x S (= (g (f x)) x))) (forall y T (exists x S (= (f x) y))))\ntable f 0\ntable g 0 0\nend\n"},
    {"a relation as a table of truth values", CK_ACCEPTED, "",
     "certificate counterexample\nsort S 2\nsymbol R S S bool\n"
     "statement (implies (and true (forall x S (forall y S (forall z S (implies (and (R x y) (R y z)) (R x z)))))) "
     "(forall x S (forall y S (implies (R x y) (R y x)))))\ntable R 0 1 0 0\nend\n"},
    {"an empty set refutes a statement that needs an element", CK_ACCEPTED, "",
     "certificate counterexample\nsort S 0\nsymbol f S S\nstatement (exists x S (= (f x) (f x)))\ntable f\nend\n"},

    /* Dimostrazioni sbagliate. */
    {"an instance missing: the facts are not contradictory", CK_REJECTED, "non sono contraddittorie",
     "certificate structure\n" NEUTRAL_HEAD NEUTRAL "case (not (= u v))\ninstance " LEFT " v\nend\n"},
    {"no steps at all", CK_REJECTED, "non sono contraddittorie",
     "certificate structure\n" NEUTRAL_HEAD NEUTRAL "case (not (= u v))\nend\n"},
    {"an instance of a condition that the statement does not contain", CK_REJECTED, "non è comparsa",
     "certificate structure\n" NEUTRAL_HEAD NEUTRAL "case (not (= u v))\ninstance (forall x S (= (perp x u) x)) v\ninstance " RIGHT " u\nend\n"},
    {"the statement changed: the right neutral element made a left one", CK_REJECTED, "non è comparsa",
     "certificate structure\n" NEUTRAL_HEAD LEFT_LEFT "case (not (= u v))\ninstance " LEFT " v\ninstance " RIGHT " u\nend\n"},
    {"an instance on an element of the other set", CK_REJECTED, "in un altro insieme",
     INVERSE_HEAD "case (not " INJECTIVE ")\nwitness " INJECTIVE " a_1\n"
     "witness (forall b S (implies (= (f a_1) (f b)) (= a_1 b))) b_2\ninstance (forall x S (= (g (f x)) x)) (f a_1)\n" INVERSE_SECOND "end\n"},
    {"an instance on an element that was never named", CK_REJECTED, "non è dichiarato",
     "certificate structure\n" NEUTRAL_HEAD NEUTRAL "case (not (= u v))\ninstance " LEFT " w\ninstance " RIGHT " u\nend\n"},
    {"an instance with more elements than quantifiers", CK_REJECTED, "di troppo",
     "certificate structure\n" NEUTRAL_HEAD NEUTRAL "case (not (= u v))\ninstance " LEFT " v u\ninstance " RIGHT " u\nend\n"},
    {"a name reused: the inverse called like the neutral element", CK_REJECTED, "non c'è ancora", GROUP("e")},
    {"a name given twice in the same case", CK_REJECTED, "non c'è ancora", FOUR_HEAD FOUR_NAMES("a_1") FOUR_STEPS "end\n"},
    {"a witness for a condition without a quantifier", CK_REJECTED, "comincia con un quantificatore",
     "certificate structure\n" NEUTRAL_HEAD NEUTRAL "case (not (= u v))\nwitness (= u v) c\nend\n"},
    {"a claim left out: the cases do not cover the statement", CK_REJECTED, "non coprono",
     INVERSE_HEAD INVERSE_FIRST "end\n"},
    {"a name from an earlier case used in a later one", CK_REJECTED, "non è dichiarato",
     INVERSE_HEAD INVERSE_FIRST "case (not " SURJECTIVE ")\ninstance (forall x S (= (g (f x)) x)) a_1\nend\n"},
    {"an element invented for a set that may be empty", CK_REJECTED, "con vuoti: S",
     "certificate structure\nsort S\nsymbol f S S\nstatement (exists x S (= (f x) (f x)))\n"
     "case (not (exists x S (= (f x) (f x))))\nwitness (exists x S (= (f x) (f x))) c\ninstance (exists x S (= (f x) (f x))) c\nend\n"},
    {"an element invented for an empty set beside one that is not", CK_REJECTED, "con vuoti: T",
     "certificate structure\nsort S\nsort T\nsymbol u S\nstatement (implies true (exists y T (= y y)))\n"
     "case (not (exists y T (= y y)))\nwitness (exists y T (= y y)) y_1\ninstance (exists y T (= y y)) y_1\nend\n"},
    {"equal first arguments do not make two values equal", CK_REJECTED, "non sono contraddittorie",
     "certificate structure\nsort S\nsymbol m S S S\nstatement (forall a S (forall b S (= (m a b) (m a a))))\n"
     "case (not (forall a S (forall b S (= (m a b) (m a a)))))\nwitness (forall a S (forall b S (= (m a b) (m a a)))) a_1\n"
     "witness (forall b S (= (m a_1 b) (m a_1 a_1))) b_1\nend\n"},
    {"equal second arguments do not make two values equal", CK_REJECTED, "non sono contraddittorie",
     "certificate structure\nsort S\nsymbol m S S S\nstatement (forall a S (forall b S (= (m a b) (m b b))))\n"
     "case (not (forall a S (forall b S (= (m a b) (m b b)))))\nwitness (forall a S (forall b S (= (m a b) (m b b)))) a_1\n"
     "witness (forall b S (= (m a_1 b) (m b b))) b_1\nend\n"},
    {"a projection without T non-empty: an element of T taken from nowhere", CK_REJECTED, "con vuoti: T",
     PROJECTION_HEAD "statement (implies (and true " PROJECTION ") " REACHED ")\ncase (not " REACHED ")\nwitness " REACHED " z_1\n"
     "instance (exists x S (exists y T (= (pi x y) z_1))) z_1\nwitness (exists y T (= (pi z_1 y) z_1)) t_1\n"
     "instance " PROJECTION " z_1 t_1\ninstance (exists x S (exists y T (= (pi x y) z_1))) z_1 t_1\nend\n"},

    /* Modelli sbagliati. */
    {"a table cell changed: in that model the statement holds", CK_REJECTED, "non è un controesempio",
     "certificate counterexample\n" MODEL_HEAD LEFT_LEFT "table perp 1 0 1 1\ntable u 1\ntable v 0\nend\n"},
    {"the statement changed: the model is no counterexample to the true one", CK_REJECTED, "non è un controesempio",
     "certificate counterexample\n" MODEL_HEAD NEUTRAL "table perp 0 0 1 1\ntable u 1\ntable v 0\nend\n"},
    {"a counterexample presented as an example", CK_REJECTED, "non è un esempio",
     "certificate example\n" MODEL_HEAD LEFT_LEFT "table perp 0 0 1 1\ntable u 1\ntable v 0\nend\n"},
    {"an example presented as a counterexample", CK_REJECTED, "non è un controesempio",
     "certificate counterexample\nsort S 2\nsymbol f S S\nstatement (exists x S (not (= (f x) x)))\ntable f 1 0\nend\n"},
    {"a value outside its set", CK_REJECTED, "fuori dal suo insieme",
     "certificate counterexample\n" MODEL_HEAD LEFT_LEFT "table perp 0 0 1 1\ntable u 2\ntable v 0\nend\n"},
    {"a table with a value missing", CK_REJECTED, "meno valori",
     "certificate counterexample\n" MODEL_HEAD LEFT_LEFT "table perp 0 0 1\ntable u 1\ntable v 0\nend\n"},
    {"a table with a value too many", CK_REJECTED, "più valori",
     "certificate counterexample\n" MODEL_HEAD LEFT_LEFT "table perp 0 0 1 1 0\ntable u 1\ntable v 0\nend\n"},
    {"a symbol without its table", CK_REJECTED, "non ha la sua tabella",
     "certificate counterexample\n" MODEL_HEAD LEFT_LEFT "table perp 0 0 1 1\ntable u 1\nend\n"},
    {"a named element of an empty set", CK_REJECTED, "fuori dal suo insieme",
     "certificate counterexample\nsort S 0\nsymbol u S\nstatement (exists x S (= x u))\ntable u 0\nend\n"},
    {"a map from a set with elements into an empty one", CK_REJECTED, "fuori dal suo insieme",
     "certificate counterexample\nsort S 1\nsort T 0\nsymbol f S T\nstatement (forall x S (= (f x) (f x)))\ntable f 0\nend\n"},

    /* Certificati scritti male. */
    {"a size in a proof", CK_REJECTED, "nei soli modelli",
     "certificate structure\nsort S 2\nsymbol perp S S S\nsymbol u S\nsymbol v S\n" NEUTRAL "case (not (= u v))\nend\n"},
    {"a model without sizes", CK_REJECTED, "nei soli modelli",
     "certificate counterexample\nsort S\nsymbol u S\nstatement (= u u)\ntable u 0\nend\n"},
    {"an equality between elements of two sets", CK_REJECTED, "insiemi diversi",
     "certificate structure\nsort S\nsort T\nsymbol f S T\nsymbol u S\nstatement (= (f u) u)\nend\n"},
    {"a map written without its argument", CK_REJECTED, "numero di argomenti",
     "certificate structure\nsort S\nsymbol f S S\nsymbol u S\nstatement (= f u)\nend\n"},
    {"an element where a condition is needed", CK_REJECTED, "serve una condizione",
     "certificate structure\n" NEUTRAL_HEAD "statement (perp u v)\nend\n"},
    {"a quantifier over a set that is not declared", CK_REJECTED, "insieme dichiarato",
     "certificate structure\nsort S\nstatement (forall x V (= x x))\nend\n"},
    {"a step before any case", CK_REJECTED, "fuori posto",
     "certificate structure\n" NEUTRAL_HEAD NEUTRAL "instance " LEFT " v\nend\n"},
    {"a table in a proof", CK_REJECTED, "fuori posto",
     "certificate structure\n" NEUTRAL_HEAD NEUTRAL "table u 0\nend\n"},
    {"missing end", CK_REJECTED, "scritto male",
     "certificate structure\n" NEUTRAL_HEAD NEUTRAL "case (not (= u v))\ninstance " LEFT " v\ninstance " RIGHT " u\n"},

    /* Certificati che il verificatore non sa controllare. */
    {"a set larger than the checker handles", CK_UNSUPPORTED, "troppo grande",
     "certificate counterexample\nsort S 9\nsymbol u S\nstatement (= u u)\ntable u 0\nend\n"},
    {"more sets than the checker handles", CK_UNSUPPORTED, "troppi insiemi",
     "certificate structure\nsort A\nsort B\nsort C\nsort D\nsort E\nsort F\nsort G\nstatement true\nend\n"},
    {"a model where the statement takes too long to compute", CK_UNSUPPORTED, "troppo grande per calcolarvi",
     "certificate counterexample\nsort S 8\nstatement (forall a S (forall b S (forall c S (forall d S (forall e S (forall f S (forall g S "
     "(forall h S (forall i S (= a a))))))))))\nend\n"},
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
    char message[512];
    const char *why;

    /* La tabella: risposta attesa, e il motivo per chi non è accettato. */
    for (size_t i = 0; i < sizeof cases / sizeof *cases; i++) {
        const char *text = cases[i].text;
        CkAnswer answer = ck_check(&text, &why);
        snprintf(message, sizeof message, "\"%s\" should be %s but is %s (%s)", cases[i].name,
                 answer_name(cases[i].expected), answer_name(answer), why);
        check(answer == cases[i].expected, message);
        snprintf(message, sizeof message, "\"%s\" should mention \"%s\" as its reason, not \"%s\"", cases[i].name, cases[i].why, why);
        check(answer != cases[i].expected || strstr(why, cases[i].why) != NULL, message);
    }

    /* Un certificato non lascia tracce nel successivo: dopo uno respinto con
     * i suoi nomi, uno giusto che riusa gli stessi nomi resta accettato; e
     * due certificati in un testo si leggono uno dopo l'altro. */
    const char *reused = FOUR_HEAD FOUR_NAMES("a_1") FOUR_STEPS "end\n";
    ck_check(&reused, &why);
    const char *again = FOUR_HEAD FOUR_NAMES("b_1") FOUR_STEPS "end\n";
    check(ck_check(&again, &why) == CK_ACCEPTED, "a rejected certificate should not affect the next one");
    const char *two = INVERSE_HEAD INVERSE_FIRST INVERSE_SECOND "end\n" INVERSE_HEAD INVERSE_FIRST "end\n";
    check(ck_check(&two, &why) == CK_ACCEPTED, "the first of two certificates should be accepted");
    check(ck_check(&two, &why) == CK_REJECTED, "the second of two certificates should be read next, and rejected");

    printf("%s: %d of %d structure certificate checks passed\n", failures ? "FAIL" : "PASS", checks - failures, checks);
    return failures ? 1 : 0;
}
