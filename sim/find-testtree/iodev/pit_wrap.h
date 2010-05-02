/////////////////////////////////////////////////////////////////////////
// $Id: pit_wrap.h,v 1.32 2009/02/08 09:05:52 vruppert Exp $
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
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

#ifndef _BX_PIT_WRAP_H
#define _BX_PIT_WRAP_H

#include "bochs.h"
#include "pit82c54.h"

#if BX_USE_PIT_SMF
#  define BX_PIT_SMF  static
#  define BX_PIT_THIS thePit->
#else
#  define BX_PIT_SMF
#  define BX_PIT_THIS this->
#endif

class bx_pit_c : public bx_devmodel_c {
public:
  bx_pit_c();
  virtual ~bx_pit_c() {}
  virtual void init(void);
  virtual void reset(unsigned type);
  virtual void register_state(void);

private:
  static Bit32u read_handler(void *this_ptr, Bit32u address, unsigned io_len);
  static void   write_handler(void *this_ptr, Bit32u address, Bit32u value, unsigned io_len);
#if !BX_USE_PIT_SMF
  Bit32u   read(Bit32u addr, unsigned len);
  void write(Bit32u addr, Bit32u Value, unsigned len);
#endif

  struct s_type {
    pit_82C54 timer;
    Bit8u   speaker_data_on;
    bx_bool refresh_clock_div2;
    Bit64u last_usec;
    Bit32u last_next_event_time;
    Bit64u total_ticks;
    Bit64u total_usec;
    int  timer_handle[3];
  } s;

  static void timer_handler(void *this_ptr);
  BX_PIT_SMF void handle_timer();
  BX_PIT_SMF bx_bool periodic(Bit32u usec_delta);

  BX_PIT_SMF void  irq_handler(bx_bool value);

  BX_PIT_SMF Bit16u get_timer(int Timer);
};

#endif  // #ifndef _BX_PIT_WRAP_H
