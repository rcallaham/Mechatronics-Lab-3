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
 Timer_A_getEnabledInterruptStatusvol
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

static volatile uint8_t readdata;

//Have we debounced the switch? 0 if no, 1 if yes.
volatile uint8_t debounced = 0;

//Is the stopwatch running? 1 if yes, 0 if no.
volatile uint8_t timerRunning = 1;

//Char array used to store the 8bit color values for the RGB LED
volatile char nums[3];

//Tracks the length of nums[]
volatile uint8_t numsLen = 0;

//Have we received a r, g, or b recently? 0 if false, 1 if true.
volatile uint8_t rgbState = 0;

//Which LED should we modify? R, G, or B?
volatile uint32_t color = 0;

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
const Timer_A_UpModeConfig A0_PWM = {
TIMER_A_CLOCKSOURCE_SMCLK, // Usually DCO clock, which in this case we set to 12MHz in main()
        TIMER_A_CLOCKSOURCE_DIVIDER_1, 255, //based on example slides. equal to CCR0
        TIMER_A_TAIE_INTERRUPT_DISABLE, // Should Timer_A send interrupts to Processor *at all*
        TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE, // Should Timer_A reaching its period value (stored in CCR0) trigger an interrupt?
        TIMER_A_DO_CLEAR };

// All 3 CCR configs for the 3 pins of the RGB LED

//CCR1

const Timer_A_CompareModeConfig a0_CCR1 = {
        TIMER_A_CAPTURECOMPARE_REGISTER_1,
        TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE,
        TIMER_A_OUTPUTMODE_SET_RESET, 0 };

//CR2
const Timer_A_CompareModeConfig a0_CCR2 = {
        TIMER_A_CAPTURECOMPARE_REGISTER_2,
        TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE,
        TIMER_A_OUTPUTMODE_SET_RESET, 0     //25% duty cycle
        };

//CCR3
const Timer_A_CompareModeConfig a0_CCR3 = {
        TIMER_A_CAPTURECOMPARE_REGISTER_3,
        TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE,
        TIMER_A_OUTPUTMODE_SET_RESET, 0 //75% duty cycle
        };

// PROVIDED: Timer_A struct for our debounce timer, triggering its interrupt when it hits its period value in CCR0
const Timer_A_UpModeConfig debounce_Config = {
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

    //Set input pins
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(
            GPIO_PORT_P1,
            GPIO_PIN2 | GPIO_PIN3,
            GPIO_PRIMARY_MODULE_FUNCTION);

    /* TODO: LAB3 Set DCO to 12MHz */
    CS_setDCOFrequency(12000000);

    //YEAH,WE DO NEED THIS BRENDYN ugh
    SysCtl_enableSRAMBankRetention(SYSCTL_SRAM_BANK1);

//    Generic Interrupt enabling:
    MAP_Interrupt_enableSleepOnIsrExit();
    MAP_Interrupt_enableMaster();

//      TODO: LAB3: UART module needs to be configured to both transmit and receive. How it reacts to specific commands is elsewhere
//        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ENTER CODE FOR LAB3 UART init HERE:
    MAP_UART_initModule(EUSCI_A0_BASE, &uartConfig);
    MAP_UART_enableModule(EUSCI_A0_BASE);
    MAP_UART_enableInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
    MAP_Interrupt_enableInterrupt(INT_EUSCIA0);
//        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ End CODE FOR LAB3 UART init

//      TODO: LAB3: Configure the Stopwatch (TIMER32_0) and button input (Port1) according to spec.
//           Configure Timer_A2 for debouncing, using its interrupt to prevent it from cycling forever.
//            (Struct for Timer A2 is provided, such that when it hits its CCR0 an interrupt is triggered
//          Start with the stopwatch paused, so it only starts actually counting in response to the relevant UART commands

    //Timer32 initialization and interrupt enabling
    MAP_Timer32_initModule(TIMER32_0_BASE, TIMER32_PRESCALER_1, TIMER32_32BIT,
    TIMER32_PERIODIC_MODE);
    MAP_Timer32_setCount(TIMER32_0_BASE, UINT32_MAX);
    MAP_Timer32_clearInterruptFlag(TIMER32_0_BASE);
    MAP_Timer32_enableInterrupt(TIMER32_0_BASE);
    MAP_Interrupt_enableInterrupt(TIMER32_0_INTERRUPT);
    MAP_Timer32_startTimer(TIMER32_0_BASE, 0);

    //Enable interrupts on GPIO port 1, pin 4
    MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P1,
    GPIO_PIN4);
    MAP_GPIO_clearInterruptFlag(GPIO_PORT_P1, GPIO_PIN4);
    MAP_GPIO_enableInterrupt(GPIO_PORT_P1, GPIO_PIN4);
    MAP_GPIO_interruptEdgeSelect(GPIO_PORT_P1, GPIO_PIN4,
    GPIO_HIGH_TO_LOW_TRANSITION);
    MAP_Interrupt_enableInterrupt(INT_PORT1);
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ENTER CODE FOR LAB3 stopwatch init HERE:

//           Configure Timer_A2 for debouncing, using its interrupt to prevent it from cycling forever.
//            (Struct for Timer A2 is provided, such that when it hits its CCR0 an interrupt is triggered
//          Start with the stopwatch paused, so it only starts actually counting in response to the relevant UART commands

    //        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ENTER CODE FOR LAB3 stopwatch init HERE:

    //Configure timer A2 and enable interrupt
    MAP_Timer_A_configureUpMode(TIMER_A2_BASE, &debounce_Config);
    MAP_Interrupt_enableInterrupt(INT_TA2_0);

//        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ End CODE FOR LAB3 stopwatch init

//          TODO: LAB3, Part 2: Prepare RGB LED driven by PWM signals driven by TIMER_A0 with multiple CCRs. Note that because the processor
//            doesn't care about when specifically the timer A0 is cycling, no interrupts from it or its CCRs are needed.
//          We can initially drive them with a 100% duty cycle for testing; the UART commands can easily change the duty cycle on their own
//        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ENTER CODE FOR LAB3 RC_RGB init HERE:
    // Initialize CCRS
    MAP_Timer_A_configureUpMode(TIMER_A0_BASE, &A0_PWM);
    MAP_Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);

    MAP_Timer_A_initCompare(TIMER_A0_BASE, &a0_CCR1);

    MAP_Timer_A_initCompare(TIMER_A0_BASE, &a0_CCR2);

    MAP_Timer_A_initCompare(TIMER_A0_BASE, &a0_CCR3);

//        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ End CODE FOR LAB3 RC_RGB init

//          PROVIDED: configure RGB pins (2.0, 2.1, 2.2) for outputting PWM function, mapped from Timer_A0's CCRs 1, 2, and 3:
    PMAP_configurePorts(portMapping, PMAP_P2MAP, 3,
    PMAP_DISABLE_RECONFIGURATION);
    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(
            GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2,
            GPIO_PRIMARY_MODULE_FUNCTION);

    while (1)
    {
        PCM_gotoLPM0InterruptSafe(); //Infinite loop to set to low power mode
    }
}

// Helper function to clarify UART command behavior
// TODO: Lab3: Resumes, Pauses, or Restarts stopwatch depending on restart flag and the stopwatch state
static void startStopStopwatch(bool restart)
{
    if (restart) //if ! is called, restart the timer
    {
        MAP_Timer32_setCount(TIMER32_0_BASE, 0); //resets timer
        timerRunning = 1;      //resets timer running variable
    }

    if (timerRunning == 1)    // is timer running
    {
        MAP_Timer32_haltTimer(TIMER32_0_BASE);   //stop timer
        timerRunning = 0;
    }
    else //timer is not running
    {
        MAP_Timer32_startTimer(TIMER32_0_BASE, 0); //start timer
        timerRunning = 1;
    }
    //timerRunning = !timerRunning; //toggle timer running state
}
// END Stopwatch State Helper Function Definition ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Helper function for sampling stopwatch. This avoids code duplication between UART and button sampling
// TODO: Lab3: Sample current stopwatch value, convert to number of elapsed milliseconds, and transmit to computer
void stopwatchSample()
{
    uint32_t watchTime = ((UINT32_MAX - Timer32_getValue(TIMER32_0_BASE))
            / 12000);
    printf(EUSCI_A0_BASE, "Current Time: %n milliseconds\r\n", watchTime);
}
// END Sample Stopwatch code ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// PROVIDED: Converts a string of numeric characters into their corresponding whole number.
uint16_t atoi(volatile char* s, uint8_t l)
{
    uint16_t r = 0;
    uint8_t i;
    for (i = 0; i < l; i++)
    {
        r = 10 * r + (s[i] - 48);
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

    if (status & EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG)
    {
        readdata = UART_receiveData(EUSCI_A0_BASE);
        MAP_UART_clearInterruptFlag(EUSCI_A0_BASE, status);

        //Handles the receipt of characters after an r, g, or b
        if (rgbState == 1)
        {
            //adds numeric characters to the nums array
            if (readdata >= '0' && readdata <= '9')
            {
                nums[numsLen] = readdata;
                numsLen++;
                return;
            }
            //after receiving a non numeric char, uses atoi to compute an int from values in
            //nums, clear nums, reset numsLen, resets rgb state, and changes the duty cycle
            //based on the color.
            else
            {
                uint8_t duty = atoi(nums, numsLen);

                nums[0] = 0;
                nums[1] = 0;
                nums[2] = 0;

                numsLen = 0;

                rgbState = 0;

                switch (color)
                {
                case 'r':
                {
                    MAP_Timer_A_setCompareValue(TIMER_A0_BASE,
                    TIMER_A_CAPTURECOMPARE_REGISTER_1,
                                                duty);

                    break;
                }
                case 'g':
                {
                    MAP_Timer_A_setCompareValue(TIMER_A0_BASE,
                    TIMER_A_CAPTURECOMPARE_REGISTER_2,
                                                duty);
                    break;
                }
                case 'b':
                {
                    MAP_Timer_A_setCompareValue(TIMER_A0_BASE,
                    TIMER_A_CAPTURECOMPARE_REGISTER_3,
                                                duty);
                    break;
                }
                default:
                {
                    return;
                }

                }
            }
        }

        //r
        if (readdata == 114)
        {
            rgbState = 1;
            color = 'r';
            return;
        }
        //g
        else if (readdata == 103)
        {
            rgbState = 1;
            color = 'g';
            return;
        }
        //b
        else if (readdata == 98)
        {
            rgbState = 1;
            color = 'b';
            return;
        }
        //s
        else if (readdata == 115)
        {
            startStopStopwatch(false);
        }
        //!
        else if (readdata == 33)
        {
            startStopStopwatch(true);
        }
        //p
        else if (readdata == 112)
        {
            stopwatchSample();
        }
        //echo undesired character
        else
        {
            MAP_UART_transmitData(EUSCI_A0_BASE, readdata);
            MAP_UART_transmitData(EUSCI_A0_BASE, '.');
        }

    }
}
//    END LAB3 UART COMMAND LOGIC ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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
    MAP_Timer_A_startCounter(TIMER_A2_BASE, TIMER_A_UP_MODE);

    //get pin
    uint32_t status;
    status = MAP_GPIO_getEnabledInterruptStatus(GPIO_PORT_P1);
    MAP_GPIO_clearInterruptFlag(GPIO_PORT_P1, GPIO_PIN4);

    // sample stopwatch
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
    MAP_Timer_A_clearInterruptFlag(TIMER_A2_BASE);
}

extern void T32_INT0_IRQHandler()
{
//      Stopwatch Interrupt: Complains about stopwatch count running out in the far, far future
    MAP_Timer32_setCount(TIMER32_0_BASE, UINT32_MAX);
    MAP_Timer32_startTimer(TIMER32_0_BASE, 0);

}

// END LAB3 BUTTON AND STOPWATCH INTERRUPTS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
