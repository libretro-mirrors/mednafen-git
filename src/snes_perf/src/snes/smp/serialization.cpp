#ifdef SMP_CPP

void SMP::serialize(serializer &s) {
  Processor::serialize(s);
  core_serialize(s);

  s.integer(status.clock_counter);
  s.integer(status.dsp_counter);

  s.integer(status.iplrom_enabled);

  s.integer(status.dsp_addr);

  s.integer(status.ram0);
  s.integer(status.ram1);

  s.integer(t0.stage1_ticks);
  s.integer(t0.stage2_ticks);
  s.integer(t0.stage3_ticks);
  s.integer(t0.enabled);
  s.integer(t0.target);

  s.integer(t1.stage1_ticks);
  s.integer(t1.stage2_ticks);
  s.integer(t1.stage3_ticks);
  s.integer(t1.enabled);
  s.integer(t1.target);

  s.integer(t2.stage1_ticks);
  s.integer(t2.stage2_ticks);
  s.integer(t2.stage3_ticks);
  s.integer(t2.enabled);
  s.integer(t2.target);

  s.array(&apuram[0], 65536);
}

#endif
