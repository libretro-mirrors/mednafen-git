/******************************************************************************/
/* Mednafen Fast SNES Emulation Module                                        */
/******************************************************************************/
/* superfx.cpp:
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

/*
 TODO:
	Correctly handle differences between MC1, GSU1, GSU2(especially in regards to memory maps).

	Correct timings(also clock source vs NTSC/PAL vs MC1/GSUn).

	Test multiplication performance when running from uncached ROM/RAM.

	Correct handling of Rd == 4 in FMULT and LMULT(run tests).

	Test SBK behavior when immediately after each of the various RAM access instructions.


*/

#include "common.h"
#include "superfx.h"

namespace MDFN_IEN_SNES_FAUST
{

static uint16 R[16 + 1];
static uint8 Prefetch;
static size_t PrefixSL8;	// 0, 1, 2, 3; << 8
static bool PrefixB;

static uint8 Rs;
static uint8 Rd;

static const uint8* ProgMemMap[0x80];
static size_t ProgMemMapMask[0x80];
static const uint8* PBR_Ptr;
static size_t PBR_Ptr_Mask;
static uint8 PBR;
static uint16 CBR;
static uint8 ROMBR;
static uint8 RAMBR;
static uint8 VCR;
static bool ClockSelect;
static bool MultSpeed;

static uint8 SCBR;
static uint8 SCMR;
static uint8 POR;
static uint8 ColorData;

static struct
{
 uint8 TagX;
 uint8 TagY;
 uint8 data[8];
 uint8 opaque;
} PixelCache;
//
//
//
static uint8 CacheData[0x200];
static bool CacheValid[0x20];

static uint8 ROMBuffer;

static uint16 LastRAMOffset;

static bool FlagV, FlagS, FlagC, FlagZ;
static bool Running;
static bool IRQPending;
static bool IRQMask;

static uint8 GPRWriteLatch;

static uint32 superfx_timestamp;
static uint32 rom_read_finish_ts;
static uint32 ram_write_finish_ts;
static uint32 pixcache_write_finish_ts;

static uint32 cycles_per_op;
static uint32 cycles_per_rom_read;
static uint32 cycles_per_ram;
static uint32 cycles_per_8x8mult;
static uint32 cycles_per_16x16mult;
//
//
//
static void RecalcMultTiming(void)
{
 SNES_DBG("[SuperFX] Clock speed: %d, mult speed: %d\n", ClockSelect, MultSpeed);

 // FIXME: timings
 if(ClockSelect | MultSpeed)
 {
  cycles_per_16x16mult = 6;
  cycles_per_8x8mult = 0;
 }
 else
 {
  cycles_per_16x16mult = 14;
  cycles_per_8x8mult = 2;
 }
}

static INLINE void SetCFGR(uint8 val)
{
 MultSpeed = (val >> 5) & 1;
 IRQMask = val >> 7;
 CPU_SetIRQ(IRQPending & !IRQMask, CPU_IRQSOURCE_CART);
 //
 RecalcMultTiming();
}

static INLINE void SetCLSR(uint8 val)
{
 ClockSelect = val & 0x1;
 //
 if(ClockSelect)
 {
  cycles_per_op = 1;
  cycles_per_rom_read = 5;
  cycles_per_ram = 3;
 }
 else
 {
  cycles_per_op = 2;
  cycles_per_rom_read = 6;
  cycles_per_ram = 3;
 }
 //
 RecalcMultTiming();
}

static INLINE void SetPBR(uint8 val)
{
 PBR = val;
 PBR_Ptr = ProgMemMap[val & 0x7F];
 PBR_Ptr_Mask = ProgMemMapMask[val & 0x7F];
}

static void DoROMRead(void)
{
 if(superfx_timestamp < rom_read_finish_ts)
 {
  //printf("DoROMRead() delay: %d\n", rom_read_finish_ts - superfx_timestamp);
 }
 superfx_timestamp = std::max<uint32>(superfx_timestamp, rom_read_finish_ts);
 ROMBuffer = ProgMemMap[ROMBR & 0x7F][R[14] & ProgMemMapMask[ROMBR & 0x7F]];
 rom_read_finish_ts = superfx_timestamp + cycles_per_rom_read;
}

static uint8 GetROMBuffer(void)
{
 if(superfx_timestamp < rom_read_finish_ts)
 {
  //printf("GetROMBuffer() delay: %d\n", rom_read_finish_ts - superfx_timestamp);
 }
 superfx_timestamp = std::max<uint32>(superfx_timestamp, rom_read_finish_ts);

 return ROMBuffer;
}

static INLINE void WriteR(size_t w, uint16 val)
{
 R[w] = val;

 if(w == 14)
  DoROMRead();
}

// Only needed to be used when reg might == 15
static INLINE uint16 ReadR(size_t w)
{
 return R[w + (w == 15)];
}

static INLINE void CMODE(void)
{
 POR = ReadR(Rs) & 0x1F;
}

static INLINE void WriteColorData(uint8 tmp)
{
 tmp = (tmp & 0xF0) + ((tmp >> (POR & 0x4)) & 0x0F);

 if(POR & 0x8)
  tmp = (ColorData & 0xF0) + (tmp & 0x0F);

 ColorData = tmp;
}

static INLINE void COLOR(void)
{
 WriteColorData(ReadR(Rs));
}

static INLINE void GETC(void)
{
 uint8 tmp = GetROMBuffer();

 WriteColorData(tmp);
}

static INLINE void CalcSZ(uint16 v)
{
 FlagS = v >> 15;
 FlagZ = !v;
}

static INLINE unsigned CalcTNO(unsigned x, unsigned y)
{
 unsigned tno;
 const unsigned shs = ((SCMR >> 2) & 0x1) | ((SCMR >> 4) & 0x2);

 if(shs == 0x3 || (POR & 0x10))
 {
  tno = ((y >> 7) << 9) + ((x >> 7) << 8) + (((y >> 3) & 0xF) << 4) + ((x >> 3) & 0xF);
 }
 else switch(shs)
 {
  case 0: tno = (x >> 3) * 0x10 + (y >> 3); break;
  case 1: tno = (x >> 3) * 0x14 + (y >> 3); break;
  case 2: tno = (x >> 3) * 0x18 + (y >> 3); break;
 }

 tno &= 0x3FF;

 return tno;
}

static void FlushPixelCache(void)
{
 if(!PixelCache.opaque) // FIXME: correct?
  return;

 superfx_timestamp = std::max<int64>(superfx_timestamp, pixcache_write_finish_ts);
 pixcache_write_finish_ts = std::max<int64>(superfx_timestamp, ram_write_finish_ts);
 //
 //
 //
 const unsigned bpp = SCMR & 0x3;
 static const unsigned tnoshtab[4] = { 4, 5, 6, 6 };
 const unsigned x = PixelCache.TagX;
 const unsigned y = PixelCache.TagY;
 const unsigned finey = y & 7;
 unsigned tno = CalcTNO(x, y);
 unsigned tra = (tno << tnoshtab[bpp]) + (SCBR << 10) + (finey << 1);
 uint8* p = &Cart.RAM[tra & Cart.RAM_Mask];

 if(PixelCache.opaque != 0xFF)
 {
  // FIXME: time
  static const uint8 wtt[4] = { 2 * 3, 4 * 3, 8 * 3, 8 * 3};
  pixcache_write_finish_ts += wtt[bpp];
 }
 //
 //
 //
 {
  const unsigned mask = ~PixelCache.opaque;

  p[0x00] &= mask;
  p[0x01] &= mask;
  if(bpp >= 1)
  {
   p[0x10] &= mask;
   p[0x11] &= mask;

   if(bpp >= 2)
   {
    p[0x20] &= mask;
    p[0x21] &= mask;
    p[0x30] &= mask;
    p[0x31] &= mask;
   }
  }
 }
 //
 //
 //
 for(int i = 0; i < 8; i++)
 {
  if(PixelCache.opaque & 0x01)
  {
   const unsigned color = PixelCache.data[i];

   p[0x00] |= ((color >> 0) & 1) << i;
   p[0x01] |= ((color >> 1) & 1) << i;
   if(bpp >= 1)
   {
    p[0x10] |= ((color >> 2) & 1) << i;
    p[0x11] |= ((color >> 3) & 1) << i;

    if(bpp >= 2)
    {
     p[0x20] |= ((color >> 4) & 1) << i;
     p[0x21] |= ((color >> 5) & 1) << i;
     p[0x30] |= ((color >> 6) & 1) << i;
     p[0x31] |= ((color >> 7) & 1) << i;
    }
   }
  }
  //
  PixelCache.opaque >>= 1;
 }
 // FIXME: time
 static const uint8 wtt[4] = { 2 * 3, 4 * 3, 8 * 3, 8 * 3};
 pixcache_write_finish_ts += wtt[bpp];
 //
 //
 //
 ram_write_finish_ts = pixcache_write_finish_ts;
}

static INLINE void PLOT(void)
{
 const unsigned x = (uint8)R[1];
 const unsigned y = (uint8)R[2];

 if(PixelCache.TagX != (x & 0xF8) || PixelCache.TagY != y)
 {
  FlushPixelCache();
  PixelCache.TagX = x & 0xF8;
  PixelCache.TagY = y;
 }

 const unsigned finex = x & 7;
 const unsigned bpp = SCMR & 0x3;

 uint8 color = ColorData;
 bool visible = true;
 //
 color >>= ((((x ^ y) & 1) << 1) & POR & ~bpp) << 1;
 if(!(POR & 0x1))
 {
  static const unsigned masktab[4] = { 0x3, 0xF, 0xFF, 0xFF };

  if(!(color & masktab[bpp] & ((POR & 8) ? 0xF : 0xFF)))
   visible = false;
 }

 PixelCache.data[7 - finex] = color;
 PixelCache.opaque |= visible << (7 - finex);

 if(PixelCache.opaque == 0xFF)	// Set to 1, or set?
  FlushPixelCache();
 //
 WriteR(1, R[1] + 1);
}

static INLINE void RPIX(void)
{
 FlushPixelCache();
 //
 //
 //
 const unsigned x = (uint8)R[1];
 const unsigned y = (uint8)R[2];
 const unsigned finex = x & 7;
 const unsigned finey = y & 7;
 unsigned tno = CalcTNO(x, y);
 unsigned tra;

 const unsigned bpp = SCMR & 0x3;
 static const unsigned tnoshtab[4] = { 4, 5, 6, 6 };

 tra = (tno << tnoshtab[bpp]) + (SCBR << 10) + (finey << 1);
 uint8* p = &Cart.RAM[tra & Cart.RAM_Mask];
 uint8 color = 0;
 const unsigned shift = 7 - finex;

 color |= ((p[0x00] >> shift) & 1) << 0;
 color |= ((p[0x01] >> shift) & 1) << 1;
 if(bpp >= 1)
 {
  color |= ((p[0x10] >> shift) & 1) << 2;
  color |= ((p[0x11] >> shift) & 1) << 3;

  if(bpp >= 2)
  {
   color |= ((p[0x20] >> shift) & 1) << 4;
   color |= ((p[0x21] >> shift) & 1) << 5;
   color |= ((p[0x30] >> shift) & 1) << 6;
   color |= ((p[0x31] >> shift) & 1) << 7;
  }
 }

 CalcSZ(color); // FIXME?
 WriteR(Rd, color);
}

static INLINE void STOP(void)
{
 Running = false;
 IRQPending = true;
 //CPU_SetIRQ(IRQPending & !IRQMask, CPU_IRQSOURCE_CART);
}

static INLINE void SetCBR(uint16 val)
{
 CBR = val;
 memset(CacheValid, 0, sizeof(CacheValid));
}

static INLINE void CACHE(void)
{
 SetCBR(ReadR(15) & 0xFFF0);
}

static INLINE void ADD(uint16 arg)
{
 const uint32 tmp = ReadR(Rs) + arg;

 FlagV = (int16)((ReadR(Rs) ^ ~arg) & (ReadR(Rs) ^ tmp)) < 0;
 FlagC = tmp >> 16;
 CalcSZ(tmp);

 WriteR(Rd, tmp);
}

static INLINE void ADC(uint16 arg)
{
 const uint32 tmp = ReadR(Rs) + arg + FlagC;

 FlagV = (int16)((ReadR(Rs) ^ ~arg) & (ReadR(Rs) ^ tmp)) < 0;
 FlagC = tmp >> 16;
 CalcSZ(tmp);

 WriteR(Rd, tmp);
}

static INLINE void SUB(uint16 arg)
{
 const uint32 tmp = ReadR(Rs) - arg;

 FlagV = (int16)((ReadR(Rs) ^ arg) & (ReadR(Rs) ^ tmp)) < 0;
 FlagC = ((tmp >> 16) ^ 1) & 1;
 CalcSZ(tmp);

 WriteR(Rd, tmp);
}

static INLINE void SBC(uint16 arg)
{
 const uint32 tmp = ReadR(Rs) - arg - !FlagC;

 FlagV = (int16)((ReadR(Rs) ^ arg) & (ReadR(Rs) ^ tmp)) < 0;
 FlagC = ((tmp >> 16) ^ 1) & 1;
 CalcSZ(tmp);

 WriteR(Rd, tmp);
}

static INLINE void CMP(uint16 arg)
{
 const uint32 tmp = ReadR(Rs) - arg;

 FlagV = (int16)((ReadR(Rs) ^ arg) & (ReadR(Rs) ^ tmp)) < 0;
 FlagC = ((tmp >> 16) ^ 1) & 1;
 CalcSZ(tmp);
}

static INLINE void AND(uint16 arg)
{
 const uint16 tmp = ReadR(Rs) & arg;

 CalcSZ(tmp);

 WriteR(Rd, tmp);
}

static INLINE void BIC(uint16 arg)
{
 const uint16 tmp = ReadR(Rs) & ~arg;

 CalcSZ(tmp);

 WriteR(Rd, tmp);
}

static INLINE void OR(uint16 arg)
{
 const uint16 tmp = ReadR(Rs) | arg;

 CalcSZ(tmp);

 WriteR(Rd, tmp);
}

static INLINE void XOR(uint16 arg)
{
 const uint16 tmp = ReadR(Rs) ^ arg;

 CalcSZ(tmp);

 WriteR(Rd, tmp);
}

static INLINE void NOT(void)
{
 const uint16 tmp = ~ReadR(Rs);

 CalcSZ(tmp);

 WriteR(Rd, tmp);
}

static INLINE void LSR(void)
{
 const uint16 tmp = ReadR(Rs) >> 1;

 FlagC = ReadR(Rs) & 1;
 CalcSZ(tmp);

 WriteR(Rd, tmp);
}

static INLINE void ASR(void)
{
 const uint16 tmp = (int16)ReadR(Rs) >> 1;

 FlagC = ReadR(Rs) & 1;
 CalcSZ(tmp);

 WriteR(Rd, tmp);
}

static INLINE void ROL(void)
{
 const bool NewFlagC = ReadR(Rs) >> 15;
 const uint16 tmp = (ReadR(Rs) << 1) | FlagC;

 FlagC = NewFlagC;
 CalcSZ(tmp);

 WriteR(Rd, tmp);
}

static INLINE void ROR(void)
{
 const bool NewFlagC = ReadR(Rs) & 1;
 const uint16 tmp = (ReadR(Rs) >> 1) | (FlagC << 15);

 FlagC = NewFlagC;
 CalcSZ(tmp);

 WriteR(Rd, tmp);
}

static INLINE void DIV2(void)
{
 //const uint16 tmp = ((int16)ReadR(Rs) + (ReadR(Rs) >> 15)) >> 1;
 uint16 tmp = ((int16)ReadR(Rs) >> 1); // + (ReadR(Rs) == 0xFFFF);

 if(tmp == 0xFFFF)
  tmp = 0;

 FlagC = ReadR(Rs) & 1;
 CalcSZ(tmp);

 WriteR(Rd, tmp);
}

static INLINE void INC(uint8 w)
{
 const uint16 tmp = R[w] + 1;

 CalcSZ(tmp);

 WriteR(w, tmp);
}

static INLINE void DEC(uint8 w)
{
 const uint16 tmp = R[w] - 1;

 CalcSZ(tmp);

 WriteR(w, tmp);
}

static INLINE void SWAP(void)
{
 const uint16 tmp = (ReadR(Rs) >> 8) | (ReadR(Rs) << 8);

 CalcSZ(tmp);

 WriteR(Rd, tmp);
}

static INLINE void SEX(void)
{
 const uint16 tmp = (int8)ReadR(Rs);

 CalcSZ(tmp);

 WriteR(Rd, tmp);
}

static INLINE void LOB(void)
{
 const uint16 tmp = (uint8)ReadR(Rs);

 FlagS = tmp >> 7;
 FlagZ = !tmp;

 WriteR(Rd, tmp);
}

static INLINE void HIB(void)
{
 const uint16 tmp = ReadR(Rs) >> 8;

 FlagS = ReadR(Rs) >> 15;
 FlagZ = !tmp;

 WriteR(Rd, tmp);
}

static INLINE void MERGE(void)
{
 const uint16 tmp = (R[7] & 0xFF00) + (R[8] >> 8);

 FlagS = (bool)(tmp & 0x8080);
 FlagV = (bool)(tmp & 0xC0C0);
 FlagC = (bool)(tmp & 0xE0E0);
 FlagZ = (bool)(tmp & 0xF0F0);

 WriteR(Rd, tmp);
}

static INLINE void FMULT(void)
{
 superfx_timestamp += cycles_per_16x16mult;
 //
 if(Rd == 4)
  SNES_DBG("[SuperFX] FMULT with Rd=4\n");

 const uint32 tmp = (int16)ReadR(Rs) * (int16)R[6];

 // FIXME?
 FlagS = tmp >> 31;
 FlagZ = !tmp;
 FlagC = (tmp >> 15) & 1;

 WriteR(Rd, tmp >> 16);
}

static INLINE void LMULT(void)
{
 superfx_timestamp += cycles_per_16x16mult;
 //
 if(Rd == 4)
  SNES_DBG("[SuperFX] LMULT with Rd=4\n");

 const uint32 tmp = (int16)ReadR(Rs) * (int16)R[6];

 // FIXME?
 FlagS = tmp >> 31;
 FlagZ = !tmp;
 FlagC = (tmp >> 15) & 1;

 WriteR(Rd, tmp >> 16);
 WriteR(4, tmp);
}

static INLINE void MULT(uint16 arg)
{
 superfx_timestamp += cycles_per_8x8mult;
 //
 const uint16 tmp = (int8)ReadR(Rs) * (int8)arg;

 CalcSZ(tmp);

 WriteR(Rd, tmp);
}

static INLINE void UMULT(uint16 arg)
{
 superfx_timestamp += cycles_per_8x8mult;
 //
 const uint16 tmp = (uint8)ReadR(Rs) * (uint8)arg;

 CalcSZ(tmp);

 WriteR(Rd, tmp);
}
//
//
//
static INLINE void JMP(uint8 Rn)
{
 WriteR(15, R[Rn]);
}

static INLINE void LJMP(uint8 Rn)
{
 const uint16 target = ReadR(Rs);

 WriteR(15, target);
 SetPBR(R[Rn]);
 SetCBR(target & 0xFFF0);
}

static INLINE void LOOP(void)
{
 const uint16 result = R[12] - 1;

 CalcSZ(result);
 WriteR(12, result);

 if(!FlagZ)
  WriteR(15, R[13]);
}

static INLINE void LINK(uint8 arg)
{
 WriteR(11, ReadR(15) + arg);
}
//
//
//
static uint8 ReadOp(void)
{
 uint8 ret = Prefetch;

#if 1
 Prefetch = PBR_Ptr[R[15] & PBR_Ptr_Mask];
 size_t cindex = R[15] - CBR;
 if(MDFN_UNLIKELY(cindex >= 0x200))
 {
  if(MDFN_LIKELY((PBR & 0x7F) < 0x60))
  {
   superfx_timestamp = std::max<uint32>(superfx_timestamp, rom_read_finish_ts);
   superfx_timestamp += cycles_per_rom_read;
  }
  else
  {
   superfx_timestamp = std::max<uint32>(superfx_timestamp, ram_write_finish_ts);
   superfx_timestamp += 3;
  }
 }
#else
 uint16 addr = R[15];
 size_t cindex = addr - CBR;
 if(MDFN_UNLIKELY(cindex >= 0x200))
 {
  Prefetch = PBR_Ptr[addr & PBR_Ptr_Mask];

  if(MDFN_LIKELY((PBR & 0x7F) < 0x60))
  {
   superfx_timestamp = std::max<uint32>(superfx_timestamp, rom_read_finish_ts);
   superfx_timestamp += cycles_per_rom_read;
  }
  else
  {
   superfx_timestamp = std::max<uint32>(superfx_timestamp, ram_write_finish_ts);
   superfx_timestamp += 3;
  }
 }
 else
 {
  const size_t cvi = cindex >> 4;

  if(MDFN_UNLIKELY(!CacheValid[cvi]))
  {
   uint8* d = &CacheData[cindex & ~0xF];

   CacheValid[cvi] = true;

   if(MDFN_LIKELY((PBR & 0x7F) < 0x60))
   {
    superfx_timestamp = std::max<uint32>(superfx_timestamp, rom_read_finish_ts);
    rom_read_finish_ts = superfx_timestamp;
   }

   for(unsigned i = 0; i < 0x10; i++)
   {
    const uint32 ra = (addr & 0xFFF0) + i;

    if(MDFN_LIKELY((PBR & 0x7F) < 0x60))
    {
     rom_read_finish_ts += cycles_per_rom_read;

     if(ra == addr)
      superfx_timestamp = rom_read_finish_ts;
    }

    d[i] = PBR_Ptr[ra & PBR_Ptr_Mask];
   }
  }
  Prefetch = CacheData[cindex];
 }
#endif

 R[16] = R[15];
 R[15]++;
 superfx_timestamp += cycles_per_op;

 return ret;
}

static uint16 ReadOp16(void)
{
 uint16 ret;

 ret  = ReadOp();
 ret |= ReadOp() << 8;

 return ret;
}

static INLINE void RelBranch(bool cond)
{
 int8 disp = ReadOp();

 if(cond)
  WriteR(15, ReadR(15) + disp);
}

static INLINE uint8 ReadRAM8(uint16 offs)
{
 superfx_timestamp = std::max<uint32>(superfx_timestamp, ram_write_finish_ts);
 superfx_timestamp += 4; // FIXME
 //
 //
 uint8 ret;

 //LastRAMOffset = offs;

 ret = Cart.RAM[((RAMBR << 16) + offs) & Cart.RAM_Mask];

 return ret;
}

static INLINE uint16 ReadRAM16(uint16 offs)
{
 superfx_timestamp = std::max<uint32>(superfx_timestamp, ram_write_finish_ts);
 superfx_timestamp += 6; // FIXME
 //
 //
 uint16 ret;

// assert(!(offs & 1));

 //LastRAMOffset = offs;

 ret  = Cart.RAM[((RAMBR << 16) + (offs ^ 0)) & Cart.RAM_Mask] << 0;
 ret |= Cart.RAM[((RAMBR << 16) + (offs ^ 1)) & Cart.RAM_Mask] << 8;

 return ret;
}

static INLINE void WriteRAM8(uint16 offs, uint8 val)
{
 superfx_timestamp = std::max<uint32>(superfx_timestamp, ram_write_finish_ts);
 ram_write_finish_ts = superfx_timestamp + 3; // FIXME
 //
 //
 //LastRAMOffset = offs;

 Cart.RAM[((RAMBR << 16) + (offs ^ 0)) & Cart.RAM_Mask] = val;
}

static INLINE void WriteRAM16(uint16 offs, uint16 val)
{
 superfx_timestamp = std::max<uint32>(superfx_timestamp, ram_write_finish_ts);
 //superfx_timestamp += whatever; // FIXME?
 ram_write_finish_ts = superfx_timestamp + 5; // FIXME
 //
 //
// assert(!(offs & 1));

 //LastRAMOffset = offs;

 Cart.RAM[((RAMBR << 16) + (offs ^ 0)) & Cart.RAM_Mask] = val >> 0;
 Cart.RAM[((RAMBR << 16) + (offs ^ 1)) & Cart.RAM_Mask] = val >> 8;
}

static void Update(uint32 timestamp)
{
 if(MDFN_UNLIKELY(!Running))
  superfx_timestamp = timestamp;

 if((SCMR & 0x18) != 0x18)
  superfx_timestamp = timestamp;

 while(superfx_timestamp < timestamp)
 {
  uint8 opcode;

  opcode = ReadOp();

  #define OPCASE_PFX(pfx, o) case ((pfx) << 8) + (o)
  #define OPCASE(o) OPCASE_PFX(0, o): OPCASE_PFX(1, o): OPCASE_PFX(2, o): OPCASE_PFX(3, o)

  #define OPCASE_NP(o) OPCASE_PFX(0, o)
  #define OPCASE_3D(o) OPCASE_PFX(1, o)
  #define OPCASE_3E(o) OPCASE_PFX(2, o)
  #define OPCASE_3F(o) OPCASE_PFX(3, o)

  #define OPCASE_PFX_Xn(pfx, o)	\
	OPCASE_PFX(pfx, (o) + 0x0): OPCASE_PFX(pfx, (o) + 0x1): OPCASE_PFX(pfx, (o) + 0x2): OPCASE_PFX(pfx, (o) + 0x3): OPCASE_PFX(pfx, (o) + 0x4): OPCASE_PFX(pfx, (o) + 0x5): OPCASE_PFX(pfx, (o) + 0x6): OPCASE_PFX(pfx, (o) + 0x7):	\
	OPCASE_PFX(pfx, (o) + 0x8): OPCASE_PFX(pfx, (o) + 0x9): OPCASE_PFX(pfx, (o) + 0xA): OPCASE_PFX(pfx, (o) + 0xB): OPCASE_PFX(pfx, (o) + 0xC): OPCASE_PFX(pfx, (o) + 0xD): OPCASE_PFX(pfx, (o) + 0xE): OPCASE_PFX(pfx, (o) + 0xF)

  #define OPCASE_Xn(o) OPCASE_PFX_Xn(0, o): OPCASE_PFX_Xn(1, o): OPCASE_PFX_Xn(2, o): OPCASE_PFX_Xn(3, o)

  #define OPCASE_NP_Xn(o) OPCASE_PFX_Xn(0, o)
  #define OPCASE_3D_Xn(o) OPCASE_PFX_Xn(1, o)
  #define OPCASE_3E_Xn(o) OPCASE_PFX_Xn(2, o)
  #define OPCASE_3F_Xn(o) OPCASE_PFX_Xn(3, o)

  // 1?, 2?, 5?, 6?, 7?, 8?, 9?, C?, 
#if 0
  printf("Op: %03x --- ", (unsigned)(PrefixSL8 + opcode));
  for(unsigned i = 0; i < 16; i++)
   printf("%04x ", R[i]);

  printf(" ROMBR=%02x", ROMBR);
  printf("\n");
#endif
  switch(PrefixSL8 + opcode)
  {
   default:
	SNES_DBG("[SuperFX] Unknown op 0x%03x\n", (unsigned)(PrefixSL8 + opcode));
	break;

   //
   // GETB
   //
   OPCASE_NP(0xEF):
   {
    uint8 tmp = GetROMBuffer();

    WriteR(Rd, tmp);
   }
   break;

   //
   // GETBH
   //
   OPCASE_3D(0xEF):
   {
    uint8 tmp = GetROMBuffer();

    WriteR(Rd, (ReadR(Rs) & 0x00FF) + (tmp << 8));
   }
   break;

   //
   // GETBL
   //
   OPCASE_3E(0xEF):
   {
    uint8 tmp = GetROMBuffer();

    WriteR(Rd, (ReadR(Rs) & 0xFF00) + (tmp << 0));
   }
   break;

   //
   // GETBS
   //
   OPCASE_3F(0xEF):
   {
    uint8 tmp = GetROMBuffer();

    WriteR(Rd, (int8)tmp);
   }
   break;

   //
   // LDB
   //
   OPCASE_3D(0x40): OPCASE_3D(0x41): OPCASE_3D(0x42): OPCASE_3D(0x43): OPCASE_3D(0x44): OPCASE_3D(0x45): OPCASE_3D(0x46): OPCASE_3D(0x47):
   OPCASE_3D(0x48): OPCASE_3D(0x49): OPCASE_3D(0x4A): OPCASE_3D(0x4B):
   {
    uint16 offs = R[opcode & 0xF];
    uint8 tmp;

    tmp = ReadRAM8(offs);
    LastRAMOffset = offs;

    WriteR(Rd, tmp);
   }
   break;

   //
   // LDW
   //
   OPCASE_NP(0x40): OPCASE_NP(0x41): OPCASE_NP(0x42): OPCASE_NP(0x43): OPCASE_NP(0x44): OPCASE_NP(0x45): OPCASE_NP(0x46): OPCASE_NP(0x47):
   OPCASE_NP(0x48): OPCASE_NP(0x49): OPCASE_NP(0x4A): OPCASE_NP(0x4B):
   {
    uint16 offs = R[opcode & 0xF];
    uint16 tmp;

    tmp = ReadRAM16(offs);
    LastRAMOffset = offs;

    WriteR(Rd, tmp);
   }
   break;

   //
   //  LM
   //
   OPCASE_3D_Xn(0xF0):
   {
    const uint16 offs = ReadOp16();
    const uint16 tmp = ReadRAM16(offs);

    LastRAMOffset = offs;

    WriteR(opcode & 0xF, tmp);
   }
   break;

   //
   // LMS
   //
   OPCASE_3D_Xn(0xA0):
   {
    const uint16 offs = ReadOp() << 1;
    const uint16 tmp = ReadRAM16(offs);

    //printf("%04x\n", offs);

    LastRAMOffset = offs;

    WriteR(opcode & 0xF, tmp);
   }
   break;

   //
   // STB
   //
   OPCASE_3D(0x30): OPCASE_3D(0x31): OPCASE_3D(0x32): OPCASE_3D(0x33): OPCASE_3D(0x34): OPCASE_3D(0x35): OPCASE_3D(0x36): OPCASE_3D(0x37):
   OPCASE_3D(0x38): OPCASE_3D(0x39): OPCASE_3D(0x3A): OPCASE_3D(0x3B):
   {
    WriteRAM8(R[opcode & 0xF], ReadR(Rs));
   }
   break;

   //
   // STW
   //
   OPCASE_NP(0x30): OPCASE_NP(0x31): OPCASE_NP(0x32): OPCASE_NP(0x33): OPCASE_NP(0x34): OPCASE_NP(0x35): OPCASE_NP(0x36): OPCASE_NP(0x37):
   OPCASE_NP(0x38): OPCASE_NP(0x39): OPCASE_NP(0x3A): OPCASE_NP(0x3B):
   {
    WriteRAM16(R[opcode & 0xF], ReadR(Rs));
   }
   break;

   //
   // SM
   //
   OPCASE_3E_Xn(0xF0):
   {
    const uint16 offs = ReadOp16();

    WriteRAM16(offs, ReadR(opcode & 0xF));
   }
   break;

   //
   // SMS
   //
   OPCASE_3E_Xn(0xA0):
   {
    const uint16 offs = ReadOp() << 1;

    WriteRAM16(offs, ReadR(opcode & 0xF));
   }
   break;

   //
   // SBK
   //
   OPCASE(0x90):
   {
    WriteRAM16(LastRAMOffset, ReadR(Rs));
   }
   break;

   //
   // IBT
   //
   OPCASE_NP_Xn(0xA0):
   {
    const uint8 imm = ReadOp();

    WriteR(opcode & 0xF, (int8)imm);
   }
   break;

   //
   // IWT
   //
   OPCASE_NP_Xn(0xF0):
   {
    const uint16 imm = ReadOp16();

    WriteR(opcode & 0xF, imm);
   }
   break;

   //
   // RAMB
   //
   OPCASE_3E(0xDF):
   {
    RAMBR = ReadR(Rs) & 0x1;
   }
   break;

   //
   // ROMB
   //
   OPCASE_3F(0xDF):
   {
    ROMBR = ReadR(Rs) & 0xFF;
//	assert(!(ROMBR & 0x80));
   }
   break;

   OPCASE_3D(0x4E): CMODE(); break;
   OPCASE_NP(0x4E): COLOR(); break;
   OPCASE_NP(0xDF): GETC(); break;
   OPCASE_NP(0x4C): PLOT(); break;
   OPCASE_3D(0x4C): RPIX(); break;

   //
   //
   //
   //
   //

   OPCASE(0x00): STOP(); superfx_timestamp = timestamp; break; // STOP
   OPCASE(0x01): break; // NOP
   OPCASE(0x02): CACHE(); break; // CACHE

   OPCASE_NP_Xn(0x50): ADD(ReadR(opcode & 0xF)); break;
   OPCASE_3E_Xn(0x50): ADD(opcode & 0xF); break;

   OPCASE_3D_Xn(0x50): ADC(ReadR(opcode & 0xF)); break;
   OPCASE_3F_Xn(0x50): ADC(opcode & 0xF); break;

   OPCASE_NP_Xn(0x60): SUB(ReadR(opcode & 0xF)); break;
   OPCASE_3E_Xn(0x60): SUB(opcode & 0xF); break;

   OPCASE_3D_Xn(0x60): SBC(ReadR(opcode & 0xF)); break;
   OPCASE_3F_Xn(0x60): CMP(ReadR(opcode & 0xF)); break;

   /* AND Rn  */ OPCASE_NP(0x71): OPCASE_NP(0x72): OPCASE_NP(0x73): OPCASE_NP(0x74): OPCASE_NP(0x75): OPCASE_NP(0x76): OPCASE_NP(0x77):
   OPCASE_NP(0x78): OPCASE_NP(0x79): OPCASE_NP(0x7A): OPCASE_NP(0x7B): OPCASE_NP(0x7C): OPCASE_NP(0x7D): OPCASE_NP(0x7E): OPCASE_NP(0x7F):
	AND(ReadR(opcode & 0xF));
	break;

   /* AND #n     */ OPCASE_3E(0x71): OPCASE_3E(0x72): OPCASE_3E(0x73): OPCASE_3E(0x74): OPCASE_3E(0x75): OPCASE_3E(0x76): OPCASE_3E(0x77):
   OPCASE_3E(0x78): OPCASE_3E(0x79): OPCASE_3E(0x7A): OPCASE_3E(0x7B): OPCASE_3E(0x7C): OPCASE_3E(0x7D): OPCASE_3E(0x7E): OPCASE_3E(0x7F):
	AND(opcode & 0xF);
	break;

   /* BIC Rn     */ OPCASE_3D(0x71): OPCASE_3D(0x72): OPCASE_3D(0x73): OPCASE_3D(0x74): OPCASE_3D(0x75): OPCASE_3D(0x76): OPCASE_3D(0x77):
   OPCASE_3D(0x78): OPCASE_3D(0x79): OPCASE_3D(0x7A): OPCASE_3D(0x7B): OPCASE_3D(0x7C): OPCASE_3D(0x7D): OPCASE_3D(0x7E): OPCASE_3D(0x7F):
	BIC(ReadR(opcode & 0xF));
	break;

   /* BIC #n     */ OPCASE_3F(0x71): OPCASE_3F(0x72): OPCASE_3F(0x73): OPCASE_3F(0x74): OPCASE_3F(0x75): OPCASE_3F(0x76): OPCASE_3F(0x77):
   OPCASE_3F(0x78): OPCASE_3F(0x79): OPCASE_3F(0x7A): OPCASE_3F(0x7B): OPCASE_3F(0x7C): OPCASE_3F(0x7D): OPCASE_3F(0x7E): OPCASE_3F(0x7F):
	BIC(opcode & 0xF);
	break;

   /*  OR Rn  */ OPCASE_NP(0xC1): OPCASE_NP(0xC2): OPCASE_NP(0xC3): OPCASE_NP(0xC4): OPCASE_NP(0xC5): OPCASE_NP(0xC6): OPCASE_NP(0xC7):
   OPCASE_NP(0xC8): OPCASE_NP(0xC9): OPCASE_NP(0xCA): OPCASE_NP(0xCB): OPCASE_NP(0xCC): OPCASE_NP(0xCD): OPCASE_NP(0xCE): OPCASE_NP(0xCF):
	OR(ReadR(opcode & 0xF));
	break;

   /*  OR #n     */ OPCASE_3E(0xC1): OPCASE_3E(0xC2): OPCASE_3E(0xC3): OPCASE_3E(0xC4): OPCASE_3E(0xC5): OPCASE_3E(0xC6): OPCASE_3E(0xC7):
   OPCASE_3E(0xC8): OPCASE_3E(0xC9): OPCASE_3E(0xCA): OPCASE_3E(0xCB): OPCASE_3E(0xCC): OPCASE_3E(0xCD): OPCASE_3E(0xCE): OPCASE_3E(0xCF):
	OR(opcode & 0xF);
	break;

   /* XOR Rn     */ OPCASE_3D(0xC1): OPCASE_3D(0xC2): OPCASE_3D(0xC3): OPCASE_3D(0xC4): OPCASE_3D(0xC5): OPCASE_3D(0xC6): OPCASE_3D(0xC7):
   OPCASE_3D(0xC8): OPCASE_3D(0xC9): OPCASE_3D(0xCA): OPCASE_3D(0xCB): OPCASE_3D(0xCC): OPCASE_3D(0xCD): OPCASE_3D(0xCE): OPCASE_3D(0xCF):
	XOR(ReadR(opcode & 0xF));
	break;

   /* XOR #n     */ OPCASE_3F(0xC1): OPCASE_3F(0xC2): OPCASE_3F(0xC3): OPCASE_3F(0xC4): OPCASE_3F(0xC5): OPCASE_3F(0xC6): OPCASE_3F(0xC7):
   OPCASE_3F(0xC8): OPCASE_3F(0xC9): OPCASE_3F(0xCA): OPCASE_3F(0xCB): OPCASE_3F(0xCC): OPCASE_3F(0xCD): OPCASE_3F(0xCE): OPCASE_3F(0xCF):
	XOR(opcode & 0xF);
	break;

   OPCASE(0x4F): NOT(); break;

   OPCASE(0x03): LSR(); break; // LSR
   OPCASE_NP(0x96): ASR(); break; // ASR
   OPCASE(0x04): ROL(); break; // ROL
   OPCASE(0x97): ROR(); break; // ROR
   OPCASE_3D(0x96): DIV2(); break; // DIV2 (TODO: semantics!)

   //
   // INC
   //
   OPCASE(0xD0): OPCASE(0xD1): OPCASE(0xD2): OPCASE(0xD3): OPCASE(0xD4): OPCASE(0xD5): OPCASE(0xD6): OPCASE(0xD7):
   OPCASE(0xD8): OPCASE(0xD9): OPCASE(0xDA): OPCASE(0xDB): OPCASE(0xDC): OPCASE(0xDD): OPCASE(0xDE): //
   {
    INC(opcode & 0xF);
   }
   break;

   //
   // DEC
   //
   OPCASE(0xE0): OPCASE(0xE1): OPCASE(0xE2): OPCASE(0xE3): OPCASE(0xE4): OPCASE(0xE5): OPCASE(0xE6): OPCASE(0xE7):
   OPCASE(0xE8): OPCASE(0xE9): OPCASE(0xEA): OPCASE(0xEB): OPCASE(0xEC): OPCASE(0xED): OPCASE(0xEE): //
   {
    DEC(opcode & 0xF);
   }
   break;

   OPCASE(0x4D): SWAP(); break; // SWAP
   OPCASE(0x95): SEX(); break; // SEX
   OPCASE(0x9E): LOB(); break; // LOB
   OPCASE(0xC0): HIB(); break; // HIB
   OPCASE(0x70): MERGE(); break; // MERGE

   OPCASE_NP(0x9F): FMULT(); break; // FMULT
   OPCASE_3D(0x9F): LMULT(); break; // LMULT

   OPCASE_NP_Xn(0x80): MULT(ReadR(opcode & 0xF)); break; // MULT Rn
   OPCASE_3E_Xn(0x80): MULT(opcode & 0xF); break; // MULT #n

   OPCASE_3D_Xn(0x80): UMULT(ReadR(opcode & 0xF)); break; // UMULT Rn
   OPCASE_3F_Xn(0x80): UMULT(opcode & 0xF); break; // UMULT #n

   //
   // Relative branching
   //
   OPCASE(0x05): RelBranch(true);	    goto SkipPrefixReset; // BRA
   OPCASE(0x06): RelBranch(FlagS == FlagV); goto SkipPrefixReset; // BGE
   OPCASE(0x07): RelBranch(FlagS != FlagV); goto SkipPrefixReset; // BLT
   OPCASE(0x08): RelBranch(!FlagZ);	    goto SkipPrefixReset; // BNE
   OPCASE(0x09): RelBranch( FlagZ);	    goto SkipPrefixReset; // BEQ
   OPCASE(0x0A): RelBranch(!FlagS);	    goto SkipPrefixReset; // BPL
   OPCASE(0x0B): RelBranch( FlagS);	    goto SkipPrefixReset; // BMI
   OPCASE(0x0C): RelBranch(!FlagC);	    goto SkipPrefixReset; // BCC
   OPCASE(0x0D): RelBranch( FlagC);	    goto SkipPrefixReset; // BCS
   OPCASE(0x0E): RelBranch(!FlagV);	    goto SkipPrefixReset; // BVC
   OPCASE(0x0F): RelBranch( FlagV);	    goto SkipPrefixReset; // BVS

   OPCASE_NP(0x98): OPCASE_NP(0x99): OPCASE_NP(0x9A): OPCASE_NP(0x9B): OPCASE_NP(0x9C): OPCASE_NP(0x9D): JMP(opcode & 0xF); break; // JMP
   OPCASE_3D(0x98): OPCASE_3D(0x99): OPCASE_3D(0x9A): OPCASE_3D(0x9B): OPCASE_3D(0x9C): OPCASE_3D(0x9D): LJMP(opcode & 0xF); break; // LJMP

   OPCASE(0x3C): LOOP(); break; // LOOP
   OPCASE(0x91): OPCASE(0x92): OPCASE(0x93): OPCASE(0x94): LINK(opcode & 0xF); break; // LINK

   OPCASE(0x3D): PrefixSL8 |= 0x1 << 8; goto SkipPrefixReset; // ALT1
   OPCASE(0x3E): PrefixSL8 |= 0x2 << 8; goto SkipPrefixReset; // ALT2
   OPCASE(0x3F): PrefixSL8 |= 0x3 << 8; goto SkipPrefixReset; // ALT3

   //
   // TO/MOVE
   //
   OPCASE_Xn(0x10):
   {
    if(PrefixB)
    {
     //
     // MOVE
     //
     const uint16 tmp = ReadR(Rs);
     WriteR(opcode & 0xF, tmp);
    }
    else
    {
     //
     // TO
     //
     Rd = opcode & 0xF;
     goto SkipPrefixReset;	// TO
    }
   }
   break;

   //
   // WITH
   //
   OPCASE_Xn(0x20):
   {
    Rs = Rd = opcode & 0xF;
    PrefixB = true;
   }
   goto SkipPrefixReset;

   //
   // FROM/MOVES
   //
   OPCASE_Xn(0xB0):
   {
    if(PrefixB)
    {
     //
     // MOVES
     //
     const uint16 tmp = ReadR(opcode & 0xF);

     FlagV = (bool)(tmp & 0x80);
     CalcSZ(tmp);

     WriteR(Rd, tmp);
    }
    else
    {
     //
     // FROM
     //
     Rs = opcode & 0xF;
     goto SkipPrefixReset;
    }
   }
   break;
  }
  //
  Rs = 0;
  Rd = 0;
  PrefixSL8 = 0;
  PrefixB = false;
  //
  SkipPrefixReset:;
 }
}

static uint32 EventHandler(uint32 timestamp)
{
 Update(timestamp);

 return timestamp + 128; //SNES_EVENT_MAXTS;
}

static void AdjustTS(int32 delta)
{
 superfx_timestamp += delta;

 pixcache_write_finish_ts = std::max<int64>(0, (int64)pixcache_write_finish_ts + delta);
 rom_read_finish_ts = std::max<int64>(0, (int64)rom_read_finish_ts + delta);
 ram_write_finish_ts = std::max<int64>(0, (int64)ram_write_finish_ts + delta);
}

static MDFN_COLD void Reset(bool powering_up)
{
 Running = false;

 memset(R, 0, sizeof(R));
 Prefetch = 0;
 PrefixSL8 = 0;
 PrefixB = false;
 Rs = 0;
 Rd = 0;

 SetPBR(0x00); //PBR = 0;
 SetCBR(0x0000); //CBR = 0;
 ROMBR = 0;
 RAMBR = 0;

 SetCLSR(0x00); //ClockSelect = false;
 SetCFGR(0x00); //MultSpeed = false;
                  //IRQMask = false;
 

 SCBR = 0;
 SCMR = 0;
 POR = 0;
 ColorData = 0;

 memset(&PixelCache, 0, sizeof(PixelCache));

 memset(CacheData, 0, sizeof(CacheData));
 memset(CacheValid, 0, sizeof(CacheValid));

 ROMBuffer = 0;
 LastRAMOffset = 0;

 FlagV = false;
 FlagS = false;
 FlagC = false;
 FlagZ = false;

 Running = false;
 IRQPending = false;

 GPRWriteLatch = 0;
 //
 //
 superfx_timestamp = CPUM.timestamp;
 rom_read_finish_ts = superfx_timestamp;
 ram_write_finish_ts = superfx_timestamp;
 pixcache_write_finish_ts = superfx_timestamp;
}

template<signed cyc, unsigned rom_offset = 0>
static DEFREAD(MainCPU_ReadLoROM)
{
 //if(MDFN_UNLIKELY(DBG_InHLRead))
 //{
 // return (Cart.ROM + rom_offset)[(A & 0x7FFF) | ((A >> 1) & 0x3F8000)];
 //}
 if(MDFN_LIKELY(!DBG_InHLRead))
 {
  if(cyc >= 0)
   CPUM.timestamp += cyc;
  else
   CPUM.timestamp += MemSelect ? MEMCYC_FAST : MEMCYC_SLOW;
 }

 return (Cart.ROM + rom_offset)[(A & 0x7FFF) | ((A >> 1) & 0x3F8000)];
}

template<signed cyc, unsigned rom_offset = 0>
static DEFREAD(MainCPU_ReadHiROM)
{
 //if(MDFN_UNLIKELY(DBG_InHLRead))
 //{
  //return (Cart.ROM + rom_offset)[A & 0x3FFFFF];
 //}

 if(MDFN_LIKELY(!DBG_InHLRead))
 {
  if(cyc >= 0)
   CPUM.timestamp += cyc;
  else
   CPUM.timestamp += MemSelect ? MEMCYC_FAST : MEMCYC_SLOW;
 }

 return (Cart.ROM + rom_offset)[A & 0x3FFFFF];
}

static DEFREAD(MainCPU_ReadRAM8K)
{
 if(Running && (SCMR & 0x18) == 0x18)
  SNES_DBG("[SuperFX] RAM read while SuperFX is running: %06x\n", A);

 if(MDFN_LIKELY(!DBG_InHLRead))
  CPUM.timestamp += MEMCYC_SLOW;
 //
 //
 const size_t raw_sram_index = (A & 0x1FFF);

 return Cart.RAM[raw_sram_index & Cart.RAM_Mask];
}

static DEFWRITE(MainCPU_WriteRAM8K)
{
 if(Running && (SCMR & 0x18) == 0x18)
  SNES_DBG("[SuperFX] RAM write while SuperFX is running: %06x %02x\n", A, V);

 CPUM.timestamp += MEMCYC_SLOW;
 //
 //
 const size_t raw_sram_index = (A & 0x1FFF);

 Cart.RAM[raw_sram_index & Cart.RAM_Mask] = V;
}

static DEFREAD(MainCPU_ReadRAM)
{
 if(Running && (SCMR & 0x18) == 0x18)
  SNES_DBG("[SuperFX] RAM read while SuperFX is running: %06x\n", A);

 if(MDFN_LIKELY(!DBG_InHLRead))
  CPUM.timestamp += MEMCYC_SLOW;
 //
 //
 const size_t raw_sram_index = A & 0x1FFFF;

 return Cart.RAM[raw_sram_index & Cart.RAM_Mask];
}

static DEFWRITE(MainCPU_WriteRAM)
{
 if(Running && (SCMR & 0x18) == 0x18)
  SNES_DBG("[SuperFX] RAM write while SuperFX is running: %06x %02x\n", A, V);

 CPUM.timestamp += MEMCYC_SLOW;
 //
 //
 const size_t raw_sram_index = A & 0x1FFFF;

 Cart.RAM[raw_sram_index & Cart.RAM_Mask] = V;
}


static DEFREAD(MainCPU_ReadGPR)
{
 if(MDFN_LIKELY(!DBG_InHLRead))
  CPUM.timestamp += MEMCYC_FAST;
 //
 //
 return ne16_rbo_le<uint8>(R, A & 0x1F);
}

static DEFWRITE(MainCPU_WriteGPR)
{
 if(Running && (SCMR & 0x18) == 0x18)
  SNES_DBG("[SuperFX] GPR write while SuperFX is running: %06x %02x\n", A, V);

 CPUM.timestamp += MEMCYC_FAST;
 //
 //
 //SNES_DBG("[SuperFX] Write GPR 0x%06x 0x%02x\n", A, V);
 if(A & 0x1)
 {
  const size_t index = (A >> 1) & 0xF;
  const uint16 tmp = (V << 8) + GPRWriteLatch;

  R[index] = tmp;

  if(index == 0xF)
  {
   Prefetch = 0x01;
   Running = true;
   PrefixSL8 = 0;
   PrefixB = 0;
   Rs = 0;
   Rd = 0;
   //assert((bool)tmp == (bool)(SCMR & 0x10));
  }
 }
 else
  GPRWriteLatch = V;
}

template<unsigned T_A>
static DEFREAD(MainCPU_ReadIO)
{
 if(MDFN_LIKELY(!DBG_InHLRead))
  CPUM.timestamp += MEMCYC_FAST;
 //
 //
 uint8 ret = 0;

 switch(T_A & 0xF)
 {
  default:
	SNES_DBG("[SuperFX] Unknown read 0x%06x\n", A);
	break;

  case 0x0:
	ret = (FlagZ << 1) | (FlagC << 2) | (FlagS << 3) | (FlagV << 4) | (Running << 5);
	break;

  case 0x1:
	ret = ((PrefixSL8 >> 8) & 0x3) | (PrefixB << 4) | (IRQPending << 7);
	if(MDFN_LIKELY(!DBG_InHLRead))
	{
	 IRQPending = false;
	 CPU_SetIRQ(false, CPU_IRQSOURCE_CART);
	}
	break;

  case 0x4:
	ret = PBR;
	break;

  case 0x6:
	ret = ROMBR;
	break;

  case 0xB:
	ret = VCR;
	break;

  case 0xC:
	ret = RAMBR;
	break;

  case 0xE:
	ret = CBR;
	break;

  case 0xF:
	ret = CBR >> 8;
	break;
 }

 return ret;
}

template<unsigned T_A>
static DEFWRITE(MainCPU_WriteIO)
{
 CPUM.timestamp += MEMCYC_FAST;
 //
 //
 //SNES_DBG("[SuperFX] IO write 0x%06x 0x%02x\n", A, V);

 switch(T_A & 0xF)
 {
  default:
	SNES_DBG("[SuperFX] Unknown write 0x%06x 0x%02x\n", A, V);
	break;

  case 0x0:
	FlagZ = (bool)(V & 0x02);
	FlagC = (bool)(V & 0x04);
	FlagS = (bool)(V & 0x08);
	FlagV = (bool)(V & 0x10);
	Running = (bool)(V & 0x20);
	if(!Running)
	 SetCBR(0x0000);
	break;
/*
  case 0x1:
	PrefixSL8 = (V & 0x03) << 8;
	PrefixB = (V >> 4) & 1;
	break;
*/
  case 0x4:
	SetPBR(V);
	break;

  case 0x7:
	SetCFGR(V);
	break;

  case 0x8:
	SCBR = V & 0x3F;
	break;

  case 0x9:
	SetCLSR(V);
	break;

  case 0xA:
	SCMR = V;
	break;
 }
}

static DEFREAD(MainCPU_ReadCache)
{
 if(MDFN_LIKELY(!DBG_InHLRead))
  CPUM.timestamp += MEMCYC_FAST;
 //
 //
 SNES_DBG("[SuperFX] Cache read %06x\n", A);
 return CacheData[A & 0x1FF];
}

static DEFWRITE(MainCPU_WriteCache)
{
 CPUM.timestamp += MEMCYC_FAST;
 //
 //
 SNES_DBG("[SuperFX] Cache write %06x %02x\n", A, V);
 const size_t index = A & 0x1FF;

 CacheData[index] = V;
 CacheValid[index >> 4] |= ((A & 0xF) == 0xF);
}

static void StateAction(StateMem* sm, const unsigned load, const bool data_only)
{
 SFORMAT StateRegs[] =
 {
  SFVAR(R),
  SFVAR(Prefetch),
  SFVAR(PrefixSL8),
  SFVAR(PrefixB),

  SFVAR(Rs),
  SFVAR(Rd),

  SFVAR(PBR),
  SFVAR(CBR),
  SFVAR(ROMBR),
  SFVAR(RAMBR),
  SFVAR(ClockSelect),
  SFVAR(MultSpeed),

  SFVAR(SCBR),
  SFVAR(SCMR),
  SFVAR(POR),
  SFVAR(ColorData),

  SFVAR(PixelCache.TagX),
  SFVAR(PixelCache.TagY),
  SFVAR(PixelCache.data),
  SFVAR(PixelCache.opaque),

  SFVAR(CacheData),
  SFVAR(CacheValid),

  SFVAR(ROMBuffer),

  SFVAR(LastRAMOffset),

  SFVAR(FlagV),
  SFVAR(FlagS),
  SFVAR(FlagC),
  SFVAR(FlagZ),
  SFVAR(Running),
  SFVAR(IRQPending),
  SFVAR(IRQMask),

  SFVAR(GPRWriteLatch),
  //
  // FIXME: save states in debugger:?
  SFVAR(superfx_timestamp),
  SFVAR(rom_read_finish_ts),
  SFVAR(ram_write_finish_ts),
  SFVAR(pixcache_write_finish_ts),

  SFEND
 };

 MDFNSS_StateAction(sm, load, data_only, StateRegs, "SuperFX");

 if(load)
 {
  SetCLSR(ClockSelect);
  SetPBR(PBR);
 }
}

void CART_SuperFX_Init(const int32 master_clock, const int32 ocmultiplier)
{
 assert(Cart.RAM_Size);

 superfx_timestamp = 0;
 rom_read_finish_ts = 0;
 ram_write_finish_ts = 0;
 pixcache_write_finish_ts = 0;

 VCR = 0x04;

 for(unsigned bank = 0x00; bank < 0x100; bank++)
 {
  if(!(bank & 0x40))
  {
   Set_A_Handlers((bank << 16) | 0x3000, (bank << 16) | 0x301F, MainCPU_ReadGPR, MainCPU_WriteGPR);

   #define SHP(a, ta) Set_A_Handlers((bank << 16) | a, MainCPU_ReadIO<ta>, MainCPU_WriteIO<ta>);
   SHP(0x3030, 0x0)
   SHP(0x3031, 0x1)
   SHP(0x3032, 0x2)
   SHP(0x3033, 0x3)
   SHP(0x3034, 0x4)
   SHP(0x3035, 0x5)
   SHP(0x3036, 0x6)
   SHP(0x3037, 0x7)
   SHP(0x3038, 0x8)
   SHP(0x3039, 0x9)
   SHP(0x303A, 0xA)
   SHP(0x303B, 0xB)
   SHP(0x303C, 0xC)
   SHP(0x303D, 0xD)
   SHP(0x303E, 0xE)
   SHP(0x303F, 0xF)
   #undef SHP

   Set_A_Handlers((bank << 16) | 0x3100, (bank << 16) | 0x32FF, MainCPU_ReadCache, MainCPU_WriteCache);
   Set_A_Handlers((bank << 16) | 0x3300, (bank << 16) | 0x34FF, MainCPU_ReadCache, MainCPU_WriteCache);

   Set_A_Handlers((bank << 16) | 0x6000, (bank << 16) | 0x7FFF, MainCPU_ReadRAM8K, MainCPU_WriteRAM8K);
  }

  if(bank < 0x40)
  {
   Set_A_Handlers((bank << 16) | 0x8000, (bank << 16) | 0xFFFF, MainCPU_ReadLoROM<MEMCYC_SLOW>, OBWrite_SLOW);
  }
  else if(bank < 0x60)
  {
   Set_A_Handlers((bank << 16) | 0x0000, (bank << 16) | 0xFFFF, MainCPU_ReadHiROM<MEMCYC_SLOW>, OBWrite_SLOW);
  }
  else if(bank < 0x70)
  {
   //Set_A_Handlers((bank << 16) | 0x0000, (bank << 16) | 0xFFFF, MainCPU_ReadHiROM<MEMCYC_SLOW>, OBWrite_SLOW);
  }
  else if(bank == 0x70 || bank == 0x71)
  {
   Set_A_Handlers((bank << 16) | 0x0000, (bank << 16) | 0xFFFF, MainCPU_ReadRAM, MainCPU_WriteRAM);
  }
  else if(bank >= 0x80 && bank < 0xC0)
  {
   Set_A_Handlers((bank << 16) | 0x8000, (bank << 16) | 0xFFFF, MainCPU_ReadLoROM<-1, 0x200000>, OBWrite_SLOW);
  }
  else if(bank >= 0xC0)
  {
   Set_A_Handlers((bank << 16) | 0x0000, (bank << 16) | 0xFFFF, MainCPU_ReadHiROM<-1, 0x200000>, OBWrite_SLOW);
  }
 }
 //
 //
 //
 for(unsigned bank = 0x00; bank < 0x80; bank++)
 {
  static const uint8 dummy = 0xFF;
  const uint8* p = &dummy;
  size_t m = 0;

  if(bank < 0x40)
  {
   p = &Cart.ROM[bank << 15];
   m = 0x7FFF;
  }
  else if(bank < 0x60)
  {
   p = &Cart.ROM[((bank - 0x40) & 0x1F) << 16];
   m = 0xFFFF;
  }
  else if(bank < 0x80)
  {
   if(Cart.RAM)
   {
    p = &Cart.RAM[(bank << 16) & Cart.RAM_Mask];
    m = 0xFFFF & Cart.RAM_Mask;
   }
  }
  ProgMemMap[bank] = p;
  ProgMemMapMask[bank] = m;
 }
 //
 //
 //
 Cart.AdjustTS = AdjustTS;
 Cart.EventHandler = EventHandler;
 Cart.Reset = Reset;
 Cart.StateAction = StateAction;
}

}
