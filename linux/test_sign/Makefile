#makefile:

#################################
CC := gcc
CPP := g++ -std=c++11
AR := ar

# release版本不要加 -g  
CFLAGS	=  -Wall -g -DSECP256K1_BUILD
CPPFLAGS = -Werror -D_FILE_OFFSET_BITS=64
LIBS    := -lpthread -lrt -ldl 
LIBS	+= -lsecp256k1 -Wl,-rpath /usr/local/lib

LOCAL_PATH = ../../..
MY_SPATH_1 := $(LOCAL_PATH)/clchain/src/libclutil
MY_SPATH_2 := $(LOCAL_PATH)/clchain/src/secp256k1_1
MY_SPATH_3 := $(LOCAL_PATH)/clchain/src/libcrypto
MY_SPATH_4 := $(LOCAL_PATH)/clchain/src/test_sign

#
MY_SRC_LIST := $(wildcard  $(MY_SPATH_1)/*.cpp  $(MY_SPATH_1)/*.c)
MY_SRC_LIST += $(wildcard  $(MY_SPATH_2)/*.cpp  $(MY_SPATH_2)/*.c)
MY_SRC_LIST += $(wildcard  $(MY_SPATH_3)/*.cpp  $(MY_SPATH_3)/*.c)
MY_SRC_LIST += $(wildcard  $(MY_SPATH_4)/*.cpp  $(MY_SPATH_4)/*.c)

#去掉路径
LOCAL_SRC_FILES := $(notdir $(MY_SRC_LIST))

outpath := ./bin/
output := $(outpath)test_sign
sources :=  $(LOCAL_SRC_FILES)
objects := $(subst .c,.o,$(subst .cpp,.o,$(sources)))
depends := $(subst .c,.d,$(subst .cpp,.d,$(sources)))
archives :=

INCS := -I$(MY_SPATH_1) -I$(MY_SPATH_2) -I$(MY_SPATH_3) -I$(MY_SPATH_4)

INCS		+= -I../../../secp256k1-master/include
archives	+=   

####寻找目标文件和依赖文件的路径
VPATH := $(subst -I, , $(INCS)) 

################################
all: $(output)

%.o:%.c
	$(CC) $(CFLAGS) -o $@ -c $< $(INCS)
	
%.o:%.cpp
	$(CPP) $(CFLAGS) $(CPPFLAGS) -o $@ -c $< $(INCS)

$(output): $(objects) $(archives)
	-mkdir $(outpath)
	$(CPP) -o $@ $^ $(LIBS)

#################################
#depend:
%.d: %.c
	@set -e; rm -f $@;			\
	$(CC) -MM $(CFLAGS) $(INCS) $< > $@.$$$$;                   \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@;   \
	rm -f $@.$$$$
	
%.d: %.cpp
	@set -e; rm -f $@;			\
	$(CPP) -MM $(CPPFLAGS) $(INCS) $< > $@.$$$$;                   \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@;   \
	rm -f $@.$$$$

include $(depends)		
#################################
.PHONY : print clean
print:
	#echo $(sources)
	#echo $(objects)
	#echo $(depends)
	#echo $(CFLAGS)
	#echo $(CPPFLAGS)
	#echo $(VPATH)
			
clean:
	-rm -rf *.o
	-rm *.d
	-rm *.d.*
	-rm $(output)
##################################
