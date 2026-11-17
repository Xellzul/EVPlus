// COMPILE: -lang c++ -bool off
/* window.c -- libvorbis 1.0rc3 lib/window.c, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. BurgerLib's copy (allocates only for type 0, checks the allocation, and computes the slopes with a running phase and one walking pointer); altered: comments removed. Notes: Research/notes/burgervorbis.md */

#include <stdlib.h>
#include <math.h>
#include "os.h"
#include "misc.h"
#include "window.h"

// FUNCTION: EVNOVA 0x00547500
float *_vorbis_window(int type, int window,int left,int right){
  float *ret;
  int leftbegin,rightbegin,i;
  float x,step,fi,fr;
  float *p;

  if(type==0){
    ret=(float *)_ogg_calloc(window,sizeof(*ret));
    if(ret){
      leftbegin=window/4-left/2;
      rightbegin=window-window/4-right/2;
      p=ret+leftbegin;

      step=1.f/left*(M_PI/2.f);
      fi=.5f;
      for(i=0;i<left;i++){
	x=fi*step;
	x=sin(x);
	x*=x;
	x*=M_PI/2.f;
	x=sin(x);
	*p++=x;
	fi+=1.f;
      }

      for(i=leftbegin+left;i<rightbegin;i++)
	*p++=1.f;

      fr=right;
      fi=fr-.5f;
      step=1.f/fr*(M_PI/2.f);
      for(i=0;i<right;i++){
	x=fi*step;
	x=sin(x);
	x*=x;
	x*=M_PI/2.f;
	x=sin(x);
	*p++=x;
	fi-=1.f;
      }
      return(ret);
    }
  }
  return(NULL);
}
