#include <Arduino.h>
#include <SPI.h>
#include "w25qxx.h"

SPISettings setting = SPISettings(1000000, MSBFIRST, SPI_MODE0);
struct w25q_flash flash;

// Sample Data
char data[10] = "forkbomb";

char sample_buf[270];

/* */
extern "C" {

    void spi_flash_transfer(void *in, void *out, unsigned size) {
        digitalWrite(15, 0);
        SPI.transferBytes((unsigned char *)out, (unsigned char *)in, size);
        digitalWrite(15, 1);
    }
    
    void spi_flash_delay(unsigned time) {
        delay(time);
    }
    
}
void setup() {
    Serial.begin(9600);
    SPI.begin();
    SPI.beginTransaction(setting);
    // Setup CS pin
    pinMode(15, OUTPUT);
    digitalWrite(15, 1);
    delay(2000);
    // Setup Chip
    w25q_mount(&flash, spi_flash_transfer, spi_flash_delay);
    Serial.println("");
    Serial.printf("Chip Model: %x\n", flash.model);
    Serial.printf("Chip Size: %u pages\n", flash.size);
    Serial.println("Erasing the first sector...");
    w25q_erase(&flash, 0, 4095);
    Serial.println("Done erasing the first sector, now reading");
    w25q_read(&flash, 0, sample_buf, 260);
    Serial.printf("Data: \n%s\n", &sample_buf[4]);
    Serial.println("Writing data to the first page");
    for (unsigned i = 0; i < 32; i++) {
        w25q_write(&flash, i * 8, data, 8);
    }
    Serial.println("Done writing, now reading data.");
    w25q_read(&flash, 0, sample_buf, 260);
    sample_buf[260] = '\0';
    Serial.printf("Data: \n%s\n", &sample_buf[4]);
}

void loop() {

}