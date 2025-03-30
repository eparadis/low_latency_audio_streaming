# Makefile for compiling main.c as "rate_limited_data_generator"

CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = rate_limited_data_generator
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
