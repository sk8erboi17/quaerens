12 / 09 / 2026 - Start project

A prover and solver for mathematical statements written in LaTeX, in dependency-free C: it reads the statements, proves or solves them with the sign table, and re-checks every result with an independent certificate checker.
It produces a `.tex`/PDF document with proofs, solutions and plots (drawn by a small matplotlib rewritten in C), backed by tests and fuzz scripts that keep it from ever proving a falsehood.
