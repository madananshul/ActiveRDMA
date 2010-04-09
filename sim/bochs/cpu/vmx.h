/////////////////////////////////////////////////////////////////////////
// $Id: vmx.h,v 1.7 2009/10/14 20:45:29 sshwarts Exp $
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

#ifndef _BX_VMX_INTEL_H_
#define _BX_VMX_INTEL_H_

#define VMX_VMCS_AREA_SIZE   4096
#define VMX_VMCS_REVISION_ID 0x10

// VMCS pointer is always 64-bit variable
#define BX_INVALID_VMCSPTR BX_CONST64(0xFFFFFFFFFFFFFFFF)

// VMX error codes
enum VMX_error_code {
    VMXERR_NO_ERROR = 0,
    VMXERR_VMCALL_IN_VMX_ROOT_OPERATION = 1,
    VMXERR_VMCLEAR_WITH_INVALID_ADDR = 2,
    VMXERR_VMCLEAR_WITH_VMXON_VMCS_PTR = 3,
    VMXERR_VMLAUNCH_NON_CLEAR_VMCS = 4,
    VMXERR_VMRESUME_NON_LAUNCHED_VMCS = 5,
    VMXERR_VMRESUME_VMCS_CORRUPTED = 6,
    VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD = 7,
    VMXERR_VMENTRY_INVALID_VM_HOST_STATE_FIELD = 8,
    VMXERR_VMPTRLD_INVALID_PHYSICAL_ADDRESS = 9,
    VMXERR_VMPTRLD_WITH_VMXON_PTR = 10,
    VMXERR_VMPTRLD_INCORRECT_VMCS_REVISION_ID = 11,
    VMXERR_UNSUPPORTED_VMCS_COMPONENT_ACCESS = 12,
    VMXERR_VMWRITE_READ_ONLY_VMCS_COMPONENT = 13,
    VMXERR_RESERVED14 = 14,
    VMXERR_VMXON_IN_VMX_ROOT_OPERATION = 15,
    VMXERR_VMENTRY_INVALID_EXECUTIVE_VMCS = 16,
    VMXERR_VMENTRY_NON_LAUNCHED_EXECUTIVE_VMCS = 17,
    VMXERR_VMENTRY_NOT_VMXON_EXECUTIVE_VMCS = 18,
    VMXERR_VMCALL_NON_CLEAR_VMCS = 19,
    VMXERR_VMCALL_INVALID_VMEXIT_FIELD = 20,
    VMXERR_RESERVED21 = 21,
    VMXERR_VMCALL_INVALID_MSEG_REVISION_ID = 22,
    VMXERR_VMXOFF_WITH_CONFIGURED_SMM_MONITOR = 23,
    VMXERR_VMCALL_WITH_INVALID_SMM_MONITOR_FEATURES = 24,
    VMXERR_VMENTRY_INVALID_VM_CONTROL_FIELD_IN_EXECUTIVE_VMCS = 25,
    VMXERR_VMENTRY_MOV_SS_BLOCKING = 26,
    VMXERR_RESERVED27 = 27
};

enum VMX_vmexit_reason {
   VMX_VMEXIT_EXCEPTION_NMI = 0,
   VMX_VMEXIT_EXTERNAL_INTERRUPT = 1,
   VMX_VMEXIT_TRIPLE_FAULT = 2,
   VMX_VMEXIT_INIT = 3,
   VMX_VMEXIT_SIPI = 4,
   VMX_VMEXIT_IO_SMI = 5,
   VMX_VMEXIT_SMI = 6,
   VMX_VMEXIT_INTERRUPT_WINDOW = 7,
   VMX_VMEXIT_NMI_WINDOW = 8,
   VMX_VMEXIT_TASK_SWITCH = 9,
   VMX_VMEXIT_CPUID = 10,
   VMX_VMEXIT_RESERVED11 = 11,
   VMX_VMEXIT_HLT = 12,
   VMX_VMEXIT_INVD = 13,
   VMX_VMEXIT_INVLPG = 14,
   VMX_VMEXIT_RDPMC = 15,
   VMX_VMEXIT_RDTSC = 16,
   VMX_VMEXIT_RSM = 17,
   VMX_VMEXIT_VMCALL = 18,
   VMX_VMEXIT_VMCLEAR = 19,
   VMX_VMEXIT_VMLAUNCH = 20,
   VMX_VMEXIT_VMPTRLD = 21,
   VMX_VMEXIT_VMPTRST = 22,
   VMX_VMEXIT_VMREAD = 23,
   VMX_VMEXIT_VMRESUME = 24,
   VMX_VMEXIT_VMWRITE = 25,
   VMX_VMEXIT_VMXOFF = 26,
   VMX_VMEXIT_VMXON = 27,
   VMX_VMEXIT_CR_ACCESS = 28,
   VMX_VMEXIT_DR_ACCESS = 29,
   VMX_VMEXIT_IO_INSTRUCTION = 30,
   VMX_VMEXIT_RDMSR = 31,
   VMX_VMEXIT_WRMSR = 32,
   VMX_VMEXIT_VMENTRY_FAILURE_GUEST_STATE = 33,
   VMX_VMEXIT_VMENTRY_FAILURE_MSR = 34,
   VMX_VMEXIT_RESERVED35 = 35,
   VMX_VMEXIT_MWAIT = 36,
   VMX_VMEXIT_MONITOR_TRAP_FLAG = 37,
   VMX_VMEXIT_RESERVED38 = 38,
   VMX_VMEXIT_MONITOR = 39,
   VMX_VMEXIT_PAUSE = 40,
   VMX_VMEXIT_VMENTRY_FAILURE_MCA = 41, // will never happen in Bochs
   VMX_VMEXIT_RESERVED42 = 42,
   VMX_VMEXIT_TPR_THRESHOLD = 43,
   VMX_VMEXIT_APIC_ACCESS = 44,
   VMX_VMEXIT_RESERVED45 = 45,
   VMX_VMEXIT_GDTR_IDTR_ACCESS = 46,
   VMX_VMEXIT_LDTR_TR_ACCESS = 47,
   VMX_VMEXIT_EPT_VIOLATION = 48,
   VMX_VMEXIT_EPT_MISCONFIGURATION = 49,
   VMX_VMEXIT_INVEPT = 50,
   VMX_VMEXIT_RDTSCP = 51,
   VMX_VMEXIT_VMX_PREEMTION_TIMER_FIRED = 52,
   VMX_VMEXIT_INVVPID = 53,
   VMX_VMEXIT_WBINVD = 54,
   VMX_VMEXIT_XSETBV = 55
};

// VMexit on CR register access
enum {
   VMX_VMEXIT_CR_ACCESS_CR_WRITE = 0,
   VMX_VMEXIT_CR_ACCESS_CR_READ,
   VMX_VMEXIT_CR_ACCESS_CLTS,
   VMX_VMEXIT_CR_ACCESS_LMSW
};

// VMENTRY error on loading guest state qualification
enum VMX_vmentry_error {
   VMENTER_ERR_NO_ERROR = 0,
   VMENTER_ERR_GUEST_STATE_PDPTR_LOADING = 2,
   VMENTER_ERR_GUEST_STATE_INJECT_NMI_BLOCKING_EVENTS = 3,
   VMENTER_ERR_GUEST_STATE_LINK_POINTER = 4
};

// VMABORT error code
enum VMX_vmabort_code {
   VMABORT_SAVING_GUEST_MSRS_FAILURE,
   VMABORT_HOST_PDPTR_CORRUPTED,
   VMABORT_VMEXIT_VMCS_CORRUPTED,
   VMABORT_LOADING_HOST_MSRS,
   VMABORT_VMEXIT_MACHINE_CHECK_ERROR
};

// =============
//  VMCS fields
// =============

/* VMCS 16-bit guest-state fields */
/* binary 0000_10xx_xxxx_xxx0 */
#define VMCS_16BIT_GUEST_ES_SELECTOR                       0x00000800
#define VMCS_16BIT_GUEST_CS_SELECTOR                       0x00000802
#define VMCS_16BIT_GUEST_SS_SELECTOR                       0x00000804
#define VMCS_16BIT_GUEST_DS_SELECTOR                       0x00000806
#define VMCS_16BIT_GUEST_FS_SELECTOR                       0x00000808
#define VMCS_16BIT_GUEST_GS_SELECTOR                       0x0000080A
#define VMCS_16BIT_GUEST_LDTR_SELECTOR                     0x0000080C
#define VMCS_16BIT_GUEST_TR_SELECTOR                       0x0000080E

/* VMCS 16-bit host-state fields */
/* binary 0000_11xx_xxxx_xxx0 */
#define VMCS_16BIT_HOST_ES_SELECTOR                        0x00000C00
#define VMCS_16BIT_HOST_CS_SELECTOR                        0x00000C02
#define VMCS_16BIT_HOST_SS_SELECTOR                        0x00000C04
#define VMCS_16BIT_HOST_DS_SELECTOR                        0x00000C06
#define VMCS_16BIT_HOST_FS_SELECTOR                        0x00000C08
#define VMCS_16BIT_HOST_GS_SELECTOR                        0x00000C0A
#define VMCS_16BIT_HOST_TR_SELECTOR                        0x00000C0C

/* VMCS 64-bit control fields */
/* binary 0010_00xx_xxxx_xxx0 */
#define VMCS_64BIT_CONTROL_IO_BITMAP_A                     0x00002000
#define VMCS_64BIT_CONTROL_IO_BITMAP_A_HI                  0x00002001
#define VMCS_64BIT_CONTROL_IO_BITMAP_B                     0x00002002
#define VMCS_64BIT_CONTROL_IO_BITMAP_B_HI                  0x00002003
#define VMCS_64BIT_CONTROL_MSR_BITMAPS                     0x00002004
#define VMCS_64BIT_CONTROL_MSR_BITMAPS_HI                  0x00002005
#define VMCS_64BIT_CONTROL_VMEXIT_MSR_STORE_ADDR           0x00002006
#define VMCS_64BIT_CONTROL_VMEXIT_MSR_STORE_ADDR_HI        0x00002007
#define VMCS_64BIT_CONTROL_VMEXIT_MSR_LOAD_ADDR            0x00002008
#define VMCS_64BIT_CONTROL_VMEXIT_MSR_LOAD_ADDR_HI         0x00002009
#define VMCS_64BIT_CONTROL_VMENTRY_MSR_LOAD_ADDR           0x0000200A
#define VMCS_64BIT_CONTROL_VMENTRY_MSR_LOAD_ADDR_HI        0x0000200B
#define VMCS_64BIT_CONTROL_EXECUTIVE_VMCS_PTR              0x0000200C
#define VMCS_64BIT_CONTROL_EXECUTIVE_VMCS_PTR_HI           0x0000200D
#define VMCS_64BIT_CONTROL_TSC_OFFSET                      0x00002010
#define VMCS_64BIT_CONTROL_TSC_OFFSET_HI                   0x00002011
#define VMCS_64BIT_CONTROL_VIRTUAL_APIC_PAGE_ADDR          0x00002012
#define VMCS_64BIT_CONTROL_VIRTUAL_APIC_PAGE_ADDR_HI       0x00002013
#define VMCS_64BIT_CONTROL_APIC_ACCESS_ADDR                0x00002014
#define VMCS_64BIT_CONTROL_APIC_ACCESS_ADDR_HI             0x00002015

/* VMCS 64-bit guest state fields */
/* binary 0010_10xx_xxxx_xxx0 */
#define VMCS_64BIT_GUEST_LINK_POINTER                      0x00002800
#define VMCS_64BIT_GUEST_LINK_POINTER_HI                   0x00002801
#define VMCS_64BIT_GUEST_IA32_DEBUGCTL                     0x00002802
#define VMCS_64BIT_GUEST_IA32_DEBUGCTL_HI                  0x00002803
#define VMCS_64BIT_GUEST_IA32_PAT                          0x00002804
#define VMCS_64BIT_GUEST_IA32_PAT_HI                       0x00002805
#define VMCS_64BIT_GUEST_IA32_EFER                         0x00002806
#define VMCS_64BIT_GUEST_IA32_EFER_HI                      0x00002807

/* VMCS 64-bit host state fields */
/* binary 0010_11xx_xxxx_xxx0 */
#define VMCS_64BIT_HOST_IA32_PAT                           0x00002C00
#define VMCS_64BIT_HOST_IA32_PAT_HI                        0x00002C01
#define VMCS_64BIT_HOST_IA32_EFER                          0x00002C02
#define VMCS_64BIT_HOST_IA32_EFER_HI                       0x00002C03

/* VMCS 32_bit control fields */
/* binary 0100_00xx_xxxx_xxx0 */
#define VMCS_32BIT_CONTROL_PIN_BASED_EXEC_CONTROLS         0x00004000
#define VMCS_32BIT_CONTROL_PROCESSOR_BASED_EXEC_CONTROLS   0x00004002
#define VMCS_32BIT_CONTROL_EXECUTION_BITMAP                0x00004004
#define VMCS_32BIT_CONTROL_PAGE_FAULT_ERR_CODE_MASK        0x00004006
#define VMCS_32BIT_CONTROL_PAGE_FAULT_ERR_CODE_MATCH       0x00004008
#define VMCS_32BIT_CONTROL_CR3_TARGET_COUNT                0x0000400A
#define VMCS_32BIT_CONTROL_VMEXIT_CONTROLS                 0x0000400C
#define VMCS_32BIT_CONTROL_VMEXIT_MSR_STORE_COUNT          0x0000400E
#define VMCS_32BIT_CONTROL_VMEXIT_MSR_LOAD_COUNT           0x00004010
#define VMCS_32BIT_CONTROL_VMENTRY_CONTROLS                0x00004012
#define VMCS_32BIT_CONTROL_VMENTRY_MSR_LOAD_COUNT          0x00004014
#define VMCS_32BIT_CONTROL_VMENTRY_INTERRUPTION_INFO       0x00004016
#define VMCS_32BIT_CONTROL_VMENTRY_EXCEPTION_ERR_CODE      0x00004018
#define VMCS_32BIT_CONTROL_VMENTRY_INSTRUCTION_LENGTH      0x0000401A
#define VMCS_32BIT_CONTROL_TPR_THRESHOLD                   0x0000401C

/* VMCS 32-bit read only data fields */
/* binary 0100_01xx_xxxx_xxx0 */
#define VMCS_32BIT_INSTRUCTION_ERROR                       0x00004400
#define VMCS_32BIT_VMEXIT_REASON                           0x00004402
#define VMCS_32BIT_VMEXIT_INTERRUPTION_INFO                0x00004404
#define VMCS_32BIT_VMEXIT_INTERRUPTION_ERR_CODE            0x00004406
#define VMCS_32BIT_IDT_VECTORING_INFO                      0x00004408
#define VMCS_32BIT_IDT_VECTORING_ERR_CODE                  0x0000440A
#define VMCS_32BIT_VMEXIT_INSTRUCTION_LENGTH               0x0000440C
#define VMCS_32BIT_VMEXIT_INSTRUCTION_INFO                 0x0000440E

/* VMCS 32-bit guest-state fields */
/* binary 0100_10xx_xxxx_xxx0 */
#define VMCS_32BIT_GUEST_ES_LIMIT                          0x00004800
#define VMCS_32BIT_GUEST_CS_LIMIT                          0x00004802
#define VMCS_32BIT_GUEST_SS_LIMIT                          0x00004804
#define VMCS_32BIT_GUEST_DS_LIMIT                          0x00004806
#define VMCS_32BIT_GUEST_FS_LIMIT                          0x00004808
#define VMCS_32BIT_GUEST_GS_LIMIT                          0x0000480A
#define VMCS_32BIT_GUEST_LDTR_LIMIT                        0x0000480C
#define VMCS_32BIT_GUEST_TR_LIMIT                          0x0000480E
#define VMCS_32BIT_GUEST_GDTR_LIMIT                        0x00004810
#define VMCS_32BIT_GUEST_IDTR_LIMIT                        0x00004812
#define VMCS_32BIT_GUEST_ES_ACCESS_RIGHTS                  0x00004814
#define VMCS_32BIT_GUEST_CS_ACCESS_RIGHTS                  0x00004816
#define VMCS_32BIT_GUEST_SS_ACCESS_RIGHTS                  0x00004818
#define VMCS_32BIT_GUEST_DS_ACCESS_RIGHTS                  0x0000481A
#define VMCS_32BIT_GUEST_FS_ACCESS_RIGHTS                  0x0000481C
#define VMCS_32BIT_GUEST_GS_ACCESS_RIGHTS                  0x0000481E
#define VMCS_32BIT_GUEST_LDTR_ACCESS_RIGHTS                0x00004820
#define VMCS_32BIT_GUEST_TR_ACCESS_RIGHTS                  0x00004822
#define VMCS_32BIT_GUEST_INTERRUPTIBILITY_STATE            0x00004824
#define VMCS_32BIT_GUEST_ACTIVITY_STATE                    0x00004826
#define VMCS_32BIT_GUEST_SMBASE                            0x00004828
#define VMCS_32BIT_GUEST_IA32_SYSENTER_CS_MSR              0x0000482A

/* VMCS 32-bit host-state fields */
/* binary 0100_11xx_xxxx_xxx0 */
#define VMCS_32BIT_HOST_IA32_SYSENTER_CS_MSR               0x00004C00

/* VMCS natural width control fields */
/* binary 0110_00xx_xxxx_xxx0 */
#define VMCS_CONTROL_CR0_GUEST_HOST_MASK                   0x00006000
#define VMCS_CONTROL_CR4_GUEST_HOST_MASK                   0x00006002
#define VMCS_CONTROL_CR0_READ_SHADOW                       0x00006004
#define VMCS_CONTROL_CR4_READ_SHADOW                       0x00006006
#define VMCS_CR3_TARGET0                                   0x00006008
#define VMCS_CR3_TARGET1                                   0x0000600A
#define VMCS_CR3_TARGET2                                   0x0000600C
#define VMCS_CR3_TARGET3                                   0x0000600E
                                                           
/* VMCS natural width read only data fields */
/* binary 0110_01xx_xxxx_xxx0 */
#define VMCS_VMEXIT_QUALIFICATION                          0x00006400
#define VMCS_IO_RCX                                        0x00006402
#define VMCS_IO_RSI                                        0x00006404
#define VMCS_IO_RDI                                        0x00006406
#define VMCS_IO_RIP                                        0x00006408
#define VMCS_GUEST_LINEAR_ADDR                             0x0000640A

/* VMCS natural width guest state fields */
/* binary 0110_10xx_xxxx_xxx0 */
#define VMCS_GUEST_CR0                                     0x00006800
#define VMCS_GUEST_CR3                                     0x00006802
#define VMCS_GUEST_CR4                                     0x00006804
#define VMCS_GUEST_ES_BASE                                 0x00006806
#define VMCS_GUEST_CS_BASE                                 0x00006808
#define VMCS_GUEST_SS_BASE                                 0x0000680A
#define VMCS_GUEST_DS_BASE                                 0x0000680C
#define VMCS_GUEST_FS_BASE                                 0x0000680E
#define VMCS_GUEST_GS_BASE                                 0x00006810
#define VMCS_GUEST_LDTR_BASE                               0x00006812
#define VMCS_GUEST_TR_BASE                                 0x00006814
#define VMCS_GUEST_GDTR_BASE                               0x00006816
#define VMCS_GUEST_IDTR_BASE                               0x00006818
#define VMCS_GUEST_DR7                                     0x0000681A
#define VMCS_GUEST_RSP                                     0x0000681C
#define VMCS_GUEST_RIP                                     0x0000681E
#define VMCS_GUEST_RFLAGS                                  0x00006820
#define VMCS_GUEST_PENDING_DBG_EXCEPTIONS                  0x00006822
#define VMCS_GUEST_IA32_SYSENTER_ESP_MSR                   0x00006824
#define VMCS_GUEST_IA32_SYSENTER_EIP_MSR                   0x00006826

/* VMCS natural width host state fields */
/* binary 0110_11xx_xxxx_xxx0 */
#define VMCS_HOST_CR0                                      0x00006C00
#define VMCS_HOST_CR3                                      0x00006C02
#define VMCS_HOST_CR4                                      0x00006C04
#define VMCS_HOST_FS_BASE                                  0x00006C06
#define VMCS_HOST_GS_BASE                                  0x00006C08
#define VMCS_HOST_TR_BASE                                  0x00006C0A
#define VMCS_HOST_GDTR_BASE                                0x00006C0C
#define VMCS_HOST_IDTR_BASE                                0x00006C0E
#define VMCS_HOST_IA32_SYSENTER_ESP_MSR                    0x00006C10
#define VMCS_HOST_IA32_SYSENTER_EIP_MSR                    0x00006C12
#define VMCS_HOST_RSP                                      0x00006C14
#define VMCS_HOST_RIP                                      0x00006C16

// ===============================
//  VMCS fields encoding/decoding
// ===============================

// extract VMCS field using its encoding
#define VMCS_FIELD(encoding)        ((encoding) & 0x3ff)

// check if the VMCS field encoding corresponding to HI part of 64-bit value
#define IS_VMCS_FIELD_HI(encoding)  ((encoding) & 1)

// bits 11:10 of VMCS field encoding indicate field's type
#define VMCS_FIELD_TYPE(encoding)   (((encoding) >> 10) & 3)

#define VMCS_FIELD_TYPE_CONTROL        0x0
#define VMCS_FIELD_TYPE_READ_ONLY      0x1
#define VMCS_FIELD_TYPE_GUEST_STATE    0x2
#define VMCS_FIELD_TYPE_HOST_STATE     0x3

// bits 14:13 of VMCS field encoding indicate field's width
#define VMCS_FIELD_WIDTH(encoding)  (((encoding) >> 13) & 3)

#define VMCS_FIELD_WIDTH_16BIT         0x0
#define VMCS_FIELD_WIDTH_64BIT         0x1
#define VMCS_FIELD_WIDTH_32BIT         0x2
#define VMCS_FIELD_WIDTH_NATURAL_WIDTH 0x3

#define VMCS_FIELD_INDEX(encoding) \
    ((VMCS_FIELD_WIDTH(encoding) << 2) + VMCS_FIELD_TYPE(encoding))

// =============
//  VMCS layout
// =============

#define VMCS_REVISION_ID_FIELD_ADDR              (0x0000)
#define VMCS_VMX_ABORT_FIELD_ADDR                (0x0004)
#define VMCS_LAUNCH_STATE_FIELD_ADDR             (0x0008)

// invent Bochs CPU VMCS layout - allocate 32 fields of each type
#define VMCS_DATA_OFFSET                         (0x0010)

#if ((VMCS_DATA_OFFSET + 32*16*4) > VMX_VMCS_AREA_SIZE)
  #error "VMCS area size exceeded !"
#endif

// =============
//  VMCS state
// =============

enum VMX_state {
   VMCS_STATE_CLEAR = 0,
   VMCS_STATE_LAUNCHED
};

// ================
//  VMCS structure
// ================

typedef struct bx_VMCS_GUEST_STATE 
{
   bx_address cr0;
   bx_address cr3;
   bx_address cr4;
   bx_address dr7;

   bx_address rip;
   bx_address rsp;
   bx_address rflags;

   bx_segment_reg_t sregs[6];

   bx_global_segment_reg_t gdtr;
   bx_global_segment_reg_t idtr;
   bx_segment_reg_t        ldtr;
   bx_segment_reg_t        tr;

   Bit64u ia32_debugctl_msr;
   bx_address sysenter_esp_msr;
   bx_address sysenter_eip_msr;
   Bit32u sysenter_cs_msr;

   Bit32u smbase;
   Bit32u activity_state;
   Bit32u interruptibility_state;
   Bit32u tmpDR6;

   Bit64u link_pointer;

   Bit64u pat_msr;
#if BX_SUPPORT_X86_64
   Bit64u efer_msr;
#endif
} VMCS_GUEST_STATE;

typedef struct bx_VMCS_HOST_STATE
{
   bx_address cr0;
   bx_address cr3;
   bx_address cr4;

   Bit16u segreg_selector[6];

   bx_address fs_base;
   bx_address gs_base;

   bx_address gdtr_base;
   bx_address idtr_base;

   Bit32u tr_selector;
   bx_address tr_base;

   bx_address rsp;
   bx_address rip;

   bx_address sysenter_esp_msr;
   bx_address sysenter_eip_msr;
   Bit32u sysenter_cs_msr;

   Bit64u pat_msr;
#if BX_SUPPORT_X86_64
   Bit64u efer_msr;
#endif
} VMCS_HOST_STATE;

typedef struct bx_VMCS
{
  //
  // VM-Execution Control Fields
  //

#define VMX_VM_EXEC_CTRL1_EXTERNAL_INTERRUPT_VMEXIT (1 << 0)
#define VMX_VM_EXEC_CTRL1_NMI_VMEXIT                (1 << 3)
#define VMX_VM_EXEC_CTRL1_VIRTUAL_NMI               (1 << 5)
#define VMX_VM_EXEC_CTRL1_VMX_PREEMPTION_TIMER      (1 << 6)

#ifdef BX_VMX_ENABLE_ALL

#define VMX_VM_EXEC_CTRL1_SUPPORTED_BITS (0x00000069)

#else // only really supported features

#define VMX_VM_EXEC_CTRL1_SUPPORTED_BITS \
       (VMX_VM_EXEC_CTRL1_EXTERNAL_INTERRUPT_VMEXIT | \
        VMX_VM_EXEC_CTRL1_NMI_VMEXIT)

#endif

   Bit32u vmexec_ctrls1;

#define VMX_VM_EXEC_CTRL2_INTERRUPT_WINDOW_VMEXIT   (1 << 2)
#define VMX_VM_EXEC_CTRL2_TSC_OFFSET                (1 << 3)
#define VMX_VM_EXEC_CTRL2_HLT_VMEXIT                (1 << 7)
#define VMX_VM_EXEC_CTRL2_INVLPG_VMEXIT             (1 << 9)
#define VMX_VM_EXEC_CTRL2_MWAIT_VMEXIT              (1 << 10)
#define VMX_VM_EXEC_CTRL2_RDPMC_VMEXIT              (1 << 11)
#define VMX_VM_EXEC_CTRL2_RDTSC_VMEXIT              (1 << 12)
#define VMX_VM_EXEC_CTRL2_CR3_WRITE_VMEXIT          (1 << 15) /* legacy must be '1 */
#define VMX_VM_EXEC_CTRL2_CR3_READ_VMEXIT           (1 << 16) /* legacy must be '1 */
#define VMX_VM_EXEC_CTRL2_CR8_WRITE_VMEXIT          (1 << 19)
#define VMX_VM_EXEC_CTRL2_CR8_READ_VMEXIT           (1 << 20)
#define VMX_VM_EXEC_CTRL2_TPR_SHADOW                (1 << 21)
#define VMX_VM_EXEC_CTRL2_NMI_WINDOW_VMEXIT         (1 << 22)
#define VMX_VM_EXEC_CTRL2_DRx_ACCESS_VMEXIT         (1 << 23)
#define VMX_VM_EXEC_CTRL2_IO_VMEXIT                 (1 << 24)
#define VMX_VM_EXEC_CTRL2_IO_BITMAPS                (1 << 25)
#define VMX_VM_EXEC_CTRL2_MONITOR_TRAP_FLAG         (1 << 27)
#define VMX_VM_EXEC_CTRL2_MSR_BITMAPS               (1 << 28)
#define VMX_VM_EXEC_CTRL2_MONITOR_VMEXIT            (1 << 29)
#define VMX_VM_EXEC_CTRL2_PAUSE_VMEXIT              (1 << 30)
#define VMX_VM_EXEC_CTRL2_SECONDARY_CONTROLS        (1 << 31)

#ifdef BX_VMX_ENABLE_ALL

#define VMX_VM_EXEC_CTRL2_SUPPORTED_BITS (0xFBF99E8C)

#else // only really supported features

#define VMX_VM_EXEC_CTRL2_SUPPORTED_BITS \
       (VMX_VM_EXEC_CTRL2_INTERRUPT_WINDOW_VMEXIT | \
        VMX_VM_EXEC_CTRL2_TSC_OFFSET | \
        VMX_VM_EXEC_CTRL2_HLT_VMEXIT | \
        VMX_VM_EXEC_CTRL2_INVLPG_VMEXIT | \
        VMX_VM_EXEC_CTRL2_MWAIT_VMEXIT | \
        VMX_VM_EXEC_CTRL2_RDPMC_VMEXIT | \
        VMX_VM_EXEC_CTRL2_RDTSC_VMEXIT | \
        VMX_VM_EXEC_CTRL2_CR3_WRITE_VMEXIT | \
        VMX_VM_EXEC_CTRL2_CR3_READ_VMEXIT | \
        VMX_VM_EXEC_CTRL2_CR8_WRITE_VMEXIT | \
        VMX_VM_EXEC_CTRL2_CR8_READ_VMEXIT | \
        VMX_VM_EXEC_CTRL2_NMI_WINDOW_VMEXIT | \
        VMX_VM_EXEC_CTRL2_DRx_ACCESS_VMEXIT | \
        VMX_VM_EXEC_CTRL2_IO_VMEXIT | \
        VMX_VM_EXEC_CTRL2_IO_BITMAPS | \
        VMX_VM_EXEC_CTRL2_MSR_BITMAPS | \
        VMX_VM_EXEC_CTRL2_MONITOR_VMEXIT | \
        VMX_VM_EXEC_CTRL2_PAUSE_VMEXIT)

#endif

   Bit32u vmexec_ctrls2;
   Bit32u vm_exceptions_bitmap;
   Bit32u vm_pf_mask;
   Bit32u vm_pf_match;
   Bit64u io_bitmap_addr[2];
   Bit64u tsc_offset;
   bx_phy_address msr_bitmap_addr;

   bx_address vm_cr0_mask;
   bx_address vm_cr0_read_shadow;
   bx_address vm_cr4_mask;
   bx_address vm_cr4_read_shadow;

#define VMX_CR3_TARGET_MAX_CNT 4

   Bit32u vm_cr3_target_cnt;
   bx_address vm_cr3_target_value[4];

   bx_phy_address virtual_apic_page_addr;
   Bit32u vm_tpr_threshold;

   Bit64u executive_vmcsptr;

   //
   // VM-Exit Control Fields
   //

#define VMX_VMEXIT_CTRL1_SAVE_DBG_CTRLS             (1 <<  2) /* legacy must be '1 */
#define VMX_VMEXIT_CTRL1_HOST_ADDR_SPACE_SIZE       (1 <<  9)
#define VMX_VMEXIT_CTRL1_LOAD_PERF_GLOBAL_CTRL_MSR  (1 << 12)
#define VMX_VMEXIT_CTRL1_INTA_ON_VMEXIT             (1 << 15)
#define VMX_VMEXIT_CTRL1_STORE_PAT_MSR              (1 << 18)
#define VMX_VMEXIT_CTRL1_LOAD_PAT_MSR               (1 << 19)
#define VMX_VMEXIT_CTRL1_STORE_EFER_MSR             (1 << 20)
#define VMX_VMEXIT_CTRL1_LOAD_EFER_MSR              (1 << 21)
#define VMX_VMEXIT_CTRL1_STORE_VMX_PREEMPTION_TIMER (1 << 22)

#ifdef BX_VMX_ENABLE_ALL

#define VMX_VMEXIT_CTRL1_SUPPORTED_BITS (0x007C9204)

#else // only really supported features

#define VMX_VMEXIT_CTRL1_SUPPORTED_BITS \
       (VMX_VMEXIT_CTRL1_SAVE_DBG_CTRLS | \
        VMX_VMEXIT_CTRL1_HOST_ADDR_SPACE_SIZE | \
        VMX_VMEXIT_CTRL1_INTA_ON_VMEXIT)

#endif

   Bit32u vmexit_ctrls;

   Bit32u vmexit_msr_store_cnt;
   bx_phy_address vmexit_msr_store_addr;
   Bit32u vmexit_msr_load_cnt;
   bx_phy_address vmexit_msr_load_addr;
   
   //
   // VM-Entry Control Fields
   //

#define VMX_VMENTRY_CTRL1_LOAD_DBG_CTRLS                    (1 <<  2) /* legacy must be '1 */
#define VMX_VMENTRY_CTRL1_X86_64_GUEST                      (1 <<  9)
#define VMX_VMENTRY_CTRL1_SMM_ENTER                         (1 << 10)
#define VMX_VMENTRY_CTRL1_DEACTIVATE_DUAL_MONITOR_TREATMENT (1 << 11)
#define VMX_VMENTRY_CTRL1_LOAD_PERF_GLOBAL_CTRL_MSR         (1 << 13)
#define VMX_VMENTRY_CTRL1_LOAD_PAT_MSR                      (1 << 14)
#define VMX_VMENTRY_CTRL1_LOAD_EFER_MSR                     (1 << 15)

#ifdef BX_VMX_ENABLE_ALL

#define VMX_VMENTRY_CTRL1_SUPPORTED_BITS (0x0000EE04)

#else // only really supported features

#define VMX_VMENTRY_CTRL1_SUPPORTED_BITS \
       (VMX_VMENTRY_CTRL1_LOAD_DBG_CTRLS | \
        VMX_VMENTRY_CTRL1_X86_64_GUEST | \
        VMX_VMENTRY_CTRL1_SMM_ENTER | \
        VMX_VMENTRY_CTRL1_DEACTIVATE_DUAL_MONITOR_TREATMENT)

#endif
   
   Bit32u vmentry_ctrls;

   Bit32u vmentry_msr_load_cnt;
   bx_phy_address vmentry_msr_load_addr;

   Bit32u vmentry_interr_info;
   Bit32u vmentry_excep_err_code;
   Bit32u vmentry_instr_length;

   //
   // VMCS Hidden and Read-Only Fields
   //
/*
   Bit32u vmexit_reason;
   bx_address vmexit_qualification;
   Bit32u vmexit_instr_info;
   Bit32u vmexit_instr_length;
   bx_address vmexit_guest_laddr;
   Bit32u vmexit_excep_info;
   Bit32u vmexit_excep_error_code;

   bx_address vmexit_io_rcx;
   bx_address vmexit_io_rsi;
   bx_address vmexit_io_rdi;
   bx_address vmexit_io_rip;

   Bit32u vm_instr_error;
*/
   Bit32u idt_vector_info;
   Bit32u idt_vector_error_code;

   //
   // VMCS Host State
   //

   VMCS_HOST_STATE host_state;

} VMCS_CACHE;

enum VMX_Activity_State {
   BX_VMX_ACTIVE_STATE = 0,
   BX_VMX_STATE_HLT,
   BX_VMX_STATE_SHUTDOWN,
   BX_VMX_STATE_WAIT_FOR_SIPI,
   BX_VMX_LAST_ACTIVITY_STATE
};

#define PIN_VMEXIT(ctrl) (BX_CPU_THIS_PTR vmcs.vmexec_ctrls1 & (ctrl))
#define     VMEXIT(ctrl) (BX_CPU_THIS_PTR vmcs.vmexec_ctrls2 & (ctrl))

#define BX_VMX_INTERRUPTS_BLOCKED_BY_STI      (1 << 0)
#define BX_VMX_INTERRUPTS_BLOCKED_BY_MOV_SS   (1 << 1)
#define BX_VMX_INTERRUPTS_BLOCKED_SMI_BLOCKED (1 << 2)
#define BX_VMX_INTERRUPTS_BLOCKED_NMI_BLOCKED (1 << 3)

#define BX_VMX_INTERRUPTIBILITY_STATE_MASK \
    (BX_VMX_INTERRUPTS_BLOCKED_BY_STI | BX_VMX_INTERRUPTS_BLOCKED_BY_MOV_SS | \
     BX_VMX_INTERRUPTS_BLOCKED_SMI_BLOCKED | \
     BX_VMX_INTERRUPTS_BLOCKED_NMI_BLOCKED)

//
// IA32_VMX_BASIC MSR (0x480)
// --------------

//
// 31:00 32-bit VMCS revision id
// 32:47 VMCS region size, 0 <= size <= 4096
// 48:48 use 32-bit physical address, set when x86_64 disabled
// 49:49 support of dual-monitor treatment of SMI and SMM
// 53:50 memory type used for VMCS access
// 54:54 logical processor reports information in the VM-exit 
//       instruction-information field on VM exits due to
//       execution of INS/OUTS
// 55:55 set if any VMX controls that default to `1 may be
//       cleared to `0
// 56:63 reserved, must be zero
//

#define VMX_MSR_VMX_BASIC_LO (VMX_VMCS_REVISION_ID)
#define VMX_MSR_VMX_BASIC_HI \
     (VMX_VMCS_AREA_SIZE | ((!BX_SUPPORT_X86_64) << 16) | (BX_MEMTYPE_WB << 18))

#define VMX_MSR_VMX_BASIC \
   ((((Bit64u) VMX_MSR_VMX_BASIC_HI) << 32) | VMX_MSR_VMX_BASIC_LO)


// ------------------------------------------------------------------------
//              reserved bit (must be '1) settings for VMX MSRs
// ------------------------------------------------------------------------

// -----------------------------------------
//  3322|2222|2222|1111|1111|11  |    |
//  1098|7654|3210|9876|5432|1098|7654|3210
// -----------------------------------------
//  ----.----.----.----.----.----.---1.-11-  MSR (0x481) IA32_MSR_VMX_PINBASED_CTRLS
//  ----.-1--.----.---1.111-.---1.-111.--1-  MSR (0x482) IA32_MSR_VMX_PROCBASED_CTRLS
//  ----.----.----.--11.-11-.11-1.1111.1111  MSR (0x483) IA32_MSR_VMX_VMEXIT_CTRLS
//  ----.----.----.----.---1.---1.1111.1111  MSR (0x484) IA32_MSR_VMX_VMENTRY_CTRLS
//

// IA32_MSR_VMX_PINBASED_CTRLS MSR (0x481)
// ---------------------------

// Bits 1, 2 and 4 must be '1 

// Allowed 0-settings: VMentry fail if a bit is '0 in pin-based vmexec controls
// but set to '1 in this MSR
#define VMX_MSR_VMX_PINBASED_CTRLS_LO  (0x00000016)
// Allowed 1-settings: VMentry fail if a bit is '1 in pin-based vmexec controls
// but set to '0 in this MSR.
#define VMX_MSR_VMX_PINBASED_CTRLS_HI \
       (VMX_VM_EXEC_CTRL1_SUPPORTED_BITS | VMX_MSR_VMX_PINBASED_CTRLS_LO)

#define VMX_MSR_VMX_PINBASED_CTRLS \
   ((((Bit64u) VMX_MSR_VMX_PINBASED_CTRLS_HI) << 32) | VMX_MSR_VMX_PINBASED_CTRLS_LO)


// IA32_MSR_VMX_PROCBASED_CTRLS MSR (0x482)
// ----------------------------

// Bits 1, 4-6, 8, 13-16, 26 must be '1
// Bits 0, 17, 18 must be '0
// Bits 19-21 also must be '0 when x86-64 is not supported

// Allowed 0-settings (must be '1 bits)
#define VMX_MSR_VMX_PROCBASED_CTRLS_LO (0x0401E172)
// Allowed 1-settings
#define VMX_MSR_VMX_PROCBASED_CTRLS_HI \
       (VMX_VM_EXEC_CTRL2_SUPPORTED_BITS | VMX_MSR_VMX_PROCBASED_CTRLS_LO)

#define VMX_MSR_VMX_PROCBASED_CTRLS \
   ((((Bit64u) VMX_MSR_VMX_PROCBASED_CTRLS_HI) << 32) | VMX_MSR_VMX_PROCBASED_CTRLS_LO)


// IA32_MSR_VMX_VMEXIT_CTRLS MSR (0x483)
// -------------------------

// Bits 0-8, 10, 11, 13, 14, 16, 17 must be '1

// Allowed 0-settings (must be '1 bits)
#define VMX_MSR_VMX_VMEXIT_CTRLS_LO    (0x00036DFF)
// Allowed 1-settings
#define VMX_MSR_VMX_VMEXIT_CTRLS_HI \
       (VMX_VMEXIT_CTRL1_SUPPORTED_BITS | VMX_MSR_VMX_VMEXIT_CTRLS_LO)

#define VMX_MSR_VMX_VMEXIT_CTRLS \
   ((((Bit64u) VMX_MSR_VMX_VMEXIT_CTRLS_HI) << 32) | VMX_MSR_VMX_VMEXIT_CTRLS_LO)


// IA32_MSR_VMX_VMENTRY_CTRLS MSR (0x484)
// --------------------------

// Bits 0-8, 12 must be '1

// Allowed 0-settings (must be '1 bits)
#define VMX_MSR_VMX_VMENTRY_CTRLS_LO   (0x000011FF)
// Allowed 1-settings
#define VMX_MSR_VMX_VMENTRY_CTRLS_HI \
       (VMX_VMEXIT_CTRL1_SUPPORTED_BITS | VMX_MSR_VMX_VMENTRY_CTRLS_LO)

#define VMX_MSR_VMX_VMENTRY_CTRLS \
   ((((Bit64u) VMX_MSR_VMX_VMENTRY_CTRLS_HI) << 32) | VMX_MSR_VMX_VMENTRY_CTRLS_LO)


//
// IA32_VMX_CR0_FIXED0 MSR (0x486)   IA32_VMX_CR0_FIXED1 MSR (0x487)
// -------------------               -------------------

// allowed 0-setting in CR0 in VMX mode
// bits PE(0), NE(5) and PG(31) required to be set in CR0 to enter VMX mode
#define VMX_MSR_CR0_FIXED0_LO          (0x80000021)
#define VMX_MSR_CR0_FIXED0_HI          (0x00000000)

#define VMX_MSR_CR0_FIXED0 \
   ((((Bit64u) VMX_MSR_CR0_FIXED0_HI) << 32) | VMX_MSR_CR0_FIXED0_LO)

// allowed 1-setting in CR0 in VMX mode
#define VMX_MSR_CR0_FIXED1_LO          (0xFFFFFFFF)
#define VMX_MSR_CR0_FIXED1_HI          (0x00000000)

#define VMX_MSR_CR0_FIXED1 \
   ((((Bit64u) VMX_MSR_CR0_FIXED1_HI) << 32) | VMX_MSR_CR0_FIXED1_LO)


//
// IA32_VMX_CR4_FIXED0 MSR (0x488)   IA32_VMX_CR4_FIXED1 MSR (0x489)
// -------------------               -------------------

// allowed 0-setting in CR0 in VMX mode
// bit VMXE(13) required to be set in CR4 to enter VMX mode
#define VMX_MSR_CR4_FIXED0_LO          (0x00002000)
#define VMX_MSR_CR4_FIXED0_HI          (0x00000000)

#define VMX_MSR_CR4_FIXED0 \
   ((((Bit64u) VMX_MSR_CR4_FIXED0_HI) << 32) | VMX_MSR_CR4_FIXED0_LO)

// allowed 1-setting in CR0 in VMX mode
#define VMX_MSR_CR4_FIXED1_LO GET32L(get_cr4_allow_mask())
#define VMX_MSR_CR4_FIXED1_HI GET32H(get_cr4_allow_mask())

#define VMX_MSR_CR4_FIXED1 \
   ((((Bit64u) VMX_MSR_CR4_FIXED1_HI) << 32) | VMX_MSR_CR4_FIXED1_LO)


//
// IA32_VMX_VMCS_ENUM MSR (0x48A)
// ------------------

//
// 09:01 highest index value used for any VMCS encoding
// 63:10 reserved, must be zero
//

#define VMX_HIGHEST_VMCS_ENCODING 0x2A

#define VMX_MSR_VMCS_ENUM_LO (VMX_HIGHEST_VMCS_ENCODING)
#define VMX_MSR_VMCS_ENUM_HI (0x00000000)

#define VMX_MSR_VMCS_ENUM \
   ((((Bit64u) VMX_MSR_VMCS_ENUM_HI) << 32) | VMX_MSR_VMCS_ENUM_LO)


#endif // _BX_VMX_INTEL_H_
