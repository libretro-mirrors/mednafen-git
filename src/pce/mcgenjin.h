/* Mednafen - Multi-system Emulator
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __MDFN_PCE_MCGENJIN_H
#define __MDFN_PCE_MCGENJIN_H

#include <vector>
#include <blip/Blip_Buffer.h>
#include "../cdrom/SimpleFIFO.h"

class MCGenjin_CS_Device
{
 public:

 MCGenjin_CS_Device();
 virtual ~MCGenjin_CS_Device();

 virtual void Power(void);
 virtual void EndFrame(int32 timestamp);

 virtual int StateAction(StateMem *sm, int load, int data_only, const char *sname);

 virtual uint8 Read(int32 timestamp, uint32 A);
 virtual void Write(int32 timestamp, uint32 A, uint8 V);

 virtual uint32 GetNVSize(void);
 virtual void ReadNV(uint8 *buffer, uint32 offset, uint32 count);
 virtual void WriteNV(const uint8 *buffer, uint32 offset, uint32 count);
};

class MCGenjin
{
 public:

 MCGenjin(Blip_Buffer *bb, const uint8 *rr, uint32 rr_size);
 ~MCGenjin();

 void Power(void);
 void EndFrame(int32 timestamp);
 int StateAction(StateMem *sm, int load, int data_only);

 INLINE unsigned GetNVPDC(void) { return 2; }
 uint32 GetNVSize(const unsigned di);
 void ReadNV(const unsigned di, uint8 *buffer, uint32 offset, uint32 count);
 void WriteNV(const unsigned di, const uint8 *buffer, uint32 offset, uint32 count);

 INLINE uint8 combobble(uint8 v)
 {
  if(dlr)
   return ((((v * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL) >> 32);
  else
   return v;
 }

 template<unsigned ar>
 INLINE uint8 ReadTP(int32 timestamp, uint32 A)
 {
  uint8 ret = 0xFF;

  switch(ar)
  {
   case 0: ret = combobble(rom[A & 0x3FFFF & (rom.size() - 1)]);
	   break;

   case 1: {
	    const uint32 rom_addr_or = bank_select << 18;
	    const uint32 rom_addr_xor = ((stmode_control & 0x7E) << 6) | (stmode_control & STMODEC_MASK_ENABLE);
	    uint32 rom_addr_and = (rom.size() - 1);

	    if((stmode_control & 0x80) && !(A & 0x1))
	     rom_addr_and &= ~0x1FFC;

	    ret = combobble(rom[(((A & 0x3FFFF) | rom_addr_or) ^ rom_addr_xor) & rom_addr_and]);
	   }
	   break;

   case 2: ret = cs[0]->Read(timestamp, A & 0x3FFFF); break;
   case 3: ret = cs[1]->Read(timestamp, A & 0x3FFFF); break;
  }

  return ret;
 }

 template<unsigned ar>
 INLINE void WriteTP(int32 timestamp, uint32 A, uint8 V)
 {
  switch(ar)
  {
   case 0:
   case 1:
	switch((A & addr_write_mask) | (~addr_write_mask & 0xF))
	{
	 case 0x0: avl_mask[0] = V; break;
	 case 0x1: avl_mask[1] = V; break;
	 case 0x2: avl_mask[2] = V; break;
	 case 0x3: avl_mask[3] = V; break;

         case 0x4: avl[0] &= 0xFF00; avl[0] |= V; break;
         case 0x5: avl[1] &= 0xFF00; avl[1] |= V; break;
         case 0x6: avl[2] &= 0xFF00; avl[2] |= V; break;
         case 0x7: avl[3] &= 0xFF00; avl[3] |= V; break;

         case 0x8: avl[0] &= 0x00FF; avl[0] |= V << 8; break;
         case 0x9: avl[1] &= 0x00FF; avl[1] |= V << 8; break;
         case 0xA: avl[2] &= 0x00FF; avl[2] |= V << 8; break;
         case 0xB: avl[3] &= 0x00FF; avl[3] |= V << 8; break;

	 case 0xC:
		for(unsigned i = 0; i < 4; i++)
		{
		 if(V & (1U << i))
		  shadow_avl[i] += avl[i];
		 else
		  shadow_avl[i] = avl[i];
		}
		pcm_out_shift = (V >> 4) & 0x3;
		//
		{
		 uint8 pcm_out = (((shadow_avl[0] >> 8) & avl_mask[0]) + ((shadow_avl[1] >> 8) & avl_mask[1]) + ((shadow_avl[2] >> 8) & avl_mask[2]) + ((shadow_avl[3] >> 8) & avl_mask[3])) >> pcm_out_shift;
		 pcm_synth.offset_inline(timestamp, pcm_out - pcm_lastout, bsbuf);
		 pcm_lastout = pcm_out;
		}
		break;

	 case 0xD:
		stmode_control = V;
		break;

	 case 0xE:
		dlr = V & 0x1;
		break;

	 case 0xF:
		bank_select = V;
		break;
	}
	break;

   case 2: return cs[0]->Write(timestamp, A & 0x3FFFF, V);
   case 3: return cs[1]->Write(timestamp, A & 0x3FFFF, V);
  }
 }


 INLINE uint8 Read(int32 timestamp, uint32 A)
 {
  switch((A >> 18) & 0x3)
  {
   default: return 0xFF;
   case 0: return ReadTP<0>(timestamp, A);
   case 1: return ReadTP<1>(timestamp, A);
   case 2: return ReadTP<2>(timestamp, A);
   case 3: return ReadTP<3>(timestamp, A);
  }
 }

 INLINE void Write(int32 timestamp, uint32 A, uint8 V)
 {
  switch((A >> 18) & 0x3)
  {
   case 0: WriteTP<0>(timestamp, A, V);
   case 1: WriteTP<1>(timestamp, A, V);
   case 2: WriteTP<2>(timestamp, A, V);
   case 3: WriteTP<3>(timestamp, A, V);
  }
 }

 private:

 std::vector<uint8> rom;

 MCGenjin_CS_Device *cs[2];

 uint8 bank_select;
 uint8 dlr;

 enum { STMODEC_MASK_ENABLE = 0x80 };

 Blip_Buffer *bsbuf;

 //
 //
 //
 uint8 addr_write_mask;

 uint8 stmode_control;

 uint16 avl[4];
 uint8 avl_mask[4];
 uint16 shadow_avl[4];
 uint8 pcm_out_shift;

 //
 int32 pcm_lastout;
 Blip_Synth <blip_good_quality, 1> pcm_synth;
};

#endif
