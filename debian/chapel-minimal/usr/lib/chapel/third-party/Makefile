ifndef CHPL_MAKE_HOME
export CHPL_MAKE_HOME=$(shell pwd)/..
endif
include $(CHPL_MAKE_HOME)/make/Makefile.base

ifdef CHPL_DEVELOPER
DEBUG=1
export DEBUG
WARNINGS=1
export WARNINGS
else
OPTIMIZE=1
export OPTIMIZE
endif

default: all

all:

clean: FORCE

cleanall: FORCE

clobber: FORCE

depend:

test-venv: $(CHPL_VENV_TEST_REQS)
$(CHPL_VENV_TEST_REQS): $(CHPL_VENV_TEST_REQUIREMENTS_FILE)

chpldoc-venv: $(CHPL_VENV_SPHINX_BUILD)
$(CHPL_VENV_SPHINX_BUILD): $(CHPL_VENV_CHPLDOC_REQUIREMENTS_FILE)

# See gasnet/Makefile for explanation of the post-install step
gasnet: $(GASNET_INSTALL_DIR)
$(GASNET_INSTALL_DIR):

try-gmp: FORCE
ifeq ($(wildcard $(GMP_BUILD_DIR)),)
	@echo "Speculatively attempting to build gmp"
	-@$(MAKE) GMP_SPECULATIVE=yes gmp
else ifeq ($(wildcard $(GMP_H_FILE)),)
	$(info Speculative build of gmp squashed due to previous failures.)
endif

gmp: $(GMP_H_FILE)
$(GMP_H_FILE):

hwloc: $(HWLOC_INSTALL_DIR)
$(HWLOC_INSTALL_DIR):

jemalloc: $(JEMALLOC_INSTALL_DIR)
$(JEMALLOC_INSTALL_DIR):
	
libunwind: $(LIBUNWIND_INSTALL_DIR)
$(LIBUNWIND_INSTALL_DIR):

massivethreads: $(MASSIVETHREADS_INSTALL_DIR)
$(MASSIVETHREADS_INSTALL_DIR):

ifeq ($(CHPL_MAKE_HWLOC), hwloc)
qthread: $(HWLOC_INSTALL_DIR) $(QTHREAD_INSTALL_DIR)
else
qthread: $(QTHREAD_INSTALL_DIR)
endif
$(QTHREAD_INSTALL_DIR):
$(QTHREAD_ALIASES): qthread

llvm: FORCE

libhdfs3: FORCE

try-re2: FORCE
ifeq ($(wildcard $(RE2_BUILD_DIR)),)
	@echo "Speculatively attempting to build re2"
	-@$(MAKE) re2
else
ifeq ($(wildcard $(RE2_H_FILE)),)
	$(info Speculative build of re2 squashed due to previous failures.)
endif
endif

re2: $(RE2_H_FILE)
$(RE2_H_FILE):

-include Makefile.devel

FORCE:

