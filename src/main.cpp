#include <Arduino.h>
#include <M5StickCPlus.h>
#include "driver/i2c.h"

#define _I2C_NUMBER(num) I2C_NUM_##num
#define I2C_NUMBER(num) _I2C_NUMBER(num)

#define I2C_SLAVE_SCL_IO GPIO_NUM_33 /*!< gpio number for i2c slave clock */
#define I2C_SLAVE_SDA_IO GPIO_NUM_32 /*!< gpio number for i2c slave data */
#define I2C_SLAVE_NUM I2C_NUMBER(0) /*!< I2C port number for slave dev */

#define I2C_SLAVE_TX_BUF_LEN (2 * 512) /*!< I2C slave tx buffer size */
#define I2C_SLAVE_RX_BUF_LEN (2 * 512) /*!< I2C slave rx buffer size */

uint8_t deui[8];
bool debugMode = false;
bool firstTime = true;
i2c_port_t i2c_slave_port = I2C_SLAVE_NUM;
/**
 * @brief i2c slave initialization
 */
static esp_err_t i2c_slave_init(void)
{

    i2c_config_t conf_slave;
    conf_slave.mode = I2C_MODE_SLAVE;
    conf_slave.sda_io_num = I2C_SLAVE_SDA_IO;
    conf_slave.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf_slave.scl_io_num = I2C_SLAVE_SCL_IO;
    conf_slave.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf_slave.slave.addr_10bit_en = 0;
    conf_slave.slave.slave_addr = 0x002c;
    esp_err_t err = i2c_param_config(i2c_slave_port, &conf_slave);
    if (err != ESP_OK) {
        return err;
    }
    return i2c_driver_install(i2c_slave_port, conf_slave.mode, I2C_SLAVE_RX_BUF_LEN, I2C_SLAVE_TX_BUF_LEN, 0);
}

// the setup routine runs once when M5StickC starts up
void setup()
{

    // initialize the M5StickC object
    M5.begin();
    M5.Lcd.setRotation(1);
    Serial.begin(750000);
    //M5.Lcd.println("Starting I2C");

    esp_err_t err = i2c_slave_init();

    if (err != ESP_OK) {
        M5.Lcd.println(esp_err_to_name(err));
    }
}
int64_t lastChange = 0;
// the loop routine runs over and over again forever
void loop()
{
    if (debugMode) {
        M5.Lcd.fillScreen(TFT_BLACK);
        M5.Lcd.fillRect(0, 0, 240, 45, TFT_DARKGREY);
        M5.Lcd.setCursor(0, 5);
        M5.Lcd.setTextFont(1);
        M5.Lcd.setTextSize(5);
        M5.Lcd.setTextColor(TFT_ORANGE);
        M5.Lcd.println("  Debug");
        M5.Lcd.setTextSize(2);
        M5.Lcd.setCursor(0, 53);
        M5.Lcd.setTextColor(TFT_GREENYELLOW);
        uint8_t receivedData = 0;
        uint8_t dataShown = 0;
        while (debugMode) {
            if (M5.BtnA.read() == 1) {
                if (esp_timer_get_time() - lastChange > 500 * 1000) {
                    debugMode = !debugMode;
                    Serial.println("DebugBtnPressed");
                    lastChange = esp_timer_get_time();
                }
            }
            uint8_t dataCount = i2c_slave_read_buffer(i2c_slave_port, &receivedData, 1, 100 / portTICK_PERIOD_MS);
            if (dataCount > 0) { //Some data has arrived
                Serial.print("Data ");
                Serial.printf("%02X   %02d\n", receivedData, dataShown);
                M5.Lcd.printf("%02X", receivedData);
                dataShown++;
                if (dataShown > 50) {
                    dataShown = 0;
                    M5.Lcd.fillScreen(TFT_BLACK);
                    M5.Lcd.fillRect(0, 0, 240, 45, TFT_DARKGREY);
                    M5.Lcd.setCursor(0, 5);
                    M5.Lcd.setTextFont(1);
                    M5.Lcd.setTextSize(5);
                    M5.Lcd.setTextColor(TFT_ORANGE);
                    M5.Lcd.println("  Debug");
                    M5.Lcd.setTextSize(2);
                    M5.Lcd.setCursor(0, 53);
                    M5.Lcd.setTextColor(TFT_GREENYELLOW);
                    M5.Lcd.printf("%02X", receivedData);
                }
            }
        }
    } else {
        M5.Lcd.fillScreen(TFT_BLACK);
        M5.Lcd.fillRect(0, 0, 240, 45, TFT_DARKGREY);
        M5.Lcd.setCursor(0, 5);
        M5.Lcd.setTextFont(1);
        M5.Lcd.setTextSize(5);
        M5.Lcd.setTextColor(TFT_ORANGE);
        M5.Lcd.println(" DevEUI");
        M5.Lcd.setTextSize(3);
        M5.Lcd.setCursor(0, 60);
        M5.Lcd.setTextColor(TFT_GREENYELLOW);
        if (firstTime) {
            M5.Lcd.printf("Waiting for");
            M5.Lcd.setCursor(0, 90);
            M5.Lcd.printf("  DevEUI...");
        } else {
            M5.Lcd.printf("%02X:%02X:%02X:%02X", deui[0], deui[1], deui[2], deui[3]);
            M5.Lcd.setCursor(0, 90);
            M5.Lcd.printf("  %02X:%02X:%02X:%02X", deui[4], deui[5], deui[6], deui[7]);
        }

        bool validData = false;
        uint8_t receivedData = 0;
        while (!validData) {
            if (M5.BtnA.read() == 1) {
                if (esp_timer_get_time() - lastChange > 500 * 1000) {
                    debugMode = !debugMode;
                    Serial.println("DebugBtnPressed");
                    lastChange = esp_timer_get_time();
                    validData = true;
                }
            }
            uint8_t dataCount = i2c_slave_read_buffer(i2c_slave_port, &receivedData, 1, 100 / portTICK_PERIOD_MS);
            if (dataCount > 0) { //Some data has arrived
                M5.Lcd.fillScreen(TFT_GREEN);
                Serial.print("Data Arrived ");
                Serial.printf("%02X\n", receivedData);
                if (receivedData == 0x10) { //register address
                    uint8_t dataRead = 0;
                    while (dataRead < 8) {
                        if (M5.BtnA.read() == 1) {
                            if (esp_timer_get_time() - lastChange > 500 * 1000) {
                                debugMode = !debugMode;
                                Serial.println("DebugBtnPressed");
                                lastChange = esp_timer_get_time();
                                validData = true;
                            }
                        }
                        dataCount = i2c_slave_read_buffer(i2c_slave_port, &receivedData, 1, 100 / portTICK_PERIOD_MS);
                        if (dataCount > 0) {
                            Serial.print("Data ");
                            Serial.printf("%02X\n", receivedData);
                            deui[dataRead] = receivedData;
                            dataRead++;
                        }
                    }
                    validData = true;
                    firstTime = false;
                }
            }
        }
    }
}