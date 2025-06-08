

#pragma once
#include "driverlib.h"
#include "device.h"
#include "board.h"
#include "i2c_commands.h"

// --- Global Variables for Interrupt-Driven I2C ---
volatile bool     g_i2cTransactionComplete = false;
volatile uint32_t g_i2cErrorStatus       = 0; // To capture NACK, ARB_LOST, etc.


// I2CB Interrupt Handler for master transmit operations
__interrupt void i2c_b_tx_isr(void)
{
    uint32_t status_flags_at_entry = I2C_getStatus(I2CB_BASE); // Capture current status
    uint32_t interrupt_triggered_flags = I2C_getInterruptStatus(I2CB_BASE); // Capture which interrupts triggered

    // --- CRITICAL: Clear interrupt sources and acknowledge PIE first ---
    // Clear the specific interrupt flags that triggered the ISR.
    I2C_clearInterruptStatus(I2CB_BASE, interrupt_triggered_flags);
    // Clear the corresponding I2C module status bits.
    I2C_clearStatus(I2CB_BASE, status_flags_at_entry); // Clear all status flags that were set when entering ISR
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP8); // Acknowledge PIE interrupt

    // 1. Handle Errors (NACK, Arbitration Lost) - Highest priority as they require immediate action.
    if (interrupt_triggered_flags & (I2C_INT_NO_ACK | I2C_INT_ARB_LOST))
    {
        if (interrupt_triggered_flags & I2C_INT_NO_ACK)
        {
            g_i2cErrorStatus |= I2C_STS_NO_ACK;
            // only send a stop signal when the no ack error is being handled
            // we wouldn't want to stop another thing writing to the bus.
            I2C_sendStopCondition(I2CB_BASE);
        }
        if (interrupt_triggered_flags & I2C_INT_ARB_LOST)
        {
            g_i2cErrorStatus |= I2C_STS_ARB_LOST;
        }
        g_i2cTransactionComplete = true; // Signal completion (with error)
    }
    // // 3. --- NEW PRIMARY SUCCESS SIGNAL: REG_ACCESS_RDY ---
    // // This interrupt fires when the last command (e.g., I2C_sendStopCondition)
    // // has been fully processed by the I2C module.
    // else if (interrupt_triggered_flags & I2C_INT_REG_ACCESS_RDY)
    // {
    //     // Only mark complete if no error has occurred.
    //     if (g_i2cErrorStatus == 0)
    //     {
    //         g_i2cTransactionComplete = true; // Signal successful completion after command processed
    //     }
    // }
    // 3. Handle STOP Condition Detected (Definitive transaction completion signal)
    // This is the final confirmation that the STOP was successfully sent and detected.
    else if (interrupt_triggered_flags & I2C_INT_STOP_CONDITION)
    {
        g_i2cTransactionComplete = true; // Signal successful completion
    }
}

// Initialize the I2C bus 
void initI2C(void)
{
    I2C_disableModule(I2CB_BASE);

    I2C_initMaster(I2CB_BASE, DEVICE_SYSCLK_FREQ, 100000, I2C_DUTYCYCLE_50);
    I2C_setBitCount(I2CB_BASE, I2C_BITCOUNT_8);
    I2C_setSlaveAddress(I2CB_BASE, I2C_SLAVE_ADDR);
    I2C_setEmulationMode(I2CB_BASE, I2C_EMULATION_FREE_RUN);

    // --- Enable I2C Interrupts ---
    I2C_enableInterrupt(I2CB_BASE, I2C_INT_NO_ACK |
                                   I2C_INT_ARB_LOST | I2C_INT_STOP_CONDITION|
                                   I2C_INT_REG_ACCESS_RDY);
    // --- Enable FIFO mode ---
    I2C_enableFIFO(I2CB_BASE);
    I2C_clearInterruptStatus(I2CB_BASE, I2C_INT_RXFF | I2C_INT_TXFF);

    // Register the ISR with the CPU's interrupt controller
    Interrupt_register(INT_I2CB, &i2c_b_tx_isr);

    // Enable I2CB TX interrupt in the PIE and CPU
    Interrupt_enable(INT_I2CB);
    EINT; // Enable Global Interrupts (if not already done elsewhere)
    ERTM; // Enable Real-Time Interrupt Mode (if using C2000Ware drivers)

    I2C_enableModule(I2CB_BASE); // Enable the I2C module
}

// This function initiates an I2C byte transfer and waits for its completion.
// Returns true on success, false on error.
bool i2cSendByte(uint8_t data)
{
    // now just wait until the send is complete
    uint32_t timeout = I2C_TIMEOUT_COUNT;

    while((I2C_getStopConditionStatus(I2CB_BASE) || I2C_isBusBusy(I2CB_BASE)) && timeout > 0){
        timeout--;
    }

    // set the global flags up to wait for at the while loop
    g_i2cTransactionComplete = false;
    g_i2cErrorStatus = 0;

    // this is setting up the config for the module to send out our byte
    I2C_setSlaveAddress(I2CB_BASE, I2C_SLAVE_ADDR);
    I2C_setDataCount(I2CB_BASE, 1);

    // Put the data into the transmit FIFO.
    I2C_putData(I2CB_BASE, data);
    I2C_setConfig(I2CB_BASE, I2C_MASTER_SEND_MODE);

    // calling the start and then the stop immediately is how we get this to send
    I2C_sendStartCondition(I2CB_BASE);
    I2C_sendStopCondition(I2CB_BASE);

    // now just wait until the send is complete
    timeout = I2C_TIMEOUT_COUNT;

    while(!g_i2cTransactionComplete && timeout > 0){
        timeout--;
    }

    // by the time we hit here the write should be done and there should be no errors
    return (g_i2cTransactionComplete && (g_i2cErrorStatus == 0));
}
