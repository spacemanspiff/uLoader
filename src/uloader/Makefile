SOURCES		:=	source source/libpng/pngu source/libwbfs source/resources source/resources_alt

.PHONY: $(SOURCES)

$(SOURCES):
	cd uloader && make && cp -f -t .. *.dol  && rm -f *.dol
	cd uloader_alt && make && cp -f -t .. *.dol  && rm -f *.dol
	cd uloader_port1 && make && cp -f -t .. *.dol  && rm -f *.dol
	cd uloader_alt_port1 && make && cp -f -t .. *.dol  && rm -f *.dol



clean:
	cd uloader && make clean
	cd uloader_alt && make clean
	cd uloader_port1 && make clean
	cd uloader_alt_port1 && make clean
	

