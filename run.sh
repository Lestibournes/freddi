meson setup --wipe build
cd build
ninja
cd src
./freddi $1
