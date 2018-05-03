###############################################################################
## Copyright (C), 2016-2018
## file     : Makefile  $(TOP_DIR)
## brief    : for all project main makefile control
## author   : wangguixing
## versoion : V0.1  create
## date     : 2018.04.14
###############################################################################

TOP_DIR := $(shell pwd)

export TOP_DIR

include ./Makefile.param

.PHONY: all clean

all:
	$(call make_subdir, ./src)


clean:
	$(call make_clean_subdir, ./src)
