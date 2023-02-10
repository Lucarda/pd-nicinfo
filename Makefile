# library name
lib.name = nicinfo


cflags += 
ldlibs += -lpthread -lssl -lcrypto


class.sources = \
	nicinfo.c \


datafiles = \
  nicinfo-help.pd \
  CHANGELOG.txt \



# On Msys2 install these packages:
#   pacman -S mingw64/mingw-w64-x86_64-openssl
#   pacman -S mingw32/mingw-w64-i686-openssl
#   pacman -S mingw32/mingw-w64-i686-ntldd-git 
#   pacman -S mingw64/mingw-w64-x86_64-ntldd-git 
define forWindows
  cflags += 
  ldlibs +=  -lws2_32 -liphlpapi
  datafiles += 
endef

# homebrew must be installed and with it get openssl:
#   brew install openssl
define forDarwin
  cflags += -I/usr/local/opt/openssl@3/include
  ldflags += -L/usr/local/opt/openssl@3/lib
  datafiles +=
endef


# include Makefile.pdlibbuilder
# (for real-world projects see the "Project Management" section
# in tips-tricks.md)
PDLIBBUILDER_DIR=./pd-lib-builder
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder

    
#localdep_linux: install
#	scripts/localdeps.linux.sh "${installpath}/nicinfo.${extension}"

localdep_windows: install
	scripts/localdeps.win.sh "${installpath}/nicinfo.${extension}"

localdep_macos: install
	scripts/localdeps.macos.sh "${installpath}/nicinfo.${extension}"
