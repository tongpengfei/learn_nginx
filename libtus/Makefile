
FLAGS   = -g -Wall -fPIC
FLAGS  += -DDEBUG
#FLAGS += -O3
LINK    = @echo linking $@ $(LIBS) && g++
CC      = @echo compiling c $(FLAGS) $(HEADER) $@ && gcc
GCC     = @echo compiling cpp $(FLAGS) $@ && g++
DEFINES =
HEADER  = -I./
LIBS    =
LINKFLAGS  = -shared

#LINKFLAGS += -rdynamic

TUS_SRC=.
BIN_PATH = $(TUS_SRC)/../bin/


#LIBS += -L. -lpthread
LIBS += -lrt

OBJECTS = tus.o
		 
TARGET_SO = libtus.so

all : mkdir_bin $(TARGET_SO)

mkdir_bin :
	$(shell if ! test -d $(BIN_PATH); then mkdir $(BIN_PATH); fi;)

$(TARGET_SO) : $(OBJECTS)
	$(LINK) $(FLAGS) $(LINKFLAGS) -shared -o $@ $^ $(LIBS)

.c.o:
	$(CC) -c $(HEADER) $(FLAGS) -o $@ $<

.cpp.o:
	$(GCC) -c $(HEADER) $(FLAGS) -fpermissive -o $@ $<

install:all
	cp -f $(TARGET_SO) $(BIN_PATH)

clean:
	rm -rf utils/*o
	rm -rf *.exe *.o *.so
	rm -rf main
	rm -rf $(BIN_PATH)
	rm -rf $(OBJECT)

