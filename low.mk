
TARGET = yo

all: $(TARGET)

clean:
	-rm -f $(TARGET).ll $(TARGET).s $(TARGET)

dump: $(TARGET)
	readelf -r -s $<

%.ll: %.low
	./lowc $<

%.s: %.ll
	llc -o $@ $<

%: %.s
	gcc -o $@ $<