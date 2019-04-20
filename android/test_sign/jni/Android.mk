
#编译时到src目录，执行以下命令即可编译,只能不带APP_BUILD_SCRIPT路径参数才可以用上default.properties 来改变平台
# ndk-build
# ndk-build APP_BUILD_SCRIPT=./jni/Android.mk NDK_PROJECT_PATH=../android NDK_DEBUG=0 V=1

#TARGET_PLATFORM 试过无效果，只有通过AndroidManifest.xml,default.properties 来控制使用平台版本
#TARGET_PLATFORM = android-12
#android 的debug 版本可以ndk-build NDK_DEBUG=1 方式执行debug 或者Application.mk中加入APP_OPTIM := debug 
#或者在AndroidManifest.xml的<application>下加入android:debuggable="true"
#
#coredump信息跟踪
# 加-fexceptions 参数并不能catch 段错误，可能仍要处理11信号,但catch()不到。
#程序dump产生coredump信息，如保存成foo.txt,可用下命令查看调用栈，编译时需要-g
#需要3段信息： pid:signal 11段；backstrace段； stack段；
#ndk-stack -sym $PROJECT_PATH/obj/local/armeabi -dump foo.txt
#

#LOCAL_PATH := $(call my-dir)
LOCAL_PATH := ../../..

include $(CLEAR_VARS)
LOCAL_MODULE :=test_sign
MY_LIBPATH_1 := $(LOCAL_PATH)/clchain/src/libclutil
MY_LIBPATH_2 := $(LOCAL_PATH)/clchain/src/secp256k1
MY_LIBPATH_3 := $(LOCAL_PATH)/clchain/src/libcrypto
MY_LIBPATH_4 := $(LOCAL_PATH)/clchain/src/test_sign


LOCAL_C_INCLUDES := $(MY_LIBPATH_1) $(MY_LIBPATH_2) $(MY_LIBPATH_3) $(MY_LIBPATH_4)
LOCAL_CFLAGS +=
LOCAL_CPPFLAGS += -D_FILE_OFFSET_BITS=64 -Wall -Werror -std=gnu++11

#
MY_CPP_LIST := $(wildcard  $(MY_LIBPATH_1)/*.cpp)
MY_CPP_LIST += $(wildcard  $(MY_LIBPATH_2)/*.c)
MY_CPP_LIST += $(wildcard  $(MY_LIBPATH_3)/*.cpp)
MY_CPP_LIST += $(wildcard  $(MY_LIBPATH_4)/*.cpp)
#
#去掉"$(LOCAL_PATH)/"路径
LOCAL_SRC_FILES := $(MY_CPP_LIST:$(LOCAL_PATH)/%=%)
#以下两种去法一样的
#LOCAL_SRC_FILES := $(subst $(LOCAL_PATH)/, , $(MY_CPP_LIST)) 
#LOCAL_SRC_FILES := $(patsubst $(LOCAL_PATH)/%, %, $(MY_CPP_LIST))  

LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog -latomic

#BUILD_EXECUTABLE / BUILD_SHARED_LIBRARY 
include $(BUILD_EXECUTABLE)


