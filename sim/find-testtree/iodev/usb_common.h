/////////////////////////////////////////////////////////////////////////
// $Id: usb_common.h,v 1.10 2009/04/10 20:26:14 vruppert Exp $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2009  Benjamin D Lunt (fys at frontiernet net)
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


#ifndef BX_IODEV_USB_COMMON_H
#define BX_IODEV_USB_COMMON_H

#define USB_TOKEN_IN    0x69
#define USB_TOKEN_OUT   0xE1
#define USB_TOKEN_SETUP 0x2D

#define USB_MSG_ATTACH   0x100
#define USB_MSG_DETACH   0x101
#define USB_MSG_RESET    0x102

#define USB_RET_NODEV  (-1)
#define USB_RET_NAK    (-2)
#define USB_RET_STALL  (-3)
#define USB_RET_BABBLE (-4)
#define USB_RET_ASYNC  (-5)

#define USB_SPEED_LOW   0
#define USB_SPEED_FULL  1
#define USB_SPEED_HIGH  2

#define USB_STATE_NOTATTACHED 0
#define USB_STATE_ATTACHED    1
//#define USB_STATE_POWERED     2
#define USB_STATE_DEFAULT     3
#define USB_STATE_ADDRESS     4
#define USB_STATE_CONFIGURED  5
#define USB_STATE_SUSPENDED   6

#define USB_DIR_OUT  0
#define USB_DIR_IN   0x80

#define USB_TYPE_MASK			(0x03 << 5)
#define USB_TYPE_STANDARD		(0x00 << 5)
#define USB_TYPE_CLASS			(0x01 << 5)
#define USB_TYPE_VENDOR			(0x02 << 5)
#define USB_TYPE_RESERVED		(0x03 << 5)

#define USB_RECIP_MASK			0x1f
#define USB_RECIP_DEVICE		0x00
#define USB_RECIP_INTERFACE		0x01
#define USB_RECIP_ENDPOINT		0x02
#define USB_RECIP_OTHER			0x03

#define DeviceRequest ((USB_DIR_IN|USB_TYPE_STANDARD|USB_RECIP_DEVICE)<<8)
#define DeviceOutRequest ((USB_DIR_OUT|USB_TYPE_STANDARD|USB_RECIP_DEVICE)<<8)
#define InterfaceRequest \
        ((USB_DIR_IN|USB_TYPE_STANDARD|USB_RECIP_INTERFACE)<<8)
#define InterfaceInClassRequest \
        ((USB_DIR_IN|USB_TYPE_CLASS|USB_RECIP_INTERFACE)<<8)
#define InterfaceOutRequest \
        ((USB_DIR_OUT|USB_TYPE_STANDARD|USB_RECIP_INTERFACE)<<8)
#define InterfaceOutClassRequest \
        ((USB_DIR_OUT|USB_TYPE_CLASS|USB_RECIP_INTERFACE)<<8)
#define EndpointRequest ((USB_DIR_IN|USB_TYPE_STANDARD|USB_RECIP_ENDPOINT)<<8)
#define EndpointOutRequest \
        ((USB_DIR_OUT|USB_TYPE_STANDARD|USB_RECIP_ENDPOINT)<<8)

#define USB_REQ_GET_STATUS		0x00
#define USB_REQ_CLEAR_FEATURE		0x01
#define USB_REQ_SET_FEATURE		0x03
#define USB_REQ_SET_ADDRESS		0x05
#define USB_REQ_GET_DESCRIPTOR		0x06
#define USB_REQ_SET_DESCRIPTOR		0x07
#define USB_REQ_GET_CONFIGURATION	0x08
#define USB_REQ_SET_CONFIGURATION	0x09
#define USB_REQ_GET_INTERFACE		0x0A
#define USB_REQ_SET_INTERFACE		0x0B
#define USB_REQ_SYNCH_FRAME		0x0C

#define USB_DEVICE_SELF_POWERED		0
#define USB_DEVICE_REMOTE_WAKEUP	1

// USB 1.1
#define USB_DT_DEVICE			0x01
#define USB_DT_CONFIG			0x02
#define USB_DT_STRING			0x03
#define USB_DT_INTERFACE		0x04
#define USB_DT_ENDPOINT			0x05
// USB 2.0
#define USB_DT_DEVICE_QUALIFIER         0x06
#define USB_DT_OTHER_SPEED_CONFIG       0x07
#define USB_DT_INTERFACE_POWER          0x08

class usb_device_c;

struct USBPacket {
  int pid;
  Bit8u devaddr;
  Bit8u devep;
  Bit8u *data;
  int len;
  usb_device_c *dev;
};

enum usbdev_type {
  USB_DEV_TYPE_NONE=0,
  USB_DEV_TYPE_MOUSE,
  USB_DEV_TYPE_TABLET,
  USB_DEV_TYPE_KEYPAD,
  USB_DEV_TYPE_DISK,
  USB_DEV_TYPE_CDROM,
  USB_DEV_TYPE_HUB
};

usbdev_type usb_init_device(const char *devname, logfunctions *hc, usb_device_c **device);


class usb_device_c : public logfunctions {
public:
  usb_device_c(void);
  virtual ~usb_device_c(void) {}

  virtual int handle_packet(USBPacket *p);
  virtual void handle_reset() {}
  virtual int handle_control(int request, int value, int index, int length, Bit8u *data) {return 0;}
  virtual int handle_data(USBPacket *p) {return 0;}
  void register_state(bx_list_c *parent);
  virtual void register_state_specific(bx_list_c *parent) {}
  virtual void after_restore_state() {}
  virtual void cancel_packet(USBPacket *p) {}

  bx_bool get_connected() {return d.connected;}
  usbdev_type get_type() {return d.type;}
  int get_speed() {return d.speed;}
  Bit8u get_address() {return d.addr;}

  void usb_send_msg(int msg);

protected:
  struct {
    enum usbdev_type type;
    bx_bool connected;
    int speed;
    Bit8u addr;
    Bit8u config;
    char devname[32];

    int state;
    Bit8u setup_buf[8];
    Bit8u data_buf[1024];
    int remote_wakeup;
    int setup_state;
    int setup_len;
    int setup_index;
    bx_bool stall;
  } d;

  void usb_dump_packet(Bit8u *data, unsigned size);
  int set_usb_string(Bit8u *buf, const char *str);
};

static inline void usb_defer_packet(USBPacket *p, usb_device_c *dev)
{
  p->dev = dev;
}

static inline void usb_cancel_packet(USBPacket *p)
{
  p->dev->cancel_packet(p);
}

#endif
