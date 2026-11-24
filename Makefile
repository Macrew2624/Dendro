CC     = gcc
CFLAGS = -Wall -Wextra -Werror -g
TARGET = dendro
SRC    = src/main.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $@

debug: CFLAGS += -fsanitize=adress,undefined
debug: clean $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: debug clean