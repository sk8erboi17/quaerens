"""Fuzzing delle identita con gli indici del calcolo tensoriale (capitoli 37 e
39 di parser.c, capitolo 38 di main.c, capitolo 19 di checker.c).

Uso:

    python3 -I fuzz_tensors.py <mathresearcher> <seme> <quanti> <cartella di lavoro>
        genera enunciati a caso, veri, falsi e fuori linguaggio, e confronta i
        verdetti del programma con l'oracolo;

    python3 -I fuzz_tensors.py data <mathresearcher> <file .tex> <true|false|beyond> <cartella di lavoro>
        giudica con l'oracolo gli enunciati di un file di tests/tensor_calculus/
        (il terzo conto sui dati presi dai libri) e li confronta con il programma;

    python3 -I fuzz_tensors.py certificates <checker> <seme> <quanti> <cartella di lavoro>
        scrive certificati giusti e sbagliati (affermazione, punto, valori,
        dimensione, voci metric e given) e confronta le risposte del
        verificatore con l'oracolo.

L'ORACOLO non chiama mai il programma per decidere, e non ne copia il
ragionamento. Ha un lettore suo del LaTeX. Stabilisce la convenzione di
Einstein com'e definita nei libri: svolge tutte le parentesi, e in ogni
prodotto che ne risulta conta quante volte compare ogni lettera tra gli indici
(una: libera; due: sommata; di piu: l'enunciato non ha senso). Il motore e il
verificatore non svolgono nulla: usano regole nodo per nodo, e sommano
ciascuno a modo suo. Poi l'oracolo calcola ogni prodotto svolto per forza
bruta, su tutti i valori delle lettere sommate; il segno di una permutazione
lo ricava dai cicli, non dalle coppie fuori ordine. Le componenti dei tensori
qualsiasi sono indeterminate di un polinomio a coefficienti razionali
(fractions), e due membri sono uguali se lo sono come polinomi, per ogni valore
degli indici liberi: e una decisione, non una prova a campione.

Con una metrica dichiarata tra le ipotesi (capitolo 39 di parser.c) il
programma svolge ogni indice alzato o abbassato in una contrazione con la
metrica, e decide l'identita svolta. L'oracolo non svolge nulla: calcola
l'identita com'e scritta, moltiplicando ogni componente per il numero della
diagonale (o per il suo reciproco) di ogni indice che non sta nella posizione
dichiarata, e controlla le posizioni prodotto per prodotto: un indice sommato
una volta in alto e una in basso, uno libero sempre nella stessa posizione.

Le ipotesi possono dare un tensore per componenti, con una tabella o con un
elenco, e dire da quale numero cominciano i valori degli indici; gli indici
possono essere lettere greche. Il programma, leggendo, sostituisce le lettere
greche con lettere latine, porta i valori a cominciare da 1 e mette gli
elementi delle tabelle nel certificato. L'oracolo non fa nessuna delle tre
cose: una lettera greca resta il suo comando, i valori restano quelli
scritti (da 0 a d - 1 se cosi e dichiarato), e la componente di un tensore
dato e il polinomio del suo elemento, cercato per riga e colonna nella
tabella com'e scritta.

Gli elementi della diagonale della metrica possono essere funzioni delle
coordinate: polinomi, o frazioni di polinomi, in lettere senza indici. Allora
l'oracolo calcola ogni membro come una frazione di polinomi (Ratio), senza
ridurla, e due valori sono uguali quando lo e il prodotto in croce: e
l'uguaglianza dove gli elementi della metrica sono definiti e diversi da
zero. Il programma tiene le frazioni in un altro modo, e il verificatore in
un terzo (un polinomio su un prodotto di potenze dei numeratori e dei
denominatori della metrica).

Righe stampate. BUG: il programma ha dimostrato un falso, ha dichiarato falso
un vero, ha dato un controesempio o dei valori sbagliati, oppure il
verificatore ha accettato un certificato che l'oracolo giudica sbagliato.
SUSPECT: i due non sono d'accordo su che cosa sia un enunciato leggibile, il
verificatore ha respinto un certificato giusto, oppure il programma ha
risposto NON DIMOSTRATO per un motivo che non e un limite dichiarato (per
esempio perche il verificatore ha respinto il certificato del motore): va
guardato a mano. I verdetti NON DIMOSTRATO per un limite dichiarato (un conto
oltre il budget, un certificato troppo grande per il verificatore) sono
contati e non sono errori. Con un BUG lo
script esce con 1. Tutti i file prodotti stanno nella cartella di lavoro.
"""

import itertools
import os
import random
import re
import subprocess
import sys
from fractions import Fraction


class Outside(Exception):
    """L'enunciato non e del linguaggio: il motivo, per chi guarda a mano."""


# La metrica dell'enunciato che l'oracolo sta giudicando, o None. Un dizionario:
# "c" i numeri della diagonale di g_{ij}; per un enunciato scritto (non per un
# certificato) anche "written" vero, "name" la lettera della metrica,
# "reference" le posizioni degli indici di ogni tensore nella prima occorrenza
# e, se dichiarato, "eps" il valore di epsilon con gli indici in ordine ed
# "eps_up" se e dato con gli indici in alto. La mette chi comincia un conto:
# read_statement() per un enunciato, check_counterexample() per rifare quello
# di un enunciato gia letto, main_certificates() per un certificato,
# generate() per ricominciare senza.
METRIC = None

# I tensori dati per componenti dell'enunciato che l'oracolo sta giudicando:
# dal nome alla lista degli alberi dei suoi elementi, per righe. E il numero
# da cui cominciano i valori degli indici, 0 o 1. Li mette chi mette METRIC.
GIVEN = {}
ORIGIN = 1

# Le lettere greche che possono fare da indice: \delta e \varepsilon no,
# sono i simboli del linguaggio.
GREEK = {"\\" + name for name in ("alpha beta gamma iota kappa lambda mu nu pi rho sigma tau phi varphi chi psi omega "
                                    "eta theta xi zeta").split()}


def is_letter(token):
    return token is not None and len(token) == 1 and token.isalpha()


def poly_times(p, q):
    """Il prodotto di due polinomi nelle componenti: dizionari da monomio
    (componenti in ordine) a coefficiente."""
    result = {}
    for a, x in p.items():
        for b, y in q.items():
            key = tuple(sorted(a + b))
            result[key] = result.get(key, 0) + x * y
    return {key: c for key, c in result.items() if c != 0}


def poly_plus(p, q):
    result = dict(p)
    for key, c in q.items():
        result[key] = result.get(key, 0) + c
    return {key: c for key, c in result.items() if c != 0}


class Ratio:
    """Una frazione di polinomi nelle componenti, non ridotta. Due frazioni
    sono uguali quando lo e il prodotto in croce: i denominatori non sono mai
    il polinomio nullo, quindi e l'uguaglianza dove non si annullano."""

    __hash__ = None

    def __init__(self, num, den=None):
        self.num = dict(num)
        self.den = dict(den) if den is not None else {(): Fraction(1)}

    @staticmethod
    def of(x):
        if isinstance(x, Ratio):
            return x
        if isinstance(x, dict):
            return Ratio(x)
        return Ratio({(): Fraction(x)} if x != 0 else {})

    def __mul__(self, other):
        other = Ratio.of(other)
        return Ratio(poly_times(self.num, other.num), poly_times(self.den, other.den))

    __rmul__ = __mul__

    def __truediv__(self, other):
        other = Ratio.of(other)
        return Ratio(poly_times(self.num, other.den), poly_times(self.den, other.num))

    def __rtruediv__(self, other):
        return Ratio.of(other) / self

    def __add__(self, other):
        other = Ratio.of(other)
        return Ratio(poly_plus(poly_times(self.num, other.den), poly_times(other.num, self.den)), poly_times(self.den, other.den))

    def __eq__(self, other):
        other = Ratio.of(other)
        return poly_times(self.num, other.den) == poly_times(other.num, self.den)

    def __ne__(self, other):
        return not self == other


def is_index(token):
    """Una lettera che puo fare da indice: latina, o greca."""
    return is_letter(token) or token in GREEK

# I limiti dichiarati del programma: un NON DIMOSTRATO con uno di questi motivi
# e onesto, con un altro va guardato.
LIMITS = ("non sa controllare il certificato", "budget di lavoro", "monomio con troppi fattori",
          "troppe quantità distinte", "memoria di lavoro esaurita")

# Il motivo con cui l'oracolo dice che un enunciato non e di questo linguaggio
# perche non ha ne una delta ne un simbolo di Levi-Civita con i suoi indici.
NOT_INDEXED = "senza delta ne simbolo di Levi-Civita"


# ---------------------------------------------------------------------------
# 1. Il lettore dell'oracolo
# ---------------------------------------------------------------------------
#
# Un albero e una tupla: ("num", Fraction), ("sym", tipo, nome, indici,
# posizioni) con gli indici lettere o interi e le posizioni vere per quelli in
# alto, ("neg", a), ("add", a, b), ("sub", a, b), ("mul", a, b). Il tipo di un
# simbolo: "d" la delta, "e" il simbolo di Levi-Civita, "t" un tensore
# qualsiasi, "g" la metrica dichiarata di un enunciato, che vale secondo le
# posizioni dei suoi indici; "L" e "U" la metrica e la sua inversa in un
# certificato, dove le posizioni non ci sono.

TOKEN = re.compile(r"\\\\|\\[a-zA-Z]+|\\[{}]|\\[ ,;!:]|[0-9]+|[a-zA-Z]|\S")
SKIPPED = {"\\ ", "\\,", "\\;", "\\!", "\\:", "\\displaystyle", "\\left", "\\right", "\\quad"}
SYMBOLS = {"\\delta": "d", "\\varepsilon": "e", "\\epsilon": "e"}


class Reader:
    """Discesa ricorsiva sui token di un enunciato."""

    def __init__(self, text):
        self.tokens = [t for t in TOKEN.findall(text) if t not in SKIPPED]
        self.pos = 0
        self.metric_name = None
        self.values = False  # vero per i valori scritti dal programma: li un numero in alto e un indice
        self.sums = []      # (lettera, quanti valori percorre, albero del prodotto)
        self.symbols = []   # tutti i simboli letti, per i controlli d'insieme
        self.given = {}     # tensori dati: nome -> {"places", "elements"}; None mentre si leggono i suoi elementi
        self.seen = set()   # i tensori nominati negli elementi delle tabelle
        self.origins = set()  # da dove cominciano i valori degli indici, secondo chi lo dichiara
        self.listed = set()   # le lettere che un'ipotesi elenca come indici
        self.said = []      # le dimensioni dette dalle ipotesi
        self.scalars = False  # vero mentre si legge un elemento della metrica: le lettere sono scalari, e un numero in alto e un esponente
        self.greek = False  # vero se un indice e una lettera greca

    def peek(self):
        return self.tokens[self.pos] if self.pos < len(self.tokens) else None

    def take(self, expected=None):
        token = self.peek()
        if token is None or (expected is not None and token != expected):
            raise Outside("atteso %r, trovato %r" % (expected, token))
        self.pos += 1
        return token

    def number(self):
        token = self.take()
        if not token.isdigit():
            raise Outside("numero atteso")
        return int(token)

    def signed(self):
        """Un numero con segno, intero o frazione di interi."""
        negative = self.peek() == "-"
        if negative:
            self.take()
        if self.peek() in ("\\frac", "\\dfrac", "\\tfrac"):
            self.take()
            self.take("{")
            above = self.number()
            self.take("}")
            self.take("{")
            below = self.number()
            self.take("}")
            if below == 0:
                raise Outside("denominatore nullo")
            result = Fraction(above, below)
        else:
            result = Fraction(self.number())
        return -result if negative else result

    def value_set(self):
        """L'insieme dei valori degli indici, \\{1, 2, 3\\} o \\{0, 1, 2, 3\\}:
        annota da dove cominciano e ritorna quanti sono."""
        self.take("\\{")
        first = self.number()
        if first not in (0, 1):
            raise Outside("i valori cominciano da 0 o da 1")
        count = 1
        while self.peek() == ",":
            self.take()
            if self.number() != first + count:
                raise Outside("i valori vanno in ordine")
            count += 1
        self.take("\\}")
        self.origins.add(first)
        return count

    def power(self, tree):
        """Un esponente dopo una lettera o una parentesi, in un elemento
        della metrica: il prodotto ripetuto."""
        self.take("^")
        if self.peek() == "{":
            self.take()
            times = self.number()
            self.take("}")
        else:
            token = self.take()
            if not token.isdigit() or len(token) != 1:
                raise Outside("esponente")
            times = int(token)
        if not 1 <= times <= 16:
            raise Outside("esponente fuori da 1..16")
        result = tree
        for _ in range(times - 1):
            result = ("mul", result, tree)
        return result

    def metric_entry(self):
        """Un elemento della diagonale: un numero (una Fraction), oppure una
        funzione delle coordinate: la coppia degli alberi del numeratore e
        del denominatore."""
        start = self.pos
        try:
            number = self.signed()
            if self.peek() in (",", ")"):
                return number
        except Outside:
            pass
        self.pos = start
        self.scalars = True
        try:
            negative = self.peek() == "-"
            top = None
            if negative:
                self.take()
            if self.peek() in ("\\frac", "\\dfrac", "\\tfrac"):
                try:
                    self.take()
                    self.take("{")
                    top = self.sum()
                    self.take("}")
                    self.take("{")
                    bottom = self.sum()
                    self.take("}")
                    if self.starts_factor():
                        top = ("mul", top, self.term())
                    if self.peek() not in (",", ")"):
                        top = None
                except Outside:
                    top = None
            if top is None:
                # Non e una frazione sola: una somma, riletta dall'inizio.
                self.pos = start
                top, bottom = self.sum(), ("num", Fraction(1))
            elif negative:
                top = ("neg", top)
        finally:
            self.scalars = False
        for symbol in symbols_of(top) + symbols_of(bottom):
            if symbol[1] != "t" or symbol[3] or symbol[2] in self.given:
                raise Outside("elemento della metrica")
        return top, bottom

    def environment(self):
        """Il nome di un ambiente tra graffe, dopo \\begin o \\end."""
        self.take("{")
        letters = []
        while is_letter(self.peek()):
            letters.append(self.take())
        self.take("}")
        return "".join(letters)

    def element(self):
        """Un elemento di una tabella o di un elenco: una somma senza lettere
        tra gli indici, fatta di numeri e di componenti di tensori qualsiasi,
        con un numero solo davanti ai fattori che moltiplica."""
        sums = len(self.sums)
        tree = self.sum()
        if len(self.sums) != sums:
            raise Outside("segno di somma in un elemento")

        def check(node):
            if node[0] == "num":
                return
            if node[0] == "sym":
                if node[1] != "t" or node[2] in self.given or any(isinstance(i, str) for i in node[3]):
                    raise Outside("elemento di una tabella")
                self.seen.add(node[2])
                return
            if node[0] == "mul" and node[2][0] == "num":
                raise Outside("due numeri uno dopo l'altro in un elemento")
            for child in node[1:]:
                check(child)
        check(tree)
        return tree

    def components(self, rank):
        """Il secondo membro di un'ipotesi che da un tensore per componenti:
        un elenco tra parentesi per un indice, una tabella per due. Ritorna
        gli elementi per righe, e quante sono le righe."""
        if self.peek() == "(":
            if rank != 1:
                raise Outside("un elenco per un tensore che non ha un indice")
            self.take()
            elements = [self.element()]
            while self.peek() == ",":
                self.take()
                elements.append(self.element())
            self.take(")")
            return elements, len(elements)
        self.take("\\begin")
        if rank != 2:
            raise Outside("una tabella per un tensore che non ha due indici")
        name = self.environment()
        if name not in ("pmatrix", "bmatrix", "matrix"):
            raise Outside("ambiente %r" % name)
        rows = [[self.element()]]
        while True:
            if self.peek() == "&":
                self.take()
                rows[-1].append(self.element())
                continue
            if self.peek() != "\\\\":
                break
            self.take()
            if self.peek() == "\\end":
                break
            rows.append([self.element()])
        self.take("\\end")
        if self.environment() != name:
            raise Outside("la tabella si chiude con un altro ambiente")
        if any(len(row) != len(rows) for row in rows):
            raise Outside("tabella non quadrata")
        return [element for row in rows for element in row], len(rows)

    def hypotheses(self):
        """Le ipotesi davanti a \\implies: la metrica diagonale, il valore di
        epsilon, i tensori dati per componenti, i valori degli indici.
        Ritorna il contesto della metrica, o None se non ne dichiarano una."""
        metric = {"written": True}
        while True:
            token = self.take()
            if token in SYMBOLS and SYMBOLS[token] == "e":
                if "eps" in metric:
                    raise Outside("epsilon dichiarato due volte")
                where = self.take()
                if where not in ("_", "^"):
                    raise Outside("ipotesi su epsilon")
                self.take("{")
                digits = self.take()
                self.take("}")
                self.take("=")
                # Gli indici in ordine, a partire da 1 o da 0: dicono anche
                # da dove cominciano i valori.
                if not digits.isdigit() or digits[0] not in "01" or digits != "".join(str(int(digits[0]) + k) for k in range(len(digits))):
                    raise Outside("indici di epsilon non in ordine")
                self.origins.add(int(digits[0]))
                metric["eps"], metric["eps_up"], metric["eps_digits"] = self.signed(), where == "^", len(digits)
                if metric["eps"] == 0:
                    raise Outside("epsilon dichiarato zero")
            elif is_index(token) and self.peek() in (",", "\\in"):
                # I valori degli indici: le lettere elencate sono indici.
                letters = [token]
                while self.peek() == ",":
                    self.take()
                    letters.append(self.take())
                    if not is_index(letters[-1]):
                        raise Outside("elenco di indici")
                self.take("\\in")
                self.listed.update(letters)
                self.greek = self.greek or any(letter in GREEK for letter in letters)
                self.said.append(self.value_set())
            elif is_letter(token):
                # Un tensore con i suoi indici, lettere tutte diverse: la
                # metrica, o un tensore dato per componenti.
                indices, places = self.groups("t", token)
                if not indices or any(not isinstance(i, str) for i in indices) or len(set(indices)) != len(indices):
                    raise Outside("gli indici del tensore di un'ipotesi")
                self.take("=")
                if self.peek() in ("\\operatorname", "\\mathrm"):
                    self.take()
                    if "c" in metric:
                        raise Outside("due metriche")
                    if len(indices) != 2 or places[0] != places[1] or token in self.given or token in self.seen:
                        raise Outside("ipotesi sulla metrica")
                    self.take("{")
                    for letter in "diag":
                        self.take(letter)
                    self.take("}")
                    self.take("(")
                    entries = [self.metric_entry()]
                    while self.peek() == ",":
                        self.take()
                        entries.append(self.metric_entry())
                    self.take(")")
                    # Un elemento che e una funzione vale la frazione dei
                    # polinomi dei suoi due alberi, che non hanno indici: ne
                    # il numeratore ne il denominatore possono essere nulli.
                    for k, entry in enumerate(entries):
                        if isinstance(entry, tuple):
                            if any(s[2] == token for tree in entry for s in symbols_of(tree)):
                                raise Outside("la metrica tra i suoi elementi")
                            above, below = value(entry[0], 1, {}), value(entry[1], 1, {})
                            if not above or not below:
                                raise Outside("elemento della metrica identicamente zero")
                            entries[k] = Ratio(above, below)
                    if any(not isinstance(c, Ratio) and c == 0 for c in entries):
                        raise Outside("zero sulla diagonale")
                    metric["c"] = [1 / c for c in entries] if places[0] else entries
                    metric["name"] = self.metric_name = token
                    self.said.append(len(entries))
                else:
                    if token in self.given or token in self.seen or token == self.metric_name:
                        raise Outside("la lettera di un tensore dato e gia usata")
                    # Mentre si leggono, gli elementi non lo possono nominare.
                    self.given[token] = None
                    elements, size = self.components(len(indices))
                    self.given[token] = {"places": tuple(places), "elements": elements}
                    self.said.append(size)
            else:
                raise Outside("ipotesi non letta")
            token = self.take()
            if token == "\\implies":
                break
            if token != "\\land":
                raise Outside("tra due ipotesi serve \\land")
        if "eps" in metric and "c" not in metric:
            raise Outside("manca la metrica")
        return metric if "c" in metric else None

    def starts_factor(self):
        token = self.peek()
        return token is not None and (token.isdigit() or (len(token) == 1 and token.isalpha()) or token == "("
                                      or token in SYMBOLS or token in ("\\frac", "\\dfrac", "\\tfrac"))

    def sum(self):
        negative = self.peek() == "-"
        if self.peek() in ("+", "-"):
            self.take()
        tree = self.term()
        if negative:
            tree = ("neg", tree)
        while self.peek() in ("+", "-"):
            kind = "add" if self.take() == "+" else "sub"
            tree = (kind, tree, self.term())
        return tree

    def term(self):
        signs = []
        while self.peek() == "\\sum":
            self.take()
            self.take("_")
            self.take("{")
            letter = self.take()
            if not is_index(letter):
                raise Outside("indice di somma")
            self.greek = self.greek or letter in GREEK
            self.take("=")
            first = self.number()
            if first not in (0, 1):
                raise Outside("una somma comincia da 1 o da 0")
            self.origins.add(first)
            self.take("}")
            self.take("^")
            if self.peek() == "{":
                self.take()
                upper = self.number()
                self.take("}")
            else:
                token = self.take()
                if not token.isdigit() or len(token) != 1:
                    raise Outside("limite della somma")
                upper = int(token)
            if upper < 1:
                raise Outside("limite della somma")
            signs.append((letter, upper - first + 1))
        tree = self.factor()
        while True:
            if self.peek() == "\\cdot":
                self.take()
                if not self.starts_factor():
                    raise Outside("fattore atteso dopo \\cdot")
            elif not self.starts_factor():
                break
            tree = ("mul", tree, self.factor())
        self.sums.extend((letter, count, tree) for letter, count in signs)
        return tree

    def factor(self):
        token = self.peek()
        if token is None:
            raise Outside("enunciato incompleto")
        if token.isdigit():
            self.take()
            return ("num", Fraction(int(token)))
        if token in ("\\frac", "\\dfrac", "\\tfrac"):
            self.take()
            self.take("{")
            above = self.number()
            self.take("}")
            self.take("{")
            below = self.number()
            self.take("}")
            if below == 0:
                raise Outside("denominatore nullo")
            return ("num", Fraction(above, below))
        if token == "(":
            self.take()
            tree = self.sum()
            self.take(")")
            if self.peek() == "^":
                if not self.scalars:
                    raise Outside("potenza")
                tree = self.power(tree)
            return tree
        if self.scalars and is_letter(token):
            # In un elemento della metrica una lettera e uno scalare, senza
            # indici da leggere; cio che la segue in alto e un esponente.
            self.take()
            symbol = ("sym", "t", token, (), ())
            self.symbols.append(symbol)
            return self.power(symbol) if self.peek() == "^" else symbol
        if token in SYMBOLS or (len(token) == 1 and token.isalpha()):
            self.take()
            kind = "g" if token == self.metric_name else SYMBOLS.get(token, "t")
            indices, places = self.groups(kind, token)
            symbol = ("sym", kind, token if kind in "tg" else "", tuple(indices), tuple(places))
            self.symbols.append(symbol)
            return symbol
        raise Outside("simbolo inatteso %r" % token)

    def groups(self, kind, name):
        indices, places = [], []
        while True:
            if self.peek() == "{" and self.pos + 1 < len(self.tokens) and self.tokens[self.pos + 1] == "}":
                self.pos += 2
                continue
            if self.peek() not in ("_", "^"):
                return indices, places
            upper = self.take() == "^"
            entries = []
            if self.peek() == "{":
                self.take()
                while self.peek() != "}":
                    entries.append((self.take(), True))
                self.take("}")
                if not entries:
                    raise Outside("gruppo di indici vuoto")
            else:
                entries.append((self.take(), False))
            for token, braced in entries:
                if is_index(token):
                    self.greek = self.greek or token in GREEK
                    indices.append(token)
                    places.append(upper)
                elif token.isdigit():
                    # Un numero in alto: solo con una metrica, e non accanto a
                    # un tensore qualsiasi, dove potrebbe essere una potenza;
                    # oppure nei valori scritti dal programma, o accanto a un
                    # tensore dato con una tabella, che ha sempre due indici.
                    table = kind == "t" and self.given.get(name) is not None and len(self.given[name]["places"]) == 2
                    if (upper and not self.values and not table and (self.metric_name is None or kind == "t")) or (not braced and len(token) > 1):
                        raise Outside("indice numerico")
                    for digit in token:
                        indices.append(int(digit))
                        places.append(upper)
                else:
                    raise Outside("indice %r" % token)


def read_statement(text, known_dimension=0, known_origin=1):
    """Legge un enunciato. Ritorna (dimensione, sinistro, destro, liberi), e
    lascia in METRIC la metrica dichiarata, o None, in GIVEN i tensori dati
    per componenti e in ORIGIN il numero da cui cominciano i valori degli
    indici. Con known_dimension il testo sono i valori dei due membri scritti
    dal programma, in quella dimensione e con i valori da known_origin."""
    global METRIC, GIVEN, ORIGIN
    METRIC, GIVEN, ORIGIN = None, {}, 1
    reader = Reader(text)
    reader.values = bool(known_dimension)
    metric = reader.hypotheses() if "\\implies" in reader.tokens else None
    declared, listed = set(), 0
    while reader.peek() == "\\forall":
        reader.take()
        while True:
            letter = reader.take()
            if not is_index(letter):
                raise Outside("indice quantificato")
            reader.greek = reader.greek or letter in GREEK
            declared.add(letter)
            if reader.peek() != ",":
                break
            reader.take()
        reader.take("\\in")
        count = reader.value_set()
        if listed and listed != count:
            raise Outside("due insiemi di valori")
        listed = count

    # Tutta l'identita tra parentesi dopo i quantificatori, oppure no. I
    # simboli letti nelle ipotesi restano.
    start, kept = reader.pos, len(reader.symbols)
    sides = None
    if listed and reader.peek() == "(":
        try:
            reader.take()
            left = reader.sum()
            reader.take("=")
            right = reader.sum()
            reader.take(")")
            if reader.peek() is None:
                sides = (left, right)
        except Outside:
            pass
    if sides is None:
        reader.pos, reader.sums = start, []
        del reader.symbols[kept:]
        left = reader.sum()
        reader.take("=")
        right = reader.sum()
        if reader.peek() is not None:
            raise Outside("testo dopo l'identita")
        sides = (left, right)
    left, right = sides

    # Le convenzioni del linguaggio che riguardano com'e scritto l'enunciato:
    # i tensori, la dimensione dichiarata, i quantificatori, i segni di somma.
    # Un enunciato con gli indici si riconosce da una delta o da un simbolo
    # di Levi-Civita con i suoi indici: senza, il programma lo legge in un
    # altro linguaggio (a_{i} e il termine di una successione).
    if not known_dimension and not metric and not any(s[1] in "de" and s[3] for s in reader.symbols):
        raise Outside(NOT_INDEXED)
    used = {i for s in reader.symbols for i in s[3] if isinstance(i, str)} | {letter for letter, _, _ in reader.sums} | reader.listed
    # Senza metrica un tensore, e anche epsilon, ha gli indici sempre nelle
    # stesse posizioni; con la metrica conta solo quanti sono, e le posizioni
    # della prima occorrenza sono quelle dichiarate. Per un tensore dato la
    # prima occorrenza e la sua ipotesi.
    shapes, reference = {}, {}
    for name, given in reader.given.items():
        shapes[name] = len(given["places"]) if metric else (len(given["places"]), given["places"])
        reference[name] = given["places"]
    for _, kind, name, indices, places in reader.symbols:
        if kind == "t":
            shape = len(indices) if metric else (len(indices), places)
            if shapes.setdefault(name, shape) != shape:
                raise Outside("il tensore %s e scritto in due modi" % name)
            reference.setdefault(name, places)
        if kind == "g" and len(indices) != 2:
            raise Outside("la metrica ha due indici")
    if not metric and len({s[4] for s in reader.symbols if s[1] == "e"}) > 1:
        raise Outside("epsilon con gli indici in posizioni diverse senza una metrica")
    if used & set(shapes) or (metric and metric["name"] in used):
        raise Outside("una lettera e sia indice sia nome")
    epsilons = {len(s[3]) for s in reader.symbols if s[1] == "e"}
    if len(epsilons) > 1 or any(rank < 2 for rank in epsilons):
        raise Outside("simboli di Levi-Civita")
    said = {d for d in [listed, known_dimension] + reader.said + list(epsilons) + [count for _, count, _ in reader.sums] if d}
    if len(said) != 1:
        raise Outside("dimensione non dichiarata, o dichiarata in due modi")
    dimension = said.pop()
    # Da dove cominciano i valori: da 1 se nessuno lo dice; con un indice
    # greco va detto. I numeri scritti come indici negli elementi delle
    # tabelle devono essere tra i valori, come quelli dell'identita
    # (free_of()).
    if known_dimension:
        reader.origins.add(known_origin)
    if len(reader.origins) > 1 or (reader.greek and not reader.origins):
        raise Outside("i valori degli indici non sono dichiarati, o lo sono in due modi")
    origin = reader.origins.pop() if reader.origins else 1
    for given in reader.given.values():
        for element in given["elements"]:
            if any(not origin <= i <= origin + dimension - 1 for s in symbols_of(element) for i in s[3]):
                raise Outside("indice numerico fuori dai valori in un elemento")
    if metric:
        if epsilons and "eps" not in metric:
            raise Outside("manca il valore di epsilon tra le ipotesi")
        if "eps" in metric and metric["eps_digits"] != dimension:
            raise Outside("epsilon dichiarato con un altro numero di indici")
        metric["reference"] = reference
    METRIC, ORIGIN = metric, origin
    GIVEN = {name: given["elements"] for name, given in reader.given.items()}
    free = conventions(left, right, dimension)
    if listed and declared != free:
        raise Outside("i quantificatori non nominano gli indici liberi")
    for letter, _, tree in reader.sums:
        if not any(count_letters(factors).get(letter) == 2 for _, factors in expand(tree)):
            raise Outside("segno di somma su un indice non ripetuto")
    return dimension, left, right, free


# ---------------------------------------------------------------------------
# 2. La convenzione di Einstein, svolgendo le parentesi
# ---------------------------------------------------------------------------

def expand(tree):
    """Ritorna la somma di prodotti: [(coefficiente, [simboli])]."""
    kind = tree[0]
    if kind == "num":
        return [(tree[1], [])]
    if kind == "sym":
        return [(Fraction(1), [tree])]
    if kind == "neg":
        return [(-c, f) for c, f in expand(tree[1])]
    if kind in ("add", "sub"):
        sign = 1 if kind == "add" else -1
        return expand(tree[1]) + [(sign * c, f) for c, f in expand(tree[2])]
    return [(a * b, f + g) for a, f in expand(tree[1]) for b, g in expand(tree[2])]


def count_letters(factors):
    counts = {}
    for symbol in factors:
        for index in symbol[3]:
            if isinstance(index, str):
                counts[index] = counts.get(index, 0) + 1
    return counts


def zero_alone(tree):
    """Lo 0 scritto da solo: il tensore nullo, con gli indici liberi dell'altro."""
    return tree[0] == "num" and tree[1] == 0


def free_of(tree, dimension, ranks):
    """Gli indici liberi di un membro, o None per lo 0 da solo. Controlla i
    simboli e la convenzione in ogni prodotto svolto, e che i prodotti abbiano
    tutti gli stessi indici liberi: tranne gli addendi che sono lo 0 da solo,
    che si riconoscono scendendo nelle somme prima di svolgere."""
    if zero_alone(tree):
        return None
    if tree[0] in ("add", "sub"):
        sets = [f for f in (free_of(tree[1], dimension, ranks), free_of(tree[2], dimension, ranks)) if f is not None]
        if len(sets) == 2 and sets[0] != sets[1]:
            raise Outside("addendi con indici liberi diversi")
        return sets[0] if sets else frozenset()
    result = None
    written = bool(METRIC and METRIC.get("written"))
    for _, factors in expand(tree):
        for _, kind, name, indices, places in factors:
            if kind in "dgLU" and len(indices) != 2:
                raise Outside("delta o metrica con %d indici" % len(indices))
            if kind in "gLU" and not METRIC:
                raise Outside("la metrica senza la sua dichiarazione")
            if kind == "d" and written and places[0] == places[1]:
                raise Outside("con una metrica la delta ha un indice in alto e uno in basso")
            if kind == "e" and len(indices) != dimension:
                raise Outside("simbolo di Levi-Civita con %d indici in dimensione %d" % (len(indices), dimension))
            if kind == "t" and ranks.setdefault(name, len(indices)) != len(indices):
                raise Outside("tensore con due ranghi")
            if any(isinstance(i, int) and not ORIGIN <= i <= ORIGIN + dimension - 1 for i in indices):
                raise Outside("indice numerico fuori dai valori")
        counts = count_letters(factors)
        if any(n > 2 for n in counts.values()):
            raise Outside("un indice compare piu di due volte in un prodotto")
        free = frozenset(letter for letter, n in counts.items() if n == 1)
        if written:
            # Dove sta ogni lettera: una sommata una volta in alto e una in
            # basso; una libera porta con se la sua posizione, che deve
            # essere la stessa in ogni prodotto.
            where = {}
            for symbol in factors:
                for index, up in zip(symbol[3], symbol[4]):
                    if isinstance(index, str):
                        where.setdefault(index, []).append(up)
            if any(len(ups) == 2 and ups[0] == ups[1] for ups in where.values()):
                raise Outside("un indice sommato compare due volte nella stessa posizione")
            free = frozenset((letter, ups[0]) for letter, ups in where.items() if len(ups) == 1)
        if result is not None and free != result:
            raise Outside("prodotti con indici liberi diversi")
        result = free
    return result


def conventions(left, right, dimension):
    """Ritorna gli indici liberi dell'identita, o solleva Outside."""
    ranks = {}
    sets = [f for f in (free_of(left, dimension, ranks), free_of(right, dimension, ranks)) if f is not None]
    if len(sets) == 2 and sets[0] != sets[1]:
        raise Outside("membri con indici liberi diversi")
    return {x[0] if isinstance(x, tuple) else x for x in sets[0]} if sets else set()


# ---------------------------------------------------------------------------
# 3. Il conto per forza bruta
# ---------------------------------------------------------------------------

def sign_of(values):
    """Segno della permutazione values di 1..n, dai cicli: un ciclo di
    lunghezza L vale L - 1 scambi. 0 se values non e una permutazione."""
    n = len(values)
    if sorted(values) != list(range(1, n + 1)):
        return 0
    seen, swaps = [False] * n, 0
    for start in range(n):
        length, at = 0, start
        while not seen[at]:
            seen[at] = True
            at = values[at] - 1
            length += 1
        swaps += max(length - 1, 0)
    return -1 if swaps % 2 else 1


def value(tree, dimension, point):
    """Il valore di un membro nel punto: un polinomio nelle componenti dei
    tensori qualsiasi, come dizionario da monomio (componenti in ordine) a
    coefficiente; con una metrica di funzioni, una frazione di polinomi
    (Ratio), se un suo elemento entra nel conto. I valori degli indici vanno
    da ORIGIN in su."""
    total, ratio = {}, None
    for coefficient, factors in expand(tree):
        counts = count_letters(factors)
        dummies = sorted(letter for letter, n in counts.items() if n == 2)
        for choice in itertools.product(range(ORIGIN, ORIGIN + dimension), repeat=len(dummies)):
            where = dict(point)
            where.update(zip(dummies, choice))
            product, scale = {(): Fraction(coefficient)}, None
            written = METRIC and METRIC.get("written")
            for _, kind, name, indices, places in factors:
                at = [where[i] if isinstance(i, str) else i for i in indices]
                number, declared, component = Fraction(1), None, None
                if kind == "d":
                    number = Fraction(1 if at[0] == at[1] else 0)
                elif kind in "gLU":
                    # La metrica diagonale: c in basso, 1/c in alto, la delta
                    # con un indice in alto e uno in basso.
                    c = METRIC["c"][at[0] - ORIGIN]
                    if at[0] != at[1]:
                        number = Fraction(0)
                    elif kind == "L" or (kind == "g" and not places[0] and not places[1]):
                        number = c
                    elif kind == "U" or (kind == "g" and places[0] and places[1]):
                        number = 1 / c
                elif kind == "e":
                    number = Fraction(sign_of([v - ORIGIN + 1 for v in at]))
                    if written:
                        number *= METRIC["eps"]
                        declared = (METRIC["eps_up"],) * len(at)
                else:
                    if name in GIVEN:
                        # Un tensore dato: l'elemento di quella riga e di
                        # quella colonna, che non ha lettere tra gli indici.
                        row = 0
                        for v in at:
                            row = row * dimension + v - ORIGIN
                        component = value(GIVEN[name][row], dimension, {})
                    else:
                        component = {((name, tuple(at)),): Fraction(1)}
                    if written:
                        declared = METRIC["reference"][name]
                # Un indice fuori dalla posizione dichiarata e stato alzato
                # (per 1/c) o abbassato (per c).
                for k in range(len(at) if declared else 0):
                    if places[k] != declared[k]:
                        number = number / METRIC["c"][at[k] - ORIGIN] if places[k] else number * METRIC["c"][at[k] - ORIGIN]
                if number == 0:
                    product = {}
                    break
                if isinstance(number, Ratio):
                    # Un elemento di una metrica di funzioni: resta da parte,
                    # e moltiplica il prodotto alla fine.
                    scale = number if scale is None else scale * number
                else:
                    product = {key: c * number for key, c in product.items()}
                if isinstance(component, Ratio):
                    scale = component if scale is None else scale * component
                elif component is not None:
                    product = poly_times(product, component)
                if not product:
                    break
            if scale is not None and product:
                ratio = Ratio(product) * scale if ratio is None else ratio + Ratio(product) * scale
            else:
                for key, c in product.items():
                    total[key] = total.get(key, 0) + c
    total = {key: c for key, c in total.items() if c != 0}
    return total if ratio is None else ratio + total


def judge(text):
    """Ritorna ("TRUE", stato), ("FALSE", stato) o ("OUTSIDE", motivo)."""
    try:
        dimension, left, right, free = read_statement(text)
    except Outside as problem:
        return "OUTSIDE", str(problem)
    except RecursionError:
        return "OUTSIDE", "troppo annidato"
    letters = sorted(free)
    state = {"dimension": dimension, "left": left, "right": right, "letters": letters, "failing": [], "metric": METRIC, "given": GIVEN, "origin": ORIGIN}
    for choice in itertools.product(range(ORIGIN, ORIGIN + dimension), repeat=len(letters)):
        point = dict(zip(letters, choice))
        if value(left, dimension, point) != value(right, dimension, point):
            state["failing"].append(point)
    return ("FALSE" if state["failing"] else "TRUE"), state


# ---------------------------------------------------------------------------
# 4. Scrivere: LaTeX per il programma, certificati per il verificatore
# ---------------------------------------------------------------------------

# Vero mentre si scrive un enunciato con una metrica: le posizioni degli
# indici si scrivono come sono.
EXACT = False


def latex(tree, rng, top=True):
    kind = tree[0]
    if kind == "num":
        c = tree[1]
        return str(c.numerator) if c.denominator == 1 else "\\frac{%d}{%d}" % (c.numerator, c.denominator)
    if kind == "sym":
        _, what, name, indices, places = tree
        head = name if what in "tg" else "\\delta" if what == "d" else rng.choice(("\\varepsilon", "\\varepsilon", "\\epsilon"))
        # In alto o in basso e lo stesso per la delta e per il simbolo di
        # Levi-Civita: ogni tanto si scrivono in alto gli indici letterali.
        # Non con una metrica (EXACT), dove le posizioni contano.
        if what != "t" and not EXACT and rng.random() < 0.2:
            raised = rng.choice(((True,) * len(indices), (True,) + (False,) * (len(indices) - 1)))
            places = tuple(up and isinstance(i, str) for up, i in zip(raised, indices))
        text, i, groups = head, 0, 0
        while i < len(indices):
            j = i
            while j < len(indices) and places[j] == places[i]:
                j += 1
            # Tra due gruppi le graffe vuote che li tengono distanti,
            # T^{i}{}_{j}: dal terzo gruppo ci sono sempre, perche in TeX due
            # pedici di seguito sono un errore.
            if groups >= 2 or (groups == 1 and rng.random() < 0.5):
                text += "{}"
            # Dopo una lettera greca uno spazio: \mui sarebbe un altro comando.
            text += ("^" if places[i] else "_") + "{" + "".join(str(x) + " " * (x in GREEK) for x in indices[i:j]).rstrip() + "}"
            i, groups = j, groups + 1
        return text
    if kind == "neg":
        inner = latex(tree[1], rng, False)
        return "-" + (inner if tree[1][0] in ("sym", "num", "mul") else "\\left( %s \\right)" % inner)
    if kind in ("add", "sub"):
        right = latex(tree[2], rng, False)
        if tree[2][0] in ("add", "sub", "neg"):
            right = "(%s)" % right
        text = "%s %s %s" % (latex(tree[1], rng, False), "+" if kind == "add" else "-", right)
        return text
    parts = []
    for side in (tree[1], tree[2]):
        inner = latex(side, rng, False)
        parts.append("(%s)" % inner if side[0] in ("add", "sub", "neg") else inner)
    return parts[0] + rng.choice((" ", " ", "", " \\cdot ")) + parts[1] if not parts[1][0].isdigit() else parts[0] + " \\cdot " + parts[1]


def certificate_of(tree):
    kind = tree[0]
    if kind == "num":
        c = tree[1]
        return str(c.numerator) if c.denominator == 1 else "(frac %d %d)" % (c.numerator, c.denominator)
    if kind == "sym":
        _, what, name, indices, _ = tree
        head = "t " + name if what == "t" else {"d": "delta", "e": "eps", "L": "metric", "U": "inverse"}[what]
        return "(" + " ".join([head] + [str(i) for i in indices]) + ")"
    if kind == "neg":
        return "(neg %s)" % certificate_of(tree[1])
    return "(%s %s %s)" % ({"add": "+", "sub": "-", "mul": "*"}[kind], certificate_of(tree[1]), certificate_of(tree[2]))


def polynomial_tree(poly):
    """Un albero con il valore del polinomio poly, con gli indici numerici."""
    tree = None
    for key in sorted(poly):
        term = ("num", abs(Fraction(poly[key])))
        for name, at in key:
            term = ("mul", term, ("sym", "t", name, at, (False,) * len(at)))
        if tree is None:
            tree = ("neg", term) if poly[key] < 0 else term
        else:
            tree = ("sub" if poly[key] < 0 else "add", tree, term)
    return tree if tree is not None else ("num", Fraction(0))


# ---------------------------------------------------------------------------
# 5. Generare enunciati
# ---------------------------------------------------------------------------

INDEX_LETTERS = "ijklmnpqrs"
VECTORS, MATRICES = "abcuvw", "STM"

# Identita note, da alterare: il seme del caso vero. Le lettere degli indici
# vengono poi rinominate a caso.
KNOWN = [
    r"\varepsilon_{ijk}\varepsilon_{imn} = \delta_{jm}\delta_{kn} - \delta_{jn}\delta_{km}",
    r"\varepsilon_{ijk}\varepsilon_{ljk} = 2\delta_{il}",
    r"\varepsilon_{ijk}\varepsilon_{ijk} = 6",
    r"\varepsilon_{ij}\varepsilon_{kl} = \delta_{ik}\delta_{jl} - \delta_{il}\delta_{jk}",
    r"\varepsilon_{ijkl}\varepsilon_{ijkm} = 6\delta_{lm}",
    r"\varepsilon_{ijk} = \varepsilon_{jki}",
    r"\varepsilon_{ijk}\delta_{jk} = 0",
    r"\varepsilon_{ijk} a_{j} a_{k} = 0",
    r"\varepsilon_{ijk} a_{j} b_{k} = -\varepsilon_{ijk} b_{j} a_{k}",
    r"\varepsilon_{ijk}\varepsilon_{klm} a_{j} b_{l} c_{m} = b_{i} a_{m} c_{m} - c_{i} a_{l} b_{l}",
    r"\varepsilon_{ijk}\varepsilon_{imn} a_{j} b_{k} a_{m} b_{n} = a_{j} a_{j} b_{k} b_{k} - a_{j} b_{j} a_{k} b_{k}",
    r"\varepsilon_{ijk} a_{i} b_{j} c_{k} = \varepsilon_{ijk} b_{i} c_{j} a_{k}",
    r"\varepsilon_{ijk} S_{jk} = \frac{1}{2}\varepsilon_{ijk}(S_{jk} - S_{kj})",
    r"\forall i, k \in \{1, 2, 3\}\ (\delta_{ij}\delta_{jk} = \delta_{ik})",
    r"\forall i \in \{1, 2, 3\}\ (\delta_{ij} a_{j} = a_{i})",
    r"\forall i, k \in \{1, 2\}\ (\delta_{ij} S_{jk} = S_{ik})",
    r"\sum_{i=1}^{3} \delta_{ii} = 3",
    r"\sum_{i=1}^{4} \delta_{ij}\delta_{ij} = 4",
    r"\varepsilon_{ij} S_{i1} S_{j2} = S_{11} S_{22} - S_{21} S_{12}",
]


def map_tree(tree, change):
    """Ritorna tree con ogni simbolo sostituito da change(simbolo)."""
    if tree[0] == "sym":
        return change(tree)
    if tree[0] == "num":
        return tree
    return (tree[0],) + tuple(map_tree(child, change) for child in tree[1:])


def symbols_of(tree):
    if tree[0] == "sym":
        return [tree]
    if tree[0] == "num":
        return []
    return [s for child in tree[1:] for s in symbols_of(child)]


def random_monomial(rng, dimension, free, dummies, use_epsilon):
    """Un prodotto in cui ogni lettera di free compare una volta e ogni
    lettera di dummies due: i posti si riempiono con simboli presi a caso."""
    # In ordine: un insieme di lettere non ha un ordine suo, e con lo stesso
    # seme due esecuzioni davano enunciati diversi.
    slots = sorted(free) + list(dummies) * 2
    if rng.random() < 0.15 or not slots:
        slots.append(rng.randint(1, dimension))
    rng.shuffle(slots)
    factors = []
    vectors, matrices = list(VECTORS), list(MATRICES)
    while slots:
        options = ["v"]
        if len(slots) >= 2:
            options += ["d", "m"]
        if use_epsilon and len(slots) >= dimension:
            options += ["e", "e"]
        what = rng.choice(options)
        size = {"v": 1, "d": 2, "m": 2, "e": dimension}[what]
        indices, slots = tuple(slots[:size]), slots[size:]
        if what in ("d", "e"):
            factors.append(("sym", what, "", indices, (False,) * size))
        else:
            name = rng.choice(vectors if what == "v" else matrices)
            factors.append(("sym", "t", name, indices, (False,) * size))
    rng.shuffle(factors)
    tree = factors[0]
    for factor in factors[1:]:
        tree = ("mul", tree, factor)
    coefficient = rng.choice((1, 1, 1, 2, 3, -1, Fraction(1, 2)))
    if coefficient == -1:
        return ("neg", tree)
    return tree if coefficient == 1 else ("mul", ("num", Fraction(coefficient)), tree)


def random_side(rng, dimension, free, use_epsilon):
    count = rng.choice((1, 1, 2, 2, 3))
    tree = None
    for _ in range(count):
        pool = [c for c in INDEX_LETTERS if c not in free]
        dummies = rng.sample(pool, rng.choice((0, 1, 1, 2)))
        term = random_monomial(rng, dimension, free, dummies, use_epsilon)
        tree = term if tree is None else (rng.choice(("add", "add", "sub")), tree, term)
    return tree


def perturb(rng, tree, dimension):
    """Una piccola alterazione: due indici scambiati, un indice cambiato, un
    segno, un fattore. Puo lasciare l'enunciato vero, o renderlo mal formato:
    lo decide l'oracolo."""
    choice = rng.randrange(5)
    symbols = [s for s in symbols_of(tree) if len(s[3]) >= 1]
    if choice <= 1 and symbols:
        target = rng.choice(symbols)
        indices = list(target[3])
        if choice == 0 and len(indices) >= 2:
            a, b = rng.sample(range(len(indices)), 2)
            indices[a], indices[b] = indices[b], indices[a]
        else:
            indices[rng.randrange(len(indices))] = rng.choice(list(INDEX_LETTERS[:6]) + [rng.randint(1, dimension)])
        done = []

        def change(symbol):
            if symbol is target and not done:
                done.append(1)
                return symbol[:3] + (tuple(indices),) + symbol[4:]
            return symbol
        return map_tree(tree, change)
    if choice == 2:
        return ("neg", tree)
    if choice == 3:
        return ("mul", ("num", Fraction(rng.choice((2, 3)))), tree)
    return ("add", tree, tree) if tree[0] != "num" else tree


def declare(rng, dimension, left, right, free, body):
    """Il testo dell'enunciato con la sua dimensione dichiarata, quando il
    modo c'e: il simbolo di Levi-Civita, i quantificatori, un segno di somma."""
    has_epsilon = any(s[1] == "e" for s in symbols_of(left) + symbols_of(right))
    if free and (not has_epsilon or rng.random() < 0.3):
        listed = ", ".join(str(v) for v in range(1, dimension + 1))
        wrap = rng.choice(("\\left( %s \\right)", "(%s)", "%s"))
        return "\\forall %s \\in \\{%s\\}\\ %s" % (", ".join(sorted(free)), listed, wrap % body)
    if has_epsilon:
        return body
    for side_tree in (left,):
        first = side_tree
        while first[0] in ("add", "sub"):
            first = first[1]
        counts = count_letters([s for _, f in expand(first) for s in f]) if first[0] != "neg" else {}
        repeated = sorted(letter for letter, n in counts.items() if n == 2)
        if repeated and len(expand(first)) == 1:
            return "\\sum_{%s=1}^{%d} %s" % (repeated[0], dimension, body)
    return body  # senza dichiarazione: fuori linguaggio, e l'oracolo lo sa


def factored_side(rng, dimension, free, use_epsilon):
    """Un membro (X + Y) Z, con una o due lettere sommate tra la parentesi e
    Z: la somma avviene in un prodotto che ha una somma per fattore, e chi
    somma nodo per nodo deve trovarla li."""
    pool = [c for c in INDEX_LETTERS if c not in free]
    rng.shuffle(pool)
    count = rng.choice((1, 1, 2))
    shared, inner, outer = set(pool[:count]), pool[count:count + 2], pool[count + 2:count + 3]
    ordered = sorted(free)
    cut = rng.randint(0, len(ordered))
    first, second = set(ordered[:cut]) | shared, set(ordered[cut:]) | shared
    bracket = (rng.choice(("add", "sub")),
               random_monomial(rng, dimension, first, rng.sample(inner, rng.choice((0, 1))), use_epsilon),
               random_monomial(rng, dimension, first, rng.sample(inner, rng.choice((0, 1))), use_epsilon))
    other = random_monomial(rng, dimension, second, outer[:rng.choice((0, 1))], use_epsilon)
    return ("mul", bracket, other) if rng.random() < 0.5 else ("mul", other, bracket)


# ---------------------------------------------------------------------------
# 5b. Generare enunciati con una metrica dichiarata
# ---------------------------------------------------------------------------
#
# Chi genera mette le posizioni degli indici nodo per nodo, per avere
# enunciati quasi sempre ben formati. Non e l'oracolo: se sbaglia, esce un
# enunciato fuori linguaggio, e a dirlo e sempre judge(), che svolge le
# parentesi e guarda le posizioni prodotto per prodotto.

METRIC_NAMES = "gggGh"

# Identita note con la metrica dichiarata: valgono per ogni metrica diagonale
# (alzare e abbassare, la traccia) o proprio per quella scritta (i prodotti
# di due epsilon, dove entra il determinante).
KNOWN_METRIC = [
    r"g_{ij} = \operatorname{diag}(-1, 1, 1, 1) \land \varepsilon_{1234} = 1 \implies \varepsilon^{ijkl}\varepsilon_{ijkl} = -24",
    r"g_{ij} = \operatorname{diag}(-1, 1, 1, 1) \land \varepsilon_{1234} = 1 \implies \varepsilon^{ijkl}\varepsilon_{ijkm} = -6\delta^{l}_{m}",
    r"g_{ij} = \operatorname{diag}(-1, 1, 1, 1) \land \varepsilon_{1234} = 1 \implies \varepsilon^{ijkl}\varepsilon_{ijmn} = -2(\delta^{k}_{m}\delta^{l}_{n} - \delta^{k}_{n}\delta^{l}_{m})",
    r"g_{ij} = \operatorname{diag}(1, 1, 1) \land \varepsilon_{123} = 1 \implies \varepsilon^{ijk}\varepsilon_{imn} = \delta^{j}_{m}\delta^{k}_{n} - \delta^{j}_{n}\delta^{k}_{m}",
    r"g_{ij} = \operatorname{diag}(2, 3) \land \varepsilon_{12} = 1 \implies \varepsilon^{ij}\varepsilon_{ij} = \frac{1}{3}",
    r"g_{ij} = \operatorname{diag}(1, -1, -1, -1) \implies g^{ij} g_{ij} = 4",
    r"g_{ij} = \operatorname{diag}(1, -1, -1, -1) \implies g^{ij} g_{jk} = \delta^{i}_{k}",
    r"g_{ij} = \operatorname{diag}(1, -1, -1, -1) \implies A^{i} B_{i} = A_{i} B^{i}",
    r"g_{ij} = \operatorname{diag}(1, -1, -1, -1) \implies g_{ij} A^{i} B^{j} = A^{i} B_{i}",
    r"g_{ij} = \operatorname{diag}(1, -1, -1, -1) \implies T^{ij} g_{ij} = T^{i}{}_{i}",
    r"g_{ij} = \operatorname{diag}(-1, 1, 1, 1) \implies F^{ij} F_{ij} = F_{ij} F^{ij}",
    r"g_{ij} = \operatorname{diag}(-1, 1, 1, 1) \implies S_{ij} A^{i} A^{j} = S^{ij} A_{i} A_{j}",
]


def once_letters(tree):
    """Le lettere che in tree compaiono una volta sola in un prodotto: quelle
    libere, lette sul primo prodotto svolto."""
    counts = count_letters(expand(tree)[0][1])
    return {letter for letter, n in counts.items() if n == 1}


def place(rng, tree, where):
    """tree con le posizioni degli indici: le lettere di where dove dice
    where, quelle sommate una volta in alto e una in basso, scelte a caso nel
    nodo che le somma. Un indice numerico va in alto ogni tanto; su un
    tensore qualsiasi, dove sarebbe una potenza e va rifiutato, molto di
    rado: rompe quella regola sola."""
    kind = tree[0]
    if kind == "num":
        return tree
    if kind == "sym":
        inside, places = {}, []
        for index in tree[3]:
            if not isinstance(index, str):
                places.append(rng.random() < (0.3 if tree[1] != "t" else 0.04))
            elif index in where:
                places.append(where[index])
            else:
                inside[index] = not inside[index] if index in inside else rng.random() < 0.5
                places.append(inside[index])
        return tree[:4] + (tuple(places),)
    if kind == "mul":
        sides = [dict(where), dict(where)]
        for letter in sorted(once_letters(tree[1]) & once_letters(tree[2])):
            sides[0][letter] = rng.random() < 0.5
            sides[1][letter] = not sides[0][letter]
        return ("mul", place(rng, tree[1], sides[0]), place(rng, tree[2], sides[1]))
    return (kind,) + tuple(place(rng, child, where) for child in tree[1:])


def with_metric(rng, tree, name):
    """tree con la metrica name al posto delle delta che hanno i due indici
    dalla stessa parte, e ogni tanto anche delle altre: con un indice in alto
    e uno in basso la metrica e la delta. Una volta su venti una delta con i
    due indici dalla stessa parte resta com'e: rompe la sola regola sulla
    delta, con tutte le altre posizioni in ordine, e va rifiutata."""
    def change(symbol):
        if symbol[1] == "d" and rng.random() < (0.95 if symbol[4][0] == symbol[4][1] else 0.2):
            return ("sym", "g", name) + symbol[3:]
        return symbol
    return map_tree(tree, change)


def number_text(q):
    q = Fraction(q)
    return str(q.numerator) if q.denominator == 1 else "%s\\frac{%d}{%d}" % ("-" if q < 0 else "", abs(q.numerator), q.denominator)


def metric_statement(rng, name, entries, epsilon, body):
    """Il testo dell'enunciato body con le sue ipotesi: la metrica name con i
    numeri entries sulla diagonale, una volta su quattro dichiarata dalla sua
    inversa, e il valore di epsilon, se epsilon non e None: (valore, in alto,
    quante cifre)."""
    upper = rng.random() < 0.25
    parts = ["%s%s{ij} = \\%s{diag}(%s)" % (name, "^" if upper else "_", rng.choice(("operatorname", "operatorname", "mathrm")),
                                            ", ".join(number_text(1 / c if upper else c) for c in entries))]
    if epsilon:
        parts.append("\\varepsilon%s{%s} = %s" % ("^" if epsilon[1] else "_", "".join(str(k + 1) for k in range(epsilon[2])), number_text(epsilon[0])))
        rng.shuffle(parts)
    return "%s \\implies %s" % (" \\land ".join(parts), body)


def numeric_tree(poly, weights):
    """Un albero con il valore del polinomio poly, con gli indici numerici in
    basso. Le componenti di poly sono quelle dichiarate (METRIC, dopo una
    lettura): se weights e vero, la componente scritta in basso di un tensore
    dichiarato con un indice in alto e quella dichiarata per il numero della
    diagonale, T_{1} = c_1 T^{1}, e il coefficiente va diviso per quel numero.
    Senza, e il valore che scriverebbe chi dimentica la metrica."""
    adjusted = {}
    for key, coefficient in poly.items():
        for name, at in key:
            for k, up in enumerate(METRIC["reference"][name]):
                if up and weights:
                    coefficient = coefficient / METRIC["c"][at[k] - ORIGIN]
        adjusted[key] = coefficient
    return polynomial_tree(adjusted)


def generate_metric(rng):
    """Un enunciato con una metrica diagonale dichiarata. Le famiglie:
    un'identita nota, anche alterata o con un'altra metrica; due membri a caso
    con le posizioni in ordine, il secondo uguale al primo scritto in un altro
    modo (anche con una coppia di indici sommati scambiata tra alto e basso,
    che non cambia il valore), alterato, con una sola posizione cambiata (che
    rompe una regola delle posizioni e basta), oppure il valore del primo
    scritto con gli indici numerici, giusto o come lo scriverebbe chi
    dimentica la metrica."""
    global EXACT
    if rng.random() < 0.25:
        # Un'identita nota, con le lettere rinominate.
        try:
            dimension, left, right, free = read_statement(rng.choice(KNOWN_METRIC))
        except Outside:
            raise AssertionError("un'identita nota con la metrica non si legge")
        name, entries = METRIC["name"], list(METRIC["c"])
        epsilon = (METRIC["eps"], METRIC["eps_up"], dimension) if "eps" in METRIC else None
        letters = sorted({i for s in symbols_of(left) + symbols_of(right) for i in s[3] if isinstance(i, str)})
        renamed = dict(zip(letters, rng.sample(INDEX_LETTERS, len(letters))))

        def rename(symbol):
            return symbol[:3] + (tuple(renamed.get(i, i) for i in symbol[3]),) + symbol[4:]
        left, right = map_tree(left, rename), map_tree(right, rename)
        free = sorted(renamed[c] for c in free)
        change = rng.random()
        if change < 0.3:
            if rng.random() < 0.5:
                left = perturb(rng, left, dimension)
            else:
                right = perturb(rng, right, dimension)
        elif change < 0.6:
            # Un'altra metrica: vero o falso, lo dice l'oracolo.
            k = rng.randrange(dimension)
            entries[k] = entries[k] * rng.choice((-1, 2, Fraction(1, 2)))
        elif change < 0.7 and epsilon:
            epsilon = (epsilon[0] * rng.choice((-1, 2)), epsilon[1] != (rng.random() < 0.5), dimension)
        EXACT = True
        body = "%s = %s" % (latex(left, rng), latex(right, rng))
        EXACT = False
    else:
        dimension = rng.choice((2, 2, 3, 3, 4))
        entries = [Fraction(rng.choice((1, 1, 1, -1, -1, 2, 3, Fraction(1, 2), -2))) for _ in range(dimension)]
        name = rng.choice(METRIC_NAMES)
        free = sorted(rng.sample(INDEX_LETTERS[:4], rng.choice((0, 0, 1, 1, 2))))
        where = {letter: rng.random() < 0.5 for letter in free}
        use_epsilon = rng.random() < 0.5
        side = factored_side if rng.random() < 0.3 else random_side
        left = with_metric(rng, place(rng, side(rng, dimension, set(free), use_epsilon), where), name)

        # Il valore di epsilon: quasi sempre dichiarato se epsilon c'e (senza,
        # l'enunciato e rifiutato), ogni tanto anche se non c'e, e ogni tanto
        # con una cifra in piu o in meno.
        epsilon = None
        if rng.random() < (0.94 if any(s[1] == "e" for s in symbols_of(left)) else 0.1):
            epsilon = (Fraction(rng.choice((1, 1, 1, -1, 2))), rng.random() < 0.5, dimension if rng.random() < 0.97 else dimension + rng.choice((-1, 1)))
        EXACT = True
        left_text = latex(left, rng)
        EXACT = False

        how = rng.random()
        right = shuffled(rng, left)
        if how < 0.2:
            pass
        elif how < 0.38:
            # Una lettera sommata scambiata tra alto e basso, dovunque compare.
            pairs = sorted({letter for _, f in expand(right) for letter, n in count_letters(f).items() if n == 2})
            if pairs:
                letter = rng.choice(pairs)
                right = with_metric(rng, map_tree(right, lambda s: s[:4] + (tuple(not up if i == letter else up for i, up in zip(s[3], s[4])),)), name)
        elif how < 0.58:
            right = perturb(rng, right, dimension)
        elif how < 0.66:
            # Una sola posizione cambiata: un indice libero in alto in un
            # membro e in basso nell'altro, o uno sommato due volte dalla
            # stessa parte.
            targets = [s for s in symbols_of(right) if any(isinstance(i, str) for i in s[3])]
            if targets:
                target = rng.choice(targets)
                k = rng.choice([n for n, i in enumerate(target[3]) if isinstance(i, str)])
                done = []

                def move(symbol):
                    if symbol is target and not done:
                        done.append(1)
                        return symbol[:4] + (tuple(not up if n == k else up for n, up in enumerate(symbol[4])),)
                    return symbol
                right = map_tree(right, move)
        elif how < 0.72:
            right = ("num", Fraction(0))
        elif free or how < 0.82:
            # Un altro membro a caso con gli stessi indici liberi, anche
            # della forma (X + Y) Z: i suoi tensori sono quasi tutti gia
            # dichiarati dal primo membro, e quindi con indici spostati
            # anche dentro la parentesi e nel fattore che la moltiplica.
            other = factored_side if rng.random() < 0.5 else random_side
            right = with_metric(rng, place(rng, other(rng, dimension, set(free), use_epsilon), where), name)
        else:
            # Il valore del primo membro, con gli indici numerici: giusto,
            # senza i numeri della diagonale per gli indici spostati, con la
            # metrica euclidea al posto di quella dichiarata, o alterato.
            right = ("num", Fraction(0))
            try:
                _, read_left, _, _ = read_statement(metric_statement(rng, name, entries, epsilon, left_text + " = 0"))
                variant = rng.random()
                if 0.7 <= variant < 0.85:
                    METRIC["c"] = [Fraction(1)] * dimension
                right = numeric_tree(value(read_left, dimension, {}), variant < 0.5 or variant >= 0.85)
                if variant >= 0.85:
                    right = perturb(rng, right, dimension)
            except Outside:
                pass
        EXACT = True
        body = "%s = %s" % (left_text, latex(right, rng))
        EXACT = False

    # La dimensione la dice la metrica: i quantificatori ci possono essere,
    # e ogni tanto ne dicono un'altra.
    if free and rng.random() < 0.6:
        listed = ", ".join(str(v) for v in range(1, dimension + (1 if rng.random() < 0.95 else 2)))
        body = "\\forall %s \\in \\{%s\\}\\ %s" % (", ".join(free), listed, rng.choice(("\\left( %s \\right)", "(%s)")) % body)
    # Ogni tanto senza le ipotesi: nessuna metrica e sottintesa, e l'enunciato
    # vale solo se e uno di quelli senza metrica.
    if rng.random() < 0.05:
        return body
    return metric_statement(rng, name, entries, epsilon, body)


# ---------------------------------------------------------------------------
# 5c. Generare enunciati con tensori dati, indici greci, valori da 0
# ---------------------------------------------------------------------------
#
# Come per la metrica, chi genera non e l'oracolo: compone un enunciato e
# lascia a judge() di dire se e vero, falso o fuori linguaggio. Per avere
# molti veri, il secondo membro e spesso il valore del primo calcolato
# dall'oracolo sul testo gia scritto; i falsi di poco nascono cambiando dopo
# una cosa sola nelle ipotesi: un elemento di una tabella, un segno della
# metrica, la posizione di un indice di una tabella, il numero da cui
# cominciano i valori.

GREEK_POOL = ["\\mu", "\\nu", "\\rho", "\\sigma", "\\alpha", "\\beta", "\\gamma", "\\lambda", "\\kappa", "\\tau"]

# Elementi della diagonale che sono funzioni delle coordinate, con lettere che
# non fanno da indice ne da nome di un tensore negli enunciati generati. Una
# e identicamente zero, e una ha un indice: vanno rifiutate.
FUNCTION_ENTRIES = ["t^{2}", "t^{2}", "t^{2} f^{2}", "t", "2 f", "x^{2}", "\\frac{1}{1 - K t^{2}}", "(R + t f)^{2}", "\\frac{t^{2}}{1 + f^{2}}", "R - 1",
                    "\\frac{1}{2} t^{3}", "1 - \\frac{1}{2} f", "\\frac{1}{t^{2}}", "-\\frac{f}{t}", "\\frac{R}{t - 1}", "\\frac{1}{1 - K t^{2}}",
                    "t - t", "t_{1}", "\\delta_{11}"]


def element_tree(rng, dimension):
    """Un elemento di una tabella: un numero, oppure un piccolo polinomio in
    scalari e in componenti con l'indice numerico, da 1 a dimension."""
    number = ("num", Fraction(rng.choice((0, 0, 1, 1, 2, 3, 5, Fraction(1, 2), Fraction(2, 3)))))
    scalar = ("sym", "t", rng.choice("xyz"), (), ())
    component = ("sym", "t", rng.choice("EB"), (rng.randint(1, dimension),), (False,))
    pick = rng.random()
    if pick < 0.55:
        tree = number
    elif pick < 0.7:
        tree = scalar
    elif pick < 0.85:
        tree = component
    elif pick < 0.93:
        tree = ("mul", ("num", Fraction(rng.choice((2, 3)))), rng.choice((scalar, component)))
    else:
        tree = ("add", rng.choice((scalar, component)), number)
    return ("neg", tree) if rng.random() < 0.25 and tree[0] != "add" else tree


def respell(tree, letters, shift):
    """tree con le lettere degli indici cambiate secondo letters e i numeri
    scritti come indici aumentati di shift."""
    return map_tree(tree, lambda s: s[:3] + (tuple(letters.get(i, i) if isinstance(i, str) else i + shift for i in s[3]),) + s[4:])


def given_text(rng, name, places, elements, holders, shift):
    """L'ipotesi che da per componenti il tensore name: scritto con gli
    indici holders nelle posizioni places, poi un elenco per un indice e una
    tabella per due."""
    cells = [latex(respell(element, {}, shift), rng) for element in elements]
    # Ogni tanto un numero preceduto da un 1 senza nulla in mezzo, "1 2": chi
    # lo leggesse come prodotto avrebbe lo stesso valore, e gli elementi
    # sarebbero ancora quanti devono. Rompe la sola regola dei due numeri uno
    # dopo l'altro, e va rifiutato.
    numbers = [k for k, cell in enumerate(cells) if cell[0].isdigit()]
    if numbers and rng.random() < 0.04:
        k = rng.choice(numbers)
        cells[k] = "1 " + cells[k]
    if len(places) == 1:
        body = "(%s)" % ", ".join(cells)
    else:
        size = int(round(len(cells) ** 0.5))
        environment = rng.choice(("pmatrix", "pmatrix", "bmatrix", "matrix"))
        rows = " \\\\ ".join(" & ".join(cells[r * size:(r + 1) * size]) for r in range(size))
        body = "\\begin{%s} %s \\end{%s}" % (environment, rows, environment)
    return "%s = %s" % (latex(("sym", "t", name, tuple(holders), tuple(places)), rng), body)


def generate_given(rng):
    """Un enunciato con tensori dati per componenti, con indici greci o con i
    valori degli indici che cominciano da 0, con o senza metrica."""
    global EXACT
    dimension = rng.choice((2, 2, 3, 3, 4))
    origin = rng.choice((1, 1, 0))
    greek = rng.random() < 0.4
    metric = rng.random() < 0.65
    entries = [Fraction(rng.choice((1, 1, 1, -1, -1, 2, 3, Fraction(1, 2)))) for _ in range(dimension)]
    # Una volta su tre, con una metrica, alcuni elementi sono funzioni delle
    # coordinate, scritte come testo: l'oracolo le legge come il resto.
    functions = [rng.choice(FUNCTION_ENTRIES) if metric and rng.random() < 0.6 else None for _ in range(dimension)] if rng.random() < 0.5 else [None] * dimension
    name = rng.choice("gh")
    given = {}
    if rng.random() < 0.2:
        # Una sola componente di una tabella, con gli indici numerici: con
        # una metrica anche alzati o abbassati.
        # Ogni tanto la tabella e un elenco: accanto a un vettore dato un
        # numero in alto resta rifiutato, perche potrebbe essere una potenza.
        free = []
        rank = 2 if rng.random() < 0.75 else 1
        given["T"] = [tuple(metric and rng.random() < 0.5 for _ in range(rank)), [element_tree(rng, dimension) for _ in range(dimension ** rank)]]
        left = ("sym", "t", "T", tuple(rng.randint(1, dimension) for _ in range(rank)), tuple(metric and rng.random() < 0.5 for _ in range(rank)))
        if not metric:
            left = ("mul", ("sym", "d", "", (1, 1), (False, False)), left[:4] + (given["T"][0],))
    else:
        free = sorted(rng.sample(INDEX_LETTERS[:4], rng.choice((0, 0, 0, 1, 1, 2))))
        where = {letter: rng.random() < 0.5 for letter in free}
        left = (factored_side if rng.random() < 0.15 else random_side)(rng, dimension, set(free), rng.random() < (0.25 if metric else 0.6))
        if metric:
            left = with_metric(rng, place(rng, left, where), name)
        elif not any(s[1] in "de" for s in symbols_of(left)):
            # Senza metrica l'enunciato si riconosce da una delta.
            left = ("mul", ("sym", "d", "", (1, 1), (False, False)), left)
        # I tensori dati: alcuni di quelli del primo membro con uno o due
        # indici, nelle posizioni che l'ipotesi sceglie.
        ranks = {}
        for symbol in symbols_of(left):
            if symbol[1] == "t" and len(symbol[3]) in (1, 2):
                ranks.setdefault(symbol[2], set()).add(len(symbol[3]))
        names = sorted(tensor for tensor, found in ranks.items() if len(found) == 1)
        rng.shuffle(names)
        for tensor in names[:rng.choice((0, 1, 1, 2, 2))]:
            rank = min(ranks[tensor])
            given[tensor] = [tuple(metric and rng.random() < 0.5 for _ in range(rank)), [element_tree(rng, dimension) for _ in range(dimension ** rank)]]
    # Ogni tanto un elemento nomina un tensore dato, lo stesso o un altro:
    # rompe quella regola sola, e l'enunciato va rifiutato.
    if given and rng.random() < 0.04:
        tensor, other = rng.choice(sorted(given)), rng.choice(sorted(given))
        rank = len(given[other][0])
        given[tensor][1][rng.randrange(len(given[tensor][1]))] = ("sym", "t", other, (1,) * rank, (False,) * rank)
    has_epsilon = any(s[1] == "e" for s in symbols_of(left))
    epsilon = (Fraction(rng.choice((1, 1, 1, -1, 2))), rng.random() < 0.5) if metric and rng.random() < (0.94 if has_epsilon else 0.05) else None

    # Come l'enunciato scrive gli indici: lettere greche, e i numeri contati
    # dall'origine.
    letters = sorted({i for s in symbols_of(left) for i in s[3] if isinstance(i, str)})
    spell = dict(zip(letters, rng.sample(GREEK_POOL, len(letters)))) if greek and len(letters) <= len(GREEK_POOL) else {}
    shift = origin - 1
    left = respell(left, spell, shift)
    free = [spell.get(letter, letter) for letter in free]
    holders = rng.sample(GREEK_POOL, 2) if greek else rng.sample("ijkl", 2)
    listed = rng.sample(sorted(set(spell.values()) or set(letters)), min(len(letters), rng.choice((1, 2)))) or holders[:1]
    said = rng.random() < (0.85 if origin == 0 or greek or not (metric or given) else 0.3)
    quantified = bool(free) and rng.random() < 0.6
    EXACT = metric
    left_text = latex(left, rng)
    EXACT = False

    def hypotheses(first):
        """Le ipotesi, con i valori degli indici che cominciano da first."""
        parts = []
        if metric:
            upper = rng.random() < 0.25
            shown = [("-" if c < 0 else "") + function if function else number_text(1 / c if upper else c) for c, function in zip(entries, functions)]
            parts.append("%s%s{%s} = \\%s{diag}(%s)" % (name, "^" if upper else "_", " ".join(holders), rng.choice(("operatorname", "operatorname", "mathrm")),
                                                         ", ".join(shown)))
        if epsilon:
            parts.append("\\varepsilon%s{%s} = %s" % ("^" if epsilon[1] else "_", "".join(str(first + k) for k in range(dimension)), number_text(epsilon[0])))
        for tensor in sorted(given):
            parts.append(given_text(rng, tensor, given[tensor][0], given[tensor][1], holders[:len(given[tensor][0])], shift))
        if said:
            parts.append("%s \\in \\{%s\\}" % (", ".join(listed), ", ".join(str(first + k) for k in range(dimension))))
        rng.shuffle(parts)
        return parts

    # Un segno di somma davanti al primo prodotto, su una sua lettera
    # ripetuta: dice anche lui da dove cominciano i valori, e quanti sono.
    summed = None
    head = left
    while head[0] in ("add", "sub"):
        head = head[1]
    if not free and head[0] != "neg" and len(expand(head)) == 1 and rng.random() < 0.2:
        repeated = sorted(letter for letter, n in count_letters(expand(head)[0][1]).items() if n == 2)
        summed = repeated[0] if repeated else None

    def statement(parts, body, first):
        if summed:
            body = "\\sum_{%s=%d}^{%d} %s" % (summed, first, first + dimension - 1, body)
        if quantified:
            body = "\\forall %s \\in \\{%s\\}\\ %s" % (", ".join(free), ", ".join(str(first + k) for k in range(dimension)),
                                                       rng.choice(("\\left( %s \\right)", "(%s)")) % body)
        return "%s \\implies %s" % (" \\land ".join(parts), body) if parts else body

    # Il secondo membro: il valore del primo, quando non ha indici liberi e
    # l'oracolo lo sa leggere; altrimenti il primo scritto in un altro modo,
    # alterato, o un altro membro a caso.
    exact = None
    if not free:
        try:
            read_dimension, read_left, _, _ = read_statement(statement(hypotheses(origin), left_text + " = 0", origin))
            poly = value(read_left, read_dimension, {})
            if any(functions):
                # Con una metrica di funzioni il valore e una frazione N / D:
                # l'enunciato vero e D per il primo membro uguale a N, se i
                # due polinomi si sanno scrivere (componenti dichiarate in
                # basso). Altrimenti resta un membro a caso.
                poly = Ratio.of(poly)
                lower = all(not any(METRIC["reference"][tensor]) for part in (poly.num, poly.den) for key in part for tensor, _ in key)
                if lower:
                    # Una volta su quattro il denominatore e dimenticato: un
                    # falso che ha il numeratore giusto.
                    exact = polynomial_tree(poly.num)
                    if poly.den != {(): Fraction(1)} and rng.random() < 0.75:
                        left_text = "(%s)(%s)" % (latex(polynomial_tree(poly.den), rng), left_text)
            else:
                exact = numeric_tree(poly, True) if METRIC else polynomial_tree(poly)
        except (Outside, KeyError, IndexError):
            pass
    how, first = rng.random(), origin
    if exact is None:
        right = shuffled(rng, left) if how < 0.5 else perturb(rng, shuffled(rng, left), dimension) if how < 0.8 else ("num", Fraction(0))
    elif how < 0.4:
        right = exact
    elif how < 0.52:
        right = perturb(rng, exact, dimension)
    else:
        # Il valore giusto, e poi una cosa sola cambiata nelle ipotesi.
        right = exact
        tensor = rng.choice(sorted(given)) if given else None
        if how < 0.66 and tensor:
            k = rng.randrange(len(given[tensor][1]))
            given[tensor][1][k] = ("add", given[tensor][1][k], ("num", Fraction(1)))
        elif how < 0.78 and metric:
            entries[rng.randrange(dimension)] *= -1
        elif how < 0.88 and tensor and metric:
            k = rng.randrange(len(given[tensor][0]))
            given[tensor][0] = tuple(not up if n == k else up for n, up in enumerate(given[tensor][0]))
        elif said or quantified or epsilon:
            first = 1 - origin
    EXACT = metric
    body = "%s = %s" % (left_text, latex(right, rng))
    EXACT = False
    return statement(hypotheses(first), body, first)


def generate(rng, metric=True):
    """Ritorna il testo di un enunciato con almeno una delta o un simbolo di
    Levi-Civita, salvo una volta su venti: quelli senza il programma non li
    riconosce, e l'oracolo lo sa. Se metric e vero, una volta su quattro ha
    tensori dati per componenti, indici greci o valori da 0, e una volta su
    quattro una metrica dichiarata e basta."""
    global METRIC, GIVEN, ORIGIN
    if metric and rng.random() < 0.25:
        return generate_given(rng)
    if metric and rng.random() < 0.34:
        return generate_metric(rng)
    # Senza metrica: i conti che servono a generare non devono vedere quella
    # dell'enunciato di prima, ne i suoi tensori dati.
    METRIC, GIVEN, ORIGIN = None, {}, 1
    for _ in range(50):
        text = generate_any(rng)
        if "\\delta" in text or "psilon" in text or rng.random() < 0.05:
            break
    return text


def generate_any(rng):
    """Ritorna il testo di un enunciato."""
    family = rng.random()
    if family < 0.35:
        # Un'identita nota, con le lettere rinominate e forse alterata.
        try:
            dimension, left, right, free = read_statement(rng.choice(KNOWN))
        except Outside:
            raise AssertionError("un'identita nota non si legge")
        if rng.random() < 0.12:
            # Senza metrica epsilon e un tensore hanno gli indici sempre nelle
            # stesse posizioni: uno solo dei loro simboli scritto in alto e
            # rifiutato, se quel simbolo compare anche altrove.
            targets = [s for s in symbols_of(left) + symbols_of(right) if s[1] in "et" and s[3] and all(isinstance(i, str) for i in s[3])]
            if targets:
                target = rng.choice(targets)
                done = []

                def raise_one(symbol):
                    if symbol is target and not done:
                        done.append(1)
                        return symbol[:4] + ((True,) * len(symbol[3]),)
                    return symbol
                left, right = map_tree(left, raise_one), map_tree(right, raise_one)
        letters = sorted({i for s in symbols_of(left) + symbols_of(right) for i in s[3] if isinstance(i, str)})
        renamed = dict(zip(letters, rng.sample(INDEX_LETTERS, len(letters))))

        def rename(symbol):
            return symbol[:3] + (tuple(renamed.get(i, i) for i in symbol[3]),) + symbol[4:]
        left, right = map_tree(left, rename), map_tree(right, rename)
        free = {renamed[c] for c in free}
        if rng.random() < 0.55:
            if rng.random() < 0.5:
                left = perturb(rng, left, dimension)
            else:
                right = perturb(rng, right, dimension)
        if rng.random() < 0.3:
            left, right = right, left
    else:
        # Due membri a caso con gli stessi indici liberi: il secondo e il
        # primo rimescolato (vero), alterato, zero, oppure un altro a caso.
        dimension = rng.choice((2, 3, 3, 3, 4))
        free = set(rng.sample(INDEX_LETTERS[:4], rng.choice((0, 0, 1, 1, 2))))
        use_epsilon = rng.random() < 0.6
        left = (factored_side if rng.random() < 0.2 else random_side)(rng, dimension, free, use_epsilon)
        how = rng.random()
        if how < 0.3:
            right = shuffled(rng, left)
        elif how < 0.6:
            right = perturb(rng, shuffled(rng, left), dimension)
        elif how < 0.7:
            right = ("num", Fraction(0))
        elif how < 0.85 and not free:
            poly = value(left, dimension, {}) if well_formed(left, dimension) else {}
            right = polynomial_tree(poly)
            if rng.random() < 0.4:
                right = perturb(rng, right, dimension)
        else:
            right = random_side(rng, dimension, free, use_epsilon)
    # Una regola della convenzione rotta da sola, lasciando in ordine tutto il
    # resto: un'alterazione qualsiasi cambia quasi sempre anche gli indici
    # liberi, e allora chi rifiuta l'enunciato non dice se conosce la regola.
    # (a) Uno scalare con un indice che nel primo membro e gia sommato: quella
    # lettera compare quattro volte in un prodotto. (b) Un numero diverso da
    # zero al posto di un membro con indici liberi: solo lo 0 e il tensore
    # nullo.
    alone = rng.random()
    if alone < 0.05:
        repeated = sorted({letter for _, factors in expand(left) for letter, n in count_letters(factors).items() if n == 2})
        if repeated:
            again = ("sym", "t", rng.choice(VECTORS), (rng.choice(repeated),), (False,))
            left = ("mul", left, ("mul", again, again))
    elif alone < 0.09 and free:
        right = ("num", Fraction(rng.choice((1, 2, 3))))
    body = "%s = %s" % (latex(left, rng), latex(right, rng))
    return declare(rng, dimension, left, right, free, body)


def well_formed(tree, dimension):
    try:
        free_of(tree, dimension, {})
        return True
    except Outside:
        return False


def shuffled(rng, tree):
    """Lo stesso valore scritto in un altro modo: addendi e fattori scambiati,
    le lettere sommate rinominate dentro ogni prodotto."""
    terms = expand(tree)
    rng.shuffle(terms)
    result = None
    for coefficient, factors in terms:
        counts = count_letters(factors)
        dummies = [letter for letter, n in counts.items() if n == 2]
        pool = [c for c in INDEX_LETTERS if counts.get(c, 0) != 1]
        renamed = dict(zip(dummies, rng.sample(pool, len(dummies))))
        factors = [s[:3] + (tuple(renamed.get(i, i) for i in s[3]),) + s[4:] for s in factors]
        rng.shuffle(factors)
        term = ("num", abs(coefficient))
        for factor in factors:
            term = factor if term == ("num", Fraction(1)) else ("mul", term, factor)
        if coefficient < 0:
            term = ("neg", term) if result is None else term
        if result is None:
            result = term
        else:
            result = ("sub" if coefficient < 0 else "add", result, term)
    return result if result is not None else ("num", Fraction(0))


# ---------------------------------------------------------------------------
# 6. Confrontare con il programma
# ---------------------------------------------------------------------------

def run_program(program, texts, path):
    with open(path, "w", encoding="utf-8") as out:
        out.write("\\documentclass{article}\n\\usepackage{amsmath,amssymb}\n\\begin{document}\n\\begin{enumerate}\n")
        for text in texts:
            out.write("\\item $\\displaystyle %s$\n" % text)
        out.write("\\end{enumerate}\n\\end{document}\n")
    run = subprocess.run([program, path], capture_output=True, text=True, timeout=1800)
    with open(path + ".log", "w", encoding="utf-8") as out:
        out.write(run.stdout + run.stderr)
    lines = [line.split(". ", 1)[1] for line in run.stdout.splitlines() if re.match(r"\s*\d+\. ", line)]
    return run.returncode, lines


def check_counterexample(verdict, state):
    """Controlla il punto e i valori scritti nel verdetto FALSO. Ritorna il
    problema, o None."""
    found = re.match(r"FALSO: controesempio (.*?): (.*) \(in dimensione", verdict)
    if not found:
        return "verdetto FALSO senza controesempio leggibile"
    point = {}
    if found.group(1) != "senza indici liberi":
        for piece in found.group(1).split(", "):
            letter, _, number = piece.partition(" = ")
            point[letter] = int(number)
    if sorted(point) != state["letters"]:
        return "il controesempio non da un valore agli indici liberi"
    global METRIC, GIVEN, ORIGIN
    dimension = state["dimension"]
    METRIC, GIVEN, ORIGIN = state["metric"], state["given"], state["origin"]
    if any(not ORIGIN <= v <= ORIGIN + dimension - 1 for v in point.values()):
        return "il controesempio da a un indice un valore che non ha"
    left, right = value(state["left"], dimension, point), value(state["right"], dimension, point)
    if left == right:
        return "nel controesempio i due membri sono uguali"
    values = re.match(r"il primo membro vale (.*), il secondo (.*)$", found.group(2))
    if not values:
        return None if found.group(2) == "i due membri sono diversi" else "valori del controesempio non leggibili"
    try:
        _, shown_left, shown_right, shown_free = read_statement("%s = %s" % values.groups(), dimension, state["origin"])
    except Outside as problem:
        return "i valori scritti non si rileggono: %s" % problem
    if shown_free or value(shown_left, dimension, {}) != left or value(shown_right, dimension, {}) != right:
        return "i valori scritti non sono quelli dei due membri"
    # Con una metrica i valori sono polinomi nelle componenti dichiarate, e
    # ognuna va scritta con gli indici dove l'enunciato li dichiara: A_{1} al
    # posto di A^{1} sarebbe un altro numero.
    if state["metric"]:
        for symbol in symbols_of(shown_left) + symbols_of(shown_right):
            if symbol[4] != state["metric"]["reference"][symbol[2]]:
                return "nei valori scritti una componente ha un indice fuori dalla posizione dichiarata"
    return None


def compare(texts, verdicts, expected_class=None):
    """Stampa BUG e SUSPECT e ritorna il numero dei BUG."""
    bugs, counts = 0, {}
    for number, (text, verdict) in enumerate(zip(texts, verdicts), 1):
        truth, state = judge(text)
        found = re.match(r"NON SUPPORTATO|NON DIMOSTRATO|DIMOSTRATO|FALSO", verdict)
        said = found.group(0) if found else "?"
        # Senza delta ne simbolo di Levi-Civita l'enunciato non e di questo
        # linguaggio: il programma puo deciderlo in un altro (un conto sui
        # numeri scritti, una successione), e qui non c'e nulla da
        # confrontare. Resta un errore se lo ha letto come identita con gli
        # indici.
        if truth == "OUTSIDE" and state == NOT_INDEXED and "valori degli indici" not in verdict and "in dimensione" not in verdict:
            counts["other language"] = counts.get("other language", 0) + 1
            continue
        key = "%s -> %s" % (truth, said)
        counts[key] = counts.get(key, 0) + 1
        problem = None
        if expected_class and truth != expected_class:
            print("SUSPECT %d: the oracle says %s (%s), the data file expects %s\n    %s" % (number, truth, state if truth == "OUTSIDE" else "", expected_class, text))
        if truth == "TRUE" and said == "FALSO":
            problem = "a true statement was called false"
        elif truth == "FALSE" and said == "DIMOSTRATO":
            problem = "a false statement was proved"
        elif truth == "OUTSIDE" and said in ("DIMOSTRATO", "FALSO"):
            print("SUSPECT %d: the oracle cannot read it (%s), the program decided it: %s\n    %s" % (number, state, verdict[:90], text))
        elif truth != "OUTSIDE" and said == "NON SUPPORTATO" and "enunciato troppo lungo" in verdict:
            # Un limite del programma su ogni enunciato, non di questo
            # linguaggio: un membro scritto componente per componente lo
            # supera. E contato, non segnalato.
            counts["too long for the program"] = counts.get("too long for the program", 0) + 1
            counts[key] -= 1
        elif truth != "OUTSIDE" and said == "NON SUPPORTATO":
            print("SUSPECT %d: the oracle reads it as %s, the program refuses it: %s\n    %s" % (number, truth, verdict[:110], text))
        elif truth == "FALSE" and said == "FALSO":
            problem = check_counterexample(verdict, state)
        if truth != "OUTSIDE" and said in ("DIMOSTRATO", "FALSO") and "certificato verificato" not in verdict:
            problem = problem or "a verdict without a verified certificate"
        # NON DIMOSTRATO e onesto solo per un limite dichiarato: il budget, o
        # un certificato troppo grande per il verificatore. Ogni altro motivo
        # (il verificatore ha respinto il certificato del motore, dei valori
        # scritti non si rileggono) vuol dire che uno dei due sbaglia.
        if said == "NON DIMOSTRATO" and not any(limit in verdict for limit in LIMITS):
            print("SUSPECT %d: not proved for a reason that is not a declared limit: %s\n    %s" % (number, verdict[:170], text))
        if problem:
            bugs += 1
            print("BUG %d: %s\n    %s\n    %s" % (number, problem, text, verdict[:200]))
    for key in sorted(counts):
        if counts[key]:
            print("  %-28s %d" % (key, counts[key]))
    return bugs


def main_statements(program, seed, count, work):
    rng = random.Random(seed)
    texts = [generate(rng) for _ in range(count)]
    code, verdicts = run_program(program, texts, os.path.join(work, "tensors_%d.tex" % seed))
    if code not in (0, 1) or len(verdicts) != len(texts):
        print("BUG: expected %d verdicts, got %d (exit %d)" % (len(texts), len(verdicts), code))
        return 1
    bugs = compare(texts, verdicts)
    print("seed %d: %d statements, %d BUG" % (seed, len(texts), bugs))
    return 1 if bugs else 0


def main_data(program, path, expected, work):
    text = open(path, encoding="utf-8").read()
    texts = re.findall(r"\\item \$\\displaystyle (.*)\$\s*$", text, re.M)
    code, verdicts = run_program(program, texts, os.path.join(work, "data_" + os.path.basename(path)))
    if code not in (0, 1) or len(verdicts) != len(texts):
        print("BUG: expected %d verdicts, got %d (exit %d)" % (len(texts), len(verdicts), code))
        return 1
    wanted = {"true": "TRUE", "false": "FALSE", "beyond": "OUTSIDE"}[expected]
    agreed = sum(1 for t in texts if judge(t)[0] == wanted)
    bugs = compare(texts, verdicts, wanted)
    print("%s: %d statements, %d judged %s by the oracle, %d BUG" % (path, len(texts), agreed, wanted, bugs))
    return 1 if bugs or agreed != len(texts) else 0


# ---------------------------------------------------------------------------
# 7. Certificati per il verificatore
# ---------------------------------------------------------------------------

def expected_answer(dimension, left, right, claim, point, shown):
    """Che cosa deve rispondere il verificatore: True per accettato. I
    tensori dati sono in GIVEN, con gli elementi come sono scritti nel
    certificato."""
    for name, elements in GIVEN.items():
        # Un elemento non ha lettere tra gli indici e non nomina un tensore
        # dato: altrimenti il certificato va respinto, e non c'e conto da fare.
        for element in elements:
            if any(s[1] == "t" and s[2] in GIVEN or any(isinstance(i, str) or not 1 <= i <= dimension for i in s[3]) for s in symbols_of(element)):
                return False
    try:
        free = conventions(left, right, dimension)
        for tree in shown:
            if tree is not None and free_of(tree, dimension, {}) not in (None, frozenset()):
                return False
        # I ranghi dei tensori valgono per tutto il certificato.
        ranks = {}
        for tree in (left, right) + tuple(t for t in shown if t is not None):
            for symbol in symbols_of(tree):
                if symbol[1] == "t" and ranks.setdefault(symbol[2], len(symbol[3])) != len(symbol[3]):
                    return False
            if not zero_alone(tree):
                free_of(tree, dimension, {})
    except Outside:
        return False
    if claim == "holds":
        if any(t is not None for t in shown):
            return False
        return all(value(left, dimension, dict(zip(sorted(free), choice))) == value(right, dimension, dict(zip(sorted(free), choice)))
                   for choice in itertools.product(range(1, dimension + 1), repeat=len(free)))
    if set(point) != free or any(not 1 <= v <= dimension for v in point.values()):
        return False
    sides = [value(left, dimension, point), value(right, dimension, point)]
    if sides[0] == sides[1]:
        return False
    return all(tree is None or value(tree, dimension, {}) == sides[k] for k, tree in enumerate(shown))


def main_certificates(checker, seed, count, work):
    global METRIC, GIVEN, ORIGIN
    rng = random.Random(seed)
    cases = []
    while len(cases) < count:
        text = generate(rng, metric=False)
        metric_line = None
        try:
            reader_dimension, left, right, _ = read_statement(text)
        except Outside:
            # Anche un enunciato che rompe la convenzione puo diventare un
            # certificato, che va respinto: lo si rilegge senza i controlli.
            reader = Reader(re.sub(r"^\\forall[^(]*?\\}\\ ", "", text))
            try:
                left = reader.sum()
                reader.take("=")
                right = reader.sum()
            except (Outside, RecursionError):
                continue
            if reader.peek() is not None or reader.sums:
                continue
            reader_dimension = rng.choice((2, 3, 4))
        # Un certificato non ha tensori dati finche non li mette qui sotto, e
        # i suoi valori vanno da 1 a d.
        GIVEN, ORIGIN = {}, 1
        dimension = reader_dimension if rng.random() < 0.85 else rng.choice((1, 2, 3, 4))

        # Una regola della convenzione rotta da sola, in un certificato che
        # un verificatore senza quella regola accetterebbe. (a) I due membri
        # uguali, con uno scalare in piu il cui indice e gia sommato: la
        # lettera compare quattro volte in un prodotto, e i due membri hanno
        # comunque lo stesso valore. (b) Nel secondo membro un indice libero
        # in piu, in una delta che vale 1 quando l'indice vale 1: i due
        # membri hanno indici liberi diversi, e lo stesso valore nel primo
        # punto che un verificatore proverebbe.
        twisted = False
        letters = {i for s in symbols_of(left) + symbols_of(right) for i in s[3] if isinstance(i, str)}
        twist = rng.random()
        if twist < 0.06:
            repeated = sorted({letter for _, f in expand(left) for letter, n in count_letters(f).items() if n == 2})
            if repeated:
                again = ("sym", "t", rng.choice(VECTORS), (rng.choice(repeated),), (False,))
                left = right = ("mul", left, ("mul", again, again))
                twisted = True
        elif twist < 0.12:
            unused = [c for c in INDEX_LETTERS if c not in letters]
            if unused:
                right = ("mul", right, ("sym", "d", "", (unused[0], 1), (False, False)))
                twisted = True
        if twisted:
            dimension = reader_dimension

        # Una volta su quattro il certificato ha la voce metric, e alcune
        # delle sue delta diventano la metrica o la sua inversa: sono simboli
        # del linguaggio dei certificati, senza posizioni. L'oracolo fa il
        # conto con i numeri giusti; ogni tanto la voce scritta manca, ha uno
        # zero, o ha un numero in meno o in piu, e allora il certificato va
        # respinto (se manca, solo quando un simbolo la usa).
        METRIC, metric_fault = None, False
        if rng.random() < 0.25:
            entries = [Fraction(rng.choice((1, -1, 2, 3, Fraction(1, 2), -2))) for _ in range(dimension)]
            METRIC = {"c": entries}

            def metricize(symbol):
                return symbol[:1] + (rng.choice("LU"),) + symbol[2:] if symbol[1] == "d" and rng.random() < 0.7 else symbol
            left, right = map_tree(left, metricize), map_tree(right, metricize)
            said, fault = list(entries), rng.random()
            if fault < 0.08:
                said = None
                metric_fault = any(s[1] in "LU" for s in symbols_of(left) + symbols_of(right))
            elif fault < 0.14:
                said[rng.randrange(dimension)] = Fraction(0)
            elif fault < 0.2:
                said = said[:-1] if rng.random() < 0.5 else said + [Fraction(1)]
            if said is not None:
                metric_fault = said != entries
                written = [str(c.numerator) if c.denominator == 1 else "(frac %d %d)" % (c.numerator, c.denominator) for c in said]
                # Una volta su tre alcuni elementi sono funzioni, (/ N D):
                # l'oracolo conta con la frazione dei due polinomi. Ogni
                # tanto un numeratore e identicamente zero, o ha una lettera
                # tra gli indici: allora il certificato va respinto.
                if not metric_fault and rng.random() < 0.5:
                    scalar = lambda letter: ("sym", "t", letter, (), ())
                    pool = [(("mul", scalar("x"), scalar("x")), ("num", Fraction(1))),
                            (("num", Fraction(1)), ("sub", ("num", Fraction(1)), ("mul", scalar("y"), scalar("x")))),
                            (scalar("y"), ("add", scalar("x"), ("num", Fraction(2)))),
                            (("mul", ("num", Fraction(2)), scalar("z")), scalar("y"))]
                    for k in range(dimension):
                        if rng.random() < 0.5:
                            above, below = rng.choice(pool)
                            fault = rng.random()
                            if fault < 0.05:
                                # Un elemento identicamente zero, e un
                                # enunciato che un verificatore che lo
                                # accettasse direbbe vero: 5 g_kk = 7 g_kk.
                                above = ("sub", scalar("x"), scalar("x"))
                                entry = ("sym", "L", "", (k + 1, k + 1), (False, False))
                                left, right, twisted = ("mul", ("num", Fraction(5)), entry), ("mul", ("num", Fraction(7)), entry), True
                            elif fault < 0.1:
                                below = ("sym", "t", "E", ("i",), (False,))
                            if fault < 0.1:
                                metric_fault = True
                            else:
                                METRIC["c"][k] = Ratio(value(above, 1, {}), value(below, 1, {}))
                            written[k] = "(/ %s %s)" % (certificate_of(above), certificate_of(below))
                metric_line = "metric " + " ".join(written)
        # Una volta su cinque alcuni tensori dell'enunciato sono dati per
        # componenti. L'affermazione e i valori scritti sono calcolati con
        # quegli elementi; poi, ogni tanto, la voce scritta ne ha uno
        # cambiato (e allora l'oracolo rifa il conto con quello), oppure e
        # scritta male: un elemento in meno o in piu, un rango che non e
        # quello con cui il tensore compare, un elemento con una lettera tra
        # gli indici o che nomina il tensore stesso. Allora va respinto.
        GIVEN, given_lines, given_fault, altered = {}, [], False, None
        if rng.random() < 0.2:
            ranks = {}
            for symbol in symbols_of(left) + symbols_of(right):
                if symbol[1] == "t" and len(symbol[3]) in (1, 2):
                    ranks.setdefault(symbol[2], set()).add(len(symbol[3]))
            names = sorted(tensor for tensor, found in ranks.items() if len(found) == 1)
            rng.shuffle(names)
            for tensor in names[:rng.choice((1, 1, 2))]:
                rank = min(ranks[tensor])
                GIVEN[tensor] = [element_tree(rng, dimension) for _ in range(dimension ** rank)]
                written, said_rank, fault = list(GIVEN[tensor]), rank, rng.random()
                if fault < 0.05:
                    written.pop()
                elif fault < 0.1:
                    written.append(("num", Fraction(1)))
                elif fault < 0.15:
                    said_rank = rank + rng.choice((-1, 1))
                elif fault < 0.2:
                    written[rng.randrange(len(written))] = ("sym", "t", tensor, (1,) * rank, (False,) * rank)
                elif fault < 0.25:
                    written[rng.randrange(len(written))] = ("sym", "t", "E", ("i",), (False,))
                elif fault < 0.45:
                    k = rng.randrange(len(written))
                    altered = (tensor, k, ("add", written[k], ("num", Fraction(1))))
                    written[k] = altered[2]
                given_fault = given_fault or (fault < 0.25 and (written != GIVEN[tensor] or said_rank != rank))
                given_lines.append("given %s %d %s" % (tensor, said_rank, " ".join(certificate_of(e) for e in written)))
                if rng.random() < 0.05:
                    # La stessa voce due volte: un certificato scritto male.
                    given_lines.append(given_lines[-1])
                    given_fault = True
        try:
            free = sorted(conventions(left, right, dimension))
        except Outside:
            free = sorted({i for s in symbols_of(left) + symbols_of(right) for i in s[3] if isinstance(i, str)})[:2]
        # Con dei tensori dati e senza indici liberi, piu di meta delle volte
        # il secondo membro e il valore del primo: un certificato vero, che
        # un verificatore che leggesse male le componenti respingerebbe. Una
        # volta su quattro quel valore e calcolato con una tabella letta per
        # colonne: allora il certificato e sbagliato, se la tabella non e
        # simmetrica, e solo chi la legge per colonne lo accetterebbe.
        functions = bool(METRIC) and any(isinstance(c, Ratio) for c in METRIC["c"])
        if (GIVEN or functions) and not free and not twisted and not given_fault and rng.random() < 0.6:
            tables = [tensor for tensor in sorted(GIVEN) if len(GIVEN[tensor]) == dimension ** 2 and dimension > 1]
            swapped = rng.choice(tables) if tables and rng.random() < 0.25 else None
            kept = GIVEN[swapped] if swapped else None
            if swapped:
                GIVEN[swapped] = [kept[(k % dimension) * dimension + k // dimension] for k in range(dimension ** 2)]
            try:
                found = value(left, dimension, {})
                if isinstance(found, dict):
                    right = polynomial_tree(found)
                else:
                    # Una frazione N / D: il certificato vero e D per il
                    # primo membro uguale a N.
                    left, right = ("mul", polynomial_tree(found.den), left), polynomial_tree(found.num)
            except (Outside, KeyError, IndexError):
                pass
            if swapped:
                GIVEN[swapped] = kept
        claim, point, shown = "holds", {}, [None, None]
        if not twisted and rng.random() < 0.55:
            claim = "fails"
            point = {letter: rng.randint(1, dimension) for letter in free}
            if rng.random() < 0.1 and point:
                point.pop(sorted(point)[0])
            if rng.random() < 0.6:
                for k, tree in enumerate((left, right)):
                    # Un albero che rompe la convenzione puo non avere un
                    # valore: una lettera senza numero, un indice numerico
                    # oltre la diagonale della metrica.
                    try:
                        found = value(tree, dimension, point)
                        shown[k] = polynomial_tree(found) if isinstance(found, dict) else None
                    except (Outside, KeyError, IndexError):
                        shown[k] = None
                    if shown[k] is not None and rng.random() < 0.2:
                        shown[k] = ("add", shown[k], ("num", Fraction(1)))
        # L'elemento cambiato: da qui l'oracolo conta con quello scritto.
        if altered:
            GIVEN[altered[0]][altered[1]] = altered[2]
        lines = ["certificate indices", "dimension %d" % dimension] + ([metric_line] if metric_line else []) + given_lines
        lines.append("statement (= %s %s)" % (certificate_of(left), certificate_of(right)))
        if claim == "holds":
            lines.append("holds")
        else:
            lines.append(" ".join(["fails"] + ["%s %d" % (letter, v) for letter, v in sorted(point.items())]))
            for word, tree in zip(("left", "right"), shown):
                if tree is not None:
                    lines.append("%s %s" % (word, certificate_of(tree)))
        lines.append("end")
        try:
            wanted = not metric_fault and not given_fault and expected_answer(dimension, left, right, claim, point, shown)
        except (KeyError, IndexError):
            wanted = False
        cases.append(("\n".join(lines) + "\n", wanted))
    path = os.path.join(work, "tensors_%d.cert" % seed)
    with open(path, "w", encoding="utf-8") as out:
        out.write("".join(text for text, _ in cases))
    run = subprocess.run([checker, path], capture_output=True, text=True, timeout=1800)
    with open(path + ".log", "w", encoding="utf-8") as out:
        out.write(run.stdout + run.stderr)
    answers = [line.split(". ", 1)[1] for line in run.stdout.splitlines() if re.match(r"\s*\d+\. ", line)]
    if len(answers) != len(cases):
        print("BUG: expected %d answers, got %d (exit %d)" % (len(cases), len(answers), run.returncode))
        return 1
    bugs, counts = 0, {}
    for number, ((text, wanted), answer) in enumerate(zip(cases, answers), 1):
        said = answer.split(":")[0]
        key = "%s -> %s" % ("valid" if wanted else "wrong", said)
        counts[key] = counts.get(key, 0) + 1
        if said == "ACCETTATO" and not wanted:
            bugs += 1
            print("BUG %d: the checker accepted a certificate the oracle judges wrong\n%s" % (number, text))
        elif said == "RESPINTO" and wanted:
            print("SUSPECT %d: the checker rejected a certificate the oracle judges valid (%s)\n%s" % (number, answer, text))
    for key in sorted(counts):
        print("  %-28s %d" % (key, counts[key]))
    print("seed %d: %d certificates, %d BUG" % (seed, len(cases), bugs))
    return 1 if bugs else 0


def main():
    arguments = sys.argv[1:]
    if len(arguments) == 5 and arguments[0] == "data" and arguments[3] in ("true", "false", "beyond"):
        os.makedirs(arguments[4], exist_ok=True)
        return main_data(arguments[1], arguments[2], arguments[3], arguments[4])
    if len(arguments) == 5 and arguments[0] == "certificates":
        os.makedirs(arguments[4], exist_ok=True)
        return main_certificates(arguments[1], int(arguments[2]), int(arguments[3]), arguments[4])
    if len(arguments) == 4:
        os.makedirs(arguments[3], exist_ok=True)
        return main_statements(arguments[0], int(arguments[1]), int(arguments[2]), arguments[3])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
