/*******************************************************************************
  MPLAB Harmony Application Source File
  
  Company:
    Microchip Technology Inc.
  
  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It 
    implements the logic of the application's state machine and it may call 
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2013-2014 released Microchip Technology Inc.  All rights reserved.

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

#include "app.h"
#include "Mc32DriverLcd.h"
#include "Mc32gest_RS232.h"
#include "gestPWM.h"

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.
    
    Application strings and buffers are be defined outside this structure.
*/

APP_DATA appData;

//S_pwmSettings PWMData;      //Variable pour l'appel de la fonction GPWM_Initialize

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary callback functions.
*/

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


/* TODO:  Add any necessary local functions.
*/


// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;
    
    
    /* TODO: Initialize your application's state machine and other
     * parameters.
     */
}


/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Tasks ( void )
{
    // Declaration des variables locales
    static uint8_t commStatus  = 0;  // Variable d'etat de connexion
    static uint8_t cntCycles = 0; // Variable de compteur de cycles du programme
    
    /* Check the application's current state. */
    switch ( appData.state )
    {
        /* Application's initial state. */
        case APP_STATE_INIT:
        {
            GPWM_Initialize(&PWMData); 
            
            lcd_init();     //Initialiser le lcd
            lcd_bl_on();    // Allume backlight de LCD
            
            printf_lcd("Local Settings");
            lcd_gotoxy(1,2); 
            printf_lcd("TP2 PWM&RS232 23-24");
            lcd_gotoxy(1,3); 
            printf_lcd("Melissa Perret");
            lcd_gotoxy(1,4); 
            printf_lcd("Luis Garcia");
            
            BSP_InitADC10();    //Initialisation de l'ADC 
            LED_Off();          //Eteindre toutes les leds
            InitFifoComm();     // Initialisation de la FIFO

            APP_UpdateState(APP_STATE_WAIT); //Fait passer à l'état WAIT
            break;
        }

        case APP_STATE_WAIT:
        {
            break; //Ne rien faire / attendre
        }
        
        case APP_STATE_SERVICE_TASKS:
        { 
        // Appel de fonction GetMessage, retourne 1 ou 0 pour etat de connexion
            commStatus = GetMessage(&PWMData);
            
            // Si etat de connexion = 0 passe en local settings
            if(commStatus == LOCAL)
            {
                // Appel de fonction pour obtention de vitesse et angle
                // Affichage sur LCD, execution PWM et gestion du moteur
                GPWM_GetSettings(&PWMData);
                GPWM_DispSettings(&PWMData, commStatus);
                GPWM_ExecPWM(&PWMData);
            }
            else
            {
                // Connexion remote = affichage des valeurs de vitesse et angle
                // Execution PWM et gestion des moteurs
                GPWM_DispSettings(&PWMData, commStatus);
                GPWM_ExecPWM(&PWMData);
            }
            
            // Envoie des données par UART chaque 5 cycles du programme
            if(cntCycles > 4)
            {
                // Reset de compteur de cycles de programme
                cntCycles = 0;
                // Verification d'etat de connexion
                if(commStatus != LOCAL)
                {
                    // Obtention des parametres de vitesse et angle
                    GPWM_GetSettings(&PWMDataToSend);
                    // Appel de fonction pour sauvegarder les valeurs de 
                    // PWMDataToSend dans FIFO et envoi par UART
                    SendMessage(&PWMDataToSend);
                }
                else
                {
                    // Appel de fonction pour sauvegarder les valeurs de
                    // PWMData (Parametres locaux) dans FIFO en envoi par UART
                    SendMessage(&PWMData);
                }
            }
            else
            {
                // Incrementation de 1 de compteur de cycles du programme
                cntCycles++;
            }
            
            APP_UpdateState(APP_STATE_WAIT); //Va dans l'état d'attente
            break;
        }
        
        default:
        {
            break;
        }
    }
}

//-------------------------------------------
// Auteur: JAR, MPT
// Description: Fonction qui modifie l'état de la machine d'état 
// Entrées: APP_STATES newState
// Sorties: -
//-------------------------------------------
void APP_UpdateState(APP_STATES newState)
{
    appData.state = newState; //Mise à jour d'état
}

//-------------------------------------------
// Auteur: JAR, MPT
// Description: Fonction pour éteindre les leds
// Entrées: -
// Sorties: -
//-------------------------------------------
void LED_Off (void) 
{
    BSP_LEDOff(BSP_LED_0); 
    BSP_LEDOff(BSP_LED_1);
    BSP_LEDOff(BSP_LED_2);
    BSP_LEDOff(BSP_LED_3);
    BSP_LEDOff(BSP_LED_4);
    BSP_LEDOff(BSP_LED_5);
    BSP_LEDOff(BSP_LED_6);
    BSP_LEDOff(BSP_LED_7);
}
 

/*******************************************************************************
 End of File
 */
