all:install
	
compile:
	g++ N151ZU-fan-controller.cpp -o N151ZU-fan-controller

install-bin: compile
	chmod +x N151ZU-fan-controller
	cp N151ZU-fan-controller /usr/local/bin/

install-service:install-bin
	cp N151ZU-fan-controller.service /etc/systemd/system/
	systemctl enable N151ZU-fan-controller.service

launch-service:install-service
	service N151ZU-fan-controller start
	
clean:
	rm N151ZU-fan-controller

uninstall:
	service N151ZU-fan-controller stop
	systemctl disable N151ZU-fan-controller.service
	rm /etc/systemd/system/N151ZU-fan-controller.service
	rm /usr/local/bin/N151ZU-fan-controller
