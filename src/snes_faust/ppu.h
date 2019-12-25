/******************************************************************************/
/* Mednafen Fast SNES Emulation Module                                        */
/******************************************************************************/
/* ppu.h:
**  Copyright (C) 2015-2019 Mednafen Team
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

#ifndef __MDFN_SNES_FAUST_PPU_H
#define __MDFN_SNES_FAUST_PPU_H

namespace MDFN_IEN_SNES_FAUST
{

void PPU_Init(const bool IsPAL) MDFN_COLD;

enum
{
 PPU_HFILTER_NONE = 0,
 PPU_HFILTER_512,
 PPU_HFILTER_PHR256BLEND,
 PPU_HFILTER_PHR256BLEND_512,
 PPU_HFILTER_512_BLEND
};
void PPU_SetGetVideoParams(MDFNGI* gi, const bool caspect, const unsigned hfilter) MDFN_COLD;
void PPU_Kill(void) MDFN_COLD;
void PPU_Reset(bool powering_up) MDFN_COLD;
void PPU_ResetTS(void);

void PPU_StateAction(StateMem* sm, const unsigned load, const bool data_only);

void PPU_StartFrame(EmulateSpecStruct* espec);

uint32 PPU_UpdateLineIRQ(uint32 timestamp) MDFN_HOT;
uint32 PPU_Update(uint32 timestamp) MDFN_HOT;
//
//
//
uint16 PPU_PeekVRAM(uint32 addr) MDFN_COLD;
void PPU_PokeVRAM(uint32 addr, uint16 val) MDFN_COLD;

uint16 PPU_PeekCGRAM(uint32 addr) MDFN_COLD;
void PPU_PokeCGRAM(uint32 addr, uint16 val) MDFN_COLD;

uint8 PPU_PeekOAM(uint32 addr) MDFN_COLD;
void PPU_PokeOAM(uint32 addr, uint8 val) MDFN_COLD;

uint8 PPU_PeekOAMHI(uint32 addr) MDFN_COLD;
void PPU_PokeOAMHI(uint32 addr, uint8 val) MDFN_COLD;
//
//
//
enum
{
 PPU_GSREG_NMITIMEEN,
 PPU_GSREG_HTIME,
 PPU_GSREG_VTIME,
 PPU_GSREG_NMIFLAG,
 PPU_GSREG_IRQFLAG,
 PPU_GSREG_HVBJOY,

 PPU_GSREG_SCANLINE,

 PPU_GSREG_BGMODE,
 PPU_GSREG_MOSAIC,

 PPU_GSREG_W12SEL,
 PPU_GSREG_W34SEL,
 PPU_GSREG_WOBJSEL,
 PPU_GSREG_WH0,     
 PPU_GSREG_WH1,
 PPU_GSREG_WH2,
 PPU_GSREG_WH3,
 PPU_GSREG_WBGLOG,
 PPU_GSREG_WOBJLOG,
 PPU_GSREG_TM,
 PPU_GSREG_TS,

 PPU_GSREG_CGWSEL,
 PPU_GSREG_CGADSUB,

 PPU_GSREG_BG1HOFS,
 PPU_GSREG_BG1VOFS,
 PPU_GSREG_BG2HOFS,
 PPU_GSREG_BG2VOFS,
 PPU_GSREG_BG3HOFS,
 PPU_GSREG_BG3VOFS,
 PPU_GSREG_BG4HOFS,
 PPU_GSREG_BG4VOFS,

 PPU_GSREG_M7SEL,
 PPU_GSREG_M7A,
 PPU_GSREG_M7B,
 PPU_GSREG_M7C,
 PPU_GSREG_M7D,
 PPU_GSREG_M7X,
 PPU_GSREG_M7Y,
 PPU_GSREG_M7HOFS,
 PPU_GSREG_M7VOFS,

 PPU_GSREG_SCREENMODE,
};

uint32 PPU_GetRegister(const unsigned id, char* const special, const uint32 special_len) MDFN_COLD;
void PPU_SetRegister(const unsigned id, const uint32 value) MDFN_COLD;

}

#endif
