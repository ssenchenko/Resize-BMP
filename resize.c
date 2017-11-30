/**
 * resize.c is a copy from 
 * copy.c
 * with important changes
 *
 * Computer Science 50
 * Problem Set 4
 *
 * Copies a BMP piece by piece and (following features have been added)
 * resizes the picture according to the coefficient n which is positive int <= 100
 * reduces the number of direct writing operations to file to 1 per row
 */
       
#include <stdio.h>
#include <stdlib.h>

#include "bmp.h"

int main(int argc, char* argv[])
{
    // ensure proper usage
    if (argc != 4)
    {
        printf("Usage: ./resize n infile outfile\n");
        return 1;
    }
    
    // check the resize factor n
    int n = atoi(argv[1]);
    if (n <= 0 || n > 100) {
        printf("n cannot be 0, negative or greater than 100");
        return 5; // exceptions 1-4 are taken
    }

    // remember filenames
    char* infile = argv[2];
    char* outfile = argv[3];

    // open input file 
    FILE* inptr = fopen(infile, "r");
    if (inptr == NULL)
    {
        printf("Could not open %s.\n", infile);
        return 2;
    }

    // open output file
    FILE* outptr = fopen(outfile, "w");
    if (outptr == NULL)
    {
        fclose(inptr);
        fprintf(stderr, "Could not create %s.\n", outfile);
        return 3;
    }

    // read infile's BITMAPFILEHEADER
    BITMAPFILEHEADER bf;
    fread(&bf, sizeof(BITMAPFILEHEADER), 1, inptr);

    // read infile's BITMAPINFOHEADER
    BITMAPINFOHEADER bi;
    fread(&bi, sizeof(BITMAPINFOHEADER), 1, inptr);

    // ensure infile is (likely) a 24-bit uncompressed BMP 4.0
    if (bf.bfType != 0x4d42 || bf.bfOffBits != 54 || bi.biSize != 40 || 
        bi.biBitCount != 24 || bi.biCompression != 0)
    {
        fclose(outptr);
        fclose(inptr);
        fprintf(stderr, "Unsupported file format.\n");
        return 4;
    }

    // save original size - we'll need it to read the original file
    LONG biWidthOld = bi.biWidth;
    LONG biHeightOld = bi.biHeight;
    // change bi and bf fields of the enlarged bmp
    bi.biWidth *= n;
    bi.biHeight *= n;
    // size of the header remains the same, we need to change the image size only
    bf.bfSize -= bi.biSizeImage;
    // formula for biSizeImage and explaination can be found here
    // https://msdn.microsoft.com/en-us/library/ms969901.aspx
    bi.biSizeImage = ((((bi.biWidth * bi.biBitCount) + 31) & ~31) >> 3) * abs(bi.biHeight);
    bf.bfSize += bi.biSizeImage;
    
    // write outfile's BITMAPFILEHEADER
    fwrite(&bf, sizeof(BITMAPFILEHEADER), 1, outptr);

    // write outfile's BITMAPINFOHEADER
    fwrite(&bi, sizeof(BITMAPINFOHEADER), 1, outptr);

    // determine padding for scanlines for the old width
    int padding_old =  (4 - (biWidthOld * sizeof(RGBTRIPLE)) % 4) % 4;
    // determine padding for scanlines for the NEW width
    int padding_new =  (4 - (bi.biWidth * sizeof(RGBTRIPLE)) % 4) % 4;
    
    // temporary storage to read one tripple at a time
    RGBTRIPLE triple;
    // temporary storage to write the scanline of the new file
    void *line = malloc(bi.biWidth * sizeof(RGBTRIPLE) + padding_new);
    if (line == NULL) 
        return 6;
    RGBTRIPLE *scanline = (RGBTRIPLE *) line; // treat allocated memory as a RGBTRIPLE array to fill it in tripples
    BYTE *byteline = (BYTE *) line; // treat allocated memory as a BYTE array to add padding
    
    // iterate over infile's scanlines of the original file
    for (int i = 0, biHeight = abs(biHeightOld); i < biHeight; i++)
    {
        int q = 0; // counter for the new scanline
        // iterate over pixels in scanline in the original file
        for (int j = 0; j < biWidthOld; j++)
        {
            // read RGB triple from infile
            fread(&triple, sizeof(RGBTRIPLE), 1, inptr);

            // add n RGB tripleS to a new scanline
            for (int k = 0; k <= n - 1; k++) {
                *(scanline + q++) = triple; // easy to add tripples
            }
        }
        // add padding to the new scanline if any are needed
        for (int k = 0; k < padding_new; k++)
             *(byteline + q * sizeof(RGBTRIPLE) + k) = 0x00; // compute pointer to the last element in BYTEs
        
        // write a byteline to a new file n times
        for (int k = 0; k <= n - 1; k++) {
            // only one writing operation needed
            fwrite(byteline, sizeof(BYTE), bi.biWidth * sizeof(RGBTRIPLE) + padding_new, outptr);
        }
        
        // skip over padding in the original file, if any
        // because you cannot iterate over them
        fseek(inptr, padding_old, SEEK_CUR);
    }
    
    free(line);

    // close infile
    fclose(inptr);

    // close outfile
    fclose(outptr);

    // that's all folks
    return 0;
}
