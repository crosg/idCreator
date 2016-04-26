.SUFFIXES: .c .o

CC = gcc
CFLAG = -pipe -o1  -W -Wall -Werror -g \
 		-Wpointer-arith -Wno-unused-parameter -Wunused-function -Wunused-variable -Wunused-value 
INC_PATH = -I/usr/local/include \
			-I../../include
LIB_PATH = 
DEFS := 
TARGET_PATH = ./
TARGET = spx_map_test
DIRS = ../../

SYS := $(strip $(shell uname -s | tr '[:upper:]' '[:lower:]'))
ifeq ($(SYS), linux)
	DEFS += -DSpxLinux
else
	ifeq ($(SYS),unix)
		DEFS += -DSpxUnix
	else
		ifeq ($(SYS),darwin)
			DEFS += -DSpxMac
		endif
	endif
endif

BITS := $(findstring 64,$(shell uname -m ))
ifeq (64, $(BITS))
	DEFS += -DSpx64
else
	ifeq (32, $(BITS))
		DEFS += -DSpx32
	else
		DEFS += -DSpx32
	endif
endif

SRC_FILE = $(wildcard *.c) \
			$(wildcard ../../*.c)

SHARED_OBJS = $(patsubst %.c,%.o,$(SRC_FILE) )

all:$(TARGET)

%.o: %.c
	$(CC) $(CFLAG) -c $< -o $@ $(LIB_PATH) $(INC_PATH) $(DEFS)

$(TARGET) : $(SHARED_OBJS)
	$(CC) $(CFLAG) $(SHARED_OBJS) $(LIB_PATH) $(INC_PATH) -o $(TARGET) $(DEFS)

install:
	cp -f $(TARGET) $(TARGET_PATH)
clean:
	rm -f $(SHARED_OBJS) $(TARGET)
