MODULE_DIR = ../utils
#PREFIX=arm-unknown-linux-gnueabi-
include $(MODULE_DIR)/config.mk

OBJDIR := obj/
CFLAGS := -g -O1 -Wall -I$(MODULE_DIR)
CXXFLAGS := -std=c++0x

include $(MODULE_DIR)/coreutils/module.mk
include $(MODULE_DIR)/bbsutils/module.mk
include $(MODULE_DIR)/crypto/module.mk
include $(MODULE_DIR)/sqlite3/module.mk

TARGET := bbs
LOCAL_FILES += bbs.cpp loginmanager.cpp messageboard.cpp comboard.cpp bbstools.cpp
LOCAL_FILES += main.cpp 
LIBS += -lbfd
#LDFLAGS += -g
#CFLAGS += -DMY_UNIT_TEST
#TARGET := test
#LOCAL_FILES += catch.cpp

CC=ccache clang -Qunused-arguments
CXX=ccache clang++ -Qunused-arguments
#CXX = arm-unknown-linux-gnueabi-g++
#CC = arm-unknown-linux-gnueabi-gcc
#CFLAGS += -DARM -DRASPBERRYPI
#HOST := raspberrypi

include $(MODULE_DIR)/build.mk
