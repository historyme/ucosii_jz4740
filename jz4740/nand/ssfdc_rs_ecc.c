/* Reed-Solomon decoder
 * Copyright 2002 Phil Karn, KA9Q
 * May be used under the terms of the GNU Lesser General Public License (LGPL)
 */


#include "ssfdc_rs_ecc.h"

struct CONV_DATA {
	unsigned char  shift0;
	unsigned char  mask0;
	unsigned char  mask1;
	unsigned char  merge_shift;
} ConvData [8]= {
	{0x0, 0xff, 0x01, 0x8},  /* 0 */
	{0x1, 0x7f, 0x03, 0x7},  /* 1 */
	{0x2, 0x3f, 0x07, 0x6},  /* 2 */
	{0x3, 0x1f, 0x0f, 0x5},  /* 3 */
	{0x4, 0x0f, 0x1f, 0x4},  /* 4 */
	{0x5, 0x07, 0x3f, 0x3},  /* 5 */
	{0x6, 0x03, 0x7f, 0x2},  /* 6 */
	{0x7, 0x01, 0xff, 0x1},  /* 7 */
};


void parity2EccStore(data_t *parity, unsigned char *ecc_store )
{
	unsigned short i;

	memset(ecc_store, 0x0, 9);	
	for(i=0; i<8; i++){
		ecc_store[i] = (char)parity[i];
		ecc_store[8] |= ((parity[i] & 0x100) >> 8);
		if( i != 7 )
			ecc_store[8] <<= 1;
	}

}

void eccStore2Parity(unsigned char *ecc_store, data_t *parity)
{
	unsigned short i;
	unsigned char tmp = 0x80;

	memset(parity, 0x0, 8);
	for(i=0; i<8; i++){
		parity[i] = (int)ecc_store[i];
		parity[i] |= ( (ecc_store[8] & tmp) << (i+1) );
		tmp >>= 1;		
	}

}

void Data2Sym(data_t in[512], data_t out[512])
{
	int i, j, shift, high, low;

        shift = 0;
        j = 0;
        for (i = 0; i < 511; i++){
		low = in[i] >> ConvData[shift].shift0;
		low = low & ConvData[shift].mask0;
		high = in[i + 1] & ConvData[shift].mask1;
		out[j++] = low | (high << ConvData[shift].merge_shift);
		if (shift == 7)	{
			shift = 0;
			i++;
		}
		else
			shift++;
        }
	if (i == 511)
        {
		out[j] = in[i] >> ConvData[shift].shift0;
		out[j] = out[j] & ConvData[shift].mask0;
		j++;
        }
	
        for (; j < 503; j++)
		out[j] = 0;
	
	return;
}


static int modnn(struct rs *rs,int x)
{
	while (x >= rs->nn) {
		x -= rs->nn;
		x = (x >> rs->mm) + (x & rs->nn);
	}
	return x;
}

static	data_t lambda[256 +1], s[256];
static	data_t b[256 +1], t[256 +1], omega[256 +1];
static	data_t root[256], reg[256 +1], loc[256];


int decode_rs(void *p,data_t *data, int *eras_pos, int no_eras)
{
	struct rs *rs = (struct rs *)p;

	int deg_lambda, el, deg_omega;
	int i, j, r,k;
	data_t u,q,tmp,num1,num2,den,discr_r;

	
	int syn_error, count;
	for(i=0;i<(rs->nroots);i++)
		s[i] = data[0];
	
	for(j=1;j<(rs->nn)-(rs->pad);j++){
		for(i=0;i<(rs->nroots);i++){
			if(s[i] == 0){
				s[i] = data[j];
			} else {
				s[i] = data[j] ^ (rs->alpha_to)[modnn(rs,(rs->index_of)[s[i]] + ((rs->fcr)+i)*(rs->prim))];
			}
		}
	}
	syn_error = 0;
	for(i=0;i<(rs->nroots);i++){
		syn_error |= s[i];
		s[i] = (rs->index_of)[s[i]];
	}

	if (!syn_error) {
		count = 0;
		goto finish;
	}
	memset(&lambda[1],0,(rs->nroots)*sizeof(lambda[0]));
	lambda[0] = 1;
	
	if (no_eras > 0) {
		
		lambda[1] = (rs->alpha_to)[modnn(rs,(rs->prim)*((rs->nn)-1-eras_pos[0]))];
		for (i = 1; i < no_eras; i++) {
			u = modnn(rs,(rs->prim)*((rs->nn)-1-eras_pos[i]));
			for (j = i+1; j > 0; j--) {
				tmp = (rs->index_of)[lambda[j - 1]];
				if(tmp != ((rs->nn)))
					lambda[j] ^= (rs->alpha_to)[modnn(rs,u + tmp)];
			}
		}

		for(i = 1;i <= no_eras; i++)
			reg[i] = (rs->index_of)[lambda[i]];
		
		count = 0;

		for (i = 1,k=(rs->iprim)-1; i <= (rs->nn); i++,k = modnn(rs,k+(rs->iprim))) {
			q = 1;
			for (j = 1; j <= no_eras; j++)
				if (reg[j] != ((rs->nn))) {
					reg[j] = modnn(rs,reg[j] + j);
					q ^= (rs->alpha_to)[reg[j]];
				}
			
			if (q != 0)
				continue;
			
			root[count] = i;
			loc[count] = k;
			count++;
		}

		if (count != no_eras) {
			count = -1;
			goto finish;
		}
	}
	
	for(i=0;i<(rs->nroots)+1;i++)
		b[i] = (rs->index_of)[lambda[i]];
	
	r = no_eras;
	el = no_eras;
	while (++r <= (rs->nroots)) {
		
		discr_r = 0;
		for (i = 0; i < r; i++){
			if ((lambda[i] != 0) && (s[r-i-1] != ((rs->nn)))) {
				discr_r ^= (rs->alpha_to)[modnn(rs,(rs->index_of)[lambda[i]] + s[r-i-1])];
			}
		}
		discr_r = (rs->index_of)[discr_r];
		if (discr_r == ((rs->nn))) {
			
			memmove(&b[1],b,(rs->nroots)*sizeof(b[0]));
			b[0] = ((rs->nn));
		} 
		else {
			t[0] = lambda[0];
			for (i = 0 ; i < (rs->nroots); i++) {
				if (b[i] != ((rs->nn)))
					t[i+1] = lambda[i+1] ^ (rs->alpha_to)[modnn(rs,discr_r + b[i])];
				else
					t[i+1] = lambda[i+1];
			}
			if (2 * el <= r + no_eras - 1) {
				el = r + no_eras - el;
				for (i = 0; i <= (rs->nroots); i++)
					b[i] = (lambda[i] == 0) ? ((rs->nn)) : modnn(rs,(rs->index_of)[lambda[i]] - discr_r + (rs->nn));
			} 
			else {
				memmove(&b[1],b,(rs->nroots)*sizeof(b[0]));
				b[0] = ((rs->nn));
			}
			memcpy(lambda,t,((rs->nroots)+1)*sizeof(t[0]));
		}
	}
	

	deg_lambda = 0;
	for(i=0;i<(rs->nroots)+1;i++){
		lambda[i] = (rs->index_of)[lambda[i]];
		if(lambda[i] != ((rs->nn)))
			deg_lambda = i;
	}
	memcpy(&reg[1],&lambda[1],(rs->nroots)*sizeof(reg[0]));
	count = 0;

	for (i = 1,k=(rs->iprim)-1; i <= (rs->nn); i++,k = modnn(rs,k+(rs->iprim))) {
		q = 1;
		for (j = deg_lambda; j > 0; j--){
			if (reg[j] != ((rs->nn))) {
				reg[j] = modnn(rs,reg[j] + j);
				q ^= (rs->alpha_to)[reg[j]];
			}
		}
		if (q != 0)
			continue;
		root[count] = i;
		loc[count] = k;
		
		if(++count == deg_lambda)
			break;
	}
	if (deg_lambda != count) {
		count = -1;
		goto finish;
	}
	deg_omega = deg_lambda-1;
	for (i = 0; i <= deg_omega;i++){
		tmp = 0;
		for(j=i;j >= 0; j--){
			if ((s[i - j] != ((rs->nn))) && (lambda[j] != ((rs->nn))))
				tmp ^= (rs->alpha_to)[modnn(rs,s[i - j] + lambda[j])];
		}
		omega[i] = (rs->index_of)[tmp];
	}

	for (j = count-1; j >=0; j--) {
		num1 = 0;
		for (i = deg_omega; i >= 0; i--) {
			if (omega[i] != ((rs->nn)))
				num1 ^= (rs->alpha_to)[modnn(rs,omega[i] + i * root[j])];
		}
		num2 = (rs->alpha_to)[modnn(rs,root[j] * ((rs->fcr) - 1) + (rs->nn))];
		den = 0;
		

		for (i = ((deg_lambda) < ((rs->nroots)-1) ? (deg_lambda) : ((rs->nroots)-1)) & ~1; i >= 0; i -=2) {
			if(lambda[i+1] != ((rs->nn)))
				den ^= (rs->alpha_to)[modnn(rs,lambda[i+1] + i * root[j])];
		}
		if (num1 != 0 && loc[j] >= (rs->pad)) {
			data[loc[j]-(rs->pad)] ^= (rs->alpha_to)[modnn(rs,(rs->index_of)[num1] + (rs->index_of)[num2] + (rs->nn) - (rs->index_of)[den])];

		}
	}

 finish:
	if(eras_pos != ((void *)0)){
		for(i=0;i<count;i++)
			eras_pos[i] = loc[i];
	}
	return count;
}


void encode_rs(void *p,data_t *data, data_t *bb)
{
	struct rs *rs = (struct rs *)p;
	int i, j;
	data_t feedback;

	memset(bb,0,(rs->nroots)*sizeof(data_t));

	for(i=0;i<(rs->nn)-(rs->nroots)-(rs->pad);i++){

		feedback = (rs->index_of)[data[i] ^ bb[0]];
		if(feedback != ((rs->nn))){
				for(j=1;j<(rs->nroots);j++)
					bb[j] ^= (rs->alpha_to)[modnn(rs,feedback + (rs->genpoly)[(rs->nroots)-j])];
		}

		memmove(&bb[0],&bb[1],sizeof(data_t)*((rs->nroots)-1));
	
		if(feedback != ((rs->nn))) {
			bb[(rs->nroots)-1] = (rs->alpha_to)[modnn(rs,feedback + (rs->genpoly)[0])];
		}
		else
			bb[(rs->nroots)-1] = 0;
	}

}

void free_rs_int(void *p)
{
	struct rs *rs = (struct rs *)p;
  
	free(rs->alpha_to);
	free(rs->index_of);
	free(rs->genpoly);
	free(rs);
}

struct rs *init_rs_int(int symsize,int gfpoly,int fcr,int prim,int nroots,int pad)
{
	struct rs *rs;
		
	int i, j, sr,root,iprim;
		
	rs = ((void *)0);
	if(symsize < 0 || symsize > 8*sizeof(data_t)){
		goto done;
	}

	if(fcr < 0 || fcr >= (1<<symsize))
		goto done;
	if(prim <= 0 || prim >= (1<<symsize))
		goto done;
	if(nroots < 0 || nroots >= (1<<symsize))
		goto done;
	if(pad < 0 || pad >= ((1<<symsize) -1 - nroots))
		goto done;
		
	rs = (struct rs *)malloc(sizeof(struct rs));
	if(rs == ((void *)0))
		goto done;
	rs->mm = symsize;
	rs->nn = (1<<symsize)-1;
	rs->pad = pad;
	rs->alpha_to = (data_t *) malloc(sizeof(data_t)*(rs->nn+1));
	if(rs->alpha_to == ((void *)0)){
		free(rs);
		rs = ((void *)0);
		goto done;
	}
	rs->index_of = (data_t *)malloc(sizeof(data_t)*(rs->nn+1));
	if(rs->index_of == ((void *)0)){
		free(rs->alpha_to);
		free(rs);
		rs = ((void *)0);
		goto done;
	}
	rs->index_of[0] = ((rs->nn));
	rs->alpha_to[((rs->nn))] = 0;
	sr = 1;
	for(i=0;i<rs->nn;i++){
		rs->index_of[sr] = i;
		rs->alpha_to[i] = sr;
		sr <<= 1;
		if(sr & (1<<symsize))
			sr ^= gfpoly;
		sr &= rs->nn;
	}
	if(sr != 1){
		free(rs->alpha_to);
		free(rs->index_of);
		free(rs);
		rs = ((void *)0);
		goto done;
	}
	rs->genpoly = (data_t *)malloc(sizeof(data_t)*(nroots+1));
	if(rs->genpoly == ((void *)0)){
		free(rs->alpha_to);
		free(rs->index_of);
		free(rs);
		rs = ((void *)0);
		goto done;
	}
	rs->fcr = fcr;
	rs->prim = prim;
	rs->nroots = nroots;
	for(iprim=1;(iprim % prim) != 0;iprim += rs->nn)
		;
	rs->iprim = iprim / prim;
	rs->genpoly[0] = 1;
	for (i = 0,root=fcr*prim; i < nroots; i++,root += prim) {
		rs->genpoly[i+1] = 1;
		for (j = i; j > 0; j--){
			if (rs->genpoly[j] != 0)
				rs->genpoly[j] = rs->genpoly[j-1] ^ rs->alpha_to[modnn(rs,rs->index_of[rs->genpoly[j]] + root)];
			else
				rs->genpoly[j] = rs->genpoly[j-1];
		}
		rs->genpoly[0] = rs->alpha_to[modnn(rs,rs->index_of[rs->genpoly[0]] + root)];
	}
	for (i = 0; i <= nroots; i++)
		rs->genpoly[i] = rs->index_of[rs->genpoly[i]];
	done:;
	return rs;
}
