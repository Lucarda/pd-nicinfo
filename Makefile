# library name
lib.name = nicinfo


cflags += 
ldlibs += -lpthread -lssl -lcrypto


class.sources = \
	nicinfo.c \


datafiles = \
  nicinfo-help.pd \
  CHANGELOG.txt \


define forWindows
  cflags += 
  ldlibs +=  -lws2_32 -liphlpapi
  datafiles += ./scripts/localdeps.win.sh ./scripts/windep.sh
endef




# include Makefile.pdlibbuilder
# (for real-world projects see the "Project Management" section
# in tips-tricks.md)
PDLIBBUILDER_DIR=./pd-lib-builder
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder


localdep_windows: install
	cd "${installpath}"; ./windep.sh nicinfo.$(extension)
    
#        libcrypto-1_1-x64.dll => /mingw64/bin/libcrypto-1_1-x64.dll (0x7ff877c10000)
#        libssl-1_1-x64.dll => /mingw64/bin/libssl-1_1-x64.dll (0x7ff8b0f70000)
