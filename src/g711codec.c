#include "stdio.h"
#include "g711codec.h"

static short seg_end[8] = {0xFF, 0x1FF, 0x3FF, 0x7FF,
			    0xFFF, 0x1FFF, 0x3FFF, 0x7FFF};

static int search(int val, short	*table, int	size)
{
	int	i;

	for (i = 0; i < size; i++) {
		if (val <= *table++)
			return (i);
	}
	return (size);
}

/*
* alaw2linear() - Convert an A-law value to 16-bit linear PCM
*
*/
static int alaw2linear( unsigned char a_val )
{
	int	t;
	int	seg;

	a_val ^= 0x55;

	t = (a_val & QUANT_MASK) << 4;
	seg = ( (unsigned)a_val & SEG_MASK ) >> SEG_SHIFT;
	switch (seg) 
	{
		case 0:
			t += 8;
			break;
		case 1:
			t += 0x108;
			break;
		default:
			t += 0x108;
			t <<= seg - 1;
	}
	return ((a_val & SIGN_BIT) ? t : -t);
}


/*
* ulaw2linear() - Convert a u-law value to 16-bit linear PCM
*
* First, a biased linear code is derived from the code word. An unbiased
* output can then be obtained by subtracting 33 from the biased code.
*
* Note that this function expects to be passed the complement of the
* original code word. This is in keeping with ISDN conventions.
*/
static int ulaw2linear(unsigned char u_val)
{
	int	t;

	/* Complement to obtain normal u-law value. */
	u_val = ~u_val;

	/*
	* Extract and bias the quantization bits. Then
	* shift up by the segment number and subtract out the bias.
	*/
	t = ((u_val & QUANT_MASK) << 3) + BIAS;
	t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;

	return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}


/*
 * linear2alaw() - Convert a 16-bit linear PCM value to 8-bit A-law
 *
 */
unsigned char linear2alaw(int pcm_val)	/* 2's complement (16-bit range) */
{
	int		mask;
	int		seg;
	unsigned char	aval;

	if (pcm_val >= 0) {
		mask = 0xD5;		/* sign (7th) bit = 1 */
	} else {
		mask = 0x55;		/* sign bit = 0 */
		pcm_val = -pcm_val - 8;
	}

	/* Convert the scaled magnitude to segment number. */
	seg = search(pcm_val, seg_end, 8);

	/* Combine the sign, segment, and quantization bits. */

	if (seg >= 8)		/* out of range, return maximum value. */
		return (0x7F ^ mask);
	else {
		aval = seg << SEG_SHIFT;
		if (seg < 2)
			aval |= (pcm_val >> 4) & QUANT_MASK;
		else
			aval |= (pcm_val >> (seg + 3)) & QUANT_MASK;
		return (aval ^ mask);
	}
}


/*
 * linear2ulaw() - Convert a linear PCM value to u-law
 *
 */
unsigned char linear2ulaw(int pcm_val)	/* 2's complement (16-bit range) */
{
	int		mask;
	int		seg;
	unsigned char	uval;

	/* Get the sign and the magnitude of the value. */
	if (pcm_val < 0) {
		pcm_val = BIAS - pcm_val;
		mask = 0x7F;
	} else {
		pcm_val += BIAS;
		mask = 0xFF;
	}

	/* Convert the scaled magnitude to segment number. */
	seg = search(pcm_val, seg_end, 8);

	/*
	 * Combine the sign, segment, quantization bits;
	 * and complement the code word.
	 */
	if (seg >= 8)		/* out of range, return maximum value. */
		return (0x7F ^ mask);
	else {
		uval = (seg << 4) | ((pcm_val >> (seg + 3)) & 0xF);
		return (uval ^ mask);
	}
}


int g711a_decode( short amp[], const unsigned char g711a_data[], int g711a_bytes )
{
	int i;
	int samples;
	unsigned char code;
	int sl;

	for ( samples = i = 0; ; )
	{
		if (i >= g711a_bytes)
			break;
		code = g711a_data[i++];

		sl = alaw2linear( code );

		amp[samples++] = (short) sl;
	}
	return samples*2;
}

int g711u_decode(short amp[], const unsigned char g711u_data[], int g711u_bytes)
{
	int i;
	int samples;
	unsigned char code;
	int sl;

	for (samples = i = 0;;)
	{
		if (i >= g711u_bytes)
			break;
		code = g711u_data[i++];

		sl = ulaw2linear(code);

		amp[samples++] = (short) sl;
	}
	return samples*2;
}

int g711a_encode(unsigned char g711_data[], const short amp[], int len)
{
    int i;

    for (i = 0;  i < len;  i++)
	{
        g711_data[i] = linear2alaw(amp[i]);
    }

    return len;
}

int g711u_encode(unsigned char g711_data[], const short amp[], int len)
{
    int i;

    for (i = 0;  i < len;  i++)
	{
        g711_data[i] = linear2ulaw(amp[i]);
    }

    return len;
}

/*
 * function: convert PCM audio format to g711 alaw/ulaw.(zqj)
 *	 InAudioData:	PCM data prepared for encoding to g711 alaw/ulaw.
 *   OutAudioData:	encoded g711 alaw/ulaw.
 *   DataLen:		PCM data size.
 *   reserve:		reserved param, no use.
 */

/*alaw*/
int PCM2G711a( char *InAudioData, char *OutAudioData, int DataLen, int reserve )
{	
	//check params.
	if( (NULL == InAudioData) && (NULL == OutAudioData) && (0 == DataLen) )
	{
		printf("Error, empty data or transmit failed, exit !\n");	
		return -1;
	}
	// printf("DataLen = %d, %s, %d\n", DataLen, __func__, __LINE__);

	int Retaen = 0; 
	//printf("G711a encode start......\n");
	Retaen = g711a_encode( (unsigned char *)OutAudioData, (short*)InAudioData, DataLen/2 );
	//printf("DataLen = %d ,Retaen = %d, %s, %d\n", DataLen,Retaen, __func__, __LINE__);

	return Retaen; //index successfully encoded data len.
}

/*ulaw*/
int PCM2G711u( char *InAudioData, char *OutAudioData, int DataLen, int reserve )
{	
	//check params.
	if( (NULL == InAudioData) && (NULL == OutAudioData) && (0 == DataLen) )
	{
		printf("Error, empty data or transmit failed, exit !\n");	
		return -1;
	}
	// printf("DataLen = %d, %s, %d\n", DataLen, __func__, __LINE__);

	int Retuen = 0; 
	printf("G711u encode start......\n");
	Retuen = g711u_encode( (unsigned char *)OutAudioData, (short*)InAudioData, DataLen/2 );
	printf("Retuen = %d, %s, %d\n", Retuen, __func__, __LINE__);

	return Retuen; 
}

/*
 * function: convert g711 alaw audio format to PCM.(zqj)
 *	 InAudioData:	g711 alaw data prepared for encoding to PCM.
 *   OutAudioData:	encoded PCM audio data.
 *   DataLen:		g711a data size.
 *   reserve:		reserved param, no use.
 */

/*alaw*/
int G711a2PCM( char *InAudioData, char *OutAudioData, int DataLen, int reserve )
{
	//check param.
	if( (NULL == InAudioData) && (NULL == OutAudioData) && (0 == DataLen) )
	{
		printf("Error, empty data or transmit failed, exit !\n");	
		return -1;
	}
	// printf("DataLen = %d, %s, %d\n", DataLen, __func__, __LINE__);

	int Retade = 0;
	// printf("G711a decode start......\n");
	Retade = g711a_decode( (short*)OutAudioData, (unsigned char *)InAudioData, DataLen );
	// printf("Retade = %d, %s, %d\n", Retade, __func__, __LINE__);

	return Retade;	//index successfully decoded data len.
}

/*ulaw*/
int G711u2PCM( char *InAudioData, char *OutAudioData, int DataLen, int reserve )
{
	//check param.
	if( (NULL == InAudioData) && (NULL == OutAudioData) && (0 == DataLen) )
	{
		printf("Error, empty data or transmit failed, exit !\n");	
		return -1;
	}
	// printf("DataLen = %d, %s, %d\n", DataLen, __func__, __LINE__);

	int Retude = 0;
	// printf("G711u decode start......\n");
	Retude = g711u_decode( (short*)OutAudioData, (unsigned char *)InAudioData, DataLen );
	// printf("Retude = %d, %s, %d\n", Retude, __func__, __LINE__);

	return Retude;	
}

