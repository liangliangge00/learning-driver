/* ov5648_8909_lib.c
 *
 * Copyright (c) 2015 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#include <stdio.h>
#include "sensor_lib.h"
#include "ov5648_8909_lib.h"

#define SENSOR_MODEL_NO_OV5648_8909 "ov5648_8909"
#define OV5648_8909_LOAD_CHROMATIX(n) \
  "libchromatix_"SENSOR_MODEL_NO_OV5648_8909"_"#n".so"

#undef DEBUG_INFO
#define OV5648_8909_DEBUG
#ifdef OV5648_8909_DEBUG
#include <utils/Log.h>
#define SERR(fmt, args...) \
  ALOGE("%s:%d "fmt"\n", __func__, __LINE__, ##args)

#define DEBUG_INFO(fmt, args...) SERR(fmt, ##args)
#else
#define DEBUG_INFO(fmt, args...) do { } while (0)
#endif

static sensor_lib_t sensor_lib_ptr;

static struct msm_sensor_power_setting power_setting[] = {
  {
    .seq_type = SENSOR_VREG,
    .seq_val = CAM_VIO,
    .config_val = 0,
    .delay = 0,
  },
  {
    .seq_type = SENSOR_VREG,
    .seq_val = CAM_VANA,
    .config_val = 0,
    .delay = 0,
  },

  /*
  {
    .seq_type = SENSOR_VREG,
    .seq_val = CAM_VAF,
    .config_val = 0,
    .delay = 0,
  },
 
  {
    .seq_type = SENSOR_GPIO,
    .seq_val = SENSOR_GPIO_AF_PWDM,
    .config_val = GPIO_OUT_LOW,
    .delay = 1,
  },

  {
    .seq_type = SENSOR_GPIO,
    .seq_val = SENSOR_GPIO_AF_PWDM,
    .config_val = GPIO_OUT_HIGH,
    .delay = 5,
  },
  */
  //luffy add
   
  {
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VANA,
		.config_val = GPIO_OUT_HIGH,
		.delay = 10,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VDIG,
		.config_val = GPIO_OUT_HIGH,
		.delay = 10,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VIO,
		.config_val = GPIO_OUT_HIGH,
		.delay = 10,
	},
  {
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VDIG,
		.config_val = 0,
		.delay = 1,
	},
  {
    .seq_type = SENSOR_GPIO,
    .seq_val = SENSOR_GPIO_RESET,
    .config_val = GPIO_OUT_LOW,
    .delay = 0,
  },
  {
    .seq_type = SENSOR_GPIO,
    .seq_val = SENSOR_GPIO_RESET,
    .config_val = GPIO_OUT_HIGH,
    .delay = 10,
  },
  {
    .seq_type = SENSOR_GPIO,
    .seq_val = SENSOR_GPIO_STANDBY,
    .config_val = GPIO_OUT_LOW,
    .delay = 0,
  },
  {
    .seq_type = SENSOR_GPIO,
    .seq_val = SENSOR_GPIO_STANDBY,
    .config_val = GPIO_OUT_HIGH,
    .delay = 5,
  },
  {
    .seq_type = SENSOR_CLK,
    .seq_val = SENSOR_CAM_MCLK,
    .config_val = 24000000,
    .delay = 10,
  },
  {
    .seq_type = SENSOR_I2C_MUX,
    .seq_val = 0,
    .config_val = 0,
    .delay = 0,
  },
};

static struct msm_sensor_power_setting power_down_setting[] = {
  {
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VANA,
		.config_val = GPIO_OUT_LOW,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VDIG,
		.config_val = GPIO_OUT_LOW,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VIO,
		.config_val = GPIO_OUT_LOW,
		.delay = 10,
	},
  {
    .seq_type = SENSOR_GPIO,
    .seq_val = SENSOR_GPIO_STANDBY,
    .config_val = GPIO_OUT_LOW,
    .delay = 0,
  },
  {
    .seq_type = SENSOR_CLK,
    .seq_val = SENSOR_CAM_MCLK,
    .config_val = 0,
    .delay = 10,
  },
};

static struct msm_camera_sensor_slave_info sensor_slave_info = {
  /* Camera slot where this camera is mounted */
  .camera_id = CAMERA_0,
  /* sensor slave address */
  .slave_addr = 0x6c,
  /* sensor i2c frequency*/
  .i2c_freq_mode = I2C_FAST_MODE,
  /* sensor address type */
  .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
  /* sensor id info*/
  .sensor_id_info = {
    /* sensor id register address */
    .sensor_id_reg_addr = 0x300a,
    /* sensor id */
    .sensor_id = 0x5648,
  },
  /* power up / down setting */
  .power_setting_array = {
    .power_setting = power_setting,
    .size = ARRAY_SIZE(power_setting),
    
    .power_down_setting = power_down_setting,
    .size_down = ARRAY_SIZE(power_down_setting),
  },
};

static struct msm_sensor_init_params sensor_init_params = {
  .modes_supported = CAMERA_MODE_2D_B,
  /* sensor position (back or front) */
  .position = BACK_CAMERA_B,
  /* sensor mounting direction */
  .sensor_mount_angle = SENSOR_MOUNTANGLE_270,
};

static sensor_output_t sensor_output = {
  /* sensor image format */
  .output_format = SENSOR_BAYER,
  /* sensor data output interface */
  .connection_mode = SENSOR_MIPI_CSI,
  /* sensor data */
  .raw_output = SENSOR_10_BIT_DIRECT,
};

static struct msm_sensor_output_reg_addr_t output_reg_addr = {
  .x_output = 0x3808,
  .y_output = 0x380a,
  .line_length_pclk = 0x380c,
  .frame_length_lines = 0x380e,
};

static struct msm_sensor_exp_gain_info_t exp_gain_info = {
  .coarse_int_time_addr = 0x3500,
  .global_gain_addr = 0x350a,
  .vert_offset = 4,
};

static sensor_aec_data_t aec_info = {
  .max_gain = 12.0,
  .max_linecount = 26880,
};

static sensor_lens_info_t default_lens_info = {
  .focal_length = 3.16,
  .pix_size = 1.4,
  .f_number = 2.4,
  .total_f_dist = 1.2,
  .hor_view_angle = 56.0,
  .ver_view_angle = 42.0,
};

/* mipi_csi interface lane config */
#ifndef VFE_40
static struct csi_lane_params_t csi_lane_params = {
  .csi_lane_assign = 0x4320,
  .csi_lane_mask = 0x7,
  .csi_if = 1,
  .csid_core = {0},
  .csi_phy_sel = 0,
};
#else
static struct csi_lane_params_t csi_lane_params = {
  .csi_lane_assign = 0x4320,
  .csi_lane_mask = 0x7,
  .csi_if = 1,
  .csid_core = {0},
  .csi_phy_sel = 0,
};
#endif

static struct msm_camera_i2c_reg_setting init_reg_setting[] = {
  {
    .reg_setting = init_reg_array,
    .size = ARRAY_SIZE(init_reg_array),
    .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
    .data_type = MSM_CAMERA_I2C_BYTE_DATA,
    .delay = 0,
  },
};

static struct sensor_lib_reg_settings_array init_settings_array = {
  .reg_settings = init_reg_setting,
  .size = 1,
};

static struct msm_camera_i2c_reg_array start_reg_array[] = {
  {0x0100, 0x01},
  {0x4800, 0x04},
  {0x4202, 0x00},  /* streaming on */
};


static  struct msm_camera_i2c_reg_setting start_settings = {
  .reg_setting = start_reg_array,
  .size = ARRAY_SIZE(start_reg_array),
  .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 10,
};

static struct msm_camera_i2c_reg_array stop_reg_array[] = {
  {0x0100, 0x00},
  {0x4800, 0x25},
  {0x4202, 0x0f},  /* streaming off*/
};

static struct msm_camera_i2c_reg_setting stop_settings = {
  .reg_setting = stop_reg_array,
  .size = ARRAY_SIZE(stop_reg_array),
  .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 10,
};

static struct msm_camera_i2c_reg_array groupon_reg_array[] = {
  {0x3208, 0x00},
};

static struct msm_camera_i2c_reg_setting groupon_settings = {
  .reg_setting = groupon_reg_array,
  .size = ARRAY_SIZE(groupon_reg_array),
  .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 0,
};

static struct msm_camera_i2c_reg_array groupoff_reg_array[] = {
  {0x3208, 0x10},
  {0x3208, 0xA0},
};

static struct msm_camera_i2c_reg_setting groupoff_settings = {
  .reg_setting = groupoff_reg_array,
  .size = ARRAY_SIZE(groupoff_reg_array),
  .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
  .data_type = MSM_CAMERA_I2C_BYTE_DATA,
  .delay = 0,
};

static struct msm_camera_csid_vc_cfg ov5648_8909_cid_cfg[] = {
  {0, CSI_RAW10, CSI_DECODE_10BIT},
  {1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params ov5648_8909_csi_params = {
  .csid_params = {
    /* mipi data lane number */
    .lane_cnt = 2,
    .lut_params = {
      .num_cid = ARRAY_SIZE(ov5648_8909_cid_cfg),
      .vc_cfg = {
         &ov5648_8909_cid_cfg[0],
         &ov5648_8909_cid_cfg[1],
      },
    },
  },

  /* mipi csi phy config */
  .csiphy_params = {
    .lane_cnt = 2,
    .settle_cnt = 0x1B,
#ifndef VFE_40
    .combo_mode = 1,
#endif
  },
};

static struct sensor_pix_fmt_info_t ov5648_8909_pix_fmt0_fourcc[] = {
  { V4L2_PIX_FMT_SBGGR10 },
};

static struct sensor_pix_fmt_info_t ov5648_8909_pix_fmt1_fourcc[] = {
  { MSM_V4L2_PIX_FMT_META },
};

static sensor_stream_info_t ov5648_8909_stream_info[] = {
  {1, &ov5648_8909_cid_cfg[0], ov5648_8909_pix_fmt0_fourcc},
  {1, &ov5648_8909_cid_cfg[1], ov5648_8909_pix_fmt1_fourcc},
};

static sensor_stream_info_array_t ov5648_8909_stream_info_array = {
  .sensor_stream_info = ov5648_8909_stream_info,
  .size = ARRAY_SIZE(ov5648_8909_stream_info),
};

static struct msm_camera_i2c_reg_setting res_settings[] = {
  {
    .reg_setting = res0_reg_array,
    .size = ARRAY_SIZE(res0_reg_array),
    .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
    .data_type = MSM_CAMERA_I2C_BYTE_DATA,
    .delay = 0,
  },

#if 0
  {
    .reg_setting = res1_reg_array,
    .size = ARRAY_SIZE(res1_reg_array),
    .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
    .data_type = MSM_CAMERA_I2C_BYTE_DATA,
    .delay = 0,
  },

  {
    .reg_setting = res2_reg_array,
    .size = ARRAY_SIZE(res2_reg_array),
    .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
    .data_type = MSM_CAMERA_I2C_BYTE_DATA,
    .delay = 0,
  },
 
  {
    .reg_setting = res3_reg_array,
    .size = ARRAY_SIZE(res3_reg_array),
    .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
    .data_type = MSM_CAMERA_I2C_BYTE_DATA,
    .delay = 0,
  },
   
  {
    .reg_setting = res4_reg_array,
    .size = ARRAY_SIZE(res4_reg_array),
    .addr_type = MSM_CAMERA_I2C_WORD_ADDR,
    .data_type = MSM_CAMERA_I2C_BYTE_DATA,
    .delay = 0,
  },
#endif
};

static struct sensor_lib_reg_settings_array res_settings_array = {
  .reg_settings = res_settings,
  .size = ARRAY_SIZE(res_settings),
};

static struct msm_camera_csi2_params *csi_params[] = {
  &ov5648_8909_csi_params, /* RES 0*/
  //&ov5648_8909_csi_params, /* RES 1*/
  //&ov5648_8909_csi_params, /* RES 2*/
  //&ov5648_8909_csi_params, /* RES 3*/
  //&ov5648_8909_csi_params, /* RES 4*/
};

static struct sensor_lib_csi_params_array csi_params_array = {
  .csi2_params = &csi_params[0],
  .size = ARRAY_SIZE(csi_params),
};

static struct sensor_crop_parms_t crop_params[] = {
  { 0, 0, 0, 0 }, /* RES 0 */
  //{ 0, 0, 0, 0 }, /* RES 1 */
  //{ 0, 0, 0, 0 }, /* RES 2 */
  //{ 0, 0, 0, 0 }, /* RES 3 */
  //{ 0, 0, 0, 0 }, /* RES 4 */
};

static struct sensor_lib_crop_params_array crop_params_array = {
  .crop_params = crop_params,
  .size = ARRAY_SIZE(crop_params),
};

static struct sensor_lib_out_info_t sensor_out_info[] = {
  {
    /* full size @ 15 fps */
    .x_output = 0xa20, /* 2592 */
    .y_output = 0x798, /* 1944 */
    .line_length_pclk = 0xb00, /* 2816 */
    .frame_length_lines = 0x7c0, /* 1984 */
    .vt_pixel_clk = 83800000,
    .op_pixel_clk = 84000000,
    .binning_factor = 1,
    .max_fps = 15.0,
    .min_fps = 3.75,
    .mode = SENSOR_DEFAULT_MODE,
  },

#if 0
  {
    /* 1/4 size @ 30 fps */
    .x_output = 0x510, /* 1296 */
    .y_output = 0x3cc, /* 972 */
    .line_length_pclk = 0xb00, /* 2816 */
    .frame_length_lines = 0x3e0, /* 992 */
    .vt_pixel_clk = 83800000,
    .op_pixel_clk = 84000000,
    .binning_factor = 1,
    .max_fps = 30.0,
    .min_fps = 7.5,
    .mode = SENSOR_DEFAULT_MODE,
  },
 
  {
    /* 1/4 size @ 60 fps */
    .x_output = 0x510, /* 1296 */
    .y_output = 0x3cc, /* 972 */
    .line_length_pclk = 0x580, /* 1408 */
    .frame_length_lines = 0x3e0, /* 992 */
    .vt_pixel_clk = 83800000,
    .op_pixel_clk = 84000000,
    .binning_factor = 1,
    .max_fps = 60.00,
    .min_fps = 7.5,
    .mode = SENSOR_DEFAULT_MODE,
  },
  
  {
    /* VGA @ 90 fps */
    .x_output = 0x280, /* 640 */
    .y_output = 0x1E0, /* 480 */
    .line_length_pclk = 0x728, /* 1832 */
    .frame_length_lines = 0x1fc, /* 508 */
    .vt_pixel_clk = 83800000,
    .op_pixel_clk = 84000000,
    .binning_factor = 1,
    .max_fps = 90.00,
    .min_fps = 7.5,
    .mode = SENSOR_DEFAULT_MODE,
  },
  
  {
    /* QVGA @ 120 fps */
    .x_output = 0x140, /* 320 */
    .y_output = 0xF0, /* 240 */
    .line_length_pclk = 0xa38, /* 2616 */
    .frame_length_lines = 0x10c, /* 268 */
    .vt_pixel_clk = 83800000,
    .op_pixel_clk = 84000000,
    .binning_factor = 1,
    .max_fps = 120.00,
    .min_fps = 7.5,
    .mode = SENSOR_DEFAULT_MODE,
  },
#endif
};

static struct sensor_lib_out_info_array out_info_array = {
  .out_info = sensor_out_info,
  .size = ARRAY_SIZE(sensor_out_info),
};

static sensor_res_cfg_type_t ov5648_8909_res_cfg[] = {
  SENSOR_SET_STOP_STREAM,
  SENSOR_SET_NEW_RESOLUTION, /* set stream config */
  SENSOR_SET_CSIPHY_CFG,
  SENSOR_SET_CSID_CFG,
  SENSOR_LOAD_CHROMATIX, /* set chromatix prt */
  SENSOR_SEND_EVENT, /* send event */
  SENSOR_SET_START_STREAM,
};

static struct sensor_res_cfg_table_t ov5648_8909_res_table = {
  .res_cfg_type = ov5648_8909_res_cfg,
  .size = ARRAY_SIZE(ov5648_8909_res_cfg),
};

static struct sensor_lib_chromatix_t ov5648_8909_chromatix[] = {
  {
    .common_chromatix = OV5648_8909_LOAD_CHROMATIX(common),
    .camera_preview_chromatix = OV5648_8909_LOAD_CHROMATIX(zsl), /* RES0 */
    .camera_snapshot_chromatix = OV5648_8909_LOAD_CHROMATIX(snapshot), /* RES0 */
    .camcorder_chromatix = OV5648_8909_LOAD_CHROMATIX(default_video), /* RES0 */
    .liveshot_chromatix = OV5648_8909_LOAD_CHROMATIX(liveshot), /* RES0 */
  },

#if 0
  {
    .common_chromatix = OV5648_8909_LOAD_CHROMATIX(common),
    .camera_preview_chromatix = OV5648_8909_LOAD_CHROMATIX(preview), /* RES1 */
    .camera_snapshot_chromatix = OV5648_8909_LOAD_CHROMATIX(preview), /* RES1 */
    .camcorder_chromatix = OV5648_8909_LOAD_CHROMATIX(default_video), /* RES1 */
    .liveshot_chromatix = OV5648_8909_LOAD_CHROMATIX(liveshot), /* RES1 */
  },

  {
    .common_chromatix = OV5648_8909_LOAD_CHROMATIX(common),
    .camera_preview_chromatix = OV5648_8909_LOAD_CHROMATIX(hfr_60fps), /* RES2 */
    .camera_snapshot_chromatix = OV5648_8909_LOAD_CHROMATIX(hfr_60fps), /* RES2 */
    .camcorder_chromatix = OV5648_8909_LOAD_CHROMATIX(hfr_60fps), /* RES2 */
    .liveshot_chromatix = OV5648_8909_LOAD_CHROMATIX(liveshot), /* RES2 */
  },
 
  {
    .common_chromatix = OV5648_8909_LOAD_CHROMATIX(common),
    .camera_preview_chromatix = OV5648_8909_LOAD_CHROMATIX(hfr_90fps), /* RES3 */
    .camera_snapshot_chromatix = OV5648_8909_LOAD_CHROMATIX(hfr_90fps), /* RES3 */
    .camcorder_chromatix = OV5648_8909_LOAD_CHROMATIX(hfr_90fps), /* RES3 */
    .liveshot_chromatix = OV5648_8909_LOAD_CHROMATIX(liveshot), /* RES3 */
  },
 
  {
    .common_chromatix = OV5648_8909_LOAD_CHROMATIX(common),
    .camera_preview_chromatix = OV5648_8909_LOAD_CHROMATIX(hfr_120fps), /* RES4 */
    .camera_snapshot_chromatix = OV5648_8909_LOAD_CHROMATIX(hfr_120fps), /* RES4 */
    .camcorder_chromatix = OV5648_8909_LOAD_CHROMATIX(hfr_120fps), /* RES4 */
    .liveshot_chromatix = OV5648_8909_LOAD_CHROMATIX(liveshot), /* RES4 */
  },
#endif
};

static struct sensor_lib_chromatix_array ov5648_8909_lib_chromatix_array = {
  .sensor_lib_chromatix = ov5648_8909_chromatix,
  .size = ARRAY_SIZE(ov5648_8909_chromatix),
};

/*===========================================================================
 * FUNCTION    - ov5648_8909_real_to_register_gain -
 *
 * DESCRIPTION:
 *==========================================================================*/
static uint16_t ov5648_8909_real_to_register_gain(float gain)
{
  uint16_t reg_gain, reg_temp;
  if(gain > 10.0)
    gain = 10.0;
  if(gain < 1.0)
    gain = 1.0;
  reg_gain = (uint16_t)gain;
  reg_temp = reg_gain<<4;
  reg_gain = reg_temp | (((uint16_t)((gain - (float)reg_gain)*16.0))&0x0f);
  return reg_gain;

}

/*===========================================================================
 * FUNCTION    - ov5648_8909_register_to_real_gain -
 *
 * DESCRIPTION:
 *==========================================================================*/
static float ov5648_8909_register_to_real_gain(uint16_t reg_gain)
{
  float real_gain;
  real_gain = (float) ((float)(reg_gain>>4)+(((float)(reg_gain&0x0f))/16.0));
  return real_gain;
}

/*===========================================================================
 * FUNCTION    - ov5648_8909_calculate_exposure -
 *
 * DESCRIPTION:
 *==========================================================================*/
static int32_t ov5648_8909_calculate_exposure(float real_gain,
  uint16_t line_count, sensor_exposure_info_t *exp_info)
{
  if (!exp_info) {
    return -1;
  }
  exp_info->reg_gain = ov5648_8909_real_to_register_gain(real_gain);
  float sensor_real_gain = ov5648_8909_register_to_real_gain(exp_info->reg_gain);
  exp_info->digital_gain = real_gain / sensor_real_gain;
  exp_info->line_count = line_count;
  return 0;
}

/*===========================================================================
 * FUNCTION    - ov5648_8909_fill_exposure_array -
 *
 * DESCRIPTION:
 *==========================================================================*/
static int32_t ov5648_8909_fill_exposure_array(uint16_t gain, uint32_t line,
  uint32_t fl_lines, int32_t luma_avg, uint32_t fgain,
  struct msm_camera_i2c_reg_setting* reg_setting)
{
  uint16_t reg_count = 0;
  uint16_t i = 0;

  if (!reg_setting) {
    return -1;
  }

  for (i = 0; i < sensor_lib_ptr.groupon_settings->size; i++) {
    reg_setting->reg_setting[reg_count].reg_addr =
      sensor_lib_ptr.groupon_settings->reg_setting[i].reg_addr;
    reg_setting->reg_setting[reg_count].reg_data =
      sensor_lib_ptr.groupon_settings->reg_setting[i].reg_data;
    reg_count = reg_count + 1;
  }

  reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.output_reg_addr->frame_length_lines;
  reg_setting->reg_setting[reg_count].reg_data = (fl_lines & 0xFF00) >> 8;
  reg_count = reg_count + 1;

  reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.output_reg_addr->frame_length_lines + 1;
  reg_setting->reg_setting[reg_count].reg_data = (fl_lines & 0xFF);
  reg_count = reg_count + 1;

  reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.exp_gain_info->coarse_int_time_addr;
  reg_setting->reg_setting[reg_count].reg_data = line  >> 12;
  reg_count = reg_count + 1;

  reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.exp_gain_info->coarse_int_time_addr + 1;
  reg_setting->reg_setting[reg_count].reg_data = line >> 4;
  reg_count = reg_count + 1;

    reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.exp_gain_info->coarse_int_time_addr + 2;
  reg_setting->reg_setting[reg_count].reg_data = line << 4;
  reg_count = reg_count + 1;

  reg_setting->reg_setting[reg_count].reg_addr =
      sensor_lib_ptr.exp_gain_info->global_gain_addr;
  reg_setting->reg_setting[reg_count].reg_data = (gain & 0xFF00) >> 8;
  reg_count = reg_count + 1;

  reg_setting->reg_setting[reg_count].reg_addr =
    sensor_lib_ptr.exp_gain_info->global_gain_addr + 1;
  reg_setting->reg_setting[reg_count].reg_data = (gain & 0xFF);
  reg_count = reg_count + 1;

  for (i = 0; i < sensor_lib_ptr.groupoff_settings->size; i++) {
    reg_setting->reg_setting[reg_count].reg_addr =
      sensor_lib_ptr.groupoff_settings->reg_setting[i].reg_addr;
    reg_setting->reg_setting[reg_count].reg_data =
      sensor_lib_ptr.groupoff_settings->reg_setting[i].reg_data;
    reg_count = reg_count + 1;
  }

  reg_setting->size = reg_count;
  reg_setting->addr_type = MSM_CAMERA_I2C_WORD_ADDR;
  reg_setting->data_type = MSM_CAMERA_I2C_BYTE_DATA;
  reg_setting->delay = 0;
  return 0;
}

static sensor_exposure_table_t ov5648_8909_expsoure_tbl = {
  .sensor_calculate_exposure = ov5648_8909_calculate_exposure,
  .sensor_fill_exposure_array = ov5648_8909_fill_exposure_array,
};

static sensor_lib_t sensor_lib_ptr = {
  /* sensor slave info */
  .sensor_slave_info = &sensor_slave_info,
  /* sensor init params */
  .sensor_init_params = &sensor_init_params,
  /* sensor actuator name */
  .actuator_name = "a3907",
  /* module eeprom name */
  .eeprom_name = "sunny_q5v22e",
  /* sensor output settings */
  .sensor_output = &sensor_output,
  /* sensor output register address */
  .output_reg_addr = &output_reg_addr,
  /* sensor exposure gain register address */
  .exp_gain_info = &exp_gain_info,
  /* sensor aec info */
  .aec_info = &aec_info,
  /* sensor snapshot exposure wait frames info */
  .snapshot_exp_wait_frames = 2,
  /* number of frames to skip after start stream */
  .sensor_num_frame_skip = 2,
  /* number of frames to skip after start HDR stream */
  .sensor_num_HDR_frame_skip = 2,
  /* sensor exposure table size */
  .exposure_table_size = 10,
  /* sensor lens info */
  .default_lens_info = &default_lens_info,
  /* csi lane params */
  .csi_lane_params = &csi_lane_params,
  /* csi cid params */
  .csi_cid_params = ov5648_8909_cid_cfg,
  /* csi csid params array size */
  .csi_cid_params_size = ARRAY_SIZE(ov5648_8909_cid_cfg),
  /* init settings */
  .init_settings_array = &init_settings_array,
  /* start settings */
  .start_settings = &start_settings,
  /* stop settings */
  .stop_settings = &stop_settings,
  /* group on settings */
  .groupon_settings = &groupon_settings,
  /* group off settings */
  .groupoff_settings = &groupoff_settings,
  /* resolution cfg table */
  .sensor_res_cfg_table = &ov5648_8909_res_table,
  /* res settings */
  .res_settings_array = &res_settings_array,
  /* out info array */
  .out_info_array = &out_info_array,
  /* crop params array */
  .crop_params_array = &crop_params_array,
  /* csi params array */
  .csi_params_array = &csi_params_array,
  /* sensor port info array */
  .sensor_stream_info_array = &ov5648_8909_stream_info_array,
  /* exposure funtion table */
  .exposure_func_table = &ov5648_8909_expsoure_tbl,
  /* chromatix array */
  .chromatix_array = &ov5648_8909_lib_chromatix_array,
};

/*===========================================================================
 * FUNCTION    - ov5648_8909_open_lib -
 *
 * DESCRIPTION:
 *==========================================================================*/
void *ov5648_8909_open_lib(void)
{
  return &sensor_lib_ptr;
}
