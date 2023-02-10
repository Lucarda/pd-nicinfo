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
