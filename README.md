# csc

This project was inspired by [nob.h](https://github.com/tsoding/nob.h). I always want to do some simple things don't by scripting language.

I won't spend too much time in this, just expect it help me no need to suffer from dynamic typing.

## Implement

Just call shell Command and other exec,may be add more feature.

## How To Use

include file and write build command.

```cpp
#include "csc.hpp"

int main(int argc, char** argv) {
    using namespace csc;
    Cmd cmd;
    try {
        cmd.Append("clang++", "-o", "main", "main.cpp");
        csc::run_cmd(cmd);

    } catch (const std::exception& e) {
        csc_log(CSC_ERRO, e.what());
        return 1;
    }
}
```
