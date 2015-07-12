all:
	make -C src
	make -C src install

clean:
	make -C src clean
	rm -f pmdapi.exe

