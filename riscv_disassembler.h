#ifndef RISCV_DISASSEMBLER_H
#define RISCV_DISASSEMBLER_H

#include <stdint.h>
#include "elf_definitions.h"


/**
 * @brief Disassembles the .text section of an ELF file.
 * * @param file_contents Pointer to the raw bytes of the ELF file.
 * @param text_header Pointer to the section header for the .text section.
 * @param sym_tab Pointer to the symbol table.
 * @param sym_count Number of symbols in the symbol table.
 * @param str_tab Pointer to the string table for symbol names.
 */
void disassemble_text_section(const unsigned char *file_contents, const Elf32_Shdr *text_header, const Elf32_Sym *sym_tab, int sym_count, const char *str_tab);

#ifdef RISCV_DISASSEMBLER_IMPL

#include <stdio.h>
#include <string.h>


// Helper to get register ABI name
static inline const char* get_register_name(uint32_t n) {
    const char *registers[] = {
        "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
        "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
        "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
        "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
    };
    if (n < 32) {
        return registers[n];
    }
    return "unk";
}

// Helper to extract bits from an instruction
static inline uint32_t extract_bits(uint32_t instruction, int start, int len) {
    return (instruction >> start) & ((1 << len) - 1);
}

// Find a symbol name by its address
static inline const char* find_symbol_by_addr(uint32_t addr, const Elf32_Sym *sym_tab, int sym_count, const char *str_tab) {
    for (int i = 0; i < sym_count; i++) {
        if (sym_tab[i].st_value == addr && ELF_ST_TYPE(sym_tab[i].st_info) != 0) {
            return &str_tab[sym_tab[i].st_name];
        }
    }
    return NULL;
}

// Disassembles a single instruction
static void disassemble_instruction(uint32_t instruction, uint32_t addr, const Elf32_Sym *sym_tab, int sym_count, const char *str_tab) {
    uint32_t opcode = extract_bits(instruction, 0, 7);
    uint32_t rd = extract_bits(instruction, 7, 5);
    uint32_t funct3 = extract_bits(instruction, 12, 3);
    uint32_t rs1 = extract_bits(instruction, 15, 5);
    uint32_t rs2 = extract_bits(instruction, 20, 5);
    uint32_t funct7 = extract_bits(instruction, 25, 7);

    // Correct immediate value extraction
    int32_t imm_i = (int32_t)(instruction) >> 20;
    int32_t imm_s = ((int32_t)extract_bits(instruction, 25, 7) << 5) | extract_bits(instruction, 7, 5);
    if (imm_s & 0x800) imm_s -= 0x1000; // Sign extend
    int32_t imm_b = ((int32_t)(instruction & 0x80000000) >> 19) | ((instruction & 0x80) << 4) | ((instruction >> 20) & 0x7e0) | ((instruction >> 7) & 0x1e);
    if (imm_b & 0x1000) imm_b -= 0x2000; // Sign extend
    int32_t imm_u = (int32_t)(instruction & 0xfffff000);
    int32_t imm_j = ((int32_t)(instruction & 0x80000000) >> 11) | (instruction & 0xff000) | ((instruction >> 9) & 0x800) | ((instruction >> 20) & 0x7fe);
    if (imm_j & 0x100000) imm_j -= 0x200000; // Sign extend

    printf("   %05x:\t%08x\t", addr, instruction);

    switch (opcode) {
        case 0x33: // R-type
            switch (funct3) {
                case 0x0: printf(funct7 == 0x20 ? "sub\t" : "add\t"); break;
                case 0x1: printf("sll\t"); break;
                case 0x2: printf("slt\t"); break;
                case 0x4: printf("xor\t"); break;
                case 0x5: printf(funct7 == 0x20 ? "sra\t" : "srl\t"); break;
                case 0x6: printf("or\t"); break;
                case 0x7: printf("and\t"); break;
                default: printf("unknown_r\t"); break;
            }
            printf("%s, %s, %s", get_register_name(rd), get_register_name(rs1), get_register_name(rs2));
            break;
        case 0x13: // I-type
            switch (funct3) {
                case 0x0: printf("addi\t"); break;
                case 0x2: printf("slti\t"); break;
                case 0x3: printf("sltiu\t"); break;
                case 0x4: printf("xori\t"); break;
                case 0x6: printf("ori\t"); break;
                case 0x7: printf("andi\t"); break;
                case 0x1: printf("slli\t"); break;
                case 0x5: printf(funct7 == 0x20 ? "srai\t" : "srli\t"); break;
                default: printf("unknown_i\t"); break;
            }
            printf("%s, %s, %d", get_register_name(rd), get_register_name(rs1), imm_i);
            break;
        case 0x03: // I-type (load)
             switch (funct3) {
                case 0x0: printf("lb\t"); break;
                case 0x1: printf("lh\t"); break;
                case 0x2: printf("lw\t"); break;
                case 0x4: printf("lbu\t"); break;
                case 0x5: printf("lhu\t"); break;
                default: printf("unknown_load\t"); break;
            }
            printf("%s, %d(%s)", get_register_name(rd), imm_i, get_register_name(rs1));
            break;
        case 0x23: // S-type
            switch (funct3) {
                case 0x0: printf("sb\t"); break;
                case 0x1: printf("sh\t"); break;
                case 0x2: printf("sw\t"); break;
                default: printf("unknown_s\t"); break;
            }
            printf("%s, %d(%s)", get_register_name(rs2), imm_s, get_register_name(rs1));
            break;
        case 0x63: // B-type
            switch (funct3) {
                case 0x0: printf("beq\t"); break;
                case 0x1: printf("bne\t"); break;
                case 0x4: printf("blt\t"); break;
                case 0x5: printf("bge\t"); break;
                case 0x6: printf("bltu\t"); break;
                case 0x7: printf("bgeu\t"); break;
                default: printf("unknown_b\t"); break;
            }
            uint32_t target_addr_b = addr + imm_b;
            const char *symbol_b = find_symbol_by_addr(target_addr_b, sym_tab, sym_count, str_tab);
            if (symbol_b) {
                 printf("%s, %s, %x <%s>", get_register_name(rs1), get_register_name(rs2), target_addr_b, symbol_b);
            } else {
                 printf("%s, %s, 0x%x", get_register_name(rs1), get_register_name(rs2), target_addr_b);
            }
            break;
        case 0x37: // U-type (lui)
            printf("lui\t%s, 0x%x", get_register_name(rd), (uint32_t)imm_u >> 12);
            break;
        case 0x17: // U-type (auipc)
            printf("auipc\t%s, 0x%x", get_register_name(rd), (uint32_t)imm_u >> 12);
            break;
        case 0x6f: // J-type (jal)
            uint32_t target_addr_j = addr + imm_j;
            const char *symbol_j = find_symbol_by_addr(target_addr_j, sym_tab, sym_count, str_tab);
            printf("jal\t%s, ", get_register_name(rd));
            if (symbol_j) {
                printf("0x%x <%s>", target_addr_j, symbol_j);
            } else {
                printf("0x%x", target_addr_j);
            }
            break;
        case 0x67: // I-type (jalr)
            printf("jalr\t%s, %d(%s)", get_register_name(rd), imm_i, get_register_name(rs1));
            break;
        case 0x73: // ecall/ebreak
            if (extract_bits(instruction, 20, 12) == 0) printf("ecall");
            else if (extract_bits(instruction, 20, 12) == 1) printf("ebreak");
            else printf("unknown_system");
            break;
        default:
            printf("unknown_instruction");
            break;
    }
    printf("\n");
}

void disassemble_text_section(const unsigned char *file_contents, const Elf32_Shdr *text_header, const Elf32_Sym *sym_tab, int sym_count, const char *str_tab) {
    printf("\nDisassembly of section .text:\n");

    const unsigned char *text_section = file_contents + text_header->sh_offset;
    uint32_t text_size = text_header->sh_size;

    for (uint32_t i = 0; i < text_size; i += 4) {
        uint32_t current_addr = text_header->sh_addr + i;
        
        const char *symbol = find_symbol_by_addr(current_addr, sym_tab, sym_count, str_tab);
        if (symbol) {
            printf("\n%08x <%s>:\n", current_addr, symbol);
        }

        uint32_t instruction = *(uint32_t*)(text_section + i);
        disassemble_instruction(instruction, current_addr, sym_tab, sym_count, str_tab);
    }
}

#endif // RISCV_DISASSEMBLER_IMPL
#endif // RISCV_DISASSEMBLER_H
