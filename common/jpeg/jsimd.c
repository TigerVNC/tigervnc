/*
 * jsimd.c
 *
 * Copyright 2009 Pierre Ossman <ossman@cendio.se> for Cendio AB
 * 
 * Based on the x86 SIMD extension for IJG JPEG library,
 * Copyright (C) 1999-2006, MIYASAKA Masaru.
 *
 * This file contains the interface between the "normal" portions
 * of the library and the SIMD implementations.
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jsimd.h"
#include "jdct.h"
#include "jsimddct.h"
#include "simd/jsimd.h"

static unsigned int simd_support = ~0;

/*
 * Check what SIMD accelerations are supported.
 *
 * FIXME: This code is racy under a multi-threaded environment.
 */
LOCAL(void)
init_simd (void)
{
  if (simd_support != ~0)
    return;

#ifdef WITH_SIMD
  simd_support = jpeg_simd_cpu_support();
#else
  simd_support = JSIMD_NONE;
#endif
}

GLOBAL(int)
jsimd_can_rgb_ycc (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;
  if ((RGB_PIXELSIZE != 3) && (RGB_PIXELSIZE != 4))
    return 0;

  if (simd_support & JSIMD_MMX)
    return 1;

  return 0;
}

GLOBAL(int)
jsimd_can_ycc_rgb (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;
  if ((RGB_PIXELSIZE != 3) && (RGB_PIXELSIZE != 4))
    return 0;

  if (simd_support & JSIMD_MMX)
    return 1;

  return 0;
}

GLOBAL(void)
jsimd_rgb_ycc_convert (j_compress_ptr cinfo,
                       JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
                       JDIMENSION output_row, int num_rows)
{
#ifdef WITH_SIMD
  if (simd_support & JSIMD_MMX)
    jsimd_rgb_ycc_convert_mmx(cinfo->image_width, input_buf,
        output_buf, output_row, num_rows);
#endif
}

GLOBAL(void)
jsimd_ycc_rgb_convert (j_decompress_ptr cinfo,
                       JSAMPIMAGE input_buf, JDIMENSION input_row,
                       JSAMPARRAY output_buf, int num_rows)
{
#ifdef WITH_SIMD
  if (simd_support & JSIMD_MMX)
    jsimd_ycc_rgb_convert_mmx(cinfo->output_width, input_buf,
        input_row, output_buf, num_rows);
#endif
}

GLOBAL(int)
jsimd_can_h2v2_downsample (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;

  if (simd_support & JSIMD_MMX)
    return 1;

  return 0;
}

GLOBAL(int)
jsimd_can_h2v1_downsample (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;

  if (simd_support & JSIMD_MMX)
    return 1;

  return 0;
}

GLOBAL(void)
jsimd_h2v2_downsample (j_compress_ptr cinfo, jpeg_component_info * compptr,
                       JSAMPARRAY input_data, JSAMPARRAY output_data)
{
#ifdef WITH_SIMD
  if (simd_support & JSIMD_MMX)
    jsimd_h2v2_downsample_mmx(cinfo->image_width, cinfo->max_v_samp_factor,
        compptr->v_samp_factor, compptr->width_in_blocks,
        input_data, output_data);
#endif
}

GLOBAL(void)
jsimd_h2v1_downsample (j_compress_ptr cinfo, jpeg_component_info * compptr,
                       JSAMPARRAY input_data, JSAMPARRAY output_data)
{
#ifdef WITH_SIMD
  if (simd_support & JSIMD_MMX)
    jsimd_h2v1_downsample_mmx(cinfo->image_width, cinfo->max_v_samp_factor,
        compptr->v_samp_factor, compptr->width_in_blocks,
        input_data, output_data);
#endif
}

GLOBAL(int)
jsimd_can_h2v2_upsample (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;

  if (simd_support & JSIMD_MMX)
    return 1;

  return 0;
}

GLOBAL(int)
jsimd_can_h2v1_upsample (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;

  if (simd_support & JSIMD_MMX)
    return 1;

  return 0;
}

GLOBAL(void)
jsimd_h2v2_upsample (j_decompress_ptr cinfo,
                     jpeg_component_info * compptr, 
                     JSAMPARRAY input_data,
                     JSAMPARRAY * output_data_ptr)
{
#ifdef WITH_SIMD
  if (simd_support & JSIMD_MMX)
    jsimd_h2v2_upsample_mmx(cinfo->max_v_samp_factor,
        cinfo->output_width, input_data, output_data_ptr);
#endif
}

GLOBAL(void)
jsimd_h2v1_upsample (j_decompress_ptr cinfo,
                     jpeg_component_info * compptr, 
                     JSAMPARRAY input_data,
                     JSAMPARRAY * output_data_ptr)
{
#ifdef WITH_SIMD
  if (simd_support & JSIMD_MMX)
    jsimd_h2v1_upsample_mmx(cinfo->max_v_samp_factor,
        cinfo->output_width, input_data, output_data_ptr);
#endif
}

GLOBAL(int)
jsimd_can_h2v2_fancy_upsample (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;

  if (simd_support & JSIMD_MMX)
    return 1;

  return 0;
}

GLOBAL(int)
jsimd_can_h2v1_fancy_upsample (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;

  if (simd_support & JSIMD_MMX)
    return 1;

  return 0;
}

GLOBAL(void)
jsimd_h2v2_fancy_upsample (j_decompress_ptr cinfo,
                           jpeg_component_info * compptr, 
                           JSAMPARRAY input_data,
                           JSAMPARRAY * output_data_ptr)
{
#ifdef WITH_SIMD
  if (simd_support & JSIMD_MMX)
    jsimd_h2v2_fancy_upsample_mmx(cinfo->max_v_samp_factor,
        compptr->downsampled_width, input_data, output_data_ptr);
#endif
}

GLOBAL(void)
jsimd_h2v1_fancy_upsample (j_decompress_ptr cinfo,
                           jpeg_component_info * compptr, 
                           JSAMPARRAY input_data,
                           JSAMPARRAY * output_data_ptr)
{
#ifdef WITH_SIMD
  if (simd_support & JSIMD_MMX)
    jsimd_h2v1_fancy_upsample_mmx(cinfo->max_v_samp_factor,
        compptr->downsampled_width, input_data, output_data_ptr);
#endif
}

GLOBAL(int)
jsimd_can_h2v2_merged_upsample (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;

  if (simd_support & JSIMD_MMX)
    return 1;

  return 0;
}

GLOBAL(int)
jsimd_can_h2v1_merged_upsample (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;

  if (simd_support & JSIMD_MMX)
    return 1;

  return 0;
}

GLOBAL(void)
jsimd_h2v2_merged_upsample (j_decompress_ptr cinfo,
                            JSAMPIMAGE input_buf,
                            JDIMENSION in_row_group_ctr,
                            JSAMPARRAY output_buf)
{
#ifdef WITH_SIMD
  if (simd_support & JSIMD_MMX)
    jsimd_h2v2_merged_upsample_mmx(cinfo->output_width, input_buf,
        in_row_group_ctr, output_buf);
#endif
}

GLOBAL(void)
jsimd_h2v1_merged_upsample (j_decompress_ptr cinfo,
                            JSAMPIMAGE input_buf,
                            JDIMENSION in_row_group_ctr,
                            JSAMPARRAY output_buf)
{
#ifdef WITH_SIMD
  if (simd_support & JSIMD_MMX)
    jsimd_h2v1_merged_upsample_mmx(cinfo->output_width, input_buf,
        in_row_group_ctr, output_buf);
#endif
}

GLOBAL(int)
jsimd_can_convsamp (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (DCTSIZE != 8)
    return 0;
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;
  if (sizeof(DCTELEM) != 2)
    return 0;

  if (simd_support & JSIMD_MMX)
    return 1;

  return 0;
}

GLOBAL(int)
jsimd_can_convsamp_float (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (DCTSIZE != 8)
    return 0;
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;
  if (sizeof(FAST_FLOAT) != 4)
    return 0;

  if (simd_support & JSIMD_3DNOW)
    return 1;

  return 0;
}

GLOBAL(void)
jsimd_convsamp (JSAMPARRAY sample_data, JDIMENSION start_col,
                DCTELEM * workspace)
{
#ifdef WITH_SIMD
  if (simd_support & JSIMD_MMX)
    jsimd_convsamp_mmx(sample_data, start_col, workspace);
#endif
}

GLOBAL(void)
jsimd_convsamp_float (JSAMPARRAY sample_data, JDIMENSION start_col,
                      FAST_FLOAT * workspace)
{
#ifdef WITH_SIMD
  if (simd_support & JSIMD_3DNOW)
    jsimd_convsamp_float_3dnow(sample_data, start_col, workspace);
#endif
}

GLOBAL(int)
jsimd_can_fdct_islow (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (DCTSIZE != 8)
    return 0;
  if (sizeof(DCTELEM) != 2)
    return 0;

  if (simd_support & JSIMD_MMX)
    return 1;

  return 0;
}

GLOBAL(int)
jsimd_can_fdct_ifast (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (DCTSIZE != 8)
    return 0;
  if (sizeof(DCTELEM) != 2)
    return 0;

  if (simd_support & JSIMD_MMX)
    return 1;

  return 0;
}

GLOBAL(int)
jsimd_can_fdct_float (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (DCTSIZE != 8)
    return 0;
  if (sizeof(FAST_FLOAT) != 4)
    return 0;

  if (simd_support & JSIMD_3DNOW)
    return 1;

  return 0;
}

GLOBAL(void)
jsimd_fdct_islow (DCTELEM * data)
{
#ifdef WITH_SIMD
  if (simd_support & JSIMD_MMX)
    jsimd_fdct_islow_mmx(data);
#endif
}

GLOBAL(void)
jsimd_fdct_ifast (DCTELEM * data)
{
#ifdef WITH_SIMD
  if (simd_support & JSIMD_MMX)
    jsimd_fdct_ifast_mmx(data);
#endif
}

GLOBAL(void)
jsimd_fdct_float (FAST_FLOAT * data)
{
#ifdef WITH_SIMD
  if (simd_support & JSIMD_3DNOW)
    jsimd_fdct_float_3dnow(data);
#endif
}

GLOBAL(int)
jsimd_can_quantize (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (DCTSIZE != 8)
    return 0;
  if (sizeof(JCOEF) != 2)
    return 0;
  if (sizeof(DCTELEM) != 2)
    return 0;

  if (simd_support & JSIMD_MMX)
    return 1;

  return 0;
}

GLOBAL(int)
jsimd_can_quantize_float (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (DCTSIZE != 8)
    return 0;
  if (sizeof(JCOEF) != 2)
    return 0;
  if (sizeof(FAST_FLOAT) != 4)
    return 0;

  if (simd_support & JSIMD_3DNOW)
    return 1;

  return 0;
}

GLOBAL(void)
jsimd_quantize (JCOEFPTR coef_block, DCTELEM * divisors,
                DCTELEM * workspace)
{
#ifdef WITH_SIMD
  if (simd_support & JSIMD_MMX)
    jsimd_quantize_mmx(coef_block, divisors, workspace);
#endif
}

GLOBAL(void)
jsimd_quantize_float (JCOEFPTR coef_block, FAST_FLOAT * divisors,
                      FAST_FLOAT * workspace)
{
#ifdef WITH_SIMD
  if (simd_support & JSIMD_3DNOW)
    jsimd_quantize_float_3dnow(coef_block, divisors, workspace);
#endif
}

GLOBAL(int)
jsimd_can_idct_2x2 (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (DCTSIZE != 8)
    return 0;
  if (sizeof(JCOEF) != 2)
    return 0;
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;
  if (sizeof(ISLOW_MULT_TYPE) != 2)
    return 0;

  if (simd_support & JSIMD_MMX)
    return 1;

  return 0;
}

GLOBAL(int)
jsimd_can_idct_4x4 (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (DCTSIZE != 8)
    return 0;
  if (sizeof(JCOEF) != 2)
    return 0;
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;
  if (sizeof(ISLOW_MULT_TYPE) != 2)
    return 0;

  if (simd_support & JSIMD_MMX)
    return 1;

  return 0;
}

GLOBAL(void)
jsimd_idct_2x2 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
                JCOEFPTR coef_block, JSAMPARRAY output_buf,
                JDIMENSION output_col)
{
#if WITH_SIMD
  if (simd_support & JSIMD_MMX)
    jsimd_idct_2x2_mmx(compptr->dct_table, coef_block, output_buf, output_col);
#endif
}

GLOBAL(void)
jsimd_idct_4x4 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
                JCOEFPTR coef_block, JSAMPARRAY output_buf,
                JDIMENSION output_col)
{
#if WITH_SIMD
  if (simd_support & JSIMD_MMX)
    jsimd_idct_4x4_mmx(compptr->dct_table, coef_block, output_buf, output_col);
#endif
}

GLOBAL(int)
jsimd_can_idct_islow (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (DCTSIZE != 8)
    return 0;
  if (sizeof(JCOEF) != 2)
    return 0;
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;
  if (sizeof(ISLOW_MULT_TYPE) != 2)
    return 0;

  if (simd_support & JSIMD_MMX)
    return 1;

  return 0;
}

GLOBAL(int)
jsimd_can_idct_ifast (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (DCTSIZE != 8)
    return 0;
  if (sizeof(JCOEF) != 2)
    return 0;
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;
  if (sizeof(IFAST_MULT_TYPE) != 2)
    return 0;
  if (IFAST_SCALE_BITS != 2)
    return 0;

  if (simd_support & JSIMD_MMX)
    return 1;

  return 0;
}

GLOBAL(int)
jsimd_can_idct_float (void)
{
  init_simd();

  if (DCTSIZE != 8)
    return 0;
  if (sizeof(JCOEF) != 2)
    return 0;
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;
  if (sizeof(FAST_FLOAT) != 4)
    return 0;
  if (sizeof(FLOAT_MULT_TYPE) != 4)
    return 0;

  if (simd_support & JSIMD_3DNOW)
    return 1;

  return 0;
}

GLOBAL(void)
jsimd_idct_islow (j_decompress_ptr cinfo, jpeg_component_info * compptr,
                JCOEFPTR coef_block, JSAMPARRAY output_buf,
                JDIMENSION output_col)
{
#if WITH_SIMD
  if (simd_support & JSIMD_MMX)
    jsimd_idct_islow_mmx(compptr->dct_table, coef_block, output_buf, output_col);
#endif
}

GLOBAL(void)
jsimd_idct_ifast (j_decompress_ptr cinfo, jpeg_component_info * compptr,
                JCOEFPTR coef_block, JSAMPARRAY output_buf,
                JDIMENSION output_col)
{
#if WITH_SIMD
  if (simd_support & JSIMD_MMX)
    jsimd_idct_ifast_mmx(compptr->dct_table, coef_block, output_buf, output_col);
#endif
}

GLOBAL(void)
jsimd_idct_float (j_decompress_ptr cinfo, jpeg_component_info * compptr,
                JCOEFPTR coef_block, JSAMPARRAY output_buf,
                JDIMENSION output_col)
{
#if WITH_SIMD
  if (simd_support & JSIMD_3DNOW)
    jsimd_idct_float_3dnow(compptr->dct_table, coef_block,
        output_buf, output_col);
#endif
}

