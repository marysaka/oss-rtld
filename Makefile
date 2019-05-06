.PHONY: clean all aarch64

all: aarch64

aarch64:
	$(MAKE) -f Makefile.aarch64

clean:
	$(MAKE) -f Makefile.aarch64 clean
