#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "elf_definitions.h"

#define RISCV_DISASSEMBLER_IMPL
#include "riscv_disassembler.h"

#define HEXDUMP_IMPL
#include "hexdump.h"

// @brief Prints a formatted error message to stderr and exits the program.
// @param message The error message to print.
 void exit_with_error(const char *message);

// Function Prototypes
void print_section_headers(const Elf32_Ehdr *ehdr, const Elf32_Shdr *shdr, const char *shstrtab);
void print_symbol_table(const Elf32_Sym *symtab, int count, const char *strtab, const Elf32_Shdr *shdr, const char *shstrtab);
void print_file_header(const char *filename, const Elf32_Ehdr *ehdr);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <flag> <filename>\n", argv[0]);
        fprintf(stderr, "Flags:\n");
        fprintf(stderr, "  -h: Display section headers\n");
        fprintf(stderr, "  -t: Display symbol table\n");
        fprintf(stderr, "  -d: Disassemble .text section\n");
        fprintf(stderr, "  -x: Display a hexdump of the file\n");
        return EXIT_FAILURE;
    }

    const char *flag = argv[1];
    const char *filename = argv[2];

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        exit_with_error("Could not open file.");
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        exit_with_error("Could not get file stats.");
    }
    long file_size = st.st_size;
    unsigned char *file_contents = malloc(file_size);
    if (!file_contents) {
        close(fd);
        exit_with_error("Could not allocate memory for file.");
    }

    if (read(fd, file_contents, file_size) != file_size) {
        free(file_contents);
        close(fd);
        exit_with_error("Could not read file.");
    }
    close(fd);

    // --- Start ELF Parsing ---
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)file_contents;
    if (memcmp(ehdr->e_ident, "\x7f""ELF", 4) != 0) {
        // For hexdump, we don't need to exit if it's not ELF.
        if (strcmp(flag, "-x") != 0) {
            exit_with_error("Not a valid ELF file.");
        }
    }

    if (strcmp(flag, "-x") == 0) {
        hexdump(filename, file_contents, file_size);
        free(file_contents);
        return EXIT_SUCCESS;
    }

    // The rest of the flags require a valid ELF file
    Elf32_Shdr *shdr = (Elf32_Shdr *)(file_contents + ehdr->e_shoff);
    const char *shstrtab = (const char *)(file_contents + shdr[ehdr->e_shstrndx].sh_offset);

    print_file_header(filename, ehdr);

    if (strcmp(flag, "-h") == 0) {
        print_section_headers(ehdr, shdr, shstrtab);
    } else if (strcmp(flag, "-t") == 0 || strcmp(flag, "-d") == 0) {
        Elf32_Shdr *symtab_hdr = NULL;
        Elf32_Shdr *strtab_hdr = NULL;
        Elf32_Shdr *text_hdr = NULL;

        for (int i = 0; i < ehdr->e_shnum; i++) {
            if (shdr[i].sh_type == 2 /* SHT_SYMTAB */) {
                symtab_hdr = &shdr[i];
            }
            if (strcmp(&shstrtab[shdr[i].sh_name], ".strtab") == 0) {
                strtab_hdr = &shdr[i];
            }
             if (strcmp(&shstrtab[shdr[i].sh_name], ".text") == 0) {
                text_hdr = &shdr[i];
            }
        }

        if (!symtab_hdr || !strtab_hdr) {
            exit_with_error("Could not find symbol table or string table.");
        }

        Elf32_Sym *symtab = (Elf32_Sym *)(file_contents + symtab_hdr->sh_offset);
        int sym_count = symtab_hdr->sh_size / symtab_hdr->sh_entsize;
        const char *strtab = (const char *)(file_contents + strtab_hdr->sh_offset);
        
        if (strcmp(flag, "-t") == 0) {
             print_symbol_table(symtab, sym_count, strtab, shdr, shstrtab);
        } else { // -d
            if (!text_hdr) {
                 exit_with_error("Could not find .text section.");
            }
            disassemble_text_section(file_contents, text_hdr, symtab, sym_count, strtab);
        }

    } else {
        fprintf(stderr, "Invalid flag: %s\n", flag);
    }

    free(file_contents);
    return EXIT_SUCCESS;
}

void exit_with_error(const char *message) {
    fprintf(stderr, "Error: %s\n", message);
    exit(EXIT_FAILURE);
}

void print_file_header(const char *filename, const Elf32_Ehdr *ehdr) {
    printf("\n%s:     file format ", filename);
    switch(ehdr->e_ident[4]) { // EI_CLASS
        case 1: printf("elf32-"); break;
        case 2: printf("elf64-"); break;
        default: printf("elf-unknown-"); break;
    }
    switch(ehdr->e_machine) {
        case 0xF3: printf("riscv\n"); break; // EM_RISCV
        case 0x03: printf("x86\n"); break;   // EM_386
        default: printf("unknown-machine\n"); break;
    }
}

void print_section_headers(const Elf32_Ehdr *ehdr, const Elf32_Shdr *shdr, const char *shstrtab) {
    printf("\nSections:\n");
    printf("  [Nr] Name              Type            Addr     Off    Size   ES Flg Lk Inf Al\n");

    for (int i = 0; i < ehdr->e_shnum; i++) {
        printf("  [%2d] %-17s %-15x %08x %06x %06x %02x %3x %2d %3d %2d\n",
               i,
               &shstrtab[shdr[i].sh_name],
               shdr[i].sh_type,
               shdr[i].sh_addr,
               shdr[i].sh_offset,
               shdr[i].sh_size,
               shdr[i].sh_entsize,
               (unsigned int)shdr[i].sh_flags,
               shdr[i].sh_link,
               shdr[i].sh_info,
               shdr[i].sh_addralign);
    }
}

void print_symbol_table(const Elf32_Sym *symtab, int count, const char *strtab, const Elf32_Shdr *shdr, const char *shstrtab) {
    printf("\nSYMBOL TABLE:\n");
    printf("   Value  Size Type    Bind   Vis      Ndx Name\n");

    for (int i = 0; i < count; i++) {
        const char *sec_name = "";
        if (symtab[i].st_shndx < 0xFF00) { // SHN_LORESERVE
             sec_name = &shstrtab[shdr[symtab[i].st_shndx].sh_name];
        } else if (symtab[i].st_shndx == 0xFFF1) { // SHN_ABS
            sec_name = "ABS";
        } else {
            sec_name = "UND";
        }

        printf("%08x %5d %-7s %-6s %-8s %-3s %s\n",
               symtab[i].st_value,
               symtab[i].st_size,
               "NOTYPE", // Simplified
               "GLOBAL", // Simplified
               "DEFAULT", // Simplified
               sec_name,
               &strtab[symtab[i].st_name]);
    }
}
