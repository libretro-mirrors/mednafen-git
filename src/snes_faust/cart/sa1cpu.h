/******************************************************************************/
/* Mednafen Fast SNES Emulation Module                                        */
/******************************************************************************/
/* sa1cpu.h:
**  Copyright (C) 2019 Mednafen Team
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __MDFN_SNES_FAUST_CART_SA1CPU_H
#define __MDFN_SNES_FAUST_CART_SA1CPU_H

namespace MDFN_IEN_SNES_FAUST
{
namespace SA1CPU
{
//
//
class CPU65816;

struct CPU_Misc
{
 uint32 timestamp;
 uint32 next_event_ts;
 uint32 running_mask;
 uint32 PIN_Delay;

 enum
 {
  HALTED_NOT = 0,
  HALTED_WAI = 1 << 0,
  HALTED_STP = 1 << 1,
  HALTED_DMA = 1 << 2
 };

 uint8 halted;
 uint8 mdr;

 uint8 CombinedNIState;
 bool NMILineState;
 bool PrevNMILineState;
 uint8 MultiIRQState;
 enum { IRQNMISuppress = false };

 readfunc ReadFuncs[512 + 256];
 writefunc WriteFuncs[512 + 256];

 // +1 so we can avoid a masking for 16-bit reads/writes(note that this
 // may result in the address passed to the read/write handlers being
 // 0x1000000 instead of 0x000000 in some cases, so code with that in mind.
 uint16 RWIndex[0x8000 + 1];	// (2**24 / 512)
 //
 //
 //
 void RunDMA(void) MDFN_HOT;
 void EventHandler(void) MDFN_HOT;
};

extern CPU_Misc CPUM;

INLINE uint8 CPU_Read(uint32 A)
{
 const size_t i = CPUM.RWIndex[A >> 9];
 uint8 ret = CPUM.ReadFuncs[i ? i : (A & 0x1FF)](A);

 CPUM.mdr = ret;

 return ret;
}

INLINE void CPU_Write(uint32 A, uint8 V)
{
 const size_t i = CPUM.RWIndex[A >> 9];
 CPUM.mdr = V;
 CPUM.WriteFuncs[i ? i : (A & 0x1FF)](A, V);
}

INLINE void CPU_IO(void)
{
 CPUM.timestamp += 2;
}

INLINE void CPU_SetIRQ(bool active)
{
 CPUM.CombinedNIState &= ~0x04;
 CPUM.CombinedNIState |= active ? 0x04 : 0x00;
}

INLINE void CPU_SetNMI(bool active)
{
 if((CPUM.NMILineState ^ active) & active)
  CPUM.CombinedNIState |= 0x01;

 CPUM.NMILineState = active;
}

INLINE void CPU_SetIRQNMISuppress(bool active)
{
 CPUM.CombinedNIState &= ~0x80;
 CPUM.CombinedNIState |= active ? 0x80 : 0x00;
}

void CPU_Init(void) MDFN_COLD;
void CPU_Reset(bool powering_up) MDFN_COLD;
void CPU_StateAction(StateMem* sm, const unsigned load, const bool data_only, const char* sname, const char* sname_core);
void CPU_Run(void) MDFN_HOT;

void CPU_ClearRWFuncs(void) MDFN_COLD;
void CPU_SetRWHandlers(uint32 A1, uint32 A2, readfunc read_handler, writefunc write_handler, bool special = false) MDFN_COLD;

INLINE void CPU_Exit(void)
{
 CPUM.running_mask = 0;
 CPUM.next_event_ts = 0;
}
//
//
}
}
#endif
