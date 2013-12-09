MODULE_DIR = ../utils
include $(MODULE_DIR)/config.mk

OBJDIR := obj/
CFLAGS := -g -Wall -O2 -I$(MODULE_DIR)
CXXFLAGS := -std=c++0x

include $(MODULE_DIR)/coreutils/module.mk
include $(MODULE_DIR)/bbsutils/module.mk

TARGET := bbs
LOCAL_FILES += bbstest.cpp

CC=ccache clang -Qunused-arguments
CXX=ccache clang++ -Qunused-arguments

include $(MODULE_DIR)/build.mk
