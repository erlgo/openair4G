%Channel Capacity for LTE Mode 6 Beamforming
%clear all;
%clc;
%close all;
%warning off all
%format('long');
%load 'estimates_UE.mat';
%load 'SISO.mat';
%M1=4;% QAM on first antenna
[x, y, z] = size(estimates_UE(1).channel);

z = z/2;
h = zeros(1,z);

MultiAntenna_Rx = 1; % 0 for Single antenna receivers
% 1 for Multi Antenna receivers(2 antennas here)

%precoding matrix values q
qi = [1 -1 1i -1i];

chcap_beamforming_4Qam_eNB1 = zeros(1,length(h), length(estimates_UE));
chcap_beamforming_4Qam_eNB2 = zeros(1,length(h), length(estimates_UE));
chcap_beamforming_4Qam_eNB3 = zeros(1,length(h), length(estimates_UE));

chcap_beamforming_16Qam_eNB1 = zeros(1,length(h), length(estimates_UE));
chcap_beamforming_16Qam_eNB2 = zeros(1,length(h), length(estimates_UE));
chcap_beamforming_16Qam_eNB3 = zeros(1,length(h), length(estimates_UE));

chcap_beamforming_64Qam_eNB1 = zeros(1,length(h), length(estimates_UE));
chcap_beamforming_64Qam_eNB2 = zeros(1,length(h), length(estimates_UE));
chcap_beamforming_64Qam_eNB3 = zeros(1,length(h), length(estimates_UE));

SNR_eNB1 = zeros(1,length(h), length(estimates_UE));
SNR_eNB2 = zeros(1,length(h), length(estimates_UE));
SNR_eNB3 = zeros(1,length(h), length(estimates_UE));

for est=1:length(estimates_UE)
    
    N0 = double(estimates_UE(est).phy_measurements(1).n0_power(1));
    
    if MultiAntenna_Rx == 1
        
        % Channels for Sector 1
        h_eNB1_h11 = double(estimates_UE(est).channel(1,1,1:2:end)) + 1j*double(estimates_UE(est).channel(1,1,2:2:end));
        h_eNB1_h12 = double(estimates_UE(est).channel(1,2,1:2:end)) + 1j*double(estimates_UE(est).channel(1,2,2:2:end));
        h_eNB1_h21 = double(estimates_UE(est).channel(1,3,1:2:end)) + 1j*double(estimates_UE(est).channel(1,3,2:2:end));
        h_eNB1_h22 = double(estimates_UE(est).channel(1,4,1:2:end)) + 1j*double(estimates_UE(est).channel(1,4,2:2:end));
        % Channels for Sector 2
        h_eNB2_h11 = double(estimates_UE(est).channel(2,1,1:2:end)) + 1j*double(estimates_UE(est).channel(2,1,2:2:end));
        h_eNB2_h12 = double(estimates_UE(est).channel(2,2,1:2:end)) + 1j*double(estimates_UE(est).channel(2,2,2:2:end));
        h_eNB2_h21 = double(estimates_UE(est).channel(2,3,1:2:end)) + 1j*double(estimates_UE(est).channel(2,3,2:2:end));
        h_eNB2_h22 = double(estimates_UE(est).channel(2,4,1:2:end)) + 1j*double(estimates_UE(est).channel(2,4,2:2:end));
        % Channels for Sector 3
        h_eNB3_h11 = double(estimates_UE(est).channel(3,1,1:2:end)) + 1j*double(estimates_UE(est).channel(3,1,2:2:end));
        h_eNB3_h12 = double(estimates_UE(est).channel(3,2,1:2:end)) + 1j*double(estimates_UE(est).channel(3,2,2:2:end));
        h_eNB3_h21 = double(estimates_UE(est).channel(3,3,1:2:end)) + 1j*double(estimates_UE(est).channel(3,3,2:2:end));
        h_eNB3_h22 = double(estimates_UE(est).channel(3,4,1:2:end)) + 1j*double(estimates_UE(est).channel(3,4,2:2:end));
        
        for qq=1:4
            
            Rx1(qq).eNB1 =  abs(h_eNB1_h11 + qi(qq)*h_eNB1_h21).^2;
            Rx2(qq).eNB1 =  abs(h_eNB1_h12 + qi(qq)*h_eNB1_h22).^2;
            
            Rx1(qq).eNB2 =  abs(h_eNB2_h11 + qi(qq)*h_eNB2_h21).^2;
            Rx2(qq).eNB2 =  abs(h_eNB2_h12 + qi(qq)*h_eNB2_h22).^2;
            
            Rx1(qq).eNB3 =  abs(h_eNB3_h11 + qi(qq)*h_eNB3_h21).^2;
            Rx2(qq).eNB3 =  abs(h_eNB3_h12 + qi(qq)*h_eNB3_h22).^2;
        end
        
        for i=1:4
            SNR_eNB1_q(i).snr = 10*log10((Rx1(i).eNB1(:) + Rx2(i).eNB1(:))/N0);
            sum_eNB1_q(i) = sum(SNR_eNB1_q(i).snr);
            SNR_eNB2_q(i).snr = 10*log10((Rx1(i).eNB2(:) + Rx2(i).eNB2(:))/N0);
            sum_eNB2_q(i) = sum(SNR_eNB2_q(i).snr);
            SNR_eNB3_q(i).snr = 10*log10((Rx1(i).eNB3(:) + Rx2(i).eNB3(:))/N0);
            sum_eNB3_q(i) = sum(SNR_eNB3_q(i).snr);
        end
        
        [value_eNB1, index_eNB1] = max(sum_eNB1_q);
        [value_eNB2, index_eNB2] = max(sum_eNB2_q);
        [value_eNB3, index_eNB3] = max(sum_eNB3_q);
        
        %SNR calculation for
        SNR_eNB1(1, :, est) = SNR_eNB1_q(index_eNB1).snr;
        SNR_eNB2(1, :, est) = SNR_eNB2_q(index_eNB2).snr;
        SNR_eNB3(1, :, est) = SNR_eNB3_q(index_eNB3).snr;
        
    else
        % Channels for Sector 1
        h_eNB1_h11 = double(estimates_UE(est).channel(1,1,1:2:end)) + 1j*double(estimates_UE(est).channel(1,1,2:2:end));
        h_eNB1_h21 = double(estimates_UE(est).channel(1,3,1:2:end)) + 1j*double(estimates_UE(est).channel(1,3,2:2:end));
        
        % Channels for Sector 2
        h_eNB2_h11 = double(estimates_UE(est).channel(2,1,1:2:end)) + 1j*double(estimates_UE(est).channel(2,1,2:2:end));
        h_eNB2_h21 = double(estimates_UE(est).channel(2,3,1:2:end)) + 1j*double(estimates_UE(est).channel(2,3,2:2:end));
        
        % Channels for Sector 3
        h_eNB3_h11 = double(estimates_UE(est).channel(3,1,1:2:end)) + 1j*double(estimates_UE(est).channel(3,1,2:2:end));
        h_eNB3_h21 = double(estimates_UE(est).channel(3,3,1:2:end)) + 1j*double(estimates_UE(est).channel(3,3,2:2:end));
        
        
        for qq=1:4
            
            Rx1(qq).eNB1 =  abs(h_eNB1_h11 + qi(qq)*h_eNB1_h21).^2;
            %Rx2(qq).eNB1 =  abs(h_eNB1_h12 + qi(qq)*h_eNB1_h22).^2;
            
            Rx1(qq).eNB2 =  abs(h_eNB2_h11 + qi(qq)*h_eNB2_h21).^2;
            %Rx2(qq).eNB2 =  abs(h_eNB2_h12 + qi(qq)*h_eNB2_h22).^2;
            
            Rx1(qq).eNB3 =  abs(h_eNB3_h11 + qi(qq)*h_eNB3_h21).^2;
            %Rx2(qq).eNB3 =  abs(h_eNB3_h12 + qi(qq)*h_eNB3_h22).^2;
        end
        
        for i=1:4
            SNR_eNB1_q(i).snr = 10*log10((Rx1(i).eNB1(:))/N0);
            sum_eNB1_q(i) = sum(SNR_eNB1_q(i).snr);
            SNR_eNB2_q(i).snr = 10*log10((Rx1(i).eNB2(:))/N0);
            sum_eNB2_q(i) = sum(SNR_eNB2_q(i).snr);
            SNR_eNB3_q(i).snr = 10*log10((Rx1(i).eNB3(:))/N0);
            sum_eNB3_q(i) = sum(SNR_eNB3_q(i).snr);
        end
        
        [value_eNB1, index_eNB1] = max(sum_eNB1_q);
        [value_eNB2, index_eNB2] = max(sum_eNB2_q);
        [value_eNB3, index_eNB3] = max(sum_eNB3_q);
        
        %SNR calculation for
        SNR_eNB1(1, :, est) = SNR_eNB1_q(index_eNB1).snr;
        SNR_eNB2(1, :, est) = SNR_eNB2_q(index_eNB2).snr;
        SNR_eNB3(1, :, est) = SNR_eNB3_q(index_eNB3).snr;
        
    end
    
    nan_in_SNR = isnan(SNR_eNB1);
    nan_in_SNR = find(nan_in_SNR == 1);
    SNR_eNB1(nan_in_SNR) = 0;
    
    nan_in_SNR = isnan(SNR_eNB2);
    nan_in_SNR = find(nan_in_SNR == 1);
    SNR_eNB2(nan_in_SNR) = 0;
    
    nan_in_SNR = isnan(SNR_eNB3);
    nan_in_SNR = find(nan_in_SNR == 1);
    SNR_eNB3(nan_in_SNR) = 0;
    
    
    while (min(SNR_eNB1(1, :, est)) < -20)
        [value, index]  = min(SNR_eNB1(1, :, est));
        SNR_eNB1(1, index, est) = -20;
        continue
        
    end
    
    while (max(SNR_eNB1(1, :, est)) > 40)
        [value, index]  = max(SNR_eNB1(1, :, est));
        SNR_eNB1(1, index, est) = 40;
        continue
        
    end
    
    while (min(SNR_eNB2(1, :, est)) < -20)
        [value, index]  = min(SNR_eNB2(1, :, est));
        SNR_eNB2(1, index, est) = -20;
        continue
        
    end
    while (max(SNR_eNB2(1, :, est)) > 40)
        [value, index]  = max(SNR_eNB2(1, :, est));
        SNR_eNB2(1, index, est) = 40;
        continue
        
    end
    
    while (min(SNR_eNB3(1, :, est)) < -20)
        [value, index]  = min(SNR_eNB3(1, :, est));
        SNR_eNB3(1, index, est) = -20;
        continue
        
    end
    while (max(SNR_eNB3(1, :, est)) > 40)
        [value, index]  = max(SNR_eNB3(1, :, est));
        SNR_eNB3(1, index, est) = 40;
        continue
        
    end
    
    % version 1: using tabulated values
    for const=1:3
        if const ==1
            for c=1:length(h)
                chcap_beamforming_4Qam_eNB1(1, c, est) = c_siso_4Qam(find(SNR == round(SNR_eNB1(1, c, est))));
                chcap_beamforming_4Qam_eNB2(1, c, est) = c_siso_4Qam(find(SNR == round(SNR_eNB2(1, c, est))));
                chcap_beamforming_4Qam_eNB3(1, c, est) = c_siso_4Qam(find(SNR == round(SNR_eNB3(1, c, est))));
            end
        else if const==2
                for c=1:length(h)
                    chcap_beamforming_16Qam_eNB1(1, c, est) = c_siso_16Qam(find(SNR == round(SNR_eNB1(1, c, est))));
                    chcap_beamforming_16Qam_eNB2(1, c, est) = c_siso_16Qam(find(SNR == round(SNR_eNB2(1, c, est))));
                    chcap_beamforming_16Qam_eNB3(1, c, est) = c_siso_16Qam(find(SNR == round(SNR_eNB3(1, c, est))));
                end
            else
                for c=1:length(h)
                    chcap_beamforming_64Qam_eNB1(1, c, est) = c_siso_64Qam(find(SNR == round(SNR_eNB1(1, c, est))));
                    chcap_beamforming_64Qam_eNB2(1, c, est) = c_siso_64Qam(find(SNR == round(SNR_eNB2(1, c, est))));
                    chcap_beamforming_64Qam_eNB3(1, c, est) = c_siso_64Qam(find(SNR == round(SNR_eNB3(1, c, est))));
                end
            end
        end
    end
end

if MultiAntenna_Rx == 1
    save 'chcap_beamforming_MultiAntennaUE_Measurements.mat' 'chcap_beamforming*' 'SNR_eNB1' 'SNR_eNB2' 'SNR_eNB3'
else
    save 'chcap_beamforming_SingleAntennaUE_Measurements' 'chcap_beamforming*' 'SNR_eNB1' 'SNR_eNB2' 'SNR_eNB3'
end

% % version 2: do Monte Carlo simulation for every channel
% N0 = double(estimates_UE(1).phy_measurements(1).n0_power(1));
%
% N = 1000;  % No of noise realizations
% for v=1:length(h)
%     seed(1) = 13579;
%     seed(2) = 24680;
%     rand('state', seed(1));
%     randn('state', seed(2));
%     %N0=10^(-SNR(v)/10);
%     %P=10^(SNR(v)/10);
%     sigmasq_x1=1;
%     [stream1]=sqrt(sigmasq_x1)*mapping(M1);     %x1   1x648
%     logsum_siso = 0;
%     %h=sqrt(1/2).*(randn(1,1)+sqrt(-1)*randn(1,1));
%     %h = estimates_UE(1).channel(1,1,i+1)
%     for s1=1:M1
%         x1=stream1(s1);
%         for k=1:N
%             z=sqrt(N0)*sqrt(1/2).*(randn(1,1)+sqrt(-1)*randn(1,1));
%             alpha_siso=h(v);
%             y_siso=alpha_siso*x1+z;
%             den_siso=exp((-1/N0)*norm(y_siso-alpha_siso*x1)^2);
%             neu_siso=0;
%             for g=1:M1;
%                 x1_dd=stream1(g);
%                 neu_siso=neu_siso+exp((-1/N0)*norm(y_siso-alpha_siso*x1_dd)^2);
%             end
%             logsum_siso=logsum_siso+log2(neu_siso/den_siso);
%         end
%     end
%     chcap_siso_single_stream(v)=log2(M1)-logsum_siso/(M1*N);  % For SISO
% end









