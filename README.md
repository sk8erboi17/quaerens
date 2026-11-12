# mathresearcher

> **Lavori in corso.** Il programma dimostra solo una parte degli enunciati che
> gli vengono dati; quando non sa trattare un enunciato lo dice, invece di
> inventare una risposta. Formato di ingresso, messaggi e struttura del codice
> possono ancora cambiare.

Un dimostratore automatico scritto in C, senza dipendenze: un file per il
programma e uno, piccolo e indipendente, per ricontrollarne le dimostrazioni.
Legge enunciati matematici scritti in LaTeX e, per ciascuno, prova a costruire
una dimostrazione.

## Cosa fa oggi

Per ogni enunciato risponde con uno di quattro verdetti:

| Verdetto | Significato |
|---|---|
| `DIMOSTRATO` | la dimostrazione è stata costruita e ricontrollata |
| `FALSO` | c'è un controesempio, che viene stampato |
| `NON DIMOSTRATO` | il metodo si applica ma non chiude la dimostrazione |
| `NON SUPPORTATO` | l'enunciato usa notazioni o ragionamenti fuori portata |

Tratta tre famiglie di enunciati.

**Uguaglianze in `n` dimostrate per induzione.** Somme e prodotti con una
formula chiusa, per esempio

```latex
\sum_{k=1}^{n} k \cdot 2^{k} = \left(n - 1\right) \cdot 2^{n + 1} + 2
```

Il programma verifica il caso base e il passo induttivo con calcolo esatto.
Sa trattare polinomi e frazioni, potenze come `2^n` e `a^n`, fattoriali,
binomiali, numeri di Fibonacci, seni e coseni, radici, logaritmi e
arcotangenti.

L'induzione prova che i due membri sono uguali *dove sono definiti*. Prima di
rispondere `DIMOSTRATO` il programma controlla quindi:

- che esistano dei valori dei parametri con cui i due membri si calcolano
  davvero, con i numeri, per i primi `n`. Così `\ln(-x) = \ln(x)` non risulta
  dimostrato. Se i numeri semplici non vanno bene, i valori da provare vengono
  ricavati dalle condizioni dell'enunciato: per `\sqrt{x - 5000}` ne sceglie
  uno oltre 5000;
- che con quei valori ogni divisione, radice e logaritmo abbia senso per
  *tutti* gli `n`, non solo per i primi. Di `\frac{n - 50}{n - 50} = 1`
  risponde "per n = 50 l'enunciato non è definito: divisione per zero"; di
  `\ln(60 - n)` che per `n = 60` c'è il logaritmo di un numero non positivo;
  di `\frac{2^{n} - 4096 n}{2^{n} - 4096 n} = 1` che manca `n = 16`. Se la
  divisione per zero la fa la dimostrazione, e non l'enunciato, lo dice. Sa
  decidere i polinomi in `n` e in potenze come `2^n`, anche insieme
  (`2^n - n` non si annulla mai), e il segno di un seno o di un coseno dal
  quadrante in cui cade l'angolo;
- oppure, se nessun valore fisso dei parametri va bene per ogni `n`, che per
  ogni `n` vadano bene tutti i valori abbastanza grandi: la somma dei
  `\ln(x - k)` è definita per `x > n`, e risulta dimostrata;
- che nessun divisore sia zero di nascosto: `2\cos\frac{\pi}{3} - 1` è zero
  anche se l'algebra non lo riduce a zero. Un divisore conta come diverso da
  zero solo quando una regola lo prova.

Se uno di questi controlli non sa decidere, la risposta è `NON DIMOSTRATO`.

**Enunciati sugli insiemi.** Uguaglianze, inclusioni ed equivalenze tra
espressioni con unione, intersezione, differenza, insieme delle parti e
prodotto cartesiano, per esempio

```latex
(S \setminus T) \setminus V = S \setminus (T \cup V)
```

Le lettere maiuscole senza pedice sono insiemi. Il programma decide
l'enunciato esaminando quali regioni del diagramma di Venn possono essere
vuote, e lo ricontrolla con un secondo metodo, su tutti gli insiemi di un
piccolo universo. Poi scrive la dimostrazione in inglese, nello stile delle
soluzioni di un libro: "sia x un elemento di ...; allora ...; viceversa ...".
Per un enunciato falso stampa un controesempio, per esempio `S = {1}, T = {2}`.

**Enunciati con quantificatori su insiemi di numeri.** Frasi con "per ogni" ed
"esiste" su ℕ, ℕ₀, ℤ, ℚ e ℚ⁺, anche a proposito di un'applicazione definita da
una formula. È così che si scrive che un'applicazione è iniettiva o
suriettiva: per esteso, con la definizione. Per esempio, "n ↦ 3n da ℕ₀ a ℕ₀ è
iniettiva":

```latex
(f \colon \mathbb{N}_{0} \to \mathbb{N}_{0} \land \forall n \in \mathbb{N}_{0}\ (f(n) = 3n))
    \implies \forall a, b \in \mathbb{N}_{0}\ (f(a) = f(b) \implies a = b)
```

Il programma non conosce le parole "iniettiva" e "suriettiva": applica poche
regole generali (scomporre un polinomio, trovarne le radici razionali,
studiarne il segno, risolvere un'equazione, provare i numeri più semplici) e
scrive ogni passo. Due regole collegano questa famiglia alla prima:

- un "per ogni `n` naturale" seguito da un'uguaglianza viene dimostrato **per
  induzione**, con lo stesso metodo e gli stessi controlli degli enunciati
  della prima famiglia. Così
  `\forall n \in \mathbb{N}_{0}\ (\sum_{k=1}^{n} k = \frac{n(n+1)}{2})` è
  dimostrato, anche dentro un enunciato più grande;
- un "per ogni `x` intero" viene dimostrato **per casi sul resto** di `x` in
  una divisione. È così che dimostra che ogni intero è pari o dispari,
  `\forall x \in \mathbb{Z}\ \exists y \in \mathbb{Z}\ (x = 2y \lor x = 2y + 1)`,
  che `x^3 - x` è multiplo di 6, che un quadrato diviso per 4 dà resto 0 o 1.
  Il modulo lo trova da solo, da 2 a 12.

Le espressioni possono contenere somme, potenze e funzioni; le regole sui
polinomi valgono però solo per le espressioni razionali, e le altre passano
dall'induzione o da un conto tra numeri in cui ogni logaritmo, radice e
divisione viene controllato. Poi ricontrolla il risultato calcolando con i numeri su un
campione. Se l'enunciato è falso ne dimostra la negazione e
stampa i numeri che la provano, per esempio `a = 1, b = -1`.

**Controllo delle dimostrazioni scritte.** Ogni verdetto `DIMOSTRATO` ha la
sua dimostrazione scritta, con i passaggi. Per le induzioni: i due membri nel
caso base con il loro valore, i termini che ogni somma guadagna passando da
`n` a `n + 1`, il valore che l'ipotesi induttiva dà alla somma, e ciò che i
due membri diventano. Ciò che è scritto viene ricontrollato per un'altra
strada: per gli insiemi e per i quantificatori ogni fatto, prima di essere
scritto, con un metodo diverso da quello che lo ha ricavato; per le induzioni
ogni formula viene riletta dal testo, come se l'avesse scritta l'utente, e
ricalcolata con i numeri. Una dimostrazione che non supera il controllo non
finisce nel documento.

Le frasi che collegano i fatti sono scritte secondo schemi fissi. Non c'è un
controllo automatico che le capisca: le fissano i test, con una frase attesa
per ogni combinazione di uguale e diverso, affermato e negato.

**Certificati, e un verificatore indipendente.** I controlli descritti fin
qui li fa il programma su se stesso: un errore nella sua aritmetica li
attraverserebbe tutti. Per questo ogni dimostrazione per induzione viene
anche consegnata a un secondo programma, `checker.c`, sotto forma di
*certificato*: l'enunciato, il caso base, e un solo dato in più, che basta a
ricontrollare il passo induttivo senza rifare la ricerca.

```
certificate induction
variable n
base 1
statement (= (prod k 1 n (/ (+ k 1) k)) (+ n 1))
factor (/ (+ 2 n) (+ 1 n))
end
```

Il dato in più è il *fattore* `W`: il certificato afferma che la differenza
dei due membri in `n + 1` è `W` volte quella in `n`. Il verificatore
ricalcola le due differenze con i suoi polinomi e controlla tre cose: che nel
caso base i due membri coincidano, che l'uguaglianza con `W` valga davvero, e
che il denominatore di `W` non possa annullarsi dove l'enunciato è definito
(altrimenti basterebbe scegliere `W` uguale al rapporto delle due differenze,
e si "dimostrerebbe" qualunque cosa). Trovare `W` è il lavoro del motore;
controllarlo sono poche moltiplicazioni e un confronto.

`checker.c` non usa nulla di `main.c`: ha i suoi numeri, il suo lettore, i
suoi polinomi, e si compila da solo. Risponde *accettato*, *respinto*, oppure
*non controllabile* quando il certificato chiede qualcosa che non conosce; la
terza risposta non è un sì. Se respinge, il verdetto è `NON DIMOSTRATO`.

Il verdetto dice sempre come stanno le cose:

```
DIMOSTRATO per induzione su n >= 1; certificato verificato
DIMOSTRATO per induzione su n >= 1; senza certificato (l'enunciato o il passo induttivo usano qualcosa che la notazione dei certificati non ha)
DIMOSTRATO per ogni scelta degli insiemi; senza certificato
```

Che cosa dimostra un certificato accettato è scritto con precisione in testa
a `checker.c`, ed è meno del verdetto:

- dimostra che i due membri sono uguali per ogni `n` dal caso base in su,
  *dove l'enunciato è definito* fino a quel `n`. Che sia definito per ogni
  `n` lo stabiliscono i controlli del motore descritti sopra, che il
  certificato non copre;
- riguarda l'enunciato come compare nel certificato, riscritto dal motore in
  una notazione con l'operatore davanti: la lettura del LaTeX resta fuori;
- quando dice "con un'ipotesi in più sui parametri", il certificato vale dove
  un'espressione dei parametri, che vi è dichiarata, non è zero.

La notazione dei certificati sa dire frazioni, somme, prodotti, potenze come
`2^n` e `x^n`, `(-1)^n`, fattoriali, binomiali, numeri di Fibonacci e numeri
armonici. Non sa dire radici, logaritmi, seni e arcotangenti.

## Cosa non fa ancora

- Disuguaglianze.
- Somme il cui termine dipende anche da `n`, come quelle con `\binom{n}{k}`.
- Enunciati scritti a parole, derivate, integrali.
- Le condizioni scritte dopo la formula, tranne l'inizio dell'induzione
  (`$n\ge 2$`). Con una condizione non letta, che sia una formula come
  `$a>0$` o una parola come "pari", un enunciato può essere dimostrato ma non
  dichiarato falso; con una seconda uguaglianza nello stesso `\item` la
  risposta è `NON SUPPORTATO`. Un inciso tra parentesi fatto di sole parole,
  come "(nucleo di Dirichlet)", è letto come un commento: non ferma il
  verdetto `FALSO`, che però dichiara di averlo ignorato. Nel dubbio (una
  lettera isolata, una cifra, parole come "pari", "se", "per") l'inciso resta
  una condizione.
- Decidere che un'espressione è definita per ogni `n` in tutti i casi. Non
  lo sa fare, e risponde `NON DIMOSTRATO` anche se l'enunciato è vero, per
  espressioni con fattoriali o somme (`n! - 5`), per un'espressione
  trigonometrica che non sia un solo seno o coseno (per un fattore positivo),
  per un angolo molto vicino a un multiplo di π/2, per il segno di costanti
  come `\ln 3 - 1`.
- Scegliere i valori dei parametri in ogni caso: li ricava dalle condizioni
  che sono polinomi in un parametro o nella radice di un'espressione di primo
  grado (`\sqrt{x - 5000} > 100`), un parametro alla volta. Se la condizione
  passa per il logaritmo o il seno del parametro prova solo pochi numeri
  semplici.
- Combinare i metodi liberamente. L'induzione entra negli enunciati con
  quantificatori solo per un "per ogni `n`" senza altre variabili intorno; i
  casi sul resto non si annidano; gli enunciati sugli insiemi restano un
  linguaggio a parte.
- Un certificato per ogni dimostrazione. Lo hanno le induzioni con somme,
  prodotti, potenze, fattoriali, binomiali e successioni per ricorrenza. Non
  lo hanno ancora le induzioni con radici, logaritmi, seni e arcotangenti, né
  le dimostrazioni sugli insiemi e quelle con quantificatori (dove
  un'induzione interna viene comunque data al verificatore, ma il verdetto
  d'insieme resta "senza certificato"). E anche dove c'è, il certificato non
  copre il fatto che l'enunciato sia definito per ogni `n`.
- Negli enunciati con quantificatori: numeri reali e complessi,
  disuguaglianze, valore assoluto, applicazioni definite per casi o a valori
  in coppie.
- Applicazioni tra insiemi qualsiasi, relazioni d'equivalenza e d'ordine,
  cardinalità.
- Nelle dimostrazioni per induzione con seni e coseni i risultati dei conti
  non vengono scritti: il programma li fa con gli esponenziali complessi, una
  forma che non si legge bene. Il testo lo dice, e riporta i passaggi che può.
- Per gli insiemi delle parti e per gli enunciati con "esiste un insieme" la
  dimostrazione scritta ragiona per regioni e casi, non ancora in forma
  discorsiva.

## Compilare e usare

Servono GCC o Clang.

```sh
cc -std=c11 -O2 -Wall -Wextra -o mathresearcher main.c   # il programma (include checker.c)
cc -std=c11 -O2 -Wall -Wextra -o checker checker.c       # il verificatore, da solo
```

Modi d'uso:

```sh
./mathresearcher file.tex                         # un verdetto per enunciato
./mathresearcher --proofs dimostrazioni.pdf file.tex   # scrive anche le dimostrazioni
./mathresearcher --certificates certificati.cert file.tex   # scrive anche i certificati
./mathresearcher                                  # legge un enunciato da stdin
./checker certificati.cert                        # ricontrolla i certificati, senza il motore
```

Un file di ingresso è un documento LaTeX in cui ogni `\item` di un elenco
contiene un enunciato, scritto come `$\displaystyle ...$` oppure `\[ ... \]`.

Con `--certificates` il programma scrive in un file i certificati che il
verificatore ha accettato, uno dopo l'altro. `checker` li rilegge e dice di
ognuno se lo accetta: è lo stesso controllo, fatto da un eseguibile che del
motore non contiene nulla. Esce con 0 solo se li accetta tutti.

Con `--proofs` il programma scrive un documento LaTeX con ogni enunciato e la
sua dimostrazione. Se il nome finisce in `.pdf` scrive il `.tex` con lo stesso
nome e lo compila: per questo serve un compilatore LaTeX installato
(`tectonic` oppure `pdflatex`). È l'unico strumento esterno usato, e solo per
questo passo.

## Test

I test stanno in `tests/`, divisi per dominio. Ogni dominio ha enunciati veri,
che devono essere dimostrati, ed enunciati falsi, che non devono esserlo mai.

| Cartella | Contenuto |
|---|---|
| `tests/engine/` | comportamento generale del motore |
| `tests/sums_products_inequalities/` | somme e prodotti con formula chiusa |
| `tests/logs_trig_powers_roots/` | logaritmi, trigonometria, potenze, radici |
| `tests/set_theory/` | operazioni tra insiemi e prodotti cartesiani; applicazioni, relazioni, insiemi finiti e infiniti |
| `tests/integers/` | parità, resti e divisibilità |
| `tests/checker/` | il verificatore da solo, con certificati giusti, sbagliati e non controllabili; e i certificati scritti dal motore, anche alterati |

Tra gli enunciati falsi ce ne sono di costruiti per ingannare: uguali al vero
per i primi undici valori di `n` o più, definiti e veri per i primi `n` ma non
definiti più avanti, con una divisione per un numero che è zero solo grazie a
un'identità, con il logaritmo di un seno o di un coseno negativo, veri per
tutti i resti di una divisione tranne uno, veri su un insieme più piccolo,
veri con un altro codominio.

Ogni test si compila e si lancia con un solo comando, scritto in testa al file.
Per esempio:

```sh
cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_engine tests/engine/test_engine.c && /tmp/test_engine
```

Due test oggi falliscono, perché chiedono che tutti i loro enunciati siano
dimostrati e non lo sono ancora: quello di `logs_trig_powers_roots` (110
enunciati su 200) e, in `set_theory`, `test_maps_and_relations.c`
(applicazioni e relazioni: 10 enunciati su 136).

## Fuzz

Quasi tutti gli errori trovati finora li hanno trovati gli script di
`tools/fuzz/`. Generano enunciati (o certificati) a caso, li danno al
programma e confrontano la risposta con un oracolo indipendente scritto in
Python: un conto fatto per un'altra strada, con i numeri. Non servono né per
compilare né per i test; chiedono solo Python 3.

| Script | Cosa mette alla prova |
|---|---|
| `fuzz_induction.py` | uguaglianze per induzione, con varianti false insidiose |
| `fuzz_maps.py` | enunciati con quantificatori e applicazioni |
| `fuzz_sets.py` | enunciati sugli insiemi |
| `fuzz_integers.py` | resti e divisibilità |
| `fuzz_checker.py` | il verificatore, con certificati alterati e costruiti per ingannare |
| `fuzz_crash.py` | robustezza: enunciati rovinati, con un eseguibile compilato con i sanitizer |
| `latex_oracle.py` | la lettura del LaTeX, con un secondo lettore |
| `compare.py` | due versioni del programma sugli stessi file |

```sh
python3 -I tools/fuzz/fuzz_induction.py ./mathresearcher 1 1000 /tmp/fuzz
```

Una riga `BUG` vuol dire che il programma ha dimostrato un falso.

## Struttura

- `main.c`: il programma, con il motore, organizzato in capitoli come un libro.
- `checker.c`: il verificatore indipendente dei certificati. Non usa nulla di
  `main.c`.
- `AGENTS.md`: le regole seguite nello scrivere codice e test.
- `tests/`: dati (`.tex`), descrizioni (`*_about.tex`) e programmi di test (`.c`).
- `tools/fuzz/`: gli script di fuzz, in Python.
