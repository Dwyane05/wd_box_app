ARCH?=mips

TOPDIR=${PWD}

CFLAGS := -Wall -Wno-psabi
#CFLAGS += -I${PWD}
CFLAGS += -I${PWD}/../include
CFLAGS +=  -DDEBUG

Q=@
CROSS_COMPILE= /opt/ingenic_compiler/mips-gcc472-glibc216/bin/mips-linux-gnu-
#CROSS_COMPILE=/opt/allwiner_compiler/external-toolchain/bin/arm-linux-gnueabi-
LIBS_PATH=/opt/yoko_libs

#libzbar
#CFLAGS += -I${LIBS_PATH}/allwinner/zbar_qronly/include
#LDFLAGS += -lzbar -L${LIBS_PATH}/allwinner/zbar_qronly/lib

#libopencv
CFLAGS += -I${LIBS_PATH}/ingenic/opencv320/include
LDFLAGS += -lopencv_core -lopencv_video -lopencv_videoio  -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -lopencv_features2d -lopencv_xfeatures2d -lopencv_flann -lopencv_ml -lopencv_shape -lopencv_calib3d -L${LIBS_PATH}/ingenic/opencv320/lib

CXX=${CROSS_COMPILE}g++
CC=${CROSS_COMPILE}gcc

# for optimizing, close it for debug
#CFLAGS += -O 
#CFLAGS += -Os
#CFLAGS += -std=gnu++11
#CFLAGS += -std=c++11

LDFLAGS += -lrt -lpthread 

ifdef FULL
CFLAGS_FULL:= -Wextra -Wfloat-equal -save-temps 
else
CFLAGS_FULL:=
endif

#CFLAGS += -I${PWD}/api
#LDFLAGS += -lasound -lm

src := main.cpp 

src += gprs/gprs.cpp
CFLAGS += -I${PWD}/gprs

src += json/json_reader.cpp json/json_writer.cpp json/json_value.cpp 
CFLAGS += -I${PWD}/json

src += msqueue/msqueue.cpp 
CFLAGS += -I${PWD}/msqueue

src += gpio/gpio.cpp 
CFLAGS += -I${PWD}/gpio

src += serial/serial.cpp 
CFLAGS += -I${PWD}/serial

src += button/button.cpp 
CFLAGS += -I${PWD}/button

src += lock/lock.cpp 
CFLAGS += -I${PWD}/lock

src += heart/heart.cpp 
CFLAGS += -I${PWD}/heart

src += app_process/msg_process.cpp app_process/image_lock_process.cpp app_process/capture_process.cpp 
CFLAGS += -I${PWD}/app_process
#
src += hodgepodge/hodgepodge.cpp 
CFLAGS += -I${PWD}/hodgepodge

src += pic_match/surfPictureMatch.cpp 
CFLAGS += -I${PWD}/pic_match

#MISCDIR = src/misc
#MISC_FILE = $(foreach d,$(MISCDIR),$(wildcard $(d)/*.cpp) )
#src += $(MISC_FILE)
src += misc/error.cpp misc/sleepus.cpp

objects=$(patsubst %.cpp,%.o, $(src))
target = wd_box_app

.PHONY:all
all:${target}

${target}:$(objects)
	$(Q)$(CXX) $^ ${LDFLAGS} -o $@
	
%.o:%.cpp
	$(Q)$(CXX) -c $^ ${CFLAGS} ${CFLAGS_FULL} -o $@

.PHONY:clean
clean:
	@rm -f  ${target}
	@find -name "*.o" -execdir rm {} \;

	