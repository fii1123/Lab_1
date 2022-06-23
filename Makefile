PACKET_FILTER_SRC=src/packet-filter/
ERRORS_SRC=src/packet-filter/errors.c
STATISTICS_PRINTER_SRC=src/statistics-printer/main.c

all: packet_filter_a packet_filter_b statistics_printer

packet_filter_a: $(PACKET_FILTER_SRC)packet_filter_a.c $(ERRORS_SRC)
	$(CC) $(CFLAGS) -I/packet_filter -o $@ $^ -lrt -lpthread
	
packet_filter_b: $(PACKET_FILTER_SRC)/packet_filter_b.c $(ERRORS_SRC)
	$(CC) $(CFLAGS) -I/packet_filter -o $@ $^ -lrt -lpthread	
	
statistics_printer: $(STATISTICS_PRINTER_SRC)
	$(CC) $(CFLAGS) -o $@ $^ -lrt
	
install: all
	install -d -m 0755 /usr/bin/
	install -m 0755 packet_filter_a packet_filter_b statistics_printer /usr/bin/
	
uninstall:
	rm -rf $(addprefix /usr/bin/, packet_filter statistics_printer)

deb:
	dpkg-buildpackage -D

mrproper:
	dh_clean

.PHONY: all install uninstall deb mrproper