noinst_PROGRAMS = gen68k
gen68k_SOURCES = c68k/c68kexec.c c68k/c68k.c c68k/gen68k.c
gen68k_CFLAGS = -DC68K_GEN
gen68k_LDFLAGS = -DC68K_GEN
nodist_pkgdata_DATA = c68k/c68k_op0.inc
c68k/c68k_op0.inc: gen68k
        cd c68k && ../gen68k

