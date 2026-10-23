# rsc

mostly use for run cpp file, also manager lib or package

## directorys

default should install in

- linux: /home/user_name/rsc

- windows: C:/Users/user_name/rsc

below use /rsc as install dir express

1. `/rsc/bin` should exposed in environment, include executable file symlink

2. `/rsc/lib` should exposed in environment, include shared/dynamic lib file symlink

3. `/rsc/package` package location

4. `/rsc/script` when use rsc run cpp file, default cache executable file in this dir

## package

`/rsc/package_name/version` package content dir

`/rsc/package_name/version/bin` package executable file dir

`/rsc/package_name/version/lib` package shared/dynamic lib file dir

### cpp lib

`/rsc/package_name/version/include` package includes file dir

`/rsc/package_name/version/import` package extend useable file dir
