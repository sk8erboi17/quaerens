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
 *     (capitoli 8 e 9);
 *   - gli enunciati sugli insiemi: SetNode (capitolo 14);
 *   - gli enunciati con quantificatori, gli insiemi numerici e gli insiemi
 *     di soluzioni: Formula (capitolo 17), le cui foglie sono espressioni;
 *   - gli enunciati su insiemi qualsiasi, con elementi, operazioni,
 *     applicazioni e relazioni senza una regola: Sentence (paragrafo 17o,
 *     in fondo al capitolo 17), le cui foglie sono Elem.
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
 * solito sì; oppure è un numero. Gli alberi stanno nell'arena. */
typedef struct {
    int variable;     /* la lettera x rispetto a cui si deriva */
    int order;        /* la lettera n dell'ordine di derivazione, o 0 se è un numero */
    long long times;  /* l'ordine, quando è un numero: almeno 1 */
    Node *function;   /* la funzione da derivare */
    Node *formula;    /* ciò che l'enunciato dice della derivata */
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
 *                | '(' expr ')' | \left| expr \right|
 *                | \frac group group | \binom group group
 *                | \sqrt ['[' expr ']'] group
 *                | (\sum | \prod) '_' '{' lettera '=' expr '}' '^' group term
 *                | funzione ['^' group] argomento
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
 *     derivative := \frac '{' 'd' ['^' group] '}' '{' 'd' lettera ['^' group] '}' term '=' expr
 *
 * Una quarta, per i limiti (vedi parse_limit_statement(), 9i):
 *
 *     limit  := [\nexists] \lim '_' '{' lettera (\to | \rightarrow) punto '}' expr ['=' valore]
 *     punto  := ['+' | '-'] \infty | expr [ '^' ('+' | '-' | '{+}' | '{-}') ]
 *     valore := ['+' | '-'] \infty | expr
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
 *     si legge solo in testa a un enunciato su una derivata.
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
 * Esponenziale, seno e coseno iperbolici, arcoseno e arcocoseno non sono
 * nodi propri dell'albero: si leggono con la loro definizione, scritta con
 * le funzioni che il motore già tratta. Così ogni metodo del motore li
 * conosce senza una regola in più. Le definizioni valgono su tutto il campo
 * di esistenza, estremi compresi:
 *
 *     \exp u = e^{u},     \sinh u = (e^{u} - e^{-u}) / 2,     \cosh u = (e^{u} + e^{-u}) / 2,
 *     \arcsin u = 2 \arctan(u / (1 + \sqrt{1 - u^2})),        \arccos u = \pi/2 - \arcsin u.
 *
 * La formula dell'arcoseno è la bisezione dell'angolo: se a = \arcsin u,
 * con a tra -\pi/2 e \pi/2, allora tan(a/2) = sin a / (1 + cos a) =
 * u / (1 + \sqrt{1 - u^2}), e a/2 sta tra -\pi/4 e \pi/4, dove l'arcotangente
 * inverte la tangente. Vale anche in u = 1 e u = -1, e non altrove, perché
 * fuori da [-1, 1] la radice non c'è: il campo di esistenza è lo stesso.
 * La lettera e è il numero di Nepero.
 *
 * Warning: se aggiungi un nome qui, aggiungi la sua definizione in
 * parse_defined_function(). */
static const char *const defined_functions[] = {"exp", "sinh", "cosh", "arcsin", "arccos"};

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

    /* Le posizioni in defined_functions[]: 0 \exp, 1 \sinh, 2 \cosh,
     * 3 \arcsin, 4 \arccos. */
    if (which == 0) {
        value = up;
    } else if (which <= 2) {
        value = node_new(N_DIV, node_new(which == 1 ? N_SUB : N_ADD, up, down), node_num(2));
    } else {
        Node *root = node_call(FN_ROOT, node_new(N_SUB, node_num(1), node_new(N_POW, u, node_num(2))), node_num(2));
        Node *half = node_call(FN_ARCTAN, node_new(N_DIV, u, node_new(N_ADD, node_num(1), root)), NULL);
        value = node_new(N_MUL, node_num(2), half);
        if (which == 4) {
            Node *pi = node_new(N_VAR, NULL, NULL);
            pi->name = VAR_PI;
            value = node_new(N_SUB, node_new(N_DIV, pi, node_num(2)), value);
        }
    }
    return read->kind == N_POW ? node_new(N_POW, value, read->b) : value;
}

/* Ritorna 1 se text comincia con il segno di derivata di Leibniz: \frac,
 * \dfrac o \tfrac, sopra {d} oppure {d^...}, sotto {d seguito da una
 * lettera. Prima del segno si saltano gli spazi e i comandi di sola
 * presentazione \displaystyle e \textstyle, come fa next_token().
 *
 * Serve in due posti. In testa a un enunciato dice che vale la pena di
 * leggerlo come enunciato su una derivata (capitolo 27). Dentro
 * un'espressione dice che lì c'è un segno di derivata, e il lettore lo
 * rifiuta (paragrafo 9d): letto come frazione delle lettere d e x,
 * \frac{d}{dx} x^{2} = x sarebbe un'uguaglianza vera, (1/x) x^2 = x, e il
 * programma "dimostrerebbe" che la derivata di x^2 è x. È successo, con un
 * \displaystyle davanti che questa funzione non saltava: vedi
 * tests/derivatives/false_nth_derivatives.tex. */
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

    /* 9d. Comandi con argomenti tra graffe. Il segno di derivata non è una
     * frazione: in testa all'enunciato lo legge parse_derivative_statement(),
     * dentro un'espressione non ha un significato che il programma tratti
     * (vedi starts_with_derivative()). */
    if (is_command("frac") || is_command("dfrac") || is_command("tfrac")) {
        if (starts_with_derivative(token.start)) { syntax("il segno di derivata si legge solo in testa all'enunciato"); return node_num(0); }
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
           is_command("sqrt") || is_command("sum") || is_command("prod") || is_command("pi") || defined_function() >= 0;
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
 * Il segno di derivata è la frazione di Leibniz. L'ordine è l'esponente,
 * lo stesso sopra e sotto: una lettera, un numero da 1 in su, oppure
 * niente, che vuol dire 1. La funzione è il term che segue il segno, come
 * per il corpo di una somma. Ritorna 1 se text ha questa forma; altrimenti
 * ritorna 0 e syntax_error dice perché. */
static int parse_derivative_statement(const char *text, DerivativeStatement *out) {
    const char *shape = "il segno di derivata si scrive \\frac{d^{n}}{dx^{n}}, con lo stesso ordine sopra e sotto";
    Node *above = NULL, *below = NULL;
    memset(out, 0, sizeof *out);
    syntax_error = NULL;
    cursor = text;
    next_token();
    if (!is_command("frac") && !is_command("dfrac") && !is_command("tfrac")) syntax(shape);
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
    out->variable = token.ch;
    next_token();
    if (is_char('^')) { next_token(); below = parse_group(); }
    expect_char('}', shape);

    /* L'ordine: niente, una lettera o un numero, uguale nei due posti. */
    out->times = 1;
    if (!above != !below) syntax(shape);
    else if (above && above->kind == N_VAR && below->kind == N_VAR && above->name == below->name) out->order = above->name;
    else if (above && above->kind == N_NUM && below->kind == N_NUM && above->value == below->value && above->value >= 1) out->times = above->value;
    else if (above) syntax(shape);

    /* La funzione, il segno di uguale, la formula. */
    out->function = parse_term();
    expect_char('=', "dopo la funzione da derivare serve = e la formula della derivata");
    out->formula = parse_expr();
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
            if (token.kind != T_LETTER || !isupper(token.ch) || vars == MAX_SET_VARS) {
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

        /* Un quantificatore per lettera, dall'ultima alla prima. */
        SetNode *node = set_negation();
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
    default:
        n->type = formulas ? TY_FORMULA : TY_ERROR; break;
    }
    return n->type;
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
 * grammatica e un albero suoi: sono in fondo al capitolo, nel paragrafo 17o.
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
 * Warning: se aggiungi un tipo, aggiungi il ramo che lo costruisce nel
 * lettore di questo paragrafo e i suoi casi nel capitolo 28: in
 * model_holds(), in known_open(), in truth(), in put_sentence() e in
 * cert_sentence(), e la sua parola al lettore del verificatore (capitolo 11
 * di checker.c). */
typedef enum {
    S_TRUE,                         /* una dichiarazione: non afferma nulla */
    S_EQUAL,                        /* left = right */
    S_HOLDS,                        /* left, una parte o una relazione
                                       applicata, vale */
    S_NOT,                          /* non a */
    S_AND, S_OR, S_IMPLIES, S_IFF,  /* connettivi tra a e b */
    S_FORALL, S_EXISTS              /* per ogni / esiste un elemento, la
                                       variabile di posto slot: a */
} SentenceKind;

/* Un nodo dell'albero di un enunciato su insiemi qualsiasi: una condizione.
 * I campi non usati dal tipo sono NULL o zero. I nodi vivono nell'arena. */
typedef struct Sentence Sentence;
struct Sentence {
    SentenceKind kind;
    Elem *left, *right;       /* S_EQUAL: i due membri. S_HOLDS: left */
    Sentence *a, *b;          /* le condizioni componenti */
    int slot;                 /* S_FORALL, S_EXISTS: la variabile legata */
    const char *start, *end;  /* com'è scritto nell'enunciato: [start, end) */
};

/* Le variabili quantificate dell'enunciato in corso. Ogni quantificatore ha
 * un posto suo, anche se due usano la stessa lettera: così nel capitolo 28
 * una variabile si riconosce dal posto. slot_sort[i] è l'insieme in cui
 * varia, o -1 finché un quantificatore senza insieme non lo ha imparato
 * dall'uso. */
static int slot_letter[MAX_SLOTS], slot_sort[MAX_SLOTS];
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

/* Ritorna il posto della variabile visibile di lettera letter, o -1. */
static int visible_find(int letter) {
    for (int i = visible_count - 1; i >= 0; i--)
        if (slot_letter[visible[i]] == letter) return visible[i];
    return -1;
}

/* Registra symbol tra i simboli dell'enunciato. Un nome già preso, da un
 * simbolo, da un insieme o da una variabile in vista, è un errore. */
static void symbol_add(Symbol symbol) {
    int taken = symbol_find(symbol.name, symbol.index) >= 0 || (!symbol.index && visible_find(symbol.name) >= 0);
    for (int i = 0; i < sort_count; i++) taken = taken || (!symbol.index && sort_name[i] == symbol.name);
    if (taken) syntax("la stessa lettera ha due significati nello stesso enunciato");
    else if (symbol_count == MAX_SYMBOLS) syntax("troppi simboli dichiarati in un enunciato");
    else symbols[symbol_count++] = symbol;
}

/* name: legge un nome, con il suo pedice, e lo supera. Ritorna il nome
 * interno e mette il pedice in *index; ritorna 0, senza leggere nulla, se il
 * token corrente non è un nome. */
static int abstract_name(int *index) {
    int name = token.kind == T_LETTER ? token.ch : 0;
    *index = 0;
    for (size_t i = 0; !name && i < sizeof greek_letters / sizeof *greek_letters; i++)
        if (is_command(greek_letters[i])) name = GREEK_NAMES + (int)i;
    if (!name) return 0;
    next_token();
    if (!is_char('_')) return name;

    /* Il pedice: una cifra, con o senza graffe. */
    next_token();
    int braces = is_char('{');
    if (braces) next_token();
    if (token.kind != T_NUM || token.value < 1 || token.value > 9) syntax("come pedice di un nome è riconosciuta una cifra da 1 a 9");
    *index = (int)token.value;
    next_token();
    if (braces) expect_char('}', "manca }");
    return name;
}

/* Ritorna il posto in sort_name[] dell'insieme di lettera letter,
 * registrandolo se è nuovo. Una lettera che non può indicare un insieme è un
 * errore. */
static int sort_index(int letter) {
    if (letter >= 128 || !isupper(letter) || symbol_find(letter, 0) >= 0) {
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
 * posto in sort_name[]. */
static int abstract_sort(void) {
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
    *s = (Sentence){kind, NULL, NULL, a, b, 0, start, consumed};
    return s;
}

static Elem *abstract_term(void);
static Sentence *abstract_formula(void);
static Sentence *abstract_unary(void);

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
    int slot = name && !index ? visible_find(name) : -1, symbol = name ? symbol_find(name, index) : -1;
    if (slot >= 0) return elem_new(-1, slot, NULL, NULL);
    if (symbol < 0 || symbols[symbol].range == TRUTH) {
        syntax(name ? "una lettera non è né quantificata né dichiarata come elemento o applicazione"
                    : "serve un elemento: una lettera, un'applicazione f(x), o un'operazione tra parentesi");
        return elem_new(-1, 0, NULL, NULL);
    }

    /* Gli argomenti, ognuno nel suo insieme. */
    Elem *e = elem_new(symbol, 0, NULL, NULL);
    if (symbols[symbol].arity == 0) return e;
    if (token.kind == T_OPEN) next_token(); else syntax("un'applicazione dichiarata va scritta con i suoi argomenti, f(x)");
    for (int i = 0; i < symbols[symbol].arity && !syntax_error; i++) {
        if (i) expect_char(',', "tra i due argomenti di un'applicazione serve una virgola");
        e->arg[i] = abstract_term();
        elem_expect(e->arg[i], symbols[symbol].domain[i]);
    }
    if (token.kind == T_CLOSE) next_token(); else syntax("manca la parentesi chiusa");
    return e;
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

/* side := term | '(' term ',' term ')'
 *
 * Mette in out[] l'elemento, o le due componenti della coppia, e ritorna
 * quanti sono; 0 se ciò che segue non è né l'uno né l'altra. Una parentesi
 * aperta può cominciare una coppia o un elemento, (a \perp b) \perp c: si
 * prova la coppia e, se dopo il primo elemento non c'è la virgola, si torna
 * indietro. */
static int abstract_side(Elem **out) {
    if (token.kind == T_OPEN) {
        ParserState before = parser_save();
        const char *kept = consumed;
        next_token();
        out[0] = abstract_term();
        if (!syntax_error && is_char(',')) {
            next_token();
            out[1] = abstract_term();
            if (token.kind == T_CLOSE) next_token(); else syntax("manca la parentesi chiusa di una coppia");
            return syntax_error ? 0 : 2;
        }
        parser_restore(before);
        consumed = kept;
    }
    out[0] = abstract_term();
    return syntax_error ? 0 : 1;
}

/* Prova a leggere un confronto o un'appartenenza:
 *
 *     side ('=' | \neq) side        side (\in | \notin) name
 *
 * Ritorna la condizione, o NULL se ciò che segue non ha questa forma: allora
 * il lettore va rimesso dov'era da chi chiama, che decide che altro può
 * essere. start è l'inizio del testo. */
static Sentence *abstract_atom(const char *start) {
    Elem *left[2], *right[2];
    int count = abstract_side(left);
    if (!count) return NULL;

    /* Appartenenza a una parte o a una relazione dichiarata. */
    if (is_command("in") || is_command("notin")) {
        int negative = is_command("notin"), index;
        next_token();
        int name = abstract_name(&index), symbol = name ? symbol_find(name, index) : -1;
        if (symbol < 0 || symbols[symbol].range != TRUTH || symbols[symbol].arity != count) {
            syntax("dopo \\in serve una parte dichiarata, per un elemento, o una relazione dichiarata, per una coppia");
            return NULL;
        }
        for (int i = 0; i < count; i++) elem_expect(left[i], symbols[symbol].domain[i]);
        Sentence *member = sentence_new(S_HOLDS, NULL, NULL, start);
        member->left = elem_new(symbol, 0, left[0], count == 2 ? left[1] : NULL);
        return negative ? sentence_new(S_NOT, member, NULL, start) : member;
    }

    /* Uguaglianza, tra elementi o tra coppie: componente per componente. */
    int distinct = is_command("neq") || is_command("ne");
    if (!distinct && !is_char('=')) return NULL;
    next_token();
    if (abstract_side(right) != count) {
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
    Symbol symbol = {0, 0, 0, {0, 0}, 0};
    symbol.name = operation_name();
    int operation = symbol.name != 0;
    if (operation) next_token(); else symbol.name = abstract_name(&symbol.index);

    /* f \colon S \to T, \perp \colon S \times T \to V. */
    if (symbol.name && is_command("colon")) {
        next_token();
        symbol.domain[0] = abstract_sort();
        symbol.arity = 1;
        if (is_command("times")) { next_token(); symbol.domain[1] = abstract_sort(); symbol.arity = 2; }
        if (is_command("to") || is_command("rightarrow") || is_command("longrightarrow")) next_token();
        else syntax("in una dichiarazione f \\colon A \\to B manca \\to");
        symbol.range = abstract_sort();
        if (operation && symbol.arity != 2) syntax("un'operazione si dichiara su un insieme di coppie, A \\times B");
        symbol_add(symbol);
        return sentence_new(S_TRUE, NULL, NULL, start);
    }

    /* X \subseteq S, R \subseteq S \times T. */
    if (symbol.name && !operation && is_command("subseteq")) {
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
            if (slot_count == MAX_SLOTS) { syntax("troppe variabili quantificate in un enunciato"); return sentence_new(S_TRUE, NULL, NULL, start); }
            Elem *some = elem_new(-1, slot_count, NULL, NULL);
            Sentence *body = sentence_new(part >= 0 ? S_HOLDS : S_EQUAL, NULL, NULL, start);
            body->left = part >= 0 ? elem_new(part, 0, some, NULL) : some;
            body->right = part >= 0 ? NULL : some;
            Sentence *exists = sentence_new(S_EXISTS, body, NULL, start);
            exists->slot = slot_count;
            slot_letter[slot_count] = tolower(symbol.name);
            slot_sort[slot_count++] = sort;
            return exists;
        }
    }

    /* u \in S, con u una lettera che non vuol dire ancora nulla: un
     * elemento di S a cui l'enunciato dà un nome. */
    if (plain && islower(symbol.name) && is_command("in") && visible_find(symbol.name) < 0 && symbol_find(symbol.name, 0) < 0) {
        next_token();
        symbol.range = abstract_sort();
        symbol_add(symbol);
        return sentence_new(S_TRUE, NULL, NULL, start);
    }
    parser_restore(before);
    consumed = kept;
    return NULL;
}

/* Ritorna 1 se la condizione s, negata se negated, obbliga la variabile di
 * posto slot a comparire come argomento di una parte o di una relazione:
 * cioè se ogni volta che s (o la sua negazione) è vera, lo è una di quelle
 * appartenenze. Le regole sono quelle di imposes_membership() (17m), e la
 * risposta 0 vuol dire solo "non risulta". */
static int sentence_forces(const Sentence *s, int negated, int slot) {
    switch (s->kind) {
    case S_HOLDS:
        for (int i = 0; i < 2; i++)
            if (s->left->arg[i] && s->left->arg[i]->symbol < 0 && s->left->arg[i]->slot == slot) return !negated;
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
    default:
        return 0;
    }
}

/* Legge un quantificatore, di cui il token corrente è \forall o \exists:
 * le lettere, il loro insieme, la condizione. Vedi le convenzioni in testa
 * al paragrafo per le tre forme dell'insieme. */
static Sentence *abstract_quantifier(const char *start) {
    SentenceKind kind = is_command("forall") ? S_FORALL : S_EXISTS;
    int letter[MAX_BOUND], letters = 0;
    do {
        next_token();  /* supera il quantificatore, poi ogni virgola */
        if (token.kind != T_LETTER || letters == MAX_BOUND) {
            syntax("dopo un quantificatore servono delle lettere separate da virgole, non più di 8");
            break;
        }
        letter[letters++] = token.ch;
        next_token();
    } while (is_char(','));

    /* L'insieme: una parte dichiarata (allora part è il suo simbolo), un
     * insieme qualsiasi, oppure nessuno. Dopo un insieme, ^ e un altro
     * insieme dicono che le lettere sono applicazioni. */
    int sort = -1, part = -1, maps = 0, domain = 0;
    if (is_command("in")) {
        next_token();
        ParserState before = parser_save();
        const char *kept = consumed;
        int index, name = abstract_name(&index);
        part = name ? symbol_find(name, index) : -1;
        if (part >= 0 && symbols[part].range == TRUTH && symbols[part].arity == 1) {
            sort = symbols[part].domain[0];
        } else {
            part = -1;
            parser_restore(before);
            consumed = kept;
            sort = abstract_sort();
        }
        if (part < 0 && is_char('^')) {
            next_token();
            int braces = is_char('{');
            if (braces) next_token();
            domain = abstract_sort();
            if (braces) expect_char('}', "manca }");
            maps = 1;
        }
    }

    /* "Per ogni applicazione": una dichiarazione, e poi la condizione. */
    if (maps) {
        if (kind != S_FORALL || !abstract_positive)
            syntax("\\forall f \\in T^{S} è letto solo in testa alla tesi, dove vale quanto dichiarare f; un'applicazione da cercare non è trattata");
        for (int i = 0; i < letters && !syntax_error; i++) symbol_add((Symbol){letter[i], 0, 1, {domain, 0}, sort});
        map_quantifiers++;
        return abstract_unary();
    }

    /* Le variabili, in vista mentre si legge la condizione. */
    int first = slot_count, outer = visible_count, positive = abstract_positive;
    for (int i = 0; i < letters && !syntax_error; i++) {
        if (visible_find(letter[i]) >= 0 || symbol_find(letter[i], 0) >= 0) { syntax("la stessa lettera ha due significati nello stesso punto dell'enunciato"); break; }
        if (slot_count == MAX_SLOTS) { syntax("troppe variabili quantificate in un enunciato"); break; }
        slot_letter[slot_count] = letter[i];
        slot_sort[slot_count] = sort;
        visible[visible_count++] = slot_count++;
    }
    if (kind == S_EXISTS) abstract_positive = 0;
    Sentence *node = abstract_unary();
    abstract_positive = positive;
    visible_count = outer;
    if (syntax_error) return node;

    /* Senza insieme: ogni lettera deve averlo imparato dall'uso, e la
     * condizione deve obbligarla a stare lì (per un "per ogni" deve farlo
     * la sua negazione, come in 17m). */
    for (int i = 0; i < letters && sort < 0; i++)
        if (slot_sort[first + i] < 0 || !sentence_forces(node, kind == S_FORALL, first + i))
            syntax("un quantificatore senza insieme è ammesso solo se la condizione obbliga la lettera a stare in un insieme, con un'appartenenza a una parte o a una relazione dichiarata");

    /* \in X, con X una parte: la forma svolta. Poi un nodo per lettera,
     * dall'ultima alla prima. */
    for (int i = letters - 1; i >= 0 && part >= 0; i--) {
        Sentence *member = sentence_new(S_HOLDS, NULL, NULL, start);
        member->left = elem_new(part, 0, elem_new(-1, first + i, NULL, NULL), NULL);
        node = sentence_new(kind == S_FORALL ? S_IMPLIES : S_AND, member, node, start);
    }
    for (int i = letters - 1; i >= 0; i--) {
        node = sentence_new(kind, node, NULL, start);
        node->slot = first + i;
    }
    return node;
}

/* unary: vedi la grammatica in testa al paragrafo. */
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
    if (is_command("forall") || is_command("exists")) return abstract_quantifier(start);
    Sentence *declared = abstract_declaration(start);
    if (declared) return declared;

    /* Un confronto o un'appartenenza. Anche loro possono cominciare con una
     * parentesi: se la lettura non riesce si torna indietro, e la parentesi
     * deve aprire una formula. Il tentativo può aver dato un insieme a una
     * variabile che non lo aveva: anche quello si rimette com'era. */
    int sorts_before[MAX_SLOTS];
    ParserState before = parser_save();
    const char *kept = consumed;
    memcpy(sorts_before, slot_sort, sizeof sorts_before);
    Sentence *atom = abstract_atom(start);
    if (atom && !syntax_error) return atom;
    const char *problem = syntax_error;
    parser_restore(before);
    consumed = kept;
    memcpy(slot_sort, sorts_before, sizeof sorts_before);
    if (token.kind == T_OPEN) {
        next_token();
        Sentence *inner = abstract_formula();
        if (token.kind == T_CLOSE) next_token(); else syntax("manca la parentesi chiusa");

        /* Se nemmeno così si legge, l'errore più preciso è quello
         * incontrato leggendo il confronto. */
        if (syntax_error && problem) syntax_error = problem;
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
    if (map_quantifiers != before) syntax("\\forall f \\in T^{S} è letto solo in testa alla tesi, non tra le ipotesi");
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
        if (map_quantifiers != before) syntax("\\forall f \\in T^{S} è letto solo in testa alla tesi, non in un'equivalenza");
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
    symbol_count = sort_count = slot_count = visible_count = 0;
    abstract_positive = 1;
    abstract_example = map_quantifiers = 0;

    /* In testa, gli "esiste" di un esempio: insiemi, parti e relazioni,
     * un'applicazione. Un "esiste" di un altro tipo è già parte della
     * condizione: si torna indietro e lo legge lei. */
    while (is_command("exists") && !syntax_error) {
        ParserState before = parser_save();
        const char *kept = consumed;
        Symbol shape = {0, 0, 1, {0, 0}, TRUTH};
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
    int misplaced = root->kind == S_IMPLIES && !abstract_example ? declaration_misplaced(root->a, 1) || declaration_misplaced(root->b, 0)
                                                                  : declaration_misplaced(root, 0);
    if (misplaced) syntax("un'applicazione, un'operazione, una parte o un elemento si dichiarano solo tra le ipotesi, in cima all'enunciato");
    return root;
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

