template<unsigned timer_frequency>
class sSMPTimer {
public:
  uint8 target;
  uint8 stage1_ticks;
  uint8 stage2_ticks;
  uint8 stage3_ticks;
  bool enabled;

  void tick();
};

sSMPTimer<128> t0;
sSMPTimer<128> t1;
sSMPTimer< 16> t2;

alwaysinline void add_clocks(unsigned clocks);
alwaysinline void cycle_edge();
