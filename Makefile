############
# Makefile #
############

# library name
lib.name = gui

#add /Headers to the search path
# cflags = -I Headers

# A_Math  := Headers/audio_math.c

# Sources: ##############################################################

dial.class.sources := dial.c

#########################################################################

# helpfiles, abstractions, readme

# datafiles = \
# $(wildcard Classes/Abstractions/*.pd) \
# $(wildcard Help-files/*.pd) \
# README.md

#########################################################################

# Directory where Pd API m_pd.h should be found, and other Pd header files.
PDINCLUDEDIR= ./

# include Makefile.pdlibbuilder from submodule directory 'pd-lib-builder'
PDLIBBUILDER_DIR=pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder