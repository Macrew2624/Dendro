CC     = gcc
CFLAGS = -Wall -Wextra -Werror -g
TARGET = dendro
SRC    = src/main.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $@

debug: CFLAGS += -fsanitize=address,undefined
debug: clean $(TARGET)

clean:
	rm -f $(TARGET)

test: $(TARGET)
	sh tests/run.sh

.PHONY: debug clean