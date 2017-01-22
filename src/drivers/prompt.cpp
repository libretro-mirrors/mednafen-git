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

#include "main.h"
#include <trio/trio.h>
#include "prompt.h"
#include <mednafen/string/ConvertUTF.h>

HappyPrompt::HappyPrompt(void)
{
        PromptText = "";
	kb_buffer.clear();
        kb_cursor_pos = 0;
}

HappyPrompt::HappyPrompt(const std::string &ptext, const std::string &zestring)
{
        PromptText = "";
        kb_buffer.clear();
        kb_cursor_pos = 0;
	SetText(ptext);
	SetKBB(zestring);
}

HappyPrompt::~HappyPrompt()
{

}

void HappyPrompt::TheEnd(const std::string &pstring)
{


}

void HappyPrompt::SetText(const std::string &ptext)
{
 PromptText = ptext;
}

void HappyPrompt::SetKBB(const std::string &zestring)
{
 std::vector<UTF32> utf32_buffer(zestring.size() * 4, 0);
 const UTF8* sourceStart = (const UTF8*)zestring.c_str();
 const UTF8* sourceEnd = sourceStart + zestring.size();
 UTF32* targetStart = utf32_buffer.data();
 UTF32* targetEnd = targetStart + utf32_buffer.size();

 if(ConvertUTF8toUTF32(&sourceStart, sourceEnd, &targetStart, targetEnd,lenientConversion) == conversionOK)
 {
  size_t meow_count = targetStart - utf32_buffer.data();

  for(size_t x = 0; x < meow_count; x++)
  {
   kb_buffer.push_back(utf32_buffer[x]);
   kb_cursor_pos++;
  }
 }
}

void HappyPrompt::Draw(MDFN_Surface *surface, const MDFN_Rect *rect)
{
 std::vector<uint32> PromptAnswer;
 std::vector<uint32> PromptAnswerOffsets;
 std::vector<uint32> PromptAnswerWidths;
 uint32 offset_accum = 0;

 PromptAnswerOffsets.clear();
 PromptAnswerWidths.clear();

 for(unsigned int i = 0; i < kb_buffer.size(); i++)
 {
  uint32 gw;
  UTF32 tmp_str[2];

  PromptAnswer.push_back(kb_buffer[i]);
  PromptAnswerOffsets.push_back(offset_accum);

  tmp_str[0] = kb_buffer[i];
  tmp_str[1] = 0;

  gw = GetTextPixLength(tmp_str, MDFN_FONT_6x13_12x13);

  offset_accum += gw;
  PromptAnswerWidths.push_back(gw);
 }
 PromptAnswer.push_back(0);
 PromptAnswerOffsets.push_back(offset_accum);

/*
******************************
* PC                         *
* F00F                       *
******************************
*/
 const int32 box_x = ((rect->w / 2) - (6 * 39 / 2));
 const int32 box_y = ((rect->h / 2) - (4 * 13 / 2));

 MDFN_DrawFillRect(surface, box_x, box_y, 39 * 6, 4 * 13, surface->MakeColor(0x00, 0x00, 0x00, 0xFF));

 DrawText(surface, box_x, box_y + 13 * 0, "╭────────────────────────────────────╮", surface->MakeColor(0xFF,0xFF,0xFF,0xFF), MDFN_FONT_6x13_12x13);
 DrawText(surface, box_x, box_y + 13 * 1, "│                                    │", surface->MakeColor(0xFF,0xFF,0xFF,0xFF), MDFN_FONT_6x13_12x13);
 DrawText(surface, box_x, box_y + 13 * 2, "│                                    │", surface->MakeColor(0xFF,0xFF,0xFF,0xFF), MDFN_FONT_6x13_12x13);
 DrawText(surface, box_x, box_y + 13 * 3, "╰────────────────────────────────────╯", surface->MakeColor(0xFF,0xFF,0xFF,0xFF), MDFN_FONT_6x13_12x13);

 DrawText(surface, box_x + 2 * 6, box_y + 13 * 1 + 0, PromptText, surface->MakeColor(0xFF, 0x00, 0xFF, 0xFF), MDFN_FONT_6x13_12x13);
 DrawText(surface, box_x + 2 * 6, box_y + 13 * 2 + 2, &PromptAnswer[0], surface->MakeColor(0x00, 0xFF, 0x00, 0xFF), MDFN_FONT_6x13_12x13);

 if(Time::MonoMS() & 0x80)
 {
  uint32 xpos;

  xpos = PromptAnswerOffsets[kb_cursor_pos];
  if(xpos < ((39 - 4) * 6))
  {
   const char *blinky_thingy = "▉";
   if(PromptAnswerWidths.size() > kb_cursor_pos && PromptAnswerWidths[kb_cursor_pos] == 12)
    blinky_thingy = "▉▉";
   DrawText(surface, box_x + 2 * 6 + xpos, box_y + (13 * 2 + 2), blinky_thingy, surface->MakeColor(0x00, 0xFF, 0x00, 0xFF), MDFN_FONT_6x13_12x13);
  }
 }
}

void HappyPrompt::Event(const SDL_Event *event)
{
 if(event->type == SDL_KEYDOWN)
 {
  switch(event->key.keysym.sym)
  {
   case SDLK_HOME:
        kb_cursor_pos = 0;
        break;
   case SDLK_END:
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
   case SDLK_RETURN:
        {
         std::string concat_str;

         for(unsigned int i = 0; i < kb_buffer.size(); i++)
          concat_str += kb_buffer[i];

	 TheEnd(concat_str);
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
	        if(event->key.keysym.unicode)
        	{
	         kb_buffer.insert(kb_buffer.begin() + kb_cursor_pos, event->key.keysym.unicode);
        	 kb_cursor_pos++;
	        }
        	break;
  }
 }
}
