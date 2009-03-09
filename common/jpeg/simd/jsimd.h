/*
 * simd/jsimd.h
 *
 * Copyright 2009 Pierre Ossman <ossman@cendio.se> for Cendio AB
 * 
 * Based on the x86 SIMD extension for IJG JPEG library,
 * Copyright (C) 1999-2006, MIYASAKA Masaru.
 *
 */

/* Bitmask for supported acceleration methods */

#define JSIMD_NONE    0x00
#define JSIMD_MMX     0x01
#define JSIMD_3DNOW   0x02

/* Short forms of external names for systems with brain-damaged linkers. */

#ifdef NEED_SHORT_EXTERNAL_NAMES
#define jpeg_simd_cpu_support                 jSiCpuSupport
#define jsimd_rgb_ycc_convert_mmx             jSRGBYCCM
#define jsimd_ycc_rgb_convert_mmx             jSYCCRGBM
#define jsimd_h2v2_downsample_mmx             jSDnH2V2M
#define jsimd_h2v1_downsample_mmx             jSDnH2V1M
#define jsimd_h2v2_upsample_mmx               jSUpH2V2M
#define jsimd_h2v1_upsample_mmx               jSUpH2V1M
#define jsimd_h2v2_fancy_upsample_mmx         jSFUpH2V2M
#define jsimd_h2v1_fancy_upsample_mmx         jSFUpH2V1M
#define jsimd_h2v2_merged_upsample_mmx        jSMUpH2V2M
#define jsimd_h2v1_merged_upsample_mmx        jSMUpH2V1M
#define jsimd_convsamp_mmx                    jSConvM
#define jsimd_convsamp_float_3dnow            jSConvF3D
#define jsimd_fdct_islow_mmx                  jSFDMIS
#define jsimd_fdct_ifast_mmx                  jSFDMIF
#define jsimd_fdct_float_3dnow                jSFD3DF
#define jsimd_quantize_mmx                    jSQuantM
#define jsimd_quantize_float_3dnow            jSQuantF3D
#define jsimd_idct_2x2_mmx                    jSIDM22
#define jsimd_idct_4x4_mmx                    jSIDM44
#define jsimd_idct_islow_mmx                  jSIDMIS
#define jsimd_idct_ifast_mmx                  jSIDMIF
#define jsimd_idct_float_3dnow                jSID3DF
#endif /* NEED_SHORT_EXTERNAL_NAMES */

/* SIMD Ext: retrieve SIMD/CPU information */
EXTERN(unsigned int) jpeg_simd_cpu_support JPP((void));

/* SIMD Color Space Conversion */
EXTERN(void) jsimd_rgb_ycc_convert_mmx
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_ycc_rgb_convert_mmx
        JPP((JDIMENSION out_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));

/* SIMD Downsample */
EXTERN(void) jsimd_h2v2_downsample_mmx
        JPP((JDIMENSION image_width, int max_v_samp_factor,
             JDIMENSION v_samp_factor, JDIMENSION width_blocks,
             JSAMPARRAY input_data, JSAMPARRAY output_data));
EXTERN(void) jsimd_h2v1_downsample_mmx
        JPP((JDIMENSION image_width, int max_v_samp_factor,
             JDIMENSION v_samp_factor, JDIMENSION width_blocks,
             JSAMPARRAY input_data, JSAMPARRAY output_data));

/* SIMD Upsample */
EXTERN(void) jsimd_h2v2_upsample_mmx
        JPP((int max_v_samp_factor, JDIMENSION output_width,
             JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));
EXTERN(void) jsimd_h2v1_upsample_mmx
        JPP((int max_v_samp_factor, JDIMENSION output_width,
             JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));

EXTERN(void) jsimd_h2v2_fancy_upsample_mmx
        JPP((int max_v_samp_factor, JDIMENSION downsampled_width,
             JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));
EXTERN(void) jsimd_h2v1_fancy_upsample_mmx
        JPP((int max_v_samp_factor, JDIMENSION downsampled_width,
             JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));

EXTERN(void) jsimd_h2v2_merged_upsample_mmx
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v1_merged_upsample_mmx
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));

/* SIMD Sample Conversion */
EXTERN(void) jsimd_convsamp_mmx JPP((JSAMPARRAY sample_data,
                                     JDIMENSION start_col,
                                     DCTELEM * workspace));

EXTERN(void) jsimd_convsamp_float_3dnow JPP((JSAMPARRAY sample_data,
                                             JDIMENSION start_col,
                                             FAST_FLOAT * workspace));

/* SIMD Forward DCT */
EXTERN(void) jsimd_fdct_islow_mmx JPP((DCTELEM * data));
EXTERN(void) jsimd_fdct_ifast_mmx JPP((DCTELEM * data));

EXTERN(void) jsimd_fdct_float_3dnow JPP((FAST_FLOAT * data));

/* SIMD Quantization */
EXTERN(void) jsimd_quantize_mmx JPP((JCOEFPTR coef_block,
                                     DCTELEM * divisors,
                                     DCTELEM * workspace));

EXTERN(void) jsimd_quantize_float_3dnow JPP((JCOEFPTR coef_block,
                                             FAST_FLOAT * divisors,
                                             FAST_FLOAT * workspace));

/* SIMD Reduced Inverse DCT */
EXTERN(void) jsimd_idct_2x2_mmx JPP((void * dct_table,
                                     JCOEFPTR coef_block,
                                     JSAMPARRAY output_buf,
                                     JDIMENSION output_col));
EXTERN(void) jsimd_idct_4x4_mmx JPP((void * dct_table,
                                     JCOEFPTR coef_block,
                                     JSAMPARRAY output_buf,
                                     JDIMENSION output_col));

/* SIMD Inverse DCT */
EXTERN(void) jsimd_idct_islow_mmx JPP((void * dct_table,
                                       JCOEFPTR coef_block,
                                       JSAMPARRAY output_buf,
                                       JDIMENSION output_col));
EXTERN(void) jsimd_idct_ifast_mmx JPP((void * dct_table,
                                       JCOEFPTR coef_block,
                                       JSAMPARRAY output_buf,
                                       JDIMENSION output_col));

EXTERN(void) jsimd_idct_float_3dnow JPP((void * dct_table,
                                         JCOEFPTR coef_block,
                                         JSAMPARRAY output_buf,
                                         JDIMENSION output_col));

