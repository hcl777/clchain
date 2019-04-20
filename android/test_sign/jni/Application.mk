

#ABI全称是：Application binary interface，即：应用程序二进制接口
#常见的ABI类型包括：armeabi，armeabi-v7a，x86，x86_64，mips，mips64，arm64-v8a等
#APP_ABI := armeabi arm64-v8a x86
#APP_ABI := arm64-v8a
APP_ABI := armeabi

#编译debug版本: 在此修改不方便，可以直接ndk-build NDK_DEBUG=1
#APP_OPTIM := debug

#支持std标准库c++11 ,NDK_TOOLCHAIN_VERSION=4.8以上才支持
APP_STL := gnustl_static

