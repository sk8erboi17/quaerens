/* =============================================================================
 * checker: verificatore indipendente dei certificati di mathresearcher
 * =============================================================================
 *
 * COSA FA
 *
 * mathresearcher (main.c) cerca le dimostrazioni. Quando ne trova una per
 * induzione scrive un certificato: l'enunciato e un solo dato in più, che
 * basta a ricontrollare il passo induttivo senza rifare la ricerca. Questo
 * programma legge il certificato e risponde:
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
 * (per ricorrenza, o con una formula), e "generic", descritta più sotto.
 *
 * IL TEOREMA CHE UN CERTIFICATO ACCETTATO DIMOSTRA
 *
 * Sia L = R l'enunciato, n la variabile, b il caso base. Diciamo che
 * l'enunciato è definito per un valore di n (e dei parametri) se, nel
 * calcolo dei due membri,
 *
 *   - nessuna divisione ha il divisore nullo (una potenza con esponente
 *     negativo è una divisione), e
 *   - nessuna potenza con l'esponente che dipende da n ha la base nulla.
 *
 * Il teorema:
 *
 *     Per ogni intero n >= b e per ogni valore dei parametri tali che
 *       (a) l'enunciato è definito per tutti i valori da b a n, e
 *       (b) ogni espressione "generic" è definita e diversa da zero,
 *     vale L = R.
 *
 * Il certificato NON dice che l'enunciato è definito: quella è un'ipotesi
 * del teorema, e resta da stabilire a chi lo usa. (n - 50)/(n - 50) = 1 ha
 * un certificato valido: dice che i due membri sono uguali per n da 1 a 49.
 * Da 50 in su il teorema non dice più nulla, perché in 50 l'enunciato non è
 * definito.
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
 * Le somme e le potenze. Una somma fino a n non ha una formula, ma non
 * serve: la si tratta come una lettera X, e la somma fino a n + 1 è X più il
 * termine nuovo. Lo stesso per un prodotto, per 2^n (con 2^{n+1} = 2 2^n) e
 * per i termini di una ricorrenza. Un'uguaglianza tra polinomi in cui X è
 * una lettera qualsiasi vale in particolare quando X è la somma vera: per
 * questo trattare queste quantità come indipendenti non può far accettare
 * il falso. Può far respingere il vero, se la dimostrazione usa un legame
 * tra loro che qui non si vede.
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
 *    9. Controllo di un certificato
 *   10. main
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
 * a = b q. b non è il polinomio nullo, e nessuno dei due contiene
 * l'indeterminata s.
 *
 * Divisione dei polinomi in più indeterminate: l'ultimo termine di un
 * polinomio, nell'ordine dei monomi, è il suo termine direttivo, e il termine
 * direttivo di un prodotto è il prodotto dei termini direttivi. Finché il
 * resto non è zero, il suo termine direttivo deve essere un multiplo di
 * quello di b: lo si toglie sottraendo il multiplo giusto di b, e si
 * continua. Se a un certo punto non è un multiplo, b non divide a.
 *
 * A ogni passo il termine direttivo del resto scende, quindi il ciclo
 * finisce. Il quoziente viene comunque ricontrollato moltiplicando: così la
 * risposta "divide" non dipende dal ragionamento sul ciclo, solo da un
 * prodotto e da un confronto. */
static int ck_poly_divides(CkPoly b, CkPoly a) {
    CkPoly rest = a, quotient = ck_poly_number(ck_num_int(0));
    const CkTerm *lead = &b.term[b.count - 1];
    while (rest.count > 0 && ck_status == CK_ACCEPTED) {
        CkTerm *piece = ck_alloc(sizeof *piece);
        const CkTerm *top = &rest.term[rest.count - 1];
        for (int v = 0; v < CK_MAX_VARS; v++) {
            if (top->e[v] < lead->e[v]) return 0;
            piece->e[v] = (signed char)(top->e[v] - lead->e[v]);
        }
        piece->coef = ck_num_mul(top->coef, ck_num_inv(lead->coef));
        CkPoly single = {1, piece};
        quotient = ck_poly_add(quotient, single, 0);
        rest = ck_poly_add(rest, ck_poly_mul(single, b), 1);
    }
    return ck_status == CK_ACCEPTED && ck_poly_equal(ck_poly_mul(b, quotient), a);
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
        ck_reject("l'enunciato divide per un'espressione che vale sempre zero");
        return ck_rat_int(0);
    }
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
 *
 * Un intero può avere il segno meno davanti. Una lettera è un solo
 * carattere: la variabile di induzione, un parametro, o un indice di somma.
 */

/* Tipi di nodo. */
typedef enum { CK_NUMBER, CK_LETTER, CK_ADD, CK_SUB, CK_MUL, CK_DIV, CK_NEG, CK_POW, CK_SUM, CK_PROD, CK_FACT, CK_BINOM, CK_SEQ } CkKind;

/* Un nodo dell'albero di un'espressione. I nodi stanno nell'arena.
 *   NUMBER: value.            LETTER: name.
 *   ADD, SUB, MUL, DIV, POW, BINOM: i due operandi a e b.   NEG, FACT: a.
 *   SUM, PROD: name è l'indice, a e b gli estremi, c il termine.
 *   SEQ: name è la successione, a l'indice. */
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
 *
 * Il riferimento. Per ogni somma serve l'estremo più basso con cui compare,
 * così ogni altra occorrenza si ottiene aggiungendo termini e mai
 * togliendone (togliere un fattore a un prodotto sarebbe dividere per un
 * fattore che può valere zero). Per trovarlo ogni certificato viene valutato
 * due volte: la prima, di prova, serve solo ad abbassare i riferimenti; la
 * seconda fa i conti veri, e non deve più trovarne uno da abbassare.
 */

typedef enum { CK_VAR_LETTER, CK_VAR_BIG, CK_VAR_EXP, CK_VAR_SIGN, CK_VAR_SEQ } CkVarKind;

/* Un'indeterminata. I campi usati dipendono da kind:
 *   LETTER: name.
 *   BIG:    node è la somma o il prodotto, dilation e offset dicono il
 *           riferimento dilation * n + offset, lower è l'estremo inferiore.
 *   EXP:    prime è il numero primo che fa da base, oppure 0 e allora la
 *           base è il polinomio `base`, nell'arena.
 *   SEQ:    name è la successione, dilation e offset come per BIG, position
 *           è il j di X(dilation * n + offset + j). */
typedef struct {
    CkVarKind kind;
    char name;
    const CkNode *node;
    long long dilation, offset, lower;
    long long prime;
    CkPoly base;
    int position;
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

/* Ritorna l'indeterminata p^n per il primo p, creandola se manca. */
static int ck_var_prime(long long p) {
    for (int i = 0; i < ck_var_count; i++)
        if (ck_vars[i].kind == CK_VAR_EXP && ck_vars[i].prime == p) return i;
    return ck_var_new((CkVar){.kind = CK_VAR_EXP, .prime = p});
}

/* Ritorna l'indeterminata B^n per il polinomio B, creandola se manca. */
static int ck_var_base(CkPoly base) {
    for (int i = 0; i < ck_var_count; i++)
        if (ck_vars[i].kind == CK_VAR_EXP && ck_vars[i].prime == 0 && ck_poly_equal(ck_vars[i].base, base)) return i;
    return ck_var_new((CkVar){.kind = CK_VAR_EXP, .base = base});
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
 * indice, stesso estremo inferiore, stesso termine, stessa dilatazione. */
static int ck_var_big(const CkNode *node, long long lower, long long dilation, long long offset) {
    for (int i = 0; i < ck_var_count; i++) {
        const CkVar *v = &ck_vars[i];
        if (v->kind != CK_VAR_BIG || v->node->kind != node->kind || v->lower != lower || v->dilation != dilation) continue;
        if (v->node->name != node->name || !ck_same(v->node->c, node->c)) continue;
        ck_lower_reference(i, offset);
        return i;
    }
    return ck_var_new((CkVar){.kind = CK_VAR_BIG, .node = node, .lower = lower, .dilation = dilation, .offset = offset});
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

/* Come vale la variabile di induzione in questo momento. */
static char ck_variable;   /* il suo nome */
static int ck_numeric;     /* 1: vale il numero ck_point; 0: vale n + ck_shift */
static long long ck_point;
static long long ck_shift;

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
    int v = ck_var_big(node, lower, a, c);
    long long reference = ck_vars[v].offset;
    if (c - reference > CK_MAX_EXTRA) { ck_unsupported("troppi termini tra due estremi della stessa somma"); return total; }
    total = ck_rat_poly(ck_poly_variable(v));
    CkPoly scaled = ck_poly_mul(ck_poly_number(ck_num_int(a)), ck_poly_variable(ck_var_letter(ck_variable)));
    for (long long j = reference + 1; j <= c && ck_status == CK_ACCEPTED; j++) {
        CkRat value = ck_eval_body(node, ck_rat_poly(ck_poly_add(scaled, ck_poly_number(ck_num_int(j)), 0)));
        total = product ? ck_rat_mul(total, value) : ck_rat_add(total, value, 0);
    }
    return total;
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
            result = ck_rat_mul(result, ck_rat_power(ck_rat_poly(ck_poly_variable(ck_var_base(side[i]))), p));
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
            if (part[j] > 1000000000000LL) { ck_unsupported("base troppo grande da scomporre in fattori primi"); return result; }
            long long rest = (long long)part[j];
            for (long long r = 2; rest > 1; r++) {
                long long times = 0;
                if (r * r > rest) r = rest;
                while (rest % r == 0) { rest /= r; times++; }
                if (times) result = ck_rat_mul(result, ck_rat_power(ck_rat_poly(ck_poly_variable(ck_var_prime(r))), (j ? -times : times) * p));
            }
        }

        /* I parametri. */
        for (int v = 0; v < CK_MAX_VARS; v++)
            if (t->e[v]) result = ck_rat_mul(result, ck_rat_power(ck_rat_poly(ck_poly_variable(ck_var_base(ck_poly_variable(v)))), t->e[v] * p));
    }
    return result;
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
        /* Un indice legato, la variabile di induzione, o un parametro. */
        for (int i = ck_env_count - 1; i >= 0; i--)
            if (ck_env[i].name == node->name) return ck_env[i].value;
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
    }
    return ck_rat_int(0);
}

/* =============================================================================
 * 9. CONTROLLO DI UN CERTIFICATO
 * ============================================================================
 *
 * I tre controlli descritti in testa al file, più le condizioni che rendono
 * lecito trattare somme e successioni come lettere.
 */

/* Le voci di un certificato, come sono state lette. */
typedef struct {
    char variable;
    long long base;
    const CkNode *left, *right;         /* i due membri dell'enunciato */
    const CkNode *factor;               /* W */
    const CkNode *generic[CK_MAX_GENERIC];
    int generics;
} CkCertificate;

/* Se il testo comincia con la parola word, seguita da uno spazio o dalla
 * fine della riga, la supera e ritorna 1. */
static int ck_word(const char *word) {
    size_t length = strlen(word);
    ck_skip();
    if (strncmp(ck_cursor, word, length) != 0 || (ck_cursor[length] && !isspace((unsigned char)ck_cursor[length]))) return 0;
    ck_cursor += length;
    return 1;
}

/* Legge le voci del certificato che comincia in ck_cursor, fino a "end"
 * compreso. Una voce sconosciuta o mancante rende il certificato sbagliato. */
static void ck_read_certificate(CkCertificate *cert) {
    static const char *const twice = "certificato scritto male: una voce compare due volte";
    int seen_variable = 0, seen_base = 0, ended = 0, recurrence;
    memset(cert, 0, sizeof *cert);
    while (isspace((unsigned char)*ck_cursor)) ck_cursor++;
    if (!ck_word("certificate") || !ck_word("induction")) { ck_reject("certificato scritto male: manca l'intestazione"); return; }
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
        } else if (ck_word("generic")) {
            if (cert->generics == CK_MAX_GENERIC) { ck_unsupported("troppe voci generic"); return; }
            cert->generic[cert->generics++] = ck_read();
        } else {
            ck_reject("certificato scritto male: voce sconosciuta o \"end\" mancante");
        }
    }
    if (ck_status == CK_ACCEPTED && (!seen_variable || !seen_base || !cert->left || !cert->factor))
        ck_reject("certificato incompleto: servono variable, base, statement e factor");

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
            divides = ck_poly_divides(below, power);
            power = ck_poly_mul(power, factors);
        }
        if (ck_status == CK_ACCEPTED && !divides) ck_reject("il fattore ha un denominatore che può annullarsi dove l'enunciato è definito");
    }
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
    ck_cursor = *text;
    ck_read_certificate(&cert);
    *text = ck_cursor;

    /* La valutazione di prova e quella vera. Tra l'una e l'altra restano le
     * indeterminate con i loro riferimenti; i polinomi si ricalcolano. */
    for (int pass = 0; pass < 2 && ck_status == CK_ACCEPTED; pass++) {
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
 * 10. MAIN
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
