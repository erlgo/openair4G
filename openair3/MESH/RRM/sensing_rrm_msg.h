/*!
*******************************************************************************

\file       sensing_msg.h

\brief      Fichier d'entete contenant les declarations des types, des defines ,
et des fonctions relatives aux messages RRC-RRM ou RRC-RRCI. 
            
Les fonctions servent à créer le buffer de message, remplir 
l'entete et copier les parametres de fonction. Chaque fonction 
retourne le message qui pourra être envoye sur le socket entre le 
CMM et le RRM ou RRCI.

\author     BURLOT Pascal

\date       17/07/08

\par     Historique:
L.IACOBELLI 2009-10-19
+ sensing messages 

*******************************************************************************
*/

#ifndef __SENSING_RRM_MSG_H
#define __SENSING_RRM_MSG_H


#ifdef OPENAIR2_IN 
#include "rrm_sock.h"
#else
#include "RRC/MESH/rrc_rrm_interface.h" 
#include "RRC/MESH/L3_rrc_defs.h"
#endif
//#include "../../../openair2/RRC/MESH/rrc_rrm_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
*******************************************************************************
\brief  Enumeration des messages entre RRM/RRCI et le RRC
*/
typedef enum { 
RRC_UPDATE_SENS             , ///< Message SENSING->RRM : update of the sensing information measured by the nodes
RRM_SCAN_ORD                , ///< Message RRM->SENSING : order to scann indicated channels
RRM_END_SCAN_ORD            , ///< Message RRM->SENSING : end of a scanning process in sensors
SNS_END_SCAN_CONF           , ///< Message SENSING->RRC : end of a scanning process in sensors
NB_MSG_SENSING_RRM            ///< Nombre de message RRM-SENSING
} MSG_SENSING_RRM_T ;

/*! 
*******************************************************************************
\brief  Definition des parametres de la fonction rrm_scan_ord() dans 
une structure permettant le passage des parametres via un socket
*/
typedef struct {

unsigned int        NB_chan                 ; ///< Number of channels to scan if 0 means all channels
unsigned int        Meas_tpf                ; ///< time on each carrier           //mod_lor_10_02_19
unsigned int        Overlap                 ; ///< overlap factor (percentage)    //mod_lor_10_02_19
unsigned int        Sampl_nb                ; ///< number of samples per sub-band //mod_lor_10_04_01
Sens_ch_t           ch_to_scan[NB_SENS_MAX] ; ///< Vector of channels to scan     //mod_lor_10_02_19
} rrm_scan_ord_t ;

/*! 
*******************************************************************************
\brief  Definition des parametres de la fonction rrm_end_scan_ord() dans 
une structure permettant le passage des parametres via un socket
*/
typedef struct {
unsigned int        NB_chan              ; //!< Number of channels 
unsigned int        channels[NB_SENS_MAX]; //!< Vector of channels
} rrm_end_scan_ord_t ;

/*! 
*******************************************************************************
\brief  Definition des parametres de les fonctions rrc_update_sens() 
        rdans une structure permettant le passage 
        des parametres via un socket
*/
typedef struct {
    double              info_time              ; //!< Date of the message
    L2_ID               L2_id                  ; //!< Layer 2 ID (MAC) of sensing node
    unsigned int        NB_info                ; //!< number of sensed channels
    Sens_ch_t           Sens_meas[NB_SENS_MAX] ; //!< sensing information 
} rrc_update_sens_t; 


msg_t *msg_rrm_scan_ord( Instance_t inst, unsigned int NB_chan, unsigned int Meas_tpf, unsigned int Overlap, 
           unsigned int Sampl_nb, Sens_ch_t *ch_to_scan, Transaction_t Trans_id );  //mod_lor_10_02_19
msg_t *msg_rrm_end_scan_ord(Instance_t inst, unsigned int NB_chan, unsigned int *channels,
              Transaction_t Trans_id ); 

#endif /* SENSING_MSG_H */
