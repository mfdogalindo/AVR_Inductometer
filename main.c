#include "config.h"

#include <util/delay.h>
#include <avr/interrupt.h>
#include "lib/ssd1306.h"

// Dirección I2C del SSD1306
#define SSD1306_I2C_ADDR 0x3C

// Variables globales
volatile uint32_t seconds = 0;

// Rutina de interrupción del temporizador
ISR(TIMER1_COMPA_vect)
{
    seconds++;
}

int main(void)
{
    SSD1306_Init(SSD1306_I2C_ADDR); // 0x3C

    // DRAWING
    // -------------------------------------------------------------------------------------
    SSD1306_ClearScreen();            // clear screen
    SSD1306_SetPosition(0, 0);        // set position
    SSD1306_DrawString("Hola"); // draw string
    SSD1306_SetPosition(7, 1);                 // set position
    SSD1306_DrawString("Te amo polarcita"); // draw string
                                      // SSD1306_DrawLine(0, MAX_X, 18, 18);        // draw line
    SSD1306_UpdateScreen(SSD1306_I2C_ADDR);    // update

    _delay_ms(1000);
    SSD1306_InverseScreen(SSD1306_ADDR);

    _delay_ms(1000);
    SSD1306_NormalScreen(SSD1306_ADDR);

    DDRB |= (1 << PB5);

    // Habilitar interrupciones globales
    sei();

    // Loop principal
    char buffer[16];
    while (1)
    {
        PORTB ^= (1 << PB5);

        _delay_ms(500);
        SSD1306_InverseScreen(SSD1306_ADDR);

        _delay_ms(500);
        SSD1306_NormalScreen(SSD1306_ADDR);
    }

    return 0;
}
