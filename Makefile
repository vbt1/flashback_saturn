
SDL_CFLAGS = `sdl-config --cflags`
SDL_CFLAGS = 
SDL_LIBS = `sdl-config --libs`
SDL_LIBS = 

SGLDIR = C:/SaturnOrbit/SGL_302j
SGLIDR = $(SGLDIR)/inc

#LIBS = C:/vbt/saturn/SBL6/SEGALIB/LIB/vbtelf4/sega_bpl.a C:/vbt/saturn/SBL6/SEGALIB/LIB/vbtelf4/sega_gfs.a

DEFINES = -DBYPASS_PROTECTION
#DEFINES = -DBYPASS_PROTECTION -DNDEBUG

CXX = sh-elf-g++
CXXFLAGS:= connard -m2 -fno-lto -fno-builtin -O -Wall -Wuninitialized -Wno-unknown-pragmas -Wshadow -I$(SGLIDR) -I$(SBLIDR) -fpermissive
CXXFLAGS+= -Wundef -Wreorder -Wwrite-strings -Wnon-virtual-dtor -Wno-multichar
CXXFLAGS+= $(SDL_CFLAGS) $(DEFINES)

SRCS = collision.cpp cutscene.cpp file.cpp game.cpp graphics.cpp main.cpp menu.cpp \
	mixer.cpp mod_player.cpp piege.cpp resource.cpp scaler.cpp sfx_player.cpp \
	staticres.cpp systemstub_sdl.cpp unpack.cpp util.cpp video.cpp sat_mem_checker.cpp gfs_wrap.cpp lz.cpp

OBJS = $(SRCS:.cpp=.o) 
DEPS = $(SRCS:.cpp=.d)

rs: $(OBJS) $(LIBS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) $(SDL_LIBS)

.cpp.o:
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $*.o

clean:
	rm -f *.o *.d

-include $(DEPS)
