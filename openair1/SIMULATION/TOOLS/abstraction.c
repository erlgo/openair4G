/*******************************************************************************
    OpenAirInterface 
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is 
   included in this distribution in the file called "COPYING". If not, 
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr
  
  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/
#include <math.h>
#include <cblas.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "PHY/TOOLS/defs.h"
#include "defs.h"

// NEW code with lookup table for sin/cos based on delay profile (TO BE TESTED)

double **cos_lut=NULL,**sin_lut=NULL;


//#if 1



void init_freq_channel(channel_desc_t *desc,uint16_t nb_rb,int16_t n_samples) {


  double delta_f,freq;  // 90 kHz spacing
  double delay;
  int16_t f;
  uint8_t l;


  cos_lut = (double **)malloc(n_samples*sizeof(double*));
  sin_lut = (double **)malloc(n_samples*sizeof(double*));
  


  delta_f = nb_rb*180000/(n_samples-1);

  for (f=-(n_samples>>1);f<(n_samples>>1);f++) {
    freq=delta_f*(double)f*1e-6;// due to the fact that delays is in mus

    cos_lut[f+(n_samples>>1)] = (double *)malloc((int)desc->nb_taps*sizeof(double));
    sin_lut[f+(n_samples>>1)] = (double *)malloc((int)desc->nb_taps*sizeof(double));


    for (l=0;l<(int)desc->nb_taps;l++) {
      if (desc->nb_taps==1) 
	delay = desc->delays[l];
      else
	delay = desc->delays[l]+NB_SAMPLES_CHANNEL_OFFSET/desc->BW;

      cos_lut[f+(n_samples>>1)][l] = cos(2*M_PI*freq*delay);
      sin_lut[f+(n_samples>>1)][l] = sin(2*M_PI*freq*delay);
      //printf("values cos:%d, sin:%d\n", cos_lut[f][l], sin_lut[f][l]);
      
    }
  }
}

void freq_channel(channel_desc_t *desc,uint16_t nb_rb,int16_t n_samples) {


  int16_t f,f2,d;
  uint8_t aarx,aatx,l;
  double *clut,*slut;
  static int freq_channel_init=0;
  static int n_samples_max=0;

 // printf("no of samples:%d,",n_samples);
 // printf("no of taps:%d,",(int)desc->nb_taps);

  if (freq_channel_init == 0) {
    // we are initializing the lut for the largets possible n_samples=12*nb_rb+1 
    // if called with n_samples<12*nb_rb+1, we decimate the lut
    n_samples_max=12*nb_rb+1;
    init_freq_channel(desc,nb_rb,n_samples_max);
    freq_channel_init=1;
  }
  d=n_samples_max/n_samples;
   
  start_meas(&desc->interp_freq);
  for (f=-n_samples_max/2,f2=-n_samples/2;f<n_samples_max/2;f+=d,f2++) {
	clut = cos_lut[n_samples_max/2+f];
        slut = sin_lut[n_samples_max/2+f];
        
      for (aarx=0;aarx<desc->nb_rx;aarx++) {
	for (aatx=0;aatx<desc->nb_tx;aatx++) {
	  desc->chF[aarx+(aatx*desc->nb_rx)][n_samples/2+f2].x=0.0;
	  desc->chF[aarx+(aatx*desc->nb_rx)][n_samples/2+f2].y=0.0;
	  for (l=0;l<(int)desc->nb_taps;l++) {
		
	    desc->chF[aarx+(aatx*desc->nb_rx)][n_samples/2+f2].x+=(desc->a[l][aarx+(aatx*desc->nb_rx)].x*clut[l]+
								  desc->a[l][aarx+(aatx*desc->nb_rx)].y*slut[l]);
	    desc->chF[aarx+(aatx*desc->nb_rx)][n_samples/2+f2].y+=(-desc->a[l][aarx+(aatx*desc->nb_rx)].x*slut[l]+
								  desc->a[l][aarx+(aatx*desc->nb_rx)].y*clut[l]);
	  }
	}
      }
  }
  stop_meas(&desc->interp_freq);
}

double compute_pbch_sinr(channel_desc_t *desc,
			 channel_desc_t *desc_i1, 
			 channel_desc_t *desc_i2,
			 double snr_dB,double snr_i1_dB,
			 double snr_i2_dB,
			 uint16_t nb_rb) {

  double avg_sinr,snr=pow(10.0,.1*snr_dB),snr_i1=pow(10.0,.1*snr_i1_dB),snr_i2=pow(10.0,.1*snr_i2_dB);
  uint16_t f;
  uint8_t aarx,aatx;
  double S;
  struct complex S_i1;
  struct complex S_i2;

  avg_sinr=0.0;
  //  printf("nb_rb %d\n",nb_rb);
  for (f=(nb_rb-6);f<(nb_rb+6);f++) {
    S = 0.0;
    S_i1.x =0.0;
    S_i1.y =0.0;
    S_i2.x =0.0;
    S_i2.y =0.0;
    for (aarx=0;aarx<desc->nb_rx;aarx++) {
      for (aatx=0;aatx<desc->nb_tx;aatx++) {
	S    += (desc->chF[aarx+(aatx*desc->nb_rx)][f].x*desc->chF[aarx+(aatx*desc->nb_rx)][f].x + 
	         desc->chF[aarx+(aatx*desc->nb_rx)][f].y*desc->chF[aarx+(aatx*desc->nb_rx)][f].y);
	//	printf("%d %d chF[%d] => (%f,%f)\n",aarx,aatx,f,desc->chF[aarx+(aatx*desc->nb_rx)][f].x,desc->chF[aarx+(aatx*desc->nb_rx)][f].y);
	       
	if (desc_i1) {
	  S_i1.x += (desc->chF[aarx+(aatx*desc->nb_rx)][f].x*desc_i1->chF[aarx+(aatx*desc->nb_rx)][f].x + 
		     desc->chF[aarx+(aatx*desc->nb_rx)][f].y*desc_i1->chF[aarx+(aatx*desc->nb_rx)][f].y);
	  S_i1.y += (desc->chF[aarx+(aatx*desc->nb_rx)][f].x*desc_i1->chF[aarx+(aatx*desc->nb_rx)][f].y - 
		     desc->chF[aarx+(aatx*desc->nb_rx)][f].y*desc_i1->chF[aarx+(aatx*desc->nb_rx)][f].x);
	}
	if (desc_i2) {
	  S_i2.x += (desc->chF[aarx+(aatx*desc->nb_rx)][f].x*desc_i2->chF[aarx+(aatx*desc->nb_rx)][f].x + 
		     desc->chF[aarx+(aatx*desc->nb_rx)][f].y*desc_i2->chF[aarx+(aatx*desc->nb_rx)][f].y);
	  S_i2.y += (desc->chF[aarx+(aatx*desc->nb_rx)][f].x*desc_i2->chF[aarx+(aatx*desc->nb_rx)][f].y - 
		     desc->chF[aarx+(aatx*desc->nb_rx)][f].y*desc_i2->chF[aarx+(aatx*desc->nb_rx)][f].x);
	}
      }
    } 
    //    printf("snr %f f %d : S %f, S_i1 %f, S_i2 %f\n",snr,f-nb_rb,S,snr_i1*sqrt(S_i1.x*S_i1.x + S_i1.y*S_i1.y),snr_i2*sqrt(S_i2.x*S_i2.x + S_i2.y*S_i2.y));
    avg_sinr += (snr*S/(desc->nb_tx+snr_i1*sqrt(S_i1.x*S_i1.x + S_i1.y*S_i1.y)+snr_i2*sqrt(S_i2.x*S_i2.x + S_i2.y*S_i2.y)));
  }
  //  printf("avg_sinr %f (%f,%f,%f)\n",avg_sinr/12.0,snr,snr_i1,snr_i2);
  return(10*log10(avg_sinr/12.0));
}
 

double compute_sinr(channel_desc_t *desc,
		    channel_desc_t *desc_i1, 
		    channel_desc_t *desc_i2,
		    double snr_dB,double snr_i1_dB,
		    double snr_i2_dB,
		    uint16_t nb_rb) {

  double avg_sinr,snr=pow(10.0,.1*snr_dB),snr_i1=pow(10.0,.1*snr_i1_dB),snr_i2=pow(10.0,.1*snr_i2_dB);
  uint16_t f;
  uint8_t aarx,aatx;
  double S;
  struct complex S_i1;
  struct complex S_i2;

  DevAssert( nb_rb > 0 );
  
  avg_sinr=0.0;
  //  printf("nb_rb %d\n",nb_rb);
  for (f=0;f<2*nb_rb;f++) {
    S = 0.0;
    S_i1.x =0.0;
    S_i1.y =0.0;
    S_i2.x =0.0;
    S_i2.y =0.0;
    for (aarx=0;aarx<desc->nb_rx;aarx++) {
      for (aatx=0;aatx<desc->nb_tx;aatx++) {
	S    += (desc->chF[aarx+(aatx*desc->nb_rx)][f].x*desc->chF[aarx+(aatx*desc->nb_rx)][f].x + 
	         desc->chF[aarx+(aatx*desc->nb_rx)][f].y*desc->chF[aarx+(aatx*desc->nb_rx)][f].y);
	if (desc_i1) {
	  S_i1.x += (desc->chF[aarx+(aatx*desc->nb_rx)][f].x*desc_i1->chF[aarx+(aatx*desc->nb_rx)][f].x + 
		     desc->chF[aarx+(aatx*desc->nb_rx)][f].y*desc_i1->chF[aarx+(aatx*desc->nb_rx)][f].y);
	  S_i1.y += (desc->chF[aarx+(aatx*desc->nb_rx)][f].x*desc_i1->chF[aarx+(aatx*desc->nb_rx)][f].y - 
		     desc->chF[aarx+(aatx*desc->nb_rx)][f].y*desc_i1->chF[aarx+(aatx*desc->nb_rx)][f].x);
	}
	if (desc_i2) {
	  S_i2.x += (desc->chF[aarx+(aatx*desc->nb_rx)][f].x*desc_i2->chF[aarx+(aatx*desc->nb_rx)][f].x + 
		     desc->chF[aarx+(aatx*desc->nb_rx)][f].y*desc_i2->chF[aarx+(aatx*desc->nb_rx)][f].y);
	  S_i2.y += (desc->chF[aarx+(aatx*desc->nb_rx)][f].x*desc_i2->chF[aarx+(aatx*desc->nb_rx)][f].y - 
		     desc->chF[aarx+(aatx*desc->nb_rx)][f].y*desc_i2->chF[aarx+(aatx*desc->nb_rx)][f].x);
	}
      }
    }
    //        printf("f %d : S %f, S_i1 %f, S_i2 %f\n",f-nb_rb,snr*S,snr_i1*sqrt(S_i1.x*S_i1.x + S_i1.y*S_i1.y),snr_i2*sqrt(S_i2.x*S_i2.x + S_i2.y*S_i2.y));
    avg_sinr += (snr*S/(desc->nb_tx+snr_i1*sqrt(S_i1.x*S_i1.x + S_i1.y*S_i1.y)+snr_i2*sqrt(S_i2.x*S_i2.x + S_i2.y*S_i2.y)));
  }
  //  printf("avg_sinr %f (%f,%f,%f)\n",avg_sinr/12.0,snr,snr_i1,snr_i2);
  return(10*log10(avg_sinr/(nb_rb*2)));
}

int pbch_polynomial_degree=6;
double pbch_awgn_polynomial[7]={-7.2926e-05, -2.8749e-03, -4.5064e-02, -3.5301e-01, -1.4655e+00, -3.6282e+00, -6.6907e+00};

void load_pbch_desc(FILE *pbch_file_fd) {

  int i, ret;
  char dummy[25];

  ret = fscanf(pbch_file_fd,"%d",&pbch_polynomial_degree);
  if (ret < 0) {
    printf("fscanf failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  if (pbch_polynomial_degree>6) {
    printf("Illegal degree for pbch interpolation polynomial %d\n",pbch_polynomial_degree);
    exit(-1);
  }

  printf("PBCH polynomial : ");

  for (i=0;i<=pbch_polynomial_degree;i++) {
    ret = fscanf(pbch_file_fd,"%s",dummy);
    if (ret < 0) {
      printf("fscanf failed: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }
    pbch_awgn_polynomial[i] = strtod(dummy,NULL);
    printf("%f ",pbch_awgn_polynomial[i]);
  }
  printf("\n");
} 

double pbch_bler(double sinr) {

  int i;
  double log10_bler=pbch_awgn_polynomial[pbch_polynomial_degree];
  double sinrpow=sinr;
  double bler=0.0;
  //  printf("log10bler %f\n",log10_bler);
  if (sinr<-10.0)
    bler= 1.0;
  else if (sinr>=0.0)
    bler=0.0;
  else  {
    for (i=1;i<=pbch_polynomial_degree;i++) {
      //    printf("sinrpow %f\n",sinrpow);
      log10_bler += (pbch_awgn_polynomial[pbch_polynomial_degree-i]*sinrpow);
      sinrpow *= sinr;
      //    printf("log10bler %f\n",log10_bler);
    }
    bler = pow(10.0,log10_bler);
  }
  //printf ("sinr %f bler %f\n",sinr,bler);
  return(bler);

}
