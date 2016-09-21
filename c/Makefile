# vim: noet:

.SUFFIXES: .c

C 			= gcc
CFLAGS 		= -Wall -fpic -g -c
SOURCES		= $(wildcard ./src/*.c)
OBJS 		= $(wildcard ./src/*.o)
BUILD_PATH 	= "bin"

.c.o:
	@$(C) $(CFLAGS) $< -o $@

all:$(SOURCES:%.c=%.o) pre
	@$(C) $(OBJS) -o $(BUILD_PATH)/main

pre:
	@mkdir -p $(BUILD_PATH)
