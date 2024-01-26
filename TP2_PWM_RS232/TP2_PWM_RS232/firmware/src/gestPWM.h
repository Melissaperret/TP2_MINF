#ifndef GestPWM_H
#define GestPWM_H
/*--------------------------------------------------------*/
// GestPWM.h
/*--------------------------------------------------------*/
//	Description :	Gestion des PWM 
//			        pour TP1 2016-2017
//
//	Auteur 		: 	C. HUBER
//
//	Version		:	V1.1
//	Compilateur	:	XC32 V1.42 + Harmony 1.08
//
/*--------------------------------------------------------*/

#include <stdint.h>



/*--------------------------------------------------------*/
// Définition des fonctions prototypes
/*--------------------------------------------------------*/


typedef struct {
    uint8_t absSpeed;    //Vitesse 0 à 99
    uint8_t absAngle;    //Angle  0 à 180
    int8_t SpeedSetting; //Consigne vitesse -99 à +99
    int8_t AngleSetting; //Consigne angle  -90 à +90
} S_pwmSettings;

extern S_pwmSettings PWMData; 

extern uint8_t compteur; 

void GPWM_Initialize(S_pwmSettings *pData);
void GPWM_GetSettings(S_pwmSettings *pData);	//Obtention vitesse et angle
void GPWM_DispSettings(S_pwmSettings *pData, int remote);	//Affichage
void GPWM_ExecPWM(S_pwmSettings *pData);		//Execution PWM et gestion moteur.
void GPWM_ExecPWMSoft(S_pwmSettings *pData);	//Execution PWM software.


#endif
