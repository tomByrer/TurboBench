/*
 * Range Coder portion derived from code by Eugene Shelwien (c) 2003.
 */

/*
 * Copyright (c) 2014 Genome Research Ltd.
 * Author(s): James Bonfield
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 * 
 *    2. Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 * 
 *    3. Neither the names Genome Research Ltd and Wellcome Trust Sanger
 *    Institute nor the names of its contributors may be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY GENOME RESEARCH LTD AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL GENOME RESEARCH
 * LTD OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Author: James Bonfield, Wellcome Trust Sanger Institute. 2014
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <float.h>

//#define TEST_MAIN

#define ABS(a) ((a)>0?(a):-(a))
#define BLK_SIZE 1000000

#define UNROLLED

/*-----------------------------------------------------------------------------
 * Arithmetic coder. clr.cdr from fqz_comp. Adapted from Eugene Shelwien's
 * original from his coders6c2 archive.
 */

/*
 * Note it is up to the calling code to ensure that no overruns on input and
 * output buffers occur.
 *
 * Call the input() and output() functions to set and query the current
 * buffer locations.
 */

static inline uint32_t i_log2(const uint32_t x) {
    uint32_t y;
    asm ( "\tbsr %1, %0\n"
	  : "=r"(y)
	  : "r" (x)
	  );

    // or 31-__builtin_clz(x);

    return y;
}

#define  TOP	   (1<<24)

#define TF_SHIFT 14
#define TOTFREQ (1<<TF_SHIFT)

typedef unsigned char uc;

typedef struct {
    uint64_t low;
    uint32_t range, code;

    uc *in_buf;
    uc *out_buf;
} RngCoder;

double RC_recip[65536];
void RC_init(void) {
    int i;
    for (i = 0; i < 65536; i++)
	RC_recip[i] = (1.0+DBL_EPSILON)/i;
}

//uint32_t RC_recip2b[65536][2];
uint32_t RC_recip3[65536];
void RC_init2(void) {
    int i;
    for (i = 1; i < 65536; i++) {
	//double d = 1.0/i;
	//d *= ((uint64_t)1)<<32;
	//d *= ((uint64_t)1)<<32;
	//d += 255;
	//RC_recip2b[i][0] = ((uint64_t)d)>>32;
	//RC_recip2b[i][1] = (uint64_t)d;

	RC_recip3[i] = 1+((1LL<<31) * ((1LL<<(i_log2(i)+1)) - i)) / i;
    }
}


static inline void RC_input(RngCoder *rc, char *in) {
    rc->out_buf = rc->in_buf = (uc *)in;
}

static inline void RC_output(RngCoder *rc, char *out) {
    rc->in_buf = rc->out_buf = (uc *)out;
}

static inline int RC_size_out(RngCoder *rc) {
    return rc->out_buf - rc->in_buf;
}

static inline int RC_size_in(RngCoder *rc) {
    return rc->in_buf - rc->out_buf;
}

static inline void RC_StartEncode(RngCoder *rc) { 
    rc->low=0;
    rc->range=(uint32_t)-1; 
}

static inline void RC_StartDecode(RngCoder *rc) { 
    int i;

    rc->code=0;
    rc->low=0;  
    rc->range=(uint32_t)-1;
    for (i = 0; i < 8; i++)
	rc->code = (rc->code<<8) | *rc->in_buf++;
}

static inline void RC_FinishEncode(RngCoder *rc) { 
    int i;
    for (i = 0; i < 8; i++) {
	*rc->out_buf++ = rc->low>>56;
	rc->low<<=8;
    }
}

static inline void RC_FinishDecode(RngCoder *rc) {}

static inline void RC_Encode(RngCoder *rc, uint32_t cumFreq, uint32_t freq) {
    //rc->low  += cumFreq * (rc->range/= totFreq);
    rc->low  += cumFreq * (rc->range >>= TF_SHIFT);
    rc->range*= freq;

    if (cumFreq + freq > TOTFREQ)
	abort();

    while (rc->range<TOP) {
	if ( (uc)((rc->low^(rc->low+rc->range))>>56) ) 
	    rc->range = (((uint32_t)rc->low | (TOP-1))-(uint32_t)rc->low);
	*rc->out_buf++ = rc->low>>56, rc->range<<=8, rc->low<<=8;
    }
}

static inline uint32_t RC_GetFreq(RngCoder *rc) {
    //fprintf(stderr, "%d / %d = %d\n", rc->code, (rc->range >> TF_SHIFT), rc->code / (rc->range >> TF_SHIFT));
#if 1
    return rc->code / (rc->range >>= TF_SHIFT); // 10.57
    //return rc->code * RC_recip[rc->range >>= TF_SHIFT]; // 10.99

#elif defined(RC_MDIV)
    // 9.94 elapsed
    rc->range >>= TF_SHIFT;
    uint32_t t = (rc->code * (uint64_t)RC_recip3[rc->range]) >> 31;

    return (((rc->code - t)>>1) + t) >> i_log2(rc->range);
    //return (rc->code + t) >> (1+i_log2(rc->range));

#endif

#if 0
    /* Multiply 32-bit rc->code by 64-bit RC_recip2[rc->range], which is
     * split into two 32-bit registers r1, r2.
     *
     * Alas it's typically slower than the division, but keeping it here
     * incase we get old systems that are slower dividing.
     */
    rc->range >>= TF_SHIFT;
    uint64_t a  = rc->code;
    uint64_t r1 = a * RC_recip2b[rc->range][0];
    uint64_t r2 = a * RC_recip2b[rc->range][1];
    return (r1 + (r2>>32)) >> 32;
#endif
}

static inline void RC_Decode(RngCoder *rc, uint32_t cumFreq, uint32_t freq) {
    uint32_t temp = cumFreq*rc->range;
    rc->low  += temp;
    rc->code -= temp;
    rc->range*= freq;
 
    while (rc->range<TOP) {
	if ( (uc)((rc->low^(rc->low+rc->range))>>56) ) 
	    rc->range = (((uint32_t)rc->low | (TOP-1))-(uint32_t)rc->low);
	rc->code = (rc->code<<8) | *rc->in_buf++, rc->range<<=8, rc->low<<=8;
    }
}

//#define GHDR

#ifdef GHDR
#include "../../../dev/bit/bitput.h"
#include "../../../dev/bit/bitget.h"
static unsigned char *headwr(uint32_t *freqs, unsigned char *op) { int i;
  bitencdef(enc); /*unsigned char *_op=op; op+=2;*/ bitencnew(&enc, op); 
  for(i=0;i<256;i++) { unsigned fg = freqs[i]+1; bitgput16(&enc, fg); } 
  return bitflush(&enc); //op = bitflush(&enc); *(unsigned short *)_op= (op-_op)-2; return op;
}

static unsigned char *headrd(uint32_t *freqs, unsigned char *ip) { int i;
  bitdecdef(dec); /*unsigned l = *(unsigned short *)ip; ip+=2;*/ bitdecnew(&dec, ip);
  for(i=0;i<256;i++) freqs[i] = bitgget16(&dec)-1; 
  return bitalign(&dec); //ip+l;
}
#endif

#ifdef UNROLLED
/*-----------------------------------------------------------------------------
 * Memory to memory compression functions.
 *
 * These are unrolled 4 or 8 way versions. The input/output is not compatible
 * with the original versions, but obviously unrolled encoder works with the
 * unrolled decoder
 */
unsigned char *arith_compress_O0(unsigned char *in, unsigned int in_size,
				 unsigned int *out_size, unsigned char *out_buf) {
    //unsigned char *out_buf = malloc(2*in_size + 256*256*2);
    unsigned char *cp;
    int F[256], C[256], T = 0, i, j, n, i_end;
    unsigned char c;
    RngCoder rc[8];
    char blk0[BLK_SIZE];
    char blk1[BLK_SIZE];
    char blk2[BLK_SIZE];
    char blk3[BLK_SIZE];

    if (!out_buf)
	return NULL;

    // Compute statistics
    memset(F, 0, 256*sizeof(int));
    memset(C, 0, 256*sizeof(int));
    for (i = 0; i < in_size; i++) {
	  F[c = in[i]]++;
	  T++;
    }

    // Normalise so T[i] == 65536
    for (n = j = 0; j < 256; j++)
	if (F[j])
	  n++;

    for (j = 0; j < 256; j++) {
	if (!F[j])
	    continue;
	if ((F[j] *= ((double)TOTFREQ-n)/T) == 0)
	    F[j] = 1;
    }

    // Encode statistis.
    // FIXME: use range coder to encode these more efficiently
    cp = out_buf+4;
    for (T = j = 0; j < 256; j++) {
	  C[j] = T;
	  T += F[j];
        #ifndef GHDR
	  if (F[j]) { *cp++ = j; *cp++ = F[j]>>8; *cp++ = F[j]&0xff; }
        #endif
    }
      #ifndef GHDR
    *cp++ = 0;
      #else
    cp = headwr(F, cp);
      #endif
    RC_output(&rc[0], blk0);
    RC_output(&rc[1], blk1);
    RC_output(&rc[2], blk2);
    RC_output(&rc[3], blk3);

    RC_StartEncode(&rc[0]);
    RC_StartEncode(&rc[1]);
    RC_StartEncode(&rc[2]);
    RC_StartEncode(&rc[3]);

    // Tight encoding loop
    i_end = in_size&~3;
    for (i = 0; i < i_end; i+=4) {
	unsigned char c[4];
	c[0] = in[i+0];
	c[1] = in[i+1];
	c[2] = in[i+2];
	c[3] = in[i+3];

	RC_Encode(&rc[0], C[c[0]], F[c[0]]);
	RC_Encode(&rc[1], C[c[1]], F[c[1]]);
	RC_Encode(&rc[2], C[c[2]], F[c[2]]);
	RC_Encode(&rc[3], C[c[3]], F[c[3]]);
    }

    // Tidy up any remainder that isn't a multiple of 4
    for (; i < in_size; i++) {
	unsigned char c;
	c = in[i];
	RC_Encode(&rc[0], C[c], F[c]);
    }

    // Flush encoders and collate buffers
    RC_FinishEncode(&rc[0]);
    RC_FinishEncode(&rc[1]);
    RC_FinishEncode(&rc[2]);
    RC_FinishEncode(&rc[3]);

    *cp++ = (RC_size_out(&rc[0]) >> 0) & 0xff;
    *cp++ = (RC_size_out(&rc[0]) >> 8) & 0xff;
    *cp++ = (RC_size_out(&rc[0]) >>16) & 0xff;
    *cp++ = (RC_size_out(&rc[0]) >>24) & 0xff;
    memcpy(cp, blk0, RC_size_out(&rc[0])); cp += RC_size_out(&rc[0]);

    *cp++ = (RC_size_out(&rc[1]) >> 0) & 0xff;
    *cp++ = (RC_size_out(&rc[1]) >> 8) & 0xff;
    *cp++ = (RC_size_out(&rc[1]) >>16) & 0xff;
    *cp++ = (RC_size_out(&rc[1]) >>24) & 0xff;
    memcpy(cp, blk1, RC_size_out(&rc[1])); cp += RC_size_out(&rc[1]);

    *cp++ = (RC_size_out(&rc[2]) >> 0) & 0xff;
    *cp++ = (RC_size_out(&rc[2]) >> 8) & 0xff;
    *cp++ = (RC_size_out(&rc[2]) >>16) & 0xff;
    *cp++ = (RC_size_out(&rc[2]) >>24) & 0xff;
    memcpy(cp, blk2, RC_size_out(&rc[2])); cp += RC_size_out(&rc[2]);
	
    *cp++ = (RC_size_out(&rc[3]) >> 0) & 0xff;
    *cp++ = (RC_size_out(&rc[3]) >> 8) & 0xff;
    *cp++ = (RC_size_out(&rc[3]) >>16) & 0xff;
    *cp++ = (RC_size_out(&rc[3]) >>24) & 0xff;
    memcpy(cp, blk3, RC_size_out(&rc[3])); cp += RC_size_out(&rc[3]);
	
    *out_size = cp - out_buf;

    cp = out_buf;
    *cp++ = (in_size>> 0) & 0xff;
    *cp++ = (in_size>> 8) & 0xff;
    *cp++ = (in_size>>16) & 0xff;
    *cp++ = (in_size>>24) & 0xff;

    return out_buf;
}

typedef struct {
    struct {
	int F;
	int C;
    } fc[256];
    unsigned char *R;
} ari_decoder;

unsigned char *arith_uncompress_O0(unsigned char *in, unsigned int in_size,
				   unsigned int *out_size, unsigned char *out_buf) {
    /* Load in the static tables */
    unsigned char *cp = in + 4;
    int i, j, x, out_sz, i_end;
    RngCoder rc[4];
    unsigned int sz;
    //char *out_buf;
    ari_decoder D;

    memset(&D, 0, sizeof(D));

    out_sz = ((in[0])<<0) | ((in[1])<<8) | ((in[2])<<16) | ((in[3])<<24);
    //out_buf = malloc(out_sz);  if (!out_buf) return NULL;

    //fprintf(stderr, "out_sz=%d\n", out_sz);

      #ifdef GHDR
    unsigned fr[256];
    cp = headrd(fr, cp);
    x = 0;
    for(j=0;j<256;j++) {
      if(!fr[j]) continue;
  	  D.fc[j].F = fr[j]; //(cp[0]<<8) | (cp[1]);
    
	  D.fc[j].C = x;

	/* Build reverse lookup table */
	  if (!D.R) D.R = (unsigned char *)malloc(TOTFREQ);
	  memset(&D.R[x], j, D.fc[j].F);
	  x += D.fc[j].F;
    } 
      #else
    // Precompute reverse lookup of frequency.
    j = *cp++;
    x = 0;
    do {
	D.fc[j].F = (cp[0]<<8) | (cp[1]);
    
	D.fc[j].C = x;

	/* Build reverse lookup table */
	if (!D.R) D.R = (unsigned char *)malloc(TOTFREQ);
	memset(&D.R[x], j, D.fc[j].F);

	x += D.fc[j].F;
    cp += 2; j = *cp++;
    } while(j);
     #endif

    sz = cp[0] + (cp[1]<<8) + (cp[2]<<16) + (cp[3]<<24);
    RC_input(&rc[0], (char *)cp+4);
    RC_StartDecode(&rc[0]);
    cp += sz+4;

    sz = cp[0] + (cp[1]<<8) + (cp[2]<<16) + (cp[3]<<24);
    RC_input(&rc[1], (char *)cp+4);
    RC_StartDecode(&rc[1]);
    cp += sz+4;

    sz = cp[0] + (cp[1]<<8) + (cp[2]<<16) + (cp[3]<<24);
    RC_input(&rc[2], (char *)cp+4);
    RC_StartDecode(&rc[2]);
    cp += sz+4;
	
    sz = cp[0] + (cp[1]<<8) + (cp[2]<<16) + (cp[3]<<24);
    RC_input(&rc[3], (char *)cp+4);
    RC_StartDecode(&rc[3]);
    cp += sz+4;


    // Tight decoding loop, unrolled 4-ways
    i_end = out_sz&~3;
    for (i = 0; i < i_end; i+=4) {
	uint32_t freq[4];
	unsigned char c[4];
      #if 1
	freq[0] = RC_GetFreq(&rc[0]);
	freq[1] = RC_GetFreq(&rc[1]);
	freq[2] = RC_GetFreq(&rc[2]);
	freq[3] = RC_GetFreq(&rc[3]);
      #else
    int j;
    for(j=0; j<4; j++) rc[j].range >>= TF_SHIFT; 
    unsigned rcc[4],rcr[4];
    for(j=0; j<4; j++) { rcc[j] = rc[j].code; rcr[j]=rc[j].range; }
    __m128i v = _mm_load_si128((__m128i *)rcc);
    _mm128_div_epu32(v, _mm_load_si128((__m128i *)rcr));
    _mm_storeu_si128((__m128i *)freq, v);
    //for(j=0;j<4;j++) { rcc[j] = rc[j].code; rcr[j]=rc[j].range;}
    //for(j=0;j<4;j++) freq[j] = rc[j].code / rc[j].range;
//    freq[j] = RC_GetFreq(&rc[j]);
      #endif
	c[0] = D.R[freq[0]];
	c[1] = D.R[freq[1]];
	c[2] = D.R[freq[2]];
	c[3] = D.R[freq[3]];

	RC_Decode(&rc[0], D.fc[c[0]].C, D.fc[c[0]].F);
	RC_Decode(&rc[1], D.fc[c[1]].C, D.fc[c[1]].F);
	RC_Decode(&rc[2], D.fc[c[2]].C, D.fc[c[2]].F);
	RC_Decode(&rc[3], D.fc[c[3]].C, D.fc[c[3]].F);

	out_buf[i+0] = c[0];
	out_buf[i+1] = c[1];
	out_buf[i+2] = c[2];
	out_buf[i+3] = c[3];
    }

    // Tidy up any remainder that isn't a multiple of 4
    for (; i < out_sz; i++) {
	uint32_t freq = RC_GetFreq(&rc[0]);
	unsigned char c = D.R[freq];
	RC_Decode(&rc[0], D.fc[c].C, D.fc[c].F);
	out_buf[i] = c;
    }


    RC_FinishDecode(&rc[0]);
    RC_FinishDecode(&rc[1]);
    RC_FinishDecode(&rc[2]);
    RC_FinishDecode(&rc[3]);

    *out_size = out_sz;

    if (D.R) free(D.R);

    return (unsigned char *)out_buf;
}

unsigned char *arith_compress_O1(unsigned char *in, unsigned int in_size,
				 unsigned int *out_size) {
    unsigned char *out_buf = malloc(2*in_size + 256*256*2);
    unsigned char *cp = out_buf;
    RngCoder rc[8];
    unsigned int last, i, j, i8[8], l8[8], i_end;
    int F[256][256], C[256][256], T[256];
    unsigned char c;
    // FIXME: too large for stack size on some systems.
    char *blk = malloc(BLK_SIZE+8);

    if (!out_buf || !out_buf) {
	if (blk) free(blk);
	if (out_buf) free(out_buf);
	return NULL;
    }

    cp = out_buf+4;

    memset(F, 0, 256*256*sizeof(int));
    memset(C, 0, 256*256*sizeof(int));
    memset(T, 0, 256*sizeof(int));
    for (last = i = 0; i < in_size; i++) {
	F[last][c = in[i]]++;
	T[last]++;
	last = c;
    }
    F[0][in[1*(in_size>>3)]]++;
    F[0][in[2*(in_size>>3)]]++;
    F[0][in[3*(in_size>>3)]]++;
    F[0][in[4*(in_size>>3)]]++;
    F[0][in[5*(in_size>>3)]]++;
    F[0][in[6*(in_size>>3)]]++;
    F[0][in[7*(in_size>>3)]]++;
    T[0]+=7;

    // Normalise so T[i] == 65536
    for (i = 0; i < 256; i++) {
	int t = T[i], t2, n;

	if (t == 0)
	    continue;

	for (n = j = 0; j < 256; j++)
	    if (F[i][j])
		n++;

	for (t2 = j = 0; j < 256; j++) {
	    if (!F[i][j])
	    	continue;
	    if ((F[i][j] *= ((double)TOTFREQ-n)/t) == 0)
	    	F[i][j] = 1;
	    t2 += F[i][j];
	}

	// No need to actually boost frequencies so the real sum is
	// 65536. Just accept loss in precision by having a bit of
	// rounding at the end.
	assert(t2 <= TOTFREQ);
    }

    //assert(in_size < TOP);
    for (i = 0; i < 256; i++) {
	unsigned int x = 0;
	if (!T[i])
	    continue;

	*cp++ = i;
	for (j = 0; j < 256; j++) {
	    C[i][j] = x;
	    x += F[i][j];
	    if (F[i][j]) {
	    	*cp++ = j;
	    	*cp++ = F[i][j]>>8;
	    	*cp++ = F[i][j]&0xff;
	    }
	}
	*cp++ = 0;
	T[i] = x;
    }
    *cp++ = 0;


    /* Initialise our 8 range coders with their appropriate buffers */
    RC_output(&rc[0], blk+0*(BLK_SIZE>>3));
    RC_output(&rc[1], blk+1*(BLK_SIZE>>3));
    RC_output(&rc[2], blk+2*(BLK_SIZE>>3));
    RC_output(&rc[3], blk+3*(BLK_SIZE>>3));
    RC_output(&rc[4], blk+4*(BLK_SIZE>>3));
    RC_output(&rc[5], blk+5*(BLK_SIZE>>3));
    RC_output(&rc[6], blk+6*(BLK_SIZE>>3));
    RC_output(&rc[7], blk+7*(BLK_SIZE>>3));

    RC_StartEncode(&rc[0]);
    RC_StartEncode(&rc[1]);
    RC_StartEncode(&rc[2]);
    RC_StartEncode(&rc[3]);
    RC_StartEncode(&rc[4]);
    RC_StartEncode(&rc[5]);
    RC_StartEncode(&rc[6]);
    RC_StartEncode(&rc[7]);

    i_end = (in_size>>3);
    i8[0] = 0 * i_end;
    i8[1] = 1 * i_end;
    i8[2] = 2 * i_end;
    i8[3] = 3 * i_end;
    i8[4] = 4 * i_end;
    i8[5] = 5 * i_end;
    i8[6] = 6 * i_end;
    i8[7] = 7 * i_end;


    /* Main busy loop */
    for (l8[0] = l8[1] = l8[2] = l8[3] = l8[4] = l8[5] = l8[6] = l8[7] = 0;
	 //i8[0] < i_end;
	 i_end--;
	 i8[0]++,i8[1]++,i8[2]++,i8[3]++,i8[4]++,i8[5]++,i8[6]++,i8[7]++) {
	unsigned char c[8];
	c[0] = in[i8[0]];
	c[1] = in[i8[1]];
	c[2] = in[i8[2]];
	c[3] = in[i8[3]];
	c[4] = in[i8[4]];
	c[5] = in[i8[5]];
	c[6] = in[i8[6]];
	c[7] = in[i8[7]];
	
	RC_Encode(&rc[0], C[l8[0]][c[0]], F[l8[0]][c[0]]);
	RC_Encode(&rc[1], C[l8[1]][c[1]], F[l8[1]][c[1]]);
	RC_Encode(&rc[2], C[l8[2]][c[2]], F[l8[2]][c[2]]);
	RC_Encode(&rc[3], C[l8[3]][c[3]], F[l8[3]][c[3]]);
	RC_Encode(&rc[4], C[l8[4]][c[4]], F[l8[4]][c[4]]);
	RC_Encode(&rc[5], C[l8[5]][c[5]], F[l8[5]][c[5]]);
	RC_Encode(&rc[6], C[l8[6]][c[6]], F[l8[6]][c[6]]);
	RC_Encode(&rc[7], C[l8[7]][c[7]], F[l8[7]][c[7]]);
	
	l8[0] = c[0];
	l8[1] = c[1];
	l8[2] = c[2];
	l8[3] = c[3];
	l8[4] = c[4];
	l8[5] = c[5];
	l8[6] = c[6];
	l8[7] = c[7];
    }


    /* Remainder of buffer for when not a multiple of 8 */
    for (; i8[7] < in_size; i8[7]++) {
	unsigned char c;
	c = in[i8[7]];
	RC_Encode(&rc[7], C[l8[7]][c], F[l8[7]][c]);
	l8[7] = c;
    }
    

    /* Flush encoders */
    RC_FinishEncode(&rc[0]);
    RC_FinishEncode(&rc[1]);
    RC_FinishEncode(&rc[2]);
    RC_FinishEncode(&rc[3]);
    RC_FinishEncode(&rc[4]);
    RC_FinishEncode(&rc[5]);
    RC_FinishEncode(&rc[6]);
    RC_FinishEncode(&rc[7]);
    

    /* Move decoder output to the final compressed buffer */
    *cp++ = (RC_size_out(&rc[0]) >> 0) & 0xff;
    *cp++ = (RC_size_out(&rc[0]) >> 8) & 0xff;
    *cp++ = (RC_size_out(&rc[0]) >>16) & 0xff;
    *cp++ = (RC_size_out(&rc[0]) >>24) & 0xff;
    memcpy(cp, blk+0*(BLK_SIZE>>3), RC_size_out(&rc[0]));
    cp += RC_size_out(&rc[0]);

    *cp++ = (RC_size_out(&rc[1]) >> 0) & 0xff;
    *cp++ = (RC_size_out(&rc[1]) >> 8) & 0xff;
    *cp++ = (RC_size_out(&rc[1]) >>16) & 0xff;
    *cp++ = (RC_size_out(&rc[1]) >>24) & 0xff;
    memcpy(cp, blk+1*(BLK_SIZE>>3), RC_size_out(&rc[1]));
    cp += RC_size_out(&rc[1]);

    *cp++ = (RC_size_out(&rc[2]) >> 0) & 0xff;
    *cp++ = (RC_size_out(&rc[2]) >> 8) & 0xff;
    *cp++ = (RC_size_out(&rc[2]) >>16) & 0xff;
    *cp++ = (RC_size_out(&rc[2]) >>24) & 0xff;
    memcpy(cp, blk+2*(BLK_SIZE>>3), RC_size_out(&rc[2]));
    cp += RC_size_out(&rc[2]);

    *cp++ = (RC_size_out(&rc[3]) >> 0) & 0xff;
    *cp++ = (RC_size_out(&rc[3]) >> 8) & 0xff;
    *cp++ = (RC_size_out(&rc[3]) >>16) & 0xff;
    *cp++ = (RC_size_out(&rc[3]) >>24) & 0xff;
    memcpy(cp, blk+3*(BLK_SIZE>>3), RC_size_out(&rc[3]));
    cp += RC_size_out(&rc[3]);

    *cp++ = (RC_size_out(&rc[4]) >> 0) & 0xff;
    *cp++ = (RC_size_out(&rc[4]) >> 8) & 0xff;
    *cp++ = (RC_size_out(&rc[4]) >>16) & 0xff;
    *cp++ = (RC_size_out(&rc[4]) >>24) & 0xff;
    memcpy(cp, blk+4*(BLK_SIZE>>3), RC_size_out(&rc[4]));
    cp += RC_size_out(&rc[4]);

    *cp++ = (RC_size_out(&rc[5]) >> 0) & 0xff;
    *cp++ = (RC_size_out(&rc[5]) >> 8) & 0xff;
    *cp++ = (RC_size_out(&rc[5]) >>16) & 0xff;
    *cp++ = (RC_size_out(&rc[5]) >>24) & 0xff;
    memcpy(cp, blk+5*(BLK_SIZE>>3), RC_size_out(&rc[5]));
    cp += RC_size_out(&rc[5]);

    *cp++ = (RC_size_out(&rc[6]) >> 0) & 0xff;
    *cp++ = (RC_size_out(&rc[6]) >> 8) & 0xff;
    *cp++ = (RC_size_out(&rc[6]) >>16) & 0xff;
    *cp++ = (RC_size_out(&rc[6]) >>24) & 0xff;
    memcpy(cp, blk+6*(BLK_SIZE>>3), RC_size_out(&rc[6]));
    cp += RC_size_out(&rc[6]);

    *cp++ = (RC_size_out(&rc[7]) >> 0) & 0xff;
    *cp++ = (RC_size_out(&rc[7]) >> 8) & 0xff;
    *cp++ = (RC_size_out(&rc[7]) >>16) & 0xff;
    *cp++ = (RC_size_out(&rc[7]) >>24) & 0xff;
    memcpy(cp, blk+7*(BLK_SIZE>>3), RC_size_out(&rc[7]));
    cp += RC_size_out(&rc[7]);

    *out_size = cp - out_buf;

    cp = out_buf;
    *cp++ = (in_size>> 0) & 0xff;
    *cp++ = (in_size>> 8) & 0xff;
    *cp++ = (in_size>>16) & 0xff;
    *cp++ = (in_size>>24) & 0xff;

    free(blk);
    return out_buf;
}

unsigned char *arith_uncompress_O1(unsigned char *in, unsigned int in_size,
				   unsigned int *out_size) {
    /* Load in the static tables */
    unsigned char *cp = in + 4;
    int i, j, i_end, i8[8], l8[8], x, out_sz;
    RngCoder rc[8];
    char *out_buf;
    ari_decoder D[256];
    uint32_t sz;

    memset(D, 0, 256*sizeof(*D));

    out_sz = ((in[0])<<0) | ((in[1])<<8) | ((in[2])<<16) | ((in[3])<<24);
    out_buf = malloc(out_sz);
    if (!out_buf)
	return NULL;

    //fprintf(stderr, "out_sz=%d\n", out_sz);

    i = *cp++;
    do {
	j = *cp++;
	x = 0;
	do {
	    D[i].fc[j].F = (cp[0]<<8) | (cp[1]);
	    D[i].fc[j].C = x;

	    /* Build reverse lookup table */
	    if (!D[i].R) D[i].R = (unsigned char *)malloc(TOTFREQ);
	    memset(&D[i].R[x], j, D[i].fc[j].F);

	    x += D[i].fc[j].F;
	    cp += 2;
	    //fprintf(stderr, "F[%d][%d]=%d\n", i, j, F[i][j]);
	    j = *cp++;
	} while(j);

	i = *cp++;
    } while (i);


    /* Start up the 8 parallel decoders */
    sz = cp[0] + (cp[1]<<8) + (cp[2]<<16) + (cp[3]<<24);
    RC_input(&rc[0], (char *)cp+4);
    cp += sz+4;

    sz = cp[0] + (cp[1]<<8) + (cp[2]<<16) + (cp[3]<<24);
    RC_input(&rc[1], (char *)cp+4);
    cp += sz+4;

    sz = cp[0] + (cp[1]<<8) + (cp[2]<<16) + (cp[3]<<24);
    RC_input(&rc[2], (char *)cp+4);
    cp += sz+4;

    sz = cp[0] + (cp[1]<<8) + (cp[2]<<16) + (cp[3]<<24);
    RC_input(&rc[3], (char *)cp+4);
    cp += sz+4;

    sz = cp[0] + (cp[1]<<8) + (cp[2]<<16) + (cp[3]<<24);
    RC_input(&rc[4], (char *)cp+4);
    cp += sz+4;

    sz = cp[0] + (cp[1]<<8) + (cp[2]<<16) + (cp[3]<<24);
    RC_input(&rc[5], (char *)cp+4);
    cp += sz+4;

    sz = cp[0] + (cp[1]<<8) + (cp[2]<<16) + (cp[3]<<24);
    RC_input(&rc[6], (char *)cp+4);
    cp += sz+4;

    sz = cp[0] + (cp[1]<<8) + (cp[2]<<16) + (cp[3]<<24);
    RC_input(&rc[7], (char *)cp+4);
    cp += sz+4;

    RC_StartDecode(&rc[0]);
    RC_StartDecode(&rc[1]);
    RC_StartDecode(&rc[2]);
    RC_StartDecode(&rc[3]);
    RC_StartDecode(&rc[4]);
    RC_StartDecode(&rc[5]);
    RC_StartDecode(&rc[6]);
    RC_StartDecode(&rc[7]);

    i_end = out_sz>>3;
    i8[0] = 0*i_end;
    i8[1] = 1*i_end;
    i8[2] = 2*i_end;
    i8[3] = 3*i_end;
    i8[4] = 4*i_end;
    i8[5] = 5*i_end;
    i8[6] = 6*i_end;
    i8[7] = 7*i_end;

    /* The main busy loop. Decode from 8 striped locations */
    for (l8[0] = l8[1] = l8[2] = l8[3] = l8[4] = l8[5] = l8[6] = l8[7] = 0;
	 //i < i_end;
	 i_end--;
	 i8[0]++,i8[1]++,i8[2]++,i8[3]++,i8[4]++,i8[5]++,i8[6]++,i8[7]++) {
	uint32_t freq[8];
	unsigned char c[8];

	freq[0] = RC_GetFreq(&rc[0]);
	freq[1] = RC_GetFreq(&rc[1]);
	freq[2] = RC_GetFreq(&rc[2]);
	freq[3] = RC_GetFreq(&rc[3]);
	freq[4] = RC_GetFreq(&rc[4]);
	freq[5] = RC_GetFreq(&rc[5]);
	freq[6] = RC_GetFreq(&rc[6]);
	freq[7] = RC_GetFreq(&rc[7]);

	c[0] = D[l8[0]].R[freq[0]];
	c[1] = D[l8[1]].R[freq[1]];
	c[2] = D[l8[2]].R[freq[2]];
	c[3] = D[l8[3]].R[freq[3]];
	c[4] = D[l8[4]].R[freq[4]];
	c[5] = D[l8[5]].R[freq[5]];
	c[6] = D[l8[6]].R[freq[6]];
	c[7] = D[l8[7]].R[freq[7]];

	RC_Decode(&rc[0], D[l8[0]].fc[c[0]].C, D[l8[0]].fc[c[0]].F);
	RC_Decode(&rc[1], D[l8[1]].fc[c[1]].C, D[l8[1]].fc[c[1]].F);
	RC_Decode(&rc[2], D[l8[2]].fc[c[2]].C, D[l8[2]].fc[c[2]].F);
	RC_Decode(&rc[3], D[l8[3]].fc[c[3]].C, D[l8[3]].fc[c[3]].F);
	RC_Decode(&rc[4], D[l8[4]].fc[c[4]].C, D[l8[4]].fc[c[4]].F);
	RC_Decode(&rc[5], D[l8[5]].fc[c[5]].C, D[l8[5]].fc[c[5]].F);
	RC_Decode(&rc[6], D[l8[6]].fc[c[6]].C, D[l8[6]].fc[c[6]].F);
	RC_Decode(&rc[7], D[l8[7]].fc[c[7]].C, D[l8[7]].fc[c[7]].F);

	out_buf[i8[0]] = c[0];
	out_buf[i8[1]] = c[1];
	out_buf[i8[2]] = c[2];
	out_buf[i8[3]] = c[3];
	out_buf[i8[4]] = c[4];
	out_buf[i8[5]] = c[5];
	out_buf[i8[6]] = c[6];
	out_buf[i8[7]] = c[7];

	l8[0] = c[0];
	l8[1] = c[1];
	l8[2] = c[2];
	l8[3] = c[3];
	l8[4] = c[4];
	l8[5] = c[5];
	l8[6] = c[6];
	l8[7] = c[7];
    }

    /* Remainder */
    for (; i8[7] < out_sz; i8[7]++) {
	uint32_t freq = RC_GetFreq(&rc[7]);
	unsigned char c = D[l8[7]].R[freq];

	RC_Decode(&rc[7], D[l8[7]].fc[c].C, D[l8[7]].fc[c].F);
	out_buf[i8[7]] = c;
	l8[7] = c;
    }


    /* Tidyup */
    RC_FinishDecode(&rc[0]);
    RC_FinishDecode(&rc[1]);
    RC_FinishDecode(&rc[2]);
    RC_FinishDecode(&rc[3]);
    RC_FinishDecode(&rc[4]);
    RC_FinishDecode(&rc[5]);
    RC_FinishDecode(&rc[6]);
    RC_FinishDecode(&rc[7]);

    *out_size = out_sz;

    for (i = 0; i < 256; i++)
	if (D[i].R) free(D[i].R);

    return (unsigned char *)out_buf;
}


#else /* UNROLLED */
/*-----------------------------------------------------------------------------
 * Memory to memory compression functions.
 *
 * These are original versions without any manual loop unrolling. They
 * are easier to understand, but can be up to 2x slower.
 */

unsigned char *arith_compress_O0(unsigned char *in, unsigned int in_size,
				 unsigned int *out_size, unsigned char *out_buf) {
    //unsigned char *out_buf = malloc(2*in_size + 256*256*2);
    unsigned char *cp;
    RngCoder rc;
    int F[256], C[256], T = 0, i, j, n;
    unsigned char c;

    if (!out_buf)
	return NULL;

    // Compute statistics
    memset(F, 0, 256*sizeof(int));
    memset(C, 0, 256*sizeof(int));
    for (i = 0; i < in_size; i++) {
	F[c = in[i]]++;
	T++;
    }

    // Normalise so T[i] == 65536
    for (n = j = 0; j < 256; j++)
	if (F[j])
	    n++;

    for (j = 0; j < 256; j++) {
	if (!F[j])
	    continue;
	if ((F[j] *= ((double)TOTFREQ-n)/T) == 0)
	    F[j] = 1;
    }


    // Encode statistis.
    cp = out_buf+4;
    for (T = j = 0; j < 256; j++) {
	C[j] = T;
	T += F[j];
	if (F[j]) {
	    *cp++ = j;
	    *cp++ = F[j]>>8;
	    *cp++ = F[j]&0xff;
	}
    }
    *cp++ = 0;

    RC_output(&rc, (char *)cp);
    RC_StartEncode(&rc);
    for (i = 0; i < in_size; i++) {
	unsigned char c = in[i];
	RC_Encode(&rc, C[c], F[c]);
    }
    RC_FinishEncode(&rc);

    // Finalise block size and return it
    *out_size = RC_size_out(&rc) + (cp - out_buf);

    cp = out_buf;
    *cp++ = (in_size>> 0) & 0xff;
    *cp++ = (in_size>> 8) & 0xff;
    *cp++ = (in_size>>16) & 0xff;
    *cp++ = (in_size>>24) & 0xff;

    return out_buf;
}

typedef struct {
    struct {
	int F;
	int C;
    } fc[256];
    unsigned char *R;
} ari_decoder;

unsigned char *arith_uncompress_O0(unsigned char *in, unsigned int in_size,
				   unsigned int *out_size, unsigned char *out_buf) {
    /* Load in the static tables */
    unsigned char *cp = in + 4, c;
    int i, j, x, out_sz;
    RngCoder rc;
    //char *out_buf;
    ari_decoder D;

    memset(&D, 0, sizeof(D));

    out_sz = ((in[0])<<0) | ((in[1])<<8) | ((in[2])<<16) | ((in[3])<<24);
    //out_buf = malloc(out_sz); if (!out_buf) return NULL;

    //fprintf(stderr, "out_sz=%d\n", out_sz);

    // Precompute reverse lookup of frequency.
    {
	j = *cp++;
	x = 0;
	do {
	    D.fc[j].F = (cp[0]<<8) | (cp[1]);
	    D.fc[j].C = x;

	    /* Build reverse lookup table */
	    if (!D.R) D.R = (unsigned char *)malloc(TOTFREQ);
	    memset(&D.R[x], j, D.fc[j].F);

	    x += D.fc[j].F;
	    cp += 2;
	    j = *cp++;
	} while(j);
    }

    RC_input(&rc, (char *)cp);
    RC_StartDecode(&rc);
    for (i = 0; i < out_sz; i++) {
	uint32_t freq = RC_GetFreq(&rc);
	c = D.R[freq];
	RC_Decode(&rc, D.fc[c].C, D.fc[c].F);
	out_buf[i] = c;
    }
    RC_FinishDecode(&rc);

    *out_size = out_sz;

    if (D.R) free(D.R);

    return (unsigned char *)out_buf;
}
unsigned char *arith_compress_O1(unsigned char *in, unsigned int in_size,
				 unsigned int *out_size, unsigned char *out_buf) {
    //unsigned char *out_buf = malloc(2*in_size + 256*256*2);
    unsigned char *cp = out_buf;
    RngCoder rc;
    unsigned int last;

    if (!out_buf)
	return NULL;

    cp = out_buf+4;

    if (1) {
	int F[256][256], C[256][256], T[256], i, j;
	unsigned char c;

	memset(F, 0, 256*256*sizeof(int));
	memset(C, 0, 256*256*sizeof(int));
	memset(T, 0, 256*sizeof(int));
	for (last = i = 0; i < in_size; i++) {
	    F[last][c = in[i]]++;
	    T[last]++;
	    last = c;
	}

	// Normalise so T[i] == 65536
	for (i = 0; i < 256; i++) {
	    int t = T[i], t2, n;

	    if (t == 0)
		continue;

	    for (n = j = 0; j < 256; j++)
		if (F[i][j])
		    n++;

	    for (t2 = j = 0; j < 256; j++) {
		if (!F[i][j])
		    continue;
		if ((F[i][j] *= ((double)TOTFREQ-n)/t) == 0)
		    F[i][j] = 1;
		t2 += F[i][j];
	    }

	    // No need to actually boost frequencies so the real sum is
	    // 65536. Just accept loss in precision by having a bit of
	    // rounding at the end.
	    assert(t2 <= TOTFREQ);
	}

	for (i = 0; i < 256; i++) {
	    unsigned int x = 0;
	    if (!T[i])
		continue;

	    *cp++ = i;
	    for (j = 0; j < 256; j++) {
		C[i][j] = x;
		x += F[i][j];
		if (F[i][j]) {
		    *cp++ = j;
		    *cp++ = F[i][j]>>8;
		    *cp++ = F[i][j]&0xff;
		    //fprintf(stderr, "F[%d][%d]=%d\n", i, j, F[i][j]);
		}
	    }
	    *cp++ = 0;
	    T[i] = x;
	}
	*cp++ = 0;

	RC_output(&rc, (char *)cp);
	RC_StartEncode(&rc);
	for (last = i = 0; i < in_size; i++) {
	    unsigned char c = in[i];
	    //fprintf(stderr, "c=%d => C[][] + F[] = %d+%d\n", c, C[last][c], F[last][c]);
	    RC_Encode(&rc, C[last][c], F[last][c]);
	    last = c;
	}
	RC_FinishEncode(&rc);
    }
    
    *out_size = RC_size_out(&rc) + (cp - out_buf);

    cp = out_buf;
    *cp++ = (in_size>> 0) & 0xff;
    *cp++ = (in_size>> 8) & 0xff;
    *cp++ = (in_size>>16) & 0xff;
    *cp++ = (in_size>>24) & 0xff;

    return out_buf;
}

unsigned char *arith_uncompress_O1(unsigned char *in, unsigned int in_size,
				   unsigned int *out_size, unsigned char *out_buf) {
    /* Load in the static tables */
    unsigned char *cp = in + 4, c;
    int i, j, x, last, out_sz;
    RngCoder rc;
    //char *out_buf;
    ari_decoder D[256];

    memset(D, 0, 256*sizeof(*D));

    out_sz = ((in[0])<<0) | ((in[1])<<8) | ((in[2])<<16) | ((in[3])<<24);
    //out_buf = malloc(out_sz); if (!out_buf) return NULL;

    //fprintf(stderr, "out_sz=%d\n", out_sz);

    i = *cp++;
    do {
	j = *cp++;
	x = 0;
	do {
	    D[i].fc[j].F = (cp[0]<<8) | (cp[1]);
	    D[i].fc[j].C = x;

	    /* Build reverse lookup table */
	    if (!D[i].R) D[i].R = (unsigned char *)malloc(TOTFREQ);
	    memset(&D[i].R[x], j, D[i].fc[j].F);

	    x += D[i].fc[j].F;
	    cp += 2;
	    //fprintf(stderr, "F[%d][%d]=%d\n", i, j, F[i][j]);
	    j = *cp++;
	} while(j);

	i = *cp++;
    } while (i);

    // Precompute reverse lookup of frequency.

    RC_input(&rc, (char *)cp);
    RC_StartDecode(&rc);
    for (last = i = 0; i < out_sz; i++) {
	uint32_t freq = RC_GetFreq(&rc);
	// Direct lookup table
	c = D[last].R[freq];

	// Linear scan
	//for (c = 0; c < 256; c++) {
	//    if (C[last][R[last][c]] + F[last][R[last][c]]> freq) {
	//	c = R[last][c];
	//	break;
	//    }
	//}

	//Binary search
	// To do

	//fprintf(stderr, "Freq=%d => %d. Decode(%d, %d)\n", freq, c, C[last][c], F[last][c]);
	RC_Decode(&rc, D[last].fc[c].C, D[last].fc[c].F);
	out_buf[i] = c;
	last = c;
    }
    RC_FinishDecode(&rc);

    *out_size = out_sz;

    for (i = 0; i < 256; i++)
	if (D[i].R) free(D[i].R);

    return (unsigned char *)out_buf;
}
#endif /* UNROLLED */

#ifdef TEST_MAIN

/*-----------------------------------------------------------------------------
 * Simple interface to the order-0 vs order-1 encoders and decoders.
 */
unsigned char *arith_compress(unsigned char *in, unsigned int in_size,
			      unsigned int *out_size, int order) {
    return order
	? arith_compress_O1(in, in_size, out_size)
	: arith_compress_O0(in, in_size, out_size);
}

unsigned char *arith_uncompress(unsigned char *in, unsigned int in_size,
				unsigned int *out_size, int order) {
    return order
	? arith_uncompress_O1(in, in_size, out_size)
	: arith_uncompress_O0(in, in_size, out_size);
}

/*-----------------------------------------------------------------------------
 * Main
 */
int main(int argc, char **argv) {
    int opt, order = 1;
    unsigned char in_buf[BLK_SIZE*2];
    int decode = 0;

    extern char *optarg;
    extern int optind, opterr, optopt;

    while ((opt = getopt(argc, argv, "o:d")) != -1) {
	switch (opt) {
	case 'o':
	    order = atoi(optarg);
	    break;

	case 'd':
	    decode = 1;
	    break;
	}
    }

    order = order ? 1 : 0; // Only support O(0) and O(1)

    if (decode) {
	// Only used in some test implementations of RC_GetFreq()
	RC_init();
	RC_init2();

	for (;;) {
	    uint32_t in_size, out_size;
	    unsigned char *out;

	    order = fgetc(stdin);
	    if (4 != fread(&in_size, 1, 4, stdin))
		break;
	    if (in_size != fread(in_buf, 1, in_size, stdin)) {
		fprintf(stderr, "Truncated input\n");
		exit(1);
	    }
	    out = arith_uncompress(in_buf, in_size, &out_size, order);
	    if (!out)
		abort();

	    fwrite(out, 1, out_size, stdout);
	    free(out);
	}
    } else {
	for (;;) {
	    uint32_t in_size, out_size;
	    unsigned char *out;

	    in_size = fread(in_buf, 1, BLK_SIZE, stdin);
	    if (in_size <= 0)
		break;

	    out = arith_compress(in_buf, in_size, &out_size, order);

	    fputc(order, stdout);
	    fwrite(&out_size, 1, 4, stdout);
	    fwrite(out, 1, out_size, stdout);
	    free(out);
	}
    }

    return 0;
}
#endif
