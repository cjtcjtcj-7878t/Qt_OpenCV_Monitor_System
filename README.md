# Qt_OpenCV_Monitor_System
基于Qt6.11.1+C++17+OpenCV4.10+MySQL+家庭版Windows11开发的智能监控管理系统，支持摄像头管理、视频录制、移动侦测、用户登录等功能。
（其实没有多智能...学学写的）
# 此程序已经在Windows生成exe可执行程序支持双击即运行,exe程序位于bin文件夹名为MonitorSystem.exe
src文件夹中存放程序源码，其中third_library放置程序涉及到的第三方库OpenCV4.10以及MYSQL配置文件，你可根据其中的txt文档进行给Qt6 creator进行链接以上第三方库操作。

若想要将代码进行本地运行除了完成Qt6 creator的链接第三方库操作，还需要完成以下操作。

bin文件夹存放了exe程序以及程序需要的扩展文件。
其中sql脚本还需要你在本地的Navicate Premium中执行一次。初此之外，数据库INI配置文件也需要你手动更改相关参数。（databaseName不需要更改）。以上操作完成才可正常使用“登录”功能。

将INI文件中相关参数改成你本地的Navicate Premium相关参数。
（未完待续....）
