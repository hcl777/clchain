
#����ʱ��srcĿ¼��ִ����������ɱ���,ֻ�ܲ���APP_BUILD_SCRIPT·�������ſ�������default.properties ���ı�ƽ̨
# ndk-build
# ndk-build APP_BUILD_SCRIPT=./jni/Android.mk NDK_PROJECT_PATH=../android NDK_DEBUG=0 V=1

#TARGET_PLATFORM �Թ���Ч����ֻ��ͨ��AndroidManifest.xml,default.properties ������ʹ��ƽ̨�汾
#TARGET_PLATFORM = android-12
#android ��debug �汾����ndk-build NDK_DEBUG=1 ��ʽִ��debug ����Application.mk�м���APP_OPTIM := debug 
#������AndroidManifest.xml��<application>�¼���android:debuggable="true"
#
#coredump��Ϣ����
# ��-fexceptions ����������catch �δ��󣬿�����Ҫ����11�ź�,��catch()������
#����dump����coredump��Ϣ���籣���foo.txt,����������鿴����ջ������ʱ��Ҫ-g
#��Ҫ3����Ϣ�� pid:signal 11�Σ�backstrace�Σ� stack�Σ�
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
#ȥ��"$(LOCAL_PATH)/"·��
LOCAL_SRC_FILES := $(MY_CPP_LIST:$(LOCAL_PATH)/%=%)
#��������ȥ��һ����
#LOCAL_SRC_FILES := $(subst $(LOCAL_PATH)/, , $(MY_CPP_LIST)) 
#LOCAL_SRC_FILES := $(patsubst $(LOCAL_PATH)/%, %, $(MY_CPP_LIST))  

LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog -latomic

#BUILD_EXECUTABLE / BUILD_SHARED_LIBRARY 
include $(BUILD_EXECUTABLE)


