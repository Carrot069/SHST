import platform as pf

jom_path = None
qmake_path = None

if pf.system() == "Darwin":
    jom_path = ""
elif pf.system() == "Windows":
    qmake_path = "C:\Qt\5.12.11\msvc2017_64\bin\qmake.exe"
    jom_path = "C:\Qt\Tools\QtCreator\bin\jom\jom.exe"

def clean():
    clean_cmd = "-f Makefile.Debug clean"

#C:\Qt\Tools\QtCreator\bin\jom\jom.exe -f Makefile.Debug clean

"""
15:22:25: 为项目chemi-server执行步骤 ...
15:22:25: 正在启动 "C:\Qt\5.12.11\msvc2017_64\bin\qmake.exe" E:\Users\SHST\code\capture\chemi-server\chemi-server.pro -spec win32-msvc "CONFIG+=qtquickcompiler"

15:22:28: 进程"C:\Qt\5.12.11\msvc2017_64\bin\qmake.exe"正常退出。
15:22:28: 正在启动 "C:\Qt\Tools\QtCreator\bin\jom\jom.exe" -f E:/Users/SHST/code/capture/build/chemi-server-Desktop_Qt_5_12_11_MSVC2017_64bit-Release/Makefile qmake_all


jom 1.1.3 - empower your cores

15:22:28: 进程"C:\Qt\Tools\QtCreator\bin\jom\jom.exe"正常退出。
15:22:28: 正在启动 "C:\Qt\Tools\QtCreator\bin\jom\jom.exe" 

	C:\Qt\Tools\QtCreator\bin\jom\jom.exe -f Makefile.Release
	cl -c -nologo -Zc:wchar_t -FS -Zc:rvalueCast -Zc:inline -Zc:strictStrings -Zc:throwingNew -Zc:referenceBinding -Zc:__cplusplus /utf-8 -O2 -MD -W3 -w34100 -w34189 -w44996 -w44456 -w44457 -w44458 -wd4577 -wd4467 -EHsc -DUNICODE -D_UNICODE -DWIN32 -D_ENABLE_EXTENDED_ALIGNED_STORAGE -DWIN64 -DQT_DEPRECATED_WARNINGS -DCHEMI_CAPTURE -DPVCAM -DTCP_SERVER -DKSJ_CAMERA -DAPP_VERSION=\"2.0.1.55\" -DHARDLOCK -DKSJAPI_EXPORTS -DQT_NO_DEBUG_OUTPUT -DQT_NO_DEBUG -DQT_QUICK_LIB -DQT_PRINTSUPPORT_LIB -DQT_SVG_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_QML_LIB -DQT_WEBSOCKETS_LIB -DQT_NETWORK_LIB -DQT_SQL_LIB -DQT_CORE_LIB -DNDEBUG -I..\..\chemi-server -I. -I..\..\libusb-1.0.23\include\libusb-1.0 -I..\..\chemi-server\include -I..\..\libtiff_4.1.0_x64 -I..\..\chemi-server\pvcam\Inc -I..\..\chemi-server\ksj\KSJApi.Inc -I..\..\chemi-server\ksj\common -I..\..\chemi-server\ksj\KSJApi.Lib -I..\..\chemi-server\rockey3 -IC:\Qt\5.12.11\msvc2017_64\include -IC:\Qt\5.12.11\msvc2017_64\include\QtQuick -IC:\Qt\5.12.11\msvc2017_64\include\QtPrintSupport -IC:\Qt\5.12.11\msvc2017_64\include\QtSvg -IC:\Qt\5.12.11\msvc2017_64\include\QtWidgets -IC:\Qt\5.12.11\msvc2017_64\include\QtGui -IC:\Qt\5.12.11\msvc2017_64\include\QtANGLE -IC:\Qt\5.12.11\msvc2017_64\include\QtQml -IC:\Qt\5.12.11\msvc2017_64\include\QtWebSockets -IC:\Qt\5.12.11\msvc2017_64\include\QtNetwork -IC:\Qt\5.12.11\msvc2017_64\include\QtSql -IC:\Qt\5.12.11\msvc2017_64\include\QtCore -Irelease -I/include -IC:\Qt\5.12.11\msvc2017_64\mkspecs\win32-msvc -Forelease\ @C:\Users\SHST\AppData\Local\Temp\WzPvCamera.obj.11604.78.jom
WzPvCamera.cpp
	cl -c -nologo -Zc:wchar_t -FS -Zc:rvalueCast -Zc:inline -Zc:strictStrings -Zc:throwingNew -Zc:referenceBinding -Zc:__cplusplus /utf-8 -O2 -MD -W3 -w34100 -w34189 -w44996 -w44456 -w44457 -w44458 -wd4577 -wd4467 -EHsc -DUNICODE -D_UNICODE -DWIN32 -D_ENABLE_EXTENDED_ALIGNED_STORAGE -DWIN64 -DQT_DEPRECATED_WARNINGS -DCHEMI_CAPTURE -DPVCAM -DTCP_SERVER -DKSJ_CAMERA -DAPP_VERSION=\"2.0.1.55\" -DHARDLOCK -DKSJAPI_EXPORTS -DQT_NO_DEBUG_OUTPUT -DQT_NO_DEBUG -DQT_QUICK_LIB -DQT_PRINTSUPPORT_LIB -DQT_SVG_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_QML_LIB -DQT_WEBSOCKETS_LIB -DQT_NETWORK_LIB -DQT_SQL_LIB -DQT_CORE_LIB -DNDEBUG -I..\..\chemi-server -I. -I..\..\libusb-1.0.23\include\libusb-1.0 -I..\..\chemi-server\include -I..\..\libtiff_4.1.0_x64 -I..\..\chemi-server\pvcam\Inc -I..\..\chemi-server\ksj\KSJApi.Inc -I..\..\chemi-server\ksj\common -I..\..\chemi-server\ksj\KSJApi.Lib -I..\..\chemi-server\rockey3 -IC:\Qt\5.12.11\msvc2017_64\include -IC:\Qt\5.12.11\msvc2017_64\include\QtQuick -IC:\Qt\5.12.11\msvc2017_64\include\QtPrintSupport -IC:\Qt\5.12.11\msvc2017_64\include\QtSvg -IC:\Qt\5.12.11\msvc2017_64\include\QtWidgets -IC:\Qt\5.12.11\msvc2017_64\include\QtGui -IC:\Qt\5.12.11\msvc2017_64\include\QtANGLE -IC:\Qt\5.12.11\msvc2017_64\include\QtQml -IC:\Qt\5.12.11\msvc2017_64\include\QtWebSockets -IC:\Qt\5.12.11\msvc2017_64\include\QtNetwork -IC:\Qt\5.12.11\msvc2017_64\include\QtSql -IC:\Qt\5.12.11\msvc2017_64\include\QtCore -Irelease -I/include -IC:\Qt\5.12.11\msvc2017_64\mkspecs\win32-msvc -Forelease\ @C:\Users\SHST\AppData\Local\Temp\KSJ_GS.obj.11604.125.jom
KSJ_GS.cpp
	cl -c -nologo -Zc:wchar_t -FS -Zc:strictStrings -O2 -MD -W3 -w44456 -w44457 -w44458 -DUNICODE -D_UNICODE -DWIN32 -D_ENABLE_EXTENDED_ALIGNED_STORAGE -DWIN64 -DQT_DEPRECATED_WARNINGS -DCHEMI_CAPTURE -DPVCAM -DTCP_SERVER -DKSJ_CAMERA -DAPP_VERSION=\"2.0.1.55\" -DHARDLOCK -DKSJAPI_EXPORTS -DQT_NO_DEBUG_OUTPUT -DQT_NO_DEBUG -DQT_QUICK_LIB -DQT_PRINTSUPPORT_LIB -DQT_SVG_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_QML_LIB -DQT_WEBSOCKETS_LIB -DQT_NETWORK_LIB -DQT_SQL_LIB -DQT_CORE_LIB -DNDEBUG -I..\..\chemi-server -I. -I..\..\libusb-1.0.23\include\libusb-1.0 -I..\..\chemi-server\include -I..\..\libtiff_4.1.0_x64 -I..\..\chemi-server\pvcam\Inc -I..\..\chemi-server\ksj\KSJApi.Inc -I..\..\chemi-server\ksj\common -I..\..\chemi-server\ksj\KSJApi.Lib -I..\..\chemi-server\rockey3 -IC:\Qt\5.12.11\msvc2017_64\include -IC:\Qt\5.12.11\msvc2017_64\include\QtQuick -IC:\Qt\5.12.11\msvc2017_64\include\QtPrintSupport -IC:\Qt\5.12.11\msvc2017_64\include\QtSvg -IC:\Qt\5.12.11\msvc2017_64\include\QtWidgets -IC:\Qt\5.12.11\msvc2017_64\include\QtGui -IC:\Qt\5.12.11\msvc2017_64\include\QtANGLE -IC:\Qt\5.12.11\msvc2017_64\include\QtQml -IC:\Qt\5.12.11\msvc2017_64\include\QtWebSockets -IC:\Qt\5.12.11\msvc2017_64\include\QtNetwork -IC:\Qt\5.12.11\msvc2017_64\include\QtSql -IC:\Qt\5.12.11\msvc2017_64\include\QtCore -Irelease -I/include -IC:\Qt\5.12.11\msvc2017_64\mkspecs\win32-msvc -Forelease\ @C:\Users\SHST\AppData\Local\Temp\aes.obj.11604.1312.jom
aes.c
..\..\chemi-server\aes\aes.c: warning C4819: 该文件包含不能在当前代码页(936)中表示的字符。请将该文件保存为 Unicode 格式以防止数据丢失
"""