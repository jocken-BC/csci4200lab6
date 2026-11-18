#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>

/* =========================================================================
 * Hardware Constants & Bitmasks (CS:APP 1-Level Model)
 * ========================================================================= */
#define PAGE_SIZE           4096                  // 4 KB pages (2^12 bytes)
#define RAM_SIZE            (16 * 1024 * 1024)    // 16 MB Physical RAM

// Field Shift Amounts & Bitmasks
#define VPO_SHIFT           0
#define VPN_SHIFT           12

#define VPO_MASK            0x00000FFF            // Lower 12 bits (Offset)
#define VPN_MASK            0x000FFFFF            // Upper 20 bits (Page Index)
#define PPN_MASK            0xFFFFF000            // Upper 20 bits (Physical Page Number)

// Page Table Entry (PTE) Control Flag Bits
#define PTE_VALID           0x1                   // Bit 0: Valid / Present
#define PTE_RW              0x2                   // Bit 1: Read/Write (1=Read/Write, 0=Read-Only)
#define PTE_US              0x4                   // Bit 2: User/Supervisor (1=User, 0=Supervisor)

/* =========================================================================
 * Data Types & Enums
 * ========================================================================= */
typedef uint32_t vaddr_t;
typedef uint32_t paddr_t;
typedef uint32_t pte_t;

typedef enum {
    READ  = 0,
    WRITE = 1
} access_type_t;

typedef enum {
    SUPERVISOR = 0,
    USER       = 1
} privilege_t;

typedef enum {
    TRANSLATION_SUCCESS              =  0,
    FAULT_PAGE_FAULT                 = -1,
    FAULT_PROTECTION_READ_ONLY       = -2,
    FAULT_PROTECTION_SUPERVISOR_ONLY = -3
} status_code_t;

typedef struct {
    uint8_t *ram;        // Flat byte array simulating physical RAM
    paddr_t pt_paddr;    // Physical address of the single Page Table
} mmu_t;

/* =========================================================================
 * Function Declarations
 * ========================================================================= */
status_code_t translate_address(mmu_t *mmu, vaddr_t vaddr, access_type_t access, 
                                privilege_t mode, paddr_t *paddr_out);
mmu_t* init_mmu(void);
void free_mmu(mmu_t *mmu);
void setup_sample_page_table(mmu_t *mmu);
void process_trace_file(mmu_t *mmu, const char *filename);

/* =========================================================================
 * STUDENT TASK: Virtual Address Translation (1-Level Page Table)
 * ========================================================================= */
/**
 * Translates a 32-bit Virtual Address into a 32-bit Physical Address
 * using a single-level page table.
 *
 * @param mmu       Pointer to the simulated MMU containing physical RAM.
 * @param vaddr     The 32-bit Virtual Address to translate.
 * @param access    READ or WRITE access type.
 * @param mode      USER or SUPERVISOR privilege level.
 * @param paddr_out Pointer where the translated Physical Address should be stored.
 *
 * @return TRANSLATION_SUCCESS if successful, or the appropriate status_code_t fault.
 */
status_code_t translate_address(mmu_t *mmu, vaddr_t vaddr, access_type_t access, 
                                privilege_t mode, paddr_t *paddr_out) 
{
    /* TODO: Implement 1-level page table address translation.
     *
     * Step 1: Extract Virtual Address Fields
     *   - Extract VPO (bits 11:0)
     *   - Extract VPN (bits 31:12)
     *
     * Step 2: Access Page Table Entry (PTE)
     *   - Page table base address is at mmu->pt_paddr.
     *   - Index into page table using VPN to get the PTE.
     *
     * Step 3: Check Valid Bit
     *   - If PTE Bit 0 (PTE_VALID) is 0:
     *     -> Return FAULT_PAGE_FAULT
     *
     * Step 4: Perform Protection Checks
     *   - If access == WRITE and PTE Bit 1 (PTE_RW) is 0:
     *     -> Return FAULT_PROTECTION_READ_ONLY
     *   - If mode == USER and PTE Bit 2 (PTE_US) is 0:
     *     -> Return FAULT_PROTECTION_SUPERVISOR_ONLY
     *
     * Step 5: Construct Physical Address
     *   - Extract PPN (upper 20 bits: pte & PPN_MASK)
     *   - Combine PPN with VPO to compute physical address
     *   - Store result in *paddr_out and return TRANSLATION_SUCCESS
     */

    (void)mmu; (void)vaddr; (void)access; (void)mode; (void)paddr_out;
    return FAULT_PAGE_FAULT;
}

/* =========================================================================
 * Driver & System Infrastructure
 * ========================================================================= */

mmu_t* init_mmu(void) {
    mmu_t *mmu = (mmu_t *)malloc(sizeof(mmu_t));
    if (!mmu) {
        perror("Failed to allocate MMU structure");
        exit(EXIT_FAILURE);
    }

    mmu->ram = (uint8_t *)calloc(RAM_SIZE, sizeof(uint8_t));
    if (!mmu->ram) {
        perror("Failed to allocate physical RAM");
        free(mmu);
        exit(EXIT_FAILURE);
    }

    // Place single Page Table at Physical Address 0x00001000
    mmu->pt_paddr = 0x00001000;
    return mmu;
}

void free_mmu(mmu_t *mmu) {
    if (mmu) {
        if (mmu->ram) free(mmu->ram);
        free(mmu);
    }
}

void setup_sample_page_table(mmu_t *mmu) {
    pte_t *pt = (pte_t *)(mmu->ram + mmu->pt_paddr);

    // Entry VPN 0x00100: Valid, Read-Write, User -> PPN 0x00100
    pt[0x00100] = 0x00100000 | PTE_VALID | PTE_RW | PTE_US;

    // Entry VPN 0x00101: Valid, Read-Only, User -> PPN 0x00101
    pt[0x00101] = 0x00101000 | PTE_VALID | PTE_US;

    // Entry VPN 0x00102: Valid, Read-Write, Supervisor-Only -> PPN 0x00102
    pt[0x00102] = 0x00102000 | PTE_VALID | PTE_RW;

    // Entry VPN 0x00103: Invalid (Triggers Page Fault)
    pt[0x00103] = 0x00000000;
}

void process_trace_file(mmu_t *mmu, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open trace file '%s'\n", filename);
        exit(EXIT_FAILURE);
    }

    vaddr_t vaddr;
    char rw_char, mode_char;

    printf("\n%-12s %-8s %-12s %-12s %-30s\n", 
           "VADDR", "TYPE", "PRIVILEGE", "PADDR", "STATUS / RESULT");
    printf("--------------------------------------------------------------------------------\n");

    while (fscanf(file, "%i %c %c", &vaddr, &rw_char, &mode_char) == 3) {
        access_type_t access = (rw_char == 'W' || rw_char == 'w') ? WRITE : READ;
        privilege_t mode = (mode_char == 'U' || mode_char == 'u') ? USER : SUPERVISOR;

        paddr_t paddr = 0;
        status_code_t status = translate_address(mmu, vaddr, access, mode, &paddr);

        printf("0x%08X   %-8s %-12s ", 
               vaddr, 
               (access == WRITE) ? "WRITE" : "READ", 
               (mode == USER) ? "USER" : "SUPERVISOR");

        if (status == TRANSLATION_SUCCESS) {
            printf("0x%08X   [SUCCESS] Translated\n", paddr);
        } else {
            printf("0x--------   [FAULT] ");
            switch (status) {
                case FAULT_PAGE_FAULT:
                    printf("Page Fault (Page Not Present in Memory)\n");
                    break;
                case FAULT_PROTECTION_READ_ONLY:
                    printf("Protection Violation (Write to Read-Only Page)\n");
                    break;
                case FAULT_PROTECTION_SUPERVISOR_ONLY:
                    printf("Protection Violation (User Access to Supervisor Page)\n");
                    break;
                default:
                    printf("Unknown Fault\n");
                    break;
            }
        }
    }

    fclose(file);
}

int main(int argc, char *argv[]) {
    char *trace_filename = NULL;
    int opt;

    while ((opt = getopt(argc, argv, "t:")) != -1) {
        switch (opt) {
            case 't':
                trace_filename = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s -t <tracefile>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (!trace_filename) {
        fprintf(stderr, "Error: Missing required option -t <tracefile>\n");
        fprintf(stderr, "Usage: %s -t <tracefile>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    mmu_t *mmu = init_mmu();
    setup_sample_page_table(mmu);

    process_trace_file(mmu, trace_filename);

    free_mmu(mmu);

    return 0;
}
