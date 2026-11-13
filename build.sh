#!/bin/bash
set -e
cd "$(dirname "$0")"
I=${IMGUI_DIR:-/root/ext/imgui}; M=${MINHOOK_DIR:-/root/ext/minhook}
# One DLL per game: GAME picks the game module (src/games/<GAME>), RENDERER the renderer module.
GAME=${GAME:-gothic1}; RENDERER=${RENDERER:-gd3d11}
G=src/games/$GAME; R=src/renderers/$RENDERER
[ -d $G ] || { echo "unknown game module: $G"; exit 1; }
[ -d $R ] || { echo "unknown renderer module: $R"; exit 1; }
mkdir -p obj out
CXX="i686-w64-mingw32-g++-posix"; CC="i686-w64-mingw32-gcc-posix"
FL="-O2 -DNDEBUG -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS -I$I -I$I/backends -I$M/include -Isrc -Isrc/shared -I$R -I$G"
objs=()
for f in $I/imgui.cpp $I/imgui_draw.cpp $I/imgui_tables.cpp $I/imgui_widgets.cpp $I/backends/imgui_impl_dx11.cpp $I/backends/imgui_impl_win32.cpp; do
  o=obj/$(basename $f .cpp).o; [ -f $o ] || $CXX $FL -std=c++17 -c $f -o $o; objs+=($o); done
for f in $M/src/buffer.c $M/src/hook.c $M/src/trampoline.c $M/src/hde/hde32.c; do
  o=obj/mh_$(basename $f .c).o; [ -f $o ] || $CC -O2 -I$M/include -c $f -o $o; objs+=($o); done
for f in src/*.cpp src/shared/*.cpp $(find $R $G -name "*.cpp" | sort); do
  o=obj/$(echo $f | tr / _ | sed "s/.cpp$//").o; $CXX $FL -std=c++17 -Wall -Wno-unused-function -Wno-sign-compare -c $f -o $o; objs+=($o); done
$CXX -shared -static -static-libgcc -static-libstdc++ -o out/G1CrazyTrainer.dll "${objs[@]}" \
  -ld3d11 -ldxgi -ld3dcompiler -ldwmapi -lgdi32 -luser32 -limm32 -lshell32 -lole32 -lcomdlg32 -lwindowscodecs -luuid -Wl,--enable-stdcall-fixup -s
ls -la out/
