#
# apt-get install build-essential pkg-config libdbus-1-dev libapparmor-dev
# make clean
# make
# make install

# This should match SNAP_APP_ARCH
BUILD_ARCH := $(shell dpkg-architecture | grep DEB_BUILD_ARCH= | cut -f 2 -d '=')

CFLAGS += -g -O0 -Wall -Wstrict-prototypes

INSTALL_DIR_APP := ../package-dir-app/bin
INSTALL_DIR_FWK := ../package-dir-fwk/bin

all: dbus_message dbus_service

dbus_common.o: dbus_common.c dbus_common.h
	${CC} ${CFLAGS} ${LDFLAGS} $< -c ${LDLIBS} $(shell pkg-config --cflags --libs dbus-1 libapparmor)

dbus_message: dbus_message.c dbus_common.o
	${CC} ${CFLAGS} ${LDFLAGS} $^ -o $@ ${LDLIBS} $(shell pkg-config --cflags --libs dbus-1 libapparmor)

dbus_service: dbus_message dbus_service.c dbus_common.o
	${CC} ${CFLAGS} ${LDFLAGS} $(filter-out dbus_message, $^) -o $@ ${LDLIBS} $(shell pkg-config --cflags --libs dbus-1 libapparmor)

install: dbus_message dbus_service
	cp -f dbus_message ${INSTALL_DIR_APP}/dbus_message.${BUILD_ARCH}
	cp -f dbus_service ${INSTALL_DIR_FWK}/dbus_service.${BUILD_ARCH}

clean:
	rm -f ./*.o
	rm -f ./dbus_message dbus_service
