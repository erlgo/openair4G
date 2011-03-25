/*________________________phy_procedures_lte.c________________________

  Authors : Raymond Knopp, Florian Kaltenberger
  Company : EURECOM
  Emails  : knopp@eurecom.fr, kaltenbe@eurecom.fr
  ________________________________________________________________*/


#include "PHY/defs.h"
#include "PHY/extern.h"
#include "MAC_INTERFACE/defs.h"
#include "MAC_INTERFACE/extern.h"
#include "SCHED/defs.h"
#include "SCHED/extern.h"

//#ifdef CBMIMO1
#include "ARCH/CBMIMO1/DEVICE_DRIVER/extern.h"
#include "ARCH/CBMIMO1/DEVICE_DRIVER/defs.h"
#include "ARCH/CBMIMO1/DEVICE_DRIVER/from_grlib_softregs.h"
//#endif

#define DEBUG_PHY

#ifdef USER_MODE
#define DEBUG_PHY
#endif

#ifdef OPENAIR2
#include "LAYER2/MAC/extern.h"
#include "LAYER2/MAC/defs.h"
#endif

#define DIAG_PHY

#define NS_PER_SLOT 500000

extern inline unsigned int taus(void);
extern int exit_openair;

unsigned char dlsch_input_buffer[2700] __attribute__ ((aligned(16)));
int eNB_sync_buffer0[640*6] __attribute__ ((aligned(16)));
int eNB_sync_buffer1[640*6] __attribute__ ((aligned(16)));
int *eNB_sync_buffer[2] = {eNB_sync_buffer0, eNB_sync_buffer1};


unsigned int max_peak_val; 
int max_sect_id, max_sync_pos;

//DCI_ALLOC_t dci_alloc[8];

#ifdef EMOS
fifo_dump_emos_eNB emos_dump_eNB;
#endif

#ifdef DIAG_PHY
extern int rx_sig_fifo;
#endif

static unsigned char I0_clear = 1;


s32 add_ue(s16 rnti, PHY_VARS_eNB *phy_vars_eNB) {
  u8 i;

  msg("[PHY] eNB %d Adding UE with rnti %x\n",phy_vars_eNB->Mod_id,rnti);
  for (i=0;i<NUMBER_OF_UE_MAX;i++) {
    if ((phy_vars_eNB->dlsch_eNB[i]==NULL) || (phy_vars_eNB->ulsch_eNB[i]==NULL)) 
      msg("[PHY] Can't add UE, not enough memory allocated\n");
    else {
      if (phy_vars_eNB->eNB_UE_stats[i].crnti==0) {
	msg("[PHY] UE_id %d\n",i);
	phy_vars_eNB->dlsch_eNB[i][0]->rnti = rnti;
	phy_vars_eNB->ulsch_eNB[i]->rnti = rnti;
	phy_vars_eNB->eNB_UE_stats[i].crnti = rnti;
	return(i);
      }
    }
  }
  return(-1);
}

s32 remove_ue(u16 rnti, PHY_VARS_eNB *phy_vars_eNB) {
  u8 i;

  msg("[PHY] eNB %d removing UE with rnti %x\n",phy_vars_eNB->Mod_id,rnti);
  for (i=0;i<NUMBER_OF_UE_MAX;i++) {
    if ((phy_vars_eNB->dlsch_eNB[i]==NULL) || (phy_vars_eNB->ulsch_eNB[i]==NULL)) 
      msg("[PHY] Can't remove UE, not enough memory allocated\n");
    else {
      if (phy_vars_eNB->eNB_UE_stats[i].crnti==rnti) {
	msg("[PHY] UE_id %d\n",i);
	phy_vars_eNB->dlsch_eNB[i][0]->rnti = 0;
	phy_vars_eNB->ulsch_eNB[i]->rnti = 0;
	phy_vars_eNB->eNB_UE_stats[i].crnti = 0;
	return(i);
      }
    }
  }
  return(-1);
}

s8 find_next_ue_index(PHY_VARS_eNB *phy_vars_eNB) {
  u8 i;

  for (i=0;i<NUMBER_OF_UE_MAX;i++) {
    if ((phy_vars_eNB->dlsch_eNB[i]) && 
	(phy_vars_eNB->dlsch_eNB[i][0]) && 
	(phy_vars_eNB->dlsch_eNB[i][0]->rnti==0)) {
      return(i);
    }
  }
  return(-1);
}

s8 find_ue(u16 rnti, PHY_VARS_eNB *phy_vars_eNB) {
  u8 i;

  for (i=0;i<NUMBER_OF_UE_MAX;i++) {
    if ((phy_vars_eNB->dlsch_eNB[i]) && 
	(phy_vars_eNB->dlsch_eNB[i][0]) && 
	(phy_vars_eNB->dlsch_eNB[i][0]->rnti==rnti)) {
      return(i);
    }
  }
  return(-1);
}

int get_ue_active_harq_pid(u8 Mod_id,u16 rnti,u8 subframe,u8 *harq_pid,u8 *round,u8 ul_flag) {

  LTE_eNB_DLSCH_t *DLSCH_ptr;  
  LTE_eNB_ULSCH_t *ULSCH_ptr;  
  u8 subframe_m4,subframe_p4; 
  u8 i;
  s8 UE_id = find_ue(rnti,PHY_vars_eNB_g[Mod_id]);

  if (UE_id==-1) {
    msg("[PHY] Cannot find UE with rnti %x\n",rnti);
    *round=0;
    return(-1);
  }

  if (ul_flag == 0)  {// this is a DL request
    DLSCH_ptr = PHY_vars_eNB_g[Mod_id]->dlsch_eNB[UE_id][0];
    
    if (subframe<4)
      subframe_m4 = subframe+6;
    else
      subframe_m4 = subframe-4;

    printf("get_ue_active_harq_pid: subframe_m4 %d\n",subframe_m4);

    // switch on TDD or FDD configuration here later
    *harq_pid = DLSCH_ptr->harq_ids[subframe];
    if ((*harq_pid<DLSCH_ptr->Mdlharq) && 
	((DLSCH_ptr->harq_processes[*harq_pid]->round > 0)))
      *round = DLSCH_ptr->harq_processes[*harq_pid]->round;
    else if ((subframe_m4==5) || (subframe_m4==6)) {
      *harq_pid = DLSCH_ptr->harq_ids[subframe_m4];
      *round    = DLSCH_ptr->harq_processes[*harq_pid]->round;
    }
    else {
      // get first free harq_pid (i.e. round 0)
      for (i=0;i<DLSCH_ptr->Mdlharq;i++) {
	if (DLSCH_ptr->harq_processes[i]!=NULL) {
	  if (DLSCH_ptr->harq_processes[i]->round == 0) {
	    *harq_pid = i;
	    *round = 0;
	    return(0);
	  }
	}
	else {
	  msg("[PHY] eNB %d DLSCH process %d for rnti %x (UE_id %d) not allocated\n",Mod_id,i,rnti,UE_id);
	  return(-1);
	}
      }
    }
  }
  else {  // This is a UL request

    ULSCH_ptr = PHY_vars_eNB_g[Mod_id]->ulsch_eNB[UE_id];
    subframe_p4 = subframe+4;
    if (subframe_p4>9)
      subframe_p4-=10;

    // Note this is for TDD configuration 3,4,5 only
    *harq_pid = subframe_p4-2;
    *round    = ULSCH_ptr->harq_processes[*harq_pid]->round;
  }
  return(0);
}


#ifdef EMOS
void phy_procedures_emos_eNB_TX(unsigned char next_slot) {

  unsigned char sect_id,i;

  if (next_slot==1) {
    emos_dump_eNB.timestamp = rt_get_time_ns();
    emos_dump_eNB.frame_tx = mac_xface->frame;
  }
  if (next_slot%2==0) {
    for (i=0; i<2; i++) 
      memcpy(&emos_dump_eNB.DCI_alloc[i][next_slot>>1], &CH_mac_inst[0].DCI_pdu.dci_alloc[i], sizeof(DCI_ALLOC_t));
  }
  if (next_slot==19) {
    debug_msg("[PHY_PROCEDURES_eNB] frame %d, slot %d, Writing EMOS data to FIFO\n",mac_xface->frame, next_slot);
    if (rtf_put(CHANSOUNDER_FIFO_MINOR, &emos_dump_eNB, sizeof(fifo_dump_emos_eNB))!=sizeof(fifo_dump_emos_eNB)) {
      debug_msg("[PHY_PROCEDURES_eNB] frame %d, slot %d, Problem writing EMOS data to FIFO\n",mac_xface->frame, next_slot);
      return;
    }
  }
}
#endif

void phy_procedures_eNB_S_TX(unsigned char next_slot,PHY_VARS_eNB *phy_vars_eNB,u8 abstraction_flag) {

  int sect_id = 0, aa;

  if (next_slot%2==0) {
#ifdef DEBUG_PHY
    debug_msg("[PHY_PROCEDURES_eNB] Frame %d, slot %d: Generating pilots for DL-S\n",mac_xface->frame,next_slot);
#endif
    
    for (sect_id=0;sect_id<number_of_cards;sect_id++) {
      if (abstraction_flag == 0) {

	for (aa=0; aa<phy_vars_eNB->lte_frame_parms.nb_antennas_tx; aa++) {
	  
	  
	  /*
	    #ifdef DEBUG_PHY
	    printf("Clearing TX buffer %d at %p, length %d \n",aa,
	    &phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id][aa][next_slot*(phy_vars_eNB->lte_frame_parms.N_RB_DL*12)*(phy_vars_eNB->lte_frame_parms.symbols_per_tti>>1)],
	    (phy_vars_eNB->lte_frame_parms.N_RB_DL*12)*(phy_vars_eNB->lte_frame_parms.symbols_per_tti>>1)*sizeof(mod_sym_t));
	    #endif
	  */
#ifdef IFFT_FPGA
	  memset(&phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id][aa][next_slot*(phy_vars_eNB->lte_frame_parms.N_RB_DL*12)*(phy_vars_eNB->lte_frame_parms.symbols_per_tti>>1)],
		 0,(phy_vars_eNB->lte_frame_parms.N_RB_DL*12)*(phy_vars_eNB->lte_frame_parms.symbols_per_tti>>1)*sizeof(mod_sym_t));
#else
	  memset(&phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id][aa][next_slot*phy_vars_eNB->lte_frame_parms.ofdm_symbol_size*(phy_vars_eNB->lte_frame_parms.symbols_per_tti>>1)],
		 0,phy_vars_eNB->lte_frame_parms.ofdm_symbol_size*(phy_vars_eNB->lte_frame_parms.symbols_per_tti>>1)*sizeof(mod_sym_t));
#endif
	}
 	
	generate_pilots_slot(phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id],
			     AMP,
			     &phy_vars_eNB->lte_frame_parms,
			     sect_id,
			     next_slot);
	
	generate_pss(phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id],
		     AMP,
		     &phy_vars_eNB->lte_frame_parms,
		     sect_id,
		     2,
		     next_slot);
      
      }
      else {
	generate_pss_emul(phy_vars_eNB,sect_id);
      }
    }
  }
}
 
void phy_procedures_eNB_S_RX(unsigned char last_slot,PHY_VARS_eNB *phy_vars_eNB,u8 abstraction_flag) {

  int aa,l,sync_pos,sync_pos_slot;
  unsigned int sync_val;
  unsigned char sect_id=0, UE_id=find_next_ue_index(phy_vars_eNB);
  int time_in, time_out;
  short *x, *y;
  //  char fname[100],vname[100];


  if (last_slot%2==1) {
    
    /*
      for (sect_id = 0; sect_id < number_of_cards; sect_id++) {
      for (l=0;l<phy_vars_eNB->lte_frame_parms.symbols_per_tti/2;l++) {
	
      slot_fep_ul(&phy_vars_eNB->lte_frame_parms,
      phy_vars_eNB->lte_eNB_common_vars,
      l,
      last_slot,
      sect_id,
      #ifdef USER_MODE
      0
      #else
      1
      #endif
      );
      }
      }
    */

#ifndef USER_MODE
    time_in = openair_get_mbox();
#endif


    // we alternately process the signals from the three different sectors
#ifdef USER_MODE
    sect_id = 0;
#else
    sect_id = mac_xface->frame % number_of_cards; 
#endif
    //sect_id = 2;

    if (sect_id == 0) {
      max_peak_val = 0;
      max_sect_id = 0;
      max_sync_pos = 0;
    }


    if (abstraction_flag == 0) {
    // look for PSS in the last 3 symbols of the last slot
    // but before we need to zero pad the gaps that the HW removed
    // also add the signals from all antennas of all eNBs

      bzero(eNB_sync_buffer[0],640*6*sizeof(int));
      bzero(eNB_sync_buffer[1],640*6*sizeof(int));
      
      
      for (aa=0; aa<phy_vars_eNB->lte_frame_parms.nb_antennas_rx; aa++) {
	for (l=PRACH_SYMBOL; l<phy_vars_eNB->lte_frame_parms.symbols_per_tti/2; l++) {
	  
	  x = (short*) &eNB_sync_buffer[aa][(l-PRACH_SYMBOL)*(phy_vars_eNB->lte_frame_parms.ofdm_symbol_size+phy_vars_eNB->lte_frame_parms.nb_prefix_samples)+phy_vars_eNB->lte_frame_parms.nb_prefix_samples];
#ifdef USER_MODE
	  y = (short*) &phy_vars_eNB->lte_eNB_common_vars.rxdata[sect_id][aa][(last_slot*phy_vars_eNB->lte_frame_parms.symbols_per_tti/2+l)*(phy_vars_eNB->lte_frame_parms.ofdm_symbol_size+phy_vars_eNB->lte_frame_parms.nb_prefix_samples)+phy_vars_eNB->lte_frame_parms.nb_prefix_samples];
#else
	  y = (short*) &phy_vars_eNB->lte_eNB_common_vars.rxdata[sect_id][aa][(last_slot*phy_vars_eNB->lte_frame_parms.symbols_per_tti/2+l)*phy_vars_eNB->lte_frame_parms.ofdm_symbol_size];
#endif
	  //z = x;
	  //add_vector16(x,y,z,phy_vars_eNB->lte_frame_parms.ofdm_symbol_size*2);
	  memcpy(x,y,phy_vars_eNB->lte_frame_parms.ofdm_symbol_size*sizeof(int));
	}
      }

      /*      
#ifdef USER_MODE
#ifdef DEBUG_PHY
      write_output("eNB_sync_rx0.m","eNB_sync_rx0",&phy_vars_eNB->lte_eNB_common_vars.rxdata[sect_id][0][(last_slot*phy_vars_eNB->lte_frame_parms.symbols_per_tti/2+PRACH_SYMBOL)*(phy_vars_eNB->lte_frame_parms.ofdm_symbol_size+phy_vars_eNB->lte_frame_parms.nb_prefix_samples)*6+phy_vars_eNB->lte_frame_parms.nb_prefix_samples],(phy_vars_eNB->lte_frame_parms.ofdm_symbol_size+phy_vars_eNB->lte_frame_parms.nb_prefix_samples),1,1);
      write_output("eNB_sync_buffer0.m","eNB_sync_buf0",eNB_sync_buffer[0],(phy_vars_eNB->lte_frame_parms.ofdm_symbol_size+phy_vars_eNB->lte_frame_parms.nb_prefix_samples)*(phy_vars_eNB->lte_frame_parms.symbols_per_tti/2-PRACH_SYMBOL),1,1);
      write_output("eNB_sync_buffer1.m","eNB_sync_buf1",eNB_sync_buffer[1],(phy_vars_eNB->lte_frame_parms.ofdm_symbol_size+phy_vars_eNB->lte_frame_parms.nb_prefix_samples)*(phy_vars_eNB->lte_frame_parms.symbols_per_tti/2-PRACH_SYMBOL),1,1);
#endif      
#endif
      */
    
      sync_pos_slot = 0; //this is where the sync pos should be wrt eNB_sync_buffer
      
      sync_pos = lte_sync_time_eNB(eNB_sync_buffer, 
				   &phy_vars_eNB->lte_frame_parms, 
				   0,//sect_id,
				   (phy_vars_eNB->lte_frame_parms.symbols_per_tti/2 - PRACH_SYMBOL) * 
				   (phy_vars_eNB->lte_frame_parms.ofdm_symbol_size+phy_vars_eNB->lte_frame_parms.nb_prefix_samples),
				   &sync_val,
				   phy_vars_eNB->lte_eNB_common_vars.sync_corr[sect_id]);
      
#ifdef USER_MODE
#ifdef DEBUG_PHY    
      //      if (sect_id==0)
      //	write_output("sync_corr_eNB.m","synccorr",phy_vars_eNB->lte_eNB_common_vars.sync_corr[sect_id],
      //		     (phy_vars_eNB->lte_frame_parms.symbols_per_tti/2 - PRACH_SYMBOL) * 
      //		     (phy_vars_eNB->lte_frame_parms.ofdm_symbol_size+phy_vars_eNB->lte_frame_parms.nb_prefix_samples),1,2);
#endif    
#endif
    }
    else {
      sync_pos = lte_sync_time_eNB_emul(phy_vars_eNB,
					0,//sect_id,
					&sync_val);

    }

    if ((sync_pos>=0) && (sync_val > max_peak_val)) {
      max_peak_val = sync_val;
      max_sect_id = sect_id;
      max_sync_pos = sync_pos;
    }
      
    debug_msg("[PHY_PROCEDURES_eNB] Frame %d, found sect_id=%d, sync_pos=%d, sync_val=%u, max_peak_val=%u, max_peak_pos=%d\n",mac_xface->frame,sect_id,sync_pos,sync_val,max_peak_val,max_sync_pos);
      
#ifndef USER_MODE
      time_out = openair_get_mbox();
#endif
      
    if (max_peak_val>0) {
      if (sect_id==number_of_cards-1) {
	// this should be handled by the MAC
	phy_vars_eNB->eNB_UE_stats[UE_id].UE_timing_offset = cmax(max_sync_pos - sync_pos_slot - phy_vars_eNB->lte_frame_parms.nb_prefix_samples/8,0);
	phy_vars_eNB->eNB_UE_stats[UE_id].mode = PRACH;
	phy_vars_eNB->eNB_UE_stats[UE_id].sector = max_sect_id;
#ifdef DEBUG_PHY
	msg("[PHY_PROCEDURES_eNB] frame %d, slot %d: Found user %x in sector %d at pos %d val %d (time_in %d, time_out %d)\n",
	    mac_xface->frame, last_slot, 
	    UE_id, max_sect_id,
	    max_sync_pos, 
	    max_peak_val,
	    time_in, time_out);
#endif

#ifdef OPENAIR2
	msg("[PHY_PROCEDURES_eNB] frame %d, slot %d: Calling initiate_ra_proc (%p(%d,%d,%d,%d))\n",
	    mac_xface->frame, last_slot, mac_xface->initiate_ra_proc,
	    phy_vars_eNB->Mod_id,
	    0,
	    cmax(max_sync_pos - sync_pos_slot - phy_vars_eNB->lte_frame_parms.nb_prefix_samples/8,0),
	    max_sect_id);

	mac_xface->initiate_ra_proc(phy_vars_eNB->Mod_id,
				    0,
				    cmax(max_sync_pos - sync_pos_slot - phy_vars_eNB->lte_frame_parms.nb_prefix_samples/8,0),
				    max_sect_id);
#endif
	max_peak_val = 0;
	max_sect_id = 0;
	max_sync_pos = 0;
	
      }
    }
    else {
      debug_msg("[PHY_PROCEDURES_eNB] frame %d, slot %d: No user found\n",mac_xface->frame,last_slot);
      max_peak_val = 0;
      max_sect_id = 0;
      max_sync_pos = 0;
      
    }
    
    
    // Get noise levels
    
    
    for (sect_id=0;sect_id<number_of_cards;sect_id++) {
      /*
	sprintf(fname,"rxsigF0_%d.m",sect_id);
	sprintf(vname,"rxsF0_%d",sect_id);
	write_output(fname,vname, &phy_vars_eNB->lte_eNB_common_vars.rxdataF[sect_id][0][(19*phy_vars_eNB->lte_frame_parms.ofdm_symbol_size)<<1],512*2,2,1);
	sprintf(fname,"rxsigF1_%d.m",sect_id);
	sprintf(vname,"rxsF1_%d",sect_id);
	write_output(fname,vname, &phy_vars_eNB->lte_eNB_common_vars.rxdataF[sect_id][1][(19*phy_vars_eNB->lte_frame_parms.ofdm_symbol_size)<<1],512*2,2,1);
      */

      if (abstraction_flag == 0) {
	lte_eNB_I0_measurements(phy_vars_eNB,
				sect_id,
				phy_vars_eNB->first_run_I0_measurements);
      }
      else {
	lte_eNB_I0_measurements_emul(phy_vars_eNB,
				     sect_id);
      }
    }

    if (I0_clear == 1)
      I0_clear = 0;
  }
}

#ifdef EMOS
void phy_procedures_emos_eNB_RX(unsigned char last_slot) {

  unsigned char sect_id,i,aa;

  if (last_slot%2==1) {
    memcpy(&emos_dump_eNB.phy_vars_eNB->eNB_UE_stats[(last_slot>>1)-2],&phy_vars_eNB->eNB_UE_stats,sizeof(LTE_phy_vars_eNB->eNB_UE_stats));
  }

  if (last_slot==4) {
    emos_dump_eNB.rx_total_gain_dB = PHY_vars->rx_total_gain_eNB_dB;
    emos_dump_eNB.mimo_mode = openair_daq_vars.dlsch_transmission_mode;
  }

  if (last_slot==8) {
    emos_dump_eNB.ulsch_errors = phy_vars_eNB->eNB_UE_stats[1].ulsch_errors;
    for (sect_id = 0; sect_id<3; sect_id++)  
      memcpy(&emos_dump_eNB.PHY_measurements_eNB[sect_id],
	     &PHY_vars->PHY_measurements_eNB[sect_id],
	     sizeof(PHY_MEASUREMENTS_eNB));

  }

  if (last_slot%2==1) {
    for (sect_id = 0; sect_id<3; sect_id++)  
      for (aa=0; aa<phy_vars_eNB->lte_frame_parms.nb_antennas_rx; aa++) 
	memcpy(&emos_dump_eNB.channel[(last_slot>>1)-2][sect_id][aa][0],
	       phy_vars_eNB->lte_eNB_common_vars.srs_ch_estimates[sect_id][aa],
	       phy_vars_eNB->lte_frame_parms.ofdm_symbol_size*sizeof(int));
  }

}

#endif

void phy_procedures_eNB_TX(unsigned char next_slot,PHY_VARS_eNB *phy_vars_eNB,u8 abstraction_flag) {

  u8 *pbch_pdu=&phy_vars_eNB->pbch_pdu[0];
  //  unsigned int nb_dci_ue_spec = 0, nb_dci_common = 0;
  u16 input_buffer_length, re_allocated;
  u32 sect_id = 0,i,aa;
  u8 harq_pid, num_dci;
#ifdef OPENAIR2
  DCI_PDU *DCI_pdu;
#endif
  u8 *DLSCH_pdu;
  s8 UE_id;
  u8 num_pdcch_symbols;
  s16 crnti;

  for (sect_id = 0 ; sect_id < number_of_cards; sect_id++) {

   if (abstraction_flag==0) {
     if (next_slot%2 == 0) {
       for (aa=0; aa<phy_vars_eNB->lte_frame_parms.nb_antennas_tx;aa++) {
	 
#ifdef IFFT_FPGA
	 memset(&phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id][aa][next_slot*(phy_vars_eNB->lte_frame_parms.N_RB_DL*12)*(phy_vars_eNB->lte_frame_parms.symbols_per_tti>>1)],
		0,(phy_vars_eNB->lte_frame_parms.N_RB_DL*12)*(phy_vars_eNB->lte_frame_parms.symbols_per_tti)*sizeof(mod_sym_t));
#else
	 memset(&phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id][aa][next_slot*phy_vars_eNB->lte_frame_parms.ofdm_symbol_size*(phy_vars_eNB->lte_frame_parms.symbols_per_tti>>1)],
		0,phy_vars_eNB->lte_frame_parms.ofdm_symbol_size*(phy_vars_eNB->lte_frame_parms.symbols_per_tti)*sizeof(mod_sym_t));
#endif
       }
     }
    
     generate_pilots_slot(phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id],
			  AMP,
			  &phy_vars_eNB->lte_frame_parms,
			  sect_id,
			  next_slot);

   }
   
    
   if (next_slot == 1) {
      
      if ((mac_xface->frame&3) == 0) {
	((u8*) pbch_pdu)[1] = ((mac_xface->frame>>2)<<2)&0xff;
	((u8*) pbch_pdu)[0] = (mac_xface->frame>>6)&0xff;
      }
   
#ifdef DEBUG_PHY
      debug_msg("[PHY_PROCEDURES_eNB] Frame %d, slot %d: Calling generate_pbch, pdu[0]=%d\n",mac_xface->frame, next_slot,((u8*) pbch_pdu)[0]);
#endif

      if (abstraction_flag==0) {
	
	generate_pbch(phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id],
		      AMP,
		      &phy_vars_eNB->lte_frame_parms,
		      pbch_pdu,
		      mac_xface->frame&3);
	
	if (phy_vars_eNB->lte_frame_parms.frame_type == 0) {
	  generate_pss(phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id],
		       AMP,
		       &phy_vars_eNB->lte_frame_parms,
		       sect_id,
		       (phy_vars_eNB->lte_frame_parms.Ncp==0) ? 6 : 5,
		       next_slot);
	  //generate_sss
	}
      }
      else {
	generate_pbch_emul(phy_vars_eNB,pbch_pdu); 
      }
   }
   /*
   if (next_slot==1) {
       debug_msg("slot %d, txdataF=%p\n",next_slot,&phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id][0][next_slot*(phy_vars_eNB->lte_frame_parms.N_RB_DL*12)*(phy_vars_eNB->lte_frame_parms.symbols_per_tti>>1)]);
       for (i=0;i<10;i++) 
	 debug_msg("%08x\n",((unsigned int*)&phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id][0][next_slot*(phy_vars_eNB->lte_frame_parms.N_RB_DL*12)*(phy_vars_eNB->lte_frame_parms.symbols_per_tti>>1)])[i]);
   }
   */
  
   
   if (next_slot == 11) {
     
     if (abstraction_flag==0) {
       
	if (phy_vars_eNB->lte_frame_parms.frame_type == 0) {
	  generate_pss(phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id],
		       AMP,
		       &phy_vars_eNB->lte_frame_parms,
		       sect_id,
		       (phy_vars_eNB->lte_frame_parms.Ncp==0) ? 6 : 5,
		       next_slot);
	  //generate_sss2
	}
      }
   }
  }

  sect_id=0;

  if ((next_slot % 2)==0) {
    printf("UE %d: Mode %s\n",0,mode_string[phy_vars_eNB->eNB_UE_stats[0].mode]);
    mac_xface->eNB_dlsch_ulsch_scheduler(phy_vars_eNB->Mod_id,next_slot>>1);

#ifdef EMOS
    emos_dump_eNB.dci_cnt[next_slot>>1] = DCI_pdu->Num_common_dci + DCI_pdu->Num_ue_spec_dci; //nb_dci_common+nb_dci_ue_spec;
#endif

    // Parse DCI received from MAC
    DCI_pdu = mac_xface->get_dci_sdu(phy_vars_eNB->Mod_id,next_slot>>1);
#ifdef DEBUG_PHY
    //msg("[PHY][eNB] Subframe %d : ***************DCI******************\n",next_slot>>1);
#endif
    for (i=0;i<DCI_pdu->Num_common_dci + DCI_pdu->Num_ue_spec_dci ; i++) {
#ifdef DEBUG_PHY
      debug_msg("[PHY][eNB] Subframe %d : Doing DCI index %d/%d\n",next_slot>>1,i,DCI_pdu->Num_common_dci + DCI_pdu->Num_ue_spec_dci);
      if (((mac_xface->frame%100) == 0) || (mac_xface->frame < 20))
	dump_dci(&phy_vars_eNB->lte_frame_parms,&DCI_pdu->dci_alloc[i]);
#endif

      if (DCI_pdu->dci_alloc[i].rnti == SI_RNTI) {
	generate_eNB_dlsch_params_from_dci(next_slot>>1,
					   &DCI_pdu->dci_alloc[i].dci_pdu[0],
					   DCI_pdu->dci_alloc[i].rnti,
					   DCI_pdu->dci_alloc[i].format,
					   &phy_vars_eNB->dlsch_eNB_SI,
					   &phy_vars_eNB->lte_frame_parms,
					   SI_RNTI,
					   RA_RNTI,
					   P_RNTI,
					   phy_vars_eNB->eNB_UE_stats[0].DL_pmi_single);
      }
      else if (DCI_pdu->dci_alloc[i].rnti == RA_RNTI) {
	printf("[PHY] enb %d RA generate_eNB_dlsch_params_from_dci\n", phy_vars_eNB->Mod_id);
	generate_eNB_dlsch_params_from_dci(next_slot>>1,
					   &DCI_pdu->dci_alloc[i].dci_pdu[0],
					   DCI_pdu->dci_alloc[i].rnti,
					   DCI_pdu->dci_alloc[i].format,
					   &phy_vars_eNB->dlsch_eNB_ra,
					   &phy_vars_eNB->lte_frame_parms,
					   SI_RNTI,
					   RA_RNTI,
					   P_RNTI,
					   phy_vars_eNB->eNB_UE_stats[0].DL_pmi_single);
      }
      else if (DCI_pdu->dci_alloc[i].format == format0) {  // this is a ULSCH allocation

	harq_pid = subframe2harq_pid(&phy_vars_eNB->lte_frame_parms,((next_slot>>1)+4)%10);
	if (harq_pid==255) {
	  msg("Bad harq_pid\n");
#ifdef USER_MODE
	  exit(-1);
#else
	  exit_openair=1;
#endif
	}
	UE_id = find_ue((s16)DCI_pdu->dci_alloc[i].rnti,phy_vars_eNB);
	if (UE_id<0) {
	  msg("Unknown UE_id for rnti %x\n",(s16)DCI_pdu->dci_alloc[i].rnti);
#ifdef USER_MODE
	  exit(-1);
#else
	  exit_openair=1;
#endif
	}
	//#ifdef DEBUG_PHY
	  msg("[PHY_PROCEDURES_eNB] Frame %d, slot %d (%d): Generated ULSCH %d (rnti %x, dci %x) DCI, format 0 (DCI pos %d/%d), aggregation %d\n",mac_xface->frame,
	      next_slot,next_slot>>1,UE_id,DCI_pdu->dci_alloc[i].rnti,
	      *(unsigned int *)&DCI_pdu->dci_alloc[i].dci_pdu[0],
	      i,DCI_pdu->Num_common_dci + DCI_pdu->Num_ue_spec_dci,
	      DCI_pdu->dci_alloc[i].L);
	  //#endif
	
	    generate_eNB_ulsch_params_from_dci(&DCI_pdu->dci_alloc[i].dci_pdu[0],
					       DCI_pdu->dci_alloc[i].rnti,
					       (next_slot>>1),
					       format0,
					       phy_vars_eNB->ulsch_eNB[UE_id],
					       &phy_vars_eNB->lte_frame_parms,
					       SI_RNTI,
					       RA_RNTI,
					       P_RNTI,
					       0);  // do_srs
	
	    //#ifdef DEBUG_PHY
	    msg("[PHY PROCEDURES eNB] frame %d, subframe %d Setting scheduling flag for ULSCH %d harq_pid %d\n",
		mac_xface->frame,next_slot>>1,UE_id,harq_pid);
	    //#endif
	    phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->subframe_scheduling_flag = 1;
	    //}
	
      }
      
      else { // this is a DLSCH allocation
	/*
	  msg("DLSCH pdu alloc: (rballoc %x,mcs %d, ndi %d, rv %d)\n",
	  ((DCI2_5MHz_2A_M10PRB_TDD_t*)&DCI_pdu->dci_alloc[i].dci_pdu[0])->rballoc,
	  ((DCI2_5MHz_2A_M10PRB_TDD_t*)&DCI_pdu->dci_alloc[i].dci_pdu[0])->mcs1,
	  ((DCI2_5MHz_2A_M10PRB_TDD_t*)&DCI_pdu->dci_alloc[i].dci_pdu[0])->ndi1,
	  ((DCI2_5MHz_2A_M10PRB_TDD_t*)&DCI_pdu->dci_alloc[i].dci_pdu[0])->rv1);
	*/
	msg("[PHY][eNB] Searching for RNTI %x\n",DCI_pdu->dci_alloc[i].rnti);
	UE_id = find_ue((s16)DCI_pdu->dci_alloc[i].rnti,phy_vars_eNB);
	if (UE_id>=0) {
	  generate_eNB_dlsch_params_from_dci(next_slot>>1,
					     &DCI_pdu->dci_alloc[i].dci_pdu[0],
					     DCI_pdu->dci_alloc[i].rnti,
					     DCI_pdu->dci_alloc[i].format,
					     phy_vars_eNB->dlsch_eNB[(u8)UE_id],
					     &phy_vars_eNB->lte_frame_parms,
					     SI_RNTI,
					     RA_RNTI,
					     P_RNTI,
					     phy_vars_eNB->eNB_UE_stats[(u8)UE_id].DL_pmi_single);
      
	  msg("[PHY_PROCEDURES_eNB] Frame %d, slot %d: Generated DLSCH DCI (rnti %x => %x,%x), format %d, aggregation %d\n",mac_xface->frame, next_slot,
	      DCI_pdu->dci_alloc[i].rnti,
	      *(unsigned int*)DCI_pdu->dci_alloc[i].dci_pdu,
	      *(unsigned int*)(1+DCI_pdu->dci_alloc[i].dci_pdu),
	      DCI_pdu->dci_alloc[i].format,
	      DCI_pdu->dci_alloc[i].L);
	}
	else {
	  msg("[PHY_PROCEDURES_eNB] Frame %d : No UE_id with corresponding rnti %x, dropping DLSCH\n",mac_xface->frame,(s16)DCI_pdu->dci_alloc[i].rnti);
	}
      }

    

    } // loop over DCI

    // if we have PHICH to generate
    if (is_phich_subframe(&phy_vars_eNB->lte_frame_parms,next_slot>>1)) {
      //      debug_msg("[PHY_PROCEDURES_eNB] Frame %d, slot %d: Calling generate_phich_top\n",mac_xface->frame, next_slot);
      if (abstraction_flag==0) {
      //      generate_phich_top(&phy_vars_eNB->lte_frame_parms,next_slot>>1,phy_vars_eNB->ulsch_eNB[0],phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id]);
      }
      else {
	generate_phich_emul(phy_vars_eNB,next_slot>>1,phy_vars_eNB->ulsch_eNB[0]);
      }
    }

    // if we have DCI to generate do it now
    if ((DCI_pdu->Num_common_dci + DCI_pdu->Num_ue_spec_dci)>0) {

#ifdef DEBUG_PHY
      debug_msg("[PHY_PROCEDURES_eNB] Frame %d, slot %d: Calling generate_dci_top\n",mac_xface->frame, next_slot);
#endif

      if (abstraction_flag == 0) {
	for (sect_id=0;sect_id<number_of_cards;sect_id++) 
	  num_pdcch_symbols = generate_dci_top(DCI_pdu->Num_ue_spec_dci,//nb_dci_ue_spec,
					       DCI_pdu->Num_common_dci,//nb_dci_common,
					       DCI_pdu->dci_alloc,
					       0,
					       AMP,
					       &phy_vars_eNB->lte_frame_parms,
					       phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id],
					       next_slot>>1);
      }
      else {
	num_pdcch_symbols = generate_dci_top_emul(phy_vars_eNB,DCI_pdu->Num_ue_spec_dci,DCI_pdu->Num_common_dci,DCI_pdu->dci_alloc,next_slot>>1);
      }
#ifdef DEBUG_PHY
      debug_msg("[PHY_PROCEDURES_eNB] Frame %d, slot %d: num_pdcch_symbols=%d\n",mac_xface->frame, next_slot,num_pdcch_symbols);
#endif

    }
    else {  // for emulation!!
      phy_vars_eNB->num_ue_spec_dci[(next_slot>>1)&1]=0;
      phy_vars_eNB->num_common_dci[(next_slot>>1)&1]=0;
    }
    // For even next slots generate dlsch
    for (UE_id=0;UE_id<NUMBER_OF_UE_MAX;UE_id++) {
      if ((phy_vars_eNB->dlsch_eNB[(u8)UE_id][0])&&
	  (phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->rnti>0)&&
	  (phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->active == 1)) {
	harq_pid = phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->current_harq_pid;
	input_buffer_length = phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->harq_processes[harq_pid]->TBS/8;
      
	//      for (i=0;i<input_buffer_length;i++)
	//	CH_mac_inst[0].DLSCH_pdu[0][0].payload[0][i]= (unsigned char)(taus()&0xff);
      

#ifdef DEBUG_PHY
	debug_msg("[PHY_PROCEDURES_eNB] Frame %d, slot %d: Calling generate_dlsch for rnti %x (harq_pid %d) with input size = %d, G %d\n",mac_xface->frame, next_slot, phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->rnti, harq_pid,input_buffer_length,
		  get_G(&phy_vars_eNB->lte_frame_parms,
			phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->nb_rb,
			phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->rb_alloc,
			get_Qm(phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->harq_processes[harq_pid]->mcs),
			num_pdcch_symbols,next_slot>>1));
#endif

	phy_vars_eNB->eNB_UE_stats[(u8)UE_id].dlsch_sliding_cnt++;
	if (phy_vars_eNB->dlsch_eNB[UE_id][0]->harq_processes[harq_pid]->Ndi == 1) {
	  DLSCH_pdu = mac_xface->get_dlsch_sdu(phy_vars_eNB->Mod_id,
					       phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->rnti,
					       0);
	  

#ifdef DEBUG_PHY
	  debug_msg("[PHY] eNB DLSCH SDU: \n");
	  for (i=0;i<phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->harq_processes[harq_pid]->TBS>>3;i++)
	    debug_msg("%x.",(u8)DLSCH_pdu[i]);
	  debug_msg("\n");
#endif
	}
	else {
	  msg("[PHY][eNB] This DLSCH is a retransmission\n");
	}
	if (abstraction_flag==0) {

	  // 36-212
	  dlsch_encoding(DLSCH_pdu,
			 &phy_vars_eNB->lte_frame_parms,
			 num_pdcch_symbols,
			 phy_vars_eNB->dlsch_eNB[(u8)UE_id][0],
			 next_slot>>1);
	  // 36-211
	  dlsch_scrambling(&phy_vars_eNB->lte_frame_parms,
			   num_pdcch_symbols,
			   phy_vars_eNB->dlsch_eNB[(u8)UE_id][0],
			   get_G(&phy_vars_eNB->lte_frame_parms,
				 phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->nb_rb,
				 phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->rb_alloc,
				 get_Qm(phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->harq_processes[harq_pid]->mcs),
				 num_pdcch_symbols,next_slot>>1),
			   0,
			   next_slot);      
	  for (sect_id=0;sect_id<number_of_cards;sect_id++)
	    
	    re_allocated = dlsch_modulation(phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id],
					    AMP,
					    next_slot/2,
					    &phy_vars_eNB->lte_frame_parms,
					    num_pdcch_symbols,
					    phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]);
	  /*
	    if (mimo_mode == DUALSTREAM) {
	    dlsch_encoding(input_buffer,
	    phy_vars_eNB->lte_frame_parms,
	    dlsch_eNB[1]);
	    
	    re_allocated += dlsch_modulation(phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id],
	    1024,
	    next_slot>>1,
	    phy_vars_eNB->lte_frame_parms,
	    dlsch_eNB[1]);
	    }
	  */
	}
	else {
	  dlsch_encoding_emul(phy_vars_eNB,
			      DLSCH_pdu,
			      phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]);
	}
	phy_vars_eNB->dlsch_eNB[(u8)UE_id][0]->active = 0;
	
	//#ifdef DEBUG_PHY    
	debug_msg("[PHY_PROCEDURES_eNB] Frame %d, slot %d, DLSCH re_allocated = %d mod id %d\n",
		  mac_xface->frame, next_slot, re_allocated, phy_vars_eNB->Mod_id);
	//#endif
      }
    }

    if (phy_vars_eNB->dlsch_eNB_SI->active == 1) {
      input_buffer_length = phy_vars_eNB->dlsch_eNB_SI->harq_processes[0]->TBS/8;


      DLSCH_pdu = mac_xface->get_dlsch_sdu(phy_vars_eNB->Mod_id,
					   SI_RNTI,
					   0);


      
#ifdef DEBUG_PHY
      debug_msg("[PHY_PROCEDURES_eNB] Frame %d, slot %d: Calling generate_dlsch (SI) with input size = %d, num_pdcch_symbols %d\n",mac_xface->frame, next_slot, input_buffer_length,num_pdcch_symbols);
      for (i=0;i<input_buffer_length;i++)
	debug_msg("dlsch_input_buffer[%d]=%x\n",i,DLSCH_pdu[i]);
#endif

      if (abstraction_flag == 0) {

	dlsch_encoding(DLSCH_pdu,
		       &phy_vars_eNB->lte_frame_parms,
		       num_pdcch_symbols,
		       phy_vars_eNB->dlsch_eNB_SI,
		       next_slot>>1);
	
	dlsch_scrambling(&phy_vars_eNB->lte_frame_parms,
			 num_pdcch_symbols,
			 phy_vars_eNB->dlsch_eNB_SI,
			 get_G(&phy_vars_eNB->lte_frame_parms,
			       phy_vars_eNB->dlsch_eNB_SI->nb_rb,
			       phy_vars_eNB->dlsch_eNB_SI->rb_alloc,
			       get_Qm(phy_vars_eNB->dlsch_eNB_SI->harq_processes[0]->mcs),
			       num_pdcch_symbols,next_slot>>1),
			 0,
			 next_slot);      
	
	for (sect_id=0;sect_id<number_of_cards;sect_id++) 
	  re_allocated = dlsch_modulation(phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id],
					  AMP,
					  next_slot/2,
					  &phy_vars_eNB->lte_frame_parms,
					  num_pdcch_symbols,
					  phy_vars_eNB->dlsch_eNB_SI);
      } 
      else {
	dlsch_encoding_emul(phy_vars_eNB,
			    dlsch_input_buffer,
			    phy_vars_eNB->dlsch_eNB_SI);
      }
      phy_vars_eNB->dlsch_eNB_SI->active = 0;
      
#ifdef DEBUG_PHY    
      debug_msg("[PHY_PROCEDURES_eNB] Frame %d, slot %d, DLSCH (SI) re_allocated = %d\n",mac_xface->frame, next_slot, re_allocated);
#endif
      
    }
  
    if (phy_vars_eNB->dlsch_eNB_ra->active == 1) {
      input_buffer_length = phy_vars_eNB->dlsch_eNB_ra->harq_processes[0]->TBS/8;
      crnti = mac_xface->fill_rar(0,
				  dlsch_input_buffer,
				  phy_vars_eNB->lte_frame_parms.N_RB_UL,
				  input_buffer_length);

      UE_id = add_ue(crnti,phy_vars_eNB);
      if (UE_id==-1) {
	mac_xface->macphy_exit("[PHY_PROCEDURES_eNB] Max user count reached.\n");
      }
      msg("[PHY_PROCEDURES_eNB] Added UE_id %d for rnti %x\n",UE_id,crnti);

      phy_vars_eNB->eNB_UE_stats[UE_id].mode = RA_RESPONSE;
      
      generate_eNB_ulsch_params_from_rar(dlsch_input_buffer,
					 (next_slot>>1),
					 phy_vars_eNB->ulsch_eNB[UE_id],
					 &phy_vars_eNB->lte_frame_parms);

      phy_vars_eNB->ulsch_eNB[UE_id]->RRCConnRequest_active = 1;

      get_RRCConnReq_alloc(&phy_vars_eNB->lte_frame_parms,
			   next_slot>>1,
			   mac_xface->frame,
			   &phy_vars_eNB->ulsch_eNB[UE_id]->RRCConnRequest_frame,
			   &phy_vars_eNB->ulsch_eNB[UE_id]->RRCConnRequest_subframe);
      
      //      for (i=0;i<input_buffer_length;i++)
      //	dlsch_input_buffer[i]= (unsigned char)(taus()&0xff);
      
#ifdef DEBUG_PHY
      msg("[PHY_PROCEDURES_eNB] Frame %d, next slot %d: Calling generate_dlsch (RA) with input size = %d,RRCConnRequest frame %d, RRCConnRequest subframe %d\n",mac_xface->frame, next_slot,input_buffer_length, phy_vars_eNB->ulsch_eNB[UE_id]->RRCConnRequest_frame,phy_vars_eNB->ulsch_eNB[UE_id]->RRCConnRequest_subframe);
      //      for (i=0;i<input_buffer_length;i++)
      //	debug_msg("RA : dlsch_input_buffer[%d]=%x\n",i,dlsch_input_buffer[i]);
#endif

      if (abstraction_flag == 0) {

	dlsch_encoding(dlsch_input_buffer,
		       &phy_vars_eNB->lte_frame_parms,
		       num_pdcch_symbols,
		       phy_vars_eNB->dlsch_eNB_ra,
		       next_slot>>1);

	phy_vars_eNB->dlsch_eNB_ra->rnti = RA_RNTI;
	dlsch_scrambling(&phy_vars_eNB->lte_frame_parms,
			 num_pdcch_symbols,
			 phy_vars_eNB->dlsch_eNB_ra,
			 get_G(&phy_vars_eNB->lte_frame_parms,
			       phy_vars_eNB->dlsch_eNB_ra->nb_rb,
			       phy_vars_eNB->dlsch_eNB_ra->rb_alloc,
			       get_Qm(phy_vars_eNB->dlsch_eNB_ra->harq_processes[0]->mcs),
			       num_pdcch_symbols,next_slot>>1),
			 0,
			 next_slot);
	for (sect_id=0;sect_id<number_of_cards;sect_id++) 
	  re_allocated = dlsch_modulation(phy_vars_eNB->lte_eNB_common_vars.txdataF[sect_id],
					  AMP,
					  next_slot/2,
					  &phy_vars_eNB->lte_frame_parms,
					  num_pdcch_symbols,
					  phy_vars_eNB->dlsch_eNB_ra);
      }
      else {
	dlsch_encoding_emul(phy_vars_eNB,
			    dlsch_input_buffer,
			    phy_vars_eNB->dlsch_eNB_ra);
      }
      phy_vars_eNB->dlsch_eNB_ra->active = 0;
	
#ifdef DEBUG_PHY    
      msg("[PHY_PROCEDURES_eNB] Frame %d, slot %d, DLSCH (RA) re_allocated = %d\n",mac_xface->frame, next_slot, re_allocated);
#endif
    }
  }



#ifdef EMOS
  phy_procedures_emos_eNB_TX(next_slot);
#endif
}
  
void process_RRCConnRequest(PHY_VARS_eNB *phy_vars_eNB,u8 last_slot,u8 UE_id, u8 harq_pid) {
  // this prepares the demodulation of the first PUSCH of a new user, containing RRC connection request

  phy_vars_eNB->ulsch_eNB[UE_id]->RRCConnRequest_flag = 0;

  if ((phy_vars_eNB->ulsch_eNB[UE_id]->RRCConnRequest_active == 1) && 
      ((last_slot%2)==1) && 
      (phy_vars_eNB->ulsch_eNB[UE_id]->RRCConnRequest_subframe == (last_slot>>1)) &&
      (phy_vars_eNB->ulsch_eNB[UE_id]->RRCConnRequest_frame == (mac_xface->frame)))   {

    //    harq_pid = 0;

    phy_vars_eNB->ulsch_eNB[UE_id]->RRCConnRequest_active = 0;
    phy_vars_eNB->ulsch_eNB[UE_id]->RRCConnRequest_flag = 1;
    phy_vars_eNB->ulsch_eNB[UE_id]->harq_processes[harq_pid]->subframe_scheduling_flag=1;
    msg("[PHY_PROCEDURES_eNB] frame %d, slot %d, subframe %d: Doing RRCConnRequest for UE %d\n",mac_xface->frame,last_slot,last_slot>>1,UE_id);
  }
}
// This function retrieves the harq_pid of the corresponding DLSCH process
// and updates the error statistics of the DLSCH based on the received ACK
// info from UE along with the round index.  It also performs the fine-grain
// rate-adaptation based on the error statistics derived from the ACK/NAK process

void process_HARQ_feedback(u8 UE_id, u8 subframe, PHY_VARS_eNB *phy_vars_eNB) {

  u8 dl_harq_pid,j;
  u8 ACK_index;
  LTE_eNB_DLSCH_t *dlsch             =  phy_vars_eNB->dlsch_eNB[UE_id][0];
  LTE_eNB_UE_stats *ue_stats         =  &phy_vars_eNB->eNB_UE_stats[UE_id];
  LTE_DL_eNB_HARQ_t *dlsch_harq_proc;
 
  for (ACK_index=0;ACK_index<2;ACK_index++) {  // Fix this for FDD and other TDD modes
    
    dl_harq_pid     = dlsch->harq_ids[ul_ACK_subframe2_dl_subframe(&phy_vars_eNB->lte_frame_parms,
								   subframe,
								   ACK_index)];
    msg("[PHY] eNB %d subframe %d : Checking ACK_index %d for UE %d => dl_harq_pid %d\n",phy_vars_eNB->Mod_id,subframe,ACK_index,UE_id,dl_harq_pid);
    if (dl_harq_pid<dlsch->Mdlharq) {

      
      dlsch_harq_proc = dlsch->harq_processes[dl_harq_pid];
      msg("[PHY] eNB %d Process %d status %d, round %d\n",phy_vars_eNB->Mod_id,dl_harq_pid,dlsch_harq_proc->status,dlsch_harq_proc->round);
      if ((dl_harq_pid<dlsch->Mdlharq) &&
	  (dlsch->harq_processes[dl_harq_pid]->status == ACTIVE)) {
	// dl_harq_pid of DLSCH is still active
	
	msg("[PHY] eNB %d Process %d is active\n",phy_vars_eNB->Mod_id,dl_harq_pid);
	if (phy_vars_eNB->ulsch_eNB[(u8)UE_id]->o_ACK[ACK_index] == 0) {
	  // Received NAK 
	  
	  // First increment round-0 NAK counter 
	  if (dlsch_harq_proc->round == 0)
	    ue_stats->dlsch_NAK[dlsch_harq_proc->round]++;
	  
	  // then Increment DLSCH round index 
	  dlsch_harq_proc->round++;
	  
	  //      printf("Frame %d Setting round to %d for pid %d (subframe %d)\n",mac_xface->frame,
	  //	     dlsch_harq_proc->round,dl_harq_pid,last_slot>>1);
	  
	  if (dlsch_harq_proc->round == dlsch->Mdlharq) {
	    // This was the last round for DLSCH so reset round and increment l2_error counter
	    dlsch_harq_proc->round = 0;
	    ue_stats->dlsch_l2_errors++;
	  }
	}
	else {
	  msg("[PHY][eNB] ACK Received, resetting process\n");
	  // Received ACK so set round to 0 and set dlsch_harq_pid IDLE
	  dlsch_harq_proc->round  = 0;
	  dlsch_harq_proc->status = SCH_IDLE; 
	  //	printf("Frame %d Setting round to 0 for pid %d (subframe %d)\n",mac_xface->frame,dl_harq_pid,last_slot>>1);
	}
	
	// Do fine-grain rate-adaptation for DLSCH 
	if (ue_stats->dlsch_NAK[0] > dlsch->error_threshold) {
	  if (ue_stats->dlsch_mcs_offset == 1)
	    ue_stats->dlsch_mcs_offset=0;
	  else
	    ue_stats->dlsch_mcs_offset=-1;
	}
	
	// Clear NAK stats and adjust mcs offset
	// after measurement window timer expires
	if ((ue_stats->dlsch_sliding_cnt == dlsch->ra_window_size) ) {
	  if ((ue_stats->dlsch_mcs_offset == 0) && (ue_stats->dlsch_NAK[0] < 2))
	    ue_stats->dlsch_mcs_offset = 1;
	  if ((ue_stats->dlsch_mcs_offset == 1) && (ue_stats->dlsch_NAK[0] > 2))
	    ue_stats->dlsch_mcs_offset = 0;
	  if ((ue_stats->dlsch_mcs_offset == 0) && (ue_stats->dlsch_NAK[0] > 2))
	    ue_stats->dlsch_mcs_offset = -1;
	  if ((ue_stats->dlsch_mcs_offset == -1) && (ue_stats->dlsch_NAK[0] < 2))
	    ue_stats->dlsch_mcs_offset = 0;
	  
	  for (j=0;j<phy_vars_eNB->dlsch_eNB[j][0]->Mdlharq;j++)
	    ue_stats->dlsch_NAK[j] = 0;
	  ue_stats->dlsch_sliding_cnt = 0;
	}
	
	
      }
    }
  }
}

void phy_procedures_eNB_RX(unsigned char last_slot,PHY_VARS_eNB *phy_vars_eNB,u8 abstraction_flag) {
  //RX processing
  unsigned int l, ret,i,j;
  unsigned int sect_id=0,UE_id=0;
  int *ulsch_power;
  unsigned char harq_pid,relay_flag=0,diversity_scheme=0;
  int sync_pos;


  //  debug_msg("Running phy_procedures_eNB_RX(%d), eNB_mode = %s\n",last_slot,mode_string[phy_vars_eNB->eNB_UE_stats.mode[UE_id]]);
  if (abstraction_flag == 0) {
    for (l=0;l<phy_vars_eNB->lte_frame_parms.symbols_per_tti/2;l++) {
      
      for (sect_id=0;sect_id<number_of_cards;sect_id++) {
	slot_fep_ul(&phy_vars_eNB->lte_frame_parms,
		    &phy_vars_eNB->lte_eNB_common_vars,
		    l,
		    last_slot,
		    sect_id,
#ifdef USER_MODE
		    0
#else
		    1
#endif
		    );
      }
    }
    
    sect_id = 0;
    for (UE_id=0;UE_id<NUMBER_OF_UE_MAX;UE_id++) {
      
      if ((phy_vars_eNB->eNB_UE_stats[UE_id].mode>PRACH) && (last_slot%2==1)) {
	
	debug_msg("[PHY_PROCEDURES_eNB] frame %d, slot %d: Doing SRS estimation and measurements for UE_id %d (UE_mode %d)\n",
		  mac_xface->frame, last_slot, 
		  UE_id,phy_vars_eNB->eNB_UE_stats[UE_id].mode);
	
	for (sect_id=0;sect_id<number_of_cards;sect_id++) {
	  
	  lte_srs_channel_estimation(&phy_vars_eNB->lte_frame_parms,
				     &phy_vars_eNB->lte_eNB_common_vars,
				     &phy_vars_eNB->lte_eNB_srs_vars[UE_id],
				     &phy_vars_eNB->soundingrs_ul_config_dedicated[UE_id],
				     last_slot>>1,
				     sect_id);
	  lte_eNB_srs_measurements(phy_vars_eNB,
				   sect_id,
				   UE_id,
				   1);

	  debug_msg("[PHY_PROCEDURES_eNB] frame %d, slot %d: UE_id %d, sect_id %d: RX RSSI %d (from SRS)\n",
		    mac_xface->frame, last_slot, 
		    UE_id,sect_id,
		    phy_vars_eNB->PHY_measurements_eNB[sect_id].rx_rssi_dBm[UE_id]);
	}
	
	sect_id=0;
#ifdef USER_MODE
	//write_output("srs_est0.m","srsest0",phy_vars_eNB->lte_eNB_common_vars.srs_ch_estimates[0][0],512,1,1);
	//write_output("srs_est1.m","srsest1",phy_vars_eNB->lte_eNB_common_vars.srs_ch_estimates[0][1],512,1,1);
#endif
	
	//msg("timing advance in\n");
	sync_pos = lte_est_timing_advance(&phy_vars_eNB->lte_frame_parms,
					  &phy_vars_eNB->lte_eNB_srs_vars[UE_id],
					  &sect_id,
					  phy_vars_eNB->first_run_timing_advance[UE_id],
					  number_of_cards,
					  24576);
	
	//msg("timing advance out\n");
	
	//phy_vars_eNB->eNB_UE_stats[UE_id].UE_timing_offset = sync_pos - phy_vars_eNB->lte_frame_parms.nb_prefix_samples/8;
	phy_vars_eNB->eNB_UE_stats[UE_id].UE_timing_offset = 0;
	phy_vars_eNB->eNB_UE_stats[UE_id].sector = sect_id;
	
	debug_msg("[PHY_PROCEDURES_eNB] frame %d, slot %d: user %d in sector %d: timing_advance = %d\n",
		  mac_xface->frame, last_slot, 
		  UE_id, sect_id,
		  phy_vars_eNB->eNB_UE_stats[UE_id].UE_timing_offset);
      }
    }
      	
  }

#ifdef OPENAIR2
  /*
  */
    // Check for active processes in current subframe
  harq_pid = subframe2harq_pid(&phy_vars_eNB->lte_frame_parms,last_slot>>1);

  for (i=0;i<NUMBER_OF_UE_MAX;i++) {

    if (phy_vars_eNB->eNB_UE_stats[i].mode == RA_RESPONSE)
      process_RRCConnRequest(phy_vars_eNB,last_slot,i,harq_pid);

    if ((phy_vars_eNB->ulsch_eNB[i]) &&
	(phy_vars_eNB->ulsch_eNB[i]->rnti>0) &&
	(phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->subframe_scheduling_flag==1) && 
	((last_slot%2)==1)) {

#ifdef DEBUG_PHY
      msg("[PHY_PROCEDURES_eNB] frame %d, slot %d, subframe %d: Scheduling ULSCH %d Reception for rnti %x harq_pid %d\n",mac_xface->frame,last_slot,last_slot>>1,i,phy_vars_eNB->ulsch_eNB[i]->rnti,harq_pid);
#endif

      //#ifdef DEBUG_PHY
      if (phy_vars_eNB->ulsch_eNB[i]->RRCConnRequest_flag == 1)
	msg("[PHY_PROCEDURES_eNB] frame %d, slot %d, subframe %d: Scheduling ULSCH Reception for RRCConnRequest in Sector %d\n",
	    mac_xface->frame,last_slot,last_slot>>1,phy_vars_eNB->eNB_UE_stats[i].sector);
      else
	msg("[PHY_PROCEDURES_eNB] frame %d, slot %d, subframe %d: Scheduling ULSCH Reception for UE %d Mode %s sect_id %d\n",
	    mac_xface->frame,last_slot,last_slot>>1,i,mode_string[phy_vars_eNB->eNB_UE_stats[i].mode],phy_vars_eNB->eNB_UE_stats[i].sector);
      //#endif

      if (abstraction_flag==0) {
	ulsch_power = rx_ulsch(&phy_vars_eNB->lte_eNB_common_vars,
			       phy_vars_eNB->lte_eNB_ulsch_vars[i],  
			       &phy_vars_eNB->lte_frame_parms,
			       last_slot>>1,
			       phy_vars_eNB->eNB_UE_stats[i].sector,  // this is the effective sector id
			       phy_vars_eNB->ulsch_eNB[i],
			       relay_flag,
			       diversity_scheme);
      }
      else {
	ulsch_power = rx_ulsch_emul(phy_vars_eNB,
				    last_slot>>1,
				    phy_vars_eNB->eNB_UE_stats[i].sector,  // this is the effective sector id
				    i);
      }


      for (j=0;j<phy_vars_eNB->lte_frame_parms.nb_antennas_rx;j++)
	phy_vars_eNB->eNB_UE_stats[i].UL_rssi[j] = dB_fixed(ulsch_power[j]) - phy_vars_eNB->rx_total_gain_eNB_dB;

#ifdef DEBUG_PHY
      msg("[PHY_PROCEDURES_eNB] frame %d, slot %d, subframe %d: ULSCH RX power (%d,%d) dB\n",mac_xface->frame,last_slot,last_slot>>1,dB_fixed(ulsch_power[0]),dB_fixed(ulsch_power[1]));
#endif

      if (abstraction_flag == 0) {
	ret = ulsch_decoding(phy_vars_eNB->lte_eNB_ulsch_vars[i]->llr,
			     &phy_vars_eNB->lte_frame_parms,
			     phy_vars_eNB->ulsch_eNB[i],
			     last_slot>>1,
			     0);  // control_only_flag
      }
      else {
	ret = ulsch_decoding_emul(phy_vars_eNB,
				  last_slot>>1,
				  i);
      }
      msg("[PHY_PROCEDURES_eNB] frame %d, slot %d, subframe %d: ULSCH %d RX power (%d,%d) dB ACK (%d,%d)\n",mac_xface->frame,last_slot,last_slot>>1,i,dB_fixed(ulsch_power[0]),dB_fixed(ulsch_power[1]),phy_vars_eNB->ulsch_eNB[i]->o_ACK[0],phy_vars_eNB->ulsch_eNB[i]->o_ACK[1]);

    
      phy_vars_eNB->eNB_UE_stats[i].ulsch_decoding_attempts[harq_pid][phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round]++;
 
      phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->subframe_scheduling_flag=0;

      
      if (phy_vars_eNB->ulsch_eNB[i]->cqi_crc_status == 1) {
#ifdef DEBUG_PHY
	if (((mac_xface->frame%100) == 0) || (mac_xface->frame < 10)) 
	  print_CQI(phy_vars_eNB->ulsch_eNB[i]->o,phy_vars_eNB->ulsch_eNB[i]->o_RI,wideband_cqi,0);
#endif
	extract_CQI(phy_vars_eNB->ulsch_eNB[i]->o,phy_vars_eNB->ulsch_eNB[i]->o_RI,wideband_cqi,&phy_vars_eNB->eNB_UE_stats[i]);
	phy_vars_eNB->eNB_UE_stats[i].rank = phy_vars_eNB->ulsch_eNB[i]->o_RI[0];
      }

      if (ret == (1+MAX_TURBO_ITERATIONS)) {
	phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->phich_active = 1;
	phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->phich_ACK = 0;
	phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round++;
	if (phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round==
	    phy_vars_eNB->ulsch_eNB[i]->Mdlharq) {
	  phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round=0;
	  phy_vars_eNB->eNB_UE_stats[i].ulsch_errors[harq_pid]++;
	  phy_vars_eNB->eNB_UE_stats[i].ulsch_consecutive_errors[harq_pid]++;
	}

	if (phy_vars_eNB->ulsch_eNB[i]->RRCConnRequest_flag == 1) {
	  //	eNB_generate_RRCConnReq_ack = 0;
	  phy_vars_eNB->eNB_UE_stats[i].mode = PRACH;
	  remove_ue(phy_vars_eNB->eNB_UE_stats[i].crnti,phy_vars_eNB);
	}

	// If we've dropped the UE, go back to PRACH mode for this UE
	if (phy_vars_eNB->eNB_UE_stats[i].ulsch_consecutive_errors[harq_pid] == 20) {
	  phy_vars_eNB->eNB_UE_stats[i].mode = PRACH;
	  remove_ue(phy_vars_eNB->eNB_UE_stats[i].crnti,phy_vars_eNB);
	  phy_vars_eNB->eNB_UE_stats[i].ulsch_consecutive_errors[harq_pid]=0;
	}
      }  // ulsch in error
      else {
	phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->phich_active = 1;
	phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->phich_ACK = 1;
	phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->round = 0;
	phy_vars_eNB->eNB_UE_stats[i].ulsch_consecutive_errors[harq_pid] = 0;

	if (phy_vars_eNB->ulsch_eNB[i]->RRCConnRequest_flag == 1) {
	  printf("[PHY]Terminating ra_proc for harq %d, UE %d\n",harq_pid,i);
	  mac_xface->terminate_ra_proc(phy_vars_eNB->Mod_id,phy_vars_eNB->ulsch_eNB[i]->rnti,phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->b);


	  phy_vars_eNB->eNB_UE_stats[i].mode = PUSCH;
	  phy_vars_eNB->ulsch_eNB[i]->RRCConnRequest_flag = 0;

	  msg("[PHY_PROCEDURES_eNB] Frame %d : RX Subframe %d Setting UE %d mode to PUSCH\n",mac_xface->frame,last_slot>>1,i);

	  for (j=0;j<phy_vars_eNB->dlsch_eNB[i][0]->Mdlharq;j++) {
	    phy_vars_eNB->eNB_UE_stats[i].dlsch_NAK[j]=0;
	    phy_vars_eNB->eNB_UE_stats[i].dlsch_sliding_cnt=0;
	  }
	}
	else {
	  msg("ULSCH SDU (RX) %d bytes:",phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->TBS>>3);
	  for (j=0;j<phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->TBS>>3;j++)
	    msg("%x.",phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->b[j]);
	  msg("\n");
	  mac_xface->rx_sdu(phy_vars_eNB->Mod_id,phy_vars_eNB->ulsch_eNB[i]->rnti,phy_vars_eNB->ulsch_eNB[i]->harq_processes[harq_pid]->b);
	}

	if (phy_vars_eNB->ulsch_eNB[i]->RRCConnRequest_flag == 1) {
	  msg("[PHY_PROCEDURES_eNB] frame %d, slot %d, subframe %d, eNB %d: received ULSCH (RRCConnectionRequest) for UE %d, ret = %d, CQI CRC Status %d\n",mac_xface->frame, last_slot, last_slot>>1, phy_vars_eNB->eNB_UE_stats[i].sector, i, ret, phy_vars_eNB->ulsch_eNB[i]->cqi_crc_status);  
	}
	else {
	  //#ifdef DEBUG_PHY
	  debug_msg("[PHY_PROCEDURES_eNB] frame %d, slot %d, subframe %d, sect %d: received ULSCH harq_pid %d for UE %d, ret = %d, CQI CRC Status %d, ulsch_errors %d/%d\n",mac_xface->frame, last_slot, last_slot>>1, phy_vars_eNB->eNB_UE_stats[i].sector, harq_pid, i, ret, phy_vars_eNB->ulsch_eNB[i]->cqi_crc_status,phy_vars_eNB->eNB_UE_stats[i].ulsch_errors[harq_pid],phy_vars_eNB->eNB_UE_stats[i].ulsch_decoding_attempts[harq_pid][0]);
	  
	  // process HARQ feedback
	  msg("[PHY_PROCEDURES_eNB] eNB %d Processing HARQ feedback for UE %d\n",phy_vars_eNB->Mod_id,i);
	  //#endif
	  process_HARQ_feedback(i,last_slot>>1,phy_vars_eNB);

	}
      }  // ulsch not in error
    }
  } // loop i=0 ... NUMBER_OF_UE_MAX-1
#endif //OPENAIR2
#ifdef EMOS
    phy_procedures_emos_eNB_RX(last_slot);
#endif

}
  
//#undef DEBUG_PHY

void phy_procedures_eNB_lte(unsigned char last_slot, unsigned char next_slot,PHY_VARS_eNB *phy_vars_eNB,u8 abstraction_flag) {

  if (((phy_vars_eNB->lte_frame_parms.frame_type == 1)&&(subframe_select(&phy_vars_eNB->lte_frame_parms,next_slot>>1)==SF_DL))||
      (phy_vars_eNB->lte_frame_parms.frame_type == 0)){
#ifdef DEBUG_PHY
      msg("[PHY_PROCEDURES_LTE] Frame% d: Calling phy_procedures_eNB_TX(%d)\n",mac_xface->frame, next_slot);
#endif
      phy_procedures_eNB_TX(next_slot,phy_vars_eNB,abstraction_flag);
  }
  if (((phy_vars_eNB->lte_frame_parms.frame_type == 1 )&&(subframe_select(&phy_vars_eNB->lte_frame_parms,last_slot>>1)==SF_UL))||
      (phy_vars_eNB->lte_frame_parms.frame_type == 0)){
#ifdef DEBUG_PHY
      msg("[PHY_PROCEDURES_LTE] Frame% d: Calling phy_procedures_eNB_RX(%d)\n",mac_xface->frame, last_slot);
#endif
      phy_procedures_eNB_RX(last_slot,phy_vars_eNB,abstraction_flag);
  }
  if ((subframe_select(&phy_vars_eNB->lte_frame_parms,next_slot>>1)==SF_S)) {
#ifdef DEBUG_PHY
      msg("[PHY_PROCEDURES_LTE] Frame% d: Calling phy_procedures_eNB_S_TX(%d)\n",mac_xface->frame, next_slot);
#endif
      phy_procedures_eNB_S_TX(next_slot,phy_vars_eNB,abstraction_flag);
  }
  if ((phy_vars_eNB->lte_frame_parms.frame_type==1) && (subframe_select(&phy_vars_eNB->lte_frame_parms,last_slot>>1))==SF_S) {
#ifdef DEBUG_PHY
      msg("[PHY_PROCEDURES_LTE] Frame% d: Calling phy_procedures_eNB_S_RX(%d)\n",mac_xface->frame, last_slot);
#endif
      phy_procedures_eNB_S_RX(last_slot,phy_vars_eNB,abstraction_flag);
  }
}

