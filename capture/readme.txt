目录说明：

根目录
    inno-script-server —— 无线方案服务端的打包脚本
    inno-script-client —— 无线方案客户端的打包脚本 delphi
    inno-script-gel —— 凝胶采集软件的打包脚本
    inno-script —— 化学发光采集软件的打包脚本
    font —— 第三方字体文件
    qt-project —— Qt项目文件
        build —— Qt项目的构建目录，其中的子目录对应具体的项目和平台（比如32为和64位）
        chemi —— 化学发光的项目文件，这是目前正在出货的 化学发光/荧光多功能 用的版本（此文件下的子文件夹和其他项目文件夹下的结构相似，所以只介绍此目录下的子目录）
            deploy —— 脚手架脚本，方便在mac和windows部署文件
            i18n —— 多语言文件，详细解释可参考分析软件的
            icon —— 图标文件
            images —— 程序内使用的按钮图标之类的图片
            include —— C++ 头文件
            lib —— C++ 库文件
            libusb-1.0 —— libusb 库文件
            qml —— 所有QML文件的根目录
            source —— C++ 源代码文件
            third-party —— 第三方库
                aes —— 用于解密相机序列号文件的第三方 AES 库文件
                atik —— a相机SDK的头文件和源文件
                exiv2 —— 处理图片的第三方库，主要用来给tiff文件增加一些tag
                ksj —— 凝胶用的相机的sdk
                pvcam —— Q相机的sdk
                rockey3 —— 加密锁的sdk
                spdlog —— Log组件，现在弃用了
        chemi-client —— 无线方案的客户端
        chemi-server —— 无线方案的服务端
        chemi-server-daemon —— 无线方案服务端的守护进程，在比较小的概率下服务端会崩溃，此进程负责重启运行服务进程
        crash —— 多项目共用的脚手架程序，负责收集崩溃时的信息
        gel —— 凝胶采集软件，早期版本是从chemi中分离出来的，目前分化已较大
        demo-data —— 测试用的一些图片
        libtiff** —— 第三方库，用来操作Tiff文件
        libusb** —— 第三方库，用来进行usb通信
        pseudo-color-make —— 制作伪彩调色板的脚手架程序

    setup_files —— 需要手动建立，所有打包脚本产生的安装包都会存放在此目录，也有可能需要新建子目录，这个可以在运行打包脚本时看到错误后再建立
