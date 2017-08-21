# Directories
DIR_BIN			:= bin
DIR_LIB			:= lib
DIR_OBJ			:= obj
DIR_SRC			:= src
DIR_INC			:= include
DIR_TEST		:= tst
DIR_DEP			:= dep

# Output
OUT_LIB			:= libcomp.a

# Tools
CC				:= cc
CX				:= g++
AR				:= ar

# flags
CXXFLAGS		:= -std=c++11
CFLAGS			:= -g -pthread #-DDEBUG_MESSAGES

# srcs
C_SRCS			:= $(wildcard $(DIR_SRC)/*.c)
CX_SRCS			:= $(wildcard $(DIR_SRC)/*.cpp)
SRCS			:= $(C_SRCS) $(CX_SRCS)
C_SRCS_IN_DIR	:= $(subst $(DIR_SRC),../$(DIR_SRC),$(C_SRCS))
CX_SRCS_IN_DIR	:= $(subst $(DIR_SRC),../$(DIR_SRC),$(CX_SRCS))

OBJS			:= $(subst .cpp,.o,$(SRCS))
OBJS			:= $(subst .cc,.o,$(OBJS))
OBJS			:= $(subst .c,.o,$(OBJS))

# gTest
GTEST_DIR		:= $(DIR_DEP)/googletest
FLAGS			:=
LIBS			:= $(GTEST_DIR)/build/googlemock/libgmock.a $(GTEST_DIR)/build/googlemock/gtest/libgtest.a
INCLUDE			:= -I$(GTEST_DIR)/googletest/include/ -I$(GTEST_DIR)/googlemock/include/ -Isrc/ -I$(DIR_INC)/
TEST_OUT		:= test
TEST_SRCS		:= $(wildcard $(DIR_TEST)/*.cpp)
TEST_OBJS		:= $(subst .cpp,.o,$(TEST_SRCS))

.PHONY: default

default: $(TEST_OUT)

configure:
	cd $(GTEST_DIR) && ./configure

deps:
	cd $(GTEST_DIR) && make

%.o: %.c
	$(CC) $(CFLAGS) $(FLAGS) $(INC) $< -c -o $@

%.o: %.cpp
	$(CX) $(CFLAGS) $(CXXFLAGS) $(FLAGS) $(INC) $< -c -o $@

%.o: %.cc
	$(CX) $(CFLAGS) $(CXXFLAGS) $(FLAGS) $(INC) $< -c -o $@

$(DIR_TEST)/%.o: $(DIR_TEST)/%.cpp
	$(CX) $(CFLAGS) $(CXXFLAGS) $(FLAGS) $(INCLUDE) $< -c -o $@

$(OUT_LIB): $(OBJS)
	$(AR) rcs $(DIR_LIB)/$@ $(OBJS)

$(TEST_OUT): $(OUT_LIB) $(TEST_OBJS)
	$(CX) $(CFLAGS) $(CXXFLAGS) $(FLAGS) $(TEST_OBJS) $(DIR_LIB)/$(OUT_LIB) $(LIBS) -o $(DIR_BIN)/$@

all: deps $(TEST_OUT)

cleanAll:
	cd $(GTEST_DIR) && make clean
	rm -rf $(DIR_BIN)/* $(DIR_LIB)/*

clean:
	rm -rf $(DIR_BIN)/* $(DIR_LIB)/*
	find $(DIR_SRC)/ -name "*.o" -print0 | xargs -0 rm -f
	find $(DIR_TEST)/ -name "*.o" -print0 | xargs -0 rm -f
