/*******************************************************************************
 System Interrupts File

  File Name:
    system_interrupt.c

  Summary:
    Raw ISR definitions.

  Description:
    This file contains a definitions of the raw ISRs required to support the
    interrupt sub-system.

  Summary:
    This file contains source code for the interrupt vector functions in the
    system.

  Description:
    This file contains source code for the interrupt vector functions in the
    system.  It implements the system and part specific vector "stub" functions
    from which the individual "Tasks" functions are called for any modules
    executing interrupt-driven in the MPLAB Harmony system.

  Remarks:
    This file requires access to the systemObjects global data structure that
    contains the object handles to all MPLAB Harmony module objects executing
    interrupt-driven in the system.  These handles are passed into the individual
    module "Tasks" functions to identify the instance of the module to maintain.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2011-2014 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 *******************************************************************************/
// DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include "system/common/sys_common.h"
#include "app.h"
#include "C:\microchip\harmony\v2_06\apps\MINF\TP\TP2_MINF\TP2_PWM_RS232\TP2_PWM_RS232\firmware\src\gestPWM.h"
#include "system_definitions.h"
#include <stdint.h>


// *****************************************************************************
// *****************************************************************************
// Section: System Interrupt Vector Functions
// *****************************************************************************
// *****************************************************************************

//S_pwmSettings pData;  //Variable de sauvegarde de la vitesse et de l'angle

uint8_t compteur = 0; //Variable de comptage pour le PWMSoft 

void __ISR(_TIMER_1_VECTOR, ipl4AUTO) IntHandlerDrvTmrInstance0(void)
{
    LED0_W = 1;  //Sert � prendre les mesures, mais peut �tre retir� pour le code final  
    static uint8_t i = 0;

    //Attente de 3secondes pour l'initialisation 
    if(i<150) //20x150 = 3000ms
    {
        i++;
    }
    else
    {
        APP_UpdateState(APP_STATE_SERVICE_TASKS);
        LED0_W = 0; //Sert � prendre les mesures, mais peut �tre retir� pour le code final 
    }
    
    PLIB_INT_SourceFlagClear(INT_ID_0,INT_SOURCE_TIMER_1); //Clear le flag de l'interruption
}

void __ISR(_TIMER_2_VECTOR, ipl0AUTO) IntHandlerDrvTmrInstance1(void)
{
    PLIB_INT_SourceFlagClear(INT_ID_0,INT_SOURCE_TIMER_2); //Clear le flag de l'interruption
}

void __ISR(_TIMER_3_VECTOR, ipl0AUTO) IntHandlerDrvTmrInstance2(void)
{
    PLIB_INT_SourceFlagClear(INT_ID_0,INT_SOURCE_TIMER_3); //Clear le flag de l'interruption
}
//void __ISR(_TIMER_4_VECTOR, ipl7AUTO) IntHandlerDrvTmrInstance3(void)
//{
////    LED1_W = 1; //Sert � prendre les mesures, mais peut �tre retir� pour le code final 
////    
////    GPWM_ExecPWMSoft(&pData); //Appel de la fonction qui cr�e le PWMSoft 
////    
////    compteur++;
////    
////    //Reset du compteur pour le PWMsoft 
////    if(compteur >= 100) 
////    {
////        compteur = 0;
////    }
////    
////    LED1_W = 0; //Sert � prendre les mesures, mais peut �tre retir� pour le code final 
//}
/*******************************************************************************
 End of File
*/
