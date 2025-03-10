CC := gcc
CFLAGS := -Wall -Wextra -O2
TARGET := test-virtio-cs695
SRC := test-virtio-cs695.c

all: $(TARGET)

$(TARGET): $(SRC)
	@$(CC) $(CFLAGS) -o $(TARGET) $(SRC) && echo "Build successful" || echo "Build failed"

clean:
	@rm -f $(TARGET) && echo "Cleanup successful" || echo "Nothing to clean"

.PHONY: all clean