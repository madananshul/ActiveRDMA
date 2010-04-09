/////////////////////////////////////////////////////////////////////////
// $Id: msr.cc,v 1.29 2009/11/04 17:04:28 sshwarts Exp $
/////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2008-2009 Stanislav Shwartsman
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

#if BX_SUPPORT_X86_64==0
// Make life easier for merging code.
#define RAX EAX
#define RDX EDX
#endif

#if BX_CPU_LEVEL >= 5
bx_bool BX_CPP_AttrRegparmN(2) BX_CPU_C::rdmsr(Bit32u index, Bit64u *msr)
{
  Bit64u val64 = 0;

  switch(index) {

#if BX_SUPPORT_SEP
    case BX_MSR_SYSENTER_CS:
      val64 = BX_CPU_THIS_PTR msr.sysenter_cs_msr;
      break;

    case BX_MSR_SYSENTER_ESP:
      val64 = BX_CPU_THIS_PTR msr.sysenter_esp_msr;
      break;

    case BX_MSR_SYSENTER_EIP:
      val64 = BX_CPU_THIS_PTR msr.sysenter_eip_msr;
      break;
#endif

#if BX_CPU_LEVEL >= 6
    case BX_MSR_MTRRCAP:   // read only MSR
      val64 = BX_CONST64(0x0000000000000508);
      break;

    case BX_MSR_MTRRPHYSBASE0:
    case BX_MSR_MTRRPHYSMASK0:
    case BX_MSR_MTRRPHYSBASE1:
    case BX_MSR_MTRRPHYSMASK1:
    case BX_MSR_MTRRPHYSBASE2:
    case BX_MSR_MTRRPHYSMASK2:
    case BX_MSR_MTRRPHYSBASE3:
    case BX_MSR_MTRRPHYSMASK3:
    case BX_MSR_MTRRPHYSBASE4:
    case BX_MSR_MTRRPHYSMASK4:
    case BX_MSR_MTRRPHYSBASE5:
    case BX_MSR_MTRRPHYSMASK5:
    case BX_MSR_MTRRPHYSBASE6:
    case BX_MSR_MTRRPHYSMASK6:
    case BX_MSR_MTRRPHYSBASE7:
    case BX_MSR_MTRRPHYSMASK7:
      val64 = BX_CPU_THIS_PTR msr.mtrrphys[index - BX_MSR_MTRRPHYSBASE0];
      break;

    case BX_MSR_MTRRFIX64K_00000:
      val64 = BX_CPU_THIS_PTR msr.mtrrfix64k_00000;
      break;
    case BX_MSR_MTRRFIX16K_80000:
    case BX_MSR_MTRRFIX16K_A0000:
      val64 = BX_CPU_THIS_PTR msr.mtrrfix16k[index - BX_MSR_MTRRFIX16K_80000];
      break;

    case BX_MSR_MTRRFIX4K_C0000:
    case BX_MSR_MTRRFIX4K_C8000:
    case BX_MSR_MTRRFIX4K_D0000:
    case BX_MSR_MTRRFIX4K_D8000:
    case BX_MSR_MTRRFIX4K_E0000:
    case BX_MSR_MTRRFIX4K_E8000:
    case BX_MSR_MTRRFIX4K_F0000:
    case BX_MSR_MTRRFIX4K_F8000:
      val64 = BX_CPU_THIS_PTR msr.mtrrfix4k[index - BX_MSR_MTRRFIX4K_C0000];
      break;

    case BX_MSR_PAT:
      val64 = BX_CPU_THIS_PTR msr.pat;
      break;

    case BX_MSR_MTRR_DEFTYPE:
      val64 = BX_CPU_THIS_PTR msr.mtrr_deftype;
      break;
#endif

    case BX_MSR_TSC:
      val64 = BX_CPU_THIS_PTR get_TSC();
      break;

#if BX_SUPPORT_APIC
    case BX_MSR_APICBASE:
      val64 = BX_CPU_THIS_PTR msr.apicbase;
      BX_INFO(("RDMSR: Read %08x:%08x from MSR_APICBASE", GET32H(val64), GET32L(val64)));
      break;
#endif

#if BX_SUPPORT_VMX
/*
    case BX_MSR_IA32_SMM_MONITOR_CTL:
      BX_PANIC(("Dual-monitor treatment of SMI and SMM is not implemented"));
      break;
*/
    case BX_MSR_VMX_BASIC:
      val64 = VMX_MSR_VMX_BASIC;
      break;
    case BX_MSR_VMX_PINBASED_CTRLS:
      val64 = VMX_MSR_VMX_PINBASED_CTRLS;
      break;
    case BX_MSR_VMX_PROCBASED_CTRLS:
      val64 = VMX_MSR_VMX_PROCBASED_CTRLS;
      break;
    case BX_MSR_VMX_VMEXIT_CTRLS:
      val64 = VMX_MSR_VMX_VMEXIT_CTRLS;
      break;
    case BX_MSR_VMX_VMENTRY_CTRLS:
      val64 = VMX_MSR_VMX_VMENTRY_CTRLS;
      break;
    case BX_MSR_VMX_CR0_FIXED0:
      val64 = VMX_MSR_CR0_FIXED0;
      break;
    case BX_MSR_VMX_CR0_FIXED1:
      val64 = VMX_MSR_CR0_FIXED1;
      break;
    case BX_MSR_VMX_CR4_FIXED0:
      val64 = VMX_MSR_CR4_FIXED0;
      break;
    case BX_MSR_VMX_CR4_FIXED1:
      val64 = VMX_MSR_CR4_FIXED1;
      break;
    case BX_MSR_VMX_VMCS_ENUM:
      val64 = VMX_MSR_VMCS_ENUM;
      break;
#endif

#if BX_SUPPORT_X86_64
    case BX_MSR_EFER:
      val64 = BX_CPU_THIS_PTR efer.get32();
      break;

    case BX_MSR_STAR:
      val64 = MSR_STAR;
      break;

    case BX_MSR_LSTAR:
      val64 = MSR_LSTAR;
      break;

    case BX_MSR_CSTAR:
      val64 = MSR_CSTAR;
      break;

    case BX_MSR_FMASK:
      val64 = MSR_FMASK;
      break;

    case BX_MSR_FSBASE:
      val64 = MSR_FSBASE;
      break;

    case BX_MSR_GSBASE:
      val64 = MSR_GSBASE;
      break;

    case BX_MSR_KERNELGSBASE:
      val64 = MSR_KERNELGSBASE;
      break;

    case BX_MSR_TSC_AUX:
      val64 = MSR_TSC_AUX;   // 32 bit MSR
      break;
#endif  // #if BX_SUPPORT_X86_64

    default:
#if BX_CONFIGURE_MSRS
      if (index < BX_MSR_MAX_INDEX && BX_CPU_THIS_PTR msrs[index]) {
        val64 = BX_CPU_THIS_PTR msrs[index]->get64();
        break;
      }
#endif
      // failed to find the MSR, could #GP or ignore it silently
      BX_ERROR(("RDMSR: Unknown register %#x", index));

#if BX_IGNORE_BAD_MSR == 0
      return 0; // will result in #GP fault due to unknown MSR
#endif
  }

  *msr = val64;
  return 1;
}
#endif // BX_CPU_LEVEL >= 5

void BX_CPP_AttrRegparmN(1) BX_CPU_C::RDMSR(bxInstruction_c *i)
{
#if BX_CPU_LEVEL >= 5
  if (!real_mode() && CPL != 0) {
    BX_ERROR(("RDMSR: CPL != 0 not in real mode"));
    exception(BX_GP_EXCEPTION, 0, 0);
  }

  Bit32u index = ECX;
  Bit64u val64 = 0;

#if BX_SUPPORT_VMX
  VMexit_MSR(i, VMX_VMEXIT_RDMSR, index);
#endif

  if (!rdmsr(index, &val64))
    exception(BX_GP_EXCEPTION, 0, 0);

  RAX = GET32L(val64);
  RDX = GET32H(val64);
#else
  BX_INFO(("RDMSR: Pentium CPU required, use --enable-cpu-level=5"));
  exception(BX_UD_EXCEPTION, 0, 0);
#endif
}

#if BX_CPU_LEVEL >= 6
BX_CPP_INLINE bx_bool isMemTypeValidMTRR(unsigned memtype)
{
  switch(memtype) {
  case BX_MEMTYPE_UC:
  case BX_MEMTYPE_WC:
  case BX_MEMTYPE_WT:
  case BX_MEMTYPE_WP:
  case BX_MEMTYPE_WB:
    return 1;
  default:
    return 0;
  }
}

BX_CPP_INLINE bx_bool isMemTypeValidPAT(unsigned memtype)
{
  return (memtype == 0x07) /* UC- */ || isMemTypeValidMTRR(memtype);
}
#endif

#if BX_CPU_LEVEL >= 5
bx_bool BX_CPP_AttrRegparmN(2) BX_CPU_C::wrmsr(Bit32u index, Bit64u val_64)
{
  Bit32u val32_lo = GET32L(val_64);
  Bit32u val32_hi = GET32H(val_64);

  BX_INSTR_WRMSR(BX_CPU_ID, index, val_64);

  switch(index) {

#if BX_SUPPORT_SEP
    case BX_MSR_SYSENTER_CS:
      BX_CPU_THIS_PTR msr.sysenter_cs_msr = val32_lo;
      break;

    case BX_MSR_SYSENTER_ESP:
#if BX_SUPPORT_X86_64
      if (! IsCanonical(val_64)) {
        BX_ERROR(("WRMSR: attempt to write non-canonical value to MSR_SYSENTER_ESP !"));
        return 0;
      }
#endif
      BX_CPU_THIS_PTR msr.sysenter_esp_msr = val_64;
      break;

    case BX_MSR_SYSENTER_EIP:
#if BX_SUPPORT_X86_64
      if (! IsCanonical(val_64)) {
        BX_ERROR(("WRMSR: attempt to write non-canonical value to MSR_SYSENTER_EIP !"));
        return 0;
      }
#endif
      BX_CPU_THIS_PTR msr.sysenter_eip_msr = val_64;
      break;
#endif

#if BX_CPU_LEVEL >= 6
    case BX_MSR_MTRRCAP:
      BX_ERROR(("WRMSR: MTRRCAP is read only MSR"));
      return 0;

    case BX_MSR_MTRRPHYSBASE0:
    case BX_MSR_MTRRPHYSBASE1:
    case BX_MSR_MTRRPHYSBASE2:
    case BX_MSR_MTRRPHYSBASE3:
    case BX_MSR_MTRRPHYSBASE4:
    case BX_MSR_MTRRPHYSBASE5:
    case BX_MSR_MTRRPHYSBASE6:
    case BX_MSR_MTRRPHYSBASE7:
      if (! IsValidPhyAddr(val_64)) {
        BX_ERROR(("WRMSR[0x%08x]: attempt to write invalid phy addr to variable range MTRR %08x:%08x", index, val32_hi, val32_lo));
        return 0;
      }
      // handle 8-11 reserved bits
      if (! isMemTypeValidMTRR(val32_lo & 0xFFF)) {
        BX_ERROR(("WRMSR: attempt to write invalid Memory Type to BX_MSR_MTRRPHYSBASE"));
        return 0;
      }
      BX_CPU_THIS_PTR msr.mtrrphys[index - BX_MSR_MTRRPHYSBASE0] = val_64;
      break;
    case BX_MSR_MTRRPHYSMASK0:
    case BX_MSR_MTRRPHYSMASK1:
    case BX_MSR_MTRRPHYSMASK2:
    case BX_MSR_MTRRPHYSMASK3:
    case BX_MSR_MTRRPHYSMASK4:
    case BX_MSR_MTRRPHYSMASK5:
    case BX_MSR_MTRRPHYSMASK6:
    case BX_MSR_MTRRPHYSMASK7:
      if (! IsValidPhyAddr(val_64)) {
        BX_ERROR(("WRMSR[0x%08x]: attempt to write invalid phy addr to variable range MTRR %08x:%08x", index, val32_hi, val32_lo));
        return 0;
      }
      // handle 10-0 reserved bits
      if (val32_lo & 0x7ff) {
        BX_ERROR(("WRMSR[0x%08x]: variable range MTRR reserved bits violation %08x:%08x", index, val32_hi, val32_lo));
        return 0;
      }
      BX_CPU_THIS_PTR msr.mtrrphys[index - BX_MSR_MTRRPHYSBASE0] = val_64;
      break;

    case BX_MSR_MTRRFIX64K_00000:
      if (! isMemTypeValidMTRR(val32_lo & 0xFF) ||
          ! isMemTypeValidMTRR((val32_lo >>  8) & 0xFF) || 
          ! isMemTypeValidMTRR((val32_lo >> 16) & 0xFF) || 
          ! isMemTypeValidMTRR(val32_lo >> 24) ||
          ! isMemTypeValidMTRR(val32_hi & 0xFF) ||
          ! isMemTypeValidMTRR((val32_hi >>  8) & 0xFF) || 
          ! isMemTypeValidMTRR((val32_hi >> 16) & 0xFF) || 
          ! isMemTypeValidMTRR(val32_hi >> 24))
      {
        BX_ERROR(("WRMSR: attempt to write invalid Memory Type to MSR_MTRRFIX64K_00000 !"));
        return 0;
      }
      BX_CPU_THIS_PTR msr.mtrrfix64k_00000 = val_64;
      break;
    case BX_MSR_MTRRFIX16K_80000:
    case BX_MSR_MTRRFIX16K_A0000:
      if (! isMemTypeValidMTRR(val32_lo & 0xFF) ||
          ! isMemTypeValidMTRR((val32_lo >>  8) & 0xFF) || 
          ! isMemTypeValidMTRR((val32_lo >> 16) & 0xFF) || 
          ! isMemTypeValidMTRR(val32_lo >> 24) ||
          ! isMemTypeValidMTRR(val32_hi & 0xFF) ||
          ! isMemTypeValidMTRR((val32_hi >>  8) & 0xFF) || 
          ! isMemTypeValidMTRR((val32_hi >> 16) & 0xFF) || 
          ! isMemTypeValidMTRR(val32_hi >> 24))
      {
        BX_ERROR(("WRMSR: attempt to write invalid Memory Type to MSR_MTRRFIX16K regsiter !"));
        return 0;
      }
      BX_CPU_THIS_PTR msr.mtrrfix16k[index - BX_MSR_MTRRFIX16K_80000] = val_64;
      break;

    case BX_MSR_MTRRFIX4K_C0000:
    case BX_MSR_MTRRFIX4K_C8000:
    case BX_MSR_MTRRFIX4K_D0000:
    case BX_MSR_MTRRFIX4K_D8000:
    case BX_MSR_MTRRFIX4K_E0000:
    case BX_MSR_MTRRFIX4K_E8000:
    case BX_MSR_MTRRFIX4K_F0000:
    case BX_MSR_MTRRFIX4K_F8000:
      if (! isMemTypeValidMTRR(val32_lo & 0xFF) ||
          ! isMemTypeValidMTRR((val32_lo >>  8) & 0xFF) || 
          ! isMemTypeValidMTRR((val32_lo >> 16) & 0xFF) || 
          ! isMemTypeValidMTRR(val32_lo >> 24) ||
          ! isMemTypeValidMTRR(val32_hi & 0xFF) ||
          ! isMemTypeValidMTRR((val32_hi >>  8) & 0xFF) || 
          ! isMemTypeValidMTRR((val32_hi >> 16) & 0xFF) || 
          ! isMemTypeValidMTRR(val32_hi >> 24))
      {
        BX_ERROR(("WRMSR: attempt to write invalid Memory Type to fixed memory range MTRR !"));
        return 0;
      }
      BX_CPU_THIS_PTR msr.mtrrfix4k[index - BX_MSR_MTRRFIX4K_C0000] = val_64;
      break;

    case BX_MSR_PAT:
      if (! isMemTypeValidPAT(val32_lo & 0xFF) ||
          ! isMemTypeValidPAT((val32_lo >>  8) & 0xFF) || 
          ! isMemTypeValidPAT((val32_lo >> 16) & 0xFF) || 
          ! isMemTypeValidPAT(val32_lo >> 24) ||
          ! isMemTypeValidPAT(val32_hi & 0xFF) ||
          ! isMemTypeValidPAT((val32_hi >>  8) & 0xFF) || 
          ! isMemTypeValidPAT((val32_hi >> 16) & 0xFF) || 
          ! isMemTypeValidPAT(val32_hi >> 24))
      {
        BX_ERROR(("WRMSR: attempt to write invalid Memory Type to MSR_PAT"));
        return 0;
      }
      
      BX_CPU_THIS_PTR msr.pat = val_64;
      break;

    case BX_MSR_MTRR_DEFTYPE:
      if (! isMemTypeValidMTRR(val32_lo & 0xFF)) {
        BX_ERROR(("WRMSR: attempt to write invalid Memory Type to MSR_MTRR_DEFTYPE"));
        return 0;
      }
      if (val32_hi || (val32_lo & 0xfffff300)) {
        BX_ERROR(("WRMSR: attempt to reserved bits in MSR_MTRR_DEFTYPE"));
        return 0;
      }
      BX_CPU_THIS_PTR msr.mtrr_deftype = val32_lo;
      break;
#endif

    case BX_MSR_TSC:
      BX_INFO(("WRMSR: write 0x%08x%08x to MSR_TSC", val32_hi, val32_lo));
      BX_CPU_THIS_PTR set_TSC(val_64);
      break;

#if BX_SUPPORT_APIC
    case BX_MSR_APICBASE:
      return relocate_apic(val_64);
#endif

#if BX_SUPPORT_VMX
    case BX_MSR_VMX_BASIC:
    case BX_MSR_VMX_PINBASED_CTRLS:
    case BX_MSR_VMX_PROCBASED_CTRLS:
    case BX_MSR_VMX_VMEXIT_CTRLS:
    case BX_MSR_VMX_VMENTRY_CTRLS:
    case BX_MSR_VMX_MISC:
    case BX_MSR_VMX_CR0_FIXED0:
    case BX_MSR_VMX_CR0_FIXED1:
    case BX_MSR_VMX_CR4_FIXED0:
    case BX_MSR_VMX_CR4_FIXED1:
    case BX_MSR_VMX_VMCS_ENUM:
    case BX_MSR_VMX_TRUE_PINBASED_CTRLS:
    case BX_MSR_VMX_TRUE_PROCBASED_CTRLS:
    case BX_MSR_VMX_TRUE_VMEXIT_CTRLS:
    case BX_MSR_VMX_TRUE_VMENTRY_CTRLS:
      BX_ERROR(("WRMSR: VMX read only MSR"));
      return 0;
#endif

#if BX_SUPPORT_X86_64
    case BX_MSR_EFER:
      if (val_64 & ~BX_EFER_SUPPORTED_BITS) {
        BX_ERROR(("WRMSR: attempt to set reserved bits of EFER MSR !"));
        return 0;
      }

      /* #GP(0) if changing EFER.LME when cr0.pg = 1 */
      if ((BX_CPU_THIS_PTR efer.get_LME() != ((val32_lo >> 8) & 1)) &&
           BX_CPU_THIS_PTR  cr0.get_PG())
      {
        BX_ERROR(("WRMSR: attempt to change LME when CR0.PG=1"));
        return 0;
      }

      BX_CPU_THIS_PTR efer.set32((val32_lo & BX_EFER_SUPPORTED_BITS & ~BX_EFER_LMA_MASK)
              | (BX_CPU_THIS_PTR efer.get32() & BX_EFER_LMA_MASK)); // keep LMA untouched
      break;

    case BX_MSR_STAR:
      MSR_STAR = val_64;
      break;

    case BX_MSR_LSTAR:
      if (! IsCanonical(val_64)) {
        BX_ERROR(("WRMSR: attempt to write non-canonical value to MSR_LSTAR !"));
        return 0;
      }
      MSR_LSTAR = val_64;
      break;

    case BX_MSR_CSTAR:
      if (! IsCanonical(val_64)) {
        BX_ERROR(("WRMSR: attempt to write non-canonical value to MSR_CSTAR !"));
        return 0;
      }
      MSR_CSTAR = val_64;
      break;

    case BX_MSR_FMASK:
      MSR_FMASK = (Bit32u) val_64;
      break;

    case BX_MSR_FSBASE:
      if (! IsCanonical(val_64)) {
        BX_ERROR(("WRMSR: attempt to write non-canonical value to MSR_FSBASE !"));
        return 0;
      }
      MSR_FSBASE = val_64;
      break;

    case BX_MSR_GSBASE:
      if (! IsCanonical(val_64)) {
        BX_ERROR(("WRMSR: attempt to write non-canonical value to MSR_GSBASE !"));
        return 0;
      }
      MSR_GSBASE = val_64;
      break;

    case BX_MSR_KERNELGSBASE:
      if (! IsCanonical(val_64)) {
        BX_ERROR(("WRMSR: attempt to write non-canonical value to MSR_KERNELGSBASE !"));
        return 0;
      }
      MSR_KERNELGSBASE = val_64;
      break;

    case BX_MSR_TSC_AUX:
      MSR_TSC_AUX = val32_lo;
      break;
#endif  // #if BX_SUPPORT_X86_64

    default:
#if BX_CONFIGURE_MSRS
      if (index < BX_MSR_MAX_INDEX && BX_CPU_THIS_PTR msrs[index]) {
        if (! BX_CPU_THIS_PTR msrs[index]->set64(val_64)) {
          BX_ERROR(("WRMSR: Write failed to MSR %#x - #GP fault", index));
          return 0;
        }
        break;
      }
#endif
      // failed to find the MSR, could #GP or ignore it silently
      BX_ERROR(("WRMSR: Unknown register %#x", index));
#if BX_IGNORE_BAD_MSR == 0
      return 0;
#endif
  }

  return 1;
}
#endif // BX_CPU_LEVEL >= 5

#if BX_SUPPORT_APIC
bx_bool BX_CPU_C::relocate_apic(Bit64u val_64)
{
  /* MSR_APICBASE
   *  0:7    Reserved
   *  8      This is set if CPU is BSP
   *  9      Reserved
   *  10     X2APIC mode bit (1=enabled 0=disabled)
   *  11     APIC Global Enable bit (1=enabled 0=disabled)
   *  12:35  APIC Base Address (physical)
   *  36:63  Reserved
   */

#define BX_MSR_APICBASE_RESERVED_BITS 0x6ff

  if (BX_CPU_THIS_PTR msr.apicbase & 0x800) {
    Bit32u val32_hi = GET32H(val_64), val32_lo = GET32L(val_64);
    BX_INFO(("WRMSR: wrote %08x:%08x to MSR_APICBASE", val32_hi, val32_lo));
#if BX_SUPPORT_X86_64
    if (! IsValidPhyAddr(val_64)) {
      BX_ERROR(("relocate_apic: invalid physical address"));
      return 0;
    }
#endif
    if (val32_lo & BX_MSR_APICBASE_RESERVED_BITS) {
      BX_ERROR(("relocate_apic: attempt to set reserved bits"));
      return 0;
    }
    BX_CPU_THIS_PTR msr.apicbase = (bx_phy_address) val_64;
    BX_CPU_THIS_PTR lapic.set_base(BX_CPU_THIS_PTR msr.apicbase);
    // TLB flush is required for emulation correctness
    TLB_flush();  // don't care about performance of apic relocation
  }
  else {
    BX_INFO(("WRMSR: MSR_APICBASE APIC global enable bit cleared !"));
  }

  return 1;
}
#endif

void BX_CPP_AttrRegparmN(1) BX_CPU_C::WRMSR(bxInstruction_c *i)
{
#if BX_CPU_LEVEL >= 5
  if (!real_mode() && CPL != 0) {
    BX_ERROR(("WRMSR: CPL != 0 not in real mode"));
    exception(BX_GP_EXCEPTION, 0, 0);
  }

  Bit64u val_64 = ((Bit64u) EDX << 32) | EAX;
  Bit32u index = ECX;

#if BX_SUPPORT_VMX
  VMexit_MSR(i, VMX_VMEXIT_WRMSR, index);
#endif

  if (! wrmsr(index, val_64))
    exception(BX_GP_EXCEPTION, 0, 0);
#else
  BX_INFO(("WRMSR: Pentium CPU required, use --enable-cpu-level=5"));
  exception(BX_UD_EXCEPTION, 0, 0);
#endif
}

#if BX_CONFIGURE_MSRS

int BX_CPU_C::load_MSRs(const char *file)
{
  char line[512];
  unsigned linenum = 0;
  Bit32u index, type;
  Bit32u reset_hi, reset_lo;
  Bit32u rsrv_hi, rsrv_lo;
  Bit32u ignr_hi, ignr_lo;

  FILE *fd = fopen (file, "r");
  if (fd == NULL) return -1;
  int retval = 0;
  do {
    linenum++;
    char* ret = fgets(line, sizeof(line)-1, fd);
    line[sizeof(line) - 1] = '\0';
    size_t len = strlen(line);
    if (len>0 && line[len-1] < ' ')
      line[len-1] = '\0';

    if (ret != NULL && strlen(line)) {
      if (line[0] == '#') continue;
      retval = sscanf(line, "%x %d %08x %08x %08x %08x %08x %08x",
         &index, &type, &reset_hi, &reset_lo, &rsrv_hi, &rsrv_lo, &ignr_hi, &ignr_lo);

      if (retval < 8) {
        retval = -1;
        BX_PANIC(("%s:%d > error parsing MSRs config file!", file, linenum));
        break;  // quit parsing after first error
      }
      if (index >= BX_MSR_MAX_INDEX) {
        BX_PANIC(("%s:%d > MSR index is too big !", file, linenum));
        continue;
      }
      if (BX_CPU_THIS_PTR msrs[index]) {
        BX_PANIC(("%s:%d > MSR[0x%03x] is already defined!", file, linenum, index));
        continue;
      }
      if (type > 2) {
        BX_PANIC(("%s:%d > MSR[0x%03x] unknown type !", file, linenum, index));
        continue;
      }

      BX_INFO(("loaded MSR[0x%03x] type=%d %08x:%08x %08x:%08x %08x:%08x", index, type,
        reset_hi, reset_lo, rsrv_hi, rsrv_lo, ignr_hi, ignr_lo));

      BX_CPU_THIS_PTR msrs[index] = new MSR(index, type,
        ((Bit64u)(reset_hi) << 32) | reset_lo,
        ((Bit64u) (rsrv_hi) << 32) | rsrv_lo,
        ((Bit64u) (ignr_hi) << 32) | ignr_lo);
    }
  } while (!feof(fd));

  fclose(fd);
  return retval;
}

#endif
