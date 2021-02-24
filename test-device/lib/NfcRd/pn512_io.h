#ifndef __PN512_IO_H__
#define __PN512_IO_H__

#include <linux/ioctl.h>
#include <linux/types.h>

#define FIFO_MAX_SIZE	64

struct pn512_reg
{
	unsigned char address;
	unsigned char value;
};

struct pn512_transfer_tx
{
	unsigned char		buf[FIFO_MAX_SIZE];
	unsigned char		length;
	unsigned char		command;
};

struct pn512_transfer_rx
{
	unsigned char		buf[FIFO_MAX_SIZE];
	unsigned int		bitlen;
};

#define	TYPE					'S' 
#define PN512_IOC_REGWRITE      _IOWR(TYPE, 0, unsigned int)
#define PN512_IOC_REGREAD		_IOWR(TYPE, 1, unsigned int)
#define PN512_IOC_REGSBIT		_IOWR(TYPE, 2, unsigned int)
#define PN512_IOC_REGCBIT		_IOWR(TYPE, 3, unsigned int)
#define PN512_IOC_TXDATA		_IOWR(TYPE, 4, unsigned int)
#define PN512_IOC_RXDATA		_IOWR(TYPE, 5, unsigned int)
#define PN512_IOC_IRQENABLE     _IOWR(TYPE, 6, unsigned int)
#define PN512_IOC_IRQDISABLE    _IOWR(TYPE, 7, unsigned int)
#define PN512_IOC_ISOTYPE		_IOWR(TYPE, 8, unsigned int)
#define PN512_IOC_RFMODE		_IOWR(TYPE, 9, unsigned int)
#define PN512_IOC_SETTIMER      _IOWR(TYPE, 10, unsigned int)
#define PN512_IOC_HWRESET		_IOWR(TYPE, 11, unsigned int)
#define PN512_IOC_SWRESET		_IOWR(TYPE, 12, unsigned int)
#define PN512_IOC_SETPWD		_IOWR(TYPE, 13, unsigned int)


//添加LPCD模式控制
#define PN512_IOC_LPCD			_IOWR(TYPE, 14, unsigned int)
#define PN512_IOC_STARTCAL		_IOWR(TYPE, 15, unsigned int)

#define	PN512_ISOTYPE_A			0
#define	PN512_ISOTYPE_B			1

#define	PN512_RFMODE_OFF		0
#define	PN512_RFMODE_TX1ON		1
#define	PN512_RFMODE_TX2ON		2
#define	PN512_RFMODE_ALLON		3

class PN512_IO
{
public:
	PN512_IO(int fd_pn512);

	int 	fd;
	
	unsigned char reg_read(unsigned char address);
	void reg_write(unsigned char address, unsigned char value);
	void reg_setbit(unsigned char address, unsigned char mask);
	void reg_clearbit(unsigned char address, unsigned char mask);

	int fifo_clear(void);

	void set_timer(unsigned int time_ms);
	void set_rfmode(unsigned char mode);
	void set_isotype(unsigned char type);
	void hw_reset(void);
	void sw_reset(void);
	void hw_powerdown(int enable);
	
	int transfer(unsigned char command, unsigned int timeout,
	unsigned char *tx_buf, unsigned char tx_len,
	unsigned char *rx_buf, unsigned char rx_size, unsigned int *rx_bitlen);

	int dev_test();
	
	void lpcd_calibration(void);
	void lpcd_enable(int enable);
	void irq_enable(int enable);	
	
private:
	struct pn512_transfer_rx	rx_data;
	struct pn512_transfer_tx	tx_data;	
};


#endif
