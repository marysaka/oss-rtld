.PHONY: clean all aarch64 armv7

all: aarch64 armv7

aarch64:
	$(MAKE) -f Makefile.aarch64

armv7:
	$(MAKE) -f Makefile.armv7

clean:
	$(MAKE) -f Makefile.aarch64 clean
	$(MAKE) -f Makefile.armv7 clean
