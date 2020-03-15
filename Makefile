TARGET = hue
SRC = src/main.c
LINK = lib/cJSON/cJSON.c

$(TARGET): $(SRC)
	$(CC) $(SRC) $(LINK) -o $(TARGET) -Wall -Werror -pedantic -std=c99 -O3 -lcurl
