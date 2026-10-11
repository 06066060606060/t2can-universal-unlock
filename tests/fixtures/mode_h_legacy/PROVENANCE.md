# Mode H reference fixtures

These five headers are byte-identical copies of completed T2CAN-Universal-v3.28.3-LP_YL. They are independent test oracles, never production includes.

`test_mode_h_equivalence.cpp` compares the current production engine with the frozen Rev4 engine, including every named output/state/config member and persisted layout. Historical standalone Rev1/Rev2/Rev3 regression tests retain their original assertions against these fixtures. Current Rev4 regression tests include `mode_h_pure.h`.

`test_mode_h_module_contract.py` pins fixture SHA-256 hashes and rejects legacy production includes. Do not edit fixtures alongside the engine to make comparisons pass.
