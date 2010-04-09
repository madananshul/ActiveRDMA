/////////////////////////////////////////////////////////////////////////
// $Id: misc_mem.cc,v 1.140 2009/10/23 13:23:31 sshwarts Exp $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2002  MandrakeSoft S.A.
//
//    MandrakeSoft S.A.
//    43, rue d'Aboukir
//    75002 Paris - France
//    http://www.linux-mandrake.com/
//    http://www.mandrakesoft.com/
//
//  I/O memory handlers API Copyright (C) 2003 by Frank Cornelis
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
//
/////////////////////////////////////////////////////////////////////////

#include "bochs.h"
#include "cpu/cpu.h"
#include "iodev/iodev.h"
#define LOG_THIS BX_MEM(0)->

// alignment of memory vector, must be a power of 2
#define BX_MEM_VECTOR_ALIGN 4096
#define BX_MEM_HANDLERS   ((BX_CONST64(1) << BX_PHY_ADDRESS_WIDTH) >> 20) /* one per megabyte */

BX_MEM_C::BX_MEM_C()
{
  put("MEM0");

  vector = NULL;
  actual_vector = NULL;
  blocks = NULL;
  len    = 0;
  used_blocks = 0;

  memory_handlers = NULL;
}

Bit8u* BX_MEM_C::alloc_vector_aligned(Bit32u bytes, Bit32u alignment)
{
  Bit64u test_mask = alignment - 1;
  BX_MEM_THIS actual_vector = new Bit8u [(Bit32u)(bytes + test_mask)];
  if (BX_MEM_THIS actual_vector == 0) {
    BX_PANIC(("alloc_vector_aligned: unable to allocate host RAM !"));
    return 0;
  }
  // round address forward to nearest multiple of alignment.  Alignment
  // MUST BE a power of two for this to work.
  Bit64u masked = ((Bit64u)(BX_MEM_THIS actual_vector + test_mask)) & ~test_mask;
  Bit8u *vector = (Bit8u *) masked;
  // sanity check: no lost bits during pointer conversion
  assert(sizeof(masked) >= sizeof(vector));
  // sanity check: after realignment, everything fits in allocated space
  assert(vector+bytes <= BX_MEM_THIS actual_vector+bytes+test_mask);
  return vector;
}

BX_MEM_C::~BX_MEM_C()
{
  cleanup_memory();
}

void BX_MEM_C::init_memory(Bit64u guest, Bit64u host)
{
  unsigned idx;

  BX_DEBUG(("Init $Id: misc_mem.cc,v 1.140 2009/10/23 13:23:31 sshwarts Exp $"));

  // accept only memory size which is multiply of 1M
  BX_ASSERT((host & 0xfffff) == 0);
  BX_ASSERT((guest & 0xfffff) == 0);

  if (BX_MEM_THIS actual_vector != NULL) {
    BX_INFO(("freeing existing memory vector"));
    delete [] BX_MEM_THIS actual_vector;
    BX_MEM_THIS actual_vector = NULL;
    BX_MEM_THIS vector = NULL;
    BX_MEM_THIS blocks = NULL;
  }
  BX_MEM_THIS vector = alloc_vector_aligned(host + BIOSROMSZ + EXROMSIZE + 4096, BX_MEM_VECTOR_ALIGN);
  BX_INFO(("allocated memory at %p. after alignment, vector=%p",
	BX_MEM_THIS actual_vector, BX_MEM_THIS vector));

  BX_MEM_THIS len = guest;
  BX_MEM_THIS allocated = host;
  BX_MEM_THIS rom = &BX_MEM_THIS vector[host];
  BX_MEM_THIS bogus = &BX_MEM_THIS vector[host + BIOSROMSZ + EXROMSIZE];
  memset(BX_MEM_THIS rom, 0xff, BIOSROMSZ + EXROMSIZE + 4096);
  for (idx = 0; idx < 65; idx++)
    BX_MEM_THIS rom_present[idx] = 0;

  // block must be large enough to fit num_blocks in 32-bit
  BX_ASSERT((BX_MEM_THIS len / BX_MEM_BLOCK_LEN) <= 0xffffffff);

  Bit32u num_blocks = BX_MEM_THIS len / BX_MEM_BLOCK_LEN;
  BX_INFO(("%.2fMB", (float)(BX_MEM_THIS len / (1024.0*1024.0))));
  BX_INFO(("mem block size = 0x%08x, blocks=%u", BX_MEM_BLOCK_LEN, num_blocks));
  BX_MEM_THIS blocks = new Bit8u* [num_blocks];
  if (0) {
    // all guest memory is allocated, just map it
    for (idx = 0; idx < num_blocks; idx++) {
      BX_MEM_THIS blocks[idx] = BX_MEM_THIS vector + (idx * BX_MEM_BLOCK_LEN);
    }
    BX_MEM_THIS used_blocks = num_blocks;
  }
  else {
    // host cannot allocate all requested guest memory
    for (idx = 0; idx < num_blocks; idx++) {
      BX_MEM_THIS blocks[idx] = NULL;
    }
    BX_MEM_THIS used_blocks = 0;
  }

  BX_MEM_THIS memory_handlers = new struct memory_handler_struct *[BX_MEM_HANDLERS];
  for (idx = 0; idx < BX_MEM_HANDLERS; idx++)
    BX_MEM_THIS memory_handlers[idx] = NULL;

  BX_MEM_THIS pci_enabled = SIM->get_param_bool(BXPN_I440FX_SUPPORT)->get();
  BX_MEM_THIS smram_available = 0;
  BX_MEM_THIS smram_enable = 0;
  BX_MEM_THIS smram_restricted = 0;

#if BX_SUPPORT_MONITOR_MWAIT
  BX_MEM_THIS monitor_active = new bx_bool[BX_SMP_PROCESSORS];
  for (int i=0; i<BX_SMP_PROCESSORS;i++) {
    BX_MEM_THIS monitor_active[i] = 0;
  }
  BX_MEM_THIS n_monitors = 0;
#endif

  BX_MEM_THIS register_state();
}

void BX_MEM_C::allocate_block(Bit32u block)
{
  Bit32u max_blocks = BX_MEM_THIS allocated / BX_MEM_BLOCK_LEN;
  if (BX_MEM_THIS used_blocks >= max_blocks) {
    BX_PANIC(("FATAL ERROR: all available memory is already allocated !"));
  }
  else {
    BX_MEM_THIS blocks[block] = BX_MEM_THIS vector + (BX_MEM_THIS used_blocks * BX_MEM_BLOCK_LEN);
    BX_MEM_THIS used_blocks++;
  }
  BX_DEBUG(("allocate_block: used_blocks=%d of %d", BX_MEM_THIS used_blocks, max_blocks));
}

Bit64s memory_param_save_handler(void *devptr, bx_param_c *param)
{
  const char *pname = param->get_name();
  if (! strncmp(pname, "blk", 3)) {
     Bit32u blk_index = atoi(pname + 3);
     if (! BX_MEM(0)->blocks[blk_index]) {
        return -1;
     }
     else {
        Bit32u val = (Bit32u) (BX_MEM(0)->blocks[blk_index] - BX_MEM(0)->vector);
        if ((val & (BX_MEM_BLOCK_LEN-1)) == 0)
           return val / BX_MEM_BLOCK_LEN;
     }
  }

  return -1;
}

void memory_param_restore_handler(void *devptr, bx_param_c *param, Bit64s val)
{
  const char *pname = param->get_name();
  if (! strncmp(pname, "blk", 3)) {
     Bit32u blk_index = atoi(pname + 3);
     if((Bit32s) val < 0)
        BX_MEM(0)->blocks[blk_index] = NULL;
     else
        BX_MEM(0)->blocks[blk_index] = BX_MEM(0)->vector + val * BX_MEM_BLOCK_LEN;
  }
}

void BX_MEM_C::register_state()
{
  bx_list_c *list = new bx_list_c(SIM->get_bochs_root(), "memory", "Memory State", 6);
  new bx_shadow_data_c(list, "ram", BX_MEM_THIS vector, BX_MEM_THIS allocated);
  BXRS_DEC_PARAM_FIELD(list, len, BX_MEM_THIS len);
  BXRS_DEC_PARAM_FIELD(list, allocated, BX_MEM_THIS allocated);
  BXRS_DEC_PARAM_FIELD(list, used_blocks, BX_MEM_THIS used_blocks);

  Bit32u num_blocks = BX_MEM_THIS len / BX_MEM_BLOCK_LEN;
  bx_list_c *mapping = new bx_list_c(list, "mapping", num_blocks);
  for (Bit32u blk=0; blk < num_blocks; blk++) {
    char param_name[15];
    sprintf(param_name, "blk%d", blk);
    bx_param_num_c *param = new bx_param_num_c(mapping, param_name, "", "", 0, BX_MAX_BIT32U, 0);
    param->set_base(BASE_DEC);
    param->set_sr_handlers(this, memory_param_save_handler, memory_param_restore_handler);
  }

#if BX_SUPPORT_MONITOR_MWAIT
  bx_list_c *monitors = new bx_list_c(list, "monitors", BX_SMP_PROCESSORS+1);
  BXRS_PARAM_BOOL(monitors, n_monitors, BX_MEM_THIS n_monitors);
  for (int i=0;i<BX_SMP_PROCESSORS;i++) {
    char param_name[15];
    sprintf(param_name, "cpu%d_monitor", i);
    new bx_shadow_bool_c(monitors, param_name, &BX_MEM_THIS monitor_active[i]);
  }
#endif
}

void BX_MEM_C::cleanup_memory()
{
  unsigned idx;

  if (BX_MEM_THIS vector != NULL) {
    delete [] BX_MEM_THIS actual_vector;
    BX_MEM_THIS actual_vector = NULL;
    BX_MEM_THIS vector = NULL;
    BX_MEM_THIS rom = NULL;
    BX_MEM_THIS bogus = NULL;
    delete [] BX_MEM_THIS blocks;
    BX_MEM_THIS blocks = 0;
    BX_MEM_THIS used_blocks = 0;
    if (BX_MEM_THIS memory_handlers != NULL) {
      for (idx = 0; idx < BX_MEM_HANDLERS; idx++) {
        struct memory_handler_struct *memory_handler = BX_MEM_THIS memory_handlers[idx];
        struct memory_handler_struct *prev = NULL;
        while (memory_handler) {
          prev = memory_handler;
          memory_handler = memory_handler->next;
          delete prev;
        }
      }
      delete [] BX_MEM_THIS memory_handlers;
      BX_MEM_THIS memory_handlers = NULL;
    }
  }
}

//
// Values for type:
//   0 : System Bios
//   1 : VGA Bios
//   2 : Optional ROM Bios
//
void BX_MEM_C::load_ROM(const char *path, bx_phy_address romaddress, Bit8u type)
{
  struct stat stat_buf;
  int fd, ret, i, start_idx, end_idx;
  unsigned long size, max_size, offset;
  bx_bool is_bochs_bios = 0;

  if (*path == '\0') {
    if (type == 2) {
      BX_PANIC(("ROM: Optional ROM image undefined"));
    }
    else if (type == 1) {
      BX_PANIC(("ROM: VGA BIOS image undefined"));
    }
    else {
      BX_PANIC(("ROM: System BIOS image undefined"));
    }
    return;
  }
  // read in ROM BIOS image file
  fd = open(path, O_RDONLY
#ifdef O_BINARY
            | O_BINARY
#endif
           );
  if (fd < 0) {
    if (type < 2) {
      BX_PANIC(("ROM: couldn't open ROM image file '%s'.", path));
    }
    else {
      BX_ERROR(("ROM: couldn't open ROM image file '%s'.", path));
    }
    return;
  }
  ret = fstat(fd, &stat_buf);
  if (ret) {
    if (type < 2) {
      BX_PANIC(("ROM: couldn't stat ROM image file '%s'.", path));
    }
    else {
      BX_ERROR(("ROM: couldn't stat ROM image file '%s'.", path));
    }
    return;
  }

  size = (unsigned long)stat_buf.st_size;

  if (type > 0) {
    max_size = 0x20000;
  } else {
    max_size = BIOSROMSZ;
  }
  if (size > max_size) {
    close(fd);
    BX_PANIC(("ROM: ROM image too large"));
    return;
  }
  if (type == 0) {
    if (romaddress > 0) {
      if ((romaddress + size) != 0x100000 && (romaddress + size)) {
        close(fd);
        BX_PANIC(("ROM: System BIOS must end at 0xfffff"));
        return;
      }
    } else {
      romaddress = -size;
    }
    offset = romaddress & BIOS_MASK;
    if ((romaddress & 0xf0000) < 0xf0000) {
      BX_MEM_THIS rom_present[64] = 1;
    }
    is_bochs_bios = (strstr(path, "BIOS-bochs-latest") != NULL);
  } else {
    if ((size % 512) != 0) {
      close(fd);
      BX_PANIC(("ROM: ROM image size must be multiple of 512 (size = %ld)", size));
      return;
    }
    if ((romaddress % 2048) != 0) {
      close(fd);
      BX_PANIC(("ROM: ROM image must start at a 2k boundary"));
      return;
    }
    if ((romaddress < 0xc0000) ||
        (((romaddress + size - 1) > 0xdffff) && (romaddress < 0xe0000))) {
      close(fd);
      BX_PANIC(("ROM: ROM address space out of range"));
      return;
    }
    if (romaddress < 0xe0000) {
      offset = (romaddress & EXROM_MASK) + BIOSROMSZ;
      start_idx = ((romaddress - 0xc0000) >> 11);
      end_idx = start_idx + (size >> 11) + (((size % 2048) > 0) ? 1 : 0);
    } else {
      offset = romaddress & BIOS_MASK;
      start_idx = 64;
      end_idx = 64;
    }
    for (i = start_idx; i < end_idx; i++) {
      if (BX_MEM_THIS rom_present[i]) {
        close(fd);
        BX_PANIC(("ROM: address space 0x%x already in use", (i * 2048) + 0xc0000));
        return;
      } else {
        BX_MEM_THIS rom_present[i] = 1;
      }
    }
  }
  while (size > 0) {
    ret = read(fd, (bx_ptr_t) &BX_MEM_THIS rom[offset], size);
    if (ret <= 0) {
      BX_PANIC(("ROM: read failed on BIOS image: '%s'",path));
    }
    size -= ret;
    offset += ret;
  }
  close(fd);
  offset -= (unsigned long)stat_buf.st_size;
  if (((romaddress & 0xfffff) != 0xe0000) ||
      ((BX_MEM_THIS rom[offset] == 0x55) && (BX_MEM_THIS rom[offset+1] == 0xaa))) {
    Bit8u checksum = 0;
    for (i = 0; i < stat_buf.st_size; i++) {
      checksum += BX_MEM_THIS rom[offset + i];
    }
    if (checksum != 0) {
      if (type == 1) {
        BX_PANIC(("ROM: checksum error in VGABIOS image: '%s'", path));
      } else if (is_bochs_bios) {
        BX_ERROR(("ROM: checksum error in BIOS image: '%s'", path));
      }
    }
  }
  BX_INFO(("rom at 0x%05x/%u ('%s')",
			(unsigned) romaddress,
			(unsigned) stat_buf.st_size,
 			path));
}

void BX_MEM_C::load_RAM(const char *path, bx_phy_address ramaddress, Bit8u type)
{
  struct stat stat_buf;
  int fd, ret;
  unsigned long size, offset;

  if (*path == '\0') {
    BX_PANIC(("RAM: Optional RAM image undefined"));
    return;
  }
  // read in RAM BIOS image file
  fd = open(path, O_RDONLY
#ifdef O_BINARY
            | O_BINARY
#endif
           );
  if (fd < 0) {
    BX_PANIC(("RAM: couldn't open RAM image file '%s'.", path));
    return;
  }
  ret = fstat(fd, &stat_buf);
  if (ret) {
    BX_PANIC(("RAM: couldn't stat RAM image file '%s'.", path));
    return;
  }

  size = (unsigned long)stat_buf.st_size;

  offset = ramaddress;
  while (size > 0) {
    ret = read(fd, (bx_ptr_t) BX_MEM_THIS get_vector(offset), size);
    if (ret <= 0) {
      BX_PANIC(("RAM: read failed on RAM image: '%s'",path));
    }
    size -= ret;
    offset += ret;
  }
  close(fd);
  BX_INFO(("ram at 0x%05x/%u ('%s')",
			(unsigned) ramaddress,
			(unsigned) stat_buf.st_size,
 			path));
}

#if (BX_DEBUGGER || BX_DISASM || BX_GDBSTUB)
bx_bool BX_MEM_C::dbg_fetch_mem(BX_CPU_C *cpu, bx_phy_address addr, unsigned len, Bit8u *buf)
{
  bx_bool ret = 1;

  for (; len>0; len--) {
    // Reading standard PCI/ISA Video Mem / SMMRAM
    if (addr >= 0x000a0000 && addr < 0x000c0000) {
      if (BX_MEM_THIS smram_enable || cpu->smm_mode())
        *buf = *(BX_MEM_THIS get_vector(addr));
      else
        *buf = DEV_vga_mem_read(addr);
    }
#if BX_SUPPORT_PCI
    else if (BX_MEM_THIS pci_enabled && (addr >= 0x000c0000 && addr < 0x00100000))
    {
      switch (DEV_pci_rd_memtype (addr)) {
        case 0x0:  // Read from ROM
          if ((addr & 0xfffe0000) == 0x000e0000) {
            *buf = BX_MEM_THIS rom[addr & BIOS_MASK];
          }
          else {
            *buf = BX_MEM_THIS rom[(addr & EXROM_MASK) + BIOSROMSZ];
          }
          break;
        case 0x1:  // Read from ShadowRAM
          *buf = *(BX_MEM_THIS get_vector(addr));
          break;
        default:
          BX_PANIC(("dbg_fetch_mem: default case"));
      }
    }
#endif  // #if BX_SUPPORT_PCI
    else if (addr < BX_MEM_THIS len)
    {
      if (addr < 0x000c0000 || addr >= 0x00100000) {
        *buf = *(BX_MEM_THIS get_vector(addr));
      }
      // must be in C0000 - FFFFF range
      else if ((addr & 0xfffe0000) == 0x000e0000) {
        *buf = BX_MEM_THIS rom[addr & BIOS_MASK];
      }
      else {
        *buf = BX_MEM_THIS rom[(addr & EXROM_MASK) + BIOSROMSZ];
      }
    }
#if BX_PHY_ADDRESS_LONG
    else if (addr > BX_CONST64(0xffffffff)) {
      *buf = 0xff;
      ret = 0; // error, beyond limits of memory
    }
#endif
    else if (addr >= (bx_phy_address)~BIOS_MASK)
    {
      *buf = BX_MEM_THIS rom[addr & BIOS_MASK];
    }
    else
    {
      *buf = 0xff;
      ret = 0; // error, beyond limits of memory
    }
    buf++;
    addr++;
  }
  return ret;
}
#endif

#if BX_DEBUGGER || BX_GDBSTUB
bx_bool BX_MEM_C::dbg_set_mem(bx_phy_address addr, unsigned len, Bit8u *buf)
{
  if ((addr + len - 1) > BX_MEM_THIS len) {
    return(0); // error, beyond limits of memory
  }
  for (; len>0; len--) {
    // Write to standard PCI/ISA Video Mem / SMMRAM
    if (addr >= 0x000a0000 && addr < 0x000c0000) {
      if (BX_MEM_THIS smram_enable)
        *(BX_MEM_THIS get_vector(addr)) = *buf;
      else
        DEV_vga_mem_write(addr, *buf);
    }
#if BX_SUPPORT_PCI
    else if (BX_MEM_THIS pci_enabled && (addr >= 0x000c0000 && addr < 0x00100000))
    {
      switch (DEV_pci_wr_memtype (addr)) {
        case 0x0:  // Ignore write to ROM
          break;
        case 0x1:  // Write to ShadowRAM
          *(BX_MEM_THIS get_vector(addr)) = *buf;
          break;
        default:
          BX_PANIC(("dbg_fetch_mem: default case"));
      }
    }
#endif  // #if BX_SUPPORT_PCI
    else if ((addr < 0x000c0000 || addr >= 0x00100000) && (addr < (bx_phy_address)(~BIOS_MASK)))
    {
      *(BX_MEM_THIS get_vector(addr)) = *buf;
    }
    buf++;
    addr++;
  }
  return(1);
}

bx_bool BX_MEM_C::dbg_crc32(bx_phy_address addr1, bx_phy_address addr2, Bit32u *crc)
{
  *crc = 0;
  if (addr1 > addr2)
    return(0);

  if (addr2 >= BX_MEM_THIS len)
    return(0); // error, specified address past last phy mem addr

  unsigned len = 1 + addr2 - addr1;

  // do not cross 4K boundary
  while(1) { 
    unsigned remainsInPage = 0x1000 - (addr1 & 0xfff);
    unsigned access_length = (len < remainsInPage) ? len : remainsInPage;
    *crc = crc32(BX_MEM_THIS get_vector(addr1), access_length);
    addr1 += access_length;
    len -= access_length;
  }

  return(1);
}
#endif

//
// Return a host address corresponding to the guest physical memory
// address (with A20 already applied), given that the calling
// code will perform an 'op' operation.  This address will be
// used for direct access to guest memory.
// Values of 'op' are { BX_READ, BX_WRITE, BX_EXECUTE, BX_RW }.
//
// The other assumption is that the calling code _only_ accesses memory
// directly within the page that encompasses the address requested.
//

//
// Memory map inside the 1st megabyte:
//
// 0x00000 - 0x7ffff    DOS area (512K)
// 0x80000 - 0x9ffff    Optional fixed memory hole (128K)
// 0xa0000 - 0xbffff    Standard PCI/ISA Video Mem / SMMRAM (128K)
// 0xc0000 - 0xdffff    Expansion Card BIOS and Buffer Area (128K)
// 0xe0000 - 0xeffff    Lower BIOS Area (64K)
// 0xf0000 - 0xfffff    Upper BIOS Area (64K)
//

Bit8u *BX_MEM_C::getHostMemAddr(BX_CPU_C *cpu, bx_phy_address addr, unsigned rw)
{
  bx_phy_address a20addr = A20ADDR(addr);

  bx_bool is_bios = (a20addr >= (bx_phy_address)~BIOS_MASK);
#if BX_PHY_ADDRESS_LONG
  if (a20addr > BX_CONST64(0xffffffff)) is_bios = 0;
#endif

#if BX_SUPPORT_APIC
  if (cpu != NULL) {
    if (cpu->lapic.is_selected(a20addr))
      return(NULL); // Vetoed!  APIC address space
  }
#endif

  bx_bool write = rw & 1;

  // allow direct access to SMRAM memory space for code and veto data
  if ((cpu != NULL) && (rw == BX_EXECUTE)) {
    // reading from SMRAM memory space
    if ((a20addr >= 0x000a0000 && a20addr < 0x000c0000) && (BX_MEM_THIS smram_available))
    {
      if (BX_MEM_THIS smram_enable || cpu->smm_mode())
        return BX_MEM_THIS get_vector(a20addr);
    }
  }

#if BX_SUPPORT_MONITOR_MWAIT
  if (write && BX_MEM_THIS is_monitor(a20addr & ~0xfff, 0x1000)) {
    // Vetoed! Write monitored page !
    return(NULL);
  }
#endif

  struct memory_handler_struct *memory_handler = BX_MEM_THIS memory_handlers[a20addr >> 20];
  while (memory_handler) {
    if (memory_handler->begin <= a20addr &&
        memory_handler->end >= a20addr) {
      return(NULL); // Vetoed! memory handler for i/o apic, vram, mmio and PCI PnP
    }
    memory_handler = memory_handler->next;
  }

  if (! write) {
    if ((a20addr >= 0x000a0000 && a20addr < 0x000c0000))
      return(NULL); // Vetoed!  Mem mapped IO (VGA)
#if BX_SUPPORT_PCI
    else if (BX_MEM_THIS pci_enabled && (a20addr >= 0x000c0000 && a20addr < 0x00100000))
    {
      switch (DEV_pci_rd_memtype (a20addr)) {
        case 0x0:   // Read from ROM
          if ((a20addr & 0xfffe0000) == 0x000e0000) {
            return (Bit8u *) &BX_MEM_THIS rom[a20addr & BIOS_MASK];
          }
          else {
            return (Bit8u *) &BX_MEM_THIS rom[(a20addr & EXROM_MASK) + BIOSROMSZ];
          }
          break;
        case 0x1:   // Read from ShadowRAM
          return BX_MEM_THIS get_vector(a20addr);
        default:
          BX_PANIC(("getHostMemAddr(): default case"));
          return(NULL);
      }
    }
#endif
    else if(a20addr < BX_MEM_THIS len && ! is_bios)
    {
      if (a20addr < 0x000c0000 || a20addr >= 0x00100000) {
        return BX_MEM_THIS get_vector(a20addr);
      }
      // must be in C0000 - FFFFF range
      else if ((a20addr & 0xfffe0000) == 0x000e0000) {
        return (Bit8u *) &BX_MEM_THIS rom[a20addr & BIOS_MASK];
      }
      else {
        return((Bit8u *) &BX_MEM_THIS rom[(a20addr & EXROM_MASK) + BIOSROMSZ]);
      }
    }
#if BX_PHY_ADDRESS_LONG
    else if (a20addr > BX_CONST64(0xffffffff)) {
      // Error, requested addr is out of bounds.
      return (Bit8u *) &BX_MEM_THIS bogus[a20addr & 0xfff];
    }
#endif
    else if (a20addr >= (bx_phy_address)~BIOS_MASK)
    {
      return (Bit8u *) &BX_MEM_THIS rom[a20addr & BIOS_MASK];
    }
    else
    {
      // Error, requested addr is out of bounds.
      return (Bit8u *) &BX_MEM_THIS bogus[a20addr & 0xfff];
    }
  }
  else
  { // op == {BX_WRITE, BX_RW}
    if (a20addr >= BX_MEM_THIS len || is_bios)
      return(NULL); // Error, requested addr is out of bounds.
    else if (a20addr >= 0x000a0000 && a20addr < 0x000c0000)
      return(NULL); // Vetoed!  Mem mapped IO (VGA)
#if BX_SUPPORT_PCI
    else if (BX_MEM_THIS pci_enabled && (a20addr >= 0x000c0000 && a20addr < 0x00100000))
    {
      // Veto direct writes to this area. Otherwise, there is a chance
      // for Guest2HostTLB and memory consistency problems, for example
      // when some 16K block marked as write-only using PAM registers.
      return(NULL);
    }
#endif
    else
    {
      if (a20addr < 0x000c0000 || a20addr >= 0x00100000) {
        return BX_MEM_THIS get_vector(a20addr);
      }
      else {
        return(NULL);  // Vetoed!  ROMs
      }
    }
  }
}

/*
 * One needs to provide both a read_handler and a write_handler.
 * XXX: maybe we should check for overlapping memory handlers
 */
  bx_bool
BX_MEM_C::registerMemoryHandlers(void *param, memory_handler_t read_handler,
		memory_handler_t write_handler, bx_phy_address begin_addr, bx_phy_address end_addr)
{
  if (end_addr < begin_addr)
    return 0;
  if (!read_handler || !write_handler)
    return 0;
  BX_INFO(("Register memory access handlers: 0x" FMT_PHY_ADDRX " - 0x" FMT_PHY_ADDRX, begin_addr, end_addr));
  for (unsigned page_idx = begin_addr >> 20; page_idx <= end_addr >> 20; page_idx++) {
    struct memory_handler_struct *memory_handler = new struct memory_handler_struct;
    memory_handler->next = BX_MEM_THIS memory_handlers[page_idx];
    BX_MEM_THIS memory_handlers[page_idx] = memory_handler;
    memory_handler->read_handler = read_handler;
    memory_handler->write_handler = write_handler;
    memory_handler->param = param;
    memory_handler->begin = begin_addr;
    memory_handler->end = end_addr;
  }
  return 1;
}

  bx_bool
BX_MEM_C::unregisterMemoryHandlers(memory_handler_t read_handler, memory_handler_t write_handler,
		bx_phy_address begin_addr, bx_phy_address end_addr)
{
  bx_bool ret = 1;
  BX_INFO(("Memory access handlers unregistered: 0x" FMT_PHY_ADDRX " - 0x" FMT_PHY_ADDRX, begin_addr, end_addr));
  for (unsigned page_idx = begin_addr >> 20; page_idx <= end_addr >> 20; page_idx++) {
    struct memory_handler_struct *memory_handler = BX_MEM_THIS memory_handlers[page_idx];
    struct memory_handler_struct *prev = NULL;
    while (memory_handler &&
         memory_handler->read_handler != read_handler &&
         memory_handler->write_handler != write_handler &&
         memory_handler->begin != begin_addr &&
         memory_handler->end != end_addr)
    {
      prev = memory_handler;
      memory_handler = memory_handler->next;
    }
    if (!memory_handler) {
      ret = 0;  // we should have found it
      continue; // anyway, try the other pages
    }
    if (prev)
      prev->next = memory_handler->next;
    else
      BX_MEM_THIS memory_handlers[page_idx] = memory_handler->next;
    delete memory_handler;
  }
  return ret;
}

void BX_MEM_C::enable_smram(bx_bool enable, bx_bool restricted)
{
  BX_MEM_THIS smram_available = 1;
  BX_MEM_THIS smram_enable = (enable > 0);
  BX_MEM_THIS smram_restricted = (restricted > 0);
}

void BX_MEM_C::disable_smram(void)
{
  BX_MEM_THIS smram_available  = 0;
  BX_MEM_THIS smram_enable     = 0;
  BX_MEM_THIS smram_restricted = 0;
}

// check if SMRAM is aavailable for CPU data accesses
bx_bool BX_MEM_C::is_smram_accessible(void)
{
  return(BX_MEM_THIS smram_available) &&
        (BX_MEM_THIS smram_enable || !BX_MEM_THIS smram_restricted);
}

#if BX_SUPPORT_MONITOR_MWAIT

//
// MONITOR/MWAIT - x86arch way to optimize idle loops in CPU
//

void BX_MEM_C::set_monitor(unsigned cpu)
{
  BX_ASSERT(cpu < BX_SMP_PROCESSORS);
  if (! BX_MEM_THIS monitor_active[cpu]) {
    BX_MEM_THIS monitor_active[cpu] = 1;
    BX_MEM_THIS n_monitors++;
    BX_DEBUG(("activate monitor for cpu=%d", cpu));
  }
  else {
    BX_DEBUG(("monitor for cpu=%d already active !", cpu));
  }
}

void BX_MEM_C::clear_monitor(unsigned cpu)
{
  BX_ASSERT(cpu < BX_SMP_PROCESSORS);
  BX_MEM_THIS monitor_active[cpu] = 0;
  BX_MEM_THIS n_monitors--;
  BX_DEBUG(("deactivate monitor for cpu=%d", cpu));
}

bx_bool BX_MEM_C::is_monitor(bx_phy_address begin_addr, unsigned len)
{
  if (BX_MEM_THIS n_monitors == 0) return 0;

  for (int i=0; i<BX_SMP_PROCESSORS;i++) {
    if (BX_MEM_THIS monitor_active[i]) {
      if (BX_CPU(i)->is_monitor(begin_addr, len))
        return 1;
    }
  }

  return 0; // // this is NOT monitored page
}

void BX_MEM_C::check_monitor(bx_phy_address begin_addr, unsigned len)
{
  if (BX_MEM_THIS n_monitors == 0) return;

  for (int i=0; i<BX_SMP_PROCESSORS;i++) {
    if (BX_MEM_THIS monitor_active[i]) {
      BX_CPU(i)->check_monitor(begin_addr, len);
    }
  }
}

#endif
