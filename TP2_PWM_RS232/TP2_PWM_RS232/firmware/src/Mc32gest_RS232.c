// Mc32Gest_RS232.C
// Canevas manipulatio TP2 RS232 SLO2 2017-18
// Fonctions d'�mission et de r�ception des message
// CHR 20.12.2016 ajout traitement int error
// CHR 22.12.2016 evolution des marquers observation int Usart
// SCA 03.01.2018 nettoy� r�ponse interrupt pour ne laisser que les 3 ifs

#include <xc.h>
#include <sys/attribs.h>
#include "system_definitions.h"
// Ajout CHR
#include <GenericTypeDefs.h>
#include "app.h"
#include "GesFifoTh32.h"
#include "Mc32gest_RS232.h"
#include "gestPWM.h"
#include "Mc32CalCrc16.h"


typedef union {
        uint16_t val;
        struct {uint8_t lsb;
                uint8_t msb;} shl;
} U_manip16;


// Definition pour les messages
#define MESS_SIZE  5
// avec int8_t besoin -86 au lieu de 0xAA
#define STX_code  (-86)

// Structure d�crivant le message
typedef struct {
    uint8_t Start;
    int8_t  Speed;
    int8_t  Angle;
    uint8_t MsbCrc;
    uint8_t LsbCrc;
} StruMess;

// Struct pour �mission des messages
StruMess TxMess;
// Struct pour r�ception des messages
StruMess RxMess;

// Declaration des FIFO pour r�ception et �mission
#define FIFO_RX_SIZE ( (4*MESS_SIZE) + 1)  // 4 messages
#define FIFO_TX_SIZE ( (4*MESS_SIZE) + 1)  // 4 messages

int8_t fifoRX[FIFO_RX_SIZE];
// Declaration du descripteur du FIFO de r�ception
S_fifo descrFifoRX;


int8_t fifoTX[FIFO_TX_SIZE];
// Declaration du descripteur du FIFO d'�mission
S_fifo descrFifoTX;


// Initialisation de la communication s�rielle
void InitFifoComm(void)
{    
    // Initialisation du fifo de r�ception
    InitFifo ( &descrFifoRX, FIFO_RX_SIZE, fifoRX, 0 );
    // Initialisation du fifo d'�mission
    InitFifo ( &descrFifoTX, FIFO_TX_SIZE, fifoTX, 0 );
    
    // Init RTS 
    RS232_RTS = 1;   // interdit �mission par l'autre
   
} // InitComm

 //-------------------------------------------
// Auteur: LGA, MPT
// Description: Check les valeurs sur la FIFO, test de valeurs correctes et 
//              sauvegarde des valeurs de vitesse et d'angle re�us dans pData
// Entr�es:- pointeur: S_pwmSettings: pData
// Sorties: entier 32 bits sign� 
// Valeur de retour 0  = pas de message re�u donc local (data non modifi�)
// Valeur de retour 1  = message re�u donc en remote (data mis � jour)
//-------------------------------------------
int GetMessage(S_pwmSettings *pData)
{
    // Declaration de variables locales
    static int8_t commStatus = 0;               
    uint8_t nbDonnesLues = 0, flagMessageOk = 0, compteurData = 0, i = 0;
    static uint8_t cntEssaies = 0;
    int8_t tabDatasRecus[5] = {0};
    U_manip16 crc16;
    uint16_t valCrc = 0;
    
    //Retourne la valeur de nombre de donn�es dans la FIFO 
    nbDonnesLues = GetReadSize(&descrFifoRX); 
    
    // Si nombre de valeurs (byte) dans FIFO est sup�rieur ou �gale � 5
    if(nbDonnesLues >= MESS_SIZE)
    {
        for(i = 0; i < 5; i++)
        {
            //Appel de la fonction qui r�cup�re 1 byte de la FIFO et 
            //sauvegarde dans le tableau tabDatasRecus
            commStatus = GetCharFromFifo(&descrFifoRX, &tabDatasRecus[i]); 
        }
      
        // Si le premier byte recu est 0xAA
        if(tabDatasRecus[0] == STX_code)
        {
            // Inc�rmentation de 1 � l'indice de position du tableau
            compteurData++;
            
            // Boucle de test pour valider les 5 valeurs recus 
            while(flagMessageOk == 0)
            {
                // Test anti message tronqu�, (test si trame coup�)
                if(tabDatasRecus[compteurData] != STX_code)
                {
                    // Si toutes les donnes recus ne sont pas 0xAA
                    if(compteurData > 4)
                    {
                        compteurData = 0;
                        
                        // Sauvegarde des values de CRC dans structure d'union 
                        // pour CRC
                        crc16.shl.msb = tabDatasRecus[3];
                        crc16.shl.lsb = tabDatasRecus[4];
                        
                        // Calcul de CRC
                        valCrc = 0xFFFF;
                        valCrc = updateCRC16(valCrc, tabDatasRecus[0]);
                        valCrc = updateCRC16(valCrc, tabDatasRecus[1]);
                        valCrc = updateCRC16(valCrc, tabDatasRecus[2]);
                        
                        // Si le CRC recu et le calcul� sont egaux
                        if(crc16.val == valCrc)
                        {
                            // Suavegarde valeurs de vitesse et angle sur pData
                            pData->SpeedSetting = tabDatasRecus[1];
                            if(tabDatasRecus[1] < 0)
                            {
                                pData->absSpeed = tabDatasRecus[1] * -1;
                            }
                            else
                            {
                                pData->absSpeed = tabDatasRecus[1];
                            }
                            pData->AngleSetting = tabDatasRecus[2];
                            pData->absAngle = tabDatasRecus[2] + 99;
                            commStatus = 1;
                            flagMessageOk = 1;
                            cntEssaies = 0;
                        }        
                        else
                        {
                            // Erreur de CRC
                            // Incremente de 1 compteur d'essaie
                            LED6_W = !LED6_R;
                            flagMessageOk = 1;
                            cntEssaies++;
                        }
                    }
                    else
                    {
                        
                        compteurData++;
                    }
                    
                }
                else
                {
                    // Erreur, message tronque� 
                    flagMessageOk = 1;
                    compteurData = 0;
                    cntEssaies++;
                }
            }
        }
        else
        {
            cntEssaies++;
        }
    }
    else
    {
        // Si le nombre de cycles avec des erreurs ou le nombre de valeurs dans FIFO
        // < 5
        if(cntEssaies < 10)
        {
            cntEssaies++;
        }
        else
        {
            //LED6_W = !LED6_R;
            commStatus = 0;
            cntEssaies = 0;
            compteurData = 0;
        }
    }
    
    // Gestion controle de flux de la r�ception
    if(GetWriteSpace ( &descrFifoRX) >= (2*MESS_SIZE)) 
    {
        // autorise �mission par l'autre
        RS232_RTS = 0;
    }
    return commStatus;
} // GetMessage


//-------------------------------------------
// Auteur: LGA, MPT
// Description: Fonction de check s'il y a des valeurs a envoyer, et construit structure pour envoi
// Entr�es: pointeur pData avec valeurs de Vitesse, angle
// Sorties: -
//-------------------------------------------
void SendMessage(S_pwmSettings *pData)
{
    // Variables locales
    int8_t freeSize;        // Variable de nombre de spaces vides dans FIFO
    U_manip16 valCrc16;     
    
    // Appel de fonction pour connaitre le space disponible dans FIFO
    freeSize = GetWriteSpace(&descrFifoTX);
    
    valCrc16.val = 0xFFFF;
    
    // Si espace disponible dans FIFO >= 5 (taille du message)
    if(freeSize >= MESS_SIZE)
    {
        // Preparation des donn�es a envoyer
        TxMess.Start = 0xAA;
        TxMess.Speed = pData->SpeedSetting;
        TxMess.Angle = pData->AngleSetting;
        valCrc16.val = updateCRC16(valCrc16.val, TxMess.Start);
        valCrc16.val = updateCRC16(valCrc16.val, TxMess.Speed);
        valCrc16.val = updateCRC16(valCrc16.val, TxMess.Angle);

        TxMess.MsbCrc = valCrc16.shl.msb; 
        TxMess.LsbCrc = valCrc16.shl.lsb; 

        // D�pose le message dans le fifo
        PutCharInFifo (&descrFifoTX, TxMess.Start);
        PutCharInFifo (&descrFifoTX, TxMess.Speed);
        PutCharInFifo (&descrFifoTX, TxMess.Angle);
        PutCharInFifo (&descrFifoTX, TxMess.MsbCrc);
        PutCharInFifo (&descrFifoTX, TxMess.LsbCrc);  
    }    
    // Gestion du controle de flux
    // si on a un caract�re � envoyer et que CTS = 0
    freeSize = GetReadSize(&descrFifoTX);
    if ((RS232_CTS == 0) && (freeSize > 0))
    {
        // Autorise int �mission    
        PLIB_INT_SourceEnable(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);                
    }
}


// Interruption USART1
// !!!!!!!!
// Attention ne pas oublier de supprimer la r�ponse g�n�r�e dans system_interrupt
// !!!!!!!!
 void __ISR(_UART_1_VECTOR, ipl5AUTO) _IntHandlerDrvUsartInstance0(void)
{
     // Declaration de variables locales
     uint8_t TxSize, freeSize;
     int8_t c;
     int8_t i_cts = 0;
     BOOL TxBuffFull;
     
    USART_ERROR UsartStatus;    

    // Marque d�but interruption avec Led3
    LED3_W = 1;
    
    // Is this an Error interrupt ?
    if ( PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_USART_1_ERROR) &&
                 PLIB_INT_SourceIsEnabled(INT_ID_0, INT_SOURCE_USART_1_ERROR) ) {
        /* Clear pending interrupt */
        PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_1_ERROR);
        // Traitement de l'erreur � la r�ception.
        while (PLIB_USART_ReceiverDataIsAvailable(USART_ID_1))
        {
            PLIB_USART_ReceiverByteReceive(USART_ID_1);
        }
    }

    // Is this an RX interrupt ?
    if ( PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_USART_1_RECEIVE) &&
                 PLIB_INT_SourceIsEnabled(INT_ID_0, INT_SOURCE_USART_1_RECEIVE) ) 
    {        
        // Oui Test si erreur parit� ou overrun
        UsartStatus = PLIB_USART_ErrorsGet(USART_ID_1);

        if ( (UsartStatus & (USART_ERROR_PARITY |
                             USART_ERROR_FRAMING | USART_ERROR_RECEIVER_OVERRUN)) == 0) 
        {
            // Traitement RX � faire ICI
            // Lecture des caract�res depuis le buffer HW -> fifo SW
            // (pour savoir s'il y a une data dans le buffer HW RX : PLIB_USART_ReceiverDataIsAvailable())
            while(PLIB_USART_ReceiverDataIsAvailable(USART_ID_1))
            {
                // (Lecture via fonction PLIB_USART_ReceiverByteReceive())
                c = PLIB_USART_ReceiverByteReceive(USART_ID_1);
                PutCharInFifo(&descrFifoRX, c); 
           }
                         
            LED4_W = !LED4_R; // Toggle Led4
            
            // buffer is empty, clear interrupt flag
            PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_1_RECEIVE);
        } 
        else 
        {
            // Suppression des erreurs
            // La lecture des erreurs les efface sauf pour overrun
            if ( (UsartStatus & USART_ERROR_RECEIVER_OVERRUN) == USART_ERROR_RECEIVER_OVERRUN) 
            {
                   PLIB_USART_ReceiverOverrunErrorClear(USART_ID_1);
            }
        }

        // Traitement controle de flux reception � faire ICI
        // Gerer sortie RS232_RTS en fonction de place dispo dans fifo reception
        // ...
        freeSize = GetWriteSpace(&descrFifoRX);
        
        if(freeSize <= 6)
        {
            RS232_RTS = 1;
        }

        
    } // end if RX

    
    // Is this an TX interrupt ?
    if ( PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT) && 
            PLIB_INT_SourceIsEnabled(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT) ) 
    {
        // Traitement TX � faire ICI
        TxSize = GetReadSize(&descrFifoTX);
        // Envoi des caract�res depuis le fifo SW -> buffer HW
            
        // Avant d'�mettre, on v�rifie 3 conditions :
        //  Si CTS = 0 autorisation d'�mettre (entr�e RS232_CTS)
        //  S'il y a un carat�res � �mettre dans le fifo
        i_cts = RS232_CTS;
        
        //  S'il y a de la place dans le buffer d'�mission (PLIB_USART_TransmitterBufferIsFull)
        TxBuffFull = PLIB_USART_TransmitterBufferIsFull(USART_ID_1);
        
        
        if((i_cts == 0) && (TxSize > 0) && (TxBuffFull == false))
        {
            do{
                GetCharFromFifo(&descrFifoTX, &c);
                PLIB_USART_TransmitterByteSend(USART_ID_1, c);
                i_cts = RS232_CTS;
                TxSize = GetReadSize(&descrFifoTX);
                TxBuffFull = PLIB_USART_TransmitterBufferIsFull(USART_ID_1);
                
            }while((i_cts == 0) && (TxSize > 0) && (TxBuffFull == false));
        }
        //   (envoi avec PLIB_USART_TransmitterByteSend())
	   
        
        LED5_W = !LED5_R; // Toggle Led5
		
        // disable TX interrupt (pour �viter une interrupt. inutile si plus rien � transmettre)
        PLIB_INT_SourceDisable(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);
        
        // Clear the TX interrupt Flag (Seulement apres TX) 
        PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);
    }
    // Marque fin interruption avec Led3
    LED3_W = 0;
 }




