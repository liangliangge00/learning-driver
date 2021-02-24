#define LOG_TAG "PN512_IO"
//#define LOG_NDDEBUG 0

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <cassert>
#include <cstdlib>

#include <cutils/log.h>
#include <cutils/properties.h>
#include <cutils/str_parms.h>

#include "pn512_reg.h"
#include "pn512_io.h"

PN512_IO::PN512_IO(int fd_pn512)
{
	fd = fd_pn512;
}

unsigned char PN512_IO::reg_read(unsigned char address)
{
	struct pn512_reg reg;

	reg.address = address;
	ioctl(fd, PN512_IOC_REGREAD, &reg);
	
	return reg.value;
}

void PN512_IO::reg_write(unsigned char address, unsigned char value)
{
	struct pn512_reg reg;

	reg.address = address;
	reg.value = value;
	ioctl(fd, PN512_IOC_REGWRITE, &reg);
}

void PN512_IO::reg_setbit(unsigned char address, unsigned char mask)
{
	struct pn512_reg reg;

	reg.address = address;
	reg.value = mask;
	ioctl(fd, PN512_IOC_REGSBIT, &reg);
}

void PN512_IO::reg_clearbit(unsigned char address, unsigned char mask)
{
	struct pn512_reg reg;

	reg.address = address;
	reg.value = mask;
	ioctl(fd, PN512_IOC_REGCBIT, &reg);
}

int PN512_IO::fifo_clear(void)
{
	//清除FIFO缓冲
	reg_setbit(FIFOLevelReg, 0x80);
	if (reg_read(FIFOLevelReg) == 0)
		return 0;
	else
		return -1;
}

void PN512_IO::set_timer(unsigned int time_ms)
{
	ioctl(fd, PN512_IOC_SETTIMER, time_ms);
}

void PN512_IO::set_rfmode(unsigned char mode)
{
	ioctl(fd, PN512_IOC_RFMODE, mode);
}

void PN512_IO::set_isotype(unsigned char type)
{
	ioctl(fd, PN512_IOC_ISOTYPE, type);
}

void PN512_IO::hw_reset(void)
{
	ioctl(fd, PN512_IOC_HWRESET, 0);
}

void PN512_IO::sw_reset(void)
{
	ioctl(fd, PN512_IOC_SWRESET, 0);
}

void PN512_IO::hw_powerdown(int enable)
{
	ioctl(fd, PN512_IOC_SETPWD, enable);
}

int PN512_IO::transfer(unsigned char command, unsigned int timeout,
	unsigned char *tx_buf, unsigned char tx_len,
	unsigned char *rx_buf, unsigned char rx_size, unsigned int *rx_bitlen)
{
	int ret, byte;

	tx_data.command = command;
	if (tx_len > FIFO_MAX_SIZE)
		tx_data.length = FIFO_MAX_SIZE;
	else
		tx_data.length = tx_len;

	memcpy(tx_data.buf, tx_buf, tx_data.length);

	ioctl(fd, PN512_IOC_TXDATA, &tx_data);
	
	set_timer(timeout);
	ret = ioctl(fd, PN512_IOC_RXDATA, &rx_data);

	if (ret > 0)
	{
	//	ALOGD("%s [%d]:rx %d: %x ... %x\n", __func__, ret, rx_data.bitlen, rx_data.buf[0], rx_data.buf[ret - 1]);
		
		if(rx_bitlen != NULL)
			*rx_bitlen = rx_data.bitlen;

		if (ret > rx_size)
			ret = rx_size;
		if(rx_buf != NULL)
		memcpy(rx_buf, rx_data.buf, ret);
	}
	//else
	//	ALOGD("%s [%d]\n", __func__, ret);

	return ret;
}

int PN512_IO::dev_test(void)
{
	unsigned char reg;
	
	//启动读写器模式
	reg_write(ControlReg, 0x10);
	reg = reg_read(ControlReg);

	// CWGsN = 0xF; ModGsN = 0x4
	reg_write(GsNReg, 0xF0 | 0x04);
	reg = reg_read(GsNReg);
	if (reg != 0xF4)
	{
		ALOGE("%s: NFC device test error", __func__);
		return -1;
	}

	return 0;
}

void PN512_IO::lpcd_enable(int enable)
{
	ioctl(fd, PN512_IOC_LPCD, enable); 
}

void PN512_IO::lpcd_calibration(void)
{
	ioctl(fd, PN512_IOC_STARTCAL, 0);
}

void PN512_IO::irq_enable(int enable)
{
	if(enable)
		ioctl(fd, PN512_IOC_IRQENABLE, 0);
	else
		ioctl(fd, PN512_IOC_IRQDISABLE, 0);
	
}
