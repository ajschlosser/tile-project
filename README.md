### Build on macOS with g++

#### Install build dependencies

`$ pip3 install conan`

`$ mkdir -p build`

`$ cd build`

`$ conan install ..`

`$ brew install sdl2 sdl2_image wxmac`

#### Build

```
$ g++ src/**/*.cc                     \
    -I ./include                      \
    -o bin/game                       \
    -lSDL2                            \
    -lSDL2_image                      \
    $(cat build/conanbuildinfo.args)  \
    $(wx-config --cxxflags)           \
    $(wx-config --libs)               \
    -std=gnu++2a
```

#### Run

`$ bin/game`