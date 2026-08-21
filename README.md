# csc

This project was inspired by [nob.h](https://github.com/tsoding/nob.h). I always want to do some simple things don't by scripting language.

I won't spend too much time in this, just expect it help me no need to suffer from dynamic typing.

## Implement

Just call shell Command and other exec,may be add more feature.

## How To Use

include file and write execute command.(wait for change)

> clang++ ./src/common/argp/argp.cpp ./src/common/cmd/cmd.cpp ./src/common/csc/csc.cpp ./src/common/csc/target.cpp ./src/common/csc/tool.cpp ./src/common/csc/tool_chain.cpp ./src/common/log/log.cpp ./src/common/rsc/rsc.cpp ./src/windows/cmd/cmd.cpp ./src/windows/csc/csc.cpp ./src/windows/csc/tool_chain.cpp ./src/windows/rsc/os.cpp ./compile.cpp -I./include -o compile.exe -std=c++26 -luuid -lole32
