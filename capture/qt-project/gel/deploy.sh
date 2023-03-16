#!/bin/bash
rm /Users/pipiluu/Code/_project/chemi/qt-project/build-gel-Desktop_Qt_5_12_8_clang_64bit-Release/SHSTCaptureGel.dmg
rm /Users/pipiluu/Code/_project/chemi/qt-project/build-gel-Desktop_Qt_5_12_8_clang_64bit-Debug/SHSTCaptureGel.dmg
/Users/pipiluu/Qt5.12.8/5.12.8/clang_64/bin/macdeployqt /Users/pipiluu/Code/_project/chemi/qt-project/build-gel-Desktop_Qt_5_12_8_clang_64bit-Release/SHSTCaptureGel.app -dmg -qmldir=/Users/pipiluu/Code/_project/chemi/qt-project/gel
/Users/pipiluu/Qt5.12.8/5.12.8/clang_64/bin/macdeployqt /Users/pipiluu/Code/_project/chemi/qt-project/build-gel-Desktop_Qt_5_12_8_clang_64bit-Debug/SHSTCaptureGel.app -dmg -qmldir=/Users/pipiluu/Code/_project/chemi/qt-project/gel
