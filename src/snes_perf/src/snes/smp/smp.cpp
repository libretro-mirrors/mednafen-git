#include <snes.hpp>

#include "core/core.cpp"

#define SMP_CPP
namespace SNES {

SMP smp;

#include "serialization.cpp"
#include "iplrom.cpp"
#include "memory/memory.cpp"
#include "timing/timing.cpp"

void SMP::step(unsigned clocks) {
  clock += clocks * (uint64)cpu.frequency;
  dsp.clock -= clocks;
}

void SMP::synchronize_cpu() {
  if(CPU::Threaded == true) {
    if(clock >= 0 && scheduler.sync.i != Scheduler::SynchronizeMode::All) co_switch(cpu.thread);
  } else {
    while(clock >= 0) cpu.enter();
  }
}

void SMP::synchronize_dsp() {
  if(DSP::Threaded == true) {
    if(dsp.clock < 0 && scheduler.sync.i != Scheduler::SynchronizeMode::All) co_switch(dsp.thread);
  } else {
    while(dsp.clock < 0) dsp.enter();
  }
}

void SMP::Enter() { smp.enter(); }

void SMP::enter() {
  while(true) {
    if(scheduler.sync.i == Scheduler::SynchronizeMode::All) {
      scheduler.exit(Scheduler::ExitReason::SynchronizeEvent);
    }

    op_step();
  }
}

void SMP::op_step() {
  do_op(op_readpc());
}

void SMP::power() {
  //targets not initialized/changed upon reset
  t0.target = 0;
  t1.target = 0;
  t2.target = 0;

  reset();
}

void SMP::reset() {
  create(Enter, system.apu_frequency());

  regs.pc = 0xffc0;
  regs.a = 0x00;
  regs.x = 0x00;
  regs.y = 0x00;
  regs.sp = 0xef;
  regs.p = 0x02;

  for(unsigned i = 0; i < 65536; i++) {
    apuram[i] = 0x00;
  }

  status.clock_counter = 0;
  status.dsp_counter = 0;

  //$00f1
  status.iplrom_enabled = true;

  //$00f2
  status.dsp_addr = 0x00;

  //$00f8,$00f9
  status.ram0 = 0x00;
  status.ram1 = 0x00;

  t0.stage1_ticks = 0;
  t1.stage1_ticks = 0;
  t2.stage1_ticks = 0;

  t0.stage2_ticks = 0;
  t1.stage2_ticks = 0;
  t2.stage2_ticks = 0;

  t0.stage3_ticks = 0;
  t1.stage3_ticks = 0;
  t2.stage3_ticks = 0;

  t0.enabled = false;
  t1.enabled = false;
  t2.enabled = false;
}

SMP::SMP() {
}

SMP::~SMP() {
}

}
