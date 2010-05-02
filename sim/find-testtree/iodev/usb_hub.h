/////////////////////////////////////////////////////////////////////////
// $Id: usb_hub.h,v 1.5 2009/04/09 17:32:53 vruppert Exp $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2009  Volker Ruppert
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
/////////////////////////////////////////////////////////////////////////

// USB hub emulation support ported from the Qemu project

#ifndef BX_IODEV_USB_HUB_H
#define BX_IODEV_USB_HUB_H


// number of ports defined in bochs.h

class usb_hub_device_c : public usb_device_c {
public:
  usb_hub_device_c(Bit8u ports);
  virtual ~usb_hub_device_c(void);

  virtual int handle_packet(USBPacket *p);
  virtual void handle_reset();
  virtual int handle_control(int request, int value, int index, int length, Bit8u *data);
  virtual int handle_data(USBPacket *p);
  virtual void register_state_specific(bx_list_c *parent);
  virtual void after_restore_state();

private:
  struct {
    Bit8u n_ports;
    bx_list_c *config;
    bx_list_c *state;
    char serial_number[16];
    struct {
      // our data
      usb_device_c *device;  // device connected to this port

      Bit16u PortStatus;
      Bit16u PortChange;
    } usb_port[BX_N_USB_HUB_PORTS];
  } hub;

  int broadcast_packet(USBPacket *p);
  void init_device(Bit8u port, const char *devname);
  void remove_device(Bit8u port);
  void usb_set_connect_status(Bit8u port, int type, bx_bool connected);

  static const char *hub_param_handler(bx_param_string_c *param, int set,
                                       const char *oldval, const char *val, int maxlen);
};

#endif
