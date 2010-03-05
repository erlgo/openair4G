#include "PHY/defs.h"
#include "PHY/extern.h"
#include "PHY/CODING/defs.h"
#include "PHY/CODING/extern.h"
#include "PHY/LTE_TRANSPORT/defs.h"
#include "defs.h"

#define OFDMA_ULSCH

#define DEBUG_ULSCH_MODULATION

void ulsch_modulation(mod_sym_t **txdataF,
		      short amp,
		      unsigned int subframe,
		      LTE_DL_FRAME_PARMS *frame_parms,
		      LTE_UE_ULSCH_t *ulsch) {

#ifdef IFFT_FPGA
  unsigned char qam64_table_offset = 0;
  unsigned char qam16_table_offset = 0;
  unsigned char qpsk_table_offset = 0; 
#else
  unsigned char qam64_table_offset_re = 0;
  unsigned char qam64_table_offset_im = 0;
  unsigned char qam16_table_offset_re = 0;
  unsigned char qam16_table_offset_im = 0;
  short gain_lin_QPSK;
#endif
  short re_offset,re_offset0,i,Msymb,j,nsymb,Msc_PUSCH,l;
  unsigned char harq_pid = subframe2harq_pid_tdd(frame_parms->tdd_config,subframe);
  unsigned char Q_m = get_Qm(ulsch->harq_processes[harq_pid]->mcs);
  mod_sym_t *txptr;
  unsigned int symbol_offset;
  unsigned short first_rb = ulsch->harq_processes[harq_pid]->first_rb;
  unsigned short nb_rb = ulsch->harq_processes[harq_pid]->nb_rb,G;


  G = ulsch->harq_processes[harq_pid]->nb_rb * (12 * Q_m) * (ulsch->Nsymb_pusch);
#ifdef DEBUG_ULSCH_MODULATION
  msg("ulsch_modulation.c: Doing modulation for G=%d bits, nb_rb %d, Q_m %d, Nsymb_pusch %d\n",
      G,ulsch->harq_processes[harq_pid]->nb_rb,Q_m, ulsch->Nsymb_pusch);
#endif
  // scrambling (Note the placeholding bits are handled in ulsch_coding.c directly!)
  for (i=0;i<G;i++) {
    ulsch->b_tilde[i] = ulsch->h[i];  // put Gold scrambling here later
  }

#ifndef IFFT_FPGA
  gain_lin_QPSK = (short)((amp*ONE_OVER_SQRT2_Q15)>>15);  
#endif

  // Modulation

  Msymb = G/Q_m;
  for (i=0,j=0;i<Msymb;i++,j+=Q_m) {

    switch (Q_m) {

    case 2:

#ifndef IFFT_FPGA
      ((short*)&ulsch->d[i])[0] = (ulsch->b_tilde[j] == 0)  ? (-gain_lin_QPSK) : gain_lin_QPSK;
      ((short*)&ulsch->d[i])[1] = (ulsch->b_tilde[j+1] == 0)? (-gain_lin_QPSK) : gain_lin_QPSK;
#else
      qpsk_table_offset = 1;
      if (ulsch->b_tilde[j] == 1)
	qpsk_table_offset+=1;
      if (ulsch->b_tilde[j+1] == 1) 
	qpsk_table_offset+=2;
      
      ulsch->d[i] = (mod_sym_t) qpsk_table_offset;
#endif    

      break;

    case 4:
#ifndef IFFT_FPGA
      qam16_table_offset_re = 0;
      if (ulsch->b_tilde[j] == 1)
	qam16_table_offset_re+=2;

      if (ulsch->b_tilde[j+1] == 1)
	qam16_table_offset_re+=1;
      
      
      qam16_table_offset_im = 0;
      if (ulsch->b_tilde[j+2] == 1)
	qam16_table_offset_im+=2;

      if (ulsch->b_tilde[j+3] == 1)
	qam16_table_offset_im+=1;

      
      ((short*)&ulsch->d[i])[0]=(short)(((int)amp*qam16_table[qam16_table_offset_re])>>15);
      ((short*)&ulsch->d[i])[1]=(short)(((int)amp*qam16_table[qam16_table_offset_im])>>15);

#else
      qam16_table_offset = 5;
      if (ulsch->b_tilde[j] == 1)
	qam16_table_offset+=2;

      if (ulsch->b_tilde[j+1] == 1)
	qam16_table_offset+=1;

      if (ulsch->b_tilde[j+2] == 1)
	qam16_table_offset+=8;

      if (ulsch->b_tilde[j+3] == 1)
	qam16_table_offset+=4;

      
      ulsch->d[i] = (mod_sym_t) qam16_table_offset;
      
#endif
      
      break;
     
    case 6:

#ifndef IFFT_FPGA
      qam64_table_offset_re = 0;
      if (ulsch->b_tilde[j] == 1)
	qam64_table_offset_re+=4;
      
      if (ulsch->b_tilde[j+1] == 1)
	qam64_table_offset_re+=2;
      
      if (ulsch->b_tilde[j+2] == 1)
	qam64_table_offset_re+=1;
      
      qam64_table_offset_im = 0;
      if (ulsch->b_tilde[j+3] == 1)
	qam64_table_offset_im+=4;
      
      if (ulsch->b_tilde[j+4] == 1)
	qam64_table_offset_im+=2;
      
      if (ulsch->b_tilde[j+5] == 1)
	qam64_table_offset_im+=1;
      
      
      ((short*)&ulsch->d[i])[0]=(short)(((int)amp*qam64_table[qam64_table_offset_re])>>15);
      ((short*)&ulsch->d[i])[1]=(short)(((int)amp*qam64_table[qam64_table_offset_im])>>15);

#else
      
      qam64_table_offset = 21;
      if (ulsch->b_tilde[j] == 1)
	qam64_table_offset+=4;
      
      if (ulsch->b_tilde[j+1] == 1)
	qam64_table_offset+=2;
      
      if (ulsch->b_tilde[j+2] == 1)
	qam64_table_offset+=1;
      
      
      
      if (ulsch->b_tilde[j+3] == 1)
	qam64_table_offset+=32;
      
      if (ulsch->b_tilde[j+4] == 1)
	qam64_table_offset+=16;
      
      if (ulsch->b_tilde[j+5] == 1)
	qam64_table_offset+=8;
      
      
      ulsch->d[i] = (mod_sym_t) qam64_table_offset;
      
#endif //IFFT_FPGA
      break;

    }
  }
    // Transform Precoding

#ifdef OFDMA_ULSCH
  for (i=0;i<Msymb;i++) {
    ulsch->z[i] = ulsch->d[i]; 

        if (i<16)
          printf("z[%d]  -> %d\n",i,ulsch->z[i]);
  }
#else

#endif

  // Mapping
  nsymb = (frame_parms->Ncp==0) ? 14:12;
  Msc_PUSCH = ulsch->harq_processes[harq_pid]->nb_rb*12;

#ifdef IFFT_FPGA

  for (j=0,l=0;l<(nsymb-1);l++) {
    re_offset = ulsch->harq_processes[harq_pid]->first_rb*12 + frame_parms->N_RB_DL*12/2;
    if (re_offset > (frame_parms->N_RB_DL*12))
      re_offset -= (frame_parms->N_RB_DL*12);

    symbol_offset = (unsigned int)frame_parms->N_RB_DL*12*(l+(subframe*nsymb));
    txptr = &txdataF[0][symbol_offset];
    printf("symbol %d: symbol_offset %d\n",l,symbol_offset);
    if (((frame_parms->Ncp == 0) && ((l==3) || (l==10)))||
	((frame_parms->Ncp == 1) && ((l==2) || (l==8)))) {
    }
    else {
      printf("copying %d REs\n",Msc_PUSCH);
      for (i=0;i<Msc_PUSCH;i++,j++) {
	txptr[re_offset++] = ulsch->z[j];
	if (re_offset==(frame_parms->N_RB_DL*12))
	  re_offset = 0;                                 
      }
    }
  }
 
#else
  re_offset0 = frame_parms->first_carrier_offset + (ulsch->harq_processes[harq_pid]->first_rb*12);
  if (re_offset0>frame_parms->ofdm_symbol_size) {
    re_offset0 -= frame_parms->ofdm_symbol_size;
    re_offset0++;
  }
  for (j=0,l=0;l<(nsymb-1);l++) {
    re_offset = re_offset0;
    symbol_offset = (unsigned int)frame_parms->ofdm_symbol_size*(l+(subframe*nsymb));
    printf("symbol %d (subframe %d): symbol_offset %d\n",l,subframe,symbol_offset);
    txptr = &txdataF[0][symbol_offset];
    if (((frame_parms->Ncp == 0) && ((l==3) || (l==10)))||
	((frame_parms->Ncp == 1) && ((l==2) || (l==8)))) {
    }
    // Skip reference symbols
    else {
      printf("copying %d REs\n",Msc_PUSCH);
      for (i=0;i<Msc_PUSCH;i++,j++) {
	txptr[re_offset++] = ulsch->z[j];
	if (re_offset==frame_parms->ofdm_symbol_size)
	  re_offset = 1;                                 // Skip DC
      }
    }
  }
#endif
  

}

