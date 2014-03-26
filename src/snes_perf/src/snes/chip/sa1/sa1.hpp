#include "bus/bus.hpp"

class SA1 : public Coprocessor, public CPUcore, public MMIO {
public:
  #include "dma/dma.hpp"
  #include "memory/memory.hpp"
  #include "mmio/mmio.hpp"

  struct Status {
    uint8 tick_counter;

    bool interrupt_pending;
    uint16 interrupt_vector;

    uint16 scanlines;
    uint16 vcounter;
    uint16 hcounter;
  } status;

  static void Enter();
  void enter();
  void interrupt(uint16 vector);
  void tick();

  alwaysinline void trigger_irq();
  alwaysinline void last_cycle();
  alwaysinline bool interrupt_pending();

  void init() NALL_COLD;
  void enable();
  void power() NALL_COLD;
  void reset() NALL_COLD;

  void serialize(serializer&) NALL_COLD;
  SA1() NALL_COLD;
};

extern SA1 sa1;
extern SA1Bus sa1bus;
