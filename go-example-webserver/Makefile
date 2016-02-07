.DEFAULT_GOAL := snap

DESTDIR ?= .
SNAPNAME = $(shell find ./magic-bin/ -type f -printf "%f\n" | head -1)

magic-bin/x86_64-linux-gnu:
	mkdir $(DESTDIR)/magic-bin/x86_64-linux-gnu --parents

magic-bin/arm-linux-gnueabihf:
	mkdir $(DESTDIR)/magic-bin/arm-linux-gnueabihf --parents

build_amd64: magic-bin/x86_64-linux-gnu
	cd $(DESTDIR)/magic-bin/x86_64-linux-gnu && \
		go build ../../ && mv build $(SNAPNAME)

build_arm: magic-bin/arm-linux-gnueabihf
	cd $(DESTDIR)/magic-bin/arm-linux-gnueabihf && \
		GOARCH=arm go build ../../ && mv build $(SNAPNAME)

snap: build_amd64 build_arm
	#snappy build .

install:
	cp -R  ../build/magic-bin ../install/

clean:
	rm -rf ./magic-bin/x86_64-linux-gnu
	rm -rf ./magic-bin/arm-linux-gnueabihf
	rm -rf ./*multi.snap