/*
 * LibrePIA is an attempt to create a LIBrary to Read and Edit PIA plot files (.pc3, .pmp, .stb, .ctb).
 *
 * Copyright (C) 2020 Paolo Caroni <kenren89@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 */

/* WARNING, the LibrePIA project at the moment is only a proof of concept*/

/* include dependancy*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>
/* include header files*/
#include "ctb.h"

/* declare header and info common to all PIA files*/
unsigned char header[37],header2[11],checksum[12];
unsigned int readed_decompressed_size=0, readed_compressed_size=0, readed_Adler32=0;
int k=0,PIA_uncompressed_line_number=0;
/*int PIA_ver=0, subclass_ver=0; unused*/


/* obtain information of version,file subclass type and sizes*/
 unsigned long read_header(char *infilename)
 {
    /* open compressed PIA file but without zlib*/
    FILE *infile = fopen(infilename, "rb");

    /* READ header and other info*/
    fread(header, 1, sizeof(header), infile);
    fread(header2, 1, sizeof(header2), infile);
    fread(checksum, 1, sizeof(checksum), infile);

    /* verify if it's a PIA file*/
    if (header[0]=='P' && header[1]=='I' && header[2]=='A' && header[3]=='F' && header[4]=='I' && header[5]=='L' && header[6]=='E')
    {
    }
    /* if in the header there is something wrong*/
    else
    {
    fprintf(stderr, "WARNING:This isn't a PIA file\n\n");
    }

    /* verify PIA file version*/
    /* PIA version*/
    if (header[15]=='2' && header[16]=='.' && header[17]=='0')
    {
    /*PIA_ver=2; unused the actual aim is 2.0 only*/
    }
    /* PIA version isn't 2.0*/
    else
    {
    fprintf(stderr, "WARNING:unsupported PIA file version\n\n");
    }

    /* verify subclass version*/
    /* subclass version*/
    if (header[25]=='1')
    {
    /*subclass_ver=1; unused the actual aim is 1 only*/
    }
    /* subclass_ver isn't 1*/
    else
    {
    fprintf(stderr, "WARNING:unsupported sub-class version\n\n");
    }

    /* readed_Adler32 set to the combination of the number 49-50-51-52 bytes*/
    readed_Adler32 = (readed_Adler32 << 8) + checksum[3];
    readed_Adler32 = (readed_Adler32 << 8) + checksum[2];
    readed_Adler32 = (readed_Adler32 << 8) + checksum[1];
    readed_Adler32 = (readed_Adler32 << 8) + checksum[0];

    /* readed_decompressed_size set to the combination of the number 53-54-55-56 bytes*/
    readed_decompressed_size = (readed_decompressed_size << 8) + checksum[7];
    readed_decompressed_size = (readed_decompressed_size << 8) + checksum[6];
    readed_decompressed_size = (readed_decompressed_size << 8) + checksum[5];
    readed_decompressed_size = (readed_decompressed_size << 8) + checksum[4];

    /* readed_compressed_size set to the combination of the number 57-58-59-60 bytes*/
    readed_compressed_size = (readed_compressed_size << 8) + checksum[11];
    readed_compressed_size = (readed_compressed_size << 8) + checksum[10];
    readed_compressed_size = (readed_compressed_size << 8) + checksum[9];
    readed_compressed_size = (readed_compressed_size << 8) + checksum[8];

    /* debug function*/
    #if DEBUG
    printf("expected compressed size: %d\n", readed_compressed_size);
    printf("expected decompressed size: %d\n", readed_decompressed_size-48-4-4-4);
    #endif

    /* close input and output file*/
    fclose(infile);
 }


/* funtion for unzip PIA file*/
 int decompress_data(char *infilename, char *outfilename)
 {
    /* open compressed PIA file*/
    gzFile infile = gzopen(infilename, "rb");
    /* create uncompressed file*/
    FILE *outfile = fopen(outfilename, "wb");
    /* verify if files exist*/
    if (!infile || !outfile) return -1;

    /* declare buffer, buffer size is the compressed size*/
    char buffer[readed_compressed_size];
    /* declare data, data size is equal to decompressed size without header(48byte), Adler 32 checksum(4byte), 
    decompressed size (4byte) and compressed size(4byte), in total (60byte)*/
    char data[readed_decompressed_size-48-4-4-4];
    /* number of byte readed*/
    int num_read = 0;

    /* first 48 bytes = header (not compressed, composed by "PIAFILEVERSION_2.0,???VER1,compress/r/npmzlibcodec)
    next 4 bytes Adler32 checksum
    next 4 bytes (unsigned int) decompressed stream size
    next 4 bytes (unsigned int) compressed stream size*/
    gzseek (infile, 48+4+4+4, SEEK_CUR);

    /* read input compressed data file*/
    while ((num_read = gzread(infile, buffer, sizeof(buffer))) > 0)
    {
       /* inflate buffer into data*/
       /* zlib struct*/
       z_stream infstream;
       infstream.zalloc = Z_NULL;
       infstream.zfree = Z_NULL;
       infstream.opaque = Z_NULL;

       /* setup "buffer" as the input and "data" as the decompressed output*/
       /* size of input*/
       infstream.avail_in = (uInt)sizeof(buffer);
       /* input char array*/
       infstream.next_in = (Bytef *)buffer;
       /* size of output*/
       infstream.avail_out = (uInt)sizeof(data);
       /* output char array*/
       infstream.next_out = (Bytef *)data;
         
       /* the real DE-compression work*/
       inflateInit(&infstream);
       inflate(&infstream, Z_NO_FLUSH);
       inflateEnd(&infstream);

       /* write uncompressed data*/
       fwrite(data, 1, readed_decompressed_size-60, outfile);

       /* Obtain information of output file size*/
       unsigned long output_file_size = ftell(outfile);

       /* debug*/
       #if DEBUG
       printf("readed %d compressed bytes\n", num_read);
       printf("writed %d decompressed bytes\n", output_file_size);
       #endif

       /* count number of line in uncompressed stream*/
       for(k=0;data[k];k++)
       {
       if(data[k]=='\n'){PIA_uncompressed_line_number++;}
       }
       #if DEBUG
       printf("uncompressed data splitted in %d lines\n", PIA_uncompressed_line_number);
       #endif

    }

    /* close input and output file*/
    gzclose(infile);
    fclose(outfile);
 }

/* funtion for read CTB uncompressed text form file*/
 int ctb_parser(char *infilename)
 {
     fprintf(stderr, "Work in progress...\n");

    /* remove old value from k*/
    k=0;

    /* open decompressed PIA file, this is very stupid, there is the data string,
    but I don't know how analyze it... needs more C abilities*/
    FILE *infile = fopen(infilename, "rb");

    /* Parse ctb file*/
    /* first line, description, can contain space*/
    fgets(line_buffer,sizeof(line_buffer),infile);
    sscanf(line_buffer,"description=%[^\n]",&description);
    /* second line, aci_table_available, always TRUE*/
    fgets(line_buffer,sizeof(line_buffer),infile);
    sscanf(line_buffer,"aci_table_available=%c",&aci_table_available);
    /* third line, scale_factor*/
    fgets(line_buffer,sizeof(line_buffer),infile);
    sscanf(line_buffer,"scale_factor=%f",&scale_factor);
    /* fourth line, apply factor*/
    fgets(line_buffer,sizeof(line_buffer),infile);
    sscanf(line_buffer,"apply_factor=%c",&apply_factor);
    /* fifth line, custom_lineweight_display_units*/
    fgets(line_buffer,sizeof(line_buffer),infile);
    sscanf(line_buffer,"custom_lineweight_display_units=%d",&custom_lineweight_display_units);


    #if DEBUG
    printf("description=%s\n",description);
    if (aci_table_available==84)
    {
    printf("aci_table_available=TRUE\n");
    }
    else if (aci_table_available==70)
    {
    printf("aci_table_available=FALSE\n");
    }
    else
    {
    printf("aci_table_available=%d\n",aci_table_available);
    }
    printf("scale_factor=%1.1f\n",scale_factor);
    if (apply_factor==84)
    {
    printf("apply_factor=TRUE\n");
    }
    else if (apply_factor==70)
    {
    printf("apply_factor=FALSE\n");
    }
    else
    {
    printf("apply_factor=%d\n",apply_factor);
    }
    printf("custom_lineweight_display_units=%d\n",custom_lineweight_display_units);
    #endif

    /* line 6 to 262 read aci_table as matrix*/
    fgets(line_buffer,sizeof(line_buffer),infile);
    if(line_buffer[0]=='a' && line_buffer[1]=='c' && line_buffer[2]=='i'&& line_buffer[3]=='_' && line_buffer[4]=='t' && line_buffer[5]=='a')
    {
    }
    else
    {
    #if DEBUG
    printf("aci_table on wrong line\n");
    printf("%s\n",line_buffer);
    #endif
    }
    /* get line 7 to 261 (255 values)*/
    for(k=0;k<255;k++)
    {
    /* stupid code, try new approach*/
    fgets(aci_table[k],20,infile);
    #if DEBUG
    printf("%s",aci_table[k]);
    #endif
    }
    /* line 262 end of aci_table*/
    fgets(line_buffer,sizeof(line_buffer),infile);
    if(line_buffer[0]=='}')
    {
    }
    else
    {
    #if DEBUG
    printf("end of aci_table on wrong line\n");
    printf("expected } obtained %c\n",line_buffer[0]);
    #endif
    }

    /* line 263 start of plot_style sctruct*/
    fgets(line_buffer,sizeof(line_buffer),infile);
    if(line_buffer[0]=='p' && line_buffer[1]=='l' && line_buffer[2]=='o'&& line_buffer[3]=='t' && line_buffer[4]=='_' && line_buffer[5]=='s')
    {
    }
    else
    {
    #if DEBUG
    printf("plot_style on wrong line\n");
    printf("%s\n",line_buffer);
    #endif
    }

    k=0;
    for(k=0;k<254;k++)
    {
       /* color value color init (number)*/
       fgets(line_buffer,sizeof(line_buffer),infile);
/* known bug segmentation fault
       sscanf(line_buffer,"%d{",color_number[k]);
       //#if DEBUG
       if(k!=color_number[k])
       {
       printf("wrong color number, expected \"%d\" found \"%d\"\n", k, color_number[k]);
       }
       //#endif
*/
       /* color value name*/
       fgets(line_buffer,sizeof(line_buffer),infile);
       sscanf(line_buffer,"  name=%12s",&name[k]);
       #if DEBUG
       printf("name=%s\n",name[k]);
       #endif
       /* color value localized_name*/
       fgets(line_buffer,sizeof(line_buffer),infile);
       sscanf(line_buffer,"  localized_name=%12s",&localized_name[k]);
       #if DEBUG
       printf("localized_name=%s\n",localized_name[k]);
       #endif
       /* color value description*/
       fgets(line_buffer,sizeof(line_buffer),infile);
       sscanf(line_buffer,"  description=%1025s",&color_description[k]);
       #if DEBUG
       printf("description=%s\n",color_description[k]);
       #endif
       /* color value color*/
       fgets(line_buffer,sizeof(line_buffer),infile);
       sscanf(line_buffer,"  color=%d",&color[k]);
       #if DEBUG
       printf("color=%d\n",color[k]);
       #endif
       /* Verify if mode_color is present*/
       if (PIA_uncompressed_line_number>4700)
       {
       /* color value mode_color*/
       fgets(line_buffer,sizeof(line_buffer),infile);
       sscanf(line_buffer,"  mode_color=%d",&mode_color[k]);
       #if DEBUG
       printf("mode_color=%d\n",mode_color[k]);
       #endif
       }
       /* color value color_policy*/
       fgets(line_buffer,sizeof(line_buffer),infile);
       sscanf(line_buffer,"  color_policy=%d",&color_policy[k]);
       #if DEBUG
       printf("color_policy=%d\n",color_policy[k]);
       #endif
       /* color value physical_pen_number*/
       fgets(line_buffer,sizeof(line_buffer),infile);
       sscanf(line_buffer,"  physical_pen_number=%d",&physical_pen_number[k]);
       #if DEBUG
       printf("physical_pen_number=%d\n",physical_pen_number[k]);
       #endif
       /* color value virtual_pen_number*/
       fgets(line_buffer,sizeof(line_buffer),infile);
       sscanf(line_buffer," virtual_pen_number=%d",&virtual_pen_number[k]);
       #if DEBUG
       printf("virtual_pen_number=%d\n",virtual_pen_number[k]);
       #endif
       /* color value screen*/
       fgets(line_buffer,sizeof(line_buffer),infile);
       sscanf(line_buffer," screen=%d",&screen[k]);
       #if DEBUG
       printf("screen=%d\n",screen[k]);
       #endif
       /* color value linepattern_size*/
       fgets(line_buffer,sizeof(line_buffer),infile);
       sscanf(line_buffer," linepattern_size=%f",&linepattern_size[k]);
       #if DEBUG
       printf("linepattern_size=%1.1f\n",linepattern_size[k]);
       #endif
       /* color value linetype*/
       fgets(line_buffer,sizeof(line_buffer),infile);
       sscanf(line_buffer," linetype=%d",&linetype[k]);
       #if DEBUG
       printf("linetype=%d\n",linetype[k]);
       #endif
       /* color value adaptive_linetype (TRUE or FALSE)*/
       fgets(line_buffer,sizeof(line_buffer),infile);
       sscanf(line_buffer," adaptive_linetype=%c",&adaptive_linetype[k]);
       #if DEBUG
       if (aci_table_available==84)
       {
       printf("adaptive_linetype=TRUE\n");
       }
       else if (aci_table_available==70)
       {
       printf("adaptive_linetype=FALSE\n");
       }
       else
       {
       printf("adaptive_linetype=%d\n",adaptive_linetype[k]);
       }
       #endif
       /* color value lineweight*/
       fgets(line_buffer,sizeof(line_buffer),infile);
       sscanf(line_buffer," lineweight=%d",&lineweight[k]);
       #if DEBUG
       printf("lineweight=%d\n",lineweight[k]);
       #endif
       /* color value fill_style*/
       fgets(line_buffer,sizeof(line_buffer),infile);
       sscanf(line_buffer," fill_style=%d",&fill_style[k]);
       #if DEBUG
       printf("fill_style=%d\n",fill_style[k]);
       #endif
       /* color value end_style*/
       fgets(line_buffer,sizeof(line_buffer),infile);
       sscanf(line_buffer," end_style=%d",&end_style[k]);
       #if DEBUG
       printf("end_style=%d\n",end_style[k]);
       #endif
       /* color value join_style*/
       fgets(line_buffer,sizeof(line_buffer),infile);
       sscanf(line_buffer," join_style=%d",&join_style[k]);
       #if DEBUG
       printf("join_style=%d\n",join_style[k]);
       #endif
       /* color value end of color }*/
       fgets(line_buffer,sizeof(line_buffer),infile);


    
       }


 }

 /* proof of concept for decompress PIA file in a text form,
 can be used for all PIA file (ctb, stb, pc3, pmp)*/
 int main(int argc, char **argv)
 {
    read_header(argv[1]);
    decompress_data(argv[1],argv[2]);
    /* Verify subclass type*/
    if (header[19]=='C' && header[20]=='T' && header[21]=='B')
    {
    ctb_parser(argv[2]);
    }

    else if (header[19]=='S' && header[20]=='T' && header[21]=='B')
    {
    fprintf(stderr, "Sorry, the .stb subclass type isn't yet supported\n\n");
    }

    else if (header[19]=='P' && header[20]=='C' && header[21]=='3')
    {
    fprintf(stderr, "Sorry, the .pc3 subclass type isn't yet supported.\nMaybe .pc3 would be never supported since is correlated to .hdi file.\n\n");
    }

    else if (header[19]=='P' && header[20]=='M' && header[21]=='P')
    {
    fprintf(stderr, "Sorry, the .pmp subclass type isn't yet supported.\nThe support of .pmp. isn't a priority.\n\n");
    }

    /* if in the header there is something wrong*/
    else
    {
    fprintf(stderr, "WARNING:This isn't a known subclass type\n\n");
    }
 }


