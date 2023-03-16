#!/bin/sh
build_path=./../../build/chemi-Desktop_Qt_5_12_11_clang_64bit-Release

test -f $build_path/SHSTCaptureMini.dmg && rm $build_path/SHSTCaptureMini.dmg
create-dmg \
  --volname "SHSTCaptureMini" \
  --volicon "./../Icon.icns" \
    --window-pos 200 120 \
  --window-size 850 550 \
  --icon-size 128 \
  --icon "SHSTCaptureMini.app" 260 265 \
  --hide-extension "SHSTCaptureMini.app" \
  --app-drop-link 605 265 \
  $build_path/"SHSTCaptureMini.dmg" \
  $build_path/"/SHSTCaptureMini.app"
