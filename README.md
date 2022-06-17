# Arm64-Simulator
2022-06-17

This is a portable simulator for Arm64 machine instructions.  It reads executable files, such as those in the "Test-exes" subdirectory, and loads them into a memory array.  It then provides single-stepping through the executable's instructions via a simulated Fetch-Decode-Execute cycle, and allows inspection of the CPU register contents after any or every instruction.

The test executables are expected to be assembled on a Raspberry Pi running a 64-bit Linux operating system such as 64-bit RaspiOS.  Source code and a Makefile are included, mostly to illustrate how to create additional test executables as desired.
