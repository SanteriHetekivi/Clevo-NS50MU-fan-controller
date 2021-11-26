service_active:=$(shell systemctl is-active NS50MU-fan-controller.service)

ifneq ($(VERBOSE) , ON)
	VERBOSE=OFF
endif

compile:
ifeq ($(VERBOSE) , ON)
	g++ -DVERBOSE NS50MU-fan-controller.cpp -o NS50MU-fan-controller
else
	g++ NS50MU-fan-controller.cpp -o NS50MU-fan-controller
endif

install-bin: compile
	chmod +x NS50MU-fan-controller
ifeq ($(service_active) , active)
	systemctl stop NS50MU-fan-controller
endif
	cp NS50MU-fan-controller /usr/local/bin/
ifeq ($(service_active) , active)
	systemctl start NS50MU-fan-controller
endif

install-service: install-bin
	cp NS50MU-fan-controller.service /etc/systemd/system/
	systemctl enable NS50MU-fan-controller.service

all: install-service
	systemctl start NS50MU-fan-controller

clean:
	rm NS50MU-fan-controller

uninstall:
	systemctl stop NS50MU-fan-controller
	systemctl disable NS50MU-fan-controller.service
	rm /etc/systemd/system/NS50MU-fan-controller.service
	rm /usr/local/bin/NS50MU-fan-controller
