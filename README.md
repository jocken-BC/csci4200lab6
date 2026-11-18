# Virtual Memory Lab: 1-Level Page Table Address Translation

## Overview
In this lab, you will implement the core address translation logic of a simulated Memory Management Unit (MMU) modeled after Chapter 9 (*Virtual Memory*) of **Computer Systems: A Programmer's Perspective (CS:APP)**. 

Your task is to translate 32-bit virtual addresses into 32-bit physical addresses using a single-level page table stored in simulated physical RAM, while enforcing page validity and memory protection rules.

---

## System Architecture & Specifications

### Address Space & Memory Layout
* **Virtual Address Width:** 32 bits (`vaddr_t`)
* **Physical Address Width:** 32 bits (`paddr_t`)
* **Page Size:** 4 KB (`4096` bytes / 2¹² bytes)
* **Simulated Physical RAM:** 16 MB (`RAM_SIZE`)
* **Page Table Base Address (`mmu->pt_paddr`):** `0x00001000`

### 32-Bit Virtual Address Breakdown
A 32-bit virtual address is split into two fields:

| Bits `31:12` (20 bits) | Bits `11:0` (12 bits) |
| :--- | :--- |
| **VPN** (Virtual Page Number) | **VPO** (Virtual Page Offset) |

* **VPO (`VPO_MASK = 0x00000FFF`):** The byte offset within the 4 KB page. Because virtual and physical pages are the same size, PPO = VPO.
* **VPN (`VPN_MASK = 0x000FFFFF`):** Used as an index into the single-level page table after right-shifting by `VPN_SHIFT` (`12`).

### Page Table Entry (PTE) Format
Each Page Table Entry (`pte_t`) is 32 bits wide. The upper 20 bits hold the Physical Page Number (aligned to a 4 KB boundary), and the lower 3 bits act as control and protection flags:

| Bits `31:12` | Bits `11:3` | Bit `2` (`0x4`) | Bit `1` (`0x2`) | Bit `0` (`0x1`) |
| :--- | :--- | :--- | :--- | :--- |
| **PPN** (`PPN_MASK = 0xFFFFF000`) | *Unused* | **U/S** (`PTE_US`) | **R/W** (`PTE_RW`) | **Valid** (`PTE_VALID`) |

* **`PTE_VALID` (`0x1`):** `1` if the page is mapped in physical memory; `0` if unmapped (triggers a page fault).
* **`PTE_RW` (`0x2`):** `1` if both `READ` and `WRITE` accesses are allowed; `0` if the page is **Read-Only**.
* **`PTE_US` (`0x4`):** `1` if accessible by both `USER` and `SUPERVISOR` modes; `0` if restricted to **Supervisor-Only** mode.

---

## Your Task

Open the starter C file and implement the `translate_address` function:

```c
status_code_t translate_address(mmu_t *mmu, vaddr_t vaddr, access_type_t access, 
                                privilege_t mode, paddr_t *paddr_out);
```

### Step-by-Step Translation Logic
1. **Extract Virtual Address Fields:**
   * Isolate the 12-bit **VPO** and the 20-bit **VPN** from `vaddr` using the provided shift and mask constants.
2. **Locate the Page Table Entry (PTE):**
   * The page table begins at `mmu->ram + mmu->pt_paddr`.
   * Cast this location to a `pte_t *` array and index into it using the `VPN`. *(Note: Ensure you handle out-of-bounds VPNs safely if applicable, or index directly into the allocated page table).*
3. **Check the Valid Bit:**
   * If `PTE_VALID` is `0`, return `FAULT_PAGE_FAULT`.
4. **Enforce Memory Protection (in order):**
   * **Write Protection:** If `access == WRITE` and `PTE_RW` is `0`, return `FAULT_PROTECTION_READ_ONLY`.
   * **Privilege Protection:** If `mode == USER` and `PTE_US` is `0`, return `FAULT_PROTECTION_SUPERVISOR_ONLY`.
5. **Construct the Physical Address:**
   * Extract the upper 20 bits of the PTE (`pte & PPN_MASK`) and combine them with the `VPO`.
   * Write the resulting 32-bit physical address to `*paddr_out` and return `TRANSLATION_SUCCESS`.

> **Important:** Do not modify `init_mmu`, `free_mmu`, `setup_sample_page_table`, `process_trace_file`, or `main` unless instructed otherwise.

---

## Building and Running

### Compilation
Compile the simulator with `gcc` using standard warning flags:
```bash
gcc -Wall -Wextra -Werror -g mmu_lab.c -o mmu_sim
```
*(Replace `mmu_lab.c` with the actual filename of your source file.)*

### Running with a Trace File
Run the compiled binary by passing a trace file with the `-t` flag:
```bash
./mmu_sim -t sample_trace.txt
```

### Trace File Format
Each line in a trace file represents a single memory access formatted as:
```text
<virtual_address> <R|W> <U|S>
```
* `<virtual_address>`: 32-bit hex (e.g., `0x00100040`) or decimal address
* `<R|W>`: Access type (`R` = Read, `W` = Write)
* `<U|S>`: CPU privilege level (`U` = User mode, `S` = Supervisor mode)

#### Example Trace & Pre-configured Mappings
The starter code pre-populates four entries in `setup_sample_page_table`:
* `VPN 0x00100` → `PPN 0x00100` (Valid, Read/Write, User)
* `VPN 0x00101` → `PPN 0x00101` (Valid, Read-Only, User)
* `VPN 0x00102` → `PPN 0x00102` (Valid, Read/Write, Supervisor-Only)
* `VPN 0x00103` → Invalid (`0x00000000`)

Given a sample trace file:
```text
0x00100024 R U
0x00101008 W U
0x00102100 R U
0x00102100 W S
0x00103000 R S
```

Your output should match:
```text
VADDR        TYPE     PRIVILEGE    PADDR        STATUS / RESULT               
--------------------------------------------------------------------------------
0x00100024   READ     USER         0x00100024   [SUCCESS] Translated
0x00101008   WRITE    USER         0x--------   [FAULT] Protection Violation (Write to Read-Only Page)
0x00102100   READ     USER         0x--------   [FAULT] Protection Violation (User Access to Supervisor Page)
0x00102100   WRITE    SUPERVISOR   0x00102100   [SUCCESS] Translated
0x00103000   READ     SUPERVISOR   0x--------   [FAULT] Page Fault (Page Not Present in Memory)
```

---

## Common Pitfalls & Tips
* **Operator Precedence:** Bitwise operators (`&`, `|`, `>>`) have lower precedence than comparison operators (`==`, `!=`). Always wrap bitwise expressions in parentheses (e.g., `if ((pte & PTE_VALID) == 0)`).
* **Pointer Arithmetic:** `mmu->ram` is a `uint8_t *` (byte pointer), while `mmu->pt_paddr` is a byte offset. Once cast to `(pte_t *)`, indexing `pt[vpn]` automatically scales by `sizeof(pte_t)` (4 bytes).
* **Fault Priority:** Always check `PTE_VALID` **before** checking `PTE_RW` or `PTE_US`. If a page is not valid, its protection bits are meaningless.
