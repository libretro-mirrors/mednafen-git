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

/*
 FIXME(minor thing): Due to changes made to the bsnes core on Dec 15, 2012 to reduce input latency, there is the possibility
 of 15 lines of garbage being shown at the bottom of the screen when a PAL game switches from the 239-height mode to the 224-height
 mode when the screen isn't black.
*/

#include <mednafen/mednafen.h>
#include <mednafen/md5.h>
#include <mednafen/general.h>
#include "src/snes/libsnes/libsnes.hpp"
#include <mednafen/mempatcher.h>
#include <mednafen/PSFLoader.h>
#include <mednafen/player.h>
#include <mednafen/FileStream.h>
#include <mednafen/resampler/resampler.h>
#include <vector>

static void Cleanup(void);

static SpeexResamplerState *resampler = NULL;
static int32 ResampInPos;
static int16 ResampInBuffer[4096][2];
static bool PrevFrameInterlaced;

class SNSFLoader2 : public PSFLoader
{
 public:

 SNSFLoader2(MDFNFILE *fp);
 virtual ~SNSFLoader2();

 virtual void HandleEXE(const uint8 *data, uint32 len, bool ignore_pcsp = false);
 virtual void HandleReserved(const uint8 *data, uint32 len);

 PSFTags tags;
 std::vector<uint8> ROM_Data;
};

static SNSFLoader2 *snsf_loader = NULL;

static bool InProperEmu;
static bool SoundOn;
static double SoundLastRate = 0;

static int32 CycleCounter;
static MDFN_Surface *tsurf = NULL;
static int32 *tlw = NULL;
static MDFN_Rect *tdr = NULL;
static EmulateSpecStruct *es = NULL;

static int InputType[2];
static uint8 *InputPtr[8] = { NULL };
static uint16 PadLatch[8];
static bool MultitapEnabled[2];
static bool HasPolledThisFrame;

static int16 MouseXLatch[2];
static int16 MouseYLatch[2];
static uint8 MouseBLatch[2];

static uint8 *CustomColorMap = NULL;
//static uint32 ColorMap[32768];
static std::vector<uint32> ColorMap;

static void LoadCPalette(const char *syspalname, uint8 **ptr, uint32 num_entries) MDFN_COLD;
static void LoadCPalette(const char *syspalname, uint8 **ptr, uint32 num_entries)
{
 std::string colormap_fn = MDFN_MakeFName(MDFNMKF_PALETTE, 0, syspalname).c_str();

 MDFN_printf(_("Loading custom palette from \"%s\"...\n"),  colormap_fn.c_str());
 MDFN_indent(1);

 try
 {
  FileStream fp(colormap_fn.c_str(), FileStream::MODE_READ);

  *ptr = new uint8[num_entries * 3];

  fp.read(*ptr, num_entries * 3);
 }
 catch(MDFN_Error &e)
 {
  MDFN_printf(_("Error: %s\n"), e.what());
  MDFN_indent(-1);

  if(e.GetErrno() != ENOENT)
   throw;

  return;
 }
 catch(std::exception &e)
 {
  MDFN_printf(_("Error: %s\n"), e.what());
  MDFN_indent(-1);
  throw;
 }

 MDFN_indent(-1);
}


static void BuildColorMap(MDFN_PixelFormat &format)
{
 for(int x = 0; x < 32768; x++) 
 {
  int r, g, b;

  b = (x & (0x1F <<  0)) << 3;
  g = (x & (0x1F <<  5)) >> (5 - 3);
  r = (x & (0x1F << 10)) >> (5 * 2 - 3);

  //r = ((((x >> 0) & 0x1F) * 255 + 15) / 31);
  //g = ((((x >> 5) & 0x1F) * 255 + 15) / 31);
  //b = ((((x >> 10) & 0x1F) * 255 + 15) / 31);

  if(CustomColorMap)
  {
   r = CustomColorMap[x * 3 + 0];
   g = CustomColorMap[x * 3 + 1];
   b = CustomColorMap[x * 3 + 2];
  }

  ColorMap[x] = format.MakeColor(r, g, b);
 }
}

static void video_refresh(const uint16_t *data, unsigned *line, unsigned height, bool interlaced, bool field)
{
 PrevFrameInterlaced = interlaced;

 if(snsf_loader)
  return;

 if(!tsurf || !tlw || !tdr)
  return;

 if(es->skip && !interlaced)
  return;

 if(!interlaced)
  field = 0;

 const uint16 *source_line = data;
 uint32 *dest_line = tsurf->pixels + ((interlaced && field) ? tsurf->pitch32 : 0);

 tlw[0] = 0;	// Mark line widths as valid(since field == 1 would skip it).

 for(int y = 0; y < height; y++, source_line += 512, dest_line += tsurf->pitch32 << interlaced)
 {
  tlw[(y << interlaced) + field] = line[y];

  if(line[y] == 512 && (source_line[0] & 0x8000))
  {
   //source_line[0] &= ~0x8000;

   tlw[(y << interlaced) + field] = 256;
   for(int x = 0; x < 256; x++)
   {
    uint16 p1 = source_line[(x << 1) | 0] & 0x7FFF;
    uint16 p2 = source_line[(x << 1) | 1] & 0x7FFF;

    dest_line[x] = ColorMap[(p1 + p2 - ((p1 ^ p2) & 0x0421)) >> 1];
   }
  }
  else
  {
   unsigned w = line[y];

   for(int x = 0; x < w; x++)
    dest_line[x] = ColorMap[source_line[x]];
  }
 }

 tdr->w = 256;
 tdr->h = height << interlaced;
 es->InterlaceOn = interlaced;
 es->InterlaceField = field;
}

static void audio_sample(uint16_t l_sample, uint16_t r_sample)
{
 CycleCounter++;

 if(!SoundOn)
  return;

 if(ResampInPos < 4096)
 {
#if 0
  {
   static int min=0x7FFF;
   if((int16)r_sample < min)
   {
    printf("%d\n", (int16)r_sample);
    min = (int16)r_sample;
   }
  }
#endif
  //l_sample = (rand() & 0x7FFF) - 0x4000;
  //r_sample = (rand() & 0x7FFF) - 0x4000;
  ResampInBuffer[ResampInPos][0] = (int16)l_sample;
  ResampInBuffer[ResampInPos][1] = (int16)r_sample;
  ResampInPos++;
 }
 else
 {
  MDFN_DispMessage("Buffer overflow?");
 }
}

static void input_poll(void)
{
 if(!InProperEmu)
  return;

 HasPolledThisFrame = true;

 for(int port = 0; port < 2; port++)
 {
  switch(InputType[port])
  {
   case SNES_DEVICE_JOYPAD:
	PadLatch[port] = MDFN_de16lsb(InputPtr[port]);
	break;

   case SNES_DEVICE_MULTITAP:
	for(int index = 0; index < 4; index++)
        {
         if(!index)
          PadLatch[port] = MDFN_de16lsb(InputPtr[port]);
         else
	 {
	  int pi = 2 + 3 * (port ^ 1) + (index - 1);
          PadLatch[pi] = MDFN_de16lsb(InputPtr[pi]);
	 }
        }
        break;

   case SNES_DEVICE_MOUSE:
	MouseXLatch[port] = (int32)MDFN_de32lsb(InputPtr[port] + 0);
	MouseYLatch[port] = (int32)MDFN_de32lsb(InputPtr[port] + 4);
	MouseBLatch[port] = *(uint8 *)(InputPtr[port] + 8);
	break;
  }
 }
}

static INLINE int16 sats32tos16(int32 val)
{
 if(val > 32767)
  val = 32767;
 if(val < -32768)
  val = -32768;

 return(val);
}

static int16_t input_state(bool port, unsigned device, unsigned index, unsigned id)
{
 if(!HasPolledThisFrame)
  printf("input_state(...) before input_poll() for frame, %d %d %d %d\n", port, device, index, id);

 switch(device)
 {
 	case SNES_DEVICE_JOYPAD:
	{
	  return((PadLatch[port] >> id) & 1);
	}
	break;

	case SNES_DEVICE_MULTITAP:
	{
	 if(!index)
          return((PadLatch[port] >> id) & 1);
         else
	  return((PadLatch[2 + 3 * (port ^ 1) + (index - 1)] >> id) & 1);
	}
	break;

	case SNES_DEVICE_MOUSE:
	{
	 assert(port < 2);
	 switch(id)
	 {
	  case SNES_DEVICE_ID_MOUSE_X:
		return(sats32tos16(MouseXLatch[port]));
		break;

	  case SNES_DEVICE_ID_MOUSE_Y:
		return(sats32tos16(MouseYLatch[port]));
		break;

	  case SNES_DEVICE_ID_MOUSE_LEFT:
		return((int)(bool)(MouseBLatch[port] & 1));
		break;

	  case SNES_DEVICE_ID_MOUSE_RIGHT:
		return((int)(bool)(MouseBLatch[port] & 2));
		break;
	 }
	}
	break;
 }

 return(0);
}

// For loading: Return false on fatal error during loading, or true on success(or file not found)
static bool SaveMemorySub(bool load, const char *extension, unsigned typeA = -1U, unsigned typeB = -1U)
{
 const std::string path = MDFN_MakeFName(MDFNMKF_SAV, 0, extension);
 std::vector<PtrLengthPair> MemToSave;

 uint8 *memoryA = NULL;
 uint8 *memoryB = NULL;
 uint32 memoryA_size = 0;
 uint32 memoryB_size = 0;

 if(typeA != -1U)
 {
  memoryA = snes_get_memory_data(typeA);
  memoryA_size = snes_get_memory_size(typeA);
 }

 if(typeB != -1U)
 {
  memoryB = snes_get_memory_data(typeB);
  memoryB_size = snes_get_memory_size(typeB);
 }

 if(!load)
  return(true);

 if(load)
 {
  gzFile gp;

  errno = 0;
  gp = gzopen(path.c_str(), "rb");
  if(!gp)
  {
   ErrnoHolder ene(errno);
   if(ene.Errno() == ENOENT)
    return(true);

   MDFN_PrintError(_("Error opening save file \"%s\": %s"), path.c_str(), ene.StrError());
   return(false);
  }

  if(memoryA && memoryA_size != 0)
  {
   errno = 0;
   if(gzread(gp, memoryA, memoryA_size) != memoryA_size)
   {
    ErrnoHolder ene(errno);

    MDFN_PrintError(_("Error reading save file \"%s\": %s"), path.c_str(), ene.StrError());
    return(false);
   }
  }

  if(memoryB && memoryB_size != 0)
  {
   errno = 0;
   if(gzread(gp, memoryB, memoryB_size) != memoryB_size)
   {
    ErrnoHolder ene(errno);

    MDFN_PrintError(_("Error reading save file \"%s\": %s"), path.c_str(), ene.StrError());
    return(false);
   }
  }

  gzclose(gp);

  return(true);
 }
 else
 {
  if(memoryA && memoryA_size != 0)
   MemToSave.push_back(PtrLengthPair(memoryA, memoryA_size));

  if(memoryB && memoryB_size != 0)
   MemToSave.push_back(PtrLengthPair(memoryB, memoryB_size));

  return(MDFN_DumpToFile(path.c_str(), 6, MemToSave));
 }
}

#if 0
#define SNES_MEMORY_CARTRIDGE_RAM       0
#define SNES_MEMORY_CARTRIDGE_RTC       1
#define SNES_MEMORY_BSX_RAM             2
#define SNES_MEMORY_BSX_PRAM            3
#define SNES_MEMORY_SUFAMI_TURBO_A_RAM  4
#define SNES_MEMORY_SUFAMI_TURBO_B_RAM  5
#define SNES_MEMORY_GAME_BOY_RAM        6
#define SNES_MEMORY_GAME_BOY_RTC        7
#endif

static bool SaveLoadMemory(bool load)
{
 bool ret = true;

 ret &= SaveMemorySub(load, "srm", SNES_MEMORY_CARTRIDGE_RAM);
 ret &= SaveMemorySub(load, "rtc", SNES_MEMORY_CARTRIDGE_RTC);

 ret &= SaveMemorySub(load, "brm", SNES_MEMORY_BSX_RAM);
 ret &= SaveMemorySub(load, "psr", SNES_MEMORY_BSX_PRAM);

 ret &= SaveMemorySub(load, "str", SNES_MEMORY_SUFAMI_TURBO_A_RAM, SNES_MEMORY_SUFAMI_TURBO_B_RAM);

 ret &= SaveMemorySub(load, "sav", SNES_MEMORY_GAME_BOY_RAM);
 ret &= SaveMemorySub(load, "gbr", SNES_MEMORY_GAME_BOY_RTC);

 return(ret);
}

#if 0
static bool SaveLoadMemory(bool load)
{
  if(SNES::cartridge.loaded() == false)
   return(FALSE);

  bool ret = true;

  switch(SNES::cartridge.mode())
  {
    case SNES::Cartridge::ModeNormal:
    case SNES::Cartridge::ModeBsxSlotted: 
    {
      ret &= SaveMemorySub(load, "srm", &SNES::memory::cartram);
      ret &= SaveMemorySub(load, "rtc", &SNES::memory::cartrtc);
    }
    break;

    case SNES::Cartridge::ModeBsx:
    {
      ret &= SaveMemorySub(load, "srm", &SNES::memory::bsxram );
      ret &= SaveMemorySub(load, "psr", &SNES::memory::bsxpram);
    }
    break;

    case SNES::Cartridge::ModeSufamiTurbo:
    {
     ret &= SaveMemorySub(load, "srm", &SNES::memory::stAram, &SNES::memory::stBram);
    }
    break;

    case SNES::Cartridge::ModeSuperGameBoy:
    {
     ret &= SaveMemorySub(load, "sav", &SNES::memory::gbram);
     ret &= SaveMemorySub(load, "rtc", &SNES::memory::gbrtc);
    }
    break;
  }

 return(ret);
}
#endif

static bool TestMagic(MDFNFILE *fp)
{
 if(PSFLoader::TestMagic(0x23, fp))
  return(true);

 if(strcasecmp(fp->ext, "smc") && strcasecmp(fp->ext, "swc") && strcasecmp(fp->ext, "sfc") && strcasecmp(fp->ext, "fig") &&
        strcasecmp(fp->ext, "bs") && strcasecmp(fp->ext, "st"))
 {
  return(false);
 }

 return(true);
}

static void SetupInit(void)
{
 snes_init();

 snes_set_video_refresh(video_refresh);
 snes_set_audio_sample(audio_sample);
 snes_set_input_poll(input_poll);
 snes_set_input_state(input_state);
}

static void SetupMisc(bool PAL)
{
 PrevFrameInterlaced = false;

 //SNES::video.set_mode(PAL ? SNES::Video::ModePAL : SNES::Video::ModeNTSC);

 // Nominal FPS values are a bit off, FIXME(and contemplate the effect on netplay sound buffer overruns/underruns)
 MDFNGameInfo->fps = PAL ? 838977920 : 1008307711;
 MDFNGameInfo->MasterClock = MDFN_MASTERCLOCK_FIXED(32040.5);

 if(!snsf_loader)
 {
  MDFNGameInfo->nominal_width = MDFN_GetSettingB("snes_perf.correct_aspect") ? (PAL ? 344/*354*/ : 292) : 256;
  MDFNGameInfo->nominal_height = PAL ? 239 : 224;
  MDFNGameInfo->lcm_height = MDFNGameInfo->nominal_height * 2;
 }

 ResampInPos = 0;
 SoundLastRate = 0;
}

SNSFLoader2::SNSFLoader2(MDFNFILE *fp)
{
 uint32 size_tmp;
 uint8 *export_ptr;

 tags = Load(0x23, 8 + 1024 * 8192, fp);

 size_tmp = ROM_Data.size();

 assert(size_tmp <= (8192 * 1024));

 snes_load_cartridge_normal(NULL, &ROM_Data[0], size_tmp);
 ROM_Data.resize(0);
}

SNSFLoader2::~SNSFLoader2()
{

}

void SNSFLoader2::HandleReserved(const uint8 *data, uint32 len)
{
 uint32 o = 0;

 if(len < 9)
  return;

 while((o + 8) <= len)
 {
  uint32 header_type = MDFN_de32lsb(&data[o + 0]);
  uint32 header_size = MDFN_de32lsb(&data[o + 4]);

  printf("%08x %08x\n", header_type, header_size);

  o += 8;

  switch(header_type)
  {
   case 0xFFFFFFFF:	// EOR
	if(header_size)
	{
	 throw MDFN_Error(0, _("SNSF Reserved Section EOR has non-zero(=%u) size."), header_size);
	}

	if(o < len)
	{
	 throw MDFN_Error(0, _("SNSF Reserved Section EOR, but more data(%u bytes) available."), len - o);
	}
	break;

   default:
	throw MDFN_Error(0, _("SNSF Reserved Section Unknown/Unsupported Data Type 0x%08x"), header_type);
	break;

   case 0:	// SRAM
	{
	 uint32 srd_offset, srd_size;

	 if((len - o) < 4)
	 {
	  throw MDFN_Error(0, _("SNSF Reserved Section SRAM block, insufficient data for subheader."));
	 }
	 srd_offset = MDFN_de32lsb(&data[o]);
	 o += 4;
	 srd_size = len - o;

	 if(srd_size > 0x20000)
	 {
	  throw MDFN_Error(0, _("SNSF Reserved Section SRAM block size(=%u) is too large."), srd_size);
	 }

	 if(((uint64)srd_offset + srd_size) > 0x20000)
	 {
	  throw MDFN_Error(0, _("SNSF Reserved Section SRAM block combined offset+size(=%ull) is too large."), (unsigned long long)srd_offset + srd_size);
	 }

	 printf("SRAM(not implemented yet): %08x %08x\n", srd_offset, srd_size);
	//printf("%d\n", SNES::memory::cartram.size());
	}
	break;
  }


  o += header_size;
 }

 printf("Reserved: %d\n", len);
}


void SNSFLoader2::HandleEXE(const uint8 *data, uint32 size, bool ignore_pcsp)
{
 if(size < 8)
 {
  throw MDFN_Error(0, _("SNSF Missing full program section header."));
 }

 const uint32 header_offset = MDFN_de32lsb(&data[0]);
 const uint32 header_size = MDFN_de32lsb(&data[4]);
 const uint8 *rdata = &data[8];

 printf("%08x %08x\n", header_offset, header_size);

 if(header_offset > (1024 * 8192))
 {
  throw MDFN_Error(0, _("SNSF Header Field Offset(=%u) is too large."), header_offset);
 }

 if(header_size > (1024 * 8192))
 {
  throw MDFN_Error(0, _("SNSF Header Field Size(=%u) is too large."), header_size);
 }

 if(((uint64)header_offset + header_size) > (1024 * 8192))
 {
  throw MDFN_Error(0, _("SNSF Combined Header Fields Offset(=%u) + Size(=%u) is too large."), header_offset, header_size);
 }

 if((size - 8) < header_size)
 {
  throw(MDFN_Error(0, _("SNSF Insufficient data(need %u bytes, have %u bytes)"), header_size, size - 8));
 }

 if((header_offset + header_size) > ROM_Data.size())
  ROM_Data.resize(header_offset + header_size, 0x00);

 memcpy(&ROM_Data[header_offset], rdata, header_size);
}

static bool LoadSNSF(MDFNFILE *fp)
{
 bool PAL = false;

 SetupInit();

 MultitapEnabled[0] = false;
 MultitapEnabled[1] = false;


 try
 {
  std::vector<std::string> SongNames;

  snsf_loader = new SNSFLoader2(fp);

  SongNames.push_back(snsf_loader->tags.GetTag("title"));

  Player_Init(1, snsf_loader->tags.GetTag("game"), snsf_loader->tags.GetTag("artist"), snsf_loader->tags.GetTag("copyright"), SongNames);
 }
 catch(std::exception &e)
 {
  MDFND_PrintError(e.what());
  Cleanup();
  return 0;
 }

 //SNES::system.power();
 PAL = snes_get_region();

 SetupMisc(PAL);

 return(true);
}

static void Cleanup(void)
{
 //SNES::memory::cartrom.map(NULL, 0); // So it delete[]s the pointer it took ownership of.

 if(CustomColorMap)
 {
  delete[] CustomColorMap;
  CustomColorMap = NULL;
 }

 if(snsf_loader)
 {
  delete snsf_loader;
  snsf_loader = NULL;
 }

 ColorMap.resize(0);

 if(resampler)
 {
  speex_resampler_destroy(resampler);
  resampler = NULL;
 }

 snes_unload_cartridge();
 snes_term();
}

static int Load(MDFNFILE *fp)
{
 bool PAL = FALSE;

 CycleCounter = 0;

 try
 {
  if(PSFLoader::TestMagic(0x23, fp))
  {
   return LoadSNSF(fp);
  }
  // Allocate 8MiB of space regardless of actual ROM image size, to prevent malformed or corrupted ROM images
  // from crashing the bsnes cart loading code.

  const uint32 header_adjust = (((fp->size & 0x7FFF) == 512) ? 512 : 0);

  if((fp->size - header_adjust) > (8192 * 1024))
  {
   throw MDFN_Error(0, _("SNES ROM image is too large."));
  }

  md5_context md5;

  md5.starts();
  md5.update(fp->data, fp->size);
  md5.update((const uint8*)"snes_perf", strlen("snes_perf"));
  md5.finish(MDFNGameInfo->MD5);

  SetupInit();

  snes_load_cartridge_normal(NULL, fp->data + header_adjust, fp->size - header_adjust);

  PAL = snes_get_region();

  SetupMisc(PAL);

  MultitapEnabled[0] = MDFN_GetSettingB("snes_perf.input.port1.multitap");
  MultitapEnabled[1] = MDFN_GetSettingB("snes_perf.input.port2.multitap");

  if(!SaveLoadMemory(true))
  {
   Cleanup();
   return(0);
  }

  //printf(" %d %d\n", FSettings.SndRate, resampler.max_write());

  MDFNMP_Init(1024, (1 << 24) / 1024);

  //MDFNMP_AddRAM(131072, 0x7E << 16, SNES::memory::wram.data());

  ColorMap.resize(32768);

  LoadCPalette(NULL, &CustomColorMap, 32768);
 }
 catch(std::exception &e)
 {
  Cleanup();
  throw;
 }

 return(1);
}

static void CloseGame(void)
{
 if(!snsf_loader)
 {
  SaveLoadMemory(false);
 }
 Cleanup();
}

static void Emulate(EmulateSpecStruct *espec)
{
 //printf("\nFrame: 0x%02x\n", *(uint8*)InputPtr[0]);

 tsurf = espec->surface;
 tlw = espec->LineWidths;
 tdr = &espec->DisplayRect;
 es = espec;

 if(!snsf_loader)
 {
  if(espec->VideoFormatChanged)
   BuildColorMap(espec->surface->format);
 }

 if(SoundLastRate != espec->SoundRate)
 {
  if(resampler)
  {
   speex_resampler_destroy(resampler);
   resampler = NULL;
  }
  int err = 0;
  int quality = MDFN_GetSettingUI("snes_perf.apu.resamp_quality");

  resampler = speex_resampler_init_frac(2, 64081, 2 * (int)(espec->SoundRate ? espec->SoundRate : 48000),
					   32040.5, (int)(espec->SoundRate ? espec->SoundRate : 48000), quality, &err);
  SoundLastRate = espec->SoundRate;

  //printf("%f ms\n", 1000.0 * speex_resampler_get_input_latency(resampler) / 32040.5);
 }

 if(!snsf_loader)
 {
  MDFNMP_ApplyPeriodicCheats();
 }

 // Make sure to trash any leftover samples, generated from system.runtosave() in save state saving, if sound is now disabled.
 if(SoundOn && !espec->SoundBuf)
 {
  ResampInPos = 0;
 }

 SoundOn = espec->SoundBuf ? true : false;

 HasPolledThisFrame = false;
 InProperEmu = TRUE;

 snes_run(espec->skip && !PrevFrameInterlaced);

 tsurf = NULL;
 tlw = NULL;
 tdr = NULL;
 es = NULL;
 InProperEmu = FALSE;

 espec->MasterCycles = CycleCounter;
 CycleCounter = 0;

 //printf("%d\n", espec->MasterCycles);

 if(espec->SoundBuf)
 {
  spx_uint32_t in_len; // "Number of input samples in the input buffer. Returns the number of samples processed. This is all per-channel."
  spx_uint32_t out_len; // "Size of the output buffer. Returns the number of samples written. This is all per-channel."

  in_len = ResampInPos;
  out_len = 524288; //8192;     // FIXME, real size.

  speex_resampler_process_interleaved_int(resampler, (const spx_int16_t *)ResampInBuffer, &in_len, (spx_int16_t *)espec->SoundBuf, &out_len);

  assert(in_len <= ResampInPos);

  if((ResampInPos - in_len) > 0)
   memmove(ResampInBuffer, ResampInBuffer + in_len, (ResampInPos - in_len) * sizeof(int16) * 2);

  ResampInPos -= in_len;

  espec->SoundBufSize = out_len;
 }

 MDFNGameInfo->mouse_sensitivity = MDFN_GetSettingF("snes_perf.mouse_sensitivity");

 if(snsf_loader)
 {
  if(!espec->skip)
  {
   espec->LineWidths[0] = ~0;
   Player_Draw(espec->surface, &espec->DisplayRect, 0, espec->SoundBuf, espec->SoundBufSize);
  }
 }


#if 0
 {
  static int skipframe = 3;

  if(skipframe)
   skipframe--;
  else
  {
   static unsigned fc = 0;
   static uint64 cc = 0;
   static uint64 cc2 = 0;

   fc++;
   cc += espec->MasterCycles;
   cc2 += espec->SoundBufSize;

   printf("%f %f\n", (double)fc / ((double)cc / 32040.5), (double)fc / ((double)cc2 / espec->SoundRate));
  }
 }
#endif
}

static int StateAction(StateMem *sm, int load, int data_only)
{
 const uint32 length = snes_serialize_size();
 uint8 *ptr = NULL;

 if(!(ptr = (uint8 *)MDFN_calloc(1, length, _("SNES save state buffer"))))
  return(0);

 if(load)
 {
  SFORMAT StateRegs[] =
  {
   SFARRAYN(ptr, length, "SpidersSpidersEverywhere"),
   SFARRAY16(PadLatch, 8),
   SFARRAY16(MouseXLatch, 2),
   SFARRAY16(MouseYLatch, 2),
   SFARRAY(MouseBLatch, 2),
   SFEND
  };

  if(!MDFNSS_StateAction(sm, 1, data_only, StateRegs, "DATA"))
  {
   free(ptr);
   return(0);
  }

  if(!snes_unserialize(ptr, length))
  {
   free(ptr);
   return(0);
  }
 }
 else // save:
 {
  snes_serialize(ptr, length);

  SFORMAT StateRegs[] =
  {
   SFARRAYN(ptr, length, "SpidersSpidersEverywhere"),
   SFARRAY16(PadLatch, 8),
   SFARRAY16(MouseXLatch, 2),
   SFARRAY16(MouseYLatch, 2),
   SFARRAY(MouseBLatch, 2),
   SFEND
  };

  if(!MDFNSS_StateAction(sm, 0, data_only, StateRegs, "DATA"))
  {
   free(ptr);
   return(0);
  }
 }

 if(ptr)
 {
  free(ptr);
  ptr = NULL;
 }

 return(1);
}

struct StrToBSIT_t
{
 const char *str;
 const int id;
};

static const StrToBSIT_t StrToBSIT[] =
{
 { "none",   	SNES_DEVICE_NONE },
 { "gamepad",   SNES_DEVICE_JOYPAD },
 { "multitap",  SNES_DEVICE_MULTITAP },
 { "mouse",   	SNES_DEVICE_MOUSE },
 { "superscope",   SNES_DEVICE_SUPER_SCOPE },
 { "justifier",   SNES_DEVICE_JUSTIFIER },
 { "justifiers",   SNES_DEVICE_JUSTIFIERS },
 { NULL,	-1	},
};


static void SetInput(int port, const char *type, void *ptr)
{
 assert(port >= 0 && port < 8);

 if(port < 2)
 {
  const StrToBSIT_t *sb = StrToBSIT;
  int id = -1;

  if(MultitapEnabled[port] && !strcmp(type, "gamepad"))
   type = "multitap";

  while(sb->str && id == -1)
  {
   if(!strcmp(type, sb->str))
    id = sb->id;
   sb++;
  }
  assert(id != -1);

  InputType[port] = id;

  snes_set_controller_port_device(port, id);
 }

 InputPtr[port] = (uint8 *)ptr;
}

static void SetLayerEnableMask(uint64 mask)
{

}


static void DoSimpleCommand(int cmd)
{
 switch(cmd)
 {
  case MDFN_MSC_RESET: snes_reset(); break;
  case MDFN_MSC_POWER: snes_power(); break;
 }
}

static const InputDeviceInputInfoStruct GamepadIDII[] =
{
 { "b", "B (center, lower)", 7, IDIT_BUTTON_CAN_RAPID, NULL },
 { "y", "Y (left)", 6, IDIT_BUTTON_CAN_RAPID, NULL },
 { "select", "SELECT", 4, IDIT_BUTTON, NULL },
 { "start", "START", 5, IDIT_BUTTON, NULL },
 { "up", "UP ↑", 0, IDIT_BUTTON, "down" },
 { "down", "DOWN ↓", 1, IDIT_BUTTON, "up" },
 { "left", "LEFT ←", 2, IDIT_BUTTON, "right" },
 { "right", "RIGHT →", 3, IDIT_BUTTON, "left" },
 { "a", "A (right)", 9, IDIT_BUTTON_CAN_RAPID, NULL },
 { "x", "X (center, upper)", 8, IDIT_BUTTON_CAN_RAPID, NULL },
 { "l", "Left Shoulder", 10, IDIT_BUTTON, NULL },
 { "r", "Right Shoulder", 11, IDIT_BUTTON, NULL },
};

static const InputDeviceInputInfoStruct MouseIDII[0x4] =
{
 { "x_axis", "X Axis", -1, IDIT_X_AXIS_REL },
 { "y_axis", "Y Axis", -1, IDIT_Y_AXIS_REL },
 { "left", "Left Button", 0, IDIT_BUTTON, NULL },
 { "right", "Right Button", 1, IDIT_BUTTON, NULL },
};

#if 0
static const InputDeviceInputInfoStruct SuperScopeIDII[] =
{

};
#endif

static InputDeviceInfoStruct InputDeviceInfoSNESPort[] =
{
 // None
 {
  "none",
  "none",
  NULL,
  NULL,
  0,
  NULL
 },

 // Gamepad
 {
  "gamepad",
  "Gamepad",
  NULL,
  NULL,
  sizeof(GamepadIDII) / sizeof(InputDeviceInputInfoStruct),
  GamepadIDII,
 },

 // Mouse
 {
  "mouse",
  "Mouse",
  NULL,
  NULL,
  sizeof(MouseIDII) / sizeof(InputDeviceInputInfoStruct),
  MouseIDII,
 },
#if 0
 {
  "superscope",
  "Super Scope",
  gettext_noop("Monkey!"),
  NULL,
  sizeof(SuperScopeIDII) / sizeof(InputDeviceInputInfoStruct),
  SuperScopeIDII
 },
#endif
};


static InputDeviceInfoStruct InputDeviceInfoTapPort[] =
{
 // Gamepad
 {
  "gamepad",
  "Gamepad",
  NULL,
  NULL,
  sizeof(GamepadIDII) / sizeof(InputDeviceInputInfoStruct),
  GamepadIDII,
 },
};


static const InputPortInfoStruct PortInfo[] =
{
 { "port1", "Port 1/1A", sizeof(InputDeviceInfoSNESPort) / sizeof(InputDeviceInfoStruct), InputDeviceInfoSNESPort, "gamepad" },
 { "port2", "Port 2/2A", sizeof(InputDeviceInfoSNESPort) / sizeof(InputDeviceInfoStruct), InputDeviceInfoSNESPort, "gamepad" },
 { "port3", "Port 2B", sizeof(InputDeviceInfoTapPort) / sizeof(InputDeviceInfoStruct), InputDeviceInfoTapPort, "gamepad" },
 { "port4", "Port 2C", sizeof(InputDeviceInfoTapPort) / sizeof(InputDeviceInfoStruct), InputDeviceInfoTapPort, "gamepad" },
 { "port5", "Port 2D", sizeof(InputDeviceInfoTapPort) / sizeof(InputDeviceInfoStruct), InputDeviceInfoTapPort, "gamepad" },
 { "port6", "Port 1B", sizeof(InputDeviceInfoTapPort) / sizeof(InputDeviceInfoStruct), InputDeviceInfoTapPort, "gamepad" },
 { "port7", "Port 1C", sizeof(InputDeviceInfoTapPort) / sizeof(InputDeviceInfoStruct), InputDeviceInfoTapPort, "gamepad" },
 { "port8", "Port 1D", sizeof(InputDeviceInfoTapPort) / sizeof(InputDeviceInfoStruct), InputDeviceInfoTapPort, "gamepad" },
};

static InputInfoStruct SNESInputInfo =
{
 sizeof(PortInfo) / sizeof(InputPortInfoStruct),
 PortInfo
};

static const MDFNSetting SNESSettings[] =
{
 { "snes_perf.input.port1.multitap", MDFNSF_EMU_STATE | MDFNSF_UNTRUSTED_SAFE, gettext_noop("Enable multitap on SNES port 1."), NULL, MDFNST_BOOL, "0", NULL, NULL },
 { "snes_perf.input.port2.multitap", MDFNSF_EMU_STATE | MDFNSF_UNTRUSTED_SAFE, gettext_noop("Enable multitap on SNES port 2."), NULL, MDFNST_BOOL, "0", NULL, NULL },

 { "snes_perf.mouse_sensitivity", MDFNSF_NOFLAGS, gettext_noop("Emulated mouse sensitivity."), NULL, MDFNST_FLOAT, "0.50", NULL, NULL, NULL },

 { "snes_perf.correct_aspect", MDFNSF_CAT_VIDEO, gettext_noop("Correct the aspect ratio."), gettext_noop("Note that regardless of this setting's value, \"512\" and \"256\" width modes will be scaled to the same dimensions for display."), MDFNST_BOOL, "0" },

 { "snes_perf.apu.resamp_quality", MDFNSF_NOFLAGS, gettext_noop("APU output resampler quality."), gettext_noop("0 is lowest quality and latency and CPU usage, 10 is highest quality and latency and CPU usage.\n\nWith a Mednafen sound output rate of about 32041Hz or higher: Quality \"0\" resampler has approximately 0.125ms of latency, quality \"5\" resampler has approximately 1.25ms of latency, and quality \"10\" resampler has approximately 3.99ms of latency."), MDFNST_UINT, "5", "0", "10" },

 { NULL }
};

static const FileExtensionSpecStruct KnownExtensions[] =
{
 { ".smc", "Super Magicom ROM Image" },
 { ".swc", "Super Wildcard ROM Image" },
 { ".sfc", "Cartridge ROM Image" },
 { ".fig", "Cartridge ROM Image" },

 { ".bs", "BS-X EEPROM Image" },
 { ".st", "Sufami Turbo Cartridge ROM Image" },

 { NULL, NULL }
};

MDFNGI EmulatedSNES_Perf =
{
 "snes_perf",
 "Super Nintendo Entertainment System/Super Famicom (Performance Core)",
 KnownExtensions,
 MODPRIO_INTERNAL_LOW,
 NULL,						// Debugger
 &SNESInputInfo,
 Load,
 TestMagic,
 NULL,
 NULL,
 CloseGame,
 SetLayerEnableMask,
 NULL,	// Layer names, null-delimited
 NULL,
 NULL,
 NULL, //InstallReadPatch,
 NULL, //RemoveReadPatches,
 NULL, //MemRead,
 NULL,
 true,
 StateAction,
 Emulate,
 SetInput,
 DoSimpleCommand,
 SNESSettings,
 0,
 0,
 FALSE, // Multires

 512,   // lcm_width
 480,   // lcm_height           (replaced in game load)
 NULL,  // Dummy

 256,   // Nominal width	(replaced in game load)
 240,   // Nominal height	(replaced in game load)
 
 512,	// Framebuffer width
 512,	// Framebuffer height

 2,     // Number of output sound channels
};


