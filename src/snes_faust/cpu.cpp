/******************************************************************************/
/* Mednafen Fast SNES Emulation Module                                        */
/******************************************************************************/
/* cpu.cpp:
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

// TODO: attribute packed?

/*
 Emulation mode isn't emulated, since commercially-released SNES games generally don't need it.

 16-bit DP and stack data read/writes to $00:FFFF may not be emulated correctly(not that it really matters for the SNES).

 IRQ/NMI timing is extremely approximate.
*/



//
// Stack and direct always use bank 0
//   (note: take care to make sure all address calculations for these wrap around to bank 0 by implicit or explicit (uint16)/&0xFFFF)
//

// Page boundary crossing; (d),y a,x a,y; "Extra read of invalid address"???


/*
 Notes:
  REP, SEP, RTI, and PLP instructions change X and M bits.

  16->8->16-bit accumulator mode switch preserves upper 8 bits.

  16->8->16-bit index register mode switch zeroes upper 8 bits.

*/

#include <mednafen/mednafen.h>
#include "cpu.h"
#include "snes.h"

namespace MDFN_IEN_SNES_FAUST
{

void DBG_CPUHook(uint32 PCPBR, uint8 P);

#include "cpu_hlif.inc"


}
