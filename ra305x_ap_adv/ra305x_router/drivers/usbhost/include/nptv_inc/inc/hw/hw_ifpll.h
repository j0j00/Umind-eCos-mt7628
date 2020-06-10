#ifndef _HW_IFPLL_H_
#define _HW_IFPLL_H_

//Page HISTORY
#define IO_IFPLL_BASE (IO_VIRT + 0x61000)

//Page TABLE
#define REG_IFPLL_CFG0 (IO_IFPLL_BASE + 0x600)
    #define FLD_RG_ULJPLL_PWD Fld(1,31,AC_MSKB3)//[31:31]
    #define FLD_RG_ULJPLL_FBDIV Fld(7,24,AC_MSKB3)//[30:24]
    #define FLD_RG_ULJPLL_POSDIV Fld(2,22,AC_MSKB2)//[23:22]
    #define FLD_RG_ULJPLL_CKCTRL Fld(2,20,AC_MSKB2)//[21:20]
    #define FLD_RG_ULJPLL_FBSEL Fld(2,18,AC_MSKB2)//[19:18]
    #define FLD_RG_ULJPLL_BPC Fld(2,16,AC_MSKB2)//[17:16]
    #define FLD_RG_ULJPLL_BPA Fld(5,11,AC_MSKB1)//[15:11]
    #define FLD_RG_ULJPLL_BPB Fld(3,8,AC_MSKB1)//[10:8]
    #define FLD_RG_ULJPLL_FPEN Fld(1,7,AC_MSKB0)//[7:7]
    #define FLD_RG_ULJPLL_BAND Fld(6,1,AC_MSKB0)//[6:1]
    #define FLD_RG_ULJPLL_ACCEN Fld(1,0,AC_MSKB0)//[0:0]
#define REG_IFPLL_CFG1 (IO_IFPLL_BASE + 0x604)
    #define FLD_RG_ULJPLL_BR Fld(3,29,AC_MSKB3)//[31:29]
    #define FLD_RG_ULJPLL_BI Fld(4,25,AC_MSKB3)//[28:25]
    #define FLD_RG_ULJPLL_AUTOK_VCO Fld(1,24,AC_MSKB3)//[24:24]
    #define FLD_RG_ULJPLL_AUTOK_LOAD Fld(1,23,AC_MSKB2)//[23:23]
    #define FLD_RG_ULJPLL_BC Fld(5,18,AC_MSKB2)//[22:18]
    #define FLD_RG_ULJPLL_KVSEL Fld(1,17,AC_MSKB2)//[17:17]
    #define FLD_RG_ULJPLL_LOAD_RSTB Fld(1,16,AC_MSKB2)//[16:16]
    #define FLD_RG_ULJPLL_TCADJ Fld(6,10,AC_MSKB1)//[15:10]
    #define FLD_RG_ULJPLL_BYPASS Fld(2,8,AC_MSKB1)//[9:8]
    #define FLD_RG_ULJPLL_RLEN Fld(1,7,AC_MSKB0)//[7:7]
    #define FLD_RG_ULJPLL_RLPOL Fld(1,6,AC_MSKB0)//[6:6]
    #define FLD_RG_ULJPLL_MONEN Fld(1,5,AC_MSKB0)//[5:5]
    #define FLD_RG_ULJPLL_MONCKEN Fld(1,4,AC_MSKB0)//[4:4]
    #define FLD_RG_ULJPLL_DIV3_RSTB Fld(1,3,AC_MSKB0)//[3:3]
    #define FLD_RG_EXSEL Fld(1,2,AC_MSKB0)//[2:2]
    #define FLD_RG_PXSEL Fld(1,1,AC_MSKB0)//[1:1]
    #define FLD_RG_TSTCKEN Fld(1,0,AC_MSKB0)//[0:0]
#define REG_IFPLL_CFG2 (IO_IFPLL_BASE + 0x608)
    #define FLD_RG_CLKEN Fld(1,31,AC_MSKB3)//[31:31]
    #define FLD_RG_DCKEN Fld(1,30,AC_MSKB3)//[30:30]
    #define FLD_RG_CVBSCKEN Fld(1,29,AC_MSKB3)//[29:29]
    #define FLD_RG_PWMCKEN Fld(1,28,AC_MSKB3)//[28:28]
    #define FLD_RG_XTALEN1 Fld(1,27,AC_MSKB3)//[27:27]
    #define FLD_RG_XTALEN2 Fld(1,26,AC_MSKB3)//[26:26]
    #define FLD_RG_ULJPLL_DET_EN Fld(1,25,AC_MSKB3)//[25:25]
    #define FLD_RG_ULJPLL_BAND_OPT Fld(1,24,AC_MSKB3)//[24:24]
    #define FLD_RG_ULJPLL_OD_27M_EN Fld(1,23,AC_MSKB2)//[23:23]
    #define FLD_RG_ULJPLL_RESERVE Fld(7,16,AC_MSKB2)//[22:16]
#define RGS_IFPLL_CFG3 (IO_IFPLL_BASE + 0x60C)
    #define FLD_RGS_ULJPLL_VCOCAL_CPLT Fld(1,31,AC_MSKB3)//[31:31]
    #define FLD_RGS_ULJPLL_VCOCAL_FAIL Fld(1,30,AC_MSKB3)//[30:30]
    #define FLD_RGS_ULJPLL_VCOCAL_STATE Fld(7,23,AC_MSKW32)//[29:23]

#endif /* _HW_IFPLL_H_ */