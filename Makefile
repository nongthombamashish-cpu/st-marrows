# St. Marrow's - build with w64devkit (MinGW-w64 GCC + make) on Windows.
#
#   make           debug build   -> StMarrows.exe (console visible, debug symbols)
#   make release   optimized     -> StMarrows.exe (no console window, no DLLs needed)
#   make dist      release + dist/StMarrows-vX.Y.Z-win64.zip, ready for a GitHub Release
#   make clean

CC      := gcc
TARGET  := StMarrows.exe
SRC     := $(wildcard src/*.c)
RAYLIB  := libs/raylib

CFLAGS  := -std=c11 -Wall -Wextra -I$(RAYLIB)/include
LDFLAGS := -L$(RAYLIB)/lib
LDLIBS  := -lraylib -lopengl32 -lgdi32 -lwinmm

.PHONY: all debug release dist clean

all: debug

debug: CFLAGS += -g -O0
debug: clean $(TARGET)

release: CFLAGS  += -O2 -DNDEBUG
release: LDFLAGS += -s -static -mwindows
release: clean $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $@ $(LDFLAGS) $(LDLIBS)

dist: release
	powershell -NoProfile -ExecutionPolicy Bypass -File tools/package.ps1

clean:
	rm -f $(TARGET)
