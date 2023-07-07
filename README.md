# operating-system

An x86-64 (WIP) operating system written in pure C17.
So far, only the kernel has been made.

## Features

The kernel has the following implemented:

* UART (com1 on x86_64, pl011 on aarch64) drivers
* Interrupt Descriptor Table and Vector Allocation
* Physical Memory Buddy Allocator using a vmemmap of `struct page`
* General memory allocation with kmalloc()

The following is presently being worked on.
* Real Time Clock
* Page Fault Interrupt
* Keyboard (ps2) driver
* Parsing ACPI Tables
* Local APIC and I/O APIC
* Local APIC Timer
* PCI(e) drivers

In addition, the project has a very broad standard library that includes:

* Abstract Allocator
  * Used to implement `malloc()` and `kmalloc()`
* Abstract `printf()` formatter
  * Used to implement `kprintf()` and `format_to_string()` among many use cases
* Abstract `strftime()` formatter
  * Doesn't support timezones (atm)
  * Used to implement a very basic `strftime()`
* ASCII APIs
* Alignment APIs
  * Defines and provides general-purpose APIs to check for alignment
* Integer Conversion APIs
  * Can convert from and to strings, with the following configurable integer formats:
    * Binary (with `0b` prefix)
    * Octal (with `0` prefix)
    * Decimal (with `0d` prefix)
    * Hexadecimal (with `0x` prefix)
* Macro APIs
  * General purpose macros used extensively throughout the project for:
    * Performance
    * Semantics

Among many others

## Running
operating-system currently supports two architectures: x86_64 and aarch64
(riscv64 being worked on but not runnable).

To build and run for an architecture, the compiler `$ARCH-elf-gcc` is
needed.
Run using:

```make clean && make run ARCH=<selected-arch>```
