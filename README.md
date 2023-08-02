# operating-system

An  operating system written in C17 (gnu17), for x86_64, aarch64, and riscv64.

So far, only the kernel is being worked on.

## Features

The kernel has the following implemented:

* UART drivers; uart8250 on x86_64, riscv64 and pl011 on aarch64
* GDT, IDT, and support for dynamic vector allocation
* Physical Memory Buddy Allocator using a vmemmap of `struct page`
* General memory allocation with kmalloc() using a slab allocator
* RTC (google,goldfish-rtc on riscv64) Clock and HPET on x86_64
* Page Fault Interrupt and other exceptions
* Keyboard (ps2) driver
* Parsing ACPI Tables and Flattened Device Tree (when available)
* Local APIC and I/O APIC
* Local APIC Timer
* PCI(e) device detection

The following is currently being worked on.
* AHCI, SATA, IDE controllers to read disk
* VirtIO drivers to detect devices when PCI(e) isn't available (as is the case on riscv64)
* Filesystem following VFS model
* Network Card drivers
* SMP support to run multiple cores
* Processes, threads, and scheduler to run tasks

In addition, the project has a very broad standard library that includes:

* Abstract `printf()` formatter
  * Used to implement `kprintf()` and `format_to_string()` among many use cases
* Abstract `strftime()` formatter
  * Doesn't support timezones (atm)
  * Used to implement a very basic `kstrftime()`
* ASCII APIs
* Alignment APIs
  * Defines and provides general-purpose APIs to check for alignment
* Integer Conversion APIs
  * Can convert to and from strings, with the following configurable integer formats:
    * Binary (with `0b` prefix)
    * Octal (with `0` or `0o` prefix)
    * Decimal
    * Hexadecimal (with `0x` prefix)
    * Alpbabetic (with `0a` prefix)

Among many others

## Running
operating-system currently supports the `x86_64`, `aarch64` (`arm64`), and `riscv64` architectures.
`x86_64` is the most feature complete, while `arm64` and `riscv64` are catching up

To build and run, only clang and ld are needed, except for `x86_64`, which
needs a cross compiler, such as `x86_64-elf-gcc` available on homebrew, because
sysv-abi support is needed but is not provided for by clang.

Otherwise, building for `riscv64` and `arm64` can be done natively with clang/ld
directly from llvm (not apple)

Run using:

```make clean && make run ARCH=<arch>```
