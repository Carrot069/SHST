#!/bin/bash
cp /usr/local/lib/libusb-1.0.0.dylib /Users/pipiluu/shenhua/code/capture/qt-project/build/chemi-Desktop_Qt_5_12_12_clang_64bit-Release/SHSTCapture.app/Contents/Frameworks/

otool -L /Users/pipiluu/shenhua/code/capture/qt-project/build/chemi-Desktop_Qt_5_12_12_clang_64bit-Release/SHSTCapture.app/Contents/MacOS/SHSTCapture
install_name_tool -change /src/staging/libusb/macos64/lib/libusb-1.0.0.dylib @executable_path/../Frameworks/libusb-1.0.0.dylib /Users/pipiluu/shenhua/code/capture/qt-project/build/chemi-Desktop_Qt_5_12_12_clang_64bit-Release/SHSTCapture.app/Contents/MacOS/SHSTCapture