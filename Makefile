OBJDIR := obj/
CFLAGS := -g -Wall -O2
CXXFLAGS := -std=c++0x

MODULE_DIR = ../utils
CFLAGS += -I$(MODULE_DIR) -I$(MODULE_DIR)/netlink -pthread
MODULES := $(MODULE_DIR)/coreutils $(MODULE_DIR)/bbsutils $(MODULE_DIR)/netlink

TARGET := bbs
LIBS := -pthread -lz
OBJS := bbstest.o

#LINUX_CC := clang #gcc-4.7
#LINUX_CXX := clang++ #g++-4.7
#CFLAGS += -pthread -D__GCC_HAVE_SYNC_COMPARE_AND_SWAP_1 -D__GCC_HAVE_SYNC_COMPARE_AND_SWAP_2 -D__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4 -D__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8

#CC=arm-linux-androideabi-clang
#CXX=arm-linux-androideabi-clang++

LINUX_CC=ccache clang -Qunused-arguments
LINUX_CXX=ccache clang++ -Qunused-arguments

#LINUX_CFLAGS := $(LINUX_CFLAGS) `curl-config --cflags`
#LINUX_LIBS := `curl-config --libs`

all : start_rule

include $(MODULE_DIR)/Makefile.inc
