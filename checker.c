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
 * cui si controlla che portino a una contraddizione (capitolo 11). Il resto
 * di questo commento riguarda le induzioni; i teoremi degli altri tipi
 * sono più sotto. Questo programma legge il certificato e risponde:
 *
 *   - ACCETTATO: tutti i controlli sono riusciti;
 *   - RESPINTO: un controllo è fallito, il certificato non dimostra nulla;
 *   - NON CONTROLLABILE: il certificato usa qualcosa che questo programma non
 *     conosce, o i numeri sono troppo grandi. Non è un sì.
 *
 * Trovare una dimostrazione è difficile, controllarla è facile: qui non c'è
 * nessuna ricerca, solo somme e prodotti di polinomi e il confronto di due
 * risultati. Per questo il programma è piccolo, e lo si può leggere tutto.
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
 *     relazione).
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
 *   12. Controllo di un certificato
 *   13. main
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
        ck_reject(ck_recording ? "con i valori dati ai parametri l'enunciato divide per zero"
                  : ck_numeric ? "nel caso base l'enunciato divide per zero"
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
    CK_TRUE, CK_FORALL, CK_EXISTS, CK_APP, CK_VAR, CK_ATOM
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
 *   EQ: a = b tra due termini; con b NULL, a è una parte o una relazione
 *       applicata, e la condizione è che valga.
 *   ATOM: value è una condizione elementare (11e), in uno scheletro. */
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
 *
 * Il riferimento. Per ogni somma serve l'estremo più basso con cui compare,
 * così ogni altra occorrenza si ottiene aggiungendo termini e mai
 * togliendone (togliere un fattore a un prodotto sarebbe dividere per un
 * fattore che può valere zero). Per trovarlo ogni certificato viene valutato
 * due volte: la prima, di prova, serve solo ad abbassare i riferimenti; la
 * seconda fa i conti veri, e non deve più trovarne uno da abbassare.
 */

typedef enum { CK_VAR_LETTER, CK_VAR_BIG, CK_VAR_EXP, CK_VAR_SIGN, CK_VAR_SEQ, CK_VAR_LN } CkVarKind;

/* Un'indeterminata. I campi usati dipendono da kind:
 *   LETTER: name.
 *   BIG:    node è la somma o il prodotto, dilation e offset dicono il
 *           riferimento dilation * n + offset, lower è l'estremo inferiore.
 *   EXP, LN: prime è il numero primo che fa da base, oppure 0 e allora la
 *           base è il polinomio `base`, nell'arena.
 *   SEQ:    name è la successione, dilation e offset come per BIG, position
 *           è il j di X(dilation * n + offset + j).
 * stated vale 1 per una somma o un prodotto che compare nell'enunciato
 * calcolato con i parametri fissati (capitolo 9), e non solo nel fattore. */
typedef struct {
    CkVarKind kind;
    char name;
    const CkNode *node;
    long long dilation, offset, lower;
    long long prime;
    CkPoly base;
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
        int parameter = ck_vars[v].kind == CK_VAR_LETTER && ck_vars[v].name != ck_variable;
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
 *         se è s = (-1)^n non dà nulla, perché |s| = 1;
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
    CkPoly result = ck_poly_number(ck_num_int(0)), n = ck_poly_variable(ck_var_letter(ck_variable));
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

        /* ln|lettera|, secondo il tipo. */
        CkPoly one;
        if (x->kind == CK_VAR_SIGN) continue;
        if (x->kind == CK_VAR_EXP && x->prime) one = ck_poly_mul(n, ck_poly_variable(ck_var_of_base(CK_VAR_LN, x->prime, none)));
        else if (x->kind == CK_VAR_EXP) one = ck_poly_mul(n, ck_ln_poly(x->base, depth + 1));
        else one = ck_poly_variable(ck_var_of_base(CK_VAR_LN, 0, ck_poly_variable(v)));
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
 * Finché l'argomento è scritto come un prodotto, un quoziente, un opposto o
 * una potenza a esponente intero si applicano le regole di 8g a com'è
 * scritto, senza svolgere i conti: di (n + 1)^3 si prende 3 ln|n + 1|,
 * invece di sviluppare il cubo e doverlo poi scomporre di nuovo. Il resto si
 * calcola, e va a ck_ln_poly(): numeratore meno denominatore. Per "è
 * definito per ogni n" (capitolo 9) ogni pezzo calcolato conta come un
 * divisore: non deve annullarsi. */
static CkPoly ck_ln_node(const CkNode *node) {
    long long a, c;
    if (ck_status != CK_ACCEPTED) return ck_poly_number(ck_num_int(0));
    if (node->kind == CK_MUL || node->kind == CK_DIV) return ck_poly_add(ck_ln_node(node->a), ck_ln_node(node->b), node->kind == CK_DIV);
    if (node->kind == CK_NEG) return ck_ln_node(node->a);
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
    int structure;                      /* 1 per un certificato del capitolo 11, già controllato mentre lo si leggeva */
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
 *
 * In una condizione, (nome termine...) è una parte o una relazione
 * applicata: "x sta in X", "(x, y) sta in R". Un nome è la variabile legata
 * più interna che si chiama così, altrimenti un simbolo. Il lettore
 * controlla gli insiemi: f(x) solo con x nel dominio di f, un'uguaglianza
 * solo tra due elementi dello stesso insieme.
 *
 * Un'interpretazione dà a ogni insieme dei suoi elementi, quanti si vuole,
 * anche infiniti, anche nessuno; a ogni simbolo un elemento, una funzione o
 * un sottoinsieme, come dice la dichiarazione. I certificati sono di tre
 * tipi, e ognuno dimostra un teorema diverso.
 *
 *   11a. "certificate counterexample", con una voce "table" per simbolo:
 *        un'interpretazione finita in cui l'enunciato è falso. TEOREMA: non
 *        è vero che l'enunciato vale per ogni scelta degli insiemi e dei
 *        simboli.
 *   11b. "certificate example", con le stesse voci: un'interpretazione in
 *        cui l'enunciato è vero. TEOREMA: esistono degli insiemi, con il
 *        numero di elementi indicato, e dei simboli per cui vale.
 *   11c. "certificate structure", con le voci case, instance e witness
 *        (11d). TEOREMA: l'enunciato vale per ogni scelta degli insiemi,
 *        finiti o infiniti, anche vuoti, e dei simboli.
 *
 * Per 11a e 11b si calcola: con insiemi finiti un quantificatore prova ogni
 * elemento, un simbolo si legge nella sua tabella, e il valore
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
 * voce witness (11d), che vale solo nel suo caso. */
typedef struct {
    char name[CK_MAX_NAME];
    int arity;    /* 0, 1 o 2 argomenti */
    int arg[2];   /* gli insiemi degli argomenti: posti in ck_sorts[] */
    int result;   /* l'insieme del valore, o CK_BOOL */
    int witness;  /* 1 per un nome dato da witness */
    int *table;   /* 11a, 11b: il valore di ogni cella, nell'arena; NULL finché manca la sua voce table */
} CkSymbol;

static CkSort ck_sorts[CK_MAX_SORTS];
static int ck_sort_count;
static CkSymbol ck_symbols[CK_MAX_SYMBOLS];
static int ck_symbol_count;
static int ck_declared;  /* i simboli dichiarati: i primi ck_declared di ck_symbols[] */

/* Le variabili legate visibili mentre si legge, dalla più esterna, con i
 * loro insiemi. */
static char ck_bound[CK_MAX_BOUND][CK_MAX_NAME];
static int ck_bound_sort[CK_MAX_BOUND];
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

    /* Una variabile: senza parentesi, la più interna con quel nome. */
    for (int i = ck_bound_count - 1; i >= 0 && !open; i--)
        if (strcmp(ck_bound[i], name) == 0) {
            node->kind = CK_VAR;
            node->value = ck_bound_count - 1 - i;
            *sort = ck_bound_sort[i];
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
        {"forall", CK_FORALL, 0}, {"exists", CK_EXISTS, 0},
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
             * mentre si legge c. */
            char set[CK_MAX_NAME];
            if (ck_bound_count == CK_MAX_BOUND) { ck_unsupported("troppe variabili legate una dentro l'altra"); return node; }
            if (!ck_name(ck_bound[ck_bound_count]) || !ck_name(set) || (node->value = ck_sort_of(set)) < 0) {
                ck_reject("certificato scritto male: dopo forall ed exists servono una variabile e un insieme dichiarato");
                return node;
            }
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
 * elementi i quantificatori hanno provato. */
static int ck_values[CK_MAX_BOUND];
static long long ck_tried;

/* Ritorna il valore del termine t nel modello, con depth variabili legate. */
static int ck_value(const CkNode *t, int depth) {
    if (t->kind == CK_VAR) return ck_values[depth - 1 - t->value];
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
    int every = c->kind == CK_FORALL;
    for (int x = 0; x < ck_sorts[c->value].size && ck_status == CK_ACCEPTED; x++) {
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
 * conto. */

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
    if (c->kind != CK_EQ && c->kind != CK_FORALL && c->kind != CK_EXISTS) {
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

/* Ritorna, come bit, gli insiemi dei nomi dati da witness che compaiono
 * nell'albero t. */
static unsigned ck_names(const CkNode *t) {
    if (!t) return 0;
    unsigned here = t->kind == CK_APP && ck_symbols[t->value].witness ? 1u << ck_symbols[t->value].result : 0;
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
    copy->a = ck_subst(c->a, depth + (c->kind == CK_FORALL || c->kind == CK_EXISTS), t);
    copy->b = ck_subst(c->b, depth, t);
    return copy;
}

/* Legge la condizione Q di una voce instance o witness. Deve cominciare con
 * un quantificatore ed essere già comparsa nel caso, come condizione
 * elementare. */
static const CkNode *ck_read_available(void) {
    const CkNode *q = ck_read_condition();
    if (ck_status != CK_ACCEPTED) return q;
    if (q->kind != CK_FORALL && q->kind != CK_EXISTS) { ck_reject("instance e witness vogliono una condizione che comincia con un quantificatore"); return q; }
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

/* La voce instance Q t1 ... tk (11d). */
static void ck_instance(void) {
    const CkNode *q = ck_read_available(), *body = q;
    unsigned mask = ck_names(q);
    int count = 0;
    for (ck_skip(); ck_status == CK_ACCEPTED && *ck_cursor && *ck_cursor != '\n'; ck_skip(), count++) {
        int sort;
        const CkNode *t = ck_read_term(&sort);
        if (body->kind != q->kind || sort != body->value) { ck_reject("un'istanza ha un termine di troppo, o in un altro insieme"); return; }
        mask |= ck_names(t);
        body = ck_subst(body->a, 0, t);
    }
    if (ck_status != CK_ACCEPTED) return;
    if (count == 0) { ck_reject("un'istanza senza termini"); return; }
    ck_suppose(q->kind == CK_FORALL ? ck_implies(q, body) : ck_implies(body, q), mask);
}

/* La voce witness Q c (11d). */
static void ck_witness(void) {
    const CkNode *q = ck_read_available();
    if (ck_status != CK_ACCEPTED) return;
    if (ck_symbol_count == CK_MAX_SYMBOLS) { ck_unsupported("troppi nomi in un caso"); return; }
    CkSymbol *c = &ck_symbols[ck_symbol_count];
    memset(c, 0, sizeof *c);
    if (!ck_name(c->name) || ck_symbol_of(c->name) >= 0) { ck_reject("witness: serve un nome che nel caso non c'è ancora"); return; }
    c->result = (int)q->value;
    c->witness = 1;
    CkNode *t = ck_node(CK_APP);
    t->value = ck_symbol_count++;
    const CkNode *body = ck_subst(q->a, 0, t);
    ck_suppose(q->kind == CK_EXISTS ? ck_implies(q, body) : ck_implies(body, q), ck_names(q) | 1u << q->value);
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
    return empty >> atom->quantified->value & 1 ? atom->quantified->kind == CK_FORALL : -1;
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
 * 12. CONTROLLO DI UN CERTIFICATO
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
            cert->structure = 1;
            ck_check_structure(kind);
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
    ck_cursor = *text;
    ck_read_certificate(&cert);
    *text = ck_cursor;

    /* Un enunciato sugli insiemi si decide con un conto suo. */
    if (ck_status == CK_ACCEPTED && cert.sets) ck_check_sets(cert.sets);

    /* Un'induzione: la valutazione di prova e quella vera. Tra l'una e
     * l'altra restano le indeterminate con i loro riferimenti; i polinomi si
     * ricalcolano. */
    for (int pass = 0; pass < 2 && ck_status == CK_ACCEPTED && !cert.sets && !cert.structure; pass++) {
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
 * 13. MAIN
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
