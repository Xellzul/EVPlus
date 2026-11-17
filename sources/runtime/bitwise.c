// COMPILE: -lang c++ -bool off
/* bitwise.c -- libogg 1.0rc3 src/bitwise.c, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in ogg.h. BurgerLib's copy (4 KB growth, allocation checks, bit counts clamped to 32, unsigned fields); altered: comments and the functions the exe does not link removed. Notes: Research/notes/burgervorbis.md */

#include <string.h>
#include <stdlib.h>
#include "ogg.h"

#define BUFFER_INCREMENT 4096

static const uint32_t mask[]=
{0x00000000,0x00000001,0x00000003,0x00000007,0x0000000f,
 0x0000001f,0x0000003f,0x0000007f,0x000000ff,0x000001ff,
 0x000003ff,0x000007ff,0x00000fff,0x00001fff,0x00003fff,
 0x00007fff,0x0000ffff,0x0001ffff,0x0003ffff,0x0007ffff,
 0x000fffff,0x001fffff,0x003fffff,0x007fffff,0x00ffffff,
 0x01ffffff,0x03ffffff,0x07ffffff,0x0fffffff,0x1fffffff,
 0x3fffffff,0x7fffffff,0xffffffff };

/* From libogg 1. */
// FUNCTION: EVNOVA 0x00543cf0
void oggpack_writeinit(oggpack_buffer *b){
  unsigned char *buffer;
  FastMemSet(b,0,sizeof(*b));
  buffer=(unsigned char *)_ogg_malloc(BUFFER_INCREMENT);
  if(buffer){
    b->ptr=buffer;
    b->buffer=buffer;
    b->storage=BUFFER_INCREMENT;
    buffer[0]='\0';
  }
}

/* From libogg 1. */
// FUNCTION: EVNOVA 0x00543d40
void oggpack_writeclear(oggpack_buffer *b){
  _ogg_free(b->buffer);
  FastMemSet(b,0,sizeof(*b));
}

/* From libogg 1. */
// FUNCTION: EVNOVA 0x00543d80
void oggpack_readinit(oggpack_buffer *b,unsigned char *buf,int bytes){
  FastMemSet(b,0,sizeof(*b));
  b->buffer=buf;
  b->ptr=buf;
  b->storage=bytes;
}

/* From libogg 1. */
// FUNCTION: EVNOVA 0x00543dc0
void oggpack_write(oggpack_buffer *b,uint32_t value,unsigned int bits){
  uint32_t storage=b->storage;
  uint32_t endbyte=b->endbyte;
  uint32_t endbit;
  unsigned char *ptr;

  if(endbyte+4>=storage){
    unsigned char *buffer=(unsigned char *)_ogg_realloc(b->buffer,storage+BUFFER_INCREMENT);
    b->buffer=buffer;
    if(!buffer){
      b->storage=0;
      b->ptr=0;
      b->endbit=0;
      b->endbyte=0;
      return;
    }
    b->storage=storage+BUFFER_INCREMENT;
    b->ptr=buffer+endbyte;
  }

  if(bits>32)bits=32;
  value&=mask[bits];
  endbit=b->endbit;
  bits+=endbit;
  ptr=b->ptr;

  ptr[0]|=value<<endbit;

  if(bits>=8){
    ptr[1]=value>>(8-endbit);
    if(bits>=16){
      ptr[2]=value>>(16-endbit);
      if(bits>=24){
	ptr[3]=value>>(24-endbit);
	if(bits>=32){
	  if(endbit)
	    ptr[4]=value>>(32-endbit);
	  else
	    ptr[4]=0;
	}
      }
    }
  }

  b->endbyte=endbyte+bits/8;
  b->ptr=ptr+bits/8;
  b->endbit=bits&7;
}

/* From libogg 1. */
// FUNCTION: EVNOVA 0x00543f20
int32_t oggpack_look(oggpack_buffer *b,unsigned int bits){
  uint32_t ret;
  uint32_t m;
  uint32_t storage;

  if(bits>32)bits=32;
  m=mask[bits];
  bits+=b->endbit;
  storage=b->storage;

  if(b->endbyte+4>=storage){
    if(b->endbyte+(bits-1)/8>=storage)return(-1);
  }

  ret=b->ptr[0]>>b->endbit;
  if(bits>8){
    ret|=b->ptr[1]<<(8-b->endbit);
    if(bits>16){
      ret|=b->ptr[2]<<(16-b->endbit);
      if(bits>24){
	ret|=b->ptr[3]<<(24-b->endbit);
	if(bits>32)
	  ret|=b->ptr[4]<<(32-b->endbit);
      }
    }
  }
  return(m&ret);
}

/* From libogg 1. */
// FUNCTION: EVNOVA 0x00544000
void oggpack_adv(oggpack_buffer *b,unsigned int bits){
  bits+=b->endbit;
  b->ptr+=bits/8;
  b->endbyte+=bits/8;
  b->endbit=bits&7;
}

/* From libogg 1. */
// FUNCTION: EVNOVA 0x00544020
int32_t oggpack_read(oggpack_buffer *b,unsigned int bits){
  uint32_t ret;
  uint32_t m;
  uint32_t endbit;
  unsigned char *ptr;
  uint32_t bytes;

  if(bits>32)bits=32;
  m=mask[bits];
  endbit=b->endbit;
  bits+=endbit;
  ptr=b->ptr;

  if(b->endbyte+4>=b->storage){
    ret=-1UL;
    if(b->endbyte+(bits-1)/8>=b->storage)goto overflow;
  }

  ret=ptr[0]>>endbit;
  if(bits>8){
    ret|=ptr[1]<<(8-endbit);
    if(bits>16){
      ret|=ptr[2]<<(16-endbit);
      if(bits>24){
	ret|=ptr[3]<<(24-endbit);
	if(bits>32 && endbit){
	  ret|=ptr[4]<<(32-endbit);
	}
      }
    }
  }
  ret&=m;

 overflow:

  bytes=bits/8;
  b->ptr=ptr+bytes;
  b->endbyte+=bytes;
  b->endbit=bits&7;
  return(ret);
}

/* From libogg 1. */
// FUNCTION: EVNOVA 0x00544110
int32_t oggpack_read1(oggpack_buffer *b){
  uint32_t ret;
  uint32_t endbyte=b->endbyte;
  uint32_t endbit=b->endbit;

  if(endbyte>=b->storage){
    ret=-1UL;
    goto overflow;
  }

  ret=(b->ptr[0]>>endbit)&1;

 overflow:

  endbit++;
  if(endbit>=8){
    b->ptr++;
    endbit=0;
    b->endbyte=endbyte+1;
  }
  b->endbit=endbit;
  return(ret);
}

/* From libogg 1. */
// FUNCTION: EVNOVA 0x00544160
int32_t oggpack_bits(oggpack_buffer *b){
  return(b->endbyte*8+b->endbit);
}
