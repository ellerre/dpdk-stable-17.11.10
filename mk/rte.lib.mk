#   BSD LICENSE
#
#   Copyright(c) 2010-2014 Intel Corporation. All rights reserved.
#   All rights reserved.
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions
#   are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#     * Neither the name of Intel Corporation nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

include $(RTE_SDK)/mk/internal/rte.compile-pre.mk
include $(RTE_SDK)/mk/internal/rte.install-pre.mk
include $(RTE_SDK)/mk/internal/rte.clean-pre.mk
include $(RTE_SDK)/mk/internal/rte.build-pre.mk

EXTLIB_BUILD ?= n

# VPATH contains at least SRCDIR
VPATH += $(SRCDIR)

ifneq ($(CONFIG_RTE_MAJOR_ABI),)
ifneq ($(LIBABIVER),)
LIBABIVER := $(CONFIG_RTE_MAJOR_ABI)
endif
endif

ifeq ($(CONFIG_RTE_BUILD_SHARED_LIB),y)
LIB := $(patsubst %.a,%.so.$(LIBABIVER),$(LIB))
ifeq ($(EXTLIB_BUILD),n)
ifeq ($(CONFIG_RTE_MAJOR_ABI),)
ifeq ($(CONFIG_RTE_NEXT_ABI),y)
LIB := $(LIB).1
endif
endif
CPU_LDFLAGS += --version-script=$(SRCDIR)/$(EXPORT_MAP)
endif
endif


_BUILD = $(LIB)
PREINSTALL = $(SYMLINK-FILES-y)
_INSTALL = $(INSTALL-FILES-y) $(RTE_OUTPUT)/lib/$(LIB)
_CLEAN = doclean

LDLIBS += $(EXECENV_LDLIBS-y)

.PHONY: all
all: install

.PHONY: install
ifeq ($(SYMLINK-FILES-y),)
install: build _postinstall
else
install: _preinstall build _postinstall
build: _preinstall
endif

_postinstall: build

.PHONY: build
build: _postbuild

exe2cmd = $(strip $(call dotfile,$(patsubst %,%.cmd,$(1))))

ifeq ($(LINK_USING_CC),1)
# Override the definition of LD here, since we're linking with CC
LD := $(CC) $(CPU_CFLAGS) $(EXTRA_CFLAGS)
_CPU_LDFLAGS := $(call linkerprefix,$(CPU_LDFLAGS))
override EXTRA_LDFLAGS := $(call linkerprefix,$(EXTRA_LDFLAGS))
else
_CPU_LDFLAGS := $(CPU_LDFLAGS)
endif

O_TO_A = $(AR) crDs $(LIB) $(OBJS-y)
O_TO_A_STR = $(subst ','\'',$(O_TO_A)) #'# fix syntax highlight
O_TO_A_DISP = $(if $(V),"$(O_TO_A_STR)","  AR $(@)")
O_TO_A_CMD = "cmd_$@ = $(O_TO_A_STR)"
O_TO_A_DO = @set -e; \
	echo $(O_TO_A_DISP); \
	$(O_TO_A) && \
	echo $(O_TO_A_CMD) > $(call exe2cmd,$(@))

ifneq ($(CC_SUPPORTS_Z),false)
NO_UNDEFINED := -z defs
endif

O_TO_S = $(LD) -L$(RTE_SDK_BIN)/lib $(_CPU_LDFLAGS) $(EXTRA_LDFLAGS) \
	  -shared $(OBJS-y) $(NO_UNDEFINED) $(LDLIBS) -Wl,-soname,$(LIB) -o $(LIB)
O_TO_S_STR = $(subst ','\'',$(O_TO_S)) #'# fix syntax highlight
O_TO_S_DISP = $(if $(V),"$(O_TO_S_STR)","  LD $(@)")
O_TO_S_DO = @set -e; \
	echo $(O_TO_S_DISP); \
	$(O_TO_S) && \
	echo $(O_TO_S_CMD) > $(call exe2cmd,$(@))

-include .$(LIB).cmd

#
# Archive objects in .a file if needed
#
ifeq ($(CONFIG_RTE_BUILD_SHARED_LIB),y)
$(LIB): $(OBJS-y) $(DEP_$(LIB)) FORCE
ifeq ($(LIBABIVER),)
	@echo "Must Specify a $(LIB) ABI version"
	@false
endif
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	$(if $(D),\
		@echo -n "$< -> $@ " ; \
		echo -n "file_missing=$(call boolean,$(file_missing)) " ; \
		echo -n "cmdline_changed=$(call boolean,$(call cmdline_changed,$(O_TO_S_STR))) " ; \
		echo -n "depfile_missing=$(call boolean,$(depfile_missing)) " ; \
		echo "depfile_newer=$(call boolean,$(depfile_newer)) ")
	$(if $(or \
		$(file_missing),\
		$(call cmdline_changed,$(O_TO_S_STR)),\
		$(depfile_missing),\
		$(depfile_newer)),\
		$(O_TO_S_DO))

else
$(LIB): $(OBJS-y) $(DEP_$(LIB)) FORCE
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	$(if $(D),\
	    @echo -n "$< -> $@ " ; \
	    echo -n "file_missing=$(call boolean,$(file_missing)) " ; \
	    echo -n "cmdline_changed=$(call boolean,$(call cmdline_changed,$(O_TO_A_STR))) " ; \
	    echo -n "depfile_missing=$(call boolean,$(depfile_missing)) " ; \
	    echo "depfile_newer=$(call boolean,$(depfile_newer)) ")
	$(if $(or \
	    $(file_missing),\
	    $(call cmdline_changed,$(O_TO_A_STR)),\
	    $(depfile_missing),\
	    $(depfile_newer)),\
	    $(O_TO_A_DO))
endif

#
# install lib in $(RTE_OUTPUT)/lib
#
$(RTE_OUTPUT)/lib/$(LIB): $(LIB)
	@echo "  INSTALL-LIB $(LIB)"
	@[ -d $(RTE_OUTPUT)/lib ] || mkdir -p $(RTE_OUTPUT)/lib
	$(Q)cp -f $(LIB) $(RTE_OUTPUT)/lib
ifeq ($(CONFIG_RTE_BUILD_SHARED_LIB),y)
	$(Q)ln -s -f $< $(shell echo $@ | sed 's/\.so.*/.so/')
endif

#
# Clean all generated files
#
.PHONY: clean
clean: _postclean

.PHONY: doclean
doclean:
	$(Q)rm -rf $(LIB) $(OBJS-all) $(DEPS-all) $(DEPSTMP-all) \
	  $(CMDS-all) .$(LIB).cmd $(INSTALL-FILES-all) *.pmd.c *.pmd.o
	$(Q)rm -f $(_BUILD_TARGETS) $(_INSTALL_TARGETS) $(_CLEAN_TARGETS)

include $(RTE_SDK)/mk/internal/rte.compile-post.mk
include $(RTE_SDK)/mk/internal/rte.install-post.mk
include $(RTE_SDK)/mk/internal/rte.clean-post.mk
include $(RTE_SDK)/mk/internal/rte.build-post.mk

.PHONY: FORCE
FORCE:
