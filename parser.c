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
 * Tre notazioni, tre alberi, una sola tecnica (discesa ricorsiva: una
 * funzione per regola della grammatica):
 *
 *   - le espressioni, con somme, prodotti, potenze e funzioni: Node
 *     (capitoli 8 e 9);
 *   - gli enunciati sugli insiemi: SetNode (capitolo 14);
 *   - gli enunciati con quantificatori, gli insiemi numerici e gli insiemi
 *     di soluzioni: Formula (capitolo 17), le cui foglie sono espressioni.
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
 *                  | \{ lettera \in numbers (\mid | ':') formula \}
 *     formula     := implication { \iff implication }
 *     implication := disjunction [ \implies implication ]
 *     disjunction := conjunction { \lor conjunction }
 *     conjunction := unary { \land unary }
 *     unary       := \lnot unary
 *                  | (\forall | \exists) lettera { ',' lettera } \in numbers unary
 *                  | lettera \colon numbers \to numbers
 *                  | \begin{cases} formula { \\ formula } \end{cases}
 *                  | expr relation expr { relation expr }
 *                  | '(' formula ')'
 *     relation    := '=' | \neq | '<' | '>' | \le | \ge
 *     numbers     := (\mathbb{N} | \mathbb{N}_{0} | \mathbb{Z} | \mathbb{Q} | \mathbb{Q}^{+}
 *                     | \mathbb{R} | \mathbb{R}^{+})
 *                    { \setminus \{ numero { ',' numero } \} }
 *
 * expr è l'espressione del capitolo 9, che riconosce anche f(x) quando f è
 * un'applicazione dichiarata.
 *
 * Tre scritture dei libri di scuola. Una catena di confronti, a < b \le c,
 * dice a < b e b \le c. Un sistema, \begin{cases} ... \end{cases}, è la
 * congiunzione delle sue righe. E \{x \in \mathbb{R} \mid c\}, l'insieme
 * delle soluzioni di c, non afferma nulla: chiede per quali x vale c, e la
 * risposta del motore è un verdetto RISOLTO. Per questo sta solo in cima a un
 * enunciato (17h).
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
    F_LESS, F_LESS_EQ,                /* left < right, left <= right: solo con la
                                         tabella dei segni (18p) */
    F_AND, F_OR, F_IMPLIES, F_IFF,    /* connettivi tra a e b */
    F_NOT,                            /* non a */
    F_FORALL, F_EXISTS,               /* per ogni / esiste variable in numbers: a */
    F_SOLVE,                          /* \{variable \in numbers \mid a\}: non è una
                                         condizione ma una domanda, "per quali
                                         valori vale a?", e sta solo in cima a
                                         un enunciato (17h) */
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
    Node *left, *right;       /* i confronti (da F_EQUAL a F_LESS_EQ): i due membri */
    int swapped;              /* F_LESS, F_LESS_EQ: 1 se nell'enunciato c'era > o
                                 \ge, e quindi left è il membro scritto a destra.
                                 Serve solo a chi deve mostrare i membri com'erano
                                 scritti (il grafico, 18q) */
    Formula *a, *b;           /* le condizioni componenti */
    int variable;             /* F_FORALL, F_EXISTS, F_SOLVE: la lettera legata */
    NumberSet numbers;        /* F_FORALL, F_EXISTS, F_SOLVE: l'insieme su cui varia */
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
    Formula *chain = NULL;
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

/* 17h. statement := formula | \{ lettera \in insieme (\mid | :) formula \}
 *
 * La seconda forma è l'insieme delle soluzioni di una condizione, come
 * \{x \in \mathbb{R} \mid \frac{x - 2}{x + 3} \ge 0\}. Chi la scrive non
 * afferma nulla: chiede per quali valori la condizione vale, e la risposta è
 * un verdetto RISOLTO. Per questo sta solo in cima: dentro una formula una
 * \{ resta un errore di sintassi. */
static Formula *logic_statement(void) {
    const char *start = token.start;
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
        /* La lettera e, quando nessun quantificatore la lega, è il numero di
         * Nepero. Per le regole che lo trattano come una lettera qualsiasi
         * non cambia nulla: ciò che dimostrano vale per ogni valore, e ciò
         * che smentiscono è un polinomio a coefficienti razionali che non
         * si annulla, e in e non si annulla perché e è trascendente. */
        if (node->name != VAR_PI && node->name != 'e' && !scope_find(node->name)) fail("negli enunciati con quantificatori ogni lettera deve essere quantificata");
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
    case F_EQUAL: case F_DISTINCT: case F_LESS: case F_LESS_EQ:
        term_check(f->left);
        term_check(f->right);
        return;
    case F_MAP:
        fail("un'applicazione si può dichiarare solo tra le ipotesi");
        return;
    case F_FORALL: case F_EXISTS: case F_SOLVE:
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

