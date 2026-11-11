#!/bin/bash
set -e
cd /root/g1t
I=/root/ext/imgui; M=/root/ext/minhook
mkdir -p obj out
CXX="i686-w64-mingw32-g++-posix"; CC="i686-w64-mingw32-gcc-posix"
FL="-O2 -DNDEBUG -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS -I$I -I$I/backends -I$M/include -Isrc"
objs=()
for f in $I/imgui.cpp $I/imgui_draw.cpp $I/imgui_tables.cpp $I/imgui_widgets.cpp $I/backends/imgui_impl_dx11.cpp $I/backends/imgui_impl_win32.cpp; do
  o=obj/$(basename $f .cpp).o; [ -f $o ] || $CXX $FL -std=c++17 -c $f -o $o; objs+=($o); done
for f in $M/src/buffer.c $M/src/hook.c $M/src/trampoline.c $M/src/hde/hde32.c; do
  o=obj/mh_$(basename $f .c).o; [ -f $o ] || $CC -O2 -I$M/include -c $f -o $o; objs+=($o); done
for f in src/*.cpp src/core/*.cpp src/mods/*.cpp; do
  o=obj/$(echo $f | tr / _ | sed "s/.cpp$//").o; $CXX $FL -std=c++17 -Wall -Wno-unused-function -Wno-sign-compare -c $f -o $o; objs+=($o); done
$CXX -shared -static -static-libgcc -static-libstdc++ -o out/G1CrazyTrainer.dll "${objs[@]}" \
  -ld3d11 -ldxgi -ld3dcompiler -ldwmapi -lgdi32 -luser32 -limm32 -lshell32 -Wl,--enable-stdcall-fixup -s
ls -la out/
