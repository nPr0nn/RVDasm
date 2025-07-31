# RVDasm - A Simple RISC-V Disassembler

**RVDasm** is a lightweight, command-line disassembler for 32-bit RISC-V ELF (Executable and Linkable Format) files.  
It's written in C and designed to be a simple, educational tool for understanding the basics of executable file formats and assembly language.

---

## Features

- **ELF Header Parsing**  
  Displays detailed information from the ELF header.

- **Section Header Listing** (`-h`)  
  Prints a formatted list of all section headers in the executable.

- **Symbol Table Display** (`-t`)  
  Shows the symbol table, including function and object names.

- **RISC-V Disassembly** (`-d`)  
  Disassembles the `.text` section into human-readable RISC-V assembly instructions.

- **Hexdump** (`-x`)  
  Provides a full hexdump of the entire file for in-depth analysis.

---

## Building

The project uses a standard `Makefile` for easy compilation.  
Make sure you have `gcc` and `make` installed on your system.

To build the project, run:

```bash
make
````

This will compile the source files and create an executable named `mydisasm` in the same directory.

To clean up build files:

```bash
make clean
```

---

## Usage

The tool is operated via command-line flags.
Basic syntax:

```bash
./mydisasm <flag> <filename>
```

### Examples

* **Display Section Headers** (`-h`)

  ```bash
  ./mydisasm -h your_executable_file
  ```

* **Display Symbol Table** (`-t`)

  ```bash
  ./mydisasm -t your_executable_file
  ```

* **Disassemble the .text Section** (`-d`)

  ```bash
  ./mydisasm -d your_executable_file
  ```

* **Display Hexdump of the File** (`-x`)

  ```bash
  ./mydisasm -x your_executable_file
  ```

---

## Sample Binaries

The `bin/` directory contains a few pre-compiled 32-bit RISC-V ELF binaries (`.x` files) for testing.

Example:

```bash
./mydisasm -d bin/hello.x
```

---

## License

This project is licensed under the **MIT License**.
See the [LICENSE](LICENSE) file for details.
