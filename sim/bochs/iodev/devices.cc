/////////////////////////////////////////////////////////////////////////
// $Id: devices.cc,v 1.148 2009/10/15 16:14:30 sshwarts Exp $
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
//  I/O port handlers API Copyright (C) 2003 by Frank Cornelis
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


#include "iodev.h"

#include "iodev/virt_timer.h"
#include "iodev/slowdown_timer.h"

#define LOG_THIS bx_devices.

/* main memory size (in Kbytes)
 * subtract 1k for extended BIOS area
 * report only base memory, not extended mem
 */
#define BASE_MEMORY_IN_K  640


bx_devices_c bx_devices;


// constructor for bx_devices_c
bx_devices_c::bx_devices_c()
{
  put("DEV");

  read_port_to_handler = NULL;
  write_port_to_handler = NULL;
  io_read_handlers.next = NULL;
  io_read_handlers.handler_name = NULL;
  io_write_handlers.next = NULL;
  io_write_handlers.handler_name = NULL;
  init_stubs();

  for (unsigned i=0; i < BX_MAX_IRQS; i++) {
    irq_handler_name[i] = NULL;
  }
}

bx_devices_c::~bx_devices_c()
{
  // nothing needed for now
  timer_handle = BX_NULL_TIMER_HANDLE;
}

void bx_devices_c::init_stubs()
{
#if BX_SUPPORT_PCI
  pluginPciBridge = &stubPci;
  pluginPci2IsaBridge = &stubPci2Isa;
  pluginPciIdeController = &stubPciIde;
#if BX_SUPPORT_ACPI
  pluginACPIController = &stubACPIController;
#endif
#endif
  pluginKeyboard = &stubKeyboard;
  pluginDmaDevice = &stubDma;
  pluginFloppyDevice = &stubFloppy;
  pluginCmosDevice = &stubCmos;
  pluginVgaDevice = &stubVga;
  pluginPicDevice = &stubPic;
  pluginHardDrive = &stubHardDrive;
  pluginNE2kDevice =&stubNE2k;
  pluginSpeaker = &stubSpeaker;
#if BX_SUPPORT_IODEBUG
  pluginIODebug = &stubIODebug;
#endif
#if BX_SUPPORT_IOAPIC
  pluginIOAPIC = &stubIOAPIC;
#endif
#if 0
  g2h = NULL;
#endif
}

void bx_devices_c::init(BX_MEM_C *newmem)
{
  unsigned i;
  const char def_name[] = "Default";
  bx_list_c *plugin_ctrl;
  bx_param_bool_c *plugin;
#if !BX_PLUGINS
  const char *plugname;
#endif

  BX_DEBUG(("Init $Id: devices.cc,v 1.148 2009/10/15 16:14:30 sshwarts Exp $"));
  mem = newmem;

  /* set builtin default handlers, will be overwritten by the real default handler */
  register_default_io_read_handler(NULL, &default_read_handler, def_name, 7);
  io_read_handlers.next = &io_read_handlers;
  io_read_handlers.prev = &io_read_handlers;
  io_read_handlers.usage_count = 0; // not used with the default handler

  register_default_io_write_handler(NULL, &default_write_handler, def_name, 7);
  io_write_handlers.next = &io_write_handlers;
  io_write_handlers.prev = &io_write_handlers;
  io_write_handlers.usage_count = 0; // not used with the default handler

  if (read_port_to_handler)
    delete [] read_port_to_handler;
  if (write_port_to_handler)
    delete [] write_port_to_handler;
  read_port_to_handler = new struct io_handler_struct *[PORTS];
  write_port_to_handler = new struct io_handler_struct *[PORTS];

  /* set handlers to the default one */
  for (i=0; i < PORTS; i++) {
    read_port_to_handler[i] = &io_read_handlers;
    write_port_to_handler[i] = &io_write_handlers;
  }

  for (i=0; i < BX_MAX_IRQS; i++) {
    delete [] irq_handler_name[i];
    irq_handler_name[i] = NULL;
  }

  // removable devices init
  bx_keyboard.dev = NULL;
  bx_keyboard.enq_event = NULL;
  for (i=0; i < 2; i++) {
    bx_mouse[i].dev = NULL;
    bx_mouse[i].enq_event = NULL;
    bx_mouse[i].enabled_changed = NULL;
  }
  // common mouse settings
  mouse_captured = SIM->get_param_bool(BXPN_MOUSE_ENABLED)->get();
  mouse_type = SIM->get_param_enum(BXPN_MOUSE_TYPE)->get();

  // register as soon as possible - the devices want to have their timers !
  bx_virt_timer.init();
  bx_slowdown_timer.init();

  // BBD: At present, the only difference between "core" and "optional"
  // plugins is that initialization and reset of optional plugins is handled
  // by the plugin device list ().  Init and reset of core plugins is done
  // "by hand" in this file.  Basically, we're using core plugins when we
  // want to control the init order.
  //
  PLUG_load_plugin(cmos, PLUGTYPE_CORE);
  PLUG_load_plugin(dma, PLUGTYPE_CORE);
  PLUG_load_plugin(pic, PLUGTYPE_CORE);
  PLUG_load_plugin(pit, PLUGTYPE_CORE);
  PLUG_load_plugin(vga, PLUGTYPE_CORE);
  PLUG_load_plugin(floppy, PLUGTYPE_CORE);

  // PCI logic (i440FX)
  if (SIM->get_param_bool(BXPN_I440FX_SUPPORT)->get()) {
#if BX_SUPPORT_PCI
    PLUG_load_plugin(pci, PLUGTYPE_CORE);
    PLUG_load_plugin(pci2isa, PLUGTYPE_CORE);
  } else {
    plugin_ctrl = (bx_list_c*)SIM->get_param(BXPN_PLUGIN_CTRL);
    SIM->get_param_bool(BX_PLUGIN_PCI_IDE, plugin_ctrl)->set(0);
    SIM->get_param_bool(BX_PLUGIN_ACPI, plugin_ctrl)->set(0);
  }
#else
    BX_ERROR(("Bochs is not compiled with PCI support"));
  }
#endif

  // optional plugins not controlled by separate option
  plugin_ctrl = (bx_list_c*)SIM->get_param(BXPN_PLUGIN_CTRL);
  for (i = 0; i < (unsigned)plugin_ctrl->get_size(); i++) {
    plugin = (bx_param_bool_c*)(plugin_ctrl->get(i));
    if (plugin->get()) {
#if BX_PLUGINS
      PLUG_load_opt_plugin(plugin->get_name());
#else
      // workaround in case of plugins disabled
      plugname = plugin->get_name();
      if (!strcmp(plugname, BX_PLUGIN_UNMAPPED)) {
        PLUG_load_plugin(unmapped, PLUGTYPE_OPTIONAL);
      }
      else if (!strcmp(plugname, BX_PLUGIN_BIOSDEV)) {
        PLUG_load_plugin(biosdev, PLUGTYPE_OPTIONAL);
      }
      else if (!strcmp(plugname, BX_PLUGIN_SPEAKER)) {
        PLUG_load_plugin(speaker, PLUGTYPE_OPTIONAL);
      }
      else if (!strcmp(plugname, BX_PLUGIN_EXTFPUIRQ)) {
        PLUG_load_plugin(extfpuirq, PLUGTYPE_OPTIONAL);
      }
#if BX_SUPPORT_GAMEPORT
      else if (!strcmp(plugname, BX_PLUGIN_GAMEPORT)) {
        PLUG_load_plugin(gameport, PLUGTYPE_OPTIONAL);
      }
#endif
#if BX_SUPPORT_IODEBUG
      else if (!strcmp(plugname, BX_PLUGIN_IODEBUG)) {
        PLUG_load_plugin(iodebug, PLUGTYPE_OPTIONAL);
      }
#endif
#if BX_SUPPORT_PCI
      else if (!strcmp(plugname, BX_PLUGIN_PCI_IDE)) {
        PLUG_load_plugin(pci_ide, PLUGTYPE_OPTIONAL);
      }
#endif
#if BX_SUPPORT_ACPI
      else if (!strcmp(plugname, BX_PLUGIN_ACPI)) {
        PLUG_load_plugin(acpi, PLUGTYPE_OPTIONAL);
      }
#endif
#if BX_SUPPORT_APIC
      else if (!strcmp(plugname, BX_PLUGIN_IOAPIC)) {
        PLUG_load_plugin(ioapic, PLUGTYPE_OPTIONAL);
      }
#endif
#endif
    }
  }

  PLUG_load_plugin(keyboard, PLUGTYPE_OPTIONAL);
#if BX_SUPPORT_BUSMOUSE
  if (mouse_type == BX_MOUSE_TYPE_BUS) {
    PLUG_load_plugin(busmouse, PLUGTYPE_OPTIONAL);
  }
#endif
  if (is_harddrv_enabled())
    PLUG_load_plugin(harddrv, PLUGTYPE_OPTIONAL);
  if (is_serial_enabled())
    PLUG_load_plugin(serial, PLUGTYPE_OPTIONAL);
  if (is_parallel_enabled())
    PLUG_load_plugin(parallel, PLUGTYPE_OPTIONAL);

#if BX_SUPPORT_PCI
  if (SIM->get_param_bool(BXPN_I440FX_SUPPORT)->get()) {
#if BX_SUPPORT_PCIVGA && BX_SUPPORT_VBE
    if ((DEV_is_pci_device("pcivga")) &&
        (!strcmp(SIM->get_param_string(BXPN_VGA_EXTENSION)->getptr(), "vbe"))) {
      PLUG_load_plugin(pcivga, PLUGTYPE_OPTIONAL);
    }
#endif
#if BX_SUPPORT_USB_UHCI
    if (is_usb_uhci_enabled()) {
      PLUG_load_plugin(usb_uhci, PLUGTYPE_OPTIONAL);
    }
#endif
#if BX_SUPPORT_USB_OHCI
    if (is_usb_ohci_enabled()) {
      PLUG_load_plugin(usb_ohci, PLUGTYPE_OPTIONAL);
    }
#endif
#if BX_SUPPORT_PCIDEV
    if (SIM->get_param_num(BXPN_PCIDEV_VENDOR)->get() != 0xffff) {
      PLUG_load_plugin(pcidev, PLUGTYPE_OPTIONAL);
    }
#endif
#if BX_SUPPORT_PCIPNIC
  if (SIM->get_param_bool(BXPN_PNIC_ENABLED)->get()) {
    PLUG_load_plugin(pcipnic, PLUGTYPE_OPTIONAL);
  }
#endif
  }
#endif

  // NE2000 NIC
  if (SIM->get_param_bool(BXPN_NE2K_ENABLED)->get()) {
#if BX_SUPPORT_NE2K
    PLUG_load_plugin(ne2k, PLUGTYPE_OPTIONAL);
#else
    BX_ERROR(("Bochs is not compiled with NE2K support"));
#endif
  }

  //--- SOUND ---
  if (SIM->get_param_bool(BXPN_SB16_ENABLED)->get()) {
#if BX_SUPPORT_SB16
    PLUG_load_plugin(sb16, PLUGTYPE_OPTIONAL);
#else
    BX_ERROR(("Bochs is not compiled with SB16 support"));
#endif
  }

  // CMOS RAM & RTC
  pluginCmosDevice->init();

  /*--- 8237 DMA ---*/
  pluginDmaDevice->init();

  //--- FLOPPY ---
  pluginFloppyDevice->init();

#if BX_SUPPORT_PCI
  pluginPciBridge->init();
  pluginPci2IsaBridge->init();
#endif

  /*--- VGA adapter ---*/
  pluginVgaDevice->init();

  /*--- 8259A PIC ---*/
  pluginPicDevice->init();

  /*--- 82C54 PIT ---*/
  pluginPitDevice->init();

#if 0
  // Guest to Host interface.  Used with special guest drivers
  // which move data to/from the host environment.
  g2h = &bx_g2h;
  g2h->init();
#endif

  // system hardware
  register_io_read_handler(this, &read_handler, 0x0092,
                           "Port 92h System Control", 1);
  register_io_write_handler(this, &write_handler, 0x0092,
                            "Port 92h System Control", 1);

  // misc. CMOS
  Bit32u memory_in_k = mem->get_memory_len() / 1024;
  Bit32u extended_memory_in_k = memory_in_k > 1024 ? (memory_in_k - 1024) : 0;
  if (extended_memory_in_k > 0xfc00) extended_memory_in_k = 0xfc00;

  DEV_cmos_set_reg(0x15, (Bit8u) BASE_MEMORY_IN_K);
  DEV_cmos_set_reg(0x16, (Bit8u) (BASE_MEMORY_IN_K >> 8));
  DEV_cmos_set_reg(0x17, (Bit8u) (extended_memory_in_k & 0xff));
  DEV_cmos_set_reg(0x18, (Bit8u) ((extended_memory_in_k >> 8) & 0xff));
  DEV_cmos_set_reg(0x30, (Bit8u) (extended_memory_in_k & 0xff));
  DEV_cmos_set_reg(0x31, (Bit8u) ((extended_memory_in_k >> 8) & 0xff));

  Bit32u extended_memory_in_64k = memory_in_k > 16384 ? (memory_in_k - 16384) / 64 : 0;
  if (extended_memory_in_64k > 0xffff) extended_memory_in_64k = 0xffff;

  DEV_cmos_set_reg(0x34, (Bit8u) (extended_memory_in_64k & 0xff));
  DEV_cmos_set_reg(0x35, (Bit8u) ((extended_memory_in_64k >> 8) & 0xff));

  if (timer_handle != BX_NULL_TIMER_HANDLE) {
    timer_handle = bx_pc_system.register_timer(this, timer_handler,
      (unsigned) BX_IODEV_HANDLER_PERIOD, 1, 1, "devices.cc");
  }

  // Clear fields for bulk IO acceleration transfers.
  bulkIOHostAddr = 0;
  bulkIOQuantumsRequested = 0;
  bulkIOQuantumsTransferred = 0;

  bx_init_plugins();

  /* now perform checksum of CMOS memory */
  DEV_cmos_checksum();
}

void bx_devices_c::reset(unsigned type)
{
  mem->disable_smram();
#if BX_SUPPORT_PCI
  if (SIM->get_param_bool(BXPN_I440FX_SUPPORT)->get()) {
    pluginPciBridge->reset(type);
    pluginPci2IsaBridge->reset(type);
  }
#endif
  pluginCmosDevice->reset(type);
  pluginDmaDevice->reset(type);
  pluginFloppyDevice->reset(type);
  pluginVgaDevice->reset(type);
  pluginPicDevice->reset(type);
  pluginPitDevice->reset(type);
  // now reset optional plugins
  bx_reset_plugins(type);
}

void bx_devices_c::register_state()
{
  bx_virt_timer.register_state();
#if BX_SUPPORT_PCI
  if (SIM->get_param_bool(BXPN_I440FX_SUPPORT)->get()) {
    pluginPciBridge->register_state();
    pluginPci2IsaBridge->register_state();
  }
#endif
  pluginCmosDevice->register_state();
  pluginDmaDevice->register_state();
  pluginFloppyDevice->register_state();
  pluginVgaDevice->register_state();
  pluginPicDevice->register_state();
  pluginPitDevice->register_state();
  // now register state of optional plugins
  bx_plugins_register_state();
}

void bx_devices_c::after_restore_state()
{
  bx_slowdown_timer.after_restore_state();
#if BX_SUPPORT_PCI
  if (SIM->get_param_bool(BXPN_I440FX_SUPPORT)->get()) {
    pluginPciBridge->after_restore_state();
    pluginPci2IsaBridge->after_restore_state();
  }
#endif
  pluginCmosDevice->after_restore_state();
  pluginVgaDevice->after_restore_state();
  bx_plugins_after_restore_state();
}

void bx_devices_c::exit()
{
  // delete i/o handlers before unloading plugins
  struct io_handler_struct *io_read_handler = io_read_handlers.next;
  struct io_handler_struct *curr = NULL;
  while (io_read_handler != &io_read_handlers) {
    io_read_handler->prev->next = io_read_handler->next;
    io_read_handler->next->prev = io_read_handler->prev;
    curr = io_read_handler;
    io_read_handler = io_read_handler->next;
    delete [] curr->handler_name;
    delete curr;
  }
  struct io_handler_struct *io_write_handler = io_write_handlers.next;
  while (io_write_handler != &io_write_handlers) {
    io_write_handler->prev->next = io_write_handler->next;
    io_write_handler->next->prev = io_write_handler->prev;
    curr = io_write_handler;
    io_write_handler = io_write_handler->next;
    delete [] curr->handler_name;
    delete curr;
  }

  bx_virt_timer.setup();
  bx_slowdown_timer.exit();

  PLUG_unload_plugin(pit);
  PLUG_unload_plugin(cmos);
  PLUG_unload_plugin(dma);
  PLUG_unload_plugin(pic);
  PLUG_unload_plugin(vga);
  PLUG_unload_plugin(floppy);
#if BX_SUPPORT_PCI
  PLUG_unload_plugin(pci);
  PLUG_unload_plugin(pci2isa);
#endif
  bx_unload_plugins();
  init_stubs();
}

Bit32u bx_devices_c::read_handler(void *this_ptr, Bit32u address, unsigned io_len)
{
#if !BX_USE_DEV_SMF
  bx_devices_c *class_ptr = (bx_devices_c *) this_ptr;
  return class_ptr->port92_read(address, io_len);
}

Bit32u bx_devices_c::port92_read(Bit32u address, unsigned io_len)
{
#else
  UNUSED(this_ptr);
#endif  // !BX_USE_DEV_SMF

  BX_DEBUG(("port92h read partially supported!!!"));
  BX_DEBUG(("  returning %02x", (unsigned) (BX_GET_ENABLE_A20() << 1)));
  return(BX_GET_ENABLE_A20() << 1);
}

void bx_devices_c::write_handler(void *this_ptr, Bit32u address, Bit32u value, unsigned io_len)
{
#if !BX_USE_DEV_SMF
  bx_devices_c *class_ptr = (bx_devices_c *) this_ptr;
  class_ptr->port92_write(address, value, io_len);
}

void bx_devices_c::port92_write(Bit32u address, Bit32u value, unsigned io_len)
{
#else
  UNUSED(this_ptr);
#endif  // !BX_USE_DEV_SMF

  BX_DEBUG(("port92h write of %02x partially supported!!!", (unsigned) value));
  BX_DEBUG(("A20: set_enable_a20() called"));
  BX_SET_ENABLE_A20((value & 0x02) >> 1);
  BX_DEBUG(("A20: now %u", (unsigned) BX_GET_ENABLE_A20()));
  if (value & 0x01) { /* high speed reset */
    BX_INFO(("iowrite to port0x92 : reset resquested"));
    bx_pc_system.Reset(BX_RESET_SOFTWARE);
  }
}

// This defines the builtin default read handler,
// so Bochs does not segfault if unmapped is not loaded
Bit32u bx_devices_c::default_read_handler(void *this_ptr, Bit32u address, unsigned io_len)
{
  UNUSED(this_ptr);
  return 0xffffffff;
}

// This defines the builtin default write handler,
// so Bochs does not segfault if unmapped is not loaded
void bx_devices_c::default_write_handler(void *this_ptr, Bit32u address, Bit32u value, unsigned io_len)
{
  UNUSED(this_ptr);
}

void bx_devices_c::timer_handler(void *this_ptr)
{
  bx_devices_c *class_ptr = (bx_devices_c *) this_ptr;
  class_ptr->timer();
}

void bx_devices_c::timer()
{
  // separate calls to bx_gui->handle_events from the keyboard code.
  {
    static int multiple=0;
    if (++multiple==10)
    {
      multiple=0;
      SIM->periodic();
      if (! bx_pc_system.kill_bochs_request)
	bx_gui->handle_events();
    }
  }
}

bx_bool bx_devices_c::register_irq(unsigned irq, const char *name)
{
  if (irq >= BX_MAX_IRQS) {
    BX_PANIC(("IO device %s registered with IRQ=%d above %u",
             name, irq, (unsigned) BX_MAX_IRQS-1));
    return 0;
  }
  if (irq_handler_name[irq]) {
    BX_PANIC(("IRQ %u conflict, %s with %s", irq, irq_handler_name[irq], name));
    return 0;
  }
  irq_handler_name[irq] = new char[strlen(name)+1];
  strcpy(irq_handler_name[irq], name);
  return 1;
}

bx_bool bx_devices_c::unregister_irq(unsigned irq, const char *name)
{
  if (irq >= BX_MAX_IRQS) {
    BX_PANIC(("IO device %s tried to unregister IRQ %d above %u",
             name, irq, (unsigned) BX_MAX_IRQS-1));
    return 0;
  }
  if (!irq_handler_name[irq]) {
    BX_INFO(("IO device %s tried to unregister IRQ %d, not registered",
	      name, irq));
    return 0;
  }

  if (strcmp(irq_handler_name[irq], name)) {
    BX_INFO(("IRQ %u not registered to %s but to %s", irq,
      name, irq_handler_name[irq]));
    return 0;
  }
  delete [] irq_handler_name[irq];
  irq_handler_name[irq] = NULL;
  return 1;
}

bx_bool bx_devices_c::register_io_read_handler(void *this_ptr, bx_read_handler_t f,
                                               Bit32u addr, const char *name, Bit8u mask)
{
  addr &= 0xffff;

  if (!f)
    return 0;

  /* first check if the port already has a handlers != the default handler */
  if (read_port_to_handler[addr] &&
      read_port_to_handler[addr] != &io_read_handlers) { // the default
    BX_ERROR(("IO device address conflict(read) at IO address %Xh",
              (unsigned) addr));
    BX_ERROR(("  conflicting devices: %s & %s",
              read_port_to_handler[addr]->handler_name, name));
    return 0;
  }

  /* first find existing handle for function or create new one */
  struct io_handler_struct *curr = &io_read_handlers;
  struct io_handler_struct *io_read_handler = NULL;
  do {
    if (curr->funct == f &&
        curr->mask == mask &&
        curr->this_ptr == this_ptr &&
        !strcmp(curr->handler_name, name)) { // really want the same name too
      io_read_handler = curr;
      break;
    }
    curr = curr->next;
  } while (curr->next != &io_read_handlers);

  if (!io_read_handler) {
    io_read_handler = new struct io_handler_struct;
    io_read_handler->funct = (void *)f;
    io_read_handler->this_ptr = this_ptr;
    io_read_handler->handler_name = new char[strlen(name)+1];
    strcpy(io_read_handler->handler_name, name);
    io_read_handler->mask = mask;
    io_read_handler->usage_count = 0;
    // add the handler to the double linked list of handlers
    io_read_handlers.prev->next = io_read_handler;
    io_read_handler->next = &io_read_handlers;
    io_read_handler->prev = io_read_handlers.prev;
    io_read_handlers.prev = io_read_handler;
  }

  io_read_handler->usage_count++;
  read_port_to_handler[addr] = io_read_handler;
  return 1; // address mapped successfully
}

bx_bool bx_devices_c::register_io_write_handler(void *this_ptr, bx_write_handler_t f,
                                                Bit32u addr, const char *name, Bit8u mask)
{
  addr &= 0xffff;

  if (!f)
    return 0;

  /* first check if the port already has a handlers != the default handler */
  if (write_port_to_handler[addr] &&
      write_port_to_handler[addr] != &io_write_handlers) { // the default
    BX_ERROR(("IO device address conflict(write) at IO address %Xh",
              (unsigned) addr));
    BX_ERROR(("  conflicting devices: %s & %s",
              write_port_to_handler[addr]->handler_name, name));
    return 0;
  }

  /* first find existing handle for function or create new one */
  struct io_handler_struct *curr = &io_write_handlers;
  struct io_handler_struct *io_write_handler = NULL;
  do {
    if (curr->funct == f &&
        curr->mask == mask &&
        curr->this_ptr == this_ptr &&
        !strcmp(curr->handler_name, name)) { // really want the same name too
      io_write_handler = curr;
      break;
    }
    curr = curr->next;
  } while (curr->next != &io_write_handlers);

  if (!io_write_handler) {
    io_write_handler = new struct io_handler_struct;
    io_write_handler->funct = (void *)f;
    io_write_handler->this_ptr = this_ptr;
    io_write_handler->handler_name = new char[strlen(name)+1];
    strcpy(io_write_handler->handler_name, name);
    io_write_handler->mask = mask;
    io_write_handler->usage_count = 0;
    // add the handler to the double linked list of handlers
    io_write_handlers.prev->next = io_write_handler;
    io_write_handler->next = &io_write_handlers;
    io_write_handler->prev = io_write_handlers.prev;
    io_write_handlers.prev = io_write_handler;
  }

  io_write_handler->usage_count++;
  write_port_to_handler[addr] = io_write_handler;
  return 1; // address mapped successfully
}

bx_bool bx_devices_c::register_io_read_handler_range(void *this_ptr, bx_read_handler_t f,
                                             Bit32u begin_addr, Bit32u end_addr,
                                             const char *name, Bit8u mask)
{
  Bit32u addr;
  begin_addr &= 0xffff;
  end_addr &= 0xffff;

  if (end_addr < begin_addr) {
    BX_ERROR(("!!! end_addr < begin_addr !!!"));
    return 0;
  }

  if (!f) {
    BX_ERROR(("!!! f == NULL !!!"));
    return 0;
  }

  /* first check if the port already has a handlers != the default handler */
  for (addr = begin_addr; addr <= end_addr; addr++)
    if (read_port_to_handler[addr] &&
        read_port_to_handler[addr] != &io_read_handlers) { // the default
      BX_ERROR(("IO device address conflict(read) at IO address %Xh",
                (unsigned) addr));
      BX_ERROR(("  conflicting devices: %s & %s",
                read_port_to_handler[addr]->handler_name, name));
      return 0;
  }

  /* first find existing handle for function or create new one */
  struct io_handler_struct *curr = &io_read_handlers;
  struct io_handler_struct *io_read_handler = NULL;
  do {
    if (curr->funct == f &&
        curr->mask == mask &&
        curr->this_ptr == this_ptr &&
        !strcmp(curr->handler_name, name)) {
      io_read_handler = curr;
      break;
    }
    curr = curr->next;
  } while (curr->next != &io_read_handlers);

  if (!io_read_handler) {
    io_read_handler = new struct io_handler_struct;
    io_read_handler->funct = (void *)f;
    io_read_handler->this_ptr = this_ptr;
    io_read_handler->handler_name = new char[strlen(name)+1];
    strcpy(io_read_handler->handler_name, name);
    io_read_handler->mask = mask;
    io_read_handler->usage_count = 0;
    // add the handler to the double linked list of handlers
    io_read_handlers.prev->next = io_read_handler;
    io_read_handler->next = &io_read_handlers;
    io_read_handler->prev = io_read_handlers.prev;
    io_read_handlers.prev = io_read_handler;
  }

  io_read_handler->usage_count += end_addr - begin_addr + 1;
  for (addr = begin_addr; addr <= end_addr; addr++)
	  read_port_to_handler[addr] = io_read_handler;
  return 1; // address mapped successfully
}

bx_bool bx_devices_c::register_io_write_handler_range(void *this_ptr, bx_write_handler_t f,
                                              Bit32u begin_addr, Bit32u end_addr,
                                              const char *name, Bit8u mask)
{
  Bit32u addr;
  begin_addr &= 0xffff;
  end_addr &= 0xffff;

  if (end_addr < begin_addr) {
    BX_ERROR(("!!! end_addr < begin_addr !!!"));
    return 0;
  }

  if (!f) {
    BX_ERROR(("!!! f == NULL !!!"));
    return 0;
  }

  /* first check if the port already has a handlers != the default handler */
  for (addr = begin_addr; addr <= end_addr; addr++)
    if (write_port_to_handler[addr] &&
        write_port_to_handler[addr] != &io_write_handlers) { // the default
      BX_ERROR(("IO device address conflict(read) at IO address %Xh",
                (unsigned) addr));
      BX_ERROR(("  conflicting devices: %s & %s",
                write_port_to_handler[addr]->handler_name, name));
      return 0;
    }

  /* first find existing handle for function or create new one */
  struct io_handler_struct *curr = &io_write_handlers;
  struct io_handler_struct *io_write_handler = NULL;
  do {
    if (curr->funct == f &&
        curr->mask == mask &&
        curr->this_ptr == this_ptr &&
        !strcmp(curr->handler_name, name)) {
      io_write_handler = curr;
      break;
    }
    curr = curr->next;
  } while (curr->next != &io_write_handlers);

  if (!io_write_handler) {
    io_write_handler = new struct io_handler_struct;
    io_write_handler->funct = (void *)f;
    io_write_handler->this_ptr = this_ptr;
    io_write_handler->handler_name = new char[strlen(name)+1];
    strcpy(io_write_handler->handler_name, name);
    io_write_handler->mask = mask;
    io_write_handler->usage_count = 0;
    // add the handler to the double linked list of handlers
    io_write_handlers.prev->next = io_write_handler;
    io_write_handler->next = &io_write_handlers;
    io_write_handler->prev = io_write_handlers.prev;
    io_write_handlers.prev = io_write_handler;
  }

  io_write_handler->usage_count += end_addr - begin_addr + 1;
  for (addr = begin_addr; addr <= end_addr; addr++)
	  write_port_to_handler[addr] = io_write_handler;
  return 1; // address mapped successfully
}


// Registration of default handlers (mainly be the unmapped device)
bx_bool bx_devices_c::register_default_io_read_handler(void *this_ptr, bx_read_handler_t f,
                                               const char *name, Bit8u mask)
{
  io_read_handlers.funct = (void *)f;
  io_read_handlers.this_ptr = this_ptr;
  if (io_read_handlers.handler_name) {
    delete [] io_read_handlers.handler_name;
  }
  io_read_handlers.handler_name = new char[strlen(name)+1];
  strcpy(io_read_handlers.handler_name, name);
  io_read_handlers.mask = mask;

  return 1;
}

bx_bool bx_devices_c::register_default_io_write_handler(void *this_ptr, bx_write_handler_t f,
                                                const char *name, Bit8u mask)
{
  io_write_handlers.funct = (void *)f;
  io_write_handlers.this_ptr = this_ptr;
  if (io_write_handlers.handler_name) {
    delete [] io_write_handlers.handler_name;
  }
  io_write_handlers.handler_name = new char[strlen(name)+1];
  strcpy(io_write_handlers.handler_name, name);
  io_write_handlers.mask = mask;

  return 1;
}

bx_bool bx_devices_c::unregister_io_read_handler(void *this_ptr, bx_read_handler_t f,
                                         Bit32u addr, Bit8u mask)
{
  addr &= 0xffff;

  struct io_handler_struct *io_read_handler = read_port_to_handler[addr];

  //BX_INFO(("Unregistering I/O read handler at %#x", addr));

  if (!io_read_handler) {
    BX_ERROR((">>> NO IO_READ_HANDLER <<<"));
    return 0;
  }

  if (io_read_handler == &io_read_handlers) {
    BX_ERROR((">>> CANNOT UNREGISTER THE DEFAULT IO_READ_HANDLER <<<"));
    return 0; // cannot unregister the default handler
  }

  if (io_read_handler->funct != f) {
    BX_ERROR((">>> NOT THE SAME IO_READ_HANDLER FUNC <<<"));
    return 0;
  }

  if (io_read_handler->this_ptr != this_ptr) {
    BX_ERROR((">>> NOT THE SAME IO_READ_HANDLER THIS_PTR <<<"));
    return 0;
  }

  if (io_read_handler->mask != mask) {
    BX_ERROR((">>> NOT THE SAME IO_READ_HANDLER MASK <<<"));
    return 0;
  }

  read_port_to_handler[addr] = &io_read_handlers; // reset to default
  io_read_handler->usage_count--;

  if (!io_read_handler->usage_count) { // kill this handler entry
    io_read_handler->prev->next = io_read_handler->next;
    io_read_handler->next->prev = io_read_handler->prev;
    delete [] io_read_handler->handler_name;
    delete io_read_handler;
  }
  return 1;
}

bx_bool bx_devices_c::unregister_io_write_handler(void *this_ptr, bx_write_handler_t f,
                                          Bit32u addr, Bit8u mask)
{
  addr &= 0xffff;

  struct io_handler_struct *io_write_handler = write_port_to_handler[addr];

  if (!io_write_handler)
    return 0;

  if (io_write_handler == &io_write_handlers)
    return 0; // cannot unregister the default handler

  if (io_write_handler->funct != f)
    return 0;

  if (io_write_handler->this_ptr != this_ptr)
    return 0;

  if (io_write_handler->mask != mask)
    return 0;

  write_port_to_handler[addr] = &io_write_handlers; // reset to default
  io_write_handler->usage_count--;

  if (!io_write_handler->usage_count) { // kill this handler entry
    io_write_handler->prev->next = io_write_handler->next;
    io_write_handler->next->prev = io_write_handler->prev;
    delete [] io_write_handler->handler_name;
    delete io_write_handler;
  }
  return 1;
}

bx_bool bx_devices_c::unregister_io_read_handler_range(void *this_ptr, bx_read_handler_t f,
                                               Bit32u begin, Bit32u end, Bit8u mask)
{
  begin &= 0xffff;
  end &= 0xffff;
  Bit32u addr;
  bx_bool ret = 1;

  /*
   * the easy way this time
   */
  for (addr = begin; addr <= end; addr++)
    if (!unregister_io_read_handler(this_ptr, f, addr, mask))
      ret = 0;

  return ret;
}

bx_bool bx_devices_c::unregister_io_write_handler_range(void *this_ptr, bx_write_handler_t f,
                                                Bit32u begin, Bit32u end, Bit8u mask)
{
  begin &= 0xffff;
  end &= 0xffff;
  Bit32u addr;
  bx_bool ret = 1;

  /*
   * the easy way this time
   */
  for (addr = begin; addr <= end; addr++)
    if (!unregister_io_write_handler(this_ptr, f, addr, mask))
      ret = 0;

  return ret;
}


/*
 * Read a byte of data from the IO memory address space
 */

  Bit32u BX_CPP_AttrRegparmN(2)
bx_devices_c::inp(Bit16u addr, unsigned io_len)
{
  struct io_handler_struct *io_read_handler;
  Bit32u ret;

  BX_INSTR_INP(addr, io_len);

  io_read_handler = read_port_to_handler[addr];
  if (io_read_handler->mask & io_len) {
	ret = ((bx_read_handler_t)io_read_handler->funct)(io_read_handler->this_ptr, (Bit32u)addr, io_len);
  } else {
    switch (io_len) {
      case 1: ret = 0xff; break;
      case 2: ret = 0xffff; break;
      default: ret = 0xffffffff; break;
    }
    if (addr != 0x0cf8) { // don't flood the logfile when probing PCI
      BX_ERROR(("read from port 0x%04x with len %d returns 0x%x", addr, io_len, ret));
    }
  }

  BX_INSTR_INP2(addr, io_len, ret);
  BX_DBG_IO_REPORT(addr, io_len, BX_READ, ret);

  return(ret);
}


/*
 * Write a byte of data to the IO memory address space.
 */

  void BX_CPP_AttrRegparmN(3)
bx_devices_c::outp(Bit16u addr, Bit32u value, unsigned io_len)
{
  struct io_handler_struct *io_write_handler;

  BX_INSTR_OUTP(addr, io_len, value);
  BX_DBG_IO_REPORT(addr, io_len, BX_WRITE, value);

  io_write_handler = write_port_to_handler[addr];
  if (io_write_handler->mask & io_len) {
	((bx_write_handler_t)io_write_handler->funct)(io_write_handler->this_ptr, (Bit32u)addr, value, io_len);
  } else if (addr != 0x0cf8) { // don't flood the logfile when probing PCI
    BX_ERROR(("write to port 0x%04x with len %d ignored", addr, io_len));
  }
}

bx_bool bx_devices_c::is_harddrv_enabled(void)
{
  char pname[24];

  for (int i=0; i<BX_MAX_ATA_CHANNEL; i++) {
    sprintf(pname, "ata.%d.resources.enabled", i);
    if (SIM->get_param_bool(pname)->get())
      return 1;
  }
  return 0;
}

bx_bool bx_devices_c::is_serial_enabled(void)
{
  char pname[24];

  for (int i=0; i<BX_N_SERIAL_PORTS; i++) {
    sprintf(pname, "ports.serial.%d.enabled", i+1);
    if (SIM->get_param_bool(pname)->get())
      return 1;
  }
  return 0;
}

bx_bool bx_devices_c::is_parallel_enabled(void)
{
  char pname[26];

  for (int i=0; i<BX_N_PARALLEL_PORTS; i++) {
    sprintf(pname, "ports.parallel.%d.enabled", i+1);
    if (SIM->get_param_bool(pname)->get())
      return 1;
  }
  return 0;
}

bx_bool bx_devices_c::is_usb_ohci_enabled(void)
{
  if (SIM->get_param_bool(BXPN_OHCI_ENABLED)->get()) {
    return 1;
  }
  return 0;
}

bx_bool bx_devices_c::is_usb_uhci_enabled(void)
{
  if (SIM->get_param_bool(BXPN_UHCI_ENABLED)->get()) {
    return 1;
  }
  return 0;
}

// removable keyboard/mouse registration
void bx_devices_c::register_removable_keyboard(void *dev, bx_keyb_enq_t keyb_enq)
{
  if (bx_keyboard.dev == NULL) {
    bx_keyboard.dev = dev;
    bx_keyboard.enq_event = keyb_enq;
  }
}

void bx_devices_c::unregister_removable_keyboard(void *dev)
{
  if (dev == bx_keyboard.dev) {
    bx_keyboard.dev = NULL;
    bx_keyboard.enq_event = NULL;
  }
}

void bx_devices_c::register_default_mouse(void *dev, bx_mouse_enq_t mouse_enq,
                                          bx_mouse_enabled_changed_t mouse_enabled_changed)
{
  if (bx_mouse[0].dev == NULL) {
    bx_mouse[0].dev = dev;
    bx_mouse[0].enq_event = mouse_enq;
    bx_mouse[0].enabled_changed = mouse_enabled_changed;
  }
}

void bx_devices_c::register_removable_mouse(void *dev, bx_mouse_enq_t mouse_enq,
                                            bx_mouse_enabled_changed_t mouse_enabled_changed)
{
  if (bx_mouse[1].dev == NULL) {
    bx_mouse[1].dev = dev;
    bx_mouse[1].enq_event = mouse_enq;
    bx_mouse[1].enabled_changed = mouse_enabled_changed;
  }
}

void bx_devices_c::unregister_removable_mouse(void *dev)
{
  if (dev == bx_mouse[1].dev) {
    bx_mouse[1].dev = NULL;
    bx_mouse[1].enq_event = NULL;
    bx_mouse[1].enabled_changed = NULL;
  }
}

bx_bool bx_devices_c::optional_key_enq(Bit8u *scan_code)
{
  if (bx_keyboard.dev != NULL) {
    return bx_keyboard.enq_event(bx_keyboard.dev, scan_code);
  }
  return 0;
}

// common mouse device handlers
void bx_devices_c::mouse_enabled_changed(bx_bool enabled)
{
  mouse_captured = enabled;

  if ((bx_mouse[1].dev != NULL) && (bx_mouse[1].enabled_changed != NULL)) {
    bx_mouse[1].enabled_changed(bx_mouse[1].dev, enabled);
    return;
  }

  if ((bx_mouse[0].dev != NULL) && (bx_mouse[0].enabled_changed != NULL)) {
    bx_mouse[0].enabled_changed(bx_mouse[0].dev, enabled);
  }
}

void bx_devices_c::mouse_motion(int delta_x, int delta_y, int delta_z, unsigned button_state)
{
  // If mouse events are disabled on the GUI headerbar, don't
  // generate any mouse data
  if (!mouse_captured)
    return;

  // if a removable mouse is connected, redirect mouse data to the device
  if (bx_mouse[1].dev != NULL) {
    bx_mouse[1].enq_event(bx_mouse[1].dev, delta_x, delta_y, delta_z, button_state);
    return;
  }

  // if a mouse is connected, direct mouse data to the device
  if (bx_mouse[0].dev != NULL) {
    bx_mouse[0].enq_event(bx_mouse[0].dev, delta_x, delta_y, delta_z, button_state);
  }
}

void bx_pci_device_stub_c::register_pci_state(bx_list_c *list, Bit8u *pci_conf)
{
  char name[6];

  bx_list_c *pci = new bx_list_c(list, "pci_conf", 256);
  for (unsigned i=0; i<256; i++) {
    sprintf(name, "0x%02x", i);
    new bx_shadow_num_c(pci, name, &pci_conf[i], BASE_HEX);
  }
}
