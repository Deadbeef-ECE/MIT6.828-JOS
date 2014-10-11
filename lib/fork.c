// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at vpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	pde_t pde = vpt[PGNUM(addr)];

	if(!((err & FEC_WR) && (pde &PTE_COW) ))
		panic("Unrecoverable page fault at address[0x%x]!\n", addr);

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

	// LAB 4: Your code here.
	envid_t thisenv_id = sys_getenvid();
	sys_page_alloc(thisenv_id, PFTEMP, PTE_P|PTE_W|PTE_U);
	memmove((void*)PFTEMP, (void*)ROUNDDOWN(addr, PGSIZE), PGSIZE);
	sys_page_map(thisenv_id, (void*)PFTEMP, thisenv_id,(void*)ROUNDDOWN(addr, PGSIZE), 
		PTE_U|PTE_W|PTE_P);
	//panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
	// LAB 4: Your code here.
	int map_sz = pn*PGSIZE;
	envid_t thisenv_id = sys_getenvid();
	int perm = vpt[pn]&PTE_SYSCALL;

	if(perm & PTE_COW || perm & PTE_W){
		perm |= PTE_COW;
		perm &= ~PTE_W;
	}
	//cprintf("thisenv_id[%p]\n", thisenv_id);

	if((r = sys_page_map(thisenv_id, (void*)map_sz, envid, (void*)map_sz, perm)) < 0)
		return r;
	if((r = sys_page_map(thisenv_id, (void*)map_sz, thisenv_id, (void*)map_sz, perm)) < 0)
		return r;
	//panic("duppage not implemented");
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use vpd, vpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	envid_t child_id;
	uint32_t pg_cow_ptr;
	int r;

	set_pgfault_handler(pgfault);

	if((child_id = sys_exofork()) < 0)
		panic("Fork error\n");
	if(child_id == 0){
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}
	for(pg_cow_ptr = UTEXT; pg_cow_ptr < UXSTACKTOP - PGSIZE; pg_cow_ptr += PGSIZE){
		if ((vpd[PDX(pg_cow_ptr)]&PTE_P) && (vpt[PGNUM(pg_cow_ptr)]&(PTE_P|PTE_U)))
			duppage(child_id, PGNUM(pg_cow_ptr));
	}
	if((r = sys_page_alloc(child_id, (void *)(UXSTACKTOP - PGSIZE), PTE_U|PTE_P|PTE_W)))
		panic("Alloc exception stack error: %e\n", r);

	extern void _pgfault_upcall(void);
	sys_env_set_pgfault_upcall(child_id, _pgfault_upcall);

	sys_env_set_status(child_id, ENV_RUNNABLE);
	return child_id;
	//panic("fork not implemented");
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
