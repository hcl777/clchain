

#ABIȫ���ǣ�Application binary interface������Ӧ�ó�������ƽӿ�
#������ABI���Ͱ�����armeabi��armeabi-v7a��x86��x86_64��mips��mips64��arm64-v8a��
#APP_ABI := armeabi arm64-v8a x86
#APP_ABI := arm64-v8a
APP_ABI := armeabi

#����debug�汾: �ڴ��޸Ĳ����㣬����ֱ��ndk-build NDK_DEBUG=1
#APP_OPTIM := debug

#֧��std��׼��c++11 ,NDK_TOOLCHAIN_VERSION=4.8���ϲ�֧��
APP_STL := gnustl_static

