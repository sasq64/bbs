MODULE_DIR = ../utils
include $(MODULE_DIR)/config.mk

OBJDIR := obj/
CFLAGS := -g -Wall -I$(MODULE_DIR)
CXXFLAGS := -std=c++0x
LIBS += -ldl

include $(MODULE_DIR)/coreutils/module.mk
include $(MODULE_DIR)/bbsutils/module.mk
include $(MODULE_DIR)/crypto/module.mk
include $(MODULE_DIR)/sqlite3/module.mk

TARGET := bbs
LOCAL_FILES += bbs.cpp

CC=ccache clang -Qunused-arguments
CXX=ccache clang++ -Qunused-arguments

include $(MODULE_DIR)/build.mk
