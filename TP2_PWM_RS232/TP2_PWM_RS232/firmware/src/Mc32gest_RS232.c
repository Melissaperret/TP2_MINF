// Mc32Gest_RS232.C
// Canevas manipulatio TP2 RS232 SLO2 2017-18
// Fonctions d'émission et de réception des message
// CHR 20.12.2016 ajout traitement int error
// CHR 22.12.2016 evolution des marquers observation int Usart
// SCA 03.01.2018 nettoyé réponse interrupt pour ne laisser que les 3 ifs

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
#include <stdint.h>

typedef union {
        uint16_t val;
        struct {uint8_t lsb;
                uint8_t msb;} shl;
} U_manip16;


// Definition pour les messages
#define MESS_SIZE  5
#define TAILLE_TABLEAU 3
// avec int8_t besoin -86 au lieu de 0xAA
#define STX_code  (-86)

// Structure décrivant le message
typedef struct {
    uint8_t Start;
    int8_t  Speed;
    int8_t  Angle;
    uint8_t MsbCrc;
    uint8_t LsbCrc;
} StruMess;



// Struct pour émission des messages
StruMess TxMess;
// Struct pour réception des messages
StruMess RxMess;

// Declaration des FIFO pour réception et émission
#define FIFO_RX_SIZE ( (4*MESS_SIZE) + 1)  // 4 messages
#define FIFO_TX_SIZE ( (4*MESS_SIZE) + 1)  // 4 messages

int8_t fifoRX[FIFO_RX_SIZE];
// Declaration du descripteur du FIFO de réception
S_fifo descrFifoRX;


int8_t fifoTX[FIFO_TX_SIZE];
// Declaration du descripteur du FIFO d'émission
S_fifo descrFifoTX;


// Initialisation de la communication sérielle
void InitFifoComm(void)
{    
    // Initialisation du fifo de réception
    InitFifo ( &descrFifoRX, FIFO_RX_SIZE, fifoRX, 0 );
    // Initialisation du fifo d'émission
    InitFifo ( &descrFifoTX, FIFO_TX_SIZE, fifoTX, 0 );
    
    // Init RTS 
    RS232_RTS = 1;   // interdit émission par l'autre
   
} // InitComm

 //-------------------------------------------
// Auteur: LGA, MPT
// Description: Check les valeurs sur la FIFO, test de valeurs correctes et 
//              sauvegarde des valeurs de vitesse et d'angle reçus dans pData
// Entrées:- pointeur: S_pwmSettings: pData
// Sorties: entier 32 bits signé 
// Valeur de retour 0  = pas de message reçu donc local (data non modifié)
// Valeur de retour 1  = message reçu donc en remote (data mis à jour)
//-------------------------------------------
int GetMessage(S_pwmSettings *pData)
{
    // Declaration de variables locales
    static int8_t commStatus = 0;               
    uint8_t nbDonnesLues = 0;
    uint8_t dataIndex = 0;
    static uint8_t cntEssaies = 0;
    int8_t tabDatasRecus[MESS_SIZE] = {0};
    U_manip16 crc16;
    uint16_t valCrc = 0;
    
    //Retourne la valeur de nombre de données dans la FIFO 
    nbDonnesLues = GetReadSize(&descrFifoRX); 
    
    // Si nombre de valeurs (byte) dans FIFO est supérieur ou égale à 5
    if(nbDonnesLues >= MESS_SIZE)
    {
        for(dataIndex = 0; dataIndex < MESS_SIZE; dataIndex ++)
        {
            //Appel de la fonction qui récupère 1 byte de la FIFO et 
            //sauvegarde dans le tableau tabDatasRecus
            commStatus = GetCharFromFifo(&descrFifoRX, &tabDatasRecus[dataIndex]); 
        }

        // Sauvegarde des valeurs de CRC dans la structure se trouvant dans
        // l'uniotn (pour CRC)
        crc16.shl.msb = RxMess.MsbCrc;
        crc16.shl.lsb = RxMess.LsbCrc;

        // Calcul de CRC
        valCrc = 0xFFFF;
        for(dataIndex = 0; dataIndex < TAILLE_TABLEAU; dataIndex ++)
        {
            valCrc = updateCRC16(valCrc, tabDatasRecus[dataIndex]);
        }

        // Si le CRC recu et le calcule sont egaux
        if(crc16.val == valCrc)
        {
            // Sauvegarde les valeurs de vitesse et angle sur pData
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
            cntEssaies = 0;
        }        
        else
        {
            // Erreur de CRC
            // Incremente de 1 compteur d'essai
            LED6_W = !LED6_R;
            cntEssaies++;
        }
    }
    else
    {
        // Si le nombre de cycles avec des erreurs ou le nombre de valeurs dans
        // la FIFO est inférieure à 5
        if(cntEssaies < 10)
        {
            cntEssaies++;
        }
        else
        {
            //LED6_W = !LED6_R;
            commStatus = 0;
            cntEssaies = 0;
        }
    }
    
    // Gestion controle de flux de la réception
    if(GetWriteSpace ( &descrFifoRX) >= (2*MESS_SIZE)) 
    {
        // autorise émission par l'autre
        RS232_RTS = 0;
    }
    return commStatus;
} // GetMessage

//-------------------------------------------
// Auteur: LGA, MPT
// Description: Regarde s'il y a des valeurs à envoyer, et construit une 
//              structure pour l'envoie des données 
// Entrées: Entrées:- pointeur: S_pwmSettings: pData
// Sorties: -
//-------------------------------------------
void SendMessage(S_pwmSettings *pData)
{
    // Variables locales
    int8_t freeSize;        // Variable de nombre d'espaces vides dans la FIFO
    U_manip16 valCrc16;     
    
    // Appel de fonction pour connaitre l'espace disponible dans la FIFO
    freeSize = GetWriteSpace(&descrFifoTX);
    
    valCrc16.val = 0xFFFF;
    
    // Si l'espace disponible dans la FIFO est supérieure ou égale à MESS_SIZE
    if(freeSize >= MESS_SIZE)
    {
        // Preparation des donnàes a envoyer
        TxMess.Start = 0xAA;
        TxMess.Speed = pData->SpeedSetting;
        TxMess.Angle = pData->AngleSetting;
        valCrc16.val = updateCRC16(valCrc16.val, TxMess.Start);
        valCrc16.val = updateCRC16(valCrc16.val, TxMess.Speed);
        valCrc16.val = updateCRC16(valCrc16.val, TxMess.Angle);

        TxMess.MsbCrc = valCrc16.shl.msb; 
        TxMess.LsbCrc = valCrc16.shl.lsb; 

        // Dépose le message dans la fifo
        PutCharInFifo (&descrFifoTX, TxMess.Start);
        PutCharInFifo (&descrFifoTX, TxMess.Speed);
        PutCharInFifo (&descrFifoTX, TxMess.Angle);
        PutCharInFifo (&descrFifoTX, TxMess.MsbCrc);
        PutCharInFifo (&descrFifoTX, TxMess.LsbCrc);  
    }    
    // Gestion du controle de flux
    // si on a un caractère à envoyer et que CTS = 0
    freeSize = GetReadSize(&descrFifoTX);
    if ((RS232_CTS == 0) && (freeSize > 0))
    {
        // Autorise int émission    
        PLIB_INT_SourceEnable(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);                
    }
}

// Interruption USART1
// !!!!!!!!
// Attention ne pas oublier de supprimer la réponse générée dans system_interrupt
// !!!!!!!!
 void __ISR(_UART_1_VECTOR, ipl5AUTO) _IntHandlerDrvUsartInstance0(void)
{
     // Declaration de variables locales
     uint8_t TxSize, freeSize;
     int8_t c;
     BOOL TxBuffFull;
     
    USART_ERROR UsartStatus;    

    // Marque début interruption avec Led3
    LED3_W = 1;
    
    // Is this an Error interrupt ?
    if ( PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_USART_1_ERROR) &&
                 PLIB_INT_SourceIsEnabled(INT_ID_0, INT_SOURCE_USART_1_ERROR) ) {
        /* Clear pending interrupt */
        PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_1_ERROR);
        // Traitement de l'erreur à la réception.
        while (PLIB_USART_ReceiverDataIsAvailable(USART_ID_1))
        {
            PLIB_USART_ReceiverByteReceive(USART_ID_1);
        }
    }

    // Is this an RX interrupt ?
    if ( PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_USART_1_RECEIVE) &&
                 PLIB_INT_SourceIsEnabled(INT_ID_0, INT_SOURCE_USART_1_RECEIVE) ) 
    {        
        // Oui Test si erreur parité ou overrun
        UsartStatus = PLIB_USART_ErrorsGet(USART_ID_1);

        if ( (UsartStatus & (USART_ERROR_PARITY |
                             USART_ERROR_FRAMING | USART_ERROR_RECEIVER_OVERRUN)) == 0) 
        {
            // Traitement RX à faire ICI
            // Lecture des caractères depuis le buffer HW -> fifo SW
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
            // La lecture des erreurs les effaces sauf pour overrun
            if ( (UsartStatus & USART_ERROR_RECEIVER_OVERRUN) == USART_ERROR_RECEIVER_OVERRUN) 
            {
                   PLIB_USART_ReceiverOverrunErrorClear(USART_ID_1);
            }
        }

        // Traitement controle de flux reception à faire ICI
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
        // Traitement TX à faire ICI
        TxSize = GetReadSize(&descrFifoTX);
        // Envoi des caractères depuis le fifo SW -> buffer HW
            
        //  S'il y a de la place dans le buffer d'émission (PLIB_USART_TransmitterBufferIsFull)
        TxBuffFull = PLIB_USART_TransmitterBufferIsFull(USART_ID_1);
        
        
        if((RS232_CTS == 0) && (TxSize > 0) && (TxBuffFull == false))
        {
            do{
                GetCharFromFifo(&descrFifoTX, &c);
                PLIB_USART_TransmitterByteSend(USART_ID_1, c);
                TxSize = GetReadSize(&descrFifoTX);
                TxBuffFull = PLIB_USART_TransmitterBufferIsFull(USART_ID_1);
                
            }while((RS232_CTS == 0) && (TxSize > 0) && (TxBuffFull == false));
        }
        //   (envoi avec PLIB_USART_TransmitterByteSend())
	   
        
        LED5_W = !LED5_R; // Toggle Led5
		
        // disable TX interrupt (pour éviter une interrupt. inutile si plus rien à transmettre)
        PLIB_INT_SourceDisable(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);
        
        // Clear the TX interrupt Flag (Seulement apres TX) 
        PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);
    }
    // Marque fin interruption avec Led3
    LED3_W = 0;
 }




