dummy:

install:
	mkdir -p ${DESTDIR}/etc/ld.so.conf.d
	install -m644 nprocinfo.conf ${DESTDIR}/etc/ld.so.conf.d/nprocinfo.conf
	mkdir -p ${DESTDIR}/home/y/lib
	install -m755 libnprocinfo.so.1 ${DESTDIR}/home/y/lib/libnprocinfo.so.1
	ln -s libnprocinfo.so.1 ${DESTDIR}/home/y/lib/libnprocinfo.so
	mkdir -p ${DESTDIR}/home/y/include/nprocinfo
	install -m644 procinfo.h ${DESTDIR}/home/y/include/nprocinfo/procinfo.h
