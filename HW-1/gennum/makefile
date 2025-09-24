TARGET = main
LIBNAME = gennum
LIBFILE = lib$(LIBNAME).a

CPPFLAGS = -g
LDFLAGS = -L.
LDLIBS = -l$(LIBNAME)

all: $(TARGET)

clean:
	rm -f $(TARGET) $(LIBFILE) *.o

$(LIBFILE): genint.o genfloat.o
	ar r $@ $^
	@echo "Library build complete"

$(TARGET): $(TARGET).cpp $(LIBFILE)
	g++ $(CFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@
	@echo "Build complete"