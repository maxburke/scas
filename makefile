OS := $(shell uname)

.PHONY: all
all:
	cd common; make all OS=$(OS); cd ..
	cd server; make all OS=$(OS); cd ..
	cd client; make all OS=$(OS); cd ..

.PHONY: clean
clean:
	cd common; make clean OS=$(OS); cd ..
	cd server; make clean OS=$(OS); cd ..
	cd client; make clean OS=$(OS); cd ..

