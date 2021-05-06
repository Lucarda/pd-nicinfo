# library name
lib.name = nicinfo


cflags += 
ldlibs += -lpthread


class.sources = \
	nicinfo.c \






datafiles = \
  nicinfo-help.pd \
  CHANGELOG.txt \


define forWindows
  cflags += 
  ldlibs +=  -lws2_32 -liphlpapi
  datafiles += scripts/localdeps.win.sh scripts/windep.sh
endef




# include Makefile.pdlibbuilder
# (for real-world projects see the "Project Management" section
# in tips-tricks.md)
PDLIBBUILDER_DIR=./pd-lib-builder
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder


localdep_windows: install
	cd "${installpath}"; ./windep.sh nicinfo.dll
