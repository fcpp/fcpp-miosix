##
## Makefile for Miosix embedded OS
##
MAKEFILE_VERSION := 1.09
GCCMAJOR := $(shell arm-miosix-eabi-gcc --version | \
                    perl -e '$$_=<>;/\(GCC\) (\d+)/;print "$$1"')
## Path to kernel directory (edited by init_project_out_of_git_repo.pl)
KPATH := miosix-kernel/miosix
## Path to config directory (edited by init_project_out_of_git_repo.pl)
CONFPATH := .
include $(CONFPATH)/config/Makefile.inc

##
## List here subdirectories which contains makefiles
##
SUBDIRS := $(KPATH)

##
## List here your source files (both .s, .c and .cpp)
##
SRC :=                                              \
fcpp/src/lib/common/algorithm.cpp                   \
fcpp/src/lib/common/multitype_map.cpp               \
fcpp/src/lib/common/mutex.cpp                       \
fcpp/src/lib/common/ostream.cpp                     \
fcpp/src/lib/common/profiler.cpp                    \
fcpp/src/lib/common/random_access_map.cpp           \
fcpp/src/lib/common/serialize.cpp                   \
fcpp/src/lib/common/tagged_tuple.cpp                \
fcpp/src/lib/common/traits.cpp                      \
fcpp/src/lib/common.cpp                             \
fcpp/src/lib/component/base.cpp                     \
fcpp/src/lib/component/calculus.cpp                 \
fcpp/src/lib/component/identifier.cpp               \
fcpp/src/lib/component/logger.cpp                   \
fcpp/src/lib/component/randomizer.cpp               \
fcpp/src/lib/component/scheduler.cpp                \
fcpp/src/lib/component/storage.cpp                  \
fcpp/src/lib/component/timer.cpp                    \
fcpp/src/lib/component.cpp                          \
fcpp/src/lib/coordination/collection.cpp            \
fcpp/src/lib/coordination/election.cpp              \
fcpp/src/lib/coordination/geometry.cpp              \
fcpp/src/lib/coordination/spreading.cpp             \
fcpp/src/lib/coordination/utils.cpp                 \
fcpp/src/lib/coordination.cpp                       \
fcpp/src/lib/data/field.cpp                         \
fcpp/src/lib/data/tuple.cpp                         \
fcpp/src/lib/data/vec.cpp                           \
fcpp/src/lib/data.cpp                               \
fcpp/src/lib/deployment/hardware_connector.cpp      \
fcpp/src/lib/deployment/hardware_identifier.cpp     \
fcpp/src/lib/deployment/hardware_logger.cpp         \
fcpp/src/lib/deployment/os.cpp                      \
fcpp/src/lib/deployment.cpp                         \
fcpp/src/lib/internal/context.cpp                   \
fcpp/src/lib/internal/flat_ptr.cpp                  \
fcpp/src/lib/internal/trace.cpp                     \
fcpp/src/lib/internal/twin.cpp                      \
fcpp/src/lib/internal.cpp                           \
fcpp/src/lib/option/aggregator.cpp                  \
fcpp/src/lib/option/connect.cpp                     \
fcpp/src/lib/option/distribution.cpp                \
fcpp/src/lib/option/metric.cpp                      \
fcpp/src/lib/option/sequence.cpp                    \
fcpp/src/lib/option.cpp                             \
fcpp/src/lib/simulation/batch.cpp                   \
fcpp/src/lib/simulation/simulated_connector.cpp     \
fcpp/src/lib/simulation/simulated_positioner.cpp    \
fcpp/src/lib/simulation/spawner.cpp                 \
fcpp/src/lib/simulation.cpp                         \
fcpp/src/lib/beautify.cpp                           \
fcpp/src/lib/fcpp.cpp                               \
fcpp/src/lib/settings.cpp                           \
src/driver.cpp                                      \
src/main.cpp

##
## List here additional static libraries with relative path
##
LIBS :=

##
## List here additional include directories (in the form -Iinclude_dir)
##
INCLUDE_DIRS :=							\
-Ifcpp/src -DFCPP_SYSTEM=FCPP_SYSTEM_EMBEDDED -DFCPP_ENVIRONMENT=FCPP_ENVIRONMENT_PHYSICAL -DFCPP_TRACE=32

##############################################################################
## You should not need to modify anything below                             ##
##############################################################################

ifeq ("$(VERBOSE)","1")
Q := 
ECHO := @true
else
Q := @
ECHO := @echo
endif

## Replaces both "foo.cpp"-->"foo.o" and "foo.c"-->"foo.o"
OBJ := $(addsuffix .o, $(basename $(SRC)))

## Includes the miosix base directory for C/C++
## Always include CONFPATH first, as it overrides the config file location
CXXFLAGS := $(CXXFLAGS_BASE) -I$(CONFPATH) -I$(CONFPATH)/config/$(BOARD_INC)  \
            -I. -I$(KPATH) -I$(KPATH)/arch/common -I$(KPATH)/$(ARCH_INC)      \
            -I$(KPATH)/$(BOARD_INC) $(INCLUDE_DIRS)
CFLAGS   := $(CFLAGS_BASE)   -I$(CONFPATH) -I$(CONFPATH)/config/$(BOARD_INC)  \
            -I. -I$(KPATH) -I$(KPATH)/arch/common -I$(KPATH)/$(ARCH_INC)      \
            -I$(KPATH)/$(BOARD_INC) $(INCLUDE_DIRS)
AFLAGS   := $(AFLAGS_BASE)
LFLAGS   := $(LFLAGS_BASE)
DFLAGS   := -MMD -MP

## libmiosix.a is among stdlibs because needs to be within start/end group
STDLIBS  := -lmiosix -lstdc++ -lc -lm -lgcc

ifneq ($(GCCMAJOR),4)
	STDLIBS += -latomic
endif

LINK_LIBS := $(LIBS) -L$(KPATH) -Wl,--start-group $(STDLIBS) -Wl,--end-group

all: all-recursive main

clean: clean-recursive clean-topdir

program:
	$(PROGRAM_CMDLINE)

all-recursive:
	$(foreach i,$(SUBDIRS),$(MAKE) -C $(i)                               \
	  KPATH=$(shell perl $(KPATH)/_tools/relpath.pl $(i) $(KPATH))       \
	  CONFPATH=$(shell perl $(KPATH)/_tools/relpath.pl $(i) $(CONFPATH)) \
	  || exit 1;)

clean-recursive:
	$(foreach i,$(SUBDIRS),$(MAKE) -C $(i)                               \
	  KPATH=$(shell perl $(KPATH)/_tools/relpath.pl $(i) $(KPATH))       \
	  CONFPATH=$(shell perl $(KPATH)/_tools/relpath.pl $(i) $(CONFPATH)) \
	  clean || exit 1;)

clean-topdir:
	-rm -f $(OBJ) main.elf main.hex main.bin main.map $(OBJ:.o=.d)

main: main.elf
	$(ECHO) "[CP  ] main.hex"
	$(Q)$(CP) -O ihex   main.elf main.hex
	$(ECHO) "[CP  ] main.bin"
	$(Q)$(CP) -O binary main.elf main.bin
	$(Q)$(SZ) main.elf

main.elf: $(OBJ) all-recursive
	$(ECHO) "[LD  ] main.elf"
	$(Q)$(CXX) $(LFLAGS) -o main.elf $(OBJ) $(KPATH)/$(BOOT_FILE) $(LINK_LIBS)

%.o: %.s
	$(ECHO) "[AS  ] $<"
	$(Q)$(AS)  $(AFLAGS) $< -o $@

%.o : %.c
	$(ECHO) "[CC  ] $<"
	$(Q)$(CC)  $(DFLAGS) $(CFLAGS) $< -o $@

%.o : %.cpp
	$(ECHO) "[CXX ] $<"
	$(Q)$(CXX) $(DFLAGS) $(CXXFLAGS) $< -o $@

#pull in dependecy info for existing .o files
-include $(OBJ:.o=.d)
