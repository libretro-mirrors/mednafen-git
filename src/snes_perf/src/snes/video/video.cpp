#ifdef SYSTEM_CPP

Video video;

const uint8_t Video::cursor[15 * 15] = {
  0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,
  0,0,0,0,1,1,2,2,2,1,1,0,0,0,0,
  0,0,0,1,2,2,1,2,1,2,2,1,0,0,0,
  0,0,1,2,1,1,0,1,0,1,1,2,1,0,0,
  0,1,2,1,0,0,0,1,0,0,0,1,2,1,0,
  0,1,2,1,0,0,1,2,1,0,0,1,2,1,0,
  1,2,1,0,0,1,1,2,1,1,0,0,1,2,1,
  1,2,2,1,1,2,2,2,2,2,1,1,2,2,1,
  1,2,1,0,0,1,1,2,1,1,0,0,1,2,1,
  0,1,2,1,0,0,1,2,1,0,0,1,2,1,0,
  0,1,2,1,0,0,0,1,0,0,0,1,2,1,0,
  0,0,1,2,1,1,0,1,0,1,1,2,1,0,0,
  0,0,0,1,2,2,1,2,1,2,2,1,0,0,0,
  0,0,0,0,1,1,2,2,2,1,1,0,0,0,0,
  0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,
};

void Video::draw_cursor(uint16_t color, int x, int y) {
  uint16_t *data = (uint16_t*)ppu.output;
  if(ppu.interlace() && ppu.field()) data += 512;

  for(int cy = 0; cy < 15; cy++) {
    int vy = y + cy - 7;
    if(vy <= 0 || vy >= 240) continue;  //do not draw offscreen

    bool hires = (line_width[vy] == 512);
    for(int cx = 0; cx < 15; cx++) {
      int vx = x + cx - 7;
      if(vx < 0 || vx >= 256) continue;  //do not draw offscreen
      uint8_t pixel = cursor[cy * 15 + cx];
      if(pixel == 0) continue;
      uint16_t pixelcolor = (pixel == 1) ? 0 : color;

      if(hires == false) {
        *((uint16_t*)data + vy * 1024 + vx) = pixelcolor;
      } else {
        *((uint16_t*)data + vy * 1024 + vx * 2 + 0) = pixelcolor;
        *((uint16_t*)data + vy * 1024 + vx * 2 + 1) = pixelcolor;
      }
    }
  }
}

void Video::update() {
  switch(input.port[1].device.i) {
    case Input::Device::SuperScope: draw_cursor(0x001f, input.port[1].superscope.x, input.port[1].superscope.y); break;
    case Input::Device::Justifiers: draw_cursor(0x02e0, input.port[1].justifier.x2, input.port[1].justifier.y2); //fallthrough
    case Input::Device::Justifier:  draw_cursor(0x001f, input.port[1].justifier.x1, input.port[1].justifier.y1); break;
    case Input::Device::None:
    case Input::Device::Joypad:
    case Input::Device::Mouse:
    case Input::Device::Multitap: break;
  }

  unsigned yoffset = 1;
  uint16_t *data = (uint16_t*)ppu.output;
  unsigned width = 256;
  unsigned height = !ppu.overscan() ? 224 : 239;

  system.interface->video_refresh(ppu.output + 512 * yoffset, &line_width[yoffset], height, frame_interlace, ppu.field());

  //system.interface->video_refresh(ppu.output + 1024, width, height);

  frame_hires = false;
  frame_interlace = false;
}

void Video::scanline() {
  unsigned y = cpu.vcounter();
  if(y >= 240) return;

  //if(y == 0)
  // frame_field = cpu.field();

  frame_hires |= ppu.hires();
  frame_interlace |= ppu.interlace();
  unsigned width = (ppu.hires() == false ? 256 : 512);
  line_width[y] = width;
}

void Video::init() {
  frame_hires = false;
  frame_interlace = false;
  for(unsigned i = 0; i < 240; i++) line_width[i] = 256;
}

#endif
