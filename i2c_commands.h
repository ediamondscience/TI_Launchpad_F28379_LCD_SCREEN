#include "driverlib.h"
#include "device.h"
#include "board.h"
#include "c2000ware_libraries.h"

#define I2C_SLAVE_ADDR   0x27    // Change as needed
#define I2C_NUM_BYTES    1
// Define a timeout for robustness
#define I2C_TIMEOUT_COUNT   1000000 // Adjust as needed based on your clock and I2C speed

/// Initialize the I2C bus for pins 37/38 (I2CB)
void initI2C(void);

/// Send a byte to address 0x27
bool i2cSendByte(uint8_t data);
