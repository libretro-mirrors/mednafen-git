/******************************************************************************/
/* Mednafen - Multi-system Emulator                                           */
/******************************************************************************/
/* text.cpp:
**  Copyright (C) 2005-2016 Mednafen Team
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
** European-centric fixed-width bitmap font text rendering, with some CJK support.
*/

#include "video-common.h"
#include <mednafen/string/ConvertUTF.h>
#include "font-data.h"

static const struct
{
        uint8 glyph_width;
        uint8 glyph_height;
	int8 extension;
        uint8 entry_bsize;
	const uint8* base_ptr;
} FontDescriptors[_MDFN_FONT_COUNT] =
{
 { 5, 7, 	-1,			sizeof(FontData5x7[0]),		&FontData5x7[0].data[0] },
 { 6, 9,	-1,			sizeof(FontData6x9[0]),		&FontData6x9[0].data[0] },
/*
 { 6, 10,	-1,			sizeof(FontData6x10[0]),	&FontData6x10[0].data[0] },
*/
 { 6, 12,	-1,			sizeof(FontData6x12[0]),	&FontData6x12[0].data[0] },
 #ifdef WANT_INTERNAL_CJK
 { 6, 13,	MDFN_FONT_12x13,	sizeof(FontData6x13[0]),	&FontData6x13[0].data[0] },
 { 9, 18,	MDFN_FONT_18x18,	sizeof(FontData9x18[0]),	&FontData9x18[0].data[0] },
 { 12, 13, 	-1,			sizeof(FontData12x13[0]),	&FontData12x13[0].data[0] },
 { 18, 18, 	-1,			sizeof(FontData18x18[0]),	&FontData18x18[0].data[0] },
 #else
 { 6, 13,	-1,			sizeof(FontData6x13[0]),	&FontData6x13[0].data[0] },
 { 9, 18,	-1,			sizeof(FontData9x18[0]),	&FontData9x18[0].data[0] },
 #endif
};

static uint16 FontDataIndexCache[_MDFN_FONT_COUNT][65536];

uint32 GetFontHeight(const unsigned fontid)
{
 return FontDescriptors[fontid].glyph_height;
}

template<typename T>
static void BuildIndexCache(const unsigned wf, const T* const fsd, const size_t fsd_count)
{
 for(size_t i = 0; i < fsd_count; i++)
  FontDataIndexCache[wf][fsd[i].glyph_num] = i;
}

void MDFN_InitFontData(void)
{
 memset(FontDataIndexCache, 0xFF, sizeof(FontDataIndexCache));

 BuildIndexCache(MDFN_FONT_5x7,        FontData5x7,  FontData5x7_Size / sizeof(font5x7));
 BuildIndexCache(MDFN_FONT_6x9,        FontData6x9,  FontData6x9_Size / sizeof(font6x9));
// BuildIndexCache(MDFN_FONT_6x10,       FontData6x10, FontData6x10_Size / sizeof(font6x10));
 BuildIndexCache(MDFN_FONT_6x12,       FontData6x12, FontData6x12_Size / sizeof(font6x12));
 BuildIndexCache(MDFN_FONT_6x13_12x13, FontData6x13, FontData6x13_Size / sizeof(font6x13));
 BuildIndexCache(MDFN_FONT_9x18_18x18, FontData9x18, FontData9x18_Size / sizeof(font9x18));

 #ifdef WANT_INTERNAL_CJK
 BuildIndexCache(MDFN_FONT_12x13,      FontData12x13, FontData12x13_Size / sizeof(font12x13));
 BuildIndexCache(MDFN_FONT_18x18,      FontData18x18, FontData18x18_Size / sizeof(font18x18));
 #endif
}

size_t utf32_strlen(UTF32 *s)
{
 size_t ret = 0;

 while(*s++) ret++;

 return(ret);
}

static uint32 DrawTextSub(const UTF32 *utf32_buf, uint32 slen, const uint8 **glyph_ptrs, uint8 *glyph_width, uint8 *glyph_ov_width, uint32 fontid)
{
 uint32 ret = 0;

 for(uint32 x = 0; x < slen; x++)
 {
  uint32 thisglyph = utf32_buf[x];
  bool GlyphFound = false;
  uint32 recurse_fontid = fontid;

  while(!GlyphFound)
  {
   if(thisglyph < 0x10000 && FontDataIndexCache[recurse_fontid][thisglyph] != 0xFFFF)
   {
    glyph_ptrs[x] = FontDescriptors[recurse_fontid].base_ptr + (FontDescriptors[recurse_fontid].entry_bsize * FontDataIndexCache[recurse_fontid][thisglyph]);
    glyph_width[x] = FontDescriptors[recurse_fontid].glyph_width;
    GlyphFound = true;
   }
   else if(FontDescriptors[recurse_fontid].extension != -1)
    recurse_fontid = FontDescriptors[recurse_fontid].extension;
   else
    break;
  }

  if(!GlyphFound)
  {
   glyph_ptrs[x] = FontDescriptors[fontid].base_ptr + (FontDescriptors[fontid].entry_bsize * FontDataIndexCache[fontid][0xFFFD]);
   glyph_width[x] = FontDescriptors[fontid].glyph_width;
  }

  if((thisglyph >= 0x0300 && thisglyph <= 0x036F) || (thisglyph >= 0xFE20 && thisglyph <= 0xFE2F))
   glyph_ov_width[x] = 0;
  //else if(MDFN_UNLIKELY(thisglyph < 0x20))
  //{
  // if(thisglyph == '\b')	(If enabling this, need to change all glyph_ov_width types to int8)
  // {
  //  glyph_width[x] = 0;
  //  glyph_ov_width[x] = std::max<int64>(-(int64)ret, -FontDescriptors[fontid].glyph_width);
  // }
  //}
  else
   glyph_ov_width[x] = glyph_width[x];

  ret += (((x + 1) == slen) ? glyph_width[x] : glyph_ov_width[x]);
 }

 return ret;
}

uint32 GetTextPixLength(const char* text, uint32 fontid)
{
 uint32 max_glyph_len = strlen((char *)text);

 if(MDFN_LIKELY(max_glyph_len > 0))
 {
  uint32 slen;
  const uint8 *glyph_ptrs[max_glyph_len];
  uint8 glyph_width[max_glyph_len];
  uint8 glyph_ov_width[max_glyph_len];

  const UTF8 *src_begin = (UTF8 *)text;
  UTF32 utf32_buf[max_glyph_len];
  UTF32 *tstart = utf32_buf;

  ConvertUTF8toUTF32(&src_begin, (UTF8*)text + max_glyph_len, &tstart, &tstart[max_glyph_len], lenientConversion);
  slen = (tstart - utf32_buf);
  return DrawTextSub(utf32_buf, slen, glyph_ptrs, glyph_width, glyph_ov_width, fontid);
 }

 return 0;
}

uint32 GetTextPixLength(const UTF32* text, uint32 fontid)
{
 uint32 max_glyph_len = utf32_strlen((UTF32 *)text);

 if(MDFN_LIKELY(max_glyph_len > 0))
 {
  uint32 slen;
  const uint8 *glyph_ptrs[max_glyph_len];
  uint8 glyph_width[max_glyph_len];
  uint8 glyph_ov_width[max_glyph_len];

  slen = utf32_strlen((UTF32 *)text);
  return DrawTextSub((UTF32*)text, slen, glyph_ptrs, glyph_width, glyph_ov_width, fontid);
 }

 return 0;
}

template<typename T>
static uint32 DoRealDraw(T* const surfp, uint32 pitch, const int32 x, const int32 y, const int32 bx0, const int32 bx1, const int32 by0, const int32 by1, uint32 fgcolor, uint32 slen, uint32 glyph_height, const uint8 *glyph_ptrs[], const uint8 glyph_width[], const uint8 glyph_ov_width[])
{
 uint32 gy_start = std::min<int64>(glyph_height, std::max<int64>(0, (int64)by0 - y));
 uint32 gy_bound = std::min<int64>(glyph_height, std::max<int64>(0, (int64)by1 - y));
 T* dest = surfp + y * pitch + x;
 uint32 ret = 0;

 for(uint32 n = 0; n < slen; n++)
 {
  uint32 gx_start = std::min<int64>(glyph_width[n], std::max<int64>(0, (int64)bx0 - x - ret));
  uint32 gx_bound = std::min<int64>(glyph_width[n], std::max<int64>(0, (int64)bx1 - x - ret));
  size_t sd_inc = (glyph_width[n] >> 3) + 1;
  const uint8* sd = glyph_ptrs[n] + (sd_inc * gy_start);
  T* dd = dest + (gy_start * pitch);

  //printf("x=%d, y=%d --- %d %d\n", x, y, gx_start, gx_bound);

  for(uint32 gy = gy_start; MDFN_LIKELY(gy < gy_bound); gy++)
  {
   for(uint32 gx = gx_start; MDFN_LIKELY(gx < gx_bound); gx++)
   {
    if((sd[gx >> 3] << (gx & 0x7)) & 0x80)
     dd[gx] = fgcolor;
   }
   dd += pitch;
   sd += sd_inc;
  }

  dest += glyph_ov_width[n];
  ret += ((n + 1) == slen) ? glyph_width[n] : glyph_ov_width[n];
 }

 return ret;
}

template<typename T>
static uint32 DrawTextBase(MDFN_Surface* surf, const MDFN_Rect* cr, int32 x, int32 y, T* text, uint32 color, uint32 shadcolor, uint32 fontid, uint32 hcenterw, const bool shadow)
{
 int32 bx0, bx1;
 int32 by0, by1;

 if(cr)
 {
  bx0 = std::max<int32>(0, cr->x);
  bx1 = std::min<int64>(surf->w, std::max<int64>(0, (int64)cr->x + cr->w));

  by0 = std::max<int32>(0, cr->y);
  by1 = std::min<int64>(surf->h, std::max<int64>(0, (int64)cr->y + cr->h));
 }
 else
 {
  bx0 = 0;
  bx1 = surf->w;

  by0 = 0;
  by1 = surf->h;
 }

 //
 //
 //
 uint32 max_glyph_len;
 uint32 slen;

 if(sizeof(T) == 4)
  max_glyph_len = slen = utf32_strlen((UTF32 *)text);
 else
  max_glyph_len = strlen((const char*)text);

 uint32 pixwidth;
 const uint8* glyph_ptrs[max_glyph_len];
 uint8 glyph_width[max_glyph_len];
 uint8 glyph_ov_width[max_glyph_len];
 UTF32 utf32_buf[(sizeof(T) == 4) ? 1 : max_glyph_len];
 UTF32* eptr;

 if(sizeof(T) == 4)
  eptr = (UTF32*)text;
 else
 {
  const UTF8* src_begin = (UTF8*)text;
  UTF32* tstart = utf32_buf;

  ConvertUTF8toUTF32(&src_begin, (UTF8*)text + max_glyph_len, &tstart, &tstart[max_glyph_len], lenientConversion);
  slen = (tstart - utf32_buf);
  eptr = utf32_buf;
 }

 pixwidth = DrawTextSub(eptr, slen, glyph_ptrs, glyph_width, glyph_ov_width, fontid);

 if(hcenterw && hcenterw > pixwidth)
  x += (int32)(hcenterw - pixwidth) / 2;

 switch(surf->format.bpp)
 {
  default:
	return 0;

  case 16:
	if(shadow)
	 DoRealDraw(surf->pix<uint16>(), surf->pitchinpix, x + 1, y + 1, bx0, bx1, by0, by1, shadcolor, slen, FontDescriptors[fontid].glyph_height, glyph_ptrs, glyph_width, glyph_ov_width);

	return DoRealDraw(surf->pix<uint16>(), surf->pitchinpix, x, y, bx0, bx1, by0, by1, color, slen, FontDescriptors[fontid].glyph_height, glyph_ptrs, glyph_width, glyph_ov_width);

  case 32:
	if(shadow)
	 DoRealDraw(surf->pix<uint32>(), surf->pitchinpix, x + 1, y + 1, bx0, bx1, by0, by1, shadcolor, slen, FontDescriptors[fontid].glyph_height, glyph_ptrs, glyph_width, glyph_ov_width);

	return DoRealDraw(surf->pix<uint32>(), surf->pitchinpix, x, y, bx0, bx1, by0, by1, color, slen, FontDescriptors[fontid].glyph_height, glyph_ptrs, glyph_width, glyph_ov_width);
 }
}

uint32 DrawText(MDFN_Surface* surf, int32 x, int32 y, const char* text, uint32 color, uint32 fontid, uint32 hcenterw)
{
 return DrawTextBase(surf, nullptr, x, y, text, color, 0, fontid, hcenterw, false);
}

uint32 DrawText(MDFN_Surface* surf, const MDFN_Rect& cr, int32 x, int32 y, const char* text, uint32 color, uint32 fontid, uint32 hcenterw)
{
 return DrawTextBase(surf, &cr, x, y, text, color, 0, fontid, hcenterw, false);
}

uint32 DrawTextShadow(MDFN_Surface* surf, int32 x, int32 y, const char* text, uint32 color, uint32 shadcolor, uint32 fontid, uint32 hcenterw)
{
 return DrawTextBase(surf, nullptr, x, y, text, color, shadcolor, fontid, hcenterw, true);
}

uint32 DrawTextShadow(MDFN_Surface* surf, const MDFN_Rect& cr, int32 x, int32 y, const char* text, uint32 color, uint32 shadcolor, uint32 fontid, uint32 hcenterw)
{
 return DrawTextBase(surf, &cr, x, y, text, color, shadcolor, fontid, hcenterw, true);
}

//
//
//
uint32 DrawText(MDFN_Surface* surf, int32 x, int32 y, const uint32* text, uint32 color, uint32 fontid, uint32 hcenterw)
{
 return DrawTextBase(surf, nullptr, x, y, text, color, 0, fontid, hcenterw, false);
}

uint32 DrawText(MDFN_Surface* surf, const MDFN_Rect& cr, int32 x, int32 y, const uint32* text, uint32 color, uint32 fontid, uint32 hcenterw)
{
 return DrawTextBase(surf, &cr, x, y, text, color, 0, fontid, hcenterw, false);
}

uint32 DrawTextShadow(MDFN_Surface* surf, int32 x, int32 y, const uint32* text, uint32 color, uint32 shadcolor, uint32 fontid, uint32 hcenterw)
{
 return DrawTextBase(surf, nullptr, x, y, text, color, shadcolor, fontid, hcenterw, true);
}

uint32 DrawTextShadow(MDFN_Surface* surf, const MDFN_Rect& cr, int32 x, int32 y, const uint32* text, uint32 color, uint32 shadcolor, uint32 fontid, uint32 hcenterw)
{
 return DrawTextBase(surf, &cr, x, y, text, color, shadcolor, fontid, hcenterw, true);
}
