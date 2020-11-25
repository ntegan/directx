

CXX=x86_64-w64-mingw32-g++


.default_goal = all

objects = Vertices.o
executable = dxmain

$(executable): $(objects)
	$(CXX) $(objects) -o $(executable) -ld3d9

.PHONY: all
all: $(executable)

.PHONY: clean
clean: 
	$(RM) $(objects) $(executable)
