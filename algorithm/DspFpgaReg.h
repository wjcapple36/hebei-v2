/*
 **************************************************************************************
 *                        桂林聚联科技有限公司
 *
 *  文件描述： DSP与FPGA通信时的寄存器定义
 *
 *  文件名  ： DspFpgaReg.h
 *  创建者  ： 彭怀敏
 *  创建日期： 2011-12-8 17:42:23
 *  当前版本： v1.0
 *
 **************************************************************************************
*/
#ifndef _DSPFPGAREG_H__
#define _DSPFPGAREG_H__


/*
 **************************************************************************************
 *   OTDR通道数据起始地址
 **************************************************************************************
*/
#define	FPGA_OTDR_BASE_ADDR	                0xB0000000    // FPGA映射到DSP的地址空间起始地址

// FPGA寄存器地址偏移
#define	FPGA_CTRL_FLASH_PAGE_OFFSET			0x3FFFC	        // Flash页面
#define FPGA_OTDR_CTRL_LAMBDA_OFFSET        0x3FFF8	        // 激光器波长
#define	FPGA_OTDR_CTRL_PULSE_WIDTH_OFFSET	0x3FFF4	        // 脉冲宽度
#define	FPGA_OTDR_CTRL_SAMPLE_RATE_OFFSET	0x3FFF0	        // 采样时钟选择
#define	FPGA_OTDR_CTRL_SAMPLE_NUM_OFFSET	0x3FFEC	        // 采样点数
#define	FPGA_OTDR_CTRL_ACC_COUNT_OFFSET		0x3FFE8	        // 累加次数
#define	FPGA_OTDR_CTRL_START_ADC_OFFSET		0x3FFE4	        // 启动ADC
#define	FPGA_OTDR_CTRL_SAMPLE_DONE_OFFSET	0x3FFE0	        // FPGA采样完成
#define	FPGA_DSP_RESET_OFFSET				0x3FFDC	        // DSP Reset
#define	FPGA_CTRL_POWER_CTRL_OFFSET         0x3FFD8	        // FPGA功率控制
#define	FPGA_CTRL_RCV_OFFSET                0x3FFD4	        // 接收机通道选择
#define	FPGA_CTRL_LASER_ON_OFF              0x3FFD0	        // 激光器发光控制
#define	FPGA_OTDR_CTRL_CHAN_OFFSET			0x3FFCC	        // OTDR通道选择
#define FPGA_CTRL_ECHO_DSP_OFFSET           0x3FFC8         // FPGA回应
#define FPGA_CTRL_APDV_OFFSET               0x3FFC4         // APD电压控制
#define FPGA_OTDR_CODE_OFFSET               0x3FFC0         // 码元地址

/*
 **************************************************************************************
 *   寄存器操作宏定义
 **************************************************************************************
*/
// 设置激光器波长
#define	OTDR_SET_LAMBDA(lambda)     (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_OTDR_CTRL_LAMBDA_OFFSET) = lambda)

// 设置脉冲宽度
#define	OTDR_SET_PULSE_WIDTH(pw)    (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_OTDR_CTRL_PULSE_WIDTH_OFFSET) = pw)

// 设置采样率
#define	OTDR_SET_SAMPLE_RATE(clk)   (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_OTDR_CTRL_SAMPLE_RATE_OFFSET) = clk)

// 设置采样点数
#define	OTDR_SET_SAMPLE_NUM(num)    (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_OTDR_CTRL_SAMPLE_NUM_OFFSET) = num)

// 设置累加次数
#define	OTDR_SET_ACC_COUNT(cc)      (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_OTDR_CTRL_ACC_COUNT_OFFSET) = cc)

// 启动FPGA采集数据
#define	FPGA_START_ADC()            (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_OTDR_CTRL_START_ADC_OFFSET) = 0xABCD)

// DSP复位
#define DSP_RESET()                 (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_DSP_RESET_OFFSET) = 0xDEAD)

// 设置功率控制
#define	OTDR_SET_POWER(p)           (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_CTRL_POWER_CTRL_OFFSET) = p)

// 设置通道
#define	OTDR_SET_CHAN(chan)         (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_OTDR_CTRL_CHAN_OFFSET) = chan)

// FPGA回应
#define FPGA_ECHO_WRITE(v)          (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_CTRL_ECHO_DSP_OFFSET) = v)
#define FPGA_ECHO_READ()            (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_CTRL_ECHO_DSP_OFFSET))

// APD电压控制
#define APDV_HIGH       2   // 小脉宽
#define APDV_LOW        1   // 大脉宽
#define OTDR_SET_APDV(v)            (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_CTRL_APDV_OFFSET) = v)

// 写入序列码元
#define OTDR_CODE_WRITE(v)          (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_OTDR_CODE_OFFSET) = v)

// 设置接收机通道       S1 S2 S3 S4
#define RCV_LOW         0x6     // 低增益   640ns≤脉宽≤20us拼接曲线低增益时采用
#define RCV_MID_LOW     0x7     // 中低增益   脉宽≤40ns时采用
#define RCV_MIDDLE      0xA     // 中增益   640ns≤脉宽≤2560ns未拼接时采用     5120ns≤脉宽≤20us拼接曲线低增益时采用
#define RCV_HIGH        0xC     // 高增益   80ns≤脉宽≤320ns未拼接时采用       640ns≤脉宽≤2560ns拼接曲线高增益时采用
#define RCV_VERY_HIGH   0xD     // 超高增益 5120ns≤脉宽≤20us拼接曲线超高增益时采用

#define R_6 RCV_LOW
#define R_7 RCV_MID_LOW
#define R_A RCV_MIDDLE
#define R_C RCV_HIGH
#define R_D RCV_VERY_HIGH

#define OTDR_SET_RECEIVER(rcv)      (*(volatile Uint32 *)(FPGA_OTDR_BASE_ADDR + FPGA_CTRL_RCV_OFFSET) = rcv)

// 数据采集完成
#define	SAMPLE_DONE		0x1234
#define	SAMPLE_DEAD		0xDEAD

// 采样时钟定义
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
