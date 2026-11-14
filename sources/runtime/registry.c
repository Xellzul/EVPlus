// COMPILE: -lang c++ -bool off
/* registry.c -- libvorbis 1.0rc3 lib/registry.c, (C) COPYRIGHT 1994-2001 by the XIPHOPHORUS Company; licence in codec.h. Stock in BurgerLib's copy (the four arrays at .data 0x005832d4); altered: comments and residue_free_info (not linked) removed. Notes: Research/notes/burgervorbis.md */

#include <stdio.h>
#include "codec.h"
#include "codec_internal.h"
#include "registry.h"
#include "registry-api.h"
#include "misc.h"

extern vorbis_func_time      time0_exportbundle;
extern vorbis_func_floor     floor0_exportbundle;
extern vorbis_func_floor     floor1_exportbundle;
extern vorbis_func_residue   residue0_exportbundle;
extern vorbis_func_residue   residue1_exportbundle;
extern vorbis_func_residue   residue2_exportbundle;
extern vorbis_func_mapping   mapping0_exportbundle;

vorbis_func_time      *_time_P[]={
  &time0_exportbundle,
};

vorbis_func_floor     *_floor_P[]={
  &floor0_exportbundle,
  &floor1_exportbundle,
};

vorbis_func_residue   *_residue_P[]={
  &residue0_exportbundle,
  &residue1_exportbundle,
  &residue2_exportbundle,
};

vorbis_func_mapping   *_mapping_P[]={
  &mapping0_exportbundle,
};
