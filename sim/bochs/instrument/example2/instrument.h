/////////////////////////////////////////////////////////////////////////
// $Id: instrument.h,v 1.1 2009/11/04 15:48:28 sshwarts Exp $
/////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2009 Stanislav Shwartsman
//          Written by Stanislav Shwartsman [sshwarts at sourceforge net]
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA


// possible types passed to BX_INSTR_TLB_CNTRL()
#define BX_INSTR_MOV_CR3      10
#define BX_INSTR_INVLPG       11
#define BX_INSTR_TASKSWITCH   12

// possible types passed to BX_INSTR_CACHE_CNTRL()
#define BX_INSTR_INVD         20
#define BX_INSTR_WBINVD       21

// possible types passed to BX_INSTR_FAR_BRANCH()
#define BX_INSTR_IS_CALL      10
#define BX_INSTR_IS_RET       11
#define BX_INSTR_IS_IRET      12
#define BX_INSTR_IS_JMP       13
#define BX_INSTR_IS_INT       14
#define BX_INSTR_IS_SYSCALL   15
#define BX_INSTR_IS_SYSRET    16
#define BX_INSTR_IS_SYSENTER  17
#define BX_INSTR_IS_SYSEXIT   18

// possible types passed to BX_INSTR_PREFETCH_HINT()
#define BX_INSTR_PREFETCH_NTA 0
#define BX_INSTR_PREFETCH_T0  1
#define BX_INSTR_PREFETCH_T1  2
#define BX_INSTR_PREFETCH_T2  3

#define BX_INSTRUMENT_IA_OPCODE 1

#if BX_INSTRUMENTATION

class bxInstruction_c;

// called from the CPU core

void bx_instr_initialize(unsigned cpu);
void bx_instr_reset(unsigned cpu, unsigned type);

void bx_instr_interrupt(unsigned cpu, unsigned vector);
void bx_instr_exception(unsigned cpu, unsigned vector, unsigned error_code);
void bx_instr_hwinterrupt(unsigned cpu, unsigned vector, Bit16u cs, bx_address eip);

void bx_instr_before_execution(unsigned cpu, bxInstruction_c *i);

/* initialization/deinitialization of instrumentalization*/
#define BX_INSTR_INIT_ENV()
#define BX_INSTR_EXIT_ENV()

/* simulation init, shutdown, reset */
#define BX_INSTR_INITIALIZE(cpu_id)      bx_instr_initialize(cpu_id)
#define BX_INSTR_EXIT(cpu_id)
#define BX_INSTR_RESET(cpu_id, type)     bx_instr_reset(cpu_id, type)
#define BX_INSTR_HLT(cpu_id)
#define BX_INSTR_MWAIT(cpu_id, addr, len, flags)
#define BX_INSTR_NEW_INSTRUCTION(cpu_id)

/* called from command line debugger */
#define BX_INSTR_DEBUG_PROMPT()
#define BX_INSTR_DEBUG_CMD(cmd)

/* branch resoultion */
#define BX_INSTR_CNEAR_BRANCH_TAKEN(cpu_id, new_eip)
#define BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(cpu_id)
#define BX_INSTR_UCNEAR_BRANCH(cpu_id, what, new_eip)
#define BX_INSTR_FAR_BRANCH(cpu_id, what, new_cs, new_eip)

/* decoding completed */
#define BX_INSTR_OPCODE(cpu_id, opcode, len, is32, is64)

/* exceptional case and interrupt */
#define BX_INSTR_EXCEPTION(cpu_id, vector, error_code) \
                       bx_instr_exception(cpu_id, vector, error_code)

#define BX_INSTR_INTERRUPT(cpu_id, vector) bx_instr_interrupt(cpu_id, vector)
#define BX_INSTR_HWINTERRUPT(cpu_id, vector, cs, eip) bx_instr_hwinterrupt(cpu_id, vector, cs, eip)

/* TLB/CACHE control instruction executed */
#define BX_INSTR_CLFLUSH(cpu_id, laddr, paddr)
#define BX_INSTR_CACHE_CNTRL(cpu_id, what)
#define BX_INSTR_TLB_CNTRL(cpu_id, what, new_cr3)
#define BX_INSTR_PREFETCH_HINT(cpu_id, what, seg, offset)

/* execution */
#define BX_INSTR_BEFORE_EXECUTION(cpu_id, i) bx_instr_before_execution(cpu_id, i)
   
#define BX_INSTR_AFTER_EXECUTION(cpu_id, i)
#define BX_INSTR_REPEAT_ITERATION(cpu_id, i)

/* memory access */
#define BX_INSTR_LIN_ACCESS(cpu_id, lin, phy, len, rw)

/* memory access */
#define BX_INSTR_MEM_DATA_ACCESS(cpu_id, seg, offset, len, rw)

/* called from memory object */
#define BX_INSTR_PHY_WRITE(cpu_id, addr, len)
#define BX_INSTR_PHY_READ(cpu_id, addr, len)

/* feedback from device units */
#define BX_INSTR_INP(addr, len)
#define BX_INSTR_INP2(addr, len, val)
#define BX_INSTR_OUTP(addr, len, val)

/* wrmsr callback */
#define BX_INSTR_WRMSR(cpu_id, addr, value)

#else // BX_INSTRUMENTATION

/* initialization/deinitialization of instrumentalization */
#define BX_INSTR_INIT_ENV()
#define BX_INSTR_EXIT_ENV()

/* simulation init, shutdown, reset */
#define BX_INSTR_INITIALIZE(cpu_id)
#define BX_INSTR_EXIT(cpu_id)
#define BX_INSTR_RESET(cpu_id, type)
#define BX_INSTR_HLT(cpu_id)
#define BX_INSTR_MWAIT(cpu_id, addr, len, flags)
#define BX_INSTR_NEW_INSTRUCTION(cpu_id)

/* called from command line debugger */
#define BX_INSTR_DEBUG_PROMPT()
#define BX_INSTR_DEBUG_CMD(cmd)

/* branch resoultion */
#define BX_INSTR_CNEAR_BRANCH_TAKEN(cpu_id, new_eip)
#define BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(cpu_id)
#define BX_INSTR_UCNEAR_BRANCH(cpu_id, what, new_eip)
#define BX_INSTR_FAR_BRANCH(cpu_id, what, new_cs, new_eip)

/* decoding completed */
#define BX_INSTR_OPCODE(cpu_id, opcode, len, is32, is64)

/* exceptional case and interrupt */
#define BX_INSTR_EXCEPTION(cpu_id, vector, error_code)
#define BX_INSTR_INTERRUPT(cpu_id, vector)
#define BX_INSTR_HWINTERRUPT(cpu_id, vector, cs, eip)

/* TLB/CACHE control instruction executed */
#define BX_INSTR_CLFLUSH(cpu_id, laddr, paddr)
#define BX_INSTR_CACHE_CNTRL(cpu_id, what)
#define BX_INSTR_TLB_CNTRL(cpu_id, what, new_cr3)
#define BX_INSTR_PREFETCH_HINT(cpu_id, what, seg, offset)

/* execution */
#define BX_INSTR_BEFORE_EXECUTION(cpu_id, i)
#define BX_INSTR_AFTER_EXECUTION(cpu_id, i)
#define BX_INSTR_REPEAT_ITERATION(cpu_id, i)

/* memory access */
#define BX_INSTR_LIN_ACCESS(cpu_id, lin, phy, len, rw)

/* memory access */
#define BX_INSTR_MEM_DATA_ACCESS(cpu_id, seg, offset, len, rw)

/* called from memory object */
#define BX_INSTR_PHY_WRITE(cpu_id, addr, len)
#define BX_INSTR_PHY_READ(cpu_id, addr, len)

/* feedback from device units */
#define BX_INSTR_INP(addr, len)
#define BX_INSTR_INP2(addr, len, val)
#define BX_INSTR_OUTP(addr, len, val)

/* wrmsr callback */
#define BX_INSTR_WRMSR(cpu_id, addr, value)

#endif // BX_INSTRUMENTATION
