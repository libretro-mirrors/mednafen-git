AUTOMAKE_OPTIONS = subdir-objects
DEFS = -DLOCALEDIR=\"$(datadir)/locale\" @DEFS@ @MATH_OPTIMIZER_FLAGS@ -funroll-loops
DEFAULT_INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/intl -I$(top_srcdir)

noinst_LIBRARIES	=	libpc.a
libpc_a_SOURCES 	= 	pc.cpp

