class Interface;

class System : property<System> {
public:
  struct Region{ enum e{ NTSC = 0, PAL = 1, Autodetect = 2 } i; };
  struct ExpansionPortDevice{ enum e{ None = 0, BSX = 1 } i; };

  void run();
  void runtosave();

  void init(Interface*) NALL_COLD;
  void term() NALL_COLD;
  void power() NALL_COLD;
  void reset() NALL_COLD;
  void unload() NALL_COLD;

  void frame();
  void scanline();

  //return *active* system information (settings are cached upon power-on)
  Region region;
  ExpansionPortDevice expansion;
  readonly<unsigned> cpu_frequency;
  readonly<unsigned> apu_frequency;
  readonly<unsigned> serialize_size;

  serializer serialize();
  bool unserialize(serializer&) NALL_COLD;

  System() NALL_COLD;

private:
  Interface *interface;
  void runthreadtosave();

  void serialize(serializer&) NALL_COLD;
  void serialize_all(serializer&) NALL_COLD;
  void serialize_init() NALL_COLD;

  friend class Cartridge;
  friend class Video;
  friend class Audio;
  friend class Input;

  unsigned exit_line_counter;
};

#include <video/video.hpp>
#include <audio/audio.hpp>
#include <input/input.hpp>

#include <config/config.hpp>
#include <debugger/debugger.hpp>
#include <interface/interface.hpp>
#include <scheduler/scheduler.hpp>

extern System system;
