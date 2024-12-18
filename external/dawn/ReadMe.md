
DAWN_URL="https://dawn.googlesource.com/dawn"

DAWN_BRANCH="chromium/6370"

$env:all_proxy="socks5://127.0.0.1:7890"

cmake -S . -B out -DDAWN_FETCH_DEPENDENCIES=ON -DDAWN_ENABLE_INSTALL=ON

cmake --build out --config Release

cmake --install out/Release --prefix install/Release

copy 
out/gen/include 
to
install/Release/include