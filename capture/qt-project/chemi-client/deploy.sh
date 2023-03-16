#!/bin/bash
rm /Users/pipiluu/Code/_project/chemi/qt-project/build-chemi-Desktop_Qt_5_12_8_clang_64bit-Release/SHSTCapture.dmg
/Users/pipiluu/Qt5.12.8/5.12.8/clang_64/bin/macdeployqt /Users/pipiluu/Code/_project/chemi/qt-project/build-chemi-Desktop_Qt_5_12_8_clang_64bit-Release/SHSTCapture.app -dmg -qmldir=/Users/pipiluu/Code/_project/chemi/qt-project/chemi

/Users/pipiluu/Qt5.12.8/5.12.8/clang_64/bin/macdeployqt /Users/pipiluu/Code/_project/chemi/qt-project/build-chemi-Desktop_Qt_5_12_8_clang_64bit-Debug/SHSTCapture.app -dmg -qmldir=/Users/pipiluu/Code/_project/chemi/qt-project/chemi
