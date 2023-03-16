#!/bin/sh
export build_path=../../build/chemi-Desktop_Qt_5_12_12_clang_64bit-Release
test -f $build_path/SHSTCapture.dmg && rm -rf $build_path/SHSTCapture.dmg
create-dmg \
  --volname "SHSTCapture" \
  --volicon "../Icon.icns" \
  --window-pos 200 120 \
  --window-size 850 550 \
  --icon-size 128 \
  --icon "SHSTCapture.app" 260 265 \
  --hide-extension "SHSTCapture.app" \
  --app-drop-link 605 265 \
  "SHSTCapture.dmg" \
  $build_path"/SHSTCapture.app"

mv SHSTCapture.dmg ../../../setup_files