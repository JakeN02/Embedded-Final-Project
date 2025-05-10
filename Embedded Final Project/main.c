/* --COPYRIGHT--,BSD
 * Copyright (c) 2020, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
//*****************************************************************************
//         GUI Composer Simple JSON Demo using MSP430
//
// Texas Instruments, Inc.
// ******************************************************************************

#include <msp430.h>
#include <math.h>

// Pin Assignment
#define PIN_HEAT_REQ     BIT2   // P1.2 : Call-For-Heat
#define PIN_LED_IGN      BIT0   // P2.0 : Ignitor LED
#define PIN_VALVE_PILOT  BIT5   // P2.5 : Pilot-Valve Solenoid
#define PIN_SERVO_PWM    BIT0   // P5.0 : PWM to Main Valve

#define PIN_LED_R        BIT0   // P6.0 : Red
#define PIN_LED_G        BIT1   // P6.1 : Green
#define PIN_LED_B        BIT2   // P6.2 : Blue

// Servo timing , 20 ms 
#define SERVO_PERIOD    (20000 - 1) // 50 Hz period 
#define SERVO_CLOSED_US 800         // 0.8 ms Pulse
#define SERVO_OPEN_US   3000        // 3.0 ms Pulse

typedef enum { IDLE, FIRING, READY } SystemState;

volatile unsigned char g_heatReq = 0;

// Declarations
static void setupGPIO(void);
static void setupServoPWM(void);
static void setStatusLED(SystemState s);
static void shutdownOutputs(void);
static void performIgnition(void);

// Main Loop
void main(void)
{
    WDTCTL = WDTPW | WDTHOLD; // Stop Watchdog

    setupGPIO();
    setupServoPWM();

    PM5CTL0 &= ~LOCKLPM5; // Unlock GPIO pins
    __enable_interrupt();

    while (1)
    {       // Should check setpoint ADC value
            // Should check thermister ADC value  
        if (g_heatReq)    //And Boiler Temp>Setpoint     
        {
            setStatusLED(FIRING);
            performIgnition();   
            setStatusLED(READY);
        }
        else      //Should be Boiler Temp>=Setpoint             
        {
            shutdownOutputs();
            setStatusLED(IDLE);
        }

        __delay_cycles(100000);
    }
}
  // Pin Mapping
static void setupGPIO(void)
{
    P1DIR &= ~PIN_HEAT_REQ;
    P1REN |=  PIN_HEAT_REQ;
    P1OUT |=  PIN_HEAT_REQ;
    P1IES &= ~PIN_HEAT_REQ;
    P1IFG &= ~PIN_HEAT_REQ;
    P1IE  |=  PIN_HEAT_REQ;
    P2DIR |= PIN_LED_IGN | PIN_VALVE_PILOT;
    P2OUT &= ~(PIN_LED_IGN | PIN_VALVE_PILOT);
    P6DIR |= PIN_LED_R | PIN_LED_G | PIN_LED_B;
    P6OUT &= ~(PIN_LED_R | PIN_LED_G | PIN_LED_B);
    P5DIR  |=  PIN_SERVO_PWM;
    P5SEL0 |=  PIN_SERVO_PWM;
    P5SEL1 &= ~PIN_SERVO_PWM;
}

static void setupServoPWM(void)
{
    TB2CCR0  = SERVO_PERIOD;                
    TB2CCTL1 = OUTMOD_7;                      
    TB2CCR1  = SERVO_CLOSED_US;               
    TB2CTL   = TBSSEL__SMCLK | MC__UP | TBCLR;
}

static void setStatusLED(SystemState s)
{
    switch (s)
    {
        case IDLE:
            P6OUT = (P6OUT & ~(PIN_LED_R | PIN_LED_G)) | PIN_LED_B;
            break;
        case FIRING:
            P6OUT = (P6OUT & ~(PIN_LED_G | PIN_LED_B)) | PIN_LED_R;
            break;
        case READY:
            P6OUT = (P6OUT & ~(PIN_LED_R | PIN_LED_B)) | PIN_LED_G;
            break;
    }
}

static void shutdownOutputs(void)
{
    P2OUT &= ~(PIN_LED_IGN | PIN_VALVE_PILOT); // Turn off Pilot Valve and LED
    TB2CCR1 = SERVO_CLOSED_US;                 // Close Main Valve
}

static void performIgnition(void)
{
    P2OUT |= PIN_VALVE_PILOT; // Open Pilot Valve

    // Blink ignitor LED for 2s
    for (unsigned char i = 0; i < 10; i++)
    {
        P2OUT ^= PIN_LED_IGN;
        __delay_cycles(100000);
    }

    TB2CCR1 = SERVO_OPEN_US; // Open Main Valve
}

// Interupt Command
#pragma vector = PORT1_VECTOR
__interrupt void PORT1_ISR(void)
{
    if (P1IFG & PIN_HEAT_REQ)
    {
        g_heatReq ^= 1;       
        P1IFG &= ~PIN_HEAT_REQ;
    }
}
