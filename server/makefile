CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?=-Wall -Werror -g
LDFLAGS ?= -pthread -lrt
TARGET = aesdsocket
SRC = aesdsocket.c

all: default

default: $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
