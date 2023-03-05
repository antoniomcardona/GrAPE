

void lcd_init(void); // initialize lcd

void lcd_send_cmd(char cmd); // send command to the lcd

void lcd_send_data(char data); // send data to the lcd

void lcd_send_string(char *str); // send string to the lcd

void lcd_put_cur(int row, int col); // put cursor at the entered position row (0 or 1), col (0-15);

void lcd_clear(void); // clear LCD screen

void lcd_blink_ON(void); // LCD blink ON

void lcd_blink_OFF(void); // LCD blink OFF

void initializePins(void); // initialize GPIOs (digital read)

void buttonCode(void); // UP, DOWN, and SELECT buttons

void initial_LCD_display(void); // displaying receivers

void selectedDisplay(void); // displaying selected receiver

void scanningDisplay(void);  // scanning animation