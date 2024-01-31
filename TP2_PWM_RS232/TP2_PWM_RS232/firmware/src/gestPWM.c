/*--------------------------------------------------------*/
// GestPWM.c
/*--------------------------------------------------------*/
//	Description :	Gestion des PWM 
//			        pour TP1 2023-2024
//
//	Auteur 		: 	Perret & Garcia
//
//	Version		:	V1.1
//	Compilateur	:	XC32 V5.50 + Harmony 1.08
//
/*--------------------------------------------------------*/

#include "GestPWM.h"
#include "app.h"
#include "Mc32DriverAdc.h"
#include "Mc32DriverLcd.h"
#include "C:\microchip\harmony\v2_06\framework\peripheral\oc\processor\oc_p32mx795f512l.h"

S_pwmSettings PWMDataToSend, PWMData;

//-------------------------------------------
// Auteur: JAR, MPT
// Description: Cette fonction permet d'initialiser le pont en H, 
// les OCs et les timers. 
// Entrées:- pointeur: S_pwmSettings: pData
// Sorties: -
//-------------------------------------------
void GPWM_Initialize(S_pwmSettings *pData)
{ 
    //Initialisation état du pont en H
    BSP_EnableHbrige();
    
    //Démarrage des OCs
    DRV_OC0_Start();
    DRV_OC1_Start();

    //Démarrage des timers 1 à 3
    DRV_TMR0_Start();
    DRV_TMR1_Start();
    DRV_TMR2_Start();

}

//-------------------------------------------
// Auteur: JAR, MPT
// Description: Obtention vitesse et angle 
// (mise a jour des 4 champs de la structure)
// Entrées:- pointeur: S_pwmSettings: pData
// Sorties: -
//-------------------------------------------
void GPWM_GetSettings(S_pwmSettings *pData)	
{
    int16_t chan0Val0a198 = 0; //Récupération de la valeur de vitesse de 0 à 198
    static S_ADCResults tabValADC[10]; //Tableau de sauvegarde des valeurs des ADC
    static uint8_t i = 0; //Variable de positionnement pour le tableau 
    static uint8_t j = 0; //Variable de comptage pour la boucle for 
    float moyenneChan0; //Variable de sauvegarde de la moyenne du canal 0 de l'ADC
    float moyenneChan1; //Variable de sauvegarde de la moyenne du canal 1 de l'ADC

    //Lecture du convertisseur AD
    tabValADC[i] = BSP_ReadAllADC();
    
    //Remise à 0 des moyennes
    moyenneChan0 = 0;  
    moyenneChan1 = 0;
    
    //Calculs des moyennes 
    for(j = 0; j < 10 ; j++)
    {
        moyenneChan0+= tabValADC[j].Chan0;
        moyenneChan1 += tabValADC[j].Chan1;
    }
    
    moyenneChan0 = moyenneChan0 / 10; 
    moyenneChan1 = moyenneChan1 / 10; 
    
    //Calculs de la vitesse et vitesse absolue
    chan0Val0a198 = ((moyenneChan0*198)/1023); //1023 * X = 198 => X = 198/1023
    pData->SpeedSetting = chan0Val0a198 - 99; //Décalage de la vitesse pour qu'elle soit entre -99 et +99
    pData->absSpeed = abs(pData->SpeedSetting);
    
    //Calcul de l'angle 
    pData->absAngle = ((moyenneChan1*180)/1023); //1023 * X = 180 => X = 180/1023
    pData->AngleSetting = pData->absAngle - 90; //Décalage de l'angle pour qu'il soit entre -90 et +90
    
    //Incrémentation et remise à 0 de l'indice du tableau 
    i++;
    if(i >= 10)
    {
        i=0;
    }
}

//-------------------------------------------
// Auteur: JAR, MPT
// Description: Affichage des informations en exploitant la structure
// Entrées:- pointeur: S_pwmSettings: pData
// Sorties: -
//-------------------------------------------
void GPWM_DispSettings(S_pwmSettings *pData, int remote)
{ 
    static uint8_t i = 0; 
    //S_pwmSettings pDataTemp = 0;

    //Clear et affiche sur le LCD seulement une fois

    if(i == 0)
    {
        lcd_ClearLine(1);
        lcd_ClearLine(2);
        lcd_ClearLine(3);
        lcd_ClearLine(4);
        lcd_gotoxy(1,2); 
        printf_lcd("SpeedSetting");
        lcd_gotoxy(1,3); 
        printf_lcd("adbSpeed");
        lcd_gotoxy(1,4); 
        printf_lcd("Angle");
        i++;
    }

    if(remote == LOCAL)
    {
        lcd_gotoxy(1,1); //colonne 1, ligne 1 
        printf_lcd("Local Settings      ");
    }
    else
    {
        lcd_gotoxy(1,1); //colonne 1, ligne 1 
        printf_lcd("** Remote Settings");
    }

    
    //Affiche les valeurs de vitesse et d'angle sur le LCD
    lcd_gotoxy(14,2); 
    printf_lcd("%3d ", pData->SpeedSetting);
    lcd_gotoxy(15,3); 
    printf_lcd("%2d ", pData->absSpeed);
    lcd_gotoxy(14,4); 
    printf_lcd("%3d ", pData->AngleSetting);
}

//-------------------------------------------
// Auteur: JAR, MPT
// Description: Execution PWM et gestion moteur à partir des informations 
// dans la structure
// Entrées:- pointeur: S_pwmSettings: pData
// Sorties: -
//-------------------------------------------
void GPWM_ExecPWM(S_pwmSettings *pData)
{
    //Fait tourner le moteur dans le sens horaire 
    if(pData->SpeedSetting > 0)
    {
        AIN1_HBRIDGE_W = 1;
        AIN2_HBRIDGE_W = 0;
    }
    //Fait tourner le moteur dans le sens antihoraire 
    else if (pData->SpeedSetting < 0)
    {
        AIN1_HBRIDGE_W = 0;
        AIN2_HBRIDGE_W = 1;
    }
    //Fait que le moteur ne tourne pas
    else 
    {
        AIN1_HBRIDGE_W = 0;
        AIN2_HBRIDGE_W = 0;
    }
    
    //Calcul de la largueur d'impulsion pour le moteur DC
    PLIB_OC_PulseWidth16BitSet(OC_ID_2, (pData->absSpeed*1999)/100);
    //Calcul de la largeur d'impulsion pour le servomoteur 
    PLIB_OC_PulseWidth16BitSet(OC_ID_3, (((pData->absAngle*4500)/180)+1500));
}

//-------------------------------------------
// Auteur: JAR, MPT
// Description: Execution PWM software
// Entrées:- pointeur: S_pwmSettings: pData
// Sorties: -
//-------------------------------------------
void GPWM_ExecPWMSoft(S_pwmSettings *pData)
{
    if(compteur < pData->absSpeed)
    {
        LED2_W = 1;
    }
    else
    {
        LED2_W = 0;
    }
}


