/* -*- C++ -*-
 * 
 *  sarconv.cpp - Images in SAR archive are re-scaled to 320x240 size
 *
 *  Copyright (c) 2001-2004 Ogapee. All rights reserved.
 *
 *  ogapee@aqua.dti2.ne.jp
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "SarReader.h"

extern int errno;
extern int scale_ratio_upper;
extern int scale_ratio_lower;

extern size_t rescaleJPEG( unsigned char *original_buffer, size_t length, unsigned char **rescaled_buffer );
extern size_t rescaleBMP( unsigned char *original_buffer, size_t length, unsigned char **rescaled_buffer );

#ifdef main
#undef main
#endif

int main( int argc, char **argv )
{
    SarReader cSR;
    unsigned long length, offset = 0, buffer_length = 0;
    unsigned char *buffer = NULL, *rescaled_buffer = NULL;
    unsigned int i, count;
    bool psp_flag = false;
    bool psp2_flag = false;
    FILE *fp;

    if ( argc >= 4 ){
        while ( argc > 4 ){
            if ( !strcmp( argv[1], "-psp" ) ) psp_flag = true;
            argc--;
            argv++;
        }
        int s = atoi( argv[1] );
        if      ( s == 640 ){ scale_ratio_upper = 1; scale_ratio_lower = 2; }
        else if ( s == 800 ){ scale_ratio_upper = 2; scale_ratio_lower = 5; }
        else argc = 1;
    }
    if ( argc != 4 ){
        fprintf( stderr, "Usage: sarconv [-psp] [-psp2] 640 arc_file rescaled_arc_file\n");
        fprintf( stderr, "Usage: sarconv [-psp] [-psp2] 800 arc_file rescaled_arc_file\n");
        exit(-1);
    }
    if( psp_flag ) { scale_ratio_upper *= 9; scale_ratio_lower *= 8; }
    if( psp2_flag ) { scale_ratio_upper *= 6; scale_ratio_lower *= 5; }

    if ( (fp = fopen( argv[3], "wb" ) ) == NULL ){
        fprintf( stderr, "can't open file %s for writing.\n", argv[3] );
        exit(-1);
    }
    if (cSR.open( argv[2] ) != 0){
        fprintf( stderr, "can't open file %s\n", argv[2] );
        exit(-1);
    }
    count = cSR.getNumFiles();

    SarReader::FileInfo sFI;
    
    for ( i=0 ; i<count ; i++ ){
        printf( "%d/%d\n", i, count );
        sFI = cSR.getFileByIndex( i );
        if ( i==0 ) offset = sFI.offset;
        length = cSR.getFileLength( sFI.name );
        if ( length > buffer_length ){
            if ( buffer ) delete[] buffer;
            buffer = new unsigned char[length];
            buffer_length = length;
        }

        sFI.offset = offset;
        if ( (strlen( sFI.name ) > 3 && !strcmp( sFI.name + strlen( sFI.name ) - 3, "JPG") ||
              strlen( sFI.name ) > 4 && !strcmp( sFI.name + strlen( sFI.name ) - 4, "JPEG") ) ){
            if ( cSR.getFile( sFI.name, buffer ) != length ){
                fprintf( stderr, "file %s can't be retrieved %ld\n", sFI.name, length );
                continue;
            }
            sFI.length = rescaleJPEG( buffer, length, &rescaled_buffer );
            cSR.putFile( fp, i, sFI.offset, sFI.length, sFI.length, true, rescaled_buffer );
        }
        else if ( strlen( sFI.name ) > 3 && !strcmp( sFI.name + strlen( sFI.name ) - 3, "BMP") ){
            if ( cSR.getFile( sFI.name, buffer ) != length ){
                fprintf( stderr, "file %s can't be retrieved %ld\n", sFI.name, length );
                continue;
            }
            sFI.length = rescaleBMP( buffer, length, &rescaled_buffer );
            cSR.putFile( fp, i, sFI.offset, sFI.length, sFI.length, true, rescaled_buffer );
        }
        else{
            cSR.putFile( fp, i, sFI.offset, sFI.length, sFI.original_length, false, buffer );
        }
        
        offset += sFI.length;
    }
    cSR.writeHeader( fp );

    fclose(fp);

    if ( rescaled_buffer ) delete[] rescaled_buffer;
    if ( buffer ) delete[] buffer;
    
    exit(0);
}
