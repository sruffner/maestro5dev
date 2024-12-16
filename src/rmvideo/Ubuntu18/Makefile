APPNAME = rmvideo

OBJS = rmvmain.o rmvdisplay.o rmvio.o rmviosim.o rmvionet.o rmvrenderer.o \
   rmvtarget.o rmvmediamgr.o vidbuffer.o utilities.o

CC ?= g++
COPTS ?= -g

LIBS = -lGL -lX11 -lXext -lXrandr -lpthread \
   -lXxf86vm -lrt -lavcodec -lavformat -lswscale -lavutil -lm

$(APPNAME) : $(OBJS)
	g++ -o $(APPNAME) $(COPTS) build/*.o $(LIBS)

rmvmain.o : rmvmain.cpp rmvdisplay.h
	g++ -c $(COPTS) $< -o build/$@

rmvdisplay.o : rmvdisplay.cpp rmvdisplay.h rmvrenderer.h rmvtarget.h rmvio.h rmviosim.h \
   rmvionet.h utilities.h rmvideo_common.h
	g++ -c $(COPTS) $< -o build/$@

rmvio.o : rmvio.cpp rmvio.h rmvideo_common.h
	g++ -c $(COPTS) $< -o build/$@

rmviosim.o : rmviosim.cpp rmviosim.h rmvio.h rmvideo_common.h utilities.h
	g++ -c $(COPTS) $< -o build/$@

rmvionet.o : rmvionet.cpp rmvionet.h rmvio.h rmvideo_common.h utilities.h
	g++ -c $(COPTS) $< -o build/$@

rmvrenderer.o : rmvrenderer.cpp rmvrenderer.h rmvtarget.h rmvdisplay.h \
   rmvideo_common.h utilities.h
	g++ -c $(COPTS) $< -o build/$@

rmvtarget.o : rmvtarget.cpp rmvtarget.h rmvrenderer.h rmvideo_common.h utilities.h
	g++ -c $(COPTS) $< -o build/$@

rmvmediamgr.o : rmvmediamgr.cpp rmvmediamgr.h stb_image.h rmvio.h \
   rmvideo_common.h
	g++ -c $(COPTS) $< -o build/$@

vidbuffer.o : vidbuffer.cpp vidbuffer.h utilities.h
	g++ -c $(COPTS) $< -o build/$@

utilities.o : utilities.cpp utilities.h
	g++ -c $(COPTS) $< -o build/$@

.PHONY : clean
clean :
	-rm -f $(APPNAME) build/*.o
