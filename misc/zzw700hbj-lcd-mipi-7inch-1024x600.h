#ifndef __TELPO_ZZW700HBJ_LCD_MIPI_7INCH_1024X600_H__
#define __TELPO_ZZW700HBJ_LCD_MIPI_7INCH_1024X600_H__

#include "sample_comm.h"
#include "hi_mipi_tx.h"

/*============================= mipi 7 inch 1024x600 lcd config ====================================*/
combo_dev_cfg_t ZZW700HBJ_MIPI_TX_7INCH_1024X600_60_CONFIG =
{
    .devno = 0,
    .lane_id = {0, 1, 2, 3},
    .output_mode = OUTPUT_MODE_DSI_VIDEO,
    .output_format = OUT_FORMAT_RGB_24_BIT,
    .video_mode =  BURST_MODE,

    .sync_info = {
        .vid_pkt_size     = 1024, // hact
        .vid_hsa_pixels   = 4,  // hsa   //20
        .vid_hbp_pixels   = 60,  // hbp  //20
        .vid_hline_pixels = 1224, // hact + hsa + hbp + hfp  //hfb=32 //972
        .vid_vsa_lines    = 2,   // vsa
        .vid_vbp_lines    = 16,  // vbp
        .vid_vfp_lines    = 16,   // vfp
        .vid_active_lines = 634,// vact
        .edpi_cmd_size    = 0,
    },

    .phy_data_rate = 495,
    .pixel_clk = 46561,
};

VO_SYNC_INFO_S ZZW700HBJ_MIPI_TX_7INCH_1024X600_60_SYNC_INFO = 
{
	.u16Hact	= 1024,
	.u16Hbb		= 64,
	.u16Hfb		= 136,
	.u16Hpw		= 4,
	.u16Vact	= 600,
	.u16Vbb		= 18,
	.u16Vfb		= 16,
	.u16Vpw		= 2,
};

VO_USER_INTFSYNC_INFO_S ZZW700HBJ_MIPI_TX_7INCH_1024X600_60_USER_INTFSYNC_INFO = 
{
	.stUserIntfSyncAttr = 
	{
		.stUserSyncPll	= 
		{
			.u32Fbdiv	= 380,
			.u32Frac	= 0x3f7271,
			.u32Refdiv	= 4,
			.u32Postdiv1= 7,
			.u32Postdiv2= 7,
		},
	},

	.u32DevDiv			= 1,
	.u32PreDiv			= 1,
};

lcd_resoluton_t ZZW700HBJ_MIPI_TX_7INCH_1024X600_60_LCD_RESOLUTION = 
{
	.pu32W	= 1024,
	.pu32H	= 600,
	.pu32Frm= 60,
};

HI_VOID ZZW700HBJ_InitScreen_mipi_7inch_1024x600(HI_S32 s32fd)
{

    HI_S32     fd = s32fd;
    HI_S32     s32Ret;
    cmd_info_t cmd_info = {0};
    
	SAMPLE_PRT("%s, %d.\n", __FUNCTION__, __LINE__);

	cmd_info.devno         = 0;
	cmd_info.cmd_size      = 0xAB80;
	cmd_info.data_type     = 0x15;
	cmd_info.cmd           = NULL;
    s32Ret = ioctl(fd, HI_MIPI_TX_SET_CMD, &cmd_info);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
	usleep(1000);

    cmd_info.devno         = 0;
	cmd_info.cmd_size      = 0x4B81;
	cmd_info.data_type     = 0x15;
	cmd_info.cmd           = NULL;
    s32Ret = ioctl(fd, HI_MIPI_TX_SET_CMD, &cmd_info);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
	usleep(1000);

    cmd_info.devno         = 0;
	cmd_info.cmd_size      = 0x8482;
	cmd_info.data_type     = 0x15;
	cmd_info.cmd           = NULL;
    s32Ret = ioctl(fd, HI_MIPI_TX_SET_CMD, &cmd_info);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
	usleep(1000);

    cmd_info.devno         = 0;
	cmd_info.cmd_size      = 0x8883;
	cmd_info.data_type     = 0x15;
	cmd_info.cmd           = NULL;
    s32Ret = ioctl(fd, HI_MIPI_TX_SET_CMD, &cmd_info);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
	usleep(1000);

    cmd_info.devno         = 0;
	cmd_info.cmd_size      = 0xA884;
	cmd_info.data_type     = 0x15;
	cmd_info.cmd           = NULL;
    s32Ret = ioctl(fd, HI_MIPI_TX_SET_CMD, &cmd_info);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
	usleep(1000);

    cmd_info.devno         = 0;
	cmd_info.cmd_size      = 0xE385;
	cmd_info.data_type     = 0x15;
	cmd_info.cmd           = NULL;
    s32Ret = ioctl(fd, HI_MIPI_TX_SET_CMD, &cmd_info);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
	usleep(1000);

    cmd_info.devno         = 0;
	cmd_info.cmd_size      = 0xB886;
	cmd_info.data_type     = 0x15;
	cmd_info.cmd           = NULL;
    s32Ret = ioctl(fd, HI_MIPI_TX_SET_CMD, &cmd_info);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
	usleep(1000);

    cmd_info.devno         = 0;
	cmd_info.cmd_size      = 0x11;
	cmd_info.data_type     = 0x05;
	cmd_info.cmd           = NULL;
    s32Ret = ioctl(fd, HI_MIPI_TX_SET_CMD, &cmd_info);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
	usleep(120000);

	cmd_info.devno         = 0;
	cmd_info.cmd_size      = 0x29;
	cmd_info.data_type     = 0x05;
	cmd_info.cmd           = NULL;
    s32Ret = ioctl(fd, HI_MIPI_TX_SET_CMD, &cmd_info);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("MIPI_TX SET CMD failed\n");
        close(fd);
        return;
    }
	usleep(5000);
}

#endif
