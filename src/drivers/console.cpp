/******************************************************************************/
/* Mednafen - Multi-system Emulator                                           */
/******************************************************************************/
/* console.cpp:
**  Copyright (C) 2006-2016 Mednafen Team
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

#include "main.h"
#include "console.h"
#include <math.h>
#include "nongl.h"
#include <mednafen/string/string.h>

MDFNConsole::MDFNConsole(bool setshellstyle)
{
 kb_cursor_pos = 0;
 shellstyle = setshellstyle;
 prompt_visible = true;
 Scrolled = 0;
 opacity = 0xC0;

 ScrolledVecTarg = -1;
 LastPageSize = 0;
}

MDFNConsole::~MDFNConsole()
{

}

bool MDFNConsole::TextHook(const std::string &text)
{
 WriteLine(text);

 return(1);
}

int MDFNConsole::Event(const SDL_Event *event)
{
  switch(event->type)
  {
   case SDL_KEYDOWN:
                    if(event->key.keysym.mod & KMOD_ALT)
                     break;
                    switch(event->key.keysym.sym)
                    {
		     case SDLK_HOME:
			if(event->key.keysym.mod & KMOD_SHIFT)
			 Scrolled = -1;
			else
			 kb_cursor_pos = 0;
			break;
		     case SDLK_END:
			if(event->key.keysym.mod & KMOD_SHIFT)
			 Scrolled = 0;
			else
			 kb_cursor_pos = kb_buffer.size();
			break;

	             case SDLK_LEFT:
	                if(kb_cursor_pos)
	                 kb_cursor_pos--;
	                break;

	             case SDLK_RIGHT:
	                if(kb_cursor_pos < kb_buffer.size())
	                 kb_cursor_pos++;
	                break;

		     case SDLK_UP: 
			Scrolled = Scrolled + 1;
			break;

		     case SDLK_DOWN: 
			Scrolled = std::max<int32>(0, Scrolled - 1);
			break;

		     case SDLK_PAGEUP:
			Scrolled = (Scrolled + LastPageSize);
			break;

		     case SDLK_PAGEDOWN:
			Scrolled = std::max<int64>(0, (int64)Scrolled - LastPageSize);
			break;

                     case SDLK_RETURN:
                     {
                      std::string concat_str;
                      for(unsigned int i = 0; i < kb_buffer.size(); i++)
                       concat_str += kb_buffer[i];

		      TextHook(strdup(concat_str.c_str()));
                      kb_buffer.clear();
		      kb_cursor_pos = 0;
                     }
                     break;
	           case SDLK_BACKSPACE:
	                if(kb_buffer.size() && kb_cursor_pos)
	                {
	                  kb_buffer.erase(kb_buffer.begin() + kb_cursor_pos - 1, kb_buffer.begin() + kb_cursor_pos);
	                  kb_cursor_pos--;
	                }
	                break;
  	            case SDLK_DELETE:
	                if(kb_buffer.size() && kb_cursor_pos < kb_buffer.size())
	                {
	                 kb_buffer.erase(kb_buffer.begin() + kb_cursor_pos, kb_buffer.begin() + kb_cursor_pos + 1);
	                }
	                break;
                     default:
		     if(event->key.keysym.unicode >= 0x20)
                     {
		      const char16_t tmp = event->key.keysym.unicode;
	              kb_buffer.insert(kb_buffer.begin() + kb_cursor_pos, UTF16_to_UTF8(&tmp, 1));
	              kb_cursor_pos++;
                     }
                     break;
                    }
                    break;
  }
 return(1);
}

MDFN_Surface* MDFNConsole::Draw(const MDFN_PixelFormat& pformat, const int32 dim_w, const int32 dim_h, const unsigned fontid, const uint32 hex_color)
{
 const int32 font_height = GetFontHeight(fontid);
 const uint32 color = pformat.MakeColor((hex_color >> 16) & 0xFF, (hex_color >> 8) & 0xFF, (hex_color >> 0) & 0xFF, 0xFF);
 const uint32 shadcolor = pformat.MakeColor(0x00, 0x00, 0x01, 0xFF);
 bool scroll_resync = false;
 bool draw_scrolled_notice = false;

 if(Scrolled < 0)
 {
  scroll_resync = true;
  ScrolledVecTarg = 0;
 }

 if(!surface || surface->w != dim_w)
 {
  Scrolled = 0;

  if(ScrolledVecTarg >= 0)
   scroll_resync = true;
 }

 if(!surface || surface->w != dim_w || surface->h != dim_h || memcmp(&surface->format, &pformat, sizeof(MDFN_PixelFormat)))
 {
  surface.reset(nullptr);
  surface.reset(new MDFN_Surface(nullptr, dim_w, dim_h, dim_w, pformat));
  tmp_surface.reset(nullptr);
 }

 if(!tmp_surface || tmp_surface->h < (font_height + 1))
 {
  tmp_surface.reset(nullptr);
  tmp_surface.reset(new MDFN_Surface(nullptr, 1024, font_height + 1, 1024, pformat));
 }
 //
 //
 //
 const int32 w = surface->w;
 const int32 h = surface->h;

 surface->Fill(0, 0, 0, opacity);

 //
 //
 //
 int32 scroll_counter = 0;
 int32 destline;
 int32 vec_index = TextLog.size() - 1;

 destline = ((h - font_height) / font_height) - 1;

 if(shellstyle)
  vec_index--;
 else if(!prompt_visible)
 {
  destline = (h / font_height) - 1;
  //vec_index--;
 }

 LastPageSize = (destline + 1);

 while(destline >= 0 && vec_index >= 0)
 {
  int32 pw = GetTextPixLength(TextLog[vec_index].c_str(), fontid) + 1;

  if(pw > tmp_surface->w)
  {
   tmp_surface.reset(nullptr);
   tmp_surface.reset(new MDFN_Surface(nullptr, pw, font_height + 1, pw, pformat));
  }

  tmp_surface->Fill(0, 0, 0, opacity);
  DrawTextShadow(tmp_surface.get(), 0, 0, TextLog[vec_index], color, shadcolor, fontid);
  int32 numlines = (uint32)ceil((double)pw / w);

  while(numlines > 0 && destline >= 0)
  {
   if(scroll_resync && vec_index == ScrolledVecTarg && numlines == 1)
   {
    Scrolled = scroll_counter;
    scroll_resync = false;
   }

   if(!scroll_resync && scroll_counter >= Scrolled)
   {
    if(scroll_counter == Scrolled)
     ScrolledVecTarg = vec_index;
    //
    int32 offs = (numlines - 1) * w;
    MDFN_Rect tmp_rect, dest_rect;
    tmp_rect.x = offs;
    tmp_rect.y = 0;
    tmp_rect.h = font_height;
    tmp_rect.w = (pw - offs) > (int32)w ? w : pw - offs;

    dest_rect.x = 0;
    dest_rect.y = destline * font_height;
    dest_rect.w = tmp_rect.w;
    dest_rect.h = tmp_rect.h;

    MDFN_StretchBlitSurface(tmp_surface.get(), tmp_rect, surface.get(), dest_rect);
    destline--;
   }
   else
    draw_scrolled_notice = true;

   numlines--;
   scroll_counter++;
  }
  vec_index--;
 }

 if(prompt_visible)
 {
  std::string concat_str;

  if(shellstyle)
  {
   int t = TextLog.size() - 1;
   if(t >= 0)
    concat_str = TextLog[t];
   else
    concat_str = "";
  }
  else
   concat_str = "#>";

  for(unsigned int i = 0; i < kb_buffer.size(); i++)
  {
   if(i == kb_cursor_pos && (Time::MonoMS() & 0x100))
    concat_str += "▉";
   else
    concat_str += kb_buffer[i];
  }

  if(kb_cursor_pos == kb_buffer.size())
  {
   if(Time::MonoMS() & 0x100)
    concat_str += "▉";
   else
    concat_str += " ";
  }

  {
   int32 nw = GetTextPixLength(concat_str.c_str()) + 1;

   if(nw > tmp_surface->w)
   {
    tmp_surface.reset(nullptr);
    tmp_surface.reset(new MDFN_Surface(nullptr, nw, font_height + 1, nw, surface->format));
   }
  }

  tmp_surface->Fill(0, 0, 0, opacity);
  MDFN_Rect tmp_rect, dest_rect;
  const uint32 tpl = DrawTextShadow(tmp_surface.get(), 0, 0, concat_str, color, shadcolor, fontid);

  tmp_rect.x = 0;
  tmp_rect.y = 0;
  tmp_rect.w = std::min<uint32>(tpl, tmp_surface->w);
  tmp_rect.h = font_height;

  if(tmp_rect.w >= w)
  {
   tmp_rect.x = tmp_rect.w - w;
   tmp_rect.w -= tmp_rect.x;
  }
  dest_rect.x = 0;
  dest_rect.y = h - (font_height + 1);
  dest_rect.w = tmp_rect.w;
  dest_rect.h = tmp_rect.h;

  MDFN_StretchBlitSurface(tmp_surface.get(), tmp_rect, surface.get(), dest_rect);
 }

 if(draw_scrolled_notice)
  DrawText(surface.get(), surface->w - 8, surface->h - 14, "↕", pformat.MakeColor(0x00, 0xFF, 0x00, 0xFF), fontid);

 return surface.get();
}

void MDFNConsole::WriteLine(const std::string &text)
{
 TextLog.push_back(text);
}

void MDFNConsole::AppendLastLine(const std::string &text)
{
 if(TextLog.size()) // Should we throw an exception if this isn't true?
  TextLog[TextLog.size() - 1] += text;
}

void MDFNConsole::ShowPrompt(bool shown)
{
 prompt_visible = shown;
}
