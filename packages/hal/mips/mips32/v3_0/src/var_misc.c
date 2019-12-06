//==========================================================================
//
//      var_misc.c
//
//      HAL implementation miscellaneous functions
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####
// -------------------------------------------
// This file is part of eCos, the Embedded Configurable Operating System.
// Copyright (C) 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
//
// eCos is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 or (at your option) any later
// version.
//
// eCos is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License
// along with eCos; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// As a special exception, if other files instantiate templates or use
// macros or inline functions from this file, or you compile this file
// and link it with other works to produce a work based on this file,
// this file does not by itself cause the resulting work to be covered by
// the GNU General Public License. However the source code for this file
// must still be made available in accordance with section (3) of the GNU
// General Public License v2.
//
// This exception does not invalidate any other reasons why a work based
// on this file might be covered by the GNU General Public License.
// -------------------------------------------
// ####ECOSGPLCOPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    nickg
// Contributors: nickg, jlarmour, dmoseley
// Date:         2000-07-14
// Purpose:      HAL miscellaneous functions
// Description:  This file contains miscellaneous functions provided by the
//               HAL.
//
//####DESCRIPTIONEND####
//
//========================================================================*/

#include <pkgconf/hal.h>

#include <cyg/infra/cyg_type.h>         // Base types
#include <cyg/infra/cyg_trac.h>         // tracing macros
#include <cyg/infra/cyg_ass.h>          // assertion macros

#include <cyg/hal/hal_intr.h>

#define CYGARC_HAL_COMMON_EXPORT_CPU_MACROS
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/var_arch.h>
#include <cyg/hal/plf_io.h>
#include <cyg/hal/hal_cache.h>

/*------------------------------------------------------------------------*/
// Array which stores the configured priority levels for the configured
// interrupts.

volatile CYG_BYTE hal_interrupt_level[CYGNUM_HAL_ISR_COUNT];

/*------------------------------------------------------------------------*/

void hal_variant_init(void)
{
	//diag_printf("%s\n",__FUNCTION__);

}

/*
 * Uncomment the following to allow for dynamic cache sizing.
 * Currently we are going to assume the exact part specified in the ecosconfig stuff.
 * Perhaps in the near future this can all be done dynamically.
 */
/* define DYNAMIC_CACHE_SIZING */

#if 0
#ifndef DYNAMIC_CACHE_SIZING
#warning "                                                                           \n\
STILL NEED TO IMPLEMENT DYNAMIC_CACHE_SIZING.                                        \n\
ALSO, the HAL_PLATFORM_CPU/etc defines need to be dynamic.                           \n\
ALSO, need to do big endian stuff as well.                                           \n\
Determine if network debug is necessary.                                             \n\
Remove MIPS memc_init code"
#endif
#endif

/*------------------------------------------------------------------------*/
// Initialize the caches

int hal_init_icache(unsigned long config1_val)
{
#ifdef DYNAMIC_CACHE_SIZING
  int icache_linesize, icache_assoc, icache_sets, icache_lines, icache_size;
  unsigned long cache_addr;

  switch (config1_val & CONFIG1_IL)
    {
    case CONFIG1_ICACHE_LINE_SIZE_16_BYTES: icache_linesize = 16;      break;
    case CONFIG1_ICACHE_NOT_PRESET:         return -1;                 break;
    default:      /* Error */               return -1;                 break;
    }

  switch (config1_val & CONFIG1_IA)
    {
    case CONFIG1_ICACHE_DIRECT_MAPPED:      icache_assoc = 1;          break;
    case CONFIG1_ICACHE_2_WAY:              icache_assoc = 2;          break;
    case CONFIG1_ICACHE_3_WAY:              icache_assoc = 3;          break;
    case CONFIG1_ICACHE_4_WAY:              icache_assoc = 4;          break;
    default:      /* Error */               return -1;                 break;
    }

  switch (config1_val & CONFIG1_IS)
    {
    case CONFIG1_ICACHE_64_SETS_PER_WAY:    icache_sets = 64;          break;
    case CONFIG1_ICACHE_128_SETS_PER_WAY:   icache_sets = 128;         break;
    case CONFIG1_ICACHE_256_SETS_PER_WAY:   icache_sets = 256;         break;
    default:      /* Error */               return -1;                 break;
    }

  icache_lines = icache_sets * icache_assoc;
  icache_size = icache_lines * icache_linesize;
#endif /* DYNAMIC_CACHE_SIZING */

  /*
   * Reset does not invalidate the cache so let's do so now.
   */
  HAL_ICACHE_INVALIDATE_ALL();

#ifdef DYNAMIC_CACHE_SIZING
  return icache_size;
#else
  return HAL_ICACHE_SIZE;
#endif
}

int hal_init_dcache(unsigned long config1_val)
{
#ifdef DYNAMIC_CACHE_SIZING
  int dcache_linesize, dcache_assoc, dcache_sets, dcache_lines, dcache_size;

  switch (config1_val & CONFIG1_DL)
    {
    case CONFIG1_DCACHE_LINE_SIZE_16_BYTES: dcache_linesize = 16;      break;
    case CONFIG1_DCACHE_NOT_PRESET:         return -1;                 break;
    default:      /* Error */               return -1;                 break;
    }

  switch (config1_val & CONFIG1_DA)
    {
    case CONFIG1_DCACHE_DIRECT_MAPPED:      dcache_assoc = 1;          break;
    case CONFIG1_DCACHE_2_WAY:              dcache_assoc = 2;          break;
    case CONFIG1_DCACHE_3_WAY:              dcache_assoc = 3;          break;
    case CONFIG1_DCACHE_4_WAY:              dcache_assoc = 4;          break;
    default:      /* Error */               return -1;                 break;
    }

  switch (config1_val & CONFIG1_DS)
    {
    case CONFIG1_DCACHE_64_SETS_PER_WAY:    dcache_sets = 64;          break;
    case CONFIG1_DCACHE_128_SETS_PER_WAY:   dcache_sets = 128;         break;
    case CONFIG1_DCACHE_256_SETS_PER_WAY:   dcache_sets = 256;         break;
    default:      /* Error */               return -1;                 break;
    }

  dcache_lines = dcache_sets * dcache_assoc;
  dcache_size = dcache_lines * dcache_linesize;
#endif /* DYNAMIC_CACHE_SIZING */

  /*
   * Reset does not invalidate the cache so let's do so now.
   */

 #ifdef   CYG_HAL_STARTUP_RAM
  HAL_DCACHE_SYNC();
 #else
  HAL_DCACHE_INVALIDATE_ALL();
#endif

#ifdef DYNAMIC_CACHE_SIZING
  return dcache_size;
#else
  return HAL_DCACHE_SIZE;
#endif
}

void hal_c_cache_init(unsigned long config1_val)
{
    unsigned val;

    if (hal_init_icache(config1_val) == -1)
    {
        /* Error */
        ;
    }

    if (hal_init_dcache(config1_val) == -1)
    {
        /* Error */
        ;
    }

    // enable cached KSEG0
    asm volatile("mfc0 %0,$16;" : "=r"(val));
    val &= ~7;
    val |= 3;
    asm volatile("mtc0 %0,$16;" : : "r"(val));
}

void hal_icache_sync(void)
{
    HAL_ICACHE_INVALIDATE_ALL();
}

void hal_dcache_sync(void)
{
    HAL_DCACHE_INVALIDATE_ALL();
}


/*------------------------------------------------------------------------*/
/* Exception                                                              */
static const cyg_uint8 *__greg_name[] = {"z0", "at", "v0", "v1", "a0", "a1", "a2", "a3",
		"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
		"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
		"t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"};
static const cyg_uint8 *__except_name[] = {"Int", "Mod", "TLBL", "TLBS", "AdEL", "AdES", "IBE", "DBE",
		"Sys", "Bp", "RI", "CpU", "Ov", "TR", "E14", "FPE",
		"E16", "E17", "C2E", "E19", "E20", "E21", "MDMX", "WATCH",
		"Mcheck", "Thread", "E26", "E27", "E28", "E29", "CacheErr", "E31"};

#include "inst.h"

struct stackframe {
	unsigned long sp;
	unsigned long pc;
	unsigned long ra;
};

static inline int get_mem(unsigned long addr, unsigned long *result)
{
//	unsigned long *address = (unsigned long *) addr;
	memcpy(result, (unsigned long *)addr, 4);
	return 0;
}

/*
 * These two instruction helpers were taken from process.c
 */
static inline int is_ra_save_ins(union mips_instruction *ip)
{
	/* sw / sd $ra, offset($sp) */
	return (ip->i_format.opcode == sw_op || ip->i_format.opcode == sd_op)
		&& ip->i_format.rs == 29 && ip->i_format.rt == 31;
}

static inline int is_sp_move_ins(union mips_instruction *ip)
{
	/* addiu/daddiu sp,sp,-imm */
	if (ip->i_format.rs != 29 || ip->i_format.rt != 29)
		return 0;
	if (ip->i_format.opcode == addiu_op || ip->i_format.opcode == daddiu_op)
		return 1;
	return 0;
}

/*
 * Looks for specific instructions that mark the end of a function.
 * This usually means we ran into the code area of the previous function.
 */
static inline int is_end_of_function_marker(union mips_instruction *ip)
{
	/* jr ra */
	if (ip->r_format.func == jr_op && ip->r_format.rs == 31)
		return 1;
	/* lui gp */
	if (ip->i_format.opcode == lui_op && ip->i_format.rt == 28)
		return 1;
	return 0;
}

/*
 * TODO for userspace stack unwinding:
 * - handle cases where the stack is adjusted inside a function
 *     (generally doesn't happen)
 * - find optimal value for max_instr_check
 * - try to find a way to handle leaf functions
 */

static inline int unwind_user_frame(struct stackframe *old_frame,
				    const unsigned int max_instr_check)
{
	struct stackframe new_frame = *old_frame;
	unsigned long ra_offset = 0;
	size_t stack_size = 0;
	unsigned long addr;

	if (old_frame->pc == 0 || old_frame->sp == 0 || old_frame->ra == 0)
		return -9;

	for (addr = new_frame.pc; (addr + max_instr_check > new_frame.pc)
		&& (!ra_offset || !stack_size); --addr) {
		union mips_instruction ip;

		if (get_mem(addr, (unsigned long *) &ip))
			return -11;

		if (is_sp_move_ins(&ip)) {
			int stack_adjustment = ip.i_format.simmediate;
			if (stack_adjustment > 0)
				// This marks the end of the previous function,  which means we overran.
				break;
			stack_size = (unsigned) stack_adjustment;
		} else if (is_ra_save_ins(&ip)) {
			int ra_slot = ip.i_format.simmediate;
			if (ra_slot < 0)
				// This shouldn't happen.
				break;
			ra_offset = ra_slot;
		} else if (is_end_of_function_marker(&ip))
			break;
	}

	if (!ra_offset || !stack_size)
		return -1;

	if (ra_offset) {
		new_frame.ra = old_frame->sp + ra_offset;
		if (get_mem(new_frame.ra, &(new_frame.ra)))
			return -13;
	}

	if (stack_size) {
		new_frame.sp = old_frame->sp + stack_size;
		if (get_mem(new_frame.sp, &(new_frame.sp)))
			return -14;
	}

	if (new_frame.sp > old_frame->sp)
		return -2;

	new_frame.pc = old_frame->ra;
	*old_frame = new_frame;

	return 0;
}

extern int os_show_mem(int argc , char* argv[]);

volatile int _wait_in_except = 0;
void hal_mips_exception_dump(CYG_ADDRWORD data, int except_num, CYG_ADDRWORD info)
{
	int i;
	HAL_SavedRegisters *preg = (HAL_SavedRegisters *) info;

	diag_printf("\n\nExcept %d: %s\n", except_num, __except_name[except_num]);
	for (i=0; i<32; i+= 4)
		diag_printf("    %s=%08x %s=%08x %s=%08x %s=%08x\n",
			__greg_name[i], preg->d[i], __greg_name[i+1], preg->d[i+1],
			__greg_name[i+2], preg->d[i+2], __greg_name[i+3], preg->d[i+3]);
//	diag_printf("    hi=%08x lo=%08x\n\n");
	diag_printf("    pc=%08x sr=%08x cause=%08x, badva=%08x\n\n",
		preg->pc, preg->sr, preg->cause, preg->badvr);

	{
		extern unsigned int _stext;
		extern unsigned int _etext;
		unsigned long sp = preg->d[29];
		unsigned long fp = preg->d[30];   //sp > fp
		unsigned long pc = preg->pc;
		unsigned int s = &_stext;
		unsigned int e = &_etext;

		struct stackframe frame;
		frame.pc = preg->pc;
		frame.sp = preg->d[29];
		frame.ra = preg->d[31];
		int depth = 5;

        diag_dump_buf_with_offset((unsigned char *)frame.sp-512, 512, (unsigned char *)frame.sp-512);

		diag_printf("call trace:\n");
		while (!unwind_user_frame(&frame, 512))
		{
			diag_printf("pc = %08x, sp = %08x, ra = %08x\n", frame.pc, frame.sp, frame.ra);
			depth--;
			if (depth <= 0)
			{
				break;
			}
		}
	}

	diag_printf("System halted\n");

	//added by haitao,dump mem usage
	os_show_mem(1,NULL);
	
#define MT7628_RSTCTL	0xb0000034
#define MON_CMD_REBOOT      0x02   //reboot

	HAL_REG32(MT7628_RSTCTL) = 0xffffffff;

	_wait_in_except = 1;
	/*  Halt the system  */
	while(_wait_in_except) {
		mon_snd_cmd(MON_CMD_REBOOT);
	}
}


/*------------------------------------------------------------------------*/
/* End of var_misc.c                                                      */

