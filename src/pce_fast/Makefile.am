AUTOMAKE_OPTIONS = subdir-objects
DEFS = -DLOCALEDIR=\"$(datadir)/locale\" @DEFS@ 
DEFAULT_INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include -I$(top_srcdir)/intl

noinst_LIBRARIES	=	libpce_fast.a
mednafen_SOURCES 	= 	huc6280.cpp pce.cpp vdc.cpp input.cpp huc.cpp hes.cpp pcecd.cpp pcecd_drive.cpp psg.cpp

libpce_fast_a_SOURCES	=	$(mednafen_SOURCES)
