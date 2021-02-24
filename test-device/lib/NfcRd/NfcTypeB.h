#ifndef __NFCTYPEB_H__
#define __NFCTYPEB_H__

class NfcTypeB
{
public:
	NfcTypeB(PN512_IO *dev);

	int Halt(unsigned char *card_sn);
	int WakeUp(unsigned char *card_sn, unsigned char *buf, unsigned int *rx_bitlen);
	int Request(unsigned int *bitlen, unsigned char *buff, unsigned char *card_sn);
	int Select(unsigned char *card_sn, unsigned int *bitlen, unsigned char *buff);
	int GetUID(unsigned int *bitlen, unsigned char *buff);
	
private:
	PN512_IO *nfc_dev;

};


#endif
