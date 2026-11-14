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
 *     un insieme vuoto; e con le parti (11g): un'ipotesi su ogni parte usata
 *     con parti definite da una voce part, l'esistenza di una parte che dà
 *     un nome o che si dimostra con una parte definita, un modello in cui si
 *     provano tutti i sottoinsiemi;
 *   - respinti, per le parti: una definizione con la regola sbagliata, che
 *     nomina sé stessa o una variabile libera, o con un nome già usato;
 *     un'istanza con una parte dell'insieme sbagliato, con un elemento o con
 *     un'applicazione al posto di una parte; un \{c\} con c inventato per un
 *     insieme che può essere vuoto; un modello in cui l'ipotesi su ogni parte
 *     non vale; un esempio smentito dalla parte vuota;
 *   - respinti: un'istanza mancante, su un termine dell'insieme sbagliato o
 *     su un elemento mai nominato, di una condizione che non c'è; un nome
 *     già usato; un caso dimenticato; l'enunciato cambiato; una cella della
 *     tabella cambiata; e le dimostrazioni che inventano un elemento di un
 *     insieme che potrebbe essere vuoto, il falso "vale anche per gli
 *     insiemi vuoti";
 *   - non controllabili: modelli o enunciati troppo grandi. Non devono mai
 *     diventare un "accettato";
 *   - 11h, gli insiemi di parti e di applicazioni: accettate le dimostrazioni
 *     con le loro leggi (voci axiom) e i modelli sulle parti vere; respinte
 *     le leggi false (due parti qualsiasi uguali, la parte di Russell, che
 *     dimostrerebbe che nessun elemento è uguale a sé, una comprensione che
 *     nomina la sua parte, una comprensione in un insieme che non è di
 *     parti), una tabella per l'appartenenza, che il verificatore calcola da
 *     sé, e la dimostrazione senza la comprensione che usa;
 *   - 11j, l'equipotenza: accettate le dimostrazioni con una biiezione con
 *     un nome (witness), un testimone (instance) e le applicazioni definite
 *     (define), e i modelli in cui si contano gli elementi; respinti un
 *     testimone che non è una biiezione (la diagonale di S \times S), un ramo
 *     dimenticato, nessun testimone, le applicazioni nell'ordine sbagliato o
 *     con gli insiemi sbagliati, un nome già usato, e le definizioni di forma
 *     sbagliata: che nominano sé stesse (una parte che non contiene il suo
 *     valore, un punto fisso di un'applicazione che non ne ha, che
 *     dimostrerebbero il falso), su un insieme sbagliato, con i due lati
 *     scambiati o con un valore che non è un insieme; e la biiezione con un
 *     nome tra un insieme con elementi e uno vuoto, che non esiste, e
 *     farebbe passare "S non vuoto implica T non vuoto";
 *   - 11k, la legge delle applicazioni con due valori: accettata la
 *     dimostrazione che la usa; respinte le leggi false (un'applicazione che
 *     vale u e v insieme, che dimostrerebbe che due elementi qualsiasi sono
 *     uguali, e "ogni applicazione" al posto di "una"), la stessa legge
 *     scritta con x = p girato, che il verificatore non riconosce, e la
 *     dimostrazione senza le istanze.
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

/* 11g. "Se f(X) \subseteq f(Y) implica X \subseteq Y per ogni X, Y, f è
 * iniettiva": l'ipotesi su tutte le parti usata con X = \{b\} e Y = \{a\},
 * definite con due voci part. PART_PROOF ha per argomenti la definizione di
 * X_3, che le alterazioni cambiano, l'elemento che le istanze di quella
 * definizione nominano, e le parti scelte. */
#define PART_RULE "(forallpart X S (forallpart Y S (implies (forall c T (implies (exists d S (and (X d) (= (f d) c))) " \
    "(exists g S (and (Y g) (= (f g) c))))) (forall h S (implies (X h) (Y h))))))"
#define PART_INJECTIVE "(forall a S (forall b S (implies (= (f a) (f b)) (= a b))))"
#define PART_HEAD "certificate structure\nsort S\nsort T\nsymbol f S T\nstatement (implies " PART_RULE " " PART_INJECTIVE ")\n" \
    "case (not " PART_INJECTIVE ")\nwitness " PART_INJECTIVE " a_1\nwitness (forall b S (implies (= (f a_1) (f b)) (= a_1 b))) b_2\n"
#define PART_PROOF(single_x, x, choice) PART_HEAD single_x "part Y_4 j S (= j a_1)\ninstance " PART_RULE " " choice "\n" \
    "instance (forall j S (iff (Y_4 j) (= j a_1))) a_1\n" \
    "witness (forall c T (implies (exists d S (and (X_3 d) (= (f d) c))) (exists g S (and (Y_4 g) (= (f g) c))))) c_5\n" \
    "witness (exists d S (and (X_3 d) (= (f d) c_5))) d_6\ninstance (forall j S (iff (X_3 j) (= j " x "))) d_6\n" \
    "instance (exists g S (and (Y_4 g) (= (f g) c_5))) a_1\ninstance (forall j S (iff (X_3 j) (= j " x "))) b_2\n" \
    "instance (forall j S (iff (Y_4 j) (= j a_1))) b_2\ninstance (forall h S (implies (X_3 h) (Y_4 h))) b_2\nend\n"
#define SINGLE_X "part X_3 j S (= j b_2)\n"

/* 11g. "Se f(X) = T per una parte X, f è suriettiva": l'esistenza di una
 * parte, tra le ipotesi, dà una parte con un nome. */
#define PART_EXISTS "(existspart X S (forall a T (exists b S (and (X b) (= (f b) a)))))"
#define PART_SURJECTIVE "(forall y T (exists x S (= (f x) y)))"

/* 11g. "Esiste X con f(X) = f(S)": X = S, che c'è anche quando S è vuoto. */
#define WHOLE "(existspart X S (forall a T (iff (exists b S (and (X b) (= (f b) a))) (exists c S (= (f c) a)))))"

/* 11g. Un'ipotesi che vale per ogni parte di un insieme con un solo
 * elemento o vuoto, e una conclusione che chiede un elemento. */
#define TWO_PARTS "(forallpart X S (or (forall y S (X y)) (forall y S (not (X y)))))"

/* Due elementi neutri a sinistra, u e v: un modello in cui sono diversi. */
#define LEFT_LEFT "statement (implies (and (and (and (and true true) true) " LEFT ") (forall x S (= (perp v x) x))) (= u v))\n"
#define MODEL_HEAD "sort S 2\nsymbol perp S S S\nsymbol u S\nsymbol v S\n"

/* 11h. f(x) = S \setminus \{x\}, con f \colon S \to \mathcal{P}(S): f è
 * iniettiva, e S non è un'immagine. PS è l'insieme delle parti di S, inPS
 * l'appartenenza; le voci axiom sono le leggi delle parti, e la seconda
 * affermazione usa la parte "tutto S" di una comprensione. */
#define PARTS_HEAD "certificate structure\nsort S\nsort PS parts S inPS\nsymbol f S PS\n" \
    "statement (implies (and (exists s S (= s s)) (forall x S (forall c S (iff (inPS c (f x)) (not (= c x)))))) " \
    "(and (forall a S (forall b S (implies (= (f a) (f b)) (= a b)))) (not (forall y PS (exists x S (= (f x) y))))))\n"
#define PARTS_EXTENSIONAL "axiom (forall d PS (forall g PS (implies (forall h S (iff (inPS h d) (inPS h g))) (= d g))))\n"
#define PARTS_WHOLE "axiom (exists m PS (forall n S (iff (inPS n m) true)))\n"
#define PARTS_RULE "(forall x S (forall c S (iff (inPS c (f x)) (not (= c x)))))"
#define PARTS_CASES "case (not (forall a S (forall b S (implies (= (f a) (f b)) (= a b)))))\n" \
    "witness (forall a S (forall b S (implies (= (f a) (f b)) (= a b)))) a_4\n" \
    "witness (forall b S (implies (= (f a_4) (f b)) (= a_4 b))) b_5\n" \
    "instance " PARTS_RULE " a_4 a_4\ninstance " PARTS_RULE " b_5 a_4\n" \
    "case (not (not (forall y PS (exists x S (= (f x) y)))))\n" \
    "witness (exists m PS (forall n S (iff (inPS n m) true))) m_3\n" \
    "instance (forall y PS (exists x S (= (f x) y))) m_3\nwitness (exists x S (= (f x) m_3)) x_6\n" \
    "instance " PARTS_RULE " x_6 x_6\ninstance (forall n S (iff (inPS n m_3) true)) x_6\nend\n"

/* 11h. Due applicazioni uguali in ogni punto sono la stessa: l'enunciato è
 * la legge stessa, e la dimostrazione ne usa la voce axiom. */
#define MAPS_LAW "(forall b MSS (forall c MSS (implies (forall d S (= (apMSS b d) (apMSS c d))) (= b c))))"
#define MAPS_STATEMENT "(forall f MSS (forall g MSS (implies (forall a S (= (apMSS f a) (apMSS g a))) (= f g))))"
#define MAPS_PROOF(law) "certificate structure\nsort S\nsort MSS maps S S apMSS\nstatement " MAPS_STATEMENT "\naxiom " law "\n" \
    "case (not " MAPS_STATEMENT ")\nwitness " MAPS_STATEMENT " f_1\n" \
    "witness (forall g MSS (implies (forall a S (= (apMSS f_1 a) (apMSS g a))) (= f_1 g))) g_2\n" \
    "instance " law " g_2 f_1\nwitness (forall d S (= (apMSS g_2 d) (apMSS f_1 d))) d_3\n" \
    "instance (forall a S (= (apMSS f_1 a) (apMSS g_2 a))) d_3\nend\n"

/* 11h. f(x) = S \setminus \{x\} non è suriettiva: un controesempio a chi lo
 * dice, con S di un elemento e f(s_0) la parte vuota, la numero 0. */
#define PARTS_MODEL(size, extra) "certificate counterexample\nsort S " size "\nsort PS parts S inPS\nsymbol f S PS\n" \
    "statement (implies (and (exists s S (= s s)) (forall x S (forall a S (iff (inPS a (f x)) (not (= a x)))))) (forall y PS (exists x S (= (f x) y))))\n" \
    "table f 0\n" extra "end\n"

/* 11j. S \times T \sim T \times S: le quattro applicazioni del testimone,
 * definite con d1, e i due rami della dimostrazione, come li scrive il
 * motore. SWAP(gamma2) ha per argomento il valore di \gamma_{2}(f, j). */
#define SWAP_HEAD "certificate structure\nsort S\nsort T\nstatement (equipotent (S T) (T S))\ncase (not (equipotent (S T) (T S)))\n"
#define SWAP_DEFINE(gamma2) "define gamma_1 S T T (forall f S (forall j T (= (gamma_1 f j) j)))\n" \
    "define gamma_2 S T S (forall f S (forall j T (= (gamma_2 f j) " gamma2 ")))\n" \
    "define delta_3 T S S (forall h T (forall g S (= (delta_3 h g) g)))\n" \
    "define delta_4 T S T (forall h T (forall g S (= (delta_4 h g) h)))\n" \
    "instance (equipotent (S T) (T S)) gamma_1 gamma_2 delta_3 delta_4\n"
#define SWAP_RULES "instance (forall f S (forall j T (= (gamma_1 f j) j))) a_5 b_6\n" \
    "instance (forall f S (forall j T (= (gamma_2 f j) f))) a_5 b_6\n" \
    "instance (forall h T (forall g S (= (delta_3 h g) g))) (gamma_1 a_5 b_6) (gamma_2 a_5 b_6)\n" \
    "instance (forall h T (forall g S (= (delta_4 h g) h))) (gamma_1 a_5 b_6) (gamma_2 a_5 b_6)\n"
#define SWAP_FIRST "witness (forall a S (forall b T (and (= (delta_3 (gamma_1 a b) (gamma_2 a b)) a) (= (delta_4 (gamma_1 a b) (gamma_2 a b)) b)))) a_5\n" \
    "witness (forall b T (and (= (delta_3 (gamma_1 a_5 b) (gamma_2 a_5 b)) a_5) (= (delta_4 (gamma_1 a_5 b) (gamma_2 a_5 b)) b))) b_6\n" SWAP_RULES
#define SWAP_SECOND "witness (forall c T (forall d S (and (= (gamma_1 (delta_3 c d) (delta_4 c d)) c) (= (gamma_2 (delta_3 c d) (delta_4 c d)) d)))) c_7\n" \
    "witness (forall d S (and (= (gamma_1 (delta_3 c_7 d) (delta_4 c_7 d)) c_7) (= (gamma_2 (delta_3 c_7 d) (delta_4 c_7 d)) d))) d_8\n" \
    "instance (forall f S (forall j T (= (gamma_1 f j) j))) (delta_3 c_7 d_8) (delta_4 c_7 d_8)\n" \
    "instance (forall f S (forall j T (= (gamma_2 f j) f))) (delta_3 c_7 d_8) (delta_4 c_7 d_8)\n" \
    "instance (forall h T (forall g S (= (delta_3 h g) g))) c_7 d_8\ninstance (forall h T (forall g S (= (delta_4 h g) h))) c_7 d_8\n"

/* 11j. S \sim T \implies \mathcal{P}(S) \sim \mathcal{P}(T): la biiezione
 * dell'ipotesi con un nome, l'immagine delle parti definita con d2, e i due
 * rami. PARTS_IMAGE(define, inverse) ha per argomenti la voce define di
 * \gamma_{3}, che le alterazioni cambiano, e l'istanza dell'equipotenza. */
#define IMAGE_RULE "(forall H PS (forall F T (iff (inPT F (gamma_3 H)) (inPS (psi_2 F) H))))"
#define INVERSE_RULE "(forall K PT (forall C S (iff (inPS C (delta_4 K)) (inPT (varphi_1 C) K))))"
#define PS_LAW "(forall f PS (forall g PS (implies (forall h S (iff (inPS h f) (inPS h g))) (= f g))))"
#define PT_LAW "(forall j PT (forall k PT (implies (forall m T (iff (inPT m j) (inPT m k))) (= j k))))"
#define PARTS_IMAGE(define, instance) "certificate structure\nsort S\nsort T\nsort PS parts S inPS\nsort PT parts T inPT\n" \
    "statement (implies (equipotent S T) (equipotent PS PT))\naxiom " PS_LAW "\naxiom " PT_LAW "\n" \
    "case (not (equipotent PS PT))\nwitness (equipotent S T) varphi_1 psi_2\n" define \
    "define delta_4 PT PS " INVERSE_RULE "\n" instance \
    "witness (forall c PS (= (delta_4 (gamma_3 c)) c)) c_5\ninstance " PS_LAW " (delta_4 (gamma_3 c_5)) c_5\n" \
    "witness (forall h S (iff (inPS h (delta_4 (gamma_3 c_5))) (inPS h c_5))) h_6\ninstance (forall a S (= (psi_2 (varphi_1 a)) a)) h_6\n" \
    "instance " INVERSE_RULE " (gamma_3 c_5) h_6\ninstance " IMAGE_RULE " c_5 (varphi_1 h_6)\n" \
    "witness (forall d PT (= (gamma_3 (delta_4 d)) d)) d_8\ninstance " PT_LAW " (gamma_3 (delta_4 d_8)) d_8\n" \
    "witness (forall m T (iff (inPT m (gamma_3 (delta_4 d_8))) (inPT m d_8))) m_9\ninstance (forall b T (= (varphi_1 (psi_2 b)) b)) m_9\n" \
    "instance " IMAGE_RULE " (delta_4 d_8) m_9\ninstance " INVERSE_RULE " d_8 (psi_2 m_9)\nend\n"
#define IMAGE_DEFINE "define gamma_3 PS PT " IMAGE_RULE "\n"
#define IMAGE_INSTANCE "instance (equipotent PS PT) gamma_3 delta_4\n"

/* 11j. Gli insiemi vuoti e le biiezioni con un nome. Con S non vuoto e T
 * vuoto nessuna funzione va da S a T: il nome varphi_1 di una voce witness
 * equipotent non indica nulla, e l'istanza su (varphi_1 s_1) va lasciata
 * fuori. Se non lo fosse, "S non vuoto implica T non vuoto" passerebbe, con
 * l'equipotenza tra le ipotesi solo per poterle dare un nome. */
#define EMPTY_TRAP(hypothesis) "certificate structure\nsort S\nsort T\n" \
    "statement (implies (and " hypothesis " (exists s S (= s s))) (exists t T (= t t)))\n" \
    "case (not (implies (and " hypothesis " (exists s S (= s s))) (exists t T (= t t))))\n" \
    "witness (exists s S (= s s)) s_1\nwitness (equipotent S T) varphi_1 psi_2\ninstance (exists t T (= t t)) (varphi_1 s_1)\nend\n"

/* 11k. Una legge delle applicazioni con due valori, h(x) = u se x = p e v
 * altrimenti, e la dimostrazione che con lei ogni valore in un punto è
 * preso da qualche applicazione: l'istanza con u = v, poi quella in p. */
#define TWO_VALUES(eq, some) "(forall p S (forall u T (forall v T (" some " h MST (forall x S (and (implies " eq " (= (apMST h x) u)) " \
    "(implies (not " eq ") (= (apMST h x) v))))))))"
#define TWO_VALUES_HEAD "certificate structure\nsort S\nsort T\nsort MST maps S T apMST\n"
#define VALUE_REACHED "(forall p S (forall u T (exists h MST (= (apMST h p) u))))"
#define TWO_VALUES_PROOF(law, instances) TWO_VALUES_HEAD "statement " VALUE_REACHED "\naxiom " law "\ncase (not " VALUE_REACHED ")\n" \
    "witness " VALUE_REACHED " p_1\nwitness (forall u T (exists h MST (= (apMST h p_1) u))) u_2\n" instances "end\n"
#define TWO_VALUES_INSTANCES "instance " TWO_VALUES("(= x p)", "exists") " p_1 u_2 u_2\n" \
    "witness (exists h MST (forall x S (and (implies (= x p_1) (= (apMST h x) u_2)) (implies (not (= x p_1)) (= (apMST h x) u_2))))) h_3\n" \
    "instance (forall x S (and (implies (= x p_1) (= (apMST h_3 x) u_2)) (implies (not (= x p_1)) (= (apMST h_3 x) u_2)))) p_1\n" \
    "instance (exists h MST (= (apMST h p_1) u_2)) h_3\n"
#define ONE_VALUE_LAW "(forall p S (forall u T (forall v T (exists h MST (forall x S (and (= (apMST h x) u) (= (apMST h x) v)))))))"

static const Case cases[] = {
    /* 11j. L'equipotenza, le biiezioni con un nome, le applicazioni
     * definite. */
    {"equipotence: the swap of a product, with maps of two arguments", CK_ACCEPTED, "", SWAP_HEAD SWAP_DEFINE("f") SWAP_FIRST SWAP_SECOND "end\n"},
    {"a witness that is not a bijection: the diagonal of S times S", CK_REJECTED, "non sono contraddittorie",
     "certificate structure\nsort S\nstatement (equipotent (S S) (S S))\ncase (not (equipotent (S S) (S S)))\n"
     "define gamma_1 S S S (forall f S (forall j S (= (gamma_1 f j) f)))\ndefine gamma_2 S S S (forall f S (forall j S (= (gamma_2 f j) f)))\n"
     "define delta_3 S S S (forall h S (forall g S (= (delta_3 h g) h)))\ndefine delta_4 S S S (forall h S (forall g S (= (delta_4 h g) g)))\n"
     "instance (equipotent (S S) (S S)) gamma_1 gamma_2 delta_3 delta_4\n"
     "witness (forall a S (forall b S (and (= (delta_3 (gamma_1 a b) (gamma_2 a b)) a) (= (delta_4 (gamma_1 a b) (gamma_2 a b)) b)))) a_5\n"
     "witness (forall b S (and (= (delta_3 (gamma_1 a_5 b) (gamma_2 a_5 b)) a_5) (= (delta_4 (gamma_1 a_5 b) (gamma_2 a_5 b)) b))) b_6\n"
     "instance (forall f S (forall j S (= (gamma_1 f j) f))) a_5 b_6\ninstance (forall f S (forall j S (= (gamma_2 f j) f))) a_5 b_6\n"
     "instance (forall h S (forall g S (= (delta_3 h g) h))) (gamma_1 a_5 b_6) (gamma_2 a_5 b_6)\n"
     "instance (forall h S (forall g S (= (delta_4 h g) g))) (gamma_1 a_5 b_6) (gamma_2 a_5 b_6)\nend\n"},
    {"the swap without the second branch", CK_REJECTED, "non sono contraddittorie", SWAP_HEAD SWAP_DEFINE("f") SWAP_FIRST "end\n"},
    {"a definition that names itself in its value", CK_REJECTED, "forme ammesse", SWAP_HEAD SWAP_DEFINE("(gamma_2 f j)") SWAP_FIRST SWAP_SECOND "end\n"},
    {"a definition with a value in the wrong set", CK_REJECTED, "insiemi diversi", SWAP_HEAD SWAP_DEFINE("j") SWAP_FIRST SWAP_SECOND "end\n"},
    {"the maps of an equipotence instance in the wrong order", CK_REJECTED, "insiemi giusti",
     SWAP_HEAD "define gamma_1 S T T (forall f S (forall j T (= (gamma_1 f j) j)))\ndefine gamma_2 S T S (forall f S (forall j T (= (gamma_2 f j) f)))\n"
     "instance (equipotent (S T) (T S)) gamma_2 gamma_1 gamma_1 gamma_2\nend\n"},
    {"an equipotence with no witness at all", CK_REJECTED, "non sono contraddittorie",
     "certificate structure\nsort S\nsort T\nstatement (equipotent S T)\ncase (not (equipotent S T))\nend\n"},
    {"equipotence: the image of parts by a named bijection", CK_ACCEPTED, "", PARTS_IMAGE(IMAGE_DEFINE, IMAGE_INSTANCE)},
    {"the image of parts without the instance of the equipotence", CK_REJECTED, "non è comparsa", PARTS_IMAGE(IMAGE_DEFINE, "")},
    {"the image of parts defined over the wrong set", CK_REJECTED, "un forall per ogni argomento",
     PARTS_IMAGE("define gamma_3 PS PT (forall H PT (forall F T (iff (inPT F H) (inPT F H))))\n", IMAGE_INSTANCE)},
    {"a definition of parts with its two sides exchanged", CK_REJECTED, "forme ammesse",
     PARTS_IMAGE("define gamma_3 PS PT (forall H PS (forall F T (iff (inPS (psi_2 F) H) (inPT F (gamma_3 H)))))\n", IMAGE_INSTANCE)},
    {"a definition of parts by its own membership", CK_REJECTED, "forme ammesse",
     PARTS_IMAGE("define gamma_3 PS PT (forall H PS (forall F T (iff (inPT F (gamma_3 H)) (not (inPT F (gamma_3 H))))))\n", IMAGE_INSTANCE)},
    {"a map to parts defined by not containing its own value would prove that no element equals itself", CK_REJECTED, "forme ammesse",
     "certificate structure\nsort S\nsort PS parts S inPS\nstatement (forall s S (not (= s s)))\ncase (not (forall s S (not (= s s))))\n"
     "witness (forall s S (not (= s s))) s_1\n"
     "define gamma_1 S PS (forall x S (forall y S (iff (inPS y (gamma_1 x)) (not (inPS y (gamma_1 x))))))\n"
     "instance (forall x S (forall y S (iff (inPS y (gamma_1 x)) (not (inPS y (gamma_1 x)))))) s_1 s_1\nend\n"},
    {"a map defined as a fixed point of a map without fixed points", CK_REJECTED, "forme ammesse",
     "certificate structure\nsort S\nsymbol f S S\nstatement (implies (forall x S (not (= (f x) x))) (forall s S (not (= s s))))\n"
     "case (not (implies (forall x S (not (= (f x) x))) (forall s S (not (= s s)))))\nwitness (forall s S (not (= s s))) s_1\n"
     "define delta_1 S S (forall x S (= (delta_1 x) (f (delta_1 x))))\n"
     "instance (forall x S (= (delta_1 x) (f (delta_1 x)))) s_1\ninstance (forall x S (not (= (f x) x))) (delta_1 s_1)\nend\n"},
    {"a definition with a value that is not a set", CK_REJECTED, "insiemi dichiarati",
     PARTS_IMAGE("define gamma_3 PS bool (forall H PS (= H H))\n", IMAGE_INSTANCE)},
    {"an equipotence instance with the maps swapped", CK_REJECTED, "insiemi giusti",
     PARTS_IMAGE(IMAGE_DEFINE, "instance (equipotent PS PT) delta_4 gamma_3\n")},
    {"a named bijection given an old name", CK_REJECTED, "non c'è ancora",
     "certificate structure\nsort S\nsort T\nstatement (implies (equipotent S T) (equipotent T S))\ncase (not (equipotent T S))\n"
     "witness (equipotent S T) varphi_1 varphi_1\nend\n"},
    {"equipotence: the inverse of a named bijection", CK_ACCEPTED, "",
     "certificate structure\nsort S\nsort T\nstatement (implies (equipotent S T) (equipotent T S))\ncase (not (equipotent T S))\n"
     "witness (equipotent S T) varphi_1 psi_2\ninstance (equipotent T S) psi_2 varphi_1\n"
     "witness (forall c T (= (varphi_1 (psi_2 c)) c)) c_3\ninstance (forall b T (= (varphi_1 (psi_2 b)) b)) c_3\n"
     "witness (forall d S (= (psi_2 (varphi_1 d)) d)) d_4\ninstance (forall a S (= (psi_2 (varphi_1 a)) a)) d_4\nend\n"},
    {"a named bijection used the wrong way round", CK_REJECTED, "insiemi giusti",
     "certificate structure\nsort S\nsort T\nstatement (implies (equipotent S T) (equipotent T S))\ncase (not (equipotent T S))\n"
     "witness (equipotent S T) varphi_1 psi_2\ninstance (equipotent T S) varphi_1 psi_2\nend\n"},
    {"an instance of an equipotence that is not in the case", CK_REJECTED, "non è comparsa",
     "certificate structure\nsort S\nsort T\nstatement (equipotent S S)\ncase (not (equipotent S S))\ninstance (equipotent S T) gamma_1 delta_2\nend\n"},
    {"a bijection from a set with elements to an empty one names nothing", CK_REJECTED, "con vuoti: T",
     EMPTY_TRAP("(or (equipotent S T) (not (equipotent S T)))")},
    {"with the equipotence as a hypothesis the same entries prove it", CK_ACCEPTED, "", EMPTY_TRAP("(equipotent S T)")},
    {"equipotence in a model: S times T against S times S", CK_ACCEPTED, "",
     "certificate counterexample\nsort S 2\nsort T 1\nstatement (equipotent (S T) (S S))\nend\n"},
    {"equipotence in a model with as many elements on both sides", CK_REJECTED, "non è un controesempio",
     "certificate counterexample\nsort S 2\nsort T 2\nstatement (equipotent (S T) (S S))\nend\n"},

    /* 11h. Gli insiemi di parti e di applicazioni, e le loro leggi. */
    {"parts as objects: S minus a point is injective and misses S", CK_ACCEPTED, "",
     PARTS_HEAD PARTS_EXTENSIONAL PARTS_WHOLE PARTS_CASES},
    {"the same proof without the comprehension of the whole set", CK_REJECTED, "non è comparsa",
     PARTS_HEAD PARTS_EXTENSIONAL PARTS_CASES},
    {"two maps equal at every point are equal: extensionality as an axiom", CK_ACCEPTED, "", MAPS_PROOF(MAPS_LAW)},
    {"a false law of maps: any two maps are equal", CK_REJECTED, "non è una legge",
     MAPS_PROOF("(forall b MSS (forall c MSS (implies (forall d S (= (apMSS b d) (apMSS b d))) (= b c))))")},
    {"a false law of parts: any two parts are equal", CK_REJECTED, "non è una legge",
     PARTS_HEAD "axiom (forall d PS (forall g PS (= d g)))\n" PARTS_CASES},
    {"Russell's part, defined by not belonging to itself, is not a comprehension", CK_REJECTED, "non è una legge",
     PARTS_HEAD "axiom (exists z PS (forall t S (iff (inPS t z) (not (inPS t z)))))\n" PARTS_CASES},
    {"Russell's part would prove that no element equals itself", CK_REJECTED, "non è una legge",
     "certificate structure\nsort S\nsort PS parts S inPS\nstatement (forall s S (not (= s s)))\n"
     "axiom (exists z PS (forall t S (iff (inPS t z) (not (inPS t z)))))\n"
     "case (not (forall s S (not (= s s))))\nwitness (forall s S (not (= s s))) s_1\n"
     "witness (exists z PS (forall t S (iff (inPS t z) (not (inPS t z))))) z_2\n"
     "instance (forall t S (iff (inPS t z_2) (not (inPS t z_2)))) s_1\nend\n"},
    {"a comprehension in a set that is not a set of parts", CK_REJECTED, "non è una legge",
     PARTS_HEAD "axiom (exists z S (forall t S (iff (= t z) true)))\n" PARTS_CASES},
    {"a comprehension whose part is a parameter of its own condition", CK_REJECTED, "non è una legge",
     PARTS_HEAD "axiom (forall q PS (exists z PS (forall t S (iff (inPS t z) (inPS t q)))))\n"
     "axiom (exists z PS (forall t S (iff (inPS t z) (forall w PS (inPS t z)))))\n" PARTS_CASES},
    {"a counterexample computed on the true set of parts", CK_ACCEPTED, "", PARTS_MODEL("1", "")},
    {"a table for the membership, which the checker computes", CK_REJECTED, "due volte", PARTS_MODEL("1", "table inPS 0 1\n")},
    {"a larger powerset still requires a complete function table", CK_REJECTED, "meno valori", PARTS_MODEL("4", "")},
    {"all sixteen subsets of a four-element set are checked", CK_ACCEPTED, "",
     "certificate counterexample\nsort S 4\nsort PS parts S inPS\nsymbol f S PS\n"
     "statement (implies (and (exists s S (= s s)) (forall x S (forall a S (iff (inPS a (f x)) (not (= a x)))))) "
     "(forall y PS (exists x S (= (f x) y))))\ntable f 14 13 11 7\nend\n"},
    {"a set of parts over a set that is not declared", CK_REJECTED, "dichiarati prima",
     "certificate structure\nsort PS parts S inPS\nstatement true\nend\n"},

    /* 11k. La legge delle applicazioni con due valori. */
    {"a map with two values: every value at a point is reached", CK_ACCEPTED, "",
     TWO_VALUES_PROOF(TWO_VALUES("(= x p)", "exists"), TWO_VALUES_INSTANCES)},
    {"the same proof without its instances", CK_REJECTED, "non sono contraddittorie",
     TWO_VALUES_PROOF(TWO_VALUES("(= x p)", "exists"), "")},
    {"the same law written with p = x is not recognized", CK_REJECTED, "non è una legge",
     TWO_VALUES_PROOF(TWO_VALUES("(= p x)", "exists"), "")},
    {"a false law of maps: every map has the two values", CK_REJECTED, "non è una legge",
     TWO_VALUES_PROOF(TWO_VALUES("(= x p)", "forall"), "")},
    {"a false law of maps: a map with two values at once would make any two elements equal", CK_REJECTED, "non è una legge",
     TWO_VALUES_HEAD "statement (implies (exists s S (= s s)) (forall u T (forall v T (= u v))))\naxiom " ONE_VALUE_LAW "\n"
     "case (not (forall u T (forall v T (= u v))))\nwitness (exists s S (= s s)) s_1\nwitness (forall u T (forall v T (= u v))) u_2\n"
     "witness (forall v T (= u_2 v)) v_3\ninstance " ONE_VALUE_LAW " s_1 u_2 v_3\n"
     "witness (exists h MST (forall x S (and (= (apMST h x) u_2) (= (apMST h x) v_3)))) h_4\n"
     "instance (forall x S (and (= (apMST h_4 x) u_2) (= (apMST h_4 x) v_3))) s_1\nend\n"},

    /* 11i. I numeri scritti: G = \{(1, 2), (2, 1)\} è simmetrica, e non
     * transitiva. */
    {"a relation given by its pairs of numbers is symmetric", CK_ACCEPTED, "",
     "certificate example\nsort Num numbers n1 n2\n"
     "statement (forall x Num (forall y Num (implies (or (and (= x n1) (= y n2)) (and (= x n2) (= y n1))) (or (and (= y n1) (= x n2)) (and (= y n2) (= x n1))))))\nend\n"},
    {"the same relation claimed transitive", CK_REJECTED, "non è un esempio",
     "certificate example\nsort Num numbers n1 n2\n"
     "statement (forall x Num (forall y Num (forall z Num (implies (and (or (and (= x n1) (= y n2)) (and (= x n2) (= y n1))) (or (and (= y n1) (= z n2)) (and (= y n2) (= z n1)))) "
     "(or (and (= x n1) (= z n2)) (and (= x n2) (= z n1)))))))\nend\n"},
    {"two numbers made equal by a table", CK_REJECTED, "due volte",
     "certificate example\nsort Num numbers n1 n2\nstatement (= n1 n1)\ntable n2 0\nend\n"},
    {"a set of numbers without numbers", CK_REJECTED, "senza numeri",
     "certificate example\nsort Num numbers\nstatement true\nend\n"},

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

    /* Le parti (11g), scritte dal motore. */
    {"a hypothesis on every part used with two singletons defined by part", CK_ACCEPTED, "", PART_PROOF(SINGLE_X, "b_2", "X_3 Y_4")},
    {"an existence of a part among the hypotheses gives a named part", CK_ACCEPTED, "",
     "certificate structure\nsort S\nsort T\nsymbol f S T\nstatement (implies " PART_EXISTS " " PART_SURJECTIVE ")\n"
     "case (not " PART_SURJECTIVE ")\nwitness " PART_EXISTS " X_1\nwitness " PART_SURJECTIVE " y_2\n"
     "instance (forall a T (exists b S (and (X_1 b) (= (f b) a)))) y_2\nwitness (exists b S (and (X_1 b) (= (f b) y_2))) b_3\n"
     "instance (exists x S (= (f x) y_2)) b_3\nend\n"},
    {"an existence of a part proved with the whole set, even when S is empty", CK_ACCEPTED, "",
     "certificate structure\nsort S\nsort T\nsymbol f S T\nstatement " WHOLE "\ncase (not " WHOLE ")\npart X_1 d S true\n"
     "instance " WHOLE " X_1\nwitness (forall a T (iff (exists b S (and (X_1 b) (= (f b) a))) (exists c S (= (f c) a)))) a_2\n"
     "witness (exists b S (and (X_1 b) (= (f b) a_2))) b_3\ninstance (exists c S (= (f c) a_2)) b_3\n"
     "witness (exists c S (= (f c) a_2)) c_4\ninstance (forall d S (iff (X_1 d) true)) c_4\n"
     "instance (exists b S (and (X_1 b) (= (f b) a_2))) c_4\nend\n"},
    {"an image defined by part is the witness of an existence of a part", CK_ACCEPTED, "",
     "certificate structure\nsort S\nsort T\nsymbol f S T\nsymbol Z S bool\n"
     "statement (implies (forall a S (forall b S (implies (= (f a) (f b)) (= a b)))) (existspart Y T (forall c S (iff (Y (f c)) (Z c)))))\n"
     "case (not (existspart Y T (forall c S (iff (Y (f c)) (Z c)))))\npart Y_1 d T (exists g S (and (Z g) (= (f g) d)))\n"
     "instance (existspart Y T (forall c S (iff (Y (f c)) (Z c)))) Y_1\nwitness (forall c S (iff (Y_1 (f c)) (Z c))) c_2\n"
     "instance (forall d T (iff (Y_1 d) (exists g S (and (Z g) (= (f g) d))))) (f c_2)\n"
     "witness (exists g S (and (Z g) (= (f g) (f c_2)))) g_3\ninstance (forall a S (forall b S (implies (= (f a) (f b)) (= a b)))) g_3 c_2\n"
     "instance (exists g S (and (Z g) (= (f g) (f c_2)))) c_2\nend\n"},

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
    {"a counterexample to a hypothesis on every part: every subset is tried", CK_ACCEPTED, "",
     "certificate counterexample\nsort S 2\nsort T 1\nsymbol f S T\n"
     "statement (implies (forallpart Y T (iff (forall c S (not (Y (f c)))) (forall d T (not (Y d))))) " PART_INJECTIVE ")\ntable f 0 0\nend\n"},
    {"an example where a condition holds for every part", CK_ACCEPTED, "",
     "certificate example\nsort S 2\nsymbol f S S\nstatement (forallpart X S (forall a S (implies (exists b S (and (X b) (= (f b) a))) (X a))))\n"
     "table f 0 1\nend\n"},

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

    /* Parti sbagliate (11g). */
    {"a part defined by the wrong rule: both singletons are {a}", CK_REJECTED, "non sono contraddittorie",
     PART_PROOF("part X_3 j S (= j a_1)\n", "a_1", "X_3 Y_4")},
    {"an instance on parts with a map in place of a part", CK_REJECTED, "parte dell'insieme giusto", PART_PROOF(SINGLE_X, "b_2", "X_3 f")},
    {"an instance on parts with elements in place of parts", CK_REJECTED, "parte dell'insieme giusto", PART_PROOF(SINGLE_X, "b_2", "a_1 b_2")},
    {"an instance on parts with a part of the other set", CK_REJECTED, "parte dell'insieme giusto",
     PART_PROOF(SINGLE_X "part W_9 k T (= k (f a_1))\n", "b_2", "X_3 W_9")},
    {"an instance on parts with one part too many", CK_REJECTED, "di troppo", PART_PROOF(SINGLE_X, "b_2", "X_3 Y_4 Y_4")},
    {"a part named like a declared symbol", CK_REJECTED, "non c'è ancora", PART_PROOF("part f j S (= j b_2)\n", "b_2", "X_3 Y_4")},
    {"a part whose rule names the part itself", CK_REJECTED, "non è dichiarato", PART_PROOF("part X_3 j S (X_3 j)\n", "b_2", "X_3 Y_4")},
    {"a part whose rule has a variable that is not bound", CK_REJECTED, "non è dichiarato", PART_PROOF("part X_3 j S (= j k)\n", "b_2", "X_3 Y_4")},
    {"a part of a set that is not declared", CK_REJECTED, "insieme dichiarato", PART_PROOF("part X_3 j V (= j b_2)\n", "b_2", "X_3 Y_4")},
    {"a singleton of an element invented for a set that may be empty", CK_REJECTED, "con vuoti: S",
     "certificate structure\nsort S\nstatement (implies " TWO_PARTS " (exists x S (= x x)))\ncase (not (implies " TWO_PARTS " (exists x S (= x x))))\n"
     "witness (exists x S (= x x)) c\npart D y S (= y c)\ninstance " TWO_PARTS " D\ninstance (exists x S (= x x)) c\nend\n"},
    {"a named part for a statement on every part that is not in the case", CK_REJECTED, "non è comparsa",
     PART_HEAD "witness (forallpart X S (forall h S (X h))) X_3\nend\n"},
    {"a bound part written where an element is needed", CK_REJECTED, "dove serve un elemento",
     "certificate structure\nsort S\nstatement (forallpart X S (exists y S (= y X)))\nend\n"},
    {"a part entry before any case", CK_REJECTED, "fuori posto",
     "certificate structure\nsort S\nstatement true\npart D x S true\nend\n"},

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
    {"a model where the hypothesis on every part fails is no counterexample", CK_REJECTED, "non è un controesempio",
     "certificate counterexample\nsort S 2\nsort T 2\nsymbol f S T\n"
     "statement (implies (forallpart Y T (iff (forall c S (not (Y (f c)))) (forall d T (not (Y d))))) " PART_INJECTIVE ")\ntable f 0 0\nend\n"},
    {"an example that fails on the empty part", CK_REJECTED, "non è un esempio",
     "certificate example\nsort S 2\nsymbol f S S\nstatement (forallpart X S (not (forall a S (iff (exists b S (and (X b) (= (f b) a))) (X a)))))\n"
     "table f 1 0\nend\n"},

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

    /* Risorse e dichiarazioni: anche varianti grandi ma controllabili. */
    {"a larger set does not make a false counterexample valid", CK_REJECTED, "nel modello",
     "certificate counterexample\nsort S 9\nsymbol u S\nstatement (= u u)\ntable u 0\nend\n"},
    {"seven declared sets have no compiled declaration ceiling", CK_ACCEPTED, "",
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
