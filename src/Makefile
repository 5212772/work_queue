###############################################################################
## Copyright (C), 2016-2018
## file     : Makefile  $(TOP_DIR)/src
## brief    : for all project main makefile control
## author   : wangguixing
## versoion : V0.1  create
## date     : 2018.04.14
###############################################################################

###############################################################
# PREPARATION : Do PREPARATION before build target bin or LIB.
#               But! This will define after include $(BUILD_BIN).
#               e.g.
#                   include $(BUILD_BIN)
#
#                   $(PREPARATION):
#                   	$(call make_subdir, ./common)
#
# COPY_TO_DIR : copy target bin to destination dir
#
# TARGET_BIN  : target bin
# BUILD_BIN   : will build bin for target
# 
# TARGET_LIB        : target static lib
# BUILD_STATIC_LIB  : will build static lib for target lib
#
# ENV_CLEAR: If call TOP_MK_PARAM file, best to call ENV_CLEAR,
#            so that set CFLAG ... init.
#
# TOP_MK_PARAM : This is top dir Makefile.param file. if you
#                want to independent build local files, you
#                must be include Absolute top path Makefile.param.
#                e.g.  include ../../../Makefile.param
#
###############################################################

include $(ENV_CLEAR)

include $(TOP_MK_PARAM)

.PHONY: all clean

PREPARATION :=

TARGET_BIN = work_queue

COPY_TO_DIR = $(TOP_DIR)/release

CXXFlAGS += -I$(TOP_DIR)/src
CXXFlAGS += -I$(TOP_DIR)/include

include $(BUILD_BIN)

$(PREPARATION):
	@$(RM) $(TARGET_BIN)
	@$(RM) $(COPY_TO_DIR)/$(TARGET_BIN)
