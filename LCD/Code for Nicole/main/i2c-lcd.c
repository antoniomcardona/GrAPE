
/** Put this in the src folder **/

#include "i2c-lcd.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "unistd.h"
#include "driver/gpio.h"
#include "string.h"

#define UP_BUTTON GPIO_NUM_12
#define DOWN_BUTTON GPIO_NUM_13
#define SELECT_BUTTON GPIO_NUM_35

#define SLAVE_ADDRESS_LCD 0x4E >> 1
// #define SLAVE_ADDRESS_LCD 0x4E>>1 // change this according to ur setup

esp_err_t err;

#define I2C_NUM I2C_NUM_0

static const char *TAG = "LCD";

void lcd_send_cmd(char cmd)
{
	char data_u, data_l;
	uint8_t data_t[4];			  // 8 bits
	data_u = (cmd & 0xf0);		  // separate up and bottom half into 4 bits each
	data_l = ((cmd << 4) & 0xf0); // ^
	data_t[0] = data_u | 0x0C;	  // en=1, rs=0
	data_t[1] = data_u | 0x08;	  // en=0, rs=0
	data_t[2] = data_l | 0x0C;	  // en=1, rs=0
	data_t[3] = data_l | 0x08;	  // en=0, rs=0
	err = i2c_master_write_to_device(I2C_NUM, SLAVE_ADDRESS_LCD, data_t, 4, 1000);
	// return 0 if successful and -1 if not

	if (err != 0)
		ESP_LOGI(TAG, "Error in sending data");
}

void lcd_send_data(char data)
{
	char data_u, data_l;
	uint8_t data_t[4];
	data_u = (data & 0xf0);
	data_l = ((data << 4) & 0xf0);
	data_t[0] = data_u | 0x0D; // en=1, rs=0
	data_t[1] = data_u | 0x09; // en=0, rs=0
	data_t[2] = data_l | 0x0D; // en=1, rs=0
	data_t[3] = data_l | 0x09; // en=0, rs=0
	err = i2c_master_write_to_device(I2C_NUM, SLAVE_ADDRESS_LCD, data_t, 4, 1000);
	if (err != 0)
		ESP_LOGI(TAG, "Error in sending data");
}

/*Clear Screen*/
void lcd_clear(void)
{
	lcd_send_cmd(0x01);
	usleep(1000);
}

/*Cursor Blink ON*/
void lcd_blink_ON(void)
{
	lcd_send_cmd(0xD); // 1 D C B
	usleep(1000);	   // 1 1 0 1
}

/*Cursor Blink OFF*/
void lcd_blink_OFF(void)
{
	lcd_send_cmd(0xC); // 1 D C B
	usleep(1000);	   // 1 1 0 0
}

/*Initialize Digital Pins*/
void initializePins(void)
{
	gpio_set_direction(DOWN_BUTTON, GPIO_MODE_DEF_INPUT);
}

/*Select Cursor Position*/
void lcd_put_cur(int row, int col)
{
	switch (row)
	{
	case 0:
		col |= 0x80; // First Line
		break;
	case 1:
		col |= 0xC0; // Second Line
		break;
	case 2:
		col |= 0x94; // Third Line
		break;
	case 3:
		col |= 0xD4; // Fourth Line
		break;
	}

	lcd_send_cmd(col);
}

void lcd_init(void)
{
	// 4 bit initialisation
	usleep(50000); // wait for >50ms
	lcd_send_cmd(0x30);
	usleep(5000); // wait for >5ms
	lcd_send_cmd(0x30);
	usleep(200); // wait for >0.2ms
	lcd_send_cmd(0x30);
	usleep(10000);
	lcd_send_cmd(0x20); // 4bit mode
	usleep(10000);

	// dislay initialisation
	lcd_send_cmd(0x28); // Function set --> DL=0 (4 bit mode), N = 1 (2 line display) F = 0 (5x8 characters)
	usleep(1000);
	lcd_send_cmd(0x08); // Display on/off control --> D=0,C=0, B=0  ---> display off
	usleep(1000);
	lcd_send_cmd(0x01); // clear display
	usleep(1000);
	usleep(1000);
	lcd_send_cmd(0x06); // Entry mode set --> I/D = 1 (increment cursor) & S = 0 (no shift)
	usleep(1000);
	lcd_send_cmd(0x0C); // Display on/off control --> D = 1, C and B = 0. (Cursor and blink, last two bits)
	usleep(1000);
}

void lcd_send_string(char *str)
{
	while (*str)
		lcd_send_data(*str++);
}
