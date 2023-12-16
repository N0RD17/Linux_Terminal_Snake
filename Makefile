# Compiler
CC = gcc

# Compiler Flags
CFLAGS = -Wall -Wextra -pedantic -g

# Directories
OBJ_DIR = obj

#Shell command
$(shell mkdir -p ${OBJ_DIR})

# Source files
SRCS = $(wildcard ./*.c)

# Objects
OBJS = $(patsubst ./%.c, ${OBJ_DIR}/%.o, ${SRCS})

# Compiler output
COUT = Terminal_Snake

all: ${COUT}

${COUT}: ${OBJS}
	${CC} ${CFLAGS} -o $@ $^ ${CLIBS}

${OBJ_DIR}/%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $<


test: ${COUT}
	./${COUT}

clean:
	rm -rf ${COUT} ${OBJ_DIR}

.PHONY: test clean
