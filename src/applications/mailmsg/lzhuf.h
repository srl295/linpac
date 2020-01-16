/**************************************************************
      lzhuf encoder/decoder class by cz6don, based on:

  ***********************************************************
        lzhuf.c
        written by Haruyasu Yoshizaki 11/20/1988
        some minor changes 4/6/1989
        comments translated by Haruhiko Okumura 4/7/1989
**************************************************************/

class LZHufPacker
{
protected:
	short int crc_fputc(short int c);
	short int crc_fgetc();
	void InitTree(void);
	void InsertNode(short int r);
	void DeleteNode(short int p);
	short int GetBit(void);
	short int GetByte(void);
	void Putcode(short int l, unsigned short c);
	void StartHuff(void);
	void reconst(void);
	void update(short int c);
	void EncodeChar(unsigned short c);
	void EncodePosition(unsigned short c);
	void EncodeEnd(void);
	int  DecodeChar(void);
	int  DecodePosition(void);

	unsigned short *freq;   /* frequency table */
	unsigned short int *prnt;   /* pointers to parent nodes, except for the */
                        /* elements [T..T + N_CHAR - 1] which are used to get */
                        /* the positions of leaves corresponding to the codes. */
	unsigned short int *son;             /* pointers to child nodes (son[], son[] + 1) */

	unsigned short getbuf, putbuf;
	unsigned char getlen, putlen;

unsigned char   *text_buf;
unsigned short int       match_position, match_length, *lson, *rson, *dad;

unsigned char *dataOut, *input;
unsigned short crc;
unsigned long length, inPos, outPos, outLen;
short cleanup;

public:

	LZHufPacker(short clu = 1);
	~LZHufPacker();

	void Encode(unsigned long len, void *data);	// compression
	void Decode(unsigned long len, void *data, unsigned long plen);	// decompression
	unsigned short GetCRC() {return crc;};
	void* GetData() {return (void*)dataOut;};
	long GetLength() {return length;};
};
