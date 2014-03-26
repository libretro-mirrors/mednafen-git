#ifdef SMP_CPP

void SMP::add_clocks(unsigned clocks) {
  step(clocks);
  synchronize_dsp();
  synchronize_cpu();
}

void SMP::cycle_edge() {
  t0.tick();
  t1.tick();
  t2.tick();
}

template<unsigned timer_frequency>
void SMP::sSMPTimer<timer_frequency>::tick() {
    //stage 1 increment
    stage1_ticks++;
    if(stage1_ticks < timer_frequency) return;

    stage1_ticks -= timer_frequency;
    if(enabled == false) return;

    //stage 2 increment
    stage2_ticks++;

    if(stage2_ticks != target) return;

    //stage 3 increment
    stage2_ticks = 0;
    stage3_ticks++;
    stage3_ticks &= 15;
}

#endif
