# AGENTS.md

Istruzioni per gli agenti che lavorano su questo repository.

## Regola sulla lingua: commenti in italiano, test in inglese

- **Tutti i commenti nel codice sono scritti in italiano**: commenti di design, function comments, why/teacher/checklist/guide comments, banner dei capitoli. Vale anche per i commenti dentro i file di test.
- **I test sono scritti in inglese**: nomi dei test, descrizioni, messaggi di errore e asserzioni.
- Gli identificatori (variabili, funzioni, tipi) e i nomi dei file sono in inglese.

## Regola di struttura: due file `.c`, ognuno organizzato come un libro

Il codice vive in **due file sorgente**:

- `main.c`: il programma, con il motore. Tutto ciò che legge, cerca, calcola e scrive sta qui.
- `checker.c`: il verificatore indipendente dei certificati (vedi la regola sul verificatore, più sotto). È l'unica eccezione al file unico, voluta perché chi controlla una dimostrazione non deve condividere codice con chi l'ha prodotta.

Non creare altri `.c` né header separati, salvo richiesta esplicita.

Ognuno dei due file si legge dall'inizio alla fine come un libro, quindi va diviso in **capitoli/paragrafi** nell'ordine in cui un lettore ne ha bisogno:

1. **Commento di design in testa al file** (vedi sotto): cosa fa il programma, come, e perché con queste scelte.
2. **Indice dei capitoli**, in un commento, subito dopo il commento di design.
3. **Capitoli numerati**, ognuno aperto da un banner ben visibile, ad esempio:

```c
/* =============================================================================
 * 3. PARSING DELL'INPUT
 * ============================================================================
 *
 * Breve introduzione al capitolo: che problema risolve, quali strutture usa,
 * cosa si assume vero quando si arriva qui.
 */
```

4. Ordine tipico dei capitoli: include e costanti → tipi e strutture dati → utilità di base → logica del dominio → I/O → `main()` in fondo. Le dipendenze vanno dall'alto verso il basso: un capitolo usa solo ciò che è già stato presentato sopra, così da non servire prototipi in avanti se non strettamente necessari.
5. Dentro ogni capitolo, il codice è diviso in **paragrafi**: gruppi di istruzioni correlate separati da una riga vuota e introdotti da un commento (guide comment).

Quando aggiungi codice, mettilo nel capitolo giusto. Se non esiste un capitolo adatto, creane uno nuovo e aggiorna l'indice.

## Regola sul verificatore: `checker.c` resta piccolo e indipendente

Quando il motore dimostra un enunciato scrive, dove può, un **certificato**: un testo in un linguaggio piccolo e fisso che contiene l'enunciato e i pochi dati che servono a ricontrollare la dimostrazione. `checker.c` legge il certificato e lo accetta o lo respinge. Trovare una dimostrazione è difficile, controllarla è facile: per questo il verificatore può essere piccolo, e ci si può fidare di lui più che del motore.

- **Indipendenza.** `checker.c` usa solo la libreria standard C. Non include `main.c` e non usa nessuna sua funzione, tipo o variabile: ha la sua aritmetica, il suo lettore, i suoi polinomi. Deve compilarsi da solo, con `cc -std=c11 -O2 -Wall -Wextra -o checker checker.c`, e un test in `tests/checker/` lo compila così proprio per garantirlo.
- **La duplicazione è voluta.** Che in `main.c` e in `checker.c` ci siano due somme di polinomi è il punto, non un difetto. La review non deve "eliminare la duplicazione" spostando codice dall'uno all'altro o in un file comune.
- **Verso unico.** `main.c` può includere `checker.c` (così il progetto resta un solo comando di compilazione) e chiamarne solo la funzione d'ingresso, quella che riceve il testo di un certificato e risponde. `checker.c` non sa nulla di `main.c`. I suoi nomi hanno il prefisso `ck_`, per non scontrarsi con quelli del programma.
- **Piccolo.** Nel verificatore entra solo ciò che serve a controllare, mai a cercare. Niente euristiche, niente tentativi: se per accettare un certificato servirebbe una ricerca, è il certificato che deve portare con sé il risultato della ricerca.
- **Mai indebolirlo per far passare un certificato.** Se il verificatore respinge il certificato di una dimostrazione, si corregge il motore, o si arricchisce il certificato. Una regola nuova entra nel verificatore solo con la sua giustificazione matematica scritta in un teacher comment, e con i test che mostrano che i certificati sbagliati restano respinti.
- **Tre risposte.** Il verificatore risponde "accettato", "respinto" (un controllo è fallito) oppure "non so controllarlo" (il certificato usa qualcosa che non conosce, o i numeri sono troppo grandi). La terza non è un sì.
- **Che cosa certifica, va detto.** Il commento di design di `checker.c` enuncia con precisione il teorema che un certificato accettato dimostra, comprese le ipotesi (per esempio "dove i due membri sono definiti"). Il motore non deve presentare come certificato più di questo.

## Regola sulle dipendenze: nessuna dipendenza esterna

**Non introdurre dipendenze esterne**, a meno che l'utente non le richieda esplicitamente. Questo vale per librerie di terze parti, framework, strumenti di build aggiuntivi e codice copiato da altri progetti.

- Usa solo la libreria standard C (e, se necessario, la libreria matematica `-lm`).
- Se ti serve una funzionalità mancante, scrivila nel file: una versione piccola e adatta al caso d'uso vale più di una libreria generica.
- Se pensi che una dipendenza sia davvero necessaria, **fermati e chiedilo all'utente** spiegando perché, invece di aggiungerla.
- Il progetto deve compilarsi con un solo comando del compilatore, `cc -std=c11 -O2 -o mathresearcher main.c` (`main.c` include `checker.c`). Il verificatore si compila anche da solo, con `cc -std=c11 -O2 -o checker checker.c`.

## Regola sugli strumenti di fuzz: `tools/fuzz/`, fuori dal programma e dai test

`tools/fuzz/` contiene script Python che generano enunciati e certificati a caso e confrontano le risposte del programma con un **oracolo indipendente**: un secondo modo di stabilire la verità, scritto nello script, che non chiama mai il programma per decidere.

- **Non sono né programma né test.** Il progetto si compila e i test girano senza Python. È un'eccezione dichiarata alla regola "solo C", come il compilatore LaTeX per i PDF: non aggiungerne altre senza chiedere.
- **Solo libreria standard di Python 3.** Niente pacchetti da installare. Si lanciano con `python3 -I`, e l'uso è scritto in testa a ogni script.
- **Il fuzz trova, i test fissano.** Un errore trovato da uno script diventa un test in `tests/`, in C, con l'enunciato o il certificato come dato. Uno script non sostituisce un test.
- **Quando usarli.** Dopo una modifica al motore o al verificatore, con semi nuovi. Una riga `BUG` vuol dire che il programma ha dimostrato un falso, o che il verificatore ha accettato un certificato falso: è il caso più grave, e va corretto prima di tutto il resto. `SUSPECT` vuol dire che l'oracolo non è riuscito a confermare nella sua finestra di numeri: va guardato a mano.
- **L'oracolo resta indipendente.** Non si corregge un oracolo copiandovi il ragionamento del motore: se i due non sono d'accordo, si capisce chi ha ragione con un terzo conto.
- I file che gli script producono (enunciati, certificati, risultati) vanno in una cartella di lavoro data come argomento, mai dentro il repository.

## Regola sul motore: sempre general-purpose, mai hardcoded

Il **motore** è tutto ciò che esegue la **ricerca**, il **calcolo** e la **produzione del risultato finale**. Deve essere **general-purpose**. Non deve mai essere hardcoded né specifico per un problema, un teorema, un dataset o un esempio.

Questo vale per tutte e tre le parti: la ricerca non segue percorsi fissi verso una risposta nota, il calcolo non contiene formule valide solo per un caso, e il risultato finale viene prodotto dal metodo generale, mai scritto a mano o preconfezionato.

- Il motore non contiene conoscenza di casi particolari: niente `if` su un problema noto, niente costanti magiche legate a un esempio, niente scorciatoie che funzionano solo per un input specifico.
- Ogni problema entra nel motore **come dato** (input, definizione, parametri), mai come codice nel motore. Se per supportare un nuovo problema devi modificare il motore, l'astrazione è sbagliata: correggila.
- Se un caso particolare richiede logica propria, tienila **fuori dal motore**, in un capitolo separato del file (ad esempio "problemi/esempi"), che usa il motore attraverso la sua interfaccia generale.
- Non barare sui test: un risultato ottenuto perché il motore "sa già" la risposta non vale. Il motore deve arrivarci con il metodo generale.
- Quando una soluzione specifica sembra più comoda, chiediti quale regola generale la produce, e implementa quella. Documenta la scelta in un design comment.
- In review verifica esplicitamente che nessun dettaglio specifico sia filtrato nel motore.

## Regola sui file di testo richiesti: sempre `.tex`

Quando l'utente chiede un file di testo (ad esempio "fammi un `.txt` di X del tipo Y" da mettere in `tests/`), l'agente **non produce un `.txt`: produce un file `.tex`**. I `.txt` non esistono più nel progetto.

- Il formato segue gli esempi già presenti in `tests/` (ad esempio `tests/sums_products_inequalities/simple_inductions.tex`): documento `article` completo con `amsmath`/`amssymb`, titolo, breve introduzione e un `\begin{enumerate}` con un `\item` per elemento, formula in `$\displaystyle ...$`.
- Prima di creare un nuovo file, leggi un `.tex` già presente in `tests/` e rispetta lo stesso preambolo e la stessa struttura.
- La matematica è sempre in sintassi LaTeX, mai in Unicode o ASCII-art. Nessun pacchetto non standard.
- Il file va nella cartella `tests/` del dominio giusto (vedi sezione sui test).
- Se il programma stesso produce documenti di output (ad esempio il risultato finale), usa la stessa convenzione `.tex`, scrivendoli con la sola libreria standard C.
- I certificati (opzione `--certificates`) non sono documenti: sono dati per `checker`, nella sua notazione, e stanno in un file `.cert`. Nei test i certificati sono scritti dentro i file `.c` di `tests/checker/`.

## Regola sui test: nella cartella `tests/`

L'agente scrive i test e li mette **sempre nella cartella `tests/`**, mai accanto al sorgente principale e mai dentro il file `.c` del programma.

- Il programma resta in `main.c` e `checker.c`; la regola sui file sorgente vale per il codice del programma, non per i test.
- I test del verificatore stanno in `tests/checker/`: certificati giusti che devono essere accettati e, soprattutto, certificati sbagliati che devono essere respinti (un passo alterato, un numero cambiato, un enunciato diverso da quello dimostrato).
- I test sono **suddivisi per dominio**: un file (o una sottocartella) per ogni dominio, ad esempio `tests/algebra/`, `tests/geometria/`, `tests/analisi/`. Un test sta nel dominio a cui appartiene il problema che verifica, non in un contenitore generico.
- I test del motore stesso (ricerca, calcolo, produzione del risultato) stanno in un dominio a parte, ad esempio `tests/motore/`, e verificano il comportamento generale senza riferirsi a un problema specifico.
- Se serve un nuovo dominio, crealo; non mettere test di domini diversi nello stesso file.
- I test non introducono dipendenze esterne: niente framework di test, solo C standard (ad esempio asserzioni semplici e un eseguibile che restituisce un codice di uscita diverso da zero in caso di fallimento).
- I test devono poter essere compilati ed eseguiti con un solo comando, documentato in un commento in testa al file di test.
- I casi particolari vivono nei test, come dati forniti al motore. Non vanno mai copiati dentro il motore per far passare un test.
- Servono anche test anti-regressione contro le false dimostrazioni (vedi la regola sulla correttezza), sempre nel dominio giusto in `tests/`.
- Quando correggi un bug, aggiungi in `tests/` un test che lo riproduce. Nella review finale rieseguili tutti per escludere regressioni.

## Regola sulla correttezza: il motore non deve mai dimostrare il falso

**Ogni volta** che lavori sul motore, o fai girare il motore su un enunciato, devi assicurarti che **non stia dimostrando qualcosa di falso**. Una "dimostrazione" di un'affermazione falsa è il peggior bug possibile, peggiore di un crash: il motore deve preferire non rispondere piuttosto che rispondere male.

- **Verifica indipendente del risultato:** ogni dimostrazione prodotta va controllata con un metodo diverso da quello che l'ha generata (ad esempio un controllo passo per passo di ogni deduzione, oppure una verifica numerica o esaustiva sui casi piccoli). Un risultato non verificato non è un risultato.
- **Certificato:** dove il linguaggio dei certificati copre l'enunciato, il verdetto "dimostrato" richiede che `checker.c` abbia accettato il certificato. Un certificato respinto è un errore del motore: il verdetto diventa "non dimostrato", e l'enunciato diventa un test. Dove il linguaggio non arriva, il verdetto deve dire che il certificato manca.
- **Test su enunciati falsi:** in `tests/` devono esistere enunciati **falsi** (controesempi noti, formule con un coefficiente sbagliato, disuguaglianze che valgono solo per certi `n`). Il motore deve rifiutarli o segnalare che non riesce a dimostrarli, mai "dimostrarli".
- **Test anti-regressione:** oltre ai falsi generici, in `tests/` ci devono essere test anti-regressione che impediscono al motore di essere spinto, da una modifica futura, a provare qualcosa di falso. Ogni volta che il motore dimostra (o rischia di dimostrare) un falso, l'enunciato che lo ha causato diventa un test permanente che deve restare respinto. Questi test non si cancellano né si indeboliscono per far passare una modifica: se falliscono, si corregge il motore.
- **Ogni passo deve essere giustificato:** il motore applica solo regole di deduzione valide e dichiara le ipotesi usate. Niente passaggi taciti, niente assunzioni non dichiarate, niente divisioni per zero o dominî ignorati.
- **Nel dubbio, fallire onestamente:** se il motore non riesce a dimostrare un enunciato, restituisce "non dimostrato", non un esito positivo e non un tentativo spacciato per dimostrazione.
- **Dopo ogni modifica al motore:** riesegui tutti i test, enunciati falsi inclusi, e controlla che nessuno sia diventato "dimostrato". Una semplificazione o un'ottimizzazione che fa passare un falso è una regressione.
- **Riportare con onestà:** nel riferire il risultato di una task, dì chiaramente cosa è stato verificato e come, e cosa no.

## Regola sulla semplicità: codice minimale

Scrivi il minimo codice che risolve il problema in modo corretto e leggibile.

- Niente funzionalità non richieste, niente generalizzazioni "per il futuro", niente parametri o opzioni che nessuno usa.
- Niente astrazioni premature: introduci una funzione, una struct o una macro solo quando elimina una duplicazione reale o rende il codice più chiaro.
- Preferisci soluzioni semplici e dirette a quelle ingegnose. Se una soluzione semplice basta, dillo in un design comment (vedi sotto).
- Meno righe non significa codice migliore: la leggibilità e i commenti non vanno sacrificati per accorciare.

## Review obbligatoria prima di completare una task

**Prima di dichiarare conclusa una task, rileggi tutto il codice che hai toccato** (e quello che lo circonda) e fai una review critica con due obiettivi:

1. **Eliminare la complessità inutile**: codice morto, variabili e parametri non usati, casi speciali superflui, rami impossibili, indirezioni che non servono, duplicazioni.
2. **Trovare dove astrarre meglio**: logica ripetuta che merita una funzione, responsabilità mescolate in un'unica funzione, nomi che non dicono cosa fanno, capitoli del file che non rispecchiano più il contenuto.

Vincoli della review, per non peggiorare le cose:

- **Niente sub-ottimizzazione**: non rendere il codice più lento o più sprecone di memoria solo per semplificarlo, soprattutto nei cicli critici. Se una scelta meno semplice è motivata dalle prestazioni, lasciala e spiegala in un why comment.
- **Niente regressioni**: ogni semplificazione o refactoring deve preservare il comportamento esistente. Dopo aver modificato, ricompila (senza warning) e riesegui i test o gli esempi disponibili, e confronta i risultati con quelli di prima.
- Se non puoi verificare che un refactoring sia sicuro, non farlo e segnalalo all'utente.
- Aggiorna commenti, indice dei capitoli e commento di design in base a ciò che la review ha cambiato.

## Regola sui commenti

I commenti sono parte del codice, non un'aggiunta. Scrivili per il lettore futuro (che può essere te stesso tra sei mesi): servono a **ridurre il carico cognitivo** e a spiegare ciò che il codice da solo non può dire. Un commento scritto bene è anche un test: se non riesci a spiegare un comportamento, probabilmente c'è un buco da sistemare.

Non basta "il codice si spiega da solo". Molti commenti non dicono *cosa* fa il codice, ma *perché* lo fa, o perché lo fa così invece che nel modo che sembrerebbe più naturale.

### Tipi di commento da scrivere

1. **Function comments**: sopra ogni funzione (e macro, struct, blocco con interfaccia propria). Descrivono contratto, parametri, valore di ritorno, effetti collaterali, casi di errore. Chi legge il chiamante deve poter trattare la funzione come una scatola nera senza leggerne il corpo. Quando cambi la funzione, aggiorna il commento nello stesso momento.

   **Ogni `struct` (e `enum`, `union`, `typedef`) deve avere un commento** che dice:
   - **a quale astrazione serve**: quale concetto del dominio o del motore rappresenta e che ruolo ha nel programma;
   - **a cosa si riferisce**: cosa indica un'istanza (es. "un'espressione", "uno stato di ricerca") e, campo per campo, cosa significa ogni membro, con unità, intervalli validi, invarianti e chi possiede la memoria puntata.

   ```c
   /* Rappresenta <astrazione>: <cosa indica una singola istanza>.
    * Invariante: <condizioni sempre vere>. */
   struct Example {
       int n;      /* Numero di elementi validi in 'v'. */
       double *v;  /* Elementi, posseduti dalla struct. */
   };
   ```

2. **Design comments**: in testa al file e all'inizio dei capitoli importanti. Spiegano l'approccio generale, gli algoritmi scelti, i trucchi usati e, se la soluzione sembra troppo semplice, quali alternative sono state considerate e perché questa basta. La semplicità deve risultare frutto di un ragionamento, non di pigrizia.

3. **Why comments**: spiegano il motivo di un'istruzione anche quando *cosa* fa è ovvio. Obbligatori quando l'ordine delle operazioni è importante, quando una riga sembra spostabile ma non lo è, o quando c'è un vincolo non evidente (invarianti, stabilità numerica, casi limite, bug passati).

4. **Teacher comments**: questo è un progetto di matematica. Quando il codice usa un concetto, un teorema, una formula o un algoritmo che il lettore potrebbe non ricordare, **insegnalo nel commento**: enuncia il risultato, giustifica i passaggi, indica le ipotesi, e se serve riporta un riferimento. Quando un algoritmo ha più casi, descrivi i casi nel commento con etichette (`3a`, `3b`…) e riusa le stesse etichette nel codice che li implementa.

5. **Checklist comments**: dove una modifica qui richiede una modifica altrove (aggiungere un caso a un `enum` e a una tabella, rispettare un ordine di chiamata, ecc.) lascia un avviso esplicito: `Warning: se aggiungi X, aggiorna anche Y()`.

6. **Guide comments**: breve commento prima di ogni paragrafo di codice che dice cosa sta per succedere. Danno ritmo, dividono il codice in sezioni e confermano al lettore di aver capito bene. Sono encouraged, ma devono essere più veloci da leggere del codice che introducono.

   Quando lavori con API a stato implicito (stack, buffer, iteratori, indici), **annota lo stato dopo ogni passo**, ad esempio `/* Stack: a, b, c */`, così il lettore non deve ricostruirlo a mente.

### Tipi di commento da evitare

- **Trivial comments**: commenti che costano più fatica del codice che descrivono. Vietato: `i++; /* incrementa i */`.
- **Backup comments**: niente codice vecchio commentato "per sicurezza". C'è git. Se il codice non è pronto, non va committato.
- **Debt comments** (`TODO`, `FIXME`, `XXX`, "hack"): da evitare. Se proprio inevitabili, devono essere specifici (cosa manca, quando servirebbe, dove intervenire) e vanno segnalati all'utente a fine lavoro. Meglio spiegare nel commento di design perché una cosa non c'è ancora e dove andrebbe aggiunta.

### Regole pratiche

- Un commento che spiega un bug corretto o una regressione ("lo spostiamo qui perché prima…") è un buon commento: aggiungilo nel momento in cui correggi.
- Commenti e codice devono restare coerenti: se modifichi il comportamento, modifica il commento. Un commento stale è peggio di nessun commento.
- Scrivi per essere letto da programmatori anche non esperti del dominio matematico: i teacher comments servono a questo.
- Non aggiungere commenti decorativi o ripetitivi; ogni commento deve guadagnarsi il suo posto riducendo lo sforzo del lettore o trasmettendo un'informazione che il codice non ha.

## Prima di consegnare

- Il progetto è ancora fatto di `main.c` e `checker.c` soltanto, senza dipendenze esterne non richieste?
- `checker.c` compila da solo, non usa nulla di `main.c`, e non è stato indebolito per far passare un certificato?
- Il motore è ancora general-purpose, senza nulla di hardcoded o specifico per un problema?
- I test nuovi o aggiornati sono in `tests/` e passano tutti?
- Il motore non dimostra nulla di falso? Risultati verificati in modo indipendente e test su enunciati falsi tutti respinti?
- Ho fatto la review: complessità inutile rimossa, astrazioni migliorate, nessuna regressione né sub-ottimizzazione?
- Il commento di design e l'indice dei capitoli sono aggiornati?
- Commenti in italiano e test in inglese?
- Ogni nuova funzione ha il suo function comment?
- Ogni nuova `struct` dice a quale astrazione serve e a cosa si riferisce?
- Ogni scelta non ovvia ha il suo why comment? Ogni concetto matematico non banale ha il suo teacher comment?
- Nessun commento banale, nessun codice commentato, nessun TODO senza spiegazione?
