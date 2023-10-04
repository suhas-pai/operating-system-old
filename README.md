# operating-system

An  operating system written in C17 (gnu17), for x86_64, aarch64, and riscv64.

So far, only the kernel is being worked on.

## Features

The kernel has the following implemented:
* UART drivers; uart8250 on x86_64, riscv64 and pl011 on aarch64
* Physical Memory Buddy Allocator using a vmemmap of `struct page`
* General memory allocation with kmalloc() using a slab allocator
* RTC (google,goldfish-rtc on riscv64) and LAPIC Timer, HPET on x86_64
* Keyboard (ps2) driver
* Parsing ACPI Tables and Flattened Device Tree (when available)
* Local APIC and I/O APIC
* PCI(e) Device Initialization with drivers

The following is currently being worked on.
* AHCI, SATA, IDE controllers to read disk
* VirtIO drivers to detect devices when PCI(e) isn't available (as is the case on riscv64)
* Filesystem following VFS model
* Network Card drivers
* SMP support to run multiple cores
* Processes, threads, and scheduler to run tasks

In addition, this project has a broad standard library that includes:

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

Several options are available to configure launching in QEMU:
  * `MEM` to configure memory size (in QEMU units) (By Default `4G`)
  * `SMP` to configure amount of cpus (By Default 4)
  * `DEBUG` to allow starting in QEMU's debugger mode (By default 0)
  * `CONSOLE` to allow starting in QEMU's console mode (By default 0)

To build and run, only clang and ld.lld are needed (from LLVM).
Run using:

```make clean && make run ARCH=<arch> MEM=<mem> SMP=<smp>```
