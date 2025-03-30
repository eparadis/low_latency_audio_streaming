# Makefile for compiling main.c as "console_loopback_timer"

CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = console_loopback_timer
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)