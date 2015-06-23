/* base64.c : base-64 / MIME encode/decode */
/* PUBLIC DOMAIN - Jon Mayo - November 13, 2003 */
#include "c_types.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include "base64.h"

static const uint8_t base64enc_tab[64]= "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int base64_encode(size_t in_len, const unsigned char *in, size_t out_len, char *out) {
	unsigned ii, io;
	uint32_t v;
	unsigned rem;

	for(io=0,ii=0,v=0,rem=0;ii<in_len;ii++) {
		unsigned char ch;
		ch=in[ii];
		v=(v<<8)|ch;
		rem+=8;
		while(rem>=6) {
			rem-=6;
			if(io>=out_len) return -1; /* truncation is failure */
			out[io++]=base64enc_tab[(v>>rem)&63];
		}
	}
	if(rem) {
		v<<=(6-rem);
		if(io>=out_len) return -1; /* truncation is failure */
		out[io++]=base64enc_tab[v&63];
	}
	while(io&3) {
		if(io>=out_len) return -1; /* truncation is failure */
		out[io++]='=';
	}
	if(io>=out_len) return -1; /* no room for null terminator */
	out[io]=0;
	return io;
}
