/*
 ***************************************************************************
 * MediaTek Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 1997-2012, MediaTek, Inc.
 *
 * All rights reserved. MediaTek source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of MediaTek. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of MediaTek Technology, Inc. is obtained.
 ***************************************************************************

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define PATH_OF_MCU_BIN_IN "/embedded/mcu/bin/"
#define PATH_OF_MCU_BIN_OUT "/embedded/include/mcu/"
#define PATH_OF_EEPROM_IN "/eeprom/"
#define PATH_OF_EEPROM_OUT "/embedded/include/eeprom/"
#define PATH_OF_ROM_PATCH_IN "/embedded/mcu/bin/"
#define PATH_OF_ROM_PATCH_OUT "/embedded/include/mcu/"


int bin2h(char *infname, char *outfname, char *fw_name)
{
	int ret = 0;
    FILE *infile, *outfile;
    unsigned char c;
    int i=0;

    infile = fopen(infname,"r");

    if (infile == (FILE *) NULL) {
		printf("Can't read file %s \n",infname);
		return -1;
    }

    outfile = fopen(outfname,"w");

    if (outfile == (FILE *) NULL) {
		printf("Can't open write file %s \n",outfname);
		fclose(infile);
       	return -1;
    }

    fputs("/* AUTO GEN PLEASE DO NOT MODIFY IT */ \n",outfile);
    fputs("/* AUTO GEN PLEASE DO NOT MODIFY IT */ \n",outfile);
    fputs("\n",outfile);
    fputs("\n",outfile);

	fprintf(outfile, "UCHAR %s[] = {\n", fw_name);

    while(1) {
		char cc[3];

		c = getc(infile);

		if (feof(infile))
	   		break;

		memset(cc,0,2);

		if (i >= 16) {
	   		fputs("\n", outfile);
	   		i = 0;
		}

		fputs("0x", outfile);
		sprintf(cc,"%02x",c);
		fputs(cc, outfile);
		fputs(", ", outfile);
		i++;
    }

    fputs("} ;\n", outfile);
    fclose(infile);
    fclose(outfile);
}

int main(int argc ,char *argv[])
{
    char infname[512], ine2pname[512], in_rom_patch[512];
	char infname1[512];
    char outfname[512], oute2pname[512], out_rom_patch[512];
	char outfname1[512];
	char chipsets[1024];
	char fw_name[128], e2p_name[128], rom_patch_name[128];
	char fw_name1[128];
    char *rt28xxdir;
    char *chipset, *token;
	char *wow, *rt28xx_mode;
	int is_bin2h_fw = 0, is_bin2h_rom_patch = 0, is_bin2h_e2p=0;

//#ifdef __ECOS	
	//char *singleSKU_v2;
	char inskuname[512];
	char outskuname[512];
	char sku_name[128];
	int is_bin2h_sku = 0;
//#endif
	int len=0;

    rt28xxdir = (char *)getenv("RT28xx_DIR");
    chipset = (char *)getenv("CHIPSET");
	wow = (char *)getenv("HAS_WOW_SUPPORT");
	rt28xx_mode = (char *)getenv("RT28xx_MODE");

//#ifdef __ECOS
	//singleSKU_v2 = (char *)getenv("HAS_SINGLE_SKU_V2_SUPPORT");
//#endif

    if(!rt28xxdir) {
		printf("Environment value \"RT28xx_DIR\" not export \n");
	 	return -1;
    }

    if(!chipset) {
		printf("Environment value \"CHIPSET\" not export \n");
		return -1;
    }
	len = strlen(chipset);
	if(len > 1024)
		len = 1024;
	memcpy(chipsets, chipset, strlen(chipset));

	if (strlen(rt28xxdir) > (sizeof(infname)-100)) {
		printf("Environment value \"RT28xx_DIR\" is too long!\n");
		return -1;
	}

	chipset = strtok(chipsets, " ");

	while (chipset != NULL) {
		printf("chipset = %s\n", chipset);
    	memset(infname, 0, 512);
    	memset(infname1, 0, 512);
		memset(ine2pname, 0, 512);
    	memset(outfname, 0, 512);
    	memset(outfname1, 0, 512);
		memset(oute2pname, 0, 512);
		memset(fw_name, 0, 128);
		memset(fw_name1, 0, 128);
		memset(e2p_name, 0, 128);
		memset(in_rom_patch, 0, 512);
		memset(out_rom_patch, 0, 512);
		memset(rom_patch_name, 0, 128);
    	strcat(infname,rt28xxdir);
    	strcat(infname1,rt28xxdir);
		strcat(ine2pname, rt28xxdir);
		strcat(in_rom_patch, rt28xxdir);
    	strcat(outfname,rt28xxdir);
    	strcat(outfname1,rt28xxdir);
		strcat(oute2pname, rt28xxdir);
		strcat(out_rom_patch, rt28xxdir);
		is_bin2h_fw = 0;
		is_bin2h_rom_patch = 0;
		is_bin2h_e2p = 0;
        strcat(infname,PATH_OF_MCU_BIN_IN);
        strcat(outfname,PATH_OF_MCU_BIN_OUT);
        strcat(ine2pname, PATH_OF_EEPROM_IN);
        strcat(oute2pname, PATH_OF_EEPROM_OUT);
        strcat(in_rom_patch, PATH_OF_ROM_PATCH_IN);
        strcat(out_rom_patch, PATH_OF_ROM_PATCH_OUT);
        strcat(infname1, PATH_OF_MCU_BIN_IN);
        strcat(outfname1, PATH_OF_MCU_BIN_OUT);
//#ifdef __ECOS
		memset(inskuname, 0, 512);
		memset(outskuname, 0, 512);
		memset(sku_name, 0, 128);
		strcat(inskuname,rt28xxdir);
		strcat(outskuname,rt28xxdir);
		is_bin2h_sku = 0;
		strcat(inskuname,PATH_OF_EEPROM_IN);
		strcat(outskuname,PATH_OF_EEPROM_OUT);
//#endif

		if (strncmp(chipset, "2860",4) == 0) {
			strcat(infname,"rt2860.bin");
    		strcat(outfname,"rt2860_firmware.h");
			strcat(fw_name, "RT2860_FirmwareImage");
			is_bin2h_fw = 1;
		} else if (strncmp(chipset, "2870",4) == 0) {
			if (wow && (strncmp(wow, "y", 1) == 0) && rt28xx_mode && (strncmp(rt28xx_mode, "STA", 3) == 0)) {
	    		strcat(infname,"rt2870_wow.bin");
    			strcat(outfname,"rt2870_wow_firmware.h");
				strcat(fw_name, "RT2870_WOW_FirmwareImage");
				is_bin2h_fw = 1;
			} else {
	    		strcat(infname,"rt2870.bin");
    			strcat(outfname,"rt2870_firmware.h");
				strcat(fw_name, "RT2870_FirmwareImage");
				is_bin2h_fw = 1;
			}
		} else if (strncmp(chipset, "3090",4) == 0) {
	    	strcat(infname,"rt2860.bin");
    		strcat(outfname,"rt2860_firmware.h");
			strcat(fw_name, "RT2860_FirmwareImage");
			is_bin2h_fw = 1;
		} else if (strncmp(chipset, "2070",4) == 0) {
			if (wow && (strncmp(wow, "y", 1) == 0) && rt28xx_mode && (strncmp(rt28xx_mode, "STA", 3) == 0)) {
	    		strcat(infname,"rt2870_wow.bin");
    			strcat(outfname,"rt2870_wow_firmware.h");
				strcat(fw_name, "RT2870_WOW_FirmwareImage");
				is_bin2h_fw = 1;
			} else {
	    		strcat(infname,"rt2870.bin");
    			strcat(outfname,"rt2870_firmware.h");
				strcat(fw_name, "RT2870_FirmwareImage");
				is_bin2h_fw = 1;
			}
		} else if (strncmp(chipset, "3070",4) == 0) {
			if (wow && (strncmp(wow, "y", 1) == 0) && rt28xx_mode && (strncmp(rt28xx_mode, "STA", 3) == 0)) {
	    		strcat(infname,"rt2870_wow.bin");
    			strcat(outfname,"rt2870_wow_firmware.h");
				strcat(fw_name, "RT2870_WOW_FirmwareImage");
				is_bin2h_fw = 1;
			} else {
	    		strcat(infname,"rt2870.bin");
    			strcat(outfname,"rt2870_firmware.h");
				strcat(fw_name, "RT2870_FirmwareImage");
				is_bin2h_fw = 1;
			}
		} else if (strncmp(chipset, "3572",4) == 0) {
			if (wow && (strncmp(wow, "y", 1) == 0) && rt28xx_mode && (strncmp(rt28xx_mode, "STA", 3) == 0)) {
	    		strcat(infname,"rt2870_wow.bin");
    			strcat(outfname,"rt2870_wow_firmware.h");
				strcat(fw_name, "RT2870_WOW_FirmwareImage");
				is_bin2h_fw = 1;
			} else {
	    		strcat(infname,"rt2870.bin");
    			strcat(outfname,"rt2870_firmware.h");
				strcat(fw_name, "RT2870_FirmwareImage");
				is_bin2h_fw = 1;
			}
		} else if (strncmp(chipset, "3573",4) == 0) {
			if (wow && (strncmp(wow, "y", 1) == 0) && rt28xx_mode && (strncmp(rt28xx_mode, "STA", 3) == 0)) {
	    		strcat(infname,"rt2870_wow.bin");
    			strcat(outfname,"rt2870_wow_firmware.h");
				strcat(fw_name, "RT2870_WOW_FirmwareImage");
				is_bin2h_fw = 1;
			} else {
	    		strcat(infname,"rt2870.bin");
    			strcat(outfname,"rt2870_firmware.h");
				strcat(fw_name, "RT2870_FirmwareImage");
				is_bin2h_fw = 1;
			}
		} else if (strncmp(chipset, "3370",4) == 0) {
			if (wow && (strncmp(wow, "y", 1) == 0) && rt28xx_mode && (strncmp(rt28xx_mode, "STA", 3) == 0)) {
	    		strcat(infname,"rt2870_wow.bin");
    			strcat(outfname,"rt2870_wow_firmware.h");
				strcat(fw_name, "RT2870_WOW_FirmwareImage");
				is_bin2h_fw = 1;
			} else {
	    		strcat(infname,"rt2870.bin");
    			strcat(outfname,"rt2870_firmware.h");
				strcat(fw_name, "RT2870_FirmwareImage");
				is_bin2h_fw = 1;
			}
		} else if (strncmp(chipset, "5370",4) == 0) {
			if (wow && (strncmp(wow, "y", 1) == 0) && rt28xx_mode && (strncmp(rt28xx_mode, "STA", 3) == 0)) {
	    		strcat(infname,"rt2870_wow.bin");
    			strcat(outfname,"rt2870_wow_firmware.h");
				strcat(fw_name, "RT2870_WOW_FirmwareImage");
				is_bin2h_fw = 1;
			} else {
	    		strcat(infname,"rt2870.bin");
    			strcat(outfname,"rt2870_firmware.h");
				strcat(fw_name, "RT2870_FirmwareImage");
				is_bin2h_fw = 1;
			}
		} else if (strncmp(chipset, "5572",4) == 0) {
			strcat(infname,"rt2870.bin");
    		strcat(outfname,"rt2870_firmware.h");
			strcat(fw_name, "RT2870_FirmwareImage");
			is_bin2h_fw = 1;
		} else if (strncmp(chipset, "5592",4) == 0) {
			strcat(infname,"rt2860.bin");
    		strcat(outfname,"rt2860_firmware.h");
			strcat(fw_name, "RT2860_FirmwareImage");
			is_bin2h_fw = 1;
		} else if ((strncmp(chipset, "mt7601e", 7) == 0)
				|| (strncmp(chipset, "mt7601u", 7) == 0)) {
			strcat(infname,"MT7601_formal_1.7.bin");
			strcat(outfname,"mt7601_firmware.h");
			strcat(fw_name, "MT7601_FirmwareImage");
			is_bin2h_fw = 1;
			strcat(ine2pname, "MT7601_USB_V0_D-20130416.bin");
			strcat(oute2pname, "mt7601_e2p.h");
			strcat(e2p_name, "MT7601_E2PImage");
			is_bin2h_e2p = 1;
		} else if ((strncmp(chipset, "mt7650e", 7) == 0)
				|| (strncmp(chipset, "mt7650u", 7) == 0)
				|| (strncmp(chipset, "mt7630e", 7) == 0)
				|| (strncmp(chipset, "mt7630u", 7) == 0)) {
			strcat(infname, "MT7650.bin"); // pmu
    		strcat(outfname,"mt7650_firmware.h");
			strcat(fw_name, "MT7650_FirmwareImage");
			is_bin2h_fw = 1;
		} else if ((strncmp(chipset, "mt7610e", 7) == 0)
				|| (strncmp(chipset, "mt7610u", 7) == 0)) {
			strcat(infname, "MT7650.bin"); // pmu
    		strcat(outfname, "mt7610_firmware.h");
			strcat(fw_name, "MT7610_FirmwareImage");
			is_bin2h_fw = 1;

			if ((strncmp(chipset, "mt7610e", 7) == 0)) {
				strcat(ine2pname, "MT7610U_FEM_V1_1.bin");
				strcat(oute2pname, "mt7610e_e2p.h");
				strcat(e2p_name, "MT7610E_E2PImage");
			} else if ((strncmp(chipset, "mt7610u", 7) == 0)) {
				strcat(ine2pname, "MT7610U_FEM_V1_1.bin");
				strcat(oute2pname, "mt7610u_e2p.h");
				strcat(e2p_name, "MT7610U_E2PImage");
			}
		} else if ((strncmp(chipset, "mt7662e", 7) == 0)
				|| (strncmp(chipset, "mt7662u", 7) == 0)
				|| (strncmp(chipset, "mt7632e", 7) == 0)
				|| (strncmp(chipset, "mt7632u", 7) == 0)
				|| (strncmp(chipset, "mt7612e", 7) == 0)
				|| (strncmp(chipset, "mt7612u", 7) == 0)) {
			strcat(infname, "mt7662_firmware_e3_v1.4.bin");
    		strcat(outfname, "mt7662_firmware.h");
			strcat(fw_name, "MT7662_FirmwareImage");
			strcat(in_rom_patch, "mt7662_patch_e3_hdr_v0.0.2_P48.bin");
			strcat(out_rom_patch, "mt7662_rom_patch.h");
			strcat(rom_patch_name, "mt7662_rom_patch");
			strcat(ine2pname, "MT7662E2_EEPROM_20130527.bin");
			strcat(oute2pname, "mt76x2_e2p.h");
			strcat(e2p_name, "MT76x2_E2PImage");
			is_bin2h_fw = 1;
			is_bin2h_rom_patch = 1;
			is_bin2h_e2p = 1;
		} else if ((strncmp(chipset, "mt7636u", 7) == 0)
					|| (strncmp(chipset, "mt7636s", 7) == 0)) {
			strcat(in_rom_patch, "mt7636_patch_e1_hdr.bin");
			strcat(out_rom_patch, "mt7636_rom_patch.h");
			strcat(rom_patch_name, "mt7636_rom_patch");
			strcat(infname, "WIFI_RAM_CODE_MT7636.bin");
    		strcat(outfname, "mt7636_firmware.h");
			strcat(fw_name, "MT7636_FirmwareImage");

			strcat(ine2pname, "iPAiLNA/MT7636_EEPROM.bin");
			strcat(oute2pname, "mt7636_e2p.h");
			strcat(e2p_name, "MT7636_E2PImage");
			is_bin2h_fw = 1;
			is_bin2h_rom_patch = 1;
			is_bin2h_e2p = 1;
		} else if ((strncmp(chipset, "mt7603e", 7) == 0)
					|| (strncmp(chipset, "mt7603u", 7) == 0)) {
			strcat(infname, "WIFI_RAM_CODE_MT7603_e1.bin");
			strcat(outfname, "mt7603_firmware.h");
			strcat(infname1, "WIFI_RAM_CODE_MT7603_e2.bin");
			//strcat(infname1, "MT7603_ram_20140305_e2_drv_tv01.bin");
			strcat(outfname1, "mt7603_e2_firmware.h");
			strcat(fw_name, "MT7603_FirmwareImage");
			strcat(fw_name1, "MT7603_e2_FirmwareImage");

			strcat(e2p_name, "MT7603_E2PImage");
			strcat(ine2pname, "MT7603E_EEPROM.bin");
			strcat(oute2pname, "mt7603_e2p.h");
			is_bin2h_fw = 1;
			is_bin2h_e2p = 1;
		} else if (strncmp(chipset, "mt7628", 7) == 0) {
			//strcat(infname, "MT7628_ram_20140212_fpga_tv01.bin");
            		strcat(infname, "WIFI_RAM_CODE_MT7628_e1.bin");
			strcat(outfname, "mt7628_firmware.h");
			strcat(fw_name, "MT7628_FirmwareImage");
			strcat(infname1, "WIFI_RAM_CODE_MT7628_e2.bin");
			strcat(outfname1, "mt7628_e2_firmware.h");
			strcat(fw_name1, "MT7628_e2_FirmwareImage");

			strcat(e2p_name, "MT7628_E2PImage");
			strcat(ine2pname, "MT7603E1E2_EEPROM_layout_20140226.bin");
			strcat(oute2pname, "mt7628_e2p.h");
			is_bin2h_fw = 0;
			is_bin2h_e2p = 0;
//#ifdef __ECOS
		if (1/*strncmp(singleSKU_v2, "y", 1) == 0*/){
			strcat(sku_name, "MT7628_SKUImage");
			strcat(inskuname, "SingleSKU.DAT");
			strcat(outskuname, "mt7628_sku.h");
			is_bin2h_sku = 1;
		}
//#endif
			
        } else if (strncmp(chipset, "mt7615", 7) == 0) {
            //strcat(infname, "MT7615_ram_20140212_fpga_tv01.bin");
            strcat(infname, "WIFI_RAM_CODE_MT7615.bin");
            strcat(outfname, "mt7615_firmware.h");
            strcat(fw_name, "MT7615_FirmwareImage");
            is_bin2h_fw = 1;
		} else {
			printf("unknown chipset = %s\n", chipset);
		}

		if (is_bin2h_fw)
		{
     		bin2h(infname, outfname, fw_name);
     		bin2h(infname1, outfname1, fw_name1);
		}

		if (is_bin2h_rom_patch)
			bin2h(in_rom_patch, out_rom_patch, rom_patch_name);

		if (is_bin2h_e2p)
			bin2h(ine2pname, oute2pname, e2p_name);
		
//#ifdef __ECOS
		if (is_bin2h_sku)
			bin2h(inskuname, outskuname, sku_name);
//#endif

		chipset = strtok(NULL, " ");
	}

    exit(0);
}