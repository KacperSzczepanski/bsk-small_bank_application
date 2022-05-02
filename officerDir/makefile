SRC = officerApp.c

CXX = gcc

all: officerApp

officerApp:
	$(CXX) -o officerApp $(SRC) -lpam -lpam_misc

.PHONY: clean
clean:
	rm -rf *.o officerApp