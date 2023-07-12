# operating-system

An x86-64 (WIP) operating system written in pure C17.
So far, only the kernel has been made.

## Features

The kernel has the following implemented:

* UART (com1 on x86_64, pl011 on aarch64, uart8502 on riscv64) drivers
* Interrupt Descriptor Table and Vector Allocation
* Physical Memory Buddy Allocator using a vmemmap of `struct page`
* General memory allocation with kmalloc() using a slab allocator

The following is currently being worked on.
* Real Time Clock
* Page Fault Interrupt
* Keyboard (ps2) driver
* Parsing ACPI Tables
* Local APIC and I/O APIC
* Local APIC Timer
* PCI(e) drivers

In addition, the project has a very broad standard library that includes:

* Abstract `printf()` formatter
  * Used to implement `kprintf()` and `format_to_string()` among many use cases
* Abstract `strftime()` formatter
  * Doesn't support timezones (atm)
  * Used to implement a very basic `strftime()`
* ASCII APIs
* Alignment APIs
  * Defines and provides general-purpose APIs to check for alignment
* Integer Conversion APIs
  * Can convert to and from strings, with the following configurable integer formats:
    * Binary (with `0b` prefix)
    * Octal (with `0` or `0o` prefix)
    * Decimal
    * Hexadecimal (with `0x` prefix)
    * Alpbabeti (with `0a` prefix)

Among many others

## Running
operating-system currently supports the `x86_64`, `aarch64` (`arm64`), and `riscv64` architectures.
`x86_64` is the most feature complete, while `arm64` and `riscv64` are catching up

To build and run, only clang and ld are needed, except for `x86_64`, which
needs a cross compiler, such as `x86_64-elf-gcc` available on homebrew, because
sysv-abi support is needed.

Otherwise, building for `riscv64` and `arm64` can be done natively with clang/ld
directly from llvm (not apple)

Run using:

```make clean && make run ARCH=<arch>```
