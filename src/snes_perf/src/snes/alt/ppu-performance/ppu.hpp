#ifndef __SNES_PPU_H
#define __SNES_PPU_H

class PPU;

class PPU : public Processor, public PPUcounter, public MMIO {
public:
  enum{ Threaded = true };
  alwaysinline void step(unsigned clocks);
  alwaysinline void synchronize_cpu();

  void latch_counters();
  bool interlace() const;
  bool overscan() const;
  bool hires() const;

  void enter();
  void power() NALL_COLD;
  void reset() NALL_COLD;
  void scanline();
  void frame();

  void layer_enable(unsigned layer, unsigned priority, bool enable) NALL_COLD;
  void set_frameskip(bool frameskip) NALL_COLD;

  void serialize(serializer&) NALL_COLD;
  PPU() NALL_COLD;
  ~PPU() NALL_COLD;

private:
  uint16 *surface;
  uint16 *output;

  #include "mmio/mmio.hpp"
  #include "window/window.hpp"
  #include "cache/cache.hpp"
  #include "background/background.hpp"
  #include "sprite/sprite.hpp"
  #include "screen/screen.hpp"

  Cache cache;
  Background bg1;
  Background bg2;
  Background bg3;
  Background bg4;
  Sprite oam;
  Screen screen;

  struct Display {
    bool interlace;
    bool overscan;
    unsigned width;
    unsigned height;
    bool frameskip;
  } display;

  static void Enter();
  void add_clocks(unsigned clocks);
  void render_scanline();

  friend class PPU::Cache;
  friend class PPU::Background;
  friend class PPU::Sprite;
  friend class PPU::Screen;
  friend class Video;
};

#if defined(DEBUGGER)
  #include "debugger/debugger.hpp"
  extern PPUDebugger ppu;
#else
  extern PPU ppu;
#endif

#endif
