magic-bin/x86_64-linux-gnu:
	mkdir ./magic-bin/x86_64-linux-gnu

magic-bin/arm-linux-gnueabihf:
	mkdir ./magic-bin/arm-linux-gnueabihf

build_amd64: magic-bin/x86_64-linux-gnu
	cd ./magic-bin/x86_64-linux-gnu && \
		go build ../../

build_arm: magic-bin/arm-linux-gnueabihf
	cd ./magic-bin/arm-linux-gnueabihf && \
		GOARCH=arm go build ../../

snap: build_amd64 build_arm
	snappy build .

clean:
	rm -rf ./magic-bin/x86_64-linux-gnu
	rm -rf ./magic-bin/arm-linux-gnueabihf
	rm -rf ./*multi.snap