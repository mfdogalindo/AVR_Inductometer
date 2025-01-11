#include "config.h"

#include <util/delay.h>
#include <avr/interrupt.h>
#include "lib/ssd1306.h"
#include "lib/serial.h"
#include <stdio.h>

// Dirección I2C del SSD1306
#define SSD1306_I2C_ADDR 0x3C

#define CALIBRATION 1.11
#define ARRAY_LEN 6
#define STR_LEN 32
#define CONST 1010647491 // Aproximación de 4 pi^2 16MHz^2 C / 1'000
#define DELTA_MAX 100

uint8_t i, flag, power, divisor;
uint16_t values[ARRAY_LEN];
uint16_t dmin, dmax;
uint32_t davg, davgsquared, L, Ldecimal, denominator;
char strBuffer[STR_LEN];

char unitBuffer[2];

// Variables globales
volatile uint32_t counter = 0;

ISR(TIMER1_CAPT_vect)
{
    // PORTB ^= (1 << PB5);
    if (flag == 0)
        TCNT1 = 0;
    else if (flag <= ARRAY_LEN)
        values[flag - 1] = ICR1;

    flag++;
}

int main(void)
{
    serial_init();

    SSD1306_Init(SSD1306_I2C_ADDR); // 0x3C

    // DRAWING
    // -------------------------------------------------------------------------------------
    SSD1306_ClearScreen();                  // clear screen
    SSD1306_SetPosition(0, 0);              // set position
    SSD1306_DrawString("Inductometer");     // draw string
    SSD1306_SetPosition(0, 1);              // set position
    SSD1306_DrawString("V 1.0.0");          // draw string
                                            // SSD1306_DrawLine(0, MAX_X, 18, 18);        // draw line
    SSD1306_UpdateScreen(SSD1306_I2C_ADDR); // update

    _delay_ms(1000);
    SSD1306_InverseScreen(SSD1306_ADDR);

    _delay_ms(1000);
    SSD1306_NormalScreen(SSD1306_ADDR);

    DDRB |= (1 << PB5); // Configurar pin 13 como salida
    DDRB |= (1 << PB4); // Configurar pin 12 como salida

    DIDR1 |= (1 << AIN0D); // Disable Digital Inputs at AIN0

    // Habilitar interrupciones globales
    sei();
    SREG |= (1 << 7);

    // Configura el comparador analógico (AC)
    ACSR &= ~(1 << ACD);    // Habilitar AC
    ACSR &= ~(1 << ACBG);   // Deshabilitar referencia bandgap
    ADCSRB |= (1 << ACME);  // Habilitar Mux
    ADCSRA &= ~(1 << ADEN); // Deshabilitar ADC
    ADMUX |= 0b00000111;    // Seleccionar ADC7 como entrada inversora de AC

    // Setup of time TC1
    ACSR = (1 << ACIC);     // Select AC as TC1 input capture trigger
    TCCR1B = 0b00000001;    // Use prescaler = 1
    PRR &= ~(1 << PRTIM1);  // Disable Power reduction to TC1
    TCCR1B |= (1 << ICNC1); // Enable noise cancelling (optional)
    TIMSK1 |= (1 << ICIE1); // Input capture interrupt enable for TC1

    serial_string("Inductometer V 1.0.0\n");

    while (1)
    {
        PORTB ^= (1 << PB5);
        _delay_ms(500);

        flag = 0;

        PORTB |= (1 << PB4); // Establecer pin D12 en ALTO
        // Retraso aproximado de 250 ns
        asm("nop");
        asm("nop");
        asm("nop");

        TCNT1 = 0;            // Resetear el contador
        PORTB &= ~(1 << PB4); // Establecer pin D12 en BAJO

        _delay_ms(100); // Espera para tomar muestras

        // 'Divisor' es el número de períodos válidos medidos
        divisor = ARRAY_LEN - 1;

        dmax = 0;
        dmin = 0xffff;

        // Recorrer todos los valores (períodos)
        for (i = ARRAY_LEN - 1; i > 0; i--)
        {
            // Si un valor es nulo, ajustar "divisor" y saltar iteración
            serial_string("----------------------");
            serial_break();
            sprintf(strBuffer, "Value [%d]: %u\r\n ", i, values[i]);
            serial_string(strBuffer);

            if (values[i] == 0)
            {
                divisor = i - 1;
                continue;
            }
            // Encontrar período máximo
            if (values[i] - values[i - 1] > dmax)
            {
                dmax = values[i] - values[i - 1];
            }
            // Encontrar período mínimo
            if (values[i] - values[i - 1] < dmin)
            {
                dmin = values[i] - values[i - 1];
            }
            sprintf(strBuffer, "Dmax: %u \tDmin: %u\r\n", dmax, dmin);
            serial_string(strBuffer);
        }

        // Calcular promedio de período de oscilación
        davg = (uint32_t)(values[divisor] - values[0]) / ((uint32_t)divisor);

        // flush values
        for (i = 0; i < ARRAY_LEN; i++)
            values[i] = 0;

        sprintf(strBuffer, "D AVG:%f\r\n", davg);
        serial_string(strBuffer);
        davgsquared = davg * davg;
        denominator = CONST;

        // Encontrar potencia del denominador tal que L sea n*100 e-m para formato decimal
        L = davgsquared / denominator;
        for (power = 0; L < 100; power++)
        {
            denominator /= 10;
            L = davgsquared / denominator;
        }

        L = L * CALIBRATION;

        // Corregir errores de redondeo
        if (L >= 1000)
        {
            L /= 10;
            power--;
        }

        sprintf(strBuffer, "%d", (uint16_t)L);


        // Insertar '.' en la posición correcta dentro de la cadena para el decimal
        if (power % 3)
        {
            for (i = STR_LEN - 1; i >= 4 - (power % 3); i--)
                strBuffer[i] = strBuffer[i - 1];

            strBuffer[i] = '.';
        }

        serial_string(strBuffer);
        serial_break();

        SSD1306_ClearScreen();
        SSD1306_UpdateScreen(SSD1306_I2C_ADDR);
        SSD1306_SetPosition(0, 0);

        uint8_t validValue = (dmin <= davg &&
            davg <= dmax &&
            dmin > 0 &&
            ((uint16_t)davg) / (dmax - dmin) >= DELTA_MAX && strBuffer[0] != '-') ? 1 : 0;

        // Verificar validez de las mediciones
        if (validValue == 1)
        {
            if (power >= 9)
                sprintf(unitBuffer, "nH");
            else if (power >= 6)
                sprintf(unitBuffer, "uH");
            else if (power >= 3)
                sprintf(unitBuffer, "mH");
        }
        else if (dmin == 0xffff)
            sprintf(strBuffer, "Open");
        else
            sprintf(strBuffer, "Error");

        SSD1306_DrawString(strBuffer);
        if(validValue == 1)
        {
            SSD1306_SetPosition(0, 1);
            SSD1306_DrawString(unitBuffer);
        }
        SSD1306_UpdateScreen(SSD1306_I2C_ADDR);
        SSD1306_UpdateScreen(SSD1306_I2C_ADDR);
        serial_string(strBuffer);
        serial_break();
    
    }

    return 0;
}
