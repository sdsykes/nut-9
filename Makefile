all : wunderc

wunderc : wunder.o
	cc wunder.o -o wunderc

clean :
	rm -f wunderc wunder.o

.PHONY: all clean
