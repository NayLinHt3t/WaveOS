
CC = gcc

CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -g

TARGET = waveos

SRC = src/main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
