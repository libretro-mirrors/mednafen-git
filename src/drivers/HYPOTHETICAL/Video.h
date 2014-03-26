#ifndef __MDFN_DRIVERS_VIDEO_H
#define __MDFN_DRIVERS_VIDEO_H

class VideoDriver
{
 public:

 VideoDriver();
 ~VideoDriver();

 struct ModeParams
 {
  unsigned w;			// In and Out
  unsigned h;			// In and Out
  MDFN_PixelFormat format;	// format.bpp In, all Out
  double refresh_rate;		// In and Out
  bool fullscreen;		// In and Out
  bool double_buffered;		// In and Out
  double pixel_aspect_ratio;	// In and Out
 };

 void SetMode(const ModeParams *desired_mode, ModeParams *obtained_mode);

 void BlitSurface(const MDFN_Surface *src_surface, const MDFN_Rect *src_rect, const MDFN_Rect *dest_rect, bool source_alpha = false, unsigned ip = 0, int scanlines = 0, const MDFN_Rect *original_src_rect = NULL, int rotated = MDFN_ROTATE0);

 void Flip(void);

 // Ideas for DOS and realtime fbdev:
#if 0
 void WaitVSync(void);
 bool TestVSync(void);
 bool TestVSyncSLC(void);	// Test if vsync start happened since last call.
 uint32 GetPixClock(void);
 uint32 GetPixClockGranularity(void);
 void SetPixClock(uint32 val); 
#endif

 // static global setting and per-module setting defs here?
};


#endif
