############
# Makefile #
############

# library name
lib.name = guis

#add /Headers to the search path
# cflags = -I Headers

# Sources: ##############################################################

dial.class.sources := dial.c
plist.class.sources := preset.c

#########################################################################

# helpfiles, abstractions, readme

datafiles = \
$(wildcard ./examples/*.txt) \
$(wildcard *-plugin.tcl) \
preset.r.pd \
preset.s.pd \
$(wildcard *-help.pd) \
LICENSE \
README.md

#########################################################################

# Directory where Pd API m_pd.h should be found, and other Pd header files.
PDINCLUDEDIR= ./

# include Makefile.pdlibbuilder from submodule directory 'pd-lib-builder'
PDLIBBUILDER_DIR=pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder