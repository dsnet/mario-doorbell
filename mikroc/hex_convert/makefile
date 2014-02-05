all:
	gcc -o hex_convert hex_convert.c

run: all
	./hex_convert sounds/coin.wav sounds/life-up.wav sounds/mushroom.wav sounds/mario.wav sounds/outta-time.wav sounds/down-pipe.wav | tee eeprom.log

clean:
	rm -rf hex_convert
