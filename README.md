# Operating_System_Assignment

## Description:

This project simulates a simplified operating system, focusing on core components such as:

*   **Process Management:** Process scheduling and dispatching.
*   **Memory Management:** Virtual memory allocation and deallocation.
*   **System Calls:** Providing an interface between applications and the kernel.

## Key Components:

*   **`scheduler.c` and `sched.h`:** Implements the scheduling algorithm, using a Multi-Level Queue (MLQ).
*   **`queue.c` and `queue.h`:** Provides the queue data structure for managing processes.
*   **`mm.c`, `mm-vm.c`, `mm-memphy.c`, `mm.h`, `os-mm.h`:** Implements the paging-based virtual memory management system, including virtual-to-physical memory mapping and swapping.
*   **`libmem.c`, `libmem.h`:**  Provides a library for user processes to interact with the memory management system (e.g., `alloc`, `free`, `read`, `write`).
*   **`syscall.c`, `syscall.h`:** Defines the interface and mechanism for executing system calls.
*   **`sys_killall.c`:** (Example) Implements a specific system call (`killall`), allowing processes to be terminated.
*   **`cpu.c`, `cpu.h`:** Simulates the CPU.
*   **`timer.c`, `timer.h`:** Simulates the timer.
*   **`common.h`:** Defines common structures and functions.
*   **`os.c`:** Contains the `main` function to initialize and run the operating system.
*   **`loader.c`**: Loads programs.
*   **`bitopts.h`**: Bitwise operations.
*   **`os-cfg.h`**: Configuration settings.

## How to Compile and Run:

1.  Use the `make all` command to compile the source code.
2.  Run the simulated operating system with the command: `./os <configure_file>`, where `<configure_file>` is the path to the configuration file.

## System Configuration (Configure File):

The configuration file (`<configure_file>`) allows defining system parameters, including:

*   Time slice for each process.
*   Number of CPUs.
*   Number of processes to run and their paths, priorities.
*   (Optional) RAM and SWAP sizes (if MM_FIXED_MEMSZ is not defined).

## System Calls:

Applications can interact with the kernel through system calls defined in `syscall.h` and implemented in `src/sys_xxxhandler.c`.
