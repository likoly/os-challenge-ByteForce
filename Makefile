CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lssl -lcrypto -lpthread
TARGET = x86_64/bin/linux/server
SRC = code/server.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) -O3 $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)
