
/*
STORE LOG STRINGS IN EEPROM
Improve Exercise 1 by adding a persistent log that stores messages to EEPROM. When the program starts it writes “Boot” to
the log and every time when state or LEDs is changed the state change message, as described in Exercise 1, is also written to the log.
The log must have the following properties:
Log starts from address 0 in the EEPROM.
First two kilobytes (2048 bytes) of EEPROM are used for the log.
Log is persistent, after power up writing to log continues from location where it left off last time.
The log is erased only when it is full or by user command.
Each log entry is reserved 64 bytes.
First entry is at address 0, second at address 64, third at address 128, etc.
Log can contain up to 32 entries.
A log entry consists of a string that contains maximum 61 characters, a terminating null character (zero) and two-byte CRC that is used to validate the integrity of the data.
A maximum length log entry uses all 64 bytes. A shorter entry will not use all reserved bytes. The string must contain at least one character.
When a log entry is written to the log, the string is written to the log including the terminating zero. Immediately after the terminating zero follows a 16-bit CRC, MSB first followed by LSB.
Entry is written to the first unused (invalid) location that is available.
If the log is full then the log is erased first and then entry is written to address 0.
User can read the content of the log by typing read and pressing enter.
Program starts reading and validating log entries starting from address zero. If a valid string is found it is printed and program reads string from the next location.
A string is valid if the first character is not zero, there is a zero in the string before index 62, and the string passes the CRC validation.
Printing stops when an invalid string is encountered or the end log are is reached.
User can erase the log by typing erase and pressing enter.
Erasing is done by writing a zero at the first byte of every log entry.*/

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

//define I2C pins
#define I2C0_SDA_PIN 16
#define I2C0_SCL_PIN 17

//define LED pins
#define LED1 22
#define LED2 21
#define LED3 20

//define Buttons
#define SW_0 9
#define SW_1 8
#define SW_2 7

//EEPROME address
#define DEVADDR 0x50
#define BAUDRATE 100000
#define I2C_MEMORY_SIZE 32768

#define ENTRY_SIZE 64
#define MAX_ENTRIES 32
#define STRLEN 62
#define FIRST_ADDRESS 0


typedef struct ledstate{
    uint8_t state;
    uint8_t not_state;
} ledstate;

void initializePins(void);
void write_to_eeprom(uint16_t memory_address, uint8_t *data, size_t length);
void read_from_eeprom(uint16_t memory_address, uint8_t *data_read, size_t length);
void set_led_state(ledstate *ls, uint8_t value);
bool led_state_is_valid(ledstate *ls);
bool pressed(uint button);
void printLastThreeBits(uint8_t byte);


//void write_to_eeprom(uint16_t memory_address, uint8_t value){
//    uint8_t  buf[3];
//    buf[0] = (uint8_t)(memory_address >>8); //high byte of memory address
//    buf[1] = (uint8_t)(memory_address); //low byte of memory address
//    buf[2] = value;
//    i2c_write_blocking(i2c0, DEVADDR, buf, 3, false);
//    sleep_ms(10);
//}
//
//
//void read_from_eeprom(uint16_t memory_address, uint8_t *value){
//    uint8_t  buf[2];
//    buf[0] = (uint8_t)(memory_address >>8); //high byte of memory address
//    buf[1] = (uint8_t)(memory_address); //low byte of memory address
////    uint8_t  value = 0;
//    i2c_write_blocking(i2c0, DEVADDR, buf, 2, true);
//    i2c_read_blocking(i2c0, DEVADDR, value, 1, false);
////    return value;
//}

int main() {

    initializePins();

    ledstate ls;
    uint8_t stored_led_state = 0;
    uint8_t stored_led_not_state = 0;
    printf("Boot\n");
    //turn middle led on when boot the system
    uint16_t led_state_address = I2C_MEMORY_SIZE-1; //highest address
    uint16_t led_not_state_address = I2C_MEMORY_SIZE-2;
    read_from_eeprom(led_state_address, &stored_led_state,1);
    read_from_eeprom(led_not_state_address, &stored_led_not_state,1);
    ls.state = stored_led_state;
    ls.not_state = stored_led_not_state;


    if(!led_state_is_valid(&ls)){
        set_led_state(&ls, 0x02); //middle led is 0x02 (binary : 00000010)
        write_to_eeprom(led_state_address, &ls.state,1);
        write_to_eeprom(led_not_state_address, &ls.not_state,1);
        gpio_put(LED2, ls.state & 0x02);
        sleep_ms(100);
    }else{
        gpio_put(LED3, ls.state & 0x01);
        gpio_put(LED2, ls.state & 0x02);
        gpio_put(LED1, ls.state & 0x04);
    }


    while(true){
        if(!pressed(SW_0)){
            ls.state ^= 0x01;
            printf("SW_0 is pressed\n");
            gpio_put(LED3, ls.state & 0x01); //update led light
            //Write new state to EEPROM
            set_led_state(&ls, ls.state);
            write_to_eeprom(led_state_address, &ls.state,1);
            write_to_eeprom(led_not_state_address, &ls.not_state,1);
//            sleep_ms(100);

            printf("Time since boot: %llu seconds \n", time_us_64()/1000000);
            printLastThreeBits(ls.state);

            while(!gpio_get(SW_0));
        }
        if(!pressed(SW_1)){
            ls.state ^= 0x02;
            printf("SW_1 is pressed\n");
            gpio_put(LED2, ls.state & 0x02);
            //Write new state to EEPROM
            set_led_state(&ls, ls.state);
            write_to_eeprom(led_state_address, &ls.state,1);
            write_to_eeprom(led_not_state_address, &ls.not_state,1);
//            sleep_ms(100);

//            printf("Time since boot: %lld seconds, LED state: 0x%02X\n", time_us_64() / 1000000, ls.state);
            printf("Time since boot: %llu seconds \n", time_us_64()/1000000);
            printLastThreeBits(ls.state);

            while(!gpio_get(SW_1));
        }
        if(!pressed(SW_2)){
            ls.state ^= 0x04;
            printf("SW_2 is pressed\n");
            gpio_put(LED1, ls.state & 0x04);

            //Write new state to EEPROM
            set_led_state(&ls, ls.state);
            write_to_eeprom(led_state_address, &ls.state,1);
            write_to_eeprom(led_not_state_address, &ls.not_state,1);
//            sleep_ms(100);
//            printf("Time since boot: %lld seconds, LED state: 0x%02X\n", time_us_64() / 1000000, ls.state);
            printf("Time since boot: %llu seconds \n", time_us_64()/1000000);
            printLastThreeBits(ls.state);
            while(!gpio_get(SW_2));
        }

    }
    return 0;
}
//
//
void write_to_eeprom(uint16_t memory_address, uint8_t *data, size_t length){
    uint8_t buf[2+ length];
    buf[0] = (uint8_t)(memory_address >> 8); //high byte of memory address
    buf[1] = (uint8_t)(memory_address); //low byte of memory address
    for(size_t i = 0; i<length; ++i){
        buf[i+2] = data[i];
//        printf("write: %d\n", buf[i+2]);
    }
    i2c_write_blocking(i2c0, DEVADDR, buf, length+2, false);
    sleep_ms(50);

}

void read_from_eeprom(uint16_t memory_address, uint8_t *data_read, size_t length){
    uint8_t buf[2+ length];
    buf[0] = (uint8_t)(memory_address >>8); //high byte of memory address
    buf[1] = (uint8_t)(memory_address); //low byte of memory address
    i2c_write_blocking(i2c0, DEVADDR, buf, 2, true);
    i2c_read_blocking(i2c0, DEVADDR, data_read, length, false);
}


void set_led_state(ledstate *ls, uint8_t value){
    ls->state = value;
    ls->not_state = ~value;
}

bool led_state_is_valid(ledstate *ls){
    return ls->state == (uint8_t) ~ls->not_state;
}

void printLastThreeBits(uint8_t byte) {
    uint8_t lastThreeBits = byte & 0x07;
    // Printing each bit
    int led_n = 1;
    for (int i = 2; i >= 0; i--) {
        printf("Led %d: %d  ", led_n, (lastThreeBits >> i) & 0x01);
        led_n++;
    }
    printf("\n");
}

bool pressed(uint button) {
    int press = 0;
    int release = 0;
    while (press < 3 && release < 3) {
        if (gpio_get(button)) {
            press++;
            release = 0;
        } else {
            release++;
            press = 0;
        }
        sleep_ms(10);
    }
    if (press > release) return true;
    else return false;
}

void initializePins(void){

    // Initialize chosen serial port
    stdio_init_all();

    //Initialize I2C
    i2c_init(i2c1, BAUDRATE);
    i2c_init(i2c0, BAUDRATE);

    gpio_set_function(I2C0_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL_PIN, GPIO_FUNC_I2C);

    //Initialize led
    gpio_init(LED1);
    gpio_set_dir(LED1, GPIO_OUT);
    gpio_init(LED2);
    gpio_set_dir(LED2, GPIO_OUT);
    gpio_init(LED3);
    gpio_set_dir(LED3, GPIO_OUT);

    //Initialize buttons
    gpio_set_function(SW_0, GPIO_FUNC_SIO);
    gpio_set_dir(SW_0, GPIO_IN);
    gpio_pull_up(SW_0);

    gpio_set_function(SW_1, GPIO_FUNC_SIO);
    gpio_set_dir(SW_1, GPIO_IN);
    gpio_pull_up(SW_1);

    gpio_set_function(SW_2, GPIO_FUNC_SIO);
    gpio_set_dir(SW_2, GPIO_IN);
    gpio_pull_up(SW_2);

}