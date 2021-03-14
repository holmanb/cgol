Instructions
==================

Build Deps
----------
meson, ninja, gcc, openmp, bats (optional)

Build
-----
cd src/c/
meson build
ninja -C build

Run
---
cd src/c/
./cgol    # will use the default data set (to be executed using the symlink for now)
./cgol -h # view more options

Test
----
cd src/c/
./test/test.bats
