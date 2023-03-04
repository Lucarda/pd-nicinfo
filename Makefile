# library name
lib.name = nicinfo


ldlibs += -lpthread 


class.sources = \
	nicinfo.c \


datafiles = \
  nicinfo-help.pd \
  CHANGELOG.txt \



define forWindows
  ldlibs +=  -lws2_32 -liphlpapi
endef


PDLIBBUILDER_DIR=./pd-lib-builder
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder


# On Msys2 install these packages:
#   pacman -S mingw32/mingw-w64-i686-ntldd-git 
#   pacman -S mingw64/mingw-w64-x86_64-ntldd-git
localdep_windows: install
	scripts/localdeps.win.sh "${installpath}/nicinfo.${extension}"
