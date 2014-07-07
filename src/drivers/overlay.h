#ifndef __MDFN_DRIVERS_OVERLAY_H
#define __MDFN_DRIVERS_OVERLAY_H

void OV_Blit(MDFN_Surface *src_surface, const MDFN_Rect *src_rect, const MDFN_Rect *original_src_rect,
        const SDL_Rect *dest_rect, int softscale, int scanlines, int rotated);

void OV_Kill(void);

#endif
