/*
This is the file with all of the LCD Drivers
*/


#include "lcd_commands.h"

const uint8_t LCD_I2C_ADDR = 0x27;
const bool BACKLIGHT_ON = true;
/// write something to the screen
// message: The command to send to the LCD screen
void LCD_Send_Byte(LCD_Message message, bool write2) {
    uint8_t firstByte = (message.data & 0xF0) | (message.read_write << 1) | (message.register_select) | BACKLIGHT_ON << 3;
    LCD_pulse_value(firstByte);
    if (write2) {
        uint8_t lastByte = ((message.data & 0xF) << 4) | (message.read_write << 1) | (message.register_select) | BACKLIGHT_ON << 3;
        LCD_pulse_value(lastByte);
    }
}

void LCD_pulse_value(uint8_t message) {
    const uint8_t enableBit = 0x4; // this bit is hooked to the lcd enable
    i2cSendByte(message); // we're initializing the value before we send the enable pulse
    DEVICE_DELAY_US(50);
    i2cSendByte(message | enableBit); // pulse enable
    DEVICE_DELAY_US(50); // enable must be high > 450ns per datasheet
    i2cSendByte(message & ~enableBit); // pulse enable off
    DEVICE_DELAY_US(50); // Messages take 37us to process, wait a little extra
}

void LCD_Initialize() {
    // sequence copied from the data sheet, pg 48
    // https://www.farnell.com/datasheets/50586.pdf

    DEVICE_DELAY_US(50000); // wait at least 50ms for LCD to finish powering
    const LCD_Message initMsg = {0, 0, 0x00}; // initial message just to pull rs/rw low
    LCD_Send_Byte(initMsg, false);
    DEVICE_DELAY_US(1000000); // now wait a full second

    // this should reset the function set to the 8 bit control
    const LCD_Message functionSet1 = {0, 0, 0x30};
    LCD_Send_Byte(functionSet1, false); // send first reset signal
    DEVICE_DELAY_US(4500); // wait 4500 us
    LCD_Send_Byte(functionSet1, false); // send second reset signal
    DEVICE_DELAY_US(4500); // wait another 4500 us
    LCD_Send_Byte(functionSet1, false); // send third reset signal
    DEVICE_DELAY_US(150); // wait another 150 us

    // now we can set into 4 bit mode for the I2C expander chip
    const LCD_Message functionSet2 = {0, 0, 0x20};
    LCD_Send_Byte(functionSet2, false);


    // set up the number of lines in the display, repeat once
    const LCD_Message lineSetup = {0, 0, 0x28};
    LCD_Send_Byte(lineSetup, true);
    LCD_Send_Byte(lineSetup, true);

    const LCD_Message displaySet = {0, 0, 0x0F};
    LCD_Send_Byte(displaySet, true);

    const LCD_Message LCD_Clear = {0, 0, 0x01};
    LCD_Send_Byte(LCD_Clear, true);

    const LCD_Message LCD_Set_Increment = {0, 0, 0x06};
    LCD_Send_Byte(LCD_Set_Increment, true); // set the increment mode
}

void LCD_Send_Cursor_To_Line(uint8_t line) {
    uint8_t lineAddr;
    switch(line) {
        case 0:
            lineAddr = 0x00;
            break;
        case 1:
            lineAddr = 0x40;
            break;
        case 2:
            lineAddr = 0x14;
            break;
        case 3:
            lineAddr = 0x54;
            break;
        default:
            ESTOP0; // you fucked up
    }
    LCD_Message setAddr = {0, 0, 0x80 | lineAddr};
    LCD_Send_Byte(setAddr, true);
}

void LCD_Write_Message(const char* message, uint8_t length) {
    uint8_t counter;
    for(counter = 0; counter < length; counter++) {
        char write = *(message + counter);
        // make up a new LCD_Message, characters are just ascii
        LCD_Message writeChar = {1, 0, (uint8_t)write};
        LCD_Send_Byte(writeChar, true);
        DEVICE_DELAY_US(50);
    }
}

void LCD_Clear() {
    const LCD_Message clearScreen = {0, 0, 0x01};
    LCD_Send_Byte(clearScreen, true);
    DEVICE_DELAY_US(2000); // the clear command takes at least 2ms
}

void LCD_Return_Home() {
    const LCD_Message returnHome = {0, 0, 0x02};
    LCD_Send_Byte(returnHome, true);
    DEVICE_DELAY_US(2000); // so does the home!
}
