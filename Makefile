CC=clang
CFLAGS=-Wall -Wextra -std=c11
SRC=src/main.c src/ipc.c
OUT=tp1

# En Linux probablemente necesites: -pthread -lrt
# En macOS, normalmente no hace falta -lrt, y -pthread es opcional.
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
  LDFLAGS=-pthread -lrt
else
  LDFLAGS=
endif

all: build

build:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

clean:
	rm -f $(OUT)