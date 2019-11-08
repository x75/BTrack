# Makefile to build class 'helloworld' for Pure Data.
# Needs Makefile.pdlibbuilder as helper makefile for platform-dependent build
# settings and rules.

# library name
lib.name = btrack

cflags = -DUSE_FFTW
ldlibs += -lfftw3

# input source file (class name == source file basename)
# class.sources = btrack~.cpp ../../src/BTrack.cpp ../../src/OnsetDetectionFunction.cpp
btrack~.class.sources = btrack~.cpp ../../src/BTrack.cpp ../../src/OnsetDetectionFunction.cpp

# all extra files to be included in binary distribution of the library
datafiles = btrack~-help.pd

# include Makefile.pdlibbuilder from submodule directory 'pd-lib-builder'
PDLIBBUILDER_DIR=./pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder
