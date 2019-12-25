/******************************************************************************/
/* Mednafen Fast SNES Emulation Module                                        */
/******************************************************************************/
/* cpu_hlif.inc:
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

#include "Core65816.h"

template<typename T>
INLINE void Core65816::MemWrite(uint32 addr, T val)
{
 CPU_Write(addr, val);

 if(sizeof(T) == 2)
 {
  addr++;
  CPU_Write(addr, val >> 8);
 }
}

template<typename T>
INLINE T Core65816::MemRead(uint32 addr)
{
 T ret;

 ret = CPU_Read(addr);

 if(sizeof(T) == 2)
 {
  addr++;
  ret |= CPU_Read(addr) << 8;
 }

 return ret;
}
//static bool popread = false;
INLINE uint8 Core65816::OpRead(uint32 addr)
{
 uint8 ret = MemRead<uint8>(addr);

 //if(popread)
 // SNES_DBG("  %02x\n", ret);

 return ret;
}

INLINE void Core65816::IO(void)
{
 CPU_IO();
}

INLINE void Core65816::Instr_STP(void)
{
 SNES_DBG("[CPU] STP\n");
 CPUM.halted |= CPU_Misc::HALTED_STP;

 if(CPUM.timestamp < CPUM.next_event_ts)
  CPUM.timestamp = CPUM.next_event_ts;
}

INLINE void Core65816::Instr_WAI(void)
{
 SampleIRQ();	// Important(to match games' expectations).

 if(!CPUM.CombinedNIState)
 {
  CPUM.halted |= CPU_Misc::HALTED_WAI;

  if(CPUM.timestamp < CPUM.next_event_ts)
   CPUM.timestamp = CPUM.next_event_ts;
 }
}

INLINE void Core65816::SampleIRQ(void)
{
 CPUM.PIN_Delay = ((P ^ I_FLAG) | 0x01) & CPUM.CombinedNIState;
}

INLINE void Core65816::BranchOccurred(unsigned iseq)
{
#ifdef SNES_DBG_ENABLE
 DBG_AddBranchTrace(PCPBR, iseq);
#endif
}

#include "Core65816.inc"

static Core65816 core;
CPU_Misc CPUM;

void CPU_Run(void)
{
#ifdef HAVE_COMPUTED_GOTO
 const void* const LoopTable[4] =
 {
  &&Loop_0, &&Loop_1, &&Loop_2, &&Loop_3,
 };
 #define GOTO_MXLOOPTABLE() goto *LoopTable[(lc.P >> 4) & 0x3];
#else
 #define GOTO_MXLOOPTABLE() { switch((lc.P >> 4) & 0x3) { case 0: goto Loop_0; case 1: goto Loop_1; case 2: goto Loop_2; case 3: goto Loop_3; } }
#endif

 Core65816 lc = core;

 CPUM.running_mask = ~0U;
 GOTO_MXLOOPTABLE();

 #define CPULBMX Loop_0
 #define CPULBMTYPE	uint16
 #define CPULBXTYPE	uint16
 #include "cpu_loop_body.inc"
 #undef CPULBXTYPE
 #undef CPULBMTYPE
 #undef CPULBMX

 #define CPULBMX Loop_1
 #define CPULBMTYPE	uint16
 #define CPULBXTYPE	uint8
 #include "cpu_loop_body.inc"
 #undef CPULBXTYPE
 #undef CPULBMTYPE
 #undef CPULBMX

 #define CPULBMX Loop_2
 #define CPULBMTYPE	uint8
 #define CPULBXTYPE	uint16
 #include "cpu_loop_body.inc"
 #undef CPULBXTYPE
 #undef CPULBMTYPE
 #undef CPULBMX

 #define CPULBMX Loop_3
 #define CPULBMTYPE	uint8
 #define CPULBXTYPE	uint8
 #include "cpu_loop_body.inc"
 #undef CPULBXTYPE
 #undef CPULBMTYPE
 #undef CPULBMX


 ExitCat: ;
 core = lc;
}

void CPU_Reset(bool powering_up)
{
 CPUM.halted = 0;
 CPUM.mdr = 0x00;

 CPUM.CombinedNIState &= ~0x01;

 if(powering_up)
  core.Power();
 //
 CPUM.PIN_Delay = 0x40; // Trigger reset
}

void CPU_Init(void)
{
 CPUM.timestamp = 0;

 CPUM.PIN_Delay = 0;
 CPUM.PrevNMILineState = false;
 CPUM.CombinedNIState = 0x00;
 CPUM.MultiIRQState = 0x00;
 CPUM.NMILineState = false;
 CPUM.PrevNMILineState = false;
}

void CPU_StateAction(StateMem* sm, const unsigned load, const bool data_only, const char* sname, const char* sname_core)
{
 if(load && load < 0x00102300)
 {
  SFORMAT BWCStateRegs[] = 
  {
   SFVARN(CPUM.PIN_Delay, "PIN_Delay"),
   SFEND
  };
  MDFNSS_StateAction(sm, load, data_only, BWCStateRegs, sname_core);
 }
 //
 //
 //
 SFORMAT StateRegs[] =
 {
  SFVAR(CPUM.PIN_Delay),
  SFVAR(CPUM.mdr),
  SFVAR(CPUM.halted),
  SFVAR(CPUM.CombinedNIState),
  SFVAR(CPUM.NMILineState),
  SFVAR(CPUM.PrevNMILineState),
  SFVAR(CPUM.MultiIRQState),
  SFEND
 };

 MDFNSS_StateAction(sm, load, data_only, StateRegs, sname);

 if(load && load < 0x00102300)
 {
  uint8 new_halted = 0;

  switch(CPUM.halted)
  {
   case 1: new_halted = CPU_Misc::HALTED_WAI; break;
   case 2: new_halted = CPU_Misc::HALTED_STP; break;
   case 3: new_halted = CPU_Misc::HALTED_DMA; break;
  }
  CPUM.halted = new_halted;
 }

 core.StateAction(sm, load, data_only, sname_core);
}
//
//
//
uint32 CPU_GetRegister(const unsigned id, char* const special, const uint32 special_len)
{
 return core.GetRegister(id, special, special_len);
}

void CPU_SetRegister(const unsigned id, const uint32 value)
{
 core.SetRegister(id, value);
}

