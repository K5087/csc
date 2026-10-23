# csc

This project was inspired by [nob.h](https://github.com/tsoding/nob.h). I always
want to do some simple things don't by scripting language.

I won't spend too much time in this,
just expect it help me no need to suffer from dynamic typing.

## Dependency

llvm-clang tool-chain

## Build Project

<details open>
<summary>Windows</summary>

1. gen boot

   > clang++ src/common/argp/argp.cpp src/common/cmd/cmd.cpp src/common/csc/csc.cpp src/common/csc/depend.cpp src/common/csc/graph.cpp src/common/csc/target.cpp src/common/csc/tool.cpp src/common/csc/tool_chain.cpp src/common/csc/unit.cpp src/common/json/debug.cpp src/common/json/decode.cpp src/common/json/encode.cpp src/common/json/parse.cpp src/common/json/type.cpp src/common/log/log.cpp src/common/rsc/dev.cpp src/common/rsc/rsc.cpp src/windows/cmd/cmd.cpp src/windows/csc/target.cpp src/windows/csc/tool_chain.cpp src/windows/rsc/os.cpp build.cpp -Iinclude -o boot.exe -std=c++26 -luuid -lole32

2. boot

   > ./boot.exe

3. update

   > ./build/build.exe

</details>

<details>
<summary>Linux</summary>

1. gen boot
   > clang++ src/common/argp/argp.cpp src/common/cmd/cmd.cpp src/common/csc/csc.cpp src/common/csc/depend.cpp src/common/csc/graph.cpp src/common/csc/target.cpp src/common/csc/tool.cpp src/common/csc/tool_chain.cpp src/common/csc/unit.cpp src/common/json/debug.cpp src/common/json/decode.cpp src/common/json/encode.cpp src/common/json/parse.cpp src/common/json/type.cpp src/common/log/log.cpp src/common/rsc/dev.cpp src/common/rsc/rsc.cpp src/unix/cmd/cmd.cpp src/unix/csc/target.cpp src/unix/csc/tool_chain.cpp src/unix/rsc/os.cpp build.cpp -Iinclude -o boot -std=c++26 -luuid -lole32

2. boot

   > ./boot

3. update

   > ./build/build

</details>

## prepare

1. use build program install

   > ./build/build.exe -i

> [!Tip]
   > default install location
   > - Unix: /home/user_name/rsc
   > - Windows: C:/Users/user_name/rsc

2. add install_dir/bin install_dir/lib to environment

## How To Use

```cpp
// main.cpp
#include<iostream>
int main(int argc,char** argv){
    std::cout<<"Hello,World\n";
    return 0;
}

// build.cpp
#include <csc/csc.hpp>

using namespace csc;

fs::path current = CURRENT_DIR;
fs::path build_dir = current / "build";

int main(int argc, char* argv[]) {
    update_self(argc, argv, {__FILE__});

    auto target = make::exec_target("main");
    target->build = build_dir;
    target->AddSource("main.cpp");
    target->tool_chain = std::make_shared<LLVM>();
    if (!build_target(target)) { return -1; }
    return 0;
}
```

> csc build.cpp -l
>
> ./build.exe
>
> ./build/main.exe

## TODO

use rsc as package manger (how to import package)

improve compile_commands.json generate
