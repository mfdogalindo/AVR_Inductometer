
exe.hex: exe.elf
	avr-objcopy -O ihex -R .eeprom exe.elf exe.hex

exe.elf: main.c 
	avr-gcc -mmcu=atmega328p -O3 main.c ./lib/twi.c ./lib/ssd1306.c -o exe.elf && avr-objcopy -O ihex -R .eeprom exe.elf exe.hex

u: exe.hex
	sudo avrdude -p atmega328p -c arduino -P /dev/cu.usbserial-11320 -b 57600 -U flash:w:exe.hex:i -v

asm: main.c
	avr-gcc -mmcu=atmega328p -S -O3 main.c -o main.asm


clean:
	rm *.hex *.o *.asm *.elf