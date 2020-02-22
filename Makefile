TARGET = coach
SRC = $(wildcard *.c)
OBJ = ${SRC:.c=.o}

CC = tcc
CFLAGS = -c -O3 -Wall
LFLAGS = -lncursesw -lpthread

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LFLAGS)

%.o: %.c
	$(CC) $< -o $@ $(CFLAGS)

clean:
	rm $(TARGET) $(OBJ)
