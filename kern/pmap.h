/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_PMAP_H
#define JOS_KERN_PMAP_H
#include "inc/mmu.h"
#include "inc/types.h"
#ifndef JOS_KERNEL
#error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/memlayout.h>
#include <inc/assert.h>
struct Env;

extern char bootstacktop[], bootstack[];

extern struct PageInfo *pages;
extern size_t npages;

extern pde_t *kern_pgdir;

/* This macro takes a kernel virtual address -- an address that points above
 * KERNBASE, where the machine's maximum 256MB of physical memory is mapped --
 * and returns the corresponding physical address.  It panics if you pass it a
 * non-kernel virtual address.
 */
#define PADDR(kva) _paddr(__FILE__, __LINE__, kva)

static inline physaddr_t
_paddr(const char *file, int line, void *kva)
{
	if ((uint32_t)kva < KERNBASE)
		_panic(file, line, "PADDR called with invalid kva %08lx", kva);
	return (physaddr_t)kva - KERNBASE;
}

/* This macro takes a physical address and returns the corresponding kernel
 * virtual address.  It panics if you pass an invalid physical address. */
#define KADDR(pa) _kaddr(__FILE__, __LINE__, pa)

static inline void *
_kaddr(const char *file, int line, physaddr_t pa)
{
	if (PGNUM(pa) >= npages)
		_panic(file, line, "KADDR called with invalid pa %08lx", pa);
	return (void *)(pa + KERNBASE);
}

enum {
	// For page_alloc, zero the returned physical page.
	ALLOC_ZERO = 1 << 0,
};

void mem_init(void);

void page_init(void);
struct PageInfo *palloc(size_t power_of_two, int alloc_flags);
void pfree(struct PageInfo *pp);
struct PageInfo *page_alloc(int alloc_flags);
void page_free(struct PageInfo *pp);
int page_insert(pde_t *pgdir, struct PageInfo *pp, void *va, int perm);
void page_remove(pde_t *pgdir, void *va);
struct PageInfo *page_lookup(pde_t *pgdir, void *va, pte_t **pte_store);
void page_decref(struct PageInfo *pp);

void tlb_invalidate(pde_t *pgdir, void *va);

void *mmio_map_region(physaddr_t pa, size_t size);

int user_mem_check(struct Env *env, const void *va, size_t len, int perm);
void user_mem_assert(struct Env *env, const void *va, size_t len, int perm);

static inline physaddr_t
page2pa(struct PageInfo *pp)
{
	return (pp - pages) << PGSHIFT;
}

static inline struct PageInfo *
pa2page(physaddr_t pa)
{
	if (PGNUM(pa) >= npages) {
		cprintf("pgnum: %d\n", PGNUM(pa));
		panic("pa2page called with invalid pa");
	}
	return &pages[PGNUM(pa)];
}

static inline void *
page2kva(struct PageInfo *pp)
{
	return KADDR(page2pa(pp));
}

pte_t *pgdir_walk(pde_t *pgdir, const void *va, int create);

static inline bool
is_sp_entry(pte_t *pgdir, uintptr_t va)
{
	return pgdir[PTE_ADDR(va)] & PTE_PS;
}

static inline physaddr_t
get_sp_entry(pte_t *pgdir, uintptr_t va, off_t offset)
{
	uintptr_t page_addr = PDX(pgdir[PDX(va)]) << PDXSHIFT;
	return page_addr | SPGOFF(offset);
}

#endif /* !JOS_KERN_PMAP_H */
