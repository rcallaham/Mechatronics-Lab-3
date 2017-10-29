/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
/*
 * NEW FUNCTIONS or STRUCTS USED IN THE LAB3 KEY:
 CS_setDCOFrequency
 eUSCI_UART_Config
 EUSCIA0_IRQHandler
 GPIO_setAsPeripheralModuleFunctionInputPin
 Interrupt_enableSleepOnIsrExit
 PMAP_configurePorts
 printf  (from provided printf.h, NOT from stdio.h)
 Timer_A_clearCaptureCompareInterrupt
 Timer_A_clearInterruptFlag
 Timer_A_CompareModeConfig
 Timer_A_configureUpMode
 Timer_A_getCaptureCompareEnabledInterruptStatus
 Timer_A_getEnabledInterruptStatus
 Timer_A_initCompare
 Timer_A_setCompareValue
 Timer_A_startCounter
 Timer_A_UpModeConfig
 UART_clearInterruptFlag
 UART_enableInterrupt
 UART_enableModule
 UART_getEnabledInterruptStatus
 UART_initModule
 UART_receiveData
 Many functions from Lab2 are also used, of course.
 */

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>
#include "printf.h"

int MAX_DUTY_CYCLE = 255;
static volatile uint8_t readdata;
int debounceDelay = 3000;
int debounced = 0;

// TODO: Lab3: create UART, Timer_A, and CCR config structures ~~~~~~~~~~
// UART config
const eUSCI_UART_Config uartConfig = {
EUSCI_A_UART_CLOCKSOURCE_SMCLK,          // SMCLK Clock Source
        78,                                     // BRDIV = 78
        2,                                       // UCxBRF = 2
        0,                                       // UCxBRS = 0
        EUSCI_A_UART_NO_PARITY,                  // No Parity
        EUSCI_A_UART_LSB_FIRST,                  // LSB First
        EUSCI_A_UART_ONE_STOP_BIT,               // One stop bit
        EUSCI_A_UART_MODE,                       // UART mode
        EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION  // Oversampling
        };
// Timer_A for driving RGB LED via PWM
Timer_A_UpModeConfig A0_PWM =
{
     TIMER_A_CLOCKSOURCE_SMCLK, // Usually DCO clock, which in this case we set to 12MHz in main()
     TIMER_A_CLOCKSOURCE_DIVIDER_1,
     256, //10ms Debounce Delay
     TIMER_A_TAIE_INTERRUPT_ENABLE, // Should Timer_A send interrupts to Processor *at all*
     TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE, // Should Timer_A reaching its period value (stored in CCR0) trigger an interrupt?
     TIMER_A_DO_CLEAR
};

// All 3 CCR configs for the 3 pins of the RGB LED

//CCR1

Timer_A_CompareModeConfig A0_CCR0 =
{
 TIMER_A_CAPTURECOMPARE_REGISTER_1,
 TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABL,
 TIMER_A_OUTPUTMODE_OUTBITVALUE,
256  // 100% duty cycle
 };

//CR2
Timer_A_CompareModeConfig A0_CCR1 =
{
 TIMER_A_CAPTURECOMPARE_REGISTER_2,
 TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABL,
 Timer_A_OUTPUTMODE_OUTBITVALUE,
 127        //25% duty cycle
};

//CCR3
Timer_A_CompareModeConfig A0_CCR2 =
{
 TIMER_A_CAPTURECOMPARE_REGISTER_3,
 TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABL,
 Timer_A_OutputMODE_OUTBITVALUE,
 64     //75% duty cycle
};


// PROVIDED: Timer_A struct for our debounce timer, triggering its interrupt when it hits its period value in CCR0
Timer_A_UpModeConfig debounce_Config = {
TIMER_A_CLOCKSOURCE_SMCLK, // Usually DCO clock, which in this case we set to 12MHz in main()
        TIMER_A_CLOCKSOURCE_DIVIDER_1, 12 * 10 ^ 4, //10ms Debounce Delay
        TIMER_A_TAIE_INTERRUPT_ENABLE, // Should Timer_A send interrupts to Processor *at all*
        TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE, // Should Timer_A reaching its period value (stored in CCR0) trigger an interrupt?
        TIMER_A_DO_CLEAR };

// END CONFIG STRUCTS  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// PROVIDED: Port mapping to drive pins 2.0, 2.1, 2.0 (RGB LED) by Timer_A0's CCR1, CCR2, CCR3 respectively.
const uint8_t portMapping[] = {
// Will be used for Port 2; In order of pin number, 0-indexed
        PMAP_TA0CCR1A, PMAP_TA0CCR2A, PMAP_TA0CCR3A,
        PMAP_NONE,
        PMAP_NONE, PMAP_NONE,
        PMAP_NONE,
        PMAP_NONE };

int main(void)
{
    /* Halting WDT  */
    WDT_A_holdTimer();

    /* TODO: LAB3 Set DCO to 12MHz */
    CS_setDCOFrequency(12000000);

//    Generic Interrupt enabling:
    Interrupt_enableSleepOnIsrExit();
    Interrupt_enableMaster();

//      TODO: LAB3: UART module needs to be configured to both transmit and receive. How it reacts to specific commands is elsewhere
//        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ENTER CODE FOR LAB3 UART init HERE:

//        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ End CODE FOR LAB3 UART init

//      TODO: LAB3: Configure the Stopwatch (TIMER32_0) and button input (Port1) according to spec.
//           Configure Timer_A2 for debouncing, using its interrupt to prevent it from cycling forever.
//            (Struct for Timer A2 is provided, such that when it hits its CCR0 an interrupt is triggered
//          Start with the stopwatch paused, so it only starts actually counting in response to the relevant UART commands
     
 //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ENTER CODE FOR LAB3 stopwatch init HERE:
 
//           Configure Timer_A2 for debouncing, using its interrupt to prevent it from cycling forever.
//            (Struct for Timer A2 is provided, such that when it hits its CCR0 an interrupt is triggered
//          Start with the stopwatch paused, so it only starts actually counting in response to the relevant UART commands

 //        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ENTER CODE FOR LAB3 stopwatch init HERE:
    Timer32_initModule(TIMER32_1_BASE, TIMER32_PRESCALER_1, TIMER32_32BIT,
        TIMER32_PERIODIC_MODE);
        Timer32_clearInterruptFlag(TIMER32_1_BASE);
        Timer32_enableInterrupt(TIMER32_1_BASE);
        MAP_Interrupt_enableInterrupt(TIMER32_1_INTERRUPT);
//        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ End CODE FOR LAB3 stopwatch init

//          TODO: LAB3, Part 2: Prepare RGB LED driven by PWM signals driven by TIMER_A0 with multiple CCRs. Note that because the processor
//            doesn't care about when specifically the timer A0 is cycling, no interrupts from it or its CCRs are needed.
//          We can initially drive them with a 100% duty cycle for testing; the UART commands can easily change the duty cycle on their own
//        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ENTER CODE FOR LAB3 RC_RGB init HERE:
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0 + GPIO_PIN1 + GPIO_PIN2);

//        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ End CODE FOR LAB3 RC_RGB init

//          PROVIDED: configure RGB pins (2.0, 2.1, 2.2) for outputting PWM function, mapped from Timer_A0's CCRs 1, 2, and 3:
    PMAP_configurePorts(portMapping, PMAP_P2MAP, 3,
                        PMAP_DISABLE_RECONFIGURATION);
    GPIO_setAsPeripheralModuleFunctionOutputPin(
            GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2,
            GPIO_PRIMARY_MODULE_FUNCTION);

    while (1)
    {
        PCM_gotoLPM0InterruptSafe();
    }
}

// Helper function to clarify UART command behavior
// TODO: Lab3: Resumes, Pauses, or Restarts stopwatch depending on restart flag and the stopwatch state
static void startStopStopwatch(bool restart)
{

}
// END Stopwatch State Helper Function Definition ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Helper function for sampling stopwatch. This avoids code duplication between UART and button sampling
// TODO: Lab3: Sample current stopwatch value, convert to number of elapsed milliseconds, and transmit to computer
void stopwatchSample()
{
    uint32_t watchTime = Timer32_getValue(TIMER32_0_BASE);
    MAP_UART_transmitData(EUSCI_A0_BASE, (INT32_MAX - watchTime) / 3000000);
}
// END Sample Stopwatch code ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// PROVIDED: Converts a string of numeric characters into their corresponding whole number.
uint16_t atoi(volatile char* s, uint8_t l)
{
    uint16_t r = 0;
    uint8_t i;
    for (i = 0; i < l; i++)
    {
        r = 10 * r + (s[i] - '0');
    }
    return r;
}

///* EUSCI A0 UART ISR */
extern void EUSCIA0_IRQHandler(void)
{
// TODO: Lab3: UART Command Reception logic (reacts according to the specific received character)

//    also tracks parsing state (most recently seen letter, and numeric characters in current number) to understand
//          the relevant multicharacter commands, as per spec

     uint32_t status = MAP_UART_getEnabledInterruptStatus(EUSCI_A0_BASE);

    MAP_UART_clearInterruptFlag(EUSCI_A0_BASE, status);

    if (status & EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG)
    {
        readdata = MAP_UART_receiveData(EUSCI_A0_BASE);

        // Toggle Red LED if the character received is an "L":
        if (readdata == 76)
        {
            MAP_GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);

            const char mytext[] = "Toggle LED\n\r";
            int ichar;
            for (ichar = 0; ichar < 12; ichar++)
            {
                MAP_UART_transmitData(EUSCI_A0_BASE, mytext[ichar]);
            }
        }

        if (readdata == 'r' || readdata == 'g' || readdata == 'b')
        {
            uint8_t i;
            for (i = 0; i < 3; i++)
            {
                atoi(readdata, i);
            }
        }
        else if (readdata == 's')
        {
            startStopStopwatch(false);
        }
        else if (readdata == '!')
        {
            startStopStopwatch(true);
        }
        else if (readdata == 'p')
        {
            stopwatchSample();
        }
        else
        {
            MAP_UART_transmitData(EUSCI_A0_BASE, '.' + readdata + '.\n');
        }
    }
}
//    END LAB3 UART COMMAND LOGIC ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
}

// TODO: Lab3: Button Interrupt, Debounce Interrupt, and Stopwatch Interrupt ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
extern void PORT1_IRQHandler()
{
    //debouncing logic
    if (debounced == 1)
    {
        MAP_GPIO_clearInterruptFlag(GPIO_PORT_P1, GPIO_PIN4);
        return;
    }

    debounced = 1;
    TimerA_startTimer(TIMER_A0_BASE, 1);

    //get pin
    uint32_t status;
    status = MAP_GPIO_getEnabledInterruptStatus(GPIO_PORT_P1);
    MAP_GPIO_clearInterruptFlag(GPIO_PORT_P1, GPIO_PIN4);

    if (status & GPIO_PIN4)
    {
        stopwatchSample();
    }
}

extern void TA2_0_IRQHandler()
{
//      Debounce Interrupt: Clears our debouncing flag so we listen to future button interrupts again, and
//          prevents TA2 from cycling again like it normally would.
 
     //reset debounced to zero, allowing new button presses
    debounced = 0;
    // clear interrupt flag
    Timer_A_clearInterruptFlag(TIMER_A0_BASE);
}

extern void T32_INT0_IRQHandler()
{
    //      Stopwatch Interrupt: Complains about stopwatch count running out in the far, far future
}

// END LAB3 BUTTON AND STOPWATCH INTERRUPTS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
