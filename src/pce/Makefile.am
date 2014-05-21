AUTOMAKE_OPTIONS = subdir-objects
DEFS = -DLOCALEDIR=\"$(datadir)/locale\" @DEFS@ 
DEFAULT_INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include -I$(top_srcdir)/intl

noinst_LIBRARIES	=	libpce.a
mednafen_SOURCES 	= 	pce.cpp vce.cpp input.cpp huc.cpp pcecd.cpp hes.cpp tsushin.cpp subhw.cpp mcgenjin.cpp
mednafen_SOURCES	+=	input/gamepad.cpp input/tsushinkb.cpp input/mouse.cpp
if WANT_DEBUGGER
mednafen_SOURCES	+=	debug.cpp
endif

libpce_a_SOURCES	=	$(mednafen_SOURCES)
