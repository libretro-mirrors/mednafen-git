SUBDIRS	= trio
AUTOMAKE_OPTIONS = subdir-objects
DEFS = @DEFS@ @SNDFILE_CFLAGS@
DEFAULT_INCLUDES = -I$(top_builddir)/include -I$(top_srcdir)/include -I$(top_builddir)/intl

bin_PROGRAMS	=	mednafen

mednafen_SOURCES 	= 	debug.cpp error.cpp mempatcher.cpp settings.cpp endian.cpp Time.cpp mednafen.cpp git.cpp file.cpp general.cpp memory.cpp netplay.cpp state.cpp state_rewind.cpp movie.cpp player.cpp PSFLoader.cpp SSFLoader.cpp SNSFLoader.cpp SPCReader.cpp tests.cpp qtrecord.cpp Stream.cpp MemoryStream.cpp FileStream.cpp IPSPatcher.cpp
mednafen_LDADD 		= 	trio/libtrio.a
mednafen_DEPENDENCIES	=	trio/libtrio.a

if HAVE_SDL
SUBDIRS 		+=	drivers
mednafen_LDADD          +=	drivers/libmdfnsdl.a
mednafen_DEPENDENCIES   +=	drivers/libmdfnsdl.a

#
# Resource file wasn't linking in properly when it was in an archive, so we resort to this!
#
if WIN32
.rc.o:
	@WINDRES@ -o $@ $<

mednafen_SOURCES	+=	drivers/win-resource.rc
endif

endif

if DOS
SUBDIRS			+=	drivers_dos
mednafen_LDADD		+=	drivers_dos/libmdfndos.a
mednafen_DEPENDENCIES	+=	drivers_dos/libmdfndos.a
endif

SUBDIRS			+=	sexyal
mednafen_LDADD		+=	sexyal/libsexyal.a @ALSA_LIBS@ @JACK_LIBS@
mednafen_DEPENDENCIES	+=	sexyal/libsexyal.a

if HAVE_SDL
mednafen_LDADD		+=	@SDL_LIBS@
endif

include cdplay/Makefile.am.inc
include demo/Makefile.am.inc

if WANT_GB_EMU
include gb/Makefile.am.inc
endif

if WANT_GBA_EMU
include gba/Makefile.am.inc
endif

if WANT_LYNX_EMU
include lynx/Makefile.am.inc
endif

if WANT_MD_EMU
include md/Makefile.am.inc
endif

if WANT_NES_EMU
include nes/Makefile.am.inc
endif

if WANT_NGP_EMU
SUBDIRS += ngp
mednafen_LDADD          +=      ngp/libngp.a
mednafen_DEPENDENCIES   +=      ngp/libngp.a
endif

if WANT_PCE_EMU
include pce/Makefile.am.inc
endif

if WANT_PCE_FAST_EMU
include pce_fast/Makefile.am.inc
endif

if WANT_PCFX_EMU
include pcfx/Makefile.am.inc
endif

if WANT_PSX_EMU
include psx/Makefile.am.inc
endif

if WANT_SMS_EMU
include sms/Makefile.am.inc
endif

if WANT_SNES_EMU
SUBDIRS += snes
mednafen_LDADD          +=      snes/libsnes.a
mednafen_DEPENDENCIES   +=      snes/libsnes.a
endif

if WANT_SNES_FAUST_EMU
include snes_faust/Makefile.am.inc
endif

SS_SUBDIRS =
if WANT_SS_EMU
SS_SUBDIRS 		=	ss
mednafen_LDADD          +=      ss/libss.a
mednafen_DEPENDENCIES   +=      ss/libss.a
endif

if WANT_SSFPLAY_EMU
SS_SUBDIRS 		=	ss
mednafen_LDADD          +=      ss/libssfplay.a
mednafen_DEPENDENCIES   +=      ss/libssfplay.a
endif

SUBDIRS	+= $(SS_SUBDIRS)

if WANT_VB_EMU
include vb/Makefile.am.inc
endif

if WANT_WSWAN_EMU
include wswan/Makefile.am.inc
endif


if WANT_DEBUGGER
SUBDIRS += desa68
mednafen_LDADD		+=	desa68/libdesa68.a
mednafen_DEPENDENCIES	+=	desa68/libdesa68.a
endif

include hw_cpu/Makefile.am.inc
include hw_misc/Makefile.am.inc
include hw_sound/Makefile.am.inc
include hw_video/Makefile.am.inc
include cdrom/Makefile.am.inc
include sound/Makefile.am.inc
include net/Makefile.am.inc

SUBDIRS                 +=      quicklz
mednafen_LDADD          +=      quicklz/libmdfnquicklz.a
mednafen_DEPENDENCIES   +=      quicklz/libmdfnquicklz.a

include compress/Makefile.am.inc
include hash/Makefile.am.inc
include string/Makefile.am.inc
include video/Makefile.am.inc
include resampler/Makefile.am.inc
include cputest/Makefile.am.inc
include cheat_formats/Makefile.am.inc

if HAVE_EXTERNAL_TREMOR
mednafen_LDADD		+=	@TREMOR_LIBS@
else
SUBDIRS += tremor
mednafen_LDADD          +=      tremor/libvorbisidec.a
mednafen_DEPENDENCIES   +=      tremor/libvorbisidec.a
endif

if HAVE_EXTERNAL_MPCDEC
mednafen_LDADD		+=	@MPCDEC_LIBS@
else
SUBDIRS += mpcdec
mednafen_LDADD          +=      mpcdec/libmpcdec.a
mednafen_DEPENDENCIES   +=      mpcdec/libmpcdec.a
endif

mednafen_LDADD		+= 	@SNDFILE_LIBS@ @ZLIB_LIBS@ @LIBINTL@ @LIBICONV@

