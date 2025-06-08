#include "i2c_commands.h"

/// a struct for sending data to the MPU that runs the screen
typedef struct {
    // select a register
    bool register_select;
    // read or write to the mpu
    bool read_write;
    // data value. In cases where 2 messages are being sent, order is most significant nibble first
    uint8_t data;
} LCD_Message;

/// Initialize the LCD display to accept input via I2C
void LCD_Initialize();

// Write text to the LCD screen
// text: pointer to a char array with the letters to write to the screen
// length: uint8_t with the length of the char array
void LCD_Write_Message(const char* text, uint8_t length);

// Write a command to the LCD screen
// message: LCD_Message to write to the screen
// write2: write 2 messages to the piggy back board on the screen to send it whole byte
// order is most significant nibble, then least significant nibble
void LCD_Send_Byte(LCD_Message message, bool write2);

// Write to the I2C board and pulse the enable for the required time
// message: byte to send the I2C adapter. Will be pulsed with enable value
void LCD_pulse_value(uint8_t message);

// Clear the lcd screen
void LCD_Clear();

// Send the cursor to the specified line. Valid values 0-3
void LCD_Send_Cursor_To_Line(uint8_t line);

// return the cursor home
void LCD_Return_Home();
