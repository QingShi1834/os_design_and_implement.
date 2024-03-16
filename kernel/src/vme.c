#include "klib.h"
#include "vme.h"
#include "proc.h"

static TSS32 tss;
typedef union free_page {
    union free_page *next;
    char buf[PGSIZE];
} page_t;

// 定义空闲页列表的头指针
static page_t *free_page_list = NULL;
void init_gdt() {
  static SegDesc gdt[NR_SEG];
  gdt[SEG_KCODE] = SEG32(STA_X | STA_R,   0,     0xffffffff, DPL_KERN);
  gdt[SEG_KDATA] = SEG32(STA_W,           0,     0xffffffff, DPL_KERN);
  gdt[SEG_UCODE] = SEG32(STA_X | STA_R,   0,     0xffffffff, DPL_USER);
  gdt[SEG_UDATA] = SEG32(STA_W,           0,     0xffffffff, DPL_USER);
  gdt[SEG_TSS]   = SEG16(STS_T32A,     &tss,  sizeof(tss)-1, DPL_KERN);
  set_gdt(gdt, sizeof(gdt[0]) * NR_SEG);
  set_tr(KSEL(SEG_TSS));
}

void set_tss(uint32_t ss0, uint32_t esp0) {
  tss.ss0 = ss0;
  tss.esp0 = esp0;
}

static PD kpd;
static PT kpt[PHY_MEM / PT_SIZE] __attribute__((used));
/**
 * PD（Page Directory）：页目录，是x86架构下分页机制中的一个数据结构，用于存储页表的物理地址。在虚拟内存系统中，页目录用于将虚拟地址映射到物理地址的页表上。

 * PDE（Page Directory Entry）：页目录项，是页目录中的一个条目，用于指向页表或者大页（超级页）。每个页目录项存储了指向页表或者大页的物理地址以及一些控制位。

 * kpt：内核页表，是内核使用的页表，用于管理内核空间的虚拟地址和物理地址的映射关系。

 * NR_PDE：表示页面目录条目的数量。在给定的物理内存大小下，可以计算出需要多少个页面目录条目来管理整个物理内存范围。
 * NR_PTE是一个表示每个页表中PTE（Page Table Entry）数量的常量。在一个页表中，会有多个PTE条目，每个PTE条目对应着一页物理内存的映射。
 */
void init_page() {
  extern char end;
  panic_on((size_t)(&end) >= KER_MEM - PGSIZE, "Kernel too big (MLE)");
  static_assert(sizeof(PTE) == 4, "PTE must be 4 bytes");
  static_assert(sizeof(PDE) == 4, "PDE must be 4 bytes");
  static_assert(sizeof(PT) == PGSIZE, "PT must be one page");
  static_assert(sizeof(PD) == PGSIZE, "PD must be one page");
  // Lab1-4: init kpd and kpt, identity mapping of [0 (or 4096), PHY_MEM)
//  TODO();
    // Initialize kernel page directory kpd
    for (int i = 0; i < PHY_MEM / PT_SIZE; i++) {
        kpd.pde[i].val = MAKE_PDE(&kpt[i], 3);
    }

    // Initialize kernel page tables kpt
    for (int i = 0; i < PHY_MEM / PT_SIZE; i++) {
        for (int j = 0; j < NR_PTE; j++) {
            kpt[i].pte[j].val = MAKE_PTE((i << DIR_SHIFT) | (j << TBL_SHIFT),3);
        }
    }
  kpt[0].pte[0].val = 0;
  set_cr3(&kpd);
  set_cr0(get_cr0() | CR0_PG);
  // Lab1-4: init free memory at [KER_MEM, PHY_MEM), a heap for kernel
//  TODO();
    // 初始化空闲页列表
    uint32_t current = KER_MEM; // 对齐到下一页
    uint32_t limit = PHY_MEM;

    free_page_list = NULL;
    while (current < limit) {
        page_t *original_head = free_page_list;
        free_page_list = (page_t*)current;
        free_page_list->next = original_head;
        current += PGSIZE;
    }
}

void *kalloc() {
  // Lab1-4: alloc a page from kernel heap, abort when heap empty
//  TODO();
    if (free_page_list == NULL) {
        return NULL; // 没有可用的空闲页
    }

    page_t *allocated_page = free_page_list; // 获取空闲页列表的头部页
    free_page_list = free_page_list->next; // 更新空闲页列表的头指针
    //将alloc页面内容清0
    memset(allocated_page,0,PGSIZE);

    return (void*)allocated_page;
}

void kfree(void *ptr) {
  // Lab1-4: free a page to kernel heap
  // you can just do nothing :)
  //TODO();
    page_t *page_to_free = (page_t*)ptr;
    page_to_free->next = free_page_list; // 将要回收的页插入到空闲页列表的头部
    free_page_list = page_to_free;
    // 懒得重置内存了  反正alloc的时候会重置
}

PD *vm_alloc() {
  // Lab1-4: alloc a new pgdir, map memory under PHY_MEM identityly
//  TODO();
    // Allocate a page for the page directory
    PD *pd = (PD *)kalloc();
    if (pd == NULL) {
        panic("Failed to allocate memory for page directory");
    }

    // Initialize the page directory
    for (int i = 0; i < NR_PDE; i++) {
        if (i < 32) {
            pd->pde[i].val = MAKE_PDE(&kpt[i], 1); // Identity mapping with privilege level 1
        } else {
            pd->pde[i].val = 0; // Clear the rest of the page directory entries
        }
    }

    return pd;
}

void vm_teardown(PD *pgdir) {
  // Lab1-4: free all pages mapping above PHY_MEM in pgdir, then free itself
  // you can just do nothing :)
  //TODO();
    // Free all page tables and their corresponding physical pages
    for (int i = 0; i < NR_PDE; i++) {
        if (i < PHY_MEM / PT_SIZE) {
            continue; // Skip the kernel page tables kpt
        }

        if (pgdir->pde[i].present==0) {
            continue;
        }

        PT *pt = PDE2PT(pgdir->pde[i]); // Extract the physical address of the page table

        for (int j = 0; j < NR_PTE; j++) {
            if (pt->pte[j].present==0) {
                continue;
            }
            page_t *page = PTE2PG(pt->pte[j]); // Extract the physical address of the page

            if (page != NULL) {
                kfree(page); // Free the physical page
            }
        }

        kfree(pt); // Free the page table
    }

    kfree(pgdir); // Free the page directory
}

PD *vm_curr() {
  return (PD*)PAGE_DOWN(get_cr3());
}

PTE *vm_walkpte(PD *pgdir, size_t va, int prot) {
  // Lab1-4: return the pointer of PTE which match va
  // if not exist (PDE of va is empty) and prot&1, alloc PT and fill the PDE
  // if not exist (PDE of va is empty) and !(prot&1), return NULL
  // remember to let pde's prot |= prot, but not pte
  assert((prot & ~7) == 0);
//  TODO();
    uint32_t pd_index = ADDR2DIR(va);
    uint32_t pt_index = ADDR2TBL(va);

    PDE* pde = &(pgdir->pde[pd_index]);
    // Check if PDE is present
    if (!(pde->val & PTE_P)) {
        // PDE is not present

        if (prot&0x1) {
            // Allocate PT and fill PDE
            PT* pt = (PT*)kalloc();
            if (!pt) {
                return NULL; // Allocation failed
            }

            // Clear all PTEs in the newly allocated PT
            for (int i = 0; i < 1024; i++) {
                pt->pte[i].val = 0;
            }
            pde->val =MAKE_PDE(pt,prot);
//            *pde = (uint32_t)pt | prot | PTE_P; // Update PDE with new PT address and permissions
        } else {
            return NULL; // PDE is empty and !(prot&1), return NULL
        }
    }

    // Get the PT address from PDE
    PT *pt = PDE2PT(*pde);
    PTE* pte = &(pt->pte[pt_index]); // 找到对应的页表项


    return pte;
}

void *vm_walk(PD *pgdir, size_t va, int prot) {
  // Lab1-4: translate va to pa
  // if prot&1 and prot voilation ((pte->val & prot & 7) != prot), call vm_pgfault
  // if va is not mapped and !(prot&1), return NULL
  TODO();
}

void vm_map(PD *pgdir, size_t va, size_t len, int prot) {
  // Lab1-4: map [PAGE_DOWN(va), PAGE_UP(va+len)) at pgdir, with prot
  // if have already mapped pages, just let pte->prot |= prot
  assert(prot & PTE_P);
  assert((prot & ~7) == 0);
  size_t start = PAGE_DOWN(va);
  size_t end = PAGE_UP(va + len);
  assert(start >= PHY_MEM);
  assert(end >= start);
//  TODO();
    for (size_t vaddr = PAGE_DOWN(va); vaddr < PAGE_UP(va + len); vaddr += PGSIZE) {
        PTE *pte = vm_walkpte(pgdir, vaddr, prot);
        if (pte == NULL) {
            // Unable to find or allocate PTE
            continue;
        }

        // Allocate a physical page and map it
        void *phy_page = kalloc();
        if (!phy_page) {
            // Allocation failed
            continue;
        }
        if(pte->present==0){
            // Map the virtual address to the physical page with the given permissions
            pte->val = MAKE_PTE((uint32_t)phy_page, prot);
        }else{
            pte->val |= prot;
        }
    }
}

void vm_unmap(PD *pgdir, size_t va, size_t len) {
  // Lab1-4: unmap and free [va, va+len) at pgdir
  // you can just do nothing :)
  //assert(ADDR2OFF(va) == 0);
  //assert(ADDR2OFF(len) == 0);
  //TODO();
}

void vm_copycurr(PD *pgdir) {
  // Lab2-2: copy memory mapped in curr pd to pgdir
  TODO();
}

void vm_pgfault(size_t va, int errcode) {
  printf("pagefault @ 0x%p, errcode = %d\n", va, errcode);
  panic("pgfault");
}
