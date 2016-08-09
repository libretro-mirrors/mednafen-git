/******************************************************************************/
/* Mednafen - Multi-system Emulator                                           */
/******************************************************************************/
/* overlay.h:
**  Copyright (C) 2010-2016 Mednafen Team
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

#ifndef __MDFN_DRIVERS_OVERLAY_H
#define __MDFN_DRIVERS_OVERLAY_H

void OV_Blit(const MDFN_Surface *src_surface, const MDFN_Rect *src_rect, const MDFN_Rect *original_src_rect,
        const SDL_Rect *dest_rect, SDL_Surface *dest_surface, int softscale, int scanlines, int rotated);

void OV_Kill(void);

#endif
