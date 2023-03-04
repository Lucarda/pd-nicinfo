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
