/******************************************************************************/
/* Mednafen Fast SNES Emulation Module                                        */
/******************************************************************************/
/* sdd1.cpp:
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

#include "common.h"
#include "sdd1.h"

namespace MDFN_IEN_SNES_FAUST
{

static uint8 ROMBank[4];
static uintptr_t ROMBankPtr[4];

static INLINE void RecalcROMBankPtr(size_t rbi)
{
 ROMBankPtr[rbi] = (uintptr_t)&Cart.ROM[(ROMBank[rbi] & 0x7) << 20] - (0xC00000 + (rbi << 20));
}

template<signed cyc>
static DEFREAD(MainCPU_ReadFixedROM)
{
 if(MDFN_LIKELY(!DBG_InHLRead))
 {
  CPUM.timestamp += (cyc >= 0) ? cyc : (MemSelect ? MEMCYC_FAST : MEMCYC_SLOW);
 }
 //
 //
 return Cart.ROM[A & 0x7FFF];
}

template<signed cyc>
static DEFREAD(MainCPU_ReadBankedROM)
{
 if(MDFN_LIKELY(!DBG_InHLRead))
 {
  CPUM.timestamp += (cyc >= 0) ? cyc : (MemSelect ? MEMCYC_FAST : MEMCYC_SLOW);
 }
 //
 //
 return *(uint8*)(ROMBankPtr[(A >> 20) & 0x3] + A);
}

template<signed cyc, unsigned w>
static DEFWRITE(MainCPU_WriteIO)
{
 CPUM.timestamp += (cyc >= 0) ? cyc : (MemSelect ? MEMCYC_FAST : MEMCYC_SLOW);
 //
 //
 SNES_DBG("[SDD1] IO write 0x%06x 0x%02x\n", A, V);

 switch(w)
 {
  case 0x4:
  case 0x5:
  case 0x6:
  case 0x7:
	{
	 size_t rbi = w & 0x3;
	 ROMBank[rbi] = V;
	 RecalcROMBankPtr(rbi);
	}
	break;
 }
}

template<signed cyc, unsigned w>
static DEFREAD(MainCPU_ReadIO)
{
 if(MDFN_LIKELY(!DBG_InHLRead))
 {
  CPUM.timestamp += (cyc >= 0) ? cyc : (MemSelect ? MEMCYC_FAST : MEMCYC_SLOW);
 }
 //
 //
 uint8 ret = 0;

 SNES_DBG("[SDD1] IO read 0x%06x\n", A);

 switch(w)
 {
  case 0x4:
  case 0x5:
  case 0x6:
  case 0x7:
	ret = ROMBank[w & 0x3];  
	break;
 }

 return ret;
}

static MDFN_COLD void Reset(bool powering_up)
{
 for(size_t rbi = 0; rbi < 4; rbi++)
 {
  ROMBank[rbi] = 0;
  RecalcROMBankPtr(rbi);
 }
}

static void StateAction(StateMem* sm, const unsigned load, const bool data_only)
{
 SFORMAT StateRegs[] =
 {
  SFVAR(ROMBank),
  SFEND
 };

 MDFNSS_StateAction(sm, load, data_only, StateRegs, "SDD1");

 if(load)
 {
  for(size_t rbi = 0; rbi < 4; rbi++)
   RecalcROMBankPtr(rbi);
 }
}

void CART_SDD1_Init(const int32 master_clock)
{
 puts("SDD1");
 Set_A_Handlers(0x008000, 0x00FFFF, MainCPU_ReadFixedROM<MEMCYC_SLOW>, OBWrite_SLOW);
 Set_A_Handlers(0xC00000, 0xFFFFFF, MainCPU_ReadBankedROM<-1>, OBWrite_VAR);

 for(unsigned bank = 0x00; bank < 0x100; bank++)
 {
  if(!(bank & 0x40))
  {
   #define SHP(ta) Set_A_Handlers((bank << 16) | (0x4800 + ta), MainCPU_ReadIO<MEMCYC_FAST, ta>, MainCPU_WriteIO<MEMCYC_FAST, ta>);
   SHP(0x0)
   SHP(0x1)
   SHP(0x2)
   SHP(0x3)
   SHP(0x4)
   SHP(0x5)
   SHP(0x6)
   SHP(0x7)
   #undef SHP
  }
 }
 //
 //
 //
 Cart.Reset = Reset;
 Cart.StateAction = StateAction;
}

}
