#Makefile template for shared library
# Adopted from : https://gist.github.com/xuhdev/1873316

CC = cc # C compiler
CFLAGS = -fPIC -Wall -Wextra -O0 -g 
LDFLAGS = -shared 
RM = rm -f 
TARGET_LIB = libmaddb.so 

SRCS = util.c
OBJS = $(SRCS:.c=.o)

.PHONY: all
	all: ${TARGET_LIB}

$(TARGET_LIB): $(OBJS)
		$(CC) ${LDFLAGS} -o $@ $^

$(SRCS:.c=.d):%.d:%.c
		$(CC) $(CFLAGS) -MM $< >$@

include $(SRCS:.c=.d)

.PHONY: clean
clean:
	-${RM} ${TARGET_LIB} ${OBJS} $(SRCS:.c=.d)
