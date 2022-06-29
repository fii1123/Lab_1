PACKET_FILTER_SRC=src/packet-filter/
LIBS_SRC=src/packet-filter/safe_functions.c
STATISTICS_PRINTER_SRC=src/statistics-printer/main.c

all: packet-filter-a packet-filter-b statistics-printer

packet-filter-a: $(PACKET_FILTER_SRC)packet-filter-a.c $(LIBS_SRC)
	$(CC) $(CFLAGS) -I/packet-filter -o $@ $^ -lrt -lpthread
	
packet-filter-b: $(PACKET_FILTER_SRC)/packet-filter-b.c $(LIBS_SRC)
	$(CC) $(CFLAGS) -I/packet-filter -o $@ $^ -lrt -lpthread	
	
statistics-printer: $(STATISTICS_PRINTER_SRC)
	$(CC) $(CFLAGS) -o $@ $^ -lrt
	
clean:
	rm -f packet-filter-a packet-filter-b statistics-printer

install: all
	install -d -m 0755 /usr/bin/
	install -m 0755 packet-filter-a packet-filter-b statistics-printer /usr/bin/
	
uninstall:
	rm -rf $(addprefix /usr/bin/, packet-filter-a packet-filter-b statistics-printer)

deb:
	dpkg-buildpackage -D

mrproper:
	dh_clean

.PHONY: all install uninstall deb mrproper