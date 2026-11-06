CC = gcc
CFLAGS = -Wall -O2 -Iinclude
LDFLAGS = -lcurl

SRCS = src/main.c src/http.c src/parser.c src/cJSON.c
TARGET = fukfukgo

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe
