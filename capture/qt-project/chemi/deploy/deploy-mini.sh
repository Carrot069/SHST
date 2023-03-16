#!/bin/bash
export PATH=$PATH:~/Qt/5.12.11/clang_64/bin/
app_path=./../../build/chemi-Desktop_Qt_5_12_11_clang_64bit-Release/SHSTCaptureMini.app

macdeployqt $app_path -qmldir=./../

cp ./../../../font/SourceHanSansSC-Medium.otf $app_path/Contents/MacOS/
cp ./../../../font/digital-dismay.regular.otf $app_path/Contents/MacOS/
cp /usr/local/lib/libusb-1.0.0.dylib $app_path/Contents/Frameworks/

install_name_tool -change /src/staging/libusb/macos64/lib/libusb-1.0.0.dylib @executable_path/../Frameworks/libusb-1.0.0.dylib $app_path/Contents/MacOS/SHSTCaptureMini
otool -L $app_path/Contents/MacOS/SHSTCaptureMini