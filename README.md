# RISC-V Transient Execution Attacks

This repository holds all the work-in-progress code used to check if RISC-V implementations are susceptible to transient execution attacks (TEAs).

# Target Configurations

```

T-Head TH1520 SoC (quad XuanTie C910 cores) of Sipeed Lichee Pi 4A SBC

```

# Implemented Attacks

The following attacks are implemented within the repo.

## Spectre-BCB (Bounds Check Bypass) or Spectre-v1 [1]
   * Spectre-BCB.c
## Spectre-BTI (Branch Target Injection) or Spectre-v2 [1]
   * Spectre-BTI.c
## Spectre-RSB (Return Stack Buffer attack) or Spectre-v5 [2]
   * Spectre-RSB.c + stack.S
   * May not succeed on machines w/o RAS (Return Address Stack) or something similar.
## CVE-2018-3639: Spectre-SSB (Speculative Store Bypass) or Spectre-v4
   * Spectre-SSB
   * Will not succeed on machines w/o MDP (Memory Dependence Prediction) mechanisms.

# WIP / Not Completed Attacks

The following attacks are in-progress and are not working yet.

# Building the tests

To build you need to run `make`

# Running the Tests

Built "baremetal" binaries can directly run on targets that are specified above.

# References

[1] P. Kocher, D. Genkin, D. Gruss, W. Haas, M. Hamburg, M. Lipp, S. Mangard, T. Prescher, M. Schwarz, and Y. Yarom, “Spectre attacks: Exploiting speculative execution,” ArXiv e-prints, Jan. 2018

[2] E. M. Koruyeh, K. N. Khasawneh, C. Song, N. Abu-Ghazaleh, “Spectre Returns! Speculation Attacks using the Return Stack Buffer,” 12th USENIX Workshop on Offensive Technologies, 2018
