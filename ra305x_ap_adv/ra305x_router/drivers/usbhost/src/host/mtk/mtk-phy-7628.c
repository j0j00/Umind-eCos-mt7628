#include "mtk-phy.h"
#include "mtk-phy-7628.h"
#include <cyg/hal/hal_if.h>

int phy_init_flag = 0;
int slewrate_cal_flag = 0;

PHY_INT32 phy_init(struct u3phy_info *info)
{
	PHY_INT32 temp;

    diag_printf("7628 phy_init 11\n");
	if (phy_init_flag == 0) {
        diag_printf("7628 phy_init 22\n");
		temp = U3PhyReadReg32(((PHY_UINT32)&info->u2phy_regs->u2phyac2));
		temp = U3PhyReadReg32(((PHY_UINT32)&info->u2phy_regs->u2phyacr0));
		temp = U3PhyReadReg32(((PHY_UINT32)&info->u2phy_regs->u2phydcr0));

		U3PhyWriteReg32(((PHY_UINT32)&info->u2phy_regs->u2phydcr0), 0xffff02);
		temp = U3PhyReadReg32(((PHY_UINT32)&info->u2phy_regs->u2phydcr0));
		U3PhyWriteReg32(((PHY_UINT32)&info->u2phy_regs->u2phydcr0), 0x555502);
		temp = U3PhyReadReg32(((PHY_UINT32)&info->u2phy_regs->u2phydcr0));
		U3PhyWriteReg32(((PHY_UINT32)&info->u2phy_regs->u2phydcr0), 0xaaaa02);
		temp = U3PhyReadReg32(((PHY_UINT32)&info->u2phy_regs->u2phydcr0));
		U3PhyWriteReg32(((PHY_UINT32)&info->u2phy_regs->u2phydcr0), 0x402);
		temp = U3PhyReadReg32(((PHY_UINT32)&info->u2phy_regs->u2phydcr0));

		U3PhyWriteReg32(((PHY_UINT32)&info->u2phy_regs->u2phyac0), 0x48086a);
		U3PhyWriteReg32(((PHY_UINT32)&info->u2phy_regs->u2phyac1), 0x4400001c);
		U3PhyWriteReg32(((PHY_UINT32)&info->u2phy_regs->u2phyacr3), 0xc0200000);
		U3PhyWriteReg32(((PHY_UINT32)&info->u2phy_regs->u2phydtm0), 0x2000000);

		phy_init_flag = 1;
	}

	return PHY_TRUE;
}


//not used on SoC
PHY_INT32 phy_change_pipe_phase(struct u3phy_info *info, PHY_INT32 phy_drv, PHY_INT32 pipe_phase)
{
	return PHY_TRUE;
}

PHY_INT32 eyescan_init(struct u3phy_info *info)
{
	return PHY_TRUE;
}

PHY_INT32 phy_eyescan(struct u3phy_info *info, PHY_INT32 x_t1, PHY_INT32 y_t1, PHY_INT32 x_br, PHY_INT32 y_br, PHY_INT32 delta_x, PHY_INT32 delta_y
		, PHY_INT32 eye_cnt, PHY_INT32 num_cnt, PHY_INT32 PI_cal_en, PHY_INT32 num_ignore_cnt)
{
	return PHY_TRUE;
}

PHY_INT32 u2_slew_rate_calibration(struct u3phy_info *info)
{
	PHY_INT32 i=0;
	//PHY_INT32 j=0;
	//PHY_INT8 u1SrCalVal = 0;
	//PHY_INT8 u1Reg_addr_HSTX_SRCAL_EN;
	PHY_INT32 fgRet = 0;	
	PHY_INT32 u4FmOut = 0;	
	PHY_INT32 u4Tmp = 0;
	//PHY_INT32 temp;

	if (slewrate_cal_flag == 0) {
		slewrate_cal_flag = 1;

		// => RG_USB20_HSTX_SRCAL_EN = 1
		// enable HS TX SR calibration
		U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->u2phyacr0)
			, RG_USB20_HSTX_SRCAL_EN_OFST, RG_USB20_HSTX_SRCAL_EN, 0x1);
		//DRV_MSLEEP(1);
		CYGACC_CALL_IF_DELAY_US(1 * 1000); // mdelay(100) Jody

		// => RG_FRCK_EN = 1    
		// Enable free run clock
		U3PhyWriteField32(((PHY_UINT32)&info->sifslv_fm_regs->fmmonr1)
			, RG_FRCK_EN_OFST, RG_FRCK_EN, 1);

		// MT6290 HS signal quality patch
		// => RG_CYCLECNT = 400
		// Setting cyclecnt =400
		U3PhyWriteField32(((PHY_UINT32)&info->sifslv_fm_regs->fmcr0)
			, RG_CYCLECNT_OFST, RG_CYCLECNT, 0x400);

		// => RG_FREQDET_EN = 1
		// Enable frequency meter
		U3PhyWriteField32(((PHY_UINT32)&info->sifslv_fm_regs->fmcr0)
			, RG_FREQDET_EN_OFST, RG_FREQDET_EN, 0x1);

		// wait for FM detection done, set 10ms timeout
		for(i=0; i<10; i++){
			// => u4FmOut = USB_FM_OUT
			// read FM_OUT
			u4FmOut = U3PhyReadReg32(((PHY_UINT32)&info->sifslv_fm_regs->fmmonr0));
			diag_printf("FM_OUT value: u4FmOut = %d(0x%08X)\n", u4FmOut, u4FmOut);

			// check if FM detection done 
			if (u4FmOut != 0)
			{
				fgRet = 0;
				diag_printf("FM detection done! loop = %d\n", i);
			
				break;
			}

			fgRet = 1;
			//DRV_MSLEEP(1);
			CYGACC_CALL_IF_DELAY_US(1 * 1000); // mdelay(100) Jody
		}
		// => RG_FREQDET_EN = 0
		// disable frequency meter
		U3PhyWriteField32(((PHY_UINT32)&info->sifslv_fm_regs->fmcr0)
			, RG_FREQDET_EN_OFST, RG_FREQDET_EN, 0);

		// => RG_FRCK_EN = 0
		// disable free run clock
		U3PhyWriteField32(((PHY_UINT32)&info->sifslv_fm_regs->fmmonr1)
			, RG_FRCK_EN_OFST, RG_FRCK_EN, 0);

		// => RG_USB20_HSTX_SRCAL_EN = 0
		// disable HS TX SR calibration
		U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->u2phyacr0)
			, RG_USB20_HSTX_SRCAL_EN_OFST, RG_USB20_HSTX_SRCAL_EN, 0);
		//DRV_MSLEEP(1);
		CYGACC_CALL_IF_DELAY_US(1 * 1000); // mdelay(100) Jody

		if(u4FmOut == 0){
			U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->u2phyacr0)
				, RG_USB20_HSTX_SRCTRL_OFST, RG_USB20_HSTX_SRCTRL, 0x4);
		
			fgRet = 1;
		}
		else{
			// set reg = (1024/FM_OUT) * 25 * 0.028 (round to the nearest digits)
			u4Tmp = (((1024 * 25 * U2_SR_COEF_7628) / u4FmOut) + 500) / 1000;
			diag_printf("SR calibration value u1SrCalVal = %d\n", (PHY_UINT8)u4Tmp);
			U3PhyWriteField32(((PHY_UINT32)&info->u2phy_regs->u2phyacr0)
				, RG_USB20_HSTX_SRCTRL_OFST, RG_USB20_HSTX_SRCTRL, u4Tmp);
		}
	}

	return fgRet;
}

