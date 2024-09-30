// Copyright https://github.com/WangXuan95
// source: https://github.com/WangXuan95/JPEG-LS_extension
// 
// A implementation of JPEG-LS extension (ITU-T T.870) image encoder/decoder
// see https://www.itu.int/rec/T-REC-T.870/en
// Warning: This implementation is not fully compliant with ITU-T T.870 standard.
//


const char *title = "JPEG-LS extension ITU-T T.870 (non-std) v0.1\n";



typedef    unsigned char        UI8;



#define    ABS(x)               ( ((x) < 0) ? (-(x)) : (x) )                         // get absolute value
#define    MAX(x, y)            ( ((x)<(y)) ? (y) : (x) )                            // get the minimum value of x, y
#define    MIN(x, y)            ( ((x)<(y)) ? (x) : (y) )                            // get the maximum value of x, y
#define    CLIP(x, min, max)    ( MIN(MAX((x), (min)), (max)) )                      // clip x between min~max

#define    GET2D(ptr,xsz,y,x)   (*( (ptr) + (xsz)*(y) + (x) ))

#define    TQ                   9                                                    // 0~13, see Table G.3
#define    RESET                64

#define    NUM_CONTEXT          1094                                                 // actually only use [2,...,1093]

#define    MAX_CNT              255




void setParameters (int BPP, int NEAR, int *pMAXVAL, int *pRANGE, int *pT1, int *pT2, int *pT3, int *pT4, int *pINIT_A) {
    int tmp;
    *pMAXVAL = (1 << BPP) - 1;
    *pRANGE = (*pMAXVAL + 2*NEAR) / (1 + 2*NEAR) + 1;
    tmp = (MIN(*pMAXVAL, 4095) + 128) >> 8;
    *pT1 =      tmp + 2 + 3 * NEAR;                                // Figure G.10
    *pT2 =  4 * tmp + 3 + 5 * NEAR;                                // Figure G.10
    *pT3 = 17 * tmp + 4 + 7 * NEAR;                                // Figure G.10
    *pT4 = (*pT1) + 2;                                             // Figure G.10
    *pINIT_A = MAX(2, ((*pRANGE+(1<<5))>>6));
}




void sampleNeighbourPixels (int *img, int xsz, int i, int j, int *a, int *b, int *c, int *d, int *e) {
    *b = (i<=0            ) ?  0 : GET2D(img, xsz, i-1, j  );
    *c = (i<=0 || j<=0    ) ? *b : GET2D(img, xsz, i-1, j-1);
    *d = (i<=0 || j+1>=xsz) ? *b : GET2D(img, xsz, i-1, j+1);
    *a = (j<=0            ) ? *b : GET2D(img, xsz, i  , j-1);
    *e = (j<=1            ) ? *a : GET2D(img, xsz, i  , j-2);
}



int gradientQuantize (int val, int T0, int T1, int T2, int T3) {
    int absval = ABS(val);
    int sign = (val < 0);
    if      (absval < T0)
        return 0;
    else if (absval < T1)
        return sign ? -1 : 1;
    else if (absval < T2)
        return sign ? -2 : 2;
    else if (absval < T3)
        return sign ? -3 : 3;
    else
        return sign ? -4 : 4;
}



int getQ (int NEAR, int T1, int T2, int T3, int T4, int a, int b, int c, int d, int e, int *psign, int *pnearq) {
    int Q1 = gradientQuantize(d-b, NEAR+1, T1, T2, T3);
    int Q2 = gradientQuantize(b-c, NEAR+1, T1, T2, T3);
    int Q3 = gradientQuantize(c-a, NEAR+1, T1, T2, T3);
    int Q4 = gradientQuantize(a-e, T4, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF);
    
    int q  = ( 81*Q1 + 9*Q2 + Q3 ) * 3 + Q4;
    
    *psign = (q < 0) ? -1 : +1;
    
    q = ABS(q);
    if (q <= 1)
        q = 0;
    
    *pnearq = NEAR;
    if (NEAR)
        if ( ABS(Q1) + ABS(Q2) + ABS(Q3) >= TQ )
            (*pnearq) ++;
    
    return q;
}



int predict (int a, int b, int c) {
    if      (c >= MAX(a, b))
        return MIN(a, b);
    else if (c <= MIN(a, b))
        return MAX(a, b);
    else
        return a + b - c;
}



int getAct (int nflat, int Nq, int Aq) {
    int act, k;
    if (nflat) {
        for (k=0; (Nq<<k)<Aq+Nq/2 ; k++);
        if (k>0) {
            act = 2 * k;
            if ( 5*(Nq<<k) <= 7*(Aq+Nq/2) )
                act ++;
        } else {
            act = 1;
        }
        act = MIN(act, 11);
    } else {
        act = 0;
    }
    return act;
}



void initContext (int INIT_A, int *N, int *A, int *B, int *C) {
    int i;
    for (i=0; i<NUM_CONTEXT; i++) {
        N[i] = 1;
        A[i] = INIT_A;
        B[i] = 0;
        C[i] = 0;
    }
}



void updateContext (int *pNq, int *pAq, int *pBq, int *pCq, int errval) {
    int Nq=*pNq;
    int Aq=*pAq;
    int Bq=*pBq;
    int Cq=*pCq;
    
    Bq += (Bq > 0) ? -errval : errval;
    if (errval < 0)
        Aq --;
    Aq += ABS(errval);
    if (Nq >= RESET) {
        Nq >>= 1;
        Aq >>= 1;
        if (Bq > 0)
            Bq >>= 1;
        else
            Bq = -((1-Bq)>>1);
    }
    Nq ++;
    if (2*Bq <= -Nq) {
        Bq += Nq;
        if (2*Bq <= -Nq)
            Bq = -(Nq>>1) + 1;
        Cq --;
    } else if (2*Bq > Nq) {
        Bq -= Nq;
        if (2*Bq > Nq)
            Bq = Nq >> 1;
        Cq ++;
    }
    Cq = CLIP(Cq, -128, 127);
    
    *pNq = Nq;
    *pAq = Aq;
    *pBq = Bq;
    *pCq = Cq;
}



typedef struct {
    int  LPScnt;         // init: 2 for root, 4 for tail
    int  MLcnt;          // init: 4 for root, 8 for tail
    int  MPSvalue;       // init: 0 always
} BIN_CTX_t;


void initBinCtxSet (BIN_CTX_t binCtxSet [][8][32]) {
    int i, j, k;
    for (i=0; i<12+256; i++)
        for (j=0; j<8; j++)
            for (k=0; k<32; k++) {
                binCtxSet[i][j][k].LPScnt   = (k==0) ? 2 : 4;
                binCtxSet[i][j][k].MLcnt    = (k==0) ? 4 : 8;
                binCtxSet[i][j][k].MPSvalue = 0;
            }
}



const int Av [31] = { 0x7ab6, 0x7068, 0x6678, 0x5ce2, 0x53a6, 0x4ac0, 0x4230, 0x39f4, 0x33fc, 0x301a, 0x2c4c, 0x2892, 0x24ea, 0x2156, 0x1dd6, 0x1a66, 0x170a, 0x13c0, 0x1086, 0x0d60, 0x0b0e, 0x0986, 0x0804, 0x0686, 0x050a, 0x0394, 0x027e, 0x01c6, 0x013e, 0x0100, 0x0000};
const int Th [30] = { 0x7800, 0x7000, 0x6800, 0x6000, 0x5800, 0x5000, 0x4800, 0x4000, 0x3c00, 0x3800, 0x3400, 0x3000, 0x2c00, 0x2800, 0x2400, 0x2000, 0x1c00, 0x1800, 0x1400, 0x1000, 0x0e00, 0x0c00, 0x0a00, 0x0800, 0x0600, 0x0400, 0x0300, 0x0200, 0x0180, 0x0001 };


typedef struct {
    int  Creg;
    int  Areg;
    UI8  Buf0;
    UI8  Buf1;
    UI8 *pbuf;
} ARI_CODER_t;



ARI_CODER_t newAriEncoder (UI8 *pbuf) {
    ARI_CODER_t co = {0, 0xff*0xff, 0, 0, pbuf};
    return co;
}



ARI_CODER_t newAriDecoder (UI8 *pbuf) {
    ARI_CODER_t co = {0, 0xff*0xff, 0, 0, pbuf};
    co.pbuf += 2;
    co.Creg  = ( *(co.pbuf++) ) * 0xff;
    co.Creg +=   *(co.pbuf++);
    return co;
}



int AriGetAvd (BIN_CTX_t *ctx, int Areg) {                        // Search of suitable Av
    int Prob, Aindex, wct, Avd, Hd;

    Prob = (ctx->LPScnt << 16) / ctx->MLcnt;

    for (Aindex=0; Aindex<30 && (Prob<=Th[Aindex]); Aindex++);
    
    for (wct=0; Areg<(0x8000>>wct); wct++);

    if ( (ctx->MLcnt == MAX_CNT) && (ctx->LPScnt == 1) )
        Avd = 0x0002;
    else
        Avd = Av[Aindex] >> wct;
    
    Hd = 0x8000 >> wct;
    Avd = Areg - Avd;
    if (Avd < Hd)
        Avd = (Avd+Hd) / 2;

    return Avd;
}



void AriUpdateCounter (BIN_CTX_t *ctx, int bin) {
    if (ctx->MLcnt == MAX_CNT){
        if (bin != ctx->MPSvalue) {
            ctx->MLcnt  = (ctx->MLcnt +1) / 2 + 1;
            ctx->LPScnt = (ctx->LPScnt+1) / 2 + 1;
        } else if (ctx->LPScnt!=1) {
            ctx->MLcnt  = (ctx->MLcnt +1) / 2 + 1;
            ctx->LPScnt = (ctx->LPScnt+1) / 2;
        }
    } else {
        ctx->MLcnt ++;
        if (bin != ctx->MPSvalue)
            ctx->LPScnt ++;
    }
    if (ctx->MLcnt < ctx->LPScnt*2) {
        ctx->LPScnt = ctx->MLcnt - ctx->LPScnt;
        ctx->MPSvalue = !(ctx->MPSvalue);
    }
}



void AriEncode (ARI_CODER_t *co, BIN_CTX_t *ctx, int bin) {
    const int Avd = AriGetAvd(ctx, co->Areg);

    if (bin == ctx->MPSvalue) {                                     // Update of Creg and Areg ---------------------------------------------
        co->Areg  = Avd;
    } else {
        co->Areg -= Avd;
        co->Creg += Avd;
    }

    AriUpdateCounter(ctx, bin);                                     // Update of counters ---------------------------------------------

    if ( co->Areg < 0x100 ) {                                       // Renormalization of Areg and Creg and output data bit stream ---------------------------------------------
        if( co->Creg >= 0xff*0xff ) {
            co->Creg -= 0xff*0xff;
            co->Buf0 ++;
            if (co->Buf0 == 0xff) {
                co->Buf0 = 0;
                co->Buf1 ++;
            }
        }
        *(co->pbuf++) = co->Buf1;                                   // AppendByteToStream
        co->Buf1 = co->Buf0;
        co->Buf0 = (UI8)( ((co->Creg >> 8) + co->Creg + 1) >> 8 );
        co->Creg += co->Buf0;
        co->Creg = ((co->Creg&0xff) << 8) - (co->Creg & 0xff);
        co->Areg = (co->Areg << 8) - co->Areg;
    }
}



void AriEncodeFinish (ARI_CODER_t *co) {
    int i;
    for (i=0; i<4; i++) {
        if( co->Creg >= 0xff*0xff ) {
            co->Creg -= 0xff*0xff;
            co->Buf0 ++;
            if (co->Buf0 == 0xff) {
                co->Buf0 = 0;
                co->Buf1 ++;
            }
        }
        *(co->pbuf++) = co->Buf1;                                   // AppendByteToStream
        co->Buf1 = co->Buf0;
        co->Buf0 = (UI8)( ((co->Creg >> 8) + co->Creg + 1) >> 8 );
        co->Creg += co->Buf0;
        co->Creg = ((co->Creg&0xff) << 8) - (co->Creg & 0xff);
    }
}



int AriDecode (ARI_CODER_t *co, BIN_CTX_t *ctx) {
    const int Avd = AriGetAvd(ctx, co->Areg);
    int bin;

    if (co->Creg < Avd) {                                            // Detemination of bin ---------------------------------------------
        co->Areg = Avd;
        bin = ctx->MPSvalue;
    } else {
        co->Creg = co->Creg - Avd;
        co->Areg = co->Areg - Avd;
        bin = !(ctx->MPSvalue);
    }

    AriUpdateCounter(ctx, bin);                                      // Update of counters ---------------------------------------------

    if (co->Areg < 0x100) {                                          // Renormalization of Areg and Creg
        co->Creg = (co->Creg << 8) - co->Creg + *(co->pbuf++);       // GetByte
        co->Areg = (co->Areg << 8) - co->Areg;
    }

    return bin;
}



void treePosUpdate (int *pact, int *pver) {
    int  act = *pact;
    int  ver = *pver;
    
    ver ++;
    if (act < 12) {
        if        ( (act&1) == 0 ) {
            if ( (act==0 && ver>=4) || ver>=6 )
                act ++;
        } else if ( ver>=8 ) {
            act ++;
            ver = 4;
        }
    }
    
    *pact = act;
    *pver = ver;
}



void encodeMerrval (ARI_CODER_t *co, BIN_CTX_t binCtxSet [][8][32], int merrval, int act) {
    int  k, ver=0, subpos=1;
    int bin;

    for (;;) {
        k   = act >> 1;
        bin = ver < (merrval >> k) ;
        AriEncode(co, &binCtxSet[act][ver][0], bin);
        if (!bin)
            break;
        treePosUpdate(&act, &ver);
    }
    
    for (k--; k>=0; k--) {
        bin = (merrval >> k) & 1;
        AriEncode(co, &binCtxSet[act][ver][subpos], bin);
        subpos += bin ? (1<<k) : 1;
    }
}



int decodeMerrval (ARI_CODER_t *co, BIN_CTX_t binCtxSet [][8][32], int act) {
    int  k, ver=0, subpos=1, merrval;
    int bin;

    for (;;) {
        k   = act >> 1;
        bin = AriDecode(co, &binCtxSet[act][ver][0]);
        if (!bin)
            break;
        treePosUpdate(&act, &ver);
    }
    merrval = (ver<<k);
    
    for (k--; k>=0; k--) {
        bin = AriDecode(co, &binCtxSet[act][ver][subpos]);
        if (bin)
            merrval += (1<<k);
        subpos += bin ? (1<<k) : 1;
    }

    return merrval;
}



void putHeader (UI8 **ppbuf, int ysz, int xsz, int BPP, int NEAR) {
    int i;
    for (i=0; title[i]!=0; i++)                   // put title
        *((*ppbuf)++) = (UI8)title[i];
    *((*ppbuf)++) = (UI8)BPP;                  // put BPP
    *((*ppbuf)++) = (UI8)NEAR;                 // put NEAR
    *((*ppbuf)++) = (UI8)(xsz >>  8);          // put image width 
    *((*ppbuf)++) = (UI8)(xsz >>  0);
    *((*ppbuf)++) = (UI8)(ysz >>  8);          // put image height 
    *((*ppbuf)++) = (UI8)(ysz >>  0);
}



// return:  -1:failed  0:success
int getHeader (UI8 **ppbuf, int *pysz, int *pxsz, int *pBPP, int *pNEAR) {
    int i;
    for (i=0; title[i]; i++)                       // check title 
        if ( *((*ppbuf)++) != (UI8)title[i] )
            return -1;
    *pBPP  =   *((*ppbuf)++);                   // get BPP 
    *pNEAR =   *((*ppbuf)++);                   // get NEAR 
    *pxsz  = ( *((*ppbuf)++) ) << 8;            // get image width  
    *pxsz +=   *((*ppbuf)++) ;
    *pysz  = ( *((*ppbuf)++) ) << 8;            // get image height 
    *pysz +=   *((*ppbuf)++) ;
    return 0;
}



// return: positive: JPEG-LS stream length , -1:failed
int JLSxEncode (UI8 *pbuf, int *img, int *imgrcon, int ysz, int xsz, int BPP, int NEAR) {
    int  MAXVAL, RANGE, T1, T2, T3, T4, INIT_A;
    
    int  i, j;
    
    UI8 *buf_base = pbuf;
    
    int  N[NUM_CONTEXT], A[NUM_CONTEXT], B[NUM_CONTEXT], C[NUM_CONTEXT];
    
    BIN_CTX_t binCtxSet [12+256][8] [32];
    
    ARI_CODER_t co;
    
    BPP = MAX(8, BPP);
    if (BPP > 16)
        return -1;
    
    setParameters(BPP, NEAR, &MAXVAL, &RANGE, &T1, &T2, &T3, &T4, &INIT_A);
    
    initContext(INIT_A, N, A, B, C);

    initBinCtxSet(binCtxSet);

    putHeader(&pbuf, ysz, xsz, BPP, NEAR);
    
    co = newAriEncoder(pbuf);
    
    for (i=0; i<ysz; i++) {
        for (j=0; j<xsz; j++) {
            int  x, a, b, c, d, e, px, rx, sign, q, nearq, nflat, errval, merrval, act;
            
            x = GET2D(img, xsz, i, j);                                                         // pixel to be encoded
            sampleNeighbourPixels(imgrcon, xsz, i, j, &a, &b, &c, &d, &e);

            q = getQ(NEAR, T1, T2, T3, T4, a, b, c, d, e, &sign, &nearq);                      // Figure A.2, A.5, A.6, A.7
            nflat = q != 0;                                                                    // Figure A.3, A.4

            px = predict(a, b, c);                                                             // Figure A.8

            if (nflat)
                px = CLIP(px+sign*C[q], 0, MAXVAL);                                            // Figure A.9
            
            if (nflat && (B[q]>0))
                sign *= -1;                                                                    // Figure A.12 (sign flip)
            
            errval = sign * (x - px);                                                          // Figure A.11
            
            if (errval > 0)
                errval =  ( (nearq + errval) / (2*nearq + 1) );                                // Figure A.13
            else
                errval = -( (nearq - errval) / (2*nearq + 1) );
            
            if (errval < 0)                                                                    // Figure A.14
                errval += RANGE;
            if (errval >= (RANGE+1)/2)
                errval -= RANGE;
            
            rx = px + sign*errval*(2*nearq+1);
            
            if      (rx < -nearq)
                rx += RANGE * (2*nearq + 1);
            else if (rx > (MAXVAL+nearq))
                rx -= RANGE * (2*nearq + 1);
            
            rx = CLIP(rx, 0, MAXVAL);
            
            GET2D(imgrcon, xsz, i, j) = rx;
            
            merrval = (errval>=0) ? 2*errval : -2*errval-1;                                    // Figure A.15
            
            act = getAct(nflat, N[q], A[q]);                                                   // Figure A.16

            encodeMerrval(&co, binCtxSet, merrval, act);                                       // Figure A.17

            if (nflat)
                updateContext(&N[q], &A[q], &B[q], &C[q], errval);                             // Figure A.20
        }
    }

    AriEncodeFinish(&co);

    return co.pbuf - buf_base;
}



// return:  -1:failed  0:success
int JLSxDecode (UI8 *pbuf, int *img, int *pysz, int *pxsz, int *pBPP, int *pNEAR) {
    int  BPP, NEAR, MAXVAL, RANGE, T1, T2, T3, T4, INIT_A;
    
    int  ysz, xsz, i, j;
    
    int  N[NUM_CONTEXT], A[NUM_CONTEXT], B[NUM_CONTEXT], C[NUM_CONTEXT];
    
    BIN_CTX_t binCtxSet [12+256][8] [32];
    
    ARI_CODER_t co;

    if ( getHeader(&pbuf, &ysz, &xsz, &BPP, &NEAR) )
        return -1;
    
    if ( BPP < 8 || BPP > 16 )
        return -1;

    co = newAriDecoder(pbuf);
    
    setParameters(BPP, NEAR, &MAXVAL, &RANGE, &T1, &T2, &T3, &T4, &INIT_A);
    
    initContext(INIT_A, N, A, B, C);

    initBinCtxSet(binCtxSet);
    
    for (i=0; i<ysz; i++) {
        for (j=0; j<xsz; j++) {
            int  a, b, c, d, e, px, rx, sign, q, nearq, nflat, errval, merrval, act;

            sampleNeighbourPixels(img, xsz, i, j, &a, &b, &c, &d, &e);

            q = getQ(NEAR, T1, T2, T3, T4, a, b, c, d, e, &sign, &nearq);                      // Figure A.2, A.5, A.6, A.7
            nflat = q != 0;                                                                    // Figure A.3, A.4

            px = predict(a, b, c);                                                             // Figure A.8

            if (nflat)
                px = CLIP(px+sign*C[q], 0, MAXVAL);                                            // Figure A.9
            
            if (nflat && (B[q]>0))
                sign *= -1;                                                                    // Figure A.12 (sign flip)
            
            act = getAct(nflat, N[q], A[q]);                                                   // Figure A.16

            merrval = decodeMerrval(&co, binCtxSet, act);                                      // Figure A.17 (reverse)
            
            if (merrval & 1)                                                                   // Figure A.15 (reverse)
                errval = -((merrval+1) >> 1);
            else
                errval =    merrval >> 1;

            if (nflat)
                updateContext(&N[q], &A[q], &B[q], &C[q], errval);                             // Figure A.20
            
            rx = px + sign*errval*(2*nearq+1);
            
            if      (rx < -nearq)
                rx += RANGE * (2*nearq + 1);
            else if (rx > (MAXVAL+nearq))
                rx -= RANGE * (2*nearq + 1);
            
            rx = CLIP(rx, 0, MAXVAL);
            
            GET2D(img, xsz, i, j) = rx;
        }
    }
    
    *pysz = ysz;
    *pxsz = xsz;
    *pBPP = BPP;
    *pNEAR = NEAR;

    return 0;
}


