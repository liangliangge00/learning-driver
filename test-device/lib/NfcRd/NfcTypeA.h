#ifndef __NFCTYPEA_H__
#define __NFCTYPEA_H__

class NfcTypeA
{
public:
	NfcTypeA(PN512_IO *dev);
	
	int Request(unsigned char *pTagType);
	int Anticollision(unsigned char selcode, unsigned char *pSnr);
	int Select(unsigned char selcode, unsigned char *pSnr, unsigned char *pSak);
	int WakeUp(unsigned char *pTagType);
	int Halt(void);
	int CardActive(unsigned char *uid);

private:
	PN512_IO *nfc_dev;

};

#endif
