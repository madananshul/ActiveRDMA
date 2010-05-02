/////////////////////////////////////////////////////////////////////////
// $Id: init.cc,v 1.219 2009/10/30 09:13:19 sshwarts Exp $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2001  MandrakeSoft S.A.
//
//    MandrakeSoft S.A.
//    43, rue d'Aboukir
//    75002 Paris - France
//    http://www.linux-mandrake.com/
//    http://www.mandrakesoft.com/
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
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
/////////////////////////////////////////////////////////////////////////

#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_X86_64==0
// Make life easier merging cpu64 & cpu code.
#define RIP EIP
#endif

BX_CPU_C::BX_CPU_C(unsigned id): bx_cpuid(id)
#if BX_SUPPORT_APIC
   ,lapic (this, id)
#endif
{
  // in case of SMF, you cannot reference any member data
  // in the constructor because the only access to it is via
  // global variables which aren't initialized quite yet.
  char buffer[16];
  sprintf(buffer, "CPU%x", bx_cpuid);
  put(buffer);
}

#if BX_WITH_WX

#define IF_SEG_REG_GET(x) \
  if (!strcmp(param->get_name(), #x)) { \
    return BX_CPU(cpu)->sregs[BX_SEG_REG_##x].selector.value; \
  }
#define IF_SEG_REG_SET(reg, val) \
  if (!strcmp(param->get_name(), #reg)) { \
    BX_CPU(cpu)->load_seg_reg(&BX_CPU(cpu)->sregs[BX_SEG_REG_##reg],val); \
  }
#define IF_LAZY_EFLAG_GET(flag) \
    if (!strcmp(param->get_name(), #flag)) { \
      return BX_CPU(cpu)->get_##flag(); \
    }
#define IF_LAZY_EFLAG_SET(flag, val) \
    if (!strcmp(param->get_name(), #flag)) { \
      BX_CPU(cpu)->set_##flag(val); \
    }
#define IF_EFLAG_GET(flag) \
    if (!strcmp(param->get_name(), #flag)) { \
      return BX_CPU(cpu)->get_##flag(); \
    }
#define IF_EFLAG_SET(flag, val) \
    if (!strcmp(param->get_name(), #flag)) { \
      BX_CPU(cpu)->set_##flag(val); \
    }


// implement get/set handler for parameters that need unusual set/get
static Bit64s cpu_param_handler(bx_param_c *param, int set, Bit64s val)
{
#if BX_SUPPORT_SMP
  int cpu = atoi(param->get_parent()->get_name());
#endif
  if (set) {
    if (!strcmp(param->get_name(), "LDTR")) {
      BX_CPU(cpu)->panic("setting LDTR not implemented");
    }
    if (!strcmp(param->get_name(), "TR")) {
      BX_CPU(cpu)->panic("setting LDTR not implemented");
    }
    IF_SEG_REG_SET(CS, val);
    IF_SEG_REG_SET(DS, val);
    IF_SEG_REG_SET(SS, val);
    IF_SEG_REG_SET(ES, val);
    IF_SEG_REG_SET(FS, val);
    IF_SEG_REG_SET(GS, val);
    IF_LAZY_EFLAG_SET(OF, val);
    IF_LAZY_EFLAG_SET(SF, val);
    IF_LAZY_EFLAG_SET(ZF, val);
    IF_LAZY_EFLAG_SET(AF, val);
    IF_LAZY_EFLAG_SET(PF, val);
    IF_LAZY_EFLAG_SET(CF, val);
    IF_EFLAG_SET(ID,   val);
    IF_EFLAG_SET(VIP,  val);
    IF_EFLAG_SET(VIF,  val);
    IF_EFLAG_SET(AC,   val);
    IF_EFLAG_SET(VM,   val);
    IF_EFLAG_SET(RF,   val);
    IF_EFLAG_SET(NT,   val);
    IF_EFLAG_SET(IOPL, val);
    IF_EFLAG_SET(DF,   val);
    IF_EFLAG_SET(IF,   val);
    IF_EFLAG_SET(TF,   val);
  } else {
    if (!strcmp(param->get_name(), "LDTR")) {
      return BX_CPU(cpu)->ldtr.selector.value;
    }
    if (!strcmp(param->get_name(), "TR")) {
      return BX_CPU(cpu)->tr.selector.value;
    }
    IF_SEG_REG_GET (CS);
    IF_SEG_REG_GET (DS);
    IF_SEG_REG_GET (SS);
    IF_SEG_REG_GET (ES);
    IF_SEG_REG_GET (FS);
    IF_SEG_REG_GET (GS);
    IF_LAZY_EFLAG_GET(OF);
    IF_LAZY_EFLAG_GET(SF);
    IF_LAZY_EFLAG_GET(ZF);
    IF_LAZY_EFLAG_GET(AF);
    IF_LAZY_EFLAG_GET(PF);
    IF_LAZY_EFLAG_GET(CF);
    IF_EFLAG_GET(ID);
    IF_EFLAG_GET(VIP);
    IF_EFLAG_GET(VIF);
    IF_EFLAG_GET(AC);
    IF_EFLAG_GET(VM);
    IF_EFLAG_GET(RF);
    IF_EFLAG_GET(NT);
    IF_EFLAG_GET(IOPL);
    IF_EFLAG_GET(DF);
    IF_EFLAG_GET(IF);
    IF_EFLAG_GET(TF);
  }
  return val;
}
#undef IF_SEG_REG_GET
#undef IF_SEG_REG_SET

#endif

// BX_CPU_C constructor
void BX_CPU_C::initialize(void)
{
  BX_CPU_THIS_PTR set_INTR (0);

#if BX_CONFIGURE_MSRS
  for (unsigned n=0; n < BX_MSR_MAX_INDEX; n++) {
    BX_CPU_THIS_PTR msrs[n] = 0;
  }
  const char *msrs_filename = SIM->get_param_string(BXPN_CONFIGURABLE_MSRS_PATH)->getptr();
  load_MSRs(msrs_filename);
#endif

  init_SMRAM();

#if BX_SUPPORT_VMX
  init_VMCS();
#endif

#if BX_WITH_WX
  register_wx_state();
#endif
}

#if BX_WITH_WX
void BX_CPU_C::register_wx_state(void)
{
  if (SIM->get_param(BXPN_WX_CPU_STATE) != NULL) {
    // Register some of the CPUs variables as shadow parameters so that
    // they can be visible in the config interface.
    // (Experimental, obviously not a complete list)
    bx_param_num_c *param;
    char cpu_name[10], cpu_title[10], cpu_pname[16];
    const char *fmt16 = "%04X";
    const char *fmt32 = "%08X";
    Bit32u oldbase = bx_param_num_c::set_default_base(16);
    const char *oldfmt = bx_param_num_c::set_default_format(fmt32);
    sprintf(cpu_name, "%d", BX_CPU_ID);
    sprintf(cpu_title, "CPU %d", BX_CPU_ID);
    sprintf(cpu_pname, "%s.%d", BXPN_WX_CPU_STATE, BX_CPU_ID);
    if (SIM->get_param(cpu_pname) == NULL) {
      bx_list_c *list = new bx_list_c(SIM->get_param(BXPN_WX_CPU_STATE),
         cpu_name, cpu_title, 60);

#define DEFPARAM_NORMAL(name,field) \
    new bx_shadow_num_c(list, #name, &(field))

      DEFPARAM_NORMAL(EAX, EAX);
      DEFPARAM_NORMAL(EBX, EBX);
      DEFPARAM_NORMAL(ECX, ECX);
      DEFPARAM_NORMAL(EDX, EDX);
      DEFPARAM_NORMAL(ESP, ESP);
      DEFPARAM_NORMAL(EBP, EBP);
      DEFPARAM_NORMAL(ESI, ESI);
      DEFPARAM_NORMAL(EDI, EDI);
      DEFPARAM_NORMAL(EIP, EIP);
      DEFPARAM_NORMAL(DR0, dr[0]);
      DEFPARAM_NORMAL(DR1, dr[1]);
      DEFPARAM_NORMAL(DR2, dr[2]);
      DEFPARAM_NORMAL(DR3, dr[3]);
      DEFPARAM_NORMAL(DR6, dr6);
      DEFPARAM_NORMAL(DR7, dr7);
      DEFPARAM_NORMAL(CR0, cr0.val32);
      DEFPARAM_NORMAL(CR2, cr2);
      DEFPARAM_NORMAL(CR3, cr3);
#if BX_CPU_LEVEL >= 4
      DEFPARAM_NORMAL(CR4, cr4.val32);
#endif

      // segment registers require a handler function because they have
      // special get/set requirements.
#define DEFPARAM_SEG_REG(x) \
    param = new bx_param_num_c(list, \
      #x, #x, "", 0, 0xffff, 0); \
    param->set_handler(cpu_param_handler); \
    param->set_format(fmt16);
#define DEFPARAM_GLOBAL_SEG_REG(name,field) \
    param = new bx_shadow_num_c(list, \
        #name"_base", &(field.base)); \
    param = new bx_shadow_num_c(list, \
        #name"_limit", &(field.limit));

      DEFPARAM_SEG_REG(CS);
      DEFPARAM_SEG_REG(DS);
      DEFPARAM_SEG_REG(SS);
      DEFPARAM_SEG_REG(ES);
      DEFPARAM_SEG_REG(FS);
      DEFPARAM_SEG_REG(GS);
      DEFPARAM_SEG_REG(LDTR);
      DEFPARAM_SEG_REG(TR);
      DEFPARAM_GLOBAL_SEG_REG(GDTR, BX_CPU_THIS_PTR gdtr);
      DEFPARAM_GLOBAL_SEG_REG(IDTR, BX_CPU_THIS_PTR idtr);
#undef DEFPARAM_NORMAL
#undef DEFPARAM_SEG_REG
#undef DEFPARAM_GLOBAL_SEG_REG

      param = new bx_shadow_num_c(list, "EFLAGS",
          &BX_CPU_THIS_PTR eflags);

      // flags implemented in lazy_flags.cc must be done with a handler
      // that calls their get function, to force them to be computed.
#define DEFPARAM_EFLAG(name) \
    param = new bx_param_bool_c(list, \
            #name, #name, "", get_##name()); \
    param->set_handler(cpu_param_handler);
#define DEFPARAM_LAZY_EFLAG(name) \
    param = new bx_param_bool_c(list, \
            #name, #name, "", get_##name()); \
    param->set_handler(cpu_param_handler);

#if BX_CPU_LEVEL >= 4
      DEFPARAM_EFLAG(ID);
      DEFPARAM_EFLAG(VIP);
      DEFPARAM_EFLAG(VIF);
      DEFPARAM_EFLAG(AC);
#endif
#if BX_CPU_LEVEL >= 3
      DEFPARAM_EFLAG(VM);
      DEFPARAM_EFLAG(RF);
#endif
#if BX_CPU_LEVEL >= 2
      DEFPARAM_EFLAG(NT);
      // IOPL is a special case because it is 2 bits wide.
      param = new bx_shadow_num_c(
              list,
              "IOPL",
              &BX_CPU_THIS_PTR eflags, 10,
              12, 13);
      param->set_range(0, 3);
      param->set_format("%d");
#endif
      DEFPARAM_LAZY_EFLAG(OF);
      DEFPARAM_EFLAG(DF);
      DEFPARAM_EFLAG(IF);
      DEFPARAM_EFLAG(TF);
      DEFPARAM_LAZY_EFLAG(SF);
      DEFPARAM_LAZY_EFLAG(ZF);
      DEFPARAM_LAZY_EFLAG(AF);
      DEFPARAM_LAZY_EFLAG(PF);
      DEFPARAM_LAZY_EFLAG(CF);

      // restore defaults
      bx_param_num_c::set_default_base(oldbase);
      bx_param_num_c::set_default_format(oldfmt);
    }
  }
}
#endif

// save/restore functionality
void BX_CPU_C::register_state(void)
{
  unsigned n;
  char name[10];

  sprintf(name, "cpu%d", BX_CPU_ID);

  bx_list_c *cpu = new bx_list_c(SIM->get_bochs_root(), name, name, 50 + BX_GENERAL_REGISTERS);

  BXRS_PARAM_SPECIAL32(cpu, cpu_version, param_save_handler, param_restore_handler);
  BXRS_PARAM_SPECIAL32(cpu, cpuid_std,   param_save_handler, param_restore_handler);
  BXRS_PARAM_SPECIAL32(cpu, cpuid_ext,   param_save_handler, param_restore_handler);
  BXRS_DEC_PARAM_SIMPLE(cpu, cpu_mode);
  BXRS_HEX_PARAM_SIMPLE(cpu, activity_state);
  BXRS_HEX_PARAM_SIMPLE(cpu, inhibit_mask);
  BXRS_HEX_PARAM_SIMPLE(cpu, debug_trap);
#if BX_SUPPORT_X86_64
  BXRS_HEX_PARAM_SIMPLE(cpu, RAX);
  BXRS_HEX_PARAM_SIMPLE(cpu, RBX);
  BXRS_HEX_PARAM_SIMPLE(cpu, RCX);
  BXRS_HEX_PARAM_SIMPLE(cpu, RDX);
  BXRS_HEX_PARAM_SIMPLE(cpu, RSP);
  BXRS_HEX_PARAM_SIMPLE(cpu, RBP);
  BXRS_HEX_PARAM_SIMPLE(cpu, RSI);
  BXRS_HEX_PARAM_SIMPLE(cpu, RDI);
  BXRS_HEX_PARAM_SIMPLE(cpu, R8);
  BXRS_HEX_PARAM_SIMPLE(cpu, R9);
  BXRS_HEX_PARAM_SIMPLE(cpu, R10);
  BXRS_HEX_PARAM_SIMPLE(cpu, R11);
  BXRS_HEX_PARAM_SIMPLE(cpu, R12);
  BXRS_HEX_PARAM_SIMPLE(cpu, R13);
  BXRS_HEX_PARAM_SIMPLE(cpu, R14);
  BXRS_HEX_PARAM_SIMPLE(cpu, R15);
  BXRS_HEX_PARAM_SIMPLE(cpu, RIP);
#else
  BXRS_HEX_PARAM_SIMPLE(cpu, EAX);
  BXRS_HEX_PARAM_SIMPLE(cpu, EBX);
  BXRS_HEX_PARAM_SIMPLE(cpu, ECX);
  BXRS_HEX_PARAM_SIMPLE(cpu, EDX);
  BXRS_HEX_PARAM_SIMPLE(cpu, ESP);
  BXRS_HEX_PARAM_SIMPLE(cpu, EBP);
  BXRS_HEX_PARAM_SIMPLE(cpu, ESI);
  BXRS_HEX_PARAM_SIMPLE(cpu, EDI);
  BXRS_HEX_PARAM_SIMPLE(cpu, EIP);
#endif
  BXRS_PARAM_SPECIAL32(cpu, EFLAGS,
         param_save_handler, param_restore_handler);
#if BX_CPU_LEVEL >= 3
  BXRS_HEX_PARAM_FIELD(cpu, DR0, dr[0]);
  BXRS_HEX_PARAM_FIELD(cpu, DR1, dr[1]);
  BXRS_HEX_PARAM_FIELD(cpu, DR2, dr[2]);
  BXRS_HEX_PARAM_FIELD(cpu, DR3, dr[3]);
  BXRS_HEX_PARAM_FIELD(cpu, DR6, dr6);
  BXRS_HEX_PARAM_FIELD(cpu, DR7, dr7);
#endif
  BXRS_HEX_PARAM_FIELD(cpu, CR0, cr0.val32);
  BXRS_HEX_PARAM_FIELD(cpu, CR2, cr2);
  BXRS_HEX_PARAM_FIELD(cpu, CR3, cr3);
#if BX_CPU_LEVEL >= 4
  BXRS_HEX_PARAM_FIELD(cpu, CR4, cr4.val32);
#endif
#if BX_SUPPORT_XSAVE
  BXRS_HEX_PARAM_FIELD(cpu, XCR0, xcr0.val32);
#endif

  for(n=0; n<6; n++) {
    bx_segment_reg_t *segment = &BX_CPU_THIS_PTR sregs[n];
    bx_list_c *sreg = new bx_list_c(cpu, strseg(segment), 9);
    BXRS_PARAM_SPECIAL16(sreg, selector,
           param_save_handler, param_restore_handler);
    BXRS_HEX_PARAM_FIELD(sreg, base, segment->cache.u.segment.base);
    BXRS_HEX_PARAM_FIELD(sreg, limit_scaled, segment->cache.u.segment.limit_scaled);
    BXRS_PARAM_SPECIAL8 (sreg, ar_byte,
           param_save_handler, param_restore_handler);
    BXRS_PARAM_BOOL(sreg, granularity, segment->cache.u.segment.g);
    BXRS_PARAM_BOOL(sreg, d_b, segment->cache.u.segment.d_b);
#if BX_SUPPORT_X86_64
    BXRS_PARAM_BOOL(sreg, l, segment->cache.u.segment.l);
#endif
    BXRS_PARAM_BOOL(sreg, avl, segment->cache.u.segment.avl);
  }

  bx_list_c *GDTR = new bx_list_c(cpu, "GDTR", 2);
  BXRS_HEX_PARAM_FIELD(GDTR, base, gdtr.base);
  BXRS_HEX_PARAM_FIELD(GDTR, limit, gdtr.limit);

  bx_list_c *IDTR = new bx_list_c(cpu, "IDTR", 2);
  BXRS_HEX_PARAM_FIELD(IDTR, base, idtr.base);
  BXRS_HEX_PARAM_FIELD(IDTR, limit, idtr.limit);

  bx_list_c *LDTR = new bx_list_c(cpu, "LDTR", 8);
  BXRS_PARAM_SPECIAL16(LDTR, selector, param_save_handler, param_restore_handler);
  BXRS_HEX_PARAM_FIELD(LDTR, base,  ldtr.cache.u.segment.base);
  BXRS_HEX_PARAM_FIELD(LDTR, limit_scaled, ldtr.cache.u.segment.limit_scaled);
  BXRS_PARAM_SPECIAL8 (LDTR, ar_byte, param_save_handler, param_restore_handler);
  BXRS_PARAM_BOOL(LDTR, granularity, ldtr.cache.u.segment.g);
  BXRS_PARAM_BOOL(LDTR, d_b, ldtr.cache.u.segment.d_b);
  BXRS_PARAM_BOOL(LDTR, avl, ldtr.cache.u.segment.avl);

  bx_list_c *TR = new bx_list_c(cpu, "TR", 8);
  BXRS_PARAM_SPECIAL16(TR, selector, param_save_handler, param_restore_handler);
  BXRS_HEX_PARAM_FIELD(TR, base,  tr.cache.u.segment.base);
  BXRS_HEX_PARAM_FIELD(TR, limit_scaled, tr.cache.u.segment.limit_scaled);
  BXRS_PARAM_SPECIAL8 (TR, ar_byte, param_save_handler, param_restore_handler);
  BXRS_PARAM_BOOL(TR, granularity, tr.cache.u.segment.g);
  BXRS_PARAM_BOOL(TR, d_b, tr.cache.u.segment.d_b);
  BXRS_PARAM_BOOL(TR, avl, tr.cache.u.segment.avl);

  BXRS_HEX_PARAM_SIMPLE(cpu, smbase);

#if BX_CPU_LEVEL >= 5
  bx_list_c *MSR = new bx_list_c(cpu, "MSR", 45);

#if BX_SUPPORT_APIC
  BXRS_HEX_PARAM_FIELD(MSR, apicbase, msr.apicbase);
#endif
#if BX_SUPPORT_X86_64
  BXRS_HEX_PARAM_FIELD(MSR, EFER, efer.val32);
  BXRS_HEX_PARAM_FIELD(MSR,  star, msr.star);
  BXRS_HEX_PARAM_FIELD(MSR, lstar, msr.lstar);
  BXRS_HEX_PARAM_FIELD(MSR, cstar, msr.cstar);
  BXRS_HEX_PARAM_FIELD(MSR, fmask, msr.fmask);
  BXRS_HEX_PARAM_FIELD(MSR, kernelgsbase, msr.kernelgsbase);
  BXRS_HEX_PARAM_FIELD(MSR, tsc_aux, msr.tsc_aux);
#endif
  BXRS_HEX_PARAM_FIELD(MSR, tsc_last_reset, msr.tsc_last_reset);
#if BX_SUPPORT_SEP
  BXRS_HEX_PARAM_FIELD(MSR, sysenter_cs_msr,  msr.sysenter_cs_msr);
  BXRS_HEX_PARAM_FIELD(MSR, sysenter_esp_msr, msr.sysenter_esp_msr);
  BXRS_HEX_PARAM_FIELD(MSR, sysenter_eip_msr, msr.sysenter_eip_msr);
#endif
#if BX_CPU_LEVEL >= 6
  BXRS_HEX_PARAM_FIELD(MSR, mtrrphysbase0, msr.mtrrphys[0]);
  BXRS_HEX_PARAM_FIELD(MSR, mtrrphysmask0, msr.mtrrphys[1]);
  BXRS_HEX_PARAM_FIELD(MSR, mtrrphysbase1, msr.mtrrphys[2]);
  BXRS_HEX_PARAM_FIELD(MSR, mtrrphysmask1, msr.mtrrphys[3]);
  BXRS_HEX_PARAM_FIELD(MSR, mtrrphysbase2, msr.mtrrphys[4]);
  BXRS_HEX_PARAM_FIELD(MSR, mtrrphysmask2, msr.mtrrphys[5]);
  BXRS_HEX_PARAM_FIELD(MSR, mtrrphysbase3, msr.mtrrphys[6]);
  BXRS_HEX_PARAM_FIELD(MSR, mtrrphysmask3, msr.mtrrphys[7]);
  BXRS_HEX_PARAM_FIELD(MSR, mtrrphysbase4, msr.mtrrphys[8]);
  BXRS_HEX_PARAM_FIELD(MSR, mtrrphysmask4, msr.mtrrphys[9]);
  BXRS_HEX_PARAM_FIELD(MSR, mtrrphysbase5, msr.mtrrphys[10]);
  BXRS_HEX_PARAM_FIELD(MSR, mtrrphysmask5, msr.mtrrphys[11]);
  BXRS_HEX_PARAM_FIELD(MSR, mtrrphysbase6, msr.mtrrphys[12]);
  BXRS_HEX_PARAM_FIELD(MSR, mtrrphysmask6, msr.mtrrphys[13]);
  BXRS_HEX_PARAM_FIELD(MSR, mtrrphysbase7, msr.mtrrphys[14]);
  BXRS_HEX_PARAM_FIELD(MSR, mtrrphysmask7, msr.mtrrphys[15]);

  BXRS_HEX_PARAM_FIELD(MSR, mtrrfix64k_00000, msr.mtrrfix64k_00000);
  BXRS_HEX_PARAM_FIELD(MSR, mtrrfix16k_80000, msr.mtrrfix16k[0]);
  BXRS_HEX_PARAM_FIELD(MSR, mtrrfix16k_a0000, msr.mtrrfix16k[1]);

  BXRS_HEX_PARAM_FIELD(MSR, mtrrfix4k_c0000, msr.mtrrfix4k[0]);
  BXRS_HEX_PARAM_FIELD(MSR, mtrrfix4k_c8000, msr.mtrrfix4k[1]);
  BXRS_HEX_PARAM_FIELD(MSR, mtrrfix4k_d0000, msr.mtrrfix4k[2]);
  BXRS_HEX_PARAM_FIELD(MSR, mtrrfix4k_d8000, msr.mtrrfix4k[3]);
  BXRS_HEX_PARAM_FIELD(MSR, mtrrfix4k_e0000, msr.mtrrfix4k[4]);
  BXRS_HEX_PARAM_FIELD(MSR, mtrrfix4k_e8000, msr.mtrrfix4k[5]);
  BXRS_HEX_PARAM_FIELD(MSR, mtrrfix4k_f0000, msr.mtrrfix4k[6]);
  BXRS_HEX_PARAM_FIELD(MSR, mtrrfix4k_f8000, msr.mtrrfix4k[7]);

  BXRS_HEX_PARAM_FIELD(MSR, pat, msr.pat);
  BXRS_HEX_PARAM_FIELD(MSR, mtrr_deftype, msr.mtrr_deftype);
#endif
#if BX_CONFIGURE_MSRS
  bx_list_c *MSRS = new bx_list_c(cpu, "MSRS", BX_MSR_MAX_INDEX);
  for(n=0; n < BX_MSR_MAX_INDEX; n++) {
    if (! msrs[n]) continue;
    sprintf(name, "msr_0x%03x", n);
    bx_list_c *m = new bx_list_c(MSRS, name, 6);
    BXRS_HEX_PARAM_FIELD(m, index, msrs[n]->index);
    BXRS_DEC_PARAM_FIELD(m, type, msrs[n]->type);
    BXRS_HEX_PARAM_FIELD(m, val64, msrs[n]->val64);
    BXRS_HEX_PARAM_FIELD(m, reset, msrs[n]->reset_value);
    BXRS_HEX_PARAM_FIELD(m, reserved, msrs[n]->reserved);
    BXRS_HEX_PARAM_FIELD(m, ignored, msrs[n]->ignored);
  }
#endif
#endif

#if BX_SUPPORT_FPU || BX_SUPPORT_MMX
  bx_list_c *fpu = new bx_list_c(cpu, "FPU", 17);
  BXRS_HEX_PARAM_FIELD(fpu, cwd, the_i387.cwd);
  BXRS_HEX_PARAM_FIELD(fpu, swd, the_i387.swd);
  BXRS_HEX_PARAM_FIELD(fpu, twd, the_i387.twd);
  BXRS_HEX_PARAM_FIELD(fpu, foo, the_i387.foo);
  BXRS_HEX_PARAM_FIELD(fpu, fcs, the_i387.fcs);
  BXRS_HEX_PARAM_FIELD(fpu, fip, the_i387.fip);
  BXRS_HEX_PARAM_FIELD(fpu, fds, the_i387.fds);
  BXRS_HEX_PARAM_FIELD(fpu, fdp, the_i387.fdp);
  for (n=0; n<8; n++) {
    sprintf(name, "st%d", n);
    bx_list_c *STx = new bx_list_c(fpu, name, 8);
    BXRS_HEX_PARAM_FIELD(STx, exp,      the_i387.st_space[n].exp);
    BXRS_HEX_PARAM_FIELD(STx, fraction, the_i387.st_space[n].fraction);
  }
  BXRS_DEC_PARAM_FIELD(fpu, tos, the_i387.tos);
#endif

#if BX_SUPPORT_SSE
  bx_list_c *sse = new bx_list_c(cpu, "SSE", 2*BX_XMM_REGISTERS+1);
  BXRS_HEX_PARAM_FIELD(sse, mxcsr, mxcsr.mxcsr);
  for (n=0; n<BX_XMM_REGISTERS; n++) {
    sprintf(name, "xmm%02d_hi", n);
    new bx_shadow_num_c(sse, name, &xmm[n].xmm64u(1), BASE_HEX);
    sprintf(name, "xmm%02d_lo", n);
    new bx_shadow_num_c(sse, name, &xmm[n].xmm64u(0), BASE_HEX);
  }
#endif

#if BX_SUPPORT_MONITOR_MWAIT
  bx_list_c *monitor_list = new bx_list_c(cpu, "MONITOR", 3);
  BXRS_HEX_PARAM_FIELD(monitor_list, begin_addr, monitor.monitor_begin);
  BXRS_HEX_PARAM_FIELD(monitor_list, end_addr,   monitor.monitor_end);
  BXRS_PARAM_BOOL(monitor_list, armed, monitor.armed);
#endif

#if BX_SUPPORT_APIC
  lapic.register_state(cpu);
#endif

#if BX_SUPPORT_VMX
  register_vmx_state(cpu);
#endif

  BXRS_HEX_PARAM_SIMPLE32(cpu, async_event);
  BXRS_PARAM_BOOL(cpu, INTR, INTR);

#if BX_X86_DEBUGGER
  BXRS_PARAM_BOOL(cpu, in_repeat, in_repeat);
#endif

  BXRS_PARAM_BOOL(cpu, in_smm, in_smm);
  BXRS_PARAM_BOOL(cpu, disable_SMI, disable_SMI);
  BXRS_PARAM_BOOL(cpu, pending_SMI, pending_SMI);
  BXRS_PARAM_BOOL(cpu, disable_NMI, disable_NMI);
  BXRS_PARAM_BOOL(cpu, pending_NMI, pending_NMI);
  BXRS_PARAM_BOOL(cpu, disable_INIT, disable_INIT);
  BXRS_PARAM_BOOL(cpu, pending_INIT, pending_INIT);
  BXRS_PARAM_BOOL(cpu, trace, trace);
}

Bit64s BX_CPU_C::param_save_handler(void *devptr, bx_param_c *param)
{
#if !BX_USE_CPU_SMF
  BX_CPU_C *class_ptr = (BX_CPU_C *) devptr;
  return class_ptr->param_save(param);
}

Bit64s BX_CPU_C::param_save(bx_param_c *param)
{
#else
  UNUSED(devptr);
#endif // !BX_USE_CPU_SMF
  const char *pname, *segname;
  bx_segment_reg_t *segment = NULL;
  Bit64s val = 0;

  pname = param->get_name();
  if (!strcmp(pname, "cpu_version")) {
    val = get_cpu_version_information();
  } else if (!strcmp(pname, "cpuid_std")) {
    val = get_std_cpuid_features();
  } else if (!strcmp(pname, "cpuid_ext")) {
    val = get_extended_cpuid_features();
  } else if (!strcmp(pname, "EFLAGS")) {
    val = BX_CPU_THIS_PTR read_eflags();
  } else if (!strcmp(pname, "ar_byte") || !strcmp(pname, "selector")) {
    segname = param->get_parent()->get_name();
    if (!strcmp(segname, "CS")) {
      segment = &BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS];
    } else if (!strcmp(segname, "DS")) {
      segment = &BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS];
    } else if (!strcmp(segname, "SS")) {
      segment = &BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS];
    } else if (!strcmp(segname, "ES")) {
      segment = &BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES];
    } else if (!strcmp(segname, "FS")) {
      segment = &BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS];
    } else if (!strcmp(segname, "GS")) {
      segment = &BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS];
    } else if (!strcmp(segname, "LDTR")) {
      segment = &BX_CPU_THIS_PTR ldtr;
    } else if (!strcmp(segname, "TR")) {
      segment = &BX_CPU_THIS_PTR tr;
    }
    if (segment != NULL) {
      if (!strcmp(pname, "ar_byte")) {
        val = get_ar_byte(&(segment->cache));
      }
      else if (!strcmp(pname, "selector")) {
        val = segment->selector.value;
      }
    }
  }
  else {
    BX_PANIC(("Unknown param %s in param_save handler !", pname));
  }
  return val;
}

void BX_CPU_C::param_restore_handler(void *devptr, bx_param_c *param, Bit64s val)
{
#if !BX_USE_CPU_SMF
  BX_CPU_C *class_ptr = (BX_CPU_C *) devptr;
  class_ptr->param_restore(param, val);
}

void BX_CPU_C::param_restore(bx_param_c *param, Bit64s val)
{
#else
  UNUSED(devptr);
#endif // !BX_USE_CPU_SMF
  const char *pname, *segname;
  bx_segment_reg_t *segment = NULL;

  pname = param->get_name();
  if (!strcmp(pname, "cpu_version")) {
    if (val != get_cpu_version_information()) {
      BX_PANIC(("save/restore: CPU version mismatch"));
    }
  } else if (!strcmp(pname, "cpuid_std")) {
    if (val != get_std_cpuid_features()) {
      BX_PANIC(("save/restore: CPUID mismatch"));
    }
  } else if (!strcmp(pname, "cpuid_ext")) {
    if (val != get_extended_cpuid_features()) {
      BX_PANIC(("save/restore: CPUID mismatch"));
    }
  } else if (!strcmp(pname, "EFLAGS")) {
    BX_CPU_THIS_PTR setEFlags((Bit32u)val);
  } else if (!strcmp(pname, "ar_byte") || !strcmp(pname, "selector")) {
    segname = param->get_parent()->get_name();
    if (!strcmp(segname, "CS")) {
      segment = &BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS];
    } else if (!strcmp(segname, "DS")) {
      segment = &BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS];
    } else if (!strcmp(segname, "SS")) {
      segment = &BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS];
    } else if (!strcmp(segname, "ES")) {
      segment = &BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES];
    } else if (!strcmp(segname, "FS")) {
      segment = &BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS];
    } else if (!strcmp(segname, "GS")) {
      segment = &BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS];
    } else if (!strcmp(segname, "LDTR")) {
      segment = &BX_CPU_THIS_PTR ldtr;
    } else if (!strcmp(segname, "TR")) {
      segment = &BX_CPU_THIS_PTR tr;
    }
    if (segment != NULL) {
      bx_descriptor_t *d = &(segment->cache);
      bx_selector_t *selector = &(segment->selector);
      if (!strcmp(pname, "ar_byte")) {
        set_ar_byte(d, (Bit8u)val);
      }
      else if (!strcmp(pname, "selector")) {
        parse_selector((Bit16u)val, selector);
        // validate the selector
        if ((selector->value & 0xfffc) != 0) d->valid = 1;
        else d->valid = 0;
      }
    }
  }
  else {
    BX_PANIC(("Unknown param %s in param_restore handler !", pname));
  }
}

void BX_CPU_C::after_restore_state(void)
{
  if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_IA32_V8086) CPL = 3;

  if (!SetCR0(cr0.val32))
    BX_PANIC(("Incorrect CR0 state !"));
  SetCR3(cr3);
  TLB_flush();
#if BX_SUPPORT_VMX
  set_VMCSPTR(BX_CPU_THIS_PTR vmcsptr);
#endif
  assert_checks();
  invalidate_prefetch_q();
  debug(RIP);
}
// end of save/restore functionality

BX_CPU_C::~BX_CPU_C()
{
  BX_INSTR_EXIT(BX_CPU_ID);
  BX_DEBUG(("Exit."));
}

void BX_CPU_C::reset(unsigned source)
{
  unsigned n;

  if (source == BX_RESET_HARDWARE)
    BX_INFO(("cpu hardware reset"));
  else if (source == BX_RESET_SOFTWARE)
    BX_INFO(("cpu software reset"));
  else
    BX_INFO(("cpu reset"));

#if BX_SUPPORT_X86_64
  RAX = 0; // processor passed test :-)
  RBX = 0;
  RCX = 0;
  RDX = get_cpu_version_information();
  RBP = 0;
  RSI = 0;
  RDI = 0;
  RSP = 0;
  R8  = 0;
  R9  = 0;
  R10 = 0;
  R11 = 0;
  R12 = 0;
  R13 = 0;
  R14 = 0;
  R15 = 0;
#else
  // general registers
  EAX = 0; // processor passed test :-)
  EBX = 0;
  ECX = 0;
  EDX = get_cpu_version_information();
  EBP = 0;
  ESI = 0;
  EDI = 0;
  ESP = 0;
#endif

  // initialize NIL register
  BX_WRITE_32BIT_REGZ(BX_NIL_REGISTER, 0);

  // status and control flags register set
  BX_CPU_THIS_PTR setEFlags(0x2); // Bit1 is always set

  BX_CPU_THIS_PTR inhibit_mask = 0;
  BX_CPU_THIS_PTR activity_state = BX_ACTIVITY_STATE_ACTIVE;
  BX_CPU_THIS_PTR debug_trap = 0;

  /* instruction pointer */
#if BX_CPU_LEVEL < 2
  BX_CPU_THIS_PTR prev_rip = EIP = 0x00000000;
#else /* from 286 up */
  BX_CPU_THIS_PTR prev_rip = RIP = 0x0000FFF0;
#endif

  /* CS (Code Segment) and descriptor cache */
  /* Note: on a real cpu, CS initially points to upper memory.  After
   * the 1st jump, the descriptor base is zero'd out.  Since I'm just
   * going to jump to my BIOS, I don't need to do this.
   * For future reference:
   *   processor  cs.selector   cs.base    cs.limit    EIP
   *        8086    FFFF          FFFF0        FFFF   0000
   *        286     F000         FF0000        FFFF   FFF0
   *        386+    F000       FFFF0000        FFFF   FFF0
   */
  parse_selector(0xf000,
          &BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector);

  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.valid    = SegValidCache | SegAccessROK | SegAccessWOK;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.p        = 1;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.dpl      = 0;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.segment  = 1;  /* data/code segment */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.type     = BX_DATA_READ_WRITE_ACCESSED;

  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.base         = 0xFFFF0000;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled = 0xFFFF;

#if BX_CPU_LEVEL >= 3
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.g   = 0; /* byte granular */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b = 0; /* 16bit default size */
#if BX_SUPPORT_X86_64
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.l   = 0; /* 16bit default size */
#endif
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.avl = 0;
#endif

  updateFetchModeMask();
  flushICaches();

  /* DS (Data Segment) and descriptor cache */
  parse_selector(0x0000,
          &BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].selector);

  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.valid    = SegValidCache | SegAccessROK | SegAccessWOK;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.p        = 1;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.dpl      = 0;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.segment  = 1; /* data/code segment */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.type     = BX_DATA_READ_WRITE_ACCESSED;

  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.base         = 0x00000000;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.limit_scaled = 0xFFFF;
#if BX_CPU_LEVEL >= 3
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.avl = 0;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.g   = 0; /* byte granular */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.d_b = 0; /* 16bit default size */
#if BX_SUPPORT_X86_64
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.l   = 0; /* 16bit default size */
#endif
#endif

  // use DS segment as template for the others
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS] = BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS];
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES] = BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS];
#if BX_CPU_LEVEL >= 3
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS] = BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS];
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS] = BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS];
#endif

  /* GDTR (Global Descriptor Table Register) */
  BX_CPU_THIS_PTR gdtr.base  = 0x00000000;
  BX_CPU_THIS_PTR gdtr.limit =     0xFFFF;

  /* IDTR (Interrupt Descriptor Table Register) */
  BX_CPU_THIS_PTR idtr.base  = 0x00000000;
  BX_CPU_THIS_PTR idtr.limit =     0xFFFF; /* always byte granular */

  /* LDTR (Local Descriptor Table Register) */
  BX_CPU_THIS_PTR ldtr.selector.value = 0x0000;
  BX_CPU_THIS_PTR ldtr.selector.index = 0x0000;
  BX_CPU_THIS_PTR ldtr.selector.ti    = 0;
  BX_CPU_THIS_PTR ldtr.selector.rpl   = 0;

  BX_CPU_THIS_PTR ldtr.cache.valid    = 1; /* valid */
  BX_CPU_THIS_PTR ldtr.cache.p        = 1; /* present */
  BX_CPU_THIS_PTR ldtr.cache.dpl      = 0; /* field not used */
  BX_CPU_THIS_PTR ldtr.cache.segment  = 0; /* system segment */
  BX_CPU_THIS_PTR ldtr.cache.type     = BX_SYS_SEGMENT_LDT;
  BX_CPU_THIS_PTR ldtr.cache.u.segment.base       = 0x00000000;
  BX_CPU_THIS_PTR ldtr.cache.u.segment.limit_scaled =   0xFFFF;
  BX_CPU_THIS_PTR ldtr.cache.u.segment.avl = 0;
  BX_CPU_THIS_PTR ldtr.cache.u.segment.g   = 0;  /* byte granular */

  /* TR (Task Register) */
  BX_CPU_THIS_PTR tr.selector.value = 0x0000;
  BX_CPU_THIS_PTR tr.selector.index = 0x0000; /* undefined */
  BX_CPU_THIS_PTR tr.selector.ti    = 0;
  BX_CPU_THIS_PTR tr.selector.rpl   = 0;

  BX_CPU_THIS_PTR tr.cache.valid    = 1; /* valid */
  BX_CPU_THIS_PTR tr.cache.p        = 1; /* present */
  BX_CPU_THIS_PTR tr.cache.dpl      = 0; /* field not used */
  BX_CPU_THIS_PTR tr.cache.segment  = 0; /* system segment */
  BX_CPU_THIS_PTR tr.cache.type     = BX_SYS_SEGMENT_BUSY_386_TSS;
  BX_CPU_THIS_PTR tr.cache.u.segment.base         = 0x00000000;
  BX_CPU_THIS_PTR tr.cache.u.segment.limit_scaled =     0xFFFF;
  BX_CPU_THIS_PTR tr.cache.u.segment.avl = 0;
  BX_CPU_THIS_PTR tr.cache.u.segment.g   = 0;  /* byte granular */

  // DR0 - DR7 (Debug Registers)
#if BX_CPU_LEVEL >= 3
  for (n=0; n<4; n++)
    BX_CPU_THIS_PTR dr[n] = 0;
#endif

  BX_CPU_THIS_PTR dr7 = 0x00000400;
#if   BX_CPU_LEVEL == 3
  BX_CPU_THIS_PTR dr6 = 0xFFFF1FF0;
#elif BX_CPU_LEVEL == 4
  BX_CPU_THIS_PTR dr6 = 0xFFFF1FF0;
#elif BX_CPU_LEVEL == 5
  BX_CPU_THIS_PTR dr6 = 0xFFFF0FF0;
#elif BX_CPU_LEVEL == 6
  BX_CPU_THIS_PTR dr6 = 0xFFFF0FF0;
#else
#  error "DR6: CPU > 6"
#endif

#if BX_X86_DEBUGGER
  BX_CPU_THIS_PTR in_repeat = 0;
#endif
  BX_CPU_THIS_PTR in_smm = 0;
  BX_CPU_THIS_PTR disable_SMI = 0;
  BX_CPU_THIS_PTR pending_SMI = 0;
  BX_CPU_THIS_PTR disable_NMI = 0;
  BX_CPU_THIS_PTR pending_NMI = 0;
  BX_CPU_THIS_PTR disable_INIT = 0;
  BX_CPU_THIS_PTR pending_INIT = 0;
#if BX_CPU_LEVEL >= 4 && BX_SUPPORT_ALIGNMENT_CHECK
  BX_CPU_THIS_PTR alignment_check_mask = 0;
#endif

  if (source == BX_RESET_HARDWARE) {
    BX_CPU_THIS_PTR smbase = 0x30000; // do not change SMBASE on INIT
  }

  BX_CPU_THIS_PTR cr0.set32(0x60000010);
  // handle reserved bits
#if BX_CPU_LEVEL == 3
  // reserved bits all set to 1 on 386
  BX_CPU_THIS_PTR cr0.val32 |= 0x7ffffff0;
#endif

#if BX_CPU_LEVEL >= 3
  BX_CPU_THIS_PTR cr2 = 0;
  BX_CPU_THIS_PTR cr3 = 0;
  BX_CPU_THIS_PTR cr3_masked = 0;
#endif

#if BX_CPU_LEVEL >= 4
  BX_CPU_THIS_PTR cr4.set32(0);
#endif

#if BX_SUPPORT_XSAVE
  BX_CPU_THIS_PTR xcr0.set32(0x1);
#endif

/* initialise MSR registers to defaults */
#if BX_CPU_LEVEL >= 5
#if BX_SUPPORT_APIC
  /* APIC Address, APIC enabled and BSP is default, we'll fill in the rest later */
  BX_CPU_THIS_PTR msr.apicbase = BX_LAPIC_BASE_ADDR;
  BX_CPU_THIS_PTR lapic.reset(source);
  BX_CPU_THIS_PTR msr.apicbase |= 0x900;
  BX_CPU_THIS_PTR lapic.set_base(BX_CPU_THIS_PTR msr.apicbase);
#endif
#if BX_SUPPORT_X86_64
  BX_CPU_THIS_PTR efer.set32(0);

  BX_CPU_THIS_PTR msr.star  = 0;
  BX_CPU_THIS_PTR msr.lstar = 0;
  BX_CPU_THIS_PTR msr.cstar = 0;
  BX_CPU_THIS_PTR msr.fmask = 0x00020200;
  BX_CPU_THIS_PTR msr.kernelgsbase = 0;
  BX_CPU_THIS_PTR msr.tsc_aux = 0;
#endif
  if (source == BX_RESET_HARDWARE) {
    BX_CPU_THIS_PTR set_TSC(0); // do not change TSC on INIT
  }
#endif

#if BX_SUPPORT_SEP
  BX_CPU_THIS_PTR msr.sysenter_cs_msr  = 0;
  BX_CPU_THIS_PTR msr.sysenter_esp_msr = 0;
  BX_CPU_THIS_PTR msr.sysenter_eip_msr = 0;
#endif

  // Do not change MTRR on INIT
#if BX_CPU_LEVEL >= 6
  if (source == BX_RESET_HARDWARE) {
    for (n=0; n<16; n++)
      BX_CPU_THIS_PTR msr.mtrrphys[n] = 0;

    BX_CPU_THIS_PTR msr.mtrrfix64k_00000 = 0; // all fix range MTRRs undefined according to manual
    BX_CPU_THIS_PTR msr.mtrrfix16k[0] = 0;
    BX_CPU_THIS_PTR msr.mtrrfix16k[1] = 0;

    for (n=0; n<8; n++)
      BX_CPU_THIS_PTR msr.mtrrfix4k[n] = 0;

    BX_CPU_THIS_PTR msr.pat = BX_CONST64(0x0007040600070406);
    BX_CPU_THIS_PTR msr.mtrr_deftype = 0;
  }
#endif

  // All configurable MSRs do not change on INIT
#if BX_CONFIGURE_MSRS
  if (source == BX_RESET_HARDWARE) {
    for (n=0; n < BX_MSR_MAX_INDEX; n++) {
      if (BX_CPU_THIS_PTR msrs[n])
        BX_CPU_THIS_PTR msrs[n]->reset();
    }
  }
#endif

  BX_CPU_THIS_PTR EXT = 0;

  TLB_init();

  // invalidate the prefetch queue
  BX_CPU_THIS_PTR eipPageBias = 0;
  BX_CPU_THIS_PTR eipPageWindowSize = 0;
  BX_CPU_THIS_PTR eipFetchPtr = NULL;

  handleCpuModeChange();

#if BX_DEBUGGER
  BX_CPU_THIS_PTR stop_reason = STOP_NO_REASON;
  BX_CPU_THIS_PTR magic_break = 0;
  BX_CPU_THIS_PTR trace_reg = 0;
  BX_CPU_THIS_PTR trace_mem = 0;
#endif

  BX_CPU_THIS_PTR trace = 0;

  // Reset the Floating Point Unit
#if BX_SUPPORT_FPU
  if (source == BX_RESET_HARDWARE) {
    BX_CPU_THIS_PTR the_i387.reset();
  }
#endif

  // Reset XMM state
#if BX_SUPPORT_SSE >= 1  // unchanged on #INIT
  if (source == BX_RESET_HARDWARE) {
    for(n=0; n<BX_XMM_REGISTERS; n++)
    {
      BX_CPU_THIS_PTR xmm[n].xmm64u(0) = 0;
      BX_CPU_THIS_PTR xmm[n].xmm64u(1) = 0;
    }

    BX_CPU_THIS_PTR mxcsr.mxcsr = MXCSR_RESET;
  }
#endif

#if BX_SUPPORT_VMX
  BX_CPU_THIS_PTR in_vmx = BX_CPU_THIS_PTR in_vmx_guest = 0;
  BX_CPU_THIS_PTR in_event = 0;
  BX_CPU_THIS_PTR vmx_interrupt_window = 0;
  BX_CPU_THIS_PTR vmcsptr = BX_CPU_THIS_PTR vmxonptr = BX_INVALID_VMCSPTR;
  BX_CPU_THIS_PTR vmcshostptr = 0;
#endif

#if BX_SUPPORT_SMP
  // notice if I'm the bootstrap processor.  If not, do the equivalent of
  // a HALT instruction.
  int apic_id = lapic.get_id();
  if (BX_BOOTSTRAP_PROCESSOR == apic_id) {
    // boot normally
    BX_CPU_THIS_PTR msr.apicbase |= 0x0100; /* set bit 8 BSP */
    BX_INFO(("CPU[%d] is the bootstrap processor", apic_id));
  } else {
    // it's an application processor, halt until IPI is heard.
    BX_CPU_THIS_PTR msr.apicbase &= ~0x0100; /* clear bit 8 BSP */
    BX_INFO(("CPU[%d] is an application processor. Halting until IPI.", apic_id));
    activity_state = BX_ACTIVITY_STATE_WAIT_FOR_SIPI;
    disable_INIT = 1; // INIT is disabled when CPU is waiting for SIPI
    async_event = 1;
  }
#endif

  // initialize CPUID values - make sure apicbase already initialized
  set_cpuid_defaults();

  BX_INSTR_RESET(BX_CPU_ID, source);
}

void BX_CPU_C::sanity_checks(void)
{
  Bit8u al, cl, dl, bl, ah, ch, dh, bh;
  Bit16u ax, cx, dx, bx, sp, bp, si, di;
  Bit32u eax, ecx, edx, ebx, esp, ebp, esi, edi;

  EAX = 0xFFEEDDCC;
  ECX = 0xBBAA9988;
  EDX = 0x77665544;
  EBX = 0x332211FF;
  ESP = 0xEEDDCCBB;
  EBP = 0xAA998877;
  ESI = 0x66554433;
  EDI = 0x2211FFEE;

  al = AL;
  cl = CL;
  dl = DL;
  bl = BL;
  ah = AH;
  ch = CH;
  dh = DH;
  bh = BH;

  if ( al != (EAX & 0xFF) ||
       cl != (ECX & 0xFF) ||
       dl != (EDX & 0xFF) ||
       bl != (EBX & 0xFF) ||
       ah != ((EAX >> 8) & 0xFF) ||
       ch != ((ECX >> 8) & 0xFF) ||
       dh != ((EDX >> 8) & 0xFF) ||
       bh != ((EBX >> 8) & 0xFF) )
  {
    BX_PANIC(("problems using BX_READ_8BIT_REGx()!"));
  }

  ax = AX;
  cx = CX;
  dx = DX;
  bx = BX;
  sp = SP;
  bp = BP;
  si = SI;
  di = DI;

  if ( ax != (EAX & 0xFFFF) ||
       cx != (ECX & 0xFFFF) ||
       dx != (EDX & 0xFFFF) ||
       bx != (EBX & 0xFFFF) ||
       sp != (ESP & 0xFFFF) ||
       bp != (EBP & 0xFFFF) ||
       si != (ESI & 0xFFFF) ||
       di != (EDI & 0xFFFF) )
  {
    BX_PANIC(("problems using BX_READ_16BIT_REG()!"));
  }

  eax = EAX;
  ecx = ECX;
  edx = EDX;
  ebx = EBX;
  esp = ESP;
  ebp = EBP;
  esi = ESI;
  edi = EDI;

  if (sizeof(Bit8u)  != 1  ||  sizeof(Bit8s)  != 1)
    BX_PANIC(("data type Bit8u or Bit8s is not of length 1 byte!"));
  if (sizeof(Bit16u) != 2  ||  sizeof(Bit16s) != 2)
    BX_PANIC(("data type Bit16u or Bit16s is not of length 2 bytes!"));
  if (sizeof(Bit32u) != 4  ||  sizeof(Bit32s) != 4)
    BX_PANIC(("data type Bit32u or Bit32s is not of length 4 bytes!"));
  if (sizeof(Bit64u) != 8  ||  sizeof(Bit64s) != 8)
    BX_PANIC(("data type Bit64u or Bit64u is not of length 8 bytes!"));

  BX_DEBUG(("#(%u)all sanity checks passed!", BX_CPU_ID));
}

void BX_CPU_C::assert_checks(void)
{
  // check CPU mode consistency
#if BX_SUPPORT_X86_64
  if (BX_CPU_THIS_PTR efer.get_LMA()) {
    if (! BX_CPU_THIS_PTR cr0.get_PE()) {
      BX_PANIC(("assert_checks: EFER.LMA is set when CR0.PE=0 !"));
    }
    if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.l) {
      if (BX_CPU_THIS_PTR cpu_mode != BX_MODE_LONG_64)
        BX_PANIC(("assert_checks: unconsistent cpu_mode BX_MODE_LONG_64 !"));
    }
    else {
      if (BX_CPU_THIS_PTR cpu_mode != BX_MODE_LONG_COMPAT)
        BX_PANIC(("assert_checks: unconsistent cpu_mode BX_MODE_LONG_COMPAT !"));
    }
  }
  else
#endif
  {
    if (BX_CPU_THIS_PTR cr0.get_PE()) {
      if (BX_CPU_THIS_PTR get_VM()) {
        if (BX_CPU_THIS_PTR cpu_mode != BX_MODE_IA32_V8086)
          BX_PANIC(("assert_checks: unconsistent cpu_mode BX_MODE_IA32_V8086 !"));
      }
      else {
        if (BX_CPU_THIS_PTR cpu_mode != BX_MODE_IA32_PROTECTED)
          BX_PANIC(("assert_checks: unconsistent cpu_mode BX_MODE_IA32_PROTECTED !"));
      }
    }
    else {
      if (BX_CPU_THIS_PTR cpu_mode != BX_MODE_IA32_REAL)
        BX_PANIC(("assert_checks: unconsistent cpu_mode BX_MODE_IA32_REAL !"));
    }
  }

  // check CR0 consistency
  if (BX_CPU_THIS_PTR cr0.get_PG() && ! BX_CPU_THIS_PTR cr0.get_PE())
    BX_PANIC(("assert_checks: CR0.PG=1 with CR0.PE=0 !"));
#if BX_CPU_LEVEL >= 4
  if (BX_CPU_THIS_PTR cr0.get_NW() && ! BX_CPU_THIS_PTR cr0.get_CD())
    BX_PANIC(("assert_checks: CR0.NW=1 with CR0.CD=0 !"));
#endif


#if BX_SUPPORT_X86_64
  // VM should be OFF in long mode
  if (long_mode()) {
    if (BX_CPU_THIS_PTR get_VM()) BX_PANIC(("assert_checks: VM is set in long mode !"));
  }

  // CS.L and CS.D_B are mutualy exclusive
  if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.l &&
      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b)
  {
    BX_PANIC(("assert_checks: CS.l and CS.d_b set together !"));
  }
#endif

  // check LDTR type
  if (BX_CPU_THIS_PTR ldtr.cache.valid)
  {
    if (BX_CPU_THIS_PTR ldtr.cache.type != BX_SYS_SEGMENT_LDT)
    {
      BX_PANIC(("assert_checks: LDTR is not LDT type !"));
    }
  }

  // check Task Register type
  if(BX_CPU_THIS_PTR tr.cache.valid)
  {
    switch(BX_CPU_THIS_PTR tr.cache.type)
    {
      case BX_SYS_SEGMENT_BUSY_286_TSS:
      case BX_SYS_SEGMENT_AVAIL_286_TSS:
#if BX_CPU_LEVEL >= 3
        if (BX_CPU_THIS_PTR tr.cache.u.segment.g != 0)
          BX_PANIC(("assert_checks: tss286.g != 0 !"));
        if (BX_CPU_THIS_PTR tr.cache.u.segment.avl != 0)
          BX_PANIC(("assert_checks: tss286.avl != 0 !"));
#endif
        break;
      case BX_SYS_SEGMENT_BUSY_386_TSS:
      case BX_SYS_SEGMENT_AVAIL_386_TSS:
        break;
      default:
        BX_PANIC(("assert_checks: TR is not TSS type !"));
    }
  }

#if BX_SUPPORT_MONITOR_MWAIT
  if (BX_CPU_THIS_PTR monitor.monitor_end < BX_CPU_THIS_PTR monitor.monitor_begin)
    BX_PANIC(("assert_checks: MONITOR range is not set correctly !"));
#endif
}
