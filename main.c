#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <avr/interrupt.h>

// ---------------- LCD (PORTD) ----------------
#define LCD_PORT PORTD
#define LCD_DDR  DDRD
#define RS PD2
#define EN PD3

// ---------------- RELAY ----------------
#define RELAY PB0

// ---------------- RESET BUTTON ----------------
#define BTN_RESET PC2

// ---------------- GLOBALS ----------------
volatile float energy_wh = 0.0;
volatile float g_power   = 0.0;
uint8_t overload_latch   = 0;
float peak_current       = 0.0;
float peak_power         = 0.0;

// ---------------- LCD FUNCTIONS ----------------
void lcd_pulse()
{
	LCD_PORT |= (1<<EN);
	_delay_ms(1);
	LCD_PORT &= ~(1<<EN);
	_delay_ms(1);
}

void lcd_cmd(unsigned char cmd)
{
	LCD_PORT = (LCD_PORT & 0x03) | (cmd & 0xF0);
	LCD_PORT &= ~(1<<RS);
	lcd_pulse();
	LCD_PORT = (LCD_PORT & 0x03) | (cmd << 4);
	LCD_PORT &= ~(1<<RS);
	lcd_pulse();
}

void lcd_data(unsigned char data)
{
	LCD_PORT = (LCD_PORT & 0x03) | (data & 0xF0);
	LCD_PORT |= (1<<RS);
	lcd_pulse();
	LCD_PORT = (LCD_PORT & 0x03) | (data << 4);
	LCD_PORT |= (1<<RS);
	lcd_pulse();
}

void lcd_init()
{
	LCD_DDR = 0xFF;
	_delay_ms(50);
	lcd_cmd(0x02);
	_delay_ms(5);
	lcd_cmd(0x28);
	lcd_cmd(0x0C);
	lcd_cmd(0x06);
	lcd_cmd(0x01);
	_delay_ms(5);
}

void lcd_print(char *str)
{
	while(*str)
	lcd_data(*str++);
}

// ---------------- ADC ----------------
void adc_init()
{
	ADMUX  = (1<<REFS0);
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
	ADCSRA |= (1<<ADSC);
	while(ADCSRA & (1<<ADSC));
}

int adc_read_avg(char ch)
{
	long sum = 0;
	int i;
	for(i = 0; i < 16; i++)
	{
		ADMUX = (ADMUX & 0xF0) | (ch & 0x0F);
		_delay_ms(1);
		ADCSRA |= (1<<ADSC);
		while(ADCSRA & (1<<ADSC));
		sum += ADC;
	}
	return (int)(sum / 16);
}

// ---------------- TIMER1 ----------------
void timer1_init()
{
	TCCR1B |= (1<<WGM12)|(1<<CS12)|(1<<CS10);
	OCR1A = 15624;
	TIMSK1 |= (1<<OCIE1A);
}

ISR(TIMER1_COMPA_vect)
{
	energy_wh += g_power / 3600.0;
}

// ---------------- BUTTON ----------------
uint8_t button_pressed(uint8_t pin)
{
	if(!(PINC & (1<<pin)))
	{
		_delay_ms(20);
		if(!(PINC & (1<<pin)))
		return 1;
	}
	return 0;
}

// ---------------- MAIN ----------------
int main()
{
	char v_str[8], i_str[8], p_str[8], e_str[10];
	float Vadc, Vadci;
	float Voltage, Current, Power;
	uint8_t page = 0;
	uint8_t page_timer = 0;

	// RELAY
	DDRB |= (1<<RELAY);
	PORTB |= (1<<RELAY);

	// BUTTON
	DDRC  &= ~(1<<BTN_RESET);
	PORTC |=  (1<<BTN_RESET);

	lcd_init();
	adc_init();
	timer1_init();
	sei();

	while(1)
	{
		int adc0 = adc_read_avg(0);   // A0 ? voltage divider
		int adc1 = adc_read_avg(1);   // A1 ? ACS712 VIOUT

		Vadc  = (adc0 * 5.0) / 1023.0;
		Vadci = (adc1 * 5.0) / 1023.0;

		// -------- VOLTAGE --------
		// A0 measured = 0.8V at 220V mains
		// scaling = 220 / 0.8 = 275.0
		Voltage = Vadc * 275.0;

		// -------- CURRENT (ACS712-5A) --------
		// A1 measured = 1.4V at 0A
		// sensitivity = 185mV/A
		Current = (Vadci - 1.4) / 0.185;
		if(Current < 0) Current = 0;

		// -------- POWER --------
		Power = Voltage * Current;
		g_power = Power;

		// -------- RELAY CONTROL WITH LATCH --------
		if(Current > 5.0)
		{
			peak_current = Current;
			peak_power   = Power;
			overload_latch = 1;
			PORTB &= ~(1<<RELAY);
		}

		if(overload_latch == 0)
		{
			PORTB |= (1<<RELAY);
		}

		// -------- RESET BUTTON --------
		if(button_pressed(BTN_RESET))
		{
			cli();
			energy_wh = 0.0;
			sei();
			overload_latch = 0;
			peak_current   = 0.0;
			peak_power     = 0.0;
			PORTB |= (1<<RELAY);
			lcd_cmd(0x80);
			lcd_print("System Reset!   ");
			lcd_cmd(0xC0);
			lcd_print("                ");
			_delay_ms(1000);
		}

		// -------- FLOAT TO STRING --------
		dtostrf(Voltage, 5, 1, v_str);
		dtostrf(energy_wh, 7, 4, e_str);

		if(overload_latch)
		{
			dtostrf(peak_current, 4, 2, i_str);
			dtostrf(peak_power,   5, 1, p_str);
		}
		else
		{
			dtostrf(Current, 4, 2, i_str);
			dtostrf(Power,   5, 1, p_str);
		}

		// -------- PAGE TIMER --------
		page_timer++;
		if(page == 0 && page_timer >= 10)
		{
			page_timer = 0;
			page = 1;
		}
		else if(page == 1 && page_timer >= 10)
		{
			page_timer = 0;
			page = 0;
		}

		// -------- LCD DISPLAY --------
		if(page == 0)
		{
			lcd_cmd(0x80);
			lcd_print("V=");
			lcd_print(v_str);
			lcd_print(" I=");
			lcd_print(i_str);

			lcd_cmd(0xC0);
			lcd_print("P=");
			lcd_print(p_str);
			lcd_print(" R=");
			if(PORTB & (1<<RELAY))
			lcd_print("ON ");
			else
			lcd_print("OFF");
		}
		else
		{
			lcd_cmd(0x80);
			lcd_print("Energy Used:    ");
			lcd_cmd(0xC0);
			lcd_print(e_str);
			lcd_print(" Wh         ");
		}

		_delay_ms(500);
	}
}
