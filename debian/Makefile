dummy:

install:
	mkdir -p ${DESTDIR}/etc/ld.so.conf.d
	install -m644 nprocinfo.conf ${DESTDIR}/etc/ld.so.conf.d/nprocinfo.conf
	mkdir -p ${DESTDIR}/home/z/lib
	install -m755 libnprocinfo.so.1 ${DESTDIR}/home/z/lib/libnprocinfo.so.1
	ln -s libnprocinfo.so.1 ${DESTDIR}/home/z/lib/libnprocinfo.so
	mkdir -p ${DESTDIR}/home/z/include/nprocinfo
	install -m644 procinfo.h ${DESTDIR}/home/z/include/nprocinfo/procinfo.h
