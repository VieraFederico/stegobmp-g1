CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lssl -lcrypto

SRC = src/main.c src/bmp_handler/bmp_handler.c src/lsb1/lsb1.c
OBJ = $(SRC:.c=.o)
TARGET = stegobmp

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm -f $(OBJ) $(TARGET)