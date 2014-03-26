#include "SPC_DSP.h"

class DSP : public Processor, public ChipDebugger {
public:
  enum{ Threaded = false };
  alwaysinline void step(unsigned clocks);
  alwaysinline void synchronize_smp();

  uint8 read(uint8 addr);
  void write(uint8 addr, uint8 data);

  void enter();
  void power() NALL_COLD;
  void reset() NALL_COLD;

  void channel_enable(unsigned channel, bool enable) NALL_COLD;

  void serialize(serializer&) NALL_COLD;
  bool property(unsigned id, string &name, string &value) { return false; }
  DSP() NALL_COLD;

private:
  SPC_DSP spc_dsp;
  int16 samplebuffer[8192];
  bool channel_enabled[8];
};

extern DSP dsp;
