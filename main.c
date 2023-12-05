//
//Implement a program that switches LEDs on and off and remembers the state of the LEDs across reboot and/or power off.
//The program should work as follows:
//When the program starts it reads the state of the LEDs from EEPROM. If no valid state is found in the EEPROM the
//  middle LED is switched on and the other two are switched off. The program must print number of seconds since power up
//  and the state of the LEDs to stdout. Use time_us_64() to get a timestamp and convert that to seconds.
//Each of the buttons SW0, SW1, and SW2 on the development board is associated with an LED. When user presses a button,
//  the corresponding LED toggles. Pressing and holding the button may not make the LED to blink or to toggle multiple
//  times. When state of the LEDs is changed the new state must be printed to stdout with a number of seconds since
// program was started.
//When the state of an LEDs changes the program stores the state of all LEDs in the EEPROM and prints the state to LEDs
//  to the debug UART. The program must employ a method to validate that settings read from the EEPROM are correct.
//The program must use the highest possible EEPROM address to store the LED state.

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

            printf("Time since boot: %llu seconds \n", time_us_64()/1000000);
            printLastThreeBits(ls.state);
            while(!gpio_get(SW_2));
        }

    }
    return 0;
}


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