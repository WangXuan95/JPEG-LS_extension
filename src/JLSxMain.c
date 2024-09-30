// Copyright https://github.com/WangXuan95
// source: https://github.com/WangXuan95/JPEG-LS_extension
// 
// A implementation of JPEG-LS extension (ITU-T T.870) image encoder/decoder
// see https://www.itu.int/rec/T-REC-T.870/en
// Warning: This implementation is not fully compliant with ITU-T T.870 standard.
//


#include <stdio.h>


extern int JLSxEncode (unsigned char *pbuf, int *img, int *imgrcon, int ysz, int xsz, int BPP, int NEAR);     // import from JLSx.c
extern int JLSxDecode (unsigned char *pbuf, int *img, int *pysz, int *pxsz, int *pBPP, int *pNEAR);           // import from JLSx.c



// return:   -1:failed   0:success
int loadPGMfile (const char *filename, int *img, int *pysz, int *pxsz, int *pBPP) {
    int i, maxval=-1;
    FILE *fp;

    *pysz = -1;
    *pxsz = -1;
    
    if ( (fp = fopen(filename, "rb")) == NULL )
        return -1;

    if ( fgetc(fp) != 'P' ) {
        fclose(fp);
        return -1;
    }
    
    if ( fgetc(fp) != '5' ) {
        fclose(fp);
        return -1;
    }

    if ( fscanf(fp, "%d", pxsz) < 1 ) {
        fclose(fp);
        return -1;
    }
    
    if ( fscanf(fp, "%d", pysz) < 1 ) {
        fclose(fp);
        return -1;
    }

    if ( fscanf(fp, "%d", &maxval) < 1 ) {
        fclose(fp);
        return -1;
    }
    
    if ((*pxsz) < 1 || (*pysz) < 1 || maxval < 1 || maxval > 65535) {
        fclose(fp);
        return -1;
    }
    
    for ( (*pBPP)=1 ; (1<<(*pBPP))<=maxval ; (*pBPP)++ );
    
    fgetc(fp);                                                 // skip a white char
    
    for (i=0; i<(*pxsz)*(*pysz); i++) {
        int pixel;
        
        if (feof(fp)) {                                        // pixels not enough
            fclose(fp);
            return -1;
        }
        
        pixel = fgetc(fp) & 0xFF;
        if (maxval >= 256) {                                   // two bytes per pixel
            pixel <<= 8;
            pixel += fgetc(fp) & 0xFF;
        }
        
        if (pixel < 0)
            pixel = 0;
        if (pixel > maxval)
            pixel = maxval;
        
        *(img++) = pixel;
    }

    fclose(fp);
    return 0;
}



// return:   -1:failed   0:success
int writePGMfile (const char *filename, const int *img, const int ysz, const int xsz, const int BPP) {
    int i, maxval;
    FILE *fp;
    
    if (xsz < 1 || ysz < 1 || BPP < 1 || BPP > 16)
        return -1;
    
    maxval = (1<<BPP) - 1;
    
    if ( (fp = fopen(filename, "wb")) == NULL )
        return -1;

    fprintf(fp, "P5\n%d %d\n%d\n", xsz, ysz, maxval);

    for (i=0; i<xsz*ysz; i++) {
        int pixel = *(img++);
        
        if (pixel < 0)
            pixel = 0;
        if (pixel > maxval)
            pixel = maxval;
        
        if (maxval >= 256)                                     // two bytes per pixel
            if ( fputc( ((pixel>>8)&0xFF) , fp) == EOF ) {
                fclose(fp);
                return -1;
            }
        
        if ( fputc( (pixel&0xFF) , fp) == EOF ) {
            fclose(fp);
            return -1;
        }
    }

    fclose(fp);
    return 0;
}



// return:   -1:failed   positive:file length
int readBytesFromFile (const char *filename, unsigned char *buffer, const int len_limit) {
    const unsigned char *buffer_base    = buffer;
    const unsigned char *buffer_end_ptr = buffer + len_limit;
    
    FILE *fp;
    
    if ( (fp = fopen(filename, "rb")) == NULL )
        return -1;

    while (buffer<buffer_end_ptr) {
        if (feof(fp))
            return buffer - buffer_base;
        *(buffer++) = (unsigned char)fgetc(fp);
    }

    return -1;
}



// return:   -1:failed   0:success
int writeBytesToFile (const char *filename, const unsigned char *buffer, const int len) {
    const unsigned char *buffer_end_ptr = buffer + len;
    
    FILE *fp;
    
    if ( (fp = fopen(filename, "wb")) == NULL )
        return -1;

    for (; buffer<buffer_end_ptr; buffer++) {
        if ( fputc( *buffer , fp) == EOF ) {
            fclose(fp);
            return -1;
        }
    }

    fclose(fp);
    return 0;
}




// return:  1:match   0:mismatch
int suffix_match (const char *string, const char *suffix) {
    const char *p1, *p2;
    for (p1=string; *p1; p1++);
    for (p2=suffix; *p2; p2++);
    while (*p1 == *p2) {
        if (p2 <= suffix)
            return 1;
        if (p1 <= string)
            return 0;
        p1 --;
        p2 --;
    }
    return 0;
}




#define   MAX_YSZ            8192
#define   MAX_XSZ            8192
#define   JLS_LENGTH_LIMIT   (MAX_YSZ*MAX_XSZ*4)



int main (int argc, char **argv) {

    static           int img        [MAX_YSZ*MAX_XSZ];
    static           int imgrcon    [MAX_YSZ*MAX_XSZ];
    static unsigned char jls_buffer [JLS_LENGTH_LIMIT] = {0};

    const char *src_fname=NULL, *dst_fname=NULL;

    int ysz=-1 , xsz=-1 , bpp=-1 , near=0 , jls_length;


    printf("JPEG-LS extension ITU-T T.870 (a non-standard version)\n");
    printf("see https://github.com/WangXuan95/JPEG-LS_extension\n");
    printf("\n");


    if (argc < 3) {                                    // illegal arguments: print USAGE and exit
        printf("Usage:\n");
        printf("    Encode:\n");
        printf("        %s  <input-image-file(.pgm)>  <output-file(.jlsxn)>  [<near>]\n" , argv[0] );
        printf("    Decode:\n");
        printf("        %s  <input-file(.jlsxn)>  <output-image-file(.pgm)>\n" , argv[0] );
        printf("\n");
        printf("Note: the file suffix \".jlsxn\" means JPEG-LS eXtension Non-standard\n");
        printf("\n");
        return -1;
    }
    
    
    src_fname = argv[1];
    dst_fname = argv[2];
    
    
    if (argc >= 4) {
        if ( sscanf(argv[3], "%d", &near) <= 0 )
            near = 0;
    }


    printf("  input  file        = %s\n" , src_fname);
    printf("  output file        = %s\n" , dst_fname);


    if ( suffix_match(src_fname, ".pgm") || suffix_match(src_fname, ".pnm") || suffix_match(src_fname, ".ppm") ) {                                   // src file is a pgm, encode
        
        printf("  near               = %d\n"       , near);

        if ( loadPGMfile(src_fname, img, &ysz, &xsz, &bpp) ) {
            printf("open %s failed\n", src_fname);
            return -1;
        }
    
        printf("  image shape        = %d x %d\n"  , xsz , ysz );
        printf("  original bpp       = %d\n"       , bpp );
        printf("  original size      = %d bits\n"  , xsz*ysz*bpp );

        jls_length = JLSxEncode(jls_buffer, img, imgrcon, ysz, xsz, bpp, near);
        
        if (jls_length < 0) {
            printf("encode failed\n");
            return -1;
        }

        printf("  compressed length  = %d Bytes\n" , jls_length );
        printf("  compression ratio  = %.5f\n"     , xsz*ysz*bpp/(8.0*jls_length) );
        printf("  compressed bpp     = %.5f\n"     , (8.0*jls_length)/(xsz*ysz) );

        if ( writeBytesToFile(dst_fname, jls_buffer, jls_length) ) {
            printf("write %s failed\n", dst_fname);
            return -1;
        }

    } else {
        
        jls_length = readBytesFromFile(src_fname, jls_buffer, JLS_LENGTH_LIMIT);

        if ( jls_length < 0 ) {
            printf("open %s failed\n", src_fname);
            return -1;
        }
        
        printf("  JLS length         = %d Bytes\n" , jls_length );

        if ( JLSxDecode(jls_buffer, img, &ysz, &xsz, &bpp, &near) < 0 ) {
            printf("decode failed\n");
            return -1;
        }
    
        printf("  near               = %d\n"       , near);
        printf("  image shape        = %d x %d\n"  , xsz , ysz );
        printf("  image bpp          = %d\n"       , bpp );
        printf("  image size         = %d bits\n"  , xsz*ysz*bpp );
        printf("  compression ratio  = %.5f\n"     , xsz*ysz*bpp/(8.0*jls_length) );
        printf("  compressed bpp     = %.5f\n"     , (8.0*jls_length)/(xsz*ysz) );

        if ( writePGMfile(dst_fname, img, ysz, xsz, bpp) ) {
            printf("write %s failed\n", dst_fname);
            return -1;
        }
    }

    return 0;
}
