/* Test del motore: parsing, valutazione, induzione, insiemi, quantificatori,
 * lettura dei file .tex.
 *
 * Verifica il comportamento generale, non un elenco di esercizi: ogni caso è
 * un enunciato piccolo scelto per esercitare una regola (una convenzione del
 * parser, un tipo di atomo, una cautela del passo induttivo, una regola sui
 * quantificatori) e dichiara quale esito ci si aspetta.
 *
 * I casi più importanti sono quelli che NON devono risultare dimostrati: un
 * dimostratore che approva un enunciato falso è peggio di uno che non risponde.
 *
 * Compilazione ed esecuzione, dalla cartella principale del repository:
 *
 *     cc -std=c11 -O2 -Wall -Wextra -o /tmp/test_engine \
 *         tests/engine/test_engine.c && /tmp/test_engine
 */

/* Il programma è un solo file con il proprio main(): lo si include cambiando
 * nome a quel main, così il test può chiamarne le funzioni interne. */
#define main mathresearcher_main
#include "../../main.c"
#undef main

/* Un caso di prova: un enunciato e l'esito atteso da prove(). */
typedef struct {
    const char *name;     /* descrizione del caso, in inglese */
    const char *formula;  /* enunciato in LaTeX */
    Outcome expected;
} Case;

static const Case cases[] = {
    /* Enunciati veri che il metodo deve chiudere, uno per tipo di atomo. */
    {"arithmetic series", "\\sum_{k=1}^{n} k = \\frac{n\\left(n + 1\\right)}{2}", PROVED},
    {"geometric series from k = 0", "\\sum_{k=0}^{n} 3^{k} = \\frac{3^{n + 1} - 1}{2}", PROVED},
    {"geometric series with a symbolic ratio", "\\sum_{k=1}^{n} a^{k} = \\frac{a^{n + 1} - a}{a - 1}", PROVED},
    {"alternating sum uses (-1)^n", "\\sum_{k=1}^{n} \\left(-1\\right)^{k} k = \\frac{\\left(-1\\right)^{n}\\left(2n + 1\\right) - 1}{4}", PROVED},
    {"factorial telescoping sum", "\\sum_{k=1}^{n} k k! = \\left(n + 1\\right)! - 1", PROVED},
    {"binomial with a constant lower index", "\\sum_{k=1}^{n} \\binom{k}{2} = \\binom{n + 1}{3}", PROVED},
    {"binomial with a symbolic lower index", "\\prod_{k=1}^{n} \\frac{4k - 2}{k} = \\binom{2n}{n}", PROVED},
    {"Fibonacci recurrence", "\\sum_{k=1}^{n} F_{k} = F_{n + 2} - 1", PROVED},
    {"Fibonacci with a dilated index", "\\sum_{k=1}^{n} F_{2k} = F_{2n + 1} - 1", PROVED},
    {"harmonic numbers are a library sum", "\\sum_{k=1}^{n} \\frac{1}{k} = H_{n}", PROVED},
    {"telescoping product", "\\prod_{k=1}^{n} \\left(1 + \\frac{1}{k}\\right) = n + 1", PROVED},
    {"upper bound 2n adds two terms per step", "\\sum_{k=1}^{2n} \\left(-1\\right)^{k} k = n", PROVED},
    {"two sums in one statement", "\\sum_{k=1}^{n} \\cos^{2}\\left(k x\\right) + \\sum_{k=1}^{n} \\sin^{2}\\left(k x\\right) = n", PROVED},
    {"identity with no sum at all", "\\left(n + 1\\right)^{2} = n^{2} + 2n + 1", PROVED},
    {"Pythagorean identity follows from exponentials", "\\sin^{2}\\left(n x\\right) + \\cos^{2}\\left(n x\\right) = 1", PROVED},
    {"sine of pi over two is exact", "\\sin\\left(\\frac{\\pi}{2}\\right) = 1", PROVED},
    {"half angles", "\\sum_{k=1}^{n} \\frac{\\tan\\left(\\frac{x}{2^{k}}\\right)}{2^{k}} = \\frac{\\cot\\left(\\frac{x}{2^{n}}\\right)}{2^{n}} - \\cot x", PROVED},
    {"square root of a constant", "\\sqrt{12} = 2\\sqrt{3}", PROVED},
    {"square root of a product splits", "\\sqrt{n\\left(n + 1\\right)} = \\sqrt{n}\\,\\sqrt{n + 1}", PROVED},
    {"perfect cube under a cube root", "\\sqrt[3]{n^{3} + 3n^{2} + 3n + 1} = n + 1", PROVED},
    {"telescoping square roots", "\\sum_{k=1}^{n} \\frac{1}{\\sqrt{k} + \\sqrt{k + 1}} = \\sqrt{n + 1} - 1", PROVED},
    {"logarithm of a power", "\\ln\\left(n^{2}\\right) = 2\\ln n", PROVED},
    {"change of logarithm base", "\\log_{2}\\left(4^{n}\\right) = 2n", PROVED},
    {"telescoping logarithms", "\\sum_{k=1}^{n} \\ln\\left(1 + \\frac{1}{k}\\right) = \\ln\\left(n + 1\\right)", PROVED},
    {"logarithm of a factorial", "\\sum_{k=1}^{n} \\ln k = \\ln\\left(n!\\right)", PROVED},
    {"logarithms that only cancel after exponentiating", "\\ln\\left(1 - a\\right) + \\ln\\left(1 + a\\right) = \\ln\\left(1 - a^{2}\\right)", PROVED},
    {"arctangent of one is exact", "\\arctan 1 = \\frac{\\pi}{4}", PROVED},
    {"arctangent is odd", "\\arctan\\left(-n\\right) = -\\arctan n", PROVED},
    {"two arctangents adding up to a quarter of pi", "\\arctan\\left(\\frac{1}{2}\\right) + \\arctan\\left(\\frac{1}{3}\\right) = \\frac{\\pi}{4}", PROVED},
    {"arctangents on the correct branch", "\\arctan 2 + \\arctan 3 = \\frac{3\\pi}{4}", PROVED},
    {"telescoping arctangents", "\\sum_{k=1}^{n} \\arctan\\left(\\frac{1}{k^{2} + k + 1}\\right) = \\arctan\\left(n + 1\\right) - \\frac{\\pi}{4}", PROVED},
    {"arctangents with a real parameter", "\\sum_{k=1}^{n} \\arctan\\left(\\frac{x}{1 + k\\left(k + 1\\right)x^{2}}\\right) = \\arctan\\left(\\left(n + 1\\right)x\\right) - \\arctan x", PROVED},

    /* Definito per ogni n: i due membri sono uguali finché esistono, e i
     * primi undici n non mostrano nulla. Il verdetto dipende solo dal fatto
     * che ogni divisione, logaritmo e radice abbia senso per tutti gli n. */
    {"a quotient undefined at a later n", "\\frac{n - 50}{n - 50} = 1", NOT_PROVED},
    {"a quotient undefined just after the values computed with numbers", "\\frac{n - 12}{n - 12} = 1", NOT_PROVED},
    {"a sum with a term undefined at a later index", "\\sum_{k=1}^{n} \\frac{1}{k - 50} - \\sum_{k=1}^{n} \\frac{1}{k - 50} = 0", NOT_PROVED},
    {"a telescoping sum whose terms stop being defined", "\\sum_{k=1}^{n} \\frac{1}{\\left(k - 100\\right)\\left(k - 99\\right)} = -\\frac{1}{99} - \\frac{1}{n - 99}", NOT_PROVED},
    {"a logarithm undefined from a later n", "\\ln\\left(60 - n\\right) = \\ln\\left(60 - n\\right)", NOT_PROVED},
    {"a square root undefined from a later n", "\\sqrt{100 - n} \\cdot \\sqrt{100 - n} = 100 - n", NOT_PROVED},
    {"a power of two that reaches a zero of the denominator", "\\frac{2^{n} - 4096}{2^{n} - 4096} = 1", NOT_PROVED},
    {"a division made inside the proof by a factor that vanishes later",
     "\\frac{\\prod_{k=1}^{n} \\left(k - 50\\right)}{\\prod_{k=1}^{n - 1} \\left(k - 50\\right)} = n - 50", NOT_PROVED},
    {"a denominator the program cannot follow for every n", "\\frac{n! - 5}{n! - 5} = 1", NOT_PROVED},
    {"a denominator with a power of two and n, never zero", "\\frac{2^{n} - n}{2^{n} - n} = 1", PROVED},
    {"a denominator with two powers that is zero at a later n", "\\frac{3^{n} - 2^{n} - 527345}{3^{n} - 2^{n} - 527345} = 1", NOT_PROVED},
    {"a denominator with a power and n that is zero at a later n", "\\frac{2^{n} - 4096 n}{2^{n} - 4096 n} = 1", NOT_PROVED},
    {"a denominator negative at first and then always positive", "\\frac{2^{n} - 100 n - 1000}{2^{n} - 100 n - 1000} = 1", PROVED},
    {"a logarithm of a cosine that is negative", "\\ln\\left(\\cos 2\\right) = \\ln\\left(\\cos 2\\right)", NOT_PROVED},
    {"a logarithm of a sine that is negative", "\\sum_{k=1}^{n} \\ln\\left(\\sin 4\\right) = n \\ln\\left(\\sin 4\\right)", NOT_PROVED},
    {"a logarithm of a cosine that stays positive", "\\sum_{k=1}^{n} \\ln\\left(\\cos\\left(\\frac{x}{2^{k}}\\right)\\right) = "
     "\\ln\\left(\\frac{\\sin x}{2^{n}\\,\\sin\\left(\\frac{x}{2^{n}}\\right)}\\right)", PROVED},
    {"a sine at a fraction of pi that is never zero",
     "\\prod_{k=1}^{n} \\cos\\left(\\frac{\\pi}{2^{k + 1}}\\right) = \\frac{1}{2^{n}\\,\\sin\\left(\\frac{\\pi}{2^{n + 1}}\\right)}", PROVED},
    {"a logarithm whose argument is a product of tangents",
     "\\sum_{k=1}^{n} \\ln\\left(1 - \\tan^{2}\\left(\\frac{x}{2^{k}}\\right)\\right) = "
     "\\ln\\left(\\frac{2^{n}\\,\\tan\\left(\\frac{x}{2^{n}}\\right)}{\\tan x}\\right)", PROVED},
    {"a logarithm of a sine whose sign changes with n", "\\ln\\left(\\sin n\\right) = \\ln\\left(\\sin n\\right)", NOT_PROVED},
    {"a computation that stops must not count as a confirmation", "x^{100} + \\ln\\left(-x\\right) = x^{100} + \\ln\\left(x\\right)", NOT_PROVED},
    {"a divisor that is zero by an identity the zero test knows",
     "\\frac{1}{\\arctan\\left(\\frac{1}{2}\\right) + \\arctan\\left(\\frac{1}{3}\\right) - \\frac{\\pi}{4}} = "
     "\\frac{1}{\\arctan\\left(\\frac{1}{2}\\right) + \\arctan\\left(\\frac{1}{3}\\right) - \\frac{\\pi}{4}}", NOT_PROVED},
    {"a divisor that is zero by an identity the algebra does not see",
     "\\frac{1}{2\\cos\\left(\\frac{\\pi}{3}\\right) - 1} = \\frac{1}{2\\cos\\left(\\frac{\\pi}{3}\\right) - 1}", NOT_PROVED},
    {"a sum divided by a hidden zero",
     "\\sum_{k=1}^{n} \\frac{k}{2\\cos\\left(\\frac{\\pi}{3}\\right) - 1} = \\frac{n\\left(n + 1\\right)}{2\\left(2\\cos\\left(\\frac{\\pi}{3}\\right) - 1\\right)}", NOT_PROVED},
    {"a divisor with square roots that is never zero", "\\frac{n}{\\sqrt{2} + \\sqrt{3}} = \\frac{n}{\\sqrt{2} + \\sqrt{3}}", PROVED},
    {"a divisor with logarithms of primes that is never zero", "\\frac{n}{\\ln 2 + \\ln 3} = \\frac{n}{\\ln 6}", PROVED},
    {"a divisor with a sine at a rational angle", "\\frac{n}{\\sin\\left(1\\right)} = \\frac{n}{\\sin\\left(1\\right)}", PROVED},
    {"a denominator with no integer zero", "\\frac{2n - 41}{2n - 41} = 1", PROVED},
    {"a denominator whose zeros come before the starting point", "\\frac{n + 3}{n + 3} = 1", PROVED},
    {"a denominator in powers of two that is never zero",
     "\\sum_{k=1}^{n} \\frac{2^{k}}{\\left(2^{k} - 1\\right)\\left(2^{k + 1} - 1\\right)} = 1 - \\frac{1}{2^{n + 1} - 1}", PROVED},
    {"a denominator with a parameter", "\\frac{1}{n - a} = \\frac{1}{n - a}", PROVED},
    {"a tangent at a rational angle is defined for every n", "\\tan\\left(n x\\right) = \\frac{\\sin\\left(n x\\right)}{\\cos\\left(n x\\right)}", PROVED},
    {"a logarithm in a base that is never one", "\\sum_{k=2}^{n} \\frac{1}{\\log_{k} a} = \\log_{a}\\left(n!\\right)", PROVED},

    /* I valori dei parametri: l'enunciato è definito solo per valori
     * particolari, che i numeri semplici non incontrano. */
    {"a statement defined only for large values of a parameter", "\\sum_{k=1}^{n} \\sqrt{x - 5000} = n \\sqrt{x - 5000}", PROVED},
    {"a logarithm defined only for large values of a parameter", "\\sum_{k=1}^{n} \\ln\\left(x - 123456\\right) = n \\ln\\left(x - 123456\\right)", PROVED},
    {"a parameter confined between two roots",
     "\\sum_{k=1}^{n} \\sqrt{\\left(x - 1414\\right)\\left(1415 - x\\right)} = n \\sqrt{\\left(x - 1414\\right)\\left(1415 - x\\right)}", PROVED},
    {"two parameters, each with its own condition",
     "\\sum_{k=1}^{n} \\left(\\sqrt{x - 5000} + \\sqrt{y - 7000}\\right) = n \\sqrt{x - 5000} + n \\sqrt{y - 7000}", PROVED},
    {"a condition that ties two parameters", "\\sum_{k=1}^{n} \\sqrt{y - x - 9000} = n \\sqrt{y - x - 9000}", PROVED},
    {"conditions that no value of the parameter satisfies",
     "\\sum_{k=1}^{n} \\sqrt{x - 5000} \\sqrt{4000 - x} = n \\sqrt{x - 5000} \\sqrt{4000 - x}", NOT_PROVED},
    {"a domain that shrinks as n grows: every large value works", "\\sum_{k=1}^{n} \\ln\\left(x - k\\right) = \\ln\\left(\\prod_{k=1}^{n} \\left(x - k\\right)\\right)", PROVED},
    {"a domain that needs very negative values", "\\sum_{k=1}^{n} \\ln\\left(k - x\\right) = \\ln\\left(\\prod_{k=1}^{n} \\left(k - x\\right)\\right)", PROVED},
    {"a domain that shrinks, with two parameters", "\\sum_{k=1}^{n} \\ln\\left(x - y - k\\right) = \\ln\\left(\\prod_{k=1}^{n} \\left(x - y - k\\right)\\right)", PROVED},
    {"two conditions that no value satisfies together",
     "\\ln\\left(x - n\\right) + \\ln\\left(n - x\\right) = \\ln\\left(n - x\\right) + \\ln\\left(x - n\\right)", NOT_PROVED},
    {"a radicand that is never positive",
     "\\sum_{k=1}^{n} \\sqrt{\\left(x - k\\right)\\left(k - x\\right)} = \\sum_{k=1}^{n} \\sqrt{\\left(x - k\\right)\\left(k - x\\right)}", NOT_PROVED},
    {"a parameter under a root with a bound on the root", "\\sum_{k=1}^{n} \\ln\\left(\\sqrt{x} - 100\\right) = n \\ln\\left(\\sqrt{x} - 100\\right)", PROVED},
    {"a shifted parameter under a root with a bound on the root",
     "\\sum_{k=1}^{n} \\ln\\left(\\sqrt{x - 5000} - 100\\right) = n \\ln\\left(\\sqrt{x - 5000} - 100\\right)", PROVED},
    {"a root confined to a narrow interval",
     "\\sum_{k=1}^{n} \\ln\\left(30 - \\sqrt{2 x + 7}\\right) \\ln\\left(\\sqrt{2 x + 7} - 29\\right) = "
     "n \\ln\\left(30 - \\sqrt{2 x + 7}\\right) \\ln\\left(\\sqrt{2 x + 7} - 29\\right)", PROVED},
    {"conditions with very large coefficients",
     "\\sum_{k=1}^{n} \\sqrt{\\left(x^{2} - 2000000\\right)\\left(2000100 - x^{2}\\right)} = "
     "n \\sqrt{\\left(x^{2} - 2000000\\right)\\left(2000100 - x^{2}\\right)}", PROVED},

    /* I metodi combinati: l'induzione e i casi sul resto come regole degli
     * enunciati con quantificatori. */
    {"a quantified statement proved by induction from zero",
     "\\forall n \\in \\mathbb{N}_{0}\\ \\left(\\sum_{k=1}^{n} k = \\frac{n\\left(n + 1\\right)}{2}\\right)", PROVED},
    {"induction on a letter other than n", "\\forall m \\in \\mathbb{N}\\ \\left(\\sum_{k=1}^{m} 2^{k} = 2^{m + 1} - 2\\right)", PROVED},
    {"induction from the first element of a set with a point removed",
     "\\forall n \\in \\mathbb{N} \\setminus \\{1\\}\\ \\left(\\prod_{k=2}^{n} \\left(1 - \\frac{1}{k}\\right) = \\frac{1}{n}\\right)", PROVED},
    {"induction and polynomial rules in the same statement",
     "\\forall n \\in \\mathbb{N}\\ \\left(\\sum_{k=1}^{n} k = \\frac{n\\left(n + 1\\right)}{2}\\right) \\land \\forall x \\in \\mathbb{Q}\\ (x + 1 \\neq x)", PROVED},
    {"a false quantified sum formula", "\\forall n \\in \\mathbb{N}\\ \\left(\\sum_{k=1}^{n} k = n^{2}\\right)", REFUTED},
    {"a quantified statement undefined at a later n", "\\forall n \\in \\mathbb{N}\\ \\left(\\frac{n - 50}{n - 50} = 1\\right)", NOT_PROVED},
    {"a sum with a value to be found", "\\exists n \\in \\mathbb{N}\\ \\left(\\sum_{k=1}^{n} k = 10\\right)", PROVED},
    {"a sum that never takes a value", "\\exists n \\in \\mathbb{N}\\ \\left(\\sum_{k=1}^{n} k = 11\\right)", NOT_PROVED},
    {"every integer is even or odd", "\\forall x \\in \\mathbb{Z}\\ \\exists y \\in \\mathbb{Z}\\ (x = 2 y \\lor x = 2 y + 1)", PROVED},
    {"three remainders modulo three", "\\forall x \\in \\mathbb{Z}\\ \\exists y \\in \\mathbb{Z}\\ (x = 3 y \\lor x = 3 y + 1 \\lor x = 3 y + 2)", PROVED},
    {"a product of two consecutive integers is even", "\\forall x \\in \\mathbb{Z}\\ \\exists y \\in \\mathbb{Z}\\ (x^{2} + x = 2 y)", PROVED},
    {"a product of three consecutive integers is a multiple of six", "\\forall x \\in \\mathbb{Z}\\ \\exists y \\in \\mathbb{Z}\\ (x^{3} - x = 6 y)", PROVED},
    {"squares modulo four", "\\forall x \\in \\mathbb{Z}\\ \\exists y \\in \\mathbb{Z}\\ (x^{2} = 4 y \\lor x^{2} = 4 y + 1)", PROVED},
    {"remainders on the natural numbers", "\\forall x \\in \\mathbb{N}\\ \\exists y \\in \\mathbb{N}_{0}\\ (x = 2 y + 1 \\lor x = 2 y + 2)", PROVED},
    {"not every integer is even", "\\forall x \\in \\mathbb{Z}\\ \\exists y \\in \\mathbb{Z}\\ (x = 2 y)", REFUTED},
    {"no square leaves remainder two modulo four", "\\forall x \\in \\mathbb{Z}\\ \\exists y \\in \\mathbb{Z}\\ (x^{2} = 4 y + 2)", REFUTED},
    {"three consecutive integers need not give a multiple of four", "\\forall x \\in \\mathbb{Z}\\ \\exists y \\in \\mathbb{Z}\\ (x^{3} - x = 4 y)", REFUTED},
    {"a remainder that leaves out the natural number one", "\\forall x \\in \\mathbb{N}\\ \\exists y \\in \\mathbb{N}\\ (x = 2 y \\lor x = 2 y + 1)", REFUTED},

    /* Enunciati sugli insiemi: un caso per tipo di oggetto e di forma. */
    {"set identity by membership table", "S \\setminus (T \\cup V) = (S \\setminus T) \\cap (S \\setminus V)", PROVED},
    {"set inclusion", "S \\cap T \\subseteq S \\cup V", PROVED},
    {"equivalence between two conditions on sets", "S \\subseteq T \\iff S \\cup T = T", PROVED},
    {"equivalence under hypotheses", "(X \\subseteq S \\land Y \\subseteq S) \\implies (X \\subseteq Y \\iff S \\setminus Y \\subseteq S \\setminus X)", PROVED},
    {"power set of an intersection", "\\mathcal{P}(S \\cap T) = \\mathcal{P}(S) \\cap \\mathcal{P}(T)", PROVED},
    {"power sets with a disjunction", "\\mathcal{P}(S) \\cup \\mathcal{P}(T) = \\mathcal{P}(S \\cup T) \\iff (S \\subseteq T \\lor T \\subseteq S)", PROVED},
    {"Cartesian product of an intersection", "(S \\cap T) \\times V = (S \\times V) \\cap (T \\times V)", PROVED},
    {"existence of a set", "S = T \\iff \\exists V \\left( S \\cap V = T \\cap V \\land S \\cup V = T \\cup V \\right)", PROVED},
    {"the empty set is a subset of every set", "\\emptyset \\subseteq S", PROVED},
    {"difference is not associative", "S \\setminus (T \\setminus V) = (S \\setminus T) \\setminus V", REFUTED},
    {"power set of a union is larger", "\\mathcal{P}(S) \\cup \\mathcal{P}(T) = \\mathcal{P}(S \\cup T)", REFUTED},
    {"products need the sets to be non-empty", "S \\times T \\subseteq V \\times W \\iff (S \\subseteq V \\land T \\subseteq W)", REFUTED},
    {"an existence claim that is always true", "S = T \\iff \\exists V \\left( S \\cap V = T \\cap V \\right)", REFUTED},
    {"a false equivalence with an existence claim", "\\exists V \\left( V \\neq \\emptyset \\land V \\subseteq S \\land V \\neq S \\right) \\iff S \\neq S", REFUTED},
    {"an existence claim the method cannot settle", "\\exists V \\left( V \\neq \\emptyset \\land V \\subseteq S \\land V \\neq S \\right)", NOT_PROVED},

    /* Antiregressione: questo enunciato è stato dimostrato per errore. Senza
     * simboli come l'unione veniva letto come prodotto di numeri, dove è vero;
     * per i prodotti cartesiani è falso. */
    {"Cartesian product is not commutative", "S \\times T = T \\times S", REFUTED},
    {"capital letters with a subscript are still numbers", "F_{n + 2} = F_{n + 1} + F_{n}", PROVED},
    {"set mixed with a collection of sets", "S \\cup \\mathcal{P}(T) = S", UNSUPPORTED},
    {"too many sets", "A \\cup B \\cup C \\cup D \\cup E = A", UNSUPPORTED},

    /* Enunciati con quantificatori: un caso per regola di deduzione. */
    {"a non-zero constant factor",
     "(f \\colon \\mathbb{Z} \\to \\mathbb{Z} \\land \\forall n \\in \\mathbb{Z}\\ (f(n) = n + 5)) \\implies (\\forall a, b \\in \\mathbb{Z}\\ (f(a) = f(b) \\implies a = b) \\land \\forall y \\in \\mathbb{Z}\\ \\exists x \\in \\mathbb{Z}\\ (f(x) = y))", PROVED},
    {"a factor that vanishes only where the conclusion holds",
     "(f \\colon \\mathbb{N}_{0} \\to \\mathbb{N}_{0} \\land \\forall n \\in \\mathbb{N}_{0}\\ (f(n) = n^{2})) \\implies \\forall a, b \\in \\mathbb{N}_{0}\\ (f(a) = f(b) \\implies a = b)", PROVED},
    {"a factor that is positive on a shifted half-line",
     "(f \\colon \\mathbb{N} \\to \\mathbb{N} \\land \\forall n \\in \\mathbb{N}\\ (f(n) = (n + 1)^{2})) \\implies \\forall a, b \\in \\mathbb{N}\\ (f(a) = f(b) \\implies a = b)", PROVED},
    {"strictly positive variables",
     "(f \\colon \\mathbb{Q}^{+} \\to \\mathbb{Q}^{+} \\land \\forall x \\in \\mathbb{Q}^{+}\\ (f(x) = x^{2})) \\implies \\forall a, b \\in \\mathbb{Q}^{+}\\ (f(a) = f(b) \\implies a = b)", PROVED},
    {"the integers split into two half-lines",
     "(f \\colon \\mathbb{Z} \\to \\mathbb{N}_{0} \\land \\forall n \\in \\mathbb{Z}\\ (f(n) = (n + 1)^{2})) \\implies \\lnot (\\forall a, b \\in \\mathbb{Z}\\ (f(a) = f(b) \\implies a = b))", PROVED},
    {"an integer-valued polynomial with fractional coefficients",
     "(f \\colon \\mathbb{Z} \\to \\mathbb{Z} \\land \\forall n \\in \\mathbb{Z}\\ (f(n) = \\frac{n^{2} + n}{2})) \\implies \\lnot (\\forall a, b \\in \\mathbb{Z}\\ (f(a) = f(b) \\implies a = b))", PROVED},
    {"a solution that must stay above a lower bound",
     "(f \\colon \\mathbb{N}_{0} \\to \\mathbb{N} \\land \\forall n \\in \\mathbb{N}_{0}\\ (f(n) = n + 1)) \\implies \\forall y \\in \\mathbb{N}\\ \\exists x \\in \\mathbb{N}_{0}\\ (f(x) = y)", PROVED},
    {"a solution that must avoid a removed point",
     "(f \\colon \\mathbb{Q} \\setminus \\{1\\} \\to \\mathbb{Q} \\setminus \\{1\\} \\land \\forall x \\in \\mathbb{Q} \\setminus \\{1\\}\\ (f(x) = \\frac{x + 1}{x - 1})) \\implies \\forall y \\in \\mathbb{Q} \\setminus \\{1\\}\\ \\exists x \\in \\mathbb{Q} \\setminus \\{1\\}\\ (f(x) = y)", PROVED},
    {"no rational root",
     "(f \\colon \\mathbb{N}_{0} \\to \\mathbb{N}_{0} \\land \\forall n \\in \\mathbb{N}_{0}\\ (f(n) = n^{2})) \\implies \\lnot (\\forall y \\in \\mathbb{N}_{0}\\ \\exists x \\in \\mathbb{N}_{0}\\ (f(x) = y))", PROVED},
    {"an equivalence is two implications",
     "\\forall x \\in \\mathbb{Z}\\ (2x = 0 \\iff x = 0)", PROVED},
    {"existence by solving a linear equation",
     "\\exists x \\in \\mathbb{Q}\\ (7x = 3)", PROVED},
    {"existence with the same value for every case",
     "\\forall y \\in \\mathbb{Q}\\ \\exists x \\in \\mathbb{Q}\\ (xy = 0)", PROVED},
    {"roots outside the set do not count",
     "\\forall n \\in \\mathbb{N}\\ (n^{2} = 4 \\implies n = 2)", PROVED},
    {"roots inside the set must satisfy the conclusion",
     "\\forall n \\in \\mathbb{N}_{0}\\ (n^{2} = n \\implies n^{3} = n)", PROVED},
    {"a divisor that cannot vanish",
     "\\forall x \\in \\mathbb{Q}^{+}\\ (\\frac{1}{x} \\neq 0)", PROVED},
    {"the square root of two is not rational",
     "\\forall x \\in \\mathbb{Q}\\ (x^{2} \\neq 2)", PROVED},
    {"a disjunction and a negation of comparisons",
     "\\forall x \\in \\mathbb{Z}\\ ((x = 1 \\lor x \\neq 1) \\land \\lnot (x \\neq x))", PROVED},

    /* Enunciati falsi con quantificatori: la negazione viene dimostrata. */
    {"doubling is not onto the integers",
     "(f \\colon \\mathbb{Z} \\to \\mathbb{Z} \\land \\forall n \\in \\mathbb{Z}\\ (f(n) = 2n)) \\implies \\forall y \\in \\mathbb{Z}\\ \\exists x \\in \\mathbb{Z}\\ (f(x) = y)", REFUTED},
    {"squaring is not injective on the integers",
     "(f \\colon \\mathbb{Z} \\to \\mathbb{Z} \\land \\forall n \\in \\mathbb{Z}\\ (f(n) = n^{2})) \\implies \\forall a, b \\in \\mathbb{Z}\\ (f(a) = f(b) \\implies a = b)", REFUTED},
    {"true on the natural numbers, false on the integers",
     "\\forall n \\in \\mathbb{Z}\\ (n^{2} = 4 \\implies n = 2)", REFUTED},
    {"a solution that falls below the lower bound",
     "\\forall x \\in \\mathbb{N}\\ \\exists y \\in \\mathbb{N}\\ (x = y + 1)", REFUTED},
    {"a solution that is not an integer",
     "\\exists x \\in \\mathbb{Z}\\ (2x = 1)", REFUTED},
    {"a value missing from a larger codomain",
     "(f \\colon \\mathbb{Q} \\setminus \\{1\\} \\to \\mathbb{Q} \\land \\forall x \\in \\mathbb{Q} \\setminus \\{1\\}\\ (f(x) = \\frac{x + 1}{x - 1})) \\implies \\forall y \\in \\mathbb{Q}\\ \\exists x \\in \\mathbb{Q} \\setminus \\{1\\}\\ (f(x) = y)", REFUTED},
    {"a root inside the set breaks the implication",
     "\\forall n \\in \\mathbb{N}_{0}\\ (n^{2} = n \\implies n = 1)", REFUTED},

    /* Ipotesi che nessuna applicazione soddisfa: l'enunciato è vero a vuoto,
     * quindi non va smentito; e non c'è nulla da dimostrare. */
    {"a rule whose values leave the codomain",
     "(f \\colon \\mathbb{Z} \\to \\mathbb{N} \\land \\forall n \\in \\mathbb{Z}\\ (f(n) = (n + 1)^{2})) \\implies \\forall a, b \\in \\mathbb{Z}\\ (f(a) = f(b) \\implies a = b)", NOT_PROVED},
    {"a rule undefined at a point of the domain",
     "(f \\colon \\mathbb{Q} \\to \\mathbb{Q} \\land \\forall x \\in \\mathbb{Q}\\ (f(x) = \\frac{1}{x})) \\implies \\forall a, b \\in \\mathbb{Q}\\ (f(a) = f(b) \\implies a = b)", NOT_PROVED},
    {"a comparison undefined at a point of the set",
     "\\forall x \\in \\mathbb{Q}\\ (\\frac{x}{x} = 1)", NOT_PROVED},

    /* Enunciati veri o falsi che le regole non raggiungono. */
    {"cubing is injective, but the sign rule cannot show it",
     "(f \\colon \\mathbb{Z} \\to \\mathbb{Z} \\land \\forall n \\in \\mathbb{Z}\\ (f(n) = n^{3})) \\implies \\forall a, b \\in \\mathbb{Z}\\ (f(a) = f(b) \\implies a = b)", NOT_PROVED},
    /* Falso, e ora smentito: per x pari si prende y = 1, per x dispari
     * y = 0 (casi sul resto, 18o). Prima il programma non sapeva né
     * dimostrarlo né smentirlo; non deve mai dimostrarlo. */
    {"a value that would have to depend on the other variable",
     "\\exists x \\in \\mathbb{Z}\\ \\forall y \\in \\mathbb{Z}\\ (x = y)", REFUTED},

    /* Fuori dal linguaggio: va dichiarato. Sui numeri reali la regola delle
     * radici razionali darebbe per vero x^2 diverso da 2. */
    {"real numbers are not accepted",
     "\\forall x \\in \\mathbb{R}\\ (x^{2} \\neq 2)", UNSUPPORTED},
    {"a letter that is not quantified",
     "\\forall x \\in \\mathbb{Z}\\ (x + c = c + x)", UNSUPPORTED},
    {"a letter quantified inside itself",
     "\\forall x \\in \\mathbb{Z}\\ \\exists x \\in \\mathbb{Z}\\ (x = x)", UNSUPPORTED},
    {"a map applied outside its domain",
     "(f \\colon \\mathbb{N} \\to \\mathbb{N} \\land \\forall n \\in \\mathbb{N}\\ (f(n) = n + 1)) \\implies \\forall x \\in \\mathbb{Z}\\ (f(x) = x + 1)", UNSUPPORTED},
    {"a map without a rule",
     "(f \\colon \\mathbb{Z} \\to \\mathbb{Z}) \\implies \\forall a \\in \\mathbb{Z}\\ (f(a) = f(a))", UNSUPPORTED},
    {"a rule that refers to its own map",
     "(f \\colon \\mathbb{Z} \\to \\mathbb{Z} \\land \\forall n \\in \\mathbb{Z}\\ (f(n) = f(n))) \\implies \\forall a \\in \\mathbb{Z}\\ (f(a) = f(a))", UNSUPPORTED},
    {"an inequality between quantified numbers",
     "\\forall x \\in \\mathbb{N}\\ (x \\ge 1)", UNSUPPORTED},

    /* Antiregressione: enunciati veri che la verifica sul campione non riusciva a
     * confermare, perché con molte variabili esauriva la memoria di lavoro. */
    {"five universal variables can still be checked on a sample",
     "\\forall a, b, c, d, e \\in \\mathbb{Z}\\ (a + b + c + d + e = e + d + c + b + a)", PROVED},
    {"eight nested universal quantifiers",
     "\\forall a \\in \\mathbb{N}\\ \\forall b \\in \\mathbb{N}\\ \\forall c \\in \\mathbb{N}\\ \\forall d \\in \\mathbb{N}\\ \\forall e \\in \\mathbb{N}\\ \\forall g \\in \\mathbb{N}\\ \\forall h \\in \\mathbb{N}\\ \\forall m \\in \\mathbb{N}\\ (a = a)", PROVED},

    /* Antiregressione: ricerche che crescono in modo esponenziale si fermano con
     * un "non dimostrato", non per memoria esaurita. */
    {"too many values to try one after another",
     "\\exists a \\in \\mathbb{Z}\\ \\exists b \\in \\mathbb{Z}\\ \\exists c \\in \\mathbb{Z}\\ \\exists d \\in \\mathbb{Z}\\ \\exists e \\in \\mathbb{Z}\\ \\exists g \\in \\mathbb{Z}\\ (a^{2} + b^{2} + c^{2} + d^{2} + e^{2} = 1000003 \\land g = g)", NOT_PROVED},
    {"a chain of equivalences too long to explore",
     "\\forall x \\in \\mathbb{N}\\ (x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1 \\iff x = 1)", NOT_PROVED},

    /* Un insieme di interi privato dei suoi primi elementi comincia più in alto. */
    {"integers without their first elements",
     "(f \\colon \\mathbb{N} \\setminus \\{2, 1\\} \\to \\mathbb{N} \\land \\forall n \\in \\mathbb{N} \\setminus \\{1, 2\\}\\ (f(n) = n - 2)) \\implies \\forall y \\in \\mathbb{N}\\ \\exists x \\in \\mathbb{N} \\setminus \\{1, 2\\}\\ (f(x) = y)", PROVED},

    /* Antiregressione: due copie della stessa somma erano due quantità diverse, e
     * un'espressione con un prodotto al denominatore non risultava uguale a se
     * stessa. Somme diverse devono restare diverse. */
    {"an expression with a product in a denominator equals itself",
     "\\frac{2}{\\prod_{k=1}^{n} 2} = \\frac{2}{\\prod_{k=1}^{n} 2}", PROVED},
    {"the square of a sum is the sum times itself",
     "\\left(\\sum_{k=1}^{n} k\\right)^{2} = \\left(\\sum_{k=1}^{n} k\\right) \\cdot \\sum_{k=1}^{n} k", PROVED},
    {"the same sum taken one term further",
     "n \\sum_{k=1}^{n + 1} k = n \\sum_{k=1}^{n} k + n\\left(n + 1\\right)", PROVED},
    {"sums with different terms stay different",
     "n \\sum_{k=1}^{n} k = n \\sum_{k=1}^{n} k^{2}", REFUTED},
    {"sums with different starting points stay different",
     "n \\sum_{k=1}^{n} k = n \\sum_{k=2}^{n} k", REFUTED},
    {"a sum and a product of the same terms stay different",
     "n \\sum_{k=1}^{n} k = n \\prod_{k=1}^{n} k", REFUTED},

    /* Antiregressione: enunciati che l'algebra accetta ma che non hanno senso in
     * nessun punto, o non per ogni n. Risultavano dimostrati "dove i due membri
     * sono definiti". */
    {"two logarithms that are never defined together",
     "\\ln\\left(-x\\right) = \\ln\\left(x\\right)", NOT_PROVED},
    {"a logarithm of an expression that is never positive",
     "\\ln\\left(-x^{2} - 1\\right) = \\ln\\left(x^{2} + 1\\right)", NOT_PROVED},
    {"a root of an expression that is never positive",
     "\\sqrt{-x^{2} - 1} \\cdot \\sqrt{-x^{2} - 1} = -x^{2} - 1", NOT_PROVED},
    {"a denominator that an identity between logarithms makes zero",
     "\\frac{\\ln 2 - \\ln 2}{\\ln\\left(1 - a\\right) + \\ln\\left(1 + a\\right) - \\ln\\left(1 - a^{2}\\right)} = 0", NOT_PROVED},
    {"an expression undefined for one of the first values of n",
     "\\frac{n - 5}{n - 5} = 1", NOT_PROVED},
    {"a product that divides by zero from its third factor on",
     "\\prod_{k=1}^{n} \\frac{k - 3}{k - 3} = 1", NOT_PROVED},
    {"a logarithm that stops being defined at n = 10",
     "\\ln\\left(\\left(10 - n\\right)^{2}\\right) = 2\\ln\\left(10 - n\\right)", NOT_PROVED},

    /* Gli stessi controlli non devono fermare gli enunciati sensati: basta un
     * punto in cui tutto è definito, anche con un parametro negativo, grande, o
     * lontano dai poli. */
    {"defined only for a negative parameter",
     "\\sqrt{-x}\\,\\sqrt{-x} = -x", PROVED},
    {"defined only for a large parameter",
     "\\ln\\left(x - 100\\right) + \\ln 2 = \\ln\\left(2x - 200\\right)", PROVED},
    {"poles that depend on the parameter",
     "\\sum_{k=1}^{n} \\frac{1}{\\left(k - a\\right)\\left(k + 1 - a\\right)} = \\frac{1}{1 - a} - \\frac{1}{n + 1 - a}", PROVED},

    /* "Esiste" con un'equazione di grado più alto: i valori possibili sono le
     * radici razionali, anche lontane dai numeri più semplici. */
    {"a value found among the rational roots",
     "\\exists x \\in \\mathbb{Z}\\ (x^{2} = 1681)", PROVED},
    {"the same value refutes the opposite claim",
     "\\forall x \\in \\mathbb{Z}\\ (x^{2} \\neq 1681)", REFUTED},
    {"a rational root that is not an integer",
     "\\exists x \\in \\mathbb{Q}\\ (x^{2} = \\frac{49}{169})", PROVED},
    {"a root must also satisfy the rest of the condition",
     "\\exists x \\in \\mathbb{Q}\\ (6x^{3} - 11x^{2} + 6x = 1 \\land 3x \\neq 1 \\land 2x \\neq 1)", PROVED},
    {"the only root in the set is excluded by the rest",
     "\\exists x \\in \\mathbb{N}\\ (x^{2} = 1681 \\land x \\neq 41)", REFUTED},
    {"no rational root, so no value",
     "\\exists x \\in \\mathbb{Q}\\ (x^{2} = 2)", REFUTED},

    /* Convenzioni del parser. */
    {"implicit multiplication", "\\sum_{k=1}^{n} 2k = n n + n", PROVED},
    {"unbraced exponents and digits are single tokens", "\\sum_{k=1}^n k = \\frac{n^2 + n}2", PROVED},
    {"a sum extends over the whole following term", "\\sum_{k=1}^{n} 2 k + 1 = n^{2} + n + 1", PROVED},
    {"spacing commands are ignored", "\\displaystyle \\sum_{k=1}^{n}\\, k \\;=\\; \\frac{n^{2} + n}{2}", PROVED},

    /* Enunciati falsi: non devono mai risultare dimostrati. */
    {"wrong constant fails the base case", "\\sum_{k=1}^{n} k = \\frac{n^{2} + n}{2} + 1", REFUTED},
    {"right base case but wrong step", "\\sum_{k=1}^{n} k = \\frac{n^{2} + n}{2} + n - 1", REFUTED},
    {"wrong closed form of a geometric series", "\\sum_{k=1}^{n} 2^{k} = 2^{n + 1} - 1", REFUTED},
    {"wrong product", "\\prod_{k=1}^{n} \\left(1 + \\frac{1}{k}\\right) = n + 2", REFUTED},
    {"false statement without sums", "2^{n} = n^{2}", REFUTED},
    {"true only for even n", "\\sum_{k=1}^{n} \\left(-1\\right)^{k} = 0", REFUTED},
    {"a vanishing coefficient must not hide a wrong formula",
     "\\left(n - 1\\right)\\sum_{k=1}^{n} k = \\left(n - 1\\right)\\left(\\frac{n^{2} + n}{2} + 7\\right)", REFUTED},
    {"square roots are not negative", "\\sqrt{4} = -2", REFUTED},
    {"sine formula with a cosine in it",
     "\\sum_{k=1}^{n} \\sin\\left(k x\\right) = \\frac{\\sin\\left(\\frac{n x}{2}\\right)\\,\\cos\\left(\\frac{\\left(n + 1\\right)x}{2}\\right)}{\\sin\\left(\\frac{x}{2}\\right)}", NOT_PROVED},
    {"root of x squared is not x for negative x", "\\sqrt{x^{2}} = x", NOT_PROVED},
    {"wrong telescoping roots", "\\sum_{k=1}^{n} \\frac{1}{\\sqrt{k} + \\sqrt{k + 1}} = \\sqrt{n + 1}", NOT_PROVED},
    {"wrong telescoping logarithms", "\\sum_{k=1}^{n} \\ln\\left(1 + \\frac{1}{k}\\right) = \\ln\\left(n + 2\\right)", NOT_PROVED},
    {"sum of roots is not the root of the sum", "\\sqrt{n + 1} + \\sqrt{n} = \\sqrt{2n + 1}", NOT_PROVED},
    {"same tangent but wrong branch of the arctangent", "\\arctan 2 + \\arctan 3 = -\\frac{\\pi}{4}", NOT_PROVED},
    {"arctangent identity true only for positive x", "\\arctan x + \\arctan\\left(\\frac{1}{x}\\right) = \\frac{\\pi}{2}", NOT_PROVED},
    {"wrong telescoping arctangents", "\\sum_{k=1}^{n} \\arctan\\left(\\frac{1}{2k^{2}}\\right) = \\arctan\\left(\\frac{n}{n + 2}\\right)", NOT_PROVED},
    {"arctangent is not linear", "\\arctan\\left(2n\\right) = 2\\arctan n", NOT_PROVED},

    /* Enunciati veri fuori dalla portata del metodo: la risposta onesta è
     * "non dimostrato", non un sì. */
    {"needs a lemma about the inner sum", "\\left(\\sum_{k=1}^{n} k\\right)^{2} = \\sum_{k=1}^{n} k^{3}", NOT_PROVED},
    {"hypothesis with a coefficient depending on n", "\\frac{1}{n}\\sum_{k=1}^{n} k = \\frac{n + 1}{2}", NOT_PROVED},
    {"Machin's formula has too many arctangents for the size bound",
     "4\\arctan\\left(\\frac{1}{5}\\right) - \\arctan\\left(\\frac{1}{239}\\right) = \\frac{\\pi}{4}", NOT_PROVED},
    {"needs Cassini's identity for Fibonacci numbers",
     "\\sum_{k=1}^{n} \\arctan\\left(\\frac{1}{F_{2k + 1}}\\right) = \\frac{\\pi}{4} - \\arctan\\left(\\frac{1}{F_{2n + 2}}\\right)", NOT_PROVED},

    /* Ciò che il motore non tratta deve essere dichiarato, non indovinato. */
    {"inequalities are reported as unsupported", "2^{n} \\ge n", UNSUPPORTED},
    {"summand depending on n", "\\sum_{k=0}^{n} \\binom{n}{k} = 2^{n}", UNSUPPORTED},
    {"unknown LaTeX command", "\\int_0^1 x = \\frac{1}{2}", UNSUPPORTED},
    {"incomplete statement", "\\sum_{k=1}^{n} k =", UNSUPPORTED},
    {"missing relation", "\\sum_{k=1}^{n} k", UNSUPPORTED},
    {"sequence that is not in the library", "a_{n} = 1", UNSUPPORTED},
    {"logarithm of a negative number", "\\ln\\left(-2\\right) = \\ln 2", UNSUPPORTED},
    {"division by zero", "\\frac{1}{n - n} = 1", UNSUPPORTED},
    {"absolute value outside a logarithm has no rule yet", "\\left|n\\right| = n", UNSUPPORTED},
    {"arctangent of a complex expression", "\\arctan\\left(\\sin x\\right) = \\arctan\\left(\\sin x\\right)", UNSUPPORTED},
};

/* Nome di un esito, per i messaggi. */
static const char *outcome_name(Outcome outcome) {
    switch (outcome) {
    case PROVED:      return "PROVED";
    case REFUTED:     return "REFUTED";
    case NOT_PROVED:  return "NOT_PROVED";
    case UNSUPPORTED: return "UNSUPPORTED";
    }
    return "?";
}

/* Numero di controlli falliti finora, e di controlli fatti con check(). */
static int failures, extra_checks;

/* Registra un controllo: se condition è falsa stampa message e conta il
 * fallimento. */
static void check(int condition, const char *message) {
    extra_checks++;
    if (condition) return;
    printf("FAIL: %s\n", message);
    failures++;
}

/* Dimostra il testo di un \item e ritorna il verdetto. */
static Verdict item_verdict(const char *item) { return prove_item(item, item + strlen(item)); }

/* Ritorna 1 se la condizione sugli insiemi scritta in formula, stampata come
 * la stampano le dimostrazioni (put_set()), si rilegge come una condizione
 * equivalente: si chiede al programma di dimostrare "originale se e solo se
 * stampata". Controlla che ciò che una dimostrazione scrive sia ciò che il
 * programma intendeva. */
static int set_prints_back(const char *formula) {
    char statement[2 * MAX_FORMULA];
    proof_reset();
    arena_used = arena_keep;
    failure = NULL;
    set_var_count = 0;
    syntax_error = NULL;
    cursor = formula;
    next_token();
    SetNode *root = set_formula();
    if (syntax_error || failure || set_check(root) != TY_FORMULA) return 0;
    put_set(root);
    snprintf(statement, sizeof statement, "(%s) \\iff (%s)", formula, proof_text);
    return prove(statement, 'n', 0, 0).outcome == PROVED;
}

/* Lo stesso per un'espressione riscritta dall'albero sintattico, come nelle
 * dimostrazioni per induzione (put_node()), con n sostituita da n + 1:
 * "originale con n + 1 al posto di n = stampata" deve risultare dimostrato.
 * shifted è l'espressione originale con la sostituzione già fatta a mano. */
static int node_prints_back(const char *expression, const char *shifted) {
    char statement[2 * MAX_FORMULA];
    proof_reset();
    arena_used = arena_keep;
    failure = NULL;
    Node *tree = parse_expression(expression);
    if (syntax_error || failure) return 0;
    shown_name = 'n';
    shown_value = node_affine(1, 'n', 1);
    put_node(tree, 0);
    shown_value = NULL;
    snprintf(statement, sizeof statement, "%s = %s", shifted, proof_text);
    return prove(statement, 'n', 0, 0).outcome == PROVED;
}

/* Lo stesso per il valore di un'espressione, stampato come nelle
 * dimostrazioni per induzione (put_rat()): somme, potenze, radici, logaritmi
 * e successioni devono rileggersi come le quantità di partenza. */
static int value_prints_back(const char *expression) {
    char statement[2 * MAX_FORMULA];
    proof_reset();
    arena_used = arena_keep;
    atom_count = base_poly_count = atan_count = 0;
    failure = NULL;
    induction_var = 'n';
    default_base = domain_floor = 1;
    Node *tree = parse_expression(expression);
    if (syntax_error) return 0;
    Rat value = side_value(tree, rat_atom(atom_var('n')));
    unwritable = 0;
    if (!failure) put_rat(value);
    if (failure || unwritable) return 0;
    snprintf(statement, sizeof statement, "%s = %s", expression, proof_text);
    return prove(statement, 'n', 0, 0).outcome == PROVED;
}

/* Decide con condition_holds() se l'espressione scritta in expression
 * rispetta kind per ogni n >= base: ritorna 1, 0 (con il primo n in cui
 * fallisce in *where) o -1, come quella funzione; -2 se l'espressione non si
 * legge o non è un polinomio negli atomi. */
static int condition_answer(const char *expression, Requirement kind, long long base, long long *where) {
    arena_used = arena_keep;
    atom_count = base_poly_count = atan_count = 0;
    failure = NULL;
    induction_var = 'n';
    default_base = 1;
    domain_floor = base;
    Node *tree = parse_expression(expression);
    if (syntax_error) return -2;
    Rat value = side_value(tree, rat_atom(atom_var('n')));
    if (failure || !poly_is_one(value.den)) return -2;
    return condition_holds(kind, value.num, base, where);
}

/* Ritorna il polinomio negli atomi a cui si riduce l'espressione scritta in
 * expression, e mette in *x l'unico atomo che vi compare: la lettera x,
 * oppure, sotto una radice, l'atomo "x preso come base". */
static Poly expression_poly(const char *expression, int *x) {
    arena_used = arena_keep;
    atom_count = base_poly_count = atan_count = 0;
    failure = NULL;
    induction_var = 'n';
    default_base = domain_floor = 1;
    Node *tree = parse_expression(expression);
    Rat value = side_value(tree, rat_atom(atom_var('n')));
    *x = atom_var('x');
    for (int i = 0; i < value.num.count; i++)
        if (value.num.term[i].mono.count) *x = value.num.term[i].mono.factor[0].atom;
    return value.num;
}

/* Ritorna 1 se la dimostrazione dell'enunciato statement contiene la frase
 * sentence. Serve a fissare le frasi che collegano i fatti: chi le cambia
 * deve rileggerle e aggiornare il test. */
static int proof_says(const char *statement, const char *sentence) {
    Verdict verdict = prove(statement, 'n', 0, 0);
    return verdict.outcome == PROVED && verdict.proof && strstr(verdict.proof, sentence) != NULL;
}

/* Lo stesso per un'espressione razionale, stampata come nelle dimostrazioni
 * con quantificatori (put_ratio()): "originale = stampata" deve risultare
 * dimostrato. */
static int ratio_prints_back(const char *expression) {
    char statement[2 * MAX_FORMULA];
    Poly num, den;
    proof_reset();
    arena_used = arena_keep;
    atom_count = 0;
    failure = NULL;
    env_count = 0;
    Node *tree = parse_expression(expression);
    if (syntax_error) return 0;
    Rat value = eval(tree);
    if (failure || !rat_polynomials(value, &num, &den)) return 0;
    put_ratio(num, den);
    snprintf(statement, sizeof statement, "%s = %s", expression, proof_text);
    return prove(statement, 'n', 0, 0).outcome == PROVED;
}

int main(void) {
    engine_init();

    /* Casi in tabella: l'esito di prove() deve essere quello dichiarato. */
    int count = (int)(sizeof cases / sizeof *cases);
    for (int i = 0; i < count; i++) {
        Verdict verdict = prove(cases[i].formula, 'n', 0, 0);
        if (verdict.outcome == cases[i].expected) continue;
        printf("FAIL: %s: expected %s, got %s\n      %s\n", cases[i].name,
               outcome_name(cases[i].expected), outcome_name(verdict.outcome), cases[i].formula);
        failures++;
    }

    /* Caso base e variabile di induzione scelti dal chiamante. */
    Verdict from_zero = prove("\\sum_{k=0}^{m} 2^{k} = 2^{m + 1} - 1", 'm', 1, 0);
    check(from_zero.outcome == PROVED && from_zero.base == 0, "induction on m starting from 0 should be proved with base 0");

    /* Caso base scelto dal programma: il primo n per cui la somma ha termini. */
    Verdict from_two = prove("\\sum_{k=2}^{n} \\frac{1}{k\\left(k - 1\\right)} = 1 - \\frac{1}{n}", 'n', 0, 0);
    check(from_two.outcome == PROVED && from_two.base == 2, "a sum starting at k = 2 should use n = 2 as its base case");

    /* Un controesempio riportato deve essere un n in cui l'enunciato cade. */
    Verdict witness = prove("\\sum_{k=1}^{n} k = \\frac{n^{2} + n}{2} + n - 1", 'n', 0, 0);
    check(witness.outcome == REFUTED && witness.witness == 2, "the counterexample should be n = 2");

    /* Lettura di un \item: formula in evidenza, con o senza intervallo. */
    Verdict display = item_verdict(" \\[ \\sum_{k=2}^{n} \\ln\\left(1 - \\frac{1}{k}\\right) = -\\ln n \\]\n{\\small per ogni intero $n\\ge 2$.}\n");
    check(display.outcome == PROVED && display.base == 2, "a display formula followed by a range should use that range");
    Verdict inline_display = item_verdict(" $\\displaystyle \\sum_{k=1}^{n} k = \\frac{n^{2} + n}{2}$\n");
    check(inline_display.outcome == PROVED && inline_display.base == 1, "a displaystyle formula should be read as a statement");
    Verdict prose = item_verdict(" Sia $a_0=0$. Allora $a_n=0$ per ogni $n\\ge0$.\n");
    check(prose.outcome == UNSUPPORTED, "an item written in prose should be unsupported");

    /* Ogni enunciato dimostrato porta con sé la dimostrazione scritta, e ogni
     * enunciato sugli insiemi smentito porta il controesempio. */
    Verdict by_element = prove("S \\setminus (S \\setminus T) = S \\cap T", 'n', 0, 0);
    check(by_element.outcome == PROVED && by_element.proof && strstr(by_element.proof, "Let $x \\in"), "a set identity should be proved by following an arbitrary element");
    Verdict by_induction = prove("\\sum_{k=1}^{n} k = \\frac{n^{2} + n}{2}", 'n', 0, 0);
    check(by_induction.outcome == PROVED && by_induction.proof && strstr(by_induction.proof, "induction"), "an induction should come with a written proof");
    Verdict counter = prove("S \\cup T = S", 'n', 0, 0);
    check(counter.outcome == REFUTED && counter.detail && strcmp(counter.detail, "S = {}, T = {1}") == 0, "the smallest counterexample to S \\cup T = S is S = {}, T = {1}");

    /* Un enunciato con quantificatori smentito riporta i numeri scelti, e
     * la sua dimostrazione dice che è falso. */
    Verdict square = prove("(f \\colon \\mathbb{Z} \\to \\mathbb{Z} \\land \\forall n \\in \\mathbb{Z}\\ (f(n) = n^{2})) "
                           "\\implies \\forall a, b \\in \\mathbb{Z}\\ (f(a) = f(b) \\implies a = b)", 'n', 0, 0);
    check(square.outcome == REFUTED && square.detail && strcmp(square.detail, "a = 1, b = -1") == 0, "the counterexample to the injectivity of the square should be a = 1, b = -1");
    check(square.proof && strstr(square.proof, "The statement is false."), "a refuted quantified statement should say that it is false");

    /* Una dimostrazione con quantificatori mostra il valore che ha ricavato. */
    Verdict solved = prove("\\forall y \\in \\mathbb{Q}\\ \\exists x \\in \\mathbb{Q}\\ (2x = y)", 'n', 0, 0);
    check(solved.outcome == PROVED && solved.proof && strstr(solved.proof, "take $x = \\frac{1}{2}y$"), "the written proof should show the solution x = y/2");

    /* Dopo un enunciato che dichiara un'applicazione, la stessa lettera
     * torna a essere una variabile qualsiasi. */
    Verdict letter = prove("f\\left(f + 1\\right) = f^{2} + f", 'n', 0, 0);
    check(letter.outcome == PROVED, "a map declared by one statement should not leak into the next one");

    /* Antiregressione: una condizione scritta dopo la formula e non capita dal
     * programma. L'enunciato "per ogni n pari" veniva dichiarato falso per
     * n = 1; ora non può essere smentito, ma può ancora essere dimostrato. */
    Verdict even = item_verdict(" $\\displaystyle \\sum_{k=1}^{n} \\left(-1\\right)^{k} = 0$ per ogni $n$ pari.\n");
    check(even.outcome == NOT_PROVED, "a statement with a condition that was not read should not be refuted");
    Verdict restricted = item_verdict(" \\[ \\sum_{k=1}^{n} a^{k} = \\frac{a^{n + 1} - a}{a - 1} \\]\n{\\small per ogni intero $n\\ge 1$, con $a\\neq 1$.}\n");
    check(restricted.outcome == PROVED && restricted.base == 1, "a condition that was not read should not prevent a proof");

    /* Antiregressione: un \\item con due uguaglianze risultava dimostrato
     * guardando solo la prima, anche con la seconda falsa. */
    Verdict two = item_verdict(" $\\displaystyle \\sum_{k=1}^{n} k = \\frac{n^{2} + n}{2}$ e inoltre $\\displaystyle \\sum_{k=1}^{n} k^{2} = n$.\n");
    check(two.outcome == UNSUPPORTED, "an item with a second equality should not be reported as proved");

    /* Antiregressione: un caso base negativo veniva ignorato, e l'enunciato
     * risultava dimostrato da n = 1 invece che dal punto indicato. */
    Verdict below = item_verdict(" \\[ \\sum_{k=-3}^{n} 1 = n + 4 \\]\nper ogni intero $n\\ge -3$.\n");
    check(below.outcome == PROVED && below.base == -3, "a negative starting point should be read");
    Verdict wrong_below = item_verdict(" \\[ \\arctan\\left(n\\right) + \\arctan\\left(\\frac{1}{n}\\right) = \\frac{\\pi}{2} \\]\nper ogni intero $n\\ge -5$.\n");
    check(wrong_below.outcome != PROVED, "a statement that fails at its negative starting point should not be proved from n = 1");

    /* Antiregressione: una condizione scritta a parole, senza formula. Con
     * "pari" l'enunciato non può essere dichiarato falso; con le sole parole
     * di "per ogni intero" sì, in italiano e in inglese. */
    Verdict even_word = item_verdict(" $\\displaystyle \\sum_{k=1}^{n} \\left(-1\\right)^{k} = 0$ per ogni intero $n\\ge 1$ pari.\n");
    check(even_word.outcome == NOT_PROVED, "a condition written as a word should prevent a refutation");
    Verdict plain_words = item_verdict(" $\\displaystyle \\sum_{k=1}^{n} \\left(-1\\right)^{k} = 0$ {\\small per ogni intero $n\\ge 1$.}\n");
    check(plain_words.outcome == REFUTED, "the words of 'per ogni intero' are not a condition");
    Verdict english_words = item_verdict(" $\\displaystyle \\sum_{k=1}^{n} \\left(-1\\right)^{k} = 0$ for every integer $n\\ge 1$.\n");
    check(english_words.outcome == REFUTED, "the words of 'for every integer' are not a condition");

    /* Un enunciato non definito in un punto vale da dove comincia a esserlo. */
    Verdict from_six = item_verdict(" \\[ \\frac{n - 5}{n - 5} = 1 \\]\nper ogni intero $n\\ge 6$.\n");
    check(from_six.outcome == PROVED && from_six.base == 6, "a statement undefined at n = 5 should be proved from n = 6");

    /* Il valore trovato tra le radici razionali è quello riportato. */
    Verdict root = prove("\\forall x \\in \\mathbb{Z}\\ (x^{2} \\neq 1681)", 'n', 0, 0);
    check(root.outcome == REFUTED && root.detail && strcmp(root.detail, "x = 41") == 0, "the counterexample to x^2 != 1681 should be x = 41");

    /* Ciò che le dimostrazioni stampano deve rileggersi come ciò che il
     * programma intendeva: condizioni sugli insiemi ed espressioni razionali,
     * con le parentesi, i segni e le frazioni al posto giusto. */
    static const char *const set_formulas[] = {
        "S \\setminus (T \\setminus V) = (S \\setminus T) \\cup (S \\cap V)",
        "S \\setminus T \\setminus V = (S \\setminus T) \\setminus V",
        "(S \\cup T) \\cap V \\subseteq S \\cup (T \\cap V)",
        "S \\triangle (T \\triangle V) = (S \\triangle T) \\triangle V",
        "(S \\subseteq T \\land T \\subseteq V) \\implies S \\subseteq V",
        "S \\subseteq T \\implies (T \\subseteq V \\implies S \\subseteq V)",
        "\\lnot (S = T) \\iff (S \\neq T \\lor S \\setminus S \\neq \\emptyset)",
        "(S \\subseteq T \\iff S \\cup T = T) \\land (S \\setminus T = \\emptyset \\implies S \\subseteq T)",
        "\\mathcal{P}(S \\cap T) = \\mathcal{P}(S) \\cap \\mathcal{P}(T)",
        "\\mathcal{P}(S \\setminus T) \\subseteq (\\mathcal{P}(S) \\setminus \\mathcal{P}(T)) \\cup \\{\\emptyset\\}",
        "(S \\times T) \\cap (V \\times W) = (S \\cap V) \\times (T \\cap W)",
    };
    for (size_t i = 0; i < sizeof set_formulas / sizeof *set_formulas; i++)
        check(set_prints_back(set_formulas[i]), set_formulas[i]);
    static const char *const expressions[] = {
        "3a - 3b",
        "\\frac{a^{2} - b^{2}}{2}",
        "\\left(x + 1\\right)^{3} - x^{3}",
        "\\frac{x + 1}{x - 1} - 1",
        "-x^{2} y + \\frac{2}{3} x y^{2} - 7",
        "\\frac{1}{x} + \\frac{1}{y}",
        "\\frac{a}{1 - a} - \\frac{a^{2}}{a - 1}",
        "-\\frac{5}{3}",
        "a b c - \\frac{a}{b}",
        "\\frac{-x - 1}{-x + 1}",
    };
    for (size_t i = 0; i < sizeof expressions / sizeof *expressions; i++)
        check(ratio_prints_back(expressions[i]), expressions[i]);

    /* Le espressioni riscritte dalle dimostrazioni per induzione, con le
     * loro parentesi: somme dentro prodotti, potenze di funzioni, fattoriali,
     * frazioni, indici di somma che nascondono una variabile. */
    static const char *const trees[][2] = {
        {"n \\left(n + 1\\right) / 2", "\\left(n + 1\\right) \\left(n + 2\\right) / 2"},
        {"2 n - 3 \\left(n - 1\\right)^{2}", "2 \\left(n + 1\\right) - 3 n^{2}"},
        {"n \\sum_{k=1}^{n} k", "\\left(n + 1\\right) \\sum_{k=1}^{n + 1} k"},
        {"\\sum_{k=1}^{n} k \\cdot 2 + n", "\\sum_{k=1}^{n + 1} 2 k + n + 1"},
        {"\\left(\\sum_{k=1}^{n} k\\right)^{2} - n!", "\\left(\\sum_{k=1}^{n + 1} k\\right)^{2} - \\left(n + 1\\right)!"},
        {"\\sin^{2}\\left(n x\\right) - \\cos\\left(2 n x\\right)", "\\sin^{2}\\left(\\left(n + 1\\right) x\\right) - \\cos\\left(2 \\left(n + 1\\right) x\\right)"},
        {"\\frac{2^{n} - n}{n + 1} - \\binom{n}{2}", "\\frac{2^{n + 1} - n - 1}{n + 2} - \\binom{n + 1}{2}"},
        {"-\\sqrt{n} \\sqrt[3]{n + 7} - \\ln\\left(n\\right)", "-\\sqrt{n + 1} \\sqrt[3]{n + 8} - \\ln\\left(n + 1\\right)"},
        {"F_{n + 1} F_{n} - \\left(-1\\right)^{n} n", "F_{n + 2} F_{n + 1} + \\left(-1\\right)^{n} \\left(n + 1\\right)"},
        {"\\sum_{n=1}^{n} n - \\prod_{k=1}^{n} \\left(1 + \\frac{1}{k}\\right)",
         "\\sum_{n=1}^{n + 1} n - \\prod_{k=1}^{n + 1} \\left(1 + \\frac{1}{k}\\right)"},
    };
    for (size_t i = 0; i < sizeof trees / sizeof *trees; i++)
        check(node_prints_back(trees[i][0], trees[i][1]), trees[i][0]);

    /* I valori scritti dalle dimostrazioni per induzione, con ogni tipo di
     * atomo: devono rileggersi come le espressioni da cui vengono. */
    static const char *const values[] = {
        "2^{n} n - \\frac{3}{n + 1}",
        "\\sqrt{n + 1} \\sqrt{n} + \\sqrt{2} + \\sqrt[3]{n + 5}",
        "\\ln\\left(n + 1\\right) \\ln 2 - \\ln\\left(n + 3\\right)^{2}",
        "\\left(-1\\right)^{n} n + F_{n + 1}^{2} - F_{2n}",
        "n! \\cdot H_{n} + \\left(2n\\right)!",
        "\\arctan\\left(\\frac{1}{n}\\right) + \\pi n",
        "\\left(n + 1\\right)^{n} a^{n} - \\frac{1}{3^{n}} + 6^{n}",
        "\\left(\\sum_{k=1}^{n} \\frac{1}{k^{2}}\\right)^{2} \\prod_{k=1}^{n} \\left(1 + \\frac{1}{k^{2}}\\right)",
        "\\frac{2^{2^{n}} + 1}{n^{2} + n + 1} - \\frac{5}{7} n^{3}",
    };
    for (size_t i = 0; i < sizeof values / sizeof *values; i++)
        check(value_prints_back(values[i]), values[i]);

    /* Il controllo delle formule scritte (13f) deve accorgersi di una
     * formula sbagliata. La terza coppia è l'errore che il controllo ha
     * trovato davvero: senza parentesi un \\prod inghiotte il fattore che lo
     * segue. */
    static const char *const pairs[][2] = {
        {"n + 1", "1 + n"},
        {"n + 1", "n + 2"},
        {"\\left(n + 1\\right)!", "\\prod_{k=1}^{n} k \\cdot \\left(n + 1\\right)"},
        {"\\left(n + 1\\right)!", "\\left(\\prod_{k=1}^{n} k\\right) \\cdot \\left(n + 1\\right)"},
        {"\\sum_{k=1}^{n + 1} k", "\\sum_{k=1}^{n} k + n"},
        {"\\sqrt{n} \\sqrt{n}", "n"},
    };
    static const int agree[] = {1, 0, 0, 1, 0, 1};
    for (size_t i = 0; i < sizeof pairs / sizeof *pairs; i++) {
        proof_reset();
        arena_used = arena_keep;
        atom_count = 0;
        failure = NULL;
        induction_var = 'n';
        domain_floor = 1;
        proof_add("%s", pairs[i][0]);
        Span left = {0, proof_length};
        proof_add("%s", pairs[i][1]);
        Span right = {left.to, proof_length};
        check(reread_equal(left, right, 1) == agree[i], pairs[i][1]);
    }

    /* Le frasi delle dimostrazioni per induzione: caso base, termini
     * aggiunti, ipotesi, conclusione. */
    static const char *const induction[][2] = {
        {"\\sum_{k=1}^{n} k = \\frac{n\\left(n + 1\\right)}{2}",
         "\\item \\emph{Base case}, $n = 1$. The left-hand side is $\\sum_{k=1}^{1} k = 1$ and the right-hand side is "
         "$\\frac{1 \\cdot \\left(1 + 1\\right)}{2} = 1$: the two values are equal."},
        {"\\sum_{k=1}^{n} k = \\frac{n\\left(n + 1\\right)}{2}",
         "\\item \\emph{Inductive hypothesis}. Assume the statement for some $n \\ge 1$: $\\sum_{k=1}^{n} k = \\frac{n \\cdot \\left(n + 1\\right)}{2}$."},
        {"\\sum_{k=1}^{n} k = \\frac{n\\left(n + 1\\right)}{2}", "\\item $\\sum_{k=1}^{n + 1} k = \\sum_{k=1}^{n} k + \\left(n + 1\\right)$."},
        {"\\sum_{k=1}^{n} k = \\frac{n\\left(n + 1\\right)}{2}",
         "\\item By the inductive hypothesis, $\\sum_{k=1}^{n} k = \\frac{1}{2}\\,n + \\frac{1}{2}\\,n^{2}$."},
        {"\\sum_{k=1}^{n} k = \\frac{n\\left(n + 1\\right)}{2}",
         "\\item With these replacements, both sides of the statement for $n + 1$ become $1 + \\frac{3}{2}\\,n + \\frac{1}{2}\\,n^{2}$."},
        {"\\prod_{k=1}^{n} \\left(1 + \\frac{1}{k}\\right) = n + 1",
         "= \\left(\\prod_{k=1}^{n} \\left(1 + \\frac{1}{k}\\right)\\right) \\cdot \\left(1 + \\frac{1}{n + 1}\\right)$."},
        {"\\prod_{k=1}^{n} \\left(1 + \\frac{1}{k}\\right) = n + 1",
         "the left-hand side for $n + 1$ becomes $\\frac{2 + 3\\,n + n^{2}}{1 + n}$ and the right-hand side becomes $2 + n$. "
         "They are equal: their difference, written as one fraction, has numerator $0$."},
        {"\\sum_{k=1}^{2n} \\left(-1\\right)^{k} k = n",
         "+ \\left(-1\\right)^{2n + 1} \\cdot \\left(2n + 1\\right) + \\left(-1\\right)^{2n + 2} \\cdot \\left(2n + 2\\right)$."},
        {"\\left(n + 1\\right)^{2} = n^{2} + 2n + 1",
         "\\item \\emph{Inductive step}. Here the inductive hypothesis is not needed. For $n + 1$ the statement is "
         "$\\left(n + 1 + 1\\right)^{2} = \\left(n + 1\\right)^{2} + 2\\left(n + 1\\right) + 1$, and written out both sides are $4 + 4\\,n + n^{2}$."},
        {"\\sum_{k=1}^{n} \\cos^{2}\\left(k x\\right) + \\sum_{k=1}^{n} \\sin^{2}\\left(k x\\right) = n",
         "expressions in that form are not written out below."},
    };
    for (size_t i = 0; i < sizeof induction / sizeof *induction; i++)
        check(proof_says(induction[i][0], induction[i][1]), induction[i][1]);

    /* Le frasi che collegano i fatti nelle dimostrazioni con quantificatori,
     * una per ogni combinazione: uguale o diverso, affermato o negato,
     * numeri o lettere, da solo o come conseguenza di un'ipotesi. Una frase
     * con "holds" al posto di "fails" sarebbe una dimostrazione sbagliata
     * anche con tutti i fatti giusti. */
    static const char *const sentences[][2] = {
        {"\\forall x \\in \\mathbb{Q}\\ (x + 1 = 1 + x)", "\\item $x + 1 = 1 + x$ holds: written out, the two sides are the same expression."},
        {"\\forall x \\in \\mathbb{Q}\\ (x + 1 \\neq x)",
         "\\item $x + 1 \\neq x$ holds: the difference of the two sides has the numerator $1$, a constant that is not zero."},
        {"\\lnot \\exists x \\in \\mathbb{Q}\\ (x + 1 = x)",
         "\\item $x + 1 = x$ fails: the difference of the two sides has the numerator $1$, a constant that is not zero."},
        {"\\forall x \\in \\mathbb{N}\\ (2 + 2 = 4)", "\\item $2 + 2 = 4$ holds: both sides are equal to $4$."},
        {"\\forall x \\in \\mathbb{N}\\ (2 + 2 \\neq 5)", "\\item $2 + 2 \\neq 5$ holds: the two sides are $4$ and $5$."},
        {"\\lnot \\forall x \\in \\mathbb{N}\\ (2 + 2 = 5)", "\\item $2 + 2 = 5$ fails: the two sides are $4$ and $5$."},
        {"\\lnot \\exists x \\in \\mathbb{N}\\ (2 + 2 \\neq 4)", "\\item $2 + 2 \\neq 4$ fails: both sides are equal to $4$."},
        {"\\forall x \\in \\mathbb{N}\\ (x^{2} + 1 \\neq 0)", "\\item The condition $x^{2} + 1 \\neq 0$ means $x^{2} + 1 \\neq 0$. The polynomial $x^{2} + 1$ has no rational root"},
        {"\\lnot \\exists x \\in \\mathbb{N}\\ (x^{2} + 1 = 0)", "\\item The equality $x^{2} + 1 = 0$ would mean $x^{2} + 1 = 0$. The polynomial $x^{2} + 1$ has no rational root"},
        {"\\lnot \\exists x \\in \\mathbb{N}\\ (x^{2} + 1 = 0)", "none of them is a root. So the equality never holds."},
        {"\\forall x \\in \\mathbb{Q}\\ \\forall y \\in \\mathbb{Q}\\ (2 x + 1 = 2 y + 1 \\implies x = y)",
         "\\item Suppose that $2 x + 1 = 2 y + 1$ holds, that is $2x - 2y = 0$. Now $2x - 2y = 2\\left(x - y\\right)$. "
         "The constant $2$ is not zero. Hence $x - y = 0$, that is $x = y$."},
        {"\\forall x \\in \\mathbb{Q}\\ \\forall y \\in \\mathbb{Q}\\ (x \\neq y \\implies 2 x + 1 \\neq 2 y + 1)",
         "\\item Suppose that $2 x + 1 \\neq 2 y + 1$ fails, that is $2x - 2y = 0$. Now $2x - 2y = 2\\left(x - y\\right)$. "
         "The constant $2$ is not zero. Hence $x - y = 0$, so $x \\neq y$ fails."},
        {"\\forall x \\in \\mathbb{Q} \\setminus \\{1\\}\\ \\forall y \\in \\mathbb{Q} \\setminus \\{1\\}\\ (\\frac{x}{x - 1} = \\frac{y}{y - 1} \\implies x = y)",
         "Suppose that $\\frac{x}{x - 1} = \\frac{y}{y - 1}$ holds, after clearing denominators, that is $-x + y = 0$."},
        {"\\lnot \\forall x \\in \\mathbb{N}\\ (2 + 2 = 5)", "\\item Take $x = 1$, an element of $\\mathbb{N}$."},
        {"\\forall x \\in \\mathbb{Z}\\ \\exists y \\in \\mathbb{Z}\\ (x = 2 y \\lor x = 2 y + 1)",
         "\\item Every integer is $2t + r$ for exactly one integer $t$ and one $r$ among $0, 1$: division by $2$ with remainder. So there are $2$ cases."},
        {"\\forall x \\in \\mathbb{Z}\\ \\exists y \\in \\mathbb{Z}\\ (x = 2 y \\lor x = 2 y + 1)",
         "\\item \\emph{Case $x = 2t + 1$}, with $t \\in \\mathbb{Z}$.\n\\begin{itemize}\n\\item We look for $y$ with $x = 2 y + 1$. Written out, this equation is $-2y = -2t$."},
        {"\\forall x \\in \\mathbb{N}\\ \\exists y \\in \\mathbb{N}_{0}\\ (x = 2 y + 1 \\lor x = 2 y + 2)",
         "\\item \\emph{Case $x = 2t$}, with $t \\in \\mathbb{N}$."},
        {"\\forall x \\in \\mathbb{N}\\ \\exists y \\in \\mathbb{N}_{0}\\ (x = 2 y + 1 \\lor x = 2 y + 2)",
         "\\item \\emph{Case $x = 2t + 1$}, with $t \\in \\mathbb{N}_{0}$."},
        {"\\forall n \\in \\mathbb{N}_{0}\\ \\left(\\sum_{k=1}^{n} k = \\frac{n\\left(n + 1\\right)}{2}\\right)",
         "\\item We prove $\\sum_{k=1}^{n} k = \\frac{n\\left(n + 1\\right)}{2}$ for every $n \\in \\mathbb{N}_{0}$.\n\\item By induction on $n$.\n"
         "\\begin{itemize}\n\\item \\emph{Base case}, $n = 0$. The left-hand side is $\\sum_{k=1}^{0} k = 0$"},
    };
    for (size_t i = 0; i < sizeof sentences / sizeof *sentences; i++)
        check(proof_says(sentences[i][0], sentences[i][1]), sentences[i][1]);

    /* La decisione "per ogni n" (13c), regola per regola: l'esito e, quando
     * la condizione fallisce, il primo n in cui fallisce. */
    static const struct { const char *expression; Requirement kind; long long base; int answer; long long where; } decisions[] = {
        {"n^{2} - 400", NEVER_ZERO, 1, 0, 20},
        {"n^{2} - 400", NEVER_ZERO, 21, 1, 0},
        {"2n - 41", NEVER_ZERO, 1, 1, 0},
        {"n^{2} - n + 1", ALWAYS_POSITIVE, 1, 1, 0},
        {"100 - n", ALWAYS_POSITIVE, 1, 0, 100},
        {"100 - n", NEVER_NEGATIVE, 1, 0, 101},
        {"\\left(n - 3\\right)^{2}", NEVER_NEGATIVE, 1, 1, 0},
        {"\\left(n - 3\\right)^{2}", ALWAYS_POSITIVE, 1, 0, 3},
        {"2^{n} - 4096", NEVER_ZERO, 1, 0, 12},
        {"2^{n} - 4097", NEVER_ZERO, 1, 1, 0},
        {"2 \\cdot 4^{n} - 3 \\cdot 2^{n} + 1", NEVER_ZERO, 1, 1, 0},
        {"4096 - 2^{n}", ALWAYS_POSITIVE, 1, 0, 12},
        {"2^{2^{n}} - 1", ALWAYS_POSITIVE, 1, 1, 0},
        {"2^{n} - n", NEVER_ZERO, 1, 1, 0},
        {"2^{n} - n", ALWAYS_POSITIVE, 1, 1, 0},
        {"2^{n} - n^{2}", NEVER_ZERO, 1, 0, 2},
        {"2^{n} - n^{2}", NEVER_ZERO, 5, 1, 0},
        {"2^{n} - 100 n - 1000", NEVER_ZERO, 1, 1, 0},
        {"2^{n} - 100 n - 1000", ALWAYS_POSITIVE, 1, 0, 1},
        {"2^{n} - 4096 n", NEVER_ZERO, 1, 0, 16},
        {"n^{3} 2^{n} - 3^{n}", NEVER_ZERO, 1, 1, 0},
        {"3^{n} - 2^{n} - n", NEVER_ZERO, 1, 0, 1},
        {"3^{n} - 2^{n} - n", ALWAYS_POSITIVE, 2, 1, 0},
        {"n! - 5", NEVER_ZERO, 1, -1, 0},
        {"\\cos\\left(\\frac{1}{2^{n}}\\right)", ALWAYS_POSITIVE, 1, 1, 0},
        {"\\sin\\left(\\frac{\\pi}{2^{n + 1}}\\right)", ALWAYS_POSITIVE, 1, 1, 0},
        {"2^{n} \\sin\\left(\\frac{3}{2^{n}}\\right)", ALWAYS_POSITIVE, 1, 1, 0},
        {"\\cos\\left(2\\right)", ALWAYS_POSITIVE, 1, 0, 1},
        {"\\cos\\left(2\\right)", NEVER_ZERO, 1, 1, 0},
        {"\\sin\\left(2\\right)", ALWAYS_POSITIVE, 1, 1, 0},
        {"\\sin\\left(4\\right)", ALWAYS_POSITIVE, 1, 0, 1},
        {"\\cos\\left(5\\right)", ALWAYS_POSITIVE, 1, 1, 0},
        {"\\sin\\left(-1\\right)", ALWAYS_POSITIVE, 1, 0, 1},
        {"\\cos\\left(\\frac{3 \\pi}{4}\\right)", ALWAYS_POSITIVE, 1, -1, 0},
        {"\\sin\\left(\\frac{\\pi}{2} + \\frac{\\pi}{2^{n + 1}}\\right)", ALWAYS_POSITIVE, 1, 1, 0},
        {"\\cos\\left(\\frac{\\pi}{2} + \\frac{\\pi}{2^{n + 1}}\\right)", ALWAYS_POSITIVE, 1, 0, 1},
        {"\\cos\\left(n\\right)", ALWAYS_POSITIVE, 1, -1, 0},
        {"\\cos^{2}\\left(n\\right)", NEVER_NEGATIVE, 1, 1, 0},
        {"\\cos^{2}\\left(n\\right)", ALWAYS_POSITIVE, 1, 1, 0},
        {"3 + \\sin\\left(n\\right) + \\sin\\left(n\\right)", ALWAYS_POSITIVE, 1, 1, 0},
        {"1 + \\sin\\left(n\\right) + \\sin\\left(n\\right)", ALWAYS_POSITIVE, 1, -1, 0},
        {"n \\sqrt{2} - 5 \\sqrt{2} + \\sqrt{3} n - 5 \\sqrt{3}", NEVER_ZERO, 1, 0, 5},
        {"n \\sqrt{2} - 5 \\sqrt{3}", NEVER_ZERO, 1, 1, 0},
        {"\\sqrt{n} + \\sqrt{n + 1}", NEVER_ZERO, 1, 1, 0},
        {"\\ln\\left(n + 1\\right)", NEVER_ZERO, 1, 1, 0},
        {"\\ln\\left(n\\right)", NEVER_ZERO, 1, 0, 1},
        {"\\sin\\left(2 n\\right)", NEVER_ZERO, 1, 1, 0},
        {"\\sin\\left(2 n - 6\\right)", NEVER_ZERO, 1, -1, 0},
        {"\\sin\\left(\\pi n\\right) + n", NEVER_ZERO, 1, 1, 0},
        {"n! + 1", ALWAYS_POSITIVE, 1, 1, 0},
        {"n", NEVER_ZERO, 0, 0, 0},
        {"\\arctan\\left(\\frac{1}{2}\\right) + \\arctan\\left(\\frac{1}{3}\\right) - \\frac{\\pi}{4}", NEVER_ZERO, 1, 0, 1},
        {"2\\cos\\left(\\frac{\\pi}{3}\\right) - 1", NEVER_ZERO, 1, -1, 0},
        {"\\arctan\\left(\\frac{1}{2}\\right) + \\arctan\\left(\\frac{1}{3}\\right)", NEVER_ZERO, 1, 1, 0},
        {"\\ln 2 + \\ln 3", ALWAYS_POSITIVE, 1, 1, 0},
        {"n \\ln 2 - \\ln 3", NEVER_ZERO, 1, -1, 0},
    };
    for (size_t i = 0; i < sizeof decisions / sizeof *decisions; i++) {
        long long where = 0;
        int answer = condition_answer(decisions[i].expression, decisions[i].kind, decisions[i].base, &where);
        check(answer == decisions[i].answer && (answer != 0 || where == decisions[i].where), decisions[i].expression);
    }

    /* Quando l'enunciato smette di essere definito, il verdetto dice dove. */
    Verdict later = prove("\\frac{2^{n} - 4096}{2^{n} - 4096} = 1", 'n', 0, 0);
    check(later.outcome == NOT_PROVED && strstr(later.reason, "n = 12 "), "the verdict should name the first n at which the statement is not defined");
    Verdict first = prove("\\log_{25 - n}\\left(25 - n\\right) = 1", 'n', 0, 0);
    check(first.outcome == NOT_PROVED && strstr(first.reason, "n = 24 "), "among several failing conditions the smallest n should be reported");

    /* E dice anche che cosa: una divisione, un logaritmo, una radice
     * dell'enunciato, oppure una divisione fatta dalla dimostrazione. */
    static const char *const reasons[][2] = {
        {"\\frac{n - 50}{n - 50} = 1", "per n = 50 l'enunciato non è definito: divisione per zero"},
        {"\\ln\\left(60 - n\\right) = \\ln\\left(60 - n\\right)", "per n = 60 l'enunciato non è definito: logaritmo di un numero non positivo"},
        {"\\sqrt{100 - n} \\cdot \\sqrt{100 - n} = 100 - n", "per n = 101 l'enunciato non è definito: radice di un numero negativo"},
        {"\\frac{2^{n} - 4096 n}{2^{n} - 4096 n} = 1", "per n = 16 l'enunciato non è definito: divisione per zero"},
        {"\\frac{\\prod_{k=1}^{n} \\left(k - 50\\right)}{\\prod_{k=1}^{n - 1} \\left(k - 50\\right)} = n - 50",
         "per n = 50 la dimostrazione divide un prodotto per un suo fattore, che lì vale zero"},
        {"\\ln\\left(\\cos 2\\right) = \\ln\\left(\\cos 2\\right)", "per n = 1 l'enunciato non è definito: logaritmo di un numero non positivo"},
    };
    for (size_t i = 0; i < sizeof reasons / sizeof *reasons; i++) {
        Verdict verdict = prove(reasons[i][0], 'n', 0, 0);
        check(verdict.outcome == NOT_PROVED && strcmp(verdict.reason, reasons[i][1]) == 0, reasons[i][1]);
    }

    /* Le radici reali che propongono i valori dei parametri, e il conto
     * esatto che li accetta o li scarta. */
    static const double cubic[] = {6, -7, 0, 1};
    double found[4];
    int real = real_roots(cubic, 3, found);
    check(real == 3 && found[0] > -3.000001 && found[0] < -2.999999 && found[1] > 0.999999 && found[1] < 1.000001 &&
          found[2] > 1.999999 && found[2] < 2.000001, "the real roots of x^3 - 7x + 6 should be -3, 1 and 2");
    int known = 1, x_atom;
    Poly parabola = expression_poly("x^{2} - 2", &x_atom);
    check(power_sum_sign(parabola, x_atom, 1, (Frac){3, 2}, &known) == 1 && power_sum_sign(parabola, x_atom, 1, (Frac){7, 5}, &known) == -1 && known,
          "the sign of x^2 - 2 at 3/2 and at 7/5 should be exact");
    Poly radical = expression_poly("\\sqrt{x} - 100", &x_atom);
    check(power_sum_sign(radical, x_atom, 2, (Frac){101, 1}, &known) == 1 && power_sum_sign(radical, x_atom, 2, (Frac){100, 1}, &known) == 0 &&
          power_sum_sign(radical, x_atom, 2, (Frac){99, 1}, &known) == -1 && known, "the sign of sqrt(x) - 100 where sqrt(x) is 101, 100 and 99");
    Poly huge = expression_poly("4000000000000 x^{2} - 9000000000001", &x_atom);
    check(power_sum_sign(huge, x_atom, 1, (Frac){3, 2}, &known) == -1 && power_sum_sign(huge, x_atom, 1, (Frac){1500001, 1000000}, &known) == 1 && known,
          "coefficients beyond a billion should not prevent an exact sign");

    /* Un commento tra parentesi dopo la formula non è una condizione: non
     * impedisce il verdetto FALSO, che però dichiara di averlo ignorato. Un
     * inciso che restringe i casi resta una condizione non letta. */
    static const struct { const char *tail; Outcome expected; int noted; } remarks[] = {
        {"per ogni $n\\ge 1$ (nucleo di Dirichlet).", REFUTED, 1},
        {"per ogni $n\\ge 1$ (formula di Gauss).", REFUTED, 1},
        {"for every $n\\ge 1$ (the formula of Gauss).", REFUTED, 1},
        {"per ogni $n\\ge 1$.", REFUTED, 0},
        {"per ogni $n\\ge 1$ (caso pari).", NOT_PROVED, 0},
        {"per ogni $n\\ge 1$ (n pari).", NOT_PROVED, 0},
        {"per ogni $n\\ge 1$ (per valori grandi).", NOT_PROVED, 0},
        {"per ogni $n\\ge 1$ (solo se dispari).", NOT_PROVED, 0},
        {"per ogni $n\\ge 1$ (formula di Gauss, 1786).", NOT_PROVED, 0},
        {"per ogni $n\\ge 1$ (con $a > 0$).", NOT_PROVED, 0},
        {"for every $n\\ge 1$ (even case).", NOT_PROVED, 0},
        {"for every $n\\ge 1$ (when n is large).", NOT_PROVED, 0},
        {"per ogni $n\\ge 1$ (nucleo di Dirichlet", NOT_PROVED, 0},

        /* Una lettera isolata potrebbe essere una variabile: nel dubbio
         * l'inciso resta una condizione, anche se qui è un genitivo. */
        {"for every $n\\ge 1$ (Gauss's formula).", NOT_PROVED, 0},
    };
    for (size_t i = 0; i < sizeof remarks / sizeof *remarks; i++) {
        char item[512];
        snprintf(item, sizeof item, " $\\displaystyle \\sum_{k=1}^{n} k = \\frac{n^{2} + n}{2} + 1$ %s\n", remarks[i].tail);
        Verdict verdict = item_verdict(item);
        check(verdict.outcome == remarks[i].expected && (verdict.note != NULL) == remarks[i].noted, remarks[i].tail);
    }
    Verdict noted = item_verdict(" $\\displaystyle \\sum_{k=1}^{n} k = \\frac{n^{2} + n}{2}$ per ogni $n\\ge 1$ (formula di Gauss).\n");
    check(noted.outcome == PROVED && noted.note == NULL, "a remark should not change a proof, nor be mentioned in it");

    /* L'insieme scelto per un "esiste" va descritto con le regioni giuste:
     * qui è T privato di S, cioè gli elementi che non stanno in S e stanno
     * in T. */
    Verdict chosen_set = prove("S \\subseteq T \\implies \\exists V \\left( S \\cup V = T \\land S \\cap V = \\emptyset \\right)", 'n', 0, 0);
    check(chosen_set.outcome == PROVED && chosen_set.proof && strstr(chosen_set.proof, "we take $V = (S^{c} \\cap T)$"),
          "the set chosen for an existence claim should be described as (S^c cap T)");

    /* Nessun passo delle dimostrazioni scritte fin qui deve aver fallito il
     * suo controllo indipendente. */
    check(unjustified_steps == 0, "every step of every written proof should pass its independent check");

    /* La riga del verdetto dice sempre se c'è un certificato verificato dal
     * verificatore indipendente, e se manca, quando si sa, perché. */
    static const char *const lines[][2] = {
        {"\\sum_{k=1}^{n} k = \\frac{n^{2} + n}{2}", "DIMOSTRATO per induzione su n >= 1; certificato verificato"},
        {"x \\prod_{k=1}^{n} \\frac{k + 1}{k} = x \\left(n + 1\\right)",
         "DIMOSTRATO per induzione su n >= 1; certificato verificato con un'ipotesi in più sui parametri"},
        {"\\sum_{k=1}^{n} \\ln\\left(1 + \\frac{1}{k}\\right) = \\ln\\left(n + 1\\right)",
         "DIMOSTRATO per induzione su n >= 1; senza certificato (l'enunciato o il passo induttivo usano qualcosa che la notazione dei certificati non ha)"},
        {"S \\cup T = T \\cup S", "DIMOSTRATO per ogni scelta degli insiemi; senza certificato"},
        {"\\forall x \\in \\mathbb{Z}\\ (x + 0 = x)", "DIMOSTRATO con le regole sui quantificatori; senza certificato"},
        {"\\sum_{k=1}^{n} k = \\frac{n^{2} + n}{2} + n - 1", "FALSO: non vale per n = 2"},
    };
    for (size_t i = 0; i < sizeof lines / sizeof *lines; i++) {
        char line[512];
        Verdict verdict = prove(lines[i][0], 'n', 0, 0);
        FILE *memory = tmpfile();
        check(memory != NULL, "a temporary file should be available");
        if (!memory) continue;
        verdict_print(memory, &verdict);
        rewind(memory);
        line[fread(line, 1, sizeof line - 1, memory)] = 0;
        fclose(memory);
        check(strcmp(line, lines[i][1]) == 0, lines[i][1]);
    }

    /* Un file che non esiste è un errore, non un file vuoto. */
    Report report;
    check(!prove_file("tests/engine/no_such_file.tex", &report, NULL, NULL, NULL), "a missing file should be reported");

    int checks = count + extra_checks;
    printf("%s: %d of %d engine checks passed\n", failures ? "FAIL" : "PASS", checks - failures, checks);
    return failures != 0;
}
