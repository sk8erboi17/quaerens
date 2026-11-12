r"""Esponenziali e logaritmi: controllo dei dati presi dal libro e fuzzing differenziale.

Uso:
    python3 -I fuzz_exponentials_logarithms.py check-data <exercises.jsonl>
    python3 -I fuzz_exponentials_logarithms.py fuzz <eseguibile> <seme> <quanti> <cartella di lavoro>

A COSA SERVE

Il programma impara le equazioni e le disequazioni esponenziali e logaritmiche
e i campi di esistenza. Questo script è il secondo parere: non chiama mai il
programma per sapere che cosa è vero.

  check-data  controlla i DATI, senza il programma. Ogni riga del file è un
      esercizio di un libro: la richiesta di soluzioni (`solve`), il risultato
      stampato (`expected`), l'enunciato "per ogni x: condizione <=>
      risultato" (`check`) e alcune varianti false con un testimone
      (`false_checks`). Lo script calcola condizione e risultato in molti
      punti e segnala ogni punto in cui sono diversi; poi verifica che ogni
      variante falsa sia davvero diversa nel suo testimone.

  fuzz  genera enunciati di cui la verità è nota PER COSTRUZIONE, li scrive in
      <cartella>/fuzz_exponentials_logarithms_<seme>.tex, li dà al programma e
      confronta le risposte. Accanto scrive un file .expected con ciò che
      l'oracolo si aspetta, enunciato per enunciato.

L'ORACOLO NUMERICO (usato da check-data, e da fuzz come controllo di sé)

Un lettore suo del LaTeX (la grammatica è quella scritta nei commenti dei
capitoli 9 e 17 di parser.c, ridotta a ciò che serve qui: una sola variabile
x, la lettera e per il numero di Nepero, \ln e \log_{b}, radici, frazioni,
valori assoluti, potenze) e un calcolatore che sostituisce un numero al posto
di x nell'enunciato com'è scritto, senza portarlo in un'altra forma.

Precisione garantita. Ogni valore è un intervallo con gli estremi `decimal`,
arrotondati verso l'esterno a ogni operazione: l'intervallo contiene sempre il
numero vero. Somme, prodotti e quozienti usano l'arrotondamento per difetto e
per eccesso; exp, ln e sqrt di `decimal` sono arrotondati correttamente, e
l'intervallo si allarga di due unità sull'ultima cifra per parte. Si calcola
prima con 50 cifre e, se un segno non si decide, con 110. Un segno è certo
quando l'intervallo non contiene lo zero. Se con 110 cifre lo contiene ancora,
ed è tutto dentro +-10^-45 (per un confronto: 10^-45 volte il più grande dei
due membri), il valore è trattato come zero: è un PAREGGIO. Un pareggio è
atteso in un punto speciale (una soluzione stampata, il bordo del campo di
esistenza); altrove è segnalato come SUSPECT. Se l'intervallo contiene lo
zero ma è più largo, il punto è SUSPECT senza verdetto.

Che cosa vuol dire "non ha valore" (la convenzione dei libri: un confronto è
falso dove uno dei suoi membri non ha valore):

  - una frazione dove il denominatore è zero;
  - un logaritmo dove l'argomento non è positivo;
  - una radice di indice pari dove il radicando è negativo; l'indice deve
    essere un intero da 2 in su, anche quando contiene x (esercizio 237);
  - una potenza con esponente intero scritto in cifre vale per ogni base
    (non zero se l'esponente è negativo); con qualunque altro esponente, in
    particolare uno che contiene x, la base deve essere positiva: è così che
    il libro intende f(x)^{g(x)} (esercizio 315).

DOVE GUARDA check-data

  - una griglia: i multipli di 1/16 tra -12 e 12, gli interi fino a +-130,
    e +-1000;
  - i punti speciali: i numeri con cui x è confrontata nel risultato, e i
    punti in cui cambia il segno di qualcosa nella condizione (un confronto,
    un denominatore, l'argomento di un logaritmo, un radicando). Questi ultimi
    non sono calcolati con una formula: tra due punti consecutivi della
    griglia in cui i segni sono diversi si cerca per bisezione, fino a
    un'ampiezza di 10^-70, e il punto speciale è l'intervallo trovato;
  - accanto a ogni punto speciale i punti a distanza 10^-3, 10^-9 e 10^-20, e
    un punto tra due speciali consecutivi.

Che cosa conferma, e che cosa no. Conferma che condizione e risultato hanno
la stessa verità in tutti i punti provati, compresi quelli in cui può
cambiare. Non è una dimostrazione: un punto in cui un confronto è solo
sfiorato (una radice doppia) è visto solo se è nella griglia o è scritto nel
risultato, e due cambi di segno tra due punti consecutivi della griglia che
si annullano a vicenda non sono visti.

Le righe di check-data: DIFFER (condizione e risultato diversi: il risultato
stampato non è confermato), DATA (il file è incoerente: `solve` e `check` non
hanno la stessa condizione, una variante falsa non è diversa nel suo
testimone, ...), SUSPECT, NOTE.

L'ORACOLO PER COSTRUZIONE (fuzz)

Ogni generatore costruisce la condizione a partire dalle sue soluzioni, con
conti esatti sulle frazioni. Un caso è una lista di punti di rottura in x e
la verità in ogni cella (ogni punto, ogni intervallo tra due punti).

  (i) sostituzione t = b^{px+q}. La condizione è N(t)/D(t) ? 0, con N e D
      prodotti di fattori (t - c) scelti dal generatore: il segno in t è
      costante tra due radici consecutive e si calcola con le frazioni. Poi
      t = b^{px+q} è positivo e monotono in x (cresce se p ln b > 0), quindi
      le celle in x sono le celle in t > 0, nello stesso ordine o
      rovesciate; la radice c > 0 cade in x = (log_b c - q)/p, che è
      razionale quando c è una potenza di b e altrimenti si scrive con un
      logaritmo. Il testo può essere scritto sviluppato (9^x + 9 = 10 * 3^x):
      vale (b^k)^E = b^{kE}, spostare un termine da un membro all'altro non
      cambia il confronto, e dividere per t^m > 0 nemmeno.
 (ii) sostituzione t = \log_b x. Come sopra, ma t prende tutti i valori reali
      per x > 0, la radice c cade in x = b^c, e per x <= 0 la condizione non
      ha valore.
(iii) monomi, b^{E(x)} ? b^{F(x)}. b^u è strettamente monotono in u: il
      confronto equivale a E ? F (rovesciato se b < 1) dove E e F sono
      definiti. E - F è costruito dalle sue radici e dai suoi poli, razionali.
 (iv) somme di logaritmi, \log_b f + \log_b g ? c (e differenze). Hanno
      valore solo per f > 0 e g > 0; lì equivalgono a f g ? b^c (rovesciato se
      b < 1), per le proprietà dei logaritmi. I coefficienti sono scelti in
      modo che f g - b^c abbia radici razionali, e che il campo di esistenza
      tolga qualcosa alle soluzioni del solo polinomio.
  (v) campi di esistenza, \{x \mid E = E\}. E è costruita da pezzi di cui il
      generatore conosce il segno esatto in ogni razionale, senza calcolare
      logaritmi: \log_b(L) - k ha il segno di L - b^k (rovesciato se b < 1),
      b^L - b^k quello di L - k, \sqrt{L} - k quello di L - k^2, dove L è
      a x + c oppure |a x + c|. Radici, logaritmi e reciproci di questi pezzi
      (e dei loro prodotti e quozienti) hanno valore dove il segno lo
      permette.

Controllo di sé. Ogni condizione generata viene anche riletta dall'oracolo
numerico, dal testo, e confrontata con le celle: sono due strade diverse verso
la stessa verità. Se non coincidono lo script stampa ORACLE (è un errore di
questo script, non del programma) e scarta il caso.

Dalle soluzioni escono tre tipi di enunciato: la richiesta \{x \mid ...\},
l'enunciato vero "condizione <=> soluzioni" e uno guastato (un estremo
spostato, un < per un <=, un pezzo tolto), di cui la verità è ricalcolata. Le
soluzioni che il programma scrive sono rilette da questo lettore (i logaritmi
di numeri sono calcolati) e confrontate con le celle in ogni punto di rottura,
in ogni estremo scritto dal programma e tra un punto e l'altro.

Segnala BUG quando il programma dimostra un enunciato falso, dichiara falso
un enunciato vero, o scrive soluzioni diverse da quelle dell'oracolo. SUSPECT
quando non riesce a leggere le soluzioni scritte dal programma. NOTE, una
riga per generatore e per motivo, per gli enunciati che il programma non
decide.
"""
import json
import os
import random
import re
import subprocess
import sys
from decimal import (MAX_EMAX, MIN_EMIN, ROUND_CEILING, ROUND_FLOOR, ROUND_HALF_EVEN, Context, Decimal,
                     DivisionByZero, InvalidOperation)
from fractions import Fraction
from math import gcd

rnd = random.Random()

ZERO, ONE = Decimal(0), Decimal(1)
DIGITS = (50, 110)           # le cifre dei due tentativi di calcolo
CONFIRM = 1000               # le cifre con cui si conferma un pareggio in un numero esatto
TIE = Decimal("1e-45")       # sotto questa soglia un valore che non si separa dallo zero è un pareggio
WIDTH = Decimal("1e-70")     # l'ampiezza a cui si ferma la bisezione
NUDGE = Decimal("1e-30")     # di quanto ci si sposta da un punto in cui c'è uno zero
BESIDE = (Decimal("1e-3"), Decimal("1e-9"), Decimal("1e-20"))
EXACT = Context(prec=1000)   # per i conti senza arrotondamento sui punti di prova (somme, metà)

NEXISTS, FORALL = "\\nexists x \\in \\mathbb{R}", "\\forall x \\in \\mathbb{R}"


class Syntax(Exception):
    """Il testo non è nel sottoinsieme di LaTeX che questo lettore conosce."""


class Undefined(Exception):
    """Un'espressione non ha valore nel punto (convenzione dei libri)."""


class Imprecise(Exception):
    """Con le cifre a disposizione un segno non si decide, e non è un pareggio."""


# ---------------------------------------------------------------- intervalli
class Arith:
    """Aritmetica degli intervalli con un numero fisso di cifre. Un intervallo è una coppia
    (basso, alto) di Decimal; ogni operazione ritorna un intervallo che contiene tutti i
    risultati possibili, perché arrotonda il basso per difetto e l'alto per eccesso.

    Un numero troppo grande per decimal non ferma il conto: arrotondato per difetto diventa il
    più grande numero scrivibile, per eccesso diventa infinito, e l'intervallo resta giusto. Un
    conto senza senso tra infiniti (infinito meno infinito) solleva InvalidOperation."""

    def __init__(self, digits):
        def context(rounding):
            return Context(prec=digits, rounding=rounding, Emax=MAX_EMAX, Emin=MIN_EMIN,
                           traps=[InvalidOperation, DivisionByZero])
        self.down, self.up, self.near = context(ROUND_FLOOR), context(ROUND_CEILING), context(ROUND_HALF_EVEN)
        self.e = self.exp((ONE, ONE))

    def fraction(self, q):
        n, d = Decimal(q.numerator), Decimal(q.denominator)
        return (self.down.divide(n, d), self.up.divide(n, d))

    def add(self, a, b):
        return (self.down.add(a[0], b[0]), self.up.add(a[1], b[1]))

    def sub(self, a, b):
        return (self.down.subtract(a[0], b[1]), self.up.subtract(a[1], b[0]))

    def mul(self, a, b):
        down, up = self.down.multiply, self.up.multiply
        return (min(down(a[0], b[0]), down(a[0], b[1]), down(a[1], b[0]), down(a[1], b[1])),
                max(up(a[0], b[0]), up(a[0], b[1]), up(a[1], b[0]), up(a[1], b[1])))

    def div(self, a, b):
        """a / b, per un b che non contiene lo zero."""
        down, up = self.down.divide, self.up.divide
        return (min(down(a[0], b[0]), down(a[0], b[1]), down(a[1], b[0]), down(a[1], b[1])),
                max(up(a[0], b[0]), up(a[0], b[1]), up(a[1], b[0]), up(a[1], b[1])))

    def power(self, a, n):
        """a^n per un intero n >= 0, con i quadrati ripetuti."""
        out = (ONE, ONE)
        while n:
            if n & 1:
                out = self.mul(out, a)
            a, n = self.mul(a, a), n >> 1
        return out

    def monotone(self, function, a):
        """Una funzione crescente calcolata sui due estremi. exp, ln e sqrt di decimal sono
        arrotondati correttamente (errore entro mezza unità sull'ultima cifra): allargare di due
        unità per parte dà un intervallo che contiene di certo il valore vero."""
        low = function(a[0])
        high = low if a[0] == a[1] else function(a[1])
        return (self.down.next_minus(self.down.next_minus(low)), self.up.next_plus(self.up.next_plus(high)))

    def exp(self, a):
        low, high = self.monotone(self.near.exp, a)
        return (max(low, ZERO), high)

    def ln(self, a):
        """ln a, per un a tutto positivo."""
        return self.monotone(self.near.ln, a)

    def sqrt(self, a):
        """La radice quadrata di un a senza numeri negativi."""
        low, high = self.monotone(self.near.sqrt, a)
        return (max(low, ZERO), high)


LADDER = [Arith(digits) for digits in DIGITS]
TOP = LADDER[-1]
LONG_LADDER = LADDER + [Arith(CONFIRM)]


def negate(a):
    # copy_negate e copy_abs non arrotondano; gli operatori - e abs() di Decimal sì.
    return (a[1].copy_negate(), a[0].copy_negate())


def compare(a, b):
    """-1 se l'intervallo a sta tutto sotto b, 1 se sta tutto sopra, 0 se si toccano."""
    return 1 if a[0] > b[1] else -1 if a[1] < b[0] else 0


def shift(a, amount):
    """L'intervallo a spostato del numero amount, senza arrotondare."""
    return (EXACT.add(a[0], amount), EXACT.add(a[1], amount))


def describe(a):
    """Un intervallo scritto per una persona."""
    if a[0] == a[1]:
        return "x = %s" % format(a[0], "f")
    return "x ~ %s" % format(TOP.near.create_decimal(a[0]), ".30g")


# ---------------------------------------------------------------- lettore del LaTeX
class Node:
    """Un nodo dell'albero di un'espressione o di una condizione.

    op: "num", "x", "e", "const" (un numero dato come intervallo), "neg", "+", "-", "*", "/",
    "^", "root" (radicando, indice), "abs", "ln", "log" (argomento, base); per le condizioni
    "cmp" (value è la relazione), "and", "or", "not", "iff", "implies", "true", "false",
    "forall", "solve". id è un numero diverso per ogni nodo: identifica i segni che la
    valutazione annota. depends dice se sotto il nodo compare x; exact è il valore come
    frazione, quando il nodo è un numero razionale scritto senza x né funzioni; twin dice che
    un confronto ha i due membri scritti allo stesso modo, come E = E."""
    __slots__ = ("op", "args", "value", "id", "depends", "exact", "twin")
    count = 0

    def __init__(self, op, *args, value=None):
        self.op, self.args, self.value = op, args, value
        Node.count += 1
        self.id = Node.count
        self.depends = op == "x" or any(a.depends for a in args)
        self.exact = fold(self)
        self.twin = op == "cmp" and same(args[0], args[1])


def fold(node):
    """Il valore di node come frazione, o None."""
    if node.op == "num":
        return Fraction(node.value)
    values = [a.exact for a in node.args]
    if not values or None in values:
        return None
    if node.op == "neg":
        return -values[0]
    if node.op == "+":
        return values[0] + values[1]
    if node.op == "-":
        return values[0] - values[1]
    if node.op == "*":
        return values[0] * values[1]
    if node.op == "/":
        return values[0] / values[1] if values[1] else None
    if node.op == "^" and values[1].denominator == 1 and abs(values[1]) <= 64 and (values[0] or values[1] >= 0):
        return values[0] ** int(values[1])
    return None


IGNORED = {"displaystyle", "textstyle", "quad", "qquad", "limits"}
RELATION_COMMANDS = {"neq": "!=", "ne": "!=", "le": "<=", "leq": "<=", "ge": ">=", "geq": ">="}


def tokenize(text):
    """Il testo diviso in token (tipo, valore). \\left( e ( sono lo stesso token; le sbarre del
    valore assoluto restano distinte in apertura e chiusura, e vanno scritte con \\left e \\right."""
    tokens, i = [], 0
    while i < len(text):
        c = text[i]
        if c.isspace():
            i += 1
        elif c.isdigit():
            j = i
            while j < len(text) and text[j].isdigit():
                j += 1
            tokens.append(("num", text[i:j]))
            i = j
        elif c.isalpha():
            tokens.append(("let", c))
            i += 1
        elif c == "\\":
            j = i + 1
            if j < len(text) and not text[j].isalpha():
                if text[j] in ",;!: ":
                    i = j + 1
                    continue
                if text[j] in "{}\\":
                    tokens.append(("esc", text[j]))
                    i = j + 1
                    continue
                raise Syntax("unknown command at: %s" % text[i:i + 12])
            while j < len(text) and text[j].isalpha():
                j += 1
            name, i = text[i + 1:j], j
            if name in ("left", "right"):
                while i < len(text) and text[i].isspace():
                    i += 1
                d = text[i:i + 1]
                i += 1
                if d == "|":
                    tokens.append(("absopen" if name == "left" else "absclose", d))
                elif name == "left" and d in ("(", "["):
                    tokens.append(("open", d))
                elif name == "right" and d in (")", "]"):
                    tokens.append(("close", d))
                else:
                    raise Syntax("unknown delimiter after \\%s" % name)
            elif name not in IGNORED:
                tokens.append(("cmd", name))
        elif c == "(":
            tokens.append(("open", c))
            i += 1
        elif c == ")":
            tokens.append(("close", c))
            i += 1
        else:
            tokens.append(("chr", c))
            i += 1
    tokens.append(("end", ""))
    return tokens


class Reader:
    """Discesa ricorsiva sulla grammatica dei capitoli 9 e 17 di parser.c: una funzione per
    regola. names associa a un nome come "r_{1}" l'intervallo del numero che indica."""

    def __init__(self, text, names=None):
        self.tokens, self.pos, self.names = tokenize(text), 0, names or {}

    def peek(self):
        return self.tokens[self.pos]

    def take(self):
        self.pos += 1
        return self.tokens[self.pos - 1]

    def at(self, kind, value=None):
        token = self.tokens[self.pos]
        return token[0] == kind and (value is None or token[1] == value)

    def expect(self, kind, value=None):
        if not self.at(kind, value):
            raise Syntax("expected %s, found %s" % (value or kind, self.peek()[1] or "the end"))
        return self.take()

    # group := '{' expr '}' | un solo token
    def group(self):
        if self.at("chr", "{"):
            self.take()
            inner = self.expr()
            self.expect("chr", "}")
            return inner
        if self.at("num") and len(self.peek()[1]) > 1:
            # Senza graffe TeX prende una cifra sola: x^12 è x^1 seguito da 2.
            digits = self.peek()[1]
            self.tokens[self.pos] = ("num", digits[1:])
            return Node("num", value=int(digits[0]))
        return self.primary()

    # funzione ['^' group] ['_' group] argomento, con argomento := '(' expr ')' | factor
    def function(self, name):
        exponent = base = None
        while True:
            if self.at("chr", "^") and exponent is None:
                self.take()
                exponent = self.group()
            elif self.at("chr", "_") and name == "log" and base is None:
                self.take()
                base = self.group()
            else:
                break
        if self.at("open"):
            self.take()
            argument = self.expr()
            self.expect("close")
        else:
            argument = self.factor()
        if name == "log" and base is None:
            raise Syntax("\\log needs its base: write \\log_{10} for the base-10 logarithm")
        if base is not None and (base.depends or base.exact is None or base.exact <= 0 or base.exact == 1):
            raise Syntax("the base of a logarithm must be a positive rational number different from 1")
        call = Node("ln", argument) if name == "ln" else Node("log", argument, base)
        return Node("^", call, exponent) if exponent is not None else call

    def primary(self):
        kind, value = self.peek()
        if kind == "num":
            self.take()
            return Node("num", value=int(value))
        if kind == "let":
            self.take()
            if value == "x":
                return Node("x")
            if value == "e":
                return Node("e")
            if value == "r" and self.at("chr", "_"):
                self.take()
                index = self.group()
                name = "r_{%s}" % index.value
                if name not in self.names:
                    raise Syntax("unknown name %s" % name)
                return Node("const", value=self.names[name])
            raise Syntax("unknown letter %s" % value)
        if kind == "open":
            self.take()
            inner = self.expr()
            self.expect("close")
            return inner
        if kind == "absopen":
            self.take()
            inner = self.expr()
            self.expect("absclose")
            return Node("abs", inner)
        if kind == "cmd" and value in ("frac", "dfrac", "tfrac"):
            self.take()
            above = self.group()
            return Node("/", above, self.group())
        if kind == "cmd" and value == "sqrt":
            self.take()
            index = Node("num", value=2)
            if self.at("chr", "["):
                self.take()
                index = self.expr()
                self.expect("chr", "]")
            if index.exact is not None and (index.exact.denominator != 1 or index.exact < 2):
                raise Syntax("the index of a root must be an integer from 2 up")
            return Node("root", self.group(), index)
        if kind == "cmd" and value in ("ln", "log"):
            self.take()
            return self.function(value)
        raise Syntax("unexpected %s" % (value or "end of text"))

    # factor := primary { '^' group }
    def factor(self):
        node = self.primary()
        while self.at("chr", "^"):
            self.take()
            node = Node("^", node, self.group())
        return node

    def starts_factor(self):
        kind, value = self.peek()
        return kind in ("num", "let", "open", "absopen") or (kind == "cmd" and value in ("frac", "dfrac", "tfrac", "sqrt", "ln", "log"))

    # term := factor { [\cdot | '/'] factor }: due fattori accostati si moltiplicano
    def term(self):
        node = self.factor()
        while True:
            if self.at("cmd", "cdot") or self.at("cmd", "times"):
                self.take()
                node = Node("*", node, self.factor())
            elif self.at("chr", "/"):
                self.take()
                node = Node("/", node, self.factor())
            elif self.starts_factor():
                node = Node("*", node, self.factor())
            else:
                return node

    # expr := ['-' | '+'] term { ('+' | '-') term }
    def expr(self):
        if self.at("chr", "-"):
            self.take()
            node = Node("neg", self.term())
        else:
            if self.at("chr", "+"):
                self.take()
            node = self.term()
        while self.at("chr", "+") or self.at("chr", "-"):
            op = self.take()[1]
            node = Node(op, node, self.term())
        return node

    def relation(self):
        kind, value = self.peek()
        if kind == "chr" and value in "=<>" and value:
            return value
        if kind == "cmd" and value in RELATION_COMMANDS:
            return RELATION_COMMANDS[value]
        return None

    # expr relation expr { relation expr }: una catena a < b \le c dice a < b e b \le c
    def comparison(self):
        left, chain = self.expr(), None
        while self.relation():
            relation = self.relation()
            self.take()
            right = self.expr()
            link = Node("cmp", left, right, value=relation)
            chain = link if chain is None else Node("and", chain, link)
            left = right
        if chain is None:
            raise Syntax("a relation is expected after an expression")
        return chain

    def reals(self):
        self.expect("let", "x")
        self.expect("cmd", "in")
        self.expect("cmd", "mathbb")
        self.expect("chr", "{")
        self.expect("let", "R")
        self.expect("chr", "}")

    def unary(self):
        if self.at("cmd", "lnot") or self.at("cmd", "neg"):
            self.take()
            return Node("not", self.unary())
        if self.at("cmd", "forall"):
            self.take()
            self.reals()
            return Node("forall", self.unary())
        # Anche un confronto può cominciare con una parentesi: prima si prova il confronto, e
        # se non riesce si torna indietro e la parentesi deve aprire una condizione.
        start = self.pos
        try:
            return self.comparison()
        except Syntax:
            self.pos = start
            if not self.at("open"):
                raise
        self.take()
        inner = self.formula()
        self.expect("close")
        return inner

    def junction(self, op, commands, lower):
        node = lower()
        while self.at("cmd") and self.peek()[1] in commands:
            self.take()
            node = Node(op, node, lower())
        return node

    def conjunction(self):
        return self.junction("and", ("land", "wedge"), self.unary)

    def disjunction(self):
        return self.junction("or", ("lor", "vee"), self.conjunction)

    def implication(self):
        node = self.disjunction()
        if self.at("cmd") and self.peek()[1] in ("implies", "Rightarrow", "Longrightarrow"):
            self.take()
            node = Node("implies", node, self.implication())
        return node

    def formula(self):
        return self.junction("iff", ("iff", "Leftrightarrow", "Longleftrightarrow"), self.implication)

    # statement := formula | \{ x \in \mathbb{R} (\mid | ':') formula \}
    def statement(self):
        if not self.at("esc", "{"):
            return self.formula()
        self.take()
        self.reals()
        if not (self.at("cmd", "mid") or self.at("chr", ":")):
            raise Syntax("\\mid is missing")
        self.take()
        node = Node("solve", self.formula())
        self.expect("esc", "}")
        return node


def parse(text, rule="statement", names=None):
    """L'albero di text letto con la regola data: "statement", "formula" oppure "expr"."""
    reader = Reader(text, names)
    node = getattr(reader, rule)()
    reader.expect("end")
    return node


def parse_result(text, names=None):
    """Le soluzioni scritte come le scrive il programma: una condizione, oppure "non esiste" / "per ogni"."""
    text = text.strip()
    if text == NEXISTS:
        return Node("false")
    if text == FORALL:
        return Node("true")
    return parse(text, "formula", names)


def split_check(node):
    """Condizione e risultato di un enunciato che conferma un risultato. Le tre forme:
    per ogni x (condizione <=> risultato); per ogni x non (condizione), quando non ci sono
    soluzioni; per ogni x (condizione), quando lo sono tutti i numeri."""
    if node.op != "forall":
        raise Syntax("a check must begin with \\forall x \\in \\mathbb{R}")
    body = node.args[0]
    if body.op == "iff":
        return body.args
    if body.op == "not":
        return body.args[0], Node("false")
    return body, Node("true")


def walk(node):
    yield node
    for a in node.args:
        yield from walk(a)


def same(a, b):
    """True se i due alberi sono scritti allo stesso modo."""
    return a.op == b.op and a.value == b.value and len(a.args) == len(b.args) and all(same(p, q) for p, q in zip(a.args, b.args))


# ---------------------------------------------------------------- valutazione
class Env:
    """Una valutazione in un punto: l'aritmetica, l'intervallo di x, e ciò che si annota strada
    facendo. sig associa all'id di un nodo il segno che lì è stato deciso (di un confronto la
    differenza dei membri, di una frazione il denominatore, di un logaritmo l'argomento, di una
    radice il radicando, di una potenza la base): chi non compare non è stato calcolato. ties
    sono gli id dei segni decisi per pareggio; gap è il più grande scarto relativo tra i due
    membri di un'uguaglianza trovata vera."""

    def __init__(self, arith, x, top):
        self.arith, self.x, self.top = arith, x, top
        self.sig, self.ties, self.gap = {}, [], ZERO

    def sign(self, node, a, scale=ONE):
        if a[0] > 0:
            s = 1
        elif a[1] < 0:
            s = -1
        elif a[0] == 0 and a[1] == 0:
            s = 0
        else:
            # Lo zero è dentro l'intervallo. Solo con tutte le cifre, e solo se l'intervallo è
            # minuscolo, lo si prende per zero.
            if not self.top or not a[0].is_finite() or not a[1].is_finite() or not scale.is_finite():
                raise Imprecise()
            if max(a[0].copy_abs(), a[1]) > self.arith.up.multiply(TIE, scale):
                raise Imprecise()
            s = 0
            self.ties.append(node.id)
        self.sig[node.id] = s
        return s


def magnitude(a, b):
    return max(ONE, a[0].copy_abs(), a[1].copy_abs(), b[0].copy_abs(), b[1].copy_abs())


def value(node, env):
    """L'intervallo che contiene il valore di node nel punto env.x. Solleva Undefined dove node
    non ha valore, Imprecise se un segno che serve non si decide."""
    arith, op = env.arith, node.op
    if node.exact is not None:
        return arith.fraction(node.exact)
    if op == "x":
        return env.x
    if op == "e":
        return arith.e
    if op == "const":
        return node.value
    a = value(node.args[0], env)
    if op == "neg":
        return negate(a)
    if op == "abs":
        return a if a[0] >= 0 else negate(a) if a[1] <= 0 else (ZERO, max(a[0].copy_negate(), a[1]))
    if op == "ln" or op == "log":
        if env.sign(node, a) <= 0:
            raise Undefined()
        return arith.ln(a) if op == "ln" else arith.div(arith.ln(a), arith.ln(arith.fraction(node.args[1].exact)))
    if op == "^":
        return power(node, a, env)
    if op == "root":
        return root(node, a, env)
    b = value(node.args[1], env)
    if op == "+":
        return arith.add(a, b)
    if op == "-":
        return arith.sub(a, b)
    if op == "*":
        return arith.mul(a, b)
    if env.sign(node, b) == 0:
        raise Undefined()
    return arith.div(a, b)


def power(node, base, env):
    """base elevato all'esponente di node. Un esponente intero scritto in cifre è un prodotto
    ripetuto, e vale per ogni base; ogni altro esponente chiede una base positiva, e la potenza
    è exp(esponente * ln base)."""
    arith, exponent = env.arith, node.args[1]
    q = exponent.exact
    if q is not None and q.denominator == 1:
        if q >= 0:
            return arith.power(base, int(q))
        if env.sign(node, base) == 0:
            raise Undefined()
        return arith.div((ONE, ONE), arith.power(base, -int(q)))
    e = value(exponent, env)
    s = env.sign(node, base)
    if s == 0 and q is not None and q > 0:
        return (ZERO, ZERO)
    if s <= 0:
        raise Undefined()
    return arith.exp(arith.mul(e, arith.ln(base)))


def root(node, radicand, env):
    """La radice di radicand con l'indice di node: un intero da 2 in su, anche quando è
    un'espressione in x (allora ha valore solo nei punti in cui l'espressione vale un intero)."""
    arith, index = env.arith, node.args[1]
    if index.exact is not None:
        n = int(index.exact)
    else:
        i = value(index, env)
        n = int(i[0].to_integral_value(ROUND_HALF_EVEN))
        if not (i[0] <= n <= i[1]) or n < 2:
            raise Undefined()
        if i[0] != i[1]:
            if not env.top or EXACT.subtract(i[1], i[0]) > TIE:
                raise Imprecise()
            env.ties.append(node.id)
    s = env.sign(node, radicand)
    if s == 0:
        return (ZERO, ZERO)
    if s < 0 and n % 2 == 0:
        raise Undefined()
    a = radicand if s > 0 else negate(radicand)
    r = arith.sqrt(a) if n == 2 else arith.exp(arith.div(arith.ln(a), (Decimal(n), Decimal(n))))
    return r if s > 0 else negate(r)


RELATIONS = {"<": lambda s: s < 0, "<=": lambda s: s <= 0, ">": lambda s: s > 0, ">=": lambda s: s >= 0,
             "=": lambda s: s == 0, "!=": lambda s: s != 0}


def truth(node, env):
    """La verità della condizione node nel punto env.x. Un confronto è falso dove uno dei suoi
    membri non ha valore. I connettivi calcolano sempre tutti e due i lati: così i segni
    annotati non dipendono da quale lato decide."""
    op = node.op
    if op == "cmp":
        try:
            left = value(node.args[0], env)
            if node.twin:
                # Due membri scritti allo stesso modo sono uguali dove hanno valore: non serve
                # sottrarli, e la differenza non è un segno da seguire.
                return RELATIONS[node.value](0)
            right = value(node.args[1], env)
        except Undefined:
            return False
        scale = magnitude(left, right)
        difference = env.arith.sub(left, right)
        s = env.sign(node, difference, scale)
        if s == 0 and node.value == "=":
            env.gap = max(env.gap, env.arith.up.divide(max(difference[0].copy_abs(), difference[1].copy_abs()), scale))
        return RELATIONS[node.value](s)
    if op == "true" or op == "false":
        return op == "true"
    p = truth(node.args[0], env)
    if op == "not":
        return not p
    q = truth(node.args[1], env)
    return (p and q) if op == "and" else (p or q) if op == "or" else (p == q) if op == "iff" else (not p or q)


def evaluate(formulas, x):
    """La verità di ogni condizione nel punto x (un intervallo), con l'ambiente che l'ha
    calcolata. Prova con poche cifre e poi con di più; solleva Imprecise se non bastano.

    Un pareggio è ammesso solo all'ultimo tentativo. Quando x è un numero esatto l'ultimo
    tentativo ha CONFIRM cifre: due membri che si avvicinano senza toccarsi (1 - 5^{-x} e 1 per
    x grande) vengono così separati, e resta in pareggio solo ciò che coincide per CONFIRM
    cifre. Quando x è un intervallo le cifre in più non servono: l'ampiezza è quella di x."""
    ladder = LONG_LADDER if x[0] == x[1] else LADDER
    for arith in ladder:
        env = Env(arith, x, arith is ladder[-1])
        try:
            return [truth(f, env) for f in formulas], env
        except Imprecise:
            continue
        except InvalidOperation:
            break
    raise Imprecise()


def constant(node):
    """L'intervallo del numero scritto da node, che non contiene x."""
    if node.depends:
        raise Syntax("a number is expected, without x")
    try:
        return value(node, Env(TOP, None, True))
    except (Undefined, Imprecise):
        raise Syntax("a number has no value, or cannot be computed")


def printed_numbers(formula):
    """I numeri con cui x è confrontata in formula: gli estremi scritti in un risultato."""
    out = []
    for node in walk(formula):
        if node.op == "cmp":
            for side, other in ((0, 1), (1, 0)):
                if node.args[side].op == "x" and not node.args[other].depends:
                    out.append(constant(node.args[other]))
    return out


def distinct(formulas):
    """Le condizioni di formulas senza quelle scritte allo stesso modo di una precedente, e per
    ognuna di formulas l'indice di chi la rappresenta: serve a non calcolare due volte la stessa."""
    kept, index = [], []
    for f in formulas:
        match = [i for i, g in enumerate(kept) if same(f, g)]
        index.append(match[0] if match else len(kept))
        if not match:
            kept.append(f)
    return kept, index


def arrange(intervals):
    """Gli intervalli in ordine, tenendone uno solo (il più stretto) di quelli che si toccano."""
    out = []
    for a in sorted(intervals, key=lambda a: (a[0], a[1])):
        if out and compare(out[-1], a) == 0:
            if EXACT.subtract(a[1], a[0]) < EXACT.subtract(out[-1][1], out[-1][0]):
                out[-1] = a
        else:
            out.append(a)
    return out


def between(specials):
    """Un punto prima del primo intervallo, uno tra due consecutivi, uno dopo l'ultimo."""
    if not specials:
        return [(ZERO, ZERO)]
    out = [shift(specials[0], -1)]
    for a, b in zip(specials, specials[1:]):
        middle = EXACT.divide(EXACT.add(a[1], b[0]), 2)
        out.append((middle, middle))
    out.append(shift(specials[-1], 1))
    return out


# ---------------------------------------------------------------- check-data: i punti
def grid():
    """La griglia: i multipli di 1/16 tra -12 e 12, gli interi fino a +-130, e +-300."""
    points = [Decimal(k) / 16 for k in range(-192, 193)]
    points += [Decimal(s * k) for k in range(13, 131) for s in (1, -1)] + [Decimal(300), Decimal(-300)]
    return sorted(points)


def signature(condition, ids, x):
    """I segni annotati valutando condition nel numero x, nell'ordine di ids: None per chi non è
    stato calcolato. Un Imprecise diventa una firma a sé: il chiamante la vede diversa dalle
    vicine e ci arriva con la bisezione, dove poi il punto è segnalato."""
    try:
        env = evaluate([condition], (x, x))[1]
    except Imprecise:
        return ("imprecise",)
    return tuple(env.sig.get(i) for i in ids)


def changes(condition, ids, low, high, sig_low, sig_high, found):
    """Aggiunge a found i punti tra low e high in cui cambia un segno di condition: intervalli
    larghi al più WIDTH, oppure un numero solo quando la bisezione cade su uno zero.

    Tutto ciò che può far cambiare verità a una condizione di questo linguaggio passa per lo
    zero di una quantità annotata: un confronto cambia dove i membri si incontrano o dove uno
    smette di avere valore, e un'espressione smette di avere valore dove un denominatore, un
    argomento o un radicando passa per lo zero. Per questo basta seguire i segni."""
    if sig_low == sig_high:
        return
    if 0 in sig_low:
        # low è già tra i punti trovati: si riparte da poco più in là.
        low = EXACT.add(low, NUDGE)
        sig_low = signature(condition, ids, low)
    if 0 in sig_high:
        high = EXACT.subtract(high, NUDGE)
        sig_high = signature(condition, ids, high)
    if low >= high or sig_low == sig_high or 0 in sig_low or 0 in sig_high:
        return
    if EXACT.subtract(high, low) <= WIDTH:
        found.append((low, high))
        return
    middle = EXACT.divide(EXACT.add(low, high), 2)
    sig_middle = signature(condition, ids, middle)
    if 0 in sig_middle:
        found.append((middle, middle))
    changes(condition, ids, low, middle, sig_low, sig_middle, found)
    changes(condition, ids, middle, high, sig_middle, sig_high, found)


def special_points(condition, points):
    """I punti in cui cambia un segno di condition, cercati tra i numeri di points (in ordine)."""
    ids = [n.id for n in walk(condition) if n.op in ("cmp", "/", "^", "root", "ln", "log")]
    sigs = [signature(condition, ids, p) for p in points]
    found = [(p, p) for p, s in zip(points, sigs) if 0 in s]
    for i in range(len(points) - 1):
        changes(condition, ids, points[i], points[i + 1], sigs[i], sigs[i + 1], found)
    return found


# ---------------------------------------------------------------- check-data: un esercizio
class Report:
    """Ciò che check-data ha da dire di un esercizio: le righe da stampare e i conti."""

    def __init__(self, number):
        self.number, self.lines = number, []
        self.points = self.solutions = self.variants = 0
        self.differs = self.broken = False
        self.gap = ZERO

    def say(self, label, text):
        self.lines.append("%s: exercise %s: %s" % (label, self.number, text))


def runs(points, flags):
    """Raggruppa i punti consecutivi con flag vero: ritorna (primo, ultimo, quanti) per gruppo."""
    out, start = [], None
    for i, flag in enumerate(flags + [False]):
        if flag and start is None:
            start = i
        elif not flag and start is not None:
            out.append((points[start], points[i - 1], i - start))
            start = None
    return out


def check_exercise(ex, report):
    solve = parse(ex["solve"])
    if solve.op != "solve":
        raise Syntax("solve is not a request \\{x \\in \\mathbb{R} \\mid ...\\}")
    asked = solve.args[0]
    condition, result = split_check(parse(ex["check"]))
    expected = parse_result(ex["expected"])
    confirmed = ex.get("book_confirmed", True)
    reference = result
    extra = []
    if not confirmed:
        # Il risultato stampato è dichiarato sbagliato: i dati portano quello corretto, che deve
        # reggere al posto suo, e le varianti false sono misurate su quello.
        fixed_condition, reference = split_check(parse(ex["corrected_check"]))
        extra = [fixed_condition, reference, parse_result(ex["corrected_expected"])]
    if ex["kind"] == "domain" and not (asked.op == "cmp" and asked.value == "=" and same(asked.args[0], asked.args[1])):
        report.say("DATA", "a domain exercise must be written E = E")

    # I punti: la griglia, i punti speciali, i loro vicini, e un punto tra due speciali.
    numbers = grid()
    printed = printed_numbers(result) + printed_numbers(expected) + printed_numbers(reference)
    specials = arrange(printed + special_points(condition, numbers))
    points = [((n, n), describe((n, n)), "grid") for n in numbers]
    for s in specials:
        if s[0] != s[1] or s[0] not in numbers:
            points.append((s, describe(s), "special"))
        for d in BESIDE:
            points.append((shift(s, -d), "%s - %s" % (describe(s), d), "beside"))
            points.append((shift(s, d), "%s + %s" % (describe(s), d), "beside"))
    for m in between(specials)[1:-1]:
        if m[0] not in numbers:
            points.append((m, describe(m), "between"))
    points.sort(key=lambda p: (p[0][0], p[0][1]))
    report.points = len(points)

    # Condizione e risultato in ogni punto.
    formulas, index = distinct([condition, result, asked, expected] + extra)
    differ, mismatch = [], {"solve": 0, "expected": 0, "corrected": 0}
    for x, label, kind in points:
        try:
            values, env = evaluate(formulas, x)
            values = [values[i] for i in index]
        except Imprecise:
            report.say("SUSPECT", "cannot decide a sign at %s" % label)
            differ.append(False)
            continue
        if env.ties and kind in ("beside", "between"):
            report.say("SUSPECT", "two sides agree to 1e-45 at %s, which is not a special point" % label)
        differ.append(values[0] != values[1])
        mismatch["solve"] += values[2] != values[0]
        mismatch["expected"] += values[3] != values[1]
        if extra:
            mismatch["corrected"] += values[4] != values[0] or values[6] != values[5] or values[5] != values[0]
    if mismatch["solve"]:
        report.say("DATA", "solve and check do not have the same condition (%d points)" % mismatch["solve"])
    if mismatch["expected"]:
        report.say("DATA", "expected and the result in check differ (%d points)" % mismatch["expected"])
    if mismatch["corrected"]:
        report.say("DATA", "the corrected result is not confirmed either (%d points)" % mismatch["corrected"])
    report.broken = bool(mismatch["solve"] or mismatch["expected"] or mismatch["corrected"])

    if any(differ):
        report.differs = True
        if confirmed:
            report.say("DIFFER", "condition and printed result differ at %d of %d points" % (sum(differ), len(points)))
        else:
            report.say("NOTE", "the printed result is not confirmed, as the data declares (%d of %d points); the corrected result is" % (sum(differ), len(points)))
        for first, last, count in runs(points, differ):
            side = "the condition is %s, the result says %s" % tuple("true" if v else "false" for v in evaluate([condition, result], first[0])[0])
            where = first[1] if count == 1 else "%s ... %s (%d points)" % (first[1], last[1], count)
            report.lines.append("   at %s: %s" % (where, side))
    elif not confirmed:
        report.say("DATA", "book_confirmed is false but the printed result agrees with the condition everywhere")
        report.broken = True

    # Le soluzioni stampate di un'equazione: i due membri devono coincidere entro 10^-45.
    if ex["kind"].endswith("equation"):
        for node in walk(expected):
            if node.op == "cmp" and node.value == "=" and node.args[0].op == "x":
                values, env = evaluate([condition], constant(node.args[1]))
                if values[0]:
                    report.solutions += 1
                    report.gap = max(report.gap, env.gap)

    # Le varianti false: nel testimone condizione e risultato alterato devono essere diversi.
    for i, variant in enumerate(ex["false_checks"], 1):
        v_condition, v_result = split_check(parse(variant["check"]))
        witness = constant(parse(variant["witness"], "expr"))
        values, env = evaluate([v_condition, v_result, condition], witness)
        if values[0] != values[2] or not same(v_condition, condition):
            report.say("DATA", "false variant %d does not have the condition of the exercise" % i)
            report.broken = True
        elif values[0] == values[1]:
            report.say("DATA", "false variant %d does not differ at its witness %s" % (i, variant["witness"]))
            report.broken = True
        else:
            report.variants += 1
    if not 2 <= len(ex["false_checks"]) <= 3:
        report.say("DATA", "%d false variants (2 or 3 are expected)" % len(ex["false_checks"]))


def check_data(path):
    with open(path, encoding="utf-8") as lines:
        exercises = [json.loads(line) for line in lines if line.strip()]
    reports = []
    for ex in exercises:
        report = Report(ex.get("number"))
        try:
            check_exercise(ex, report)
        except (Syntax, KeyError) as problem:
            report.say("DATA", "cannot read the exercise: %r" % (problem,))
            report.broken = True
        print("\n".join(report.lines), end="\n" if report.lines else "")
        reports.append((ex, report))
    undeclared = [str(r.number) for ex, r in reports if r.differs and ex.get("book_confirmed", True)]
    declared = [str(r.number) for ex, r in reports if r.differs and not ex.get("book_confirmed", True)]
    broken = [str(r.number) for ex, r in reports if r.broken]
    print("checked %d exercises at %d points: %d printed results confirmed, %d not confirmed%s"
          % (len(reports), sum(r.points for _, r in reports), sum(not r.differs and not r.broken for _, r in reports),
             len(undeclared) + len(declared), " (%s)" % ", ".join(undeclared + declared) if undeclared or declared else ""))
    print("%d false variants differ at their witnesses; %d printed solutions of equations hold, the two sides agreeing within %.1e"
          % (sum(r.variants for _, r in reports), sum(r.solutions for _, r in reports), max([r.gap for _, r in reports] + [ZERO])))
    if broken:
        print("exercises with DATA problems: %s" % ", ".join(broken))
    return 1 if undeclared or broken else 0


# ---------------------------------------------------------------- fuzz: scrivere il LaTeX
def frac_tex(q):
    q = Fraction(q)
    if q.denominator == 1:
        return str(q.numerator)
    return "%s\\frac{%d}{%d}" % ("-" if q < 0 else "", abs(q.numerator), q.denominator)


def affine_tex(a, c):
    """a x + c scritto come su un libro: 2x - 1, 1 - x, \\frac{1}{2}x + 3; il solo numero se a = 0."""
    def monomial(k):
        return "x" if k == 1 else "%sx" % frac_tex(k)
    a, c = Fraction(a), Fraction(c)
    if a == 0:
        return frac_tex(c)
    if c == 0:
        return ("-" if a < 0 else "") + monomial(abs(a))
    if a < 0 and c > 0:
        return "%s - %s" % (frac_tex(c), monomial(-a))
    return "%s%s %s %s" % ("-" if a < 0 else "", monomial(abs(a)), "+" if c > 0 else "-", frac_tex(abs(c)))


def sum_tex(terms, glue=""):
    """La somma dei termini (coefficiente, testo), nell'ordine dato; un testo vuoto è un termine
    noto. glue separa il coefficiente dal testo: serve \\cdot davanti a una potenza di un
    numero, perché 10 3^{x} si leggerebbe 103^{x}."""
    out = ""
    for coefficient, text in terms:
        if coefficient == 0:
            continue
        size = abs(coefficient)
        body = frac_tex(size) if not text else text if size == 1 else "%s%s%s" % (frac_tex(size), glue, text)
        if not out:
            out = ("-" if coefficient < 0 else "") + body
        else:
            out += " %s %s" % ("-" if coefficient < 0 else "+", body)
    return out or "0"


def x_power(k):
    return "" if k == 0 else "x" if k == 1 else "x^{%d}" % k


def polynomial_tex(coefficients):
    """Il polinomio in x con coefficients[k] davanti a x^k, dal grado più alto."""
    return sum_tex([(c, x_power(k)) for k, c in reversed(list(enumerate(coefficients)))])


def base_tex(b):
    """La base b scritta dove va elevata a potenza."""
    if b == "e":
        return "e"
    return str(b.numerator) if b.denominator == 1 else "\\left(%s\\right)" % frac_tex(b)


def log_name(b):
    return "\\ln" if b == "e" else "\\log_{%s}" % frac_tex(b)


def grows(b):
    """True se b^u e \\log_b u crescono con u."""
    return b == "e" or b > 1


def sign(v):
    return (v > 0) - (v < 0)


TEX_RELATIONS = {"<": lambda s: s < 0, "\\le": lambda s: s <= 0, ">": lambda s: s > 0, "\\ge": lambda s: s >= 0,
                 "=": lambda s: s == 0, "\\neq": lambda s: s != 0}
FLIPPED = {"<": ">", "\\le": "\\ge", ">": "<", "\\ge": "\\le", "=": "=", "\\neq": "\\neq"}


def some_relation():
    return rnd.choice(["<", "\\le", ">", "\\ge", "<", ">", "=", "\\neq"])


# ---------------------------------------------------------------- fuzz: punti, celle, pezzi
class Point:
    """Un punto di rottura: un numero di cui l'oracolo conosce la scrittura esatta. frac è il
    suo valore quando è razionale; tex la scrittura in LaTeX; interval l'intervallo che lo
    contiene, calcolato dal valore o, per un numero irrazionale, leggendo tex."""

    def __init__(self, frac=None, tex=None):
        self.frac = None if frac is None else Fraction(frac)
        self.tex = frac_tex(self.frac) if tex is None else tex
        self.interval = TOP.fraction(self.frac) if frac is not None else constant(parse(tex, "expr"))


def cells_from(roots, holds):
    """Le radici in ordine e la verità di holds in ogni cella: prima della prima radice, in
    ogni radice, tra due consecutive, dopo l'ultima. Tra due radici consecutive la verità non
    cambia: è il chiamante a garantire che roots contenga ogni punto in cui può cambiare."""
    roots = sorted(set(roots))
    if not roots:
        return [], [holds(Fraction(0))]
    samples = [roots[0] - 1]
    for i, r in enumerate(roots):
        samples += [r, (r + roots[i + 1]) / 2 if i + 1 < len(roots) else r + 1]
    return roots, [holds(s) for s in samples]


def rational_case(condition, roots, holds):
    roots, cells = cells_from(roots, holds)
    return condition, [Point(r) for r in roots], cells


def locate(breaks, x):
    """L'indice della cella in cui cade l'intervallo x: 2k + 1 per il punto k, 2k per
    l'intervallo che lo precede."""
    for k, p in enumerate(breaks):
        side = compare(x, p.interval)
        if side == 0:
            return 2 * k + 1
        if side < 0:
            return 2 * k
    return 2 * len(breaks)


def pieces_of(breaks, cells):
    """Le celle vere consecutive, unite: una lista di (basso, compreso, alto, compreso), dove
    basso e alto sono Point, oppure None per un pezzo senza quell'estremo."""
    pieces, i = [], 0
    while i < len(cells):
        if not cells[i]:
            i += 1
            continue
        j = i
        while j + 1 < len(cells) and cells[j + 1]:
            j += 1
        low = None if i == 0 else breaks[(i - 1) // 2]
        high = None if j == len(cells) - 1 else breaks[j // 2]
        pieces.append((low, i % 2 == 1, high, j % 2 == 1))
        i = j + 1
    return pieces


def pieces_tex(pieces):
    """I pezzi scritti come li scrive il programma."""
    def one(piece):
        low, low_closed, high, high_closed = piece
        if low is None and high is None:
            return FORALL
        if low is high:
            return "x = %s" % low.tex
        if low is None:
            return "x %s %s" % ("\\le" if high_closed else "<", high.tex)
        if high is None:
            return "x %s %s" % ("\\ge" if low_closed else ">", low.tex)
        return "%s %s x %s %s" % (low.tex, "\\le" if low_closed else "<", "\\le" if high_closed else "<", high.tex)
    return " \\lor ".join(one(p) for p in pieces) if pieces else NEXISTS


def in_pieces(pieces, x):
    for low, low_closed, high, high_closed in pieces:
        above = low is None or compare(x, low.interval) > 0 or (low_closed and compare(x, low.interval) == 0)
        below = high is None or compare(x, high.interval) < 0 or (high_closed and compare(x, high.interval) == 0)
        if above and below:
            return True
    return False


def first_difference(breaks, cells, others, member):
    """Il primo punto in cui member(x) è diverso dalla cella dell'oracolo, o None. Prova ogni
    punto di rottura, ogni intervallo di others (gli estremi scritti da chi è sotto esame, i
    vicini), un punto tra due consecutivi e uno oltre ciascun estremo."""
    specials = arrange([p.interval for p in breaks] + others)
    for x in specials + between(specials):
        if member(x) != cells[locate(breaks, x)]:
            return x
    return None


def check_text(condition, pieces):
    """L'enunciato che dice che le soluzioni di condition sono pieces."""
    if not pieces:
        return "%s\\ \\lnot \\left( %s \\right)" % (FORALL, condition)
    if pieces[0][0] is None and pieces[0][2] is None:
        return "%s\\ \\left( %s \\right)" % (FORALL, condition)
    return "%s\\ \\left( %s \\iff \\left( %s \\right) \\right)" % (FORALL, condition, pieces_tex(pieces))


def spoil(pieces):
    """Un insieme di soluzioni sbagliato di poco, o None se non trova come sbagliarlo."""
    pieces = [list(p) for p in pieces]
    kind = rnd.random()
    if kind < 0.3 and pieces:
        del pieces[rnd.randrange(len(pieces))]
        return [tuple(p) for p in pieces]
    ends = [(i, k) for i, p in enumerate(pieces) for k in (0, 2) if p[k] is not None]
    if not ends:
        return None
    i, k = rnd.choice(ends)
    low, _, high, _ = pieces[i]
    offset = rnd.choice([Fraction(1, 2), Fraction(-1, 2), 1, -1, Fraction(1, 7)])
    if low is high:
        if low.frac is None:
            return None
        moved = Point(low.frac + offset)
        pieces[i] = [moved, True, moved, True]
    elif kind < 0.65:
        pieces[i][k + 1] = not pieces[i][k + 1]
    elif pieces[i][k].frac is None:
        return None
    else:
        pieces[i][k] = Point(pieces[i][k].frac + offset)
    low, _, high, _ = pieces[i]
    if low is not None and high is not None and low is not high and compare(low.interval, high.interval) >= 0:
        return None
    return [tuple(p) for p in pieces]


# ---------------------------------------------------------------- fuzz: (i) e (ii), sostituzioni
def product_at(factors, t):
    out = Fraction(1)
    for c, m in factors:
        out *= (t - c) ** m
    return out


def expand(factors):
    """I coefficienti interi di un multiplo positivo del prodotto dei fattori (t - c)^m: il
    multiplo toglie i denominatori senza cambiare né gli zeri né il segno."""
    coefficients = [Fraction(1)]
    for c, m in factors:
        for _ in range(m):
            coefficients = [(coefficients[k - 1] if k else 0) - c * (coefficients[k] if k < len(coefficients) else 0)
                            for k in range(len(coefficients) + 1)]
    scale = 1
    for c in coefficients:
        scale = scale * c.denominator // gcd(scale, c.denominator)
    return [c * scale for c in coefficients]


def substitution(roots):
    """Una condizione razionale in t con le radici date: (fattori sopra, fattori sotto,
    relazione), che vuol dire prodotto dei fattori sopra / prodotto dei fattori sotto ? 0.
    Ritorna anche la funzione che ne dà la verità in un razionale t."""
    factors = [(c, 2 if rnd.random() < 0.15 else 1) for c in roots]
    below = factors[-1:] if len(factors) > 1 and rnd.random() < 0.35 else []
    above = factors[:len(factors) - len(below)]
    relation = some_relation()

    def holds(t):
        d = product_at(below, t)
        return d != 0 and TEX_RELATIONS[relation](sign(product_at(above, t) / d))
    return (above, below, relation), holds


def dress(condition, power, glue, shiftable):
    """Il testo di una condizione in t, dove power(k) scrive t^k: scomposta in fattori, oppure
    sviluppata, con i termini negativi portati a destra una volta su due. Se shiftable, il
    polinomio sviluppato può essere diviso per t, che allora deve essere positivo."""
    above, below, relation = condition
    t = power(1)

    def factored(factors):
        parts = []
        for c, m in factors:
            inner = t if c == 0 else "%s %s %s" % (t, "-" if c > 0 else "+", frac_tex(abs(c)))
            alone = len(factors) == 1 and m == 1
            parts.append(inner if alone else "\\left(%s\\right)%s" % (inner, "^{%d}" % m if m > 1 else ""))
        return "".join(parts)

    def expanded(factors, lower=0):
        return [(c, power(k - lower)) for k, c in reversed(list(enumerate(expand(factors))))]

    if not below and len(above) == 1 and above[0][1] == 1 and rnd.random() < 0.5:
        return "%s %s %s" % (t, relation, frac_tex(above[0][0]))
    if rnd.random() < 0.45:
        top = factored(above)
        return "\\frac{%s}{%s} %s 0" % (top, factored(below), relation) if below else "%s %s 0" % (top, relation)
    if below:
        return "\\frac{%s}{%s} %s 0" % (sum_tex(expanded(above), glue), sum_tex(expanded(below), glue), relation)
    terms = expanded(above, 1 if shiftable and rnd.random() < 0.35 else 0)
    if rnd.random() < 0.5:
        left, right = [term for term in terms if term[0] > 0], [(-c, text) for c, text in terms if c < 0]
        return "%s %s %s" % (sum_tex(left, glue), relation, sum_tex(right, glue))
    return "%s %s 0" % (sum_tex(terms, glue), relation)


def overlaps(point, direct):
    """Controlla che la scrittura di un punto irrazionale valga ciò che il conto diretto dice."""
    if compare(point.interval, direct) != 0:
        raise AssertionError("the text %s does not have the value it should" % point.tex)
    return point


def exp_substitution():
    """(i) Una condizione razionale in t = b^{px+q}."""
    b = rnd.choice([Fraction(2), Fraction(3), Fraction(5), Fraction(10), Fraction(1, 2), Fraction(1, 3),
                    Fraction(3, 2), Fraction(2, 3), Fraction(2), Fraction(3), "e"])
    p, q = Fraction(rnd.choice([1, 1, 1, 2, -1, 3, -2, Fraction(1, 2)])), Fraction(rnd.randint(-2, 2))
    powers = [Fraction(1)] if b == "e" else [b ** k for k in range(-2, 4)]
    roots = []
    while len(roots) < rnd.choice([1, 2, 2, 3]):
        kind = rnd.random()
        c = (rnd.choice(powers) if kind < 0.6 else Fraction(rnd.choice([-1, -2, -3, 0, -5])) if kind < 0.8
             else Fraction(rnd.choice([3, 5, 6, 7, 12]), rnd.choice([1, 1, 4])))
        if c not in roots:
            roots.append(c)
    condition, holds = substitution(roots)

    def x_of(c):
        """Il punto x in cui b^{px+q} = c, cioè (log_b c - q) / p."""
        for k in range(-8, 9):
            if (b == "e" and k == 0 and c == 1) or (b != "e" and b ** k == c):
                return Point((k - q) / p)
        log = "%s %s" % (log_name(b), frac_tex(c))
        top = log if q == 0 else "%s %s %s" % (log, "-" if q > 0 else "+", frac_tex(abs(q)))
        if p == 1:
            tex = top
        elif p == -1:
            tex = "-%s" % log if q == 0 else "%s - %s" % (frac_tex(q), log)
        elif p.denominator == 1:
            tex = "%s\\frac{%s}{%d}" % ("-" if p < 0 else "", top, int(abs(p)))
        else:
            tex = "%s%s" % (frac_tex(1 / p), log) if q == 0 else "%s\\left(%s\\right)" % (frac_tex(1 / p), top)
        ln_b = (ONE, ONE) if b == "e" else TOP.ln(TOP.fraction(b))
        direct = TOP.div(TOP.sub(TOP.div(TOP.ln(TOP.fraction(c)), ln_b), TOP.fraction(q)), TOP.fraction(p))
        return overlaps(Point(tex=tex), direct)

    # Le celle in t > 0: sotto la prima radice positiva, in ogni radice positiva, tra due, sopra l'ultima.
    positive = sorted(c for c in roots if c > 0)
    samples = [positive[0] / 2] if positive else [Fraction(1)]
    for i, c in enumerate(positive):
        samples += [c, (c + positive[i + 1]) / 2 if i + 1 < len(positive) else c + 1]
    breaks, cells = [x_of(c) for c in positive], [holds(t) for t in samples]
    if (p > 0) != grows(b):
        breaks.reverse()
        cells.reverse()

    def power(k):
        if k == 0:
            return ""
        if b != "e" and b.denominator == 1 and k % 2 == 0 and b * b <= 100 and rnd.random() < 0.5:
            return "%d^{%s}" % (int(b * b), affine_tex(k * p / 2, k * q / 2))
        return "%s^{%s}" % (base_tex(b), affine_tex(k * p, k * q))
    return dress(condition, power, " \\cdot ", True), breaks, cells


def power_point(b, c):
    """Il punto b^c: una frazione quando c è intero e b razionale, altrimenti scritto con e o con una radice."""
    if b != "e" and c.denominator == 1:
        return Point(b ** int(c))
    if c == 0:
        return Point(1)
    name = "e" if b == "e" else str(b)
    if c.denominator == 1:
        tex = name if c == 1 else "%s^{%d}" % (name, int(c)) if c > 1 else "\\frac{1}{%s}" % (name if c == -1 else "%s^{%d}" % (name, int(-c)))
    else:
        tex = {Fraction(1, 2): "\\sqrt{%s}", Fraction(-1, 2): "\\frac{1}{\\sqrt{%s}}", Fraction(3, 2): "%s\\sqrt{%s}"}[c]
        tex = tex % ((name, name) if c == Fraction(3, 2) else name)
    ln_b = (ONE, ONE) if b == "e" else TOP.ln(TOP.fraction(b))
    return overlaps(Point(tex=tex), TOP.exp(TOP.mul(TOP.fraction(c), ln_b)))


def log_substitution():
    """(ii) Una condizione razionale in t = \\log_b x."""
    b = rnd.choice([Fraction(2), Fraction(3), Fraction(10), Fraction(5), Fraction(1, 2), Fraction(1, 3), Fraction(2), "e"])
    whole = b == "e" or b.denominator == 1
    roots = []
    while len(roots) < rnd.choice([1, 2, 2, 3]):
        c = Fraction(rnd.choice([1, -1, 3]), 2) if whole and rnd.random() < 0.2 else Fraction(rnd.randint(-3, 3))
        if c not in roots:
            roots.append(c)
    condition, holds = substitution(roots)
    roots, cells = cells_from(roots, holds)
    breaks = [power_point(b, c) for c in roots]
    if not grows(b):
        breaks.reverse()
        cells.reverse()
    name = "%s x" % log_name(b)

    def power(k):
        return "" if k == 0 else name if k == 1 else "\\left(%s\\right)^{%d}" % (name, k)
    # Per x <= 0 il logaritmo non ha valore: due celle false, e lo zero come primo punto di rottura.
    return dress(condition, power, "", False), [Point(0)] + breaks, [False, False] + cells


# ---------------------------------------------------------------- fuzz: (iii) monomi
def poly_mul(a, b):
    out = [Fraction(0)] * (len(a) + len(b) - 1)
    for i, p in enumerate(a):
        for j, q in enumerate(b):
            out[i + j] += p * q
    return out


def poly_add(a, b):
    return [(a[k] if k < len(a) else 0) + (b[k] if k < len(b) else 0) for k in range(max(len(a), len(b)))]


def poly_at(a, x):
    return sum((c * x ** k for k, c in enumerate(a)), Fraction(0))


def monomial():
    """(iii) b^{E(x)} ? b^{F(x)}, con E e F razionali in x; E e F sono (numeratore, denominatore o None)."""
    b = rnd.choice([Fraction(2), Fraction(3), Fraction(5), Fraction(10), Fraction(1, 2), Fraction(1, 3),
                    Fraction(2, 3), Fraction(3, 2), "e"])
    relation = some_relation()
    k, r1, r2 = rnd.choice([1, -1, 2]), rnd.randint(-4, 4), rnd.randint(-4, 4)
    gap = poly_mul([Fraction(k)], poly_mul([-r1, 1], [-r2, 1]) if rnd.random() < 0.6 else [-r1, 1])
    line = [Fraction(rnd.randint(-3, 3)), Fraction(rnd.choice([0, 0, 1, 1, 2, -1]))]
    shape = rnd.random()
    if shape < 0.4:
        # E = F + k (x - r1)(x - r2): polinomi.
        low, high, roots = (line, None), (poly_add(line, gap), None), [r1, r2]
    elif shape < 0.75:
        # E = (F (x - s) + k (x - r1)(x - r2)) / (x - s): E - F = k (x - r1)(x - r2) / (x - s).
        s = rnd.randint(-4, 4)
        low, high, roots = (line, None), (poly_add(poly_mul(line, [-s, 1]), gap), [-s, 1]), [r1, r2, s]
    else:
        # E = a / (x - s1), F = c / (x - s2): E - F si annulla in (a s2 - c s1) / (a - c).
        a, c = rnd.sample([1, 2, 3, -1, -2, 4], 2)
        s1, s2 = rnd.sample(range(-4, 5), 2)
        low, high = ([Fraction(c)], [-s2, 1]), ([Fraction(a)], [-s1, 1])
        roots = [s1, s2, Fraction(a * s2 - c * s1, a - c)]
    direction = relation if grows(b) else FLIPPED[relation]

    def at(e, x):
        d = poly_at(e[1], x) if e[1] else Fraction(1)
        return None if d == 0 else poly_at(e[0], x) / d

    def holds(x):
        e, f = at(high, x), at(low, x)
        return e is not None and f is not None and TEX_RELATIONS[direction](sign(e - f))

    def tex(e):
        return polynomial_tex(e[0]) if e[1] is None else "\\frac{%s}{%s}" % (polynomial_tex(e[0]), polynomial_tex(e[1]))
    left = "%s^{%s}" % (base_tex(b), tex(high))
    constant_f = low[1] is None and low[0][1] == 0
    if constant_f and (b != "e" or low[0][0] in (0, 1)) and rnd.random() < 0.6:
        # Il secondo membro scritto come numero: b^m.
        right = ("e" if low[0][0] else "1") if b == "e" else frac_tex(b ** int(low[0][0]))
    elif low[1] is None and b != "e" and b.denominator == 1 and not constant_f and rnd.random() < 0.3:
        # (1/b)^{-F} = b^F.
        right = "\\left(\\frac{1}{%d}\\right)^{%s}" % (int(b), polynomial_tex([-c for c in low[0]]))
    else:
        right = "%s^{%s}" % (base_tex(b), tex(low))
    return rational_case("%s %s %s" % (left, relation, right), [Fraction(r) for r in roots], holds)


# ---------------------------------------------------------------- fuzz: (iv) somme di logaritmi
def rational_sqrt(q):
    """La radice quadrata di q se è razionale, altrimenti None."""
    if q < 0:
        return None
    n, d = int(q.numerator ** 0.5 + 0.5), int(q.denominator ** 0.5 + 0.5)
    return Fraction(n, d) if n * n == q.numerator and d * d == q.denominator else None


def log_sum():
    """(iv) \\log_b f + \\log_b g ? c, oppure \\log_b f - \\log_b g ? c, con f e g di primo grado."""
    # Somma o differenza si decide prima di cercare: la somma chiede radici razionali, e scegliendo
    # dentro il ciclo quasi tutti i casi tenuti sarebbero differenze.
    plus = rnd.random() < 0.7
    for attempt in range(5000):
        b = rnd.choice([Fraction(2), Fraction(3), Fraction(10), Fraction(6), Fraction(1, 2), Fraction(1, 3), Fraction(2), "e"])
        c = 0 if b == "e" else rnd.choice([0, 1, 1, 2, 3, -1, -2])
        level = Fraction(1) if b == "e" else b ** c
        a1, a2 = rnd.choice([1, 1, 1, 2, -1]), rnd.choice([1, 1, 1, 2, -1])
        c1, c2 = rnd.randint(-6, 6), rnd.randint(-6, 6)
        relation = some_relation()
        direction = relation if grows(b) else FLIPPED[relation]
        f, g = [Fraction(c1), Fraction(a1)], [Fraction(c2), Fraction(a2)]
        # Dove f > 0 e g > 0: f g ? b^c per la somma, f ? b^c g per la differenza.
        gap = poly_add(poly_mul(f, g), [-level]) if plus else poly_add(f, [-level * v for v in g])
        roots = [Fraction(-c1, a1), Fraction(-c2, a2)]
        if len(gap) == 3 and gap[2] != 0:
            root = rational_sqrt(gap[1] * gap[1] - 4 * gap[2] * gap[0])
            if root is None:
                continue
            roots += [(-gap[1] - root) / (2 * gap[2]), (-gap[1] + root) / (2 * gap[2])]
        elif gap[1] != 0:
            roots.append(-gap[0] / gap[1])

        def alone(x):
            return TEX_RELATIONS[direction](sign(poly_at(gap, x)))

        def holds(x):
            return poly_at(f, x) > 0 and poly_at(g, x) > 0 and alone(x)
        # Si tengono i casi in cui il campo di esistenza toglie qualcosa alle soluzioni del solo polinomio.
        if cells_from(roots, holds)[1] == cells_from(roots, alone)[1] and attempt < 4900:
            continue
        name = log_name(b)
        text = "%s(%s) %s %s(%s) %s %d" % (name, affine_tex(a1, c1), "+" if plus else "-", name, affine_tex(a2, c2), relation, c)
        return rational_case(text, roots, holds)
    raise AssertionError("no sum of logarithms found")


# ---------------------------------------------------------------- fuzz: (v) campi di esistenza
class Piece:
    """Un pezzo di espressione di cui l'oracolo sa dire, in ogni razionale x, se ha valore e
    con quale segno: sign_at(x) è None, -1, 0 oppure 1. breaks sono i punti in cui la risposta
    può cambiare."""

    def __init__(self, tex, breaks, sign_at):
        self.tex, self.breaks, self.sign_at = tex, breaks, sign_at


class Inner:
    """Ciò su cui lavora un pezzo: a x + c, oppure |a x + c|."""

    def __init__(self):
        self.a, self.c = Fraction(rnd.choice([1, 1, 1, 2, -1, -2, 3])), Fraction(rnd.randint(-5, 5))
        self.absolute = rnd.random() < 0.15
        self.plain = affine_tex(self.a, self.c)
        self.tex = "\\left| %s \\right|" % self.plain if self.absolute else self.plain

    def at(self, x):
        v = self.a * x + self.c
        return abs(v) if self.absolute else v

    def levels(self, k):
        """I punti in cui vale k."""
        if not self.absolute:
            return [(k - self.c) / self.a]
        return [(k - self.c) / self.a, (-k - self.c) / self.a] if k >= 0 else []

    def logarithm(self, b):
        if self.absolute or self.plain == "x":
            return "%s%s%s" % (log_name(b), "" if self.absolute else " ", self.tex)
        return "%s(%s)" % (log_name(b), self.tex)


def minus(k):
    """Il testo di "- k" in coda a un'espressione: niente se k = 0, "+ |k|" se k < 0."""
    return "" if k == 0 else " %s %s" % ("-" if k > 0 else "+", frac_tex(abs(k)))


def log_piece():
    """\\log_b(L) - k: ha valore per L > 0, e il segno di L - b^k (rovesciato se b < 1)."""
    b, inner = rnd.choice([Fraction(2), Fraction(3), Fraction(10), Fraction(1, 2), Fraction(1, 3), "e"]), Inner()
    k = 0 if b == "e" else rnd.choice([0, 0, 1, 2, -1])
    level, way = (Fraction(1) if b == "e" else b ** k), (1 if grows(b) else -1)

    def sign_at(x):
        v = inner.at(x)
        return None if v <= 0 else way * sign(v - level)
    return Piece(inner.logarithm(b) + minus(k), inner.levels(0) + inner.levels(level), sign_at)


def exp_piece():
    """b^L - b^k, con b^k scritto come numero: ha sempre valore, e il segno di L - k (rovesciato se b < 1)."""
    b, inner, k = rnd.choice([Fraction(2), Fraction(3), Fraction(5), Fraction(1, 2), Fraction(1, 3)]), Inner(), rnd.randint(-1, 3)
    way = 1 if grows(b) else -1
    return Piece("%s^{%s} - %s" % (base_tex(b), inner.tex, frac_tex(b ** k)), inner.levels(k), lambda x: way * sign(inner.at(x) - k))


def sqrt_piece():
    """\\sqrt{L} - k con k >= 0: ha valore per L >= 0, e il segno di L - k^2."""
    inner, k = Inner(), rnd.choice([Fraction(0), Fraction(1), Fraction(2), Fraction(3), Fraction(1, 2)])

    def sign_at(x):
        v = inner.at(x)
        return None if v < 0 else sign(v - k * k)
    return Piece("\\sqrt{%s}%s" % (inner.tex, minus(k)), inner.levels(0) + inner.levels(k * k), sign_at)


def log_log_piece():
    """\\log_{b1} \\log_{b2}(L) - m, con b1^m intero: ha valore dove \\log_{b2} L > 0, e lì ha
    il segno di \\log_{b2} L - b1^m, cioè di L - b2^{b1^m}, rovesciato una volta per ogni base
    minore di 1."""
    b1, m = rnd.choice([(Fraction(2), 0), (Fraction(2), 1), (Fraction(2), 2), (Fraction(3), 0), (Fraction(3), 1),
                        (Fraction(1, 2), 0), (Fraction(1, 2), -1), (Fraction(1, 2), -2), (Fraction(1, 3), -1)])
    b2, inner = rnd.choice([Fraction(2), Fraction(3), Fraction(1, 2), Fraction(1, 3)]), Inner()
    way1, way2 = (1 if grows(b1) else -1), (1 if grows(b2) else -1)
    level = b2 ** int(b1 ** m)

    def sign_at(x):
        v = inner.at(x)
        if v <= 0 or way2 * sign(v - 1) <= 0:
            return None
        return way1 * way2 * sign(v - level)
    return Piece("%s%s%s" % (log_name(b1), inner.logarithm(b2), minus(m)),
                 inner.levels(0) + inner.levels(1) + inner.levels(level), sign_at)


def rational_piece():
    """Un prodotto di fattori di primo grado, a volte diviso per un altro: il segno è quello del conto esatto."""
    r1, r2, k = rnd.randint(-4, 4), rnd.randint(-4, 4), rnd.choice([1, 1, -1])
    top = poly_mul([Fraction(k)], poly_mul([-r1, 1], [-r2, 1]) if rnd.random() < 0.5 else [-r1, 1])
    s = rnd.randint(-4, 4) if rnd.random() < 0.3 else None

    def sign_at(x):
        return sign(poly_at(top, x)) if s is None else None if x == s else sign(poly_at(top, x) / (x - s))
    tex = polynomial_tex(top) if s is None else "\\frac{%s}{%s}" % (polynomial_tex(top), polynomial_tex([-s, 1]))
    return Piece(tex, [Fraction(r1), Fraction(r2)] + ([Fraction(s)] if s is not None else []), sign_at)


def some_piece():
    """Un pezzo, oppure il prodotto o il quoziente di due: il segno è il prodotto dei segni."""
    def atom():
        return rnd.choice([log_piece, log_piece, exp_piece, sqrt_piece, log_log_piece, rational_piece])()
    kind = rnd.random()
    if kind < 0.6:
        return atom()
    p, q = atom(), atom()
    if kind < 0.8:
        def sign_at(x):
            s, t = p.sign_at(x), q.sign_at(x)
            return None if s is None or t is None else s * t
        return Piece("\\left(%s\\right)\\left(%s\\right)" % (p.tex, q.tex), p.breaks + q.breaks, sign_at)

    def sign_at(x):
        s, t = p.sign_at(x), q.sign_at(x)
        return None if s is None or not t else s * t
    return Piece("\\frac{%s}{%s}" % (p.tex, q.tex), p.breaks + q.breaks, sign_at)


def some_term():
    """Una radice quadrata, un logaritmo o un reciproco di un pezzo, con la condizione sul segno
    del pezzo che gli dà valore. Ritorna (testo, punti di rottura, funzione "ha valore in x")."""
    piece, kind = some_piece(), rnd.random()
    if kind < 0.4:
        return "\\sqrt{%s}" % piece.tex, piece.breaks, lambda x: piece.sign_at(x) in (0, 1)
    if kind < 0.75:
        name = log_name(rnd.choice([Fraction(2), Fraction(10), Fraction(1, 2), "e"]))
        return "%s\\left(%s\\right)" % (name, piece.tex), piece.breaks, lambda x: piece.sign_at(x) == 1
    return "\\frac{%d}{%s}" % (rnd.randint(1, 5), piece.tex), piece.breaks, lambda x: piece.sign_at(x) in (-1, 1)


def domain():
    """(v) Il campo di esistenza di un'espressione: la richiesta è E = E, vera dove E ha valore."""
    tex, breaks, defined = some_term()
    kind = rnd.random()
    if kind < 0.35:
        other, more, also = some_term()
        tex, breaks = "%s %s %s" % (tex, rnd.choice("+-"), other), breaks + more
        first = defined

        def defined(x):
            return first(x) and also(x)
    elif kind < 0.6:
        divisor = rnd.choice([log_piece, exp_piece, sqrt_piece])()
        tex, breaks = "\\frac{%s}{%s}" % (tex, divisor.tex), breaks + divisor.breaks
        first = defined

        def defined(x):
            return first(x) and divisor.sign_at(x) in (-1, 1)
    return rational_case("%s = %s" % (tex, tex), breaks, defined)


# ---------------------------------------------------------------- fuzz: enunciati e risposte
GENERATORS = [exp_substitution, log_substitution, monomial, log_sum, domain]


def make_case(index):
    """Ritorna (testo LaTeX, verità, generatore, punti di rottura, celle): verità è True o False
    per un enunciato, None per una richiesta di soluzioni."""
    generator = GENERATORS[index % len(GENERATORS)]
    while True:
        condition, breaks, cells = generator()
        # Il controllo di sé: la condizione riletta dal testo deve dare le stesse celle.
        formula = parse(condition, "formula")
        near = [shift(p.interval, d) for p in breaks for d in (Decimal("1e-6"), Decimal("-1e-6"))]
        try:
            wrong = first_difference(breaks, cells, near, lambda x: evaluate([formula], x)[0][0])
        except Imprecise:
            wrong = "a point where a sign cannot be decided"
        if wrong is not None:
            print("ORACLE: %s builds a condition that its own numeric check does not confirm at %s\n   %s"
                  % (generator.__name__, wrong if isinstance(wrong, str) else describe(wrong), condition))
            continue
        pieces = pieces_of(breaks, cells)
        shape = rnd.random()
        if shape < 0.4:
            return "\\{x \\in \\mathbb{R} \\mid %s\\}" % condition, None, generator.__name__, breaks, cells
        if shape < 0.7:
            return check_text(condition, pieces), True, generator.__name__, breaks, cells
        spoiled = spoil(pieces)
        if spoiled is None:
            continue
        ends = [p.interval for piece in spoiled for p in (piece[0], piece[2]) if p is not None]
        true = first_difference(breaks, cells, ends, lambda x: in_pieces(spoiled, x)) is None
        return check_text(condition, spoiled), true, generator.__name__, breaks, cells


def check_solution(answer, breaks, cells):
    """Confronta le soluzioni scritte dal programma con le celle dell'oracolo, in ogni punto di
    rottura, in ogni estremo scritto dal programma e tra un punto e l'altro. Ritorna "ok",
    "bug" oppure "suspect"."""
    text = answer.split(": ", 1)[1] if ": " in answer else answer
    remark = ""
    m = re.search(r" \((?:estremi irrazionali|[^()\\]*\bè la radice)", text)
    if m:
        text, remark = text[:m.start()], text[m.start():]

    # Una radice senza scrittura ha un nome, e il programma dice tra quali numeri sta: l'oracolo, che
    # conosce tutti i punti di rottura, deve trovarne lì uno solo, ed è quello.
    names = {}
    for m in re.finditer(r"(r_\{\d+\}) è la radice di .*? tra (-?\d+(?:/\d+)?) e (-?\d+(?:/\d+)?)", remark):
        low, high = TOP.fraction(Fraction(m.group(2))), TOP.fraction(Fraction(m.group(3)))
        inside = [p for p in breaks if compare(p.interval, low) > 0 and compare(p.interval, high) < 0]
        if len(inside) != 1:
            return "bug"
        names[m.group(1)] = inside[0].interval
    try:
        solutions = parse_result(text, names)
        ends = printed_numbers(solutions)
        wrong = first_difference(breaks, cells, ends, lambda x: evaluate([solutions], x)[0][0])
    except (Syntax, Imprecise):
        return "suspect"
    return "ok" if wrong is None else "bug"


def fuzz(program, seed, count, work):
    rnd.seed(seed)
    cases = [make_case(i) for i in range(count)]
    stem = os.path.join(work, "fuzz_exponentials_logarithms_%d" % seed)
    with open(stem + ".tex", "w", encoding="utf-8") as out:
        out.write("\\begin{enumerate}\n")
        for text, _, _, _, _ in cases:
            out.write("\\item \\[ %s \\]\n" % text)
        out.write("\\end{enumerate}\n")
    with open(stem + ".expected", "w", encoding="utf-8") as out:
        for number, (text, true, generator, breaks, cells) in enumerate(cases, 1):
            kind = "solve" if true is None else "true" if true else "false"
            out.write("%d\t%s\t%s\t%s\n" % (number, generator, kind, pieces_tex(pieces_of(breaks, cells))))
    run = subprocess.run([program, stem + ".tex"], capture_output=True, text=True, errors="replace", timeout=3600)
    lines = [l for l in run.stdout.splitlines() if re.match(r"\s*\d+\. ", l)]
    if len(lines) != len(cases) or run.returncode not in (0, 1) or "Sanitizer" in run.stderr or "runtime error" in run.stderr:
        print("BUG: the program answered %d of %d statements (exit %d)\n%s" % (len(lines), len(cases), run.returncode, run.stderr[-2000:]))
        return
    tally, undecided = {}, {}
    for number, ((text, true, generator, breaks, cells), line) in enumerate(zip(cases, lines), 1):
        answer = line.split(". ", 1)[1]
        verdict = ("P" if answer.startswith("DIMOSTRATO") else "R" if answer.startswith("FALSO") else "S" if answer.startswith("RISOLTO")
                   else "N" if answer.startswith("NON DIMOSTRATO") else "U")
        kind = "solve" if true is None else "true" if true else "false"
        tally[(generator, kind, verdict)] = tally.get((generator, kind, verdict), 0) + 1
        if true is None and verdict == "S":
            outcome = check_solution(answer, breaks, cells)
            if outcome == "bug":
                print("BUG: statement %d, the written solutions are wrong: %s\n   %s\n   the oracle says: %s"
                      % (number, answer[:300], text, pieces_tex(pieces_of(breaks, cells))))
            elif outcome == "suspect":
                print("SUSPECT: statement %d, cannot read the written solutions: %s\n   %s" % (number, answer[:300], text))
        elif (true is None) != (verdict == "S") and verdict in ("P", "R", "S"):
            print("BUG: statement %d got the wrong kind of answer: %s\n   %s" % (number, answer[:90], text))
        elif (verdict == "P" and not true) or (verdict == "R" and true):
            print("BUG: statement %d is %s but the program says %s\n   %s" % (number, "true" if true else "false", answer[:90], text))
        elif verdict in ("N", "U"):
            undecided.setdefault((generator, answer[:110]), []).append((number, text))
    for (generator, reason), found in sorted(undecided.items()):
        print("NOTE: %d statements of %s not decided (%s), for example statement %d\n   %s"
              % (len(found), generator, reason, found[0][0], found[0][1][:300]))
    if "ATTENZIONE" in run.stdout:
        print("BUG: the program discarded a step of its own proof: %s" % [l for l in run.stdout.splitlines() if "ATTENZIONE" in l])
    print("seed %d:" % seed, ", ".join("%s/%s/%s=%d" % (k[0], k[1], k[2], v) for k, v in sorted(tally.items())))


def main():
    if len(sys.argv) == 3 and sys.argv[1] == "check-data":
        sys.exit(check_data(sys.argv[2]))
    if len(sys.argv) == 6 and sys.argv[1] == "fuzz":
        fuzz(sys.argv[2], int(sys.argv[3]), int(sys.argv[4]), sys.argv[5])
        return
    sys.exit(__doc__.split("\n\nA COSA SERVE")[0])


if __name__ == "__main__":
    main()
