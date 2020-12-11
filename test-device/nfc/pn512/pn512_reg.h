
#ifndef __PN512_REG_H__
#define __PN512_REG_H__

/*******************************************
FM175xx 寄存器定义
********************************************/
#define 	CommandReg					0x01
#define 	ComIEnReg					0x02
#define 	DivIEnReg					0x03
#define 	ComIrqReg					0x04
#define 	DivIrqReg					0x05
#define 	ErrorReg					0x06
#define 	Status1Reg					0x07
#define 	Status2Reg					0x08
#define 	FIFODataReg					0x09
#define 	FIFOLevelReg				0x0A
#define 	WaterLevelReg				0x0B
#define 	ControlReg					0x0C
#define 	BitFramingReg				0x0D
#define 	CollReg						0x0E
#define 	ModeReg						0x11
#define 	TxModeReg					0x12
#define 	RxModeReg					0x13
#define 	TxControlReg				0x14
#define 	TxAutoReg					0x15
#define 	TxSelReg					0x16
#define 	RxSelReg					0x17
#define 	RxThresholdReg				0x18
#define 	DemodReg					0x19
#define 	MfTxReg						0x1C
#define 	MfRxReg						0x1D
#define 	SerialSpeedReg				0x1F
#define 	CRCMSBReg					0x21
#define 	CRCLSBReg					0x22
#define 	ModWidthReg					0x24
#define 	GsNOffReg					0x23
#define 	TxBitPhaseReg				0x25
#define 	RFCfgReg					0x26
#define 	GsNReg						0x27
#define 	CWGsPReg					0x28
#define 	ModGsPReg					0x29
#define 	TModeReg					0x2A
#define 	TPrescalerReg				0x2B
#define 	TReloadMSBReg				0x2C
#define 	TReloadLSBReg				0x2D
#define 	TCounterValMSBReg 			0x2E
#define 	TCounterValLSBReg			0x2F
#define 	TestSel1Reg					0x31
#define 	TestSel2Reg					0x32
#define 	TestPinEnReg				0x33
#define 	TestPinValueReg 			0x34
#define 	TestBusReg					0x35
#define 	AutoTestReg					0x36
#define 	VersionReg					0x37
#define 	AnalogTestReg				0x38
#define 	TestDAC1Reg					0x39
#define 	TestDAC2Reg					0x3A
#define		TestADCReg					0x3B

#define 	Idle						0x00
#define 	Mem							0x01
#define 	RandomID					0x02
#define 	CalcCRC						0x03
#define 	Transmit					0x04
#define 	NoCmdChange					0x07
#define 	Receive						0x08
#define 	Transceive					0x0C
#define 	MFAuthent					0x0E
#define 	SoftReset					0x0F

#define 	LPCD_GAIN_INDEX				3		//LPCD校准Gain的序号
#define		JBIT_BIT_CTRL_CLR			0x00	//Lpcd register Bit ctrl clear bit
#define		BFL_JBIT_CALIBRATE_VMID_EN	0x01	//lPCD test mode
#define		JBIT_CARD_IN_IRQ			0x01	//irq of card in
#define		JBIT_LPCD_EN				0x01	//enble LPCD
#define		JREG_LPCD_CTRL1				0x01	//Lpcd Ctrl register1
#define		JREG_LPCD_CTRL2				0x02	//Lpcd Ctrl register2
#define		JBIT_LPCD_RSTN				0X02	//lpcd reset
#define 	JREG_COMMIEN         		0x02    //Contains Communication interrupt enable bits andbit for Interrupt inversion.
#define  	JREG_DIVIEN         		0x03    //Contains RfOn, RfOff, CRC and Mode Interrupt enable and bit to switch Interrupt pin to PushPull mode.
#define		JREG_LPCD_CTRL3				0x03	//Lpcd Ctrl register3
#define		JREG_LPCD_CTRL4				0x04	//Lpcd Ctrl register4
#define		JBIT_LPCD_CALIBRATE_EN		0x04	//into lpcd calibra mode
#define		JBIT_CALIBRATE_IRQ			0x04	//irq of calib end
#define		JREG_LPCD_BIAS_CURRENT		0x05	//Lpcd bias current register
#define		JREG_LPCD_ADC_REFERECE		0x06	//Lpcd adc reference register
#define		JREG_LPCD_T1CFG				0x07	//T1Cfg[3:0] register  
#define		JREG_LPCD_T2CFG				0x08	//T2Cfg[4:0] register 
#define		JBIT_LPCD_SENSE_1			0x08	//Compare times 1 or 3
#define		JREG_LPCD_T3CFG				0x09	//T2Cfg[4:0] register 
#define		JREG_LPCD_VMIDBD_CFG		0x0A	//VmidBdCfg[4:0] register 
#define		JREG_LPCD_AUTO_WUP_CFG		0x0B	//Auto_Wup_Cfg register 
#define		JREG_LPCD_MISC				0x1C	//LPCD Misc Register 
#define		JREG_LPCD_ADC_RESULT_L		0x0C	//ADCResult[5:0] Register 
#define		JREG_LPCD_ADC_RESULT_H		0x0D	//ADCResult[7:6] Register 
#define		JREG_LPCD_THRESHOLD_MIN_L	0x0E	//LpcdThreshold_L[5:0] Register
#define		JREG_LPCD_THRESHOLD_MIN_H	0x0F	//LpcdThreshold_L[7:6] Register  
#define		JREG_EXT_REG_ENTRANCE		0x0F	//ext register entrance
#define		JBIT_LPCD_IE				0x10	//Enable LPCD IE
#define		JREG_LPCD_THRESHOLD_MAX_L	0x10	//LpcdThreshold_H[5:0] Register 
#define		JREG_LPCD_THRESHOLD_MAX_H	0x11	//LpcdThreshold_H[7:6] Register
#define		JREG_LPCD_IRQ				0x12	//LpcdStatus Register  
#define		JBIT_BIT_CTRL_SET			0x20	//Lpcd register Bit ctrl set bit
#define  	JREG_GSN             		0x27    //Contains the conductance and the modulation settings for the N-MOS transistor during active modulation (no load modulation setting!).
#define  	JREG_TESTPINEN       		0x33    //Test register 
#define  	JREG_VERSION       			0x37    //Contains the product number and the version .
#define		JBIT_EXT_REG_WR_ADDR		0X40	//wrire address cycle
#define		BIT7               			0x80
#define		JBIT_EXT_REG_RD_ADDR		0X80	//read address cycle
#define		JBIT_EXT_REG_WR_DATA		0XC0	//write data cycle






#endif




