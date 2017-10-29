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

// TODO: Lab3: create UART, Timer_A, and CCR config structures ~~~~~~~~~~
// UART config

// Timer_A for driving RGB LED via PWM 69

// All 3 CCR configs for the 3 pins of the RGB LED

// PROVIDED: Timer_A struct for our debounce timer, triggering its interrupt when it hits its period value in CCR0
Timer_A_UpModeConfig debounce_Config =
{
     TIMER_A_CLOCKSOURCE_SMCLK, // Usually DCO clock, which in this case we set to 12MHz in main()
     TIMER_A_CLOCKSOURCE_DIVIDER_1,
     12*10^4, //10ms Debounce Delay
     TIMER_A_TAIE_INTERRUPT_ENABLE, // Should Timer_A send interrupts to Processor *at all*
     TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE, // Should Timer_A reaching its period value (stored in CCR0) trigger an interrupt?
     TIMER_A_DO_CLEAR
};

// END CONFIG STRUCTS  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// PROVIDED: Port mapping to drive pins 2.0, 2.1, 2.0 (RGB LED) by Timer_A0's CCR1, CCR2, CCR3 respectively.
const uint8_t portMapping[] =
{
// Will be used for Port 2; In order of pin number, 0-indexed
        PMAP_TA0CCR1A,  PMAP_TA0CCR2A,  PMAP_TA0CCR3A,
        PMAP_NONE,      PMAP_NONE,      PMAP_NONE,
        PMAP_NONE,      PMAP_NONE
};


int main(void)
{
    /* Halting WDT  */
    WDT_A_holdTimer();

    /* TODO: LAB3 Set DCO to 12MHz */


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



//        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ End CODE FOR LAB3 RC_RGB init

//          PROVIDED: configure RGB pins (2.0, 2.1, 2.2) for outputting PWM function, mapped from Timer_A0's CCRs 1, 2, and 3:
            PMAP_configurePorts(portMapping, PMAP_P2MAP, 3, PMAP_DISABLE_RECONFIGURATION);
            GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2,
                    GPIO_PRIMARY_MODULE_FUNCTION);

    while(1)
    {
        PCM_gotoLPM0InterruptSafe();
    }
}

// Helper function to clarify UART command behavior
// TODO: Lab3: Resumes, Pauses, or Restarts stopwatch depending on restart flag and the stopwatch state
static void startStopStopwatch(bool restart){

}
// END Stopwatch State Helper Function Definition ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Helper function for sampling stopwatch. This avoids code duplication between UART and button sampling
// TODO: Lab3: Sample current stopwatch value, convert to number of elapsed milliseconds, and transmit to computer
void stopwatchSample(){

}
// END Sample Stopwatch code ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// PROVIDED: Converts a string of numeric characters into their corresponding whole number.
uint16_t atoi(volatile char* s, uint8_t l){
    uint16_t r = 0;
    uint8_t i;
    for (i = 0; i < l;i++){
        r = 10*r + (s[i]-'0');
    }
    return r;
}

///* EUSCI A0 UART ISR */
extern void EUSCIA0_IRQHandler(void)
{
// TODO: Lab3: UART Command Reception logic (reacts according to the specific received character)


//    also tracks parsing state (most recently seen letter, and numeric characters in current number) to understand
//          the relevant multicharacter commands, as per spec

//    END LAB3 UART COMMAND LOGIC ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
}

// TODO: Lab3: Button Interrupt, Debounce Interrupt, and Stopwatch Interrupt ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
extern void PORT1_IRQHandler(){
    //      Button 4: Samples current stopwatch value and sends it to computer (debounced)
    uint32_t button;
    button = MAP_GPIO_getEnabledInterruptStatus(GPIO_PORT_P1);
    MAP_GPIO_clearInterruptFlag(GPIO_PORT_P1, GPIO_PIN4);

    //only respond to button 1.4
    if(button & GPIO_PIN4)
    {
        // get value from timer
        uint32_t Elapsed_Time;
        Elapsed_Time = Timer32_getValue(uint32_t TIMER32_1_BASE);

        //send timer value to terminal (to do)


    }
}

extern void TA2_0_IRQHandler(){
//      Debounce Interrupt: Clears our debouncing flag so we listen to future button interrupts again, and
//          prevents TA2 from cycling again like it normally would.
}

extern void T32_INT0_IRQHandler(){
    //      Stopwatch Interrupt: Complains about stopwatch count running out in the far, far future
}

// END LAB3 BUTTON AND STOPWATCH INTERRUPTS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
