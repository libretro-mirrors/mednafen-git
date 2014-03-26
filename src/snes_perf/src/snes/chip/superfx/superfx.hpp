#include "bus/bus.hpp"

class SuperFX : public Coprocessor, public MMIO {
public:
  #include "core/core.hpp"
  #include "memory/memory.hpp"
  #include "mmio/mmio.hpp"
  #include "timing/timing.hpp"
  #include "disasm/disasm.hpp"

  static void Enter();
  void enter();
  void init() NALL_COLD;
  void enable();
  void power() NALL_COLD;
  void reset() NALL_COLD;
  void serialize(serializer&) NALL_COLD;

private:
  unsigned clockmode;
  unsigned instruction_counter;
};

extern SuperFX superfx;
extern SuperFXBus superfxbus;
