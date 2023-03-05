/* i2c - Simple example

   Simple I2C example that shows how to initialize I2C
   as well as reading and writing from and to registers for a sensor connected over I2C.

   The sensor used in this example is a MPU9250 inertial measurement unit.

   For other examples please check:
   https://github.com/espressif/esp-idf/tree/master/examples

   See README.md file to get detailed usage of this example.

   This example buttonCode is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "i2c-lcd.h"
#include "time.h"
#include "string.h"

#define UP_BUTTON GPIO_NUM_12
#define DOWN_BUTTON GPIO_NUM_13
#define SELECT_BUTTON GPIO_NUM_35

uint8_t UP_BUTTON_STATE = 0;
uint8_t DOWN_BUTTON_STATE = 0;
uint8_t SELECT_BUTTON_STATE = 0;

char buffer[10];

static const char *TAG = "i2c-simple-example";

/**
 * @brief i2c master initialization
 */
static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_NUM_0;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,   // Might need to change this to "I2C_NUM_0"
        .sda_io_num = GPIO_NUM_23, // SDA - J2 -> 4
        .scl_io_num = GPIO_NUM_19, // SCL - J2 -> 3
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000, // Original 100000
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
}

void delay(int number_of_seconds)
{
    // Converting time into milli_seconds
    int milli_seconds = 1000 * number_of_seconds;

    // Storing start time
    clock_t start_time = clock();

    // looping till required time is not achieved
    while (clock() < start_time + milli_seconds)
        ;
}


// char name[] = {'[', 'N', 'i', 'c', 'o', 'l', 'e', ']', '[', 'A', 'n', 't', 'o', 'n', 'i', 'o', ']', '[', 'M', 'a', 'r', 'k', ']', '\0'};
//char name[] = {'[', 'N', 'i', 'c', 'o', 'l', 'e', ']', '[', 'A', 'n', 't', 'o', 'n', 'i', 'o', ']', '[', 'M', 'a', 'r', 'k', ']', '[', 'M', 'i', 'c', 'a', 'h', ' ', 'B',
            //    'r', 'u', 'c', 'e', ']', '[', 'R', 'o', 'b', 'e', 'r', 't', 'o', ']', '[', 'N', 'a', 't', 'h', 'a', 'n', ' ', 'F', 'i', 'n', 'l', 'e', 'y', ']', '\0'};
char name[] = {'[', 'N', 'i', 'c', 'o', 'l', 'e', ']', '[', 'A', 'n', 't', 'o', 'n', 'i', 'o', ']', '[', 'M', 'a', 'r', 'k', ']', '[', 'N', 'a', 't', 'h', 'a', 'n',
                 ' ', 'F', 'i', 'n', 'l', 'e', 'y', ']', '\0'};
//char name[] = {'[', 'J', 'B', 'L', ' ', 'C', 'h', 'a', 'r', 'g', 'e', ' ', '2', ']', '\0'};

/*Current State*/
/*If more than 4 names, the LCD attempts to display all of them*/

/*Working On*/
/*1. Display only 4 names when given more than 4 names*/
/*2. Scroll down menu using the variable 'pages' to select which names to display in which page. Max of 5 pages or 20 names*/

int receiversTotal = 0;
int pos = 0;
int pages = 1;
int row = 0;


void app_main(void)
{
    //uint8_t data[2];
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

    // ESP_ERROR_CHECK(i2c_driver_delete(I2C_MASTER_NUM));
    // ESP_LOGI(TAG, "I2C unitialized successfully");

    lcd_init();
    initializePins();

    gpio_set_direction(UP_BUTTON, GPIO_MODE_INPUT);     // pin12
    gpio_set_direction(DOWN_BUTTON, GPIO_MODE_INPUT);   // pin13
    gpio_set_direction(SELECT_BUTTON, GPIO_MODE_INPUT); // pin14

    initial_LCD_display();
}

void initial_LCD_display()
{
    char str[20]; // create string variable for name
    int i, j = 0;
    int name_num = 0; // initialize name number
    for (i = 0; i < sizeof(name) / sizeof(name[0]); i++)
    {
        if (name[i] == '[')
        {               // start of a new name
            j = 0;      // reset index variable
            name_num++; // increment name number
        }
        else if (name[i] == ']')
        {                  // end of current name
            str[j] = '\0'; // add null terminator
            /*Displays each name onto the LCD*/
            lcd_put_cur(row, 0); //  this will set the cursor from pos = 0 to pos = 3
            lcd_send_string(str); // sends string to LCD
            printf("%s\n", str);  //DEBUG
            row++;  //pos increment
            if (row == 4) // shows the first 4 names regardles of the total number of names
            {
                break;
            }
        }
        else
        {                     // non-bracket characters
            str[j] = name[i]; // copy non-bracket characters to string variable
            j++;
        }
    }
    receiversTotal = name_num; // total number of receivers!
    // 4 names = 1 page
    // 8 names = 2 pages
    // 12 names = 3 pages
    // 16 names = 4 pages
    // 20 names = 5 pages
    if (name_num > 0 && name_num < 5)
    {
        pages = 1;
        printf("Total number of pages: %d\n", pages);
    }
    else if (name_num > 4 && name_num < 9)
    {
        pages = 2;
        printf("Total number of pages: %d\n", pages);
    }
    else if (name_num > 8 && name_num < 13)
    {
        pages = 3;
        printf("Total number of pages: %d\n", pages);
    }
    else if (name_num > 12 && name_num < 17)
    {
        pages = 4;
        printf("Total number of pages: %d\n", pages);
    }
    else if (name_num > 16 && name_num < 21)
    {
        pages = 5;
        printf("Total number of pages: %d\n", pages);
    }
    
    printf("Total number of receivers: %d\n", receiversTotal); //DEBUG
    lcd_put_cur(0, 0);  // blinking cursor to home
    pos = 0;  // re-initialize pos = 0
    buttonCode();  // button code
}

void selectedDisplay()
{
    char str[20]; // create string variable for name
    int i, j = 0;

    int name_num = 0; // initialize name number
    for (i = 0; i < sizeof(name) / sizeof(name[0]); i++)
    {
        if (name[i] == '[') // start of a new name
        {
            j = 0;      // reset index variable
            name_num++; // increment name number
        }
        else if (name[i] == ']') // end of current name
        {
            str[j] = '\0'; // add null terminator
            printf("Name number: %d\n", name_num);
            if (pos == 0 && name_num == 1)
            {
                printf("Pos in function: %d\n", pos); // DEBUG
                printf("String: %s\n", str);          // DEBUG
                lcd_clear();
                delay(1);
                scanningDisplay();
                lcd_put_cur(1, 6);
                lcd_send_string(str);
                printf("Connected to: %s\n", str); // DEBUG
                break;
            }
            else if (pos == 1 && name_num == 2)
            {
                printf("Pos in function: %d\n", pos); // DEBUG
                printf("String: %s\n", str);          // DEBUG
                lcd_clear();
                delay(1);
                scanningDisplay();
                lcd_put_cur(1, 6);
                lcd_send_string(str);
                printf("Connected to: %s\n", str); // DEBUG
                break;
            }
            else if (pos == 2 && name_num == 3)
            {
                printf("Pos in function: %d\n", pos); // DEBUG
                printf("String: %s\n", str);          // DEBUG
                lcd_clear();
                delay(1);
                scanningDisplay();
                lcd_put_cur(1, 6);
                lcd_send_string(str);
                lcd_blink_OFF();
                printf("Connected to: %s\n", str); // DEBUG
                break;
            }
            else if (pos == 3 && name_num == 4)
            {
                printf("Pos in function: %d\n", pos); // DEBUG
                printf("String: %s\n", str);          // DEBUG
                lcd_clear();
                delay(1);
                scanningDisplay();
                lcd_put_cur(1, 6);
                lcd_send_string(str);
                lcd_blink_OFF();
                printf("Connected to: %s\n", str); // DEBUG
                break;
            }
        }
        else // non-bracket characters
        {
            str[j] = name[i]; // copy non-bracket characters to string variable
            j++;
        }
    }

    printf("Selected Name's position: %d\n", name_num); // DEBUG

    if (pos == 3 && (name_num == 3 || name_num == 2 || name_num == 1))
    {
        buttonCode();
    }
    else if (pos == 2 && (name_num == 2 || name_num == 1))
    {
        buttonCode();
    }
    else if (pos == 1 && name_num == 1)
    {
        buttonCode();
    }
    else if (pos == 0 && name_num == 0)
    {
        buttonCode();
    }
}

void scanningDisplay()
{
    lcd_put_cur(1, 4);
    lcd_send_string("Connecting"); // cols 14, 15, 16 need ...
    int tick = 14;  // column where the string "..." will start in the animation
    for (tick = 14; tick < 17; tick++)
    {
        lcd_put_cur(1, tick); // iteration displaying '.', '.', '.' subsequently
        delay(1);
        lcd_send_string(".");
    }
    delay(1);
    lcd_clear();
    delay(1);
}

void page2_display()
{
    char str[20]; // create string variable for name
    int i, j = 0;
    int name_num = 0; // initialize name number
    for (i = 0; i < sizeof(name) / sizeof(name[0]); i++)
    {
        if (name[i] == '[')
        {               // start of a new name
            j = 0;      // reset index variable
            name_num++; // increment name number
        }
        else if (name[i] == ']')
        {                  // end of current name
            str[j] = '\0'; // add null terminator
            //if (name_num < )
            /*Displays each name onto the LCD*/
            lcd_put_cur(row, 0); //  this will set the cursor from pos = 0 to pos = 3
            lcd_send_string(str); // sends string to LCD
            printf("%s\n", str);  //DEBUG
            row++;  //pos increment
            if (row == 4) // shows the first 4 names regardles of the total number of names
            {
                break;
            }
        }
        else
        {                     // non-bracket characters
            str[j] = name[i]; // copy non-bracket characters to string variable
            j++;
        }
    }
    receiversTotal = name_num; // total number of receivers!
    // 4 names = 1 page
    // 8 names = 2 pages
    // 12 names = 3 pages
    // 16 names = 4 pages
    // 20 names = 5 pages
    if (name_num > 0 && name_num < 5)
    {
        pages = 1;
        printf("Total number of pages: %d\n", pages);
    }
    else if (name_num > 4 && name_num < 9)
    {
        pages = 2;
        printf("Total number of pages: %d\n", pages);
    }
    else if (name_num > 8 && name_num < 13)
    {
        pages = 3;
        printf("Total number of pages: %d\n", pages);
    }
    else if (name_num > 12 && name_num < 17)
    {
        pages = 4;
        printf("Total number of pages: %d\n", pages);
    }
    else if (name_num > 16 && name_num < 21)
    {
        pages = 5;
        printf("Total number of pages: %d\n", pages);
    }
    
    printf("Total number of receivers: %d\n", receiversTotal); //DEBUG
    lcd_put_cur(0, 0);  // blinking cursor to home
    pos = 0;  // re-initialize pos = 0
    buttonCode();  // button code
}

void buttonCode()
{
    lcd_blink_ON();
    while (true)
    {
        /*Digital Read*/
        UP_BUTTON_STATE = gpio_get_level(UP_BUTTON);
        DOWN_BUTTON_STATE = gpio_get_level(DOWN_BUTTON);
        SELECT_BUTTON_STATE = gpio_get_level(SELECT_BUTTON);

        /*UP BUTTON*/
        if (UP_BUTTON_STATE == 1)
        {
            if (pos == 0)
            {
                // here the cursor is at the top, therefore nothing needs to happen
                printf("Pos: %d\n", pos); // DEBUG
                delay(1);
                continue;
            }
            else if (pos == 1)
            {
                // here the cursor is at the 2nd line, therefore we move the cursor to 1st line

                pos = pos - 1;
                printf("Pos: %d\n", pos); // DEBUG
                lcd_put_cur(0, 0);
                delay(1);

                continue;
            }
            else if (pos == 2)
            {
                // here the cursor is at the 3rd line, therefore we move the cursor to 2nd line

                pos = pos - 1;
                printf("Pos: %d\n", pos); // DEBUG
                lcd_put_cur(1, 0);
                delay(1);

                continue;
            }
            else if (pos == 3)
            {
                // here the cursor is at the bottom, therefore we move the cursor to the 3rd line

                pos = pos - 1;
                printf("Pos: %d\n", pos); // DEBUG
                lcd_put_cur(2, 0);
                delay(1);

                continue;
            }
        }
        /*DOWN BUTTON*/
        if (DOWN_BUTTON_STATE == 1)
        {
            if (pos == 0)
            {
                // here the cursor is at the top, therefore we move the cursor to 2nd line

                pos = pos + 1;
                printf("Pos: %d\n", pos); // DEBUG
                printf("Page: %d\n", pages); // DEBUG
                lcd_put_cur(1, 0);
                delay(1);
                continue;
            }
            else if (pos == 1)
            {
                // here the cursor is at the 2nd line, therefore we move the cursor to 3rd line

                pos = pos + 1;
                printf("Pos: %d\n", pos); // DEBUG
                printf("Page: %d\n", pages); // DEBUG
                lcd_put_cur(2, 0);
                delay(1);
                continue;
            }
            else if (pos == 2)
            {
                // here the cursor is at the 3rd line, therefore we move the cursor to 4th line

                pos = pos + 1;
                printf("Pos: %d\n", pos); // DEBUG
                printf("Page: %d\n", pages); // DEBUG
                lcd_put_cur(3, 0);
                delay(1);
                continue;
            }
            else if (pos == 3)
            {
                // pos = pos + 1; //TESTING
                // lcd_clear(); //TESTING
                // initial_LCD_display(); //TESTING
                // printf("Pos: %d\n", pos); // DEBUG
                printf("Page: %d\n", pages); // DEBUG
                delay(1);
                continue;
            }
        }
        /*SELECT BUTTON*/
        if (SELECT_BUTTON_STATE == 1)
        {
            delay(1);
            printf("Activated GPIO #: %d\n", SELECT_BUTTON); // DEBUG
            lcd_blink_OFF();
            selectedDisplay();
        }

        vTaskDelay(1);
    }
}
