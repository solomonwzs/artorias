# vim: noet:

.SUFFIXES: .c
.PHONY: deps

C 			= gcc

LUA53_INC 	= ./deps/lua53/include
LUA53_LIB 	= -L./deps/lua53/lib -llua -Wl,-R./deps/lua53/lib

LUA51_INC 	= ./deps/luajit/include
LUA51_LIB 	= -L./deps/luajit/lib -lluajit-5.1 -Wl,-R./deps/luajit/lib

LUA_INC 	= $(LUA53_INC)
LUA_LIB 	= $(LUA53_LIB)
LIB 		= $(LUA_LIB) -lm -ldl
CFLAGS 		= -Wall -fpic -g -c -I$(LUA_INC)
LMCFLAGS 	= -g -fPIC -shared -Wl,-z,defs $(LUA_LIB)
MAIN_PATH 	= "src"
BUILD_PATH 	= "bin"
VALGRIND 	= valgrind

LUA_ADA_OBJ = $(MAIN_PATH)/lua_adapter51.o $(MAIN_PATH)/lua_adapter53.o

ALL_SRC 	= $(wildcard src/*.c)
LM_SRC 		= $(wildcard src/lm_*.c)
MAIN_SRC 	= $(filter-out $(LM_SRC), $(ALL_SRC))

.c.o:
	@echo -e "\033[0;33m*\033[0m $@"
	@$(C) $(CFLAGS) $< -o $@

all:obj pre
	@$(C) $(MAIN_PATH)/*.o $(LIB) -o $(BUILD_PATH)/main
	@echo -e "\033[0;32m[OK]\033[0m main ok"

obj:$(ALL_SRC:%.c=%.o)

lm:lm_base lm_counter lm_socket
	@echo -e "\033[0;32m[OK]\033[0m lua modules ok"

lm_base:obj
	@$(C) $(LMCFLAGS) $(MAIN_PATH)/$@.o \
		$(MAIN_PATH)/lua_bind.o \
		$(MAIN_PATH)/lua_utils.o \
		$(MAIN_PATH)/mem_pool.o \
		$(LUA_ADA_OBJ) -o $(BUILD_PATH)/$@.so

lm_counter:obj
	@$(C) $(LMCFLAGS) $(MAIN_PATH)/$@.o \
		$(MAIN_PATH)/mem_pool.o \
		$(LUA_ADA_OBJ) -o $(BUILD_PATH)/$@.so

lm_socket:obj
	@$(C) $(LMCFLAGS) $(MAIN_PATH)/$@.o \
		$(MAIN_PATH)/lua_bind.o \
		$(MAIN_PATH)/lua_utils.o \
		$(MAIN_PATH)/mem_pool.o \
		$(MAIN_PATH)/thread.o \
		$(MAIN_PATH)/server.o \
		$(MAIN_PATH)/rb_tree.o \
		$(LUA_ADA_OBJ) -o $(BUILD_PATH)/$@.so

pre:
	@mkdir -p $(BUILD_PATH)

clean:
	@rm $(MAIN_PATH)/*.o
	@rm $(BUILD_PATH)/*

run: all lm
	@$(BUILD_PATH)/main "conf/example.lua"

mem_check: all lm
	$(VALGRIND) \
		--tool=memcheck \
		--leak-check=yes \
		--show-reachable=yes \
		--num-callers=20 \
		--track-fds=yes \
		$(BUILD_PATH)/main

deps:
	@bash install_deps.sh
