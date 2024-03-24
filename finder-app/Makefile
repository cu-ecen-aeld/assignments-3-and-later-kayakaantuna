# Default compiler
CC=gcc
# Compiler flags
CFLAGS=-Wall

# Object files
OBJ=writer.o

# Check if CROSS_COMPILE is set
ifdef CROSS_COMPILE
	CC=$(CROSS_COMPILE)gcc
endif

# Default target
all: writer

writer: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

# Pattern rule for object files
%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

# Clean target
clean:
	rm -f $(OBJ) writer

.PHONY: all clean
