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
#define SELECT_BUTTON GPIO_NUM_36

uint8_t UP_BUTTON_STATE = 0;
uint8_t DOWN_BUTTON_STATE = 0;
uint8_t SELECT_BUTTON_STATE = 0;

int pos = 0;
char buffer[10];

static const char *TAG = "i2c-simple-example";


/**
 * @brief i2c master initialization
 */
static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_NUM_0;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,  // Might need to change this to "I2C_NUM_0"
        .sda_io_num = GPIO_NUM_23,  //SDA - J2 -> 4
        .scl_io_num = GPIO_NUM_19,  //SCL - J2 -> 3
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,  // Original 100000
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



void app_main(void)
{
    uint8_t data[2];
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");


    //ESP_ERROR_CHECK(i2c_driver_delete(I2C_MASTER_NUM));
    //ESP_LOGI(TAG, "I2C unitialized successfully");

    lcd_init();
    initializePins();

    // char name[] = {'[', 'N', 'i', 'c', 'o', 'l', 'e', ']', '[', 'A', 'n', 't', 'o', 'n', 'i', 'o', ']', '\0'};
    // char str[100];
    // int i = 0;
    // int j = 0;
    // for (i = 0; i < sizeof(name) / sizeof(name[0]); i++) {
    //     /*Remove Brackets from Bluetooth Receivers*/
    //     if(name[i] != '[' && name[i] != ']') 
    //     {
    //         str[j] = name[i];;
    //         j++;
    //     }
    // }

    // char name[] = {'[', 'N', 'i', 'c', 'o', 'l', 'e', ']', '[', 'A', 'n', 't', 'o', 'n', 'i', 'o', ']', '\0'};
    // char str1[20]; // create string variable for name1
    // char str2[20]; // create string variable for name2
    // int i, j = 0;
    // int name_num = 1; // initialize name number
    // for (i = 0; i < sizeof(name) / sizeof(name[0]); i++) {
    //     if (name[i] == '[') { // start of a new name
    //         j = 0; // reset index variable
    //         name_num++; // increment name number
    //     } else if (name[i] == ']') { // end of current name
    //         if (name_num == 2) { // current name is "Antonio"
    //             str1[j] = '\0'; // add null terminator
    //             printf("name 1: ");
    //             printf("%s\n", str1); // print name2
    //         } else { // current name is "Nicole"
    //             str2[j] = '\0'; // add null terminator
    //             printf("name 2: ");
    //             printf("%s\n", str2); // print name1
    //         }
    //     } else { // non-bracket characters
    //         if (name_num == 2) { // current name is "Antonio"
    //             str1[j] = name[i]; // copy non-bracket characters to string variable
    //             j++;
    //         } else { // current name is "Nicole"
    //             str2[j] = name[i]; // copy non-bracket characters to string variable
    //             j++;
    //         }
    //     }
    // }

    

    // lcd_put_cur(0, 0);
    // lcd_send_string(str1);
    // lcd_put_cur(1, 0);
    // lcd_send_string(str2);
    // lcd_put_cur(2, 0);
    // lcd_send_string("Nathan's Speaker");
    // lcd_put_cur(3, 0);
    // lcd_send_string("GrAPE");    

    gpio_set_direction(UP_BUTTON, GPIO_MODE_INPUT);  //pin12
    gpio_set_direction(DOWN_BUTTON, GPIO_MODE_INPUT);  //pin13
    gpio_set_direction(SELECT_BUTTON, GPIO_MODE_INPUT);  //pin14

    iterationDisplay();
    lcd_put_cur(0, 0);
    lcd_blink_ON();
    buttonCode();

}

void iterationDisplay ()
{
    char name[] = {'[', 'N', 'i', 'c', 'o', 'l', 'e', ']', '[', 'A', 'n', 't', 'o', 'n', 'i', 'o', ']', '[', 'M', 'a', 'r', 'k', ']', '\0'};
    char str[20]; // create string variable for name
    int i, j = 0;
    int row = 0;
    int name_num = 0; // initialize name number
    for (i = 0; i < sizeof(name) / sizeof(name[0]); i++) {
        if (name[i] == '[') { // start of a new name
            j = 0; // reset index variable
            name_num++; // increment name number
        } else if (name[i] == ']') { // end of current name
            str[j] = '\0'; // add null terminator
            lcd_put_cur(row, 0);
            lcd_send_string(str);
            row++;
        } else { // non-bracket characters
            str[j] = name[i]; // copy non-bracket characters to string variable
            j++;
        }
    }
}

void selectedDisplay1 ()
{
    char name1[] = {'[', 'N', 'i', 'c', 'o', 'l', 'e', ']', '[', 'A', 'n', 't', 'o', 'n', 'i', 'o', ']', '[', 'M', 'a', 'r', 'k', ']', '\0'};
    char str1[20]; // create string variable for name
    int i1, j1 = 0;
    
    int name_num1 = 0; // initialize name number
    for (i1 = 0; i1 < sizeof(name1) / sizeof(name1[0]); i1++) {
        if (name1[i1] == '[') { // start of a new name
            j1 = 0; // reset index variable
            name_num1++; // increment name number
        } else if (name1[i1] == ']') { // end of current name
            str1[j1] = '\0'; // add null terminator
                lcd_clear();
                delay(2);
                lcd_put_cur(1, 6);
                lcd_send_string(str1);
                lcd_blink_OFF();
                printf("%s\n", str1);  //DEBUG
                break;
        } else { // non-bracket characters
            str1[j1] = name1[i1]; // copy non-bracket characters to string variable
            j1++;
        }
    }
    buttonCode();
}

void buttonCode () 
{
    while(true) 
    {
        UP_BUTTON_STATE = gpio_get_level(UP_BUTTON);
        DOWN_BUTTON_STATE = gpio_get_level(DOWN_BUTTON);
        SELECT_BUTTON_STATE = gpio_get_level(SELECT_BUTTON);

        /*UP BUTTON*/
        if (UP_BUTTON_STATE == 1)
        {
            if (pos == 0)
            {
                //here the cursor is at the top, therefore nothing needs to happen
                delay(1); 
                continue;
            }
            else if (pos == 1)
            {
                //here the cursor is at the 2nd line, therefore we move the cursor to 1st line
                pos = pos - 1;
                lcd_put_cur(0, 0);
                delay(1);
                
                continue;
            }
            else if (pos == 2) 
            {
                //here the cursor is at the 3rd line, therefore we move the cursor to 2nd line
                pos = pos - 1;
                lcd_put_cur(1, 0);
                delay(1);
                
                continue;
            }
            else if (pos == 3) 
            {
                //here the cursor is at the bottom, therefore we move the cursor to the 3rd line
                pos = pos - 1;
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
                //here the cursor is at the top, therefore we move the cursor to 2nd line
                pos = pos + 1; 
                lcd_put_cur(1, 0);
                //sprintf(buffer, "Pos:%d", pos);
                //lcd_send_string(buffer);
                delay(1);
                continue;
            }
            else if (pos == 1)
            {
                //here the cursor is at the 2nd line, therefore we move the cursor to 3rd line
                pos = pos + 1;
                lcd_put_cur(2, 0);
                //printf(buffer, "Pos1:%d", pos);
                //lcd_send_string(buffer);
                delay(1);
                continue;
            }
            else if (pos == 2) 
            {
                //here the cursor is at the 3rd line, therefore we move the cursor to 4th line
                pos = pos + 1;
                lcd_put_cur(3, 0);
                //sprintf(buffer, "Pos2:%d", pos);
                //lcd_send_string(buffer);
                delay(1);
                continue;
            }
            else if (pos == 3) 
            {
                //here the cursor is at the bottom, therefore nothing needs to happen
                //delay(3);
                continue;
            }
        }
        /*SELECT BUTTON*/
        if (SELECT_BUTTON_STATE == 1)
        {
            delay(1);
            printf("Hi %d\n", SELECT_BUTTON);
            selectedDisplay1();
        }

        vTaskDelay(1);
    }
}
