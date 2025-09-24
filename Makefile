CC=clang
CFLAGS=-Wall -Wextra -std=c11
LDFLAGS=
# En Linux probablemente necesites: LDFLAGS=-pthread -lrt

SRC=src/main.c src/ipc.c
OUT=tp1

all: build

build:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

clean:
	rm -f $(OUT)