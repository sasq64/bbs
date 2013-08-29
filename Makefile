OBJDIR := obj/
CFLAGS := -g -Wall
CXXFLAGS := -std=c++0x

MODULE_DIR = ../utils
CFLAGS += -I$(MODULE_DIR) -I$(MODULE_DIR)/netlink
MODULES := $(MODULE_DIR)/coreutils $(MODULE_DIR)/bbsutils $(MODULE_DIR)/webutils $(MODULE_DIR)/netlink

TARGET := utest
LIBS := -lz
OBJS := bbstest.o

LINUX_CC := gcc-4.7
LINUX_CXX := g++-4.7

LINUX_CFLAGS := $(LINUX_CFLAGS) `curl-config --cflags`
LINUX_LIBS := `curl-config --libs`

all : start_rule

run :
	./utest

include $(MODULE_DIR)/Makefile.inc
