/////////////////////////////////////////////////////////////////////////
// $Id: vmx.cc,v 1.26 2009/10/08 14:33:08 sshwarts Exp $
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
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA B 02110-1301 USA
//
/////////////////////////////////////////////////////////////////////////

#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#include "iodev/iodev.h"

#if BX_SUPPORT_X86_64==0
#define RIP EIP
#define RSP ESP
#endif

#if BX_SUPPORT_VMX

#define VMCSPTR_VALID() (BX_CPU_THIS_PTR vmcsptr != BX_INVALID_VMCSPTR)

////////////////////////////////////////////////////////////
// VMCS access
////////////////////////////////////////////////////////////

static unsigned vmcs_map[16][1+VMX_HIGHEST_VMCS_ENCODING];

void BX_CPU_C::init_VMCS(void)
{
  static bx_bool vmcs_map_ready = 0;

  if (vmcs_map_ready) return;
  vmcs_map_ready = 1;

  for (unsigned type=0; type<16; type++) {
    for (unsigned field=0; field <= VMX_HIGHEST_VMCS_ENCODING; field++) {
       vmcs_map[type][field] = 0xffffffff;
    }
  }

#if 1
  // try to build generic VMCS map
  for (unsigned type=0; type<16; type++) {
    for (unsigned field=0; field <= VMX_HIGHEST_VMCS_ENCODING; field++) {
       // allocate 32 fields of 4 byte each per type
       if (vmcs_map[type][field] != 0xffffffff) {
          BX_PANIC(("VMCS type %d field %d is already initialized", type, field));
       }
       vmcs_map[type][field] = VMCS_DATA_OFFSET + (type*64 + field) * 4;
       if(vmcs_map[type][field] >= VMX_VMCS_AREA_SIZE) {
          BX_PANIC(("VMCS type %d field %d is out of VMCS boundaries", type, field));
       }
    }
  }
#else
  // define your own VMCS format
#include "vmcs.h"
#endif
}

void BX_CPU_C::set_VMCSPTR(Bit64u vmxptr)
{
  BX_CPU_THIS_PTR vmcsptr = vmxptr;

  if (vmxptr != BX_INVALID_VMCSPTR)
    BX_CPU_THIS_PTR vmcshostptr = (bx_hostpageaddr_t) BX_MEM(0)->getHostMemAddr(BX_CPU_THIS, vmxptr, BX_WRITE);
  else
    BX_CPU_THIS_PTR vmcshostptr = 0;
}

Bit16u BX_CPU_C::VMread16(unsigned encoding)
{
  Bit16u field;

  unsigned offset = vmcs_map[VMCS_FIELD_INDEX(encoding)][VMCS_FIELD(encoding)];
  if(offset >= VMX_VMCS_AREA_SIZE)
    BX_PANIC(("VMread16: can't access encoding 0x%08x, offset=0x%x", encoding, offset));
  bx_phy_address pAddr = BX_CPU_THIS_PTR vmcsptr + offset;

  BX_ASSERT(VMCS_FIELD_WIDTH(encoding) == VMCS_FIELD_WIDTH_16BIT);

  if (BX_CPU_THIS_PTR vmcshostptr) {
    Bit16u *hostAddr = (Bit16u*) (BX_CPU_THIS_PTR vmcshostptr | offset);
    ReadHostDWordFromLittleEndian(hostAddr, field);
  }
  else {
    access_read_physical(pAddr, 2, (Bit8u*)(&field));
  }

  BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID, pAddr, 2, BX_READ, (Bit8u*)(&field));

  return field;
}

// write 16-bit value into VMCS 16-bit field
void BX_CPU_C::VMwrite16(unsigned encoding, Bit16u val_16)
{
  unsigned offset = vmcs_map[VMCS_FIELD_INDEX(encoding)][VMCS_FIELD(encoding)];
  if(offset >= VMX_VMCS_AREA_SIZE)
    BX_PANIC(("VMwrite16: can't access encoding 0x%08x, offset=0x%x", encoding, offset));
  bx_phy_address pAddr = BX_CPU_THIS_PTR vmcsptr + offset;

  BX_ASSERT(VMCS_FIELD_WIDTH(encoding) == VMCS_FIELD_WIDTH_16BIT);

  if (BX_CPU_THIS_PTR vmcshostptr) {
    Bit16u *hostAddr = (Bit16u*) (BX_CPU_THIS_PTR vmcshostptr | offset);
    pageWriteStampTable.decWriteStamp(pAddr);
    WriteHostWordToLittleEndian(hostAddr, val_16);
  }
  else {
    access_write_physical(pAddr, 2, (Bit8u*)(&val_16));
  }

  BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID, pAddr, 2, BX_WRITE, (Bit8u*)(&val_16));
}

Bit32u BX_CPU_C::VMread32(unsigned encoding)
{
  Bit32u field;

  unsigned offset = vmcs_map[VMCS_FIELD_INDEX(encoding)][VMCS_FIELD(encoding)];
  if(offset >= VMX_VMCS_AREA_SIZE)
    BX_PANIC(("VMread32: can't access encoding 0x%08x, offset=0x%x", encoding, offset));
  bx_phy_address pAddr = BX_CPU_THIS_PTR vmcsptr + offset;

  if (BX_CPU_THIS_PTR vmcshostptr) {
    Bit32u *hostAddr = (Bit32u*) (BX_CPU_THIS_PTR vmcshostptr | offset);
    ReadHostDWordFromLittleEndian(hostAddr, field);
  }
  else {
    access_read_physical(pAddr, 4, (Bit8u*)(&field));
  }

  BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID, pAddr, 4, BX_READ, (Bit8u*)(&field));

  return field;
}

// write 32-bit value into VMCS field
void BX_CPU_C::VMwrite32(unsigned encoding, Bit32u val_32)
{
  unsigned offset = vmcs_map[VMCS_FIELD_INDEX(encoding)][VMCS_FIELD(encoding)];
  if(offset >= VMX_VMCS_AREA_SIZE)
    BX_PANIC(("VMwrite32: can't access encoding 0x%08x, offset=0x%x", encoding, offset));
  bx_phy_address pAddr = BX_CPU_THIS_PTR vmcsptr + offset;

  if (BX_CPU_THIS_PTR vmcshostptr) {
    Bit32u *hostAddr = (Bit32u*) (BX_CPU_THIS_PTR vmcshostptr | offset);
    pageWriteStampTable.decWriteStamp(pAddr);
    WriteHostDWordToLittleEndian(hostAddr, val_32);
  }
  else {
    access_write_physical(pAddr, 4, (Bit8u*)(&val_32));
  }

  BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID, pAddr, 4, BX_WRITE, (Bit8u*)(&val_32));
}

Bit64u BX_CPU_C::VMread64(unsigned encoding)
{
  BX_ASSERT(!IS_VMCS_FIELD_HI(encoding));

  Bit64u field;

  unsigned offset = vmcs_map[VMCS_FIELD_INDEX(encoding)][VMCS_FIELD(encoding)];
  if(offset >= VMX_VMCS_AREA_SIZE)
    BX_PANIC(("VMread64: can't access encoding 0x%08x, offset=0x%x", encoding, offset));
  bx_phy_address pAddr = BX_CPU_THIS_PTR vmcsptr + offset;

  if (BX_CPU_THIS_PTR vmcshostptr) {
    Bit64u *hostAddr = (Bit64u*) (BX_CPU_THIS_PTR vmcshostptr | offset);
    ReadHostQWordFromLittleEndian(hostAddr, field);
  }
  else {
    access_read_physical(pAddr, 8, (Bit8u*)(&field));
  }

  BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID, pAddr, 8, BX_READ, (Bit8u*)(&field));

  return field;
}

// write 64-bit value into VMCS field
void BX_CPU_C::VMwrite64(unsigned encoding, Bit64u val_64)
{
  BX_ASSERT(!IS_VMCS_FIELD_HI(encoding));

  unsigned offset = vmcs_map[VMCS_FIELD_INDEX(encoding)][VMCS_FIELD(encoding)];
  if(offset >= VMX_VMCS_AREA_SIZE)
    BX_PANIC(("VMwrite64: can't access encoding 0x%08x, offset=0x%x", encoding, offset));
  bx_phy_address pAddr = BX_CPU_THIS_PTR vmcsptr + offset;

  if (BX_CPU_THIS_PTR vmcshostptr) {
    Bit64u *hostAddr = (Bit64u*) (BX_CPU_THIS_PTR vmcshostptr | offset);
    pageWriteStampTable.decWriteStamp(pAddr);
    WriteHostQWordToLittleEndian(hostAddr, val_64);
  }
  else {
    access_write_physical(pAddr, 8, (Bit8u*)(&val_64));
  }

  BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID, pAddr, 8, BX_WRITE, (Bit8u*)(&val_64));
}

////////////////////////////////////////////////////////////
// VMfail/VMsucceed
////////////////////////////////////////////////////////////

BX_CPP_INLINE void BX_CPU_C::VMsucceed(void)
{
  setEFlagsOSZAPC(0);
}

BX_CPP_INLINE void BX_CPU_C::VMfailInvalid(void)
{
  setEFlagsOSZAPC(EFlagsCFMask);
}

BX_CPP_INLINE void BX_CPU_C::VMfail(Bit32u error_code)
{
  if (VMCSPTR_VALID()) { // executed only if there is a current VMCS
     setEFlagsOSZAPC(EFlagsZFMask);
     VMwrite32(VMCS_32BIT_INSTRUCTION_ERROR, error_code);
  }
  else {
     setEFlagsOSZAPC(EFlagsCFMask);
  }
}

void BX_CPU_C::VMabort(VMX_vmabort_code error_code)
{
  Bit32u abort = error_code;
  bx_phy_address pAddr = BX_CPU_THIS_PTR vmcsptr + VMCS_VMX_ABORT_FIELD_ADDR;
  access_write_physical(pAddr, 4, &abort);
  BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID, pAddr, 4, BX_WRITE, (Bit8u*)(&abort));
  shutdown();
}

unsigned BX_CPU_C::VMXReadRevisionID(bx_phy_address pAddr)
{
  Bit32u revision;
  access_read_physical(pAddr + VMCS_REVISION_ID_FIELD_ADDR, 4, &revision);
  BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID, pAddr + VMCS_REVISION_ID_FIELD_ADDR, 4,
          BX_READ, (Bit8u*)(&revision));

  return revision;
}

////////////////////////////////////////////////////////////
// VMenter
////////////////////////////////////////////////////////////

extern struct BxExceptionInfo exceptions_info[];

#define VMENTRY_INJECTING_EVENT(vmentry_interr_info) (vmentry_interr_info & 0x80000000)

VMX_error_code BX_CPU_C::VMenterLoadCheckVmControls(void)
{
  VMCS_CACHE *vm = &BX_CPU_THIS_PTR vmcs;

  //
  // Load VM-execution control fields to VMCS Cache
  //

  vm->vmexec_ctrls1 = VMread32(VMCS_32BIT_CONTROL_PIN_BASED_EXEC_CONTROLS);
  vm->vmexec_ctrls2 = VMread32(VMCS_32BIT_CONTROL_PROCESSOR_BASED_EXEC_CONTROLS);
  vm->vm_exceptions_bitmap = VMread32(VMCS_32BIT_CONTROL_EXECUTION_BITMAP);
  vm->vm_pf_mask = VMread32(VMCS_32BIT_CONTROL_PAGE_FAULT_ERR_CODE_MASK);
  vm->vm_pf_match = VMread32(VMCS_32BIT_CONTROL_PAGE_FAULT_ERR_CODE_MATCH);
  vm->io_bitmap_addr[0] = VMread64(VMCS_64BIT_CONTROL_IO_BITMAP_A);
  vm->io_bitmap_addr[1] = VMread64(VMCS_64BIT_CONTROL_IO_BITMAP_B);
  vm->tsc_offset = VMread64(VMCS_64BIT_CONTROL_TSC_OFFSET);
  vm->msr_bitmap_addr = (bx_phy_address) VMread64(VMCS_64BIT_CONTROL_MSR_BITMAPS);
  vm->vm_cr0_mask = VMread64(VMCS_CONTROL_CR0_GUEST_HOST_MASK);
  vm->vm_cr4_mask = VMread64(VMCS_CONTROL_CR4_GUEST_HOST_MASK);
  vm->vm_cr0_read_shadow = VMread64(VMCS_CONTROL_CR0_READ_SHADOW);
  vm->vm_cr4_read_shadow = VMread64(VMCS_CONTROL_CR4_READ_SHADOW);

  vm->vm_cr3_target_cnt = VMread32(VMCS_32BIT_CONTROL_CR3_TARGET_COUNT);
  for (int n=0; n<VMX_CR3_TARGET_MAX_CNT; n++)
    vm->vm_cr3_target_value[n] = VMread64(VMCS_CR3_TARGET0 + 2*n);

  vm->vm_tpr_threshold = VMread32(VMCS_32BIT_CONTROL_TPR_THRESHOLD);
  vm->virtual_apic_page_addr = (bx_phy_address) VMread64(VMCS_64BIT_CONTROL_VIRTUAL_APIC_PAGE_ADDR);
  vm->executive_vmcsptr = (bx_phy_address) VMread64(VMCS_64BIT_CONTROL_EXECUTIVE_VMCS_PTR);

  //
  // Check VM-execution control fields
  //

  if (~vm->vmexec_ctrls1 & VMX_MSR_VMX_PINBASED_CTRLS_LO) {
     BX_ERROR(("VMFAIL: VMCS EXEC CTRL: VMX pin-based controls allowed 0-settings"));
     return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
  }

  if (vm->vmexec_ctrls1 & ~VMX_MSR_VMX_PINBASED_CTRLS_HI) {
     BX_ERROR(("VMFAIL: VMCS EXEC CTRL: VMX pin-based controls allowed 1-settings"));
     return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
  }

  if (~vm->vmexec_ctrls2 & VMX_MSR_VMX_PROCBASED_CTRLS_LO) {
     BX_ERROR(("VMFAIL: VMCS EXEC CTRL: VMX proc-based controls allowed 0-settings"));
     return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
  }

  if (vm->vmexec_ctrls2 & ~VMX_MSR_VMX_PROCBASED_CTRLS_HI) {
     BX_ERROR(("VMFAIL: VMCS EXEC CTRL: VMX proc-based controls allowed 1-settings"));
     return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
  }

  if (vm->vm_cr3_target_cnt > VMX_CR3_TARGET_MAX_CNT) {
     BX_ERROR(("VMFAIL: VMCS EXEC CTRL: too may CR3 targets %d", vm->vm_cr3_target_cnt));
     return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
  }

  if (vm->vmexec_ctrls2 & VMX_VM_EXEC_CTRL2_IO_BITMAPS) {
     // I/O bitmaps control enabled
     for (int bitmap=0; bitmap < 2; bitmap++) {
       if (vm->io_bitmap_addr[bitmap] & 0xfff) {
         BX_ERROR(("VMFAIL: VMCS EXEC CTRL: I/O bitmap %c must be 4K aligned", 'A' + bitmap));
         return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
       }
       if (! IsValidPhyAddr(vm->io_bitmap_addr[bitmap])) {
         BX_ERROR(("VMFAIL: VMCS EXEC CTRL: I/O bitmap %c phy addr malformed", 'A' + bitmap));
         return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
       }
     }
  }

  if (vm->vmexec_ctrls2 & VMX_VM_EXEC_CTRL2_MSR_BITMAPS) {
     // MSR bitmaps control enabled
     if ((vm->msr_bitmap_addr & 0xfff) != 0 || ! IsValidPhyAddr(vm->msr_bitmap_addr)) {
       BX_ERROR(("VMFAIL: VMCS EXEC CTRL: MSR bitmap phy addr malformed"));
       return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
     }
  }

  if (! (vm->vmexec_ctrls1 & VMX_VM_EXEC_CTRL1_VIRTUAL_NMI)) {
     if (vm->vmexec_ctrls2 & VMX_VM_EXEC_CTRL2_NMI_WINDOW_VMEXIT) {
       BX_ERROR(("VMFAIL: VMCS EXEC CTRL: misconfigured virtual NMI control"));
       return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
     }
  }

  if (vm->vmexec_ctrls2 & VMX_VM_EXEC_CTRL2_TPR_SHADOW) {
     if ((vm->virtual_apic_page_addr & 0xfff) != 0 || ! IsValidPhyAddr(vm->virtual_apic_page_addr)) {
       BX_ERROR(("VMFAIL: VMCS EXEC CTRL: virtual apic phy addr malformed"));
       return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
     }

     if (vm->vm_tpr_threshold & 0xfffffff0) {
       BX_ERROR(("VMFAIL: VMCS EXEC CTRL: TPR threshold too big"));
       return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
     }

     if (vm->vm_tpr_threshold > VMX_Read_TPR_Shadow()) {
       BX_ERROR(("VMFAIL: VMCS EXEC CTRL: TPR threshold > TPR shadow"));
       return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
     }
  }

/*
  VM entries perform the following checks on the VM-execution control fields:

       If the "use TPR shadow" VM-execution control is 1, the virtual-APIC address must
        satisfy the following checks:
        The following items describe the treatment of bytes 81H-83H on the virtual-
        APIC page (see Section 20.6.8) if all of the above checks are satisfied and the
        "use TPR shadow" VM-execution control is 1, treatment depends upon the
        setting of the "virtualize APIC accesses" VM-execution control:2
        - If the "virtualize APIC accesses" VM-execution control is 0, the bytes may be
        cleared. (If the bytes are not cleared, they are left unmodified.)
        - If the "virtualize APIC accesses" VM-execution control is 1, the bytes are
        cleared.
        - Any clearing of the bytes occurs even if the VM entry subsequently fails.
         If the "use TPR shadow" VM-execution control is 1, bits 31:4 of the TPR threshold
        VM-execution control field must be 0.
       The following check is performed if the "use TPR shadow" VM-execution control is
        1 and the "virtualize APIC accesses" VM-execution control is 0: the value of
        bits 3:0 of the TPR threshold VM-execution control field should not be greater
        than the value of bits 7:4 in byte 80H on the virtual-APIC page (see Section
        20.6.8).
       If the "virtualize APIC-accesses" VM-execution control is 1, the APIC-access
        address must satisfy the following checks:
        - Bits 11:0 of the address must be 0.
        - On processors that support Intel 64 architecture, the address should not set
        any bits beyond the processor's physical-address width.
        - On processors that support the IA-32 architecture, the address should not set
        any bits in the range 63:32.
*/

  //
  // Load VM-exit control fields to VMCS Cache
  //

  vm->vmexit_ctrls = VMread32(VMCS_32BIT_CONTROL_VMEXIT_CONTROLS);
  vm->vmexit_msr_store_cnt = VMread32(VMCS_32BIT_CONTROL_VMEXIT_MSR_STORE_COUNT);
  vm->vmexit_msr_store_addr = VMread64(VMCS_64BIT_CONTROL_VMEXIT_MSR_STORE_ADDR);
  vm->vmexit_msr_load_cnt = VMread32(VMCS_32BIT_CONTROL_VMEXIT_MSR_LOAD_COUNT);
  vm->vmexit_msr_load_addr = VMread64(VMCS_64BIT_CONTROL_VMEXIT_MSR_LOAD_ADDR);

  //
  // Check VM-exit control fields
  //

  if (~vm->vmexit_ctrls & VMX_MSR_VMX_VMEXIT_CTRLS_LO) {
     BX_ERROR(("VMFAIL: VMCS EXEC CTRL: VMX vmexit controls allowed 0-settings"));
     return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
  }

  if (vm->vmexit_ctrls & ~VMX_MSR_VMX_VMEXIT_CTRLS_HI) {
     BX_ERROR(("VMFAIL: VMCS EXEC CTRL: VMX vmexit controls allowed 1-settings"));
     return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
  }

  if (vm->vmexit_msr_store_cnt > 0) {
     if ((vm->vmexit_msr_store_addr & 0xf) != 0 || ! IsValidPhyAddr(vm->vmexit_msr_store_addr)) {
       BX_ERROR(("VMFAIL: VMCS VMEXIT CTRL: msr store addr malformed"));
       return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
     }

     Bit64u last_byte = vm->vmexit_msr_store_addr + (vm->vmexit_msr_store_cnt * 16) - 1;
     if (! IsValidPhyAddr(last_byte)) {
       BX_ERROR(("VMFAIL: VMCS VMEXIT CTRL: msr store addr too high"));
       return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
     }
  }

  if (vm->vmexit_msr_load_cnt > 0) {
     if ((vm->vmexit_msr_load_addr & 0xf) != 0 || ! IsValidPhyAddr(vm->vmexit_msr_load_addr)) {
       BX_ERROR(("VMFAIL: VMCS VMEXIT CTRL: msr load addr malformed"));
       return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
     }

     Bit64u last_byte = (Bit64u) vm->vmexit_msr_load_addr + (vm->vmexit_msr_load_cnt * 16) - 1;
     if (! IsValidPhyAddr(last_byte)) {
       BX_ERROR(("VMFAIL: VMCS VMEXIT CTRL: msr load addr too high"));
       return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
     }
  }

  //
  // Load VM-entry control fields to VMCS Cache
  //

  vm->vmentry_ctrls = VMread32(VMCS_32BIT_CONTROL_VMENTRY_CONTROLS);
  vm->vmentry_msr_load_cnt = VMread32(VMCS_32BIT_CONTROL_VMENTRY_MSR_LOAD_COUNT);
  vm->vmentry_msr_load_addr = VMread64(VMCS_64BIT_CONTROL_VMENTRY_MSR_LOAD_ADDR);

  //
  // Check VM-entry control fields
  //

  if (~vm->vmentry_ctrls & VMX_MSR_VMX_VMENTRY_CTRLS_LO) {
     BX_ERROR(("VMFAIL: VMCS EXEC CTRL: VMX vmentry controls allowed 0-settings"));
     return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
  }

  if (vm->vmentry_ctrls & ~VMX_MSR_VMX_VMENTRY_CTRLS_HI) {
     BX_ERROR(("VMFAIL: VMCS EXEC CTRL: VMX vmentry controls allowed 1-settings"));
     return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
  }

  if (vm->vmentry_ctrls & VMX_VMENTRY_CTRL1_DEACTIVATE_DUAL_MONITOR_TREATMENT) {
     if (! BX_CPU_THIS_PTR in_smm) {
       BX_ERROR(("VMFAIL: VMENTRY from outside SMM with dual-monitor treatment enabled"));
       return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
     }
  }

  if (vm->vmentry_msr_load_cnt > 0) {
     if ((vm->vmentry_msr_load_addr & 0xf) != 0 || ! IsValidPhyAddr(vm->vmentry_msr_load_addr)) {
       BX_ERROR(("VMFAIL: VMCS VMENTRY CTRL: msr load addr malformed"));
       return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
     }

     Bit64u last_byte = vm->vmentry_msr_load_addr + (vm->vmentry_msr_load_cnt * 16) - 1;
     if (! IsValidPhyAddr(last_byte)) {
       BX_ERROR(("VMFAIL: VMCS VMENTRY CTRL: msr load addr too high"));
       return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
     }
  }

  //
  // Check VM-entry event injection info
  //

  vm->vmentry_interr_info = VMread32(VMCS_32BIT_CONTROL_VMENTRY_INTERRUPTION_INFO);
  vm->vmentry_excep_err_code = VMread32(VMCS_32BIT_CONTROL_VMENTRY_EXCEPTION_ERR_CODE);
  vm->vmentry_instr_length = VMread32(VMCS_32BIT_CONTROL_VMENTRY_INSTRUCTION_LENGTH);

  if (VMENTRY_INJECTING_EVENT(vm->vmentry_interr_info)) {

     /* the VMENTRY injecting event to the guest */
     unsigned vector = vm->vmentry_interr_info & 0xff;
     unsigned event_type = (vm->vmentry_interr_info >>  8) & 7;
     unsigned push_error = (vm->vmentry_interr_info >> 11) & 1;
     unsigned error_code = push_error ? vm->vmentry_excep_err_code : 0;

     unsigned push_error_reference = 0;
     if (event_type == BX_HARDWARE_EXCEPTION && vector < BX_CPU_HANDLED_EXCEPTIONS &&
             exceptions_info[vector].push_error) push_error_reference = 1;

     if (vm->vmentry_interr_info & 0x7ffff000) {
        BX_ERROR(("VMFAIL: VMENTRY broken interruption info field"));
        return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
     }

     switch (event_type) {
       case BX_EXTERNAL_INTERRUPT:
         break;

       case BX_NMI:
         if (vector != 2) {
           BX_ERROR(("VMFAIL: VMENTRY bad injected event vector %d", vector));
           return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
         }
         break;

       case BX_HARDWARE_EXCEPTION:
         if (vector > 31) {
           BX_ERROR(("VMFAIL: VMENTRY bad injected event vector %d", vector));
           return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
         }
         break;

       case BX_SOFTWARE_INTERRUPT:
       case BX_PRIVILEGED_SOFTWARE_INTERRUPT:
       case BX_SOFTWARE_EXCEPTION:
         if (vm->vmentry_instr_length == 0 || vm->vmentry_instr_length > 15) {
           BX_ERROR(("VMFAIL: VMENTRY bad injected event instr length"));
           return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
         }
         break;

       default:
         BX_ERROR(("VMFAIL: VMENTRY bad injected event type %d", event_type));
         return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
     }

     if (push_error != push_error_reference) {
        BX_ERROR(("VMFAIL: VMENTRY bad injected event vector %d", vector));
        return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
     }
     if (error_code & 0x7fff0000) {
        BX_ERROR(("VMFAIL: VMENTRY bad error code 0x%08x for injected event %d", error_code, vector));
        return VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD;
     }
  }

  return VMXERR_NO_ERROR;
}

VMX_error_code BX_CPU_C::VMenterLoadCheckHostState(void)
{
  VMCS_CACHE *vm = &BX_CPU_THIS_PTR vmcs;
  VMCS_HOST_STATE *host_state = &vm->host_state;
  bx_bool x86_64_host = 0, x86_64_guest = 0;

  //
  // VM Host State Checks Related to Address-Space Size
  //

  Bit32u vmexit_ctrls = vm->vmexit_ctrls;
  if (vmexit_ctrls & VMX_VMEXIT_CTRL1_HOST_ADDR_SPACE_SIZE) {
     x86_64_host = 1;
  }
  Bit32u vmentry_ctrls = vm->vmentry_ctrls;
  if (vmentry_ctrls & VMX_VMENTRY_CTRL1_X86_64_GUEST) {
     x86_64_guest = 1;
  }

  if (! long_mode()) {
     if (x86_64_host || x86_64_guest) {
        BX_ERROR(("VMFAIL: VMCS x86-64 guest(%d)/host(%d) controls invalid on VMENTRY", x86_64_guest, x86_64_host));
        return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
     }
  }
  else {
     if (! x86_64_host) {
        BX_ERROR(("VMFAIL: VMCS x86-64 host control invalid on VMENTRY"));
        return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
     }
  }

  //
  // Load and Check VM Host State to VMCS Cache
  //

  host_state->cr0 = (bx_address) VMread64(VMCS_HOST_CR0);
  if (~host_state->cr0 & VMX_MSR_CR0_FIXED0) {
     BX_ERROR(("VMFAIL: VMCS host state invalid CR0 0x%08x", (Bit32u) host_state->cr0));
     return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
  }

  if (host_state->cr0 & ~VMX_MSR_CR0_FIXED1) {
     BX_ERROR(("VMFAIL: VMCS host state invalid CR0 0x%08x", (Bit32u) host_state->cr0));
     return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
  }

  host_state->cr3 = (bx_address) VMread64(VMCS_HOST_CR3);
#if BX_SUPPORT_X86_64
  if (! IsValidPhyAddr(host_state->cr3)) {
     BX_ERROR(("VMFAIL: VMCS host state invalid CR3"));
     return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
  }
#endif

  host_state->cr4 = (bx_address) VMread64(VMCS_HOST_CR4);
  if (~host_state->cr4 & VMX_MSR_CR4_FIXED0) {
     BX_ERROR(("VMFAIL: VMCS host state invalid CR4 0x" FMT_ADDRX, host_state->cr4));
     return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
  }
  if (host_state->cr4 & ~VMX_MSR_CR4_FIXED1) {
     BX_ERROR(("VMFAIL: VMCS host state invalid CR4 0x" FMT_ADDRX, host_state->cr4));
     return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
  }

  for(int n=0; n<6; n++) {
     host_state->segreg_selector[n] = VMread16(VMCS_16BIT_HOST_ES_SELECTOR + 2*n);
     if (host_state->segreg_selector[n] & 7) {
        BX_ERROR(("VMFAIL: VMCS host segreg %d TI/RPL != 0", n));
        return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
     }
  }

  if (host_state->segreg_selector[BX_SEG_REG_CS] == 0) {
     BX_ERROR(("VMFAIL: VMCS host CS selector 0"));
     return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
  }

  if (! x86_64_host && host_state->segreg_selector[BX_SEG_REG_SS] == 0) {
     BX_ERROR(("VMFAIL: VMCS host SS selector 0"));
     return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
  }

  host_state->tr_selector = VMread16(VMCS_16BIT_HOST_TR_SELECTOR);
  if (! host_state->tr_selector || (host_state->tr_selector & 7) != 0) {
     BX_ERROR(("VMFAIL: VMCS invalid host TR selector"));
     return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
  }

  host_state->tr_base = (bx_address) VMread64(VMCS_HOST_TR_BASE);
#if BX_SUPPORT_X86_64
  if (! IsCanonical(host_state->tr_base)) {
     BX_ERROR(("VMFAIL: VMCS host TR BASE non canonical"));
     return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
  }
#endif

  host_state->fs_base = (bx_address) VMread64(VMCS_HOST_FS_BASE);
  host_state->gs_base = (bx_address) VMread64(VMCS_HOST_GS_BASE);
#if BX_SUPPORT_X86_64
  if (! IsCanonical(host_state->fs_base)) {
     BX_ERROR(("VMFAIL: VMCS host FS BASE non canonical"));
     return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
  }
  if (! IsCanonical(host_state->gs_base)) {
     BX_ERROR(("VMFAIL: VMCS host GS BASE non canonical"));
     return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
  }
#endif

  host_state->gdtr_base = (bx_address) VMread64(VMCS_HOST_GDTR_BASE);
  host_state->idtr_base = (bx_address) VMread64(VMCS_HOST_IDTR_BASE);
#if BX_SUPPORT_X86_64
  if (! IsCanonical(host_state->gdtr_base)) {
     BX_ERROR(("VMFAIL: VMCS host GDTR BASE non canonical"));
     return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
  }
  if (! IsCanonical(host_state->idtr_base)) {
     BX_ERROR(("VMFAIL: VMCS host IDTR BASE non canonical"));
     return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
  }
#endif

  host_state->sysenter_esp_msr = (bx_address) VMread64(VMCS_HOST_IA32_SYSENTER_ESP_MSR);
  host_state->sysenter_eip_msr = (bx_address) VMread64(VMCS_HOST_IA32_SYSENTER_EIP_MSR);
  host_state->sysenter_cs_msr = (Bit16u) VMread32(VMCS_32BIT_HOST_IA32_SYSENTER_CS_MSR);

#if BX_SUPPORT_X86_64
  if (! IsCanonical(host_state->sysenter_esp_msr)) {
     BX_ERROR(("VMFAIL: VMCS host SYSENTER_ESP_MSR non canonical"));
     return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
  }

  if (! IsCanonical(host_state->sysenter_eip_msr)) {
     BX_ERROR(("VMFAIL: VMCS host SYSENTER_EIP_MSR non canonical"));
     return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
  }
#endif

  host_state->rsp = (bx_address) VMread64(VMCS_HOST_RSP);
  host_state->rip = (bx_address) VMread64(VMCS_HOST_RIP);

#if BX_SUPPORT_X86_64
  if (x86_64_host) {
     if ((host_state->cr4 & (1<<5)) == 0) { // PAE
        BX_ERROR(("VMFAIL: VMCS host CR4.PAE=0 with x86-64 host"));
        return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
     }
     if (! IsCanonical(host_state->rip)) {
        BX_ERROR(("VMFAIL: VMCS host RIP non-canonical"));
        return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
     }
  }
  else {
     if (GET32H(host_state->rip) != 0) {
        BX_ERROR(("VMFAIL: VMCS host RIP > 32 bit"));
        return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
     }
  }
#endif

  return VMXERR_NO_ERROR;
}

BX_CPP_INLINE bx_bool IsLimitAccessRightsConsistent(Bit32u limit, Bit32u ar)
{
  bx_bool g = (ar >> 15) & 1;

  // access rights reserved bits set
  if (ar & 0xfffe0f00) return 0;

  if (g) {
    // if any of the bits in limit[11:00] are '0 <=> G must be '0
    if ((limit & 0xfff) != 0xfff)
       return 0;
  }
  else {
    // if any of the bits in limit[31:20] are '1 <=> G must be '1
    if ((limit & 0xfff00000) != 0)
       return 0;
  }

  return 1;
}

Bit32u BX_CPU_C::VMenterLoadCheckGuestState(Bit64u *qualification)
{
  static const char *segname[] = { "ES", "CS", "SS", "DS", "FS", "GS" };

  VMCS_GUEST_STATE guest;
  VMCS_CACHE *vm = &BX_CPU_THIS_PTR vmcs;

  *qualification = VMENTER_ERR_NO_ERROR;

  //
  // Load and Check Guest State from VMCS
  //

  guest.rflags = VMread64(VMCS_GUEST_RFLAGS);
  // RFLAGS reserved bits [63:22], bit 15, bit 5, bit 3 must be zero
  if (guest.rflags & BX_CONST64(0xFFFFFFFFFFC08028)) {
     BX_ERROR(("VMENTER FAIL: RFLAGS reserved bits are set"));
     return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
  }
  // RFLAGS[1] must be always set
  if ((guest.rflags & 0x2) == 0) {
     BX_ERROR(("VMENTER FAIL: RFLAGS[1] cleared"));
     return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
  }

  bx_bool v8086_guest = 0;
  if (guest.rflags & EFlagsVMMask)
     v8086_guest = 1;

  bx_bool x86_64_guest = 0; // can't be 1 if X86_64 is not supported (checked before)
  Bit32u vmentry_ctrls = vm->vmentry_ctrls;
#if BX_SUPPORT_X86_64
  if (vmentry_ctrls & VMX_VMENTRY_CTRL1_X86_64_GUEST) {
     BX_DEBUG(("VMENTER to x86-64 guest"));
     x86_64_guest = 1;
  }
#endif

  if (x86_64_guest && v8086_guest) {
     BX_ERROR(("VMENTER FAIL: Enter to x86-64 guest with RFLAGS.VM"));
     return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
  }

  guest.cr0 = VMread64(VMCS_GUEST_CR0);
  if (~guest.cr0 & VMX_MSR_CR0_FIXED0) {
     BX_ERROR(("VMENTER FAIL: VMCS guest invalid CR0"));
     return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
  }

  if (guest.cr0 & ~VMX_MSR_CR0_FIXED1) {
     BX_ERROR(("VMENTER FAIL: VMCS guest invalid CR0"));
     return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
  }

  guest.cr3 = VMread64(VMCS_GUEST_CR3);
#if BX_SUPPORT_X86_64
  if (! IsValidPhyAddr(guest.cr3)) {
     BX_ERROR(("VMENTER FAIL: VMCS guest invalid CR3"));
     return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
  }
#endif

  guest.cr4 = VMread64(VMCS_GUEST_CR4);
  if (~guest.cr4 & VMX_MSR_CR4_FIXED0) {
     BX_ERROR(("VMENTER FAIL: VMCS guest invalid CR4"));
     return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
  }

  if (guest.cr4 & ~VMX_MSR_CR4_FIXED1) {
     BX_ERROR(("VMENTER FAIL: VMCS guest invalid CR4"));
     return VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD;
  }

#if BX_SUPPORT_X86_64
  if (x86_64_guest && (guest.cr4 & (1<<5)) == 0) { // PAE
     BX_ERROR(("VMENTER FAIL: VMCS guest CR4.PAE=0 in x86-64 mode"));
     return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
  }
#endif

#if BX_SUPPORT_X86_64
  if (vmentry_ctrls & VMX_VMENTRY_CTRL1_LOAD_DBG_CTRLS) {
     guest.dr7 = VMread64(VMCS_GUEST_DR7);
     if (GET32H(guest.dr7)) {
        BX_ERROR(("VMENTER FAIL: VMCS guest invalid DR7"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
     }
  }
#endif

  //
  // Load and Check Guest State from VMCS - Segment Registers
  //

  for (int n=0; n<6; n++) {
     Bit16u selector = VMread16(VMCS_16BIT_GUEST_ES_SELECTOR + 2*n);
     bx_address base = (bx_address) VMread64(VMCS_GUEST_ES_BASE + 2*n);
     Bit32u limit = VMread32(VMCS_32BIT_GUEST_ES_LIMIT + 2*n);
     Bit32u ar = VMread32(VMCS_32BIT_GUEST_ES_ACCESS_RIGHTS + 2*n) >> 8;
     bx_bool invalid = (ar >> 16) & 1;

     if (set_segment_ar_data(&guest.sregs[n], !invalid, 
                  (Bit16u) selector, base, limit, (Bit16u) ar));

     if (v8086_guest) {
        // guest in V8086 mode
        if (base != ((bx_address)(selector << 4))) {
          BX_ERROR(("VMENTER FAIL: VMCS v8086 guest bad %s.BASE", segname[n]));
          return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
        if (limit != 0xffff) {
          BX_ERROR(("VMENTER FAIL: VMCS v8086 guest %s.LIMIT != 0xFFFF", segname[n]));
          return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
        // present, expand-up read/write accessed, segment, DPL=3
        if (ar != 0xF3) {
          BX_ERROR(("VMENTER FAIL: VMCS v8086 guest %s.AR != 0xF3", segname[n]));
          return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }

        continue; // go to next segment register
     }

#if BX_SUPPORT_X86_64
     if (n >= BX_SEG_REG_FS) {
        if (! IsCanonical(base)) {
          BX_ERROR(("VMENTER FAIL: VMCS guest %s.BASE non canonical", segname[n]));
          return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
     }
#endif

     if (n != BX_SEG_REG_CS && invalid)
        continue;

#if BX_SUPPORT_X86_64
     if (n == BX_SEG_REG_SS && (selector & BX_SELECTOR_RPL_MASK) == 0) {
        // SS is allowed to be NULL selector if going to 64-bit guest
        if (x86_64_guest && guest.sregs[BX_SEG_REG_CS].cache.u.segment.l)
           continue;
     }

     if (n < BX_SEG_REG_FS) {
        if (GET32H(base) != 0) {
          BX_ERROR(("VMENTER FAIL: VMCS guest %s.BASE > 32 bit", segname[n]));
          return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
     }
#endif

     if (! guest.sregs[n].cache.segment) {
        BX_ERROR(("VMENTER FAIL: VMCS guest %s not segment", segname[n]));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
     }

     if (! guest.sregs[n].cache.p) {
        BX_ERROR(("VMENTER FAIL: VMCS guest %s not present", segname[n]));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
     }

     if (! IsLimitAccessRightsConsistent(limit, ar)) {
        BX_ERROR(("VMENTER FAIL: VMCS guest %s.AR/LIMIT malformed", segname[n]));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
     }

     if (n == BX_SEG_REG_CS) {
        // CS checks
        switch (guest.sregs[BX_SEG_REG_CS].cache.type) {
          case BX_CODE_EXEC_ONLY_ACCESSED:
          case BX_CODE_EXEC_READ_ACCESSED:
             // non-conforming segment
             if (guest.sregs[n].selector.rpl != guest.sregs[n].cache.dpl) {
               BX_ERROR(("VMENTER FAIL: VMCS guest non-conforming CS.RPL <> CS.DPL"));
               return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
             }
             break;
          case BX_CODE_EXEC_ONLY_CONFORMING_ACCESSED:
          case BX_CODE_EXEC_READ_CONFORMING_ACCESSED:
             // conforming segment
             if (guest.sregs[n].selector.rpl < guest.sregs[n].cache.dpl) {
               BX_ERROR(("VMENTER FAIL: VMCS guest non-conforming CS.RPL < CS.DPL"));
               return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
             }
             break;
          default:
             BX_ERROR(("VMENTER FAIL: VMCS guest CS.TYPE"));
             return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }

#if BX_SUPPORT_X86_64
        if (x86_64_guest) {
          if (guest.sregs[BX_SEG_REG_CS].cache.u.segment.d_b && guest.sregs[BX_SEG_REG_CS].cache.u.segment.l) {
             BX_ERROR(("VMENTER FAIL: VMCS x86_64 guest wrong CS.D_B/L"));
             return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
          }
        }
#endif
     }
     else if (n == BX_SEG_REG_SS) {
        // SS checks
        switch (guest.sregs[BX_SEG_REG_SS].cache.type) {
          case BX_DATA_READ_WRITE_ACCESSED:
          case BX_DATA_READ_WRITE_EXPAND_DOWN_ACCESSED:
             break;
          default:
             BX_ERROR(("VMENTER FAIL: VMCS guest SS.TYPE"));
             return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }
     }
     else {
        // DS, ES, FS, GS
        if ((guest.sregs[n].cache.type & 0x1) == 0) {
           BX_ERROR(("VMENTER FAIL: VMCS guest %s not ACCESSED", segname[n]));
           return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
        }

        if (guest.sregs[n].cache.type & 0x8) {
           if ((guest.sregs[n].cache.type & 0x2) == 0) {
              BX_ERROR(("VMENTER FAIL: VMCS guest CODE segment %s not READABLE", segname[n]));
              return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
           }
        }

        if (guest.sregs[n].cache.type < 11) {
           // data segment or non-conforming code segment
           if (guest.sregs[n].selector.rpl > guest.sregs[n].cache.dpl) {
             BX_ERROR(("VMENTER FAIL: VMCS guest non-conforming %s.RPL < %s.DPL", segname[n], segname[n]));
             return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
           }
        }
     }
  }

  if (! v8086_guest) {
     if (guest.sregs[BX_SEG_REG_SS].selector.rpl != guest.sregs[BX_SEG_REG_CS].selector.rpl) {
        BX_ERROR(("VMENTER FAIL: VMCS guest CS.RPL != SS.RPL"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
     }

     if (guest.sregs[BX_SEG_REG_SS].selector.rpl != guest.sregs[BX_SEG_REG_SS].cache.dpl) {
        BX_ERROR(("VMENTER FAIL: VMCS guest SS.RPL <> SS.DPL"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
     }
  }

  //
  // Load and Check Guest State from VMCS - GDTR/IDTR
  //

  Bit64u gdtr_base = VMread64(VMCS_GUEST_GDTR_BASE);
  Bit32u gdtr_limit = VMread32(VMCS_32BIT_GUEST_GDTR_LIMIT);
  Bit64u idtr_base = VMread64(VMCS_GUEST_IDTR_BASE);
  Bit32u idtr_limit = VMread32(VMCS_32BIT_GUEST_IDTR_LIMIT);

#if BX_SUPPORT_X86_64
  if (! IsCanonical(gdtr_base) || ! IsCanonical(idtr_base)) {
    BX_ERROR(("VMENTER FAIL: VMCS guest IDTR/IDTR.BASE non canonical"));
    return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
  }
#endif
  if (gdtr_limit > 0xffff || idtr_limit > 0xffff) {
     BX_ERROR(("VMENTER FAIL: VMCS guest GDTR/IDTR limit > 0xFFFF"));
     return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
  }

  //
  // Load and Check Guest State from VMCS - LDTR
  //

  Bit16u ldtr_selector = VMread16(VMCS_16BIT_GUEST_LDTR_SELECTOR);
  Bit64u ldtr_base = VMread64(VMCS_GUEST_LDTR_BASE);
  Bit32u ldtr_limit = VMread32(VMCS_32BIT_GUEST_LDTR_LIMIT);
  Bit32u ldtr_ar = VMread32(VMCS_32BIT_GUEST_LDTR_ACCESS_RIGHTS) >> 8;
  bx_bool ldtr_invalid = (ldtr_ar >> 16) & 1;
  if (set_segment_ar_data(&guest.ldtr, !ldtr_invalid, 
         (Bit16u) ldtr_selector, ldtr_base, ldtr_limit, (Bit16u)(ldtr_ar)))
  {
     // ldtr is valid
     if (guest.ldtr.selector.ti) {
        BX_ERROR(("VMENTER FAIL: VMCS guest LDTR.TI set"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
     }
     if (guest.ldtr.cache.type != BX_SYS_SEGMENT_LDT) {
        BX_ERROR(("VMENTER FAIL: VMCS guest incorrect LDTR type"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
     }
     if (guest.ldtr.cache.segment) {
        BX_ERROR(("VMENTER FAIL: VMCS guest LDTR is not system segment"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
     }
     if (! guest.ldtr.cache.p) {
        BX_ERROR(("VMENTER FAIL: VMCS guest LDTR not present"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
     }
     if (! IsLimitAccessRightsConsistent(ldtr_limit, ldtr_ar)) {
        BX_ERROR(("VMENTER FAIL: VMCS guest LDTR.AR/LIMIT malformed"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
     }
#if BX_SUPPORT_X86_64
     if (! IsCanonical(ldtr_base)) {
        BX_ERROR(("VMENTER FAIL: VMCS guest LDTR.BASE non canonical"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
     }
#endif
  }

  //
  // Load and Check Guest State from VMCS - TR
  //

  Bit16u tr_selector = VMread16(VMCS_16BIT_GUEST_TR_SELECTOR);
  Bit64u tr_base = VMread64(VMCS_GUEST_TR_BASE);
  Bit32u tr_limit = VMread32(VMCS_32BIT_GUEST_TR_LIMIT);
  Bit32u tr_ar = VMread32(VMCS_32BIT_GUEST_TR_ACCESS_RIGHTS) >> 8;
  bx_bool tr_invalid = (tr_ar >> 16) & 1;

#if BX_SUPPORT_X86_64
  if (! IsCanonical(tr_base)) {
    BX_ERROR(("VMENTER FAIL: VMCS guest TR.BASE non canonical"));
    return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
  }
#endif

  set_segment_ar_data(&guest.tr, !tr_invalid, 
      (Bit16u) tr_selector, tr_base, tr_limit, (Bit16u)(tr_ar));

  if (tr_invalid) {
     BX_ERROR(("VMENTER FAIL: VMCS guest TR invalid"));
     return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
  }
  if (guest.tr.selector.ti) {
     BX_ERROR(("VMENTER FAIL: VMCS guest TR.TI set"));
     return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
  }
  if (guest.tr.cache.segment) {
     BX_ERROR(("VMENTER FAIL: VMCS guest TR is not system segment"));
     return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
  }
  if (! guest.tr.cache.p) {
     BX_ERROR(("VMENTER FAIL: VMCS guest TR not present"));
     return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
  }
  if (! IsLimitAccessRightsConsistent(tr_limit, tr_ar)) {
    BX_ERROR(("VMENTER FAIL: VMCS guest TR.AR/LIMIT malformed"));
    return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
  }

  switch(guest.tr.cache.type) {
    case BX_SYS_SEGMENT_BUSY_386_TSS:
      break;
    case BX_SYS_SEGMENT_BUSY_286_TSS:
      if (! x86_64_guest) break;
      // fall through
    default:
      BX_ERROR(("VMENTER FAIL: VMCS guest incorrect TR type"));
      return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
  }

  //
  // Load and Check Guest State from VMCS - MSRS
  //

  guest.ia32_debugctl_msr = VMread64(VMCS_64BIT_GUEST_IA32_DEBUGCTL);
  guest.smbase = VMread32(VMCS_32BIT_GUEST_SMBASE);

  guest.sysenter_esp_msr = VMread64(VMCS_GUEST_IA32_SYSENTER_ESP_MSR);
  guest.sysenter_eip_msr = VMread64(VMCS_GUEST_IA32_SYSENTER_EIP_MSR);
  guest.sysenter_cs_msr = VMread32(VMCS_32BIT_GUEST_IA32_SYSENTER_CS_MSR);

#if BX_SUPPORT_X86_64
  if (! IsCanonical(guest.sysenter_esp_msr)) {
    BX_ERROR(("VMENTER FAIL: VMCS guest SYSENTER_ESP_MSR non canonical"));
    return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
  }
  if (! IsCanonical(guest.sysenter_eip_msr)) {
    BX_ERROR(("VMENTER FAIL: VMCS guest SYSENTER_EIP_MSR non canonical"));
    return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
  }
#endif

  guest.rip = VMread64(VMCS_GUEST_RIP);
  guest.rsp = VMread64(VMCS_GUEST_RSP);

#if BX_SUPPORT_X86_64
  if (! x86_64_guest || !guest.sregs[BX_SEG_REG_CS].cache.u.segment.l) {
    if (GET32H(guest.rip) != 0) {
       BX_ERROR(("VMENTER FAIL: VMCS guest RIP > 32 bit"));
       return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }
  }
#endif

  //
  // Load and Check Guest Non-Registers State from VMCS
  //

  guest.link_pointer = VMread64(VMCS_64BIT_GUEST_LINK_POINTER);
  if (guest.link_pointer != BX_INVALID_VMCSPTR) {
    if ((guest.link_pointer & 0xfff) != 0 || ! IsValidPhyAddr(guest.link_pointer)) {
      *qualification = (Bit64u) VMENTER_ERR_GUEST_STATE_LINK_POINTER;
      BX_ERROR(("VMFAIL: VMCS link pointer malformed"));
      return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }

    Bit32u revision = VMXReadRevisionID((bx_phy_address) guest.link_pointer);
    if (revision != VMX_VMCS_REVISION_ID) {
      *qualification = (Bit64u) VMENTER_ERR_GUEST_STATE_LINK_POINTER;
      BX_ERROR(("VMFAIL: VMCS link pointer incorrect revision ID %d != %d", revision, VMX_VMCS_REVISION_ID));
      return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }

    if (! BX_CPU_THIS_PTR in_smm || (vmentry_ctrls & VMX_VMENTRY_CTRL1_SMM_ENTER) != 0) {
      if (guest.link_pointer == BX_CPU_THIS_PTR vmcsptr) {
        *qualification = (Bit64u) VMENTER_ERR_GUEST_STATE_LINK_POINTER;
        BX_ERROR(("VMFAIL: VMCS link pointer equal to current VMCS pointer"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
      }
    }
    else {
      if (guest.link_pointer == BX_CPU_THIS_PTR vmxonptr) {
        *qualification = (Bit64u) VMENTER_ERR_GUEST_STATE_LINK_POINTER;
        BX_ERROR(("VMFAIL: VMCS link pointer equal to VMXON pointer"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
      }
    }
  }

  guest.tmpDR6 = VMread64(VMCS_GUEST_PENDING_DBG_EXCEPTIONS);
  if (guest.tmpDR6 & BX_CONST64(0xFFFFFFFFFFFFAFF0)) {
    BX_ERROR(("VMENTER FAIL: VMCS guest tmpDR6 reserved bits"));
    return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
  }

  guest.activity_state = VMread32(VMCS_32BIT_GUEST_ACTIVITY_STATE);
  if (guest.activity_state >= BX_VMX_LAST_ACTIVITY_STATE) {
    BX_ERROR(("VMENTER FAIL: VMCS guest activity state %d", guest.activity_state));
    return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
  }

  if (guest.activity_state == BX_VMX_STATE_HLT) {
    if (guest.sregs[BX_SEG_REG_SS].cache.dpl != 0) {
      BX_ERROR(("VMENTER FAIL: VMCS guest HLT state with SS.DPL=%d", guest.sregs[BX_SEG_REG_SS].cache.dpl));
      return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }
  }

  guest.interruptibility_state = VMread32(VMCS_32BIT_GUEST_INTERRUPTIBILITY_STATE);
  if (guest.interruptibility_state & ~BX_VMX_INTERRUPTIBILITY_STATE_MASK) {
    BX_ERROR(("VMENTER FAIL: VMCS guest interruptibility state broken"));
    return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
  }

  if ((guest.interruptibility_state & BX_VMX_INTERRUPTS_BLOCKED_BY_STI) &&
      (guest.interruptibility_state & BX_VMX_INTERRUPTS_BLOCKED_BY_MOV_SS))
  {
    BX_ERROR(("VMENTER FAIL: VMCS guest interruptibility state broken"));
    return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
  }

  if ((guest.rflags & EFlagsIFMask) == 0) {
    if (guest.interruptibility_state & BX_VMX_INTERRUPTS_BLOCKED_BY_STI) {
      BX_ERROR(("VMENTER FAIL: VMCS guest interrupts can't be blocked by STI when EFLAGS.IF = 0"));
      return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }
  }

  if (VMENTRY_INJECTING_EVENT(vm->vmentry_interr_info)) {
    unsigned event_type = (vm->vmentry_interr_info >> 8) & 7;
    if (event_type == BX_EXTERNAL_INTERRUPT) {
      if ((guest.interruptibility_state & 3) != 0 || (guest.rflags & EFlagsIFMask) == 0) {
        BX_ERROR(("VMENTER FAIL: VMCS guest interrupts blocked when injecting external interrupt"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
      }
    }
    if (event_type == BX_NMI) {
      if ((guest.interruptibility_state & 3) != 0) {
        BX_ERROR(("VMENTER FAIL: VMCS guest interrupts blocked when injecting NMI"));
        return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
      }
    }
  }

  if (vmentry_ctrls & VMX_VMENTRY_CTRL1_SMM_ENTER) {
    if (! (guest.interruptibility_state & BX_VMX_INTERRUPTS_BLOCKED_SMI_BLOCKED)) {
      BX_ERROR(("VMENTER FAIL: VMCS SMM guest should block SMI"));
      return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }
  }

  if (guest.interruptibility_state & BX_VMX_INTERRUPTS_BLOCKED_SMI_BLOCKED) {
    if (! BX_CPU_THIS_PTR in_smm) {
      BX_ERROR(("VMENTER FAIL: VMCS SMI blocked when not in SMM mode"));
      return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }
  }

#if BX_CPU_LEVEL >= 6
  if (! x86_64_guest && (guest.cr4 & (1 << 5)) != 0 /* PAE */) {
    // CR0.PG is always set in VMX mode
    if (! CheckPDPTR(guest.cr3)) {
      *qualification = VMENTER_ERR_GUEST_STATE_PDPTR_LOADING;
      BX_ERROR(("VMENTER: Guest State PDPTRs Checks Failed"));
      return VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE;
    }
  }
#endif

  //
  // Load Guest State -> VMENTER
  //

#if BX_SUPPORT_X86_64
  // set EFER.LMA and EFER.LME before write to CR4
  if (x86_64_guest)
     BX_CPU_THIS_PTR efer.set32(BX_CPU_THIS_PTR efer.get32() |  ((1 << 8) | (1 << 10)));
  else
     BX_CPU_THIS_PTR efer.set32(BX_CPU_THIS_PTR efer.get32() & ~((1 << 8) | (1 << 10)));
#endif

  Bit32u old_cr0 = BX_CPU_THIS_PTR cr0.get32();

#define VMX_KEEP_CR0_BITS 0x7FFAFFD0

  // load CR0, keep bits ET(4), reserved bits 15:6, 17, 28:19, NW(29), CD(30)
  if (!SetCR0((old_cr0 & VMX_KEEP_CR0_BITS) | (guest.cr0 & ~VMX_KEEP_CR0_BITS))) {
    BX_PANIC(("VMENTER CR0 is broken !"));
  }
  if (!SetCR4(guest.cr4)) { // cannot fail if CR4 checks were correct
    BX_PANIC(("VMENTER CR4 is broken !"));
  }
  SetCR3(guest.cr3);

  if (vmentry_ctrls & VMX_VMENTRY_CTRL1_LOAD_DBG_CTRLS) {
    // always clear bits 15:14 and set bit 10
    BX_CPU_THIS_PTR dr7 = (guest.dr7 & ~0xc000) | 0400;
  }

  RIP = BX_CPU_THIS_PTR prev_rip = guest.rip;
  RSP = guest.rsp;

  // set flags directly, avoid setEFlags side effects
  BX_CPU_THIS_PTR eflags = (Bit32u) guest.rflags;
  BX_CPU_THIS_PTR lf_flags_status = 0; // OSZAPC flags are known.
  
  for(unsigned segreg=0; segreg<6; segreg++)
    BX_CPU_THIS_PTR sregs[segreg] = guest.sregs[segreg];

  if (v8086_guest) CPL = 3;

  BX_CPU_THIS_PTR gdtr.base = gdtr_base;
  BX_CPU_THIS_PTR gdtr.limit = gdtr_limit;
  BX_CPU_THIS_PTR idtr.base = idtr_base;
  BX_CPU_THIS_PTR idtr.limit = idtr_limit;

  BX_CPU_THIS_PTR ldtr = guest.ldtr;
  BX_CPU_THIS_PTR tr = guest.tr;

  BX_CPU_THIS_PTR msr.sysenter_esp_msr = guest.sysenter_esp_msr;
  BX_CPU_THIS_PTR msr.sysenter_eip_msr = guest.sysenter_eip_msr;
  BX_CPU_THIS_PTR msr.sysenter_cs_msr  = guest.sysenter_cs_msr;

  //
  // Load Guest Non-Registers State -> VMENTER
  //

  BX_CPU_THIS_PTR async_event = 0;
  if (guest.rflags & (EFlagsTFMask|EFlagsRFMask))
    BX_CPU_THIS_PTR async_event = 1;

  if (vm->vmentry_ctrls & VMX_VMENTRY_CTRL1_SMM_ENTER)
    BX_PANIC(("VMENTER: entry to SMM is not implemented yet !"));

  if (VMENTRY_INJECTING_EVENT(vm->vmentry_interr_info)) {
    // the VMENTRY injecting event to the guest
    BX_CPU_THIS_PTR inhibit_mask = 0; // do not block interrupts
    BX_CPU_THIS_PTR debug_trap = 0;
  }
  else {
    if (guest.tmpDR6 & (1 << 12))
      BX_CPU_THIS_PTR debug_trap = guest.tmpDR6 & 0x0000400F;
    else
      BX_CPU_THIS_PTR debug_trap = guest.tmpDR6 & 0x00004000;
    if (BX_CPU_THIS_PTR debug_trap)
      BX_CPU_THIS_PTR async_event = 1;

    if (guest.interruptibility_state & BX_VMX_INTERRUPTS_BLOCKED_BY_STI)
      BX_CPU_THIS_PTR inhibit_mask = BX_INHIBIT_INTERRUPTS;
    else if (guest.interruptibility_state & BX_VMX_INTERRUPTS_BLOCKED_BY_MOV_SS)
      BX_CPU_THIS_PTR inhibit_mask = BX_INHIBIT_INTERRUPTS | BX_INHIBIT_DEBUG;
    else BX_CPU_THIS_PTR inhibit_mask = 0;
  }

  if (BX_CPU_THIS_PTR inhibit_mask)
    BX_CPU_THIS_PTR async_event = 1;

  if (guest.interruptibility_state & BX_VMX_INTERRUPTS_BLOCKED_NMI_BLOCKED)
    BX_CPU_THIS_PTR disable_NMI = 1;

  if (vm->vmexec_ctrls2 & VMX_VM_EXEC_CTRL2_INTERRUPT_WINDOW_VMEXIT) {
    BX_CPU_THIS_PTR async_event = 1;
    BX_CPU_THIS_PTR vmx_interrupt_window = 1; // set up interrupt window exiting
  }

#if BX_SUPPORT_MONITOR_MWAIT
  BX_CPU_THIS_PTR monitor.reset_monitor();
#endif

  invalidate_prefetch_q();
#if BX_SUPPORT_ALIGNMENT_CHECK
  handleAlignmentCheck();
#endif
  handleCpuModeChange();

  return VMXERR_NO_ERROR;
}

void BX_CPU_C::VMenterInjectEvents(void)
{
  VMCS_CACHE *vm = &BX_CPU_THIS_PTR vmcs;

  if (! VMENTRY_INJECTING_EVENT(vm->vmentry_interr_info))
     return;

  /* the VMENTRY injecting event to the guest */
  unsigned vector = vm->vmentry_interr_info & 0xff;
  unsigned type = (vm->vmentry_interr_info >> 8) & 7;
  unsigned push_error = vm->vmentry_interr_info & (1 << 11);
  unsigned error_code = push_error ? vm->vmentry_excep_err_code : 0;

  bx_bool is_INT = 0;
  switch(type) {
    case BX_EXTERNAL_INTERRUPT:
    case BX_NMI:
    case BX_HARDWARE_EXCEPTION:
      BX_CPU_THIS_PTR EXT = 1;
      is_INT = 0;
      break;

    case BX_SOFTWARE_INTERRUPT:
    case BX_PRIVILEGED_SOFTWARE_INTERRUPT:
    case BX_SOFTWARE_EXCEPTION:
      is_INT = 1;
      break;

    default:
      BX_PANIC(("VMENTER: unsupported event injection type %d !", type));
  }

  // keep prev_rip value/unwind in case of event delivery failure
  if (is_INT)
    RIP += vm->vmentry_instr_length;

  BX_ERROR(("VMENTER: Injecting vector 0x%02x (error_code 0x%04x)", vector, error_code));

  if (type == BX_HARDWARE_EXCEPTION) {
    // record exception the same way as BX_CPU_C::exception does
    if (vector < BX_CPU_HANDLED_EXCEPTIONS)
      BX_CPU_THIS_PTR curr_exception = exceptions_info[vector].exception_type;
    else // else take default value
      BX_CPU_THIS_PTR curr_exception = exceptions_info[BX_CPU_HANDLED_EXCEPTIONS].exception_type;

    BX_CPU_THIS_PTR errorno = 1;
  }

  vm->idt_vector_info = vm->vmentry_interr_info & ~0x80000000;
  vm->idt_vector_error_code = error_code;

  interrupt(vector, type, push_error, error_code);
}

Bit32u BX_CPU_C::LoadMSRs(Bit32u msr_cnt, bx_phy_address pAddr)
{
  Bit64u msr_lo, msr_hi;

  for (Bit32u msr = 1; msr <= msr_cnt; msr++) {
    access_read_physical(pAddr,     8, &msr_lo);
    BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID, pAddr, 8, BX_READ, (Bit8u*)(&msr_lo));
    access_read_physical(pAddr + 8, 8, &msr_hi);
    BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID, pAddr + 8, 8, BX_READ, (Bit8u*)(&msr_hi));

    if (GET32H(msr_lo))
      return msr;

    Bit32u index = GET32L(msr_lo);

#if BX_SUPPORT_X86_64
    if (index == BX_MSR_FSBASE || index == BX_MSR_GSBASE)
      return msr;
#endif

    if (! wrmsr(index, msr_hi))
      return msr;

    pAddr += 16; // to next MSR
  }

  return 0;
}

Bit32u BX_CPU_C::StoreMSRs(Bit32u msr_cnt, bx_phy_address pAddr)
{
  Bit64u msr_lo, msr_hi;

  for (Bit32u msr = 1; msr <= msr_cnt; msr++) {
    access_read_physical(pAddr, 8, &msr_lo);
    BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID, pAddr, 8, BX_READ, (Bit8u*)(&msr_lo));

    if (GET32H(msr_lo))
      return msr;

    Bit32u index = GET32L(msr_lo);

    if (! rdmsr(index, &msr_hi))
      return msr;

    access_write_physical(pAddr + 8, 8, &msr_hi);
    BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID, pAddr + 8, 8, BX_WRITE, (Bit8u*)(&msr_hi));

    pAddr += 16; // to next MSR
  }

  return 0;
}

////////////////////////////////////////////////////////////
// VMexit
////////////////////////////////////////////////////////////

void BX_CPU_C::VMexitSaveGuestState(void)
{
  VMCS_CACHE *vm = &BX_CPU_THIS_PTR vmcs;

  VMwrite64(VMCS_GUEST_CR0, BX_CPU_THIS_PTR cr0.get32());
  VMwrite64(VMCS_GUEST_CR3, BX_CPU_THIS_PTR cr3);
  VMwrite64(VMCS_GUEST_CR4, BX_CPU_THIS_PTR cr4.get32());
  if (vm->vmexit_ctrls & VMX_VMEXIT_CTRL1_SAVE_DBG_CTRLS)
     VMwrite64(VMCS_GUEST_DR7, BX_CPU_THIS_PTR dr7);
  VMwrite64(VMCS_GUEST_RIP, RIP);
  VMwrite64(VMCS_GUEST_RSP, RSP);
  VMwrite64(VMCS_GUEST_RFLAGS, BX_CPU_THIS_PTR read_eflags());

  for (int n=0; n<6; n++) {
     Bit32u selector = BX_CPU_THIS_PTR sregs[n].selector.value;
     bx_bool invalid = !BX_CPU_THIS_PTR sregs[n].cache.valid;
     bx_address base = BX_CPU_THIS_PTR sregs[n].cache.u.segment.base;
     Bit32u limit = BX_CPU_THIS_PTR sregs[n].cache.u.segment.limit_scaled;
     Bit32u ar = get_descriptor_h(&BX_CPU_THIS_PTR sregs[n].cache) & 0x00f0ff00;

     VMwrite16(VMCS_16BIT_GUEST_ES_SELECTOR + 2*n, selector);
     VMwrite64(VMCS_GUEST_ES_BASE + 2*n, base);
     VMwrite32(VMCS_32BIT_GUEST_ES_LIMIT + 2*n, limit);
     VMwrite32(VMCS_32BIT_GUEST_ES_ACCESS_RIGHTS + 2*n, ar | (invalid << 24));
  }

  // save guest LDTR
  Bit32u ldtr_selector = BX_CPU_THIS_PTR ldtr.selector.value;
  bx_bool ldtr_invalid = !BX_CPU_THIS_PTR ldtr.cache.valid;
  bx_address ldtr_base = BX_CPU_THIS_PTR ldtr.cache.u.segment.base;
  Bit32u ldtr_limit = BX_CPU_THIS_PTR ldtr.cache.u.segment.limit_scaled;
  Bit32u ldtr_ar = get_descriptor_h(&BX_CPU_THIS_PTR ldtr.cache) & 0x00f0ff00;

  VMwrite16(VMCS_16BIT_GUEST_LDTR_SELECTOR, ldtr_selector);
  VMwrite64(VMCS_GUEST_LDTR_BASE, ldtr_base);
  VMwrite32(VMCS_32BIT_GUEST_LDTR_LIMIT, ldtr_limit);
  VMwrite32(VMCS_32BIT_GUEST_LDTR_ACCESS_RIGHTS, ldtr_ar | (ldtr_invalid << 24));

  // save guest TR
  Bit32u tr_selector = BX_CPU_THIS_PTR tr.selector.value;
  bx_bool tr_invalid = !BX_CPU_THIS_PTR tr.cache.valid;
  bx_address tr_base = BX_CPU_THIS_PTR tr.cache.u.segment.base;
  Bit32u tr_limit = BX_CPU_THIS_PTR tr.cache.u.segment.limit_scaled;
  Bit32u tr_ar = get_descriptor_h(&BX_CPU_THIS_PTR tr.cache) & 0x00f0ff00;

  VMwrite16(VMCS_16BIT_GUEST_TR_SELECTOR, tr_selector);
  VMwrite64(VMCS_GUEST_TR_BASE, tr_base);
  VMwrite32(VMCS_32BIT_GUEST_TR_LIMIT, tr_limit);
  VMwrite32(VMCS_32BIT_GUEST_TR_ACCESS_RIGHTS, tr_ar | (tr_invalid << 24));

  VMwrite64(VMCS_GUEST_GDTR_BASE, BX_CPU_THIS_PTR gdtr.base);
  VMwrite32(VMCS_32BIT_GUEST_GDTR_LIMIT, BX_CPU_THIS_PTR gdtr.limit);
  VMwrite64(VMCS_GUEST_IDTR_BASE, BX_CPU_THIS_PTR idtr.base);
  VMwrite32(VMCS_32BIT_GUEST_IDTR_LIMIT, BX_CPU_THIS_PTR idtr.limit);

  VMwrite64(VMCS_GUEST_IA32_SYSENTER_ESP_MSR, BX_CPU_THIS_PTR msr.sysenter_esp_msr);
  VMwrite64(VMCS_GUEST_IA32_SYSENTER_EIP_MSR, BX_CPU_THIS_PTR msr.sysenter_eip_msr);
  VMwrite32(VMCS_32BIT_GUEST_IA32_SYSENTER_CS_MSR, BX_CPU_THIS_PTR msr.sysenter_cs_msr);

  Bit32u tmpDR6 = BX_CPU_THIS_PTR debug_trap;
  if (tmpDR6 & 0xf) tmpDR6 |= (1 << 12);
  VMwrite64(VMCS_GUEST_PENDING_DBG_EXCEPTIONS, tmpDR6 & 0x0000500f);
  
  Bit32u interruptibility_state = 0;
  if (BX_CPU_THIS_PTR inhibit_mask & BX_INHIBIT_INTERRUPTS_SHADOW) {
     if (BX_CPU_THIS_PTR inhibit_mask & BX_INHIBIT_DEBUG_SHADOW)
        interruptibility_state |= BX_VMX_INTERRUPTS_BLOCKED_BY_MOV_SS;
     else
        interruptibility_state |= BX_VMX_INTERRUPTS_BLOCKED_BY_STI;
  }
  if (BX_CPU_THIS_PTR disable_SMI)
    interruptibility_state |= BX_VMX_INTERRUPTS_BLOCKED_SMI_BLOCKED;
  if (BX_CPU_THIS_PTR disable_NMI)
    interruptibility_state |= BX_VMX_INTERRUPTS_BLOCKED_NMI_BLOCKED;
  VMwrite32(VMCS_32BIT_GUEST_INTERRUPTIBILITY_STATE, interruptibility_state);
}

void BX_CPU_C::VMexitLoadHostState(void)
{
  VMCS_HOST_STATE *host_state = &BX_CPU_THIS_PTR vmcs.host_state;
  bx_bool x86_64_host = 0;
#if BX_SUPPORT_X86_64
  Bit32u vmexit_ctrls = BX_CPU_THIS_PTR vmcs.vmexit_ctrls;
  if (vmexit_ctrls & VMX_VMEXIT_CTRL1_HOST_ADDR_SPACE_SIZE) {
     BX_DEBUG(("VMEXIT to x86-64 host"));
     x86_64_host = 1;
  }
#endif  

#if BX_SUPPORT_X86_64
  // set EFER.LMA and EFER.LME before write to CR4
  if (x86_64_host)
     BX_CPU_THIS_PTR efer.set32(BX_CPU_THIS_PTR efer.get32() |  ((1 << 8) | (1 << 10)));
  else
     BX_CPU_THIS_PTR efer.set32(BX_CPU_THIS_PTR efer.get32() & ~((1 << 8) | (1 << 10)));
#endif

  Bit32u old_cr0 = BX_CPU_THIS_PTR cr0.get32();

  // ET, CD, NW, 28:19, 17, 15:6, and VMX fixed bits not modified Section 19.8
  if (!SetCR0((old_cr0 & VMX_KEEP_CR0_BITS) | (host_state->cr0 & ~VMX_KEEP_CR0_BITS))) {
    BX_PANIC(("VMEXIT CR0 is broken !"));
  }
  if (!SetCR4(host_state->cr4)) {
    BX_PANIC(("VMEXIT CR4 is broken !"));
  }
  SetCR3(host_state->cr3);

  BX_CPU_THIS_PTR dr7 = 0x00000400;

  BX_CPU_THIS_PTR msr.sysenter_cs_msr = host_state->sysenter_cs_msr;
  BX_CPU_THIS_PTR msr.sysenter_esp_msr = host_state->sysenter_esp_msr;
  BX_CPU_THIS_PTR msr.sysenter_eip_msr = host_state->sysenter_eip_msr;

  // CS selector loaded from VMCS
  //    valid   <= 1
  //    base    <= 0
  //    limit   <= 0xffffffff, g <= 1
  //    present <= 1
  //    dpl     <= 0
  //    type    <= segment, BX_CODE_EXEC_READ_ACCESSED
  //    d_b     <= loaded from 'host-address space size' VMEXIT control
  //    l       <= loaded from 'host-address space size' VMEXIT control

  parse_selector(host_state->segreg_selector[BX_SEG_REG_CS],
               &BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector);

  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.valid    = SegValidCache | SegAccessROK | SegAccessWOK;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.p        = 1;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.dpl      = 0;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.segment  = 1;  /* data/code segment */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.type     = BX_CODE_EXEC_READ_ACCESSED;

  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.base         = 0;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled = 0xffffffff;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.avl = 0;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.g   = 1; /* page granular */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b = !x86_64_host;
#if BX_SUPPORT_X86_64
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.l   =  x86_64_host;
#endif

  // DATA selector loaded from VMCS
  //    valid   <= if selector is not all-zero
  //    base    <= 0
  //    limit   <= 0xffffffff, g <= 1
  //    present <= 1
  //    dpl     <= 0
  //    type    <= segment, BX_DATA_READ_WRITE_ACCESSED
  //    d_b     <= 1
  //    l       <= 0

  for (unsigned segreg = 0; segreg < 6; segreg++)
  {
    if (segreg == BX_SEG_REG_CS) continue;

    parse_selector(host_state->segreg_selector[segreg],
               &BX_CPU_THIS_PTR sregs[segreg].selector);

    if (! host_state->segreg_selector[segreg]) {
       BX_CPU_THIS_PTR sregs[segreg].cache.valid    = 0;
    }
    else {
       BX_CPU_THIS_PTR sregs[segreg].cache.valid    = SegValidCache;
       BX_CPU_THIS_PTR sregs[segreg].cache.p        = 1;
       BX_CPU_THIS_PTR sregs[segreg].cache.dpl      = 0;
       BX_CPU_THIS_PTR sregs[segreg].cache.segment  = 1;  /* data/code segment */
       BX_CPU_THIS_PTR sregs[segreg].cache.type     = BX_DATA_READ_WRITE_ACCESSED;
       BX_CPU_THIS_PTR sregs[segreg].cache.u.segment.base         = 0;
       BX_CPU_THIS_PTR sregs[segreg].cache.u.segment.limit_scaled = 0xffffffff;
       BX_CPU_THIS_PTR sregs[segreg].cache.u.segment.avl = 0;
       BX_CPU_THIS_PTR sregs[segreg].cache.u.segment.g   = 1; /* page granular */
       BX_CPU_THIS_PTR sregs[segreg].cache.u.segment.d_b = 1;
#if BX_SUPPORT_X86_64
       BX_CPU_THIS_PTR sregs[segreg].cache.u.segment.l   = 0;
#endif
    }
  }

  // SS.DPL always clear 
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.dpl = 0;

  if (x86_64_host || BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.valid)
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.base = host_state->fs_base;

  if (x86_64_host || BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.valid)
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.base = host_state->gs_base;

  // TR selector loaded from VMCS
  parse_selector(host_state->tr_selector, &BX_CPU_THIS_PTR tr.selector);

  BX_CPU_THIS_PTR tr.cache.valid    = 1; /* valid */
  BX_CPU_THIS_PTR tr.cache.p        = 1; /* present */
  BX_CPU_THIS_PTR tr.cache.dpl      = 0; /* field not used */
  BX_CPU_THIS_PTR tr.cache.segment  = 0; /* system segment */
  BX_CPU_THIS_PTR tr.cache.type     = BX_SYS_SEGMENT_BUSY_386_TSS;
  BX_CPU_THIS_PTR tr.cache.u.segment.base         = host_state->tr_base;
  BX_CPU_THIS_PTR tr.cache.u.segment.limit_scaled = 0x67;
  BX_CPU_THIS_PTR tr.cache.u.segment.avl = 0;
  BX_CPU_THIS_PTR tr.cache.u.segment.g   = 0; /* byte granular */

  // unusable LDTR
  BX_CPU_THIS_PTR ldtr.selector.value = 0x0000;
  BX_CPU_THIS_PTR ldtr.selector.index = 0x0000;
  BX_CPU_THIS_PTR ldtr.selector.ti    = 0;
  BX_CPU_THIS_PTR ldtr.selector.rpl   = 0;
  BX_CPU_THIS_PTR ldtr.cache.valid    = 0; /* invalid */

  BX_CPU_THIS_PTR gdtr.base = host_state->gdtr_base;
  BX_CPU_THIS_PTR gdtr.limit = 0xFFFF;

  BX_CPU_THIS_PTR idtr.base = host_state->idtr_base;
  BX_CPU_THIS_PTR idtr.limit = 0xFFFF;

  RIP = host_state->rip;
  RSP = host_state->rsp;

  BX_CPU_THIS_PTR inhibit_mask = 0;
  BX_CPU_THIS_PTR debug_trap = 0;

  // set flags directly, avoid setEFlags side effects
  BX_CPU_THIS_PTR eflags = 0x2;        // Bit1 is always set.
  BX_CPU_THIS_PTR lf_flags_status = 0; // OSZAPC flags are known.

#if BX_SUPPORT_MONITOR_MWAIT
  BX_CPU_THIS_PTR monitor.reset_monitor();
#endif

  invalidate_prefetch_q();
#if BX_SUPPORT_ALIGNMENT_CHECK
  handleAlignmentCheck();
#endif
  handleCpuModeChange();
}

void BX_CPU_C::VMexit(bxInstruction_c *i, Bit32u reason, Bit64u qualification)
{
  VMCS_CACHE *vm = &BX_CPU_THIS_PTR vmcs;

  // VMEXITs are FAULT-like: restore RIP/RSP to value before VMEXIT occurred
  RIP = BX_CPU_THIS_PTR prev_rip;
  if (BX_CPU_THIS_PTR speculative_rsp)
    RSP = BX_CPU_THIS_PTR prev_rsp;

  //
  // STEP 0: Update VMEXIT reason
  //

  VMwrite32(VMCS_32BIT_VMEXIT_REASON, reason);
  VMwrite64(VMCS_VMEXIT_QUALIFICATION, qualification);

  if (i != 0)
     VMwrite32(VMCS_32BIT_VMEXIT_INSTRUCTION_LENGTH, i->ilen());

  reason &= 0xffff; /* keep only basic VMEXIT reason */

  if (reason != VMX_VMEXIT_EXCEPTION_NMI) {
    VMwrite32(VMCS_32BIT_VMEXIT_INTERRUPTION_INFO, 0);
  }

  if (BX_CPU_THIS_PTR in_event) {
    VMwrite32(VMCS_32BIT_IDT_VECTORING_INFO, vm->idt_vector_info | 0x80000000);
    VMwrite32(VMCS_32BIT_IDT_VECTORING_ERR_CODE, vm->idt_vector_error_code);
    BX_CPU_THIS_PTR in_event = 0;
  }
  else {
    VMwrite32(VMCS_32BIT_IDT_VECTORING_INFO, 0);
  }

  //
  // STEP 1: Saving Guest State to VMCS
  //
  if (reason != VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE && reason != VMX_VMEXIT_VMENTRY_FAILURE_MSR) {
    // clear VMENTRY interruption info field
    VMwrite32(VMCS_32BIT_CONTROL_VMENTRY_INTERRUPTION_INFO, vm->vmentry_interr_info & ~0x80000000);

    VMexitSaveGuestState();

    Bit32u msr = StoreMSRs(vm->vmexit_msr_store_cnt, vm->vmexit_msr_store_addr);
    if (msr) { 
      BX_ERROR(("VMABORT: Error when saving guest MSR number %d", msr));
      VMabort(VMABORT_SAVING_GUEST_MSRS_FAILURE);
    }
  }

  //
  // STEP 2: Load Host State
  //
  VMexitLoadHostState();

  //
  // STEP 3: Load Host MSR registers
  //

  Bit32u msr = LoadMSRs(vm->vmexit_msr_load_cnt, vm->vmexit_msr_load_addr);
  if (msr) {
    BX_ERROR(("VMABORT: Error when loading host MSR number %d", msr));
    VMabort(VMABORT_LOADING_HOST_MSRS);
  }

  //
  // STEP 4: Go back to VMX host
  //

  BX_CPU_THIS_PTR disable_INIT = 1; // INIT is disabled in VMX root mode
  BX_CPU_THIS_PTR in_vmx_guest = 0;
  BX_CPU_THIS_PTR vmx_interrupt_window = 0;

  longjmp(BX_CPU_THIS_PTR jmp_buf_env, 1); // go back to main decode loop
}

#endif // BX_SUPPORT_VMX

////////////////////////////////////////////////////////////
// VMX instructions
////////////////////////////////////////////////////////////

void BX_CPU_C::VMXON(bxInstruction_c *i)
{
#if BX_SUPPORT_VMX
  if (! BX_CPU_THIS_PTR cr4.get_VMXE() || ! protected_mode() || BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_COMPAT)
    exception(BX_UD_EXCEPTION, 0, 0);

  if (! BX_CPU_THIS_PTR in_vmx) {
    if (CPL != 0 || ! BX_CPU_THIS_PTR cr0.get_NE() || 
            ! BX_CPU_THIS_PTR cr0.get_PE() || BX_GET_ENABLE_A20() == 0 /* ||
                   (bit 0 (lock bit) of IA32_FEATURE_CONTROL MSR is clear) ||
                   (bit 2 of IA32_FEATURE_CONTROL MSR is clear)*/)
    exception(BX_GP_EXCEPTION, 0, 0);

    bx_address eaddr = BX_CPU_CALL_METHODR(i->ResolveModrm, (i));
    Bit64u pAddr = read_virtual_qword(i->seg(), eaddr); // keep 64-bit
    if ((pAddr & 0xfff) != 0 || ! IsValidPhyAddr(pAddr)) {
       BX_ERROR(("VMXON: invalid or not page aligned physical address !"));
       VMfailInvalid();
       return;
    }

    Bit32u revision = VMXReadRevisionID((bx_phy_address) pAddr);
    if (revision != VMX_VMCS_REVISION_ID) {
       BX_ERROR(("VMXON: not expected (%d != %d) VMCS revision id !", revision, VMX_VMCS_REVISION_ID));
       VMfailInvalid();
       return;
    }
      
    BX_CPU_THIS_PTR vmcsptr = BX_INVALID_VMCSPTR;
    BX_CPU_THIS_PTR vmcshostptr = 0;
    BX_CPU_THIS_PTR vmxonptr = pAddr;
    BX_CPU_THIS_PTR in_vmx = 1;
    BX_CPU_THIS_PTR disable_INIT = 1; // INIT is disabled in VMX root mode
    // block and disable A20M;

#if BX_SUPPORT_MONITOR_MWAIT
    BX_CPU_THIS_PTR monitor.reset_monitor();
#endif

    VMsucceed();
    return;
  }

  if (BX_CPU_THIS_PTR in_vmx_guest) { // in VMX non-root operation
    BX_ERROR(("VMEXIT: VMXON in VMX non-root operation"));
    VMexit_Instruction(i, VMX_VMEXIT_VMXON);
  }
  else {
    // in VMX root operation mode
    if (CPL != 0)
      exception(BX_GP_EXCEPTION, 0, 0);

    VMfail(VMXERR_VMXON_IN_VMX_ROOT_OPERATION);
  }
#else
  BX_INFO(("VMXON: required VMX support, use --enable-vmx option"));
  exception(BX_UD_EXCEPTION, 0, 0);
#endif  
}

void BX_CPU_C::VMXOFF(bxInstruction_c *i)
{
#if BX_SUPPORT_VMX
  if (! BX_CPU_THIS_PTR in_vmx || BX_CPU_THIS_PTR get_VM() || BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_COMPAT)
    exception(BX_UD_EXCEPTION, 0, 0);

  if (BX_CPU_THIS_PTR in_vmx_guest) {
    BX_ERROR(("VMEXIT: VMXOFF in VMX non-root operation"));
    VMexit_Instruction(i, VMX_VMEXIT_VMXOFF);
  }

  if (CPL != 0)
    exception(BX_GP_EXCEPTION, 0, 0);

/*
        if dual-monitor treatment of SMIs and SMM is active
                THEN VMfail(VMXERR_VMXOFF_WITH_CONFIGURED_SMM_MONITOR);
        else
*/
  {
    BX_CPU_THIS_PTR vmxonptr = BX_INVALID_VMCSPTR;
    BX_CPU_THIS_PTR in_vmx = 0;  // leave VMX operation mode
    BX_CPU_THIS_PTR disable_INIT = 0;
     // unblock and enable A20M;
#if BX_SUPPORT_MONITOR_MWAIT
    BX_CPU_THIS_PTR monitor.reset_monitor();
#endif
    VMsucceed();
  }
#else
  BX_INFO(("VMXOFF: required VMX support, use --enable-vmx option"));
  exception(BX_UD_EXCEPTION, 0, 0);
#endif  
}

void BX_CPU_C::VMCALL(bxInstruction_c *i)
{
#if BX_SUPPORT_VMX
  if (! BX_CPU_THIS_PTR in_vmx)
    exception(BX_UD_EXCEPTION, 0, 0);

  if (BX_CPU_THIS_PTR in_vmx_guest) {
    BX_ERROR(("VMEXIT: VMCALL in VMX non-root operation"));
    VMexit_Instruction(i, VMX_VMEXIT_VMCALL);
  }

  if (BX_CPU_THIS_PTR get_VM() || BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_COMPAT)
    exception(BX_UD_EXCEPTION, 0, 0);

  if (CPL != 0)
    exception(BX_GP_EXCEPTION, 0, 0);

  if (BX_CPU_THIS_PTR in_smm /*|| 
        (the logical processor does not support the dual-monitor treatment of SMIs and SMM) ||
        (the valid bit in the IA32_SMM_MONITOR_CTL MSR is clear)*/)
  {
    VMfail(VMXERR_VMCALL_IN_VMX_ROOT_OPERATION);
    return;
  }
/*
        if dual-monitor treatment of SMIs and BX_CPU_THIS_PTR in_smm
                THEN perform an SMM VMexit (see Section 24.16.2
                     of the IntelR 64 and IA-32 Architectures Software Developer's Manual, Volume 3B);
*/
  if (! VMCSPTR_VALID()) {
    BX_ERROR(("VMFAIL: VMCALL with invalid VMCS ptr"));
    VMfailInvalid();
    return;
  }

  Bit32u launch_state;
  access_read_physical(BX_CPU_THIS_PTR vmcsptr + VMCS_LAUNCH_STATE_FIELD_ADDR, 4, &launch_state);
  BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID, BX_CPU_THIS_PTR vmcsptr + VMCS_LAUNCH_STATE_FIELD_ADDR, 4,
        BX_READ, (Bit8u*)(&launch_state));

  if (launch_state != VMCS_STATE_CLEAR) {
    BX_ERROR(("VMFAIL: VMCALL with launched VMCS"));
    VMfail(VMXERR_VMCALL_NON_CLEAR_VMCS);
    return;
  }

  BX_PANIC(("VMCALL: not implemented yet"));
/*
        if VM-exit control fields are not valid (see Section 24.16.6.1 of the IntelR 64 and IA-32 Architectures
                                                 Software Developer's Manual, Volume 3B)
                THEN VMfail(VMXERR_VMCALL_INVALID_VMEXIT_FIELD);
        else
                enter SMM;
                read revision identifier in MSEG;
                if revision identifier does not match that supported by processor
                THEN
                        leave SMM;
                        VMfailValid(VMXERR_VMCALL_INVALID_MSEG_REVISION_ID);
                else
                        read SMM-monitor features field in MSEG (see Section 24.16.6.2,
                        in the IntelR 64 and IA-32 Architectures Software Developer's Manual, Volume 3B);
                        if features field is invalid
                        THEN
                                leave SMM;
                                VMfailValid(VMXERR_VMCALL_WITH_INVALID_SMM_MONITOR_FEATURES);
                        else activate dual-monitor treatment of SMIs and SMM (see Section 24.16.6
                             in the IntelR 64 and IA-32 Architectures Software Developer's Manual, Volume 3B);
                        FI;
                FI;
        FI;
*/
#else
  BX_INFO(("VMCALL: required VMX support, use --enable-vmx option"));
  exception(BX_UD_EXCEPTION, 0, 0);
#endif  
}

void BX_CPU_C::VMLAUNCH(bxInstruction_c *i)
{
#if BX_SUPPORT_VMX
  if (! BX_CPU_THIS_PTR in_vmx || BX_CPU_THIS_PTR get_VM() || BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_COMPAT)
    exception(BX_UD_EXCEPTION, 0, 0);

  unsigned vmlaunch = 0;
  if ((i->rm() & 0x7) == 0x2) {
    BX_INFO(("VMLAUNCH VMCS ptr: 0x" FMT_ADDRX64, BX_CPU_THIS_PTR vmcsptr));
    vmlaunch = 1;
  }

  if (BX_CPU_THIS_PTR in_vmx_guest) {
    BX_ERROR(("VMEXIT: VMLAUNCH in VMX non-root operation"));
    VMexit_Instruction(i, vmlaunch ? VMX_VMEXIT_VMLAUNCH : VMX_VMEXIT_VMRESUME);
  }

  if (CPL != 0)
    exception(BX_GP_EXCEPTION, 0, 0);

  if (! VMCSPTR_VALID()) {
    BX_ERROR(("VMFAIL: VMLAUNCH with invalid VMCS ptr !"));
    VMfailInvalid();
    return;
  }

  if ((BX_CPU_THIS_PTR inhibit_mask & BX_INHIBIT_INTERRUPTS_BY_MOVSS_SHADOW) == BX_INHIBIT_INTERRUPTS_BY_MOVSS_SHADOW) {
    BX_ERROR(("VMFAIL: VMLAUNCH with interrupts blocked by MOV_SS !"));
    VMfail(VMXERR_VMENTRY_MOV_SS_BLOCKING);
    return;
  }

  Bit32u launch_state;
  access_read_physical(BX_CPU_THIS_PTR vmcsptr + VMCS_LAUNCH_STATE_FIELD_ADDR, 4, &launch_state);
  BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID, BX_CPU_THIS_PTR vmcsptr + VMCS_LAUNCH_STATE_FIELD_ADDR, 4,
        BX_READ, (Bit8u*)(&launch_state));

  if (vmlaunch) {
    if (launch_state != VMCS_STATE_CLEAR) {
       BX_ERROR(("VMFAIL: VMLAUNCH with non-clear VMCS!"));
       VMfail(VMXERR_VMLAUNCH_NON_CLEAR_VMCS);
       return;
    }
  }
  else {
    if (launch_state != VMCS_STATE_LAUNCHED) {
       BX_ERROR(("VMFAIL: VMRESUME with non-launched VMCS!"));
       VMfail(VMXERR_VMRESUME_NON_LAUNCHED_VMCS);
       return;
    }
  }

  ///////////////////////////////////////////////////////
  // STEP 1: Load and Check VM-Execution Control Fields
  // STEP 2: Load and Check VM-Exit Control Fields
  // STEP 3: Load and Check VM-Entry Control Fields
  ///////////////////////////////////////////////////////

  VMX_error_code error = VMenterLoadCheckVmControls();
  if (error != VMXERR_NO_ERROR) {
    VMfail(error);
    return;
  }

  ///////////////////////////////////////////////////////
  // STEP 4: Load and Check Host State
  ///////////////////////////////////////////////////////

  error = VMenterLoadCheckHostState();
  if (error != VMXERR_NO_ERROR) {
    VMfail(error);
    return;
  }

  ///////////////////////////////////////////////////////
  // STEP 5: Load and Check Guest State
  ///////////////////////////////////////////////////////

  Bit64u qualification = VMENTER_ERR_NO_ERROR;
  Bit32u state_load_error = VMenterLoadCheckGuestState(&qualification);
  if (state_load_error) {
    BX_ERROR(("VMEXIT: Guest State Checks Failed"));
    VMexit(0, state_load_error | (1 << 31), qualification);
  }

  Bit32u msr = LoadMSRs(BX_CPU_THIS_PTR vmcs.vmentry_msr_load_cnt, BX_CPU_THIS_PTR vmcs.vmentry_msr_load_addr);
  if (msr) {
    BX_ERROR(("VMEXIT: Error when loading guest MSR 0x%08x", msr));
    VMexit(0, VMX_VMEXIT_VMENTRY_FAILURE_MSR | (1 << 31), msr);
  }

  ///////////////////////////////////////////////////////
  // STEP 6: Update VMCS 'launched' state
  ///////////////////////////////////////////////////////
 
  if (vmlaunch) {
    launch_state = VMCS_STATE_LAUNCHED;
    bx_phy_address pAddr = BX_CPU_THIS_PTR vmcsptr + VMCS_LAUNCH_STATE_FIELD_ADDR;
    access_write_physical(pAddr, 4, &launch_state);
    BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID, pAddr, 4, BX_WRITE, (Bit8u*)(&launch_state));
  }

/*
                Check settings of VMX controls and host-state area;
                if invalid settings
                THEN VMfailValid(VM entry with invalid VMX-control field(s)) or
                     VMfailValid(VM entry with invalid host-state field(s)) or
                     VMfailValid(VM entry with invalid executive-VMCS pointer)) or
                     VMfailValid(VM entry with non-launched executive VMCS) or
                     VMfailValid(VM entry with executive-VMCS pointer not VMXON pointer)
                     VMfailValid(VM entry with invalid VM-execution control fields in executive VMCS)
                (as appropriate);
                else
                        Attempt to load guest state and PDPTRs as appropriate;
                        clear address-range monitoring;
                        if failure in checking guest state or PDPTRs
                                THEN VM entry fails (see Section 22.7, in the
                                                     IntelR 64 and IA-32 Architectures Software Developer's Manual, Volume 3B);
                                else
                                        Attempt to load MSRs from VM-entry MSR-load area;
                                        if failure
                                                THEN VM entry fails (see Section 22.7, in the IntelR 64 and IA-32
                                                                     Architectures Software Developer's Manual, Volume 3B);
                                        else {
                                                if VMLAUNCH
                                                        THEN launch state of VMCS <== "launched";
                                                if in SMM and "entry to SMM" VM-entry control is 0
                                                THEN
                                                                if "deactivate dual-monitor treatment" VM-entry control is 0
                                                                        THEN SMM-transfer VMCS pointer <== current-VMCS pointer;
                                                                FI;
                                                                if executive-VMCS pointer is VMX pointer
                                                                        THEN current-VMCS pointer <== VMCS-link pointer;
                                                                        else current-VMCS pointer <== executive-VMCS pointer;
                                                                FI;
                                                                leave SMM;
                                                FI;
                                                VMsucceed();
                                        }
                                FI;
                FI;
*/

  BX_CPU_THIS_PTR in_vmx_guest = 1;
  BX_CPU_THIS_PTR disable_INIT = 0;

  ///////////////////////////////////////////////////////
  // STEP 7: Inject events to the guest
  ///////////////////////////////////////////////////////

  VMenterInjectEvents();

#else
  BX_INFO(("VMLAUNCH/VMRESUME: required VMX support, use --enable-vmx option"));
  exception(BX_UD_EXCEPTION, 0, 0);
#endif  
}

void BX_CPU_C::VMPTRLD(bxInstruction_c *i)
{
#if BX_SUPPORT_VMX
  if (! BX_CPU_THIS_PTR in_vmx || BX_CPU_THIS_PTR get_VM() || BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_COMPAT)
    exception(BX_UD_EXCEPTION, 0, 0);

  if (BX_CPU_THIS_PTR in_vmx_guest) {
    BX_ERROR(("VMEXIT: VMPTRLD in VMX non-root operation"));
    VMexit_Instruction(i, VMX_VMEXIT_VMPTRLD);
  }

  if (CPL != 0)
    exception(BX_GP_EXCEPTION, 0, 0);

  bx_address eaddr = BX_CPU_CALL_METHODR(i->ResolveModrm, (i));
  Bit64u pAddr = read_virtual_qword(i->seg(), eaddr); // keep 64-bit
  if ((pAddr & 0xfff) != 0 || ! IsValidPhyAddr(pAddr)) {
    BX_ERROR(("VMFAIL: invalid or not page aligned physical address !"));
    VMfail(VMXERR_VMPTRLD_INVALID_PHYSICAL_ADDRESS);
    return;
  }

  if (pAddr == BX_CPU_THIS_PTR vmxonptr) {
    BX_ERROR(("VMFAIL: VMPTRLD with VMXON ptr !"));
    VMfail(VMXERR_VMPTRLD_WITH_VMXON_PTR);
  }
  else {
    Bit32u revision = VMXReadRevisionID((bx_phy_address) pAddr);
    if (revision != VMX_VMCS_REVISION_ID) {
       BX_ERROR(("VMPTRLD: not expected (%d != %d) VMCS revision id !", revision, VMX_VMCS_REVISION_ID));
       VMfail(VMXERR_VMPTRLD_INCORRECT_VMCS_REVISION_ID);
    }
    else {
       set_VMCSPTR(pAddr);
       VMsucceed();
    }
  }
#else
  BX_INFO(("VMPTRLD: required VMX support, use --enable-vmx option"));
  exception(BX_UD_EXCEPTION, 0, 0);
#endif  
}

void BX_CPU_C::VMPTRST(bxInstruction_c *i)
{
#if BX_SUPPORT_VMX
  if (! BX_CPU_THIS_PTR in_vmx || BX_CPU_THIS_PTR get_VM() || BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_COMPAT)
    exception(BX_UD_EXCEPTION, 0, 0);

  if (BX_CPU_THIS_PTR in_vmx_guest) {
    BX_ERROR(("VMEXIT: VMPTRST in VMX non-root operation"));
    VMexit_Instruction(i, VMX_VMEXIT_VMPTRST);
  }

  if (CPL != 0)
    exception(BX_GP_EXCEPTION, 0, 0);

  bx_address eaddr = BX_CPU_CALL_METHODR(i->ResolveModrm, (i));
  write_virtual_qword(i->seg(), eaddr, BX_CPU_THIS_PTR vmcsptr);
  VMsucceed();
#else         
  BX_INFO(("VMPTRST: required VMX support, use --enable-vmx option"));
  exception(BX_UD_EXCEPTION, 0, 0);
#endif  
}

void BX_CPU_C::VMREAD(bxInstruction_c *i)
{
#if BX_SUPPORT_VMX
  if (! BX_CPU_THIS_PTR in_vmx || BX_CPU_THIS_PTR get_VM() || BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_COMPAT)
    exception(BX_UD_EXCEPTION, 0, 0);

  if (BX_CPU_THIS_PTR in_vmx_guest) {
    BX_ERROR(("VMEXIT: VMREAD in VMX non-root operation"));
    VMexit_Instruction(i, VMX_VMEXIT_VMREAD);
  }

  if (CPL != 0)
    exception(BX_GP_EXCEPTION, 0, 0);

  if (! VMCSPTR_VALID()) {
    BX_ERROR(("VMFAIL: VMREAD with invalid VMCS ptr !"));
    VMfailInvalid();
    return;
  }

#if BX_SUPPORT_X86_64
  if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) {
    Bit64u enc = BX_READ_64BIT_REG(i->nnn());
    if (enc >> 32) {
      VMfail(VMXERR_UNSUPPORTED_VMCS_COMPONENT_ACCESS);
      return;
    }
  }
#endif
  unsigned encoding = BX_READ_32BIT_REG(i->nnn());

  Bit64u field_64;

  switch(encoding) {
    /* VMCS 16-bit guest-state fields */
    /* binary 0000_10xx_xxxx_xxx0 */
    case VMCS_16BIT_GUEST_ES_SELECTOR:
    case VMCS_16BIT_GUEST_CS_SELECTOR:
    case VMCS_16BIT_GUEST_SS_SELECTOR:
    case VMCS_16BIT_GUEST_DS_SELECTOR:
    case VMCS_16BIT_GUEST_FS_SELECTOR:
    case VMCS_16BIT_GUEST_GS_SELECTOR:
    case VMCS_16BIT_GUEST_LDTR_SELECTOR:
    case VMCS_16BIT_GUEST_TR_SELECTOR:
      // fall through

    /* VMCS 16-bit host-state fields */
    /* binary 0000_11xx_xxxx_xxx0 */
    case VMCS_16BIT_HOST_ES_SELECTOR:
    case VMCS_16BIT_HOST_CS_SELECTOR:
    case VMCS_16BIT_HOST_SS_SELECTOR:
    case VMCS_16BIT_HOST_DS_SELECTOR:
    case VMCS_16BIT_HOST_FS_SELECTOR:
    case VMCS_16BIT_HOST_GS_SELECTOR:
    case VMCS_16BIT_HOST_TR_SELECTOR:
      field_64 = VMread16(encoding);
      break;

    /* VMCS 32_bit control fields */
    /* binary 0100_00xx_xxxx_xxx0 */
    case VMCS_32BIT_CONTROL_PIN_BASED_EXEC_CONTROLS:
    case VMCS_32BIT_CONTROL_PROCESSOR_BASED_EXEC_CONTROLS:
    case VMCS_32BIT_CONTROL_EXECUTION_BITMAP:
    case VMCS_32BIT_CONTROL_PAGE_FAULT_ERR_CODE_MASK:
    case VMCS_32BIT_CONTROL_PAGE_FAULT_ERR_CODE_MATCH:
    case VMCS_32BIT_CONTROL_CR3_TARGET_COUNT:
    case VMCS_32BIT_CONTROL_VMEXIT_CONTROLS:
    case VMCS_32BIT_CONTROL_VMEXIT_MSR_STORE_COUNT:
    case VMCS_32BIT_CONTROL_VMEXIT_MSR_LOAD_COUNT:
    case VMCS_32BIT_CONTROL_VMENTRY_CONTROLS:
    case VMCS_32BIT_CONTROL_VMENTRY_MSR_LOAD_COUNT:
    case VMCS_32BIT_CONTROL_VMENTRY_INTERRUPTION_INFO:
    case VMCS_32BIT_CONTROL_VMENTRY_EXCEPTION_ERR_CODE:
    case VMCS_32BIT_CONTROL_VMENTRY_INSTRUCTION_LENGTH:
    case VMCS_32BIT_CONTROL_TPR_THRESHOLD:
      // fall through

    /* VMCS 32-bit read only data fields */
    /* binary 0100_01xx_xxxx_xxx0 */
    case VMCS_32BIT_INSTRUCTION_ERROR:
    case VMCS_32BIT_VMEXIT_REASON:
    case VMCS_32BIT_VMEXIT_INTERRUPTION_INFO:
    case VMCS_32BIT_VMEXIT_INTERRUPTION_ERR_CODE:
    case VMCS_32BIT_IDT_VECTORING_INFO:
    case VMCS_32BIT_IDT_VECTORING_ERR_CODE:
    case VMCS_32BIT_VMEXIT_INSTRUCTION_LENGTH:
    case VMCS_32BIT_VMEXIT_INSTRUCTION_INFO:
      // fall through

    /* VMCS 32-bit guest-state fields */
    /* binary 0100_10xx_xxxx_xxx0 */
    case VMCS_32BIT_GUEST_ES_LIMIT:
    case VMCS_32BIT_GUEST_CS_LIMIT:
    case VMCS_32BIT_GUEST_SS_LIMIT:
    case VMCS_32BIT_GUEST_DS_LIMIT:
    case VMCS_32BIT_GUEST_FS_LIMIT:
    case VMCS_32BIT_GUEST_GS_LIMIT:
    case VMCS_32BIT_GUEST_LDTR_LIMIT:
    case VMCS_32BIT_GUEST_TR_LIMIT:
    case VMCS_32BIT_GUEST_GDTR_LIMIT:
    case VMCS_32BIT_GUEST_IDTR_LIMIT:
    case VMCS_32BIT_GUEST_INTERRUPTIBILITY_STATE:
    case VMCS_32BIT_GUEST_ACTIVITY_STATE:
    case VMCS_32BIT_GUEST_SMBASE:
    case VMCS_32BIT_GUEST_IA32_SYSENTER_CS_MSR:
      field_64 = VMread32(encoding);
      break;

    case VMCS_32BIT_GUEST_ES_ACCESS_RIGHTS:
    case VMCS_32BIT_GUEST_CS_ACCESS_RIGHTS:
    case VMCS_32BIT_GUEST_SS_ACCESS_RIGHTS:
    case VMCS_32BIT_GUEST_DS_ACCESS_RIGHTS:
    case VMCS_32BIT_GUEST_FS_ACCESS_RIGHTS:
    case VMCS_32BIT_GUEST_GS_ACCESS_RIGHTS:
    case VMCS_32BIT_GUEST_LDTR_ACCESS_RIGHTS:
    case VMCS_32BIT_GUEST_TR_ACCESS_RIGHTS:
      field_64 = VMread32(encoding) >> 8;
      break;

    /* VMCS 32-bit host-state fields */
    /* binary 0100_11xx_xxxx_xxx0 */
    case VMCS_32BIT_HOST_IA32_SYSENTER_CS_MSR:
      field_64 = VMread32(encoding);
      break;

    /* VMCS 64-bit control fields */
    /* binary 0010_00xx_xxxx_xxx0 */
    case VMCS_64BIT_CONTROL_IO_BITMAP_A:
    case VMCS_64BIT_CONTROL_IO_BITMAP_B:
    case VMCS_64BIT_CONTROL_MSR_BITMAPS:
    case VMCS_64BIT_CONTROL_VMEXIT_MSR_STORE_ADDR:
    case VMCS_64BIT_CONTROL_VMEXIT_MSR_LOAD_ADDR:
    case VMCS_64BIT_CONTROL_VMENTRY_MSR_LOAD_ADDR:
    case VMCS_64BIT_CONTROL_EXECUTIVE_VMCS_PTR:
    case VMCS_64BIT_CONTROL_TSC_OFFSET:
    case VMCS_64BIT_CONTROL_VIRTUAL_APIC_PAGE_ADDR:
      field_64 = VMread64(encoding);
      break;

    case VMCS_64BIT_CONTROL_IO_BITMAP_A_HI:
    case VMCS_64BIT_CONTROL_IO_BITMAP_B_HI:
    case VMCS_64BIT_CONTROL_MSR_BITMAPS_HI:
    case VMCS_64BIT_CONTROL_VMEXIT_MSR_STORE_ADDR_HI:
    case VMCS_64BIT_CONTROL_VMEXIT_MSR_LOAD_ADDR_HI:
    case VMCS_64BIT_CONTROL_VMENTRY_MSR_LOAD_ADDR_HI:
    case VMCS_64BIT_CONTROL_EXECUTIVE_VMCS_PTR_HI:
    case VMCS_64BIT_CONTROL_TSC_OFFSET_HI:
    case VMCS_64BIT_CONTROL_VIRTUAL_APIC_PAGE_ADDR_HI:
      field_64 = VMread32(encoding);
      break;

    /* VMCS 64-bit guest state fields */
    /* binary 0010_10xx_xxxx_xxx0 */
    case VMCS_64BIT_GUEST_LINK_POINTER:
    case VMCS_64BIT_GUEST_IA32_DEBUGCTL:
    case VMCS_64BIT_GUEST_IA32_PAT:
    case VMCS_64BIT_GUEST_IA32_EFER:
      field_64 = VMread64(encoding);
      break;

    case VMCS_64BIT_GUEST_LINK_POINTER_HI:
    case VMCS_64BIT_GUEST_IA32_DEBUGCTL_HI:
    case VMCS_64BIT_GUEST_IA32_PAT_HI:
    case VMCS_64BIT_GUEST_IA32_EFER_HI:
      field_64 = VMread32(encoding);
      break;

    /* VMCS 64-bit host state fields */
    /* binary 0010_11xx_xxxx_xxx0 */
    case VMCS_64BIT_HOST_IA32_PAT:
    case VMCS_64BIT_HOST_IA32_EFER:
      field_64 = VMread64(encoding);
      break;

    case VMCS_64BIT_HOST_IA32_PAT_HI:
    case VMCS_64BIT_HOST_IA32_EFER_HI:
      field_64 = VMread32(encoding);
      break;

    /* VMCS natural width control fields */
    /* binary 0110_00xx_xxxx_xxx0 */
    case VMCS_CONTROL_CR0_GUEST_HOST_MASK:
    case VMCS_CONTROL_CR4_GUEST_HOST_MASK:
    case VMCS_CONTROL_CR0_READ_SHADOW:
    case VMCS_CONTROL_CR4_READ_SHADOW:
    case VMCS_CR3_TARGET0:
    case VMCS_CR3_TARGET1:
    case VMCS_CR3_TARGET2:
    case VMCS_CR3_TARGET3:
      // fall through

    /* VMCS natural width read only data fields */
    /* binary 0110_01xx_xxxx_xxx0 */
    case VMCS_VMEXIT_QUALIFICATION:
    case VMCS_IO_RCX:
    case VMCS_IO_RSI:
    case VMCS_IO_RDI:
    case VMCS_IO_RIP:
    case VMCS_GUEST_LINEAR_ADDR:
      // fall through

    /* VMCS natural width guest state fields */
    /* binary 0110_10xx_xxxx_xxx0 */
    case VMCS_GUEST_CR0:
    case VMCS_GUEST_CR3:
    case VMCS_GUEST_CR4:
    case VMCS_GUEST_ES_BASE:
    case VMCS_GUEST_CS_BASE:
    case VMCS_GUEST_SS_BASE:
    case VMCS_GUEST_DS_BASE:
    case VMCS_GUEST_FS_BASE:
    case VMCS_GUEST_GS_BASE:
    case VMCS_GUEST_LDTR_BASE:
    case VMCS_GUEST_TR_BASE:
    case VMCS_GUEST_GDTR_BASE:
    case VMCS_GUEST_IDTR_BASE:
    case VMCS_GUEST_DR7:
    case VMCS_GUEST_RSP:
    case VMCS_GUEST_RIP:
    case VMCS_GUEST_RFLAGS:
    case VMCS_GUEST_PENDING_DBG_EXCEPTIONS:
    case VMCS_GUEST_IA32_SYSENTER_ESP_MSR:
    case VMCS_GUEST_IA32_SYSENTER_EIP_MSR:
      // fall through

    /* VMCS natural width host state fields */
    /* binary 0110_11xx_xxxx_xxx0 */
    case VMCS_HOST_CR0:
    case VMCS_HOST_CR3:
    case VMCS_HOST_CR4:
    case VMCS_HOST_FS_BASE:
    case VMCS_HOST_GS_BASE:
    case VMCS_HOST_TR_BASE:
    case VMCS_HOST_GDTR_BASE:
    case VMCS_HOST_IDTR_BASE:
    case VMCS_HOST_IA32_SYSENTER_ESP_MSR:
    case VMCS_HOST_IA32_SYSENTER_EIP_MSR:
    case VMCS_HOST_RSP:
    case VMCS_HOST_RIP:
      field_64 = VMread64(encoding);
      break;

    default:
      BX_ERROR(("VMREAD: not supported field %08x", encoding));
      VMfail(VMXERR_UNSUPPORTED_VMCS_COMPONENT_ACCESS);
      return;
  };

#if BX_SUPPORT_X86_64
  if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) {
    if (i->modC0()) {
       BX_WRITE_64BIT_REG(i->rm(), field_64);
    }
    else {
       Bit64u eaddr = BX_CPU_CALL_METHODR(i->ResolveModrm, (i));
       write_virtual_qword_64(i->seg(), eaddr, field_64);
    }
  }
  else
#endif
  {
    Bit32u field_32 = GET32L(field_64);

    if (i->modC0()) {
       BX_WRITE_32BIT_REGZ(i->rm(), field_32);
    }
    else {
       Bit32u eaddr = BX_CPU_CALL_METHODR(i->ResolveModrm, (i));
       write_virtual_dword_32(i->seg(), eaddr, field_32);
    }
  }
 
  VMsucceed();
#else         
  BX_INFO(("VMREAD: required VMX support, use --enable-vmx option"));
  exception(BX_UD_EXCEPTION, 0, 0);
#endif  
}

void BX_CPU_C::VMWRITE(bxInstruction_c *i)
{
#if BX_SUPPORT_VMX
  if (! BX_CPU_THIS_PTR in_vmx || BX_CPU_THIS_PTR get_VM() || BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_COMPAT)
    exception(BX_UD_EXCEPTION, 0, 0);

  if (BX_CPU_THIS_PTR in_vmx_guest) {
    BX_ERROR(("VMEXIT: VMWRITE in VMX non-root operation"));
    VMexit_Instruction(i, VMX_VMEXIT_VMWRITE);
  }

  if (CPL != 0)
    exception(BX_GP_EXCEPTION, 0, 0);

  if (! VMCSPTR_VALID()) {
    BX_ERROR(("VMFAIL: VMWRITE with invalid VMCS ptr !"));
    VMfailInvalid();
    return;
  }

  unsigned encoding;
  Bit64u val_64;
  Bit32u val_32;

#if BX_SUPPORT_X86_64
  if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) {
    Bit64u enc_64;

    if (i->modC0()) {
       val_64 = BX_READ_64BIT_REG(i->rm());
    }
    else {
       Bit64u eaddr = BX_CPU_CALL_METHODR(i->ResolveModrm, (i));
       val_64 = read_virtual_qword_64(i->seg(), eaddr);
    }

    enc_64 = BX_READ_64BIT_REG(i->nnn());
    if (enc_64 >> 32) {
       BX_ERROR(("VMWRITE: not supported field !"));
       VMfail(VMXERR_UNSUPPORTED_VMCS_COMPONENT_ACCESS);
       return;
    }
    encoding = GET32L(enc_64);
    val_32   = GET32L(val_64);
  }
  else
#endif
  {
    if (i->modC0()) {
       val_32 = BX_READ_32BIT_REG(i->rm());
    }
    else {
       Bit32u eaddr = BX_CPU_CALL_METHODR(i->ResolveModrm, (i));
       val_32 = read_virtual_dword_32(i->seg(), eaddr);
    }

    encoding = BX_READ_32BIT_REG(i->nnn());
    val_64   = (Bit64u) val_32;
  }
/*
  if (VMCS_FIELD_TYPE(encoding) == VMCS_FIELD_TYPE_READ_ONLY)
  {
     BX_ERROR(("VMWRITE: write to read only field %08x", encoding));
     VMfail(VMXERR_VMWRITE_READ_ONLY_VMCS_COMPONENT);
     return;
  }
*/
  switch(encoding) {
    /* VMCS 16-bit guest-state fields */
    /* binary 0000_10xx_xxxx_xxx0 */
    case VMCS_16BIT_GUEST_ES_SELECTOR:
    case VMCS_16BIT_GUEST_CS_SELECTOR:
    case VMCS_16BIT_GUEST_SS_SELECTOR:
    case VMCS_16BIT_GUEST_DS_SELECTOR:
    case VMCS_16BIT_GUEST_FS_SELECTOR:
    case VMCS_16BIT_GUEST_GS_SELECTOR:
    case VMCS_16BIT_GUEST_LDTR_SELECTOR:
    case VMCS_16BIT_GUEST_TR_SELECTOR:
      // fall through

    /* VMCS 16-bit host-state fields */
    /* binary 0000_11xx_xxxx_xxx0 */
    case VMCS_16BIT_HOST_ES_SELECTOR:
    case VMCS_16BIT_HOST_CS_SELECTOR:
    case VMCS_16BIT_HOST_SS_SELECTOR:
    case VMCS_16BIT_HOST_DS_SELECTOR:
    case VMCS_16BIT_HOST_FS_SELECTOR:
    case VMCS_16BIT_HOST_GS_SELECTOR:
    case VMCS_16BIT_HOST_TR_SELECTOR:
      VMwrite16(encoding, val_32 & 0xffff);
      break;

    /* VMCS 32_bit control fields */
    /* binary 0100_00xx_xxxx_xxx0 */
    case VMCS_32BIT_CONTROL_PIN_BASED_EXEC_CONTROLS:
    case VMCS_32BIT_CONTROL_PROCESSOR_BASED_EXEC_CONTROLS:
    case VMCS_32BIT_CONTROL_EXECUTION_BITMAP:
    case VMCS_32BIT_CONTROL_PAGE_FAULT_ERR_CODE_MASK:
    case VMCS_32BIT_CONTROL_PAGE_FAULT_ERR_CODE_MATCH:
    case VMCS_32BIT_CONTROL_CR3_TARGET_COUNT:
    case VMCS_32BIT_CONTROL_VMEXIT_CONTROLS:
    case VMCS_32BIT_CONTROL_VMEXIT_MSR_STORE_COUNT:
    case VMCS_32BIT_CONTROL_VMEXIT_MSR_LOAD_COUNT:
    case VMCS_32BIT_CONTROL_VMENTRY_CONTROLS:
    case VMCS_32BIT_CONTROL_VMENTRY_MSR_LOAD_COUNT:
    case VMCS_32BIT_CONTROL_VMENTRY_INTERRUPTION_INFO:
    case VMCS_32BIT_CONTROL_VMENTRY_EXCEPTION_ERR_CODE:
    case VMCS_32BIT_CONTROL_VMENTRY_INSTRUCTION_LENGTH:
    case VMCS_32BIT_CONTROL_TPR_THRESHOLD:
      // fall through

    /* VMCS 32-bit guest-state fields */
    /* binary 0100_10xx_xxxx_xxx0 */
    case VMCS_32BIT_GUEST_ES_LIMIT:
    case VMCS_32BIT_GUEST_CS_LIMIT:
    case VMCS_32BIT_GUEST_SS_LIMIT:
    case VMCS_32BIT_GUEST_DS_LIMIT:
    case VMCS_32BIT_GUEST_FS_LIMIT:
    case VMCS_32BIT_GUEST_GS_LIMIT:
    case VMCS_32BIT_GUEST_LDTR_LIMIT:
    case VMCS_32BIT_GUEST_TR_LIMIT:
    case VMCS_32BIT_GUEST_GDTR_LIMIT:
    case VMCS_32BIT_GUEST_IDTR_LIMIT:
    case VMCS_32BIT_GUEST_INTERRUPTIBILITY_STATE:
    case VMCS_32BIT_GUEST_ACTIVITY_STATE:
    case VMCS_32BIT_GUEST_SMBASE:
    case VMCS_32BIT_GUEST_IA32_SYSENTER_CS_MSR:
      VMwrite32(encoding, val_32);
      break;

    case VMCS_32BIT_GUEST_ES_ACCESS_RIGHTS:
    case VMCS_32BIT_GUEST_CS_ACCESS_RIGHTS:
    case VMCS_32BIT_GUEST_SS_ACCESS_RIGHTS:
    case VMCS_32BIT_GUEST_DS_ACCESS_RIGHTS:
    case VMCS_32BIT_GUEST_FS_ACCESS_RIGHTS:
    case VMCS_32BIT_GUEST_GS_ACCESS_RIGHTS:
    case VMCS_32BIT_GUEST_LDTR_ACCESS_RIGHTS:
    case VMCS_32BIT_GUEST_TR_ACCESS_RIGHTS:
      VMwrite32(encoding, val_32 << 8);
      break;

    /* VMCS 32-bit host-state fields */
    /* binary 0100_11xx_xxxx_xxx0 */
    case VMCS_32BIT_HOST_IA32_SYSENTER_CS_MSR:
      VMwrite32(encoding, val_32);
      break;

    /* VMCS 64-bit control fields */
    /* binary 0010_00xx_xxxx_xxx0 */
    case VMCS_64BIT_CONTROL_IO_BITMAP_A_HI:
    case VMCS_64BIT_CONTROL_IO_BITMAP_B_HI:
    case VMCS_64BIT_CONTROL_MSR_BITMAPS_HI:
    case VMCS_64BIT_CONTROL_VMEXIT_MSR_STORE_ADDR_HI:
    case VMCS_64BIT_CONTROL_VMEXIT_MSR_LOAD_ADDR_HI:
    case VMCS_64BIT_CONTROL_VMENTRY_MSR_LOAD_ADDR_HI:
    case VMCS_64BIT_CONTROL_EXECUTIVE_VMCS_PTR_HI:
    case VMCS_64BIT_CONTROL_TSC_OFFSET_HI:
    case VMCS_64BIT_CONTROL_VIRTUAL_APIC_PAGE_ADDR_HI:
      // fall through

    /* VMCS 64-bit guest state fields */
    /* binary 0010_10xx_xxxx_xxx0 */
    case VMCS_64BIT_GUEST_LINK_POINTER_HI:
    case VMCS_64BIT_GUEST_IA32_DEBUGCTL_HI:
    case VMCS_64BIT_GUEST_IA32_PAT_HI:
    case VMCS_64BIT_GUEST_IA32_EFER_HI:
      // fall through

    /* VMCS 64-bit host state fields */
    /* binary 0010_11xx_xxxx_xxx0 */
    case VMCS_64BIT_HOST_IA32_PAT_HI:
    case VMCS_64BIT_HOST_IA32_EFER_HI:
      VMwrite32(encoding, val_32);
      break;

    /* VMCS 64-bit control fields */
    /* binary 0010_00xx_xxxx_xxx0 */
    case VMCS_64BIT_CONTROL_IO_BITMAP_A:
    case VMCS_64BIT_CONTROL_IO_BITMAP_B:
    case VMCS_64BIT_CONTROL_MSR_BITMAPS:
    case VMCS_64BIT_CONTROL_VMEXIT_MSR_STORE_ADDR:
    case VMCS_64BIT_CONTROL_VMEXIT_MSR_LOAD_ADDR:
    case VMCS_64BIT_CONTROL_VMENTRY_MSR_LOAD_ADDR:
    case VMCS_64BIT_CONTROL_EXECUTIVE_VMCS_PTR:
    case VMCS_64BIT_CONTROL_TSC_OFFSET:
    case VMCS_64BIT_CONTROL_VIRTUAL_APIC_PAGE_ADDR:
      // fall through

    /* VMCS 64-bit guest state fields */
    /* binary 0010_10xx_xxxx_xxx0 */
    case VMCS_64BIT_GUEST_LINK_POINTER:
    case VMCS_64BIT_GUEST_IA32_DEBUGCTL:
    case VMCS_64BIT_GUEST_IA32_PAT:
    case VMCS_64BIT_GUEST_IA32_EFER:
      // fall through

    /* VMCS 64-bit host state fields */
    /* binary 0010_11xx_xxxx_xxx0 */
    case VMCS_64BIT_HOST_IA32_PAT:
    case VMCS_64BIT_HOST_IA32_EFER:
      // fall through

    /* VMCS natural width control fields */
    /* binary 0110_00xx_xxxx_xxx0 */
    case VMCS_CONTROL_CR0_GUEST_HOST_MASK:
    case VMCS_CONTROL_CR4_GUEST_HOST_MASK:
    case VMCS_CONTROL_CR0_READ_SHADOW:
    case VMCS_CONTROL_CR4_READ_SHADOW:
    case VMCS_CR3_TARGET0:
    case VMCS_CR3_TARGET1:
    case VMCS_CR3_TARGET2:
    case VMCS_CR3_TARGET3:
      // fall through

    /* VMCS natural width guest state fields */
    /* binary 0110_10xx_xxxx_xxx0 */
    case VMCS_GUEST_CR0:
    case VMCS_GUEST_CR3:
    case VMCS_GUEST_CR4:
    case VMCS_GUEST_ES_BASE:
    case VMCS_GUEST_CS_BASE:
    case VMCS_GUEST_SS_BASE:
    case VMCS_GUEST_DS_BASE:
    case VMCS_GUEST_FS_BASE:
    case VMCS_GUEST_GS_BASE:
    case VMCS_GUEST_LDTR_BASE:
    case VMCS_GUEST_TR_BASE:
    case VMCS_GUEST_GDTR_BASE:
    case VMCS_GUEST_IDTR_BASE:
    case VMCS_GUEST_DR7:
    case VMCS_GUEST_RSP:
    case VMCS_GUEST_RIP:
    case VMCS_GUEST_RFLAGS:
    case VMCS_GUEST_PENDING_DBG_EXCEPTIONS:
    case VMCS_GUEST_IA32_SYSENTER_ESP_MSR:
    case VMCS_GUEST_IA32_SYSENTER_EIP_MSR:
      // fall through

    /* VMCS natural width host state fields */
    /* binary 0110_11xx_xxxx_xxx0 */
    case VMCS_HOST_CR0:
    case VMCS_HOST_CR3:
    case VMCS_HOST_CR4:
    case VMCS_HOST_FS_BASE:
    case VMCS_HOST_GS_BASE:
    case VMCS_HOST_TR_BASE:
    case VMCS_HOST_GDTR_BASE:
    case VMCS_HOST_IDTR_BASE:
    case VMCS_HOST_IA32_SYSENTER_ESP_MSR:
    case VMCS_HOST_IA32_SYSENTER_EIP_MSR:
    case VMCS_HOST_RSP:
    case VMCS_HOST_RIP:
      VMwrite64(encoding, val_64);
      break;

    /* VMCS 32-bit read only data fields */
    /* binary 0100_01xx_xxxx_xxx0 */
    case VMCS_32BIT_INSTRUCTION_ERROR:
    case VMCS_32BIT_VMEXIT_REASON:
    case VMCS_32BIT_VMEXIT_INTERRUPTION_INFO:
    case VMCS_32BIT_VMEXIT_INTERRUPTION_ERR_CODE:
    case VMCS_32BIT_IDT_VECTORING_INFO:
    case VMCS_32BIT_IDT_VECTORING_ERR_CODE:
    case VMCS_32BIT_VMEXIT_INSTRUCTION_LENGTH:
    case VMCS_32BIT_VMEXIT_INSTRUCTION_INFO:

    /* VMCS natural width read only data fields */
    /* binary 0110_01xx_xxxx_xxx0 */
    case VMCS_VMEXIT_QUALIFICATION:
    case VMCS_IO_RCX:
    case VMCS_IO_RSI:
    case VMCS_IO_RDI:
    case VMCS_IO_RIP:
    case VMCS_GUEST_LINEAR_ADDR:
      BX_ERROR(("VMWRITE: write to read/only field %08x", encoding));
      VMfail(VMXERR_VMWRITE_READ_ONLY_VMCS_COMPONENT);
      return;

    default:
      BX_ERROR(("VMWRITE: write to not supported field %08x", encoding));
      VMfail(VMXERR_UNSUPPORTED_VMCS_COMPONENT_ACCESS);
      return;
  };

  VMsucceed();
#else         
  BX_INFO(("VMWRITE: required VMX support, use --enable-vmx option"));
  exception(BX_UD_EXCEPTION, 0, 0);
#endif  
}

void BX_CPU_C::VMCLEAR(bxInstruction_c *i)
{
#if BX_SUPPORT_VMX
  if (! BX_CPU_THIS_PTR in_vmx || BX_CPU_THIS_PTR get_VM() || BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_COMPAT)
    exception(BX_UD_EXCEPTION, 0, 0);

  if (BX_CPU_THIS_PTR in_vmx_guest) {
    BX_ERROR(("VMEXIT: VMCLEAR in VMX non-root operation"));
    VMexit_Instruction(i, VMX_VMEXIT_VMCLEAR);
  }

  if (CPL != 0)
    exception(BX_GP_EXCEPTION, 0, 0);

  bx_address eaddr = BX_CPU_CALL_METHODR(i->ResolveModrm, (i));
  Bit64u pAddr = read_virtual_qword(i->seg(), eaddr); // keep 64-bit
  if ((pAddr & 0xfff) != 0 || ! IsValidPhyAddr(pAddr)) {
    BX_ERROR(("VMFAIL: VMCLEAR with invalid physical address!"));
    VMfail(VMXERR_VMCLEAR_WITH_INVALID_ADDR);
    return;
  }

  if (pAddr == BX_CPU_THIS_PTR vmxonptr) {
    BX_ERROR(("VMFAIL: VMLEAR with VMXON ptr !"));
    VMfail(VMXERR_VMCLEAR_WITH_VMXON_VMCS_PTR);
  }
  else {
    // ensure that data for VMCS referenced by the operand is in memory
    // initialize implementation-specific data in VMCS region

    // clear VMCS launch state
    Bit32u launch_state = VMCS_STATE_CLEAR;
    access_write_physical(pAddr + VMCS_LAUNCH_STATE_FIELD_ADDR, 4, &launch_state);
    BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID, pAddr + VMCS_LAUNCH_STATE_FIELD_ADDR, 4,
            BX_WRITE, (Bit8u*)(&launch_state));

    if (pAddr == BX_CPU_THIS_PTR vmcsptr) {
        BX_CPU_THIS_PTR vmcsptr = BX_INVALID_VMCSPTR;
        BX_CPU_THIS_PTR vmcshostptr = 0;
    }

    VMsucceed();
  }
#else         
  BX_INFO(("VMCLEAR: required VMX support, use --enable-vmx option"));
  exception(BX_UD_EXCEPTION, 0, 0);
#endif  
}

#if BX_SUPPORT_VMX
void BX_CPU_C::register_vmx_state(bx_param_c *parent)
{
  // register VMX state for save/restore param tree
  bx_list_c *vmx = new bx_list_c(parent, "VMX", 6);

  BXRS_HEX_PARAM_FIELD(vmx, vmcsptr, BX_CPU_THIS_PTR vmcsptr);
  BXRS_HEX_PARAM_FIELD(vmx, vmxonptr, BX_CPU_THIS_PTR vmxonptr);
  BXRS_PARAM_BOOL(vmx, in_vmx, BX_CPU_THIS_PTR in_vmx);
  BXRS_PARAM_BOOL(vmx, in_vmx_guest, BX_CPU_THIS_PTR in_vmx_guest);
  BXRS_PARAM_BOOL(vmx, vmx_interrupt_window, BX_CPU_THIS_PTR vmx_interrupt_window);

  bx_list_c *vmcache = new bx_list_c(vmx, "VMCS_CACHE", 5);

  //
  // VM-Execution Control Fields
  //

  bx_list_c *vmexec_ctrls = new bx_list_c(vmcache, "VMEXEC_CTRLS", 21);

  BXRS_HEX_PARAM_FIELD(vmexec_ctrls, vmexec_ctrls1, BX_CPU_THIS_PTR vmcs.vmexec_ctrls1);
  BXRS_HEX_PARAM_FIELD(vmexec_ctrls, vmexec_ctrls2, BX_CPU_THIS_PTR vmcs.vmexec_ctrls2);
  BXRS_HEX_PARAM_FIELD(vmexec_ctrls, vm_exceptions_bitmap, BX_CPU_THIS_PTR vmcs.vm_exceptions_bitmap);
  BXRS_HEX_PARAM_FIELD(vmexec_ctrls, vm_pf_mask, BX_CPU_THIS_PTR vmcs.vm_pf_mask);
  BXRS_HEX_PARAM_FIELD(vmexec_ctrls, vm_pf_match, BX_CPU_THIS_PTR vmcs.vm_pf_match);
  BXRS_HEX_PARAM_FIELD(vmexec_ctrls, io_bitmap_addr1, BX_CPU_THIS_PTR vmcs.io_bitmap_addr[0]);
  BXRS_HEX_PARAM_FIELD(vmexec_ctrls, io_bitmap_addr2, BX_CPU_THIS_PTR vmcs.io_bitmap_addr[1]);
  BXRS_HEX_PARAM_FIELD(vmexec_ctrls, tsc_offset, BX_CPU_THIS_PTR vmcs.tsc_offset);
  BXRS_HEX_PARAM_FIELD(vmexec_ctrls, msr_bitmap_addr, BX_CPU_THIS_PTR vmcs.msr_bitmap_addr);
  BXRS_HEX_PARAM_FIELD(vmexec_ctrls, vm_cr0_mask, BX_CPU_THIS_PTR vmcs.vm_cr0_mask);
  BXRS_HEX_PARAM_FIELD(vmexec_ctrls, vm_cr0_read_shadow, BX_CPU_THIS_PTR vmcs.vm_cr0_read_shadow);
  BXRS_HEX_PARAM_FIELD(vmexec_ctrls, vm_cr4_mask, BX_CPU_THIS_PTR vmcs.vm_cr4_mask);
  BXRS_HEX_PARAM_FIELD(vmexec_ctrls, vm_cr4_read_shadow, BX_CPU_THIS_PTR vmcs.vm_cr4_read_shadow);
  BXRS_DEC_PARAM_FIELD(vmexec_ctrls, vm_cr3_target_cnt, BX_CPU_THIS_PTR vmcs.vm_cr3_target_cnt);
  BXRS_HEX_PARAM_FIELD(vmexec_ctrls, vm_cr3_target_value1, BX_CPU_THIS_PTR vmcs.vm_cr3_target_value[0]);
  BXRS_HEX_PARAM_FIELD(vmexec_ctrls, vm_cr3_target_value2, BX_CPU_THIS_PTR vmcs.vm_cr3_target_value[1]);
  BXRS_HEX_PARAM_FIELD(vmexec_ctrls, vm_cr3_target_value3, BX_CPU_THIS_PTR vmcs.vm_cr3_target_value[2]);
  BXRS_HEX_PARAM_FIELD(vmexec_ctrls, vm_cr3_target_value4, BX_CPU_THIS_PTR vmcs.vm_cr3_target_value[3]);
  BXRS_HEX_PARAM_FIELD(vmexec_ctrls, virtual_apic_page_addr, BX_CPU_THIS_PTR vmcs.virtual_apic_page_addr);
  BXRS_HEX_PARAM_FIELD(vmexec_ctrls, vm_tpr_threshold, BX_CPU_THIS_PTR vmcs.vm_tpr_threshold);
  BXRS_HEX_PARAM_FIELD(vmexec_ctrls, executive_vmcsptr, BX_CPU_THIS_PTR vmcs.executive_vmcsptr);

  //
  // VM-Exit Control Fields
  //

  bx_list_c *vmexit_ctrls = new bx_list_c(vmcache, "VMEXIT_CTRLS", 5);

  BXRS_HEX_PARAM_FIELD(vmexit_ctrls, vmexit_ctrls, BX_CPU_THIS_PTR vmcs.vmexit_ctrls);
  BXRS_DEC_PARAM_FIELD(vmexit_ctrls, vmexit_msr_store_cnt, BX_CPU_THIS_PTR vmcs.vmexit_msr_store_cnt);
  BXRS_HEX_PARAM_FIELD(vmexit_ctrls, vmexit_msr_store_addr, BX_CPU_THIS_PTR vmcs.vmexit_msr_store_addr);
  BXRS_DEC_PARAM_FIELD(vmexit_ctrls, vmexit_msr_load_cnt, BX_CPU_THIS_PTR vmcs.vmexit_msr_load_cnt);
  BXRS_HEX_PARAM_FIELD(vmexit_ctrls, vmexit_msr_load_addr, BX_CPU_THIS_PTR vmcs.vmexit_msr_load_addr);
   
  //
  // VM-Entry Control Fields
  //

  bx_list_c *vmentry_ctrls = new bx_list_c(vmcache, "VMENTRY_CTRLS", 6);
   
  BXRS_HEX_PARAM_FIELD(vmentry_ctrls, vmentry_ctrls, BX_CPU_THIS_PTR vmcs.vmentry_ctrls);
  BXRS_DEC_PARAM_FIELD(vmentry_ctrls, vmentry_msr_load_cnt, BX_CPU_THIS_PTR vmcs.vmentry_msr_load_cnt);
  BXRS_HEX_PARAM_FIELD(vmentry_ctrls, vmentry_msr_load_addr, BX_CPU_THIS_PTR vmcs.vmentry_msr_load_addr);
  BXRS_HEX_PARAM_FIELD(vmentry_ctrls, vmentry_interr_info, BX_CPU_THIS_PTR vmcs.vmentry_interr_info);
  BXRS_HEX_PARAM_FIELD(vmentry_ctrls, vmentry_excep_err_code, BX_CPU_THIS_PTR vmcs.vmentry_excep_err_code);
  BXRS_HEX_PARAM_FIELD(vmentry_ctrls, vmentry_instr_length, BX_CPU_THIS_PTR vmcs.vmentry_instr_length);

  //
  // VM-Exit Information Fields
  //
/*
  bx_list_c *vmexit_info = new bx_list_c(vmcache, "VMEXIT_INFO", 14);

  BXRS_HEX_PARAM_FIELD(vmexit_info, vmexit_reason, BX_CPU_THIS_PTR vmcs.vmexit_reason);
  BXRS_HEX_PARAM_FIELD(vmexit_info, vmexit_qualification, BX_CPU_THIS_PTR vmcs.vmexit_qualification);
  BXRS_HEX_PARAM_FIELD(vmexit_info, vmexit_excep_info, BX_CPU_THIS_PTR vmcs.vmexit_excep_info);
  BXRS_HEX_PARAM_FIELD(vmexit_info, vmexit_excep_error_code, BX_CPU_THIS_PTR vmcs.vmexit_excep_error_code);
  BXRS_HEX_PARAM_FIELD(vmexit_info, vmexit_idt_vector_info, BX_CPU_THIS_PTR vmcs.vmexit_idt_vector_info);
  BXRS_HEX_PARAM_FIELD(vmexit_info, vmexit_idt_vector_error_code, BX_CPU_THIS_PTR vmcs.vmexit_idt_vector_error_code);
  BXRS_HEX_PARAM_FIELD(vmexit_info, vmexit_instr_info, BX_CPU_THIS_PTR vmcs.vmexit_instr_info);
  BXRS_HEX_PARAM_FIELD(vmexit_info, vmexit_instr_length, BX_CPU_THIS_PTR vmcs.vmexit_instr_length);
  BXRS_HEX_PARAM_FIELD(vmexit_info, vmexit_guest_laddr, BX_CPU_THIS_PTR vmcs.vmexit_guest_laddr);
  BXRS_HEX_PARAM_FIELD(vmexit_info, vmexit_io_rcx, BX_CPU_THIS_PTR vmcs.vmexit_io_rcx);
  BXRS_HEX_PARAM_FIELD(vmexit_info, vmexit_io_rsi, BX_CPU_THIS_PTR vmcs.vmexit_io_rsi);
  BXRS_HEX_PARAM_FIELD(vmexit_info, vmexit_io_rdi, BX_CPU_THIS_PTR vmcs.vmexit_io_rdi);
  BXRS_HEX_PARAM_FIELD(vmexit_info, vmexit_io_rip, BX_CPU_THIS_PTR vmcs.vmexit_io_rip);
  BXRS_HEX_PARAM_FIELD(vmexit_info, vm_instr_error, BX_CPU_THIS_PTR vmcs.vm_instr_error);
*/
  //
  // VMCS Host State
  //

  bx_list_c *host = new bx_list_c(vmcache, "HOST_STATE", 22);

#undef NEED_CPU_REG_SHORTCUTS

  BXRS_HEX_PARAM_FIELD(host, CR0, BX_CPU_THIS_PTR vmcs.host_state.cr0);
  BXRS_HEX_PARAM_FIELD(host, CR3, BX_CPU_THIS_PTR vmcs.host_state.cr3);
  BXRS_HEX_PARAM_FIELD(host, CR4, BX_CPU_THIS_PTR vmcs.host_state.cr4);
  BXRS_HEX_PARAM_FIELD(host, ES, BX_CPU_THIS_PTR vmcs.host_state.segreg_selector[BX_SEG_REG_ES]);
  BXRS_HEX_PARAM_FIELD(host, CS, BX_CPU_THIS_PTR vmcs.host_state.segreg_selector[BX_SEG_REG_CS]);
  BXRS_HEX_PARAM_FIELD(host, SS, BX_CPU_THIS_PTR vmcs.host_state.segreg_selector[BX_SEG_REG_SS]);
  BXRS_HEX_PARAM_FIELD(host, DS, BX_CPU_THIS_PTR vmcs.host_state.segreg_selector[BX_SEG_REG_DS]);
  BXRS_HEX_PARAM_FIELD(host, FS, BX_CPU_THIS_PTR vmcs.host_state.segreg_selector[BX_SEG_REG_FS]);
  BXRS_HEX_PARAM_FIELD(host, FS_BASE, BX_CPU_THIS_PTR vmcs.host_state.fs_base);
  BXRS_HEX_PARAM_FIELD(host, GS, BX_CPU_THIS_PTR vmcs.host_state.segreg_selector[BX_SEG_REG_GS]);
  BXRS_HEX_PARAM_FIELD(host, GS_BASE, BX_CPU_THIS_PTR vmcs.host_state.gs_base);
  BXRS_HEX_PARAM_FIELD(host, GDTR_BASE, BX_CPU_THIS_PTR vmcs.host_state.gdtr_base);
  BXRS_HEX_PARAM_FIELD(host, IDTR_BASE, BX_CPU_THIS_PTR vmcs.host_state.idtr_base);
  BXRS_HEX_PARAM_FIELD(host, TR, BX_CPU_THIS_PTR vmcs.host_state.tr_selector);
  BXRS_HEX_PARAM_FIELD(host, TR_BASE, BX_CPU_THIS_PTR vmcs.host_state.tr_base);
  BXRS_HEX_PARAM_FIELD(host, RSP, BX_CPU_THIS_PTR vmcs.host_state.rsp);
  BXRS_HEX_PARAM_FIELD(host, RIP, BX_CPU_THIS_PTR vmcs.host_state.rip);
  BXRS_HEX_PARAM_FIELD(host, sysenter_esp_msr, BX_CPU_THIS_PTR vmcs.host_state.sysenter_esp_msr);
  BXRS_HEX_PARAM_FIELD(host, sysenter_eip_msr, BX_CPU_THIS_PTR vmcs.host_state.sysenter_eip_msr);
  BXRS_HEX_PARAM_FIELD(host, sysenter_cs_msr, BX_CPU_THIS_PTR vmcs.host_state.sysenter_cs_msr);
  BXRS_HEX_PARAM_FIELD(host, pat_msr, BX_CPU_THIS_PTR vmcs.host_state.pat_msr);
#if BX_SUPPORT_X86_64
  BXRS_HEX_PARAM_FIELD(host, efer_msr, BX_CPU_THIS_PTR vmcs.host_state.efer_msr);
#endif
}

#endif // BX_SUPPORT_VMX
