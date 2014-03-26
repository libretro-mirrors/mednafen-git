include nall/Makefile
snes := snes
profile := performance

fpic = -fPIC
extraflags = -O3 -fomit-frame-pointer -I. -I$(snes) $(fpic)

# profile-guided instrumentation. Compile your frontend with this as well. :)
# extraflags += -fprofile-generate --coverage

# profile-guided optimization
# extraflags += -fprofile-use

# implicit rules
compile = \
  $(strip \
    $(if $(filter %.c,$<), \
      $(CC) $(CFLAGS) $(extraflags) $1 -c $< -o $@, \
      $(if $(filter %.cpp,$<), \
        $(CXX) $(CXXFLAGS) $(extraflags) $1 -c $< -o $@ \
      ) \
    ) \
  )

all: library;

set-static:
	$(eval fpic := )

static: set-static static-library;

install: library-install;

uninstall: library-uninstall;

%.o: $<; $(call compile)
include $(snes)/Makefile

clean: 
	-@$(call delete,obj/*.o)
	-@$(call delete,obj/*.a)
	-@$(call delete,obj/*.so)
	-@$(call delete,obj/*.dylib)
	-@$(call delete,obj/*.dll)
	-@$(call delete,out/*.a)
	-@$(call delete,out/*.so)
	-@$(call delete,*.res)
	-@$(call delete,*.pgd)
	-@$(call delete,*.pgc)
	-@$(call delete,*.ilk)
	-@$(call delete,*.pdb)
	-@$(call delete,*.manifest)

archive-all:
	tar -cjf libsnes.tar.bz2 libco nall obj out snes Makefile cc.bat clean.bat sync.sh

help:;
