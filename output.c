/* =============================================================================
 * output.c: la generazione del documento finale
 * =============================================================================
 *
 * COSA FA
 *
 * Scrive il risultato del programma come documento: per ogni enunciato letto
 * la formula, poi la dimostrazione o le soluzioni che il motore ha trovato,
 * oppure il verdetto quando una dimostrazione non c'è, e sotto il grafico
 * quando il motore ne ha fatto uno. Il documento è un file LaTeX, con il
 * preambolo dei file in tests/; su richiesta lo fa compilare in PDF.
 *
 * Qui non si decide nulla e non si calcola nulla: si riporta ciò che il
 * motore ha già stabilito e ricontrollato. Se scrivere un risultato
 * richiedesse un conto, quel conto va fatto nel motore, dove viene
 * verificato, e consegnato qui come testo.
 *
 * COME È FATTO
 *
 * Due file, scritti insieme. Il documento .tex, una riga alla volta, con
 * fprintf. E il PDF dei grafici, che ha lo stesso nome con "-plots.pdf" in
 * fondo e una pagina per grafico: lo scrive plot.c, e il documento include
 * ogni pagina con \includegraphics. Il PDF dei grafici si apre al primo
 * grafico, così un documento senza grafici non lascia un file vuoto accanto.
 *
 * L'unica cosa che qui si sceglie è la disposizione delle formule: una
 * troppo larga per stare in una riga (una derivata non semplificata, un
 * intervallo con estremi di sedici cifre) va in evidenza, e \fitmath la
 * rimpicciolisce se ancora non ci sta (put_proof()). Il testo della formula
 * resta quello del motore.
 *
 * PERCHÉ IL PDF FINALE PASSA DA LATEX
 *
 * I grafici sono linee e poche scritte, e plot.c li scrive in PDF da solo.
 * Il documento invece è fatto di formule: comporle (frazioni, radici, indici,
 * allineamenti, andate a capo) è il mestiere di un compilatore LaTeX, e
 * riscriverne uno non sarebbe codice minimale. Per questo il PDF finale si
 * ottiene chiamando un compilatore già installato, l'unica cosa esterna su
 * cui il programma si appoggia; senza, resta il documento .tex, completo.
 *
 * LA SPIEGAZIONE
 *
 * Di un enunciato di cui il motore ha registrato i passaggi (capitolo 40 di
 * main.c) il documento non riporta solo la dimostrazione: prima spiega, come
 * alla lavagna. Che cosa si chiede, la figura, poi un passo alla volta, e in
 * fondo la dimostrazione di sempre, in piccolo, come registro. È ciò che
 * --proofs fa sempre, dove il motore ha registrato i passaggi: chi chiede
 * le dimostrazioni scritte le vuole capire. Dove il motore non ha
 * registrato nulla, la voce resta quella di prima, carattere per carattere;
 * e così tutto il documento con --plain-proofs, che non fa registrare
 * nulla (main(), capitolo 23): qui non c'è un secondo modo di scrivere, c'è
 * la stessa funzione che riceve una spiegazione vuota.
 *
 * Il motore consegna dati: passi con un tipo e delle formule, e una tabella
 * di segni. Qui ogni tipo di passo ha la sua frase, sempre la stessa
 * (capitolo 41): nessuna frase è scritta per un esercizio, e nessun numero
 * viene calcolato. La lingua è quella del resto del documento.
 *
 * COME STA NEL PROGRAMMA
 *
 * È un volume dello stesso libro di main.c, che lo include dopo il suo
 * capitolo 20. Usa ciò che main.c ha già presentato: il verdetto (capitolo
 * 13), la riga che lo riassume, verdict_print() (capitolo 20), e la
 * spiegazione (capitolo 40). Di plot.c usa l'apertura del PDF e l'aggiunta
 * di una pagina. I capitoli hanno i numeri 41 e 22: la numerazione è una
 * sola per main.c, parser.c e output.c, e il 41, nato dopo, sta prima
 * perché il 22 lo usa.
 *
 * INDICE DEI CAPITOLI
 *
 *   41. La spiegazione scritta
 *   22. Il documento finale
 */

/* =============================================================================
 * 41. LA SPIEGAZIONE SCRITTA
 * ============================================================================
 *
 * Trasforma in testo la spiegazione che il motore ha registrato (Explanation,
 * capitolo 40): una frase per ogni tipo di passo, e la tabella dei segni
 * come tabella. Le formule sono quelle dei passi, copiate; di suo questo
 * capitolo mette le parole, la numerazione dei passi e la disposizione
 * (quante colonne di tabella per riga).
 *
 * È diviso in due funzioni perché in mezzo va la figura, che è del capitolo
 * 22: put_explanation_opening() dice che cosa si chiede (e per un'induzione
 * mostra subito i primi valori, in una tabellina), poi viene il grafico,
 * poi put_explanation_steps() fa il resto. Si parte dal problema, da un
 * esempio e dal disegno; il meccanismo viene dopo.
 *
 * Warning: le formule in evidenza usano \fitmath, definita nel preambolo
 * (document_open(), capitolo 22), e i segni \checkmark e \nexists di
 * amssymb, che il preambolo carica.
 */

/* Quante colonne della tabella dei segni si scrivono, al più, in una riga
 * del documento: di più, con intestazioni come -\frac{3}{2} < x < -1, non
 * starebbero nella pagina; una tabella più larga va a capo in più pezzi, e
 * \fitmath stringe quello che ancora non ci sta. */
#define TABLE_COLUMNS 6

/* Ritorna il sostegno da mettere in una casella con la formula text: una
 * riga invisibile, più alta se la formula ha una frazione, che tiene la
 * riga della tabella staccata da quelle accanto e dalle linee. */
static const char *table_strut(const char *text) {
    return strstr(text, "\\frac") ? "\\rule[-2.6ex]{0pt}{6.4ex}" : "\\rule[-1.4ex]{0pt}{4.4ex}";
}

/* Scrive su out l'apertura della spiegazione: che cosa chiede l'enunciato,
 * con la condizione in evidenza, e il piano del lavoro. */
static void put_explanation_opening(FILE *out, const Explanation *explanation) {
    const ExplainStep *ask = &explanation->step[0];
    int x = explanation->variable;
    fprintf(out, "\\textbf{Worked solution.}\n\n");
    if (explanation->request == EXPLAIN_INDUCTION) {
        /* Che cosa si afferma, e subito i primi casi, in una tabellina: i
         * passi EXPLAIN_VALUE, che per questo put_explanation_steps() salta. */
        fprintf(out, "\\emph{What is claimed.} That for every integer $%c \\ge %s$\n\\[ \\fitmath{%s} \\]\nBoth sides are numbers that depend on $%c$. "
                     "Before proving anything, compute them for the first values of $%c$:\n\\[ \\begin{array}{c|cc}\n"
                     "%c & \\text{left side} & \\text{right side} \\\\ \\hline\n", x, ask->text[1], ask->text[0], x, x, x);
        for (int i = 1; i < explanation->steps; i++) {
            const ExplainStep *step = &explanation->step[i];
            if (step->kind == EXPLAIN_VALUE) fprintf(out, "%s%s & %s & %s \\\\\n", table_strut(step->text[1]), step->text[0], step->text[1], step->text[2]);
        }
        fprintf(out, "\\end{array} \\]\nIn every row the two sides are the same number. A table cannot reach every $%c$: the proof does.\n\n", x);
        return;
    }
    if (explanation->request == EXPLAIN_DERIVATIVE) {
        fprintf(out, "\\emph{What is asked.} The derivative with respect to $%c$ of\n\\[ \\fitmath{%s} \\]\nthat is, the function that gives the slope of "
                     "its graph at each $%c$. The expression is built from smaller pieces, and each rule of differentiation gives the derivative "
                     "of a piece from the derivatives of the pieces inside it. So the plan is: start from the innermost pieces and work outwards, "
                     "one rule at a time.\n\n", x, ask->text[0], x);
        return;
    }
    if (explanation->request == EXPLAIN_SOLVE)
        fprintf(out, "\\emph{What is asked.} Find every $%c$ in $%s$ for which this condition is true:\n\\[ \\fitmath{%s} \\]\n", x, ask->text[1], ask->text[0]);
    else if (explanation->equivalence)
        fprintf(out, "\\emph{What is claimed.} That for every $%c$ in $%s$ the condition\n\\[ \\fitmath{%s} \\]\nis true exactly when this one is:\n"
                     "\\[ \\fitmath{%s} \\]\nIn other words, that the second line describes the solutions of the first.\n",
                x, ask->text[1], ask->text[0], ask->text[2]);
    else
        fprintf(out, "\\emph{What is claimed.} That this condition is true for %s $%c$ in $%s$:\n\\[ \\fitmath{%s} \\]\n",
                explanation->request == EXPLAIN_FORALL ? "every" : "at least one", x, ask->text[1], ask->text[0]);
    fprintf(out, "A comparison is true at a value of $%c$ only when both of its sides have a value there. Its truth can change only "
                 "where a side stops having a value or the two sides meet. So the plan is: find those points, then look at each "
                 "piece of the line between them.\n\n", x);
}

/* Scrive su out il titolo del prossimo passo, numerato: *number è il numero
 * dell'ultimo già scritto. */
static void put_step_title(FILE *out, int *number, const char *title) {
    fprintf(out, "\\emph{Step %d. %s}", ++*number, title);
}

/* Scrive su out la tabella dei segni della spiegazione: una colonna per
 * ogni punto e per ogni intervallo, una riga per ogni fattore, poi le righe
 * dei confronti. Una tabella con più di TABLE_COLUMNS colonne si scrive in
 * più pezzi di larghezza simile, uno sotto l'altro, ognuno con le sue
 * intestazioni. */
static void put_explanation_table(FILE *out, const Explanation *explanation) {
    static const char *const sign[] = {"-", "0", "+", "\\nexists"};
    int pieces = (explanation->columns + TABLE_COLUMNS - 1) / TABLE_COLUMNS, width = (explanation->columns + pieces - 1) / pieces;
    for (int first = 0; first < explanation->columns; first += width) {
        int last = first + width < explanation->columns ? first + width : explanation->columns;
        fprintf(out, "\\[ \\fitmath{\\begin{array}{l|");
        for (int column = first; column < last; column++) fputc('c', out);
        fprintf(out, "}\n");

        /* Le intestazioni: una tabella senza punti ha una colonna sola. */
        for (int column = first; column < last; column++) {
            if (explanation->head[column]) fprintf(out, " & %s\\displaystyle %s", table_strut(explanation->head[column]), explanation->head[column]);
            else fprintf(out, " & \\text{every } %c", explanation->variable);
        }
        fprintf(out, " \\\\ \\hline\n");

        /* Le righe. Una linea separa i fattori da ciò che se ne ricava. */
        for (int l = 0; l < explanation->lines; l++) {
            const ExplainLine *line = &explanation->line[l];
            int truth = line->kind != EXPLAIN_LINE_FACTOR && line->kind != EXPLAIN_LINE_VALUE;
            if (l > 0 && line->kind != EXPLAIN_LINE_FACTOR && explanation->line[l - 1].kind == EXPLAIN_LINE_FACTOR) fprintf(out, "\\hline\n");
            /* \displaystyle: in una casella le frazioni sarebbero piccole. */
            if (line->label) fprintf(out, "%s\\displaystyle %s", table_strut(line->label), line->label);
            else if (line->kind == EXPLAIN_LINE_CONDITION) fprintf(out, "\\text{%s}", explanation->equivalence ? "first condition" : "whole condition");
            else fprintf(out, "\\text{the two agree}");
            for (int column = first; column < last; column++)
                fprintf(out, " & %s", truth ? (line->cell[column] ? "\\checkmark" : "\\times") : sign[line->cell[column] + 1]);
            fprintf(out, l + 1 < explanation->lines ? " \\\\\n" : "\n");
        }
        fprintf(out, "\\end{array}} \\]\n");
    }
    fprintf(out, "{\\small $+$, $-$, $0$: the sign of that line's expression in that column; $\\nexists$: it has no value there; "
                 "$\\checkmark$: true; $\\times$: false.}\n\n");
}

/* La frase di ogni regola di derivazione (DerivativeRule, 27n), e per
 * RULE_FUNCTION quella di ogni funzione (Function): la regola generale,
 * scritta con u, v e c, mai con l'espressione dell'esercizio. Le formule
 * sono quelle di 27c-27f, in testa al capitolo 27 di main.c.
 *
 * Warning: se aggiungi una regola o una funzione a derive(), aggiungi qui la
 * sua frase. */
static const char *const derivative_rule[] = {
    [RULE_SUM] = "term by term: the derivative of a sum is the sum of the derivatives",
    [RULE_FACTOR] = "a constant factor stays in front: $(c\\,u)' = c\\,u'$",
    [RULE_PRODUCT] = "product rule: $(u\\,v)' = u'v + u\\,v'$",
    [RULE_QUOTIENT] = "quotient rule: $\\left(\\frac{u}{v}\\right)' = \\frac{u'v - u\\,v'}{v^{2}}$",
    [RULE_POWER] = "power rule: $(u^{c})' = c\\,u^{c-1}u'$",
    [RULE_EXPONENTIAL] = "exponential rule: $(c^{u})' = c^{u}\\ln c\\cdot u'$",
    [RULE_TOWER] = "written as $e^{v \\ln u}$: $(u^{v})' = u^{v}\\left(v'\\ln u + \\frac{v\\,u'}{u}\\right)$",
};
static const char *const function_rule[] = {
    [FN_FACT] = "", [FN_BINOM] = "",
    [FN_SIN] = "$(\\sin u)' = \\cos u\\cdot u'$",
    [FN_COS] = "$(\\cos u)' = -\\sin u\\cdot u'$",
    [FN_TAN] = "$(\\tan u)' = \\frac{u'}{\\cos^{2} u}$",
    [FN_COT] = "$(\\cot u)' = -\\frac{u'}{\\sin^{2} u}$",
    [FN_ARCTAN] = "$(\\arctan u)' = \\frac{u'}{1 + u^{2}}$",
    [FN_LN] = "$(\\ln u)' = \\frac{u'}{u}$",
    [FN_LOG] = "$\\log_{b} u = \\frac{\\ln u}{\\ln b}$, and $(\\ln u)' = \\frac{u'}{u}$",
    [FN_ROOT] = "$(\\sqrt[m]{u})' = \\frac{\\sqrt[m]{u}\\,u'}{m\\,u}$",
    [FN_ABS] = "$|u|' = \\frac{u'u}{|u|}$",
    [FN_ARCSIN] = "$(\\arcsin u)' = \\frac{u'}{\\sqrt{1 - u^{2}}}$",
    [FN_ARCCOS] = "$(\\arccos u)' = -\\frac{u'}{\\sqrt{1 - u^{2}}}$",
    [FN_ARCCOT] = "$(\\arccot u)' = -\\frac{u'}{1 + u^{2}}$",
};

/* La frase di ogni passaggio della catena di un passo induttivo
 * (ExplainRewrite, 40b). */
static const char *const rewrite_reason[] = {
    [REWRITE_SPLIT_SUM] = "split off the last term of the sum",
    [REWRITE_SPLIT_PRODUCT] = "split off the last factor of the product",
    [REWRITE_HYPOTHESIS] = "inductive hypothesis",
    [REWRITE_EXPAND] = "carry out the operations",
};

/* Scrive su out i passi della spiegazione dopo l'apertura: per ogni
 * confronto la catena delle sue forme, poi gli zeri dei fattori, la
 * tabella, i valori di prova e la risposta; per una derivata una riga per
 * regola, e la risposta; per un'induzione l'idea, il caso base, l'ipotesi,
 * il passo e la conclusione. Ogni tipo di passo ha qui la sua frase.
 *
 * I passi dello stesso tipo arrivano uno dopo l'altro, nell'ordine in cui
 * il motore li registra (explain_signs()): un gruppo si apre al primo e si
 * chiude quando il tipo cambia. */
static void put_explanation_steps(FILE *out, const Explanation *explanation) {
    int x = explanation->variable, number = 0;
    for (int i = 1; i < explanation->steps; i++) {
        const ExplainStep *step = &explanation->step[i], *next = i + 1 < explanation->steps ? step + 1 : NULL;
        int first = explanation->step[i - 1].kind != step->kind, last = !next || next->kind != step->kind;
        switch (step->kind) {
        case EXPLAIN_ASK:
            break;
        case EXPLAIN_COMPARISON: {
            /* Il campo di esistenza, poi la catena: le righe che seguono
             * sono i suoi passi, fino al prossimo confronto. */
            int chained = next && (next->kind == EXPLAIN_MOVE || next->kind == EXPLAIN_COMBINE || next->kind == EXPLAIN_FACTOR);
            if (step->group == 1) { put_step_title(out, &number, "One side, one fraction, factors."); fputc(' ', out); }
            if (explanation->comparisons > 1) fprintf(out, "\\emph{Comparison %d}, $%s$. ", step->group, step->text[0]);
            if (step->count) fprintf(out, "The comparison has no value for $%s$, where a denominator is zero: there it counts as false. ", step->text[1]);
            else fprintf(out, "Both sides have a value for every $%c$ of the set. ", x);
            if (!chained) {
                fprintf(out, "Nothing needs rewriting: one side is already $0$, and the other is already written with factors that cannot be split further.\n\n");
                break;
            }
            fprintf(out, "Where both sides have a value, each line says the same as the one above it:\n"
                         "\\[ \\fitmath{\\begin{aligned}\n & %s", step->text[0]);
            break;
        }
        case EXPLAIN_MOVE: case EXPLAIN_COMBINE: case EXPLAIN_FACTOR: {
            const char *why = step->kind == EXPLAIN_MOVE ? "subtract the right side from both sides"
                            : step->kind == EXPLAIN_COMBINE ? (step->flag ? "bring everything to one fraction, with a common denominator" : "carry out the operations and collect like terms")
                            : (step->flag ? "split numerator and denominator into factors" : "split into factors");
            fprintf(out, " \\\\\n \\iff\\quad & %s %s 0 && \\text{(%s)}", step->text[0], step->text[1], why);
            if (!next || (next->kind != EXPLAIN_MOVE && next->kind != EXPLAIN_COMBINE && next->kind != EXPLAIN_FACTOR)) fprintf(out, "\n\\end{aligned}} \\]\n\n");
            break;
        }
        case EXPLAIN_ZERO:
            if (first) {
                put_step_title(out, &number, "Where each factor is zero.");
                fprintf(out, " A product or a quotient can change sign only where one of its factors is zero.\n\\begin{itemize}\n");
            }
            if (step->count == 0) fprintf(out, "\\item $%s$ is never zero: it is %s for every $%c$.\n", step->text[0], step->flag > 0 ? "positive" : "negative", x);
            else if (step->count == 1) fprintf(out, "\\item $%s$ is zero only for $%c = %s$.\n", step->text[0], x, step->text[1]);
            else fprintf(out, "\\item $%s$ is zero only for $%c = %s$ and for $%c = %s$.\n", step->text[0], x, step->text[1], x, step->text[2]);
            if (last) fprintf(out, "\\end{itemize}\n\n");
            break;
        case EXPLAIN_POINTS:
            put_step_title(out, &number, "The sign table.");
            if (step->count > 1) fprintf(out, " In increasing order, the points where something can change are $%s$. Between two consecutive points no factor "
                                              "is zero, so no sign changes: one column for each point and one for each piece between them tell everything.\n", step->text[0]);
            else if (step->count) fprintf(out, " The only point where something can change is $%s$. On each side of it no factor is zero, so no sign changes: "
                                               "one column for the point and one for each side tell everything.\n", step->text[0]);
            else fprintf(out, " There is no point where something can change, so the table has a single column.\n");
            break;
        case EXPLAIN_TABLE:
            put_explanation_table(out, explanation);
            break;
        case EXPLAIN_TEST:
            if (first) {
                put_step_title(out, &number, "One value in each piece.");
                fprintf(out, " The table can be checked by hand: put a value of each piece in place of $%c$ and compute the two sides.\n\\begin{itemize}\n", x);
            }
            fprintf(out, "\\item $%c = %s$", x, step->text[0]);
            if (explanation->comparisons > 1) fprintf(out, ", comparison %d", step->group);
            fprintf(out, ": the left side is $%s$, the right side is $%s$, and $%s %s %s$ is %s.\n", step->text[1], step->text[2],
                    step->text[1], step->text[3], step->text[2], step->flag ? "true" : "false");
            if (last) fprintf(out, "\\end{itemize}\n\n");
            break;
        case EXPLAIN_DERIVE:
            if (first) {
                put_step_title(out, &number, "One rule at a time.");
                fprintf(out, " Each line is the derivative of one piece; a piece that contains others uses the lines above it.\n\\begin{itemize}\n");
            }
            fprintf(out, "\\item $\\displaystyle \\frac{d}{d%c}\\left(%s\\right) = %s$\\quad %s", x, step->text[0], step->text[1],
                    step->flag == RULE_FUNCTION ? function_rule[step->count] : derivative_rule[step->flag]);
            if (step->flag == RULE_POWER || step->flag == RULE_EXPONENTIAL || step->flag == RULE_FUNCTION)
                fprintf(out, step->group ? ", with $u$ the inner expression (chain rule)" : ", with $u = %c$ and $u' = 1$", x);
            fprintf(out, ".\n");
            if (last) fprintf(out, "\\end{itemize}\n\n");
            break;
        case EXPLAIN_VALUE:
            break;
        case EXPLAIN_BASE: {
            /* Prima l'idea, che è di ogni induzione; poi il caso base, che è
             * la prima riga della tabella. Un membro che è già un numero non
             * ha bisogno di "= valore". */
            const char *base = explanation->step[0].text[1];
            put_step_title(out, &number, "The idea: induction.");
            fprintf(out, " Two facts are enough. The claim is true for the first value, $%c = %s$: the base case. And whenever it is true for some "
                         "$%c$, it is true for $%c + 1$ as well: the inductive step. Then it is true for $%s$, hence for the value after it, hence "
                         "for the next one, and so on: for every $%c \\ge %s$.\n\n", x, base, x, x, base, x, base);
            put_step_title(out, &number, "Base case.");
            fprintf(out, " For $%c = %s$", x, base);
            for (int s = 0; s < 2; s++) {
                fprintf(out, s ? " and the right side is $" : " the left side is $");
                if (strcmp(step->text[s], step->text[2 + s]) != 0) fprintf(out, "%s = ", step->text[s]);
                fprintf(out, "%s$", step->text[2 + s]);
            }
            fprintf(out, ": the first row of the table.\n\n");
            break;
        }
        case EXPLAIN_HYPOTHESIS:
            put_step_title(out, &number, "Inductive hypothesis.");
            fprintf(out, " Suppose the claim is true for some $%c \\ge %s$:\n\\[ \\fitmath{%s} \\]\n", x, explanation->step[0].text[1], step->text[0]);
            break;
        case EXPLAIN_TARGET:
            if (step->group == 1) {
                put_step_title(out, &number, "Inductive step.");
                fprintf(out, " Compute the two sides of the claim for $%c + 1$; the hypothesis is used where it says so.\n\n", x);
            }
            fprintf(out, "%s side:\n\\[ \\fitmath{\\begin{aligned}\n & %s", step->group == 1 ? "Left" : "Right", step->text[0]);
            break;
        case EXPLAIN_REWRITE:
            fprintf(out, " \\\\\n ={} & %s && \\text{(%s)}", step->text[0], rewrite_reason[step->flag]);
            if (last) fprintf(out, "\n\\end{aligned}} \\]\n");
            if (last && next && next->kind != EXPLAIN_TARGET) fprintf(out, "Both sides become the same expression, so the claim is true for $%c + 1$.\n\n", x);
            break;
        case EXPLAIN_ANSWER:
            put_step_title(out, &number, explanation->request == EXPLAIN_INDUCTION ? "Conclusion." : "The answer.");
            if (explanation->request == EXPLAIN_INDUCTION)
                fprintf(out, " %s the claim for every integer $%c \\ge %s$.\n\n", step->count ? "The base case and the inductive step give"
                        : "The inductive step is carried out in the proof on record below. With the base case, it gives", x, explanation->step[0].text[1]);
            else if (explanation->request == EXPLAIN_DERIVATIVE)
                fprintf(out, " The last line is the derivative that was asked:\n\\[ \\fitmath{\\frac{d}{d%c}\\left(%s\\right) = %s} \\]\n", x, explanation->step[0].text[0], step->text[0]);
            else if (explanation->request == EXPLAIN_SOLVE && step->count == 0)
                fprintf(out, " The condition is true exactly in the columns marked $\\checkmark$ on the last line of the table:\n\\[ \\fitmath{%s} \\]\n", step->text[0]);
            else if (explanation->request == EXPLAIN_SOLVE)
                fprintf(out, " The last line of the table is $%s$ in every column: %s $%c$ of the set satisfies the condition.\n\n",
                        step->count == 1 ? "\\checkmark" : "\\times", step->count == 1 ? "every" : "no", x);
            else if (!step->count)
                fprintf(out, " The last line of the table is $%s$ in every column: the statement is %s.\n\n", step->flag ? "\\checkmark" : "\\times", step->flag ? "true" : "false");
            else {
                fprintf(out, " The last line of the table is $%s$ ", step->flag ? "\\checkmark" : "\\times");
                if (step->text[0]) fprintf(out, "in the column $%s$", step->text[0]); else fprintf(out, "in its only column");
                fprintf(out, ": %s, so the statement is %s.\n\n", step->flag ? "a value with the property exists" : "there the claim fails", step->flag ? "true" : "false");
            }
            break;
        }
    }
}

/* =============================================================================
 * 22. IL DOCUMENTO FINALE
 * ============================================================================
 *
 * L'ordine delle chiamate: document_open(), poi per ogni file di enunciati
 * document_section(), document_list() che apre l'elenco, un document_item()
 * per enunciato, document_list() che lo chiude; alla fine document_close().
 * Una voce con la spiegazione (capitolo 41) la scrive document_item() intorno
 * al grafico: l'apertura, il grafico, i passi, la dimostrazione.
 */

/* Il documento finale in corso di scrittura, con il PDF dei suoi grafici. */
typedef struct {
    FILE *tex;              /* il documento LaTeX, aperto in scrittura */
    char tex_path[1024];    /* il suo percorso */
    int want_pdf;           /* 1 se alla fine va compilato in PDF */
    PlPdf *plots;           /* il PDF dei grafici; NULL finché non ce n'è uno */
    char plots_path[1040];  /* il suo percorso: quello del documento, con
                               "-plots.pdf" al posto dell'estensione */
    int plots_pages;        /* pagine scritte finora nel PDF dei grafici */
    int plots_failed;       /* 1 se un grafico non si è potuto scrivere */
} Document;

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

/* Oltre quanti caratteri di sorgente una formula in linea di una
 * dimostrazione va in evidenza (put_proof()). È una stima della larghezza:
 * una formula più corta sta quasi sempre in una riga, una più lunga quasi
 * mai, e in evidenza \fitmath la stringe se ancora non ci sta. */
#define LONG_INLINE_FORMULA 120

/* Scrive su out il testo LaTeX di una formula, già in modo matematico, in
 * evidenza e stretta alla larghezza della riga se serve (vedi \fitmath nel
 * preambolo, document_open()). Il segno di punteggiatura che la seguiva nel
 * testo, se c'è (punctuation diverso da 0), va dentro la formula, come nei
 * libri: altrimenti comincerebbe la riga dopo. */
static void put_display(FILE *out, const char *formula, size_t length, char punctuation) {
    /* La punteggiatura sta dentro la scatola di \fitmath: fuori, una formula
     * già stretta alla riga la supererebbe proprio di quel segno. */
    fprintf(out, "\n\\[ \\fitmath{%.*s", (int)length, formula);
    if (punctuation) fprintf(out, "\\,%c", punctuation);
    fprintf(out, "} \\]\n");
}

/* Ritorna 1 se la formula, di length caratteri di sorgente, è troppo larga
 * per stare in linea: più lunga di LONG_INLINE_FORMULA, oppure non corta e
 * con un numero di almeno 10 cifre, come gli estremi decimali degli
 * intervalli dei controlli con i numeri, che LaTeX non sa spezzare. */
static int long_formula(const char *formula, size_t length) {
    if (length > LONG_INLINE_FORMULA) return 1;
    int run = 0;
    for (size_t i = 0; i < length && length > LONG_INLINE_FORMULA / 2; i++) {
        run = isdigit((unsigned char)formula[i]) ? run + 1 : 0;
        if (run >= 10) return 1;
    }
    return 0;
}

/* Scrive su out la dimostrazione proof, così com'è tranne la disposizione
 * delle formule. Le derivate scritte dai capitoli 27 e 30 e gli sviluppi del
 * capitolo 29 sono formule lunghe, che in linea uscirebbero dal margine:
 * dentro \left( ... \right) e \frac LaTeX non sa andare a capo. Una formula
 * in linea, $...$, troppo larga (long_formula()) va quindi in
 * evidenza, e ogni formula in evidenza, \[ ... \], passa da \fitmath, che la
 * rimpicciolisce solo se è più larga della riga. Il contenuto delle formule
 * non cambia: qui si decide solo dove stanno.
 *
 * Un dollaro preceduto da una barra (\$) non apre una formula. Una formula
 * non chiusa, che il motore non scrive mai, viene copiata com'è. */
static void put_proof(FILE *out, const char *proof) {
    const char *p = proof;
    while (*p) {
        const char *open = NULL, *close = NULL;
        int display = 0;
        /* \[ apre una formula in evidenza; \\[2pt], un a capo con uno spazio
         * dentro un allineamento, no. */
        if (p[0] == '\\' && p[1] == '[' && (p == proof || p[-1] != '\\')) {
            open = p + 2;
            close = strstr(open, "\\]");
            display = 1;
        } else if (p[0] == '$' && (p == proof || p[-1] != '\\') && p[1] != '$') {
            open = p + 1;
            for (close = open; *close && !(*close == '$' && close[-1] != '\\'); close++) {}
            if (!*close) close = NULL;
        }
        if (!open || !close) {
            /* Testo, o una barra che non apre una formula. */
            fputc(*p++, out);
            if (open && !close) { fputs(p, out); return; }
            continue;
        }
        size_t length = (size_t)(close - open);
        p = close + (display ? 2 : 1);
        if (display || long_formula(open, length)) {
            char punctuation = strchr(",.;:", *p) && *p ? *p : 0;
            put_display(out, open, length, punctuation);
            if (punctuation) p++;
        } else {
            fprintf(out, "$%.*s$", (int)length, open);
        }
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

/* Compila in PDF il documento LaTeX tex_path, mettendo il PDF nella stessa
 * cartella. Ritorna 1 se riesce.
 *
 * È l'unico punto in cui il programma si appoggia a qualcosa di esterno: un
 * compilatore LaTeX già installato, chiamato con system(). Si provano in
 * ordine i comandi di compilers[]; il documento .tex resta comunque, e chi
 * non ha un compilatore lo può usare altrove.
 *
 * Il compilatore lavora dentro la cartella del documento. Così il PDF dei
 * grafici, che il documento nomina senza cartella, viene trovato da tutti e
 * due i compilatori: uno cerca i file accanto al documento, l'altro nella
 * cartella da cui è stato lanciato. */
static int compile_pdf(const char *tex_path) {
    static const char *const compilers[] = {
        "cd '%s' && tectonic '%s' > /dev/null 2>&1",
        "cd '%s' && pdflatex -interaction=batchmode -halt-on-error '%s' > /dev/null 2>&1",
    };
    char directory[1024], command[4096];

    /* I percorsi vanno tra apici nel comando: uno che ne contiene non si può
     * passare in sicurezza. */
    if (strchr(tex_path, '\'') || strlen(tex_path) >= sizeof directory) return 0;
    strcpy(directory, tex_path);
    char *slash = strrchr(directory, '/');
    const char *name = slash ? tex_path + (slash - directory) + 1 : tex_path;
    if (slash) *slash = 0; else strcpy(directory, ".");

    for (size_t i = 0; i < sizeof compilers / sizeof *compilers; i++) {
        snprintf(command, sizeof command, compilers[i], directory, name);
        if (system(command) == 0) return 1;
    }
    return 0;
}

/* Comincia il documento *document nel file path e ne scrive il preambolo,
 * che è quello dei file in tests/ più il pacchetto per includere i grafici.
 * Con un nome che finisce in .pdf scrive il documento LaTeX con lo stesso
 * nome e l'estensione .tex, e document_close() poi lo compila. Ritorna NULL
 * se riesce, altrimenti il motivo, in italiano. */
static const char *document_open(Document *document, const char *path) {
    memset(document, 0, sizeof *document);
    size_t length = strlen(path);
    if (length < 5 || length >= sizeof document->tex_path) return "nome del documento non valido";
    strcpy(document->tex_path, path);
    document->want_pdf = strcmp(path + length - 4, ".pdf") == 0;
    if (document->want_pdf) strcpy(document->tex_path + length - 4, ".tex");
    strcpy(document->plots_path, document->tex_path);
    strcpy(document->plots_path + length - 4, "-plots.pdf");
    document->tex = fopen(document->tex_path, "w");
    if (!document->tex) return "impossibile scrivere il documento";
    fprintf(document->tex, "\\documentclass[11pt,a4paper]{article}\n\\usepackage[T1]{fontenc}\n"
                           "\\usepackage[utf8]{inputenc}\n\\usepackage[english]{babel}\n"
                           "\\usepackage{amsmath,amssymb}\n\\usepackage{graphicx}\n\\usepackage[margin=2cm]{geometry}\n"
                           "\\setlength{\\parindent}{0pt}\n\\setlength{\\emergencystretch}{3em}\n"
                           "\\allowdisplaybreaks\n\n"
                           /* \arccot non è un comando di LaTeX né di amsmath, ma gli
                            * enunciati e le derivate lo scrivono (capitolo 27): senza
                            * questa riga un documento che lo contiene non si compila. */
                           "\\DeclareMathOperator{\\arccot}{arccot}\n"
                           /* \fitmath{f}: la formula f in una scatola, rimpicciolita
                            * alla larghezza della riga solo se ne è più larga
                            * (put_proof()). Usa solo graphicx, già caricato. */
                           "\\newsavebox{\\fitbox}\n"
                           "\\newcommand{\\fitmath}[1]{\\sbox{\\fitbox}{$\\displaystyle #1$}%%\n"
                           "  \\ifdim\\wd\\fitbox>\\linewidth\\resizebox{\\linewidth}{!}{\\usebox{\\fitbox}}\\else\\usebox{\\fitbox}\\fi}\n\n"
                           "\\title{Proofs}\n\\author{}\n\\date{}\n"
                           "\\begin{document}\n\\maketitle\n");
    return NULL;
}

/* Scrive il titolo di una parte del documento: il nome del file di enunciati
 * che sta per essere letto. */
static void document_section(Document *document, const char *title) {
    fprintf(document->tex, "\n\\section*{");

    /* Il titolo è un percorso, una parola sola per LaTeX: dopo ogni / e ogni
     * _ gli si permette di andare a capo, o uscirebbe dal margine. */
    for (const char *c = title; *c; c++) {
        char one[2] = {*c, 0};
        put_escaped(document->tex, one);
        if (*c == '/' || *c == '_') fputs("\\allowbreak{}", document->tex);
    }
    fprintf(document->tex, "}\n");
}

/* Apre (opening = 1) o chiude (opening = 0) l'elenco degli enunciati di un
 * file. */
static void document_list(Document *document, int opening) {
    fprintf(document->tex, opening ? "\\begin{enumerate}\n" : "\\end{enumerate}\n");
}

/* Scrive nel documento il grafico plot, con due righe che dicono come
 * leggerlo: quello di una tabella dei segni (18q), oppure, con values
 * diverso da zero, quello dei primi valori di un enunciato per induzione
 * (40b).
 *
 * Il grafico diventa una pagina del PDF dei grafici, e il documento la
 * include per numero di pagina. Se il PDF non si può scrivere il documento
 * resta senza quel grafico, e document_close() lo segnala. */
static void document_figure(Document *document, const PlFigure *plot, int values) {
    /* La pagina del grafico, e la sua inclusione. Il nome è senza cartella:
     * i due file stanno insieme (vedi compile_pdf()). */
    if (!document->plots) document->plots = pl_pdf_open(document->plots_path);
    if (!document->plots || !pl_pdf_savefig(document->plots, plot)) { document->plots_failed = 1; return; }
    const char *slash = strrchr(document->plots_path, '/');
    fprintf(document->tex, "\\begin{center}\n\\includegraphics[page=%d,width=0.8\\textwidth]{%s}\n\\end{center}\n",
            ++document->plots_pages, slash ? slash + 1 : document->plots_path);
    if (values)
        fprintf(document->tex, "{\\small \\textbf{Graph.} The rows of the table: a dot for each value of the left side, and a line that joins the values "
                               "of the right side. Every dot lies on the line, because the two sides are the same number.}\n\n");
    else
        fprintf(document->tex, "{\\small \\textbf{Graph.} Each panel shows the two sides of one comparison, or one side and the axis when the other is $0$. "
                               "Green bands: where that comparison holds. Gray bands and dashed lines: where a side has no value. "
                               "Bottom line: where the whole condition holds.}\n\n");
}

/* Scrive una voce dell'elenco: l'enunciato com'è scritto nel file (formula,
 * o NULL se l'\item non ha una formula leggibile), poi la dimostrazione del
 * verdetto, o in sua mancanza il verdetto stesso, e infine il grafico plot,
 * se non è NULL.
 *
 * Se il motore ha registrato una spiegazione dell'enunciato (explanation
 * con dei passi, e un verdetto che ha la sua dimostrazione) la voce comincia
 * da lì: che cosa si chiede, il grafico, i passi; la dimostrazione viene in
 * fondo, in piccolo, come registro di ciò che i passi hanno spiegato. Senza
 * spiegazione la voce è quella di sempre. */
static void document_item(Document *document, const char *formula, const Verdict *verdict, const PlFigure *plot, const Explanation *explanation) {
    FILE *tex = document->tex;
    int explained = explanation && explanation->steps > 0 && verdict->proof;
    fprintf(tex, "\\item ");

    /* La formula letta da "$\displaystyle ...$" comincia già con quel
     * comando: non lo si scrive due volte. Una troppo larga per la riga va in
     * evidenza, stretta se serve, come nelle dimostrazioni (put_proof()). */
    if (formula && long_formula(formula, strlen(formula))) put_display(tex, formula, strlen(formula), 0);
    else if (formula) fprintf(tex, strncmp(formula, "\\displaystyle", 13) == 0 ? "$%s$\n\n" : "$\\displaystyle %s$\n\n", formula);
    if (explained) {
        put_explanation_opening(tex, explanation);
    } else if (verdict->proof) {
        put_proof(tex, verdict->proof);
        fprintf(tex, "\n\n");
    } else {
        fprintf(tex, "\\textbf{No proof.} ");
        put_plain_text(tex, verdict);
        fprintf(tex, "\n\n");
    }
    if (plot) document_figure(document, plot, explained && explanation->request == EXPLAIN_INDUCTION);
    if (!explained) return;

    /* I passi, e poi la dimostrazione: il \par chiude l'ultimo capoverso
     * prima che finisca il carattere piccolo, che altrimenti non varrebbe
     * per la sua interlinea. */
    put_explanation_steps(tex, explanation);
    fprintf(tex, "{\\small \\textbf{The proof on record.} ");
    put_proof(tex, verdict->proof);
    fprintf(tex, "\\par}\n\n");
}

/* Finisce il documento: chiude il file LaTeX e il PDF dei grafici e, se era
 * stato chiesto un PDF, lo fa compilare. Ritorna NULL se tutto è riuscito,
 * altrimenti che cosa non è riuscito, in italiano: il testo vale fino alla
 * chiamata successiva. */
static const char *document_close(Document *document) {
    static char problem[1200];
    problem[0] = '\0';
    fprintf(document->tex, "\\end{document}\n");
    if (fclose(document->tex) != 0) snprintf(problem, sizeof problem, "il documento %s non è stato scritto per intero", document->tex_path);
    if ((document->plots && !pl_pdf_close(document->plots)) || document->plots_failed)
        snprintf(problem, sizeof problem, "il PDF dei grafici %s non è stato scritto per intero", document->plots_path);
    else if (!problem[0] && document->want_pdf && !compile_pdf(document->tex_path))
        snprintf(problem, sizeof problem, "nessun compilatore LaTeX ha prodotto il PDF; resta il documento %s", document->tex_path);
    return problem[0] ? problem : NULL;
}
