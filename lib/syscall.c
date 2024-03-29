// System call stubs.

#include <inc/syscall.h>
#include <inc/lib.h>

/* @yuhangj
 * add new function from inc/x86.h
 */
#include <inc/x86.h>


static int32_t
fast_syscall(int num, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4){
	uint32_t ret;
	// @yuhangj
	// the implemetation of sysenter.
	// It first saves ebp, then moves all arguments into registers.
	// It then load the return address into esi and the original stack into esp
	// It then calls sysenter and begins running in sysenter_handler in 
	// kern/trapentry.S
	// When it returns it recovers its ebp value and moves the return value in
	// eax into the ret variable, which it returns.
	asm volatile("pushl %%ebp;"
					"movl %1, %%eax;"
					"movl %2, %%edx;"
					"movl %3, %%ecx;"
					"movl %4, %%ebx;"
					"movl %5, %%edi;"
					"leal ctn_syse, %%esi;"
					"movl %%esp, %%ebp;"
						"sysenter;"
						"ctn_syse: "
						"popl %%ebp;"
						"movl %%eax, %0"
						: "=m"(ret)
						: "m"(num), "m"(a1), "m"(a2), "m"(a3), "m"(a4));
	return ret;
}

static inline int32_t
syscall(int num, int check, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	int32_t ret;

	/* @yuhangj
	 * relative asm function defined in inc/x86.h
	 * Check if we can use sysenter/sysexit instruction 
	 * If not list, execute regular trap handler.
	 */
	if(get_cpu_features() & CPUID_FLAG_SEP){
		switch(num)
		{
			case SYS_cputs:
			case SYS_cgetc:
			case SYS_getenvid:
				//return fast_syscall(num, a1, a2, a3, a4);
			case SYS_env_destroy:
			case SYS_yield:
			default:
				break;
		}
	}

	// Generic system call: pass system call number in AX,
	// up to five parameters in DX, CX, BX, DI, SI.
	// Interrupt kernel with T_SYSCALL.
	//
	// The "volatile" tells the assembler not to optimize
	// this instruction away just because we don't use the
	// return value.
	//
	// The last clause tells the assembler that this can
	// potentially change the condition codes and arbitrary
	// memory locations.

	asm volatile("int %1\n"
		: "=a" (ret)
		: "i" (T_SYSCALL),
		  "a" (num),
		  "d" (a1),
		  "c" (a2),
		  "b" (a3),
		  "D" (a4),
		  "S" (a5)
		: "cc", "memory");

	if(check && ret > 0)
		panic("syscall %d returned %d (> 0)", num, ret);

	return ret;
}

void
sys_cputs(const char *s, size_t len)
{
	syscall(SYS_cputs, 0, (uint32_t)s, len, 0, 0, 0);
}

int
sys_cgetc(void)
{
	return syscall(SYS_cgetc, 0, 0, 0, 0, 0, 0);
}

int
sys_env_destroy(envid_t envid)
{
	return syscall(SYS_env_destroy, 1, envid, 0, 0, 0, 0);
}

envid_t
sys_getenvid(void)
{
	 return syscall(SYS_getenvid, 0, 0, 0, 0, 0, 0);
}

void
sys_yield(void)
{
	syscall(SYS_yield, 0, 0, 0, 0, 0, 0);
}

int
sys_page_alloc(envid_t envid, void *va, int perm)
{
	return syscall(SYS_page_alloc, 1, envid, (uint32_t) va, perm, 0, 0);
}

int
sys_page_map(envid_t srcenv, void *srcva, envid_t dstenv, void *dstva, int perm)
{
	return syscall(SYS_page_map, 1, srcenv, (uint32_t) srcva, dstenv, (uint32_t) dstva, perm);
}

int
sys_page_unmap(envid_t envid, void *va)
{
	return syscall(SYS_page_unmap, 1, envid, (uint32_t) va, 0, 0, 0);
}

// sys_exofork is inlined in lib.h

int
sys_env_set_status(envid_t envid, int status)
{
	return syscall(SYS_env_set_status, 1, envid, status, 0, 0, 0);
}

int
sys_env_set_pgfault_upcall(envid_t envid, void *upcall)
{
	return syscall(SYS_env_set_pgfault_upcall, 1, envid, (uint32_t) upcall, 0, 0, 0);
}

int
sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, int perm)
{
	return syscall(SYS_ipc_try_send, 0, envid, value, (uint32_t) srcva, perm, 0);
}

int
sys_ipc_recv(void *dstva)
{
	return syscall(SYS_ipc_recv, 1, (uint32_t)dstva, 0, 0, 0, 0);
}

