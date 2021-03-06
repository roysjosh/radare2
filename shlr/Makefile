include ../config-user.mk
include ../mk/${COMPILER}.mk

WGET?=wget
CS_VER=2.1.2
CS_TAR=https://github.com/aquynh/capstone/archive/$(CS_VER).tar.gz
CS_TAR=
CS_URL=https://github.com/aquynh/capstone.git
CS_UPD=20140612
CS_BRA=next
#CS_TIP=04b9da4c4a03e7c17ef4786dd31d3dbd58866203
CS_TIP=5493c87c2776b784e2692d3ec6258814de8b84ac

.PHONY: capstone-sync capstone-build all clean mrproper libgdbr

HOST_CC?=gcc
SHLR?=$(shell pwd)
AR?=ar
RANLIB?=ranlib
MODS=sdb udis86 java tcc zip grub
SDB_URL=git://github.com/radare/sdb
#SDB_URL=/Users/pancake/prg/sdb
PWD=$(shell pwd)
SDB_CONFIG=${PWD}/sdb-config.mk

all: sdb/sdb libgdbr capstone-build
	@for a in ${MODS} ; do \
		echo $(MAKE) -C $$a ; \
		$(MAKE) -C $$a RANLIB="$(RANLIB)" CC_AR="$(CC_AR)" HAVE_VALA= ROOT="${PWD}/../" CC="${CC}" ; \
	done

clean mrproper:
	-cd capstone && $(MAKE) clean
	cd gdb && $(MAKE) clean
	rm -f sdb/src/.sdb sdb/sdb
	cd sdb && $(MAKE) clean
	@for a in ${MODS} ; do $(MAKE) -C $$a clean ; done

$(SDBLIB) sdb/sdb:
	cd sdb ; $(MAKE) src/sdb-version.h
	cd sdb ; $(MAKE) clean
	cd sdb ; \
		$(MAKE) clean && \
		$(MAKE) CC=${HOST_CC} CFLAGS="${HOST_CFLAGS} -g" && \
		cp -f src/sdb src/.sdb
	cd sdb ; $(MAKE) clean
	cd sdb ; $(MAKE) src/sdb-version.h
	cd sdb/src ; $(MAKE) ARCH=xxx RANLIB="${RANLIB}" CFLAGS_SHARED=-fPIC \
		CC="${CC}" AR="${AR}" CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}" libsdb.a
	${RANLIB} sdb/src/libsdb.a
	cp -f sdb/src/.sdb sdb/sdb

.PHONY: sdb-sync sync-sdb sdbclean
F=README.md config.mk src Makefile Jamroot
SYNCFILES=$(addprefix sdb.vc/,${F})
I=../libr/include

libgdbr:
	cd gdb ; $(MAKE) all

sdb-sync sync-sdb:
	rm -rf sdb sdb.vc
	git clone ${SDB_URL} sdb.vc
	mkdir -p sdb
	cp -rf ${SYNCFILES} sdb
	rm -rf sdb.vc $I/sdb
	mkdir -p $I/sdb
	rm -f src/sdb-version.h
	cd sdb ; $(MAKE) src/sdb-version.h
	cp -f sdb/src/*.h $I/sdb
	echo '#include <sdb/sdb.h>' > $I/sdb.h
	mkdir -p sdb/test sdb/memcache
	sed -e 's,HAVE_VALA=,HAVE_VALA=#,' sdb/config.mk > .t
	mv .t sdb/config.mk
	echo all clean mrproper: | tee sdb/test/Makefile > sdb/memcache/Makefile
	git add $I/sdb*
	git add sdb

$(TCCLIB) libr_tcc/libr_tcc.a:
	cd libr_tcc ; $(MAKE)

udis-sync udis86-sync sync-udis sync-udis86:
	cd udis86 ; $(MAKE) sync

tcc-clean tccclean:
	cd libr_tcc ; $(MAKE) clean

CFILES=i386-asm.c i386-gen.c libtcc.c tccasm.c tccelf.c tccgen.c tccpp.c
HFILES=tcc.h i386-asm.h tcclib.h tcctok.h stab.h
HFILES+=elf.h libtcc.h config.h i386-tok.h

# TODO: use mob branch?

.PHONY: sdb-sync sync-sdb sdbclean
tcc-sync sync-tcc:
	rm -rf _
	git clone git://repo.or.cz/tinycc.git _
	cd _ ; ./configure --prefix=${PREFIX}
	mkdir -p tcc
	for a in ${CFILES} ${HFILES} ; do cp -f _/$$a tcc ; done
	cp -f _/VERSION _/COPYING _/stab.def tcc
	git add tcc
	rm -rf _

D=${DESTDIR}/${PREFIX}
install:
	#mkdir -p ${D}/lib
	#cp -f tcc/libr_tcc* ${D}/lib
	#cp -f zip/libr_z* ${D}/lib

install-symlink symstall:
	mkdir -p ${D}/lib
	#cd tcc ; for a in libr_tcc* ; do \
	#	ln -fs `pwd`/$$a ${D}/lib/$$a ; done

uninstall deinstall:
	#rm -f ${D}/lib/libr_tcc*
	#rm -f ${D}/lib/libr_z*


ifeq ($(CS_TAR),)
capstone: capstone-sync

capstone-sync:
	if [ ! -d capstone ]; then \
		git clone $(CS_URL) ; \
	fi
	cd capstone ; \
		git checkout $(CS_BRA) ; \
		git reset --hard $(CS_TIP) || \
	( if [ -d .git ]; then \
		git pull ; git reset --hard $(CS_TIP) ; \
	else true ; fi)

.PHONY: capstone
else

capstone-sync: capstone

capstone-clean:
	cd capstone ; $(MAKE) clean

capstone: capstone-$(CS_VER).tar.gz
	tar xzvf capstone-$(CS_VER).tar.gz
	rm -rf capstone
	mv capstone-$(CS_VER) capstone
	#cd capstone ; for a in ../capstone-patches/* ; do patch -p1 < $$a ; done

capstone-$(CS_VER).tar.gz:
	$(WGET) --no-check-certificate -O capstone-$(CS_VER).tar.gz -c $(CS_TAR)
endif

capstone-build: capstone
	cd capstone && CFLAGS="-Dmips=mips $(CFLAGS)" LDFLAGS="$(LDFLAGS)" \
		$(MAKE) CC="$(CC)" AR_EXT=a \
		RANLIB="$(RANLIB)" AR="$(AR)" \
		libcapstone.a 

