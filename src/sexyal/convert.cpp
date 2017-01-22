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

#include "sexyal.h"
#include <mednafen/endian.h>

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "convert.h"
#include <stdlib.h>

static inline uint32 ConvertRandU32(void)
{
 static uint32 x = 123456789;
 static uint32 y = 987654321;
 static uint32 z = 43219876;
 static uint32 c = 6543217;
 uint64 t;

 x = 314527869 * x + 1234567;
 y ^= y << 5; y ^= y >> 7; y ^= y << 22;
 t = 4294584393ULL * z + c; c = t >> 32; z = t;

 return(x + y + z);
}

template<typename dsf_t, uint32 dsf>
static inline dsf_t SAMP_CONVERT(int16 in_sample)
{
 if(dsf == SEXYAL_FMT_PCMU8)
 {
  int tmp = (in_sample + 32768 + (uint8)(ConvertRandU32() & 0xFF)) >> 8;

  if(tmp < 0)
   tmp = 0;

  if(tmp > 255)
   tmp = 255;

  return(tmp);
 }

 if(dsf == SEXYAL_FMT_PCMS8)
 {
  int tmp = (in_sample + (uint8)(ConvertRandU32() & 0xFF)) >> 8;

  if(tmp < -128)
   tmp = -128;

  if(tmp > 127)
   tmp = 127;

  return(tmp);
 }

 if(dsf == SEXYAL_FMT_PCMU16)
  return(in_sample + 32768);

 if(dsf == SEXYAL_FMT_PCMS16)
  return(in_sample);

 if(dsf == SEXYAL_FMT_PCMU24)
  return((in_sample + 32768) << 8);

 if(dsf == SEXYAL_FMT_PCMS24)
  return(in_sample << 8);

 if(dsf == SEXYAL_FMT_PCMU32)
  return((uint32)(in_sample + 32768) << 16);

 if(dsf == SEXYAL_FMT_PCMS32)
  return(in_sample << 16);

 if(dsf == SEXYAL_FMT_PCMFLOAT)
  return((float)in_sample / 32768);
}


template<typename dsf_t, uint32 dsf>
static void ConvertLoop(const int16 *src, dsf_t *dest, const int src_chan, const int dest_chan, const bool dest_noninterleaved, int32 frames)
{
 if(src_chan == 1 && dest_chan == 1)
 {
  for(int i = 0; i < frames; i++)
  {
   dest[0] = SAMP_CONVERT<dsf_t, dsf>(src[0]);
   src++;
   dest++;
  }
 }
 else if(src_chan == 2 && dest_chan == 1)
 {
  for(int i = 0; i < frames; i++)
  {
   int32 mt = (src[0] + src[1]) >> 1;

   dest[0] = SAMP_CONVERT<dsf_t, dsf>(mt);
   src += 2;
   dest += 1;
  }
 }
 else if(src_chan == 1 && dest_chan >= 2)
 {
  if(dest_noninterleaved)
  {
   for(int i = 0; i < frames; i++)
   {
    dsf_t temp = SAMP_CONVERT<dsf_t, dsf>(*src);

    dest[0 * frames] = temp;
    dest[1 * frames] = temp;

    src += 1;
    dest += 1;

    for(int padc = 2; padc < dest_chan; padc++)
     dest[padc * frames] = SAMP_CONVERT<dsf_t, dsf>(0);
   }
  }
  else
  {
   for(int i = 0; i < frames; i++)
   {
    dsf_t temp = SAMP_CONVERT<dsf_t, dsf>(*src);
    dest[0] = temp;
    dest[1] = temp;
    src += 1;
    dest += 2;

    for(int padc = 2; padc < dest_chan; padc++)
     *dest++ = SAMP_CONVERT<dsf_t, dsf>(0);
   }
  }
 }
 else //if(src_chan == 2 && dest_chan >= 2)
 {
  if(dest_noninterleaved)
  {
   for(int i = 0; i < frames; i++)
   {
    dest[0 * frames] = SAMP_CONVERT<dsf_t, dsf>(src[0]);
    dest[1 * frames] = SAMP_CONVERT<dsf_t, dsf>(src[1]);
    src += 2;
    dest += 1;
   }

   for(int padc = 2; padc < dest_chan; padc++)
    dest[padc * frames] = SAMP_CONVERT<dsf_t, dsf>(0);
  }
  else
  {
   for(int i = 0; i < frames; i++)
   {
    dest[0] = SAMP_CONVERT<dsf_t, dsf>(src[0]);
    dest[1] = SAMP_CONVERT<dsf_t, dsf>(src[1]);
    src += 2;
    dest += 2;

    for(int padc = 2; padc < dest_chan; padc++)
     *dest++ = SAMP_CONVERT<dsf_t, dsf>(0);
   }
  }
 }
}


/* Only supports one input sample format right now:  SEXYAL_FMT_PCMS16 */
void SexiALI_Convert(const SexyAL_format *srcformat, const SexyAL_format *destformat, const void *vsrc, void *vdest, uint32 frames)
{
 const int16 *src = (int16 *)vsrc;

 assert(srcformat->noninterleaved == false);

 if(destformat->sampformat == srcformat->sampformat)
  if(destformat->channels == srcformat->channels)
   if(destformat->revbyteorder == srcformat->revbyteorder)
    if(destformat->noninterleaved == srcformat->noninterleaved)
    {
     memcpy(vdest, vsrc, frames * (destformat->sampformat >> 4) * destformat->channels);
     return;
    }

 switch(destformat->sampformat)
 {
  case SEXYAL_FMT_PCMU8:
	ConvertLoop<uint8, SEXYAL_FMT_PCMU8>(src, (uint8*)vdest, srcformat->channels, destformat->channels, destformat->noninterleaved, frames);
	break;

  case SEXYAL_FMT_PCMS8:
	ConvertLoop<int8, SEXYAL_FMT_PCMS8>(src, (int8*)vdest, srcformat->channels, destformat->channels, destformat->noninterleaved, frames);
	break;

  case SEXYAL_FMT_PCMU16:
	ConvertLoop<uint16, SEXYAL_FMT_PCMU16>(src, (uint16*)vdest, srcformat->channels, destformat->channels, destformat->noninterleaved, frames);
	break;

  case SEXYAL_FMT_PCMS16:
	ConvertLoop<int16, SEXYAL_FMT_PCMS16>(src, (int16*)vdest, srcformat->channels, destformat->channels, destformat->noninterleaved, frames);
	break;

  case SEXYAL_FMT_PCMU24:
	ConvertLoop<uint32, SEXYAL_FMT_PCMU24>(src, (uint32*)vdest, srcformat->channels, destformat->channels, destformat->noninterleaved, frames);
	break;

  case SEXYAL_FMT_PCMS24:
	ConvertLoop<int32, SEXYAL_FMT_PCMS24>(src, (int32*)vdest, srcformat->channels, destformat->channels, destformat->noninterleaved, frames);
	break;

  case SEXYAL_FMT_PCMU32:
	ConvertLoop<uint32, SEXYAL_FMT_PCMU32>(src, (uint32*)vdest, srcformat->channels, destformat->channels, destformat->noninterleaved, frames);
	break;

  case SEXYAL_FMT_PCMS32:
	ConvertLoop<int32, SEXYAL_FMT_PCMS32>(src, (int32*)vdest, srcformat->channels, destformat->channels, destformat->noninterleaved, frames);
	break;

  case SEXYAL_FMT_PCMFLOAT:
	ConvertLoop<float, SEXYAL_FMT_PCMFLOAT>(src, (float*)vdest, srcformat->channels, destformat->channels, destformat->noninterleaved, frames);
	break;
 }

 if(destformat->revbyteorder != srcformat->revbyteorder)
 {
  if((destformat->sampformat >> 4) == 2)
  {
   uint16 *dest = (uint16 *)vdest;
   for(uint32 x = 0; x < frames * destformat->channels; x++)
   {
    *dest = MDFN_bswap16(*dest);
    dest++;
   }
  }
  else if((destformat->sampformat >> 4) == 4)
  {
   uint32 *dest = (uint32 *)vdest;
   for(uint32 x = 0; x < frames * destformat->channels; x++)
   {
    *dest = MDFN_bswap32(*dest);
    dest++;
   }
  }

 }
}
