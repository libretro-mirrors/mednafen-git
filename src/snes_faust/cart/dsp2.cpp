/******************************************************************************/
/* Mednafen Fast SNES Emulation Module                                        */
/******************************************************************************/
/* dsp2.cpp:
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
#include "dsp2.h"

namespace MDFN_IEN_SNES_FAUST
{

template<signed cyc>
static DEFREAD(MainCPU_ReadDR)
{
 if(MDFN_LIKELY(!DBG_InHLRead))
 {
  CPUM.timestamp += (cyc >= 0) ? cyc : (MemSelect ? MEMCYC_FAST : MEMCYC_SLOW);
 }
 //
 //
 return 0;
}

template<signed cyc>
static DEFWRITE(MainCPU_WriteDR)
{
 CPUM.timestamp += (cyc >= 0) ? cyc : (MemSelect ? MEMCYC_FAST : MEMCYC_SLOW);
 //
 //
 SNES_DBG("[DSP2] WriteDR: %02x\n", V);
}

template<signed cyc>
static DEFREAD(MainCPU_ReadSR)
{
 if(MDFN_LIKELY(!DBG_InHLRead))
 {
  CPUM.timestamp += (cyc >= 0) ? cyc : (MemSelect ? MEMCYC_FAST : MEMCYC_SLOW);
 }
 //
 //
 //abort();
 return 0x80;
}

void CART_DSP2_Init(const int32 master_clock)
{
 for(unsigned bank = 0x00; bank < 0x100; bank++)
 {
  if(bank >= 0x20 && bank <= 0x3F)
  {
   Set_A_Handlers((bank << 16) | 0x8000, (bank << 16) | 0xBFFF, MainCPU_ReadDR<MEMCYC_SLOW>, MainCPU_WriteDR<MEMCYC_SLOW>);
   Set_A_Handlers((bank << 16) | 0xC000, (bank << 16) | 0xFFFF, MainCPU_ReadSR<MEMCYC_SLOW>, OBWrite_SLOW);
  }
 }
}

}
