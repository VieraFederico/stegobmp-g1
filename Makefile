CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lssl -lcrypto

SRC = src/main.c src/bmp_handler/bmp_handler.c src/lsb1/lsb1.c src/lsb4/lsb4.cs src/lsbi/lsbi.c src/utils/util.c
OBJ = $(SRC:.c=.o)

stegobmp: $(OBJ)
    $(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

clean:
    rm -f $(OBJ) stegobmp