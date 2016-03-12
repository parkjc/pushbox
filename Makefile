LDLIBS=-lm -lusb-1.0
CFLAGS=-Os -Wall
DOCCO=docco

pushbox: AutoMail.c scales.h
	gcc $(mysql_config --libs) -l wiringPi -lm -lusb-1.0 -Os -Wall -o test AutoMail.c

mail: AutoMail.c scales.h
	$(CC) $(mysql_config --libs) $(CFLAGS) $< $(LDLIBS) -o $@ -l wiringPi
usbscale: usbscale.c scales.h
	$(CC) $(CFLAGS) $< $(LDLIBS) -o $@

lsusb: lsusb.c scales.h

docs: usbscale.c
	$(DOCCO) usbscale.c

clean:
	rm -f lsscale
	rm -f usbscale
