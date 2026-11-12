/* =============================================================================
 * mathresearcher: dimostratore di enunciati matematici scritti in LaTeX
 * =============================================================================
 *
 * COSA FA
 *
 * Legge enunciati nella variabile intera n, scritti in LaTeX, per esempio
 *
 *     \sum_{k=1}^{n} k 2^{k} = \left(n - 1\right) 2^{n + 1} + 2
 *
 * e prova a dimostrarli per induzione su n. Per ogni enunciato risponde:
 *
 *   - DIMOSTRATO: caso base e passo induttivo sono stati verificati;
 *   - FALSO: esiste un n, che viene stampato, in cui i due membri differiscono;
 *   - NON DIMOSTRATO: il metodo si applica ma non chiude la dimostrazione;
 *   - NON SUPPORTATO: l'enunciato usa notazioni o conti fuori dalla portata.
 *
 * Tratta altre due famiglie di enunciati, ognuna con il suo metodo e con la
 * dimostrazione scritta per esteso:
 *
 *   - quelli sugli insiemi, come (S \setminus T) \setminus V = S \setminus
 *     (T \cup V), decisi esaminando le regioni di un diagramma di Venn
 *     (capitoli da 14 a 16);
 *   - quelli con quantificatori su insiemi di numeri, come "per ogni y in Z
 *     esiste x in Z con x + 5 = y", anche a proposito di un'applicazione
 *     definita da una formula: è così che si scrive che è iniettiva o
 *     suriettiva (capitoli da 17 a 19).
 *
 * I metodi si combinano dove serve. Dentro un enunciato con quantificatori
 * un "per ogni n naturale" seguito da un'uguaglianza con somme viene
 * dimostrato per induzione: il metodo del capitolo 13 è una delle regole del
 * capitolo 18, e le espressioni sono le stesse in tutti e due. Gli enunciati
 * sugli insiemi restano invece un linguaggio a parte.
 *
 * Gli enunciati arrivano da un file .tex (un \item per enunciato, capitolo 21)
 * oppure, senza argomenti, da una riga di stdin. Dopo la formula un \item può
 * dire da quale n vale l'enunciato; ciò che il programma non sa leggere lo
 * tratta come una condizione, e allora non dichiara falso l'enunciato. Su
 * richiesta le dimostrazioni vengono scritte in un documento LaTeX e
 * compilate in PDF (capitolo 22).
 *
 * COME LO FA
 *
 *   testo LaTeX --[parsing, cap. 9]--> albero --[valutazione, cap. 11]-->
 *   frazione di polinomi --[induzione, cap. 13]--> verdetto
 *
 * Il parsing guarda solo i caratteri e costruisce l'albero sintattico. La
 * valutazione trasforma l'albero in una forma algebrica unica; l'induzione
 * valuta lo stesso albero tre volte, con n che vale n0, n e n+1.
 *
 * L'IDEA CENTRALE: SUCCESSIONI COME LETTERE
 *
 * Una somma S(n) = f(1) + ... + f(n) non ha in generale una formula chiusa, ma
 * per l'induzione non serve: basta sapere che S(n+1) = S(n) + f(n+1). Il
 * programma tratta quindi S(n) come una lettera nuova, un "atomo", e scrive
 * S(n+1) come atomo più l'ultimo termine. Lo stesso vale per i prodotti, per
 * n!, per 2^n, per i numeri di Fibonacci, per sin(n x): ognuno è un atomo, e
 * il suo valore in n+1 si scrive con gli atomi in n.
 *
 * Fatto questo, un enunciato L(n) = R(n) diventa una frazione di polinomi
 * D = L - R negli atomi, e l'induzione è algebra:
 *
 *   - caso base: con n = n0 somme e prodotti si svolgono termine per termine e
 *     D deve ridursi a zero;
 *   - passo: l'ipotesi induttiva D(n) = 0 permette di ricavare un atomo (di
 *     solito la somma) in funzione degli altri; sostituendolo in D(n+1) deve
 *     restare il polinomio nullo.
 *
 * Decidere se un polinomio è nullo è immediato: si guarda se ha termini.
 * Decidere che non lo è mai, per nessun n, è un'altra questione, e serve
 * per i divisori: vedi "Limiti di validità" più sotto.
 *
 * PERCHÉ QUESTE SCELTE
 *
 * Perché un albero e non la forma postfissa di prima. Lo stesso enunciato va
 * valutato con n legato a valori diversi e con l'indice k di una somma
 * sostituito da n+1: serve una struttura su cui tornare più volte, e la
 * ricorsione sull'albero è la più diretta. Gli enunciati sono corti, quindi la
 * profondità non è un problema.
 *
 * Perché non servono regole di "spostamento" per ogni funzione. Non c'è una
 * tabella che dica come si passa da n a n+1 per il fattoriale, per le potenze
 * o per il seno: basta rivalutare l'albero con n+1 al posto di n e portare
 * ogni risultato in forma canonica rispetto agli atomi in n. Le sole
 * conoscenze sono le definizioni (capitolo 10), scritte come dati.
 *
 * Perché frazioni non ridotte. Semplificare richiederebbe il massimo comun
 * divisore tra polinomi in più variabili. Per decidere un'uguaglianza basta
 * che il numeratore della differenza sia nullo.
 *
 * Perché gli esponenziali complessi. sin e cos si scrivono con e^{i x}; così
 * le formule di addizione e sin^2 + cos^2 = 1 non vanno insegnate al
 * programma: sono conseguenze dell'algebra degli esponenti (capitolo 11).
 *
 * Perché radici e logaritmi passano da una scomposizione in fattori. Una
 * radice è una potenza a esponente frazionario e un logaritmo trasforma i
 * prodotti in somme: scomponendo l'argomento in fattori di segno noto, la
 * radice di 12 e 2 per la radice di 3 diventano lo stesso monomio, e
 * ln(n+1) - ln(n) e ln(1 + 1/n) la stessa somma (capitoli 11 e 12).
 *
 * COSA NON C'È ANCORA, E DOVE ANDREBBE
 *
 *   - Disuguaglianze: il parsing le riconosce, ma servirebbe un metodo per
 *     decidere il segno di un'espressione. Andrebbe nel capitolo 13.
 *   - Valori assoluti fuori da un logaritmo: il parsing li riconosce; manca
 *     la regola di valutazione nel capitolo 11.
 *   - Identità tra i termini di una successione, come quella di Cassini per
 *     Fibonacci, F(n) F(n+2) - F(n+1)^2 = (-1)^{n+1}: gli atomi F(n) e F(n+1)
 *     sono trattati come lettere indipendenti. Servirebbe dimostrarle per
 *     induzione come lemmi e usarle nel test di annullamento (capitolo 12).
 *   - Somme il cui termine dipende anche da n (per esempio con \binom{n}{k}):
 *     passando da n a n+1 cambiano tutti i termini, non solo l'ultimo.
 *   - Enunciati in prosa, derivate e integrali: non sono una formula sola.
 *   - Negli enunciati con quantificatori: i numeri reali e complessi, le
 *     disuguaglianze, il valore assoluto, le applicazioni definite per casi o
 *     a valori in coppie, e gli insiemi qualsiasi al posto di quelli numerici.
 *     Il linguaggio è nel capitolo 17; ogni nuovo modo di ragionare è una
 *     regola in più nel capitolo 18. L'induzione vi entra solo per un "per
 *     ogni n" senza altre variabili intorno.
 *   - Un certificato per tutto. Lo hanno le induzioni scritte con somme,
 *     prodotti, potenze, fattoriali, binomiali e successioni per ricorrenza
 *     (vedi "I certificati"). Non lo hanno le induzioni con radici,
 *     logaritmi, seni e arcotangenti, né gli enunciati sugli insiemi e
 *     quelli con quantificatori: lì resta il solo controllo sui numeri e sui
 *     campioni. Servirebbe che ogni regola dei capitoli 15 e 18 emettesse un
 *     passo in una notazione che checker.c sappia ricontrollare, e che 13c
 *     certificasse anche "questo divisore non si annulla mai".
 *
 * In tutti questi casi la risposta è NON SUPPORTATO, mai un verdetto inventato.
 *
 * LIMITI DI VALIDITÀ
 *
 * Un verdetto DIMOSTRATO per induzione vale per ogni n dal caso base in su, e
 * per i valori dei parametri in cui tutte le espressioni dell'enunciato sono
 * definite: denominatori non nulli, radicandi non negativi, argomenti dei
 * logaritmi positivi. I parametri, come x, sono numeri reali.
 *
 * L'induzione da sola non basta a dirlo: è algebra sulle lettere, e prova che
 * i due membri sono uguali "dove sono definiti". Tre controlli, tutti nel
 * capitolo 13, fanno il resto.
 *
 *   - Che quel "dove" esista (13e): servono dei valori dei parametri con cui
 *     i due membri si calcolano davvero, con i numeri, per i primi n. Così
 *     ln(-x) = ln(x) non risulta dimostrato: l'algebra lo accetta, perché
 *     tratta i logaritmi come ln|x|, ma non c'è un x in cui i due membri
 *     abbiano senso insieme. Se i numeri semplici non bastano, i valori da
 *     provare si ricavano dalle condizioni stesse dell'enunciato.
 *   - Che duri per ogni n (13c): con quei valori dei parametri, ogni
 *     divisione, radice e logaritmo deve avere senso per tutti gli n, non
 *     solo per i primi. (n - 50)/(n - 50) = 1 non risulta dimostrato, e il
 *     verdetto dice che per n = 50 c'è una divisione per zero. Se nessun
 *     valore fisso dei parametri va bene per ogni n, basta che per ogni n
 *     vadano bene tutti i valori abbastanza grandi (13d): è il caso della
 *     somma dei ln(x - k), definita per x > n.
 *   - Che un divisore non sia zero di nascosto: 2 cos(pi/3) - 1 è zero, ma
 *     l'algebra non lo riduce a zero. Un divisore conta come "diverso da
 *     zero" solo se una regola lo prova.
 *
 * Quando uno di questi controlli non sa decidere, il verdetto è NON
 * DIMOSTRATO. Capita anche con enunciati veri: le regole di 13c conoscono i
 * polinomi in n e in potenze come 2^n, i segni evidenti, il segno di un seno
 * o di un coseno dal suo quadrante. Non sanno dire che n! - 5 non si annulla,
 * né il segno di un'espressione trigonometrica che non sia un solo seno o
 * coseno, né quello di ln 3 - 1: lì servirebbero altre regole.
 *
 * Negli enunciati con quantificatori questa riserva non c'è: ogni variabile
 * ha il suo insieme, e che nessun divisore si annulli lì fa parte di ciò che
 * viene dimostrato (capitolo 18).
 *
 * LE DIMOSTRAZIONI SCRITTE
 *
 * Ogni verdetto DIMOSTRATO ha la sua dimostrazione scritta, e ciò che vi è
 * scritto viene ricontrollato per una strada diversa da quella che lo ha
 * prodotto. Per gli insiemi e per i quantificatori si ricontrolla ogni fatto
 * prima di scriverlo (16h, 16i e i controlli del capitolo 18). Per
 * l'induzione si fa di più: ogni formula scritta viene riletta dal testo,
 * con il parser degli enunciati, e ricalcolata con i numeri (13g). Una
 * dimostrazione che non supera il controllo non viene stampata.
 *
 * Le frasi che collegano i fatti ("quindi", "holds", "fails") sono scritte
 * dal programma secondo schemi fissi: non c'è un controllo automatico che le
 * capisca. Le fissano i test, con una frase attesa per ogni combinazione di
 * uguale e diverso, affermato e negato (tests/engine).
 *
 * I CERTIFICATI
 *
 * I controlli descritti fin qui sono del programma su se stesso: un errore
 * nella sua aritmetica o nel suo modo di leggere le formule li attraversa
 * tutti. Per questo una dimostrazione per induzione viene anche consegnata,
 * come certificato, a un secondo programma: checker.c, che ha la sua
 * aritmetica e i suoi polinomi, non usa nulla di questo file, ed è abbastanza
 * piccolo da poterlo leggere tutto (13h). Se lo respinge, il verdetto è NON
 * DIMOSTRATO.
 *
 * Il certificato dimostra una cosa precisa, enunciata in testa a checker.c,
 * e non di più: per ogni n dal caso base in su i due membri sono uguali,
 * dove l'enunciato è definito fino a quel n. Che sia definito per ogni n lo
 * stabiliscono le regole di 13c e 13d, che restano controlli del solo
 * motore. Resta fuori dal certificato anche la lettura del LaTeX: l'enunciato
 * vi compare riscritto nella sua notazione, e chi vuole può confrontarlo.
 *
 * Il verdetto dice sempre se il certificato c'è: "certificato verificato",
 * oppure "senza certificato", con il motivo quando è un'induzione.
 *
 * COMPILAZIONE
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o mathresearcher main.c
 *
 * Servono GCC o Clang: i conti esatti usano __int128. main.c include
 * checker.c, che deve stare nella stessa cartella; il verificatore si compila
 * anche da solo (vedi la testa di checker.c). Per avere le dimostrazioni in
 * PDF serve inoltre un compilatore LaTeX installato (tectonic oppure
 * pdflatex); senza, il programma scrive comunque il documento .tex.
 *
 * INDICE DEI CAPITOLI
 *
 *    1. Include e limiti
 *    2. Errori e memoria
 *    3. Numeri esatti: razionali e razionali complessi
 *    4. Monomi e atomi
 *    5. Polinomi negli atomi
 *    6. Creazione degli atomi
 *    7. Frazioni di polinomi
 *    8. Albero sintattico
 *    9. Parsing: dal LaTeX all'albero
 *   10. Vocabolario: le successioni note, come dati
 *   11. Valutazione: dall'albero alla frazione
 *   12. Test di annullamento
 *   13. Induzione: caso base, passo, verdetto
 *   14. Insiemi: notazione e albero
 *   15. Insiemi: decisione per regioni
 *   16. Insiemi: verifica indipendente e dimostrazione scritta
 *   17. Quantificatori: notazione e albero
 *   18. Quantificatori: regole di dimostrazione
 *   19. Quantificatori: verifica indipendente e verdetto
 *   20. Dimostrare un enunciato
 *   21. Lettura degli enunciati da un file .tex
 *   22. main
 */

/* =============================================================================
 * 1. INCLUDE E LIMITI
 * ============================================================================
 *
 * Solo libreria standard. I limiti servono a fermare un conto che cresce
 * troppo con un NON SUPPORTATO, invece di esaurire la macchina.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>

/* Il verificatore dei certificati (13h). È un programma a sé, incluso qui
 * solo perché il progetto resti un unico comando di compilazione: senza il
 * suo main(), e di tutto ciò che contiene questo file chiama soltanto
 * ck_check(). Il verificatore non sa nulla di questo file.
 *
 * Warning: non usare qui altri nomi che cominciano con ck_, Ck o CK_. Il
 * controllo vale qualcosa solo finché i due programmi non hanno codice in
 * comune: nemmeno un'utilità "innocua". */
#define CK_NO_MAIN
#include "checker.c"

#define ARENA_BYTES ((size_t)256 << 20)  /* memoria di lavoro per un enunciato */
#define MAX_ATOMS 256        /* atomi distinti in un enunciato */
#define MAX_FACTORS 8        /* atomi distinti in un monomio */
#define MAX_ENV 16           /* variabili legate contemporaneamente */
#define MAX_ORDER 4          /* ordine massimo di una ricorrenza del vocabolario */
#define MAX_UNROLL 4096      /* termini svolti a mano di una somma o prodotto */
#define MAX_POWER 512        /* massimo esponente intero costante */
#define MAX_DEGREE 4         /* grado massimo dell'ipotesi nel passo induttivo */
#define MAX_PRODUCT 4000000  /* termini di un prodotto di polinomi */
#define MAX_FORMULA 4096     /* lunghezza di un enunciato */
#define SEARCH_SPAN 10       /* valori di n provati in cerca di un controesempio */
#define MAX_SET_VARS 4       /* insiemi distinti in un enunciato sugli insiemi */
#define MAX_CONFIGS 16       /* configurazioni elencate in una dimostrazione */
#define PROOF_BYTES 65536    /* lunghezza massima di una dimostrazione scritta */
#define CERTIFICATE_BYTES 65536  /* lunghezza massima di un certificato */
#define MAX_MAPS 4           /* applicazioni dichiarate in un enunciato */
#define MAX_HOLES 4          /* punti tolti da un insieme numerico con \setminus */
#define MAX_BOUND 8          /* quantificatori uno dentro l'altro */
#define MAX_LITERALS 4       /* confronti in una disgiunzione */
#define SAMPLE_SIZE 12       /* elementi di un insieme numerico provati uno a uno */
#define MAX_CHECKS 4096      /* combinazioni di valori provate dalla verifica sul campione */
#define MAX_SEARCH 20000     /* passi di ricerca per un enunciato con quantificatori */
#define MAX_POINT 8          /* parametri a cui la verifica numerica dà un valore */
#define MAX_CONDITIONS 64    /* condizioni di esistenza annotate in un conto */
#define MAX_SIGN_FACTORS 8   /* fattori di cui una condizione chiede il segno del prodotto */
#define MAX_CASES 12         /* casi sul resto di una divisione, in una dimostrazione */
#define MAX_EQUATIONS 12     /* uguaglianze provate per ricavare il valore di un "esiste" */
#define MAX_SCAN 100000      /* valori di n provati uno a uno per il segno di un polinomio */

/* Valore di Token.ch per le graffe scritte \{ e \}: 256 più il carattere, per
 * distinguerle dalle graffe di raggruppamento di LaTeX. */
#define ESCAPED 256
#define MAX_PARTS 24         /* fattori distinti nella scomposizione di un polinomio */
#define MAX_UNIVARIATE 16    /* grado massimo di un polinomio in n da scomporre */
#define MAX_DIVISORS 2048    /* divisori di un intero provati come radici razionali */

/* Nomi interni di variabili, fuori dall'intervallo dei caratteri: \pi, e una
 * lettera ausiliaria che vale zero o più (vedi body_is_positive()). */
#define VAR_PI 256
#define VAR_SLACK 257

/* I razionali tengono numeratore e denominatore sotto 2^62, così il prodotto
 * di due di essi e la somma di due prodotti stanno sempre in un __int128. */
#define COEF_LIMIT ((__int128)1 << 62)

/* =============================================================================
 * 2. ERRORI E MEMORIA
 * ============================================================================
 *
 * Errori. Un conto può uscire dai limiti in molti punti (overflow, divisione
 * per zero, notazione senza regola). Invece di propagare un codice attraverso
 * ogni chiamata, la prima causa viene annotata in `failure` e il conto
 * prosegue producendo valori senza significato: chi lo ha avviato controlla
 * `failure` alla fine e scarta tutto.
 *
 * Memoria. Polinomi e nodi dell'albero vivono in un'unica zona ("arena") da
 * cui si prende spazio in fila e che si svuota tutta insieme tra un enunciato
 * e il successivo. Non c'è free(): i valori intermedi sono tanti e piccoli, e
 * la loro vita finisce comunque con l'enunciato.
 */

/* Motivo del primo errore del conto in corso, in italiano; NULL se va bene. */
static const char *failure;

/* Annota reason come causa del fallimento, se è la prima. */
static void fail(const char *reason) { if (!failure) failure = reason; }

/* 1 se il conto in corso è fallito perché un'espressione non è definita nel
 * punto in cui la si calcolava: divisione per zero, logaritmo di un numero non
 * positivo, radice di un numero negativo. Serve a distinguere questo caso dai
 * limiti del programma, come i numeri troppo grandi. Chi vuole la distinzione
 * lo azzera prima del conto, insieme a `failure`. */
static int undefined;

/* Come fail(), per i casi in cui un'espressione non è definita. */
static void fail_undefined(const char *reason) {
    if (!failure) undefined = 1;
    fail(reason);
}

/* Il messaggio dell'arena piena. Ha un nome perché una ricerca che la
 * riempie (capitolo 19) lo riconosce: lì vuol dire "troppi tentativi", non
 * un enunciato fuori portata. */
static const char *const OUT_OF_MEMORY = "memoria di lavoro esaurita";

/* L'arena: arena_used è il numero di byte già assegnati. */
static _Alignas(16) char arena[ARENA_BYTES];
static size_t arena_used;

/* Arrotonda bytes al multiplo di 16 successivo, l'unità di assegnazione. */
static size_t arena_round(size_t bytes) { return (bytes + 15) & ~(size_t)15; }

/* Ritorna un blocco di bytes byte, allineato a 16 e non inizializzato, oppure
 * NULL (con fail()) se l'arena è piena. */
static void *arena_alloc(size_t bytes) {
    bytes = arena_round(bytes);
    if (bytes > ARENA_BYTES - arena_used) { fail(OUT_OF_MEMORY); return NULL; }
    void *block = arena + arena_used;
    arena_used += bytes;
    return block;
}

/* =============================================================================
 * 3. NUMERI ESATTI: RAZIONALI E RAZIONALI COMPLESSI
 * ============================================================================
 *
 * Nessun numero in virgola mobile: un errore di arrotondamento farebbe
 * sembrare diverso da zero un valore nullo, e la dimostrazione crollerebbe.
 *
 * I coefficienti dei polinomi sono numeri a + b i con a e b razionali. La
 * parte immaginaria serve solo alla trigonometria (capitolo 11): negli
 * enunciati senza seni e coseni resta sempre zero.
 */

/* Un numero razionale num/den. Invariante: den > 0 e la frazione è ridotta ai
 * minimi termini, quindi due Frac uguali hanno campi uguali. Lo stesso tipo
 * rappresenta gli esponenti dei monomi (capitolo 4). */
typedef struct { long long num, den; } Frac;

/* Massimo comun divisore di |a| e |b| con l'algoritmo di Euclide:
 * gcd(a, b) = gcd(b, a mod b), finché il resto è zero. */
static __int128 gcd128(__int128 a, __int128 b) {
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b) { __int128 t = a % b; a = b; b = t; }
    return a;
}

/* Ritorna num/den ridotta ai minimi termini. Fallisce se den è zero o se il
 * risultato esce da COEF_LIMIT. */
static Frac frac_make(__int128 num, __int128 den) {
    if (den == 0) { fail_undefined("divisione per zero"); return (Frac){0, 1}; }
    if (den < 0) { num = -num; den = -den; }
    __int128 g = gcd128(num, den);
    if (g > 1) { num /= g; den /= g; }
    if (num >= COEF_LIMIT || num <= -COEF_LIMIT || den >= COEF_LIMIT) {
        fail("numeri troppo grandi per il calcolo esatto");
        return (Frac){0, 1};
    }
    return (Frac){(long long)num, (long long)den};
}

/* Operazioni elementari sui razionali. */
static Frac frac_int(long long v) { return (Frac){v, 1}; }
static Frac frac_neg(Frac a) { return (Frac){-a.num, a.den}; }
static Frac frac_add(Frac a, Frac b) {
    return frac_make((__int128)a.num * b.den + (__int128)b.num * a.den, (__int128)a.den * b.den);
}
static Frac frac_mul(Frac a, Frac b) {
    return frac_make((__int128)a.num * b.num, (__int128)a.den * b.den);
}
static Frac frac_div(Frac a, Frac b) {
    return frac_make((__int128)a.num * b.den, (__int128)a.den * b.num);
}

/* Confronto: negativo, zero o positivo secondo che a sia minore, uguale o
 * maggiore di b. */
static int frac_cmp(Frac a, Frac b) {
    __int128 left = (__int128)a.num * b.den, right = (__int128)b.num * a.den;
    return (left > right) - (left < right);
}

/* Parte intera inferiore di a: il più grande intero non maggiore di a. La
 * divisione del C tronca verso zero, quindi i negativi vanno corretti. */
static long long frac_floor(Frac a) {
    long long q = a.num / a.den;
    return (a.num % a.den < 0) ? q - 1 : q;
}

/* Un numero complesso re + im i a coordinate razionali: il coefficiente di un
 * termine di polinomio. */
typedef struct { Frac re, im; } Num;

static Num num_int(long long v) { return (Num){frac_int(v), frac_int(0)}; }
static Num num_frac(Frac v) { return (Num){v, frac_int(0)}; }
static int num_is_zero(Num a) { return a.re.num == 0 && a.im.num == 0; }
static int num_equal(Num a, Num b) { return frac_cmp(a.re, b.re) == 0 && frac_cmp(a.im, b.im) == 0; }
static Num num_neg(Num a) { return (Num){frac_neg(a.re), frac_neg(a.im)}; }
static Num num_add(Num a, Num b) { return (Num){frac_add(a.re, b.re), frac_add(a.im, b.im)}; }

/* (a + b i)(c + d i) = (ac - bd) + (ad + bc) i. */
static Num num_mul(Num a, Num b) {
    return (Num){frac_add(frac_mul(a.re, b.re), frac_neg(frac_mul(a.im, b.im))),
                 frac_add(frac_mul(a.re, b.im), frac_mul(a.im, b.re))};
}

/* 1 / (a + b i) = (a - b i) / (a^2 + b^2): si moltiplica sopra e sotto per il
 * coniugato. Fallisce se a + b i è zero. */
static Num num_inv(Num a) {
    Frac norm = frac_add(frac_mul(a.re, a.re), frac_mul(a.im, a.im));
    return (Num){frac_div(a.re, norm), frac_neg(frac_div(a.im, norm))};
}

/* =============================================================================
 * 4. MONOMI E ATOMI
 * ============================================================================
 *
 * Un ATOMO è una quantità che l'algebra tratta come una lettera indivisibile.
 * Ce ne sono sei tipi (le etichette da 4f a 4h sono più sotto, in
 * term_reduce()):
 *
 *   4a. VAR: una variabile dell'enunciato: n, oppure un parametro come x.
 *   4b. BIGOP: una somma o un prodotto con estremo superiore a n + c, per
 *       esempio la somma di f(k) per k da 1 a n.
 *   4c. REC: un termine X(a n + j) di una successione definita da una
 *       ricorrenza lineare, per esempio il numero di Fibonacci F(n).
 *   4d. POW: una potenza base^M, dove M è un monomio. Con M non banale è una
 *       potenza a esponente simbolico: 2^n, a^n, (n+1)^n, e^{i n x}. Con
 *       M = 1 è la base stessa presa come lettera, e serve per le radici:
 *       la radice quadrata di n + 1 è l'atomo "(n+1)" elevato a 1/2.
 *   4e. LN: il logaritmo naturale del valore assoluto di una base.
 *   4i. ATAN: l'arcotangente di un'espressione reale.
 *
 * Un MONOMIO è un prodotto di atomi elevati a esponenti razionali, anche
 * negativi: 1/2^n è il monomio (2^n)^{-1}, e^{i x/2} è (e^{i x})^{1/2}.
 *
 * Perché gli atomi si possono trattare come lettere indipendenti. Il test "è
 * zero?" cancella solo termini con lo stesso monomio. Se due monomi diversi
 * rappresentassero in realtà la stessa quantità, il test potrebbe non
 * accorgersi di uno zero (e rispondere NON DIMOSTRATO), ma non dichiarerà mai
 * nullo qualcosa che non lo è: ogni cancellazione è un'uguaglianza vera. Per
 * perdere il meno possibile, il capitolo 6 crea gli atomi in forma canonica:
 * 4^n diventa (2^n)^2, non un atomo a sé.
 */

/* Un atomo elevato a un esponente razionale non nullo. */
typedef struct {
    int atom;  /* indice nella tabella `atoms` */
    Frac exp;  /* esponente, mai zero */
} Factor;

/* Un monomio: il prodotto dei suoi fattori. Invariante: i fattori sono in
 * ordine crescente di atomo e senza ripetizioni, così due monomi uguali hanno
 * la stessa rappresentazione. count == 0 è il monomio 1. */
typedef struct {
    int count;                   /* fattori validi in `factor` */
    Factor factor[MAX_FACTORS];
} Mono;

/* Un termine di polinomio: coefficiente per monomio. */
typedef struct {
    Num coef;
    Mono mono;
} Term;

typedef struct Node Node;  /* nodo dell'albero sintattico, capitolo 8 */

/* I tipi di atomo descritti sopra. */
typedef enum { ATOM_VAR, ATOM_BIGOP, ATOM_REC, ATOM_POW, ATOM_LN, ATOM_ATAN } AtomKind;

/* Cosa fa da base in un atomo POW o LN:
 *   BASE_EI:    la base è e^i, cioè l'atomo vale e^{i M} (trigonometria);
 *   BASE_PRIME: un numero primo;
 *   BASE_ATOM:  un altro atomo (un parametro, o n stesso);
 *   BASE_POLY:  un polinomio non riducibile a monomio, per esempio n + 1. */
typedef enum { BASE_EI, BASE_PRIME, BASE_ATOM, BASE_POLY } BaseKind;

/* Come un atomo e^{i M} si semplifica quando M contiene pi greco:
 *   PI_NONE:    nessuna regola;
 *   PI_CONST:   M = pi, l'atomo è il numero e^{i pi} = -1;
 *   PI_INTEGER: M = pi per una potenza di n: l'atomo vale 1 al quadrato. */
typedef enum { PI_NONE, PI_CONST, PI_INTEGER } PiMode;

/* Un atomo. I campi usati dipendono da kind:
 *
 *   VAR:   id = nome della variabile.
 *   BIGOP: node = il nodo somma/prodotto che lo definisce, dilation = a,
 *          offset = c: l'atomo vale la somma (o il prodotto) del corpo di
 *          node per l'indice dall'estremo inferiore fino ad a n + c.
 *   REC:   id = posizione nel vocabolario, dilation = a, offset = j:
 *          l'atomo vale X(a n + j).
 *   POW:   base e id dicono la base (id è il primo, l'atomo o la posizione in
 *          `base_polys`), exponent è il monomio M.
 *   LN:    base e id dicono la base, come per POW.
 *   ATAN:  id = posizione dell'argomento in `atan_args` (capitolo 11).
 *
 * pi e depends_on_n sono calcolati alla creazione. */
typedef struct {
    AtomKind kind;
    long long id;
    const Node *node;
    long long dilation, offset;
    BaseKind base;
    Mono exponent;
    PiMode pi;
    int depends_on_n;  /* 1 se il valore cambia al variare di n */
    int positive;      /* BIGOP: 1 se è un prodotto di fattori tutti positivi */
} Atom;

/* Tabella degli atomi dell'enunciato in corso. */
static Atom atoms[MAX_ATOMS];
static int atom_count;

/* Nome della variabile su cui si fa induzione nell'enunciato in corso. */
static int induction_var = 'n';

/* Ritorna il monomio atom^exp. */
static Mono mono_atom(int atom, Frac exp) {
    Mono m;
    m.count = 1;
    m.factor[0] = (Factor){atom, exp};
    return m;
}

/* Ritorna a * b: i fattori si fondono mantenendo l'ordine, e gli esponenti
 * dello stesso atomo si sommano (x^p * x^q = x^{p+q}). Un esponente che
 * diventa zero fa sparire il fattore. */
static Mono mono_mul(const Mono *a, const Mono *b) {
    Mono r;
    r.count = 0;
    int i = 0, j = 0;
    while (i < a->count || j < b->count) {
        Factor f;
        if (j == b->count || (i < a->count && a->factor[i].atom < b->factor[j].atom)) {
            f = a->factor[i++];
        } else if (i == a->count || b->factor[j].atom < a->factor[i].atom) {
            f = b->factor[j++];
        } else {
            f = a->factor[i++];
            f.exp = frac_add(f.exp, b->factor[j++].exp);
            if (f.exp.num == 0) continue;
        }
        if (r.count == MAX_FACTORS) { fail("monomio con troppi fattori"); break; }
        r.factor[r.count++] = f;
    }
    return r;
}

/* Ritorna m^q: ogni esponente viene moltiplicato per q. */
static Mono mono_pow(const Mono *m, Frac q) {
    Mono r;
    r.count = 0;
    if (q.num == 0) return r;
    for (int i = 0; i < m->count; i++)
        r.factor[r.count++] = (Factor){m->factor[i].atom, frac_mul(m->factor[i].exp, q)};
    return r;
}

/* Ordine totale sui monomi, usato per tenere i polinomi ordinati: confronto
 * fattore per fattore, prima per atomo e poi per esponente; a parità vince il
 * più corto. Ritorna negativo, zero o positivo. */
static int mono_cmp(const Mono *a, const Mono *b) {
    for (int i = 0; i < a->count && i < b->count; i++) {
        if (a->factor[i].atom != b->factor[i].atom) return a->factor[i].atom - b->factor[i].atom;
        int c = frac_cmp(a->factor[i].exp, b->factor[i].exp);
        if (c) return c;
    }
    return a->count - b->count;
}

/* Ritorna 1 se il monomio contiene un atomo che dipende da n. */
static int mono_depends_on_n(const Mono *m) {
    for (int i = 0; i < m->count; i++)
        if (atoms[m->factor[i].atom].depends_on_n) return 1;
    return 0;
}

/* Ritorna l'esponente con cui atom compare nel monomio m, zero se manca. */
static Frac mono_exponent(const Mono *m, int atom) {
    for (int i = 0; i < m->count; i++)
        if (m->factor[i].atom == atom) return m->factor[i].exp;
    return frac_int(0);
}

/* Ritorna m con l'esponente di atom sostituito da exp (zero lo toglie). */
static Mono mono_with_exponent(const Mono *m, int atom, Frac exp) {
    Mono rest;
    rest.count = 0;
    for (int i = 0; i < m->count; i++)
        if (m->factor[i].atom != atom) rest.factor[rest.count++] = m->factor[i];
    if (exp.num == 0) return rest;
    Mono factor = mono_atom(atom, exp);
    return mono_mul(&rest, &factor);
}

/* Applica al termine t le identità che legano un atomo a un numero,
 * modificandolo sul posto. Dopo la chiamata gli esponenti interessati sono in
 * forma canonica, così monomi uguali restano riconoscibili.
 *
 * 4f. Un atomo E = e^{i pi} elevato a r:
 *       - E^2 = 1, quindi r si riduce modulo 2 all'intervallo [0, 2);
 *       - E^1 = -1, quindi da r >= 1 si toglie 1 cambiando segno al
 *         coefficiente;
 *       - E^{1/2} = e^{i pi/2} = i, quindi da r >= 1/2 si toglie 1/2
 *         moltiplicando il coefficiente per i.
 *     Resta r in [0, 1/2).
 *
 * 4g. Un atomo E = e^{i pi n} (o e^{i pi n^2}, ...): vale solo la prima
 *     regola, E^2 = e^{2 pi i n} = 1 perché n è intero. E^1 = (-1)^n resta
 *     un atomo.
 *
 * 4h. Un atomo "primo p preso come lettera" elevato a r (nasce dalle radici:
 *     la radice di 2 è p^{1/2}): la parte intera k di r è un numero vero,
 *     p^k, che passa nel coefficiente. Resta r in [0, 1). */
static void term_reduce(Term *t) {
    for (int i = 0; i < t->mono.count; i++) {
        Factor *f = &t->mono.factor[i];
        const Atom *atom = &atoms[f->atom];
        Frac r = f->exp;

        if (atom->pi != PI_NONE) {
            /* 4f, 4g. Riduzione modulo 2. */
            r = frac_add(r, frac_int(-2 * frac_floor(frac_div(r, frac_int(2)))));

            /* 4f. Estrazione di -1 e di i, solo per la costante e^{i pi}. */
            if (atom->pi == PI_CONST) {
                if (frac_cmp(r, frac_int(1)) >= 0) {
                    t->coef = num_neg(t->coef);
                    r = frac_add(r, frac_int(-1));
                }
                if (frac_cmp(r, (Frac){1, 2}) >= 0) {
                    t->coef = num_mul(t->coef, (Num){frac_int(0), frac_int(1)});
                    r = frac_add(r, (Frac){-1, 2});
                }
            }
        } else if (atom->kind == ATOM_POW && atom->base == BASE_PRIME && atom->exponent.count == 0) {
            /* 4h. Parte intera dell'esponente di un primo. */
            long long whole = frac_floor(r);
            if (whole > MAX_POWER || whole < -MAX_POWER) { fail("esponente costante troppo grande"); return; }
            Num step = num_frac(whole > 0 ? frac_int(atom->id) : (Frac){1, atom->id});
            for (long long k = (whole > 0 ? whole : -whole); k > 0 && !failure; k--) t->coef = num_mul(t->coef, step);
            r = frac_add(r, frac_int(-whole));
        } else {
            continue;
        }

        /* Il fattore sparisce se l'esponente è diventato zero. */
        f->exp = r;
        if (r.num == 0) {
            for (int j = i + 1; j < t->mono.count; j++) t->mono.factor[j - 1] = t->mono.factor[j];
            t->mono.count--;
            i--;
        }
    }
}

/* =============================================================================
 * 5. POLINOMI NEGLI ATOMI
 * ============================================================================
 *
 * Un polinomio è una somma di termini. La forma è canonica: termini ordinati
 * per monomio, monomi tutti diversi, nessun coefficiente nullo. Ne segue il
 * test su cui si regge tutto il programma: un polinomio è zero se e solo se
 * non ha termini.
 *
 * I polinomi sono valori immutabili: ogni operazione ne costruisce uno nuovo
 * nell'arena e non tocca gli operandi.
 */

/* Un polinomio in forma canonica. `term` punta a `count` termini nell'arena
 * (NULL se count == 0); la memoria è dell'arena, non del polinomio. */
typedef struct {
    int count;
    Term *term;
} Poly;

/* Confronto tra termini per qsort: ordine dei monomi. */
static int term_cmp(const void *a, const void *b) {
    return mono_cmp(&((const Term *)a)->mono, &((const Term *)b)->mono);
}

/* Ritorna spazio per capacity termini, o NULL se l'arena è piena. */
static Term *terms_alloc(long long capacity) {
    if (capacity > MAX_PRODUCT) { fail("espressione troppo grande"); return NULL; }
    return arena_alloc((size_t)capacity * sizeof(Term));
}

/* Porta in forma canonica i primi `used` termini di buffer, che era stato
 * chiesto con terms_alloc(capacity), e ritorna il polinomio risultante. */
static Poly poly_finish(Term *buffer, long long capacity, int used) {
    /* Ordina, poi somma i coefficienti dei termini con lo stesso monomio. */
    qsort(buffer, (size_t)used, sizeof(Term), term_cmp);
    int merged = 0;
    for (int i = 0; i < used; i++) {
        if (merged > 0 && mono_cmp(&buffer[merged - 1].mono, &buffer[i].mono) == 0)
            buffer[merged - 1].coef = num_add(buffer[merged - 1].coef, buffer[i].coef);
        else
            buffer[merged++] = buffer[i];
    }

    /* Elimina i termini che la somma ha annullato. */
    int count = 0;
    for (int i = 0; i < merged; i++)
        if (!num_is_zero(buffer[i].coef)) buffer[count++] = buffer[i];

    /* Restituisce all'arena lo spazio non usato. Si può fare solo se il buffer
     * è l'ultimo blocco assegnato, ed è il caso normale: il prodotto di due
     * polinomi chiede spazio per tutti i prodotti parziali, quasi tutti poi
     * fusi tra loro. Senza questo passo l'arena si riempirebbe di vuoti. */
    size_t old_bytes = arena_round((size_t)capacity * sizeof(Term));
    size_t new_bytes = arena_round((size_t)count * sizeof(Term));
    if ((char *)buffer + old_bytes == arena + arena_used) arena_used -= old_bytes - new_bytes;

    return (Poly){count, count ? buffer : NULL};
}

/* Ritorna il polinomio formato dal solo termine t (o zero se il coefficiente è
 * nullo), dopo aver applicato le identità di term_reduce(). */
static Poly poly_term(Term t) {
    term_reduce(&t);
    Term *buffer = terms_alloc(1);
    if (!buffer) return (Poly){0, NULL};
    buffer[0] = t;
    return poly_finish(buffer, 1, 1);
}

/* Ritorna il polinomio costante c. */
static Poly poly_const(Num c) {
    Term t;
    t.coef = c;
    t.mono.count = 0;
    return poly_term(t);
}

/* Ritorna a + b, oppure a - b se subtract è diverso da zero. */
static Poly poly_add(Poly a, Poly b, int subtract) {
    long long capacity = (long long)a.count + b.count;
    Term *buffer = terms_alloc(capacity);
    if (!buffer) return (Poly){0, NULL};
    for (int i = 0; i < a.count; i++) buffer[i] = a.term[i];
    for (int i = 0; i < b.count; i++) {
        buffer[a.count + i] = b.term[i];
        if (subtract) buffer[a.count + i].coef = num_neg(b.term[i].coef);
    }
    return poly_finish(buffer, capacity, (int)capacity);
}

/* Ritorna a * b: ogni termine di a per ogni termine di b. */
static Poly poly_mul(Poly a, Poly b) {
    long long capacity = (long long)a.count * b.count;
    Term *buffer = terms_alloc(capacity);
    if (!buffer) return (Poly){0, NULL};
    int used = 0;
    for (int i = 0; i < a.count; i++)
        for (int j = 0; j < b.count; j++) {
            Term t;
            t.coef = num_mul(a.term[i].coef, b.term[j].coef);
            t.mono = mono_mul(&a.term[i].mono, &b.term[j].mono);
            term_reduce(&t);
            buffer[used++] = t;
        }
    return poly_finish(buffer, capacity, used);
}

/* Ritorna 1 se p è il polinomio costante 1. */
static int poly_is_one(Poly p) {
    return p.count == 1 && p.term[0].mono.count == 0 && num_equal(p.term[0].coef, num_int(1));
}

/* Ritorna 1 se a e b sono lo stesso polinomio. Grazie alla forma canonica
 * basta il confronto termine per termine. */
static int poly_equal(Poly a, Poly b) {
    if (a.count != b.count) return 0;
    for (int i = 0; i < a.count; i++)
        if (mono_cmp(&a.term[i].mono, &b.term[i].mono) != 0 || !num_equal(a.term[i].coef, b.term[i].coef))
            return 0;
    return 1;
}

/* Ritorna 1 se a = lambda * b per una costante lambda non nulla. */
static int poly_proportional(Poly a, Poly b) {
    if (a.count != b.count || a.count == 0) return 0;
    Num ratio = num_mul(a.term[0].coef, num_inv(b.term[0].coef));
    for (int i = 0; i < a.count; i++)
        if (mono_cmp(&a.term[i].mono, &b.term[i].mono) != 0 ||
            !num_equal(a.term[i].coef, num_mul(ratio, b.term[i].coef)))
            return 0;
    return 1;
}

/* Ritorna 1 se in p compare un atomo che dipende da n. */
static int poly_depends_on_n(Poly p) {
    for (int i = 0; i < p.count; i++)
        if (mono_depends_on_n(&p.term[i].mono)) return 1;
    return 0;
}

/* Scompone p secondo le potenze dell'atomo x: p = part[0] + part[1] x + ... +
 * part[d] x^d, con le part[j] prive di x. Ritorna il grado d (0 se x non
 * compare), oppure -1 se x compare con esponente negativo, frazionario o
 * maggiore di max_degree. part deve avere max_degree + 1 posti. */
static int poly_split(Poly p, int x, Poly *part, int max_degree) {
    int degree = 0;
    for (int j = 0; j <= max_degree; j++) {
        Term *buffer = terms_alloc(p.count);
        if (!buffer && p.count) return -1;
        int used = 0;
        for (int i = 0; i < p.count; i++) {
            /* Il termine va in part[j] se x vi compare alla j, senza x. */
            Term t = p.term[i];
            Frac e = mono_exponent(&t.mono, x);
            if (e.den != 1 || e.num < 0 || e.num > max_degree) return -1;
            if (e.num != j) continue;
            t.mono = mono_with_exponent(&t.mono, x, frac_int(0));
            buffer[used++] = t;
            if (j > degree) degree = j;
        }
        part[j] = poly_finish(buffer, p.count, used);
    }
    return degree;
}

/* Legge p come polinomio nel solo atomo x, a coefficienti razionali.
 *
 * Se p ha questa forma, con grado al più MAX_UNIVARIATE, mette in *scale il
 * più piccolo intero positivo che moltiplicato per p dà coefficienti interi,
 * in c[0..MAX_UNIVARIATE] quei coefficienti (zero dove mancano) e ritorna il
 * grado, 0 per il polinomio nullo. Altrimenti ritorna -1: quando compare un
 * altro atomo, un esponente negativo o non intero, un coefficiente complesso,
 * oppure un numero oltre il miliardo. Quest'ultimo tetto tiene lontani
 * dall'overflow i conti di chi poi usa i coefficienti. */
static int poly_coefficients(Poly p, int x, long long *c, long long *scale) {
    const long long cap = 1000000000;
    int degree = 0;

    /* Il moltiplicatore: il minimo comune multiplo dei denominatori. */
    *scale = 1;
    for (int i = 0; i < p.count; i++) {
        long long den = p.term[i].coef.re.den;
        __int128 multiple = (__int128)*scale / gcd128(*scale, den) * den;
        if (multiple > cap) return -1;
        *scale = (long long)multiple;
    }

    /* I coefficienti, ciascuno al posto del suo esponente. */
    for (int j = 0; j <= MAX_UNIVARIATE; j++) c[j] = 0;
    for (int i = 0; i < p.count; i++) {
        const Term *t = &p.term[i];
        Frac e = frac_int(0);
        if (t->mono.count == 1 && t->mono.factor[0].atom == x) e = t->mono.factor[0].exp;
        else if (t->mono.count != 0) return -1;
        if (t->coef.im.num != 0 || e.den != 1 || e.num < 0 || e.num > MAX_UNIVARIATE) return -1;
        __int128 value = (__int128)t->coef.re.num * (*scale / t->coef.re.den);
        if (value > cap || value < -cap) return -1;
        c[e.num] = (long long)value;
        if (e.num > degree) degree = (int)e.num;
    }
    return degree;
}

/* =============================================================================
 * 6. CREAZIONE DEGLI ATOMI
 * ============================================================================
 *
 * Ogni atomo è registrato una volta sola: chiedere due volte "2^n" restituisce
 * lo stesso indice. È questo a far sì che due occorrenze della stessa quantità
 * si riconoscano e si cancellino.
 */

/* Polinomi che fanno da base a un atomo POW con BASE_POLY, ciascuno una volta. */
static Poly base_polys[MAX_ATOMS];
static int base_poly_count;

/* Prototipo in avanti necessario: il confronto tra due somme guarda dentro i
 * nodi dell'albero, che sono definiti nel capitolo 8. */
static int bigop_same(const Node *a, const Node *b);

/* Cerca nella tabella un atomo uguale a proto e ne ritorna l'indice;
 * se non c'è lo aggiunge. Due atomi sono uguali se coincidono in tutti i campi
 * che li identificano; per i BIGOP l'offset non conta (vedi atom_bigop()), e
 * le somme si confrontano per come sono scritte, con bigop_same(). */
static int atom_intern(const Atom *proto) {
    for (int i = 0; i < atom_count; i++) {
        const Atom *a = &atoms[i];
        if (a->kind != proto->kind || a->id != proto->id || a->dilation != proto->dilation || a->base != proto->base)
            continue;
        if (a->kind == ATOM_BIGOP && !bigop_same(a->node, proto->node)) continue;
        if (a->kind == ATOM_REC && a->offset != proto->offset) continue;
        if (a->kind == ATOM_POW && mono_cmp(&a->exponent, &proto->exponent) != 0) continue;
        return i;
    }
    if (atom_count == MAX_ATOMS) { fail("troppe quantità distinte nell'enunciato"); return 0; }
    atoms[atom_count] = *proto;
    return atom_count++;
}

/* Ritorna l'atomo della variabile name. */
static int atom_var(int name) {
    Atom a = {0};
    a.kind = ATOM_VAR;
    a.id = name;
    a.depends_on_n = (name == induction_var);
    return atom_intern(&a);
}

/* Ritorna l'atomo della somma (o prodotto) definita da node con estremo
 * superiore dilation * n + offset.
 *
 * L'offset vale solo alla prima richiesta: fissa quale somma l'atomo
 * rappresenta, e le richieste successive con la stessa somma e la stessa
 * dilation ritornano quell'atomo anche con un offset diverso. Sta al chiamante
 * aggiungere o togliere i termini di differenza (vedi eval_bigop()).
 *
 * "La stessa somma" vuol dire scritta allo stesso modo, non lo stesso nodo:
 * due copie della somma dei k da 1 a n nello stesso enunciato sono un atomo
 * solo. Prima erano due lettere indipendenti, e il programma non riusciva a
 * dimostrare nemmeno che 2 diviso un prodotto è uguale a se stesso: dove le
 * due copie erano moltiplicate o divise per qualcosa, l'ipotesi induttiva non
 * bastava a legarle. */
static int atom_bigop(const Node *node, long long dilation, long long offset) {
    Atom a = {0};
    a.kind = ATOM_BIGOP;
    a.node = node;
    a.dilation = dilation;
    a.offset = offset;
    a.depends_on_n = 1;
    return atom_intern(&a);
}

/* Ritorna l'atomo X(dilation * n + offset) della successione del vocabolario
 * in posizione sequence. */
static int atom_rec(int sequence, long long dilation, long long offset) {
    Atom a = {0};
    a.kind = ATOM_REC;
    a.id = sequence;
    a.dilation = dilation;
    a.offset = offset;
    a.depends_on_n = 1;
    return atom_intern(&a);
}

/* Ritorna l'atomo base^exponent, con la base descritta da base e id. */
static int atom_pow(BaseKind base, long long id, const Mono *exponent) {
    Atom a = {0};
    a.kind = ATOM_POW;
    a.base = base;
    a.id = id;
    a.exponent = *exponent;

    /* Dipende da n se ne dipende l'esponente oppure la base. */
    a.depends_on_n = mono_depends_on_n(exponent) ||
                     (base == BASE_ATOM && atoms[id].depends_on_n) ||
                     (base == BASE_POLY && poly_depends_on_n(base_polys[id]));

    /* Riconosce e^{i pi} ed e^{i pi n^j}, a cui si applicano le identità di
     * term_reduce(): l'esponente deve essere pi per (eventualmente) una
     * potenza intera positiva di n, e nient'altro. */
    if (base == BASE_EI) {
        int has_pi = 0, has_n = 0, other = 0;
        for (int i = 0; i < exponent->count; i++) {
            const Atom *f = &atoms[exponent->factor[i].atom];
            Frac e = exponent->factor[i].exp;
            if (f->kind == ATOM_VAR && f->id == VAR_PI && e.num == 1 && e.den == 1) has_pi = 1;
            else if (f->kind == ATOM_VAR && f->id == induction_var && e.den == 1 && e.num > 0) has_n = 1;
            else other = 1;
        }
        if (has_pi && !other) a.pi = has_n ? PI_INTEGER : PI_CONST;
    }
    return atom_intern(&a);
}

/* Ritorna l'atomo ln|base|, con la base descritta da base e id. */
static int atom_ln(BaseKind base, long long id) {
    Atom a = {0};
    a.kind = ATOM_LN;
    a.base = base;
    a.id = id;
    a.depends_on_n = (base == BASE_ATOM && atoms[id].depends_on_n) ||
                     (base == BASE_POLY && poly_depends_on_n(base_polys[id]));
    return atom_intern(&a);
}

/* Ritorna la posizione di p in base_polys, aggiungendolo se manca. */
static int base_poly_intern(Poly p) {
    for (int i = 0; i < base_poly_count; i++)
        if (poly_equal(base_polys[i], p)) return i;
    if (base_poly_count == MAX_ATOMS) { fail("troppe basi distinte nell'enunciato"); return 0; }
    base_polys[base_poly_count] = p;
    return base_poly_count++;
}

/* =============================================================================
 * 7. FRAZIONI DI POLINOMI
 * ============================================================================
 *
 * Il valore di un'espressione è una frazione num/den di polinomi negli atomi,
 * con le regole delle frazioni di scuola:
 *
 *     a1/b1 + a2/b2 = (a1 b2 + a2 b1) / (b1 b2)
 *     a1/b1 * a2/b2 = (a1 a2) / (b1 b2)
 *     a1/b1 : a2/b2 = (a1 b2) / (b1 a2)
 *
 * Le frazioni non vengono ridotte ai minimi termini (il perché è nel commento
 * di design), con un'eccezione che non costa nulla: se il denominatore è un
 * solo termine lo si porta al numeratore cambiando segno agli esponenti. Così
 * 1/2^n diventa (2^n)^{-1} e i denominatori restano piccoli.
 *
 * Invariante: senza errori den non è mai il polinomio nullo. Per questo "la
 * frazione è zero" equivale a "num non ha termini".
 */

/* La frazione num/den. */
typedef struct { Poly num, den; } Rat;

/* Ritorna num/den, applicando la semplificazione del denominatore monomio.
 * Fallisce se den è zero. */
static Rat rat_make(Poly num, Poly den) {
    Rat r = {num, den};
    if (den.count == 0) {
        fail_undefined("divisione per zero");
        r.den = poly_const(num_int(1));
    } else if (den.count == 1 && !poly_is_one(den)) {
        Term inverse;
        inverse.coef = num_inv(den.term[0].coef);
        inverse.mono = mono_pow(&den.term[0].mono, frac_int(-1));
        r.num = poly_mul(num, poly_term(inverse));
        r.den = poly_const(num_int(1));
    }
    return r;
}

/* Frazioni elementari: l'intero v, il razionale v e l'atomo `atom`. */
static Rat rat_int(long long v) { return (Rat){poly_const(num_int(v)), poly_const(num_int(1))}; }
static Rat rat_frac(Frac v) { return (Rat){poly_const(num_frac(v)), poly_const(num_int(1))}; }
static Rat rat_atom(int atom) {
    Term t;
    t.coef = num_int(1);
    t.mono = mono_atom(atom, frac_int(1));
    return (Rat){poly_term(t), poly_const(num_int(1))};
}

/* Ritorna a + b, oppure a - b se subtract è diverso da zero. Se i denominatori
 * coincidono si sommano solo i numeratori: capita spesso, e tiene piccole le
 * frazioni. */
static Rat rat_add(Rat a, Rat b, int subtract) {
    if (poly_equal(a.den, b.den)) return rat_make(poly_add(a.num, b.num, subtract), a.den);
    return rat_make(poly_add(poly_mul(a.num, b.den), poly_mul(b.num, a.den), subtract),
                    poly_mul(a.den, b.den));
}

/* Ritorna a * b. */
static Rat rat_mul(Rat a, Rat b) { return rat_make(poly_mul(a.num, b.num), poly_mul(a.den, b.den)); }

/* CONDIZIONI DI ESISTENZA
 *
 * Una divisione, un logaritmo, una radice hanno senso solo se ciò su cui
 * operano rispetta una condizione: diverso da zero, positivo, non negativo.
 * Quando il valore è un numero la condizione si controlla sul momento (vedi
 * fail_undefined()). Quando è un'espressione in n non si può: vale per certi
 * n e non per altri. Allora la si annota in questo elenco, e chi ha chiesto
 * il conto decide poi se vale per ogni n (capitolo 13, 13c).
 *
 * L'elenco si riempie solo mentre conditions_on è diverso da zero: durante la
 * dimostrazione resta spento, perché lì le condizioni sono ipotesi
 * dell'enunciato ("vale dove i due membri sono definiti"). */

/* Cosa si chiede a un'espressione perché un'operazione sia definita. */
typedef enum { NEVER_ZERO, ALWAYS_POSITIVE, NEVER_NEGATIVE } Requirement;

/* Una condizione di esistenza: il prodotto dei `count` polinomi di factor[]
 * deve rispettare `kind` per ogni n ammesso. Quasi sempre il fattore è uno
 * solo; sono di più quando chi pone la condizione conosce l'espressione già
 * divisa in fattori, e conviene non perderli moltiplicandoli (vedi
 * require_positive() nel capitolo 11). `why` dice che cosa succede se la
 * condizione non vale, con le stesse parole di fail_undefined(). I termini
 * dei polinomi stanno nell'arena: la condizione vale finché l'arena non viene
 * svuotata. */
typedef struct {
    Requirement kind;
    int count;                      /* fattori validi in factor: almeno 1 */
    Poly factor[MAX_SIGN_FACTORS];
    const char *why;
} Condition;

static Condition conditions[MAX_CONDITIONS];
static int condition_count;
static int conditions_on;    /* 1 mentre le condizioni vanno annotate */
static int conditions_lost;  /* 1 se l'elenco non è bastato a contenerle */

/* Il perché di una divisione che non è scritta nell'enunciato: la fa il
 * conto, quando riscrive un prodotto fino a n - 1 come il prodotto fino a n
 * diviso per l'ultimo fattore (vedi eval_bigop()). `rewriting` è diverso da
 * zero mentre il conto fa una divisione di questo tipo. */
static const char *const REWRITTEN = "la dimostrazione divide un prodotto per un suo fattore, che lì vale zero";
static int rewriting;

/* Annota che il prodotto dei `count` polinomi di factor[] deve rispettare
 * kind, se l'elenco è acceso. Una condizione già presente non viene ripetuta.
 * Dividere per un numero diverso da zero non pone condizioni, e lo zero è già
 * stato rifiutato da chi divide. */
static void require_product(Requirement kind, const Poly *factor, int count, const char *why) {
    if (!conditions_on || failure) return;
    if (kind == NEVER_ZERO && count == 1 && factor[0].count == 1 && factor[0].term[0].mono.count == 0) return;
    for (int i = 0; i < condition_count; i++) {
        int same = conditions[i].kind == kind && conditions[i].count == count;
        for (int f = 0; f < count && same; f++) same = poly_equal(conditions[i].factor[f], factor[f]);
        if (same) return;
    }
    if (condition_count == MAX_CONDITIONS || count > MAX_SIGN_FACTORS) { conditions_lost = 1; return; }
    Condition *c = &conditions[condition_count++];
    c->kind = kind;
    c->count = count;
    c->why = why;
    for (int f = 0; f < count; f++) c->factor[f] = factor[f];
}

/* Annota che value deve rispettare kind. */
static void require(Requirement kind, Poly value, const char *why) { require_product(kind, &value, 1, why); }

/* Ritorna a / b. Fallisce se b è la frazione nulla, e annota che b non deve
 * annullarsi (il suo denominatore è già stato annotato quando b è nata da
 * un'altra divisione). */
static Rat rat_div(Rat a, Rat b) {
    require(NEVER_ZERO, b.num, rewriting ? REWRITTEN : "divisione per zero");
    return rat_make(poly_mul(a.num, b.den), poly_mul(a.den, b.num));
}

/* Ritorna a^e per e intero, anche negativo, con a^0 = 1. Moltiplicazioni
 * ripetute: gli esponenti costanti degli enunciati sono piccoli. */
static Rat rat_pow_int(Rat a, long long e) {
    if (e < -MAX_POWER || e > MAX_POWER) { fail("esponente costante troppo grande"); return rat_int(0); }
    if (e < 0) { a = rat_div(rat_int(1), a); e = -e; }
    Rat r = rat_int(1);
    for (long long i = 0; i < e && !failure; i++) r = rat_mul(r, a);
    return r;
}

/* Ritorna p con l'atomo x sostituito da value. In p l'atomo deve comparire
 * con esponenti interi tra 0 e MAX_UNIVARIATE, altrimenti fallisce.
 *
 * Schema di Horner: p = c0 + c1 x + ... + cd x^d si calcola come
 * (...(cd x + c(d-1)) x + ...) x + c0, con d moltiplicazioni. */
static Rat poly_substitute(Poly p, int x, Rat value) {
    Poly part[MAX_UNIVARIATE + 1], one = poly_const(num_int(1));
    int degree = poly_split(p, x, part, MAX_UNIVARIATE);
    if (degree < 0) { fail("polinomio di grado troppo alto in una variabile da sostituire"); return rat_int(0); }
    Rat result = {part[degree], one};
    for (int j = degree - 1; j >= 0; j--) result = rat_add(rat_mul(result, value), (Rat){part[j], one}, 0);
    return result;
}

/* Se r è un numero (nessun atomo) lo mette in *value e ritorna 1. */
static int rat_number(Rat r, Num *value) {
    if (!poly_is_one(r.den)) return 0;
    if (r.num.count == 0) { *value = num_int(0); return 1; }
    if (r.num.count != 1 || r.num.term[0].mono.count != 0) return 0;
    *value = r.num.term[0].coef;
    return 1;
}

/* Se r è un numero intero lo mette in *value e ritorna 1. */
static int rat_integer(Rat r, long long *value) {
    Num v;
    if (!rat_number(r, &v) || v.im.num != 0 || v.re.den != 1) return 0;
    *value = v.re.num;
    return 1;
}

/* Se r ha la forma a * n + c con a e c interi (n è la variabile di induzione)
 * mette i due numeri in *a e *c e ritorna 1. Un intero è il caso a = 0. */
static int rat_affine(Rat r, long long *a, long long *c) {
    int n = atom_var(induction_var);
    *a = *c = 0;
    if (!poly_is_one(r.den)) return 0;
    for (int i = 0; i < r.num.count; i++) {
        const Term *t = &r.num.term[i];
        if (t->coef.im.num != 0 || t->coef.re.den != 1) return 0;
        if (t->mono.count == 0) *c = t->coef.re.num;
        else if (t->mono.count == 1 && t->mono.factor[0].atom == n &&
                 t->mono.factor[0].exp.num == 1 && t->mono.factor[0].exp.den == 1)
            *a = t->coef.re.num;
        else return 0;
    }
    return 1;
}

/* =============================================================================
 * 8. ALBERO SINTATTICO
 * ============================================================================
 *
 * La forma in cui un enunciato passa dal parsing alla valutazione. L'albero
 * registra solo com'è scritta la formula: che `F_{k}` sia un numero di
 * Fibonacci o che `!` sia un prodotto lo stabilisce la valutazione,
 * consultando il vocabolario.
 */

/* Tipi di nodo. Tra parentesi i figli usati.
 *
 * Warning: se aggiungi un tipo, aggiungi il ramo che lo costruisce nel parser
 * (capitolo 9) e il suo case in eval() (capitolo 11); se può comparire in un
 * enunciato con quantificatori, anche in term_check() (capitolo 17). */
typedef enum {
    N_NUM,    /* intero letterale: value */
    N_VAR,    /* variabile: name */
    N_SEQ,    /* lettera con pedice, come F_{k}: name, a = pedice */
    N_NEG,    /* -a */
    N_ADD,    /* a + b */
    N_SUB,    /* a - b */
    N_MUL,    /* a * b */
    N_DIV,    /* a / b */
    N_POW,    /* a ^ b */
    N_BIGOP,  /* somma o prodotto: name = indice, a = estremo inferiore,
                 b = estremo superiore, c = corpo, op = 1 se è un prodotto */
    N_CALL,   /* funzione: op = quale (Function), a = argomento, b = secondo
                 argomento se c'è */
    N_APPLY   /* applicazione dichiarata dall'enunciato, come f(x): name = la
                 lettera f, a = argomento */
} NodeKind;

/* Le funzioni che il parser riconosce. Il secondo argomento, quando esiste, è
 * indicato tra parentesi.
 *
 * Warning: se aggiungi una funzione, aggiungi il suo nome in function_names[]
 * (capitolo 9), se è un comando LaTeX, e il suo case in eval_call(). */
typedef enum {
    FN_FACT,    /* a! */
    FN_BINOM,   /* coefficiente binomiale "a su b" */
    FN_SIN, FN_COS, FN_TAN, FN_COT,
    FN_ARCTAN,
    FN_LN,
    FN_LOG,     /* logaritmo di a in base b */
    FN_ROOT,    /* radice di a di indice b */
    FN_ABS      /* valore assoluto */
} Function;

/* Un nodo dell'albero: una sotto-espressione dell'enunciato. I figli non usati
 * dal tipo sono NULL. I nodi vivono nell'arena. */
struct Node {
    NodeKind kind;
    long long value;  /* N_NUM */
    int name;         /* N_VAR, N_SEQ, N_BIGOP: una lettera, o VAR_PI */
    int op;           /* N_BIGOP, N_CALL */
    Node *a, *b, *c;
};

/* Ritorna 1 se i due alberi sono scritti allo stesso modo, nodo per nodo. */
static int node_equal(const Node *a, const Node *b) {
    if (a == b) return 1;
    if (!a || !b) return 0;
    return a->kind == b->kind && a->value == b->value && a->name == b->name && a->op == b->op &&
           node_equal(a->a, b->a) && node_equal(a->b, b->b) && node_equal(a->c, b->c);
}

/* Ritorna 1 se le somme (o i prodotti) a e b aggiungono gli stessi termini
 * partendo dallo stesso punto: stessa operazione, stesso indice, stesso
 * estremo inferiore, stesso corpo. L'estremo superiore non si guarda: due
 * somme che differiscono solo lì sono la stessa successione di somme parziali
 * presa in due punti, ed è ciò che un atomo BIGOP rappresenta.
 *
 * È lecito identificarle perché un atomo nasce solo per somme il cui corpo e
 * il cui estremo inferiore non dipendono da variabili legate (lo controlla
 * eval_bigop()): due scritture uguali hanno allora lo stesso valore. */
static int bigop_same(const Node *a, const Node *b) {
    return a == b || (a->op == b->op && a->name == b->name && node_equal(a->a, b->a) && node_equal(a->c, b->c));
}

/* Un insieme di numeri razionali tra quelli che un enunciato può nominare:
 * \mathbb{N}, \mathbb{N}_{0}, \mathbb{Z}, \mathbb{Q}, \mathbb{Q}^{+}, privati
 * eventualmente di qualche punto con \setminus. È l'insieme su cui varia la
 * variabile di un quantificatore, oppure il dominio o il codominio di
 * un'applicazione. Le operazioni sono nel capitolo 17.
 *
 * Invariante: i punti tolti sono distinti e appartengono tutti all'insieme di
 * partenza, così due insiemi uguali hanno gli stessi punti in `hole`. */
typedef struct {
    int integers;             /* 1: solo numeri interi; 0: tutti i razionali */
    int bounded;              /* 1: c'è un estremo inferiore, `low` */
    long long low;            /* l'estremo inferiore, se bounded */
    int strict;               /* 1: low è escluso (x > low); 0: è compreso */
    int holes;                /* punti tolti, validi in `hole` */
    Frac hole[MAX_HOLES];
    const char *start, *end;  /* com'è scritto nell'enunciato: [start, end) */
} NumberSet;

/* Un'applicazione dichiarata dall'enunciato con "f \colon A \to B". La regola
 * che la definisce, "f(x) = espressione", arriva dopo, da un'altra ipotesi:
 * finché manca, body è NULL e parameter non ha significato. */
typedef struct {
    int name;                    /* la lettera f */
    NumberSet domain, codomain;  /* A e B */
    int parameter;               /* la variabile x della regola */
    Node *body;                  /* il valore di f(x), nell'arena; NULL se manca */
} Map;

/* Le applicazioni dichiarate dall'enunciato in corso. Chi le dichiara deve
 * svuotare la tabella quando ha finito: finché una lettera è qui, il parser
 * delle espressioni la legge come applicazione e non come variabile. */
static Map maps[MAX_MAPS];
static int map_count;

/* Ritorna l'applicazione di nome name, o NULL se non è stata dichiarata. */
static Map *map_find(int name) {
    for (int i = 0; i < map_count; i++)
        if (maps[i].name == name) return &maps[i];
    return NULL;
}

/* Come sono messi in relazione i due membri di un enunciato. */
typedef enum { REL_EQ, REL_GE, REL_LE, REL_GT, REL_LT } Relation;

/* Un enunciato: membro sinistro, relazione, membro destro. */
typedef struct {
    Node *lhs, *rhs;
    Relation relation;
} Statement;

/* =============================================================================
 * 9. PARSING: DAL LATEX ALL'ALBERO
 * ============================================================================
 *
 * Il sottoinsieme di LaTeX accettato, dalla precedenza più bassa alla più alta:
 *
 *     statement := expr ('=' | \ge | \le | '<' | '>') expr
 *     expr      := ['-' | '+'] term { ('+' | '-') term }
 *     term      := factor { [\cdot | '/'] factor }
 *     factor    := primary { '!' | '^' group }
 *     primary   := numero | lettera ['_' group] | \pi
 *                | applicazione '(' expr ')'
 *                | '(' expr ')' | \left| expr \right|
 *                | \frac group group | \binom group group
 *                | \sqrt ['[' expr ']'] group
 *                | (\sum | \prod) '_' '{' lettera '=' expr '}' '^' group term
 *                | funzione ['^' group] argomento
 *     group     := '{' expr '}' | un solo token
 *     argomento := '(' expr ')' | factor
 *
 * Le convenzioni che la grammatica fissa, e che in LaTeX sono implicite:
 *
 *   - Due fattori accostati si moltiplicano: `2k`, `n(n+1)`, `k k!`.
 *   - Una somma si estende a tutto il term che la segue: in `\sum f g + h`
 *     il sommando è f g, e h resta fuori.
 *   - L'argomento di una funzione senza parentesi è un solo fattore:
 *     `\sin x y` vale (sin x) y.
 *   - `\sin^{2}(x)` è il quadrato di sin(x).
 *   - Senza graffe un esponente o un pedice è un solo token: `x^12` vale
 *     x^1 seguito da 2, come in TeX.
 *
 * La tecnica è la discesa ricorsiva: una funzione per regola, e ogni funzione
 * chiama quella della regola di precedenza superiore.
 *
 * Errori: al primo problema syntax() annota il motivo e porta il cursore a
 * fine testo, così tutte le funzioni in corso vedono "fine" e ritornano senza
 * bisogno di propagare un codice.
 */

/* Tipi di token. \left( e ( producono entrambi T_OPEN: per il valore assoluto
 * invece \left| e \right| restano distinti, perché una sbarra da sola non
 * dice se apre o chiude. */
typedef enum { T_END, T_NUM, T_LETTER, T_COMMAND, T_OPEN, T_CLOSE, T_ABS_OPEN, T_ABS_CLOSE, T_CHAR } TokenKind;

/* Il token corrente: l'unità che il parser guarda per decidere. */
typedef struct {
    TokenKind kind;
    long long value;    /* T_NUM */
    int digits;         /* T_NUM: numero di cifre */
    int ch;             /* T_LETTER, T_CHAR: il carattere */
    char name[24];      /* T_COMMAND: il nome senza barra */
    const char *start;  /* dove inizia nel testo */
} Token;

/* Stato del parser, valido durante una chiamata a parse_statement() o
 * parse_expression(). */
static const char *cursor;        /* prossimo carattere da leggere */
static Token token;               /* token corrente */
static const char *consumed;      /* dove finisce l'ultimo token già superato */
static const char *syntax_error;  /* primo errore, o NULL */

/* Annota message come errore di sintassi (se è il primo) e ferma il parsing. */
static void syntax(const char *message) {
    if (!syntax_error) syntax_error = message;
    cursor = "";
    token.kind = T_END;
}

/* Legge il prossimo token in `token`. I comandi di sola presentazione
 * (spazi come \, e \!, \displaystyle) vengono saltati. */
static void next_token(void) {
    static const char *const ignored[] = {"displaystyle", "textstyle", "quad", "qquad", "limits"};

    /* Il token che si sta lasciando finisce dove il cursore è adesso: chi ha
     * appena letto un pezzo di enunciato sa così dove quel pezzo termina, e
     * può citarlo nella dimostrazione scritta (capitolo 17). */
    consumed = cursor;
    for (;;) {
        while (isspace((unsigned char)*cursor)) cursor++;
        token.start = cursor;
        unsigned char c = (unsigned char)*cursor;

        /* Fine del testo. */
        if (c == 0) { token.kind = T_END; return; }

        /* Numero intero. */
        if (isdigit(c)) {
            token.kind = T_NUM;
            token.value = 0;
            token.digits = 0;
            while (isdigit((unsigned char)*cursor)) {
                if (token.value > (LLONG_MAX - 9) / 10) { syntax("numero troppo grande"); return; }
                token.value = token.value * 10 + (*cursor++ - '0');
                token.digits++;
            }
            return;
        }

        /* Lettera: ogni lettera è un simbolo a sé. */
        if (isalpha(c)) { token.kind = T_LETTER; token.ch = *cursor++; return; }

        /* Comando: barra seguita da lettere, oppure spazio come \, */
        if (c == '\\') {
            cursor++;
            if (!isalpha((unsigned char)*cursor)) {
                if (*cursor && strchr(",;!: ", *cursor)) { cursor++; continue; }
                if (*cursor == '{' || *cursor == '}') {
                    token.kind = T_CHAR;
                    token.ch = ESCAPED + *cursor++;
                    return;
                }
                syntax("comando LaTeX non riconosciuto");
                return;
            }
            size_t length = 0;
            while (isalpha((unsigned char)*cursor)) {
                if (length + 1 < sizeof token.name) token.name[length++] = *cursor;
                cursor++;
            }
            token.name[length] = 0;

            /* \left e \right qualificano il delimitatore che segue. */
            int left = strcmp(token.name, "left") == 0, right = strcmp(token.name, "right") == 0;
            if (left || right) {
                while (isspace((unsigned char)*cursor)) cursor++;
                char d = *cursor;
                if (d == '|') token.kind = left ? T_ABS_OPEN : T_ABS_CLOSE;
                else if (left && (d == '(' || d == '[')) token.kind = T_OPEN;
                else if (right && (d == ')' || d == ']')) token.kind = T_CLOSE;
                else { syntax("delimitatore non riconosciuto dopo \\left o \\right"); return; }
                cursor++;
                return;
            }

            /* Comandi di sola presentazione: non producono token. */
            int skip = 0;
            for (size_t i = 0; i < sizeof ignored / sizeof *ignored; i++)
                if (strcmp(token.name, ignored[i]) == 0) skip = 1;
            if (skip) continue;

            token.kind = T_COMMAND;
            return;
        }

        /* Parentesi tonde e ogni altro carattere singolo. */
        cursor++;
        if (c == '(') token.kind = T_OPEN;
        else if (c == ')') token.kind = T_CLOSE;
        else { token.kind = T_CHAR; token.ch = c; }
        return;
    }
}

/* Ritorna 1 se il token corrente è il carattere c. */
static int is_char(int c) { return token.kind == T_CHAR && token.ch == c; }

/* Ritorna 1 se il token corrente è il comando \name. */
static int is_command(const char *name) { return token.kind == T_COMMAND && strcmp(token.name, name) == 0; }

/* Consuma il carattere c, o segnala che manca. */
static void expect_char(int c, const char *message) {
    if (is_char(c)) next_token(); else syntax(message);
}

/* Ritorna un nuovo nodo di tipo kind con figli a e b; gli altri campi sono a
 * zero. Se l'arena è piena ritorna un nodo di riserva, così il parser non deve
 * controllare ogni allocazione: l'errore è già in `failure`. */
static Node *node_new(NodeKind kind, Node *a, Node *b) {
    static Node spare;
    Node *node = arena_alloc(sizeof(Node));
    if (!node) node = &spare;
    memset(node, 0, sizeof *node);
    node->kind = kind;
    node->a = a;
    node->b = b;
    return node;
}

/* Ritorna il nodo dell'intero value. */
static Node *node_num(long long value) {
    Node *node = node_new(N_NUM, NULL, NULL);
    node->value = value;
    return node;
}

/* Ritorna il nodo della funzione fn applicata ad a (e b, se serve). */
static Node *node_call(Function fn, Node *a, Node *b) {
    Node *node = node_new(N_CALL, a, b);
    node->op = fn;
    return node;
}

/* Comandi LaTeX che sono funzioni, e la funzione corrispondente. */
static const struct { const char *name; Function fn; } function_names[] = {
    {"sin", FN_SIN}, {"cos", FN_COS}, {"tan", FN_TAN}, {"cot", FN_COT},
    {"arctan", FN_ARCTAN}, {"ln", FN_LN}, {"log", FN_LOG},
};

/* Se il token corrente è il comando di una funzione mette la funzione in *fn
 * e ritorna 1. */
static int is_function(Function *fn) {
    for (size_t i = 0; i < sizeof function_names / sizeof *function_names; i++)
        if (is_command(function_names[i].name)) { *fn = function_names[i].fn; return 1; }
    return 0;
}

/* Prototipi in avanti inevitabili: la grammatica è ricorsiva. */
static Node *parse_expr(void);
static Node *parse_term(void);
static Node *parse_factor(void);
static Node *parse_primary(void);

/* group := '{' expr '}' | un solo token */
static Node *parse_group(void) {
    if (is_char('{')) {
        next_token();
        Node *inner = parse_expr();
        expect_char('}', "manca }");
        return inner;
    }

    /* Senza graffe TeX prende un token solo, e per TeX ogni cifra è un token:
     * `\frac12` è un mezzo. Il nostro lettore ha già raggruppato le cifre,
     * quindi ne teniamo una e riportiamo il cursore subito dopo. */
    if (token.kind == T_NUM && token.digits > 1) {
        Node *digit = node_num(token.start[0] - '0');
        cursor = token.start + 1;
        next_token();
        return digit;
    }
    return parse_primary();
}

/* (\sum | \prod) '_' '{' lettera '=' expr '}' '^' group term
 *
 * Il token corrente è \sum o \prod. */
static Node *parse_bigop(void) {
    int product = is_command("prod");
    next_token();

    /* Pedice: indice ed estremo inferiore. Il segno '=' qui dentro è parte
     * della notazione, non la relazione dell'enunciato. */
    expect_char('_', "dopo \\sum o \\prod serve il pedice con l'indice");
    expect_char('{', "il pedice di \\sum o \\prod va tra graffe");
    if (token.kind != T_LETTER) { syntax("l'indice di \\sum o \\prod deve essere una lettera"); return node_num(0); }
    int index = token.ch;
    next_token();
    expect_char('=', "manca = nel pedice di \\sum o \\prod");
    Node *lower = parse_expr();
    expect_char('}', "manca } nel pedice di \\sum o \\prod");

    /* Apice: estremo superiore. */
    expect_char('^', "manca l'estremo superiore di \\sum o \\prod");
    Node *upper = parse_group();

    /* Corpo: tutto il term che segue. */
    Node *node = node_new(N_BIGOP, lower, upper);
    node->c = parse_term();
    node->name = index;
    node->op = product;
    return node;
}

/* funzione ['^' group] argomento, con \log che ammette anche '_' group.
 *
 * Il token corrente è il comando della funzione fn. */
static Node *parse_function(Function fn) {
    next_token();

    /* Esponente e base, in qualsiasi ordine. */
    Node *exponent = NULL, *base = NULL;
    for (;;) {
        if (is_char('^') && !exponent) { next_token(); exponent = parse_group(); }
        else if (is_char('_') && fn == FN_LOG && !base) { next_token(); base = parse_group(); }
        else break;
    }

    /* Argomento: tra parentesi, oppure un solo fattore. */
    Node *argument;
    if (token.kind == T_OPEN) {
        next_token();
        argument = parse_expr();
        if (token.kind == T_CLOSE) next_token(); else syntax("manca la parentesi chiusa");
    } else {
        argument = parse_factor();
    }

    Node *call = node_call(fn, argument, base);
    return exponent ? node_new(N_POW, call, exponent) : call;
}

/* primary: vedi la grammatica in testa al capitolo. */
static Node *parse_primary(void) {
    Function fn;

    /* 9a. Numero. */
    if (token.kind == T_NUM) {
        Node *node = node_num(token.value);
        next_token();
        return node;
    }

    /* 9b. Lettera: un'applicazione dichiarata dall'enunciato, che vuole il suo
     * argomento tra parentesi, oppure una variabile, con eventuale pedice. */
    if (token.kind == T_LETTER) {
        int name = token.ch;
        next_token();
        Node *node;
        if (map_find(name)) {
            if (token.kind != T_OPEN) { syntax("un'applicazione va scritta con l'argomento tra parentesi, come f(x)"); return node_num(0); }
            next_token();
            node = node_new(N_APPLY, parse_expr(), NULL);
            if (token.kind == T_CLOSE) next_token(); else syntax("manca la parentesi chiusa");
        } else if (is_char('_')) {
            next_token();
            node = node_new(N_SEQ, parse_group(), NULL);
        } else {
            node = node_new(N_VAR, NULL, NULL);
        }
        node->name = name;
        return node;
    }

    /* 9c. Parentesi e valore assoluto. */
    if (token.kind == T_OPEN) {
        next_token();
        Node *inner = parse_expr();
        if (token.kind == T_CLOSE) next_token(); else syntax("manca la parentesi chiusa");
        return inner;
    }
    if (token.kind == T_ABS_OPEN) {
        next_token();
        Node *inner = parse_expr();
        if (token.kind == T_ABS_CLOSE) next_token(); else syntax("manca \\right|");
        return node_call(FN_ABS, inner, NULL);
    }

    /* 9d. Comandi con argomenti tra graffe. */
    if (is_command("frac") || is_command("dfrac") || is_command("tfrac")) {
        next_token();
        Node *above = parse_group();
        return node_new(N_DIV, above, parse_group());
    }
    if (is_command("binom")) {
        next_token();
        Node *upper = parse_group();
        return node_call(FN_BINOM, upper, parse_group());
    }
    if (is_command("sqrt")) {
        next_token();
        Node *index = NULL;
        if (is_char('[')) {
            next_token();
            index = parse_expr();
            expect_char(']', "manca ] nell'indice della radice");
        }
        Node *radicand = parse_group();
        return node_call(FN_ROOT, radicand, index ? index : node_num(2));
    }

    /* 9e. Somme, prodotti, funzioni, pi greco. */
    if (is_command("sum") || is_command("prod")) return parse_bigop();
    if (is_function(&fn)) return parse_function(fn);
    if (is_command("pi")) {
        next_token();
        Node *node = node_new(N_VAR, NULL, NULL);
        node->name = VAR_PI;
        return node;
    }

    /* 9f. Nient'altro può iniziare un'espressione. */
    syntax(token.kind == T_COMMAND ? "comando LaTeX non supportato" : "espressione incompleta o simbolo inatteso");
    return node_num(0);
}

/* factor := primary { '!' | '^' group } */
static Node *parse_factor(void) {
    Node *node = parse_primary();
    for (;;) {
        if (is_char('!')) {
            next_token();
            if (is_char('!')) syntax("il doppio fattoriale non è supportato");
            node = node_call(FN_FACT, node, NULL);
        } else if (is_char('^')) {
            next_token();
            node = node_new(N_POW, node, parse_group());
        } else {
            return node;
        }
    }
}

/* Ritorna 1 se il token corrente può iniziare un fattore: serve a riconoscere
 * la moltiplicazione implicita. */
static int starts_factor(void) {
    Function fn;
    return token.kind == T_NUM || token.kind == T_LETTER || token.kind == T_OPEN ||
           token.kind == T_ABS_OPEN || is_function(&fn) || is_command("frac") ||
           is_command("dfrac") || is_command("tfrac") || is_command("binom") ||
           is_command("sqrt") || is_command("sum") || is_command("prod") || is_command("pi");
}

/* term := factor { [\cdot | '/'] factor } */
static Node *parse_term(void) {
    Node *node = parse_factor();
    for (;;) {
        if (is_command("cdot") || is_command("times")) {
            next_token();
            node = node_new(N_MUL, node, parse_factor());
        } else if (is_char('/')) {
            next_token();
            node = node_new(N_DIV, node, parse_factor());
        } else if (starts_factor()) {
            node = node_new(N_MUL, node, parse_factor());
        } else {
            return node;
        }
    }
}

/* expr := ['-' | '+'] term { ('+' | '-') term } */
static Node *parse_expr(void) {
    Node *node;
    if (is_char('-')) { next_token(); node = node_new(N_NEG, parse_term(), NULL); }
    else { if (is_char('+')) next_token(); node = parse_term(); }
    for (;;) {
        if (is_char('+')) { next_token(); node = node_new(N_ADD, node, parse_term()); }
        else if (is_char('-')) { next_token(); node = node_new(N_SUB, node, parse_term()); }
        else return node;
    }
}

/* Analizza text come una sola espressione e ne ritorna l'albero. In caso di
 * errore syntax_error è diverso da NULL. */
static Node *parse_expression(const char *text) {
    syntax_error = NULL;
    cursor = text;
    next_token();
    Node *node = parse_expr();
    if (token.kind != T_END) syntax("testo inatteso dopo l'espressione");
    return node;
}

/* Analizza text come enunciato e lo mette in *statement. Ritorna 1 se riesce;
 * altrimenti ritorna 0 e syntax_error dice perché. */
static int parse_statement(const char *text, Statement *statement) {
    syntax_error = NULL;
    cursor = text;
    next_token();
    statement->lhs = parse_expr();

    /* La relazione tra i due membri. */
    if (is_char('=')) statement->relation = REL_EQ;
    else if (is_command("ge") || is_command("geq")) statement->relation = REL_GE;
    else if (is_command("le") || is_command("leq")) statement->relation = REL_LE;
    else if (is_char('>')) statement->relation = REL_GT;
    else if (is_char('<')) statement->relation = REL_LT;
    else syntax("manca la relazione tra i due membri");
    next_token();

    statement->rhs = parse_expr();
    if (token.kind != T_END) syntax("testo inatteso dopo l'enunciato");
    return syntax_error == NULL;
}

/* =============================================================================
 * 10. VOCABOLARIO: LE SUCCESSIONI NOTE, COME DATI
 * ============================================================================
 *
 * Il motore non sa cosa sia un fattoriale o un numero di Fibonacci: lo legge
 * da questa tabella. Una successione si può definire in due modi.
 *
 *   10a. Con una formula nell'indice m, scritta nello stesso LaTeX degli
 *        enunciati. Il fattoriale è un prodotto, il numero armonico una
 *        somma: il motore li tratta come ogni altra somma o prodotto.
 *
 *   10b. Con una ricorrenza lineare a coefficienti costanti di ordine d:
 *        X(m+d) = coef[0] X(m+d-1) + ... + coef[d-1] X(m), con i valori
 *        iniziali X(0), ..., X(d-1). Fibonacci è d = 2, coefficienti 1 e 1.
 *
 * Per aggiungere una successione basta una riga qui (per esempio i numeri di
 * Lucas: {'L', NULL, 2, {1, 1}, {2, 1}, NULL}).
 */

/* Una successione del vocabolario. */
typedef struct {
    int name;                   /* lettera con cui compare (pedice), o '!' */
    const char *definition;     /* 10a: formula in m; NULL nel caso 10b */
    int order;                  /* 10b: ordine d della ricorrenza */
    long long coef[MAX_ORDER];  /* 10b: coefficienti, dal termine più recente */
    long long init[MAX_ORDER];  /* 10b: X(0), ..., X(d-1) */
    Node *tree;                 /* 10a: la definizione già analizzata */
} Sequence;

static Sequence library[] = {
    {'!', "\\prod_{j=1}^{m} j", 0, {0}, {0}, NULL},
    {'H', "\\sum_{j=1}^{m} \\frac{1}{j}", 0, {0}, {0}, NULL},
    {'F', NULL, 2, {1, 1}, {0, 1}, NULL},
};
#define LIBRARY_COUNT ((int)(sizeof library / sizeof *library))

/* Quanto dell'arena è occupato dal vocabolario: da non svuotare mai. */
static size_t arena_keep;

/* Prepara il motore: analizza le definizioni del vocabolario. Va chiamata una
 * volta prima di ogni altra funzione dei capitoli successivi. */
static void engine_init(void) {
    arena_used = 0;
    for (int i = 0; i < LIBRARY_COUNT; i++)
        if (library[i].definition) library[i].tree = parse_expression(library[i].definition);
    arena_keep = arena_used;
}

/* =============================================================================
 * 11. VALUTAZIONE: DALL'ALBERO ALLA FRAZIONE
 * ============================================================================
 *
 * eval() calcola il valore di un nodo come frazione di polinomi negli atomi.
 * Le variabili possono essere "legate" a un valore: l'indice di una somma
 * durante il calcolo del corpo, e soprattutto n, che l'induzione lega di volta
 * in volta a un numero, a se stessa o a n + 1. Una variabile non legata è un
 * parametro dell'enunciato e diventa un atomo.
 *
 * Il punto delicato è riportare ogni risultato agli stessi atomi "in n",
 * qualunque sia il valore legato a n:
 *
 *   - somme e prodotti: eval_bigop();
 *   - successioni per ricorrenza: eval_recurrence();
 *   - potenze con esponente simbolico: power() e symbolic_power();
 *   - radici e logaritmi: fractional_power() e logarithm(), che si appoggiano
 *     alla scomposizione in fattori di factorize();
 *   - seni e coseni: exp_i().
 */

/* Una variabile legata e il suo valore. */
typedef struct {
    int name;
    Rat value;
} Binding;

/* Le variabili legate in questo momento: l'ultima aggiunta nasconde le
 * precedenti con lo stesso nome. */
static Binding env[MAX_ENV];
static int env_count;

/* Più di zero mentre si valuta una definizione del vocabolario. */
static int library_depth;

/* Il più piccolo n per cui tutte le somme e i prodotti scritti nell'enunciato
 * hanno almeno un termine. È il caso base quando l'enunciato non ne indica
 * uno; eval_bigop() lo alza. */
static long long default_base;

/* Un valore sotto il quale n non scende mai nell'enunciato in corso. Serve a
 * stabilire il segno di espressioni come n - 1 (vedi factorize()). */
static long long domain_floor;

/* I valori dati ai parametri dell'enunciato quando lo si calcola in un punto
 * preciso: il parametro point_name[i] vale point_value[i]. Durante la
 * dimostrazione l'elenco è vuoto e i parametri restano lettere; lo riempie
 * solo la verifica numerica (13e), che lega questi valori per primi: sono
 * quindi i primi point_count posti di `env` (vedi difference()). */
static int point_name[MAX_POINT];
static Frac point_value[MAX_POINT];
static int point_count;

/* Lega name a value. */
static void env_push(int name, Rat value) {
    if (env_count == MAX_ENV) { fail("troppe variabili legate annidate"); return; }
    env[env_count++] = (Binding){name, value};
}

/* Scioglie l'ultima variabile legata. */
static void env_pop(void) { if (env_count > 0) env_count--; }

/* Ritorna il valore legato a name, o NULL se name è un parametro libero. */
static const Rat *env_lookup(int name) {
    for (int i = env_count - 1; i >= 0; i--)
        if (env[i].name == name) return &env[i].value;
    return NULL;
}

/* Ritorna 1 se name è legata a un valore che cambia durante il conto: n, o
 * l'indice di una somma. I parametri a cui la verifica numerica ha dato un
 * valore sono legati anche loro, ma a una costante, e non contano. */
static int env_varies(int name) {
    for (int i = env_count - 1; i >= point_count; i--)
        if (env[i].name == name) return 1;
    return 0;
}

static Rat eval(const Node *node);

/* Ritorna 1 se nel sotto-albero node compare, libera, una variabile che in
 * questo momento è legata a un valore che cambia (vedi env_varies()). shadow elenca i `shadows` indici di somme interne
 * già incontrati: sono legati dalla loro somma, non dall'ambiente. */
static int uses_bound_variable(const Node *node, const int *shadow, int shadows) {
    if (!node) return 0;
    if (node->kind == N_VAR) {
        for (int i = 0; i < shadows; i++)
            if (shadow[i] == node->name) return 0;
        return env_varies(node->name);
    }
    if (node->kind == N_BIGOP) {
        if (uses_bound_variable(node->a, shadow, shadows) || uses_bound_variable(node->b, shadow, shadows))
            return 1;
        int inner[MAX_ENV + 1];
        if (shadows >= MAX_ENV) return 1;
        memcpy(inner, shadow, (size_t)shadows * sizeof(int));
        inner[shadows] = node->name;
        return uses_bound_variable(node->c, inner, shadows + 1);
    }
    return uses_bound_variable(node->a, shadow, shadows) || uses_bound_variable(node->b, shadow, shadows) ||
           uses_bound_variable(node->c, shadow, shadows);
}

/* Valuta il corpo della somma o prodotto node con l'indice legato a index. */
static Rat eval_body(const Node *node, Rat index) {
    env_push(node->name, index);
    Rat value = eval(node->c);
    env_pop();
    return value;
}

/* Ritorna 1 se l'atomo è non negativo per ogni n ammesso: n stesso, quando
 * parte da zero o più; la lettera ausiliaria; le potenze di un numero primo;
 * i prodotti di fattori positivi. */
static int atom_is_nonnegative(int atom) {
    const Atom *a = &atoms[atom];
    if (a->kind == ATOM_VAR) return a->id == VAR_SLACK || (a->id == induction_var && domain_floor >= 0);
    if (a->kind == ATOM_BIGOP) return a->positive;
    return a->kind == ATOM_POW && a->base == BASE_PRIME;
}

static int poly_is_real(Poly p);

/* Ritorna 1 se l'atomo ha certamente valore reale. Le variabili sono numeri
 * reali per convenzione; non lo sono gli atomi e^{i M}, e per prudenza nemmeno
 * somme e prodotti, il cui termine potrebbe contenerne. */
static int atom_is_real(int atom) {
    const Atom *a = &atoms[atom];
    if (a->kind == ATOM_BIGOP) return 0;
    if (a->kind != ATOM_POW) return 1;
    if (a->base == BASE_EI) return 0;
    if (a->base == BASE_ATOM) return atom_is_real((int)a->id);
    return a->base == BASE_PRIME || poly_is_real(base_polys[a->id]);
}

/* Ritorna 1 se p ha certamente valore reale: coefficienti reali e atomi
 * reali. */
static int poly_is_real(Poly p) {
    for (int i = 0; i < p.count; i++) {
        if (p.term[i].coef.im.num != 0) return 0;
        for (int f = 0; f < p.term[i].mono.count; f++)
            if (!atom_is_real(p.term[i].mono.factor[f].atom)) return 0;
    }
    return 1;
}

/* Ritorna 1 se p è evidentemente positivo: una somma di termini non negativi
 * con coefficiente positivo, uno dei quali è una costante. Un termine è non
 * negativo se ogni suo fattore è un atomo non negativo oppure una potenza pari
 * di un atomo reale, come x^2.
 *
 * È una condizione sufficiente, non necessaria: n^2 - n + 1 è positivo ma non
 * la soddisfa. */
static int poly_is_positive(Poly p) {
    int constant = 0;
    for (int i = 0; i < p.count; i++) {
        const Term *t = &p.term[i];
        if (t->coef.im.num != 0 || t->coef.re.num <= 0) return 0;
        for (int f = 0; f < t->mono.count; f++) {
            const Factor *factor = &t->mono.factor[f];
            int even = factor->exp.den == 1 && factor->exp.num % 2 == 0;
            if (!atom_is_nonnegative(factor->atom) && !(even && atom_is_real(factor->atom))) return 0;
        }
        if (t->mono.count == 0) constant = 1;
    }
    return constant;
}

/* Ritorna 1 se il corpo di node è positivo per ogni indice da lower in su.
 *
 * Il corpo viene valutato nell'indice lower + t, dove t è una lettera
 * ausiliaria che vale zero o più: se numeratore e denominatore del risultato
 * sono evidentemente positivi, il corpo lo è per ogni t. Per il fattoriale il
 * corpo è l'indice stesso, e in 1 + t dà 1 + t: positivo.
 *
 * Un errore durante questa prova non riguarda l'enunciato e viene scartato. */
static int body_is_positive(const Node *node, long long lower) {
    const char *saved = failure;
    int recording = conditions_on;

    /* È una prova fatta con una lettera ausiliaria, non un conto
     * dell'enunciato: le sue divisioni non pongono condizioni. */
    conditions_on = 0;
    Rat value = eval_body(node, rat_add(rat_int(lower), rat_atom(atom_var(VAR_SLACK)), 0));
    int positive = failure == saved && poly_is_positive(value.num) && poly_is_positive(value.den);
    failure = saved;
    conditions_on = recording;
    return positive;
}

/* Valuta una somma o un prodotto.
 *
 * Sia lower l'estremo inferiore (un intero) e a n + c il valore dell'estremo
 * superiore con le variabili legate attuali. Due casi.
 *
 * 11a. a = 0: gli estremi sono numeri. Si svolge: il corpo viene valutato per
 *      ogni indice da lower a c e i risultati sommati (o moltiplicati). Se
 *      c = lower - 1 non ci sono termini e il risultato è 0 (o 1).
 *
 * 11b. a > 0: l'estremo dipende da n. Il risultato è un atomo, che rappresenta
 *      la somma fino a un estremo di riferimento a n + r, corretto con i
 *      termini di differenza:
 *
 *          somma fino ad a n + c  =  atomo + corpo(a n + r + 1) + ... + corpo(a n + c)
 *
 *      se c > r, e con i termini da c + 1 a r sottratti se c < r. Per i
 *      prodotti si moltiplica e si divide. È il cuore dell'induzione: con n+1
 *      al posto di n, la somma fino a n diventa "atomo più ultimo termine".
 *
 *      Il riferimento r è l'estremo con cui l'atomo viene chiesto la prima
 *      volta, cioè quello scritto nell'enunciato (la prima valutazione è con n
 *      legato a se stesso). Scegliere invece sempre r = 0 sarebbe sbagliato:
 *      per una somma da 5 a n + 10 l'atomo "somma da 5 a n" non avrebbe
 *      termini per n piccolo.
 *
 *      Il caso richiede che il corpo dipenda solo dal proprio indice e da
 *      parametri: se dipendesse da n, passando a n+1 cambierebbero tutti i
 *      termini e non basterebbe aggiungere l'ultimo; se dipendesse dall'indice
 *      di una somma esterna, lo stesso atomo indicherebbe somme diverse. */
static Rat eval_bigop(const Node *node) {
    int product = node->op;
    long long lower, a, c;
    Rat upper = eval(node->b);
    if (!rat_integer(eval(node->a), &lower)) { fail("estremo inferiore di somma o prodotto non costante"); return rat_int(0); }
    if (!rat_affine(upper, &a, &c)) { fail("estremo superiore non della forma a n + c"); return rat_int(0); }

    /* 11a. Estremi numerici: si svolge. */
    if (a == 0) {
        if (c < lower - 1) { fail("somma o prodotto con estremo superiore troppo piccolo"); return rat_int(0); }
        if (c - lower >= MAX_UNROLL) { fail("somma o prodotto con troppi termini da svolgere"); return rat_int(0); }
        Rat total = rat_int(product ? 1 : 0);
        for (long long k = lower; k <= c && !failure; k++) {
            Rat value = eval_body(node, rat_int(k));
            total = product ? rat_mul(total, value) : rat_add(total, value, 0);
        }
        return total;
    }

    /* 11b. Estremo che dipende da n: atomo più termini di differenza. */
    if (a < 0) { fail("estremo superiore decrescente in n"); return rat_int(0); }
    if (uses_bound_variable(node->a, NULL, 0) || uses_bound_variable(node->c, &node->name, 1)) {
        fail("somma o prodotto il cui termine dipende da n: aggiungere l'ultimo termine non basta");
        return rat_int(0);
    }
    int known = atom_count;
    int atom = atom_bigop(node, a, c);
    long long reference = atoms[atom].offset;

    /* Alla nascita di un atomo prodotto si stabilisce se è positivo: lo usano
     * radici e logaritmi (vedi factorize()). */
    if (atom_count > known && product) atoms[atom].positive = body_is_positive(node, lower);
    if (c - reference > MAX_UNROLL || reference - c > MAX_UNROLL) { fail("troppi termini di differenza"); return rat_int(0); }

    /* Solo le somme scritte nell'enunciato decidono il caso base: devono avere
     * almeno un termine, cioè a n + c >= lower. */
    if (library_depth == 0) {
        long long need = lower - c;
        need = (need > 0) ? (need + a - 1) / a : -((-need) / a);
        if (need > default_base) default_base = need;
    }

    Rat total = rat_atom(atom);
    Rat an = rat_mul(rat_int(a), rat_atom(atom_var(induction_var)));
    for (long long j = reference + 1; j <= c && !failure; j++) {
        Rat value = eval_body(node, rat_add(an, rat_int(j), 0));
        total = product ? rat_mul(total, value) : rat_add(total, value, 0);
    }
    for (long long j = c + 1; j <= reference && !failure; j++) {
        Rat value = eval_body(node, rat_add(an, rat_int(j), 0));
        rewriting = product;
        total = product ? rat_div(total, value) : rat_add(total, value, 1);
        rewriting = 0;
    }
    return total;
}

/* Valuta X(index) per la successione s del vocabolario definita per ricorrenza
 * (caso 10b), con index = a n + c.
 *
 * Si tiene una finestra di d valori consecutivi X(t), ..., X(t+d-1) e la si fa
 * scorrere di un posto alla volta:
 *
 *   avanti:   X(t+d) = coef[0] X(t+d-1) + ... + coef[d-1] X(t)
 *   indietro: X(t-1) = (X(t+d-1) - coef[0] X(t+d-2) - ... - coef[d-2] X(t)) / coef[d-1]
 *
 * La seconda è la prima risolta rispetto al termine più vecchio.
 *
 * Se a = 0 la finestra parte dai valori iniziali X(0), ..., X(d-1), che sono
 * numeri, e dopo c passi contiene il numero X(c). Se a > 0 parte dagli atomi
 * X(a n), ..., X(a n + d - 1), e dopo c passi esprime X(a n + c) come loro
 * combinazione: per Fibonacci, F(n+2) = F(n) + F(n+1). */
static Rat eval_recurrence(int sequence, Rat index) {
    const Sequence *s = &library[sequence];
    int d = s->order;
    long long a, c;
    if (!rat_affine(index, &a, &c) || a < 0) { fail("indice di successione non della forma a n + c"); return rat_int(0); }
    if (c > MAX_UNROLL || c < -MAX_UNROLL) { fail("indice di successione troppo lontano"); return rat_int(0); }

    /* Finestra iniziale. */
    Rat window[MAX_ORDER];
    for (int j = 0; j < d; j++)
        window[j] = (a == 0) ? rat_int(s->init[j]) : rat_atom(atom_rec(sequence, a, j));

    /* Passi in avanti. */
    for (long long step = 0; step < c && !failure; step++) {
        Rat next = rat_int(0);
        for (int i = 0; i < d; i++) next = rat_add(next, rat_mul(rat_int(s->coef[i]), window[d - 1 - i]), 0);
        for (int j = 0; j + 1 < d; j++) window[j] = window[j + 1];
        window[d - 1] = next;
    }

    /* Passi all'indietro. */
    for (long long step = 0; step < -c && !failure; step++) {
        Rat previous = window[d - 1];
        for (int i = 0; i + 1 < d; i++)
            previous = rat_add(previous, rat_mul(rat_int(s->coef[i]), window[d - 2 - i]), 1);
        previous = rat_div(previous, rat_int(s->coef[d - 1]));
        for (int j = d - 1; j > 0; j--) window[j] = window[j - 1];
        window[0] = previous;
    }
    return window[0];
}

/* Valuta il termine di indice index della successione chiamata name. */
static Rat eval_sequence(int name, Rat index) {
    for (int i = 0; i < LIBRARY_COUNT; i++) {
        if (library[i].name != name) continue;
        if (!library[i].definition) return eval_recurrence(i, index);

        /* 10a: si valuta la formula con m legato all'indice. */
        env_push('m', index);
        library_depth++;
        Rat value = eval(library[i].tree);
        library_depth--;
        env_pop();
        return value;
    }
    fail("successione con pedice non presente nel vocabolario");
    return rat_int(0);
}

/* Moltiplica il monomio di out per atom^exp; con exp nullo non fa nulla. */
static void term_times(Term *out, int atom, Frac exp) {
    if (exp.num == 0) return;
    Mono factor = mono_atom(atom, exp);
    out->mono = mono_mul(&out->mono, &factor);
}

/* Toglie da *value il suo più piccolo fattore primo: mette il primo in *prime
 * e quante volte divide in *times. Ritorna 0, senza fare nulla, quando *value
 * è 1 o meno. Chiamata ripetutamente elenca la scomposizione in fattori primi.
 *
 * Si prova a dividere per 2, 3, 4, ...: il primo divisore trovato è per forza
 * primo, e se nessuno fino alla radice quadrata divide, il numero stesso è
 * primo. *value non deve superare 10^12, o il ciclo sarebbe troppo lungo. */
static int pop_prime(long long *value, long long *prime, long long *times) {
    if (*value <= 1) return 0;
    long long p = 2;
    while (p * p <= *value && *value % p != 0) p++;
    if (p * p > *value) p = *value;
    *prime = p;
    *times = 0;
    while (*value % p == 0) { *value /= p; (*times)++; }
    return 1;
}

/* Ritorna il monomio comune a tutti i termini di p, che non deve essere nullo:
 * per ogni atomo il minimo esponente tra i termini, contando zero dove manca.
 * Con esponenti negativi anche x^{-1} può essere comune:
 * x + 1/x = x^{-1} (x^2 + 1). */
static Mono poly_common(Poly p) {
    Mono common = p.term[0].mono;
    for (int i = 1; i < p.count; i++) {
        const Mono *other = &p.term[i].mono;
        Mono low;
        low.count = 0;
        int x = 0, y = 0;
        while (x < common.count || y < other->count) {
            Factor f;
            if (y == other->count || (x < common.count && common.factor[x].atom < other->factor[y].atom)) {
                /* Atomo assente nell'altro termine: conta zero, quindi resta
                 * comune solo un esponente negativo. */
                f = common.factor[x++];
                if (f.exp.num > 0) continue;
            } else if (x == common.count || other->factor[y].atom < common.factor[x].atom) {
                f = other->factor[y++];
                if (f.exp.num > 0) continue;
            } else {
                f = common.factor[x++];
                if (frac_cmp(other->factor[y].exp, f.exp) < 0) f.exp = other->factor[y].exp;
                y++;
            }
            low.factor[low.count++] = f;
        }
        common = low;
    }
    return common;
}

/* Ritorna l'atomo che rappresenta (atom)^m, con m un monomio. Se atom è già
 * una potenza b^{m'} il risultato è b^{m' m}, per la regola (b^p)^q = b^{pq}:
 * così (e^{i x})^n ed e^{i n x} sono lo stesso atomo. */
static int atom_raised(int atom, const Mono *m) {
    if (atoms[atom].kind != ATOM_POW) return atom_pow(BASE_ATOM, atom, m);
    Mono merged = mono_mul(&atoms[atom].exponent, m);
    return atom_pow(atoms[atom].base, atoms[atom].id, &merged);
}

/* Ritorna 1 se il monomio vale un numero intero per ogni n ammesso: è un
 * prodotto di potenze, a esponente intero positivo, di n e di atomi come 2^n. */
static int mono_is_integer(const Mono *m) {
    for (int i = 0; i < m->count; i++) {
        const Atom *a = &atoms[m->factor[i].atom];
        Frac e = m->factor[i].exp;
        if (e.den != 1 || e.num < 0) return 0;
        if (a->kind == ATOM_VAR && a->id == induction_var) continue;
        if (a->kind == ATOM_POW && a->base == BASE_PRIME && domain_floor >= 0 && mono_is_integer(&a->exponent)) continue;
        return 0;
    }
    return 1;
}

/* Ritorna p^{q m}, dove p è un polinomio non nullo, m un monomio non banale
 * (l'esponente simbolico, per esempio n) e q un razionale.
 *
 * Per riconoscere la stessa potenza scritta in modi diversi, p viene scomposto
 * in fattori di cui si eleva ciascuno separatamente, usando (u v)^e = u^e v^e:
 *
 *     p = (segno o unità immaginaria) * (numero razionale positivo)
 *         * (monomio comune a tutti i termini) * (polinomio primitivo)
 *
 *   11c. Il segno: (-1)^{q m} = e^{i pi q m}. L'unità immaginaria i, che
 *        compare con i seni, è e^{i pi/2}, quindi dà e^{i pi q m / 2}.
 *   11d. Il numero: si scompone in fattori primi, ognuno con il suo atomo.
 *        Così 4^n = (2^n)^2 e 6^n = 2^n 3^n.
 *   11e. Il monomio comune: ogni suo atomo viene elevato con atom_raised().
 *   11f. Il polinomio primitivo, cioè ciò che resta: ha coefficienti interi
 *        senza divisori comuni e primo coefficiente positivo, quindi è lo
 *        stesso per tutti i multipli di p. Se non è 1 diventa la base di un
 *        atomo.
 *
 * La regola (u v)^e = u^e v^e vale per e intero ma non in generale: con basi
 * negative ed esponente non intero può fallire. Per questo la scomposizione si
 * fa solo quando l'esponente q m è un intero per ogni n (11b'); altrimenti la
 * base resta un blocco unico e diventa un atomo così com'è. */
static Rat symbolic_power(Poly p, const Mono *m, Frac q) {
    Term out;
    out.coef = num_int(1);
    out.mono.count = 0;
    if (p.count == 0) { fail("zero elevato a un esponente simbolico"); return rat_int(0); }

    /* 11b'. Esponente non intero: nessuna scomposizione. Se la base è un solo
     * atomo lo si eleva direttamente, e una potenza di un primo, che è
     * positiva, può ancora fondere gli esponenti: (2^n)^x = 2^{n x}. */
    if (q.den != 1 || !mono_is_integer(m)) {
        const Term *only = &p.term[0];
        int single = p.count == 1 && only->mono.count == 1 && num_equal(only->coef, num_int(1)) &&
                     only->mono.factor[0].exp.num == 1 && only->mono.factor[0].exp.den == 1;
        int atom = single ? only->mono.factor[0].atom : 0;
        if (single && atoms[atom].kind == ATOM_POW && atoms[atom].base == BASE_PRIME) term_times(&out, atom_raised(atom, m), q);
        else if (single) term_times(&out, atom_pow(BASE_ATOM, atom, m), q);
        else term_times(&out, atom_pow(BASE_POLY, base_poly_intern(p), m), q);
        return (Rat){poly_term(out), poly_const(num_int(1))};
    }

    /* Monomio comune, e ciò che resta dopo averlo raccolto. */
    Mono common = poly_common(p);
    Term divisor;
    divisor.coef = num_int(1);
    divisor.mono = mono_pow(&common, frac_int(-1));
    Poly rest = poly_mul(p, poly_term(divisor));
    if (failure || rest.count == 0) return rat_int(0);

    /* Contenuto numerico. Se tutti i coefficienti sono immaginari puri si
     * raccoglie i; se sono misti non si raccoglie nulla. */
    int imaginary = 1, real = 1;
    for (int i = 0; i < rest.count; i++) {
        if (rest.term[i].coef.re.num != 0) imaginary = 0;
        if (rest.term[i].coef.im.num != 0) real = 0;
    }
    Frac content = frac_int(1);
    if (real || imaginary) {
        /* Massimo comun divisore dei numeratori diviso minimo comune multiplo
         * dei denominatori, con il segno del primo coefficiente. */
        __int128 g = 0, l = 1;
        for (int i = 0; i < rest.count; i++) {
            Frac v = imaginary ? rest.term[i].coef.im : rest.term[i].coef.re;
            g = gcd128(g, v.num);
            l = l / gcd128(l, v.den) * v.den;
            if (l >= COEF_LIMIT) { fail("numeri troppo grandi per il calcolo esatto"); return rat_int(0); }
        }
        Frac first = imaginary ? rest.term[0].coef.im : rest.term[0].coef.re;
        content = frac_make(first.num < 0 ? -g : g, l);
        divisor.coef = num_inv(imaginary ? (Num){frac_int(0), content} : num_frac(content));
        divisor.mono.count = 0;
        rest = poly_mul(rest, poly_term(divisor));
    }

    /* 11c. Segno e unità immaginaria. */
    if (content.num < 0 || imaginary) {
        Mono pi = mono_atom(atom_var(VAR_PI), frac_int(1));
        Mono turn = mono_mul(m, &pi);
        int e = atom_pow(BASE_EI, 0, &turn);
        if (content.num < 0) term_times(&out, e, q);
        if (imaginary) term_times(&out, e, frac_div(q, frac_int(2)));
    }

    /* 11d. Fattori primi di numeratore e denominatore del contenuto. */
    for (int side = 0; side < 2; side++) {
        long long value = side ? content.den : (content.num < 0 ? -content.num : content.num);
        long long prime, times;
        if (value > 1000000000000LL) { fail("numero troppo grande da scomporre in fattori primi"); return rat_int(0); }
        while (pop_prime(&value, &prime, &times))
            term_times(&out, atom_pow(BASE_PRIME, prime, m), frac_mul(frac_int(side ? -times : times), q));
    }

    /* 11e. Monomio comune. */
    for (int i = 0; i < common.count; i++)
        term_times(&out, atom_raised(common.factor[i].atom, m), frac_mul(common.factor[i].exp, q));

    /* 11f. Polinomio primitivo. */
    if (!poly_is_one(rest)) term_times(&out, atom_pow(BASE_POLY, base_poly_intern(rest), m), q);

    return (Rat){poly_term(out), poly_const(num_int(1))};
}

/* Un fattore di una scomposizione: base elevata a power. La base è descritta
 * come negli atomi POW: un primo, un atomo, o un polinomio di `base_polys`. */
typedef struct {
    BaseKind kind;
    long long id;
    Frac power;
} Part;

/* La scomposizione di un polinomio p in fattori di cui è noto il segno:
 * p = (prodotto delle part) * rest. Ogni part ha base non negativa per tutti
 * gli n ammessi; rest raccoglie ciò di cui il segno non è noto, compreso il
 * segno stesso di p, e vale 1 quando non resta nulla. */
typedef struct {
    int count;             /* part valide */
    Part part[MAX_PARTS];
    Poly rest;
} Factorization;

/* Aggiunge alla scomposizione il fattore base^power, sommando l'esponente se
 * la base è già presente. */
static void part_add(Factorization *f, BaseKind kind, long long id, Frac power) {
    for (int i = 0; i < f->count; i++)
        if (f->part[i].kind == kind && f->part[i].id == id) {
            f->part[i].power = frac_add(f->part[i].power, power);
            return;
        }
    if (f->count == MAX_PARTS) { fail("troppi fattori in un radicando o nell'argomento di un logaritmo"); return; }
    f->part[f->count++] = (Part){kind, id, power};
}

/* Ritorna il polinomio c[0] + c[1] n + ... + c[degree] n^degree. */
static Poly poly_univariate(const long long *c, int degree) {
    int n = atom_var(induction_var);
    Term *buffer = terms_alloc(degree + 1);
    if (!buffer) return (Poly){0, NULL};
    int used = 0;
    for (int i = 0; i <= degree; i++) {
        if (c[i] == 0) continue;
        Term t;
        t.coef = num_int(c[i]);
        t.mono.count = 0;
        if (i > 0) t.mono = mono_atom(n, frac_int(i));
        buffer[used++] = t;
    }
    return poly_finish(buffer, degree + 1, used);
}

/* Mette in out i divisori positivi di value (positivo) e ne ritorna il numero,
 * oppure 0 se sono più di MAX_DIVISORS. I divisori vanno a coppie d e
 * value/d, quindi basta cercare d fino alla radice quadrata. */
static int divisors(long long value, long long *out) {
    int count = 0;
    for (long long d = 1; d * d <= value; d++) {
        if (value % d != 0) continue;
        if (count + 2 > MAX_DIVISORS) return 0;
        out[count++] = d;
        if (d != value / d) out[count++] = value / d;
    }
    return count;
}

/* Dice se a n + b divide il polinomio di coefficienti c[0..degree], cioè se
 * -b/a ne è una radice: ritorna 1 se lo è, 0 se non lo è, -1 se i numeri
 * escono dall'aritmetica esatta e non si può dire. Si calcola
 * a^degree * p(-b/a), che è l'intero somma dei c[i] (-b)^i a^{degree-i}, con
 * lo schema di Horner partendo dal coefficiente di grado più alto.
 *
 * Il terzo esito va tenuto distinto dal "no": chi conclude che un polinomio
 * non ha radici (capitolo 18) non può contare su una risposta data per
 * mancanza di cifre. */
static int is_root(const long long *c, int degree, long long a, long long b) {
    __int128 value = 0, a_power = 1;
    for (int i = degree; i >= 0; i--) {
        __int128 scaled, term;
        if (__builtin_mul_overflow(value, (__int128)-b, &scaled) ||
            __builtin_mul_overflow((__int128)c[i], a_power, &term) ||
            __builtin_add_overflow(scaled, term, &value))
            return -1;
        if (i > 0 && __builtin_mul_overflow(a_power, (__int128)a, &a_power)) return -1;
    }
    return value == 0;
}

/* Divide sul posto il polinomio c[0..degree] per a n + b. Ritorna 1 se la
 * divisione è esatta a coefficienti interi, lasciando in c[0..degree-1] il
 * quoziente; altrimenti ritorna 0 e c resta com'era.
 *
 * Se c = (a n + b) q, confrontando i coefficienti: c[degree] = a q[degree-1],
 * c[i] = a q[i-1] + b q[i], c[0] = b q[0]. Si ricavano i q dall'alto. */
static int deflate(long long *c, int degree, long long a, long long b) {
    long long q[MAX_UNIVARIATE + 1];
    if (c[degree] % a != 0) return 0;
    q[degree - 1] = c[degree] / a;
    for (int i = degree - 1; i >= 1; i--) {
        long long rest = c[i] - b * q[i];
        if (rest % a != 0) return 0;
        q[i - 1] = rest / a;
    }
    if (c[0] != b * q[0]) return 0;
    for (int i = 0; i < degree; i++) c[i] = q[i];
    c[degree] = 0;
    return 1;
}

/* Trova le radici razionali del polinomio c[0] + c[1] x + ... + c[degree]
 * x^degree, a coefficienti interi con c[degree] diverso da zero. Le mette,
 * distinte, in root[] (che deve avere degree + 1 posti) e ne ritorna il
 * numero; ritorna -1 se non riesce a stabilirle con certezza.
 *
 * Teorema delle radici razionali. Sia u/v una radice, con u e v interi senza
 * divisori comuni e v > 0. Sostituendo e moltiplicando per v^degree:
 *
 *     c[0] v^d + c[1] u v^{d-1} + ... + c[d] u^d = 0.
 *
 * Tutti i termini tranne il primo contengono u, quindi u divide c[0] v^d; non
 * avendo fattori in comune con v, divide c[0]. Allo stesso modo v divide
 * c[d]. Se c[0] non è zero i candidati sono dunque in numero finito, e
 * provandoli tutti si trovano tutte le radici razionali. Se c[0] è zero, lo
 * zero è una radice: si divide per x e si ricomincia. */
static int rational_roots(const long long *c, int degree, Frac *root) {
    int count = 0;

    /* Le potenze di x che dividono il polinomio danno la radice zero. */
    while (degree >= 1 && c[0] == 0) {
        if (!count) root[count++] = frac_int(0);
        c++;
        degree--;
    }
    if (degree < 1) return count;

    /* I candidati: -b/a con a divisore di c[degree] e b di c[0], primi tra
     * loro, con i due segni. is_root(c, degree, a, b) prova la radice -b/a. */
    long long lead[MAX_DIVISORS], tail[MAX_DIVISORS];
    int leads = divisors(llabs(c[degree]), lead), tails = divisors(llabs(c[0]), tail);
    if (!leads || !tails) return -1;
    for (int i = 0; i < leads; i++)
        for (int j = 0; j < tails; j++) {
            if (gcd128(lead[i], tail[j]) != 1) continue;
            for (int sign = -1; sign <= 1; sign += 2) {
                int found = is_root(c, degree, lead[i], sign * tail[j]);
                if (found < 0) return -1;
                if (found) root[count++] = frac_make(-sign * tail[j], lead[i]);
            }
        }
    return count;
}

/* Stacca da p i fattori di primo grado a n + b che sono non negativi per ogni
 * n ammesso: li aggiunge a f e ritorna il quoziente. Se p non è un polinomio
 * nella sola n a coefficienti interi lo ritorna com'è.
 *
 * Teorema delle radici razionali: se una frazione ridotta -b/a è radice di un
 * polinomio a coefficienti interi, allora a divide il coefficiente di grado
 * massimo e b divide il termine noto. I candidati sono quindi in numero
 * finito e si provano tutti. Ciò che resta dopo aver tolto i fattori di primo
 * grado (per esempio n^2 + n + 1) non viene scomposto oltre.
 *
 * a n + b è non negativo per n >= domain_floor quando a > 0 e lo è nel punto
 * più basso, n = domain_floor. */
static Poly split_linear_factors(Poly p, Factorization *f) {
    long long c[MAX_UNIVARIATE + 1], scale;

    /* Coefficienti di p, se è della forma richiesta: devono essere già interi,
     * perché il quoziente va restituito senza cambiarlo di un fattore. */
    int degree = poly_coefficients(p, atom_var(induction_var), c, &scale);
    if (degree < 0 || scale != 1) return p;

    /* Finché si trova una radice si stacca il fattore e si ricomincia: così
     * una radice multipla viene tolta più volte. */
    int found = 1, changed = 0;
    while (found && degree >= 1 && c[0] != 0) {
        long long lead[MAX_DIVISORS], tail[MAX_DIVISORS];
        int leads = divisors(llabs(c[degree]), lead), tails = divisors(llabs(c[0]), tail);
        found = 0;
        for (int x = 0; x < leads && !found; x++)
            for (int y = 0; y < tails && !found; y++)
                for (int sign = -1; sign <= 1 && !found; sign += 2) {
                    long long a = lead[x], b = sign * tail[y];
                    if (a * domain_floor + b < 0) continue;
                    if (is_root(c, degree, a, b) != 1 || !deflate(c, degree, a, b)) continue;
                    long long linear[2] = {b, a};
                    part_add(f, BASE_POLY, base_poly_intern(poly_univariate(linear, 1)), frac_int(1));
                    degree--;
                    found = changed = 1;
                }
    }
    return changed ? poly_univariate(c, degree) : p;
}

/* Scompone il polinomio non nullo p in fattori di segno noto (vedi
 * Factorization). Serve a radici e logaritmi, che si distribuiscono sui
 * fattori solo quando questi sono non negativi:
 *
 *     radice(u v) = radice(u) radice(v),    ln(u v) = ln(u) + ln(v).
 *
 * Tre passi, ognuno dei quali stacca fattori dal polinomio:
 *
 *   11k. Il monomio comune ai termini. I suoi atomi non negativi diventano
 *        fattori; gli altri finiscono in rest.
 *   11l. Il contenuto numerico: il massimo comun divisore dei coefficienti,
 *        scomposto in fattori primi. Il segno resta in rest.
 *   11m. I fattori di primo grado in n, con split_linear_factors().
 *
 * Se i coefficienti non sono tutti reali (succede con seni e coseni) si fa
 * solo il passo 11k. */
static Factorization factorize(Poly p) {
    Factorization f;
    f.count = 0;

    /* 11k. Monomio comune. */
    Mono common = poly_common(p);
    Term unknown;
    unknown.coef = num_int(1);
    unknown.mono.count = 0;
    for (int i = 0; i < common.count; i++) {
        if (atom_is_nonnegative(common.factor[i].atom)) part_add(&f, BASE_ATOM, common.factor[i].atom, common.factor[i].exp);
        else term_times(&unknown, common.factor[i].atom, common.factor[i].exp);
    }
    Term divisor;
    divisor.coef = num_int(1);
    divisor.mono = mono_pow(&common, frac_int(-1));
    Poly rest = poly_mul(p, poly_term(divisor));

    int real = 1;
    for (int i = 0; i < rest.count; i++)
        if (rest.term[i].coef.im.num != 0) real = 0;
    if (real && !failure) {
        /* 11l. Contenuto: massimo comun divisore dei numeratori diviso minimo
         * comune multiplo dei denominatori. */
        __int128 g = 0, l = 1;
        for (int i = 0; i < rest.count; i++) {
            g = gcd128(g, rest.term[i].coef.re.num);
            l = l / gcd128(l, rest.term[i].coef.re.den) * rest.term[i].coef.re.den;
            if (l >= COEF_LIMIT) { fail("numeri troppo grandi per il calcolo esatto"); break; }
        }
        Frac content = failure ? frac_int(1) : frac_make(g, l);
        if (content.num > 1000000000000LL || content.den > 1000000000000LL) fail("numero troppo grande da scomporre in fattori primi");
        for (int side = 0; side < 2 && !failure; side++) {
            long long value = side ? content.den : content.num, prime, times;
            while (pop_prime(&value, &prime, &times)) part_add(&f, BASE_PRIME, prime, frac_int(side ? -times : times));
        }
        divisor.coef = num_inv(num_frac(content));
        divisor.mono.count = 0;
        rest = poly_mul(rest, poly_term(divisor));

        /* 11m. Fattori di primo grado. */
        if (!failure) rest = split_linear_factors(rest, &f);
    }
    f.rest = poly_mul(rest, poly_term(unknown));
    return f;
}

/* Ritorna base^q con q razionale non intero: una radice, o una sua potenza.
 *
 * Numeratore e denominatore della base vengono scomposti con factorize() e
 * ogni fattore u diventa l'atomo "u preso come lettera" elevato a q volte il
 * suo esponente: radice(12) = 2 radice(3) nasce così, da 12 = 2^2 * 3.
 *
 * Può restare una parte di segno non noto, per esempio n + a con a parametro.
 * Se resta in uno solo tra numeratore e denominatore la si tratta come gli
 * altri fattori: la base è non negativa per ipotesi (altrimenti la radice non
 * sarebbe definita) e tutti gli altri fattori lo sono, quindi lo è anche lei.
 * Se resta in entrambi non si può dire nulla dei due segni presi da soli, e si
 * rinuncia. */
static Rat fractional_power(Rat base, Frac q) {
    Term out;
    Mono none;
    out.coef = num_int(1);
    out.mono.count = none.count = 0;

    /* Zero: 0^q = 0 per q positivo. */
    if (base.num.count == 0) {
        if (q.num < 0) fail_undefined("zero elevato a un esponente negativo");
        return rat_int(0);
    }

    /* Con esponente negativo la base sta a denominatore. */
    if (q.num < 0) require(NEVER_ZERO, base.num, "zero elevato a un esponente negativo");

    Factorization side[2] = {factorize(base.num), factorize(base.den)};
    if (failure) return rat_int(0);
    int loose[2] = {!poly_is_one(side[0].rest), !poly_is_one(side[1].rest)};
    if (loose[0] && loose[1]) { fail("radice di una frazione in cui non si conosce il segno dei due termini"); return rat_int(0); }
    for (int s = 0; s < 2; s++) {
        Frac direction = frac_int(s ? -1 : 1);

        /* Fattori di segno noto. */
        for (int i = 0; i < side[s].count; i++) {
            const Part *part = &side[s].part[i];
            Frac e = frac_mul(frac_mul(part->power, q), direction);
            term_times(&out, part->kind == BASE_ATOM ? (int)part->id : atom_pow(part->kind, part->id, &none), e);
        }

        /* Parte di segno non noto. Se è una costante è -1 (o un numero
         * complesso): la base è negativa e la radice non è definita. */
        if (!loose[s]) continue;
        if (side[s].rest.count == 1 && side[s].rest.term[0].mono.count == 0) { fail_undefined("radice di un numero negativo"); return rat_int(0); }

        /* Gli altri fattori sono non negativi: la base lo è se lo è questo. */
        require(NEVER_NEGATIVE, side[s].rest, "radice di un numero negativo");
        term_times(&out, atom_pow(BASE_POLY, base_poly_intern(side[s].rest), &none), frac_mul(q, direction));
    }
    return (Rat){poly_term(out), poly_const(num_int(1))};
}

/* Ritorna ln|x| come combinazione di atomi LN.
 *
 * Il logaritmo del valore assoluto trasforma prodotti in somme senza
 * condizioni sui segni: ln|u v| = ln|u| + ln|v| e ln|u^e| = e ln|u| per ogni
 * u e v non nulli. Si scompongono quindi numeratore e denominatore con
 * factorize() e si sommano i logaritmi dei fattori, ognuno con il suo
 * esponente. Dove l'enunciato scrive ln(x) con x positivo, ln|x| è lo stesso
 * numero.
 *
 * Una potenza p^M di un primo dà M ln(p), così ln(2^n) = n ln 2. */
static Rat logarithm(Rat x) {
    if (x.num.count == 0) { fail_undefined("logaritmo di zero"); return rat_int(0); }
    require(NEVER_ZERO, x.num, "logaritmo di zero");
    Rat total = rat_int(0);
    for (int s = 0; s < 2 && !failure; s++) {
        Factorization f = factorize(s ? x.den : x.num);

        /* Fattori di segno noto. */
        for (int i = 0; i < f.count && !failure; i++) {
            const Part *part = &f.part[i];
            Term t;
            t.coef = num_frac(part->power);
            t.mono.count = 0;
            if (part->kind == BASE_ATOM && atoms[part->id].kind == ATOM_POW && atoms[part->id].base == BASE_PRIME) {
                t.mono = atoms[part->id].exponent;
                term_times(&t, atom_ln(BASE_PRIME, atoms[part->id].id), frac_int(1));
            } else {
                term_times(&t, atom_ln(part->kind, part->id), frac_int(1));
            }
            total = rat_add(total, (Rat){poly_term(t), poly_const(num_int(1))}, s);
        }

        /* Parte di segno non noto. Una costante qui è -1, e ln|-1| = 0; un
         * polinomio dà un atomo, con il segno scelto in modo che p e -p, che
         * hanno lo stesso valore assoluto, diano lo stesso atomo. */
        Poly rest = f.rest;
        if (rest.count == 1 && rest.term[0].mono.count == 0) {
            if (rest.term[0].coef.im.num != 0) fail("logaritmo di un numero complesso");
            continue;
        }
        if (rest.count > 0 && rest.term[0].coef.im.num == 0 && rest.term[0].coef.re.num < 0)
            rest = poly_add(poly_const(num_int(0)), rest, 1);
        total = rat_add(total, rat_atom(atom_ln(BASE_POLY, base_poly_intern(rest))), s);
    }
    return total;
}

/* Ritorna ln(x) per un argomento scritto senza valore assoluto, che quindi
 * deve essere positivo. In generale è un'ipotesi dell'enunciato; quando x è un
 * numero la si controlla, e un argomento non positivo è un errore. Quando non
 * lo è, la condizione la annota chi chiama, che conosce l'argomento com'è
 * scritto (vedi require_positive()). */
static Rat logarithm_of_positive(Rat x) {
    Num value;
    if (rat_number(x, &value) && (value.im.num != 0 || value.re.num <= 0)) {
        fail_undefined("logaritmo di un numero non positivo");
        return rat_int(0);
    }

    return logarithm(x);
}

/* Argomenti degli atomi ATAN dell'enunciato in corso, ciascuno una volta:
 * l'atomo di id k vale l'arcotangente di atan_args[k]. */
static Rat atan_args[MAX_ATOMS];
static int atan_count;

/* Ritorna arctan(x) come combinazione di atomi.
 *
 * L'argomento deve essere reale. Tre regole portano arcotangenti uguali allo
 * stesso atomo:
 *
 *   - arctan(0) = 0;
 *   - l'arcotangente è dispari, arctan(-u) = -arctan(u): si tiene l'argomento
 *     con numeratore e denominatore di primo coefficiente positivo, e il segno
 *     esce fuori;
 *   - arctan(1) = pi/4, perché tan(pi/4) = 1. Tra gli argomenti razionali è
 *     l'unico valore, con 0, che sia un multiplo razionale di pi greco. */
static Rat arctangent(Rat x) {
    if (!poly_is_real(x.num) || !poly_is_real(x.den)) { fail("arcotangente di un'espressione che non è certamente reale"); return rat_int(0); }
    if (x.num.count == 0) return rat_int(0);

    /* Segno: lo si porta fuori. */
    int negative = 0;
    if (x.num.term[0].coef.re.num < 0) { x.num = poly_add(poly_const(num_int(0)), x.num, 1); negative = !negative; }
    if (x.den.term[0].coef.re.num < 0) { x.den = poly_add(poly_const(num_int(0)), x.den, 1); negative = !negative; }

    Rat value;
    if (poly_equal(x.num, x.den)) {
        value = rat_div(rat_atom(atom_var(VAR_PI)), rat_int(4));
    } else {
        /* Cerca un atomo con lo stesso argomento: p1/q1 = p2/q2 quando
         * p1 q2 = p2 q1. */
        int index = 0;
        while (index < atan_count &&
               !poly_equal(poly_mul(x.num, atan_args[index].den), poly_mul(atan_args[index].num, x.den)))
            index++;
        if (index == MAX_ATOMS) { fail("troppe arcotangenti distinte nell'enunciato"); return rat_int(0); }
        if (index == atan_count) atan_args[atan_count++] = x;

        Atom atom = {0};
        atom.kind = ATOM_ATAN;
        atom.id = index;
        atom.depends_on_n = poly_depends_on_n(x.num) || poly_depends_on_n(x.den);
        value = rat_atom(atom_intern(&atom));
    }
    return negative ? rat_add(rat_int(0), value, 1) : value;
}

/* Ritorna base^exponent.
 *
 * 11g. Esponente intero costante: moltiplicazioni ripetute.
 *
 * 11h. Altrimenti l'esponente deve essere un polinomio, cioè una somma di
 *      termini q M con q razionale e M monomio; poiché b^{u+v} = b^u b^v, si
 *      eleva un termine alla volta. Il termine costante (M = 1) è una potenza
 *      intera oppure, se q non è intero, una radice: fractional_power(). Gli
 *      altri passano da symbolic_power(), separatamente per numeratore e
 *      denominatore della base. Così 2^{n+1} diventa 2 * 2^n. */
static Rat power(Rat base, Rat exponent) {
    /* 11g. */
    long long e;
    if (rat_integer(exponent, &e)) return rat_pow_int(base, e);

    /* 11h. */
    if (!poly_is_one(exponent.den)) { fail("esponente con un denominatore che non è un monomio"); return rat_int(0); }
    Rat result = rat_int(1);
    for (int i = 0; i < exponent.num.count && !failure; i++) {
        const Term *t = &exponent.num.term[i];
        Frac q = t->coef.re;
        if (t->coef.im.num != 0) { fail("esponente complesso"); break; }
        if (t->mono.count == 0) {
            result = rat_mul(result, q.den == 1 ? rat_pow_int(base, q.num) : fractional_power(base, q));
        } else {
            /* Un esponente simbolico negativo porta la base a denominatore;
             * uno che non è sempre intero vuole una base non negativa. */
            Mono m = t->mono;
            if (q.num < 0) require(NEVER_ZERO, base.num, "zero elevato a un esponente negativo");
            if (conditions_on && (q.den != 1 || !mono_is_integer(&m)))
                require(NEVER_NEGATIVE, poly_mul(base.num, base.den), "potenza a esponente non intero di un numero negativo");
            result = rat_mul(result, rat_div(symbolic_power(base.num, &m, q), symbolic_power(base.den, &m, q)));
        }
    }
    return result;
}

/* Ritorna e^{i sign theta}, con sign uguale a +1 o -1.
 *
 * L'angolo theta deve essere un polinomio: una somma di termini q M con q
 * razionale. Poiché e^{i (u + v)} = e^{i u} e^{i v}, il risultato è il
 * prodotto degli atomi e^{i M} elevati a q. Per esempio, con theta = (n+1) x/2
 * = n x/2 + x/2 si ottiene (e^{i n x})^{1/2} (e^{i x})^{1/2}: le formule di
 * addizione di seno e coseno sono già qui dentro. */
static Rat exp_i(Rat theta, int sign) {
    Term out;
    out.coef = num_int(1);
    out.mono.count = 0;
    if (!poly_is_one(theta.den)) { fail("angolo con un denominatore che non è un monomio"); return rat_int(0); }
    for (int i = 0; i < theta.num.count; i++) {
        const Term *t = &theta.num.term[i];
        if (t->coef.im.num != 0) { fail("angolo complesso"); return rat_int(0); }
        term_times(&out, atom_pow(BASE_EI, 0, &t->mono), frac_mul(t->coef.re, frac_int(sign)));
    }
    return (Rat){poly_term(out), poly_const(num_int(1))};
}

/* Raccoglie in factor[] i fattori in cui è scritta l'espressione del nodo, e
 * ne aggiorna il numero in *count. Un prodotto o un quoziente danno i fattori
 * dei due operandi; un segno meno dà il fattore -1; una potenza a esponente
 * dispari dà quelli della base. Tutto il resto è un fattore solo, il suo
 * valore: numeratore e denominatore.
 *
 * Una potenza a esponente pari non cambia il segno del prodotto, ma lo
 * annulla dove si annulla la base: se `positive` è diverso da zero si annota
 * a parte che la base non deve essere zero. Con troppi fattori ritorna 0. */
static int sign_factors(const Node *node, Poly *factor, int *count, int positive) {
    if (failure) return 0;
    if (node->kind == N_MUL || node->kind == N_DIV)
        return sign_factors(node->a, factor, count, positive) && sign_factors(node->b, factor, count, positive);
    if (node->kind == N_POW && node->b->kind == N_NUM && node->b->value % 2 == 0) {
        if (positive) require(NEVER_ZERO, eval(node->a).num, "logaritmo di zero");
        return 1;
    }
    if (node->kind == N_POW && node->b->kind == N_NUM) return sign_factors(node->a, factor, count, positive);

    /* Un segno meno, e poi ciò che nega; oppure un fattore così com'è. */
    Rat value = node->kind == N_NEG ? rat_int(-1) : eval(node);
    Poly piece[2] = {value.num, value.den};
    for (int i = 0; i < 2; i++) {
        if (poly_is_one(piece[i])) continue;
        if (*count == MAX_SIGN_FACTORS) return 0;
        factor[(*count)++] = piece[i];
    }
    return node->kind == N_NEG ? sign_factors(node->a, factor, count, positive) : 1;
}

/* Annota che l'argomento di un logaritmo, il nodo `argument` di valore x,
 * deve essere positivo. Il segno di un prodotto si decide meglio fattore per
 * fattore che sul prodotto svolto: di sin(a) cos(b) si sa dire il segno dei
 * due fattori, della somma di quattro esponenziali a cui si riduce no. Per
 * questo la condizione porta i fattori dell'argomento com'è scritto; se sono
 * troppi porta il prodotto di numeratore e denominatore del valore, che ha
 * lo stesso segno. */
static void require_positive(const Node *argument, Rat x) {
    const char *why = "logaritmo di un numero non positivo";
    Poly factor[MAX_SIGN_FACTORS];
    int count = 0;
    if (!conditions_on || failure) return;
    if (sign_factors(argument, factor, &count, 1) && count > 0) require_product(ALWAYS_POSITIVE, factor, count, why);
    else if (!failure) require(ALWAYS_POSITIVE, poly_mul(x.num, x.den), why);
}

/* Valuta una chiamata di funzione. */
static Rat eval_call(const Node *node) {
    /* Il logaritmo di un valore assoluto è esattamente ciò che logarithm()
     * calcola, quindi lì il valore assoluto si attraversa. Altrove non c'è
     * ancora una regola per trattarlo. */
    const Node *argument = node->a;
    if ((node->op == FN_LN || node->op == FN_LOG) && argument->kind == N_CALL && argument->op == FN_ABS)
        argument = argument->a;
    int absolute = (argument != node->a);

    Rat x = eval(argument);
    switch ((Function)node->op) {
    case FN_FACT:
        return eval_sequence('!', x);

    case FN_BINOM: {
        /* Coefficiente binomiale "x su r".
         *
         * 11i. r intero costante: il prodotto di (x - r + j) / j per j da 1 a
         *      r, cioè x (x-1) ... (x-r+1) / r!. È un polinomio in x e vale
         *      anche quando x < r, dove dà correttamente zero.
         * 11j. r simbolico: x! / (r! (x-r)!), valida per 0 <= r <= x. */
        Rat r = eval(node->b);
        long long count;
        if (rat_integer(r, &count)) {
            if (count < 0 || count > MAX_UNROLL) { fail("coefficiente binomiale con indice inferiore non valido"); return rat_int(0); }
            Rat result = rat_int(1);
            for (long long j = 1; j <= count && !failure; j++)
                result = rat_div(rat_mul(result, rat_add(x, rat_int(count - j), 1)), rat_int(j));
            return result;
        }
        Rat below = rat_mul(eval_sequence('!', r), eval_sequence('!', rat_add(x, r, 1)));
        return rat_div(eval_sequence('!', x), below);
    }

    case FN_SIN: case FN_COS: case FN_TAN: case FN_COT: {
        /* Formule di Eulero: con E = e^{i x},
         *     cos x = (E + 1/E) / 2,    sin x = (E - 1/E) / (2 i).
         * Dividere per 2 i equivale a moltiplicare per -i/2. */
        Rat forward = exp_i(x, 1), backward = exp_i(x, -1);
        Rat half = rat_div(rat_int(1), rat_int(2));
        Rat minus_half_i = {poly_const((Num){frac_int(0), (Frac){-1, 2}}), poly_const(num_int(1))};
        Rat cosine = rat_mul(rat_add(forward, backward, 0), half);
        Rat sine = rat_mul(rat_add(forward, backward, 1), minus_half_i);
        if (node->op == FN_SIN) return sine;
        if (node->op == FN_COS) return cosine;
        return node->op == FN_TAN ? rat_div(sine, cosine) : rat_div(cosine, sine);
    }

    case FN_ROOT: {
        /* La radice di indice r è la potenza 1/r. */
        long long index;
        if (!rat_integer(eval(node->b), &index) || index < 2) { fail("indice di radice non intero o minore di 2"); return rat_int(0); }
        return fractional_power(x, (Frac){1, index});
    }

    case FN_LN:
        if (!absolute) require_positive(argument, x);
        return absolute ? logarithm(x) : logarithm_of_positive(x);

    case FN_LOG: {
        /* Cambio di base: il logaritmo di x in base b è ln x / ln b. */
        if (!node->b) { fail("logaritmo senza base: scrivere \\ln oppure \\log_{b}"); return rat_int(0); }
        if (!absolute) require_positive(argument, x);
        Rat above = absolute ? logarithm(x) : logarithm_of_positive(x);
        Rat below = eval(node->b);
        require_positive(node->b, below);
        return rat_div(above, logarithm_of_positive(below));
    }

    case FN_ARCTAN:
        return arctangent(x);

    case FN_ABS:
        fail("valore assoluto non ancora supportato");
        return rat_int(0);
    }
    return rat_int(0);
}

/* Ritorna il valore del nodo con le variabili legate attuali. In caso di
 * errore `failure` è impostato e il valore non ha significato. */
static Rat eval(const Node *node) {
    /* Dopo un errore il risultato verrà scartato: ci si ferma subito, anche
     * per non svolgere somme con estremi privi di senso. */
    if (failure) return rat_int(0);

    switch (node->kind) {
    case N_NUM:
        return rat_int(node->value);
    case N_VAR: {
        const Rat *bound = env_lookup(node->name);
        return bound ? *bound : rat_atom(atom_var(node->name));
    }
    case N_SEQ:
        return eval_sequence(node->name, eval(node->a));
    case N_NEG:
        return rat_add(rat_int(0), eval(node->a), 1);
    case N_BIGOP:
        return eval_bigop(node);
    case N_CALL:
        return eval_call(node);
    case N_APPLY: {
        /* f(a) è la regola di f calcolata con la sua variabile legata ad a.
         * La regola non contiene altre lettere (lo controlla il capitolo 17),
         * quindi le variabili legate in questo momento non la toccano. */
        const Map *map = map_find(node->name);
        if (!map || !map->body) { fail("applicazione senza la regola che la definisce"); return rat_int(0); }
        env_push(map->parameter, eval(node->a));
        Rat value = eval(map->body);
        env_pop();
        return value;
    }
    default:
        break;
    }

    /* Operatori binari. I due operandi vanno valutati in istruzioni separate:
     * come argomenti della stessa chiamata il C non ne fisserebbe l'ordine, e
     * l'ordine decide con quale estremo nasce un atomo (vedi eval_bigop()). */
    Rat a = eval(node->a);
    Rat b = eval(node->b);
    switch (node->kind) {
    case N_ADD: return rat_add(a, b, 0);
    case N_SUB: return rat_add(a, b, 1);
    case N_MUL: return rat_mul(a, b);
    case N_DIV: return rat_div(a, b);
    default:    return power(a, b);
    }
}

/* =============================================================================
 * 12. TEST DI ANNULLAMENTO
 * ============================================================================
 *
 * "Questo polinomio è zero?" è la domanda a cui si riduce ogni dimostrazione.
 * Per quasi tutti gli atomi la risposta è nella forma canonica: è zero se non
 * ha termini. Due famiglie di atomi però soddisfano identità che la forma
 * canonica non vede, e vanno applicate qui.
 *
 *   12a. Radici. L'atomo "u preso come lettera", con u un polinomio, compare
 *        con esponenti frazionari: la radice quadrata di u è u^{1/2}. Ma
 *        u^1 non è una lettera nuova: è il polinomio u. Quindi di ogni
 *        esponente si separa la parte intera, che torna a essere il polinomio.
 *
 *   12b. Logaritmi. Una combinazione c1 ln|u1| + ... + ck ln|uk| con
 *        coefficienti interi è il logaritmo di |u1^c1 ... uk^ck|, quindi è
 *        zero quando quel prodotto vale 1 oppure -1. E quest'ultima è
 *        un'uguaglianza tra frazioni, senza più logaritmi.
 *
 *   12c. Arcotangenti. Per u reale, arctan(u) è l'argomento, cioè l'angolo,
 *        del numero complesso 1 + i u. Gli argomenti di un prodotto si
 *        sommano, quindi c1 arctan(u1) + ... + ck arctan(uk), con coefficienti
 *        interi, è l'argomento di Z = (1 + i u1)^c1 ... (1 + i uk)^ck, a meno
 *        di multipli di 2 pi. Se Z è un numero reale positivo il suo
 *        argomento è zero, e la combinazione è un multiplo di 2 pi; se
 *        inoltre è piccola in valore assoluto, quel multiplo è zero.
 *
 * Le tre regole sono vere identità, quindi un "sì" resta una dimostrazione.
 * Non sono complete: uno zero che dipende da altre identità (per esempio tra
 * radici di polinomi non scomposti) riceve un "no", e l'enunciato resta NON
 * DIMOSTRATO.
 */

/* Ritorna un polinomio che è zero se e solo se lo è p, in cui ogni atomo
 * "polinomio u preso come lettera" compare con esponente in [0, 1) (12a).
 *
 * Per un atomo x di base u: se k è il più piccolo tra le parti intere degli
 * esponenti di x (contando zero dove x manca), si moltiplica tutto p per
 * x^{-k}, che non cambia l'essere zero, e poi in ogni termine si sostituisce
 * la parte intera j dell'esponente con il polinomio u^j. I termini con la
 * stessa parte intera si raccolgono, e le potenze di u si moltiplicano con lo
 * schema di Horner: ((P_top u + P_{top-1}) u + ...) + P_k.
 *
 * Si procede dall'atomo creato per ultimo al primo: la base di un atomo
 * contiene solo atomi creati prima di lui, quindi sostituirla può far
 * comparire solo esponenti ancora da sistemare, mai già sistemati. */
static Poly poly_reduce_radicals(Poly p) {
    for (int x = atom_count - 1; x >= 0 && !failure; x--) {
        const Atom *atom = &atoms[x];
        if (atom->kind != ATOM_POW || atom->base != BASE_POLY || atom->exponent.count != 0) continue;

        /* Intervallo delle parti intere degli esponenti di x. */
        long long low = 0, high = 0;
        for (int i = 0; i < p.count; i++) {
            long long whole = frac_floor(mono_exponent(&p.term[i].mono, x));
            if (whole < low) low = whole;
            if (whole > high) high = whole;
        }
        if (low == 0 && high == 0) continue;
        if (high - low > MAX_POWER) { fail("esponente di una radice troppo grande"); break; }

        /* Horner, dalla parte intera più alta alla più bassa. */
        Poly result = poly_const(num_int(0));
        for (long long whole = high; whole >= low; whole--) {
            Term *buffer = terms_alloc(p.count);
            if (!buffer && p.count) return p;
            int used = 0;
            for (int i = 0; i < p.count; i++) {
                Term t = p.term[i];
                Frac e = mono_exponent(&t.mono, x);
                if (frac_floor(e) != whole) continue;
                t.mono = mono_with_exponent(&t.mono, x, frac_add(e, frac_int(-whole)));
                buffer[used++] = t;
            }
            Poly same = poly_finish(buffer, p.count, used);
            result = poly_add(poly_mul(result, base_polys[atom->id]), same, 0);
        }
        p = result;
    }
    return p;
}

/* Ritorna, come frazione, la base u dell'atomo ln|u| di indice atom. */
static Rat ln_base(int atom) {
    const Atom *a = &atoms[atom];
    if (a->base == BASE_PRIME) return rat_int(a->id);
    if (a->base == BASE_ATOM) return rat_atom((int)a->id);
    return (Rat){base_polys[a->id], poly_const(num_int(1))};
}

/* Ritorna 1 se p, non nullo, è una combinazione di logaritmi che vale zero
 * per la regola 12b.
 *
 * Ogni termine deve contenere un solo atomo LN, al primo grado. Il resto del
 * monomio è il suo "cofattore": i termini con lo stesso cofattore M formano un
 * gruppo M (c1 ln|u1| + ... + ck ln|uk|), e p è zero se lo è ogni gruppo.
 * I coefficienti razionali si rendono interi moltiplicando il gruppo per il
 * minimo comune multiplo dei denominatori. */
static int log_combination_vanishes(Poly p) {
    int *log_atom = arena_alloc((size_t)p.count * sizeof(int));
    Mono *cofactor = arena_alloc((size_t)p.count * sizeof(Mono));
    if (!log_atom || !cofactor) return 0;

    /* Separa in ogni termine l'atomo LN dal cofattore. */
    for (int i = 0; i < p.count; i++) {
        const Term *t = &p.term[i];
        log_atom[i] = -1;
        for (int f = 0; f < t->mono.count; f++) {
            if (atoms[t->mono.factor[f].atom].kind != ATOM_LN) continue;
            if (log_atom[i] >= 0 || t->mono.factor[f].exp.num != 1 || t->mono.factor[f].exp.den != 1) return 0;
            log_atom[i] = t->mono.factor[f].atom;
        }
        if (log_atom[i] < 0 || t->coef.im.num != 0) return 0;
        cofactor[i] = mono_with_exponent(&t->mono, log_atom[i], frac_int(0));
    }

    /* Un gruppo alla volta: il primo termine non ancora trattato e tutti
     * quelli con il suo stesso cofattore. log_atom[j] = -1 segna i trattati. */
    for (int i = 0; i < p.count; i++) {
        if (log_atom[i] < 0) continue;

        /* Minimo comune multiplo dei denominatori dei coefficienti. */
        long long scale = 1;
        for (int j = i; j < p.count; j++) {
            if (log_atom[j] < 0 || mono_cmp(&cofactor[j], &cofactor[i]) != 0) continue;
            long long den = p.term[j].coef.re.den;
            scale = scale / (long long)gcd128(scale, den) * den;
            if (scale > MAX_POWER) return 0;
        }

        /* Il prodotto delle basi, ognuna elevata al proprio coefficiente. */
        Rat product = rat_int(1);
        for (int j = p.count - 1; j >= i; j--) {
            if (log_atom[j] < 0 || mono_cmp(&cofactor[j], &cofactor[i]) != 0) continue;
            long long e = p.term[j].coef.re.num * (scale / p.term[j].coef.re.den);
            product = rat_mul(product, rat_pow_int(ln_base(log_atom[j]), e));
            log_atom[j] = -1;
        }
        if (failure) return 0;

        /* Il gruppo è zero se il prodotto vale 1 oppure -1. */
        if (poly_reduce_radicals(poly_add(product.num, product.den, 1)).count != 0 &&
            poly_reduce_radicals(poly_add(product.num, product.den, 0)).count != 0)
            return 0;
    }
    return 1;
}

/* Ritorna 1 se p, non nullo, è una combinazione di arcotangenti che vale zero
 * per la regola 12c.
 *
 * Come per i logaritmi, ogni termine deve contenere un solo "angolo" al primo
 * grado, e i termini si raggruppano per cofattore. Un angolo è un atomo ATAN
 * oppure pi greco, che si riscrive come 4 arctan(1).
 *
 * Per un gruppo c1 arctan(u1) + ... + ck arctan(uk), con i coefficienti resi
 * interi e uj = pj/qj:
 *
 *   - Il segno di Z. Invece di dividere, si moltiplica per i coniugati:
 *     Z è un reale positivo esattamente quando lo è
 *
 *         W = prodotto dei (1 + i sj uj)^|cj|,   sj = segno di cj,
 *
 *     perché W è Z per il quadrato del modulo dei fattori a esponente
 *     negativo. Scrivendo 1 + i sj uj = (qj + i sj pj)/qj, W è la frazione
 *     V/Q con V = prodotto dei (qj + i sj pj)^|cj| e Q = prodotto dei
 *     qj^|cj|. Si chiede che V non abbia parte immaginaria e che V Q sia
 *     evidentemente positivo: V e Q hanno allora lo stesso segno.
 *
 *   - La grandezza. Ogni arcotangente è minore di pi/2 in valore assoluto, e
 *     arctan(1) vale esattamente pi/4. Contando 2 per ogni arcotangente e 1
 *     per ogni arctan(1), se il totale pesato con |cj| non supera 8 la
 *     combinazione è minore di 8 * pi/4 = 2 pi in valore assoluto: l'unico
 *     multiplo di 2 pi possibile è zero. Serve almeno un'arcotangente diversa
 *     da arctan(1), altrimenti la disuguaglianza non sarebbe stretta. */
static int arctan_combination_vanishes(Poly p) {
    int pi = atom_var(VAR_PI);
    int *angle = arena_alloc((size_t)p.count * sizeof(int));
    Mono *cofactor = arena_alloc((size_t)p.count * sizeof(Mono));
    if (!angle || !cofactor) return 0;

    /* Separa in ogni termine l'angolo dal cofattore. */
    for (int i = 0; i < p.count; i++) {
        const Term *t = &p.term[i];
        angle[i] = -1;
        for (int f = 0; f < t->mono.count; f++) {
            int atom = t->mono.factor[f].atom;
            if (atoms[atom].kind != ATOM_ATAN && atom != pi) continue;
            if (angle[i] >= 0 || t->mono.factor[f].exp.num != 1 || t->mono.factor[f].exp.den != 1) return 0;
            angle[i] = atom;
        }
        if (angle[i] < 0 || t->coef.im.num != 0) return 0;
        cofactor[i] = mono_with_exponent(&t->mono, angle[i], frac_int(0));
    }

    /* Un gruppo alla volta, come in log_combination_vanishes(). */
    for (int i = 0; i < p.count; i++) {
        if (angle[i] < 0) continue;

        /* Coefficienti interi: minimo comune multiplo dei denominatori, dopo
         * aver contato pi greco come 4 arctan(1). */
        long long scale = 1;
        for (int j = i; j < p.count; j++) {
            if (angle[j] < 0 || mono_cmp(&cofactor[j], &cofactor[i]) != 0) continue;
            Frac c = frac_mul(p.term[j].coef.re, frac_int(angle[j] == pi ? 4 : 1));
            scale = scale / (long long)gcd128(scale, c.den) * c.den;
            if (scale > 8) return 0;
        }

        /* V, Q e il controllo sulla grandezza. */
        Poly v = poly_const(num_int(1)), q = poly_const(num_int(1));
        long long weight = 0;
        int proper = 0;
        for (int j = p.count - 1; j >= i; j--) {
            if (angle[j] < 0 || mono_cmp(&cofactor[j], &cofactor[i]) != 0) continue;
            int unit = (angle[j] == pi);
            Frac c = frac_mul(p.term[j].coef.re, frac_int(unit ? 4 * scale : scale));
            long long times = c.num < 0 ? -c.num : c.num;
            Rat u = unit ? rat_int(1) : atan_args[atoms[angle[j]].id];
            angle[j] = -1;
            weight += times * (unit ? 1 : 2);
            if (!unit) proper = 1;
            if (weight > 8) return 0;

            /* Il fattore q + i s p, moltiplicato |c| volte. */
            Num turn = {frac_int(0), frac_int(c.num < 0 ? -1 : 1)};
            Poly factor = poly_add(u.den, poly_mul(poly_const(turn), u.num), 0);
            for (long long k = 0; k < times; k++) {
                v = poly_mul(v, factor);
                q = poly_mul(q, u.den);
            }
        }
        if (failure || !proper) return 0;

        /* V reale, e V Q positivo. */
        for (int k = 0; k < v.count; k++)
            if (v.term[k].coef.im.num != 0) return 0;
        if (!poly_is_positive(poly_mul(v, q))) return 0;
    }
    return 1;
}

/* Ritorna 1 se p è zero, tenendo conto delle identità 12a, 12b e 12c. Un
 * errore durante il conto (in `failure`) vale come "no". */
static int poly_vanishes(Poly p) {
    p = poly_reduce_radicals(p);
    if (failure) return 0;
    return p.count == 0 || log_combination_vanishes(p) || arctan_combination_vanishes(p);
}

/* =============================================================================
 * 13. INDUZIONE: CASO BASE, PASSO, VERDETTO
 * ============================================================================
 *
 * Sia D(n) = L(n) - R(n) la differenza dei due membri. L'enunciato dice che
 * D(n) = 0 per ogni n >= n0. Il principio di induzione chiede due cose:
 *
 *   13a. Caso base: D(n0) = 0. Con n = n0 ogni somma e prodotto si svolge e
 *        D(n0) è un'espressione senza n: deve annullarsi.
 *
 *   13b. Passo: per ogni n >= n0, da D(n) = 0 segue D(n+1) = 0.
 *        Si scrivono D(n) e D(n+1) con gli stessi atomi. L'ipotesi induttiva
 *        D(n) = 0 è un'equazione tra gli atomi: se un atomo X (di solito la
 *        somma dell'enunciato) vi compare al primo grado, numeratore
 *        A X + B = 0, se ne ricava X = -B/A. Sostituendo nel numeratore di
 *        D(n+1), che è un polinomio C0 + C1 X + ... + Cd X^d, e moltiplicando
 *        per A^d si ottiene
 *
 *            C0 A^d + C1 (-B) A^{d-1} + ... + Cd (-B)^d
 *
 *        che non contiene più X: se è il polinomio nullo il passo è
 *        dimostrato. È ciò che si fa a mano: "per ipotesi induttiva la somma
 *        fino a n vale R(n), quindi la somma fino a n+1 vale R(n) + f(n+1), e
 *        resta da controllare che sia R(n+1)".
 *
 * Quando l'ipotesi si può usare. Ricavare X = -B/A richiede A diverso da zero
 * per ogni n >= n0. Senza questa cautela si "dimostrerebbero" enunciati falsi:
 * in (n-1) S(n) = (n-1) g(n) il caso base n = 1 dice 0 = 0 qualunque sia g.
 * Il programma accetta due situazioni in cui la condizione è garantita:
 *
 *   - A è proporzionale al denominatore di D: allora X compare in D con un
 *     coefficiente costante (il caso "somma = formula"), e A non si annulla
 *     dove l'enunciato è definito;
 *   - A non dipende da n: dipende solo dai parametri, e un'uguaglianza tra
 *     funzioni razionali dei parametri, vera dove A non è zero, è vera ovunque
 *     sia definita.
 *
 * Negli altri casi servirebbe dimostrare che A non ha zeri interi: il
 * programma non lo sa fare e risponde NON DIMOSTRATO.
 *
 * Se l'induzione non riesce si cerca un controesempio: D valutata in pochi n
 * successivi. Un valore che è un numero diverso da zero prova che l'enunciato
 * è falso.
 *
 * Se l'induzione riesce, il verdetto non è ancora dato:
 *
 *   13c. si decide se i due membri sono definiti per ogni n, con un valore
 *        fisso dei parametri;
 *   13d. oppure, se un valore fisso non c'è, per tutti i valori dei parametri
 *        abbastanza grandi;
 *   13e. la dimostrazione viene ricontrollata con i numeri (verify_instances());
 *   13f. si scrive la dimostrazione, con i suoi passaggi;
 *   13g. ogni formula scritta viene riletta e ricontrollata;
 *   13h. si scrive il certificato dell'induzione e lo si dà al verificatore
 *        indipendente, checker.c.
 */

/* Esito di un tentativo di dimostrazione. */
typedef enum { PROVED, REFUTED, NOT_PROVED, UNSUPPORTED } Outcome;

/* Il verdetto su un enunciato, di qualunque tipo.
 *
 * I campi variable, base e witness riguardano le dimostrazioni per induzione;
 * per gli altri enunciati variable vale 0, method dice come sono stati
 * dimostrati e il controesempio, se c'è, è il testo in detail. Le stringhe
 * puntate non appartengono al verdetto: method e note sono costanti; reason,
 * detail, proof e certificate restano valide fino al verdetto successivo. */
typedef struct {
    Outcome outcome;
    int variable;        /* variabile di induzione, o 0 */
    long long base;      /* PROVED per induzione: il caso base n0 */
    long long witness;   /* REFUTED per induzione: un n in cui non vale */
    const char *reason;  /* NOT_PROVED, UNSUPPORTED: perché, in italiano;
                            PROVED per induzione senza certificato: perché
                            il certificato manca */
    const char *detail;  /* REFUTED senza induzione: il controesempio, o NULL */
    const char *proof;   /* dimostrazione scritta in LaTeX, o NULL */
    const char *method;  /* PROVED senza induzione: come, in italiano */
    const char *note;    /* REFUTED: avvertenza su come è stato letto l'enunciato, o NULL */
    const char *certificate;  /* PROVED: il certificato che il verificatore
                                 indipendente ha accettato (13h), o NULL */
} Verdict;

/* Il testo della dimostrazione dell'enunciato in corso: LaTeX, in inglese come
 * gli altri testi destinati a un documento. proof_reset() lo svuota,
 * proof_add() vi aggiunge un pezzo con la sintassi di printf. Un testo più
 * lungo di PROOF_BYTES viene troncato. */
static char proof_text[PROOF_BYTES];
static size_t proof_length;

/* Quanti passi di una dimostrazione scritta non hanno superato il controllo
 * indipendente, dall'avvio del programma. Le dimostrazioni sugli insiemi e
 * quelle con quantificatori ricontrollano ogni fatto che stanno per scrivere
 * con un metodo diverso da quello che lo ha ricavato (16h e 16i, e i
 * controlli del capitolo 18); quelle per induzione rileggono ogni formula
 * scritta e la ricalcolano con i numeri (13g). Se un passo non regge, la
 * dimostrazione viene abbandonata e il contatore sale. Sale anche quando il
 * verificatore indipendente respinge il certificato di un'induzione (13h).
 * Deve restare a zero: un valore diverso vuol dire che una regola ha
 * sbagliato, e i test lo controllano. */
static int unjustified_steps;

static void proof_reset(void) {
    proof_length = 0;
    proof_text[0] = 0;
}

static void proof_add(const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    int written = vsnprintf(proof_text + proof_length, PROOF_BYTES - proof_length, format, arguments);
    va_end(arguments);
    if (written > 0) proof_length += (size_t)written;
    if (proof_length >= PROOF_BYTES) proof_length = PROOF_BYTES - 1;
}

/* Ritorna L - R per l'enunciato, con la variabile di induzione legata a n e i
 * parametri legati ai valori del punto in uso, se ce n'è uno. */
static Rat difference(const Statement *statement, Rat n) {
    env_count = 0;
    library_depth = 0;

    /* I parametri vanno legati per primi: env_varies() li riconosce dal posto. */
    for (int i = 0; i < point_count; i++) env_push(point_name[i], rat_frac(point_value[i]));
    env_push(induction_var, n);
    Rat left = eval(statement->lhs);
    Rat right = eval(statement->rhs);
    env_count = 0;
    return rat_add(left, right, 1);
}

/* Passo induttivo (13b): d è D(n), d1 è D(n+1), scritte con gli stessi atomi.
 * Ritorna 1 se da D(n) = 0 segue D(n+1) = 0, e allora mette in *used l'atomo
 * ricavato dall'ipotesi, oppure -1 se l'ipotesi non è servita. */
static int step_holds(Rat d, Rat d1, int *used) {
    /* D(n+1) è già zero: non serve nemmeno l'ipotesi. */
    *used = -1;
    if (poly_vanishes(d1.num)) return 1;

    /* Si prova a ricavare dall'ipotesi ogni somma o prodotto, uno alla volta. */
    for (int x = 0; x < atom_count && !failure; x++) {
        if (atoms[x].kind != ATOM_BIGOP) continue;

        /* Ipotesi: A x + B = 0, con A che non si annulla. */
        Poly hypothesis[2], part[MAX_DEGREE + 1];
        if (poly_split(d.num, x, hypothesis, 1) != 1) continue;
        Poly a = hypothesis[1], b = hypothesis[0];
        if (!poly_proportional(a, d.den) && poly_depends_on_n(a)) continue;

        /* Tesi: C0 + C1 x + ... + Cd x^d = 0. */
        int degree = poly_split(d1.num, x, part, MAX_DEGREE);
        if (degree < 0) continue;

        /* Sostituzione x = -B/A, moltiplicata per A^d. Le potenze di -B e di A
         * si accumulano mentre j cresce e d - j decresce. */
        Poly minus_b = poly_add(poly_const(num_int(0)), b, 1);
        Poly b_power = poly_const(num_int(1));
        Poly sum = poly_const(num_int(0));
        for (int j = 0; j <= degree; j++) {
            Poly piece = poly_mul(part[j], b_power);
            for (int k = j; k < degree; k++) piece = poly_mul(piece, a);
            sum = poly_add(sum, piece, 0);
            b_power = poly_mul(b_power, minus_b);
        }
        if (!failure && poly_vanishes(sum)) { *used = x; return 1; }
    }
    return 0;
}

/* 13c. DEFINITO PER OGNI n
 *
 * L'induzione è algebra sulle lettere: prova che i due membri sono uguali
 * "dove sono definiti". Perché il verdetto "per ogni n >= n0" sia vero serve
 * anche che lo siano per ogni n >= n0. In
 *
 *     (n - 50) / (n - 50) = 1
 *
 * l'algebra non trova nulla da ridire, e i primi n nemmeno, ma per n = 50 il
 * membro sinistro non esiste. Lo stesso vale per le divisioni che fa il conto
 * stesso: scrivere un prodotto fino a n - 1 come "prodotto fino a n diviso
 * l'ultimo fattore" (vedi eval_bigop()) è lecito solo dove quel fattore non è
 * zero.
 *
 * Le condizioni di esistenza (capitolo 7) vengono raccolte calcolando D(n) e
 * D(n+1) con n lettera e i parametri legati a numeri: sono polinomi negli
 * atomi, e condition_holds() decide ciascuna per ogni intero n >= n0. Bastano
 * queste due valutazioni: il caso n = n0 è già stato calcolato con i numeri,
 * D(n+1) contiene tutto ciò che compare passando da n a n + 1 (i due membri
 * in n + 1 e i termini nuovi di ogni somma), e D(n) le divisioni fatte per
 * scrivere l'ipotesi.
 *
 * La decisione ha tre risposte: vale (1), non vale (0, con un n in cui
 * fallisce), non so (-1). Le regole, nell'ordine in cui si provano (nel file
 * stanno nell'ordine in cui una ha bisogno dell'altra):
 *
 *   13c-1. Un seno o un coseno, eventualmente per un fattore positivo: il
 *          segno viene dal quadrante in cui cade l'angolo (trig_sign()).
 *   13c-2. Si raccoglie il monomio comune ai termini. I suoi atomi devono
 *          essere mai nulli (per "diverso da zero") o positivi (per i segni):
 *          n da 1 in su, le potenze di un primo, i prodotti di fattori
 *          positivi, le potenze di una base che a sua volta lo è.
 *   13c-3. Ciò che resta è un polinomio in una sola quantità che cresce con
 *          n, la "variabile principale": n stessa, o una potenza come 2^n.
 *          Il segno si decide con il limite di Cauchy (sign_holds()).
 *   13c-4. Ciò che resta contiene la variabile principale e radici di numeri
 *          primi, come la radice di 2: gli zeri si trovano con le radici
 *          razionali (zero_free()).
 *   13c-5. Altrimenti si guarda se il segno è evidente: coefficienti tutti
 *          dello stesso segno e fattori non negativi (evident_sign()).
 *   13c-6. Un'espressione che non decresce ed è positiva per n = n0, o che
 *          non cresce ed è negativa (monotone_sign()).
 *   13c-7. Un'espressione in n e in potenze come 2^n e 3^n, che crescono a
 *          velocità diverse: il segno per differenze (difference_rule()).
 *   13c-8. Per "diverso da zero", con seni e coseni: esponenziali complessi
 *          che non possono cancellarsi (exponentials_never_cancel()).
 *
 * Ogni regola conclude solo ciò che è certo. In particolare nessuna prende
 * per "diverso da zero" un'espressione solo perché non è il polinomio nullo:
 * tra gli atomi ci sono identità che la forma canonica non vede. Quando
 * nessuna regola decide, la risposta è "non so", e il verdetto sarà
 * NON DIMOSTRATO: di n! - 5 il programma non sa dire che non si annulla. */

/* Ritorna il segno, -1, 0 o 1, di c[0] + c[1] x + ... + c[degree] x^degree
 * nell'intero x, con lo schema di Horner. Se i numeri escono dall'aritmetica
 * esatta mette 0 in *known, che chi chiama ha messo a 1. */
static int univariate_sign(const long long *c, int degree, long long x, int *known) {
    __int128 value = 0;
    for (int i = degree; i >= 0; i--)
        if (__builtin_mul_overflow(value, (__int128)x, &value) || __builtin_add_overflow(value, (__int128)c[i], &value)) {
            *known = 0;
            return 0;
        }
    return (value > 0) - (value < 0);
}

/* Ritorna +1 se il monomio, per n >= base, è positivo e non decresce al
 * crescere di n; -1 se è positivo e non cresce; 0 se non si sa, o se non ha
 * fattori. I mattoni sono n (da 1 in su) e le potenze p^M di un primo, che
 * seguono l'andamento dell'esponente M perché p > 1; un esponente negativo
 * rovescia l'andamento. Un prodotto di quantità positive che vanno tutte
 * nello stesso verso va in quel verso. */
static int mono_trend(const Mono *m, long long base) {
    int trend = 0;
    for (int i = 0; i < m->count; i++) {
        const Atom *a = &atoms[m->factor[i].atom];
        int own;
        if (a->kind == ATOM_VAR && a->id == induction_var && base >= 1) own = 1;
        else if (a->kind == ATOM_POW && a->base == BASE_PRIME) own = mono_trend(&a->exponent, base);
        else return 0;
        if (m->factor[i].exp.num < 0) own = -own;
        if (own == 0 || (trend && own != trend)) return 0;
        trend = own;
    }
    return trend;
}

/* Mette in *value il valore del monomio per n = base e ritorna 1, se è un
 * razionale che si calcola con potenze intere di n e di numeri primi;
 * altrimenti ritorna 0. Numeri troppo grandi lasciano l'errore in `failure`. */
static int mono_at(const Mono *m, long long base, Frac *value) {
    *value = frac_int(1);
    for (int i = 0; i < m->count && !failure; i++) {
        const Atom *a = &atoms[m->factor[i].atom];
        Frac e = m->factor[i].exp, v, power;
        if (e.den != 1 || e.num > 62 || e.num < -62) return 0;
        if (a->kind == ATOM_VAR && a->id == induction_var) {
            v = frac_int(base);
        } else if (a->kind == ATOM_POW && a->base == BASE_PRIME) {
            if (!mono_at(&a->exponent, base, &power) || power.den != 1 || power.num > 62 || power.num < -62) return 0;
            v = frac_int(1);
            for (long long k = llabs(power.num); k > 0 && !failure; k--)
                v = power.num > 0 ? frac_mul(v, frac_int(a->id)) : frac_div(v, frac_int(a->id));
        } else {
            return 0;
        }
        if (v.num == 0 && e.num < 0) return 0;
        for (long long k = llabs(e.num); k > 0 && !failure; k--) *value = e.num > 0 ? frac_mul(*value, v) : frac_div(*value, v);
    }
    return !failure;
}

/* LA VARIABILE PRINCIPALE
 *
 * Le regole 13c-3 e 13c-4 decidono un polinomio in una sola quantità che
 * cresce con n prendendo valori interi: n stessa, oppure una potenza come
 * 2^n. Di 2 A^2 - 3 A + 1 con A = 2^n si trovano le radici intere in A, e poi
 * si guarda se 2^n le raggiunge. Così la stessa regola che per n - 50 trova
 * n = 50 trova per 2^n - 4096 il valore n = 12. */

/* Ritorna l'atomo che fa da variabile principale di q per n >= base: l'unico
 * atomo di q che dipende da n, se è n oppure una potenza p^M di un primo con
 * M intero e crescente. Se q non dipende da n ritorna n. Ritorna -1 se gli
 * atomi che dipendono da n sono più d'uno, o di un altro tipo. */
static int main_atom(Poly q, long long base) {
    int found = -1;
    for (int i = 0; i < q.count; i++)
        for (int f = 0; f < q.term[i].mono.count; f++) {
            int atom = q.term[i].mono.factor[f].atom;
            if (!atoms[atom].depends_on_n) continue;
            if (found >= 0 && atom != found) return -1;
            found = atom;
        }
    if (found < 0) return atom_var(induction_var);
    const Atom *a = &atoms[found];
    if (a->kind == ATOM_VAR) return found;
    int power = a->kind == ATOM_POW && a->base == BASE_PRIME && mono_is_integer(&a->exponent);
    return power && mono_trend(&a->exponent, base) == 1 ? found : -1;
}

/* Mette in *value il valore intero della variabile principale `atom` per
 * n = at, e ritorna 1; ritorna 0 se non è un intero che si sa calcolare. */
static int main_at(int atom, long long at, long long *value) {
    Mono single = mono_atom(atom, frac_int(1));
    Frac v;
    if (!mono_at(&single, at, &v) || v.den != 1) return 0;
    *value = v.num;
    return 1;
}

/* 13c-3. Decide se il polinomio a coefficienti interi c[0..degree] nella
 * variabile principale `atom`, con c[degree] diverso da zero, è positivo
 * (strict) o non negativo per ogni intero n >= base. Ritorna 1, oppure 0 con
 * il primo n in cui fallisce in *where, oppure -1 se i valori da provare sono
 * troppi o troppo grandi.
 *
 * Limite di Cauchy: ogni radice reale r del polinomio soddisfa
 * |r| < 1 + M, dove M è il più grande |c[i]| / |c[degree]| con i < degree.
 * Infatti se |x| >= 1 + M la somma dei termini di grado più basso vale al più
 * M |c[degree]| (|x|^degree - 1) / (|x| - 1) <= |c[degree]| (|x|^degree - 1),
 * meno del termine di grado massimo: il polinomio non può annullarsi.
 *
 * Oltre il limite il segno quindi non cambia più. La variabile principale
 * cresce con n: si provano i suoi valori uno dopo l'altro finché non
 * superano il limite. Se nessuno fallisce, nemmeno l'ultimo, il segno resta
 * quello buono per sempre. */
static int sign_holds(const long long *c, int degree, int strict, int atom, long long base, long long *where) {
    long long largest = 0, x;
    for (int i = 0; i < degree; i++)
        if (llabs(c[i]) > largest) largest = llabs(c[i]);
    long long bound = largest / llabs(c[degree]) + 2;
    for (long long at = base; at - base <= MAX_SCAN; at++) {
        int known = 1;
        if (!main_at(atom, at, &x)) return -1;
        int sign = univariate_sign(c, degree, x, &known);
        if (!known) return -1;
        if (sign < 0 || (strict && sign == 0)) { *where = at; return 0; }
        if (degree == 0 || x >= bound) return 1;
    }
    return -1;
}

/* 13c-4. Decide se q è diverso da zero per ogni intero n >= base, quando la
 * variabile principale `atom` vi compare solo con potenze intere e gli altri
 * atomi sono radici di numeri primi. Ritorna 1, oppure 0 con il più piccolo n in cui
 * si annulla in *where, oppure -1 se q non ha questa forma o un conto non
 * riesce.
 *
 * Detta A la variabile principale, q è una somma c_1(A) m_1 + ... +
 * c_r(A) m_r, dove gli m_j sono monomi distinti negli atomi costanti e i c_j
 * polinomi a coefficienti razionali. Se gli m_j sono linearmente
 * indipendenti sui razionali, q si annulla dove A vale r solo se tutti i c_j
 * si annullano in r. I valori interi di A in cui q si annulla sono allora
 * tra le radici intere di uno qualunque dei c_j, che il teorema delle radici
 * razionali elenca tutte (rational_roots()). Di ognuna si guarda se A la
 * raggiunge per qualche n >= base, e se annulla tutto q.
 *
 * L'indipendenza va garantita, non supposta: qui un legame nascosto tra gli
 * m_j farebbe perdere uno zero, cioè accettare una divisione per zero. Per
 * questo gli atomi costanti ammessi sono solo le radici di numeri primi,
 * come la radice di 2 e la radice cubica di 5: i loro prodotti distinti sono
 * linearmente indipendenti sui razionali (teorema di Besicovitch; il caso
 * più noto è che 1, radice di 2, radice di 3 e radice di 6 lo sono). Altri
 * atomi costanti soddisfano identità: arctan(1/2) + arctan(1/3) = pi/4, e
 * 2 cos(pi/3) = 1 tra le potenze di e^{i pi}. */
static int zero_free(Poly q, int atom, long long base, long long *where) {
    const long long cap = 1000000000;
    int answer = 1;
    Poly part[MAX_UNIVARIATE + 1];
    Frac coefficient[MAX_UNIVARIATE + 1], root[MAX_UNIVARIATE + 1];
    long long c[MAX_UNIVARIATE + 1];

    /* q = part[0] + part[1] A + ... + part[degree] A^degree. */
    int degree = poly_split(q, atom, part, MAX_UNIVARIATE);
    if (degree < 0) return -1;
    for (int j = 0; j <= degree; j++)
        for (int i = 0; i < part[j].count; i++)
            for (int f = 0; f < part[j].term[i].mono.count; f++) {
                const Atom *a = &atoms[part[j].term[i].mono.factor[f].atom];
                if (a->kind != ATOM_POW || a->base != BASE_PRIME || a->exponent.count != 0) return -1;
            }

    /* Il polinomio c(A) che moltiplica uno dei monomi costanti: quello del
     * primo termine della parte di grado massimo. Di un coefficiente
     * complesso si guarda la parte reale, o quella immaginaria se la reale
     * manca: c(r) = 0 le annulla entrambe. */
    const Term *chosen = &part[degree].term[0];
    int imaginary = chosen->coef.re.num == 0;
    __int128 scale = 1;
    for (int j = 0; j <= degree; j++) {
        coefficient[j] = frac_int(0);
        for (int i = 0; i < part[j].count; i++)
            if (mono_cmp(&part[j].term[i].mono, &chosen->mono) == 0)
                coefficient[j] = imaginary ? part[j].term[i].coef.im : part[j].term[i].coef.re;
        scale = scale / gcd128(scale, coefficient[j].den) * coefficient[j].den;
        if (scale > cap) return -1;
    }
    for (int j = 0; j <= degree; j++) {
        __int128 value = (__int128)coefficient[j].num * (scale / coefficient[j].den);
        if (value > cap || value < -cap) return -1;
        c[j] = (long long)value;
    }

    /* Le radici intere di c(A). */
    int roots = degree >= 1 ? rational_roots(c, degree, root) : 0;
    if (roots < 0) return -1;
    for (int k = 0; k < roots; k++) {
        if (root[k].den != 1) continue;

        /* A cresce: raggiunge la radice per un solo n, o la salta. */
        long long at = base, x = 0;
        while (at - base <= MAX_SCAN && main_at(atom, at, &x) && x < root[k].num) at++;
        if (failure || at - base > MAX_SCAN) return -1;
        if (x != root[k].num) continue;

        /* La radice annulla tutto q? Schema di Horner sui polinomi. */
        Poly value = part[degree], r = poly_const(num_int(root[k].num));
        for (int j = degree - 1; j >= 0; j--) value = poly_add(poly_mul(value, r), part[j], 0);
        if (failure) return -1;
        if (poly_vanishes(value) && (answer == 1 || at < *where)) { answer = 0; *where = at; }
    }
    return answer;
}

static int condition_holds(Requirement kind, Poly p, long long base, long long *where);

/* Ritorna 1 se l'atomo è certamente positivo per ogni n >= base: n da 1 in
 * su, pi greco, le potenze di un primo, i prodotti di fattori positivi, le
 * potenze di una base positiva (una radice di n + 1 è l'atomo "n + 1" elevato
 * a 1/2), il logaritmo di un primo, l'arcotangente di un argomento positivo.
 * Zero vuol dire "non si sa". */
static int atom_is_positive(int atom, long long base) {
    const Atom *a = &atoms[atom];
    long long where;
    if (a->kind == ATOM_VAR) return a->id == VAR_PI || (a->id == induction_var && base >= 1);
    if (a->kind == ATOM_BIGOP) return a->positive;
    if (a->kind == ATOM_ATAN) {
        Rat u = atan_args[a->id];
        return condition_holds(ALWAYS_POSITIVE, poly_mul(u.num, u.den), base, &where) == 1;
    }
    if (a->kind == ATOM_LN) return a->base == BASE_PRIME;
    if (a->kind != ATOM_POW || a->base == BASE_EI) return 0;
    if (a->base == BASE_PRIME) return 1;
    if (a->base == BASE_ATOM) return atom_is_positive((int)a->id, base);
    return condition_holds(ALWAYS_POSITIVE, base_polys[a->id], base, &where) == 1;
}

/* Decide se l'atomo è diverso da zero per ogni n >= base: 1, 0 (con lo zero
 * in *where) o -1. Una potenza si annulla solo se si annulla la base; e^{i M}
 * mai; un'arcotangente solo se si annulla l'argomento. */
static int atom_never_zero(int atom, long long base, long long *where) {
    const Atom *a = &atoms[atom];
    if (a->kind == ATOM_VAR && a->id == induction_var) {
        *where = 0;
        return base >= 1;
    }
    if (a->kind == ATOM_VAR) return a->id == VAR_PI ? 1 : -1;
    if (a->kind == ATOM_BIGOP) return a->positive ? 1 : -1;
    if (a->kind == ATOM_ATAN) return condition_holds(NEVER_ZERO, atan_args[a->id].num, base, where);
    if (a->kind == ATOM_REC) return -1;
    if (a->base == BASE_PRIME || (a->kind == ATOM_POW && a->base == BASE_EI)) return 1;
    Poly inner = a->base == BASE_ATOM ? rat_atom((int)a->id).num : base_polys[a->id];
    if (a->kind == ATOM_POW) return condition_holds(NEVER_ZERO, inner, base, where);

    /* Un logaritmo ln|u| vale zero solo dove u = 1 oppure u = -1. */
    int below = condition_holds(NEVER_ZERO, poly_add(inner, poly_const(num_int(1)), 1), base, where);
    return below == 1 ? condition_holds(NEVER_ZERO, poly_add(inner, poly_const(num_int(1)), 0), base, where) : below;
}

/* 13c-5. Ritorna 1 se q è evidentemente positivo per ogni n >= base, -1 se
 * evidentemente negativo, 0 se non si vede. Evidente vuol dire: coefficienti
 * tutti dello stesso segno; ogni fattore positivo, oppure non negativo con
 * esponente positivo, oppure potenza pari di un atomo reale; e almeno un
 * termine fatto solo di fattori positivi, che tiene la somma lontana da
 * zero. */
static int evident_sign(Poly q, long long base) {
    int sign = 0, strict = 0;
    for (int i = 0; i < q.count; i++) {
        const Term *t = &q.term[i];
        int positive = 1, own = t->coef.re.num > 0 ? 1 : -1;
        if (t->coef.im.num != 0 || (sign && own != sign)) return 0;
        sign = own;
        for (int f = 0; f < t->mono.count; f++) {
            const Factor *factor = &t->mono.factor[f];
            if (atom_is_positive(factor->atom, base)) continue;
            int even = factor->exp.den == 1 && factor->exp.num % 2 == 0;
            if (factor->exp.num < 0 || !(atom_is_nonnegative(factor->atom) || (even && atom_is_real(factor->atom)))) return 0;
            positive = 0;
        }
        if (positive) strict = 1;
    }
    return strict ? sign : 0;
}

/* 13c-6. Ritorna 1 se q è positivo per ogni n >= base, -1 se è negativo, 0
 * se questa regola non lo stabilisce.
 *
 * Se ogni termine che dipende da n non decresce al crescere di n (un
 * monomio che non decresce con coefficiente positivo, o uno che non cresce
 * con coefficiente negativo), q non decresce: se è positivo per n = base lo
 * è per sempre. Allo stesso modo, se nessun termine cresce e q è negativo in
 * base, resta negativo. Così 2^{2^n} - 1 è positivo da n = 1 in poi, mentre
 * di 2^n - 4096, che in n = 1 è negativo e cresce, non si conclude nulla (e
 * infatti si annulla per n = 12). */
static int monotone_sign(Poly q, long long base) {
    int direction = 0;
    Frac total = frac_int(0), v;
    for (int i = 0; i < q.count; i++) {
        const Term *t = &q.term[i];
        if (t->coef.im.num != 0 || !mono_at(&t->mono, base, &v)) return 0;
        total = frac_add(total, frac_mul(t->coef.re, v));
        if (t->mono.count == 0) continue;
        int own = mono_trend(&t->mono, base);
        if (t->coef.re.num < 0) own = -own;
        if (own == 0 || (direction && own != direction)) return 0;
        direction = own;
    }
    if (failure) return 0;
    if (direction >= 0 && total.num > 0) return 1;
    if (direction <= 0 && total.num < 0) return -1;
    return 0;
}

/* Divide il monomio m in esponenziali complessi e resto. Mette in *wave i
 * fattori e^{i M} di m e in *angle l'angolo complessivo, cioè la somma degli
 * esponenti: il prodotto dei fattori di *wave vale e^{i angle}. Ritorna 1 se
 * ci riesce; 0 se tra gli esponenziali ce n'è uno a cui si applicano le
 * identità con pi greco (4f, 4g), che qui non si sanno trattare. */
static int split_wave(const Mono *m, Mono *wave, Poly *angle) {
    wave->count = 0;
    *angle = poly_const(num_int(0));
    for (int f = 0; f < m->count; f++) {
        const Atom *a = &atoms[m->factor[f].atom];
        if (a->kind != ATOM_POW || a->base != BASE_EI) continue;
        if (a->pi != PI_NONE) return 0;
        Term t;
        t.coef = num_frac(m->factor[f].exp);
        t.mono = a->exponent;
        *angle = poly_add(*angle, poly_term(t), 0);
        wave->factor[wave->count++] = m->factor[f];
    }
    return 1;
}

/* Ritorna il termine t privato dei fattori di wave. */
static Term term_without(Term t, const Mono *wave) {
    for (int f = 0; f < wave->count; f++) t.mono = mono_with_exponent(&t.mono, wave->factor[f].atom, frac_int(0));
    return t;
}

/* Ritorna 1 se l'atomo vale un numero algebrico per ogni n intero: n, i
 * termini delle successioni per ricorrenza, e le potenze con base algebrica
 * ed esponente razionale, come 2^n o la radice di n + 1. Non lo sono, o non
 * si sa, pi greco, i logaritmi, le arcotangenti, le somme, i parametri.
 * Serve alla regola 13c-8. */
static int poly_is_algebraic(Poly p);
static int atom_is_algebraic(int atom) {
    const Atom *a = &atoms[atom];
    if (a->kind == ATOM_VAR) return a->id == induction_var;
    if (a->kind == ATOM_REC) return 1;
    if (a->kind != ATOM_POW || a->base == BASE_EI || !mono_is_integer(&a->exponent)) return 0;
    if (a->base == BASE_ATOM) return atom_is_algebraic((int)a->id);
    return a->base == BASE_PRIME || poly_is_algebraic(base_polys[a->id]);
}

/* Ritorna 1 se ogni atomo del monomio vale un numero algebrico. */
static int mono_is_algebraic(const Mono *m) {
    for (int i = 0; i < m->count; i++)
        if (!atom_is_algebraic(m->factor[i].atom)) return 0;
    return 1;
}

/* Ritorna 1 se ogni atomo di p vale un numero algebrico. */
static int poly_is_algebraic(Poly p) {
    for (int i = 0; i < p.count; i++)
        if (!mono_is_algebraic(&p.term[i].mono)) return 0;
    return 1;
}

/* 13c-8. Ritorna 1 se q, che contiene esponenziali complessi e^{i M}, è
 * certamente diverso da zero per ogni intero n >= base; -1 se non lo sa
 * stabilire. È il caso dei denominatori con seni e coseni, come sin(n x) con
 * x legato a un numero razionale.
 *
 * Teorema di Lindemann-Weierstrass: se t_1, ..., t_r sono numeri algebrici
 * distinti, allora e^{t_1}, ..., e^{t_r} sono linearmente indipendenti sui
 * numeri algebrici: una loro combinazione a coefficienti algebrici vale zero
 * solo se tutti i coefficienti sono zero. (Ne segue, per esempio, che sin(1)
 * non è zero, cioè che pi greco non è razionale.)
 *
 * Si scrive q = g_1 e^{i t_1} + ... + g_r e^{i t_r}, raccogliendo i termini
 * con gli stessi esponenziali: ogni t_j è la somma degli esponenti, un
 * polinomio negli atomi, e ogni g_j è il polinomio che resta. Se tutti gli
 * atomi coinvolti sono algebrici, per ogni n intero i t_j e i g_j sono numeri
 * algebrici. Basta allora un indice j tale che, per ogni n >= base, g_j non
 * sia zero e t_j sia diverso da tutti gli altri t_k: anche se altri
 * esponenti coincidono tra loro, e^{i t_j} resta con il suo coefficiente non
 * nullo, e la somma non può annullarsi. Le due richieste sono a loro volta
 * condizioni "mai zero", decise da condition_holds(). */
static int exponentials_never_cancel(Poly q, long long base) {
    enum { MOST = 24 };
    Mono wave[MOST];   /* gli esponenziali di ogni termine */
    Poly angle[MOST];  /* il loro esponente complessivo t */
    long long where;
    if (q.count > MOST) return -1;

    /* Ogni termine si divide in esponenziali e resto, tutti algebrici. */
    int waves = 0;
    for (int i = 0; i < q.count; i++) {
        if (!split_wave(&q.term[i].mono, &wave[i], &angle[i])) return -1;
        Term rest = term_without(q.term[i], &wave[i]);
        if (!mono_is_algebraic(&rest.mono) || !poly_is_algebraic(angle[i])) return -1;
        waves += wave[i].count;
    }
    if (!waves || failure) return -1;

    /* Si cerca il termine j che non può essere cancellato. */
    for (int j = 0; j < q.count; j++) {
        int alone = 1, repeated = 0;
        Poly g = poly_const(num_int(0));
        for (int k = 0; k < j; k++)
            if (mono_cmp(&wave[k], &wave[j]) == 0) repeated = 1;
        if (repeated) continue;

        /* Il coefficiente g_j, e gli esponenti degli altri gruppi. */
        for (int k = 0; k < q.count && alone; k++) {
            if (mono_cmp(&wave[k], &wave[j]) == 0) {
                g = poly_add(g, poly_term(term_without(q.term[k], &wave[k])), 0);
            } else {
                alone = condition_holds(NEVER_ZERO, poly_add(angle[j], angle[k], 1), base, &where) == 1;
            }
        }
        if (failure) return -1;
        if (alone && condition_holds(NEVER_ZERO, g, base, &where) == 1) return 1;
    }
    return -1;
}

/* 13c-1. IL SEGNO DI UN SENO O DI UN COSENO
 *
 * Seni e coseni sono calcolati come esponenziali complessi (capitolo 11), e
 * in quella forma il segno non si vede. Lo si ritrova riconoscendo le due
 * forme più semplici:
 *
 *     c e^{i t} + c e^{-i t}      = 2 c cos t
 *     -i c e^{i t} + i c e^{-i t} = 2 c sin t
 *
 * eventualmente moltiplicate per un fattore positivo, come 2^n. Il segno di
 * cos t e di sin t dipende dal quadrante: dall'intero m per cui
 *
 *     m pi/2 < t < (m + 1) pi/2.
 *
 * Con m = 0 sono positivi entrambi, con m = 1 solo il seno, con m = 2
 * nessuno dei due, con m = 3 solo il coseno, e poi si ricomincia.
 *
 * Quando l'angolo dipende da n il quadrante deve essere lo stesso per ogni
 * n: le due disuguaglianze sono a loro volta condizioni "positivo per ogni
 * n", decise da condition_holds(). Se l'angolo è un multiplo di pi greco,
 * t = pi r, si chiede m < 2 r < m + 1, senza approssimazioni. Altrimenti
 * servono dei numeri razionali tra cui stia pi greco, e si usano quelli di
 * Archimede, 223/71 < pi < 22/7: per m positivo basta t > m (22/7)/2 da una
 * parte e t < (m + 1) (223/71)/2 dall'altra, e per m negativo i due limiti
 * si scambiano. Sono condizioni un po' più strette del necessario: un angolo
 * molto vicino a un multiplo di pi/2 resta senza risposta. */

/* Cerca il quadrante dell'angolo `angle` per ogni n >= base: ritorna 1 e
 * mette in *quadrant l'intero m descritto sopra, se lo trova tra -8 e 8;
 * altrimenti ritorna 0. Se in_pi è diverso da zero l'angolo vero è pi greco
 * per `angle`. */
static int angle_quadrant(Poly angle, int in_pi, long long base, int *quadrant) {
    const Frac below = {223, 71}, above = {22, 7};
    long long where;
    for (int step = 0; step <= 16 && !failure; step++) {
        /* I candidati nell'ordine 0, 1, -1, 2, -2, ... */
        int m = step % 2 ? (step + 1) / 2 : -(step / 2);
        Frac low = frac_int(m), high = frac_int(m + 1);
        if (!in_pi) {
            low = frac_mul(low, frac_div(m >= 0 ? above : below, frac_int(2)));
            high = frac_mul(high, frac_div(m + 1 > 0 ? below : above, frac_int(2)));
        }

        /* scaled è 2 r per un multiplo di pi greco, l'angolo stesso altrimenti. */
        Poly scaled = in_pi ? poly_mul(angle, poly_const(num_int(2))) : angle;
        if (condition_holds(ALWAYS_POSITIVE, poly_add(scaled, poly_const(num_frac(low)), 1), base, &where) != 1) continue;
        if (condition_holds(ALWAYS_POSITIVE, poly_add(poly_const(num_frac(high)), scaled, 1), base, &where) != 1) continue;
        *quadrant = m;
        return 1;
    }
    return 0;
}

/* Ritorna il segno di p per ogni n >= base, +1 o -1, quando p è una delle
 * forme descritte sopra; 0 se non lo è o se il quadrante non si trova. Mette
 * in *strict 1 se il segno è stretto, 0 se p può anche valere zero.
 *
 * Oltre alle due forme con seno e coseno ne riconosce una terza, con un
 * termine costante: c0 + c e^{i t} + (coniugato di c) e^{-i t}, che vale
 * c0 + 2 Re(c e^{i t}) e sta quindi tra c0 - 2|c| e c0 + 2|c|. Se |c0| supera
 * 2|c| ha il segno di c0; se lo uguaglia lo ha in senso largo. È il caso di
 * cos^2 t = 1/2 + (e^{2 i t} + e^{-2 i t})/4, che nasce come denominatore di
 * una tangente al quadrato. */
static int trig_sign(Poly p, long long base, int *strict) {
    Mono wave[3];
    Poly angle[3];
    Term rest[3];
    int still = -1, first = -1, second = -1, quadrant;
    if (p.count < 2 || p.count > 3) return 0;

    /* Due termini con esponenziali opposti, e al più uno senza. */
    for (int i = 0; i < p.count; i++) {
        if (!split_wave(&p.term[i].mono, &wave[i], &angle[i])) return 0;
        rest[i] = term_without(p.term[i], &wave[i]);
        if (wave[i].count == 0 && still < 0) still = i;
        else if (first < 0) first = i;
        else second = i;
    }
    if (second < 0 || wave[first].count == 0 || wave[second].count == 0 || (p.count == 3 && still < 0)) return 0;
    if (mono_mul(&wave[first], &wave[second]).count != 0) return 0;

    /* Lo stesso fattore reale in tutti i termini, positivo; i coefficienti
     * dei due esponenziali coniugati tra loro. */
    Num c = rest[first].coef, d = rest[second].coef;
    if (mono_cmp(&rest[first].mono, &rest[second].mono) != 0 || frac_cmp(c.re, d.re) != 0 || frac_cmp(c.im, frac_neg(d.im)) != 0) return 0;
    for (int f = 0; f < rest[first].mono.count; f++)
        if (!atom_is_positive(rest[first].mono.factor[f].atom, base)) return 0;

    /* Con il termine costante: il confronto tra c0^2 e 4 |c|^2. */
    if (p.count == 3) {
        Num c0 = rest[still].coef;
        if (mono_cmp(&rest[still].mono, &rest[first].mono) != 0 || c0.im.num != 0) return 0;
        Frac size = frac_mul(frac_int(4), frac_add(frac_mul(c.re, c.re), frac_mul(c.im, c.im)));
        int compare = frac_cmp(frac_mul(c0.re, c0.re), size);
        if (failure || compare < 0) return 0;
        *strict = compare > 0;
        return c0.re.num > 0 ? 1 : -1;
    }

    /* L'angolo è un multiplo di pi greco se pi compare, alla prima potenza,
     * in tutti i suoi termini; se compare solo in alcuni si rinuncia. */
    int pi = atom_var(VAR_PI), with = 0;
    Poly turn = angle[first];
    for (int i = 0; i < turn.count; i++) with += frac_cmp(mono_exponent(&turn.term[i].mono, pi), frac_int(1)) == 0;
    if (with != 0 && with != turn.count) return 0;
    if (with) {
        Term divisor;
        divisor.coef = num_int(1);
        divisor.mono = mono_atom(pi, frac_int(-1));
        turn = poly_mul(turn, poly_term(divisor));
    }
    if (failure || !angle_quadrant(turn, with != 0, base, &quadrant)) return 0;

    /* Coefficiente reale: coseno. Immaginario: seno, con il segno di -Im c,
     * perché -i c' e^{i t} + i c' e^{-i t} = 2 c' sin t. */
    int q = ((quadrant % 4) + 4) % 4;
    *strict = 1;
    if (c.im.num == 0) return (c.re.num > 0 ? 1 : -1) * (q == 0 || q == 3 ? 1 : -1);
    if (c.re.num == 0) return (c.im.num < 0 ? 1 : -1) * (q == 0 || q == 1 ? 1 : -1);
    return 0;
}

/* 13c-7. IL SEGNO PER DIFFERENZE
 *
 * Per le espressioni fatte di n e di potenze come 2^n e 3^n, dove più
 * quantità crescono a velocità diverse: 2^n - n, 3^n - 2^n - n.
 *
 * Lemma. Sia b > 0. Se q(N) > 0 e q(n+1) - b q(n) >= 0 per ogni n >= N,
 * allora q(n) > 0 per ogni n >= N: per induzione, q(n+1) >= b q(n) > 0. Lo
 * stesso vale con >= al posto di >.
 *
 * La differenza q(n+1) - b q(n) è un'espressione dello stesso tipo, e più
 * semplice: se b è la più piccola delle basi che compaiono in q (1 per i
 * termini che sono polinomi in n), i termini con quella base perdono un
 * grado, perché b^{n+1} (n+1)^d - b b^n n^d = b^{n+1} ((n+1)^d - n^d), e
 * dopo abbastanza passi spariscono. Si ripete finché resta un'espressione di
 * segno evidente. Per 2^n - n: la differenza con b = 1 è 2^n - 1, la
 * successiva 2^n, che è positiva; e i valori in n = 1 sono 1 e 1, positivi.
 *
 * Il lemma chiede che ogni differenza parta bene in N. Se non succede per
 * N = base si prova più avanti, controllando uno per uno i valori saltati:
 * così 2^n - 100 n - 1000, negativo all'inizio e poi sempre positivo, risulta
 * mai nullo, e di 2^n - n^2 si trova che vale zero per n = 2. */

/* Ritorna 1 se q è fatto solo di n e di potenze p^n di numeri primi, con
 * esponenti interi non negativi e coefficienti reali. */
static int in_growth_class(Poly q) {
    for (int i = 0; i < q.count; i++) {
        if (q.term[i].coef.im.num != 0) return 0;
        for (int f = 0; f < q.term[i].mono.count; f++) {
            const Factor *factor = &q.term[i].mono.factor[f];
            const Atom *a = &atoms[factor->atom];
            int power = a->kind == ATOM_POW && a->base == BASE_PRIME && a->exponent.count == 1 &&
                        a->exponent.factor[0].atom == atom_var(induction_var) && frac_cmp(a->exponent.factor[0].exp, frac_int(1)) == 0;
            int letter = a->kind == ATOM_VAR && a->id == induction_var;
            if ((!power && !letter) || factor->exp.den != 1 || factor->exp.num < 0 || factor->exp.num > MAX_UNIVARIATE) return 0;
        }
    }
    return 1;
}

/* Ritorna q(n+1) scritto con gli stessi atomi, per q della classe di
 * in_growth_class(): n diventa n + 1 e ogni p^n diventa p p^n. */
static Poly poly_shift(Poly q) {
    int n = atom_var(induction_var);
    Poly out = poly_substitute(q, n, rat_add(rat_atom(n), rat_int(1), 0)).num;
    for (int x = 0; x < atom_count && !failure; x++) {
        if (atoms[x].kind != ATOM_POW || atoms[x].base != BASE_PRIME || atoms[x].exponent.count == 0) continue;
        out = poly_substitute(out, x, rat_mul(rat_int(atoms[x].id), rat_atom(x))).num;
    }
    return out;
}

/* Mette in *value il valore di q per n = at, e ritorna 1; ritorna 0 se non è
 * un razionale che si sa calcolare (vedi mono_at()). */
static int poly_at(Poly q, long long at, Frac *value) {
    Frac v;
    *value = frac_int(0);
    for (int i = 0; i < q.count; i++) {
        if (q.term[i].coef.im.num != 0 || !mono_at(&q.term[i].mono, at, &v)) return 0;
        *value = frac_add(*value, frac_mul(q.term[i].coef.re, v));
    }
    return !failure;
}

/* Ritorna 1 se il lemma qui sopra dimostra che q(n) > 0 (strict) o q(n) >= 0
 * per ogni n >= from; 0 se non ci riesce. depth limita il numero di
 * differenze successive. */
static int grows_positive(Poly q, long long from, int strict, int depth) {
    Frac value;
    if (q.count == 0) return !strict;
    if (!poly_at(q, from, &value) || value.num < 0 || (strict && value.num == 0)) return 0;
    int evident = evident_sign(q, from);
    if (evident || depth == 0) return evident > 0;

    /* La base più piccola tra i termini: il prodotto dei primi di ciascuno. */
    long long smallest = 0;
    for (int i = 0; i < q.count; i++) {
        long long own = 1;
        for (int f = 0; f < q.term[i].mono.count; f++) {
            const Atom *a = &atoms[q.term[i].mono.factor[f].atom];
            for (long long e = a->kind == ATOM_POW ? q.term[i].mono.factor[f].exp.num : 0; e > 0; e--)
                if (__builtin_mul_overflow(own, a->id, &own)) return 0;
        }
        if (!smallest || own < smallest) smallest = own;
    }
    Poly next = poly_add(poly_shift(q), poly_mul(q, poly_const(num_int(smallest))), 1);
    return !failure && grows_positive(next, from, 0, depth - 1);
}

/* Decide con il lemma se q, della classe di in_growth_class(), rispetta kind
 * per ogni n >= base: 1, 0 (con il primo n in cui fallisce in *where) o -1. */
static int difference_rule(Requirement kind, Poly q, long long base, long long *where) {
    Poly minus = poly_add(poly_const(num_int(0)), q, 1);
    if (!in_growth_class(q)) return -1;
    for (long long from = base; from <= base + 64; from++) {
        /* Il valore in `from`, calcolato: i precedenti sono già stati visti. */
        Frac value;
        if (!poly_at(q, from, &value)) return -1;
        if (kind == NEVER_ZERO ? value.num == 0 : kind == ALWAYS_POSITIVE ? value.num <= 0 : value.num < 0) {
            *where = from;
            return 0;
        }

        /* Da qui in poi il segno non cambia più? */
        if (grows_positive(q, from, kind != NEVER_NEGATIVE, 40)) return 1;
        if (kind == NEVER_ZERO && grows_positive(minus, from, 1, 40)) return 1;
        if (failure) return -1;
    }
    return -1;
}

/* Decide se il polinomio negli atomi p rispetta kind per ogni intero
 * n >= base: ritorna 1 se sì, 0 se no (e mette in *where un n in cui
 * fallisce), -1 se non lo sa stabilire. Le regole sono descritte in testa al
 * paragrafo 13c. */
static int condition_holds(Requirement kind, Poly p, long long base, long long *where) {
    long long c[MAX_UNIVARIATE + 1], scale;
    *where = base;

    /* Lo zero, anche quando lo è per un'identità che solo il test di
     * annullamento riconosce: dividere per arctan(1/2) + arctan(1/3) - pi/4
     * è dividere per zero. */
    if (poly_vanishes(p)) return kind == NEVER_NEGATIVE;
    if (failure) return -1;

    /* 13c-1. Un seno o un coseno: il segno viene dal quadrante. Un segno in
     * senso largo basta per "non negativo"; per "positivo" bisogna anche che
     * p non si annulli, e lo dicono le altre regole. */
    int strict = 0, trig = trig_sign(p, base, &strict);
    if (failure) return -1;
    *where = base;
    if (trig && strict) return kind == NEVER_ZERO || trig > 0;
    if (trig > 0 && kind == NEVER_NEGATIVE) return 1;
    if (trig > 0 && kind == ALWAYS_POSITIVE) return condition_holds(NEVER_ZERO, p, base, where) == 1 ? 1 : -1;
    if (trig < 0 && kind == ALWAYS_POSITIVE) return 0;

    /* 13c-2. Il monomio comune. */
    Mono common = poly_common(p);
    for (int i = 0; i < common.count; i++) {
        int atom = common.factor[i].atom;
        int fine = kind == NEVER_ZERO ? atom_never_zero(atom, base, where) : (atom_is_positive(atom, base) ? 1 : -1);
        if (fine != 1) return fine;
    }
    Term divisor;
    divisor.coef = num_int(1);
    divisor.mono = mono_pow(&common, frac_int(-1));
    Poly q = poly_mul(p, poly_term(divisor));
    if (failure) return -1;
    *where = base;

    /* 13c-3. Un polinomio nella sola variabile principale: il segno si
     * decide. */
    int atom = main_atom(q, base);
    int degree = atom >= 0 ? poly_coefficients(q, atom, c, &scale) : -1;
    if (degree >= 0 && kind != NEVER_ZERO) {
        int holds = sign_holds(c, degree, kind == ALWAYS_POSITIVE, atom, base, where);
        if (holds >= 0) return holds;
    }

    /* 13c-4. La variabile principale e radici di primi: gli zeri si trovano. */
    if (atom >= 0 && kind == NEVER_ZERO) {
        int free = zero_free(q, atom, base, where);
        if (free >= 0) return free;
    }
    if (failure) return -1;
    *where = base;

    /* 13c-5, 13c-6. Un segno evidente, o garantito dall'andamento. Una
     * espressione sempre negativa non si annulla, ma non è né positiva né
     * non negativa, già per n = base. */
    int sign = evident_sign(q, base);
    if (!sign) sign = monotone_sign(q, base);
    if (failure) return -1;
    if (sign > 0) return 1;
    if (sign < 0) return kind == NEVER_ZERO;

    /* 13c-7. n e potenze come 2^n: il segno per differenze. */
    int grown = difference_rule(kind, q, base, where);
    if (grown >= 0) return grown;
    if (failure) return -1;
    *where = base;

    /* 13c-8. Esponenziali complessi che non si cancellano. */
    return kind == NEVER_ZERO ? exponentials_never_cancel(q, base) : -1;
}

/* Decide una condizione dell'elenco per ogni n >= base: 1, 0 (con un n in cui
 * fallisce in *where) o -1. Prima sul prodotto dei suoi fattori; se così non
 * si decide, e i fattori sono più d'uno, sul segno di ciascuno: il prodotto
 * di fattori che hanno ognuno un segno fisso ha un segno fisso. */
static int condition_decided(const Condition *c, long long base, long long *where) {
    Poly product = c->factor[0];
    for (int f = 1; f < c->count; f++) product = poly_mul(product, c->factor[f]);
    int holds = failure ? -1 : condition_holds(c->kind, product, base, where);
    if (holds >= 0 || c->count == 1 || c->kind == NEVER_ZERO || failure) return holds;

    int sign = 1;
    for (int f = 0; f < c->count; f++) {
        Poly minus = poly_add(poly_const(num_int(0)), c->factor[f], 1);
        if (condition_holds(ALWAYS_POSITIVE, c->factor[f], base, where) == 1) continue;
        if (failure || condition_holds(ALWAYS_POSITIVE, minus, base, where) != 1) return -1;
        sign = -sign;
    }
    *where = base;
    return sign > 0;
}

/* Decide se l'enunciato, con i parametri legati ai valori del punto in uso, è
 * definito per ogni n >= base: ritorna 1 se sì, 0 se no (con un n in cui non
 * lo è in *where e il perché in *why), -1 se non lo sa stabilire. Vedi il
 * paragrafo 13c.
 *
 * Le valutazioni sono tre, e dicono a quale n si riferisce una condizione
 * che fallisce: quelle nate dal calcolo di D(n) a quell'n, quelle nate solo
 * da D(n+1) al successivo, quelle nate solo da D(base) al caso base. La
 * terza serve perché il calcolo con i numeri non controlla tutto: un
 * logaritmo di cos(1) non è un numero, e il suo segno lo decide solo
 * condition_holds(). */
static int defined_for_every(const Statement *statement, long long base, long long *where, const char **why) {
    Rat n = rat_atom(atom_var(induction_var));
    int answer = 1, broken = 0;

    /* Le tre valutazioni, con l'elenco delle condizioni acceso. */
    condition_count = conditions_lost = 0;
    conditions_on = 1;
    difference(statement, n);
    int first = condition_count;
    difference(statement, rat_add(n, rat_int(1), 0));
    int second = condition_count;
    difference(statement, rat_int(base));
    conditions_on = 0;
    if (failure || conditions_lost) answer = -1;
    failure = NULL;

    /* Ogni condizione va decisa. Una che fallisce chiude la questione, ma si
     * guardano tutte, per riferire il più piccolo n in cui qualcosa manca. */
    for (int i = 0; i < condition_count; i++) {
        long long at;
        int holds = condition_decided(&conditions[i], base, &at);
        if (failure) holds = -1;
        failure = NULL;
        if (holds < 0) answer = -1;
        if (holds != 0) continue;
        at = i >= second ? base : at + (i >= first);
        if (!broken || at < *where) { *where = at; *why = conditions[i].why; }
        broken = 1;
    }
    return broken ? 0 : answer;
}

/* Ritorna la posizione nel vocabolario della successione di cui l'atomo BIGOP
 * `atom` è la definizione, o -1 se è una somma scritta nell'enunciato. */
static int bigop_library(int atom) {
    for (int i = 0; i < LIBRARY_COUNT; i++)
        if (library[i].tree == atoms[atom].node) return i;
    return -1;
}

/* 13d. DEFINITO PER PARAMETRI ABBASTANZA GRANDI
 *
 * Il paragrafo 13c cerca un valore dei parametri che vada bene per ogni n.
 * Non sempre esiste: la somma dei ln(x - k) per k da 1 a n è definita per
 * x > n, e nessun x supera tutti gli n. Eppure per ogni n i valori buoni ci
 * sono: tutti quelli abbastanza grandi. È ciò che serve perché l'enunciato
 * non sia vuoto per nessun n, e qui lo si dimostra così.
 *
 * Un polinomio in x, per x abbastanza grande, ha il segno del suo
 * coefficiente di grado massimo (oltre il limite di Cauchy non ha più
 * radici). Una condizione di esistenza che è un polinomio in x con
 * coefficienti in n vale quindi "da un certo x in poi" se il coefficiente di
 * grado massimo ha il segno giusto: e questo, non contenendo più x, è una
 * condizione in n che condition_holds() sa decidere per ogni n.
 *
 * Le condizioni sono in numero finito per ogni n: i due membri in n e in
 * n + 1, e i termini delle somme, uno per ogni indice fino a n + 1. Ognuna
 * vale oltre una sua soglia, quindi tutte insieme oltre la più grande. Per i
 * termini delle somme la condizione si scrive una volta sola, con l'indice
 * generico "estremo inferiore + t" e t >= 0, come fa body_is_positive().
 *
 * Con più parametri si mandano all'infinito uno dopo l'altro: il primo
 * dell'elenco molto più in fretta del secondo, e così via. Conta allora il
 * coefficiente di grado massimo nel primo parametro, poi in quello il
 * coefficiente di grado massimo nel secondo. Si provano i due ordini
 * estremi, e i due versi: tutti verso più infinito, tutti verso meno
 * infinito (dove una potenza dispari cambia segno). */

/* I parametri mandati all'infinito, dal più veloce al più lento, e il verso:
 * +1 verso più infinito, -1 verso meno infinito. Vuoto fuori da
 * defined_eventually(). */
static int far_name[MAX_POINT];
static int far_count;
static int far_sign;

/* Ritorna 1 se la lettera name è uno dei parametri mandati all'infinito. */
static int is_far(long long name) {
    for (int i = 0; i < far_count; i++)
        if (far_name[i] == name) return 1;
    return 0;
}

/* Ritorna 1 se nel sotto-albero node compare, libero, uno di quei parametri.
 * shadow elenca gli indici di somma che nascondono una lettera con lo stesso
 * nome, come in uses_bound_variable(). */
static int node_has_parameter(const Node *node, const int *shadow, int shadows) {
    if (!node) return 0;
    if (node->kind == N_VAR) {
        for (int i = 0; i < shadows; i++)
            if (shadow[i] == node->name) return 0;
        return is_far(node->name);
    }
    if (node->kind == N_BIGOP) {
        int inner[MAX_ENV + 1];
        if (shadows >= MAX_ENV || node_has_parameter(node->a, shadow, shadows) || node_has_parameter(node->b, shadow, shadows)) return 1;
        memcpy(inner, shadow, (size_t)shadows * sizeof(int));
        inner[shadows] = node->name;
        return node_has_parameter(node->c, inner, shadows + 1);
    }
    return node_has_parameter(node->a, shadow, shadows) || node_has_parameter(node->b, shadow, shadows) ||
           node_has_parameter(node->c, shadow, shadows);
}

static int poly_has_parameter(Poly p);

/* Ritorna 1 se il valore dell'atomo dipende da uno dei parametri mandati
 * all'infinito, anche di nascosto: nel termine di una somma, nella base di
 * una potenza o di un logaritmo, nell'argomento di un'arcotangente. */
static int atom_has_parameter(int atom) {
    const Atom *a = &atoms[atom];
    if (a->kind == ATOM_VAR) return is_far(a->id);
    if (a->kind == ATOM_REC) return 0;
    if (a->kind == ATOM_BIGOP) return bigop_library(atom) < 0 && node_has_parameter(a->node, NULL, 0);
    if (a->kind == ATOM_ATAN) return poly_has_parameter(atan_args[a->id].num) || poly_has_parameter(atan_args[a->id].den);
    for (int f = 0; f < a->exponent.count; f++)
        if (atom_has_parameter(a->exponent.factor[f].atom)) return 1;
    if (a->base == BASE_ATOM) return atom_has_parameter((int)a->id);
    return a->base == BASE_POLY && poly_has_parameter(base_polys[a->id]);
}

/* Ritorna 1 se p contiene un atomo che dipende da uno di quei parametri. */
static int poly_has_parameter(Poly p) {
    for (int i = 0; i < p.count; i++)
        for (int f = 0; f < p.term[i].mono.count; f++)
            if (atom_has_parameter(p.term[i].mono.factor[f].atom)) return 1;
    return 0;
}

static int eventual_sign(Poly p, long long base);

/* Ritorna 1 se il prodotto dell'atomo BIGOP `atom` ha tutti i fattori
 * positivi per parametri abbastanza grandi: il corpo, valutato nell'indice
 * generico, ha numeratore e denominatore dello stesso segno. Allora per ogni
 * n il prodotto, che ha un numero finito di fattori, è positivo da un certo
 * punto in poi. Un errore durante la prova non riguarda l'enunciato. */
static int product_far_positive(int atom, long long base) {
    const Node *node = atoms[atom].node;
    const char *saved = failure;
    long long lower;
    int positive = 0;
    env_count = 0;
    library_depth = 0;
    if (node->op && rat_integer(eval(node->a), &lower)) {
        Rat value = eval_body(node, rat_add(rat_int(lower), rat_atom(atom_var(VAR_SLACK)), 0));
        int above = failure ? 0 : eventual_sign(value.num, base), below = failure ? 0 : eventual_sign(value.den, base);
        positive = above * below == 1;
    }
    failure = saved;
    return positive;
}

/* Ritorna il segno che p finisce per avere, per ogni n >= base, quando i
 * parametri sono abbastanza grandi nel verso e nell'ordine in uso: +1 o -1;
 * 2 se si sa solo che non si annulla; 0 se non si sa dire nulla.
 *
 * p si scrive come monomio comune per il resto. Nel monomio comune ogni
 * atomo deve avere un segno noto: i parametri stessi, gli atomi senza
 * parametri di cui 13c conosce il segno, i prodotti di fattori positivi, le
 * potenze di una base positiva. Il resto deve essere un polinomio nei
 * parametri: conta il suo coefficiente di grado massimo, che non li contiene
 * più. */
static int eventual_sign(Poly p, long long base) {
    long long where;
    if (p.count == 0) return 0;

    /* Senza parametri: una condizione in n, da decidere per ogni n. */
    if (!poly_has_parameter(p)) {
        if (condition_holds(ALWAYS_POSITIVE, p, base, &where) == 1) return 1;
        if (condition_holds(ALWAYS_POSITIVE, poly_add(poly_const(num_int(0)), p, 1), base, &where) == 1) return -1;
        return condition_holds(NEVER_ZERO, p, base, &where) == 1 ? 2 : 0;
    }

    /* Il monomio comune. */
    int sign = 1, known = 1;
    Mono common = poly_common(p);
    for (int i = 0; i < common.count; i++) {
        int atom = common.factor[i].atom;
        const Atom *a = &atoms[atom];
        Frac e = common.factor[i].exp;
        if (a->kind == ATOM_VAR && is_far(a->id)) {
            if (far_sign < 0 && e.den != 1) return 0;
            if (far_sign < 0 && e.num % 2) sign = -sign;
        } else if (!atom_has_parameter(atom)) {
            if (atom_is_positive(atom, base)) continue;
            if (atom_never_zero(atom, base, &where) != 1) return 0;
            known = 0;
        } else if (a->kind == ATOM_POW && a->base == BASE_EI) {
            known = 0;
        } else if (a->kind == ATOM_BIGOP) {
            if (!product_far_positive(atom, base)) return 0;
        } else if (a->kind != ATOM_POW || a->base != BASE_POLY || eventual_sign(base_polys[a->id], base) != 1) {
            return 0;
        }
    }
    Term divisor;
    divisor.coef = num_int(1);
    divisor.mono = mono_pow(&common, frac_int(-1));
    Poly lead = poly_mul(p, poly_term(divisor));

    /* Il coefficiente di grado massimo, un parametro dopo l'altro. */
    for (int i = 0; i < far_count && !failure; i++) {
        Poly part[MAX_UNIVARIATE + 1];
        int degree = poly_split(lead, atom_var(far_name[i]), part, MAX_UNIVARIATE);
        if (degree < 0) return 0;
        if (far_sign < 0 && degree % 2) sign = -sign;
        lead = part[degree];
    }
    if (failure || poly_has_parameter(lead)) return 0;
    int tail = eventual_sign(lead, base);
    if (tail == 0) return 0;
    return tail == 2 || !known ? 2 : sign * tail;
}

/* Ritorna 1 se per ogni n >= base l'enunciato è definito per tutti i valori
 * dei parametri abbastanza grandi, in uno dei versi e degli ordini provati;
 * -1 se non riesce a stabilirlo. I parametri sono quelli di point_name[].
 * Vedi il paragrafo 13d. */
static int defined_eventually(const Statement *statement, long long base) {
    Rat n = rat_atom(atom_var(induction_var));
    int count = point_count, known = atom_count, found = 0;

    /* Le condizioni, con i parametri come lettere: i due membri in n e in
     * n + 1, e il termine generico di ogni somma e prodotto. */
    point_count = 0;
    condition_count = conditions_lost = 0;
    conditions_on = 1;
    failure = NULL;
    difference(statement, n);
    difference(statement, rat_add(n, rat_int(1), 0));
    for (int x = 0; x < known && !failure; x++) {
        long long lower;
        if (atoms[x].kind != ATOM_BIGOP) continue;
        env_count = 0;
        library_depth = 0;
        if (!rat_integer(eval(atoms[x].node->a), &lower)) { conditions_lost = 1; break; }
        eval_body(atoms[x].node, rat_add(rat_int(lower), rat_atom(atom_var(VAR_SLACK)), 0));
    }
    conditions_on = 0;
    point_count = count;
    int lost = failure || conditions_lost;
    failure = NULL;

    /* I due ordini e i due versi. Ogni condizione chiede il segno del
     * prodotto dei suoi fattori. */
    far_count = count;
    for (int attempt = 0; attempt < (count > 1 ? 4 : 2) && !found && !lost; attempt++) {
        far_sign = attempt % 2 ? -1 : 1;
        for (int i = 0; i < count; i++) far_name[i] = point_name[attempt < 2 ? i : count - 1 - i];
        found = 1;
        for (int i = 0; i < condition_count && found; i++) {
            const Condition *c = &conditions[i];
            int sign = 1;
            for (int f = 0; f < c->count && found; f++) {
                int own = eventual_sign(c->factor[f], base);
                if (failure || own == 0 || (own == 2 && c->kind != NEVER_ZERO)) found = 0;
                if (own == -1) sign = -sign;
                failure = NULL;
            }
            if (c->kind != NEVER_ZERO && sign < 0) found = 0;
        }
    }
    far_count = 0;
    return found ? 1 : -1;
}

/* I numeri semplici che la verifica numerica dà ai parametri: positivi e
 * negativi, interi e frazioni, grandi e piccoli. */
static const Frac point_samples[] = {{2, 1}, {3, 1}, {1, 2}, {5, 1}, {1, 3}, {-2, 1}, {3, 2}, {7, 1},
                                     {-1, 2}, {4, 1}, {-3, 1}, {1000, 1}, {1, 1000}};
enum { POINT_SAMPLES = sizeof point_samples / sizeof *point_samples };

/* Ciò che un conto di prova lascia dietro di sé e che va tolto quando il
 * conto non serve più: gli atomi, le basi e le arcotangenti nati durante il
 * conto, e la memoria presa dall'arena. */
typedef struct {
    int atoms, polys, atans;  /* quanti ce n'erano prima del conto */
    size_t arena;             /* byte dell'arena occupati prima del conto */
} Scratch;

/* Ritorna lo stato di adesso, da ridare a scratch_drop() dopo il conto. */
static Scratch scratch_mark(void) { return (Scratch){atom_count, base_poly_count, atan_count, arena_used}; }

/* Toglie tutto ciò che è nato dopo mark. */
static void scratch_drop(Scratch mark) {
    atom_count = mark.atoms;
    base_poly_count = mark.polys;
    atan_count = mark.atans;
    arena_used = mark.arena;
}

/* I VALORI DEI PARAMETRI SCELTI GUARDANDO LE CONDIZIONI
 *
 * La verifica numerica (13e) prova per i parametri pochi numeri semplici. Un
 * enunciato definito solo per valori particolari, come la radice di
 * x - 5000, non ne incontrerebbe mai uno buono. Quando i numeri semplici non
 * bastano si guarda che cosa l'enunciato chiede: le condizioni di esistenza
 * raccolte con i parametri lasciati come lettere sono, nei casi comuni,
 * polinomi nei parametri. Per ogni parametro, uno alla volta, le condizioni
 * che contengono solo lui dividono la retta in intervalli, separati dalle
 * radici reali dei polinomi: il segno di ogni polinomio è costante dentro un
 * intervallo, quindi basta provare un punto per intervallo.
 *
 * Le radici si cercano in virgola mobile, il solo punto del programma in cui
 * non si usa l'aritmetica esatta. Non è un rischio: servono solo a proporre
 * dei candidati. Ogni candidato viene poi controllato con conti esatti, qui
 * e nella verifica vera e propria. Un errore di arrotondamento può far
 * perdere un candidato buono, mai farne accettare uno cattivo. */

/* Mette in root[], in ordine crescente, le radici reali del polinomio
 * c[0] + c[1] x + ... + c[degree] x^degree, approssimate, e ne ritorna il
 * numero. root deve avere almeno max(degree, 1) posti.
 *
 * Tra due zeri consecutivi della derivata il polinomio è monotono, quindi ha
 * al più una radice, e c'è esattamente quando il segno agli estremi è
 * diverso: la si trova per bisezione. Gli zeri della derivata, che ha un
 * grado in meno, si trovano allo stesso modo, fino al grado 1. Agli estremi
 * della retta fa da confine il limite di Cauchy (vedi sign_holds()). */
static int real_roots(const double *c, int degree, double *root) {
    double derivative[MAX_UNIVARIATE + 1], turn[MAX_UNIVARIATE + 1], bound = 0;
    while (degree > 0 && c[degree] == 0) degree--;
    if (degree < 1) return 0;
    if (degree == 1) { root[0] = -c[0] / c[1]; return 1; }
    for (int i = 0; i < degree; i++) {
        double ratio = c[i] / c[degree];
        if (ratio < 0) ratio = -ratio;
        if (ratio > bound) bound = ratio;
        derivative[i] = (i + 1) * c[i + 1];
    }
    bound += 1;
    int turns = real_roots(derivative, degree - 1, turn), count = 0;

    /* Gli intervalli di monotonia: da -bound al primo zero della derivata, e
     * così via fino a bound. */
    double left = -bound;
    for (int k = 0; k <= turns; k++) {
        double right = k < turns ? turn[k] : bound, at[2] = {left, right}, value[2];
        for (int s = 0; s < 2; s++) {
            value[s] = 0;
            for (int i = degree; i >= 0; i--) value[s] = value[s] * at[s] + c[i];
        }
        if (value[1] == 0) {
            root[count++] = right;
        } else if (value[0] != 0 && (value[0] < 0) != (value[1] < 0)) {
            /* Bisezione: il segno in `low` resta quello di value[0]. */
            double low = left, high = right;
            for (int step = 0; step < 200; step++) {
                double middle = (low + high) / 2, v = 0;
                for (int i = degree; i >= 0; i--) v = v * middle + c[i];
                if ((v < 0) == (value[0] < 0)) low = middle; else high = middle;
            }
            root[count++] = (low + high) / 2;
        }
        left = right;
    }
    return count;
}

/* Ritorna il segno, -1, 0 o 1, del polinomio p nel punto in cui l'atomo
 * `atom` vale y^root, con y razionale: un termine c atom^{a/b} vi vale
 * c y^{a root / b}. Chi chiama garantisce che in p compare solo quell'atomo,
 * con esponenti non negativi il cui denominatore divide root. Il conto è
 * esatto, su interi a 128 bit; se non bastano mette 0 in *known, che chi
 * chiama ha messo a 1.
 *
 * Con y = u/v e K il più grande degli esponenti interi, moltiplicando per
 * v^K e per il minimo comune multiplo L dei denominatori dei coefficienti
 * ogni termine diventa un intero, e il segno non cambia perché v e L sono
 * positivi. */
static int power_sum_sign(Poly p, int atom, long long root, Frac y, int *known) {
    long long top = 0;
    __int128 scale = 1, total = 0;
    for (int i = 0; i < p.count; i++) {
        Frac e = mono_exponent(&p.term[i].mono, atom);
        long long den = p.term[i].coef.re.den;
        if (e.num * (root / e.den) > top) top = e.num * (root / e.den);
        scale = scale / gcd128(scale, den) * den;
        if (scale > COEF_LIMIT) { *known = 0; return 0; }
    }
    for (int i = 0; i < p.count; i++) {
        Frac e = mono_exponent(&p.term[i].mono, atom);
        long long power = e.num * (root / e.den);
        __int128 term = (__int128)p.term[i].coef.re.num * (scale / p.term[i].coef.re.den);
        for (long long k = 0; k < top; k++)
            if (__builtin_mul_overflow(term, (__int128)(k < power ? y.num : y.den), &term)) { *known = 0; return 0; }
        if (__builtin_add_overflow(total, term, &total)) { *known = 0; return 0; }
    }
    return (total > 0) - (total < 0);
}

/* Sceglie per i parametri dell'enunciato dei valori che rispettano le sue
 * condizioni di esistenza, e li mette in point_value[]. Ritorna 1 se ci
 * riesce, 0 se non trova nulla da proporre. Chi chiama deve poi verificare
 * il punto: qui si guardano solo le condizioni che si sanno leggere,
 * calcolate nel primo e nell'ultimo n della verifica numerica.
 *
 * I parametri si fissano uno alla volta. Fissato un valore, lo si sostituisce
 * nelle condizioni, così quelle che legano due parametri diventano
 * condizioni sul secondo.
 *
 * Per il parametro x si sanno leggere le condizioni che sono polinomi in una
 * sola quantità T, anche con esponenti frazionari. Di solito T è x stesso.
 * Ma se l'enunciato contiene la radice di un'espressione di primo grado,
 * come x - 5000, T è quell'espressione: la condizione
 * "radice(x - 5000) > 100" si legge "T^{1/2} > 100". Con gli esponenti
 * frazionari si pone T = y^d, dove d è il minimo comune multiplo dei loro
 * denominatori: le condizioni diventano polinomi in y, e scegliendo y
 * razionale le radici di T sono numeri esatti, che la verifica sa calcolare.
 * Da y si risale a T e poi a x. */
static int choose_point(const Statement *statement, long long base) {
    int count = point_count, useful = 0;

    /* Le condizioni, con i parametri come lettere. Un errore ferma solo la
     * raccolta: si usa ciò che è stato annotato fino a lì. Di una condizione
     * su più fattori conta il prodotto. */
    point_count = 0;
    condition_count = conditions_lost = 0;
    conditions_on = 1;
    failure = NULL;
    difference(statement, rat_int(base));
    failure = NULL;
    difference(statement, rat_int(base + SEARCH_SPAN));
    conditions_on = 0;
    undefined = 0;
    point_count = count;
    for (int i = 0; i < condition_count; i++) {
        for (int f = 1; f < conditions[i].count; f++) conditions[i].factor[0] = poly_mul(conditions[i].factor[0], conditions[i].factor[f]);
        conditions[i].count = 1;
    }
    if (failure) condition_count = 0;
    failure = NULL;

    for (int j = 0; j < count; j++) {
        int x = atom_var(point_name[j]), carrier = x, roots = 0, candidates = 0, chosen = -1;
        long long c[MAX_UNIVARIATE + 1], scale, root = 1;
        Frac slope = frac_int(1), shift = frac_int(0);
        Poly view[MAX_CONDITIONS];
        int usable[MAX_CONDITIONS];
        double real[MAX_CONDITIONS * MAX_UNIVARIATE];
        Frac candidate[POINT_SAMPLES + MAX_CONDITIONS * MAX_UNIVARIATE + 1];

        /* La quantità T: la base di una radice, se è di primo grado in x. */
        for (int i = 0; i < condition_count && carrier == x; i++)
            for (int t = 0; t < conditions[i].factor[0].count && carrier == x; t++)
                for (int f = 0; f < conditions[i].factor[0].term[t].mono.count; f++) {
                    int atom = conditions[i].factor[0].term[t].mono.factor[f].atom;
                    const Atom *a = &atoms[atom];
                    if (a->kind != ATOM_POW || a->base != BASE_POLY || a->exponent.count != 0) continue;
                    if (poly_coefficients(base_polys[a->id], x, c, &scale) != 1) continue;
                    carrier = atom;
                    slope = frac_make(c[1], scale);
                    shift = frac_make(c[0], scale);
                    break;
                }

        /* Le condizioni scritte in T, e quelle che vi si sanno leggere: solo
         * T, con esponenti non negativi. d è il loro denominatore comune. */
        for (int i = 0; i < condition_count; i++) {
            Rat back = rat_div(rat_add(rat_atom(carrier), rat_frac(shift), 1), rat_frac(slope));
            view[i] = carrier == x ? conditions[i].factor[0] : poly_substitute(conditions[i].factor[0], x, back).num;
            usable[i] = !failure;
            failure = NULL;
            for (int t = 0; t < view[i].count && usable[i]; t++) {
                const Term *term = &view[i].term[t];
                Frac e = mono_exponent(&term->mono, carrier);
                usable[i] = term->coef.im.num == 0 && term->mono.count == (e.num != 0) && e.num >= 0 && e.den <= 12;
            }
            for (int t = 0; t < view[i].count && usable[i]; t++) {
                long long den = mono_exponent(&view[i].term[t].mono, carrier).den;
                root = root / (long long)gcd128(root, den) * den;
            }
        }

        /* Le radici reali, in y, dei polinomi a cui si riducono. */
        for (int i = 0; i < condition_count; i++) {
            double coefficient[MAX_UNIVARIATE + 1] = {0};
            int degree = 0;
            for (int t = 0; t < view[i].count && usable[i]; t++) {
                Frac e = mono_exponent(&view[i].term[t].mono, carrier), value = view[i].term[t].coef.re;
                long long power = e.num * (root / e.den);
                if (power > MAX_UNIVARIATE) { usable[i] = 0; break; }
                coefficient[power] += (double)value.num / (double)value.den;
                if (power > degree) degree = (int)power;
            }
            if (usable[i] && degree >= 1) {
                roots += real_roots(coefficient, degree, real + roots);
                useful = 1;
            }
        }

        /* In ordine crescente, con un inserimento alla volta. */
        for (int a = 1; a < roots; a++)
            for (int b = a; b > 0 && real[b - 1] > real[b]; b--) {
                double swap = real[b];
                real[b] = real[b - 1];
                real[b - 1] = swap;
            }

        /* I candidati per y: prima i numeri semplici, poi un numero per ogni
         * intervallo tra due radici (il più semplice: un intero se c'è, se
         * no un multiplo di 1/2, di 1/4, ...) e uno oltre ciascun estremo. */
        for (int k = 0; k < POINT_SAMPLES; k++) candidate[candidates++] = point_samples[k];
        for (int k = 0; k <= roots && roots > 0; k++) {
            double low = k > 0 ? real[k - 1] : real[0] - 2, high = k < roots ? real[k] : real[roots - 1] + 2;
            if (low < -1e12 || high > 1e12) continue;
            for (long long den = 1; den <= 1048576; den *= 2) {
                long long num = (long long)(low * (double)den);
                while ((double)num <= low * (double)den) num++;
                if ((double)num < high * (double)den) { candidate[candidates++] = frac_make(num, den); break; }
            }
        }

        /* Il primo candidato che rispetta, con un conto esatto, tutte le
         * condizioni lette. Con T = y^d e d > 1, y è una radice: positivo. */
        for (int k = 0; k < candidates && chosen < 0; k++) {
            int fine = root == 1 || candidate[k].num > 0;
            for (int i = 0; i < condition_count && fine; i++) {
                int known = 1, sign = usable[i] ? power_sum_sign(view[i], carrier, root, candidate[k], &known) : 1;
                Requirement kind = conditions[i].kind;
                fine = known && (kind == NEVER_ZERO ? sign != 0 : kind == ALWAYS_POSITIVE ? sign > 0 : sign >= 0);
            }
            if (fine) chosen = k;
        }
        if (chosen < 0) return 0;

        /* Da y a T = y^d, e da T a x. */
        Frac value = frac_int(1);
        for (long long k = 0; k < root && !failure; k++) value = frac_mul(value, candidate[chosen]);
        point_value[j] = frac_div(frac_add(value, frac_neg(shift)), slope);

        /* Il valore scelto entra nelle condizioni che restano; quelle in cui
         * non si riesce a sostituirlo non contano più. */
        for (int i = 0; i < condition_count && !failure; i++) {
            Poly next = poly_substitute(conditions[i].factor[0], x, rat_frac(point_value[j])).num;
            if (failure) { conditions[i].kind = NEVER_ZERO; next = poly_const(num_int(1)); }
            failure = NULL;
            conditions[i].factor[0] = next;
        }
        if (failure) { failure = NULL; return 0; }
    }
    return useful;
}

/* 13e. Verifica indipendente di un enunciato già dimostrato, a partire dal
 * caso base `base`. Ritorna NULL se conferma la dimostrazione, altrimenti il
 * motivo, in italiano, per cui il verdetto non va emesso.
 *
 * La dimostrazione è algebra sulle lettere: vale "dove i due membri sono
 * definiti", e non dice se quel dove esiste. ln(-x) = ln(x) passa, perché i
 * logaritmi sono trattati come ln|x|, ma non c'è un solo x in cui tutti e due
 * i membri abbiano senso. Qui si fa ciò che l'algebra non fa: si danno ai
 * parametri dei valori numerici e si calcolano davvero i due membri per
 * n = base, base + 1, ..., base + SEARCH_SPAN. Con i numeri al posto delle
 * lettere i controlli della valutazione (capitolo 11) scattano: logaritmo di
 * un numero non positivo, radice di un numero negativo, divisione per zero.
 * Poi, se `every` è diverso da zero, si decide se nello stesso punto i due
 * membri sono definiti anche per tutti gli n successivi (13c); e se non lo
 * sono, se per ogni n lo sono per tutti i valori dei parametri abbastanza
 * grandi (13d).
 *
 * Serve un punto in cui tutto è definito per ogni n. I valori provati per i
 * parametri sono pochi numeri semplici, positivi e negativi, grandi e
 * piccoli, combinati in modi diversi a ogni tentativo; se nessuno va bene,
 * un ultimo tentativo usa valori scelti guardando le condizioni di esistenza
 * (choose_point()). Si rinuncia, e il verdetto diventa NON DIMOSTRATO, in
 * quattro casi:
 *
 *   - in nessun punto provato il caso base è definito: l'enunciato sembra
 *     vuoto;
 *   - in ogni punto provato c'è un n in cui un'espressione non è definita:
 *     l'enunciato non può valere "per ogni n";
 *   - c'è un punto che i numeri non smentiscono, ma non si sa stabilire che
 *     regga per tutti gli n, o i numeri sono troppo grandi per calcolarlo;
 *   - in un punto i due membri sono numeri diversi: sarebbe un errore del
 *     programma, e la dimostrazione non va creduta.
 *
 * Un conto che si ferma per un limite del programma, come i numeri troppo
 * grandi, non dice nulla: da lì in poi quel punto non viene più calcolato,
 * senza obiezioni. Ma se si ferma già al primo valore il punto non ha
 * confermato niente, e non conta.
 *
 * `every` è zero quando si ricontrolla una formula scritta in una
 * dimostrazione (13g): lì conta solo che i numeri tornino.
 *
 * I conti fatti qui non servono dopo: atomi e memoria tornano com'erano. */
static const char *verify_instances(const Statement *statement, long long base, int every) {
    static char text[200];
    Scratch mark = scratch_mark();
    const char *problem = NULL;
    long long reached = base - 1;  /* l'ultimo n fin dove un tentativo è arrivato */
    const char *why = NULL;        /* che cosa non è definito in quell'n */
    int undecided = 0;             /* 1 se un punto non è smentito, ma nemmeno confermato per ogni n */
    int far = 0;                   /* esito di defined_eventually(), 0 finché non è stato chiesto */

    /* I parametri: le lettere dell'enunciato diverse dalla variabile di
     * induzione. Pi greco è un numero, non un parametro. */
    point_count = 0;
    for (int i = 0; i < mark.atoms && point_count < MAX_POINT; i++)
        if (atoms[i].kind == ATOM_VAR && atoms[i].id != induction_var && atoms[i].id != VAR_PI && atoms[i].id != VAR_SLACK)
            point_name[point_count++] = (int)atoms[i].id;

    /* Senza parametri c'è un solo punto da provare. Con i parametri, dopo i
     * tentativi con i numeri semplici ce n'è uno con valori scelti apposta. */
    int attempts = point_count ? 4 * POINT_SAMPLES : 1, confirmed = 0;
    for (int attempt = 0; attempt <= attempts && !confirmed && !problem; attempt++) {
        if (attempt == attempts) {
            int chosen = point_count && choose_point(statement, base);
            scratch_drop(mark);
            if (!chosen) break;
        } else {
            /* Valori diversi per parametri diversi, e combinazioni diverse a
             * ogni giro dell'elenco. */
            for (int j = 0; j < point_count; j++)
                point_value[j] = point_samples[(attempt * (2 * j + 1) + (attempt / POINT_SAMPLES) * j + j) % POINT_SAMPLES];
        }

        confirmed = 1;
        for (long long c = base; c <= base + SEARCH_SPAN && confirmed && !problem; c++) {
            Num value;
            failure = NULL;
            undefined = 0;
            Rat d = difference(statement, rat_int(c));
            int limit = failure && !undefined;
            if (failure && undefined) {
                confirmed = 0;
                if (c > reached) { reached = c; why = failure; }
            } else if (limit && c == base) {
                /* Nemmeno il primo valore si è potuto calcolare: questo punto
                 * non ha confermato nulla. Prenderlo per buono farebbe
                 * passare x^100 + ln(-x) = x^100 + ln(x), dove i numeri
                 * diventano troppo grandi prima di arrivare al logaritmo. */
                confirmed = 0;
                undecided = 1;
            } else if (!failure && rat_number(d, &value) && !num_is_zero(value)) {
                snprintf(text, sizeof text, "la verifica numerica non conferma la dimostrazione: per %c = %lld i due membri sono diversi",
                         induction_var, c);
                problem = text;
            }
            scratch_drop(mark);
            if (limit) break;
        }

        /* 13c. Il punto regge per i primi n: regge per tutti? */
        if (confirmed && every && !problem) {
            long long where = base;
            const char *broken = NULL;
            failure = NULL;
            int answer = defined_for_every(statement, base, &where, &broken);
            scratch_drop(mark);

            /* 13d. Questo valore non va bene per ogni n: ma forse per ogni n
             * vanno bene tutti i valori abbastanza grandi. Lo si chiede una
             * volta sola, perché non dipende dal punto. */
            if (answer != 1 && point_count && !far) {
                far = defined_eventually(statement, base);
                scratch_drop(mark);
            }
            if (answer != 1 && far != 1) confirmed = 0;
            if (answer < 0) undecided = 1;
            if (answer == 0 && where > reached) { reached = where; why = broken; }
        }
    }
    if (!confirmed && !problem) {
        if (undecided) {
            snprintf(text, sizeof text, "non riesco a stabilire che le espressioni dell'enunciato siano definite per ogni %c", induction_var);
            problem = text;
        } else if (reached <= base && point_count) {
            problem = "non trovo valori dei parametri per cui tutte le espressioni dell'enunciato sono definite";
        } else if (why == REWRITTEN) {
            snprintf(text, sizeof text, "per %c = %lld %s", induction_var, reached, why);
            problem = text;
        } else {
            snprintf(text, sizeof text, "per %c = %lld l'enunciato non è definito: %s", induction_var, reached, why);
            problem = text;
        }
    }
    failure = NULL;
    undefined = 0;
    point_count = 0;
    return problem;
}

/* 13f. LA DIMOSTRAZIONE SCRITTA
 *
 * Il testo ripercorre ciò che il programma ha fatto, con le espressioni vere:
 *
 *   - caso base: i due membri con il numero al posto di n, e il loro valore;
 *   - ipotesi induttiva: l'enunciato per n;
 *   - passo: l'enunciato per n + 1; per ogni somma o prodotto, i termini che
 *     si aggiungono passando da n a n + 1; il valore che l'ipotesi dà alla
 *     somma ricavata; e ciò che i due membri diventano dopo la sostituzione.
 *
 * Servono due modi di scrivere un'espressione. put_node() riscrive un albero
 * com'è nell'enunciato, eventualmente con una variabile sostituita: serve per
 * "il membro sinistro per n + 1" e per "il termine con k = n + 1". put_rat()
 * scrive invece una frazione di polinomi negli atomi, cioè la forma a cui la
 * valutazione riduce ogni espressione: serve per i valori e per i risultati
 * dei conti. Un'espressione troppo lunga non viene scritta, e il testo dice
 * solo che il conto è stato fatto. */

/* Scrive il numero v in LaTeX. */
static void put_frac(Frac v) {
    if (v.den == 1) proof_add("%lld", v.num);
    else proof_add("%s\\frac{%lld}{%lld}", v.num < 0 ? "-" : "", llabs(v.num), v.den);
}

/* La sostituzione in corso mentre put_node() scrive: la variabile shown_name
 * va scritta come l'espressione shown_value. Con shown_value NULL non si
 * sostituisce nulla. */
static int shown_name;
static const Node *shown_value;

/* Ritorna quanto strettamente il nodo tiene insieme i suoi operandi, per
 * decidere le parentesi: 1 somme e differenze, 2 somme e prodotti scritti con
 * \sum e \prod, 3 prodotti, 4 potenze e fattoriali, 5 ciò che è già un blocco
 * unico.
 *
 * Il livello 2 esiste perché un \sum prende come corpo tutto il prodotto che
 * lo segue (capitolo 9): come addendo non vuole parentesi, come fattore sì,
 * altrimenti inghiottirebbe i fattori scritti dopo. */
static int node_precedence(const Node *n) {
    switch (n->kind) {
    case N_ADD: case N_SUB: case N_NEG: return 1;
    case N_BIGOP: return 2;
    case N_MUL: return 3;
    case N_POW: return 4;
    case N_CALL: return n->op == FN_FACT ? 4 : 5;
    case N_NUM: return n->value < 0 ? 1 : 5;
    default: return 5;
    }
}

/* Scrive in LaTeX l'espressione dell'albero n, applicando la sostituzione in
 * corso. need è la precedenza minima che il contesto accetta senza
 * parentesi: un nodo che lega meno viene messo tra parentesi.
 *
 * Warning: se aggiungi un tipo di nodo o una funzione, aggiungine il caso
 * anche qui; ciò che questa funzione scrive deve rileggersi, con il parser
 * del capitolo 9, come l'espressione di partenza. */
static void put_node(const Node *n, int need) {
    static const char *const call_name[] = {[FN_SIN] = "sin", [FN_COS] = "cos", [FN_TAN] = "tan", [FN_COT] = "cot",
                                            [FN_ARCTAN] = "arctan", [FN_LN] = "ln"};

    /* La variabile sostituita. Dentro l'espressione che la sostituisce non
     * si sostituisce di nuovo. */
    if (n->kind == N_VAR && shown_value && n->name == shown_name) {
        const Node *value = shown_value;
        shown_value = NULL;
        put_node(value, need);
        shown_value = value;
        return;
    }

    int wrap = node_precedence(n) < need;
    if (wrap) proof_add("\\left(");
    switch (n->kind) {
    case N_NUM:
        proof_add("%lld", n->value);
        break;
    case N_VAR:
        if (n->name == VAR_PI) proof_add("\\pi"); else proof_add("%c", n->name);
        break;
    case N_SEQ:
        proof_add("%c_{", n->name);
        put_node(n->a, 0);
        proof_add("}");
        break;
    case N_NEG:
        proof_add("-");
        put_node(n->a, 3);
        break;
    case N_ADD: case N_SUB:
        put_node(n->a, 1);
        proof_add(n->kind == N_ADD ? " + " : " - ");
        put_node(n->b, 2);
        break;
    case N_MUL: {
        /* Un numero davanti a una lettera si scrive accostato, 2n; ma non se
         * al posto della lettera va scritto un numero, perché 2 e 1
         * accostati si leggerebbero 21. */
        int replaced = shown_value && n->b->kind == N_VAR && n->b->name == shown_name;
        int juxtaposed = n->a->kind == N_NUM && n->b->kind == N_VAR && !(replaced && shown_value->kind == N_NUM);
        put_node(n->a, 3);
        if (!juxtaposed) proof_add(" \\cdot ");
        put_node(n->b, 4);
        break;
    }
    case N_DIV:
        proof_add("\\frac{");
        put_node(n->a, 0);
        proof_add("}{");
        put_node(n->b, 0);
        proof_add("}");
        break;
    case N_POW:
        /* La potenza di una funzione si scrive \cos^{2}(x), come la legge il
         * parser. */
        if (n->a->kind == N_CALL && n->a->op >= FN_SIN && n->a->op <= FN_LN) {
            proof_add("\\%s^{", call_name[n->a->op]);
            put_node(n->b, 0);
            proof_add("}\\left(");
            put_node(n->a->a, 0);
            proof_add("\\right)");
            break;
        }
        put_node(n->a, 5);
        proof_add("^{");
        put_node(n->b, 0);
        proof_add("}");
        break;
    case N_BIGOP: {
        /* L'indice nasconde, nel corpo, una variabile con lo stesso nome. */
        const Node *kept = shown_value;
        proof_add("\\%s_{%c=", n->op ? "prod" : "sum", n->name);
        put_node(n->a, 0);
        proof_add("}^{");
        put_node(n->b, 0);
        proof_add("} ");
        if (n->name == shown_name) shown_value = NULL;
        put_node(n->c, 3);
        shown_value = kept;
        break;
    }
    case N_CALL:
        if (n->op == FN_FACT) {
            put_node(n->a, 5);
            proof_add("!");
        } else if (n->op == FN_BINOM) {
            proof_add("\\binom{");
            put_node(n->a, 0);
            proof_add("}{");
            put_node(n->b, 0);
            proof_add("}");
        } else if (n->op == FN_ROOT) {
            proof_add("\\sqrt");
            if (n->b->kind != N_NUM || n->b->value != 2) { proof_add("["); put_node(n->b, 0); proof_add("]"); }
            proof_add("{");
            put_node(n->a, 0);
            proof_add("}");
        } else if (n->op == FN_ABS) {
            proof_add("\\left|");
            put_node(n->a, 0);
            proof_add("\\right|");
        } else {
            if (n->op == FN_LOG) { proof_add("\\log_{"); put_node(n->b, 0); proof_add("}"); }
            else proof_add("\\%s", call_name[n->op]);
            proof_add("\\left(");
            put_node(n->a, 0);
            proof_add("\\right)");
        }
        break;
    case N_APPLY:
        proof_add("%c\\left(", n->name);
        put_node(n->a, 0);
        proof_add("\\right)");
        break;
    }
    if (wrap) proof_add("\\right)");
}

/* Ritorna l'albero dell'espressione a * variable + c, con a positivo, scritta
 * nel modo più breve: n, n + 1, 2 n - 1. */
static Node *node_affine(long long a, int variable, long long c) {
    Node *n = node_new(N_VAR, NULL, NULL);
    n->name = variable;
    if (a != 1) n = node_new(N_MUL, node_num(a), n);
    if (c > 0) return node_new(N_ADD, n, node_num(c));
    if (c < 0) return node_new(N_SUB, n, node_num(-c));
    return n;
}

/* Prototipi in avanti necessari: un atomo può contenere una frazione (le
 * arcotangenti), un polinomio (le basi) o un monomio (gli esponenti), e
 * questi sono fatti di atomi. */
static void put_rat(Rat r);
static void put_terms(Poly p);
static void put_mono(const Mono *m);

/* Diventa 1 quando put_rat() incontra una quantità che il LaTeX degli
 * enunciati non sa dire: un numero immaginario, o una potenza di e^i che non
 * sia una potenza di -1. Seni e coseni sono calcolati con gli esponenziali
 * complessi, ma in quella forma non si possono né scrivere in modo leggibile
 * né rileggere: chi chiama rinuncia a scrivere l'espressione. */
static int unwritable;

/* Scrive la somma o il prodotto dell'atomo BIGOP `atom` con `shift` termini
 * in più: l'estremo superiore è dilation * n + offset + shift. Le successioni
 * del vocabolario si scrivono con il loro nome: n!, H_{n}. Un atomo è una
 * funzione di n così com'è: la sostituzione in corso non lo riguarda. */
static void put_bigop(int atom, long long shift) {
    const Atom *a = &atoms[atom];
    const Node *upper = node_affine(a->dilation, induction_var, a->offset + shift), *kept = shown_value;
    int named = bigop_library(atom);
    shown_value = NULL;
    if (named >= 0 && library[named].name == '!') {
        put_node(upper, 5);
        proof_add("!");
    } else if (named >= 0) {
        proof_add("%c_{", library[named].name);
        put_node(upper, 0);
        proof_add("}");
    } else {
        proof_add("\\%s_{%c=", a->node->op ? "prod" : "sum", a->node->name);
        put_node(a->node->a, 0);
        proof_add("}^{");
        put_node(upper, 0);
        proof_add("} ");
        put_node(a->node->c, 3);
    }
    shown_value = kept;
}

/* Scrive l'atomo elevato a exp, diverso da zero. Ogni tipo di atomo si scrive
 * come la quantità che rappresenta (vedi il capitolo 4), nel LaTeX che il
 * parser del capitolo 9 sa rileggere:
 *
 *   VAR    la lettera;
 *   BIGOP  la somma o il prodotto, tra parentesi, o il nome nel vocabolario;
 *   REC    il termine della successione, come F_{n + 1};
 *   POW    base^{esponente}: l'esponente dell'atomo e exp si moltiplicano;
 *          l'esponente 1/2 su una base presa come lettera è una radice;
 *          e^{i pi M} con M intero è (-1)^M (vedi 4g);
 *   LN     \ln del valore assoluto della base;
 *   ATAN   \arctan dell'argomento.
 *
 * Warning: se aggiungi un tipo di atomo, aggiungine il caso qui. */
static void put_atom(int atom, Frac exp) {
    const Atom *a = &atoms[atom];
    int one = exp.num == 1 && exp.den == 1;
    switch (a->kind) {
    case ATOM_VAR:
        if (a->id == VAR_SLACK) unwritable = 1;
        else if (a->id == VAR_PI) proof_add("\\pi");
        else proof_add("%c", (int)a->id);
        break;
    case ATOM_REC:
        proof_add("%c_{", library[a->id].name);
        put_node(node_affine(a->dilation, induction_var, a->offset), 0);
        proof_add("}");
        break;
    case ATOM_BIGOP: {
        int named = bigop_library(atom) >= 0;
        if (!named) proof_add("\\left(");
        put_bigop(atom, 0);
        if (!named) proof_add("\\right)");
        break;
    }
    case ATOM_POW: {
        /* Le potenze di e^i: si scrive solo (-1)^M, togliendo il pi greco
         * dall'esponente. */
        if (a->base == BASE_EI) {
            if (a->pi != PI_INTEGER || !one) { unwritable = 1; return; }
            proof_add("\\left(-1\\right)^{");
            for (int i = 0, written = 0; i < a->exponent.count; i++) {
                const Factor *f = &a->exponent.factor[i];
                if (atoms[f->atom].kind == ATOM_VAR && atoms[f->atom].id == VAR_PI) continue;
                if (written++) proof_add("\\,");
                put_atom(f->atom, f->exp);
            }
            proof_add("}");
            return;
        }

        /* La base: un numero o una lettera da soli, altrimenti tra parentesi;
         * sotto radice le parentesi non servono. */
        int root = a->exponent.count == 0 && exp.num == 1 && exp.den == 2;
        int simple = a->base == BASE_PRIME || (a->base == BASE_ATOM && atoms[a->id].kind == ATOM_VAR);
        proof_add(root ? "\\sqrt{" : simple ? "" : "\\left(");
        if (a->base == BASE_PRIME) proof_add("%lld", a->id);
        else if (a->base == BASE_ATOM) put_atom((int)a->id, frac_int(1));
        else put_terms(base_polys[a->id]);
        proof_add(root ? "}" : simple ? "" : "\\right)");
        if (root || (a->exponent.count == 0 && one)) return;

        /* L'esponente: exp per il monomio dell'atomo. */
        proof_add("^{");
        if (a->exponent.count == 0) {
            put_frac(exp);
        } else {
            if (exp.num == -1 && exp.den == 1) proof_add("-");
            else if (!one) { put_frac(exp); proof_add("\\,"); }
            put_mono(&a->exponent);
        }
        proof_add("}");
        return;
    }
    case ATOM_LN:
        /* Senza parentesi \ln 2^{3} sarebbe il logaritmo di 2^3. */
        proof_add(one ? "\\ln " : "\\left(\\ln ");
        if (a->base == BASE_PRIME) {
            proof_add("%lld", a->id);
        } else {
            proof_add("\\left|");
            if (a->base == BASE_ATOM) put_atom((int)a->id, frac_int(1)); else put_terms(base_polys[a->id]);
            proof_add("\\right|");
        }
        if (!one) proof_add("\\right)");
        break;
    case ATOM_ATAN:
        proof_add("\\arctan\\left(");
        put_rat(atan_args[a->id]);
        proof_add("\\right)");
        break;
    }
    if (!one) { proof_add("^{"); put_frac(exp); proof_add("}"); }
}

/* Scrive il monomio m: i suoi fattori, separati da uno spazio sottile. */
static void put_mono(const Mono *m) {
    for (int i = 0; i < m->count; i++) {
        if (i) proof_add("\\,");
        put_atom(m->factor[i].atom, m->factor[i].exp);
    }
}

/* Scrive il polinomio negli atomi p, un termine dopo l'altro: segno, valore
 * assoluto del coefficiente (taciuto se è 1 davanti a degli atomi), atomi. */
static void put_terms(Poly p) {
    if (p.count == 0) { proof_add("0"); return; }
    for (int i = 0; i < p.count; i++) {
        const Term *t = &p.term[i];
        if (t->coef.im.num != 0) { unwritable = 1; return; }
        Frac size = t->coef.re;
        int negative = size.num < 0, bare = t->mono.count == 0, unit;
        if (negative) size = frac_neg(size);
        unit = size.num == 1 && size.den == 1;
        proof_add("%s", i ? (negative ? " - " : " + ") : (negative ? "-" : ""));
        if (!unit || bare) put_frac(size);
        if (bare) continue;

        /* Tra il coefficiente e una potenza di un numero ci vuole il punto:
         * 2 e 3^n accostati si leggerebbero 23^n. */
        const Factor *first = &t->mono.factor[0];
        const Atom *a = &atoms[first->atom];
        int root = a->exponent.count == 0 && first->exp.num == 1 && first->exp.den == 2;
        int digit = a->kind == ATOM_POW && a->base == BASE_PRIME && !root;
        if (!unit) proof_add(digit ? " \\cdot " : "\\,");
        put_mono(&t->mono);
    }
}

/* Scrive la frazione di polinomi r; solo il numeratore se il denominatore è
 * 1. Come per put_bigop(), la sostituzione in corso non riguarda gli atomi. */
static void put_rat(Rat r) {
    const Node *kept = shown_value;
    shown_value = NULL;
    if (poly_is_one(r.den)) {
        put_terms(r.num);
    } else {
        proof_add("\\frac{");
        put_terms(r.num);
        proof_add("}{");
        put_terms(r.den);
        proof_add("}");
    }
    shown_value = kept;
}

/* Un pezzo del testo della dimostrazione in corso: i caratteri [from, to) di
 * proof_text. È una formula appena scritta, che si vuole poter rileggere. */
typedef struct {
    size_t from, to;
} Span;

/* Toglie dal testo della dimostrazione ciò che segue i primi length
 * caratteri. */
static void proof_cut(size_t length) {
    proof_length = length;
    proof_text[length] = 0;
}

/* Scrive il membro `side` dell'enunciato con la variabile di induzione
 * sostituita da value (NULL: così com'è) e ritorna il pezzo scritto. */
static Span put_side(const Node *side, const Node *value) {
    Span span = {proof_length, 0};
    shown_name = induction_var;
    shown_value = value;
    put_node(side, 0);
    shown_value = NULL;
    span.to = proof_length;
    return span;
}

/* Scrive prefix e poi il valore r, e ritorna 1 con il pezzo del valore in
 * *span. Se r non si può scrivere, perché troppo lungo, o non reale, o
 * perché un conto è fallito, non scrive nulla e ritorna 0. */
static int put_value(const char *prefix, Rat r, Span *span) {
    size_t start = proof_length;
    if (failure || r.num.count + r.den.count > 14) return 0;
    proof_add("%s", prefix);
    span->from = proof_length;
    unwritable = 0;
    put_rat(r);
    span->to = proof_length;
    if (!unwritable) return 1;
    proof_cut(start);
    return 0;
}

/* 13g. IL CONTROLLO DELLE FORMULE SCRITTE
 *
 * Ritorna 1 se l'uguaglianza tra le due formule scritte nei pezzi a e b della
 * dimostrazione è confermata per un'altra strada: le due formule vengono
 * rilette dal testo con il parser degli enunciati, come se le avesse scritte
 * l'utente, e l'uguaglianza passa per la verifica numerica 13e a partire da
 * n = base.
 *
 * Chi scrive la formula e chi la controlla non hanno nulla in comune: la
 * scrittura parte dagli alberi e dai polinomi negli atomi, il controllo dai
 * caratteri del testo e dai numeri. Un errore nel modo di scrivere (una
 * parentesi mancante, una sostituzione fatta male, un segno) cambia il valore
 * di ciò che si rilegge, e qui si vede. */
static int reread_equal(Span a, Span b, long long base) {
    char text[MAX_FORMULA];
    Statement statement;
    size_t kept = arena_used;
    int length = snprintf(text, sizeof text, "%.*s = %.*s", (int)(a.to - a.from), proof_text + a.from,
                          (int)(b.to - b.from), proof_text + b.from);
    int holds = length > 0 && (size_t)length < sizeof text && parse_statement(text, &statement) && !failure &&
                verify_instances(&statement, base, 0) == NULL;
    arena_used = kept;
    failure = NULL;
    return holds;
}

/* Ritorna 1 se i due pezzi del testo contengono gli stessi caratteri. */
static int span_same(Span a, Span b) {
    return a.to - a.from == b.to - b.from && memcmp(proof_text + a.from, proof_text + b.from, a.to - a.from) == 0;
}

/* Ritorna il valore del membro `side` dell'enunciato con la variabile di
 * induzione legata a n. */
static Rat side_value(const Node *side, Rat n) {
    env_count = 0;
    library_depth = 0;
    env_push(induction_var, n);
    Rat value = eval(side);
    env_count = 0;
    return value;
}

/* Scrive la dimostrazione per induzione dell'enunciato, già verificata: caso
 * base `base`, d = D(n), e `used` l'atomo ricavato dall'ipotesi nel passo
 * (-1 se non è servita), come li ha lasciati step_holds().
 *
 * Ogni formula scritta viene riletta e ricontrollata con reread_equal(). Se
 * anche una sola non regge, tutto il testo è sostituito da uno schema senza
 * formule e unjustified_steps sale: il verdetto resta, perché non dipende dal
 * testo, ma un passaggio non confermato non va stampato.
 *
 * I conti rifatti qui servono solo a mostrare le espressioni: un loro errore,
 * come un'espressione fuori dai limiti, toglie una frase dal testo ma non
 * tocca il verdetto, quindi `failure` torna com'era.
 *
 * Il testo si aggiunge a quello già scritto. Con nested diverso da zero è un
 * passo di una dimostrazione più grande (regola 18n): un \item con i suoi
 * sotto-passi, senza l'intestazione e senza il quadratino finale. */
static void put_induction_proof(const Statement *statement, long long base, Rat d, int used, int nested) {
    int variable = induction_var, sound = 1, valued = 1;
    const char *saved = failure;
    size_t opening = proof_length;
    const Node *side[2] = {statement->lhs, statement->rhs}, *successor = node_affine(1, variable, 1);
    Rat next = rat_add(rat_atom(atom_var(variable)), rat_int(1), 0);
    Span text[2], value[2];

    /* Caso base: i due membri con il numero al posto di n, e il loro valore.
     * Un membro che è già un numero non ha bisogno di "= valore". */
    proof_add(nested ? "\\item By induction on $%c$.\n" : "\\textbf{Proof.} By induction on $%c$.\n", variable);
    proof_add("\\begin{itemize}\n\\item \\emph{Base case}, $%c = %lld$. ", variable, base);
    for (int s = 0; s < 2; s++) {
        Rat low = side_value(side[s], rat_int(base));
        proof_add(s ? " and the right-hand side is $" : "The left-hand side is $");
        text[s] = put_side(side[s], node_num(base));
        if (side[s]->kind == N_NUM || (side[s]->kind == N_VAR && side[s]->name == variable)) value[s] = text[s];
        else if (put_value(" = ", low, &value[s])) sound &= reread_equal(text[s], value[s], base);
        else valued = 0;
        proof_add("$");
    }
    sound &= reread_equal(text[0], text[1], base);
    proof_add(valued && span_same(value[0], value[1]) ? ": the two values are equal.\n"
                  : ": the two sides are equal, because their difference reduces to $0$ by exact algebra.\n");

    /* I due membri per n + 1, come li ha calcolati il passo induttivo. */
    Rat up[2] = {side_value(side[0], next), side_value(side[1], next)};

    if (used < 0) {
        /* Nessuna somma da ricavare: i due membri coincidono per ogni n. */
        proof_add("\\item \\emph{Inductive step}. Here the inductive hypothesis is not needed. For $%c + 1$ the statement is $", variable);
        text[0] = put_side(side[0], successor);
        proof_add(" = ");
        text[1] = put_side(side[1], successor);
        proof_add("$");
        sound &= reread_equal(text[0], text[1], base);
        if (!failure && rat_add(up[0], up[1], 1).num.count == 0 && put_value(", and written out both sides are $", up[0], &value[0])) {
            proof_add("$.\n");
            sound &= reread_equal(text[0], value[0], base) & reread_equal(text[1], value[0], base);
        } else {
            proof_add(", and the difference of the two sides reduces to $0$ by exact algebra.\n");
        }
    } else {
        /* L'ipotesi, e ciò che va dimostrato. */
        proof_add("\\item \\emph{Inductive hypothesis}. Assume the statement for some $%c \\ge %lld$: $", variable, base);
        put_side(side[0], NULL);
        proof_add(" = ");
        put_side(side[1], NULL);
        proof_add("$.\n\\item \\emph{Inductive step}. We prove the statement for $%c + 1$: $", variable);
        text[0] = put_side(side[0], successor);
        proof_add(" = ");
        text[1] = put_side(side[1], successor);
        proof_add("$.\n\\begin{itemize}\n");
        sound &= reread_equal(text[0], text[1], base);

        /* Seni e coseni sono calcolati come esponenziali complessi (capitolo
         * 11): il testo lo dice, perché in quella forma i risultati dei conti
         * non vengono scritti. */
        for (int x = 0, told = 0; x < atom_count && !told; x++)
            if (atoms[x].kind == ATOM_POW && atoms[x].base == BASE_EI && atoms[x].pi == PI_NONE) {
                proof_add("\\item Sines and cosines are computed as complex exponentials, $\\cos\\theta = \\frac{e^{i\\theta} + "
                          "e^{-i\\theta}}{2}$ and $\\sin\\theta = \\frac{e^{i\\theta} - e^{-i\\theta}}{2i}$; expressions in "
                          "that form are not written out below.\n");
                told = 1;
            }

        /* I termini che ogni somma o prodotto guadagna passando a n + 1: gli
         * indici da dilation * n + offset + 1 a dilation * (n + 1) + offset. */
        for (int x = 0; x < atom_count; x++) {
            const Atom *a = &atoms[x];
            if (a->kind != ATOM_BIGOP) continue;
            Span longer = {proof_length + strlen("\\item $"), 0}, split;
            proof_add("\\item $");
            put_bigop(x, a->dilation);
            longer.to = proof_length;
            proof_add(" = ");
            split.from = proof_length;

            /* Un \prod prende come corpo tutti i fattori che lo seguono
             * (capitolo 9): senza parentesi inghiottirebbe quelli nuovi. */
            int wrap = a->node->op && bigop_library(x) < 0;
            if (wrap) proof_add("\\left(");
            put_bigop(x, 0);
            if (wrap) proof_add("\\right)");
            for (long long j = 1; j <= a->dilation; j++) {
                proof_add(a->node->op ? " \\cdot " : " + ");
                shown_name = a->node->name;
                shown_value = node_affine(a->dilation, variable, a->offset + j);
                put_node(a->node->c, a->node->op ? 4 : 2);
                shown_value = NULL;
            }
            split.to = proof_length;
            proof_add("$.\n");
            sound &= reread_equal(longer, split, base);
        }

        /* Il valore che l'ipotesi dà all'atomo ricavato: da A x + B = 0. */
        Poly hypothesis[2];
        Rat solved = rat_int(0);
        int split = poly_split(d.num, used, hypothesis, 1) == 1;
        if (split) solved = rat_make(poly_add(poly_const(num_int(0)), hypothesis[0], 1), hypothesis[1]);
        Span atom = {proof_length + strlen("\\item By the inductive hypothesis, $"), 0};
        size_t start = proof_length;
        proof_add("\\item By the inductive hypothesis, $");
        put_bigop(used, 0);
        atom.to = proof_length;
        if (split && put_value(" = ", solved, &value[0])) {
            proof_add("$.\n");
            sound &= reread_equal(atom, value[0], base);
        } else {
            proof_cut(start);
            proof_add("\\item The inductive hypothesis gives the value of $");
            put_bigop(used, 0);
            proof_add("$.\n");
        }

        /* Ciò che i due membri diventano con quel valore al posto dell'atomo. */
        for (int s = 0; s < 2 && split && !failure; s++)
            up[s] = rat_div(poly_substitute(up[s].num, used, solved), poly_substitute(up[s].den, used, solved));
        char opening[112];
        snprintf(opening, sizeof opening, "\\item With these replacements, the left-hand side for $%c + 1$ becomes $", variable);
        start = proof_length;
        int shown = split && put_value(opening, up[0], &value[0]) && put_value("$ and the right-hand side becomes $", up[1], &value[1]);
        if (shown && span_same(value[0], value[1])) {
            /* Lo stesso testo due volte: lo si dice una volta sola. */
            proof_cut(start);
            snprintf(opening, sizeof opening, "\\item With these replacements, both sides of the statement for $%c + 1$ become $", variable);
            put_value(opening, up[0], &value[0]);
            proof_add("$.\n");
            sound &= reread_equal(text[0], value[0], base) & reread_equal(text[1], value[0], base);
        } else if (shown) {
            proof_add("$. They are equal: their difference, written as one fraction, has numerator $0$.\n");
            sound &= reread_equal(text[0], value[0], base) & reread_equal(text[1], value[1], base);
        } else {
            proof_cut(start);
            proof_add("\\item With these replacements, the difference of the two sides for $%c + 1$ reduces to $0$ by exact algebra.\n", variable);
        }
        proof_add("\\end{itemize}\n");
    }
    proof_add(nested ? "\\end{itemize}\n" : "\\end{itemize}\n$\\Box$");

    /* Un passaggio non confermato: resta solo lo schema, senza formule. */
    if (!sound || proof_length >= PROOF_BYTES - 1) {
        if (!sound) unjustified_steps++;
        proof_cut(opening);
        proof_add(nested ? "\\item " : "\\textbf{Proof.} ");
        proof_add("By induction on $%c$: the base case $%c = %lld$ and the inductive step were both "
                  "checked by exact algebra on the difference of the two sides.%s", variable, variable, base, nested ? "\n" : " $\\Box$");
    }
    failure = saved;
}

/* 13h. IL CERTIFICATO
 *
 * I controlli fatti fin qui sono tutti del motore: stessa aritmetica, stesso
 * modo di leggere le formule. L'ultimo è di un altro programma. Il motore
 * scrive un certificato, cioè l'enunciato, il caso base e un "fattore" W per
 * cui D(n+1) = W D(n), e checker.c lo ricontrolla con i suoi polinomi. Che
 * cosa dimostra un certificato accettato è scritto in testa a checker.c, ed
 * è meno del verdetto: dice che i due membri sono uguali dove sono definiti.
 * Che lo siano per ogni n resta affidato a 13c e 13d.
 *
 * Il fattore. L'ipotesi induttiva è A X + B = 0, e il numeratore di D(n+1) è
 * un polinomio in X, P(X) = C0 + C1 X + ... + Cd X^d. Il passo induttivo
 * (13b) ha controllato che A^d P(-B/A) = 0. Per il teorema del resto vuol
 * dire che A X + B divide A^d P(X):
 *
 *     A^d P(X) = Q(X) (A X + B).
 *
 * Q si trova con la regola di Ruffini. Con Y = A X, A^d P(X) è il polinomio
 * in Y di coefficienti c_j = C_j A^{d-j}, da dividere per Y + B:
 *
 *     s_{d-1} = c_d,    s_{j-1} = c_j - B s_j,    Q = s_0 + s_1 Y + ... + s_{d-1} Y^{d-1},
 *
 * e ciò che avanza, c_0 - B s_0, è proprio la quantità che 13b ha trovato
 * nulla. Con D(n) = (A X + B)/D0 e D(n+1) = P(X)/D1 si ottiene
 *
 *     D(n+1) = W D(n),    W = Q D0 / (A^d D1).
 *
 * Se D(n+1) è zero senza bisogno dell'ipotesi, W = 0.
 *
 * Un divisore di W che non è un divisore dell'enunciato. Quando A dipende
 * solo dai parametri (il secondo caso di 13b) può restare a denominatore di
 * W. Il verificatore allora respinge, a meno che il certificato dichiari A
 * tra le espressioni "generic": il teorema certificato vale dove A non è
 * zero, e l'estensione agli altri valori dei parametri resta l'argomento di
 * 13b, che il certificato non copre. Si prova prima senza.
 *
 * La notazione dei certificati ha meno parole di quella degli enunciati:
 * niente radici, logaritmi, seni. Quando l'enunciato o il fattore contengono
 * qualcosa che non sa dire, il certificato non c'è, e il verdetto lo dichiara.
 */

/* Il certificato dell'enunciato in corso, nella notazione di checker.c. */
static char certificate_text[CERTIFICATE_BYTES];
static size_t certificate_length;

/* Diventa 1 quando il certificato in corso non si può scrivere: una quantità
 * che la notazione non ha, o un testo più lungo di CERTIFICATE_BYTES. */
static int certificate_broken;

/* Le successioni del vocabolario nominate nel certificato in corso: il bit i
 * è la riga i di library[]. Vanno definite nel certificato stesso. */
static unsigned certificate_sequences;
_Static_assert(LIBRARY_COUNT <= 32, "certificate_sequences ha un bit per ogni riga del vocabolario");

/* Perché l'ultimo certificato chiesto a certify() non c'è o non vale, in
 * italiano. Resta valido fino alla chiamata successiva. */
static char certificate_problem[256];

/* Aggiunge un pezzo al certificato, con la sintassi di printf. */
static void cert_add(const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    int written = vsnprintf(certificate_text + certificate_length, CERTIFICATE_BYTES - certificate_length, format, arguments);
    va_end(arguments);
    if (written < 0 || (size_t)written >= CERTIFICATE_BYTES - certificate_length) certificate_broken = 1;
    else certificate_length += (size_t)written;
}

/* Scrive una lettera dell'enunciato. \pi e la lettera ausiliaria di
 * body_is_positive() non sono parametri: la notazione non li ha. */
static void cert_letter(long long name) {
    if (name < 128 && isalpha((int)name)) cert_add("%c", (int)name);
    else certificate_broken = 1;
}

static void cert_node(const Node *n);

/* Scrive "(word a)" oppure, se b non è NULL, "(word a b)". */
static void cert_call(const char *word, const Node *a, const Node *b) {
    cert_add("(%s ", word);
    cert_node(a);
    if (b) {
        cert_add(" ");
        cert_node(b);
    }
    cert_add(")");
}

/* Scrive il termine di indice index della riga `entry` del vocabolario. Il
 * fattoriale è una parola della notazione; le altre successioni si nominano,
 * e certify() ne aggiunge la definizione.
 *
 * Warning: "(fact a)" è per il verificatore il prodotto degli interi da 1 ad
 * a. Se cambi la riga '!' del vocabolario, il certificato non dice più ciò
 * che il motore ha dimostrato: va cambiato anche qui. */
static void cert_term(int entry, const Node *index) {
    if (library[entry].name == '!') {
        cert_call("fact", index, NULL);
        return;
    }
    certificate_sequences |= 1u << entry;
    cert_add("(seq %c ", library[entry].name);
    cert_node(index);
    cert_add(")");
}

/* Scrive la somma o il prodotto del nodo node, con upper al posto del suo
 * estremo superiore. */
static void cert_bigop(const Node *node, const Node *upper) {
    cert_add("(%s ", node->op ? "prod" : "sum");
    cert_letter(node->name);
    cert_add(" ");
    cert_node(node->a);
    cert_add(" ");
    cert_node(upper);
    cert_add(" ");
    cert_node(node->c);
    cert_add(")");
}

/* Scrive l'albero n nella notazione dei certificati: l'operatore davanti, e
 * tutte le parentesi.
 *
 * Warning: se aggiungi un tipo di nodo o una funzione, decidi qui se la
 * notazione dei certificati lo sa dire; altrimenti resta senza certificato. */
static void cert_node(const Node *n) {
    static const char *const binary[] = {[N_ADD] = "+", [N_SUB] = "-", [N_MUL] = "*", [N_DIV] = "/", [N_POW] = "^"};
    switch (n->kind) {
    case N_NUM:
        cert_add("%lld", n->value);
        return;
    case N_VAR:
        cert_letter(n->name);
        return;
    case N_SEQ:
        for (int i = 0; i < LIBRARY_COUNT; i++)
            if (library[i].name == n->name) { cert_term(i, n->a); return; }
        certificate_broken = 1;
        return;
    case N_NEG:
        cert_call("neg", n->a, NULL);
        return;
    case N_BIGOP:
        cert_bigop(n, n->b);
        return;
    case N_CALL:
        if (n->op == FN_FACT) cert_call("fact", n->a, NULL);
        else if (n->op == FN_BINOM) cert_call("binom", n->a, n->b);
        else certificate_broken = 1;
        return;
    case N_APPLY:
        certificate_broken = 1;
        return;
    default:
        cert_call(binary[n->kind], n->a, n->b);
    }
}

static void cert_poly(Poly p);

/* Scrive l'atomo `atom` come la quantità che rappresenta (capitolo 4).
 *
 *   VAR    la lettera;
 *   BIGOP  la somma o il prodotto fino al suo estremo, o il termine della
 *          successione del vocabolario di cui è la definizione;
 *   REC    il termine della successione;
 *   POW    base^M; e^{i pi M} con M intero è (-1)^M (vedi 4g).
 *
 * Logaritmi, arcotangenti e le altre potenze di e^i (seni e coseni) non
 * hanno una scrittura.
 *
 * Warning: se aggiungi un tipo di atomo, aggiungine il caso qui. */
static void cert_atom(int atom) {
    const Atom *a = &atoms[atom];
    switch (a->kind) {
    case ATOM_VAR:
        cert_letter(a->id);
        return;
    case ATOM_REC:
        cert_term((int)a->id, node_affine(a->dilation, induction_var, a->offset));
        return;
    case ATOM_BIGOP: {
        const Node *upper = node_affine(a->dilation, induction_var, a->offset);
        int named = bigop_library(atom);
        if (named >= 0) cert_term(named, upper); else cert_bigop(a->node, upper);
        return;
    }
    case ATOM_POW: {
        /* L'esponente; per (-1)^M è quello dell'atomo senza il pi greco. */
        Mono exponent = a->exponent;
        if (a->base == BASE_EI) {
            if (a->pi != PI_INTEGER) { certificate_broken = 1; return; }
            exponent = mono_with_exponent(&exponent, atom_var(VAR_PI), frac_int(0));
        }

        /* La base, da sola se l'atomo è la base presa come lettera. */
        if (exponent.count) cert_add("(^ ");
        if (a->base == BASE_EI) cert_add("-1");
        else if (a->base == BASE_PRIME) cert_add("%lld", a->id);
        else if (a->base == BASE_ATOM) cert_atom((int)a->id);
        else cert_poly(base_polys[a->id]);
        if (!exponent.count) return;
        cert_add(" ");
        cert_poly(poly_term((Term){num_int(1), exponent}));
        cert_add(")");
        return;
    }
    default:
        certificate_broken = 1;
    }
}

/* Scrive il polinomio negli atomi p: la somma dei suoi termini, ciascuno il
 * coefficiente per i suoi atomi elevati ai loro esponenti. Un coefficiente
 * immaginario o un esponente non intero (una radice) non hanno scrittura. */
static void cert_poly(Poly p) {
    if (p.count == 0) cert_add("0");
    for (int i = 0; i < p.count; i++) {
        const Term *t = &p.term[i];
        Frac c = t->coef.re;
        if (t->coef.im.num != 0) certificate_broken = 1;

        /* Somme e prodotti hanno due operandi: "(+ primo (+ secondo ...))",
         * "(* (* primo secondo) terzo)". Il coefficiente 1 davanti a degli
         * atomi si tace; `shown` conta i fattori già scritti. */
        int shown = c.num != c.den || t->mono.count == 0;
        if (i + 1 < p.count) cert_add("(+ ");
        for (int j = 1; j < shown + t->mono.count; j++) cert_add("(* ");
        if (shown && c.den == 1) cert_add("%lld", c.num);
        else if (shown) cert_add("(/ %lld %lld)", c.num, c.den);
        for (int j = 0; j < t->mono.count; j++) {
            Frac e = t->mono.factor[j].exp;
            if (e.den != 1) certificate_broken = 1;
            if (shown) cert_add(" ");
            if (e.num != 1) cert_add("(^ ");
            cert_atom(t->mono.factor[j].atom);
            if (e.num != 1) cert_add(" %lld)", e.num);
            if (shown++) cert_add(")");
        }
        if (i + 1 < p.count) cert_add(" ");
    }
    for (int i = 1; i < p.count; i++) cert_add(")");
}

/* Scrive il certificato dell'induzione appena riuscita e lo fa controllare
 * dal verificatore indipendente. statement, base, d, d1 e used sono quelli
 * di induct(): l'enunciato, il caso base, D(n), D(n+1) e l'atomo ricavato
 * dall'ipotesi (o -1).
 *
 * Ritorna la risposta del verificatore, e allora il certificato è in
 * certificate_text; oppure CK_UNSUPPORTED se non si è potuto scrivere. Quando
 * la risposta non è CK_ACCEPTED il motivo è in certificate_problem. */
static CkAnswer certify(const Statement *statement, long long base, Rat d, Rat d1, int used) {
    const char *saved = failure, *why = "";
    Poly a = poly_const(num_int(1)), top = poly_const(num_int(0)), bottom = a;
    CkAnswer answer = CK_UNSUPPORTED;

    /* Il fattore W = top/bottom, con la regola di Ruffini. Le due
     * scomposizioni sono quelle già riuscite in step_holds(): qui possono
     * mancare solo se è finita la memoria. */
    Poly hypothesis[2], part[MAX_DEGREE + 1];
    int degree = used < 0 ? -1 : poly_split(d1.num, used, part, MAX_DEGREE);
    if (used >= 0 && (degree < 0 || poly_split(d.num, used, hypothesis, 1) != 1)) fail(OUT_OF_MEMORY);
    if (used >= 0 && !failure) {
        Poly a_power[MAX_DEGREE + 1], y_power[MAX_DEGREE + 1];
        a = hypothesis[1];

        /* Le potenze di A e di Y = A X. */
        a_power[0] = y_power[0] = poly_const(num_int(1));
        for (int j = 1; j <= degree; j++) {
            a_power[j] = poly_mul(a_power[j - 1], a);
            y_power[j] = poly_mul(y_power[j - 1], poly_mul(a, rat_atom(used).num));
        }

        /* s vale s_j quando entra in Q, poi diventa s_{j-1}. */
        Poly s = part[degree], q = top;
        for (int j = degree - 1; j >= 0; j--) {
            q = poly_add(q, poly_mul(s, y_power[j]), 0);
            s = poly_add(poly_mul(part[j], a_power[degree - j]), poly_mul(hypothesis[0], s), 1);
        }
        top = poly_mul(q, d.den);
        bottom = poly_mul(a_power[degree], d1.den);
    }

    /* Numeratore e denominatore proporzionali: W è un numero. Capita con
     * ogni enunciato "somma = formula", dove W = 1. */
    if (!failure && poly_proportional(top, bottom)) {
        top = poly_const(num_mul(top.term[0].coef, num_inv(bottom.term[0].coef)));
        bottom = poly_const(num_int(1));
    }

    /* Prima senza "generic", poi dichiarando A. */
    for (int generic = 0; generic < 2 && !failure; generic++) {
        /* Le voci: variabile, caso base, enunciato, fattore. */
        certificate_length = 0;
        certificate_broken = 0;
        certificate_sequences = 0;
        cert_add("certificate induction\nvariable %c\nbase %lld\nstatement (= ", induction_var, base);
        cert_node(statement->lhs);
        cert_add(" ");
        cert_node(statement->rhs);
        cert_add(poly_is_one(bottom) ? ")\nfactor " : ")\nfactor (/ ");
        cert_poly(top);
        if (!poly_is_one(bottom)) {
            cert_add(" ");
            cert_poly(bottom);
            cert_add(")");
        }
        if (generic) {
            cert_add("\ngeneric ");
            cert_poly(a);
        }

        /* Le successioni nominate. Scrivere una definizione può nominarne
         * un'altra: si continua finché ne restano. */
        for (unsigned written = 0, pending; (pending = certificate_sequences & ~written) != 0;) {
            int i = 0;
            while (!(pending >> i & 1)) i++;
            written |= 1u << i;
            if (library[i].definition) {
                cert_add("\ndefine %c m ", library[i].name);
                cert_node(library[i].tree);
                continue;
            }
            cert_add("\nsequence %c %d", library[i].name, library[i].order);
            for (int j = 0; j < library[i].order; j++) cert_add(" %lld", library[i].coef[j]);
            for (int j = 0; j < library[i].order; j++) cert_add(" %lld", library[i].init[j]);
        }
        cert_add("\nend\n");
        if (certificate_broken) {
            answer = CK_UNSUPPORTED;
            why = "l'enunciato o il passo induttivo usano qualcosa che la notazione dei certificati non ha";
            break;
        }

        /* Il controllo. Dopo un rifiuto si riprova solo se A è un'espressione
         * dei parametri che si può dichiarare diversa da zero. */
        const char *text = certificate_text;
        answer = ck_check(&text, &why);
        if (answer != CK_REJECTED || used < 0 || poly_depends_on_n(a)) break;
    }

    /* Un errore nei conti del fattore (numeri troppo grandi) non riguarda
     * l'enunciato: vuol dire solo che il certificato non c'è. */
    if (failure) {
        answer = CK_UNSUPPORTED;
        why = failure;
    }
    failure = saved;
    snprintf(certificate_problem, sizeof certificate_problem, "%s%s",
             answer == CK_REJECTED ? "il verificatore indipendente ha respinto il certificato: " : "", why);
    return answer;
}

/* Il metodo dell'induzione su un enunciato già analizzato: caso base, passo,
 * verifiche e dimostrazione scritta, come descritto in testa al capitolo.
 *
 * variable è la variabile di induzione; se has_base è diverso da zero il caso
 * base è base, altrimenti lo sceglie il programma: il più piccolo n, almeno 1,
 * per cui le somme e i prodotti dell'enunciato hanno almeno un termine.
 * nested dice come scrivere la dimostrazione (vedi put_induction_proof()).
 *
 * Usa la tabella degli atomi così com'è: chi chiama decide se svuotarla. */
static Verdict induct(const Statement *input, int variable, int has_base, long long base, int nested) {
    Verdict verdict = {UNSUPPORTED, variable, base, 0, NULL, NULL, NULL, NULL, NULL, NULL};
    Statement statement = *input;
    induction_var = variable;
    default_base = 1;
    domain_floor = has_base ? base : 1;
    if (statement.relation != REL_EQ) { verdict.reason = "le disuguaglianze non sono ancora supportate"; return verdict; }

    /* D(n). È la prima valutazione, quindi quella che fa nascere gli atomi con
     * gli estremi scritti nell'enunciato, e che calcola default_base. */
    Rat n = rat_atom(atom_var(variable));
    Rat d = difference(&statement, n);
    if (failure) { verdict.reason = failure; return verdict; }
    if (!has_base) base = default_base;
    verdict.base = base;

    /* 13a. Caso base. */
    Rat d0 = difference(&statement, rat_int(base));
    int base_holds = !failure && poly_vanishes(d0.num);
    if (failure) { verdict.reason = failure; return verdict; }
    if (!base_holds) {
        /* Un numero diverso da zero è una smentita certa. Un'espressione con
         * dei parametri potrebbe invece essere uno zero che l'algebra non sa
         * riconoscere: non si può concludere nulla. */
        Num value;
        if (rat_number(d0, &value)) { verdict.outcome = REFUTED; verdict.witness = base; }
        else { verdict.outcome = NOT_PROVED; verdict.reason = "il caso base non si riduce a zero"; }
        return verdict;
    }

    /* 13b. Passo induttivo. */
    Rat d1 = difference(&statement, rat_add(n, rat_int(1), 0));
    if (failure) { verdict.reason = failure; return verdict; }
    int used;
    int proved = step_holds(d, d1, &used);
    if (failure) { verdict.reason = failure; return verdict; }
    if (proved) {
        /* 13c, 13d, 13e. Verifica indipendente, con i numeri al posto delle lettere. */
        const char *problem = verify_instances(&statement, base, 1);
        if (problem) {
            verdict.outcome = NOT_PROVED;
            verdict.reason = problem;
            return verdict;
        }

        /* 13f, 13g. La dimostrazione scritta, con i suoi passaggi. */
        put_induction_proof(&statement, base, d, used, nested);

        /* 13h. Il certificato. Un rifiuto vuol dire che il motore e il
         * verificatore non sono d'accordo: nel dubbio non è dimostrato, e
         * l'enunciato va guardato come un errore del programma. */
        CkAnswer answer = certify(&statement, base, d, d1, used);
        if (answer == CK_REJECTED) {
            unjustified_steps++;
            verdict.outcome = NOT_PROVED;
            verdict.reason = certificate_problem;
            return verdict;
        }
        if (answer == CK_ACCEPTED) verdict.certificate = certificate_text;
        else verdict.reason = certificate_problem;
        verdict.outcome = PROVED;
        verdict.proof = proof_text;
        return verdict;
    }

    /* Ricerca di un controesempio nei valori successivi al caso base. Un
     * errore qui (per esempio numeri troppo grandi) interrompe solo la
     * ricerca. */
    for (long long c = base + 1; c <= base + SEARCH_SPAN; c++) {
        Num value;
        Rat dc = difference(&statement, rat_int(c));
        if (failure) break;
        if (rat_number(dc, &value) && !num_is_zero(value)) {
            verdict.outcome = REFUTED;
            verdict.witness = c;
            return verdict;
        }
    }
    failure = NULL;
    verdict.outcome = NOT_PROVED;
    verdict.reason = "il passo induttivo non si chiude";
    return verdict;
}

/* Prova a dimostrare per induzione l'enunciato scritto in formula, con la
 * variabile di induzione e il caso base indicati (vedi induct()).
 *
 * Svuota l'arena e la tabella degli atomi: i risultati di chiamate precedenti
 * non sono più validi. */
static Verdict prove_by_induction(const char *formula, int variable, int has_base, long long base) {
    Verdict verdict = {UNSUPPORTED, variable, base, 0, NULL, NULL, NULL, NULL, NULL, NULL};
    Statement statement;

    /* Ripartenza da zero per il nuovo enunciato, e parsing. */
    proof_reset();
    arena_used = arena_keep;
    atom_count = 0;
    base_poly_count = 0;
    atan_count = 0;
    failure = NULL;
    if (!parse_statement(formula, &statement)) { verdict.reason = syntax_error; return verdict; }
    if (failure) { verdict.reason = failure; return verdict; }
    return induct(&statement, variable, has_base, base, 0);
}

/* =============================================================================
 * 14. INSIEMI: NOTAZIONE E ALBERO
 * ============================================================================
 *
 * Seconda famiglia di enunciati: quelli sugli insiemi, per esempio
 *
 *     (S \setminus T) \setminus V = S \setminus (T \cup V)
 *     S \cap T \subseteq V \iff S \subseteq V \cup (S \setminus T)
 *
 * Le lettere maiuscole sono insiemi qualsiasi, e l'enunciato si intende per
 * ogni loro scelta. La grammatica, dalla precedenza più bassa alla più alta:
 *
 *     formula     := implication { \iff implication }
 *     implication := disjunction [ \implies implication ]
 *     disjunction := conjunction { \lor conjunction }
 *     conjunction := negation { \land negation }
 *     negation    := \lnot negation | \exists lettera negation | relation
 *     relation    := union [ ('=' | \subseteq | \supseteq | \neq) union ]
 *     union       := inter { (\cup | \setminus | \triangle) inter }
 *     inter       := product { \cap product }
 *     product     := primary { \times primary }
 *     primary     := '(' formula ')' | lettera maiuscola | \emptyset
 *                  | \mathcal{P} '(' formula ')' | \{ \emptyset \}
 *
 * Le parentesi racchiudono una formula intera, così la stessa regola serve per
 * raggruppare sia espressioni tra insiemi sia condizioni. Che ogni operatore
 * riceva operandi del tipo giusto lo controlla, dopo, set_check().
 *
 * Gli oggetti sono di tre tipi, e non si mescolano:
 *
 *   - insiemi di elementi: le lettere e ciò che se ne ottiene con unione,
 *     intersezione, differenza e differenza simmetrica;
 *   - collezioni di insiemi: l'insieme delle parti \mathcal{P}(A), cioè
 *     l'insieme di tutti i sottoinsiemi di A, e \{\emptyset\};
 *   - insiemi di coppie: il prodotto cartesiano A \times B.
 */

/* Tipi di nodo di un enunciato sugli insiemi.
 *
 * Warning: se aggiungi un tipo, aggiungi il ramo che lo costruisce nel parser
 * di questo capitolo, il suo tipo in set_check(), e il suo case in member() o
 * set_holds() (capitolo 15), in concrete() o concrete_holds() e in put_set()
 * (capitolo 16). */
typedef enum {
    SET_VAR,         /* una lettera: var è il suo indice */
    SET_EMPTY,       /* l'insieme vuoto */
    SET_ONLY_EMPTY,  /* la collezione il cui unico elemento è l'insieme vuoto */
    SET_POWER,       /* insieme delle parti di a */
    SET_UNION, SET_INTER, SET_DIFF, SET_SYMDIFF,  /* a op b */
    SET_PRODUCT,     /* a per b */
    SET_EQ, SET_SUBSET, SET_NEQ,                  /* relazioni tra a e b */
    SET_AND, SET_OR, SET_IMPLIES, SET_IFF,        /* connettivi tra a e b */
    SET_NOT,         /* non a */
    SET_EXISTS       /* esiste un insieme, di indice var, tale che a */
} SetKind;

/* Il tipo di ciò che un nodo denota. TY_ANY è l'insieme vuoto, che può stare
 * al posto di un oggetto di qualunque tipo; TY_ERROR segnala operandi di tipo
 * sbagliato. */
typedef enum { TY_ERROR, TY_ANY, TY_SET, TY_FAMILY, TY_PAIRS, TY_FORMULA } SetType;

/* Un nodo dell'albero di un enunciato sugli insiemi. I nodi vivono nell'arena.
 * support e supports valgono solo per le relazioni e li riempie
 * relation_supports() (capitolo 15). */
typedef struct SetNode SetNode;
struct SetNode {
    SetKind kind;
    SetType type;       /* riempito da set_check() */
    int var;            /* SET_VAR, SET_EXISTS: indice in set_var_name */
    SetNode *a, *b;     /* operandi; NULL se non usati */
    unsigned *support;  /* configurazioni minime in cui la relazione fallisce */
    int supports;       /* quante sono */
};

/* Le lettere usate come insiemi nell'enunciato in corso, in ordine di
 * comparsa: la lettera di indice i è set_var_name[i]. */
static int set_var_name[MAX_SET_VARS];
static int set_var_count;

/* Ritorna l'indice della lettera name, registrandola se è nuova. */
static int set_var_index(int name) {
    for (int i = 0; i < set_var_count; i++)
        if (set_var_name[i] == name) return i;
    if (set_var_count == MAX_SET_VARS) { syntax("troppi insiemi distinti in un enunciato: al massimo 4"); return 0; }
    set_var_name[set_var_count] = name;
    return set_var_count++;
}

/* Ritorna un nuovo nodo di tipo kind con operandi a e b. Come node_new(), se
 * l'arena è piena ritorna un nodo di riserva. */
static SetNode *set_node(SetKind kind, SetNode *a, SetNode *b) {
    static SetNode spare;
    SetNode *node = arena_alloc(sizeof(SetNode));
    if (!node) node = &spare;
    memset(node, 0, sizeof *node);
    node->kind = kind;
    node->a = a;
    node->b = b;
    return node;
}

static SetNode *set_formula(void);

/* primary: vedi la grammatica in testa al capitolo. */
static SetNode *set_primary(void) {
    /* Parentesi. */
    if (token.kind == T_OPEN) {
        next_token();
        SetNode *inner = set_formula();
        if (token.kind == T_CLOSE) next_token(); else syntax("manca la parentesi chiusa");
        return inner;
    }

    /* Una lettera maiuscola è un insieme. */
    if (token.kind == T_LETTER && isupper(token.ch)) {
        SetNode *node = set_node(SET_VAR, NULL, NULL);
        node->var = set_var_index(token.ch);
        next_token();
        return node;
    }

    /* Insieme vuoto. */
    if (is_command("emptyset") || is_command("varnothing")) {
        next_token();
        return set_node(SET_EMPTY, NULL, NULL);
    }

    /* Insieme delle parti: \mathcal{P}( ... ). */
    if (is_command("mathcal")) {
        next_token();
        expect_char('{', "dopo \\mathcal serve {P}");
        if (token.kind != T_LETTER || token.ch != 'P') syntax("l'unico \\mathcal riconosciuto è \\mathcal{P}, l'insieme delle parti");
        next_token();
        expect_char('}', "dopo \\mathcal serve {P}");
        if (token.kind != T_OPEN) { syntax("dopo \\mathcal{P} serve l'argomento tra parentesi"); return set_node(SET_EMPTY, NULL, NULL); }
        next_token();
        SetNode *inner = set_formula();
        if (token.kind == T_CLOSE) next_token(); else syntax("manca la parentesi chiusa");
        return set_node(SET_POWER, inner, NULL);
    }

    /* La collezione \{\emptyset\}. */
    if (is_char(ESCAPED + '{')) {
        next_token();
        if (!is_command("emptyset") && !is_command("varnothing")) syntax("tra \\{ e \\} è riconosciuto solo \\emptyset");
        next_token();
        expect_char(ESCAPED + '}', "manca \\}");
        return set_node(SET_ONLY_EMPTY, NULL, NULL);
    }

    syntax("enunciato sugli insiemi incompleto o simbolo inatteso");
    return set_node(SET_EMPTY, NULL, NULL);
}

/* product := primary { \times primary } */
static SetNode *set_product(void) {
    SetNode *node = set_primary();
    while (is_command("times")) { next_token(); node = set_node(SET_PRODUCT, node, set_primary()); }
    return node;
}

/* inter := product { \cap product } */
static SetNode *set_inter(void) {
    SetNode *node = set_product();
    while (is_command("cap")) { next_token(); node = set_node(SET_INTER, node, set_product()); }
    return node;
}

/* union := inter { (\cup | \setminus | \triangle) inter } */
static SetNode *set_union(void) {
    SetNode *node = set_inter();
    for (;;) {
        SetKind kind;
        if (is_command("cup")) kind = SET_UNION;
        else if (is_command("setminus") || is_command("smallsetminus") || is_command("backslash")) kind = SET_DIFF;
        else if (is_command("triangle") || is_command("bigtriangleup")) kind = SET_SYMDIFF;
        else return node;
        next_token();
        node = set_node(kind, node, set_inter());
    }
}

/* relation := union [ ('=' | \subseteq | \supseteq | \neq) union ] */
static SetNode *set_relation(void) {
    SetNode *left = set_union();
    if (is_char('=')) { next_token(); return set_node(SET_EQ, left, set_union()); }
    if (is_command("subseteq")) { next_token(); return set_node(SET_SUBSET, left, set_union()); }
    if (is_command("neq") || is_command("ne")) { next_token(); return set_node(SET_NEQ, left, set_union()); }

    /* A contiene B è lo stesso che B contenuto in A. */
    if (is_command("supseteq")) { next_token(); return set_node(SET_SUBSET, set_union(), left); }
    return left;
}

/* negation := \lnot negation | \exists lettera negation | relation */
static SetNode *set_negation(void) {
    if (is_command("lnot") || is_command("neg")) {
        next_token();
        return set_node(SET_NOT, set_negation(), NULL);
    }
    if (is_command("exists")) {
        next_token();
        if (token.kind != T_LETTER || !isupper(token.ch)) { syntax("dopo \\exists serve la lettera di un insieme"); return set_node(SET_EMPTY, NULL, NULL); }
        int var = set_var_index(token.ch);
        next_token();
        SetNode *node = set_node(SET_EXISTS, set_negation(), NULL);
        node->var = var;
        return node;
    }
    return set_relation();
}

/* conjunction := negation { \land negation } */
static SetNode *set_conjunction(void) {
    SetNode *node = set_negation();
    while (is_command("land") || is_command("wedge")) { next_token(); node = set_node(SET_AND, node, set_negation()); }
    return node;
}

/* disjunction := conjunction { \lor conjunction } */
static SetNode *set_disjunction(void) {
    SetNode *node = set_conjunction();
    while (is_command("lor") || is_command("vee")) { next_token(); node = set_node(SET_OR, node, set_conjunction()); }
    return node;
}

/* implication := disjunction [ \implies implication ]
 *
 * Associa a destra: A \implies B \implies C è A \implies (B \implies C). */
static SetNode *set_implication(void) {
    SetNode *node = set_disjunction();
    if (is_command("implies") || is_command("Rightarrow") || is_command("Longrightarrow")) {
        next_token();
        return set_node(SET_IMPLIES, node, set_implication());
    }
    return node;
}

/* formula := implication { \iff implication } */
static SetNode *set_formula(void) {
    SetNode *node = set_implication();
    while (is_command("iff") || is_command("Leftrightarrow") || is_command("Longleftrightarrow")) {
        next_token();
        node = set_node(SET_IFF, node, set_implication());
    }
    return node;
}

/* Ritorna il tipo comune a due oggetti, o TY_ERROR se sono di tipo diverso.
 * L'insieme vuoto prende il tipo dell'altro. */
static SetType set_join(SetType x, SetType y) {
    if (x == TY_ANY) return y;
    if (y == TY_ANY) return x;
    return x == y ? x : TY_ERROR;
}

/* Calcola il tipo di ogni nodo del sotto-albero, lo annota in type e ritorna
 * quello della radice: TY_ERROR se un operatore riceve operandi sbagliati, per
 * esempio l'unione di un insieme con una collezione di insiemi. */
static SetType set_check(SetNode *n) {
    SetType a = n->a ? set_check(n->a) : TY_ANY, b = n->b ? set_check(n->b) : TY_ANY;
    SetType both = set_join(a, b);
    int collection = both == TY_ANY || both == TY_SET || both == TY_FAMILY || both == TY_PAIRS;
    int plain = (a == TY_SET || a == TY_ANY) && (b == TY_SET || b == TY_ANY);
    int formulas = a == TY_FORMULA && (b == TY_FORMULA || !n->b);

    switch (n->kind) {
    case SET_VAR:        n->type = TY_SET; break;
    case SET_EMPTY:      n->type = TY_ANY; break;
    case SET_ONLY_EMPTY: n->type = TY_FAMILY; break;
    case SET_POWER:      n->type = plain ? TY_FAMILY : TY_ERROR; break;
    case SET_PRODUCT:    n->type = plain ? TY_PAIRS : TY_ERROR; break;
    case SET_UNION: case SET_INTER: case SET_DIFF: case SET_SYMDIFF:
        n->type = collection ? both : TY_ERROR; break;
    case SET_EQ: case SET_SUBSET: case SET_NEQ:
        n->type = collection ? TY_FORMULA : TY_ERROR; break;
    default:
        n->type = formulas ? TY_FORMULA : TY_ERROR; break;
    }
    return n->type;
}

/* =============================================================================
 * 15. INSIEMI: DECISIONE PER REGIONI
 * ============================================================================
 *
 * Come si decide un enunciato che deve valere per ogni scelta degli insiemi.
 *
 * Regioni. Dati k insiemi, ogni elemento sta in esattamente una delle 2^k
 * regioni del diagramma di Venn: una regione è la scelta, per ciascun insieme,
 * tra "ci sta" e "non ci sta". Una regione è il numero i cui bit dicono a
 * quali insiemi appartiene: con S di indice 0 e T di indice 1, la regione 1 è
 * "in S e non in T".
 *
 * Il fatto su cui si regge tutto: se un oggetto appartiene o no a
 * un'espressione dipende solo dalle regioni che tocca.
 *
 *   15a. Un elemento x: da quale regione lo contiene. x sta in A unito B se
 *        sta in A o in B, e così via.
 *   15b. Un insieme X: da quali regioni incontra. X sta nell'insieme delle
 *        parti di A se X è contenuto in A, cioè se ogni regione che X incontra
 *        sta in A; sta in {vuoto} se non ne incontra nessuna.
 *   15c. Una coppia (x, y): dalle regioni di x e di y. Sta in A per B se x sta
 *        in A e y sta in B.
 *
 * Configurazioni. Per insiemi dati, alcune regioni contengono elementi e altre
 * sono vuote: questa è la "configurazione". Un'uguaglianza o un'inclusione tra
 * due espressioni fallisce quando esiste un oggetto che sta in una sola delle
 * due (per l'inclusione: nella prima e non nella seconda), e un tale oggetto
 * esiste quando le regioni che tocca sono tutte non vuote. Quindi ogni
 * relazione ha un elenco di "supporti", insiemi di regioni, e fallisce
 * esattamente nelle configurazioni che contengono per intero un supporto.
 *
 * Decisione. La verità di tutto l'enunciato dipende allora solo dalla
 * configurazione. Le configurazioni sono 2^(2^k), ognuna è realizzata da
 * qualche scelta degli insiemi (un elemento per ogni regione non vuota), e
 * ogni scelta degli insiemi ne realizza una. L'enunciato vale per ogni scelta
 * degli insiemi se e solo se vale in tutte le configurazioni: si provano
 * tutte. Con al più 4 insiemi sono al più 65536.
 *
 * Il metodo decide davvero, in entrambi i sensi: un enunciato che fallisce in
 * una configurazione è falso, e la configurazione dà un controesempio.
 *
 * Il quantificatore "esiste un insieme V". Non si può trattare V come gli
 * altri insiemi, perché V va scelto, non è arbitrario. Due casi:
 *
 *   15d. "Esiste V" è un'ipotesi (sta a sinistra di un'implicazione). Dire
 *        "se esiste V con una certa proprietà allora C" equivale a dire "per
 *        ogni V, se V ha la proprietà allora C": V diventa un insieme
 *        arbitrario come gli altri.
 *   15e. "Esiste V" è una tesi. Si cerca un V esplicito costruito dagli altri
 *        insiemi, cioè un'unione di loro regioni: si provano tutte. Trovarlo
 *        dimostra l'esistenza. Non trovarlo non prova nulla, perché V potrebbe
 *        dover dividere una regione, e la risposta è NON DIMOSTRATO.
 */

/* Ritorna 1 se l'oggetto descritto da u e v appartiene all'espressione n.
 * Il significato di u e v dipende dal tipo di n (vedi 15a, 15b, 15c):
 *
 *   insieme di elementi:    u è la regione dell'elemento;
 *   collezione di insiemi:  u ha un bit per ogni regione che l'insieme incontra;
 *   insieme di coppie:      u e v sono le regioni delle due componenti. */
static int member(const SetNode *n, unsigned u, unsigned v) {
    switch (n->kind) {
    case SET_VAR:        return (u >> n->var) & 1;
    case SET_ONLY_EMPTY: return u == 0;
    case SET_POWER:
        for (unsigned region = 0; u >> region; region++)
            if (((u >> region) & 1) && !member(n->a, region, 0)) return 0;
        return 1;
    case SET_PRODUCT:    return member(n->a, u, 0) && member(n->b, v, 0);
    case SET_UNION:      return member(n->a, u, v) || member(n->b, u, v);
    case SET_INTER:      return member(n->a, u, v) && member(n->b, u, v);
    case SET_DIFF:       return member(n->a, u, v) && !member(n->b, u, v);
    case SET_SYMDIFF:    return member(n->a, u, v) != member(n->b, u, v);
    default:             return 0;  /* SET_EMPTY */
    }
}

/* Confronto per qsort: prima le configurazioni con meno regioni. */
static int by_size(const void *a, const void *b) {
    return __builtin_popcount(*(const unsigned *)a) - __builtin_popcount(*(const unsigned *)b);
}

/* Riempie support e supports della relazione n con i supporti minimi: gli
 * insiemi di regioni, nessuno contenuto in un altro, tali che la relazione
 * fallisce esattamente quando le regioni di uno di essi sono tutte non vuote.
 *
 * Si esaminano tutti gli oggetti del tipo giusto: le regioni per gli elementi,
 * gli insiemi di regioni per gli insiemi, le coppie di regioni per le coppie.
 * Ogni oggetto che distingue i due membri dà un supporto: le regioni che
 * tocca. Bastano i minimi, perché una configurazione che ne contiene uno più
 * grande contiene anche il più piccolo. */
static void relation_supports(SetNode *n) {
    SetType type = set_join(n->a->type, n->b->type);
    unsigned regions = 1u << set_var_count;
    unsigned long long objects = type == TY_FAMILY ? 1ull << regions
                               : type == TY_PAIRS ? (unsigned long long)regions * regions : regions;
    unsigned *found = arena_alloc((size_t)objects * sizeof(unsigned));
    if (!found) return;

    /* Tutti gli oggetti che distinguono i due membri. */
    int count = 0;
    for (unsigned long long object = 0; object < objects; object++) {
        unsigned u = type == TY_PAIRS ? (unsigned)(object / regions) : (unsigned)object;
        unsigned v = type == TY_PAIRS ? (unsigned)(object % regions) : 0;
        int left = member(n->a, u, v), right = member(n->b, u, v);
        if (n->kind == SET_SUBSET ? !(left && !right) : left == right) continue;
        found[count++] = type == TY_FAMILY ? u : type == TY_PAIRS ? (1u << u) | (1u << v) : 1u << u;
    }

    /* Dal più piccolo al più grande, si tiene un supporto solo se non ne
     * contiene uno già tenuto. */
    qsort(found, (size_t)count, sizeof(unsigned), by_size);
    int kept = 0;
    for (int i = 0; i < count; i++) {
        int redundant = 0;
        for (int j = 0; j < kept && !redundant; j++)
            if ((found[j] & ~found[i]) == 0) redundant = 1;
        if (!redundant) found[kept++] = found[i];
    }
    n->support = found;
    n->supports = kept;
}

/* Calcola i supporti di tutte le relazioni del sotto-albero. */
static void set_prepare(SetNode *n) {
    if (!n) return;
    if (n->kind == SET_EQ || n->kind == SET_SUBSET || n->kind == SET_NEQ) { relation_supports(n); return; }
    set_prepare(n->a);
    set_prepare(n->b);
}

/* Ritorna 1 se la formula n è vera nella configurazione in cui le regioni non
 * vuote sono quelle dei bit di nonempty. Il quantificatore si limita a
 * valutare il proprio corpo: come viene trattato il suo insieme lo decide chi
 * sceglie le configurazioni (vedi 15d e 15e). */
static int set_holds(const SetNode *n, unsigned nonempty) {
    switch (n->kind) {
    case SET_EQ: case SET_SUBSET: case SET_NEQ: {
        int fails = 0;
        for (int i = 0; i < n->supports && !fails; i++)
            if ((n->support[i] & ~nonempty) == 0) fails = 1;
        return n->kind == SET_NEQ ? fails : !fails;
    }
    case SET_AND:     return set_holds(n->a, nonempty) && set_holds(n->b, nonempty);
    case SET_OR:      return set_holds(n->a, nonempty) || set_holds(n->b, nonempty);
    case SET_IMPLIES: return !set_holds(n->a, nonempty) || set_holds(n->b, nonempty);
    case SET_IFF:     return set_holds(n->a, nonempty) == set_holds(n->b, nonempty);
    case SET_NOT:     return !set_holds(n->a, nonempty);
    default:          return set_holds(n->a, nonempty);  /* SET_EXISTS */
    }
}

/* Ritorna 1 se n è vera in ogni configurazione le cui regioni non vuote stanno
 * tra quelle di allowed. Altrimenti ritorna 0 e mette in *failing, tra le
 * configurazioni in cui n è falsa, una con il minor numero di regioni non
 * vuote: darà il controesempio più piccolo. */
static int set_valid(const SetNode *n, unsigned allowed, unsigned *failing) {
    int valid = 1;

    /* Tutti i sottoinsiemi di allowed: (p - 1) & allowed è il successivo in
     * ordine decrescente, e dopo lo zero il giro è finito. */
    for (unsigned p = allowed;; p = (p - 1) & allowed) {
        if (!set_holds(n, p) && (valid || __builtin_popcount(p) < __builtin_popcount(*failing))) {
            valid = 0;
            *failing = p;
        }
        if (p == 0) break;
    }
    return valid;
}

/* Ritorna le regioni compatibili con la scelta, per l'insieme di indice var,
 * di un'unione di regioni degli altri insiemi (caso 15e). mask ha un bit per
 * ogni regione degli altri insiemi: 1 se quella regione fa parte dell'insieme.
 * Una regione di tutti gli insiemi è compatibile se il suo bit var coincide
 * con quanto mask dice della regione che resta togliendo quel bit. */
static unsigned witness_regions(int var, unsigned mask) {
    unsigned allowed = 0;
    for (unsigned region = 0; region < (1u << set_var_count); region++) {
        unsigned low = region & ((1u << var) - 1), high = region >> (var + 1);
        unsigned others = low | (high << var);
        if (((region >> var) & 1) == ((mask >> others) & 1)) allowed |= 1u << region;
    }
    return allowed;
}

/* =============================================================================
 * 16. INSIEMI: VERIFICA INDIPENDENTE E DIMOSTRAZIONE SCRITTA
 * ============================================================================
 *
 * Verifica indipendente. Il capitolo 15 ragiona per regioni. Qui lo stesso
 * enunciato viene ricontrollato in un modo che non ne dipende: si prende un
 * universo piccolo, {1, ..., N}, si assegnano agli insiemi tutti i suoi
 * sottoinsiemi in tutti i modi possibili, e ogni espressione viene calcolata
 * davvero, come insieme. Se i due metodi non sono d'accordo, il verdetto non
 * viene emesso.
 *
 * Non è una seconda dimostrazione: N è piccolo. È un controllo contro gli
 * errori del programma, che è ciò a cui serve.
 *
 * Dimostrazione scritta. Il testo è in LaTeX. La forma preferita è quella
 * discorsiva dei libri: "sia x un elemento del primo insieme; allora... quindi
 * x sta nel secondo; viceversa...". La costruisce chase(), con i passi da 16d
 * a 16g, quando l'enunciato parla di insiemi di elementi o di coppie.
 *
 * Ogni fatto che quel testo afferma viene ricontrollato prima di essere
 * scritto, con un metodo che non usa le regole di chase(): è il controllo
 * 16h.
 *
 * Quando quella forma non si applica (insiemi delle parti, ipotesi "non
 * vuoto", quantificatori) o richiederebbe troppi casi, il testo segue invece
 * il ragionamento per regioni del capitolo 15, in una di tre forme:
 *
 *   16a. Una sola relazione tra insiemi di elementi o di coppie: la tabella di
 *        appartenenza, con una riga per ogni caso.
 *   16b. Un'equivalenza o un'implicazione tra condizioni: per ciascuna, in
 *        quali configurazioni fallisce; il confronto dei due elenchi conclude.
 *   16c. Ogni altro enunciato: per ogni relazione le configurazioni in cui
 *        fallisce, e l'esame di tutte le configurazioni.
 *
 * Anche questi testi affermano dei fatti: le righe di una tabella, gli elenchi
 * di configurazioni, l'insieme scelto per un "esiste". Ognuno viene
 * ricalcolato con gli insiemi veri prima di essere scritto: è il controllo
 * 16i.
 */

/* Ritorna il valore dell'espressione n quando gli insiemi valgono value[],
 * sottoinsiemi di un universo di size elementi. Il risultato è una maschera:
 *
 *   insieme di elementi:    il bit e dice se l'elemento e ne fa parte;
 *   collezione di insiemi:  il bit X dice se l'insieme di maschera X ne fa
 *                           parte;
 *   insieme di coppie:      il bit x * size + y dice se (x, y) ne fa parte.
 *
 * In tutti e tre i casi unione, intersezione e differenza sono le operazioni
 * sui bit. size non deve superare 5, perché le maschere abbiano 32 bit. */
static unsigned concrete(const SetNode *n, const unsigned *value, int size) {
    unsigned a = n->a ? concrete(n->a, value, size) : 0, b = n->b ? concrete(n->b, value, size) : 0;
    unsigned result = 0;
    switch (n->kind) {
    case SET_VAR:        return value[n->var];
    case SET_ONLY_EMPTY: return 1u;  /* l'insieme vuoto ha maschera 0 */
    case SET_POWER:
        for (unsigned x = 0; x < (1u << size); x++)
            if ((x & ~a) == 0) result |= 1u << x;
        return result;
    case SET_PRODUCT:
        for (int x = 0; x < size; x++)
            for (int y = 0; y < size; y++)
                if (((a >> x) & 1) && ((b >> y) & 1)) result |= 1u << (x * size + y);
        return result;
    case SET_UNION:      return a | b;
    case SET_INTER:      return a & b;
    case SET_DIFF:       return a & ~b;
    case SET_SYMDIFF:    return a ^ b;
    default:             return 0;  /* SET_EMPTY */
    }
}

/* Ritorna 1 se la formula n è vera quando gli insiemi valgono value[]. Il
 * quantificatore prova davvero tutti i sottoinsiemi dell'universo, scrivendo
 * in value[] l'insieme in esame. */
static int concrete_holds(const SetNode *n, unsigned *value, int size) {
    switch (n->kind) {
    case SET_EQ:      return concrete(n->a, value, size) == concrete(n->b, value, size);
    case SET_NEQ:     return concrete(n->a, value, size) != concrete(n->b, value, size);
    case SET_SUBSET:  return (concrete(n->a, value, size) & ~concrete(n->b, value, size)) == 0;
    case SET_AND:     return concrete_holds(n->a, value, size) && concrete_holds(n->b, value, size);
    case SET_OR:      return concrete_holds(n->a, value, size) || concrete_holds(n->b, value, size);
    case SET_IMPLIES: return !concrete_holds(n->a, value, size) || concrete_holds(n->b, value, size);
    case SET_IFF:     return concrete_holds(n->a, value, size) == concrete_holds(n->b, value, size);
    case SET_NOT:     return !concrete_holds(n->a, value, size);
    default:          /* SET_EXISTS */
        for (unsigned candidate = 0; candidate < (1u << size); candidate++) {
            value[n->var] = candidate;
            if (concrete_holds(n->a, value, size)) return 1;
        }
        return 0;
    }
}

/* Ritorna 1 se root è vera per ogni assegnazione di sottoinsiemi di un
 * universo di size elementi agli insiemi dell'enunciato. bound è l'indice
 * dell'insieme legato da un quantificatore, o -1: quello non si assegna qui. */
static int concrete_valid(const SetNode *root, int bound, int size) {
    unsigned value[MAX_SET_VARS] = {0};
    for (;;) {
        if (!concrete_holds(root, value, size)) return 0;

        /* Assegnazione successiva, come un contachilometri: si incrementa il
         * primo insieme, e quando ha fatto il giro si passa al seguente. */
        int i = 0;
        for (; i < set_var_count; i++) {
            if (i == bound) continue;
            if (++value[i] < (1u << size)) break;
            value[i] = 0;
        }
        if (i == set_var_count) return 1;
    }
}

/* Costruisce gli insiemi che realizzano la configurazione nonempty, con un
 * elemento per ogni regione non vuota: li mette in value[] come maschere su un
 * universo di *size elementi. */
static void configuration_sets(unsigned nonempty, unsigned *value, int *size) {
    *size = 0;
    for (int i = 0; i < set_var_count; i++) value[i] = 0;
    for (unsigned region = 0; nonempty >> region; region++) {
        if (!((nonempty >> region) & 1)) continue;
        for (int i = 0; i < set_var_count; i++)
            if ((region >> i) & 1) value[i] |= 1u << *size;
        (*size)++;
    }
}

/* 16i. Controllo di un fatto delle forme 16b e 16c: "la condizione n fallisce
 * esattamente nelle configurazioni che contengono una di quelle di list".
 * Gli elenchi nascono dal conto per regioni del capitolo 15; qui, per ogni
 * configurazione contenuta in allowed, si costruiscono gli insiemi veri e si
 * calcola la condizione con concrete_holds(), che delle regioni non sa nulla.
 * Si provano le configurazioni con al più 5 regioni non vuote, perché gli
 * insiemi concreti hanno al più 5 elementi.
 *
 * Con holds diverso da zero il fatto è "vale esattamente", come per una
 * relazione "diverso da". Ritorna 1 se il fatto è confermato; altrimenti
 * annota il passo come non giustificato e ritorna 0. */
static int list_confirmed(const SetNode *n, unsigned allowed, const unsigned *list, int count, int holds) {
    for (unsigned p = allowed;; p = (p - 1) & allowed) {
        if (__builtin_popcount(p) <= 5) {
            unsigned value[MAX_SET_VARS];
            int size, listed = 0;
            for (int i = 0; i < count; i++)
                if ((list[i] & ~p) == 0) listed = 1;
            configuration_sets(p, value, &size);
            if ((concrete_holds(n, value, size) != 0) != (holds ? listed : !listed)) { unjustified_steps++; return 0; }
        }
        if (p == 0) break;
    }
    return 1;
}

/* Aggiunge alla dimostrazione l'espressione o la formula n, in LaTeX. Un
 * operando viene messo tra parentesi quando è a sua volta un'operazione dello
 * stesso livello: tra insiemi sotto un'operazione tra insiemi, tra condizioni
 * sotto un connettivo. */
static void put_set(const SetNode *n) {
    static const char *const symbol[] = {
        [SET_UNION] = "\\cup", [SET_INTER] = "\\cap", [SET_DIFF] = "\\setminus", [SET_SYMDIFF] = "\\triangle",
        [SET_PRODUCT] = "\\times", [SET_EQ] = "=", [SET_SUBSET] = "\\subseteq", [SET_NEQ] = "\\neq",
        [SET_AND] = "\\land", [SET_OR] = "\\lor", [SET_IMPLIES] = "\\implies", [SET_IFF] = "\\iff",
    };
    switch (n->kind) {
    case SET_VAR:        proof_add("%c", set_var_name[n->var]); return;
    case SET_EMPTY:      proof_add("\\emptyset"); return;
    case SET_ONLY_EMPTY: proof_add("\\{\\emptyset\\}"); return;
    case SET_POWER:      proof_add("\\mathcal{P}("); put_set(n->a); proof_add(")"); return;
    case SET_NOT:        proof_add("\\lnot ("); put_set(n->a); proof_add(")"); return;
    case SET_EXISTS:     proof_add("\\exists %c\\,(", set_var_name[n->var]); put_set(n->a); proof_add(")"); return;
    default:             break;
    }

    /* Operatori binari. */
    int logical = n->kind >= SET_AND;
    for (int side = 0; side < 2; side++) {
        const SetNode *operand = side ? n->b : n->a;
        int nested = logical ? operand->kind >= SET_AND && operand->kind != SET_NOT && operand->kind != SET_EXISTS
                             : operand->kind >= SET_UNION && operand->kind <= SET_PRODUCT && n->kind <= SET_PRODUCT;
        if (side) proof_add(" %s ", symbol[n->kind]);
        if (nested) proof_add("(");
        put_set(operand);
        if (nested) proof_add(")");
    }
}

/* Aggiunge alla dimostrazione il nome di una regione, per esempio
 * S \cap T^{c} per "in S e non in T". */
static void put_region(unsigned region) {
    for (int i = 0; i < set_var_count; i++)
        proof_add("%s%c%s", i ? " \\cap " : "", set_var_name[i], ((region >> i) & 1) ? "" : "^{c}");
}

/* Aggiunge alla dimostrazione la descrizione delle configurazioni di list, da
 * leggere dopo "fails exactly": ogni elemento è un insieme di regioni che
 * devono essere tutte non vuote. */
static void put_configurations(const unsigned *list, int count) {
    if (count == 0) { proof_add("in no case"); return; }
    if (list[0] == 0) { proof_add("in every case"); return; }

    /* Il caso più comune: basta una regione non vuota. */
    int single = 1;
    for (int i = 0; i < count; i++)
        if (__builtin_popcount(list[i]) != 1) single = 0;
    if (single && count <= MAX_CONFIGS) {
        proof_add(count == 1 ? "when the region " : "when at least one of the regions ");
        for (int i = 0; i < count; i++) {
            proof_add("%s$", i ? ", " : "");
            put_region((unsigned)__builtin_ctz(list[i]));
            proof_add("$");
        }
        proof_add(" is non-empty");
        return;
    }

    /* In generale: più regioni non vuote insieme. */
    for (int i = 0; i < count && i < MAX_CONFIGS; i++) {
        int size = __builtin_popcount(list[i]), first = 1;
        proof_add("%swhen the region%s ", i ? "; or " : "", size == 1 ? "" : "s");
        for (unsigned region = 0; list[i] >> region; region++) {
            if (!((list[i] >> region) & 1)) continue;
            proof_add("%s$", first ? "" : " and ");
            put_region(region);
            proof_add("$");
            first = 0;
        }
        proof_add(size == 1 ? " is non-empty" : size == 2 ? " are both non-empty" : " are all non-empty");
    }
    if (count > MAX_CONFIGS) proof_add("; or in %d further situations of the same kind", count - MAX_CONFIGS);
}

/* Ritorna 1 se nel sotto-albero c'è una relazione tra oggetti di tipo type. */
static int set_relates(const SetNode *n, SetType type) {
    if (!n) return 0;
    if (n->kind == SET_EQ || n->kind == SET_SUBSET || n->kind == SET_NEQ) {
        SetType operands = set_join(n->a->type, n->b->type);
        return operands == type || (type == TY_SET && operands == TY_ANY);
    }
    return set_relates(n->a, type) || set_relates(n->b, type);
}

/* Aggiunge alla dimostrazione la premessa comune alle forme 16b e 16c: cosa
 * sono le regioni e perché una relazione fallisce in certe configurazioni. Si
 * citano solo i tipi di oggetto presenti nell'enunciato root. */
static void put_regions_preamble(const SetNode *root) {
    proof_add("The sets ");
    for (int i = 0; i < set_var_count; i++) proof_add("%s$%c$", i ? ", " : "", set_var_name[i]);
    proof_add(" divide all elements into regions: a region is fixed by saying, for each of these sets, whether an "
              "element belongs to it. We write $A^{c}$ for ``not in $A$'', so that for instance $");
    put_region(1);
    proof_add("$ is the region of the elements that lie in $%c$ and in none of the other sets. ", set_var_name[0]);
    if (set_relates(root, TY_SET))
        proof_add("Whether an element belongs to a set built from them depends only on its region; so an inclusion "
                  "$A \\subseteq B$ fails exactly when some region contained in $A$ but not in $B$ is non-empty, and "
                  "an equality $A = B$ fails exactly when some region contained in only one of $A$, $B$ is non-empty. ");
    if (set_relates(root, TY_FAMILY))
        proof_add("A set $X$ belongs to $\\mathcal{P}(A)$ exactly when every region that $X$ meets is contained in "
                  "$A$, and to $\\{\\emptyset\\}$ exactly when $X$ meets no region; so whether $X$ belongs to a "
                  "collection of sets depends only on the regions it meets, and two such collections differ exactly "
                  "when some set, with one element in each of certain regions and no other element, lies in only one "
                  "of them. This requires those regions to be all non-empty. ");
    if (set_relates(root, TY_PAIRS))
        proof_add("A pair $(x, y)$ belongs to $A \\times B$ exactly when $x \\in A$ and $y \\in B$; so whether it "
                  "belongs to a set of pairs depends only on the regions of $x$ and of $y$, and two sets of pairs "
                  "differ exactly when some pair lies in only one of them, which requires the regions of its two "
                  "components to be non-empty. ");
    proof_add("\n\n");
}

/* Aggiunge alla dimostrazione un elenco con, per ogni relazione del
 * sotto-albero, le configurazioni in cui fallisce (forma 16c). */
static void put_relation_items(const SetNode *n) {
    if (!n) return;
    if (n->kind == SET_EQ || n->kind == SET_SUBSET || n->kind == SET_NEQ) {
        proof_add("\\item $");
        put_set(n);
        proof_add(n->kind == SET_NEQ ? "$ holds exactly " : "$ fails exactly ");
        put_configurations(n->support, n->supports);
        proof_add(".\n");
        return;
    }
    put_relation_items(n->a);
    put_relation_items(n->b);
}

/* Ritorna 1 se la formula è fatta solo di uguaglianze e inclusioni unite da
 * "e" e "o". Una formula così, se fallisce in una configurazione, fallisce in
 * ogni configurazione con più regioni non vuote: per descriverla bastano le
 * configurazioni minime in cui fallisce. */
static int set_is_monotone(const SetNode *n) {
    if (n->kind == SET_EQ || n->kind == SET_SUBSET) return 1;
    return (n->kind == SET_AND || n->kind == SET_OR) && set_is_monotone(n->a) && set_is_monotone(n->b);
}

/* Mette in out le configurazioni minime, tra quelle contenute in allowed, in
 * cui la formula monotona n fallisce, dalla più piccola alla più grande, e ne
 * ritorna il numero; -1 se sono più di max. Una configurazione è minima se
 * togliendole una qualsiasi regione la formula torna vera. */
static int minimal_failures(const SetNode *n, unsigned allowed, unsigned *out, int max) {
    int count = 0;
    for (unsigned p = allowed;; p = (p - 1) & allowed) {
        int minimal = !set_holds(n, p);
        for (unsigned region = 0; minimal && p >> region; region++)
            if (((p >> region) & 1) && !set_holds(n, p & ~(1u << region))) minimal = 0;
        if (minimal) {
            if (count == max) return -1;
            out[count++] = p;
        }
        if (p == 0) break;
    }
    qsort(out, (size_t)count, sizeof(unsigned), by_size);
    return count;
}

/* Raccoglie in *firsts e *seconds le lettere che nel sotto-albero compaiono
 * nel primo e nel secondo fattore di un prodotto cartesiano: sono quelle a cui
 * possono appartenere le componenti x e y di una coppia. into è il mucchio a
 * cui va la lettera corrente, o NULL fuori da un prodotto. */
static void product_factors(const SetNode *n, unsigned *firsts, unsigned *seconds, unsigned *into) {
    if (!n) return;
    if (n->kind == SET_VAR && into) *into |= 1u << n->var;
    if (n->kind == SET_PRODUCT) {
        product_factors(n->a, firsts, seconds, firsts);
        product_factors(n->b, firsts, seconds, seconds);
        return;
    }
    product_factors(n->a, firsts, seconds, into);
    product_factors(n->b, firsts, seconds, into);
}

/* Forma 16a: se root è una sola relazione tra insiemi di elementi o di coppie
 * scrive la dimostrazione con la tabella di appartenenza e ritorna 1;
 * altrimenti non scrive nulla e ritorna 0.
 *
 * Per gli insiemi di elementi le colonne sono "x appartiene a S" per ogni
 * insieme S. Per le coppie (x, y) sono "x appartiene a S" per gli insiemi che
 * compaiono come primo fattore di un prodotto e "y appartiene a T" per quelli
 * che compaiono come secondo. */
static int put_table_proof(const SetNode *root) {
    if (root->kind != SET_EQ && root->kind != SET_SUBSET) return 0;
    SetType type = set_join(root->a->type, root->b->type);
    unsigned firsts = (1u << set_var_count) - 1, seconds = 0;
    if (type == TY_PAIRS) {
        firsts = 0;
        product_factors(root, &firsts, &seconds, NULL);
    } else if (type != TY_SET && type != TY_ANY) {
        return 0;
    }
    int columns = __builtin_popcount(firsts) + __builtin_popcount(seconds);
    if (columns > 4) return 0;
    size_t start = proof_length;

    /* Premessa. */
    const char *object = type == TY_PAIRS ? "$(x, y)$" : "$x$";
    proof_add("\\textbf{Proof.} Let %s be an arbitrary %s. Each row of the table is one possible combination of the "
              "memberships in its first columns ($1$ = belongs, $0$ = does not belong); the last two columns say, "
              "by the definitions of the operations, whether %s belongs to each side.\n\n",
              object, type == TY_PAIRS ? "pair" : "element", object);

    /* Intestazione. */
    proof_add("\\begin{center}\\begin{tabular}{");
    for (int i = 0; i < columns; i++) proof_add("c");
    proof_add("|cc}\n");
    for (int i = 0; i < set_var_count; i++)
        if ((firsts >> i) & 1) proof_add("$x \\in %c$ & ", set_var_name[i]);
    for (int i = 0; i < set_var_count; i++)
        if ((seconds >> i) & 1) proof_add("$y \\in %c$ & ", set_var_name[i]);
    proof_add("$");
    put_set(root->a);
    proof_add("$ & $");
    put_set(root->b);
    proof_add("$ \\\\ \\hline\n");

    /* Una riga per ogni combinazione: u e v sono le regioni di x e di y. */
    for (unsigned u = 0; u < (1u << set_var_count); u++) {
        if (u & ~firsts) continue;
        for (unsigned v = 0; v < (1u << set_var_count); v++) {
            if (v & ~seconds) continue;
            for (int i = 0; i < set_var_count; i++)
                if ((firsts >> i) & 1) proof_add("%u & ", (u >> i) & 1);
            for (int i = 0; i < set_var_count; i++)
                if ((seconds >> i) & 1) proof_add("%u & ", (v >> i) & 1);
            int left = member(root->a, u, v), right = member(root->b, u, v);
            proof_add("%d & %d \\\\\n", left, right);

            /* 16i. Controllo della riga. Le due colonne si ricalcolano con
             * gli insiemi veri: un universo di due elementi, x (il numero 0)
             * e y (il numero 1), messi negli insiemi che la riga dice. Per
             * gli insiemi di elementi si guarda x, cioè il bit 0; per quelli
             * di coppie la coppia (x, y), cioè il bit 0 * 2 + 1. E la riga
             * deve essere compatibile con ciò che la conclusione dirà di
             * tutte: colonne uguali, oppure la prima non oltre la seconda. */
            unsigned value[MAX_SET_VARS], bit = type == TY_PAIRS ? 2u : 1u;
            for (int i = 0; i < set_var_count; i++) value[i] = ((u >> i) & 1) | (((v >> i) & 1) << 1);
            int agrees = ((concrete(root->a, value, 2) & bit) != 0) == left && ((concrete(root->b, value, 2) & bit) != 0) == right;
            if (!agrees || (root->kind == SET_EQ ? left != right : left && !right)) {
                unjustified_steps++;
                proof_length = start;
                proof_text[start] = 0;
                return 0;
            }
        }
    }
    proof_add("\\end{tabular}\\end{center}\n\n");

    /* Conclusione. */
    if (root->kind == SET_EQ)
        proof_add("The last two columns are equal in every row, so the two sides have the same elements. $\\Box$");
    else
        proof_add("In every row where the first of the last two columns is $1$, the second is $1$ as well, so every "
                  "element of the left-hand side belongs to the right-hand side. $\\Box$");
    return 1;
}

/* Forma 16b: se root è un'equivalenza o un'implicazione tra condizioni
 * monotone, eventualmente sotto ipotesi che sono inclusioni o uguaglianze tra
 * insiemi di elementi, scrive la dimostrazione per confronto delle
 * configurazioni e ritorna 1; altrimenti non scrive nulla e ritorna 0.
 *
 * Le ipotesi di quel tipo dicono solo che certe regioni sono vuote: le si
 * toglie dalle regioni ammesse e si ragiona sulle altre. */
static int put_comparison_proof(const SetNode *root) {
    const SetNode *hypothesis = NULL, *core = root;
    unsigned all = (1u << (1u << set_var_count)) - 1, allowed = all;
    if (set_var_count == 5) return 0;

    /* Ipotesi, se ci sono: la parte sinistra di un'implicazione esterna. */
    if (root->kind == SET_IMPLIES && (root->b->kind == SET_IFF || root->b->kind == SET_IMPLIES)) {
        unsigned empty[MAX_CONFIGS];
        hypothesis = root->a;
        core = root->b;
        if (!set_is_monotone(hypothesis)) return 0;
        int count = minimal_failures(hypothesis, all, empty, MAX_CONFIGS);
        if (count < 0 || !list_confirmed(hypothesis, all, empty, count, 0)) return 0;
        for (int i = 0; i < count; i++) {
            if (__builtin_popcount(empty[i]) != 1) return 0;
            allowed &= ~empty[i];
        }
    }
    if ((core->kind != SET_IFF && core->kind != SET_IMPLIES) || !set_is_monotone(core->a) || !set_is_monotone(core->b))
        return 0;

    /* Le configurazioni minime in cui ciascuna delle due condizioni fallisce. */
    unsigned left[MAX_CONFIGS], right[MAX_CONFIGS];
    int lefts = minimal_failures(core->a, allowed, left, MAX_CONFIGS);
    int rights = minimal_failures(core->b, allowed, right, MAX_CONFIGS);
    if (lefts < 0 || rights < 0) return 0;

    /* 16i. I due elenchi, ricontrollati con gli insiemi veri, e ciò che il
     * testo dirà del loro confronto: ogni configurazione minima in cui la
     * seconda condizione fallisce ne contiene una in cui fallisce la prima
     * e, per un'equivalenza, anche il viceversa. */
    if (!list_confirmed(core->a, allowed, left, lefts, 0) || !list_confirmed(core->b, allowed, right, rights, 0)) return 0;
    for (int way = 0; way < (core->kind == SET_IFF ? 2 : 1); way++) {
        const unsigned *weak = way ? left : right, *strong = way ? right : left;
        int weaks = way ? lefts : rights, strongs = way ? rights : lefts;
        for (int i = 0; i < weaks; i++) {
            int contains = 0;
            for (int j = 0; j < strongs; j++)
                if ((strong[j] & ~weak[i]) == 0) contains = 1;
            if (!contains) { unjustified_steps++; return 0; }
        }
    }

    proof_add("\\textbf{Proof.} ");
    put_regions_preamble(root);
    if (hypothesis) {
        unsigned empty = all & ~allowed;
        proof_add("By the hypothesis $");
        put_set(hypothesis);
        proof_add("$, ");
        if (empty) {
            int first = 1;
            for (unsigned region = 0; empty >> region; region++) {
                if (!((empty >> region) & 1)) continue;
                proof_add("%s$", first ? "the following regions are empty: " : ", ");
                put_region(region);
                proof_add("$");
                first = 0;
            }
            proof_add("; only the remaining regions are considered below. ");
        } else {
            proof_add("no region is forced to be empty. ");
        }
    }

    if (core->kind == SET_IFF) {
        /* Le due condizioni falliscono nelle stesse configurazioni: gli
         * elenchi coincidono, e se ne stampa uno. */
        proof_add("Each of the two conditions $");
        put_set(core->a);
        proof_add("$ and $");
        put_set(core->b);
        proof_add("$ fails exactly ");
        put_configurations(left, lefts);
        proof_add(". Hence one holds if and only if the other does. $\\Box$");
    } else {
        proof_add("The condition $");
        put_set(core->a);
        proof_add("$ fails exactly ");
        put_configurations(left, lefts);
        proof_add("; the condition $");
        put_set(core->b);
        proof_add("$ fails exactly ");
        put_configurations(right, rights);
        proof_add(". Every situation in which the second fails is one in which the first fails as well; hence the "
                  "first implies the second. $\\Box$");
    }
    return 1;
}

/* 16i. Ritorna 1 se per ogni relazione del sotto-albero l'elenco delle
 * configurazioni in cui fallisce (o vale, se è "diverso da"), che la forma
 * 16c sta per scrivere, è confermato da list_confirmed(). */
static int relations_confirmed(const SetNode *n) {
    if (!n) return 1;
    if (n->kind == SET_EQ || n->kind == SET_SUBSET || n->kind == SET_NEQ)
        return list_confirmed(n, (1u << (1u << set_var_count)) - 1, n->support, n->supports, n->kind == SET_NEQ);
    return relations_confirmed(n->a) && relations_confirmed(n->b);
}

/* Forma 16c: scrive la dimostrazione generica di root. witness, se non è
 * NULL, è il quantificatore per cui è stato trovato l'insieme esplicito
 * unione delle regioni di mask (vedi 15e). Ritorna 0, senza scrivere nulla,
 * se un elenco che starebbe per scrivere non supera il controllo 16i. */
static int put_general_proof(const SetNode *root, const SetNode *witness, unsigned mask) {
    int single = root->kind == SET_EQ || root->kind == SET_SUBSET;
    if (!single && !relations_confirmed(root)) return 0;
    proof_add("\\textbf{Proof.} ");
    put_regions_preamble(root);

    /* Una sola relazione, tra collezioni di insiemi o tra insiemi di coppie:
     * non ci sono condizioni da combinare, si confrontano i due membri su
     * tutti gli oggetti possibili. */
    if (single) {
        unsigned regions = 1u << set_var_count;
        if (set_relates(root, TY_FAMILY))
            proof_add("There are $%u$ ways of choosing which regions a set $X$ meets. In each of them, ", 1u << regions);
        else
            proof_add("There are $%u$ ways of choosing the regions of $x$ and of $y$ for a pair $(x, y)$. In each of them, ", regions * regions);
        proof_add(root->kind == SET_EQ ? "it belongs to the left-hand side exactly when it belongs to the right-hand side. "
                                         "Hence the two sides have the same elements. $\\Box$"
                                       : "if it belongs to the left-hand side then it belongs to the right-hand side. "
                                         "Hence the left-hand side is contained in the right-hand side. $\\Box$");
        return 1;
    }
    proof_add("For the conditions in the statement this gives:\n\\begin{itemize}\n");
    put_relation_items(root);
    proof_add("\\end{itemize}\n");

    if (witness) {
        /* L'insieme scelto, scritto come unione di regioni degli altri. */
        proof_add("For the set $%c$ whose existence is claimed we take $%c = ", set_var_name[witness->var], set_var_name[witness->var]);
        int first = 1;
        for (unsigned others = 0; mask >> others; others++) {
            if (!((mask >> others) & 1)) continue;
            unsigned low = others & ((1u << witness->var) - 1), high = others >> witness->var;
            unsigned region = low | (high << (witness->var + 1));
            proof_add("%s(", first ? "" : " \\cup ");
            for (int i = 0, printed = 0; i < set_var_count; i++) {
                if (i == witness->var) continue;
                proof_add("%s%c%s", printed++ ? " \\cap " : "", set_var_name[i], ((region >> i) & 1) ? "" : "^{c}");
            }
            proof_add(")");
            first = 0;
        }
        proof_add("%s$; where $%c$ instead appears in a hypothesis, it is an arbitrary set and counts as one more "
                  "set dividing the elements into regions. ", first ? "\\emptyset" : "", set_var_name[witness->var]);
    }
    proof_add("The statement is a logical combination of these conditions, so its truth depends only on which "
              "regions are empty. It was checked in each of the possible cases, and it holds in all of them. $\\Box$");
    return 1;
}

/* Un fatto su un punto: sta (sign = 1) o non sta (sign = 0) in node. Per un
 * insieme di elementi coord dice di quale componente si parla, 0 per x e 1 per
 * y; per un insieme di coppie il punto è la coppia (x, y) e coord non conta. */
typedef struct {
    const SetNode *node;
    int coord, sign;
} Fact;

/* Lo stato di una dimostrazione "per inseguimento degli elementi": ciò che si
 * sa del punto in esame e ciò che resta da sviluppare.
 *
 * Di ogni lettera si sa se x (indice 0) o y (indice 1) le appartiene: il bit
 * della lettera in known dice se lo si sa, quello in truth cosa si sa.
 * pending sono i fatti affermati ma non ancora scomposti. list_start serve a
 * sapere se l'elenco di passaggi in corso è ancora vuoto. */
typedef struct {
    unsigned known[2], truth[2];
    Fact pending[24];
    int pendings;
    size_t list_start;  /* lunghezza del testo quando è iniziato l'elenco di passaggi in corso */
    unsigned long long alive[4];  /* 16h: i mondi ancora possibili, un bit per mondo */
} Chase;

/* Passi ancora concessi: se si esauriscono la dimostrazione discorsiva viene
 * abbandonata, per non scrivere pagine di casi. Un valore negativo dice che
 * va abbandonata comunque. */
static int chase_budget;

/* Le ipotesi utilizzabili durante un inseguimento, come inclusioni: ogni
 * punto di hypothesis_sub[i] sta in hypothesis_sup[i]. */
static const SetNode *hypothesis_sub[16], *hypothesis_sup[16];
static int hypotheses;

/* Le lettere su cui ha senso distinguere due casi, per x e per y. */
static unsigned chase_letters[2];

/* Quanti casi sono aperti uno dentro l'altro. Ogni caso è un elenco rientrato
 * nel documento, e LaTeX ne ammette quattro annidati: oltre il terzo livello
 * di casi la dimostrazione discorsiva viene abbandonata. */
static int chase_depth;

/* 16h. Controllo di ogni passo.
 *
 * chase() ricava i fatti con le sue regole: scompone, usa le ipotesi, apre
 * casi. Qui gli stessi fatti vengono ricontrollati senza regole, per forza
 * bruta. Un "mondo" è un modo completo di dire a quali insiemi appartengono x
 * e y: due regioni, u per x e v per y. Con al più quattro insiemi i mondi sono
 * al più 16 * 16 = 256, e il mondo (u, v) ha il numero 16 u + v.
 *
 * Ogni ramo della dimostrazione porta con sé l'elenco dei mondi ancora
 * possibili, Chase.alive: all'inizio quelli che rispettano le ipotesi; ogni
 * volta che il testo assume qualcosa ("sia x in ...", "caso x in T") si
 * tolgono i mondi in cui non vale. Allora:
 *
 *   - un fatto che il testo afferma come conseguenza deve valere in tutti i
 *     mondi rimasti;
 *   - "questo caso non può presentarsi" richiede che non ne sia rimasto
 *     nessuno;
 *   - quando si distinguono due casi, ogni mondo rimasto deve stare in uno
 *     dei due.
 *
 * Se un controllo fallisce la dimostrazione discorsiva viene abbandonata,
 * come quando i passi concessi finiscono, e il contatore unjustified_steps
 * sale: il verdetto non cambia, perché è già stato stabilito in altro modo,
 * ma quel testo non viene scritto. */

/* Ritorna 1 se nel mondo (u, v) il fatto è vero. */
static int fact_in_world(Fact fact, unsigned u, unsigned v) {
    int inside = fact.node->type == TY_PAIRS ? member(fact.node, u, v) : member(fact.node, fact.coord ? v : u, 0);
    return inside == fact.sign;
}

/* Ritorna 1 se il mondo numero w è ancora possibile nel ramo c. */
static int world_alive(const Chase *c, unsigned w) { return (c->alive[w / 64] >> (w % 64)) & 1; }

/* Da qui in poi il ramo c assume il fatto: toglie i mondi in cui non vale. */
static void worlds_assume(Chase *c, Fact fact) {
    for (unsigned w = 0; w < 256; w++)
        if (!fact_in_world(fact, w >> 4, w & 15)) c->alive[w / 64] &= ~(1ULL << (w % 64));
}

/* Annota che un passo non ha superato il controllo e fa abbandonare la
 * dimostrazione discorsiva. Ritorna 0, per comodità di chi chiama. */
static int step_unjustified(void) {
    unjustified_steps++;
    chase_budget = -1;
    return 0;
}

/* Ritorna 1 se il fatto vale in tutti i mondi possibili nel ramo c; altrimenti
 * annota il passo come non giustificato e ritorna 0. */
static int worlds_entail(const Chase *c, Fact fact) {
    for (unsigned w = 0; w < 256; w++)
        if (world_alive(c, w) && !fact_in_world(fact, w >> 4, w & 15)) return step_unjustified();
    return 1;
}

/* Ritorna 1 se nel ramo c non è rimasto nessun mondo possibile; altrimenti
 * annota il passo come non giustificato e ritorna 0. */
static int worlds_none(const Chase *c) {
    return (c->alive[0] | c->alive[1] | c->alive[2] | c->alive[3]) == 0 ? 1 : step_unjustified();
}

/* Aggiunge un fatto da sviluppare. */
static void chase_push(Chase *c, const SetNode *node, int coord, int sign) {
    if (c->pendings == (int)(sizeof c->pending / sizeof *c->pending)) { chase_budget = -1; return; }
    c->pending[c->pendings++] = (Fact){node, coord, sign};
}

/* Ritorna 1 se da ciò che si sa segue che il punto sta in n, 0 se segue che
 * non ci sta, -1 se non si può ancora dire. */
static int chase_value(const SetNode *n, int coord, const Chase *c) {
    if (n->kind == SET_VAR) return ((c->known[coord] >> n->var) & 1) ? (int)((c->truth[coord] >> n->var) & 1) : -1;
    if (n->kind == SET_EMPTY) return 0;
    int a = chase_value(n->a, n->kind == SET_PRODUCT ? 0 : coord, c);
    int b = chase_value(n->b, n->kind == SET_PRODUCT ? 1 : coord, c);
    switch (n->kind) {
    case SET_UNION:   return a == 1 || b == 1 ? 1 : a == 0 && b == 0 ? 0 : -1;
    case SET_DIFF:    return a == 0 || b == 1 ? 0 : a == 1 && b == 0 ? 1 : -1;
    case SET_SYMDIFF: return a < 0 || b < 0 ? -1 : a != b;
    default:          return a == 0 || b == 0 ? 0 : a == 1 && b == 1 ? 1 : -1;  /* intersezione, prodotto */
    }
}

/* Aggiunge alla dimostrazione il fatto "il punto sta / non sta in n". */
static void put_fact(const SetNode *n, int coord, int sign) {
    proof_add("$%s %s ", n->type == TY_PAIRS ? "(x, y)" : coord ? "y" : "x", sign ? "\\in" : "\\notin");
    put_set(n);
    proof_add("$");
}

/* Aggiunge alla dimostrazione la giustificazione di ciò che chase_value()
 * dice di n: per ogni operazione, quali fatti sui suoi operandi la decidono,
 * partendo dalle più interne. Per esempio "As x in S and x not in T,
 * x in S \ T". Delle lettere non c'è nulla da giustificare. */
static void chase_explain(const SetNode *n, int coord, const Chase *c) {
    if (n->kind == SET_VAR || n->kind == SET_EMPTY) return;
    int ca = n->kind == SET_PRODUCT ? 0 : coord, cb = n->kind == SET_PRODUCT ? 1 : coord;
    int a = chase_value(n->a, ca, c), b = chase_value(n->b, cb, c), sign = chase_value(n, coord, c);

    /* Quali operandi servono: uno solo quando basta a decidere. */
    int use_a = 1, use_b = 1;
    if (n->kind == SET_UNION && sign) { use_a = a == 1; use_b = !use_a; }
    if ((n->kind == SET_INTER || n->kind == SET_PRODUCT) && !sign) { use_a = a == 0; use_b = !use_a; }
    if (n->kind == SET_DIFF && !sign) { use_a = a == 0; use_b = !use_a; }

    if (use_a) chase_explain(n->a, ca, c);
    if (use_b) chase_explain(n->b, cb, c);

    /* 16h. I fatti citati e quello che se ne ricava devono essere veri in
     * tutti i mondi possibili. */
    if ((use_a && !worlds_entail(c, (Fact){n->a, ca, a})) || (use_b && !worlds_entail(c, (Fact){n->b, cb, b})) ||
        !worlds_entail(c, (Fact){n, coord, sign}))
        return;
    proof_add("\\item As ");
    if (use_a) put_fact(n->a, ca, a);
    if (use_a && use_b) proof_add(" and ");
    if (use_b) put_fact(n->b, cb, b);
    proof_add(", ");
    put_fact(n, coord, sign);
    proof_add(".\n");
}

/* Prosegue la dimostrazione che il punto, di cui si sa quanto è in c, sta in
 * goal, scrivendone il testo. Ritorna 1 se ogni caso si chiude, 0 se la
 * dimostrazione va abbandonata.
 *
 * È il modo in cui queste dimostrazioni si scrivono a mano:
 *
 *   16d. Si scompongono i fatti noti con le definizioni: "x sta in A meno B"
 *        dà "x sta in A" e "x non sta in B". Quando la definizione lascia due
 *        possibilità, come per "x sta in A unito B", si distinguono due casi.
 *   16e. Se ormai si sa che il punto sta in goal, lo si giustifica e il caso è
 *        chiuso.
 *   16f. Altrimenti si usa un'ipotesi A contenuto in B: se il punto sta in A
 *        allora sta in B, e se non sta in B allora non sta in A.
 *   16g. Se nessuna ipotesi dice qualcosa di nuovo, si distinguono due casi
 *        secondo che x stia o no in una lettera di cui non si sa ancora nulla.
 *
 * Un caso si chiude anche quando ciò che si sa è contraddittorio: non può
 * presentarsi. Ogni passo aggiunge un'informazione su un numero finito di
 * lettere, quindi il procedimento termina. */
static int chase(Chase c, const SetNode *goal) {
    if (--chase_budget < 0) return 0;

    /* 16d. Scomposizione dei fatti in attesa. Si prende per primo il fatto
     * più semplice: una lettera, poi un fatto già deciso da ciò che si sa, poi
     * uno che si scompone senza casi. Così i casi si aprono solo quando sono
     * inevitabili, e spesso ciò che si è appreso nel frattempo li rende
     * inutili. */
    while (c.pendings > 0) {
        int pick = 0, best = -1;
        for (int i = 0; i < c.pendings; i++) {
            const SetNode *m = c.pending[i].node;
            int sign = c.pending[i].sign, rank = 0;
            if (m->kind == SET_VAR) rank = 3;
            else if (chase_value(m, c.pending[i].coord, &c) >= 0) rank = 2;
            else if (sign ? m->kind == SET_INTER || m->kind == SET_PRODUCT || m->kind == SET_DIFF : m->kind == SET_UNION) rank = 1;
            if (rank >= best) { best = rank; pick = i; }
        }
        Fact fact = c.pending[pick];
        c.pending[pick] = c.pending[--c.pendings];
        const SetNode *n = fact.node;
        int now = chase_value(n, fact.coord, &c);
        if (now == fact.sign) continue;
        if (now >= 0) {
            /* Ciò che si sa dice il contrario: il caso non può presentarsi. */
            if (!worlds_none(&c)) return 0;
            if (n->kind == SET_EMPTY) {
                proof_add("\\item But no element belongs to $\\emptyset$, so this case cannot occur.\n");
                return 1;
            }
            chase_explain(n, fact.coord, &c);
            proof_add("\\item This contradicts ");
            put_fact(n, fact.coord, fact.sign);
            proof_add(", so this case cannot occur.\n");
            return 1;
        }
        if (n->kind == SET_VAR) {
            c.known[fact.coord] |= 1u << n->var;
            if (fact.sign) c.truth[fact.coord] |= 1u << n->var;
            continue;
        }

        /* Le due possibilità che la definizione lascia aperte, ognuna fatta
         * di uno o due fatti sugli operandi; se è una sola, i suoi fatti
         * valgono insieme. */
        int ca = n->kind == SET_PRODUCT ? 0 : fact.coord, cb = n->kind == SET_PRODUCT ? 1 : fact.coord;
        Fact option[2][2];
        int size[2] = {0, 0}, options = 2;
        if ((fact.sign && (n->kind == SET_INTER || n->kind == SET_PRODUCT || n->kind == SET_DIFF)) ||
            (!fact.sign && n->kind == SET_UNION)) {
            options = 1;
            option[0][size[0]++] = (Fact){n->a, ca, fact.sign};
            option[0][size[0]++] = (Fact){n->b, cb, n->kind == SET_DIFF ? 0 : fact.sign};
        } else if (n->kind == SET_SYMDIFF) {
            option[0][size[0]++] = (Fact){n->a, ca, 1};
            option[0][size[0]++] = (Fact){n->b, cb, !fact.sign};
            option[1][size[1]++] = (Fact){n->a, ca, 0};
            option[1][size[1]++] = (Fact){n->b, cb, fact.sign};
        } else {
            option[0][size[0]++] = (Fact){n->a, ca, fact.sign};
            option[1][size[1]++] = (Fact){n->b, cb, n->kind == SET_DIFF ? 1 : fact.sign};
        }

        /* Una possibilità già smentita da ciò che si sa non va considerata. */
        int open[2] = {0, 0}, opens = 0;
        for (int i = 0; i < options; i++) {
            open[i] = 1;
            for (int j = 0; j < size[i]; j++)
                if (chase_value(option[i][j].node, option[i][j].coord, &c) == !option[i][j].sign) open[i] = 0;
            opens += open[i];
        }
        if (opens == 0) {
            if (!worlds_none(&c)) return 0;
            proof_add("\\item But ");
            put_fact(n, fact.coord, fact.sign);
            proof_add(" is impossible here, so this case cannot occur.\n");
            return 1;
        }
        if (opens == 1) {
            int only = open[0] ? 0 : 1;
            for (int j = 0; j < size[only]; j++)
                if (!worlds_entail(&c, option[only][j])) return 0;
            proof_add("\\item Then ");
            for (int j = 0; j < size[only]; j++) {
                if (j) proof_add(" and ");
                put_fact(option[only][j].node, option[only][j].coord, option[only][j].sign);
                chase_push(&c, option[only][j].node, option[only][j].coord, option[only][j].sign);
            }
            proof_add(".\n");
            continue;
        }

        /* Due casi, ognuno proseguito per conto suo in un elenco rientrato. */
        if (chase_depth == 3) return 0;

        /* 16h. I due casi devono coprire tutti i mondi possibili. */
        for (unsigned w = 0; w < 256; w++) {
            int covered = 0;
            for (int i = 0; i < 2 && !covered; i++) {
                covered = 1;
                for (int j = 0; j < size[i]; j++)
                    if (!fact_in_world(option[i][j], w >> 4, w & 15)) covered = 0;
            }
            if (world_alive(&c, w) && !covered) return step_unjustified();
        }
        proof_add("\\item Two cases are possible.\n");
        for (int i = 0; i < 2; i++) {
            Chase branch = c;
            proof_add("\\item \\emph{Case} ");
            for (int j = 0; j < size[i]; j++) {
                if (j) proof_add(" and ");
                put_fact(option[i][j].node, option[i][j].coord, option[i][j].sign);
                chase_push(&branch, option[i][j].node, option[i][j].coord, option[i][j].sign);
                worlds_assume(&branch, option[i][j]);
            }
            proof_add(":\n\\begin{itemize}\n");
            branch.list_start = proof_length;
            chase_depth++;
            int closed = chase(branch, goal);
            chase_depth--;
            if (!closed) return 0;
            proof_add("\\end{itemize}\n");
        }
        return 1;
    }

    /* 16e. La tesi è raggiunta. Se non c'è nulla da giustificare, perché la
     * tesi è tra i fatti già noti, e l'elenco dei passaggi è ancora vuoto, la
     * si enuncia comunque: in LaTeX un elenco non può restare senza voci. */
    if (chase_value(goal, 0, &c) == 1) {
        chase_explain(goal, 0, &c);
        if (!worlds_entail(&c, (Fact){goal, 0, 1})) return 0;
        if (proof_length == c.list_start) {
            proof_add("\\item So ");
            put_fact(goal, 0, 1);
            proof_add(".\n");
        }
        return 1;
    }

    /* 16f. Un'ipotesi che aggiunge un fatto nuovo. */
    for (int i = 0; i < hypotheses; i++) {
        const SetNode *sub = hypothesis_sub[i], *sup = hypothesis_sup[i];
        int inside = chase_value(sub, 0, &c), above = chase_value(sup, 0, &c);
        int forward = inside == 1 && above != 1, backward = above == 0 && inside != 0;
        if (!forward && !backward) continue;
        chase_explain(forward ? sub : sup, 0, &c);
        if (!worlds_entail(&c, (Fact){forward ? sup : sub, 0, forward})) return 0;
        proof_add("\\item Since $");
        put_set(sub);
        proof_add(" \\subseteq ");
        put_set(sup);
        proof_add("$ by assumption, ");
        put_fact(forward ? sup : sub, 0, forward);
        proof_add(".\n");
        chase_push(&c, forward ? sup : sub, 0, forward);
        return chase(c, goal);
    }

    /* 16g. Due casi su una lettera di cui non si sa ancora nulla. */
    for (int coord = 0; coord < 2; coord++)
        for (int var = 0; var < set_var_count; var++) {
            if (!((chase_letters[coord] >> var) & 1) || ((c.known[coord] >> var) & 1)) continue;
            if (chase_depth == 3) return 0;
            proof_add("\\item Two cases are possible.\n");
            for (int sign = 1; sign >= 0; sign--) {
                Chase branch = c;
                SetNode letter = {SET_VAR, TY_SET, var, NULL, NULL, NULL, 0};
                branch.known[coord] |= 1u << var;
                if (sign) branch.truth[coord] |= 1u << var;
                worlds_assume(&branch, (Fact){&letter, coord, sign});
                proof_add("\\item \\emph{Case} $%s %s %c$:\n\\begin{itemize}\n",
                          coord ? "y" : "x", sign ? "\\in" : "\\notin", set_var_name[var]);
                branch.list_start = proof_length;
                chase_depth++;
                int closed = chase(branch, goal);
                chase_depth--;
                if (!closed) return 0;
                proof_add("\\end{itemize}\n");
            }
            return 1;
        }
    return 0;
}

/* Ritorna le lettere che compaiono nel sotto-albero, un bit per lettera. */
static unsigned set_letters(const SetNode *n) {
    if (!n) return 0;
    return (n->kind == SET_VAR ? 1u << n->var : 0) | set_letters(n->a) | set_letters(n->b);
}

/* Scrive la dimostrazione che ogni punto di sub sta in sup, con le ipotesi
 * correnti. Ritorna 0 se va abbandonata. L'insieme vuoto è contenuto in ogni
 * insieme, e in quel caso non c'è nulla da scrivere. converse diverso da zero
 * fa cominciare il testo con "Conversely": è la seconda metà di
 * un'uguaglianza. */
static int narrate_inclusion(const SetNode *sub, const SetNode *sup, int converse) {
    if (sub->kind == SET_EMPTY) return 1;
    int pairs = sub->type == TY_PAIRS || sup->type == TY_PAIRS;
    Chase start = {0};

    /* Le lettere su cui distinguere i casi: per le coppie quelle dei primi e
     * dei secondi fattori, per gli elementi quelle dei due membri e delle
     * ipotesi. */
    chase_letters[0] = chase_letters[1] = 0;
    if (pairs) {
        product_factors(sub, &chase_letters[0], &chase_letters[1], NULL);
        product_factors(sup, &chase_letters[0], &chase_letters[1], NULL);
    } else {
        chase_letters[0] = set_letters(sub) | set_letters(sup);
        for (int i = 0; i < hypotheses; i++) chase_letters[0] |= set_letters(hypothesis_sub[i]) | set_letters(hypothesis_sup[i]);
    }

    proof_add("%s $%s \\in ", sup->kind == SET_EMPTY ? (converse ? "Conversely, suppose that" : "Suppose that")
                                                       : (converse ? "Conversely, let" : "Let"), pairs ? "(x, y)" : "x");
    put_set(sub);
    proof_add("$.\n\\begin{itemize}\n");
    start.list_start = proof_length;
    chase_push(&start, sub, 0, 1);

    /* 16h. I mondi possibili all'inizio: quelli che rispettano le ipotesi,
     * ristretti poi a quelli in cui il punto sta in sub. */
    unsigned regions = 1u << set_var_count;
    for (unsigned u = 0; u < regions; u++)
        for (unsigned v = 0; v < regions; v++) {
            int possible = 1;
            for (int i = 0; i < hypotheses; i++)
                if (fact_in_world((Fact){hypothesis_sub[i], 0, 1}, u, v) && !fact_in_world((Fact){hypothesis_sup[i], 0, 1}, u, v))
                    possible = 0;
            if (possible) start.alive[(16 * u + v) / 64] |= 1ULL << ((16 * u + v) % 64);
        }
    worlds_assume(&start, (Fact){sub, 0, 1});
    if (!chase(start, sup)) return 0;
    proof_add("\\end{itemize}\n");
    if (sup->kind == SET_EMPTY) {
        proof_add("So there is no such element.\n\n");
    } else {
        proof_add("Hence $");
        put_set(sub);
        proof_add(" \\subseteq ");
        put_set(sup);
        proof_add("$.\n\n");
    }
    return 1;
}

/* Scrive la dimostrazione di tutte le relazioni della congiunzione n: di ogni
 * inclusione direttamente, di ogni uguaglianza con le due inclusioni. */
static int narrate_goals(const SetNode *n) {
    if (n->kind == SET_AND) return narrate_goals(n->a) && narrate_goals(n->b);
    if (!narrate_inclusion(n->a, n->b, 0)) return 0;
    if (n->kind == SET_SUBSET) return 1;
    if (!narrate_inclusion(n->b, n->a, n->a->kind != SET_EMPTY)) return 0;
    proof_add("Therefore $");
    put_set(n);
    proof_add("$.\n\n");
    return 1;
}

/* Aggiunge alle ipotesi le relazioni della congiunzione n: un'inclusione così
 * com'è, un'uguaglianza come due inclusioni. */
static void add_hypotheses(const SetNode *n) {
    if (n->kind == SET_AND) { add_hypotheses(n->a); add_hypotheses(n->b); return; }
    for (int way = 0; way < (n->kind == SET_EQ ? 2 : 1) && hypotheses < 16; way++) {
        hypothesis_sub[hypotheses] = way ? n->b : n->a;
        hypothesis_sup[hypotheses++] = way ? n->a : n->b;
    }
}

/* Ritorna 1 se n è una congiunzione di inclusioni e uguaglianze tra insiemi di
 * elementi; con pairs diverso da zero sono ammessi anche gli insiemi di
 * coppie. */
static int set_is_plain(const SetNode *n, int pairs) {
    if (n->kind == SET_AND) return set_is_plain(n->a, pairs) && set_is_plain(n->b, pairs);
    if (n->kind != SET_EQ && n->kind != SET_SUBSET) return 0;
    SetType type = set_join(n->a->type, n->b->type);
    return type == TY_SET || type == TY_ANY || (pairs && type == TY_PAIRS);
}

/* Forma discorsiva, quella preferita: se root è un'inclusione o
 * un'uguaglianza, oppure un'implicazione o un'equivalenza tra condizioni di
 * quel tipo (anche sotto ipotesi dello stesso tipo), scrive la dimostrazione
 * per inseguimento degli elementi e ritorna 1. Altrimenti, o se i casi da
 * distinguere sono troppi, non lascia nulla e ritorna 0.
 *
 * Con delle ipotesi sono ammessi solo insiemi di elementi: le ipotesi vengono
 * applicate al punto x, non a una coppia. */
static int put_narrative_proof(const SetNode *root) {
    const SetNode *given = NULL, *core = root;
    if (root->kind == SET_IMPLIES) { given = root->a; core = root->b; }
    int equivalence = core->kind == SET_IFF;
    if (given && !set_is_plain(given, 0)) return 0;
    if (equivalence ? !set_is_plain(core->a, 0) || !set_is_plain(core->b, 0) : !set_is_plain(core, !given)) return 0;

    hypotheses = 0;
    chase_budget = 300;
    chase_depth = 0;
    proof_add("\\textbf{Proof.} ");
    if (given) {
        add_hypotheses(given);
        proof_add("Assume $");
        put_set(given);
        proof_add("$.\n\n");
    }
    int complete;
    if (!equivalence) {
        complete = narrate_goals(core);
    } else {
        /* Le due implicazioni, ognuna con il proprio lato come ipotesi. */
        int kept = hypotheses;
        add_hypotheses(core->a);
        proof_add("\\smallskip ($\\Rightarrow$) Assume that $");
        put_set(core->a);
        proof_add("$.\n\n");
        complete = narrate_goals(core->b);
        hypotheses = kept;
        add_hypotheses(core->b);
        proof_add("\\smallskip ($\\Leftarrow$) Conversely, assume that $");
        put_set(core->b);
        proof_add("$.\n\n");
        complete = complete && narrate_goals(core->a);
    }
    if (!complete || chase_budget < 0) { proof_reset(); return 0; }
    proof_add("\\hfill $\\Box$");
    return 1;
}

/* Il controesempio di un enunciato falso, come testo: vale fino al verdetto
 * successivo. */
static char counterexample_text[256];

/* Costruisce con configuration_sets() gli insiemi che realizzano la
 * configurazione nonempty e ne scrive la descrizione, per esempio
 * "S = {1, 2}, T = {}", in counterexample_text. bound, se non è -1, è
 * l'indice di un insieme da non riportare. */
static void counterexample(unsigned nonempty, int bound, unsigned *value, int *size) {
    configuration_sets(nonempty, value, size);

    size_t length = 0;
    counterexample_text[0] = 0;
    for (int i = 0; i < set_var_count; i++) {
        if (i == bound) continue;
        length += (size_t)snprintf(counterexample_text + length, sizeof counterexample_text - length,
                                   "%s%c = {", length ? ", " : "", set_var_name[i]);
        for (int e = 0, printed = 0; e < *size && length < sizeof counterexample_text; e++)
            if ((value[i] >> e) & 1)
                length += (size_t)snprintf(counterexample_text + length, sizeof counterexample_text - length,
                                           "%s%d", printed++ ? ", " : "", e + 1);
        if (length < sizeof counterexample_text)
            length += (size_t)snprintf(counterexample_text + length, sizeof counterexample_text - length, "}");
    }
}

/* 16i. Controllo dell'insieme scelto per un "esiste" (15e): l'unione delle
 * regioni di mask, per la lettera del quantificatore exists. Per ogni scelta
 * degli altri insiemi tra i sottoinsiemi di un universo di size elementi si
 * costruisce davvero quell'insieme e si calcola la condizione del
 * quantificatore; se other non è NULL basta che valga quando vale other.
 * Ritorna 1 se regge sempre; altrimenti annota il passo come non giustificato
 * e ritorna 0. */
static int witness_confirmed(const SetNode *exists, const SetNode *other, unsigned mask, int size) {
    unsigned value[MAX_SET_VARS] = {0};
    for (;;) {
        /* Un elemento sta nell'insieme scelto se la sua regione rispetto agli
         * altri insiemi è una di quelle di mask. In quella regione gli altri
         * insiemi occupano i bit in ordine, saltando la lettera di exists. */
        value[exists->var] = 0;
        for (int e = 0; e < size; e++) {
            unsigned region = 0;
            for (int i = 0, place = 0; i < set_var_count; i++)
                if (i != exists->var) region |= ((value[i] >> e) & 1) << place++;
            if ((mask >> region) & 1) value[exists->var] |= 1u << e;
        }
        if ((!other || concrete_holds(other, value, size)) && !concrete_holds(exists->a, value, size)) {
            unjustified_steps++;
            return 0;
        }

        /* Assegnazione successiva degli altri insiemi, come in
         * concrete_valid(). */
        int i = 0;
        for (; i < set_var_count; i++) {
            if (i == exists->var) continue;
            if (++value[i] < (1u << size)) break;
            value[i] = 0;
        }
        if (i == set_var_count) return 1;
    }
}

/* Decide l'enunciato sugli insiemi scritto in formula e ne ritorna il
 * verdetto, con la dimostrazione scritta o il controesempio.
 *
 * Svuota l'arena: i risultati di chiamate precedenti non sono più validi. */
static Verdict prove_sets(const char *formula) {
    Verdict verdict = {UNSUPPORTED, 0, 0, 0, NULL, NULL, NULL, "per ogni scelta degli insiemi", NULL, NULL};

    /* Ripartenza da zero e parsing. */
    proof_reset();
    arena_used = arena_keep;
    failure = NULL;
    set_var_count = 0;
    syntax_error = NULL;
    cursor = formula;
    next_token();
    SetNode *root = set_formula();
    if (token.kind != T_END) syntax("testo inatteso dopo l'enunciato");
    if (syntax_error) { verdict.reason = syntax_error; return verdict; }
    if (failure) { verdict.reason = failure; return verdict; }
    if (set_check(root) != TY_FORMULA) {
        verdict.reason = "enunciato sugli insiemi mal formato: operandi di tipo diverso, o manca una relazione";
        return verdict;
    }
    set_prepare(root);
    if (failure) { verdict.reason = failure; return verdict; }

    /* Il quantificatore, se c'è: uno solo, e solo come enunciato intero o come
     * uno dei due lati di un'equivalenza o di un'implicazione. */
    const SetNode *exists = NULL, *other = NULL;
    int as_hypothesis = 0, as_thesis = 0;
    if (root->kind == SET_EXISTS) {
        exists = root;
        as_thesis = 1;
    } else if (root->kind == SET_IFF || root->kind == SET_IMPLIES) {
        if (root->a->kind == SET_EXISTS) { exists = root->a; other = root->b; as_hypothesis = 1; as_thesis = root->kind == SET_IFF; }
        else if (root->b->kind == SET_EXISTS) { exists = root->b; other = root->a; as_thesis = 1; as_hypothesis = root->kind == SET_IFF; }
    }
    int quantifiers = 0;
    for (const SetNode *stack[64], **top = stack, *n = root; n || top > stack; ) {
        if (!n) { n = *--top; continue; }
        if (n->kind == SET_EXISTS) quantifiers++;
        if (n->b && top < stack + 64) *top++ = n->b;
        n = n->a;
    }
    if (quantifiers > (exists ? 1 : 0)) {
        verdict.reason = "quantificatore in una posizione non trattata";
        return verdict;
    }

    /* Decisione per regioni (capitolo 15). */
    unsigned all = (1u << (1u << set_var_count)) - 1, failing = 0, mask = 0;
    int valid, found = 1;
    if (!exists) {
        valid = set_valid(root, all, &failing);
    } else {
        /* 15d: "esiste V" come ipotesi, con V arbitrario. */
        SetNode to_other = {SET_IMPLIES, TY_FORMULA, 0, (SetNode *)exists, (SetNode *)other, NULL, 0};
        SetNode to_exists = {SET_IMPLIES, TY_FORMULA, 0, (SetNode *)other, (SetNode *)exists, NULL, 0};
        valid = !as_hypothesis || set_valid(&to_other, all, &failing);

        /* 15e: "esiste V" come tesi, cercando V tra le unioni di regioni. */
        if (valid && as_thesis) {
            unsigned ignored = 0, candidates = 1u << (1u << (set_var_count - 1));
            const SetNode *goal = other ? &to_exists : exists;
            found = 0;
            for (mask = 0; mask < candidates && !found; mask++)
                found = set_valid(goal, witness_regions(exists->var, mask), &ignored);
            mask--;
        }
    }
    if (valid && !found) {
        verdict.outcome = NOT_PROVED;
        verdict.reason = "non trovo, tra le unioni di regioni, un insieme che soddisfi la condizione di esistenza";
        return verdict;
    }

    /* Enunciato falso: il controesempio, ricontrollato calcolando davvero. */
    if (!valid) {
        unsigned value[MAX_SET_VARS];
        int size;
        counterexample(failing, exists ? exists->var : -1, value, &size);
        if (size > 5 || concrete_holds(root, value, size)) {
            verdict.outcome = NOT_PROVED;
            verdict.reason = "la verifica indipendente non conferma il controesempio";
            return verdict;
        }
        proof_add("The statement is false. Counterexample: $");
        for (int i = 0, printed = 0; i < set_var_count; i++) {
            if (exists && i == exists->var) continue;
            proof_add("%s%c = ", printed++ ? ",\\ " : "", set_var_name[i]);
            if (!value[i]) { proof_add("\\emptyset"); continue; }
            proof_add("\\{");
            for (int e = 0, listed = 0; e < size; e++)
                if ((value[i] >> e) & 1) proof_add("%s%d", listed++ ? ", " : "", e + 1);
            proof_add("\\}");
        }
        proof_add("$.");
        verdict.outcome = REFUTED;
        verdict.detail = counterexample_text;
        verdict.proof = proof_text;
        return verdict;
    }

    /* Enunciato vero: verifica indipendente su un universo piccolo. Con più
     * insiemi l'universo è più piccolo, perché le assegnazioni restino
     * poche migliaia. */
    int free_sets = set_var_count - (exists ? 1 : 0), universe = free_sets >= 4 ? 3 : 4;
    if (!concrete_valid(root, exists ? exists->var : -1, universe)) {
        verdict.outcome = NOT_PROVED;
        verdict.reason = "la verifica indipendente ha trovato un caso in cui l'enunciato non vale";
        return verdict;
    }

    /* Dimostrazione scritta (capitolo 16), con il controllo di ciò che
     * afferma: se nemmeno la forma generale lo supera, non c'è un testo di
     * cui fidarsi e il verdetto non viene emesso. */
    int written = 1;
    if (exists || (!put_narrative_proof(root) && !put_table_proof(root) && !put_comparison_proof(root)))
        written = (!as_thesis || witness_confirmed(exists, other, mask, universe)) &&
                  put_general_proof(root, as_thesis ? exists : NULL, mask);
    if (!written) {
        verdict.outcome = NOT_PROVED;
        verdict.reason = "la dimostrazione scritta non ha superato il controllo dei suoi passi";
        return verdict;
    }
    verdict.outcome = PROVED;
    verdict.proof = proof_text;
    return verdict;
}

/* =============================================================================
 * 17. QUANTIFICATORI: NOTAZIONE E ALBERO
 * ============================================================================
 *
 * Terza famiglia di enunciati: quelli che dicono qualcosa di tutti i numeri di
 * un insieme, o dell'esistenza di un numero, per esempio
 *
 *     \forall a, b \in \mathbb{N}_{0}\ (a^{2} = b^{2} \implies a = b)
 *     \lnot (\forall y \in \mathbb{Z}\ \exists x \in \mathbb{Z}\ (2x = y))
 *
 * Tra le ipotesi un enunciato può dichiarare un'applicazione e darne la
 * regola; la tesi parla poi di quell'applicazione:
 *
 *     (f \colon \mathbb{Z} \to \mathbb{Z} \land \forall n \in \mathbb{Z}\ (f(n) = n + 5))
 *         \implies \forall y \in \mathbb{Z}\ \exists x \in \mathbb{Z}\ (f(x) = y)
 *
 * È il modo in cui si scrive "f è suriettiva": per esteso, con la sua
 * definizione. Il motore non conosce parole come iniettiva o suriettiva: vede
 * solo quantificatori, connettivi e confronti, e lavora su quelli.
 *
 * La grammatica, dalla precedenza più bassa alla più alta:
 *
 *     formula     := implication { \iff implication }
 *     implication := disjunction [ \implies implication ]
 *     disjunction := conjunction { \lor conjunction }
 *     conjunction := unary { \land unary }
 *     unary       := \lnot unary
 *                  | (\forall | \exists) lettera { ',' lettera } \in numbers unary
 *                  | lettera \colon numbers \to numbers
 *                  | expr ('=' | \neq) expr
 *                  | '(' formula ')'
 *     numbers     := (\mathbb{N} | \mathbb{N}_{0} | \mathbb{Z} | \mathbb{Q} | \mathbb{Q}^{+})
 *                    { \setminus \{ numero { ',' numero } \} }
 *
 * expr è l'espressione del capitolo 9, che riconosce anche f(x) quando f è
 * un'applicazione dichiarata.
 *
 * Gli insiemi sono tutti fatti di numeri razionali, e non per caso: la regola
 * 18b, che esclude gli zeri di un polinomio, trova solo le radici razionali.
 * Tra i numeri reali x^2 = 2 ha soluzioni che quella regola non vede, quindi
 * \mathbb{R} non è accettato finché non c'è un metodo adatto.
 *
 * Dopo il parsing, formula_check() controlla ciò che la grammatica non dice:
 * che ogni lettera sia quantificata, e una volta sola; che le espressioni
 * siano razionali; che un'applicazione sia applicata a una variabile del suo
 * dominio.
 */

/* Ritorna 1 se il numero value appartiene a set. */
static int numbers_contain(const NumberSet *set, Frac value) {
    if (set->integers && value.den != 1) return 0;
    if (set->bounded) {
        int side = frac_cmp(value, frac_int(set->low));
        if (side < 0 || (side == 0 && set->strict)) return 0;
    }
    for (int i = 0; i < set->holes; i++)
        if (frac_cmp(value, set->hole[i]) == 0) return 0;
    return 1;
}

/* Ritorna 1 se a e b sono lo stesso insieme. Per i punti tolti si conta
 * sull'invariante di NumberSet: a parità di numero, basta che quelli di a
 * manchino anche in b. */
static int numbers_equal(const NumberSet *a, const NumberSet *b) {
    if (a->integers != b->integers || a->bounded != b->bounded || a->holes != b->holes) return 0;
    if (a->bounded && (a->low != b->low || a->strict != b->strict)) return 0;
    for (int i = 0; i < a->holes; i++)
        if (numbers_contain(b, a->hole[i])) return 0;
    return 1;
}

/* Mette in out i primi `wanted` elementi di set, in un ordine fisso che parte
 * dai numeri più semplici. Serve a cercare un testimone (18j) e a fare la
 * verifica su un campione (capitolo 19).
 *
 * L'ordine. Ogni razionale si scrive in un solo modo come p/q ridotta, con
 * q > 0; la sua "altezza" è il più grande tra |p| e q. I numeri di altezza h
 * sono in numero finito, quindi elencarli per altezza crescente li raggiunge
 * tutti: prima 0, poi 1 e -1, poi 2, -2, 1/2, -1/2, e così via. Di ogni
 * candidato si tiene solo ciò che appartiene a set; tra gli interi conta solo
 * q = 1. Gli insiemi sono infiniti, quindi `wanted` elementi si trovano
 * sempre. */
static void numbers_sample(const NumberSet *set, Frac *out, int wanted) {
    int count = 0;
    for (long long h = 0; count < wanted; h++) {
        long long widest = (h > 1 && !set->integers) ? h : 1;
        for (long long q = 1; q <= widest; q++) {
            /* Altezza h vuol dire p = h quando q è più piccolo di h; quando
             * q = h, p va da 1 a h. L'altezza 0 è il solo zero. */
            for (long long p = (q < h) ? h : (h ? 1 : 0); p <= h; p++) {
                if (gcd128(p, q) != 1) continue;
                for (int sign = 1; sign >= (p ? -1 : 1); sign -= 2) {
                    Frac value = frac_make(sign * p, q);
                    if (count < wanted && numbers_contain(set, value)) out[count++] = value;
                }
            }
        }
    }
}

/* Tipi di nodo di un enunciato con quantificatori.
 *
 * Warning: se aggiungi un tipo, aggiungi il ramo che lo costruisce nel parser
 * di questo capitolo, il suo case in formula_check(), in junction() o in
 * establish() (capitolo 18) e in sample_holds() (capitolo 19). */
typedef enum {
    F_EQUAL, F_DISTINCT,              /* left = right, left diverso da right */
    F_AND, F_OR, F_IMPLIES, F_IFF,    /* connettivi tra a e b */
    F_NOT,                            /* non a */
    F_FORALL, F_EXISTS,               /* per ogni / esiste variable in numbers: a */
    F_MAP                             /* dichiarazione di un'applicazione */
} FormulaKind;

/* Un nodo dell'albero di un enunciato con quantificatori: una condizione, cioè
 * qualcosa che può essere vero o falso. I campi non usati dal tipo sono NULL o
 * zero. I nodi vivono nell'arena.
 *
 * Gli ultimi quattro campi sono i soli scritti dopo il parsing: li riempie la
 * dimostrazione (capitolo 18), perché la verifica indipendente (capitolo 19)
 * possa ricontrollare ciò che ha scelto. Quando un "per ogni" è dimostrato
 * per casi sul resto (regola 18o), il valore scelto per un "esiste" al suo
 * interno cambia da un caso all'altro: per questo i valori sono MAX_CASES, e
 * fuori da una dimostrazione per casi si usa il primo. */
typedef struct Formula Formula;
struct Formula {
    FormulaKind kind;
    Node *left, *right;       /* F_EQUAL, F_DISTINCT: i due membri */
    Formula *a, *b;           /* le condizioni componenti */
    int variable;             /* F_FORALL, F_EXISTS: la lettera quantificata */
    NumberSet numbers;        /* F_FORALL, F_EXISTS: l'insieme su cui varia */
    const char *start, *end;  /* com'è scritto nell'enunciato: [start, end) */
    unsigned has_witness;     /* il bit r dice se witness[r] è stato scelto */
    Rat witness[MAX_CASES];   /* il valore scelto per variable nel caso r: un
                                 numero, o un'espressione nelle variabili più
                                 esterne */
    int modulus;              /* F_FORALL dimostrato per casi sul resto: il
                                 modulo m; altrimenti 0 */
    int quotient;             /* e la lettera t di variable = m t + r */
};

/* Una variabile quantificata e l'insieme su cui varia. */
typedef struct {
    int name;                  /* la lettera */
    const NumberSet *numbers;  /* l'insieme: sta nel nodo del quantificatore */
} Bound;

/* Lo stato del lettore di token. Rimetterlo com'era fa riprendere la lettura
 * da un punto precedente: serve dove un solo token non basta a decidere quale
 * regola della grammatica applicare (17c e 17d). I nodi creati nel tentativo
 * abbandonato restano nell'arena, inutilizzati. */
typedef struct {
    const char *cursor;
    Token token;
    const char *syntax_error;
} ParserState;

/* Ritorna lo stato attuale del lettore. */
static ParserState parser_save(void) { return (ParserState){cursor, token, syntax_error}; }

/* Riporta il lettore allo stato state, ottenuto da parser_save(). */
static void parser_restore(ParserState state) {
    cursor = state.cursor;
    token = state.token;
    syntax_error = state.syntax_error;
}

/* Ritorna un nuovo nodo di tipo kind con componenti a e b, il cui testo va da
 * start alla fine dell'ultimo token già letto: va quindi chiamata dopo aver
 * letto tutto il pezzo di enunciato che il nodo rappresenta. Come node_new(),
 * se l'arena è piena ritorna un nodo di riserva. */
static Formula *formula_node(FormulaKind kind, Formula *a, Formula *b, const char *start) {
    static Formula spare;
    Formula *node = arena_alloc(sizeof(Formula));
    if (!node) node = &spare;
    memset(node, 0, sizeof *node);
    node->kind = kind;
    node->a = a;
    node->b = b;
    node->start = start;
    node->end = consumed;
    return node;
}

/* numbers: vedi la grammatica in testa al capitolo. */
static NumberSet parse_numbers(void) {
    NumberSet set = {0};
    set.start = token.start;

    /* La lettera tra graffe. */
    int letter = 0;
    if (!is_command("mathbb")) syntax("serve un insieme numerico, come \\mathbb{N} o \\mathbb{Q}");
    next_token();
    expect_char('{', "dopo \\mathbb serve una lettera tra graffe");
    if (token.kind == T_LETTER) letter = token.ch;
    next_token();
    expect_char('}', "dopo \\mathbb serve una lettera tra graffe");

    /* Il pedice 0 oppure l'apice +, con o senza graffe. */
    int zero = 0, plus = 0;
    if (is_char('_') || is_char('^')) {
        int subscript = is_char('_');
        next_token();
        int braces = is_char('{');
        if (braces) next_token();
        if (subscript && token.kind == T_NUM && token.value == 0) zero = 1;
        else if (!subscript && is_char('+')) plus = 1;
        else syntax("dopo un insieme numerico sono riconosciuti solo il pedice 0 e l'apice +");
        next_token();
        if (braces) expect_char('}', "manca }");
    }

    /* Quale insieme è. \mathbb{N} parte da 1, \mathbb{N}_{0} da 0. */
    if (letter == 'N' && !plus) { set.integers = 1; set.bounded = 1; set.low = zero ? 0 : 1; }
    else if (letter == 'Z' && !zero && !plus) set.integers = 1;
    else if (letter == 'Q' && !zero) set.bounded = set.strict = plus;
    else syntax("insieme numerico non trattato: sono ammessi \\mathbb{N}, \\mathbb{N}_{0}, \\mathbb{Z}, \\mathbb{Q} e \\mathbb{Q}^{+}");

    /* I punti tolti. Devono appartenere all'insieme ed essere distinti: è
     * l'invariante di NumberSet. */
    while (is_command("setminus")) {
        next_token();
        expect_char(ESCAPED + '{', "dopo \\setminus servono dei numeri tra \\{ e \\}");
        for (;;) {
            Num value;
            Node *element = parse_expr();
            if (syntax_error) break;
            if (!rat_number(eval(element), &value) || value.im.num != 0) { syntax("tra \\{ e \\} servono dei numeri"); break; }
            if (set.holes == MAX_HOLES || !numbers_contain(&set, value.re)) {
                syntax("i punti tolti con \\setminus devono appartenere all'insieme, essere distinti e non più di 4");
                break;
            }
            set.hole[set.holes++] = value.re;
            if (!is_char(',')) break;
            next_token();
        }
        expect_char(ESCAPED + '}', "manca \\}");
    }
    set.end = consumed;

    /* Un insieme di interi privato del suo primo elemento comincia da quello
     * dopo: \mathbb{N} \setminus \{1\} sono gli interi da 2 in su. Scriverlo
     * così, e non come punto tolto, serve alla regola dei segni (18c), che
     * ragiona a partire dall'estremo inferiore: altrimenti "n - 1 è almeno 1"
     * non risulterebbe. Dopo ogni spostamento si ricontrollano tutti i punti. */
    for (int i = 0; set.integers && set.bounded && i < set.holes; ) {
        if (frac_cmp(set.hole[i], frac_int(set.low)) != 0) { i++; continue; }
        set.hole[i] = set.hole[--set.holes];
        set.low++;
        i = 0;
    }
    return set;
}

/* Prototipo in avanti inevitabile: la grammatica è ricorsiva. */
static Formula *logic_formula(void);

/* unary: vedi la grammatica in testa al capitolo. */
static Formula *logic_unary(void) {
    const char *start = token.start;

    /* 17a. Negazione. */
    if (is_command("lnot") || is_command("neg")) {
        next_token();
        Formula *inner = logic_unary();
        return formula_node(F_NOT, inner, NULL, start);
    }

    /* 17b. Quantificatore: le lettere, l'insieme, la condizione. */
    if (is_command("forall") || is_command("exists")) {
        FormulaKind kind = is_command("forall") ? F_FORALL : F_EXISTS;
        int name[MAX_BOUND], names = 0;
        do {
            next_token();  /* supera il quantificatore, poi ogni virgola */
            if (token.kind != T_LETTER || names == MAX_BOUND) {
                syntax("dopo un quantificatore servono delle lettere separate da virgole, non più di 8");
                break;
            }
            name[names++] = token.ch;
            next_token();
        } while (is_char(','));
        if (!is_command("in")) syntax("dopo le lettere di un quantificatore servono \\in e un insieme numerico");
        next_token();
        NumberSet numbers = parse_numbers();
        Formula *node = logic_unary();

        /* \forall a, b è \forall a \forall b: un nodo per lettera, costruiti
         * dall'ultima lettera alla prima. */
        for (int i = names - 1; i >= 0; i--) {
            node = formula_node(kind, node, NULL, start);
            node->variable = name[i];
            node->numbers = numbers;
        }
        return node;
    }

    /* 17c. Dichiarazione di un'applicazione. Si distingue da un confronto che
     * comincia con una lettera solo al token dopo, \colon: si guarda avanti
     * e, se non c'è, si torna indietro. */
    if (token.kind == T_LETTER) {
        ParserState before = parser_save();
        int name = token.ch;
        next_token();
        if (is_command("colon")) {
            next_token();
            NumberSet domain = parse_numbers();
            if (is_command("to") || is_command("rightarrow") || is_command("longrightarrow")) next_token();
            else syntax("in una dichiarazione f \\colon A \\to B manca \\to");
            NumberSet codomain = parse_numbers();
            if (map_find(name)) syntax("la stessa applicazione è dichiarata due volte");
            else if (map_count == MAX_MAPS) syntax("troppe applicazioni dichiarate in un enunciato");
            else if (!syntax_error) maps[map_count++] = (Map){name, domain, codomain, 0, NULL};
            return formula_node(F_MAP, NULL, NULL, start);
        }
        parser_restore(before);
    }

    /* 17d. Confronto tra due espressioni. Anche un confronto può cominciare
     * con una parentesi, come (n + 1)^{2} = 4, quindi prima si prova a leggere
     * un'espressione seguita da = o \neq; se non riesce, si torna indietro e
     * la parentesi deve aprire una formula (17e). */
    ParserState before = parser_save();
    Node *left = parse_expr();
    const char *problem = syntax_error;
    if (!problem && (is_char('=') || is_command("neq") || is_command("ne"))) {
        FormulaKind kind = is_char('=') ? F_EQUAL : F_DISTINCT;
        next_token();
        Node *right = parse_expr();
        Formula *node = formula_node(kind, NULL, NULL, start);
        node->left = left;
        node->right = right;
        return node;
    }
    parser_restore(before);

    /* 17e. Formula tra parentesi. */
    if (token.kind == T_OPEN) {
        next_token();
        Formula *inner = logic_formula();
        if (token.kind == T_CLOSE) next_token(); else syntax("manca la parentesi chiusa");
        return inner;
    }

    /* 17f. Nient'altro è una condizione. Se l'espressione era illeggibile si
     * riporta quell'errore, che è il più preciso. */
    syntax(problem ? problem : "dopo un'espressione serve = oppure \\neq: le altre relazioni non sono ancora trattate");
    Formula *nothing = formula_node(F_EQUAL, NULL, NULL, start);
    nothing->left = nothing->right = node_num(0);
    return nothing;
}

/* conjunction := unary { \land unary } */
static Formula *logic_conjunction(void) {
    const char *start = token.start;
    Formula *node = logic_unary();
    while (is_command("land") || is_command("wedge")) {
        next_token();
        Formula *right = logic_unary();
        node = formula_node(F_AND, node, right, start);
    }
    return node;
}

/* disjunction := conjunction { \lor conjunction } */
static Formula *logic_disjunction(void) {
    const char *start = token.start;
    Formula *node = logic_conjunction();
    while (is_command("lor") || is_command("vee")) {
        next_token();
        Formula *right = logic_conjunction();
        node = formula_node(F_OR, node, right, start);
    }
    return node;
}

/* implication := disjunction [ \implies implication ]
 *
 * Associa a destra, come per gli insiemi (capitolo 14). */
static Formula *logic_implication(void) {
    const char *start = token.start;
    Formula *node = logic_disjunction();
    if (is_command("implies") || is_command("Rightarrow") || is_command("Longrightarrow")) {
        next_token();
        Formula *right = logic_implication();
        node = formula_node(F_IMPLIES, node, right, start);
    }
    return node;
}

/* formula := implication { \iff implication } */
static Formula *logic_formula(void) {
    const char *start = token.start;
    Formula *node = logic_implication();
    while (is_command("iff") || is_command("Leftrightarrow") || is_command("Longleftrightarrow")) {
        next_token();
        Formula *right = logic_implication();
        node = formula_node(F_IFF, node, right, start);
    }
    return node;
}

/* Le variabili quantificate visibili nel punto dell'enunciato che
 * formula_check() sta esaminando, dalla più esterna. */
static Bound scope[MAX_BOUND];
static int scope_count;

/* Il maggior numero di quantificatori uno dentro l'altro incontrato da
 * formula_check() da quando è stato azzerato. Serve alla verifica sul
 * campione (capitolo 19) per decidere quanti valori provare. */
static int quantifier_depth;

/* Ritorna la variabile quantificata di nome name visibile in questo punto, o
 * NULL se non c'è. */
static const Bound *scope_find(int name) {
    for (int i = 0; i < scope_count; i++)
        if (scope[i].name == name) return &scope[i];
    return NULL;
}

/* Controlla che nell'espressione node ogni lettera abbia un significato;
 * altrimenti annota il motivo con fail().
 *
 * Sono ammesse tutte le espressioni del capitolo 9: le regole del capitolo 18
 * lavorano sui polinomi e lasciano stare ciò che non lo è, mentre l'induzione
 * (18n) e i conti con i numeri trattano anche somme, potenze e funzioni. Una
 * lettera deve essere una variabile quantificata, oppure l'indice di una
 * somma dentro il suo termine. Un'applicazione deve avere per argomento una
 * variabile che varia nel suo dominio: è questo a garantire che f(x) sia
 * definito ogni volta che compare, senza doverlo ridimostrare. */
static void term_check(const Node *node) {
    if (!node || failure) return;
    switch (node->kind) {
    case N_NUM:
        return;
    case N_VAR:
        if (node->name != VAR_PI && !scope_find(node->name)) fail("negli enunciati con quantificatori ogni lettera deve essere quantificata");
        return;
    case N_APPLY: {
        const Map *map = map_find(node->name);
        const Bound *argument = node->a->kind == N_VAR ? scope_find(node->a->name) : NULL;
        if (!map->body) fail("un'applicazione è usata prima della regola che la definisce");
        else if (!argument || !argument->numbers || !numbers_equal(argument->numbers, &map->domain))
            fail("un'applicazione va applicata a una variabile che varia nel suo dominio");
        return;
    }
    case N_BIGOP:
        /* L'indice vale solo nel termine, e non può chiamarsi come una
         * variabile già in uso: nelle regole una lettera è un atomo solo. */
        term_check(node->a);
        term_check(node->b);
        if (scope_find(node->name) || map_find(node->name)) { fail("la stessa lettera ha due significati nello stesso punto dell'enunciato"); return; }
        if (scope_count == MAX_BOUND) { fail("troppi quantificatori uno dentro l'altro"); return; }
        scope[scope_count++] = (Bound){node->name, NULL};
        term_check(node->c);
        scope_count--;
        return;
    default:
        term_check(node->a);
        term_check(node->b);
    }
}

/* Ritorna 1 se l'espressione node è razionale: numeri, variabili, le quattro
 * operazioni, potenze con esponente intero non negativo scritto in cifre, e
 * applicazioni dichiarate (la cui regola è a sua volta razionale, vedi
 * read_definitions()).
 *
 * È il confine delle regole da 18a a 18h, che ragionano sui polinomi e per
 * cui "definito" vuol dire soltanto "nessun divisore è zero". Per un
 * logaritmo o una radice non basta: lì serve anche il segno di ciò su cui
 * operano, e quelle regole non lo controllano. */
static int term_is_rational(const Node *node) {
    if (!node) return 1;
    switch (node->kind) {
    case N_NUM: case N_APPLY:
        return 1;
    case N_VAR:
        return node->name != VAR_PI;
    case N_NEG:
        return term_is_rational(node->a);
    case N_ADD: case N_SUB: case N_MUL: case N_DIV:
        return term_is_rational(node->a) && term_is_rational(node->b);
    case N_POW:
        return node->b->kind == N_NUM && term_is_rational(node->a);
    default:
        return 0;
    }
}

/* Controlla la condizione f, che non è un'ipotesi: ogni lettera quantificata
 * una volta sola, nessuna dichiarazione, espressioni ammesse da term_check().
 * Annota il primo problema con fail().
 *
 * Perché una lettera non può essere quantificata dentro se stessa. Nelle
 * regole del capitolo 18 una variabile è un atomo, e l'atomo è identificato
 * dalla lettera: due variabili diverse con la stessa lettera, visibili
 * insieme, diventerebbero la stessa. */
static void formula_check(const Formula *f) {
    switch (f->kind) {
    case F_EQUAL: case F_DISTINCT:
        term_check(f->left);
        term_check(f->right);
        return;
    case F_MAP:
        fail("un'applicazione si può dichiarare solo tra le ipotesi");
        return;
    case F_FORALL: case F_EXISTS:
        if (scope_find(f->variable) || map_find(f->variable)) { fail("la stessa lettera ha due significati nello stesso punto dell'enunciato"); return; }
        if (scope_count == MAX_BOUND) { fail("troppi quantificatori uno dentro l'altro"); return; }
        scope[scope_count++] = (Bound){f->variable, &f->numbers};
        if (scope_count > quantifier_depth) quantifier_depth = scope_count;
        formula_check(f->a);
        scope_count--;
        return;
    default:
        formula_check(f->a);
        if (f->b) formula_check(f->b);
    }
}

/* Legge le ipotesi f: una congiunzione di dichiarazioni di applicazioni, già
 * registrate dal parser, e di regole della forma
 *
 *     \forall x \in A\ (f(x) = espressione)
 *
 * dove A è il dominio di f. Completa ogni applicazione con la sua regola;
 * un'ipotesi di altro tipo viene annotata con fail(). */
static void read_definitions(const Formula *f) {
    if (f->kind == F_AND) { read_definitions(f->a); read_definitions(f->b); return; }
    if (f->kind == F_MAP) return;

    /* Riconosce la regola e l'applicazione che definisce. */
    const Formula *rule = f->kind == F_FORALL ? f->a : NULL;
    const Node *applied = rule && rule->kind == F_EQUAL && rule->left->kind == N_APPLY ? rule->left : NULL;
    if (!applied || applied->a->kind != N_VAR || applied->a->name != f->variable) {
        fail("ipotesi non trattata: sono ammesse la dichiarazione di un'applicazione e la sua regola");
        return;
    }
    Map *map = map_find(applied->name);
    if (map->body) { fail("un'applicazione ha due regole"); return; }
    if (!numbers_equal(&f->numbers, &map->domain)) { fail("la regola di un'applicazione deve valere su tutto il suo dominio"); return; }

    /* L'espressione può contenere solo la variabile della regola. La si
     * controlla prima di registrarla: così una regola che cita la propria
     * applicazione trova body ancora NULL e viene respinta da term_check(). */
    scope[0] = (Bound){f->variable, &f->numbers};
    scope_count = 1;
    term_check(rule->right);
    scope_count = 0;

    /* Che la regola dia un valore per ogni elemento del dominio lo dimostra
     * map_defined() (18m), che sa farlo solo guardando i divisori. */
    if (!term_is_rational(rule->right)) { fail("la regola di un'applicazione deve essere un'espressione razionale"); return; }
    map->parameter = f->variable;
    map->body = rule->right;
}

/* =============================================================================
 * 18. QUANTIFICATORI: REGOLE DI DIMOSTRAZIONE
 * ============================================================================
 *
 * establish() dimostra una condizione seguendone la struttura. Ogni caso è una
 * regola di deduzione, e ogni regola scrive nella dimostrazione la frase che
 * la giustifica: il testo finale è la traccia di ciò che il programma ha
 * fatto, non un riassunto scritto dopo.
 *
 * Lo stato di una dimostrazione in corso è fatto di due elenchi di variabili.
 *
 *   - Le variabili "generiche", in universal[]: quelle introdotte con "sia x
 *     un elemento di A" per dimostrare un "per ogni". Nell'algebra restano
 *     lettere, cioè atomi, e ciò che si dimostra deve valere per ogni loro
 *     valore nell'insieme.
 *   - Le variabili a cui è stato dato un valore, nell'ambiente di eval()
 *     (capitolo 11): quelle di un "esiste", legate al numero o
 *     all'espressione scelti.
 *
 * Un confronto tra due espressioni diventa così una frazione di polinomi
 * nelle sole variabili generiche, e tutto si riduce a tre domande su un
 * polinomio: è il polinomio nullo? Si annulla da qualche parte, negli insiemi
 * dati? Che segno ha? Le regole, nell'ordine in cui compaiono:
 *
 *   18a. Una costante non nulla non si annulla.
 *   18b. Un polinomio in una variabile si annulla solo nelle sue radici, e
 *        quelle razionali si trovano tutte.
 *   18c. Regola dei segni: su una semiretta, un polinomio senza coefficienti
 *        negativi non è negativo.
 *   18d. Un polinomio di grado d che è intero in d + 1 interi consecutivi è
 *        intero in tutti gli interi.
 *   18e. Appartenenza a un insieme numerico: intero, non sotto l'estremo
 *        inferiore, diverso dai punti tolti.
 *   18f. Ogni divisione scritta ha il divisore diverso da zero.
 *   18g. Un confronto tra numeri, o tra espressioni identiche, si decide
 *        calcolando.
 *   18h. Fattore comune: se p = e q e q non si annulla, p = 0 implica e = 0.
 *   18i. "Esiste x" con un'equazione in x: di primo grado la si risolve; di
 *        grado più alto, con coefficienti numerici, si provano le sue radici
 *        razionali. Se la condizione è una disgiunzione si prova l'equazione
 *        di ciascuna parte: "esiste x con A o B" vale se esiste x con A.
 *   18j. "Esiste x" negli altri casi: si provano i numeri più semplici.
 *   18k. "Per ogni x": si introduce una variabile generica.
 *   18l. Connettivi: una congiunzione chiede tutte le parti, a una
 *        disgiunzione ne basta una.
 *   18m. La regola di un'applicazione la definisce davvero: il valore esiste
 *        e sta nel codominio.
 *   18n. "Per ogni n" in un insieme di interi da un certo punto in su, di
 *        un'uguaglianza: per induzione, con il metodo del capitolo 13.
 *   18o. "Per ogni x" intero: per casi sul resto di x in una divisione.
 *
 * Le espressioni possono contenere anche somme, potenze e funzioni, come
 * nel capitolo 9. Le regole da 18a a 18h lavorano sui polinomi, e davanti a
 * un'espressione che non lo è semplicemente non si applicano: restano
 * l'induzione (18n) e, quando le variabili hanno un valore, il calcolo.
 *
 * Ogni regola è un'implicazione vera, quindi un "sì" è una dimostrazione. Non
 * sono complete: quando nessuna si applica la risposta è "no", e il verdetto
 * sarà NON DIMOSTRATO.
 *
 * Controllo dei passi. Prima di scrivere un fatto ricavato con un conto, la
 * regola lo ricontrolla per un'altra strada: una scomposizione rimoltiplicando
 * i fattori, una sostituzione tornando indietro, un elenco di radici
 * sostituendole una per una e cercandone di dimenticate tra i primi elementi
 * dell'insieme, una copertura con semirette guardando dove cadono quegli
 * elementi. Se il controllo fallisce la regola non si applica, e il contatore
 * unjustified_steps (capitolo 13) lo registra: così ciò che il testo afferma
 * non dipende da un solo calcolo.
 *
 * La negazione non ha regole sue. Dimostrare "non A" è stabilire A con il
 * segno rovesciato: i due quantificatori si scambiano (non per ogni x vale P
 * equivale a esiste x per cui non vale P), e così congiunzione e
 * disgiunzione. Per questo le funzioni prendono, oltre alla condizione, il
 * parametro `negated`.
 */

/* Le variabili generiche della dimostrazione in corso, dalla più esterna. */
static Bound universal[MAX_BOUND];
static int universal_count;

/* La dimostrazione per casi sul resto in corso (regola 18o): il modulo m, o 0
 * se non ce n'è una, e il resto r del caso che si sta trattando. split_case
 * dice anche quale dei valori di un "esiste" si sta scegliendo, o
 * ricontrollando (vedi Formula): fuori dai casi vale 0. */
static int split_modulus, split_case;

/* Quante volte establish() può ancora essere chiamata per la prova in corso.
 * Provare un valore dopo l'altro per più "esiste" annidati, o le due letture
 * di una lunga catena di equivalenze, fa crescere i tentativi in modo
 * esponenziale: finiti i passi la ricerca si ferma e il verdetto è NON
 * DIMOSTRATO, invece di esaurire tempo e memoria. */
static int search_left;

/* Ritorna la posizione in universal[] della variabile il cui atomo è atom, o
 * -1 se atom non è una variabile generica. */
static int universal_index(int atom) {
    for (int i = 0; i < universal_count; i++)
        if (atom_var(universal[i].name) == atom) return i;
    return -1;
}

/* Un valore dato a una variabile per dimostrare un "esiste". */
typedef struct {
    int name;     /* la lettera */
    Frac value;   /* il numero scelto */
} Choice;

/* I numeri scelti finora, dal quantificatore più esterno. Quando la
 * dimostrazione smentisce l'enunciato, sono il controesempio da riportare. */
static Choice chosen[MAX_BOUND];
static int chosen_count;

/* 1 se una scelta è stata fatta dentro un caso sul resto (18o): allora i
 * numeri scelti cambiano da un caso all'altro, e non formano un controesempio
 * da riportare. */
static int chosen_by_cases;

/* Annota che alla variabile name è stato dato il valore value. */
static void choose(int name, Frac value) {
    if (split_modulus) chosen_by_cases = 1;
    if (chosen_count < MAX_BOUND) chosen[chosen_count++] = (Choice){name, value};
}

/* Un punto della dimostrazione scritta e dell'elenco delle scelte a cui si
 * può tornare quando un tentativo fallisce. */
typedef struct {
    size_t proof;  /* lunghezza del testo */
    int chosen;    /* numero di scelte */
} Mark;

/* Ritorna il punto in cui si è adesso. */
static Mark mark_here(void) { return (Mark){proof_length, chosen_count}; }

/* Cancella il testo e le scelte aggiunti dopo mark. */
static void rollback(Mark mark) {
    proof_length = mark.proof;
    proof_text[proof_length] = 0;
    chosen_count = mark.chosen;
}

/* Lettere da scrivere con un apice, per atomo: la regola dei segni (18c)
 * indica con x' il parametro della semiretta su cui fa variare x. */
static char primed[MAX_ATOMS];

/* Scrive il testo dell'enunciato che corrisponde alla condizione f. */
static void put_source(const Formula *f) { proof_add("%.*s", (int)(f->end - f->start), f->start); }

/* Scrive l'insieme set com'è scritto nell'enunciato. */
static void put_numbers(const NumberSet *set) { proof_add("%.*s", (int)(set->end - set->start), set->start); }

/* Ritorna il grado del monomio: la somma dei suoi esponenti. */
static long long mono_degree(const Mono *m) {
    long long degree = 0;
    for (int i = 0; i < m->count; i++) degree += m->factor[i].exp.num;
    return degree;
}

/* Ritorna il termine di p che put_poly() scrive per primo: il primo, nella
 * forma canonica, tra quelli di grado più alto. p non deve essere nullo. */
static const Term *poly_leading(Poly p) {
    const Term *leading = &p.term[0];
    for (int i = 1; i < p.count; i++)
        if (mono_degree(&p.term[i].mono) > mono_degree(&leading->mono)) leading = &p.term[i];
    return leading;
}

/* Scrive in LaTeX il polinomio p, dai termini di grado più alto. p deve essere
 * un polinomio "vero" nelle variabili dell'enunciato (vedi rat_polynomials()). */
static void put_poly(Poly p) {
    if (p.count == 0) { proof_add("0"); return; }
    int written = 0;
    for (long long degree = mono_degree(&poly_leading(p)->mono); degree >= 0; degree--)
        for (int i = 0; i < p.count; i++) {
            const Term *t = &p.term[i];
            if (mono_degree(&t->mono) != degree) continue;

            /* Segno e valore assoluto del coefficiente. Davanti a delle
             * lettere il coefficiente 1 non si scrive. */
            Frac size = t->coef.re;
            int negative = size.num < 0;
            if (negative) size = frac_neg(size);
            proof_add("%s", written++ ? (negative ? " - " : " + ") : (negative ? "-" : ""));
            if (t->mono.count == 0 || size.num != 1 || size.den != 1) put_frac(size);

            /* Le lettere, con apice ed esponente. */
            for (int k = 0; k < t->mono.count; k++) {
                const Factor *f = &t->mono.factor[k];
                proof_add("%c%s", (int)atoms[f->atom].id, primed[f->atom] ? "'" : "");
                if (f->exp.num != 1) proof_add("^{%lld}", f->exp.num);
            }
        }
}

/* Scrive p come fattore di un prodotto: tra parentesi se ha più termini. */
static void put_factor(Poly p) {
    if (p.count > 1) { proof_add("\\left("); put_poly(p); proof_add("\\right)"); }
    else put_poly(p);
}

/* Ritorna -p. */
static Poly poly_negate(Poly p) { return poly_add(poly_const(num_int(0)), p, 1); }

/* Scrive la frazione num/den, o solo num se den è 1. Se den comincia con un
 * segno meno si cambia segno a entrambi: si legge meglio. */
static void put_ratio(Poly num, Poly den) {
    if (poly_is_one(den)) { put_poly(num); return; }
    if (poly_leading(den)->coef.re.num < 0) { num = poly_negate(num); den = poly_negate(den); }
    proof_add("\\frac{");
    put_poly(num);
    proof_add("}{");
    put_poly(den);
    proof_add("}");
}

/* Ritorna 1 se p è una costante, zero compreso. */
static int poly_is_constant(Poly p) { return p.count == 0 || (p.count == 1 && p.term[0].mono.count == 0); }

/* Ritorna 1 se l'atomo x compare in p. */
static int poly_uses(Poly p, int x) {
    for (int i = 0; i < p.count; i++)
        if (mono_exponent(&p.term[i].mono, x).num != 0) return 1;
    return 0;
}

/* Scrive r come frazione di due polinomi "veri": coefficienti razionali, solo
 * atomi che sono variabili, esponenti interi non negativi. Mette i due
 * polinomi in *num e *den e ritorna 1; ritorna 0 se r non si può scrivere
 * così.
 *
 * Serve perché rat_make() porta al numeratore un denominatore fatto di un
 * solo termine, lasciando esponenti negativi: 1/x diventa x^{-1}. Qui si fa
 * il passo inverso, moltiplicando sopra e sotto per la potenza di x che
 * manca. Dove r è definita quella potenza non è zero, quindi r si annulla
 * esattamente dove si annulla *num. */
static int rat_polynomials(Rat r, Poly *num, Poly *den) {
    *num = r.num;
    *den = r.den;

    /* Solo variabili, con esponenti interi e coefficienti reali. */
    for (int side = 0; side < 2; side++) {
        Poly p = side ? r.den : r.num;
        for (int i = 0; i < p.count; i++) {
            if (p.term[i].coef.im.num != 0) return 0;
            for (int k = 0; k < p.term[i].mono.count; k++)
                if (atoms[p.term[i].mono.factor[k].atom].kind != ATOM_VAR || p.term[i].mono.factor[k].exp.den != 1) return 0;
        }
    }

    /* Per ogni variabile, l'esponente più basso con cui compare: se è
     * negativo si moltiplica tutto per la potenza opposta. */
    for (int x = 0; x < atom_count && !failure; x++) {
        Frac lowest = frac_int(0);
        for (int side = 0; side < 2; side++) {
            Poly p = side ? *den : *num;
            for (int i = 0; i < p.count; i++) {
                Frac e = mono_exponent(&p.term[i].mono, x);
                if (frac_cmp(e, lowest) < 0) lowest = e;
            }
        }
        if (lowest.num == 0) continue;
        Term lift;
        lift.coef = num_int(1);
        lift.mono = mono_atom(x, frac_neg(lowest));
        *num = poly_mul(*num, poly_term(lift));
        *den = poly_mul(*den, poly_term(lift));
    }
    return !failure;
}

/* Controllo di un passo: ciò che la dimostrazione sta per dire delle radici
 * del polinomio p, nella variabile di atomo x che varia in set. Ritorna 1 se
 *
 *   - ognuno dei `roots` numeri di root[] è davvero una radice;
 *   - tra i primi elementi di set non c'è una radice che manca dall'elenco.
 *
 * rational_roots() le ha trovate con un conto su interi; qui si sostituisce
 * ogni numero nel polinomio com'è. Il secondo controllo non può essere
 * completo, ma guarda proprio dove un errore farebbe danno: una radice
 * dimenticata dentro l'insieme renderebbe falso "p non si annulla mai".
 * Se un controllo fallisce annota il passo come non giustificato e ritorna 0. */
static int roots_confirmed(Poly p, int x, const Frac *root, int roots, const NumberSet *set) {
    Frac sample[2 * SAMPLE_SIZE];
    Num value;
    for (int k = 0; k < roots; k++) {
        Rat at = poly_substitute(p, x, rat_frac(root[k]));
        if (failure) return 0;
        if (!rat_number(at, &value) || !num_is_zero(value)) { unjustified_steps++; return 0; }
    }
    numbers_sample(set, sample, 2 * SAMPLE_SIZE);
    for (int i = 0; i < 2 * SAMPLE_SIZE; i++) {
        int listed = 0;
        for (int k = 0; k < roots; k++)
            if (frac_cmp(sample[i], root[k]) == 0) listed = 1;
        if (!listed && rat_number(poly_substitute(p, x, rat_frac(sample[i])), &value) && num_is_zero(value)) { unjustified_steps++; return 0; }
    }
    return 1;
}

/* Una semiretta su cui la regola dei segni fa variare una variabile x:
 * x = origin + direction * t, con t >= 0, oppure t > 0 se strict. */
typedef struct {
    long long origin;  /* il punto di partenza */
    int direction;     /* +1 verso destra, -1 verso sinistra */
    int strict;        /* 1 se il punto di partenza è escluso */
    int used;          /* 1 se x compare nel polinomio in esame */
} Ray;

/* La semiretta in esame per ogni variabile generica, nella stessa posizione
 * di universal[]. Vale durante una chiamata a signs(). */
static Ray ray[MAX_BOUND];

/* Che cosa signs() deve stabilire di un polinomio. */
typedef enum {
    SIGN_NONNEGATIVE,  /* non è mai negativo */
    SIGN_POSITIVE,     /* è sempre positivo */
    SIGN_NONZERO       /* non è mai zero, oppure lo è solo dove lo è `guard` */
} SignGoal;

/* 18c. Regola dei segni.
 *
 * Stabilisce il segno del polinomio p per tutti i valori delle variabili
 * generiche, secondo goal, e scrive il ragionamento. Ritorna 1 se ci riesce.
 * Va chiamata con index = 0 e shown = p: shown è il polinomio di partenza, da
 * citare nel testo, mentre p cambia a ogni sostituzione.
 *
 * L'idea. Un polinomio in t >= 0 i cui coefficienti non sono negativi è una
 * somma di termini non negativi: non è negativo, ed è positivo se uno dei
 * termini lo è di sicuro (il termine noto, oppure un termine in sole variabili
 * strettamente positive). Per ricondursi a questo caso si copre l'insieme di
 * ogni variabile x con una o due semirette:
 *
 *   - un insieme con estremo inferiore c, come \mathbb{N}: x = c + t;
 *   - tutti gli interi: x = t, oppure x = -1 - t;
 *   - tutti i razionali: x = t, oppure x = -t.
 *
 * e su ciascuna si sostituisce x. Con più variabili si esaminano tutte le
 * combinazioni di semirette: il segno deve risultare in ognuna.
 *
 * Con guard e goal SIGN_NONZERO c'è un caso in più. Una somma di termini non
 * negativi è zero solo se lo è ogni termine; se tra i termini c'è una potenza
 * di una sola variabile, quella variabile deve valere zero. Se ogni termine
 * di guard contiene una variabile così forzata, dove p è zero lo è anche
 * guard. È ciò che serve per a + b con a, b >= 0: è zero solo per
 * a = b = 0, dove a - b è zero. */
static int signs(Poly shown, Poly p, const Poly *guard, int index, SignGoal goal) {
    if (failure) return 0;

    /* 18c-1. Una variabile alla volta: le semirette che coprono il suo
     * insieme. Dopo la sostituzione x = origin + direction * x la lettera x
     * indica il parametro t della semiretta. */
    if (index < universal_count) {
        const NumberSet *set = universal[index].numbers;
        int x = atom_var(universal[index].name);
        ray[index].used = poly_uses(p, x) || (guard && poly_uses(*guard, x));
        if (!ray[index].used) return signs(shown, p, guard, index + 1, goal);

        Ray cover[2] = {{set->bounded ? set->low : 0, 1, set->bounded && set->strict, 1},
                        {set->integers ? -1 : 0, -1, 0, 1}};
        int covers = set->bounded ? 1 : 2;

        /* Controllo del passo: ogni elemento dell'insieme deve stare su una
         * delle semirette, altrimenti il ragionamento per casi ne lascerebbe
         * fuori qualcuno. Lo si verifica sui primi elementi dell'insieme: il
         * parametro t = direction * (x - origin) non deve essere negativo. */
        Frac sample[SAMPLE_SIZE];
        numbers_sample(set, sample, SAMPLE_SIZE);
        for (int i = 0; i < SAMPLE_SIZE; i++) {
            int covered = 0;
            for (int k = 0; k < covers; k++) {
                Frac t = frac_mul(frac_int(cover[k].direction), frac_add(sample[i], frac_int(-cover[k].origin)));
                if (t.num > 0 || (t.num == 0 && !cover[k].strict)) covered = 1;
            }
            if (!covered) { unjustified_steps++; return 0; }
        }
        for (int k = 0; k < covers; k++) {
            Rat line = rat_add(rat_int(cover[k].origin), rat_mul(rat_int(cover[k].direction), rat_atom(x)), 0);
            Poly moved = poly_substitute(p, x, line).num, moved_guard = {0, NULL};
            if (guard) moved_guard = poly_substitute(*guard, x, line).num;
            ray[index] = cover[k];
            if (!signs(shown, moved, guard ? &moved_guard : NULL, index + 1, goal)) return 0;
        }
        return 1;
    }

    /* 18c-2. Tutte le variabili sono su una semiretta: si guardano i
     * coefficienti. `certain` dice se c'è un termine sicuramente diverso da
     * zero; forced[i] se un termine è potenza della sola variabile i. */
    int negative = 0, positive = 0, certain = 0, constant = 0;
    char forced[MAX_BOUND] = {0};
    for (int i = 0; i < p.count; i++) {
        const Term *t = &p.term[i];
        int all_strict = 1;
        if (t->coef.re.num < 0) negative++; else positive++;
        for (int k = 0; k < t->mono.count; k++) {
            int who = universal_index(t->mono.factor[k].atom);
            if (who < 0) return 0;
            if (!ray[who].strict) all_strict = 0;
            if (t->mono.count == 1) forced[who] = 1;
        }
        if (all_strict) certain = 1;
        if (t->mono.count == 0) constant = 1;
    }

    /* 18c-3. Il segno che ne segue. `conclusion` è la parola con cui lo si
     * dirà, oppure NULL nel caso con guard: zero solo dove ogni termine è
     * zero, e lì deve annullarsi anche guard. */
    const char *conclusion = NULL;
    if (goal == SIGN_NONNEGATIVE && !negative) {
        conclusion = "not negative";
    } else if (goal != SIGN_NONNEGATIVE && !negative && certain) {
        conclusion = "positive";
    } else if (goal == SIGN_NONZERO && !positive && certain) {
        conclusion = "negative";
    } else if (goal == SIGN_NONZERO && guard && p.count && !(negative && positive)) {
        for (int i = 0; i < guard->count; i++) {
            int vanishes = 0;
            for (int k = 0; k < guard->term[i].mono.count; k++) {
                int who = universal_index(guard->term[i].mono.factor[k].atom);
                if (who >= 0 && forced[who]) vanishes = 1;
            }
            if (!vanishes) return 0;
        }
    } else {
        return 0;
    }

    /* Controllo del passo: tornando indietro dalle semirette, cioè
     * sostituendo t = direction * (x - origin), si deve ritrovare il
     * polinomio di partenza. È ciò che il testo affermerà scrivendo
     * "p = ..." con le variabili nuove. */
    Rat back = {p, poly_const(num_int(1))};
    for (int i = 0; i < universal_count; i++) {
        if (!ray[i].used) continue;
        int x = atom_var(universal[i].name);
        Rat inverse = rat_mul(rat_int(ray[i].direction), rat_add(rat_atom(x), rat_int(ray[i].origin), 1));
        back = poly_substitute(back.num, x, inverse);
    }
    if (failure) return 0;
    if (!poly_is_one(back.den) || !poly_equal(back.num, shown)) { unjustified_steps++; return 0; }

    /* 18c-4. Il testo: le semirette, poi il polinomio prima e dopo la
     * sostituzione. Una variabile su una semiretta che non parte da zero
     * verso destra viene riscritta, e da lì in poi porta l'apice. */
    int listed = 0, shifted = 0;
    for (int i = 0; i < universal_count; i++) {
        if (!ray[i].used) continue;
        int name = universal[i].name;
        const char *order = ray[i].strict ? ">" : "\\ge";
        proof_add("%s", listed++ ? ", " : " For ");
        if (ray[i].origin == 0 && ray[i].direction > 0) { proof_add("$%c %s 0$", name, order); continue; }
        shifted = 1;
        proof_add("$%c = ", name);
        if (ray[i].origin) proof_add("%lld %c ", ray[i].origin, ray[i].direction > 0 ? '+' : '-');
        else proof_add("-");
        proof_add("%c'$ with $%c' %s 0$", name, name, order);
    }
    proof_add("%s $", listed ? ":" : "");
    put_poly(shown);
    for (int i = 0; i < universal_count; i++)
        primed[atom_var(universal[i].name)] = ray[i].used && (ray[i].origin != 0 || ray[i].direction < 0);
    if (shifted) { proof_add(" = "); put_poly(p); }
    proof_add("$");
    if (conclusion) {
        /* Per "non negativo" bastano i coefficienti; per un segno stretto
         * serve anche il termine sicuramente diverso da zero. */
        proof_add(" has no %s coefficient", negative ? "positive" : "negative");
        if (goal != SIGN_NONNEGATIVE) proof_add(" and %s", constant ? "a constant term" : "a term whose variables are all positive");
        proof_add(", so it is %s.", conclusion);
    } else {
        proof_add(" has no %s coefficient, so it is zero only if each of its terms is zero. That forces",
                  negative ? "positive" : "negative");
        for (int i = 0, written = 0; i < universal_count; i++)
            if (forced[i]) proof_add("%s $%c%s = 0$", written++ ? "," : "", universal[i].name, primed[atom_var(universal[i].name)] ? "'" : "");
        proof_add(", and then $");
        put_poly(*guard);
        proof_add(" = 0$.");
    }
    memset(primed, 0, sizeof primed);
    return 1;
}

/* 18a, 18b, 18c. Dimostra che il polinomio p non si annulla per nessun valore
 * delle variabili generiche; se guard non è NULL basta meno: che p si annulli
 * solo dove si annulla anche guard. Scrive il ragionamento e ritorna 1 se ci
 * riesce. p deve essere un polinomio "vero" (vedi rat_polynomials()). */
static int zero_only_where(Poly p, const Poly *guard) {
    if (failure || p.count == 0) return 0;

    /* 18a. Una costante. */
    if (poly_is_constant(p)) {
        proof_add(" The constant $");
        put_poly(p);
        proof_add("$ is not zero.");
        return 1;
    }

    /* 18b. Una sola variabile: si annulla solo nelle radici, e quelle che
     * contano stanno nell'insieme, che è fatto di razionali. */
    for (int i = 0; i < universal_count; i++) {
        long long c[MAX_UNIVARIATE + 1], scale;
        Frac root[MAX_UNIVARIATE + 1];
        const NumberSet *set = universal[i].numbers;
        int x = atom_var(universal[i].name);
        int degree = poly_coefficients(p, x, c, &scale);
        if (degree < 1) continue;
        int roots = rational_roots(c, degree, root);
        if (roots < 0) break;
        if (!roots_confirmed(p, x, root, roots, set)) return 0;

        /* Una radice nell'insieme è ammessa solo se lì si annulla guard. */
        int inside = 0;
        for (int k = 0; k < roots; k++) {
            Num value;
            if (!numbers_contain(set, root[k])) continue;
            if (!guard || !rat_number(poly_substitute(*guard, x, rat_frac(root[k])), &value) || !num_is_zero(value)) return 0;
            inside++;
        }

        proof_add(" The polynomial $");
        put_poly(p);
        if (roots == 0) {
            proof_add("$ has no rational root: the rational root theorem leaves finitely many candidates, and none of them is a root.");
            return 1;
        }
        proof_add("$ has the rational root%s $", roots > 1 ? "s" : "");
        for (int k = 0; k < roots; k++) {
            if (k) proof_add(",\\ ");
            put_frac(root[k]);
        }
        proof_add("$ and no other; ");
        if (!inside) {
            proof_add("%s in $", roots > 1 ? "none of them is" : "it is not");
            put_numbers(set);
            proof_add("$.");
        } else {
            proof_add("wherever a root is in $");
            put_numbers(set);
            proof_add("$, also $");
            put_poly(*guard);
            proof_add("$ is zero.");
        }
        return 1;
    }

    /* 18c. Più variabili, o radici non determinabili: la regola dei segni. */
    return signs(p, p, guard, 0, SIGN_NONZERO);
}

/* 18d. Dimostra che il polinomio p è un numero intero per ogni valore delle
 * variabili generiche, scrive il ragionamento e ritorna 1 se ci riesce.
 *
 * Due casi. Se le variabili sono intere e i coefficienti pure, è ovvio. Se p
 * ha una sola variabile n e grado d, basta che sia intero per n = 0, 1, ...,
 * d. Infatti (formula di Newton) p(n) è la somma, per k da 0 a d, di
 * D_k * binom(n, k), dove D_k è la differenza k-esima di p in zero: una
 * combinazione a coefficienti interi di p(0), ..., p(k), quindi un intero. E
 * binom(n, k) è intero per ogni n intero, anche negativo. Così (n^2 + n)/2 è
 * sempre intero, pur avendo coefficienti frazionari. */
static int always_integer(Poly p) {
    /* Le variabili devono essere tutte intere. */
    int whole = 1;
    for (int i = 0; i < p.count; i++) {
        if (p.term[i].coef.re.den != 1) whole = 0;
        for (int k = 0; k < p.term[i].mono.count; k++) {
            int who = universal_index(p.term[i].mono.factor[k].atom);
            if (who < 0 || !universal[who].numbers->integers) return 0;
        }
    }
    if (whole) {
        proof_add(" It is an integer, because the coefficients of $");
        put_poly(p);
        proof_add("$ are integers.");
        return 1;
    }

    /* Una sola variabile: i valori in 0, ..., grado. I coefficienti c sono
     * quelli di p moltiplicati per scale, quindi p(k) è intero quando scale
     * divide la somma dei c[j] k^j. Con c fino a un miliardo e k, j fino a
     * 16 la somma sta comodamente in un __int128. */
    for (int i = 0; i < universal_count; i++) {
        long long c[MAX_UNIVARIATE + 1], scale;
        int degree = poly_coefficients(p, atom_var(universal[i].name), c, &scale);
        if (degree < 1) continue;
        for (long long k = 0; k <= degree; k++) {
            __int128 value = 0;
            for (int j = degree; j >= 0; j--) value = value * k + c[j];
            if (value % scale != 0) return 0;
        }
        proof_add(" It is an integer: $");
        put_poly(p);
        proof_add("$ is an integer for $%c = 0, \\ldots, %d$, and a polynomial of degree $%d$ that is an integer at "
                  "$%d$ consecutive integers is an integer at every integer.", universal[i].name, degree, degree, degree + 1);
        return 1;
    }
    return 0;
}

/* 18e. Dimostra che value appartiene a set per ogni valore delle variabili
 * generiche, scrive il ragionamento e ritorna 1 se ci riesce. Le frasi hanno
 * per soggetto "it": chi chiama ha appena nominato il valore. */
static int always_in(Rat value, const NumberSet *set) {
    Poly num, den;
    Num number;
    if (!rat_polynomials(value, &num, &den)) return 0;

    /* Un numero: basta guardarlo. */
    if (rat_number(value, &number)) {
        if (!numbers_contain(set, number.re)) return 0;
        proof_add(" Indeed $");
        put_frac(number.re);
        proof_add("$ is in $");
        put_numbers(set);
        proof_add("$.");
        return 1;
    }

    /* Intero, se l'insieme è di interi. */
    if (!set->integers) proof_add(" It is a rational number.");
    else if (!poly_is_one(den) || !always_integer(num)) return 0;

    /* Non sotto l'estremo inferiore: value - low = above/den, e si guarda il
     * segno di above e quello di den. */
    if (set->bounded) {
        Poly above = poly_add(num, poly_mul(poly_const(num_int(set->low)), den), 1);
        if (set->low == 0) {
            proof_add(set->strict ? " It is positive." : " It is not negative.");
        } else {
            proof_add(" It is %s $%lld$, since subtracting $%lld$ leaves $", set->strict ? "greater than" : "at least", set->low, set->low);
            put_ratio(above, den);
            proof_add("$.");
        }
        if (!poly_is_one(den)) proof_add(" We look at the numerator and at the denominator.");
        if (!signs(above, above, NULL, 0, set->strict ? SIGN_POSITIVE : SIGN_NONNEGATIVE)) return 0;
        if (!poly_is_one(den) && !signs(den, den, NULL, 0, SIGN_POSITIVE)) return 0;
    }

    /* Diverso da ogni punto tolto: value = h vorrebbe dire num - h den = 0. */
    for (int i = 0; i < set->holes; i++) {
        Poly gap = poly_add(num, poly_mul(poly_const(num_frac(set->hole[i])), den), 1);
        proof_add(" It is not $");
        put_frac(set->hole[i]);
        proof_add("$: that would mean $");
        put_poly(gap);
        proof_add(" = 0$.");
        if (!zero_only_where(gap, NULL)) return 0;
    }
    return 1;
}

/* 18f. Dimostra che ogni divisione scritta nell'espressione node ha il
 * divisore diverso da zero, per ogni valore delle variabili generiche. Scrive
 * il ragionamento, salvo per i divisori che sono numeri, e ritorna 1 se ci
 * riesce.
 *
 * Non entra nelle regole delle applicazioni: che f(x) sia definita per x nel
 * dominio lo stabilisce una volta per tutte map_defined() (18m), e
 * term_check() garantisce che l'argomento stia nel dominio.
 *
 * Si controllano prima le divisioni più interne: un divisore si può calcolare
 * solo se ciò che contiene è definito. */
static int term_defined(const Node *node) {
    Poly num, den;
    if (!node || node->kind == N_APPLY) return 1;
    if (!term_defined(node->a) || !term_defined(node->b)) return 0;
    if (node->kind != N_DIV) return 1;
    if (!rat_polynomials(eval(node->b), &num, &den)) return 0;
    if (poly_is_constant(num)) return num.count != 0;
    proof_add(" The divisor $");
    put_ratio(num, den);
    proof_add("$ is never zero.");
    return zero_only_where(num, NULL);
}

/* Un confronto dentro una disgiunzione da dimostrare. */
typedef struct {
    Formula *source;  /* il confronto com'è scritto nell'enunciato */
    int equation;     /* 1 se si chiede che i due membri siano uguali, 0 se
                         diversi: dipende dal confronto e dalle negazioni */
    int cleared;      /* 1 se value è stato ottenuto togliendo denominatori */
    int computed;     /* 1 se il confronto è stato deciso tra numeri, con
                         closed_gap(): allora value è 0 (uguali) o 1 (diversi) */
    Poly value;       /* il numeratore di membro sinistro - membro destro */
} Literal;

/* Ritorna 1 se ogni variabile dell'espressione node ha in questo momento un
 * numero per valore, cioè se l'espressione si può calcolare. shadow elenca
 * gli indici di somma che nascondono una variabile con lo stesso nome. */
static int term_is_closed(const Node *node, const int *shadow, int shadows) {
    Num value;
    if (!node) return 1;
    if (node->kind == N_VAR) {
        for (int i = 0; i < shadows; i++)
            if (shadow[i] == node->name) return 1;
        const Rat *bound = env_lookup(node->name);
        return node->name == VAR_PI || (bound && rat_number(*bound, &value));
    }
    if (node->kind == N_BIGOP) {
        int inner[MAX_ENV + 1];
        if (shadows >= MAX_ENV || !term_is_closed(node->a, shadow, shadows) || !term_is_closed(node->b, shadow, shadows)) return 0;
        memcpy(inner, shadow, (size_t)shadows * sizeof(int));
        inner[shadows] = node->name;
        return term_is_closed(node->c, inner, shadows + 1);
    }
    return term_is_closed(node->a, shadow, shadows) && term_is_closed(node->b, shadow, shadows) && term_is_closed(node->c, shadow, shadows);
}

/* 18g, per le espressioni che non sono razionali. Calcola i due membri del
 * confronto source, in cui ogni variabile ha un numero per valore, e decide
 * se sono uguali: ritorna 1 se lo sono, 0 se sono diversi, -1 se non lo sa
 * dire o se un membro non è definito.
 *
 * Qui "definito" non si riduce ai divisori: il conto annota le condizioni di
 * esistenza (capitolo 7) e ognuna deve risultare vera, con le regole del
 * paragrafo 13c. Senza questo passo ln(cos 2) = ln(cos 2) sarebbe "la stessa
 * espressione", e quindi vero, pur non esistendo. Che due valori siano
 * diversi si conclude solo se una di quelle regole prova che la differenza
 * non è zero. Un errore di calcolo vuol dire solo che non si sa. */
static int closed_gap(const Formula *source) {
    long long where;
    if (failure || !term_is_closed(source->left, NULL, 0) || !term_is_closed(source->right, NULL, 0)) return -1;
    condition_count = conditions_lost = 0;
    conditions_on = 1;
    Rat gap = rat_add(eval(source->left), eval(source->right), 1);
    conditions_on = 0;
    int answer = failure || conditions_lost ? -1 : 1;
    for (int i = 0; i < condition_count && answer == 1; i++)
        if (condition_decided(&conditions[i], 1, &where) != 1) answer = -1;
    if (answer == 1 && !poly_vanishes(gap.num)) answer = condition_holds(NEVER_ZERO, gap.num, 1, &where) == 1 ? 0 : -1;
    if (failure) answer = -1;
    failure = NULL;
    return answer;
}

/* Come si legge un connettivo tenuto conto della negazione: una congiunzione
 * o una disgiunzione di due parti, ognuna con il suo segno. */
typedef struct {
    int conjunction;  /* 1 congiunzione, 0 disgiunzione */
    Formula *x, *y;   /* le due parti */
    int nx, ny;       /* 1 se la parte va presa negata */
} Junction;

/* Se f è una congiunzione, una disgiunzione o un'implicazione mette in *j il
 * modo di leggerla, negata se negated, e ritorna 1.
 *
 * Leggi di De Morgan: non (A e B) = (non A) o (non B), e viceversa. Per
 * l'implicazione: A implica B = (non A) o B, quindi la sua negazione è
 * A e (non B). */
static int junction(Formula *f, int negated, Junction *j) {
    if (f->kind != F_AND && f->kind != F_OR && f->kind != F_IMPLIES) return 0;
    j->conjunction = (f->kind == F_AND) != negated;
    j->x = f->a;
    j->y = f->b;
    j->nx = f->kind == F_IMPLIES ? !negated : negated;
    j->ny = negated;
    return 1;
}

/* Se f, negata se negated, è un confronto o una disgiunzione di confronti, li
 * aggiunge a literal[] aggiornando *count e ritorna 1. Altrimenti ritorna 0,
 * e literal[] può contenere confronti raccolti a metà, da ignorare. */
static int gather(Formula *f, int negated, Literal *literal, int *count) {
    Junction j;
    if (f->kind == F_NOT) return gather(f->a, !negated, literal, count);
    if (f->kind == F_EQUAL || f->kind == F_DISTINCT) {
        if (*count == MAX_LITERALS) return 0;
        literal[(*count)++] = (Literal){f, (f->kind == F_EQUAL) != negated, 0, 0, {0, NULL}};
        return 1;
    }
    return junction(f, negated, &j) && !j.conjunction &&
           gather(j.x, j.nx, literal, count) && gather(j.y, j.ny, literal, count);
}

/* Scrive "il confronto vale" oppure "il confronto non vale", secondo che ciò
 * che si chiede (uguali se equation) coincida o no con ciò che è scritto. */
static void put_outcome(const Formula *source, int equation) {
    proof_add(" $");
    put_source(source);
    proof_add("$ %s", (source->kind == F_EQUAL) == equation ? "holds" : "fails");
}

/* 18h, prima parte. Divide *h per e finché la divisione è esatta, lasciando
 * in *h il quoziente, e ritorna quante volte ha diviso. e deve essere di primo
 * grado, con coefficiente costante, in una variabile generica x: e = c x + r.
 *
 * Regola di Ruffini. Sia rho = -r/c la radice di e in x. Scritto h come
 * p_d x^d + ... + p_0, con i p_j senza x, il quoziente per x - rho ha
 * coefficienti q_{d-1} = p_d e q_{j-1} = p_j + rho q_j, e il resto è
 * p_0 + rho q_0. La divisione è esatta quando il resto è il polinomio nullo;
 * allora h = (x - rho) Q = e (Q / c). */
static int divide_out(Poly *h, Poly e) {
    for (int i = 0; i < universal_count; i++) {
        int x = atom_var(universal[i].name), times = 0;
        Poly linear[2];
        if (poly_split(e, x, linear, 1) != 1 || !poly_is_constant(linear[1])) continue;
        Num c = linear[1].term[0].coef;
        Poly rho = poly_mul(linear[0], poly_const(num_neg(num_inv(c))));

        for (;;) {
            Poly piece[MAX_UNIVARIATE + 1];
            int degree = poly_split(*h, x, piece, MAX_UNIVARIATE);
            if (degree < 1) break;

            /* carry vale q_j quando comincia il passo j. */
            Poly quotient = poly_const(num_int(0)), carry = piece[degree];
            for (int j = degree - 1; j >= 0; j--) {
                Term power;
                power.coef = num_int(1);
                power.mono.count = 0;
                if (j > 0) power.mono = mono_atom(x, frac_int(j));
                quotient = poly_add(quotient, poly_mul(carry, poly_term(power)), 0);
                carry = poly_add(piece[j], poly_mul(rho, carry), 0);
            }
            if (failure || carry.count != 0) break;
            *h = poly_mul(quotient, poly_const(num_inv(c)));
            times++;
        }
        return times;
    }
    return 0;
}

/* 18g, 18h. Dimostra che, per ogni valore delle variabili generiche, vale
 * almeno uno dei `count` confronti di literal[]. Scrive il ragionamento, come
 * seguito di un \item già aperto, e ritorna 1 se ci riesce.
 *
 * Ogni confronto diventa un polinomio, il numeratore della differenza dei due
 * membri: "uguali" vuol dire polinomio zero, "diversi" polinomio non zero.
 *
 *   18g. Se un confronto vale sempre la disgiunzione è dimostrata: un
 *        "uguali" con polinomio nullo, o un "diversi" con una costante non
 *        nulla. È sempre il caso quando non ci sono variabili generiche,
 *        perché allora i polinomi sono numeri. Un confronto tra espressioni
 *        non razionali si decide solo così, tra numeri, e con le condizioni
 *        di esistenza controllate (closed_gap()).
 *   18h. Tolti i confronti che non valgono mai, la forma trattata è
 *        "p diverso da 0, oppure e = 0", cioè "p = 0 implica e = 0", anche
 *        senza la seconda parte. Si divide p per e finché si può (p = e^k q:
 *        dove e non è zero, p è zero solo se lo è q), poi si chiede a
 *        zero_only_where() che q si annulli al più dove si annulla e.
 *
 * Una disgiunzione di soli "uguali" non identici non può valere per ogni
 * valore: il prodotto dei polinomi non è nullo, e gli insiemi sono infiniti. */
static int prove_clause(Literal *literal, int count) {
    /* Il polinomio di ogni confronto, dopo aver controllato le divisioni. */
    for (int i = 0; i < count; i++) {
        const Formula *source = literal[i].source;
        Poly den;

        /* Espressioni non razionali: solo tra numeri, con closed_gap(). */
        literal[i].computed = !term_is_rational(source->left) || !term_is_rational(source->right);
        if (literal[i].computed) {
            int equal = closed_gap(source);
            if (equal < 0) return 0;
            literal[i].value = poly_const(num_int(equal ? 0 : 1));
            continue;
        }
        if (!term_defined(source->left) || !term_defined(source->right)) return 0;
        Rat gap = rat_add(eval(source->left), eval(source->right), 1);
        if (failure || !rat_polynomials(gap, &literal[i].value, &den)) return 0;
        literal[i].cleared = !poly_is_one(den);
    }

    /* 18g. Un confronto che vale sempre. Se i due membri sono numeri si
     * riportano, così il lettore può rifare il conto. */
    for (int i = 0; i < count; i++) {
        const Literal *l = &literal[i];
        Num left, right;
        int always = l->equation ? l->value.count == 0 : (poly_is_constant(l->value) && l->value.count != 0);
        if (!always) continue;
        put_outcome(l->source, l->equation);
        if (l->computed) {
            proof_add(l->equation ? ": computed exactly, the two sides are equal." : ": computed exactly, the two sides are different.");
        } else if (rat_number(eval(l->source->left), &left) && rat_number(eval(l->source->right), &right)) {
            proof_add(l->equation ? ": both sides are equal to $" : ": the two sides are $");
            put_frac(left.re);
            if (!l->equation) { proof_add("$ and $"); put_frac(right.re); }
            proof_add("$.");
        } else if (l->equation) {
            proof_add(": written out, the two sides are the same expression.");
        } else {
            proof_add(": the difference of the two sides has the numerator $");
            put_poly(l->value);
            proof_add("$, a constant that is not zero.");
        }
        return 1;
    }

    /* 18h. Restano i confronti che dipendono dalle variabili: le costanti
     * rimaste sono confronti che non valgono mai. */
    const Literal *distinct = NULL, *equal = NULL;
    for (int i = 0; i < count; i++) {
        if (poly_is_constant(literal[i].value)) continue;
        if (!literal[i].equation && !distinct) distinct = &literal[i];
        else if (literal[i].equation && !equal) equal = &literal[i];
        else return 0;
    }
    if (!distinct) return 0;
    Poly h = distinct->value;
    const char *clearing = distinct->cleared ? ", after clearing denominators," : "";

    /* Un solo "diversi": il polinomio non deve annullarsi mai. */
    if (!equal) {
        int written_equal = distinct->source->kind == F_EQUAL;
        proof_add(written_equal ? " The equality $" : " The condition $");
        put_source(distinct->source);
        proof_add(written_equal ? "$%s would mean $" : "$%s means $", clearing);
        put_poly(h);
        proof_add(written_equal ? " = 0$." : " \\neq 0$.");
        if (!zero_only_where(h, NULL)) return 0;
        if (written_equal) proof_add(" So the equality never holds.");
        return 1;
    }

    /* "Diversi oppure uguali": p = 0 deve implicare e = 0. */
    Poly e = equal->value;
    proof_add(" Suppose that $");
    put_source(distinct->source);
    proof_add("$ %s%s that is $", distinct->source->kind == F_EQUAL ? "holds" : "fails", distinct->cleared ? clearing : ",");
    put_poly(h);
    proof_add(" = 0$.");
    int times = divide_out(&h, e);

    /* Controllo del passo: rimoltiplicando i fattori si deve ritrovare il
     * polinomio di partenza. La divisione è fatta con la regola di Ruffini,
     * il controllo con il prodotto. */
    Poly rebuilt = h;
    for (int i = 0; i < times; i++) rebuilt = poly_mul(rebuilt, e);
    if (failure) return 0;
    if (!poly_equal(rebuilt, distinct->value)) { unjustified_steps++; return 0; }
    if (times && !poly_is_one(h)) {
        proof_add(" Now $");
        put_poly(distinct->value);
        proof_add(" = ");
        put_factor(h);

        /* Il fattore e: tra parentesi se ha più termini o un esponente. */
        int brackets = e.count > 1 || times > 1;
        proof_add(brackets ? "\\left(" : " \\cdot ");
        put_poly(e);
        if (brackets) proof_add("\\right)");
        if (times > 1) proof_add("^{%d}", times);
        proof_add("$.");
    }
    if (!poly_is_one(h) && !zero_only_where(h, &e)) return 0;
    proof_add(" Hence $");
    put_poly(e);
    proof_add(" = 0$, ");
    proof_add(equal->source->kind == F_EQUAL ? "that is $" : "so $");
    put_source(equal->source);
    proof_add(equal->source->kind == F_EQUAL ? "$." : "$ fails.");
    return 1;
}

/* Prototipo in avanti inevitabile: establish(), solve_with() e
 * by_remainders() si chiamano a vicenda. */
static int establish(Formula *f, int negated);

/* Ritorna 1 se la lettera name compare nel sotto-albero node, come variabile,
 * come indice di una somma o come nome di una successione. */
static int node_uses_letter(const Node *node, int name) {
    if (!node) return 0;
    if (node->name == name && (node->kind == N_VAR || node->kind == N_BIGOP || node->kind == N_SEQ || node->kind == N_APPLY)) return 1;
    return node_uses_letter(node->a, name) || node_uses_letter(node->b, name) || node_uses_letter(node->c, name);
}

/* Ritorna 1 se la lettera name compare nella condizione f: quantificata, o
 * dentro un'espressione. */
static int formula_uses_letter(const Formula *f, int name) {
    if (!f) return 0;
    if ((f->kind == F_FORALL || f->kind == F_EXISTS) && f->variable == name) return 1;
    return node_uses_letter(f->left, name) || node_uses_letter(f->right, name) ||
           formula_uses_letter(f->a, name) || formula_uses_letter(f->b, name);
}

/* 18n. Dimostra "per ogni n in A vale L = R" per induzione su n, quando A è
 * un insieme di interi da un certo punto in su e non ci sono altre variabili
 * in gioco. all è il nodo del quantificatore ed equation il confronto.
 * Scrive la dimostrazione, con i suoi passaggi, e ritorna 1 se ci riesce.
 *
 * È il metodo del capitolo 13 usato come una regola: lì arriva un enunciato
 * intero, qui un pezzo di un enunciato più grande. Le due espressioni sono
 * già alberi dello stesso tipo, quindi non c'è nulla da rileggere.
 *
 * L'induzione vuole la tabella degli atomi per sé: un atomo ricorda se
 * dipende dalla variabile di induzione, e quelli nati finora sono stati
 * creati senza saperlo. La tabella viene quindi messa da parte e ripristinata
 * alla fine, com'era: i valori scelti per gli "esiste" già dimostrati, che la
 * verifica rileggerà, contengono i numeri d'ordine dei vecchi atomi. */
static int by_induction(const Formula *all, const Formula *equation) {
    static Atom kept_atoms[MAX_ATOMS];
    static Poly kept_polys[MAX_ATOMS];
    static Rat kept_atans[MAX_ATOMS];
    Statement statement = {equation->left, equation->right, REL_EQ};
    int atoms_before = atom_count, polys_before = base_poly_count, atans_before = atan_count, variable_before = induction_var;
    long long default_before = default_base, floor_before = domain_floor;
    size_t arena_before = arena_used, start = proof_length;
    const char *syntax_before = syntax_error;

    /* Via la tabella di prima; l'arena continua da dov'è. */
    memcpy(kept_atoms, atoms, sizeof(Atom) * (size_t)atoms_before);
    memcpy(kept_polys, base_polys, sizeof(Poly) * (size_t)polys_before);
    memcpy(kept_atans, atan_args, sizeof(Rat) * (size_t)atans_before);
    atom_count = base_poly_count = atan_count = 0;

    proof_add("\\item We prove $");
    put_source(equation);
    proof_add("$ for every $%c \\in ", all->variable);
    put_numbers(&all->numbers);
    proof_add("$.\n");
    int proved = induct(&statement, all->variable, 1, all->numbers.low + all->numbers.strict, 1).outcome == PROVED;
    if (!proved) proof_cut(start);

    /* Tutto com'era. Un errore dell'induzione vuol dire solo che questa
     * regola non si applica. */
    memcpy(atoms, kept_atoms, sizeof(Atom) * (size_t)atoms_before);
    memcpy(base_polys, kept_polys, sizeof(Poly) * (size_t)polys_before);
    memcpy(atan_args, kept_atans, sizeof(Rat) * (size_t)atans_before);
    atom_count = atoms_before;
    base_poly_count = polys_before;
    atan_count = atans_before;
    induction_var = variable_before;
    default_base = default_before;
    domain_floor = floor_before;
    arena_used = arena_before;
    syntax_error = syntax_before;
    failure = NULL;
    env_count = 0;
    return proved;
}

/* 18o. Dimostra "per ogni x in A vale P", con A un insieme di interi senza
 * punti tolti, per casi sul resto di x in una divisione. f è il nodo del
 * quantificatore e negated il segno con cui va letta P. Scrive la
 * dimostrazione e ritorna 1 se ci riesce; se ritorna 0 il testo scritto va
 * cancellato da chi chiama.
 *
 * Divisione con resto: fissato m > 0, ogni intero x si scrive in un solo
 * modo come x = m t + r, con t intero e r tra 0 e m - 1. Dimostrare P per
 * ogni x equivale quindi a dimostrarla per x = m t + r, per ogni t, in
 * ognuno degli m casi. In ciascun caso x è legata all'espressione m t + r, e
 * t è una variabile generica nuova: le altre regole lavorano come sempre. È
 * così che "ogni intero è pari o dispari" si riduce a due equazioni di primo
 * grado: con x = 2 t si prende y = t, con x = 2 t + 1 ancora y = t.
 *
 * Il modulo non si conosce in anticipo: si provano m = 2, 3, ... fino a
 * MAX_CASES. Se x deve stare in A = {x >= a}, allora m t + r >= a, cioè t
 * parte dal più piccolo intero non minore di (a - r)/m.
 *
 * Un caso dentro l'altro non si fa: il valore scelto per un "esiste" è
 * ricordato caso per caso (vedi Formula), con un solo numero di caso. */
static int by_remainders(Formula *f, int negated) {
    static char written[48];
    const NumberSet *set = &f->numbers;
    int quotient = 0;
    if (!set->integers || set->holes || split_modulus || failure) return 0;

    /* Una lettera per il quoziente che l'enunciato non usa. */
    for (const char *letter = "tsuvwpqhgedcba"; *letter && !quotient; letter++) {
        int taken = map_find(*letter) != NULL || formula_uses_letter(f, *letter) || env_lookup(*letter) != NULL;
        for (int i = 0; i < universal_count; i++) taken = taken || universal[i].name == *letter;
        if (!taken) quotient = *letter;
    }
    if (!quotient) return 0;

    /* I tentativi di questa regola sono contati a parte: provare cinque
     * moduli non deve consumare la ricerca di chi sta intorno. */
    int outside = search_left > MAX_SEARCH / 20 ? search_left - MAX_SEARCH / 20 : 0;
    search_left -= outside;
    for (int m = 2; m <= MAX_CASES && search_left > 0 && !failure; m++) {
        Mark mark = mark_here();
        int holds = 1;
        proof_add("\\item Every integer is $%d%c + r$ for exactly one integer $%c$ and one $r$ among $0", m, quotient, quotient);
        for (int r = 1; r < m; r++) proof_add(", %d", r);
        proof_add("$: division by $%d$ with remainder. So there are $%d$ cases.\n", m, m);
        split_modulus = m;
        for (int r = 0; r < m && holds; r++) {
            /* L'insieme in cui varia il quoziente, e come scriverlo. */
            NumberSet part = {0};
            long long first = set->low + set->strict - r;
            part.integers = 1;
            part.bounded = set->bounded;
            part.low = first >= 0 ? (first + m - 1) / m : -((-first) / m);
            if (!part.bounded) snprintf(written, sizeof written, "\\mathbb{Z}");
            else if (part.low == 0 || part.low == 1) snprintf(written, sizeof written, part.low ? "\\mathbb{N}" : "\\mathbb{N}_{0}");
            else snprintf(written, sizeof written, "\\{%lld, %lld, \\ldots\\}", part.low, part.low + 1);
            part.start = written;
            part.end = written + strlen(written);

            proof_add("\\item \\emph{Case $%c = %d%c", f->variable, m, quotient);
            if (r) proof_add(" + %d", r);
            proof_add("$}, with $%c \\in %s$.\n\\begin{itemize}\n", quotient, written);

            /* x legata a m t + r, e t generica. */
            split_case = r;
            universal[universal_count++] = (Bound){quotient, &part};
            env_push(f->variable, rat_add(rat_mul(rat_int(m), rat_atom(atom_var(quotient))), rat_int(r), 0));
            holds = establish(f->a, negated);
            env_pop();
            universal_count--;
            proof_add("\\end{itemize}\n");
        }
        split_modulus = split_case = 0;
        if (holds) {
            f->modulus = m;
            f->quotient = quotient;
            search_left += outside;
            return 1;
        }
        rollback(mark);
    }
    search_left += outside;
    return 0;
}


/* Elenca in list[], a partire dal posto count, le uguaglianze di f (negata
 * se negated) da cui si può ricavare il valore di una variabile, e ritorna il
 * nuovo numero di elementi; list ha MAX_EQUATIONS posti.
 *
 * In una congiunzione ogni uguaglianza deve valere, quindi il valore cercato
 * la soddisfa: è una condizione necessaria. In una disgiunzione basta che
 * valga una parte, quindi un'uguaglianza di una parte non è necessaria; ma
 * risolverla è un modo di rendere vera la condizione ("esiste y con A o B"
 * vale se esiste y con A). In tutti e due i casi il valore trovato è solo
 * una proposta: chi chiama ricontrolla l'intera condizione.
 *
 * Non entra nei quantificatori: un'uguaglianza che sta sotto un "per ogni y"
 * più interno dipende da y, e risolverla darebbe un valore che cambia con y. */
static int find_equations(Formula *f, int negated, const Formula **list, int count) {
    Junction j;
    if (f->kind == F_NOT) return find_equations(f->a, !negated, list, count);
    if (f->kind == F_EQUAL || f->kind == F_DISTINCT) {
        if ((f->kind == F_EQUAL) != negated && count < MAX_EQUATIONS) list[count++] = f;
        return count;
    }
    if (!junction(f, negated, &j)) return count;
    return find_equations(j.y, j.ny, list, find_equations(j.x, j.nx, list, count));
}

/* 18i, secondo caso. L'uguaglianza `equation`, richiesta dalla condizione del
 * quantificatore f, ha per numeratore il polinomio num, che non è di primo
 * grado in x. Se num contiene solo x, con coefficienti numerici, i valori
 * possibili di x sono le sue radici, e quelle razionali si trovano tutte
 * (vedi rational_roots()): si prova ciascuna di quelle che stanno
 * nell'insieme. Scrive il ragionamento e ritorna 1 se una funziona.
 *
 * È ciò che permette di trovare x = 41 per x^2 = 1681, che nessun elenco dei
 * "numeri più semplici" (18j) raggiungerebbe. cleared dice se num è stato
 * ottenuto togliendo denominatori. */
static int solve_by_roots(Formula *f, int negated, const Formula *equation, Poly num, int cleared) {
    long long c[MAX_UNIVARIATE + 1], scale;
    Frac root[MAX_UNIVARIATE + 1];
    int degree = poly_coefficients(num, atom_var(f->variable), c, &scale);
    int roots = degree >= 2 ? rational_roots(c, degree, root) : 0;
    if (roots >= 0 && degree >= 2 && !roots_confirmed(num, atom_var(f->variable), root, roots, &f->numbers)) return 0;
    for (int k = 0; k < roots; k++) {
        Mark mark = mark_here();
        if (!numbers_contain(&f->numbers, root[k])) continue;
        proof_add("\\item We look for $%c$ with $", f->variable);
        put_source(equation);
        proof_add("$. %s this equation is $", cleared ? "After clearing denominators," : "Written out,");
        put_poly(num);
        proof_add(" = 0$, whose rational roots are $");
        for (int j = 0; j < roots; j++) {
            if (j) proof_add(",\\ ");
            put_frac(root[j]);
        }
        proof_add("$. Take $%c = ", f->variable);
        put_frac(root[k]);
        proof_add("$, an element of $");
        put_numbers(&f->numbers);
        proof_add("$.\n");

        /* La condizione, con x legata alla radice. */
        choose(f->variable, root[k]);
        env_push(f->variable, rat_frac(root[k]));
        int holds = establish(f->a, negated);
        env_pop();
        if (holds) { f->has_witness |= 1u << split_case; f->witness[split_case] = rat_frac(root[k]); return 1; }
        rollback(mark);
    }
    return 0;
}

/* 18i. Prova a dimostrare "esiste x in A tale che ..." risolvendo
 * l'uguaglianza `equation`, una di quelle di find_equations(). f è il nodo
 * del quantificatore e negated il segno con cui va letta la sua condizione.
 * Scrive il ragionamento e ritorna 1 se ci riesce; se ritorna 0 il testo
 * scritto va cancellato da chi chiama.
 *
 * Se l'uguaglianza, tolti i denominatori, è di grado più alto in x, la si passa a solve_by_roots(). Se è di primo grado,
 * cioè alpha x + beta = 0 con alpha e beta senza x, l'unico valore possibile
 * è x = -beta/alpha. Va controllato tutto ciò che questo
 * sottintende: che alpha non si annulli mai, che il valore stia in A, e
 * infine che con quel valore la condizione valga davvero. Quest'ultimo
 * controllo ricalcola tutto da capo, quindi non si fida dei passaggi fatti
 * per ricavare x. */
static int solve_with(Formula *f, int negated, const Formula *equation) {
    int x = atom_var(f->variable);
    Poly num, den, part[2], shown_num, shown_den;
    Num number;
    if (failure) return 0;

    /* L'equazione come alpha x + beta = 0. Qui x non è legata: è un atomo.
     *
     * È un conto fatto per tentativo, senza aver controllato i divisori: con
     * i valori dati alle altre variabili può capitare una divisione per zero.
     * Vuol dire solo che così non si ricava nulla, quindi l'errore si
     * cancella; all'ingresso `failure` era NULL, e non si perde altro. */
    Rat gap = rat_add(eval(equation->left), eval(equation->right), 1);
    if (failure) { failure = NULL; return 0; }
    if (!rat_polynomials(gap, &num, &den)) return 0;
    if (poly_split(num, x, part, 1) != 1) return solve_by_roots(f, negated, equation, num, !poly_is_one(den));
    Poly alpha = part[1], minus_beta = poly_negate(part[0]);
    Rat solution = rat_make(minus_beta, alpha);
    if (!rat_polynomials(solution, &shown_num, &shown_den)) return 0;

    /* Controllo del passo: alpha x - (-beta) deve ridare il numeratore
     * dell'equazione, che il testo sta per riscrivere in quella forma. */
    Poly linear = poly_add(poly_mul(alpha, rat_atom(x).num), minus_beta, 1);
    if (failure) return 0;
    if (!poly_equal(linear, num)) { unjustified_steps++; return 0; }

    proof_add("\\item We look for $%c$ with $", f->variable);
    put_source(equation);
    proof_add("$. %s this equation is $", poly_is_one(den) ? "Written out," : "After clearing denominators,");
    if (!poly_is_one(alpha)) put_factor(alpha);
    proof_add("%c = ", f->variable);
    put_poly(minus_beta);
    proof_add("$.");

    /* Si può dividere per alpha solo se non si annulla mai. */
    if (!poly_is_constant(alpha)) {
        proof_add(" The coefficient of $%c$ is never zero.", f->variable);
        if (!zero_only_where(alpha, NULL)) return 0;
    }

    /* Il valore, e la sua appartenenza ad A. */
    proof_add("\n\\item So take $%c = ", f->variable);
    put_ratio(shown_num, shown_den);
    proof_add("$. It has to be an element of $");
    put_numbers(&f->numbers);
    proof_add("$.");
    if (!always_in(solution, &f->numbers)) return 0;
    proof_add("\n");

    /* La condizione, con x legata al valore trovato. */
    if (rat_number(solution, &number)) choose(f->variable, number.re);
    env_push(f->variable, solution);
    int holds = establish(f->a, negated);
    env_pop();
    if (holds) { f->has_witness |= 1u << split_case; f->witness[split_case] = solution; }
    return holds;
}

/* 18i. Prova a dimostrare "esiste x in A tale che ..." risolvendo, una dopo
 * l'altra, le uguaglianze da cui x si può ricavare. Ritorna 1 se una
 * funziona; se ritorna 0 non lascia testo né scelte. */
static int solve_for(Formula *f, int negated) {
    const Formula *equation[MAX_EQUATIONS];
    int count = find_equations(f->a, negated, equation, 0);
    for (int i = 0; i < count && !failure && search_left > 0; i++) {
        Mark mark = mark_here();
        if (solve_with(f, negated, equation[i])) return 1;
        rollback(mark);
    }
    return 0;
}

/* Dimostra la condizione f, o la sua negazione se negated, per ogni valore
 * delle variabili generiche e con i valori dati alle altre. Scrive il
 * ragionamento come una serie di \item e ritorna 1 se ci riesce; se ritorna 0
 * non lascia testo né scelte. Le regole sono descritte in testa al capitolo. */
static int establish(Formula *f, int negated) {
    Mark mark = mark_here();
    Literal literal[MAX_LITERALS];
    Junction j;
    int count = 0;
    if (failure || search_left <= 0) return 0;
    search_left--;
    if (f->kind == F_NOT) return establish(f->a, !negated);

    /* 18g, 18h. Un confronto, o una disgiunzione di confronti. */
    if (gather(f, negated, literal, &count)) {
        /* I polinomi che prove_clause() calcola servono solo a lei: resta il
         * testo che ha scritto. La memoria che ha preso si può ridare, a
         * meno che non sia nata la base di una potenza o l'argomento di
         * un'arcotangente, che restano nelle tabelle degli atomi. Senza
         * questo passo una ricerca lunga riempie l'arena molto prima di
         * finire i tentativi. */
        size_t before = arena_used;
        int polys = base_poly_count, atans = atan_count;
        proof_add("\\item");
        int proved = prove_clause(literal, count);
        if (polys == base_poly_count && atans == atan_count && failure != OUT_OF_MEMORY) arena_used = before;
        if (proved) { proof_add("\n"); return 1; }
        rollback(mark);
        return 0;
    }

    /* 18l. Congiunzione: tutte e due le parti. Disgiunzione che non è fatta
     * di soli confronti: una parte basta, e si prova prima l'una poi l'altra. */
    if (junction(f, negated, &j)) {
        if (j.conjunction) {
            if (establish(j.x, j.nx) && establish(j.y, j.ny)) return 1;
            rollback(mark);
            return 0;
        }
        for (int side = 0; side < 2; side++) {
            Formula *part = side ? j.y : j.x;
            int sign = side ? j.ny : j.nx;
            proof_add("\\item It is enough to show that $");
            put_source(part);
            proof_add("$ %s.\n", sign ? "fails" : "holds");
            if (establish(part, sign)) return 1;
            rollback(mark);
        }
        return 0;
    }

    /* 18l. Equivalenza: le due implicazioni. La sua negazione: una delle due
     * implicazioni non vale. I due nodi provvisori servono solo a passare le
     * implicazioni a questa stessa funzione. */
    if (f->kind == F_IFF) {
        Formula forward = *f, backward = *f;
        forward.kind = backward.kind = F_IMPLIES;
        backward.a = f->b;
        backward.b = f->a;
        if (negated) {
            if (establish(&forward, 1) || establish(&backward, 1)) return 1;
        } else {
            proof_add("\\item ($\\Rightarrow$)\n");
            if (establish(&forward, 0)) {
                proof_add("\\item ($\\Leftarrow$)\n");
                if (establish(&backward, 0)) return 1;
            }
        }
        rollback(mark);
        return 0;
    }
    if (f->kind != F_FORALL && f->kind != F_EXISTS) return 0;

    /* 18k. "Per ogni": variabili generiche. Più quantificatori di seguito
     * sullo stesso insieme si introducono insieme, come in "siano a, b". */
    if ((f->kind == F_FORALL) != negated) {
        Formula *last = f;
        int outer = universal_count;
        f->modulus = 0;
        proof_add("\\item Let $%c", f->variable);
        universal[universal_count++] = (Bound){f->variable, &f->numbers};
        while (last->a->kind == f->kind && numbers_equal(&last->a->numbers, &f->numbers)) {
            last = last->a;
            proof_add(", %c", last->variable);
            universal[universal_count++] = (Bound){last->variable, &last->numbers};
        }
        proof_add(" \\in ");
        put_numbers(&f->numbers);
        proof_add("$.\n");
        int holds = establish(last->a, negated);
        universal_count = outer;
        if (holds) return 1;

        /* Le due regole che seguono valgono per una variabile sola, intera.
         * Il tentativo diretto può essersi fermato su un conto che con una
         * lettera non si fa, come una somma fino a n: non è un errore
         * dell'enunciato. */
        const NumberSet *set = &f->numbers;
        if (last != f || !set->integers || set->holes || search_left <= 0) { rollback(mark); return 0; }
        failure = NULL;

        /* 18n. Per induzione: un'uguaglianza, su una semiretta di interi,
         * senza altre variabili in gioco. */
        const Formula *body = f->a;
        int sign = negated;
        while (body->kind == F_NOT) { body = body->a; sign = !sign; }
        if (set->bounded && outer == 0 && env_count == 0 && (body->kind == F_EQUAL || body->kind == F_DISTINCT) &&
            (body->kind == F_EQUAL) != sign) {
            rollback(mark);
            if (by_induction(f, body)) return 1;
        }

        /* 18o. Per casi sul resto di una divisione. */
        if (by_remainders(f, negated)) return 1;
        failure = NULL;
        rollback(mark);
        return 0;
    }

    /* 18i. "Esiste": prima risolvendo un'equazione. Un valore scelto in un
     * tentativo precedente, per questo stesso caso, non conta più. */
    f->has_witness &= ~(1u << split_case);
    if (solve_for(f, negated)) return 1;
    rollback(mark);

    /* 18j. Poi provando i numeri più semplici dell'insieme. Il valore è un
     * numero fisso anche se ci sono variabili generiche: va bene quando lo
     * stesso numero serve per tutti i loro valori. */
    Frac sample[SAMPLE_SIZE];
    numbers_sample(&f->numbers, sample, SAMPLE_SIZE);
    for (int i = 0; i < SAMPLE_SIZE && !failure; i++) {
        proof_add("\\item Take $%c = ", f->variable);
        put_frac(sample[i]);
        proof_add("$, an element of $");
        put_numbers(&f->numbers);
        proof_add("$.\n");
        choose(f->variable, sample[i]);
        env_push(f->variable, rat_frac(sample[i]));
        int holds = establish(f->a, negated);
        env_pop();
        if (holds) { f->has_witness |= 1u << split_case; f->witness[split_case] = rat_frac(sample[i]); return 1; }
        rollback(mark);
    }
    return 0;
}

/* 18m. Dimostra che la regola di map definisce un'applicazione dal dominio al
 * codominio: per ogni x del dominio il valore esiste, perché nessun divisore
 * si annulla, e appartiene al codominio. Scrive il ragionamento come un \item
 * e ritorna 1 se ci riesce.
 *
 * Senza questo passo le ipotesi dell'enunciato potrebbero non essere
 * soddisfatte da nessuna applicazione, e allora qualunque tesi ne seguirebbe:
 * un verdetto FALSO sarebbe sbagliato, e uno DIMOSTRATO non direbbe nulla. */
static int map_defined(const Map *map) {
    Poly num, den;
    proof_add("\\item \\emph{The rule defines a map from $");
    put_numbers(&map->domain);
    proof_add("$ to $");
    put_numbers(&map->codomain);
    proof_add("$.} Let $%c \\in ", map->parameter);
    put_numbers(&map->domain);
    proof_add("$.");

    /* La variabile della regola è generica nel dominio. */
    universal[0] = (Bound){map->parameter, &map->domain};
    universal_count = 1;
    int defined = term_defined(map->body);
    Rat value = eval(map->body);
    if (defined && rat_polynomials(value, &num, &den)) {
        proof_add(" The value is $");
        put_ratio(num, den);
        proof_add("$.");
        defined = always_in(value, &map->codomain);
    } else {
        defined = 0;
    }
    universal_count = 0;
    proof_add("\n");
    return defined && !failure;
}

/* Dimostra la tesi f dividendola nelle affermazioni di cui è la congiunzione:
 * ognuna ha il suo titolo, con il testo dell'enunciato, e sotto i suoi passi.
 * Ritorna 1 se le dimostra tutte. */
static int prove_claims(Formula *f) {
    if (f->kind == F_AND) return prove_claims(f->a) && prove_claims(f->b);
    proof_add("\\item \\emph{Claim:} $");
    put_source(f);
    proof_add("$.\n\\begin{itemize}\n");
    int holds = establish(f, 0);
    proof_add("\\end{itemize}\n");
    return holds;
}

/* =============================================================================
 * 19. QUANTIFICATORI: VERIFICA INDIPENDENTE E VERDETTO
 * ============================================================================
 *
 * Verifica indipendente. Il capitolo 18 ragiona con i polinomi: scompone,
 * risolve, studia segni. Qui lo stesso enunciato viene ricontrollato senza
 * algebra, calcolando con i numeri: ogni "per ogni" si prova sui primi
 * elementi dell'insieme, ogni "esiste" con il valore scelto dalla
 * dimostrazione (quello del caso giusto, se la dimostrazione è per casi sul
 * resto), e ogni confronto è un confronto tra due numeri. Se la
 * dimostrazione dice x = (y + 1)/(y - 1), per ogni y del campione si calcola
 * quel numero, si guarda se sta nell'insieme e se soddisfa la condizione.
 *
 * Come per gli insiemi (capitolo 16) non è una seconda dimostrazione, perché
 * il campione è finito: è un controllo contro gli errori del programma. Se
 * non conferma la dimostrazione, il verdetto non viene emesso.
 */

/* Calcola il valore che la dimostrazione ha scelto per la variabile del
 * quantificatore f, ora che le variabili più esterne sono legate a numeri: le
 * sostituisce una per volta nell'espressione scelta. Mette il numero in
 * *value e ritorna 1; ritorna 0 se non ne esce un numero razionale. */
static int witness_value(const Formula *f, Frac *value) {
    Rat chosen_value = f->witness[split_case];
    Num number;
    for (int i = 0; i < env_count && !failure; i++) {
        Poly num, den;
        int x = atom_var(env[i].name);
        if (!rat_polynomials(chosen_value, &num, &den)) return 0;
        chosen_value = rat_div(poly_substitute(num, x, env[i].value), poly_substitute(den, x, env[i].value));
    }
    if (failure || !rat_number(chosen_value, &number) || number.im.num != 0) return 0;
    *value = number.re;
    return 1;
}

/* Quanti elementi di ogni insieme prova la verifica: SAMPLE_SIZE, oppure meno
 * quando i quantificatori annidati sono tanti. Lo fissa decide_quantified(). */
static int sample_width;

/* Ritorna 1 se la condizione f, negata se negated, risulta vera calcolando
 * con i numeri sul campione, come descritto in testa al capitolo. Tutte le
 * variabili visibili in f devono essere legate a numeri.
 *
 * I conti non lasciano nulla che serva dopo, solo un sì o un no: ogni volta
 * la memoria usata viene restituita all'arena riportando arena_used dov'era.
 * Senza questo passo, le migliaia di confronti di un enunciato con molte
 * variabili riempivano l'arena, e un enunciato vero restava non confermato. */
static int sample_holds(const Formula *f, int negated) {
    Frac sample[SAMPLE_SIZE], value;
    Num left, right;
    size_t mark = arena_used;
    if (failure) return 0;

    switch (f->kind) {
    case F_NOT:
        return sample_holds(f->a, !negated);
    case F_MAP:
        return !negated;
    case F_EQUAL: case F_DISTINCT: {
        /* Espressioni non razionali: come nella dimostrazione, il confronto
         * conta solo se i due membri esistono (closed_gap()). Gli atomi nati
         * dal conto, come la radice di 2, vanno tolti insieme alla memoria:
         * le loro basi vi abitano. */
        if (!term_is_rational(f->left) || !term_is_rational(f->right)) {
            Scratch before = scratch_mark();
            int equal = closed_gap(f);
            scratch_drop(before);
            return equal >= 0 && (equal == (f->kind == F_EQUAL)) != negated;
        }
        int numbers = rat_number(eval(f->left), &left) && rat_number(eval(f->right), &right);
        arena_used = mark;
        return numbers && (num_equal(left, right) == (f->kind == F_EQUAL)) != negated;
    }
    case F_AND:
        return negated ? sample_holds(f->a, 1) || sample_holds(f->b, 1) : sample_holds(f->a, 0) && sample_holds(f->b, 0);
    case F_OR:
        return negated ? sample_holds(f->a, 1) && sample_holds(f->b, 1) : sample_holds(f->a, 0) || sample_holds(f->b, 0);
    case F_IMPLIES:
        return negated ? sample_holds(f->a, 0) && sample_holds(f->b, 1) : sample_holds(f->a, 1) || sample_holds(f->b, 0);
    case F_IFF: {
        /* Le due implicazioni, come in establish(). */
        int forward = sample_holds(f->a, 1) || sample_holds(f->b, 0);
        int backward = sample_holds(f->b, 1) || sample_holds(f->a, 0);
        if (!negated) return forward && backward;
        return (sample_holds(f->a, 0) && sample_holds(f->b, 1)) || (sample_holds(f->b, 0) && sample_holds(f->a, 1));
    }
    case F_FORALL: case F_EXISTS:
        break;
    }

    /* "Esiste": il valore scelto dalla dimostrazione, e solo quello. Deve
     * essere un numero dell'insieme e soddisfare la condizione.
     *
     * Se non regge non se ne cerca un altro: la dimostrazione ha affermato
     * qualcosa di quel valore, ed è quello che va controllato. Prima la
     * verifica ripiegava sugli elementi del campione, e una dimostrazione
     * sbagliata ("nessun x dà y = -2") veniva confermata trovando un altro y
     * la cui controimmagine era semplicemente fuori dal campione. */
    int every = (f->kind == F_FORALL) != negated;
    if (!every && (f->has_witness >> split_case & 1)) {
        int holds = witness_value(f, &value) && numbers_contain(&f->numbers, value);
        arena_used = mark;
        if (holds) {
            env_push(f->variable, rat_frac(value));
            holds = sample_holds(f->a, negated);
            env_pop();
            arena_used = mark;
        }
        return holds;
    }

    /* "Per ogni": tutti gli elementi del campione. "Esiste" per cui la
     * dimostrazione non ha scelto un valore, perché non le è servito: almeno
     * uno. */
    numbers_sample(&f->numbers, sample, sample_width);
    for (int i = 0; i < sample_width; i++) {
        /* Dimostrato per casi sul resto (18o): il numero dice in quale caso
         * si è, e quanto vale il quoziente che la dimostrazione ha usato. */
        int cases = every && f->modulus && sample[i].den == 1, outer = split_case;
        env_push(f->variable, rat_frac(sample[i]));
        if (cases) {
            long long r = ((sample[i].num % f->modulus) + f->modulus) % f->modulus;
            env_push(f->quotient, rat_int((sample[i].num - r) / f->modulus));
            split_case = (int)r;
        }
        int holds = sample_holds(f->a, negated);
        if (cases) env_pop();
        split_case = outer;
        env_pop();
        arena_used = mark;
        if (holds != every) return holds;
    }
    return every;
}

/* Dimentica ciò che una dimostrazione ha annotato nei nodi di f: i valori
 * scelti e i casi sul resto. Va chiamata prima di riusare la memoria in cui
 * quei valori stanno. */
static void forget_choices(Formula *f) {
    if (!f) return;
    f->has_witness = 0;
    f->modulus = 0;
    forget_choices(f->a);
    forget_choices(f->b);
}

/* Ritorna 1 se, sul campione del dominio, la regola di map dà sempre un
 * numero che sta nel codominio. */
static int map_holds_on_sample(const Map *map) {
    Frac sample[SAMPLE_SIZE];
    numbers_sample(&map->domain, sample, SAMPLE_SIZE);
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        Num value;
        env_push(map->parameter, rat_frac(sample[i]));
        Rat image = eval(map->body);
        env_pop();
        if (failure || !rat_number(image, &value) || value.im.num != 0 || !numbers_contain(&map->codomain, value.re)) return 0;
    }
    return 1;
}

/* Il testo delle scelte fatte, per esempio "a = 0, b = 3". */
static char choice_text[256];

/* Scrive in choice_text i numeri scelti dalla dimostrazione e ne ritorna
 * l'indirizzo, oppure NULL se non ce ne sono. */
static const char *describe_choices(void) {
    size_t length = 0;
    if (!chosen_count || chosen_by_cases) return NULL;
    for (int i = 0; i < chosen_count; i++) {
        Frac v = chosen[i].value;
        if (length < sizeof choice_text)
            length += (size_t)snprintf(choice_text + length, sizeof choice_text - length, "%s%c = %lld", i ? ", " : "", chosen[i].name, v.num);
        if (v.den != 1 && length < sizeof choice_text)
            length += (size_t)snprintf(choice_text + length, sizeof choice_text - length, "/%lld", v.den);
    }
    return choice_text;
}

/* Il lavoro di prove_quantified(), che ne descrive il contratto. Al ritorno
 * la tabella `maps` contiene ancora le applicazioni dell'enunciato. */
static Verdict decide_quantified(const char *formula) {
    Verdict verdict = {UNSUPPORTED, 0, 0, 0, NULL, NULL, NULL, "con le regole sui quantificatori", NULL, NULL};

    /* Ripartenza da zero e parsing. */
    proof_reset();
    arena_used = arena_keep;
    atom_count = 0;
    failure = NULL;
    env_count = 0;
    map_count = 0;
    syntax_error = NULL;
    cursor = formula;
    next_token();
    Formula *thesis = logic_formula();
    if (token.kind != T_END) syntax("testo inatteso dopo l'enunciato");

    /* Ipotesi e tesi, poi i controlli che la grammatica non fa. */
    if (!syntax_error && !failure) {
        if (map_count > 0 && thesis->kind == F_IMPLIES) {
            read_definitions(thesis->a);
            thesis = thesis->b;
        }
        for (int i = 0; i < map_count; i++)
            if (!maps[i].body) fail("un'applicazione è dichiarata senza la regola che la definisce");
        quantifier_depth = 0;
        if (!failure) formula_check(thesis);
    }

    /* Quanti valori provare per ogni variabile nella verifica: con d
     * quantificatori annidati le combinazioni sono larghezza^d, e si scende
     * finché non stanno in MAX_CHECKS. Mai meno di due valori. */
    sample_width = SAMPLE_SIZE;
    for (;;) {
        long long combinations = 1;
        for (int i = 0; i < quantifier_depth; i++) combinations *= sample_width;
        if (combinations <= MAX_CHECKS || sample_width == 2) break;
        sample_width--;
    }

    /* Prima la tesi, poi la sua negazione. Il testo si riscrive da capo ogni
     * volta, perché comincia in modo diverso nei due casi. Anche la memoria
     * riparte da dov'era dopo il parsing: un primo tentativo che la riempie
     * non deve togliere al secondo la possibilità di riuscire. */
    int stopped = 0;
    size_t parsed = arena_used;
    for (int negated = 0; negated < 2 && !syntax_error && !failure; negated++) {
        proof_reset();
        forget_choices(thesis);
        arena_used = parsed;
        atom_count = base_poly_count = atan_count = 0;
        chosen_count = 0;
        chosen_by_cases = 0;
        universal_count = 0;
        split_modulus = split_case = 0;
        search_left = MAX_SEARCH;
        proof_add("\\textbf{%s}\n\\begin{itemize}\n", negated ? "The statement is false." : "Proof.");

        /* 18m. Le regole definiscono davvero delle applicazioni? Se no, si
         * guarda sul campione se la regola è sbagliata o se è solo il
         * programma a non saperlo dimostrare. Il conto sul campione può
         * incontrare una divisione per zero, che qui è una risposta e non
         * un errore: per questo `failure` viene azzerato. */
        for (int i = 0; i < map_count; i++) {
            if (map_defined(&maps[i])) continue;
            int plausible = !failure && map_holds_on_sample(&maps[i]);
            failure = NULL;
            verdict.outcome = NOT_PROVED;
            verdict.reason = plausible ? "non riesco a dimostrare che la regola dà sempre un valore nel codominio"
                                       : "la regola non definisce un'applicazione: per qualche elemento del dominio il valore manca o cade fuori dal codominio";
            return verdict;
        }

        /* Le regole del capitolo 18. */
        int found;
        if (negated) {
            proof_add("\\item \\emph{The conclusion fails.}\n\\begin{itemize}\n");
            found = establish(thesis, 1);
            proof_add("\\end{itemize}\n");
        } else {
            found = prove_claims(thesis);
        }
        /* Una ricerca che finisce i tentativi, o la memoria, è una ricerca
         * interrotta: non dice nulla sull'enunciato. */
        if (search_left <= 0 || failure == OUT_OF_MEMORY) stopped = 1;
        if (failure == OUT_OF_MEMORY) failure = NULL;
        else if (!found || failure) continue;
        if (!found) continue;
        proof_add("\\end{itemize}\n$\\Box$");

        /* Verifica indipendente. Come sopra, un errore di calcolo qui è una
         * mancata conferma. */
        int confirmed = sample_holds(thesis, negated);
        for (int i = 0; i < map_count; i++) confirmed = confirmed && map_holds_on_sample(&maps[i]);
        if (failure || !confirmed) {
            failure = NULL;
            verdict.outcome = NOT_PROVED;
            verdict.reason = "la verifica indipendente, su un campione di numeri, non conferma la dimostrazione";
        } else {
            verdict.outcome = negated ? REFUTED : PROVED;
            verdict.detail = negated ? describe_choices() : NULL;
            verdict.proof = proof_text;
        }
        return verdict;
    }

    /* Notazione o conto fuori portata, oppure nessuna delle due prove
     * riuscita. */
    if (syntax_error || failure) {
        verdict.reason = syntax_error ? syntax_error : failure;
        return verdict;
    }
    verdict.outcome = NOT_PROVED;
    verdict.reason = stopped ? "la ricerca è stata interrotta: troppi tentativi senza arrivare a una dimostrazione"
                             : "le regole disponibili non bastano né a dimostrare né a smentire la tesi";
    return verdict;
}

/* Decide l'enunciato con quantificatori scritto in formula e ne ritorna il
 * verdetto, con la dimostrazione scritta.
 *
 * Se l'enunciato dichiara delle applicazioni deve avere la forma "ipotesi
 * implica tesi", con le dichiarazioni e le regole come ipotesi; altrimenti è
 * tutto tesi. Si prova a dimostrare la tesi e, se non riesce, la sua
 * negazione: nel primo caso il verdetto è DIMOSTRATO, nel secondo FALSO.
 *
 * Svuota l'arena e la tabella degli atomi: i risultati di chiamate precedenti
 * non sono più validi. */
static Verdict prove_quantified(const char *formula) {
    Verdict verdict = decide_quantified(formula);

    /* Le applicazioni dichiarate valgono solo per questo enunciato: il parser
     * delle espressioni, che serve anche agli altri metodi, non deve più
     * vederle (vedi `maps`, capitolo 8). */
    map_count = 0;
    return verdict;
}

/* =============================================================================
 * 20. DIMOSTRARE UN ENUNCIATO
 * ============================================================================
 *
 * Il punto d'ingresso unico: sceglie il metodo guardando la notazione.
 *
 * Le convenzioni, nell'ordine in cui si applicano. Un enunciato che nomina un
 * insieme numerico, con \mathbb, va alle regole sui quantificatori: ogni sua
 * lettera deve essere quantificata su uno di quegli insiemi, quindi non c'è
 * modo di leggerlo in un altro senso. Tra gli altri, una lettera maiuscola
 * senza pedice è un insieme: un enunciato che ne contiene una, o che usa un
 * simbolo proprio degli insiemi, va alla decisione per regioni. Ogni altro va
 * all'induzione, dove le maiuscole compaiono solo con un pedice, come F_{k}.
 *
 * Guardare solo i simboli non basta, e ha già causato una dimostrazione falsa:
 * S \times T = T \times S non contiene né unioni né inclusioni, veniva letto
 * come prodotto di due numeri, e come tale risultava vero. Per i prodotti
 * cartesiani è falso. Il caso è ora nei test di antiregressione.
 */

/* Ritorna 1 se formula è un enunciato sugli insiemi, secondo la convenzione
 * descritta in testa al capitolo. Usa il lettore di token del parser, e lascia
 * il suo stato da reimpostare. */
static int uses_set_notation(const char *formula) {
    static const char *const commands[] = {"cup", "cap", "setminus", "subseteq", "supseteq", "emptyset",
                                            "varnothing", "mathcal", "triangle", "exists"};
    int found = 0;
    syntax_error = NULL;
    cursor = formula;
    next_token();
    while (token.kind != T_END && !found) {
        if (token.kind == T_LETTER && isupper(token.ch)) {
            /* Una maiuscola è un insieme se non è seguita da un pedice. */
            next_token();
            found = !is_char('_');
            continue;
        }
        for (size_t i = 0; token.kind == T_COMMAND && i < sizeof commands / sizeof *commands; i++)
            if (strcmp(token.name, commands[i]) == 0) found = 1;
        next_token();
    }
    syntax_error = NULL;
    return found;
}

/* Prova a dimostrare l'enunciato scritto in formula.
 *
 * variable, has_base e base riguardano le dimostrazioni per induzione (vedi
 * prove_by_induction()) e sono ignorati per gli altri enunciati.
 *
 * I risultati di una chiamata precedente, comprese le stringhe del suo
 * verdetto, non sono più validi dopo questa. */
static Verdict prove(const char *formula, int variable, int has_base, long long base) {
    if (strstr(formula, "\\mathbb")) return prove_quantified(formula);
    if (uses_set_notation(formula)) return prove_sets(formula);
    return prove_by_induction(formula, variable, has_base, base);
}

/* Scrive su out il verdetto in una riga, senza a capo. */
static void verdict_print(FILE *out, const Verdict *verdict) {
    switch (verdict->outcome) {
    case PROVED:
        if (verdict->variable) fprintf(out, "DIMOSTRATO per induzione su %c >= %lld", verdict->variable, verdict->base);
        else fprintf(out, "DIMOSTRATO %s", verdict->method);

        /* Il certificato: c'è, oppure manca, e se si sa perché lo si dice.
         * Un certificato con una voce "generic" dimostra meno dell'enunciato
         * (vedi 13h), e anche questo va detto. */
        if (!verdict->certificate) fprintf(out, verdict->reason ? "; senza certificato (%s)" : "; senza certificato", verdict->reason);
        else if (strstr(verdict->certificate, "\ngeneric ")) fprintf(out, "; certificato verificato con un'ipotesi in più sui parametri");
        else fprintf(out, "; certificato verificato");
        break;
    case REFUTED:
        if (verdict->variable) fprintf(out, "FALSO: non vale per %c = %lld", verdict->variable, verdict->witness);
        else if (verdict->detail) fprintf(out, "FALSO: controesempio %s", verdict->detail);
        else fprintf(out, "FALSO: è dimostrata la sua negazione");
        if (verdict->note) fprintf(out, " (%s)", verdict->note);
        break;
    case NOT_PROVED:
        fprintf(out, "NON DIMOSTRATO: %s", verdict->reason);
        break;
    case UNSUPPORTED:
        fprintf(out, "NON SUPPORTATO: %s", verdict->reason);
        break;
    }
}

/* =============================================================================
 * 21. LETTURA DEGLI ENUNCIATI DA UN FILE .tex
 * ============================================================================
 *
 * Il formato è quello dei file in tests/: un documento LaTeX in cui ogni
 * \item di un elenco è un enunciato. Un \item vale come enunciato se comincia
 * con una formula in evidenza, in una di queste due forme:
 *
 *     \item $\displaystyle FORMULA$
 *     \item \[ FORMULA \]
 *
 * Il testo che segue la formula nello stesso \item può indicare da dove parte
 * l'induzione, per esempio "per ogni intero $n\ge 2$": la prima formula
 * "lettera \ge numero" dà la variabile di induzione e il caso base. In sua
 * assenza la variabile è n e il caso base lo sceglie prove(). Del resto di
 * quel testo il programma capisce solo le parole di "per ogni intero ... con
 * ...", e riconosce come commento un inciso tra parentesi che non restringe
 * nulla, come "(nucleo di Dirichlet)". Ogni altra cosa è una condizione che
 * non sa leggere, che sia una formula come "$a>0$" o una parola come "pari":
 * l'enunciato può ancora essere dimostrato, ma non dichiarato falso. Una formula con un segno = è
 * un'altra uguaglianza, e la risposta è NON SUPPORTATO: l'\item non è un solo
 * enunciato.
 *
 * Un \item che comincia con del testo, o con una formula in linea senza
 * \displaystyle, è un enunciato in prosa: non c'è una formula sola da
 * dimostrare e la risposta è NON SUPPORTATO.
 */

/* Conteggio dei verdetti su un file. */
typedef struct {
    int total;        /* \item trovati */
    int proved;
    int certified;    /* tra i dimostrati, quelli con il certificato verificato */
    int refuted;
    int not_proved;
    int unsupported;
} Report;

/* Legge tutto il file path in un blocco terminato da '\0', da liberare con
 * free(). Ritorna NULL se il file non si può leggere. */
static char *read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) return NULL;
    size_t size = 0, capacity = 1 << 16;
    char *text = malloc(capacity);
    while (text) {
        size += fread(text + size, 1, capacity - size - 1, file);
        if (size < capacity - 1) break;
        capacity *= 2;
        char *larger = realloc(text, capacity);
        if (!larger) free(text);
        text = larger;
    }
    fclose(file);
    if (text) text[size] = 0;
    return text;
}

/* Le parole che, nel testo dopo una formula, non dicono nulla dell'enunciato:
 * quelle di "per ogni intero ... con ...", in italiano e in inglese. Ogni
 * altra parola può essere una condizione, come "pari" o "primo", e il
 * programma non la capisce. */
static const char *const plain_words[] = {"per", "ogni", "intero", "interi", "con", "e", "ed",
                                          "for", "every", "all", "integer", "integers", "with", "and"};

/* Le parole che, dentro un inciso tra parentesi, fanno pensare a una
 * condizione e non a un commento: quelle che la introducono ("per", "se",
 * "quando") e quelle che restringono i casi ("pari", "primo", "positivo"). */
static const char *const restricting_words[] = {
    "per", "con", "se", "quando", "dove", "purché", "tranne", "solo", "ogni", "tutti", "caso", "casi",
    "pari", "dispari", "primo", "primi", "positivo", "positivi", "negativo", "negativi", "grande", "grandi",
    "piccolo", "piccoli", "reale", "reali", "intero", "interi", "naturale", "naturali",
    "for", "with", "if", "when", "where", "whenever", "provided", "unless", "except", "only", "every", "all", "any",
    "case", "cases", "even", "odd", "prime", "positive", "negative", "large", "small", "real", "integer", "integers",
    "natural"};

/* Ritorna 1 se la parola [word, stop) è una delle `count` di list, senza
 * distinguere maiuscole e minuscole nelle lettere non accentate. */
static int word_in(const char *word, const char *stop, const char *const *list, size_t count) {
    for (size_t i = 0; i < count; i++) {
        const char *w = list[i], *p = word;
        while (p < stop && *w && tolower((unsigned char)*p) == (unsigned char)*w) { p++; w++; }
        if (p == stop && !*w) return 1;
    }
    return 0;
}

/* Ritorna 1 se il testo [from, to), il contenuto di una coppia di parentesi,
 * è un commento e non una condizione: il nome di una formula, come "nucleo
 * di Dirichlet". Deve essere fatto di sole parole, almeno una, nessuna delle
 * quali sia tra le restricting_words o sia una lettera isolata, che sarebbe
 * il nome di una variabile ("n pari").
 *
 * È un giudizio sul testo, e può sbagliare: un inciso che restringe i casi
 * con parole che l'elenco non conosce verrebbe preso per un commento. Per
 * questo un verdetto FALSO dato ignorando un inciso lo dichiara (vedi
 * prove_item()). */
static int is_remark(const char *from, const char *to) {
    int words = 0;
    for (const char *p = from; p < to; ) {
        unsigned char ch = (unsigned char)*p;
        if (isalpha(ch) || ch >= 128) {
            const char *stop = p;
            while (stop < to && (isalpha((unsigned char)*stop) || (unsigned char)*stop >= 128)) stop++;
            if (stop - p == 1 || word_in(p, stop, restricting_words, sizeof restricting_words / sizeof *restricting_words)) return 0;
            words++;
            p = stop;
        } else if (strchr(" \t\r\n.,;:'-", ch)) {
            p++;
        } else {
            return 0;
        }
    }
    return words > 0;
}

/* Legge il testo [from, to) che segue la formula di un \item: prosa con
 * qualche formula tra dollari, come "per ogni intero $n\ge 2$, con $a>0$".
 *
 * La prima formula che è esattamente "lettera \ge numero" dice da dove parte
 * l'induzione: la lettera va in *variable, il numero (anche negativo) in
 * *base, e la funzione ritorna 1. Se non c'è ritorna 0.
 *
 * Un inciso tra parentesi che is_remark() giudica un commento viene saltato,
 * e *remark diventa 1.
 *
 * Tutto il resto, se non è fatto solo di parole di plain_words e di
 * punteggiatura, è qualcosa che il programma non sa leggere, e lo dice
 * *unread: 0 se non c'è nulla del genere; 1 se ci sono condizioni, in formula
 * come "a > 0" o a parole come "pari"; 2 se c'è una formula con un segno =,
 * cioè un'altra uguaglianza. Chi chiama deve tenerne conto (vedi read_item()
 * e prove_item()). */
static int read_conditions(const char *from, const char *to, int *variable, long long *base, int *unread, int *remark) {
    int found = 0;
    *unread = *remark = 0;
    for (const char *p = from; p < to; ) {
        unsigned char ch = (unsigned char)*p;

        /* Un inciso tra parentesi, se è un commento. Altrimenti la parentesi
         * è punteggiatura, e ciò che contiene si legge come il resto. */
        if (ch == '(') {
            const char *close = memchr(p, ')', (size_t)(to - p));
            if (close && is_remark(p + 1, close)) {
                *remark = 1;
                p = close + 1;
                continue;
            }
        }

        /* Una parola, o un comando LaTeX: barra seguita da una parola. I byte
         * oltre 127 sono le lettere accentate. Una barra seguita da altro,
         * come \, è uno spazio. L'unico comando ammesso è \small, che cambia
         * solo la grandezza dei caratteri. */
        if (isalpha(ch) || ch >= 128 || ch == '\\') {
            const char *word = ch == '\\' ? p + 1 : p, *stop = word;
            while (stop < to && (isalpha((unsigned char)*stop) || (unsigned char)*stop >= 128)) stop++;
            if (stop == word) { p = word < to ? word + 1 : word; continue; }
            int plain = ch == '\\' ? stop - word == 5 && strncmp(word, "small", 5) == 0
                                   : word_in(word, stop, plain_words, sizeof plain_words / sizeof *plain_words);
            if (!plain && !*unread) *unread = 1;
            p = stop;
            continue;
        }

        /* Spazi, punteggiatura e graffe non dicono nulla; cifre e altri
         * simboli fuori da una formula non sono stati capiti. */
        if (ch != '$') {
            if (!strchr(" \t\r\n.,;:(){}", ch) && !*unread) *unread = 1;
            p++;
            continue;
        }

        /* Una formula: da un dollaro al successivo. Un dollaro senza il suo
         * compagno è una formula non letta, ma senza dimenticare
         * un'uguaglianza già trovata prima. */
        const char *open = p, *close = memchr(open + 1, '$', (size_t)(to - open - 1));
        if (!close) {
            if (!*unread) *unread = 1;
            break;
        }
        p = close + 1;

        /* lettera \ge numero, con spazi e graffe facoltativi, e nient'altro. */
        const char *q = open + 1;
        char *after;
        while (isspace((unsigned char)*q)) q++;
        int letter = isalpha((unsigned char)*q) ? *q++ : 0;
        while (isspace((unsigned char)*q)) q++;
        int relation = letter && strncmp(q, "\\ge", 3) == 0;
        if (relation) q += q[3] == 'q' ? 4 : 3;
        while (isspace((unsigned char)*q) || *q == '{') q++;
        long long number = strtoll(q, &after, 10);
        int digits = after != q;
        q = after;
        while (isspace((unsigned char)*q) || *q == '}') q++;
        if (!relation || !digits || q != close || found) {
            if (memchr(open, '=', (size_t)(close - open))) *unread = 2;
            else if (!*unread) *unread = 1;
            continue;
        }
        found = 1;
        *variable = letter;
        *base = number;
    }
    return found;
}

/* Un enunciato letto da un \item: la formula e ciò che il testo dice su dove
 * parte l'induzione. */
typedef struct {
    char formula[MAX_FORMULA];  /* la formula in LaTeX, terminata da '\0' */
    int variable;               /* variabile di induzione */
    int has_base;               /* 1 se il testo indica il caso base */
    int conditional;            /* 1 se il testo pone una condizione non letta */
    int remark;                 /* 1 se un inciso tra parentesi è stato letto come commento */
    long long base;             /* il caso base, se has_base */
} Item;

/* Cerca il prossimo \item a partire da *position. Se lo trova mette in *item
 * e *end l'inizio e la fine del suo testo, porta *position alla fine e ritorna
 * 1; altrimenti ritorna 0. Il testo finisce al \item successivo o alla
 * chiusura dell'elenco. */
static int next_item(const char **position, const char **item, const char **end) {
    const char *p = *position;
    while ((p = strstr(p, "\\item")) != NULL) {
        p += 5;
        if (isalpha((unsigned char)*p)) continue;  /* \itemize, \itemsep, ... */
        const char *stop = strstr(p, "\\item");
        const char *list_end = strstr(p, "\\end{enumerate}");
        if (!stop || (list_end && list_end < stop)) stop = list_end;
        if (!stop) stop = p + strlen(p);
        *item = p;
        *end = *position = stop;
        return 1;
    }
    return 0;
}

/* Estrae in *out l'enunciato contenuto nel testo [item, end) di un \item.
 * Ritorna NULL se riesce; altrimenti ritorna il motivo, in italiano, per cui
 * il testo non è un enunciato utilizzabile. */
static const char *read_item(const char *item, const char *end, Item *out) {
    const char *prose = "enunciato in prosa: serve una formula sola in evidenza";
    while (item < end && isspace((unsigned char)*item)) item++;

    /* Individua la formula: \[ ... \] oppure $\displaystyle ... $. */
    const char *start, *stop;
    size_t closing = 1;  /* lunghezza del delimitatore che chiude la formula */
    if (end - item >= 2 && strncmp(item, "\\[", 2) == 0) {
        start = item + 2;
        stop = strstr(start, "\\]");
        closing = 2;
    } else if (item < end && *item == '$') {
        start = item + 1;
        while (start < end && isspace((unsigned char)*start)) start++;
        if (strncmp(start, "\\displaystyle", 13) != 0) return prose;
        stop = strchr(start, '$');
    } else {
        return prose;
    }
    if (!stop || stop > end) return prose;

    /* Copia la formula in una stringa a sé. */
    size_t length = (size_t)(stop - start);
    if (length >= sizeof out->formula) return "enunciato troppo lungo";
    memcpy(out->formula, start, length);
    out->formula[length] = 0;

    /* Variabile di induzione e caso base, se indicati dopo la formula, e ciò
     * che del testo non è stato letto. Una seconda uguaglianza può essere
     * un'altra affermazione, o un caso particolare in cui la prima non ha
     * senso: dimostrare solo la prima e rispondere DIMOSTRATO per tutto
     * l'\item sarebbe dire più di quanto si sa. */
    int unread;
    out->variable = 'n';
    out->base = 0;
    out->has_base = read_conditions(stop + closing, end, &out->variable, &out->base, &unread, &out->remark);
    out->conditional = unread == 1;
    if (unread == 2) return "dopo la formula ce n'è un'altra con un segno =: un \\item deve contenere un solo enunciato";
    return NULL;
}

/* Dimostra l'enunciato contenuto nel testo [item, end) di un \item. */
static Verdict prove_item(const char *item, const char *end) {
    Item statement;
    const char *problem = read_item(item, end, &statement);
    if (problem) return (Verdict){UNSUPPORTED, 'n', 0, 0, problem, NULL, NULL, NULL, NULL, NULL};
    Verdict verdict = prove(statement.formula, statement.variable, statement.has_base, statement.base);

    /* Una condizione scritta dopo la formula restringe i casi di cui parla
     * l'enunciato: "per ogni n pari", "con 0 < x < 1". Se il programma non
     * l'ha letta, una dimostrazione resta valida, perché copre anche quei
     * casi; un controesempio invece potrebbe essere proprio uno dei casi
     * esclusi, quindi non basta a dire FALSO. Senza questa cautela la somma
     * dei (-1)^k "per ogni n pari" risultava falsa per n = 1. */
    if (statement.conditional && verdict.outcome == REFUTED) {
        const char *reason = "in generale non vale, ma dopo la formula c'è una condizione che il programma non sa leggere";
        return (Verdict){NOT_PROVED, verdict.variable, 0, 0, reason, NULL, NULL, NULL, NULL, NULL};
    }

    /* Un inciso letto come commento non ferma un FALSO, ma chi legge il
     * verdetto deve sapere che c'era e che non è stato considerato. */
    if (statement.remark && verdict.outcome == REFUTED) verdict.note = "ignorato un commento tra parentesi dopo la formula";
    return verdict;
}

/* Scrive su out il testo, sostituendo i caratteri che in LaTeX hanno un
 * significato speciale: serve per riportare in un documento un nome di file o
 * un messaggio del programma, che possono contenere barre e graffe. */
static void put_escaped(FILE *out, const char *text) {
    for (; *text; text++) {
        if (*text == '\\') fputs("\\textbackslash{}", out);
        else if (*text == '^') fputs("\\^{}", out);
        else if (*text == '~') fputs("\\~{}", out);
        else if (strchr("{}_$%&#", *text)) fprintf(out, "\\%c", *text);
        else fputc(*text, out);
    }
}

/* Scrive su out, come testo LaTeX, la riga del verdetto. */
static void put_plain_text(FILE *out, const Verdict *verdict) {
    char line[512];
    FILE *memory = tmpfile();
    if (!memory) return;
    verdict_print(memory, verdict);
    rewind(memory);
    size_t length = fread(line, 1, sizeof line - 1, memory);
    line[length] = 0;
    fclose(memory);
    put_escaped(out, line);
}

/* Dimostra tutti gli enunciati del file path. Riempie *report con i conteggi
 * e, se out non è NULL, vi scrive una riga per enunciato. Se proofs non è
 * NULL vi scrive, come elenco LaTeX, ogni enunciato con la sua dimostrazione;
 * se certificates non è NULL vi scrive i certificati verificati, uno dopo
 * l'altro. Ritorna 0 se il file non si può leggere, 1 altrimenti. */
static int prove_file(const char *path, Report *report, FILE *out, FILE *proofs, FILE *certificates) {
    memset(report, 0, sizeof *report);
    char *text = read_file(path);
    if (!text) return 0;

    if (proofs) fprintf(proofs, "\\begin{enumerate}\n");
    const char *position = text, *item, *end;
    while (next_item(&position, &item, &end)) {
        Verdict verdict = prove_item(item, end);
        report->total++;
        switch (verdict.outcome) {
        case PROVED:      report->proved++; break;
        case REFUTED:     report->refuted++; break;
        case NOT_PROVED:  report->not_proved++; break;
        case UNSUPPORTED: report->unsupported++; break;
        }
        if (verdict.certificate) {
            report->certified++;
            if (certificates) fprintf(certificates, "%s\n", verdict.certificate);
        }
        if (out) {
            fprintf(out, "%4d. ", report->total);
            verdict_print(out, &verdict);
            fputc('\n', out);
        }

        /* Nel documento: l'enunciato com'è scritto nel file, poi la
         * dimostrazione, o in sua mancanza il verdetto. Si rilegge l'enunciato
         * dal testo perché prove_item() ne ritorna solo il verdetto. */
        if (proofs) {
            Item statement;
            fprintf(proofs, "\\item ");
            if (!read_item(item, end, &statement)) fprintf(proofs, "$\\displaystyle %s$\n\n", statement.formula);
            if (verdict.proof) {
                fprintf(proofs, "%s\n\n", verdict.proof);
            } else {
                fprintf(proofs, "\\textbf{No proof.} ");
                put_plain_text(proofs, &verdict);
                fprintf(proofs, "\n\n");
            }
        }
    }
    if (proofs) fprintf(proofs, "\\end{enumerate}\n");
    free(text);
    return 1;
}

/* =============================================================================
 * 22. MAIN
 * ============================================================================
 *
 * Modi d'uso:
 *
 *     mathresearcher FILE.tex ...                  dimostra ogni enunciato dei file
 *     mathresearcher --proofs OUT.pdf FILE.tex ... scrive anche le dimostrazioni, in LaTeX
 *                                                  (OUT.tex) e compilate in PDF
 *     mathresearcher --certificates OUT FILE.tex ...
 *                                                  scrive anche i certificati verificati,
 *                                                  da far ricontrollare a checker
 *     mathresearcher                               legge un enunciato da stdin
 *
 * Le due opzioni si possono dare insieme. Il codice di uscita è 0 se tutti
 * gli enunciati sono stati dimostrati, 1 altrimenti.
 */

/* Compila in PDF il documento LaTeX tex_path, mettendo il PDF nella stessa
 * cartella. Ritorna 1 se riesce.
 *
 * È l'unico punto in cui il programma si appoggia a qualcosa di esterno: un
 * compilatore LaTeX già installato, chiamato con system(). Scrivere un PDF con
 * formule senza un compilatore LaTeX vorrebbe dire riscriverne uno. Si provano
 * in ordine i comandi di compilers[]; il documento .tex resta comunque, e chi
 * non ha un compilatore lo può usare altrove. */
static int compile_pdf(const char *tex_path) {
    static const char *const compilers[] = {
        "tectonic --outdir '%s' '%s' > /dev/null 2>&1",
        "pdflatex -interaction=batchmode -halt-on-error -output-directory '%s' '%s' > /dev/null 2>&1",
    };
    char directory[1024], command[4096];

    /* I percorsi vanno tra apici nel comando: uno che ne contiene non si può
     * passare in sicurezza. */
    if (strchr(tex_path, '\'') || strlen(tex_path) >= sizeof directory) return 0;
    strcpy(directory, tex_path);
    char *slash = strrchr(directory, '/');
    if (slash) *slash = 0; else strcpy(directory, ".");

    for (size_t i = 0; i < sizeof compilers / sizeof *compilers; i++) {
        snprintf(command, sizeof command, compilers[i], directory, tex_path);
        if (system(command) == 0) return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
    engine_init();

    /* Senza argomenti: un solo enunciato da stdin, con la sua dimostrazione. */
    if (argc < 2) {
        char line[MAX_FORMULA];
        printf("Enunciato in LaTeX: ");
        if (!fgets(line, sizeof line, stdin)) return 1;
        Verdict verdict = prove(line, 'n', 0, 0);
        verdict_print(stdout, &verdict);
        putchar('\n');
        if (verdict.proof) printf("\n%s\n", verdict.proof);
        return verdict.outcome == PROVED ? 0 : 1;
    }

    /* Le opzioni, prima dei file.
     *
     * --proofs: il documento in cui scrivere le dimostrazioni. Con un nome
     * che finisce in .pdf si scrive il documento LaTeX con lo stesso nome e
     * l'estensione .tex, e poi lo si compila. Il preambolo è quello dei file
     * in tests/.
     *
     * --certificates: il file in cui scrivere i certificati. */
    FILE *proofs = NULL, *certificates = NULL;
    char proofs_path[1024] = "";
    int first = 1, want_pdf = 0;
    for (; first + 1 < argc && strncmp(argv[first], "--", 2) == 0; first += 2) {
        if (strcmp(argv[first], "--certificates") == 0 && !certificates) {
            certificates = fopen(argv[first + 1], "w");
            if (!certificates) { printf("ERRORE: impossibile scrivere %s\n", argv[first + 1]); return 1; }
            continue;
        }
        if (strcmp(argv[first], "--proofs") != 0 || proofs) { printf("ERRORE: opzione sconosciuta o ripetuta: %s\n", argv[first]); return 1; }
        size_t length = strlen(argv[first + 1]);
        if (length < 5 || length >= sizeof proofs_path) { printf("ERRORE: nome del documento non valido\n"); return 1; }
        strcpy(proofs_path, argv[first + 1]);
        want_pdf = strcmp(proofs_path + length - 4, ".pdf") == 0;
        if (want_pdf) strcpy(proofs_path + length - 4, ".tex");
        proofs = fopen(proofs_path, "w");
        if (!proofs) { printf("ERRORE: impossibile scrivere %s\n", proofs_path); return 1; }
        fprintf(proofs, "\\documentclass[11pt,a4paper]{article}\n\\usepackage[T1]{fontenc}\n"
                        "\\usepackage[utf8]{inputenc}\n\\usepackage[english]{babel}\n"
                        "\\usepackage{amsmath,amssymb}\n\\usepackage[margin=2cm]{geometry}\n"
                        "\\setlength{\\parindent}{0pt}\n\\setlength{\\emergencystretch}{3em}\n"
                        "\\allowdisplaybreaks\n\n\\title{Proofs}\n\\author{}\n\\date{}\n"
                        "\\begin{document}\n\\maketitle\n");
    }

    /* Un file per argomento. */
    int all_proved = 1;
    for (int i = first; i < argc; i++) {
        Report report;
        printf("%s\n", argv[i]);
        if (proofs) {
            fprintf(proofs, "\n\\section*{");
            put_escaped(proofs, argv[i]);
            fprintf(proofs, "}\n");
        }
        if (!prove_file(argv[i], &report, stdout, proofs, certificates)) {
            printf("ERRORE: impossibile leggere il file\n");
            all_proved = 0;
            continue;
        }
        printf("Riepilogo: %d enunciati, %d dimostrati (%d con certificato verificato), %d falsi, %d non dimostrati, %d non supportati.\n",
               report.total, report.proved, report.certified, report.refuted, report.not_proved, report.unsupported);
        if (report.proved != report.total) all_proved = 0;
    }
    if (unjustified_steps)
        printf("ATTENZIONE: %d passi di dimostrazione (formule rilette, o certificati respinti dal verificatore) "
               "non hanno superato il controllo indipendente e sono stati scartati\n", unjustified_steps);
    if (certificates) fclose(certificates);
    if (proofs) {
        fprintf(proofs, "\\end{document}\n");
        fclose(proofs);
        if (want_pdf && !compile_pdf(proofs_path))
            printf("ERRORE: nessun compilatore LaTeX ha prodotto il PDF; resta il documento %s\n", proofs_path);
    }
    return all_proved ? 0 : 1;
}
