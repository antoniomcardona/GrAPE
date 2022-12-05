/**
 * I2C Wiring Reference:
 * 
 * - Arduino Pin 5 (SCL) to LCD J2 Pin 3 (SCL)
 * - Arduino Pin 4 (SDA) to LCS J2 Pin 4 (SDA)
 * - GND to LCD J2 Pin 5 (VSS)
 * - 5V to LCD J2 Pin 6 (VDD)
 */

#define STARTUP_DELAY 500
#define I2C_DELAY 100
#define SLAVE_ADDRESS 0x28

#define LINE1 0x0
#define LINE2 0x40
#define LINE3 0x14
#define LINE4 0x54

// SPI Interface
uint8_t _SCL;  // 5
uint8_t _SDI;  // 4
uint8_t _CS;   // 3

// RS232 Interface
uint8_t _TX;  // 2

//I2C Interface
uint8_t _SDA;  // 4

enum Interface {
  I2C,
  SPI,
  RS232
};


Interface _interface;

// Button Pin Designation
uint8_t buttonPin1 = 8;
uint8_t buttonPin2 = 9;
uint8_t buttonPin3 = 10;
uint8_t buttonPin4 = 11;
uint8_t buttonPin5 = 12;

// Initializing Button States to LOW
uint8_t buttonState1 = 0;
uint8_t buttonState2 = 0;
uint8_t buttonState3 = 0;
uint8_t buttonState4 = 0;
uint8_t buttonState5 = 0;

// MENU
int menuPage = 0;
uint8_t position = 0x0;
uint8_t newPosition = 0x0;
int counter = 0;



const char* nicole[4] = { "Bluetooth 1", "Bluetooth 2", "Bluetooth 3", "Bluetooth 4" };




void setup() {
  Serial.begin(9600);

  initLCD_I2C(5, 4);  //initialize I2C

  startup();
  pinStartup();

  setBrightness(5);

  setCursor(LINE1);



  /**
  while (true) {
    if (buttonState1 == HIGH && buttonState2 == LOW
        && buttonState3 == LOW && buttonState4 == LOW) {
      setCursor(LINE1);
      //delay(2500);
      //clearScreen();
      //setCursor(0x44);
      //displayReceiver(nicole[0]);
      Serial.print(nicole[0]);
      //cursorBlinkOFF();
      //delay(3000);
      //clearScreen();
      //delay(500);
      //setBrightness(1);
      break;
    } else if (buttonState1 == LOW && buttonState2 == HIGH
               && buttonState3 == LOW && buttonState4 == LOW) {
      setCursor(LINE2);
      //delay(2500);
      //clearScreen();
      //setCursor(0x44);
      //displayReceiver(nicole[1]);
      Serial.print(nicole[1]);
      //cursorBlinkOFF();
      break;
    } else if (buttonState1 == LOW && buttonState2 == LOW
               && buttonState3 == HIGH && buttonState4 == LOW) {
      setCursor(LINE3);
      //delay(2500);
      //clearScreen();
      //setCursor(0x44);
      //displayReceiver(nicole[2]);
      Serial.print(nicole[2]);
      //cursorBlinkOFF();
      break;
    } else if (buttonState1 == LOW && buttonState2 == LOW
               && buttonState3 == LOW && buttonState4 == HIGH) {
      setCursor(LINE4);
      //delay(2500);
      //clearScreen();
      //setCursor(0x44);
      //displayReceiver(nicole[3]);
      Serial.print(nicole[3]);
      //cursorBlinkOFF();
      break;
    } else {
      delay(5000);
      mainMenuDraw();
      setup();
    }
  } **/
}


void loop() {
  
  menu();
  
}

void menu() {
 
  pinStartup();

  if (buttonState5 == HIGH && counter == 0) {
    setCursor(LINE2);
    delay(2000);

  } else if (buttonState5 == LOW && counter == 1) {
    setCursor(LINE3);
    delay(2000);

  } else if (buttonState5 == HIGH && counter == 2) {
    setCursor(LINE4);
    delay(2000);

  } else {
    pinStartup();
  }

  counter++;
  Serial.print(" ");
  Serial.print(counter);
}

void startup() {
  setCursor(LINE1);
  displayReceiver(nicole[0]);
  setCursor(LINE2);
  displayReceiver(nicole[1]);
  setCursor(LINE3);
  displayReceiver(nicole[2]);
  setCursor(LINE4);
  displayReceiver(nicole[3]);
}

void pinStartup() {
  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);
  pinMode(buttonPin3, INPUT);
  pinMode(buttonPin4, INPUT);
  pinMode(buttonPin5, INPUT_PULLUP);
  buttonState1 = digitalRead(buttonPin1);
  buttonState2 = digitalRead(buttonPin2);
  buttonState3 = digitalRead(buttonPin3);
  buttonState4 = digitalRead(buttonPin4);
  buttonState5 = digitalRead(buttonPin5);
  cursorBlinkON();
}

/**
 * @brief Initialize selected IO ports for I2C.
 * 
 * @param SCL Serial clock pin assigment.
 * @param SDA Serial data pin assignment.
 * @return none
 */
void initLCD_I2C(uint8_t SCL, uint8_t SDA) {
  _interface = I2C;

  // Store pin assigmnents globally
  _SCL = SCL;
  _SDA = SDA;

  // Set IO modes
  pinMode(SCL, OUTPUT);
  pinMode(SDA, OUTPUT);

  // Set starting pin states
  digitalWrite(SCL, HIGH);
  digitalWrite(SDA, HIGH);

  // Wait for display to power ON
  delay(STARTUP_DELAY);
  clearScreen();
}

/**
 * @brief Set chip/slave select HIGH and wait for 1ms.
 * 
 * @return none
 */
void setCS() {
  digitalWrite(_CS, HIGH);
  delay(1);
}

/**
 * @brief Clear chip/slave select and wait for 1ms.
 * 
 * @return none
 */
void clearCS() {
  digitalWrite(_CS, LOW);
  delay(1);
}


/**
 * @brief Send a start condition on the I2C bus.
 * 
 * @return none
 */
void startCondition() {
  clearSDA();
  clearSCL();
}

/**
 * @brief Send a stop condition on the I2C bus.
 * 
 * @return none
 */
void stopCondition() {
  setSCL();
  setSDA();
}

/**
 * @brief Set the SDA/SDI pin high on the I2C/SPI bus.
 * 
 * @return none
 */
void setSDA() {
  digitalWrite(_SDA, HIGH);
  delayMicroseconds(I2C_DELAY);
}

/**
 * @brief Clear the SDA/SDI pin on the I2C/SPI bus.
 * 
 * @return none
 */
void clearSDA() {
  digitalWrite(_SDA, LOW);
  delayMicroseconds(I2C_DELAY);
}

/**
 * @brief Set the SCL/SCK pin on the I2C/SPI bus.
 * 
 * @return none
 */
void setSCL() {
  digitalWrite(_SCL, HIGH);
  if (_interface == I2C) {
    delayMicroseconds(I2C_DELAY);
  }
}

/**
 * @brief Clear the SCL/SCK pin on the I2C/SPI bus.
 * 
 * @return none
 */
void clearSCL() {
  digitalWrite(_SCL, LOW);
  if (_interface == I2C) {
    delayMicroseconds(I2C_DELAY);
  }
}

/**
 * @brief Set the I2C bus to write mode.
 * 
 * @return none
 */
void setWriteMode() {
  putData_I2C((SLAVE_ADDRESS << 1) | 0x00);
}

/**
 * @brief Set the I2C bus to read mode.
 * 
 * @return none
 */
void setReadMode() {
  putData_I2C((SLAVE_ADDRESS << 1) | 0x01);
}

/**
 * @brief Check if an ACK/NACK was received on the I2C bus.
 * 
 * @return uint8_t The ACK/NACK read from the display.
 */
uint8_t getACK() {
  pinMode(_SDA, INPUT);
  setSCL();

  uint8_t ACK = digitalRead(_SDA);

  pinMode(_SDA, OUTPUT);
  clearSCL();

  return ACK;
}

/**
 * @brief Write 1 byte of data to the display.
 * 
 * @param data Byte of data to be written.
 * @return none
 */
void write(uint8_t data) {
  switch (_interface) {
    case I2C:
      startCondition();
      setWriteMode();
      putData_I2C(data);
      stopCondition();
      break;
  }
  delayMicroseconds(150);
}

/**
 * @brief Write an array of characters to the display.
 * 
 * @param data Pointer to the array of characters.
 * @return none
 */
void writeString(unsigned char* data) {
  // Iterate through data until null terminator is found.
  while (*data != '\0') {
    write(*data);
    data++;  // Increment pointer.
  }
}

/** Here we are probably going to need a function that allows the user
    to select a receiver from the string array.
    Utilizing "UP", "DOWN", and "SELECT" are going to be needed for this to work.

    @idea: 1. Display receivers from string array
            2. Select receiver from string array by using "UP", "DOWN", and "SELECT"
            3. Output chosen receiver to the terminal on ARDUINO UNO
**/

/** DISPLAY **/
void displayReceiver(const char* array) {
  // Iterate through data until null terminator is found.
  while (*array != '\0') {
    write(*array);
    array++;  // Increment pointer.
  }
}

void mainMenuDraw() {

  int menuLine = 0;
  int len = sizeof(nicole) / sizeof(nicole[0]);  //size of array
  setCursor(LINE1);
  delay(5000);

  /**
  if (len <= 4 && len > 0) {
    menuLine += 1;
  }
  else {
    displayOFF();
  }
**/

  while (true) {

    if (buttonState5 == HIGH) {  //down
      setCursor(LINE2);
      cursorBlinkON();
      //buttonState1 == LOW;
    } else {
      setup();
    }
  }
}





/**
 * @brief Clock each bit of data on the I2C bus and read ACK.
 * 
 * @param data Byte of data to be put on the I2C data bus.
 * @return none
 */
void putData_I2C(uint8_t data) {
  for (int i = 7; i >= 0; i--) {
    digitalWrite(_SDA, (data >> i) & 0x01);

    setSCL();
    clearSCL();
  }

  getACK();
}


/**
 * @brief Send the prefix data byte (0xFE).
 * 
 * @return none
 */
void prefix() {
  write(0xFE);
}

/**
 * @brief Turn the display ON.
 * Display is turned ON by default.
 * 
 * @return none
 */
void displayON() {
  prefix();
  write(0x41);
}

/**
 * @brief Turn the display OFF.
 * Display is turned ON by default.
 * 
 * @return none
 */
void displayOFF() {
  prefix();
  write(0x42);
}

/**
 * @brief Set the display cursor position via DDRAM address.
 * 
 * @param position Desired DDRAM address.
 * @return none
 */
void setCursor(uint8_t position) {
  prefix();
  write(0x45);
  write(position);
}

/**
 * @brief Move the cursor to line 1, column 1.
 * 
 * @return none
 */
void home() {
  prefix();
  write(0x46);
}

/**
 * @brief Clear the display screen.
 * 
 * @return none
 */
void clearScreen() {
  prefix();
  write(0x51);
  delay(2);
}

/**
 * @brief Set the display's contrast.
 * 0x00 <= contrast <= 0x32
 * Default: 0x28
 * 
 * @param contrast Desired contrast setting.
 * @return none 
 */
void setContrast(uint8_t contrast) {
  prefix();
  write(0x52);
  write(contrast);
}

/**
 * @brief Set the display's brightness.
 * 0x01 <= brightness <= 0x08
 * brightness = 0x01 | Backlight OFF
 * brightness = 0x08 | Backlight ON (100%)
 * 
 * @param brightness Desired brightness setting.
 * @return none
 */
void setBrightness(uint8_t brightness) {
  prefix();
  write(0x53);
  write(brightness);
}

/**
 * @brief Turn the underline cursor ON.
 * 
 * @return none
 */
void underlineCursorON() {
  prefix();
  write(0x47);
}

/**
 * @brief Turn the underline cursor OFF.
 * 
 * @return none
 */
void underlineCursorOFF() {
  prefix();
  write(0x48);
}

/**
 * @brief Turn the cursor blink ON.
 * 
 * @return none
 */
void cursorBlinkON() {
  prefix();
  write(0x4B);
}

/**
 * @brief Turn the cursor blink OFF.
 * 
 * @return none
 */
void cursorBlinkOFF() {
  prefix();
  write(0x4C);
}

/**
 * @brief Displays I2C address.
 * 
 * @return none
 */
void displayAddressI2C() {
  prefix();
  write(0x72);
}