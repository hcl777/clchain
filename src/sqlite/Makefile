#makefile:

#################################
CC := gcc
CPP := g++
AR := ar
CFLAGS	:= -Wall -g -Werror
#CFLAGS	+= -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_DLFCN_H=1 -DHAVE_FDATASYNC=1 -DHAVE_USLEEP=1 -DHAVE_LOCALTIME_R=1 -DHAVE_GMTIME_R=1 -DHAVE_DECL_STRERROR_R=1 -DHAVE_STRERROR_R=1 -DHAVE_POSIX_FALLOCATE=1 -DHAVE_ZLIB_H=1
#CFLAGS	+= -D_REENTRANT=1 -DSQLITE_THREADSAFE=1 -DSQLITE_ENABLE_FTS4 -DSQLITE_ENABLE_FTS5 -DSQLITE_ENABLE_JSON1 -DSQLITE_ENABLE_RTREE -DSQLITE_HAVE_ZLIB
CFLAGS += -DSQLITE_ENABLE_RTREE -DSQLITE_ENABLE_COLUMN_METADATA


LIBS    = -lpthread
INCS    = 

##test: sources := $(shell echo *.cpp)  or sources := $(shell ls *.cpp)
output := libsqlite.a
sources :=  $(wildcard *.cpp *.c)
#android系统没有sysinfo,需要使用sysinfo.S支持，即在objects 中增加 sysinfo.o即可
objects := $(subst .c,.o,$(subst .cpp,.o,$(sources)))
depends := $(subst .c,.d,$(subst .cpp,.d,$(sources)))
archives :=

################################
all: $(output)

%.o:%.c
	$(CC) $(CFLAGS) $(INCS) -o $@ -c $<
	
%.o:%.cpp
	$(CPP) $(CFLAGS) $(CPPFLAGS) $(INCS) -o $@ -c $<
	
%.o:%.S
	$(CPP) -c $(CFLAGS) $(CPPFLAGS) $(INCS) $< -o $@

$(output): $(objects) $(archives)
	$(AR) rv $@ $^
	
#################################
#depend:
-include $(depends)
%.d: %.c
	$(CC) -MM $(CFLAGS) $< > $@.$$$$;                   \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@;   \
	rm -f $@.$$$$
	
%.d: %.cpp
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$;                   \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@;   \
	rm -f $@.$$$$
	
#################################
.PHONY : print clean
print:
	echo $(sources)
	echo $(objects)
	echo $(depends)
	echo $(CFLAGS)
	echo $(CPPFLAGS)
	
clean:
	-rm *.d
	-rm *.o
	-rm $(output)
##################################

