.PHONY: all
all:
	cd common; make all; cd ..
	cd server; make all; cd ..
	cd client; make all; cd ..

.PHONY: clean
clean:
	cd common; make clean; cd ..
	cd server; make clean; cd ..
	cd client; make clean; cd ..

