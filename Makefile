PACKET_FILTER_SRC=src/packet-filter/
ERRORS_SRC=src/packet-filter/errors.c
STATISTICS_PRINTER_SRC=src/statistics-printer/main.c

all: packet-filter-a packet-filter-b statistics-printer

packet-filter-a: $(PACKET_FILTER_SRC)packet-filter-a.c $(ERRORS_SRC)
	$(CC) $(CFLAGS) -I/packet-filter -o $@ $^ -lrt -lpthread
	
packet-filter-b: $(PACKET_FILTER_SRC)/packet-filter-b.c $(ERRORS_SRC)
	$(CC) $(CFLAGS) -I/packet-filter -o $@ $^ -lrt -lpthread	
	
statistics-printer: $(STATISTICS_PRINTER_SRC)
	$(CC) $(CFLAGS) -o $@ $^ -lrt
	
install: all
	sudo install -d -m 0755 /usr/bin/
	sudo install -m 0755 packet-filter-a packet-filter-b statistics-printer /usr/bin/
	
uninstall:
	sudo rm -rf $(addprefix /usr/bin/, packet-filter statistics-printer)

deb:
	dpkg-buildpackage -D

mrproper:
	dh_clean

.PHONY: all install uninstall deb mrproper