/* =============================================================================
 * parser.c: la lettura del LaTeX
 * =============================================================================
 *
 * COSA FA
 *
 * Trasforma il testo LaTeX in alberi: è la parte di mathresearcher che
 * legge. Da un file .tex estrae gli enunciati, uno per \item; di ogni
 * enunciato costruisce l'albero che il motore, in main.c, dimostra o
 * smentisce. Qui non c'è nessuna regola di dimostrazione: un errore in questo
 * file fa leggere un enunciato diverso da quello scritto, non dimostrare un
 * enunciato falso con passi sbagliati. Per questo le convenzioni di lettura
 * (che cosa vuol dire `2k`, dove finisce il termine di una somma, che cosa è
 * un insieme e che cosa un numero) sono scritte per esteso in testa ai
 * capitoli: sono l'unico posto in cui il lettore decide qualcosa.
 *
 * COME È FATTO
 *
 * Quattro notazioni, quattro alberi, una sola tecnica (discesa ricorsiva:
 * una funzione per regola della grammatica):
 *
 *   - le espressioni, con somme, prodotti, potenze e funzioni: Node
 *     (capitoli 8 e 9), su cui poggiano anche gli enunciati sulle derivate
 *     e sugli integrali (9j);
 *   - gli enunciati sugli insiemi: SetNode (capitolo 14);
 *   - gli enunciati con quantificatori, gli insiemi numerici e gli insiemi
 *     di soluzioni: Formula (capitolo 17), le cui foglie sono espressioni;
 *   - gli enunciati su insiemi qualsiasi, con elementi, operazioni,
 *     applicazioni e relazioni senza una regola: Sentence (paragrafo 17o,
 *     in fondo al capitolo 17), le cui foglie sono Elem. Vi si scrive anche
 *     come nei libri, con immagini, controimmagini e insiemi di coppie dati
 *     da una regola (SetTerm, 17p); il lettore svolge quella notazione nelle
 *     condizioni sugli elementi (17q), che sono le sole che il motore vede.
 *     Le relazioni e le applicazioni tra le parti di un insieme si svolgono
 *     invece in un enunciato del capitolo 14 (17r, in fondo a quel
 *     capitolo).
 *
 * Svolgere una definizione è l'unico punto, oltre alle convenzioni di
 * lettura, in cui questo file può cambiare il senso di un enunciato. Lo
 * svolgimento delle relazioni tra numeri (17j) riscrive il testo, e si
 * ricontrolla rifacendolo sull'albero; quelli di 17q e 17r si fanno
 * sull'albero, dove una variabile è un posto e non una lettera, e il motore
 * li ricontrolla calcolando su modelli finiti l'enunciato com'è scritto e
 * quello svolto (28h e parts_check() in main.c).
 *
 * Il capitolo 21 legge il documento: trova gli \item, la formula di
 * ognuno e le poche parole che possono seguirla ("per ogni intero n \ge 2").
 *
 * COME STA NEL PROGRAMMA
 *
 * Non è un programma a sé: è un volume dello stesso libro di main.c, che lo
 * include dopo il suo capitolo 7. Usa ciò che main.c ha già presentato: i
 * limiti (capitolo 1), gli errori e la memoria di lavoro (capitolo 2), e i
 * tipi dei numeri esatti e delle frazioni di polinomi (capitoli 3-7), che
 * compaiono nei nodi degli alberi come annotazioni lasciate dal motore. Del
 * motore, che viene dopo, usa solo eval(), dichiarata nel capitolo 17. I
 * capitoli conservano il numero che avevano quando stavano in main.c, perché
 * i commenti, i test e il README citano le regole per numero (17d, 17h): la
 * numerazione è una sola per main.c, parser.c e output.c.
 *
 * INDICE DEI CAPITOLI
 *
 *    8. Albero sintattico
 *    9. Parsing: dal LaTeX all'albero
 *   14. Insiemi: notazione e albero
 *   17. Quantificatori: notazione e albero
 *   21. Lettura degli enunciati da un file .tex
 */

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
    N_APPLY,  /* applicazione dichiarata dall'enunciato, come f(x): name = la
                 lettera f, a = argomento */
    N_DERIV   /* segno di derivata di Leibniz dentro un'espressione (9d):
                 name = la variabile x, value = l'ordine (un numero da 1 in
                 su), a = la funzione da derivare. Lo crea solo il lettore, e
                 il motore lo sostituisce con la derivata prima di ogni conto
                 (27j): eval() e le altre letture dell'albero non lo trattano */
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
    FN_ABS,     /* valore assoluto */
    FN_ARCSIN, FN_ARCCOS,  /* le inverse di seno e coseno, a valori in
                              [-pi/2, pi/2] e in [0, pi] */
    FN_ARCCOT   /* l'inversa della cotangente, a valori in (0, pi) */
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
    int reals;                /* 1: tutti i numeri reali, non solo i razionali */
    int bounded;              /* 1: c'è un estremo inferiore, `low` */
    long long low;            /* l'estremo inferiore, se bounded */
    int strict;               /* 1: low è escluso (x > low); 0: è compreso */
    int holes;                /* punti tolti, validi in `hole` */
    Frac hole[MAX_HOLES];
    const char *start, *end;  /* com'è scritto nell'enunciato: [start, end) */
} NumberSet;

/* Un'applicazione dichiarata dall'enunciato con "f \colon A \to B". La regola
 * che la definisce, "f(x) = espressione", arriva dopo, da un'altra ipotesi:
 * finché manca, body è NULL e parameter non ha significato.
 *
 * Un'operazione binaria, dichiarata con "\ast \colon A \times B \to C", è
 * un'applicazione con due argomenti: la sua regola è "x \ast y = espressione"
 * (vedi 9g). Gli ultimi tre campi servono solo a lei. */
typedef struct {
    int name;                    /* la lettera f, oppure il nome interno di un
                                    simbolo di operazione (operation_name()) */
    NumberSet domain, codomain;  /* A e B; per un'operazione A e C */
    int parameter;               /* la variabile x della regola */
    Node *body;                  /* il valore di f(x), nell'arena; NULL se manca */
    int binary;                  /* 1 se è un'operazione binaria */
    NumberSet second;            /* operazione: B, l'insieme del secondo argomento */
    int parameter2;              /* operazione: la variabile y della regola */
} Map;

/* I simboli con cui un enunciato può indicare un'operazione binaria che
 * dichiara. Sono comandi LaTeX che per il resto della grammatica non vogliono
 * dire nulla: \cdot e \times, che sono il prodotto, non ci sono. Il nome
 * interno del simbolo di posto i è OPERATION_NAMES + i. */
static const char *const operation_symbols[] = {"ast", "star", "circ", "bullet", "diamond", "perp", "oplus", "otimes", "odot"};

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

/* Un enunciato su una successione che l'enunciato stesso definisce per
 * ricorrenza: "se a_0 = 0 e a_{n+1} = \sqrt{6 + a_n}, allora a_n < 3". Le
 * ipotesi sono uguaglianze, i valori iniziali e la ricorrenza, più
 * eventualmente "n \ge q", che dice da quale n la ricorrenza vale; le tesi
 * sono confronti. Che le ipotesi definiscano davvero una successione lo
 * controlla il motore (capitolo 26): qui c'è solo com'è scritto. */
typedef struct {
    int givens;                    /* ipotesi valide in given[] */
    Statement given[MAX_GIVEN];    /* le uguaglianze, nell'ordine in cui sono scritte */
    int has_since;                 /* 1 se tra le ipotesi c'è "n \ge q" */
    int since_name;                /* la lettera n di quell'ipotesi */
    long long since;               /* il numero q */
    int claims;                    /* tesi valide in claim[] */
    Statement claim[MAX_CLAIMS];
} SequenceStatement;

/* Un enunciato sulla derivata di una funzione: "la derivata di ordine n di
 * function, rispetto a variable, è formula". L'ordine è una lettera, quella
 * su cui si fa induzione, e allora function non la contiene e formula di
 * solito sì; oppure è un numero. Senza formula è una richiesta: "calcola la
 * derivata di function" (27k). Gli alberi stanno nell'arena. */
typedef struct {
    int variable;     /* la lettera x rispetto a cui si deriva */
    int order;        /* la lettera n dell'ordine di derivazione, o 0 se è un numero */
    long long times;  /* l'ordine, quando è un numero: almeno 1 */
    Node *function;   /* la funzione da derivare */
    Node *formula;    /* ciò che l'enunciato dice della derivata; NULL in una richiesta */
} DerivativeStatement;

/* Che cosa un enunciato su un limite dice del limite (vedi LimitStatement):
 *   LIMIT_ASKED     niente, è una richiesta: "\lim_{x \to a} f";
 *   LIMIT_VALUE     che vale il numero `value`;
 *   LIMIT_PLUS      che vale +\infty;   LIMIT_MINUS  che vale -\infty;
 *   LIMIT_UNSIGNED  che vale \infty senza segno: |f| tende a +\infty;
 *   LIMIT_NONE      che non esiste: "\nexists \lim_{x \to a} f". */
typedef enum { LIMIT_ASKED, LIMIT_VALUE, LIMIT_PLUS, LIMIT_MINUS, LIMIT_UNSIGNED, LIMIT_NONE } LimitClaim;

/* Un enunciato su un limite: "il limite di function per variable che tende
 * al punto è ...". Il punto è un'espressione senza la variabile, oppure
 * +\infty o -\infty; il limite può essere da un lato solo. Gli alberi stanno
 * nell'arena; che il punto non contenga la variabile lo controlla il motore
 * (capitolo 29). */
typedef struct {
    int variable;         /* la lettera x che tende al punto */
    int sequence;         /* 1 se la lettera è n: un indice naturale, e il punto è +\infty */
    int infinity;         /* 0 per un punto finito, +1 per +\infty, -1 per -\infty */
    Node *point;          /* il punto finito, o NULL */
    int side;             /* +1 da destra (a^{+}), -1 da sinistra (a^{-}), 0 da tutte e due */
    Node *function;       /* la funzione di cui si fa il limite */
    LimitClaim claim;     /* che cosa l'enunciato ne dice */
    Node *value;          /* il valore, per LIMIT_VALUE; altrimenti NULL */
    const char *from, *to;  /* il testo "\lim_{...} f" nell'enunciato: [from, to) */
} LimitStatement;

/* Un enunciato su un integrale (9j): "l'integrale di integrand in
 * d variable è answer". Senza estremi è un integrale indefinito, e answer è
 * una primitiva F, scritta con la sua costante (F + c) che il lettore toglie;
 * con gli estremi è un integrale definito, e answer è un numero. Senza il
 * segno di uguale è una richiesta: answer è NULL, e la risposta la cerca il
 * motore (capitolo 30). Gli alberi stanno nell'arena. */
typedef struct {
    int variable;         /* la lettera x del differenziale dx */
    Node *integrand;      /* la funzione integranda, senza il dx */
    Node *lower, *upper;  /* gli estremi; NULL tutti e due se l'integrale è indefinito */
    Node *answer;         /* F senza la costante, oppure il valore; NULL per una richiesta */
    int constant;         /* la lettera della costante di F, se c'è; altrimenti 0 */
} IntegralStatement;

/* =============================================================================
 * 9. PARSING: DAL LATEX ALL'ALBERO
 * ============================================================================
 *
 * Il sottoinsieme di LaTeX accettato, dalla precedenza più bassa alla più alta:
 *
 *     statement := expr ('=' | \ge | \le | '<' | '>') expr
 *     expr      := sum [ operazione sum ]
 *     sum       := ['-' | '+'] term { ('+' | '-') term }
 *     term      := factor { [\cdot | '/'] factor }
 *     factor    := primary { '!' | '^' group }
 *     primary   := numero | lettera ['_' group] | \pi
 *                | applicazione '(' expr ')'
 *                | '(' expr ')' | \left| expr \right| | '|' expr '|'
 *                | leibniz term
 *                | \frac group group | \binom group group
 *                | \sqrt ['[' expr ']'] group
 *                | (\sum | \prod) '_' '{' lettera '=' expr '}' '^' group term
 *                | funzione ['^' group] argomento
 *     leibniz   := \frac '{' 'd' ['^' group] '}' '{' 'd' lettera ['^' group] '}'
 *     group     := '{' expr '}' | un solo token
 *     argomento := '(' expr ')' | factor
 *
 * Una seconda forma di enunciato, per le successioni definite per
 * ricorrenza, mette delle ipotesi davanti alla tesi (vedi
 * parse_sequence_statement()):
 *
 *     sequence  := given { ',' given } \implies statement { \land statement }
 *     given     := expr '=' expr | lettera \ge numero
 *
 * Una terza, per le derivate (vedi parse_derivative_statement()):
 *
 *     derivative := leibniz term [ '=' expr ]
 *
 * Senza '=' è una richiesta: calcolare la derivata.
 *
 * Una quarta, per i limiti (vedi parse_limit_statement(), 9i):
 *
 *     limit  := [\nexists] \lim '_' '{' lettera (\to | \rightarrow) punto '}' expr ['=' valore]
 *     punto  := ['+' | '-'] \infty | expr [ '^' ('+' | '-' | '{+}' | '{-}') ]
 *     valore := ['+' | '-'] \infty | expr
 *
 * Una quinta, per gli integrali (vedi parse_integral_statement(), 9j):
 *
 *     integral  := \int [ '_' group '^' group ] expr [ '=' expr ]
 *
 * dove nella prima expr compare una volta sola il differenziale dx.
 *
 * Le convenzioni che la grammatica fissa, e che in LaTeX sono implicite:
 *
 *   - Due fattori accostati si moltiplicano: `2k`, `n(n+1)`, `k k!`.
 *   - Una somma si estende a tutto il term che la segue: in `\sum f g + h`
 *     il sommando è f g, e h resta fuori.
 *   - L'argomento di una funzione senza parentesi è un solo fattore:
 *     `\sin x y` vale (sin x) y. Fa eccezione un numero seguito da
 *     lettere, che i libri scrivono sempre per intendere il prodotto:
 *     `\sin 2x` vale sin(2x), `\cos^{2} 3x` vale cos(3x)^2.
 *   - Il segno di derivata `\frac{d}{dx}` non è mai una frazione di lettere:
 *     è la derivata del term che lo segue, come il corpo di una somma, in
 *     testa a un enunciato su una derivata o dentro un'espressione (9d).
 *     Dentro un'espressione l'ordine è un numero: `\frac{d^{2}}{dx^{2}}`.
 *   - Un valore assoluto si scrive anche con le sole sbarre, `|x|`. Dentro
 *     una sbarra aperta, la sbarra successiva la chiude: non può aprire un
 *     fattore accostato. Così `x|x|` è x per |x|, e `||x| - 1|` si legge
 *     come si deve; `|a|b|c|`, che si potrebbe leggere in due modi, è un
 *     errore di sintassi.
 *   - `\sin^{2}(x)` è il quadrato di sin(x).
 *   - Senza graffe un esponente o un pedice è un solo token: `x^12` vale
 *     x^1 seguito da 2, come in TeX.
 *   - Un'operazione dichiarata dall'enunciato, come `a \ast b`, non ha una
 *     precedenza: `a + b \ast c` si può leggere in due modi, e i libri non
 *     sono d'accordo su quale. Qui non si sceglie: i due operandi devono
 *     essere fattori soli (una lettera, un numero, una parentesi, f(x)), e
 *     il risultato si combina con altro solo tra parentesi, anche con la
 *     stessa operazione: `(a \ast b) \ast c`. Ogni altra scrittura è un errore
 *     di sintassi (9g).
 *   - `\exp`, `\sinh`, `\cosh`, `\arcsin` e `\arccos` si leggono come le
 *     funzioni che già ci sono, con la loro definizione, e la lettera e è
 *     il numero di Nepero (9h).
 *   - In un limite la funzione è tutta l'espressione che segue \lim, fino
 *     al segno = o alla fine: `\lim_{x \to 0} f + g` è il limite di f + g.
 *     \infty senza segno si accetta come punto solo per la lettera n, che è
 *     l'indice di una successione e tende a +\infty (9i).
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
static const char *syntax_where;  /* dove cominciava il token del primo errore: tra
                                     due tentativi di lettura falliti, quello andato
                                     più avanti ha l'errore più preciso (17p) */

/* Annota message come errore di sintassi (se è il primo) e ferma il parsing. */
static void syntax(const char *message) {
    if (!syntax_error) { syntax_error = message; syntax_where = token.start; }
    cursor = "";
    token.kind = T_END;
}

/* 9j, il lettore di token. Mentre si legge un integrale (vedi
 * parse_integral_statement()) vale una convenzione che altrove non vale,
 * e questo interruttore la accende: dentro la funzione integranda la
 * lettera d seguita da un'altra lettera è il differenziale, dx, un token
 * solo. Fuori da un integrale resta spento, e il lettore è quello di
 * sempre. */
static int differential_open;

/* Valore di Token.ch per il differenziale dx; token.value è la lettera x.
 * "\d" non è un comando che il lettore accetti, quindi ESCAPED + 'd' non si
 * confonde con le graffe e l'a capo. */
#define DIFFERENTIAL (ESCAPED + 'd')

/* Se differential_open è acceso e il cursore è sulla lettera d seguita, dopo
 * eventuali spazi, da un'altra lettera, consuma le due e fa del token
 * corrente il differenziale; ritorna 1. Altrimenti non tocca nulla e
 * ritorna 0. */
static int lex_differential(void) {
    const char *after = cursor + 1;
    while (isspace((unsigned char)*after)) after++;
    if (!differential_open || *cursor != 'd' || !isalpha((unsigned char)*after)) return 0;
    token.kind = T_CHAR;
    token.ch = DIFFERENTIAL;
    token.value = *after;
    cursor = after + 1;
    return 1;
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

        /* Lettera: ogni lettera è un simbolo a sé, tranne il differenziale
         * di un integrale (9j). */
        if (isalpha(c) && lex_differential()) return;
        if (isalpha(c)) { token.kind = T_LETTER; token.ch = *cursor++; return; }

        /* Comando: barra seguita da lettere, oppure spazio come \, */
        if (c == '\\') {
            cursor++;
            if (!isalpha((unsigned char)*cursor)) {
                if (*cursor && strchr(",;!: ", *cursor)) { cursor++; continue; }
                if (*cursor == '{' || *cursor == '}' || *cursor == '\\') {
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

/* Lo stato del lettore di token. Rimetterlo com'era fa riprendere la lettura
 * da un punto precedente: serve dove un solo token non basta a decidere quale
 * regola della grammatica applicare (17c e 17d, e in 17r). I nodi creati nel
 * tentativo abbandonato restano nell'arena, inutilizzati. */
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

/* Se il token corrente è uno dei simboli di operation_symbols[] ritorna il
 * suo nome interno, quello che l'operazione ha nella tabella `maps`;
 * altrimenti ritorna 0. Che l'operazione sia stata dichiarata lo dice poi
 * map_find(). */
static int operation_name(void) {
    for (size_t i = 0; i < sizeof operation_symbols / sizeof *operation_symbols; i++)
        if (is_command(operation_symbols[i])) return OPERATION_NAMES + (int)i;
    return 0;
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
    {"arcsin", FN_ARCSIN}, {"arccos", FN_ARCCOS}, {"arccot", FN_ARCCOT},
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

/* funzione ['^' group] argomento, con \log che ammette anche '_' group:
 * la base, che se manca è 10.
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

    /* Argomento: tra parentesi, oppure un solo fattore. Se quel fattore è
     * un numero seguito da lettere, l'argomento è tutto il monomio: \sin 2x
     * è sin(2x), come in ogni libro, e non (sin 2) x. Le lettere si
     * prendono finché seguono accostate: \sin 2x \cos x è sin(2x) cos(x),
     * perché \cos è un comando e non una lettera. */
    Node *argument;
    if (token.kind == T_OPEN) {
        next_token();
        argument = parse_expr();
        if (token.kind == T_CLOSE) next_token(); else syntax("manca la parentesi chiusa");
    } else {
        int number = token.kind == T_NUM;
        argument = parse_factor();
        while (number && (token.kind == T_LETTER || is_command("pi")) && !syntax_error)
            argument = node_new(N_MUL, argument, parse_factor());
    }

    /* \log senza base è il logaritmo in base 10, come nei libri di scuola;
     * il logaritmo naturale si scrive \ln. */
    if (fn == FN_LOG && !base) base = node_num(10);
    Node *call = node_call(fn, argument, base);
    return exponent ? node_new(N_POW, call, exponent) : call;
}

/* 9h. FUNZIONI DEFINITE CON LE ALTRE
 *
 * Esponenziale, seno e coseno iperbolici non sono nodi propri dell'albero:
 * si leggono con la loro definizione, scritta con le funzioni che il motore
 * già tratta. Così ogni metodo del motore li conosce senza una regola in
 * più. Le definizioni valgono su tutto R:
 *
 *     \exp u = e^{u},     \sinh u = (e^{u} - e^{-u}) / 2,     \cosh u = (e^{u} + e^{-u}) / 2.
 *
 * La lettera e è il numero di Nepero. Arcoseno e arcocoseno invece sono
 * funzioni del lettore (FN_ARCSIN, FN_ARCCOS), perché la loro derivata
 * (capitolo 27) si scrive meglio così; i limiti (capitolo 29) li sviluppano
 * con la loro scrittura per mezzo dell'arcotangente.
 *
 * Warning: se aggiungi un nome qui, aggiungi la sua definizione in
 * parse_defined_function(). */
static const char *const defined_functions[] = {"exp", "sinh", "cosh"};

/* Ritorna la posizione in defined_functions[] del comando corrente, o -1. */
static int defined_function(void) {
    for (size_t i = 0; i < sizeof defined_functions / sizeof *defined_functions; i++)
        if (is_command(defined_functions[i])) return (int)i;
    return -1;
}

/* Legge una funzione di defined_functions[] (il token corrente) e ritorna
 * l'albero della sua definizione. L'esponente e l'argomento si leggono come
 * per ogni altra funzione: per questo la si legge prima come un seno, con
 * parse_function(), e poi se ne sostituisce la chiamata. */
static Node *parse_defined_function(void) {
    int which = defined_function();
    Node *read = parse_function(FN_SIN), *call = read->kind == N_POW ? read->a : read, *u = call->a;
    Node *e = node_new(N_VAR, NULL, NULL), *value;
    e->name = 'e';
    Node *up = node_new(N_POW, e, u), *down = node_new(N_POW, e, node_new(N_NEG, u, NULL));

    /* Le posizioni in defined_functions[]: 0 \exp, 1 \sinh, 2 \cosh. */
    if (which == 0) value = up;
    else value = node_new(N_DIV, node_new(which == 1 ? N_SUB : N_ADD, up, down), node_num(2));
    return read->kind == N_POW ? node_new(N_POW, value, read->b) : value;
}

/* Ritorna 1 se text comincia con il segno di derivata di Leibniz: \frac,
 * \dfrac o \tfrac, sopra {d} oppure {d^...}, sotto {d seguito da una
 * lettera. Prima del segno si saltano gli spazi e i comandi di sola
 * presentazione \displaystyle e \textstyle, come fa next_token().
 *
 * Serve in due posti. In testa a un enunciato dice che vale la pena di
 * leggerlo come enunciato su una derivata (capitolo 27). Dentro
 * un'espressione dice che lì c'è un segno di derivata, e il lettore ne fa
 * un nodo N_DERIV (paragrafo 9d), mai una frazione: letto come frazione
 * delle lettere d e x, \frac{d}{dx} x^{2} = x sarebbe un'uguaglianza vera,
 * (1/x) x^2 = x, e il programma "dimostrerebbe" che la derivata di x^2 è x.
 * È successo, con un \displaystyle davanti che questa funzione non saltava:
 * vedi tests/derivatives/false_nth_derivatives.tex. */
static int starts_with_derivative(const char *text) {
    static const char *const fraction[] = {"\\frac", "\\dfrac", "\\tfrac"};
    static const char *const style[] = {"\\displaystyle", "\\textstyle"};
    for (int skipped = 1; skipped;) {
        skipped = 0;
        while (isspace((unsigned char)*text)) text++;
        for (size_t i = 0; i < sizeof style / sizeof *style; i++) {
            size_t length = strlen(style[i]);
            if (strncmp(text, style[i], length) == 0 && !isalpha((unsigned char)text[length])) { text += length; skipped = 1; }
        }
    }
    for (size_t i = 0; i < sizeof fraction / sizeof *fraction; i++) {
        size_t length = strlen(fraction[i]);
        if (strncmp(text, fraction[i], length) != 0 || text[length] != '{' || text[length + 1] != 'd') continue;

        /* Sopra: {d}, {d^n} con l'esponente di un carattere, o {d^{n}}. */
        const char *below = text + length + 2;
        if (below[0] == '}') {
            below += 1;
        } else if (below[0] == '^' && below[1] == '{') {
            const char *close = strchr(below + 2, '}');
            if (!close || close[1] != '}') return 0;
            below = close + 2;
        } else if (below[0] == '^' && below[1] && below[2] == '}') {
            below += 3;
        } else {
            return 0;
        }

        /* Sotto: {d seguito, dopo eventuali spazi, da una lettera. */
        if (below[0] != '{' || below[1] != 'd') return 0;
        below += 2;
        while (isspace((unsigned char)*below)) below++;
        return isalpha((unsigned char)*below);
    }
    return 0;
}

/* leibniz := \frac '{' 'd' ['^' group] '}' '{' 'd' lettera ['^' group] '}'
 *
 * Il token corrente è \frac, \dfrac o \tfrac, e starts_with_derivative() ha
 * detto che lì comincia un segno di derivata. Mette in *variable la lettera
 * x. L'ordine è l'esponente, lo stesso sopra e sotto: una lettera, che va in
 * *order; un numero da 1 in su, che va in *times con *order a 0; oppure
 * niente, che vuol dire 1. Un segno scritto in un altro modo è un errore di
 * sintassi. Lo usano tutti e due i posti in cui il segno si legge: la testa
 * di un enunciato (parse_derivative_statement()) e l'interno di
 * un'espressione (9d). */
static void parse_leibniz(int *variable, int *order, long long *times) {
    const char *shape = "il segno di derivata si scrive \\frac{d^{n}}{dx^{n}}, con lo stesso ordine sopra e sotto";
    Node *above = NULL, *below = NULL;
    *variable = *order = 0;
    *times = 1;
    next_token();

    /* Sopra: d, o d^n. */
    expect_char('{', shape);
    if (token.kind != T_LETTER || token.ch != 'd') syntax(shape);
    next_token();
    if (is_char('^')) { next_token(); above = parse_group(); }
    expect_char('}', shape);

    /* Sotto: dx, o dx^n con lo stesso n. */
    expect_char('{', shape);
    if (token.kind != T_LETTER || token.ch != 'd') syntax(shape);
    next_token();
    if (token.kind != T_LETTER) syntax(shape);
    *variable = token.ch;
    next_token();
    if (is_char('^')) { next_token(); below = parse_group(); }
    expect_char('}', shape);

    /* L'ordine: niente, una lettera o un numero, uguale nei due posti. */
    if (!above != !below) syntax(shape);
    else if (above && above->kind == N_VAR && below->kind == N_VAR && above->name == below->name) *order = above->name;
    else if (above && above->kind == N_NUM && below->kind == N_NUM && above->value == below->value && above->value >= 1) *times = above->value;
    else if (above) syntax(shape);
}

/* 9d. leibniz term dentro un'espressione: il nodo N_DERIV, con la funzione
 * da derivare che è il term che segue il segno, come il corpo di una somma.
 * Il token corrente è il \frac del segno. Dentro un'espressione l'ordine
 * deve essere un numero: una derivata n-esima chiede l'induzione del
 * capitolo 27, che parte solo da un enunciato che comincia con il segno.
 * Senza una funzione dopo il segno, \frac{d}{dx} = \frac{1}{x}, parse_term()
 * trova il simbolo inatteso e l'errore di sintassi resta. */
static Node *parse_derivative_sign(void) {
    int variable, order;
    long long times;
    parse_leibniz(&variable, &order, &times);
    if (order) { syntax("dentro un'espressione l'ordine di una derivata deve essere un numero, come in \\frac{d^{2}}{dx^{2}}"); return node_num(0); }
    Node *node = node_new(N_DERIV, parse_term(), NULL);
    node->name = variable;
    node->value = times;
    return node;
}

/* Quante sbarre di valore assoluto scritte senza \left e \right sono aperte
 * nel punto in cui si trova il lettore. Dentro una di esse una sbarra la
 * chiude, e non comincia un fattore accostato (vedi le convenzioni in testa
 * al capitolo e starts_factor()). Ogni chiamata che la aumenta la rimette
 * com'era prima di ritornare, anche dopo un errore di sintassi. */
static int open_bars;

/* 9j. Che cosa ha incontrato il lettore dentro la funzione integranda: il
 * nodo che fa le veci del differenziale (vale 1, e conta solo dove sta), la
 * sua lettera, e quanti differenziali ha letto in tutto. Li azzera
 * parse_integral_statement(). */
static Node *differential_node;
static int differential_letter, differentials;

/* 9j. Dentro un integrale, il fattore che altrove non esiste: il
 * differenziale, che diventa il numero 1 (il suo posto lo controlla poi
 * parse_integral_statement()). Ritorna NULL se il token corrente non è il
 * differenziale. Il valore assoluto scritto con le sole sbarre, \ln|x - 2|,
 * lo legge 9c, dentro e fuori da un integrale. */
static Node *parse_integral_piece(void) {
    if (is_char(DIFFERENTIAL)) {
        differentials++;
        differential_letter = (int)token.value;
        next_token();
        differential_node = node_num(1);
        return differential_node;
    }
    return NULL;
}

/* primary: vedi la grammatica in testa al capitolo. */
static Node *parse_primary(void) {
    Function fn;

    /* 9j. Il differenziale e la sbarra di un integrale. */
    Node *piece = parse_integral_piece();
    if (piece) return piece;

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
    if (is_char('|')) {
        next_token();
        open_bars++;
        Node *inner = parse_expr();
        open_bars--;
        if (is_char('|')) next_token(); else syntax("manca la sbarra che chiude il valore assoluto");
        return node_call(FN_ABS, inner, NULL);
    }

    /* 9d. Comandi con argomenti tra graffe. Il segno di derivata non è una
     * frazione: in testa all'enunciato lo legge parse_derivative_statement(),
     * dentro un'espressione parse_derivative_sign() (vedi
     * starts_with_derivative()). */
    if (is_command("frac") || is_command("dfrac") || is_command("tfrac")) {
        if (starts_with_derivative(token.start)) return parse_derivative_sign();
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
    if (defined_function() >= 0) return parse_defined_function();
    if (is_command("pi")) {
        next_token();
        Node *node = node_new(N_VAR, NULL, NULL);
        node->name = VAR_PI;
        return node;
    }

    /* 9f. Nient'altro può iniziare un'espressione. Il massimo comun divisore
     * ha un motivo a sé: è il comando che gli esercizi sui numeri usano di
     * più tra quelli che il programma non conosce, e chi legge il verdetto
     * deve sapere che manca la sua teoria, non un simbolo. */
    if (is_command("gcd") || is_command("lcm"))
        syntax("il massimo comun divisore e il minimo comune multiplo non sono trattati: servirebbero i loro teoremi (Euclide, Bézout), non solo il simbolo");
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
 * la moltiplicazione implicita. Una sbarra lo può solo fuori da ogni altra
 * sbarra aperta (open_bars). */
static int starts_factor(void) {
    Function fn;
    return token.kind == T_NUM || token.kind == T_LETTER || token.kind == T_OPEN ||
           token.kind == T_ABS_OPEN || (is_char('|') && !open_bars) || is_function(&fn) || is_command("frac") ||
           is_command("dfrac") || is_command("tfrac") || is_command("binom") ||
           is_command("sqrt") || is_command("sum") || is_command("prod") || is_command("pi") || defined_function() >= 0 ||
           is_char(DIFFERENTIAL);
}

/* Quanti segni, prodotti e quozienti sono stati letti, fuori da ogni
 * parentesi, nell'espressione in corso: zero vuol dire che finora è un fattore
 * solo. Serve a parse_expr() per sapere se un operando di un'operazione
 * dichiarata è scritto in modo non ambiguo (9g). */
static int joins;

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
        joins++;
    }
}

/* sum := ['-' | '+'] term { ('+' | '-') term } */
static Node *parse_sum(void) {
    Node *node;
    if (is_char('-')) { next_token(); node = node_new(N_NEG, parse_term(), NULL); joins++; }
    else { if (is_char('+')) next_token(); node = parse_term(); }
    for (;;) {
        if (is_char('+')) { next_token(); node = node_new(N_ADD, node, parse_term()); }
        else if (is_char('-')) { next_token(); node = node_new(N_SUB, node, parse_term()); }
        else return node;
        joins++;
    }
}

/* expr := sum [ operazione sum ]
 *
 * 9g. Un'operazione dichiarata dall'enunciato, tra due operandi. Il nodo è
 * lo stesso di f(x), N_APPLY, con due argomenti. Gli operandi devono essere
 * fattori soli, e dopo il secondo non può seguire un'altra operazione: vedi
 * le convenzioni in testa al capitolo.
 *
 * `joins` conta ciò che questa chiamata legge fuori dalle parentesi: chi è
 * stato chiamato da dentro una parentesi, una graffa o un argomento rimette
 * alla fine il conto di chi sta fuori, per il quale tutto questo è un fattore
 * solo. */
static Node *parse_expr(void) {
    int outside = joins;
    joins = 0;
    Node *node = parse_sum();
    int name = operation_name();
    if (name && map_find(name)) {
        if (joins) syntax("accanto a un'operazione dichiarata somme, prodotti e segni vanno tra parentesi");
        next_token();
        joins = 0;
        Node *right = parse_sum();
        if (joins) syntax("accanto a un'operazione dichiarata somme, prodotti e segni vanno tra parentesi");
        if (operation_name()) syntax("un'operazione dichiarata, se è ripetuta, vuole le parentesi");
        node = node_new(N_APPLY, node, right);
        node->name = name;
    }
    joins = outside;
    return node;
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

/* statement := expr ('=' | \ge | \le | '<' | '>') expr, dal token corrente:
 * lo mette in *statement. */
static void parse_comparison(Statement *statement) {
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
}

/* Analizza text come enunciato e lo mette in *statement. Ritorna 1 se riesce;
 * altrimenti ritorna 0 e syntax_error dice perché. */
static int parse_statement(const char *text, Statement *statement) {
    syntax_error = NULL;
    cursor = text;
    next_token();
    parse_comparison(statement);
    if (token.kind != T_END) syntax("testo inatteso dopo l'enunciato");
    return syntax_error == NULL;
}

/* Analizza text come enunciato su una derivata (vedi DerivativeStatement) e
 * lo mette in *out:
 *
 *     \frac{d^{n}}{dx^{n}} \sin x = \sin\left(x + \frac{n \pi}{2}\right)
 *     \frac{d^{2}}{dx^{2}} x^{3} = 6x,        \frac{d}{dx} \ln x = \frac{1}{x}
 *
 * Il segno di derivata è la frazione di Leibniz (parse_leibniz()). La
 * funzione è il term che segue il segno, come per il corpo di una somma. Se
 * dopo la funzione il testo finisce, l'enunciato è una richiesta, e formula
 * resta NULL:
 *
 *     \frac{d}{dx}\left( x^{2} \tan x \right)
 *
 * Ritorna 1 se text ha una di queste forme; altrimenti ritorna 0 e
 * syntax_error dice perché. */
static int parse_derivative_statement(const char *text, DerivativeStatement *out) {
    memset(out, 0, sizeof *out);
    syntax_error = NULL;
    cursor = text;
    next_token();
    if (!is_command("frac") && !is_command("dfrac") && !is_command("tfrac")) syntax("l'enunciato non comincia con il segno di derivata");
    parse_leibniz(&out->variable, &out->order, &out->times);

    /* La funzione; poi, se non è una richiesta, il segno di uguale e la
     * formula. */
    out->function = parse_term();
    if (token.kind != T_END) {
        expect_char('=', "dopo la funzione da derivare serve = e la formula della derivata");
        out->formula = parse_expr();
    }
    if (token.kind != T_END) syntax("testo inatteso dopo l'enunciato");
    return syntax_error == NULL;
}

/* 9i. I LIMITI
 *
 * Un enunciato su un limite comincia con \lim, o con \nexists \lim, e la
 * grammatica è quella in testa al capitolo. Il punto a cui tende la
 * variabile non si legge con le regole delle espressioni, perché il lato
 * si scrive come un esponente: in 0^{+} il + non è un esponente di 0. Si
 * stacca quindi dal testo l'eventuale ^{+} o ^{-} finale, e ciò che resta è
 * +\infty, -\infty, oppure un'espressione. Lo stesso vale per il valore
 * dopo il segno =. */

/* Ritorna text dopo gli spazi e i comandi di sola presentazione
 * \displaystyle e \textstyle, come li salta next_token(). */
static const char *skip_layout(const char *text) {
    static const char *const style[] = {"\\displaystyle", "\\textstyle"};
    for (int skipped = 1; skipped;) {
        skipped = 0;
        while (isspace((unsigned char)*text)) text++;
        for (size_t i = 0; i < sizeof style / sizeof *style; i++) {
            size_t length = strlen(style[i]);
            if (strncmp(text, style[i], length) == 0 && !isalpha((unsigned char)text[length])) { text += length; skipped = 1; }
        }
    }
    return text;
}

/* Ritorna 1 se text comincia, dopo spazi e comandi di presentazione, con
 * \lim oppure con \nexists \lim: allora è un enunciato su un limite. */
static int starts_with_limit(const char *text) {
    text = skip_layout(text);
    if (strncmp(text, "\\nexists", 8) == 0 && !isalpha((unsigned char)text[8])) text = skip_layout(text + 8);
    return strncmp(text, "\\lim", 4) == 0 && !isalpha((unsigned char)text[4]);
}

/* Se il testo [from, to) è, a meno di spazi, "+\infty" ritorna 1, se è
 * "-\infty" ritorna -1, se è "\infty" senza segno ritorna 2; altrimenti 0. */
static int infinity_text(const char *from, const char *to) {
    int sign = 2;
    while (from < to && isspace((unsigned char)*from)) from++;
    while (to > from && isspace((unsigned char)to[-1])) to--;
    if (from < to && (*from == '+' || *from == '-')) {
        sign = *from == '+' ? 1 : -1;
        for (from++; from < to && isspace((unsigned char)*from); from++) {}
    }
    return to - from == 6 && strncmp(from, "\\infty", 6) == 0 ? sign : 0;
}

/* Ritorna la graffa che chiude quella aperta subito prima di text, o NULL
 * se manca. Le graffe scritte \{ e \} non contano. */
static const char *brace_close(const char *text) {
    for (int depth = 1; *text; text++) {
        if (*text == '\\' && (text[1] == '{' || text[1] == '}')) { text++; continue; }
        if (*text == '{') depth++;
        if (*text == '}' && --depth == 0) return text;
    }
    return NULL;
}

/* Se il punto [point, *end) finisce con il lato, ^{+}, ^{-}, ^+ o ^-, porta
 * *end all'accento circonflesso e ritorna +1 o -1; altrimenti ritorna 0. */
static int side_suffix(const char *point, const char **end) {
    const char *q = *end;
    while (q > point && isspace((unsigned char)q[-1])) q--;
    int braced = q > point && q[-1] == '}';
    if (braced) for (q--; q > point && isspace((unsigned char)q[-1]); q--) {}
    if (q == point || (q[-1] != '+' && q[-1] != '-')) return 0;
    int side = q[-1] == '+' ? 1 : -1;
    for (q--; q > point && isspace((unsigned char)q[-1]); q--) {}
    if (braced && (q == point || q[-1] != '{')) return 0;
    if (braced) for (q--; q > point && isspace((unsigned char)q[-1]); q--) {}
    if (q == point || q[-1] != '^') return 0;
    *end = q - 1;
    return side;
}

/* Analizza text come enunciato su un limite (vedi LimitStatement) e lo
 * mette in *out:
 *
 *     \lim_{x \to 0} \frac{\sin x}{x} = 1,     \lim_{x \to 0^{+}} x^{x},
 *     \lim_{x \to +\infty} x^{2} = +\infty,    \nexists \lim_{x \to 0} \frac{1}{x}.
 *
 * Ritorna 1 se text ha questa forma; altrimenti ritorna 0 e syntax_error
 * dice perché. */
static int parse_limit_statement(const char *text, LimitStatement *out) {
    static char point_text[MAX_FORMULA];
    const char *shape = "un limite si scrive \\lim_{x \\to a} f, con a un numero, +\\infty o -\\infty, oppure a^{+} o a^{-}";
    memset(out, 0, sizeof *out);
    syntax_error = NULL;
    cursor = text;
    next_token();
    if (is_command("nexists")) { out->claim = LIMIT_NONE; next_token(); }
    out->from = token.start;
    if (!is_command("lim")) { syntax(shape); return 0; }
    next_token();
    expect_char('_', shape);

    /* Il pedice: la variabile, \to, il punto fino alla graffa che chiude. */
    const char *close = is_char('{') ? brace_close(cursor) : NULL;
    if (!close) { syntax(shape); return 0; }
    next_token();
    if (token.kind != T_LETTER) { syntax(shape); return 0; }
    out->variable = token.ch;
    out->sequence = out->variable == 'n';
    next_token();
    if (!is_command("to") && !is_command("rightarrow")) { syntax(shape); return 0; }
    const char *point = cursor, *end = close;
    out->side = side_suffix(point, &end);
    if ((size_t)(end - point) >= sizeof point_text) { syntax("enunciato troppo lungo"); return 0; }
    memcpy(point_text, point, (size_t)(end - point));
    point_text[end - point] = 0;

    /* Il punto: un infinito, oppure un'espressione. Senza segno l'infinito
     * è ambiguo per una variabile reale (+\infty, -\infty, o |x| che
     * cresce?), non per l'indice di una successione. */
    out->infinity = infinity_text(point_text, point_text + strlen(point_text));
    if (out->infinity == 2 && !out->sequence) { syntax("\\infty senza segno: scrivere +\\infty oppure -\\infty"); return 0; }
    if (out->infinity == 2) out->infinity = 1;
    if (out->infinity && out->side) { syntax("+\\infty e -\\infty non hanno un lato da cui avvicinarsi"); return 0; }
    if (out->sequence && out->infinity != 1) { syntax("una successione ha limite solo per n \\to +\\infty"); return 0; }
    if (!out->infinity) {
        cursor = point_text;
        next_token();
        out->point = parse_expr();
        if (token.kind != T_END) syntax("testo inatteso nel punto a cui tende la variabile");
        if (syntax_error) return 0;
    }

    /* La funzione, fino al segno = o alla fine; poi, se c'è, il valore. */
    cursor = close + 1;
    next_token();
    out->function = parse_expr();
    out->to = token.start;
    if (is_char('=') && out->claim != LIMIT_NONE) {
        int infinite = infinity_text(cursor, cursor + strlen(cursor));
        if (infinite) {
            out->claim = infinite == 2 ? LIMIT_UNSIGNED : infinite > 0 ? LIMIT_PLUS : LIMIT_MINUS;
            cursor = "";
            next_token();
        } else {
            next_token();
            out->claim = LIMIT_VALUE;
            out->value = parse_expr();
        }
    }
    if (token.kind != T_END) syntax("testo inatteso dopo l'enunciato");
    return syntax_error == NULL;
}

/* 9j. GLI INTEGRALI
 *
 *     \int \frac{x + 2}{x^{2} - 4x + 4} \, dx = \ln|x - 2| - \frac{4}{x - 2} + c
 *     \int_{0}^{1} \ln\left(1 + x^{2}\right) dx = \ln 2 - 2 + \frac{\pi}{2}
 *     \int \frac{dx}{\sqrt{x} + \sqrt[3]{x}}        \int_{2}^{3} \frac{2x - 1}{x^{2} - 9x + 20} \, dx
 *
 * Le convenzioni, che i libri lasciano implicite:
 *
 *   - Il differenziale è la lettera d seguita da un'altra lettera, la
 *     variabile: dx, dt. È un token solo, e conta come un fattore che vale
 *     1: può stare in fondo, f \, dx, oppure in un numeratore,
 *     \frac{dx}{g} e \frac{x \, dx}{g}. Deve comparire una volta sola.
 *   - La funzione integranda è tutto ciò che sta tra \int e il dx. Se è una
 *     somma, il dx deve chiudere l'ultimo termine: \int x^{2} + 1 \, dx è
 *     l'integrale di x^2 + 1. Un dx nel mezzo, \int x \, dx + 1, o dentro
 *     l'argomento di una funzione, è un errore di sintassi, non una lettura
 *     da indovinare.
 *   - Gli estremi, \int_{a}^{b} o \int^{b}_{a}, sono gruppi come gli
 *     esponenti: \int_0^1 vale da 0 a 1.
 *   - In tutto l'enunciato una sbarra | dove comincia un fattore apre un
 *     valore assoluto, che la sbarra seguente chiude: \ln|x - 2| come
 *     \ln\left|x - 2\right|. Fuori dagli integrali la sbarra da sola resta un
 *     errore, perché lì i libri la usano anche per altro.
 *   - Una primitiva si scrive con la sua costante, F + c (o + C, + k, + K):
 *     senza, \int f \, dx = F direbbe che l'integrale indefinito è una sola
 *     funzione, e non è ciò che i libri intendono.
 *
 * Che cosa vuol dire l'enunciato lo stabilisce il motore (capitolo 30). */

/* Ritorna 1 se target è un fattore di tutto tree: ci si arriva scendendo per
 * prodotti, numeratori di frazioni e segni meno. */
static int node_is_factor(const Node *tree, const Node *target) {
    if (tree == target) return 1;
    if (tree->kind == N_MUL) return node_is_factor(tree->a, target) || node_is_factor(tree->b, target);
    return (tree->kind == N_DIV || tree->kind == N_NEG) && node_is_factor(tree->a, target);
}

/* Ritorna una copia di tree senza il fattore target, che node_is_factor()
 * ha trovato: un prodotto per target diventa l'altro fattore. Il resto
 * dell'albero è condiviso. */
static Node *node_without(Node *tree, const Node *target) {
    if (tree->kind == N_MUL && tree->a == target) return tree->b;
    if (tree->kind == N_MUL && tree->b == target) return tree->a;
    if (tree->kind == N_ADD || tree->kind == N_SUB) return node_new(tree->kind, tree->a, node_without(tree->b, target));
    if (tree->kind == N_MUL) return node_is_factor(tree->a, target) ? node_new(N_MUL, node_without(tree->a, target), tree->b)
                                                                   : node_new(N_MUL, tree->a, node_without(tree->b, target));
    if ((tree->kind == N_DIV || tree->kind == N_NEG) && tree->a != target) return node_new(tree->kind, node_without(tree->a, target), tree->b);
    return tree;  /* \frac{dx}{g}: il dx vale già 1, e resta come numeratore */
}

/* Ritorna 1 se il nodo è la lettera della costante di una primitiva. */
static int is_integration_constant(const Node *node) {
    return node->kind == N_VAR && (node->name == 'c' || node->name == 'C' || node->name == 'k' || node->name == 'K');
}

/* Ritorna 1 se text comincia con \int, dopo eventuali \displaystyle. */
static int starts_with_integral(const char *text) {
    syntax_error = NULL;
    cursor = text;
    next_token();
    int found = is_command("int");
    syntax_error = NULL;
    return found;
}

/* Analizza text come enunciato su un integrale (vedi IntegralStatement e le
 * convenzioni di 9j) e lo mette in *out. Ritorna 1 se riesce; altrimenti
 * ritorna 0 e syntax_error dice perché. */
static int parse_integral_statement(const char *text, IntegralStatement *out) {
    const char *misplaced = "il differenziale dx deve moltiplicare tutta la funzione integranda, una volta sola";
    memset(out, 0, sizeof *out);
    syntax_error = NULL;
    cursor = text;
    differential_node = NULL;
    differentials = 0;

    /* \int e gli estremi, in qualsiasi ordine. Il differenziale si accende
     * prima del token che segue, che può già essere il dx. */
    next_token();
    if (!is_command("int")) syntax("l'enunciato non comincia con \\int");
    differential_open = 1;
    next_token();
    for (;;) {
        if (is_char('_') && !out->lower) { next_token(); out->lower = parse_group(); }
        else if (is_char('^') && !out->upper) { next_token(); out->upper = parse_group(); }
        else break;
    }
    if (!out->lower != !out->upper) syntax("un integrale definito vuole tutti e due gli estremi");
    if (differentials) syntax(misplaced);

    /* La funzione integranda, con il suo dx al posto giusto. Un comando che
     * la grammatica non ha la ferma prima del dx, e va detto come tale. */
    Node *integrand = parse_expr();
    differential_open = 0;
    if (token.kind == T_COMMAND) syntax("comando LaTeX non supportato");
    if (!syntax_error) {
        const Node *last = integrand;
        while (last->kind == N_ADD || last->kind == N_SUB) last = last->b;
        if (differentials != 1 || !node_is_factor(last, differential_node)) syntax(misplaced);
        else {
            out->integrand = node_without(integrand, differential_node);
            out->variable = differential_letter;
        }
    }

    /* La risposta, se c'è: per un integrale indefinito F + c, e la
     * costante si stacca qui. */
    if (is_char('=')) {
        next_token();
        Node *answer = parse_expr();
        int indefinite = !out->lower;
        if (token.kind == T_COMMAND) syntax("comando LaTeX non supportato");
        if (syntax_error) {
            /* L'errore c'è già: la costante non si cerca. */
        } else if (indefinite && is_integration_constant(answer)) {
            out->constant = answer->name;
            answer = node_num(0);
        } else if (indefinite && answer->kind == N_ADD && is_integration_constant(answer->b)) {
            out->constant = answer->b->name;
            answer = answer->a;
        } else if (indefinite) {
            syntax("manca la costante: un integrale indefinito si scrive F + c");
        }
        if (out->constant && out->constant == out->variable) syntax("la costante di una primitiva deve essere una lettera diversa dalla variabile");
        out->answer = answer;
    }
    if (token.kind != T_END) syntax("testo inatteso dopo l'enunciato");
    return syntax_error == NULL;
}

/* Analizza text come enunciato su una successione definita per ricorrenza
 * (vedi SequenceStatement) e lo mette in *out:
 *
 *     sequence := given { ',' given } \implies statement { \land statement }
 *     given    := expr '=' expr | lettera \ge numero
 *
 * per esempio
 *
 *     a_{0} = 0, a_{1} = \sin x, a_{n + 1} = 2\cos x\, a_{n} - a_{n - 1}, n \ge 1 \implies a_{n} = \sin(n x)
 *
 * La virgola separa le ipotesi; \, è uno spazio e non conta. Ritorna 1 se
 * riesce; altrimenti ritorna 0 e syntax_error dice perché. */
static int parse_sequence_statement(const char *text, SequenceStatement *out) {
    memset(out, 0, sizeof *out);
    syntax_error = NULL;
    cursor = text;
    next_token();

    /* Le ipotesi. "n \ge q" si riconosce dopo averla letta: una lettera,
     * la relazione, un numero. */
    for (;;) {
        Statement given;
        parse_comparison(&given);
        if (syntax_error) return 0;
        int number = given.rhs->kind == N_NUM || (given.rhs->kind == N_NEG && given.rhs->a->kind == N_NUM);
        if (given.relation == REL_GE && given.lhs->kind == N_VAR && number && !out->has_since) {
            out->has_since = 1;
            out->since_name = given.lhs->name;
            out->since = given.rhs->kind == N_NUM ? given.rhs->value : -given.rhs->a->value;
        } else if (given.relation != REL_EQ) {
            syntax("le ipotesi su una successione sono uguaglianze, più al massimo una della forma n \\ge numero");
        } else if (out->givens == MAX_GIVEN) {
            syntax("troppe ipotesi su una successione");
        } else {
            out->given[out->givens++] = given;
        }
        if (!is_char(',')) break;
        next_token();
    }
    if (is_command("implies") || is_command("Rightarrow") || is_command("Longrightarrow")) next_token();
    else syntax("dopo le ipotesi su una successione serve \\implies e la tesi");

    /* Le tesi. */
    for (;;) {
        if (out->claims == MAX_CLAIMS) { syntax("troppe tesi su una successione"); break; }
        parse_comparison(&out->claim[out->claims++]);
        if (!is_command("land") && !is_command("wedge")) break;
        next_token();
    }
    if (token.kind != T_END) syntax("testo inatteso dopo l'enunciato");
    return syntax_error == NULL;
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
 *     negation    := \lnot negation | relation
 *                  | (\exists | \forall) lettera { ',' lettera } [limit] negation
 *     limit       := \subseteq union | \in \mathcal{P} '(' formula ')'
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
 * I quantificatori. L'albero ne ha uno solo, "esiste un insieme": gli altri
 * modi di scrivere si riducono a quello qui, nel lettore. "Per ogni X vale P"
 * è "non esiste X per cui P non vale". Con un limite, \exists X \subseteq A
 * (o \exists X \in \mathcal{P}(A)) vuol dire "esiste X contenuto in A tale
 * che", e \forall X \subseteq A "per ogni X, se è contenuto in A allora".
 * Più lettere dopo lo stesso quantificatore sono più quantificatori, tutti
 * con lo stesso limite.
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
 * Warning: se aggiungi un tipo prima di SET_APPLY, aggiungi il ramo che lo
 * costruisce nel parser di questo capitolo, il suo tipo in set_check(), e il
 * suo case in member() o set_holds() (capitolo 15), in concrete() o
 * concrete_holds() e in put_set() (capitolo 16), e in parts_unfold() (17r).
 * SET_APPLY e SET_PAIR_IN sono della notazione dei libri (17r): li ha solo
 * l'enunciato com'è scritto, parts_unfold() li toglie, e i capitoli 15 e 16
 * li vedono solo in concrete() e concrete_holds(), per il controllo dello
 * svolgimento. */
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
    SET_EXISTS,      /* esiste un insieme, di indice var, tale che a */
    SET_APPLY,       /* 17r: l'applicazione di posto var in part_rules[]
                        applicata all'insieme a */
    SET_PAIR_IN      /* 17r: la coppia di insiemi (a, b) sta nella relazione
                        di posto var in part_rules[] */
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
 * comparsa: la lettera di indice i è set_var_name[i]. Al più MAX_SET_VARS,
 * tranne mentre si legge un enunciato di 17r (parts_world): lì le lettere
 * delle regole e quelle dei quantificatori di parti diverse della tesi sono
 * di più, e prima del capitolo 15 lo svolgimento le riduce (parts_reindex()). */
static int set_var_name[MAX_SET_LETTERS];
static int set_var_count;

/* 1 mentre si legge un enunciato su relazioni e applicazioni tra le parti di
 * un insieme (17r). */
static int parts_world;

/* Ritorna l'indice della lettera name, registrandola se è nuova. */
static int set_var_index(int name) {
    for (int i = 0; i < set_var_count; i++)
        if (set_var_name[i] == name) return i;
    if (set_var_count == (parts_world ? MAX_SET_LETTERS : MAX_SET_VARS)) {
        syntax(parts_world ? "troppe lettere in un enunciato su relazioni o applicazioni tra parti: al massimo 12"
                           : "troppi insiemi distinti in un enunciato: al massimo 4");
        return 0;
    }
    set_var_name[set_var_count] = name;
    return set_var_count++;
}

/* 17r. Una relazione tra le parti di un insieme, G = \{(X, Y) \in
 * \mathcal{P}(S)^{2} \mid c\}, o un'applicazione di \mathcal{P}(S) in sé con
 * la sua regola, \forall X \in \mathcal{P}(S)\ (f(X) = e). È un nome per la
 * condizione c, o per l'espressione e, nelle lettere param[]: (A, B) \in G
 * vuol dire c con A e B al posto di X e Y, f(A) vuol dire e con A al posto
 * di X. Vedi la testa di 17r per le convenzioni.
 *
 * Invariante: rule non contiene quantificatori, quindi nessuna lettera degli
 * argomenti può esserne catturata; contiene solo le lettere param[] e quelle
 * libere dell'enunciato. */
typedef struct {
    int name;      /* la lettera G o f */
    int arity;     /* 2 per una relazione, 1 per un'applicazione */
    int param[2];  /* le lettere della regola: indici in set_var_name */
    int base;      /* l'insieme S, di cui sono parti gli argomenti */
    SetNode *rule; /* la condizione o l'espressione; NULL per un'applicazione
                      dichiarata di cui non si è ancora letta la regola */
} PartRule;

/* Le relazioni e le applicazioni tra parti dell'enunciato in corso. */
static PartRule part_rules[MAX_PART_RULES];
static int part_rule_count;

/* Ritorna il posto in part_rules[] della relazione o dell'applicazione di
 * nome name, o -1. */
static int part_rule_find(int name) {
    for (int i = 0; i < part_rule_count; i++)
        if (part_rules[i].name == name) return i;
    return -1;
}

/* Ciò che il lettore ritorna al posto di una dichiarazione di 17r: una
 * condizione sempre vera, \emptyset \subseteq \emptyset, che lo svolgimento
 * toglie dalle congiunzioni (parts_unfold()). Si riconosce dall'indirizzo. */
static SetNode parts_declared_empty = {SET_EMPTY, TY_ANY, 0, NULL, NULL, NULL, 0};
static SetNode parts_declared = {SET_SUBSET, TY_ANY, 0, &parts_declared_empty, &parts_declared_empty, NULL, 0};

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
static SetNode *set_union(void);

/* primary: vedi la grammatica in testa al capitolo. */
static SetNode *set_primary(void) {
    /* Parentesi. */
    if (token.kind == T_OPEN) {
        next_token();
        SetNode *inner = set_formula();
        if (token.kind == T_CLOSE) next_token(); else syntax("manca la parentesi chiusa");
        return inner;
    }

    /* 17r. Un'applicazione tra parti applicata, f(A). Una relazione tra
     * parti non è un insieme di cui si parli: se ne dice solo (A, B) \in G. */
    int rule = parts_world && token.kind == T_LETTER ? part_rule_find(token.ch) : -1;
    if (rule >= 0 && part_rules[rule].arity == 2) syntax("una relazione tra parti si usa solo come (A, B) \\in G");
    if (rule >= 0 && part_rules[rule].arity == 1) {
        SetNode *node = set_node(SET_APPLY, NULL, NULL);
        node->var = rule;
        next_token();
        if (token.kind == T_OPEN) next_token(); else syntax("un'applicazione tra parti va scritta con l'argomento tra parentesi, f(X)");
        node->a = set_union();
        if (token.kind == T_CLOSE) next_token(); else syntax("manca la parentesi chiusa");
        return node;
    }

    /* Una lettera maiuscola è un insieme; in 17r anche una minuscola, perché
     * i libri scrivono \forall a, b \in \mathcal{P}(S). */
    if (token.kind == T_LETTER && (isupper(token.ch) || (parts_world && rule < 0))) {
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

/* 17r. \mathcal{P}(S): legge l'insieme delle parti di una lettera, di cui il
 * token corrente è \mathcal, e ritorna l'indice della lettera. */
static int parts_of(void) {
    next_token();
    expect_char('{', "dopo \\mathcal serve {P}");
    if (token.kind != T_LETTER || token.ch != 'P') syntax("l'unico \\mathcal riconosciuto è \\mathcal{P}, l'insieme delle parti");
    next_token();
    expect_char('}', "dopo \\mathcal serve {P}");
    if (token.kind == T_OPEN) next_token(); else syntax("dopo \\mathcal{P} serve l'insieme tra parentesi");
    int base = token.kind == T_LETTER && isupper(token.ch) ? set_var_index(token.ch) : 0;
    if (token.kind == T_LETTER && isupper(token.ch)) next_token(); else syntax("dopo \\mathcal{P} serve la lettera di un insieme tra parentesi");
    if (token.kind == T_CLOSE) next_token(); else syntax("manca la parentesi chiusa");
    return base;
}

/* 17r. Ritorna 1 se in n c'è un quantificatore, o una relazione o
 * un'applicazione tra parti: una regola di 17r non deve averne (vedi
 * PartRule). */
static int parts_rule_unfit(const SetNode *n) {
    if (!n) return 0;
    if (n->kind == SET_EXISTS || n->kind == SET_APPLY || n->kind == SET_PAIR_IN) return 1;
    return parts_rule_unfit(n->a) || parts_rule_unfit(n->b);
}

/* 17r. Prova a leggere, in un enunciato su relazioni e applicazioni tra le
 * parti di un insieme, una delle tre scritture che il capitolo 14 non ha:
 *
 *     G = \{(X, Y) \in \mathcal{P}(S)^{2} \mid c\}   una relazione tra parti
 *     f \colon \mathcal{P}(S) \to \mathcal{P}(S)       un'applicazione tra parti
 *     (A, B) \in G, (A, B) \notin G                    l'appartenenza a G
 *
 * Ritorna il nodo (parts_declared per le prime due), oppure NULL, con il
 * lettore dov'era, se ciò che segue è altro. */
static SetNode *parts_relation(void) {
    ParserState before = parser_save();
    const char *kept = consumed;

    /* Una lettera nuova, seguita da = \{ o da \colon. */
    if (token.kind == T_LETTER && part_rule_find(token.ch) < 0) {
        int name = token.ch, base = 0;
        next_token();
        if (is_command("colon")) {
            next_token();
            if (is_command("mathcal")) base = parts_of(); else syntax("in 17r un'applicazione va da \\mathcal{P}(S) a \\mathcal{P}(T)");
            if (is_command("to")) next_token(); else syntax("in una dichiarazione f \\colon A \\to B manca \\to");
            if (is_command("mathcal")) parts_of(); else syntax("in 17r un'applicazione va da \\mathcal{P}(S) a \\mathcal{P}(T)");
            if (part_rule_count == MAX_PART_RULES) syntax("troppe relazioni e applicazioni tra parti in un enunciato");
            if (!syntax_error) part_rules[part_rule_count++] = (PartRule){name, 1, {0, 0}, base, NULL};
            return &parts_declared;
        }
        if (is_char('=')) {
            next_token();
            if (is_char(ESCAPED + '{')) {
                /* (X, Y) \in \mathcal{P}(S)^{2}, o \mathcal{P}(S) \times
                 * \mathcal{P}(S), e la barra. */
                PartRule rule = {name, 2, {0, 0}, 0, NULL};
                next_token();
                if (token.kind == T_OPEN) next_token(); else syntax("una relazione tra parti si scrive G = \\{(X, Y) \\in \\mathcal{P}(S)^{2} \\mid ...\\}");
                for (int i = 0; i < 2 && !syntax_error; i++) {
                    if (token.kind != T_LETTER || part_rule_find(token.ch) >= 0) { syntax("le due componenti della coppia, in una relazione tra parti, sono due lettere"); break; }
                    rule.param[i] = set_var_index(token.ch);
                    next_token();
                    if (i == 0) expect_char(',', "le due componenti della coppia, in una relazione tra parti, sono due lettere");
                }
                if (token.kind == T_CLOSE) next_token(); else syntax("manca la parentesi chiusa");
                if (is_command("in")) next_token(); else syntax("in G = \\{(X, Y) \\in \\mathcal{P}(S)^{2} \\mid ...\\} manca \\in");
                if (is_command("mathcal")) rule.base = parts_of(); else syntax("una relazione tra parti si scrive G = \\{(X, Y) \\in \\mathcal{P}(S)^{2} \\mid ...\\}");
                if (is_command("times")) {
                    next_token();
                    if (!is_command("mathcal") || parts_of() != rule.base) syntax("le due componenti di una relazione tra parti sono parti dello stesso insieme");
                } else {
                    expect_char('^', "una relazione tra parti si scrive G = \\{(X, Y) \\in \\mathcal{P}(S)^{2} \\mid ...\\}");
                    int braces = is_char('{');
                    if (braces) next_token();
                    if (token.kind != T_NUM || token.value != 2) syntax("di un insieme è riconosciuto solo il quadrato, A^{2}");
                    next_token();
                    if (braces) expect_char('}', "manca }");
                }
                if (is_command("mid") || is_char(':')) next_token(); else syntax("in G = \\{(X, Y) \\in \\mathcal{P}(S)^{2} \\mid ...\\} manca \\mid");
                rule.rule = set_formula();
                expect_char(ESCAPED + '}', "manca \\}");
                if (!syntax_error && (rule.param[0] == rule.param[1] || rule.param[0] == rule.base || rule.param[1] == rule.base || parts_rule_unfit(rule.rule)))
                    syntax("la regola di una relazione tra parti ha due lettere diverse, diverse dall'insieme, e nessun quantificatore");
                if (part_rule_count == MAX_PART_RULES) syntax("troppe relazioni e applicazioni tra parti in un enunciato");
                if (!syntax_error) part_rules[part_rule_count++] = rule;
                return &parts_declared;
            }
        }
        parser_restore(before);
        consumed = kept;
    }

    /* (A, B) \in G. */
    if (token.kind == T_OPEN) {
        next_token();
        SetNode *first = set_union();
        if (!syntax_error && is_char(',')) {
            next_token();
            SetNode *second = set_union();
            int closed = token.kind == T_CLOSE;
            if (closed) next_token();
            int negative = is_command("notin");
            if (closed && (negative || is_command("in"))) {
                next_token();
                int rule = token.kind == T_LETTER ? part_rule_find(token.ch) : -1;
                if (rule < 0 || part_rules[rule].arity != 2) syntax("dopo (A, B) \\in serve una relazione tra parti, dichiarata con G = \\{(X, Y) \\in \\mathcal{P}(S)^{2} \\mid ...\\}");
                next_token();
                SetNode *member = set_node(SET_PAIR_IN, first, second);
                member->var = rule;
                return negative ? set_node(SET_NOT, member, NULL) : member;
            }
        }
        parser_restore(before);
        consumed = kept;
    }
    return NULL;
}

/* relation := union [ ('=' | \subseteq | \supseteq | \neq) union ]
 *
 * In 17r, prima le scritture di parts_relation(). */
static SetNode *set_relation(void) {
    if (parts_world) {
        SetNode *read = parts_relation();
        if (read) return read;
    }
    SetNode *left = set_union();
    if (is_char('=')) { next_token(); return set_node(SET_EQ, left, set_union()); }
    if (is_command("subseteq")) { next_token(); return set_node(SET_SUBSET, left, set_union()); }
    if (is_command("neq") || is_command("ne")) { next_token(); return set_node(SET_NEQ, left, set_union()); }

    /* A contiene B è lo stesso che B contenuto in A. */
    if (is_command("supseteq")) { next_token(); return set_node(SET_SUBSET, set_union(), left); }
    return left;
}

/* negation: vedi la grammatica in testa al capitolo, e lì come ogni
 * quantificatore si riduce a "esiste un insieme". */
static SetNode *set_negation(void) {
    if (is_command("lnot") || is_command("neg")) {
        next_token();
        return set_node(SET_NOT, set_negation(), NULL);
    }
    if (is_command("exists") || is_command("forall")) {
        int every = is_command("forall"), var[MAX_SET_VARS], vars = 0;
        do {
            next_token();  /* supera il quantificatore, poi ogni virgola */
            if (token.kind != T_LETTER || (!isupper(token.ch) && !parts_world) || vars == MAX_SET_VARS) {
                syntax("dopo \\exists o \\forall servono le lettere degli insiemi, non più di 4");
                return set_node(SET_EMPTY, NULL, NULL);
            }
            var[vars++] = set_var_index(token.ch);
            next_token();
        } while (is_char(','));

        /* Il limite: l'insieme che deve contenere ogni lettera. */
        SetNode *limit = NULL;
        if (is_command("subseteq")) {
            next_token();
            limit = set_union();
        } else if (is_command("in")) {
            next_token();
            SetNode *parts = set_primary();
            if (parts->kind != SET_POWER) syntax("dopo \\in, in un quantificatore su insiemi, serve un insieme delle parti \\mathcal{P}(A)");
            limit = parts->a;
        }

        /* 17r. \forall X \in \mathcal{P}(S)\ (f(X) = e), con f un'applicazione
         * tra parti dichiarata e ancora senza regola, è la sua regola: f(A)
         * vorrà dire e con A al posto di X. */
        SetNode *node = set_negation();
        if (parts_world && every && vars == 1 && limit && limit->kind == SET_VAR && node->kind == SET_EQ && node->a->kind == SET_APPLY &&
            node->a->a->kind == SET_VAR && node->a->a->var == var[0] && !part_rules[node->a->var].rule) {
            PartRule *rule = &part_rules[node->a->var];
            if (limit->var != rule->base || var[0] == rule->base || parts_rule_unfit(node->b))
                syntax("la regola di un'applicazione tra parti vale su tutte le parti del suo dominio e non ha quantificatori");
            rule->param[0] = var[0];
            rule->rule = node->b;
            return &parts_declared;
        }

        /* Un quantificatore per lettera, dall'ultima alla prima. */
        for (int i = vars - 1; i >= 0 && !syntax_error; i--) {
            if (limit) {
                SetNode *letter = set_node(SET_VAR, NULL, NULL);
                letter->var = var[i];
                node = set_node(every ? SET_IMPLIES : SET_AND, set_node(SET_SUBSET, letter, limit), node);
            }
            if (every) node = set_node(SET_NOT, node, NULL);
            node = set_node(SET_EXISTS, node, NULL);
            node->var = var[i];
            if (every) node = set_node(SET_NOT, node, NULL);
        }
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
    case SET_APPLY:      n->type = plain ? TY_SET : TY_ERROR; break;      /* 17r */
    case SET_PAIR_IN:    n->type = plain ? TY_FORMULA : TY_ERROR; break;  /* 17r */
    default:
        n->type = formulas ? TY_FORMULA : TY_ERROR; break;
    }
    return n->type;
}

/* 17r. RELAZIONI E APPLICAZIONI TRA LE PARTI DI UN INSIEME
 *
 * (Sta qui, in fondo al capitolo 14, perché sono enunciati di quel capitolo;
 * la lettera è venuta dopo quelle di 17p e 17q.)
 *
 * I libri danno relazioni tra le parti di un insieme, G = \{(X, Y) \in
 * \mathcal{P}(S)^{2} \mid X \cap Y = \emptyset\}, e applicazioni di
 * \mathcal{P}(S) in sé, f(X) = S \setminus X, e ne chiedono le proprietà:
 * G è simmetrica, f è iniettiva. Una volta svolte sono enunciati del
 * capitolo 14, con dei quantificatori su insiemi, e il capitolo 15 li decide
 * per regioni. Il lettore di quel capitolo, con parts_world acceso, legge
 * anche:
 *
 *     G = \{(X, Y) \in \mathcal{P}(S)^{2} \mid c\}   (o \mathcal{P}(S) \times \mathcal{P}(S))
 *     f \colon \mathcal{P}(S) \to \mathcal{P}(S)
 *     \forall X \in \mathcal{P}(S)\ (f(X) = e)          (la regola di f)
 *     (A, B) \in G, (A, B) \notin G, f(A)
 *
 * e lettere minuscole per gli insiemi, \forall a, b \in \mathcal{P}(S). Le
 * convenzioni:
 *
 *   - (A, B) \in G vuol dire A \subseteq S \land B \subseteq S \land c[A / X,
 *     B / Y]: la relazione è tra parti di S, e i libri, che scrivono \forall
 *     X, Y\ ((X, Y) \in G \implies ...) senza dire dove variano X e Y,
 *     intendono le parti di S. Con le due inclusioni il quantificatore può
 *     variare su ogni insieme: fuori da \mathcal{P}(S) la condizione è vera
 *     a vuoto, e l'enunciato è lo stesso.
 *   - f(A) vuol dire e[A / X]. Gli esercizi applicano f solo a parti di S.
 *   - Le regole non hanno quantificatori, né altre relazioni o applicazioni:
 *     allora una sostituzione non può catturare una lettera, perché nella
 *     regola nessuna lettera è legata.
 *
 * Lo svolgimento (parts_unfold()) sostituisce sull'albero; poi
 * parts_reindex() dà ai quantificatori di parti diverse della tesi, che
 * nel testo hanno lettere diverse, le stesse lettere, quando non stanno uno
 * dentro l'altro: \forall a, b\ (...) \land \forall y\ \exists x\ (...) usa
 * due lettere, non quattro, e il capitolo 15 regge al più quattro insiemi.
 * Il controllo, come per 17q, è un calcolo: l'enunciato com'è scritto e
 * quello svolto si calcolano con gli insiemi veri su universi piccoli
 * (concrete_holds(), capitolo 16, che per (A, B) \in G e f(A) calcola la
 * regola con A e B al posto delle sue lettere, senza sostituire nulla), e
 * devono dare lo stesso valore (parts_check(), capitolo 16).
 */

/* Mentre si svolge: l'insieme che va al posto di ogni lettera, o NULL dove
 * la lettera resta com'è. */
static SetNode *parts_to[MAX_SET_LETTERS];

/* Ritorna una copia dell'albero n, nodo per nodo. */
static SetNode *set_copy(const SetNode *n) {
    SetNode *copy = set_node(n->kind, n->a ? set_copy(n->a) : NULL, n->b ? set_copy(n->b) : NULL);
    copy->var = n->var;
    return copy;
}

/* 17r. Ritorna una copia dell'albero n svolto: i nodi SET_APPLY e
 * SET_PAIR_IN al posto dei quali c'è la regola, con le lettere sostituite
 * come dice parts_to[], e le dichiarazioni (parts_declared) tolte dalle
 * congiunzioni. Ritorna parts_declared per un albero fatto solo di
 * dichiarazioni.
 *
 * Ogni nodo dell'albero svolto è nuovo, anche quando un argomento compare
 * due volte: parts_reindex() riscrive le lettere sul posto, e un nodo
 * condiviso verrebbe riscritto due volte. */
static SetNode *parts_unfold(const SetNode *n) {
    if (n == &parts_declared) return &parts_declared;
    if (n->kind == SET_VAR && parts_to[n->var]) return set_copy(parts_to[n->var]);
    SetNode *a = n->a ? parts_unfold(n->a) : NULL, *b = n->b ? parts_unfold(n->b) : NULL;

    /* f(A): la regola con A al posto della sua lettera. (A, B) \in G: le due
     * inclusioni in S, e la regola con A e B al posto delle sue lettere. */
    if (n->kind == SET_APPLY || n->kind == SET_PAIR_IN) {
        const PartRule *rule = &part_rules[n->var];
        SetNode *saved[2] = {parts_to[rule->param[0]], parts_to[rule->param[1]]}, *base = set_node(SET_VAR, NULL, NULL);
        parts_to[rule->param[0]] = a;
        if (rule->arity == 2) parts_to[rule->param[1]] = b;
        SetNode *body = parts_unfold(rule->rule);
        parts_to[rule->param[0]] = saved[0];
        parts_to[rule->param[1]] = saved[1];
        if (n->kind == SET_APPLY) return body;
        base->var = rule->base;
        SetNode *within = set_node(SET_AND, set_node(SET_SUBSET, a, base), set_node(SET_SUBSET, b, set_copy(base)));
        return set_node(SET_AND, within, body);
    }

    /* Le dichiarazioni non affermano nulla: in una congiunzione non restano. */
    if (n->kind == SET_AND && (a == &parts_declared || b == &parts_declared)) return a == &parts_declared ? b : a;
    if (n->kind == SET_IMPLIES && a == &parts_declared) return b;
    SetNode *copy = set_node(n->kind, a, b);
    copy->var = n->var;
    return copy;
}

/* 17r. Ritorna 1 se nell'albero svolto è rimasta una dichiarazione: stava
 * dove non è una congiunzione delle ipotesi, per esempio sotto \lnot, e lì
 * \emptyset \subseteq \emptyset direbbe qualcosa che l'enunciato non dice. */
static int parts_declaration_left(const SetNode *n) {
    if (!n) return 0;
    return n == &parts_declared || parts_declaration_left(n->a) || parts_declaration_left(n->b);
}

/* 17r. Lo stato di parts_reindex(): per ogni lettera legata in vista, la
 * vecchia e la nuova, dalla più esterna; e per ogni lettera libera la nuova,
 * o -1. */
static int reindex_old[MAX_SET_LETTERS], reindex_new[MAX_SET_LETTERS], reindex_count;
static int reindex_free[MAX_SET_LETTERS];

/* 17r. Mette in reindex_free[] l'indice nuovo di ogni lettera libera di n,
 * nell'ordine in cui compaiono; *count conta quelle già trovate. bound dice,
 * per ogni lettera, quanti quantificatori in vista la legano. */
static void reindex_find_free(const SetNode *n, int *bound, int *count) {
    if (!n) return;
    if (n->kind == SET_VAR && !bound[n->var] && reindex_free[n->var] < 0) reindex_free[n->var] = (*count)++;
    if (n->kind == SET_EXISTS) bound[n->var]++;
    reindex_find_free(n->a, bound, count);
    reindex_find_free(n->b, bound, count);
    if (n->kind == SET_EXISTS) bound[n->var]--;
}

/* 17r. Riscrive in n gli indici delle lettere: le libere con quelli di
 * reindex_free[], le legate con il più piccolo indice, dopo le libere, che
 * nessun quantificatore in vista usa. *used tiene il più grande indice
 * usato più uno. */
static void reindex_apply(SetNode *n, int frees, int *used) {
    if (!n) return;
    if (n->kind == SET_VAR) {
        int found = -1;
        for (int i = reindex_count - 1; i >= 0 && found < 0; i--)
            if (reindex_old[i] == n->var) found = reindex_new[i];
        n->var = found >= 0 ? found : reindex_free[n->var];
        return;
    }
    if (n->kind != SET_EXISTS) {
        reindex_apply(n->a, frees, used);
        reindex_apply(n->b, frees, used);
        return;
    }
    if (reindex_count == MAX_SET_LETTERS) { syntax("troppi quantificatori uno dentro l'altro in un enunciato sugli insiemi"); return; }
    int index = frees;
    for (int taken = 1; taken; ) {
        taken = 0;
        for (int i = 0; i < reindex_count; i++) taken = taken || reindex_new[i] == index;
        if (taken) index++;
    }
    if (index + 1 > *used) *used = index + 1;
    reindex_old[reindex_count] = n->var;
    reindex_new[reindex_count++] = index;
    n->var = index;
    reindex_apply(n->a, frees, used);
    reindex_count--;
}

/* 17r. Dà nuovi indici alle lettere dell'albero svolto root (vedi la testa
 * di 17r) e rifà set_var_name[] e set_var_count. Le lettere libere tengono
 * il loro nome; un indice dei quantificatori prende la prima lettera
 * maiuscola che le libere non usano. Se gli indici sono più di
 * MAX_SET_VARS lo annota con syntax(). Mette in free_map[] l'indice nuovo
 * di ogni lettera libera, -1 per le altre: serve al controllo. */
static void parts_reindex(SetNode *root, int *free_map) {
    int bound[MAX_SET_LETTERS] = {0}, frees = 0, used = 0, name[MAX_SET_LETTERS];
    for (int i = 0; i < MAX_SET_LETTERS; i++) reindex_free[i] = -1;
    reindex_find_free(root, bound, &frees);
    memcpy(free_map, reindex_free, sizeof reindex_free);
    for (int i = 0; i < set_var_count; i++)
        if (reindex_free[i] >= 0) name[reindex_free[i]] = set_var_name[i];
    reindex_count = 0;
    used = frees;
    reindex_apply(root, frees, &used);
    if (used > MAX_SET_VARS) { syntax("troppi insiemi distinti in un enunciato: al massimo 4"); return; }

    /* I nomi delle lettere nuove, dopo quelle libere. */
    for (int i = frees; i < used; i++) {
        const char *letter = "XYZWVUABCDGHKLMN";
        for (int taken = 1; taken; letter++) {
            taken = 0;
            for (int j = 0; j < i; j++) taken = taken || name[j] == *letter;
        }
        name[i] = letter[-1];
    }
    memcpy(set_var_name, name, sizeof(int) * (size_t)used);
    set_var_count = used;
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
 *     statement   := formula
 *                  | [ \forall lettera \in numbers ]
 *                    \{ lettera \in numbers (\mid | ':') formula \}
 *     formula     := implication { \iff implication }
 *     implication := disjunction [ \implies implication ]
 *     disjunction := conjunction { \lor conjunction }
 *     conjunction := unary { \land unary }
 *     unary       := \lnot unary
 *                  | (\forall | \exists) lettera { ',' lettera } [\in numbers] unary
 *                  | \exists '!' lettera \in numbers unary
 *                  | lettera \colon numbers \to numbers
 *                  | operazione \colon pairs \to numbers
 *                  | lettera '=' \{ '(' lettera ',' lettera ')' \in pairs
 *                                    (\mid | ':') formula \}
 *                  | '(' expr ',' expr ')' (\in | \notin) lettera
 *                  | expr (\in | \notin) numbers
 *                  | expr (\mid | \nmid) expr
 *                  | expr \equiv expr \pmod group
 *                  | \begin{cases} formula { \\ formula } \end{cases}
 *                  | expr relation expr { relation expr }
 *                  | '(' formula ')'
 *     relation    := '=' | \neq | '<' | '>' | \le | \ge
 *     pairs       := side \times side | side '^' 2
 *     side        := numbers | '(' numbers ')'
 *     numbers     := (\mathbb{N} | \mathbb{N}_{0} | \mathbb{Z} | \mathbb{Q} | \mathbb{Q}^{+}
 *                     | \mathbb{R} | \mathbb{R}^{+})
 *                    { \setminus \{ numero { ',' numero } \} }
 *
 * expr è l'espressione del capitolo 9, che riconosce anche f(x) quando f è
 * un'applicazione dichiarata, e a \ast b quando \ast è un'operazione
 * dichiarata.
 *
 * Definizioni svolte. Tre scritture non sono condizioni nuove ma
 * abbreviazioni, e il lettore le sostituisce con ciò che abbreviano prima
 * che il motore le veda:
 *
 *   - una relazione data da una regola (17i). Dichiarata tra le ipotesi con
 *     R = \{(x, y) \in A \times B \mid c\}, la lettera R è un nome per la
 *     condizione c, e (s, t) \in R vuol dire: s sta in A, t sta in B, e vale
 *     c con s e t al posto di x e y (17j);
 *   - \exists! y \in A\ P, "esiste un solo y": esiste y in A con P, e ogni
 *     z di A con P è uguale a y (17l);
 *   - s \in A con A un insieme numerico resta invece una condizione, F_IN:
 *     il motore sa stabilirla da sé (17k).
 *
 * Divisibilità e congruenze (17n). a \mid b, "a divide b", vuol dire che b
 * è a per un numero intero; a \equiv b \pmod{m} vuol dire che m divide
 * a - b, ed è letto proprio così: un solo tipo di condizione, F_DIVIDES. La
 * barra \mid è anche quella di \{x \in A \mid c\}: lì la consuma chi legge
 * l'insieme, prima che cominci la condizione, quindi non c'è ambiguità.
 *
 * La sostituzione si fa sul testo: si riscrive il pezzo di enunciato con gli
 * argomenti, tra parentesi, al posto delle lettere, e lo si rilegge con
 * questa stessa grammatica (unfold()). Così ogni nodo dell'albero ha ancora
 * il suo pezzo di testo, quello svolto, e la dimostrazione scritta cita ciò
 * che è stato davvero dimostrato. Due cautele rendono la sostituzione fedele:
 * le parentesi intorno a ogni argomento che non sia una lettera sola, e
 * lettere nuove, mai usate nell'enunciato, per ogni variabile legata dentro
 * la regola, così che nessuna lettera degli argomenti venga catturata da un
 * quantificatore della regola.
 *
 * Quantificatori senza insieme (17m). I libri scrivono "R è simmetrica" come
 * \forall x, y\ ((x, y) \in R \implies (y, x) \in R), senza dire dove
 * variano x e y. Lo si accetta solo quando l'insieme non cambia il senso:
 * quando la condizione stessa obbliga la lettera a stare in un insieme, cioè
 * quando fuori da quell'insieme un "per ogni" è vero e un "esiste" è falso
 * comunque. Allora la lettera varia in quell'insieme; in ogni altro caso
 * l'enunciato è respinto, perché letto alla lettera parlerebbe di "ogni
 * oggetto", e non è ciò che un libro intende.
 *
 * Tre scritture dei libri di scuola. Una catena di confronti, a < b \le c,
 * dice a < b e b \le c. Un sistema, \begin{cases} ... \end{cases}, è la
 * congiunzione delle sue righe. E \{x \in \mathbb{R} \mid c\}, l'insieme
 * delle soluzioni di c, non afferma nulla: chiede per quali x vale c, e la
 * risposta del motore è un verdetto RISOLTO. Per questo sta solo in cima a un
 * enunciato (17h). Davanti può avere un solo "per ogni", quello di un
 * parametro: \forall a \in \mathbb{R}\ \{x \in \mathbb{R} \mid c\} chiede le
 * soluzioni in x per ogni valore di a.
 *
 * I numeri reali e le relazioni d'ordine li tratta una regola sola del
 * motore, la tabella dei segni (18p): le altre lavorano sui razionali con =
 * e \neq, e non per caso. La regola 18b, che esclude gli zeri di un
 * polinomio, trova solo le radici razionali, e tra i numeri reali x^2 = 2 ha
 * soluzioni che quella regola non vede.
 *
 * Dopo il parsing, formula_check() controlla ciò che la grammatica non dice:
 * che ogni lettera sia quantificata, e una volta sola; che le espressioni
 * siano razionali; che un'applicazione sia applicata a una variabile del suo
 * dominio.
 *
 * Gli insiemi qualsiasi. Tutto ciò vale quando le lettere variano tra i
 * numeri. Un enunciato in cui variano in un insieme S di cui non si sa
 * nulla, e l'operazione o l'applicazione non hanno una regola, ha una
 * grammatica e un albero suoi: sono in fondo al capitolo, nel paragrafo 17o,
 * con la notazione dei libri (17p) e il suo svolgimento (17q).
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
    if (a->integers != b->integers || a->reals != b->reals || a->bounded != b->bounded || a->holes != b->holes) return 0;
    if (a->bounded && (a->low != b->low || a->strict != b->strict)) return 0;
    for (int i = 0; i < a->holes; i++)
        if (numbers_contain(b, a->hole[i])) return 0;
    return 1;
}

/* Ritorna 1 se ogni elemento di a è anche un elemento di b. Serve dove a un
 * elemento di a si chiede solo di stare in b: l'argomento di un'applicazione
 * il cui dominio è b (argument_in()), una coppia di una relazione tra
 * elementi di b (member_is_plain()).
 *
 * Si guarda una proprietà alla volta: b può chiedere numeri interi, o
 * razionali; può avere un estremo inferiore, che a deve allora rispettare; e
 * i punti che b esclude non devono stare in a. */
static int numbers_subset(const NumberSet *a, const NumberSet *b) {
    if ((b->integers && !a->integers) || (a->reals && !b->reals)) return 0;
    if (b->bounded && (!a->bounded || a->low < b->low || (a->low == b->low && b->strict && !a->strict))) return 0;
    for (int i = 0; i < b->holes; i++)
        if (numbers_contain(a, b->hole[i])) return 0;
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
 * establish() (capitolo 18) e in sample_holds() (capitolo 19). La tabella
 * dei segni (18p) prende per un confronto ogni nodo che non sia un
 * connettivo: un tipo nuovo che non lo è va fermato prima, in
 * decide_quantified(), come F_IN e F_DIVIDES. */
typedef enum {
    F_EQUAL, F_DISTINCT,              /* left = right, left diverso da right */
    F_LESS, F_LESS_EQ,                /* left < right, left <= right: solo con la
                                         tabella dei segni (18p) */
    F_AND, F_OR, F_IMPLIES, F_IFF,    /* connettivi tra a e b */
    F_NOT,                            /* non a */
    F_FORALL, F_EXISTS,               /* per ogni / esiste variable in numbers: a */
    F_SOLVE,                          /* \{variable \in numbers \mid a\}: non è una
                                         condizione ma una domanda, "per quali
                                         valori vale a?", e sta solo in cima a
                                         un enunciato, o sotto il "per ogni" di
                                         un parametro (17h) */
    F_MAP,                            /* dichiarazione: di un'applicazione, di
                                         un'operazione o di una relazione data
                                         da una regola */
    F_IN,                             /* left appartiene a numbers (17k) */
    F_DIVIDES                         /* left divide right (17n) */
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
    Node *left, *right;       /* i confronti (da F_EQUAL a F_LESS_EQ): i due membri.
                                 F_IN: left è l'espressione, right è NULL.
                                 F_DIVIDES: left è il divisore, right ciò che
                                 divide */
    int swapped;              /* F_LESS, F_LESS_EQ: 1 se nell'enunciato c'era > o
                                 \ge, e quindi left è il membro scritto a destra.
                                 Serve solo a chi deve mostrare i membri com'erano
                                 scritti (il grafico, 18q). F_IN, F_DIVIDES: 1
                                 se c'era \notin o \nmid, e la condizione è la
                                 negazione */
    Formula *a, *b;           /* le condizioni componenti */
    int variable;             /* F_FORALL, F_EXISTS, F_SOLVE: la lettera legata */
    NumberSet numbers;        /* F_FORALL, F_EXISTS, F_SOLVE: l'insieme su cui varia.
                                 F_IN: l'insieme a cui left appartiene */
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

/* Il valore di un'espressione: è definita in main.c (capitolo 11), dopo
 * questo file. È l'unica cosa del motore che il lettore usa, per sapere quale
 * numero è un punto tolto con \setminus, che può essere scritto come
 * \frac{1}{2}: riscrivere qui un secondo calcolatore di costanti sarebbe una
 * duplicazione. */
static Rat eval(const Node *node);

/* Ritorna 1 se il token corrente è l'apice di un quadrato cartesiano, ^2 o
 * ^{2}: non fa parte dell'insieme numerico, e lo legge parse_pairs(). Il
 * lettore resta dov'era. */
static int square_follows(void) {
    ParserState here = parser_save();
    const char *here_consumed = consumed;
    int caret = is_char('^');
    next_token();
    if (is_char('{')) next_token();
    int square = caret && token.kind == T_NUM && token.value == 2 && token.digits == 1;
    parser_restore(here);
    consumed = here_consumed;
    return square;
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
    if ((is_char('_') || is_char('^')) && !square_follows()) {
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
    else if ((letter == 'Q' || letter == 'R') && !zero) { set.bounded = set.strict = plus; set.reals = letter == 'R'; }
    else syntax("insieme numerico non trattato: sono ammessi \\mathbb{N}, \\mathbb{N}_{0}, \\mathbb{Z}, \\mathbb{Q}, \\mathbb{Q}^{+}, \\mathbb{R} e \\mathbb{R}^{+}");

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

/* Prototipi in avanti inevitabili: la grammatica è ricorsiva. */
static Formula *logic_formula(void);
static Formula *logic_unary(void);

/* Le variabili quantificate visibili in un punto dell'enunciato, dalla più
 * esterna. Servono in due momenti diversi: al lettore, che in 17b le aggiunge
 * mentre legge la condizione di un quantificatore (17j e 17m devono sapere
 * dove varia una lettera), e poi a formula_check(). */
static Bound scope[MAX_BOUND];
static int scope_count;

/* Ritorna la variabile quantificata di nome name visibile in questo punto, o
 * NULL se non c'è. */
static const Bound *scope_find(int name) {
    for (int i = 0; i < scope_count; i++)
        if (scope[i].name == name) return &scope[i];
    return NULL;
}

/* Un insieme di coppie dato da una regola, R = \{(x, y) \in A \times B \mid
 * c\} (17i): una corrispondenza tra A e B, o una relazione su A se B = A. Per
 * il programma R è un nome per la condizione c, e ogni (s, t) \in R viene
 * svolto (17j).
 *
 * Invariante: le due lettere sono diverse; [start, end) è il testo di c
 * com'è scritto, e resta valido finché dura la lettura dell'enunciato. */
typedef struct {
    int name;                  /* la lettera R */
    int letter[2];             /* x e y */
    NumberSet numbers[2];      /* A e B */
    const char *start, *end;   /* il testo di c */
    const Formula *condition;  /* c letta una volta: serve ai controlli di
                                  pair_sets_check(), non alle dimostrazioni */
    int bound[MAX_RENAMED];    /* le lettere che c lega: variabili di
                                  quantificatori e indici di somme */
    int bounds;                /* quante sono */
} PairSet;

/* Gli insiemi di coppie dichiarati dall'enunciato in corso. */
static PairSet pair_sets[MAX_PAIR_SETS];
static int pair_set_count;

/* Ritorna l'insieme di coppie di nome name, o NULL se non è stato dichiarato. */
static PairSet *pair_set_find(int name) {
    for (int i = 0; i < pair_set_count; i++)
        if (pair_sets[i].name == name) return &pair_sets[i];
    return NULL;
}

/* letter_taken[c] è 1 se la lettera c compare nell'enunciato in corso, o è
 * già stata data da fresh_letter(). */
static char letter_taken[128];

/* Ritorna una lettera che l'enunciato non usa, e la segna come usata. Se sono
 * finite annota l'errore e ritorna una lettera qualsiasi. Mancano dall'elenco
 * e, che senza quantificatore è il numero di Nepero, e le lettere che in
 * stampa si confondono con delle cifre. */
static int fresh_letter(void) {
    for (const char *c = "abcdfghjkmnpqrstuvwxyzABCDFGHJKLMNPQRSTUVWXYZ"; *c; c++)
        if (!letter_taken[(int)*c]) { letter_taken[(int)*c] = 1; return *c; }
    syntax("le lettere non bastano per svolgere le definizioni dell'enunciato");
    return 'a';
}

/* Un'appartenenza (s, t) \in R che obbliga una lettera a stare in un insieme:
 * `letter` è scritta da sola al posto di una delle due componenti, e
 * l'insieme di quella componente è quello in cui la lettera varia. Chi
 * afferma `atom` afferma quindi anche che la lettera sta nel suo insieme: è
 * ciò che serve a 17m. */
typedef struct {
    const Formula *atom;  /* la condizione che svolge l'appartenenza */
    int letter;           /* la lettera */
} Guard;

/* Le appartenenze di questo tipo lette nell'enunciato in corso. */
static Guard guards[MAX_GUARDS];
static int guard_count;

/* 17m. Ritorna 1 se la condizione f, negata se negated, obbliga la variabile
 * `letter` a stare nell'insieme in cui varia: cioè se ogni volta che f (o la
 * sua negazione) è vera, un'appartenenza di guards[] per quella lettera è
 * vera.
 *
 * Le regole sono quelle della logica, e ognuna è un'implicazione vera: una
 * congiunzione impone ciò che impone una sua parte; una disgiunzione solo ciò
 * che impongono tutte e due; "A implica B" è "non A, oppure B"; "esiste w
 * tale che P" impone ciò che impone P. Un "per ogni w" e un "se e solo se"
 * non impongono nulla: la risposta 0 vuol dire solo "non risulta". */
static int imposes_membership(const Formula *f, int negated, int letter) {
    for (int i = 0; i < guard_count; i++)
        if (guards[i].atom == f && guards[i].letter == letter) return !negated;
    switch (f->kind) {
    case F_NOT:
        return imposes_membership(f->a, !negated, letter);
    case F_AND: case F_OR: case F_IMPLIES: {
        int left = imposes_membership(f->a, f->kind == F_IMPLIES ? !negated : negated, letter);
        int right = imposes_membership(f->b, negated, letter);
        return (f->kind == F_AND) != negated ? left || right : left && right;
    }
    case F_FORALL: case F_EXISTS:
        return (f->kind == F_EXISTS) != negated && imposes_membership(f->a, negated, letter);
    default:
        return 0;
    }
}

/* Il testo che si sta scrivendo per svolgere una definizione, e quanti
 * caratteri sono stati scritti in tutto per l'enunciato in corso: il secondo
 * numero ha un tetto, MAX_UNFOLDED, perché una definizione dentro l'altra
 * moltiplica la lunghezza a ogni livello. */
static char draft[4 * MAX_FORMULA];
static size_t draft_length;
static size_t unfolded;

/* Aggiunge alla bozza i caratteri [from, to). Se non c'è posto annota
 * l'errore. */
static void draft_add(const char *from, const char *to) {
    size_t length = (size_t)(to - from);
    if (length >= sizeof draft - draft_length) { syntax("enunciato troppo lungo, una volta svolte le definizioni"); return; }
    memcpy(draft + draft_length, from, length);
    draft_length += length;
}

/* Aggiunge alla bozza la stringa word. */
static void draft_word(const char *word) { draft_add(word, word + strlen(word)); }

/* Dove il lettore era arrivato quando ha cominciato a leggere un altro testo:
 * serve a tornarci. */
typedef struct {
    ParserState state;
    const char *consumed;
} Detour;

/* Porta il lettore all'inizio di text, e ritorna il punto in cui era. */
static Detour detour_begin(const char *text) {
    Detour detour = {parser_save(), consumed};
    cursor = text;
    next_token();
    return detour;
}

/* Riporta il lettore dov'era prima di detour_begin(). Un errore incontrato
 * nel frattempo resta: ferma anche la lettura a cui si torna. */
static void detour_end(Detour detour) {
    const char *problem = syntax_error;
    parser_restore(detour.state);
    consumed = detour.consumed;
    if (problem) syntax(problem);
}

/* Aggiunge alla bozza il testo [start, end) dell'enunciato in cui ogni
 * lettera from[i], per i < count, è sostituita dalla stringa to[i].
 *
 * Il testo viene diviso in token dallo stesso lettore che lo leggerà: così
 * una "lettera" è ciò che la grammatica chiama lettera, e la x dentro
 * \exists o \max non viene toccata. Ogni altro token è copiato com'è, con
 * gli spazi che lo precedono: due token che erano separati lo restano.
 *
 * Non sono variabili nemmeno le lettere tra le graffe di \mathbb{Z} e di
 * \begin{cases}, che per il lettore sono token "lettera" come gli altri:
 * dopo uno di quei comandi si copia senza sostituire fino alla graffa
 * chiusa. Prima non era così, e con una relazione le cui due lettere erano
 * (Z, y) e la regola \exists k \in \mathbb{Z}\ (Z + k = y), l'appartenenza
 * (N, 0) \in R diventava \exists k \in \mathbb{N}\ (N + k = 0): un altro
 * enunciato, e ne usciva dimostrato un falso. I due casi sono nei test. */
static void draft_rewrite(const char *start, const char *end, const int *from, const char *const *to, int count) {
    size_t length = (size_t)(end - start);
    char *copy = arena_alloc(length + 1);
    if (!copy) return;
    memcpy(copy, start, length);
    copy[length] = 0;

    Detour detour = detour_begin(copy);
    const char *copied = copy;  /* fin dove il testo è già nella bozza */
    int verbatim = 0;           /* 1 tra \mathbb o \begin e la graffa chiusa */
    while (token.kind != T_END) {
        const char *first = token.start, *replacement = NULL;
        if (is_command("mathbb") || is_command("begin") || is_command("end")) verbatim = 1;
        else if (is_char('}')) verbatim = 0;
        for (int i = 0; i < count && !verbatim; i++)
            if (token.kind == T_LETTER && token.ch == from[i]) replacement = to[i];
        next_token();  /* ora `consumed` è la fine del token appena esaminato */
        draft_add(copied, replacement ? first : consumed);
        if (replacement) draft_word(replacement);
        copied = consumed;
    }
    detour_end(detour);
}

/* Legge come formula il testo della bozza, che è la definizione svolta di un
 * pezzo di enunciato, e ne ritorna l'albero. Il testo viene copiato
 * nell'arena, dove i nodi lo citano; poi il lettore riprende da dov'era. */
static Formula *unfold(void) {
    char *text = arena_alloc(draft_length + 1);
    unfolded += draft_length;
    if (unfolded > MAX_UNFOLDED) syntax("enunciato troppo lungo, una volta svolte le definizioni");
    if (!text || syntax_error) text = "";
    else { memcpy(text, draft, draft_length); text[draft_length] = 0; }

    Detour detour = detour_begin(text);
    Formula *node = logic_formula();
    if (token.kind != T_END) syntax("testo inatteso dopo una definizione svolta");
    detour_end(detour);
    return node;
}

/* Ritorna la condizione "0 = 0" con il testo che comincia in start: è ciò
 * che una regola ritorna dopo un errore di sintassi, quando l'albero non
 * verrà usato. */
static Formula *logic_nothing(const char *start) {
    Formula *nothing = formula_node(F_EQUAL, NULL, NULL, start);
    nothing->left = nothing->right = node_num(0);
    return nothing;
}

/* side := numbers | '(' numbers ')' */
static NumberSet parse_side(void) {
    if (token.kind != T_OPEN) return parse_numbers();
    next_token();
    NumberSet set = parse_numbers();
    if (token.kind == T_CLOSE) next_token(); else syntax("manca la parentesi chiusa");
    return set;
}

/* pairs := side \times side | side '^' 2
 *
 * Mette in side[0] e side[1] gli insiemi delle due componenti. */
static void parse_pairs(NumberSet *side) {
    side[0] = parse_side();
    if (is_command("times")) {
        next_token();
        side[1] = parse_side();
        return;
    }
    side[1] = side[0];
    if (!is_char('^')) { syntax("serve un insieme di coppie: A \\times B oppure A^{2}"); return; }
    next_token();
    int braces = is_char('{');
    if (braces) next_token();
    if (token.kind != T_NUM || token.value != 2 || token.digits != 1) syntax("di un insieme è riconosciuto solo il quadrato, A^{2}");
    next_token();
    if (braces) expect_char('}', "manca }");
}

/* Aggiunge a set->bound l'indice di ogni somma o prodotto dell'espressione
 * node. */
static void bound_indices(const Node *node, PairSet *set) {
    if (!node) return;
    if (node->kind == N_BIGOP) {
        if (set->bounds == MAX_RENAMED) { syntax("troppe lettere legate nella regola di una relazione"); return; }
        set->bound[set->bounds++] = node->name;
    }
    bound_indices(node->a, set);
    bound_indices(node->b, set);
    bound_indices(node->c, set);
}

/* Aggiunge a set->bound ogni lettera legata nella condizione f: le variabili
 * dei quantificatori e gli indici delle somme. */
static void bound_letters(const Formula *f, PairSet *set) {
    if (!f) return;
    if (f->kind == F_FORALL || f->kind == F_EXISTS) {
        if (set->bounds == MAX_RENAMED) { syntax("troppe lettere legate nella regola di una relazione"); return; }
        set->bound[set->bounds++] = f->variable;
    }
    bound_indices(f->left, set);
    bound_indices(f->right, set);
    bound_letters(f->a, set);
    bound_letters(f->b, set);
}

/* 17i. Una relazione data da una regola: legge ciò che segue "R = \{" nella
 * dichiarazione R = \{(x, y) \in A \times B \mid c\} e registra R in
 * pair_sets[]. name è la lettera R e start l'inizio della dichiarazione.
 *
 * La condizione c viene letta anche qui, una volta, con x e y visibili: per
 * sapere dove finisce il suo testo, quali lettere lega, e perché
 * pair_sets_check() possa poi controllare che non contenga altre lettere. Le
 * dimostrazioni non useranno quest'albero ma una copia svolta per ogni
 * (s, t) \in R. */
static Formula *logic_pair_set(int name, const char *start) {
    PairSet set;
    memset(&set, 0, sizeof set);
    set.name = name;

    /* (x, y) \in A \times B, e la barra. */
    if (token.kind == T_OPEN) next_token(); else syntax("una relazione data da una regola si scrive R = \\{(x, y) \\in A \\times B \\mid ...\\}");
    for (int i = 0; i < 2 && !syntax_error; i++) {
        if (token.kind != T_LETTER) { syntax("le due componenti della coppia, in una relazione data da una regola, devono essere due lettere"); break; }
        set.letter[i] = token.ch;
        next_token();
        if (i == 0) expect_char(',', "le due componenti della coppia, in una relazione data da una regola, devono essere due lettere");
    }
    if (token.kind == T_CLOSE) next_token(); else syntax("le due componenti della coppia, in una relazione data da una regola, devono essere due lettere");
    if (is_command("in")) next_token(); else syntax("in R = \\{(x, y) \\in A \\times B \\mid ...\\} manca \\in");
    parse_pairs(set.numbers);
    if (is_command("mid") || is_char(':')) next_token(); else syntax("in R = \\{(x, y) \\in A \\times B \\mid ...\\} manca \\mid");

    /* Una dichiarazione sta fuori dai quantificatori: la sua regola non può
     * dipendere da una lettera che cambia. */
    if (scope_count) syntax("una relazione si dichiara tra le ipotesi, fuori dai quantificatori");
    else if (set.letter[0] == set.letter[1]) syntax("le due componenti della coppia devono essere due lettere diverse");
    else if (pair_set_find(name) || map_find(name)) syntax("la stessa lettera è dichiarata due volte");
    else if (pair_set_count == MAX_PAIR_SETS) syntax("troppe relazioni dichiarate in un enunciato");

    /* La condizione, con le due lettere visibili. */
    set.start = token.start;
    for (int i = 0; i < 2 && !syntax_error; i++) scope[scope_count++] = (Bound){set.letter[i], &set.numbers[i]};
    set.condition = logic_formula();
    scope_count = 0;
    set.end = consumed;
    expect_char(ESCAPED + '}', "manca \\}");
    bound_letters(set.condition, &set);
    if (!syntax_error) pair_sets[pair_set_count++] = set;
    return formula_node(F_MAP, NULL, NULL, start);
}

/* Ritorna 1 se l'espressione node è fatta solo di numeri e operazioni: è un
 * numero scritto per esteso, come -\frac{1}{2}. */
static int node_is_numeral(const Node *node) {
    if (!node) return 1;
    if (node->kind == N_NUM) return 1;
    if (node->kind != N_NEG && node->kind != N_ADD && node->kind != N_SUB && node->kind != N_MUL && node->kind != N_DIV) return 0;
    return node_is_numeral(node->a) && node_is_numeral(node->b);
}

/* Ritorna 1 se "argument appartiene a numbers" è vero per come l'enunciato è
 * scritto, senza bisogno di dimostrarlo: argument è una variabile che varia
 * in quell'insieme o in una sua parte, oppure un numero che vi appartiene.
 * Se è una variabile che varia proprio in quell'insieme mette la lettera in
 * *letter, altrimenti 0.
 *
 * Una lettera di un quantificatore senza insieme (17m), che ancora non sa
 * dove varia, lo impara qui: il suo insieme diventa numbers. È l'unico punto
 * in cui il lettore scrive nell'insieme di una variabile già in vista, e per
 * questo toglie il const al puntatore di `scope`. */
static int member_is_plain(const Node *argument, const NumberSet *numbers, int *letter) {
    Num value;
    *letter = 0;
    if (argument->kind == N_VAR) {
        const Bound *variable = scope_find(argument->name);
        if (!variable) return 0;
        if (!variable->numbers->start) *(NumberSet *)variable->numbers = *numbers;
        if (numbers_equal(variable->numbers, numbers)) *letter = argument->name;
        return numbers_subset(variable->numbers, numbers);
    }
    return node_is_numeral(argument) && rat_number(eval(argument), &value) && value.im.num == 0 && numbers_contain(numbers, value.re);
}

/* IL CONTROLLO DELLO SVOLGIMENTO (17j)
 *
 * Svolgere "(s, t) \in R" vuol dire riscrivere un testo: la regola di R con
 * gli argomenti al posto delle due lettere. Se la riscrittura sbaglia, il
 * programma dimostra un altro enunciato, e ogni controllo successivo lo
 * conferma, perché controlla l'enunciato riscritto. È già successo: una
 * lettera dentro \mathbb{Z} era stata presa per una variabile.
 *
 * Per questo lo svolgimento si rifà in un secondo modo, che non passa dal
 * testo: nell'albero della regola, letta una volta sola quando R è stata
 * dichiarata, si mettono gli alberi degli argomenti al posto delle due
 * lettere (formula_substitute()), e il risultato deve essere la stessa formula
 * che il lettore ha ricavato dal testo riscritto. "La stessa" a meno dei
 * nomi delle lettere legate (formula_alike()): se nella regola c'è
 * un'altra relazione, svolta anche lei, i suoi quantificatori hanno ricevuto
 * lettere nuove diverse nelle due letture. Le due strade hanno in comune
 * solo il lettore: nell'albero non ci sono né parentesi da aggiungere né
 * lettere da riconoscere dentro un comando.
 */

/* Ritorna una copia dell'espressione node in cui ogni variabile from[i], per
 * i < count, è sostituita dall'albero to[i]. L'indice di una somma che è una
 * delle from[i] prende il nome della variabile to[i]: è il modo di
 * rinominare una lettera legata. Gli alberi to[i] non vengono copiati: gli
 * alberi non si modificano mai. */
static Node *node_substitute(const Node *node, const int *from, const Node *const *to, int count) {
    if (!node) return NULL;
    for (int i = 0; i < count && node->kind == N_VAR; i++)
        if (node->name == from[i]) return (Node *)to[i];
    Node *copy = node_new(node->kind, node_substitute(node->a, from, to, count), node_substitute(node->b, from, to, count));
    copy->c = node_substitute(node->c, from, to, count);
    copy->value = node->value;
    copy->op = node->op;
    copy->name = node->name;
    for (int i = 0; i < count && node->kind == N_BIGOP; i++)
        if (node->name == from[i] && to[i]->kind == N_VAR) copy->name = to[i]->name;
    return copy;
}

/* Ritorna una copia della formula f con la stessa sostituzione di
 * node_substitute() in ogni sua espressione; la lettera di un quantificatore
 * che è una delle from[i] prende il nome della variabile to[i]. Tutto il
 * resto della formula (il testo da cui viene, l'insieme di un
 * quantificatore) passa alla copia com'è. Serve anche al motore, per
 * mettere un numero al posto di un parametro (18t). */
static Formula *formula_substitute(const Formula *f, const int *from, const Node *const *to, int count) {
    if (!f) return NULL;
    Formula *a = formula_substitute(f->a, from, to, count), *b = formula_substitute(f->b, from, to, count);
    Formula *copy = formula_node(f->kind, a, b, f->start);
    *copy = *f;
    copy->a = a;
    copy->b = b;
    copy->left = node_substitute(f->left, from, to, count);
    copy->right = node_substitute(f->right, from, to, count);
    for (int i = 0; i < count; i++)
        if (f->variable == from[i] && to[i]->kind == N_VAR) copy->variable = to[i]->name;
    return copy;
}

/* Le lettere legate incontrate scendendo insieme in due formule che si
 * stanno confrontando: alike_first[i] nella prima corrisponde ad
 * alike_second[i] nella seconda. È una pila: l'ultima coppia è la più
 * interna. */
static int alike_first[MAX_RENAMED + MAX_BOUND], alike_second[MAX_RENAMED + MAX_BOUND], alike_count;

/* Ritorna 1 se le due espressioni sono la stessa a meno dei nomi delle
 * lettere legate: una variabile legata nell'una deve essere legata nello
 * stesso punto nell'altra, una libera deve avere lo stesso nome. */
static int node_alike(const Node *a, const Node *b) {
    if (!a || !b) return a == b;
    if (a->kind != b->kind || a->value != b->value || a->op != b->op) return 0;
    if (a->kind == N_VAR) {
        for (int i = alike_count - 1; i >= 0; i--)
            if (alike_first[i] == a->name || alike_second[i] == b->name) return alike_first[i] == a->name && alike_second[i] == b->name;
        return a->name == b->name;
    }
    if (a->kind != N_BIGOP) return a->name == b->name && node_alike(a->a, b->a) && node_alike(a->b, b->b) && node_alike(a->c, b->c);

    /* Una somma: gli estremi stanno fuori dal suo indice, il termine dentro. */
    if (!node_alike(a->a, b->a) || !node_alike(a->b, b->b) || alike_count == MAX_RENAMED + MAX_BOUND) return 0;
    alike_first[alike_count] = a->name;
    alike_second[alike_count++] = b->name;
    int same = node_alike(a->c, b->c);
    alike_count--;
    return same;
}

static int formula_alike(const Formula *a, const Formula *b);

/* Ritorna 1 se le due formule hanno la stessa forma in cima e parti che
 * formula_alike() trova uguali. */
static int formula_alike_here(const Formula *a, const Formula *b) {
    if (!a || !b) return a == b;
    int binder = a->kind == F_FORALL || a->kind == F_EXISTS || a->kind == F_SOLVE;
    if (a->kind != b->kind || a->swapped != b->swapped || !node_alike(a->left, b->left) || !node_alike(a->right, b->right)) return 0;
    if ((binder || a->kind == F_IN) && !numbers_equal(&a->numbers, &b->numbers)) return 0;
    if (!binder) return formula_alike(a->a, b->a) && formula_alike(a->b, b->b);
    if (alike_count == MAX_RENAMED + MAX_BOUND) return 0;
    alike_first[alike_count] = a->variable;
    alike_second[alike_count++] = b->variable;
    int same = formula_alike(a->a, b->a);
    alike_count--;
    return same;
}

/* Ritorna 1 se f è fatta solo di appartenenze a un insieme numerico. */
static int only_memberships(const Formula *f) {
    return f->kind == F_IN || (f->kind == F_AND && only_memberships(f->a) && only_memberships(f->b));
}

/* Ritorna 1 se le due formule sono la stessa a meno dei nomi delle lettere
 * legate (vedi node_alike()); a è quella letta dal testo riscritto.
 *
 * In a possono esserci, davanti a un pezzo, delle appartenenze che in b
 * mancano. Capita quando la regola di una relazione nomina un'altra
 * relazione: (y, x) \in R, letta dove x e y sono lettere del loro insieme,
 * non ha bisogno di dire che vi appartengono; riletta con 2a al posto di x
 * lo dice. Quelle appartenenze le aggiunge lo svolgimento della relazione
 * interna, che ha il suo controllo. */
static int formula_alike(const Formula *a, const Formula *b) {
    if (formula_alike_here(a, b)) return 1;
    return a && a->kind == F_AND && only_memberships(a->a) && formula_alike(a->b, b);
}

/* 17j. Prova a leggere "(s, t) \in R" oppure "(s, t) \notin R", con R una
 * relazione dichiarata; il token corrente è la parentesi aperta. Ritorna la
 * condizione svolta, o NULL se ciò che segue non ha questa forma: allora il
 * lettore va rimesso dov'era da chi chiama.
 *
 * Svolgere vuol dire scrivere la definizione di R con s e t al posto delle
 * sue due lettere:
 *
 *     s \in A \land t \in B \land (c con s al posto di x e t al posto di y)
 *
 * Le prime due parti si omettono quando sono vere per come l'enunciato è
 * scritto (member_is_plain()). Nel testo di c ogni argomento va tra
 * parentesi, a meno che sia una lettera sola: con y^{2} e t = a + 1 si deve
 * leggere (a + 1)^{2}. E ogni lettera che c lega prende un nome nuovo, mai
 * usato nell'enunciato: se c è \exists k\ (y = kx) e l'argomento s contiene
 * una k, quella k non deve finire sotto il quantificatore di c. */
static Formula *logic_member(const char *start) {
    const Node *argument[2];
    const char *from[2], *to[2];

    /* I due argomenti, il segno di appartenenza, la relazione. */
    next_token();
    for (int i = 0; i < 2; i++) {
        from[i] = token.start;
        argument[i] = parse_expr();
        to[i] = consumed;
        if (syntax_error || (i == 0 ? !is_char(',') : token.kind != T_CLOSE)) return NULL;
        next_token();
    }
    int negative = is_command("notin");
    if (!negative && !is_command("in")) return NULL;
    next_token();
    const PairSet *set = token.kind == T_LETTER ? pair_set_find(token.ch) : NULL;
    if (!set) return NULL;
    next_token();

    /* Che cosa va al posto di ogni lettera della regola: gli argomenti al
     * posto delle due componenti, lettere nuove al posto di quelle legate. */
    int letter[2 + MAX_RENAMED], count = 0;
    const char *replacement[2 + MAX_RENAMED];
    for (int i = 0; i < 2; i++) {
        int length = (int)(to[i] - from[i]), alone = argument[i]->kind == N_VAR && length == 1;
        char *text = arena_alloc((size_t)length + 3);
        if (!text) return logic_nothing(start);
        snprintf(text, (size_t)length + 3, alone ? "%.*s" : "(%.*s)", length, from[i]);
        letter[count] = set->letter[i];
        replacement[count++] = text;
    }
    for (int i = 0; i < set->bounds; i++) {
        char *text = arena_alloc(2);
        if (!text) return logic_nothing(start);
        text[0] = (char)fresh_letter();
        text[1] = 0;
        letter[count] = set->bound[i];
        replacement[count++] = text;
    }

    /* Il testo svolto, e la sua lettura. */
    int plain[2], variable[2];
    draft_length = 0;
    for (int i = 0; i < 2; i++) {
        plain[i] = member_is_plain(argument[i], &set->numbers[i], &variable[i]);
        if (plain[i]) continue;
        draft_word(replacement[i]);
        draft_word(" \\in ");
        draft_add(set->numbers[i].start, set->numbers[i].end);
        draft_word(" \\land ");
    }
    draft_word("(");
    draft_rewrite(set->start, set->end, letter, replacement, count);
    draft_word(")");
    Formula *atom = unfold();

    /* Il controllo dello svolgimento, sull'albero. Ciò che il testo
     * riscritto ha dato, tolte le appartenenze messe davanti, deve essere
     * la regola con gli argomenti al posto delle due lettere e, al posto
     * di ogni lettera legata, la lettera nuova che ha ricevuto. */
    const Node *tree[2 + MAX_RENAMED] = {argument[0], argument[1]};
    for (int i = 2; i < count; i++) {
        Node *renamed = node_new(N_VAR, NULL, NULL);
        renamed->name = (unsigned char)replacement[i][0];
        tree[i] = renamed;
    }
    const Formula *core = plain[0] && plain[1] ? atom : atom->kind == F_AND ? atom->b : NULL;
    alike_count = 0;
    if (!syntax_error && (!core || !formula_alike(core, formula_substitute(set->condition, letter, tree, count))))
        syntax("lo svolgimento di una relazione, rifatto in un secondo modo, non dà la stessa formula");

    /* Chi afferma l'appartenenza afferma che le lettere scritte da sole
     * stanno nel loro insieme: lo si annota per 17m. */
    for (int i = 0; i < 2; i++)
        if (plain[i] && variable[i] && guard_count < MAX_GUARDS) guards[guard_count++] = (Guard){atom, variable[i]};
    return negative ? formula_node(F_NOT, atom, NULL, start) : atom;
}

/* 17l. \exists! y \in A\ P, "esiste un solo y in A tale che P"; il token
 * corrente è il punto esclamativo. Ritorna la definizione svolta:
 *
 *     \exists y \in A\ ((P) \land \forall z \in A\ ((P con z al posto di y) \implies z = y))
 *
 * con z una lettera nuova. P viene letta prima una volta, per sapere dove
 * finisce il suo testo e per avere l'albero con cui ricontrollare lo
 * svolgimento. */
static Formula *logic_unique(const char *start) {
    next_token();
    int name = token.ch;
    if (token.kind != T_LETTER) syntax("dopo \\exists! servono una lettera, \\in e un insieme numerico");
    next_token();
    if (is_command("in")) next_token(); else syntax("dopo \\exists! servono una lettera, \\in e un insieme numerico");
    NumberSet numbers = parse_numbers();

    /* Dove comincia e dove finisce P. */
    const char *from = token.start;
    int outer = scope_count;
    if (scope_count < MAX_BOUND) scope[scope_count++] = (Bound){name, &numbers};
    const Formula *condition = logic_unary();
    scope_count = outer;
    const char *to = consumed;
    if (syntax_error) return logic_nothing(start);

    /* Il testo svolto. */
    char written[2] = {(char)name, 0}, other[2] = {(char)fresh_letter(), 0};
    const char *const replacement[1] = {other};
    draft_length = 0;
    draft_word("\\exists ");
    draft_word(written);
    draft_word(" \\in ");
    draft_add(numbers.start, numbers.end);
    draft_word("\\ ((");
    draft_add(from, to);
    draft_word(") \\land \\forall ");
    draft_word(other);
    draft_word(" \\in ");
    draft_add(numbers.start, numbers.end);
    draft_word("\\ ((");
    draft_rewrite(from, to, &name, replacement, 1);
    draft_word(") \\implies ");
    draft_word(other);
    draft_word(" = ");
    draft_word(written);
    draft_word("))");
    Formula *whole = unfold();

    /* Il controllo dello svolgimento, come per le relazioni: nella formula
     * letta dal testo la prima copia di P deve essere P, e la seconda P con
     * la lettera nuova al posto di y, sostituita sull'albero. */
    Node *renamed = node_new(N_VAR, NULL, NULL);
    renamed->name = (unsigned char)other[0];
    const Node *const tree[1] = {renamed};
    const Formula *both = whole->kind == F_EXISTS ? whole->a : NULL, *every = both && both->kind == F_AND ? both->b : NULL;
    const Formula *step = every && every->kind == F_FORALL ? every->a : NULL;
    alike_count = 0;
    if (!syntax_error && (!step || step->kind != F_IMPLIES || !formula_alike(both->a, condition) ||
                          !formula_alike(step->a, formula_substitute(condition, &name, tree, 1))))
        syntax("lo svolgimento di \\exists!, rifatto in un secondo modo, non dà la stessa formula");
    return whole;
}

/* Prepara la lettura dell'enunciato il cui testo comincia in text: nessuna
 * relazione dichiarata, nessuna variabile in vista, e in letter_taken le
 * lettere che il testo usa. Il lettore resta dov'era. */
static void unfolding_reset(const char *text) {
    pair_set_count = guard_count = scope_count = 0;
    unfolded = 0;
    memset(letter_taken, 0, sizeof letter_taken);

    /* Un errore incontrato qui lo ritroverà la lettura vera, nel punto in
     * cui lo riporterà con più precisione: non va conservato. */
    ParserState here = parser_save();
    const char *here_consumed = consumed;
    cursor = text;
    for (next_token(); token.kind != T_END; next_token())
        if (token.kind == T_LETTER) letter_taken[token.ch & 127] = 1;
    parser_restore(here);
    consumed = here_consumed;
}

/* unary: vedi la grammatica in testa al capitolo. */
static Formula *logic_unary(void) {
    const char *start = token.start;

    /* 17a. Negazione. */
    if (is_command("lnot") || is_command("neg")) {
        next_token();
        Formula *inner = logic_unary();
        return formula_node(F_NOT, inner, NULL, start);
    }

    /* 17l. "Esiste un solo": si distingue da "esiste" al token dopo. */
    if (is_command("exists")) {
        ParserState before = parser_save();
        next_token();
        if (is_char('!')) return logic_unique(start);
        parser_restore(before);
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

        /* L'insieme, lo stesso per tutte le lettere. Se manca (17m) ogni
         * lettera aspetta il suo: numbers[i].start resta NULL finché
         * un'appartenenza della condizione non glielo dà. */
        NumberSet numbers[MAX_BOUND];
        memset(numbers, 0, sizeof numbers);
        int stated = is_command("in") || !pair_set_count;
        if (stated) {
            if (!is_command("in")) syntax("dopo le lettere di un quantificatore servono \\in e un insieme numerico");
            next_token();
            numbers[0] = parse_numbers();
            for (int i = 1; i < names; i++) numbers[i] = numbers[0];
        }

        /* La condizione, letta con le lettere in vista. */
        int outer = scope_count;
        for (int i = 0; i < names && scope_count < MAX_BOUND; i++) scope[scope_count++] = (Bound){name[i], &numbers[i]};
        Formula *node = logic_unary();
        scope_count = outer;

        /* 17m. Senza insieme, il quantificatore ha senso solo se la
         * condizione obbliga ogni lettera a stare nell'insieme che ha
         * ricevuto: per un "per ogni" deve farlo la negazione della
         * condizione, perché è lì che la lettera potrebbe stare fuori. */
        for (int i = 0; i < names && !stated && !syntax_error; i++)
            if (!numbers[i].start || !imposes_membership(node, kind == F_FORALL, name[i]))
                syntax("un quantificatore senza insieme è ammesso solo se la condizione obbliga la lettera a stare in un insieme, con un'appartenenza a una relazione dichiarata");

        /* \forall a, b è \forall a \forall b: un nodo per lettera, costruiti
         * dall'ultima lettera alla prima. */
        for (int i = names - 1; i >= 0; i--) {
            node = formula_node(kind, node, NULL, start);
            node->variable = name[i];
            node->numbers = numbers[i];
        }
        return node;
    }

    /* 17g. Sistema: \begin{cases} c \\ c ... \end{cases} è la congiunzione
     * delle sue righe. Si legge prima l'apertura, poi le righe, poi la
     * chiusura, che ha la stessa forma dell'apertura. */
    if (is_command("begin")) {
        Formula *node = NULL;
        for (int closing = 0; closing < 2 && !syntax_error; closing++) {
            if (!is_command(closing ? "end" : "begin")) { syntax("sistema \\begin{cases} ... \\end{cases} scritto male"); break; }
            next_token();
            expect_char('{', "dopo \\begin e \\end serve {cases}");
            for (const char *word = "cases"; *word && !syntax_error; word++) {
                if (token.kind != T_LETTER || token.ch != *word) syntax("di \\begin{...} è riconosciuto solo cases");
                next_token();
            }
            expect_char('}', "dopo \\begin e \\end serve {cases}");
            if (closing) break;
            node = logic_formula();
            while (!syntax_error && is_char(ESCAPED + '\\')) {
                next_token();
                if (is_command("end")) break;
                node = formula_node(F_AND, node, logic_formula(), start);
            }
        }
        if (node) return node;
        node = formula_node(F_EQUAL, NULL, NULL, start);
        node->left = node->right = node_num(0);
        return node;
    }

    /* 17c. Dichiarazione di un'applicazione, o di un'operazione binaria: al
     * posto della lettera c'è allora un simbolo, e il dominio è un insieme di
     * coppie. Si distingue da un confronto che comincia con una lettera solo
     * al token dopo, \colon: si guarda avanti e, se non c'è, si torna
     * indietro. */
    int symbol = operation_name();
    if (token.kind == T_LETTER || symbol) {
        ParserState before = parser_save();
        int name = symbol ? symbol : token.ch;
        next_token();
        if (is_command("colon")) {
            NumberSet domain[2];
            next_token();
            if (symbol) parse_pairs(domain); else domain[0] = domain[1] = parse_numbers();
            if (is_command("to") || is_command("rightarrow") || is_command("longrightarrow")) next_token();
            else syntax("in una dichiarazione f \\colon A \\to B manca \\to");
            NumberSet codomain = parse_numbers();
            if (map_find(name) || pair_set_find(name)) syntax("la stessa applicazione è dichiarata due volte");
            else if (map_count == MAX_MAPS) syntax("troppe applicazioni dichiarate in un enunciato");
            else if (!syntax_error) maps[map_count++] = (Map){name, domain[0], codomain, 0, NULL, symbol != 0, domain[1], 0};
            return formula_node(F_MAP, NULL, NULL, start);
        }

        /* 17i. Una relazione data da una regola: lettera, uguale, \{. */
        if (!symbol && is_char('=')) {
            next_token();
            if (is_char(ESCAPED + '{')) {
                next_token();
                return logic_pair_set(name, start);
            }
        }
        parser_restore(before);
    }

    /* 17j. Appartenenza a una relazione dichiarata: una coppia tra parentesi.
     * Se non lo è si torna indietro, e la parentesi apre un'espressione o
     * una formula (17d, 17e). */
    if (token.kind == T_OPEN && pair_set_count) {
        ParserState before = parser_save();
        Formula *member = logic_member(start);
        if (member) return member;
        parser_restore(before);
    }

    /* 17d. Confronto tra due espressioni. Anche un confronto può cominciare
     * con una parentesi, come (n + 1)^{2} = 4, quindi prima si prova a leggere
     * un'espressione seguita da = o \neq; se non riesce, si torna indietro e
     * la parentesi deve aprire una formula (17e). */
    ParserState before = parser_save();
    Node *left = parse_expr();
    const char *problem = syntax_error;
    Formula *chain = NULL;

    /* 17k. Appartenenza a un insieme numerico: non una catena di confronti
     * ma una condizione a sé, che il motore stabilisce con la regola 18e.
     * Con \notin il nodo è lo stesso, e `swapped` ricorda la negazione. */
    if (!syntax_error && (is_command("in") || is_command("notin"))) {
        int negative = is_command("notin");
        next_token();
        NumberSet numbers = parse_numbers();
        Formula *node = formula_node(F_IN, NULL, NULL, start);
        node->left = left;
        node->numbers = numbers;
        node->swapped = negative;
        return node;
    }

    /* 17n. Divisibilità e congruenza. a \equiv b \pmod{m} è "m divide
     * a - b": il nodo della differenza nasce qui. */
    if (!syntax_error && (is_command("mid") || is_command("nmid") || is_command("equiv"))) {
        int congruence = is_command("equiv"), negative = is_command("nmid");
        next_token();
        Node *right = parse_expr(), *divisor = left;
        if (congruence) {
            if (is_command("pmod")) next_token(); else syntax("dopo a \\equiv b serve il modulo, scritto \\pmod{m}");
            divisor = parse_group();
            right = node_new(N_SUB, left, right);
        }
        Formula *node = formula_node(F_DIVIDES, NULL, NULL, start);
        node->left = divisor;
        node->right = right;
        node->swapped = negative;
        return node;
    }
    while (!syntax_error) {
        /* La relazione: =, \neq, <, >, \le, \ge. Le ultime due coppie sono
         * la stessa cosa a membri scambiati. */
        int greater = is_char('>') || is_command("ge") || is_command("geq");
        int strict = is_char('<') || is_char('>'), loose = is_command("le") || is_command("leq") || is_command("ge") || is_command("geq");
        int equal = is_char('='), distinct = is_command("neq") || is_command("ne");
        if (!equal && !distinct && !strict && !loose) break;
        next_token();
        Node *right = parse_expr();
        Formula *node = formula_node(equal ? F_EQUAL : distinct ? F_DISTINCT : strict ? F_LESS : F_LESS_EQ, NULL, NULL, start);
        node->left = greater ? right : left;
        node->right = greater ? left : right;
        node->swapped = greater;

        /* Una catena come a < b \le c dice a < b e b \le c. */
        chain = chain ? formula_node(F_AND, chain, node, start) : node;
        left = right;
    }
    if (chain) return chain;
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
    syntax(problem ? problem : "dopo un'espressione serve una relazione: =, \\neq, <, >, \\le, \\ge");
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

/* 17h. statement := formula
 *                  | [ \forall lettera \in insieme ] \{ lettera \in insieme (\mid | :) formula \}
 *
 * La seconda forma è l'insieme delle soluzioni di una condizione, come
 * \{x \in \mathbb{R} \mid \frac{x - 2}{x + 3} \ge 0\}. Chi la scrive non
 * afferma nulla: chiede per quali valori la condizione vale, e la risposta è
 * un verdetto RISOLTO. Per questo sta solo in cima: dentro una formula una
 * \{ resta un errore di sintassi.
 *
 * Con un "per ogni" davanti è la richiesta con un parametro, quella degli
 * esercizi "risolvi al variare di a": l'albero è il quantificatore con la
 * richiesta come condizione. Di quantificatori ce ne può stare uno solo, e
 * senza parentesi attorno alla richiesta. */
static Formula *logic_statement(void) {
    const char *start = token.start;
    unfolding_reset(start);

    /* La richiesta con un parametro si distingue da un enunciato che comincia
     * con lo stesso quantificatore solo dopo l'insieme, dalla \{: si guarda
     * avanti e, se non c'è, si torna indietro, come in 17c e 17d. Trovata la
     * \{, la richiesta la legge questa stessa funzione. */
    if (is_command("forall")) {
        ParserState before = parser_save();
        next_token();
        int name = token.ch, letter = token.kind == T_LETTER;
        next_token();
        if (letter && is_command("in")) {
            next_token();
            NumberSet numbers = parse_numbers();
            if (!syntax_error && is_char(ESCAPED + '{')) {
                Formula *node = formula_node(F_FORALL, logic_statement(), NULL, start);
                node->variable = name;
                node->numbers = numbers;
                return node;
            }
        }
        parser_restore(before);
    }
    if (!is_char(ESCAPED + '{')) return logic_formula();
    next_token();
    int name = token.ch;
    if (token.kind != T_LETTER) syntax("in \\{x \\in A \\mid ...\\} dopo \\{ serve una lettera");
    next_token();
    if (!is_command("in")) syntax("in \\{x \\in A \\mid ...\\} dopo la lettera servono \\in e un insieme numerico");
    next_token();
    NumberSet numbers = parse_numbers();
    if (is_command("mid") || is_char(':')) next_token(); else syntax("in \\{x \\in A \\mid ...\\} manca \\mid");
    Formula *node = formula_node(F_SOLVE, logic_formula(), NULL, start);
    node->variable = name;
    node->numbers = numbers;
    expect_char(ESCAPED + '}', "manca \\}");
    return node;
}

/* Il maggior numero di quantificatori uno dentro l'altro incontrato da
 * formula_check() da quando è stato azzerato. Serve alla verifica sul
 * campione (capitolo 19) per decidere quanti valori provare. */
static int quantifier_depth;

/* Controlla che nell'espressione node ogni lettera abbia un significato;
 * altrimenti annota il motivo con fail().
 *
 * Sono ammesse tutte le espressioni del capitolo 9: le regole del capitolo 18
 * lavorano sui polinomi e lasciano stare ciò che non lo è, mentre l'induzione
 * (18n) e i conti con i numeri trattano anche somme, potenze e funzioni. Una
 * lettera deve essere una variabile quantificata, oppure l'indice di una
 * somma dentro il suo termine. Un'applicazione deve avere per argomento un
 * elemento del suo dominio (argument_in()): è questo a garantire che f(x) sia
 * definito ogni volta che compare, senza doverlo ridimostrare. */
static void term_check(const Node *node);

/* Ritorna 1 se l'espressione node, argomento di un'applicazione, appartiene
 * all'insieme domain per come è scritta. Tre casi, e in nessuno serve una
 * dimostrazione fatta qui:
 *
 *   - una variabile quantificata su quell'insieme, o su una sua parte;
 *   - un numero che vi appartiene;
 *   - il valore di un'applicazione (o di un'operazione) dichiarata con quel
 *     codominio, o con una sua parte: che i suoi valori ci stiano davvero lo
 *     dimostra, una volta per tutte, map_defined() (18m). È ciò che dà senso
 *     a f(g(x)) e a (a \ast b) \ast c.
 *
 * Ogni altra espressione viene respinta: per accettare f(x + 1) bisognerebbe
 * dimostrare che x + 1 sta nel dominio, nel punto in cui compare. */
static int argument_in(const Node *node, const NumberSet *domain) {
    Num value;
    if (node->kind == N_VAR) {
        const Bound *variable = scope_find(node->name);
        return variable && variable->numbers && numbers_subset(variable->numbers, domain);
    }
    if (node->kind == N_APPLY) {
        const Map *inner = map_find(node->name);
        term_check(node);
        return inner && numbers_subset(&inner->codomain, domain);
    }
    return node_is_numeral(node) && rat_number(eval(node), &value) && value.im.num == 0 && numbers_contain(domain, value.re);
}

static void term_check(const Node *node) {
    if (!node || failure) return;
    switch (node->kind) {
    case N_NUM:
        return;
    case N_VAR:
        /* La lettera e, quando nessun quantificatore la lega, è il numero di
         * Nepero. Per le regole che lo trattano come una lettera qualsiasi
         * non cambia nulla: ciò che dimostrano vale per ogni valore, e ciò
         * che smentiscono è un polinomio a coefficienti razionali che non
         * si annulla, e in e non si annulla perché e è trascendente. */
        if (node->name != VAR_PI && node->name != 'e' && !scope_find(node->name)) fail("negli enunciati con quantificatori ogni lettera deve essere quantificata");
        return;
    case N_APPLY: {
        const Map *map = map_find(node->name);
        if (!map->body) fail("un'applicazione è usata prima della regola che la definisce");
        else if (!argument_in(node->a, &map->domain) || (map->binary && !argument_in(node->b, &map->second)))
            fail("un'applicazione va applicata a un elemento del suo dominio: una variabile che vi varia, un numero che vi appartiene, o un valore di un'applicazione che vi arriva");
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
    case N_NUM:
        return 1;
    case N_APPLY:
        /* Gli argomenti: variabili, numeri o altre applicazioni. */
        return term_is_rational(node->a) && term_is_rational(node->b);
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
    case F_EQUAL: case F_DISTINCT: case F_LESS: case F_LESS_EQ:
        term_check(f->left);
        term_check(f->right);
        return;
    case F_IN:
        term_check(f->left);
        return;
    case F_DIVIDES:
        term_check(f->left);
        term_check(f->right);
        return;
    case F_MAP:
        fail("un'applicazione, un'operazione o una relazione si possono dichiarare solo tra le ipotesi");
        return;
    case F_FORALL: case F_EXISTS: case F_SOLVE:
        if (scope_find(f->variable) || map_find(f->variable) || pair_set_find(f->variable)) { fail("la stessa lettera ha due significati nello stesso punto dell'enunciato"); return; }
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

/* Legge le ipotesi f: una congiunzione di dichiarazioni (di applicazioni,
 * operazioni e relazioni), già registrate dal parser, e di regole della forma
 *
 *     \forall x \in A\ (f(x) = espressione)
 *     \forall x \in A\ \forall y \in B\ (x \ast y = espressione)
 *
 * dove A è il dominio di f, e A \times B quello dell'operazione \ast.
 * Completa ogni applicazione con la sua regola; un'ipotesi di altro tipo
 * viene annotata con fail(). */
static void read_definitions(const Formula *f) {
    if (f->kind == F_AND) { read_definitions(f->a); read_definitions(f->b); return; }
    if (f->kind == F_MAP) return;

    /* Riconosce la regola e l'applicazione che definisce: sotto uno o due
     * "per ogni" c'è un'uguaglianza il cui primo membro è l'applicazione,
     * con una variabile per argomento. */
    const Formula *inner = f->kind == F_FORALL && f->a->kind == F_FORALL ? f->a : NULL;
    const Formula *rule = f->kind == F_FORALL ? (inner ? inner->a : f->a) : NULL;
    const Node *applied = rule && rule->kind == F_EQUAL && rule->left->kind == N_APPLY ? rule->left : NULL;
    Map *map = applied ? map_find(applied->name) : NULL;
    if (!map || map->binary != (inner != NULL) || applied->a->kind != N_VAR || (inner && applied->b->kind != N_VAR)) {
        fail("ipotesi non trattata: sono ammesse la dichiarazione di un'applicazione, di un'operazione o di una relazione, e la regola di un'applicazione o di un'operazione");
        return;
    }
    if (map->body) { fail("un'applicazione ha due regole"); return; }

    /* Ogni argomento è la variabile di uno dei quantificatori, che varia
     * sull'insieme di quell'argomento. Per un'operazione le due variabili
     * possono essere quantificate in un ordine e usate nell'altro. */
    const Formula *first = f, *second = inner;
    if (inner && applied->a->name == inner->variable) { first = inner; second = f; }
    if (applied->a->name != first->variable || (inner && (applied->b->name != second->variable || f->variable == inner->variable))) {
        fail("gli argomenti, nella regola di un'applicazione, devono essere le variabili dei suoi quantificatori");
        return;
    }
    if (!numbers_equal(&first->numbers, &map->domain) || (inner && !numbers_equal(&second->numbers, &map->second))) {
        fail("la regola di un'applicazione deve valere su tutto il suo dominio");
        return;
    }

    /* L'espressione può contenere solo le variabili della regola. La si
     * controlla prima di registrarla: così una regola che cita la propria
     * applicazione trova body ancora NULL e viene respinta da term_check(). */
    scope[0] = (Bound){first->variable, &first->numbers};
    scope_count = 1;
    if (inner) scope[scope_count++] = (Bound){second->variable, &second->numbers};
    term_check(rule->right);
    scope_count = 0;

    /* Che la regola dia un valore per ogni elemento del dominio lo dimostra
     * map_defined() (18m), che sa farlo solo guardando i divisori. */
    if (!term_is_rational(rule->right)) { fail("la regola di un'applicazione deve essere un'espressione razionale"); return; }
    map->parameter = first->variable;
    if (inner) map->parameter2 = second->variable;
    map->body = rule->right;
}

/* Controlla la regola di ogni relazione dichiarata (17i): la condizione deve
 * parlare solo delle due lettere della coppia, e non legarle di nuovo.
 * Annota il primo problema con fail().
 *
 * Senza questo controllo una lettera libera nella regola, che da sola non
 * vuol dire nulla, prenderebbe un significato diverso in ogni punto in cui
 * la relazione è usata: quello della variabile con lo stesso nome che lì si
 * trova in vista. Va chiamata dopo la lettura dell'enunciato. */
static void pair_sets_check(void) {
    for (int i = 0; i < pair_set_count && !failure; i++) {
        const PairSet *set = &pair_sets[i];
        scope[0] = (Bound){set->letter[0], &set->numbers[0]};
        scope[1] = (Bound){set->letter[1], &set->numbers[1]};
        scope_count = 2;
        formula_check(set->condition);
        scope_count = 0;
    }
}

/* 17o. INSIEMI QUALSIASI: NOTAZIONE E ALBERO
 *
 * Fin qui ogni lettera variava in un insieme di numeri. Molti enunciati di
 * algebra parlano invece di un insieme qualsiasi, con un'operazione, delle
 * applicazioni o una relazione di cui si sa solo ciò che dicono le ipotesi:
 *
 *     (\perp \colon S \times S \to S \land u \in S \land v \in S
 *         \land \forall x \in S\ (u \perp x = x)
 *         \land \forall x \in S\ (x \perp v = x)) \implies u = v
 *
 * Qui non c'è nessuna regola da sostituire e nessun conto da fare: ci sono
 * solo elementi, simboli e uguaglianze. È un altro linguaggio, con il suo
 * albero (Sentence, le cui foglie sono Elem) e il suo capitolo del motore,
 * il 28. Un enunciato vi arriva quando non nomina insiemi numerici e parla
 * di elementi: dichiara qualcosa con \colon, oppure quantifica una lettera
 * minuscola (lo decide prove(), capitolo 20).
 *
 * La grammatica, dalla precedenza più bassa alla più alta:
 *
 *     statement   := { \exists example } formula
 *     example     := Set { ',' Set } [ \neq \emptyset ]
 *                  | name { ',' name } \subseteq Set [ \times Set ]
 *                  | name \colon Set [ \times Set ] \to Set
 *                  | lettera { ',' lettera } \in Set '^' Set
 *     formula     := implication { \iff implication }
 *     implication := disjunction [ \implies implication ]
 *     disjunction := conjunction { \lor conjunction }
 *     conjunction := unary { \land unary }
 *     unary       := \lnot unary
 *                  | (\forall | \exists) lettera { ',' lettera } [ \in range ] unary
 *                  | name \colon Set [ \times Set ] \to Set      (applicazione, operazione)
 *                  | name \subseteq Set [ \times Set ]           (parte, relazione)
 *                  | lettera \in Set                             (un elemento con un nome)
 *                  | Set \neq \emptyset
 *                  | side ('=' | \neq) side
 *                  | side (\in | \notin) name
 *                  | '(' formula ')'
 *     range       := Set | name | Set '^' Set
 *     side        := term | '(' term ',' term ')'
 *     term        := factor [ operazione factor ]
 *     factor      := name | name '(' term [ ',' term ] ')' | '(' term ')'
 *     name        := (lettera | lettera greca) [ '_' cifra ]
 *
 * Set è una lettera maiuscola che non è stata dichiarata come parte o come
 * relazione: un insieme qualsiasi, di cui non si sa nulla, nemmeno se è
 * vuoto. Le convenzioni di lettura, che sono tutto ciò che questo paragrafo
 * decide:
 *
 *   - Le dichiarazioni stanno tra le ipotesi, in cima: l'enunciato è
 *     "dichiarazioni e ipotesi implicano tesi", e vale per ogni insieme e
 *     per ogni scelta di ciò che è dichiarato. Una parte X \subseteq S è una
 *     proprietà degli elementi di S, una relazione R \subseteq S \times T
 *     una proprietà delle coppie: di loro si scrive solo x \in X e
 *     (x, y) \in R.
 *   - \forall f \in T^{S}, "per ogni applicazione da S a T", è letto solo
 *     dove equivale a una dichiarazione: in testa alla tesi, o sotto altri
 *     "per ogni" e a destra di un'implicazione. Lì "per ogni f vale P(f)" e
 *     "sia f un'applicazione; allora P(f)" dicono lo stesso.
 *   - \forall x \in X, con X una parte dichiarata di S, è "per ogni x di S,
 *     se x sta in X allora", e \exists x \in X è "esiste x in S che sta in
 *     X e". L'albero contiene la forma svolta.
 *   - Un quantificatore senza insieme, \forall x, y\ ((x, y) \in R \implies
 *     ...), è ammesso come in 17m: solo se la condizione obbliga la lettera
 *     a comparire in un'appartenenza, che ne fissa l'insieme.
 *   - Due coppie sono uguali quando lo sono le componenti: (a, b) = (c, d)
 *     è letto a = c \land b = d.
 *   - Un'operazione dichiarata si scrive tra i suoi operandi e, come in 9g,
 *     non ha precedenza: dentro un'altra va tra parentesi.
 *   - In testa all'enunciato, \exists S \neq \emptyset\ \exists R \subseteq
 *     S \times S\ (...) chiede un esempio: degli insiemi e dei simboli per
 *     cui la condizione vale. È l'unico posto in cui un insieme, una parte o
 *     un'applicazione sono cercati e non dati.
 *
 * Ogni elemento ha un insieme: quello della sua variabile, o il codominio
 * del simbolo. Il lettore lo controlla mentre legge (elem_expect()), così il
 * motore non incontra mai f(x) con x fuori dal dominio di f.
 *
 * La grammatica si allarga con la notazione dei libri, nel paragrafo 17p:
 * insiemi di elementi e di coppie scritti con una regola, immagini e
 * controimmagini, uguaglianze e inclusioni tra insiemi, \exists!, lettere
 * con un pedice, variabili che variano in un prodotto e applicazioni a
 * valori in un prodotto. Quei nodi stanno solo nell'albero letto: 17q li
 * svolge in quelli di questo paragrafo.
 */

/* Il "tipo" di ciò che vale vero o falso: il valore di una parte o di una
 * relazione applicata ai suoi argomenti. Non è un insieme dell'enunciato:
 * occupa il posto dopo l'ultimo. */
#define TRUTH MAX_SORTS

/* Le lettere greche che possono fare da nome di un'applicazione. Il nome
 * interno di quella di posto i è GREEK_NAMES + i. */
static const char *const greek_letters[] = {"alpha", "beta", "gamma", "delta", "iota", "kappa", "lambda", "mu", "nu", "pi", "rho",
                                             "sigma", "tau", "phi", "varphi", "chi", "psi", "omega", "eta", "theta", "xi", "zeta"};

/* Un simbolo di cui l'enunciato parla senza darne una regola: un elemento
 * con un nome, un'applicazione, un'operazione, una parte di un insieme o una
 * relazione. Anche gli elementi a cui la dimostrazione dà un nome (capitolo
 * 28) sono simboli, aggiunti in coda.
 *
 * Invariante: due simboli diversi non hanno lo stesso nome e lo stesso
 * pedice. */
typedef struct {
    int name;       /* una lettera; oppure OPERATION_NAMES + i per un simbolo
                       di operazione, GREEK_NAMES + i per una lettera greca */
    int index;      /* il pedice, da 1 in su; 0 se non c'è */
    int arity;      /* quanti argomenti: 0 per un elemento, 1 o 2 */
    int domain[2];  /* l'insieme di ogni argomento: un posto in sort_name[] */
    int range;      /* l'insieme del valore, o TRUTH per parti e relazioni */
    int family;     /* 1 per una famiglia di parti X_{i} (17p): una relazione
                       tra un insieme e quello degli indici, che si scrive
                       x \in X_{i} e non (x, i) \in X */
    int paired;     /* 17p: un'applicazione f a valori in un prodotto,
                       f \colon S \to T \times V, sono due applicazioni, le
                       sue componenti f_{1} e f_{2}: 1 o 2 per quelle, 0 per
                       ogni altro simbolo */
} Symbol;

/* I simboli dell'enunciato in corso, e i suoi insiemi: l'insieme di posto i
 * è la lettera sort_name[i]. */
static Symbol symbols[MAX_SYMBOLS];
static int symbol_count;
static int sort_name[MAX_SORTS];
static int sort_count;

/* Un'espressione che indica un elemento: una variabile, oppure un simbolo
 * applicato ai suoi argomenti (nessuno, per un elemento con un nome). Per
 * una parte o una relazione applicata, il "valore" è vero o falso. I nodi
 * vivono nell'arena. */
typedef struct Elem Elem;
struct Elem {
    int symbol;    /* il simbolo, posto in symbols[]; -1 per una variabile */
    int slot;      /* variabile: il suo posto in slot_sort[] */
    Elem *arg[2];  /* gli argomenti; NULL dove non servono */
};

/* Tipi di nodo di un enunciato su insiemi qualsiasi.
 *
 * Warning: se aggiungi un tipo prima di S_MEMBER, aggiungi il ramo che lo
 * costruisce nel lettore di questo paragrafo e i suoi casi nel capitolo 28:
 * in model_holds(), in known_open(), in truth(), in put_sentence() e in
 * cert_sentence(), e la sua parola al lettore del verificatore (capitolo 11
 * di checker.c); e il suo caso in unfold_sentence() (17q) e in book_holds()
 * (28h). Un tipo da S_MEMBER in poi è invece della notazione dei libri
 * (17p): va in unfold_sentence(), che lo traduce nei primi, e in
 * book_holds(), e il capitolo 28 non lo vede mai. */
typedef enum {
    S_TRUE,                         /* una dichiarazione: non afferma nulla */
    S_EQUAL,                        /* left = right */
    S_HOLDS,                        /* left, una parte o una relazione
                                       applicata, vale */
    S_NOT,                          /* non a */
    S_AND, S_OR, S_IMPLIES, S_IFF,  /* connettivi tra a e b */
    S_FORALL, S_EXISTS,             /* per ogni / esiste un elemento, la
                                       variabile di posto slot: a */
    S_MEMBER,                       /* 17p: gli elementi item[] stanno
                                       nell'insieme set */
    S_INCLUDED,                     /* 17p: set è contenuto in other, e se
                                       proper non è uguale a other */
    S_SAME,                         /* 17p: set e other sono lo stesso insieme */
    S_UNIQUE                        /* 17p: esiste un solo elemento, la
                                       variabile di posto slot, tale che a */
} SentenceKind;

/* Un insieme di elementi o di coppie scritto come nei libri: è definito
 * nel paragrafo 17p, dove si legge. */
typedef struct SetTerm SetTerm;

/* Un nodo dell'albero di un enunciato su insiemi qualsiasi: una condizione.
 * I campi non usati dal tipo sono NULL o zero. I nodi vivono nell'arena. */
typedef struct Sentence Sentence;
struct Sentence {
    SentenceKind kind;
    Elem *left, *right;          /* S_EQUAL: i due membri. S_HOLDS: left */
    Sentence *a, *b;             /* le condizioni componenti */
    int slot;                    /* S_FORALL, S_EXISTS, S_UNIQUE: la
                                    variabile legata */
    const char *start, *end;     /* com'è scritto nell'enunciato: [start, end) */
    const SetTerm *set, *other;  /* S_MEMBER: set. S_INCLUDED, S_SAME: i due
                                    insiemi */
    Elem *item[MAX_ARITY];       /* S_MEMBER: il membro, una componente per
                                    posto */
    int items;                   /* S_MEMBER: quante componenti */
    int proper;                  /* S_INCLUDED: 1 per l'inclusione propria,
                                    \subsetneq */
};

/* Le variabili quantificate dell'enunciato in corso. Ogni quantificatore ha
 * un posto suo, anche se due usano la stessa lettera: così nel capitolo 28
 * una variabile si riconosce dal posto. slot_sort[i] è l'insieme in cui
 * varia, o -1 finché un quantificatore senza insieme non lo ha imparato
 * dall'uso. slot_index[i] è il pedice della lettera, x_{1}, da 1 a 9; 0 se
 * non c'è. Anche le variabili legate da un insieme scritto come nei libri
 * (17p), e quelle che nascono svolgendolo (17q), hanno un posto qui.
 *
 * Una variabile che varia in un prodotto, x \in S \times T (17p), sono due
 * variabili, le sue componenti, che si chiamano x_{1} e x_{2}: slot_pair[i]
 * è il posto della seconda se i è la prima, e -1 altrimenti. */
static int slot_letter[MAX_SLOTS], slot_index[MAX_SLOTS], slot_sort[MAX_SLOTS], slot_pair[MAX_SLOTS];
static int slot_count;

/* I posti delle variabili visibili nel punto che il lettore sta leggendo,
 * dalla più esterna. */
static int visible[MAX_SLOTS];
static int visible_count;

/* 1 finché il lettore non è entrato in una negazione o in un "esiste": lì
 * "per ogni applicazione f" non vale quanto una dichiarazione (vedi le
 * convenzioni in testa al paragrafo). map_quantifiers conta quelli letti,
 * per chi deve accorgersi dopo di averne incontrato uno tra le ipotesi. */
static int abstract_positive;
static int map_quantifiers;

/* 1 se l'enunciato in corso chiede un esempio: comincia con degli "esiste"
 * su insiemi, parti o applicazioni. */
static int abstract_example;

/* Ritorna il posto del simbolo di nome name e pedice index, o -1. */
static int symbol_find(int name, int index) {
    for (int i = 0; i < symbol_count; i++)
        if (symbols[i].name == name && symbols[i].index == index) return i;
    return -1;
}

/* Ritorna il posto della variabile visibile di lettera letter e pedice
 * index, o -1. La seconda componente di una coppia (17p) è visibile quando
 * lo è la prima. */
static int visible_find(int letter, int index) {
    for (int i = visible_count - 1; i >= 0; i--) {
        int slot = visible[i], second = slot_pair[slot];
        if (slot_letter[slot] == letter && slot_index[slot] == index) return slot;
        if (second >= 0 && slot_letter[second] == letter && slot_index[second] == index) return second;
    }
    return -1;
}

/* Ritorna il posto della prima componente della coppia visibile di nome
 * letter (17p), o -1. Se si vede anche una variabile x senza pedice vale la
 * più interna delle due. */
static int pair_find(int letter) {
    for (int i = visible_count - 1; i >= 0; i--) {
        int slot = visible[i];
        if (slot_letter[slot] != letter) continue;
        if (slot_pair[slot] >= 0 && slot_index[slot] == 1) return slot;
        if (slot_index[slot] == 0) return -1;
    }
    return -1;
}

/* Registra symbol tra i simboli dell'enunciato. Un nome già preso, da un
 * simbolo, da un insieme o da una variabile in vista, è un errore. */
static void symbol_add(Symbol symbol) {
    int taken = symbol_find(symbol.name, symbol.index) >= 0 || visible_find(symbol.name, symbol.index) >= 0;
    for (int i = 0; i < sort_count; i++) taken = taken || (!symbol.index && sort_name[i] == symbol.name);
    if (taken) syntax("la stessa lettera ha due significati nello stesso enunciato");
    else if (symbol_count == MAX_SYMBOLS) syntax("troppi simboli dichiarati in un enunciato");
    else symbols[symbol_count++] = symbol;
}

/* name: legge un nome, con il suo pedice, e lo supera. Ritorna il nome
 * interno e mette il pedice in *index; ritorna 0, senza leggere nulla, se il
 * token corrente non è un nome.
 *
 * Se letter non è NULL, il pedice può essere anche una lettera, X_{i}: è
 * l'indice di una famiglia di parti (17p), e va in *letter, con *index a 0.
 * Altrimenti *letter resta 0. */
static int abstract_name_or_section(int *index, int *letter) {
    int name = token.kind == T_LETTER ? token.ch : 0;
    *index = 0;
    if (letter) *letter = 0;
    for (size_t i = 0; !name && i < sizeof greek_letters / sizeof *greek_letters; i++)
        if (is_command(greek_letters[i])) name = GREEK_NAMES + (int)i;
    if (!name) return 0;
    next_token();
    if (!is_char('_')) return name;

    /* Il pedice: una cifra, con o senza graffe; o, dove è ammessa, una
     * lettera. */
    next_token();
    int braces = is_char('{');
    if (braces) next_token();
    if (letter && token.kind == T_LETTER) *letter = token.ch;
    else if (token.kind != T_NUM || token.value < 1 || token.value > 9) syntax("come pedice di un nome è riconosciuta una cifra da 1 a 9");
    else *index = (int)token.value;
    next_token();
    if (braces) expect_char('}', "manca }");
    return name;
}

/* Come abstract_name_or_section(), dove il pedice può essere solo una
 * cifra. */
static int abstract_name(int *index) { return abstract_name_or_section(index, NULL); }

/* Un nome che l'enunciato dà a un insieme, G = \{...\} (17p): ogni G scritto
 * dopo vuol dire term. starred dice se il nome ha l'asterisco, R^{*}, che lo
 * distingue da R. */
typedef struct {
    int name, index, starred;
    SetTerm *term;
} Definition;

/* Le definizioni dell'enunciato in corso. */
static Definition definitions[MAX_DEFINITIONS];
static int definition_count;

/* Ritorna la definizione del nome name, con pedice index e asterisco se
 * starred, o NULL. */
static const Definition *definition_find(int name, int index, int starred) {
    for (int i = 0; i < definition_count; i++)
        if (definitions[i].name == name && definitions[i].index == index && definitions[i].starred == starred) return &definitions[i];
    return NULL;
}

/* Ritorna il posto in sort_name[] dell'insieme di lettera letter,
 * registrandolo se è nuovo. Una lettera che non può indicare un insieme è un
 * errore: una minuscola, o il nome di una parte, di una relazione o di un
 * insieme definito (17p). */
static int sort_index(int letter) {
    if (letter >= 128 || !isupper(letter) || symbol_find(letter, 0) >= 0 || definition_find(letter, 0, 0)) {
        syntax("serve un insieme: una lettera maiuscola che non sia stata dichiarata come parte o come relazione");
        return 0;
    }
    for (int i = 0; i < sort_count; i++)
        if (sort_name[i] == letter) return i;
    if (sort_count == MAX_SORTS) { syntax("troppi insiemi distinti in un enunciato: al massimo 4"); return 0; }
    sort_name[sort_count] = letter;
    return sort_count++;
}

/* Set: legge la lettera di un insieme qualsiasi e la supera. Ritorna il suo
 * posto in sort_name[].
 *
 * Due scritture che qui non sono un insieme hanno un motivo a sé, perché
 * il verdetto dica che cosa manca: \mathcal{P}(S), l'insieme delle parti
 * preso come insieme di oggetti (una famiglia di parti, un'applicazione a
 * valori parti), e un nome tra graffe, {\le}, che i libri usano per una
 * relazione scritta tra gli elementi, x \le y. */
static int abstract_sort(void) {
    if (is_command("mathcal"))
        syntax("l'insieme delle parti \\mathcal{P}(S) come insieme di oggetti (una famiglia di parti, un'applicazione o una relazione su parti) non è trattato: una parte si dichiara con X \\subseteq S");
    if (is_char('{'))
        syntax("una relazione con un nome tra graffe, {\\le}, scritta tra gli elementi come x \\le y, non è trattata: si scrive (x, y) \\in R");
    int letter = token.kind == T_LETTER ? token.ch : 0;
    if (letter) next_token();
    return sort_index(letter);
}

/* Ritorna un nuovo elemento: il simbolo symbol applicato ad a e b, oppure,
 * con symbol = -1, la variabile di posto slot. */
static Elem *elem_new(int symbol, int slot, Elem *a, Elem *b) {
    static Elem spare;
    Elem *e = arena_alloc(sizeof(Elem));
    if (!e) e = &spare;
    *e = (Elem){symbol, slot, {a, b}};
    return e;
}

/* Ritorna l'insieme a cui appartiene e: quello della variabile, che può
 * essere -1 (non ancora noto), o il codominio del simbolo. */
static int elem_sort(const Elem *e) { return e->symbol < 0 ? slot_sort[e->slot] : symbols[e->symbol].range; }

/* Controlla che e sia un elemento dell'insieme sort. Una variabile che non
 * sa ancora dove varia lo impara qui. */
static void elem_expect(const Elem *e, int sort) {
    if (e->symbol < 0 && slot_sort[e->slot] < 0) slot_sort[e->slot] = sort;
    if (elem_sort(e) != sort) syntax("un elemento è usato dove serve un elemento di un altro insieme");
}

/* Ritorna un nuovo nodo di tipo kind con componenti a e b, il cui testo va
 * da start alla fine dell'ultimo token già letto. */
static Sentence *sentence_new(SentenceKind kind, Sentence *a, Sentence *b, const char *start) {
    static Sentence spare;
    Sentence *s = arena_alloc(sizeof(Sentence));
    if (!s) s = &spare;
    memset(s, 0, sizeof *s);
    s->kind = kind;
    s->a = a;
    s->b = b;
    s->start = start;
    s->end = consumed;
    return s;
}

static Elem *abstract_term(void);
static int abstract_tuple(Elem **out, int count);
static Sentence *abstract_formula(void);
static Sentence *abstract_unary(void);

/* Il simbolo symbol applicato ai suoi argomenti, che seguono: il nome è già
 * stato letto. Ogni argomento sta nel suo insieme; una coppia (17p) dà due
 * argomenti: f(x), con x \in S \times T, è f(x_{1}, x_{2}). */
static Elem *abstract_application(int symbol) {
    Elem *e = elem_new(symbol, 0, NULL, NULL), *arg[MAX_ARITY];
    if (symbols[symbol].arity == 0) return e;
    if (token.kind != T_OPEN) { syntax("un'applicazione dichiarata va scritta con i suoi argomenti, f(x)"); return e; }

    /* Le parentesi sono quelle dell'applicazione: dentro, dei membri
     * separati da virgole, ognuno con le sue componenti. */
    int count = 0;
    next_token();
    for (;;) {
        int got = abstract_tuple(arg, count);
        if (!got) break;
        count = got;
        if (!is_char(',')) break;
        next_token();
    }
    if (token.kind == T_CLOSE) next_token(); else syntax("manca la parentesi chiusa");
    if (!syntax_error && count != symbols[symbol].arity)
        syntax("un'applicazione va scritta con tanti argomenti quanti ne ha nella sua dichiarazione");
    for (int i = 0; i < symbols[symbol].arity && !syntax_error; i++) {
        e->arg[i] = arg[i];
        elem_expect(e->arg[i], symbols[symbol].domain[i]);
    }
    return e;
}

/* factor: vedi la grammatica in testa al paragrafo. */
static Elem *abstract_factor(void) {
    if (token.kind == T_OPEN) {
        next_token();
        Elem *inner = abstract_term();
        if (token.kind == T_CLOSE) next_token(); else syntax("manca la parentesi chiusa");
        return inner;
    }

    /* Un nome: una variabile in vista, o un simbolo dichiarato. */
    int index, name = abstract_name(&index);
    int slot = name ? visible_find(name, index) : -1, symbol = name ? symbol_find(name, index) : -1;
    if (slot >= 0) return elem_new(-1, slot, NULL, NULL);
    if (symbol < 0 || symbols[symbol].range == TRUTH) {
        syntax(name && !index && (pair_find(name) >= 0 || (symbol_find(name, 1) >= 0 && symbols[symbol_find(name, 1)].paired))
                   ? "una coppia è scritta dove serve un elemento: le sue componenti sono x_{1} e x_{2}"
               : name ? "una lettera non è né quantificata né dichiarata come elemento o applicazione"
                      : "serve un elemento: una lettera, un'applicazione f(x), o un'operazione tra parentesi");
        return elem_new(-1, 0, NULL, NULL);
    }
    return abstract_application(symbol);
}

/* term := factor [ operazione factor ] */
static Elem *abstract_term(void) {
    Elem *left = abstract_factor();
    int name = operation_name(), symbol = name ? symbol_find(name, 0) : -1;
    if (symbol < 0) return left;
    next_token();
    Elem *right = abstract_factor();
    elem_expect(left, symbols[symbol].domain[0]);
    elem_expect(right, symbols[symbol].domain[1]);
    return elem_new(symbol, 0, left, right);
}

/* side := term | '(' side ',' side ')' | coppia
 *
 * Legge un membro: un elemento, una coppia, una coppia di coppie (17p). Ne
 * mette le componenti in out[], appiattite, dal posto count in poi, e
 * ritorna quante sono in tutto; 0 se ciò che segue non si legge così. Una
 * coppia può avere un nome: una variabile che varia in un prodotto, o il
 * valore di un'applicazione a valori in un prodotto (17p).
 *
 * Una parentesi aperta può cominciare una coppia o un elemento, (a \perp b)
 * \perp c: si prova la coppia e, se dopo il primo membro non c'è la
 * virgola, si torna indietro. */
static int abstract_tuple(Elem **out, int count) {
    if (token.kind == T_OPEN) {
        ParserState before = parser_save();
        const char *kept = consumed;
        next_token();
        int inner = abstract_tuple(out, count);
        if (inner && !syntax_error && is_char(',')) {
            next_token();
            inner = abstract_tuple(out, inner);
            if (token.kind == T_CLOSE) next_token(); else syntax("manca la parentesi chiusa di una coppia");
            return syntax_error ? 0 : inner;
        }

        /* Una coppia con un nome tra parentesi, (x), è ancora una coppia. */
        if (inner > count + 1 && !syntax_error && token.kind == T_CLOSE) {
            next_token();
            return inner;
        }
        parser_restore(before);
        consumed = kept;
    }

    /* Una coppia con un nome: la variabile, o l'applicazione f(...) i cui
     * valori sono coppie, che dà le sue due componenti. */
    if (token.kind == T_LETTER) {
        ParserState before = parser_save();
        const char *kept = consumed;
        int index, name = abstract_name(&index), pair = index ? -1 : pair_find(name);
        int map = !index && visible_find(name, 0) < 0 && symbol_find(name, 0) < 0 && pair < 0 ? symbol_find(name, 1) : -1;
        if (pair >= 0 && count + 2 <= MAX_ARITY) {
            out[count] = elem_new(-1, pair, NULL, NULL);
            out[count + 1] = elem_new(-1, slot_pair[pair], NULL, NULL);
            return count + 2;
        }
        if (map >= 0 && symbols[map].paired == 1 && count + 2 <= MAX_ARITY) {
            /* f(...) con f a valori in un prodotto: le sue due componenti,
             * f_{1} e f_{2}, con gli stessi argomenti. f_{2} è dichiarata
             * subito dopo f_{1} (abstract_declaration()). */
            Elem *first = abstract_application(map);
            if (syntax_error) return 0;
            out[count] = first;
            out[count + 1] = elem_new(map + 1, 0, first->arg[0], first->arg[1]);
            return count + 2;
        }
        parser_restore(before);
        consumed = kept;
    }
    if (count == MAX_ARITY) {
        syntax("un membro ha al più quattro componenti: una coppia di coppie");
        return 0;
    }
    out[count] = abstract_term();
    return syntax_error ? 0 : count + 1;
}

/* Ritorna 1 se la condizione s, negata se negated, obbliga la variabile di
 * posto slot a comparire come membro di una parte, di una relazione o di un
 * insieme scritto come nei libri (17p): cioè se ogni volta che s (o la sua
 * negazione) è vera, lo è una di quelle appartenenze. Le regole sono quelle
 * di imposes_membership() (17m), e la risposta 0 vuol dire solo "non
 * risulta". Un "esiste un solo x" impone ciò che impone "esiste x". */
static int sentence_forces(const Sentence *s, int negated, int slot) {
    switch (s->kind) {
    case S_HOLDS:
        for (int i = 0; i < 2; i++)
            if (s->left->arg[i] && s->left->arg[i]->symbol < 0 && s->left->arg[i]->slot == slot) return !negated;
        return 0;
    case S_MEMBER:
        /* Ogni insieme di 17p ha i suoi membri in un insieme qualsiasi, o in
         * un prodotto di insiemi: chi vi appartiene sta lì. */
        for (int i = 0; i < s->items; i++)
            if (s->item[i]->symbol < 0 && s->item[i]->slot == slot) return !negated;
        return 0;
    case S_NOT:
        return sentence_forces(s->a, !negated, slot);
    case S_AND: case S_OR: case S_IMPLIES: {
        int left = sentence_forces(s->a, s->kind == S_IMPLIES ? !negated : negated, slot);
        int right = sentence_forces(s->b, negated, slot);
        return (s->kind == S_AND) != negated ? left || right : left && right;
    }
    case S_FORALL: case S_EXISTS:
        return (s->kind == S_EXISTS) != negated && sentence_forces(s->a, negated, slot);
    case S_UNIQUE:
        return !negated && sentence_forces(s->a, 0, slot);
    default:
        return 0;
    }
}

/* lettera [ '_' cifra ]: legge il nome di una variabile, con il suo pedice,
 * e lo supera. Ritorna la lettera e mette il pedice in *index; ritorna 0,
 * senza leggere nulla, se il token corrente non è una lettera. */
static int abstract_letter(int *index) {
    *index = 0;
    return token.kind == T_LETTER ? abstract_name(index) : 0;
}

/* Quante condizioni di un insieme dato da una regola (17p) il lettore sta
 * leggendo una dentro l'altra. Lì una lettera può legare di nuovo una
 * variabile già in vista: \forall x, y\ ((x, y) \in \{(x, z) \mid \exists
 * y\ ...\}) è come i libri scrivono la composizione. Fuori no, come sempre
 * in 17o: la dimostrazione e il certificato scrivono le variabili con la loro
 * lettera, e due variabili con la stessa lettera una dentro l'altra
 * potrebbero confondersi. Dentro le graffe non succede, perché lo
 * svolgimento (17q) dà a ogni quantificatore di quella condizione una
 * lettera nuova. */
static int builder_depth;

/* Ritorna il posto di una variabile nuova, di lettera letter, pedice index
 * e insieme sort (-1 se lo imparerà dall'uso), senza metterla in vista; -1
 * dopo un errore, se le variabili sono troppe. */
static int slot_new(int letter, int index, int sort) {
    if (slot_count == MAX_SLOTS) { syntax("troppe variabili quantificate in un enunciato"); return -1; }
    slot_letter[slot_count] = letter;
    slot_index[slot_count] = index;
    slot_sort[slot_count] = sort;
    slot_pair[slot_count] = -1;
    return slot_count++;
}

/* Ritorna 1 se il nome letter, con il pedice index, ha già un significato
 * nel punto che il lettore sta leggendo: è un simbolo, o una variabile in
 * vista (vedi builder_depth). */
static int name_taken(int letter, int index) {
    return (!builder_depth && visible_find(letter, index) >= 0) || symbol_find(letter, index) >= 0;
}

/* Lega la lettera letter, con il pedice index, a una variabile nuova
 * dell'insieme sort (-1 se lo imparerà dall'uso), e la mette in vista.
 * Ritorna il suo posto, o -1 dopo un errore: una lettera che in quel punto
 * ha già un significato, o troppe variabili. */
static int slot_bind(int letter, int index, int sort) {
    if (name_taken(letter, index)) {
        syntax("la stessa lettera ha due significati nello stesso punto dell'enunciato");
        return -1;
    }
    int slot = slot_new(letter, index, sort);
    if (slot >= 0) visible[visible_count++] = slot;
    return slot;
}

/* 17p. Fa della variabile di posto slot, che si chiama x, senza pedice, la
 * prima componente di una coppia x \in S \times T: le componenti si
 * chiamano x_{1} e x_{2}, e la seconda varia nell'insieme second (-1 se lo
 * imparerà dall'uso). Ritorna il posto della seconda, o -1 dopo un errore:
 * x con un pedice, o un x_{1} o un x_{2} che ha già un significato. */
static int pair_make(int slot, int second) {
    int letter = slot_letter[slot];
    if (slot_index[slot] || slot_pair[slot] >= 0 || name_taken(letter, 1) || name_taken(letter, 2)) {
        syntax("una variabile che varia in un prodotto, x \\in S \\times T, ha per componenti x_{1} e x_{2}: x non deve avere un pedice, e x_{1} e x_{2} non devono avere già un significato");
        return -1;
    }
    int other = slot_new(letter, 2, second);
    if (other < 0) return -1;
    slot_index[slot] = 1;
    slot_pair[slot] = other;
    return other;
}

/* 17p. Se il membro items[] ha meno componenti di quante ne chiede un
 * insieme (arity), e le mancanti sono tante quante le sue variabili di cui
 * ancora non si sa l'insieme, quelle variabili sono coppie: \forall x, y\
 * ((x, y) \in R ...), con R una relazione tra coppie. Le trasforma, mette in
 * items[] il membro con tutte le componenti, e ritorna quante sono; se non è
 * così ritorna count, e items[] resta com'era.
 *
 * Il primo uso di una variabile senza insieme è sempre quello che glielo
 * dà, perché ogni altro uso lo chiede (elem_expect()): prima di qui x non è
 * stata letta come elemento da nessuna parte. */
static int tuple_widen(Elem **items, int count, int arity) {
    int unknown = 0, wide_count = 0;
    Elem *wide[MAX_ARITY];
    for (int i = 0; i < count; i++)
        unknown += items[i]->symbol < 0 && slot_sort[items[i]->slot] < 0 && slot_pair[items[i]->slot] < 0;
    if (!unknown || count + unknown != arity) return count;
    for (int i = 0; i < count; i++) {
        wide[wide_count++] = items[i];
        if (items[i]->symbol >= 0 || slot_sort[items[i]->slot] >= 0 || slot_pair[items[i]->slot] >= 0) continue;
        int second = pair_make(items[i]->slot, -1);
        if (second < 0) return count;
        wide[wide_count++] = elem_new(-1, second, NULL, NULL);
    }
    memcpy(items, wide, sizeof *items * (size_t)wide_count);
    return wide_count;
}

/* \in range: legge l'insieme in cui varia una lettera, dopo \in. È una
 * parte dichiarata (allora *part è il suo simbolo e *sort il suo insieme)
 * oppure un insieme qualsiasi (*part è -1). */
static void abstract_range(int *sort, int *part) {
    ParserState before = parser_save();
    const char *kept = consumed;
    int index, name = abstract_name(&index);
    *part = name ? symbol_find(name, index) : -1;
    if (*part >= 0 && symbols[*part].range == TRUTH && symbols[*part].arity == 1) {
        *sort = symbols[*part].domain[0];
        return;
    }
    *part = -1;
    parser_restore(before);
    consumed = kept;
    *sort = abstract_sort();
}

/* \mathcal{P}(Set): legge l'insieme delle parti di un insieme qualsiasi, il
 * cui token corrente è \mathcal, e ritorna il posto dell'insieme. */
static int abstract_power_sort(void) {
    next_token();
    expect_char('{', "dopo \\mathcal serve {P}");
    if (token.kind != T_LETTER || token.ch != 'P') syntax("l'unico \\mathcal riconosciuto è \\mathcal{P}, l'insieme delle parti");
    next_token();
    expect_char('}', "dopo \\mathcal serve {P}");
    if (token.kind == T_OPEN) next_token(); else syntax("dopo \\mathcal{P} serve l'insieme tra parentesi");
    int sort = abstract_sort();
    if (token.kind == T_CLOSE) next_token(); else syntax("manca la parentesi chiusa");
    return sort;
}

/* Il punto a cui il lettore torna quando un tentativo di lettura non
 * riesce: dove era nel testo, quante variabili e quanti insiemi conosceva,
 * e di ogni variabile l'insieme, il pedice e la seconda componente. Un
 * tentativo può aver legato variabili nuove (le lettere di un insieme dato
 * da una regola, 17p), aver incontrato un insieme nuovo, aver dato un
 * insieme a una variabile che non lo aveva, o averne fatto una coppia
 * (tuple_widen()). */
typedef struct {
    ParserState state;
    const char *consumed;
    int slots, sorts, visibles;
    int slot_sort[MAX_SLOTS], slot_index[MAX_SLOTS], slot_pair[MAX_SLOTS];
} ReaderMark;

/* Ritorna il punto in cui il lettore si trova. */
static ReaderMark reader_mark(void) {
    ReaderMark mark = {parser_save(), consumed, slot_count, sort_count, visible_count, {0}, {0}, {0}};
    memcpy(mark.slot_sort, slot_sort, sizeof mark.slot_sort);
    memcpy(mark.slot_index, slot_index, sizeof mark.slot_index);
    memcpy(mark.slot_pair, slot_pair, sizeof mark.slot_pair);
    return mark;
}

/* Riporta il lettore al punto mark, ottenuto da reader_mark(). */
static void reader_back(const ReaderMark *mark) {
    parser_restore(mark->state);
    consumed = mark->consumed;
    slot_count = mark->slots;
    sort_count = mark->sorts;
    visible_count = mark->visibles;
    memcpy(slot_sort, mark->slot_sort, sizeof slot_sort);
    memcpy(slot_index, mark->slot_index, sizeof slot_index);
    memcpy(slot_pair, mark->slot_pair, sizeof slot_pair);
}

/* 17p. LA NOTAZIONE DEI LIBRI: INSIEMI DI ELEMENTI E DI COPPIE
 *
 * I libri non scrivono "per ogni x, se x sta in X allora x sta in Y", ma
 * X \subseteq Y; non scrivono "t sta in f(X) se e solo se sta in f(Y), per
 * ogni t", ma f(X) = f(Y). Danno una relazione con una regola, R^{*} =
 * \{(y, x) \mid (x, y) \in R\}, ne compongono due come un insieme di coppie,
 * e ne confrontano due come insiemi. Questo paragrafo legge quella
 * notazione; il capitolo 28 non la vede, perché il paragrafo 17q la svolge
 * nelle sole condizioni di 17o, e il capitolo 28 (28h) ricontrolla lo
 * svolgimento calcolando sui modelli finiti tutti e due gli enunciati, quello
 * scritto e quello svolto.
 *
 * Gli insiemi scritti così (SetTerm, "termini d'insieme") sono insiemi di
 * elementi, cioè parti di un insieme qualsiasi, o di coppie, cioè relazioni;
 * i membri di una relazione tra coppie sono coppie di coppie. Un membro si
 * conta componente per componente: un elemento ne ha una, una coppia due,
 * una coppia di coppie quattro. La grammatica che si aggiunge a quella di
 * 17o:
 *
 *     unary     := ...
 *                | \exists! lettera [ \in range ] unary
 *                | Name '=' setterm                       (definizione)
 *                | name '_' '{' lettera '}' \subseteq Set   (famiglia, sotto \forall)
 *                | side (\in | \notin) setterm
 *                | setterm (= | \neq | \subseteq | \subsetneq | \supseteq | \supsetneq) setterm
 *     range     := ... | \subseteq Set | \in \mathcal{P}(Set)
 *     setterm   := setinter { (\cup | \setminus | \triangle) setinter }
 *     setinter  := setfactor { \cap setfactor }
 *     setfactor := '(' setterm ')' | \emptyset | Set
 *                | Name [ '^' '{' '*' '}' ]                (dichiarato, o definito)
 *                | name '_' '{' lettera '}'                (una parte di una famiglia)
 *                | name [ '^' '{' -1 '}' ] '(' setterm ')'   (immagine, controimmagine)
 *                | \{ term { ',' term } \}
 *                | \{ pattern [ \in product ] (\mid | ':') formula \}
 *                | (\bigcup | \bigcap) '_' '{' lettera \in range '}' setfactor
 *     pattern   := lettera [ '_' cifra ] | '(' pattern ',' pattern ')'
 *     product   := pfactor { \times pfactor },  pfactor := (Set | '(' product ')') [ '^' 2 ]
 *
 * Le convenzioni di lettura, che sono ciò che questo paragrafo decide:
 *
 *   - Una definizione G = \{...\}, tra le ipotesi, dà un nome a un insieme:
 *     ogni G scritto dopo è quell'insieme. Leggerla come un'ipotesi, "per
 *     ogni G, se G è quell'insieme allora P(G)", darebbe lo stesso
 *     enunciato, P(quell'insieme): l'abbreviazione è ciò che i libri
 *     intendono. È una definizione solo il nome nuovo, maiuscolo, di un
 *     insieme: R = \{...\} con R dichiarata è un'uguaglianza tra insiemi.
 *   - \{(x, y) \mid c\} è l'insieme delle coppie che soddisfano c. Le
 *     lettere dello schema sono legate qui. Il loro insieme, se non è
 *     scritto con \in A \times B, lo fissano le appartenenze di c, come per
 *     un quantificatore senza insieme (17m): c deve obbligare ogni lettera a
 *     stare in un insieme. Lo schema può ripetere una lettera: \{(x, x) \mid
 *     x \in S\} è la diagonale. x \in S, con S un insieme qualsiasi, dice
 *     solo che x è un elemento di S.
 *   - f(E), con E un insieme di elementi del dominio di f, è l'immagine: t
 *     sta in f(E) se t = f(x) per qualche x di E. f^{-1}(E) è la
 *     controimmagine: x vi sta se f(x) sta in E. f(S), con S il dominio, è
 *     l'immagine di tutto S.
 *   - \forall X \subseteq S, o \forall X \in \mathcal{P}(S), dove vale quanto
 *     una dichiarazione (gli stessi posti di \forall f \in T^{S}, 17o),
 *     dichiara una parte qualsiasi di S. Altrove non è letto: "per ogni
 *     parte" tra le ipotesi chiederebbe di scegliere delle parti, e il
 *     capitolo 28 sa scegliere solo elementi.
 *   - \forall i \in I\ (X_{i} \subseteq S) dichiara una famiglia di parti di
 *     S indicizzata da I. Per il capitolo 28 è una relazione tra S e I:
 *     x \in X_{i} vuol dire che x e i sono in relazione. \bigcup_{i \in I}
 *     E è l'insieme di ciò che sta in E per qualche i, \bigcap per ogni i.
 *   - \exists! x \in S\ P: esiste un x di S con P, e ogni z di S con P è x.
 *   - Due insiemi sono uguali quando hanno gli stessi membri. E \subsetneq F
 *     vuol dire E \subseteq F e qualche membro di F che non sta in E.
 *
 * Ogni termine d'insieme ha un'arità e un insieme per ogni componente, e il
 * lettore controlla che due insiemi confrontati, o un insieme e il membro
 * che gli si attribuisce, li abbiano uguali. L'insieme vuoto prende i suoi
 * da ciò con cui è confrontato.
 */

/* Tipi di termine d'insieme.
 *
 * Warning: se aggiungi un tipo, aggiungi il ramo che lo legge in
 * abstract_set_factor(), il suo svolgimento in unfold_member() (17q) e il
 * suo calcolo in book_member() (28h). */
typedef enum {
    ST_SYMBOL,      /* una parte o una relazione dichiarata: symbol */
    ST_SECTION,     /* X_{i}: ciò che la famiglia symbol mette in relazione con
                       l'indice item[0] */
    ST_SORT,        /* un insieme qualsiasi, tutto: sort[0] */
    ST_EMPTY,       /* l'insieme vuoto */
    ST_LIST,        /* \{item[0], ..., item[items - 1]\} */
    ST_BUILDER,     /* \{schema | condition\}: lo schema è item[], una variabile
                       per componente; le variabili, senza ripetizioni, bound[] */
    ST_IMAGE,       /* l'immagine di a attraverso l'applicazione symbol */
    ST_PREIMAGE,    /* la controimmagine di a */
    ST_UNION, ST_INTER, ST_DIFF, ST_SYMDIFF,  /* a op b */
    ST_BIG_UNION,   /* l'unione degli a al variare dell'indice, la variabile
                       bound[0]; se symbol >= 0 l'indice varia nella parte symbol */
    ST_BIG_INTER    /* l'intersezione, allo stesso modo */
} SetTermKind;

/* Un termine d'insieme (17p): un insieme di elementi o di coppie scritto
 * come nei libri. I suoi membri hanno `arity` componenti, la componente i
 * nell'insieme sort[i] (un posto in sort_name[]). arity è 0 solo per un
 * insieme vuoto che non è ancora stato confrontato con nulla. I campi non
 * usati dal tipo sono zero, symbol è -1. I nodi vivono nell'arena e non
 * cambiano dopo la lettura: un termine definito si usa dove serve senza
 * copiarlo. */
struct SetTerm {
    SetTermKind kind;
    int arity;
    int sort[MAX_ARITY];
    int symbol;             /* vedi i tipi; -1 dove non serve */
    SetTerm *a, *b;         /* gli operandi */
    Elem *item[MAX_ARITY];  /* ST_LIST: gli elementi. ST_SECTION: l'indice.
                               ST_BUILDER: lo schema */
    int items;
    int bound[MAX_ARITY];   /* ST_BUILDER: le variabili dello schema.
                               ST_BIG_*: l'indice, in bound[0] */
    int bounds;
    Sentence *condition;    /* ST_BUILDER: la condizione */
};

/* Il posto dell'indice dell'ultima famiglia di parti dichiarata
 * (\forall i \in I\ (X_{i} \subseteq S)), o -1: serve al quantificatore di
 * i per riconoscere che contiene solo la dichiarazione. */
static int family_slot;

/* Ritorna un nuovo termine d'insieme di tipo kind, senza membri finché chi
 * lo costruisce non gli dà arità e insiemi. Come elem_new(), se l'arena è
 * piena ritorna un nodo di riserva. */
static SetTerm *set_term_new(SetTermKind kind) {
    static SetTerm spare;
    SetTerm *t = arena_alloc(sizeof(SetTerm));
    if (!t) t = &spare;
    memset(t, 0, sizeof *t);
    t->kind = kind;
    t->symbol = -1;
    return t;
}

/* Dà a t l'arità e gli insiemi di model. */
static void set_term_shape(SetTerm *t, const SetTerm *model) {
    t->arity = model->arity;
    memcpy(t->sort, model->sort, sizeof t->sort);
}

/* Controlla che a e b abbiano membri dello stesso tipo: la stessa arità e
 * gli stessi insiemi, componente per componente. Un insieme vuoto non
 * ancora confrontato prende quelli dell'altro. */
static void set_terms_match(SetTerm *a, SetTerm *b) {
    if (!a->arity) set_term_shape(a, b);
    if (!b->arity) set_term_shape(b, a);
    if (a->arity != b->arity || memcmp(a->sort, b->sort, sizeof a->sort) != 0)
        syntax("due insiemi confrontati, o messi insieme, hanno membri di tipo diverso: elementi di insiemi diversi, o elementi e coppie");
}

/* Se il token corrente è un apice, lo legge: ritorna 1 per l'asterisco,
 * ^{*}, e -1 per ^{-1}. Ritorna 0, senza leggere nulla, se non c'è un apice
 * o se è un altro. */
static int abstract_superscript(void) {
    if (!is_char('^')) return 0;
    ParserState before = parser_save();
    const char *kept = consumed;
    next_token();
    int braces = is_char('{'), mark = 0;
    if (braces) next_token();
    if (is_char('*')) { mark = 1; next_token(); }
    else if (is_char('-')) {
        next_token();
        if (token.kind == T_NUM && token.value == 1) { mark = -1; next_token(); }
    }
    if (mark && braces && is_char('}')) next_token();
    else if (braces) mark = 0;
    if (!mark) { parser_restore(before); consumed = kept; }
    return mark;
}

static SetTerm *abstract_set_term(void);

/* pattern: legge lo schema di un insieme dato da una regola, da count in
 * poi, e mette le sue lettere, una per componente, in letter[] e i pedici in
 * index[]. Ritorna quante componenti ha in tutto, o 0 se ciò che segue non è
 * uno schema: allora chi chiama rimette il lettore dov'era. */
static int abstract_pattern(int *letter, int *index, int count) {
    if (token.kind == T_LETTER) {
        if (count == MAX_ARITY) return 0;
        letter[count] = abstract_letter(&index[count]);
        return count + 1;
    }
    if (token.kind != T_OPEN) return 0;
    next_token();
    count = abstract_pattern(letter, index, count);
    if (!count || !is_char(',')) return 0;
    next_token();
    count = abstract_pattern(letter, index, count);
    if (!count || token.kind != T_CLOSE) return 0;
    next_token();
    return count;
}

/* product: legge un prodotto di insiemi, S \times T, S^{2}, (S \times
 * T)^{2}, e mette in sort[], da count in poi, gli insiemi delle sue
 * componenti, appiattite. Ritorna quante sono in tutto. */
static int abstract_product(int *sort, int count) {
    for (;;) {
        int first = count;
        if (token.kind == T_OPEN) {
            next_token();
            count = abstract_product(sort, count);
            if (token.kind == T_CLOSE) next_token(); else syntax("manca la parentesi chiusa");
        } else if (count < MAX_ARITY) {
            sort[count++] = abstract_sort();
        } else {
            syntax("troppe componenti: al massimo quattro, una coppia di coppie");
            return count;
        }

        /* Il quadrato: le stesse componenti, due volte. Un'altra lettera
         * all'esponente, T^{S}, è l'insieme delle applicazioni da S a T, che
         * come insieme di oggetti di una relazione non è trattato. */
        if (is_char('^')) {
            next_token();
            int braces = is_char('{');
            if (braces) next_token();
            if (token.kind == T_LETTER) syntax("un insieme di applicazioni, T^{S}, come insieme di oggetti di una relazione non è trattato: le applicazioni si dichiarano con f \\colon S \\to T");
            if (token.kind != T_NUM || token.value != 2) syntax("di un insieme è riconosciuto solo il quadrato, A^{2}");
            next_token();
            if (braces) expect_char('}', "manca }");
            for (int i = first, end = count; i < end && !syntax_error; i++) {
                if (count == MAX_ARITY) { syntax("troppe componenti: al massimo quattro, una coppia di coppie"); break; }
                sort[count++] = sort[i];
            }
        }
        if (!is_command("times") || syntax_error) return count;
        next_token();
    }
}

/* Il resto di \{ pattern [\in product] (\mid | :) formula \}, dopo lo
 * schema, che ha `count` lettere in letter[] e index[]. Ritorna l'insieme. */
static SetTerm *abstract_builder(const int *letter, const int *index, int count) {
    SetTerm *t = set_term_new(ST_BUILDER);
    int sort[MAX_ARITY], sorts = 0;
    if (is_command("in")) {
        next_token();
        sorts = abstract_product(sort, 0);
        if (sorts != count) syntax("lo schema di un insieme dato da una regola e l'insieme in cui varia hanno un numero diverso di componenti");
    }
    if (is_command("mid") || is_char(':')) next_token(); else syntax("in \\{(x, y) \\mid ...\\} manca \\mid");

    /* Le lettere dello schema, in vista mentre si legge la condizione: una
     * variabile per lettera, anche quando la lettera si ripete. Sono legate
     * qui, come in ogni scrittura \{x \mid ...\}: una lettera già in vista
     * fuori, come la x di \forall x\ ((x, x) \in \{(x, z) \mid ...\}), dentro
     * le graffe è un'altra variabile. Non c'è pericolo di confonderle: lo
     * svolgimento (17q) mette al posto delle lettere dello schema i membri,
     * e nell'enunciato svolto non restano. */
    int outer = visible_count, positive = abstract_positive;
    for (int i = 0; i < count && !syntax_error; i++) {
        int slot = -1;
        for (int j = 0; j < t->bounds && slot < 0; j++)
            if (slot_letter[t->bound[j]] == letter[i] && slot_index[t->bound[j]] == index[i]) slot = t->bound[j];
        if (slot < 0) {
            slot = slot_new(letter[i], index[i], sorts ? sort[i] : -1);
            if (slot < 0) break;
            visible[visible_count++] = slot;
            t->bound[t->bounds++] = slot;
        } else if (sorts && slot_sort[slot] != sort[i]) {
            syntax("una lettera ripetuta nello schema deve stare ogni volta nello stesso insieme");
        }
        t->item[t->items++] = elem_new(-1, slot, NULL, NULL);
    }
    t->arity = count;
    abstract_positive = 0;
    builder_depth++;
    t->condition = abstract_formula();
    builder_depth--;
    abstract_positive = positive;
    visible_count = outer;
    expect_char(ESCAPED + '}', "manca \\}");
    if (syntax_error) return t;

    /* Senza un insieme scritto, quello di ogni lettera lo fissa la
     * condizione, che deve obbligarla a starci (17m). Una lettera dello
     * schema non diventa una coppia: lo schema ne scrive le componenti. */
    for (int i = 0; i < t->bounds && !syntax_error; i++) {
        if (slot_pair[t->bound[i]] >= 0)
            syntax("in \\{(x, y) \\mid ...\\} una lettera dello schema è un elemento: una coppia si scrive con le sue componenti, ((x_{1}, x_{2}), y)");
        else if (slot_sort[t->bound[i]] < 0 || (!sorts && !sentence_forces(t->condition, 0, t->bound[i])))
            syntax("in \\{(x, y) \\mid ...\\} senza un insieme scritto la condizione deve obbligare ogni lettera dello schema a stare in un insieme, con un'appartenenza");
    }
    for (int i = 0; i < count && !syntax_error; i++) t->sort[i] = slot_sort[t->item[i]->slot];
    return t;
}

/* \{ ... \}, dopo la graffa aperta: un insieme dato da una regola, o un
 * elenco di elementi. Si distinguono dal primo token dopo lo schema. */
static SetTerm *abstract_braces(void) {
    /* Uno schema fatto di lettere che non sono simboli, seguito da \in,
     * \mid o due punti, è quello di una regola. Altrimenti si torna
     * indietro, ed è un elenco: \{a\}, \{x, y\}. */
    ParserState before = parser_save();
    const char *kept = consumed;
    int letter[MAX_ARITY], index[MAX_ARITY];
    int count = abstract_pattern(letter, index, 0), fresh = count > 0 && !syntax_error;
    for (int i = 0; i < count && fresh; i++) fresh = symbol_find(letter[i], index[i]) < 0;
    if (fresh && (is_command("in") || is_command("mid") || is_char(':'))) return abstract_builder(letter, index, count);
    parser_restore(before);
    consumed = kept;

    /* Un elenco: elementi dello stesso insieme. */
    SetTerm *t = set_term_new(ST_LIST);
    for (;;) {
        if (t->items == MAX_ARITY) { syntax("troppi elementi tra graffe: al massimo quattro"); break; }
        t->item[t->items++] = abstract_term();
        if (syntax_error || !is_char(',')) break;
        next_token();
    }
    expect_char(ESCAPED + '}', "manca \\}");
    t->arity = 1;
    t->sort[0] = elem_sort(t->item[0]);
    if (t->sort[0] < 0 && !syntax_error) syntax("non si sa in quale insieme stiano gli elementi scritti tra graffe");
    for (int i = 1; i < t->items && !syntax_error; i++) elem_expect(t->item[i], t->sort[0]);
    return t;
}

static SetTerm *abstract_set_factor(void);

/* (\bigcup | \bigcap) '_' '{' lettera \in range '}' setfactor, con il token
 * corrente su \bigcup o \bigcap. */
static SetTerm *abstract_big(void) {
    SetTerm *t = set_term_new(is_command("bigcup") ? ST_BIG_UNION : ST_BIG_INTER);
    next_token();
    expect_char('_', "dopo \\bigcup e \\bigcap serve l'indice, come in \\bigcup_{i \\in I}");
    expect_char('{', "dopo \\bigcup e \\bigcap serve l'indice, come in \\bigcup_{i \\in I}");
    int index, sort = -1, letter = abstract_letter(&index);
    if (letter && is_command("in")) { next_token(); abstract_range(&sort, &t->symbol); }
    else syntax("dopo \\bigcup e \\bigcap serve l'indice, come in \\bigcup_{i \\in I}");
    expect_char('}', "manca }");
    if (syntax_error) return t;

    /* L'insieme di cui si fa l'unione, con l'indice in vista. */
    int outer = visible_count;
    t->bound[0] = slot_bind(letter, index, sort);
    t->bounds = 1;
    if (t->bound[0] < 0) return t;
    t->a = abstract_set_factor();
    visible_count = outer;
    set_term_shape(t, t->a);
    if (!t->arity && !syntax_error) syntax("non si sa di che cosa sia l'unione o l'intersezione di insiemi vuoti");
    return t;
}

/* setfactor: vedi la grammatica in testa al paragrafo. */
static SetTerm *abstract_set_factor(void) {
    if (token.kind == T_OPEN) {
        next_token();
        SetTerm *inner = abstract_set_term();
        if (token.kind == T_CLOSE) next_token(); else syntax("manca la parentesi chiusa");
        return inner;
    }
    if (is_command("emptyset") || is_command("varnothing")) {
        next_token();
        return set_term_new(ST_EMPTY);
    }
    if (is_char(ESCAPED + '{')) {
        next_token();
        return abstract_braces();
    }
    if (is_command("bigcup") || is_command("bigcap")) return abstract_big();

    /* Un nome, con quello che lo segue. */
    int index, letter, name = abstract_name_or_section(&index, &letter);
    if (!name) {
        syntax("serve un insieme: una parte o una relazione, un insieme dato da una regola, un'immagine f(X), o un insieme qualsiasi");
        return set_term_new(ST_EMPTY);
    }

    /* Una parte di una famiglia: X_{i}, con i l'indice. */
    if (letter) {
        SetTerm *t = set_term_new(ST_SECTION);
        int slot = visible_find(letter, 0);
        t->symbol = symbol_find(name, 0);
        if (t->symbol < 0 || !symbols[t->symbol].family || slot < 0) {
            syntax("X_{i}, con una lettera per pedice, è la parte di indice i di una famiglia dichiarata con \\forall i \\in I\\ (X_{i} \\subseteq S)");
            return t;
        }
        t->item[0] = elem_new(-1, slot, NULL, NULL);
        t->items = 1;
        elem_expect(t->item[0], symbols[t->symbol].domain[1]);
        t->arity = 1;
        t->sort[0] = symbols[t->symbol].domain[0];
        return t;
    }

    /* Un nome definito dall'enunciato. */
    int mark = abstract_superscript();
    const Definition *defined = definition_find(name, index, mark == 1);
    if (defined) return defined->term;

    /* Un'applicazione: l'immagine, o con ^{-1} la controimmagine. */
    int symbol = symbol_find(name, index);
    if (symbol >= 0 && symbols[symbol].range != TRUTH && mark != 1) {
        const Symbol *f = &symbols[symbol];
        SetTerm *t = set_term_new(mark ? ST_PREIMAGE : ST_IMAGE), want = {0};
        if (f->arity != 1) syntax("immagini e controimmagini sono lette solo per le applicazioni di un argomento");
        if (token.kind == T_OPEN) next_token(); else syntax("un'applicazione, dove serve un insieme, va scritta con l'insieme tra parentesi: f(X), f^{-1}(Y)");
        t->symbol = symbol;
        t->a = abstract_set_term();
        if (token.kind == T_CLOSE) next_token(); else syntax("manca la parentesi chiusa");
        want.arity = t->arity = 1;
        want.sort[0] = mark ? f->range : f->domain[0];
        t->sort[0] = mark ? f->domain[0] : f->range;
        if (!syntax_error) set_terms_match(t->a, &want);
        return t;
    }
    if (mark) {
        syntax(mark == 1 ? "un nome con l'asterisco va definito prima di usarlo: R^{*} = \\{...\\}" : "^{-1} è letto solo dopo un'applicazione, per la controimmagine");
        return set_term_new(ST_EMPTY);
    }

    /* Una parte o una relazione dichiarata. */
    if (symbol >= 0) {
        SetTerm *t = set_term_new(ST_SYMBOL);
        t->symbol = symbol;
        t->arity = symbols[symbol].arity;
        for (int i = 0; i < t->arity; i++) t->sort[i] = symbols[symbol].domain[i];
        if (!t->arity) syntax("un elemento non è un insieme: per un insieme di un solo elemento si scrive \\{a\\}");
        return t;
    }

    /* Un insieme qualsiasi, tutto. */
    SetTerm *t = set_term_new(ST_SORT);
    t->arity = 1;
    t->sort[0] = index ? 0 : sort_index(name);
    if (index) syntax("una lettera non è né un insieme né dichiarata come parte o relazione");
    return t;
}

/* setinter := setfactor { \cap setfactor } */
static SetTerm *abstract_set_inter(void) {
    SetTerm *t = abstract_set_factor();
    while (is_command("cap") && !syntax_error) {
        next_token();
        SetTerm *node = set_term_new(ST_INTER);
        node->a = t;
        node->b = abstract_set_factor();
        set_terms_match(node->a, node->b);
        set_term_shape(node, node->a);
        t = node;
    }
    return t;
}

/* setterm := setinter { (\cup | \setminus | \triangle) setinter } */
static SetTerm *abstract_set_term(void) {
    SetTerm *t = abstract_set_inter();
    for (;;) {
        SetTermKind kind;
        if (is_command("cup")) kind = ST_UNION;
        else if (is_command("setminus")) kind = ST_DIFF;
        else if (is_command("triangle")) kind = ST_SYMDIFF;
        else return t;
        if (syntax_error) return t;
        next_token();
        SetTerm *node = set_term_new(kind);
        node->a = t;
        node->b = abstract_set_inter();
        set_terms_match(node->a, node->b);
        set_term_shape(node, node->a);
        t = node;
    }
}

/* Prova a leggere un confronto tra due insiemi scritti come nei libri
 * (17p): setterm (= | \neq | \subseteq | \subsetneq | \supseteq |
 * \supsetneq) setterm. Ritorna la condizione, o NULL se ciò che segue non ha
 * questa forma: allora il lettore va rimesso dov'era da chi chiama. *seen
 * diventa 1 se il primo insieme si è letto e la relazione c'era: un errore
 * dopo quel punto è di un confronto tra insiemi scritto male. */
static Sentence *abstract_set_relation(const char *start, int *seen) {
    SetTerm *left = abstract_set_term();
    if (syntax_error) return NULL;
    int kind = S_INCLUDED, proper = 0, swap = 0, negate = 0;
    if (is_char('=')) kind = S_SAME;
    else if (is_command("neq") || is_command("ne")) { kind = S_SAME; negate = 1; }
    else if (is_command("subsetneq")) proper = 1;
    else if (is_command("supseteq")) swap = 1;
    else if (is_command("supsetneq")) swap = proper = 1;
    else if (!is_command("subseteq")) return NULL;
    *seen = 1;
    next_token();
    SetTerm *right = abstract_set_term();
    if (syntax_error) return NULL;
    set_terms_match(left, right);
    if (!left->arity && !syntax_error) syntax("non si sa di che cosa siano i due insiemi vuoti confrontati");
    Sentence *s = sentence_new(kind, NULL, NULL, start);
    s->set = swap ? right : left;
    s->other = swap ? left : right;
    s->proper = proper;
    return negate ? sentence_new(S_NOT, s, NULL, start) : s;
}

/* Prova a leggere un confronto o un'appartenenza:
 *
 *     side ('=' | \neq) side        side (\in | \notin) setterm
 *
 * Ritorna la condizione, o NULL se ciò che segue non ha questa forma: allora
 * il lettore va rimesso dov'era da chi chiama, che decide che altro può
 * essere. start è l'inizio del testo. */
static Sentence *abstract_atom(const char *start) {
    Elem *left[MAX_ARITY], *right[MAX_ARITY];
    int count = abstract_tuple(left, 0);
    if (!count) return NULL;

    /* Appartenenza: a una parte o a una relazione dichiarata, come in 17o,
     * o a un insieme scritto come nei libri (17p). Un insieme vuoto prende i
     * suoi insiemi dal membro; le variabili del membro di cui non si sa
     * ancora l'insieme possono essere coppie (tuple_widen()). */
    if (is_command("in") || is_command("notin")) {
        int negative = is_command("notin");
        next_token();
        SetTerm *set = abstract_set_term();
        if (syntax_error) return NULL;
        if (!set->arity) {
            set->arity = count;
            for (int i = 0; i < count; i++) set->sort[i] = elem_sort(left[i]);
        }
        count = tuple_widen(left, count, set->arity);
        if (set->arity != count) {
            syntax("un elemento appartiene a un insieme di elementi, una coppia a un insieme di coppie");
            return NULL;
        }
        for (int i = 0; i < count; i++)
            if (set->sort[i] >= 0) elem_expect(left[i], set->sort[i]);
        Sentence *member = sentence_new(set->kind == ST_SYMBOL ? S_HOLDS : S_MEMBER, NULL, NULL, start);
        if (set->kind == ST_SYMBOL) {
            member->left = elem_new(set->symbol, 0, left[0], count == 2 ? left[1] : NULL);
        } else {
            member->set = set;
            member->items = count;
            for (int i = 0; i < count; i++) member->item[i] = left[i];
        }
        return negative ? sentence_new(S_NOT, member, NULL, start) : member;
    }

    /* Uguaglianza, tra elementi o tra coppie: componente per componente. */
    int distinct = is_command("neq") || is_command("ne");
    if (!distinct && !is_char('=')) return NULL;
    next_token();
    if (abstract_tuple(right, 0) != count) {
        syntax("i due membri di un'uguaglianza devono essere due elementi o due coppie");
        return NULL;
    }
    Sentence *all = NULL;
    for (int i = 0; i < count; i++) {
        /* I due membri stanno nello stesso insieme; se di uno solo si sa
         * quale, l'altro lo impara. */
        if (elem_sort(left[i]) < 0 && elem_sort(right[i]) < 0) syntax("non si sa in quale insieme stiano i due membri di un'uguaglianza");
        else if (elem_sort(left[i]) < 0) elem_expect(left[i], elem_sort(right[i]));
        else elem_expect(right[i], elem_sort(left[i]));
        Sentence *equal = sentence_new(S_EQUAL, NULL, NULL, start);
        equal->left = left[i];
        equal->right = right[i];
        all = all ? sentence_new(S_AND, all, equal, start) : equal;
    }
    return distinct ? sentence_new(S_NOT, all, NULL, start) : all;
}

/* Prova a leggere una dichiarazione: di un'applicazione o di un'operazione,
 * di una parte o di una relazione, di un elemento con un nome, o "S non è
 * vuoto". Registra ciò che dichiara e ritorna il suo nodo; ritorna NULL,
 * con il lettore dov'era, se ciò che segue non è una dichiarazione. */
static Sentence *abstract_declaration(const char *start) {
    ParserState before = parser_save();
    const char *kept = consumed;
    Symbol symbol = {0, 0, 0, {0, 0}, 0, 0, 0};
    int section = 0, starred = 0;
    symbol.name = operation_name();
    int operation = symbol.name != 0;
    if (operation) next_token();
    else symbol.name = abstract_name_or_section(&symbol.index, &section);
    if (symbol.name && !operation && !section) starred = abstract_superscript() == 1;

    /* Un nome nuovo: non è già un simbolo, una definizione, una variabile in
     * vista o un insieme. Solo un nome nuovo si dichiara o si definisce; X
     * \subseteq Y, con X già dichiarata, è un'inclusione (17p). Un nome con
     * l'asterisco può essere solo una definizione, e R^{*} è un nome diverso
     * da R. */
    int fresh = symbol.name && !definition_find(symbol.name, symbol.index, starred) &&
                (starred || (symbol_find(symbol.name, symbol.index) < 0 && visible_find(symbol.name, symbol.index) < 0));
    for (int i = 0; i < sort_count && fresh && !starred; i++) fresh = symbol.index || sort_name[i] != symbol.name;

    /* 17p. X_{i} \subseteq S, sotto \forall i \in I: una famiglia di parti di
     * S, che per il capitolo 28 è una relazione tra S e I. */
    if (section && fresh && is_command("subseteq")) {
        int slot = visible_find(section, 0);
        if (slot < 0 || slot_sort[slot] < 0) {
            syntax("X_{i} \\subseteq S dichiara una famiglia di parti solo dentro \\forall i \\in I");
            return sentence_new(S_TRUE, NULL, NULL, start);
        }
        next_token();
        symbol.domain[0] = abstract_sort();
        symbol.domain[1] = slot_sort[slot];
        symbol.arity = 2;
        symbol.range = TRUTH;
        symbol.family = 1;
        symbol_add(symbol);
        family_slot = slot;
        return sentence_new(S_TRUE, NULL, NULL, start);
    }

    /* 17p. G = \{...\}: un nome per un insieme. Il nome è quello di un
     * insieme, maiuscolo. */
    if (fresh && !section && !operation && symbol.name < 128 && isupper(symbol.name) && is_char('=')) {
        next_token();
        SetTerm *term = abstract_set_term();
        if (!syntax_error && !term->arity) syntax("una definizione G = \\emptyset non dice di che cosa sia G un insieme");
        if (definition_count == MAX_DEFINITIONS) syntax("troppi insiemi definiti in un enunciato");
        if (!syntax_error) definitions[definition_count++] = (Definition){symbol.name, symbol.index, starred, term};
        return sentence_new(S_TRUE, NULL, NULL, start);
    }
    if (section || starred) {
        parser_restore(before);
        consumed = kept;
        return NULL;
    }

    /* f \colon S \to T, \perp \colon S \times T \to V. A valori in un
     * prodotto, f \colon S \to T \times V (17p), sono due applicazioni, le
     * componenti f_{1} e f_{2}, dichiarate una dopo l'altra. */
    if (symbol.name && is_command("colon")) {
        next_token();
        symbol.domain[0] = abstract_sort();
        symbol.arity = 1;
        if (is_command("times")) { next_token(); symbol.domain[1] = abstract_sort(); symbol.arity = 2; }
        if (is_command("to") || is_command("rightarrow") || is_command("longrightarrow")) next_token();
        else syntax("in una dichiarazione f \\colon A \\to B manca \\to");
        symbol.range = abstract_sort();
        if (operation && symbol.arity != 2) syntax("un'operazione si dichiara su un insieme di coppie, A \\times B");
        if (is_command("times") && !syntax_error) {
            if (operation || symbol.index || name_taken(symbol.name, 1) || name_taken(symbol.name, 2))
                syntax("un'applicazione f a valori in un prodotto ha per componenti f_{1} e f_{2}: f non deve avere un pedice, e f_{1} e f_{2} non devono avere già un significato");
            next_token();
            Symbol second = symbol;
            second.range = abstract_sort();
            symbol.index = symbol.paired = 1;
            second.index = second.paired = 2;
            symbol_add(symbol);
            symbol_add(second);
            return sentence_new(S_TRUE, NULL, NULL, start);
        }
        symbol_add(symbol);
        return sentence_new(S_TRUE, NULL, NULL, start);
    }

    /* X \subseteq S, R \subseteq S \times T. */
    if (fresh && !operation && is_command("subseteq")) {
        next_token();
        symbol.domain[0] = abstract_sort();
        symbol.arity = 1;
        if (is_command("times")) { next_token(); symbol.domain[1] = abstract_sort(); symbol.arity = 2; }
        symbol.range = TRUTH;
        symbol_add(symbol);
        return sentence_new(S_TRUE, NULL, NULL, start);
    }

    /* S \neq \emptyset, o X \neq \emptyset con X una parte dichiarata:
     * "esiste un elemento di S" (che sta in X), scritto con una variabile
     * che prende la minuscola della lettera. Per un insieme la condizione è
     * z = z, sempre vera. Così il motore non ha un caso in più: ne ricava
     * un elemento come da ogni altro "esiste". */
    int plain = symbol.name && !operation && !symbol.index && symbol.name < 128;
    if (plain && isupper(symbol.name) && (is_command("neq") || is_command("ne"))) {
        next_token();
        if (is_command("emptyset") || is_command("varnothing")) {
            next_token();
            int part = symbol_find(symbol.name, 0);
            if (part >= 0 && (symbols[part].range != TRUTH || symbols[part].arity != 1)) part = -1;
            int sort = part >= 0 ? symbols[part].domain[0] : sort_index(symbol.name);
            int slot = slot_new(tolower(symbol.name), 0, sort);
            if (slot < 0) return sentence_new(S_TRUE, NULL, NULL, start);
            Elem *some = elem_new(-1, slot, NULL, NULL);
            Sentence *body = sentence_new(part >= 0 ? S_HOLDS : S_EQUAL, NULL, NULL, start);
            body->left = part >= 0 ? elem_new(part, 0, some, NULL) : some;
            body->right = part >= 0 ? NULL : some;
            Sentence *exists = sentence_new(S_EXISTS, body, NULL, start);
            exists->slot = slot;
            return exists;
        }
    }

    /* u \in S, con u una lettera che non vuol dire ancora nulla: un
     * elemento di S a cui l'enunciato dà un nome. */
    if (plain && islower(symbol.name) && is_command("in") && fresh) {
        next_token();
        symbol.range = abstract_sort();
        symbol_add(symbol);
        return sentence_new(S_TRUE, NULL, NULL, start);
    }
    parser_restore(before);
    consumed = kept;
    return NULL;
}

/* Legge un quantificatore, di cui il token corrente è \forall o \exists:
 * le lettere, il loro insieme, la condizione. Vedi le convenzioni in testa
 * al paragrafo, e in 17p, per le forme dell'insieme. */
static Sentence *abstract_quantifier(const char *start) {
    SentenceKind kind = is_command("forall") ? S_FORALL : S_EXISTS;
    int letter[MAX_BOUND], index[MAX_BOUND], letters = 0;
    do {
        next_token();  /* supera il quantificatore, poi ogni virgola */
        int read = letters < MAX_BOUND ? abstract_letter(&index[letters]) : 0;
        if (!read) {
            syntax(is_char('{') ? "una relazione con un nome tra graffe, {\\le}, scritta tra gli elementi come x \\le y, non è trattata: si scrive (x, y) \\in R"
                                : "dopo un quantificatore servono delle lettere separate da virgole, non più di 8");
            break;
        }
        letter[letters++] = read;
    } while (is_char(','));

    /* Un'applicazione o un'operazione da cercare: in testa all'enunciato è
     * la richiesta di un esempio, che legge abstract_statement(); qui no. */
    if (is_command("colon"))
        syntax("un'applicazione da costruire, \\exists f \\colon S \\to T dentro l'enunciato, non è trattata: il programma cerca degli elementi, o un esempio intero in testa all'enunciato");

    /* L'insieme: una parte dichiarata (allora part è il suo simbolo), un
     * insieme qualsiasi, oppure nessuno. Dopo un insieme, ^ e un altro
     * insieme dicono che le lettere sono applicazioni; \subseteq S e \in
     * \mathcal{P}(S), che sono parti di S (17p). */
    int sort = -1, part = -1, maps = 0, parts = 0, domain = 0, second = -1;
    if (is_command("subseteq")) {
        next_token();
        sort = abstract_sort();
        parts = 1;
    } else if (is_command("in")) {
        next_token();
        if (is_command("mathcal")) {
            sort = abstract_power_sort();
            parts = 1;
        } else {
            abstract_range(&sort, &part);
        }
        if (part < 0 && !parts && is_char('^')) {
            next_token();
            int braces = is_char('{');
            if (braces) next_token();
            domain = abstract_sort();
            if (braces) expect_char('}', "manca }");
            maps = 1;
        }

        /* 17p. Un prodotto, S \times T: le lettere sono coppie. */
        if (part < 0 && !parts && !maps && is_command("times")) {
            next_token();
            second = abstract_sort();
        }
    }

    /* "Per ogni applicazione" e "per ogni parte": una dichiarazione, e poi
     * la condizione. */
    if (maps || parts) {
        if (kind != S_FORALL || !abstract_positive)
            syntax(maps ? "\\forall f \\in T^{S} è letto solo in testa alla tesi, dove vale quanto dichiarare f; un'applicazione da cercare non è trattata"
                        : "\\forall X \\subseteq S è letto solo in testa alla tesi, dove vale quanto dichiarare X; una parte da cercare, o una ipotesi su tutte le parti, non è trattata");
        for (int i = 0; i < letters && !syntax_error; i++)
            symbol_add(maps ? (Symbol){letter[i], index[i], 1, {domain, 0}, sort, 0, 0} : (Symbol){letter[i], index[i], 1, {sort, 0}, TRUTH, 0, 0});
        map_quantifiers++;
        return abstract_unary();
    }

    /* Le variabili, in vista mentre si legge la condizione. Una coppia ha
     * due posti, le sue componenti (17p). */
    int slot[MAX_BOUND], outer = visible_count, positive = abstract_positive;
    for (int i = 0; i < letters && !syntax_error; i++) {
        slot[i] = slot_bind(letter[i], index[i], sort);
        if (second >= 0 && slot[i] >= 0) pair_make(slot[i], second);
    }
    if (kind == S_EXISTS) abstract_positive = 0;
    family_slot = -1;
    Sentence *node = abstract_unary();
    abstract_positive = positive;
    visible_count = outer;
    if (syntax_error) return node;

    /* 17p. \forall i \in I\ (X_{i} \subseteq S) contiene solo la
     * dichiarazione di una famiglia: è la dichiarazione. */
    if (kind == S_FORALL && letters == 1 && node->kind == S_TRUE && family_slot == slot[0]) return node;

    /* Senza insieme: ogni lettera deve averlo imparato dall'uso, e la
     * condizione deve obbligarla a stare lì (per un "per ogni" deve farlo
     * la sua negazione, come in 17m). Una coppia lo impara per tutte e due
     * le componenti insieme, da un'appartenenza (tuple_widen()). */
    for (int i = 0; i < letters && sort < 0; i++)
        if (slot_sort[slot[i]] < 0 || (slot_pair[slot[i]] >= 0 && slot_sort[slot_pair[slot[i]]] < 0) ||
            !sentence_forces(node, kind == S_FORALL, slot[i]))
            syntax("un quantificatore senza insieme è ammesso solo se la condizione obbliga la lettera a stare in un insieme, con un'appartenenza a una parte o a una relazione dichiarata");

    /* \in X, con X una parte: la forma svolta. Poi un nodo per lettera,
     * dall'ultima alla prima; per una coppia due, la prima componente fuori. */
    for (int i = letters - 1; i >= 0 && part >= 0; i--) {
        Sentence *member = sentence_new(S_HOLDS, NULL, NULL, start);
        member->left = elem_new(part, 0, elem_new(-1, slot[i], NULL, NULL), NULL);
        node = sentence_new(kind == S_FORALL ? S_IMPLIES : S_AND, member, node, start);
    }
    for (int i = letters - 1; i >= 0; i--) {
        if (slot_pair[slot[i]] >= 0) {
            node = sentence_new(kind, node, NULL, start);
            node->slot = slot_pair[slot[i]];
        }
        node = sentence_new(kind, node, NULL, start);
        node->slot = slot[i];
    }
    return node;
}

/* 17p. \exists! x [\in range]\ P, "esiste un solo x tale che P", con il
 * token corrente sul punto esclamativo. Senza insieme, quello di x lo fissa
 * P, che deve obbligarla a starci, come per un "esiste" (17m). Con una parte
 * X per insieme, è \exists! x \in S\ (x \in X \land P). */
static Sentence *abstract_unique(const char *start) {
    next_token();
    int index, sort = -1, part = -1, letter = abstract_letter(&index);
    if (!letter) syntax("dopo \\exists! serve una lettera");
    if (is_command("in")) { next_token(); abstract_range(&sort, &part); }
    if (syntax_error) return sentence_new(S_TRUE, NULL, NULL, start);

    int outer = visible_count, positive = abstract_positive, slot = slot_bind(letter, index, sort);
    abstract_positive = 0;
    Sentence *body = abstract_unary();
    abstract_positive = positive;
    visible_count = outer;
    if (syntax_error) return body;
    if (slot_pair[slot] >= 0) syntax("\\exists! è letto per un elemento, non per una coppia");
    else if (sort < 0 && (slot_sort[slot] < 0 || !sentence_forces(body, 0, slot)))
        syntax("\\exists! senza insieme è ammesso solo se la condizione obbliga la lettera a stare in un insieme, con un'appartenenza");
    if (part >= 0) {
        Sentence *member = sentence_new(S_HOLDS, NULL, NULL, start);
        member->left = elem_new(part, 0, elem_new(-1, slot, NULL, NULL), NULL);
        body = sentence_new(S_AND, member, body, start);
    }
    Sentence *unique = sentence_new(S_UNIQUE, body, NULL, start);
    unique->slot = slot;
    return unique;
}

/* unary: vedi la grammatica in testa al paragrafo, e in 17p. */
static Sentence *abstract_unary(void) {
    const char *start = token.start;
    if (is_command("lnot") || is_command("neg")) {
        int positive = abstract_positive;
        next_token();
        abstract_positive = 0;
        Sentence *inner = abstract_unary();
        abstract_positive = positive;
        return sentence_new(S_NOT, inner, NULL, start);
    }

    /* "Esiste un solo" si distingue da "esiste" al token dopo. */
    if (is_command("exists")) {
        ParserState before = parser_save();
        next_token();
        if (is_char('!')) return abstract_unique(start);
        parser_restore(before);
    }
    if (is_command("forall") || is_command("exists")) return abstract_quantifier(start);
    if (is_char('{')) {
        syntax("una relazione con un nome tra graffe, {\\le}, scritta tra gli elementi come x \\le y, non è trattata: si scrive (x, y) \\in R");
        return sentence_new(S_TRUE, NULL, NULL, start);
    }
    Sentence *declared = abstract_declaration(start);
    if (declared) return declared;

    /* Un confronto o un'appartenenza tra elementi; poi un confronto tra
     * insiemi (17p), come f(X) = f(Y), dove i membri sono insiemi e non
     * elementi. Anche loro possono cominciare con una parentesi: se la
     * lettura non riesce si torna indietro, e la parentesi deve aprire una
     * formula. Un tentativo può aver dato un insieme a una variabile che non
     * lo aveva, o aver letto variabili e insiemi nuovi: si rimette tutto
     * com'era (reader_back()). */
    ReaderMark mark = reader_mark();
    Sentence *atom = abstract_atom(start);
    if (atom && !syntax_error) return atom;
    const char *problem = syntax_error, *where = syntax_where;
    reader_back(&mark);
    int seen = 0;
    Sentence *relation = abstract_set_relation(start, &seen);
    if (relation && !syntax_error) return relation;

    /* Il confronto tra insiemi, se è arrivato a leggere la relazione, è
     * andato più avanti: il suo errore è il più preciso. */
    if (seen && syntax_error) { problem = syntax_error; where = syntax_where; }
    reader_back(&mark);
    if (token.kind == T_OPEN) {
        next_token();
        Sentence *inner = abstract_formula();
        if (token.kind == T_CLOSE) next_token(); else syntax("manca la parentesi chiusa");

        /* Se nemmeno così si legge, l'errore più preciso è quello del
         * tentativo arrivato più avanti nel testo: il confronto, se si è
         * fermato dopo la formula tra parentesi; altrimenti questa. */
        if (syntax_error && problem && where >= syntax_where) syntax_error = problem;
        return inner;
    }
    syntax(problem ? problem : "serve una condizione: un'uguaglianza tra elementi, un'appartenenza, o una formula tra parentesi");
    return sentence_new(S_TRUE, NULL, NULL, start);
}

/* conjunction := unary { \land unary } */
static Sentence *abstract_conjunction(void) {
    const char *start = token.start;
    Sentence *node = abstract_unary();
    while (is_command("land") || is_command("wedge")) {
        next_token();
        Sentence *right = abstract_unary();
        node = sentence_new(S_AND, node, right, start);
    }
    return node;
}

/* disjunction := conjunction { \lor conjunction } */
static Sentence *abstract_disjunction(void) {
    const char *start = token.start;
    Sentence *node = abstract_conjunction();
    while (is_command("lor") || is_command("vee")) {
        next_token();
        Sentence *right = abstract_conjunction();
        node = sentence_new(S_OR, node, right, start);
    }
    return node;
}

/* implication := disjunction [ \implies implication ]
 *
 * A sinistra di un'implicazione non si è "in testa alla tesi": lì un "per
 * ogni applicazione" sarebbe un'ipotesi su tutte le applicazioni. Che il
 * primo pezzo sia un'ipotesi lo si sa solo dopo averlo letto, quindi lo si
 * controlla allora, contando quanti ne ha incontrati. */
static Sentence *abstract_implication(void) {
    const char *start = token.start;
    int before = map_quantifiers;
    Sentence *node = abstract_disjunction();
    if (!is_command("implies") && !is_command("Rightarrow") && !is_command("Longrightarrow")) return node;
    if (map_quantifiers != before) syntax("\\forall f \\in T^{S} e \\forall X \\subseteq S sono letti solo in testa alla tesi, dove valgono quanto una dichiarazione: un'ipotesi su tutte le applicazioni o su tutte le parti non è trattata");
    next_token();
    Sentence *right = abstract_implication();
    return sentence_new(S_IMPLIES, node, right, start);
}

/* formula := implication { \iff implication }
 *
 * I due lati di un'equivalenza fanno ciascuno anche da ipotesi dell'altro:
 * vale lo stesso controllo. */
static Sentence *abstract_formula(void) {
    const char *start = token.start;
    int before = map_quantifiers;
    Sentence *node = abstract_implication();
    while (is_command("iff") || is_command("Leftrightarrow") || is_command("Longleftrightarrow")) {
        next_token();
        Sentence *right = abstract_implication();
        node = sentence_new(S_IFF, node, right, start);
        if (map_quantifiers != before) syntax("\\forall f \\in T^{S} e \\forall X \\subseteq S sono letti solo in testa alla tesi, dove valgono quanto una dichiarazione: in un'equivalenza ognuno dei due lati è anche un'ipotesi, e un'ipotesi su tutte le applicazioni o su tutte le parti non è trattata");
    }
    return node;
}

/* Ritorna 1 se in s c'è una dichiarazione fuori posto. allowed dice se nel
 * punto di s una dichiarazione è ammessa: lo è tra le ipotesi, attraverso
 * le sole congiunzioni. */
static int declaration_misplaced(const Sentence *s, int allowed) {
    if (!s) return 0;
    if (s->kind == S_TRUE) return !allowed;
    if (s->kind == S_AND) return declaration_misplaced(s->a, allowed) || declaration_misplaced(s->b, allowed);
    return declaration_misplaced(s->a, 0) || declaration_misplaced(s->b, 0);
}

/* statement: legge l'enunciato su insiemi qualsiasi che comincia al token
 * corrente e ne ritorna l'albero, lasciando in symbols[] e negli altri
 * elenchi ciò che dichiara. Gli errori sono annotati con syntax(). */
static Sentence *abstract_statement(void) {
    symbol_count = sort_count = slot_count = visible_count = definition_count = builder_depth = 0;
    abstract_positive = 1;
    abstract_example = map_quantifiers = 0;
    family_slot = -1;

    /* Le lettere che il testo usa: lo svolgimento (17q) dà alle variabili
     * che crea lettere che non compaiono, prese con fresh_letter(). */
    unfolding_reset(token.start);

    /* In testa, gli "esiste" di un esempio: insiemi, parti e relazioni,
     * un'applicazione. Un "esiste" di un altro tipo è già parte della
     * condizione: si torna indietro e lo legge lei. */
    while (is_command("exists") && !syntax_error) {
        ParserState before = parser_save();
        const char *kept = consumed;
        Symbol shape = {0, 0, 1, {0, 0}, TRUTH, 0, 0};
        int name[MAX_BOUND], index[MAX_BOUND], count = 0, operation = 0, sets = 1, sorts_before = sort_count;
        do {
            next_token();  /* supera il quantificatore, poi ogni virgola */
            name[count] = operation_name();
            if (name[count]) { operation = 1; index[count] = 0; next_token(); }
            else name[count] = abstract_name(&index[count]);
            sets = sets && name[count] < 128 && isupper(name[count]) && !index[count];
        } while (name[count] && ++count < MAX_BOUND && is_char(','));
        int parts = count > 0 && !operation && is_command("subseteq"), map = count == 1 && is_command("colon");

        /* \exists f, g \in T^{S}: delle applicazioni da S a T. Si distingue
         * da \exists x \in S, che è già parte della condizione, solo
         * dall'apice dopo l'insieme. */
        if (count > 0 && !operation && !parts && !map && !sets && is_command("in")) {
            next_token();
            shape.range = abstract_sort();
            if (!syntax_error && is_char('^')) {
                next_token();
                int braces = is_char('{');
                if (braces) next_token();
                shape.domain[0] = abstract_sort();
                if (braces) expect_char('}', "manca }");
                for (int i = 0; i < count && !syntax_error; i++) {
                    shape.name = name[i];
                    shape.index = index[i];
                    symbol_add(shape);
                }
                abstract_example = 1;
                continue;
            }
        }
        if (!parts && !map && !(sets && count > 0)) {
            parser_restore(before);
            consumed = kept;
            sort_count = sorts_before;
            break;
        }
        abstract_example = 1;

        /* Insiemi, eventualmente detti non vuoti: lo sono comunque quelli
         * tra cui l'esempio viene cercato. */
        if (!parts && !map) {
            for (int i = 0; i < count; i++) sort_index(name[i]);
            if (is_command("neq") || is_command("ne")) {
                next_token();
                if (is_command("emptyset") || is_command("varnothing")) next_token(); else syntax("dopo \\exists S \\neq serve \\emptyset");
            }
            continue;
        }

        /* Il resto della dichiarazione, comune a tutti i nomi letti. */
        next_token();
        shape.domain[0] = abstract_sort();
        if (is_command("times")) { next_token(); shape.domain[1] = abstract_sort(); shape.arity = 2; }
        if (map) {
            if (is_command("to") || is_command("rightarrow") || is_command("longrightarrow")) next_token();
            else syntax("in una dichiarazione f \\colon A \\to B manca \\to");
            shape.range = abstract_sort();
            if (operation && shape.arity != 2) syntax("un'operazione si dichiara su un insieme di coppie, A \\times B");
        }
        for (int i = 0; i < count && !syntax_error; i++) {
            shape.name = name[i];
            shape.index = index[i];
            symbol_add(shape);
        }
    }

    /* La condizione. Chi chiede un esempio ha già dichiarato tutto, anche
     * gli insiemi: uno che comparisse solo qui non si saprebbe se va
     * cercato anche lui o se l'enunciato vale per ogni sua scelta. */
    int sorts_declared = sort_count;
    Sentence *root = abstract_formula();
    if (token.kind != T_END) syntax("testo inatteso dopo l'enunciato");
    if (abstract_example && sort_count != sorts_declared) syntax("in un enunciato che chiede un esempio ogni insieme va introdotto con \\exists");

    /* Nella condizione di un esempio una definizione (17p), R = \{...\}, sta
     * tra le congiunzioni più esterne, come tra le ipotesi: dà un nome a un
     * insieme fatto di ciò che l'esempio cerca. */
    int misplaced = root->kind == S_IMPLIES && !abstract_example ? declaration_misplaced(root->a, 1) || declaration_misplaced(root->b, 0)
                                                                  : declaration_misplaced(root, abstract_example);
    if (misplaced) syntax("un'applicazione, un'operazione, una parte, un elemento o un insieme si dichiarano o si definiscono solo tra le ipotesi, in cima all'enunciato");
    return root;
}

/* 17q. LO SVOLGIMENTO DELLA NOTAZIONE DEI LIBRI
 *
 * Il capitolo 28 ragiona con le sole condizioni di 17o: uguaglianze tra
 * elementi, appartenenze a una parte o a una relazione dichiarata,
 * connettivi e quantificatori su elementi. Qui ogni nodo di 17p diventa la
 * condizione che abbrevia, come la scriverebbe chi non usa la notazione degli
 * insiemi:
 *
 *     t \in \{(x, y) \mid c\}       c, con le componenti di t al posto di x e y
 *     t \in f(E)                    \exists u\ (u \in E \land f(u) = t)
 *     t \in f^{-1}(E)               f(t) \in E
 *     t \in E \cup F, \cap, \setminus, \triangle
 *                                   t \in E \lor t \in F, \land, "e non", "o l'uno o l'altro"
 *     t \in \bigcup_{i \in I} E     \exists i \in I\ (t \in E)
 *     t \in \{a, b\}                t = a \lor t = b
 *     t \in S, t \in \emptyset      vero, falso
 *     E = F, E \subseteq F          \forall t\ (t \in E \iff t \in F), \implies
 *     E \subsetneq F                E \subseteq F \land \lnot (F \subseteq E)
 *     \exists! x\ P                 \exists x\ (P \land \forall z\ (P[z / x] \implies z = x))
 *
 * dove le appartenenze a destra si svolgono a loro volta. Uno schema che
 * ripete una lettera, (x, x), dà in più l'uguaglianza delle componenti che
 * vanno al suo posto.
 *
 * La sostituzione si fa sull'albero, non sul testo, e la cattura di una
 * lettera non può succedere: una variabile è un posto (slot), non una
 * lettera. Ogni copia della condizione di un insieme, una per appartenenza,
 * ha i suoi quantificatori con posti nuovi, e così ogni variabile che lo
 * svolgimento crea (la t di un'uguaglianza tra insiemi, la u di
 * un'immagine): due copie non condividono mai un posto, nemmeno una dentro
 * l'altra. Restano le lettere, che servono a scrivere la dimostrazione e il
 * certificato, e lì una cattura sarebbe possibile: per questo ogni posto
 * nuovo riceve una lettera che l'enunciato non usa (fresh_letter(), la stessa
 * di 17j), diversa da quelle di tutte le variabili nuove. I quantificatori
 * scritti nell'enunciato, fuori da ogni insieme, tengono il loro posto e la
 * loro lettera: la dimostrazione li cita come sono scritti.
 *
 * Il controllo indipendente è nel capitolo 28 (28h): l'enunciato com'è
 * scritto, con i nodi di 17p, si calcola su molti modelli finiti con un
 * valutatore che non sostituisce nulla (lega le lettere degli insiemi ai
 * valori, come un ambiente), e deve dare lo stesso valore dell'enunciato
 * svolto.
 */

/* Mentre si svolge: l'elemento che va al posto di ogni variabile, o NULL
 * dove la variabile resta com'è. Funziona come un ambiente: chi lo cambia lo
 * rimette com'era quando ha finito. */
static Elem *unfold_to[MAX_SLOTS];

/* Quante copie della condizione di un insieme si stanno svolgendo una
 * dentro l'altra. Fuori da ogni copia (0) un quantificatore tiene il suo
 * posto. */
static int unfold_copying;

/* Ritorna il posto di una variabile nuova dell'insieme sort, con una lettera
 * che l'enunciato non usa. */
static int unfold_slot(int sort) {
    int slot = slot_new(fresh_letter(), 0, sort);
    return slot < 0 ? 0 : slot;
}

/* Ritorna 1 se s è la costante vero, -1 se è la costante falso (vero
 * negato), 0 altrimenti. Lo svolgimento le produce per t \in S e t \in
 * \emptyset. */
static int unfold_constant(const Sentence *s) {
    if (s->kind == S_TRUE) return 1;
    return s->kind == S_NOT && s->a->kind == S_TRUE ? -1 : 0;
}

/* Ritorna un nuovo nodo di tipo kind, con il testo di from.
 *
 * Un connettivo con una costante per operando si semplifica, perché la
 * dimostrazione non debba portarsi dietro "vero e ..." o "... se e solo se
 * falso". Ogni semplificazione è una legge della logica: vero \land x è x,
 * falso \land x è falso; vero \lor x è vero, falso \lor x è x; vero
 * \implies x è x, falso \implies x è vero, x \implies vero è vero, x
 * \implies falso è \lnot x; vero \iff x è x, falso \iff x è \lnot x. */
static Sentence *unfold_node(SentenceKind kind, Sentence *a, Sentence *b, const Sentence *from) {
    int ca = a ? unfold_constant(a) : 0, cb = b ? unfold_constant(b) : 0;
    if (kind == S_IFF && !ca && cb) { Sentence *swap = a; a = b; b = swap; ca = cb; cb = 0; }
    if (kind == S_AND && (ca || cb)) return ca == 1 ? b : cb == 1 ? a : ca ? a : b;
    if (kind == S_OR && (ca || cb)) return ca == -1 ? b : cb == -1 ? a : ca ? a : b;
    if (kind == S_IMPLIES && (ca == -1 || cb == 1)) return unfold_node(S_TRUE, NULL, NULL, from);
    if (kind == S_IMPLIES && ca == 1) return b;
    if (kind == S_IFF && ca == 1) return b;
    if ((kind == S_IMPLIES && cb == -1) || (kind == S_IFF && ca == -1)) {
        if (kind == S_IFF) a = b;
        kind = S_NOT;
        b = NULL;
    }
    Sentence *s = sentence_new(kind, a, b, from->start);
    s->end = from->end;
    return s;
}

/* Ritorna l'uguaglianza left = right, con il testo di from. */
static Sentence *unfold_equal(Elem *left, Elem *right, const Sentence *from) {
    Sentence *s = unfold_node(S_EQUAL, NULL, NULL, from);
    s->left = left;
    s->right = right;
    return s;
}

/* Ritorna la congiunzione di a e b; se uno dei due è NULL, l'altro. */
static Sentence *unfold_and(Sentence *a, Sentence *b, const Sentence *from) {
    return !a ? b : !b ? a : unfold_node(S_AND, a, b, from);
}

/* Ritorna l'elemento e con ogni variabile sostituita come dice unfold_to[].
 * Gli elementi non si modificano mai: dove nulla cambia si riusa e. */
static Elem *unfold_elem(Elem *e) {
    if (e->symbol < 0) return unfold_to[e->slot] ? unfold_to[e->slot] : e;
    Elem *a = e->arg[0] ? unfold_elem(e->arg[0]) : NULL, *b = e->arg[1] ? unfold_elem(e->arg[1]) : NULL;
    return a == e->arg[0] && b == e->arg[1] ? e : elem_new(e->symbol, 0, a, b);
}

/* Ritorna la variabile di posto slot, come elemento. */
static Elem *unfold_variable(int slot) { return elem_new(-1, slot, NULL, NULL); }

static Sentence *unfold_sentence(const Sentence *s);

/* Ritorna la condizione "il membro t[] sta nell'insieme set", svolta (vedi
 * la tabella in testa al paragrafo). t[] ha set->arity componenti, già
 * svolte; from dà il testo ai nodi nuovi. */
static Sentence *unfold_member(Elem *const *t, const SetTerm *set, const Sentence *from) {
    Sentence *s, *x, *y;
    switch (set->kind) {
    case ST_SYMBOL: case ST_SECTION:
        s = unfold_node(S_HOLDS, NULL, NULL, from);
        s->left = elem_new(set->symbol, 0, t[0], set->kind == ST_SECTION ? unfold_elem(set->item[0]) : set->arity == 2 ? t[1] : NULL);
        return s;
    case ST_SORT:
        return unfold_node(S_TRUE, NULL, NULL, from);
    case ST_EMPTY:
        return unfold_node(S_NOT, unfold_node(S_TRUE, NULL, NULL, from), NULL, from);
    case ST_LIST:
        s = NULL;
        for (int i = 0; i < set->items; i++) {
            x = unfold_equal(t[0], unfold_elem(set->item[i]), from);
            s = s ? unfold_node(S_OR, s, x, from) : x;
        }
        return s;
    case ST_BUILDER: {
        /* Le componenti di t vanno al posto delle lettere dello schema; una
         * lettera già sostituita da una componente precedente dà
         * un'uguaglianza. */
        Elem *saved[MAX_ARITY];
        Sentence *equal = NULL;
        for (int i = 0; i < set->bounds; i++) { saved[i] = unfold_to[set->bound[i]]; unfold_to[set->bound[i]] = NULL; }
        for (int i = 0; i < set->arity; i++) {
            int slot = set->item[i]->slot;
            if (!unfold_to[slot]) unfold_to[slot] = t[i];
            else equal = unfold_and(equal, unfold_equal(t[i], unfold_to[slot], from), from);
        }
        unfold_copying++;
        s = unfold_sentence(set->condition);
        unfold_copying--;
        for (int i = 0; i < set->bounds; i++) unfold_to[set->bound[i]] = saved[i];
        return unfold_and(equal, s, from);
    }
    case ST_IMAGE: case ST_PREIMAGE: {
        /* f(E): un u del dominio, che sta in E e va in t. f^{-1}(E): f(t) sta
         * in E. */
        const Symbol *f = &symbols[set->symbol];
        if (set->kind == ST_PREIMAGE) {
            Elem *image = elem_new(set->symbol, 0, t[0], NULL);
            return unfold_member(&image, set->a, from);
        }
        int u = unfold_slot(f->domain[0]);
        Elem *argument = unfold_variable(u);
        x = unfold_member(&argument, set->a, from);
        y = unfold_equal(elem_new(set->symbol, 0, argument, NULL), t[0], from);
        s = unfold_node(S_EXISTS, unfold_node(S_AND, x, y, from), NULL, from);
        s->slot = u;
        return s;
    }
    case ST_UNION: case ST_INTER: case ST_DIFF: case ST_SYMDIFF:
        x = unfold_member(t, set->a, from);
        y = unfold_member(t, set->b, from);
        if (set->kind == ST_UNION) return unfold_node(S_OR, x, y, from);
        if (set->kind == ST_INTER) return unfold_node(S_AND, x, y, from);
        if (set->kind == ST_DIFF) return unfold_node(S_AND, x, unfold_node(S_NOT, y, NULL, from), from);
        return unfold_node(S_NOT, unfold_node(S_IFF, x, y, from), NULL, from);
    case ST_BIG_UNION: case ST_BIG_INTER: {
        /* Un indice nuovo, che per un'unione esiste e per un'intersezione
         * varia, nel suo insieme o nella parte symbol. */
        int index = set->bound[0], fresh = unfold_slot(slot_sort[index]), every = set->kind == ST_BIG_INTER;
        Elem *saved = unfold_to[index];
        unfold_to[index] = unfold_variable(fresh);
        x = unfold_member(t, set->a, from);
        if (set->symbol >= 0) {
            y = unfold_node(S_HOLDS, NULL, NULL, from);
            y->left = elem_new(set->symbol, 0, unfold_to[index], NULL);
            x = unfold_node(every ? S_IMPLIES : S_AND, y, x, from);
        }
        unfold_to[index] = saved;
        s = unfold_node(every ? S_FORALL : S_EXISTS, x, NULL, from);
        s->slot = fresh;
        return s;
    }
    }
    return unfold_node(S_TRUE, NULL, NULL, from);
}

/* Ritorna la condizione "ogni membro di set sta in other", svolta: per ogni
 * t, t \in set implica t \in other; con iff, "se e solo se". */
static Sentence *unfold_inclusion(const SetTerm *set, const SetTerm *other, int iff, const Sentence *from) {
    int slot[MAX_ARITY];
    Elem *t[MAX_ARITY];
    for (int i = 0; i < set->arity; i++) {
        slot[i] = unfold_slot(set->sort[i]);
        t[i] = unfold_variable(slot[i]);
    }
    Sentence *s = unfold_node(iff ? S_IFF : S_IMPLIES, unfold_member(t, set, from), unfold_member(t, other, from), from);
    for (int i = set->arity - 1; i >= 0; i--) {
        s = unfold_node(S_FORALL, s, NULL, from);
        s->slot = slot[i];
    }
    return s;
}

/* Ritorna la condizione s svolta: lo stesso albero, con i nodi di 17p al
 * posto dei quali c'è ciò che abbreviano, e le variabili sostituite come dice
 * unfold_to[]. Dentro una copia (unfold_copying) ogni quantificatore riceve
 * un posto nuovo. */
static Sentence *unfold_sentence(const Sentence *s) {
    Sentence *copy, *x;
    int slot;
    switch (s->kind) {
    case S_TRUE:
        return unfold_node(S_TRUE, NULL, NULL, s);
    case S_EQUAL:
        return unfold_equal(unfold_elem(s->left), unfold_elem(s->right), s);
    case S_HOLDS:
        copy = unfold_node(S_HOLDS, NULL, NULL, s);
        copy->left = unfold_elem(s->left);
        return copy;
    case S_NOT: case S_AND: case S_OR: case S_IMPLIES: case S_IFF:
        return unfold_node(s->kind, unfold_sentence(s->a), s->b ? unfold_sentence(s->b) : NULL, s);
    case S_FORALL: case S_EXISTS: {
        Elem *saved = unfold_to[s->slot];
        slot = unfold_copying ? unfold_slot(slot_sort[s->slot]) : s->slot;
        if (unfold_copying) unfold_to[s->slot] = unfold_variable(slot);
        copy = unfold_node(s->kind, unfold_sentence(s->a), NULL, s);
        copy->slot = slot;
        unfold_to[s->slot] = saved;
        return copy;
    }
    case S_UNIQUE: {
        /* \exists y\ (P \land \forall z\ (P[z / y] \implies z = y)): la
         * seconda copia di P è una copia, con z al posto di y. */
        Elem *saved = unfold_to[s->slot];
        slot = unfold_copying ? unfold_slot(slot_sort[s->slot]) : s->slot;
        if (unfold_copying) unfold_to[s->slot] = unfold_variable(slot);
        Sentence *first = unfold_sentence(s->a);
        int other = unfold_slot(slot_sort[s->slot]);
        unfold_to[s->slot] = unfold_variable(other);
        unfold_copying++;
        Sentence *second = unfold_sentence(s->a);
        unfold_copying--;
        unfold_to[s->slot] = saved;
        x = unfold_node(S_FORALL, unfold_node(S_IMPLIES, second, unfold_equal(unfold_variable(other), unfold_variable(slot), s), s), NULL, s);
        x->slot = other;
        copy = unfold_node(S_EXISTS, unfold_node(S_AND, first, x, s), NULL, s);
        copy->slot = slot;
        return copy;
    }
    case S_MEMBER: {
        Elem *t[MAX_ARITY];
        for (int i = 0; i < s->items; i++) t[i] = unfold_elem(s->item[i]);
        return unfold_member(t, s->set, s);
    }
    case S_INCLUDED:
        x = unfold_inclusion(s->set, s->other, 0, s);
        if (!s->proper) return x;

        /* Propria: e qualche membro dell'altro insieme non sta nel primo. */
        copy = unfold_inclusion(s->other, s->set, 0, s);
        return unfold_node(S_AND, x, unfold_node(S_NOT, copy, NULL, s), s);
    case S_SAME:
        return unfold_inclusion(s->set, s->other, 1, s);
    }
    return unfold_node(S_TRUE, NULL, NULL, s);
}

/* Ritorna 1 se in s c'è un nodo di 17p: solo allora lo svolgimento cambia
 * qualcosa, e va ricontrollato (28h). */
static int sentence_has_book_notation(const Sentence *s) {
    if (!s) return 0;
    if (s->kind >= S_MEMBER) return 1;
    return sentence_has_book_notation(s->a) || sentence_has_book_notation(s->b);
}

/* Ritorna l'enunciato root svolto (vedi la testa del paragrafo). Le variabili
 * nuove si aggiungono a quelle dell'enunciato; un errore, come troppe
 * variabili o lettere finite, è annotato con syntax(). */
static Sentence *abstract_unfold(const Sentence *root) {
    memset(unfold_to, 0, sizeof unfold_to);
    unfold_copying = 0;
    return unfold_sentence(root);
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
 * oppure con una formula in linea che comincia con il segno di derivata,
 * \frac{d^n}{dx^n} o \dfrac{d^n}{dx^n}: è così che gli eserciziari scrivono
 * gli enunciati sulle derivate n-esime, e quel segno in testa a un \item non
 * può essere altro che l'inizio di un enunciato.
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
 * dimostrare e la risposta è NON SUPPORTATO. Fa eccezione la prosa che
 * definisce una successione per ricorrenza e ne afferma qualcosa, "Sia
 * $a_0 = 0$ e $a_{n+1} = \sqrt{6 + a_n}$. Allora $a_n < 3$ per ogni
 * $n \ge 0$": le formule sono tra dollari e le parole intorno sono poche e
 * fisse, quindi la si legge senza indovinare (read_sequence_item()).
 */

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

/* Ritorna 1 se la formula [from, to) è esattamente "lettera \ge numero", con
 * spazi e graffe facoltativi e nient'altro, e allora mette la lettera in
 * *letter e il numero (anche negativo) in *number. */
static int is_lower_bound(const char *from, const char *to, int *letter, long long *number) {
    const char *q = from;
    char *after;
    while (q < to && isspace((unsigned char)*q)) q++;
    if (q == to || !isalpha((unsigned char)*q)) return 0;
    *letter = *q++;
    while (isspace((unsigned char)*q)) q++;
    if (strncmp(q, "\\ge", 3) != 0) return 0;
    q += q[3] == 'q' ? 4 : 3;
    while (isspace((unsigned char)*q) || *q == '{') q++;
    *number = strtoll(q, &after, 10);
    if (after == q) return 0;
    q = after;
    while (isspace((unsigned char)*q) || *q == '}') q++;
    return q == to;
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

        /* lettera \ge numero, e nient'altro. */
        int letter;
        long long number;
        if (found || !is_lower_bound(open + 1, close, &letter, &number)) {
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

/* Il motivo per cui un \item scritto a parole non è un enunciato. */
static const char *const PROSE = "enunciato in prosa: serve una formula sola in evidenza";

/* Le parole con cui comincia un \item che definisce una successione per
 * ricorrenza, quelle che vi separano le ipotesi dalla tesi, e quelle che
 * possono stare tra una tesi e l'altra. */
static const char *const opening_words[] = {"sia", "siano", "con", "let", "with"};
static const char *const closing_words[] = {"allora", "vale", "then"};
static const char *const joining_words[] = {"e", "ed", "and"};

/* Aggiunge a out->formula, lunga *length, il testo prefix seguito dai
 * caratteri [from, to), e aggiorna *length. Ritorna 0 se non c'è posto. */
static int formula_append(Item *out, size_t *length, const char *prefix, const char *from, const char *to) {
    int written = snprintf(out->formula + *length, sizeof out->formula - *length, "%s%.*s", prefix, (int)(to - from), from);
    if (written < 0 || (size_t)written >= sizeof out->formula - *length) return 0;
    *length += (size_t)written;
    return 1;
}

/* Legge un \item che definisce una successione per ricorrenza e ne afferma
 * qualcosa, scritto a parole con le formule tra dollari:
 *
 *     Sia $a_0=0$, $a_1=\sin x$ e $a_{n+1}=2\cos x\,a_n-a_{n-1}$ per $n\ge1$.
 *     Allora $a_n=\sin(nx)$ per ogni $n\ge0$.
 *
 * e ne mette in out->formula la scrittura in formula, quella che legge
 * parse_sequence_statement():
 *
 *     a_0=0, a_1=\sin x, a_{n+1}=2\cos x\,a_n-a_{n-1}, n \ge 1 \implies a_n=\sin(nx)
 *
 * Che cosa viene letto, e nient'altro:
 *
 *   21a. una delle opening_words;
 *   21b. le ipotesi, fino a una delle closing_words: le formule che
 *        cominciano con una lettera con pedice e contengono un segno =; una
 *        formula "lettera \ge numero" dice da dove vale la regola. Ogni
 *        altra formula o parola ("$a_0>0$", "$0\le x\le\pi$", "tale che...")
 *        è una condizione che il programma non sa leggere, e *unread
 *        diventa 1: l'enunciato può ancora essere dimostrato, senza usarla,
 *        ma non dichiarato falso;
 *   21c. le tesi: le formule che si susseguono, separate solo da virgole e
 *        da una delle joining_words.
 *
 * Mette in *tail il punto dove le tesi finiscono: il testo che resta è
 * quello di ogni \item, e lo legge read_conditions(). Ritorna NULL se
 * riesce, PROSE se l'\item non ha questa forma, o un altro motivo, in
 * italiano. */
static const char *read_sequence_item(const char *item, const char *end, Item *out, const char **tail, int *unread) {
    const char *long_text = "enunciato troppo lungo";
    size_t length = 0;
    int givens = 0, claims = 0, since = 0, closed = 0;
    const char *p = item;
    *unread = 0;

    /* 21a. La parola d'apertura. */
    while (p < end && isalpha((unsigned char)*p)) p++;
    if (!word_in(item, p, opening_words, sizeof opening_words / sizeof *opening_words)) return PROSE;

    /* 21b. Le ipotesi. */
    while (p < end && !closed) {
        unsigned char ch = (unsigned char)*p;
        if (isalpha(ch) || ch >= 128) {
            const char *word = p;
            while (p < end && (isalpha((unsigned char)*p) || (unsigned char)*p >= 128)) p++;
            if (givens && word_in(word, p, closing_words, sizeof closing_words / sizeof *closing_words)) closed = 1;
            else if (!word_in(word, p, plain_words, sizeof plain_words / sizeof *plain_words)) *unread = 1;
            continue;
        }
        if (ch != '$') {
            if (!strchr(" \t\r\n.,;:(){}", ch)) *unread = 1;
            p++;
            continue;
        }
        const char *open = p + 1, *close = memchr(open, '$', (size_t)(end - open)), *first = open;
        if (!close) return PROSE;
        p = close + 1;

        /* Una formula: da dove vale la regola, un'ipotesi, o una condizione
         * non letta. */
        int letter;
        long long number;
        while (first < close && isspace((unsigned char)*first)) first++;
        if (!since && is_lower_bound(open, close, &letter, &number)) {
            char bound[48];
            snprintf(bound, sizeof bound, ", %c \\ge %lld", letter, number);
            if (!givens || !formula_append(out, &length, bound, open, open)) return givens ? long_text : PROSE;
            since = 1;
        } else if (close - first > 2 && isalpha((unsigned char)first[0]) && first[1] == '_' && memchr(first, '=', (size_t)(close - first))) {
            if (!formula_append(out, &length, givens++ ? ", " : "", open, close)) return long_text;
        } else {
            *unread = 1;
        }
    }
    if (!closed) return PROSE;

    /* 21c. Le tesi. */
    for (;;) {
        const char *q = p;
        while (q < end && (isspace((unsigned char)*q) || *q == ',')) q++;
        const char *word = q;
        while (q < end && isalpha((unsigned char)*q)) q++;
        if (q > word && !(claims && word_in(word, q, joining_words, sizeof joining_words / sizeof *joining_words))) break;
        while (q < end && isspace((unsigned char)*q)) q++;
        const char *close = q < end && *q == '$' ? memchr(q + 1, '$', (size_t)(end - q - 1)) : NULL;
        if (!close) break;
        if (!formula_append(out, &length, claims++ ? " \\land " : " \\implies ", q + 1, close)) return long_text;
        p = close + 1;
    }
    *tail = p;
    return claims ? NULL : PROSE;
}

/* Estrae in *out l'enunciato contenuto nel testo [item, end) di un \item.
 * Ritorna NULL se riesce; altrimenti ritorna il motivo, in italiano, per cui
 * il testo non è un enunciato utilizzabile. */
static const char *read_item(const char *item, const char *end, Item *out) {
    while (item < end && isspace((unsigned char)*item)) item++;

    /* Individua la formula: \[ ... \] oppure $\displaystyle ... $. Un \item
     * che comincia con una parola può definire una successione (vedi
     * read_sequence_item()), e allora la formula la compone il lettore. */
    const char *start = NULL, *stop = NULL, *tail = NULL;
    size_t closing = 1;  /* lunghezza del delimitatore che chiude la formula */
    int given_unread = 0;
    if (end - item >= 2 && strncmp(item, "\\[", 2) == 0) {
        start = item + 2;
        stop = strstr(start, "\\]");
        closing = 2;
    } else if (item < end && *item == '$') {
        /* In linea, la formula è l'enunciato solo se lo dice \displaystyle,
         * oppure se comincia con il segno di derivata, che non può essere
         * altro che l'inizio di un enunciato. */
        start = item + 1;
        while (start < end && isspace((unsigned char)*start)) start++;
        if (strncmp(start, "\\displaystyle", 13) != 0 && !starts_with_derivative(start)) return PROSE;
        stop = strchr(start, '$');
    } else {
        const char *problem = read_sequence_item(item, end, out, &tail, &given_unread);
        if (problem) return problem;
    }

    /* Copia la formula in una stringa a sé. */
    if (!tail) {
        if (!stop || stop > end) return PROSE;
        size_t length = (size_t)(stop - start);
        if (length >= sizeof out->formula) return "enunciato troppo lungo";
        memcpy(out->formula, start, length);
        out->formula[length] = 0;
        tail = stop + closing;
    }

    /* Variabile di induzione e caso base, se indicati dopo la formula, e ciò
     * che del testo non è stato letto. Una seconda uguaglianza può essere
     * un'altra affermazione, o un caso particolare in cui la prima non ha
     * senso: dimostrare solo la prima e rispondere DIMOSTRATO per tutto
     * l'\item sarebbe dire più di quanto si sa. */
    int unread;
    out->variable = 'n';
    out->base = 0;
    out->has_base = read_conditions(tail, end, &out->variable, &out->base, &unread, &out->remark);
    out->conditional = unread == 1 || given_unread;
    if (unread == 2) return "dopo la formula ce n'è un'altra con un segno =: un \\item deve contenere un solo enunciato";
    return NULL;
}

