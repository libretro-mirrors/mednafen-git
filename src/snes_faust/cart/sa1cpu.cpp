/******************************************************************************/
/* Mednafen Fast SNES Emulation Module                                        */
/******************************************************************************/
/* sa1.cpp:
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
#include "sa1.h"
#include "sa1cpu.h"

namespace MDFN_IEN_SNES_FAUST
{
namespace SA1CPU
{
//
//
#ifdef SNES_DBG_ENABLE
static INLINE void DBG_CPUHook(uint32 PCPBR, uint8 P)
{

}
#endif

#include "../cpu_hlif.inc"
//
//
void CPU_ClearRWFuncs(void)
{
 memset(CPUM.ReadFuncs, 0, sizeof(CPUM.ReadFuncs));
 memset(CPUM.WriteFuncs, 0, sizeof(CPUM.WriteFuncs));
}

void CPU_SetRWHandlers(uint32 A1, uint32 A2, readfunc read_handler, writefunc write_handler, bool special)
{
 if(special)
 {
  assert(!((A1 ^ A2) >> 9));

  CPUM.RWIndex[A1 >> 9] = 0;

  for(uint32 A = A1; A <= A2; A++)
  {
   const size_t am = A & 0x1FF;

   //assert((!CPUM.ReadFuncs[am] && !CPUM.WriteFuncs[am]) || (CPUM.ReadFuncs[am] == read_handler && CPUM.WriteFuncs[am] == write_handler));

   CPUM.ReadFuncs[am] = read_handler;
   CPUM.WriteFuncs[am] = write_handler;
  }
 }
 else
 {
  assert(!(A1 & 0x1FF) && (A2 & 0x1FF) == 0x1FF);
  assert(A1 <= A2);
  assert((A2 >> 9) < 0x8000);

  size_t index = 512;

  while(index < (512 + 256) && CPUM.ReadFuncs[index] && CPUM.WriteFuncs[index] && (CPUM.ReadFuncs[index] != read_handler || CPUM.WriteFuncs[index] != write_handler))
   index++;

  assert(index < (512 + 256));

  CPUM.ReadFuncs[index] = read_handler;
  CPUM.WriteFuncs[index] = write_handler;

  for(uint32 A = A1; A <= A2; A += 512)
   CPUM.RWIndex[A >> 9] = index;

  CPUM.RWIndex[0x8000] = CPUM.RWIndex[0x7FFF];
 }
}
//
//
}
}
