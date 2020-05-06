### Build on macOS >=10.14.6 with g++

#### Install build dependencies

This assumes that your systen already has Homebrew (https://brew.sh/).

`$ brew install sdl2 sdl2_image jsoncpp`

#### Build

```
$ g++ src/**/*.cc \
    -I./include \
    -o bin/game \
    -lSDL2 \
    -lSDL2_image \
    -ljsoncpp \
    -stdlib=libc++ \
    -std=gnu++2a \
    -m64 -O3
```

#### Run

`$ bin/game`