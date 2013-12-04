/*******************************************************************************
 *
 * Eurecom OpenAirInterface 3
 * Copyright(c) 2013 Eurecom
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Contact Information
 * Openair Admin: openair_admin@eurecom.fr
 * Openair Tech : openair_tech@eurecom.fr
 * Forums       : http://forums.eurecom.fsr/openairinterface
 * Address      : Eurecom, 2229, route des crêtes, 06560 Valbonne Sophia Antipolis, France
 *
 *******************************************************************************/
/*! \file lteRALenb_subscribe.h
 * \brief
 * \author  GAUTHIER Lionel, MAUREL Frederic, WETTERWALD Michelle
 * \date 2012
 * \version
 * \note
 * \bug
 * \warning
 */

#ifndef __LTE_RAL_UE_SUBSCRIBE_H__
#define __LTE_RAL_UE_SUBSCRIBE_H__
//-----------------------------------------------------------------------------
#        ifdef LTE_RAL_UE_PROCESS_C
#            define private_lteralue_subscribe(x)    x
#            define protected_lteralue_subscribe(x)  x
#            define public_lteralue_subscribe(x)     x
#        else
#            ifdef LTE_RAL_UE
#                define private_lteralue_subscribe(x)
#                define protected_lteralue_subscribe(x)  extern x
#                define public_lteralue_subscribe(x)     extern x
#            else
#                define private_lteralue_subscribe(x)
#                define protected_lteralue_subscribe(x)
#                define public_lteralue_subscribe(x)     extern x
#            endif
#        endif
//-----------------------------------------------------------------------------
#include "lteRALue.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/
protected_lteralue_subscribe(void mRAL_subscribe_request  (ral_ue_instance_t instanceP, MIH_C_Message_Link_Event_Subscribe_request_t*   msgP);)
protected_lteralue_subscribe(void mRAL_unsubscribe_request(ral_ue_instance_t instanceP, MIH_C_Message_Link_Event_Unsubscribe_request_t* msgP);)


#endif
