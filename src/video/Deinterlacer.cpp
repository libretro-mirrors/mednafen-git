#include "video-common.h"
#include "Deinterlacer.h"

enum
{
 DEINT_BOB_OFFSET = 0,	// Code will fall-through to this case under certain conditions, too.
 DEINT_BOB,
 DEINT_WEAVE,
};

static const unsigned DeintType = DEINT_WEAVE;

Deinterlacer::Deinterlacer()
{
 FieldBuffer = NULL;

 StateValid = false;
 PrevHeight = 0;
}

Deinterlacer::~Deinterlacer()
{
 if(FieldBuffer)
 {
  delete FieldBuffer;
  FieldBuffer = NULL;
 }
}

#if 0
void Deinterlacer::SetType(unsigned dt)
{
 if(DeintType != dt)
 {
  DeintType = dt;

  LWBuffer.resize(0);
  if(FieldBuffer)
  {
   delete FieldBuffer;
   FieldBuffer = NULL;
  }
  StateValid = false;
 }
}
#endif

template<typename T>
void Deinterlacer::InternalProcess(MDFN_Surface *surface, const MDFN_Rect &DisplayRect, MDFN_Rect *LineWidths, const bool field)
{
 //
 // We need to output with LineWidths as always being valid to handle the case of horizontal resolution change between fields
 // while in interlace mode, so clear the first LineWidths entry if it's == ~0, and
 // [...]
 const bool LineWidths_In_Valid = (LineWidths[0].w != ~0);
 if(surface->h && !LineWidths_In_Valid)
 {
  LineWidths[0].x = 0;
  LineWidths[0].w = 0;
 }

 for(int y = 0; y < DisplayRect.h / 2; y++)
 {
  // [...]
  // set all relevant source line widths to the contents of DisplayRect(also simplifies the src_lw and related pointer calculation code
  // farther below.
  if(!LineWidths_In_Valid)
   LineWidths[(y * 2) + field + DisplayRect.y] = DisplayRect;

  if(StateValid && PrevHeight == DisplayRect.h && DeintType == DEINT_WEAVE)
  {
   const T* src = FieldBuffer->pix<T>() + y * FieldBuffer->pitchinpix;
   T* dest = surface->pix<T>() + ((y * 2) + (field ^ 1) + DisplayRect.y) * surface->pitchinpix;
   MDFN_Rect *dest_lw = &LineWidths[(y * 2) + (field ^ 1) + DisplayRect.y];

   dest_lw->x = 0;
   dest_lw->w = LWBuffer[y];

   memcpy(dest, src, LWBuffer[y] * sizeof(T));
  }
  else if(DeintType == DEINT_BOB)
  {
   const T* src = surface->pix<T>() + ((y * 2) + field + DisplayRect.y) * surface->pitchinpix;
   T* dest = surface->pix<T>() + ((y * 2) + (field ^ 1) + DisplayRect.y) * surface->pitchinpix;
   const MDFN_Rect *src_lw = &LineWidths[(y * 2) + field + DisplayRect.y];
   MDFN_Rect *dest_lw = &LineWidths[(y * 2) + (field ^ 1) + DisplayRect.y];

   dest_lw->x = 0;
   dest_lw->w = src_lw->w;

   memcpy(dest, src + src_lw->x, src_lw->w * sizeof(T));
  }
  else
  {
   const MDFN_Rect *src_lw = &LineWidths[(y * 2) + field + DisplayRect.y];
   const T* src = surface->pix<T>() + ((y * 2) + field + DisplayRect.y) * surface->pitchinpix + src_lw->x;
   const int32 dly = ((y * 2) + (field + 1) + DisplayRect.y);
   T* dest = surface->pix<T>() + dly * surface->pitchinpix;

   if(y == 0 && field)
   {
    T black = surface->MakeColor(0, 0, 0);
    T* dm2 = surface->pix<T>() + (dly - 2) * surface->pitchinpix;

    LineWidths[dly - 2] = *src_lw;

    for(int x = 0; x < src_lw->w; x++)
     dm2[x] = black;
   }

   if(dly < (DisplayRect.y + DisplayRect.h))
   {
    LineWidths[dly] = *src_lw;
    memcpy(dest, src, src_lw->w * sizeof(T));
   }
  }

  //
  //
  //
  //
  //
  //
  if(DeintType == DEINT_WEAVE)
  {
   const MDFN_Rect *src_lw = &LineWidths[(y * 2) + field + DisplayRect.y];
   const T* src = surface->pix<T>() + ((y * 2) + field + DisplayRect.y) * surface->pitchinpix + src_lw->x;
   T* dest = FieldBuffer->pix<T>() + y * FieldBuffer->pitchinpix;

   memcpy(dest, src, src_lw->w * sizeof(uint32));
   LWBuffer[y] = src_lw->w;

   StateValid = true;
  }
 }

 PrevHeight = DisplayRect.h;
}

void Deinterlacer::Process(MDFN_Surface *surface, const MDFN_Rect &DisplayRect, MDFN_Rect *LineWidths, const bool field)
{
 if(DeintType == DEINT_WEAVE)
 {
  if(!FieldBuffer || FieldBuffer->w < surface->w || FieldBuffer->h < (surface->h / 2))
  {
   if(FieldBuffer)
    delete FieldBuffer;

   FieldBuffer = new MDFN_Surface(NULL, surface->w, surface->h / 2, surface->w, surface->format);
   LWBuffer.resize(FieldBuffer->h);
  }
  else if(memcmp(&surface->format, &FieldBuffer->format, sizeof(MDFN_PixelFormat)))
  {
   FieldBuffer->SetFormat(surface->format, StateValid && PrevHeight == DisplayRect.h);
  }
 }

 switch(surface->format.bpp)
 {
  case 8:
	InternalProcess<uint8>(surface, DisplayRect, LineWidths, field);
	break;

  case 16:
	InternalProcess<uint16>(surface, DisplayRect, LineWidths, field);
	break;

  case 32:
	InternalProcess<uint32>(surface, DisplayRect, LineWidths, field);
	break;
 }
}

void Deinterlacer::ClearState(void)
{
 StateValid = false;
 PrevHeight = 0;
}
