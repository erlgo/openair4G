#ifndef __PHY_VARS_H__
#define __PHY_VARS_H__

#include "PHY/types.h"
#include "PHY/defs.h" 

#ifndef USER_MODE
unsigned int RX_DMA_BUFFER[4][NB_ANTENNAS_RX];
unsigned int TX_DMA_BUFFER[4][NB_ANTENNAS_TX];
#endif

//PHY_CONFIG *PHY_config;

#include "PHY/TOOLS/twiddle64.h"
#include "PHY/TOOLS/twiddle128.h"
#include "PHY/TOOLS/twiddle256.h"
#include "PHY/TOOLS/twiddle512.h"
#include "PHY/TOOLS/twiddle1024.h"
#include "PHY/TOOLS/twiddle2048.h"
#include "PHY/TOOLS/twiddle4096.h"
#include "PHY/TOOLS/twiddle32768.h"

#ifdef OPENAIR_LTE
#include "PHY/LTE_REFSIG/primary_synch.h"
s16 *primary_synch0_time;
s16 *primary_synch1_time;
s16 *primary_synch2_time;
#endif

#include "PHY/CODING/vars.h"

//PHY_VARS *PHY_vars;
PHY_VARS_UE **PHY_vars_UE_g;
PHY_VARS_eNB **PHY_vars_eNB_g;
LTE_DL_FRAME_PARMS *lte_frame_parms_g;

short *twiddle_ifft,*twiddle_fft,*twiddle_fft_times4,*twiddle_ifft_times4,*twiddle_fft_half,*twiddle_ifft_half;

#ifndef OPENAIR_LTE
CHBCH_RX_t rx_mode = ML;
#endif //OPENAIR_LTE

unsigned short rev[1024],rev_times4[4096],rev_half[512];
u16 rev256[256],rev512[512],rev1024[1024],rev4096[4096],rev2048[2048];

#ifdef OPENAIR_LTE
char mode_string[4][20] = {"NOT SYNCHED","PRACH","RAR","PUSCH"};
#include "PHY/LTE_TRANSPORT/vars.h"
#endif

#include "PHY/CODING/scrambler.h"

#ifdef USER_MODE
#include "SIMULATION/ETH_TRANSPORT/vars.h"
#endif

#ifndef OPENAIR2
unsigned char NB_eNB_INST=0;
unsigned char NB_UE_INST=0;
unsigned char NB_INST=0;
#endif


//extern  channel_desc_t *eNB2UE[NUMBER_OF_eNB_MAX][NUMBER_OF_UE_MAX];
//extern  double ABS_SINR_eff_BLER_table[MCS_COUNT][9][9];
//extern  double ABS_beta[MCS_COUNT];odi
double sinr_bler_map[MCS_COUNT][2][16];


//for MU-MIMO abstraction using MIESM
//this 2D arrarays contains SINR, MI and RBIR in rows 1, 2, and 3 respectively
double MI_map_4qam[3][162];
double MI_map_16qam[3][197];
double MI_map_64qam[3][227];

double beta1_dlsch_MI[6][MCS_COUNT] = { {1.17, 1.11, 1.04, 1.05, 1.09, 1.09, 1.10, 1.10, 1.09, 1.03, 1.04, 1.04, 1.05, 1.07, 1.07, 1.06, 1, 1,1,1,1,1,1}, {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,1,1,1,1,1,1}, {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,1,1,1,1,1,1},  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,1,1,1,1,1,1},{1.808065416202085, 1.754544945430673,1.902272019362616, 1.790054645392961, 1.563204092967629, 1.585258289348813, 1.579349443720310, 1.570650121437345, 1.545055626608596, 1.362229442426877, 1.85, 1.79, 1.65, 1.54, 1.46, 1.39, 1.33, 1,1,1,1,1,1,1},{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,1,1,1,1,1,1}};

double beta2_dlsch_MI[6][MCS_COUNT] = { {1.17, 1.11, 1.04, 1.05, 1.09, 1.09, 1.10, 1.10, 1.09, 1.03, 1.04, 1.04, 1.05, 1.07, 1.07, 1.06, 1, 1,1,1,1,1,1,1}, {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,1,1,1,1,1,1}, {1,1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,1,1,1,1,1},  {1,1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,1,1,1,1,1},{1.079518113138858, 1.105622953570353, 1.031337449900606, 1.073342032668810, 1.242636589110353, 1.255054927783647, 1.291463834317768, 1.317048698347491, 1.354485054187984, 0.338534029291017, 1.85, 1.79, 1.65, 1.54, 1.46, 1.39, 1.33,1, 1,1,1,1,1,1},{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,1,1,1,1,1,1}};

double beta1_dlsch[6][MCS_COUNT] = { {2.3814, 0.4956, 0.5273, 1.1708, 0.8014, 0.7889, 0.8111, 0.8139, 0.8124, 0.8479, 1.9280, 1.9664, 2.3857, 2.5147, 2.4511, 3.0158, 2.8643, 5.3013, 5.8594, 6.5372, 7.8073, 7.8030, 7.5295, 7.1320}, {0.5146, 0.5549, 0.7405, 0.6913, 0.7349, 0.7000, 0.7539, 0.7955, 0.8074, 0.7760, 1.8150, 1.6561, 1.9280, 2.3563, 2.6699, 2.3086, 3.1601, 4.5316, 5.2870, 6.0983, 6.5635, 7.7024, 9.9592, 6.6173}, {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,1,1,1,1,1,1}, {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,1,1,1,1,1}, {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,1,1,1,1,1}, {0.4445, 0.5918, 0.7118, 0.7115, 0.7284, 0.7202, 0.7117, 0.8111, 0.8239, 0.7907, 1.8456, 1.8144, 2.3830, 2.6634, 2.6129, 2.8127, 2.7372, 4.9424, 4.8763, 6.8413, 7.1493, 9.4180, 10.1230, 8.9613}};
double beta2_dlsch[6][MCS_COUNT] = { {2.3639, 0.4952, 0.5207, 1.1572, 0.8026, 0.7864, 0.7996, 0.8034, 0.8200, 0.8367, 1.8701, 1.9212, 2.2947, 2.4472, 2.4091, 2.9479, 2.8973, 5.0591, 5.5134, 6.1483, 7.2166, 7.5177, 7.5704, 7.2248}, {0.5113, 0.5600, 0.7359, 0.6860, 0.7344, 0.6902, 0.7315, 0.7660, 0.7817, 0.7315, 1.7268, 1.5912, 1.8519, 2.2115, 2.4580, 2.1879, 2.9015, 4.1543, 4.6986, 5.3193, 5.6319, 6.5640, 8.2421, 5.6393}, {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,1,1,1,1,1,1}, {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,1,1,1,1,1}, {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,1,1,1,1,1}, {0.4398, 0.5823, 0.7094, 0.7043, 0.7282, 0.7041, 0.6979, 0.7762, 0.7871, 0.7469, 1.7752, 1.7443, 2.2266, 2.4767, 2.4146, 2.6040, 2.5708, 4.4488, 4.4944, 5.9630, 6.3740, 8.1097, 8.4210, 7.8027}};


#endif /*__PHY_VARS_H__ */
