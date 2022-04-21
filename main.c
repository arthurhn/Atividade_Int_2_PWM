#include "inc/hw_ints.h"
#include "inc/hw_timer.h"
#include "inc/hw_gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.c"
#include "driverlib/sysctl.h"
#include "driverlib/Timer.c"
#include "driverlib/Timer.h"
#include "driverlib/gpio.c"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include <stdio.h>
#include <stdlib.h>

#define sw_1 GPIO_PIN_4
#define sw_2 GPIO_PIN_0
#define LOW 0x00
#define HIGH 0xFF

 //aprox 2ms em 80MHz
#define time 56666

//variaveis globais: dutyCycle, estado - cor do led e indices de cada led
uint32_t dutyCycle;
int estado = 0, i = 0, j = 0, k = 0;

//debouncer nesse timer
void Timer2IntHandler(void){
    GPIOIntEnable(GPIO_PORTF_BASE, sw_1|sw_2);
    TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);

    TimerDisable(TIMER2_BASE, TIMER_A);
}

//interrupcao: sw_1 - estado, sw_2 - manipulacao do dutyCycle
void IntPortalF (void){

    if(GPIOPinRead(GPIO_PORTF_BASE, sw_1) == LOW){
       estado++;
       if(estado == 3) estado = 0;
    }

    if ( GPIOPinRead(GPIO_PORTF_BASE, sw_2) ==  LOW)
    {
        if (estado == 0){
            i -= 0.25*dutyCycle;
            if ( i < 0) i = dutyCycle;
        }else if (estado == 1){
            j -= 0.25*dutyCycle;
            if ( j < 0) j = dutyCycle;
        }else if (estado == 2){
            k -= 0.25*dutyCycle;
            if ( k < 0) k = dutyCycle;
        }
    }

    GPIOIntClear(GPIO_PORTF_BASE, sw_1|sw_2);
    GPIOIntDisable(GPIO_PORTF_BASE, sw_1|sw_2);
    TimerEnable(TIMER2_BASE, TIMER_A);
}

int main(){
//80MHz
  SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
//setagens de interrupcao
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
  SysCtlDelay(3);

  HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
  HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
  HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;
  GPIODirModeSet(GPIO_PORTF_BASE, sw_1|sw_2, GPIO_DIR_MODE_IN);
  GPIOPadConfigSet(GPIO_PORTF_BASE, sw_1|sw_2, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

  GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_INT_PIN_0|GPIO_INT_PIN_4,GPIO_FALLING_EDGE);
  IntEnable(INT_GPIOF);
  GPIOIntEnable(GPIO_PORTF_BASE, GPIO_INT_PIN_0|GPIO_INT_PIN_4);
  IntMasterEnable();

//variaveis de dutyCycle
  uint32_t Period;
  Period = SysCtlClockGet()/100000 ;
  dutyCycle = Period-2;
  i = dutyCycle;
  j = dutyCycle;
  k = dutyCycle;

  /*
    Configura vermelho no timer 0B
    Configura azul no timer 1A
    Configura verde no timer 1B
  */

  GPIOPinConfigure(GPIO_PF1_T0CCP1);
  GPIOPinConfigure(GPIO_PF2_T1CCP0);
  GPIOPinConfigure(GPIO_PF3_T1CCP1);
  GPIOPinTypeTimer(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);

//faz as configuracoes do timers para pwm
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
  SysCtlDelay(3);
  TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR|TIMER_CFG_B_PWM);
  TimerLoadSet(TIMER0_BASE, TIMER_B, Period -1);
  TimerMatchSet(TIMER0_BASE, TIMER_B, dutyCycle);

  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
  SysCtlDelay(3);
  TimerConfigure(TIMER1_BASE, TIMER_CFG_SPLIT_PAIR|TIMER_CFG_A_PWM|TIMER_CFG_B_PWM);
  TimerLoadSet(TIMER1_BASE, TIMER_A, Period -1);
  TimerLoadSet(TIMER1_BASE, TIMER_B, Period -1);
  TimerMatchSet(TIMER1_BASE, TIMER_A, dutyCycle);
  TimerMatchSet(TIMER1_BASE, TIMER_B, dutyCycle);

//liga os timers de pwm
  TimerEnable(TIMER0_BASE, TIMER_B);
  TimerEnable(TIMER1_BASE, TIMER_A|TIMER_B);

//timer de debouncer
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
  TimerConfigure(TIMER2_BASE, TIMER_CFG_A_ONE_SHOT);
  TimerLoadSet(TIMER2_BASE, TIMER_A, (SysCtlClockGet() / 4));
  IntEnable(INT_TIMER2A);
  TimerIntEnable(TIMER2_BASE, TIMER_TIMA_TIMEOUT);

  while(1){
      if(estado == 0){      //led vermelho
          TimerMatchSet(TIMER0_BASE, TIMER_B, i);
          SysCtlDelay(time);
      }

      if(estado == 1){      //led azul
          TimerMatchSet(TIMER1_BASE, TIMER_A, j);
          SysCtlDelay(time);
      }

      if(estado == 2){      //led verde
          TimerMatchSet(TIMER1_BASE, TIMER_B, k);
          SysCtlDelay(time);
          }
      }

}
