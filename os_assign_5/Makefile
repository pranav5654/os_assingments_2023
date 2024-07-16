EXE=vector matrix

all: clean $(EXE)
	./vector
%: %.cpp
	g++ -O3 -std=c++11 -o $@ $^ -lpthread

clean:
	rm -rf $(EXE) 2>/dev/null
