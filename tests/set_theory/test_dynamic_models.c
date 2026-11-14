/* Modelli dinamici: le cardinalita e le tabelle non dipendono dal prefisso
 * esaustivo o dalla larghezza di un byte. Gli oracoli sono l'identita e la
 * definizione della parte vuota; ogni modello passa anche dal checker.
 *
 * Compilazione ed esecuzione dalla cartella principale:
 * cc -std=c11 -O2 -Wall -Wextra -Werror -o /tmp/test_dynamic_models \
 *     tests/set_theory/test_dynamic_models.c && /tmp/test_dynamic_models
 */
#include "../statement_checks.h"

/* Legge dati per il motore senza cercare prima un modello piu piccolo. */
static const Sentence *read_model_statement(const char *text) {
    arena_used = arena_keep; failure = syntax_error = NULL;
    cursor = text; next_token();
    const Sentence *root = abstract_statement();
    declared_symbols = symbol_count;
    return syntax_error || failure ? NULL : root;
}

int main(void) {
    engine_init();
    Model m = {0};
    const Sentence *root = read_model_statement("(f \\colon S \\to S) \\implies \\forall x \\in S\\ (f(x) = x)");
    int passed = check_result(root != NULL, "an identity condition is parsed as data");
    if (!root) return 1;
    if (!model_dimensions(&m)) return 1;
    m.dimension[0].size = 257;
    unsigned long long work = 0;
    int found = model_sat(root, 1, &m, &work);
    int exact = found;
    for (size_t i = 0; i < 257 && exact; i++) exact = m.table[0][i] == i;
    passed = check_result(exact, "SAT constructs the exact 257-element identity without byte truncation") && passed;
    if (found) {
        cert_abstract_head("example", root, &m);
        cert_add("table "); cert_symbol(0);
        for (size_t i = 0; i < 257; i++) cert_add(" %zu", m.table[0][i]);
        cert_add("\nend\n");
        const char *text = certificate_text, *why;
        passed = check_result(!certificate_broken && ck_check(&text, &why, 0) == CK_ACCEPTED,
                              "the standalone checker independently verifies the large extracted model") && passed;
    }
    model_clear(&m);

    /* Una tabella binaria esatta supera anche i vecchi testi statici.
     * L'oracolo e f(i,j)=i; il documento elenca tutte le celle, il checker
     * ricontrolla tutti i valori, senza condividere il conto del motore. */
    root = read_model_statement("(f \\colon S \\times S \\to S) \\implies "
                                "\\forall x,y \\in S\\ (f(x,y) = x)");
    if (!model_dimensions(&m)) return 1;
    m.dimension[0].size = 300;
    if (root && model_allocate(&m)) {
        for (size_t c = 0; c < 90000; c++) m.table[0][c] = c % 300;
        proof_reset(); proof_growable = 1;
        put_model(&m);
        passed = check_result(!proof_broken && proof_length > PROOF_BYTES,
                              "a complete model document grows beyond the old text buffer") && passed;
        cert_abstract_head("example", root, &m);
        cert_add("table "); cert_symbol(0);
        for (size_t c = 0; c < 90000; c++) cert_add(" %zu", m.table[0][c]);
        cert_add("\nend\n");
        const char *text = certificate_text, *why;
        passed = check_result(!certificate_broken && certificate_length > CERTIFICATE_BYTES &&
                              ck_check(&text, &why, 0) == CK_ACCEPTED,
                              "a model certificate grows beyond its old buffer and is checked in full") && passed;
        proof_reset(); model_clear(&m);
    } else passed = check_result(0, "a large binary table can be represented") && passed;

    /* La parte e effettivamente legata dentro la condizione, non una
     * tabella dichiarata nell'intestazione della richiesta di esempio. */
    root = read_model_statement("\\exists S\\ (\\exists X \\subseteq S\\ \\forall x \\in S\\ (x \\notin X))");
    if (root) {
        if (!model_dimensions(&m)) return 1;
        m.dimension[0].size = 257; work = 0;
        found = model_sat(root, 1, &m, &work);
        passed = check_result(found, "SAT supports subsets of a 257-element domain") && passed;
        model_clear(&m);
    } else passed = check_result(0, "a subset condition is parsed") && passed;

    /* La richiesta falsa non deve diventare vera usando il dominio vuoto.
     * L'identita non ha punti non fissi in un insieme non vuoto. */
    Verdict v = prove("\\exists S \\neq \\emptyset\\ \\exists f \\colon S \\to S\\ "
                      "(\\forall x \\in S\\ (f(x) = x \\land f(x) \\neq x))", 'n', 0, 0);
    passed = check_result(v.outcome != PROVED, "an impossible nonempty example is never proved by vacuity") && passed;
    v = prove("\\forall f \\in S^{S}\\ \\exists x \\in S\\ (f(x) = f(x))", 'n', 0, 0);
    passed = check_result(v.outcome == REFUTED && v.certificate && strstr(v.certificate, "sort S 0\n") &&
                          v.proof && strstr(v.proof, "f = \\emptyset") && !strstr(v.proof, "$$"),
                          "the empty countermodel displays its empty function explicitly") && passed;
    root = read_model_statement("\\exists S \\neq \\emptyset\\ (\\forall x \\in S\\ (x = x))");
    if (root) {
        size_t *values = model_variables();
        if (!model_dimensions(&m)) return 1;
        m.dimension[0].size = 0; model_steps = 0;
        passed = check_result(!work_allocation_failed && !model_holds(&m, root, values),
                              "the certificate statement retains explicit nonemptiness") && passed;
    } else passed = check_result(0, "an explicit nonempty example is parsed") && passed;

    /* Budget insufficiente: nessuna tabella parziale diventa un risultato. */
    root = read_model_statement("(f \\colon S \\to S) \\implies \\forall x \\in S\\ (f(x) = x)");
    unsigned long long saved_work = model_work_limit;
    if (!model_dimensions(&m)) return 1;
    model_work_limit = 1; m.dimension[0].size = 257; work = 0;
    passed = check_result(root && !model_sat(root, 1, &m, &work), "work exhaustion returns no model") && passed;
    model_work_limit = saved_work; model_clear(&m);
    size_t saved_memory = model_memory_limit;
    if (!model_dimensions(&m)) return 1;
    model_memory_limit = 1024; m.dimension[0].size = 257; work = 0;
    passed = check_result(!model_sat(root, 1, &m, &work), "memory exhaustion returns no model") && passed;
    model_memory_limit = saved_memory; model_clear(&m);

    /* Visite scartate e ultima coordinata devono consumare lavoro, anche
     * quando la somma non entra in un ciclo praticabile. Il dato e vero in
     * ogni modello: esaurire il budget non puo produrre un controesempio. */
    Sentence truth = {.kind=S_TRUE};
    sort_count = 2; symbol_count = 0;
    sort_data[0].kind = sort_data[1].kind = SORT_PLAIN;
    saved_work = model_work_limit; model_work_limit = 100; work = 0;
    found = model_sat_total(&truth,0,&m,(size_t)-1,&work,0);
    passed = check_result(!found && work >= model_work_limit,
                          "dimension enumeration respects its work budget even for an unaddressably large total") && passed;
    model_work_limit = saved_work; model_clear(&m);
    passed = check_result(unjustified_steps == 0, "all reported models pass independent verification") && passed;
    return passed ? 0 : 1;
}
