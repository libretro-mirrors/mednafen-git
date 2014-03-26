class SMP : public Processor {
public:
  enum{ Threaded = true };
  alwaysinline void step(unsigned clocks);
  alwaysinline void synchronize_cpu();
  alwaysinline void synchronize_dsp();

  uint8 port_read(uint2 port) const;
  void port_write(uint2 port, uint8 data);

  void enter();
  void power() NALL_COLD;
  void reset() NALL_COLD;

  void serialize(serializer&) NALL_COLD;
  SMP() NALL_COLD;
  ~SMP() NALL_COLD;

  static const uint8 iplrom[64];

private:
  #include "memory/memory.hpp"
  #include "timing/timing.hpp"
  #include "core/core.hpp"

  struct {
    //timing
    unsigned clock_counter;
    unsigned dsp_counter;

    //$00f1
    bool iplrom_enabled;

    //$00f2
    uint8 dsp_addr;

    //$00f8,$00f9
    uint8 ram0;
    uint8 ram1;
  } status;

public:
  uint8 apuram[65536];

private:
  static void Enter();
  alwaysinline void op_step();
};

extern SMP smp;
