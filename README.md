### Build on macOS >=10.14.6 with g++

#### Install build dependencies

This assumes that your systen already has Homebrew (https://brew.sh/) and python >= 3.0. 

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
    -std=gnu++2a
```

#### Run

`$ bin/game`