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
 }

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
   const uint32 *src = FieldBuffer->pixels + y * FieldBuffer->pitch32;
   uint32 *dest = surface->pixels + ((y * 2) + (field ^ 1) + DisplayRect.y) * surface->pitch32;
   MDFN_Rect *dest_lw = &LineWidths[(y * 2) + (field ^ 1) + DisplayRect.y];

   dest_lw->x = 0;
   dest_lw->w = LWBuffer[y];

   memcpy(dest, src, LWBuffer[y] * sizeof(uint32));
  }
  else if(DeintType == DEINT_BOB)
  {
   const uint32 *src = surface->pixels + ((y * 2) + field + DisplayRect.y) * surface->pitch32;
   uint32 *dest = surface->pixels + ((y * 2) + (field ^ 1) + DisplayRect.y) * surface->pitch32;
   const MDFN_Rect *src_lw = &LineWidths[(y * 2) + field + DisplayRect.y];
   MDFN_Rect *dest_lw = &LineWidths[(y * 2) + (field ^ 1) + DisplayRect.y];

   dest_lw->x = 0;
   dest_lw->w = src_lw->w;

   memcpy(dest, src + src_lw->x, src_lw->w * sizeof(uint32));
  }
  else
  {
   const MDFN_Rect *src_lw = &LineWidths[(y * 2) + field + DisplayRect.y];
   const uint32 *src = surface->pixels + ((y * 2) + field + DisplayRect.y) * surface->pitch32 + src_lw->x;
   const int32 dly = ((y * 2) + (field + 1) + DisplayRect.y);
   uint32 *dest = surface->pixels + dly * surface->pitch32;

   if(y == 0 && field)
   {
    uint32 black = surface->MakeColor(0, 0, 0);

    LineWidths[dly - 2] = *src_lw;
    memset(&surface->pixels[(dly - 2) * surface->pitch32], black, src_lw->w * sizeof(uint32));
   }

   if(dly < (DisplayRect.y + DisplayRect.h))
   {
    LineWidths[dly] = *src_lw;
    memcpy(dest, src, src_lw->w * sizeof(uint32));
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
   const uint32 *src = surface->pixels + ((y * 2) + field + DisplayRect.y) * surface->pitch32 + src_lw->x;
   uint32 *dest = FieldBuffer->pixels + y * FieldBuffer->pitch32;

   memcpy(dest, src, src_lw->w * sizeof(uint32));
   LWBuffer[y] = src_lw->w;

   StateValid = true;
  }
 }

 PrevHeight = DisplayRect.h;
}


void Deinterlacer::ClearState(void)
{
 StateValid = false;
 PrevHeight = 0;
}
