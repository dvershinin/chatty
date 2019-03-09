CC = gcc
CFLAGS = -O2 -DHAVE_openpty
LDFLAGS = -lutil
VERSION = 0.1

TARGET = chatty dict-test

DIST =	README chatty.c dict.c dict.h dict-test.c Makefile \
	chatty-dict.en chatty-dict.ja\
	dict-list.c\
	tests

all: $(TARGET)

chatty: chatty.o dict.o
	$(CC) $(CFLAGS) -o chatty chatty.o dict.o $(LDFLAGS)

dict-test: dict-test.o dict.o
	$(CC) $(CFLAGS) -o dict-test dict-test.o dict.o $(LDFLAGS)

dict-list.o: dict-list.c
	$(CC) $(CFLAGS) `glib-config --cflags` -c dict-list.c

dict-test2: dict-test.o dict-list.o
	$(CC) $(CFLAGS) `glib-config --cflags` -o dict-test2 \
	dict-test.o dict-list.o $(LDFLAGS) `glib-config --libs`

clean:
	rm -f *.o $(TARGET)
	cd tests && make clean

dist:
	make clean
	rm -rf chatty-$(VERSION)
	rm -f chatty-$(VERSION).tar.gz

	mkdir chatty-$(VERSION)
	cp -rp $(DIST) chatty-$(VERSION)
	rm -rf chatty-$(VERSION)/tests/*~ chatty-$(VERSION)tests/*.bak 
	rm -rf chatty-$(VERSION)/tests/CVS
	tar zcf chatty-$(VERSION).tar.gz  chatty-$(VERSION)
	tar ztf chatty-$(VERSION).tar.gz
	rm -rf chatty-$(VERSION)

check: all
	@cd tests && make check

gene.txt:
	wget http://www.namazu.org/~tsuchiya/sdic/data/gene95.tar.gz
	tar zxf gene95.tar.gz gene.txt

gene.dic: gene.txt
	nkf -deSZ2 gene.txt | \
	perl -ne 'BEGIN{<>;<>;}; \
		  s/\s+$$//; $$def = <>; $$def =~ s/, */, /g; \
		  print "$$_\t$$def"' > $@

ccmalloc:
	make clean
	make CFLAGS='-g' LDFLAGS='-lccmalloc -ldl'
	./dict-test -c chatty-dict.ja
