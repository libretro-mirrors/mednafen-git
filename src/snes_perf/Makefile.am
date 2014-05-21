AM_CFLAGS = @AM_CFLAGS@ @SNES_PERF_EXTRA_FLAGS@
AM_CXXFLAGS = @AM_CXXFLAGS@ @SNES_PERF_EXTRA_FLAGS@ @SNES_PERF_EXTRA_CXXFLAGS@ @MMX_CFLAGS@ @SSE_CFLAGS@ @SSE2_CFLAGS@ -finline-functions
AUTOMAKE_OPTIONS = subdir-objects
DEFS = -DLOCALEDIR=\"$(datadir)/locale\" @DEFS@ -DNOMINMAX
DEFAULT_INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include -I$(top_srcdir)/intl -I$(top_srcdir)/src/snes_perf/src -I$(top_srcdir)/src/snes_perf/src/snes

noinst_LIBRARIES	=	libsnes_perf.a

meow			=	src/snes/libsnes/libsnes.cpp interface.cpp

profile	= performance

#ifeq ($(profile),accuracy)
#  DEFS += -DPROFILE_ACCURACY
#  snescpu = cpu
#  snessmp = smp
#  snesdsp = dsp
#  snesppu = ppu
#else ifeq ($(profile),compatibility)
#  DEFS += -DPROFILE_COMPATIBILITY
#  snescpu = cpu
#  snessmp = smp
#  snesdsp = alt/dsp
#  snesppu = alt/ppu-compatibility
#else ifeq ($(profile),performance)
  DEFS += -DPROFILE_PERFORMANCE
  snescpu = alt/cpu
  snessmp = smp
  snesdsp = alt/dsp
  snesppu = alt/ppu-performance
#endif


meow	+=	src/snes/system/system.cpp
meow	+=	src/snes/memory/memory.cpp
meow	+=	src/snes/cpu/core/core.cpp
#meow	+=	src/snes/smp/core/core.cpp

meow	+=	src/snes/alt/cpu/cpu.cpp
meow	+=	src/snes/smp/smp.cpp
meow	+=	src/snes/alt/dsp/dsp.cpp
meow	+=	src/snes/alt/ppu-performance/ppu.cpp

meow	+=	src/snes/cartridge/cartridge.cpp
meow	+=	src/snes/cheat/cheat.cpp

meow	+=	src/snes/chip/supergameboy/supergameboy.cpp
meow	+=	src/snes/chip/superfx/superfx.cpp
meow	+=	src/snes/chip/sa1/sa1.cpp
meow	+=	src/snes/chip/upd77c25/upd77c25.cpp
meow	+=	src/snes/chip/bsx/bsx.cpp
meow	+=	src/snes/chip/srtc/srtc.cpp
meow	+=	src/snes/chip/sdd1/sdd1.cpp
meow	+=	src/snes/chip/spc7110/spc7110.cpp
meow	+=	src/snes/chip/cx4/cx4.cpp
meow	+=	src/snes/chip/obc1/obc1.cpp
meow	+=	src/snes/chip/st0010/st0010.cpp
meow	+=	src/snes/chip/st0011/st0011.cpp
meow	+=	src/snes/chip/st0018/st0018.cpp
meow	+=	src/snes/chip/msu1/msu1.cpp
meow	+=	src/snes/chip/serial/serial.cpp

libsnes_perf_a_SOURCES	=	$(meow)

