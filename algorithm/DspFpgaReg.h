/*
 **************************************************************************************
 *                        ���־����Ƽ����޹�˾
 *
 *  �ļ������� DSP��FPGAͨ��ʱ�ļĴ�������
 *
 *  �ļ���  �� DspFpgaReg.h
 *  ������  �� ����
 *  �������ڣ� 2011-12-8 17:42:23
 *  ��ǰ�汾�� v1.0
 *
 **************************************************************************************
*/
#ifndef _DSPFPGAREG_H__
#define _DSPFPGAREG_H__


/*
 **************************************************************************************
 *   OTDRͨ��������ʼ��ַ
 **************************************************************************************
*/
#define	FPGA_OTDR_BASE_ADDR	                0xB0000000    // FPGAӳ�䵽DSP�ĵ�ַ�ռ���ʼ��ַ

// FPGA�Ĵ�����ַƫ��
#define	FPGA_CTRL_FLASH_PAGE_OFFSET			0x3FFFC	        // Flashҳ��
#define FPGA_OTDR_CTRL_LAMBDA_OFFSET        0x3FFF8	        // ����������
#define	FPGA_OTDR_CTRL_PULSE_WIDTH_OFFSET	0x3FFF4	        // ������
#define	FPGA_OTDR_CTRL_SAMPLE_RATE_OFFSET	0x3FFF0	        // ����ʱ��ѡ��
#define	FPGA_OTDR_CTRL_SAMPLE_NUM_OFFSET	0x3FFEC	        // ��������
#define	FPGA_OTDR_CTRL_ACC_COUNT_OFFSET		0x3FFE8	        // �ۼӴ���
#define	FPGA_OTDR_CTRL_START_ADC_OFFSET		0x3FFE4	        // ����ADC
#define	FPGA_OTDR_CTRL_SAMPLE_DONE_OFFSET	0x3FFE0	        // FPGA�������
#define	FPGA_DSP_RESET_OFFSET				0x3FFDC	        // DSP Reset
#define	FPGA_CTRL_POWER_CTRL_OFFSET         0x3FFD8	        // FPGA���ʿ���
#define	FPGA_CTRL_RCV_OFFSET                0x3FFD4	        // ���ջ�ͨ��ѡ��
#define	FPGA_CTRL_LASER_ON_OFF              0x3FFD0	        // �������������
#define	FPGA_OTDR_CTRL_CHAN_OFFSET			0x3FFCC	        // OTDRͨ��ѡ��
#define FPGA_CTRL_ECHO_DSP_OFFSET           0x3FFC8         // FPGA��Ӧ
#define FPGA_CTRL_APDV_OFFSET               0x3FFC4         // APD��ѹ����
#define FPGA_OTDR_CODE_OFFSET               0x3FFC0         // ��Ԫ��ַ

/*
 **************************************************************************************
 *   �Ĵ��������궨��
 **************************************************************************************
*/
// ���ü���������
#define	OTDR_SET_LAMBDA(lambda)     (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_OTDR_CTRL_LAMBDA_OFFSET) = lambda)

// ����������
#define	OTDR_SET_PULSE_WIDTH(pw)    (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_OTDR_CTRL_PULSE_WIDTH_OFFSET) = pw)

// ���ò�����
#define	OTDR_SET_SAMPLE_RATE(clk)   (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_OTDR_CTRL_SAMPLE_RATE_OFFSET) = clk)

// ���ò�������
#define	OTDR_SET_SAMPLE_NUM(num)    (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_OTDR_CTRL_SAMPLE_NUM_OFFSET) = num)

// �����ۼӴ���
#define	OTDR_SET_ACC_COUNT(cc)      (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_OTDR_CTRL_ACC_COUNT_OFFSET) = cc)

// ����FPGA�ɼ�����
#define	FPGA_START_ADC()            (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_OTDR_CTRL_START_ADC_OFFSET) = 0xABCD)

// DSP��λ
#define DSP_RESET()                 (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_DSP_RESET_OFFSET) = 0xDEAD)

// ���ù��ʿ���
#define	OTDR_SET_POWER(p)           (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_CTRL_POWER_CTRL_OFFSET) = p)

// ����ͨ��
#define	OTDR_SET_CHAN(chan)         (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_OTDR_CTRL_CHAN_OFFSET) = chan)

// FPGA��Ӧ
#define FPGA_ECHO_WRITE(v)          (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_CTRL_ECHO_DSP_OFFSET) = v)
#define FPGA_ECHO_READ()            (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_CTRL_ECHO_DSP_OFFSET))

// APD��ѹ����
#define APDV_HIGH       2   // С����
#define APDV_LOW        1   // ������
#define OTDR_SET_APDV(v)            (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_CTRL_APDV_OFFSET) = v)

// д��������Ԫ
#define OTDR_CODE_WRITE(v)          (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_OTDR_CODE_OFFSET) = v)

// ���ý��ջ�ͨ��       S1 S2 S3 S4
#define RCV_LOW         0x6     // ������   640ns�������20usƴ�����ߵ�����ʱ����
#define RCV_MID_LOW     0x7     // �е�����   �����40nsʱ����
#define RCV_MIDDLE      0xA     // ������   640ns�������2560nsδƴ��ʱ����     5120ns�������20usƴ�����ߵ�����ʱ����
#define RCV_HIGH        0xC     // ������   80ns�������320nsδƴ��ʱ����       640ns�������2560nsƴ�����߸�����ʱ����
#define RCV_VERY_HIGH   0xD     // �������� 5120ns�������20usƴ�����߳�������ʱ����

#define R_6 RCV_LOW
#define R_7 RCV_MID_LOW
#define R_A RCV_MIDDLE
#define R_C RCV_HIGH
#define R_D RCV_VERY_HIGH

#define OTDR_SET_RECEIVER(rcv)      (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_CTRL_RCV_OFFSET) = rcv)

// ���ݲɼ����
#define	SAMPLE_DONE		0x1234
#define	SAMPLE_DEAD		0xDEAD

// ����ʱ�Ӷ���
#define	CLK_800MHz	800		// 800 MHz
#define	CLK_400MHz	400		// 400 MHz
#define	CLK_200MHz	200		// 200 MHz
#define	CLK_100MHz	100		// 100 MHz
#define	CLK_50MHz	50      // 50 MHz
#define	CLK_25MHz	25      // 25 MHz
#define CLK_12MHz   12      // 12.5 MHz
#define CLK_6MHz    6       // 6.25 MHz

#endif  /* _DSPFPGAREG_H__ */

/*
 **************************************************************************************
 *    End    of    File
 **************************************************************************************
*/
