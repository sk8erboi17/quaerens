12 / 09 / 2026 - Start project

14 / 11 / 2026 - First week completed: sign tables, calculus and abstract set theory, with SAT counterexamples and a saturation prover, all re-checked by the independent checker.

A prover and solver for mathematical statements written in LaTeX, in dependency-free C: it reads the statements, proves or solves them with the sign table, and re-checks every result with an independent certificate checker.
It produces a `.tex`/PDF document with proofs, solutions and plots (drawn by a small matplotlib rewritten in C), backed by tests and fuzz scripts that keep it from ever proving a falsehood.
