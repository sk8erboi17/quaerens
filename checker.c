/* =============================================================================
 * checker: verificatore indipendente dei certificati di mathresearcher
 * =============================================================================
 *
 * COSA FA
 *
 * mathresearcher (main.c) cerca le dimostrazioni. Quando ne trova una per
 * induzione scrive un certificato: l'enunciato e un solo dato in più, che
 * basta a ricontrollare il passo induttivo senza rifare la ricerca. Per un
 * enunciato sugli insiemi il certificato è il solo enunciato, e questo
 * programma lo decide da capo (capitolo 10). Per un enunciato su insiemi
 * qualsiasi, con operazioni, applicazioni e relazioni di cui si sa solo ciò
 * che dicono le ipotesi, il certificato è un modello finito, su cui si
 * calcola, oppure i nomi e le istanze di una dimostrazione per assurdo, di
 * cui si controlla che portino a una contraddizione (capitolo 11). Per una
 * disequazione, un'equazione o un insieme di soluzioni in una variabile
 * reale il certificato è la tabella dei segni, di cui si ricontrolla ogni
 * riga (capitolo 12). Per una derivata, una primitiva o un integrale
 * definito il certificato porta le espressioni, e questo programma deriva da
 * sé, con regole sue (capitolo 13). Per un limite il certificato è
 * l'enunciato con la sostituzione e l'ordine degli sviluppi in serie, che
 * questo programma rifà da sé (capitolo 14). Il resto di questo commento
 * riguarda le induzioni; i teoremi degli altri tipi sono più sotto. Questo
 * programma legge il certificato e risponde:
 *
 *   - ACCETTATO: tutti i controlli sono riusciti;
 *   - RESPINTO: un controllo è fallito, il certificato non dimostra nulla;
 *   - NON CONTROLLABILE: il certificato usa qualcosa che questo programma non
 *     conosce, o i numeri sono troppo grandi. Non è un sì.
 *
 * Trovare una dimostrazione è difficile, controllarla è facile: qui non c'è
 * nessuna ricerca, solo somme, prodotti e divisioni di polinomi, catene di
 * Sturm e il confronto di due risultati. Per questo il programma è piccolo,
 * e lo si può leggere tutto.
 *
 * PERCHÉ È UN FILE A PARTE
 *
 * Chi controlla non deve condividere codice con chi ha prodotto: un errore
 * nell'aritmetica di main.c, usata da tutti e due, li farebbe sbagliare
 * nello stesso modo. Questo file non include main.c e non ne usa nulla; ha i
 * suoi numeri, i suoi polinomi, il suo lettore. Che in main.c ci sia
 * un'altra somma di polinomi è voluto. main.c include questo file, per
 * chiamare ck_check(), e mai il contrario: tutti i nomi qui cominciano con
 * ck_ (Ck per i tipi, CK_ per le costanti), e il file si compila anche da
 * solo.
 *
 * IL CERTIFICATO
 *
 * È un testo, una riga per voce:
 *
 *     certificate induction
 *     variable n
 *     base 1
 *     statement (= (sum k 1 n k) (/ (* n (+ n 1)) 2))
 *     factor 1
 *     end
 *
 * Le espressioni sono scritte con l'operatore davanti e tutte le parentesi,
 * così leggerle non richiede regole di precedenza (capitolo 6). Le voci
 * facoltative sono "sequence" e "define", che definiscono una successione
 * (per ricorrenza, o con una formula), "generic" e "defined", descritte più
 * sotto, e "brick", che suggerisce un fattore in cui scomporre gli argomenti
 * dei logaritmi (capitolo 8, 8g): un suggerimento non può far accettare il
 * falso, perché ogni scomposizione usata è controllata con una divisione.
 *
 * IL TEOREMA CHE UN CERTIFICATO ACCETTATO DIMOSTRA
 *
 * Sia L = R l'enunciato, n la variabile, b il caso base. Diciamo che
 * l'enunciato è definito per un valore di n (e dei parametri) se, nel
 * calcolo dei due membri,
 *
 *   - nessuna divisione ha il divisore nullo (una potenza con esponente
 *     negativo è una divisione),
 *   - nessuna potenza con l'esponente che dipende da n ha la base nulla, e
 *   - ogni logaritmo (ln a) ha l'argomento positivo; per (lnabs a), il
 *     logaritmo del valore assoluto, basta che l'argomento non sia zero.
 *
 * Il teorema:
 *
 *     Per ogni intero n >= b e per ogni valore dei parametri tali che
 *       (a) l'enunciato è definito per tutti i valori da b a n, e
 *       (b) ogni espressione "generic" è definita e diversa da zero,
 *     vale L = R.
 *
 * Senza la voce "defined" il certificato NON dice che l'enunciato è
 * definito: quella è un'ipotesi del teorema, e resta da stabilire a chi lo
 * usa. (n - 50)/(n - 50) = 1 ha un certificato valido: dice che i due membri
 * sono uguali per n da 1 a 49. Da 50 in su il teorema non dice più nulla,
 * perché in 50 l'enunciato non è definito.
 *
 * LA VOCE "defined"
 *
 *     defined x 2 y (/ 1 2)
 *
 * afferma in più: con i parametri a questi valori (x = 2, y = 1/2; senza
 * parametri la voce è la sola parola) l'enunciato è definito per ogni
 * intero n >= b, e le espressioni "generic" sono definite e diverse da zero.
 * Se il certificato è accettato, l'ipotesi del teorema è dunque soddisfatta
 * in quel punto, e ne segue un enunciato senza riserve:
 *
 *     Con i parametri a quei valori, per ogni intero n >= b i due membri
 *     sono definiti e uguali.
 *
 * Per un enunciato senza parametri è tutto ciò che c'è da dire. Con i
 * parametri dice che il teorema non è vuoto: c'è almeno un punto in cui vale
 * per ogni n. Come lo si controlla è nel capitolo 9. Se un divisore si
 * annulla per qualche n il certificato è respinto; se le regole del capitolo
 * 9 non bastano a decidere, non è controllabile, e chi l'ha scritto può
 * riproporlo senza la voce.
 *
 * COME LO SI CONTROLLA
 *
 * Sia D(n) = L - R. Scritta come frazione, D(n) = N0/D0; con n + 1 al posto
 * di n, D(n+1) = N1/D1. Il certificato porta una frazione W = Wn/Wd, il
 * "fattore", e afferma che D(n+1) = W D(n). I controlli sono tre.
 *
 *   1. Caso base: con n = b il numeratore di L - R è il polinomio nullo.
 *   2. Passo: N1 D0 Wd = Wn N0 D1, come polinomi. È D(n+1) = W D(n) senza
 *      denominatori: si sviluppano i due prodotti e si confrontano.
 *   3. Wd non può annullarsi dove l'enunciato è definito: Wd divide una
 *      potenza di D0 D1 G, dove G è il prodotto delle espressioni "generic"
 *      e delle potenze di numeri primi, come 2^n, che non valgono mai zero.
 *
 * Perché bastano. Si fissi n >= b con l'enunciato definito da b a n + 1 e
 * vero in n: allora N0 = 0, mentre D0 e D1 non sono zero, e quindi nemmeno
 * Wd lo è (controllo 3). Dal controllo 2, N1 D0 Wd = 0, cioè N1 = 0:
 * l'enunciato vale in n + 1. Con il caso base, è il principio di induzione.
 *
 * Il controllo 3 è ciò che impedisce il trucco più semplice: prendere
 * W = D(n+1)/D(n), che soddisfa sempre il controllo 2 ma ha a denominatore
 * proprio la quantità che l'ipotesi induttiva dice nulla.
 *
 * Con i logaritmi la voce "defined" non è ancora controllabile: servirebbe
 * stabilire il segno degli argomenti.
 *
 * Le somme e le potenze. Una somma fino a n non ha una formula, ma non
 * serve: la si tratta come una lettera X, e la somma fino a n + 1 è X più il
 * termine nuovo. Lo stesso per un prodotto, per 2^n (con 2^{n+1} = 2 2^n) e
 * per i termini di una ricorrenza. Un'uguaglianza tra polinomi in cui X è
 * una lettera qualsiasi vale in particolare quando X è la somma vera: per
 * questo trattare queste quantità come indipendenti non può far accettare
 * il falso. Può far respingere il vero, se la dimostrazione usa un legame
 * tra loro che qui non si vede.
 *
 * I TEOREMI DEGLI ALTRI CERTIFICATI
 *
 *   - "certificate sets" (capitolo 10): l'enunciato vale per ogni scelta
 *     degli insiemi indicati dalle lettere.
 *   - "certificate counterexample" (capitolo 11, 11a): l'enunciato su insiemi
 *     qualsiasi è falso nell'interpretazione finita data dalle tabelle; non
 *     vale quindi per ogni scelta degli insiemi e dei simboli.
 *   - "certificate example" (11b): esistono degli insiemi, con il numero di
 *     elementi indicato, e dei simboli per cui l'enunciato vale.
 *   - "certificate structure" (11c): l'enunciato vale per ogni scelta degli
 *     insiemi, finiti o infiniti, anche vuoti, e di un'interpretazione dei
 *     simboli dichiarati (un elemento per un elemento con un nome, una
 *     funzione per un'applicazione, un sottoinsieme per una parte o una
 *     relazione). Nei tre teoremi un quantificatore su parti dell'enunciato,
 *     forallpart o existspart (11g), vuol dire "per ogni" o "esiste" un
 *     sottoinsieme qualsiasi dell'insieme, vuoto e insieme intero compresi.
 *   - "certificate signs" (capitolo 12): con C la condizione nella variabile
 *     x, e un confronto che vale in x solo dove i suoi due membri sono
 *     definiti, "per ogni x in D vale C", "esiste x in D in cui C non vale",
 *     "esiste x in D in cui vale C" o "per nessun x in D vale C", con D i
 *     reali o i razionali, secondo ciò che il certificato afferma; oppure
 *     "per ogni x reale, C(x) vale se e solo se valgono le soluzioni scritte
 *     W(x)". Il teorema esatto è in testa al capitolo 12.
 *   - "certificate derivative" (capitolo 13): per ogni valore dei parametri,
 *     in ogni x in cui la funzione F è regolare (definita, con i radicandi
 *     diversi da zero) e G è definita, F è derivabile k volte e
 *     F^(k)(x) = G(x); per una derivata n-esima, per ogni intero n >= n0,
 *     dove F e G(n0, x), ..., G(n, x) sono regolari.
 *   - "certificate antiderivative": su ogni intervallo aperto in cui la
 *     primitiva F è regolare e f è definita, le primitive di f sono le F + c.
 *   - "certificate integral": f è continua su [a, b] e il suo integrale da
 *     a a b vale V. I teoremi esatti sono in testa al capitolo 13.
 *   - "certificate limit" (capitolo 14): da ogni lato del punto la funzione,
 *     preso il suo dominio naturale, o non ha valori in un intorno bucato, o
 *     li ha tutti e ha un limite; e il limite nel punto, dentro il dominio,
 *     è (o non è, o non esiste) ciò che il certificato afferma. Il teorema
 *     esatto, con ciò che si cita senza dimostrarlo (il teorema di Taylor con
 *     il resto di Peano, la scala delle crescite), è in testa al capitolo 14.
 *
 * COMPILAZIONE
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o checker checker.c
 *     ./checker certificati.cert
 *
 * Servono GCC o Clang: i numeri sono interi a 128 bit.
 *
 * INDICE DEI CAPITOLI
 *
 *    1. Include, limiti, esiti
 *    2. Memoria
 *    3. Numeri razionali
 *    4. Polinomi
 *    5. Frazioni di polinomi
 *    6. Espressioni: albero e lettura
 *    7. Indeterminate
 *    8. Valutazione
 *    9. Definito per ogni n
 *   10. Enunciati sugli insiemi
 *   11. Insiemi qualsiasi: modelli finiti e dimostrazioni per assurdo
 *   12. Tabelle dei segni
 *   13. Derivate e primitive
 *   14. Limiti
 *   15. Controllo di un certificato
 *   16. main
 */

/* =============================================================================
 * 1. INCLUDE, LIMITI, ESITI
 * ============================================================================
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CK_ARENA_BYTES ((size_t)48 << 20)  /* memoria per un certificato */
#define CK_MAX_VARS 24       /* indeterminate distinte */
#define CK_MAX_TERMS 40000   /* termini di un polinomio */
#define CK_MAX_EXPONENT 100  /* esponente di un'indeterminata in un monomio */
#define CK_MAX_UNROLL 4096   /* termini di una somma svolta con estremi numerici */
#define CK_MAX_EXTRA 64      /* termini aggiunti a una somma oltre il riferimento */
#define CK_MAX_POWER 512     /* esponente intero costante */
#define CK_MAX_ENV 16        /* indici di somma legati uno dentro l'altro */
#define CK_MAX_GENERIC 8     /* voci "generic" di un certificato */
#define CK_MAX_SEQUENCES 4   /* voci "sequence" e "define" di un certificato */
#define CK_MAX_ORDER 4       /* ordine di una ricorrenza */
#define CK_MAX_NONZERO 4     /* potenza di D0 D1 G provata nel controllo 3 */
#define CK_MAX_DEPTH 4000    /* parentesi una dentro l'altra in un'espressione */
#define CK_MAX_BASE 1000000000LL  /* valore assoluto del caso base */
#define CK_SPARE_BYTES 256   /* blocco di riserva di ck_alloc() */
#define CK_MAX_POINT 8       /* parametri a cui la voce "defined" dà un valore */
#define CK_MAX_DIVISORS 256  /* divisori di cui stabilire che non si annullano */
#define CK_MAX_SCAN 100000   /* valori di n provati uno a uno per un polinomio */
#define CK_MAX_BRICKS 32     /* voci "brick" di un certificato */
#define CK_MAX_FACTORED 1000000000000LL  /* il più grande numero scomposto in fattori primi */
#define CK_MAX_NAME 16       /* caratteri di un nome nel capitolo 11, compreso lo zero finale */
#define CK_MAX_SORTS 6       /* insiemi di un enunciato su insiemi qualsiasi: i casi vuoti sono 2^6 */
#define CK_MAX_SYMBOLS 256   /* simboli dichiarati, più i nomi delle voci witness di un caso */
#define CK_MAX_SIZE 8        /* elementi di un insieme in un modello */
#define CK_MAX_BOUND 64      /* variabili legate una dentro l'altra */
#define CK_MAX_EVAL 100000000LL  /* elementi provati dai quantificatori calcolando su un modello */
#define CK_MAX_GROUND 4096   /* termini chiusi di un caso */
#define CK_MAX_ATOMS 2048    /* condizioni elementari di un caso */
#define CK_MAX_FORMULAS 2048 /* condizioni supposte vere in un caso */
#define CK_MAX_CASES 64      /* voci case di un certificato */
#define CK_MAX_SEARCH 200000 /* scelte parziali provate cercando un'assegnazione coerente, per certificato */
#define CK_MAX_DEGREE 48     /* grado di un polinomio in una variabile, nel capitolo 12 */
#define CK_MAX_FACTORS 128   /* voci factor di una tabella dei segni */
#define CK_MAX_POINTS 200    /* punti di una tabella dei segni: le righe sono 2 CK_MAX_POINTS + 1 */
#define CK_MAX_ABS 4         /* valori assoluti distinti in un confronto: le scelte dei segni sono 2^4 */
#define CK_MAX_SEEN 1024     /* polinomi annotati calcolando un confronto */
#define CK_MAX_TRIED 1000000 /* razionali provati come radici di un fattore, per una riga (12g) */
#define CK_FLAT 1000000      /* esponente del resto di una funzione più piccola di ogni potenza (14c) */
#define CK_MAX_SERIES 48     /* coefficienti noti di una serie (14c) */
#define CK_MAX_SERIES_DEN 24 /* denominatore degli esponenti di una serie */
#define CK_MAX_TAYLOR 20     /* termini di una serie di Taylor (14d) */
#define CK_MAX_GOAL 64       /* l'ordine di un lato di un limite */

/* La risposta su un certificato. */
typedef enum { CK_ACCEPTED, CK_REJECTED, CK_UNSUPPORTED } CkAnswer;

/* L'esito del controllo in corso, e il perché in italiano. Il primo problema
 * incontrato resta: dopo, i conti proseguono con valori qualsiasi e i loro
 * risultati non contano, così le funzioni non devono propagare un codice. */
static CkAnswer ck_status;
static const char *ck_reason;

/* Annota che il certificato è sbagliato. */
static void ck_reject(const char *reason) {
    if (ck_status != CK_ACCEPTED) return;
    ck_status = CK_REJECTED;
    ck_reason = reason;
}

/* Annota che il certificato non si sa controllare. */
static void ck_unsupported(const char *reason) {
    if (ck_status != CK_ACCEPTED) return;
    ck_status = CK_UNSUPPORTED;
    ck_reason = reason;
}

/* =============================================================================
 * 2. MEMORIA
 * ============================================================================
 *
 * Un'arena: i blocchi si prendono uno dopo l'altro e si restituiscono tutti
 * insieme, all'inizio del certificato successivo.
 */

static _Alignas(16) char ck_arena[CK_ARENA_BYTES];
static size_t ck_used;

/* Ritorna un blocco di bytes byte allineato a 16, oppure, se l'arena è
 * piena, un blocco di riserva di CK_SPARE_BYTES byte dopo aver annotato il
 * problema: chi chiama può scriverci un elemento solo (un termine, un nodo)
 * senza controllare. Chi ne chiede di più deve guardare ck_status prima di
 * scrivere. */
static void *ck_alloc(size_t bytes) {
    static _Alignas(16) char spare[CK_SPARE_BYTES];
    bytes = (bytes + 15) & ~(size_t)15;
    if (bytes > CK_ARENA_BYTES - ck_used) {
        ck_unsupported("memoria del verificatore esaurita");
        return spare;
    }
    void *block = ck_arena + ck_used;
    ck_used += bytes;
    return block;
}

/* =============================================================================
 * 3. NUMERI RAZIONALI
 * ============================================================================
 *
 * Frazioni esatte di interi a 128 bit. Ogni operazione controlla di non
 * uscire dai 128 bit: se succede il certificato non si sa controllare, e
 * nessun risultato arrotondato entra mai in un confronto.
 */

/* Il più grande valore assoluto ammesso per numeratore e denominatore. Sta
 * sotto il massimo dei 128 bit, così cambiare segno a un numero non può mai
 * uscirne. */
#define CK_NUM_LIMIT ((__int128)1 << 126)

/* Il razionale num/den. Invariante: den > 0 e la frazione è ridotta. */
typedef struct {
    __int128 num, den;
} CkNum;

/* Ritorna il massimo comun divisore di |a| e |b| (algoritmo di Euclide). */
static __int128 ck_gcd(__int128 a, __int128 b) {
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b) {
        __int128 r = a % b;
        a = b;
        b = r;
    }
    return a;
}

/* Ritorna num/den ridotta ai minimi termini, con den positivo. */
static CkNum ck_num_make(__int128 num, __int128 den) {
    if (num > CK_NUM_LIMIT || num < -CK_NUM_LIMIT || den > CK_NUM_LIMIT) {
        ck_unsupported("numeri troppo grandi per il verificatore");
        return (CkNum){0, 1};
    }
    __int128 g = ck_gcd(num, den);
    return (CkNum){num / g, den / g};
}

/* Ritorna l'intero v. */
static CkNum ck_num_int(long long v) { return (CkNum){v, 1}; }

/* Ritorna a + b. */
static CkNum ck_num_add(CkNum a, CkNum b) {
    __int128 x, y, den;
    if (__builtin_mul_overflow(a.num, b.den, &x) || __builtin_mul_overflow(b.num, a.den, &y) ||
        __builtin_add_overflow(x, y, &x) || __builtin_mul_overflow(a.den, b.den, &den)) {
        ck_unsupported("numeri troppo grandi per il verificatore");
        return ck_num_int(0);
    }
    return ck_num_make(x, den);
}

/* Ritorna a * b. Si semplifica in croce prima di moltiplicare, così il
 * risultato sta nei 128 bit ogni volta che la frazione ridotta ci sta. I due
 * divisori comuni non sono mai zero, perché i denominatori sono positivi. */
static CkNum ck_num_mul(CkNum a, CkNum b) {
    __int128 g = ck_gcd(a.num, b.den), h = ck_gcd(b.num, a.den), num, den;
    if (__builtin_mul_overflow(a.num / g, b.num / h, &num) || __builtin_mul_overflow(a.den / h, b.den / g, &den)) {
        ck_unsupported("numeri troppo grandi per il verificatore");
        return ck_num_int(0);
    }
    return ck_num_make(num, den);
}

/* Mette in *out a b se il prodotto sta nei numeri del verificatore e ritorna
 * 1; altrimenti ritorna 0, senza annotare nulla. Serve dove uscire dai
 * numeri vuol dire solo fermarsi prima (14b, 14d). */
static int ck_num_mul_fits(CkNum a, CkNum b, CkNum *out) {
    __int128 g = ck_gcd(a.num, b.den), h = ck_gcd(b.num, a.den), num, den;
    if (__builtin_mul_overflow(a.num / g, b.num / h, &num) || __builtin_mul_overflow(a.den / h, b.den / g, &den)) return 0;
    if (num > CK_NUM_LIMIT || num < -CK_NUM_LIMIT || den > CK_NUM_LIMIT) return 0;
    *out = ck_num_make(num, den);
    return 1;
}

/* Ritorna -a. */
static CkNum ck_num_neg(CkNum a) { return (CkNum){-a.num, a.den}; }

/* Ritorna 1/a, con a diverso da zero. */
static CkNum ck_num_inv(CkNum a) { return a.num < 0 ? (CkNum){-a.den, -a.num} : (CkNum){a.den, a.num}; }

/* =============================================================================
 * 4. POLINOMI
 * ============================================================================
 *
 * Polinomi a coefficienti razionali in al più CK_MAX_VARS indeterminate
 * (capitolo 7). Un polinomio è l'elenco dei suoi termini in ordine, senza
 * monomi ripetuti e senza coefficienti nulli: due polinomi sono uguali
 * esattamente quando hanno lo stesso elenco. È l'unica forma di "sono
 * uguali?" di tutto il verificatore.
 *
 * Una sola indeterminata è speciale: s = (-1)^n, per cui vale s^2 = 1. La
 * regola è applicata nel prodotto, tenendo l'esponente di s tra 0 e 1.
 */

/* Un termine: coefficiente per il monomio in cui l'indeterminata v ha
 * esponente e[v], mai negativo. */
typedef struct {
    CkNum coef;
    signed char e[CK_MAX_VARS];
} CkTerm;

/* Un polinomio. Invariante: i termini sono in ordine crescente di monomio
 * (confronto degli esponenti, dalla prima indeterminata), distinti e con
 * coefficiente non nullo. count == 0 è il polinomio nullo. I termini stanno
 * nell'arena. */
typedef struct {
    int count;
    CkTerm *term;
} CkPoly;

_Static_assert(sizeof(CkTerm) <= CK_SPARE_BYTES, "il blocco di riserva di ck_alloc() deve contenere un termine");

/* L'indeterminata che vale (-1)^n, o -1 se il certificato non la usa. */
static int ck_sign_var;

/* Confronta i monomi di due termini: negativo, zero o positivo. */
static int ck_term_cmp(const void *a, const void *b) {
    return memcmp(((const CkTerm *)a)->e, ((const CkTerm *)b)->e, CK_MAX_VARS);
}

/* Ritorna il polinomio dei `count` termini di t, messi in ordine e sommati
 * dove il monomio coincide. Usa t stesso come memoria del risultato. */
static CkPoly ck_poly_build(CkTerm *t, int count) {
    int kept = 0;
    qsort(t, (size_t)count, sizeof *t, ck_term_cmp);
    for (int i = 0; i < count; i++) {
        if (kept > 0 && ck_term_cmp(&t[kept - 1], &t[i]) == 0) t[kept - 1].coef = ck_num_add(t[kept - 1].coef, t[i].coef);
        else t[kept++] = t[i];
        if (t[kept - 1].coef.num == 0) kept--;
    }
    return (CkPoly){kept, t};
}

/* Ritorna il polinomio costante c. */
static CkPoly ck_poly_number(CkNum c) {
    CkTerm *t = ck_alloc(sizeof *t);
    memset(t, 0, sizeof *t);
    t->coef = c;
    return (CkPoly){c.num != 0, t};
}

/* Ritorna il polinomio fatto della sola indeterminata v. */
static CkPoly ck_poly_variable(int v) {
    CkPoly p = ck_poly_number(ck_num_int(1));
    p.term[0].e[v] = 1;
    return p;
}

/* Ritorna a + b, oppure a - b se minus è diverso da zero. */
static CkPoly ck_poly_add(CkPoly a, CkPoly b, int minus) {
    CkTerm *t = ck_alloc(sizeof *t * (size_t)(a.count + b.count + 1));
    if (ck_status != CK_ACCEPTED) return (CkPoly){0, t};
    memcpy(t, a.term, sizeof *t * (size_t)a.count);
    memcpy(t + a.count, b.term, sizeof *t * (size_t)b.count);
    for (int i = 0; minus && i < b.count; i++) t[a.count + i].coef = ck_num_neg(t[a.count + i].coef);
    return ck_poly_build(t, a.count + b.count);
}

/* Ritorna a * b: ogni termine dell'uno per ogni termine dell'altro. */
static CkPoly ck_poly_mul(CkPoly a, CkPoly b) {
    long long size = (long long)a.count * b.count;
    if (size > CK_MAX_TERMS) { ck_unsupported("polinomi troppo grandi per il verificatore"); size = 0; }
    CkTerm *t = ck_alloc(sizeof *t * (size_t)(size + 1));
    if (ck_status != CK_ACCEPTED) return (CkPoly){0, t};
    int count = 0;
    for (int i = 0; i < a.count; i++)
        for (int j = 0; j < b.count; j++) {
            CkTerm *out = &t[count++];
            out->coef = ck_num_mul(a.term[i].coef, b.term[j].coef);
            for (int v = 0; v < CK_MAX_VARS; v++) {
                int e = a.term[i].e[v] + b.term[j].e[v];
                if (v == ck_sign_var) e %= 2;
                if (e > CK_MAX_EXPONENT) { ck_unsupported("esponenti troppo grandi per il verificatore"); e = 0; }
                out->e[v] = (signed char)e;
            }
        }
    return ck_poly_build(t, count);
}

/* Ritorna 1 se a e b sono lo stesso polinomio. */
static int ck_poly_equal(CkPoly a, CkPoly b) {
    if (a.count != b.count) return 0;
    for (int i = 0; i < a.count; i++)
        if (ck_term_cmp(&a.term[i], &b.term[i]) != 0 || a.term[i].coef.num != b.term[i].coef.num || a.term[i].coef.den != b.term[i].coef.den)
            return 0;
    return 1;
}

/* Ritorna 1 se p è una costante, zero compreso. */
static int ck_poly_is_constant(CkPoly p) {
    static const signed char none[CK_MAX_VARS];
    return p.count == 0 || (p.count == 1 && memcmp(p.term[0].e, none, CK_MAX_VARS) == 0);
}

/* Ritorna 1 se l'indeterminata v compare in p. */
static int ck_poly_uses(CkPoly p, int v) {
    for (int i = 0; i < p.count; i++)
        if (p.term[i].e[v]) return 1;
    return 0;
}

/* Ritorna p con il numero sign, che è 1 o -1, al posto dell'indeterminata
 * s = (-1)^n. Se il certificato non usa s ritorna p. */
static CkPoly ck_poly_at_sign(CkPoly p, int sign) {
    if (ck_sign_var < 0) return p;
    CkTerm *t = ck_alloc(sizeof *t * (size_t)(p.count + 1));
    if (ck_status != CK_ACCEPTED) return (CkPoly){0, t};
    memcpy(t, p.term, sizeof *t * (size_t)p.count);
    for (int i = 0; i < p.count; i++) {
        if (t[i].e[ck_sign_var] && sign < 0) t[i].coef = ck_num_neg(t[i].coef);
        t[i].e[ck_sign_var] = 0;
    }
    return ck_poly_build(t, p.count);
}

/* Ritorna 1 se b divide a esattamente, cioè se esiste un polinomio q con
 * a = b q, e allora mette q in *result se result non è NULL. b non è il
 * polinomio nullo. Con l'indeterminata s (s^2 = 1) in uno dei due la
 * risposta "divide" resta certa, ma "non divide" può essere solo prudente.
 *
 * Divisione dei polinomi in più indeterminate: l'ultimo termine di un
 * polinomio, nell'ordine dei monomi, è il suo termine direttivo, e il termine
 * direttivo di un prodotto è il prodotto dei termini direttivi. Finché il
 * resto non è zero, il suo termine direttivo deve essere un multiplo di
 * quello di b: lo si toglie sottraendo il multiplo giusto di b, e si
 * continua. Se a un certo punto non è un multiplo, b non divide a.
 *
 * Senza s, a ogni passo il termine direttivo del resto scende e il ciclo
 * finisce; con s la regola sui termini direttivi può cadere, e per questo i
 * passi hanno un tetto. Il quoziente viene comunque ricontrollato
 * moltiplicando: così la risposta "divide" non dipende dal ragionamento sul
 * ciclo, solo da un prodotto e da un confronto. */
static int ck_poly_divides(CkPoly b, CkPoly a, CkPoly *result) {
    CkPoly rest = a, quotient = ck_poly_number(ck_num_int(0));
    const CkTerm *lead = &b.term[b.count - 1];
    for (int step = 0; rest.count > 0 && ck_status == CK_ACCEPTED; step++) {
        CkTerm *piece = ck_alloc(sizeof *piece);
        const CkTerm *top = &rest.term[rest.count - 1];
        if (step > CK_MAX_TERMS) return 0;
        for (int v = 0; v < CK_MAX_VARS; v++) {
            if (top->e[v] < lead->e[v]) return 0;
            piece->e[v] = (signed char)(top->e[v] - lead->e[v]);
        }
        piece->coef = ck_num_mul(top->coef, ck_num_inv(lead->coef));
        CkPoly single = {1, piece};
        quotient = ck_poly_add(quotient, single, 0);
        rest = ck_poly_add(rest, ck_poly_mul(single, b), 1);
    }
    if (ck_status != CK_ACCEPTED || !ck_poly_equal(ck_poly_mul(b, quotient), a)) return 0;
    if (result) *result = quotient;
    return 1;
}

/* =============================================================================
 * 5. FRAZIONI DI POLINOMI
 * ============================================================================
 *
 * Il valore di un'espressione è una frazione num/den. Le frazioni non si
 * semplificano mai: il denominatore è il prodotto di tutti i divisori
 * incontrati nel conto. È proprio ciò che serve al controllo 3: dove
 * l'espressione è definita ogni divisore è diverso da zero, quindi lo è
 * anche il denominatore.
 */

/* La frazione num/den. Invariante: den non è il polinomio nullo. */
typedef struct {
    CkPoly num, den;
} CkRat;

/* Un divisore incontrato nel conto: il numeratore della frazione per cui si
 * è diviso, e il più piccolo n per cui quella divisione viene fatta. */
typedef struct {
    CkPoly value;
    long long from;
} CkDivisor;

/* I divisori annotati mentre ck_recording è diverso da zero; ck_record_from
 * è il valore di `from` da dare ai prossimi. Servono a stabilire che
 * l'enunciato è definito per ogni n (capitolo 9). */
static CkDivisor ck_divisors[CK_MAX_DIVISORS];
static int ck_divisor_count;
static int ck_recording;
static long long ck_record_from;

/* Annota un divisore. Una costante non si annota: se è zero è già un errore
 * per ck_rat_div(), se non lo è non c'è nulla da stabilire. */
static void ck_record(CkPoly divisor) {
    if (!ck_recording || ck_poly_is_constant(divisor)) return;
    if (ck_divisor_count == CK_MAX_DIVISORS) { ck_unsupported("troppi divisori di cui stabilire che non si annullano"); return; }
    ck_divisors[ck_divisor_count++] = (CkDivisor){divisor, ck_record_from};
}

/* Come vale la variabile di induzione in questo momento (capitolo 8). Sta
 * qui perché una divisione per zero si spiega in modo diverso nel caso base. */
static char ck_variable;   /* il suo nome */
static int ck_numeric;     /* 1: vale il numero ck_point; 0: vale n + ck_shift */
static long long ck_point;
static long long ck_shift;

/* 1 mentre si calcola un certificato del capitolo 13, in cui c'è anche una
 * variabile reale (e ck_variable, se c'è, è l'ordine di una derivata). */
static int ck_real;

/* Ritorna la frazione che vale il polinomio p. */
static CkRat ck_rat_poly(CkPoly p) { return (CkRat){p, ck_poly_number(ck_num_int(1))}; }

/* Ritorna la frazione che vale l'intero v. */
static CkRat ck_rat_int(long long v) { return ck_rat_poly(ck_poly_number(ck_num_int(v))); }

/* Ritorna a + b, oppure a - b se minus è diverso da zero. Con lo stesso
 * denominatore si sommano i numeratori: tiene piccoli i polinomi, e il
 * denominatore resta un prodotto di divisori. */
static CkRat ck_rat_add(CkRat a, CkRat b, int minus) {
    if (ck_poly_equal(a.den, b.den)) return (CkRat){ck_poly_add(a.num, b.num, minus), a.den};
    return (CkRat){ck_poly_add(ck_poly_mul(a.num, b.den), ck_poly_mul(b.num, a.den), minus), ck_poly_mul(a.den, b.den)};
}

/* Ritorna a * b. */
static CkRat ck_rat_mul(CkRat a, CkRat b) { return (CkRat){ck_poly_mul(a.num, b.num), ck_poly_mul(a.den, b.den)}; }

/* Ritorna a / b. Dividere per un'espressione identicamente nulla non ha
 * senso per nessun valore: il certificato è sbagliato. Il nuovo denominatore
 * contiene il numeratore di b, cioè il divisore. */
static CkRat ck_rat_div(CkRat a, CkRat b) {
    if (b.num.count == 0) {
        ck_reject(ck_real        ? "il certificato divide per un'espressione che vale zero"
                  : ck_recording ? "con i valori dati ai parametri l'enunciato divide per zero"
                  : ck_numeric   ? "nel caso base l'enunciato divide per zero"
                                 : "l'enunciato divide per un'espressione che vale sempre zero");
        return ck_rat_int(0);
    }
    ck_record(b.num);
    return (CkRat){ck_poly_mul(a.num, b.den), ck_poly_mul(a.den, b.num)};
}

/* Ritorna a^e per e intero, anche negativo. */
static CkRat ck_rat_power(CkRat a, long long e) {
    CkRat result = ck_rat_int(1);
    if (e > CK_MAX_POWER || e < -CK_MAX_POWER) { ck_unsupported("esponente troppo grande per il verificatore"); return result; }
    if (e < 0) { a = ck_rat_div(ck_rat_int(1), a); e = -e; }
    for (long long i = 0; i < e && ck_status == CK_ACCEPTED; i++) result = ck_rat_mul(result, a);
    return result;
}

/* =============================================================================
 * 6. ESPRESSIONI: ALBERO E LETTURA
 * ============================================================================
 *
 * La notazione del certificato:
 *
 *     espr := intero | lettera | '(' operatore argomenti ')'
 *
 *     (+ a b)  (- a b)  (* a b)  (/ a b)  (neg a)  (^ a b)
 *     (sum k a b f)     somma di f per k da a a b
 *     (prod k a b f)    prodotto di f per k da a a b
 *     (fact a)          fattoriale di a
 *     (binom a b)       coefficiente binomiale "a su b"
 *     (seq F a)         termine di indice a della successione F
 *     (ln a)            logaritmo naturale di a, che deve essere positivo
 *     (lnabs a)         logaritmo naturale del valore assoluto di a
 *
 * Negli enunciati sugli insiemi (capitolo 10) le lettere sono insiemi, e le
 * parole sono altre:
 *
 *     (empty)  (onlyempty)  (power a)
 *     (union a b)  (inter a b)  (diff a b)  (symdiff a b)  (times a b)
 *     (= a b)  (subset a b)  (neq a b)
 *     (and p q)  (or p q)  (implies p q)  (iff p q)  (not p)
 *
 * Le tabelle dei segni (capitolo 12) aggiungono i confronti tra numeri, il
 * valore assoluto e la radice quadrata:
 *
 *     (< a b)  (<= a b)  (abs a)  (sqrt a)
 *
 * Le derivate e le primitive (capitolo 13) le funzioni di una variabile
 * reale, che usano anche i limiti (capitolo 14), con altre quattro:
 *
 *     (exp a)  (sin a)  (cos a)  (atan a)  (root m a)  (pi)
 *     (tan a)  (cot a)  (arcsin a)  (arccos a)
 *
 * (root m a) è la radice di indice m di a, con m un intero scritto prima del
 * radicando; (atan a) l'arcotangente. Una parola sola per ogni funzione, con
 * lo stesso significato nei due capitoli.
 *
 * Un intero può avere il segno meno davanti. Una lettera è un solo
 * carattere: la variabile di induzione, un parametro, o un indice di somma.
 * Gli enunciati su insiemi qualsiasi (capitolo 11) hanno nomi più lunghi e
 * un lettore loro, che costruisce nodi dello stesso tipo.
 */

/* Tipi di nodo. */
typedef enum {
    CK_NUMBER, CK_LETTER, CK_ADD, CK_SUB, CK_MUL, CK_DIV, CK_NEG, CK_POW, CK_SUM, CK_PROD, CK_FACT, CK_BINOM, CK_SEQ, CK_LN, CK_LNABS,
    /* Enunciati sugli insiemi (capitolo 10). */
    CK_EMPTY, CK_ONLYEMPTY, CK_POWER, CK_UNION, CK_INTER, CK_DIFF, CK_SYMDIFF, CK_TIMES,
    CK_EQ, CK_SUBSET, CK_NEQ, CK_AND, CK_OR, CK_IMPLIES, CK_IFF, CK_NOT,
    /* Enunciati su insiemi qualsiasi (capitolo 11). */
    CK_TRUE, CK_FORALL, CK_EXISTS, CK_APP, CK_VAR, CK_ATOM, CK_FORALLPART, CK_EXISTSPART, CK_PARTVAR,
    /* Tabelle dei segni (capitolo 12). */
    CK_LESS, CK_LESS_EQ, CK_ABS, CK_SQRT,
    /* Le funzioni di una variabile reale: derivate e primitive (capitolo 13)
     * e limiti (capitolo 14). */
    CK_EXP, CK_SIN, CK_COS, CK_ATAN, CK_RADICAL, CK_PI,
    /* Solo nei limiti (capitolo 14).
     *
     * Warning: devono restare fuori dall'intervallo da CK_EXP a CK_PI, che
     * ck_eval() passa a ck_real_eval() del capitolo 13: lì ogni parola che
     * non ha un caso suo è presa per una potenza. */
    CK_TAN, CK_COT, CK_ARCSIN, CK_ARCCOS
} CkKind;

/* Un nodo dell'albero di un'espressione. I nodi stanno nell'arena.
 *   NUMBER: value.            LETTER: name.
 *   ADD, SUB, MUL, DIV, POW, BINOM: i due operandi a e b.
 *   NEG, FACT, LN, LNABS: a.
 *   SUM, PROD: name è l'indice, a e b gli estremi, c il termine.
 *   SEQ: name è la successione, a l'indice.
 * Nel capitolo 11:
 *   APP: value è il simbolo, a e b gli argomenti (NULL dove mancano).
 *   VAR: value è la distanza dal quantificatore che la lega: 0 per il
 *        più vicino che la contiene, 1 per quello prima, e così via.
 *   FORALL, EXISTS: value è l'insieme, a la condizione.
 *   FORALLPART, EXISTSPART (11g): per ogni / esiste una parte dell'insieme
 *        value; a la condizione. La parte legata conta tra le variabili
 *        legate, per le distanze di VAR e PARTVAR.
 *   PARTVAR (11g): la parte legata dal quantificatore a distanza value,
 *        applicata al termine a: "a sta nella parte".
 *   EQ: a = b tra due termini; con b NULL, a è una parte o una relazione
 *       applicata, e la condizione è che valga.
 *   ATOM: value è una condizione elementare (11e), in uno scheletro.
 * Nel capitolo 12:
 *   LESS, LESS_EQ: a < b, a <= b, come EQ e NEQ i due membri a e b.
 *   ABS, SQRT: il valore assoluto, la radice quadrata di a.
 * Nei capitoli 13 e 14:
 *   EXP, SIN, COS, ATAN: e^a, sin a, cos a, arctan a.
 *   RADICAL: la radice di indice a (un intero scritto) di b.
 *   PI: pi greco, senza operandi.
 * Nel capitolo 14:
 *   TAN, COT, ARCSIN, ARCCOS: tan a, cot a, arcsin a, arccos a. */
typedef struct CkNode CkNode;
struct CkNode {
    CkKind kind;
    long long value;
    char name;
    const CkNode *a, *b, *c;
};

_Static_assert(sizeof(CkNode) <= CK_SPARE_BYTES, "il blocco di riserva di ck_alloc() deve contenere un nodo");

/* Il punto del testo in cui si sta leggendo. */
static const char *ck_cursor;

/* Quante parentesi sono aperte nell'espressione che si sta leggendo. Ha un
 * tetto perché leggere e calcolare sono funzioni ricorsive: un testo con
 * milioni di parentesi aperte esaurirebbe la pila. */
static int ck_depth;

/* Ritorna un nodo nuovo di tipo kind, con gli altri campi a zero. */
static CkNode *ck_node(CkKind kind) {
    CkNode *node = ck_alloc(sizeof *node);
    memset(node, 0, sizeof *node);
    node->kind = kind;
    return node;
}

/* Salta spazi e tabulazioni, non le fini di riga: una voce sta su una riga. */
static void ck_skip(void) {
    while (*ck_cursor == ' ' || *ck_cursor == '\t') ck_cursor++;
}

/* Se il testo comincia con la parola word, seguita da uno spazio o dalla
 * fine della riga, la supera e ritorna 1. */
static int ck_word(const char *word) {
    size_t length = strlen(word);
    ck_skip();
    if (strncmp(ck_cursor, word, length) != 0 || (ck_cursor[length] && !isspace((unsigned char)ck_cursor[length]))) return 0;
    ck_cursor += length;
    return 1;
}

/* Legge un intero con segno e lo mette in *value; ritorna 0 se non c'è. */
static int ck_read_integer(long long *value) {
    int negative = 0, digits = 0;
    ck_skip();
    if (*ck_cursor == '-' && isdigit((unsigned char)ck_cursor[1])) { negative = 1; ck_cursor++; }
    *value = 0;
    while (isdigit((unsigned char)*ck_cursor)) {
        if (*value > 100000000000000000LL) { ck_unsupported("numero troppo grande nel certificato"); return 0; }
        *value = *value * 10 + (*ck_cursor++ - '0');
        digits++;
    }
    if (negative) *value = -*value;
    return digits > 0;
}

/* Legge un'espressione e ne ritorna l'albero. Un testo che non segue la
 * notazione rende il certificato sbagliato. */
static const CkNode *ck_read(void) {
    static const struct { const char *word; CkKind kind; int operands; } table[] = {
        {"+", CK_ADD, 2}, {"-", CK_SUB, 2}, {"*", CK_MUL, 2}, {"/", CK_DIV, 2}, {"^", CK_POW, 2}, {"neg", CK_NEG, 1},
        {"fact", CK_FACT, 1}, {"binom", CK_BINOM, 2}, {"sum", CK_SUM, 3}, {"prod", CK_PROD, 3}, {"seq", CK_SEQ, 1},
        {"ln", CK_LN, 1}, {"lnabs", CK_LNABS, 1},
        {"empty", CK_EMPTY, 0}, {"onlyempty", CK_ONLYEMPTY, 0}, {"power", CK_POWER, 1},
        {"union", CK_UNION, 2}, {"inter", CK_INTER, 2}, {"diff", CK_DIFF, 2}, {"symdiff", CK_SYMDIFF, 2}, {"times", CK_TIMES, 2},
        {"=", CK_EQ, 2}, {"subset", CK_SUBSET, 2}, {"neq", CK_NEQ, 2},
        {"and", CK_AND, 2}, {"or", CK_OR, 2}, {"implies", CK_IMPLIES, 2}, {"iff", CK_IFF, 2}, {"not", CK_NOT, 1},
        {"<", CK_LESS, 2}, {"<=", CK_LESS_EQ, 2}, {"abs", CK_ABS, 1}, {"sqrt", CK_SQRT, 1},
        {"exp", CK_EXP, 1}, {"sin", CK_SIN, 1}, {"cos", CK_COS, 1}, {"atan", CK_ATAN, 1}, {"root", CK_RADICAL, 2}, {"pi", CK_PI, 0},
        {"tan", CK_TAN, 1}, {"cot", CK_COT, 1}, {"arcsin", CK_ARCSIN, 1}, {"arccos", CK_ARCCOS, 1},
    };
    long long value;
    CkNode *node = ck_node(CK_NUMBER);
    if (ck_status != CK_ACCEPTED) return node;

    /* Un numero o una lettera. */
    if (ck_read_integer(&value)) { node->value = value; return node; }
    if (isalpha((unsigned char)*ck_cursor)) {
        node->kind = CK_LETTER;
        node->name = *ck_cursor++;
        if (isalnum((unsigned char)*ck_cursor)) ck_reject("certificato scritto male: nome di più lettere");
        return node;
    }

    /* Una parentesi: l'operatore, poi i suoi argomenti. */
    if (*ck_cursor != '(') { ck_reject("certificato scritto male: espressione attesa"); return node; }
    if (ck_depth >= CK_MAX_DEPTH) { ck_unsupported("espressione con troppe parentesi una dentro l'altra"); return node; }
    const char *word = ++ck_cursor;
    while (*ck_cursor && !isspace((unsigned char)*ck_cursor) && *ck_cursor != '(' && *ck_cursor != ')') ck_cursor++;
    size_t length = (size_t)(ck_cursor - word);
    int found = -1;
    for (int i = 0; i < (int)(sizeof table / sizeof *table); i++)
        if (strlen(table[i].word) == length && strncmp(table[i].word, word, length) == 0) found = i;
    if (found < 0) { ck_unsupported("il certificato usa un operatore che il verificatore non conosce"); return node; }
    node->kind = table[found].kind;

    /* Somme, prodotti e successioni hanno prima una lettera. */
    if (node->kind == CK_SUM || node->kind == CK_PROD || node->kind == CK_SEQ) {
        ck_skip();
        if (!isalpha((unsigned char)*ck_cursor)) { ck_reject("certificato scritto male: lettera attesa"); return node; }
        node->name = *ck_cursor++;
    }
    const CkNode **operand[3] = {&node->a, &node->b, &node->c};
    ck_depth++;
    for (int i = 0; i < table[found].operands; i++) *operand[i] = ck_read();
    ck_depth--;
    ck_skip();
    if (*ck_cursor != ')') ck_reject("certificato scritto male: parentesi chiusa attesa");
    else ck_cursor++;
    return node;
}

/* Ritorna 1 se la lettera name compare libera nell'albero: non come indice
 * di una somma che la contiene. */
static int ck_mentions(const CkNode *node, char name) {
    if (!node) return 0;
    if (node->kind == CK_LETTER) return node->name == name;
    if (node->kind == CK_SUM || node->kind == CK_PROD)
        return ck_mentions(node->a, name) || ck_mentions(node->b, name) || (node->name != name && ck_mentions(node->c, name));
    return ck_mentions(node->a, name) || ck_mentions(node->b, name) || ck_mentions(node->c, name);
}

/* Ritorna 1 se i due alberi sono scritti allo stesso modo, nodo per nodo,
 * nomi degli indici compresi. Serve a riconoscere la stessa somma in due
 * punti del certificato. Due somme uguali scritte con indici di nome diverso
 * non vengono riconosciute: è una rinuncia voluta, che evita di dover
 * ragionare su quale lettera sia legata da quale somma. */
static int ck_same(const CkNode *p, const CkNode *q) {
    if (!p || !q) return p == q;
    return p->kind == q->kind && p->value == q->value && p->name == q->name &&
           ck_same(p->a, q->a) && ck_same(p->b, q->b) && ck_same(p->c, q->c);
}

/* =============================================================================
 * 7. INDETERMINATE
 * ============================================================================
 *
 * Le lettere dei polinomi. Sono di cinque tipi.
 *
 *   7a. LETTER: la variabile di induzione n, oppure un parametro.
 *   7b. BIG: una somma o un prodotto fino a un estremo a n + c, per un certo
 *       c fisso detto riferimento. La stessa somma fino a un estremo più alto
 *       è questa lettera più i termini in mezzo (capitolo 8).
 *   7c. EXP: una potenza B^n, con B un numero primo oppure un polinomio
 *       nei soli parametri (spesso un parametro solo, come x^n).
 *   7d. SIGN: (-1)^n, la lettera s con s^2 = 1.
 *   7e. SEQ: un termine X(a n + c + j) di una successione per ricorrenza,
 *       per j da 0 all'ordine meno uno.
 *   7f. LN: il logaritmo ln|B| di un numero primo o di un polinomio B.
 *   7g. Nel capitolo 13, le funzioni di una variabile reale: E = e^u,
 *       S = sin u e C = cos u (sempre in coppia), A = arctan u, R = la radice
 *       m-esima di u, e pi greco. u è una frazione di polinomi nelle lettere
 *       già create.
 *   7h. CONST: nei limiti (capitolo 14), un numero positivo b^{1/m}: b è un
 *       primo, e, pi, il logaritmo di un primo, oppure la lettera L. Sono
 *       costanti, non funzioni di x come quelle di 7g, e hanno la regola loro,
 *       (b^{1/m})^m = b (14a).
 *
 * Il riferimento. Per ogni somma serve l'estremo più basso con cui compare,
 * così ogni altra occorrenza si ottiene aggiungendo termini e mai
 * togliendone (togliere un fattore a un prodotto sarebbe dividere per un
 * fattore che può valere zero). Per trovarlo ogni certificato viene valutato
 * due volte: la prima, di prova, serve solo ad abbassare i riferimenti; la
 * seconda fa i conti veri, e non deve più trovarne uno da abbassare.
 */

typedef enum {
    CK_VAR_LETTER, CK_VAR_BIG, CK_VAR_EXP, CK_VAR_SIGN, CK_VAR_SEQ, CK_VAR_LN,
    /* 7g. */
    CK_VAR_E, CK_VAR_SIN, CK_VAR_COS, CK_VAR_ATAN, CK_VAR_ROOT, CK_VAR_PI,
    /* 7h. */
    CK_VAR_CONST
} CkVarKind;

/* Un'indeterminata. I campi usati dipendono da kind:
 *   LETTER: name.
 *   BIG:    node è la somma o il prodotto, dilation e offset dicono il
 *           riferimento dilation * n + offset, lower è l'estremo inferiore.
 *   EXP, LN: prime è il numero primo che fa da base, oppure 0 e allora la
 *           base è il polinomio `base`, nell'arena.
 *   SEQ:    name è la successione, dilation e offset come per BIG, position
 *           è il j di X(dilation * n + offset + j).
 *   E, SIN, COS, ATAN, ROOT: l'argomento è la frazione base / below, e per
 *           ROOT prime è l'indice m della radice. La COS di una coppia sta
 *           subito dopo la sua SIN.
 *   PI:     nessun campo.
 *   CONST:  il numero b^{1/position} (14a): name dice b, 'p' il primo prime,
 *           'e' il numero di Nepero, 'q' pi greco, 'l' il logaritmo del primo
 *           prime, 'L' la lettera L = ln(1/t); position è l'indice m >= 1.
 * stated vale 1 per una somma o un prodotto che compare nell'enunciato
 * calcolato con i parametri fissati (capitolo 9), e non solo nel fattore.
 *
 * Warning: se aggiungi un tipo, decidi che cosa ne fanno ck_ln_poly() (8g),
 * ck_var_derivative() e ck_certainly_nonzero() (capitolo 13). Le lettere
 * CONST nascono e restano nei certificati dei limiti (ck_check_limit()
 * svuota la tabella), e quelle funzioni non le vedono mai. */
typedef struct {
    CkVarKind kind;
    char name;
    const CkNode *node;
    long long dilation, offset, lower;
    long long prime;
    CkPoly base, below;
    int position;
    int stated;
} CkVar;

static CkVar ck_vars[CK_MAX_VARS];
static int ck_var_count;

/* 1 durante la valutazione di prova, in cui i riferimenti si possono ancora
 * abbassare. */
static int ck_trial;

/* Aggiunge un'indeterminata e ne ritorna il numero d'ordine. */
static int ck_var_new(CkVar v) {
    if (ck_var_count == CK_MAX_VARS) { ck_unsupported("troppe quantità distinte per il verificatore"); return 0; }
    ck_vars[ck_var_count] = v;
    return ck_var_count++;
}

/* Ritorna l'indeterminata della lettera name, creandola se manca. */
static int ck_var_letter(char name) {
    for (int i = 0; i < ck_var_count; i++)
        if (ck_vars[i].kind == CK_VAR_LETTER && ck_vars[i].name == name) return i;
    return ck_var_new((CkVar){.kind = CK_VAR_LETTER, .name = name});
}

/* Ritorna l'indeterminata s = (-1)^n, creandola se manca. */
static int ck_var_sign(void) {
    if (ck_sign_var < 0) ck_sign_var = ck_var_new((CkVar){.kind = CK_VAR_SIGN});
    return ck_sign_var;
}

/* Ritorna l'indeterminata di tipo kind (EXP: B^n; LN: ln|B|) che ha per
 * base il primo `prime` oppure, se prime è 0, il polinomio base. La crea se
 * manca. */
static int ck_var_of_base(CkVarKind kind, long long prime, CkPoly base) {
    for (int i = 0; i < ck_var_count; i++)
        if (ck_vars[i].kind == kind && ck_vars[i].prime == prime && (prime || ck_poly_equal(ck_vars[i].base, base))) return i;
    return ck_var_new((CkVar){.kind = kind, .prime = prime, .base = base});
}

/* Porta il riferimento dell'indeterminata v a offset, se è più basso di
 * quello che ha. Dopo la valutazione di prova non deve più succedere. */
static void ck_lower_reference(int v, long long offset) {
    if (offset >= ck_vars[v].offset) return;
    if (!ck_trial) { ck_unsupported("una somma compare con un estremo più basso del previsto"); return; }
    ck_vars[v].offset = offset;
}

/* Ritorna l'indeterminata della somma o del prodotto node con estremo
 * dilation * n + offset, creandola se manca: stessa operazione, stesso
 * indice, stesso estremo inferiore, stesso termine, stessa dilatazione. Se
 * stated è diverso da zero annota che compare nell'enunciato. */
static int ck_var_big(const CkNode *node, long long lower, long long dilation, long long offset, int stated) {
    for (int i = 0; i < ck_var_count; i++) {
        const CkVar *v = &ck_vars[i];
        if (v->kind != CK_VAR_BIG || v->node->kind != node->kind || v->lower != lower || v->dilation != dilation) continue;
        if (v->node->name != node->name || !ck_same(v->node->c, node->c)) continue;
        ck_lower_reference(i, offset);
        if (stated) ck_vars[i].stated = 1;
        return i;
    }
    return ck_var_new((CkVar){.kind = CK_VAR_BIG, .node = node, .lower = lower, .dilation = dilation, .offset = offset, .stated = stated});
}

/* =============================================================================
 * 8. VALUTAZIONE
 * ============================================================================
 *
 * ck_eval() calcola la frazione di polinomi che è il valore di un albero.
 * La variabile di induzione vale, secondo il momento:
 *
 *   - un numero, il caso base: allora somme e prodotti si svolgono termine
 *     per termine e il risultato contiene solo i parametri;
 *   - n, oppure n + 1: allora somme, prodotti, potenze e successioni
 *     diventano indeterminate, come descritto nel capitolo 7.
 */

/* Una successione definita dal certificato, in uno di due modi.
 *
 *   - Voce "sequence", per ricorrenza: order è almeno 1 e
 *     X(m + order) = coef[0] X(m + order - 1) + ... + coef[order - 1] X(m),
 *     con i valori iniziali X(0), ..., X(order - 1).
 *   - Voce "define", con una formula: order è 0, e X(a) è il valore
 *     dell'albero formula quando la lettera parameter vale a. */
typedef struct {
    char name;
    int order;
    long long coef[CK_MAX_ORDER], init[CK_MAX_ORDER];
    char parameter;
    const CkNode *formula;
} CkSequence;

static CkSequence ck_sequences[CK_MAX_SEQUENCES];
static int ck_sequence_count;

/* Un indice di somma legato al suo valore mentre si calcola il termine. */
typedef struct {
    char name;
    CkRat value;
} CkBinding;

static CkBinding ck_env[CK_MAX_ENV];
static int ck_env_count;

/* Un parametro a cui la voce "defined" dà un valore: il nome, il valore
 * com'è scritto e il numero che è. */
typedef struct {
    char name;
    const CkNode *tree;
    CkRat value;
} CkFixed;

/* I parametri fissati dal certificato in corso. Valgono i loro numeri solo
 * mentre ck_fixing è diverso da zero; altrimenti restano lettere, perché
 * l'uguaglianza si controlla per tutti i valori dei parametri. */
static CkFixed ck_fixed[CK_MAX_POINT];
static int ck_fixed_count;
static int ck_fixing;

/* Con ck_real a 1 (capitolo 13) ck_eval() passa a ck_real_eval() le parole
 * delle funzioni di una variabile reale (exp, sin, cos, atan, root, pi), i
 * logaritmi, di cui annota le condizioni di esistenza, e le potenze, che
 * possono avere un esponente non intero. Il prototipo è in avanti perché
 * quelle regole sono del capitolo 13, che le presenta; qui basta il nome. */
static CkRat ck_real_eval(const CkNode *node);

static CkRat ck_eval(const CkNode *node);

/* Se r vale a n + c, con a e c interi, li mette in *a e *c e ritorna 1. Nel
 * caso base n è un numero e a risulta 0. */
static int ck_affine(CkRat r, long long *a, long long *c) {
    int n = ck_var_letter(ck_variable);
    *a = *c = 0;
    if (!ck_poly_is_constant(r.den) || r.den.count == 0) return 0;
    CkNum scale = ck_num_inv(r.den.term[0].coef);
    for (int i = 0; i < r.num.count; i++) {
        const CkTerm *t = &r.num.term[i];
        CkNum value = ck_num_mul(t->coef, scale);
        int degree = t->e[n];
        for (int v = 0; v < CK_MAX_VARS; v++)
            if (v != n && t->e[v]) return 0;
        if (degree > 1 || value.den != 1 || value.num > 1000000000 || value.num < -1000000000) return 0;
        *(degree ? a : c) = (long long)value.num;
    }
    return 1;
}

/* Ritorna il valore del termine di node con l'indice legato a index. */
static CkRat ck_eval_body(const CkNode *node, CkRat index) {
    if (ck_env_count == CK_MAX_ENV) { ck_unsupported("troppe somme una dentro l'altra"); return ck_rat_int(0); }
    ck_env[ck_env_count++] = (CkBinding){node->name, index};
    CkRat value = ck_eval(node->c);
    ck_env_count--;
    return value;
}

/* Ritorna il valore del termine di node di indice a n + c. */
static CkRat ck_eval_term(const CkNode *node, long long a, long long c) {
    CkPoly index = ck_poly_mul(ck_poly_number(ck_num_int(a)), ck_poly_variable(ck_var_letter(ck_variable)));
    return ck_eval_body(node, ck_rat_poly(ck_poly_add(index, ck_poly_number(ck_num_int(c)), 0)));
}

/* Valuta una somma o un prodotto.
 *
 * 8a. Estremi numerici: si svolge. Senza termini vale 0, o 1 per un
 *     prodotto.
 * 8b. Estremo superiore a n + c: è l'indeterminata con riferimento
 *     a n + r, più (o per) i termini di indice da a n + r + 1 ad a n + c.
 *     Il termine non deve dipendere da n né da un indice più esterno: se
 *     dipendesse da n, passando a n + 1 cambierebbero tutti i termini e non
 *     basterebbe aggiungere gli ultimi; se dipendesse da un indice esterno,
 *     la stessa lettera indicherebbe somme diverse. */
static CkRat ck_eval_big(const CkNode *node) {
    int product = node->kind == CK_PROD;
    long long lower, a, c, zero;
    if (!ck_affine(ck_eval(node->a), &zero, &lower) || zero != 0) { ck_unsupported("estremo inferiore di una somma non costante"); return ck_rat_int(0); }
    if (!ck_affine(ck_eval(node->b), &a, &c) || a < 0) { ck_unsupported("estremo superiore di una somma non della forma a n + c"); return ck_rat_int(0); }
    CkRat total = ck_rat_int(product ? 1 : 0);

    /* 8a. */
    if (a == 0) {
        if (c < lower - 1 || c - lower >= CK_MAX_UNROLL) { ck_unsupported("somma con estremi che il verificatore non tratta"); return total; }
        for (long long k = lower; k <= c && ck_status == CK_ACCEPTED; k++) {
            CkRat value = ck_eval_body(node, ck_rat_int(k));
            total = product ? ck_rat_mul(total, value) : ck_rat_add(total, value, 0);
        }
        return total;
    }

    /* 8b. */
    int bound = ck_mentions(node->c, ck_variable) && node->name != ck_variable;
    for (int i = 0; i < ck_env_count; i++)
        if (ck_env[i].name != node->name && ck_mentions(node->c, ck_env[i].name)) bound = 1;
    if (bound) { ck_unsupported("somma il cui termine dipende dalla variabile di induzione o da un altro indice"); return total; }
    int v = ck_var_big(node, lower, a, c, ck_fixing);
    long long reference = ck_vars[v].offset;
    if (c - reference > CK_MAX_EXTRA) { ck_unsupported("troppi termini tra due estremi della stessa somma"); return total; }
    total = ck_rat_poly(ck_poly_variable(v));
    for (long long j = reference + 1; j <= c && ck_status == CK_ACCEPTED; j++) {
        CkRat value = ck_eval_term(node, a, j);
        total = product ? ck_rat_mul(total, value) : ck_rat_add(total, value, 0);
    }
    return total;
}

/* Toglie da *rest, che è almeno 2, il suo più piccolo fattore primo: lo
 * ritorna, e mette in *times quante volte lo divideva. Si prova a dividere
 * per 2, 3, 4, ...: il primo divisore trovato è per forza primo, e se
 * nessuno fino alla radice quadrata divide, il numero stesso è primo. */
static long long ck_pop_prime(long long *rest, long long *times) {
    long long r = 2;
    while (r * r <= *rest && *rest % r != 0) r++;
    if (r * r > *rest) r = *rest;
    for (*times = 0; *rest % r == 0; (*times)++) *rest /= r;
    return r;
}

/* Ritorna il valore di (base)^(p n + q), con p diverso da zero, quando la
 * base vale la frazione B.
 *
 * Regole delle potenze, valide per esponenti interi e basi non nulle:
 * B^{p n + q} = (B^n)^p B^q, (u v)^n = u^n v^n, (u/v)^n = u^n / v^n. Con
 * queste la potenza si riduce a potenze n-esime di pochi mattoni, scelti in
 * modo che la stessa quantità sia scritta in un modo solo: 6^n = 2^n 3^n,
 * 4^n = (2^n)^2, (2x)^n = 2^n x^n. Numeratore e denominatore di B si
 * trattano separatamente.
 *
 * 8c. Un monomio c x^a y^b ..., con c un numero e x, y parametri. Il segno
 *     di c dà (-1)^n = s; il suo valore assoluto si scompone in fattori
 *     primi, e ogni primo r dà la lettera r^n; ogni parametro x dà la
 *     lettera x^n.
 * 8d. Un polinomio di più termini, nei soli parametri: è lui la base di una
 *     lettera.
 *
 * Una base che contiene n, o una somma, non ha una regola: (n + 1)^n non
 * diventa nulla di più semplice passando da n a n + 1. */
static CkRat ck_eval_power(CkRat base, long long p, long long q) {
    CkRat result = ck_rat_power(base, q);
    CkPoly side[2] = {base.num, base.den};
    if (base.num.count == 0) { ck_unsupported("zero elevato a un esponente che dipende da n"); return result; }
    ck_record(base.num);
    for (int v = 0; v < ck_var_count; v++) {
        /* Nel capitolo 13 fanno da parametri anche i numeri pi greco e
         * ln p: non dipendono da n, come (ln 3)^n. */
        CkVarKind kind = ck_vars[v].kind;
        int parameter = (kind == CK_VAR_LETTER && ck_vars[v].name != ck_variable) || (ck_real && (kind == CK_VAR_PI || (kind == CK_VAR_LN && ck_vars[v].prime)));
        if (!parameter && (ck_poly_uses(base.num, v) || ck_poly_uses(base.den, v))) {
            ck_unsupported("potenza con esponente che dipende da n e base che non è fatta di soli parametri");
            return result;
        }
    }

    /* Il numeratore con esponente p, il denominatore con esponente -p. */
    for (int i = 0; i < 2 && ck_status == CK_ACCEPTED; i++, p = -p) {
        if (side[i].count > 1) {
            /* 8d. */
            result = ck_rat_mul(result, ck_rat_power(ck_rat_poly(ck_poly_variable(ck_var_of_base(CK_VAR_EXP, 0, side[i]))), p));
            continue;
        }

        /* 8c. Il segno del coefficiente, che è una frazione. */
        const CkTerm *t = &side[i].term[0];
        __int128 part[2] = {t->coef.num, t->coef.den};
        if (part[0] < 0) {
            if (p % 2) result = ck_rat_mul(result, ck_rat_poly(ck_poly_variable(ck_var_sign())));
            part[0] = -part[0];
        }

        /* I fattori primi di numeratore e denominatore del coefficiente. */
        for (int j = 0; j < 2; j++) {
            if (part[j] > CK_MAX_FACTORED) { ck_unsupported("numero troppo grande da scomporre in fattori primi"); return result; }
            for (long long rest = (long long)part[j], times; rest > 1;) {
                int letter = ck_var_of_base(CK_VAR_EXP, ck_pop_prime(&rest, &times), (CkPoly){0, NULL});
                result = ck_rat_mul(result, ck_rat_power(ck_rat_poly(ck_poly_variable(letter)), (j ? -times : times) * p));
            }
        }

        /* I parametri. */
        for (int v = 0; v < CK_MAX_VARS; v++) {
            if (!t->e[v]) continue;
            int letter = ck_var_of_base(CK_VAR_EXP, 0, ck_poly_variable(v));
            result = ck_rat_mul(result, ck_rat_power(ck_rat_poly(ck_poly_variable(letter)), t->e[v] * p));
        }
    }
    return result;
}

/* I polinomi delle voci "brick" del certificato in corso: i fattori in cui
 * provare a scomporre l'argomento di un logaritmo (8g). Hanno il termine
 * direttivo di coefficiente 1, e si ricalcolano a ogni valutazione. */
static CkPoly ck_bricks[CK_MAX_BRICKS];
static int ck_brick_count;

/* Ritorna p, che non è il polinomio nullo, diviso per il coefficiente del
 * suo termine direttivo; mette quel coefficiente in *lead. */
static CkPoly ck_poly_monic(CkPoly p, CkNum *lead) {
    *lead = p.term[p.count - 1].coef;
    return ck_poly_mul(p, ck_poly_number(ck_num_inv(*lead)));
}

/* Ritorna ln|p| come polinomio nelle lettere-logaritmo (7f).
 *
 * 8g. Il logaritmo. Le regole ln|u v| = ln|u| + ln|v| e ln|u^e| = e ln|u|
 *     valgono per ogni u e v diversi da zero: per questo si lavora con
 *     ln|.|, senza dover conoscere il segno dei fattori. Dove l'enunciato è
 *     definito l'argomento di (ln a) è positivo, quindi ln a = ln|a|.
 *
 *     Il polinomio si scompone in fattori, e ogni fattore B dà una lettera,
 *     ln|B|:
 *       - il coefficiente del termine direttivo è un numero: va nei suoi
 *         fattori primi, ln|c| = somma degli e ln p;
 *       - una lettera x comune a tutti i termini, con esponente e, dà
 *         e ln|x|. Se la lettera è p^n dà e n ln p, se è B^n dà e n ln|B|,
 *         se è s = (-1)^n non dà nulla, perché |s| = 1; se è e^{m/Q} (7g,
 *         con Q un numero) dà e m / Q, perché ln e^u = u per ogni u reale;
 *       - i fattori che il certificato suggerisce con le voci "brick" si
 *         provano uno per uno, con la divisione esatta;
 *       - ciò che resta, se non è 1, è un fattore anche lui.
 *
 *     Alla correttezza non importa che la scomposizione sia la migliore:
 *     ogni passo è un'uguaglianza vera. Importa per riconoscere uguali due
 *     scritture della stessa quantità, come ln(n + 1) + ln(n + 2) e
 *     ln(n^2 + 3 n + 2): per questo i fattori li suggerisce chi ha trovato
 *     la dimostrazione. depth conta le basi una dentro l'altra. */
static CkPoly ck_ln_poly(CkPoly p, int depth) {
    const CkPoly none = {0, NULL};
    CkPoly result = ck_poly_number(ck_num_int(0));
    CkNum lead;
    if (p.count == 0) { ck_reject("l'enunciato contiene il logaritmo di un'espressione che vale zero"); return result; }
    if (depth > CK_MAX_ENV) { ck_unsupported("logaritmo che il verificatore non tratta"); return result; }

    /* Il coefficiente direttivo, nei suoi fattori primi. */
    p = ck_poly_monic(p, &lead);
    __int128 part[2] = {lead.num < 0 ? -lead.num : lead.num, lead.den};
    for (int j = 0; j < 2; j++) {
        if (part[j] > CK_MAX_FACTORED) { ck_unsupported("numero troppo grande da scomporre in fattori primi"); return result; }
        for (long long rest = (long long)part[j], times; rest > 1;) {
            int letter = ck_var_of_base(CK_VAR_LN, ck_pop_prime(&rest, &times), none);
            result = ck_poly_add(result, ck_poly_mul(ck_poly_number(ck_num_int(j ? -times : times)), ck_poly_variable(letter)), 0);
        }
    }

    /* Le lettere comuni a tutti i termini. Togliere a tutti lo stesso
     * esponente non cambia l'ordine dei termini. Le lettere create qui
     * dentro non compaiono in p: basta guardare quelle che già c'erano. */
    for (int v = 0, known = ck_var_count; v < known && ck_status == CK_ACCEPTED; v++) {
        const CkVar *x = &ck_vars[v];
        int low = p.term[0].e[v];
        for (int i = 1; i < p.count; i++)
            if (p.term[i].e[v] < low) low = p.term[i].e[v];
        if (low == 0) continue;
        CkTerm *t = ck_alloc(sizeof *t * (size_t)p.count);
        if (ck_status != CK_ACCEPTED) break;
        memcpy(t, p.term, sizeof *t * (size_t)p.count);
        for (int i = 0; i < p.count; i++) t[i].e[v] = (signed char)(t[i].e[v] - low);
        p.term = t;

        /* ln|lettera|, secondo il tipo. La lettera n si crea solo se serve:
         * nel capitolo 13 può non esserci una variabile di induzione. */
        CkPoly one;
        if (x->kind == CK_VAR_SIGN) continue;
        if (x->kind == CK_VAR_EXP) {
            CkPoly n = ck_poly_variable(ck_var_letter(ck_variable));
            one = ck_poly_mul(n, x->prime ? ck_poly_variable(ck_var_of_base(CK_VAR_LN, x->prime, none)) : ck_ln_poly(x->base, depth + 1));
        } else if (x->kind == CK_VAR_E && ck_poly_is_constant(x->below)) {
            one = ck_poly_mul(x->base, ck_poly_number(ck_num_inv(x->below.term[0].coef)));
        } else {
            one = ck_poly_variable(ck_var_of_base(CK_VAR_LN, 0, ck_poly_variable(v)));
        }
        result = ck_poly_add(result, ck_poly_mul(ck_poly_number(ck_num_int(low)), one), 0);
    }

    /* I fattori suggeriti: ognuno finché divide. Il grado scende a ogni
     * divisione, quindi il ciclo finisce; il tetto è una cautela. */
    for (int i = 0, steps = 0; i < ck_brick_count && ck_status == CK_ACCEPTED; i++) {
        CkPoly quotient;
        while (steps++ < 4 * CK_MAX_EXPONENT && !ck_poly_is_constant(p) && ck_poly_divides(ck_bricks[i], p, &quotient)) {
            p = quotient;
            result = ck_poly_add(result, ck_poly_variable(ck_var_of_base(CK_VAR_LN, 0, ck_bricks[i])), 0);
        }
    }

    /* Ciò che resta: 1, oppure un ultimo fattore. */
    if (ck_status == CK_ACCEPTED && !ck_poly_is_constant(p))
        result = ck_poly_add(result, ck_poly_variable(ck_var_of_base(CK_VAR_LN, 0, p)), 0);
    return result;
}

/* Ritorna ln|node|, per un argomento diverso da zero.
 *
 * Finché l'argomento è scritto come un prodotto, un quoziente, un opposto, un
 * valore assoluto, una potenza a esponente intero o una radice si applicano
 * le regole di 8g a com'è scritto, con ln|u| = ln|-u| = ln||u|| e
 * ln|u^{1/m}| = (1/m) ln|u|, senza svolgere i conti: di (n + 1)^3 si prende
 * 3 ln|n + 1|, invece di sviluppare il cubo e doverlo poi scomporre di
 * nuovo. Il valore assoluto e la radice compaiono solo nel capitolo 13, ed è
 * l'unico modo in cui lì un valore assoluto si calcola. Il resto si
 * calcola, e va a ck_ln_poly(): numeratore meno denominatore. Per "è
 * definito per ogni n" (capitolo 9) ogni pezzo calcolato conta come un
 * divisore: non deve annullarsi. */
static CkPoly ck_ln_node(const CkNode *node) {
    long long a, c;
    if (ck_status != CK_ACCEPTED) return ck_poly_number(ck_num_int(0));
    if (node->kind == CK_MUL || node->kind == CK_DIV) return ck_poly_add(ck_ln_node(node->a), ck_ln_node(node->b), node->kind == CK_DIV);
    if (node->kind == CK_NEG || (ck_real && node->kind == CK_ABS)) return ck_ln_node(node->a);
    if (ck_real && node->kind == CK_RADICAL && node->a->kind == CK_NUMBER && node->a->value > 0)
        return ck_poly_mul(ck_poly_number(ck_num_make(1, node->a->value)), ck_ln_node(node->b));
    if (node->kind == CK_POW && ck_affine(ck_eval(node->b), &a, &c) && a == 0)
        return ck_poly_mul(ck_poly_number(ck_num_int(c)), ck_ln_node(node->a));
    CkRat x = ck_eval(node);
    ck_record(x.num);
    return ck_poly_add(ck_ln_poly(x.num, 0), ck_ln_poly(x.den, 0), 1);
}

/* Ritorna il termine di indice `index` della successione name.
 *
 * 8e. Definita con una formula: si calcola la formula con il suo parametro
 *     legato all'indice. La formula non nomina altre lettere (lo controlla
 *     ck_read_certificate()), quindi ciò che è legato in questo momento non
 *     la tocca.
 * 8f. Definita per ricorrenza: si tiene una finestra di `order` termini
 *     consecutivi e la si fa scorrere in avanti. Con un indice numerico la
 *     finestra parte dai valori iniziali; con l'indice a n + c parte dalle
 *     indeterminate X(a n + r), ..., X(a n + r + order - 1), dove r è il
 *     riferimento. */
static CkRat ck_eval_sequence(char name, CkRat index) {
    const CkSequence *s = NULL;
    long long a, c, steps;
    CkRat window[CK_MAX_ORDER];
    for (int i = 0; i < ck_sequence_count; i++)
        if (ck_sequences[i].name == name) s = &ck_sequences[i];
    if (!s) { ck_unsupported("successione che il certificato non definisce"); return ck_rat_int(0); }

    /* 8e. */
    if (s->order == 0) {
        if (ck_env_count == CK_MAX_ENV) { ck_unsupported("troppe definizioni una dentro l'altra"); return ck_rat_int(0); }
        ck_env[ck_env_count++] = (CkBinding){s->parameter, index};
        CkRat value = ck_eval(s->formula);
        ck_env_count--;
        return value;
    }

    /* 8f. */
    if (!ck_affine(index, &a, &c) || a < 0) { ck_unsupported("indice di successione non della forma a n + c"); return ck_rat_int(0); }

    /* La finestra di partenza, e di quanti posti farla scorrere. */
    if (a == 0) {
        for (int j = 0; j < s->order; j++) window[j] = ck_rat_int(s->init[j]);
        steps = c;
    } else {
        int first = -1;
        for (int i = 0; i < ck_var_count && first < 0; i++)
            if (ck_vars[i].kind == CK_VAR_SEQ && ck_vars[i].name == name && ck_vars[i].dilation == a && ck_vars[i].position == 0) first = i;
        if (first < 0)
            for (int j = 0; j < s->order; j++) {
                int v = ck_var_new((CkVar){.kind = CK_VAR_SEQ, .name = name, .dilation = a, .offset = c, .position = j});
                if (j == 0) first = v;
            }
        for (int j = 0; j < s->order; j++) ck_lower_reference(first + j, c);
        for (int j = 0; j < s->order; j++) window[j] = ck_rat_poly(ck_poly_variable(first + j));
        steps = c - ck_vars[first].offset;
    }
    if (steps < 0 || steps > CK_MAX_UNROLL) { ck_unsupported("indice di successione che il verificatore non tratta"); return ck_rat_int(0); }

    /* Ogni passo: il termine nuovo, e la finestra un posto più avanti. */
    for (long long step = 0; step < steps && ck_status == CK_ACCEPTED; step++) {
        CkRat next = ck_rat_int(0);
        for (int i = 0; i < s->order; i++) next = ck_rat_add(next, ck_rat_mul(ck_rat_int(s->coef[i]), window[s->order - 1 - i]), 0);
        for (int j = 0; j + 1 < s->order; j++) window[j] = window[j + 1];
        window[s->order - 1] = next;
    }
    return window[0];
}

/* Ritorna il fattoriale dell'espressione node: il prodotto di j per j da 1
 * a node. L'indice si chiama '#', un nome che un certificato non può
 * scrivere: così tutti i fattoriali sono la stessa famiglia di prodotti, e
 * nessun prodotto scritto nel certificato si confonde con loro. */
static CkRat ck_eval_factorial(const CkNode *node) {
    CkNode *product = ck_node(CK_PROD), *one = ck_node(CK_NUMBER), *index = ck_node(CK_LETTER);
    one->value = 1;
    index->name = product->name = '#';
    product->a = one;
    product->b = node;
    product->c = index;
    return ck_eval_big(product);
}

/* Ritorna il valore dell'albero node. */
static CkRat ck_eval(const CkNode *node) {
    long long a, c;
    if (ck_status != CK_ACCEPTED) return ck_rat_int(0);
    if (ck_real && (node->kind == CK_POW || node->kind == CK_LN || node->kind == CK_LNABS || node->kind == CK_ABS || (node->kind >= CK_EXP && node->kind <= CK_PI)))
        return ck_real_eval(node);
    switch (node->kind) {
    case CK_NUMBER:
        return ck_rat_int(node->value);
    case CK_LETTER:
        /* Un indice legato, un parametro con il suo valore fissato, un
         * parametro come lettera, o la variabile di induzione. */
        for (int i = ck_env_count - 1; i >= 0; i--)
            if (ck_env[i].name == node->name) return ck_env[i].value;
        for (int i = 0; i < ck_fixed_count && ck_fixing; i++)
            if (ck_fixed[i].name == node->name) return ck_fixed[i].value;
        if (node->name != ck_variable) return ck_rat_poly(ck_poly_variable(ck_var_letter(node->name)));
        if (ck_numeric) return ck_rat_int(ck_point);
        return ck_rat_poly(ck_poly_add(ck_poly_variable(ck_var_letter(ck_variable)), ck_poly_number(ck_num_int(ck_shift)), 0));
    case CK_NEG:
        return ck_rat_add(ck_rat_int(0), ck_eval(node->a), 1);
    case CK_ADD:
        return ck_rat_add(ck_eval(node->a), ck_eval(node->b), 0);
    case CK_SUB:
        return ck_rat_add(ck_eval(node->a), ck_eval(node->b), 1);
    case CK_MUL:
        return ck_rat_mul(ck_eval(node->a), ck_eval(node->b));
    case CK_DIV:
        return ck_rat_div(ck_eval(node->a), ck_eval(node->b));
    case CK_SUM: case CK_PROD:
        return ck_eval_big(node);
    case CK_SEQ:
        return ck_eval_sequence(node->name, ck_eval(node->a));
    case CK_FACT:
        return ck_eval_factorial(node->a);
    case CK_LN: case CK_LNABS:
        /* 8g. Per "è definito per ogni n" (capitolo 9) l'argomento di ln
         * deve essere positivo, e una regola per stabilirlo qui non c'è. */
        if (ck_fixing && node->kind == CK_LN) { ck_unsupported("non so stabilire che l'argomento di un logaritmo è positivo"); return ck_rat_int(0); }
        return ck_rat_poly(ck_ln_node(node->a));
    case CK_POW: {
        /* Esponente intero: prodotti ripetuti. Esponente p n + q: le regole
         * delle potenze. */
        CkRat base = ck_eval(node->a);
        if (!ck_affine(ck_eval(node->b), &a, &c)) { ck_unsupported("esponente non della forma p n + q"); return ck_rat_int(0); }
        return a == 0 ? ck_rat_power(base, c) : ck_eval_power(base, a, c);
    }
    case CK_BINOM: {
        /* "x su r" con r intero non negativo: il prodotto di (x - r + j)/j
         * per j da 1 a r, un polinomio in x. Altrimenti x!/(r! (x - r)!). */
        if (ck_affine(ck_eval(node->b), &a, &c) && a == 0) {
            CkRat x = ck_eval(node->a), result = ck_rat_int(1);
            if (c < 0 || c > CK_MAX_EXTRA) { ck_unsupported("coefficiente binomiale che il verificatore non tratta"); return result; }
            for (long long j = 1; j <= c && ck_status == CK_ACCEPTED; j++)
                result = ck_rat_div(ck_rat_mul(result, ck_rat_add(x, ck_rat_int(c - j), 1)), ck_rat_int(j));
            return result;
        }
        CkNode *gap = ck_node(CK_SUB);
        gap->a = node->a;
        gap->b = node->b;
        return ck_rat_div(ck_eval_factorial(node->a), ck_rat_mul(ck_eval_factorial(node->b), ck_eval_factorial(gap)));
    }
    case CK_EXP: case CK_SIN: case CK_COS: case CK_ATAN: case CK_RADICAL: case CK_PI:
        ck_unsupported("il certificato di induzione usa un operatore delle funzioni di una variabile reale, che lì il verificatore non conosce");
        break;
    case CK_TAN: case CK_COT: case CK_ARCSIN: case CK_ARCCOS:
        /* Le parole dei soli limiti (capitolo 14): qui arrivano da un
         * certificato di induzione o del capitolo 13, che non le conoscono. */
        ck_unsupported("un operatore dei limiti (tan, cot, arcsin, arccos), che qui il verificatore non conosce");
        break;
    case CK_ABS:
        ck_unsupported("il valore assoluto non si calcola in un certificato di induzione");
        break;
    default:
        ck_unsupported("una parola degli enunciati sugli insiemi in un certificato di induzione");
    }
    return ck_rat_int(0);
}

/* =============================================================================
 * 9. DEFINITO PER OGNI n
 * ============================================================================
 *
 * La voce "defined" afferma che, con i parametri ai valori che indica,
 * l'enunciato è definito per ogni intero n dal caso base b in su. Qui lo si
 * controlla: è vero se nessun divisore del conto dei due membri vale zero.
 *
 * Quali divisori. Sono di due specie.
 *
 *   9a. Quelli che si incontrano calcolando i due membri in n, con le somme
 *       prese come lettere: polinomi in n e nelle altre lettere, che non
 *       devono annullarsi per nessun n >= b.
 *   9b. Quelli nascosti nei termini di una somma o di un prodotto. La
 *       lettera vale la somma fino all'indice a n + r. I termini di indice
 *       fino ad a b + r si calcolano uno per uno, con i numeri. Ogni altro
 *       ha indice a n + r - i per un n >= b + 1 e un i tra 0 e a - 1: il suo
 *       conto, fatto con n lettera, dà divisori che sono polinomi in n e
 *       non devono annullarsi per nessun n >= b + 1.
 *
 * Come si stabilisce che un polinomio non si annulla per nessun n >= m. Le
 * regole sono poche e nessuna cerca: ciascuna è un conto che finisce.
 *
 *   9c. Una lettera che moltiplica tutti i termini si mette da parte, se non
 *       vale mai zero: la potenza p^n di un primo; un prodotto, se nessuno
 *       dei suoi fattori è zero (come in 9b, guardando il valore del
 *       termine invece dei suoi divisori).
 *   9d. Ciò che resta è un polinomio nella sola n: c_0 + c_1 n + ... +
 *       c_d n^d. Limite di Cauchy: ogni sua radice reale x soddisfa
 *       |x| <= 1 + M, dove M è il più grande dei |c_i / c_d|. Infatti se
 *       |x| > 1 + M allora
 *
 *           |c_0 + ... + c_{d-1} x^{d-1}| <= |c_d| M (|x|^d - 1)/(|x| - 1) < |c_d| |x|^d,
 *
 *       e il termine di grado più alto non può essere compensato dagli
 *       altri. Oltre quel limite non ci sono radici; fino a lì gli interi
 *       sono finiti, e si provano uno per uno.
 *   9e. Oppure è un polinomio nella sola lettera T = p^n: stesso limite per
 *       T, e si provano p^m, p^{m+1}, ... finché non lo superano.
 *   9f. Oppure i suoi termini hanno tutti lo stesso segno e sono fatti di n
 *       (con m >= 0) e di potenze di primi, e almeno uno non può valere
 *       zero: una somma di quantità dello stesso segno, di cui una non
 *       nulla, non è zero.
 *
 * La lettera s = (-1)^n si tratta come nel controllo 3: due volte, con 1 e
 * con -1 al suo posto. Con 1 contano solo gli n pari, con -1 i dispari.
 *
 * Le risposte sono tre anche qui: non si annulla mai; si annulla in un n
 * preciso, e allora la voce "defined" dice il falso; non si sa.
 */

/* Le voci di un certificato, come sono state lette. I valori della voce
 * "defined" stanno in ck_fixed[] (capitolo 8). */
typedef struct {
    char variable;
    long long base;
    const CkNode *left, *right;         /* i due membri dell'enunciato */
    const CkNode *factor;               /* W */
    const CkNode *generic[CK_MAX_GENERIC];
    int generics;
    const CkNode *brick[CK_MAX_BRICKS]; /* i fattori suggeriti per i logaritmi */
    int bricks;
    int defined;                        /* 1 se c'è la voce "defined" */
    const CkNode *sets;                 /* certificato "sets": l'enunciato; altrimenti NULL */
    int checked;                        /* 1 per un certificato dei capitoli 11 e 12, già controllato mentre lo si leggeva */
} CkCertificate;

static int ck_never_zero(CkPoly q, long long from, long long *where, int depth);

/* 9c. Ritorna 1 se la lettera v non vale zero per nessun n >= from, 0 se
 * vale zero per n = *where, -1 se non si sa. */
static int ck_letter_never_zero(int v, long long from, long long *where, int depth) {
    CkVar x = ck_vars[v];
    long long last = x.dilation * from + x.offset;
    if (x.kind == CK_VAR_EXP && x.prime) return 1;
    if (x.kind != CK_VAR_BIG || x.node->kind != CK_PROD) return -1;
    if (last - x.lower >= CK_MAX_UNROLL || x.dilation > CK_MAX_EXTRA) return -1;

    /* I fattori di indice fino ad a from + r, uno per uno. Uno nullo
     * annulla il prodotto già per n = from. */
    for (long long k = x.lower; k <= last && ck_status == CK_ACCEPTED; k++) {
        CkRat value = ck_eval_body(x.node, ck_rat_int(k));
        if (!ck_poly_is_constant(value.num)) return -1;
        if (value.num.count == 0) { *where = from; return 0; }
    }

    /* Gli altri, con n lettera. */
    for (long long i = 0; i < x.dilation && ck_status == CK_ACCEPTED; i++) {
        int answer = ck_never_zero(ck_eval_term(x.node, x.dilation, x.offset - i).num, from + 1, where, depth + 1);
        if (answer != 1) return answer;
    }
    return 1;
}

/* Ritorna 1 se il polinomio q non vale zero per nessun intero n >= from, 0
 * se vale zero per n = *where (che è almeno from), -1 se non si sa. depth
 * conta i prodotti uno dentro l'altro, per fermare la ricorsione. */
static int ck_never_zero(CkPoly q, long long from, long long *where, int depth) {
    int n = ck_var_letter(ck_variable), step = ck_sign_var < 0 ? 1 : 2;
    if (depth > CK_MAX_ENV) return -1;
    for (int sign = 1; sign >= (ck_sign_var < 0 ? 1 : -1); sign -= 2) {
        /* Con s al suo valore contano solo gli n pari, o solo i dispari:
         * first è il primo. */
        CkPoly p = ck_poly_at_sign(q, sign);
        long long first = from + (step == 2 && (from % 2 != 0) != (sign < 0));
        if (ck_status != CK_ACCEPTED) return -1;
        if (p.count == 0) { *where = first; return 0; }

        /* 9c. Per ogni lettera l'esponente più basso tra i termini, cioè
         * quanto è comune a tutti. `main` è la lettera, oltre a n, il cui
         * esponente cambia da un termine all'altro; `others` quante sono. */
        signed char low[CK_MAX_VARS] = {0};
        int main = n, others = 0, uses_n = 0, degree = 0;
        for (int v = 0; v < ck_var_count; v++) {
            int high = 0;
            low[v] = v == n ? 0 : p.term[0].e[v];
            for (int i = 0; i < p.count; i++) {
                if (p.term[i].e[v] < low[v]) low[v] = p.term[i].e[v];
                if (p.term[i].e[v] > high) high = p.term[i].e[v];
            }
            if (v == n) { uses_n = high > 0; continue; }
            if (low[v] > 0) {
                int answer = ck_letter_never_zero(v, from, where, depth);
                if (answer != 1) return answer;
            }
            if (high > low[v]) { main = v; others++; }
        }

        /* 9f. Più lettere insieme: termini dello stesso segno. */
        if (others > 1 || (others == 1 && (uses_n || ck_vars[main].kind != CK_VAR_EXP || !ck_vars[main].prime))) {
            int plain = from >= 0, certain = 0;
            for (int i = 0; i < p.count; i++) {
                int positive = 1;
                if ((p.term[i].coef.num > 0) != (p.term[0].coef.num > 0)) plain = 0;
                for (int v = 0; v < ck_var_count; v++) {
                    if (p.term[i].e[v] == low[v]) continue;
                    if (v == n) positive = from >= 1;
                    else if (ck_vars[v].kind != CK_VAR_EXP || !ck_vars[v].prime) plain = 0;
                }
                if (positive) certain = 1;
            }
            if (!plain || !certain) return -1;
            continue;
        }

        /* 9d, 9e. Un polinomio in una sola quantità: i coefficienti per
         * grado, e il limite di Cauchy. */
        CkNum c[CK_MAX_EXPONENT + 1], most = ck_num_int(0);
        for (int i = 0; i <= CK_MAX_EXPONENT; i++) c[i] = ck_num_int(0);
        for (int i = 0; i < p.count; i++) {
            int e = p.term[i].e[main] - low[main];
            c[e] = p.term[i].coef;
            if (e > degree) degree = e;
        }
        for (int i = 0; i < degree; i++) {
            CkNum size = ck_num_mul(c[i], ck_num_inv(c[degree]));
            if (size.num < 0) size = ck_num_neg(size);
            if (ck_num_add(size, ck_num_neg(most)).num > 0) most = size;
        }
        __int128 limit = 1 + most.num / most.den;
        if (ck_status != CK_ACCEPTED) return -1;
        if (degree == 0) continue;

        /* I valori da provare: n da first in su, finché la quantità (n
         * stessa, oppure p^n) non supera il limite. */
        __int128 value = first, prime = ck_vars[main].prime;
        if (main != n) {
            if (first < 0) return -1;
            value = 1;
            for (long long k = 0; k < first && value <= limit; k++)
                if (__builtin_mul_overflow(value, prime, &value)) value = limit + 1;
        } else if (limit - first > (__int128)CK_MAX_SCAN * step) {
            return -1;
        }
        for (long long x = first; value <= limit && ck_status == CK_ACCEPTED; x += step) {
            /* Schema di Horner, dal grado più alto. */
            CkNum at = ck_num_int(0);
            for (int i = degree; i >= 0; i--) at = ck_num_add(ck_num_mul(at, ck_num_make(value, 1)), c[i]);
            if (ck_status == CK_ACCEPTED && at.num == 0) { *where = x; return 0; }
            for (int k = 0; k < step && value <= limit; k++) {
                if (main == n) value++;
                else if (__builtin_mul_overflow(value, prime, &value)) value = limit + 1;
            }
        }
    }
    return ck_status == CK_ACCEPTED ? 1 : -1;
}

/* 9a, 9b. Annota i divisori dell'enunciato calcolato con i parametri ai
 * valori della voce "defined". È chiamata in tutte e due le valutazioni,
 * quella di prova compresa: anche questi conti possono abbassare un
 * riferimento. */
static void ck_defined_divisors(const CkCertificate *cert) {
    /* I valori dei parametri: numeri, calcolati una volta sola. */
    for (int i = 0; i < ck_fixed_count && ck_trial; i++) {
        ck_fixed[i].value = ck_eval(ck_fixed[i].tree);
        if (!ck_poly_is_constant(ck_fixed[i].value.num) || !ck_poly_is_constant(ck_fixed[i].value.den))
            ck_reject("il valore dato a un parametro non è un numero");
    }
    ck_fixing = ck_recording = 1;
    ck_divisor_count = 0;

    /* 9a. I due membri in n. */
    ck_record_from = cert->base;
    ck_numeric = 0;
    ck_shift = 0;
    ck_eval(cert->left);
    ck_eval(cert->right);

    /* 9b. I termini nascosti. La tabella può crescere mentre la si scorre:
     * un termine può contenere un'altra somma. */
    ck_record_from = cert->base + 1;
    for (int v = 0; v < ck_var_count && ck_status == CK_ACCEPTED; v++) {
        CkVar x = ck_vars[v];
        long long last = x.dilation * cert->base + x.offset;
        if (x.kind != CK_VAR_BIG || !x.stated) continue;
        if (last - x.lower >= CK_MAX_UNROLL || x.dilation > CK_MAX_EXTRA) { ck_unsupported("somma con troppi termini da controllare uno per uno"); break; }
        for (long long k = x.lower; k <= last && ck_status == CK_ACCEPTED; k++) ck_eval_body(x.node, ck_rat_int(k));
        for (long long i = 0; i < x.dilation && ck_status == CK_ACCEPTED; i++) ck_eval_term(x.node, x.dilation, x.offset - i);
    }

    /* Le voci "generic": con quei valori devono essere numeri diversi da
     * zero, o il teorema non si applica al punto. */
    for (int i = 0; i < cert->generics && ck_status == CK_ACCEPTED; i++) {
        CkRat g = ck_eval(cert->generic[i]);
        if (!ck_poly_is_constant(g.num) || !ck_poly_is_constant(g.den) || g.num.count == 0)
            ck_unsupported("con i valori dati ai parametri una voce generic non è un numero diverso da zero");
    }
    ck_fixing = ck_recording = 0;
}

/* Stabilisce, con le regole da 9c a 9f, che nessun divisore annotato si
 * annulla. Uno che si annulla rende falsa la voce "defined"; uno di cui non
 * si sa dire rende il certificato non controllabile. */
static void ck_defined_decide(void) {
    static char text[96];
    ck_fixing = 1;
    for (int i = 0; i < ck_divisor_count && ck_status == CK_ACCEPTED; i++) {
        long long where = 0;
        int answer = ck_never_zero(ck_divisors[i].value, ck_divisors[i].from, &where, 0);
        if (answer < 0) ck_unsupported("non so stabilire che un divisore dell'enunciato non si annulla mai");
        if (answer == 0) {
            snprintf(text, sizeof text, "l'enunciato non è definito per %c = %lld: un divisore vale zero", ck_variable, where);
            ck_reject(text);
        }
    }
    ck_fixing = 0;
}

/* =============================================================================
 * 10. ENUNCIATI SUGLI INSIEMI
 * ============================================================================
 *
 * Un certificato "sets" porta solo l'enunciato:
 *
 *     certificate sets
 *     statement (= (diff (diff S T) V) (diff S (union T V)))
 *     end
 *
 * Non serve altro, perché qui il verificatore non ricontrolla una
 * dimostrazione: decide l'enunciato da capo, con un conto che finisce.
 *
 * IL TEOREMA. Se il certificato è accettato, l'enunciato vale per ogni
 * scelta degli insiemi indicati dalle lettere. Gli oggetti sono di tre tipi
 * che non si mescolano: insiemi di elementi (le lettere e ciò che se ne
 * ottiene con unione, intersezione, differenza); collezioni di insiemi
 * (l'insieme delle parti, e la collezione che contiene solo l'insieme
 * vuoto); insiemi di coppie (i prodotti cartesiani).
 *
 * COME SI DECIDE. Con k lettere ogni elemento sta in una delle 2^k - 1
 * "regioni" del diagramma di Venn: quella degli elementi che appartengono
 * proprio a certe lettere e non alle altre (la regione di chi non appartiene
 * a nessuna non serve: nessuna espressione la contiene).
 *
 *   10a. Un insieme di elementi costruito dalle lettere è un'unione di
 *        regioni. A = B vale se e solo se ogni regione che sta in uno e non
 *        nell'altro è vuota; A incluso in B, se ogni regione di A fuori da B
 *        è vuota. Quindi la verità dell'enunciato dipende solo da QUALI
 *        regioni sono vuote, e ogni scelta di regioni vuote si realizza
 *        davvero: basta mettere un elemento in ciascuna delle altre. Provate
 *        tutte le 2^(2^k - 1) scelte, l'enunciato è deciso.
 *   10b. Coppie. (a, b) sta in A per B se a sta in A e b in B: conta solo in
 *        quali regioni stanno a e b. Due insiemi di coppie sono uguali se
 *        contengono le stesse coppie per ogni scelta di due regioni non
 *        vuote.
 *   10c. Collezioni. Un insieme X sta nell'insieme delle parti di A se è
 *        incluso in A: conta solo quali regioni X tocca, e se ha elementi
 *        fuori da tutte le lettere (e allora non è incluso in nessuna
 *        espressione). X può toccare un insieme qualsiasi di regioni non
 *        vuote; è l'insieme vuoto se non ne tocca nessuna e non ha elementi
 *        fuori. Due collezioni sono uguali se contengono gli stessi X per
 *        ogni scelta di regioni toccate.
 *
 * "Esiste un insieme" non è trattato: lì conta anche quanti elementi ha una
 * regione, e il conto è un altro.
 */

/* Il tipo di ciò che un albero denota. ANY è l'insieme vuoto, che sta al
 * posto di un oggetto di qualunque tipo. */
typedef enum { CK_SET_WRONG, CK_SET_ANY, CK_SET_ELEMENTS, CK_SET_FAMILY, CK_SET_PAIRS } CkSetType;

/* Le lettere dell'enunciato in corso: la lettera ck_set_names[i] è l'insieme
 * di indice i, e la regione r (da 1 a 2^k - 1) è fatta degli elementi che
 * stanno nell'insieme i esattamente quando il bit i di r è 1. */
static char ck_set_names[4];
static int ck_set_count;

/* Mette tra le lettere dell'enunciato quelle dell'albero node. */
static void ck_set_collect(const CkNode *node) {
    if (!node) return;
    if (node->kind == CK_LETTER && !memchr(ck_set_names, node->name, (size_t)ck_set_count)) {
        if (ck_set_count == 4) { ck_unsupported("più di quattro insiemi in un enunciato"); return; }
        ck_set_names[ck_set_count++] = node->name;
    }
    ck_set_collect(node->a);
    ck_set_collect(node->b);
}

/* Ritorna il tipo dell'espressione node, o CK_SET_WRONG se i suoi pezzi sono
 * di tipi che non stanno insieme. */
static CkSetType ck_set_type(const CkNode *node) {
    CkSetType a = node->a ? ck_set_type(node->a) : CK_SET_WRONG, b = node->b ? ck_set_type(node->b) : CK_SET_WRONG;
    int plain = (a == CK_SET_ELEMENTS || a == CK_SET_ANY) && (b == CK_SET_ELEMENTS || b == CK_SET_ANY || node->kind == CK_POWER);
    switch (node->kind) {
    case CK_LETTER:    return CK_SET_ELEMENTS;
    case CK_EMPTY:     return CK_SET_ANY;
    case CK_ONLYEMPTY: return CK_SET_FAMILY;
    case CK_POWER:     return plain ? CK_SET_FAMILY : CK_SET_WRONG;
    case CK_TIMES:     return plain ? CK_SET_PAIRS : CK_SET_WRONG;
    case CK_UNION: case CK_INTER: case CK_DIFF: case CK_SYMDIFF:
        if (a == CK_SET_WRONG || b == CK_SET_WRONG) return CK_SET_WRONG;
        if (a == CK_SET_ANY) return b;
        return (b == CK_SET_ANY || a == b) ? a : CK_SET_WRONG;
    default:           return CK_SET_WRONG;
    }
}

/* Ritorna x unito, intersecato, tolto o in differenza simmetrica con y,
 * secondo kind. Vale bit per bit: per gli insiemi di regioni, e per i valori
 * 0 e 1 di "appartiene". */
static unsigned ck_set_combine(CkKind kind, unsigned x, unsigned y) {
    return kind == CK_UNION ? x | y : kind == CK_INTER ? x & y : kind == CK_DIFF ? x & ~y : x ^ y;
}

/* 10a. Ritorna le regioni di cui è fatto l'insieme di elementi node: il bit
 * r - 1 è 1 se la regione r ne fa parte. */
static unsigned ck_set_regions(const CkNode *node) {
    if (node->kind == CK_EMPTY) return 0;
    if (node->kind != CK_LETTER) return ck_set_combine(node->kind, ck_set_regions(node->a), ck_set_regions(node->b));
    int index = (int)((const char *)memchr(ck_set_names, node->name, (size_t)ck_set_count) - ck_set_names);
    unsigned regions = 0;
    for (unsigned r = 1; r < 1u << ck_set_count; r++)
        if (r >> index & 1) regions |= 1u << (r - 1);
    return regions;
}

/* 10b. Ritorna 1 se una coppia con il primo elemento nella regione first e
 * il secondo nella regione second sta nell'insieme di coppie node. */
static unsigned ck_set_has_pair(const CkNode *node, unsigned first, unsigned second) {
    if (node->kind == CK_EMPTY) return 0;
    if (node->kind == CK_TIMES) return (ck_set_regions(node->a) >> (first - 1) & 1) && (ck_set_regions(node->b) >> (second - 1) & 1);
    return ck_set_combine(node->kind, ck_set_has_pair(node->a, first, second), ck_set_has_pair(node->b, first, second)) & 1;
}

/* 10c. Ritorna 1 se un insieme X che tocca proprio le regioni di `touched`,
 * e che ha elementi fuori da tutte le lettere se outside è 1, sta nella
 * collezione node. */
static unsigned ck_set_has_subset(const CkNode *node, unsigned touched, int outside) {
    if (node->kind == CK_EMPTY) return 0;
    if (node->kind == CK_ONLYEMPTY) return !outside && touched == 0;
    if (node->kind == CK_POWER) return !outside && (touched & ~ck_set_regions(node->a)) == 0;
    return ck_set_combine(node->kind, ck_set_has_subset(node->a, touched, outside), ck_set_has_subset(node->b, touched, outside)) & 1;
}

/* Ritorna 1 se la relazione node (uguale, incluso, diverso) vale quando le
 * regioni non vuote sono quelle di alive, 0 se non vale, -1 se i due membri
 * non sono dello stesso tipo. */
static int ck_set_relation(const CkNode *node, unsigned alive) {
    CkSetType a = ck_set_type(node->a), b = ck_set_type(node->b), type = a == CK_SET_ANY ? b : a;
    int equal = 1, inside = 1;
    if (a == CK_SET_WRONG || b == CK_SET_WRONG || (b != CK_SET_ANY && b != type)) return -1;
    if (type == CK_SET_FAMILY) {
        /* Ogni insieme di regioni non vuote: i sottoinsiemi di alive, dal
         * più grande al vuoto. */
        for (unsigned touched = alive;; touched = (touched - 1) & alive) {
            for (int outside = 0; outside < 2; outside++) {
                unsigned x = ck_set_has_subset(node->a, touched, outside), y = ck_set_has_subset(node->b, touched, outside);
                if (x != y) equal = 0;
                if (x && !y) inside = 0;
            }
            if (touched == 0) break;
        }
    } else if (type == CK_SET_PAIRS) {
        for (unsigned first = 1; first < 1u << ck_set_count; first++)
            for (unsigned second = 1; second < 1u << ck_set_count; second++) {
                if (!(alive >> (first - 1) & 1) || !(alive >> (second - 1) & 1)) continue;
                unsigned x = ck_set_has_pair(node->a, first, second), y = ck_set_has_pair(node->b, first, second);
                if (x != y) equal = 0;
                if (x && !y) inside = 0;
            }
    } else {
        unsigned x = ck_set_regions(node->a) & alive, y = ck_set_regions(node->b) & alive;
        equal = x == y;
        inside = (x & ~y) == 0;
    }
    return node->kind == CK_SUBSET ? inside : node->kind == CK_NEQ ? !equal : equal;
}

/* Ritorna 1 se l'enunciato node è vero quando le regioni non vuote sono
 * quelle di alive, 0 se è falso, -1 se è scritto male. */
static int ck_set_truth(const CkNode *node, unsigned alive) {
    if (node->kind == CK_EQ || node->kind == CK_SUBSET || node->kind == CK_NEQ) return ck_set_relation(node, alive);
    if (node->kind != CK_AND && node->kind != CK_OR && node->kind != CK_IMPLIES && node->kind != CK_IFF && node->kind != CK_NOT) return -1;
    int p = ck_set_truth(node->a, alive), q = node->kind == CK_NOT ? 0 : ck_set_truth(node->b, alive);
    if (p < 0 || q < 0) return -1;
    switch (node->kind) {
    case CK_AND:     return p && q;
    case CK_OR:      return p || q;
    case CK_IMPLIES: return !p || q;
    case CK_IFF:     return p == q;
    default:         return !p;
    }
}

/* Decide l'enunciato sugli insiemi statement: lo accetta se vale per ogni
 * scelta delle regioni vuote, lo respinge se per una non vale. Allora il
 * motivo descrive insiemi per cui è falso: un elemento, chiamato con il
 * numero della sua regione, in ogni regione non vuota. */
static void ck_check_sets(const CkNode *statement) {
    static char text[256];
    ck_set_count = 0;
    ck_set_collect(statement);
    for (unsigned alive = 0; alive < 1u << ((1u << ck_set_count) - 1) && ck_status == CK_ACCEPTED; alive++) {
        int truth = ck_set_truth(statement, alive);
        if (truth < 0) ck_unsupported("enunciato sugli insiemi che il verificatore non sa leggere: tipi diversi, o parole di un altro linguaggio");
        if (truth != 0) continue;

        /* Gli insiemi del controesempio, una lettera alla volta. */
        int length = snprintf(text, sizeof text, "l'enunciato sugli insiemi è falso, per esempio con");
        for (int i = 0; i < ck_set_count; i++) {
            length += snprintf(text + length, sizeof text - (size_t)length, "%s %c = {", i ? "," : "", ck_set_names[i]);
            for (unsigned r = 1, listed = 0; r < 1u << ck_set_count; r++)
                if ((alive >> (r - 1) & 1) && (r >> i & 1)) length += snprintf(text + length, sizeof text - (size_t)length, "%s%u", listed++ ? ", " : "", r);
            length += snprintf(text + length, sizeof text - (size_t)length, "}");
        }
        ck_reject(text);
    }
}

/* =============================================================================
 * 11. INSIEMI QUALSIASI: MODELLI FINITI E DIMOSTRAZIONI PER ASSURDO
 * ============================================================================
 *
 * Gli enunciati del capitolo 28 di mathresearcher parlano di insiemi
 * qualsiasi e di simboli di cui si sa solo ciò che dicono le ipotesi:
 * elementi con un nome, applicazioni di uno o due argomenti (un'operazione
 * è un'applicazione di due), parti di un insieme e relazioni. Il
 * certificato li dichiara, una voce per riga, prima dell'enunciato:
 *
 *     sort S               un insieme; in un modello "sort S 3": tre elementi,
 *                          i numeri 0, 1, 2 (anche "sort S 0": vuoto)
 *     symbol perp S S S    gli insiemi degli argomenti, poi quello del valore,
 *     symbol u S           oppure bool per una parte o una relazione
 *     symbol R S S bool
 *
 * L'enunciato ha l'operatore davanti e tutte le parentesi, come nei capitoli
 * precedenti, ma i nomi sono parole (lettere, cifre e _):
 *
 *     termine    := nome | (nome termine [termine])
 *     condizione := true | (= termine termine) | (nome termine [termine])
 *                 | (not c) | (and c c) | (or c c) | (implies c c) | (iff c c)
 *                 | (forall x S c) | (exists x S c)
 *                 | (forallpart X S c) | (existspart X S c)
 *
 * In una condizione, (nome termine...) è una parte o una relazione
 * applicata: "x sta in X", "(x, y) sta in R". Un nome è la variabile legata
 * più interna che si chiama così, altrimenti un simbolo. Il lettore
 * controlla gli insiemi: f(x) solo con x nel dominio di f, un'uguaglianza
 * solo tra due elementi dello stesso insieme. (forallpart X S c) è "per
 * ogni parte X di S vale c", e in c (X t) dice che t sta in X (11g).
 *
 * Un'interpretazione dà a ogni insieme dei suoi elementi, quanti si vuole,
 * anche infiniti, anche nessuno; a ogni simbolo un elemento, una funzione o
 * un sottoinsieme, come dice la dichiarazione. Un quantificatore su parti
 * prova tutti i sottoinsiemi di S, compresi il vuoto e S. I certificati
 * sono di tre tipi, e ognuno dimostra un teorema diverso.
 *
 *   11a. "certificate counterexample", con una voce "table" per simbolo:
 *        un'interpretazione finita in cui l'enunciato è falso. TEOREMA: non
 *        è vero che l'enunciato vale per ogni scelta degli insiemi e dei
 *        simboli.
 *   11b. "certificate example", con le stesse voci: un'interpretazione in
 *        cui l'enunciato è vero. TEOREMA: esistono degli insiemi, con il
 *        numero di elementi indicato, e dei simboli per cui vale.
 *   11c. "certificate structure", con le voci case, instance e witness
 *        (11d), e part (11g). TEOREMA: l'enunciato vale per ogni scelta
 *        degli insiemi, finiti o infiniti, anche vuoti, e dei simboli.
 *
 * Per 11a e 11b si calcola: con insiemi finiti un quantificatore prova ogni
 * elemento (uno su parti, ogni sottoinsieme), un simbolo si legge nella sua
 * tabella, e il valore
 * dell'enunciato è un conto finito e completo. Le tabelle hanno una cella
 * per ogni scelta degli argomenti, il primo che cambia più in fretta: per
 * un'operazione su S di 2 elementi le celle sono (0,0), (1,0), (0,1), (1,1).
 * Ogni valore deve stare nel suo insieme: così un elemento con un nome in
 * un insieme vuoto, o un'applicazione da un insieme con elementi a uno
 * vuoto, non si possono scrivere, ed è giusto, perché non esistono.
 */

/* Un insieme dichiarato da una voce sort. */
typedef struct {
    char name[CK_MAX_NAME];
    int size;  /* 11a, 11b: i suoi elementi sono 0, ..., size - 1; 11c: -1 */
} CkSort;

/* Il "valore" di una parte o di una relazione: vero o falso. Occupa il posto
 * dopo l'ultimo insieme possibile. */
#define CK_BOOL CK_MAX_SORTS

/* Un simbolo: dichiarato da una voce symbol, oppure un nome nuovo dato da una
 * voce witness (11d) o part (11g), che vale solo nel suo caso. */
typedef struct {
    char name[CK_MAX_NAME];
    int arity;    /* 0, 1 o 2 argomenti */
    int arg[2];   /* gli insiemi degli argomenti: posti in ck_sorts[] */
    int result;   /* l'insieme del valore, o CK_BOOL */
    int witness;  /* 1 per un nome dato da witness o da part (11g) */
    int *table;   /* 11a, 11b: il valore di ogni cella, nell'arena; NULL finché manca la sua voce table */
} CkSymbol;

static CkSort ck_sorts[CK_MAX_SORTS];
static int ck_sort_count;
static CkSymbol ck_symbols[CK_MAX_SYMBOLS];
static int ck_symbol_count;
static int ck_declared;  /* i simboli dichiarati: i primi ck_declared di ck_symbols[] */

/* Le variabili legate visibili mentre si legge, dalla più esterna, con i
 * loro insiemi; ck_bound_part[i] è 1 se la variabile è una parte di quel
 * insieme, legata da forallpart o existspart (11g). */
static char ck_bound[CK_MAX_BOUND][CK_MAX_NAME];
static int ck_bound_sort[CK_MAX_BOUND], ck_bound_part[CK_MAX_BOUND];
static int ck_bound_count;

/* Legge un nome (una lettera, poi lettere, cifre e _) e lo mette in out.
 * Ritorna 0 se non c'è. */
static int ck_name(char *out) {
    int length = 0;
    ck_skip();
    if (!isalpha((unsigned char)*ck_cursor)) return 0;
    while (isalnum((unsigned char)*ck_cursor) || *ck_cursor == '_') {
        if (length == CK_MAX_NAME - 1) { ck_unsupported("nome troppo lungo per il verificatore"); return 0; }
        out[length++] = *ck_cursor++;
    }
    out[length] = 0;
    return 1;
}

/* Ritorna il posto dell'insieme di nome name, o -1. */
static int ck_sort_of(const char *name) {
    for (int i = 0; i < ck_sort_count; i++)
        if (strcmp(ck_sorts[i].name, name) == 0) return i;
    return -1;
}

/* Ritorna il posto del simbolo di nome name, o -1. */
static int ck_symbol_of(const char *name) {
    for (int i = 0; i < ck_symbol_count; i++)
        if (strcmp(ck_symbols[i].name, name) == 0) return i;
    return -1;
}

/* Legge un termine e ne ritorna l'albero; mette in *sort il suo insieme, o
 * CK_BOOL per una parte o una relazione applicata. Un termine scritto male,
 * o con un argomento nell'insieme sbagliato, rende il certificato
 * sbagliato. */
static const CkNode *ck_read_term(int *sort) {
    char name[CK_MAX_NAME];
    CkNode *node = ck_node(CK_APP);
    *sort = -1;
    ck_skip();
    int open = *ck_cursor == '(';
    if (open) ck_cursor++;
    if (ck_status != CK_ACCEPTED || !ck_name(name)) { ck_reject("certificato scritto male: termine atteso"); return node; }

    /* Una variabile: senza parentesi, la più interna con quel nome. Con le
     * parentesi, una parte legata applicata a un termine (11g). Una parte
     * legata non è un elemento: scritta senza parentesi è un errore. */
    for (int i = ck_bound_count - 1; i >= 0; i--) {
        if (strcmp(ck_bound[i], name) != 0) continue;
        if (ck_bound_part[i] != open) {
            if (!open) { ck_reject("una parte legata è scritta dove serve un elemento"); return node; }
            break;
        }
        node->value = ck_bound_count - 1 - i;
        if (!open) {
            node->kind = CK_VAR;
            *sort = ck_bound_sort[i];
            return node;
        }
        int got;
        if (ck_depth >= CK_MAX_DEPTH) { ck_unsupported("espressione con troppe parentesi una dentro l'altra"); return node; }
        node->kind = CK_PARTVAR;
        ck_depth++;
        node->a = ck_read_term(&got);
        ck_depth--;
        if (got != ck_bound_sort[i]) ck_reject("un argomento non sta nell'insieme che il simbolo chiede");
        *sort = CK_BOOL;
        ck_skip();
        if (*ck_cursor != ')') ck_reject("certificato scritto male: parentesi chiusa attesa");
        else ck_cursor++;
        return node;
    }

    /* Un simbolo, con i suoi argomenti. */
    int s = ck_symbol_of(name);
    if (s < 0) { ck_reject("il certificato nomina un simbolo che non è dichiarato"); return node; }
    if (open != (ck_symbols[s].arity > 0)) { ck_reject("un simbolo è scritto con un numero di argomenti diverso da quello dichiarato"); return node; }
    if (ck_depth >= CK_MAX_DEPTH) { ck_unsupported("espressione con troppe parentesi una dentro l'altra"); return node; }
    node->value = s;
    *sort = ck_symbols[s].result;
    ck_depth++;
    for (int i = 0; i < ck_symbols[s].arity; i++) {
        int got;
        const CkNode *argument = ck_read_term(&got);
        if (got != ck_symbols[s].arg[i]) ck_reject("un argomento non sta nell'insieme che il simbolo chiede");
        if (i == 0) node->a = argument; else node->b = argument;
    }
    ck_depth--;
    ck_skip();
    if (!open) return node;
    if (*ck_cursor != ')') ck_reject("certificato scritto male: parentesi chiusa attesa");
    else ck_cursor++;
    return node;
}

/* Legge una condizione e ne ritorna l'albero (la grammatica è in testa al
 * capitolo). */
static const CkNode *ck_read_condition(void) {
    static const struct { const char *word; CkKind kind; int operands; } table[] = {
        {"not", CK_NOT, 1}, {"and", CK_AND, 2}, {"or", CK_OR, 2}, {"implies", CK_IMPLIES, 2}, {"iff", CK_IFF, 2},
        {"forall", CK_FORALL, 0}, {"exists", CK_EXISTS, 0}, {"forallpart", CK_FORALLPART, 0}, {"existspart", CK_EXISTSPART, 0},
    };
    char word[CK_MAX_NAME];
    CkNode *node = ck_node(CK_TRUE);
    ck_skip();
    const char *start = ck_cursor;
    if (ck_status != CK_ACCEPTED) return node;
    if (*ck_cursor != '(') {
        if (!ck_name(word) || strcmp(word, "true") != 0) ck_reject("certificato scritto male: condizione attesa");
        return node;
    }
    if (ck_depth >= CK_MAX_DEPTH) { ck_unsupported("espressione con troppe parentesi una dentro l'altra"); return node; }
    ck_cursor++;
    ck_skip();
    ck_depth++;
    int found = -1, sort[2];
    if (*ck_cursor == '=') {
        /* Un'uguaglianza tra due elementi dello stesso insieme. */
        ck_cursor++;
        node->kind = CK_EQ;
        node->a = ck_read_term(&sort[0]);
        node->b = ck_read_term(&sort[1]);
        if (sort[0] != sort[1] || sort[0] == CK_BOOL) ck_reject("un'uguaglianza tra elementi di insiemi diversi");
    } else if (!ck_name(word)) {
        ck_reject("certificato scritto male: condizione attesa");
    } else {
        for (int i = 0; i < (int)(sizeof table / sizeof *table); i++)
            if (strcmp(table[i].word, word) == 0) found = i;
    }
    if (found < 0 && node->kind == CK_TRUE && ck_status == CK_ACCEPTED) {
        /* Una parte o una relazione applicata: si rilegge come termine. */
        ck_cursor = start;
        ck_depth--;
        node->kind = CK_EQ;
        node->a = ck_read_term(&sort[0]);
        if (sort[0] != CK_BOOL) ck_reject("serve una condizione, e il certificato scrive un elemento");
        return node;
    }
    if (found >= 0) {
        node->kind = table[found].kind;
        if (table[found].operands == 0) {
            /* (forall x S c): la variabile e il suo insieme restano in vista
             * mentre si legge c. Lo stesso per una parte (11g). */
            char set[CK_MAX_NAME];
            if (ck_bound_count == CK_MAX_BOUND) { ck_unsupported("troppe variabili legate una dentro l'altra"); return node; }
            if (!ck_name(ck_bound[ck_bound_count]) || !ck_name(set) || (node->value = ck_sort_of(set)) < 0) {
                ck_reject("certificato scritto male: dopo forall ed exists servono una variabile e un insieme dichiarato");
                return node;
            }
            ck_bound_part[ck_bound_count] = node->kind == CK_FORALLPART || node->kind == CK_EXISTSPART;
            ck_bound_sort[ck_bound_count++] = (int)node->value;
            node->a = ck_read_condition();
            ck_bound_count--;
        } else {
            node->a = ck_read_condition();
            if (table[found].operands == 2) node->b = ck_read_condition();
        }
    }
    ck_depth--;
    ck_skip();
    if (*ck_cursor != ')') ck_reject("certificato scritto male: parentesi chiusa attesa");
    else ck_cursor++;
    return node;
}

/* 11a, 11b. I valori delle variabili legate, dalla più esterna, e quanti
 * elementi i quantificatori hanno provato. Il valore di una parte legata
 * (11g) è un numero di bit: il bit x acceso se l'elemento x sta nella parte;
 * gli elementi sono al più CK_MAX_SIZE, quindi i bit bastano. */
static int ck_values[CK_MAX_BOUND];
static long long ck_tried;

/* Ritorna il valore del termine t nel modello, con depth variabili legate;
 * per una parte legata applicata, 1 se l'argomento sta nella parte. */
static int ck_value(const CkNode *t, int depth) {
    if (t->kind == CK_VAR) return ck_values[depth - 1 - t->value];
    if (t->kind == CK_PARTVAR) return ck_values[depth - 1 - t->value] >> ck_value(t->a, depth) & 1;
    const CkSymbol *s = &ck_symbols[t->value];
    int cell = s->arity > 0 ? ck_value(t->a, depth) : 0;
    if (s->arity > 1) cell += ck_value(t->b, depth) * ck_sorts[s->arg[0]].size;
    return s->table[cell];
}

/* Ritorna 1 se la condizione c vale nel modello, con depth variabili legate.
 * Un quantificatore prova ogni elemento del suo insieme: oltre CK_MAX_EVAL
 * prove il certificato non è controllabile. */
static int ck_holds(const CkNode *c, int depth) {
    switch (c->kind) {
    case CK_TRUE:    return 1;
    case CK_EQ:      return c->b ? ck_value(c->a, depth) == ck_value(c->b, depth) : ck_value(c->a, depth);
    case CK_NOT:     return !ck_holds(c->a, depth);
    case CK_AND:     return ck_holds(c->a, depth) && ck_holds(c->b, depth);
    case CK_OR:      return ck_holds(c->a, depth) || ck_holds(c->b, depth);
    case CK_IMPLIES: return !ck_holds(c->a, depth) || ck_holds(c->b, depth);
    case CK_IFF:     return ck_holds(c->a, depth) == ck_holds(c->b, depth);
    default:         break;
    }

    /* Un quantificatore prova ogni elemento; uno su parti (11g) ogni
     * sottoinsieme, come numero di bit, da 0 (il vuoto) a 2^n - 1 (tutto). */
    int every = c->kind == CK_FORALL || c->kind == CK_FORALLPART, parts = c->kind == CK_FORALLPART || c->kind == CK_EXISTSPART;
    int values = parts ? 1 << ck_sorts[c->value].size : ck_sorts[c->value].size;
    for (int x = 0; x < values && ck_status == CK_ACCEPTED; x++) {
        if (++ck_tried > CK_MAX_EVAL) { ck_unsupported("il modello è troppo grande per calcolarvi l'enunciato"); return 0; }
        ck_values[depth] = x;
        if (ck_holds(c->a, depth + 1) != every) return !every;
    }
    return every;
}

/* 11d. LA DIMOSTRAZIONE PER ASSURDO, CON I SOLI ELEMENTI NOMINATI
 *
 * Il certificato 11c porta i dati di una dimostrazione per assurdo: si
 * suppone che l'enunciato E sia falso, e se ne trae una contraddizione. Le
 * voci, dopo l'enunciato, sono di tre tipi:
 *
 *   case A           comincia un caso: si suppone A, una condizione chiusa
 *                    sui simboli dichiarati;
 *   instance Q t...  Q è una condizione già comparsa nel caso, che comincia
 *                    con dei "per ogni" (o con degli "esiste"), e t... sono
 *                    termini chiusi, uno per quantificatore, ognuno nel suo
 *                    insieme. Se ne trae "Q implica C(t...)", dove C(t...)
 *                    è ciò che segue i quantificatori, con i termini al
 *                    posto delle variabili; per gli "esiste", "C(t...)
 *                    implica Q";
 *   witness Q c      Q comincia con un "esiste x in S", e c è un nome che
 *                    nel caso non c'è ancora: diventa un elemento di S, e se
 *                    ne trae "Q implica C(c)". Per un "per ogni x in S",
 *                    "C(c) implica Q": c è un elemento per cui C non vale.
 *
 * Il verificatore calcola da sé la sostituzione, quindi un'istanza scritta
 * con il termine sbagliato non è un'istanza sbagliata: è l'istanza di un
 * altro termine, o di una condizione che non è comparsa, e allora è
 * respinta. Un termine chiuso è fatto solo di simboli dichiarati e di nomi
 * già dati: nessun elemento inventato.
 *
 * Perché le condizioni tratte sono vere. Si fissi un'interpretazione M in
 * cui E è falso, e in cui gli insiemi vuoti sono quelli di un insieme Z.
 *
 *   - Un termine chiuso indica sempre un elemento: i simboli dichiarati
 *     hanno un valore in M, e i nomi lo ricevono quando nascono. Quindi da
 *     "per ogni x in S, C(x)" segue C(t) per ogni t chiuso di S.
 *   - Un nome c nato da Q = "esiste x in S, C(x)": se Q è vera in M, a c si
 *     dà un elemento che la rende vera; se è falsa, un elemento qualsiasi di
 *     S. In tutti e due i casi "Q implica C(c)" è vera, e c non compariva
 *     prima, quindi dargli un valore non cambia nulla di ciò che c'era. Ma
 *     se S è vuoto c non può ricevere nulla: per questo il controllo si
 *     ripete per ogni Z (11f), e quando S sta in Z le voci che nominano un
 *     elemento di S sono lasciate fuori.
 *
 * Il caso giusto. Gli A dei case devono coprire ogni possibilità: "E falso"
 * e "nessun A" insieme devono essere contraddittori, e anche questo si
 * controlla (11f, alla fine). Allora in M vale uno degli A, e le condizioni
 * di quel caso sono tutte vere in M.
 *
 * 11e. LA CONTRADDIZIONE: UN CONTO FINITO E COMPLETO
 *
 * Le condizioni di un caso sono chiuse: si costruiscono con and, or, not,
 * implies e iff a partire da condizioni elementari, che sono di due tipi:
 * le uguaglianze tra termini chiusi (anche "x sta in X", che è X(x) =
 * vero, con un termine "vero" in più) e le condizioni chiuse che cominciano
 * con un quantificatore, trattate come lettere: due sono la stessa solo se
 * sono scritte allo stesso modo, a meno del nome delle variabili legate.
 * Ogni scelta di vero o falso per le condizioni elementari si prova:
 *
 *   - se rende falsa una delle condizioni del caso, è scartata;
 *   - altrimenti si guardano le uguaglianze vere: con la CHIUSURA PER
 *     CONGRUENZA (se a = b allora f(a) = f(b), anche per X(a) = X(b)) se ne
 *     ricavano le classi dei termini uguali, e la scelta è scartata se
 *     un'uguaglianza falsa ha i due termini nella stessa classe.
 *
 * Se ogni scelta è scartata, il caso è impossibile. Perché è un conto
 * completo: in M ogni condizione elementare è vera o falsa, e la scelta che
 * M fa rende vere tutte le condizioni del caso e non è scartata, perché in
 * M uguali al posto di uguali danno uguali. Quindi se M ci fosse, una
 * scelta resterebbe. Le condizioni elementari sono in numero finito, e così
 * le scelte.
 *
 * Le scelte sono 2^k, ma non si elencano una per una: si assegnano le
 * condizioni elementari una alla volta, e una scelta parziale è scartata
 * appena una condizione del caso risulta falsa qualunque cosa valgano le
 * altre, o appena la chiusura mette insieme i due termini di
 * un'uguaglianza falsa. Un'uguaglianza tra termini che la chiusura ha già
 * messo nella stessa classe vale vero senza sceglierla: in M lo è di
 * sicuro. Se le condizioni del caso sono tutte vere prima di aver scelto
 * tutto, le scelte mancanti si completano (vero alle uguaglianze dentro una
 * classe, falso alle altre) senza scartare: c'è una scelta che resta. La
 * prossima condizione elementare da scegliere si prende dalla condizione
 * del caso con meno incognite: è la potatura che fa sì che i passi forzati
 * non moltiplichino i casi. Oltre CK_MAX_SEARCH scelte parziali il
 * certificato non è controllabile.
 *
 * 11f. GLI INSIEMI VUOTI
 *
 * Per ogni Z, insieme degli insiemi vuoti, il conto di 11e si rifà così:
 *
 *   - una condizione elementare "per ogni x in S" con S in Z vale vero, una
 *     "esiste x in S" vale falso: in un insieme vuoto è così;
 *   - le voci instance e witness che nominano un elemento di un insieme di
 *     Z (un nome di Z, o un termine con un nome di Z dentro, anche tra i
 *     termini di un'istanza che poi non compaiono) sono lasciate fuori: in
 *     M quell'elemento non c'è.
 *
 * Le voci che restano sono vere in M come detto in 11d, perché ogni loro
 * termine chiuso indica un elemento di M: è fatto di simboli dichiarati,
 * che in M hanno un valore, e di nomi di insiemi fuori da Z, che lo
 * ricevono. Un termine chiuso di un insieme di Z senza nomi di Z indicherebbe
 * un elemento di un insieme vuoto: quando ce n'è uno, nessuna M ha vuoti
 * proprio gli insiemi di Z, e il conto per quel Z non serve, ma non fa
 * danno. Così "valido anche per gli insiemi vuoti" non è un argomento: è un
 * conto.
 *
 * 11g. LE PARTI
 *
 * L'enunciato può quantificare sulle parti di un insieme: (forallpart X S
 * c), "per ogni sottoinsieme X di S vale c", e (existspart X S c). In M una
 * parte di S è un sottoinsieme qualsiasi di S, vuoto e S compresi. Le voci
 * del 11d valgono anche qui, con una parte al posto di un elemento, e ce
 * n'è una in più:
 *
 *   instance Q D...  Q comincia con dei forallpart (o degli existspart), e
 *                    D... sono nomi di parti, uno per quantificatore, ognuno
 *                    una parte dell'insieme giusto: un simbolo dichiarato
 *                    "symbol D S bool", o un nome dato in questo caso. Se ne
 *                    trae "Q implica C(D...)", con (D t) al posto di (X t)
 *                    (per gli existspart, "C(D...) implica Q");
 *   witness Q C      Q comincia con un existspart (o, negato, con un
 *                    forallpart): C è un nome nuovo, una parte di S;
 *   part D x S c     D è un nome nuovo, la parte degli x di S per cui vale c,
 *                    una condizione in cui l'unica variabile libera è x: se
 *                    ne trae "per ogni x di S, (D x) se e solo se c".
 *
 * Perché le condizioni tratte sono vere in M, come in 11d. Un'istanza: in M
 * ogni nome di parte indica un sottoinsieme di S, e ciò che vale per ogni
 * sottoinsieme vale per quello. Un witness: se Q è vera c'è una parte che la
 * rende vera, e la si dà a C; se è falsa, a C se ne dà una qualsiasi. Una
 * voce part: è l'assioma di separazione, che dà, per ogni insieme S e ogni
 * condizione c, il sottoinsieme degli x di S per cui c vale; D non compariva
 * prima, quindi dargli quel valore non cambia nulla di ciò che c'era.
 *
 * Gli insiemi vuoti (11f). Una parte c'è sempre, anche di un insieme vuoto:
 * è il vuoto. Per questo un nome di parte non porta il bit del suo insieme,
 * e le voci che lo nominano restano dentro qualunque sia Z: valgono per
 * qualunque valore di quel nome, anche quando la voce che glielo dava è
 * stata lasciata fuori. Una voce part con un nome di un insieme di Z nella
 * sua condizione, come \{a\}, è lasciata fuori come ogni altra: in M a non
 * c'è. Un forallpart su un insieme vuoto non vale vero da sé, come un
 * forall: il vuoto è una parte, e la condizione va controllata su di lui. */

/* Un termine chiuso, come lo vede la chiusura per congruenza: un simbolo
 * applicato ad altri termini chiusi. Il posto 0 è "vero", il valore di una
 * parte o di una relazione che vale. */
typedef struct {
    int symbol;  /* posto in ck_symbols[]; -1 per "vero" */
    int arg[2];  /* gli argomenti, posti in ck_grounds[]; -1 dove mancano */
} CkGround;

/* Una condizione elementare di un caso (11e): un'uguaglianza tra due
 * termini chiusi, oppure una condizione chiusa che comincia con un
 * quantificatore. value è il valore scelto dalla ricerca. */
typedef struct {
    const CkNode *quantified;  /* la condizione quantificata, o NULL per un'uguaglianza */
    int left, right;           /* uguaglianza: i due termini, posti in ck_grounds[], left <= right */
    signed char value;         /* 1, 0, oppure -1 se non è ancora scelto */
} CkAtom;

/* I termini chiusi e le condizioni elementari del caso in corso; le sue
 * condizioni, come scheletri (CK_ATOM al posto delle condizioni
 * elementari), ognuna con gli insiemi dei nomi che vi compaiono (bit i:
 * l'insieme di posto i, 11f); il rappresentante della classe di ogni
 * termine (11e). */
static CkGround ck_grounds[CK_MAX_GROUND];
static int ck_ground_count;
static CkAtom ck_atoms[CK_MAX_ATOMS];
static int ck_atom_count;
static const CkNode *ck_formulas[CK_MAX_FORMULAS];
static unsigned ck_masks[CK_MAX_FORMULAS];
static int ck_formula_count;
static int ck_rep[CK_MAX_GROUND];
static long long ck_nodes;

/* Ritorna 1 se la condizione c comincia con un quantificatore, su elementi
 * o su parti (11g). */
static int ck_quantifier(const CkNode *c) {
    return c->kind == CK_FORALL || c->kind == CK_EXISTS || c->kind == CK_FORALLPART || c->kind == CK_EXISTSPART;
}

/* Ritorna il posto del termine chiuso t in ck_grounds[], aggiungendolo se
 * manca: due termini scritti allo stesso modo hanno lo stesso posto. */
static int ck_ground_of(const CkNode *t) {
    int a = t->a ? ck_ground_of(t->a) : -1, b = t->b ? ck_ground_of(t->b) : -1;
    for (int i = 0; i < ck_ground_count; i++)
        if (ck_grounds[i].symbol == t->value && ck_grounds[i].arg[0] == a && ck_grounds[i].arg[1] == b) return i;
    if (ck_ground_count == CK_MAX_GROUND) { ck_unsupported("troppi termini per il verificatore"); return 0; }
    ck_grounds[ck_ground_count] = (CkGround){(int)t->value, {a, b}};
    return ck_ground_count++;
}

/* Ritorna lo scheletro della condizione chiusa c: lo stesso albero, con un
 * nodo CK_ATOM al posto di ogni condizione elementare, che viene messa in
 * ck_atoms[] se non c'è già. */
static const CkNode *ck_skeleton(const CkNode *c) {
    CkNode *s = ck_node(c->kind);
    if (ck_status != CK_ACCEPTED || c->kind == CK_TRUE) return s;
    if (c->kind != CK_EQ && !ck_quantifier(c)) {
        s->a = ck_skeleton(c->a);
        if (c->b) s->b = ck_skeleton(c->b);
        return s;
    }
    CkAtom atom = {c->kind == CK_EQ ? NULL : c, 0, 0, -1};
    if (c->kind == CK_EQ) {
        atom.left = ck_ground_of(c->a);
        atom.right = c->b ? ck_ground_of(c->b) : 0;
        if (atom.left > atom.right) { int swap = atom.left; atom.left = atom.right; atom.right = swap; }
    }
    s->kind = CK_ATOM;
    for (s->value = 0; s->value < ck_atom_count; s->value++) {
        const CkAtom *old = &ck_atoms[s->value];
        if (atom.quantified ? old->quantified && ck_same(old->quantified, c) : !old->quantified && old->left == atom.left && old->right == atom.right) return s;
    }
    if (ck_atom_count == CK_MAX_ATOMS) { ck_unsupported("troppe condizioni elementari per il verificatore"); return s; }
    ck_atoms[ck_atom_count++] = atom;
    return s;
}

/* Aggiunge la condizione chiusa c a quelle del caso; mask dice gli insiemi
 * dei nomi che nomina (11f). */
static void ck_suppose(const CkNode *c, unsigned mask) {
    if (ck_formula_count == CK_MAX_FORMULAS) { ck_unsupported("troppe condizioni in un caso"); return; }
    ck_masks[ck_formula_count] = mask;
    ck_formulas[ck_formula_count++] = ck_skeleton(c);
}

/* Ritorna, come bit, gli insiemi dei nomi di elementi dati da witness che
 * compaiono nell'albero t. Un nome di parte non porta bit: una parte c'è
 * sempre, anche di un insieme vuoto (11g). */
static unsigned ck_names(const CkNode *t) {
    if (!t) return 0;
    const CkSymbol *s = t->kind == CK_APP ? &ck_symbols[t->value] : NULL;
    unsigned here = s && s->witness && s->result != CK_BOOL ? 1u << s->result : 0;
    return here | ck_names(t->a) | ck_names(t->b);
}

/* Ritorna la condizione (o il termine) c con il termine chiuso t al posto
 * della variabile legata dal quantificatore che sta depth quantificatori
 * sopra c, in un albero nuovo. Le altre variabili restano come sono: sono
 * legate da quantificatori dentro c, perché la condizione da cui si parte è
 * chiusa, e t non ne contiene. */
static const CkNode *ck_subst(const CkNode *c, long long depth, const CkNode *t) {
    if (!c) return NULL;
    if (c->kind == CK_VAR) return c->value == depth ? t : c;
    CkNode *copy = ck_node(c->kind);
    *copy = *c;
    copy->a = ck_subst(c->a, depth + ck_quantifier(c), t);
    copy->b = ck_subst(c->b, depth, t);
    return copy;
}

/* 11g. Ritorna la condizione c con la parte di nome symbol al posto della
 * parte legata dal quantificatore che sta depth quantificatori sopra c: ogni
 * (X t) diventa (symbol t), in un albero nuovo. Come in ck_subst(), le altre
 * variabili restano come sono. */
static const CkNode *ck_subst_part(const CkNode *c, long long depth, int symbol) {
    if (!c) return NULL;
    CkNode *copy = ck_node(c->kind);
    *copy = *c;
    if (c->kind == CK_PARTVAR && c->value == depth) {
        copy->kind = CK_APP;
        copy->value = symbol;
    }
    copy->a = ck_subst_part(c->a, depth + ck_quantifier(c), symbol);
    copy->b = ck_subst_part(c->b, depth, symbol);
    return copy;
}

/* Legge la condizione Q di una voce instance o witness. Deve cominciare con
 * un quantificatore ed essere già comparsa nel caso, come condizione
 * elementare. */
static const CkNode *ck_read_available(void) {
    const CkNode *q = ck_read_condition();
    if (ck_status != CK_ACCEPTED) return q;
    if (!ck_quantifier(q)) { ck_reject("instance e witness vogliono una condizione che comincia con un quantificatore"); return q; }
    for (int i = 0; i < ck_atom_count; i++)
        if (ck_atoms[i].quantified && ck_same(ck_atoms[i].quantified, q)) return q;
    ck_reject("instance o witness di una condizione che nel caso non è comparsa");
    return q;
}

/* Ritorna la condizione "premise implica conclusion". */
static const CkNode *ck_implies(const CkNode *premise, const CkNode *conclusion) {
    CkNode *clause = ck_node(CK_IMPLIES);
    clause->a = premise;
    clause->b = conclusion;
    return clause;
}

/* 11g. Legge il nome di una parte dell'insieme sort: un simbolo, dichiarato
 * o dato nel caso, con un solo argomento in sort e valore vero o falso.
 * Ritorna il suo posto, o -1 (e il certificato è respinto). */
static int ck_read_part(int sort) {
    char name[CK_MAX_NAME];
    int s = ck_name(name) ? ck_symbol_of(name) : -1;
    if (s < 0 || ck_symbols[s].arity != 1 || ck_symbols[s].arg[0] != sort || ck_symbols[s].result != CK_BOOL) {
        ck_reject("un'istanza su parti vuole il nome di una parte dell'insieme giusto");
        return -1;
    }
    return s;
}

/* La voce instance Q t1 ... tk (11d); per i quantificatori su parti, i
 * nomi delle parti (11g). */
static void ck_instance(void) {
    const CkNode *q = ck_read_available(), *body = q;
    unsigned mask = ck_names(q);
    int count = 0, parts = q->kind == CK_FORALLPART || q->kind == CK_EXISTSPART;
    for (ck_skip(); ck_status == CK_ACCEPTED && *ck_cursor && *ck_cursor != '\n'; ck_skip(), count++) {
        if (body->kind != q->kind) { ck_reject("un'istanza ha un termine di troppo, o in un altro insieme"); return; }
        if (parts) {
            int part = ck_read_part((int)body->value);
            if (part < 0) return;
            body = ck_subst_part(body->a, 0, part);
            continue;
        }
        int sort;
        const CkNode *t = ck_read_term(&sort);
        if (sort != body->value) { ck_reject("un'istanza ha un termine di troppo, o in un altro insieme"); return; }
        mask |= ck_names(t);
        body = ck_subst(body->a, 0, t);
    }
    if (ck_status != CK_ACCEPTED) return;
    if (count == 0) { ck_reject("un'istanza senza termini"); return; }
    ck_suppose(q->kind == CK_FORALL || q->kind == CK_FORALLPART ? ck_implies(q, body) : ck_implies(body, q), mask);
}

/* La voce witness Q c (11d). Per un quantificatore su parti c è una parte
 * nuova del suo insieme (11g), che non porta il bit di quell'insieme: una
 * parte c'è anche quando l'insieme è vuoto. */
static void ck_witness(void) {
    const CkNode *q = ck_read_available();
    if (ck_status != CK_ACCEPTED) return;
    if (ck_symbol_count == CK_MAX_SYMBOLS) { ck_unsupported("troppi nomi in un caso"); return; }
    CkSymbol *c = &ck_symbols[ck_symbol_count];
    memset(c, 0, sizeof *c);
    if (!ck_name(c->name) || ck_symbol_of(c->name) >= 0) { ck_reject("witness: serve un nome che nel caso non c'è ancora"); return; }
    c->witness = 1;
    if (q->kind == CK_FORALLPART || q->kind == CK_EXISTSPART) {
        c->arity = 1;
        c->arg[0] = (int)q->value;
        c->result = CK_BOOL;
        const CkNode *body = ck_subst_part(q->a, 0, ck_symbol_count++);
        ck_suppose(q->kind == CK_EXISTSPART ? ck_implies(q, body) : ck_implies(body, q), ck_names(q));
        return;
    }
    c->result = (int)q->value;
    CkNode *t = ck_node(CK_APP);
    t->value = ck_symbol_count++;
    const CkNode *body = ck_subst(q->a, 0, t);
    ck_suppose(q->kind == CK_EXISTS ? ck_implies(q, body) : ck_implies(body, q), ck_names(q) | 1u << q->value);
}

/* 11g. La voce part D x S c: D è la parte degli x di S per cui vale c, e se
 * ne trae "per ogni x di S, (D x) se e solo se c", scritta come la scrive il
 * lettore, così che le istanze del motore la ritrovino (ck_same()). D è un
 * nome nuovo, registrato solo dopo aver letto c: c non lo può nominare. Le
 * variabili libere di c possono essere solo x, perché il lettore non conosce
 * altre variabili legate. */
static void ck_part(void) {
    char set[CK_MAX_NAME];
    if (ck_symbol_count == CK_MAX_SYMBOLS) { ck_unsupported("troppi nomi in un caso"); return; }
    CkSymbol *d = &ck_symbols[ck_symbol_count];
    memset(d, 0, sizeof *d);
    if (!ck_name(d->name) || ck_symbol_of(d->name) >= 0) { ck_reject("part: serve un nome che nel caso non c'è ancora"); return; }
    int sort = !ck_name(ck_bound[0]) || !ck_name(set) ? -1 : ck_sort_of(set);
    if (sort < 0) { ck_reject("certificato scritto male: dopo part servono il nome, una variabile e un insieme dichiarato"); return; }
    ck_bound_sort[0] = sort;
    ck_bound_part[0] = 0;
    ck_bound_count = 1;
    const CkNode *rule = ck_read_condition();
    ck_bound_count = 0;
    if (ck_status != CK_ACCEPTED) return;
    d->arity = 1;
    d->arg[0] = sort;
    d->result = CK_BOOL;
    d->witness = 1;

    /* (forall x S (iff (D x) c)), nodo per nodo come lo costruisce il
     * lettore: x è la variabile a distanza 0. */
    CkNode *x = ck_node(CK_VAR), *member = ck_node(CK_APP), *holds = ck_node(CK_EQ), *iff = ck_node(CK_IFF), *every = ck_node(CK_FORALL);
    member->value = ck_symbol_count++;
    member->a = x;
    holds->a = member;
    iff->a = holds;
    iff->b = rule;
    every->value = sort;
    every->a = iff;
    ck_suppose(every, ck_names(rule));
}

/* Ritorna il rappresentante della classe del termine t, accorciando la
 * strada per le volte dopo. */
static int ck_find(int t) {
    while (ck_rep[t] != t) t = ck_rep[t] = ck_rep[ck_rep[t]];
    return t;
}

/* 11e. La chiusura per congruenza delle uguaglianze scelte vere: al ritorno
 * due termini hanno lo stesso rappresentante se e solo se la loro
 * uguaglianza segue da quelle, con uguali al posto di uguali.
 *
 * Si uniscono le classi delle uguaglianze vere; poi, a ogni giro, ogni
 * termine entra in una tabella con la sua "firma", il simbolo e le classi
 * degli argomenti: due termini con la stessa firma sono uguali, e se stanno
 * in classi diverse le si unisce. Un'unione cambia le firme, e allora il
 * giro ricomincia; finisce quando un giro intero non unisce nulla. */
static void ck_closure(void) {
    static int table[2 * CK_MAX_GROUND];
    unsigned size = 1;
    while (size < 2u * (unsigned)ck_ground_count) size *= 2;
    for (int t = 0; t < ck_ground_count; t++) ck_rep[t] = t;
    for (int i = 0; i < ck_atom_count; i++)
        if (!ck_atoms[i].quantified && ck_atoms[i].value == 1) ck_rep[ck_find(ck_atoms[i].left)] = ck_find(ck_atoms[i].right);
    for (int changed = 1; changed;) {
        changed = 0;
        for (unsigned h = 0; h < size; h++) table[h] = -1;
        for (int t = 1; t < ck_ground_count; t++) {
            const CkGround *g = &ck_grounds[t];
            if (g->arg[0] < 0) continue;
            int a = ck_find(g->arg[0]), b = g->arg[1] < 0 ? -1 : ck_find(g->arg[1]);
            unsigned h = ((unsigned)g->symbol * 31u + (unsigned)a * 2053u + (unsigned)(b + 1)) & (size - 1);
            for (; table[h] >= 0; h = (h + 1) & (size - 1)) {
                const CkGround *o = &ck_grounds[table[h]];
                if (o->symbol == g->symbol && ck_find(o->arg[0]) == a && (o->arg[1] < 0 ? -1 : ck_find(o->arg[1])) == b) break;
            }
            if (table[h] < 0) table[h] = t;
            else if (ck_find(table[h]) != ck_find(t)) { ck_rep[ck_find(t)] = ck_find(table[h]); changed = 1; }
        }
    }
}

/* Ritorna il valore della condizione elementare atom per la scelta in
 * corso: quello scelto; se non è scelto, vero per un'uguaglianza tra due
 * termini della stessa classe e per un "per ogni" su un insieme vuoto,
 * falso per un "esiste" su un insieme vuoto, altrimenti -1. Gli insiemi con
 * il bit in empty sono vuoti (11f), e lì una condizione quantificata non si
 * sceglie. */
static int ck_atom_value(const CkAtom *atom, unsigned empty) {
    if (atom->value >= 0) return atom->value;
    if (!atom->quantified) return ck_find(atom->left) == ck_find(atom->right) ? 1 : -1;

    /* Su parti no (11g): anche un insieme vuoto ha una parte, il vuoto. */
    const CkNode *q = atom->quantified;
    return (q->kind == CK_FORALL || q->kind == CK_EXISTS) && empty >> q->value & 1 ? q->kind == CK_FORALL : -1;
}

/* Ritorna il valore dello scheletro s per la scelta in corso: 1, 0, oppure
 * -1 se dipende da condizioni elementari non ancora scelte. Conta queste
 * ultime in *unknown, e mette in *pick la prima incontrata. */
static int ck_truth(const CkNode *s, unsigned empty, int *unknown, int *pick) {
    if (s->kind == CK_TRUE) return 1;
    if (s->kind == CK_ATOM) {
        int x = ck_atom_value(&ck_atoms[s->value], empty);
        if (x < 0 && (*unknown)++ == 0) *pick = (int)s->value;
        return x;
    }
    int x = ck_truth(s->a, empty, unknown, pick);
    if (s->kind == CK_NOT) return x < 0 ? -1 : !x;

    /* Il primo operando può bastare: falso in una congiunzione, vero in una
     * disgiunzione, falso come premessa. */
    if ((s->kind == CK_AND && x == 0) || (s->kind == CK_OR && x == 1) || (s->kind == CK_IMPLIES && x == 0)) return s->kind != CK_AND;
    int y = ck_truth(s->b, empty, unknown, pick);
    switch (s->kind) {
    case CK_AND:     return y == 0 ? 0 : x == 1 && y == 1 ? 1 : -1;
    case CK_OR:      return y == 1 ? 1 : x == 0 && y == 0 ? 0 : -1;
    case CK_IMPLIES: return y == 1 ? 1 : x == 1 && y == 0 ? 0 : -1;
    default:         return x < 0 || y < 0 ? -1 : x == y;
    }
}

/* 11e. Ritorna 1 se la scelta in corso si può completare in una che rende
 * vere tutte le condizioni del caso senza essere scartata; 0 se no. Le
 * condizioni con un nome di un insieme vuoto (bit in empty) sono lasciate
 * fuori (11f). Se la ricerca supera CK_MAX_SEARCH ritorna 0, e il
 * certificato non è controllabile. */
static int ck_consistent(unsigned empty) {
    int pick = -1, fewest = 0;
    if (++ck_nodes > CK_MAX_SEARCH) { ck_unsupported("troppe scelte da provare per il verificatore"); return 0; }
    ck_closure();
    for (int i = 0; i < ck_atom_count; i++)
        if (!ck_atoms[i].quantified && ck_atoms[i].value == 0 && ck_find(ck_atoms[i].left) == ck_find(ck_atoms[i].right)) return 0;

    /* Una condizione falsa scarta la scelta; tra quelle che non si sanno, la
     * prossima condizione elementare viene da quella con meno incognite. */
    for (int f = 0; f < ck_formula_count; f++) {
        int unknown = 0, first = -1;
        if (ck_masks[f] & empty) continue;
        int truth = ck_truth(ck_formulas[f], empty, &unknown, &first);
        if (truth == 0) return 0;
        if (truth < 0 && (pick < 0 || unknown < fewest)) { pick = first; fewest = unknown; }
    }
    if (pick < 0) return 1;
    for (int value = 1; value >= 0 && ck_status == CK_ACCEPTED; value--) {
        ck_atoms[pick].value = (signed char)value;
        if (ck_consistent(empty)) { ck_atoms[pick].value = -1; return 1; }
    }
    ck_atoms[pick].value = -1;
    return 0;
}

/* Svuota il caso, e i nomi dati nel precedente. */
static void ck_case_start(void) {
    ck_symbol_count = ck_declared;
    ck_grounds[0] = (CkGround){-1, {-1, -1}};
    ck_ground_count = 1;
    ck_atom_count = ck_formula_count = 0;
}

/* 11f. Controlla che il caso appena letto sia impossibile per ogni scelta Z
 * degli insiemi vuoti (bit in empty). Se non lo è, il certificato è
 * sbagliato, e il motivo dice quali insiemi erano vuoti. */
static void ck_check_case(void) {
    static char text[128];
    for (unsigned empty = 0; empty < 1u << ck_sort_count && ck_status == CK_ACCEPTED; empty++) {
        if (!ck_consistent(empty)) continue;
        int length = snprintf(text, sizeof text, "le condizioni di un caso non sono contraddittorie%s", empty ? ", con vuoti:" : "");
        for (int i = 0; i < ck_sort_count; i++)
            if (empty >> i & 1) length += snprintf(text + length, sizeof text - (size_t)length, " %s", ck_sorts[i].name);
        ck_reject(text);
    }
}

/* Legge e controlla un certificato su insiemi qualsiasi, dalla riga dopo
 * l'intestazione fino a "end" compreso. kind è 0 per counterexample (11a),
 * 1 per example (11b), 2 per structure (11c). */
static void ck_check_structure(int kind) {
    static const CkNode *assumed[CK_MAX_CASES];
    const CkNode *statement = NULL;
    int cases = 0;
    ck_sort_count = ck_symbol_count = ck_declared = ck_bound_count = 0;
    ck_nodes = 0;
    for (int ended = 0; !ended && ck_status == CK_ACCEPTED;) {
        char name[CK_MAX_NAME];
        long long size = -1;
        while (isspace((unsigned char)*ck_cursor)) ck_cursor++;
        if (ck_word("end")) {
            ended = 1;
        } else if (!statement && ck_word("sort")) {
            /* Un insieme, con il numero di elementi nei soli modelli. */
            if (ck_sort_count == CK_MAX_SORTS) { ck_unsupported("troppi insiemi per il verificatore"); return; }
            if (!ck_name(name) || ck_sort_of(name) >= 0) { ck_reject("certificato scritto male: sort senza nome, o due volte"); return; }
            if ((kind < 2) != ck_read_integer(&size) || (kind < 2 && size < 0)) { ck_reject("certificato scritto male: il numero di elementi c'è nei soli modelli"); return; }
            if (size > CK_MAX_SIZE) { ck_unsupported("modello troppo grande per il verificatore"); return; }
            strcpy(ck_sorts[ck_sort_count].name, name);
            ck_sorts[ck_sort_count++].size = (int)size;
        } else if (!statement && ck_word("symbol")) {
            /* Un simbolo: gli insiemi degli argomenti, poi quello del valore. */
            int sort[3], count = 0;
            if (ck_symbol_count == CK_MAX_SYMBOLS) { ck_unsupported("troppi simboli per il verificatore"); return; }
            CkSymbol *y = &ck_symbols[ck_symbol_count];
            memset(y, 0, sizeof *y);
            if (!ck_name(y->name) || ck_symbol_of(y->name) >= 0) { ck_reject("certificato scritto male: symbol senza nome, o due volte"); return; }
            for (ck_skip(); *ck_cursor && *ck_cursor != '\n'; ck_skip()) {
                if (count == 3 || !ck_name(name)) { ck_reject("certificato scritto male: symbol con troppi insiemi"); return; }
                sort[count++] = strcmp(name, "bool") == 0 ? CK_BOOL : ck_sort_of(name);
            }
            if (count == 0) { ck_reject("certificato scritto male: symbol senza insiemi"); return; }
            y->arity = count - 1;
            y->result = sort[count - 1];
            for (int i = 0; i < count; i++) {
                if (sort[i] < 0 || (sort[i] == CK_BOOL && i < y->arity)) { ck_reject("certificato scritto male: symbol con un insieme non dichiarato"); return; }
                if (i < y->arity) y->arg[i] = sort[i];
            }
            ck_symbol_count++;
        } else if (!statement && ck_word("statement")) {
            statement = ck_read_condition();
            ck_declared = ck_symbol_count;
        } else if (kind < 2 && statement && ck_word("table")) {
            /* I valori di un simbolo, cella per cella. */
            int s = ck_name(name) ? ck_symbol_of(name) : -1;
            if (s < 0 || ck_symbols[s].table) { ck_reject("table di un simbolo non dichiarato, o due volte"); return; }
            CkSymbol *y = &ck_symbols[s];
            int cells = 1, values = y->result == CK_BOOL ? 2 : ck_sorts[y->result].size;
            for (int i = 0; i < y->arity; i++) cells *= ck_sorts[y->arg[i]].size;
            y->table = ck_alloc(sizeof(int) * (size_t)cells);
            for (int c = 0; c < cells && ck_status == CK_ACCEPTED; c++) {
                long long v;
                if (!ck_read_integer(&v) || v < 0 || v >= values) { ck_reject("una tabella ha meno valori delle sue celle, o un valore fuori dal suo insieme"); return; }
                y->table[c] = (int)v;
            }
            ck_skip();
            if (*ck_cursor && *ck_cursor != '\n') { ck_reject("una tabella ha più valori delle sue celle"); return; }
        } else if (kind == 2 && statement && ck_word("case")) {
            /* Un caso nuovo: si suppongono "E falso" e A. */
            if (cases) ck_check_case();
            if (cases == CK_MAX_CASES) { ck_unsupported("troppi casi per il verificatore"); return; }
            CkNode *denied = ck_node(CK_NOT);
            denied->a = statement;
            ck_case_start();
            ck_suppose(denied, 0);
            assumed[cases] = ck_read_condition();
            ck_suppose(assumed[cases++], 0);
        } else if (kind == 2 && cases && ck_word("instance")) {
            ck_instance();
        } else if (kind == 2 && cases && ck_word("witness")) {
            ck_witness();
        } else if (kind == 2 && cases && ck_word("part")) {
            ck_part();
        } else {
            ck_reject("certificato scritto male: voce sconosciuta o fuori posto, o \"end\" mancante");
        }
    }
    if (ck_status != CK_ACCEPTED) return;
    if (!statement) { ck_reject("certificato incompleto: manca l'enunciato"); return; }

    /* 11a, 11b: il conto sul modello. */
    if (kind < 2) {
        for (int s = 0; s < ck_symbol_count; s++)
            if (!ck_symbols[s].table) { ck_reject("certificato incompleto: un simbolo non ha la sua tabella"); return; }
        ck_tried = 0;
        int truth = ck_holds(statement, 0);
        if (ck_status == CK_ACCEPTED && truth != kind)
            ck_reject(kind ? "nel modello l'enunciato è falso: non è un esempio" : "nel modello l'enunciato è vero: non è un controesempio");
        return;
    }

    /* 11c: l'ultimo caso, poi la copertura (11d): "E falso" e nessun A
     * devono essere contraddittori. */
    if (cases) ck_check_case();
    CkNode *denied = ck_node(CK_NOT);
    denied->a = statement;
    ck_case_start();
    ck_suppose(denied, 0);
    for (int i = 0; i < cases; i++) {
        CkNode *other = ck_node(CK_NOT);
        other->a = assumed[i];
        ck_suppose(other, 0);
    }
    if (ck_status == CK_ACCEPTED && ck_consistent(0)) ck_reject("i casi non coprono tutti i modi in cui l'enunciato potrebbe essere falso");
}

/* =============================================================================
 * 12. TABELLE DEI SEGNI
 * ============================================================================
 *
 * Gli enunciati della regola 18p di mathresearcher hanno una variabile sola,
 * che varia nei numeri reali o nei razionali, e confrontano espressioni
 * razionali con =, \neq, <, \le: "per ogni x vale C", "esiste x per cui vale
 * C", oppure "le soluzioni di C sono W". Il motore li decide con la tabella
 * dei segni, e il certificato è la tabella:
 *
 *     certificate signs
 *     variable x
 *     domain reals
 *     condition (<= 0 (/ (- x 2) (+ x 3)))
 *     claim solve
 *     solution (or (< x -3) (<= 2 x))
 *     factor (- x 2)
 *     factor (+ x 3)
 *     interval -5 true
 *     point -3 false
 *     interval 0 false
 *     point 2 true
 *     interval 3 true
 *     end
 *
 * Le voci, in quest'ordine:
 *
 *   variable   la variabile, una lettera;
 *   domain     reals oppure rationals: dove varia;
 *   condition  la condizione C: and, or, implies, iff e not di confronti
 *              (= a b), (neq a b), (< a b), (<= a b), i cui membri sono
 *              fatti di interi, della variabile, di + - * /, di neg, di
 *              potenze (^ a k) con k un intero scritto e di (abs a);
 *   claim      forall true, forall false, exists true, exists false, oppure
 *              solve con la voce solution: le soluzioni scritte W, cioè
 *              confronti della variabile con un numero, che può contenere la
 *              radice quadrata di un intero, (sqrt 2); oppure all (ogni
 *              numero) o none (nessuno);
 *   factor     un polinomio nella variabile, quanti se ne vuole;
 *   le righe   da sinistra: "interval s v" è un intervallo aperto, e s un
 *              suo razionale; "point r v" è il razionale r; "root k a b v" è
 *              l'unica radice del fattore numero k (da 1) tra i razionali a e
 *              b. v è true o false: la verità di C dichiarata nella riga.
 *              Intervalli e punti si alternano, e la tabella comincia e
 *              finisce con un intervallo.
 *
 * IL TEOREMA. Un confronto vale in x solo se i suoi due membri sono definiti
 * in x, cioè se nessuna divisione, nel loro calcolo, ha il divisore nullo
 * (una potenza con esponente negativo è una divisione; x^0 vale 1). Sia D
 * l'insieme dei numeri reali, o quello dei razionali. Se il certificato è
 * accettato:
 *
 *   - forall true:  per ogni x in D vale C(x);
 *   - forall false: esiste un x in D in cui C(x) non vale;
 *   - exists true:  esiste un x in D in cui vale C(x);
 *   - exists false: per nessun x in D vale C(x);
 *   - solve, con D i numeri reali: per ogni numero reale x, C(x) vale se e
 *     solo se vale W(x). W descrive esattamente le soluzioni di C.
 *
 * COME LO SI CONTROLLA. È il ragionamento della tabella dei segni: la verità
 * di C in x dipende solo dai segni in x di certi polinomi, un polinomio
 * cambia segno solo passando per una sua radice (teorema degli zeri), e
 * quindi tra due radici consecutive la verità non cambia: basta guardarla in
 * un punto di ogni intervallo e in ogni radice. Il motore ha trovato le
 * radici; qui si controlla che non ne manchi nessuna, e si ricalcola la
 * verità di ogni riga. Nessuna ricerca: divisioni, catene di Sturm e conti
 * esatti.
 *
 *   12a. Polinomi in una variabile e catene di Sturm, che contano le radici
 *        reali tra due numeri.
 *   12b. I fattori: il segno di un polinomio in una radice irrazionale si
 *        ricava dividendolo per i fattori; e si controlla che ogni sua
 *        radice sia un punto della tabella.
 *   12c. La condizione, calcolata in due modi che non hanno nulla in comune:
 *        con un razionale al posto della variabile, e come frazione di
 *        polinomi.
 *   12d. I polinomi da cui dipende la verità di C, ricavati da capo dalla
 *        condizione, hanno tutte le radici reali tra i punti della tabella:
 *        allora la verità è costante tra due punti consecutivi.
 *   12e. Le righe: ogni riga root isola una radice del suo fattore e nessuna
 *        degli altri; ogni campione sta nel suo intervallo; nessuna radice di
 *        un fattore manca. La verità di C in ogni riga, che deve essere
 *        quella dichiarata.
 *   12f. Nei razionali le righe root non contano: si controlla che le loro
 *        radici non siano razionali.
 *   12g. Le soluzioni scritte: ogni loro estremo è un punto della tabella, e
 *        in ogni riga W ha la verità di C.
 *   12h. La lettura del certificato e il verdetto, che segue dalle righe.
 */

/* 12a. POLINOMI IN UNA VARIABILE E CATENE DI STURM */

/* Un polinomio nella variabile della tabella, a coefficienti razionali,
 * scritto per gradi: c[0] + c[1] x + ... + c[degree] x^degree. Serve dove i
 * polinomi dei capitoli 4 e 5, elenchi di termini, sarebbero scomodi: la
 * divisione con resto e il valore in un numero. Invariante: c[degree] non è
 * zero; il polinomio nullo ha degree -1. */
typedef struct {
    int degree;
    CkNum c[CK_MAX_DEGREE + 1];
} CkUni;

/* Ritorna il segno del razionale a: -1, 0 o 1. */
static int ck_num_sign(CkNum a) { return (a.num > 0) - (a.num < 0); }

/* Ritorna il segno di a - b. */
static int ck_num_cmp(CkNum a, CkNum b) { return ck_num_sign(ck_num_add(a, ck_num_neg(b))); }

/* Ristabilisce l'invariante di p, dopo un conto che può averne annullato i
 * coefficienti più alti. */
static void ck_uni_trim(CkUni *p) {
    while (p->degree >= 0 && p->c[p->degree].num == 0) p->degree--;
}

/* Mette in *out il polinomio p, che contiene al più l'indeterminata 0, la
 * variabile della tabella (12c). Un grado oltre CK_MAX_DEGREE rende il
 * certificato non controllabile. */
static void ck_uni_of(CkPoly p, CkUni *out) {
    out->degree = -1;
    for (int i = 0; i < p.count; i++) {
        int e = p.term[i].e[0];
        if (e > CK_MAX_DEGREE) { ck_unsupported("polinomio di grado troppo alto per il verificatore"); out->degree = -1; return; }
        while (out->degree < e) out->c[++out->degree] = ck_num_int(0);
        out->c[e] = p.term[i].coef;
    }
    ck_uni_trim(out);
}

/* Ritorna il valore di p nel razionale x (schema di Horner). */
static CkNum ck_uni_at(const CkUni *p, CkNum x) {
    CkNum value = ck_num_int(0);
    for (int i = p->degree; i >= 0; i--) value = ck_num_add(ck_num_mul(value, x), p->c[i]);
    return value;
}

/* Divide a per b, che non è nullo: al ritorno *a è il resto, di grado minore
 * di quello di b, e *q, se q non è NULL, il quoziente. È la divisione di
 * scuola: a ogni passo si toglie ad a il multiplo di b che ne annulla il
 * termine più alto. */
static void ck_uni_divide(CkUni *a, const CkUni *b, CkUni *q) {
    if (q) {
        q->degree = a->degree >= b->degree ? a->degree - b->degree : -1;
        for (int i = 0; i <= q->degree; i++) q->c[i] = ck_num_int(0);
    }

    /* Dopo un errore i coefficienti non contano, e il primo di b può essere
     * zero: non lo si inverte. */
    if (ck_status != CK_ACCEPTED) return;
    CkNum lead = ck_num_inv(b->c[b->degree]);
    while (a->degree >= b->degree && ck_status == CK_ACCEPTED) {
        int shift = a->degree - b->degree;
        CkNum factor = ck_num_mul(a->c[a->degree], lead);
        if (q) q->c[shift] = factor;
        for (int i = 0; i < b->degree; i++) a->c[i + shift] = ck_num_add(a->c[i + shift], ck_num_neg(ck_num_mul(factor, b->c[i])));

        /* Il termine più alto si annulla per costruzione: lo si azzera senza
         * conto, così il ciclo finisce anche dopo un errore. */
        a->c[a->degree] = ck_num_int(0);
        ck_uni_trim(a);
    }
}

/* Moltiplica p, che non è nullo, per il razionale positivo che ne fa un
 * polinomio a coefficienti interi senza divisori comuni: radici e segni
 * restano quelli, e i numeri di una catena di Sturm restano piccoli. */
static void ck_uni_primitive(CkUni *p) {
    __int128 top = 0, bottom = 1;
    if (ck_status != CK_ACCEPTED) return;
    for (int i = 0; i <= p->degree; i++) {
        top = ck_gcd(top, p->c[i].num);
        if (__builtin_mul_overflow(bottom, p->c[i].den / ck_gcd(bottom, p->c[i].den), &bottom)) {
            ck_unsupported("numeri troppo grandi per il verificatore");
            return;
        }
    }
    if (top == 0) return;
    CkNum scale = ck_num_make(bottom, top);
    for (int i = 0; i <= p->degree; i++) p->c[i] = ck_num_mul(p->c[i], scale);
}

/* Ritorna il massimo comun divisore di a e b, che non sono nulli, a meno di
 * un numero diverso da zero: algoritmo di Euclide, mcd(a, b) = mcd(b, resto
 * di a diviso b). */
static CkUni ck_uni_gcd(CkUni a, CkUni b) {
    while (b.degree >= 0 && ck_status == CK_ACCEPTED) {
        ck_uni_divide(&a, &b, NULL);
        CkUni rest = a;
        a = b;
        b = rest;
        if (b.degree >= 0) ck_uni_primitive(&b);
    }
    return a;
}

/* La catena di Sturm dell'ultimo polinomio passato a ck_roots(). */
static CkUni ck_chain[CK_MAX_DEGREE + 2];
static int ck_chain_length;

/* Ritorna quante radici reali distinte ha p, che non è nullo, tra low e
 * high, che non devono essere sue radici; NULL al posto di low o di high
 * vuol dire meno o più infinito.
 *
 * Teorema di Sturm. Siano p_0 = p, p_1 = p' e p_{i+1} = -(resto della
 * divisione di p_{i-1} per p_i), fino all'ultimo p_m non nullo, che è il
 * massimo comun divisore di p e p' (algoritmo di Euclide). Detto V(t) il
 * numero di cambi di segno della successione p_0(t), ..., p_m(t), saltando
 * gli zeri, se a < b non sono radici di p allora p ha tra a e b esattamente
 * V(a) - V(b) radici reali distinte. Per p senza radici multiple (p_m
 * costante) è la forma classica: V cambia solo dove si annulla un p_i;
 * attraversando una radice di p scende di uno, perché subito prima p e p'
 * hanno segni opposti e subito dopo lo stesso; attraversando una radice di
 * un p_i intermedio non cambia, perché lì p_{i-1} = -p_{i+1}, di segni
 * opposti, e p_{i-1}, p_{i+1} non si annullano. Con radici multiple ogni p_i
 * è un multiplo di p_m, e dividere tutta la successione per p_m non cambia i
 * cambi di segno dove p_m non è zero: si ottiene la successione di p / p_m,
 * che ha le stesse radici di p, ognuna una volta. p_m divide p, quindi non si
 * annulla in a né in b. (Vedi Basu, Pollack, Roy, Algorithms in Real
 * Algebraic Geometry, teorema 2.50.)
 *
 * All'infinito ogni p_i ha il segno del suo termine di grado più alto: a più
 * infinito quello del coefficiente, a meno infinito lo stesso cambiato di
 * segno se il grado è dispari. Moltiplicare i p_i per numeri positivi, come
 * fa ck_uni_primitive(), non cambia i segni: il resto di alpha A diviso
 * beta B, con alpha e beta positivi, è alpha volte quello di A diviso B. */
static int ck_roots(const CkUni *p, const CkNum *low, const CkNum *high) {
    /* La catena: p, p', poi i resti cambiati di segno. */
    ck_chain[0] = *p;
    ck_uni_primitive(&ck_chain[0]);
    ck_chain[1].degree = p->degree - 1;
    for (int i = 1; i <= p->degree; i++) ck_chain[1].c[i - 1] = ck_num_mul(ck_chain[0].c[i], ck_num_int(i));
    ck_chain_length = p->degree > 0 ? 2 : 1;
    while (ck_chain_length > 1 && ck_status == CK_ACCEPTED) {
        CkUni *last = &ck_chain[ck_chain_length - 1], rest = ck_chain[ck_chain_length - 2];
        ck_uni_primitive(last);
        ck_uni_divide(&rest, last, NULL);
        if (rest.degree < 0) break;
        for (int i = 0; i <= rest.degree; i++) rest.c[i] = ck_num_neg(rest.c[i]);
        ck_chain[ck_chain_length++] = rest;
    }

    /* V(low) - V(high). */
    int changes[2] = {0, 0};
    for (int end = 0; end < 2; end++) {
        const CkNum *t = end ? high : low;
        for (int i = 0, last = 0; i < ck_chain_length; i++) {
            const CkUni *q = &ck_chain[i];
            int sign = t ? ck_num_sign(ck_uni_at(q, *t)) : ck_num_sign(q->c[q->degree]) * (!end && q->degree % 2 ? -1 : 1);
            if (sign && last && sign != last) changes[end]++;
            if (sign) last = sign;
        }
    }
    return changes[0] - changes[1];
}

/* 12b. I FATTORI
 *
 * Nelle righe dei punti della tabella si conoscono i segni dei fattori, e
 * ogni radice reale di un fattore è uno di quei punti (12e). Di ogni
 * polinomio p da cui dipende la verità della condizione servono due cose:
 * che anche le sue radici reali siano tutte punti della tabella, e il suo
 * segno in una radice irrazionale, dove non lo si può calcolare con i
 * numeri. Si divide p per un fattore alla volta, finché la divisione è
 * esatta: p = c f_1^e_1 ... f_m^e_m R, con c un numero e R un polinomio che
 * nessun fattore divide (spesso un numero anche lui). Non serve che sia la
 * scomposizione migliore: ogni passo è un'uguaglianza vera.
 *
 *   - Le radici di p sono quelle dei fattori con e_i > 0, che sono punti, e
 *     quelle di R. Di R si contano le radici reali distinte (Sturm), e
 *     devono essere tante quante le righe dei punti in cui R si annulla:
 *     allora sono tutte punti.
 *   - Il segno di p in un punto è quello di c, per quelli dei fattori
 *     elevati ai loro esponenti, per quello di R.
 *   - Se R si annulla nella radice rho di una riga root, del fattore f tra
 *     a e b, lo dice il massimo comun divisore g di R e f: le radici di g
 *     sono radici di f, che tra a e b ne ha una sola, rho; quindi R(rho) = 0
 *     esattamente quando g ha una radice tra a e b (Sturm; g non si annulla
 *     in a né in b, perché f non lo fa). Se R(rho) non è zero, R ha in rho
 *     il segno che ha in a: le sue radici sono punti della tabella, e tra a
 *     e b l'unico punto è rho.
 *
 * Il motore dà come fattori gli elementi della sua base (18p-4 di main.c),
 * che non hanno radici in comune; un polinomio della condizione può
 * contenerne i fattori irriducibili con esponenti diversi, come (x^2 + 1)^2
 * (x^4 - x^3 + x^2 - x + 1) con il fattore (x^2 + 1)(x^4 - x^3 + x^2 - x + 1):
 * è il caso di R. */

/* I fattori del certificato in corso, nell'ordine delle voci factor. */
static CkUni ck_factor[CK_MAX_FACTORS];
static int ck_factor_count;

/* I tre tipi di riga di una tabella. */
typedef enum { CK_INTERVAL, CK_POINT, CK_ROOT } CkRowKind;

/* Una riga della tabella: un intervallo aperto tra due punti consecutivi, o
 * un punto. value è il campione dell'intervallo, il razionale di un punto,
 * l'estremo inferiore dell'intervallo che isola una radice; high ne è
 * l'estremo superiore, e factor il fattore che vi si annulla (da 0). claimed
 * è la verità di C dichiarata nella riga, 1 o 0. */
typedef struct {
    CkRowKind kind;
    CkNum value, high;
    int factor;
    int claimed;
} CkRow;

/* Le righe del certificato in corso; nelle righe dei punti, il segno di ogni
 * fattore (12e). */
static CkRow ck_rows[2 * CK_MAX_POINTS + 1];
static int ck_row_count;
static signed char ck_row_sign[2 * CK_MAX_POINTS + 1][CK_MAX_FACTORS];

/* Ritorna 1 se il polinomio r, non nullo, si annulla nella radice della
 * riga row, una riga root: se il massimo comun divisore di r e del fattore
 * della riga ha una radice tra i suoi estremi (vedi in testa a 12b). */
static int ck_vanishes(const CkUni *r, int row) {
    const CkRow *at = &ck_rows[row];
    CkUni g = ck_uni_gcd(*r, ck_factor[at->factor]);
    return g.degree > 0 && ck_roots(&g, &at->value, &at->high) == 1;
}

/* Ritorna il segno nella riga row, una riga root, del polinomio p
 * nell'indeterminata 0, come spiegato in testa a 12b. Con row negativo
 * controlla invece che ogni radice reale di p sia un punto della tabella, e
 * se non lo è il certificato è sbagliato: alla tabella manca un punto in cui
 * la condizione può cambiare verità. */
static int ck_sign_in_row(CkPoly p, int row) {
    CkUni rest, quotient, left;
    int sign = 1, declared = 0;
    ck_uni_of(p, &rest);
    if (rest.degree < 0 || ck_status != CK_ACCEPTED) return 0;

    /* p = c f_1^e_1 ... f_m^e_m R: i fattori, e il loro segno. */
    for (int i = 0; i < ck_factor_count && rest.degree > 0 && ck_status == CK_ACCEPTED; i++)
        while (rest.degree >= ck_factor[i].degree) {
            left = rest;
            ck_uni_divide(&left, &ck_factor[i], &quotient);
            if (left.degree >= 0 || ck_status != CK_ACCEPTED) break;
            rest = quotient;
            if (row >= 0) sign *= ck_row_sign[row][i];
        }
    if (rest.degree == 0 || ck_status != CK_ACCEPTED) return sign * ck_num_sign(rest.c[0]);

    /* R: nella riga, o le sue radici. */
    if (row >= 0) return ck_vanishes(&rest, row) ? 0 : sign * ck_num_sign(ck_uni_at(&rest, ck_rows[row].value));
    for (int j = 1; j < ck_row_count && ck_status == CK_ACCEPTED; j += 2)
        declared += ck_rows[j].kind == CK_POINT ? ck_uni_at(&rest, ck_rows[j].value).num == 0 : ck_vanishes(&rest, j);
    if (ck_roots(&rest, NULL, NULL) != declared && ck_status == CK_ACCEPTED)
        ck_reject("un polinomio della condizione ha una radice che non è un punto della tabella");
    return 0;
}

/* 12c. LA CONDIZIONE, CALCOLATA IN DUE MODI
 *
 * Il primo sostituisce alla variabile un razionale e fa i conti con i
 * numeri: dà la verità in quel punto senza nessun polinomio. Il secondo
 * calcola i membri di un confronto come frazioni di polinomi nella variabile,
 * che è l'indeterminata 0 dei capitoli 4 e 5, con i valori assoluti sciolti
 * secondo una scelta dei segni, e annota i divisori incontrati: serve dove
 * la variabile è una radice irrazionale, e a sapere da quali polinomi
 * dipende la verità (12d). */

/* La variabile della tabella in corso. */
static char ck_x;

/* Il confronto in corso: i suoi valori assoluti, i più interni per primi
 * (ck_collect_abs()); i numeratori dei divisori annotati calcolandolo come
 * frazione; ck_nowhere è 1 se si è diviso per la frazione nulla, e allora,
 * con quella scelta dei segni, il confronto non ha valore in nessun punto. */
static const CkNode *ck_abs[CK_MAX_ABS];
static int ck_abs_count;
static CkPoly ck_seen[CK_MAX_SEEN];
static int ck_seen_count, ck_nowhere;

/* Mette in *k l'esponente e di una potenza, se è un intero scritto: k
 * oppure (neg k). Altrimenti ritorna 0. */
static int ck_exponent(const CkNode *e, long long *k) {
    if (e->kind == CK_NUMBER) { *k = e->value; return 1; }
    if (e->kind == CK_NEG && e->a->kind == CK_NUMBER) { *k = -e->a->value; return 1; }
    return 0;
}

/* Ritorna 1 se node è un membro di un confronto della condizione (surd = 0):
 * interi, la variabile, + - * /, neg, potenze a esponente intero scritto e
 * valori assoluti. Con surd = 1 è invece un numero delle soluzioni scritte:
 * senza la variabile e senza valori assoluti, ma con la radice quadrata di
 * un intero scritto. */
static int ck_expression_ok(const CkNode *node, int surd) {
    long long k;
    switch (node->kind) {
    case CK_NUMBER: return 1;
    case CK_LETTER: return !surd && node->name == ck_x;
    case CK_NEG:    return ck_expression_ok(node->a, surd);
    case CK_ABS:    return !surd && ck_expression_ok(node->a, surd);
    case CK_SQRT:   return surd && node->a->kind == CK_NUMBER;
    case CK_ADD: case CK_SUB: case CK_MUL: case CK_DIV:
        return ck_expression_ok(node->a, surd) && ck_expression_ok(node->b, surd);
    case CK_POW:
        return ck_exponent(node->b, &k) && k <= CK_MAX_POWER && k >= -CK_MAX_POWER && ck_expression_ok(node->a, surd);
    default:        return 0;
    }
}

/* Ritorna 1 se c è una condizione: and, or, implies, iff, not di confronti.
 * Con written = 1 sono le soluzioni scritte, e ogni confronto ha da una
 * parte la variabile e dall'altra un numero (ck_expression_ok() con surd =
 * 1). */
static int ck_condition_ok(const CkNode *c, int written) {
    switch (c->kind) {
    case CK_AND: case CK_OR: case CK_IMPLIES: case CK_IFF:
        return ck_condition_ok(c->a, written) && ck_condition_ok(c->b, written);
    case CK_NOT:
        return ck_condition_ok(c->a, written);
    case CK_EQ: case CK_NEQ: case CK_LESS: case CK_LESS_EQ:
        if (!written) return ck_expression_ok(c->a, 0) && ck_expression_ok(c->b, 0);
        return (c->a->kind == CK_LETTER && c->a->name == ck_x && ck_expression_ok(c->b, 1)) ||
               (c->b->kind == CK_LETTER && c->b->name == ck_x && ck_expression_ok(c->a, 1));
    default:
        return 0;
    }
}

/* Ritorna 1 se un confronto di tipo kind vale quando la differenza dei suoi
 * membri ha il segno side. */
static int ck_compared(CkKind kind, int side) {
    return kind == CK_EQ ? side == 0 : kind == CK_NEQ ? side != 0 : kind == CK_LESS ? side < 0 : side <= 0;
}

/* Ritorna il valore dell'espressione node, che ck_expression_ok() ha
 * accettato con surd = 0, quando la variabile vale x; mette 0 in *defined
 * se una divisione ha il divisore nullo, e allora il valore non conta. */
static CkNum ck_value_at(const CkNode *node, CkNum x, int *defined) {
    CkNum a, b;
    long long k = 0;
    if (ck_status != CK_ACCEPTED) return ck_num_int(0);
    switch (node->kind) {
    case CK_NUMBER: return ck_num_int(node->value);
    case CK_LETTER: return x;
    case CK_NEG:    return ck_num_neg(ck_value_at(node->a, x, defined));
    case CK_ABS:    a = ck_value_at(node->a, x, defined); return a.num < 0 ? ck_num_neg(a) : a;
    case CK_ADD: case CK_SUB:
        a = ck_value_at(node->a, x, defined);
        b = ck_value_at(node->b, x, defined);
        return ck_num_add(a, node->kind == CK_SUB ? ck_num_neg(b) : b);
    case CK_MUL:
        a = ck_value_at(node->a, x, defined);
        return ck_num_mul(a, ck_value_at(node->b, x, defined));
    case CK_DIV:
        a = ck_value_at(node->a, x, defined);
        b = ck_value_at(node->b, x, defined);
        if (b.num == 0) { *defined = 0; return b; }
        return ck_num_mul(a, ck_num_inv(b));
    default:
        /* Una potenza: un esponente negativo è una divisione. */
        a = ck_value_at(node->a, x, defined);
        ck_exponent(node->b, &k);
        if (k < 0 && a.num == 0) { *defined = 0; return a; }
        if (k < 0) { a = ck_num_inv(a); k = -k; }
        for (b = ck_num_int(1); k > 0 && ck_status == CK_ACCEPTED; k--) b = ck_num_mul(b, a);
        return b;
    }
}

/* Mette in ck_abs[] i valori assoluti dell'espressione node, i più interni
 * per primi, una volta ciascuno: due scritti allo stesso modo hanno lo
 * stesso argomento, quindi lo stesso valore. Così l'argomento del valore
 * assoluto numero i contiene solo valori assoluti di numero minore. */
static void ck_collect_abs(const CkNode *node) {
    if (!node) return;
    ck_collect_abs(node->a);
    ck_collect_abs(node->b);
    if (node->kind != CK_ABS) return;
    for (int i = 0; i < ck_abs_count; i++)
        if (ck_same(ck_abs[i], node)) return;
    if (ck_abs_count == CK_MAX_ABS) { ck_unsupported("troppi valori assoluti in un confronto per il verificatore"); return; }
    ck_abs[ck_abs_count++] = node;
}

/* Ritorna a / b, annotando il numeratore di b tra i divisori; se b è la
 * frazione nulla annota invece che il confronto non ha valore. */
static CkRat ck_divide(CkRat a, CkRat b) {
    if (b.num.count == 0) { ck_nowhere = 1; return a; }
    if (ck_seen_count == CK_MAX_SEEN) { ck_unsupported("condizione troppo complessa per il verificatore"); return a; }
    ck_seen[ck_seen_count++] = b.num;
    return (CkRat){ck_poly_mul(a.num, b.den), ck_poly_mul(a.den, b.num)};
}

/* Ritorna il valore di node, che ck_expression_ok() ha accettato con surd =
 * 0, come frazione di polinomi nella variabile: il valore assoluto numero i
 * di ck_abs[] vale il suo argomento, oppure, se il bit i di pattern è 1, il
 * suo argomento cambiato di segno. Le frazioni non si semplificano: il
 * denominatore è un prodotto di numeratori di divisori annotati.
 *
 * Che cosa vale questo conto in un numero x0. Si supponga che pattern sia la
 * scelta giusta in x0: il bit i è 1 se l'argomento del valore assoluto i è
 * negativo in x0. Allora l'espressione è definita in x0 se e solo se non si
 * è annotata la frazione nulla e nessun divisore annotato si annulla in x0,
 * e il suo valore è num(x0) / den(x0), con den(x0) diverso da zero. Per
 * induzione sull'albero: mettere x0 al posto della variabile rispetta somme
 * e prodotti, quindi una somma o un prodotto di frazioni vale in x0 ciò che
 * vale il conto con i numeri; il denominatore è un prodotto di divisori, che
 * non si annullano; in a / b, con a e b definiti, b vale num_b(x0) /
 * den_b(x0), che è zero esattamente quando lo è num_b(x0), il divisore
 * annotato, e se num_b è il polinomio nullo b vale zero dovunque sia
 * definito; |u| vale u o -u secondo il segno di u, cioè secondo pattern. */
static CkRat ck_fraction(const CkNode *node, unsigned pattern) {
    CkRat a, result;
    long long k = 0;
    int i = 0;
    if (ck_status != CK_ACCEPTED) return ck_rat_int(0);
    switch (node->kind) {
    case CK_NUMBER: return ck_rat_int(node->value);
    case CK_LETTER: return ck_rat_poly(ck_poly_variable(0));
    case CK_NEG:    return ck_rat_add(ck_rat_int(0), ck_fraction(node->a, pattern), 1);
    case CK_ADD: case CK_SUB:
        a = ck_fraction(node->a, pattern);
        return ck_rat_add(a, ck_fraction(node->b, pattern), node->kind == CK_SUB);
    case CK_MUL:
        a = ck_fraction(node->a, pattern);
        return ck_rat_mul(a, ck_fraction(node->b, pattern));
    case CK_DIV:
        a = ck_fraction(node->a, pattern);
        return ck_divide(a, ck_fraction(node->b, pattern));
    case CK_ABS:
        while (i < ck_abs_count - 1 && !ck_same(ck_abs[i], node)) i++;
        a = ck_fraction(node->a, pattern);
        return pattern >> i & 1 ? ck_rat_add(ck_rat_int(0), a, 1) : a;
    default:
        /* Una potenza: con esponente negativo, la potenza di 1 / base. */
        a = ck_fraction(node->a, pattern);
        ck_exponent(node->b, &k);
        if (k < 0) { a = ck_divide(ck_rat_int(1), a); k = -k; }
        for (result = ck_rat_int(1); k > 0 && ck_status == CK_ACCEPTED; k--) result = ck_rat_mul(result, a);
        return result;
    }
}

/* 12d. Controlla che ogni radice reale di ogni polinomio da cui dipende la
 * verità della condizione c sia un punto della tabella (ck_sign_in_row()).
 * I polinomi sono, per ogni confronto e per ogni scelta dei segni dei suoi
 * valori assoluti: numeratore e denominatore di ogni argomento di un valore
 * assoluto, ogni divisore, numeratore e denominatore della differenza dei
 * membri. Dove il conto ha diviso per la frazione nulla (ck_fraction())
 * quel pezzo non ha valore in nessun punto, e i suoi polinomi non contano.
 *
 * Perché basta a dire che la verità di c non cambia su un intervallo aperto
 * I tra due punti consecutivi della tabella. Nessuno di quei polinomi si
 * annulla in I, quindi ognuno vi ha un segno costante: zero se è il
 * polinomio nullo, altrimenti sempre lo stesso, perché un polinomio cambia
 * segno solo passando per una radice. Si fissi un confronto, con i valori assoluti |u_1|,
 * ..., |u_k| dal più interno. Per induzione su i: se le scelte giuste per
 * u_1, ..., u_{i-1} sono costanti in I, u_i è in I una frazione fissa N_i /
 * D_i di polinomi di segno costante, quindi anche la scelta giusta per u_i
 * lo è (dove u_i non ha valore non ce l'ha nemmeno il confronto, e la scelta
 * non conta). Con la scelta giusta, costante, il confronto in I è definito
 * o no a seconda dei segni dei divisori, costanti, e la differenza dei
 * membri ha il segno del numeratore per quello del denominatore, costanti:
 * la verità del confronto è costante, e con lei quella di c. */
static void ck_cover(const CkNode *c) {
    if (c->kind == CK_AND || c->kind == CK_OR || c->kind == CK_IMPLIES || c->kind == CK_IFF || c->kind == CK_NOT) {
        ck_cover(c->a);
        if (c->b) ck_cover(c->b);
        return;
    }
    ck_abs_count = 0;
    ck_collect_abs(c->a);
    ck_collect_abs(c->b);
    for (unsigned pattern = 0; pattern < 1u << ck_abs_count && ck_status == CK_ACCEPTED; pattern++) {
        /* Ciò che si calcola qui non serve più dopo: la memoria si rende. */
        size_t mark = ck_used;
        for (int i = 0; i < ck_abs_count && ck_status == CK_ACCEPTED; i++) {
            ck_seen_count = ck_nowhere = 0;
            CkRat argument = ck_fraction(ck_abs[i]->a, pattern);
            if (ck_nowhere) continue;
            ck_sign_in_row(argument.num, -1);
            ck_sign_in_row(argument.den, -1);
        }
        ck_seen_count = ck_nowhere = 0;
        CkRat gap = ck_rat_add(ck_fraction(c->a, pattern), ck_fraction(c->b, pattern), 1);
        if (!ck_nowhere) {
            ck_sign_in_row(gap.num, -1);
            ck_sign_in_row(gap.den, -1);
            for (int i = 0; i < ck_seen_count; i++) ck_sign_in_row(ck_seen[i], -1);
        }
        ck_used = mark;
    }
}

/* 12e. LE RIGHE */

/* Ritorna 1 se la condizione c vale nella riga row.
 *
 * In un razionale, il campione di un intervallo o un punto, la si calcola
 * con i numeri (ck_value_at()). In una riga root la variabile è una radice
 * irrazionale, e si usano i segni dei fattori (ck_sign_in_row()): prima la
 * scelta dei segni dei valori assoluti, dal più interno, dal segno del loro
 * argomento; poi, con quella scelta, che è la giusta, il confronto è
 * definito se nessun divisore si annulla, e la differenza dei membri ha il
 * segno del suo numeratore per quello del suo denominatore (il perché è in
 * testa a ck_fraction()). */
static int ck_row_truth(const CkNode *c, int row) {
    const CkRow *r = &ck_rows[row];
    switch (c->kind) {
    case CK_AND:     return ck_row_truth(c->a, row) && ck_row_truth(c->b, row);
    case CK_OR:      return ck_row_truth(c->a, row) || ck_row_truth(c->b, row);
    case CK_IMPLIES: return !ck_row_truth(c->a, row) || ck_row_truth(c->b, row);
    case CK_IFF:     return ck_row_truth(c->a, row) == ck_row_truth(c->b, row);
    case CK_NOT:     return !ck_row_truth(c->a, row);
    default:         break;
    }

    /* Un razionale: il conto con i numeri. */
    if (r->kind != CK_ROOT) {
        int defined = 1;
        CkNum left = ck_value_at(c->a, r->value, &defined), right = ck_value_at(c->b, r->value, &defined);
        return defined && ck_compared(c->kind, ck_num_cmp(left, right));
    }

    /* Una radice: la scelta dei segni dei valori assoluti, poi il
     * confronto. */
    size_t mark = ck_used;
    unsigned pattern = 0;
    ck_abs_count = 0;
    ck_collect_abs(c->a);
    ck_collect_abs(c->b);
    for (int i = 0; i < ck_abs_count && ck_status == CK_ACCEPTED; i++) {
        ck_seen_count = ck_nowhere = 0;
        CkRat argument = ck_fraction(ck_abs[i]->a, pattern);
        if (!ck_nowhere && ck_sign_in_row(argument.num, row) * ck_sign_in_row(argument.den, row) < 0) pattern |= 1u << i;
    }
    ck_seen_count = ck_nowhere = 0;
    CkRat gap = ck_rat_add(ck_fraction(c->a, pattern), ck_fraction(c->b, pattern), 1);
    int defined = !ck_nowhere, side = 0;
    for (int i = 0; i < ck_seen_count && defined; i++)
        if (ck_sign_in_row(ck_seen[i], row) == 0) defined = 0;
    if (defined) side = ck_sign_in_row(gap.num, row) * ck_sign_in_row(gap.den, row);
    ck_used = mark;
    return defined && ck_compared(c->kind, side);
}

/* Controlla le righe appena lette e mette in ck_row_sign[] i segni dei
 * fattori nelle righe dei punti:
 *
 *   - una riga root contiene una sola radice del suo fattore, e nessuna
 *     degli altri, e i suoi estremi non sono radici di nessun fattore (Sturm,
 *     ck_roots()): allora il suo fattore vi si annulla, e ogni altro ha lì il
 *     segno che ha nell'estremo inferiore, perché tra i due estremi non
 *     cambia segno;
 *   - in una riga point i segni si calcolano con i numeri;
 *   - ogni campione sta tra i punti che lo chiudono: gli intervalli delle
 *     righe sono in ordine, e non si toccano;
 *   - nessuna radice manca: ogni fattore ha, tra i reali, tante radici
 *     distinte (Sturm, da meno a più infinito) quante sono le righe dei
 *     punti in cui si annulla. Quelle righe sono numeri diversi, ognuno una
 *     sua radice: se sono tante quante le radici, sono tutte. */
static void ck_check_rows(void) {
    /* I punti. */
    for (int row = 1; row < ck_row_count && ck_status == CK_ACCEPTED; row += 2) {
        const CkRow *r = &ck_rows[row];
        if (r->kind == CK_ROOT && ck_num_cmp(r->value, r->high) >= 0) { ck_reject("l'intervallo di una radice ha gli estremi in ordine sbagliato"); return; }
        for (int i = 0; i < ck_factor_count && ck_status == CK_ACCEPTED; i++) {
            const CkUni *f = &ck_factor[i];
            int low = ck_num_sign(ck_uni_at(f, r->value)), wanted = i == r->factor;
            ck_row_sign[row][i] = (signed char)low;
            if (r->kind == CK_POINT) continue;
            if (low == 0 || ck_num_sign(ck_uni_at(f, r->high)) == 0 || ck_roots(f, &r->value, &r->high) != wanted) {
                ck_reject(wanted ? "l'intervallo di una radice non contiene una sola radice del suo fattore"
                                 : "l'intervallo di una radice contiene una radice di un altro fattore");
                return;
            }
            if (wanted) ck_row_sign[row][i] = 0;
        }
    }

    /* L'ordine: ogni campione sta tra i punti vicini. */
    for (int row = 0; row < ck_row_count && ck_status == CK_ACCEPTED; row += 2) {
        const CkRow *r = &ck_rows[row], *before = row > 0 ? &ck_rows[row - 1] : NULL, *after = row + 1 < ck_row_count ? &ck_rows[row + 1] : NULL;
        if ((before && ck_num_cmp(r->value, before->kind == CK_ROOT ? before->high : before->value) <= 0) || (after && ck_num_cmp(r->value, after->value) >= 0))
            ck_reject("il campione di un intervallo non sta tra i punti che lo chiudono");
    }

    /* Nessuna radice manca. */
    for (int i = 0; i < ck_factor_count && ck_status == CK_ACCEPTED; i++) {
        int declared = 0;
        for (int row = 1; row < ck_row_count; row += 2) declared += ck_row_sign[row][i] == 0;
        if (ck_roots(&ck_factor[i], NULL, NULL) != declared && ck_status == CK_ACCEPTED) ck_reject("a un fattore manca una radice tra i punti della tabella");
    }
}

/* 12f. NEI RAZIONALI
 *
 * Ritorna 1 se la radice della riga row, una riga root, non è un numero
 * razionale.
 *
 * Teorema delle radici razionali. Se p/q, ridotta ai minimi termini, è una
 * radice di a_d x^d + ... + a_0 a coefficienti interi, allora q divide a_d:
 * moltiplicando per q^d si ha a_d p^d = -q (a_{d-1} p^{d-1} + ... + a_0
 * q^{d-1}), quindi q divide a_d p^d, e non avendo fattori in comune con p
 * divide a_d. Il fattore, reso a coefficienti interi (ck_uni_primitive()),
 * ha in (a, b) una radice sola: è razionale se e solo se uno dei numeri p/q
 * di (a, b), con q un divisore di a_d, lo annulla. Sono in numero finito, e
 * si provano tutti: è un conto completo. Oltre CK_MAX_TRIED prove il
 * certificato non è controllabile. */
static int ck_irrational(int row) {
    const CkRow *r = &ck_rows[row];
    CkUni f = ck_factor[r->factor];
    long long tried = 0;
    ck_uni_primitive(&f);
    __int128 lead = f.c[f.degree].num < 0 ? -f.c[f.degree].num : f.c[f.degree].num;
    if (ck_status != CK_ACCEPTED || lead > CK_MAX_FACTORED) { ck_unsupported("coefficiente troppo grande per cercarne i divisori"); return 0; }
    for (long long d = 1; d * d <= (long long)lead; d++) {
        if (lead % d) continue;
        for (int twice = 0; twice < 2; twice++) {
            long long q = twice ? (long long)lead / d : d;
            if (twice && q == d) continue;

            /* I numeratori p con a < p/q < b: da floor(a q) + 1 a ceil(b q) - 1. */
            CkNum low = ck_num_mul(r->value, ck_num_int(q)), high = ck_num_mul(r->high, ck_num_int(q));
            __int128 first = low.num / low.den - (low.num % low.den < 0) + 1, last = high.num / high.den + (high.num % high.den > 0) - 1;
            for (__int128 p = first; p <= last && ck_status == CK_ACCEPTED; p++) {
                if (++tried > CK_MAX_TRIED) { ck_unsupported("troppi razionali da provare come radici di un fattore"); return 0; }
                if (ck_uni_at(&f, ck_num_make(p, q)).num == 0) return 0;
            }
        }
    }
    return ck_status == CK_ACCEPTED;
}

/* 12g. LE SOLUZIONI SCRITTE
 *
 * Gli estremi delle soluzioni scritte sono razionali oppure numeri come
 * 1 - sqrt(2) / 2: li si calcola come a + b sqrt(k), con a e b razionali e
 * k un intero che non è un quadrato, una radice per estremo. sqrt(k) è
 * allora irrazionale, e a + b sqrt(k) è zero solo con a = b = 0. Due
 * controlli: ogni estremo è uno dei punti della tabella, così W non cambia
 * verità dentro un intervallo, e si può calcolare nel campione; e in ogni
 * riga W vale esattamente quando vale C. */

/* Il numero a + b sqrt(k). k è 0 per un razionale, e allora b è 0;
 * altrimenti è un intero positivo che non è un quadrato. */
typedef struct {
    CkNum a, b;
    long long k;
} CkSurd;

/* Ritorna x + y, oppure x - y se minus è diverso da zero. */
static CkSurd ck_surd_add(CkSurd x, CkSurd y, int minus) {
    if (x.k && y.k && x.k != y.k) ck_unsupported("un estremo delle soluzioni scritte con radici di numeri diversi");
    if (minus) y = (CkSurd){ck_num_neg(y.a), ck_num_neg(y.b), y.k};
    return (CkSurd){ck_num_add(x.a, y.a), ck_num_add(x.b, y.b), x.k ? x.k : y.k};
}

/* Ritorna x y: (a + b r)(c + d r) = a c + b d k + (a d + b c) r, con r =
 * sqrt(k). */
static CkSurd ck_surd_mul(CkSurd x, CkSurd y) {
    if (x.k && y.k && x.k != y.k) ck_unsupported("un estremo delle soluzioni scritte con radici di numeri diversi");
    long long k = x.k ? x.k : y.k;
    return (CkSurd){ck_num_add(ck_num_mul(x.a, y.a), ck_num_mul(ck_num_int(k), ck_num_mul(x.b, y.b))),
                    ck_num_add(ck_num_mul(x.a, y.b), ck_num_mul(x.b, y.a)), k};
}

/* Ritorna il segno di x = a + b sqrt(k). Se a e b hanno lo stesso segno, o
 * uno dei due è zero, è quello; se hanno segni opposti vince il più grande
 * in valore assoluto tra a e b sqrt(k), cioè, confrontando i quadrati, il
 * segno è quello di a per quello di a^2 - b^2 k. */
static int ck_surd_sign(CkSurd x) {
    int a = ck_num_sign(x.a), b = ck_num_sign(x.b);
    if (b == 0) return a;
    if (a == 0 || a == b) return b;
    return a * ck_num_sign(ck_num_add(ck_num_mul(x.a, x.a), ck_num_neg(ck_num_mul(ck_num_int(x.k), ck_num_mul(x.b, x.b)))));
}

/* Ritorna il razionale r come numero a + b sqrt(k). */
static CkSurd ck_surd_of(CkNum r) { return (CkSurd){r, ck_num_int(0), 0}; }

/* Ritorna il valore del numero node, che ck_expression_ok() ha accettato con
 * surd = 1. Dividere per zero, o la radice di un negativo, rendono sbagliate
 * le soluzioni scritte. 1 / (a + b r) = (a - b r) / (a^2 - b^2 k), e il
 * denominatore non è zero se a + b r non lo è, perché r è irrazionale. */
static CkSurd ck_surd_value(const CkNode *node) {
    CkSurd x, y;
    long long k = 0;
    if (ck_status != CK_ACCEPTED) return ck_surd_of(ck_num_int(0));
    switch (node->kind) {
    case CK_NUMBER: return ck_surd_of(ck_num_int(node->value));
    case CK_NEG:    return ck_surd_add(ck_surd_of(ck_num_int(0)), ck_surd_value(node->a), 1);
    case CK_ADD: case CK_SUB:
        x = ck_surd_value(node->a);
        return ck_surd_add(x, ck_surd_value(node->b), node->kind == CK_SUB);
    case CK_MUL:
        x = ck_surd_value(node->a);
        return ck_surd_mul(x, ck_surd_value(node->b));
    case CK_SQRT: {
        /* La parte intera della radice, per bisezione: se è esatta il
         * numero è razionale. */
        long long n = node->a->value, low = 0, high = 1;
        if (n < 0) { ck_reject("le soluzioni scritte contengono la radice di un numero negativo"); return ck_surd_of(ck_num_int(0)); }
        while (high * high <= n) high *= 2;
        while (high - low > 1) {
            long long middle = low + (high - low) / 2;
            if (middle * middle <= n) low = middle; else high = middle;
        }
        return low * low == n ? ck_surd_of(ck_num_int(low)) : (CkSurd){ck_num_int(0), ck_num_int(1), n};
    }
    default:
        /* Una divisione, o una potenza: entrambe con l'inverso. */
        x = ck_surd_value(node->a);
        y = node->kind == CK_DIV ? ck_surd_value(node->b) : x;
        if (node->kind == CK_POW) ck_exponent(node->b, &k);
        if (node->kind == CK_DIV || k < 0) {
            CkNum norm = ck_num_add(ck_num_mul(y.a, y.a), ck_num_neg(ck_num_mul(ck_num_int(y.k), ck_num_mul(y.b, y.b))));
            if (norm.num == 0) { ck_reject("le soluzioni scritte dividono per zero"); return y; }
            y = ck_surd_mul((CkSurd){y.a, ck_num_neg(y.b), y.k}, ck_surd_of(ck_num_inv(norm)));
        }
        if (node->kind == CK_DIV) return ck_surd_mul(x, y);
        for (x = ck_surd_of(ck_num_int(1)), k = k < 0 ? -k : k; k > 0 && ck_status == CK_ACCEPTED; k--) x = ck_surd_mul(x, y);
        return x;
    }
}

/* Ritorna la riga della tabella, un punto, che è il numero e; -1 se e non è
 * nessuno dei punti. Una riga root ha in (a, b) una radice sola del suo
 * fattore (12e): se e sta in (a, b) e annulla il fattore, è quella. */
static int ck_row_of(CkSurd e) {
    for (int row = 1; row < ck_row_count && ck_status == CK_ACCEPTED; row += 2) {
        const CkRow *r = &ck_rows[row];
        if (r->kind == CK_POINT && ck_surd_sign(ck_surd_add(e, ck_surd_of(r->value), 1)) == 0) return row;
        if (r->kind != CK_ROOT || ck_surd_sign(ck_surd_add(e, ck_surd_of(r->value), 1)) <= 0 || ck_surd_sign(ck_surd_add(ck_surd_of(r->high), e, 1)) <= 0) continue;

        /* Il fattore in e, con lo schema di Horner. */
        const CkUni *f = &ck_factor[r->factor];
        CkSurd value = ck_surd_of(ck_num_int(0));
        for (int i = f->degree; i >= 0; i--) value = ck_surd_add(ck_surd_mul(value, e), ck_surd_of(f->c[i]), 0);
        if (ck_surd_sign(value) == 0) return row;
    }
    return -1;
}

/* Controlla che ogni numero con cui le soluzioni scritte w confrontano la
 * variabile sia un punto della tabella. */
static void ck_check_endpoints(const CkNode *w) {
    if (w->kind == CK_AND || w->kind == CK_OR || w->kind == CK_IMPLIES || w->kind == CK_IFF || w->kind == CK_NOT) {
        ck_check_endpoints(w->a);
        if (w->b) ck_check_endpoints(w->b);
        return;
    }
    if (ck_row_of(ck_surd_value(w->a->kind == CK_LETTER ? w->b : w->a)) < 0) ck_reject("un estremo delle soluzioni scritte non è un punto della tabella");
}

/* Ritorna 1 se le soluzioni scritte w valgono nella riga row. In ogni
 * confronto la variabile sta da una parte, e il numero e dall'altra: conta
 * il segno di x - e. In un razionale lo si calcola; in una radice è zero se
 * e è proprio quella radice, altrimenti è il segno di a - e, con a
 * l'estremo inferiore della riga: e è un altro punto della tabella
 * (ck_check_endpoints()), quindi non sta tra gli estremi. */
static int ck_written(const CkNode *w, int row) {
    switch (w->kind) {
    case CK_AND:     return ck_written(w->a, row) && ck_written(w->b, row);
    case CK_OR:      return ck_written(w->a, row) || ck_written(w->b, row);
    case CK_IMPLIES: return !ck_written(w->a, row) || ck_written(w->b, row);
    case CK_IFF:     return ck_written(w->a, row) == ck_written(w->b, row);
    case CK_NOT:     return !ck_written(w->a, row);
    default:         break;
    }
    int left = w->a->kind == CK_LETTER, side;
    const CkRow *r = &ck_rows[row];
    CkSurd e = ck_surd_value(left ? w->b : w->a);
    if (r->kind != CK_ROOT) side = ck_surd_sign(ck_surd_add(ck_surd_of(r->value), e, 1));
    else if (ck_row_of(e) == row) side = 0;
    else {
        side = ck_surd_sign(ck_surd_add(ck_surd_of(r->value), e, 1));
        if (side != ck_surd_sign(ck_surd_add(ck_surd_of(r->high), e, 1))) ck_reject("un estremo delle soluzioni scritte sta nell'intervallo di una radice");
    }
    return ck_compared(w->kind, left ? side : -side);
}

/* 12h. LA LETTURA E IL VERDETTO */

/* Legge un razionale: un'espressione senza la variabile, che ha un valore. */
static CkNum ck_read_rational(void) {
    int defined = 1;
    const CkNode *node = ck_read();
    if (ck_status != CK_ACCEPTED) return ck_num_int(0);
    if (!ck_expression_ok(node, 0) || ck_mentions(node, ck_x)) { ck_reject("certificato scritto male: numero atteso"); return ck_num_int(0); }
    CkNum value = ck_value_at(node, ck_num_int(0), &defined);
    if (!defined) ck_reject("certificato scritto male: un numero divide per zero");
    return value;
}

/* Legge "true" o "false" e ritorna 1 o 0; altrimenti il certificato è
 * scritto male. */
static int ck_read_truth(void) {
    if (ck_word("true")) return 1;
    if (!ck_word("false")) ck_reject("certificato scritto male: true o false attesi");
    return 0;
}

/* Salta spazi e fini di riga fino alla voce successiva. */
static void ck_next_line(void) {
    while (isspace((unsigned char)*ck_cursor)) ck_cursor++;
}

/* Legge e controlla un certificato signs, dalla riga dopo l'intestazione
 * fino a "end" compreso. */
static void ck_check_signs(void) {
    const CkNode *condition = NULL, *written = NULL;
    int rationals, claim, claimed = 0, everywhere = 0;
    ck_sign_var = -1;
    ck_factor_count = ck_row_count = 0;

    /* La variabile, l'insieme, la condizione, ciò che si afferma. */
    ck_next_line();
    int named = ck_word("variable");
    ck_skip();
    if (!named || !isalpha((unsigned char)*ck_cursor) || isalnum((unsigned char)ck_cursor[1])) { ck_reject("certificato scritto male: manca la variabile"); return; }
    ck_x = *ck_cursor++;
    ck_next_line();
    if (!ck_word("domain")) { ck_reject("certificato scritto male: manca l'insieme"); return; }
    rationals = ck_word("rationals");
    if (!rationals && !ck_word("reals")) { ck_reject("certificato scritto male: domain reals o domain rationals"); return; }
    ck_next_line();
    if (!ck_word("condition")) { ck_reject("certificato scritto male: manca la condizione"); return; }
    condition = ck_read();
    if (ck_status != CK_ACCEPTED) return;
    if (!ck_condition_ok(condition, 0)) { ck_unsupported("la condizione usa qualcosa che il verificatore delle tabelle dei segni non conosce"); return; }
    ck_next_line();
    if (!ck_word("claim")) { ck_reject("certificato scritto male: manca ciò che si afferma"); return; }
    claim = ck_word("forall") ? 0 : ck_word("exists") ? 1 : ck_word("solve") ? 2 : -1;
    if (claim < 0) { ck_reject("certificato scritto male: claim forall, exists o solve"); return; }
    if (claim < 2) claimed = ck_read_truth();

    /* Le soluzioni scritte, che si cercano solo nei reali. */
    if (claim == 2) {
        ck_next_line();
        if (rationals || !ck_word("solution")) { ck_reject("certificato scritto male: solve vuole i reali e le soluzioni"); return; }
        if (ck_word("all")) everywhere = 1;
        else if (!ck_word("none")) {
            written = ck_read();
            if (ck_status == CK_ACCEPTED && !ck_condition_ok(written, 1)) { ck_unsupported("le soluzioni scritte usano qualcosa che il verificatore non conosce"); return; }
        }
    }

    /* I fattori: polinomi non costanti. */
    for (ck_next_line(); ck_status == CK_ACCEPTED && ck_word("factor"); ck_next_line()) {
        const CkNode *node = ck_read();
        size_t mark = ck_used;
        if (ck_status != CK_ACCEPTED) return;
        if (ck_factor_count == CK_MAX_FACTORS) { ck_unsupported("troppi fattori per il verificatore"); return; }
        int plain = ck_expression_ok(node, 0);
        ck_abs_count = ck_seen_count = ck_nowhere = 0;
        if (plain) ck_collect_abs(node);
        if (!plain || ck_abs_count) { ck_reject("certificato scritto male: un fattore non è un polinomio"); return; }
        CkRat value = ck_fraction(node, 0);
        if (ck_nowhere || !ck_poly_is_constant(value.den)) { ck_reject("un fattore non è un polinomio"); return; }
        ck_uni_of(value.num, &ck_factor[ck_factor_count]);
        if (ck_factor[ck_factor_count++].degree < 1 && ck_status == CK_ACCEPTED) { ck_reject("un fattore è un numero"); return; }
        ck_used = mark;
    }

    /* Le righe, fino a "end". */
    while (ck_status == CK_ACCEPTED && !ck_word("end")) {
        CkRow *r = &ck_rows[ck_row_count];
        long long k = 0;
        if (ck_row_count == 2 * CK_MAX_POINTS + 1) { ck_unsupported("tabella troppo lunga per il verificatore"); return; }
        memset(r, 0, sizeof *r);
        if (ck_word("interval")) r->kind = CK_INTERVAL;
        else if (ck_word("point")) r->kind = CK_POINT;
        else if (ck_word("root")) r->kind = CK_ROOT;
        else { ck_reject("certificato scritto male: voce sconosciuta o \"end\" mancante"); return; }
        if (r->kind == CK_ROOT && (!ck_read_integer(&k) || k < 1 || k > ck_factor_count)) { ck_reject("certificato scritto male: una radice nomina un fattore che non c'è"); return; }
        r->factor = (int)k - 1;
        r->value = ck_read_rational();
        if (r->kind == CK_ROOT) r->high = ck_read_rational();
        r->claimed = ck_read_truth();
        if ((r->kind == CK_INTERVAL) != (ck_row_count % 2 == 0)) { ck_reject("certificato scritto male: intervalli e punti devono alternarsi"); return; }
        ck_row_count++;
        ck_next_line();
    }
    if (ck_status != CK_ACCEPTED) return;
    if (ck_row_count % 2 == 0) { ck_reject("certificato scritto male: la tabella deve finire con un intervallo"); return; }

    /* 12e, 12d, 12g: le righe, i polinomi della condizione, gli estremi
     * delle soluzioni scritte; poi la verità in ogni riga. */
    ck_check_rows();
    if (ck_status == CK_ACCEPTED) ck_cover(condition);
    if (written && ck_status == CK_ACCEPTED) ck_check_endpoints(written);
    int all = 1, some = 0, same = 1;
    for (int row = 0; row < ck_row_count && ck_status == CK_ACCEPTED; row++) {
        int truth = ck_row_truth(condition, row);
        if (ck_status != CK_ACCEPTED) return;
        if (truth != ck_rows[row].claimed) { ck_reject("in una riga la condizione non ha la verità dichiarata"); return; }

        /* 12f. Nei razionali una riga root non conta, se la sua radice non è
         * razionale; e se lo è, il certificato doveva dirla con un punto. */
        if (rationals && ck_rows[row].kind == CK_ROOT) {
            if (!ck_irrational(row)) ck_reject("una radice di una riga root è razionale: va scritta con una riga point");
            continue;
        }
        all = all && truth;
        some = some || truth;

        /* 12g. */
        if (claim == 2) same = same && truth == (written ? ck_written(written, row) : everywhere);
    }
    if (ck_status != CK_ACCEPTED) return;

    /* 12h. Il verdetto: le righe che contano coprono D (12d, 12e). */
    if (claim == 0 ? all != claimed : claim == 1 ? some != claimed : !same) ck_reject("la tabella non dà ciò che il certificato afferma");
}

/* =============================================================================
 * 13. DERIVATE E PRIMITIVE
 * ============================================================================
 *
 * Gli enunciati dei capitoli 27 e 30 di mathresearcher: "la derivata di F è
 * G", "le primitive di f sono F + c", "l'integrale di f da a a b vale V". Il
 * certificato porta le espressioni, e il verificatore deriva F da sé, con
 * regole sue, poi decide con l'algebra esatta se la differenza è zero:
 *
 *     certificate derivative            certificate antiderivative
 *     variable x                        variable x
 *     order 2                           integrand (/ 1 x)
 *     function (* x (exp x))            primitive (lnabs x)
 *     derivative (* (+ x 2) (exp x))    end
 *     end
 *
 *     certificate integral
 *     variable x
 *     lower 0
 *     upper 1
 *     integrand (^ x 2)
 *     primitive (/ (^ x 3) 3)
 *     value (/ 1 3)
 *     end
 *
 * Una derivata n-esima ha "order n base n0", con n una lettera: allora G
 * può contenere n, F no. Dopo le voci, come nel capitolo 8, possono venire
 * delle voci "brick", fattori in cui provare a scomporre gli argomenti dei
 * logaritmi: un suggerimento non può far accettare il falso.
 *
 * LE PAROLE. Quelle del capitolo 8 (le somme, i prodotti e i fattoriali solo
 * se non dipendono da x), più (exp a), (sin a), (cos a), (atan a), (pi) e
 * (root m a), la radice di indice m, un intero scritto da 2 a CK_MAX_ROOT: per
 * m dispari la radice reale, che ha il segno di a, per m pari quella non
 * negativa di a >= 0, e (abs a), il valore assoluto (in un integrale
 * definito solo dentro l'argomento di un logaritmo). Una potenza (^ a b) con
 * b intero è quella del capitolo 8; con b un razionale p/q non intero è
 * (root q a)^p, con b non costante è e^{b ln a}: in tutti e due i casi vuole
 * a > 0. Le altre lettere oltre x (e n) sono parametri.
 *
 * DEFINITA E REGOLARE. Un'espressione è definita in x se nel suo calcolo
 * nessuna divisione ha il divisore nullo (un esponente intero negativo è una
 * divisione), l'argomento di ln è positivo e quello di lnabs non è zero, la
 * base di una potenza con l'esponente non intero è positiva, il radicando di
 * una radice di indice pari non è negativo. È regolare in x se in più ogni
 * radicando, e ogni argomento di un valore assoluto, è diverso da zero: lì
 * la funzione può non essere derivabile (la radice di x, o |x|, in 0). Senza
 * radici e valori assoluti le due parole coincidono.
 *
 * I TEOREMI. Per ogni valore dei parametri:
 *
 *   - derivative, ordine k: in ogni x in cui F è regolare e G è definita,
 *     F è derivabile k volte e F^(k)(x) = G(x);
 *   - derivative, ordine n da n0: per ogni intero n >= n0, in ogni x in cui
 *     F e G(n0, x), ..., G(n, x) sono regolari, F è derivabile n volte e
 *     F^(n)(x) = G(n, x);
 *   - antiderivative: su ogni intervallo aperto I in cui F è regolare e f è
 *     definita, le primitive di f sono esattamente le F + c, con c una
 *     costante;
 *   - integral: f è continua su [a, b] (o [b, a]), F è derivabile in un
 *     intervallo aperto che lo contiene con F' = f, e l'integrale di f da a
 *     a b vale V.
 *
 * COME LO SI CONTROLLA
 *
 *   13a. Le lettere delle funzioni (7g), e l'unità di angoli ed esponenti.
 *   13b. Il calcolo delle parole: un'espressione diventa una frazione di
 *        polinomi in x e in quelle lettere, e se ne annotano le condizioni
 *        di esistenza.
 *   13c. La derivata, una regola per ogni lettera, e la regola della catena
 *        sui polinomi: è un conto che finisce, senza nessuna ricerca.
 *   13d. Le relazioni tra le lettere (sin^2 + cos^2 = 1, R^m = u) e la
 *        prova dello zero, con le sue tre risposte.
 *   13e. L'intervallo [a, b]: le condizioni di esistenza, con Sturm, e il
 *        valore F(b) - F(a).
 *   13f. La lettura del certificato, e i conti di ognuno dei tre tipi.
 */

#define CK_MAX_UNIT 720720  /* l'unità più grande di angoli ed esponenti (13a) */
#define CK_MAX_ROOT 12      /* l'indice più grande di una radice */
#define CK_MAX_TURNS 64     /* il multiplo più grande di un angolo, o di un esponente, in unità */
#define CK_MAX_DERIVATIVE 8 /* l'ordine più grande di una derivata, o il caso base di una derivata n-esima */

/* 13a. LE LETTERE DELLE FUNZIONI, E L'UNITÀ
 *
 * Ogni lettera di 7g sta per una funzione di x; due espressioni che si
 * scrivono con le stesse lettere in modi diversi devono diventare lo stesso
 * polinomio, almeno nei casi dei libri: e^{2x} = (e^x)^2, sin 2x = 2 sin x
 * cos x. Per questo un esponente, o un angolo, che è un polinomio si divide
 * nei suoi monomi, u = c_1 m_1 + ... + c_k m_k, e
 *
 *     e^u = (e^{m_1/Q})^{c_1 Q} ... (e^{m_k/Q})^{c_k Q},
 *
 * con Q, l'unità, un intero per cui ogni c_i Q è intero: la lettera è
 * e^{m/Q}, e le potenze intere si fanno con i prodotti. Per seno e coseno
 * le formule di addizione, sin(a + b) = sin a cos b + cos a sin b e
 * cos(a + b) = cos a cos b - sin a sin b, ripetute, danno sin(c m) e
 * cos(c m) come polinomi nella coppia di lettere sin(m/Q), cos(m/Q). Q si
 * trova nella prima valutazione del certificato, di prova, allargandolo
 * ogni volta che serve (il minimo comune multiplo dei denominatori); le
 * valutazioni dopo lo usano così com'è.
 *
 * Un argomento che non è un polinomio, u = N/M, si tratta allo stesso modo:
 * i monomi sono quelli di N, e la lettera è e^{m/(Q M)}, con M scritto con il
 * coefficiente direttivo 1; così sin(2/x) = 2 sin(1/x) cos(1/x). E un angolo
 * k pi/2 ha il seno e il coseno esatti, 0 e 1 o -1; ogni altro multiplo di pi
 * greco, come pi/6, ha una coppia di lettere sua.
 *
 * Perché non può far accettare il falso. Ogni scrittura è un'uguaglianza
 * vera: due lettere che stanno per quantità legate (sin x e sin(x/3), se Q
 * fosse diverso) sarebbero solo trattate come indipendenti, e
 * un'uguaglianza tra polinomi in lettere qualsiasi vale in particolare per
 * i loro valori veri. Può soltanto non riconoscere uguali due quantità
 * uguali. */

/* L'unità del certificato in corso, e 1 se la si può ancora allargare. */
static long long ck_unit;
static int ck_learning;

/* Il nome della variabile reale del certificato in corso, che è sempre
 * l'indeterminata 0: le catene di Sturm del capitolo 12 la cercano lì. */
static char ck_dx;

/* Ritorna l'indeterminata di tipo kind (7g) con argomento base / below e,
 * per una ROOT, indice index (0 per gli altri tipi), creandola se manca. Una
 * SIN si crea insieme alla sua COS, che le sta subito dopo. */
static int ck_var_real(CkVarKind kind, CkPoly base, CkPoly below, long long index) {
    for (int i = 0; i < ck_var_count; i++)
        if (ck_vars[i].kind == kind && ck_vars[i].prime == index && ck_poly_equal(ck_vars[i].base, base) && ck_poly_equal(ck_vars[i].below, below)) return i;
    int v = ck_var_new((CkVar){.kind = kind, .prime = index, .base = base, .below = below});
    if (kind == CK_VAR_SIN) ck_var_new((CkVar){.kind = CK_VAR_COS, .base = base, .below = below});
    return v;
}

/* Ritorna r con il denominatore 1, se il denominatore è un numero: così due
 * argomenti uguali hanno la stessa scrittura, e un polinomio si riconosce. */
static CkRat ck_rat_plain(CkRat r) {
    if (!ck_poly_is_constant(r.den) || r.den.count == 0) return r;
    return (CkRat){ck_poly_mul(r.num, ck_poly_number(ck_num_inv(r.den.term[0].coef))), ck_poly_number(ck_num_int(1))};
}

/* Ritorna p^e, per e intero non negativo. */
static CkPoly ck_poly_power(CkPoly p, long long e) {
    CkPoly result = ck_poly_number(ck_num_int(1));
    for (long long i = 0; i < e && ck_status == CK_ACCEPTED; i++) result = ck_poly_mul(result, p);
    return result;
}

/* Ritorna la frazione v^k, con v un'indeterminata che non vale mai zero dove
 * compare (una potenza di e, una radice dove è regolare) e k intero. Il
 * denominatore di v^-k non è una condizione da annotare. */
static CkRat ck_rat_raise(int v, long long k) {
    CkPoly power = ck_poly_power(ck_poly_variable(v), k < 0 ? -k : k), one = ck_poly_number(ck_num_int(1));
    return k < 0 ? (CkRat){one, power} : (CkRat){power, one};
}

/* Ritorna il monomio del termine t, con il coefficiente 1. */
static CkPoly ck_monomial(const CkTerm *t) {
    CkPoly m = ck_poly_number(ck_num_int(1));
    memcpy(m.term[0].e, t->e, CK_MAX_VARS);
    return m;
}

/* Mette in *k il numero intero coef Q, allargando prima l'unità se si può;
 * ritorna 0, con il certificato non controllabile, se non ci si riesce. */
static int ck_turns(CkNum coef, long long *k) {
    if (ck_unit % coef.den != 0) {
        __int128 wider = ck_unit / ck_gcd(ck_unit, coef.den) * coef.den;
        if (!ck_learning || wider > CK_MAX_UNIT) { ck_unsupported("angoli o esponenti con denominatori che il verificatore non tratta"); return 0; }
        ck_unit = (long long)wider;
    }
    __int128 turns = coef.num * (ck_unit / coef.den);
    if (turns > CK_MAX_TURNS || turns < -CK_MAX_TURNS) { ck_unsupported("un angolo o un esponente troppo grande per il verificatore"); return 0; }
    *k = (long long)turns;
    return 1;
}

/* 13b. IL CALCOLO DELLE PAROLE
 *
 * ck_real_eval() calcola, quando ck_real è 1, le parole delle funzioni e le
 * potenze, che ck_eval() le passa; tutto il resto lo fa ck_eval(). Annota le
 * condizioni di esistenza: con ck_recording a 1 i divisori vanno, come nel
 * capitolo 9, in ck_divisors[] (e lì vanno anche gli argomenti di lnabs);
 * le altre in ck_conditions[]. Servono solo agli integrali definiti
 * (13e). */

/* Una condizione di esistenza: il polinomio p deve essere positivo (positive
 * = 1) o diverso da zero. certain è 0 per le condizioni di regolarità delle
 * radici, che sono più severe del necessario per la funzione integranda: se
 * non valgono, il certificato non è sbagliato, solo non controllabile. */
typedef struct {
    CkPoly p;
    int positive, certain;
} CkCondition;

static CkCondition ck_conditions[CK_MAX_DIVISORS];
static int ck_condition_count;

/* La scelta dei segni dei valori assoluti in corso: il bit i è 1 se
 * l'argomento di ck_abs[i] (12c) è preso negativo (13f). */
static unsigned ck_abs_pattern;

/* Annota che la frazione r deve essere positiva (positive = 1) o diversa da
 * zero. r = N / M è positiva esattamente quando lo è N M, perché M non è
 * zero (il suo divisore è già annotato). Un numero si decide subito: una
 * condizione falsa ovunque vuol dire che l'espressione non è definita in
 * nessun punto dell'intervallo. */
static void ck_condition(CkRat r, int positive, int certain) {
    CkPoly p = positive ? ck_poly_mul(r.num, r.den) : r.num;
    if (!ck_recording) return;
    if (ck_poly_is_constant(p)) {
        int holds = p.count > 0 && (!positive || p.term[0].coef.num > 0);
        if (!holds && certain) ck_reject("la funzione non è definita in nessun punto dell'intervallo");
        else if (!holds) ck_unsupported("non so stabilire che la funzione integranda e la primitiva sono definite su tutto l'intervallo");
        return;
    }
    if (ck_condition_count == CK_MAX_DIVISORS) { ck_unsupported("troppe condizioni di esistenza per il verificatore"); return; }
    ck_conditions[ck_condition_count++] = (CkCondition){p, positive, certain};
}

/* Scrive u = N/M con M monico, il coefficiente del termine direttivo 1: così
 * due argomenti uguali hanno la stessa scrittura, e 2/x e 4/(2x) diventano
 * entrambi 2 per il monomio 1 su x (13a). */
static CkRat ck_rat_monic(CkRat u) {
    CkNum lead;
    CkPoly below = ck_poly_monic(u.den, &lead);
    return (CkRat){ck_poly_mul(u.num, ck_poly_number(ck_num_inv(lead))), below};
}

/* Ritorna e^u: vedi 13a. Con u = N/M, ogni monomio m di N dà la lettera
 * e^{m/(Q M)}, alla potenza intera c Q. */
static CkRat ck_real_exp(CkRat u) {
    CkRat result = ck_rat_int(1);
    u = ck_rat_monic(u);
    for (int i = 0; i < u.num.count && ck_status == CK_ACCEPTED; i++) {
        long long k;
        if (!ck_turns(u.num.term[i].coef, &k)) break;
        CkPoly below = ck_poly_mul(u.den, ck_poly_number(ck_num_int(ck_unit)));
        result = ck_rat_mul(result, ck_rat_raise(ck_var_real(CK_VAR_E, ck_monomial(&u.num.term[i]), below, 0), k));
    }
    return result;
}

static CkPoly ck_reduce(CkPoly p, int final);

/* Ritorna sin u (cosine = 0) o cos u (cosine = 1), come polinomio nelle
 * lettere di 13a. Le formule di addizione danno, dalla coppia (s, c) di un
 * angolo e (S, C) di un altro, quella della loro somma, (s C + c S,
 * c C - s S): ripetute k volte con lo stesso (S, C), danno l'angolo k
 * volte, e con S cambiato di segno l'angolo -k volte. */
static CkPoly ck_real_trig(CkRat u, int cosine) {
    CkPoly s = ck_poly_number(ck_num_int(0)), c = ck_poly_number(ck_num_int(1));
    int pi = -1;
    u = ck_rat_monic(u);
    for (int v = 0; v < ck_var_count && ck_poly_is_constant(u.den); v++)
        if (ck_vars[v].kind == CK_VAR_PI) pi = v;
    for (int i = 0; i < u.num.count && ck_status == CK_ACCEPTED; i++) {
        const CkTerm *t = &u.num.term[i];
        CkPoly ts = ck_poly_number(ck_num_int(0)), tc = ck_poly_number(ck_num_int(1)), m = ck_monomial(t);
        CkNum half = ck_num_mul(t->coef, ck_num_int(2));
        long long k;

        /* Un multiplo di pi greco: k pi/2 con i valori esatti, ogni altro con
         * la sua coppia di lettere. */
        if (pi >= 0 && ck_poly_equal(m, ck_poly_variable(pi))) {
            if (half.den == 1) {
                int quarter = (int)(((half.num % 4) + 4) % 4);
                ts = ck_poly_number(ck_num_int(quarter == 1 ? 1 : quarter == 3 ? -1 : 0));
                tc = ck_poly_number(ck_num_int(quarter == 0 ? 1 : quarter == 2 ? -1 : 0));
            } else {
                int v = ck_var_real(CK_VAR_SIN, ck_poly_mul(m, ck_poly_number(t->coef)), ck_poly_number(ck_num_int(1)), 0);
                ts = ck_poly_variable(v);
                tc = ck_poly_variable(v + 1);
            }
        } else {
            /* Ogni altro monomio: l'angolo m/(Q M), k volte. */
            if (!ck_turns(t->coef, &k)) break;
            int v = ck_var_real(CK_VAR_SIN, m, ck_poly_mul(u.den, ck_poly_number(ck_num_int(ck_unit))), 0);
            CkPoly S = ck_poly_variable(v), C = ck_poly_variable(v + 1);
            if (k < 0) S = ck_poly_mul(S, ck_poly_number(ck_num_int(-1)));
            for (long long j = 0; j < (k < 0 ? -k : k) && ck_status == CK_ACCEPTED; j++) {
                CkPoly next = ck_poly_add(ck_poly_mul(ts, C), ck_poly_mul(tc, S), 0);
                tc = ck_reduce(ck_poly_add(ck_poly_mul(tc, C), ck_poly_mul(ts, S), 1), 0);
                ts = ck_reduce(next, 0);
            }
        }

        /* La somma degli angoli. */
        CkPoly next = ck_poly_add(ck_poly_mul(s, tc), ck_poly_mul(c, ts), 0);
        c = ck_reduce(ck_poly_add(ck_poly_mul(c, tc), ck_poly_mul(s, ts), 1), 0);
        s = ck_reduce(next, 0);
    }
    return cosine ? c : s;
}

/* Ritorna arctan u. Con u un numero: arctan 0 = 0 e arctan(1) = pi/4
 * (arctan(-1) = -pi/4), perché tan(pi/4) = 1; ogni altro ha la sua
 * lettera. */
static CkRat ck_real_atan(CkRat u) {
    u = ck_rat_plain(u);
    if (ck_poly_is_constant(u.num) && ck_poly_is_constant(u.den)) {
        CkNum value = u.num.count ? u.num.term[0].coef : ck_num_int(0);
        if (value.num == 0) return ck_rat_int(0);
        if (value.den == 1 && (value.num == 1 || value.num == -1)) {
            CkPoly pi = ck_poly_variable(ck_var_real(CK_VAR_PI, (CkPoly){0, NULL}, (CkPoly){0, NULL}, 0));
            return ck_rat_poly(ck_poly_mul(pi, ck_poly_number(ck_num_make(value.num, 4))));
        }
    }
    return ck_rat_poly(ck_poly_variable(ck_var_real(CK_VAR_ATAN, u.num, u.den, 0)));
}

/* Ritorna 1 e mette in *root l'intero r >= 0 con r^m = n, se c'è; n >= 0.
 * Per bisezione: r^m cresce con r. */
static int ck_exact_root(__int128 n, long long m, __int128 *root) {
    __int128 low = 0, high = 1;
    for (;;) {
        /* high^m, fermandosi appena supera n. */
        __int128 power = 1;
        for (long long i = 0; i < m && power <= n; i++) power *= high;
        if (power > n) break;
        high *= 2;
    }
    while (high - low > 1) {
        __int128 middle = low + (high - low) / 2, power = 1;
        for (long long i = 0; i < m && power <= n; i++) power *= middle;
        if (power <= n) low = middle; else high = middle;
    }
    __int128 power = 1;
    for (long long i = 0; i < m; i++) power *= low;
    *root = low;
    return power == n;
}

/* Le radici di uno stesso radicando con indici diversi, come la radice
 * quadrata, la cubica e la sesta di x, si scrivono con una lettera sola:
 * con L il minimo comune multiplo degli indici, la radice m-esima di u è
 * R^{L/m}, con R la radice L-esima di u. Per L dispari vale per ogni u reale
 * (la radice dispari reale rispetta le potenze: (R^{L/m})^m = R^L = u, e
 * t -> t^m è crescente). Per L pari vuole u >= 0, e c'è: L è pari solo se
 * tra quelle radici ce n'è una di indice pari, che è definita solo dove
 * u >= 0, e le espressioni del certificato si confrontano solo dove sono
 * definite tutte. Per questo il radicando si riconosce dall'albero com'è
 * scritto (ck_same()), che resta lo stesso nelle tre valutazioni, e L si
 * trova nella prima, come l'unità. Le voci brick, che sono solo
 * suggerimenti, non possono contenere radici: una di indice pari lì
 * renderebbe pari L senza che le espressioni vere lo chiedano. */
typedef struct {
    const CkNode *tree;  /* il radicando, com'è scritto nel certificato */
    long long unit;      /* L */
} CkRootUnit;

static CkRootUnit ck_root_units[CK_MAX_BRICKS];
static int ck_root_unit_count;
static int ck_hinting;  /* 1 mentre si calcolano le voci brick */

/* Ritorna (radice m-esima di u)^power, con u il valore del radicando tree.
 * La radice di un numero razionale che è una potenza m-esima è esatta, con
 * il segno per m dispari; quella di indice pari di un numero negativo non
 * esiste. Ogni altra è una potenza di una lettera R, con R^L = u (13d), che
 * non vale zero dove l'espressione è regolare: la potenza negativa non è una
 * condizione da annotare. */
static CkRat ck_real_root(CkRat u, long long m, const CkNode *tree, long long power) {
    if (ck_hinting) { ck_unsupported("una voce brick contiene una radice"); return ck_rat_int(0); }
    u = ck_rat_plain(u);
    if (ck_poly_is_constant(u.num) && ck_poly_is_constant(u.den)) {
        CkNum value = u.num.count ? u.num.term[0].coef : ck_num_int(0);
        __int128 top, bottom, size = value.num < 0 ? -value.num : value.num;
        if (value.num < 0 && m % 2 == 0) { ck_reject("il certificato contiene la radice di indice pari di un numero negativo"); return ck_rat_int(0); }
        if (ck_exact_root(size, m, &top) && ck_exact_root(value.den, m, &bottom)) {
            if (top == 0 && power < 0) { ck_reject("il certificato divide per zero"); return ck_rat_int(0); }
            return ck_rat_power(ck_rat_poly(ck_poly_number(ck_num_make(value.num < 0 ? -top : top, bottom))), power);
        }
    }

    /* L: dalla tabella, che solo la prima valutazione allarga. */
    int i = 0;
    while (i < ck_root_unit_count && !ck_same(ck_root_units[i].tree, tree)) i++;
    if (i == ck_root_unit_count) {
        if (!ck_learning || i == CK_MAX_BRICKS) { ck_unsupported("radici che il verificatore non sa ricondurre a una sola"); return ck_rat_int(0); }
        ck_root_units[ck_root_unit_count++] = (CkRootUnit){tree, m};
    }
    long long unit = ck_root_units[i].unit;
    if (unit % m != 0) {
        if (!ck_learning || unit / ck_gcd(unit, m) * m > CK_MAX_UNIT) { ck_unsupported("radici che il verificatore non sa ricondurre a una sola"); return ck_rat_int(0); }
        unit = ck_root_units[i].unit = unit / (long long)ck_gcd(unit, m) * m;
    }
    return ck_rat_raise(ck_var_real(CK_VAR_ROOT, u.num, u.den, unit), unit / m * power);
}

/* Il valore di node, che è una parola delle funzioni, un logaritmo o una
 * potenza, quando ck_real è 1: vedi in testa al capitolo. */
static CkRat ck_real_eval(const CkNode *node) {
    long long a, c;
    CkRat base, exponent;
    switch (node->kind) {
    case CK_PI:
        return ck_rat_poly(ck_poly_variable(ck_var_real(CK_VAR_PI, (CkPoly){0, NULL}, (CkPoly){0, NULL}, 0)));
    case CK_ABS: {
        /* La scelta dei segni in corso (ck_calculus_pass()). */
        int i = 0;
        while (i < ck_abs_count && !ck_same(ck_abs[i], node)) i++;
        if (i == ck_abs_count) { ck_unsupported("il valore assoluto in un integrale definito si calcola solo dentro l'argomento di un logaritmo"); return ck_rat_int(0); }
        base = ck_eval(node->a);
        return ck_abs_pattern >> i & 1 ? ck_rat_add(ck_rat_int(0), base, 1) : base;
    }
    case CK_EXP:
        return ck_real_exp(ck_eval(node->a));
    case CK_SIN: case CK_COS:
        return ck_rat_poly(ck_real_trig(ck_eval(node->a), node->kind == CK_COS));
    case CK_ATAN:
        return ck_real_atan(ck_eval(node->a));
    case CK_RADICAL: {
        /* La radice m-esima di B^k è la potenza k-esima di quella di B, per
         * m dispari (vedi sopra ck_real_root()) e per k dispari: allora
         * B^k >= 0 esattamente dove B >= 0, e lì le radici rispettano le
         * potenze. Le condizioni si annotano su B, che vale zero, o è
         * negativo, esattamente dove lo è B^k. */
        long long m = node->a->value, k = 1;
        const CkNode *radicand = node->b;
        if (node->a->kind != CK_NUMBER || m < 2 || m > CK_MAX_ROOT) { ck_unsupported("radice con un indice che il verificatore non tratta"); return ck_rat_int(0); }
        if (radicand->kind == CK_POW && radicand->b->kind == CK_NUMBER && radicand->b->value >= 1 && radicand->b->value <= CK_MAX_TURNS && (m % 2 || radicand->b->value % 2)) {
            k = radicand->b->value;
            radicand = radicand->a;
        }
        base = ck_eval(radicand);
        ck_condition(base, m % 2 == 0, 0);
        return ck_real_root(base, m, radicand, k);
    }
    case CK_LN: case CK_LNABS: {
        /* La condizione sull'argomento intero, se si annotano; poi
         * ck_ln_node() ne calcola i pezzi, e le loro condizioni sono già in
         * quella. Così un valore assoluto dentro l'argomento, che si calcola
         * solo così (8g), non chiede di essere calcolato altrove. */
        int recording = ck_recording;
        if (recording) {
            base = ck_eval(node->a);
            if (node->kind == CK_LN) ck_condition(base, 1, 1);
            else ck_record(base.num);
        }
        ck_recording = 0;
        CkPoly value = ck_ln_node(node->a);
        ck_recording = recording;
        return ck_rat_poly(value);
    }
    default: {
        /* Una potenza. Esponente a n + c: il capitolo 8. Altrimenti la base
         * deve essere positiva, e la potenza è una radice o e^{b ln a}. */
        base = ck_eval(node->a);
        exponent = ck_eval(node->b);
        if (ck_affine(exponent, &a, &c)) return a == 0 ? ck_rat_power(base, c) : ck_eval_power(base, a, c);
        ck_condition(base, 1, 1);
        exponent = ck_rat_plain(exponent);
        if (ck_poly_is_constant(exponent.num) && ck_poly_is_constant(exponent.den)) {
            CkNum q = exponent.num.term[0].coef;
            if (q.den > CK_MAX_ROOT || q.num > CK_MAX_TURNS || q.num < -CK_MAX_TURNS) { ck_unsupported("una potenza che il verificatore non tratta"); return ck_rat_int(0); }
            return ck_real_root(base, (long long)q.den, node->a, (long long)q.num);
        }
        int recording = ck_recording;
        ck_recording = 0;
        CkPoly logarithm = ck_ln_node(node->a);
        ck_recording = recording;
        return ck_real_exp(ck_rat_mul(exponent, ck_rat_poly(logarithm)));
    }
    }
}

/* 13c. LA DERIVATA
 *
 * Un'espressione calcolata è una frazione N/M di polinomi nelle lettere, e
 * ogni lettera è una funzione di x. La derivata di un polinomio p nelle
 * lettere v_1, ..., v_k è, per la regola della catena per le funzioni di più
 * variabili,
 *
 *     p' = (dp/dv_1) v_1' + ... + (dp/dv_k) v_k',
 *
 * dove dp/dv è la derivata di p come polinomio nella sola v (ogni termine
 * c v^e ... diventa c e v^(e-1) ...); per una frazione vale la regola del
 * quoziente, (N/M)' = (N' M - N M') / M^2. Le derivate delle lettere:
 *
 *     x' = 1;  un parametro, n, (-1)^n, p^n, n!, pi greco: 0;
 *     (B^n)' = n B^n B'/B, per n intero, dove B non è zero;
 *     (ln|B|)' = B'/B, dove B non è zero (per B < 0, ln|B| = ln(-B), e
 *       (ln(-B))' = -B'/(-B));
 *     (e^u)' = e^u u';  (sin u)' = cos(u) u';  (cos u)' = -sin(u) u';
 *     (arctan u)' = u' / (1 + u^2);
 *     R = la radice m-esima di u: da R^m = u, m R^(m-1) R' = u', e quindi
 *       R' = u' / (m R^(m-1)) = R u' / (m u), dove u non è zero.
 *
 * Perché il risultato è la derivata di F dove F è regolare. Il calcolo di F
 * dà N/M con N/M = F dove F è definita (capitoli 5 e 8), e ogni lettera di
 * N/M viene da un pezzo di F: dove F è regolare il pezzo è definito, e la
 * lettera è derivabile, perché B, u, l'argomento di un logaritmo e il
 * radicando non sono zero lì (B divide l'argomento del suo logaritmo, che è
 * diverso da zero). L'insieme in cui F è regolare è aperto (ogni condizione
 * è stretta, su funzioni continue), quindi N/M = F in un intorno di ogni
 * suo punto, e là la derivata di F è quella di N/M, calcolata con le regole
 * qui sopra. I denominatori di quella derivata sono M^2 e quelli delle
 * lettere, B, u, m u, 1 + u^2, tutti diversi da zero lì. Le lettere della
 * derivata sono ancora quelle di F: si può derivare di nuovo, e per
 * induzione la derivata k-esima di F è quella calcolata k volte, in ogni
 * punto in cui F è regolare. */

/* Le derivate delle lettere: il bit v di ck_derived dice se ck_dvar[v] è
 * già calcolata. Si ricalcolano a ogni valutazione del certificato. */
static CkRat ck_dvar[CK_MAX_VARS];
static unsigned ck_derived;
_Static_assert(CK_MAX_VARS <= 32, "ck_derived ha un bit per ogni indeterminata");

/* Il prototipo è in avanti perché la derivata di una lettera chiede quella
 * del suo argomento, e quella di un polinomio le derivate delle lettere. */
static CkRat ck_var_derivative(int v);

/* Ritorna la derivata del polinomio p, con la regola della catena. */
static CkRat ck_poly_derivative(CkPoly p) {
    CkRat total = ck_rat_int(0);
    for (int v = 0; v < ck_var_count && ck_status == CK_ACCEPTED; v++) {
        if (!ck_poly_uses(p, v)) continue;
        CkRat slope = ck_var_derivative(v);
        if (slope.num.count == 0) continue;

        /* dp/dv. */
        CkTerm *t = ck_alloc(sizeof *t * (size_t)(p.count + 1));
        int count = 0;
        if (ck_status != CK_ACCEPTED) break;
        for (int i = 0; i < p.count; i++) {
            if (!p.term[i].e[v]) continue;
            t[count] = p.term[i];
            t[count].coef = ck_num_mul(t[count].coef, ck_num_int(p.term[i].e[v]));
            t[count++].e[v]--;
        }
        total = ck_rat_add(total, ck_rat_mul(ck_rat_poly(ck_poly_build(t, count)), slope), 0);
    }
    return total;
}

/* Ritorna la derivata della frazione r, con la regola del quoziente. */
static CkRat ck_rat_derivative(CkRat r) {
    CkRat top = ck_poly_derivative(r.num);
    if (ck_poly_is_constant(r.den)) return (CkRat){top.num, ck_poly_mul(top.den, r.den)};
    top = ck_rat_add(ck_rat_mul(top, ck_rat_poly(r.den)), ck_rat_mul(ck_rat_poly(r.num), ck_poly_derivative(r.den)), 1);
    return (CkRat){top.num, ck_poly_mul(top.den, ck_poly_mul(r.den, r.den))};
}

/* Ritorna la derivata della lettera v, con le regole in testa a 13c. */
static CkRat ck_var_derivative(int v) {
    if (ck_derived >> v & 1) return ck_dvar[v];
    const CkVar x = ck_vars[v];
    CkPoly self = ck_poly_variable(v);
    CkRat d = ck_rat_int(0), slope = d;
    if (x.kind == CK_VAR_E || x.kind == CK_VAR_SIN || x.kind == CK_VAR_COS || x.kind == CK_VAR_ATAN || x.kind == CK_VAR_ROOT)
        slope = ck_rat_derivative((CkRat){x.base, x.below});
    switch (x.kind) {
    case CK_VAR_LETTER:
        d = ck_rat_int(x.name == ck_dx);
        break;
    case CK_VAR_BIG:
        if (ck_mentions(x.node->c, ck_dx)) ck_unsupported("la derivata di una somma o di un prodotto il cui termine dipende dalla variabile");
        break;
    case CK_VAR_EXP: case CK_VAR_LN:
        if (x.prime) break;
        slope = ck_poly_derivative(x.base);
        if (x.kind == CK_VAR_EXP) slope.num = ck_poly_mul(slope.num, ck_poly_mul(self, ck_poly_variable(ck_var_letter(ck_variable))));
        d = (CkRat){slope.num, ck_poly_mul(slope.den, x.base)};
        break;
    case CK_VAR_E:
        d = (CkRat){ck_poly_mul(self, slope.num), slope.den};
        break;
    case CK_VAR_SIN: case CK_VAR_COS: {
        CkPoly other = ck_poly_variable(x.kind == CK_VAR_SIN ? v + 1 : v - 1);
        if (x.kind == CK_VAR_COS) other = ck_poly_mul(other, ck_poly_number(ck_num_int(-1)));
        d = (CkRat){ck_poly_mul(other, slope.num), slope.den};
        break;
    }
    case CK_VAR_ATAN: {
        /* u = N/M: u' / (1 + u^2) = u' M^2 / (M^2 + N^2). */
        CkPoly square = ck_poly_mul(x.below, x.below);
        d = (CkRat){ck_poly_mul(slope.num, square), ck_poly_mul(slope.den, ck_poly_add(square, ck_poly_mul(x.base, x.base), 0))};
        break;
    }
    case CK_VAR_ROOT:
        /* u = N/M: R u' / (m u) = R u' M / (m N). */
        d = (CkRat){ck_poly_mul(ck_poly_mul(self, slope.num), x.below), ck_poly_mul(ck_poly_mul(slope.den, x.base), ck_poly_number(ck_num_int(x.prime)))};
        break;
    default:
        break;
    }
    ck_dvar[v] = d;
    ck_derived |= 1u << v;
    return d;
}

/* 13d. LE RELAZIONI E LA PROVA DELLO ZERO
 *
 * Una differenza calcolata vale zero dove le due espressioni sono definite
 * se lo vale il suo numeratore, che è un polinomio nelle lettere. Un
 * polinomio nullo vale zero comunque si scelgano i valori delle lettere,
 * quindi in particolare per quelli veri. Ma le lettere non sono
 * indipendenti, e due relazioni servono a riconoscere zero ciò che lo è:
 *
 *   - sin^2 u + cos^2 u = 1: si sostituisce cos^2 u con 1 - sin^2 u, finché
 *     il coseno ha esponente 0 o 1;
 *   - R^m = u per la radice m-esima R di u = N/M: con M un numero R^m
 *     diventa N/M; altrimenti, alla fine, si moltiplica il polinomio per M,
 *     che dove u è definita non è zero (il numeratore resta zero o no
 *     insieme a lui), e M R^m diventa N.
 *
 * Ogni sostituzione rimpiazza una quantità con una uguale dove le
 * espressioni sono definite: il valore del polinomio, in quei punti, non
 * cambia, oppure è moltiplicato per un numero diverso da zero. Le lettere si
 * trattano dall'ultima alla prima: l'argomento di una lettera contiene solo
 * lettere create prima di lei, e ciò che una sostituzione introduce si
 * tratta dopo.
 *
 * Le tre risposte. Un numeratore che si riduce a zero: la differenza è
 * zero. Uno che non si riduce a zero, se contiene solo lettere algebriche
 * (x, i parametri, n e le quantità dell'induzione), è un polinomio non
 * nullo: le due espressioni sono diverse, e il certificato è respinto. Lo
 * stesso in alcuni casi con una sola funzione trascendente, come e^x o
 * sin x, o un solo numero trascendente, come pi greco o ln 2
 * (ck_certainly_nonzero()). Negli altri un numeratore non nullo può ancora
 * valere zero per una relazione che qui non si conosce, come la radice di
 * x^2 che è |x|, o ln(x^2) che è 2 ln|x|: allora il certificato non è
 * controllabile, e non è un sì. */

/* Ritorna p con le relazioni applicate, come in testa a 13d. Con final = 0
 * solo quelle che non cambiano il valore (tutte tranne R^m = N/M con M non
 * costante): servono a tenere piccoli i polinomi durante i conti. */
static CkPoly ck_reduce(CkPoly p, int final) {
    for (int v = ck_var_count - 1; v >= 0 && ck_status == CK_ACCEPTED; v--) {
        const CkVar *x = &ck_vars[v];
        if (x->kind != CK_VAR_COS && x->kind != CK_VAR_ROOT) continue;
        CkPoly value, multiplier = ck_poly_number(ck_num_int(1));
        long long m = x->kind == CK_VAR_COS ? 2 : x->prime;
        if (x->kind == CK_VAR_COS) {
            value = ck_poly_add(multiplier, ck_poly_mul(ck_poly_variable(v - 1), ck_poly_variable(v - 1)), 1);
        } else if (ck_poly_is_constant(x->below)) {
            value = ck_poly_mul(x->base, ck_poly_number(ck_num_inv(x->below.term[0].coef)));
        } else {
            if (!final) continue;
            value = x->base;
            multiplier = x->below;
        }

        /* p = A v^m + B, con A e B i termini che hanno e quelli che non
         * hanno v^m: p diventa A value + B multiplier. Il più alto
         * esponente di v scende di m a ogni giro. */
        for (int round = 0; round <= CK_MAX_EXPONENT && ck_status == CK_ACCEPTED; round++) {
            CkTerm *high = ck_alloc(sizeof *high * (size_t)(p.count + 1)), *low = ck_alloc(sizeof *low * (size_t)(p.count + 1));
            int highs = 0, lows = 0;
            if (ck_status != CK_ACCEPTED) break;
            for (int i = 0; i < p.count; i++) {
                if (p.term[i].e[v] >= m) { high[highs] = p.term[i]; high[highs++].e[v] -= (signed char)m; }
                else low[lows++] = p.term[i];
            }
            if (highs == 0) break;
            p = ck_poly_add(ck_poly_mul(ck_poly_build(high, highs), value), ck_poly_mul(ck_poly_build(low, lows), multiplier), 0);
        }
    }
    return p;
}

/* Ritorna 1 se il polinomio p, che non è il polinomio nullo, è di sicuro
 * diverso da zero nei valori veri delle sue lettere, per qualche valore dei
 * parametri, in qualche punto di ogni intervallo aperto di x: vedi in testa
 * a 13d. Le lettere algebriche sono x, i parametri, n e le quantità
 * dell'induzione ((-1)^n, p^n, B^n, le somme, le successioni), che il
 * capitolo 8 tratta da indipendenti e per la stessa ragione respinge. I casi:
 *
 *   13d-1. Solo lettere algebriche: un polinomio non nullo in valori
 *          qualsiasi. Come funzione di x vale zero solo in un numero finito
 *          di punti.
 *   13d-2. In più un solo numero trascendente t: pi greco (Lindemann, 1882),
 *          e^r con r razionale non nullo (Hermite-Lindemann), ln p con p
 *          primo (se fosse algebrico, e^{ln p} = p non sarebbe un intero).
 *          p è un polinomio nelle lettere algebriche, con coefficienti
 *          polinomi in t a coefficienti razionali, non tutti nulli; un
 *          numero trascendente non è radice di nessun polinomio non nullo a
 *          coefficienti razionali, quindi qualche coefficiente non è zero,
 *          ed è 13d-1.
 *   13d-3. In più una sola lettera E = e^{x^d/Q}, con d >= 1. Come funzione
 *          di x p(x, E) è analitica su tutta la retta: se valesse zero su un
 *          intervallo varrebbe zero ovunque (principio di identità). Ma
 *          p = c_0(x) + c_1(x) E + ... + c_k(x) E^k, con c_k non nullo, e per
 *          x -> +infinito E = e^{x^d/Q} cresce più di ogni polinomio: il
 *          termine c_k(x) E^k vince su tutti gli altri, e p non è zero.
 *   13d-4. In più la sola coppia S = sin(x^d/Q), C = cos(x^d/Q), d >= 1, con
 *          C di grado 0 o 1 (13d): p = A(x, S) + C B(x, S). Analitica anche
 *          lei; se fosse zero ovunque, A^2 = C^2 B^2 = (1 - S^2) B^2 per ogni
 *          x. Un polinomio Q(x, S) che vale zero per ogni x è il polinomio
 *          nullo: per ogni y0 in (-1, 1) sin(x^d/Q) = y0 in infiniti x, e
 *          Q(x, y0), polinomio in x con infinite radici, è zero; allora lo
 *          sono i coefficienti di ogni potenza di S. Quindi A^2 = (1 - S^2) B^2
 *          come polinomi, e il fattore S - 1, che divide il secondo membro una
 *          volta più di un quadrato, divide il primo un numero pari di volte:
 *          A = B = 0, e p sarebbe nullo.
 *   13d-5. Solo logaritmi di primi, ognuno con esponente 1 e nessun
 *          prodotto tra loro: c_1 ln p_1 + ... + c_k ln p_k + c_0, a
 *          coefficienti razionali non tutti nulli. Se valesse zero,
 *          moltiplicando per un denominatore comune d si avrebbe
 *          p_1^{d c_1} ... p_k^{d c_k} = e^{-d c_0}: razionale a sinistra,
 *          trascendente a destra se c_0 non è zero (13d-2); e con c_0 = 0 la
 *          scomposizione in primi è unica, quindi tutti i c_i sono zero. */
static int ck_certainly_nonzero(CkPoly p) {
    int other = -1, kinds = 0, logarithms = 1;
    for (int v = 0; v < ck_var_count; v++) {
        if (!ck_poly_uses(p, v)) continue;
        const CkVar *x = &ck_vars[v];
        if (!(x->kind == CK_VAR_LN && x->prime)) logarithms = 0;
        if (x->kind == CK_VAR_LETTER || x->kind == CK_VAR_SIGN || x->kind == CK_VAR_BIG || x->kind == CK_VAR_SEQ || x->kind == CK_VAR_EXP) continue;

        /* La lettera non algebrica: una sola, o la coppia sin, cos. */
        int pair = (x->kind == CK_VAR_COS && other == v - 1);
        if (!pair) kinds++;
        other = v;
        int number = x->kind == CK_VAR_PI || (x->kind == CK_VAR_LN && x->prime) || (x->kind == CK_VAR_E && ck_poly_is_constant(x->base) && ck_poly_is_constant(x->below));
        int power = 0;
        for (int w = 1; w < CK_MAX_VARS && (x->kind == CK_VAR_E || x->kind == CK_VAR_SIN || x->kind == CK_VAR_COS); w++)
            if (x->base.count == 1 && x->base.term[0].e[w]) power = -1;
        if (power == 0 && x->base.count == 1) power = x->base.term[0].e[0];
        int function = (x->kind == CK_VAR_E || x->kind == CK_VAR_SIN || x->kind == CK_VAR_COS) && ck_poly_is_constant(x->below) && power >= 1;
        if (!number && !function) kinds = 2;
    }
    for (int i = 0; i < p.count && logarithms; i++) {
        int degree = 0;
        for (int v = 0; v < ck_var_count; v++) degree += p.term[i].e[v];
        if (degree > 1) logarithms = 0;
    }
    return kinds <= 1 || logarithms;
}

/* Controlla che la frazione difference valga zero dove è definita: se non
 * è così il certificato è respinto con il motivo wrong, oppure, quando non
 * si sa dire, non controllabile (vedi in testa a 13d).
 *
 * Con dei valori assoluti la differenza è quella di una scelta dei segni
 * (13f), e la scelta può non capitare in nessun punto in cui le espressioni
 * sono definite: per |x| / sqrt(x) la scelta |x| = -x vorrebbe x < 0, dove
 * la radice non c'è. Lì una differenza non nulla non dice che il
 * certificato è sbagliato, e non lo si respinge. */
static void ck_calculus_zero(CkRat difference, const char *wrong) {
    CkPoly top = ck_reduce(difference.num, 1);
    if (ck_status != CK_ACCEPTED || top.count == 0) return;
    if (ck_abs_count == 0 && ck_certainly_nonzero(top)) ck_reject(wrong);
    else if (ck_abs_count) ck_unsupported("non so stabilire che la differenza è zero: una scelta dei segni dei valori assoluti, che può non capitare in nessun punto, non la annulla");
    else ck_unsupported("non so stabilire che la differenza è zero: le funzioni trascendenti e le radici possono avere relazioni che il verificatore non conosce");
}

/* 13e. L'INTERVALLO [a, b]
 *
 * Il teorema fondamentale del calcolo integrale: se F è derivabile in un
 * intervallo aperto che contiene [a, b], con F' = f, e f è continua su
 * [a, b], allora l'integrale di f da a a b è F(b) - F(a) (anche con b < a,
 * per la convenzione che lo scambio degli estremi cambia il segno). Basta
 * che F sia regolare e f definita in ogni punto di [a, b]: lì F' = f
 * (13c), le espressioni sono continue dove sono definite, e l'insieme dei
 * punti in cui F è regolare e f definita è aperto, quindi contiene un
 * intervallo aperto attorno a [a, b] (un compatto dentro un aperto ne è
 * lontano di una distanza positiva).
 *
 * Le condizioni di esistenza annotate calcolando F e f (13b) sono polinomi:
 * "p diverso da zero" vale su [a, b] se p non vale zero in a né in b e,
 * per il teorema di Sturm (12a), non ha radici tra i due; "p positivo" se
 * in più p(a) > 0, perché un polinomio che non si annulla su un intervallo
 * vi ha sempre lo stesso segno. Si sa fare solo con i polinomi nella sola
 * x: con le altre lettere il certificato non è controllabile.
 *
 * Gli estremi sono numeri razionali, oppure c_0 + c_1 pi + c_2 e con i c_j
 * razionali. Allora si controlla un intervallo razionale [l, h] che contiene
 * [a, b], con 333/106 < pi < 355/113 (pi = 3.14159265..., 333/106 =
 * 3.14150943..., 355/113 = 3.14159292...) e 2.718 < e < 2.719 (e =
 * 2.71828182...): una condizione che vale su [l, h] vale anche su [a, b]. Se non vale e gli estremi sono razionali, la
 * funzione non è definita in un punto di [a, b], e il certificato è
 * respinto; con pi o e negli estremi, o per una condizione di regolarità di
 * una radice, non è controllabile.
 *
 * Il valore F(b) - F(a) si calcola con x legata al valore esatto di b e di
 * a, come l'indice di una somma (capitolo 8), e si confronta con V con la
 * prova dello zero di 13d.
 *
 * Prima, però, V e gli estremi devono essere numeri reali: un logaritmo con
 * l'argomento positivo, un divisore e l'argomento di un |.| diversi da zero,
 * una radice di indice pari con il radicando non negativo, una potenza con
 * un esponente che non è un intero con la base positiva (ck_real_number()).
 * L'algebra del verificatore non lo vede da sola: per lei ln(-e) e ln(e)
 * hanno la stessa scrittura, perché il logaritmo di un prodotto si spezza
 * nei fattori, e così il certificato di \int_1^e dx/x = ln(-e) veniva
 * accettato (lo ha trovato tools/fuzz/fuzz_calculus_certificates.py). Il
 * segno di ognuno di quegli argomenti, che è un numero, si legge
 * nell'intervallo razionale di ck_endpoint(): se è certo e sbagliato il
 * certificato è respinto, se non è certo non è controllabile. */

/* Mette in [*low, *high] un intervallo razionale che contiene il valore value
 * di un estremo, c_0 + c_1 pi + c_2 e; ritorna 1 se l'estremo è razionale (e
 * allora *low = *high è lui), 0 se no. Un estremo di un'altra forma rende il
 * certificato non controllabile. e è la lettera e^{1/Q} (13a) alla Q. */
static int ck_endpoint(CkRat value, CkNum *low, CkNum *high) {
    static const long long bounds[3][4] = {{1, 1, 1, 1}, {333, 106, 355, 113}, {2718, 1000, 2719, 1000}};
    CkNum c[3] = {ck_num_int(0), ck_num_int(0), ck_num_int(0)};
    value = ck_rat_plain(value);
    if (!ck_poly_is_constant(value.den)) { ck_unsupported("un estremo che il verificatore non sa confrontare"); return 0; }
    for (int i = 0; i < value.num.count; i++) {
        /* Quale dei tre: nessuna lettera, pi greco, o e. */
        const CkTerm *t = &value.num.term[i];
        int which = 0;
        for (int v = 0; v < ck_var_count; v++) {
            const CkVar *x = &ck_vars[v];
            if (!t->e[v]) continue;
            int euler = x->kind == CK_VAR_E && ck_poly_is_constant(x->base) && ck_poly_is_constant(x->below) && x->base.count && x->base.term[0].coef.num == 1 &&
                        x->base.term[0].coef.den == 1 && t->e[v] == x->below.term[0].coef.num;
            if (which || !(euler || (x->kind == CK_VAR_PI && t->e[v] == 1))) { ck_unsupported("un estremo che il verificatore non sa confrontare"); return 0; }
            which = euler ? 2 : 1;
        }
        c[which] = t->coef;
    }

    /* 333/106 < pi < 355/113 e 2718/1000 < e < 2719/1000: ognuno dei due
     * addendi c_j t sta tra c_j per i due estremi di t, nell'ordine che dà il
     * segno di c_j. */
    *low = *high = c[0];
    for (int j = 1; j < 3; j++) {
        CkNum first = ck_num_mul(c[j], ck_num_make(bounds[j][0], bounds[j][1])), second = ck_num_mul(c[j], ck_num_make(bounds[j][2], bounds[j][3]));
        int order = ck_num_cmp(first, second) <= 0;
        *low = ck_num_add(*low, order ? first : second);
        *high = ck_num_add(*high, order ? second : first);
    }
    return c[1].num == 0 && c[2].num == 0;
}

/* Controlla che il numero scritto da node (il valore V o un estremo, senza
 * la variabile) sia un numero reale: vedi la testa di 13e. Annota il rifiuto
 * se un argomento ha di sicuro il segno sbagliato, la non controllabilità se
 * il suo segno non si sa. */
static void ck_real_number(const CkNode *node) {
    if (!node || ck_status != CK_ACCEPTED) return;
    ck_real_number(node->a);
    ck_real_number(node->b);

    /* Che cosa chiede node del suo argomento: 1 positivo, 2 diverso da zero,
     * 3 non negativo. strict è 0 quando la richiesta è più severa del
     * necessario (una potenza il cui esponente potrebbe valere un intero
     * anche se non è scritto come tale): lì un no vuol dire "non so". */
    int need = 0, strict = 1;
    const CkNode *argument = NULL;
    switch (node->kind) {
    case CK_LN: need = 1; argument = node->a; break;
    case CK_LNABS: need = 2; argument = node->a; break;
    case CK_DIV: need = 2; argument = node->b; break;
    case CK_RADICAL:
        if (node->a->kind != CK_NUMBER || node->a->value % 2 == 0) need = 3;
        argument = node->b;
        break;
    case CK_POW:
        if (node->b->kind == CK_NUMBER) { if (node->b->value < 0) need = 2; }
        else { need = 1; strict = 0; }
        argument = node->a;
        break;
    default:
        break;
    }
    if (!need) return;
    CkNum low, high;
    ck_endpoint(ck_eval(argument), &low, &high);
    if (ck_status != CK_ACCEPTED) return;
    int positive = low.num > 0, negative = high.num < 0, zero = low.num == 0 && high.num == 0;
    int holds = need == 1 ? positive : need == 2 ? positive || negative : positive || zero;
    int fails = need == 1 ? negative || zero : need == 2 ? zero : negative;
    if (holds) return;
    if (fails && strict) ck_reject("il valore o un estremo dell'integrale non è un numero reale");
    else ck_unsupported("non so stabilire che il valore o un estremo dell'integrale è un numero reale");
}

/* Controlla che ogni condizione annotata valga su [low, high], come in
 * testa a 13e; exact è 1 se l'intervallo è proprio [a, b]. */
static void ck_check_interval(CkNum low, CkNum high, int exact) {
    for (int i = 0; i < ck_divisor_count + ck_condition_count && ck_status == CK_ACCEPTED; i++) {
        CkCondition c = i < ck_divisor_count ? (CkCondition){ck_divisors[i].value, 0, 1} : ck_conditions[i - ck_divisor_count];
        CkUni u;
        int plain = 1;
        for (int v = 1; v < ck_var_count; v++)
            if (ck_poly_uses(c.p, v)) plain = 0;
        if (!plain) { ck_unsupported("non so stabilire che la funzione è definita su tutto l'intervallo: una condizione di esistenza non è un polinomio in x"); return; }
        ck_uni_of(c.p, &u);
        if (ck_status != CK_ACCEPTED) return;
        int at_low = ck_num_sign(ck_uni_at(&u, low)), at_high = ck_num_sign(ck_uni_at(&u, high));
        int holds = at_low != 0 && at_high != 0 && (ck_num_cmp(low, high) == 0 || ck_roots(&u, &low, &high) == 0) && (!c.positive || at_low > 0);
        if (ck_status != CK_ACCEPTED || holds) continue;
        if (exact && c.certain) ck_reject("la funzione integranda o la primitiva non è definita in un punto dell'intervallo di integrazione");
        else ck_unsupported("non so stabilire che la funzione integranda e la primitiva sono definite su tutto l'intervallo");
    }
}

/* 13f. LA LETTURA E I CONTI
 *
 * Il certificato si calcola tre volte, come quelli di induzione (capitolo
 * 7) ma con una volta in più: la prima trova l'unità (13a); la seconda, da
 * capo con l'unità trovata, i riferimenti delle somme, se ci sono (n! in
 * una derivata n-esima); la terza fa i conti veri, e le prove. */

/* Un certificato del capitolo 13, come è stato letto. */
typedef struct {
    int kind;               /* 0 derivative, 1 antiderivative, 2 integral */
    char order_letter;      /* derivative: la lettera n dell'ordine, o 0 */
    long long order;        /* derivative: l'ordine k, o il caso base n0 */
    const CkNode *function; /* F: la funzione da derivare, o la primitiva */
    const CkNode *claim;    /* G: la derivata affermata, o la funzione integranda */
    const CkNode *lower, *upper, *value;  /* integral: a, b e V */
    const CkNode *brick[CK_MAX_BRICKS];
    int bricks;
} CkCalculus;

/* Ritorna la k-esima derivata di r, con le relazioni che non cambiano il
 * valore applicate a ogni passo (13d). */
static CkRat ck_derivative_times(CkRat r, long long k) {
    for (long long i = 0; i < k && ck_status == CK_ACCEPTED; i++) {
        r = ck_rat_derivative(r);
        r = (CkRat){ck_reduce(r.num, 0), ck_reduce(r.den, 0)};
    }
    return r;
}

/* Ritorna il valore di node con la variabile legata al numero value. */
static CkRat ck_eval_at(const CkNode *node, CkRat value) {
    ck_env[0] = (CkBinding){ck_dx, value};
    ck_env_count = 1;
    CkRat result = ck_eval(node);
    ck_env_count = 0;
    return result;
}

/* Le identità di un certificato: F^(k) - G = 0, oppure, per una derivata
 * n-esima, il caso base e il passo da n a n + 1 sulla formula, che è
 * un'identità in n e in x. Con final = 0 si calcola soltanto. Per un
 * integrale si annotano anche le condizioni di esistenza di F e di f, in
 * ogni valutazione: si controllano prima di calcolare F negli estremi, dove
 * altrimenti un polo darebbe un motivo meno chiaro. */
static void ck_calculus_identity(const CkCalculus *cert, int final) {
    if (cert->kind == 0 && cert->order_letter) {
        ck_numeric = 1;
        ck_point = cert->order;
        CkRat start = ck_derivative_times(ck_eval(cert->function), cert->order), formula = ck_eval(cert->claim);
        if (final) ck_calculus_zero(ck_rat_add(start, formula, 1), "nel caso base la derivata calcolata dal verificatore non è la formula");
        ck_numeric = 0;
        CkRat slope = ck_derivative_times(ck_eval(cert->claim), 1);
        ck_shift = 1;
        CkRat next = ck_eval(cert->claim);
        ck_shift = 0;
        if (final) ck_calculus_zero(ck_rat_add(slope, next, 1), "la derivata della formula per n non è la formula per n + 1");
        return;
    }
    ck_recording = cert->kind == 2;
    CkRat function = ck_eval(cert->function), claim = ck_eval(cert->claim);
    ck_recording = 0;
    CkRat derivative = ck_derivative_times(function, cert->kind == 0 ? cert->order : 1);
    if (final) ck_calculus_zero(ck_rat_add(derivative, claim, 1), cert->kind == 0 ? "la derivata calcolata dal verificatore non è quella del certificato"
                                                                                   : "la derivata della primitiva non è la funzione integranda");
}

/* Fa i conti del certificato cert; con final = 0 è una valutazione di prova,
 * di cui contano solo l'unità e i riferimenti.
 *
 * I valori assoluti, in una derivata o in una primitiva, come nel capitolo
 * 12: |u| vale u dove u > 0 e -u dove u < 0, e si fanno i conti per ogni
 * scelta dei segni (ck_abs_pattern). Fissata una scelta, nell'insieme aperto
 * dei punti in cui ogni argomento ha proprio quel segno le espressioni
 * coincidono con quelle senza valore assoluto, che si possono derivare: lì
 * l'identità vale se vale per quella scelta. Ogni punto in cui gli argomenti
 * dei valori assoluti di F non sono zero sta in uno di quegli insiemi (in
 * quelli di G può anche essere zero: lì u = -u, e le due scelte danno lo
 * stesso valore). Per questo F, per essere regolare, vuole anche gli
 * argomenti dei suoi valori assoluti diversi da zero. Si provano anche le
 * scelte che non capitano in nessun punto: si chiede di più, mai di meno; e
 * per questo, con dei valori assoluti, una differenza non nulla rende il
 * certificato non controllabile, non sbagliato (ck_calculus_zero()).
 * Un integrale con un valore assoluto non è controllabile: lì F e f devono
 * essere definite su tutto [a, b], anche dove un argomento vale zero. */
static void ck_calculus_pass(const CkCalculus *cert, int final) {
    ck_real = 1;
    ck_numeric = 0;
    ck_shift = 0;
    ck_env_count = 0;
    ck_derived = 0;
    ck_divisor_count = ck_condition_count = 0;

    /* I fattori suggeriti per i logaritmi (8g), senza radici (vedi
     * ck_real_root()). */
    ck_brick_count = 0;
    ck_hinting = 1;
    for (int i = 0; i < cert->bricks && ck_status == CK_ACCEPTED; i++) {
        CkRat brick = ck_eval(cert->brick[i]);
        CkNum lead;
        if (!ck_poly_is_constant(brick.den) || ck_poly_is_constant(brick.num)) { ck_reject("una voce brick non è un polinomio non costante"); return; }
        ck_bricks[ck_brick_count++] = ck_poly_monic(brick.num, &lead);
    }
    ck_hinting = 0;

    /* Le identità, per ogni scelta dei segni dei valori assoluti. */
    ck_abs_count = 0;
    if (cert->kind != 2) {
        ck_collect_abs(cert->function);
        ck_collect_abs(cert->claim);
    }
    for (ck_abs_pattern = 0; ck_abs_pattern < 1u << ck_abs_count && ck_status == CK_ACCEPTED; ck_abs_pattern++) {
        ck_derived = 0;
        ck_calculus_identity(cert, final);
    }
    ck_abs_pattern = 0;
    if (cert->kind != 2 || ck_status != CK_ACCEPTED) return;

    /* 13e. Gli estremi, le condizioni su [a, b], il valore. */
    CkRat lower = ck_eval(cert->lower), upper = ck_eval(cert->upper);
    CkNum low[2], high[2];
    int exact = ck_endpoint(lower, &low[0], &high[0]);
    exact = ck_endpoint(upper, &low[1], &high[1]) && exact;
    if (ck_status != CK_ACCEPTED) return;
    int order = ck_num_cmp(low[0], low[1]) <= 0;
    ck_check_interval(order ? low[0] : low[1], ck_num_cmp(high[0], high[1]) >= 0 ? high[0] : high[1], exact);
    if (ck_status != CK_ACCEPTED) return;
    /* Il valore e gli estremi devono essere numeri reali (testa di 13e):
     * prima del confronto, che non lo vede. */
    ck_real_number(cert->lower);
    ck_real_number(cert->upper);
    ck_real_number(cert->value);
    CkRat gap = ck_rat_add(ck_eval_at(cert->function, upper), ck_eval_at(cert->function, lower), 1);
    if (final && ck_status == CK_ACCEPTED) ck_calculus_zero(ck_rat_add(ck_eval(cert->value), gap, 1), "il valore dell'integrale non è F(b) - F(a)");
}

/* Legge la voce word, seguita da un'espressione, e ne ritorna l'albero. */
static const CkNode *ck_read_voice(const char *word) {
    ck_next_line();
    if (!ck_word(word)) { ck_reject("certificato scritto male: una voce manca o è fuori posto"); return NULL; }
    return ck_read();
}

/* Legge e controlla un certificato del capitolo 13 di tipo kind (0
 * derivative, 1 antiderivative, 2 integral), dalla riga dopo l'intestazione
 * fino a "end" compreso. */
static void ck_check_calculus(int kind) {
    CkCalculus cert;
    memset(&cert, 0, sizeof cert);
    cert.kind = kind;
    cert.order = 1;

    /* La variabile, e per una derivata l'ordine. */
    ck_next_line();
    int named = ck_word("variable");
    ck_skip();
    if (!named || !isalpha((unsigned char)*ck_cursor) || isalnum((unsigned char)ck_cursor[1])) { ck_reject("certificato scritto male: manca la variabile"); return; }
    ck_dx = *ck_cursor++;
    if (kind == 0) {
        ck_next_line();
        if (!ck_word("order")) { ck_reject("certificato scritto male: manca l'ordine"); return; }
        ck_skip();
        if (isalpha((unsigned char)*ck_cursor) && !isalnum((unsigned char)ck_cursor[1])) {
            cert.order_letter = *ck_cursor++;
            ck_skip();
            if (cert.order_letter == ck_dx || !ck_word("base") || !ck_read_integer(&cert.order)) { ck_reject("certificato scritto male: order n base n0"); return; }
        } else if (!ck_read_integer(&cert.order)) {
            ck_reject("certificato scritto male: l'ordine è un intero o una lettera");
            return;
        }
        if (cert.order < 0 || cert.order > CK_MAX_DERIVATIVE) { ck_unsupported("ordine di derivazione che il verificatore non tratta"); return; }
    }

    /* Le espressioni, nell'ordine. */
    if (kind == 2) {
        cert.lower = ck_read_voice("lower");
        cert.upper = ck_read_voice("upper");
    }
    if (kind == 0) {
        cert.function = ck_read_voice("function");
        cert.claim = ck_read_voice("derivative");
    } else {
        cert.claim = ck_read_voice("integrand");
        cert.function = ck_read_voice("primitive");
    }
    if (kind == 2) cert.value = ck_read_voice("value");
    for (ck_next_line(); ck_status == CK_ACCEPTED && ck_word("brick"); ck_next_line()) {
        if (cert.bricks == CK_MAX_BRICKS) { ck_unsupported("troppe voci brick"); return; }
        cert.brick[cert.bricks++] = ck_read();
    }
    if (ck_status != CK_ACCEPTED) return;
    if (!ck_word("end")) { ck_reject("certificato scritto male: voce sconosciuta o \"end\" mancante"); return; }

    /* La funzione da derivare non dipende dall'ordine; estremi e valore non
     * dipendono dalla variabile. */
    if (cert.order_letter && ck_mentions(cert.function, cert.order_letter)) { ck_reject("la funzione da derivare contiene la lettera dell'ordine"); return; }
    if (kind == 2 && (ck_mentions(cert.lower, ck_dx) || ck_mentions(cert.upper, ck_dx) || ck_mentions(cert.value, ck_dx))) {
        ck_reject("gli estremi o il valore di un integrale contengono la variabile");
        return;
    }

    /* Le tre valutazioni (in testa a 13f). La prima e la seconda partono da
     * capo, la terza usa le lettere della seconda: la x è sempre la 0. Le
     * unità della prima restano: quella delle radici si riferisce agli
     * alberi letti, che stanno prima di mark. */
    size_t mark = ck_used;
    ck_unit = 1;
    ck_root_unit_count = 0;
    for (int pass = 0; pass < 3 && ck_status == CK_ACCEPTED; pass++) {
        ck_trial = pass < 2;
        ck_learning = pass == 0;
        if (pass < 2) {
            ck_used = mark;
            ck_var_count = 0;
            ck_sign_var = -1;
            ck_var_letter(ck_dx);
        }
        ck_variable = cert.order_letter;
        ck_calculus_pass(&cert, pass == 2);
    }
    ck_real = ck_hinting = 0;

    /* Come nel capitolo 8: una somma o un prodotto fino a a n + r sta per una
     * quantità vera solo se esiste dal caso base in su. */
    for (int v = 0; v < ck_var_count && cert.order_letter && ck_status == CK_ACCEPTED; v++)
        if (ck_vars[v].kind == CK_VAR_BIG && ck_vars[v].dilation * cert.order + ck_vars[v].offset < ck_vars[v].lower - 1)
            ck_unsupported("una somma o un prodotto comincia dopo il suo ultimo termine, nel caso base");
}

/* =============================================================================
 * 14. LIMITI
 * ============================================================================
 *
 * Gli enunciati del capitolo 29 di mathresearcher dicono quanto vale il
 * limite di una funzione di una variabile reale in un punto, da un lato o
 * dai due, oppure a +infinito o a -infinito. Il motore li decide con gli
 * sviluppi in serie troncati, a coefficienti esatti, e il certificato è
 * l'enunciato con la sostituzione e l'ordine di ogni lato:
 *
 *     certificate limit
 *     variable x
 *     point 0
 *     function (/ (- 1 (cos x)) (^ x 2))
 *     claim value (/ 1 2) true
 *     side right a+t 4
 *     side left a-t 4
 *     end
 *
 * Le voci, in quest'ordine:
 *
 *   variable  la variabile, una lettera;
 *   point     il punto, un'espressione costante, seguita da right o left per
 *             un limite da un lato solo; oppure +infinity o -infinity;
 *   function  la funzione: interi, la variabile, + - * / neg ^, e le parole
 *             pi, exp, ln, lnabs, abs, sin, cos, tan, cot, atan, arcsin,
 *             arccos e (root m a) del capitolo 6;
 *   claim     ciò che si afferma, poi true o false: value V, il limite è il
 *             numero V (un'espressione costante); plus, è +infinito; minus,
 *             è -infinito; unsigned, |f| tende a +infinito; none, il limite
 *             non esiste;
 *   side      un lato, la sostituzione e l'ordine. Un punto senza lato vuole
 *             le righe right e left, in quest'ordine; un punto con il lato,
 *             quella sola; un infinito, una riga infinity. Le sostituzioni:
 *             a+t da destra del punto a, a-t da sinistra, 1/t a +infinito,
 *             -1/t a -infinito; e^(-1/t) e -e^(-1/t) da destra e da sinistra
 *             di 0, e^(1/t) e -e^(1/t) a +infinito e a -infinito (14h).
 *             L'ordine N, da 1 a CK_MAX_GOAL, dice dove si fermano gli
 *             sviluppi (14d).
 *
 * IL TEOREMA. Sia f la funzione scritta nel certificato, con il suo dominio
 * naturale D: gli x in cui ogni operazione del suo calcolo ha senso. Ogni
 * divisore è diverso da zero (una potenza con esponente intero negativo è
 * una divisione; x^0 vale 1); ogni argomento di ln è positivo, di lnabs
 * diverso da zero; la base di una potenza con esponente non intero, o non
 * costante, è positiva (o nulla, con un esponente razionale positivo); il
 * radicando di una radice di indice pari non è negativo; l'argomento di
 * arcsin e di arccos sta tra -1 e 1; quello di tan non annulla il coseno,
 * quello di cot il seno. Per ogni lato del certificato sia I_d l'intorno
 * bucato di ampiezza d da quel lato: (a, a + d), (a - d, a), (1/d,
 * +infinito) o (-infinito, -1/d). Il verificatore stabilisce una di due cose:
 *
 *   - per un d > 0, I_d non ha punti di D: da quel lato f non ha valori;
 *   - per un d > 0, I_d sta tutto in D, e da quel lato f ha limite: un
 *     numero, +infinito o -infinito.
 *
 * Il limite di f nel punto è quello di f ristretta a D: se un lato non ha
 * valori conta solo l'altro; se non ne ha nessuno dei due il limite non ha
 * senso, e il certificato è respinto. Se il certificato è accettato, ciò
 * che dice claim, con la verità dichiarata, è vero:
 *
 *   - value V: true, il limite esiste ed è V; false, non è V (è un altro, o
 *     non esiste);
 *   - plus, minus: true, il limite è +infinito, -infinito; false, non lo è;
 *   - unsigned: true, ogni lato con valori ha limite infinito, quindi |f|
 *     tende a +infinito; false, un lato ha un limite finito;
 *   - none: true, i due lati hanno valori e limiti diversi, quindi il limite
 *     non esiste; false, il limite esiste.
 *
 * Il limite di una successione a_n è quello della funzione con n reale,
 * quando questa ha limite (teorema ponte): lo usa chi scrive il
 * certificato, non il verificatore.
 *
 * COME LO SI CONTROLLA. La sostituzione porta la variabile a una t > 0 che
 * tende a 0 (14h). t -> a + t, a - t, 1/t, -1/t sono biiezioni monotone da
 * un intervallo (0, d') all'intorno I_d, e x tende al punto da quel lato se
 * e solo se t tende a 0 da destra: il limite di f da quel lato è il limite
 * di f(x(t)) per t -> 0^{+}. La funzione si sviluppa in potenze di t, anche
 * frazionarie (le radici), con un resto O(t^{E}) (14c), un'operazione alla
 * volta; i coefficienti sono numeri esatti (14a), che possono contenere la
 * lettera L = ln(1/t). Ogni operazione con una condizione la controlla sul
 * termine principale del suo argomento, c t^{e} (1 + o(1)): con c diverso
 * da zero l'argomento non si annulla per t piccolo e ha il segno di c. È
 * così che si stabilisce che I_d sta in D, o che non ne ha punti (un
 * radicando con c < 0). Il limite si legge dal termine principale di f
 * (14i), e da quello di ogni lato la verità di claim (14j).
 *
 * Non c'è nessuna ricerca: l'ordine del certificato fissa ogni sviluppo, e i
 * conti sono somme, prodotti e quozienti di numeri esatti. Ciò che si cita
 * senza dimostrarlo:
 *
 *   - il teorema di Taylor con il resto di Peano: per g una tra exp, sin,
 *     cos, ln(1 + w), arctan, (1 + w)^q e per K >= 1, g(w) = a_0 + a_1 w +
 *     ... + a_{K-1} w^{K-1} + O(w^{K}) per w -> 0, con gli a_k della serie
 *     di Taylor di g in 0 (14d);
 *   - che L = ln(1/t) cresce meno di ogni potenza di 1/t: t^{eps} L^{m}
 *     tende a 0 per ogni eps > 0 e ogni m;
 *   - le identità delle funzioni elementari: e^{u + v} = e^{u} e^{v},
 *     ln(u v) = ln u + ln v e (u v)^q = u^q v^q per u, v > 0; le formule di
 *     addizione di seno e coseno, quella di sottrazione dell'arcotangente e
 *     quella che scrive l'arcoseno con l'arcotangente; i valori di seno,
 *     coseno e arcotangente negli angoli multipli di 30 e di 45 gradi (14e);
 *   - la scala delle crescite: un'esponenziale e^{G}, con G che tende a
 *     +infinito come una potenza negativa di t, cresce più di ogni potenza di
 *     1/t e di L (14g).
 *
 * I paragrafi:
 *
 *   14a. Le costanti: le lettere positive e la loro regola.
 *   14b. Il segno di una costante e il termine principale di un coefficiente.
 *   14c. Le serie troncate: somma, prodotto, il resto.
 *   14d. Le serie di Taylor, l'inverso, le potenze.
 *   14e. Le funzioni delle costanti.
 *   14f. Le funzioni vicino al punto.
 *   14g. Il fattore e^{G}: la scala delle crescite.
 *   14h. Il valore di un'espressione.
 *   14i. Il limite da un lato.
 *   14j. La lettura del certificato e la verità di ciò che afferma.
 */

/* 14a. LE COSTANTI
 *
 * I coefficienti degli sviluppi sono frazioni di polinomi (capitolo 5), le
 * cui indeterminate sono numeri positivi: le lettere CONST di 7h, b^{1/m},
 * con b un primo p (e allora m >= 2: p stesso è un razionale), e, pi, il
 * logaritmo ln p di un primo, oppure la lettera L = ln(1/t). Così \sqrt{2},
 * e^{-7/2} = (e^{1/2}) / e^{4}, \pi / (4 \ln 2) sono frazioni di polinomi.
 *
 * Un coefficiente è zero se il suo numeratore è il polinomio nullo, dopo la
 * sola regola (b^{1/m})^{m} = b, applicata a ogni conto (ck_const_reduce()).
 * È una conclusione vera, perché ogni passo è un'uguaglianza vera; il
 * contrario non vale: \sqrt{2} \sqrt{3} - \sqrt{6} non risulta zero, ed
 * e^{1/2} e^{1/3} non è e^{5/6}, perché le lettere sono diverse. Non è un
 * errore: un numero così il verificatore saprà dirlo diverso da zero solo
 * se le regole del segno (14b) lo provano, e uno zero non lo provano mai;
 * altrimenti il certificato non è controllabile. */

/* Il polinomio 1, comune a tutti i coefficienti del certificato in corso:
 * i polinomi non si modificano mai, quindi si possono condividere. */
static CkPoly ck_one;

/* Ritorna l'indeterminata CONST b^{1/root} (7h), creandola se manca. */
static int ck_const_var(char base, long long prime, int root) {
    for (int i = 0; i < ck_var_count; i++)
        if (ck_vars[i].kind == CK_VAR_CONST && ck_vars[i].name == base && ck_vars[i].prime == prime && ck_vars[i].position == root) return i;
    return ck_var_new((CkVar){.kind = CK_VAR_CONST, .name = base, .prime = prime, .position = root});
}

/* Ritorna 1 se il polinomio p contiene la lettera L, con qualunque indice. */
static int ck_poly_has_ell(CkPoly p) {
    for (int i = 0; i < p.count; i++)
        for (int v = 0; v < ck_var_count; v++)
            if (p.term[i].e[v] && ck_vars[v].kind == CK_VAR_CONST && ck_vars[v].name == 'L') return 1;
    return 0;
}

/* Ritorna 1 se il coefficiente c contiene la lettera L. */
static int ck_c_has_ell(CkRat c) { return ck_poly_has_ell(c.num) || ck_poly_has_ell(c.den); }

/* Ritorna p con la regola (b^{1/m})^{m} = b applicata finché si può: un
 * esponente k >= m diventa k - m, e b entra nel coefficiente se è un primo,
 * altrimenti come la lettera b^{1/1}. Le lettere create qui hanno indice 1,
 * a cui la regola non si applica: un passo basta. */
static CkPoly ck_const_reduce(CkPoly p) {
    int any = 0;
    for (int i = 0; i < p.count && !any; i++)
        for (int v = 0; v < ck_var_count; v++)
            if (ck_vars[v].kind == CK_VAR_CONST && ck_vars[v].position > 1 && p.term[i].e[v] >= ck_vars[v].position) any = 1;
    if (!any) return p;
    CkTerm *t = ck_alloc(sizeof *t * (size_t)(p.count + 1));
    if (ck_status != CK_ACCEPTED) return (CkPoly){0, t};
    memcpy(t, p.term, sizeof *t * (size_t)p.count);
    for (int i = 0; i < p.count; i++)
        for (int v = 0; v < ck_var_count && ck_status == CK_ACCEPTED; v++) {
            int m = ck_vars[v].position, k = t[i].e[v] / (m > 0 ? m : 1);
            if (ck_vars[v].kind != CK_VAR_CONST || m < 2 || k == 0) continue;
            t[i].e[v] = (signed char)(t[i].e[v] - k * m);
            if (ck_vars[v].name == 'p') {
                for (int j = 0; j < k; j++) t[i].coef = ck_num_mul(t[i].coef, ck_num_int(ck_vars[v].prime));
                continue;
            }
            int whole = ck_const_var(ck_vars[v].name, ck_vars[v].prime, 1);
            if (t[i].e[whole] + k > CK_MAX_EXPONENT) { ck_unsupported("esponenti troppo grandi per il verificatore"); break; }
            t[i].e[whole] = (signed char)(t[i].e[whole] + k);
        }
    return ck_poly_build(t, p.count);
}

/* Ritorna il razionale n come coefficiente. */
static CkRat ck_c_num(CkNum n) { return (CkRat){ck_poly_number(n), ck_one}; }

/* Ritorna l'indeterminata v come coefficiente. */
static CkRat ck_c_var(int v) { return (CkRat){ck_poly_variable(v), ck_one}; }

/* Se il coefficiente c è un razionale lo mette in *n e ritorna 1. */
static int ck_c_rational(CkRat c, CkNum *n) {
    if (!ck_poly_is_constant(c.num) || !ck_poly_is_constant(c.den) || c.den.count == 0) return 0;
    *n = c.num.count ? ck_num_mul(c.num.term[0].coef, ck_num_inv(c.den.term[0].coef)) : ck_num_int(0);
    return 1;
}

/* Ritorna c in forma più piccola, con lo stesso valore: la regola di 14a; un
 * denominatore costante passa nel numeratore; un altro si rende monico, e
 * se divide il numeratore la frazione diventa un polinomio. */
static CkRat ck_c_norm(CkRat c) {
    CkNum lead;
    CkPoly quotient;
    c.num = ck_const_reduce(c.num);
    c.den = ck_const_reduce(c.den);
    if (ck_status != CK_ACCEPTED || c.den.count == 0) return (CkRat){ck_poly_number(ck_num_int(0)), ck_one};
    if (ck_poly_is_constant(c.den)) return (CkRat){ck_poly_mul(c.num, ck_poly_number(ck_num_inv(c.den.term[0].coef))), ck_one};
    c.den = ck_poly_monic(c.den, &lead);
    c.num = ck_poly_mul(c.num, ck_poly_number(ck_num_inv(lead)));
    if (c.num.count == 0 || ck_poly_divides(c.den, c.num, &quotient)) return (CkRat){c.num.count ? quotient : c.num, ck_one};
    return c;
}

/* a + b, oppure a - b se minus è diverso da zero. I razionali, che sono la
 * gran parte, si sommano senza polinomi. */
static CkRat ck_c_add(CkRat a, CkRat b, int minus) {
    CkNum x, y;
    if (ck_c_rational(a, &x) && ck_c_rational(b, &y)) return ck_c_num(ck_num_add(x, minus ? ck_num_neg(y) : y));
    return ck_c_norm(ck_rat_add(a, b, minus));
}

/* a b. */
static CkRat ck_c_mul(CkRat a, CkRat b) {
    CkNum x, y;
    if (ck_c_rational(a, &x) && ck_c_rational(b, &y)) return ck_c_num(ck_num_mul(x, y));
    return ck_c_norm(ck_rat_mul(a, b));
}

/* 1 / a, con a diverso da zero. */
static CkRat ck_c_inv(CkRat a) { return ck_c_norm((CkRat){a.den, a.num}); }

/* -a. */
static CkRat ck_c_neg(CkRat a) { return ck_c_mul(a, ck_c_num(ck_num_int(-1))); }

/* 14b. IL SEGNO DI UNA COSTANTE
 *
 * Le lettere CONST sono tutte numeri positivi (L lo è per t < 1). Il segno di
 * un polinomio in queste lettere si stabilisce con tre regole, con conti
 * esatti tra razionali e senza nessun numero approssimato:
 *
 *   14b-1. se tutti i coefficienti hanno lo stesso segno, è quello: una
 *          somma di numeri dello stesso segno ha quel segno;
 *   14b-2. tolto il monomio che divide tutti i termini, che è positivo, resta
 *          una combinazione q_1 ln p_1 + ... + q_k ln p_k di logaritmi di
 *          primi diversi, con i q_i razionali non nulli. Con N il minimo
 *          comune multiplo dei loro denominatori e a_i = N q_i, che sono
 *          interi, ha il segno di ln(p_1^{a_1} ... p_k^{a_k}), cioè di P - Q,
 *          con P il prodotto dei p_i^{a_i} con a_i > 0 e Q quello dei
 *          p_i^{-a_i} con a_i < 0. P e Q sono interi diversi, per l'unicità
 *          della scomposizione in primi: il segno è un confronto tra due
 *          interi. Così ln 2 - ln 3 < 0;
 *   14b-3. tolto il monomio comune, restano le lettere di una base sola b,
 *          tra e, pi (senza radici) e un primo p: b^{1/m_1}, b^{1/m_2}, ...
 *          Con M il minimo comune multiplo degli m_i e Y = b^{1/M}, ognuna è
 *          una potenza di Y, b^{1/m_i} = Y^{M/m_i}, e il polinomio diventa un
 *          polinomio P(Y) in una variabile, a coefficienti razionali. Due
 *          razionali low < Y < high li dà un conto finito (ck_root_bounds());
 *          se P non si annulla in low né in high e non ha radici tra i due
 *          (teorema di Sturm, 12a), P ha lo stesso segno in tutto [low,
 *          high], e in Y ha quello di P(low). Così e^2 - e^3 < 0, e
 *          \sqrt{2}/2 - 1 < 0.
 *
 * Il resto, come \pi - e o 1 - \ln 2, ha un segno che le regole non danno:
 * il certificato che ne ha bisogno non è controllabile. */

/* Ritorna p^k in *out, se non esce dai numeri del verificatore: 0 se esce,
 * senza annotare nulla. */
static int ck_power_fits(long long p, __int128 k, __int128 *out) {
    *out = 1;
    for (__int128 i = 0; i < k; i++)
        if (__builtin_mul_overflow(*out, (__int128)p, out) || *out > CK_NUM_LIMIT) return 0;
    return 1;
}

/* 14b-2. Ritorna il segno di p, tolto il monomio low comune a tutti i suoi
 * termini, se ciò che resta è una combinazione di logaritmi di primi; 2
 * altrimenti. */
static int ck_log_sign(CkPoly p, const signed char *low) {
    /* Ogni termine deve essere q ln p: N è il minimo comune multiplo dei
     * denominatori, top e bottom i prodotti P e Q. */
    __int128 n = 1, top = 1, bottom = 1;
    for (int i = 0; i < p.count; i++) {
        int letter = -1, extra = 0;
        for (int v = 0; v < ck_var_count; v++) {
            int e = p.term[i].e[v] - low[v];
            if (e == 1 && letter < 0) letter = v;
            else if (e) extra = 1;
        }
        if (letter < 0 || extra || ck_vars[letter].kind != CK_VAR_CONST || ck_vars[letter].name != 'l' || ck_vars[letter].position != 1) return 2;
        __int128 g = ck_gcd(n, p.term[i].coef.den);
        if (__builtin_mul_overflow(n / g, p.term[i].coef.den, &n) || n > CK_NUM_LIMIT) return 2;
    }
    for (int i = 0; i < p.count; i++) {
        int letter = 0;
        __int128 a, power;
        for (int v = 0; v < ck_var_count; v++)
            if (p.term[i].e[v] - low[v] == 1) letter = v;
        if (__builtin_mul_overflow(p.term[i].coef.num, n / p.term[i].coef.den, &a)) return 2;
        if (!ck_power_fits(ck_vars[letter].prime, a > 0 ? a : -a, &power)) return 2;
        if (__builtin_mul_overflow(a > 0 ? top : bottom, power, a > 0 ? &top : &bottom) || top > CK_NUM_LIMIT || bottom > CK_NUM_LIMIT) return 2;
    }
    return top > bottom ? 1 : top < bottom ? -1 : 2;
}

/* Le somme parziali dell'arcotangente di x, con 0 < x < 1: la serie
 * x - x^3/3 + x^5/5 - ... ha i termini di segno alterno e in valore
 * assoluto decrescenti, quindi una somma parziale che finisce con un termine
 * positivo sta sopra la somma, una che finisce con un termine negativo sotto
 * (criterio di Leibniz). Mette in *high la somma dei primi 2 terms - 1
 * termini, che finisce con un termine positivo, e in *low quella dei primi
 * 2 terms. */
static void ck_arctan_bounds(CkNum x, int terms, CkNum *low, CkNum *high) {
    CkNum power = x, sum = ck_num_int(0);
    for (int k = 0; k < 2 * terms; k++) {
        if (k == 2 * terms - 1) *high = sum;
        sum = ck_num_add(sum, ck_num_mul(ck_num_make(k % 2 ? -1 : 1, 2 * k + 1), power));
        power = ck_num_mul(power, ck_num_mul(x, x));
    }
    *low = sum;
}

/* 14b-3. Mette in *low e *high due razionali con low < b^{1/m} < high, per
 * b il numero di Nepero ('e'), pi ('q', con m = 1) o il primo prime ('p'), e
 * ritorna 1; ritorna 0 se non li sa dare. I conti possono uscire dai numeri
 * del verificatore: chi chiama se ne accorge da ck_status.
 *
 *   - e^{x} con x = 1/m <= 1: la somma dei x^k/k! per k da 0 a n = 12 è
 *     N/D, con D = n! m^{n} e N = somma di m^{n-k} n!/k!, interi; il resto,
 *     la somma dei termini da k = n + 1, è positivo e al più x^{n+1}/(n+1)!
 *     (1 + x/(n+2) + (x/(n+2))^2 + ...) <= 2 x^{n+1}/(n+1)! = 2 / ((n + 1)
 *     m D) <= 1/D. Quindi N/D < e^{x} < (N + 1)/D;
 *   - pi = 16 arctan(1/5) - 4 arctan(1/239) (formula di Machin), con le
 *     somme parziali di ck_arctan_bounds();
 *   - p^{1/m}: il numero positivo il cui m-esima potenza è p, tra 1 e p;
 *     si dimezza l'intervallo confrontando mid^m con p, in modo esatto. */
static int ck_root_bounds(char base, long long prime, int m, CkNum *low, CkNum *high) {
    if (base == 'e') {
        /* N e D, con m <= CK_MAX_DEGREE: D = 12! 48^{12} sta sotto 10^{29}.
         * Il termine k di N è m^{n-k} n!/k!: lo si costruisce da k = n in
         * giù, moltiplicando per k m. */
        __int128 n = 0, term = 1, den = 1;
        for (int k = 12; k >= 0; k--) {
            n += term;
            term *= (__int128)k * m;
        }
        for (int k = 1; k <= 12; k++) den *= (__int128)k * m;
        *low = ck_num_make(n, den);
        *high = ck_num_make(n + 1, den);
        return 1;
    }
    if (base == 'q' && m == 1) {
        CkNum a_low, a_high, b_low, b_high;
        ck_arctan_bounds(ck_num_make(1, 5), 3, &a_low, &a_high);
        ck_arctan_bounds(ck_num_make(1, 239), 1, &b_low, &b_high);
        *low = ck_num_add(ck_num_mul(ck_num_int(16), a_low), ck_num_mul(ck_num_int(-4), b_high));
        *high = ck_num_add(ck_num_mul(ck_num_int(16), a_high), ck_num_mul(ck_num_int(-4), b_low));
        return 1;
    }
    if (base != 'p') return 0;
    *low = ck_num_int(1);
    *high = ck_num_int(prime);
    for (int step = 0; step < 48 && ck_status == CK_ACCEPTED; step++) {
        CkNum middle = ck_num_mul(ck_num_add(*low, *high), ck_num_make(1, 2)), power = ck_num_int(1);
        for (int i = 0; i < m; i++)
            if (!ck_num_mul_fits(power, middle, &power)) return 1;
        int side = ck_num_cmp(power, ck_num_int(prime));
        if (side == 0) return 0;
        *(side < 0 ? low : high) = middle;
    }
    return 1;
}

/* Ritorna il multiplo di 2^{-bits} più vicino a x da sotto (up = 0) o da
 * sopra (up = 1): la divisione per il denominatore, una cifra binaria alla
 * volta. Il resto sta sotto il denominatore, quindi il suo doppio sta nei
 * 128 bit. */
static CkNum ck_num_round(CkNum x, int bits, int up) {
    __int128 whole = x.num / x.den - (x.num % x.den < 0), rest = x.num - whole * x.den, scaled = whole;
    for (int i = 0; i < bits; i++) {
        rest *= 2;
        scaled = scaled * 2 + (rest >= x.den);
        if (rest >= x.den) rest -= x.den;
    }
    return ck_num_make(scaled + (up && rest), (__int128)1 << bits);
}

/* 14b-3. Ritorna il segno di p, tolto il monomio low comune a tutti i suoi
 * termini, se ciò che resta è un polinomio nelle radici di una base sola; 2
 * altrimenti. Gli estremi low e high si arrotondano verso l'esterno a
 * multipli di 2^{-bits}, con bits tanto più piccolo quanto più alto è il
 * grado di P, perché P(low) e i polinomi della catena di Sturm stiano nei
 * numeri del verificatore. Un conto che ne esce vuol dire solo "non si sa":
 * lo stato del controllo torna com'era. */
static int ck_root_sign(CkPoly p, const signed char *low) {
    char base = 0;
    long long prime = 0;
    int m = 1, sign = 2;
    CkUni u;
    CkNum below, above;

    /* La base, e il minimo comune multiplo M degli indici. */
    for (int i = 0; i < p.count; i++)
        for (int v = 0; v < ck_var_count; v++) {
            if (p.term[i].e[v] == low[v]) continue;
            const CkVar *x = &ck_vars[v];
            if (x->kind != CK_VAR_CONST || (x->name != 'e' && x->name != 'q' && x->name != 'p') || (base && (x->name != base || x->prime != prime))) return 2;
            base = x->name;
            prime = x->prime;
            m = (int)(m / ck_gcd(m, x->position) * x->position);
            if (m > CK_MAX_DEGREE) return 2;
        }
    if (!base) return 2;

    /* P(Y), con b^{1/m_i} = Y^{M/m_i}. */
    u.degree = -1;
    for (int i = 0; i < p.count; i++) {
        int d = 0;
        for (int v = 0; v < ck_var_count; v++) d += (p.term[i].e[v] - low[v]) * (m / (ck_vars[v].position ? ck_vars[v].position : 1));
        if (d > CK_MAX_DEGREE) return 2;
        while (u.degree < d) u.c[++u.degree] = ck_num_int(0);
        u.c[d] = ck_num_add(u.c[d], p.term[i].coef);
    }
    ck_uni_trim(&u);

    /* low < Y < high, e P senza radici in [low, high]. */
    CkAnswer saved = ck_status;
    const char *reason = ck_reason;
    if (u.degree >= 0 && ck_root_bounds(base, prime, m, &below, &above) && ck_status == CK_ACCEPTED) {
        int bits = 110 / (u.degree > 0 ? u.degree : 1);
        for (__int128 top = above.num / above.den + 1; top > 0; top /= 2) bits--;
        if (bits > 64) bits = 64;
        if (bits < 2) return 2;
        below = ck_num_round(below, bits, 0);
        above = ck_num_round(above, bits, 1);
        int at_low = ck_num_sign(ck_uni_at(&u, below)), at_high = ck_num_sign(ck_uni_at(&u, above));
        if (at_low && at_high && ck_roots(&u, &below, &above) == 0 && ck_status == CK_ACCEPTED) sign = at_low;
    }
    if (ck_status != saved) { ck_status = saved; ck_reason = reason; sign = 2; }
    return sign;
}

/* Ritorna il segno del polinomio p nelle lettere CONST: 0 per il polinomio
 * nullo, -1 o 1 se 14b-1, 14b-2 o 14b-3 lo stabiliscono, 2 se non si sa. */
static int ck_poly_sign(CkPoly p) {
    int first, same = 1;
    if (p.count == 0) return 0;
    first = ck_num_sign(p.term[0].coef);
    for (int i = 1; i < p.count; i++)
        if (ck_num_sign(p.term[i].coef) != first) same = 0;
    if (same) return first;

    /* 14b-2. Il monomio comune: l'esponente più basso di ogni lettera. */
    signed char low[CK_MAX_VARS];
    for (int v = 0; v < CK_MAX_VARS; v++) {
        low[v] = p.term[0].e[v];
        for (int i = 1; i < p.count; i++)
            if (p.term[i].e[v] < low[v]) low[v] = p.term[i].e[v];
    }

    int sign = ck_log_sign(p, low);
    return sign != 2 ? sign : ck_root_sign(p, low);
}

/* IL TERMINE PRINCIPALE DI UN COEFFICIENTE
 *
 * Un coefficiente c(L) = P(L) / Q(L) contiene L con esponenti razionali: la
 * lettera L^{1/m} vale 1/m. Si raccolgono i termini di P con la potenza più
 * alta di L, d_P: tolta L, dove hanno tutti lo stesso valore L^{d_P}, danno
 * un polinomio k_P nelle altre costanti. Se k_P è zero quei termini insieme
 * valgono zero, e si passa alla potenza successiva. Allora, per L grande,
 *
 *     c(L) = (k_P L^{d_P} + potenze più basse) / (k_Q L^{d_Q} + ...)
 *          = (k_P / k_Q) L^{d_P - d_Q} (1 + o(1)),
 *
 * perché L^{d - d_P} -> 0 per ogni d < d_P: c(L) ha il segno di k_P / k_Q,
 * non si annulla per L abbastanza grande, cioè per t abbastanza piccolo, e
 * va come L^{d_P - d_Q}. Senza L, c è una costante e la potenza è 0. */

/* Ciò che il termine principale dice del coefficiente c(L). */
typedef struct {
    int known;     /* 1 se si sa se c è zero e, se non lo è, il suo segno */
    int zero;      /* 1 se c vale zero */
    int side;      /* il segno di c(L) per L grande: +1 o -1 */
    CkNum degree;  /* d_P - d_Q */
    CkRat value;   /* k_P / k_Q, senza L */
} CkLead;

/* Ritorna la potenza di L del termine t: la somma degli esponenti delle
 * lettere L^{1/m}, divisi per m. */
static CkNum ck_ell_degree(const CkTerm *t) {
    CkNum degree = ck_num_int(0);
    for (int v = 0; v < ck_var_count; v++)
        if (t->e[v] && ck_vars[v].kind == CK_VAR_CONST && ck_vars[v].name == 'L')
            degree = ck_num_add(degree, ck_num_make(t->e[v], ck_vars[v].position));
    return degree;
}

/* Mette in *degree la potenza più alta di L in p con k_P non nullo, e in
 * *top quel k_P. Ritorna il segno di k_P, 0 se p vale zero, 2 se il segno
 * non si sa. */
static int ck_poly_top(CkPoly p, CkNum *degree, CkPoly *top) {
    while (p.count > 0 && ck_status == CK_ACCEPTED) {
        CkTerm *high = ck_alloc(sizeof *high * (size_t)p.count), *low = ck_alloc(sizeof *low * (size_t)p.count);
        int highs = 0, lows = 0;
        if (ck_status != CK_ACCEPTED) return 2;
        *degree = ck_ell_degree(&p.term[0]);
        for (int i = 1; i < p.count; i++)
            if (ck_num_cmp(ck_ell_degree(&p.term[i]), *degree) > 0) *degree = ck_ell_degree(&p.term[i]);
        for (int i = 0; i < p.count; i++) {
            if (ck_num_cmp(ck_ell_degree(&p.term[i]), *degree) != 0) { low[lows++] = p.term[i]; continue; }
            high[highs] = p.term[i];
            for (int v = 0; v < ck_var_count; v++)
                if (ck_vars[v].kind == CK_VAR_CONST && ck_vars[v].name == 'L') high[highs].e[v] = 0;
            highs++;
        }
        *top = ck_poly_build(high, highs);
        if (top->count) return ck_poly_sign(*top);
        p = ck_poly_build(low, lows);
    }
    return 0;
}

/* Il termine principale del coefficiente c. */
static CkLead ck_lead(CkRat c) {
    CkLead out = {0, 0, 0, {0, 1}, {{0, NULL}, {0, NULL}}};
    CkNum above, below;
    CkPoly top, bottom;
    int up = ck_poly_top(c.num, &above, &top), down;
    if (up == 0) { out.known = out.zero = 1; return out; }
    down = ck_poly_top(c.den, &below, &bottom);
    if (up == 2 || down == 2 || down == 0) return out;
    out.known = 1;
    out.side = up * down;
    out.degree = ck_num_add(above, ck_num_neg(below));
    out.value = ck_c_norm((CkRat){top, bottom});
    return out;
}

/* Ritorna 1 se i coefficienti a e b sono uguali, 0 se sono certamente
 * diversi (14b), -1 se non si sa. */
static int ck_c_equal(CkRat a, CkRat b) {
    CkLead lead = ck_lead(ck_c_add(a, b, 1));
    return !lead.known ? -1 : lead.zero;
}

/* 14c. LE SERIE TRONCATE
 *
 * Una serie in t, con t > 0 piccolo, è una somma finita di termini
 * c_i t^{(low + i)/D} più un resto r(t), con |r(t)| <= C t^{E/D} L^{m} per
 * t abbastanza piccolo, dove C e m sono numeri che non servono, m = 0 se la
 * serie non ha la voce logs. Una serie esatta non ha resto. E = CK_FLAT vuol
 * dire che il resto è più piccolo di ogni potenza di t (14g). Che una serie
 * descriva una funzione g, vicino a 0, è ciò che ogni operazione conserva:
 *
 *   14c-1. somma: i resti si sommano, e vince il più grande, quello con E
 *          più basso;
 *   14c-2. prodotto: (A + r_A)(B + r_B) = A B + A r_B + B r_A + r_A r_B, e
 *          |A| <= C t^{v_A}, con v_A l'esponente più basso di A (o E_A se A
 *          non ha termini): il resto del prodotto ha l'esponente più basso tra
 *          v_A + E_B e v_B + E_A. Un coefficiente che contiene L porta nel
 *          resto una potenza di L: la voce logs;
 *   14c-3. tagliare: un termine c t^{k} con k >= E si può togliere e mettere
 *          nel resto O(t^{E}), perché t^{k} <= t^{E} per t < 1; con c(L) ci
 *          mette anche una potenza di L.
 *
 * Che logs serva lo mostra x x^{\sin(1/x)} da destra di 0: x^{\sin(1/x)} =
 * e^{-L \sin(1/x)}, e -L \sin(1/x) è O(1) a meno di una potenza di L, non
 * limitata. Presa per limitata, e^{O(1)} sarebbe limitata e x x^{\sin(1/x)}
 * tenderebbe a 0, che è falso: in x = 1/(3\pi/2 + 2k\pi) vale 1. */
typedef struct {
    int den;      /* D >= 1: gli esponenti sono multipli di 1/D */
    int low;      /* l'esponente, in unità 1/D, di coef[0] */
    int count;    /* coefficienti noti */
    int end;      /* E: l'esponente del resto, in unità 1/D; CK_FLAT se è più piccolo di ogni potenza */
    int exact;    /* 1 se non c'è resto, e allora end è CK_FLAT */
    int logs;     /* 1 se il resto può avere un fattore L^{m} con m > 0 */
    CkRat *coef;  /* i coefficienti, nell'arena */
} CkSeries;

/* La precisione del lato in corso: l'ordine N del certificato (14d). */
static int ck_goal;

/* Ritorna a + b, esponenti in unità 1/D: CK_FLAT se uno dei due lo è. Due
 * esponenti veri non devono mai avvicinarsi a CK_FLAT. */
static int ck_exp_plus(int a, int b) {
    if (a >= CK_FLAT || b >= CK_FLAT) return CK_FLAT;
    long long sum = (long long)a + b;
    if (sum > CK_FLAT / 16 || sum < -CK_FLAT / 16) { ck_unsupported("esponenti troppo grandi in uno sviluppo"); return 0; }
    return (int)sum;
}

/* Ritorna una serie con i coefficienti tutti zero. */
static CkSeries ck_ser_make(int den, int low, int count, int end, int exact) {
    CkSeries s = {den, low, count > 0 ? count : 0, exact ? CK_FLAT : end, exact, 0, NULL};
    CkRat zero = {ck_poly_number(ck_num_int(0)), ck_one};
    s.coef = ck_alloc(sizeof *s.coef * (size_t)(s.count + 1));
    if (ck_status != CK_ACCEPTED) s.count = 0;
    for (int i = 0; i < s.count; i++) s.coef[i] = zero;
    return s;
}

/* Ritorna la costante c come serie esatta. */
static CkSeries ck_ser_const(CkRat c) {
    CkSeries s = ck_ser_make(1, 0, c.num.count ? 1 : 0, CK_FLAT, 1);
    if (s.count) s.coef[0] = c;
    return s;
}

/* Ritorna una funzione limitata, di cui non si sa altro: O(1). */
static CkSeries ck_ser_bounded(void) { return ck_ser_make(1, 0, 0, 0, 0); }

/* Toglie i primi coefficienti nulli e, da una serie esatta, gli ultimi: la
 * funzione non cambia. */
static CkSeries ck_ser_trim(CkSeries s) {
    while (s.count > 0 && s.coef[0].num.count == 0) { s.coef++; s.count--; s.low++; }
    while (s.exact && s.count > 0 && s.coef[s.count - 1].num.count == 0) s.count--;
    if (s.count == 0) s.low = s.exact ? 0 : s.end;
    return s;
}

/* 14c-3. Ritorna s con il resto O(t^{end/D}), se end è più basso del suo. */
static CkSeries ck_ser_cut(CkSeries s, int end) {
    if (end >= s.end) return s;
    for (int i = end - s.low > 0 ? end - s.low : 0; i < s.count; i++)
        if (ck_c_has_ell(s.coef[i])) s.logs = 1;
    if (s.low + s.count > end) s.count = end > s.low ? end - s.low : 0;
    s.exact = 0;
    s.end = end;
    if (s.count == 0) s.low = end;
    return s;
}

/* Ritorna s con gli esponenti scritti in unità 1/den, multiplo di s.den. */
static CkSeries ck_ser_rescale(CkSeries s, int den) {
    int k = den / s.den;
    if (k == 1) return s;
    CkSeries r = ck_ser_make(den, s.low * k, s.count ? (s.count - 1) * k + 1 : 0, s.end < CK_FLAT ? s.end * k : CK_FLAT, s.exact);
    for (int i = 0; i < s.count && r.count; i++) r.coef[i * k] = s.coef[i];
    r.logs = s.logs;
    if (s.count == 0) r.low = s.exact ? 0 : r.end;
    return r;
}

/* Porta a e b allo stesso denominatore degli esponenti. Ritorna 0, dopo
 * averlo annotato, se sarebbe troppo grande. */
static int ck_ser_common(CkSeries *a, CkSeries *b) {
    long long den = (long long)a->den / ck_gcd(a->den, b->den) * b->den;
    if (den > CK_MAX_SERIES_DEN) { ck_unsupported("denominatori degli esponenti troppo grandi per il verificatore"); return 0; }
    *a = ck_ser_rescale(*a, (int)den);
    *b = ck_ser_rescale(*b, (int)den);
    return 1;
}

/* Ritorna 1 se un coefficiente di s contiene L. */
static int ck_ser_has_ell(CkSeries s) {
    for (int i = 0; i < s.count; i++)
        if (ck_c_has_ell(s.coef[i])) return 1;
    return 0;
}

/* Ritorna c s. Con c che contiene L, anche il resto prende una potenza di L. */
static CkSeries ck_ser_scale(CkSeries s, CkRat c) {
    if (c.num.count == 0) return ck_ser_const(c);
    CkSeries r = ck_ser_make(s.den, s.low, s.count, s.end, s.exact);
    for (int i = 0; i < r.count; i++) r.coef[i] = ck_c_mul(s.coef[i], c);
    r.logs = s.logs || (!s.exact && ck_c_has_ell(c));
    return ck_ser_trim(r);
}

/* Ritorna s t^{shift/D}. */
static CkSeries ck_ser_shift(CkSeries s, int shift) {
    s.low = ck_exp_plus(s.low, shift);
    s.end = ck_exp_plus(s.end, shift);
    return s;
}

/* 14c-1, 14c-3. Ritorna a + b, oppure a - b se minus è diverso da zero; al
 * più CK_MAX_SERIES coefficienti, gli altri vanno nel resto. */
static CkSeries ck_ser_add(CkSeries a, CkSeries b, int minus) {
    if (!ck_ser_common(&a, &b)) return a;
    int end = a.end < b.end ? a.end : b.end, low = CK_FLAT, top = -CK_FLAT;
    if (a.count) { low = a.low; top = a.low + a.count; }
    if (b.count && b.low < low) low = b.low;
    if (b.count && b.low + b.count > top) top = b.low + b.count;
    if (low > top) low = top = end < CK_FLAT ? end : 0;

    /* La somma per esteso, poi i tagli: al resto, e al numero massimo. */
    CkSeries r = ck_ser_make(a.den, low, top - low, CK_FLAT, 1);
    for (int i = 0; i < r.count; i++) {
        int e = low + i, ia = e - a.low, ib = e - b.low;
        CkRat sum = ia >= 0 && ia < a.count ? a.coef[ia] : r.coef[i];
        if (ib >= 0 && ib < b.count) sum = ck_c_add(sum, b.coef[ib], minus);
        r.coef[i] = sum;
    }
    if (!(a.exact && b.exact)) {
        r = ck_ser_cut(r, end);
        r.exact = 0;
        r.end = end;
    }
    if (r.count > CK_MAX_SERIES) r = ck_ser_cut(r, r.low + CK_MAX_SERIES);
    r.logs = r.logs || a.logs || b.logs;
    return ck_ser_trim(r);
}

/* 14c-2, 14c-3. Ritorna a b. */
static CkSeries ck_ser_mul(CkSeries a, CkSeries b) {
    if (!ck_ser_common(&a, &b)) return a;
    if ((a.exact && a.count == 0) || (b.exact && b.count == 0)) return ck_ser_const(ck_c_num(ck_num_int(0)));
    int va = a.count ? a.low : a.end, vb = b.count ? b.low : b.end, exact = a.exact && b.exact;
    int end = ck_exp_plus(va, b.end), other = ck_exp_plus(vb, a.end), low = ck_exp_plus(va, vb);
    if (other < end) end = other;
    int top = a.count && b.count ? a.low + a.count - 1 + b.low + b.count : low;
    if (top > low + CK_MAX_SERIES) top = low + CK_MAX_SERIES;
    if (!exact && top > end) top = end;
    if (top < low) top = low;

    /* I prodotti dei termini, fino a top: quelli oltre vanno nel resto. */
    CkSeries r = ck_ser_make(a.den, low, top - low, CK_FLAT, 1);
    int dropped = 0;
    for (int i = 0; i < a.count; i++)
        for (int j = 0; j < b.count; j++) {
            int k = a.low + i + b.low + j - low;
            if (a.coef[i].num.count == 0 || b.coef[j].num.count == 0) continue;
            if (k >= r.count) { dropped = 1; continue; }
            r.coef[k] = ck_c_add(r.coef[k], ck_c_mul(a.coef[i], b.coef[j]), 0);
        }
    if (!exact || dropped) {
        r.exact = 0;
        r.end = exact ? top : end;
        r.logs = a.logs || b.logs || (!exact && (ck_ser_has_ell(a) || ck_ser_has_ell(b)));
        for (int i = 0; dropped && i < a.count; i++)
            for (int j = 0; j < b.count; j++)
                if (a.low + i + b.low + j >= top && (ck_c_has_ell(a.coef[i]) || ck_c_has_ell(b.coef[j]))) r.logs = 1;
    }
    return ck_ser_trim(r);
}

/* Toglie da *s i primi coefficienti che valgono zero e mette in *lead il
 * termine principale del primo che resta (14b). Ritorna 1 se c'è e non è
 * zero, 0 se non si sa se lo sia, -1 se non resta nessun coefficiente. */
static int ck_ser_settle(CkSeries *s, CkLead *lead) {
    for (;;) {
        *s = ck_ser_trim(*s);
        if (s->count == 0) return -1;
        *lead = ck_lead(s->coef[0]);
        if (!lead->known) return 0;
        if (!lead->zero) return 1;
        s->coef++;
        s->count--;
        s->low++;
    }
}

/* Come ck_ser_settle(), per chi ha bisogno del termine principale: se non si
 * sa il segno del primo coefficiente il certificato non è controllabile, con
 * il motivo unknown; se una serie con il resto non ha nessun coefficiente è
 * respinto, con il motivo missing, perché l'ordine non basta. Una serie
 * esattamente nulla la lascia a chi chiama. Ritorna 1 se il termine c'è. */
static int ck_ser_head(CkSeries *s, CkLead *lead, const char *unknown, const char *missing) {
    int found = ck_ser_settle(s, lead);
    if (found == 0) ck_unsupported(unknown);
    if (found < 0 && !s->exact) ck_reject(missing);
    return found == 1;
}

/* 14d. LE SERIE DI TAYLOR, L'INVERSO, LE POTENZE
 *
 * Per una funzione g tra quelle di 14d-1 e un argomento w che tende a 0,
 * g(w) = a_0 + a_1 w + ... + a_{K-1} w^{K-1} + O(w^{K}) (teorema di Taylor
 * con il resto di Peano). Se |w| <= C t^{v} L^{m}, con v > 0 l'esponente più
 * basso di w, il resto è O(t^{K v}), con un fattore L^{K m} se w contiene L.
 * La somma si calcola con lo schema di Horner, con le operazioni di 14c.
 *
 * Quanti termini, cioè l'ordine. Se w ha un resto O(t^{E}), non serve
 * andare oltre: K è il più piccolo intero con K v >= E. Se w è esatta,
 * l'ordine N del certificato fissa l'esponente a cui fermarsi, che chi
 * chiama dà come target: N per l'esponenziale, il logaritmo, il seno, il
 * coseno, l'arcotangente e (1 + w)^q; N + e per l'inverso di c t^{e} (1 + w),
 * che dopo la moltiplicazione per t^{-e} ha il resto O(t^{N}). Al più
 * CK_MAX_TAYLOR termini, meno se i coefficienti escono dai numeri del
 * verificatore: il resto lo dice.
 *
 *   14d-1. I coefficienti a_k: e^{w}, 1/k!; ln(1 + w), (-1)^{k+1}/k per
 *          k >= 1; sin w e cos w, (-1)^{j}/(2j+1)! e (-1)^{j}/(2j)!;
 *          arctan w, (-1)^{j}/(2j+1); (1 + w)^{q}, q (q - 1) ... (q - k + 1)
 *          / k!.
 *   14d-2. L'inverso. s = c t^{e} (1 + u), con c t^{e} il termine
 *          principale: c(L) non si annulla per t piccolo (14b), e u, la
 *          serie dei termini successivi divisi per c t^{e}, ha esponenti
 *          positivi e tende a 0. Allora 1/s = c^{-1} t^{-e} (1 + u)^{-1}: il
 *          limite di un quoziente è il quoziente dei termini principali.
 *   14d-3. Le potenze. s^{n} con n intero è un prodotto, o l'inverso di un
 *          prodotto; s^{q} con q non intero è c^{q} t^{e q} (1 + u)^{q}, con
 *          c > 0 perché sia definita; con una radice di indice dispari e
 *          c < 0, -((-c)^{q} t^{e q} (1 + u)^{q}). */

typedef enum { CK_TAYLOR_EXP, CK_TAYLOR_LOG, CK_TAYLOR_SIN, CK_TAYLOR_COS, CK_TAYLOR_ATAN, CK_TAYLOR_BINOMIAL } CkTaylor;

/* 14d-1. Mette in a[k] il coefficiente di w^{k} della serie kind, con
 * l'esponente q per la binomiale, e ritorna quanti ne ha messi. */
static int ck_taylor_coefficients(CkTaylor kind, CkNum q, CkNum *a) {
    CkNum factorial = ck_num_int(1), binomial = ck_num_int(1);
    int count = 0;
    for (int k = 0; k < CK_MAX_TAYLOR; k++) {
        if (k > 0) {
            CkNum next = ck_num_add(q, ck_num_int(1 - k));
            if (!ck_num_mul_fits(factorial, ck_num_int(k), &factorial) || !ck_num_mul_fits(binomial, next, &binomial) ||
                !ck_num_mul_fits(binomial, ck_num_make(1, k), &binomial)) break;
        }
        int sign = (k / 2) % 2 ? -1 : 1;
        CkNum c = ck_num_int(0);
        switch (kind) {
        case CK_TAYLOR_EXP: c = ck_num_inv(factorial); break;
        case CK_TAYLOR_LOG: if (k) c = ck_num_make(k % 2 ? 1 : -1, k); break;
        case CK_TAYLOR_SIN: if (k % 2) c = ck_num_mul(ck_num_int(sign), ck_num_inv(factorial)); break;
        case CK_TAYLOR_COS: if (k % 2 == 0) c = ck_num_mul(ck_num_int(sign), ck_num_inv(factorial)); break;
        case CK_TAYLOR_ATAN: if (k % 2) c = ck_num_make(sign, k); break;
        case CK_TAYLOR_BINOMIAL: c = binomial; break;
        }
        a[count++] = c;
    }
    return count;
}

/* Ritorna g(w) per la funzione kind (14d), con w che tende a 0 e target
 * l'esponente, in unità 1/D di w, a cui fermarsi se w è esatta. */
static CkSeries ck_ser_taylor(CkSeries w, CkTaylor kind, CkNum q, int target) {
    CkNum a[CK_MAX_TAYLOR];
    int terms = ck_taylor_coefficients(kind, q, a);
    w = ck_ser_trim(w);
    if (w.exact && w.count == 0) return ck_ser_rescale(ck_ser_const(ck_c_num(a[0])), w.den);
    int v = w.count ? w.low : w.end, end = w.exact ? target : w.end;
    if (v < 1) { ck_reject("uno sviluppo di Taylor con un argomento che non tende a 0"); return w; }
    if (end < v) end = v;
    int used = end / v + (end % v != 0);
    if (used > terms) used = terms;
    if (used < 1) used = 1;

    /* Horner: a_0 + w (a_1 + w (a_2 + ...)). */
    CkSeries sum = ck_ser_rescale(ck_ser_const(ck_c_num(a[used - 1])), w.den);
    for (int k = used - 2; k >= 0 && ck_status == CK_ACCEPTED; k--)
        sum = ck_ser_add(ck_ser_cut(ck_ser_mul(w, sum), end), ck_ser_rescale(ck_ser_const(ck_c_num(a[k])), w.den), 0);

    /* Il resto di Taylor, O(w^{used}); con le potenze di L se w ne ha. */
    long long rest = (long long)used * v;
    sum = ck_ser_cut(sum, rest >= CK_FLAT ? CK_FLAT : (int)rest);
    sum.exact = 0;
    if (sum.end >= CK_FLAT) sum.end = CK_FLAT;
    sum.logs = sum.logs || w.logs || ck_ser_has_ell(w);
    return sum;
}

/* Toglie da s, che ha il primo coefficiente c diverso da zero, il termine
 * principale: s = c t^{low} (1 + u). Mette u in *u e ritorna c. */
static CkRat ck_ser_unit(CkSeries s, CkSeries *u) {
    CkRat c = s.coef[0], inverse = ck_c_inv(c);
    *u = ck_ser_make(s.den, 1, s.count - 1, ck_exp_plus(s.end, -s.low), s.exact);
    for (int i = 1; i < s.count && i - 1 < u->count; i++) u->coef[i - 1] = ck_c_mul(s.coef[i], inverse);
    u->logs = s.logs || (!s.exact && ck_c_has_ell(c));
    *u = ck_ser_trim(*u);
    return c;
}

/* 14d-2. Mette 1/s in *out e ritorna 1; ritorna -1 se s è esattamente zero,
 * e allora 1/s non ha valori; 0 se il conto non si fa, dopo averlo annotato
 * (ck_ser_head()). positive è 1 se chi chiama sa già che il primo
 * coefficiente di s è un numero positivo. */
static int ck_ser_inverse(CkSeries s, CkSeries *out, int positive) {
    CkLead lead;
    s = ck_ser_trim(s);
    if (s.exact && s.count == 0) return -1;
    if (!positive && !ck_ser_head(&s, &lead, "il segno del primo coefficiente di un divisore non si sa stabilire",
                                  "un divisore non ha un termine principale entro l'ordine dato")) return 0;
    if (positive && s.count == 0) { ck_reject("un divisore non ha un termine principale entro l'ordine dato"); return 0; }
    CkSeries u;
    CkRat c = ck_ser_unit(s, &u);
    CkSeries sum = ck_ser_taylor(u, CK_TAYLOR_BINOMIAL, ck_num_int(-1), ck_goal * s.den + s.low);
    *out = ck_ser_shift(ck_ser_scale(sum, ck_c_inv(c)), -s.low);
    return ck_status == CK_ACCEPTED;
}

/* 14d-3. Mette s^{n} in *out, per n intero; ritorna come ck_ser_inverse(). */
static int ck_ser_power_int(CkSeries s, long long n, CkSeries *out) {
    if (n > CK_MAX_POWER || n < -CK_MAX_POWER) { ck_unsupported("esponente troppo grande per il verificatore"); return 0; }
    if (n < 0) {
        int inverted = ck_ser_inverse(s, &s, 0);
        if (inverted <= 0) return inverted;
        n = -n;
    }
    CkSeries result = ck_ser_const(ck_c_num(ck_num_int(1))), square = s;
    for (; n > 0 && ck_status == CK_ACCEPTED; n /= 2) {
        if (n % 2) result = ck_ser_mul(result, square);
        if (n > 1) square = ck_ser_mul(square, square);
    }
    *out = result;
    return ck_status == CK_ACCEPTED;
}

static int ck_const_power(CkRat c, CkNum q, CkRat *out);

/* 14d-3. Mette s^{q} in *out, per q razionale non intero; odd è 1 per una
 * radice di indice dispari, che vale anche per una base negativa. Ritorna 1;
 * -1 se la potenza non ha valori vicino al punto; 0 se il conto non si fa,
 * dopo averlo annotato. */
static int ck_ser_power(CkSeries s, CkNum q, int odd, CkSeries *out) {
    CkLead lead;
    s = ck_ser_trim(s);
    if (s.exact && s.count == 0) {
        *out = s;
        return q.num > 0 ? 1 : -1;
    }
    if (!ck_ser_head(&s, &lead, "il segno della base di una potenza non si sa stabilire", "la base di una potenza non ha un termine principale entro l'ordine dato"))
        return 0;
    if (lead.side < 0 && !odd) return -1;

    /* c^{q}, con il segno tolto e rimesso per una radice dispari. */
    CkSeries u;
    CkRat c = ck_ser_unit(s, &u), cq;
    if (lead.side < 0) c = ck_c_neg(c);
    if (!ck_const_power(c, q, &cq)) return 0;
    if (lead.side < 0) cq = ck_c_neg(cq);

    /* t^{e q}, con e = low/D: il denominatore degli esponenti che serve. */
    CkNum shift = ck_num_mul(ck_num_make(s.low, s.den), q);
    long long den = (long long)s.den / ck_gcd(s.den, shift.den) * (long long)shift.den;
    if (den > CK_MAX_SERIES_DEN || shift.num > CK_FLAT / 16 || shift.num < -CK_FLAT / 16) { ck_unsupported("denominatori degli esponenti troppo grandi per il verificatore"); return 0; }
    CkSeries sum = ck_ser_rescale(ck_ser_taylor(u, CK_TAYLOR_BINOMIAL, q, ck_goal * s.den), (int)den);
    *out = ck_ser_shift(ck_ser_scale(sum, cq), (int)(shift.num * (den / shift.den)));
    return ck_status == CK_ACCEPTED;
}

/* 14e. LE FUNZIONI DELLE COSTANTI
 *
 * Il valore di una funzione nel numero a cui tende il suo argomento, scritto
 * in modo esatto con le lettere di 14a. Le regole, tutte identità vere:
 *
 *   14e-1. b^{r}, con r = n/d razionale positivo: b^{floor(r)} per la
 *          lettera b^{1/d} elevata al resto, e per un primo b^{floor(r)} è un
 *          numero; con r negativo, 1 / b^{-r};
 *   14e-2. e^{a}, con a una somma di termini r (un razionale) e q ln p:
 *          e^{r} per i p^{q}, perché e^{u + v} = e^{u} e^{v} ed e^{q ln p} =
 *          p^{q}. Ogni altra costante all'esponente non ha scrittura;
 *   14e-3. ln c, con c = r b_1^{k_1} ... un monomio positivo: ln r + k_1 ln
 *          b_1 + ..., perché ln(u v) = ln u + ln v per u, v > 0; ln r si
 *          scompone nei primi, ln(p^{1/m}) = (1/m) ln p, ln(e^{1/m}) = 1/m;
 *   14e-4. c^{q} con c monomio positivo: r^{q} b_1^{k_1 q} ..., perché
 *          (u v)^{q} = u^{q} v^{q} per u, v > 0; poi 14e-1;
 *   14e-5. sin e cos di k pi / 12 con k multiplo di 2 o di 3, cioè degli
 *          angoli multipli di 30 e di 45 gradi: dai triangoli con angoli di
 *          30, 60, 90 gradi e di 45, 45, 90, cos(pi/6) = \sqrt{3}/2,
 *          cos(pi/4) = \sqrt{2}/2, cos(pi/3) = 1/2, e gli altri per simmetria;
 *   14e-6. arctan di 0, \pm 1, \pm \sqrt{3}, \pm \sqrt{3}/3: 0, \pm pi/4,
 *          \pm pi/3, \pm pi/6, gli angoli di cui sono le tangenti.
 *
 * Un argomento fuori da queste forme rende il certificato non controllabile. */

/* 14e-1. Ritorna b^{r} come coefficiente: b è 'p' con il primo prime, oppure
 * il nome di una lettera CONST. */
static CkRat ck_const_base_power(char base, long long prime, CkNum r) {
    /* Un esponente negativo dà l'inverso: e^{-1/6} è 1 / e^{1/6}, non
     * e^{1/6}^{5} / e, che ha lo stesso valore ma un grado più alto (14b-3). */
    if (r.num < 0) return ck_c_inv(ck_const_base_power(base, prime, ck_num_neg(r)));
    __int128 whole = r.num / r.den, rest = r.num - whole * r.den;
    CkRat value = ck_c_num(ck_num_int(1));
    if (whole > CK_MAX_POWER || r.den > CK_MAX_EXPONENT) { ck_unsupported("potenza di una costante troppo grande per il verificatore"); return value; }
    for (__int128 i = 0; i < whole && ck_status == CK_ACCEPTED; i++)
        value = ck_c_mul(value, base == 'p' ? ck_c_num(ck_num_int(prime)) : ck_c_var(ck_const_var(base, prime, 1)));
    for (__int128 i = 0; i < rest && ck_status == CK_ACCEPTED; i++) value = ck_c_mul(value, ck_c_var(ck_const_var(base, prime, (int)r.den)));
    return value;
}

/* Mette in *r il razionale numeratore / denominatore del monomio c, cioè di
 * una frazione di due polinomi di un termine solo, e ritorna 1; ritorna 0 se c
 * non è un monomio. */
static int ck_c_monomial(CkRat c, CkNum *r) {
    if (c.num.count != 1 || c.den.count != 1) return 0;
    *r = ck_num_mul(c.num.term[0].coef, ck_num_inv(c.den.term[0].coef));
    return 1;
}

/* 14e-2. Mette e^{a} in *out e ritorna 1; ritorna 0, dopo averlo annotato, se
 * a non ha la forma voluta. */
static int ck_const_exp(CkRat a, CkRat *out) {
    *out = ck_c_num(ck_num_int(1));
    if (!ck_poly_is_constant(a.den)) { ck_unsupported("l'esponenziale di una costante che il verificatore non sa scrivere"); return 0; }
    CkNum scale = ck_num_inv(a.den.term[0].coef);
    for (int i = 0; i < a.num.count && ck_status == CK_ACCEPTED; i++) {
        const CkTerm *t = &a.num.term[i];
        CkNum q = ck_num_mul(t->coef, scale);
        int letter = -1, letters = 0;
        for (int v = 0; v < ck_var_count; v++)
            if (t->e[v]) { letter = v; letters++; }
        if (letters == 0) *out = ck_c_mul(*out, ck_const_base_power('e', 0, q));
        else if (letters == 1 && t->e[letter] == 1 && ck_vars[letter].kind == CK_VAR_CONST && ck_vars[letter].name == 'l' && ck_vars[letter].position == 1)
            *out = ck_c_mul(*out, ck_const_base_power('p', ck_vars[letter].prime, q));
        else { ck_unsupported("l'esponenziale di una costante che il verificatore non sa scrivere"); return 0; }
    }
    return ck_status == CK_ACCEPTED;
}

/* Aggiunge a *sum times volte la quantità di ogni primo di n, moltiplicata per
 * scale: ln n = somma di e_p ln p, se letters è 1; altrimenti n^{scale} =
 * prodotto dei p^{e_p scale} (14e-3, 14e-4). */
static void ck_prime_parts(__int128 n, CkNum scale, int letters, CkRat *sum) {
    if (n > CK_MAX_FACTORED) { ck_unsupported("numero troppo grande da scomporre in fattori primi"); return; }
    for (long long rest = (long long)n, times; rest > 1 && ck_status == CK_ACCEPTED;) {
        long long p = ck_pop_prime(&rest, &times);
        CkNum amount = ck_num_mul(ck_num_int(times), scale);
        if (letters) *sum = ck_c_add(*sum, ck_c_mul(ck_c_num(amount), ck_c_var(ck_const_var('l', p, 1))), 0);
        else *sum = ck_c_mul(*sum, ck_const_base_power('p', p, amount));
    }
}

/* 14e-3. Mette ln c in *out, per c positiva senza L, e ritorna 1; ritorna 0,
 * dopo averlo annotato, se c non è un monomio o contiene pi. */
static int ck_const_ln(CkRat c, CkRat *out) {
    CkNum r;
    *out = ck_c_num(ck_num_int(0));
    if (!ck_c_monomial(c, &r) || r.num <= 0) { ck_unsupported("il logaritmo di una costante che il verificatore non sa scrivere"); return 0; }
    ck_prime_parts(r.num, ck_num_int(1), 1, out);
    ck_prime_parts(r.den, ck_num_int(-1), 1, out);
    for (int v = 0; v < ck_var_count && ck_status == CK_ACCEPTED; v++) {
        int k = c.num.term[0].e[v] - c.den.term[0].e[v];
        if (!k) continue;
        CkNum share = ck_num_make(k, ck_vars[v].position);
        if (ck_vars[v].name == 'p') *out = ck_c_add(*out, ck_c_mul(ck_c_num(share), ck_c_var(ck_const_var('l', ck_vars[v].prime, 1))), 0);
        else if (ck_vars[v].name == 'e') *out = ck_c_add(*out, ck_c_num(share), 0);
        else { ck_unsupported("il logaritmo di una costante che il verificatore non sa scrivere"); return 0; }
    }
    return ck_status == CK_ACCEPTED;
}

/* 14e-4. Mette c^{q} in *out, per c positiva, e ritorna 1; ritorna 0, dopo
 * averlo annotato, se c non è un monomio. */
static int ck_const_power(CkRat c, CkNum q, CkRat *out) {
    CkNum r;
    *out = ck_c_num(ck_num_int(1));
    if (!ck_c_monomial(c, &r) || r.num <= 0) { ck_unsupported("la potenza di una costante che il verificatore non sa scrivere"); return 0; }
    ck_prime_parts(r.num, q, 0, out);
    ck_prime_parts(r.den, ck_num_neg(q), 0, out);
    for (int v = 0; v < ck_var_count && ck_status == CK_ACCEPTED; v++) {
        int k = c.num.term[0].e[v] - c.den.term[0].e[v];
        if (k) *out = ck_c_mul(*out, ck_const_base_power(ck_vars[v].name, ck_vars[v].prime, ck_num_mul(ck_num_make(k, ck_vars[v].position), q)));
    }
    return ck_status == CK_ACCEPTED;
}

/* Ritorna il numero n/d \sqrt{root} (root 1: il razionale n/d). */
static CkRat ck_const_surd(long long n, long long d, int root) {
    CkRat value = ck_c_num(ck_num_make(n, d));
    return root == 1 ? value : ck_c_mul(value, ck_c_var(ck_const_var('p', root, 2)));
}

/* 14e-5. Mette sin u e cos u in *sine e *cosine, per u = k pi / 12 con k
 * multiplo di 2 o di 3, e ritorna 1; ritorna 0, dopo averlo annotato, per
 * ogni altra costante. cos(k pi/12) per k da 0 a 12, come n/d \sqrt{root};
 * per k da 12 a 24 cos(theta) = cos(2 pi - theta), e sin(theta) =
 * cos(pi/2 - theta). */
static int ck_const_sin_cos(CkRat u, CkRat *sine, CkRat *cosine) {
    static const int table[13][3] = {{1, 1, 1}, {0, 0, 0}, {1, 2, 3}, {1, 2, 2}, {1, 2, 1}, {0, 0, 0}, {0, 1, 1},
                                     {0, 0, 0}, {-1, 2, 1}, {-1, 2, 2}, {-1, 2, 3}, {0, 0, 0}, {-1, 1, 1}};
    CkNum r = ck_num_int(0);
    int pi = ck_const_var('q', 0, 1);

    /* u = r pi: un monomio di pi al primo grado, oppure zero. */
    if (u.num.count && (!ck_c_monomial(u, &r) || ck_poly_is_constant(u.num) || u.num.term[0].e[pi] != 1 || u.den.term[0].e[pi] != 0)) r.den = 0;
    for (int v = 0; v < ck_var_count && u.num.count && r.den; v++)
        if (v != pi && (u.num.term[0].e[v] || u.den.term[0].e[v])) r.den = 0;
    CkNum twelfths = r.den ? ck_num_mul(r, ck_num_int(12)) : r;
    if (!r.den || twelfths.den != 1) { ck_unsupported("seno e coseno di una costante che il verificatore non sa scrivere"); return 0; }
    long long k[2] = {(long long)(((twelfths.num % 24) + 24) % 24), 0};
    k[1] = (6 - k[0] + 24) % 24;
    for (int i = 0; i < 2; i++) {
        long long fold = k[i] <= 12 ? k[i] : 24 - k[i];
        if (!table[fold][1]) { ck_unsupported("seno e coseno di una costante che il verificatore non sa scrivere"); return 0; }
        *(i ? sine : cosine) = ck_const_surd(table[fold][0], table[fold][1], table[fold][2]);
    }
    return 1;
}

/* 14e-6. Mette arctan u in *out e ritorna 1; ritorna 0, dopo averlo
 * annotato, se u non è uno dei numeri di 14e-6. */
static int ck_const_arctan(CkRat u, CkRat *out) {
    static const struct { long long n, d; int root; long long angle; } table[] = {
        {0, 1, 1, 0}, {1, 1, 1, 3}, {1, 1, 3, 4}, {1, 3, 3, 2},
    };
    CkRat pi12 = ck_c_mul(ck_c_var(ck_const_var('q', 0, 1)), ck_c_num(ck_num_make(1, 12)));
    for (size_t i = 0; i < sizeof table / sizeof *table; i++)
        for (int sign = 1; sign >= -1; sign -= 2) {
            if (ck_c_equal(u, ck_const_surd(sign * table[i].n, table[i].d, table[i].root)) != 1) continue;
            *out = ck_c_mul(pi12, ck_c_num(ck_num_int(sign * table[i].angle)));
            return 1;
        }
    ck_unsupported("l'arcotangente di una costante che il verificatore non sa scrivere");
    return 0;
}

/* 14f. LE FUNZIONI VICINO AL PUNTO
 *
 * Ciò che si sa di una funzione vicino al punto, da un lato: che non ha
 * valori per nessun t piccolo, oppure e^{G} S, con S una serie (14c) e G un
 * esponente esatto fatto di potenze negative di t (14g; senza termini G è
 * zero e il fattore manca). Una funzione senza valori rende senza valori
 * ogni espressione che la contiene: il suo calcolo non ha senso in nessun
 * punto vicino.
 *
 * Le regole, per una serie s che non ha il fattore:
 *
 *   14f-1. e^{s}. Se s è O(1), e^{s} lo è: e^{-C} <= e^{s} <= e^{C}, purché
 *          s sia limitata davvero (senza logs). Se il resto di s tende a 0,
 *          tutti i termini con esponente negativo sono noti, e sono il
 *          fattore e^{G} (14g); il termine costante a + b L dà
 *          e^{a} t^{-b} (14e-2: e^{b L} = t^{-b}); il resto P tende a 0, ed
 *          e^{P} è una serie di Taylor.
 *   14f-2. ln s, con s = c t^{e} (1 + u) e c > 0 senza L: ln c - e L +
 *          ln(1 + u). Con c < 0 il logaritmo non ha valori.
 *   14f-3. sin e cos di s che tende a un numero u0 senza L: con s = u0 + w,
 *          sin u0 cos w + cos u0 sin w e cos u0 cos w - sin u0 sin w. Se s
 *          non tende a un numero, il seno e il coseno sono comunque limitati,
 *          tra -1 e 1: O(1). La tangente e la cotangente sono quozienti.
 *   14f-4. arctan s. Se s tende a +infinito o -infinito, arctan s =
 *          \pm pi/2 - arctan(1/s), che vale per ogni s diverso da zero con il
 *          segno di s. Se tende a u0: arctan u0 + arctan(w / (1 + u0 s)),
 *          la formula di sottrazione arctan a - arctan b = arctan((a - b) /
 *          (1 + a b)), valida per a b > -1: qui a b = u0 s tende a u0^2 >= 0,
 *          e 1 + u0 s tende a 1 + u0^2 > 0. Altrimenti arctan s è limitato.
 *   14f-5. arcsin u = 2 arctan(u / (1 + \sqrt{1 - u^2})) e arccos u = pi/2 -
 *          arcsin u, per u tra -1 e 1: con a = arcsin u, tra -pi/2 e pi/2,
 *          tan(a/2) = sin a / (1 + cos a) = u / (1 + \sqrt{1 - u^2}), perché
 *          cos a = \sqrt{1 - u^2}, e a/2 sta tra -pi/4 e pi/4, dove
 *          l'arcotangente inverte la tangente. Fuori da [-1, 1] la radice non
 *          c'è: il dominio è lo stesso.
 *   14f-6. |s| è s o -s con il segno del termine principale; una serie senza
 *          coefficienti noti resta com'è, perché il resto è già un valore
 *          assoluto. */

/* Una funzione vicino al punto, da un lato (vedi in testa a 14f). */
typedef struct {
    int undefined;     /* 1 se non ha valori per nessun t piccolo */
    CkSeries series;   /* S */
    CkSeries growth;   /* G (14g): esatta, con i soli esponenti negativi; senza termini se il fattore manca */
} CkNear;

/* Ritorna la funzione e^{growth} s, senza valori se undefined è 1. */
static CkNear ck_near(CkSeries s, CkSeries growth, int undefined) {
    CkNear v = {undefined, s, growth};
    return v;
}

/* L'esponente nullo: nessun fattore e^{G}. */
static CkSeries ck_no_growth(void) { return ck_ser_const(ck_c_num(ck_num_int(0))); }

/* Ritorna la serie s, senza fattore. */
static CkNear ck_near_series(CkSeries s) { return ck_near(s, ck_no_growth(), 0); }

/* Ritorna una funzione senza valori. */
static CkNear ck_near_nowhere(void) { return ck_near(ck_ser_bounded(), ck_no_growth(), 1); }

/* Ritorna la costante c. */
static CkNear ck_near_const(CkRat c) { return ck_near_series(ck_ser_const(c)); }

/* 14g. IL FATTORE e^{G}: LA SCALA DELLE CRESCITE
 *
 * e^{x} / x^{5} a +infinito non ha una serie in t = 1/x: e^{1/t} cresce più
 * di ogni potenza. Lo si scrive allora e^{G} S, con G = somma di g_i(L)
 * t^{-k_i}, k_i > 0, un esponente esatto. I fatti su cui si regge (è la
 * scala delle crescite dei libri: D. Gruntz, "On computing limits in a
 * symbolic manipulation system", ETH Zurigo 1996; G. H. Hardy, "Orders of
 * infinity", 1910):
 *
 *   14g-1. se H = lambda(L) t^{-k} (1 + o(1)) con k > 0 e lambda(L) ~ c L^{j},
 *          c diverso da zero, allora H tende a +infinito o -infinito con il
 *          segno di c, e per ogni e e d, e^{H} t^{e} L^{d} = exp(H - e L +
 *          d ln L) tende a +infinito o a 0, perché H - e L + d ln L = H (1 +
 *          o(1)): t^{-k} L^{j} cresce più di L e di ln L. e^{H} sta sopra o
 *          sotto ogni potenza di t e di L;
 *   14g-2. somma: se G1 - G2 = H come in 14g-1 con c > 0, e^{G1} S1 +
 *          e^{G2} S2 = e^{G1} (S1 + e^{-H} S2), ed e^{-H} S2 è più piccola di
 *          ogni potenza di t: è un resto O(t^{CK_FLAT}). Se G1 - G2 è zero le
 *          serie si sommano;
 *   14g-3. prodotto e quoziente: e^{G1} S1 e^{G2} S2 = e^{G1 + G2} S1 S2, e
 *          1 / (e^{G} S) = e^{-G} / S;
 *   14g-4. potenze, logaritmo, valore assoluto: (e^{G} S)^{q} = e^{q G}
 *          S^{q}, ln(e^{G} S) = G + ln S, |e^{G} S| = e^{G} |S|, perché
 *          e^{G} > 0;
 *   14g-5. per le funzioni che il fattore non lo trattano si usa solo dove
 *          va: con G che tende a -infinito e^{G} S è più piccola di ogni
 *          potenza di t, una serie O(t^{CK_FLAT}); con G che tende a
 *          +infinito seno e coseno restano limitati, l'arcotangente è
 *          \pm pi/2 - arctan(e^{-G} / S) (14f-4), e e^{e^{G} S} con S < 0 è
 *          più piccola di ogni potenza.
 *
 * Il limite di e^{G} S, con il termine principale di S noto: G tende a
 * +infinito, e allora la funzione tende all'infinito con il segno di S, o a
 * -infinito, e allora tende a 0 (14g-1). */

/* Dove va l'esponente g (14g-1): mette in *dir +1 se tende a +infinito, -1
 * se tende a -infinito, e ritorna 1; ritorna 0 se g vale zero; -1 dopo aver
 * annotato che il segno del suo termine principale non si sa. */
static int ck_growth_direction(CkSeries g, int *dir) {
    CkLead lead;
    int found = ck_ser_settle(&g, &lead);
    if (found < 0) return 0;
    if (found == 0) { ck_unsupported("un esponenziale di cui non si sa se cresce o decresce"); return -1; }
    *dir = lead.side;
    return 1;
}

/* Ritorna e^{g} s: un esponente nullo si toglie, e così quello di una serie
 * esattamente nulla. */
static CkNear ck_near_grown(CkSeries s, CkSeries g) {
    int dir;
    g = ck_ser_trim(g);
    s = ck_ser_trim(s);
    if (!g.exact) { ck_unsupported("un esponente con troppi termini"); return ck_near_series(s); }
    if (ck_growth_direction(g, &dir) <= 0 || (s.exact && s.count == 0)) return ck_near_series(s);
    return ck_near(s, g, 0);
}

/* Ritorna 1 se v ha il fattore e^{G}. */
static int ck_near_grows(CkNear v) { return !v.undefined && v.growth.count > 0; }

/* 14g-5. Se v ha il fattore e G tende a -infinito, ritorna la funzione come
 * serie più piccola di ogni potenza, e *dir = -1; con G che tende a
 * +infinito ritorna v e *dir = +1; senza fattore ritorna v e *dir = 0. */
static CkNear ck_near_plain(CkNear v, int *dir) {
    *dir = 0;
    if (!ck_near_grows(v)) return v;
    ck_growth_direction(v.growth, dir);
    if (*dir < 0) return ck_near_series(ck_ser_make(1, CK_FLAT, 0, CK_FLAT, 0));
    return v;
}

/* 14g-2, 14g-3. a + b, a - b, a b, a / b con il fattore in almeno uno dei due:
 * op è CK_ADD, CK_SUB, CK_MUL o CK_DIV. */
static CkNear ck_near_grown_binary(CkKind op, CkNear a, CkNear b) {
    int dir;
    if (op == CK_MUL || op == CK_DIV) {
        CkSeries factor = b.series;
        if (op == CK_DIV) {
            int inverted = ck_ser_inverse(b.series, &factor, 0);
            if (inverted < 0) return ck_near_nowhere();
            if (inverted == 0) return a;
        }
        return ck_near_grown(ck_ser_mul(a.series, factor), ck_ser_add(a.growth, b.growth, op == CK_DIV));
    }

    /* Una somma con uno zero esatto è l'altro addendo. */
    CkSeries sa = ck_ser_trim(a.series), sb = ck_ser_trim(b.series);
    if (sa.exact && sa.count == 0) return op == CK_SUB ? ck_near_grown(ck_ser_scale(b.series, ck_c_num(ck_num_int(-1))), b.growth) : b;
    if (sb.exact && sb.count == 0) return a;

    /* Chi cresce di più, con la differenza degli esponenti. */
    CkSeries delta = ck_ser_add(a.growth, b.growth, 1);
    int known = ck_growth_direction(delta, &dir);
    if (known < 0) return a;
    if (known == 0) return ck_near_grown(ck_ser_add(a.series, b.series, op == CK_SUB), a.growth);
    CkNear big = dir > 0 ? a : b;
    CkSeries s = dir < 0 && op == CK_SUB ? ck_ser_scale(big.series, ck_c_num(ck_num_int(-1))) : big.series;
    return ck_near_grown(ck_ser_add(s, ck_ser_make(1, CK_FLAT, 0, CK_FLAT, 0), 0), big.growth);
}

/* Ritorna a + b, a - b, a b o a / b, secondo op. */
static CkNear ck_near_binary(CkKind op, CkNear a, CkNear b) {
    CkSeries inverse;
    if (a.undefined) return a;
    if (b.undefined) return b;
    if (ck_near_grows(a) || ck_near_grows(b)) return ck_near_grown_binary(op, a, b);
    if (op == CK_ADD || op == CK_SUB) return ck_near_series(ck_ser_add(a.series, b.series, op == CK_SUB));
    if (op == CK_MUL) return ck_near_series(ck_ser_mul(a.series, b.series));
    int inverted = ck_ser_inverse(b.series, &inverse, 0);
    if (inverted < 0) return ck_near_nowhere();
    return ck_near_series(inverted > 0 ? ck_ser_mul(a.series, inverse) : a.series);
}

/* Divide s, che tende a un numero o a 0 (il resto tende a 0), nei suoi
 * termini con esponente negativo, *negative, esatti perché il resto tende a
 * 0, nel termine costante, *constant, e nel resto della serie, *rest, che
 * tende a 0. */
static void ck_ser_split(CkSeries s, CkSeries *negative, CkRat *constant, CkSeries *rest) {
    int count = s.low < 0 ? -s.low : 0;
    if (count > s.count) count = s.count;
    *negative = ck_ser_make(s.den, s.low, count, CK_FLAT, 1);
    for (int i = 0; i < negative->count; i++) negative->coef[i] = s.coef[i];
    *negative = ck_ser_trim(*negative);
    *rest = s;
    rest->coef += count;
    rest->count -= count;
    rest->low += count;
    *constant = ck_c_num(ck_num_int(0));
    if (rest->count && rest->low == 0) {
        *constant = rest->coef[0];
        rest->coef++;
        rest->count--;
        rest->low++;
    }
    if (rest->count == 0) rest->low = rest->exact ? 0 : rest->end;
}

/* 14f-1. e^{v}. */
static CkNear ck_near_exp(CkNear v) {
    int dir;
    if (v.undefined) return v;

    /* 14g-5: e^{e^{G} S} si sa scrivere solo se va a 0, o se G va a -infinito. */
    if (ck_near_grows(v)) {
        CkLead lead;
        CkSeries s = v.series;
        v = ck_near_plain(v, &dir);
        if (dir > 0) {
            if (ck_ser_settle(&s, &lead) == 1 && lead.side < 0) return ck_near_series(ck_ser_make(1, CK_FLAT, 0, CK_FLAT, 0));
            ck_unsupported("l'esponenziale di una funzione che cresce più di ogni potenza");
            return v;
        }
    }
    CkSeries s = ck_ser_trim(v.series), negative, rest;
    if (s.count == 0 && s.end == 0 && !s.logs) return ck_near_series(ck_ser_bounded());
    if (s.end <= 0) { ck_unsupported("l'esponenziale di una funzione di cui lo sviluppo non dice dove va"); return v; }

    /* Il termine costante a + b L: b razionale, a senza L. */
    CkRat constant, a = ck_c_num(ck_num_int(0)), base;
    CkNum b = ck_num_int(0);
    ck_ser_split(s, &negative, &constant, &rest);
    if (ck_c_has_ell(constant)) {
        if (!ck_poly_is_constant(constant.den)) { ck_unsupported("l'esponenziale di una funzione di L che il verificatore non sa scrivere"); return v; }
        for (int i = 0; i < constant.num.count; i++) {
            CkTerm t = constant.num.term[i];
            CkNum degree = ck_ell_degree(&t), c = ck_num_mul(t.coef, ck_num_inv(constant.den.term[0].coef));
            int others = 0;
            for (int w = 0; w < ck_var_count; w++)
                if (t.e[w] && !(ck_vars[w].kind == CK_VAR_CONST && ck_vars[w].name == 'L' && ck_vars[w].position == 1)) others = 1;
            if (degree.num == 0) {
                CkTerm *piece = ck_alloc(sizeof *piece);
                *piece = t;
                a = ck_c_add(a, (CkRat){{1, piece}, constant.den}, 0);
                continue;
            }
            if (others || degree.num != 1 || degree.den != 1) { ck_unsupported("l'esponenziale di una funzione di L che il verificatore non sa scrivere"); return v; }
            b = ck_num_add(b, c);
        }
    } else {
        a = constant;
    }
    if (!ck_const_exp(a, &base)) return v;

    /* e^{a} t^{-b} e^{P}, con il denominatore degli esponenti che serve. */
    long long den = (long long)rest.den / ck_gcd(rest.den, b.den) * (long long)b.den;
    if (den > CK_MAX_SERIES_DEN || b.num > CK_FLAT / 16 || b.num < -CK_FLAT / 16) { ck_unsupported("denominatori degli esponenti troppo grandi per il verificatore"); return v; }
    CkSeries sum = ck_ser_rescale(ck_ser_scale(ck_ser_taylor(rest, CK_TAYLOR_EXP, ck_num_int(0), ck_goal * rest.den), base), (int)den);
    return ck_near_grown(ck_ser_shift(sum, (int)(-b.num * (den / b.den))), negative);
}

/* 14f-2, 14g-4. ln v. */
static CkNear ck_near_ln(CkNear v) {
    CkLead lead;
    CkSeries s = v.series, u;
    CkRat ln_c;
    if (v.undefined) return v;
    /* Un argomento che vale esattamente zero non ha logaritmo, come uno
     * negativo: ln 0 non è un numero. */
    if (!ck_ser_head(&s, &lead, "il segno dell'argomento di un logaritmo non si sa stabilire", "l'argomento di un logaritmo non ha un termine principale entro l'ordine dato"))
        return s.exact && s.count == 0 && ck_status == CK_ACCEPTED ? ck_near_nowhere() : v;
    if (lead.side < 0) return ck_near_nowhere();
    if (ck_c_has_ell(s.coef[0])) { ck_unsupported("il logaritmo di un coefficiente che contiene L"); return v; }
    CkRat c = ck_ser_unit(s, &u);
    if (!ck_const_ln(c, &ln_c)) return v;

    /* ln c - (low/D) L + ln(1 + u), e con il fattore anche G. */
    CkRat ell = ck_c_mul(ck_c_num(ck_num_make(-s.low, s.den)), ck_c_var(ck_const_var('L', 0, 1)));
    CkSeries sum = ck_ser_add(ck_ser_taylor(u, CK_TAYLOR_LOG, ck_num_int(0), ck_goal * u.den), ck_ser_const(ck_c_add(ln_c, ell, 0)), 0);
    return ck_near_series(ck_near_grows(v) ? ck_ser_add(v.growth, sum, 0) : sum);
}

/* 14f-3, 14g-5. sin v, cos v, tan v, cot v: fn dice quale. */
static CkNear ck_near_trig(CkNear v, CkKind fn) {
    int quotient = fn == CK_TAN || fn == CK_COT, dir;
    CkRat u0, sine, cosine;
    CkSeries negative, w, s, c, inverse;
    if (v.undefined) return v;
    v = ck_near_plain(v, &dir);

    /* Se l'argomento non tende a un numero senza L: limitati, tra -1 e 1. */
    s = ck_ser_trim(v.series);
    int converges = dir <= 0 && s.end > 0 && (s.count == 0 || s.low >= 0);
    if (converges) {
        ck_ser_split(s, &negative, &u0, &w);
        converges = !ck_c_has_ell(u0);
    }
    if (!converges) {
        if (quotient) ck_unsupported("la tangente di qualcosa che non tende a un numero");
        return ck_near_series(ck_ser_bounded());
    }
    if (!ck_const_sin_cos(u0, &sine, &cosine)) return v;
    CkSeries ws = ck_ser_taylor(w, CK_TAYLOR_SIN, ck_num_int(0), ck_goal * w.den), wc = ck_ser_taylor(w, CK_TAYLOR_COS, ck_num_int(0), ck_goal * w.den);
    s = ck_ser_add(ck_ser_scale(wc, sine), ck_ser_scale(ws, cosine), 0);
    c = ck_ser_add(ck_ser_scale(wc, cosine), ck_ser_scale(ws, sine), 1);
    if (fn == CK_SIN) return ck_near_series(s);
    if (fn == CK_COS) return ck_near_series(c);
    int inverted = ck_ser_inverse(fn == CK_TAN ? c : s, &inverse, 0);
    if (inverted < 0) return ck_near_nowhere();
    return ck_near_series(inverted > 0 ? ck_ser_mul(fn == CK_TAN ? s : c, inverse) : s);
}

/* 14f-4, 14g-5. arctan v. */
static CkNear ck_near_arctan(CkNear v) {
    CkRat half_pi = ck_c_mul(ck_c_var(ck_const_var('q', 0, 1)), ck_c_num(ck_num_make(1, 2))), u0, angle;
    CkSeries s, negative, w, inverse, z;
    CkLead lead;
    int dir;
    if (v.undefined) return v;

    /* Verso un infinito: \pm pi/2 - arctan(1/v); con il fattore, 1/v =
     * e^{-G} / S è più piccola di ogni potenza. */
    if (ck_near_grows(v)) {
        s = v.series;
        CkNear plain = ck_near_plain(v, &dir);
        if (dir < 0) return ck_near_arctan(plain);
        if (ck_ser_settle(&s, &lead) != 1) { ck_unsupported("l'arcotangente di un esponenziale di cui non si sa il segno"); return v; }
        return ck_near_series(ck_ser_add(ck_ser_const(lead.side > 0 ? half_pi : ck_c_neg(half_pi)), ck_ser_make(1, CK_FLAT, 0, CK_FLAT, 0), 0));
    }
    s = v.series;
    if (ck_ser_settle(&s, &lead) == 1 && s.low < 0) {
        if (ck_ser_inverse(s, &inverse, 0) <= 0) return v;
        CkSeries sum = ck_ser_taylor(inverse, CK_TAYLOR_ATAN, ck_num_int(0), ck_goal * inverse.den);
        return ck_near_series(ck_ser_add(ck_ser_const(lead.side > 0 ? half_pi : ck_c_neg(half_pi)), sum, 1));
    }

    /* Verso un numero u0: la formula di sottrazione. */
    s = ck_ser_trim(v.series);
    if (s.end <= 0 || (s.count && s.low < 0)) return ck_near_series(ck_ser_bounded());
    ck_ser_split(s, &negative, &u0, &w);
    if (ck_c_has_ell(u0)) return ck_near_series(ck_ser_bounded());
    if (!ck_const_arctan(u0, &angle)) return v;
    CkSeries below = ck_ser_add(ck_ser_const(ck_c_num(ck_num_int(1))), ck_ser_scale(s, u0), 0);
    if (ck_ser_inverse(below, &z, 1) <= 0) return v;
    z = ck_ser_mul(w, z);
    return ck_near_series(ck_ser_add(ck_ser_const(angle), ck_ser_taylor(z, CK_TAYLOR_ATAN, ck_num_int(0), ck_goal * z.den), 0));
}

/* 14f-6, 14g-4. |v|. */
static CkNear ck_near_abs(CkNear v) {
    CkLead lead;
    CkSeries s = v.series;
    if (v.undefined) return v;
    int found = ck_ser_settle(&s, &lead);
    if (found == 0) { ck_unsupported("il segno dell'argomento di un valore assoluto non si sa stabilire"); return v; }
    if (found > 0 && lead.side < 0) s = ck_ser_scale(s, ck_c_num(ck_num_int(-1)));
    return ck_near_grows(v) ? ck_near_grown(s, v.growth) : ck_near_series(s);
}

/* 14d-3, 14g-4. v^{n} per n intero. */
static CkNear ck_near_power_int(CkNear v, long long n) {
    CkSeries result;
    if (v.undefined) return v;
    if (n == 0) return ck_near_const(ck_c_num(ck_num_int(1)));
    int done = ck_ser_power_int(v.series, n, &result);
    if (done < 0) return ck_near_nowhere();
    if (!done) return v;
    return ck_near_grows(v) ? ck_near_grown(result, ck_ser_scale(v.growth, ck_c_num(ck_num_int(n)))) : ck_near_series(result);
}

/* 14d-3, 14g-4. v^{q} per q razionale non intero; odd è 1 per una radice
 * di indice dispari. */
static CkNear ck_near_power(CkNear v, CkNum q, int odd) {
    CkSeries result;
    if (v.undefined) return v;
    int done = ck_ser_power(v.series, q, odd, &result);
    if (done < 0) return ck_near_nowhere();
    if (!done) return v;
    return ck_near_grows(v) ? ck_near_grown(result, ck_ser_scale(v.growth, ck_c_num(q))) : ck_near_series(result);
}

/* 14h. IL VALORE DI UN'ESPRESSIONE
 *
 * La variabile vale la sostituzione del lato in corso: a + t, a - t, t^{-1},
 * -t^{-1}, come serie esatte; oppure, salendo di un piano (D. Gruntz, il
 * "moving up"), x = e^{1/t} a +infinito, -e^{1/t} a -infinito, e^{-1/t} e
 * -e^{-1/t} da destra e da sinistra di 0: e^{G} per \pm 1, con G = \pm 1/t.
 * Anche queste sono biiezioni monotone da un (0, d') all'intorno del lato,
 * con x che tende al punto se e solo se t tende a 0^{+}: il limite non
 * cambia. Con x = e^{1/t} un \ln x vale 1/t e un \ln \ln x vale L: i
 * logaritmi scendono di un piano.
 *
 * Per il punto e per il valore dell'affermazione la variabile non c'è
 * (ck_numbers): il risultato deve essere una serie esatta costante. */

/* Il limite in corso: la variabile, la sua sostituzione, e 1 mentre si
 * calcola un numero del certificato, in cui la variabile non è ammessa. */
static char ck_lim_x;
static CkNear ck_lim_value;
static int ck_numbers;

/* Ritorna il valore di node vicino al punto, dal lato in corso. */
static CkNear ck_lim_eval(const CkNode *node) {
    CkNear a, b;
    CkNum q;
    if (ck_status != CK_ACCEPTED) return ck_near_nowhere();
    switch (node->kind) {
    case CK_NUMBER:
        return ck_near_const(ck_c_num(ck_num_int(node->value)));
    case CK_LETTER:
        if (ck_numbers || node->name != ck_lim_x) { ck_unsupported("nei limiti la sola lettera è la variabile, e un numero non la contiene"); return ck_near_nowhere(); }
        return ck_lim_value;
    case CK_PI:
        return ck_near_const(ck_c_var(ck_const_var('q', 0, 1)));
    case CK_NEG:
        return ck_near_binary(CK_SUB, ck_near_const(ck_c_num(ck_num_int(0))), ck_lim_eval(node->a));
    case CK_ADD: case CK_SUB: case CK_MUL: case CK_DIV:
        a = ck_lim_eval(node->a);
        b = ck_lim_eval(node->b);
        return ck_near_binary(node->kind, a, b);
    case CK_POW: {
        /* Un esponente costante razionale è una potenza (14d-3); ogni altro
         * vuol dire e^{b ln a}, definita dove a > 0. */
        b = ck_lim_eval(node->b);
        a = ck_lim_eval(node->a);
        CkSeries e = ck_ser_trim(b.series);
        int constant = !b.undefined && !ck_near_grows(b) && e.exact && (e.count == 0 || (e.count == 1 && e.low == 0));
        if (constant && ck_c_rational(e.count ? e.coef[0] : ck_c_num(ck_num_int(0)), &q)) {
            if (q.den != 1) return ck_near_power(a, q, 0);
            if (q.num > CK_MAX_POWER || q.num < -CK_MAX_POWER) { ck_unsupported("esponente troppo grande per il verificatore"); return a; }
            return ck_near_power_int(a, (long long)q.num);
        }
        if (a.undefined) return a;
        if (b.undefined) return b;
        return ck_near_exp(ck_near_binary(CK_MUL, b, ck_near_ln(a)));
    }
    case CK_RADICAL:
        /* (root m a): l'indice m è un intero scritto, da 2 in su, come nel
         * capitolo 13. */
        if (node->a->kind != CK_NUMBER || node->a->value < 2 || node->a->value > CK_MAX_EXPONENT) {
            ck_unsupported("radice con un indice che il verificatore non tratta");
            return ck_near_nowhere();
        }
        return ck_near_power(ck_lim_eval(node->b), ck_num_make(1, node->a->value), (int)(node->a->value % 2));
    case CK_EXP:
        return ck_near_exp(ck_lim_eval(node->a));
    case CK_LN:
        return ck_near_ln(ck_lim_eval(node->a));
    case CK_LNABS:
        return ck_near_ln(ck_near_abs(ck_lim_eval(node->a)));
    case CK_ABS:
        return ck_near_abs(ck_lim_eval(node->a));
    case CK_SIN: case CK_COS: case CK_TAN: case CK_COT:
        return ck_near_trig(ck_lim_eval(node->a), node->kind);
    case CK_ATAN:
        return ck_near_arctan(ck_lim_eval(node->a));
    case CK_ARCSIN: case CK_ARCCOS: {
        /* 14f-5: 2 arctan(u / (1 + (1 - u^2)^{1/2})), e pi/2 meno quello.
         * L'argomento si calcola una volta sola: riscritto come albero
         * comparirebbe due volte, e un arcoseno dentro l'altro costerebbe il
         * doppio a ogni livello. */
        CkNear u = ck_lim_eval(node->a), one = ck_near_const(ck_c_num(ck_num_int(1)));
        CkNear root = ck_near_power(ck_near_binary(CK_SUB, one, ck_near_power_int(u, 2)), ck_num_make(1, 2), 0);
        CkNear angle = ck_near_arctan(ck_near_binary(CK_DIV, u, ck_near_binary(CK_ADD, one, root)));
        a = ck_near_binary(CK_MUL, ck_near_const(ck_c_num(ck_num_int(2))), angle);
        if (node->kind == CK_ARCSIN) return a;
        return ck_near_binary(CK_SUB, ck_near_const(ck_c_mul(ck_c_var(ck_const_var('q', 0, 1)), ck_c_num(ck_num_make(1, 2)))), a);
    }
    default:
        ck_unsupported("il certificato usa una parola che i limiti non conoscono");
        return ck_near_nowhere();
    }
}

/* Mette in *value il numero scritto in node, senza la variabile, e ritorna 1;
 * ritorna 0, dopo averlo annotato, se non è un numero. */
static int ck_lim_number(const CkNode *node, CkRat *value) {
    ck_numbers = 1;
    ck_goal = 1;
    CkNear v = ck_lim_eval(node);
    ck_numbers = 0;
    CkSeries s = ck_ser_trim(v.series);
    if (ck_status != CK_ACCEPTED) return 0;
    if (v.undefined || ck_near_grows(v) || !s.exact || s.count > 1 || (s.count == 1 && s.low != 0)) { ck_reject("un numero del certificato non ha un valore"); return 0; }
    *value = s.count ? s.coef[0] : ck_c_num(ck_num_int(0));
    return 1;
}

/* 14i. IL LIMITE DA UN LATO
 *
 * Dal termine principale c(L) t^{e} della serie S di f, con c(L) ~ k
 * L^{d} (14b), e il resto O(t^{E}) con E > e: f = k t^{e} L^{d} (1 + o(1)),
 * perché ogni termine successivo e il resto, divisi per t^{e} L^{d}, sono
 * una potenza positiva di t per una di L, che tende a 0. Quindi f tende
 * all'infinito con il segno di k se e < 0, o e = 0 e d > 0; a 0 se e > 0, o
 * e = 0 e d < 0; a k se e = d = 0. Senza coefficienti noti, f = O(t^{E})
 * tende a 0 se E > 0; se E <= 0 l'ordine del certificato non basta, e il
 * certificato è respinto. Con il fattore e^{G}, 14g-1. */

/* Il limite da un lato. */
typedef enum { CK_SIDE_NOWHERE, CK_SIDE_FINITE, CK_SIDE_PLUS, CK_SIDE_MINUS } CkSideKind;

typedef struct {
    CkSideKind kind;
    CkRat value;   /* CK_SIDE_FINITE: il numero */
} CkSideLimit;

/* Ritorna il limite da un lato della funzione f. */
static CkSideLimit ck_side_limit(CkNear f) {
    CkSideLimit out = {CK_SIDE_FINITE, ck_c_num(ck_num_int(0))};
    CkSeries s = f.series;
    CkLead lead;
    int dir = 0;
    if (f.undefined) { out.kind = CK_SIDE_NOWHERE; return out; }

    /* 14g-1: con il fattore che va a 0 il limite è 0; con quello che va
     * all'infinito conta il segno di S. */
    if (ck_near_grows(f)) ck_growth_direction(f.growth, &dir);
    if (dir < 0) return out;
    int found = ck_ser_settle(&s, &lead);
    if (found == 0) ck_unsupported("il segno del primo coefficiente dello sviluppo non si sa stabilire");
    if (ck_status != CK_ACCEPTED) return out;
    if (found < 0) {
        if (dir == 0 && (s.exact || s.end > 0)) return out;
        ck_reject("lo sviluppo non ha un termine principale entro l'ordine dato");
        return out;
    }
    int e = s.low, d = ck_num_sign(lead.degree);
    if (dir > 0 || e < 0 || (e == 0 && d > 0)) { out.kind = lead.side > 0 ? CK_SIDE_PLUS : CK_SIDE_MINUS; return out; }
    if (e > 0 || d < 0) return out;
    out.value = ck_c_has_ell(s.coef[0]) ? lead.value : s.coef[0];
    return out;
}

/* 14j. LA LETTURA DEL CERTIFICATO E LA VERITÀ DI CIÒ CHE AFFERMA
 *
 * Con i limiti dei lati, ciò che claim dice si decide come in testa al
 * capitolo: due limiti finiti sono uguali se la loro differenza è zero (14a),
 * diversi se 14b lo prova; un infinito non è un numero, +infinito non è
 * -infinito. Se la verità che ne viene non è quella dichiarata il
 * certificato è respinto; se dipende da un confronto che non si sa fare non
 * è controllabile. */

/* Ritorna 1 se i limiti a e b sono uguali, 0 se sono diversi, -1 se non si
 * sa. */
static int ck_sides_equal(CkSideLimit a, CkSideLimit b) {
    if (a.kind != b.kind) return 0;
    return a.kind != CK_SIDE_FINITE ? 1 : ck_c_equal(a.value, b.value);
}

/* Le sostituzioni (14h): il nome nel certificato, il lato, e se è quella che
 * sale di un piano. */
static const struct { const char *word; int side; int moved; } ck_substitutions[] = {
    {"a+t", 1, 0}, {"a-t", -1, 0}, {"1/t", 2, 0}, {"-1/t", -2, 0},
    {"e^(-1/t)", 1, 1}, {"-e^(-1/t)", -1, 1}, {"e^(1/t)", 2, 1}, {"-e^(1/t)", -2, 1},
};

/* Legge e controlla un certificato limit, dalla riga dopo l'intestazione
 * fino a "end" compreso. */
static void ck_check_limit(void) {
    static const char *const claims[] = {"value", "plus", "minus", "unsigned", "none"};
    const CkNode *point = NULL, *function, *value = NULL;
    int infinity = 0, side = 0, claim = -1, truth, sides = 0, wanted[2], kinds[2] = {0, 0}, goals[2] = {1, 1};
    CkRat at = {{0, NULL}, {0, NULL}}, number = at;
    CkSideLimit limit[2] = {{CK_SIDE_NOWHERE, at}, {CK_SIDE_NOWHERE, at}};
    ck_var_count = 0;
    ck_sign_var = -1;
    ck_one = ck_poly_number(ck_num_int(1));

    /* La variabile, il punto, la funzione, ciò che si afferma. */
    ck_next_line();
    int named = ck_word("variable");
    ck_skip();
    if (!named || !isalpha((unsigned char)*ck_cursor) || isalnum((unsigned char)ck_cursor[1])) { ck_reject("certificato scritto male: manca la variabile"); return; }
    ck_lim_x = *ck_cursor++;
    ck_next_line();
    if (!ck_word("point")) { ck_reject("certificato scritto male: manca il punto"); return; }
    if (ck_word("+infinity")) infinity = 1;
    else if (ck_word("-infinity")) infinity = -1;
    else {
        point = ck_read();
        side = ck_word("right") ? 1 : ck_word("left") ? -1 : 0;
    }
    ck_next_line();
    if (!ck_word("function")) { ck_reject("certificato scritto male: manca la funzione"); return; }
    function = ck_read();
    ck_next_line();
    if (!ck_word("claim")) { ck_reject("certificato scritto male: manca ciò che si afferma"); return; }
    for (int i = 0; i < 5 && claim < 0; i++)
        if (ck_word(claims[i])) claim = i;
    if (claim < 0) { ck_reject("certificato scritto male: claim value, plus, minus, unsigned o none"); return; }
    if (claim == 0) value = ck_read();
    truth = ck_read_truth();
    if (ck_status != CK_ACCEPTED) return;

    /* I lati: quelli che il punto vuole, in quest'ordine. */
    wanted[0] = infinity ? 2 * infinity : side ? side : 1;
    wanted[1] = -1;
    int needed = infinity || side ? 1 : 2;
    for (ck_next_line(); ck_status == CK_ACCEPTED && ck_word("side"); ck_next_line()) {
        int kind = -1;
        long long goal = 0;
        int named_side = ck_word("right") ? 1 : ck_word("left") ? -1 : ck_word("infinity") ? 2 * infinity : 0;
        for (int i = 0; i < (int)(sizeof ck_substitutions / sizeof *ck_substitutions) && kind < 0; i++)
            if (ck_word(ck_substitutions[i].word)) kind = i;
        if (sides == needed || !named_side || named_side != wanted[sides]) { ck_reject("certificato scritto male: i lati non sono quelli del punto"); return; }
        if (kind < 0 || !ck_read_integer(&goal) || goal < 1 || goal > CK_MAX_GOAL) { ck_reject("certificato scritto male: un lato vuole la sostituzione e l'ordine"); return; }
        kinds[sides] = kind;
        goals[sides++] = (int)goal;
    }
    if (ck_status != CK_ACCEPTED) return;
    if (!ck_word("end")) { ck_reject("certificato scritto male: voce sconosciuta o \"end\" mancante"); return; }
    if (sides != needed) { ck_reject("certificato scritto male: i lati non sono quelli del punto"); return; }

    /* I numeri del certificato: il punto e il valore. */
    if (point && !ck_lim_number(point, &at)) return;
    if (value && !ck_lim_number(value, &number)) return;

    /* Ogni lato: la sostituzione deve portare quel lato del punto in t ->
     * 0^{+}; quella che sale di un piano, in un punto finito, vuole lo 0. */
    for (int i = 0; i < sides && ck_status == CK_ACCEPTED; i++) {
        int k = kinds[i], sign = ck_substitutions[k].side;
        if (sign != wanted[i] || (ck_substitutions[k].moved && point && ck_c_equal(at, ck_c_num(ck_num_int(0))) != 1)) {
            ck_reject("la sostituzione non porta quel lato del punto in t -> 0+");
            return;
        }
        ck_goal = goals[i];
        if (ck_substitutions[k].moved) {
            /* x = \pm e^{\pm 1/t}: e^{G} per \pm 1, con G = \pm t^{-1}. */
            CkSeries g = ck_ser_make(1, -1, 1, CK_FLAT, 1);
            g.coef[0] = ck_c_num(ck_num_int(point ? -1 : 1));
            ck_lim_value = ck_near(ck_ser_const(ck_c_num(ck_num_int(sign > 0 ? 1 : -1))), g, 0);
        } else if (!point) {
            /* x = 1/t o x = -1/t. */
            CkSeries s = ck_ser_make(1, -1, 1, CK_FLAT, 1);
            s.coef[0] = ck_c_num(ck_num_int(sign > 0 ? 1 : -1));
            ck_lim_value = ck_near_series(s);
        } else {
            /* x = a + t o x = a - t. */
            CkSeries s = ck_ser_make(1, 0, 2, CK_FLAT, 1);
            s.coef[0] = at;
            s.coef[1] = ck_c_num(ck_num_int(sign));
            ck_lim_value = ck_near_series(ck_ser_trim(s));
        }
        limit[i] = ck_side_limit(ck_lim_eval(function));
    }
    if (ck_status != CK_ACCEPTED) return;

    /* La verità di claim: dai lati con valori (14j). */
    int defined = 0, all = 1, some = 0, unknown = 0;
    for (int i = 0; i < sides; i++) {
        if (limit[i].kind == CK_SIDE_NOWHERE) continue;
        defined++;
        int here = claim == 0 ? (limit[i].kind == CK_SIDE_FINITE ? ck_c_equal(limit[i].value, number) : 0)
                 : claim == 1 ? limit[i].kind == CK_SIDE_PLUS
                 : claim == 2 ? limit[i].kind == CK_SIDE_MINUS
                 : claim == 3 ? limit[i].kind != CK_SIDE_FINITE : 1;
        if (here < 0) unknown = 1;
        all = all && here == 1;
        some = some || here == 0;
    }
    if (!defined) { ck_reject("da nessun lato la funzione ha valori: il limite non ha senso"); return; }
    int holds = all ? 1 : some ? 0 : -1;
    if (claim == 4) {
        int same = sides == 2 && defined == 2 ? ck_sides_equal(limit[0], limit[1]) : 1;
        holds = same < 0 ? -1 : !same;
    } else if (unknown && !some) {
        holds = -1;
    }
    if (holds < 0) { ck_unsupported("non si sa confrontare il limite calcolato con quello dell'affermazione"); return; }
    if (holds != truth) ck_reject("i limiti calcolati non danno ciò che il certificato afferma");
}

/* =============================================================================
 * 15. CONTROLLO DI UN CERTIFICATO
 * ============================================================================
 *
 * I tre controlli descritti in testa al file, più le condizioni che rendono
 * lecito trattare somme e successioni come lettere.
 */

/* Legge le voci del certificato che comincia in ck_cursor, fino a "end"
 * compreso. Una voce sconosciuta o mancante rende il certificato sbagliato. */
static void ck_read_certificate(CkCertificate *cert) {
    static const char *const twice = "certificato scritto male: una voce compare due volte";
    int seen_variable = 0, seen_base = 0, ended = 0, recurrence;
    memset(cert, 0, sizeof *cert);
    while (isspace((unsigned char)*ck_cursor)) ck_cursor++;
    if (!ck_word("certificate")) { ck_reject("certificato scritto male: manca l'intestazione"); return; }

    /* Un enunciato sugli insiemi: due voci sole, statement ed end. */
    if (ck_word("sets")) {
        while (isspace((unsigned char)*ck_cursor)) ck_cursor++;
        if (!ck_word("statement")) { ck_reject("certificato scritto male: manca l'enunciato"); return; }
        cert->sets = ck_read();
        while (isspace((unsigned char)*ck_cursor)) ck_cursor++;
        if (!ck_word("end")) ck_reject("certificato scritto male: voce sconosciuta o \"end\" mancante");
        return;
    }

    /* Un enunciato su insiemi qualsiasi: lo si controlla mentre lo si legge
     * (capitolo 11). */
    static const char *const structures[] = {"counterexample", "example", "structure"};
    for (int kind = 0; kind < 3; kind++)
        if (ck_word(structures[kind])) {
            cert->checked = 1;
            ck_check_structure(kind);
            return;
        }

    /* Una tabella dei segni: anche lei si controlla mentre la si legge
     * (capitolo 12). */
    if (ck_word("signs")) {
        cert->checked = 1;
        ck_check_signs();
        return;
    }

    /* Una derivata, una primitiva, un integrale (capitolo 13). */
    static const char *const calculus[] = {"derivative", "antiderivative", "integral"};
    for (int kind = 0; kind < 3; kind++)
        if (ck_word(calculus[kind])) {
            cert->checked = 1;
            ck_check_calculus(kind);
            return;
        }

    /* Un limite (capitolo 14). */
    if (ck_word("limit")) {
        cert->checked = 1;
        ck_check_limit();
        return;
    }
    if (!ck_word("induction")) { ck_reject("certificato scritto male: manca l'intestazione"); return; }
    while (ck_status == CK_ACCEPTED && !ended) {
        while (isspace((unsigned char)*ck_cursor)) ck_cursor++;
        if (ck_word("end")) {
            ended = 1;
        } else if (ck_word("variable")) {
            if (seen_variable) ck_reject(twice);
            ck_skip();
            cert->variable = *ck_cursor;
            seen_variable = isalpha((unsigned char)*ck_cursor);
            if (seen_variable) ck_cursor++;
        } else if (ck_word("base")) {
            if (seen_base) ck_reject(twice);
            seen_base = ck_read_integer(&cert->base);
            if (cert->base > CK_MAX_BASE || cert->base < -CK_MAX_BASE) ck_unsupported("caso base troppo grande per il verificatore");
        } else if ((recurrence = ck_word("sequence")) || ck_word("define")) {
            /* "sequence": nome, ordine, coefficienti, valori iniziali.
             * "define": nome, parametro, formula. */
            CkSequence *s = &ck_sequences[ck_sequence_count];
            long long order = 0;
            ck_skip();
            if (ck_sequence_count == CK_MAX_SEQUENCES || !isalpha((unsigned char)*ck_cursor)) { ck_unsupported("troppe successioni, o voce scritta male"); return; }
            memset(s, 0, sizeof *s);
            s->name = *ck_cursor++;
            for (int i = 0; i < ck_sequence_count; i++)
                if (ck_sequences[i].name == s->name) ck_reject(twice);
            ck_skip();
            if (!recurrence) {
                if (!isalpha((unsigned char)*ck_cursor)) { ck_reject("certificato scritto male: lettera attesa"); return; }
                s->parameter = *ck_cursor++;
                s->formula = ck_read();
            } else {
                if (!ck_read_integer(&order) || order < 1 || order > CK_MAX_ORDER) { ck_unsupported("ordine di ricorrenza che il verificatore non tratta"); return; }
                s->order = (int)order;
                for (int i = 0; i < 2 * s->order; i++)
                    if (!ck_read_integer(i < s->order ? &s->coef[i] : &s->init[i - s->order])) { ck_reject("certificato scritto male: ricorrenza incompleta"); return; }
            }
            ck_sequence_count++;
        } else if (ck_word("statement")) {
            /* (= L R): l'unica relazione ammessa. */
            if (cert->left) ck_reject(twice);
            ck_skip();
            if (strncmp(ck_cursor, "(=", 2) != 0) { ck_unsupported("enunciato che non è un'uguaglianza"); return; }
            ck_cursor += 2;
            cert->left = ck_read();
            cert->right = ck_read();
            ck_skip();
            if (*ck_cursor != ')') ck_reject("certificato scritto male: parentesi chiusa attesa");
            else ck_cursor++;
        } else if (ck_word("factor")) {
            if (cert->factor) ck_reject(twice);
            cert->factor = ck_read();
        } else if (ck_word("defined")) {
            /* Coppie "lettera valore", fino alla fine della riga. */
            if (cert->defined) ck_reject(twice);
            cert->defined = 1;
            for (ck_skip(); isalpha((unsigned char)*ck_cursor) && ck_status == CK_ACCEPTED; ck_skip()) {
                if (ck_fixed_count == CK_MAX_POINT) { ck_unsupported("troppi parametri con un valore"); return; }
                ck_fixed[ck_fixed_count].name = *ck_cursor++;
                ck_fixed[ck_fixed_count++].tree = ck_read();
            }
        } else if (ck_word("generic")) {
            if (cert->generics == CK_MAX_GENERIC) { ck_unsupported("troppe voci generic"); return; }
            cert->generic[cert->generics++] = ck_read();
        } else if (ck_word("brick")) {
            if (cert->bricks == CK_MAX_BRICKS) { ck_unsupported("troppe voci brick"); return; }
            cert->brick[cert->bricks++] = ck_read();
        } else {
            ck_reject("certificato scritto male: voce sconosciuta o \"end\" mancante");
        }
    }
    if (ck_status == CK_ACCEPTED && (!seen_variable || !seen_base || !cert->left || !cert->factor))
        ck_reject("certificato incompleto: servono variable, base, statement e factor");

    /* Un valore si dà a un parametro, una volta sola: non alla variabile di
     * induzione. */
    for (int i = 0; i < ck_fixed_count; i++) {
        if (ck_fixed[i].name == cert->variable) ck_reject("certificato scritto male: la voce defined dà un valore alla variabile di induzione");
        for (int j = 0; j < i; j++)
            if (ck_fixed[j].name == ck_fixed[i].name) ck_reject(twice);
    }

    /* La formula di una voce "define" può nominare una lettera sola: il suo
     * parametro, che non deve essere la variabile di induzione.
     *
     * Una somma si può trattare come una lettera solo se il suo termine non
     * cambia al variare di n (8b), e quel controllo guarda com'è scritto il
     * termine: in "define G m (* n m)" la n è nascosta dietro il nome G, e
     * la somma dei G(k) passerebbe per una somma qualunque. Era un
     * certificato falso accettato: ora è nei test. Vietare ogni altra
     * lettera, e non solo n, toglie anche il dubbio su che cosa indichi
     * nella formula una lettera che altrove è l'indice di una somma. */
    for (int i = 0; i < ck_sequence_count && ck_status == CK_ACCEPTED; i++) {
        const CkSequence *s = &ck_sequences[i];
        for (int letter = 0; letter < 128 && s->order == 0; letter++)
            if (isalpha(letter) && (letter != s->parameter || letter == cert->variable) && ck_mentions(s->formula, (char)letter))
                ck_unsupported("una voce define nomina una lettera che non è il suo parametro");
    }
}

/* Ritorna L - R con la variabile di induzione che vale n + shift. */
static CkRat ck_difference(const CkCertificate *cert, long long shift) {
    ck_numeric = 0;
    ck_shift = shift;
    return ck_rat_add(ck_eval(cert->left), ck_eval(cert->right), 1);
}

/* Fa i conti del certificato e i suoi controlli. È chiamata due volte: la
 * prima con ck_trial a 1, solo per fissare i riferimenti (capitolo 7). */
static void ck_verify(const CkCertificate *cert) {
    /* I fattori suggeriti per i logaritmi (8g): polinomi non costanti. Sono
     * solo suggerimenti: uno sbagliato non divide nulla e non ha effetto. */
    ck_numeric = 0;
    ck_shift = 0;
    ck_brick_count = 0;
    for (int i = 0; i < cert->bricks && ck_status == CK_ACCEPTED; i++) {
        CkRat brick = ck_eval(cert->brick[i]);
        CkNum lead;
        if (!ck_poly_is_constant(brick.den) || ck_poly_is_constant(brick.num)) { ck_reject("una voce brick non è un polinomio non costante"); return; }
        ck_bricks[ck_brick_count++] = ck_poly_monic(brick.num, &lead);
    }

    /* D(n) = N0/D0, D(n+1) = N1/D1, W = Wn/Wd. */
    CkRat now = ck_difference(cert, 0), next = ck_difference(cert, 1);
    ck_shift = 0;
    CkRat factor = ck_eval(cert->factor);

    /* G: i denominatori, le espressioni "generic", e le lettere p^n con p
     * primo, che non valgono mai zero. Una "generic" deve essere un
     * polinomio non nullo nei soli parametri: è un'ipotesi in più del
     * teorema, e non deve poter nascondere una condizione su n. */
    CkPoly nonzero = ck_poly_mul(now.den, next.den);
    for (int i = 0; i < cert->generics && ck_status == CK_ACCEPTED; i++) {
        CkRat g = ck_eval(cert->generic[i]);
        int plain = g.num.count > 0;
        for (int v = 0; v < ck_var_count; v++)
            if ((ck_vars[v].kind != CK_VAR_LETTER || ck_vars[v].name == ck_variable) && (ck_poly_uses(g.num, v) || ck_poly_uses(g.den, v))) plain = 0;
        if (!plain) { ck_reject("una voce generic non è un'espressione non nulla nei soli parametri"); return; }
        nonzero = ck_poly_mul(nonzero, g.num);
    }
    for (int v = 0; v < ck_var_count; v++)
        if (ck_vars[v].kind == CK_VAR_EXP && ck_vars[v].prime) nonzero = ck_poly_mul(nonzero, ck_poly_variable(v));

    /* Caso base: con n = base, e i parametri come lettere. */
    ck_numeric = 1;
    ck_point = cert->base;
    CkRat start = ck_rat_add(ck_eval(cert->left), ck_eval(cert->right), 1);
    ck_numeric = 0;
    if (cert->defined) ck_defined_divisors(cert);
    if (ck_trial || ck_status != CK_ACCEPTED) return;

    /* Le lettere stanno per quantità vere solo se queste esistono per ogni
     * n dal caso base in su: una somma fino a un estremo almeno pari a
     * quello inferiore meno uno (la somma vuota), un termine di successione
     * di indice non negativo. */
    for (int v = 0; v < ck_var_count; v++) {
        const CkVar *x = &ck_vars[v];
        long long first = x->dilation * cert->base + x->offset;
        if ((x->kind == CK_VAR_BIG && first < x->lower - 1) || (x->kind == CK_VAR_SEQ && first < 0)) {
            ck_unsupported("una somma o una successione comincia dopo il suo ultimo termine, nel caso base");
            return;
        }
    }

    /* Controllo 1. */
    if (start.num.count != 0) { ck_reject("il caso base non vale: i due membri sono diversi"); return; }

    /* Controllo 2: N1 D0 Wd = Wn N0 D1. */
    CkPoly left = ck_poly_mul(ck_poly_mul(next.num, now.den), factor.den);
    CkPoly right = ck_poly_mul(ck_poly_mul(factor.num, now.num), next.den);
    if (ck_status == CK_ACCEPTED && !ck_poly_equal(left, right)) { ck_reject("il passo induttivo non vale: il fattore non lega i due membri"); return; }

    /* Controllo 3: Wd divide una potenza di G.
     *
     * La lettera s vale 1 per n pari e -1 per n dispari, e mai altro: il
     * controllo si fa due volte, con il numero al posto di s. Così restano
     * polinomi veri, senza la regola s^2 = 1, per i quali la divisione è
     * quella di scuola. Se con uno dei due valori Wd diventa il polinomio
     * nullo, si annulla per metà degli n: respinto. */
    for (int sign = 1; sign >= -1 && ck_status == CK_ACCEPTED; sign -= 2) {
        CkPoly below = ck_poly_at_sign(factor.den, sign), factors = ck_poly_at_sign(nonzero, sign);
        CkPoly power = ck_poly_number(ck_num_int(1));
        int divides = 0;
        for (int j = 0; j <= CK_MAX_NONZERO && below.count > 0 && !divides && ck_status == CK_ACCEPTED; j++) {
            divides = ck_poly_divides(below, power, NULL);
            power = ck_poly_mul(power, factors);
        }
        if (ck_status == CK_ACCEPTED && !divides) ck_reject("il fattore ha un denominatore che può annullarsi dove l'enunciato è definito");
    }

    /* La voce "defined", per ultima: prima ciò che vale per ogni valore dei
     * parametri, poi ciò che vale nel punto. */
    if (cert->defined && ck_status == CK_ACCEPTED) ck_defined_decide();
}

/* Controlla il certificato che comincia in *text e porta *text subito dopo
 * il suo "end". Ritorna la risposta, e mette in *why il motivo quando non è
 * CK_ACCEPTED. È la sola funzione che chi include questo file deve
 * chiamare. */
static CkAnswer ck_check(const char **text, const char **why) {
    CkCertificate cert;
    ck_status = CK_ACCEPTED;
    ck_reason = "";
    ck_used = 0;
    ck_sequence_count = 0;
    ck_env_count = 0;
    ck_depth = 0;
    ck_fixed_count = ck_fixing = ck_recording = ck_divisor_count = 0;
    ck_real = 0;
    ck_cursor = *text;
    ck_read_certificate(&cert);
    *text = ck_cursor;

    /* Un enunciato sugli insiemi si decide con un conto suo. */
    if (ck_status == CK_ACCEPTED && cert.sets) ck_check_sets(cert.sets);

    /* Un'induzione: la valutazione di prova e quella vera. Tra l'una e
     * l'altra restano le indeterminate con i loro riferimenti; i polinomi si
     * ricalcolano. */
    for (int pass = 0; pass < 2 && ck_status == CK_ACCEPTED && !cert.sets && !cert.checked; pass++) {
        ck_trial = pass == 0;
        if (pass == 0) {
            ck_var_count = 0;
            ck_sign_var = -1;
            ck_variable = cert.variable;
            ck_var_letter(ck_variable);
        }
        ck_verify(&cert);
    }
    *why = ck_reason;
    return ck_status;
}

/* =============================================================================
 * 16. MAIN
 * ============================================================================
 *
 * Il verificatore come programma a sé: legge un file di certificati, uno
 * dopo l'altro, e dice di ognuno se lo accetta. Il codice di uscita è 0 se
 * li ha accettati tutti e ce n'era almeno uno, 1 altrimenti, 2 se il file
 * non si legge. Chi include questo file definisce CK_NO_MAIN, e usa solo
 * ck_check().
 */

#ifndef CK_NO_MAIN
int main(int argc, char **argv) {
    int accepted = 0, others = 0;
    if (argc != 2) { fprintf(stderr, "uso: %s <file di certificati>\n", argv[0]); return 2; }

    /* Tutto il file in memoria, con uno zero in fondo. */
    FILE *in = fopen(argv[1], "rb");
    long size = -1;
    if (in && fseek(in, 0, SEEK_END) == 0) size = ftell(in);
    char *text = size < 0 ? NULL : malloc((size_t)size + 1);
    if (!text || fseek(in, 0, SEEK_SET) != 0 || fread(text, 1, (size_t)size, in) != (size_t)size) {
        fprintf(stderr, "non riesco a leggere %s\n", argv[1]);
        return 2;
    }
    fclose(in);
    text[size] = 0;

    /* Il testo si legge fino al primo zero: un file che ne contiene uno in
     * mezzo verrebbe controllato solo in parte, senza dirlo. */
    if (strlen(text) != (size_t)size) { fprintf(stderr, "%s contiene un byte nullo: non è un file di certificati\n", argv[1]); return 2; }

    /* Un certificato dopo l'altro. Dopo uno scritto male si riparte dal
     * prossimo "certificate". */
    const char *cursor = text;
    for (int number = 1; ; number++) {
        const char *why, *here = strstr(cursor, "certificate");
        if (!here) break;
        cursor = here;
        CkAnswer answer = ck_check(&cursor, &why);
        if (cursor == here) cursor = here + 1;
        if (answer == CK_ACCEPTED) { printf("%4d. ACCETTATO\n", number); accepted++; }
        else { printf("%4d. %s: %s\n", number, answer == CK_REJECTED ? "RESPINTO" : "NON CONTROLLABILE", why); others++; }
    }
    printf("Riepilogo: %d certificati accettati, %d no.\n", accepted, others);
    free(text);

    /* Un file senza certificati non è un successo: non ha dimostrato nulla. */
    return (accepted > 0 && others == 0) ? 0 : 1;
}
#endif
