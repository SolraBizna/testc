CC=cc
CFLAGS=

all: testc.o testcread testcwrite testcdepth

testc: testc.c
	$(CC) $(CFLAGS) testc.c -c -o testc

testcread: testcread.c
	$(CC) $(CFLAGS) testcread.c -o testcread

testcwrite: testcwrite.c
	$(CC) $(CFLAGS) testcwrite.c -o testcwrite

testcdepth: testcdepth.c
	$(CC) $(CFLAGS) testcdepth.c -o testcdepth

clean:
	rm -f testc testcread testcwrite testcdepth

install: testc testcread testcwrite testcdepth
	cp testc /etc/boot.d/testc
	cp testcread /usr/local/bin/testcread
	cp testcwrite /usr/local/bin/testcwrite
	cp testcdepth /usr/local/bin/testcdepth
	cp testc_init /etc/install.d/init.d/testc
	chmod 755 /usr/local/bin/testcread /usr/local/bin/testcwrite /usr/local/bin/testcdepth /etc/install.d/init.d/testc
	chmod 644 /etc/boot.d/testc
	echo "Do not forget to run newconfig"
