////////////////////////////////////////////////////////////////////////////
//                            **** LZW-AB ****                            //
//               Adjusted Binary LZW Compressor/Decompressor              //
//                  Copyright (c) 2016-2020 David Bryant                  //
//                           All Rights Reserved                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

#ifndef LZWLIB_H_
#define LZWLIB_H_

//int lzw_compress (void (*dst)(int,void*), void *dstctx, int (*src)(void*), void *srcctx);
int lzw_compress(unsigned char *dst, unsigned char *src, size_t src_input_size);
//int lzw_decompress (void (*dst)(int,void*), void *dstctx, int (*src)(void*), void *srcctx);
int lzw_decompress(unsigned char *dst, unsigned char *src, size_t src_size);

#endif /* LZWLIB_H_ */
