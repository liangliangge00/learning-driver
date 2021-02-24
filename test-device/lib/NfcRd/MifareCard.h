#ifndef __MIFARECARD_H__
#define __MIFARECARD_H__

class MifareCard
{
public:
	MifareCard(PN512_IO *dev);

	int Auth(unsigned char mode, unsigned char sector, unsigned char *mifare_key, unsigned char *card_uid);
	int BlockSet(unsigned char block, unsigned char *buff);
	int BlockRead(unsigned char block, unsigned char *buff);
	int BlockWrite(unsigned char block, unsigned char *buff);
	int BlockInc(unsigned char block, unsigned char *buff);
	int BlockDec(unsigned char block, unsigned char *buff);
	int Transfer(unsigned char block);
	int Restore(unsigned char block);

private:
	PN512_IO *nfc_dev;
};

#endif

