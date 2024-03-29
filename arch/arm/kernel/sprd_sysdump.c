/*
 * Copyright (C) 2013 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/elf.h>
#include <linux/elfcore.h>
#include <linux/init.h>
#include <linux/highuid.h>
#include <linux/sched.h>
#include <linux/sysctl.h>
#include <asm/memory.h>
#include <asm/cacheflush.h>
#include <asm/outercache.h>

#include <mach/board.h>
#include <mach/board_htc.h>
#include <mach/hardware.h>


#define CORE_STR 		"CORE"
#ifndef ELF_CORE_EFLAGS
#define ELF_CORE_EFLAGS	0
#endif

#define NR_KCORE_MEM	80
#define SYSDUMP_MAGIC	"SPRD_SYSDUMP_119"

#if 1 
#define SYSDUMP_HEAD_ADDR_MAGIC1 0xA5A5DEAD
#define SYSDUMP_HEAD_ADDR_MAGIC2 0x6565DEAD
#endif 

struct memelfnote
{
	const char *name;
	int type;
	unsigned int datasz;
	void *data;
};

struct sysdump_info {
	char magic[16];
	char time[32];
	char dump_path[128];
	int elfhdr_size;
	int mem_num;
	unsigned long dump_mem_paddr;
};

struct sysdump_extra {
	int  enter_id;
	int  enter_cpu;
	char reason[256];
	struct pt_regs cpu_context[CONFIG_NR_CPUS];
};

struct sysdump_config {
	int enable;
	int dump_modem;
	int reboot;
	char dump_path[128];
};

#if 1 
struct sysdump_head_addr {
	unsigned int magic1;
	void *head_addr;
	unsigned int magic2;
};
#endif 

static struct sysdump_info *sprd_sysdump_info = NULL;
#if 1 
static struct sysdump_head_addr sysdump_def_head_addr;
#endif 

struct sysdump_extra sprd_sysdump_extra = {
	.enter_id = -1,
	.enter_cpu = -1,
	.reason = {0},
};

static struct sysdump_config sysdump_conf = {
	.enable = 1,
	.dump_modem = 1,
	.reboot = 1,
	.dump_path = "",
};

static size_t get_elfhdr_size(int nphdr)
{
	size_t elfhdr_len;

	elfhdr_len = sizeof(struct elfhdr) +
		(nphdr + 1)*sizeof(struct elf_phdr) +
		((sizeof(struct elf_note)) * 3 +
		roundup(sizeof(CORE_STR), 4)) +
		roundup(sizeof(struct elf_prstatus), 4) +
		roundup(sizeof(struct elf_prpsinfo), 4) +
		roundup(sizeof(struct task_struct), 4);
	elfhdr_len = PAGE_ALIGN(elfhdr_len);

	return elfhdr_len;
}

static int notesize(struct memelfnote *en)
{
	int sz;

	sz = sizeof(struct elf_note);
	sz += roundup((strlen(en->name) + 1), 4);
	sz += roundup(en->datasz, 4);

	return sz;
}

static char *storenote(struct memelfnote *men, char *bufp)
{
	struct elf_note en;

#define DUMP_WRITE(addr,nr) do { memcpy(bufp,addr,nr); bufp += nr; } while(0)

	en.n_namesz = strlen(men->name) + 1;
	en.n_descsz = men->datasz;
	en.n_type = men->type;

	DUMP_WRITE(&en, sizeof(en));
	DUMP_WRITE(men->name, en.n_namesz);

	
	bufp = (char*) roundup((unsigned long)bufp,4);
	DUMP_WRITE(men->data, men->datasz);
	bufp = (char*) roundup((unsigned long)bufp,4);

#undef DUMP_WRITE

	return bufp;
}

static void fill_prstatus(struct elf_prstatus *prstatus,
		struct task_struct *p, long signr)
{
	prstatus->pr_info.si_signo = prstatus->pr_cursig = signr;
	prstatus->pr_sigpend = p->pending.signal.sig[0];
	prstatus->pr_sighold = p->blocked.sig[0];
	rcu_read_lock();
	prstatus->pr_ppid = task_pid_vnr(rcu_dereference(p->real_parent));
	rcu_read_unlock();
	prstatus->pr_pid = task_pid_vnr(p);
	prstatus->pr_pgrp = task_pgrp_vnr(p);
	prstatus->pr_sid = task_session_vnr(p);
	if ( 0 ) {
		struct task_cputime cputime;

		
		cputime_to_timeval(cputime.utime, &prstatus->pr_utime);
		cputime_to_timeval(cputime.stime, &prstatus->pr_stime);
	} else {
		cputime_to_timeval(p->utime, &prstatus->pr_utime);
		cputime_to_timeval(p->stime, &prstatus->pr_stime);
	}
	cputime_to_timeval(p->signal->cutime, &prstatus->pr_cutime);
	cputime_to_timeval(p->signal->cstime, &prstatus->pr_cstime);
}

static int fill_psinfo(struct elf_prpsinfo *psinfo, struct task_struct *p,
		       struct mm_struct *mm)
{
	const struct cred *cred;
	unsigned int i, len;

	
	memset(psinfo, 0, sizeof(struct elf_prpsinfo));

	if (mm) {
		len = mm->arg_end - mm->arg_start;
		if (len >= ELF_PRARGSZ)
			len = ELF_PRARGSZ-1;
		if (copy_from_user(&psinfo->pr_psargs,
			           (const char __user *)mm->arg_start, len))
			return -EFAULT;
		for(i = 0; i < len; i++)
			if (psinfo->pr_psargs[i] == 0)
				psinfo->pr_psargs[i] = ' ';
		psinfo->pr_psargs[len] = 0;
	}

	rcu_read_lock();
	psinfo->pr_ppid = task_pid_vnr(rcu_dereference(p->real_parent));
	rcu_read_unlock();
	psinfo->pr_pid = task_pid_vnr(p);
	psinfo->pr_pgrp = task_pgrp_vnr(p);
	psinfo->pr_sid = task_session_vnr(p);

	i = p->state ? ffz(~p->state) + 1 : 0;
	psinfo->pr_state = i;
	psinfo->pr_sname = (i > 5) ? '.' : "RSDTZW"[i];
	psinfo->pr_zomb = psinfo->pr_sname == 'Z';
	psinfo->pr_nice = task_nice(p);
	psinfo->pr_flag = p->flags;

	rcu_read_lock();
	cred = __task_cred(p);
	SET_UID(psinfo->pr_uid, cred->uid);
	SET_GID(psinfo->pr_gid, cred->gid);
	rcu_read_unlock();
	strncpy(psinfo->pr_fname, p->comm, sizeof(psinfo->pr_fname));

	return 0;
}

static inline void crash_setup_regs(struct pt_regs *newregs,
				    struct pt_regs *oldregs)
{
	if (oldregs) {
		memcpy(newregs, oldregs, sizeof(*newregs));
	} else {
		__asm__ __volatile__ (
			"stmia	%[regs_base], {r0-r12}\n\t"
			"mov	%[_ARM_sp], sp\n\t"
			"str	lr, %[_ARM_lr]\n\t"
			"adr	%[_ARM_pc], 1f\n\t"
			"mrs	%[_ARM_cpsr], cpsr\n\t"
		"1:"
			: [_ARM_pc] "=r" (newregs->ARM_pc),
			  [_ARM_cpsr] "=r" (newregs->ARM_cpsr),
			  [_ARM_sp] "=r" (newregs->ARM_sp),
			  [_ARM_lr] "=o" (newregs->ARM_lr)
			: [regs_base] "r" (&newregs->ARM_r0)
			: "memory"
		);
	}
}


static void sysdump_fill_core_hdr(struct pt_regs *regs,
						struct sysdump_mem *sysmem, int mem_num,
						char *bufp, int nphdr, int dataoff)
{
	struct elf_prstatus prstatus;	
	struct elf_prpsinfo prpsinfo;	
	struct elf_phdr *nhdr, *phdr;
	struct elfhdr *elf;
	struct memelfnote notes[3];
	off_t offset = 0;
	int i;

	
	elf = (struct elfhdr *) bufp;
	bufp += sizeof(struct elfhdr);
	offset += sizeof(struct elfhdr);
	memcpy(elf->e_ident, ELFMAG, SELFMAG);
	elf->e_ident[EI_CLASS]	= ELF_CLASS;
	elf->e_ident[EI_DATA]	= ELF_DATA;
	elf->e_ident[EI_VERSION]= EV_CURRENT;
	elf->e_ident[EI_OSABI] = ELF_OSABI;
	memset(elf->e_ident+EI_PAD, 0, EI_NIDENT-EI_PAD);
	elf->e_type	= ET_CORE;
	elf->e_machine	= ELF_ARCH;
	elf->e_version	= EV_CURRENT;
	elf->e_entry	= 0;
	elf->e_phoff	= sizeof(struct elfhdr);
	elf->e_shoff	= 0;
	elf->e_flags	= ELF_CORE_EFLAGS;
	elf->e_ehsize	= sizeof(struct elfhdr);
	elf->e_phentsize= sizeof(struct elf_phdr);
	elf->e_phnum	= nphdr;
	elf->e_shentsize= 0;
	elf->e_shnum	= 0;
	elf->e_shstrndx	= 0;

	
	nhdr = (struct elf_phdr *) bufp;
	bufp += sizeof(struct elf_phdr);
	offset += sizeof(struct elf_phdr);
	nhdr->p_type	= PT_NOTE;
	nhdr->p_offset	= 0;
	nhdr->p_vaddr	= 0;
	nhdr->p_paddr	= 0;
	nhdr->p_filesz	= 0;
	nhdr->p_memsz	= 0;
	nhdr->p_flags	= 0;
	nhdr->p_align	= 0;

	
	for (i = 0; i < mem_num; i++) {
		phdr = (struct elf_phdr *) bufp;
		bufp += sizeof(struct elf_phdr);
		offset += sizeof(struct elf_phdr);

		phdr->p_type	= PT_LOAD;
		phdr->p_flags	= PF_R|PF_W|PF_X;
		phdr->p_offset	= dataoff;
		phdr->p_vaddr	= sysmem[i].vaddr;
		phdr->p_paddr	= sysmem[i].paddr;
		phdr->p_filesz	= phdr->p_memsz	= sysmem[i].size;
		phdr->p_align	= 0;
		dataoff += sysmem[i].size;
	}

	nhdr->p_offset	= offset;

	
	notes[0].name = CORE_STR;
	notes[0].type = NT_PRSTATUS;
	notes[0].datasz = sizeof(struct elf_prstatus);
	notes[0].data = &prstatus;

	memset(&prstatus, 0, sizeof(struct elf_prstatus));
	fill_prstatus(&prstatus, current, 0);
	if (regs)
		memcpy(&prstatus.pr_reg, regs, sizeof(*regs));
	else
		crash_setup_regs((struct pt_regs *)&prstatus.pr_reg, NULL);

	nhdr->p_filesz	= notesize(&notes[0]);
	bufp = storenote(&notes[0], bufp);

	
	notes[1].name	= CORE_STR;
	notes[1].type	= NT_PRPSINFO;
	notes[1].datasz	= sizeof(struct elf_prpsinfo);
	notes[1].data	= &prpsinfo;

	memset(&prpsinfo, 0, sizeof(struct elf_prpsinfo));
	fill_psinfo(&prpsinfo, current, current->mm);

	strcpy(prpsinfo.pr_fname, "vmlinux");
	strncpy(prpsinfo.pr_psargs, saved_command_line, ELF_PRARGSZ);

	nhdr->p_filesz	+= notesize(&notes[1]);
	bufp = storenote(&notes[1], bufp);

	
	notes[2].name	= CORE_STR;
	notes[2].type	= NT_TASKSTRUCT;
	notes[2].datasz	= sizeof(struct task_struct);
	notes[2].data	= current;

	printk("%s: data size is %d, data addr is %p",__func__,notes[2].datasz,notes[2].data);

	nhdr->p_filesz	+= notesize(&notes[2]);
	bufp = storenote(&notes[2], bufp);

	return;
} 

static void sysdump_prepare_info(int enter_id, const char *reason,
	struct pt_regs *regs)
{
	unsigned long long t;
	unsigned long nanosec_rem;
	int iocnt, i;
	char *iomem;

	strncpy(sprd_sysdump_extra.reason,
		reason, sizeof(sprd_sysdump_extra.reason));
	sprd_sysdump_extra.enter_id = enter_id;

#if 1 
#if defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY)
	
	
	if (sysdump_def_head_addr.magic1 == SYSDUMP_HEAD_ADDR_MAGIC1 && sysdump_def_head_addr.magic2 == SYSDUMP_HEAD_ADDR_MAGIC2) {
		sprd_sysdump_info = (struct sysdump_info *)sysdump_def_head_addr.head_addr;
	} else {
		sprd_sysdump_info = (struct sysdump_info *)ioremap(ELF_HEADER_ADDR, ELF_HEADER_SIZE);
	}
	printk("vaddr is %p,paddr is %p\n", sprd_sysdump_info, (void *)ELF_HEADER_ADDR);
#else
	sprd_sysdump_info = (struct sysdump_info *)phys_to_virt(SPRD_IO_MEM_BASE);
	printk("vaddr is %p,paddr is %p\n", sprd_sysdump_info, (void *)SPRD_IO_MEM_BASE);
#endif
#endif 
	memcpy(sprd_sysdump_info->magic, SYSDUMP_MAGIC,
			sizeof(sprd_sysdump_info->magic));

	t = cpu_clock(smp_processor_id());
	nanosec_rem = do_div(t, 1000000000);
	sprintf(sprd_sysdump_info->time, "%lu.%06lu", (unsigned long)t,
			nanosec_rem / 1000);

	memcpy(sprd_sysdump_info->dump_path, sysdump_conf.dump_path,
		sizeof(sprd_sysdump_info->dump_path));

	sprd_sysdump_info->dump_mem_paddr = virt_to_phys(sprd_dump_mem);
	sprd_sysdump_info->mem_num = sprd_dump_mem_num;
	sprd_sysdump_info->elfhdr_size = get_elfhdr_size(sprd_sysdump_info->mem_num);

	
	for (i = 0; i < sprd_dump_mem_num; i++) {
		if (SYSDUMP_RAM == sprd_dump_mem[i].type && 0 == sprd_dump_mem[i].size)
		sprd_dump_mem[i].size = CONFIG_PHYS_OFFSET +
						(num_physpages << PAGE_SHIFT) -
						sprd_dump_mem[i].paddr;

#if defined(LINUXS_BASE_ADDR2) && defined(TDS_BASE_ADDR)
		if (htc_get_config(HTC_DBG_FLAG_CP1_TD) & CP1_FLAG_MEM_LAYOUT_SM) {
			if ((!strcmp(sprd_dump_mem[i].name, "td_base_addr")) ||
					(!strcmp(sprd_dump_mem[i].name, "linux_base_addr2"))) {
				sprd_dump_mem[i].size = 0;
			}
		}
		else {
			if ((!strcmp(sprd_dump_mem[i].name, "tds_base_addr")) ||
					(!strcmp(sprd_dump_mem[i].name, "linuxs_base_addr2")))
				sprd_dump_mem[i].size = 0;
		}
#endif

#if defined(LINUXS_BASE_ADDR3) && defined(WS_BASE_ADDR)
		if (htc_get_config(HTC_DBG_FLAG_CP0_W) & CP0_FLAG_MEM_LAYOUT_SM) {
			if ((!strcmp(sprd_dump_mem[i].name, "w_base_addr")) ||
					(!strcmp(sprd_dump_mem[i].name, "linux_base_addr3"))) {
				sprd_dump_mem[i].size = 0;
			}
		}
		else {
			if ((!strcmp(sprd_dump_mem[i].name, "ws_base_addr")) ||
					(!strcmp(sprd_dump_mem[i].name, "linuxs_base_addr3")))
				sprd_dump_mem[i].size = 0;
		}
#endif

		if (!sysdump_conf.dump_modem && SYSDUMP_MODEM == sprd_dump_mem[i].type)
			sprd_dump_mem[i].size = 0;
	}

	sysdump_fill_core_hdr(regs,
		sprd_dump_mem,
		sprd_dump_mem_num,
		(char *)sprd_sysdump_info + sizeof(*sprd_sysdump_info),
		sprd_sysdump_info->mem_num + 1,
		sprd_sysdump_info->elfhdr_size);

	iocnt = 0;
	for (i = 0; i < sprd_dump_mem_num; i++) {
		
		if (0xffffffff != sprd_dump_mem[i].soff) {
			sprd_dump_mem[i].soff = iocnt;
			iomem = (char *)sprd_sysdump_info + sizeof(*sprd_sysdump_info) +
					sprd_sysdump_info->elfhdr_size + iocnt;
			memcpy(iomem,
				(void const *)(sprd_dump_mem[i].vaddr),
				sprd_dump_mem[i].size);
			iocnt += sprd_dump_mem[i].size;
		}
	}

	return;
}


struct sprd_debug_mmu_reg_t {
	int SCTLR;
	int TTBR0;
	int TTBR1;
	int TTBCR;
	int DACR;
	int DFSR;
	int DFAR;
	int IFSR;
	int IFAR;
	int DAFSR;
	int IAFSR;
	int PMRRR;
	int NMRRR;
	int FCSEPID;
	int CONTEXT;
	int URWTPID;
	int UROTPID;
	int POTPIDR;
};

struct sprd_debug_core_t {
	
	unsigned int r0;
	unsigned int r1;
	unsigned int r2;
	unsigned int r3;
	unsigned int r4;
	unsigned int r5;
	unsigned int r6;
	unsigned int r7;
	unsigned int r8;
	unsigned int r9;
	unsigned int r10;
	unsigned int r11;
	unsigned int r12;

	
	unsigned int r13_svc;
	unsigned int r14_svc;
	unsigned int spsr_svc;

	
	unsigned int pc;
	unsigned int cpsr;

	
	unsigned int r13_usr;
	unsigned int r14_usr;

	
	unsigned int r8_fiq;
	unsigned int r9_fiq;
	unsigned int r10_fiq;
	unsigned int r11_fiq;
	unsigned int r12_fiq;
	unsigned int r13_fiq;
	unsigned int r14_fiq;
	unsigned int spsr_fiq;

	
	unsigned int r13_irq;
	unsigned int r14_irq;
	unsigned int spsr_irq;

	
	unsigned int r13_mon;
	unsigned int r14_mon;
	unsigned int spsr_mon;

	
	unsigned int r13_abt;
	unsigned int r14_abt;
	unsigned int spsr_abt;

	
	unsigned int r13_und;
	unsigned int r14_und;
	unsigned int spsr_und;

};

DEFINE_PER_CPU(struct sprd_debug_core_t, sprd_debug_core_reg);
DEFINE_PER_CPU(struct sprd_debug_mmu_reg_t, sprd_debug_mmu_reg);


static inline void sprd_debug_save_core_reg(struct sprd_debug_core_t *core_reg)
{
	asm("str r0, [%0,#0]\n\t"	
			"mov r0, %0\n\t"		
			"str r1, [r0,#4]\n\t"	
			"str r2, [r0,#8]\n\t"	
			"str r3, [r0,#12]\n\t"	
			"str r4, [r0,#16]\n\t"	
			"str r5, [r0,#20]\n\t"	
			"str r6, [r0,#24]\n\t"	
			"str r7, [r0,#28]\n\t"	
			"str r8, [r0,#32]\n\t"	
			"str r9, [r0,#36]\n\t"	
			"str r10, [r0,#40]\n\t"	
			"str r11, [r0,#44]\n\t"	
			"str r12, [r0,#48]\n\t"	
			
			"str r13, [r0,#52]\n\t"	
			"str r14, [r0,#56]\n\t"	
			"mrs r1, spsr\n\t"		
			"str r1, [r0,#60]\n\t"
			
			"sub r1, r15, #0x4\n\t"	
			"str r1, [r0,#64]\n\t"
			"mrs r1, cpsr\n\t"		
			"str r1, [r0,#68]\n\t"
			
			"mrs r1, cpsr\n\t"		
			"and r1, r1, #0xFFFFFFE0\n\t"
			"orr r1, r1, #0x1f\n\t"
			"msr cpsr,r1\n\t"
			"str r13, [r0,#72]\n\t"	
			"str r14, [r0,#76]\n\t"	
			
			"mrs r1, cpsr\n\t"		
			"and r1,r1,#0xFFFFFFE0\n\t"
			"orr r1,r1,#0x11\n\t"
			"msr cpsr,r1\n\t"
			"str r8, [r0,#80]\n\t"	
			"str r9, [r0,#84]\n\t"	
			"str r10, [r0,#88]\n\t"	
			"str r11, [r0,#92]\n\t"	
			"str r12, [r0,#96]\n\t"	
			"str r13, [r0,#100]\n\t"	
			"str r14, [r0,#104]\n\t"	
			"mrs r1, spsr\n\t"		
			"str r1, [r0,#108]\n\t"
			
			"mrs r1, cpsr\n\t"		
			"and r1, r1, #0xFFFFFFE0\n\t"
			"orr r1, r1, #0x12\n\t"
			"msr cpsr,r1\n\t"
			"str r13, [r0,#112]\n\t"	
			"str r14, [r0,#116]\n\t"	
			"mrs r1, spsr\n\t"		
			"str r1, [r0,#120]\n\t"
			
			"mrs r1, cpsr\n\t"		
			"and r1, r1, #0xFFFFFFE0\n\t"
			"orr r1, r1, #0x16\n\t"
			"msr cpsr,r1\n\t"
			"str r13, [r0,#124]\n\t"	
			"str r14, [r0,#128]\n\t"	
			"mrs r1, spsr\n\t"		
			"str r1, [r0,#132]\n\t"
			
			"mrs r1, cpsr\n\t"		
			"and r1, r1, #0xFFFFFFE0\n\t"
			"orr r1, r1, #0x17\n\t"
			"msr cpsr,r1\n\t"
			"str r13, [r0,#136]\n\t"	
			"str r14, [r0,#140]\n\t"	
			"mrs r1, spsr\n\t"		
			"str r1, [r0,#144]\n\t"
			
			"mrs r1, cpsr\n\t"		
			"and r1, r1, #0xFFFFFFE0\n\t"
			"orr r1, r1, #0x1B\n\t"
			"msr cpsr,r1\n\t"
			"str r13, [r0,#148]\n\t"	
			"str r14, [r0,#152]\n\t"	
			"mrs r1, spsr\n\t"		
			"str r1, [r0,#156]\n\t"
			
			"mrs r1, cpsr\n\t"		
			"and r1, r1, #0xFFFFFFE0\n\t"
			"orr r1, r1, #0x13\n\t"
			"msr cpsr,r1\n\t" :		
			: "r"(core_reg)		
			: "%r0", "%r1"		
				);

	return;
}

static inline void sprd_debug_save_mmu_reg(struct sprd_debug_mmu_reg_t *mmu_reg)
{
	asm("mrc    p15, 0, r1, c1, c0, 0\n\t"	
			"str r1, [%0]\n\t"
			"mrc    p15, 0, r1, c2, c0, 0\n\t"	
			"str r1, [%0,#4]\n\t"
			"mrc    p15, 0, r1, c2, c0,1\n\t"	
			"str r1, [%0,#8]\n\t"
			"mrc    p15, 0, r1, c2, c0,2\n\t"	
			"str r1, [%0,#12]\n\t"
			"mrc    p15, 0, r1, c3, c0,0\n\t"	
			"str r1, [%0,#16]\n\t"
			"mrc    p15, 0, r1, c5, c0,0\n\t"	
			"str r1, [%0,#20]\n\t"
			"mrc    p15, 0, r1, c6, c0,0\n\t"	
			"str r1, [%0,#24]\n\t"
			"mrc    p15, 0, r1, c5, c0,1\n\t"	
			"str r1, [%0,#28]\n\t"
			"mrc    p15, 0, r1, c6, c0,2\n\t"	
			"str r1, [%0,#32]\n\t"
			
			"mrc    p15, 0, r1, c10, c2,0\n\t"	
			"str r1, [%0,#44]\n\t"
			"mrc    p15, 0, r1, c10, c2,1\n\t"	
			"str r1, [%0,#48]\n\t"
			"mrc    p15, 0, r1, c13, c0,0\n\t"	
			"str r1, [%0,#52]\n\t"
			"mrc    p15, 0, r1, c13, c0,1\n\t"	
			"str r1, [%0,#56]\n\t"
			"mrc    p15, 0, r1, c13, c0,2\n\t"	
			"str r1, [%0,#60]\n\t"
			"mrc    p15, 0, r1, c13, c0,3\n\t"	
			"str r1, [%0,#64]\n\t"
			"mrc    p15, 0, r1, c13, c0,4\n\t"	
			"str r1, [%0,#68]\n\t" :		
			: "r"(mmu_reg)			
			: "%r1", "memory"			
				);
}

static inline void sprd_debug_save_context(void)
{
	unsigned long flags;
	local_irq_save(flags);
	sprd_debug_save_mmu_reg(&per_cpu(sprd_debug_mmu_reg, smp_processor_id()));
	sprd_debug_save_core_reg(&per_cpu
			(sprd_debug_core_reg, smp_processor_id()));

	pr_emerg("(%s) context saved(CPU:%d)\n", __func__, smp_processor_id());
	local_irq_restore(flags);

	flush_cache_all();
}


extern void emergency_restart(void);
void sysdump_enter(int enter_id, const char *reason, struct pt_regs *regs)
{
	struct pt_regs * pregs;
	if (!sysdump_conf.enable)
		return;

	
	sprd_sysdump_extra.enter_cpu = smp_processor_id();

	pregs = &sprd_sysdump_extra.cpu_context[sprd_sysdump_extra.enter_cpu];
	if (regs)
		memcpy(pregs, regs, sizeof(*regs));
	else
		crash_setup_regs((struct pt_regs *)pregs, NULL);

	sprd_debug_save_context();

	smp_send_stop();
	mdelay(1000);

	printk("\n");
	printk("*****************************************************\n");
	printk("*                                                   *\n");
	printk("*  Sysdump enter, preparing debug info to dump ...  *\n");
	printk("*                                                   *\n");
	printk("*****************************************************\n");
	printk("\n");

	flush_cache_all();
	outer_flush_all();
	mdelay(1000);

	sysdump_prepare_info(enter_id, reason, regs);
	flush_cache_all();
	outer_flush_all();
	mdelay(1000);

	printk("\n");
	printk("*****************************************************\n");
	printk("*                                                   *\n");
	printk("*  Try to reboot system ...                         *\n");
	printk("*                                                   *\n");
	printk("*****************************************************\n");
	printk("\n");

#if defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_CP5DUG) || defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY) || defined(CONFIG_MACH_DUMMY)
		extern void machine_restart(char *cmd);
	machine_restart("ramdump");
#else
	emergency_restart();
#endif

	return;
}

void sysdump_ipi(struct pt_regs *regs)
{
	int cpu = smp_processor_id();

	if (sprd_sysdump_extra.enter_cpu != -1)
	{
		memcpy((void *)&(sprd_sysdump_extra.cpu_context[cpu]),
				(void *)regs, sizeof(struct pt_regs));
		sprd_debug_save_context();
	}
	return;
}


static ctl_table sysdump_sysctl_table[] = {
	{
		.procname       = "sysdump_enable",
		.data           = &sysdump_conf.enable,
		.maxlen         = sizeof(int),
		.mode           = 0644,
		.proc_handler   = proc_dointvec,
	},
	{
		.procname       = "sysdump_dump_modem",
		.data           = &sysdump_conf.dump_modem,
		.maxlen         = sizeof(int),
		.mode           = 0644,
		.proc_handler   = proc_dointvec,
	},
	{
		.procname       = "sysdump_reboot",
		.data           = &sysdump_conf.reboot,
		.maxlen         = sizeof(int),
		.mode           = 0644,
		.proc_handler   = proc_dointvec,
	},
	{
		.procname       = "sysdump_dump_path",
		.data           = sysdump_conf.dump_path,
		.maxlen         = sizeof(sysdump_conf.dump_path),
		.mode           = 0644,
		.proc_handler   = proc_dostring,
	},
	{}
};

static ctl_table sysdump_sysctl_root[] = {
	{
		.procname	= "kernel",
		.mode		= 0555,
		.child		= sysdump_sysctl_table,
	},
	{}
};


static struct ctl_table_header *sysdump_sysctl_hdr = NULL;

int sysdump_sysctl_init(void)
{
	sysdump_sysctl_hdr = register_sysctl_table(sysdump_sysctl_root);
	if (!sysdump_sysctl_hdr)
		return -ENOMEM;

#if 1 
	sysdump_def_head_addr.head_addr = (void *)ioremap(ELF_HEADER_ADDR, ELF_HEADER_SIZE);
	if (!sysdump_def_head_addr.head_addr) {
		sysdump_def_head_addr.magic1 = 0x0;
		sysdump_def_head_addr.magic2 = 0x0;
		return -ENOMEM;
	} else {
		sysdump_def_head_addr.magic1 = SYSDUMP_HEAD_ADDR_MAGIC1;
		sysdump_def_head_addr.magic2 = SYSDUMP_HEAD_ADDR_MAGIC2;
		printk("[SYSDUMP] initialize the default head address: vaddr is %p, paddr is %p\n",
			sysdump_def_head_addr.head_addr, ELF_HEADER_ADDR);
	}
#endif 

	return 0;
}

void sysdump_sysctl_exit(void)
{
	if (sysdump_sysctl_hdr)
		unregister_sysctl_table(sysdump_sysctl_hdr);
}

module_init(sysdump_sysctl_init);
module_exit(sysdump_sysctl_exit);

MODULE_AUTHOR("Jianjun.He <jianjun.he@spreadtrum.com>");
MODULE_DESCRIPTION("kernel core dump for Spreadtrum");
MODULE_LICENSE("GPL");
