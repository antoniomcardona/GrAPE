#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_log.h"

#include "esp_bt.h"
#include "bt_app_core.h"
#include "i2c-lcd.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"
#include "driver/i2s.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "time.h"
#include "string.h"
#include "freertos/ringbuf.h"

/* log tags */
#define BT_AV_TAG "BT_AV"
#define BT_RC_CT_TAG "RC_CT"

/* device name */
// char TARGET_DEVICE_NAME[423] = "Nicole is Awesome and can figure this out";
char TARGET_DEVICE_NAME[423]; // DEBUG - Antonio
// #define TARGET_DEVICE_NAME    "123456789012345678901"
#define LOCAL_DEVICE_NAME "ESP_A2DP_SRC"

/* AVRCP used transaction label */
#define APP_RC_CT_TL_GET_CAPS (0)
#define APP_RC_CT_TL_RN_VOLUME_CHANGE (1)

#define EXAMPLE_I2S_READ_LEN (16 * 1024)
/* GPIO PINS FOR LCD BUTTONS */
#define UP_BUTTON GPIO_NUM_12     // UP Button
#define DOWN_BUTTON GPIO_NUM_27   // DOWN Button
#define SELECT_BUTTON GPIO_NUM_18 // SELECT Button

/* Initializing UP, DOWN, and SELECT Buttons */
uint8_t UP_BUTTON_STATE = 0;
uint8_t DOWN_BUTTON_STATE = 0;
uint8_t SELECT_BUTTON_STATE = 0;

/* Initializing global variables */
int receiversTotal = 0;
int pos = 0;
int pages = 1;
int row = 0;

int index = 0;
int index2 = 0;
int temp = 0;
int temp2 = 0;
int count = 0;

/* Initializing buffer variable*/
char buffer[10];

#define TAG "i2c-simple-example"

/* Initializing i2c master port*/
int i2c_master_port = I2C_NUM_0;

// static const char *TAG = "i2c-simple-example";

// // #define I2C_MASTER_SCL_IO           CONFIG_I2C_MASTER_SCL      /*!< GPIO number used for I2C master clock */
// // #define I2C_MASTER_SDA_IO           CONFIG_I2C_MASTER_SDA      /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM 0 /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
// #define I2C_MASTER_FREQ_HZ          100000                     /*!< I2C master clock frequency */
// #define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
// #define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
// #define I2C_MASTER_TIMEOUT_MS       1000

struct list
{
    // char data[20];
    char data[20];
    struct list *next;
};

struct list *names = NULL;

// last two digits are '\0' which means end of string //
char listnames[423] = "[";

enum
{
    BT_APP_STACK_UP_EVT = 0x0000,   /* event for stack up */
    BT_APP_HEART_BEAT_EVT = 0xff00, /* event for heart beat */
};

/* A2DP global states */
enum
{
    APP_AV_STATE_IDLE,
    APP_AV_STATE_DISCOVERING,
    APP_AV_STATE_DISCOVERED,
    APP_AV_STATE_UNCONNECTED,
    APP_AV_STATE_CONNECTING,
    APP_AV_STATE_CONNECTED,
    APP_AV_STATE_DISCONNECTING,
};

/* sub states of APP_AV_STATE_CONNECTED */
enum
{
    APP_AV_MEDIA_STATE_IDLE,
    APP_AV_MEDIA_STATE_STARTING,
    APP_AV_MEDIA_STATE_STARTED,
    APP_AV_MEDIA_STATE_STOPPING,
};

/*********************************
 * STATIC FUNCTION DECLARATIONS
 ********************************/

/* display buffer */
void example_disp_buf(uint8_t *buf, int length);

/* installation for i2s */
static void bt_i2s_driver_install(void);

/* uninstallation for i2s */
static void bt_i2s_driver_uninstall(void);

// /* installation for i2c */
static esp_err_t i2c_master_init(void);

void delay(int number_of_seconds);

void pageOne();

void pageTwo();

void pageThree();

void pageFour();

void pageFive();

void welcomeDisplay();

void selectedDisplay();

void connectingDisplay();

void buttonCode();

void manualNames();

/* handler for bluetooth stack enabled events */
static void bt_av_hdl_stack_evt(uint16_t event, void *p_param);

/* avrc controller event handler */
static void bt_av_hdl_avrc_ct_evt(uint16_t event, void *p_param);

// char* filter_inquiry_scan_result(esp_bt_gap_cb_param_t *param);

// void addname(struct list** pp, const char *t);
// void print(struct list* n);

/* GAP callback function */
void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

/* callback function for A2DP source */
static void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);

/* callback function for A2DP source audio data stream */
static int32_t bt_app_a2d_data_cb(uint8_t *data, int32_t len);

/* callback function for AVRCP controller */
static void bt_app_rc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);

/* handler for heart beat timer */
static void bt_app_a2d_heart_beat(TimerHandle_t arg);

/* A2DP application state machine */
static void bt_app_av_sm_hdlr(uint16_t event, void *param);

/* utils for transfer BLuetooth Deveice Address into string form */
static char *bda2str(esp_bd_addr_t bda, char *str, size_t size);

/* A2DP application state machine handler for each state */
static void bt_app_av_state_unconnected_hdlr(uint16_t event, void *param);
static void bt_app_av_state_connecting_hdlr(uint16_t event, void *param);
static void bt_app_av_state_connected_hdlr(uint16_t event, void *param);
static void bt_app_av_state_disconnecting_hdlr(uint16_t event, void *param);

/*********************************
 * STATIC VARIABLE DEFINITIONS
 ********************************/

static esp_bd_addr_t s_peer_bda = {0};                       /* Bluetooth Device Address of peer device*/
static uint8_t s_peer_bdname[ESP_BT_GAP_MAX_BDNAME_LEN + 1]; /* Bluetooth Device Name of peer device*/
static int s_a2d_state = APP_AV_STATE_IDLE;                  /* A2DP global state */
static int s_media_state = APP_AV_MEDIA_STATE_IDLE;          /* sub states of APP_AV_STATE_CONNECTED */
static int s_intv_cnt = 0;                                   /* count of heart beat intervals */
static int s_connecting_intv = 0;                            /* count of heart beat intervals for connecting */
static uint32_t s_pkt_cnt = 0;                               /* count of packets */
static esp_avrc_rn_evt_cap_mask_t s_avrc_peer_rn_cap;        /* AVRC target notification event capability bit mask */
static TimerHandle_t s_tmr;                                  /* handle of heart beat timer */

/*********************************
 * STATIC FUNCTION DEFINITIONS
 ********************************/

void example_disp_buf(uint8_t *buf, int length)
{
    printf("======\n");
    for (int i = 0; i < length; i++)
    {
        printf("%02x ", buf[i]);
        if ((i + 1) % 8 == 0)
        {
            printf("\n");
        }
    }
    printf("======\n");
}

void bt_i2s_driver_install(void)
{
    /* I2S configuration parameters */
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX | I2S_MODE_ADC_BUILT_IN,
        .sample_rate = 44100,
        .bits_per_sample = 16,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, /* 2-channels */
        .communication_format = I2S_COMM_FORMAT_STAND_MSB,
        .dma_buf_count = 6,
        .dma_buf_len = 60,
        .intr_alloc_flags = 0, /* default interrupt priority */
        .use_apll = 0,
    };
    /* enable I2S */
    i2s_driver_install(0, &i2s_config, 0, NULL);
    i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_0);
}

void bt_i2s_driver_uninstall(void)
{
    i2s_driver_uninstall(0);
}

static char *bda2str(esp_bd_addr_t bda, char *str, size_t size)
{
    if (bda == NULL || str == NULL || size < 18)
    {
        return NULL;
    }

    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
            bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
    return str;
}

static bool get_name_from_eir(uint8_t *eir, uint8_t *bdname, uint8_t *bdname_len)
{
    uint8_t *rmt_bdname = NULL;
    uint8_t rmt_bdname_len = 0;

    if (!eir)
    {
        return false;
    }

    /* get complete or short local name from eir data */
    rmt_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &rmt_bdname_len);
    if (!rmt_bdname)
    {
        rmt_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &rmt_bdname_len);
    }

    if (rmt_bdname)
    {
        if (rmt_bdname_len > ESP_BT_GAP_MAX_BDNAME_LEN)
        {
            rmt_bdname_len = ESP_BT_GAP_MAX_BDNAME_LEN;
        }

        if (bdname)
        {
            memcpy(bdname, rmt_bdname, rmt_bdname_len);
            bdname[rmt_bdname_len] = '\0';
        }
        if (bdname_len)
        {
            *bdname_len = rmt_bdname_len;
        }
        return true;
    }

    return false;
}

/**
 * @brief i2c master initialization
 */
static esp_err_t i2c_master_init(void)
{

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

    // looping till required time is achieved
    while (clock() < start_time + milli_seconds)
        ;
}

void manualNames()
{
    char addedList[423] = {',', 'S', 'e', 'a', 'r', 'c', 'h', '_', 'A', 'g', 'a', 'i', 'n', ']', '\0'}; // Search_Again feature on the LCD to be added at the end

    /* Removes last bracket from listnames */
    for (index = 0; index < sizeof(listnames) / sizeof(listnames[0]); index++)
    {
        if (listnames[index] == ']')
        {
            listnames[index] = '\0';
            temp = index;
            break;
        }
    }

    /* Adds the Search_Again manual receiver to the actual list of receivers */
    for (index2 = 0; index2 < sizeof(listnames) / sizeof(listnames[0]); index2++)
    {
        listnames[temp] = addedList[index2];

        if (listnames[temp] == ']')
        {
            break;
        }

        temp++;
    }
}

void pageOne() // displays receivers 1, 2, 3, 4
{
    lcd_clear();
    /* Initializing variables */
    char str[20] = {'\0'}; // create string variable for names
    int i = 0;
    int j = 0;
    int name_num = 0; // initialize listnames number
    int line = 0;

    for (i = 0; i < sizeof(listnames) / sizeof(listnames[0]); i++)
    {
        if (listnames[i] == '[') // beginnning of input (open bracket)
        {
            name_num++; // increment number of names
            if (j == 20)
            {
                break;
            }
        }
        else if (listnames[i] != '[' && listnames[i] != ',' && listnames[i] != ']') // non bracket characters
        {
            str[j] = listnames[i]; // store characters in str
            j++;                   // increment index

            if (j == 20)
            {
                break;
            }
        }
        else if (listnames[i] == ',')
        {

            str[j] = '\0';        // set str[j] to NULL
            lcd_put_cur(line, 0); // set cursor to row, and column 0
            lcd_send_string(str); // display listnames onto the LCD
            j = 0;                // reset index
            name_num++;           // increment the number of names
            line++;               // increment row

            if (name_num == 5)
            {
                break;
            }
        }
        else if (listnames[i] == ']') // last bracket / last listnames to display
        {
            str[j] = '\0';
            lcd_put_cur(line, 0); // set cursor to row, and column 0
            lcd_send_string(str); // display listnames onto the LCD
            // row++;                // increment row
        }
    }
    receiversTotal = name_num; // total number of receivers!

    if (name_num > 0 && name_num < 5) // 4 names = 1 page
    {
        pages = 1;
        printf("Total number of pages: %d\n", pages);
    }
    else if (name_num > 4 && name_num < 9) // 8 names = 2 pages
    {
        pages = 2;
        printf("Total number of pages: %d\n", pages); // 12 names = 3 pages
    }
    else if (name_num > 8 && name_num < 13)
    {
        pages = 3;
        printf("Total number of pages: %d\n", pages); // 16 names = 4 pages
    }
    else if (name_num > 12 && name_num < 17)
    {
        pages = 4;
        printf("Total number of pages: %d\n", pages); // 20 names = 5 pages
    }
    else if (name_num > 16 && name_num < 21)
    {
        pages = 5;
        printf("Total number of pages: %d\n", pages);
    }

    lcd_put_cur(0, 0); // blinking cursor to home
    pos = 0;           // re-initialize pos = 0
}

void selectedDisplay()
{
    /* Initializing Variables */
    char str[20] = {'\0'}; // create string variable for names
    int i = 0;
    int j = 0;
    int name_num = 0; // initialize listnames number
    float value = 0;
    int value1 = 0;

    for (i = 0; i < sizeof(listnames) / sizeof(listnames[0]); i++)
    {
        if (listnames[i] == '[') // beginnning of input (open bracket)
        {
            name_num++; // increment number of names
        }
        else if (listnames[i] != '[') // non bracket characters
        {
            str[j] = listnames[i]; // store characters in str
            j++;                   // increment index
            if (j == 20)
            {
                break;
            }
            if (listnames[i] == ',' || listnames[i] == ']')
            {
                if (pos == 0 && name_num == 1)
                {
                    j -= 1;
                    str[j] = '\0';
                    value = (j / 20.0) * 10.0;
                    value1 = value;

                    if (value1 % 2 == 0)
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (9 - value + 1));
                        lcd_send_string(str);
                    }
                    else
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value));
                        lcd_send_string(str);
                    }

                    printf("Beginning Target Device name : %s\n", TARGET_DEVICE_NAME);
                    memset(TARGET_DEVICE_NAME, 0, sizeof(TARGET_DEVICE_NAME));
                    printf("Erased Target Device name : %s\n", TARGET_DEVICE_NAME);

                    strncat(TARGET_DEVICE_NAME, str, 20);
                    printf("Set new Target Device Name to : %s\n", TARGET_DEVICE_NAME);

                    break;
                }
                if (pos == 1 && name_num == 2)
                {
                    j -= 1;
                    str[j] = '\0';
                    value = (j / 20.0) * 10.0;
                    value1 = value;

                    if (value1 % 2 == 0)
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value + 1));
                        lcd_send_string(str);
                    }
                    else
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value));
                        lcd_send_string(str);
                    }

                    printf("Beginning Target Device name : %s\n", TARGET_DEVICE_NAME);
                    memset(TARGET_DEVICE_NAME, 0, sizeof(TARGET_DEVICE_NAME));
                    printf("Erased Target Device name : %s\n", TARGET_DEVICE_NAME);

                    strncat(TARGET_DEVICE_NAME, str, 20);
                    printf("Set new Target Device Name to : %s\n", TARGET_DEVICE_NAME);

                    break;
                }
                if (pos == 2 && name_num == 3)
                {
                    j -= 1;
                    str[j] = '\0';
                    value = (j / 20.0) * 10.0;
                    value1 = value;

                    if (value1 % 2 == 0)
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value + 1));
                        lcd_send_string(str);
                    }
                    else
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value));
                        lcd_send_string(str);
                    }

                    printf("Beginning Target Device name : %s\n", TARGET_DEVICE_NAME);
                    memset(TARGET_DEVICE_NAME, 0, sizeof(TARGET_DEVICE_NAME));
                    printf("Erased Target Device name : %s\n", TARGET_DEVICE_NAME);

                    strncat(TARGET_DEVICE_NAME, str, 20);
                    printf("Set new Target Device Name to : %s\n", TARGET_DEVICE_NAME);

                    break;
                }
                if (pos == 3 && name_num == 4)
                {
                    j -= 1;
                    str[j] = '\0';
                    value = (j / 20.0) * 10.0;
                    value1 = value;

                    if (value1 % 2 == 0)
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value + 1));
                        lcd_send_string(str);
                    }
                    else
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value));
                        lcd_send_string(str);
                    }

                    printf("Beginning Target Device name : %s\n", TARGET_DEVICE_NAME);
                    memset(TARGET_DEVICE_NAME, 0, sizeof(TARGET_DEVICE_NAME));
                    printf("Erased Target Device name : %s\n", TARGET_DEVICE_NAME);

                    strncat(TARGET_DEVICE_NAME, str, 20);
                    printf("Set new Target Device Name to : %s\n", TARGET_DEVICE_NAME);

                    break;
                }
                if (pos == 4 && name_num == 5)
                {
                    j -= 1;
                    str[j] = '\0';
                    value = (j / 20.0) * 10.0;
                    value1 = value;

                    if (value1 % 2 == 0)
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value + 1));
                        lcd_send_string(str);
                    }
                    else
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value));
                        lcd_send_string(str);
                    }

                    printf("Beginning Target Device name : %s\n", TARGET_DEVICE_NAME);
                    memset(TARGET_DEVICE_NAME, 0, sizeof(TARGET_DEVICE_NAME));
                    printf("Erased Target Device name : %s\n", TARGET_DEVICE_NAME);

                    strncat(TARGET_DEVICE_NAME, str, 20);
                    printf("Set new Target Device Name to : %s\n", TARGET_DEVICE_NAME);

                    break;
                }
                if (pos == 5 && name_num == 6)
                {
                    j -= 1;
                    str[j] = '\0';
                    value = (j / 20.0) * 10.0;
                    value1 = value;

                    if (value1 % 2 == 0)
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value + 1));
                        lcd_send_string(str);
                    }
                    else
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value));
                        lcd_send_string(str);
                    }

                    printf("Beginning Target Device name : %s\n", TARGET_DEVICE_NAME);
                    memset(TARGET_DEVICE_NAME, 0, sizeof(TARGET_DEVICE_NAME));
                    printf("Erased Target Device name : %s\n", TARGET_DEVICE_NAME);

                    strncat(TARGET_DEVICE_NAME, str, 20);
                    printf("Set new Target Device Name to : %s\n", TARGET_DEVICE_NAME);

                    break;
                }
                if (pos == 6 && name_num == 7)
                {
                    j -= 1;
                    str[j] = '\0';
                    value = (j / 20.0) * 10.0;
                    value1 = value;

                    if (value1 % 2 == 0)
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value + 1));
                        lcd_send_string(str);
                    }
                    else
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value));
                        lcd_send_string(str);
                    }

                    printf("Beginning Target Device name : %s\n", TARGET_DEVICE_NAME);
                    memset(TARGET_DEVICE_NAME, 0, sizeof(TARGET_DEVICE_NAME));
                    printf("Erased Target Device name : %s\n", TARGET_DEVICE_NAME);

                    strncat(TARGET_DEVICE_NAME, str, 20);
                    printf("Set new Target Device Name to : %s\n", TARGET_DEVICE_NAME);

                    break;
                }
                if (pos == 7 && name_num == 8)
                {
                    j -= 1;
                    str[j] = '\0';
                    value = (j / 20.0) * 10.0;
                    value1 = value;

                    if (value1 % 2 == 0)
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value + 1));
                        lcd_send_string(str);
                    }
                    else
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value));
                        lcd_send_string(str);
                    }

                    printf("Beginning Target Device name : %s\n", TARGET_DEVICE_NAME);
                    memset(TARGET_DEVICE_NAME, 0, sizeof(TARGET_DEVICE_NAME));
                    printf("Erased Target Device name : %s\n", TARGET_DEVICE_NAME);

                    strncat(TARGET_DEVICE_NAME, str, 20);
                    printf("Set new Target Device Name to : %s\n", TARGET_DEVICE_NAME);

                    break;
                }
                if (pos == 8 && name_num == 9)
                {
                    j -= 1;
                    str[j] = '\0';
                    value = (j / 20.0) * 10.0;
                    value1 = value;

                    if (value1 % 2 == 0)
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value + 1));
                        lcd_send_string(str);
                    }
                    else
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value));
                        lcd_send_string(str);
                    }

                    printf("Beginning Target Device name : %s\n", TARGET_DEVICE_NAME);
                    memset(TARGET_DEVICE_NAME, 0, sizeof(TARGET_DEVICE_NAME));
                    printf("Erased Target Device name : %s\n", TARGET_DEVICE_NAME);

                    strncat(TARGET_DEVICE_NAME, str, 20);
                    printf("Set new Target Device Name to : %s\n", TARGET_DEVICE_NAME);

                    break;
                }
                if (pos == 9 && name_num == 10)
                {
                    j -= 1;
                    str[j] = '\0';
                    value = (j / 20.0) * 10.0;
                    value1 = value;

                    if (value1 % 2 == 0)
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value + 1));
                        lcd_send_string(str);
                    }
                    else
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value));
                        lcd_send_string(str);
                    }

                    printf("Beginning Target Device name : %s\n", TARGET_DEVICE_NAME);
                    memset(TARGET_DEVICE_NAME, 0, sizeof(TARGET_DEVICE_NAME));
                    printf("Erased Target Device name : %s\n", TARGET_DEVICE_NAME);

                    strncat(TARGET_DEVICE_NAME, str, 20);
                    printf("Set new Target Device Name to : %s\n", TARGET_DEVICE_NAME);

                    break;
                }
                if (pos == 10 && name_num == 11)
                {
                    j -= 1;
                    str[j] = '\0';
                    value = (j / 20.0) * 10.0;
                    value1 = value;

                    if (value1 % 2 == 0)
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value + 1));
                        lcd_send_string(str);
                    }
                    else
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value));
                        lcd_send_string(str);
                    }

                    printf("Beginning Target Device name : %s\n", TARGET_DEVICE_NAME);
                    memset(TARGET_DEVICE_NAME, 0, sizeof(TARGET_DEVICE_NAME));
                    printf("Erased Target Device name : %s\n", TARGET_DEVICE_NAME);

                    strncat(TARGET_DEVICE_NAME, str, 20);
                    printf("Set new Target Device Name to : %s\n", TARGET_DEVICE_NAME);

                    break;
                }
                if (pos == 11 && name_num == 12)
                {
                    j -= 1;
                    str[j] = '\0';
                    value = (j / 20.0) * 10.0;
                    value1 = value;

                    if (value1 % 2 == 0)
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value + 1));
                        lcd_send_string(str);
                    }
                    else
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value));
                        lcd_send_string(str);
                    }

                    printf("Beginning Target Device name : %s\n", TARGET_DEVICE_NAME);
                    memset(TARGET_DEVICE_NAME, 0, sizeof(TARGET_DEVICE_NAME));
                    printf("Erased Target Device name : %s\n", TARGET_DEVICE_NAME);

                    strncat(TARGET_DEVICE_NAME, str, 20);
                    printf("Set new Target Device Name to : %s\n", TARGET_DEVICE_NAME);

                    break;
                }
                if (pos == 12 && name_num == 13)
                {
                    j -= 1;
                    str[j] = '\0';
                    value = (j / 20.0) * 10.0;
                    value1 = value;

                    if (value1 % 2 == 0)
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value + 1));
                        lcd_send_string(str);
                    }
                    else
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value));
                        lcd_send_string(str);
                    }

                    printf("Beginning Target Device name : %s\n", TARGET_DEVICE_NAME);
                    memset(TARGET_DEVICE_NAME, 0, sizeof(TARGET_DEVICE_NAME));
                    printf("Erased Target Device name : %s\n", TARGET_DEVICE_NAME);

                    strncat(TARGET_DEVICE_NAME, str, 20);
                    printf("Set new Target Device Name to : %s\n", TARGET_DEVICE_NAME);

                    break;
                }
                if (pos == 13 && name_num == 14)
                {
                    j -= 1;
                    str[j] = '\0';
                    value = (j / 20.0) * 10.0;
                    value1 = value;

                    if (value1 % 2 == 0)
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value + 1));
                        lcd_send_string(str);
                    }
                    else
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value));
                        lcd_send_string(str);
                    }

                    printf("Beginning Target Device name : %s\n", TARGET_DEVICE_NAME);
                    memset(TARGET_DEVICE_NAME, 0, sizeof(TARGET_DEVICE_NAME));
                    printf("Erased Target Device name : %s\n", TARGET_DEVICE_NAME);

                    strncat(TARGET_DEVICE_NAME, str, 20);
                    printf("Set new Target Device Name to : %s\n", TARGET_DEVICE_NAME);

                    break;
                }
                if (pos == 14 && name_num == 15)
                {
                    j -= 1;
                    str[j] = '\0';
                    value = (j / 20.0) * 10.0;
                    value1 = value;

                    if (value1 % 2 == 0)
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value + 1));
                        lcd_send_string(str);
                    }
                    else
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value));
                        lcd_send_string(str);
                    }

                    printf("Beginning Target Device name : %s\n", TARGET_DEVICE_NAME);
                    memset(TARGET_DEVICE_NAME, 0, sizeof(TARGET_DEVICE_NAME));
                    printf("Erased Target Device name : %s\n", TARGET_DEVICE_NAME);

                    strncat(TARGET_DEVICE_NAME, str, 20);
                    printf("Set new Target Device Name to : %s\n", TARGET_DEVICE_NAME);

                    break;
                }
                if (pos == 15 && name_num == 16)
                {
                    j -= 1;
                    str[j] = '\0';
                    value = (j / 20.0) * 10.0;
                    value1 = value;

                    if (value1 % 2 == 0)
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value + 1));
                        lcd_send_string(str);
                    }
                    else
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value));
                        lcd_send_string(str);
                    }

                    printf("Beginning Target Device name : %s\n", TARGET_DEVICE_NAME);
                    memset(TARGET_DEVICE_NAME, 0, sizeof(TARGET_DEVICE_NAME));
                    printf("Erased Target Device name : %s\n", TARGET_DEVICE_NAME);

                    strncat(TARGET_DEVICE_NAME, str, 20);
                    printf("Set new Target Device Name to : %s\n", TARGET_DEVICE_NAME);

                    break;
                }
                if (pos == 16 && name_num == 17)
                {
                    j -= 1;
                    str[j] = '\0';
                    value = (j / 20.0) * 10.0;
                    value1 = value;

                    if (value1 % 2 == 0)
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value + 1));
                        lcd_send_string(str);
                    }
                    else
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value));
                        lcd_send_string(str);
                    }

                    printf("Beginning Target Device name : %s\n", TARGET_DEVICE_NAME);
                    memset(TARGET_DEVICE_NAME, 0, sizeof(TARGET_DEVICE_NAME));
                    printf("Erased Target Device name : %s\n", TARGET_DEVICE_NAME);

                    strncat(TARGET_DEVICE_NAME, str, 20);
                    printf("Set new Target Device Name to : %s\n", TARGET_DEVICE_NAME);

                    break;
                }
                if (pos == 17 && name_num == 18)
                {
                    j -= 1;
                    str[j] = '\0';
                    value = (j / 20.0) * 10.0;
                    value1 = value;

                    if (value1 % 2 == 0)
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (9 - value + 1));
                        lcd_send_string(str);
                    }
                    else
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value));
                        lcd_send_string(str);
                    }

                    printf("Beginning Target Device name : %s\n", TARGET_DEVICE_NAME);
                    memset(TARGET_DEVICE_NAME, 0, sizeof(TARGET_DEVICE_NAME));
                    printf("Erased Target Device name : %s\n", TARGET_DEVICE_NAME);

                    strncat(TARGET_DEVICE_NAME, str, 20);
                    printf("Set new Target Device Name to : %s\n", TARGET_DEVICE_NAME);

                    break;
                }
                if (pos == 18 && name_num == 19)
                {
                    j -= 1;
                    str[j] = '\0';
                    value = (j / 20.0) * 10.0;
                    value1 = value;

                    if (value1 % 2 == 0)
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value + 1));
                        lcd_send_string(str);
                    }
                    else
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value));
                        lcd_send_string(str);
                    }

                    printf("Beginning Target Device name : %s\n", TARGET_DEVICE_NAME);
                    memset(TARGET_DEVICE_NAME, 0, sizeof(TARGET_DEVICE_NAME));
                    printf("Erased Target Device name : %s\n", TARGET_DEVICE_NAME);

                    strncat(TARGET_DEVICE_NAME, str, 20);
                    printf("Set new Target Device Name to : %s\n", TARGET_DEVICE_NAME);

                    break;
                }
                if (pos == 19 && name_num == 20)
                {
                    j -= 1;
                    str[j] = '\0';
                    value = (j / 20.0) * 10.0;
                    value1 = value;

                    if (value1 % 2 == 0)
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value + 1));
                        lcd_send_string(str);
                    }
                    else
                    {
                        lcd_clear();
                        delay(1);
                        connectingDisplay();
                        lcd_put_cur(1, (10 - value));
                        lcd_send_string(str);
                    }

                    printf("Beginning Target Device name : %s\n", TARGET_DEVICE_NAME);
                    memset(TARGET_DEVICE_NAME, 0, sizeof(TARGET_DEVICE_NAME));
                    printf("Erased Target Device name : %s\n", TARGET_DEVICE_NAME);

                    strncat(TARGET_DEVICE_NAME, str, 20);
                    printf("Set new Target Device Name to : %s\n", TARGET_DEVICE_NAME);

                    break;
                }
                name_num++;
                str[j] = '\0';
                j = 0;
            }
        }
        else if (listnames[i] == ']') // last bracket / last listnames to display
        {
            str[j] = '\0';
            row++; // increment row
        }
    }
}

void connectingDisplay()
{
    lcd_put_cur(1, 4);
    lcd_send_string("Connecting...");
    delay(3);
    lcd_clear();
    delay(1);
}

void welcomeDisplay()
{
    lcd_put_cur(1, 7);
    lcd_send_string("G");
    delay(1);
    lcd_put_cur(1, 8);
    lcd_send_string("r");
    lcd_put_cur(1, 9);
    lcd_send_string("A");
    delay(1);
    lcd_put_cur(1, 10);
    lcd_send_string("P");
    lcd_put_cur(1, 11);
    lcd_send_string("E");
    delay(3);
}

void searchingDisplay()
{
    lcd_clear();
    delay(1);
    lcd_put_cur(1, 5);
    lcd_send_string("Searching...");
}

void pageTwo()
{
    lcd_clear();
    /* Initializing Variables */
    char str[20] = {'\0'}; // create string variable for names
    int i = 0;
    int j = 0;
    int name_num = 0; // initialize listnames number
    int line = 0;

    for (i = 0; i < sizeof(listnames) / sizeof(listnames[0]); i++)
    {
        if (listnames[i] == '[') // beginnning of input (open bracket)
        {
            name_num++; // increment number of names
            if (j == 20)
            {
                break;
            }
            // printf("test 1 - start of string array\n");
        }
        else if (listnames[i] != '[' && listnames[i] != ',' && listnames[i] != ']') // non bracket characters
        {
            str[j] = listnames[i]; // store characters in str
            j++;                   // increment index
            if (j == 20)
            {
                break;
            }
        }
        else if (listnames[i] == ',')
        {
            str[j] = '\0'; // set str[j] to NULL
            if (name_num > 4 && name_num < 9)
            {
                lcd_put_cur(line, 0); // set cursor to row, and column 0
                lcd_send_string(str); // display listnames onto the LCD
                line++;
            }
            j = 0;      // reset index
            name_num++; // increment the number of names
            if (name_num == 9)
            {
                break;
            }
        }
        else if (listnames[i] == ']') // last bracket / last listnames to display
        {
            if (name_num > 4)
            {
                str[j] = '\0';
                lcd_put_cur(line, 0); // set cursor to row, and column 0
                lcd_send_string(str); // display listnames onto the LCD
            }
        }
    }
    receiversTotal = name_num; // total number of receivers!

    if (name_num > 0 && name_num < 5) // 4 names = 1 page
    {
        pages = 1;
        printf("Total number of pages: %d\n", pages);
    }
    else if (name_num > 4 && name_num < 9) // 8 names = 2 pages
    {
        pages = 2;
        printf("Total number of pages: %d\n", pages); // 12 names = 3 pages
    }
    else if (name_num > 8 && name_num < 13)
    {
        pages = 3;
        printf("Total number of pages: %d\n", pages); // 16 names = 4 pages
    }
    else if (name_num > 12 && name_num < 17)
    {
        pages = 4;
        printf("Total number of pages: %d\n", pages); // 20 names = 5 pages
    }
    else if (name_num > 16 && name_num < 21)
    {
        pages = 5;
        printf("Total number of pages: %d\n", pages);
    }

    lcd_put_cur(0, 0); // blinking cursor to home
    pos = 4;           // re-initialize pos = 4
}

void pageThree()
{
    lcd_clear();

    /* Initializing Variables */
    char str[20] = {'\0'}; // create string variable for names
    int i = 0;
    int j = 0;
    int name_num = 0; // initialize listnames number
    int line = 0;

    for (i = 0; i < sizeof(listnames) / sizeof(listnames[0]); i++)
    {
        if (listnames[i] == '[') // beginnning of input (open bracket)
        {
            name_num++; // increment number of names
            if (j == 20)
            {
                break;
            }
            // printf("test 1 - start of string array\n");
        }
        else if (listnames[i] != '[' && listnames[i] != ',' && listnames[i] != ']') // non bracket characters
        {
            str[j] = listnames[i]; // store characters in str
            j++;                   // increment index
            if (j == 20)
            {
                break;
            }
        }
        else if (listnames[i] == ',')
        {
            str[j] = '\0'; // set str[j] to NULL
            if (name_num > 8 && name_num < 13)
            {
                lcd_put_cur(line, 0); // set cursor to row, and column 0
                lcd_send_string(str); // display listnames onto the LCD
                line++;
            }
            j = 0;      // reset index
            name_num++; // increment the number of names
            if (name_num == 13)
            {
                break;
            }
        }
        else if (listnames[i] == ']') // last bracket / last listnames to display
        {
            if (name_num > 8)
            {
                str[j] = '\0';
                lcd_put_cur(line, 0); // set cursor to row, and column 0
                lcd_send_string(str); // display listnames onto the LCD
            }
        }
    }
    receiversTotal = name_num; // total number of receivers!

    if (name_num > 0 && name_num < 5) // 4 names = 1 page
    {
        pages = 1;
        printf("Total number of pages: %d\n", pages);
    }
    else if (name_num > 4 && name_num < 9) // 8 names = 2 pages
    {
        pages = 2;
        printf("Total number of pages: %d\n", pages); // 12 names = 3 pages
    }
    else if (name_num > 8 && name_num < 13)
    {
        pages = 3;
        printf("Total number of pages: %d\n", pages); // 16 names = 4 pages
    }
    else if (name_num > 12 && name_num < 17)
    {
        pages = 4;
        printf("Total number of pages: %d\n", pages); // 20 names = 5 pages
    }
    else if (name_num > 16 && name_num < 21)
    {
        pages = 5;
        printf("Total number of pages: %d\n", pages);
    }

    lcd_put_cur(0, 0); // blinking cursor to home
    pos = 8;           // re-initialize pos = 8
}

void pageFour()
{
    lcd_clear();
    char str[20] = {'\0'}; // create string variable for names
    int i = 0;
    int j = 0;
    int name_num = 0; // initialize listnames number
    int line = 0;

    for (i = 0; i < sizeof(listnames) / sizeof(listnames[0]); i++)
    {
        if (listnames[i] == '[') // beginnning of input (open bracket)
        {
            name_num++; // increment number of names
            if (j == 20)
            {
                break;
            }
            // printf("test 1 - start of string array\n");
        }
        else if (listnames[i] != '[' && listnames[i] != ',' && listnames[i] != ']') // non bracket characters
        {
            str[j] = listnames[i]; // store characters in str
            j++;                   // increment index
            if (j == 20)
            {
                break;
            }
        }
        else if (listnames[i] == ',')
        {
            str[j] = '\0'; // set str[j] to NULL
            if (name_num > 12 && name_num < 17)
            {
                lcd_put_cur(line, 0); // set cursor to row, and column 0
                lcd_send_string(str); // display listnames onto the LCD
                line++;
            }
            j = 0;      // reset index
            name_num++; // increment the number of names
            if (name_num == 17)
            {
                break;
            }
        }
        else if (listnames[i] == ']') // last bracket / last listnames to display
        {
            if (name_num > 12)
            {
                str[j] = '\0';
                lcd_put_cur(line, 0); // set cursor to row, and column 0
                lcd_send_string(str); // display listnames onto the LCD
            }
        }
    }
    receiversTotal = name_num; // total number of receivers!

    if (name_num > 0 && name_num < 5) // 4 names = 1 page
    {
        pages = 1;
        printf("Total number of pages: %d\n", pages);
    }
    else if (name_num > 4 && name_num < 9) // 8 names = 2 pages
    {
        pages = 2;
        printf("Total number of pages: %d\n", pages); // 12 names = 3 pages
    }
    else if (name_num > 8 && name_num < 13)
    {
        pages = 3;
        printf("Total number of pages: %d\n", pages); // 16 names = 4 pages
    }
    else if (name_num > 12 && name_num < 17)
    {
        pages = 4;
        printf("Total number of pages: %d\n", pages); // 20 names = 5 pages
    }
    else if (name_num > 16 && name_num < 21)
    {
        pages = 5;
        printf("Total number of pages: %d\n", pages);
    }

    lcd_put_cur(0, 0); // blinking cursor to home
    pos = 12;          // re-initialize pos = 12
}

void pageFive()
{
    lcd_clear();

    /* Initialzing Variables */
    char str[20] = {'\0'}; // create string variable for names
    int i = 0;
    int j = 0;
    int name_num = 0; // initialize listnames number
    int line = 0;

    for (i = 0; i < sizeof(listnames) / sizeof(listnames[0]); i++)
    {
        if (listnames[i] == '[') // beginnning of input (open bracket)
        {
            name_num++; // increment number of names
            if (j == 20)
            {
                break;
            }
            // printf("test 1 - start of string array\n");
        }
        else if (listnames[i] != '[' && listnames[i] != ',' && listnames[i] != ']') // non bracket characters
        {
            str[j] = listnames[i]; // store characters in str
            j++;                   // increment index
            if (j == 20)
            {
                break;
            }
        }
        else if (listnames[i] == ',')
        {
            str[j] = '\0'; // set str[j] to NULL
            if (name_num > 16 && name_num < 21)
            {
                lcd_put_cur(line, 0); // set cursor to row, and column 0
                lcd_send_string(str); // display listnames onto the LCD
                line++;
            }
            j = 0;      // reset index
            name_num++; // increment the number of names
            if (name_num == 21)
            {
                break;
            }
        }
        else if (listnames[i] == ']') // last bracket / last listnames to display
        {
            if (name_num > 16)
            {
                str[j] = '\0';
                lcd_put_cur(line, 0); // set cursor to row, and column 0
                lcd_send_string(str); // display listnames onto the LCD
            }
        }
    }
    receiversTotal = name_num; // total number of receivers!

    if (name_num > 0 && name_num < 5) // 4 names = 1 page
    {
        pages = 1;
        printf("Total number of pages: %d\n", pages);
    }
    else if (name_num > 4 && name_num < 9) // 8 names = 2 pages
    {
        pages = 2;
        printf("Total number of pages: %d\n", pages); // 12 names = 3 pages
    }
    else if (name_num > 8 && name_num < 13)
    {
        pages = 3;
        printf("Total number of pages: %d\n", pages); // 16 names = 4 pages
    }
    else if (name_num > 12 && name_num < 17)
    {
        pages = 4;
        printf("Total number of pages: %d\n", pages); // 20 names = 5 pages
    }
    else if (name_num > 16 && name_num < 21)
    {
        pages = 5;
        printf("Total number of pages: %d\n", pages);
    }

    lcd_put_cur(0, 0); // blinking cursor to home
}

void buttonCode()
{
    lcd_blink_ON();
    bool selected = true;
    while (selected)
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
                delay(1);
                continue;
            }
            else if (pos == 1)
            {
                pos = pos - 1;
                lcd_put_cur(0, 0);
                delay(1);

                continue;
            }
            else if (pos == 2)
            {
                pos = pos - 1;
                lcd_put_cur(1, 0);
                delay(1);

                continue;
            }
            else if (pos == 3)
            {
                pos = pos - 1;
                lcd_put_cur(2, 0);
                delay(1);

                continue;
            }
            else if (pos == 4)
            {
                pos = pos - 1;
                pageOne();
                delay(1);

                continue;
            }
            else if (pos == 5)
            {
                pos = pos - 1;
                lcd_put_cur(0, 0);
                delay(1);

                continue;
            }
            else if (pos == 6)
            {
                pos = pos - 1;
                lcd_put_cur(1, 0);
                delay(1);

                continue;
            }
            else if (pos == 7)
            {
                pos = pos - 1;
                lcd_put_cur(2, 0);
                delay(1);

                continue;
            }
            else if (pos == 8)
            {
                pos = pos - 1;
                pageTwo();
                delay(1);

                continue;
            }
            else if (pos == 9)
            {
                pos = pos - 1;
                lcd_put_cur(0, 0);
                delay(1);

                continue;
            }
            else if (pos == 10)
            {
                pos = pos - 1;
                lcd_put_cur(1, 0);
                delay(1);

                continue;
            }
            else if (pos == 11)
            {
                pos = pos - 1;
                lcd_put_cur(2, 0);
                delay(1);

                continue;
            }
            else if (pos == 12)
            {
                pos = pos - 1;
                pageThree();
                delay(1);

                continue;
            }
            else if (pos == 13)
            {
                pos = pos - 1;
                lcd_put_cur(0, 0);
                delay(1);

                continue;
            }
            else if (pos == 14)
            {
                pos = pos - 1;
                lcd_put_cur(1, 0);
                delay(1);

                continue;
            }
            else if (pos == 15)
            {
                pos = pos - 1;
                lcd_put_cur(2, 0);
                delay(1);

                continue;
            }
            else if (pos == 16)
            {
                pos = pos - 1;
                pageFour();
                delay(1);

                continue;
            }
            else if (pos == 17)
            {
                pos = pos - 1;
                lcd_put_cur(0, 0);
                delay(1);

                continue;
            }
            else if (pos == 18)
            {
                pos = pos - 1;
                lcd_put_cur(1, 0);
                delay(1);

                continue;
            }
            else if (pos == 19)
            {
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
                pos = pos + 1;
                lcd_put_cur(1, 0);
                delay(1);
                continue;
            }
            else if (pos == 1)
            {
                pos = pos + 1;
                lcd_put_cur(2, 0);
                delay(1);
                continue;
            }
            else if (pos == 2)
            {
                pos = pos + 1;
                lcd_put_cur(3, 0);
                delay(1);
                continue;
            }
            else if (pos == 3)
            {
                pos = pos + 1;
                pageTwo();
                delay(1);
                continue;
            }
            else if (pos == 4)
            {
                pos = pos + 1;
                lcd_put_cur(1, 0);
                delay(1);
                continue;
            }
            else if (pos == 5)
            {
                pos = pos + 1;
                lcd_put_cur(2, 0);
                delay(1);
                continue;
            }
            else if (pos == 6)
            {
                pos = pos + 1;
                lcd_put_cur(3, 0);
                delay(1);
                continue;
            }
            else if (pos == 7)
            {
                pos = pos + 1;
                pageThree();
                delay(1);
                continue;
            }
            else if (pos == 8)
            {
                pos = pos + 1;
                lcd_put_cur(1, 0);
                delay(1);
                continue;
            }
            else if (pos == 9)
            {
                pos = pos + 1;
                lcd_put_cur(2, 0);
                delay(1);
                continue;
            }
            else if (pos == 10)
            {
                pos = pos + 1;
                lcd_put_cur(3, 0);
                delay(1);
                continue;
            }
            else if (pos == 11)
            {
                pos = pos + 1;
                pageFour();
                delay(1);
                continue;
            }
            else if (pos == 12)
            {
                pos = pos + 1;
                lcd_put_cur(1, 0);
                delay(1);
                continue;
            }
            else if (pos == 13)
            {
                pos = pos + 1;
                lcd_put_cur(2, 0);
                delay(1);
                continue;
            }
            else if (pos == 14)
            {
                pos = pos + 1;
                lcd_put_cur(3, 0);
                delay(1);
                continue;
            }
            else if (pos == 15)
            {
                pos = pos + 1;
                pageFive();
                delay(1);
                continue;
            }
            else if (pos == 16)
            {
                pos = pos + 1;
                lcd_put_cur(1, 0);
                delay(1);
                continue;
            }
            else if (pos == 17)
            {
                pos = pos + 1;
                lcd_put_cur(2, 0);
                delay(1);
                continue;
            }
            else if (pos == 18)
            {
                pos = pos + 1;
                lcd_put_cur(3, 0);
                delay(1);
                continue;
            }
        }
        /*SELECT BUTTON*/
        if (SELECT_BUTTON_STATE == 1)
        {
            delay(1);
            lcd_blink_OFF();
            selectedDisplay();
            selected = false;
        }

        vTaskDelay(1);
    }
}

void addname(struct list **pp, char *t)
{
    struct list *p = malloc(sizeof(*p));
    strcpy(p->data, t);
    p->next = NULL;
    while (*pp)
    {
        pp = &(*pp)->next;
    }
    *pp = p;
}

void print(struct list *n)
{
    char comma = ',';
    char end = ']';
    // char search[14] = "Search_Again]";

    for (; n; n = n->next)
        if (n->next)
        {
            char data[20];
            strcpy(data, n->data);
            if (strstr(listnames, data) == NULL)
            {
                strncat(data, &comma, 1);
                strncat(listnames, data, 21);
            }
            if (listnames[strlen(listnames) - 1] != ',')
            {
                listnames[strlen(listnames) - 1] = ',';
            }
        }
        else
        {
            char data[20];
            strcpy(data, n->data);
            if (strstr(listnames, data) == NULL)
            {
                strncat(data, &comma, 1);
                strncat(listnames, data, 21);
            }
            // strncat(listnames,search,14);
            if (listnames[strlen(listnames) - 1] == ',')
            {
                listnames[strlen(listnames) - 1] = ']';
            }
            else
                strncat(listnames, &end, 1);
        }
    printf(listnames);
    printf("\n");
}

void freelist(struct list *head)
{
    struct list *tmp;
    while (head != NULL)
    {
        tmp = head;
        head = head->next;
        free(tmp);
    }
}

void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    char beginning = '[';
    // int i2c_master_port = I2C_NUM_0;

    switch (event)
    {

    /* when discovery state changed, this event comes */
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
    {
        if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED)
        {
            if (s_a2d_state == APP_AV_STATE_DISCOVERED)
            {
                s_a2d_state = APP_AV_STATE_CONNECTING;
                ESP_LOGI(BT_AV_TAG, "Device discovery stopped.");
                ESP_LOGI(BT_AV_TAG, "a2dp connecting to peer: %s", s_peer_bdname);
                // i2c_driver_delete(I2C_MASTER_NUM);
                // lcd_clear();

                /* connect source to peer device specificed by Bluetooth Device Address */
                esp_a2d_source_connect(s_peer_bda);
            }
            else
            {
                /* not discovered, continue to discover */
                ets_printf(listnames);

                manualNames(); // Search again function + adding manual receivers

                pageOne();

                /* Setting GPIO Pin Directions */
                gpio_set_direction(UP_BUTTON, GPIO_MODE_INPUT);     // pin12
                gpio_set_direction(DOWN_BUTTON, GPIO_MODE_INPUT);   // pin13
                gpio_set_direction(SELECT_BUTTON, GPIO_MODE_INPUT); // pin14

                buttonCode();

                ESP_LOGI(BT_AV_TAG, "Device discovery failed, continue to discover...");

                memset(listnames, 0, sizeof(listnames)); // Erase listnames, before next printing of names
                strncat(listnames, &beginning, 1);

                esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
            }
        }
        else if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED)
        {
            ESP_LOGI(BT_AV_TAG, "Discovery started.");
        }
        break;
    }

    /* when device discovered a result, this event comes */
    case ESP_BT_GAP_DISC_RES_EVT:
    {
        if (s_a2d_state == APP_AV_STATE_DISCOVERING)
        {

            char bda_str[18];
            uint32_t cod = 0;    /* class of device */
            int32_t rssi = -129; /* invalid value */
            uint8_t *eir = NULL;
            esp_bt_gap_dev_prop_t *p;

            /* handle the discovery results */
            ESP_LOGI(BT_AV_TAG, "Scanned device: %s", bda2str(param->disc_res.bda, bda_str, 18));
            for (int i = 0; i < param->disc_res.num_prop; i++)
            {
                p = param->disc_res.prop + i;
                switch (p->type)
                {
                case ESP_BT_GAP_DEV_PROP_COD:
                    cod = *(uint32_t *)(p->val);
                    ESP_LOGI(BT_AV_TAG, "--Class of Device: 0x%x", cod);
                    break;
                case ESP_BT_GAP_DEV_PROP_RSSI:
                    rssi = *(int8_t *)(p->val);
                    ESP_LOGI(BT_AV_TAG, "--RSSI: %d", rssi);
                    break;
                case ESP_BT_GAP_DEV_PROP_EIR:
                    eir = (uint8_t *)(p->val);
                    break;
                case ESP_BT_GAP_DEV_PROP_BDNAME:
                default:
                    break;
                }
            }

            /* search for device with MAJOR service class as "rendering" in COD */
            if (!esp_bt_gap_is_valid_cod(cod) ||
                !(esp_bt_gap_get_cod_srvc(cod) & ESP_BT_COD_SRVC_RENDERING))
            {
                break;
            }

            /* search for target device in its Extended Inqury Response */
            if (eir)
            {
                get_name_from_eir(eir, s_peer_bdname, NULL);

                // bool duplicate = valueinarray(s_peer_bdname,names); // find out if duplicate
                // if (duplicate == false) {                           // if new name, add into end of list
                char *bdname = ((char *)s_peer_bdname);
                printf(bdname);
                printf("\n");
                addname(&names, bdname);

                memset(listnames, 0, sizeof(listnames)); // Erase listnames, before next printing of names
                strncat(listnames, &beginning, 1);

                print(names); // print list and current name
                ESP_LOGI(BT_AV_TAG, "--Name of Device: %s", s_peer_bdname);
                if (strcmp((char *)s_peer_bdname, TARGET_DEVICE_NAME) == 0)
                {
                    ESP_LOGI(BT_AV_TAG, "Found a target device, address %s, name %s", bda_str, s_peer_bdname);
                    s_a2d_state = APP_AV_STATE_DISCOVERED;
                    memcpy(s_peer_bda, param->disc_res.bda, ESP_BD_ADDR_LEN);
                    ESP_LOGI(BT_AV_TAG, "Cancel device discovery ...");
                    freelist(names);
                    printf("Freed names\n");
                    printf("Cleared LCD\n");
                    i2c_driver_delete(i2c_master_port);
                    ESP_LOGI(TAG, "I2C unitialized successfully");
                    ESP_LOGI(TAG, "I2C unitialized successfully");
                    bt_i2s_driver_install();
                    printf("Installed i2s\n");
                    esp_bt_gap_cancel_discovery();
                }
            }
        }
        break;
    }

    /* when authentication completed, this event comes */
    case ESP_BT_GAP_AUTH_CMPL_EVT:
    {
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGI(BT_AV_TAG, "authentication success: %s", param->auth_cmpl.device_name);
            esp_log_buffer_hex(BT_AV_TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
        }
        else
        {
            ESP_LOGE(BT_AV_TAG, "authentication failed, status: %d", param->auth_cmpl.stat);
        }
        break;
    }
    /* when Legacy Pairing pin code requested, this event comes */
    case ESP_BT_GAP_PIN_REQ_EVT:
    {
        ESP_LOGI(BT_AV_TAG, "ESP_BT_GAP_PIN_REQ_EVT min_16_digit: %d", param->pin_req.min_16_digit);
        if (param->pin_req.min_16_digit)
        {
            ESP_LOGI(BT_AV_TAG, "Input pin code: 0000 0000 0000 0000");
            esp_bt_pin_code_t pin_code = {0};
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
        }
        else
        {
            ESP_LOGI(BT_AV_TAG, "Input pin code: 1234");
            esp_bt_pin_code_t pin_code;
            pin_code[0] = '1';
            pin_code[1] = '2';
            pin_code[2] = '3';
            pin_code[3] = '4';
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
        }
        break;
    }

#if (CONFIG_BT_SSP_ENABLED == true)
    /* when Security Simple Pairing user confirmation requested, this event comes */
    case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI(BT_AV_TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d", param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    /* when Security Simple Pairing passkey notified, this event comes */
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(BT_AV_TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey: %d", param->key_notif.passkey);
        break;
    /* when Security Simple Pairing passkey requested, this event comes */
    case ESP_BT_GAP_KEY_REQ_EVT:
        ESP_LOGI(BT_AV_TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
        break;
#endif

    /* when GAP mode changed, this event comes */
    case ESP_BT_GAP_MODE_CHG_EVT:
        ESP_LOGI(BT_AV_TAG, "ESP_BT_GAP_MODE_CHG_EVT mode: %d", param->mode_chg.mode);
        break;
    /* other */
    default:
    {
        ESP_LOGI(BT_AV_TAG, "event: %d", event);
        break;
    }
    }

    return;
}

static void bt_av_hdl_stack_evt(uint16_t event, void *p_param)
{
    ESP_LOGD(BT_AV_TAG, "%s event: %d", __func__, event);

    switch (event)
    {
    /* when stack up worked, this event comes */
    case BT_APP_STACK_UP_EVT:
    {
        char *dev_name = LOCAL_DEVICE_NAME;
        esp_bt_dev_set_device_name(dev_name);

        esp_bt_gap_register_callback(bt_app_gap_cb);

        esp_avrc_ct_init();
        esp_avrc_ct_register_callback(bt_app_rc_ct_cb);

        esp_avrc_rn_evt_cap_mask_t evt_set = {0};
        esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &evt_set, ESP_AVRC_RN_VOLUME_CHANGE);
        ESP_ERROR_CHECK(esp_avrc_tg_set_rn_evt_cap(&evt_set));

        esp_a2d_source_init();
        esp_a2d_register_callback(&bt_app_a2d_cb);
        esp_a2d_source_register_data_callback(bt_app_a2d_data_cb);

        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

        ESP_LOGI(BT_AV_TAG, "Starting device discovery...");
        s_a2d_state = APP_AV_STATE_DISCOVERING;
        esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);

        /* create and start heart beat timer */
        do
        {
            printf("Entering heart beat timer\n");
            int tmr_id = 0;
            s_tmr = xTimerCreate("connTmr", (10000 / portTICK_RATE_MS),
                                 pdTRUE, (void *)&tmr_id, bt_app_a2d_heart_beat);
            xTimerStart(s_tmr, portMAX_DELAY);
        } while (0);
        printf("Exiting heart beat timer\n");
        break;
    }
    /* other */
    default:
    {
        ESP_LOGE(BT_AV_TAG, "%s unhandled event: %d", __func__, event);
        break;
    }
    }
}

static void bt_app_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{
    printf("It is being called here\n");
    bt_app_work_dispatch(bt_app_av_sm_hdlr, event, param, sizeof(esp_a2d_cb_param_t), NULL);
}

/* generate some random noise to simulate source audio */
static int32_t bt_app_a2d_data_cb(uint8_t *data, int32_t len)
{
    if (data == NULL || len < 0)
    {
        return 0;
    }

    int *p_buf = (int *)data;
    size_t bytes_read;

    i2s_read(I2S_NUM_0, p_buf, (len >> 1), &bytes_read, portMAX_DELAY);

    return len;
}

static void bt_app_a2d_heart_beat(TimerHandle_t arg)
{
    printf("It is being called at the heart beat\n");
    bt_app_work_dispatch(bt_app_av_sm_hdlr, BT_APP_HEART_BEAT_EVT, NULL, 0, NULL);
}

static void bt_app_av_sm_hdlr(uint16_t event, void *param)
{
    ESP_LOGI(BT_AV_TAG, "%s state: %d, event: 0x%x", __func__, s_a2d_state, event);

    /* select handler according to different states. */
    switch (s_a2d_state)
    {
    case APP_AV_STATE_DISCOVERING:
    case APP_AV_STATE_DISCOVERED:
        printf("Discovered\n");
        break;
    case APP_AV_STATE_UNCONNECTED:
        printf("Unconnected\n");
        bt_app_av_state_unconnected_hdlr(event, param);
        break;
    case APP_AV_STATE_CONNECTING:
        printf("Connecting\n");
        bt_app_av_state_connecting_hdlr(event, param);
        break;
    case APP_AV_STATE_CONNECTED:
        printf("Connected");
        bt_app_av_state_connected_hdlr(event, param);
        break;
    case APP_AV_STATE_DISCONNECTING:
        printf("Disconnecting");
        bt_app_av_state_disconnecting_hdlr(event, param);
        break;
    default:
        ESP_LOGE(BT_AV_TAG, "%s invalid state: %d", __func__, s_a2d_state);
        break;
    }
}

static void bt_app_av_state_unconnected_hdlr(uint16_t event, void *param)
{
    /* handle the events of intrest in unconnected state */
    switch (event)
    {
    case ESP_A2D_CONNECTION_STATE_EVT:
    case ESP_A2D_AUDIO_STATE_EVT:
    case ESP_A2D_AUDIO_CFG_EVT:
    case ESP_A2D_MEDIA_CTRL_ACK_EVT:
        break;
    case BT_APP_HEART_BEAT_EVT:
    {
        uint8_t *bda = s_peer_bda;
        ESP_LOGI(BT_AV_TAG, "a2dp connecting to peer: %02x:%02x:%02x:%02x:%02x:%02x",
                 bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
        esp_a2d_source_connect(s_peer_bda);
        s_a2d_state = APP_AV_STATE_CONNECTING;
        s_connecting_intv = 0;
        break;
    }
    default:
    {
        ESP_LOGE(BT_AV_TAG, "%s unhandled event: %d", __func__, event);
        break;
    }
    }
}

static void bt_app_av_state_connecting_hdlr(uint16_t event, void *param)
{
    esp_a2d_cb_param_t *a2d = NULL;

    /* handle the events of intrest in connecting state */
    switch (event)
    {
    case ESP_A2D_CONNECTION_STATE_EVT:
    {
        a2d = (esp_a2d_cb_param_t *)(param);
        if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED)
        {
            ESP_LOGI(BT_AV_TAG, "a2dp connected");
            s_a2d_state = APP_AV_STATE_CONNECTED;
            s_media_state = APP_AV_MEDIA_STATE_IDLE;
            esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
            bt_i2s_driver_install();
            bt_i2s_task_start_up();
            i2s_adc_enable(I2S_NUM_0);
        }
        else if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED)
        {
            s_a2d_state = APP_AV_STATE_UNCONNECTED;
        }
        break;
    }
    case ESP_A2D_AUDIO_STATE_EVT:
    case ESP_A2D_AUDIO_CFG_EVT:
    case ESP_A2D_MEDIA_CTRL_ACK_EVT:
        break;
    case BT_APP_HEART_BEAT_EVT:
        /**
         * Switch state to APP_AV_STATE_UNCONNECTED
         * when connecting lasts more than 2 heart beat intervals.
         */
        if (++s_connecting_intv >= 2)
        {
            s_a2d_state = APP_AV_STATE_UNCONNECTED;
            s_connecting_intv = 0;
        }
        break;
    default:
        ESP_LOGE(BT_AV_TAG, "%s unhandled event: %d", __func__, event);
        break;
    }
}

static void bt_app_av_media_proc(uint16_t event, void *param)
{
    esp_a2d_cb_param_t *a2d = NULL;

    switch (s_media_state)
    {
    case APP_AV_MEDIA_STATE_IDLE:
    {
        if (event == BT_APP_HEART_BEAT_EVT)
        {
            ESP_LOGI(BT_AV_TAG, "a2dp media ready checking ...");
            esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_CHECK_SRC_RDY);
        }
        else if (event == ESP_A2D_MEDIA_CTRL_ACK_EVT)
        {
            a2d = (esp_a2d_cb_param_t *)(param);
            if (a2d->media_ctrl_stat.cmd == ESP_A2D_MEDIA_CTRL_CHECK_SRC_RDY &&
                a2d->media_ctrl_stat.status == ESP_A2D_MEDIA_CTRL_ACK_SUCCESS)
            {
                ESP_LOGI(BT_AV_TAG, "a2dp media ready, starting ...");
                esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_START);
                s_media_state = APP_AV_MEDIA_STATE_STARTING;
            }
        }
        break;
    }
    case APP_AV_MEDIA_STATE_STARTING:
    {
        if (event == ESP_A2D_MEDIA_CTRL_ACK_EVT)
        {
            a2d = (esp_a2d_cb_param_t *)(param);
            if (a2d->media_ctrl_stat.cmd == ESP_A2D_MEDIA_CTRL_START &&
                a2d->media_ctrl_stat.status == ESP_A2D_MEDIA_CTRL_ACK_SUCCESS)
            {
                ESP_LOGI(BT_AV_TAG, "a2dp media start successfully.");
                s_intv_cnt = 0;
                s_media_state = APP_AV_MEDIA_STATE_STARTED;
            }
            else
            {
                /* not started succesfully, transfer to idle state */
                ESP_LOGI(BT_AV_TAG, "a2dp media start failed.");
                s_media_state = APP_AV_MEDIA_STATE_IDLE;
            }
        }
        break;
    }
    case APP_AV_MEDIA_STATE_STARTED:
    {
        // if (event == BT_APP_HEART_BEAT_EVT) {
        //     /* stop media after 10 heart beat intervals */
        //     // if (++s_intv_cnt >= 10) {
        //     //     ESP_LOGI(BT_AV_TAG, "a2dp media stopping...");
        //     //     esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_STOP);
        //     //     s_media_state = APP_AV_MEDIA_STATE_STOPPING;
        //     //     s_intv_cnt = 0;
        //     // }
        // }
        break;
    }
    case APP_AV_MEDIA_STATE_STOPPING:
    {
        if (event == ESP_A2D_MEDIA_CTRL_ACK_EVT)
        {
            a2d = (esp_a2d_cb_param_t *)(param);
            if (a2d->media_ctrl_stat.cmd == ESP_A2D_MEDIA_CTRL_STOP &&
                a2d->media_ctrl_stat.status == ESP_A2D_MEDIA_CTRL_ACK_SUCCESS)
            {
                ESP_LOGI(BT_AV_TAG, "a2dp media stopped successfully, disconnecting...");
                s_media_state = APP_AV_MEDIA_STATE_IDLE;
                esp_a2d_source_disconnect(s_peer_bda);
                s_a2d_state = APP_AV_STATE_DISCONNECTING;
            }
            else
            {
                ESP_LOGI(BT_AV_TAG, "a2dp media stopping...");
                esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_STOP);
            }
        }
        break;
    }
    default:
    {
        break;
    }
    }
}

static void bt_app_av_state_connected_hdlr(uint16_t event, void *param)
{
    esp_a2d_cb_param_t *a2d = NULL;

    /* handle the events of intrest in connected state */
    switch (event)
    {
    case ESP_A2D_CONNECTION_STATE_EVT:
    {
        a2d = (esp_a2d_cb_param_t *)(param);
        if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED)
        {
            ESP_LOGI(BT_AV_TAG, "a2dp disconnected");
            s_a2d_state = APP_AV_STATE_UNCONNECTED;
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        }
        break;
    }
    case ESP_A2D_AUDIO_STATE_EVT:
    {
        a2d = (esp_a2d_cb_param_t *)(param);
        if (ESP_A2D_AUDIO_STATE_STARTED == a2d->audio_stat.state)
        {
            s_pkt_cnt = 0;
        }
        break;
    }
    case ESP_A2D_AUDIO_CFG_EVT:
        /* not suppposed to occur for A2DP source */
        break;
    case ESP_A2D_MEDIA_CTRL_ACK_EVT:
    case BT_APP_HEART_BEAT_EVT:
    {
        bt_app_av_media_proc(event, param);
        break;
    }
    default:
    {
        ESP_LOGE(BT_AV_TAG, "%s unhandled event: %d", __func__, event);
        break;
    }
    }
}

static void bt_app_av_state_disconnecting_hdlr(uint16_t event, void *param)
{
    esp_a2d_cb_param_t *a2d = NULL;

    /* handle the events of intrest in disconnecing state */
    switch (event)
    {
    case ESP_A2D_CONNECTION_STATE_EVT:
    {
        a2d = (esp_a2d_cb_param_t *)(param);
        if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED)
        {
            ESP_LOGI(BT_AV_TAG, "a2dp disconnected");
            s_a2d_state = APP_AV_STATE_UNCONNECTED;
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            i2s_adc_disable(I2S_NUM_0);
            bt_i2s_task_shut_down();
            bt_i2s_driver_uninstall();
        }
        break;
    }
    case ESP_A2D_AUDIO_STATE_EVT:
    case ESP_A2D_AUDIO_CFG_EVT:
    case ESP_A2D_MEDIA_CTRL_ACK_EVT:
    case BT_APP_HEART_BEAT_EVT:
        break;
    default:
    {
        ESP_LOGE(BT_AV_TAG, "%s unhandled event: %d", __func__, event);
        break;
    }
    }
}

static void bt_app_rc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param)
{
    switch (event)
    {
    case ESP_AVRC_CT_CONNECTION_STATE_EVT:
    case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT:
    case ESP_AVRC_CT_METADATA_RSP_EVT:
    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
    case ESP_AVRC_CT_REMOTE_FEATURES_EVT:
    case ESP_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT:
    case ESP_AVRC_CT_SET_ABSOLUTE_VOLUME_RSP_EVT:
    {
        bt_app_work_dispatch(bt_av_hdl_avrc_ct_evt, event, param, sizeof(esp_avrc_ct_cb_param_t), NULL);
        break;
    }
    default:
    {
        ESP_LOGE(BT_RC_CT_TAG, "Invalid AVRC event: %d", event);
        break;
    }
    }
}

static void bt_av_volume_changed(void)
{
    if (esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_TEST, &s_avrc_peer_rn_cap,
                                           ESP_AVRC_RN_VOLUME_CHANGE))
    {
        esp_avrc_ct_send_register_notification_cmd(APP_RC_CT_TL_RN_VOLUME_CHANGE, ESP_AVRC_RN_VOLUME_CHANGE, 0);
    }
}

void bt_av_notify_evt_handler(uint8_t event_id, esp_avrc_rn_param_t *event_parameter)
{
    switch (event_id)
    {
    /* when volume changed locally on target, this event comes */
    case ESP_AVRC_RN_VOLUME_CHANGE:
    {
        ESP_LOGI(BT_RC_CT_TAG, "Volume changed: %d", event_parameter->volume);
        ESP_LOGI(BT_RC_CT_TAG, "Set absolute volume: volume %d", event_parameter->volume + 5);
        esp_avrc_ct_send_set_absolute_volume_cmd(APP_RC_CT_TL_RN_VOLUME_CHANGE, event_parameter->volume + 5);
        bt_av_volume_changed();
        break;
    }
    /* other */
    default:
        break;
    }
}

/* AVRC controller event handler */
static void bt_av_hdl_avrc_ct_evt(uint16_t event, void *p_param)
{
    ESP_LOGD(BT_RC_CT_TAG, "%s evt %d", __func__, event);
    esp_avrc_ct_cb_param_t *rc = (esp_avrc_ct_cb_param_t *)(p_param);

    switch (event)
    {
    /* when connection state changed, this event comes */
    case ESP_AVRC_CT_CONNECTION_STATE_EVT:
    {
        uint8_t *bda = rc->conn_stat.remote_bda;
        ESP_LOGI(BT_RC_CT_TAG, "AVRC conn_state event: state %d, [%02x:%02x:%02x:%02x:%02x:%02x]",
                 rc->conn_stat.connected, bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);

        if (rc->conn_stat.connected)
        {
            esp_avrc_ct_send_get_rn_capabilities_cmd(APP_RC_CT_TL_GET_CAPS);
        }
        else
        {
            s_avrc_peer_rn_cap.bits = 0;
        }
        break;
    }
    /* when passthrough responsed, this event comes */
    case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT:
    {
        ESP_LOGI(BT_RC_CT_TAG, "AVRC passthrough response: key_code 0x%x, key_state %d", rc->psth_rsp.key_code, rc->psth_rsp.key_state);
        break;
    }
    /* when metadata responsed, this event comes */
    case ESP_AVRC_CT_METADATA_RSP_EVT:
    {
        ESP_LOGI(BT_RC_CT_TAG, "AVRC metadata response: attribute id 0x%x, %s", rc->meta_rsp.attr_id, rc->meta_rsp.attr_text);
        free(rc->meta_rsp.attr_text);
        break;
    }
    /* when notification changed, this event comes */
    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
    {
        ESP_LOGI(BT_RC_CT_TAG, "AVRC event notification: %d", rc->change_ntf.event_id);
        bt_av_notify_evt_handler(rc->change_ntf.event_id, &rc->change_ntf.event_parameter);
        break;
    }
    /* when indicate feature of remote device, this event comes */
    case ESP_AVRC_CT_REMOTE_FEATURES_EVT:
    {
        ESP_LOGI(BT_RC_CT_TAG, "AVRC remote features %x, TG features %x", rc->rmt_feats.feat_mask, rc->rmt_feats.tg_feat_flag);
        break;
    }
    /* when get supported notification events capability of peer device, this event comes */
    case ESP_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT:
    {
        ESP_LOGI(BT_RC_CT_TAG, "remote rn_cap: count %d, bitmask 0x%x", rc->get_rn_caps_rsp.cap_count,
                 rc->get_rn_caps_rsp.evt_set.bits);
        s_avrc_peer_rn_cap.bits = rc->get_rn_caps_rsp.evt_set.bits;

        bt_av_volume_changed();
        break;
    }
    /* when set absolute volume responsed, this event comes */
    case ESP_AVRC_CT_SET_ABSOLUTE_VOLUME_RSP_EVT:
    {
        ESP_LOGI(BT_RC_CT_TAG, "Set absolute volume response: volume %d", rc->set_volume_rsp.volume);
        break;
    }
    /* other */
    default:
    {
        ESP_LOGE(BT_RC_CT_TAG, "%s unhandled event: %d", __func__, event);
        break;
    }
    }
}

/*********************************
 * MAIN ENTRY POINT
 ********************************/

void app_main(void)
{

    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

    lcd_init();
    welcomeDisplay();
    searchingDisplay();

    /* initialize NVS — it is used to store PHY calibration data */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /*
     * This example only uses the functions of Classical Bluetooth.
     * So release the controller memory for Bluetooth Low Energy.
     */
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if (esp_bt_controller_init(&bt_cfg) != ESP_OK)
    {
        ESP_LOGE(BT_AV_TAG, "%s initialize controller failed\n", __func__);
        return;
    }
    if (esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT) != ESP_OK)
    {
        ESP_LOGE(BT_AV_TAG, "%s enable controller failed\n", __func__);
        return;
    }
    if (esp_bluedroid_init() != ESP_OK)
    {
        ESP_LOGE(BT_AV_TAG, "%s initialize bluedroid failed\n", __func__);
        return;
    }
    if (esp_bluedroid_enable() != ESP_OK)
    {
        ESP_LOGE(BT_AV_TAG, "%s enable bluedroid failed\n", __func__);
        return;
    }

#if (CONFIG_BT_SSP_ENABLED == true)
    /* set default parameters for Secure Simple Pairing */
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
#endif

    /*
     * Set default parameters for Legacy Pairing
     * Use variable pin, input pin code when pairing
     */
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
    esp_bt_pin_code_t pin_code;
    esp_bt_gap_set_pin(pin_type, 0, pin_code);

    bt_app_task_start_up();
    /* Bluetooth device name, connection mode and profile set up */
    bt_app_work_dispatch(bt_av_hdl_stack_evt, BT_APP_STACK_UP_EVT, NULL, 0, NULL);
}