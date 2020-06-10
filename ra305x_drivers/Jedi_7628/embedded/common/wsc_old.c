/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology	5th	Rd.
 * Science-based Industrial	Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2006, Ralink Technology, Inc.
 *
 * All rights reserved.	Ralink's source	code is	an unpublished work	and	the
 * use of a	copyright notice does not imply	otherwise. This	source code
 * contains	confidential trade secret material of Ralink Tech. Any attemp
 * or participation	in deciphering,	decoding, reverse engineering or in	any
 * way altering	the	source code	is stricitly prohibited, unless	the	prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	wsc.c

	Abstract:

	Revision History:
	Who			When			What
	--------	----------		----------------------------------------------
	Paul Lin	06-08-08		Initial
	Snowpin Lee 06-09-12        Do modifications and Add APIs for AP
	Snowpin Lee 07-04-19        Do modifications and Add APIs for STA
	Snowpin Lee 07-05-17        Do modifications and Add APIs for AP Client
*/

#include    "rt_config.h"
#ifdef __ECOS
#include    "cfg_id.h"
#endif /* __ECOS */

#ifdef WSC_INCLUDED
#include    "wsc_tlv.h"
/*#ifdef LINUX */
/*#include <net/iw_handler.h> */
/*#endif*/

#define WSC_UPNP_MSG_TIMEOUT			(150 * OS_HZ)
#define RTMP_WSC_NLMSG_SIGNATURE_LEN	8
#define MAX_WEPKEYNAME_LEN 				20
#define MAX_WEPKEYTYPE_LEN				20

#ifndef PF_NOFREEZE
#define PF_NOFREEZE  0
#endif

//==============================================================================
//                              add by mjh
//==============================================================================
#define WATCHDOG_TIMEOUT 30 /* 30 sec default timeout */
#define RALINK_SYSCTL_BASE 0xB0000000             //gpiomode base
#define RALINK_TIMER_BASE 0xB0000100
#define TMRSTAT     (RALINK_TIMER_BASE)  /* Timer Status Register */
#define TMR0LOAD    (TMRSTAT + 0x10)  /* Timer0 Load Value */
#define TMR0VAL     (TMRSTAT + 0x14)  /* Timer0 Counter Value */
#define TMR0CTL     (TMRSTAT + 0x18)  /* Timer0 Control */
#define TMR1LOAD    (TMRSTAT + 0x20)  /* Timer1 Load Value */
#define TMR1VAL     (TMRSTAT + 0x24)  /* Timer1 Counter Value */
#define TMR1CTL     (TMRSTAT + 0x28)  /* Timer1 Control */
#define SYSCFG      RALINK_SYSCTL_BASE + 0x10  /* System Configuration Register */
#define GPIOMODE    RALINK_SYSCTL_BASE + 0x60
#define RALINK_REG_GPIOMODE2		(GPIOMODE + 0x4)  //7628

//mjh from 7628 sdk
#define RALINK_IRQ_ADDR			RALINK_INTCL_BASE
#define RALINK_PRGIO_ADDR		RALINK_PIO_BASE    //gpio dir base

//search in rt_mmap.h pwd : sdk asm/rt2880
#define RALINK_INTCL_BASE		0xB0000200
#define RALINK_PIO_BASE			0xB0000600

#define RALINK_REG_INTENA		(RALINK_IRQ_ADDR   + 0x80)
#define RALINK_REG_INTDIS		(RALINK_IRQ_ADDR   + 0x78)

#define RALINK_REG_PIOINT		(RALINK_PRGIO_ADDR + 0x90)
#define RALINK_REG_PIOEDGE		(RALINK_PRGIO_ADDR + 0xA0)
#define RALINK_REG_PIORENA		(RALINK_PRGIO_ADDR + 0x50)
#define RALINK_REG_PIOFENA		(RALINK_PRGIO_ADDR + 0x60)
#define RALINK_REG_PIODATA		(RALINK_PRGIO_ADDR + 0x20)
#define RALINK_REG_PIODIR		(RALINK_PRGIO_ADDR + 0x00)
#define RALINK_REG_PIOSET		(RALINK_PRGIO_ADDR + 0x30)
#define RALINK_REG_PIORESET		(RALINK_PRGIO_ADDR + 0x40)
//--------------------------------------------------
#define RALINK_REG_PIO6332INT		(RALINK_PRGIO_ADDR + 0x94)
#define RALINK_REG_PIO6332EDGE		(RALINK_PRGIO_ADDR + 0xA4)
#define RALINK_REG_PIO6332RENA		(RALINK_PRGIO_ADDR + 0x54)
#define RALINK_REG_PIO6332FENA		(RALINK_PRGIO_ADDR + 0x64)
#define RALINK_REG_PIO6332DATA		(RALINK_PRGIO_ADDR + 0x24)
#define RALINK_REG_PIO6332DIR		(RALINK_PRGIO_ADDR + 0x04)
#define RALINK_REG_PIO6332SET		(RALINK_PRGIO_ADDR + 0x34)
#define RALINK_REG_PIO6332RESET		(RALINK_PRGIO_ADDR + 0x44)

#define RALINK_REG_PIO9564INT		(RALINK_PRGIO_ADDR + 0x98)
#define RALINK_REG_PIO9564EDGE		(RALINK_PRGIO_ADDR + 0xA8)
#define RALINK_REG_PIO9564RENA		(RALINK_PRGIO_ADDR + 0x58)
#define RALINK_REG_PIO9564FENA		(RALINK_PRGIO_ADDR + 0x68)
#define RALINK_REG_PIO9564DATA		(RALINK_PRGIO_ADDR + 0x28)
#define RALINK_REG_PIO9564DIR		(RALINK_PRGIO_ADDR + 0x08)
#define RALINK_REG_PIO9564SET		(RALINK_PRGIO_ADDR + 0x38)
#define RALINK_REG_PIO9564RESET		(RALINK_PRGIO_ADDR + 0x48)

//mjh 
RTMP_ADAPTER *pAd_mjh = NULL;
char mjh_wsc_flag = 0;

extern char mjh_wsc_fail;  //wsc fail flag
extern char mjh_wsc_link;
extern char mjh_wsc_flag;
char mjh_2_out = 0;
//********************************************************************

char WSC_MSG_SIGNATURE[]={"RAWSCMSG"};

extern UCHAR   WPS_OUI[];
extern UCHAR	RALINK_OUI[];

#ifdef IWSC_SUPPORT
extern UCHAR	IWSC_OUI[];
#endif // IWSC_SUPPORT //

#if defined(__ECOS) && defined(BRANCH_ADV)
extern int CFG_set(int id, void *val);
extern int CFG_str2id(char * var);
extern int CFG_commit(int id);
#else
#define CFG_set(a, b)   {}
#define CFG_str2id(a)   {}
#define CFG_commit(a)   {}
#endif /*__ECOS && BRANCH_ADV */

UINT8 WPS_DH_G_VALUE[1] = {0x02};
UINT8 WPS_DH_P_VALUE[192] = 
{
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xC9, 0x0F, 0xDA, 0xA2, 0x21, 0x68, 0xC2, 0x34,
    0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1,
    0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74,
    0x02, 0x0B, 0xBE, 0xA6, 0x3B, 0x13, 0x9B, 0x22,
    0x51, 0x4A, 0x08, 0x79, 0x8E, 0x34, 0x04, 0xDD,
    0xEF, 0x95, 0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B,
    0x30, 0x2B, 0x0A, 0x6D, 0xF2, 0x5F, 0x14, 0x37,
    0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51, 0xC2, 0x45,
    0xE4, 0x85, 0xB5, 0x76, 0x62, 0x5E, 0x7E, 0xC6,
    0xF4, 0x4C, 0x42, 0xE9, 0xA6, 0x37, 0xED, 0x6B,
    0x0B, 0xFF, 0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED,
    0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5,
    0xAE, 0x9F, 0x24, 0x11, 0x7C, 0x4B, 0x1F, 0xE6,
    0x49, 0x28, 0x66, 0x51, 0xEC, 0xE4, 0x5B, 0x3D,
    0xC2, 0x00, 0x7C, 0xB8, 0xA1, 0x63, 0xBF, 0x05,
    0x98, 0xDA, 0x48, 0x36, 0x1C, 0x55, 0xD3, 0x9A,
    0x69, 0x16, 0x3F, 0xA8, 0xFD, 0x24, 0xCF, 0x5F,
    0x83, 0x65, 0x5D, 0x23, 0xDC, 0xA3, 0xAD, 0x96,
    0x1C, 0x62, 0xF3, 0x56, 0x20, 0x85, 0x52, 0xBB,
    0x9E, 0xD5, 0x29, 0x07, 0x70, 0x96, 0x96, 0x6D,
    0x67, 0x0C, 0x35, 0x4E, 0x4A, 0xBC, 0x98, 0x04,
    0xF1, 0x74, 0x6C, 0x08, 0xCA, 0x23, 0x73, 0x27,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

/* General used field */
#ifdef DPA_T
UCHAR	STA_Wsc_Pri_Dev_Type[8] = {0x00, 0x07, 0x00, 0x50, 0xf2, 0x04, 0x00, 0x01};
#else /* DPA_T */
UCHAR	STA_Wsc_Pri_Dev_Type[8] = {0x00, 0x01, 0x00, 0x50, 0xf2, 0x04, 0x00, 0x01};
#endif /* !DPA_T */

#ifdef CONFIG_AP_SUPPORT
UCHAR	AP_Wsc_Pri_Dev_Type[8] = {0x00, 0x06, 0x00, 0x50, 0xf2, 0x04, 0x00, 0x01};

VOID    WscDelWPARetryTimer(
    IN  PRTMP_ADAPTER pAd);

#ifdef APCLI_SUPPORT
#if 0
void    WscWriteConfToApCliCfg(
    IN  PRTMP_ADAPTER   pAd,
    IN  PWSC_CTRL       pWscControl,
    IN  PWSC_CREDENTIAL pCredential,
    IN  BOOLEAN         bEnrollee);
#endif

VOID 	WscApCliLinkDown(
	IN	PRTMP_ADAPTER	pAd,
	IN  PWSC_CTRL       pWscControl);
#endif /* APCLI_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

BOOLEAN WscCheckNonce(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	MLME_QUEUE_ELEM	*Elem,
	IN  BOOLEAN         bFlag,
	IN  PWSC_CTRL       pWscControl);

VOID    WscEapActionDisabled(
    IN  PRTMP_ADAPTER       pAdapter,
    IN  PWSC_CTRL           pWscControl);

VOID    WscGetConfigErrFromNack(
    IN  PRTMP_ADAPTER       pAdapter,
    IN	MLME_QUEUE_ELEM	    *pElem,
    OUT USHORT				*pConfigError);

INT	    WscSetAuthMode(
	IN	PRTMP_ADAPTER	pAd, 
	IN  UCHAR			CurOpMode,
	IN  UCHAR			apidx,
	IN	RTMP_STRING *arg);

INT	    WscSetEncrypType(
	IN	PRTMP_ADAPTER	pAd, 
	IN  UCHAR			CurOpMode,
	IN  UCHAR			apidx,
	IN	RTMP_STRING *arg);

VOID WscSendNACK(
	IN	PRTMP_ADAPTER	pAdapter,
	IN  MAC_TABLE_ENTRY *pEntry,
	IN  PWSC_CTRL       pWscControl);

static INT wsc_write_dat_file_thread(IN ULONG data);

#ifdef CONFIG_STA_SUPPORT
VOID WscLinkDown(
	IN	PRTMP_ADAPTER	pAd);
#endif /* CONFIG_STA_SUPPORT */

VOID	WscDelListEntryByMAC(
	PLIST_HEADER		pWscEnList,
	IN  PUCHAR			pMacAddr);

NDIS_802_11_AUTHENTICATION_MODE   WscGetAuthMode(
    IN  USHORT authFlag);

NDIS_802_11_WEP_STATUS   WscGetWepStatus(
    IN  USHORT encryFlag);

/*
	Standard UUID generation procedure. The UUID format generated by this function is base on UUID std. version 1.
	It's a 16 bytes, one-time global unique number. and can show in string format like this:
			550e8400-e29b-41d4-a716-446655440000 
			
	The format of uuid is:
		uuid                        = <time_low> "-"
		                              <time_mid> "-"
		                              <time_high_and_version> "-"
		                              <clock_seq_high_and_reserved>
	    	                          <clock_seq_low> "-"
		                              <node>
		time_low                    = 4*<hex_octet>
		time_mid                    = 2*<hex_octet>
		time_high_and_version       = 2*<hex_octet>
		clock_seq_high_and_reserved = <hex_octet>
		clock_seq_low               = <hex_octet>
		node                        = 6*<hex_octet>
		hex_octet                   = <hex_digit> <hex_digit>
		hex_digit                   = "0"|"1"|"2"|"3"|"4"|"5"|"6"|"7"|"8"|"9"
		                              |"a"|"b"|"c"|"d"|"e"|"f"
		                              |"A"|"B"|"C"|"D"|"E"|"F"
	Note:
		Actually, to IOT with JumpStart, we fix the first 10 bytes of UUID string!!!!
*/
INT WscGenerateUUID(
	RTMP_ADAPTER	*pAd, 
	UCHAR 			*uuidHexStr, 
	UCHAR 			*uuidAscStr, 
	int 			apIdx,
	BOOLEAN			bUseCurrentTime)
{
	
	WSC_UUID_T uuid_t;
	unsigned long long uuid_time;
	int i;
	UINT16 clkSeq;
	char uuidTmpStr[UUID_LEN_STR+2];	

#ifdef MULTI_INF_SUPPORT
#ifdef CON_WPS_AP_SAME_UUID
	PRTMP_ADAPTER pOpposAd;
	PWSC_CTRL pWscControl;

	/* We Assume the 5G init phase after 2.4Ghz */
	if (pAd == pAdGlobalList[1])
	{
		pOpposAd = (PRTMP_ADAPTER) pAdGlobalList[0];	
		pWscControl = &pOpposAd->ApCfg.MBSSID[apIdx].WscControl;

		NdisCopyMemory(uuidHexStr, &pWscControl->Wsc_Uuid_E[0], UUID_LEN_HEX);
		NdisCopyMemory(uuidAscStr, &pWscControl->Wsc_Uuid_Str[0], UUID_LEN_STR);
		
		goto show;		
	}
#endif /* CON_WPS_AP_SAME_UUID */
#endif /* MULTI_INF_SUPPORT */

#ifdef RTMP_RBUS_SUPPORT	
/* for fixed UUID -  YYHuang 07/10/09 */
#define FIXED_UUID
#endif /* RTMP_RBUS_SUPPORT */
	
	/* Get the current time. */
	if (bUseCurrentTime)
	{
		NdisGetSystemUpTime((ULONG *)&uuid_time);
	}
	else
		uuid_time = 2860; /*xtime.tv_sec; 	// Well, we fix this to make JumpStart  happy! */
	uuid_time *= 10000000;
	uuid_time += 0x01b21dd213814000LL;
	
#ifdef RTMP_RBUS_SUPPORT	
#ifdef FIXED_UUID
    uuid_time  = 0x2880288028802880LL;
#endif
#endif /* RTMP_RBUS_SUPPORT */

	
	uuid_t.timeLow = (UINT32)uuid_time & 0xFFFFFFFF;
	uuid_t.timeMid = (UINT16)((uuid_time >>32) & 0xFFFF);
	uuid_t.timeHi_Version = (UINT16)((uuid_time >> 48) & 0x0FFF);
	uuid_t.timeHi_Version |= (1 << 12);

	/* Get the clock sequence. */
	clkSeq = (UINT16)(0x0601/*jiffies*/ & 0xFFFF);		/* Again, we fix this to make JumpStart happy! */
#ifdef RTMP_RBUS_SUPPORT
#ifdef FIXED_UUID
	clkSeq = (UINT16)0x2880;
#endif
#endif /* RTMP_RBUS_SUPPORT */

	uuid_t.clockSeqLow = clkSeq & 0xFF;
	uuid_t.clockSeqHi_Var = (clkSeq & 0x3F00) >> 8;
	uuid_t.clockSeqHi_Var |= 0x80;

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		/* copy the Mac address as the value of node */
		NdisMoveMemory(&uuid_t.node[0], &pAd->ApCfg.MBSSID[apIdx].wdev.bssid[0], sizeof(uuid_t.node));
	}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	{
#ifdef P2P_SUPPORT
		/* copy the Mac address as the value of node */
		if (apIdx >= MIN_NET_DEVICE_FOR_P2P_GO)
			NdisMoveMemory(&uuid_t.node[0], &pAd->ApCfg.MBSSID[MAIN_MBSSID].wdev.bssid[0], sizeof(uuid_t.node));
		else
#endif /* P2P_SUPPORT */
		NdisMoveMemory(&uuid_t.node[0], &pAd->CurrentAddress[0], sizeof(uuid_t.node));
	}
#endif /* CONFIG_STA_SUPPORT */

	/* Create the UUID ASCII string. */
#if 0
	snprintf((RTMP_STRING *) uuidAscStr, UUID_LEN_STR, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", 
			(unsigned int)uuid_t.timeLow, uuid_t.timeMid, uuid_t.timeHi_Version, uuid_t.clockSeqHi_Var, uuid_t.clockSeqLow, 
			uuid_t.node[0], uuid_t.node[1], uuid_t.node[2], uuid_t.node[3], uuid_t.node[4], uuid_t.node[5]);
#else
	memset(uuidTmpStr, 0, sizeof(uuidTmpStr));
	snprintf(uuidTmpStr, sizeof(uuidTmpStr), "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", 
			(unsigned int)uuid_t.timeLow, uuid_t.timeMid, uuid_t.timeHi_Version, uuid_t.clockSeqHi_Var, uuid_t.clockSeqLow, 
			uuid_t.node[0], uuid_t.node[1], uuid_t.node[2], uuid_t.node[3], uuid_t.node[4], uuid_t.node[5]);
	if (strlen(uuidTmpStr) > UUID_LEN_STR)
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("ERROR:UUID String size too large!\n"));
	strncpy((RTMP_STRING *)uuidAscStr, uuidTmpStr, UUID_LEN_STR);
#endif

	/* Create the UUID Hex format number */
	uuid_t.timeLow = cpu2be32(uuid_t.timeLow);
	NdisMoveMemory(&uuidHexStr[0], &uuid_t.timeLow, 4);
	uuid_t.timeMid = cpu2be16(uuid_t.timeMid);
	NdisMoveMemory(&uuidHexStr[4], &uuid_t.timeMid, 2);
	uuid_t.timeHi_Version = cpu2be16(uuid_t.timeHi_Version);
	NdisMoveMemory(&uuidHexStr[6], &uuid_t.timeHi_Version, 2);
	NdisMoveMemory(&uuidHexStr[8], &uuid_t.clockSeqHi_Var, 1);
	NdisMoveMemory(&uuidHexStr[9], &uuid_t.clockSeqLow, 1);
	NdisMoveMemory(&uuidHexStr[10], &uuid_t.node[0], 6);

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("The UUID Hex string is:"));
	for (i=0; i< 16; i++)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%02x", (uuidHexStr[i] & 0xff)));
	}
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("\n"));
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("The UUID ASCII string is:%s!\n", uuidAscStr));
	return 0;
}

VOID	WscInitCommonTimers(
	IN	PRTMP_ADAPTER	pAdapter,
	IN  PWSC_CTRL		pWscControl)
{
	WSC_TIMER_INIT(pAdapter, pWscControl, &pWscControl->EapolTimer, pWscControl->EapolTimerRunning, WscEAPOLTimeOutAction);
	WSC_TIMER_INIT(pAdapter, pWscControl, &pWscControl->Wsc2MinsTimer, pWscControl->Wsc2MinsTimerRunning, Wsc2MinsTimeOutAction);
	WSC_TIMER_INIT(pAdapter, pWscControl, &pWscControl->WscUPnPNodeInfo.UPnPMsgTimer, pWscControl->WscUPnPNodeInfo.bUPnPMsgTimerRunning, WscUPnPMsgTimeOutAction);
	pWscControl->WscUPnPNodeInfo.bUPnPMsgTimerPending = FALSE;
	WSC_TIMER_INIT(pAdapter, pWscControl, &pWscControl->M2DTimer, pWscControl->bM2DTimerRunning, WscM2DTimeOutAction);

#ifdef WSC_LED_SUPPORT
	WSC_TIMER_INIT(pAdapter, pWscControl, &pWscControl->WscLEDTimer, pWscControl->WscLEDTimerRunning, WscLEDTimer);
	WSC_TIMER_INIT(pAdapter, pWscControl, &pWscControl->WscSkipTurnOffLEDTimer, pWscControl->WscSkipTurnOffLEDTimerRunning, WscSkipTurnOffLEDTimer);
#endif /* WSC_LED_SUPPORT */
}

VOID	WscInitClientTimers(
	IN	PRTMP_ADAPTER	pAdapter,
	IN  PWSC_CTRL		pWScControl)
{
	WSC_TIMER_INIT(pAdapter, pWScControl, &pWScControl->WscPBCTimer, pWScControl->WscPBCTimerRunning, WscPBCTimeOutAction);
#ifdef APCLI_SUPPORT
	WSC_TIMER_INIT(pAdapter, pWScControl, &pWScControl->WscPINTimer, pWScControl->WscPINTimerRunning, WscPINTimeOutAction);
#endif
	WSC_TIMER_INIT(pAdapter, pWScControl, &pWScControl->WscScanTimer, pWScControl->WscScanTimerRunning, WscScanTimeOutAction);
	WSC_TIMER_INIT(pAdapter, pWScControl, &pWScControl->WscProfileRetryTimer, pWScControl->WscProfileRetryTimerRunning, WscProfileRetryTimeout);  /* add by johnli, fix WPS test plan 5.1.1 */
}

/*  
	==========================================================================
	Description: 
		wps state machine init, including state transition and timer init
	Parameters: 
		S - pointer to the association state machine
	==========================================================================
 */
VOID    WscStateMachineInit(
	IN	PRTMP_ADAPTER		pAd, 
	IN	STATE_MACHINE		*S, 
	OUT	STATE_MACHINE_FUNC	Trans[])	
{
	PWSC_CTRL	pWScControl;
	StateMachineInit(S,	(STATE_MACHINE_FUNC*)Trans, MAX_WSC_STATE, MAX_WSC_MSG, (STATE_MACHINE_FUNC)Drop, WSC_IDLE, WSC_MACHINE_BASE);
	StateMachineSetAction(S, WSC_IDLE, WSC_EAPOL_START_MSG, (STATE_MACHINE_FUNC)WscEAPOLStartAction);
	StateMachineSetAction(S, WSC_IDLE, WSC_EAPOL_PACKET_MSG, (STATE_MACHINE_FUNC)WscEAPAction);
	StateMachineSetAction(S, WSC_IDLE, WSC_EAPOL_UPNP_MSG, (STATE_MACHINE_FUNC)WscEAPAction);

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		UCHAR         apidx;

		for (apidx = 0; apidx < MAX_MBSSID_NUM(pAd); apidx++)
		{
			pWScControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
			pWScControl->EntryIfIdx= (MIN_NET_DEVICE_FOR_MBSSID | apidx);
			WscInitCommonTimers(pAd, pWScControl);
			pWScControl->WscUpdatePortCfgTimerRunning = FALSE;
			WSC_TIMER_INIT(pAd, pWScControl, &pWScControl->WscUpdatePortCfgTimer, pWScControl->WscUpdatePortCfgTimerRunning, WscUpdatePortCfgTimeout);
#ifdef WSC_V2_SUPPORT
			WSC_TIMER_INIT(pAd, pWScControl, &pWScControl->WscSetupLockTimer, pWScControl->WscSetupLockTimerRunning, WscSetupLockTimeout);
#endif /* WSC_V2_SUPPORT */
		}

#ifdef APCLI_SUPPORT
		for (apidx = 0; apidx < MAX_APCLI_NUM; apidx++)
		{
			pWScControl = &pAd->ApCfg.ApCliTab[apidx].WscControl;
			pWScControl->EntryIfIdx= (MIN_NET_DEVICE_FOR_APCLI | apidx);
			WscInitCommonTimers(pAd, pWScControl);
			WscInitClientTimers(pAd, pWScControl);
		}
#endif /* APCLI_SUPPORT */
	}

#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	{
		pWScControl = &pAd->StaCfg.WscControl;
		pWScControl->EntryIfIdx = BSS0;

		WscInitCommonTimers(pAd, pWScControl);
		WscInitClientTimers(pAd, pWScControl);
		
#ifdef P2P_SUPPORT
		pWScControl = &pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl;
		pWScControl->EntryIfIdx = MIN_NET_DEVICE_FOR_P2P_GO;
		WscInitCommonTimers(pAd, pWScControl);
		pWScControl->WscUpdatePortCfgTimerRunning = FALSE;
		WSC_TIMER_INIT(pAd, pWScControl, &pWScControl->WscUpdatePortCfgTimer, pWScControl->WscUpdatePortCfgTimerRunning, WscUpdatePortCfgTimeout);

		pWScControl = &pAd->ApCfg.ApCliTab[MAIN_MBSSID].WscControl;
		pWScControl->EntryIfIdx = MIN_NET_DEVICE_FOR_P2P_CLI;
		WscInitCommonTimers(pAd, pWScControl);
		WscInitClientTimers(pAd, pWScControl);
#endif /* P2P_SUPPORT */
	}
#endif /* CONFIG_STA_SUPPORT */
}

void WscM2DTimeOutAction(
    IN PVOID SystemSpecific1, 
    IN PVOID FunctionContext, 
    IN PVOID SystemSpecific2, 
    IN PVOID SystemSpecific3)
{
	/* For each state, we didn't care about the retry issue, we just send control message
		to notify the UPnP deamon that some error happened in STATE MACHINE.
	*/
	PWSC_CTRL pWscControl = (PWSC_CTRL)FunctionContext;
	PRTMP_ADAPTER pAd = NULL;
	WSC_UPNP_NODE_INFO	*pWscNodeInfo;
#ifdef CONFIG_AP_SUPPORT
	MAC_TABLE_ENTRY		*pEntry = NULL;
/*	UCHAR		        apidx = MAIN_MBSSID; */
#endif /* CONFIG_AP_SUPPORT */    
	BOOLEAN             Cancelled;
	UCHAR				CurOpMode = 0xFF;
	
	if (pWscControl)
		pAd = (PRTMP_ADAPTER)pWscControl->pAd;
	else
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: pWscControl is NULL!!\n", __FUNCTION__));
	}
	
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		CurOpMode = AP_MODE;
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		CurOpMode = STA_MODE;
#ifdef P2P_SUPPORT
	if (pWscControl->EntryIfIdx != BSS0)
		CurOpMode = AP_MODE;
#endif /* P2P_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
		pEntry = MacTableLookup(pAd, pWscControl->EntryAddr);
#endif /* CONFIG_AP_SUPPORT */
	pWscNodeInfo = &pWscControl->WscUPnPNodeInfo;
	
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("UPnP StateMachine TimeOut(State=%d!)\n", pWscControl->WscState));

	if(
#ifdef CONFIG_AP_SUPPORT
		(((pEntry == NULL) || (pWscNodeInfo->registrarID != 0)) &&  (CurOpMode == AP_MODE)) ||
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
		((pWscNodeInfo->registrarID != 0) &&  (CurOpMode == STA_MODE)) ||
#endif /* CONFIG_STA_SUPPORT */
		(0))
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s():pEntry maybe gone or already received M2 Packet!\n", __FUNCTION__));
		goto done;
	}
	
	if (pWscControl->M2DACKBalance != 0)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s(): waiting for M2DACK balance, extend the time!\n", __FUNCTION__));
		/* Waiting for M2DACK balance. */
		RTMPModTimer(&pWscControl->M2DTimer, WSC_EAP_ID_TIME_OUT);
		pWscControl->M2DACKBalance = 0;
		goto done;
	}
	else
	{	
		RTMPCancelTimer(&pWscControl->EapolTimer, &Cancelled);
		pWscControl->EapolTimerRunning = FALSE;

#ifdef CONFIG_AP_SUPPORT
		if (CurOpMode == AP_MODE)
		{
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s(): send EAP-Fail to wireless Station!\n", __FUNCTION__));
			/* Send EAPFail to Wireless Station and reset the status of Wsc. */
			WscSendEapFail(pAd, pWscControl, TRUE);
			/*pEntry->bWscCapable = FALSE; */
			if (pEntry != NULL)
				pEntry->Receive_EapolStart_EapRspId = 0;
		}
#endif /* CONFIG_AP_SUPPORT */
		pWscControl->EapMsgRunning = FALSE;
		pWscControl->WscState = WSC_STATE_OFF;
    }
	
done:
	pWscControl->bM2DTimerRunning = FALSE;
	pWscControl->M2DACKBalance = 0;
	pWscNodeInfo->registrarID = 0;
		
	
}


VOID WscUPnPMsgTimeOutAction(
    IN PVOID SystemSpecific1, 
    IN PVOID FunctionContext, 
    IN PVOID SystemSpecific2, 
    IN PVOID SystemSpecific3)
{
	PWSC_CTRL pWscControl = (PWSC_CTRL)FunctionContext;
	PRTMP_ADAPTER pAd;
	WSC_UPNP_NODE_INFO	*pWscNodeInfo;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscUPnPMsgTimeOutAction\n"));

	/*It shouldn't happened! */
	if (!pWscControl)
		return;
	
	pAd = (PRTMP_ADAPTER)pWscControl->pAd;
	pWscNodeInfo = &pWscControl->WscUPnPNodeInfo;
	
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("UPnP StateMachine TimeOut(State=%d!)\n", pWscControl->WscState));

    if (pWscNodeInfo->bUPnPMsgTimerPending)
    {
#define WSC_UPNP_TIMER_PENDIND_WAIT	2000

        RTMPModTimer(&pWscNodeInfo->UPnPMsgTimer, WSC_UPNP_TIMER_PENDIND_WAIT);
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("UPnPMsgTimer Pending......\n"));
	} 
	else
	{
		int dataLen;
		UCHAR *pWscData;

		os_alloc_mem(NULL, (UCHAR **)&pWscData, WSC_MAX_DATA_LEN);
		if (pWscData != NULL)
		{
			memset(pWscData, 0, WSC_MAX_DATA_LEN);
			dataLen = BuildMessageNACK(pAd, pWscControl, pWscData);
			WscSendUPnPMessage(pAd, (pWscControl->EntryIfIdx & 0x0F), 
									WSC_OPCODE_UPNP_DATA, WSC_UPNP_DATA_SUB_NORMAL, 
									pWscData, dataLen, 0, 0, &pAd->CurrentAddress[0], AP_MODE);
			os_free_mem(NULL, pWscData);
		}
	
		pWscNodeInfo->bUPnPInProgress = FALSE;
		pWscNodeInfo->bUPnPMsgTimerPending = FALSE;
		pWscNodeInfo->bUPnPMsgTimerRunning = FALSE;
		pWscControl->WscState = WSC_STATE_OFF;
		pWscControl->WscStatus = STATUS_WSC_FAIL;
		
		RTMPSendWirelessEvent(pAd, IW_WSC_STATUS_FAIL, NULL, (pWscControl->EntryIfIdx & 0x0F), 0);
    }

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscUPnPMsgTimeOutAction\n"));
		
}

/*
	==========================================================================
	Description:
		This function processes EapolStart packets from wps stations
		or enqueued by self.

	Return:
		None
	==========================================================================
*/
VOID WscEAPOLStartAction(
    IN PRTMP_ADAPTER    pAd, 
    IN MLME_QUEUE_ELEM  *Elem) 
{
    MAC_TABLE_ENTRY     *pEntry;
	PWSC_CTRL			pWpsCtrl = NULL;
	PHEADER_802_11      pHeader;
	PWSC_PEER_ENTRY		pWscPeer = NULL;
	UCHAR				CurOpMode = 0xFF;
#ifdef CONFIG_AP_SUPPORT
#ifdef CON_WPS
	PWSC_CTRL pApCliWpsCtrl = NULL;
#endif /* CON_WPS */
#endif /* CONFIG_AP_SUPPORT */

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscEAPOLStartAction\n"));
	
	pHeader = (PHEADER_802_11)Elem->Msg;
	pEntry = MacTableLookup(pAd, pHeader->Addr2);

	/* Cannot find this wps station in MacTable of WPS AP. */
    if (pEntry == NULL)
    {
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("pEntry is NULL.\n"));
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscEAPOLStartAction\n"));
        return;
    }
    
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		CurOpMode = AP_MODE;
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		CurOpMode = STA_MODE;
#ifdef P2P_SUPPORT
	if (Elem->OpMode != OPMODE_STA)
		CurOpMode = AP_MODE;
#endif /* P2P_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
		pWpsCtrl = &pAd->ApCfg.MBSSID[pEntry->func_tb_idx].WscControl;
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
	{
#ifdef IWSC_SUPPORT
		if ((pAd->StaCfg.BssType == BSS_ADHOC) &&
			(IWSC_PeerEapolStart(pAd, pEntry) == FALSE))
		{
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Rejected by IWSC SM. Ignore EAPOL-Start.\n"));
			return;
		}
#endif // IWSC_SUPPORT //
		pWpsCtrl = &pAd->StaCfg.WscControl;
	}
#endif /* CONFIG_STA_SUPPORT */

	if (pWpsCtrl == NULL)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: pWpsCtrl == NULL!\n", __FUNCTION__));
		return;
	}

#ifdef CONFIG_AP_SUPPORT
#ifdef CON_WPS
	pApCliWpsCtrl = &pAd->ApCfg.ApCliTab[BSS0].WscControl;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("CON_WPS: Stop the ApCli WPS, state [%d]\n", pApCliWpsCtrl->WscState));

	if ((pAd->conWscStatus != CON_WPS_STATUS_DISABLED) && 
		(pApCliWpsCtrl->WscState != WSC_STATE_OFF))
	{
		WscStop(pAd, TRUE, pApCliWpsCtrl);
		pApCliWpsCtrl->WscConfMode = WSC_DISABLE;
        /* APCLI: For stop the other side of the band with WSC SM */
        WscConWpsStop(pAd, TRUE, pApCliWpsCtrl);

	}
#endif /* CON_WPS */
#endif /* CONFIG_AP_SUPPORT */

	RTMP_SEM_LOCK(&pWpsCtrl->WscPeerListSemLock);
	WscInsertPeerEntryByMAC(&pWpsCtrl->WscPeerList, pEntry->Addr);	
	RTMP_SEM_UNLOCK(&pWpsCtrl->WscPeerListSemLock);
	
	WscMaintainPeerList(pAd, pWpsCtrl);

	/*
		Check this STA is first one or not
	*/
	if (pWpsCtrl->WscPeerList.size != 0)
	{
		pWscPeer = (PWSC_PEER_ENTRY)pWpsCtrl->WscPeerList.pHead;
		if (NdisEqualMemory(pEntry->Addr, pWscPeer->mac_addr, MAC_ADDR_LEN) == FALSE)
		{
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("This is not first WSC peer, ignore this EAPOL_Start!\n"));
			hex_dump("pEntry->Addr", pEntry->Addr, MAC_ADDR_LEN);
#ifdef CONFIG_AP_SUPPORT
			if (CurOpMode == AP_MODE)
				WscApShowPeerList(pAd, NULL);
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
			if (CurOpMode == STA_MODE)
				WscStaShowPeerList(pAd, NULL);
#endif /* CONFIG_STA_SUPPORT */
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscEAPOLStartAction\n"));
			return;
		}
	}
	
#ifdef P2P_SUPPORT
	if (P2P_GO_ON(pAd) && (pWpsCtrl->bWscTrigger == FALSE))
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Ignore this EAPOL_Start!\n"));
		return;
	}
#endif /* P2P_SUPPORT */

	
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscState = %d\n", pWpsCtrl->WscState));
    if ((pEntry->Receive_EapolStart_EapRspId == 0) ||
		(pWpsCtrl->WscState <= WSC_STATE_WAIT_REQ_ID))
    {
    	/* Receive the first EapolStart packet of this wps station. */
        pEntry->Receive_EapolStart_EapRspId |= WSC_ENTRY_GET_EAPOL_START;
		
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEAPOLStartAction - receive EAPOL-Start from %02x:%02x:%02x:%02x:%02x:%02x\n",
                                 PRINT_MAC(pEntry->Addr)));
			
		/* EapolStart packet is sent by station means this station wants to do wps process with AP. */
		pWpsCtrl->EapMsgRunning = TRUE;
		/* Update EntryAddr again */
		NdisMoveMemory(pWpsCtrl->EntryAddr, pEntry->Addr, MAC_ADDR_LEN);

		if (pEntry->bWscCapable == FALSE)
			pEntry->bWscCapable = TRUE;
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEAPOLStartAction(ra%d) - send EAP-Req(Id) to %02x:%02x:%02x:%02x:%02x:%02x\n",
					pEntry->func_tb_idx, PRINT_MAC(pEntry->Addr)));
		
#ifdef MT_MAC
		if (pEntry->bEapReqIdRetryTimerRunning == FALSE)
		{
			RTMPSetTimer(&pEntry->EapReqIdRetryTimer, WSC_EAP_EAP_FAIL_TIME_OUT);
			pEntry->bEapReqIdRetryTimerRunning = TRUE;
		}
#endif /* MT_MAC */
		
		/* Send EAP-Request/Id to station */
        WscSendEapReqId(pAd, pEntry, CurOpMode);
        if (!pWpsCtrl->EapolTimerRunning)
        {
            pWpsCtrl->EapolTimerRunning = TRUE;
			/* Set WPS_EAP Messages timeout function. */
            RTMPSetTimer(&pWpsCtrl->EapolTimer, WSC_EAP_ID_TIME_OUT);
        }
    } 
    else
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Ignore EAPOL-Start.\n"));
    
    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscEAPOLStartAction\n"));
}


/*
	==========================================================================
	Description:
		This is state machine function when receiving EAP packets 
		which is WPS Registration Protocol.

		There are two roles at our AP, as an 
		1. Enrollee		
		2. Internal Registrar
		3. Proxy

		There are two roles at our Station, as an 
		1. Enrollee		
		2. External Registrar

		Running Scenarios:
		-----------------------------------------------------------------
		1a. Adding an AP as an Enrollee to a station as an External Registrar (EAP)
			[External Registrar]<----EAP--->[Enrollee_AP]
		-----------------------------------------------------------------
		2a. Adding a station as an Enrollee to an AP with built-in Registrar (EAP)	
			[Registrar_AP]<----EAP--->[Enrollee_STA]
		-----------------------------------------------------------------
		3a. Adding an Enrollee with External Registrar (UPnP/EAP)	
			[External Registrar]<----UPnP--->[Proxy_AP]<---EAP--->[Enrollee_STA]  
		-----------------------------------------------------------------

	Return:
		None
	==========================================================================
*/
VOID WscEAPAction(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	MLME_QUEUE_ELEM	*Elem) 
{		
	UCHAR		MsgType;
	BOOLEAN		bUPnPMsg, Cancelled;
	MAC_TABLE_ENTRY	*pEntry = NULL;
	UCHAR		MacAddr[MAC_ADDR_LEN] = {0};
#ifdef CONFIG_AP_SUPPORT
	UCHAR		apidx = MAIN_MBSSID;
#endif /* CONFIG_AP_SUPPORT */
	PWSC_CTRL				pWscControl = NULL;
	PWSC_UPNP_NODE_INFO	pWscUPnPNodeInfo = NULL;
	UCHAR		CurOpMode = 0xFF;
	
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscEAPAction\n"));

	/* The first 6 bytes in Elem->Msg is the MAC address of wps peer. */
	memmove(MacAddr, Elem->Msg, MAC_ADDR_LEN);
	memmove(Elem->Msg, Elem->Msg+6, Elem->MsgLen);

#ifdef DBG
    hex_dump("(WscEAPAction)Elem->MsgLen", Elem->Msg, Elem->MsgLen);
#endif /* DBG */

	MsgType = WscRxMsgType(pAdapter, Elem);
	bUPnPMsg = Elem->MsgType == WSC_EAPOL_UPNP_MSG ? TRUE : FALSE;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEAPAction: Addr: %02x:%02x:%02x:%02x:%02x:%02x, MsgType: 0x%02X, bUPnPMsg: %s\n",
				PRINT_MAC(MacAddr), MsgType, bUPnPMsg ? "TRUE" : "FALSE"));

	if (!bUPnPMsg)
		pEntry = MacTableLookup(pAdapter, MacAddr);
			
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAdapter)
		CurOpMode = AP_MODE;
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAdapter)
		CurOpMode = STA_MODE;
#ifdef P2P_SUPPORT
	if (Elem->OpMode != OPMODE_STA)
		CurOpMode = AP_MODE;
#endif /* P2P_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */
			
#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
	{
		if (!bUPnPMsg)
		{
			if (pEntry)
			{
				if (IS_ENTRY_CLIENT(pEntry) && pEntry->func_tb_idx >= pAdapter->ApCfg.BssidNum)
				{
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEAPAction: Unknow apidex(=%d).\n", pEntry->func_tb_idx));
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscEAPAction\n"));
					return;
				}
				else
				{
					apidx = pEntry->func_tb_idx;
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEAPAction: apidex=%d.\n", pEntry->func_tb_idx));
				}
			}
			else
			{
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEAPAction: pEntry is NULL.\n"));
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscEAPAction\n"));
				return;
			}
#ifdef APCLI_SUPPORT
			/* for ap-client packets */
			if (pEntry && IS_ENTRY_APCLI(pEntry))
				pWscControl = &pAdapter->ApCfg.ApCliTab[apidx].WscControl;
			else
#endif /* APCLI_SUPPORT */
				pWscControl = &pAdapter->ApCfg.MBSSID[apidx].WscControl;
		}
		else
		{
			int i;
			for (i = 0 ; i < MAX_MBSSID_NUM(pAdapter); i++)
			{
				if(NdisEqualMemory(pAdapter->ApCfg.MBSSID[i].wdev.bssid, MacAddr, MAC_ADDR_LEN))
				{
					apidx = i;
					break;
				}
			}
			pWscControl = &pAdapter->ApCfg.MBSSID[apidx].WscControl;
			pWscUPnPNodeInfo = &pAdapter->ApCfg.MBSSID[apidx].WscControl.WscUPnPNodeInfo;
			pWscUPnPNodeInfo->bUPnPMsgTimerPending = TRUE;
		}
	}
#endif /* CONFIG_AP_SUPPORT */
	
#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
	{
		pWscControl = &pAdapter->StaCfg.WscControl;
		pWscUPnPNodeInfo = &pWscControl->WscUPnPNodeInfo;
		pWscUPnPNodeInfo->bUPnPMsgTimerPending = TRUE;
	}
#endif /* CONFIG_STA_SUPPORT */

	if (pWscControl == NULL)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: pWscControl == NULL!\n", __FUNCTION__));
		return;
	}
	
	if (pEntry && IS_ENTRY_CLIENT(pEntry))
	{
		if ((MsgType == WSC_MSG_EAP_REG_RSP_ID) || (MsgType == WSC_MSG_EAP_ENR_RSP_ID))
		{
#if 0
			if ((pEntry->Receive_EapolStart_EapRspId & WSC_ENTRY_GET_EAP_RSP_ID) == WSC_ENTRY_GET_EAP_RSP_ID)
#endif
			if (((pEntry->Receive_EapolStart_EapRspId & WSC_ENTRY_GET_EAP_RSP_ID) == WSC_ENTRY_GET_EAP_RSP_ID)
				&& (pWscControl->WscState > WSC_STATE_WAIT_M1))
			{
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEAPAction: Already receive EAP_RSP(Identitry) from this STA, ignore it.\n"));
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscEAPAction\n"));
				return;
			}
			else
				pEntry->Receive_EapolStart_EapRspId |= WSC_ENTRY_GET_EAP_RSP_ID;
		}
	}
	
	pWscControl->EapolTimerPending = TRUE;
	
#ifdef WSC_V2_SUPPORT	
	if (MsgType == WSC_MSG_EAP_FRAG_ACK)
	{
		WscSendEapFragData(pAdapter, pWscControl, pEntry);
		return;
	}
	else
#endif /* WSC_V2_SUPPORT */
	if (MsgType == WSC_MSG_EAP_REG_RSP_ID)
	{
#ifdef DPA_S
		if (pWscControl->EntryIfIdx >= MIN_NET_DEVICE_FOR_P2P_CLI)
			RtmpOSWrielessEventSend(pAdapter->p2p_dev, RT_WLAN_EVENT_CUSTOM, P2P_NOTIF_WPS_START, NULL, NULL, 0);
#endif /* DPA_S */
		/* Receive EAP-Response/Id from external registrar, so the role of AP is enrollee. */
		if (((pWscControl->WscConfMode & WSC_ENROLLEE) != 0) ||
			(((pWscControl->WscConfMode & WSC_PROXY) != 0) && bUPnPMsg))
		{
			pWscControl->WscActionMode= WSC_ENROLLEE;
			pWscControl->WscUseUPnP = bUPnPMsg ? 1 : 0;
			MsgType = WSC_MSG_EAP_RSP_ID;
			WscEapEnrolleeAction(pAdapter, Elem, WSC_MSG_EAP_RSP_ID, pEntry, pWscControl);
		}
	}
	else if (MsgType == WSC_MSG_EAP_ENR_RSP_ID)
	{
#ifdef DPA_S
		if (pWscControl->EntryIfIdx >= MIN_NET_DEVICE_FOR_P2P_CLI)
			RtmpOSWrielessEventSend(pAdapter->p2p_dev, RT_WLAN_EVENT_CUSTOM, P2P_NOTIF_WPS_START, NULL, NULL, 0);
#endif /* DPA_S */
		/* Receive EAP-Response/Id from wps enrollee station, so the role of AP is Registrar or Proxy. */
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEAPAction: Rx Identity\n"));
#ifdef MT_MAC
		/*
			Sometimes TXS would delay and received after M1.
			Cancel EapReqIdRetryTimer here if timer is still running.
		*/
		if (pEntry->bEapReqIdRetryTimerRunning)
		{
			BOOLEAN bCancelled;
			RTMPCancelTimer(&pEntry->EapReqIdRetryTimer, &bCancelled);
			pEntry->bEapReqIdRetryTimerRunning = FALSE;
		}
#endif /* MT_MAC */
		
		pWscControl->WscActionMode = WSC_REGISTRAR;
		if (bUPnPMsg)
		{
			/* Receive enrollee identity from UPnP */
		}
		else
		{
#ifdef CONFIG_AP_SUPPORT
			/* Receive enrollee identity from EAP */
			if ((pWscControl->WscMode == WSC_PBC_MODE)
#ifdef P2P_SUPPORT
				/*
					P2P doesn't need to check PBC overlapping.
				*/
				&& (pWscControl->EntryIfIdx < MIN_NET_DEVICE_FOR_P2P_GO)
#endif /* P2P_SUPPORT */
				)
			{
				/*
					Some WPS PBC Station select AP from UI directly; doesn't do PBC scan.
					Need to check DPID from STA again here.
				*/
				WscPBC_DPID_FromSTA(pAdapter, pWscControl->EntryAddr);
				WscPBCSessionOverlapCheck(pAdapter);
				if ((pAdapter->CommonCfg.WscStaPbcProbeInfo.WscPBCStaProbeCount == 1) &&
					!NdisEqualMemory(pAdapter->CommonCfg.WscStaPbcProbeInfo.StaMacAddr[0], &ZERO_MAC_ADDR[0], MAC_ADDR_LEN) &&
					(NdisEqualMemory(pAdapter->CommonCfg.WscStaPbcProbeInfo.StaMacAddr[0], &pWscControl->EntryAddr[0], 6) == FALSE))
				{
					pAdapter->CommonCfg.WscPBCOverlap = TRUE;
				}
				if (pAdapter->CommonCfg.WscPBCOverlap)
				{
					hex_dump("EntryAddr", pWscControl->EntryAddr, 6);
					hex_dump("StaMacAddr0", pAdapter->CommonCfg.WscStaPbcProbeInfo.StaMacAddr[0], 6);
					hex_dump("StaMacAddr1", pAdapter->CommonCfg.WscStaPbcProbeInfo.StaMacAddr[1], 6);
					hex_dump("StaMacAddr2", pAdapter->CommonCfg.WscStaPbcProbeInfo.StaMacAddr[2], 6);
					hex_dump("StaMacAddr3", pAdapter->CommonCfg.WscStaPbcProbeInfo.StaMacAddr[3], 6);
				}
			}
			
			if ((pWscControl->WscMode == WSC_PBC_MODE) &&
				(pAdapter->CommonCfg.WscPBCOverlap == TRUE))
			{
				/* PBC session overlap */
				pWscControl->WscStatus = STATUS_WSC_PBC_SESSION_OVERLAP;
#ifdef DPA_S
#ifdef P2P_SUPPORT
				if (pWscControl->EntryIfIdx >= MIN_NET_DEVICE_FOR_P2P_CLI)
					RtmpOSWrielessEventSend(pAdapter->p2p_dev, RT_WLAN_EVENT_CUSTOM, P2P_NOTIF_WPS_SESSION_OVERLAP, NULL, NULL, 0);
				else
#endif /* P2P_SUPPORT */
#endif /* DPA_S */
				RTMPSendWirelessEvent(pAdapter, IW_WSC_PBC_SESSION_OVERLAP, NULL, (pWscControl->EntryIfIdx & 0x0F), 0); 
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEAPAction: PBC Session Overlap!\n"));
			}
			else 
#endif /* CONFIG_AP_SUPPORT */
			if ((pWscControl->WscConfMode & WSC_PROXY_REGISTRAR) != 0)
			{
				/* Notify UPnP daemon before send Eap-Req(wsc-start) */
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: pEntry->Addr=%02x:%02x:%02x:%02x:%02x:%02x\n", 
							__FUNCTION__, PRINT_MAC(pEntry->Addr)));
#ifdef CONFIG_AP_SUPPORT
				if (CurOpMode == AP_MODE)
				{
				WscSendUPnPConfReqMsg(pAdapter, (pWscControl->EntryIfIdx & 0x0F), 
											(PUCHAR)pAdapter->ApCfg.MBSSID[pEntry->func_tb_idx].Ssid, pEntry->Addr, 2, 0, CurOpMode);
				/* Reset the UPnP timer and status. */
				if (pWscControl->bM2DTimerRunning == TRUE)
				{
					RTMPCancelTimer(&pWscControl->M2DTimer, &Cancelled);
					pWscControl->bM2DTimerRunning = FALSE;
				}
				pWscControl->WscUPnPNodeInfo.registrarID = 0;
				pWscControl->M2DACKBalance = 0;
				WscDelWPARetryTimer(pAdapter);
				}
#endif /* CONFIG_AP_SUPPORT */
				pWscControl->EapMsgRunning = TRUE;
				/* Change the state to next one */
				pWscControl->WscState = WSC_STATE_WAIT_M1;
				/* send EAP WSC_START */
				if (pEntry && IS_ENTRY_CLIENT(pEntry))
				{
					pWscControl->bWscLastOne = TRUE;
					if (CurOpMode == AP_MODE)
						WscSendMessage(pAdapter, WSC_OPCODE_START, NULL, 0, pWscControl, AP_MODE, EAP_CODE_REQ);
					else
					{						
						if (ADHOC_ON(pAdapter) && (pWscControl->WscConfMode == WSC_REGISTRAR))
							WscSendMessage(pAdapter, WSC_OPCODE_START, NULL, 0, pWscControl, STA_MODE, EAP_CODE_REQ);
						else
							WscSendMessage(pAdapter, WSC_OPCODE_START, NULL, 0, pWscControl, STA_MODE, EAP_CODE_RSP);
					}
				}
			}
		}
	}
	else if (MsgType == WSC_MSG_EAP_REQ_ID)
	{
		/* Receive EAP_Req/Identity from WPS AP or WCN */
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Receive EAP_Req/Identity from WPS AP or WCN\n"));
		if (bUPnPMsg && (pWscControl->WscConfMode == WSC_ENROLLEE))
		{
			pWscControl->WscActionMode = WSC_ENROLLEE;
			pWscControl->WscUseUPnP = 1;
			WscEapEnrolleeAction(pAdapter, Elem, WSC_MSG_EAP_REQ_START, pEntry, pWscControl);
		}
		else
		{
			/* Receive EAP_Req/Identity from WPS AP */
			if (pEntry != NULL)
				WscSendEapRspId(pAdapter, pEntry, pWscControl); 
		}
        
		if (!bUPnPMsg)
		{
			if ((pWscControl->WscState < WSC_STATE_WAIT_M1) ||
				(pWscControl->WscState > WSC_STATE_WAIT_ACK))
			{
				if (pWscControl->WscConfMode == WSC_REGISTRAR)
					pWscControl->WscState = WSC_STATE_WAIT_M1;
				else
					pWscControl->WscState = WSC_STATE_WAIT_WSC_START;
			}
		}
	}
	else if (MsgType == WSC_MSG_EAP_REQ_START)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Receive EAP_Req(Wsc_Start) from WPS AP\n"));

		/* Receive EAP_Req(Wsc_Start) from WPS AP */
		if (pWscControl->WscConfMode == WSC_ENROLLEE)
		{
			pWscControl->WscActionMode = WSC_ENROLLEE;
			pWscControl->WscUseUPnP = bUPnPMsg ? 1 : 0;
			WscEapEnrolleeAction(pAdapter, Elem, WSC_MSG_EAP_REQ_START, pEntry, pWscControl);

			if (!pWscControl->EapolTimerRunning)
			{
				pWscControl->EapolTimerRunning = TRUE;
				RTMPSetTimer(&pWscControl->EapolTimer, WSC_EAP_ID_TIME_OUT);
			}
		}
		else
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Ignore EAP_Req(Wsc_Start) from WPS AP\n"));
	}
	else if (MsgType == WSC_MSG_EAP_FAIL)
	{
		/* Receive EAP_Fail from WPS AP */
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Receive EAP_Fail from WPS AP\n"));

		if (pWscControl->WscState >= WSC_STATE_WAIT_EAPFAIL)
		{
#ifdef WIDI_SUPPORT
#ifdef CONFIG_STA_SUPPORT
			int sendFail = 0;
			if (pAdapter->StaCfg.bWIDI && !pAdapter->StaCfg.WscControl.bWscTrigger)
			{
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s : WscStatus=%d \n", __FUNCTION__, pWscControl->WscStatus));
				if (pWscControl->WscStatus != STATUS_WSC_CONFIGURED)
					sendFail = 1;
			}
#endif /* CONFIG_STA_SUPPORT */
#endif /* WIDI_SUPPORT */			
			pWscControl->WscState = WSC_STATE_OFF;
#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
			if (CurOpMode == AP_MODE)
			{
				pWscControl->WscConfMode = WSC_DISABLE;
#ifdef CON_WPS
				/* ApCli: WPS Done notify the other side of band to stop */
				if (pAdapter->conWscStatus != CON_WPS_STATUS_DISABLED)
				{
					WscConWpsStop(pAdapter, TRUE, pWscControl);
					pAdapter->conWscStatus = CON_WPS_STATUS_DISABLED;
				}
#endif /* CON_WPS */
#ifdef P2P_SUPPORT
				if (pWscControl->WscStatus == STATUS_WSC_CONFIGURED)
					pAdapter->P2pCfg.WscState = WSC_STATE_CONFIGURED;
#ifdef RT_P2P_SPECIFIC_WIRELESS_EVENT
				P2pSendWirelessEvent(pAdapter, RT_P2P_WPS_COMPLETED, NULL, NULL);
#endif /* RT_P2P_SPECIFIC_WIRELESS_EVENT */
#endif /* P2P_SUPPORT */
				/* Bring apcli interface down first */
				if(pEntry && IS_ENTRY_APCLI(pEntry) && pAdapter->ApCfg.ApCliTab[BSS0].Enable == TRUE )
				{
#ifdef P2P_SUPPORT
					UCHAR P2pIdx = P2pGroupTabSearch(pAdapter, pEntry->Addr);
#endif /* P2P_SUPPORT */
					pAdapter->ApCfg.ApCliTab[pEntry->func_tb_idx].Enable = FALSE;
					ApCliIfDown(pAdapter);
#ifdef P2P_SUPPORT
					if ((P2pIdx != P2P_NOT_FOUND) 
						&& P2P_CLI_ON(pAdapter)
						&& ((pWscControl->WscStatus == STATUS_WSC_ERROR_DEV_PWD_AUTH_FAIL) || (pWscControl->WscStatus == STATUS_WSC_FAIL)))
					{
						pAdapter->P2pTable.Client[P2pIdx].P2pClientState = P2PSTATE_DISCOVERY;
						P2pLinkDown(pAdapter, P2P_CONNECT_FAIL);
					}
#endif /* P2P_SUPPORT */
					pAdapter->ApCfg.ApCliTab[pEntry->func_tb_idx].Enable = TRUE;
				}
			}
#endif /* APCLI_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT            
			if (CurOpMode == STA_MODE)
			{
#ifdef IWSC_SUPPORT
			if ((pAdapter->OpMode == OPMODE_STA) &&
				(pAdapter->StaCfg.BssType == BSS_ADHOC) &&
				(pAdapter->StaCfg.WscControl.WscConfMode == WSC_ENROLLEE))
				pAdapter->StaCfg.IWscInfo.bReStart = TRUE;
#endif /* IWSC_SUPPORT */
				pWscControl->WscConfMode = WSC_DISABLE;
				WscLinkDown(pAdapter);
#ifdef WIDI_SUPPORT
				if (pAdapter->StaCfg.bWIDI && !pAdapter->StaCfg.WscControl.bWscTrigger)
				{					
					if (sendFail)
					{
						MLME_SCAN_REQ_STRUCT	ScanReq;

						WidiUpdateStateToDaemon(pAdapter, MIN_NET_DEVICE_FOR_MBSSID, WIDI_MSG_TYPE_ASSOC_STATUS, 
												pAdapter->CommonCfg.Bssid, NULL, 0, WIDI_WPS_STATUS_FAIL);

						ScanParmFill(pAdapter, &ScanReq, "", 0, BSS_ANY, SCAN_PASSIVE);
						MlmeEnqueue(pAdapter, SYNC_STATE_MACHINE, MT2_MLME_SCAN_REQ, 
									sizeof(MLME_SCAN_REQ_STRUCT), &ScanReq, 0);
						pAdapter->Mlme.CntlMachine.CurrState = CNTL_WAIT_OID_LIST_SCAN;
					}
					else
						WidiUpdateStateToDaemon(pAdapter, MIN_NET_DEVICE_FOR_MBSSID, WIDI_MSG_TYPE_ASSOC_STATUS, 
												pAdapter->CommonCfg.Bssid, NULL, 0, WIDI_WPS_STATUS_SUCCESS);					
				}
#endif /* WIDI_SUPPORT */				
			}
#endif /* CONFIG_STA_SUPPORT */
		}
		else if (pWscControl->WscState == WSC_STATE_RX_M2D)
		{
			/* Wait M2; */
#ifdef IWSC_SUPPORT
			/*
				We need to send EAPOL_Start again to trigger WPS process
			*/
			if (pAdapter->StaCfg.BssType == BSS_ADHOC)
			{
				pWscControl->WscState = WSC_STATE_LINK_UP;
				pWscControl->WscStatus = STATUS_WSC_LINK_UP;
				WscSendEapolStart(pAdapter, pWscControl->WscPeerMAC, STA_MODE);
			}
#endif /* IWSC_SUPPORT */
		}
		else if ((pWscControl->WscState <= WSC_STATE_WAIT_REQ_ID) && 
				 (pWscControl->WscState != WSC_STATE_FAIL))
		{
			/* Ignore. D-Link DIR-628 AP sometimes would send EAP_Fail to station after Link UP first then send EAP_Req/Identity. */
		}
		else
		{
			pWscControl->WscStatus = STATUS_WSC_FAIL;	
			RTMPSendWirelessEvent(pAdapter, IW_WSC_STATUS_FAIL, NULL, (pWscControl->EntryIfIdx & 0x0F), 0);

#ifdef IWSC_SUPPORT
			if ((pAdapter->OpMode == OPMODE_STA) &&
				(pAdapter->StaCfg.BssType == BSS_ADHOC) &&
				(pAdapter->StaCfg.WscControl.WscConfMode == WSC_ENROLLEE))
				pAdapter->StaCfg.IWscInfo.bReStart = TRUE;
#endif /* IWSC_SUPPORT */
#ifdef APCLI_SUPPORT
            if((pWscControl->WscMode == WSC_PIN_MODE) && (pWscControl->EntryIfIdx & MIN_NET_DEVICE_FOR_APCLI))
             ;
            else   
#endif                
            {    

			pWscControl->WscConfMode = WSC_DISABLE;
			/* Change the state to next one */
			pWscControl->WscState = WSC_STATE_OFF;
			}			
#ifdef CONFIG_STA_SUPPORT
			if (CurOpMode == STA_MODE)
			{
				WscLinkDown(pAdapter);
#ifdef WIDI_SUPPORT
				WidiUpdateStateToDaemon(pAdapter, MIN_NET_DEVICE_FOR_MBSSID, WIDI_MSG_TYPE_ASSOC_STATUS, 
										pAdapter->CommonCfg.Bssid, NULL, 0, WIDI_WPS_STATUS_FAIL);
#endif /* WIDI_SUPPORT */
				
			}
#endif /* CONFIG_STA_SUPPORT */
		}
	}
	else if (MsgType == WSC_MSG_M1)
	{
#ifdef CONFIG_STA_SUPPORT
		UINT32 rv = 0;
#endif /* CONFIG_STA_SUPPORT */
#if 1
		/*
			If Buffalo WPS STA doesn't receive M2D from AP, Buffalo WPS STA will stop to do WPS.
			Therefore we need to receive M1 and send M2D without trigger.
		*/
		if ((pWscControl->WscConfMode & WSC_REGISTRAR) != 0)
#else
		if ((pWscControl->WscConfMode & WSC_REGISTRAR) != 0 &&
			((pWscControl->bWscTrigger && (pWscControl->bWscAutoTigeer == FALSE))
#ifdef EASY_CONFIG_SETUP
			|| (pEntry && pEntry->bRaAutoWpsCapable)
#endif /* EASY_CONFIG_SETUP */
			))
#endif
		{
			pWscControl->WscActionMode = WSC_REGISTRAR;
			/* If Message is from EAP, but UPnP Registrar is in progress now, ignore EAP_M1 */
			if (!bUPnPMsg && pWscControl->WscUPnPNodeInfo.bUPnPInProgress)
			{
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("UPnP Registrar is working now, ignore EAP M1.\n"));
				goto out;
			}
			else
				WscEapRegistrarAction(pAdapter, Elem, MsgType, pEntry, pWscControl);
#ifdef CONFIG_STA_SUPPORT
			rv = 1;
#endif /* CONFIG_STA_SUPPORT */
		}
#ifdef CONFIG_AP_SUPPORT
		if (((pWscControl->WscConfMode & WSC_PROXY) != 0) && (!bUPnPMsg) && (CurOpMode == AP_MODE))
		{
			if ((pWscControl->bWscTrigger
#ifdef EASY_CONFIG_SETUP
			 	 || (pEntry && pEntry->bRaAutoWpsCapable)
#endif /* EASY_CONFIG_SETUP */
				 )
				&& (pWscControl->WscState >= WSC_STATE_WAIT_M3))
				;
			else
			{
				pWscControl->WscActionMode = WSC_PROXY;
				WscEapApProxyAction(pAdapter, Elem, MsgType, pEntry, pWscControl);
			}
		}
		else if ((!pWscControl->bWscTrigger) && ((pWscControl->WscConfMode & WSC_PROXY) == 0) && (pAdapter->OpMode == OPMODE_AP))
		{
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscTrigger is FALSE, ignore EAP M1.\n"));
			goto out;
		}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
		else
		{
			if ((rv == 0) && (CurOpMode == STA_MODE))
			{
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("(Line:%d)Ignore EAP M1.\n", __LINE__));
				goto out;
			}
		}
#endif /* CONFIG_STA_SUPPORT */
	}
	else if (MsgType == WSC_MSG_M3 ||
             MsgType == WSC_MSG_M5 ||
             MsgType == WSC_MSG_M7 ||
             MsgType == WSC_MSG_WSC_DONE)
	{
        BOOLEAN bNonceMatch = WscCheckNonce(pAdapter, Elem, TRUE, pWscControl);
		if (((pWscControl->WscConfMode & WSC_REGISTRAR) != 0) &&
			(pWscControl->bWscTrigger 
#ifdef EASY_CONFIG_SETUP
			 || (pEntry && pEntry->bRaAutoWpsCapable)
#endif /* EASY_CONFIG_SETUP */
			 ) &&
              bNonceMatch)
		{
			/* If Message is from EAP, but UPnP Registrar is in progress now, ignore EAP Messages */
			if (!bUPnPMsg && pWscControl->WscUPnPNodeInfo.bUPnPInProgress)
			{
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("UPnP Registrar is working now, ignore EAP Messages.\n"));
				goto out;
			}
			else
			{
				pWscControl->WscActionMode = WSC_REGISTRAR;
				WscEapRegistrarAction(pAdapter, Elem, MsgType, pEntry, pWscControl);
			}
		}
#ifdef CONFIG_AP_SUPPORT        
		else if (((pWscControl->WscConfMode & WSC_PROXY) != 0) && (!bUPnPMsg) && (CurOpMode == AP_MODE))
		{
			pWscControl->WscActionMode = WSC_PROXY;
			WscEapApProxyAction(pAdapter, Elem, MsgType, pEntry, pWscControl);
		}
#endif /* CONFIG_AP_SUPPORT */        
	}
	else if (MsgType == WSC_MSG_M2 ||
			MsgType == WSC_MSG_M2D ||
			MsgType == WSC_MSG_M4 ||
			MsgType == WSC_MSG_M6 ||
			MsgType == WSC_MSG_M8)
	{
        BOOLEAN bNonceMatch = WscCheckNonce(pAdapter, Elem, FALSE, pWscControl);
		BOOLEAN bGoWPS = FALSE;

		if ((CurOpMode == AP_MODE) ||
			((CurOpMode == STA_MODE) && 
			 (pWscControl->bWscTrigger
#ifdef CONFIG_STA_SUPPORT
#ifdef EASY_CONFIG_SETUP
			  || pAdapter->StaCfg.EasyConfigInfo.bDoAutoWps
#endif /* EASY_CONFIG_SETUP */
#endif /* CONFIG_STA_SUPPORT */
			  )))
			bGoWPS = TRUE;
		
#ifdef CONFIG_AP_SUPPORT
#ifdef WSC_V2_SUPPORT
		if ((CurOpMode == AP_MODE) &&
			((pWscControl->WscV2Info.bWpsEnable == FALSE) && (pWscControl->WscV2Info.bEnableWpsV2)))
			bGoWPS = FALSE;
#endif /* WSC_V2_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
		
		if (((pWscControl->WscConfMode & WSC_ENROLLEE) != 0) &&              
			bGoWPS &&
			bNonceMatch)		
		{
			pWscControl->WscActionMode = WSC_ENROLLEE;
			pWscControl->WscUseUPnP = bUPnPMsg ? 1 : 0;
			if (MsgType == WSC_MSG_M2)
			{
				BOOLEAN	bReadOwnPIN = TRUE;
#ifdef CONFIG_AP_SUPPORT
				/* WPS Enrollee AP only supports PIN without trigger */
				if (CurOpMode == AP_MODE)
				{
					if (pWscControl->bWscTrigger == FALSE)
				{
					pWscControl->WscMode = 1;
					WscGetConfWithoutTrigger(pAdapter, pWscControl, FALSE);
				}
					else
					{
						WscBuildBeaconIE(pAdapter, 
										pWscControl->WscConfStatus, 
										TRUE, 
										pWscControl->WscMode, 
										pWscControl->WscConfigMethods, 
										(pWscControl->EntryIfIdx & 0x0F), 
										NULL, 
										0, 
										AP_MODE);
						WscBuildProbeRespIE(pAdapter, 
										WSC_MSGTYPE_AP_WLAN_MGR, 
										pWscControl->WscConfStatus, 
										TRUE, 
										pWscControl->WscMode, 
										pWscControl->WscConfigMethods, 
										pWscControl->EntryIfIdx, 
										NULL, 
										0, 
										AP_MODE);
						APUpdateBeaconFrame(pAdapter, pWscControl->EntryIfIdx & 0x0F);
					}
				}
#endif /* CONFIG_AP_SUPPORT */

#ifdef P2P_SUPPORT
				if (P2P_CLI_ON(pAdapter) && (pWscControl->EntryIfIdx != BSS0))
				{
					UCHAR	P2pIdx = P2P_NOT_FOUND;
					P2pIdx = P2pGroupTabSearch(pAdapter, MacAddr);
					if (P2pIdx != P2P_NOT_FOUND)
					{
						PRT_P2P_CLIENT_ENTRY pP2pEntry = &pAdapter->P2pTable.Client[P2pIdx];

#if 0
						if (pP2pEntry && ((pP2pEntry->ConfigMethod & WSC_CONFMET_KEYPAD) == 0))						
#else
						if (pP2pEntry && (pAdapter->P2pCfg.ConfigMethod == WSC_CONFMET_KEYPAD))
#endif
						{
							/*
								I am KeyPad. We cannot use ConfigMethod or DPID to check peer's capability.
								Some P2P device is display but the value of ConfigMethod will be 0x0188 and  (ex. Samsung GALAXYSII).
							*/
							bReadOwnPIN = FALSE;
						}
					}
				}
#endif /* P2P_SUPPORT */
#ifdef IWSC_SUPPORT
				if (pAdapter->StaCfg.BssType == BSS_ADHOC)
					bReadOwnPIN = FALSE;
#endif /* IWSC_SUPPORT */

				if (bReadOwnPIN)
				{
					pWscControl->WscPinCodeLen = pWscControl->WscEnrolleePinCodeLen;
					WscGetRegDataPIN(pAdapter, pWscControl->WscEnrolleePinCode, pWscControl);
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("(%d) WscEnrolleePinCode: %08u\n", bReadOwnPIN, pWscControl->WscEnrolleePinCode));
				}
				else
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscPinCode: %08u\n", pWscControl->WscPinCode));
			}
			/* If Message is from EAP, but UPnP Registrar is in progress now, ignore EAP Messages */
			if (!bUPnPMsg && pWscControl->WscUPnPNodeInfo.bUPnPInProgress)
			{
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("UPnP Registrar is working now, ignore EAP Messages.\n"));
				goto out;
			}
			else
				WscEapEnrolleeAction(pAdapter, Elem, MsgType, pEntry, pWscControl);
		}
#ifdef CONFIG_AP_SUPPORT        
		else if (((pWscControl->WscConfMode & WSC_PROXY) != 0) && (bUPnPMsg) && (CurOpMode == AP_MODE))
		{
			pWscControl->WscActionMode = WSC_PROXY;
			WscEapApProxyAction(pAdapter, Elem, MsgType, pEntry, pWscControl);
		}
#endif /* CONFIG_AP_SUPPORT */        
	}
	else if (MsgType == WSC_MSG_WSC_ACK)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscState: %d\n", pWscControl->WscState));
		if (((pWscControl->WscConfMode & WSC_REGISTRAR) != 0) && 
			pWscControl->WscState <= WSC_STATE_SENT_M2D)
		{
			if (WscCheckNonce(pAdapter, Elem, TRUE, pWscControl))
			{
				if (pWscControl->M2DACKBalance > 0)
					pWscControl->M2DACKBalance--;
				pWscControl->WscState = WSC_STATE_INIT;
				pWscControl->EapMsgRunning = FALSE;
			}
		}
		else
		{
			if (((pWscControl->WscConfMode & WSC_ENROLLEE) != 0) && 
				WscCheckNonce(pAdapter, Elem, FALSE, pWscControl))
			{
				pWscControl->WscActionMode = WSC_ENROLLEE;
				pWscControl->WscUseUPnP = bUPnPMsg ? 1 : 0;
				WscEapEnrolleeAction(pAdapter, Elem, MsgType, pEntry, pWscControl);
			}
#ifdef CONFIG_AP_SUPPORT            
			else if (((pWscControl->WscConfMode & WSC_PROXY) != 0) && (CurOpMode == AP_MODE))
			{
				pWscControl->WscActionMode = WSC_PROXY;
				WscEapApProxyAction(pAdapter, Elem, MsgType, pEntry, pWscControl);
			}
#endif /* CONFIG_AP_SUPPORT */
		}
	}
	else if (MsgType == WSC_MSG_WSC_NACK)
	{
		BOOLEAN bReSetWscIE = FALSE;
		if (bUPnPMsg)
		{
			if ((pWscControl->WscState == WSC_STATE_WAIT_M8) &&
				(pWscControl->WscConfStatus == WSC_SCSTATE_CONFIGURED))
			{
				// Some external sta will send NACK when AP is configured.
				// bWscTrigger should be set FALSE, otherwise Proxy will send NACK to enrollee.
				pWscControl->bWscTrigger = FALSE;
				pWscControl->WscStatus = STATUS_WSC_CONFIGURED;
				bReSetWscIE = TRUE;
			}
#if 0
			/*
				WPS2.0 test item 4.2.3 Atheros ER configure enrollee AP fail.
				Test Step: In Atheros ER console:
					1. ER learn AP configure.
					2. ER configure AP.
						
				Desc:
					Step 1, ER only learn AP configure, then ER receive M7 will send NACK to AP by UPNP.
					And our AP will trigger 15 seconds UPNP timer to send NACK Message from UPNP to ER.
					Due to ER not receive NACK message then Step 2 will fail. 
					
			*/
#endif
			{
		        int dataLen;
		        UCHAR *pWscData;
				BOOLEAN bUPnPStatus = FALSE;

				os_alloc_mem(NULL, (UCHAR **)&pWscData, WSC_MAX_DATA_LEN);
				if (pWscData != NULL)
		        {
		            memset(pWscData, 0, WSC_MAX_DATA_LEN);
		            dataLen = BuildMessageNACK(pAdapter, pWscControl, pWscData);
	  				bUPnPStatus = WscSendUPnPMessage(pAdapter, (pWscControl->EntryIfIdx & 0x0F), WSC_OPCODE_UPNP_DATA,
	                                         WSC_UPNP_DATA_SUB_NORMAL, pWscData, dataLen,
	                                         Elem->TimeStamp.u.LowPart, Elem->TimeStamp.u.HighPart,
	                                         &pAdapter->CurrentAddress[0], CurOpMode);
					os_free_mem(NULL, pWscData);
					if (bUPnPStatus == FALSE)
				        WscUPnPErrHandle(pAdapter, pWscControl, Elem->TimeStamp.u.LowPart);
		        }

				if (pWscUPnPNodeInfo->bUPnPMsgTimerRunning == TRUE)
	            {
	                RTMPCancelTimer(&pWscUPnPNodeInfo->UPnPMsgTimer, &Cancelled);
	                pWscUPnPNodeInfo->bUPnPMsgTimerRunning = FALSE;
	            }
	            pWscUPnPNodeInfo->bUPnPInProgress = FALSE;

		        RTMPSendWirelessEvent(pAdapter, IW_WSC_STATUS_FAIL, NULL, (pWscControl->EntryIfIdx & 0x0F), 0);
    		}	

		}
		
		if (!bUPnPMsg &&
			(WscCheckNonce(pAdapter, Elem, FALSE, pWscControl) || WscCheckNonce(pAdapter, Elem, TRUE, pWscControl)))
		{
			USHORT config_error = 0;
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Receive NACK from WPS client.\n"));
			WscGetConfigErrFromNack(pAdapter, Elem, &config_error);
			/*
			If a PIN authentication or communication error occurs, 
			the Registrar MUST warn the user and MUST NOT automatically reuse the PIN. 
			Furthermore, if the Registrar detects this situation and prompts the user for a new PIN from the Enrollee device, 
			it MUST NOT accept the same PIN again without warning the user of a potential attack.
			*/
			if ((pWscControl->WscState >= WSC_STATE_WAIT_M5) && (config_error != WSC_ERROR_SETUP_LOCKED))
			{
				pWscControl->WscRejectSamePinFromEnrollee = TRUE;
				pWscControl->WscPinCode = 0;

				if (pWscControl->WscState < WSC_STATE_WAIT_M8)
				{
					pWscControl->WscStatus = STATUS_WSC_FAIL;
					RTMPSendWirelessEvent(pAdapter, IW_WSC_STATUS_FAIL, NULL, (pWscControl->EntryIfIdx & 0x0F), 0);
					bReSetWscIE = TRUE;
				}
			}

#ifdef CONFIG_AP_SUPPORT
			if ((pWscControl->WscState == WSC_STATE_OFF)
				&& (CurOpMode == AP_MODE)
				&& (pWscControl->RegData.SelfInfo.ConfigError != WSC_ERROR_NO_ERROR))
			{
				bReSetWscIE = TRUE;
			}
#endif /* CONFIG_AP_SUPPORT */

			if ((pWscControl->WscState == WSC_STATE_WAIT_M8) &&
				(pWscControl->WscConfStatus == WSC_SCSTATE_CONFIGURED))
			{
				/* Some external sta will send NACK when AP is configured. */
				/* bWscTrigger should be set FALSE, otherwise Proxy will send NACK to enrollee. */
				pWscControl->bWscTrigger = FALSE;
				bReSetWscIE = TRUE;
				pWscControl->WscStatus = STATUS_WSC_CONFIGURED;
				pWscControl->WscRejectSamePinFromEnrollee = FALSE;
				RTMPSendWirelessEvent(pAdapter, IW_WSC_STATUS_SUCCESS, NULL, (pWscControl->EntryIfIdx & 0x0F), 0);
#ifdef P2P_SUPPORT
				/*RTMPCancelTimer(&pAdapter->P2pCfg.P2pWscTimer, &Cancelled);*/
				if (P2P_GO_ON(pAdapter) && pWscControl->EntryIfIdx != BSS0)
				{
					UCHAR	P2pIdx = P2P_NOT_FOUND;
					P2pIdx = P2pGroupTabSearch(pAdapter, MacAddr);
					if (P2pIdx != P2P_NOT_FOUND)
					{
						PRT_P2P_CLIENT_ENTRY pP2pEntry = &pAdapter->P2pTable.Client[P2pIdx];
						// Update p2p Entry's state.
						pP2pEntry->P2pClientState = P2PSTATE_CLIENT_WPS_DONE;
					}
				}

				// default set extended listening to zero for each connection. If this is persistent, will set it.
				pAdapter->P2pCfg.ExtListenInterval = 0;
				pAdapter->P2pCfg.ExtListenPeriod = 0;
				if (IS_PERSISTENT_ON(pAdapter) && (pEntry->bP2pClient == TRUE))
				{
					UCHAR	P2pIdx = P2P_NOT_FOUND;
					P2pIdx = P2pGroupTabSearch(pAdapter, pEntry->Addr);

					if (IS_P2P_GO_ENTRY(pEntry))
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("pEntry is P2P GO.\n"));
					else if (IS_P2P_CLI_ENTRY(pEntry))
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("pEntry is P2P CLIENT.\n"));
					else
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("pEntry is P2P NONE.\n"));

					if ((P2pIdx != P2P_NOT_FOUND) && (IS_P2P_GO_ENTRY(pEntry) || IS_P2P_CLI_ENTRY(pEntry)))
					{
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("P2pWPSDone- Save to persistent entry. GrpCap= %x \n", pAdapter->P2pTable.Client[P2pIdx].GroupCapability));
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("3. P2pWPSDone-	Set Extended timing !!!!!!!\n"));
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("    ======== Profile :: Cnt = %d ========\n", pWscControl->WscProfile.ProfileCnt));
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("    SSID[%d] = %s.\n", pWscControl->WscProfile.Profile[0].SSID.SsidLength, pWscControl->WscProfile.Profile[0].SSID.Ssid));
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("    AuthType = %d.    EncrType = %d.\n", pWscControl->WscProfile.Profile[0].AuthType, pWscControl->WscProfile.Profile[0].EncrType));
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("    MAC = %02x:%02x:%02x:%02x:%02x:%02x.\n", PRINT_MAC(pWscControl->WscProfile.Profile[0].MacAddr)));
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("    KeyLen = %d.    KeyIdx = %d.\n", pWscControl->WscProfile.Profile[0].KeyLength, pWscControl->WscProfile.Profile[0].KeyIndex));
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("    Key :: %02x %02x %02x %02x  %02x %02x %02x %02x\n", pWscControl->WscProfile.Profile[0].Key[0], pWscControl->WscProfile.Profile[0].Key[1], pWscControl->WscProfile.Profile[0].Key[2],
													pWscControl->WscProfile.Profile[0].Key[3], pWscControl->WscProfile.Profile[0].Key[4], pWscControl->WscProfile.Profile[0].Key[5], pWscControl->WscProfile.Profile[0].Key[6], pWscControl->WscProfile.Profile[0].Key[7]));
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("             %02x %02x %02x %02x  %02x %02x %02x %02x\n", pWscControl->WscProfile.Profile[0].Key[8], pWscControl->WscProfile.Profile[0].Key[9], pWscControl->WscProfile.Profile[0].Key[10],
													pWscControl->WscProfile.Profile[0].Key[11], pWscControl->WscProfile.Profile[0].Key[12], pWscControl->WscProfile.Profile[0].Key[13], pWscControl->WscProfile.Profile[0].Key[14], pWscControl->WscProfile.Profile[0].Key[15]));

						P2pPerstTabInsert(pAdapter, pEntry->Addr, &pWscControl->WscProfile.Profile[0]);
						// this is a persistent connection.
						pAdapter->P2pCfg.ExtListenInterval = P2P_EXT_LISTEN_INTERVAL;
						pAdapter->P2pCfg.ExtListenPeriod = P2P_EXT_LISTEN_PERIOD;
					}
				}
#endif //P2P_SUPPORT //
			}
#ifdef CONFIG_AP_SUPPORT
			else if ((CurOpMode == AP_MODE) &&
					(pWscControl->WscState == WSC_STATE_WAIT_DONE) &&
					(pWscControl->WscConfStatus == WSC_SCSTATE_CONFIGURED) &&
					(pAdapter->ApCfg.MBSSID[apidx].wdev.WepStatus == Ndis802_11WEPEnabled))
			{
				bReSetWscIE = TRUE;
				pWscControl->WscStatus = STATUS_WSC_FAIL;
			}

			if ((CurOpMode == AP_MODE) && bReSetWscIE)
			{
				WscBuildBeaconIE(pAdapter, WSC_SCSTATE_CONFIGURED, FALSE, 0, 0, (pWscControl->EntryIfIdx & 0x0F), NULL, 0, CurOpMode);
				WscBuildProbeRespIE(pAdapter, WSC_MSGTYPE_AP_WLAN_MGR, WSC_SCSTATE_CONFIGURED, FALSE, 0, 0, pWscControl->EntryIfIdx, NULL, 0, CurOpMode);
				APUpdateBeaconFrame(pAdapter, pWscControl->EntryIfIdx & 0x0F);
				if (pWscControl->Wsc2MinsTimerRunning)
				{
					RTMPCancelTimer(&pWscControl->Wsc2MinsTimer, &Cancelled);
					pWscControl->Wsc2MinsTimerRunning = FALSE;
				}
				if (pWscControl->bWscTrigger)
					pWscControl->bWscTrigger = FALSE;
			}
#endif // CONFIG_AP_SUPPORT //

			if ((CurOpMode == AP_MODE)
				|| ((ADHOC_ON(pAdapter)) && (pWscControl->WscConfMode == WSC_REGISTRAR))
				)
			{
				WscSendEapFail(pAdapter, pWscControl, TRUE);
				pWscControl->WscState = WSC_STATE_FAIL;
			}
#ifdef CONFIG_STA_SUPPORT
			else if ((CurOpMode == STA_MODE) && INFRA_ON(pAdapter))
			{
				WscEapActionDisabled(pAdapter, pWscControl);
				pWscControl->WscState = WSC_STATE_WAIT_DISCONN;
#ifdef DPA_T
				/* TOSHIBA modified 2010.02.22 */
				if(pWscControl->WscStatus == 0)
				{
					pWscControl->WscStatus = STATUS_WSC_FAIL;
				}
				else
				{
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("NOW WSC STATUS = %d\n", pWscControl->WscStatus));
				}
#endif /* DPA_T */
			}
#endif /* CONFIG_STA_SUPPORT */
			
			RTMPCancelTimer(&pWscControl->EapolTimer, &Cancelled);			
			pWscControl->EapolTimerRunning = FALSE;
			pWscControl->RegData.ReComputePke = 1;
		}
	}
	else
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Unsupported Msg Type (%02X)\n", MsgType));
		pWscControl->WscStatus = STATUS_WSC_FAIL;
		pWscControl->RegData.SelfInfo.ConfigError = WSC_ERROR_NO_ERROR;
		WscSendNACK(pAdapter, pEntry, pWscControl);
		RTMPSendWirelessEvent(pAdapter, IW_WSC_STATUS_FAIL, NULL, BSS0, 0);
		goto out;
	}

	if (bUPnPMsg)
	{
		/* Messages from UPnP */
		if (pWscUPnPNodeInfo->bUPnPMsgTimerRunning)
			RTMPModTimer(&pWscUPnPNodeInfo->UPnPMsgTimer, WSC_UPNP_MSG_TIME_OUT);
	}
	else
	{
		if ((pWscControl->EapMsgRunning == TRUE) && 
			(!RTMP_TEST_FLAG(pAdapter, fRTMP_ADAPTER_HALT_IN_PROGRESS |
									   fRTMP_ADAPTER_NIC_NOT_EXIST)))
		{
			/* Messages from EAP */
			RTMPModTimer(&pWscControl->EapolTimer, WSC_EAP_MSG_TIME_OUT);
			pWscControl->EapolTimerRunning = TRUE;
		}
	}
	
	if (bUPnPMsg && pWscControl->EapolTimerRunning)
	{   
#ifdef CONFIG_AP_SUPPORT	
		if ((pWscControl->WscActionMode == WSC_PROXY) && (CurOpMode == AP_MODE))
		{
			RTMPModTimer(&pWscControl->EapolTimer, WSC_EAP_MSG_TIME_OUT);
		}
		else
#endif /* CONFIG_AP_SUPPORT */            
		{
			RTMPCancelTimer(&pWscControl->EapolTimer, &Cancelled);
			pWscControl->EapolTimerRunning = FALSE;
		}
	}

out:
	if (bUPnPMsg)
		pWscUPnPNodeInfo->bUPnPMsgTimerPending = FALSE;
	
	pWscControl->EapolTimerPending = FALSE;
	
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscEAPAction\n"));
}

/*
	============================================================================
	Enrollee			Enrollee			Enrollee	
	============================================================================	
*/
VOID WscEapEnrolleeAction(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	MLME_QUEUE_ELEM	*Elem,
	IN  UCHAR	        MsgType,
	IN  MAC_TABLE_ENTRY *pEntry,
	IN  PWSC_CTRL       pWscControl)
{
    INT     DataLen = 0, rv = 0, DH_Len = 0;
	UCHAR   OpCode/*, bssIdx*/;
    PUCHAR  WscData = NULL;
    BOOLEAN bUPnPMsg, bUPnPStatus = FALSE, Cancelled;
	WSC_UPNP_NODE_INFO *pWscUPnPInfo = &pWscControl->WscUPnPNodeInfo;
	UINT	MaxWscDataLen = WSC_MAX_DATA_LEN;
	UCHAR	CurOpMode = 0xFF;
#ifdef DPA_T
	PWSC_CREDENTIAL pCredential;
#endif /* DPA_T */

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapEnrolleeAction Enter!\n"));

	bUPnPMsg = Elem->MsgType == WSC_EAPOL_UPNP_MSG ? TRUE : FALSE;
	OpCode = bUPnPMsg ? WSC_OPCODE_UPNP_MASK : 0;
	//bssIdx = 0;

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAdapter)
		CurOpMode = AP_MODE;
#endif // CONFIG_AP_SUPPORT //

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAdapter)
		CurOpMode = STA_MODE;
#ifdef P2P_SUPPORT
	if (Elem->OpMode != OPMODE_STA)
		CurOpMode = AP_MODE;
#endif /* P2P_SUPPORT */
#endif // CONFIG_STA_SUPPORT //

#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
	{
		/* Early check. */
		if ((pWscControl->WscActionMode != WSC_ENROLLEE) ||
			(pWscControl->WscUseUPnP && pEntry) ||
			((pWscControl->WscUseUPnP == 0) && (!pEntry)))
		{	
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("EarlyCheckFailed: pWscControl->WscActionMode=%d, Configured=%d, WscUseUPnP=%d, pEntry=%p!\n", 
						pWscControl->WscActionMode, pWscControl->WscConfStatus, pWscControl->WscUseUPnP, pEntry));
			goto Fail;
		}
		//bssIdx = (pWscControl->EntryIfIdx & 0x0F);
	}
#endif /* CONFIG_AP_SUPPORT */
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("MsgType=0x%x, WscState=%d, bUPnPMsg=%d!\n", MsgType, pWscControl->WscState, bUPnPMsg));

	if (bUPnPMsg)
	{
#ifdef CONFIG_AP_SUPPORT
		if ((MsgType == WSC_MSG_EAP_RSP_ID) && (CurOpMode == AP_MODE))
		{
			/* let it pass */
		} else
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
		if ((MsgType ==WSC_MSG_EAP_REQ_START) &&  (CurOpMode == STA_MODE))
		{
			/*let it pass */
		} else 
	#endif /* CONFIG_STA_SUPPORT */
		if(MsgType ==WSC_MSG_M2 && pWscUPnPInfo->bUPnPInProgress == FALSE)
		{
#ifdef CONFIG_AP_SUPPORT
			if (CurOpMode == AP_MODE)
			{
					MAC_TABLE_ENTRY *tempEntry;
					tempEntry = MacTableLookup(pAdapter, &pWscControl->EntryAddr[0]);
					if (tempEntry)
					{
						if((tempEntry->Receive_EapolStart_EapRspId & WSC_ENTRY_GET_EAP_RSP_ID) == WSC_ENTRY_GET_EAP_RSP_ID)
						{
							goto Done;
						}
					}
				/* else cannot find the pEntry, so we need to handle this msg. */
			}
#endif /* CONFIG_AP_SUPPORT */
			pWscUPnPInfo->bUPnPInProgress = TRUE;
			/* Set the WscState as "WSC_STATE_WAIT_RESP_ID" because UPnP start from this state. */
			/* pWscControl->WscState = WSC_STATE_WAIT_RESP_ID; */
			RTMPSetTimer(&pWscUPnPInfo->UPnPMsgTimer, WSC_UPNP_MSG_TIME_OUT);
			pWscUPnPInfo->bUPnPMsgTimerRunning = TRUE;
		}
		else 
		{
			/* For other messages, we must make sure pWscUPnPInfo->bUPnPInProgress== TRUE */
			if (pWscUPnPInfo->bUPnPInProgress == FALSE)
			{
				goto Done;
			}
		}
	}

#ifdef WSC_V2_SUPPORT 
	MaxWscDataLen = MaxWscDataLen + (UINT)pWscControl->WscV2Info.ExtraTlv.TlvLen;
#endif /* WSC_V2_SUPPORT */
	os_alloc_mem(NULL, (UCHAR **)&WscData, MaxWscDataLen);
	if (WscData == NULL)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscData Allocate failed!\n"));
		goto Fail;
	}
	NdisZeroMemory(WscData, MaxWscDataLen);

	switch (MsgType)
	{
		case WSC_MSG_EAP_RSP_ID:
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapEnrolleeAction : Rx Identity(ReComputePke=%d)\n", pWscControl->RegData.ReComputePke));
		case WSC_MSG_EAP_REQ_START:
			if (MsgType == WSC_MSG_EAP_REQ_START)
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapEnrolleeAction : Rx Wsc_Start(ReComputePke=%d)\n", pWscControl->RegData.ReComputePke));
			
#ifdef WSC_NFC_SUPPORT
			if (pWscControl->bTriggerByNFC)
				; /* Do NOT need to generate EnrolleeRandom and DH public key here. */
			else 
#endif /* WSC_NFC_SUPPORT */
			{
#ifdef CONFIG_AP_SUPPORT
				/*
					We don't need to consider P2P case.
				*/
				IF_DEV_CONFIG_OPMODE_ON_AP(pAdapter)
				{					
					if ((pWscControl->bWscAutoTriggerDisable == TRUE) &&
						(pWscControl->bWscTrigger == FALSE))
					{
						if (bUPnPMsg == TRUE)
						{
							MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, 
								("%s(%d): WscAutoTrigger is disabled.\n", __FUNCTION__, __LINE__));
							WscUPnPErrHandle(pAdapter, pWscControl, Elem->TimeStamp.u.LowPart);
							os_free_mem(NULL, WscData);
							return;
						}
						else if (pEntry && IS_ENTRY_CLIENT(pEntry))
						{
							MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, 
								("%s(%d): WscAutoTrigger is disabled! Send EapFail to STA.\n", __FUNCTION__, __LINE__));
							WscSendEapFail(pAdapter, pWscControl, TRUE);
							os_free_mem(NULL, WscData);
							return;
						}
						else
							; /* Keep going. APCLI shall be the else case. */
					}
				}
#endif /* CONFIG_AP_SUPPORT */

				if (pWscControl->RegData.ReComputePke == 1)
				{
					INT idx;
                	DH_Len = sizeof(pWscControl->RegData.Pke);
					/* Enrollee 192 random bytes for DH key generation */
					for (idx = 0; idx < 192; idx++)
						pWscControl->RegData.EnrolleeRandom[idx] = RandomByte(pAdapter);
                    NdisZeroMemory(pWscControl->RegData.Pke, sizeof(pWscControl->RegData.Pke));
            		RT_DH_PublicKey_Generate (
                    	WPS_DH_G_VALUE, sizeof(WPS_DH_G_VALUE),
            	    	WPS_DH_P_VALUE, sizeof(WPS_DH_P_VALUE),
            	    	pWscControl->RegData.EnrolleeRandom, sizeof(pWscControl->RegData.EnrolleeRandom),
            	    	pWscControl->RegData.Pke, (UINT *) &DH_Len);

                    /* Need to prefix zero padding */
                    if((DH_Len != sizeof(pWscControl->RegData.Pke)) &&
                        (DH_Len < sizeof(pWscControl->RegData.Pke)))
                    {
                        UCHAR TempKey[192];
                        INT DiffCnt;
                        DiffCnt = sizeof(pWscControl->RegData.Pke) - DH_Len;

                        NdisFillMemory(&TempKey, DiffCnt, 0);
                        NdisCopyMemory(&TempKey[DiffCnt], pWscControl->RegData.Pke, DH_Len);
                        NdisCopyMemory(pWscControl->RegData.Pke, TempKey, sizeof(TempKey));
                        DH_Len += DiffCnt;
                        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: Do zero padding!\n", __FUNCTION__));
                    }
                    
					pWscControl->RegData.ReComputePke = 0;
				}
			}
			OpCode |= WSC_OPCODE_MSG;
            
			DataLen = BuildMessageM1(pAdapter, pWscControl, WscData);
			if(!bUPnPMsg)
			{
#ifdef CONFIG_AP_SUPPORT
				if (CurOpMode == AP_MODE)
				{
					if (pEntry && IS_ENTRY_CLIENT(pEntry))
						WscDelWPARetryTimer(pAdapter);
				}
#endif /* CONFIG_AP_SUPPORT */
				pWscControl->EapMsgRunning = TRUE;
				pWscControl->WscStatus = STATUS_WSC_EAP_M1_SENT;
			}
			else
				/* Sometime out-of-band registrars (ex: Vista) get M1 for collecting information of device. */
				pWscControl->WscStatus = STATUS_WSC_IDLE;
            
			/* Change the state to next one */
			if (pWscControl->WscState < WSC_STATE_SENT_M1)
		        pWscControl->WscState = WSC_STATE_SENT_M1;

		 		RTMPSendWirelessEvent(pAdapter, IW_WSC_SEND_M1, NULL, (pWscControl->EntryIfIdx & 0x0F), 0);
			break;
			
		case WSC_MSG_M2:
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapEnrolleeAction : Rx M2\n"));
            
			/* Receive M2, if we are at WSC_STATE_WAIT_M2 start, process it immediately */
			if (pWscControl->WscState == WSC_STATE_SENT_M1 ||
				pWscControl->WscState == WSC_STATE_RX_M2D)
			{
					/* Process M2 */
					pWscControl->WscStatus = STATUS_WSC_EAP_M2_RECEIVED;

					NdisMoveMemory(pWscControl->RegData.PeerInfo.MacAddr, pWscControl->EntryAddr, 6);
					if ((rv = ProcessMessageM2(pAdapter, pWscControl, Elem->Msg, Elem->MsgLen, (pWscControl->EntryIfIdx & 0x0F), &pWscControl->RegData)))
					{
							goto Fail;
					}
					else
					{
#ifdef CONFIG_AP_SUPPORT
#ifdef WSC_V2_SUPPORT
						if ((CurOpMode == AP_MODE) && pWscControl->bSetupLock)
						{
							rv = WSC_ERROR_SETUP_LOCKED;
							goto Fail;
						}
#endif /* WSC_V2_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */						
#ifdef WSC_NFC_SUPPORT	
						if (pWscControl->bTriggerByNFC && pWscControl->NfcModel == MODEL_HANDOVER )
						{
							/* NFC handover Skip M3~M8 */
							OpCode |= WSC_OPCODE_DONE;
							DataLen = BuildMessageDONE(pAdapter, pWscControl, WscData);
							//pWscControl->WscStatus = STATUS_WSC_EAP_M3_SENT;
							pWscControl->WscState = WSC_STATE_WAIT_ACK;
							pWscControl->PinAttackCount = 0;
							RTMPSendWirelessEvent(pAdapter, IW_WSC_SEND_DONE, NULL, pWscControl->EntryIfIdx, 0);							
						}
						else
#endif /* WSC_NFC_SUPPORT */
						{
						OpCode |= WSC_OPCODE_MSG;
						DataLen = BuildMessageM3(pAdapter, pWscControl, WscData);
						pWscControl->WscStatus = STATUS_WSC_EAP_M3_SENT;

						/* Change the state to next one */
						pWscControl->WscState = WSC_STATE_WAIT_M4;
		 				RTMPSendWirelessEvent(pAdapter, IW_WSC_SEND_M3, NULL, (pWscControl->EntryIfIdx & 0x0F), 0);
					}
			}
			}
			break;
			
		case WSC_MSG_M2D:
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapEnrolleeAction : Rx M2D\n"));
            
			/* Receive M2D, if we are at WSC_STATE_WAIT_M2 start, process it immediately */
			if (pWscControl->WscState == WSC_STATE_SENT_M1 ||
				pWscControl->WscState == WSC_STATE_RX_M2D)
			{
				BOOLEAN bReplyNack = FALSE;
					
				if ((rv = ProcessMessageM2D(pAdapter, Elem->Msg, Elem->MsgLen, &pWscControl->RegData)))
					goto Fail;

				pWscControl->WscStatus = STATUS_WSC_EAP_M2D_RECEIVED;
				
				if (CurOpMode == AP_MODE)
				{
					bReplyNack = TRUE;
#ifdef APCLI_SUPPORT                                                                 
					if (pEntry && !IS_ENTRY_APCLI(pEntry))
						bReplyNack = TRUE;
					else
						bReplyNack = FALSE;
#endif   
#ifdef P2P_SUPPORT
					if (P2P_INF_ON(pAdapter))
					{
						if (P2P_GO_ON(pAdapter))
							bReplyNack = TRUE;
						else
							bReplyNack = FALSE;
					}
#endif
				}
				
				if (bReplyNack)
				{
					/* For VISTA SP1 internal registrar test */
					OpCode |= WSC_OPCODE_NACK;
					DataLen = BuildMessageNACK(pAdapter, pWscControl, WscData);
	 				RTMPSendWirelessEvent(pAdapter, IW_WSC_SEND_NACK, NULL, (pWscControl->EntryIfIdx & 0x0F), 0);
				}
				else
				{
					/* When external registrar is Marvell station, */
					/* wps station sends NACK may confuse or reset Marvell wps state machine. */
					OpCode |= WSC_OPCODE_ACK;
					DataLen = BuildMessageACK(pAdapter, pWscControl, WscData);
	 				RTMPSendWirelessEvent(pAdapter, IW_WSC_SEND_ACK, NULL, (pWscControl->EntryIfIdx & 0x0F), 0);
				}

				/* Change the state to next one */
				pWscControl->WscState = WSC_STATE_RX_M2D;
			}
			break;

		case WSC_MSG_M4: 
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapEnrolleeAction : Rx M4\n"));

			/* Receive M4, if we are at WSC_STATE_WAIT_M4 start, process it immediately */
			if (pWscControl->WscState == WSC_STATE_WAIT_M4)
			{       
				/* Process M4 */
				pWscControl->WscStatus = STATUS_WSC_EAP_M4_RECEIVED;
				if ((rv = ProcessMessageM4(pAdapter, pWscControl, Elem->Msg, Elem->MsgLen, &pWscControl->RegData)))
				{
#ifdef CONFIG_AP_SUPPORT
#ifdef WSC_V2_SUPPORT
					if (CurOpMode == AP_MODE)
						WscCheckPinAttackCount(pAdapter, pWscControl);
#endif /* WSC_V2_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
					goto Fail;
				}
				else
				{
					OpCode |= WSC_OPCODE_MSG;
					DataLen = BuildMessageM5(pAdapter, pWscControl, WscData);
					pWscControl->WscStatus = STATUS_WSC_EAP_M5_SENT;

					/* Change the state to next one */
					pWscControl->WscState = WSC_STATE_WAIT_M6;
	 				RTMPSendWirelessEvent(pAdapter, IW_WSC_SEND_M5, NULL, (pWscControl->EntryIfIdx & 0x0F), 0);
				}
			}
			break;

		case WSC_MSG_M6:
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapEnrolleeAction : Rx M6\n"));

			/* Receive M6, if we are at WSC_STATE_WAIT_M6 start, process it immediately */
			if (pWscControl->WscState == WSC_STATE_WAIT_M6)
			{      
				/* Process M6 */
				pWscControl->WscStatus = STATUS_WSC_EAP_M6_RECEIVED;
				if ((rv=ProcessMessageM6(pAdapter, pWscControl, Elem->Msg, Elem->MsgLen, &pWscControl->RegData)))
				{
#ifdef CONFIG_AP_SUPPORT
#ifdef WSC_V2_SUPPORT
					if (CurOpMode == AP_MODE)
						WscCheckPinAttackCount(pAdapter, pWscControl);
#endif /* WSC_V2_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
					goto Fail;
				}
				else
				{
					OpCode |= WSC_OPCODE_MSG;

					DataLen = BuildMessageM7(pAdapter, pWscControl, WscData);
					pWscControl->WscStatus = STATUS_WSC_EAP_M7_SENT;
					/* Change the state to next one */
					pWscControl->WscState = WSC_STATE_WAIT_M8;
	 				RTMPSendWirelessEvent(pAdapter, IW_WSC_SEND_M7, NULL, (pWscControl->EntryIfIdx & 0x0F), 0);
					/*
						Complete WPS with this STA. Delete it from WscPeerList for others STA to do WSC with AP
					*/
					if (pEntry)
					{
						RTMP_SEM_LOCK(&pWscControl->WscPeerListSemLock);
						WscDelListEntryByMAC(&pWscControl->WscPeerList, pEntry->Addr);
						RTMP_SEM_UNLOCK(&pWscControl->WscPeerListSemLock);
					}
				}
			}
			break;

		case WSC_MSG_M8:
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapEnrolleeAction : Rx M8\n"));

			/* Receive M8, if we are at WSC_STATE_WAIT_M6 start, process it immediately */
			if (pWscControl->WscState == WSC_STATE_WAIT_M8)
			{
				/* Process M8 */
				pWscControl->WscStatus = STATUS_WSC_EAP_M8_RECEIVED;
				if ((rv=ProcessMessageM8(pAdapter, Elem->Msg, Elem->MsgLen, pWscControl)))
					goto Fail;
				else
				{
					OpCode |= WSC_OPCODE_DONE;
					DataLen = BuildMessageDONE(pAdapter, pWscControl, WscData);

#ifdef CONFIG_AP_SUPPORT
					if (CurOpMode == AP_MODE)
					{
						/* Change the state to next one */
#ifdef APCLI_SUPPORT
						/* Ap Client only supports Inband(EAP)-Enrollee. */
						if (!bUPnPMsg && pEntry && IS_ENTRY_APCLI(pEntry))
							pWscControl->WscState = WSC_STATE_WAIT_EAPFAIL;
						else
#endif /* APCLI_SUPPORT */
						pWscControl->WscState = WSC_STATE_WAIT_ACK;
#if 0
						pWscControl->RegData.ReComputePke = 0;
#endif
					}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
					if (CurOpMode == STA_MODE)
					{
						pWscControl->WscState = WSC_STATE_WAIT_EAPFAIL;
						pWscControl->WscStatus = STATUS_WSC_EAP_RSP_DONE_SENT;
					}
#endif /* CONFIG_STA_SUPPORT */
	 				RTMPSendWirelessEvent(pAdapter, IW_WSC_SEND_DONE, NULL, (pWscControl->EntryIfIdx & 0x0F), 0);
				}
			}
			break;	

#ifdef CONFIG_AP_SUPPORT
		case WSC_MSG_WSC_ACK:
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapEnrolleeAction : Rx ACK\n"));
            
			/* Receive ACK */
			if (pWscControl->WscState == WSC_STATE_WAIT_ACK)
			{
				/* Process ACK */
				pWscControl->WscStatus = STATUS_WSC_EAP_RAP_RSP_ACK;
				/* Send out EAP-Fail */
				WscSendEapFail(pAdapter, pWscControl, FALSE);
				pWscControl->WscState = WSC_STATE_CONFIGURED;                
				pWscControl->WscStatus = STATUS_WSC_CONFIGURED;
#ifdef P2P_SUPPORT
				pAdapter->P2pCfg.WscState = WSC_STATE_CONFIGURED;
#ifdef RT_P2P_SPECIFIC_WIRELESS_EVENT
				P2pSendWirelessEvent(pAdapter, RT_P2P_WPS_COMPLETED, NULL, NULL);
#endif /* RT_P2P_SPECIFIC_WIRELESS_EVENT */
#endif /* P2P_SUPPORT */
			}
			break;
#endif /* CONFIG_AP_SUPPORT */

		default:
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapEnrolleeAction : Unsupported Msg Type\n"));
			break;
	}
	
	if (bUPnPMsg)
	{
		if ((MsgType == WSC_MSG_M8) && (pWscControl->WscState == WSC_STATE_WAIT_ACK))
		{
			pWscControl->EapMsgRunning = FALSE;
			pWscControl->WscState = WSC_STATE_CONFIGURED;
			pWscControl->WscStatus = STATUS_WSC_CONFIGURED;
#ifdef P2P_SUPPORT
			pAdapter->P2pCfg.WscState = WSC_STATE_CONFIGURED;
#ifdef RT_P2P_SPECIFIC_WIRELESS_EVENT
			P2pSendWirelessEvent(pAdapter, RT_P2P_WPS_COMPLETED, NULL, NULL);
#endif /* RT_P2P_SPECIFIC_WIRELESS_EVENT */
#endif /* P2P_SUPPORT */
			if(pWscUPnPInfo->bUPnPMsgTimerRunning == TRUE)
			{
				RTMPCancelTimer(&pWscUPnPInfo->UPnPMsgTimer, &Cancelled);
				pWscUPnPInfo->bUPnPMsgTimerRunning = FALSE;
			}
			pWscUPnPInfo->bUPnPInProgress = FALSE;
			pWscUPnPInfo->registrarID = 0;
		}
	}
	else
	{
		if (((MsgType == WSC_MSG_WSC_ACK) && (pWscControl->WscState == WSC_STATE_CONFIGURED)) ||
			((MsgType == WSC_MSG_M8) && (pWscControl->WscState == WSC_STATE_WAIT_ACK)))
		{
			RTMPCancelTimer(&pWscControl->EapolTimer, &Cancelled);
			pWscControl->EapolTimerRunning = FALSE;
			pWscControl->EapMsgRunning = FALSE;
			/*NdisZeroMemory(pWscControl->EntryAddr, MAC_ADDR_LEN); */
		}
	}
	
	if(OpCode > WSC_OPCODE_UPNP_MASK)
		bUPnPStatus = WscSendUPnPMessage(pAdapter, (pWscControl->EntryIfIdx & 0x0F), WSC_OPCODE_UPNP_DATA, 
											WSC_UPNP_DATA_SUB_NORMAL, WscData, DataLen, 
											Elem->TimeStamp.u.LowPart, Elem->TimeStamp.u.HighPart, 
											&pAdapter->CurrentAddress[0], CurOpMode);
	else if(OpCode > 0 && OpCode < WSC_OPCODE_UPNP_MASK)
	{   
		if (pWscControl->WscState != WSC_STATE_CONFIGURED)
		{
#ifdef WSC_V2_SUPPORT
			pWscControl->WscTxBufLen = 0;
			pWscControl->pWscCurBufIdx = NULL;
			pWscControl->bWscLastOne = TRUE;			
			if (pWscControl->bWscFragment && (DataLen > pWscControl->WscFragSize))			
			{
				ASSERT(DataLen < MGMT_DMA_BUFFER_SIZE);
				NdisMoveMemory(pWscControl->pWscTxBuf, WscData, DataLen);
				pWscControl->WscTxBufLen = DataLen;
				NdisZeroMemory(WscData, DataLen);
				pWscControl->bWscLastOne = FALSE;
				pWscControl->bWscFirstOne = TRUE;
				NdisMoveMemory(WscData, pWscControl->pWscTxBuf, pWscControl->WscFragSize);
				DataLen = pWscControl->WscFragSize;
				pWscControl->WscTxBufLen -= pWscControl->WscFragSize;
				pWscControl->pWscCurBufIdx = (pWscControl->pWscTxBuf + pWscControl->WscFragSize);
			}
#endif /* WSC_V2_SUPPORT */
#ifdef CONFIG_AP_SUPPORT
			if (CurOpMode == AP_MODE)
			{
				if (pEntry && IS_ENTRY_APCLI(pEntry))
					WscSendMessage(pAdapter, OpCode, WscData, DataLen, pWscControl, AP_CLIENT_MODE, EAP_CODE_RSP);
				else
					WscSendMessage(pAdapter, OpCode, WscData, DataLen, pWscControl, AP_MODE, EAP_CODE_REQ);
			}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
			if (CurOpMode == STA_MODE)
				WscSendMessage(pAdapter, OpCode, WscData, DataLen, pWscControl, STA_MODE, EAP_CODE_RSP);
#endif /* CONFIG_STA_SUPPORT */
		}
	}
	else
		bUPnPStatus = TRUE;
	
Fail:
    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapEnrolleeAction : rv = %d\n", rv));
    if (rv)
    {          
#ifdef CONFIG_AP_SUPPORT
#ifdef WSC_V2_SUPPORT
    	if ((CurOpMode == AP_MODE) && pWscControl->bSetupLock)
			rv = WSC_ERROR_SETUP_LOCKED;
#endif /* WSC_V2_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

    	if (rv <= WSC_ERROR_DEV_PWD_AUTH_FAIL)
			pWscControl->RegData.SelfInfo.ConfigError = rv;
		else if ((rv == WSC_ERROR_HASH_FAIL) || (rv == WSC_ERROR_HMAC_FAIL))
			pWscControl->RegData.SelfInfo.ConfigError = WSC_ERROR_DECRYPT_CRC_FAIL;
			
        switch(rv)
        {
            case WSC_ERROR_DEV_PWD_AUTH_FAIL:
                pWscControl->WscStatus = STATUS_WSC_ERROR_DEV_PWD_AUTH_FAIL;
#ifdef DPA_S
#ifdef P2P_SUPPORT
				if (pWscControl->EntryIfIdx >= MIN_NET_DEVICE_FOR_P2P_CLI)
					RtmpOSWrielessEventSend(pAdapter->p2p_dev, RT_WLAN_EVENT_CUSTOM, P2P_NOTIF_WPS_WRONG_PIN, NULL, NULL, 0);
#endif /* P2P_SUPPORT */
#endif /* DPA_S */
                break;
            default:
                pWscControl->WscStatus = STATUS_WSC_FAIL;
                break;
        }
#ifdef DPA_S
#ifdef P2P_SUPPORT
		if (pWscControl->EntryIfIdx >= MIN_NET_DEVICE_FOR_P2P_CLI)
			RtmpOSWrielessEventSend(pAdapter->p2p_dev, RT_WLAN_EVENT_CUSTOM, P2P_NOTIF_WPS_FAIL, NULL, NULL, 0);
		else
#endif /* P2P_SUPPORT */
#endif /* DPA_S */
		RTMPSendWirelessEvent(pAdapter, IW_WSC_STATUS_FAIL, NULL, (pWscControl->EntryIfIdx & 0x0F), 0);
		if (bUPnPMsg)
		{
			if (pWscUPnPInfo->bUPnPMsgTimerRunning == TRUE)
			{
				RTMPCancelTimer(&pWscUPnPInfo->UPnPMsgTimer, &Cancelled);
				pWscUPnPInfo->bUPnPMsgTimerRunning = FALSE;
			}
			pWscUPnPInfo->bUPnPInProgress = FALSE;
		}
		else
			WscSendNACK(pAdapter, pEntry, pWscControl);

#ifdef CONFIG_AP_SUPPORT
		if (CurOpMode == AP_MODE)
		{
#ifdef P2P_SUPPORT
			if (P2P_CLI_ON(pAdapter))
				pWscControl->WscState = WSC_STATE_WAIT_DISCONN;
			else
#endif /* P2P_SUPPORT */
			pWscControl->WscState = WSC_STATE_OFF;
		}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
		if (CurOpMode == STA_MODE)
		{
	       	pWscControl->WscState = WSC_STATE_WAIT_DISCONN;
			if (pWscControl->WscSsid.SsidLength)
			{
				pAdapter->MlmeAux.AutoReconnectSsidLen = pWscControl->WscSsid.SsidLength;
				NdisZeroMemory(&pAdapter->MlmeAux.AutoReconnectSsid[0], MAX_LEN_OF_SSID);
				NdisMoveMemory(&pAdapter->MlmeAux.AutoReconnectSsid[0], 
							   &pWscControl->WscSsid.Ssid[0], 
							   pWscControl->WscSsid.SsidLength);
			}
			else
				pAdapter->MlmeAux.AutoReconnectSsidLen = 0;
 		}
#endif /* CONFIG_STA_SUPPORT */

		/*NdisZeroMemory(pWscControl->EntryAddr, MAC_ADDR_LEN); */
        /*pWscControl->WscMode = 1; */
#ifdef WSC_NFC_SUPPORT
		pWscControl->bTriggerByNFC = FALSE;
#endif /* WSC_NFC_SUPPORT */

        bUPnPStatus = FALSE;
    }

Done:
	if(WscData)
		os_free_mem(NULL, WscData);
	if(bUPnPMsg && (bUPnPStatus == FALSE))
		WscUPnPErrHandle(pAdapter, pWscControl, Elem->TimeStamp.u.LowPart);
		
	rv = 0;

#ifdef CONFIG_AP_SUPPORT
	if  (CurOpMode == AP_MODE)
	{
		if (((bUPnPMsg || (pEntry && IS_ENTRY_CLIENT(pEntry))) 
			 && (pWscControl->WscState == WSC_STATE_CONFIGURED || pWscControl->WscState == WSC_STATE_WAIT_ACK)) 
#ifdef APCLI_SUPPORT        
			||((!bUPnPMsg && pEntry && IS_ENTRY_APCLI(pEntry)) && (pWscControl->WscState == WSC_STATE_WAIT_EAPFAIL || pWscControl->WscState == WSC_STATE_CONFIGURED))
#endif /* APCLI_SUPPORT */        
		)
		{
			rv = 1;
		}
	}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
	{
		if ((pWscControl->WscState == WSC_STATE_WAIT_EAPFAIL) ||
			(pWscControl->WscState == WSC_STATE_CONFIGURED))
		{
    		rv = 1;
		}
	}
#endif /* CONFIG_STA_SUPPORT */

	if (rv == 1)
	{
#ifdef WSC_LED_SUPPORT
		UCHAR WPSLEDStatus;
#endif /* WSC_LED_SUPPORT */
#ifdef WSC_NFC_SUPPORT
		pWscControl->bTriggerByNFC = FALSE;
#endif /* WSC_NFC_SUPPORT */
		pWscControl->bWscTrigger = FALSE;
        pWscControl->RegData.ReComputePke = 1;
		RTMPCancelTimer(&pWscControl->EapolTimer, &Cancelled);
		if (pWscControl->Wsc2MinsTimerRunning)
		{
#ifdef APCLI_SUPPORT            
            pWscControl->WscPINSsidIsSet = FALSE;
#endif            
			pWscControl->Wsc2MinsTimerRunning = FALSE;
			RTMPCancelTimer(&pWscControl->Wsc2MinsTimer, &Cancelled);
		}
#ifdef IWSC_SUPPORT
		if ((pAdapter->OpMode == OPMODE_STA) && (pAdapter->StaCfg.BssType == BSS_ADHOC))
		{
			pAdapter->StaCfg.IWscInfo.bReStart = TRUE;
			if (pAdapter->StaCfg.IWscInfo.bIWscT1TimerRunning)
			{
				pAdapter->StaCfg.IWscInfo.bIWscT1TimerRunning = FALSE;
				RTMPCancelTimer(&pAdapter->StaCfg.IWscInfo.IWscT1Timer, &Cancelled);
			}
			pAdapter->StaCfg.IWscInfo.bIWscDevQueryReqTimerRunning = TRUE;
		}
#endif /* IWSC_SUPPORT */
		if ((pWscControl->WscConfStatus == WSC_SCSTATE_UNCONFIGURED)
#ifdef CONFIG_AP_SUPPORT
			|| (pWscControl->bWCNTest == TRUE)
#ifdef WSC_V2_SUPPORT
			|| (pWscControl->WscV2Info.bEnableWpsV2 && ((CurOpMode == AP_MODE) && !pWscControl->bSetupLock))
#endif /* WSC_V2_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
			)
		{
			pWscControl->WscStatus = STATUS_WSC_CONFIGURED;
			pWscControl->WscConfStatus = WSC_SCSTATE_CONFIGURED;
			pWscControl->WscMode = 1;
	   		RTMPSendWirelessEvent(pAdapter, IW_WSC_STATUS_SUCCESS, NULL, (pWscControl->EntryIfIdx & 0x0F), 0);

#ifdef WIDI_SUPPORT
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapEnrolleeAction : WPS Successful; ifIdx = %d\n", pWscControl->EntryIfIdx));
            WidiUpdateStateToDaemon(pAdapter, MIN_NET_DEVICE_FOR_MBSSID, WIDI_MSG_TYPE_ASSOC_STATUS, pEntry->Addr, NULL, 0, WIDI_P2P_WPS_SUCCESS);
#endif /* WIDI_SUPPORT */

#ifdef P2P_SUPPORT
#ifdef DPA_S
			if (pWscControl->EntryIfIdx >= MIN_NET_DEVICE_FOR_P2P_CLI)
				RtmpOSWrielessEventSend(pAdapter->p2p_dev, RT_WLAN_EVENT_CUSTOM, P2P_NOTIF_WPS_COMPLETE, NULL, NULL, 0);
#endif /* DPA_S */
			/*RTMPCancelTimer(&pAdapter->P2pCfg.P2pWscTimer, &Cancelled);*/
			if (P2P_GO_ON(pAdapter) && (pWscControl->EntryIfIdx != BSS0) && pEntry)
			{
				UCHAR	P2pIdx = P2P_NOT_FOUND;
				P2pIdx = P2pGroupTabSearch(pAdapter, pEntry->Addr);
				if (P2pIdx != P2P_NOT_FOUND)
				{
					PRT_P2P_CLIENT_ENTRY pP2pEntry = &pAdapter->P2pTable.Client[P2pIdx];
					// Update p2p Entry's state.
					pP2pEntry->P2pClientState = P2PSTATE_CLIENT_WPS_DONE;
				}
			}

			// default set extended listening to zero for each connection. If this is persistent, will set it.
			pAdapter->P2pCfg.ExtListenInterval = 0;
			pAdapter->P2pCfg.ExtListenPeriod = 0;
			if (IS_PERSISTENT_ON(pAdapter) && pEntry && (pEntry->bP2pClient == TRUE))
			{
				UCHAR	P2pIdx = P2P_NOT_FOUND;
				P2pIdx = P2pGroupTabSearch(pAdapter, pEntry->Addr);

				if (IS_P2P_GO_ENTRY(pEntry))
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("pEntry is P2P GO.\n"));
				else if (IS_P2P_CLI_ENTRY(pEntry))
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("pEntry is P2P CLIENT.\n"));
				else
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("pEntry is P2P NONE.\n"));

				if ((P2pIdx != P2P_NOT_FOUND) && (IS_P2P_GO_ENTRY(pEntry) || IS_P2P_CLI_ENTRY(pEntry)))
				{
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("P2pWPSDone- Save to persistent entry. GrpCap= %x \n", pAdapter->P2pTable.Client[P2pIdx].GroupCapability));
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("2. P2pWPSDone-	Set Extended timing !!!!!!!\n"));
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("    ======== Profile :: Cnt = %d ========\n", pWscControl->WscProfile.ProfileCnt));
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("    SSID[%d] = %s.\n", pWscControl->WscProfile.Profile[0].SSID.SsidLength, pWscControl->WscProfile.Profile[0].SSID.Ssid));
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("    AuthType = %d.	 EncrType = %d.\n", pWscControl->WscProfile.Profile[0].AuthType, pWscControl->WscProfile.Profile[0].EncrType));
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("    MAC = %02x:%02x:%02x:%02x:%02x:%02x.\n", PRINT_MAC(pWscControl->WscProfile.Profile[0].MacAddr)));
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("    KeyLen = %d.    KeyIdx = %d.\n", pWscControl->WscProfile.Profile[0].KeyLength, pWscControl->WscProfile.Profile[0].KeyIndex));
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("    Key :: %02x %02x %02x %02x  %02x %02x %02x %02x\n", pWscControl->WscProfile.Profile[0].Key[0], pWscControl->WscProfile.Profile[0].Key[1], pWscControl->WscProfile.Profile[0].Key[2],
												pWscControl->WscProfile.Profile[0].Key[3], pWscControl->WscProfile.Profile[0].Key[4], pWscControl->WscProfile.Profile[0].Key[5], pWscControl->WscProfile.Profile[0].Key[6], pWscControl->WscProfile.Profile[0].Key[7]));
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, (" 			%02x %02x %02x %02x  %02x %02x %02x %02x\n", pWscControl->WscProfile.Profile[0].Key[8], pWscControl->WscProfile.Profile[0].Key[9], pWscControl->WscProfile.Profile[0].Key[10],
												pWscControl->WscProfile.Profile[0].Key[11], pWscControl->WscProfile.Profile[0].Key[12], pWscControl->WscProfile.Profile[0].Key[13], pWscControl->WscProfile.Profile[0].Key[14], pWscControl->WscProfile.Profile[0].Key[15]));

					P2pPerstTabInsert(pAdapter, pEntry->Addr, &pWscControl->WscProfile.Profile[0]);
					// this is a persistent connection.
					pAdapter->P2pCfg.ExtListenInterval = P2P_EXT_LISTEN_INTERVAL;
					pAdapter->P2pCfg.ExtListenPeriod = P2P_EXT_LISTEN_PERIOD;
				}
			}
#endif //P2P_SUPPORT //

#ifdef CONFIG_AP_SUPPORT
			if (CurOpMode == AP_MODE)
			{
				pWscControl->RegData.SelfInfo.ScState = pWscControl->WscConfStatus;
#ifdef APCLI_SUPPORT
				if (!bUPnPMsg && pEntry && IS_ENTRY_APCLI(pEntry))
				{
					POS_COOKIE 	pObj = (POS_COOKIE) pAdapter->OS_Cookie;
					INT			old_if_type = pObj->ioctl_if_type;
					pObj->ioctl_if_type = INT_APCLI;
#if 1
					WscWriteConfToApCliCfg(pAdapter, pWscControl, &pWscControl->WscProfile.Profile[0], TRUE);
					pObj->ioctl_if_type = old_if_type;
/*#ifdef KTHREAD_SUPPORT */
/*					WAKE_UP(&(pAdapter->wscTask)); */
/*#else */
/*					RTMP_SEM_EVENT_UP(&(pAdapter->wscTask.taskSema)); */
/*#endif */
					RtmpOsTaskWakeUp(&(pAdapter->wscTask));
#endif
				}
				else
#endif /* APCLI_SUPPORT */
				{
					RTMPSetTimer(&pWscControl->WscUpdatePortCfgTimer, 1000);
					pWscControl->WscUpdatePortCfgTimerRunning = TRUE;
				}

				if (bUPnPMsg || (pEntry && IS_ENTRY_CLIENT(pEntry)))
				{
					WscBuildBeaconIE(pAdapter, WSC_SCSTATE_CONFIGURED, FALSE, 0, 0, (pWscControl->EntryIfIdx & 0x0F), NULL, 0, CurOpMode);
					WscBuildProbeRespIE(pAdapter, WSC_MSGTYPE_AP_WLAN_MGR, WSC_SCSTATE_CONFIGURED, FALSE, 0, 0, (pWscControl->EntryIfIdx & 0x0F), NULL, 0, CurOpMode);
					APUpdateBeaconFrame(pAdapter, pWscControl->EntryIfIdx & 0x0F);
				}
			}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
			if (CurOpMode == STA_MODE)
			{
#ifdef WAC_SUPPORT
				if (pAdapter->StaCfg.EasyConfigInfo.bEnableWAC)
				{
					pAdapter->StaCfg.EasyConfigInfo.CurrentStatus = OFC_SUCCESS;
					if (pAdapter->StaCfg.EasyConfigInfo.DoAutoWAC == 0x01)
						pAdapter->StaCfg.EasyConfigInfo.DoAutoWAC = 0xFF;
				}
#endif /* WAC_SUPPORT */	
				pWscControl->WscConfMode = WSC_DISABLE;
				if (bUPnPMsg)
				{
					pWscControl->WscState = WSC_STATE_OFF;
					WscLinkDown(pAdapter);
				}
				if (pWscControl->WscDriverAutoConnect != 0)
				{
					pAdapter->StaCfg.bAutoConnectByBssid = TRUE;
					pWscControl->WscProfile.ApplyProfileIdx = 0;  /* add by johnli, fix WPS test plan 5.1.1 */
#ifdef DPA_T
					pCredential = &pWscControl->WscProfile.Profile[0];
					if (!((pCredential->AuthType == WSC_AUTHTYPE_OPEN) &&
						(pCredential->EncrType == WSC_ENCRTYPE_NONE)))
					{
						pAdapter->StaCfg.bAutoConnectByBssid = FALSE;
						RTMPZeroMemory(pAdapter->MlmeAux.AutoReconnectSsid, MAX_LEN_OF_SSID);
						pAdapter->MlmeAux.AutoReconnectSsidLen = pAdapter->CommonCfg.SsidLen;
						RTMPMoveMemory(pAdapter->MlmeAux.AutoReconnectSsid, pAdapter->CommonCfg.Ssid, pAdapter->CommonCfg.SsidLen);
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapEnrolleeAction : Done but AuthType == WSC_AUTHTYPE_OPEN EncrType == WSC_ENCRTYPE_NONE\n"));
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE,("pAd->MlmeAux.Ssid=%s\n",pAdapter->MlmeAux.Ssid));
					}
					else
#endif /* DPA_T */
					{
						WscWriteConfToPortCfg(pAdapter, pWscControl, &pWscControl->WscProfile.Profile[0], TRUE);
						pAdapter->WriteWscCfgToDatFile = (pWscControl->EntryIfIdx & 0x0F);

						RtmpOsTaskWakeUp(&(pAdapter->wscTask));
					}
#ifdef IWSC_SUPPORT
				if ((pAdapter->StaCfg.BssType == BSS_ADHOC)&&
					(pAdapter->StaCfg.IWscInfo.bIWscDevQueryReqTimerRunning == FALSE))
				{
					pAdapter->StaCfg.IWscInfo.bIWscDevQueryReqTimerRunning = TRUE;
					RTMPSetTimer(&pAdapter->StaCfg.IWscInfo.IWscDevQueryTimer, 200);
				}
#endif /* IWSC_SUPPORT */
				}
			}
#endif /* CONFIG_STA_SUPPORT */

		}
#ifdef WSC_LED_SUPPORT
		/* The protocol is finished. */
		WPSLEDStatus = LED_WPS_SUCCESS;
		RTMPSetLED(pAdapter, WPSLEDStatus);
#endif /* WSC_LED_SUPPORT */
	}
}

#ifdef CONFIG_AP_SUPPORT
/*
	============================================================================
	Proxy			Proxy			Proxy			
	============================================================================	
*/	
VOID WscEapApProxyAction(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	MLME_QUEUE_ELEM	*Elem,
	IN  UCHAR	        MsgType,
	IN  MAC_TABLE_ENTRY *pEntry,
	IN  PWSC_CTRL       pWscControl)
{
	PUCHAR  WscData = NULL;
	BOOLEAN sendToUPnP = FALSE, bUPnPStatus = FALSE, Cancelled;
	int reqID = 0;
    WSC_UPNP_NODE_INFO *pWscUPnPInfo = &pWscControl->WscUPnPNodeInfo;
	UINT	MaxWscDataLen = WSC_MAX_DATA_LEN;
		
    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapApProxyAction Enter!\n"));

	if (Elem->MsgType == WSC_EAPOL_UPNP_MSG)
	{	
		reqID = Elem->TimeStamp.u.LowPart;
		if(reqID > 0)
			sendToUPnP = TRUE;
	}

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapApProxyAction():pEntry=%p, ElemMsgType=%ld, MsgType=%d!\n", pEntry, Elem->MsgType, MsgType));
 
	if ((pWscControl->WscActionMode != WSC_PROXY) || 
	   ((Elem->MsgType == WSC_EAPOL_PACKET_MSG) && (pEntry == NULL)))
	{	
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("EarlyCheckFailed: gWscActionMode=%d, pEntry=%p!\n", pWscControl->WscActionMode, pEntry));
		goto Fail;
	}

#ifdef WSC_V2_SUPPORT 
	MaxWscDataLen = MaxWscDataLen + (UINT)pWscControl->WscV2Info.ExtraTlv.TlvLen;
#endif /* WSC_V2_SUPPORT */
	os_alloc_mem(NULL, (UCHAR **)&WscData, MaxWscDataLen);
	if (WscData == NULL)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscData Allocate failed!\n"));
		goto Fail;
	}
	NdisZeroMemory(WscData, MaxWscDataLen);

    /* Base on state doing the Msg, State change diagram */
    if (Elem->MsgType == WSC_EAPOL_UPNP_MSG)
    {	/* WSC message send from UPnP. */
		switch (MsgType)
		{
			case WSC_MSG_M2:
			case WSC_MSG_M4:
			case WSC_MSG_M6:
			case WSC_MSG_M8:
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapApProxyAction: Rx WscMsg(%d) from UPnP, eventID=0x%x!\n", MsgType, reqID));
				WscSendMessage(pAdapter, WSC_OPCODE_MSG, Elem->Msg, Elem->MsgLen, pWscControl, AP_MODE, EAP_CODE_REQ);

				/*Notify the UPnP daemon which remote registar is negotiating with enrollee. */
				if (MsgType == WSC_MSG_M2)
				{
					pWscUPnPInfo->registrarID = Elem->TimeStamp.u.HighPart;
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s():registrarID=0x%x!\n", __FUNCTION__, pWscUPnPInfo->registrarID));
					bUPnPStatus = WscSendUPnPMessage(pAdapter, (pWscControl->EntryIfIdx & 0x0F), 
														WSC_OPCODE_UPNP_MGMT, WSC_UPNP_MGMT_SUB_REG_SELECT, 
														(PUCHAR)(&pWscUPnPInfo->registrarID), sizeof(UINT), 0, 0, NULL, AP_MODE);
					
					/*Reset the UPnP timer and status. */
					if (pWscControl->bM2DTimerRunning == TRUE)
					{
						RTMPCancelTimer(&pWscControl->M2DTimer, &Cancelled);
						pWscControl->bM2DTimerRunning = FALSE;
					}
					pWscControl->M2DACKBalance = 0;
					pWscUPnPInfo->registrarID = 0;
				}
				if (MsgType == WSC_MSG_M8)
				{
					WscBuildBeaconIE(pAdapter, WSC_SCSTATE_CONFIGURED, FALSE, 0, 0, (pWscControl->EntryIfIdx & 0x0F), NULL, 0, AP_MODE);
					WscBuildProbeRespIE(pAdapter, WSC_MSGTYPE_AP_WLAN_MGR, WSC_SCSTATE_CONFIGURED, FALSE, 0, 0, pWscControl->EntryIfIdx, NULL, 0, AP_MODE);
					APUpdateBeaconFrame(pAdapter, pWscControl->EntryIfIdx & 0x0F);
				}
				break;
				
			case WSC_MSG_M2D:
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapApProxyAction: Rx WscMsg M2D(%d) from UPnP, eventID=0x%x!\n", MsgType, reqID));

				/*If it's send by UPnP Action, response ok directly to remote UPnP Control Point! */
				if (reqID > 0)
					bUPnPStatus = WscSendUPnPMessage(pAdapter, (pWscControl->EntryIfIdx & 0x0F), 
														WSC_OPCODE_UPNP_DATA, WSC_UPNP_DATA_SUB_ACK, 
														0, 0, reqID, 0, NULL, AP_MODE);

				/*Send M2D to wireless station. */
				WscSendMessage(pAdapter, WSC_OPCODE_MSG, Elem->Msg, Elem->MsgLen, pWscControl, AP_MODE, EAP_CODE_REQ);
				pWscControl->M2DACKBalance++;
				if ((pWscUPnPInfo->registrarID == 0) && (pWscControl->bM2DTimerRunning == FALSE))
				{
					/* Add M2D timer used to trigger the EAPFail Packet! */
					RTMPSetTimer(&pWscControl->M2DTimer, WSC_UPNP_M2D_TIME_OUT);
					pWscControl->bM2DTimerRunning = TRUE;
				}
				break;
				
			case WSC_MSG_WSC_NACK:
			default:
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Recv WscMsg(%d) from UPnP, request EventID=%d! drop it!\n", MsgType, reqID));
				break;
		}
    }
	else	
	{	/*WSC msg send from EAP. */
		switch (MsgType)
		{
	        case WSC_MSG_M1:
			case WSC_MSG_M3:
			case WSC_MSG_M5:
			case WSC_MSG_M7:
	            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapApProxyAction: Rx WscMsg(%d) from EAP\n", MsgType));
				/*This msg send to event-based external registrar */
				if (MsgType == WSC_MSG_M1)
                {            
					bUPnPStatus = WscSendUPnPMessage(pAdapter, (pWscControl->EntryIfIdx & 0x0F), 
														WSC_OPCODE_UPNP_DATA, WSC_UPNP_DATA_SUB_TO_ALL, 
														Elem->Msg, Elem->MsgLen, 0, 0, &pWscControl->EntryAddr[0], AP_MODE);
                    pWscControl->WscState = WSC_STATE_SENT_M1;
                }
				else
					bUPnPStatus = WscSendUPnPMessage(pAdapter, (pWscControl->EntryIfIdx & 0x0F), 
														WSC_OPCODE_UPNP_DATA, WSC_UPNP_DATA_SUB_TO_ALL, 
														Elem->Msg, Elem->MsgLen, 0, pWscUPnPInfo->registrarID, 
														&pWscControl->EntryAddr[0], AP_MODE);
				
				break;
				
			case WSC_MSG_WSC_ACK:
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapApProxyAction: Rx WSC_ACK from EAP\n"));

				/* The M2D must appeared before the ACK, so we just need sub it when (pWscUPnPInfo->M2DACKBalance > 0) */
				if (pWscControl->M2DACKBalance > 0)
					pWscControl->M2DACKBalance--;
				break;
				
			case WSC_MSG_WSC_DONE:
	            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapApProxyAction: Rx WSC_DONE from EAP\n"));
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapApProxyAction: send WSC_DONE to UPnP Registrar!\n"));
				/*Send msg to event-based external registrar */
				bUPnPStatus = WscSendUPnPMessage(pAdapter, (pWscControl->EntryIfIdx & 0x0F), 
													WSC_OPCODE_UPNP_DATA, WSC_UPNP_DATA_SUB_TO_ONE, 
													Elem->Msg, Elem->MsgLen, 0, 
													pWscUPnPInfo->registrarID, &pWscControl->EntryAddr[0], AP_MODE);

				/*Send EAPFail to wireless station to finish the whole process. */
				WscSendEapFail(pAdapter, pWscControl, FALSE);
                
                RTMPCancelTimer(&pWscControl->EapolTimer, &Cancelled);
                pWscControl->EapolTimerRunning = FALSE;

                pEntry->bWscCapable = FALSE;
                pWscControl->EapMsgRunning = FALSE;
                NdisZeroMemory(pWscControl->EntryAddr, MAC_ADDR_LEN);

                if (pWscControl->Wsc2MinsTimerRunning)
            	{
            		pWscControl->Wsc2MinsTimerRunning = FALSE;
            		RTMPCancelTimer(&pWscControl->Wsc2MinsTimer, &Cancelled);
            	}			
	            break;
				
			default:
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Recv WSC Msg(%d) from EAP , it's impossible, drop it!\n", MsgType));
				break;
		}
	}

Fail:
	if (WscData)
		os_free_mem(NULL, WscData);
	if (sendToUPnP && (bUPnPStatus == FALSE))
    {
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Need to send UPnP but bUPnPStatus is false!MsgType=%d, regID=0x%x!\n", MsgType, reqID));
		WscUPnPErrHandle(pAdapter, pWscControl, reqID);
	}
		
}
#endif /* CONFIG_AP_SUPPORT */

/*
	============================================================================
	Registrar			Registrar			Registrar			
	============================================================================	
*/	
VOID WscEapRegistrarAction(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	MLME_QUEUE_ELEM	*Elem,
	IN  UCHAR	        MsgType,
	IN  MAC_TABLE_ENTRY *pEntry,
	IN  PWSC_CTRL       pWscControl)
{
	INT     DataLen = 0, rv = 0;
	UCHAR   OpCode = 0;
	UCHAR   *WscData = NULL;    
	BOOLEAN bUPnPMsg, bUPnPStatus = FALSE, Cancelled;
	WSC_UPNP_NODE_INFO *pWscUPnPInfo = &pWscControl->WscUPnPNodeInfo;
	UINT	MaxWscDataLen = WSC_MAX_DATA_LEN;
	UCHAR	CurOpMode = 0xFF;
#ifdef P2P_SUPPORT
		BOOLEAN bReadOwnPIN = FALSE;
#endif /* P2P_SUPPORT */
#ifdef DPA_T
	PWSC_CREDENTIAL pCredential;
#endif /* DPA_T */
	
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapRegistrarAction Enter!\n"));

#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAdapter)
			CurOpMode = AP_MODE;
#endif // CONFIG_AP_SUPPORT //
	
#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAdapter)
			CurOpMode = STA_MODE;
#ifdef P2P_SUPPORT
		if (Elem->OpMode != OPMODE_STA)
			CurOpMode = AP_MODE;
#endif /* P2P_SUPPORT */
#endif // CONFIG_STA_SUPPORT //

	bUPnPMsg = Elem->MsgType == WSC_EAPOL_UPNP_MSG ? TRUE : FALSE;

	if(bUPnPMsg)
	{
		if(MsgType == WSC_MSG_M1)
		{	/* It's a M1 message, we may need to initialize our state machine. */
			if ((pWscControl->WscActionMode == WSC_REGISTRAR) 
				&& (pWscControl->EntryIfIdx == WSC_INIT_ENTRY_APIDX)
				&& (pWscControl->WscState < WSC_STATE_WAIT_M1)
				&& (pWscUPnPInfo->bUPnPInProgress == FALSE))
			{
				pWscUPnPInfo->bUPnPInProgress = TRUE;
				/*Set the WscState as "WSC_STATE_WAIT_RESP_ID" because UPnP start from this state. */
				pWscControl->WscState = WSC_STATE_WAIT_M1;
				RTMPSetTimer(&pWscUPnPInfo->UPnPMsgTimer, WSC_UPNP_MSG_TIME_OUT);
				pWscUPnPInfo->bUPnPMsgTimerRunning = TRUE;
			}
		}
		OpCode = WSC_OPCODE_UPNP_MASK;
		
	} else {
	    if (pWscControl->EapolTimerRunning)
		pWscControl->EapolTimerRunning = FALSE;

	}

#ifdef WSC_V2_SUPPORT 
	MaxWscDataLen = MaxWscDataLen + (UINT)pWscControl->WscV2Info.ExtraTlv.TlvLen;
#endif /* WSC_V2_SUPPORT */
	os_alloc_mem(NULL, (UCHAR **)&WscData, MaxWscDataLen);
	if (WscData == NULL)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscData Allocate failed!\n"));
		goto Fail;
	}
	NdisZeroMemory(WscData, MaxWscDataLen);
	
	/* Base on state doing the Msg, State change diagram */
	switch (MsgType)
	{
		case WSC_MSG_M1:
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapRegistrarAction : Rx M1\n"));
            
			/* Receive M1, if we are at WSC_STATE_WAIT_M1 start, process it immediately */
			pWscControl->WscStatus = STATUS_WSC_EAP_M1_RECEIVED;
			if (pWscControl->WscState == WSC_STATE_WAIT_M1)
			{
				OpCode |= WSC_OPCODE_MSG;

				/* Process M1 */
				if ((rv=ProcessMessageM1(pAdapter, pWscControl, Elem->Msg, Elem->MsgLen, &pWscControl->RegData)))
					goto Fail;
				else
				{
					BOOLEAN	bSendM2D = TRUE;
#ifdef P2P_SUPPORT
					/*
						If own UI is limited UI, we need to use own PIN not PIN of Enrollee.
					*/
					if (P2P_GO_ON(pAdapter) && (pWscControl->EntryIfIdx != BSS0))
					{
						if ((pAdapter->P2pCfg.ConfigMethod & WSC_CONFMET_KEYPAD) == 0)							
							bReadOwnPIN = TRUE;
					}
					if (bReadOwnPIN)
					{
						pWscControl->WscPinCodeLen = pWscControl->WscEnrolleePinCodeLen;
						WscGetRegDataPIN(pAdapter, pWscControl->WscEnrolleePinCode, pWscControl);
					}
#endif /* P2P_SUPPORT */

#ifdef EASY_CONFIG_SETUP
					if (pEntry && pEntry->bRaAutoWpsCapable)
						bSendM2D = FALSE;
#endif /* EASY_CONFIG_SETUP */

					if (pWscControl->bWscTrigger && (!pWscControl->bWscAutoTigeer))
					{
						if (((pWscControl->WscMode == WSC_PBC_MODE) || (pWscControl->WscMode == WSC_SMPBC_MODE))
							|| (pWscControl->WscMode == WSC_PIN_MODE && pWscControl->WscPinCode != 0))
							bSendM2D = FALSE;
					}
					
#ifdef WSC_NFC_SUPPORT					
					if (pWscControl->bTriggerByNFC)
					{
						bSendM2D = FALSE;
						if  (pWscControl->RegData.SelfInfo.ConfigError == WSC_ERROR_PUBLIC_KEY_HASH_MISMATCH)
						{
							MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WSC_ERROR_PUBLIC_KEY_HASH_MISMATCH  ConfigError=%d \n", pWscControl->RegData.SelfInfo.ConfigError));
							bSendM2D = TRUE;
						}	
					}							
#endif /* WSC_NFC_SUPPORT */
					
					if (bSendM2D)
					{
						DataLen = BuildMessageM2D(pAdapter, pWscControl, WscData);
						pWscControl->WscState = WSC_STATE_SENT_M2D;
						pWscControl->M2DACKBalance++;
						if (pWscControl->bM2DTimerRunning == FALSE)
						{
							// Add M2D timer used to trigger the EAPFail Packet!
							RTMPSetTimer(&pWscControl->M2DTimer, WSC_UPNP_M2D_TIME_OUT);
							pWscControl->bM2DTimerRunning = TRUE;
						}
					}
					else
					{
						pWscControl->WscStatus = STATUS_WSC_EAP_M2_SENT;
						DataLen = BuildMessageM2(pAdapter, pWscControl, WscData);
						
						/* Change the state to next one */
#ifdef WSC_NFC_SUPPORT
						if (pWscControl->bTriggerByNFC && pWscControl->NfcModel == MODEL_HANDOVER)
						{
							pWscControl->WscState = WSC_STATE_WAIT_DONE; /* Skip M3~M8 */								
						}	
						else
#endif /* WSC_NFC_SUPPORT */
						pWscControl->WscState = WSC_STATE_WAIT_M3;
#ifdef CONFIG_STA_SUPPORT
						if ((CurOpMode == STA_MODE) && INFRA_ON(pAdapter))
						{
							if (!bUPnPMsg)
								pWscControl->WscConfStatus = pWscControl->bConfiguredAP ? WSC_SCSTATE_UNCONFIGURED : WSC_SCSTATE_CONFIGURED;
						}
#endif /* CONFIG_STA_SUPPORT */
		 				RTMPSendWirelessEvent(pAdapter, IW_WSC_SEND_M2, NULL, (pWscControl->EntryIfIdx & 0x0F), 0);
					}
				}
			}
			break;

		case WSC_MSG_M3:
			/* Receive M3 */
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapRegistrarAction : Rx M3\n"));
			if (pWscControl->WscState == WSC_STATE_WAIT_M3)
			{
				pWscControl->WscStatus = STATUS_WSC_EAP_M3_RECEIVED;

				if((rv = ProcessMessageM3(pAdapter, Elem->Msg, Elem->MsgLen, &pWscControl->RegData)))
					goto Fail;
				else
				{
					OpCode |= WSC_OPCODE_MSG;
					DataLen = BuildMessageM4(pAdapter, pWscControl, WscData);
					pWscControl->WscStatus = STATUS_WSC_EAP_M4_SENT;
					/* Change the state to next one */
					pWscControl->WscState = WSC_STATE_WAIT_M5;
	 				RTMPSendWirelessEvent(pAdapter, IW_WSC_SEND_M4, NULL, (pWscControl->EntryIfIdx & 0x0F), 0);
				}
			}
			break;

		case WSC_MSG_M5:
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapRegistrarAction : Rx M5\n"));
			if (pWscControl->WscState == WSC_STATE_WAIT_M5)
			{
				pWscControl->WscStatus = STATUS_WSC_EAP_M5_RECEIVED;

				if ((rv=ProcessMessageM5(pAdapter, pWscControl, Elem->Msg, Elem->MsgLen, &pWscControl->RegData)))
					goto Fail;
				else
				{
					OpCode |= WSC_OPCODE_MSG;
					DataLen = BuildMessageM6(pAdapter, pWscControl, WscData);
					pWscControl->WscStatus = STATUS_WSC_EAP_M6_SENT;
					/* Change the state to next one */
					pWscControl->WscState = WSC_STATE_WAIT_M7;
	 				RTMPSendWirelessEvent(pAdapter, IW_WSC_SEND_M6, NULL, (pWscControl->EntryIfIdx & 0x0F), 0);
				}
			}
			break;
		case WSC_MSG_M7:
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapRegistrarAction : Rx M7\n"));
			if (pWscControl->WscState == WSC_STATE_WAIT_M7)
			{
				pWscControl->WscStatus = STATUS_WSC_EAP_M7_RECEIVED;
				if ((rv=ProcessMessageM7(pAdapter, pWscControl, Elem->Msg, Elem->MsgLen, &pWscControl->RegData)))
					goto Fail;
				else
				{
					if (
#ifdef CONFIG_AP_SUPPORT                        
						(CurOpMode == AP_MODE) || 
#endif /* CONFIG_AP_SUPPORT */   
#ifdef CONFIG_STA_SUPPORT
						((CurOpMode == STA_MODE) && ((pWscControl->bConfiguredAP == FALSE)
#ifdef WSC_V2_SUPPORT
                            /* 
                            	Check AP is v2 or v1, Check WscV2 Enabled or not
                            */
							|| (pWscControl->WscV2Info.bForceSetAP 
								&& pWscControl->WscV2Info.bEnableWpsV2 
								&& (pWscControl->RegData.PeerInfo.Version2!= 0))
#endif /* WSC_V2_SUPPORT */
						)) ||
#endif /* CONFIG_STA_SUPPORT */
						(0))
					{
						OpCode |= WSC_OPCODE_MSG;
#ifdef IWSC_SUPPORT
						if ((pAdapter->OpMode == OPMODE_STA) && (pAdapter->StaCfg.BssType == BSS_ADHOC))
							pWscControl->WscConfStatus = WSC_SCSTATE_CONFIGURED;
#endif /* IWSC_SUPPORT */
						DataLen = BuildMessageM8(pAdapter, pWscControl, WscData);
						pWscControl->WscStatus = STATUS_WSC_EAP_M8_SENT;
						/* Change the state to next one */
						pWscControl->WscState = WSC_STATE_WAIT_DONE;
		 				RTMPSendWirelessEvent(pAdapter, IW_WSC_SEND_M8, NULL, (pWscControl->EntryIfIdx & 0x0F), 0);
#ifdef CONFIG_AP_SUPPORT
#ifdef WSC_V2_SUPPORT

						if (pWscControl->WscV2Info.bEnableWpsV2 && (CurOpMode == AP_MODE))
							WscAddEntryToAclList(pAdapter, pEntry->func_tb_idx, pEntry->Addr);
#endif /* WSC_V2_SUPPORT */
						/*
							1. Complete WPS with this STA. Delete it from WscPeerList for others STA to do WSC with AP
							2. Some WPS STA will send dis-assoc close to WSC_DONE 
							   then AP will miss WSC_DONE from STA; hence we need to call WscDelListEntryByMAC here.
						*/
						if (pEntry && (CurOpMode == AP_MODE))
						{
							RTMP_SEM_LOCK(&pWscControl->WscPeerListSemLock);
							WscDelListEntryByMAC(&pWscControl->WscPeerList, pEntry->Addr);
							RTMP_SEM_UNLOCK(&pWscControl->WscPeerListSemLock);
						}
#endif /* CONFIG_AP_SUPPORT */

					}
#ifdef CONFIG_STA_SUPPORT
					else if ((CurOpMode == STA_MODE) && 
						(pWscControl->bConfiguredAP == TRUE))
					{
						/* Some WPS AP expects to receive WSC_NACK when AP is configured */
						OpCode |= WSC_OPCODE_NACK;
						DataLen = BuildMessageNACK(pAdapter, pWscControl, WscData);
						pWscControl->WscStatus = STATUS_WSC_CONFIGURED;
						pWscControl->WscState = WSC_STATE_CONFIGURED;
						pWscControl->EapMsgRunning = FALSE;
		 				RTMPSendWirelessEvent(pAdapter, IW_WSC_SEND_NACK, NULL, (pWscControl->EntryIfIdx & 0x0F), 0);
					}
#endif /* CONFIG_STA_SUPPORT */
				}
			}
			break;

		case WSC_MSG_WSC_DONE:
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapRegistrarAction : Rx DONE\n"));
			if (pWscControl->WscState == WSC_STATE_WAIT_DONE)
			{
#ifdef CONFIG_AP_SUPPORT
				if (CurOpMode == AP_MODE)
				{
#ifdef WAC_SUPPORT
					if (pEntry && pAdapter->ApCfg.MBSSID[pWscControl->EntryIfIdx & 0x0F].EasyConfigInfo.bEnableWAC)
					{
						RTMP_SEM_LOCK(&pAdapter->ApCfg.MBSSID[pWscControl->EntryIfIdx & 0x0F].EasyConfigInfo.WAC_PeerListSemLock);
						WscDelListEntryByMAC(&pAdapter->ApCfg.MBSSID[pWscControl->EntryIfIdx & 0x0F].EasyConfigInfo.WAC_PeerList, 
											 pEntry->Addr);
						RTMP_SEM_UNLOCK(&pAdapter->ApCfg.MBSSID[pWscControl->EntryIfIdx & 0x0F].EasyConfigInfo.WAC_PeerListSemLock);
					}
#endif // WAC_SUPPORT //
					pWscControl->WscStatus = STATUS_WSC_EAP_RAP_RSP_DONE_SENT;
					/* Send EAP-Fail */
					WscSendEapFail(pAdapter, pWscControl, FALSE);
					pWscControl->WscStatus = STATUS_WSC_CONFIGURED;
#ifdef P2P_SUPPORT
					if (P2P_GO_ON(pAdapter) && 
						(pWscControl->WscConfStatus == WSC_SCSTATE_UNCONFIGURED) &&
						(pWscControl == &pAdapter->ApCfg.MBSSID[MAIN_MBSSID].WscControl))
						pAdapter->P2pCfg.bStopAuthRsp = TRUE;
#endif /* P2P_SUPPORT */

#ifdef CON_WPS
					/* AP: stop the other side of band */
					if (pAdapter->conWscStatus != CON_WPS_STATUS_DISABLED)
					{
						WscConWpsStop(pAdapter, FALSE, pWscControl);
						pAdapter->conWscStatus = CON_WPS_STATUS_DISABLED;
					}
#endif /* CON_WPS */
				}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
				if (CurOpMode == STA_MODE)
				{
					if ((CurOpMode == STA_MODE) && ADHOC_ON(pAdapter))
					{
						WscSendEapFail(pAdapter, pWscControl, FALSE);
					}
					else
					{
						OpCode |= WSC_OPCODE_ACK;
						DataLen = BuildMessageACK(pAdapter, pWscControl, WscData);
		 				RTMPSendWirelessEvent(pAdapter, IW_WSC_SEND_ACK, NULL, (pWscControl->EntryIfIdx & 0x0F), 0);
					}
#ifdef IWSC_SUPPORT
					if ((pAdapter->StaCfg.BssType == BSS_ADHOC) &&
						(pWscControl->WscMode == WSC_SMPBC_MODE))
					{
						pAdapter->StaCfg.IWscInfo.IWscSmpbcAcceptCount--;
						pWscControl->WscStatus = STATUS_WSC_CONFIGURED;
						
						if (pEntry && pEntry->bIWscSmpbcAccept)
						{
							pEntry->bIWscSmpbcAccept = FALSE;							
							WscDelListEntryByMAC(&pWscControl->WscPeerList, pEntry->Addr);
						}
						pAdapter->StaCfg.IWscInfo.SmpbcEnrolleeCount++;
					}
					else
#endif /* IWSC_SUPPORT */
					pWscControl->WscStatus = STATUS_WSC_CONFIGURED;
				}
#endif /* CONFIG_STA_SUPPORT */

				pWscControl->WscState = WSC_STATE_CONFIGURED;
				RTMPSendWirelessEvent(pAdapter, IW_WSC_STATUS_SUCCESS, NULL, (pWscControl->EntryIfIdx & 0x0F), 0);
#ifdef P2P_SUPPORT
#ifdef DPA_S
				if (pWscControl->EntryIfIdx >= MIN_NET_DEVICE_FOR_P2P_CLI)
					RtmpOSWrielessEventSend(pAdapter->p2p_dev, RT_WLAN_EVENT_CUSTOM, P2P_NOTIF_WPS_COMPLETE, NULL, NULL, 0);
#endif /* DPA_S */
				/*RTMPCancelTimer(&pAdapter->P2pCfg.P2pWscTimer, &Cancelled);*/
				if (P2P_GO_ON(pAdapter) && pEntry && pWscControl->EntryIfIdx != BSS0)
				{
					UCHAR	P2pIdx = P2P_NOT_FOUND;
					P2pIdx = P2pGroupTabSearch(pAdapter, pEntry->Addr);
					if (P2pIdx != P2P_NOT_FOUND)
					{
						PRT_P2P_CLIENT_ENTRY pP2pEntry = &pAdapter->P2pTable.Client[P2pIdx];
						// Update p2p Entry's state.
						pP2pEntry->P2pClientState = P2PSTATE_CLIENT_WPS_DONE;
					}
				}

				// default set extended listening to zero for each connection. If this is persistent, will set it.
				pAdapter->P2pCfg.ExtListenInterval = 0;
				pAdapter->P2pCfg.ExtListenPeriod = 0;
				pAdapter->P2pCfg.WscState = WSC_STATE_CONFIGURED;
#ifdef RT_P2P_SPECIFIC_WIRELESS_EVENT
				P2pSendWirelessEvent(pAdapter, RT_P2P_WPS_COMPLETED, NULL, NULL);
#endif /* RT_P2P_SPECIFIC_WIRELESS_EVENT */
				if (IS_PERSISTENT_ON(pAdapter) && pEntry && (pEntry->bP2pClient == TRUE))
				{
					UCHAR	P2pIdx = P2P_NOT_FOUND;
					P2pIdx = P2pGroupTabSearch(pAdapter, pEntry->Addr);

					if (IS_P2P_GO_ENTRY(pEntry))
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("pEntry is P2P GO.\n"));
					else if (IS_P2P_CLI_ENTRY(pEntry))
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("pEntry is P2P CLIENT.\n"));
					else
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("pEntry is P2P NONE[%d].\n", pEntry->EntryType));

					if ((P2pIdx != P2P_NOT_FOUND) && (IS_P2P_GO_ENTRY(pEntry) || IS_P2P_CLI_ENTRY(pEntry)))
					{
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("P2pWPSDone- Save to persistent entry. GrpCap= %x \n", pAdapter->P2pTable.Client[P2pIdx].GroupCapability));
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("1. P2pWPSDone-	Set Extended timing !!!!!!!\n"));
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("    ======== Profile :: Cnt = %d ========\n", pWscControl->WscProfile.ProfileCnt));
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("    SSID[%d] = %s.\n", pWscControl->WscProfile.Profile[0].SSID.SsidLength, pWscControl->WscProfile.Profile[0].SSID.Ssid));
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("    AuthType = %x.    EncrType = %x.\n", pWscControl->WscProfile.Profile[0].AuthType, pWscControl->WscProfile.Profile[0].EncrType));
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("    MAC = %02x:%02x:%02x:%02x:%02x:%02x.\n", PRINT_MAC(pWscControl->WscProfile.Profile[0].MacAddr)));
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("    KeyLen = %d.    KeyIdx = %d.\n", pWscControl->WscProfile.Profile[0].KeyLength, pWscControl->WscProfile.Profile[0].KeyIndex));
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("    Key :: %02x %02x %02x %02x  %02x %02x %02x %02x\n", pWscControl->WscProfile.Profile[0].Key[0], pWscControl->WscProfile.Profile[0].Key[1], pWscControl->WscProfile.Profile[0].Key[2],
													pWscControl->WscProfile.Profile[0].Key[3], pWscControl->WscProfile.Profile[0].Key[4], pWscControl->WscProfile.Profile[0].Key[5], pWscControl->WscProfile.Profile[0].Key[6], pWscControl->WscProfile.Profile[0].Key[7]));
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("             %02x %02x %02x %02x  %02x %02x %02x %02x\n", pWscControl->WscProfile.Profile[0].Key[8], pWscControl->WscProfile.Profile[0].Key[9], pWscControl->WscProfile.Profile[0].Key[10],
													pWscControl->WscProfile.Profile[0].Key[11], pWscControl->WscProfile.Profile[0].Key[12], pWscControl->WscProfile.Profile[0].Key[13], pWscControl->WscProfile.Profile[0].Key[14], pWscControl->WscProfile.Profile[0].Key[15]));
				
						P2pPerstTabInsert(pAdapter, pEntry->Addr, &pWscControl->WscProfile.Profile[0]);
						// this is a persistent connection.
						pAdapter->P2pCfg.ExtListenInterval = P2P_EXT_LISTEN_INTERVAL;
						pAdapter->P2pCfg.ExtListenPeriod = P2P_EXT_LISTEN_PERIOD;
					}
				}
#endif //P2P_SUPPORT //	
				pWscControl->EapMsgRunning = FALSE;
			}
			break;
		default:
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapRegistrarAction : Unsupported Msg Type\n"));
			if (WscData)
				os_free_mem(NULL, WscData);
			return;
	}

	if(OpCode > WSC_OPCODE_UPNP_MASK)
		bUPnPStatus = WscSendUPnPMessage(pAdapter, (pWscControl->EntryIfIdx & 0x0F), 
											WSC_OPCODE_UPNP_DATA, WSC_UPNP_DATA_SUB_NORMAL, 
											WscData, DataLen, 
											Elem->TimeStamp.u.LowPart, Elem->TimeStamp.u.HighPart, &pWscControl->EntryAddr[0], CurOpMode);
	else if(OpCode > 0 && OpCode < WSC_OPCODE_UPNP_MASK)
	{
#ifdef WSC_V2_SUPPORT
		pWscControl->WscTxBufLen = 0;
		pWscControl->pWscCurBufIdx = NULL;
		pWscControl->bWscLastOne = TRUE;
		if (pWscControl->bWscFragment && (DataLen > pWscControl->WscFragSize))			
		{
			ASSERT(DataLen < MGMT_DMA_BUFFER_SIZE);
			NdisMoveMemory(pWscControl->pWscTxBuf, WscData, DataLen);
			pWscControl->WscTxBufLen = DataLen;
			NdisZeroMemory(WscData, DataLen);
			pWscControl->bWscLastOne = FALSE;
			pWscControl->bWscFirstOne = TRUE;
			NdisMoveMemory(WscData, pWscControl->pWscTxBuf, pWscControl->WscFragSize);
			DataLen = pWscControl->WscFragSize;
			pWscControl->WscTxBufLen -= pWscControl->WscFragSize;
			pWscControl->pWscCurBufIdx = (pWscControl->pWscTxBuf + pWscControl->WscFragSize);
		}
#endif /* WSC_V2_SUPPORT */
#ifdef CONFIG_AP_SUPPORT
		if (CurOpMode == AP_MODE)
		{
			if (pWscControl->WscState != WSC_STATE_CONFIGURED)
				WscSendMessage(pAdapter, OpCode, WscData, DataLen, pWscControl, AP_MODE, EAP_CODE_REQ);
		}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
		if (CurOpMode == STA_MODE)
		{
			if (ADHOC_ON(pAdapter))
				WscSendMessage(pAdapter, OpCode, WscData, DataLen, pWscControl, STA_MODE, EAP_CODE_REQ);
			else
				WscSendMessage(pAdapter, OpCode, WscData, DataLen, pWscControl, STA_MODE, EAP_CODE_RSP);
		}
#endif /* CONFIG_STA_SUPPORT */
		    
	}
	else
		bUPnPStatus = TRUE;

	if(bUPnPMsg)
	{
		if(pWscControl->WscState == WSC_STATE_SENT_M2D)
		{	/*After M2D, reset the status of State Machine. */
			pWscControl->WscState = WSC_STATE_WAIT_UPNP_START;
			pWscUPnPInfo->bUPnPInProgress = FALSE;
		}
	}
Fail:
    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapRegistrarAction : rv = %d\n", rv));
    if (rv)
    {        
    	if (rv <= WSC_ERROR_DEV_PWD_AUTH_FAIL)
    	{
			pWscControl->RegData.SelfInfo.ConfigError = rv;
#ifdef DPA_S
#ifdef P2P_SUPPORT
			if (pWscControl->EntryIfIdx >= MIN_NET_DEVICE_FOR_P2P_CLI)
				RtmpOSWrielessEventSend(pAdapter->p2p_dev, RT_WLAN_EVENT_CUSTOM, P2P_NOTIF_WPS_WRONG_PIN, NULL, NULL, 0);
#endif /* P2P_SUPPORT */
#endif /* DPA_S */
    	}
		else if ((rv == WSC_ERROR_HASH_FAIL) || (rv == WSC_ERROR_HMAC_FAIL))
			pWscControl->RegData.SelfInfo.ConfigError = WSC_ERROR_DECRYPT_CRC_FAIL;
		
        switch(rv)
        {
            case WSC_ERROR_HASH_FAIL:
                pWscControl->WscStatus = STATUS_WSC_ERROR_HASH_FAIL;
                break;
            case WSC_ERROR_HMAC_FAIL:
                pWscControl->WscStatus = STATUS_WSC_ERROR_HMAC_FAIL;
                break;
            default:
                pWscControl->WscStatus = STATUS_WSC_FAIL;
                break;
        }        
#ifdef DPA_S
#ifdef P2P_SUPPORT
		if (pWscControl->EntryIfIdx >= MIN_NET_DEVICE_FOR_P2P_CLI)
			RtmpOSWrielessEventSend(pAdapter->p2p_dev, RT_WLAN_EVENT_CUSTOM, P2P_NOTIF_WPS_FAIL, NULL, NULL, 0);
		else
#endif /* P2P_SUPPORT */
#endif /* DPA_S */
		RTMPSendWirelessEvent(pAdapter, IW_WSC_STATUS_FAIL, NULL, (pWscControl->EntryIfIdx & 0x0F), 0);
        if (bUPnPMsg)
        {
			if (pWscUPnPInfo->bUPnPMsgTimerRunning == TRUE)
			{
            	RTMPCancelTimer(&pWscUPnPInfo->UPnPMsgTimer, &Cancelled);
	            pWscUPnPInfo->bUPnPMsgTimerRunning = FALSE;
			}
			pWscUPnPInfo->bUPnPInProgress = FALSE;
        }
		else
        {
            DataLen = BuildMessageNACK(pAdapter, pWscControl, WscData);            
#ifdef CONFIG_AP_SUPPORT
		if (CurOpMode == AP_MODE)
		{
				WscSendMessage(pAdapter, WSC_OPCODE_NACK, WscData, DataLen, pWscControl, AP_MODE, EAP_CODE_REQ);
			pEntry->bWscCapable = FALSE;
		}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
		if (CurOpMode == STA_MODE)
			{
				if (ADHOC_ON(pAdapter))
					WscSendMessage(pAdapter, WSC_OPCODE_NACK, WscData, DataLen, pWscControl, STA_MODE, EAP_CODE_REQ);
				else
					WscSendMessage(pAdapter, WSC_OPCODE_NACK, WscData, DataLen, pWscControl, STA_MODE, EAP_CODE_RSP);
			}
#endif /* CONFIG_STA_SUPPORT */

		RTMPCancelTimer(&pWscControl->EapolTimer, &Cancelled);
		pWscControl->EapolTimerRunning = FALSE;
	}
        /*
           If a PIN authentication or communication error occurs after sending message M6, 
           the Registrar MUST warn the user and MUST NOT automatically reuse the PIN. 
           Furthermore, if the Registrar detects this situation and prompts the user for a new PIN from the Enrollee device, 
           it MUST NOT accept the same PIN again without warning the user of a potential attack.
        */
        if (pWscControl->WscState >= WSC_STATE_WAIT_M7)
        {
            pWscControl->WscRejectSamePinFromEnrollee = TRUE;
            pWscControl->WscPinCode = 0;
        }
        pWscControl->WscState = WSC_STATE_OFF;
        pWscControl->WscStatus = STATUS_WSC_IDLE;
		/*NdisZeroMemory(pWscControl->EntryAddr, MAC_ADDR_LEN); */
        /*pWscControl->WscMode = 1; */
#ifdef WSC_NFC_SUPPORT
		pWscControl->bTriggerByNFC = FALSE;
#endif /* WSC_NFC_SUPPORT */		
		
        bUPnPStatus = FALSE;
    }

	if(WscData)
		os_free_mem(NULL, WscData);
	
	if(bUPnPMsg && (bUPnPStatus == FALSE))
		WscUPnPErrHandle(pAdapter, pWscControl, Elem->TimeStamp.u.LowPart);

	if (pWscControl->WscState == WSC_STATE_CONFIGURED)
	{
#ifdef WSC_LED_SUPPORT
		UCHAR WPSLEDStatus;
#endif /* WSC_LED_SUPPORT */
#ifdef WSC_NFC_SUPPORT
		pWscControl->bTriggerByNFC = FALSE;
#endif /* WSC_NFC_SUPPORT */

		pWscControl->bWscTrigger = FALSE;
#ifdef IWSC_SUPPORT
		if ((pAdapter->OpMode == OPMODE_STA) &&
			(pAdapter->StaCfg.BssType == BSS_ADHOC))
		{			
			if (pAdapter->StaCfg.IWscInfo.bSinglePIN)
			{
				pAdapter->StaCfg.IWscInfo.bDoNotStop = TRUE;
			}
			else
				pAdapter->StaCfg.IWscInfo.bDoNotStop = FALSE;
			RTMP_SEM_LOCK(&pWscControl->WscConfiguredPeerListSemLock);
			WscInsertPeerEntryByMAC(&pWscControl->WscConfiguredPeerList, pWscControl->WscPeerMAC);
			RTMP_SEM_UNLOCK(&pWscControl->WscConfiguredPeerListSemLock);
			NdisZeroMemory(pWscControl->WscPeerMAC, MAC_ADDR_LEN); // We need to clear here for 4-way handshaking
			MlmeEnqueue(pAdapter, IWSC_STATE_MACHINE, IWSC_MT2_MLME_STOP, 0, NULL, 0);
			RTMP_MLME_HANDLER(pAdapter);
		}
#endif /* IWSC_SUPPORT */
		if (pWscControl->Wsc2MinsTimerRunning)
		{
			pWscControl->Wsc2MinsTimerRunning = FALSE;
			RTMPCancelTimer(&pWscControl->Wsc2MinsTimer, &Cancelled);
		}
		if (bUPnPMsg)
		{
			if (pWscUPnPInfo->bUPnPMsgTimerRunning == TRUE)
			{	RTMPCancelTimer(&pWscUPnPInfo->UPnPMsgTimer, &Cancelled);
				pWscUPnPInfo->bUPnPMsgTimerRunning = FALSE;
			}
			pWscUPnPInfo->bUPnPInProgress = FALSE;
			pWscUPnPInfo->registrarID = 0;
		}
#ifdef CONFIG_AP_SUPPORT        
		else
		{
			if (CurOpMode == AP_MODE)
			{
				WscBuildBeaconIE(pAdapter, WSC_SCSTATE_CONFIGURED, FALSE, 0, 0, pEntry->func_tb_idx, NULL, 0, CurOpMode);
				WscBuildProbeRespIE(pAdapter, WSC_MSGTYPE_AP_WLAN_MGR, WSC_SCSTATE_CONFIGURED, FALSE, 0, 0, pWscControl->EntryIfIdx, NULL, 0, CurOpMode);
				APUpdateBeaconFrame(pAdapter, pEntry->func_tb_idx);
#ifdef WAC_SUPPORT
				if (pAdapter->ApCfg.MBSSID[pWscControl->EntryIfIdx & 0x0F].EasyConfigInfo.bEnableWAC)
				{
					pAdapter->ApCfg.MBSSID[pWscControl->EntryIfIdx & 0x0F].EasyConfigInfo.CurrentStatus = OFC_SUCCESS;
				}
#endif /* WAC_SUPPORT */

			}
		}
		NdisZeroMemory(&pAdapter->CommonCfg.WscStaPbcProbeInfo, sizeof(WSC_STA_PBC_PROBE_INFO));
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
		if (CurOpMode == STA_MODE)
		{
#ifdef STA_EASY_CONFIG_SETUP
			if (pEntry && (pAdapter->StaCfg.BssType == BSS_ADHOC) &&
				(pWscControl->WscConfStatus == WSC_SCSTATE_CONFIGURED))
				MacTableDeleteEntry(pAdapter, pEntry->wcid, pEntry->Addr);

			if ((pAdapter->StaCfg.BssType == BSS_INFRA) ||
				(pWscControl->WscConfStatus == WSC_SCSTATE_UNCONFIGURED))
			pWscControl->WscConfMode = WSC_DISABLE;
#else /* STA_EASY_CONFIG_SETUP */
#ifdef IWSC_SUPPORT
			if (pAdapter->StaCfg.IWscInfo.bDoNotStop == FALSE)
#endif /* IWSC_SUPPORT */
				pWscControl->WscConfMode = WSC_DISABLE;
#endif /* !STA_EASY_CONFIG_SETUP */

			if (pAdapter->StaCfg.BssType == BSS_INFRA)
				pWscControl->WscState = WSC_STATE_WAIT_EAPFAIL;
		}
#endif /* CONFIG_STA_SUPPORT */
		if (INFRA_ON(pAdapter) ||
			 (
				(pWscControl->WscConfStatus == WSC_SCSTATE_UNCONFIGURED) &&
				((CurOpMode == AP_MODE) || (ADHOC_ON(pAdapter)))
			  )
			)
		{
			pWscControl->WscConfStatus = WSC_SCSTATE_CONFIGURED;
#ifdef CONFIG_AP_SUPPORT
			if (CurOpMode == AP_MODE)
			{
				{
					/*
						Use ApplyProfileIdx to inform WscUpdatePortCfgTimer AP acts registrar.
					*/
					pWscControl->WscProfile.ApplyProfileIdx |= 0x8000;
					RTMPSetTimer(&pWscControl->WscUpdatePortCfgTimer, 1000);
					pWscControl->WscUpdatePortCfgTimerRunning = TRUE;
				}
			}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
			if (CurOpMode == STA_MODE)
			{
				pAdapter->StaCfg.bAutoConnectByBssid = TRUE;
				if ((pWscControl->bConfiguredAP) 
#ifdef WSC_V2_SUPPORT
					/* 
						Check AP is v2 or v1, Check WscV2 Enabled or not
					*/
					&& !(pWscControl->WscV2Info.bForceSetAP 
						&& pWscControl->WscV2Info.bEnableWpsV2 
						&& (pWscControl->RegData.PeerInfo.Version2!= 0))
#endif /* WSC_V2_SUPPORT */
					)
					RTMPMoveMemory(&pWscControl->WscProfile, &pWscControl->WscM7Profile, sizeof(pWscControl->WscM7Profile));

#ifdef DPA_T
				pCredential = &pWscControl->WscProfile.Profile[0];
				if (!((pCredential->AuthType == WSC_AUTHTYPE_OPEN) &&
					(pCredential->EncrType == WSC_ENCRTYPE_NONE)))
				{
					WscStop(pAdapter,
#ifdef CONFIG_AP_SUPPORT
							FALSE,
#endif /* CONFIG_AP_SUPPORT */
							pWscControl);
					pAdapter->StaCfg.bAutoConnectByBssid = FALSE;
					RTMPZeroMemory(pAdapter->MlmeAux.AutoReconnectSsid, MAX_LEN_OF_SSID);
					pAdapter->MlmeAux.AutoReconnectSsidLen = pAdapter->CommonCfg.SsidLen;
					RTMPMoveMemory(pAdapter->MlmeAux.AutoReconnectSsid, pAdapter->CommonCfg.Ssid, pAdapter->CommonCfg.SsidLen);
					if (INFRA_ON(pAdapter))
						WscLinkDown(pAdapter);
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscEapRegistrarAction : Done but AuthType == WSC_AUTHTYPE_OPEN EncrType == WSC_ENCRTYPE_NONE\n"));
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE,("pAd->MlmeAux.Ssid=%s\n",pAdapter->MlmeAux.Ssid));
				}
				else
#endif /* DPA_T */
				WscWriteConfToPortCfg(pAdapter, pWscControl, &pWscControl->WscProfile.Profile[0], TRUE);
				
#ifdef DPA_T
				if (!((pCredential->AuthType == WSC_AUTHTYPE_OPEN) &&
					(pCredential->EncrType == WSC_ENCRTYPE_NONE)))
#endif /* DPA_T */
				{
/*#ifdef KTHREAD_SUPPORT */
	/*				WAKE_UP(&(pAdapter->wscTask)); */
	/*#else */
	/*				RTMP_SEM_EVENT_UP(&(pAdapter->wscTask.taskSema)); */
	/*#endif */
					RtmpOsTaskWakeUp(&(pAdapter->wscTask));
				}
#ifdef IWSC_SUPPORT
				if (pAdapter->StaCfg.BssType == BSS_ADHOC)
				{
					if (pAdapter->StaCfg.IWscInfo.bDoNotStop == FALSE)
					{
						WscBuildBeaconIE(pAdapter, WSC_SCSTATE_CONFIGURED, FALSE, 0, 0, BSS0, NULL, 0, STA_MODE);
						WscBuildProbeRespIE(pAdapter, WSC_MSGTYPE_REGISTRAR, WSC_SCSTATE_CONFIGURED, FALSE, 0, 0, BSS0, NULL, 0, STA_MODE);
					}
					pAdapter->StaCfg.IWscInfo.bReStart = TRUE;
					WscLinkDown(pAdapter);
					if (pAdapter->StaCfg.IWscInfo.bIWscDevQueryReqTimerRunning == FALSE)
					{
						pAdapter->StaCfg.IWscInfo.bIWscDevQueryReqTimerRunning = TRUE;
						RTMPSetTimer(&pAdapter->StaCfg.IWscInfo.IWscDevQueryTimer, 200);
					}
				}				
#endif /* IWSC_SUPPORT */
			}
#endif /* CONFIG_STA_SUPPORT */
		}
#ifdef WSC_LED_SUPPORT
		/* The protocol is finished. */
		WPSLEDStatus = LED_WPS_SUCCESS;
		RTMPSetLED(pAdapter, WPSLEDStatus);
#endif /* WSC_LED_SUPPORT */
#ifdef IWSC_SUPPORT
		if (pAdapter->StaCfg.IWscInfo.bDoNotStop == FALSE)
#endif /* IWSC_SUPPORT */
		{
			pWscControl->WscPinCode = 0;
			pWscControl->WscMode = 1;
		}
		RTMPCancelTimer(&pWscControl->EapolTimer, &Cancelled);
		pWscControl->EapolTimerRunning = FALSE;

#ifdef IWSC_SUPPORT
		/*
			Some peer doesn't stop beacon and send de-auth, it will cause 4-way failed when our MAC is higher than peer.
			After WPS process complete, delete entry here for adding entry to table again for 4-way handshaking.
		*/
		if (pEntry && (pAdapter->StaCfg.BssType == BSS_ADHOC))
			MacTableDeleteEntry(pAdapter, pEntry->wcid, pEntry->Addr);
#endif /* IWSC_SUPPORT */
		return;
	}	
}

VOID WscTimeOutProcess(
    IN  PRTMP_ADAPTER       pAd,
    IN  PMAC_TABLE_ENTRY    pEntry,
    IN  INT                 nWscState,
    IN  PWSC_CTRL           pWscControl)
{
    INT         WscMode;
	UCHAR	CurOpMode = 0xFF;
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		CurOpMode = AP_MODE;
#endif // CONFIG_AP_SUPPORT //
	
#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		CurOpMode = STA_MODE;
#ifdef P2P_SUPPORT
	if (pWscControl->EntryIfIdx != BSS0)
		CurOpMode = AP_MODE;
#endif /* P2P_SUPPORT */
#endif // CONFIG_STA_SUPPORT //

    if (nWscState == WSC_STATE_WAIT_ACK)
        pWscControl->WscState = WSC_STATE_CONFIGURED;
    else if (nWscState == WSC_STATE_WAIT_RESP_ID)
        pWscControl->WscState = WSC_STATE_OFF;
    else if (nWscState == WSC_STATE_RX_M2D)
    {
        pWscControl->WscState = WSC_STATE_FAIL;
        
#ifdef CONFIG_AP_SUPPORT
		if (CurOpMode == AP_MODE)
		{
			if (pEntry && IS_ENTRY_CLIENT(pEntry))
			{
				WscSendEapFail(pAd, pWscControl, TRUE);
			}
#ifdef APCLI_SUPPORT
			if (pEntry && IS_ENTRY_APCLI(pEntry))
			{
				WscApCliLinkDown(pAd, pWscControl);
			}
#endif /* APCLI_SUPPORT */
		}
#endif /* CONFIG_AP_SUPPORT */        

		pWscControl->EapolTimerRunning = FALSE;
		pWscControl->WscRetryCount = 0;
        
#ifdef CONFIG_STA_SUPPORT
		if (CurOpMode == STA_MODE)
		{
			WscLinkDown(pAd);
		}
#endif /* CONFIG_STA_SUPPORT */

        return;
    }
    else if (nWscState == WSC_STATE_WAIT_EAPFAIL)
    {
#ifdef APCLI_SUPPORT        
       if((pWscControl->WscMode == WSC_PIN_MODE) && (pWscControl->EntryIfIdx & MIN_NET_DEVICE_FOR_APCLI))                            
           ;
      else
#endif
		{
        pWscControl->WscState = WSC_STATE_OFF;
		pWscControl->WscStatus = STATUS_WSC_CONFIGURED;
        pWscControl->WscConfMode = WSC_DISABLE;
		}
    }
    else
    {
#ifdef CONFIG_AP_SUPPORT    
        if ((pWscControl->WscActionMode == WSC_PROXY) && (pAd->OpMode == OPMODE_AP))
        {
    		pWscControl->WscState = WSC_STATE_OFF;
        }
    	else
#endif /* CONFIG_AP_SUPPORT */            
    		pWscControl->WscState = WSC_STATE_FAIL;
    }  

	if (nWscState == WSC_STATE_WAIT_M8)
		pWscControl->bWscTrigger = FALSE;
    pWscControl->WscRetryCount = 0;
	NdisZeroMemory(pWscControl->EntryAddr, MAC_ADDR_LEN);
    pWscControl->EapolTimerRunning = FALSE;
#if 0 //def CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
		pWscControl->WscPinCode = 0;
#endif /* CONFIG_AP_SUPPORT */
    if (pWscControl->WscMode == 1)
		WscMode = DEV_PASS_ID_PIN;
	else
		WscMode = DEV_PASS_ID_PBC;
    
#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
	{
		if ((pWscControl->WscConfStatus == WSC_SCSTATE_UNCONFIGURED) &&
			((nWscState == WSC_STATE_WAIT_DONE) || (nWscState == WSC_STATE_WAIT_ACK)))
		{
			pWscControl->bWscTrigger = FALSE;
			pWscControl->WscConfStatus = WSC_SCSTATE_CONFIGURED;
				WscBuildBeaconIE(pAd, pWscControl->WscConfStatus, FALSE, WscMode, pWscControl->WscConfigMethods, (pWscControl->EntryIfIdx & 0x0F), NULL, 0, CurOpMode);
				WscBuildProbeRespIE(pAd, WSC_MSGTYPE_AP_WLAN_MGR, pWscControl->WscConfStatus, FALSE, WscMode, pWscControl->WscConfigMethods, pWscControl->EntryIfIdx, NULL, 0, CurOpMode);
				pAd->WriteWscCfgToDatFile = pWscControl->EntryIfIdx;
				WscWriteConfToPortCfg(pAd, 
									pWscControl,
									&pWscControl->WscProfile.Profile[0],
									FALSE);
#ifdef P2P_SUPPORT
			if (pWscControl->EntryIfIdx & MIN_NET_DEVICE_FOR_P2P_GO)
			{
#ifdef RTMP_MAC_PCI
				OS_WAIT(1000);
#endif /* RTMP_MAC_PCI */
				P2P_GoStop(pAd);
				P2P_GoStartUp(pAd, MAIN_MBSSID);
			}
			else
#endif /* P2P_SUPPORT */
			{
				APStop(pAd);
				APStartUp(pAd);
			}

/*#ifdef KTHREAD_SUPPORT */
/*				WAKE_UP(&(pAd->wscTask)); */
/*#else */
/*				RTMP_SEM_EVENT_UP(&(pAd->wscTask.taskSema)); */
/*#endif */
				RtmpOsTaskWakeUp(&(pAd->wscTask));
		}
		else
		{
			if (pEntry && IS_ENTRY_CLIENT(pEntry))
			{
				pEntry->bWscCapable = FALSE;
				WscSendEapFail(pAd, pWscControl, TRUE);
			}
						
			WscBuildBeaconIE(pAd, pWscControl->WscConfStatus, FALSE, WscMode, pWscControl->WscConfigMethods, (pWscControl->EntryIfIdx & 0x0F), NULL, 0, CurOpMode);
			WscBuildProbeRespIE(pAd, WSC_MSGTYPE_AP_WLAN_MGR, pWscControl->WscConfStatus, FALSE, WscMode, pWscControl->WscConfigMethods, pWscControl->EntryIfIdx, NULL, 0, CurOpMode);
		}
#ifdef APCLI_SUPPORT
		if (pEntry && IS_ENTRY_APCLI(pEntry))
		{
			WscApCliLinkDown(pAd, pWscControl);
		}
#endif /* APCLI_SUPPORT */
	}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
	{
		WscLinkDown(pAd);
	}
#endif /* CONFIG_STA_SUPPORT */

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscTimeOutProcess\n"));
}

VOID WscEAPOLTimeOutAction(
    IN PVOID SystemSpecific1, 
    IN PVOID FunctionContext, 
    IN PVOID SystemSpecific2, 
    IN PVOID SystemSpecific3)
{
    PUCHAR              WscData = NULL;
    PMAC_TABLE_ENTRY    pEntry = NULL;
    PWSC_CTRL           pWscControl = NULL;
	PRTMP_ADAPTER pAd = NULL;
	UINT				MaxWscDataLen = WSC_MAX_DATA_LEN;
	UCHAR				CurOpMode = 0xFF;
    
    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscEAPOLTimeOutAction\n"));
        
    if (FunctionContext == 0)
    {
#ifdef RELEASE_EXCLUDE
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("data is NULL!!\n"));
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("<----- WscEAPOLTimeOutAction\n"));
#endif /* RELEASE_EXCLUDE */        
        return;
    }
    else
    {
        pWscControl = (PWSC_CTRL)FunctionContext;
		pAd = (PRTMP_ADAPTER)pWscControl->pAd;
		if (pAd == NULL)
		{
#ifdef RELEASE_EXCLUDE
        	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("pAd is NULL!!\n"));
        	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("<----- WscEAPOLTimeOutAction\n"));
#endif /* RELEASE_EXCLUDE */
			return;
		}
		pEntry = MacTableLookup(pWscControl->pAd, pWscControl->EntryAddr);
    }

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		CurOpMode = AP_MODE;
#endif // CONFIG_AP_SUPPORT //
	
#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		CurOpMode = STA_MODE;
#ifdef P2P_SUPPORT
	if (pWscControl->EntryIfIdx != BSS0)
		CurOpMode = AP_MODE;
#endif /* P2P_SUPPORT */
#endif // CONFIG_STA_SUPPORT //

	if ((CurOpMode == AP_MODE) || ADHOC_ON(pAd))
	{
		if (pEntry == NULL)
		{
#ifdef CONFIG_AP_SUPPORT
			/*
    			Some WPS Client will send dis-assoc close to WSC_DONE. 
    			If AP misses WSC_DONE, WPS Client still sends dis-assoc to AP.
    			AP driver needs to check wsc_state here for considering WPS process with this client is completed.
    		*/
			if ((CurOpMode == AP_MODE) && 
				((pWscControl->WscState == WSC_STATE_WAIT_DONE) || (pWscControl->WscState == WSC_STATE_WAIT_ACK)))
			{
#ifdef DPA_S
#ifdef P2P_SUPPORT
				if (pWscControl->EntryIfIdx >= MIN_NET_DEVICE_FOR_P2P_CLI)
					RtmpOSWrielessEventSend(pAd->p2p_dev, RT_WLAN_EVENT_CUSTOM, P2P_NOTIF_WPS_COMPLETE, NULL, NULL, 0);
#endif /* P2P_SUPPORT */
#endif /* DPA_S */
				pWscControl->WscStatus = STATUS_WSC_CONFIGURED;
				pWscControl->bWscTrigger = FALSE;
				pWscControl->RegData.ReComputePke = 1;
				if (pWscControl->Wsc2MinsTimerRunning)
				{
					BOOLEAN Cancelled;
					pWscControl->Wsc2MinsTimerRunning = FALSE;
					RTMPCancelTimer(&pWscControl->Wsc2MinsTimer, &Cancelled);
				}				
				WscTimeOutProcess(pAd, NULL, pWscControl->WscState, pWscControl);
			}			
#endif /* CONFIG_AP_SUPPORT */

#ifdef IWSC_SUPPORT
#ifdef CONFIG_STA_SUPPORT            
			
			if ((pAd->OpMode == OPMODE_STA) &&
				(pAd->StaCfg.BssType == BSS_ADHOC) &&
				(pAd->StaCfg.WscControl.WscConfMode == WSC_ENROLLEE))
				pAd->StaCfg.IWscInfo.bReStart = TRUE;
			
#endif // CONFIG_STA_SUPPORT //
#endif // IWSC_SUPPORT //
			pWscControl->EapolTimerRunning = FALSE;
			NdisZeroMemory(pWscControl->EntryAddr, MAC_ADDR_LEN);
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("sta is left.\n"));
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscEAPOLTimeOutAction\n"));
			return;
		}
	}

    if (!pWscControl->EapolTimerRunning)
    {
        pWscControl->WscRetryCount = 0;
        goto out;
    }
    
    if (pWscControl->EapolTimerPending)
    {
        RTMPModTimer(&pWscControl->EapolTimer, WSC_EAP_MSG_TIME_OUT);
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("EapolTimer Pending......\n"));
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscEAPOLTimeOutAction\n"));
        return;
    }

#ifdef WSC_V2_SUPPORT
	MaxWscDataLen = MaxWscDataLen + (UINT)pWscControl->WscV2Info.ExtraTlv.TlvLen;
#endif /* WSC_V2_SUPPORT */
	os_alloc_mem(NULL, (UCHAR **)&WscData, MaxWscDataLen);
    if (WscData != NULL)
        NdisZeroMemory(WscData, WSC_MAX_DATA_LEN);

#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
	{
		if (pEntry && IS_ENTRY_CLIENT(pEntry) && (pWscControl->WscState <= WSC_STATE_CONFIGURED) && (pWscControl->WscActionMode != WSC_PROXY))
		{
			/* A timer in the AP should cause to be disconnected after 5 seconds if a */
			/* valid EAP-Rsp/Identity indicating WPS is not received. */
			/* << from WPS EAPoL and RSN handling.doc >> */
			WscTimeOutProcess(pWscControl->pAd, pEntry, WSC_STATE_WAIT_RESP_ID, pWscControl);

			/* If do disassocation here, it will affect connection of non-WPS clients. */
			goto out;
		}
	}
#endif /* CONFIG_AP_SUPPORT */

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscState = %d\n", pWscControl->WscState));
    switch(pWscControl->WscState)
    {
        case WSC_STATE_WAIT_REQ_ID:
			/* For IWSC case, keep sending EAPOL_START until 2 mins timeout */
			if ((pWscControl->WscRetryCount >= 2)
#ifdef CONFIG_STA_SUPPORT
				&& (pAd->StaCfg.BssType == BSS_INFRA)
#endif /* CONFIG_STA_SUPPORT */
				)
				WscTimeOutProcess(pWscControl->pAd, pEntry, WSC_STATE_WAIT_REQ_ID, pWscControl);
			else
			{
				pWscControl->WscRetryCount++;
#ifdef CONFIG_STA_SUPPORT
				if ((pAd->StaCfg.BssType == BSS_INFRA) && (CurOpMode == STA_MODE))
					WscSendEapolStart(pAd, pAd->CommonCfg.Bssid, CurOpMode);
				else
#endif /* CONFIG_STA_SUPPORT */
					WscSendEapolStart(pAd, pEntry->Addr, CurOpMode);
				RTMPModTimer(&pWscControl->EapolTimer, WSC_EAP_MSG_TIME_OUT);
			}
            break;
        case WSC_STATE_WAIT_WSC_START:
			if (pWscControl->WscRetryCount >= 2)
		    	WscTimeOutProcess(pWscControl->pAd, pEntry, WSC_STATE_WAIT_WSC_START, pWscControl);
			else
			{
				pWscControl->WscRetryCount++;
				RTMPModTimer(&pWscControl->EapolTimer, WSC_EAP_MSG_TIME_OUT);
			}
            break;
        case WSC_STATE_WAIT_M1:
		if (pWscControl->WscRetryCount >= 2)
			WscTimeOutProcess(pWscControl->pAd, pEntry, WSC_STATE_WAIT_M1, pWscControl);
		else
		{
#ifdef CONFIG_AP_SUPPORT
				if (CurOpMode == AP_MODE)
					WscSendMessage(pWscControl->pAd, WSC_OPCODE_START, NULL, 0, pWscControl, AP_MODE, EAP_CODE_REQ);
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
				if (CurOpMode == STA_MODE)
				{
					/*WscSendMessage(pWscControl->pAd, WSC_OPCODE_START, NULL, 0, pWscControl, STA_MODE, EAP_CODE_REQ); */
					WscSendEapRspId(pAd, pEntry, pWscControl);
				}
#endif /* CONFIG_STA_SUPPORT */
				pWscControl->WscRetryCount++;
				RTMPModTimer(&pWscControl->EapolTimer, WSC_EAP_MSG_TIME_OUT);
			}
            	break;
		case WSC_STATE_SENT_M1:
			if (pWscControl->WscRetryCount >= 2)
				WscTimeOutProcess(pWscControl->pAd, pEntry, WSC_STATE_WAIT_M2, pWscControl);
			else
			{
				if (pWscControl->WscActionMode == WSC_ENROLLEE)
				{
					{
#ifdef CONFIG_AP_SUPPORT
						if (CurOpMode == AP_MODE)
						{
							if (IS_ENTRY_CLIENT(pEntry))
								WscSendMessage(pWscControl->pAd, WSC_OPCODE_MSG, pWscControl->RegData.LastTx.Data, pWscControl->RegData.LastTx.Length, pWscControl, AP_MODE, EAP_CODE_REQ);
							else if (IS_ENTRY_APCLI(pEntry))
								WscSendMessage(pWscControl->pAd, WSC_OPCODE_MSG, pWscControl->RegData.LastTx.Data, pWscControl->RegData.LastTx.Length, pWscControl, AP_CLIENT_MODE, EAP_CODE_RSP);
						}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
						if (CurOpMode == STA_MODE)
							WscSendMessage(pWscControl->pAd, WSC_OPCODE_MSG, pWscControl->RegData.LastTx.Data, pWscControl->RegData.LastTx.Length, pWscControl, STA_MODE, EAP_CODE_RSP);
#endif /* CONFIG_STA_SUPPORT */
				}
			}
			pWscControl->WscRetryCount++;
			RTMPModTimer(&pWscControl->EapolTimer, WSC_EAP_MSG_TIME_OUT);
		}
		break;
	case WSC_STATE_RX_M2D:
		if (pWscControl->WscRetryCount >= 2)
			WscTimeOutProcess(pWscControl->pAd, pEntry, WSC_STATE_RX_M2D, pWscControl);
		else
		{
			pWscControl->WscRetryCount++;
			RTMPModTimer(&pWscControl->EapolTimer, WSC_EAP_MSG_TIME_OUT);
		}
		break;
        case WSC_STATE_WAIT_PIN:
		if (pWscControl->WscRetryCount >= 2)
			WscTimeOutProcess(pWscControl->pAd, pEntry, WSC_STATE_WAIT_PIN, pWscControl);
		else
		{
			pWscControl->WscRetryCount++;
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("No PIN CODE, cannot send M2 out!\n"));
			RTMPModTimer(&pWscControl->EapolTimer, WSC_EAP_MSG_TIME_OUT);
		}
		break;
	case WSC_STATE_WAIT_M3:
		if (pWscControl->WscRetryCount >= 2)
			WscTimeOutProcess(pWscControl->pAd, pEntry, WSC_STATE_WAIT_M3, pWscControl);
		else
		{
			if (pWscControl->WscActionMode == WSC_REGISTRAR)
			{
				{
#ifdef CONFIG_AP_SUPPORT
						if (CurOpMode == AP_MODE)
							WscSendMessage(pWscControl->pAd, WSC_OPCODE_MSG, pWscControl->RegData.LastTx.Data, pWscControl->RegData.LastTx.Length, pWscControl, AP_MODE, EAP_CODE_REQ);
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
						if (CurOpMode == STA_MODE)
						{
							if (ADHOC_ON(pAd))
								WscSendMessage(pWscControl->pAd, 
										   WSC_OPCODE_MSG, 
										   pWscControl->RegData.LastTx.Data, 
										   pWscControl->RegData.LastTx.Length, 
										   pWscControl, 
										   STA_MODE, 
										   EAP_CODE_REQ);
							else
								WscSendMessage(pWscControl->pAd, 
											   WSC_OPCODE_MSG, 
											   pWscControl->RegData.LastTx.Data, 
											   pWscControl->RegData.LastTx.Length, 
											   pWscControl, 
											   STA_MODE, 
											   EAP_CODE_RSP);
						}
#endif /* CONFIG_STA_SUPPORT */
				}
			}
			pWscControl->WscRetryCount++;
			RTMPModTimer(&pWscControl->EapolTimer, WSC_EAP_MSG_TIME_OUT);
		}
		break;
        case WSC_STATE_WAIT_M4:
		if (pWscControl->WscRetryCount >= 2)
			WscTimeOutProcess(pWscControl->pAd, pEntry, WSC_STATE_WAIT_M4, pWscControl);
		else
		{
			if (pWscControl->WscActionMode == WSC_ENROLLEE)
			{
				{
#ifdef CONFIG_AP_SUPPORT
						if (CurOpMode == AP_MODE)
						{
							if (IS_ENTRY_CLIENT(pEntry))
								WscSendMessage(pWscControl->pAd, WSC_OPCODE_MSG, pWscControl->RegData.LastTx.Data, pWscControl->RegData.LastTx.Length, pWscControl, AP_MODE, EAP_CODE_REQ);
							else if (IS_ENTRY_APCLI(pEntry))
								WscSendMessage(pWscControl->pAd, WSC_OPCODE_MSG, pWscControl->RegData.LastTx.Data, pWscControl->RegData.LastTx.Length, pWscControl, AP_CLIENT_MODE, EAP_CODE_RSP);
						}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
						if (CurOpMode == STA_MODE)
							WscSendMessage(pWscControl->pAd, WSC_OPCODE_MSG, pWscControl->RegData.LastTx.Data, pWscControl->RegData.LastTx.Length, pWscControl, STA_MODE, EAP_CODE_RSP);
#endif /* CONFIG_STA_SUPPORT */
				}
			}
			pWscControl->WscRetryCount++;
			RTMPModTimer(&pWscControl->EapolTimer, WSC_EAP_MSG_TIME_OUT);
		}
		break;
	case WSC_STATE_WAIT_M5:
		if (pWscControl->WscRetryCount >= 2)
			WscTimeOutProcess(pWscControl->pAd, pEntry, WSC_STATE_WAIT_M5, pWscControl);
		else
		{
			if (pWscControl->WscActionMode == WSC_REGISTRAR)
			{         
				{
#ifdef CONFIG_AP_SUPPORT
						if (CurOpMode == AP_MODE)
							WscSendMessage(pWscControl->pAd, WSC_OPCODE_MSG, pWscControl->RegData.LastTx.Data, pWscControl->RegData.LastTx.Length, pWscControl, AP_MODE, EAP_CODE_REQ);
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
						if (CurOpMode == STA_MODE)
						{
							if (ADHOC_ON(pAd))
								WscSendMessage(pWscControl->pAd, 
											   WSC_OPCODE_MSG, 
											   pWscControl->RegData.LastTx.Data, 
											   pWscControl->RegData.LastTx.Length, 
											   pWscControl, 
											   STA_MODE, 
											   EAP_CODE_REQ);
							else
								WscSendMessage(pWscControl->pAd, 
											   WSC_OPCODE_MSG, 
											   pWscControl->RegData.LastTx.Data, 
											   pWscControl->RegData.LastTx.Length, 
											   pWscControl, 
											   STA_MODE, 
											   EAP_CODE_RSP);
						}
#endif /* CONFIG_STA_SUPPORT */
				}
			}
			pWscControl->WscRetryCount++;
			RTMPModTimer(&pWscControl->EapolTimer, WSC_EAP_MSG_TIME_OUT);
		}
		break;
        case WSC_STATE_WAIT_M6:
		if (pWscControl->WscRetryCount >= 2)
			WscTimeOutProcess(pWscControl->pAd, pEntry, WSC_STATE_WAIT_M6, pWscControl);
		else
		{
			if (pWscControl->WscActionMode == WSC_ENROLLEE)
			{
				{
#ifdef CONFIG_AP_SUPPORT
						if (CurOpMode == AP_MODE)
						{
							if (IS_ENTRY_CLIENT(pEntry))
								WscSendMessage(pWscControl->pAd, WSC_OPCODE_MSG, pWscControl->RegData.LastTx.Data, pWscControl->RegData.LastTx.Length, pWscControl, AP_MODE, EAP_CODE_REQ);
							else if (IS_ENTRY_APCLI(pEntry))
								WscSendMessage(pWscControl->pAd, WSC_OPCODE_MSG, pWscControl->RegData.LastTx.Data, pWscControl->RegData.LastTx.Length, pWscControl, AP_CLIENT_MODE, EAP_CODE_RSP);
						}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
						if (CurOpMode == STA_MODE)
							WscSendMessage(pWscControl->pAd, WSC_OPCODE_MSG, pWscControl->RegData.LastTx.Data, pWscControl->RegData.LastTx.Length, pWscControl, STA_MODE, EAP_CODE_RSP);
#endif /* CONFIG_STA_SUPPORT */
				}
			}
			pWscControl->WscRetryCount++;
			RTMPModTimer(&pWscControl->EapolTimer, WSC_EAP_MSG_TIME_OUT);
		}
		break;
        case WSC_STATE_WAIT_M7:
		if (pWscControl->WscRetryCount >= 2)
			WscTimeOutProcess(pWscControl->pAd, pEntry, WSC_STATE_WAIT_M7, pWscControl);
		else
		{
			if (pWscControl->WscActionMode == WSC_REGISTRAR)
			{
				{
#ifdef CONFIG_AP_SUPPORT
						if (CurOpMode == AP_MODE)
							WscSendMessage(pWscControl->pAd, WSC_OPCODE_MSG, pWscControl->RegData.LastTx.Data, pWscControl->RegData.LastTx.Length, pWscControl, AP_MODE, EAP_CODE_REQ);
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
						if (CurOpMode == STA_MODE)
						{
							if (ADHOC_ON(pAd))								
								WscSendMessage(pWscControl->pAd, 
											   WSC_OPCODE_MSG, 
											   pWscControl->RegData.LastTx.Data, 
											   pWscControl->RegData.LastTx.Length, 
											   pWscControl, 
											   STA_MODE, 
											   EAP_CODE_REQ);
							else
								WscSendMessage(pWscControl->pAd, 
											   WSC_OPCODE_MSG, 
											   pWscControl->RegData.LastTx.Data, 
											   pWscControl->RegData.LastTx.Length, 
											   pWscControl, 
											   STA_MODE, 
											   EAP_CODE_RSP);
						}
#endif /* CONFIG_STA_SUPPORT */
				}
			}
			pWscControl->WscRetryCount++;
			RTMPModTimer(&pWscControl->EapolTimer, WSC_EAP_MSG_TIME_OUT);
		}
		break;
        case WSC_STATE_WAIT_M8:
		if (pWscControl->WscRetryCount >= 2)
			WscTimeOutProcess(pWscControl->pAd, pEntry, WSC_STATE_WAIT_M8, pWscControl);
		else
		{
			if (pWscControl->WscActionMode == WSC_ENROLLEE)
			{
				{
#ifdef CONFIG_AP_SUPPORT
						if (CurOpMode == AP_MODE)
						{
							if (IS_ENTRY_CLIENT(pEntry))
								WscSendMessage(pWscControl->pAd, WSC_OPCODE_MSG, pWscControl->RegData.LastTx.Data, pWscControl->RegData.LastTx.Length, pWscControl, AP_MODE, EAP_CODE_REQ);
							else if (IS_ENTRY_APCLI(pEntry))
								WscSendMessage(pWscControl->pAd, WSC_OPCODE_MSG, pWscControl->RegData.LastTx.Data, pWscControl->RegData.LastTx.Length, pWscControl, AP_CLIENT_MODE, EAP_CODE_RSP);
						}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
						if (CurOpMode == STA_MODE)
							WscSendMessage(pWscControl->pAd, WSC_OPCODE_MSG, pWscControl->RegData.LastTx.Data, pWscControl->RegData.LastTx.Length, pWscControl, STA_MODE, EAP_CODE_RSP);
#endif /* CONFIG_STA_SUPPORT */
				}
			}
			pWscControl->WscRetryCount++;
			RTMPModTimer(&pWscControl->EapolTimer, WSC_EAP_MSG_TIME_OUT);
		}
		break;
	case WSC_STATE_WAIT_DONE:
		if (pWscControl->WscRetryCount >= 2)
			WscTimeOutProcess(pWscControl->pAd, pEntry, WSC_STATE_WAIT_DONE, pWscControl);
		else
		{
			if (pWscControl->WscActionMode == WSC_REGISTRAR)
			{
				{
#ifdef CONFIG_AP_SUPPORT
						if (CurOpMode == AP_MODE)
							WscSendMessage(pWscControl->pAd, WSC_OPCODE_MSG, pWscControl->RegData.LastTx.Data, pWscControl->RegData.LastTx.Length, pWscControl, AP_MODE, EAP_CODE_REQ);
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
						if (CurOpMode == STA_MODE)
						{
							if (ADHOC_ON(pAd))
								WscSendMessage(pWscControl->pAd, 
											   WSC_OPCODE_MSG, 
											   pWscControl->RegData.LastTx.Data, 
											   pWscControl->RegData.LastTx.Length, 
											   pWscControl, 
											   STA_MODE, 
											   EAP_CODE_REQ);
							else
								WscSendMessage(pWscControl->pAd, 
											   WSC_OPCODE_MSG, 
											   pWscControl->RegData.LastTx.Data, 
											   pWscControl->RegData.LastTx.Length, 
											   pWscControl, 
											   STA_MODE, 
											   EAP_CODE_RSP);
						}
#endif /* CONFIG_STA_SUPPORT */
				}
			}
			pWscControl->WscRetryCount++;
			RTMPModTimer(&pWscControl->EapolTimer, WSC_EAP_MSG_TIME_OUT);
		}
		break;
#ifdef CONFIG_AP_SUPPORT
		/* Only AP_Enrollee needs to wait EAP_ACK */
		case WSC_STATE_WAIT_ACK:
#if 1
			WscTimeOutProcess(pWscControl->pAd, pEntry, WSC_STATE_WAIT_ACK, pWscControl);
#else
			if (pWscControl->WscRetryCount >= 2)
				WscTimeOutProcess(pWscControl->pAd, pEntry, WSC_STATE_WAIT_ACK, pWscControl);
			else
			{
				if (WscData &&
					pWscControl->WscActionMode == WSC_ENROLLEE && 
					IS_ENTRY_CLIENT(pEntry))
				{
					DataLen = BuildMessageDONE(pWscControl->pAd, pWscControl, WscData);
					WscSendMessage(pWscControl->pAd, WSC_OPCODE_MSG, WscData, DataLen, pWscControl, AP_MODE, EAP_CODE_REQ);
				}
				RTMPModTimer(&pWscControl->EapolTimer, WSC_EAP_MSG_ACK_TIME_OUT);
				pWscControl->WscRetryCount++;
			}
#endif
			break;
#endif /* CONFIG_AP_SUPPORT */
        case WSC_STATE_WAIT_EAPFAIL:
            /* Wait 2 seconds */
            if (pWscControl->WscRetryCount >= 1)
                WscTimeOutProcess(pWscControl->pAd, pEntry, WSC_STATE_WAIT_EAPFAIL, pWscControl);
            else
            {
                RTMPModTimer(&pWscControl->EapolTimer, WSC_EAP_EAP_FAIL_TIME_OUT);
                pWscControl->WscRetryCount++;
			}
            break;
        default:
            break;
    }

out:
    if (WscData)
		os_free_mem(NULL, WscData);

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscEAPOLTimeOutAction\n"));
}

VOID Wsc2MinsTimeOutAction(
    IN PVOID SystemSpecific1, 
    IN PVOID FunctionContext, 
    IN PVOID SystemSpecific2, 
    IN PVOID SystemSpecific3)
{
	mjh_wsc_flag = 0;  
	mjh_2_out = 1;   //mjh 2 mins time out
	mjh_wsc_fail = 0;
	mjh_wsc_link = 0;
	printf("*********2 min time out**************\n");

	PWSC_CTRL       pWscControl = (PWSC_CTRL)FunctionContext;
	PRTMP_ADAPTER 	pAd = NULL;
#ifdef CONFIG_AP_SUPPORT
	INT	IsAPConfigured = 0;
#endif /* CONFIG_AP_SUPPORT */
	BOOLEAN         Cancelled;
	UCHAR			CurOpMode = 0xFF;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> Wsc2MinsTimeOutAction\n"));
	if (pWscControl != NULL)
	{
		pAd =  (PRTMP_ADAPTER)pWscControl->pAd;

		if (pAd == NULL)
		{
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("pAd is NULL!\n"));
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- Wsc2MinsTimeOutAction\n"));
			return;
		}
#ifdef CONFIG_AP_SUPPORT
#ifdef CON_WPS
		if (pAd->conWscStatus != CON_WPS_STATUS_DISABLED)
		{
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("CON_WPS: Reset the status to default.\n"));
			pAd->conWscStatus = CON_WPS_STATUS_DISABLED;
		}	
#endif /* CON_WPS */

				IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
					CurOpMode = AP_MODE;
#endif /* CONFIG_AP_SUPPORT */
		
#ifdef CONFIG_STA_SUPPORT
				IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
					CurOpMode = STA_MODE;
#ifdef P2P_SUPPORT
				if (pWscControl->EntryIfIdx != BSS0)
					CurOpMode = AP_MODE;
#endif /* P2P_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */

		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Wsc2MinsTimerRunning is %s\n", 
		pWscControl->Wsc2MinsTimerRunning ? "TRUE, reset WscState to WSC_STATE_OFF":"FALSE"));

#ifdef WSC_LED_SUPPORT
		/* 120 seconds WPS walk time expiration. */
		pWscControl->bWPSWalkTimeExpiration = TRUE;
#endif /* WSC_LED_SUPPORT */

		if (pWscControl->Wsc2MinsTimerRunning)
		{
			pWscControl->bWscTrigger = FALSE;
			pWscControl->EapolTimerRunning = FALSE;
			RTMPCancelTimer(&pWscControl->EapolTimer, &Cancelled);

#ifdef CONFIG_AP_SUPPORT
			if (CurOpMode == AP_MODE)
			{
				IsAPConfigured = pWscControl->WscConfStatus;
				if ((pWscControl->EntryIfIdx & 0x0F) < pAd->ApCfg.BssidNum)
				{
					WscBuildBeaconIE(pWscControl->pAd, IsAPConfigured, FALSE, 0, 0, (pWscControl->EntryIfIdx & 0x0F), NULL, 0, CurOpMode);
					WscBuildProbeRespIE(pWscControl->pAd, WSC_MSGTYPE_AP_WLAN_MGR, IsAPConfigured, FALSE, 0, 0, pWscControl->EntryIfIdx, NULL, 0, CurOpMode);
					APUpdateBeaconFrame(pWscControl->pAd, pWscControl->EntryIfIdx & 0x0F);
				}
				if ((pWscControl->WscConfMode & WSC_PROXY) == 0)
				{   /* Proxy mechanism is disabled */
					pWscControl->WscState = WSC_STATE_OFF;
				}
			}
#endif /* CONFIG_AP_SUPPORT */

			pWscControl->WscMode = 1;
			pWscControl->WscRetryCount = 0;
			pWscControl->Wsc2MinsTimerRunning = FALSE;

			pWscControl->WscSelReg = 0;
			pWscControl->WscStatus = STATUS_WSC_IDLE; 

			RTMPSendWirelessEvent(pAd, IW_WSC_2MINS_TIMEOUT, NULL, (pWscControl->EntryIfIdx & 0x0F), 0);

			if (pWscControl->WscScanTimerRunning)
			{
				pWscControl->WscScanTimerRunning = FALSE;
				RTMPCancelTimer(&pWscControl->WscScanTimer, &Cancelled);
			}
			if (pWscControl->WscPBCTimerRunning)
			{
				pWscControl->WscPBCTimerRunning = FALSE;
				RTMPCancelTimer(&pWscControl->WscPBCTimer, &Cancelled);
			}
#ifdef APCLI_SUPPORT
            if (pWscControl->WscPINTimerRunning)
            {
                pWscControl->WscPINTimerRunning = FALSE;
                RTMPCancelTimer(&pWscControl->WscPINTimer, &Cancelled);
                pWscControl->WscStatus = STATUS_WSC_FAIL;
                pWscControl->WscState = WSC_STATE_OFF;
            }
            pWscControl->WscPINSsidIsSet = FALSE;
#endif
#ifdef CONFIG_STA_SUPPORT
			if (CurOpMode == STA_MODE)
			{				
				pAd->StaCfg.bAutoConnectByBssid = FALSE;
				RTMPZeroMemory(pAd->MlmeAux.AutoReconnectSsid, MAX_LEN_OF_SSID);
				pAd->MlmeAux.AutoReconnectSsidLen = pAd->CommonCfg.SsidLen;
				RTMPMoveMemory(pAd->MlmeAux.AutoReconnectSsid, pAd->CommonCfg.Ssid, pAd->CommonCfg.SsidLen);
				if (INFRA_ON(pAd) || 
					(pWscControl->WscConfMode == WSC_ENROLLEE))
					WscLinkDown(pAd);
				else
				{
					AsicDisableSync(pAd);
					WscBuildBeaconIE(pAd, pWscControl->WscConfStatus, FALSE, 0, 0, BSS0, NULL, 0, CurOpMode);
					WscBuildProbeRespIE(pAd,
						WSC_MSGTYPE_REGISTRAR,
						pWscControl->WscConfStatus,
						FALSE,
						0,
						0,
						BSS0,
						NULL, 
						0,
						CurOpMode);
					MakeIbssBeacon(pAd);
					AsicEnableIbssSync(pAd);
				}
				pWscControl->WscConfMode = WSC_DISABLE;
				pWscControl->WscState = WSC_STATE_OFF;
#ifdef WAC_SUPPORT
				pAd->StaCfg.EasyConfigInfo.bWACAP = FALSE;
#endif /* WAC_SUPPORT */
			}
#endif /* CONFIG_STA_SUPPORT */		
#ifdef WSC_NFC_SUPPORT
			pWscControl->bTriggerByNFC = FALSE;
#endif /* WSC_NFC_SUPPORT */	
		}

#ifdef WSC_LED_SUPPORT
		/* if link is up, there shall be nothing wrong */
		/* perhaps we will set another flag to do it */
		if ((OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED)) && 
			(pWscControl->WscState == WSC_STATE_OFF) &&
			(pWscControl->WscStatus == STATUS_WSC_CONFIGURED))
		{
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscConnectTimeout --> Connection OK\n"));
		}
		else
		{
			UCHAR WPSLEDStatus;
			
			pWscControl->WscStatus = STATUS_WSC_FAIL; 
			pWscControl->WscState = WSC_STATE_OFF;

			/* WPS LED mode 7, 8, 11 or 12. */
			if ((LED_MODE(pAd) == WPS_LED_MODE_7) || 
				(LED_MODE(pAd) == WPS_LED_MODE_8) || 
				(LED_MODE(pAd) == WPS_LED_MODE_11) || 
			    (LED_MODE(pAd) == WPS_LED_MODE_12))
			{
				pWscControl->bSkipWPSTurnOffLED = FALSE;
				
				/* Turn off the WPS LED modoe due to the maximum WPS processing time is expired (120 seconds). */
				WPSLEDStatus = LED_WPS_TURN_LED_OFF;
				RTMPSetLED(pAd, WPSLEDStatus);
			}
			else if ((LED_MODE(pAd) == WPS_LED_MODE_9) /* WPS LED mode 9. */
#ifdef CONFIG_WIFI_LED_SHARE
					|| (LED_MODE(pAd) == WPS_LED_MODE_SHARE)
#endif /* CONFIG_WIFI_LED_SHARE */
					)
			{	
				if (pWscControl->WscMode == WSC_PIN_MODE) /* PIN method. */
				{
					/* The NIC using PIN method fails to finish the WPS handshaking within 120 seconds. */
					WPSLEDStatus = LED_WPS_ERROR;
					RTMPSetLED(pAd, WPSLEDStatus);
#ifdef CONFIG_WIFI_LED_SHARE
					if(LED_MODE(pAd) == WPS_LED_MODE_SHARE)
						RTMPSetTimer(&pWscControl->WscLEDTimer, WSC_WPS_FAIL_WIFI_LED_TIMEOUT);
					else
#endif /* CONFIG_WIFI_LED_SHARE */
					/* Turn off the WPS LED after 15 seconds. */
					RTMPSetTimer(&pWscControl->WscLEDTimer, WSC_WPS_FAIL_LED_PATTERN_TIMEOUT);

					/* The Ralink UI would make RT_OID_DISCONNECT_REQUEST request while it receive STATUS_WSC_EAP_FAILED. */
					/* Allow the NIC to turn off the WPS LED after WSC_WPS_SKIP_TURN_OFF_LED_TIMEOUT seconds. */
					pWscControl->bSkipWPSTurnOffLED = TRUE;
					RTMPSetTimer(&pWscControl->WscSkipTurnOffLEDTimer, WSC_WPS_SKIP_TURN_OFF_LED_TIMEOUT);
					
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: The NIC using PIN method fails to finish the WPS handshaking within 120 seconds.\n", __FUNCTION__));
				}
				else if (pWscControl->WscMode == WSC_PBC_MODE) /* PBC method. */
				{
					switch (pWscControl->WscLastWarningLEDMode) /* Based on last WPS warning LED mode. */
					{
						case 0:
						case LED_WPS_ERROR:
						case LED_WPS_SESSION_OVERLAP_DETECTED:
							/* Failed to find any partner. */
							WPSLEDStatus = LED_WPS_ERROR;
							RTMPSetLED(pAd, WPSLEDStatus);
#ifdef CONFIG_WIFI_LED_SHARE
							if(LED_MODE(pAd) == WPS_LED_MODE_SHARE)
								RTMPSetTimer(&pWscControl->WscLEDTimer, WSC_WPS_FAIL_WIFI_LED_TIMEOUT);
							else
#endif /* CONFIG_WIFI_LED_SHARE */

							/* Turn off the WPS LED after 15 seconds. */
							RTMPSetTimer(&pWscControl->WscLEDTimer, WSC_WPS_FAIL_LED_PATTERN_TIMEOUT);

							/* The Ralink UI would make RT_OID_DISCONNECT_REQUEST request while it receive STATUS_WSC_EAP_FAILED. */
							/* Allow the NIC to turn off the WPS LED after WSC_WPS_SKIP_TURN_OFF_LED_TIMEOUT seconds. */
							pWscControl->bSkipWPSTurnOffLED = TRUE;
							RTMPSetTimer(&pWscControl->WscSkipTurnOffLEDTimer, WSC_WPS_SKIP_TURN_OFF_LED_TIMEOUT);
							
							MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: Last WPS LED status is LED_WPS_ERROR.\n", __FUNCTION__));
							break;

						default:
							/* do nothing. */
							break;
					}
				}
				else
				{
					/* do nothing. */
				}
			}
			else
			{
				/* do nothing. */
			}
			
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscConnectTimeout --> Fail to connect\n"));
		}
#endif /* WSC_LED_SUPPORT */
	}

#ifdef DPA_S
#ifdef P2P_SUPPORT
		if (pWscControl->EntryIfIdx >= MIN_NET_DEVICE_FOR_P2P_CLI)
			RtmpOSWrielessEventSend(pAd->p2p_dev, RT_WLAN_EVENT_CUSTOM, P2P_NOTIF_WPS_TIMEOUT, NULL, NULL, 0);
#endif /* P2P_SUPPORT */
#endif /* DPA_S */

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- Wsc2MinsTimeOutAction\n"));
}

/*
	========================================================================
	
	Routine Description:
		Classify EAP message type for enrolee

	Arguments:
		pAd         - NIC Adapter pointer
		Elem		- The EAP packet
		
	Return Value:
		Received EAP message type

	IRQL = DISPATCH_LEVEL
	
	Note:
		
	========================================================================
*/
UCHAR	WscRxMsgType(
	IN	PRTMP_ADAPTER		pAdapter,
	IN	PMLME_QUEUE_ELEM	pElem) 
{
	USHORT				Length;
	PUCHAR				pData;
	USHORT				WscType, WscLen;
    RTMP_STRING id_data[] = {"hello"};
    RTMP_STRING fail_data[] = {"EAP_FAIL"};
    RTMP_STRING wsc_start[] = {"WSC_START"};
#ifdef WSC_V2_SUPPORT
	RTMP_STRING wsc_frag_ack[] = "WSC_FRAG_ACK";
#endif /* WSC_V2_SUPPORT */
    RTMP_STRING regIdentity[] = {"WFA-SimpleConfig-Registrar"};
    RTMP_STRING enrIdentity[] = {"WFA-SimpleConfig-Enrollee"};

    if (pElem->Msg[0] == 'W' && pElem->Msg[1] == 'F' && pElem->Msg[2] == 'A')
    {
        /* Eap-Rsp(Identity) */
		if (memcmp(regIdentity, pElem->Msg, strlen(regIdentity)) == 0)
			return  WSC_MSG_EAP_REG_RSP_ID;
        else if (memcmp(enrIdentity, pElem->Msg, strlen(enrIdentity)) == 0)
			return  WSC_MSG_EAP_ENR_RSP_ID;
    }
    else if (NdisEqualMemory(id_data, pElem->Msg, pElem->MsgLen))
    {
        /* Eap-Req/Identity(hello) */
		return  WSC_MSG_EAP_REQ_ID;
    }
    else if (NdisEqualMemory(fail_data, pElem->Msg, pElem->MsgLen))
    {
        /* Eap-Fail */
		return  WSC_MSG_EAP_FAIL;
    }
    else if (NdisEqualMemory(wsc_start, pElem->Msg, pElem->MsgLen))
    {
        /* Eap-Req(Wsc_Start) */
        return WSC_MSG_EAP_REQ_START;
    }
#ifdef WSC_V2_SUPPORT
	else if (NdisEqualMemory(wsc_frag_ack, pElem->Msg, pElem->MsgLen))
    {
        /* WSC FRAG ACK */
        return WSC_MSG_EAP_FRAG_ACK;
    }
#endif /* WSC_V2_SUPPORT */
    else
    {   /* Eap-Esp(Messages) */
        pData = pElem->Msg;
        Length = (USHORT)pElem->MsgLen;

        /* the first TLV item in EAP Messages must be WSC_IE_VERSION */
        NdisMoveMemory(&WscType, pData, 2);
        if (ntohs(WscType) != WSC_ID_VERSION)
            goto out;

        /* Not Wsc Start, We have to look for WSC_IE_MSG_TYPE to classify M2 ~ M8, the remain size must large than 4 */
		while (Length > 4)
		{
			/* arm-cpu has packet alignment issue, it's better to use memcpy to retrieve data */
			NdisMoveMemory(&WscType, pData, 2);
			NdisMoveMemory(&WscLen,  pData + 2, 2);
			WscLen = ntohs(WscLen);
			if (ntohs(WscType) == WSC_ID_MSG_TYPE)
			{
				return(*(pData + 4));	/* Found the message type */
			}
			else
			{
				pData  += (WscLen + 4);
				Length -= (WscLen + 4);
			}
		}
    }

out:
	return  WSC_MSG_UNKNOWN;
}

/*
	========================================================================
	
	Routine Description:
		Classify WSC message type

	Arguments:
		EAPType		Value of EAP message type
		MsgType		Internal Message definition for MLME state machine
		
	Return Value:
		TRUE		Found appropriate message type
		FALSE		No appropriate message type
	
	Note:
		All these constants are defined in wsc.h
		For supplicant, there is only EAPOL Key message avaliable
		
	========================================================================
*/
BOOLEAN	WscMsgTypeSubst(
	IN	UCHAR	EAPType,
	IN	UCHAR	EAPCode,
	OUT	INT		*MsgType)	
{
	switch (EAPType)
	{
		case EAPPacket:
			*MsgType = WSC_EAPOL_PACKET_MSG;
			break;
        case EAPOLStart:
            *MsgType = WSC_EAPOL_START_MSG;
			break;
		default:
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscMsgTypeSubst : unsupported EAP Type(%d); \n", EAPType));
			return FALSE;		
	}	

	return TRUE;
}
#if 0
/*
	========================================================================
	
	Routine Description:
		Init WSC MAC header

	Arguments:
		pAd	Pointer	to our adapter
		
	Return Value:
		None
		
	Note:
		
	========================================================================
*/
VOID WscMacHeaderInit(
	IN RTMP_ADAPTER *pAdapter, 
	IN OUT HEADER_802_11 *Hdr, 
	IN UCHAR *pAddr1,
	IN UCHAR *pBSSID,
	IN BOOLEAN bFromApCli)	
{
	NdisZeroMemory(Hdr,	sizeof(HEADER_802_11));
	Hdr->FC.Type = FC_TYPE_DATA;
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAdapter)
	{
#ifdef APCLI_SUPPORT
		if (bFromApCli)
			Hdr->FC.ToDs = 1;
		else
#endif /* APCLI_SUPPORT */
		{
			Hdr->FC.ToDs = 0;
			Hdr->FC.FrDs = 1;
		}
	}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT 
	IF_DEV_CONFIG_OPMODE_ON_STA(pAdapter)
		Hdr->FC.ToDs = 1;
#endif /* CONFIG_STA_SUPPORT */
	
	 /*	Addr1: DA, Addr2: BSSID, Addr3:	SA */
	COPY_MAC_ADDR(&Hdr->Addr1, pAddr1);
#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
	if ((bFromApCli) && (pAdapter->OpMode == OPMODE_AP))
	{
		COPY_MAC_ADDR(&Hdr->Addr2, &pAdapter->ApCfg.ApCliTab[0].CurrentAddress);
	}
	else
#endif /* APCLI_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
	    COPY_MAC_ADDR(&Hdr->Addr2, &pAdapter->CurrentAddress);
	COPY_MAC_ADDR(&Hdr->Addr3, pBSSID);
	Hdr->Sequence =	pAdapter->Sequence;		
}
#endif


VOID WscInitRegistrarPair(RTMP_ADAPTER *pAd, WSC_CTRL *pWscControl, UCHAR apidx)
{	
	UCHAR CurOpMode = 0xff;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscInitRegistrarPair\n"));

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		CurOpMode = AP_MODE;
#endif // CONFIG_AP_SUPPORT //

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		CurOpMode = STA_MODE;
#ifdef P2P_SUPPORT
	if (pWscControl->EntryIfIdx != BSS0)
	{
		CurOpMode = AP_MODE;
		apidx = MAIN_MBSSID;
	}
#endif /* P2P_SUPPORT */
#endif // CONFIG_STA_SUPPORT //

	pWscControl->WscActionMode = 0;

	/* 1. Version */
	/*pWscControl->RegData.SelfInfo.Version = WSC_VERSION; */

	/* 2. UUID Enrollee, last 6 bytes use MAC */
	NdisMoveMemory(&pWscControl->RegData.SelfInfo.Uuid[0], &pWscControl->Wsc_Uuid_E[0], UUID_LEN_HEX);

	/* 3. MAC address */
#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
	{
		if (apidx >= HW_BEACON_MAX_NUM)
		{
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					("%s: apidx >= HW_BEACON_MAX_NUM!\n", __FUNCTION__));
			apidx = 0;
		}
		NdisMoveMemory(pWscControl->RegData.SelfInfo.MacAddr, pAd->ApCfg.MBSSID[apidx].wdev.bssid, 6);
	}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT	
	if (CurOpMode == STA_MODE)
		NdisMoveMemory(pWscControl->RegData.SelfInfo.MacAddr, pAd->CurrentAddress, 6);
#endif /* CONFIG_STA_SUPPORT */

	/* 4. Device Name */
#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
	{
		if (!RTMP_TEST_FLAG(pWscControl, 0x04))
		NdisMoveMemory(&pWscControl->RegData.SelfInfo.DeviceName, AP_WSC_DEVICE_NAME, sizeof(AP_WSC_DEVICE_NAME));
	}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
	{
	if (!RTMP_TEST_FLAG(pWscControl, 0x04))
		{
#ifdef WIDI_SUPPORT
			if (pAd->StaCfg.bWIDI && !pAd->StaCfg.WscControl.bWscTrigger)
				NdisMoveMemory(&pWscControl->RegData.SelfInfo.DeviceName, 
								WIDI_STA_WSC_DEVICE_NAME, sizeof(WIDI_STA_WSC_DEVICE_NAME));
			else
#endif /* WIDI_SUPPORT */		
				NdisMoveMemory(&pWscControl->RegData.SelfInfo.DeviceName,
						STA_WSC_DEVICE_NAME, sizeof(STA_WSC_DEVICE_NAME));
	}
	}
#endif /* CONFIG_STA_SUPPORT */
	
	/* 5. Manufacture woody */
	if (!RTMP_TEST_FLAG(pWscControl, 0x01))
		NdisMoveMemory(&pWscControl->RegData.SelfInfo.Manufacturer,
							WSC_MANUFACTURE, sizeof(WSC_MANUFACTURE));

	/* 6. Model Name */
#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
	{
		if (!RTMP_TEST_FLAG(pWscControl, 0x02))
		NdisMoveMemory(&pWscControl->RegData.SelfInfo.ModelName, AP_WSC_MODEL_NAME, sizeof(AP_WSC_MODEL_NAME));
	}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
	{
		if (!RTMP_TEST_FLAG(pWscControl, 0x02))
		{
#ifdef WIDI_SUPPORT
			if (pAd->StaCfg.bWIDI && !pAd->StaCfg.WscControl.bWscTrigger)
				NdisMoveMemory(&pWscControl->RegData.SelfInfo.ModelName, 
								WIDI_STA_WSC_MODEL_NAME, sizeof(WIDI_STA_WSC_MODEL_NAME));
			else
#endif /* WIDI_SUPPORT */			
		NdisMoveMemory(&pWscControl->RegData.SelfInfo.ModelName, STA_WSC_MODEL_NAME, sizeof(STA_WSC_MODEL_NAME));
	}
	}
#endif /* CONFIG_STA_SUPPORT */
	
	/* 7. Model Number */

	if (!RTMP_TEST_FLAG(pWscControl, 0x08))
	NdisMoveMemory(&pWscControl->RegData.SelfInfo.ModelNumber, WSC_MODEL_NUMBER, sizeof(WSC_MODEL_NUMBER));
	
	/* 8. Serial Number */
	if (!RTMP_TEST_FLAG(pWscControl, 0x10))	
	NdisMoveMemory(&pWscControl->RegData.SelfInfo.SerialNumber, WSC_MODEL_SERIAL, sizeof(WSC_MODEL_SERIAL));
	
	/* 9. Authentication Type Flags */
	/* Open(=1), WPAPSK(=2),Shared(=4), WPA2PSK(=20),WPA(=8),WPA2(=10) */
	/* (0x01 | 0x02 | 0x04 | 0x20 | 0x08 | 0x10) = 0x3F */
	/* WCN vista logo will check this flags. */
#ifdef WSC_V2_SUPPORT
	if (pWscControl->WscV2Info.bEnableWpsV2)
		/*
			AuthTypeFlags only needs to include Open and WPA2PSK in WSC 2.0.
		*/
		pWscControl->RegData.SelfInfo.AuthTypeFlags = cpu2be16(0x0021);
	else	
#endif /* WSC_V2_SUPPORT */
	pWscControl->RegData.SelfInfo.AuthTypeFlags = cpu2be16(0x003F);
	
	/* 10. Encryption Type Flags */
	/* None(=1), WEP(=2), TKIP(=4), AES(=8) */
	/* (0x01 | 0x02 | 0x04 | 0x08) = 0x0F */
#ifdef WSC_V2_SUPPORT
	if (pWscControl->WscV2Info.bEnableWpsV2)
		/*
			EncrTypeFlags only needs to include None and AES in WSC 2.0.
		*/
		pWscControl->RegData.SelfInfo.EncrTypeFlags = cpu2be16(0x0009);
	else	
#endif /* WSC_V2_SUPPORT */
	pWscControl->RegData.SelfInfo.EncrTypeFlags  = cpu2be16(0x000F);
	
	/* 11. Connection Type Flag */
#ifdef CONFIG_STA_SUPPORT
	if ((pAd->StaCfg.BssType == BSS_ADHOC) &&
		(pAd->OpMode == OPMODE_STA))
	{
		pWscControl->RegData.SelfInfo.ConnTypeFlags = 0x02;				/* IBSS */
	}
	else
#endif // CONFIG_STA_SUPPORT //	// 11. Connection Type Flag
	pWscControl->RegData.SelfInfo.ConnTypeFlags = 0x01;					/* ESS */

	/* 12. Associate state */
	pWscControl->RegData.SelfInfo.AssocState = cpu2be16(0x0000);		/* Not associated */

	/* 13. Configure Error */
	pWscControl->RegData.SelfInfo.ConfigError = cpu2be16(0x0000);		/* No error */

	/* 14. OS Version */
	pWscControl->RegData.SelfInfo.OsVersion = cpu2be32(0x80000000);		/* first bit must be 1 */
	
	/* 15. RF Band */
	/* Some WPS AP would check RfBand value in M1, ex. D-Link DIR-628 */
	pWscControl->RegData.SelfInfo.RfBand = 0x00;
	if (WMODE_CAP_5G(pAd->CommonCfg.PhyMode))
		pWscControl->RegData.SelfInfo.RfBand |= WSC_RFBAND_50GHZ;			/* 5.0G */

	if (WMODE_CAP_2G(pAd->CommonCfg.PhyMode))
		pWscControl->RegData.SelfInfo.RfBand |= WSC_RFBAND_24GHZ;			/* 2.4G */

	/* 16. Config Method */
	pWscControl->RegData.SelfInfo.ConfigMethods = cpu2be16(pWscControl->WscConfigMethods);
	/*pWscControl->RegData.EnrolleeInfo.ConfigMethods = cpu2be16(WSC_CONFIG_METHODS);		// Label, Display, PBC */
	/*pWscControl->RegData.EnrolleeInfo.ConfigMethods = cpu2be16(0x0084);		// Label, Display, PBC */

	/* 17. Simple Config State */
#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
		pWscControl->RegData.SelfInfo.ScState = pWscControl->WscConfStatus;
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
		pWscControl->RegData.SelfInfo.ScState = 0x01;
#endif /* CONFIG_STA_SUPPORT */

	/* 18. Device Password ID */
	if (pWscControl->WscMode == WSC_PIN_MODE)
	{
#ifdef IWSC_SUPPORT
		if ((pAd->StaCfg.BssType == BSS_ADHOC) &&
			(pAd->StaCfg.IWscInfo.bLimitedUI == FALSE))
		{
			pWscControl->RegData.SelfInfo.DevPwdId = cpu2be16(DEV_PASS_ID_REG);		/* PIN mode */
		}
		else
#endif /* IWSC_SUPPORT */
		pWscControl->RegData.SelfInfo.DevPwdId = cpu2be16(DEV_PASS_ID_PIN);		/* PIN mode */
	}
#ifdef IWSC_SUPPORT
	else if (pWscControl->WscMode == WSC_SMPBC_MODE)
	{
		pWscControl->RegData.SelfInfo.DevPwdId = cpu2be16(DEV_PASS_ID_SMPBC);	// SMPBC mode
	}
#endif // IWSC_SUPPORT //
	else
	{
		pWscControl->RegData.SelfInfo.DevPwdId = cpu2be16(DEV_PASS_ID_PBC);		/* PBC */
	}

	/* 19. SSID */
#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
		NdisMoveMemory(pWscControl->RegData.SelfInfo.Ssid, pAd->ApCfg.MBSSID[apidx].Ssid, pAd->ApCfg.MBSSID[apidx].SsidLen);
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
		NdisMoveMemory(pWscControl->RegData.SelfInfo.Ssid, pAd->CommonCfg.Ssid, pAd->CommonCfg.SsidLen);
#endif /* CONFIG_STA_SUPPORT */

	/* 20. Primary Device Type */
#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
	{
#ifdef P2P_SUPPORT
		if (pWscControl->EntryIfIdx >= MIN_NET_DEVICE_FOR_P2P_CLI)
		{
			NdisMoveMemory(&pWscControl->RegData.SelfInfo.PriDeviceType, pAd->P2pCfg.DevInfo.PriDeviceType, 8);
		}
		else
#endif /* P2P_SUPPORT */
		if (pWscControl->EntryIfIdx & MIN_NET_DEVICE_FOR_APCLI)
			NdisMoveMemory(&pWscControl->RegData.SelfInfo.PriDeviceType, &STA_Wsc_Pri_Dev_Type[0], 8);
		else
			NdisMoveMemory(&pWscControl->RegData.SelfInfo.PriDeviceType, &AP_Wsc_Pri_Dev_Type[0], 8);
	}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
		NdisMoveMemory(&pWscControl->RegData.SelfInfo.PriDeviceType, &STA_Wsc_Pri_Dev_Type[0], 8);
#endif /* CONFIG_STA_SUPPORT */

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscInitRegistrarPair\n"));	
}


VOID WscSendEapReqId(
	IN	PRTMP_ADAPTER		pAd,
	IN	PMAC_TABLE_ENTRY	pEntry,
	IN  UCHAR				CurOpMode)
{
	UCHAR               Header802_3[14];
	USHORT				Length;
	IEEE8021X_FRAME		Ieee_8021x;
	EAP_FRAME			EapFrame;
	UCHAR				*pOutBuffer = NULL;
	ULONG				FrameLen = 0;
    UCHAR				Data[] = "hello";
    UCHAR				Id;
	PWSC_CTRL			pWpsCtrl = NULL;
	
	NdisZeroMemory(Header802_3,sizeof(UCHAR)*14);
	
	/* 1. Send EAP-Rsp Id */
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscSendEapReqId\n"));

#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
	{
		pWpsCtrl = &pAd->ApCfg.MBSSID[pEntry->func_tb_idx].WscControl;
		MAKE_802_3_HEADER(Header802_3, 
						  &pEntry->Addr[0], 
						  &pAd->ApCfg.MBSSID[pEntry->func_tb_idx].wdev.bssid[0], 
						  EAPOL);
	}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
	{
		pWpsCtrl = &pAd->StaCfg.WscControl;
		MAKE_802_3_HEADER(Header802_3, 
						  &pEntry->Addr[0], 
						  &pAd->CurrentAddress[0], 
						  EAPOL);
	}
#endif /* CONFIG_STA_SUPPORT */

	if (pWpsCtrl == NULL)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("pWpsCtrl == NULL!\n"));
		return;
	}

	/* Length, -1 NULL pointer of string */
	Length = sizeof(EAP_FRAME) + sizeof(Data) - 1;
	
	/* Zero 802.1x body */
	NdisZeroMemory(&Ieee_8021x, sizeof(Ieee_8021x));
	Ieee_8021x.Version = EAPOL_VER;
	Ieee_8021x.Type    = EAPPacket;
	Ieee_8021x.Length  = cpu2be16(Length);

	/* Zero EAP frame */
	NdisZeroMemory(&EapFrame, sizeof(EapFrame));
    /* RFC 3748 Ch 4.1: recommended to initalize Identifier with a
	 * random number */
	Id = RandomByte(pAd);
    if (Id == pWpsCtrl->lastId)
        Id += 1;
	EapFrame.Code   = EAP_CODE_REQ;
	EapFrame.Id     = Id;
	EapFrame.Length = cpu2be16(Length);
	EapFrame.Type   = EAP_TYPE_ID;
    pWpsCtrl->lastId = Id;
	
    /* Out buffer for transmitting EAP-Req(Identity) */
	os_alloc_mem(NULL, (UCHAR **)&pOutBuffer, MAX_LEN_OF_MLME_BUFFER);
    if(pOutBuffer == NULL)
        return;

	FrameLen = 0;
	
	/* Make	 Transmitting frame */
	MakeOutgoingFrame(pOutBuffer, &FrameLen,
					   sizeof(IEEE8021X_FRAME), &Ieee_8021x,
					   sizeof(EapFrame), &EapFrame, 
					   (sizeof(Data) - 1), Data,
		END_OF_ARGS);

	/* Copy frame to Tx ring */
	RTMPToWirelessSta(pAd, pEntry, Header802_3, sizeof(Header802_3), (PUCHAR)pOutBuffer, FrameLen, TRUE);

	pWpsCtrl->WscRetryCount = 0;
	if (pOutBuffer)
		os_free_mem(NULL, pOutBuffer);
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscSendEapReqId\n"));	
}

/*
	========================================================================
	
	Routine Description:
		Send EAPoL-Start packet to AP.

	Arguments:
		pAd         - NIC Adapter pointer
		
	Return Value:
		None
		
	IRQL = DISPATCH_LEVEL
	
	Note:
		Actions after link up
		1. Change the correct parameters
		2. Send EAPOL - START
		
	========================================================================
*/
VOID    WscSendEapolStart(
	IN	PRTMP_ADAPTER	pAdapter,
	IN  PUCHAR          pBssid,
	IN  UCHAR			CurOpMode)
{
	IEEE8021X_FRAME		Packet;
	UCHAR               Header802_3[14];
	MAC_TABLE_ENTRY     *pEntry;
	
	pEntry = MacTableLookup(pAdapter, pBssid);
	
#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
	{
		if (pAdapter->StaCfg.WscControl.WscState >= WSC_STATE_WAIT_WSC_START)
			return;
	}
#endif /* CONFIG_STA_SUPPORT */

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscSendEapolStart\n"));

	NdisZeroMemory(Header802_3,sizeof(UCHAR)*14);

	/* 1. Change the authentication to open and encryption to none if necessary. */

	/* init 802.3 header and Fill Packet */
#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
	if (CurOpMode == AP_MODE)
	{
		MAKE_802_3_HEADER(Header802_3, 
						  pBssid,
						  &pAdapter->ApCfg.ApCliTab[0].wdev.if_addr[0], 
						  EAPOL);
	}
#endif /* APCLI_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
		MAKE_802_3_HEADER(Header802_3, pBssid, &pAdapter->CurrentAddress[0], EAPOL);
#endif /* CONFIG_STA_SUPPORT */
	
	/* Zero message 2 body */
	NdisZeroMemory(&Packet, sizeof(Packet));
	Packet.Version = EAPOL_VER;
	Packet.Type    = EAPOLStart;
	Packet.Length  = cpu2be16(0);
	
	if (pEntry)
		RTMPToWirelessSta(pAdapter, pEntry, Header802_3, sizeof(Header802_3), (PUCHAR)&Packet, 4, TRUE);

#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
	if (CurOpMode == AP_MODE)
	{
		/* Update WSC status */
		pAdapter->ApCfg.ApCliTab[0].WscControl.WscStatus = STATUS_WSC_EAPOL_START_SENT;
		pAdapter->ApCfg.ApCliTab[0].WscControl.WscState = WSC_STATE_WAIT_REQ_ID;
		if (!pAdapter->ApCfg.ApCliTab[0].WscControl.EapolTimerRunning)
		{
			pAdapter->ApCfg.ApCliTab[0].WscControl.EapolTimerRunning = TRUE;
			RTMPSetTimer(&pAdapter->ApCfg.ApCliTab[0].WscControl.EapolTimer, WSC_EAPOL_START_TIME_OUT);
		}
	}
#endif /* APCLI_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
	{
		/* Update WSC status */
		pAdapter->StaCfg.WscControl.WscStatus = STATUS_WSC_EAPOL_START_SENT;
		pAdapter->StaCfg.WscControl.WscState = WSC_STATE_WAIT_REQ_ID;
		if (!pAdapter->StaCfg.WscControl.EapolTimerRunning)
		{
			pAdapter->StaCfg.WscControl.EapolTimerRunning = TRUE;
			RTMPSetTimer(&pAdapter->StaCfg.WscControl.EapolTimer, 2000);
		}
	}
#endif /* CONFIG_STA_SUPPORT */    
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscSendEapolStart\n"));
}


VOID WscSendEapRspId(
	IN PRTMP_ADAPTER pAdapter,
	IN PMAC_TABLE_ENTRY pEntry,
	IN PWSC_CTRL pWscControl)
{
	UCHAR Header802_3[14];
	USHORT Length = 0;
	IEEE8021X_FRAME Ieee_8021x;
	EAP_FRAME EapFrame;
	UCHAR *pOutBuffer = NULL;
	ULONG FrameLen = 0;
	UCHAR regIdentity[] = "WFA-SimpleConfig-Registrar-1-0";
	UCHAR enrIdentity[] = "WFA-SimpleConfig-Enrollee-1-0";
	UCHAR CurOpMode = 0xff;

	NdisZeroMemory(Header802_3,sizeof(UCHAR)*14);

	/* 1. Send EAP-Rsp Id */
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscSendEapRspId\n"));

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAdapter)
		CurOpMode = AP_MODE;
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAdapter)
		CurOpMode = STA_MODE;
#ifdef P2P_SUPPORT
	if (pWscControl->EntryIfIdx != BSS0)
		CurOpMode = AP_MODE;
#endif /* P2P_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */
	if(CurOpMode == 0xff)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_WARN, ("Unkown OpMode (CurOpMode=0x%02x)\n", CurOpMode));
		return;
	}

#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
	if (CurOpMode == AP_MODE)
	{
		MAKE_802_3_HEADER(Header802_3, 
						  &pEntry->Addr[0],
						  &pAdapter->ApCfg.ApCliTab[0].wdev.if_addr[0], 
						  EAPOL);
		Length = sizeof(EAP_FRAME) + sizeof(enrIdentity) - 1;
		pWscControl->WscConfMode = WSC_ENROLLEE; /* Ap Client only support Enrollee now. 20070518 */
	}
#endif /* APCLI_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT    
	if (CurOpMode == STA_MODE)
	{
		/* init 802.3 header and Fill Packet */
		if (INFRA_ON(pAdapter))
		{
			MAKE_802_3_HEADER(Header802_3, &pAdapter->CommonCfg.Bssid[0], &pAdapter->CurrentAddress[0], EAPOL);
		}
		else
		{
			MAKE_802_3_HEADER(Header802_3, &pWscControl->WscPeerMAC[0], &pAdapter->CurrentAddress[0], EAPOL);
		}
		
		/* Length, -1 NULL pointer of string */
		if (pWscControl->WscConfMode == WSC_ENROLLEE)	
			Length = sizeof(EAP_FRAME) + sizeof(enrIdentity) - 1;
		else if (pWscControl->WscConfMode == WSC_REGISTRAR)
			Length = sizeof(EAP_FRAME) + sizeof(regIdentity) - 1;
	}

#endif /* CONFIG_STA_SUPPORT */
   	
	/* Zero 802.1x body */
	NdisZeroMemory(&Ieee_8021x, sizeof(Ieee_8021x));
	Ieee_8021x.Version = EAPOL_VER;
	Ieee_8021x.Type    = EAPPacket;
	Ieee_8021x.Length  = cpu2be16(Length);

	/* Zero EAP frame */
	NdisZeroMemory(&EapFrame, sizeof(EapFrame));
	EapFrame.Code   = EAP_CODE_RSP;
	EapFrame.Id     = pWscControl->lastId;
	EapFrame.Length = cpu2be16(Length);
	EapFrame.Type   = EAP_TYPE_ID;

    /* Out buffer for transmitting EAP-Req(Identity) */
	os_alloc_mem(NULL, (UCHAR **)&pOutBuffer, MAX_LEN_OF_MLME_BUFFER);
    if(pOutBuffer == NULL)
        return;

	FrameLen = 0;

    if (pWscControl->WscConfMode == WSC_REGISTRAR)
    {
    	/* Make tx frame */
	MakeOutgoingFrame(pOutBuffer, &FrameLen,
						sizeof(IEEE8021X_FRAME), &Ieee_8021x,
						sizeof(EapFrame), &EapFrame,
						(sizeof(regIdentity) - 1), regIdentity,
						END_OF_ARGS);
    }
    else if (pWscControl->WscConfMode == WSC_ENROLLEE)
    {
#ifdef IWSC_SUPPORT
		if (pAdapter->StaCfg.BssType == BSS_ADHOC)
			pWscControl->WscConfStatus = WSC_SCSTATE_UNCONFIGURED;
#endif /* IWSC_SUPPORT */
        /* Make	 Transmitting frame */
    	MakeOutgoingFrame(pOutBuffer, &FrameLen,
    		sizeof(IEEE8021X_FRAME), &Ieee_8021x,
    		sizeof(EapFrame), &EapFrame, 
    		(sizeof(enrIdentity) - 1), enrIdentity,
    		END_OF_ARGS);
    }
    else
    {
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscConfMode(%d) is not WSC_REGISTRAR nor WSC_ENROLLEE.\n", pWscControl->WscConfMode));	
        goto out;
    }

	/* Copy frame to Tx ring */
#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
	if (CurOpMode == AP_MODE)
	{
		RTMPToWirelessSta(pAdapter, pEntry, Header802_3, sizeof(Header802_3), (PUCHAR)pOutBuffer, FrameLen, TRUE);
	}
#endif /* APCLI_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT	
	if (CurOpMode == STA_MODE)
		RTMPToWirelessSta((PRTMP_ADAPTER)pWscControl->pAd, &pAdapter->MacTab.Content[BSSID_WCID],
							Header802_3, LENGTH_802_3, (PUCHAR)pOutBuffer, FrameLen, TRUE);
#endif /* CONFIG_STA_SUPPORT */

	pWscControl->WscRetryCount = 0;
    if (!pWscControl->EapolTimerRunning)
    {
        pWscControl->EapolTimerRunning = TRUE;
        RTMPSetTimer(&pWscControl->EapolTimer, WSC_EAP_ID_TIME_OUT);
    }
out:
	if (pOutBuffer)
		os_free_mem(NULL, pOutBuffer);
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscSendEapRspId\n"));	
}

VOID WscUPnPErrHandle(
	IN PRTMP_ADAPTER pAd,
	IN  PWSC_CTRL	pWscControl,
	IN UINT eventID)
{
	int dataLen;
	UCHAR *pWscData;
	UCHAR	CurOpMode;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Into WscUPnPErrHandle, send WSC_OPCODE_UPNP_CTRL with eventID=0x%x!\n", eventID));

#ifdef P2P_SUPPORT
	return;
#endif /* P2P_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		CurOpMode = AP_MODE;
#endif // CONFIG_AP_SUPPORT //

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		CurOpMode = STA_MODE;
#ifdef P2P_SUPPORT
	if (pWscControl->EntryIfIdx != BSS0)
		CurOpMode = AP_MODE;
#endif /* P2P_SUPPORT */
#endif // CONFIG_STA_SUPPORT //

	os_alloc_mem(NULL, (UCHAR **)&pWscData, WSC_MAX_DATA_LEN);
	if (pWscData != NULL)
	{
		NdisZeroMemory(pWscData, WSC_MAX_DATA_LEN);
	
		dataLen = BuildMessageNACK(pAd, pWscControl, pWscData);
		WscSendUPnPMessage(pAd, (pWscControl->EntryIfIdx & 0x0F), 
								WSC_OPCODE_UPNP_DATA, WSC_UPNP_DATA_SUB_NORMAL, 
								pWscData, dataLen, eventID, 0, NULL, CurOpMode);
	
		os_free_mem(NULL, pWscData);
	} 
	else 
	{
		WscSendUPnPMessage(pAd, (pWscControl->EntryIfIdx & 0x0F), 
								WSC_OPCODE_UPNP_CTRL, 0, NULL, 0, eventID, 0, NULL, CurOpMode);
	}
}

/*
	Format of iwcustom msg WSC clientJoin message:
		1. SSID which station want to probe(32 bytes):
			<SSID string>
			*If the length if SSID string is small than 32 bytes, fill 0x0 for remaining bytes.
		2. sender MAC address(6 bytes):
		3. Status:
			Set as 1 means change APStatus as 1. 
			Set as 2 means change STAStatus as 1.
			Set as 3 means trigger msg.
								
			32         6        1 
		+----------+--------+------+
		|SSIDString| SrcMAC |Status|
*/
int WscSendUPnPConfReqMsg(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR apIdx,
	IN PUCHAR ssidStr,
	IN PUCHAR macAddr,
	IN INT	  Status,
	IN UINT   eventID,
	IN UCHAR  CurOpMode)
{
	UCHAR pData[39] = {0};
	
#ifdef P2P_SUPPORT
	if (apIdx >= MIN_NET_DEVICE_FOR_P2P_CLI)
		return 0;
#endif /* P2P_SUPPORT */

	strncpy((RTMP_STRING *) pData, (RTMP_STRING *)ssidStr, strlen((RTMP_STRING *) ssidStr));
	NdisMoveMemory(&pData[32], macAddr, MAC_ADDR_LEN);
	pData[38] = Status;
	WscSendUPnPMessage(pAd, apIdx, WSC_OPCODE_UPNP_MGMT, WSC_UPNP_MGMT_SUB_CONFIG_REQ, 
							&pData[0], 39, eventID, 0, NULL, CurOpMode);

	return 0;
}

	
/*
	NETLINK tunnel msg format send to WSCUPnP handler in user space:
	1. Signature of following string(Not include the quote, 8 bytes)
			"RAWSCMSG"
	2. eID: eventID (4 bytes)
			the ID of this message(4 bytes)
	3. aID: ackID (4 bytes)
			means that which event ID this mesage was response to.
	4. TL:  Message Total Length (4 bytes) 
			Total length of this message.
	5. F:   Flag (2 bytes)
			used to notify some specific character of this msg segment.
				Bit 1: fragment
					set as 1 if netlink layer have more segment of this Msg need to send.
				Bit 2~15: reserve, should set as 0 now.
	5. SL:  Segment Length(2 bytes)
			msg actual length in this segment, The SL may not equal the "TL" field if "F" ==1
	6. devMac: device mac address(6 bytes)
			Indicate the netdevice which this msg belong. For the wscd in user space will 
			depends this address dispatch the msg to correct UPnP Device instance to handle it.
	7. "WSC_MSG" info:

                 8                 4       4       4      2    2        6      variable length(MAXIMUM=232)
	+------------+----+----+----+--+--+------+------------------------+
	|  Signature       |eID  |aID  | TL   | F | SL|devMac| WSC_MSG                          |

*/
int WscSendUPnPMessage(
	IN PRTMP_ADAPTER	pAd, 
	IN UCHAR			devIfIdx,
	IN USHORT			msgType,
	IN USHORT			msgSubType,
	IN PUCHAR			pData,
	IN INT				dataLen,
	IN UINT				eventID,
	IN UINT				toIPAddr,
	IN PUCHAR			pMACAddr,
	IN UCHAR			CurOpMode)
{
/*	union iwreq_data wrqu; */
	RTMP_WSC_NLMSG_HDR *pNLMsgHdr;
	RTMP_WSC_MSG_HDR *pWscMsgHdr;
	
	UCHAR hdrBuf[42]; /*RTMP_WSC_NLMSG_HDR_LEN + RTMP_WSC_MSG_HDR_LEN */
	int /*totalLen,*/ leftLen, copyLen;
	PUCHAR pBuf = NULL, pBufPtr = NULL, pPos = NULL;
	PUCHAR	pDevAddr = NULL;
#ifdef CONFIG_AP_SUPPORT
	UCHAR	bssIdx = devIfIdx;
#endif /* CONFIG_AP_SUPPORT */
	ULONG Now;

#ifdef P2P_SUPPORT
	if (devIfIdx >= MIN_NET_DEVICE_FOR_P2P_CLI)
		return 0;
#endif /* P2P_SUPPORT */

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscSendUPnPMessage\n"));

	if ((msgType & WSC_OPCODE_UPNP_MASK) != WSC_OPCODE_UPNP_MASK)
		return FALSE;
		
#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
	{
#ifdef APCLI_SUPPORT
		if (devIfIdx & MIN_NET_DEVICE_FOR_APCLI)
		{
			bssIdx &= (~MIN_NET_DEVICE_FOR_APCLI);
			if (bssIdx >= MAX_APCLI_NUM)
				return FALSE;
			pDevAddr = &pAd->ApCfg.ApCliTab[bssIdx].wdev.if_addr[0];
		}
		else
#endif /* APCLI_SUPPORT */
		pDevAddr = &pAd->ApCfg.MBSSID[bssIdx].wdev.bssid[0];
	}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
	{
		pDevAddr = &pAd->CurrentAddress[0];
	}
#endif /* CONFIG_STA_SUPPORT */

	if (pDevAddr == NULL)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("pDevAddr == NULL!\n"));
		return FALSE;
	}

	/*Prepare the NLMsg header */
	memset(hdrBuf, 0, sizeof(hdrBuf));
	pNLMsgHdr = (RTMP_WSC_NLMSG_HDR *)hdrBuf;
	memcpy(pNLMsgHdr, WSC_MSG_SIGNATURE, RTMP_WSC_NLMSG_SIGNATURE_LEN);

	NdisGetSystemUpTime(&Now);
	pNLMsgHdr->envID = Now;
	pNLMsgHdr->ackID = eventID;
	pNLMsgHdr->msgLen = dataLen + RTMP_WSC_MSG_HDR_LEN;

	/* 
		In order to support multiple wscd, we need this new field to notify 
		the wscd which interface this msg send from.
	*/
	NdisMoveMemory(&pNLMsgHdr->devAddr[0],  pDevAddr, MAC_ADDR_LEN);

	/*Prepare the WscMsg header */
	pWscMsgHdr = (RTMP_WSC_MSG_HDR *)(hdrBuf + sizeof(RTMP_WSC_NLMSG_HDR));
	switch(msgType)
	{
		case WSC_OPCODE_UPNP_DATA:
				pWscMsgHdr->msgType = WSC_OPCODE_UPNP_DATA;
				break;
		case WSC_OPCODE_UPNP_MGMT:
				pWscMsgHdr->msgType = WSC_OPCODE_UPNP_MGMT;
				break;
		case WSC_OPCODE_UPNP_CTRL:
				pWscMsgHdr->msgType = WSC_OPCODE_UPNP_CTRL;
				break;
		default:
				return FALSE;
	}
	pWscMsgHdr->msgSubType = msgSubType;
	pWscMsgHdr->ipAddr = toIPAddr;
	pWscMsgHdr->msgLen = dataLen;
	
	if ((pWscMsgHdr->msgType == WSC_OPCODE_UPNP_DATA) && 
		(eventID == 0) &&
		(pMACAddr != NULL) && 
		(NdisEqualMemory(pMACAddr, ZERO_MAC_ADDR, MAC_ADDR_LEN) == FALSE))
	{
		pWscMsgHdr->msgSubType |= WSC_UPNP_DATA_SUB_INCLUDE_MAC;
		pNLMsgHdr->msgLen += MAC_ADDR_LEN;
		pWscMsgHdr->msgLen += MAC_ADDR_LEN;
	}
	
	/*Allocate memory and copy the msg. */
	/*totalLen =*/ leftLen = pNLMsgHdr->msgLen;
	pPos = pData;
	os_alloc_mem(NULL, (UCHAR **)&pBuf, IWEVCUSTOM_MSG_MAX_LEN);
	if (pBuf != NULL)
	{
		int firstSeg = 1;
	
		while(leftLen)
		{
			/*Prepare the payload */
			memset(pBuf, 0, IWEVCUSTOM_MSG_MAX_LEN);

			pNLMsgHdr->segLen = (leftLen > IWEVCUSTOM_PAYLOD_MAX_LEN ? IWEVCUSTOM_PAYLOD_MAX_LEN : leftLen);
			leftLen -= pNLMsgHdr->segLen;
			pNLMsgHdr->flags = (leftLen > 0 ? 1 : 0);

			memcpy(pBuf, pNLMsgHdr, RTMP_WSC_NLMSG_HDR_LEN);
			pBufPtr = &pBuf[RTMP_WSC_NLMSG_HDR_LEN];

			if(firstSeg){
				memcpy(pBufPtr, pWscMsgHdr, RTMP_WSC_MSG_HDR_LEN);
				pBufPtr += RTMP_WSC_MSG_HDR_LEN;
				copyLen = (pNLMsgHdr->segLen - RTMP_WSC_MSG_HDR_LEN);
				if ((pWscMsgHdr->msgSubType & WSC_UPNP_DATA_SUB_INCLUDE_MAC) == WSC_UPNP_DATA_SUB_INCLUDE_MAC)
				{
					NdisMoveMemory(pBufPtr, pMACAddr, MAC_ADDR_LEN);
					pBufPtr += MAC_ADDR_LEN;
					copyLen -= MAC_ADDR_LEN;
				}
				NdisMoveMemory(pBufPtr, pPos, copyLen);
				pPos += copyLen;
				firstSeg = 0;
			} else {
				NdisMoveMemory(pBufPtr, pPos, pNLMsgHdr->segLen);
				pPos += pNLMsgHdr->segLen;
			}
						
			/*Send WSC Msg to wscd, msg length = pNLMsgHdr->segLen + sizeof(RTMP_WSC_NLMSG_HDR) */
			RtmpOSWrielessEventSend(pAd->net_dev, RT_WLAN_EVENT_CUSTOM, RT_WSC_UPNP_EVENT_FLAG, NULL, pBuf, pNLMsgHdr->segLen + sizeof(RTMP_WSC_NLMSG_HDR));
		}
		
		os_free_mem(NULL, pBuf);
	}

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscSendUPnPMessage\n"));
	return TRUE;
}


VOID	WscSendMessage(
	IN	PRTMP_ADAPTER		pAdapter, 
	IN  UCHAR               OpCode,
	IN  PUCHAR				pData,
	IN  INT					Len,
	IN  PWSC_CTRL           pWscControl,
	IN  UCHAR               OpMode,
	IN  UCHAR               EapType)
{
	/* Inb-EAP Message */
	UCHAR               Header802_3[14];
	USHORT				Length, MsgLen;
	IEEE8021X_FRAME		Ieee_8021x;
	EAP_FRAME			EapFrame;
	WSC_FRAME			WscFrame;
	UCHAR				*pOutBuffer = NULL;
	ULONG				FrameLen = 0;
	MAC_TABLE_ENTRY     *pEntry;
#ifdef CONFIG_AP_SUPPORT
	UCHAR				bssIdx = (pWscControl->EntryIfIdx & 0x0F);
#endif /* CONFIG_AP_SUPPORT */
	UCHAR				CurOpMode = 0xFF;
    
	if ((Len <= 0) && (OpCode != WSC_OPCODE_START) && (OpCode != WSC_OPCODE_FRAG_ACK))
		return;

	/* Send message */
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscSendMessage\n"));

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAdapter)
		CurOpMode = AP_MODE;
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAdapter)
		CurOpMode = STA_MODE;
#ifdef P2P_SUPPORT
	if (pWscControl->EntryIfIdx != BSS0)
		CurOpMode = AP_MODE;
#endif /* P2P_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */
	NdisZeroMemory(Header802_3,sizeof(UCHAR)*14);

#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
	{
	        if (OpMode == AP_MODE)
	        {
				MAKE_802_3_HEADER(Header802_3, &pWscControl->EntryAddr[0], &pAdapter->ApCfg.MBSSID[bssIdx].wdev.bssid[0], EAPOL);
	        }
#ifdef APCLI_SUPPORT
	        else if (OpMode == AP_CLIENT_MODE)
	        {
				MAKE_802_3_HEADER(Header802_3, &pWscControl->EntryAddr[0], &pAdapter->ApCfg.ApCliTab[0].wdev.if_addr[0], EAPOL);
	        }
#endif /* APCLI_SUPPORT */
	}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
	{
	        if (OpMode == STA_MODE)
	        {
	        	if (INFRA_ON(pAdapter))
	        	{
					MAKE_802_3_HEADER(Header802_3, &pAdapter->CommonCfg.Bssid[0], &pAdapter->CurrentAddress[0], EAPOL);
	        	}
				else
				{
					MAKE_802_3_HEADER(Header802_3, &pWscControl->EntryAddr[0], &pAdapter->CurrentAddress[0], EAPOL);
				}
	        }
	}
#endif /* CONFIG_STA_SUPPORT */

    	/* Length = EAP + WSC_Frame + Payload */
    	Length = sizeof(EAP_FRAME) + sizeof(WSC_FRAME) + Len;
    	
		if (pWscControl->bWscFragment && (pWscControl->bWscFirstOne))
		{
			Length += 2;
			MsgLen = pWscControl->WscTxBufLen + Len;
			MsgLen = htons(MsgLen);
		}

    	/* Zero 802.1x body */
    	NdisZeroMemory(&Ieee_8021x, sizeof(Ieee_8021x));
    	Ieee_8021x.Version = EAPOL_VER;
    	Ieee_8021x.Type    = EAPPacket;
    	Ieee_8021x.Length  = cpu2be16(Length);

    	/* Zero EAP frame */
    	NdisZeroMemory(&EapFrame, sizeof(EapFrame));

        if (EapType == EAP_CODE_REQ)
        {
        	EapFrame.Code   = EAP_CODE_REQ;
        	EapFrame.Id     = ++(pWscControl->lastId);
        }
        else
        {
            EapFrame.Code   = EAP_CODE_RSP;
            EapFrame.Id     = pWscControl->lastId; /* same as eap_req id */
        }

    	EapFrame.Length = cpu2be16(Length);
    	EapFrame.Type   = EAP_TYPE_WSC;

    	/* Zero WSC Frame */
    	NdisZeroMemory(&WscFrame, sizeof(WscFrame));
    	WscFrame.SMI[0] = 0x00;
    	WscFrame.SMI[1] = 0x37;
    	WscFrame.SMI[2] = 0x2A;
    	WscFrame.VendorType = cpu2be32(WSC_VENDOR_TYPE);
    	WscFrame.OpCode = OpCode;
    	WscFrame.Flags  = 0x00;
		if (pWscControl->bWscFragment && (pWscControl->bWscLastOne == FALSE))
			WscFrame.Flags  |= WSC_MSG_FLAG_MF;

		if (pWscControl->bWscFragment && (pWscControl->bWscFirstOne))
		{
			WscFrame.Flags  |= WSC_MSG_FLAG_LF;
		}

        /* Out buffer for transmitting message */
	os_alloc_mem(NULL, (UCHAR **)&pOutBuffer, MAX_LEN_OF_MLME_BUFFER);
    if(pOutBuffer == NULL)
            return;

    	FrameLen = 0;
    	
    	/* Make	 Transmitting frame */
    	if (pData && (Len > 0))
    	{
    		if (pWscControl->bWscFragment && (pWscControl->bWscFirstOne))
    		{
    			UCHAR	LF_Len = 2;
				ULONG	TmpLen = 0;

				pWscControl->bWscFirstOne = FALSE;
    			MakeOutgoingFrame(pOutBuffer, &TmpLen,
	    						sizeof(IEEE8021X_FRAME), &Ieee_8021x,
	    						sizeof(EapFrame), &EapFrame, 
	    						sizeof(WscFrame), &WscFrame,
	    						LF_Len, &MsgLen,
	        					END_OF_ARGS);
				
				FrameLen += TmpLen;

				MakeOutgoingFrame(pOutBuffer + FrameLen, &TmpLen,
		    						Len, pData,
		        					END_OF_ARGS);

				FrameLen += TmpLen;
    		}
			else
			{
				MakeOutgoingFrame(pOutBuffer, &FrameLen,
		    						sizeof(IEEE8021X_FRAME), &Ieee_8021x,
		    						sizeof(EapFrame), &EapFrame, 
		    						sizeof(WscFrame), &WscFrame,
		    						Len, pData,
		        					END_OF_ARGS);
			}
    	}
        else
            MakeOutgoingFrame(pOutBuffer, &FrameLen,
							sizeof(IEEE8021X_FRAME), &Ieee_8021x,
							sizeof(EapFrame), &EapFrame, 
							sizeof(WscFrame), &WscFrame, 
							END_OF_ARGS);

	/* Copy frame to Tx ring */
	pEntry = MacTableLookup(pAdapter, &pWscControl->EntryAddr[0]);

	if (pEntry)
		RTMPToWirelessSta(pAdapter, pEntry, Header802_3, sizeof(Header802_3), (PUCHAR)pOutBuffer, FrameLen, TRUE);
	else
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_WARN, ("pEntry is NULL\n"));
    	
	if (pOutBuffer)
		os_free_mem(NULL, pOutBuffer);
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscSendMessage\n"));	
}

VOID WscBuildBeaconIE(
	IN	PRTMP_ADAPTER	pAd, 
	IN	UCHAR b_configured,
	IN	BOOLEAN b_selRegistrar,
	IN	USHORT devPwdId,
	IN	USHORT selRegCfgMethods,
	IN  UCHAR apidx,
	IN  UCHAR *pAuthorizedMACs,
	IN  UCHAR AuthorizedMACsLen,
	IN  UCHAR	CurOpMode)
{
	WSC_IE_HEADER 	ieHdr;
/*	UCHAR 			Data[256]; */
	UCHAR 			*Data = NULL;
	PUCHAR			pData;
	INT				Len = 0, templen = 0;
    USHORT          tempVal = 0;
	PWSC_CTRL		pWpsCtrl = NULL;
	PWSC_REG_DATA	pReg = NULL;
#ifdef EASY_CONFIG_SETUP
	UCHAR			VendorExt[32] = {0}, MIC[16] = {0};
#endif /* EASY_CONFIG_SETUP */


	/* allocate memory */
	os_alloc_mem(NULL, (UCHAR **)&Data, 256);
	if (Data == NULL)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: Allocate memory fail!!!\n", __FUNCTION__));
		return;
	}

#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
		pWpsCtrl = &pAd->ApCfg.MBSSID[apidx & 0x0F].WscControl;
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
		pWpsCtrl = &pAd->StaCfg.WscControl;
#endif /* CONFIG_STA_SUPPORT */

	pReg = &pWpsCtrl->RegData;
 
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscBuildBeaconIE\n"));
	/* WSC IE HEader */
	ieHdr.elemId = 221;
	ieHdr.length = 4;
	ieHdr.oui[0] = 0x00; ieHdr.oui[1] = 0x50; ieHdr.oui[2] = 0xF2; 
#ifdef IWSC_SUPPORT
	if ((CurOpMode == STA_MODE) && 
		(pAd->StaCfg.BssType == BSS_ADHOC))
		ieHdr.oui[3] = 0x10;
	else
#endif /* IWSC_SUPPORT */
	ieHdr.oui[3] = 0x04;

	pData = (PUCHAR) &Data[0];
	Len = 0;
	
	/* 1. Version */
	templen = AppendWSCTLV(WSC_ID_VERSION, pData, &pReg->SelfInfo.Version, 0);
	pData += templen;
	Len   += templen;

	/* 2. Simple Config State */
	templen = AppendWSCTLV(WSC_ID_SC_STATE, pData, (UINT8 *)&b_configured, 0);
	pData += templen;
	Len   += templen;
	
#ifdef CONFIG_AP_SUPPORT
#ifdef WSC_V2_SUPPORT
	if ((CurOpMode == AP_MODE) && pWpsCtrl->bSetupLock)
	{
		// AP Setup Lock
		templen = AppendWSCTLV(WSC_ID_AP_SETUP_LOCKED, pData, (UINT8 *)&pWpsCtrl->bSetupLock, 0);
		pData += templen;
		Len   += templen;
	}
#endif /* WSC_V2_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

	if ( b_selRegistrar )
	{
		/* 3.Selected Registrar */
		templen = AppendWSCTLV(WSC_ID_SEL_REGISTRAR, pData, (UINT8 *)&b_selRegistrar, 0);
    	pData += templen;
    	Len   += templen;

		/*4. Device Password ID */
		tempVal = htons(devPwdId);
		templen = AppendWSCTLV(WSC_ID_DEVICE_PWD_ID, pData, (UINT8 *)&tempVal, 0);
    	pData += templen;
    	Len   += templen;

		/* 5. Selected Registrar Config Methods */
		tempVal = selRegCfgMethods;
#ifdef IWSC_SUPPORT
		if (CurOpMode == STA_MODE)
		{
			if (pAd->StaCfg.WscControl.WscMode == WSC_PIN_MODE)
			{
				tempVal &= 0x200F;
			}
			else
			{
				tempVal &= 0x02F0;
			}
			if (pAd->StaCfg.IWscInfo.bLimitedUI)
			{
				tempVal &= (~WSC_CONFMET_KEYPAD);
			}
			else
			{
				tempVal |= WSC_CONFMET_KEYPAD;
			}
		}
#endif /* IWSC_SUPPORT */
		tempVal = htons(tempVal);
		templen = AppendWSCTLV(WSC_ID_SEL_REG_CFG_METHODS, pData, (UINT8 *)&tempVal, 0);
    	pData += templen;
    	Len   += templen;
	}

	/* 6. UUID last 6 bytes use MAC */
	templen = AppendWSCTLV(WSC_ID_UUID_E, pData, &pWpsCtrl->Wsc_Uuid_E[0], 0);
	pData += templen;
	Len   += templen;

	/* 7. RF Bands */
	if (CurOpMode == AP_MODE)
	{
		if (WMODE_CAP_5G(pAd->CommonCfg.PhyMode))
			tempVal = 2;
		else
			tempVal = 1;
	}
	else
	{
		if (pAd->CommonCfg.Channel > 14)
			tempVal = 2;
		else
			tempVal = 1;
	}

#ifdef RT_BIG_ENDIAN
	tempVal =SWAP16(tempVal);
#endif /* RT_BIG_ENDIAN */
	templen = AppendWSCTLV(WSC_ID_RF_BAND, pData, (UINT8 *)&tempVal, 0);
	pData += templen;
	Len   += templen;

#ifdef IWSC_SUPPORT
	if ((pAd->StaCfg.BssType == BSS_ADHOC) &&
		(CurOpMode == STA_MODE))
	{
		UCHAR respType;
			
		// Connection Type Flag ESS
		templen = AppendWSCTLV(WSC_ID_CONN_TYPE, pData, (UINT8 *)&pReg->SelfInfo.ConnTypeFlags, 0);
		pData += templen;
		Len   += templen;

#ifdef IWSC_TEST_SUPPORT
		/*
			This modification is for Broadcom test bed.
			Broadcom test bed use same buffer to record IWSC IE from Beacon and Probe Response.
			But the content of IWSC IE in Beacon is different from Probe Response.
		*/
		if ((pWpsCtrl->WscMode == WSC_SMPBC_MODE) &&
			(pWpsCtrl->WscConfMode == WSC_REGISTRAR))
		{
			BOOLEAN bEntryAcceptable = FALSE;
			BOOLEAN bRegistrationReady = TRUE;
			PIWSC_INFO pIWscInfo = NULL;
			
			pIWscInfo = &pAd->StaCfg.IWscInfo;
			if (pIWscInfo->bIWscEntryTimerRunning)
				bEntryAcceptable = TRUE;

			/* Entry Acceptable (only for IBSS) */
			templen = AppendWSCTLV(WSC_ID_ENTRY_ACCEPTABLE, pData, (UINT8 *)&bEntryAcceptable, 0);
			pData += templen;
			Len   += templen;

			if (pWpsCtrl->EapMsgRunning)
				bRegistrationReady = FALSE;
			
			/* Registration Ready (only for IBSS) */
			templen = AppendWSCTLV(WSC_ID_REGISTRATON_READY, pData, (UINT8 *)&bRegistrationReady, 0);
			pData += templen;
			Len   += templen;
		}
#endif /* IWSC_TEST_SUPPORT */		
		/* IWSC IP Address Configuration */
		tempVal = htons(pAd->StaCfg.IWscInfo.IpConfMethod);
		templen = AppendWSCTLV(WSC_ID_IP_ADDR_CONF_METHOD, pData, (UINT8 *)&tempVal, 0);
		pData += templen;
		Len   += templen;			
#ifdef IWSC_TEST_SUPPORT
		/*
			This modification is for Broadcom test bed.
			Broadcom test bed use same buffer to record IWSC IE from Beacon and Probe Response.
			But the content of IWSC IE in Beacon is different from Probe Response.
		*/

		/* Response Type WSC_ID_RESP_TYPE */
		if (pAd->StaCfg.WscControl.WscConfMode == WSC_REGISTRAR)
			respType = WSC_MSGTYPE_REGISTRAR;
		else
			respType = WSC_MSGTYPE_ENROLLEE_OPEN_8021X;			
		templen = AppendWSCTLV(WSC_ID_RESP_TYPE, pData, (UINT8 *)&respType, 0);
	   	pData += templen;
	   	Len   += templen;
#endif /* IWSC_TEST_SUPPORT */		

	}
#endif /* IWSC_SUPPORT */

#ifdef WSC_V2_SUPPORT
	if (pWpsCtrl->WscV2Info.bEnableWpsV2)
	{
		PWSC_TLV pWscTLV = &pWpsCtrl->WscV2Info.ExtraTlv;
		WscGenV2Msg(pWpsCtrl, 
					b_selRegistrar, 
					pAuthorizedMACs, 
					AuthorizedMACsLen, 
					&pData, 
					&Len);

		/* Extra attribute that is not defined in WSC Sepc. */
		if (pWscTLV->pTlvData && pWscTLV->TlvLen)
		{
			templen = AppendWSCTLV(pWscTLV->TlvTag, pData, (UINT8 *)pWscTLV->pTlvData, pWscTLV->TlvLen);
			pData += templen;
			Len   += templen;
		}
	}
#endif /* WSC_V2_SUPPORT */

#ifdef EASY_CONFIG_SETUP
	/*
		Only main bssid supports auto provision.
	*/
	
#ifdef CONFIG_AP_SUPPORT
	if ((pAd->ApCfg.MBSSID[MAIN_MBSSID].EasyConfigInfo.bEnable == TRUE) &&
		(apidx == MAIN_MBSSID))
#endif /* CONFIG_AP_SUPPORT */
	{
		/*
			WPS Vendor Extension Value: Ralink OUI + MIC
		*/
		NdisMoveMemory(VendorExt, RALINK_OUI, 3);
#ifdef CONFIG_AP_SUPPORT
		if (CurOpMode == AP_MODE)
			WpsCalculateMIC(pAd, &Data[0], Len, &pAd->ApCfg.MBSSID[apidx].EasyConfigInfo, pAd->ApCfg.MBSSID[apidx].Bssid, MIC);
#endif /* CONFIG_AP_SUPPORT */
#ifdef STA_EASY_CONFIG_SETUP
#ifdef CONFIG_STA_SUPPORT
		if (CurOpMode == STA_MODE)
			WpsCalculateMIC(pAd, &Data[0], Len, &pAd->StaCfg.EasyConfigInfo, pAd->CurrentAddress, MIC);
#endif /* CONFIG_STA_SUPPORT */
#endif /* STA_EASY_CONFIG_SETUP */
		NdisMoveMemory(VendorExt + 3, MIC, LEN_KEY_DESC_MIC);
		/* 
			WPS Vendor Extension	
		*/
		templen = AppendWSCTLV(WSC_ID_VENDOR_EXT, pData, (UINT8 *)VendorExt, 3 + LEN_KEY_DESC_MIC);
		pData += templen;
		Len   += templen;
	}
#endif /* EASY_CONFIG_SETUP */

#ifdef P2P_SUPPORT
	// 12. Primary Device Type
	templen = AppendWSCTLV(WSC_ID_PRIM_DEV_TYPE, pData, pReg->SelfInfo.PriDeviceType, 0);
	pData += templen;
	Len   += templen;

	// 13. Device Name
	NdisZeroMemory(pData, 32 + 4);
	templen = AppendWSCTLV(WSC_ID_DEVICE_NAME, pData, &pAd->P2pCfg.DeviceName, pAd->P2pCfg.DeviceNameLen);
	pData += templen;
	Len   += templen;
#endif /* P2P_SUPPORT */

	ieHdr.length = ieHdr.length + Len;

#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
	{
		memcpy(pAd->ApCfg.MBSSID[apidx].WscIEBeacon.Value, &ieHdr, sizeof(WSC_IE_HEADER));
		memcpy(pAd->ApCfg.MBSSID[apidx].WscIEBeacon.Value + sizeof(WSC_IE_HEADER), Data, Len);
		pAd->ApCfg.MBSSID[apidx].WscIEBeacon.ValueLen = sizeof(WSC_IE_HEADER) + Len;
	}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
	{
		memcpy(pAd->StaCfg.WpsIEBeacon.Value, &ieHdr, sizeof(WSC_IE_HEADER));
		memcpy(pAd->StaCfg.WpsIEBeacon.Value + sizeof(WSC_IE_HEADER), Data, Len);
		pAd->StaCfg.WpsIEBeacon.ValueLen = sizeof(WSC_IE_HEADER) + Len;
	}
#endif /* CONFIG_STA_SUPPORT */

	if (Data != NULL)
		os_free_mem(NULL, Data);

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscBuildBeaconIE\n"));
}

VOID WscBuildProbeRespIE(
	IN	PRTMP_ADAPTER	pAd, 
	IN	UCHAR respType,
	IN	UCHAR scState,
	IN	BOOLEAN b_selRegistrar,
	IN	USHORT devPwdId,
	IN	USHORT selRegCfgMethods,
	IN  UCHAR apidx,
	IN  UCHAR *pAuthorizedMACs,
	IN  INT   AuthorizedMACsLen,
	IN  UCHAR	CurOpMode)
{
	WSC_IE_HEADER 	ieHdr;
/*	UCHAR 			Data[512]; */
	UCHAR			*Data = NULL;
	PUCHAR			pData;
	INT				Len = 0, templen = 0;
	USHORT			tempVal = 0;
	PWSC_CTRL		pWpsCtrl = NULL;
    PWSC_REG_DATA	pReg = NULL;
#ifdef EASY_CONFIG_SETUP
	UCHAR			VendorExt[32] = {0}, MIC[16] = {0};
#endif /* EASY_CONFIG_SETUP */


	/* allocate memory */
	os_alloc_mem(NULL, (UCHAR **)&Data, 512);
	if (Data == NULL)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: Allocate memory fail!!!\n", __FUNCTION__));
		return;
	}

#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
		pWpsCtrl = &pAd->ApCfg.MBSSID[apidx & 0x0F].WscControl;
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
		pWpsCtrl = &pAd->StaCfg.WscControl;
#endif /* CONFIG_STA_SUPPORT */

	pReg = &pWpsCtrl->RegData;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscBuildProbeRespIE, apidx = %x\n", apidx));

	/* WSC IE Header */
	ieHdr.elemId = 221;
	ieHdr.length = 4;
	ieHdr.oui[0] = 0x00; ieHdr.oui[1] = 0x50; ieHdr.oui[2] = 0xF2; 
#ifdef IWSC_SUPPORT
	if ((CurOpMode == STA_MODE) &&
		(pAd->StaCfg.BssType == BSS_ADHOC))
		ieHdr.oui[3] = 0x10;
	else
#endif /* IWSC_SUPPORT */
	ieHdr.oui[3] = 0x04;

	pData = (PUCHAR) &Data[0];
	Len = 0;
	
	/* 1. Version */
	templen = AppendWSCTLV(WSC_ID_VERSION, pData, &pReg->SelfInfo.Version, 0);
	pData += templen;
	Len   += templen;

	/* 2. Simple Config State */
	templen = AppendWSCTLV(WSC_ID_SC_STATE, pData, (UINT8 *)&scState, 0);
	pData += templen;
	Len   += templen;

#ifdef CONFIG_AP_SUPPORT
#ifdef WSC_V2_SUPPORT
	if ((CurOpMode == AP_MODE) && pWpsCtrl->bSetupLock)
	{
		// AP Setup Lock
		templen = AppendWSCTLV(WSC_ID_AP_SETUP_LOCKED, pData, (UINT8 *)&pWpsCtrl->bSetupLock, 0);
		pData += templen;
		Len   += templen;
	}
#endif /* WSC_V2_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

	if ( b_selRegistrar )
	{
		/* 3. Selected Registrar */
		templen = AppendWSCTLV(WSC_ID_SEL_REGISTRAR, pData, (UINT8 *)&b_selRegistrar, 0);
    	pData += templen;
    	Len   += templen;

		/* 4. Device Password ID */
		tempVal = htons(devPwdId);
		templen = AppendWSCTLV(WSC_ID_DEVICE_PWD_ID, pData, (UINT8 *)&tempVal, 0);
    	pData += templen;
    	Len   += templen;

		/* 5. Selected Registrar Config Methods */
#ifdef IWSC_SUPPORT
		if ((CurOpMode == STA_MODE) && 
			(pAd->StaCfg.IWscInfo.bSelRegStart == FALSE))
		{
			if (pWpsCtrl->WscMode == WSC_PIN_MODE)
			{
				selRegCfgMethods &= 0x200F;
				if (pAd->StaCfg.IWscInfo.bLimitedUI)
				{
					selRegCfgMethods &= (~WSC_CONFMET_KEYPAD);
				}
				else
				{
					selRegCfgMethods |= WSC_CONFMET_KEYPAD;
				}
			}
			else
			{
				selRegCfgMethods &= 0x02F0;
			}
		}
#endif /* IWSC_SUPPORT */
		tempVal = htons(selRegCfgMethods);
		templen = AppendWSCTLV(WSC_ID_SEL_REG_CFG_METHODS, pData, (UINT8 *)&tempVal, 0);
    	pData += templen;
    	Len   += templen;

#ifdef IWSC_SUPPORT
		if ((CurOpMode == STA_MODE) && pAd->StaCfg.IWscInfo.bSelRegStart)
		{
			templen = AppendWSCTLV(WSC_ID_MAC_ADDR, pData, pAd->StaCfg.IWscInfo.RegMacAddr, 0);
			pData += templen;
			Len   += templen;
			pAd->StaCfg.IWscInfo.bSelRegStart = FALSE;
		}
#endif /* IWSC_SUPPORT */
	}

	/* 6. Response Type WSC_ID_RESP_TYPE */
	templen = AppendWSCTLV(WSC_ID_RESP_TYPE, pData, (UINT8 *)&respType, 0);
   	pData += templen;
   	Len   += templen;

	/* 7. UUID last 6 bytes use MAC */
	templen = AppendWSCTLV(WSC_ID_UUID_E, pData, &pWpsCtrl->Wsc_Uuid_E[0], 0);
	pData += templen;
	Len   += templen;

	/* 8. Manufacturer */
	NdisZeroMemory(pData, 64 + 4);
    templen = AppendWSCTLV(WSC_ID_MANUFACTURER, pData,  pReg->SelfInfo.Manufacturer, strlen((RTMP_STRING *) pReg->SelfInfo.Manufacturer));
	pData += templen;
	Len   += templen;

	/* 9. Model Name */
	NdisZeroMemory(pData, 32 + 4);
    templen = AppendWSCTLV(WSC_ID_MODEL_NAME, pData, pReg->SelfInfo.ModelName, strlen((RTMP_STRING *) pReg->SelfInfo.ModelName));
	pData += templen;
	Len   += templen;

	/* 10. Model Number */
	NdisZeroMemory(pData, 32 + 4);
    templen = AppendWSCTLV(WSC_ID_MODEL_NUMBER, pData, pReg->SelfInfo.ModelNumber, strlen((RTMP_STRING *) pReg->SelfInfo.ModelNumber));
	pData += templen;
	Len   += templen;

	/* 11. Serial Number */
	NdisZeroMemory(pData, 32 + 4);
    templen = AppendWSCTLV(WSC_ID_SERIAL_NUM, pData, pReg->SelfInfo.SerialNumber, strlen((RTMP_STRING *) pReg->SelfInfo.SerialNumber));
	pData += templen;
	Len   += templen;

	/* 12. Primary Device Type */
	templen = AppendWSCTLV(WSC_ID_PRIM_DEV_TYPE, pData, pReg->SelfInfo.PriDeviceType, 0);
	pData += templen;
	Len   += templen;

	/* 13. Device Name */
	NdisZeroMemory(pData, 32 + 4);
    templen = AppendWSCTLV(WSC_ID_DEVICE_NAME, pData, pReg->SelfInfo.DeviceName, strlen((RTMP_STRING *) pReg->SelfInfo.DeviceName));
	pData += templen;
	Len   += templen;

	/* 14. Config Methods */
	/*tempVal = htons(0x008a); */
	/*tempVal = htons(0x0084); */
#ifdef P2P_SUPPORT
	/*
		Some P2P Device will check this config method for PBC. (ex. Samsung GALAXYSII)
		If this config method doesn't include PBC, some P2P Device doesn't send provision request if we are P2P GO.
	*/
	if (apidx >= MIN_NET_DEVICE_FOR_P2P_GO)
	{
		if (pAd->P2pCfg.bSigmaEnabled == TRUE)
			/* P2P test plan 1.0.3 item 4.2.2.  
			WSC IE does not have Push Button flag 
			set ON in Config Method attribute. */
			tempVal = pWpsCtrl->WscConfigMethods & 0xff7f;
		else
		tempVal = pWpsCtrl->WscConfigMethods;
	}
	else
#endif /* P2P_SUPPORT */
	{
		/*
			WSC 1.0 WCN logo testing will check the value of config method in probe response and M1.
			Config method shall be identical in probe response and M1.
		*/
#ifdef WSC_V2_SUPPORT
		if (pWpsCtrl->WscV2Info.bEnableWpsV2)
	tempVal = pWpsCtrl->WscConfigMethods & 0xF97F;
		else
#endif /* WSC_V2_SUPPORT */
			tempVal = pWpsCtrl->WscConfigMethods & 0x00FF;
	}

	tempVal = htons(tempVal);
	templen = AppendWSCTLV(WSC_ID_CONFIG_METHODS, pData, (UINT8 *)&tempVal, 0);
	pData += templen;
	Len   += templen;


	/* 15. RF Bands */
	if (CurOpMode == AP_MODE)
	{
		if (WMODE_CAP_5G(pAd->CommonCfg.PhyMode))
			tempVal = 2;
		else
			tempVal = 1;
	}
	else
	{
		if (pAd->CommonCfg.Channel > 14)
			tempVal = 2;
		else
			tempVal = 1;
	}
#ifdef RT_BIG_ENDIAN
	tempVal =SWAP16(tempVal);
#endif /* RT_BIG_ENDIAN */
    templen = AppendWSCTLV(WSC_ID_RF_BAND, pData, (UINT8 *)&tempVal, 0);
	pData += templen;
	Len   += templen;
     
#ifdef IWSC_SUPPORT
	if ((pAd->StaCfg.BssType == BSS_ADHOC) &&
		(CurOpMode == STA_MODE))
	{
		/* Connection Type Flag ESS */
		templen = AppendWSCTLV(WSC_ID_CONN_TYPE, pData, (UINT8 *)&pReg->SelfInfo.ConnTypeFlags, 0);
		pData += templen;
		Len   += templen;

		if ((pWpsCtrl->WscMode == WSC_SMPBC_MODE) &&
			(pWpsCtrl->WscConfMode == WSC_REGISTRAR))
		{
			BOOLEAN bEntryAcceptable = FALSE;
			BOOLEAN bRegistrationReady = TRUE;
			PIWSC_INFO pIWscInfo = NULL;
			
			pIWscInfo = &pAd->StaCfg.IWscInfo;
			if (pIWscInfo->bIWscEntryTimerRunning)
				bEntryAcceptable = TRUE;

			/* Entry Acceptable (only for IBSS) */
			templen = AppendWSCTLV(WSC_ID_ENTRY_ACCEPTABLE, pData, (UINT8 *)&bEntryAcceptable, 0);
			pData += templen;
			Len   += templen;

			if (pWpsCtrl->EapMsgRunning)
				bRegistrationReady = FALSE;
			
			/* Registration Ready (only for IBSS) */
			templen = AppendWSCTLV(WSC_ID_REGISTRATON_READY, pData, (UINT8 *)&bRegistrationReady, 0);
			pData += templen;
			Len   += templen;
		}

		/* IWSC IP Address Configuration */
		tempVal = htons(pAd->StaCfg.IWscInfo.IpConfMethod);
		templen = AppendWSCTLV(WSC_ID_IP_ADDR_CONF_METHOD, pData, (UINT8 *)&tempVal, 0);
		pData += templen;
		Len   += templen;
	}
#endif /* IWSC_SUPPORT */

#ifdef WSC_V2_SUPPORT
	if (pWpsCtrl->WscV2Info.bEnableWpsV2)
	{
		PWSC_TLV pWscTLV = &pWpsCtrl->WscV2Info.ExtraTlv;
		WscGenV2Msg(pWpsCtrl, 
					b_selRegistrar, 
					pAuthorizedMACs, 
					AuthorizedMACsLen, 
					&pData, 
					&Len);

		/* Extra attribute that is not defined in WSC Sepc. */
		if (pWscTLV->pTlvData && pWscTLV->TlvLen)
		{
			templen = AppendWSCTLV(pWscTLV->TlvTag, pData, (UINT8 *)pWscTLV->pTlvData, pWscTLV->TlvLen);
			pData += templen;
			Len   += templen;
		}
	}
#endif /* WSC_V2_SUPPORT */

#ifdef EASY_CONFIG_SETUP
#ifdef CONFIG_AP_SUPPORT
	/*
		Only main bssid supports auto provision.
	*/
	if ((pAd->ApCfg.MBSSID[apidx & 0x0F].EasyConfigInfo.bEnable == TRUE) && ((apidx & 0xF) == MAIN_MBSSID))
#endif /* CONFIG_AP_SUPPORT */
	{
		/*
			WPS Vendor Extension Value: Ralink OUI + MIC
		*/
		NdisMoveMemory(VendorExt, RALINK_OUI, 3);

#ifdef CONFIG_AP_SUPPORT
		if (CurOpMode == AP_MODE)
			WpsCalculateMIC(pAd, &Data[0], Len, &pAd->ApCfg.MBSSID[apidx & 0xF].EasyConfigInfo, pAd->ApCfg.MBSSID[apidx & 0xF].Bssid, MIC);
#endif /* CONFIG_AP_SUPPORT */
#ifdef STA_EASY_CONFIG_SETUP
#ifdef CONFIG_STA_SUPPORT
		if (CurOpMode == STA_MODE)
			WpsCalculateMIC(pAd, &Data[0], Len, &pAd->StaCfg.EasyConfigInfo, pAd->CurrentAddress, MIC);
#endif /* CONFIG_STA_SUPPORT */
#endif /* STA_EASY_CONFIG_SETUP */
		
		NdisMoveMemory(VendorExt + 3, MIC, LEN_KEY_DESC_MIC);
		/* 
			WPS Vendor Extension	
		*/
		templen = AppendWSCTLV(WSC_ID_VENDOR_EXT, pData, (UINT8 *)VendorExt, 3 + LEN_KEY_DESC_MIC);
		pData += templen;
		Len   += templen;
	}
#endif /* EASY_CONFIG_SETUP */

	if (Len > 251)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Len is overflow!\n"));
	}
     
	ieHdr.length = ieHdr.length + Len;

#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
	{
		memcpy(pAd->ApCfg.MBSSID[apidx & 0xF].WscIEProbeResp.Value, &ieHdr, sizeof(WSC_IE_HEADER));
		memcpy(pAd->ApCfg.MBSSID[apidx & 0xF].WscIEProbeResp.Value + sizeof(WSC_IE_HEADER), Data, Len);
		pAd->ApCfg.MBSSID[apidx & 0xF].WscIEProbeResp.ValueLen = sizeof(WSC_IE_HEADER) + Len;
	}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
	{
		memcpy(pAd->StaCfg.WpsIEProbeResp.Value, &ieHdr, sizeof(WSC_IE_HEADER));
		memcpy(pAd->StaCfg.WpsIEProbeResp.Value + sizeof(WSC_IE_HEADER), Data, Len);
		pAd->StaCfg.WpsIEProbeResp.ValueLen = sizeof(WSC_IE_HEADER) + Len;
	}
#endif /* CONFIG_STA_SUPPORT */

	if (Data != NULL)
		os_free_mem(NULL, Data);

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscBuildProbeRespIE\n"));
}

/*
	========================================================================
	
	Routine Description:
		Ap send EAP-Fail to station

	Arguments:
		pAd    - NIC Adapter pointer
		Id			- ID between EAP-Req and EAP-Rsp pair
		pEntry		- The Station Entry information
		
	Return Value:
		None
		
	========================================================================
*/
VOID	WscSendEapFail(
	IN	PRTMP_ADAPTER		pAd,
	IN  PWSC_CTRL           pWscControl,
	IN  BOOLEAN				bSendDeAuth)
{
	UCHAR               Header802_3[14];
	USHORT				Length;
	IEEE8021X_FRAME		Ieee_8021x;
	EAP_FRAME			EapFrame;
	UCHAR				*pOutBuffer = NULL;
	ULONG				FrameLen = 0;
#ifdef CONFIG_AP_SUPPORT
	UCHAR				apidx = (pWscControl->EntryIfIdx & 0x0F);
#endif /* CONFIG_AP_SUPPORT */
	MAC_TABLE_ENTRY     *pEntry;	
	UCHAR				CurOpMode = 0xFF;
	
	NdisZeroMemory(Header802_3,sizeof(UCHAR)*14);
	
	/* 1. Send EAP-Rsp Id */
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscSendEapFail\n"));

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		CurOpMode = AP_MODE;
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		CurOpMode = STA_MODE;
#ifdef P2P_SUPPORT
	if (pWscControl->EntryIfIdx != BSS0)
		CurOpMode = AP_MODE;
#endif /* P2P_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
	{
	MAKE_802_3_HEADER(Header802_3, 
					  &pWscControl->EntryAddr[0], 
					  &pAd->ApCfg.MBSSID[apidx].wdev.bssid[0],
					  EAPOL);
	}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
	{
		MAKE_802_3_HEADER(Header802_3, 
						  &pWscControl->EntryAddr[0],
						  &pAd->CurrentAddress[0], 
						  EAPOL);
	}
#endif /* CONFIG_STA_SUPPORT */

	/* Length, -1 type size, Eap-Fail doesn't need Type item */
	Length = sizeof(EAP_FRAME) - sizeof(UCHAR);
	
	/* Zero 802.1x body */
	NdisZeroMemory(&Ieee_8021x, sizeof(Ieee_8021x));
	Ieee_8021x.Version = EAPOL_VER;
	Ieee_8021x.Type    = EAPPacket;
	Ieee_8021x.Length  = cpu2be16(Length);

	/* Zero EAP frame */
	NdisZeroMemory(&EapFrame, sizeof(EapFrame));
	EapFrame.Code   = EAP_CODE_FAIL;
	EapFrame.Id     = pWscControl->lastId;
	EapFrame.Length = cpu2be16(Length);
	
    /* Out buffer for transmitting EAP-Req(Identity) */
	os_alloc_mem(NULL, (UCHAR **)&pOutBuffer, MAX_LEN_OF_MLME_BUFFER);
    if(pOutBuffer == NULL)
        return;

	FrameLen = 0;
	
	/* Make	 Transmitting frame */
	MakeOutgoingFrame(pOutBuffer, &FrameLen,
					  sizeof(IEEE8021X_FRAME), &Ieee_8021x,
					  sizeof(EapFrame)-1, &EapFrame, END_OF_ARGS);

	pEntry = MacTableLookup(pAd, &pWscControl->EntryAddr[0]);
	/* Copy frame to Tx ring */
	RTMPToWirelessSta(pAd, pEntry, Header802_3, sizeof(Header802_3), (PUCHAR)pOutBuffer, FrameLen, TRUE);

#if 0
	/*
		EntryAddr cannot zero here, it will be zeroed in MacTableDeleteEntry.		
		NdisZeroMemory(pWscControl->EntryAddr, MAC_ADDR_LEN);
	*/
#endif

	if (pOutBuffer)
		os_free_mem(NULL, pOutBuffer);

#ifdef CONFIG_AP_SUPPORT
	if (pEntry && bSendDeAuth && (CurOpMode == AP_MODE))
	{
		MlmeDeAuthAction(pAd, pEntry, REASON_DEAUTH_STA_LEAVING, TRUE);		
	}
	if (pEntry == NULL)
	{
		/*
			If STA dis-connect un-normally, reset EntryAddr here.
		*/
		NdisZeroMemory(pWscControl->EntryAddr, MAC_ADDR_LEN);
	}
#endif /* CONFIG_AP_SUPPORT */

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscSendEapFail\n"));	
}

#ifdef CONFIG_AP_SUPPORT
VOID WscBuildAssocRespIE(
	IN	PRTMP_ADAPTER	pAd,
	IN  UCHAR 			ApIdx,
	IN  UCHAR			Reason,
	OUT	PUCHAR			pOutBuf,
	OUT	PUCHAR			pIeLen)
{
	WSC_IE_HEADER 	ieHdr;
/*	UCHAR 			Data[512] = {0}; */
	UCHAR 			*Data = NULL;
	PUCHAR			pData;
	INT				Len = 0, templen = 0;
	UINT8			tempVal = 0;
    PWSC_REG_DATA	pReg = (PWSC_REG_DATA) &pAd->ApCfg.MBSSID[ApIdx].WscControl.RegData;
#ifdef EASY_CONFIG_SETUP
	UCHAR			ExtData[4] = {0};
#endif /* EASY_CONFIG_SETUP */

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscBuildAssocRespIE\n"));

	/* allocate memory */
	os_alloc_mem(NULL, (UCHAR **)&Data, 512);
	if (Data == NULL)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: Allocate memory fail!!!\n", __FUNCTION__));
		return;
	}
	Data[0] = 0;

	/* WSC IE Header */
	ieHdr.elemId = 221;
	ieHdr.length = 4;
	ieHdr.oui[0] = 0x00; ieHdr.oui[1] = 0x50;
    ieHdr.oui[2] = 0xF2; ieHdr.oui[3] = 0x04;

	pData = (PUCHAR) &Data[0];
	Len = 0;
	
	/* Version */
	templen = AppendWSCTLV(WSC_ID_VERSION, pData, &pReg->SelfInfo.Version, 0);
	pData += templen;
	Len   += templen;

	/* Request Type */
	tempVal = WSC_MSGTYPE_AP_WLAN_MGR;
	templen = AppendWSCTLV(WSC_ID_RESP_TYPE, pData, (UINT8 *)&tempVal, 0);
	pData += templen;
	Len   += templen;

#ifdef WSC_V2_SUPPORT
	if (pAd->ApCfg.MBSSID[ApIdx].WscControl.WscV2Info.bEnableWpsV2)
	{
		WscGenV2Msg(&pAd->ApCfg.MBSSID[ApIdx].WscControl, 
					FALSE, 
					NULL, 
					0, 
					&pData, 
					&Len);
	}
#endif /* WSC_V2_SUPPORT */

#ifdef EASY_CONFIG_SETUP
	/*
		Only main bssid supports auto provision.
	*/
	if ((pAd->ApCfg.MBSSID[MAIN_MBSSID].EasyConfigInfo.bEnable == TRUE) && (ApIdx == MAIN_MBSSID))
	{
		/* WPS Vendor Extension */
		NdisMoveMemory(ExtData, RALINK_OUI, 3);
		ExtData[3] = Reason;
		templen = AppendWSCTLV(WSC_ID_VENDOR_EXT, pData, &ExtData[0], 4);
		pData += templen;
		Len   += templen;
	}
#endif /* EASY_CONFIG_SETUP */
     
	ieHdr.length = ieHdr.length + Len;
	NdisMoveMemory(pOutBuf, &ieHdr, sizeof(WSC_IE_HEADER));
	NdisMoveMemory(pOutBuf + sizeof(WSC_IE_HEADER), Data, Len);
	*pIeLen = sizeof(WSC_IE_HEADER) + Len;

	if (Data != NULL)
		os_free_mem(NULL, Data);

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscBuildAssocRespIE\n"));
}


VOID WscSelectedRegistrar(
	IN	PRTMP_ADAPTER	pAd,
	IN	PUCHAR	pReginfo,
	IN	UINT	Length,
	IN  UCHAR	apidx)
{
	PUCHAR	pData;
	//INT		IsAPConfigured;
	UCHAR   /*wsc_version,*/ wsc_sel_reg = 0;
	USHORT	wsc_dev_pass_id = 0, wsc_sel_reg_conf_mthd = 0;
	USHORT	WscType, WscLen;
	PUCHAR	pAuthorizedMACs = NULL;
	UCHAR	AuthorizedMACsLen = 0;
	PWSC_CTRL	pWscCtrl = &pAd->ApCfg.MBSSID[apidx].WscControl;

	pData = (PUCHAR)pReginfo;

	if (Length < 4)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscSelectedRegistrar --> Unknown IE \n"));
		return;
	}
	
	hex_dump("WscSelectedRegistrar - Reginfo", pReginfo, Length);
	while (Length > 4)
	{
	    /* arm-cpu has packet alignment issue, it's better to use memcpy to retrieve data */
		NdisMoveMemory(&WscType, pData, 2);
		NdisMoveMemory(&WscLen,  pData + 2, 2);
		WscLen = ntohs(WscLen);
		pData  += 4;
		Length -= 4;
		switch (ntohs(WscType))
		{
			case WSC_ID_VERSION:
				//wsc_version = *pData;
				break;

			case WSC_ID_SEL_REGISTRAR:
				wsc_sel_reg = *pData;
				break;

			case WSC_ID_DEVICE_PWD_ID:
				NdisMoveMemory(&wsc_dev_pass_id, pData, sizeof(USHORT));
				wsc_dev_pass_id = be2cpu16(wsc_dev_pass_id);
				break;

			case WSC_ID_SEL_REG_CFG_METHODS:
				NdisMoveMemory(&wsc_sel_reg_conf_mthd, pData, sizeof(USHORT));
				wsc_sel_reg_conf_mthd = be2cpu16(wsc_sel_reg_conf_mthd);
				break;

			case WSC_ID_VENDOR_EXT:
#ifdef WSC_V2_SUPPORT
				if (pWscCtrl->WscV2Info.bEnableWpsV2 && (WscLen > 0))
				{
					/*
						Find WFA_EXT_ID_AUTHORIZEDMACS
					*/
					os_alloc_mem(NULL, &pAuthorizedMACs, WscLen);
					if (pAuthorizedMACs)
					{
						NdisZeroMemory(pAuthorizedMACs, WscLen);
						WscParseV2SubItem(WFA_EXT_ID_AUTHORIZEDMACS, pData, WscLen, pAuthorizedMACs, &AuthorizedMACsLen);
					}
				}
#endif /* WSC_V2_SUPPORT */
				break;
				
			default:
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscSelectedRegistrar --> Unknown IE 0x%04x\n", WscType));
				break;
		}

		/* Offset to net WSC Ie */
		pData  += WscLen;
		Length -= WscLen;
	}

	//IsAPConfigured = pWscCtrl->WscConfStatus;

	if (wsc_sel_reg == 0x01)
	{
		pWscCtrl->WscSelReg = 1;
		WscBuildBeaconIE(pAd, WSC_SCSTATE_CONFIGURED, TRUE, wsc_dev_pass_id, wsc_sel_reg_conf_mthd, apidx, pAuthorizedMACs, AuthorizedMACsLen, AP_MODE);
		WscBuildProbeRespIE(pAd, WSC_MSGTYPE_AP_WLAN_MGR, WSC_SCSTATE_CONFIGURED, TRUE, wsc_dev_pass_id, wsc_sel_reg_conf_mthd, pWscCtrl->EntryIfIdx, pAuthorizedMACs, AuthorizedMACsLen, AP_MODE);		
#ifdef WSC_V2_SUPPORT
		hex_dump("WscSelectedRegistrar - AuthorizedMACs::", pAuthorizedMACs, AuthorizedMACsLen);
		if ((AuthorizedMACsLen == 6) && 
			(NdisEqualMemory(pAuthorizedMACs, BROADCAST_ADDR, MAC_ADDR_LEN) == FALSE) &&
			(NdisEqualMemory(pAuthorizedMACs, ZERO_MAC_ADDR, MAC_ADDR_LEN) == FALSE) &&
			(pWscCtrl->WscState <= WSC_STATE_WAIT_M3))
		{
			PWSC_PEER_ENTRY	pWscPeer = NULL;
			NdisMoveMemory(pWscCtrl->EntryAddr, pAuthorizedMACs, MAC_ADDR_LEN);
			RTMP_SEM_LOCK(&pWscCtrl->WscPeerListSemLock);
			WscClearPeerList(&pWscCtrl->WscPeerList);
			os_alloc_mem(pAd, (UCHAR **)&pWscPeer, sizeof(WSC_PEER_ENTRY));
			if (pWscPeer)
			{
				NdisZeroMemory(pWscPeer, sizeof(WSC_PEER_ENTRY));
				NdisMoveMemory(pWscPeer->mac_addr, pAuthorizedMACs, MAC_ADDR_LEN);
				NdisGetSystemUpTime(&pWscPeer->receive_time);
				insertTailList(&pWscCtrl->WscPeerList, 
								(RT_LIST_ENTRY *)pWscPeer);
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscSelectedRegistrar --> Add this MAC to WscPeerList\n"));
			}
			ASSERT(pWscPeer != NULL);
			RTMP_SEM_UNLOCK(&pWscCtrl->WscPeerListSemLock);
		}
#endif /* WSC_V2_SUPPORT */
	}
	else
	{
		pWscCtrl->WscSelReg = 0;
		WscBuildBeaconIE(pAd, WSC_SCSTATE_CONFIGURED, FALSE, 0, 0, apidx, NULL, 0, AP_MODE);
		WscBuildProbeRespIE(pAd, WSC_MSGTYPE_AP_WLAN_MGR, WSC_SCSTATE_CONFIGURED, FALSE, 0, 0, pWscCtrl->EntryIfIdx, NULL, 0, AP_MODE);
	}
	APUpdateBeaconFrame(pAd, apidx);

#ifdef WSC_V2_SUPPORT
	if (pAuthorizedMACs)
		os_free_mem(NULL, pAuthorizedMACs);
#endif /* WSC_V2_SUPPORT */
}
#endif /* CONFIG_AP_SUPPORT */

#if defined(CONFIG_STA_SUPPORT) || defined(APCLI_SUPPORT)
/*
	========================================================================
	
	Routine Description:
		Make WSC IE for the ProbeReq frame

	Arguments:
		pAd    - NIC Adapter pointer
		pOutBuf		- all of WSC IE field 
		pIeLen		- length
		
	Return Value:
		None

	IRQL = DISPATCH_LEVEL
	
	Note:
		None
		
	========================================================================
*/
VOID WscBuildProbeReqIE(
	IN	PRTMP_ADAPTER	pAd,
	IN  UCHAR			CurOpMode,
	OUT	PUCHAR			pOutBuf,
	OUT	PUCHAR			pIeLen)
{
/*	UCHAR			WscIEFixed[] = {0xdd, 0x0e, 0x00, 0x50, 0xf2, 0x04};	// length will modify later */
	WSC_IE_HEADER 	ieHdr;
/*	UCHAR			OutMsgBuf[512];		// buffer to create message contents */
	UCHAR			*OutMsgBuf = NULL;		/* buffer to create message contents */
	INT				Len =0, templen = 0;
	PUCHAR			pData;
	USHORT          tempVal = 0;
#ifdef APCLI_SUPPORT
	PWSC_REG_DATA	pReg = (PWSC_REG_DATA)&pAd->ApCfg.ApCliTab[0].WscControl.RegData;
#else	
	PWSC_REG_DATA	pReg = (PWSC_REG_DATA) &pAd->StaCfg.WscControl.RegData;
#endif /*APCLI_SUPPORT*/
#ifdef STA_EASY_CONFIG_SETUP
	UCHAR			VendorExt[32] = {0}, MIC[16] = {0};
#endif /* STA_EASY_CONFIG_SETUP */

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("-----> WscBuildProbeReqIE\n"));

	/* allocate memory */
	os_alloc_mem(NULL, (UCHAR **)&OutMsgBuf, 512);
	if (OutMsgBuf == NULL)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: Allocate memory fail!!!\n", __FUNCTION__));
		return;
	}

	/* WSC IE Header */
	ieHdr.elemId = 221;
	ieHdr.length = 4;
	ieHdr.oui[0] = 0x00; ieHdr.oui[1] = 0x50; ieHdr.oui[2] = 0xF2; 
#ifdef IWSC_SUPPORT
	if ((CurOpMode == STA_MODE) && (pAd->StaCfg.BssType == BSS_ADHOC))
		ieHdr.oui[3] = 0x10;
	else
#endif // IWSC_SUPPORT //	
	ieHdr.oui[3] = 0x04;

	pData = (PUCHAR) &OutMsgBuf[0];
	Len = 0;
				
	/* 1. Version */
	templen = AppendWSCTLV(WSC_ID_VERSION, pData, &pReg->SelfInfo.Version, 0);
	pData += templen;
	Len   += templen;

	/* 2. Request Type */
#ifdef APCLI_SUPPORT	
	if (pAd->ApCfg.ApCliTab[0].WscControl.WscConfMode == WSC_REGISTRAR)
		tempVal = WSC_MSGTYPE_REGISTRAR;
	else if (pAd->ApCfg.ApCliTab[0].WscControl.WscConfMode == WSC_ENROLLEE)
		tempVal = WSC_MSGTYPE_ENROLLEE_OPEN_8021X;
	else
		tempVal = WSC_MSGTYPE_ENROLLEE_INFO_ONLY;
#else	
	if (pAd->StaCfg.WscControl.WscConfMode == WSC_REGISTRAR)
		tempVal = WSC_MSGTYPE_REGISTRAR;
	else if (pAd->StaCfg.WscControl.WscConfMode == WSC_ENROLLEE)
		tempVal = WSC_MSGTYPE_ENROLLEE_OPEN_8021X;
	else
		tempVal = WSC_MSGTYPE_ENROLLEE_INFO_ONLY;
#endif /*APCLI_SUPPORT*/	
    templen = AppendWSCTLV(WSC_ID_REQ_TYPE, pData, (UINT8 *)&tempVal, 0);
	pData += templen;
	Len   += templen;

	/* 3. Config method */
#ifdef APCLI_SUPPORT	
#ifdef WSC_V2_SUPPORT
	if (pAd->ApCfg.ApCliTab[0].WscControl.WscV2Info.bEnableWpsV2)
	{
#if 0
		if (pAd->ApCfg.ApCliTab[0].WscControl.bWscTrigger)
		{
			if (pAd->ApCfg.ApCliTab[0].WscControl.WscMode == WSC_PIN_MODE)
			{
				tempVal = (pAd->ApCfg.ApCliTab[0].WscControl.WscConfigMethods & 0x200F);
			}
			else
			{
				tempVal = (pAd->ApCfg.ApCliTab[0].WscControl.WscConfigMethods & 0x02F0);
			}
		}
		else
#endif            
			tempVal = pAd->ApCfg.ApCliTab[0].WscControl.WscConfigMethods;
	}
	else
#endif /* WSC_V2_SUPPORT */
	{
		tempVal = (pAd->ApCfg.ApCliTab[0].WscControl.WscConfigMethods & 0x00FF);
	}
#else		
#ifdef WSC_V2_SUPPORT
	if (pAd->StaCfg.WscControl.WscV2Info.bEnableWpsV2)
	{
			tempVal = pAd->StaCfg.WscControl.WscConfigMethods;
	}
	else
#endif /* WSC_V2_SUPPORT */
	{
		tempVal = (pAd->StaCfg.WscControl.WscConfigMethods & 0x00FF);
	}
#endif /* APCLI_SUPPORT */
#ifdef IWSC_SUPPORT
	if ((CurOpMode == STA_MODE) && 
		(pAd->StaCfg.BssType == BSS_ADHOC))
	{
		if (pAd->StaCfg.IWscInfo.bLimitedUI)
		{
			tempVal &= (~WSC_CONFMET_KEYPAD);
		}
		else
		{
			tempVal |= WSC_CONFMET_KEYPAD;
		}
	}
#endif /* IWSC_SUPPORT */
	
	tempVal = cpu2be16(tempVal);
	templen = AppendWSCTLV(WSC_ID_CONFIG_METHODS, pData, (UINT8 *)&tempVal, 0);
	pData += templen;
	Len   += templen;

	/* 4. UUID */
	templen = AppendWSCTLV(WSC_ID_UUID_E, pData, pReg->SelfInfo.Uuid, 0);
	pData += templen;
	Len   += templen;

	/* 5. Primary device type */
	templen = AppendWSCTLV(WSC_ID_PRIM_DEV_TYPE, pData, pReg->SelfInfo.PriDeviceType, 0);
	pData += templen;
	Len   += templen;

	/* 6. RF band, shall change based on current channel */
    templen = AppendWSCTLV(WSC_ID_RF_BAND, pData, &pReg->SelfInfo.RfBand, 0);
	pData += templen;
	Len   += templen;

	/* 7. Associate state */
	tempVal = pReg->SelfInfo.AssocState;
	templen = AppendWSCTLV(WSC_ID_ASSOC_STATE, pData, (UINT8 *)&tempVal, 0);
	pData += templen;
	Len   += templen;
				
	/* 8. Config error */
	tempVal = pReg->SelfInfo.ConfigError;
	templen = AppendWSCTLV(WSC_ID_CONFIG_ERROR, pData, (UINT8 *)&tempVal, 0);
	pData += templen;
	Len   += templen;

	/* 9. Device password ID */
	tempVal = pReg->SelfInfo.DevPwdId;
	templen = AppendWSCTLV(WSC_ID_DEVICE_PWD_ID, pData, (UINT8 *)&tempVal, 0);
	pData += templen;
	Len   += templen;

#ifdef IWSC_SUPPORT
	if ((pAd->StaCfg.BssType == BSS_ADHOC) &&
		(CurOpMode == STA_MODE))
	{
		/* Connection Type Flag ESS */
		templen = AppendWSCTLV(WSC_ID_CONN_TYPE, pData, (UINT8 *)&pReg->SelfInfo.ConnTypeFlags, 0);
		pData += templen;
		Len   += templen;

		/* Connection Type Flag ESS */
		tempVal = htons(pAd->StaCfg.IWscInfo.IpMethod);
		templen = AppendWSCTLV(WSC_ID_IP_ADDR_CONF_METHOD, pData, (UINT8 *)&tempVal, 0);
		pData += templen;
		Len   += templen;
	}
#endif /* IWSC_SUPPORT */

#ifdef WSC_V2_SUPPORT
#ifdef APCLI_SUPPORT	
	if (pAd->ApCfg.ApCliTab[0].WscControl.WscV2Info.bEnableWpsV2)
#else
	if (pAd->StaCfg.WscControl.WscV2Info.bEnableWpsV2)
#endif /*APCLI_SUPPORT*/		
	{
		/* 10. Manufacturer */
		NdisZeroMemory(pData, 64 + 4);
		templen = AppendWSCTLV(WSC_ID_MANUFACTURER, pData,  pReg->SelfInfo.Manufacturer, strlen((RTMP_STRING *) pReg->SelfInfo.Manufacturer));
		pData += templen;
		Len   += templen;

		/* 11. Model Name */
		NdisZeroMemory(pData, 32 + 4);
		templen = AppendWSCTLV(WSC_ID_MODEL_NAME, pData, pReg->SelfInfo.ModelName, strlen((RTMP_STRING *) pReg->SelfInfo.ModelName));
		pData += templen;
		Len   += templen;

		/* 12. Model Number */
		NdisZeroMemory(pData, 32 + 4);
		templen = AppendWSCTLV(WSC_ID_MODEL_NUMBER, pData, pReg->SelfInfo.ModelNumber, strlen((RTMP_STRING *) pReg->SelfInfo.ModelNumber));
		pData += templen;
		Len   += templen;

		/* 13. Device Name */
		NdisZeroMemory(pData, 32 + 4);
		templen = AppendWSCTLV(WSC_ID_DEVICE_NAME, pData, pReg->SelfInfo.DeviceName, strlen((RTMP_STRING *) pReg->SelfInfo.DeviceName));
		pData += templen;
		Len   += templen;

		/* Version2 */
#ifdef APCLI_SUPPORT		
		WscGenV2Msg(&pAd->ApCfg.ApCliTab[0].WscControl,
					FALSE, 
					NULL, 
					0, 
					&pData, 
					&Len);
#else		
		WscGenV2Msg(&pAd->StaCfg.WscControl, 
					FALSE, 
					NULL, 
					0, 
					&pData, 
					&Len);
#endif/*APCLI_SUPPORT*/
	}
#endif /* WSC_V2_SUPPORT */

#ifdef STA_EASY_CONFIG_SETUP
	if (pAd->StaCfg.EasyConfigInfo.bRaAutoWpsAp &&
		!pAd->StaCfg.WscControl.bWscTrigger)
	{
		/*
			WPS Vendor Extension Value: Ralink OUI + MIC
		*/
		NdisMoveMemory(VendorExt, RALINK_OUI, 3);
		WpsCalculateMIC(pAd, &OutMsgBuf[0], Len, &pAd->StaCfg.EasyConfigInfo, pAd->CurrentAddress, MIC);
		NdisMoveMemory(VendorExt + 3, MIC, LEN_KEY_DESC_MIC);
		/* 
			WPS Vendor Extension	
		*/
		templen = AppendWSCTLV(WSC_ID_VENDOR_EXT, pData, (UINT8 *)VendorExt, 3 + LEN_KEY_DESC_MIC);
		pData += templen;
		Len   += templen;
	}
#endif /* STA_EASY_CONFIG_SETUP */

	ieHdr.length = ieHdr.length + Len;
	RTMPMoveMemory(pOutBuf, &ieHdr, sizeof(WSC_IE_HEADER));
	RTMPMoveMemory(pOutBuf + sizeof(WSC_IE_HEADER), OutMsgBuf, Len);
	*pIeLen = sizeof(WSC_IE_HEADER) + Len;

	if (OutMsgBuf != NULL)
		os_free_mem(NULL, OutMsgBuf);

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("<----- WscBuildProbeReqIE\n"));
}
/*
	========================================================================
	
	Routine Description:
		Make WSC IE for the AssocReq frame

	Arguments:
		pAd    - NIC Adapter pointer
		pOutBuf		- all of WSC IE field 
		pIeLen		- length
		
	Return Value:
		None

	IRQL = DISPATCH_LEVEL
	
	Note:
		None
		
	========================================================================
*/
VOID WscBuildAssocReqIE(
	/*IN	PRTMP_ADAPTER	pAd,*/
	IN  PWSC_CTRL	pWscControl,
	OUT	PUCHAR		pOutBuf,
	OUT	PUCHAR		pIeLen)
{
	WSC_IE_HEADER 	ieHdr;
/*	UCHAR 			Data[512]; */
	UCHAR			*Data = NULL;
	PUCHAR			pData;
	INT				Len = 0, templen = 0;
	UINT8			tempVal = 0;
	PWSC_REG_DATA	pReg = (PWSC_REG_DATA) &pWscControl->RegData;
#ifdef STA_EASY_CONFIG_SETUP
	UCHAR			VendorExt[32] = {0}, MIC[16] = {0};
	PRTMP_ADAPTER	pAd = NULL;
#endif /* STA_EASY_CONFIG_SETUP */

	if (pWscControl == NULL)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("WscBuildAssocReqIE: pWscControl is NULL\n"));
		return;
	}
	
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscBuildAssocReqIE\n"));

	/* allocate memory */
	os_alloc_mem(NULL, (UCHAR **)&Data, 512);
	if (Data == NULL)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: Allocate memory fail!!!\n", __FUNCTION__));
		return;
	}

	/* WSC IE Header */
	ieHdr.elemId = 221;
	ieHdr.length = 4;
	ieHdr.oui[0] = 0x00; ieHdr.oui[1] = 0x50;
    ieHdr.oui[2] = 0xF2; ieHdr.oui[3] = 0x04;

	pData = (PUCHAR) &Data[0];
	Len = 0;
	
	/* Version */
	templen = AppendWSCTLV(WSC_ID_VERSION, pData, &pReg->SelfInfo.Version, 0);
	pData += templen;
	Len   += templen;

	/* Request Type */
	if (pWscControl->WscConfMode == WSC_ENROLLEE)
		tempVal = WSC_MSGTYPE_ENROLLEE_INFO_ONLY;
	else
		tempVal = WSC_MSGTYPE_REGISTRAR;
	templen = AppendWSCTLV(WSC_ID_REQ_TYPE, pData, (UINT8 *)&tempVal, 0);
	pData += templen;
	Len   += templen;

#ifdef WSC_V2_SUPPORT
	if (pWscControl->WscV2Info.bEnableWpsV2)
	{
		/* Version2 */
		WscGenV2Msg(pWscControl, 
					FALSE, 
					NULL, 
					0, 
					&pData, 
					&Len);
	}
#endif /* WSC_V2_SUPPORT */

#ifdef STA_EASY_CONFIG_SETUP
	pAd = (PRTMP_ADAPTER)pWscControl->pAd;
	if (pAd && 
		pAd->StaCfg.EasyConfigInfo.bRaAutoWpsAp &&
		!pAd->StaCfg.WscControl.bWscTrigger)
	{
		/*
			WPS Vendor Extension Value: Ralink OUI + MIC
		*/
		NdisMoveMemory(VendorExt, RALINK_OUI, 3);
		WpsCalculateMIC(pAd, &Data[0], Len, &pAd->StaCfg.EasyConfigInfo, pAd->CurrentAddress, MIC);
		NdisMoveMemory(VendorExt + 3, MIC, LEN_KEY_DESC_MIC);
		/* 
			WPS Vendor Extension	
		*/
		templen = AppendWSCTLV(WSC_ID_VENDOR_EXT, pData, (UINT8 *)VendorExt, 3 + LEN_KEY_DESC_MIC);
		pData += templen;
		Len   += templen;
	}
#endif /* STA_EASY_CONFIG_SETUP */
	
	ieHdr.length = ieHdr.length + Len;
	RTMPMoveMemory(pOutBuf, &ieHdr, sizeof(WSC_IE_HEADER));
	RTMPMoveMemory(pOutBuf + sizeof(WSC_IE_HEADER), Data, Len);
	*pIeLen = sizeof(WSC_IE_HEADER) + Len;

	if (Data != NULL)
		os_free_mem(NULL, Data);

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscBuildAssocReqIE\n"));
}
#endif /* defined(CONFIG_STA_SUPPORT) || defined(APCLI_SUPPORT) */
#ifdef CONFIG_STA_SUPPORT
#ifdef WIDI_SUPPORT
/*
	========================================================================
	
	Routine Description:
		Make WSC IE for the ProbeReq frame with L2SD_TA Vendor Ext

	Arguments:
		pAdapter    - NIC Adapter pointer
		pOutBuf		- all of WSC IE field 
		pIeLen		- length
		
	Return Value:
		None

	IRQL = DISPATCH_LEVEL
	
	Note:
		None
		
	========================================================================
*/
VOID WscMakeProbeReqIEWithVendorExt(
	IN	PRTMP_ADAPTER	pAd,
	IN	PUCHAR			pDeviceName,
	IN	PUCHAR			pPrimaryDeviceType,
	IN	PUCHAR			pVendExt,
	IN	USHORT			vendExtLen,
	OUT	PUCHAR			pOutBuf,
	OUT	PUCHAR			pIeLen)
{
	WSC_IE_HEADER 	ieHdr;
	UCHAR			OutMsgBuf[512];		/* buffer to create message contents */
	UCHAR			Len =0, templen = 0;
	PUCHAR			pData;
	USHORT          tempVal = 0;
	PWSC_REG_DATA	pReg = (PWSC_REG_DATA) &pAd->StaCfg.WscControl.RegData;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscMakeProbeReqIEWithVendorExt\n"));

	/* WSC IE Header */
	ieHdr.elemId = 221;
	ieHdr.length = 4;
	ieHdr.oui[0] = 0x00; ieHdr.oui[1] = 0x50;
    ieHdr.oui[2] = 0xF2; ieHdr.oui[3] = 0x04;

	pData = (PUCHAR) &OutMsgBuf[0];
	Len = 0;
				
	/* 1. Version */
	templen = AppendWSCTLV(WSC_ID_VERSION, pData, &pReg->SelfInfo.Version, 0);
	pData += templen;
	Len   += templen;

	/* 2. Request Type */
	if (pAd->StaCfg.WscControl.WscConfMode == WSC_ENROLLEE)
		tempVal = WSC_MSGTYPE_ENROLLEE_INFO_ONLY;
	else
		tempVal = WSC_MSGTYPE_REGISTRAR;
    templen = AppendWSCTLV(WSC_ID_REQ_TYPE, pData, (UINT8 *)&tempVal, 0);
	pData += templen;
	Len   += templen;

	/* 3. Config method */
	tempVal = htons(0x008c);
	templen = AppendWSCTLV(WSC_ID_CONFIG_METHODS, pData, (UINT8 *)&tempVal, 0);
	pData += templen;
	Len   += templen;

	/* 4. UUID */
	templen = AppendWSCTLV(WSC_ID_UUID_E, pData, pReg->SelfInfo.Uuid, 0);
	pData += templen;
	Len   += templen;

	/* 5. Primary device type */
	templen = AppendWSCTLV(WSC_ID_PRIM_DEV_TYPE, pData, pPrimaryDeviceType, 0);
	pData += templen;
	Len   += templen;

	/* 6. RF band, shall change based on current channel */
    templen = AppendWSCTLV(WSC_ID_RF_BAND, pData, &pReg->SelfInfo.RfBand, 0);
	pData += templen;
	Len   += templen;

	/* 7. Associate state */
	tempVal = pReg->SelfInfo.AssocState;
	templen = AppendWSCTLV(WSC_ID_ASSOC_STATE, pData, (UINT8 *)&tempVal, 0);
	pData += templen;
	Len   += templen;
				
	/* 8. Config error */
	tempVal = pReg->SelfInfo.ConfigError;
	templen = AppendWSCTLV(WSC_ID_CONFIG_ERROR, pData, (UINT8 *)&tempVal, 0);
	pData += templen;
	Len   += templen;

	/* 9. Device password ID */
	tempVal = pReg->SelfInfo.DevPwdId;
	templen = AppendWSCTLV(WSC_ID_DEVICE_PWD_ID, pData, (UINT8 *)&tempVal, 0);
	pData += templen;
	Len   += templen;

	/* 10. Device Name */
	templen = AppendWSCTLV(WSC_ID_DEVICE_NAME, pData, pDeviceName, 0);
	pData += templen;
	Len   += templen;

	/* 11. Vendor Ext Attribute for L2SD Service Info */
	templen = AppendWSCTLV(WSC_ID_VENDOR_EXT, pData, pVendExt, vendExtLen);
	pData += templen;
	Len   += templen;

	ieHdr.length = ieHdr.length + Len;
	RTMPMoveMemory(pOutBuf, &ieHdr, sizeof(WSC_IE_HEADER));
	RTMPMoveMemory(pOutBuf + sizeof(WSC_IE_HEADER), OutMsgBuf, Len);
	*pIeLen = sizeof(WSC_IE_HEADER) + Len;
	
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscMakeProbeReqIEWithVendorExt\n"));
}
#endif /* WIDI_SUPPORT */

#endif /* CONFIG_STA_SUPPORT */

VOID WscProfileRetryTimeout(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3)
{
	RTMP_ADAPTER    *pAdapter = NULL;
	PWSC_CTRL		pWscControl = (PWSC_CTRL)FunctionContext;	
	BOOLEAN			bReConnect = TRUE;
	UCHAR			CurOpMode = 0xFF;

	if (pWscControl == NULL)
		return;

	pAdapter = pWscControl->pAd;

	if (pAdapter != NULL)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscProfileRetryTimeout:: WSC profile retry timeout index: %d\n", pWscControl->WscProfile.ApplyProfileIdx));	

#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAdapter)
			CurOpMode = STA_MODE;
#ifdef P2P_SUPPORT
		if (pWscControl->EntryIfIdx != BSS0)
			CurOpMode = AP_MODE;
#endif /* P2P_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAdapter)
			CurOpMode = AP_MODE;

#ifdef APCLI_SUPPORT
		if( (CurOpMode == AP_MODE) 
			&& (pAdapter->ApCfg.ApCliTab[BSS0].CtrlCurrState == APCLI_CTRL_CONNECTED)
			&& (pAdapter->ApCfg.ApCliTab[BSS0].SsidLen != 0))
		{
			INT i;
			for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
			{
				PMAC_TABLE_ENTRY pEntry = &pAdapter->MacTab.Content[i];
				STA_TR_ENTRY *tr_entry = &pAdapter->MacTab.tr_entry[i];

				if ( IS_ENTRY_APCLI(pEntry) &&
					(pEntry->Sst == SST_ASSOC) &&
					(tr_entry->PortSecured == WPA_802_1X_PORT_SECURED))
					{
						bReConnect = FALSE;
					}
			}			
		}
#endif /* APCLI_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

		if ((CurOpMode == STA_MODE) && INFRA_ON(pAdapter) && (pAdapter->IndicateMediaState == NdisMediaStateConnected))
		{
			pWscControl->WscProfileRetryTimerRunning = FALSE;
			bReConnect = FALSE;
		}
		
		if (bReConnect)
		{
			if (pWscControl->WscProfile.ApplyProfileIdx < pWscControl->WscProfile.ProfileCnt-1)
				pWscControl->WscProfile.ApplyProfileIdx++;
			else
				pWscControl->WscProfile.ApplyProfileIdx = 0;

#ifdef APCLI_SUPPORT
			if (CurOpMode == AP_MODE)
			{
				BOOLEAN apcliEn;
				WscWriteConfToApCliCfg(pAdapter, 
									   pWscControl, 
									   &pWscControl->WscProfile.Profile[pWscControl->WscProfile.ApplyProfileIdx],
									   TRUE);

				apcliEn = pAdapter->ApCfg.ApCliTab[BSS0].Enable;

				/* bring apcli interface down first */
				if(apcliEn == TRUE )
				{
					pAdapter->ApCfg.ApCliTab[BSS0].Enable = FALSE;
					ApCliIfDown(pAdapter);
					pAdapter->ApCfg.ApCliTab[BSS0].Enable = TRUE;
				}
			}
#endif /* APCLI_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
			if (CurOpMode == STA_MODE)
			{
				WscWriteConfToPortCfg(pAdapter,
									  pWscControl,
									  &pWscControl->WscProfile.Profile[pWscControl->WscProfile.ApplyProfileIdx],
									  TRUE);
			}
#endif /* CONFIG_STA_SUPPORT */

			pAdapter->WriteWscCfgToDatFile = (pWscControl->EntryIfIdx & 0x0F);
/*#ifdef KTHREAD_SUPPORT */
/*			WAKE_UP(&(pAdapter->wscTask)); */
/*#else */
/*			RTMP_SEM_EVENT_UP(&(pAdapter->wscTask.taskSema)); */
/*#endif */
			RtmpOsTaskWakeUp(&(pAdapter->wscTask));
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscProfileRetryTimeout:: WSC profile retry index: %d\n", pWscControl->WscProfile.ApplyProfileIdx));
		}
#ifdef CONFIG_STA_SUPPORT
		pAdapter->StaCfg.bAutoConnectByBssid = FALSE;
#endif /* CONFIG_STA_SUPPORT */
	}
}

VOID WscPBCTimeOutAction(
    IN PVOID SystemSpecific1, 
    IN PVOID FunctionContext, 
    IN PVOID SystemSpecific2, 
    IN PVOID SystemSpecific3)
{
	PWSC_CTRL       pWscControl = (PWSC_CTRL)FunctionContext;
	RTMP_ADAPTER    *pAd = NULL;
    BOOLEAN         Cancelled;

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("-----> WscPBCTimeOutAction\n"));

	if (pWscControl != NULL)
		pAd = pWscControl->pAd;

	if (pAd != NULL)
	{
	
	    if (pWscControl->WscPBCTimerRunning)
	    {
	        pWscControl->WscPBCTimerRunning = FALSE;
	        RTMPCancelTimer(&pWscControl->WscPBCTimer, &Cancelled);
	    }

		WscPBCExec(pAd, FALSE, pWscControl);
		
		/* call Mlme handler to execute it */
		RTMP_MLME_HANDLER(pAd);
	}
    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("<----- WscPBCTimeOutAction\n"));
}
#ifdef APCLI_SUPPORT
VOID WscPINTimeOutAction(
    IN PVOID SystemSpecific1, 
    IN PVOID FunctionContext, 
    IN PVOID SystemSpecific2, 
    IN PVOID SystemSpecific3)
{
	PWSC_CTRL       pWscControl = (PWSC_CTRL)FunctionContext;
	RTMP_ADAPTER    *pAd = NULL;
    BOOLEAN         Cancelled;

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("-----> WscPINTimeOutAction\n"));

	if (pWscControl != NULL)
		pAd = pWscControl->pAd;

	if ((pAd != NULL) && pWscControl->Wsc2MinsTimerRunning)
	{
	
	    if (pWscControl->WscPINTimerRunning)
	    {
	        pWscControl->WscPINTimerRunning = FALSE;
	        RTMPCancelTimer(&pWscControl->WscPINTimer, &Cancelled);
	    }
		WscPINExec(pAd,pWscControl);
		
		/* call Mlme handler to execute it */
		RTMP_MLME_HANDLER(pAd);
	}
    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("<----- WscPINTimeOutAction\n"));
}
#endif /* APCLI_SUPPORT */

/*
	========================================================================
	
	Routine Description:
		Exec scan after scan timer expiration 

	Arguments:
		FunctionContext		NIC Adapter pointer
		
	Return Value:
		None
		
	IRQL = DISPATCH_LEVEL
	
	Note:
		
	========================================================================
*/
VOID WscScanTimeOutAction(
    IN PVOID SystemSpecific1,
    IN PVOID FunctionContext,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3)
{
    RTMP_ADAPTER    *pAd = NULL;
	PWSC_CTRL       pWscControl = (PWSC_CTRL)FunctionContext;

		if (pWscControl == NULL)
			return;
		
	pAd = pWscControl->pAd;

	if (pAd != NULL)
	{
		/* call to execute the scan actions */
		WscScanExec(pAd, pWscControl);

		/* register 10 second timer for PBC or PIN connection execution */
		if (pWscControl->WscMode == WSC_PBC_MODE)
		{
		    /* Prevent infinite loop if conncet time out didn't stop the repeat scan */
			if (pWscControl->WscState != WSC_STATE_OFF)
	        {      
			    RTMPSetTimer(&pWscControl->WscPBCTimer, 10000);
	            pWscControl->WscPBCTimerRunning = TRUE;
	        }
		}
		else if (pWscControl->WscMode == WSC_PIN_MODE)
		{
#ifdef APCLI_SUPPORT
			if (pWscControl->WscState != WSC_STATE_OFF)
            {
                RTMPSetTimer(&pWscControl->WscPINTimer, 10000);
	            pWscControl->WscPINTimerRunning = TRUE;
            }    
#endif /* APCLI_SUPPORT */
		}

		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("!!! WscScanTimeOutAction !!!\n"));
	    
		/* call Mlme handler to execute it */
		RTMP_MLME_HANDLER(pAd);
	}
}

BOOLEAN ValidateChecksum(
	IN UINT PIN)
{
	UINT accum = 0;

	accum += 3 * ((PIN / 10000000) % 10); 
	accum += 1 * ((PIN / 1000000) % 10); 
	accum += 3 * ((PIN / 100000) % 10); 
	accum += 1 * ((PIN / 10000) % 10); 
	accum += 3 * ((PIN / 1000) % 10); 
	accum += 1 * ((PIN / 100) % 10); 
	accum += 3 * ((PIN / 10) % 10); 
	accum += 1 * ((PIN / 1) % 10); 
	
    return (0 == (accum % 10));
} /* ValidateChecksum */

/*
	Generate 4-digit random number, ex:1234
*/
UINT WscRandomGen4digitPinCode(
							 IN  PRTMP_ADAPTER   pAd)
{
	UINT    iPin;

	iPin = RandomByte2(pAd) * 256 * 256 + RandomByte2(pAd) * 256 + RandomByte2(pAd);
	iPin = iPin % 10000;

	return iPin;
}

UINT WscRandomGeneratePinCode(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			apidx)
{
	UINT 	iPin;
	UINT	checksum;

	iPin = RandomByte(pAd) * 256 * 256 + RandomByte(pAd) * 256 + RandomByte(pAd);

	iPin = iPin % 10000000;
	
#ifdef WIDI_SUPPORT
#ifdef CONFIG_STA_SUPPORT
	if (pAd->StaCfg.bWIDI)
		iPin = ((iPin / 1000) * 1000);
#endif /* CONFIG_STA_SUPPORT */
#endif /* WIDI_SUPPORT */	

	checksum = ComputeChecksum( iPin );
	iPin = iPin*10 + checksum;

	return iPin;
}

#ifdef WIDI_SUPPORT
/*
	Generate 4-digit random number, ex:1234
	Add three zero, 1234000
	Compute ckecksum, 1234000X
	
*/
UINT WscSpecialRandomGeneratePinCode(
							 IN  PRTMP_ADAPTER   pAd)
{
	UINT    iPin;
	UINT    checksum;

	iPin = RandomByte2(pAd) * 256 * 256 + RandomByte2(pAd) * 256 + RandomByte2(pAd);

	iPin = iPin % 10000000;
	iPin = ((iPin / 1000) * 1000);
	checksum = ComputeChecksum(iPin);
	iPin = iPin * 10 + checksum;

	return iPin;
}

UINT WscRandomGenerateP2PPinCode(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			apidx)
{
	UINT 	iPin;
	UINT	checksum;

	iPin = RandomByte(pAd) * 256 * 256 + RandomByte(pAd) * 256 + RandomByte(pAd);

	iPin = iPin % 10000000;
	checksum = ComputeChecksum( iPin );
	iPin = iPin*10 + checksum;

	return iPin;
}

#endif /* WIDI_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
VOID  WscInformFromWPA(
    IN  PMAC_TABLE_ENTRY    pEntry)
{
    /* WPA_STATE_MACHINE informs this Entry is already WPA_802_1X_PORT_SECURED. */
    RTMP_ADAPTER	*pAd = (PRTMP_ADAPTER)pEntry->pAd;
    BOOLEAN         Cancelled;

    if (pEntry->func_tb_idx >= pAd->ApCfg.BssidNum)
        return;

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscInformFromWPA\n"));
    
    if (MAC_ADDR_EQUAL(pEntry->Addr, pAd->ApCfg.MBSSID[pEntry->func_tb_idx].WscControl.EntryAddr))
    {
        NdisZeroMemory(pAd->ApCfg.MBSSID[pEntry->func_tb_idx].WscControl.EntryAddr, MAC_ADDR_LEN);
        RTMPCancelTimer(&pAd->ApCfg.MBSSID[pEntry->func_tb_idx].WscControl.EapolTimer, &Cancelled);
        pAd->ApCfg.MBSSID[pEntry->func_tb_idx].WscControl.EapolTimerRunning = FALSE;
        pEntry->bWscCapable = FALSE;
        pAd->ApCfg.MBSSID[pEntry->func_tb_idx].WscControl.WscState = WSC_STATE_CONFIGURED;

        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Reset EntryIfIdx to %d\n", WSC_INIT_ENTRY_APIDX));
    }

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscInformFromWPA\n"));
}

VOID WscDelWPARetryTimer(
    IN  PRTMP_ADAPTER pAd)
{
    PMAC_TABLE_ENTRY    pEntry;
    UCHAR				apidx = MAIN_MBSSID;
    BOOLEAN             Cancelled;

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscDelWPARetryTimer\n"));
    
    pEntry = MacTableLookup(pAd, pAd->ApCfg.MBSSID[apidx].WscControl.EntryAddr);
    
    if (pEntry)
    {
        RTMPCancelTimer(&pEntry->RetryTimer, &Cancelled);
        pEntry->WpaState = AS_NOTUSE;
    }

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscDelWPARetryTimer\n"));
}
#endif /* CONFIG_AP_SUPPORT */

VOID WscStop(
	IN	PRTMP_ADAPTER	pAd,
#ifdef CONFIG_AP_SUPPORT
    IN  BOOLEAN         bFromApCli,
#endif /* CONFIG_AP_SUPPORT */
	IN  PWSC_CTRL       pWscControl)
{
	PWSC_UPNP_NODE_INFO pWscUPnPInfo;
    BOOLEAN Cancelled;
#ifdef WSC_LED_SUPPORT
	UCHAR WPSLEDStatus;
#endif /* WSC_LED_SUPPORT */

#ifdef CONFIG_AP_SUPPORT	
	MAC_TABLE_ENTRY  *pEntry;
	//UCHAR apidx = (pWscControl->EntryIfIdx & 0x0F);
#endif /* CONFIG_AP_SUPPORT */
	UCHAR	CurOpMode = 0xff;

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		CurOpMode = AP_MODE;
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		CurOpMode = STA_MODE;
#ifdef P2P_SUPPORT
	if (pWscControl->EntryIfIdx != BSS0)
		CurOpMode = AP_MODE;
#endif /* P2P_SUPPORT */
#endif /* CONFIG_STA_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
		pWscControl->bConfiguredAP = FALSE;
#endif /* CONFIG_STA_SUPPORT */

	pWscUPnPInfo = &pWscControl->WscUPnPNodeInfo;
	
	if(pWscUPnPInfo->bUPnPMsgTimerRunning == TRUE)
	{
		pWscUPnPInfo->bUPnPMsgTimerRunning = FALSE;
		RTMPCancelTimer(&pWscUPnPInfo->UPnPMsgTimer, &Cancelled);
		pWscUPnPInfo->bUPnPMsgTimerPending = FALSE;
	}
	if(pWscControl->bM2DTimerRunning)
	{
		pWscControl->bM2DTimerRunning = FALSE;
		RTMPCancelTimer(&pWscControl->M2DTimer, &Cancelled);
	}
	
    pWscUPnPInfo->bUPnPInProgress = FALSE;
    pWscControl->M2DACKBalance = 0;
	pWscUPnPInfo->registrarID = 0;

	if (pWscControl->Wsc2MinsTimerRunning)
	{
		pWscControl->Wsc2MinsTimerRunning = FALSE;
		RTMPCancelTimer(&pWscControl->Wsc2MinsTimer, &Cancelled);
	}

	if (pWscControl->WscUpdatePortCfgTimerRunning)
	{
		pWscControl->WscUpdatePortCfgTimerRunning = FALSE;
		RTMPCancelTimer(&pWscControl->WscUpdatePortCfgTimer, &Cancelled);
	}

	RTMPCancelTimer(&pWscControl->EapolTimer, &Cancelled);
	pWscControl->EapolTimerRunning = FALSE;
#ifdef CONFIG_AP_SUPPORT
	if ((pWscControl->EntryIfIdx & 0x0F)< pAd->ApCfg.BssidNum)
	{
	    pEntry = MacTableLookup(pAd, pWscControl->EntryAddr);
        
		if (CurOpMode == AP_MODE)
		{
			if (pEntry && !bFromApCli) 
			{
				pEntry->bWscCapable = FALSE;
			}
		}        
	}
#endif /* CONFIG_AP_SUPPORT */
    NdisZeroMemory(pWscControl->EntryAddr, MAC_ADDR_LEN);

	pWscControl->WscSelReg = 0;
	if ( (pWscControl->WscStatus == STATUS_WSC_CONFIGURED) ||
         (pWscControl->WscStatus == STATUS_WSC_FAIL) ||
         (pWscControl->WscStatus == STATUS_WSC_PBC_TOO_MANY_AP))
         ;
    else
		pWscControl->WscStatus = STATUS_WSC_NOTUSED;
	pWscControl->WscState = WSC_STATE_OFF;
	pWscControl->lastId = 1;
	pWscControl->EapMsgRunning = FALSE;
	pWscControl->EapolTimerPending = FALSE;
	pWscControl->bWscTrigger = FALSE;

	if (pWscControl->WscScanTimerRunning)
	{
		pWscControl->WscScanTimerRunning = FALSE;
		RTMPCancelTimer(&pWscControl->WscScanTimer, &Cancelled);
	}

	if (pWscControl->WscPBCTimerRunning)
	{
		pWscControl->WscPBCTimerRunning = FALSE;
		RTMPCancelTimer(&pWscControl->WscPBCTimer, &Cancelled);
	}
	
#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
	{		
		pAd->MlmeAux.AutoReconnectSsidLen = pAd->MlmeAux.SsidLen;
		NdisZeroMemory(&pAd->MlmeAux.AutoReconnectSsid[0], MAX_LEN_OF_SSID);
		NdisMoveMemory(&pAd->MlmeAux.AutoReconnectSsid[0], &pAd->MlmeAux.Ssid[0], pAd->MlmeAux.SsidLen);
		/* YF: Reset to default after active pbc mode */
		pWscControl->RegData.SelfInfo.DevPwdId = cpu2be16(DEV_PASS_ID_PIN);
	}
#endif /* CONFIG_STA_SUPPORT */

#ifdef WSC_LED_SUPPORT
	if (pWscControl->WscLEDTimerRunning)
	{
		pWscControl->WscLEDTimerRunning = FALSE;
		RTMPCancelTimer(&pWscControl->WscLEDTimer, &Cancelled);
	}
	if (pWscControl->WscSkipTurnOffLEDTimerRunning)
	{
		pWscControl->WscSkipTurnOffLEDTimerRunning = FALSE;
		RTMPCancelTimer(&pWscControl->WscSkipTurnOffLEDTimer, &Cancelled);
	}
	/* Reset the WPS walk time. */
	pWscControl->bWPSWalkTimeExpiration = FALSE;
	WPSLEDStatus = LED_WPS_TURN_LED_OFF;
	RTMPSetLED(pAd, WPSLEDStatus);
#endif /* WSC_LED_SUPPORT */

}

#ifdef CON_WPS
VOID WscConWpsStop(
        IN  PRTMP_ADAPTER   pAd,
    	IN  BOOLEAN         bFromApCli,
	IN  PWSC_CTRL       pWscControl)
{

	UCHAR   apidx = (pWscControl->EntryIfIdx & 0x0F);

#ifdef MULTI_INF_SUPPORT
        /* Single Driver ctrl the WSC SM between the two pAd */
        PRTMP_ADAPTER pOpposAd;
        PWSC_CTRL pWpsCtrl = NULL;
        INT IsAPConfigured = 0;
        UCHAR pAdListInfo[2] = {0, 0};
        INT myBandIdx = 0, opsBandIdx = 0;
#else
	/* Separate Driver used to MiniUpnpd to comunicate each other. */
        PWSC_UPNP_CTRL_WSC_BAND_STOP pWscUpnpBandStop;
#endif /* MULTI_INF_SUPPORT */

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscConWpsStop\n"));

	if (pAd->conWscStatus == CON_WPS_STATUS_DISABLED)
		return;

#ifdef MULTI_INF_SUPPORT
	/* Update the Global pAd List Band Info */
	pOpposAd = (PRTMP_ADAPTER)pAdGlobalList[0];
	pAdListInfo[0] = RFIC_IS_5G_BAND(pOpposAd);
	
	pOpposAd = (PRTMP_ADAPTER)pAdGlobalList[1];
	pAdListInfo[1] = RFIC_IS_5G_BAND(pOpposAd);
	
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s pAdListInfo is5G---> [%d, %d]\n",
		pAdListInfo[0], pAdListInfo[1]));

	/* which band from function in */
	if (RFIC_IS_5G_BAND(pAd) == pAdListInfo[0])
	{
		myBandIdx = 0;
		opsBandIdx = 1;
	}
	else if (RFIC_IS_5G_BAND(pAd) == pAdListInfo[1])
	{
		myBandIdx = 1;
		opsBandIdx = 0;
	}
	else
		;
		

	pOpposAd = (PRTMP_ADAPTER) pAdGlobalList[opsBandIdx];

	if (bFromApCli)
	{
		pWpsCtrl = &pOpposAd->ApCfg.ApCliTab[BSS0].WscControl;
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("CON_WPS: Stop the Band[%d] ApCli WPS, state [%d]\n",
                                                        opsBandIdx, pWpsCtrl->WscState));
        if (pWpsCtrl->WscState != WSC_STATE_OFF)
        {
        	WscStop(pOpposAd, TRUE, pWpsCtrl);
                pWpsCtrl->WscConfMode = WSC_DISABLE;
        }
	}
	else
	{
        pWpsCtrl = &pOpposAd->ApCfg.MBSSID[apidx].WscControl;
        IsAPConfigured = pWpsCtrl->WscConfStatus;

        if ((pWpsCtrl->WscConfMode != WSC_DISABLE) &&
             (pWpsCtrl->bWscTrigger == TRUE))
        {
        	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("CON_WPS[%d]: Stop the AP Wsc Machine\n", opsBandIdx));
                WscBuildBeaconIE(pOpposAd, IsAPConfigured, FALSE, 0, 0, apidx, NULL, 0, AP_MODE);
                WscBuildProbeRespIE(pOpposAd, WSC_MSGTYPE_AP_WLAN_MGR, IsAPConfigured, FALSE, 0, 0,
                                          	                          apidx, NULL, 0, AP_MODE);
                APUpdateBeaconFrame(pOpposAd, apidx);
                WscStop(pOpposAd, FALSE, pWpsCtrl);
        }
	}


#else
        os_alloc_mem(NULL, (UCHAR **)&pWscUpnpBandStop, sizeof(WSC_UPNP_CTRL_WSC_BAND_STOP));

        if (pWscUpnpBandStop != NULL)
        {
        	NdisCopyMemory(pWscUpnpBandStop->ifName, pAd->net_dev->name, IFNAMSIZ);
                pWscUpnpBandStop->is2gBand = !RFIC_IS_5G_BAND(pAd);
                pWscUpnpBandStop->isApCli = bFromApCli;

                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("CON_WPS: WSC_UPNP_CTRL_WSC_BAND_STOP[%s] is2G[%d], isApCli[%d]\n",
                				pWscUpnpBandStop->ifName,
                                                pWscUpnpBandStop->is2gBand, pWscUpnpBandStop->isApCli));

                WscSendUPnPMessage(pAd, apidx, WSC_OPCODE_UPNP_CTRL, 99, (CHAR *)pWscUpnpBandStop,
                                sizeof(WSC_UPNP_CTRL_WSC_BAND_STOP), 0, 0, NULL, AP_MODE);
                os_free_mem(NULL, pWscUpnpBandStop);
         }
#endif /* MULTI_INF_SUPPORT */
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscConWpsStop\n"));
}
#endif /* CON_WPS */

VOID WscInit(
	IN	PRTMP_ADAPTER	pAd,
    IN  BOOLEAN         bFromApCli,
	IN  UCHAR       	BssIndex)
{
	IN  PWSC_CTRL       pWscControl = NULL;
#ifdef CONFIG_STA_SUPPORT
#ifdef STA_EASY_CONFIG_SETUP
	PUCHAR		ptr;
	CIPHER_KEY	CipherKey;
	UCHAR		iv_hdr[4];
#endif /* STA_EASY_CONFIG_SETUP */
#endif /* CONFIG_STA_SUPPORT */
	UCHAR		CurOpMode = AP_MODE;

#ifdef CONFIG_AP_SUPPORT	
	INT IsAPConfigured;

	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
#ifdef APCLI_SUPPORT
		if (bFromApCli)
			pWscControl = &pAd->ApCfg.ApCliTab[BssIndex & 0x0F].WscControl;
		else
#endif /* APCLI_SUPPORT */
			pWscControl = &pAd->ApCfg.MBSSID[BssIndex & 0x0F].WscControl;
	}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	{
#ifdef P2P_SUPPORT
		if (BssIndex >= MIN_NET_DEVICE_FOR_P2P_GO)
		{
			pWscControl = &pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl;
		}
		else if (bFromApCli)
		{
			pWscControl = &pAd->ApCfg.ApCliTab[MAIN_MBSSID].WscControl;
		}
		else
#endif /* P2P_SUPPORT */
		{
			pWscControl = &pAd->StaCfg.WscControl;
			CurOpMode = STA_MODE;
		}
	}
#endif /* CONFIG_STA_SUPPORT */

	if (pWscControl == NULL)
		return;

	if (pWscControl->WscEnrolleePinCode == 0)
	{
		if (pWscControl->WscEnrollee4digitPinCode)
		{
			pWscControl->WscEnrolleePinCodeLen = 4;
			pWscControl->WscEnrolleePinCode = WscRandomGen4digitPinCode(pAd);
		}
		else
		{
			pWscControl->WscEnrolleePinCode = GenerateWpsPinCode(pAd, bFromApCli, BssIndex);
			pWscControl->WscEnrolleePinCodeLen = 8;
		}
	}
	pWscControl->RegData.SelfInfo.Version = WSC_VERSION;
#ifdef WSC_V2_SUPPORT
	pWscControl->RegData.SelfInfo.Version2 = WSC_V2_VERSION;
#endif /* WSC_V2_SUPPORT */

	pWscControl->bWscLastOne = FALSE;
	pWscControl->bWscFirstOne = FALSE;

#ifdef CONFIG_STA_SUPPORT
#ifdef STA_EASY_CONFIG_SETUP
	if (CurOpMode == STA_MODE)
	{
		NdisZeroMemory(&CipherKey, sizeof(CIPHER_KEY));
		NdisMoveMemory(&CipherKey, pAd->StaCfg.EasyConfigInfo.WpsPTK, 64);
		CipherKey.CipherAlg = CIPHER_WEP128;
		CipherKey.KeyLen = 13;
		NdisZeroMemory(pAd->StaCfg.EasyConfigInfo.WpsEncryptPIN, 12);
		ptr = &pAd->StaCfg.EasyConfigInfo.WpsEncryptPIN[0];
		snprintf(ptr, sizeof(pAd->StaCfg.EasyConfigInfo.WpsEncryptPIN), "%08u", pWscControl->WscEnrolleePinCode);

		/* Construct the 4-bytes WEP IV header */
		RTMPConstructWEPIVHdr(1, 
							  CipherKey.TxTsc,
							  iv_hdr);

		RTMPSoftEncryptWEP(pAd,
						   iv_hdr,
						   &CipherKey,
						   pAd->StaCfg.EasyConfigInfo.WpsEncryptPIN, 
						   8);
	}
#endif /* STA_EASY_CONFIG_SETUP */
#endif /* CONFIG_STA_SUPPORT */

	pWscControl->WscStatus = STATUS_WSC_IDLE;
#ifdef CONFIG_AP_SUPPORT
	if (((CurOpMode == AP_MODE) && 
		(pWscControl->WscConfMode == WSC_DISABLE))
#ifdef WSC_V2_SUPPORT
		|| ((pWscControl->WscV2Info.bWpsEnable == FALSE) && pWscControl->WscV2Info.bEnableWpsV2)
#endif /* WSC_V2_SUPPORT */
		)
	{
		if (CurOpMode == AP_MODE)
		{
#ifdef APCLI_SUPPORT
			if (!bFromApCli)
#endif /* APCLI_SUPPORT */
			{
				pAd->ApCfg.MBSSID[BssIndex & 0x0F].WscIEBeacon.ValueLen = 0;
				pAd->ApCfg.MBSSID[BssIndex & 0x0F].WscIEProbeResp.ValueLen = 0;
			}
		}
	}
	else
#endif /* CONFIG_AP_SUPPORT */
	{
#ifdef P2P_SUPPORT
		if (pWscControl->WscConfMode == WSC_DISABLE)
		{
			if (BssIndex >= MIN_NET_DEVICE_FOR_P2P_GO)
			{
				pAd->ApCfg.MBSSID[MAIN_MBSSID].WscIEBeacon.ValueLen = 0;
				pAd->ApCfg.MBSSID[MAIN_MBSSID].WscIEProbeResp.ValueLen = 0;
			}
			return;
		}
#if 0
		if ((CurOpMode == AP_MODE) && (BssIndex >= MIN_NET_DEVICE_FOR_P2P_GO))
			BssIndex -= MIN_NET_DEVICE_FOR_P2P_GO; /* P2P GO build WSC_IE */
#endif
#endif /* P2P_SUPPORT */

		WscInitRegistrarPair(pAd, pWscControl, BssIndex);		
#ifdef CONFIG_AP_SUPPORT
		if (CurOpMode == AP_MODE)
		{
#ifdef APCLI_SUPPORT
			if (!bFromApCli)
#endif /* APCLI_SUPPORT */
			{
				IsAPConfigured = pWscControl->WscConfStatus;
				WscBuildBeaconIE(pAd, IsAPConfigured, FALSE, 0, 0, (BssIndex & 0x0F), NULL, 0, AP_MODE);
				WscBuildProbeRespIE(pAd, WSC_MSGTYPE_AP_WLAN_MGR, IsAPConfigured, FALSE, 0, 0, BssIndex, NULL, 0, AP_MODE);
				APUpdateBeaconFrame(pAd, pWscControl->EntryIfIdx & 0x0F);
			}
		}
#endif /* CONFIG_AP_SUPPORT */        
	}
#ifdef WSC_NFC_SUPPORT
	pWscControl->bTriggerByNFC = FALSE;
	pWscControl->bRegenPublicKey = FALSE;
	NdisZeroMemory(&pWscControl->NfcPasswd[0], NFC_DEV_PASSWD_LEN);
#endif /* WSC_NFC_SUPPORT */
}

USHORT WscGetAuthType(
    IN NDIS_802_11_AUTHENTICATION_MODE authType)
{
	switch(authType)
	{
		case Ndis802_11AuthModeOpen:
			return WSC_AUTHTYPE_OPEN;
		case Ndis802_11AuthModeWPAPSK:
			return WSC_AUTHTYPE_WPAPSK;
		case Ndis802_11AuthModeShared:
			return WSC_AUTHTYPE_SHARED;
		case Ndis802_11AuthModeWPANone:
			return WSC_AUTHTYPE_WPANONE;
		case Ndis802_11AuthModeWPA:
			return WSC_AUTHTYPE_WPA;
		case Ndis802_11AuthModeWPA1WPA2:
			return (WSC_AUTHTYPE_WPA | WSC_AUTHTYPE_WPA2);
		case Ndis802_11AuthModeWPA2:
			return WSC_AUTHTYPE_WPA2;
		case Ndis802_11AuthModeWPA1PSKWPA2PSK:
			return (WSC_AUTHTYPE_WPAPSK | WSC_AUTHTYPE_WPA2PSK);
		case Ndis802_11AuthModeWPA2PSK:
			return WSC_AUTHTYPE_WPA2PSK;
		default:
			return WSC_AUTHTYPE_OPEN;
	}
}

USHORT WscGetEncryType(
    IN NDIS_802_11_WEP_STATUS encryType)
{
	switch(encryType)
	{
		case Ndis802_11WEPDisabled:
			return WSC_ENCRTYPE_NONE;
		case Ndis802_11WEPEnabled:
			return WSC_ENCRTYPE_WEP;
		case Ndis802_11TKIPEnable:
			return WSC_ENCRTYPE_TKIP;
		case Ndis802_11TKIPAESMix:
			return (WSC_ENCRTYPE_AES | WSC_ENCRTYPE_TKIP);
		 default:
		case Ndis802_11AESEnable:
			return WSC_ENCRTYPE_AES;
	}
}

RTMP_STRING *WscGetAuthTypeStr(
    IN  USHORT authFlag)
{
	switch(authFlag)
	{
		case WSC_AUTHTYPE_OPEN:
			return "OPEN";
		case WSC_AUTHTYPE_WPAPSK:
			return "WPAPSK";
		case WSC_AUTHTYPE_SHARED:
			return "SHARED";
		case WSC_AUTHTYPE_WPANONE:
			return "WPANONE";
		case WSC_AUTHTYPE_WPA:
			return "WPA";
		case WSC_AUTHTYPE_WPA2:
			return "WPA2";
		default:
		case (WSC_AUTHTYPE_WPAPSK | WSC_AUTHTYPE_WPA2PSK):
			return "WPAPSKWPA2PSK";
		case WSC_AUTHTYPE_WPA2PSK:
			return "WPA2PSK";
		case (WSC_AUTHTYPE_OPEN | WSC_AUTHTYPE_SHARED):
			return "WEPAUTO";
	}
}

RTMP_STRING *WscGetEncryTypeStr(
    IN  USHORT encryFlag)
{
	switch(encryFlag)
	{
		case WSC_ENCRTYPE_NONE:
			return "NONE";
		case WSC_ENCRTYPE_WEP:
			return "WEP";
		case WSC_ENCRTYPE_TKIP:
			return "TKIP";
		default:
		case (WSC_ENCRTYPE_TKIP | WSC_ENCRTYPE_AES):
			return "TKIPAES";
		case WSC_ENCRTYPE_AES:
			return "AES";
	}
}

NDIS_802_11_AUTHENTICATION_MODE   WscGetAuthMode(
    IN  USHORT authFlag)
{
	switch(authFlag)
	{
		case WSC_AUTHTYPE_OPEN:
			return Ndis802_11AuthModeOpen;
		case WSC_AUTHTYPE_WPAPSK:
			return Ndis802_11AuthModeWPAPSK;
		case WSC_AUTHTYPE_SHARED:
			return Ndis802_11AuthModeShared;
		case WSC_AUTHTYPE_WPANONE:
			return Ndis802_11AuthModeWPANone;
		case WSC_AUTHTYPE_WPA:
			return Ndis802_11AuthModeWPA;
		case WSC_AUTHTYPE_WPA2:
			return Ndis802_11AuthModeWPA2;
		default:
		case (WSC_AUTHTYPE_WPAPSK | WSC_AUTHTYPE_WPA2PSK):
			return Ndis802_11AuthModeWPA1PSKWPA2PSK;
		case WSC_AUTHTYPE_WPA2PSK:
			return Ndis802_11AuthModeWPA2PSK;
	}
}

NDIS_802_11_WEP_STATUS   WscGetWepStatus(
    IN  USHORT encryFlag)
{
	switch(encryFlag)
	{
		case WSC_ENCRTYPE_NONE:
			return Ndis802_11WEPDisabled;
		case WSC_ENCRTYPE_WEP:
			return Ndis802_11WEPEnabled;
		case WSC_ENCRTYPE_TKIP:
			return Ndis802_11TKIPEnable;
		default:
		case (WSC_ENCRTYPE_TKIP | WSC_ENCRTYPE_AES):
			return Ndis802_11TKIPAESMix;
		case WSC_ENCRTYPE_AES:
			return Ndis802_11AESEnable;
	}
}

void    WscWriteConfToPortCfg(
    IN  PRTMP_ADAPTER   pAd,
    IN  PWSC_CTRL       pWscControl,
    IN  PWSC_CREDENTIAL pCredential,    
    IN  BOOLEAN         bEnrollee)
{
	UCHAR               CurApIdx = MAIN_MBSSID;
	UCHAR	CurOpMode = AP_MODE;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscWriteConfToPortCfg\n"));
    
#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	{
		if (pWscControl->EntryIfIdx == BSS0)
		CurOpMode = STA_MODE;
	}
#endif // CONFIG_STA_SUPPORT //

#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
	{
		CurApIdx = (pWscControl->EntryIfIdx & 0x0F);
	}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
	if (bEnrollee || (CurOpMode == AP_MODE))
	{
		if (CurOpMode == AP_MODE)
		{
			NdisZeroMemory(pAd->ApCfg.MBSSID[CurApIdx].Ssid, MAX_LEN_OF_SSID);
			NdisMoveMemory(pAd->ApCfg.MBSSID[CurApIdx].Ssid, pCredential->SSID.Ssid, pCredential->SSID.SsidLength);
			pAd->ApCfg.MBSSID[CurApIdx].SsidLen = pCredential->SSID.SsidLength;
#ifdef P2P_SUPPORT
			if (P2P_GO_ON(pAd))
			{
				NdisZeroMemory(pAd->P2pCfg.SSID, MAX_LEN_OF_SSID);
				pAd->P2pCfg.SSIDLen = pCredential->SSID.SsidLength;
				NdisMoveMemory(pAd->P2pCfg.SSID, pCredential->SSID.Ssid, pAd->P2pCfg.SSIDLen);
			}
#endif /* P2P_SUPPORT */
		}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
		if (CurOpMode == STA_MODE)
		{
			pAd->MlmeAux.AutoReconnectSsidLen = pCredential->SSID.SsidLength;
			NdisZeroMemory(pAd->MlmeAux.AutoReconnectSsid, NDIS_802_11_LENGTH_SSID);
			NdisMoveMemory(pAd->MlmeAux.AutoReconnectSsid, pCredential->SSID.Ssid, pAd->MlmeAux.AutoReconnectSsidLen);
			pAd->MlmeAux.SsidLen = pCredential->SSID.SsidLength;
			NdisZeroMemory(pAd->MlmeAux.Ssid, NDIS_802_11_LENGTH_SSID);
			NdisMoveMemory(pAd->MlmeAux.Ssid, pCredential->SSID.Ssid, pAd->MlmeAux.SsidLen);
			if (!NdisEqualMemory(pCredential->MacAddr, pAd->CurrentAddress, MAC_ADDR_LEN))
			{
				NdisZeroMemory(pAd->MlmeAux.Bssid, MAC_ADDR_LEN);
				NdisMoveMemory(pAd->MlmeAux.Bssid, pCredential->MacAddr, MAC_ADDR_LEN);
			}
		}
#endif /* CONFIG_STA_SUPPORT */

		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("ra%d - AuthType: %u, EncrType: %u\n", CurApIdx, pCredential->AuthType, pCredential->EncrType));
		if (pCredential->AuthType & (WSC_AUTHTYPE_WPAPSK | WSC_AUTHTYPE_WPA2PSK | WSC_AUTHTYPE_WPANONE))
		{
			if (!(pCredential->EncrType & (WSC_ENCRTYPE_TKIP | WSC_ENCRTYPE_AES)))
			{
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("AuthType is WPAPSK or WPA2PAK.\n"
                                         "Get illegal EncrType(%d) from External Registrar, set EncrType to TKIP\n", 
                                          pCredential->EncrType));
				pCredential->EncrType = WSC_ENCRTYPE_TKIP;
			}
#ifdef CONFIG_STA_SUPPORT
			if (CurOpMode == STA_MODE)
				pAd->StaCfg.WpaState = SS_START;
#endif /* CONFIG_STA_SUPPORT */
#ifdef CONFIG_AP_SUPPORT
#ifdef WSC_V2_SUPPORT
			if ((CurOpMode == AP_MODE) && (pWscControl->WscV2Info.bEnableWpsV2))
			{
				if (pCredential->AuthType == WSC_AUTHTYPE_WPAPSK)
					pCredential->AuthType = (WSC_AUTHTYPE_WPAPSK | WSC_AUTHTYPE_WPA2PSK);
				if (pCredential->EncrType == WSC_ENCRTYPE_TKIP)
					pCredential->EncrType = (WSC_ENCRTYPE_TKIP | WSC_ENCRTYPE_AES);
			}
#endif /* WSC_V2_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
		}
		WscSetAuthMode(pAd, CurOpMode, CurApIdx, WscGetAuthTypeStr(pCredential->AuthType));
		WscSetEncrypType(pAd, CurOpMode, CurApIdx, WscGetEncryTypeStr(pCredential->EncrType));
		if (pCredential->EncrType != WSC_ENCRTYPE_NONE)
		{
			if (pCredential->EncrType & (WSC_ENCRTYPE_TKIP | WSC_ENCRTYPE_AES))
			{
#ifdef CONFIG_AP_SUPPORT
				if (CurOpMode == AP_MODE)
					pAd->ApCfg.MBSSID[CurApIdx].wdev.DefaultKeyId = 1;
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
				if (CurOpMode == STA_MODE)
					pAd->StaCfg.wdev.DefaultKeyId = 0;
#endif /* CONFIG_STA_SUPPORT */

				if (pCredential->KeyLength >= 8 && pCredential->KeyLength <= 64)
				{
					UCHAR  *pPMKBuf = NULL, *pSSIDStr = NULL;
					INT		ssidLen = 0;
					RTMP_STRING PassphraseStr[65] = {0};
					
					pWscControl->WpaPskLen = pCredential->KeyLength;
					RTMPZeroMemory(pWscControl->WpaPsk, 64);
					RTMPMoveMemory(pWscControl->WpaPsk, pCredential->Key, pWscControl->WpaPskLen);
					RTMPMoveMemory(PassphraseStr, pCredential->Key, pWscControl->WpaPskLen);
#ifdef CONFIG_AP_SUPPORT
					if (CurOpMode == AP_MODE)
					{
						pPMKBuf = pAd->ApCfg.MBSSID[CurApIdx].PMK;
						pSSIDStr = (PUCHAR)pAd->ApCfg.MBSSID[CurApIdx].Ssid;
						ssidLen = pAd->ApCfg.MBSSID[CurApIdx].SsidLen;
					}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
					if (CurOpMode == STA_MODE)
					{
						RTMPZeroMemory(pAd->StaCfg.WpaPassPhrase, 64);
						RTMPMoveMemory(pAd->StaCfg.WpaPassPhrase, pCredential->Key, pWscControl->WpaPskLen);
						pAd->StaCfg.WpaPassPhraseLen = pCredential->KeyLength;
						pPMKBuf = pAd->StaCfg.PMK;
						pSSIDStr = (PUCHAR)pCredential->SSID.Ssid;
						ssidLen = pCredential->SSID.SsidLength;
					}
#endif /* CONFIG_STA_SUPPORT */
					RT_CfgSetWPAPSKKey(pAd, pCredential->Key, pWscControl->WpaPskLen, pSSIDStr, ssidLen, pPMKBuf);
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WpaPskLen = %d\n", pWscControl->WpaPskLen));
				}
				else
				{
					pWscControl->WpaPskLen = 0;
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WPAPSK: Invalid Key Length (%d)\n", pCredential->KeyLength));
				}
			}
			else if (pCredential->EncrType == WSC_ENCRTYPE_WEP)
			{
				UCHAR   WepKeyId = 0;
				USHORT  WepKeyLen = pCredential->KeyLength;

				if ((pCredential->KeyIndex >= 1) && (pCredential->KeyIndex <= 4))
				{
					WepKeyId = (pCredential->KeyIndex - 1); /* KeyIndex = 1 ~ 4 */
#ifdef CONFIG_AP_SUPPORT
					if (CurOpMode == AP_MODE)
						pAd->ApCfg.MBSSID[CurApIdx].wdev.DefaultKeyId = WepKeyId;
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT                
					if (CurOpMode == STA_MODE)
						pAd->StaCfg.wdev.DefaultKeyId = WepKeyId;
#endif /* CONFIG_STA_SUPPORT */

					 /* 5 or 13 ASCII characters */
					/* 10 or 26 Hex characters */
					if (WepKeyLen == 5 || WepKeyLen == 13 || WepKeyLen == 10 || WepKeyLen == 26)
					{
						if (WepKeyLen == 5 || WepKeyLen == 13)
						{
							pAd->SharedKey[CurApIdx][WepKeyId].KeyLen = (UCHAR)WepKeyLen;
							memcpy(pAd->SharedKey[CurApIdx][WepKeyId].Key, 
								pCredential->Key, 
								WepKeyLen);
							if (WepKeyLen == 5)
								pAd->SharedKey[CurApIdx][WepKeyId].CipherAlg = CIPHER_WEP64;
							else
								pAd->SharedKey[CurApIdx][WepKeyId].CipherAlg = CIPHER_WEP128;
						}
						else
						{
							pAd->SharedKey[CurApIdx][WepKeyId].KeyLen = (UCHAR)(WepKeyLen/2);
							AtoH((RTMP_STRING *) pCredential->Key, pAd->SharedKey[CurApIdx][WepKeyId].Key, WepKeyLen/2);
							if (WepKeyLen == 10)
								pAd->SharedKey[CurApIdx][WepKeyId].CipherAlg = CIPHER_WEP64;
							else
								pAd->SharedKey[CurApIdx][WepKeyId].CipherAlg = CIPHER_WEP128;
						}
					}
					else
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WEP: Invalid Key Length (%d)\n", pCredential->KeyLength));
				}
				else
				{
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Unsupport default key index (%d)\n", WepKeyId));
#ifdef CONFIG_AP_SUPPORT
					if (CurOpMode == AP_MODE)
						pAd->ApCfg.MBSSID[CurApIdx].wdev.DefaultKeyId = 0;
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
					if (CurOpMode == STA_MODE)
						pAd->StaCfg.wdev.DefaultKeyId = 0;
#endif /* CONFIG_STA_SUPPORT */

				}
			}
		}
#ifdef CONFIG_AP_SUPPORT
	}
	else
	{
		if (CurOpMode == AP_MODE)
		{
			pAd->ApCfg.MBSSID[CurApIdx].wdev.DefaultKeyId = 1;
			WscSetAuthMode(pAd, CurOpMode, CurApIdx, "WPAPSKWPA2PSK");
			WscSetEncrypType(pAd, CurOpMode, CurApIdx, "TKIPAES");
			pWscControl->WpaPskLen = (INT)pCredential->KeyLength;
			NdisZeroMemory(pWscControl->WpaPsk, 64);
			NdisMoveMemory(pWscControl->WpaPsk, pCredential->Key, pWscControl->WpaPskLen);
			/* Copy SSID */
			NdisZeroMemory(pAd->ApCfg.MBSSID[CurApIdx].Ssid, MAX_LEN_OF_SSID);
			NdisMoveMemory(pAd->ApCfg.MBSSID[CurApIdx].Ssid, pCredential->SSID.Ssid, pCredential->SSID.SsidLength);
			pAd->ApCfg.MBSSID[CurApIdx].SsidLen = pCredential->SSID.SsidLength;
			/*
				Hex Key 
			*/
			if(pWscControl->WscKeyASCII == 0)
			{
				AtoH((RTMP_STRING *) pWscControl->WpaPsk, pAd->ApCfg.MBSSID[CurApIdx].PMK, 32);
			}
			else
			{
				UCHAR       keyMaterial[40] = {0};
				
				RtmpPasswordHash((RTMP_STRING *)pWscControl->WpaPsk,
							 (PUCHAR) pAd->ApCfg.MBSSID[CurApIdx].Ssid, 
							 pAd->ApCfg.MBSSID[CurApIdx].SsidLen, 
							 keyMaterial);
				NdisMoveMemory(pAd->ApCfg.MBSSID[CurApIdx].PMK, keyMaterial, 32);
			}
		}
	}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT	
	if (CurOpMode == STA_MODE)
	{
		/*
			Atheros WPS Testbed AP will put A-Band BSSID in credential of M7.
			To prevent 2.4G only STA would fail to re-connect by BSSID, set profile retry timer here.
		*/
		if ((pAd->StaCfg.BssType == BSS_INFRA) &&
			(pWscControl->WscDriverAutoConnect == 2) &&
			(pWscControl->WscProfile.ProfileCnt >= 1))
		{
			pWscControl->WscProfileRetryTimerRunning = TRUE;
			RTMPSetTimer(&pWscControl->WscProfileRetryTimer, WSC_PROFILE_RETRY_TIME_OUT);
		}
		
#ifdef IWSC_SUPPORT
		if ((pAd->StaCfg.BssType == BSS_ADHOC) && 
			(pAd->StaCfg.IWscInfo.RegDepth != 0) &&
			(pAd->StaCfg.IWscInfo.AvaSubMaskListCount != 0))
		{
			if ((pCredential->AvaIpv4SubmaskList[0] == 0) &&
				(pCredential->AvaIpv4SubmaskList[1] == 0) &&
				(pCredential->AvaIpv4SubmaskList[2] == 0))
				pAd->StaCfg.IWscInfo.AvaSubMaskListCount = 0;
		}
#endif /* IWSC_SUPPORT */
	}
#endif /* CONFIG_STA_SUPPORT */

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- ra%d - WscWriteConfToPortCfg\n", CurApIdx));
}


VOID	WscWriteSsidToDatFile(
	IN  PRTMP_ADAPTER	pAd,
	IN  RTMP_STRING *pTempStr,
	IN	BOOLEAN			bNewFormat,
	IN  UCHAR			CurOpMode)
{
#ifdef CONFIG_AP_SUPPORT
	UCHAR	apidx;
#endif /* CONFIG_AP_SUPPORT */
	INT		offset = 0;

	if (bNewFormat == FALSE)
	{		
		NdisZeroMemory(pTempStr, 512);
		
#ifdef CONFIG_AP_SUPPORT
		if (CurOpMode == AP_MODE)
		{
			for (apidx = 0; apidx < pAd->ApCfg.BssidNum; apidx++)
			{
				if (apidx == 0)
				{
					NdisMoveMemory(pTempStr, "SSID=", strlen("SSID="));
					offset = strlen(pTempStr);
				}
				else 
				{
					offset = strlen(pTempStr);
					NdisMoveMemory(pTempStr + offset, ";", 1);
					offset += 1;
				}
				NdisMoveMemory(pTempStr + offset, pAd->ApCfg.MBSSID[apidx].Ssid, pAd->ApCfg.MBSSID[apidx].SsidLen);
			}
		}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
		if (CurOpMode == STA_MODE)
		{
			UINT profile_idx = pAd->StaCfg.WscControl.WscProfile.ApplyProfileIdx;
			PWSC_CREDENTIAL pCredential = &pAd->StaCfg.WscControl.WscProfile.Profile[profile_idx];
			NdisMoveMemory(pTempStr, "SSID=", strlen("SSID="));
			offset = strlen(pTempStr);
			NdisMoveMemory(pTempStr + offset, pCredential->SSID.Ssid, pCredential->SSID.SsidLength);
		}
#endif /* CONFIG_STA_SUPPORT */
	}
#ifdef CONFIG_AP_SUPPORT
	else
	{
		RTMP_STRING item_str[10] = {0};
		for (apidx = 0; apidx < pAd->ApCfg.BssidNum; apidx++)
		{
			snprintf(item_str, sizeof(item_str), "SSID%d", (apidx + 1));
			if (rtstrstr(pTempStr, item_str))
			{
				NdisZeroMemory(pTempStr, 512);
				NdisMoveMemory(pTempStr, item_str, strlen(item_str));
				offset = strlen(pTempStr);
				NdisMoveMemory(pTempStr + offset, "=", 1);
				offset += 1;
				NdisMoveMemory(pTempStr + offset, pAd->ApCfg.MBSSID[apidx].Ssid, pAd->ApCfg.MBSSID[apidx].SsidLen);
			}
			NdisZeroMemory(item_str, 10);
		}
	}
#endif /* CONFIG_AP_SUPPORT */
}


VOID	WscWriteWpaPskToDatFile(
	IN  PRTMP_ADAPTER	pAd,
	IN  RTMP_STRING *pTempStr,
	IN	BOOLEAN			bNewFormat)
{
#ifdef CONFIG_AP_SUPPORT
	UCHAR			apidx;
#endif /* CONFIG_AP_SUPPORT */
	PWSC_CTRL		pWscControl;
	INT				offset = 0;

	if (bNewFormat == FALSE)
	{		
		NdisZeroMemory(pTempStr, 512);
		
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		{
			for (apidx = 0; apidx < pAd->ApCfg.BssidNum; apidx++)
			{
				pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
				if (apidx == 0)
				{
					NdisMoveMemory(pTempStr, "WPAPSK=", strlen("WPAPSK="));
					offset = strlen(pTempStr);
				}
				else 
				{
					offset = strlen(pTempStr);
					NdisMoveMemory(pTempStr + offset, ";", 1);
					offset += 1;
				}
				NdisMoveMemory(pTempStr + offset, pWscControl->WpaPsk, pWscControl->WpaPskLen);
			}
		}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
		{
			pWscControl = &pAd->StaCfg.WscControl;
			NdisMoveMemory(pTempStr, "WPAPSK=", strlen("WPAPSK="));
			if (pWscControl->WpaPskLen)
			{
				offset = strlen(pTempStr);				
				NdisMoveMemory(pTempStr + offset, pWscControl->WpaPsk, pWscControl->WpaPskLen);
			}
		}
#endif /* CONFIG_STA_SUPPORT */
	}
#ifdef CONFIG_AP_SUPPORT
	else
	{
		RTMP_STRING item_str[10] = {0};
		for (apidx = 0; apidx < pAd->ApCfg.BssidNum; apidx++)
		{
			snprintf(item_str, sizeof(item_str), "WPAPSK%d", (apidx + 1));
			if (rtstrstr(pTempStr, item_str))
			{
				pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
				NdisZeroMemory(pTempStr, 512);
				NdisMoveMemory(pTempStr, item_str, strlen(item_str));
				offset = strlen(pTempStr);
				NdisMoveMemory(pTempStr + offset, "=", 1);
				offset += 1;
				NdisMoveMemory(pTempStr + offset, pWscControl->WpaPsk, pWscControl->WpaPskLen);
			}
			NdisZeroMemory(item_str, 10);
		}
	}
#endif /* CONFIG_AP_SUPPORT */
}

BOOLEAN WscCheckNonce(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	MLME_QUEUE_ELEM	*pElem,
	IN  BOOLEAN         bFlag,
	IN  PWSC_CTRL       pWscControl) 
{
    USHORT				Length;
	PUCHAR				pData;
	USHORT				WscType, WscLen, WscId;

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscCheckNonce\n"));
    
    if (bFlag)
    {
        /* check Registrar Nonce */
        WscId = WSC_ID_REGISTRAR_NONCE;
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("check Registrar Nonce\n"));
    }
    else
    {
        /* check Enrollee Nonce */
        WscId = WSC_ID_ENROLLEE_NONCE;
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("check Enrollee Nonce\n"));
    }
    
    pData = pElem->Msg;
    Length = pElem->MsgLen;

    /* We have to look for WSC_IE_MSG_TYPE to classify M2 ~ M8, the remain size must large than 4 */
	while (Length > 4)
	{
		WSC_IE	TLV_Recv;
        char ZeroNonce[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        
		memcpy((UINT8 *)&TLV_Recv, pData, 4);
		WscType = be2cpu16(TLV_Recv.Type);
		WscLen  = be2cpu16(TLV_Recv.Length);
		pData  += 4;
		Length -= 4;
        
		if (WscType == WscId)
		{
			if (RTMPCompareMemory(pWscControl->RegData.SelfNonce, pData, 16) == 0)
			{
			    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Nonce match!!\n"));
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscCheckNonce\n"));
				return TRUE;
			}
            else if (NdisEqualMemory(pData, ZeroNonce, 16))
            {
                /* Intel external registrar will send WSC_NACK with enrollee nonce */
                /* "10 1A 00 10 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00" */
                /* when AP is configured and user selects not to configure AP. */
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Zero Enrollee Nonce!!\n"));
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscCheckNonce\n"));
                return TRUE;
            }
		}
        
		/* Offset to net WSC Ie */
		pData  += WscLen;
		Length -= WscLen;
	}

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Nonce mismatch!!\n"));
    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscCheckNonce\n"));
    return FALSE;
}

VOID    WscGetRegDataPIN(
    IN  PRTMP_ADAPTER   pAdapter,
    IN  UINT            PinCode,
    IN  PWSC_CTRL       pWscControl)
{
	UCHAR	tempPIN[9] = {0};

    if ((pWscControl->WscMode == WSC_PBC_MODE) ||
		(pWscControl->WscMode == WSC_SMPBC_MODE))
        pWscControl->WscPinCode = 0;
    else
        pWscControl->WscPinCode = PinCode;

    memset(pWscControl->RegData.PIN, 0, 8);

	if (pWscControl->WscPinCode == 0)
	{
		snprintf((RTMP_STRING *) tempPIN, sizeof(tempPIN), "00000000");
		memcpy(pWscControl->RegData.PIN, tempPIN, 8);
		pWscControl->RegData.PinCodeLen = 8;
	}
	else
	{
		if ( pWscControl->WscPinCodeLen == 4)
		{
			UCHAR	temp4PIN[5] = {0};
			snprintf((RTMP_STRING *) temp4PIN, sizeof(temp4PIN), "%04u", pWscControl->WscPinCode);
			memcpy(pWscControl->RegData.PIN, temp4PIN, 4);
			pWscControl->RegData.PinCodeLen = 4;
		}
		else
		{
			snprintf((RTMP_STRING *) tempPIN, sizeof(tempPIN), "%08u", pWscControl->WscPinCode);
			memcpy(pWscControl->RegData.PIN, tempPIN, 8);
			pWscControl->RegData.PinCodeLen = 8;
		}
	}
	hex_dump("WscGetRegDataPIN - PIN", pWscControl->RegData.PIN, 8);
}

VOID    WscEapActionDisabled(
    IN  PRTMP_ADAPTER       pAdapter,
    IN  PWSC_CTRL           pWscControl)
{
	INT     DataLen = 0;
	UCHAR   *WscData = NULL;
	/*BOOLEAN Cancelled;*/

	os_alloc_mem(NULL, &WscData, 256);

	if (WscData == NULL)
		return;
	
	DataLen = BuildMessageNACK(pAdapter, pWscControl, WscData);

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAdapter)
	{
		if (pWscControl->EntryIfIdx & MIN_NET_DEVICE_FOR_APCLI)
			WscSendMessage(pAdapter, WSC_OPCODE_NACK, WscData, DataLen, pWscControl, AP_CLIENT_MODE, EAP_CODE_RSP);
		else
			WscSendMessage(pAdapter, WSC_OPCODE_NACK, WscData, DataLen, pWscControl, AP_MODE, EAP_CODE_REQ);
	}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAdapter)
	{
		if (ADHOC_ON(pAdapter) && (pWscControl->WscConfMode & WSC_REGISTRAR))
			WscSendMessage(pAdapter, WSC_OPCODE_NACK, WscData, DataLen, pWscControl, STA_MODE, EAP_CODE_REQ);
		else
			WscSendMessage(pAdapter, WSC_OPCODE_NACK, WscData, DataLen, pWscControl, STA_MODE, EAP_CODE_RSP);
	}
#endif /* CONFIG_STA_SUPPORT */

	if (WscData)
		os_free_mem(NULL, WscData);
	
	/* RTMPCancelTimer(&pWscControl->EapolTimer, &Cancelled); */
	/* pWscControl->EapolTimerRunning = FALSE; */
}


VOID WscGetConfigErrFromNack(
    IN RTMP_ADAPTER *pAdapter,
    IN MLME_QUEUE_ELEM *pElem,
    OUT USHORT *pConfigError)
{
    USHORT				Length = 0;
	PUCHAR				pData;
	USHORT				WscType, WscLen, ConfigError = 0;

    pData = pElem->Msg;
    Length = pElem->MsgLen;
    
	while (Length > 4)
	{
		WSC_IE	TLV_Recv;
		memcpy((UINT8 *)&TLV_Recv, pData, 4);
		WscType = be2cpu16(TLV_Recv.Type);
		WscLen  = be2cpu16(TLV_Recv.Length);
		pData  += 4;
		Length -= 4;
        
		if (WscType == WSC_ID_CONFIG_ERROR)
		{
			NdisMoveMemory(&ConfigError, pData, sizeof(USHORT));
		    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WSC_ID_CONFIG_ERROR: %d\n", ntohs(ConfigError)));
			*pConfigError = ntohs(ConfigError);
			return;
		}
        
		/* Offset to net WSC Ie */
		pData  += WscLen;
		Length -= WscLen;
	}
    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WSC_ID_CONFIG_ERROR is missing\n"));
}


INT	WscSetAuthMode(
	IN	PRTMP_ADAPTER	pAd, 
	IN  UCHAR			CurOpMode,
	IN  UCHAR			apidx,
	IN	RTMP_STRING *arg)
{
#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
	{
		UINT32	i;
		struct wifi_dev *wdev = &pAd->ApCfg.MBSSID[apidx].wdev;
		
		if (rtstrcasecmp(arg, "WEPAUTO") == TRUE)
			wdev->AuthMode = Ndis802_11AuthModeAutoSwitch;
		else if (rtstrcasecmp(arg, "OPEN") == TRUE)
			wdev->AuthMode = Ndis802_11AuthModeOpen;
		else if (rtstrcasecmp(arg, "SHARED") == TRUE)
			wdev->AuthMode = Ndis802_11AuthModeShared;
		else if (rtstrcasecmp(arg, "WPAPSK") == TRUE)
			wdev->AuthMode = Ndis802_11AuthModeWPAPSK;
		else if (rtstrcasecmp(arg, "WPA") == TRUE)
			wdev->AuthMode = Ndis802_11AuthModeWPA;
		else if (rtstrcasecmp(arg, "WPA2PSK") == TRUE)
			wdev->AuthMode = Ndis802_11AuthModeWPA2PSK;
		else if (rtstrcasecmp(arg, "WPA2") == TRUE)
			wdev->AuthMode = Ndis802_11AuthModeWPA2;
		else if (rtstrcasecmp(arg, "WPA1WPA2") == TRUE)
			wdev->AuthMode = Ndis802_11AuthModeWPA1WPA2;
		else if (rtstrcasecmp(arg, "WPAPSKWPA2PSK") == TRUE)
			wdev->AuthMode = Ndis802_11AuthModeWPA1PSKWPA2PSK;
		else
		{
			wdev->AuthMode = Ndis802_11AuthModeOpen;
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: Unknow AuthMode (%s), set AuthMode to OPEN\n", __FUNCTION__, arg));
		}

		for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
		{
			if (IS_ENTRY_CLIENT(&pAd->MacTab.Content[i]))
			{
				pAd->MacTab.tr_entry[i].PortSecured  = WPA_802_1X_PORT_NOT_SECURED;
			}
		}
		pAd->ApCfg.MBSSID[apidx].wdev.PortSecured = WPA_802_1X_PORT_NOT_SECURED;
		/*RTMPMakeRSNIE(pAd, pAd->ApCfg.MBSSID[apidx].AuthMode, pAd->ApCfg.MBSSID[apidx].WepStatus, apidx); */

		wdev->DefaultKeyId  = 0;

		if(wdev->AuthMode >= Ndis802_11AuthModeWPA)
			wdev->DefaultKeyId = 0;

		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("IF(ra%d) %s::(AuthMode=%d)\n", apidx, __FUNCTION__, wdev->AuthMode));    
	}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
	{
		struct wifi_dev *wdev = &pAd->StaCfg.wdev;
		
		if (strcmp(arg, "WEPAUTO") == 0)
			wdev->AuthMode = Ndis802_11AuthModeAutoSwitch;
		else if (strcmp(arg, "OPEN") == 0)
			wdev->AuthMode = Ndis802_11AuthModeOpen;
		else if (strcmp(arg, "SHARED") == 0)
			wdev->AuthMode = Ndis802_11AuthModeShared;
		else if (strcmp(arg, "WPAPSK") == 0)
			wdev->AuthMode = Ndis802_11AuthModeWPAPSK;
		else if (strcmp(arg, "WPANONE") == 0)
			wdev->AuthMode = Ndis802_11AuthModeWPANone;
		else if ((strcmp(arg, "WPA2PSK") == 0) || (strcmp(arg, "WPAPSKWPA2PSK") == 0))
			wdev->AuthMode = Ndis802_11AuthModeWPA2PSK;    
#ifdef WPA_SUPPLICANT_SUPPORT
		else if (strcmp(arg, "WPA") == 0)
			wdev->AuthMode = Ndis802_11AuthModeWPA;    
		else if ((strcmp(arg, "WPA2") == 0) || (strcmp(arg, "WPA1WPA2") == 0))
			wdev->AuthMode = Ndis802_11AuthModeWPA2;
#endif /* WPA_SUPPLICANT_SUPPORT */
		else
			return FALSE;  

		if ((pAd->StaCfg.BssType == BSS_ADHOC) && (wdev->AuthMode == Ndis802_11AuthModeWPAPSK))
			wdev->AuthMode = Ndis802_11AuthModeWPANone;

		wdev->PortSecured = WPA_802_1X_PORT_NOT_SECURED;

		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscSetAuthMode::(AuthMode=%d)\n", wdev->AuthMode));
	}
#endif /* CONFIG_STA_SUPPORT */

	return TRUE;
}

INT	WscSetEncrypType(
	IN	PRTMP_ADAPTER	pAd, 
	IN  UCHAR			CurOpMode,
	IN  UCHAR			apidx,
	IN	RTMP_STRING *arg)
{
#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
	{
		struct wifi_dev *wdev = &pAd->ApCfg.MBSSID[apidx].wdev;

		if (rtstrcasecmp(arg, "NONE") == TRUE)
			wdev->WepStatus = Ndis802_11WEPDisabled;
		else if (rtstrcasecmp(arg, "WEP") == TRUE)
			wdev->WepStatus = Ndis802_11WEPEnabled;
		else if (rtstrcasecmp(arg, "TKIP") == TRUE)
			wdev->WepStatus = Ndis802_11TKIPEnable;
		else if (rtstrcasecmp(arg, "AES") == TRUE)
			wdev->WepStatus = Ndis802_11AESEnable;
		else if (rtstrcasecmp(arg, "TKIPAES") == TRUE)
			wdev->WepStatus = Ndis802_11TKIPAESMix;
		else
	    	{
			wdev->WepStatus = Ndis802_11WEPDisabled;
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: Unknow EncrypType (%s), set EncrypType to NONE\n", __FUNCTION__, arg));
		}

		if (wdev->WepStatus >= Ndis802_11TKIPEnable)
			wdev->DefaultKeyId = 0;

		/*RTMPMakeRSNIE(pAd, pAd->ApCfg.MBSSID[apidx].AuthMode, pAd->ApCfg.MBSSID[apidx].WepStatus, apidx); */
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("IF(ra%d) %s::(EncrypType=%d)\n", apidx, __FUNCTION__, wdev->WepStatus));

		return TRUE;
	}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
	{
		struct wifi_dev *wdev = &pAd->StaCfg.wdev;

		if (strcmp(arg, "NONE") == 0)
		{
			if (wdev->AuthMode >= Ndis802_11AuthModeWPA)
				return TRUE;    /* do nothing */
            
			wdev->WepStatus     = Ndis802_11WEPDisabled;
			pAd->StaCfg.PairCipher    = Ndis802_11WEPDisabled;
			pAd->StaCfg.GroupCipher   = Ndis802_11WEPDisabled;
		}
		else if (strcmp(arg, "WEP") == 0)
		{
			if (wdev->AuthMode >= Ndis802_11AuthModeWPA)
				return TRUE;    /* do nothing */

			wdev->WepStatus = Ndis802_11WEPEnabled;
			pAd->StaCfg.PairCipher = Ndis802_11WEPEnabled;
			pAd->StaCfg.GroupCipher = Ndis802_11WEPEnabled;
		}
		else if (strcmp(arg, "TKIP") == 0)
		{
			if (wdev->AuthMode < Ndis802_11AuthModeWPA)
				return TRUE;    /* do nothing */

			wdev->WepStatus = Ndis802_11TKIPEnable;
			pAd->StaCfg.PairCipher = Ndis802_11TKIPEnable;
			pAd->StaCfg.GroupCipher = Ndis802_11TKIPEnable;
		}
		else if ((strcmp(arg, "AES") == 0) || (strcmp(arg, "TKIPAES") == 0))
		{
			if (wdev->AuthMode < Ndis802_11AuthModeWPA)
			return TRUE;    /* do nothing */

			wdev->WepStatus     = Ndis802_11AESEnable;
			pAd->StaCfg.PairCipher    = Ndis802_11AESEnable;
			pAd->StaCfg.GroupCipher   = Ndis802_11AESEnable;
		}
		else
			return FALSE;

		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscSetEncrypType::(EncrypType=%d)\n", wdev->WepStatus));
	}
#endif /* CONFIG_STA_SUPPORT */

	return TRUE;
}

#ifdef CONFIG_STA_SUPPORT
USHORT WscGetAuthTypeFromStr(RTMP_STRING *arg)
{
    if (rtstrcasecmp(arg, "OPEN") == TRUE)
        return WSC_AUTHTYPE_OPEN;
    else if (rtstrcasecmp(arg, "SHARED") == TRUE)
        return WSC_AUTHTYPE_SHARED;
    else if (rtstrcasecmp(arg, "WPAPSK") == TRUE)
        return WSC_AUTHTYPE_WPAPSK;
    else if (rtstrcasecmp(arg, "WPA2PSK") == TRUE)
        return WSC_AUTHTYPE_WPA2PSK;
#ifdef WPA_SUPPLICANT_SUPPORT
    else if (rtstrcasecmp(arg, "WPA") == TRUE)
        return WSC_AUTHTYPE_WPA;
    else if (rtstrcasecmp(arg, "WPA2") == TRUE)
        return WSC_AUTHTYPE_WPA2;
#endif /* WPA_SUPPLICANT_SUPPORT */
    else
        return 0;
}

USHORT WscGetEncrypTypeFromStr(RTMP_STRING *arg)
{
    if (rtstrcasecmp(arg, "NONE") == TRUE)
        return WSC_ENCRTYPE_NONE;
    else if (rtstrcasecmp(arg, "WEP") == TRUE)
        return WSC_ENCRTYPE_WEP;
    else if (rtstrcasecmp(arg, "TKIP") == TRUE)
        return WSC_ENCRTYPE_TKIP;
    else if (rtstrcasecmp(arg, "AES") == TRUE)
        return WSC_ENCRTYPE_AES;
    else
        return 0;
}
#endif /* CONFIG_STA_SUPPORT */

/*
	========================================================================
	
	Routine Description:
		Push PBC from HW/SW Buttton

	Arguments:
		pAd    - NIC Adapter pointer
		
	Return Value:
		None

	IRQL = DISPATCH_LEVEL
	
	Note:
		
	========================================================================
*/

VOID	WscPushPBCAction(
	IN	PRTMP_ADAPTER	pAd,
	IN  PWSC_CTRL   	pWscControl)
{
	BOOLEAN		Cancelled;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscPushPBCAction\n"));

	/* 0. PBC mode, disregard the SSID information, we have to get the current AP list */
	/*    and check the beacon for Push buttoned AP. */

	/* 1. Cancel old timer to prevent use push continuously */
	if (pWscControl->Wsc2MinsTimerRunning)
	{
		pWscControl->Wsc2MinsTimerRunning = FALSE;
		RTMPCancelTimer(&pWscControl->Wsc2MinsTimer, &Cancelled);
	}
	if (pWscControl->WscScanTimerRunning)
	{
		pWscControl->WscScanTimerRunning = FALSE;
		RTMPCancelTimer(&pWscControl->WscScanTimer, &Cancelled);
	}
	if (pWscControl->WscPBCTimerRunning)
	{
		pWscControl->WscPBCTimerRunning = FALSE;
		RTMPCancelTimer(&pWscControl->WscPBCTimer, &Cancelled);
	}
	
	/* Set WSC state to WSC_STATE_INIT */
	pWscControl->WscState = WSC_STATE_START;
	pWscControl->WscStatus = STATUS_WSC_SCAN_AP;

	/* Init Registrar pair structures */
	WscInitRegistrarPair(pAd, pWscControl, BSS0);

	/* For PBC, the PIN is all '0' */
	WscGetRegDataPIN(pAd, pWscControl->WscPinCode, pWscControl);
 				
	/* 2. Set 2 min timout routine */
	RTMPSetTimer(&pWscControl->Wsc2MinsTimer, WSC_TWO_MINS_TIME_OUT);
	pWscControl->Wsc2MinsTimerRunning = TRUE;
	pWscControl->bWscTrigger = TRUE;	/* start work */

#ifdef DPA_T
	pAd->StaCfg.WscPbcExtraRemainScanCount = pAd->StaCfg.WscPbcExtraScanCount;
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscPbcExtraRemainScanCount = %d\n", pAd->StaCfg.WscPbcExtraRemainScanCount));
#endif /* DPA_T */

	/* 3. Call WscScan subroutine */
	WscScanExec(pAd, pWscControl);

	/* 4. Set 10 second timer to invoke PBC connection actions. */
	RTMPSetTimer(&pWscControl->WscPBCTimer, 10000);
	pWscControl->WscPBCTimerRunning = TRUE;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscPushPBCAction\n"));
}

#ifdef APCLI_SUPPORT
VOID	WscPINAction(
	IN	PRTMP_ADAPTER	pAd,
	IN  PWSC_CTRL   	pWscControl)
{
	BOOLEAN		Cancelled;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscPINAction\n"));

	/* 0. PIN mode, disregard the SSID information, we have to get the current AP list */
	/*    and check the beacon for WPS Pin  AP. */

	/* 1. Cancel old timer to prevent use pin continuously */
	if (pWscControl->Wsc2MinsTimerRunning)
	{
		pWscControl->Wsc2MinsTimerRunning = FALSE;
		RTMPCancelTimer(&pWscControl->Wsc2MinsTimer, &Cancelled);
	}
	if (pWscControl->WscScanTimerRunning)
	{
		pWscControl->WscScanTimerRunning = FALSE;
		RTMPCancelTimer(&pWscControl->WscScanTimer, &Cancelled);
	}
	if (pWscControl->WscPINTimerRunning)
	{
		pWscControl->WscPINTimerRunning = FALSE;
		RTMPCancelTimer(&pWscControl->WscPINTimer, &Cancelled);
	}
	
	/* Set WSC state to WSC_STATE_INIT */
	pWscControl->WscState = WSC_STATE_START;
	pWscControl->WscStatus = STATUS_WSC_SCAN_AP;
    pWscControl->TryNextWpsPINAp = FALSE;
	/* Init Registrar pair structures */
	WscInitRegistrarPair(pAd, pWscControl, BSS0);
	
	NdisMoveMemory(pWscControl->RegData.SelfInfo.MacAddr,
		                     pAd->ApCfg.ApCliTab[BSS0].wdev.if_addr,
		                       6);	

	/* set PIN in case of registrar */
	WscGetRegDataPIN(pAd, pWscControl->WscPinCode, pWscControl);
 				
	/* 2. Set 2 min timout routine */
	RTMPSetTimer(&pWscControl->Wsc2MinsTimer, WSC_TWO_MINS_TIME_OUT);
	pWscControl->Wsc2MinsTimerRunning = TRUE;
	pWscControl->bWscTrigger = TRUE;	/* start work */


	/* 3. Call WscScan subroutine */
	WscScanExec(pAd, pWscControl);

	/* 4. Set 10 second timer to invoke PIN connection actions. */
	RTMPSetTimer(&pWscControl->WscPINTimer, 10000);
	pWscControl->WscPINTimerRunning = TRUE;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscPINAction\n"));
}
#endif /* APCLI_SUPPORT */

/*
	========================================================================
	
	Routine Description:
		Doing an active scan with empty SSID, the scanened list will
		be processed in PBCexec or PINexec routines

	Arguments:
		pAd         - NIC Adapter pointer
		
	Return Value:
		None
		
	IRQL = DISPATCH_LEVEL
	
	Note:
		
	========================================================================
*/
VOID	WscScanExec(
	IN	PRTMP_ADAPTER	pAd,
	IN  PWSC_CTRL   	pWscControl) 
{
#ifdef WSC_LED_SUPPORT
	UCHAR WPSLEDStatus;
#endif /* WSC_LED_SUPPORT */

	/* Prevent infinite loop if conncet time out didn't stop the repeat scan */
	if ((pWscControl->WscStatus == STATUS_WSC_FAIL) ||
		(pWscControl->WscState == WSC_STATE_OFF))
		return;
	
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("!!! WscScanExec !!!\n"));

	pWscControl->WscStatus = STATUS_WSC_SCAN_AP;

#ifdef WSC_LED_SUPPORT
	/* The protocol is connecting to a partner. */
	WPSLEDStatus = LED_WPS_IN_PROCESS;
	RTMPSetLED(pAd, WPSLEDStatus);
#endif /* WSC_LED_SUPPORT */

#ifdef APCLI_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		ApSiteSurvey(pAd, NULL, SCAN_WSC_ACTIVE, FALSE);
	}
#endif /* APCLI_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	{
		pAd->StaCfg.bNotFirstScan = TRUE;
		StaSiteSurvey(pAd, NULL, SCAN_WSC_ACTIVE);
	}
#endif /* CONFIG_STA_SUPPORT */

#ifdef DPA_S
#ifdef P2P_SUPPORT
	if (pWscControl->EntryIfIdx >= MIN_NET_DEVICE_FOR_P2P_CLI)
		RtmpOSWrielessEventSend(pAd->p2p_dev, RT_WLAN_EVENT_CUSTOM, P2P_NOTIF_WPS_STATUS_SCANNING, NULL, NULL, 0);
#endif /* P2P_SUPPORT */
#endif /* DPA_S */
}

/*
	========================================================================
	
	Routine Description:
		Doing PBC conenction verification, it will check current BSS list
		and find the correct number of PBC AP. If only 1 exists, it will
		start to make connection. Otherwise, it will set a scan timer
		to perform another scan for next PBC connection execution.

	Arguments:
		pAd         - NIC Adapter pointer
		
	Return Value:
		None
		
	IRQL = DISPATCH_LEVEL
	
	Note:
		
	========================================================================
*/
BOOLEAN	WscPBCExec(
	IN	PRTMP_ADAPTER	pAd,
	IN  BOOLEAN			bFromM2,
	IN  PWSC_CTRL       pWscControl)
{
#ifdef WSC_LED_SUPPORT
	UCHAR WPSLEDStatus;
#endif /* WSC_LED_SUPPORT */
#if defined(CONFIG_STA_SUPPORT) || defined(APCLI_SUPPORT)
	UCHAR CurOpMode = AP_MODE;
#endif /* defined(CONFIG_STA_SUPPORT) || defined(APCLI_SUPPORT) */

	if (pWscControl == NULL)
		return FALSE;

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	{
		if (pWscControl->EntryIfIdx == BSS0)
			CurOpMode = STA_MODE;
	}
#endif // CONFIG_STA_SUPPORT //

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("-----> WscPBCExec !!!\n"));
	
	/* 1. Search the qualified SSID from current SSID list */
	WscPBCBssTableSort(pAd, pWscControl);
				
	/* 2. Check the qualified AP for connection, if more than 1 AP avaliable, report error. */
	if (pWscControl->WscPBCBssCount != 1)
	{
		/* Set WSC state to WSC_FAIL */
		pWscControl->WscState = WSC_STATE_FAIL;
		if (pWscControl->WscPBCBssCount== 0)
		{
			pWscControl->WscStatus = STATUS_WSC_PBC_NO_AP;
#ifdef WSC_LED_SUPPORT
			/* Failed to find any partner. */
			WPSLEDStatus = LED_WPS_ERROR;
			RTMPSetLED(pAd, WPSLEDStatus);
#ifdef CONFIG_WIFI_LED_SUPPORT
				if (LED_MODE(pAd) == WPS_LED_MODE_SHARE)
					RTMPSetTimer(&pWscControl->WscLEDTimer, WSC_WPS_FAIL_WIFI_LED_TIMEOUT);
#endif /* CONFIG_WIFI_LED_SUPPORT */

#endif /* WSC_LED_SUPPORT */

			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("WscPBCExec --> AP list is %d, wait for next time\n", 
							pWscControl->WscPBCBssCount));

#ifdef CONFIG_STA_SUPPORT
			/*
				P2P PBC CLI doesn't need to check PBC overlapping, 
				so we don't need to consider P2P case here.
			*/
			if (pAd->StaCfg.BssType == BSS_INFRA)
#endif /* CONFIG_STA_SUPPORT */
			{
				/* 2.1. Set 1 second timer to invoke another scan */
				RTMPSetTimer(&pWscControl->WscScanTimer, 1000);
				pWscControl->WscScanTimerRunning = TRUE;
			}
		}
		else
		{
			pWscControl->WscStatus = STATUS_WSC_PBC_TOO_MANY_AP;			
#ifdef DPA_S
#ifdef P2P_SUPPORT
			if (pWscControl->EntryIfIdx >= MIN_NET_DEVICE_FOR_P2P_CLI)
				RtmpOSWrielessEventSend(pAd->p2p_dev, RT_WLAN_EVENT_CUSTOM, P2P_NOTIF_WPS_SESSION_OVERLAP, NULL, NULL, 0);
			else
#endif /* P2P_SUPPORT */
#endif /* DPA_S */
			RTMPSendWirelessEvent(pAd, IW_WSC_PBC_SESSION_OVERLAP, NULL, BSS0, 0); 

#ifdef WSC_LED_SUPPORT
			if (LED_MODE(pAd) == WPS_LED_MODE_9) /* WPS LED mode 9. */
			{
				/* In case of the WPS LED mode 9, the UI would abort the connection attempt by making the RT_OID_802_11_WSC_SET_WPS_STATE_MACHINE_TERMINATION request. */
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: Skip the WPS session overlap detected LED indication.\n", __FUNCTION__));
			}
			else /* Other LED mode. */
			{
			/* Session overlap detected. */
			WPSLEDStatus = LED_WPS_SESSION_OVERLAP_DETECTED;
			RTMPSetLED(pAd, WPSLEDStatus);
#ifdef CONFIG_WIFI_LED_SHARE
				if (LED_MODE(pAd) == WPS_LED_MODE_SHARE)
					RTMPSetTimer(&pWscControl->WscLEDTimer, WSC_WPS_OVERLAP_WIFI_LED_TIMEOUT);
#endif /* CONFIG_WIFI_LED_SHARE */


			}
#endif /* WSC_LED_SUPPORT */

			/*
				20101210 - According to the response from WFA:
				The station shall not continue scanning waiting for only one registrar to appear
			*/
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscPBCExec --> AP list is %d, stop WPS process!\n", 
						pWscControl->WscPBCBssCount));

			WscStop(pAd, 
#ifdef CONFIG_AP_SUPPORT
					FALSE,
#endif /* CONFIG_AP_SUPPORT */
					pWscControl);
			pWscControl->WscConfMode = WSC_DISABLE;
#ifdef CONFIG_STA_SUPPORT
			RTMPZeroMemory(pAd->MlmeAux.AutoReconnectSsid, MAX_LEN_OF_SSID);
			pAd->MlmeAux.AutoReconnectSsidLen = pAd->CommonCfg.SsidLen;
			RTMPMoveMemory(pAd->MlmeAux.AutoReconnectSsid, pAd->CommonCfg.Ssid, pAd->CommonCfg.SsidLen);
#endif /* CONFIG_STA_SUPPORT */
		}

#if 0		
		/* 2.1. Set 1 second timer to invoke another scan */
		RTMPSetTimer(&pWscControl->WscScanTimer, 1000);
        pWscControl->WscScanTimerRunning = TRUE;        
#endif
		/* 2.2 We have to quit for now */
		return FALSE;
	}

	if (bFromM2)
		return TRUE;

#ifdef DPA_T
	if (pAd->StaCfg.WscPbcExtraRemainScanCount > 0)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Need extra scan to confirm there is only one WPS PBC AP.\n"));
		pAd->StaCfg.WscPbcExtraRemainScanCount--;
		// Call WscScan subroutine
		WscScanExec(pAd, pWscControl);
		// Set 10 second timer to invoke PBC connection actions.
		RTMPSetTimer(&pAd->StaCfg.WscControl.WscPBCTimer, 10000);
		pAd->StaCfg.WscControl.WscPBCTimerRunning = TRUE;
		return FALSE;
	}
#endif /* DPA_T */

#ifdef CONFIG_STA_SUPPORT
	/* 3. Now we got the intend AP, Set the WSC state and enqueue the SSID connection command */
	pAd->MlmeAux.CurrReqIsFromNdis = FALSE; 
	if (pAd->Mlme.CntlMachine.CurrState != CNTL_IDLE)
	{
		RTMP_MLME_RESET_STATE_MACHINE(pAd);
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("!!! WscPBCExec --> MLME busy, reset MLME state machine !!!\n"));
	}
#endif /* CONFIG_STA_SUPPORT */
				
#ifdef CONFIG_STA_SUPPORT
	/* 4. Set WSC state to WSC_STATE_START */
	if (CurOpMode == STA_MODE)
	{
		pWscControl->WscState = WSC_STATE_START;
		pWscControl->WscStatus = STATUS_WSC_START_ASSOC;
	}
#endif /* CONFIG_STA_SUPPORT */

#ifdef WSC_LED_SUPPORT
	/* The protocol is connecting to a partner. */
	WPSLEDStatus = LED_WPS_IN_PROCESS;
	RTMPSetLED(pAd, WPSLEDStatus);
#endif /* WSC_LED_SUPPORT */

#ifdef APCLI_SUPPORT
	if (CurOpMode == AP_MODE)
	{
#ifdef RTMP_TIMER_TASK_SUPPORT	
		RTMP_STRING ChStr[5] = {0};
#else
		UCHAR channel;
#endif

		NdisMoveMemory(pWscControl->RegData.SelfInfo.MacAddr,
	                   pAd->ApCfg.ApCliTab[BSS0].wdev.if_addr,
	                   MAC_ADDR_LEN);
#ifdef RTMP_TIMER_TASK_SUPPORT
		// TODO: shiang-usw, check about this Channel setting here? Original it indicate to pAd->MlmeAux.Channel!
		snprintf(ChStr, sizeof(ChStr), "%d", pAd->ApCfg.ApCliTab[BSS0].MlmeAux.Channel);
		Set_Channel_Proc(pAd, ChStr);
#else
		channel = pAd->ApCfg.ApCliTab[BSS0].MlmeAux.Channel;
		RTEnqueueInternalCmd(pAd, CMDTHREAD_APCLI_PBC_TIMEOUT, (VOID *)&channel, sizeof(UCHAR));
#endif			
	    /* bring apcli interface down first */
		if(pAd->ApCfg.ApCliTab[BSS0].Enable == TRUE)
		{
			pAd->ApCfg.ApCliTab[BSS0].Enable = FALSE;
			ApCliIfDown(pAd);
		}
	    pAd->ApCfg.ApCliTab[BSS0].Enable = TRUE;
	}
#endif /* APCLI_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	/* Enqueue BSSID connection command */
	if (CurOpMode == STA_MODE)
	{
		if (pAd->StaCfg.BssType == BSS_INFRA)
		{
			MlmeEnqueue(pAd, 
					MLME_CNTL_STATE_MACHINE, 
					OID_802_11_BSSID, 
					sizeof(NDIS_802_11_MAC_ADDRESS),
						(VOID *)&pWscControl->WscBssid[0], 0);
		}
		else
		{
			MlmeEnqueue(pAd,
				MLME_CNTL_STATE_MACHINE,
				OID_802_11_SSID,
				sizeof(NDIS_802_11_SSID),
				(VOID *)&pAd->StaCfg.WscControl.WscSsid, 0);
		}
	}
#endif /* CONFIG_STA_SUPPORT */

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("<----- WscPBCExec !!!\n"));
	return TRUE;
}

#ifdef APCLI_SUPPORT
BOOLEAN	WscPINExec(
	IN	PRTMP_ADAPTER	pAd,
	IN  PWSC_CTRL       pWscControl)
{

	if (pWscControl == NULL)
		return FALSE;


	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("-----> WscPINExec !!!\n"));
	
	WscPINBssTableSort(pAd, pWscControl,  pWscControl->ApUuidBssid);	
	if (pWscControl->WscPINBssCount== 0)
	{
			pWscControl->WscStatus = STATUS_WSC_PIN_NO_AP;
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("WscPINExec --> AP list is %d, wait for next time\n", 
						pWscControl->WscPINBssCount));
			/* Set 1 second timer to invoke another scan */
			RTMPSetTimer(&pWscControl->WscScanTimer, 1000);
			pWscControl->WscScanTimerRunning = TRUE;
	
	}
		

	
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("<----- WscPINExec !!!\n"));
	return TRUE;
}
#endif /* APCLI_SUPPORT */

BOOLEAN WscBssWpsIESearchForPBC(
	RTMP_ADAPTER *pAd,
	WSC_CTRL *pWscControl,	
	BSS_ENTRY *pInBss,
	UUID_BSSID_CH_INFO	ApUuidBssid[],
	INT					VarIeLen,
	PUCHAR				pVar)
{
	INT					j = 0, Len = 0, idx = 0;
	BOOLEAN				bFound, bSameAP, bSelReg;
	PUCHAR				pData = NULL;
	PBEACON_EID_STRUCT	pEid;
	USHORT				DevicePasswordID;
	PWSC_IE				pWscIE;
	UUID_BSSID_CH_INFO	TmpInfo;
	UCHAR				zeros16[16]= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#ifdef IWSC_SUPPORT
	UINT8				RspType = 0;
	BOOLEAN				bEntryAcceptable = FALSE;
#endif // IWSC_SUPPORT //

	pData   = pVar;
	bFound  = FALSE;
	bSameAP = FALSE;
	bSelReg = FALSE;
	Len = VarIeLen;
	NdisZeroMemory(&TmpInfo, sizeof(UUID_BSSID_CH_INFO));
		while ((Len > 0) && (bFound == FALSE))
		{
			pEid = (PBEACON_EID_STRUCT) pData;
			
			/* No match, skip the Eid and move forward, IE_WFA_WSC = 0xdd */
			if (pEid->Eid != IE_WFA_WSC)
			{
				/* Set the offset and look for next IE */
				pData += (pEid->Len + 2);
				Len   -= (pEid->Len + 2);
				continue;
			}
			else
			{
				/* Found IE with 0xdd */
				/* check for WSC OUI -- 00 50 f2 04 */
			if ((NdisEqualMemory(pEid->Octet, WPS_OUI, 4) == FALSE)
#ifdef IWSC_SUPPORT
				&& (NdisEqualMemory(pEid->Octet, IWSC_OUI, 4) == FALSE)
#endif // IWSC_SUPPORT //
				)
				{
					/* Set the offset and look for next IE */
					pData += (pEid->Len + 2);
					Len   -= (pEid->Len + 2);
					continue;
				}				
			}
			
			/* 3. Found	AP with WSC IE in beacons, skip 6 bytes = 1 + 1 + 4 */
			pData += 6;
			Len   -= 6;

			/* 4. Start to look the PBC type within WSC VarIE */
			while (Len > 0)
			{
				/* Check for WSC IEs */
				pWscIE = (PWSC_IE) pData;

			if (be2cpu16(pWscIE->Type) == WSC_ID_SEL_REGISTRAR)
				{
				hex_dump("SelReg:", pData, 5);
				bSelReg = pWscIE->Data[0];
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("bSelReg = %d\n", bSelReg));
			}

#ifdef IWSC_SUPPORT
				if ((pAd->OpMode == OPMODE_STA) &&
					(pAd->StaCfg.BssType == BSS_ADHOC))
				{
					if (be2cpu16(pWscIE->Type) == WSC_ID_RESP_TYPE)
					{
						RspType = pWscIE->Data[0];
						if (RspType < WSC_MSGTYPE_REGISTRAR)
						{
							bFound = FALSE;
							break;
						}
						TmpInfo.RspType = RspType;
					}
					if (be2cpu16(pWscIE->Type) == WSC_ID_MAC_ADDR)
					{
						UCHAR mac_addr[MAC_ADDR_LEN];
						RTMPMoveMemory(mac_addr, (pData+4), MAC_ADDR_LEN);
						if (NdisCmpMemory(pInBss->MacAddr, mac_addr, MAC_ADDR_LEN))
						{
							bFound = FALSE;
							break;
						}
					}
					if (be2cpu16(pWscIE->Type) == WSC_ID_ENTRY_ACCEPTABLE)
					{
						hex_dump("EntryAcceptable:", pData, 5);
						bEntryAcceptable = pWscIE->Data[0];
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("bEntryAcceptable = %d\n", bEntryAcceptable));
					}
				}
#endif /* IWSC_SUPPORT */

				/* Check for device password ID, PBC = 0x0004, SMPBC = 0x0006 */
				if (be2cpu16(pWscIE->Type) == WSC_ID_DEVICE_PWD_ID)
				{
					/* Found device password ID */
#ifdef WINBOND
					/*The Winbond's platform will fail to retrive 2-bytes data, if use the original */
					/*be2cpu16<-- */
					DevicePasswordID = WINBON_GET16((PUCHAR)&pWscIE->Data[0]);
#else
					DevicePasswordID = be2cpu16(get_unaligned((USHORT *)&pWscIE->Data[0]));
					/*DevicePasswordID = be2cpu16(*((USHORT *) &pWscIE->Data[0])); */
#endif /* WINBOND */
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscPBCBssTableSort : DevicePasswordID = 0x%04x\n", DevicePasswordID));
					if (((pWscControl->WscMode == WSC_PBC_MODE) && (DevicePasswordID == DEV_PASS_ID_PBC)) ||
						((pWscControl->WscMode == WSC_SMPBC_MODE) && (DevicePasswordID == DEV_PASS_ID_SMPBC)))
					{
						/* Found matching PBC AP in current list, add it into table and add the count */
						bFound = TRUE;
						
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("DPID=PBC Found --> \n"));
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("#  Bssid %02x:%02x:%02x:%02x:%02x:%02x\n",
							pInBss->Bssid[0], pInBss->Bssid[1], pInBss->Bssid[2], pInBss->Bssid[3], pInBss->Bssid[4], pInBss->Bssid[5]));

						if (pInBss->Channel > 14)
							TmpInfo.Band = WSC_RFBAND_50GHZ;
						else
							TmpInfo.Band = WSC_RFBAND_24GHZ;

						RTMPMoveMemory(&TmpInfo.Bssid[0], &pInBss->Bssid[0], MAC_ADDR_LEN);
						TmpInfo.Channel = pInBss->Channel;
						RTMPZeroMemory(&TmpInfo.Ssid[0], MAX_LEN_OF_SSID);
						RTMPMoveMemory(&TmpInfo.Ssid[0], &pInBss->Ssid[0], pInBss->SsidLen);
						TmpInfo.SsidLen = pInBss->SsidLen;
					}
				}

				/* UUID_E is optional for beacons, but mandatory for probe-request */
				if (be2cpu16(pWscIE->Type) == WSC_ID_UUID_E)
				{
					/* Avoid error UUID-E storage from PIN mode */
					RTMPMoveMemory(&TmpInfo.Uuid[0], (UCHAR *)(pData+4), 16);
				}
				
				/* Set the offset and look for PBC information */
				/* Since Type and Length are both short type, we need to offset 4, not 2 */
				pData += (be2cpu16(pWscIE->Length) + 4);
				Len   -= (be2cpu16(pWscIE->Length) + 4);
			}
			
#ifdef IWSC_SUPPORT
		if ((pAd->StaCfg.BssType == BSS_ADHOC) && 
			(pWscControl->WscMode == WSC_SMPBC_MODE) &&
			(bEntryAcceptable == FALSE) && bFound)
		{
			bFound = FALSE;
		}
#endif /* IWSC_SUPPORT */
		
		if ((bFound == TRUE) && (bSelReg == TRUE))
			{							
				if (pWscControl->WscPBCBssCount == 8)
				{
					break;
				}
				
				if (pWscControl->WscPBCBssCount > 0)
				{
					for (j = 0; j < pWscControl->WscPBCBssCount; j++)
					{
						if (RTMPCompareMemory(&ApUuidBssid[j].Uuid[0], &TmpInfo.Uuid[0], 16) == 0)
						{
							if (RTMPCompareMemory(&TmpInfo.Uuid[0], zeros16, 16) != 0)
							{
								/*
									Same UUID, indicate concurrent AP
								 	We can indicate 1 AP only.
								*/
								bSameAP = TRUE;
								break;
							}
							else if (RTMPCompareMemory(&TmpInfo.Uuid[0], zeros16, 16) == 0)
							{
								if (ApUuidBssid[j].Band != TmpInfo.Band)
								{
									if (RTMPCompareMemory(&ApUuidBssid[j].Bssid[0], &TmpInfo.Bssid[0], 5) == 0)
									{
										/*
											Zero UUID at different band, and first 5bytes of two BSSIDs are the same.
											Indicate concurrent AP, we can indicate 1 AP only.
										*/
										bSameAP = TRUE;
										break;
									}
								}
							}
						}
						else if ((RTMPCompareMemory(&TmpInfo.Uuid[0], zeros16, 16) == 0) ||
								 (RTMPCompareMemory(&ApUuidBssid[j].Uuid[0], zeros16, 16) == 0))
						{
							if ((RTMPCompareMemory(&ApUuidBssid[j].Bssid[0], &TmpInfo.Bssid[0], 5) == 0) &&
								(ApUuidBssid[j].Band != TmpInfo.Band))
							{
								INT tmpDiff = (INT)(ApUuidBssid[j].Bssid[5] - TmpInfo.Bssid[5]);
								/*
									Zero UUID and Non-zero UUID at different band, and two BSSIDs are very close.
									Indicate concurrent AP, we can indicate 1 AP only.
								*/
								if ((tmpDiff <= 4) || 
									(tmpDiff >= -4))
								{
									bSameAP = TRUE;
									break;
								}
							}
						}
					}
				}
				
				if (bSameAP)
				{
					if ((pWscControl->WpsApBand == PREFERRED_WPS_AP_PHY_TYPE_2DOT4_G_FIRST) &&
						(TmpInfo.Band == WSC_RFBAND_24GHZ) &&
						(ApUuidBssid[j].Band != TmpInfo.Band))
					{
						RTMPMoveMemory(&(ApUuidBssid[j].Bssid[0]), &TmpInfo.Bssid[0], MAC_ADDR_LEN);
						RTMPZeroMemory(&(ApUuidBssid[j].Ssid[0]), MAX_LEN_OF_SSID);
						RTMPMoveMemory(&(ApUuidBssid[j].Ssid[0]), &TmpInfo.Ssid[0], TmpInfo.SsidLen);
						ApUuidBssid[j].SsidLen = TmpInfo.SsidLen;
						ApUuidBssid[j].Channel = TmpInfo.Channel;
					}
					else if ((pWscControl->WpsApBand == PREFERRED_WPS_AP_PHY_TYPE_5_G_FIRST) &&
							 (TmpInfo.Band == WSC_RFBAND_50GHZ) &&
							 (ApUuidBssid[j].Band != TmpInfo.Band))
					{
						RTMPMoveMemory(&(ApUuidBssid[j].Bssid[0]), &TmpInfo.Bssid[0], MAC_ADDR_LEN);
						RTMPZeroMemory(&(ApUuidBssid[j].Ssid[0]), MAX_LEN_OF_SSID);
						RTMPMoveMemory(&(ApUuidBssid[j].Ssid[0]), &TmpInfo.Ssid[0], TmpInfo.SsidLen);
						ApUuidBssid[j].SsidLen = TmpInfo.SsidLen;
						ApUuidBssid[j].Channel = TmpInfo.Channel;
					}
				}
				
				if (bSameAP == FALSE)
				{
					UCHAR index = pWscControl->WscPBCBssCount;
					
					/* Store UUID */
					RTMPMoveMemory(&(ApUuidBssid[index].Uuid[0]), &TmpInfo.Uuid[0], 16);
					RTMPMoveMemory(&(ApUuidBssid[index].Bssid[0]), &pInBss->Bssid[0], MAC_ADDR_LEN);
					RTMPZeroMemory(&(ApUuidBssid[index].Ssid[0]), MAX_LEN_OF_SSID);
					RTMPMoveMemory(&(ApUuidBssid[index].Ssid[0]), &pInBss->Ssid[0], pInBss->SsidLen);
					ApUuidBssid[index].SsidLen = pInBss->SsidLen;
					ApUuidBssid[index].Channel = pInBss->Channel;
					if (ApUuidBssid[index].Channel > 14)
						ApUuidBssid[index].Band = WSC_RFBAND_50GHZ;
					else
						ApUuidBssid[index].Band = WSC_RFBAND_24GHZ;
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("UUID-E= "));
					for(idx=0; idx<16; idx++)
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%02x  ", ApUuidBssid[index].Uuid[idx]));
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("\n"));

					pWscControl->WscPBCBssCount++;
				}
			}
		}

	return (bFound && bSelReg);
}

#ifdef APCLI_SUPPORT
BOOLEAN WscBssWpsIESearchForPIN(
	RTMP_ADAPTER *pAd,
	WSC_CTRL *pWscControl,	
	BSS_ENTRY *pInBss,
	UCHAR	*enrolleMacAddrType,
	UUID_BSSID_CH_INFO	*TmpInfo,
	INT					VarIeLen,
	PUCHAR				pVar)
{
	INT					Len = 0;
	BOOLEAN				bFound, bSelReg, bSelWpsPIN1;
	PUCHAR				pData = NULL;
	PBEACON_EID_STRUCT	pEid;
	USHORT				DevicePasswordID;
	PWSC_IE				pWscIE;
	UCHAR               WPS_OUI[] = {0x00, 0x50, 0xf2, 0x04};
	//UCHAR				macAddres[6];

	*enrolleMacAddrType = 0; /* use 2 for enrolle device mac address and 1 for wildcard MAC address and 0 for no mac info*/
	pData   = pVar;
	bFound  = FALSE;
	bSelReg = FALSE;
	bSelWpsPIN1 = FALSE;
	Len = VarIeLen;
	NdisZeroMemory(TmpInfo, sizeof(UUID_BSSID_CH_INFO));
		while ((Len > 0) && (bFound == FALSE))
		{
			pEid = (PBEACON_EID_STRUCT) pData;
			
			/* No match, skip the Eid and move forward, IE_WFA_WSC = 0xdd */
			if (pEid->Eid != IE_WFA_WSC)
			{
				/* Set the offset and look for next IE */
				pData += (pEid->Len + 2);
				Len   -= (pEid->Len + 2);
				continue;
			}
			else
			{
				/* Found IE with 0xdd */
				/* check for WSC OUI -- 00 50 f2 04 */
			if ((NdisEqualMemory(pEid->Octet, WPS_OUI, 4) == FALSE)
				)
				{
					/* Set the offset and look for next IE */
					pData += (pEid->Len + 2);
					Len   -= (pEid->Len + 2);
					continue;
				}				
			}
			
			/* 3. Found	AP with WSC IE in beacons, skip 6 bytes = 1 + 1 + 4 */
			pData += 6;
			Len   -= 6;

			if(pWscControl->ScanCountToincludeWPSPin1 > 3)
				bSelWpsPIN1 = TRUE;

			/* 4. Start to look the PBC type within WSC VarIE */
			while (Len > 0)
			{
				/* Check for WSC IEs */
				pWscIE = (PWSC_IE) pData;
												
				if (be2cpu16(pWscIE->Type) == WSC_ID_SEL_REGISTRAR)
				{
					hex_dump("SelReg:", pData, 5);
					bSelReg = pWscIE->Data[0];
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("bSelReg = %d\n", bSelReg));
				}


				/* Check for device password ID, PBC = 0x0004, SMPBC = 0x0006 */
				if (be2cpu16(pWscIE->Type) == WSC_ID_DEVICE_PWD_ID)
				{
					/* Found device password ID */
#ifdef WINBOND
					/*The Winbond's platform will fail to retrive 2-bytes data, if use the original */
					/*be2cpu16<-- */
					DevicePasswordID = WINBON_GET16((PUCHAR)&pWscIE->Data[0]);
#else
					DevicePasswordID = be2cpu16(get_unaligned((USHORT *)&pWscIE->Data[0]));
					/*DevicePasswordID = be2cpu16(*((USHORT *) &pWscIE->Data[0])); */
#endif /* WINBOND */
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscPINBssTableSort : DevicePasswordID = 0x%04x\n", DevicePasswordID));
					if (((pWscControl->WscMode == WSC_PIN_MODE) && (DevicePasswordID == DEV_PASS_ID_PIN)))
					{
						/* Found matching PIN AP in current list, add it into table and add the count */
						bFound = TRUE;
						
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("#  Bssid %02x:%02x:%02x:%02x:%02x:%02x\n",
							pInBss->Bssid[0], pInBss->Bssid[1], pInBss->Bssid[2], pInBss->Bssid[3], pInBss->Bssid[4], pInBss->Bssid[5]));
					}
				}

				/* for enrolle mac address or wildcard mac address in vendor extension */
				if ((be2cpu16(pWscIE->Type) == WSC_ID_VENDOR_EXT) && (bFound == TRUE) && (bSelReg == TRUE))
				{
					UCHAR wildCardMac[6];
                    UCHAR i;
					PSUB_WSC_IE		pSubWscIE;
					UCHAR *pSubData = pWscIE->Data+3;
					UCHAR length = pWscIE->Length-3;
					
					for(i = 0; i < 6; i++)
						wildCardMac[i] = 0xff;
		            
					while(length > 0)
					{
						pSubWscIE = (PSUB_WSC_IE)(pSubData);
						if(pSubWscIE->Type == 0x01)
						{
							if(RTMPCompareMemory((UCHAR *)(pSubWscIE->Data), wildCardMac, 6) == 0)
							{
								*enrolleMacAddrType = 1; 
							}	
							else
							{
								UCHAR macAddrLength = pSubWscIE->Length;
								UCHAR* macAddrData = pSubWscIE->Data;
								while (macAddrLength > 0)
								{
									if(RTMPCompareMemory(macAddrData , pWscControl->RegData.SelfInfo.MacAddr, 6) == 0)
									{
										*enrolleMacAddrType = 2;
										break;
									}
									macAddrLength = macAddrLength - 6;
									macAddrData = macAddrData + 6;
							
								}
							}
							
						}

						pSubData += ((pSubWscIE->Length) + 2);
						length   -= ((pSubWscIE->Length) + 2);
					}
					
					
                    	
				}
				else if(be2cpu16(pWscIE->Type) == WSC_ID_VENDOR_EXT)
				{
					bSelWpsPIN1 = FALSE;
				}

				/* UUID_E is optional for beacons, but mandatory for probe-request */
				if (be2cpu16(pWscIE->Type) == WSC_ID_UUID_E)
				{
					
					/* Avoid error UUID-E storage from PIN mode */
					RTMPMoveMemory(&TmpInfo->Uuid[0], (UCHAR *)(pData+4), 16);
				}
				
				/* Set the offset and look for PIN information */
				/* Since Type and Length are both short type, we need to offset 4, not 2 */
				pData += (be2cpu16(pWscIE->Length) + 4);
				Len   -= (be2cpu16(pWscIE->Length) + 4);
			}
					
		}
    
	return ((bFound && bSelReg)|| (bSelWpsPIN1));
}
#endif /* APCLI_SUPPORT */
/*
	========================================================================

	Routine Description:
		Find WSC PBC activated AP list

	Arguments:
		pAd         - NIC Adapter pointer
		OutTab		- Qualified AP BSS table
		
	Return Value:
		None
		
	IRQL = DISPATCH_LEVEL
	
	Note:
		All these constants are defined in wsc.h
		
	========================================================================
*/
VOID WscPBCBssTableSort(
	IN RTMP_ADAPTER *pAd,
	IN WSC_CTRL *pWscControl)
{
	INT i;
	BSS_ENTRY *pInBss;
	UUID_BSSID_CH_INFO	*ApUuidBssid = NULL;
	BOOLEAN rv = FALSE;
#if defined(APCLI_SUPPORT) || defined(CONFIG_STA_SUPPORT)
	UCHAR CurOpMode = AP_MODE;
#endif /* defined(APCLI_SUPPORT) || defined(CONFIG_STA_SUPPORT) */

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	{
		if (pWscControl->EntryIfIdx == BSS0)
			CurOpMode = STA_MODE;
	}
#endif // CONFIG_STA_SUPPORT //

#ifdef APCLI_SUPPORT
if (CurOpMode == AP_MODE)
	pWscControl = &pAd->ApCfg.ApCliTab[BSS0].WscControl;
#endif /* APCLI_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
if (CurOpMode == STA_MODE)
	pWscControl = &pAd->StaCfg.WscControl;
#endif /* CONFIG_STA_SUPPORT */

	if (pWscControl == NULL)
		return;

	/* allocate memory */
	os_alloc_mem(NULL, (UCHAR **)&ApUuidBssid, sizeof(UUID_BSSID_CH_INFO)*8);
	if (ApUuidBssid == NULL)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s: Allocate memory fail!!!\n", __FUNCTION__));
		return;
	}

	NdisZeroMemory(&ApUuidBssid[0], sizeof(UUID_BSSID_CH_INFO));
	pWscControl->WscPBCBssCount = 0;
	for (i = 0; i < pAd->ScanTab.BssNr; i++) 
	{
		/* BSS entry for VarIE processing */
		pInBss  = (BSS_ENTRY *) &pAd->ScanTab.BssEntry[i];

		/* 1. Check VarIE length */
		if (pInBss->VarIELen == 0)
			continue;

#ifdef CONFIG_STA_SUPPORT
		if ((CurOpMode == STA_MODE) && (pInBss->BssType != pAd->StaCfg.BssType))
			continue;
#endif /* CONFIG_STA_SUPPORT */

		/* 2. Search for WSC IE - 0xdd xx 00 50 f2 04 */
		rv = WscBssWpsIESearchForPBC(pAd, 
									pWscControl,
									pInBss,
									ApUuidBssid,
									pInBss->VarIELen,
									pInBss->VarIEs);
		if (rv == FALSE)
		{
			WscBssWpsIESearchForPBC(pAd, 
									pWscControl,
									pInBss,
									ApUuidBssid,
									pInBss->VarIeFromProbeRspLen,
									pInBss->pVarIeFromProbRsp);
		}
	}

	if (pWscControl->WscPBCBssCount == 1)
	{
		RTMPZeroMemory(&pWscControl->WscSsid, sizeof(NDIS_802_11_SSID));
		RTMPMoveMemory(pWscControl->WscSsid.Ssid, ApUuidBssid[0].Ssid, ApUuidBssid[0].SsidLen);
		pWscControl->WscSsid.SsidLength = ApUuidBssid[0].SsidLen;
		RTMPZeroMemory(pWscControl->WscBssid, MAC_ADDR_LEN);
		RTMPMoveMemory(pWscControl->WscBssid, ApUuidBssid[0].Bssid, MAC_ADDR_LEN);
#ifdef CONFIG_STA_SUPPORT
		RTMPZeroMemory(pWscControl->WscPeerMAC, MAC_ADDR_LEN);
		RTMPMoveMemory(pWscControl->WscPeerMAC, ApUuidBssid[0].MacAddr, MAC_ADDR_LEN);
		pAd->MlmeAux.Channel = ApUuidBssid[0].Channel;
#endif /* CONFIG_STA_SUPPORT */

#ifdef APCLI_SUPPORT
		if (CurOpMode == AP_MODE)
		{
			NdisZeroMemory(pAd->ApCfg.ApCliTab[BSS0].CfgSsid, MAX_LEN_OF_SSID);
			NdisMoveMemory(pAd->ApCfg.ApCliTab[BSS0].CfgSsid, ApUuidBssid[0].Ssid, ApUuidBssid[0].SsidLen);
			pAd->ApCfg.ApCliTab[BSS0].CfgSsidLen = (UCHAR)ApUuidBssid[0].SsidLen;
			pAd->ApCfg.ApCliTab[BSS0].MlmeAux.Channel = ApUuidBssid[0].Channel;
		}
#endif /* APCLI_SUPPORT */

	}

	if (ApUuidBssid != NULL)
		os_free_mem(NULL, ApUuidBssid);

#ifdef IWSC_SUPPORT
	if (pWscControl->WscMode == WSC_SMPBC_MODE)
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("WscPBCBssTableSort : Total %d SMPBC Registrar Found\n", pWscControl->WscPBCBssCount));
	else
#endif /* IWSC_SUPPORT */
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("WscPBCBssTableSort : Total %d PBC Registrar Found\n", pWscControl->WscPBCBssCount));
}

#ifdef APCLI_SUPPORT
VOID WscPINBssTableSort(
	IN RTMP_ADAPTER *pAd,
	IN WSC_CTRL *pWscControl,
	IN UUID_BSSID_CH_INFO	ApUuidBssid[])
{
	INT	i;
	BSS_ENTRY	*pInBss;
	BOOLEAN	rv = FALSE;
	UCHAR	CurOpMode = AP_MODE;
	UUID_BSSID_CH_INFO_LIST	*ApUuidBssidList = NULL;
	UUID_BSSID_CH_INFO_LIST	*ApUuidBssidListTemp = NULL;
	UUID_BSSID_CH_INFO		TmpInfo;
	UCHAR	zeros16[16]= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	UCHAR 	enrolleMacAddrType;
#ifdef APCLI_SUPPORT
if (CurOpMode == AP_MODE)
	pWscControl = &pAd->ApCfg.ApCliTab[BSS0].WscControl;
#endif /* APCLI_SUPPORT */

	if (pWscControl == NULL)
		return;
	pWscControl->ScanCountToincludeWPSPin1++;
	pWscControl->WscPINBssCount = 0;	
	for (i = 0; i < pAd->ScanTab.BssNr; i++) 
	{


		/* BSS entry for VarIE processing */
		pInBss  = (BSS_ENTRY *) &pAd->ScanTab.BssEntry[i];
		/*check for valid SSID */
		if(pInBss->SsidLen == 0)
			continue;

		/* 1. Check VarIE length */
		if (pInBss->VarIELen == 0)
			continue;


		/* 2. Search for WSC IE - 0xdd xx 00 50 f2 04 */
		rv = WscBssWpsIESearchForPIN(pAd, 
									pWscControl,
									pInBss,
									&enrolleMacAddrType,
									&TmpInfo,
									pInBss->VarIELen,
									pInBss->VarIEs);
		if (rv == FALSE)
		{
			rv = WscBssWpsIESearchForPIN(pAd, 
									pWscControl,
									pInBss,
									&enrolleMacAddrType,
									&TmpInfo,
									pInBss->VarIeFromProbeRspLen,
									pInBss->pVarIeFromProbRsp);
		}

		if(rv == TRUE)
		{		
				BOOLEAN 				bSameAP;					
				UUID_BSSID_CH_INFO		*ApUuidBssid;
				UUID_BSSID_CH_INFO_LIST	*ApUuidBssidListTemp = ApUuidBssidList;
                bSameAP = FALSE;

				if (pInBss->Channel > 14)
					TmpInfo.Band = WSC_RFBAND_50GHZ;
				else
					TmpInfo.Band = WSC_RFBAND_24GHZ;

				RTMPMoveMemory(&TmpInfo.Bssid[0], &pInBss->Bssid[0], MAC_ADDR_LEN);
				TmpInfo.Channel = pInBss->Channel;
				RTMPZeroMemory(&TmpInfo.Ssid[0], MAX_LEN_OF_SSID);
				RTMPMoveMemory(&TmpInfo.Ssid[0], &pInBss->Ssid[0], pInBss->SsidLen);
				TmpInfo.SsidLen = pInBss->SsidLen;
				
				if (pWscControl->WscPINBssCount > 0)
				{
					while(ApUuidBssidListTemp != NULL)
					{

						ApUuidBssid = &(ApUuidBssidListTemp->ApUuidBssid);
						if (RTMPCompareMemory(&ApUuidBssid->Uuid[0], &TmpInfo.Uuid[0], 16) == 0)
						{
							if (RTMPCompareMemory(&TmpInfo.Uuid[0], zeros16, 16) != 0)
							{
								/*
									Same UUID, indicate concurrent AP
								 	We can indicate 1 AP only.
								*/
								bSameAP = TRUE;
								break;
							}
							else if (RTMPCompareMemory(&TmpInfo.Uuid[0], zeros16, 16) == 0)
							{
								if (ApUuidBssid->Band != TmpInfo.Band)
								{
									if (RTMPCompareMemory(&ApUuidBssid->Bssid[0], &TmpInfo.Bssid[0], 5) == 0)
									{
										/*
											Zero UUID at different band, and first 5bytes of two BSSIDs are the same.
											Indicate concurrent AP, we can indicate 1 AP only.
										*/
										bSameAP = TRUE;
										break;
									}
								}
							}
						}
						else if ((RTMPCompareMemory(&TmpInfo.Uuid[0], zeros16, 16) == 0) ||
								 (RTMPCompareMemory(&ApUuidBssid->Uuid[0], zeros16, 16) == 0))
						{
							if ((RTMPCompareMemory(&ApUuidBssid->Bssid[0], &TmpInfo.Bssid[0], 5) == 0) &&
								(ApUuidBssid->Band != TmpInfo.Band))
							{
								INT tmpDiff = (INT)(ApUuidBssid->Bssid[5] - TmpInfo.Bssid[5]);
								/*
									Zero UUID and Non-zero UUID at different band, and two BSSIDs are very close.
									Indicate concurrent AP, we can indicate 1 AP only.
								*/
								if ((tmpDiff <= 4) || 
									(tmpDiff >= -4))
								{
									bSameAP = TRUE;
									break;
								}
							}
						}
                       ApUuidBssidListTemp = ApUuidBssidListTemp->next;
					}
				}
				
				if (bSameAP)
				{
					if ((pWscControl->WpsApBand == PREFERRED_WPS_AP_PHY_TYPE_2DOT4_G_FIRST) &&
						(TmpInfo.Band == WSC_RFBAND_24GHZ) &&
						(ApUuidBssid->Band != TmpInfo.Band))
					{
						RTMPMoveMemory(&(ApUuidBssid->Bssid[0]), &TmpInfo.Bssid[0], MAC_ADDR_LEN);
						RTMPZeroMemory(&(ApUuidBssid->Ssid[0]), MAX_LEN_OF_SSID);
						RTMPMoveMemory(&(ApUuidBssid->Ssid[0]), &TmpInfo.Ssid[0], TmpInfo.SsidLen);
						ApUuidBssid->SsidLen = TmpInfo.SsidLen;
						ApUuidBssid->Channel = TmpInfo.Channel;
					}
					else if ((pWscControl->WpsApBand == PREFERRED_WPS_AP_PHY_TYPE_5_G_FIRST) &&
							 (TmpInfo.Band == WSC_RFBAND_50GHZ) &&
							 (ApUuidBssid->Band != TmpInfo.Band))
					{
						RTMPMoveMemory(&(ApUuidBssid->Bssid[0]), &TmpInfo.Bssid[0], MAC_ADDR_LEN);
						RTMPZeroMemory(&(ApUuidBssid->Ssid[0]), MAX_LEN_OF_SSID);
						RTMPMoveMemory(&(ApUuidBssid->Ssid[0]), &TmpInfo.Ssid[0], TmpInfo.SsidLen);
						ApUuidBssid->SsidLen = TmpInfo.SsidLen;
						ApUuidBssid->Channel = TmpInfo.Channel;
					}
				}
				
				if (bSameAP == FALSE)
				{
                   	UUID_BSSID_CH_INFO_LIST *ApUuidBssidTemp = NULL;
					os_alloc_mem(NULL, (UCHAR **)&ApUuidBssidTemp, sizeof(UUID_BSSID_CH_INFO_LIST));
					NdisZeroMemory(ApUuidBssidTemp, sizeof(UUID_BSSID_CH_INFO_LIST));
					ApUuidBssid = &(ApUuidBssidTemp->ApUuidBssid);
					
					RTMPMoveMemory(&(ApUuidBssid->Uuid[0]), &TmpInfo.Uuid[0], 16);
					RTMPMoveMemory(&(ApUuidBssid->Bssid[0]), &pInBss->Bssid[0], MAC_ADDR_LEN);
					RTMPZeroMemory(&(ApUuidBssid->Ssid[0]), MAX_LEN_OF_SSID);
					RTMPMoveMemory(&(ApUuidBssid->Ssid[0]), &pInBss->Ssid[0], pInBss->SsidLen);
					ApUuidBssid->SsidLen = pInBss->SsidLen;
					ApUuidBssid->Channel = pInBss->Channel;
					if (ApUuidBssid->Channel > 14)
						ApUuidBssid->Band = WSC_RFBAND_50GHZ;
					else
						ApUuidBssid->Band = WSC_RFBAND_24GHZ;
			
					ApUuidBssidTemp->rssi = pInBss->Rssi;
					ApUuidBssidTemp->enrolleMacAddrType= enrolleMacAddrType;
					ApUuidBssidTemp->next = NULL;
					ApUuidBssidTemp->prev = NULL;
					
					if(pWscControl->WscPINBssCount == 0)
					{
						ApUuidBssidList = ApUuidBssidTemp;
					}
					else
					{
						UUID_BSSID_CH_INFO_LIST *ApUuidBssidPrev = NULL;
						ApUuidBssidListTemp = ApUuidBssidList;
						while(ApUuidBssidListTemp != NULL)
						{
							ApUuidBssidPrev = ApUuidBssidListTemp;
							if(ApUuidBssidTemp->enrolleMacAddrType < ApUuidBssidListTemp->enrolleMacAddrType)
							{
								ApUuidBssidListTemp = ApUuidBssidListTemp->next; 
							
							}
							else if((ApUuidBssidTemp->enrolleMacAddrType == ApUuidBssidListTemp->enrolleMacAddrType) && 
                                    (ApUuidBssidTemp->rssi < ApUuidBssidListTemp->rssi))
							{
								ApUuidBssidListTemp = ApUuidBssidListTemp->next;
							}
							else 
							{
								break;
							}
						}
                        
						if(ApUuidBssidListTemp == NULL)
						{
							ApUuidBssidPrev->next = ApUuidBssidTemp;
							ApUuidBssidTemp->prev = ApUuidBssidPrev;
						}
						else if(ApUuidBssidPrev->prev == NULL)
						{
							ApUuidBssidTemp->next = ApUuidBssidPrev;
							ApUuidBssidPrev->prev = ApUuidBssidTemp;
							ApUuidBssidList = ApUuidBssidTemp;
						}
						else
						{
							ApUuidBssidTemp->next = ApUuidBssidPrev;
							ApUuidBssidPrev->prev->next = ApUuidBssidTemp;
							ApUuidBssidTemp->prev = ApUuidBssidPrev->prev;
							ApUuidBssidPrev->prev = ApUuidBssidTemp;
						}
                    }
					
					pWscControl->WscPINBssCount++;
				}
			}

		
	}
	
	if(pWscControl->WscPINBssCount > MAX_NUMBER_OF_WPS_PIN_AP)
		pWscControl->WscPINBssCount = MAX_NUMBER_OF_WPS_PIN_AP;
	
	for( i = pWscControl->WscPINBssCount -1; i >= 0; i--)
	{
		RTMPMoveMemory(	&(pWscControl->ApUuidBssid[i]), &(ApUuidBssidList->ApUuidBssid), sizeof(UUID_BSSID_CH_INFO));
		ApUuidBssidListTemp = ApUuidBssidList;
		ApUuidBssidList = ApUuidBssidList->next;
		
		os_free_mem(NULL, ApUuidBssidListTemp);
		if(ApUuidBssidList == NULL)
			break;
	}
	if(ApUuidBssidList != NULL)
		os_free_mem(NULL, ApUuidBssidList);
	
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_OFF, ("WscPINBssTableSort : Total %d PIN Registrar Found\n", pWscControl->WscPINBssCount));
}
#endif /*APCLI_SUPPORT */

VOID	WscGenRandomKey(
	IN  	PRTMP_ADAPTER	pAd,
	IN  	PWSC_CTRL       pWscControl,
	INOUT	PUCHAR			pKey,
	INOUT	PUSHORT			pKeyLen)
{
	UCHAR   tempRandomByte = 0;
	UCHAR   idx = 0;
	UCHAR   keylen = 0;	 
	UCHAR   retry = 0;
	
	NdisZeroMemory(pKey, 64);

	/*
		Hex Key 64 digital
	*/
	if(pWscControl->WscKeyASCII == 0)
	{ 		
		UCHAR	tmpStrB[3];
		for (idx = 0; idx < 32; idx++)
		{
			NdisZeroMemory(&tmpStrB[0], sizeof(tmpStrB));
			tempRandomByte = RandomByte(pAd);
			snprintf((RTMP_STRING *) &tmpStrB[0], 3, "%02x", tempRandomByte);
			NdisMoveMemory(pKey+(idx*2), &tmpStrB[0], 2);
		}
		*pKeyLen = 64;
	}
	else
	{
		/*
			ASCII Key, random length 
		*/
		if(pWscControl->WscKeyASCII == 1)
		{
			do{
				keylen = RandomByte(pAd);
				keylen = keylen % 64;
				if(retry++ > 20)
					keylen = 8;
			}while(keylen < 8);
		}
		else
			keylen = pWscControl->WscKeyASCII;

		/*
			Generate printable ASCII (decimal 33 to 126) 
		*/
		for(idx = 0; idx < keylen; idx++) 
		{
			tempRandomByte = RandomByte(pAd)%94+33;
			*(pKey+idx) = tempRandomByte;
		}
		*pKeyLen = keylen;
	}
}

VOID	WscCreateProfileFromCfg(
	IN	PRTMP_ADAPTER		pAd,
	IN  UCHAR               OpMode,
	IN  PWSC_CTRL           pWscControl,
	OUT PWSC_PROFILE        pWscProfile)
{
    UCHAR	        apidx = (pWscControl->EntryIfIdx & 0x0F);
    USHORT          authType = 0, encyType = 0;
    UCHAR           WepKeyId = 0;
    PWSC_CREDENTIAL pCredential = NULL;
	UCHAR			CurOpMode = AP_MODE;

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	{
		if (pWscControl->EntryIfIdx == BSS0)
			CurOpMode = STA_MODE;
	}
#endif // CONFIG_STA_SUPPORT //

#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
	{
		if ((OpMode & 0x0F) == AP_MODE)
		{
			/*
				AP needs to choose the STA's authType and encyType in two cases. 		 
				1. AP is unconfigurated (authType and encyType will be updated to mixed mode by WscWriteConfToPortCfg() )		 
				2. AP's authType is mixed mode, we should choose the suitable authType and encyType to STA		 
				STA's authType and encyType depend on WscSecurityMode flag 
			*/		
			
			if (((pWscControl->WscConfStatus == WSC_SCSTATE_UNCONFIGURED ) || 
				 (pAd->ApCfg.MBSSID[apidx].wdev.AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK)) &&
				 (OpMode & REGISTRAR_ACTION))
			{			
				switch (pAd->ApCfg.MBSSID[apidx].WscSecurityMode)			
				{				
					case WPAPSKTKIP:					
						authType = WSC_AUTHTYPE_WPAPSK;					
						encyType = WSC_ENCRTYPE_TKIP;				
						break;				
					case WPAPSKAES:					
						authType = WSC_AUTHTYPE_WPAPSK;					
						encyType = WSC_ENCRTYPE_AES;				
						break;				
					case WPA2PSKTKIP:					
						authType = WSC_AUTHTYPE_WPA2PSK;					
						encyType = WSC_ENCRTYPE_TKIP;				
						break;				
					case WPA2PSKAES:					
						authType = WSC_AUTHTYPE_WPA2PSK;					
						encyType = WSC_ENCRTYPE_AES;				
						break;				
					default:					
#if 0
						authType = WSC_AUTHTYPE_WPA2PSK;					
						encyType = WSC_ENCRTYPE_AES;				
#else
						authType = (WSC_AUTHTYPE_WPAPSK | WSC_AUTHTYPE_WPA2PSK);
						encyType = (WSC_ENCRTYPE_TKIP | WSC_ENCRTYPE_AES);
#endif
						break;			
				}		

				if (pWscControl->WscConfStatus == WSC_SCSTATE_CONFIGURED)
				{
					/*
						Although AuthMode is mixed mode, cipher maybe not mixed mode.
						We need to correct cipher here.
					*/
					if (pAd->ApCfg.MBSSID[apidx].wdev.WepStatus == Ndis802_11TKIPEnable)
						encyType = WSC_ENCRTYPE_TKIP;
					if (pAd->ApCfg.MBSSID[apidx].wdev.WepStatus == Ndis802_11AESEnable)
						encyType = WSC_ENCRTYPE_AES;
				}
			}
			else
			{
				authType = WscGetAuthType(pAd->ApCfg.MBSSID[apidx].wdev.AuthMode);
				encyType = WscGetEncryType(pAd->ApCfg.MBSSID[apidx].wdev.WepStatus);
			}
			WepKeyId = pAd->ApCfg.MBSSID[apidx].wdev.DefaultKeyId;
		}
#ifdef APCLI_SUPPORT    
		else if (OpMode == AP_CLIENT_MODE)
		{
			apidx = apidx & 0x0F;
			authType = WscGetAuthType(pAd->ApCfg.ApCliTab[apidx].wdev.AuthMode);
			encyType = WscGetEncryType(pAd->ApCfg.ApCliTab[apidx].wdev.WepStatus);
			WepKeyId = pAd->ApCfg.ApCliTab[apidx].wdev.DefaultKeyId;
		}
#endif /* APCLI_SUPPORT */
	}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
	{
		authType = WscGetAuthType(pAd->StaCfg.wdev.AuthMode);
		encyType = WscGetEncryType(pAd->StaCfg.wdev.WepStatus);
		WepKeyId = pAd->StaCfg.wdev.DefaultKeyId;
	}
#endif /* CONFIG_STA_SUPPORT */    

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscGetDefaultProfileForM8\n"));

	pCredential = &pWscProfile->Profile[0]; /*Only support one credential now. 20070515 */
	NdisZeroMemory(pCredential, sizeof(WSC_CREDENTIAL));
	pWscProfile->ProfileCnt = 1;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:: pWscControl->WscConfStatus  = %d, OpMode = %d\n",
			__FUNCTION__, pWscControl->WscConfStatus, OpMode));

	/* NewKey, NewKeyIndex for M8 */
	if ((WSC_SCSTATE_UNCONFIGURED == pWscControl->WscConfStatus) &&
		((((OpMode & 0x0F) == AP_MODE)
#ifdef CONFIG_STA_SUPPORT
		  || (((OpMode & 0x0F) == STA_MODE) && (pAd->StaCfg.BssType == BSS_ADHOC))
#endif /* CONFIG_STA_SUPPORT */
		  ) && (OpMode & REGISTRAR_ACTION)))
	{
		pCredential->KeyIndex = 1;
		if ((OpMode & 0x0F) == STA_MODE)
		{
#ifdef IWSC_TEST_SUPPORT
			if (pAd->StaCfg.IWscInfo.IWscDefaultSecurity == 1)
			{
				authType = WSC_AUTHTYPE_OPEN;
				encyType = WSC_ENCRTYPE_NONE;
				pCredential->KeyLength = 0;
				NdisZeroMemory(pCredential->Key, 64);
			}
			else if (pAd->StaCfg.IWscInfo.IWscDefaultSecurity == 2)
			{
				UCHAR	idx;
				CHAR	tempRandomByte;
				authType = WSC_AUTHTYPE_OPEN;
				encyType = WSC_ENCRTYPE_WEP;
				for(idx = 0; idx < 13; idx++)
				{
					tempRandomByte = RandomByte(pAd)%94+33;
					sprintf((RTMP_STRING *) pCredential->Key+idx, "%c", tempRandomByte);					
				}
				pCredential->KeyLength = 13;
			}
			else
#endif // IWSC_TEST_SUPPORT //
			{
				WscGenRandomKey(pAd, pWscControl, pCredential->Key, &pCredential->KeyLength);
				authType = WSC_AUTHTYPE_WPA2PSK;
				encyType = WSC_ENCRTYPE_AES;
			}
		}
		else
			WscGenRandomKey(pAd, pWscControl, pCredential->Key, &pCredential->KeyLength);
	}
    else
	{
		pCredential->KeyIndex = 1;
		pCredential->KeyLength = 0;
		NdisZeroMemory(pCredential->Key, 64);
		switch (encyType)
		{
			case WSC_ENCRTYPE_NONE:
				break;
			case WSC_ENCRTYPE_WEP:
				pCredential->KeyIndex = (WepKeyId + 1);
				if (((OpMode & 0x0F) == AP_MODE || (OpMode & 0x0F) == STA_MODE) && pAd->SharedKey[apidx][WepKeyId].KeyLen)
				{
					INT i;
					for (i=0; i<pAd->SharedKey[apidx][WepKeyId].KeyLen; i++)
					{
						snprintf((RTMP_STRING *) pCredential->Key, 64, "%s%02x", pCredential->Key, pAd->SharedKey[apidx][WepKeyId].Key[i]);
					}
					pCredential->KeyLength = pAd->SharedKey[apidx][WepKeyId].KeyLen*2;
				}
#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
				else if ((OpMode == AP_CLIENT_MODE) && (pAd->ApCfg.ApCliTab[apidx].SharedKey[WepKeyId].KeyLen) && 
												(CurOpMode == AP_MODE))
				{
					INT i;
					for (i=0; i<pAd->ApCfg.ApCliTab[apidx].SharedKey[WepKeyId].KeyLen; i++)
					{
						snprintf((RTMP_STRING *) pCredential->Key, 64, "%s%02x", pCredential->Key, pAd->ApCfg.ApCliTab[apidx].SharedKey[WepKeyId].Key[i]);
					}
					pCredential->KeyLength = pAd->SharedKey[apidx][WepKeyId].KeyLen*2;
				}
#endif /* APCLI_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
				break;
			case WSC_ENCRTYPE_TKIP:
			case WSC_ENCRTYPE_AES:
			case (WSC_ENCRTYPE_AES | WSC_ENCRTYPE_TKIP):
				pCredential->KeyLength = pWscControl->WpaPskLen;
				memcpy(pCredential->Key, 
				pWscControl->WpaPsk, 
				pWscControl->WpaPskLen);
				break;
		}
	}

	pCredential->AuthType = authType;
	pCredential->EncrType = encyType;

#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
	{
		if ((OpMode & 0x0F) == AP_MODE)
		{
			NdisMoveMemory(pCredential->MacAddr, pAd->ApCfg.MBSSID[apidx].wdev.bssid, 6);
			if ((pWscControl->WscConfStatus == WSC_SCSTATE_UNCONFIGURED) &&
				(pWscControl->WscDefaultSsid.SsidLength > 0) &&
				(pWscControl->WscDefaultSsid.SsidLength < 33))
			{
				NdisMoveMemory(pCredential->SSID.Ssid, pWscControl->WscDefaultSsid.Ssid, pWscControl->WscDefaultSsid.SsidLength);
				pCredential->SSID.SsidLength = pWscControl->WscDefaultSsid.SsidLength;
			}
			else
			{
				NdisMoveMemory(pCredential->SSID.Ssid, pAd->ApCfg.MBSSID[apidx].Ssid, pAd->ApCfg.MBSSID[apidx].SsidLen);
				pCredential->SSID.SsidLength = pAd->ApCfg.MBSSID[apidx].SsidLen;
			}
		}
#ifdef APCLI_SUPPORT    
		else if (OpMode == AP_CLIENT_MODE)
		{
			NdisMoveMemory(pCredential->MacAddr, APCLI_ROOT_BSSID_GET(pAd, pAd->ApCfg.ApCliTab[apidx].MacTabWCID), 6);
			NdisMoveMemory(pCredential->SSID.Ssid, pAd->ApCfg.ApCliTab[apidx].Ssid, pAd->ApCfg.ApCliTab[apidx].SsidLen);
			pCredential->SSID.SsidLength = pAd->ApCfg.ApCliTab[apidx].SsidLen;
		}
#endif /* APCLI_SUPPORT */
	}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
	{
		if (pAd->StaCfg.BssType == BSS_INFRA)
			NdisMoveMemory(pCredential->MacAddr, pAd->CommonCfg.Bssid, 6);
		else
			NdisMoveMemory(pCredential->MacAddr, pAd->CurrentAddress, 6);
		NdisMoveMemory(pCredential->SSID.Ssid, pAd->CommonCfg.Ssid, pAd->CommonCfg.SsidLen);
		pCredential->SSID.SsidLength = pAd->CommonCfg.SsidLen;
	}
#endif /* CONFIG_STA_SUPPORT */
    
#ifdef WSC_V2_SUPPORT
	if (pWscControl->WscV2Info.bEnableWpsV2 && (OpMode & REGISTRAR_ACTION))
		NdisMoveMemory(pCredential->MacAddr, pWscControl->EntryAddr, 6);
#endif /* WSC_V2_SUPPORT */
    
    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscCreateProfileFromCfg\n"));

}

#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
void    WscWriteConfToApCliCfg(
    IN  PRTMP_ADAPTER   pAd,
    IN  PWSC_CTRL       pWscControl,
    IN  PWSC_CREDENTIAL pCredential,
    IN  BOOLEAN         bEnrollee)
{
	UCHAR			CurApIdx = BSS0;
	APCLI_STRUCT	*pApCliTab;
	
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscWriteConfToApCliCfg\n"));
    
	CurApIdx = (pWscControl->EntryIfIdx & 0x0F);
	{
		pApCliTab = &pAd->ApCfg.ApCliTab[CurApIdx];
		
		NdisZeroMemory(pApCliTab->Ssid, MAX_LEN_OF_SSID);
		NdisMoveMemory(pApCliTab->Ssid, pCredential->SSID.Ssid, pCredential->SSID.SsidLength);
		pApCliTab->SsidLen = pCredential->SSID.SsidLength;

		NdisZeroMemory(pApCliTab->CfgSsid, MAX_LEN_OF_SSID);
		NdisMoveMemory(pApCliTab->CfgSsid, pCredential->SSID.Ssid, pCredential->SSID.SsidLength);
		pApCliTab->CfgSsidLen = pCredential->SSID.SsidLength;

		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("AuthType: %d, EncrType: %d\n", pCredential->AuthType, pCredential->EncrType));
		if ((pCredential->AuthType == WSC_AUTHTYPE_WPAPSK) || 
			(pCredential->AuthType == WSC_AUTHTYPE_WPA2PSK))
		{
			if ((pCredential->EncrType != WSC_ENCRTYPE_TKIP) && (pCredential->EncrType != WSC_ENCRTYPE_AES))
			{
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("AuthType is WPAPSK or WPA2PAK.\n"
			                         "Get illegal EncrType(%d) from External Registrar, set EncrType to TKIP\n", 
			                          pCredential->EncrType));
			pCredential->EncrType = WSC_ENCRTYPE_TKIP;
			}
		}
	        Set_ApCli_AuthMode_Proc(pAd, WscGetAuthTypeStr(pCredential->AuthType));
       	 Set_ApCli_EncrypType_Proc(pAd, WscGetEncryTypeStr(pCredential->EncrType));
		 
		if (pCredential->EncrType != WSC_ENCRTYPE_NONE)
		{
				if (pCredential->EncrType & (WSC_ENCRTYPE_TKIP | WSC_ENCRTYPE_AES))
	            {
	                pApCliTab->wdev.DefaultKeyId = 0;

	                if (pCredential->KeyLength >= 8 && pCredential->KeyLength <= 64)
	                {
	                    pWscControl->WpaPskLen = (INT) pCredential->KeyLength;
							NdisZeroMemory(pWscControl->WpaPsk, 64);
							NdisMoveMemory(pWscControl->WpaPsk, pCredential->Key, pWscControl->WpaPskLen);
							RT_CfgSetWPAPSKKey(pAd, (RTMP_STRING *) pCredential->Key, pWscControl->WpaPskLen, 
										(PUCHAR)pApCliTab->Ssid, pApCliTab->SsidLen, 
										pApCliTab->PMK);
	                    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WpaPskLen = %d\n", pWscControl->WpaPskLen));
	                }
	                else
	                {
	                    pWscControl->WpaPskLen = 0;
	                    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WPAPSK: Invalid Key Length (%d)\n", pCredential->KeyLength));
	                }
	            }
	            else if (pCredential->EncrType == WSC_ENCRTYPE_WEP)
	            {
				CHAR   WepKeyId = 0;
				USHORT  WepKeyLen = pCredential->KeyLength;
				
				WepKeyId = (pCredential->KeyIndex - 1); /* KeyIndex = 1 ~ 4 */
				if ((WepKeyId >= 0) && (WepKeyId <=3))
				{
					pApCliTab->wdev.DefaultKeyId = WepKeyId;

					/* 5 or 13 ASCII characters */
					/* 10 or 26 Hex characters */
					if (WepKeyLen == 5 || WepKeyLen == 13 || WepKeyLen == 10 || WepKeyLen == 26)
					{
						if (WepKeyLen == 5 || WepKeyLen == 13)
						{
							pApCliTab->SharedKey[WepKeyId].KeyLen = WepKeyLen;
							memcpy(pApCliTab->SharedKey[WepKeyId].Key, 
									pCredential->Key,WepKeyLen);
							if (WepKeyLen == 5)
								pApCliTab->SharedKey[WepKeyId].CipherAlg = CIPHER_WEP64;
							else
								pApCliTab->SharedKey[WepKeyId].CipherAlg = CIPHER_WEP128;
							}
							else
							{
								pApCliTab->SharedKey[WepKeyId].KeyLen = (UCHAR) WepKeyLen/2;
								AtoH((RTMP_STRING *) pCredential->Key, pApCliTab->SharedKey[WepKeyId].Key, WepKeyLen/2);
								if (WepKeyLen == 10)
									pApCliTab->SharedKey[WepKeyId].CipherAlg = CIPHER_WEP64;
								else
									pApCliTab->SharedKey[WepKeyId].CipherAlg = CIPHER_WEP128;
							}
					}
					else
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WEP: Invalid Key Length (%d)\n", pCredential->KeyLength));
				}
				else
				{
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("Unsupport default key index (%d), use key Index 1.\n", WepKeyId));
					pApCliTab->wdev.DefaultKeyId = WepKeyId = 0;
				}
			}
		}
	}

	if (pWscControl->WscProfile.ProfileCnt > 1)
	{
		pWscControl->WscProfileRetryTimerRunning = TRUE;
		RTMPSetTimer(&pWscControl->WscProfileRetryTimer, WSC_PROFILE_RETRY_TIME_OUT);
	}

#ifdef P2P_SUPPORT
	if (P2P_CLI_ON(pAd))
	{
		NdisZeroMemory(pAd->P2pCfg.SSID, MAX_LEN_OF_SSID);
		pAd->P2pCfg.SSIDLen = pCredential->SSID.SsidLength;
		NdisMoveMemory(pAd->P2pCfg.SSID, pCredential->SSID.Ssid, pAd->P2pCfg.SSIDLen);
	}
#endif /* P2P_SUPPORT */

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscWriteConfToApCliCfg\n"));
}

VOID 	WscApCliLinkDown(
	IN	PRTMP_ADAPTER	pAd,
	IN  PWSC_CTRL       pWscControl)
{
	UCHAR	apidx = (pWscControl->EntryIfIdx & 0x0F);
	UCHAR	mac_addr[MAC_ADDR_LEN];
    BOOLEAN apcliEn = pAd->ApCfg.ApCliTab[apidx].Enable;	

	NdisMoveMemory(pWscControl->RegData.SelfInfo.MacAddr,
                   pAd->ApCfg.ApCliTab[apidx].wdev.if_addr, 
                   6);
    
    /* bring apcli interface down first */
	if(apcliEn == TRUE )
	{
		pAd->ApCfg.ApCliTab[apidx].Enable = FALSE;
		ApCliIfDown(pAd);
	}
    pAd->ApCfg.ApCliTab[apidx].Enable = apcliEn;
	memcpy(mac_addr, pAd->ApCfg.ApCliTab[apidx].wdev.if_addr, MAC_ADDR_LEN);
	pWscControl->WscStatus = STATUS_WSC_LINK_UP;
	pWscControl->bWscTrigger = TRUE;
}
#endif /* APCLI_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */

VOID   WpsSmProcess(
    IN PRTMP_ADAPTER        pAd,
    IN MLME_QUEUE_ELEM 	   *Elem)
{
    int                 HeaderLen = LENGTH_802_11 + LENGTH_802_1_H + sizeof(IEEE8021X_FRAME) + sizeof(EAP_FRAME);
    PHEADER_802_11      pHeader;
	PMAC_TABLE_ENTRY    pEntry = NULL;
	int                 apidx = MAIN_MBSSID;
	PWSC_CTRL			pWpsCtrl = NULL;
	UCHAR				CurOpMode = 0xFF;

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		CurOpMode = AP_MODE;
	}
#endif // CONFIG_AP_SUPPORT //

#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	{
		CurOpMode = STA_MODE;
	}
#ifdef P2P_SUPPORT
	if (Elem->OpMode == OPMODE_AP)
		CurOpMode = AP_MODE;
#endif /* P2P_SUPPORT */
#endif // CONFIG_STA_SUPPORT //

	if (CurOpMode == 0xFF)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_WARN, ("Unkown OpMode (CurOpMode=0x%02x)\n", CurOpMode));
		return;
	}
	else
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("CurOpMode=0x%02x\n", CurOpMode));
	
    pHeader = (PHEADER_802_11)Elem->Msg;
    
    if (Elem->MsgType == WSC_EAPOL_PACKET_MSG)
    {
		if ((pEntry = MacTableLookup(pAd, pHeader->Addr2)))
            apidx = pEntry->func_tb_idx;
    }
	
#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
	{
#ifdef APCLI_SUPPORT
		if (pEntry && IS_ENTRY_APCLI(pEntry))
			pWpsCtrl = &pAd->ApCfg.ApCliTab[apidx].WscControl;
		else
#endif /* APCLI_SUPPORT */
			pWpsCtrl = &pAd->ApCfg.MBSSID[apidx].WscControl;
	}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
		pWpsCtrl = &pAd->StaCfg.WscControl;
#endif /* CONFIG_STA_SUPPORT */
	
	if((Elem->MsgType == WSC_EAPOL_UPNP_MSG) && (Elem->MsgLen > HeaderLen))
	{	/*The WSC msg from UPnP daemon */
		PUCHAR		pData;
		UCHAR 		MacAddr[MAC_ADDR_LEN]= {0};
		
#ifdef RELEASE_EXCLUDE
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Receive a UPnP WSC Message!\n"));
#endif /* RELEASE_EXCLUDE */		
        /* Skip the (802.11 + 802.1h + 802.1x + EAP) header */
    	pData = (PUCHAR) &Elem->Msg[HeaderLen];
        Elem->MsgLen -= HeaderLen;
		/* The Addr1 of UPnP-Msg used to indicate the MAC address of the AP interface. Now always be ra0. */
		NdisMoveMemory(MacAddr, pHeader->Addr1, MAC_ADDR_LEN);
		NdisMoveMemory(Elem->Msg, MacAddr, MAC_ADDR_LEN);
		NdisMoveMemory(Elem->Msg+6, pData, Elem->MsgLen);
		
		StateMachinePerformAction(pAd, &pAd->Mlme.WscMachine, Elem, pAd->Mlme.WscMachine.CurrState);
	}
	else if (Elem->MsgType == WSC_EAPOL_START_MSG)
		StateMachinePerformAction(pAd, &pAd->Mlme.WscMachine, Elem, pAd->Mlme.WscMachine.CurrState);
	else if (pEntry && (Elem->MsgType == WSC_EAPOL_PACKET_MSG))
    {   /* WSC_STATE_MACHINE can service only one station at one time */
        RTMP_STRING *pData;
        PEAP_FRAME  pEapFrame;
        /* Skip the EAP LLC header */
    	pData = (RTMP_STRING *) &Elem->Msg[LENGTH_802_11 + LENGTH_802_1_H];
        pEapFrame = (PEAP_FRAME)(pData + sizeof(IEEE8021X_FRAME));
    	pData += sizeof(IEEE8021X_FRAME) + sizeof(EAP_FRAME);

		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s::  EAPOL Packet.  Code = %d.    Type = %d\n",
							__FUNCTION__, pEapFrame->Code, pEapFrame->Type));
		if (pEapFrame->Code == EAP_CODE_FAIL)
        { /* EAP-Fail */
			RTMP_STRING fail_data[] = "EAP_FAIL";
			NdisMoveMemory(Elem->Msg, pHeader->Addr2, MAC_ADDR_LEN);
            NdisMoveMemory(Elem->Msg+MAC_ADDR_LEN, fail_data, strlen(fail_data));
            Elem->MsgLen = strlen(fail_data);
			StateMachinePerformAction(pAd, &pAd->Mlme.WscMachine, Elem, pAd->Mlme.WscMachine.CurrState);
			return;
        }
        else if ((pEapFrame->Code == EAP_CODE_REQ) && (pEapFrame->Type == EAP_TYPE_ID))
        { /* EAP-Req (Identity) */
		RTMP_STRING id_data[] = "hello";

			pWpsCtrl->lastId = pEapFrame->Id;
			NdisMoveMemory(Elem->Msg, pHeader->Addr2, MAC_ADDR_LEN);
            NdisMoveMemory(Elem->Msg+MAC_ADDR_LEN, id_data, strlen(id_data));
            Elem->MsgLen = strlen(id_data);
			StateMachinePerformAction(pAd, &pAd->Mlme.WscMachine, Elem, pAd->Mlme.WscMachine.CurrState);
			return;
        }
        else if ((pEapFrame->Code == EAP_CODE_REQ) && (pEapFrame->Type == EAP_TYPE_WSC))
        { /* EAP-Req (Messages) */
            if (Elem->MsgLen <= HeaderLen)
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Elem->MsgLen(%ld) <= HeaderLen(%d) !!\n", Elem->MsgLen, HeaderLen));
                return;
            }

			pWpsCtrl->lastId = pEapFrame->Id;
            Elem->MsgLen -= (LENGTH_802_11 + LENGTH_802_1_H + sizeof(IEEE8021X_FRAME) + sizeof(EAP_FRAME));
            if (WscCheckWSCHeader((PUCHAR)pData))
            {
                PWSC_FRAME			pWsc = (PWSC_FRAME) pData;

				NdisMoveMemory(Elem->Msg, pHeader->Addr2, MAC_ADDR_LEN);
 				if (pWsc->OpCode == WSC_OPCODE_FRAG_ACK)
				{
					/*
						Send rest WSC frag data
					*/
					RTMP_STRING wsc_frag_ack[] = "WSC_FRAG_ACK";

                    NdisMoveMemory(Elem->Msg+MAC_ADDR_LEN, wsc_frag_ack, strlen(wsc_frag_ack));
                    Elem->MsgLen = strlen(wsc_frag_ack);
					StateMachinePerformAction(pAd, &pAd->Mlme.WscMachine, Elem, pAd->Mlme.WscMachine.CurrState);
				}
                else if (pWsc->OpCode == WSC_OPCODE_START)
                {
                    RTMP_STRING wsc_start[] = "WSC_START";

                    NdisMoveMemory(Elem->Msg+MAC_ADDR_LEN, wsc_start, strlen(wsc_start));
                    Elem->MsgLen = strlen(wsc_start);
 					StateMachinePerformAction(pAd, &pAd->Mlme.WscMachine, Elem, pAd->Mlme.WscMachine.CurrState);
                }
                else
                {
                	if (pWsc->Flags & WSC_MSG_FLAG_LF)
                	{
						pData += (sizeof(WSC_FRAME) + 2);
						Elem->MsgLen -= (sizeof(WSC_FRAME) + 2);
                	}
					else
					{
                    	pData += sizeof(WSC_FRAME);
                    	Elem->MsgLen -= sizeof(WSC_FRAME);
					}

					if ((pWpsCtrl->WscRxBufLen + Elem->MsgLen) < (MGMT_DMA_BUFFER_SIZE-6))
					{
						NdisMoveMemory((pWpsCtrl->pWscRxBuf + pWpsCtrl->WscRxBufLen), pData, Elem->MsgLen);
						pWpsCtrl->WscRxBufLen += Elem->MsgLen;
					}
#ifdef WSC_V2_SUPPORT
					if (pWsc->Flags & WSC_MSG_FLAG_MF)
						WscSendEapFragAck(pAd, pWpsCtrl, pEntry);
					else
#endif /* WSC_V2_SUPPORT */
					{
                    	NdisMoveMemory(Elem->Msg+MAC_ADDR_LEN, pWpsCtrl->pWscRxBuf, pWpsCtrl->WscRxBufLen);
						Elem->MsgLen = pWpsCtrl->WscRxBufLen;
						StateMachinePerformAction(pAd, &pAd->Mlme.WscMachine, Elem, pAd->Mlme.WscMachine.CurrState);
						pWpsCtrl->WscRxBufLen = 0;
						NdisZeroMemory(pWpsCtrl->pWscRxBuf, MGMT_DMA_BUFFER_SIZE);
					}
                }
				return;
            }
            else
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("ERROR: WscCheckWSCHeader() return FALSE!\n"));
                return;
            }
        }
		
        if (Elem->MsgLen <= HeaderLen)
        {
            MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Elem->MsgLen(%ld) <= HeaderLen(%d) !!\n", Elem->MsgLen, HeaderLen));
            return;
        }
        
        Elem->MsgLen -= (LENGTH_802_11 + LENGTH_802_1_H + sizeof(IEEE8021X_FRAME) + sizeof(EAP_FRAME));
        NdisMoveMemory(Elem->Msg, pHeader->Addr2, MAC_ADDR_LEN);
        if (IS_ENTRY_CLIENT(pEntry) &&
            (pEapFrame->Code == EAP_CODE_RSP) && 
            (pEapFrame->Type == EAP_TYPE_ID))
        { 
            if (strstr(pData, "SimpleConfig"))
            {
            	/* EAP-Rsp (Identity) */
            	NdisMoveMemory(Elem->Msg+6, pData, Elem->MsgLen);
				StateMachinePerformAction(pAd, &pAd->Mlme.WscMachine, Elem, pAd->Mlme.WscMachine.CurrState);
				return;
            }
            else
            {
                BOOLEAN Cancelled;
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("RTMPCancelTimer EapolTimer!!\n"));
				NdisZeroMemory(pWpsCtrl->EntryAddr, MAC_ADDR_LEN);
                pWpsCtrl->EapolTimerRunning = FALSE;
                RTMPCancelTimer(&pWpsCtrl->EapolTimer, &Cancelled);
                return;
            }
        }
        else
        {
            if (WscCheckWSCHeader((PUCHAR) pData))
            {
				/* EAP-Rsp (Messages) */
				PWSC_FRAME			pWsc = (PWSC_FRAME) pData;
				if (pWsc->OpCode == WSC_OPCODE_FRAG_ACK)
				{
					/*
						Send rest frag data
					*/
					RTMP_STRING wsc_frag_ack[] = "WSC_FRAG_ACK";
                    NdisMoveMemory(Elem->Msg+MAC_ADDR_LEN, wsc_frag_ack, strlen(wsc_frag_ack));
                    Elem->MsgLen = strlen(wsc_frag_ack);
					StateMachinePerformAction(pAd, &pAd->Mlme.WscMachine, Elem, pAd->Mlme.WscMachine.CurrState);
				}
				else
				{
					if (pWsc->Flags & WSC_MSG_FLAG_LF)
					{
						pData += (sizeof(WSC_FRAME) + 2);
						Elem->MsgLen -= (sizeof(WSC_FRAME) + 2);
					}
					else
					{
						pData += sizeof(WSC_FRAME);
						Elem->MsgLen -= sizeof(WSC_FRAME);
					}

					if ((pWpsCtrl->WscRxBufLen + Elem->MsgLen) < (MGMT_DMA_BUFFER_SIZE-6))
					{
						NdisMoveMemory((pWpsCtrl->pWscRxBuf + pWpsCtrl->WscRxBufLen), pData, Elem->MsgLen);
						pWpsCtrl->WscRxBufLen += Elem->MsgLen;
					}
#ifdef WSC_V2_SUPPORT
					if (pWsc->Flags & WSC_MSG_FLAG_MF)
						WscSendEapFragAck(pAd, pWpsCtrl, pEntry);
					else
#endif /* WSC_V2_SUPPORT */
					{
						//NdisMoveMemory(Elem->Msg+6, pData, Elem->MsgLen);
						NdisMoveMemory(Elem->Msg+6, pWpsCtrl->pWscRxBuf, pWpsCtrl->WscRxBufLen);
						Elem->MsgLen = pWpsCtrl->WscRxBufLen;
						StateMachinePerformAction(pAd, &pAd->Mlme.WscMachine, Elem, pAd->Mlme.WscMachine.CurrState);
						pWpsCtrl->WscRxBufLen = 0;
						NdisZeroMemory(pWpsCtrl->pWscRxBuf, MGMT_DMA_BUFFER_SIZE);
					}
				}
				return;
            }
            else
            {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("ERROR: WscCheckWSCHeader() return FALSE!\n"));
                return;
            }
        }
    }
    else
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_WARN, ("Unknow Message Type (=%lu)\n", Elem->MsgType));
}

#ifdef CONFIG_AP_SUPPORT

#define WSC_SINGLE_TRIGGER_APPNAME  "unknown"

#ifdef SDK_GOAHEAD_HTTPD
#undef WSC_SINGLE_TRIGGER_APPNAME
#define WSC_SINGLE_TRIGGER_APPNAME  "goahead"
#endif /* SDK_GOAHEAD_HTTPD */

#ifdef SDK_USER_LIGHTY
#undef WSC_SINGLE_TRIGGER_APPNAME
#define WSC_SINGLE_TRIGGER_APPNAME  "nvram_daemon"
#endif /* SDK_USER_LIGHTY */

INT	WscGetConfWithoutTrigger(
	IN	PRTMP_ADAPTER	pAd,
	IN  PWSC_CTRL       pWscControl,
	IN  BOOLEAN         bFromUPnP)
{
	INT                 WscMode;
	INT                 IsAPConfigured;
	//PWSC_UPNP_NODE_INFO pWscUPnPNodeInfo;
	UCHAR		apIdx;

#ifdef LINUX
#ifdef WSC_SINGLE_TRIGGER
/* +++  added by YYHuang@Ralink, 08/03/12 */
/*
 Notify user space application that WPS procedure will begin.
  Signal
    ra0: SIGXFSZ
    rai0: SIGWINCH
*/
    {
        struct task_struct *p;
        
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s - WSC_SINGLE_TRIGGER_APPNAME: %s\n", 
        		__FUNCTION__, WSC_SINGLE_TRIGGER_APPNAME));
        
        rcu_read_lock();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21)
	    for_each_process(p)
#else
	    for_each_task(p)
#endif
        {
            if(!strcmp(p->comm, WSC_SINGLE_TRIGGER_APPNAME))
            {
                if (pAd->dev_idx == 0)
                    send_sig(SIGXFSZ, p, 0);
                else
                    send_sig(SIGWINCH, p, 0);
            }
        }
        rcu_read_unlock();
    }
/* ---  added by YYHuang@Ralink, 08/03/12 */
#endif /* WSC_SINGLE_TRIGGER */
#endif /* LINUX */


	/* TODO: Is it possible ApCli call this fucntion?? */
	apIdx = (pWscControl->EntryIfIdx & 0x0F);

    IsAPConfigured = pWscControl->WscConfStatus;
    //pWscUPnPNodeInfo = &pWscControl->WscUPnPNodeInfo;

    if (pWscControl->WscConfMode == WSC_DISABLE)
    {
        pWscControl->bWscTrigger = FALSE;
        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscGetConfForUpnp:: WPS is disabled.\n"));
		return FALSE;
    }

    if (bFromUPnP)
        WscStop(pAd, FALSE, pWscControl);
    
	if (pWscControl->WscMode == 1)
		WscMode = DEV_PASS_ID_PIN;
	else
		WscMode = DEV_PASS_ID_PBC;
    
	WscBuildBeaconIE(pAd, IsAPConfigured, TRUE, WscMode, pWscControl->WscConfigMethods, (pWscControl->EntryIfIdx & 0x0F), NULL, 0, AP_MODE);
	WscBuildProbeRespIE(pAd, WSC_MSGTYPE_AP_WLAN_MGR, IsAPConfigured, TRUE, WscMode, pWscControl->WscConfigMethods, pWscControl->EntryIfIdx, NULL, 0, AP_MODE);
	APUpdateBeaconFrame(pAd, pWscControl->EntryIfIdx & 0x0F);

    /* 2mins time-out timer */
    RTMPSetTimer(&pWscControl->Wsc2MinsTimer, WSC_TWO_MINS_TIME_OUT);
    pWscControl->Wsc2MinsTimerRunning = TRUE;
    pWscControl->WscStatus = STATUS_WSC_LINK_UP;
    if (bFromUPnP)
		WscSendUPnPConfReqMsg(pAd, apIdx, (PUCHAR)pAd->ApCfg.MBSSID[apIdx].Ssid, 
									pAd->ApCfg.MBSSID[apIdx].wdev.bssid, 3, 0, AP_MODE);

    pWscControl->bWscTrigger = TRUE;
	pWscControl->bWscAutoTigeer = TRUE;
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s:: trigger WSC state machine\n", __FUNCTION__));

	return TRUE;
}
#endif /* CONFIG_AP_SUPPORT */

VOID WscSendNACK(
    IN	PRTMP_ADAPTER	pAdapter,
    IN  MAC_TABLE_ENTRY *pEntry,
	IN  PWSC_CTRL       pWscControl)
{
    INT     DataLen = 0;
    PUCHAR  pWscData = NULL;
    BOOLEAN Cancelled;
	UCHAR CurOpMode = AP_MODE;

#ifdef CONFIG_STA_SUPPORT
	if ((pAdapter->OpMode == OPMODE_STA)
		&& (pWscControl->EntryIfIdx == BSS0))
		CurOpMode = STA_MODE;
#endif /* CONFIG_STA_SUPPORT */

	os_alloc_mem(NULL, (UCHAR **)&pWscData, WSC_MAX_DATA_LEN);
	if (pWscData == NULL)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscSendNACK:: WscData Allocate failed!\n"));
		return;
	}

	NdisZeroMemory(pWscData, WSC_MAX_DATA_LEN);
    DataLen = BuildMessageNACK(pAdapter, pWscControl, pWscData);            
#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
    {
        if (pEntry && 
			(IS_ENTRY_APCLI(pEntry)
#ifdef P2P_SUPPORT
			|| (P2P_CLI_ON(pAdapter))
#endif /* P2P_SUPPORT */
			)
			)
            WscSendMessage(pAdapter, WSC_OPCODE_NACK, pWscData, DataLen, pWscControl, AP_CLIENT_MODE, EAP_CODE_RSP);
        else
            WscSendMessage(pAdapter, WSC_OPCODE_NACK, pWscData, DataLen, pWscControl, AP_MODE, EAP_CODE_REQ);
#if 0
		if (pEntry)
        	pEntry->bWscCapable = FALSE;
#endif
    }
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
	{
		if (ADHOC_ON(pAdapter) && (pWscControl->WscConfMode & WSC_REGISTRAR))
			WscSendMessage(pAdapter, WSC_OPCODE_NACK, pWscData, DataLen, pWscControl, STA_MODE, EAP_CODE_REQ);
		else
			WscSendMessage(pAdapter, WSC_OPCODE_NACK, pWscData, DataLen, pWscControl, STA_MODE, EAP_CODE_RSP);
	}
#endif /* CONFIG_STA_SUPPORT */

    RTMPCancelTimer(&pWscControl->EapolTimer, &Cancelled);
    pWscControl->EapolTimerRunning = FALSE;
    pWscControl->RegData.ReComputePke = 1;

	if (pWscData)
		os_free_mem(NULL, pWscData);
}

#ifdef WSC_INCLUDED
VOID WscCheckWpsIeFromWpsAP(
    IN  PRTMP_ADAPTER 	pAd, 
    IN  PEID_STRUCT		pEid,
#ifdef STA_EASY_CONFIG_SETUP
	OUT PUCHAR			pWpsAp,
#endif /* STA_EASY_CONFIG_SETUP */
    OUT PUSHORT			pDPIDFromAP)
{
	PUCHAR				pData;
	SHORT				Len = 0;
	PWSC_IE				pWscIE;
	USHORT				DevicePasswordID;
		
	if (NdisEqualMemory(pEid->Octet, WPS_OUI, 4)
#ifdef IWSC_SUPPORT
		|| NdisEqualMemory(pEid->Octet, IWSC_OUI, 4)
#endif // IWSC_SUPPORT //
		)
	{
		pData = (PUCHAR) pEid->Octet + 4;
		Len = (SHORT)(pEid->Len - 4);

		while (Len > 0)
		{
			WSC_IE	WscIE;
			NdisMoveMemory(&WscIE, pData, sizeof(WSC_IE));
			/* Check for WSC IEs */
			pWscIE = &WscIE;
			
			/* Check for device password ID, PIN = 0x0000, PBC = 0x0004 */
			if (pDPIDFromAP && be2cpu16(pWscIE->Type) == WSC_ID_DEVICE_PWD_ID)
			{
				/* Found device password ID */
				NdisMoveMemory(&DevicePasswordID, pData + 4, sizeof(DevicePasswordID));
				DevicePasswordID = be2cpu16(DevicePasswordID);
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("WscCheckWpsIeFromWpsAP : DevicePasswordID = 0x%04x\n", DevicePasswordID));
				if (DevicePasswordID == DEV_PASS_ID_PIN)
				{
					/* PIN */
					*pDPIDFromAP = DEV_PASS_ID_PIN;
				}
				else if (DevicePasswordID == DEV_PASS_ID_PBC)
				{
					/* PBC */
					*pDPIDFromAP = DEV_PASS_ID_PBC;
				}
			}
			
#ifdef CONFIG_STA_SUPPORT
#ifdef STA_EASY_CONFIG_SETUP
#define RALINK_AUTO_AP	0x02
			if (be2cpu16(pWscIE->Type) == WSC_ID_VENDOR_EXT)
			{
				UCHAR	vendor_id[3] = {0};
				NdisMoveMemory(vendor_id, pData + 4, 3);
				if (NdisEqualMemory(vendor_id, RALINK_OUI, 3))
				{
					if (pWpsAp)
						(*pWpsAp) |= RALINK_AUTO_AP;
					else
					{
						UCHAR reason = 0;
						reason = *(pData + 7);
						if (reason != 0) /* AP Reject STA */
						{
							if (pAd->StaCfg.BssType == BSS_INFRA)
							{
								pAd->Mlme.AutoProvisionMachine.CurrState = ADHOC_AUTO_PROVISION;
								if (pAd->StaCfg.EasyConfigInfo.ModuleType == MODULE_TV)
									MlmeEnqueue(pAd, AUTO_PROVISION_STATE_MACHINE, MT2_START_AUTO_PROVISION_IBSS, 0, NULL, 0);
								else
									MlmeEnqueue(pAd, AUTO_PROVISION_STATE_MACHINE, MT2_JOIN_AUTO_PROVISION_IBSS, 0, NULL, 0);
							}
							else
							{
								pAd->Mlme.AutoProvisionMachine.CurrState = INFRA_AUTO_PROVISION;
								MlmeEnqueue(pAd, AUTO_PROVISION_STATE_MACHINE, MT2_CONN_AP, 0, NULL, 0);
								RTMP_MLME_HANDLER(pAd);
							}
						}						
					}
				}
			}
#endif /* STA_EASY_CONFIG_SETUP */
#endif /* CONFIG_STA_SUPPORT */
			
			/* Set the offset and look for PBC information */
			/* Since Type and Length are both short type, we need to offset 4, not 2 */
			pData += (be2cpu16(pWscIE->Length) + 4);
			Len   -= (be2cpu16(pWscIE->Length) + 4);
		}
	}

    return;
}
#endif /* WSC_INCLUDED */

#ifdef CONFIG_STA_SUPPORT
VOID WscLinkDown(
	IN	PRTMP_ADAPTER	pAd)
{
	if (INFRA_ON(pAd) && OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
	{
		MLME_DISASSOC_REQ_STRUCT   DisassocReq;
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscLinkDown(): Disassociate with current WPS AP...\n"));
		DisassocParmFill(pAd, &DisassocReq, pAd->CommonCfg.Bssid, REASON_DISASSOC_STA_LEAVING);
		MlmeEnqueue(pAd, ASSOC_STATE_MACHINE, MT2_MLME_DISASSOC_REQ, 
					sizeof(MLME_DISASSOC_REQ_STRUCT), &DisassocReq, 0);
		pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_DISASSOC;
		pAd->MlmeAux.CurrReqIsFromNdis = TRUE;
		RTMP_MLME_HANDLER(pAd);
	}
	
	if (pAd->StaCfg.WscControl.WscConfMode != WSC_DISABLE)
	{		
#ifdef WSC_LED_SUPPORT
		UCHAR WPSLEDStatus;

		/* The protocol is connecting to a partner. */
		WPSLEDStatus = LED_WPS_IN_PROCESS;
		RTMPSetLED(pAd, WPSLEDStatus);
#endif /* WSC_LED_SUPPORT */

#ifdef IWSC_SUPPORT
		/*
			We need to send EAPOL_Start again to trigger WPS process
		*/
		if (pAd->StaCfg.BssType == BSS_ADHOC)
		{
			pAd->StaCfg.IWscInfo.bSendEapolStart = FALSE;
			pAd->StaCfg.WscControl.WscState = WSC_STATE_LINK_UP;
			pAd->StaCfg.WscControl.WscStatus = STATUS_WSC_LINK_UP;
			WscSendEapolStart(pAd, pAd->StaCfg.WscControl.WscPeerMAC, STA_MODE);
		}
		else
#endif /* IWSC_SUPPORT */
		pAd->StaCfg.WscControl.WscState = WSC_STATE_START;
	}
	else
	{
#ifdef STA_EASY_CONFIG_SETUP
		BOOLEAN		bCancel;
		pAd->StaCfg.EasyConfigInfo.bDoAutoWps = FALSE;
		RTMPCancelTimer(&pAd->StaCfg.EasyConfigInfo.AdhocConnTimer, &bCancel);
		if ((pAd->StaCfg.BssType == BSS_ADHOC) &&
			(pAd->StaCfg.EasyConfigInfo.ModuleType == MODULE_TV))
			pAd->StaCfg.WscControl.WscConfMode = WSC_REGISTRAR;

		pAd->StaCfg.bScanReqIsFromWebUI = FALSE;
#endif /* STA_EASY_CONFIG_SETUP */

		pAd->bConfigChanged = TRUE;
		pAd->StaCfg.WscControl.bWscTrigger = FALSE;

		if (pAd->StaCfg.BssType == BSS_INFRA)
		{
			BssTableDeleteEntry(&pAd->ScanTab, pAd->CommonCfg.Bssid, pAd->CommonCfg.Channel);
			pAd->MlmeAux.SsidBssTab.BssNr = 0;
			MlmeEnqueue(pAd, 
						MLME_CNTL_STATE_MACHINE, 
						OID_802_11_BSSID,
						MAC_ADDR_LEN,
						pAd->MlmeAux.Bssid, 0);

			RTMP_MLME_HANDLER(pAd);
		} 
#ifdef IWSC_SUPPORT
		else /* BSS_ADHOC */
		{
			NDIS_802_11_SSID	Ssid;
			if (pAd->StaCfg.IWscInfo.bReStart)
			{
				pAd->StaCfg.bNotFirstScan = FALSE;
				pAd->StaCfg.bAutoConnectByBssid = FALSE;
				pAd->StaCfg.IWscInfo.bReStart = FALSE;
				pAd->StaCfg.IWscInfo.bDoNotChangeBSSID = TRUE;

				LinkDown(pAd, FALSE);
				OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED);
				RTMP_IndicateMediaState(pAd, NdisMediaStateDisconnected);
	            pAd->ExtraInfo = GENERAL_LINK_DOWN;

				if (pAd->StaCfg.WscControl.WscStatus != STATUS_WSC_CONFIGURED)
				{
					Ssid.SsidLength = pAd->CommonCfg.SsidLen;
					NdisZeroMemory(Ssid.Ssid, MAX_LEN_OF_SSID);
					NdisMoveMemory(Ssid.Ssid, pAd->CommonCfg.Ssid, pAd->CommonCfg.SsidLen);
				}
				else
				{
					Ssid.SsidLength = pAd->MlmeAux.SsidLen;
					NdisZeroMemory(Ssid.Ssid, MAX_LEN_OF_SSID);
					NdisMoveMemory(Ssid.Ssid, pAd->MlmeAux.Ssid, Ssid.SsidLength);
				}
				MlmeEnqueue(pAd, 
	                    MLME_CNTL_STATE_MACHINE, 
	                    OID_802_11_SSID,
	                    sizeof(NDIS_802_11_SSID),
	                    (VOID *)&Ssid, 0);			
				RTMP_MLME_HANDLER(pAd);
			}		
			MlmeEnqueue(pAd, IWSC_STATE_MACHINE, IWSC_MT2_MLME_STOP, 0, NULL, 0);
			RTMP_MLME_HANDLER(pAd);
		}
#endif /* IWSC_SUPPORT */
	}
	pAd->StaCfg.WscControl.RegData.ReComputePke = 1;
	/* YF: Reset to default after active pbc mode */
		if (!pAd->StaCfg.WscControl.Wsc2MinsTimerRunning)
			pAd->StaCfg.WscControl.RegData.SelfInfo.DevPwdId = cpu2be16(DEV_PASS_ID_PIN);
	
}

ULONG WscSearchWpsApBySSID(
	IN PRTMP_ADAPTER	pAd,	
	IN PUCHAR	 		pSsid,
	IN UCHAR	 		SsidLen,
	IN INT		 		WscMode)
{
	UCHAR 		i;
	USHORT		DesiredDPID;
	BSS_ENTRY 	*pBss;
	PWSC_CTRL	pWscControl = &pAd->StaCfg.WscControl;

	if (WscMode == WSC_PBC_MODE)
		DesiredDPID = DEV_PASS_ID_PBC;
	else
		DesiredDPID = DEV_PASS_ID_PIN;

	for (i = 0; i < pAd->ScanTab.BssNr; i++) 
	{
		pBss = &pAd->ScanTab.BssEntry[i];
		if (SSID_EQUAL(pSsid, SsidLen, pBss->Ssid, pBss->SsidLen) &&
			pBss->WpsAP &&
			((pBss->WscDPIDFromWpsAP == DesiredDPID) || (DesiredDPID == DEV_PASS_ID_PIN)))
		{
			if ((pWscControl->WpsApBand == PREFERRED_WPS_AP_PHY_TYPE_5_G_FIRST) &&
				(pBss->Channel <= 14))
				continue;
			else if ((pWscControl->WpsApBand == PREFERRED_WPS_AP_PHY_TYPE_2DOT4_G_FIRST) &&
					 (pBss->Channel > 14))
				continue;
			else
				return (ULONG)i;
		}
	}
	return (ULONG)BSS_NOT_FOUND;
}
#endif /* CONFIG_STA_SUPPORT */

VOID WscPBCSessionOverlapCheck(
	IN  PRTMP_ADAPTER 	pAd)
{
	ULONG	now;
	PWSC_STA_PBC_PROBE_INFO	pWscStaPbcProbeInfo = &pAd->CommonCfg.WscStaPbcProbeInfo;
	
	pAd->CommonCfg.WscPBCOverlap = FALSE;
	if (pWscStaPbcProbeInfo->WscPBCStaProbeCount > 1)
	{
		UCHAR  i;
		
		for (i = 0; i < MAX_PBC_STA_TABLE_SIZE; i++)
		{
			NdisGetSystemUpTime(&now);
			if (pWscStaPbcProbeInfo->Valid[i] && 
				RTMP_TIME_AFTER(now, pWscStaPbcProbeInfo->ReciveTime[i] + 120*OS_HZ))
			{
				NdisZeroMemory(&(pWscStaPbcProbeInfo->StaMacAddr[i][0]), MAC_ADDR_LEN);
				pWscStaPbcProbeInfo->ReciveTime[i] = 0;
				pWscStaPbcProbeInfo->Valid[i] = FALSE;
				pWscStaPbcProbeInfo->WscPBCStaProbeCount--;
			}
		}
		
		if (pWscStaPbcProbeInfo->WscPBCStaProbeCount > 1)
			pAd->CommonCfg.WscPBCOverlap = TRUE;
	}

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscPBCSessionOverlapCheck : WscPBCStaProbeCount = %d\n", 
				pWscStaPbcProbeInfo->WscPBCStaProbeCount));
	return;
}

VOID WscPBC_DPID_FromSTA(
	IN  PRTMP_ADAPTER		pAd,	
	IN	PUCHAR				pMacAddr)
{
	INT		Index = 0;
	UCHAR	tab_idx;
	BOOLEAN bAddEntry = FALSE;
	ULONG	now;
	PWSC_STA_PBC_PROBE_INFO	pWscStaPbcProbeInfo = &pAd->CommonCfg.WscStaPbcProbeInfo;

	NdisGetSystemUpTime(&now);
	if (pWscStaPbcProbeInfo->WscPBCStaProbeCount == 0)
		bAddEntry = TRUE;
	else
	{
		for (tab_idx = 0; tab_idx < MAX_PBC_STA_TABLE_SIZE; tab_idx++)
		{
			if (NdisEqualMemory(pMacAddr, pWscStaPbcProbeInfo->StaMacAddr[tab_idx], MAC_ADDR_LEN))
			{
				pWscStaPbcProbeInfo->ReciveTime[tab_idx] = now;
				return;
			}
		}
		
		for (tab_idx = 0; tab_idx < MAX_PBC_STA_TABLE_SIZE; tab_idx++)
		{
			if (RTMP_TIME_AFTER(now, pWscStaPbcProbeInfo->ReciveTime[tab_idx] + 120*OS_HZ) || 
				NdisEqualMemory(pWscStaPbcProbeInfo->StaMacAddr[tab_idx], &ZERO_MAC_ADDR[0], MAC_ADDR_LEN))
			{
				if (pWscStaPbcProbeInfo->Valid[tab_idx] == FALSE)
				{
					Index = tab_idx;
					bAddEntry = TRUE;
					break;
				}
				else
				{
					pWscStaPbcProbeInfo->ReciveTime[tab_idx] = now;
					NdisMoveMemory(pWscStaPbcProbeInfo->StaMacAddr[tab_idx], pMacAddr, MAC_ADDR_LEN);
					return;
				}				
			}
		}
	}

	if (bAddEntry)
	{
		pWscStaPbcProbeInfo->WscPBCStaProbeCount++;
		pWscStaPbcProbeInfo->ReciveTime[Index] = now;
		pWscStaPbcProbeInfo->Valid[Index] = TRUE;
		NdisMoveMemory(pWscStaPbcProbeInfo->StaMacAddr[Index], pMacAddr, MAC_ADDR_LEN);
	}
}


VOID WscWriteConfToDatFile(RTMP_ADAPTER *pAd, UCHAR CurOpMode)
{
#ifdef __ECOS
		char	*cfgData = 0;
		PSTRING 		fileName = NULL;
		RTMP_OS_FD		file_r, file_w;
		RTMP_OS_FS_INFO 	osFSInfo;
		LONG			rv, fileLen = 0;
		PSTRING 		pTempStr = 0;
#ifdef CONFIG_AP_SUPPORT
		INT 			index = 0;
		UCHAR			apidx = (pAd->WriteWscCfgToDatFile & 0x0F);
#endif /* CONFIG_AP_SUPPORT */
		PWSC_CTRL		pWscControl = NULL;
		RTMP_STRING			WepKeyName[MAX_WEPKEYNAME_LEN] = {0};
		RTMP_STRING			WepKeyFormatName[MAX_WEPKEYTYPE_LEN] = {0};
	

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("-----> WscWriteConfToDatFile\n"));
#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
	{
		if (apidx > pAd->ApCfg.BssidNum)
		{
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("<----- WscWriteConfToDatFile (wrong apidx = %d)\n", apidx));
			return;
		}
		pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
#ifdef RTMP_RBUS_SUPPORT
		if (pAd->infType == RTMP_DEV_INF_RBUS)
			fileName = AP_PROFILE_PATH_RBUS;
		else
#endif /* RTMP_RBUS_SUPPORT */
			fileName = AP_PROFILE_PATH;

			snprintf((PSTRING) WepKeyName, sizeof(WepKeyName), "Key%dStr%d=", pAd->ApCfg.MBSSID[apidx].wdev.DefaultKeyId+1, apidx+1);
			snprintf((PSTRING) WepKeyFormatName, sizeof(WepKeyFormatName), "Key%dType=", pAd->ApCfg.MBSSID[apidx].wdev.DefaultKeyId+1);
	}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
	{
		pWscControl = &pAd->StaCfg.WscControl;
#ifdef RTMP_RBUS_SUPPORT
		if (pAd->infType == RTMP_DEV_INF_RBUS)
			fileName = STA_PROFILE_PATH_RBUS;
		else
#endif /* RTMP_RBUS_SUPPORT */
			fileName = STA_PROFILE_PATH;

		snprintf(WepKeyName, sizeof(WepKeyName), "Key%dStr=", pAd->StaCfg.DefaultKeyId+1);
		snprintf(WepKeyFormatName, sizeof(WepKeyFormatName), "Key%dType=", pAd->StaCfg.DefaultKeyId+1);
	}
#endif /* CONFIG_STA_SUPPORT */

	RtmpOSFSInfoChange(&osFSInfo, TRUE);

	file_r = RtmpOSFileOpen(fileName, O_RDONLY, 0);
	if (IS_FILE_OPEN_ERR(file_r)) 
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-->1) %s: Error opening file %s\n", __FUNCTION__, fileName));
	}
	else 
	{
		char tempStr[64] = {0};
		while((rv = RtmpOSFileRead(file_r, tempStr, 64)) > 0)
		{
			fileLen += rv;
		}
			cfgData = kmalloc(fileLen, MEM_ALLOC_FLAG);
		if (cfgData == NULL)
		{
			RtmpOSFileClose(file_r);
				DBGPRINT(RT_DEBUG_ERROR, ("CfgData kmalloc fail. (fileLen = %ld)\n", fileLen));
			goto out;
		}
		NdisZeroMemory(cfgData, fileLen);
		RtmpOSFileSeek(file_r, 0);
			rv = RtmpOSFileRead(file_r, (PSTRING)cfgData, fileLen);
		RtmpOSFileClose(file_r);
		if (rv != fileLen)
		{
				DBGPRINT(RT_DEBUG_ERROR, ("CfgData kmalloc fail, fileLen = %ld\n", fileLen));
			goto ReadErr;
		}
	}

	file_w = RtmpOSFileOpen(fileName, O_WRONLY|O_TRUNC, 0);	

		PSTRING profileTemp = 0;
		PSTRING profileTemp2 = 0;
		PSTRING profileTemp3 = 0;
		
		profileTemp = kmalloc(255, MEM_ALLOC_FLAG);
		profileTemp2 = kmalloc(255, MEM_ALLOC_FLAG);
		profileTemp3 = kmalloc(255, MEM_ALLOC_FLAG);
		
		if (!profileTemp)
		{
			DBGPRINT(RT_DEBUG_ERROR, ("profileTemp kmalloc fail. (255)\n"));
			goto WriteErr;
		} else if (!profileTemp2) {
			os_free_mem(NULL, profileTemp);
			DBGPRINT(RT_DEBUG_ERROR, ("profileTemp2 kmalloc fail. (255)\n"));
			goto WriteErr;
		} else if (!profileTemp3) {
			os_free_mem(NULL, profileTemp);
			os_free_mem(NULL, profileTemp2);
			DBGPRINT(RT_DEBUG_ERROR, ("profileTemp3 kmalloc fail. (255)\n"));
			goto WriteErr;
		}
		else
			DBGPRINT(RT_DEBUG_ERROR, ("profileTemp kmalloc succeed. (255)\n"));
	
#ifdef CONFIG_AP_SUPPORT
#ifdef SECOND_WIFI
		if (pAd->infType == RTMP_DEV_INF_PCIE) {
			DBGPRINT(RT_DEBUG_ERROR, ("CFG_WLNI_SSID1=%s\n", pAd->ApCfg.MBSSID[0].Ssid));
			DBGPRINT(RT_DEBUG_ERROR, ("CFG_WLNI_WscSSID=%s\n", pAd->ApCfg.MBSSID[0].Ssid));
			CFG_set(CFG_WLNI_SSID1, &pAd->ApCfg.MBSSID[0].Ssid); 
			CFG_set(CFG_WLNI_WscSSID, &pAd->ApCfg.MBSSID[0].Ssid); 
		} else 
#endif /* SECOND_WIFI */
		{
			DBGPRINT(RT_DEBUG_ERROR, ("CFG_WLN_SSID1=%s\n", pAd->ApCfg.MBSSID[0].Ssid));
			DBGPRINT(RT_DEBUG_ERROR, ("CFG_WLN_WscSSID=%s\n", pAd->ApCfg.MBSSID[0].Ssid));
			CFG_set(CFG_WLN_SSID1, &pAd->ApCfg.MBSSID[0].Ssid); 
			CFG_set(CFG_WLN_WscSSID, &pAd->ApCfg.MBSSID[0].Ssid); 
		}
#endif /* CONFIG_AP_SUPPORT*/
#ifdef CONFIG_AP_SUPPORT
	NdisZeroMemory(profileTemp, 255);
	NdisZeroMemory(profileTemp2, 255);
	NdisZeroMemory(profileTemp3, 255);
	INT WscAuthTypeTemp = 0;

	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		for (index = 0; index < pAd->ApCfg.BssidNum; index++)
		{
			if (pAd->ApCfg.MBSSID[index].SsidLen)
			{
				
					snprintf(profileTemp2, 255, "%s", RTMPGetRalinkAuthModeStr(pAd->ApCfg.MBSSID[index].wdev.AuthMode));
				DBGPRINT(RT_DEBUG_ERROR, ("pAd->ApCfg.MBSSID[%d].AuthMode=%s\n", index, profileTemp2));
				if (index == 0)
				{
					if (!strcmp(profileTemp2, "OPEN"))
						WscAuthTypeTemp = 1;
					else if (!strcmp(profileTemp2, "SHARED"))
						WscAuthTypeTemp = 4;
					else if (!strcmp(profileTemp2, "WPAPSK"))
						WscAuthTypeTemp = 2;
					else if (!strcmp(profileTemp2, "WPA"))
						WscAuthTypeTemp = 8;
					else if (!strcmp(profileTemp2, "WPA2PSK"))
						WscAuthTypeTemp = 32;
					else if (!strcmp(profileTemp2, "WPA2"))
						WscAuthTypeTemp = 16;
					else if (!strcmp(profileTemp2, "WPAPSKWPA2PSK"))
						WscAuthTypeTemp = 34;
					
					snprintf(profileTemp, 255, "%s", profileTemp2);
				}
				else
				{
					snprintf(profileTemp3, 255, "%s;%s", profileTemp, profileTemp2);
					snprintf(profileTemp, 255, "%s", profileTemp3);
				}
			}
		}
	}
#ifdef SECOND_WIFI
		if (pAd->infType == RTMP_DEV_INF_PCIE) {
			DBGPRINT(RT_DEBUG_ERROR, ("CFG_WLNI_AuthMode=%s\n", profileTemp));
			DBGPRINT(RT_DEBUG_ERROR, ("CFG_WLNI_WscAuthType=%d\n", WscAuthTypeTemp));
			CFG_set(CFG_WLNI_AuthMode, profileTemp); 
			CFG_set(CFG_WLNI_WscAuthType, &WscAuthTypeTemp); 
		} else 
#endif /* SECOND_WIFI */
		{
			DBGPRINT(RT_DEBUG_ERROR, ("CFG_WLN_AuthMode=%s\n", profileTemp));
			DBGPRINT(RT_DEBUG_ERROR, ("CFG_WLN_WscAuthType=%d\n", WscAuthTypeTemp));
			CFG_set(CFG_WLN_AuthMode, profileTemp); 
			CFG_set(CFG_WLN_WscAuthType, &WscAuthTypeTemp); 
		}		
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
	NdisZeroMemory(profileTemp, 255);
	NdisZeroMemory(profileTemp2, 255);
	NdisZeroMemory(profileTemp3, 255);
	INT WscEncrypTypeTemp = 0;

	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		for (index = 0; index < pAd->ApCfg.BssidNum; index++)
		{
				snprintf(profileTemp2, 255, "%s", RTMPGetRalinkEncryModeStr(pAd->ApCfg.MBSSID[index].wdev.WepStatus));
			DBGPRINT(RT_DEBUG_ERROR, ("pAd->ApCfg.MBSSID[%d].WepStatus=%s\n", index, profileTemp2));
			if (index == 0)
			{
				if (!strcmp(profileTemp2, "TKIP"))
					WscEncrypTypeTemp = 4;
				else if (!strcmp(profileTemp2, "AES"))
					WscEncrypTypeTemp = 8;
				else if (!strcmp(profileTemp2, "TKIPAES"))
					WscEncrypTypeTemp = 12;
				else if (!strcmp(profileTemp2, "NONE"))
					WscEncrypTypeTemp = 1;
				else if (!strcmp(profileTemp2, "WEP"))
					WscEncrypTypeTemp = 2;
				
				snprintf(profileTemp, 255, "%s", profileTemp2);
			}
			else
			{
				snprintf(profileTemp3, 255, "%s;%s", profileTemp, profileTemp2);
				snprintf(profileTemp, 255, "%s", profileTemp3);
			}
		}
	}
#ifdef SECOND_WIFI
		if (pAd->infType == RTMP_DEV_INF_PCIE) {
			DBGPRINT(RT_DEBUG_ERROR, ("CFG_WLNI_EncrypType=%s\n", profileTemp));
			DBGPRINT(RT_DEBUG_ERROR, ("CFG_WLNI_WscEncrypType=%d\n", WscEncrypTypeTemp));
			CFG_set(CFG_WLNI_EncrypType, profileTemp); 
			CFG_set(CFG_WLNI_WscEncrypType, &WscEncrypTypeTemp);
		} else 
#endif /* SECOND_WIFI */
		{
			DBGPRINT(RT_DEBUG_ERROR, ("CFG_WLN_EncrypType=%s\n", profileTemp));
			DBGPRINT(RT_DEBUG_ERROR, ("CFG_WLN_WscEncrypType=%d\n", WscEncrypTypeTemp));
			CFG_set(CFG_WLN_EncrypType, profileTemp); 
			CFG_set(CFG_WLN_WscEncrypType, &WscEncrypTypeTemp);
		}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
	NdisZeroMemory(profileTemp3, 255);
	
	pWscControl = &pAd->ApCfg.MBSSID[0].WscControl;

	NdisCopyMemory(profileTemp3, &pWscControl->WpaPsk[0], 64);
	profileTemp3[64] = '\0';
#ifdef SECOND_WIFI
		if (pAd->infType == RTMP_DEV_INF_PCIE) {
			DBGPRINT(RT_DEBUG_ERROR, ("CFG_WLNI_WPAPSK1=%s\n", &pWscControl->WpaPsk[0]));
			DBGPRINT(RT_DEBUG_ERROR, ("CFG_WLNI_WscNewKey=%s\n", &pWscControl->WpaPsk[0]));
			CFG_set(CFG_WLNI_WPAPSK1, profileTemp3);	
			CFG_set(CFG_WLNI_WscNewKey, profileTemp3);	
		} else 
#endif /* SECOND_WIFI */
		{
			DBGPRINT(RT_DEBUG_ERROR, ("CFG_WLN_WPAPSK1=%s\n", &pWscControl->WpaPsk[0]));
			DBGPRINT(RT_DEBUG_ERROR, ("CFG_WLN_WscNewKey=%s\n", &pWscControl->WpaPsk[0]));
			CFG_set(CFG_WLN_WPAPSK1, profileTemp3); 
			CFG_set(CFG_WLN_WscNewKey, profileTemp3);	
		}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_AP_SUPPORT

	pWscControl = &pAd->ApCfg.MBSSID[0].WscControl;
#ifdef SECOND_WIFI
		if (pAd->infType == RTMP_DEV_INF_PCIE) {
			DBGPRINT(RT_DEBUG_ERROR, ("CFG_WLNI_WscConfMode=%d\n", pWscControl->WscConfMode));
			CFG_set(CFG_WLNI_WscConfMode, &pWscControl->WscConfMode); 
		} else 
#endif /* SECOND_WIFI */
		{
			DBGPRINT(RT_DEBUG_ERROR, ("CFG_WLN_WscConfMode=%d\n", pWscControl->WscConfMode));
			CFG_set(CFG_WLN_WscConfMode, &pWscControl->WscConfMode); 
		}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_AP_SUPPORT

	INT WscConfStatusTemp = 0;
	pWscControl = &pAd->ApCfg.MBSSID[0].WscControl;
	WscConfStatusTemp = pWscControl->WscConfStatus;
#ifdef SECOND_WIFI
		if (pAd->infType == RTMP_DEV_INF_PCIE) {
			DBGPRINT(RT_DEBUG_ERROR, ("CFG_WLNI_WscConfStatus=%d\n", WscConfStatusTemp));
			CFG_set(CFG_WLNI_WscConfStatus, &WscConfStatusTemp); 
		} else 
#endif /* SECOND_WIFI */
		{
			DBGPRINT(RT_DEBUG_ERROR, ("CFG_WLN_WscConfStatus=%d\n", WscConfStatusTemp));
			CFG_set(CFG_WLN_WscConfStatus, &WscConfStatusTemp); 
		}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
	NdisZeroMemory(profileTemp, 255);

	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		for (index = 0; index < pAd->ApCfg.BssidNum; index++)
		{
			DBGPRINT(RT_DEBUG_ERROR, ("pAd->ApCfg.MBSSID[%d].wdev.DefaultKeyId=%d\n", index, pAd->ApCfg.MBSSID[index].wdev.DefaultKeyId+1));
			if (index == 0)
				snprintf(profileTemp, 255, "%d", pAd->ApCfg.MBSSID[index].wdev.DefaultKeyId+1);
			else
				snprintf(profileTemp, 255, "%s;%d", profileTemp, pAd->ApCfg.MBSSID[index].wdev.DefaultKeyId+1);
		}
	}
#ifdef SECOND_WIFI
		if (pAd->infType == RTMP_DEV_INF_PCIE) {
			DBGPRINT(RT_DEBUG_ERROR, ("CFG_WLNI_DefaultKeyID=%s\n", profileTemp));
			CFG_set(CFG_WLNI_DefaultKeyID, profileTemp); 
		} else 
#endif /* SECOND_WIFI */
		{
			DBGPRINT(RT_DEBUG_ERROR, ("CFG_WLN_DefaultKeyID=%s\n", profileTemp));
			CFG_set(CFG_WLN_DefaultKeyID, profileTemp); 
		}
#endif /* CONFIG_AP_SUPPORT */

	CFG_commit(0);

	RtmpOSFileClose(file_w);
	
		if (profileTemp)
			kfree(profileTemp);
	
		if (profileTemp2)
			kfree(profileTemp2);
	
		if (profileTemp3)
			kfree(profileTemp3);
	
	WriteErr:	
		if (pTempStr)
			kfree(pTempStr);
	ReadErr:
	WriteFileOpenErr:	 
		if (cfgData)
			kfree(cfgData);
out:
	RtmpOSFSInfoChange(&osFSInfo, FALSE);


	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("<----- WscWriteConfToDatFile\n"));
	return;
#else /* ! __ECOS */
	char	*cfgData = 0;
	RTMP_STRING *fileName = NULL;
	RTMP_OS_FD		file_r, file_w;
	RTMP_OS_FS_INFO		osFSInfo;
	LONG			rv, fileLen = 0;
	char			*offset = 0;
	RTMP_STRING *pTempStr = NULL;
#ifdef CONFIG_AP_SUPPORT
	INT				index = 0;
	UCHAR			apidx = (pAd->WriteWscCfgToDatFile & 0x0F);
#endif /* CONFIG_AP_SUPPORT */
	PWSC_CTRL		pWscControl = NULL;
	PWSC_CREDENTIAL	pCredentail = NULL;
	RTMP_STRING WepKeyName[MAX_WEPKEYNAME_LEN] = {0};
	RTMP_STRING WepKeyFormatName[MAX_WEPKEYNAME_LEN] = {0};
	INT				tempStrLen = 0;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscWriteConfToDatFile(CurOpMode = %d)\n", CurOpMode));

#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
	{
		if (apidx > pAd->ApCfg.BssidNum)
		{
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscWriteConfToDatFile (wrong apidx = %d)\n", apidx));
			return;
		}
		pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
#ifdef RTMP_RBUS_SUPPORT
		if (pAd->infType == RTMP_DEV_INF_RBUS)
			fileName = AP_PROFILE_PATH_RBUS;
		else
#endif /* RTMP_RBUS_SUPPORT */
			fileName = AP_PROFILE_PATH;

		snprintf((RTMP_STRING *) WepKeyName, sizeof(WepKeyName), "Key%dStr%d=", pAd->ApCfg.MBSSID[apidx].wdev.DefaultKeyId+1, apidx+1);
		snprintf((RTMP_STRING *) WepKeyFormatName, sizeof(WepKeyFormatName), "Key%dType=", pAd->ApCfg.MBSSID[apidx].wdev.DefaultKeyId+1);
	}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
	if (CurOpMode == STA_MODE)
	{
		pWscControl = &pAd->StaCfg.WscControl;
#ifdef RTMP_RBUS_SUPPORT
		if (pAd->infType == RTMP_DEV_INF_RBUS)
			fileName = STA_PROFILE_PATH_RBUS;
		else
#endif /* RTMP_RBUS_SUPPORT */
			fileName = STA_PROFILE_PATH;

		snprintf(WepKeyName, sizeof(WepKeyName), "Key%dStr=", pAd->StaCfg.wdev.DefaultKeyId+1);
		snprintf(WepKeyFormatName, sizeof(WepKeyFormatName), "Key%dType=", pAd->StaCfg.wdev.DefaultKeyId+1);
	}
#endif /* CONFIG_STA_SUPPORT */

	RtmpOSFSInfoChange(&osFSInfo, TRUE);

	file_r = RtmpOSFileOpen(fileName, O_RDONLY, 0);
	if (IS_FILE_OPEN_ERR(file_r)) 
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-->1) %s: Error opening file %s\n", __FUNCTION__, fileName));
		return;
	}
	else 
	{
		char tempStr[64] = {0};
		while((rv = RtmpOSFileRead(file_r, tempStr, 64)) > 0)
		{
			fileLen += rv;
		}
		os_alloc_mem(NULL, (UCHAR **)&cfgData, fileLen);
		if (cfgData == NULL)
		{
			RtmpOSFileClose(file_r);
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("CfgData mem alloc fail. (fileLen = %ld)\n", fileLen));
			goto out;
		}
		NdisZeroMemory(cfgData, fileLen);
		RtmpOSFileSeek(file_r, 0);
		rv = RtmpOSFileRead(file_r, (RTMP_STRING *)cfgData, fileLen);
		RtmpOSFileClose(file_r);
		if (rv != fileLen)
		{
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("CfgData mem alloc fail, fileLen = %ld\n", fileLen));
			goto ReadErr;
		}
	}

	file_w = RtmpOSFileOpen(fileName, O_WRONLY|O_TRUNC, 0);
	if (IS_FILE_OPEN_ERR(file_w)) 
	{
		goto WriteFileOpenErr;
	}
	else 
	{
		offset = (PCHAR) rtstrstr((RTMP_STRING *) cfgData, "Default\n");
		offset += strlen("Default\n");
		RtmpOSFileWrite(file_w, (RTMP_STRING *)cfgData, (int)(offset-cfgData));
		os_alloc_mem(NULL, (UCHAR **)&pTempStr, 512);
		if (!pTempStr)
		{
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("pTempStr mem alloc fail. (512)\n"));
			RtmpOSFileClose(file_w);
			goto WriteErr;
		}
			
		for (;;)
		{
			int i = 0;
			RTMP_STRING *ptr;
			BOOLEAN	bNewFormat = TRUE;

			NdisZeroMemory(pTempStr, 512);

			if ((size_t)(offset - cfgData) < fileLen)
			{
				ptr = (RTMP_STRING *) offset;
				while(*ptr && *ptr != '\n')
				{
					pTempStr[i++] = *ptr++;
				}
				pTempStr[i] = 0x00;

				offset += strlen(pTempStr) + 1;
				if ((strncmp(pTempStr, "SSID=", strlen("SSID=")) == 0) || 
					strncmp(pTempStr, "SSID1=", strlen("SSID1=")) == 0 ||
					strncmp(pTempStr, "SSID2=", strlen("SSID2=")) == 0 ||
					strncmp(pTempStr, "SSID3=", strlen("SSID3=")) == 0 ||
					strncmp(pTempStr, "SSID4=", strlen("SSID4=")) == 0
				)
				{
					if (rtstrstr(pTempStr, "SSID="))
						bNewFormat = FALSE;

					WscWriteSsidToDatFile(pAd, pTempStr, bNewFormat, CurOpMode);
				}
#ifdef CONFIG_STA_SUPPORT
				else if (strncmp(pTempStr, "NetworkType=", strlen("NetworkType=")) == 0)
				{
					NdisZeroMemory(pTempStr, 512);					
					if (pAd->StaCfg.BssType == BSS_ADHOC)
						snprintf(pTempStr, 512, "NetworkType=Adhoc");
					else
						snprintf(pTempStr, 512, "NetworkType=Infra");
				}
#endif /* CONFIG_STA_SUPPORT */
				else if (strncmp(pTempStr, "AuthMode=", strlen("AuthMode=")) == 0)
				{
					NdisZeroMemory(pTempStr, 512);
					snprintf(pTempStr, 512, "AuthMode=");
#ifdef CONFIG_AP_SUPPORT
					if (CurOpMode == AP_MODE)
					{
						for (index = 0; index < pAd->ApCfg.BssidNum; index++)
						{
							if (pAd->ApCfg.MBSSID[index].SsidLen)
							{
								if (index == 0)
									snprintf(pTempStr, 512, "%s%s", pTempStr, RTMPGetRalinkAuthModeStr(pAd->ApCfg.MBSSID[index].wdev.AuthMode));
								else
									snprintf(pTempStr, 512, "%s;%s", pTempStr, RTMPGetRalinkAuthModeStr(pAd->ApCfg.MBSSID[index].wdev.AuthMode));
							}
						}
					}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
					if (CurOpMode == STA_MODE)
					{
						USHORT auth_flag = WscGetAuthType(pAd->StaCfg.wdev.AuthMode);
						snprintf(pTempStr, 512, "%s%s", pTempStr, WscGetAuthTypeStr(auth_flag));
					}
#endif /* CONFIG_STA_SUPPORT */
				}
			else if (strncmp(pTempStr, "EncrypType=", strlen("EncrypType=")) == 0)
				{
					NdisZeroMemory(pTempStr, 512);
					snprintf(pTempStr, 512, "EncrypType=");
#ifdef CONFIG_AP_SUPPORT
					if (CurOpMode == AP_MODE)
					{
						for (index = 0; index < pAd->ApCfg.BssidNum; index++)
						{
							if (index == 0)
								snprintf(pTempStr, 512, "%s%s", pTempStr, RTMPGetRalinkEncryModeStr(pAd->ApCfg.MBSSID[index].wdev.WepStatus));
							else
								snprintf(pTempStr, 512, "%s;%s", pTempStr, RTMPGetRalinkEncryModeStr(pAd->ApCfg.MBSSID[index].wdev.WepStatus));
						}
					}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
					if (CurOpMode == STA_MODE)
					{
						USHORT encrypt_flag = WscGetEncryType(pAd->StaCfg.wdev.WepStatus);
						snprintf(pTempStr, 512, "%s%s", pTempStr, WscGetEncryTypeStr(encrypt_flag));
					}
#endif /* CONFIG_STA_SUPPORT */
				}    
				else if ((strncmp(pTempStr, "WPAPSK=", strlen("WPAPSK=")) == 0) || 
						(strncmp(pTempStr, "WPAPSK1=", strlen("WPAPSK1=")) == 0) ||
						(strncmp(pTempStr, "WPAPSK2=", strlen("WPAPSK2=")) == 0) ||
						(strncmp(pTempStr, "WPAPSK3=", strlen("WPAPSK3=")) == 0) ||
						(strncmp(pTempStr, "WPAPSK4=", strlen("WPAPSK4=")) == 0))
				{
						bNewFormat = TRUE;
						if (strstr(pTempStr, "WPAPSK="))                            
							bNewFormat = FALSE;
						WscWriteWpaPskToDatFile(pAd, pTempStr, bNewFormat);
				}
				else if (strncmp(pTempStr, "WscConfMode=", strlen("WscConfMode=")) == 0)
				{
						snprintf(pTempStr, 512, "WscConfMode=");
#ifdef CONFIG_AP_SUPPORT
						for (index = 0; index < pAd->ApCfg.BssidNum; index++)
						{
							pWscControl = &pAd->ApCfg.MBSSID[index].WscControl;
							if (index == 0)
								snprintf(pTempStr, 512, "%s%d", pTempStr, pWscControl->WscConfMode);
							else
								snprintf(pTempStr, 512, "%s;%d", pTempStr, pWscControl->WscConfMode);
						}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
						snprintf(pTempStr, 512, "%s%d", pTempStr, pWscControl->WscConfMode);
#endif /* CONFIG_STA_SUPPORT */
				}
				else if (strncmp(pTempStr, "WscConfStatus=", strlen("WscConfStatus=")) == 0)
				{
						snprintf(pTempStr, 512, "WscConfStatus=");
#ifdef CONFIG_AP_SUPPORT
						for (index = 0; index < pAd->ApCfg.BssidNum; index++)
						{
							pWscControl = &pAd->ApCfg.MBSSID[index].WscControl;
							if (index == 0)
								snprintf(pTempStr, 512, "%s%d", pTempStr, pWscControl->WscConfStatus);
							else
								snprintf(pTempStr, 512, "%s;%d", pTempStr, pWscControl->WscConfStatus);
						}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
						snprintf(pTempStr, 512, "%s%d", pTempStr, pWscControl->WscConfStatus);
#endif /* CONFIG_STA_SUPPORT */
				}
				else if (strncmp(pTempStr, "DefaultKeyID=", strlen("DefaultKeyID=")) == 0)
				{
					NdisZeroMemory(pTempStr, 512);
					snprintf(pTempStr, 512, "DefaultKeyID=");
#ifdef CONFIG_AP_SUPPORT
					if (CurOpMode == AP_MODE)
					{
						for (index = 0; index < pAd->ApCfg.BssidNum; index++)
						{
							if (index == 0)
								snprintf(pTempStr, 512, "%s%d", pTempStr, pAd->ApCfg.MBSSID[index].wdev.DefaultKeyId+1);
							else
								snprintf(pTempStr, 512, "%s;%d", pTempStr, pAd->ApCfg.MBSSID[index].wdev.DefaultKeyId+1);
						}
					}
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
					if (CurOpMode == STA_MODE)
						snprintf(pTempStr, 512, "%s%d", pTempStr, pAd->StaCfg.wdev.DefaultKeyId+1);
#endif /* CONFIG_STA_SUPPORT */
				}
#ifdef CONFIG_AP_SUPPORT
				else if ((strncmp(pTempStr, WepKeyFormatName, strlen(WepKeyFormatName)) == 0) &&
						 (CurOpMode == AP_MODE))
				{
					pCredentail = &pAd->ApCfg.MBSSID[apidx].WscControl.WscProfile.Profile[0];
					if (pAd->ApCfg.MBSSID[apidx].wdev.WepStatus == Ndis802_11WEPEnabled)
					{
						UCHAR idx = 0, KeyType[4] = {0};
			                        RTMP_STRING *ptr2, *temp_ptr;
						
						ptr2 = rtstrstr(pTempStr, "=");
						temp_ptr = pTempStr;
						pTempStr = ptr2+1;
						KeyType[0] = (UCHAR)(*pTempStr - 0x30);
						for (idx = 1; idx < 4; idx++)
						{
							ptr2 = rtstrstr(pTempStr, ";");
							if (ptr2 == NULL)
								break;
							pTempStr = ptr2+1;
							if ((pTempStr != NULL) ||
								(*pTempStr == '0') ||
								(*pTempStr == '1'))
								KeyType[idx] = (UCHAR)(*pTempStr - 0x30);
						}
						pTempStr = temp_ptr;			
						NdisZeroMemory(pTempStr, 512);
						NdisMoveMemory(pTempStr, WepKeyFormatName, strlen(WepKeyFormatName));
						for (idx = 0; idx < pAd->ApCfg.BssidNum; idx++)
						{
							if (idx == apidx)  
								snprintf(pTempStr, 512, "%s0", pTempStr);
							else
								snprintf(pTempStr, 512, "%s%d", pTempStr, KeyType[idx]);
							
							if (apidx < (pAd->ApCfg.BssidNum - 1))
								snprintf(pTempStr, 512, "%s;", pTempStr);
						}
					}
				}
				else if ((strncmp(pTempStr, WepKeyName, strlen(WepKeyName)) == 0) &&
						 (CurOpMode == AP_MODE))
				{
					pCredentail = &pAd->ApCfg.MBSSID[apidx].WscControl.WscProfile.Profile[0];
					if (pAd->ApCfg.MBSSID[apidx].wdev.WepStatus == Ndis802_11WEPEnabled)
					{
						NdisZeroMemory(pTempStr, 512);
						NdisMoveMemory(pTempStr, WepKeyName, strlen(WepKeyName));
						tempStrLen = strlen(pTempStr);
						if (pCredentail->KeyLength)
						{
							if ((pCredentail->KeyLength == 5) ||
								(pCredentail->KeyLength == 13))
							{
								int jjj=0;
								for (jjj=0; jjj<pCredentail->KeyLength; jjj++)
									snprintf(pTempStr, 512, "%s%02x", pTempStr, pCredentail->Key[jjj]);
							}
							else if ((pCredentail->KeyLength == 10) ||
								(pCredentail->KeyLength == 26))
							{
								NdisMoveMemory(pTempStr + tempStrLen, pCredentail->Key, pCredentail->KeyLength);
							}
						}
					}
				}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
				else if (rtstrstr(pTempStr, (RTMP_STRING *) WepKeyFormatName) &&  (CurOpMode == STA_MODE))
				{
					pCredentail = &pWscControl->WscProfile.Profile[pWscControl->WscProfile.ApplyProfileIdx];
					if (pAd->StaCfg.wdev.WepStatus == Ndis802_11WEPEnabled)
					{
						NdisZeroMemory(pTempStr, 512);
						snprintf(pTempStr, 512, "%s0", WepKeyFormatName); /* Hex */
					}
				}
				else if (rtstrstr(pTempStr, (RTMP_STRING *) WepKeyName) &&  (CurOpMode == STA_MODE))
				{
						pCredentail = &pWscControl->WscProfile.Profile[pWscControl->WscProfile.ApplyProfileIdx];
					if (pAd->StaCfg.wdev.WepStatus == Ndis802_11WEPEnabled)
					{
						NdisZeroMemory(pTempStr, 512);
						NdisMoveMemory(pTempStr, WepKeyName, strlen(WepKeyName));
						tempStrLen = strlen(pTempStr);
						if (pCredentail->KeyLength)
						{
							if ((pCredentail->KeyLength == 5) ||
								(pCredentail->KeyLength == 13))
							{
								int jjj=0;
								for (jjj=0; jjj<pCredentail->KeyLength; jjj++)
									snprintf(pTempStr, 512, "%s%02x", pTempStr, pCredentail->Key[jjj]);
							}
							else if ((pCredentail->KeyLength == 10) ||
								(pCredentail->KeyLength == 26))
							{
								NdisMoveMemory(pTempStr + tempStrLen, pCredentail->Key, pCredentail->KeyLength);
							}
						}
					}
				}
#endif /* CONFIG_STA_SUPPORT */

				RtmpOSFileWrite(file_w, pTempStr, strlen(pTempStr));
				RtmpOSFileWrite(file_w, "\n", 1);
			}
			else
			{
				break;
			}
		}
		RtmpOSFileClose(file_w);
	}

WriteErr:   
	if (pTempStr)
		os_free_mem(NULL, pTempStr);
ReadErr:
WriteFileOpenErr:    
	if (cfgData)
		os_free_mem(NULL, cfgData);
out:
	RtmpOSFSInfoChange(&osFSInfo, FALSE);


	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscWriteConfToDatFile\n"));
	return;
#endif /* __ECOS */
}

#ifdef CONFIG_AP_SUPPORT
void    WscWriteConfToAR9File(
    IN  PRTMP_ADAPTER 	pAd,
    IN  UCHAR			CurOpMode)
{
#if 0
	PCHAR			cfgData = 0;
#endif
	RTMP_STRING *fileName = NULL;
	RTMP_OS_FD		file_w;
	RTMP_OS_FS_INFO		osFSInfo;
	INT			offset = 0;
	INT			datoffset = 0;
	RTMP_STRING *pTempStr = 0;
	RTMP_STRING *pDatStr = 0;
#ifdef CONFIG_AP_SUPPORT
	INT				index = 0;
	UCHAR			apidx = MAIN_MBSSID;
#endif /* CONFIG_AP_SUPPORT */
	PWSC_CTRL		pWscControl = NULL;
	PWSC_CREDENTIAL	pCredentail = NULL;
	RTMP_STRING WepKeyName[MAX_WEPKEYNAME_LEN] = {0};
	RTMP_STRING WepKeyFormatName[MAX_WEPKEYTYPE_LEN] = {0};
	INT				tempStrLen = 0;
	RTMP_STRING item_str[10] = {0};

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-----> WscWriteConfToAR9File\n"));

#ifdef CONFIG_AP_SUPPORT
	if (CurOpMode == AP_MODE)
	{
		pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
			fileName = "/var/wps_profile.dat";

		snprintf((RTMP_STRING *) WepKeyName, sizeof(WepKeyName), "Key%dStr1=", pAd->ApCfg.MBSSID[MAIN_MBSSID].wdev.DefaultKeyId+1);
		snprintf((RTMP_STRING *) WepKeyFormatName, sizeof(WepKeyFormatName), "Key%dType=", pAd->ApCfg.MBSSID[MAIN_MBSSID].wdev.DefaultKeyId+1);
	}
#endif /* CONFIG_AP_SUPPORT */


	RtmpOSFSInfoChange(&osFSInfo, TRUE);
	file_w = RtmpOSFileOpen(fileName, O_WRONLY|O_CREAT, 0);
	if (IS_FILE_OPEN_ERR(file_w)) 
	{
		goto WriteFileOpenErr;
	}
	else 
	{
#if 0
		offset = (PCHAR) rtstrstr((RTMP_STRING *) cfgData, "Default\n");
		offset += strlen("Default\n");
		RtmpOSFileWrite(file_w, (RTMP_STRING *)cfgData, (int)(offset - cfgData));
#endif
		os_alloc_mem(NULL, (UCHAR **)&pTempStr, 512);
		if (!pTempStr)
		{
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("pTempStr mem alloc fail. (512)\n"));
			RtmpOSFileClose(file_w);
			goto WriteErr;
		}
		os_alloc_mem(NULL, (UCHAR **)&pDatStr, 4096);
		if (!pDatStr)
		{
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("pDatStr mem alloc fail. (4096)\n"));
			RtmpOSFileClose(file_w);
			goto WriteErr;
		}
			
		/*for (;;) */
		{
			NdisZeroMemory(pTempStr, 512);
			NdisZeroMemory(pDatStr, 4096);
			{
				{
					NdisZeroMemory(item_str, 10);
					for (apidx = 0; apidx < pAd->ApCfg.BssidNum; apidx++)
					{
						snprintf(item_str, sizeof(item_str), "SSID%d", (apidx + 1));
						{
							NdisMoveMemory(pTempStr, item_str, strlen(item_str));
							offset = strlen(pTempStr);
							NdisMoveMemory(pTempStr + offset, "=", 1);
							offset += 1;
							NdisMoveMemory(pTempStr + offset, pAd->ApCfg.MBSSID[apidx].Ssid, pAd->ApCfg.MBSSID[apidx].SsidLen);
							offset += pAd->ApCfg.MBSSID[apidx].SsidLen;
							NdisMoveMemory(pTempStr + offset, "\n", 1);
							offset += 1;
						}
						NdisZeroMemory(item_str, 10);
					}
				}
				NdisMoveMemory(pDatStr,pTempStr,offset);
				datoffset += offset;
				
				{
					offset=0;
					NdisZeroMemory(pTempStr, 512);
					snprintf(pTempStr, 512, "AuthMode=");
#ifdef CONFIG_AP_SUPPORT
					if (CurOpMode == AP_MODE)
					{
						for (index = 0; index < pAd->ApCfg.BssidNum; index++)
						{
							if (pAd->ApCfg.MBSSID[index].SsidLen)
							{
								if (index == 0)
									snprintf(pTempStr, 512, "%s%s", pTempStr, RTMPGetRalinkAuthModeStr(pAd->ApCfg.MBSSID[index].wdev.AuthMode));
								else
									snprintf(pTempStr, 512, "%s;%s", pTempStr, RTMPGetRalinkAuthModeStr(pAd->ApCfg.MBSSID[index].wdev.AuthMode));
							}
						}
						snprintf(pTempStr, 512, "%s\n", pTempStr);
						offset=strlen(pTempStr);
						NdisMoveMemory(pDatStr+datoffset,pTempStr,offset);
						datoffset += offset;
					}
#endif /* CONFIG_AP_SUPPORT */
				}

				{
					offset=0;
					NdisZeroMemory(pTempStr, 512);
					snprintf(pTempStr, 512, "EncrypType=");
#ifdef CONFIG_AP_SUPPORT
					if (CurOpMode == AP_MODE)
					{
						for (index = 0; index < pAd->ApCfg.BssidNum; index++)
						{
							if (index == 0)
								snprintf(pTempStr, 512, "%s%s", pTempStr, RTMPGetRalinkEncryModeStr(pAd->ApCfg.MBSSID[index].wdev.WepStatus));
							else
								snprintf(pTempStr, 512, "%s;%s", pTempStr, RTMPGetRalinkEncryModeStr(pAd->ApCfg.MBSSID[index].wdev.WepStatus));
						}
						snprintf(pTempStr, 512, "%s\n", pTempStr);
						offset=strlen(pTempStr);
						NdisMoveMemory(pDatStr+datoffset,pTempStr,offset);
						datoffset += offset;
					}
#endif /* CONFIG_AP_SUPPORT */
				}    

				{
					offset=0;
					NdisZeroMemory(pTempStr, 512);
					NdisZeroMemory(item_str, 10);
						for (apidx = 0; apidx < pAd->ApCfg.BssidNum; apidx++)
						{
							snprintf(item_str, sizeof(item_str), "WPAPSK%d", (apidx + 1));
							/*if (rtstrstr(pTempStr, item_str)) */
							{
								pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
								NdisMoveMemory(pTempStr, item_str, strlen(item_str));
								offset = strlen(pTempStr);
								NdisMoveMemory(pTempStr + offset, "=", 1);
								offset += 1;
								NdisMoveMemory(pTempStr + offset, pWscControl->WpaPsk, pWscControl->WpaPskLen);
								offset += pWscControl->WpaPskLen;
								NdisMoveMemory(pTempStr + offset, "\n", 1);
								offset += 1;
							}
							NdisZeroMemory(item_str, 10);
						}
						NdisMoveMemory(pDatStr+datoffset,pTempStr,offset);
						datoffset += offset;
				}

				{
						offset=0;
						NdisZeroMemory(pTempStr, 512);
						snprintf(pTempStr, 512, "WscConfMode=");
#ifdef CONFIG_AP_SUPPORT
						for (index = 0; index < pAd->ApCfg.BssidNum; index++)
						{
							pWscControl = &pAd->ApCfg.MBSSID[index].WscControl;
							if (index == 0)
								snprintf(pTempStr, 512, "%s%d", pTempStr, pWscControl->WscConfMode);
							else
								snprintf(pTempStr, 512, "%s;%d", pTempStr, pWscControl->WscConfMode);
						}
						snprintf(pTempStr, 512, "%s\n", pTempStr);
						offset=strlen(pTempStr);
						NdisMoveMemory(pDatStr+datoffset,pTempStr,offset);
						datoffset += offset;
#endif /* CONFIG_AP_SUPPORT */
				}

				{
						offset=0;
						NdisZeroMemory(pTempStr, 512);
						snprintf(pTempStr, 512, "WscConfStatus=");
#ifdef CONFIG_AP_SUPPORT
						for (index = 0; index < pAd->ApCfg.BssidNum; index++)
						{
							pWscControl = &pAd->ApCfg.MBSSID[index].WscControl;
							if (index == 0)
								snprintf(pTempStr, 512, "%s%d", pTempStr, pWscControl->WscConfStatus);
							else
								snprintf(pTempStr, 512, "%s;%d", pTempStr, pWscControl->WscConfStatus);
						}
						snprintf(pTempStr, 512, "%s\n", pTempStr);
						offset=strlen(pTempStr);
						NdisMoveMemory(pDatStr+datoffset,pTempStr,offset);
						datoffset += offset;
#endif /* CONFIG_AP_SUPPORT */
				}

				{
					offset=0;
					NdisZeroMemory(pTempStr, 512);
					snprintf(pTempStr, 512, "DefaultKeyID=");
#ifdef CONFIG_AP_SUPPORT
					if (CurOpMode == AP_MODE)
					{
						for (index = 0; index < pAd->ApCfg.BssidNum; index++)
						{
							pWscControl = &pAd->ApCfg.MBSSID[index].WscControl;
							if (index == 0)
								snprintf(pTempStr, 512, "%s%d", pTempStr, pAd->ApCfg.MBSSID[apidx].wdev.DefaultKeyId+1);
							else
								snprintf(pTempStr, 512, "%s;%d", pTempStr, pAd->ApCfg.MBSSID[apidx].wdev.DefaultKeyId+1);
						}
						snprintf(pTempStr, 512, "%s\n", pTempStr);
						offset=strlen(pTempStr);
						NdisMoveMemory(pDatStr+datoffset,pTempStr,offset);
						datoffset += offset;
					}

#endif /* CONFIG_AP_SUPPORT */
				}
#ifdef CONFIG_AP_SUPPORT
					if (CurOpMode == AP_MODE)
					{
						for (index = 1; index <= 4; index++)
						{
							snprintf(WepKeyFormatName, sizeof(WepKeyFormatName), "Key%dType=", index);
							/*if (rtstrstr(pTempStr, WepKeyFormatName)) */
							{
								NdisZeroMemory(pTempStr, 512);
								offset=0;
								NdisMoveMemory(pTempStr, WepKeyFormatName, strlen(WepKeyFormatName));
								for (apidx = 0; apidx < pAd->ApCfg.BssidNum; apidx++)
								{
									if (pAd->ApCfg.MBSSID[MAIN_MBSSID].wdev.WepStatus == Ndis802_11WEPEnabled)  
									{
										pCredentail = &pAd->ApCfg.MBSSID[apidx].WscControl.WscProfile.Profile[0];
										if ((pCredentail->KeyLength == 5) ||
											(pCredentail->KeyLength == 13))
											snprintf(pTempStr, 512, "%s1", pTempStr); /* ASCII */
										else
											snprintf(pTempStr, 512, "%s0", pTempStr); /* Hex */
									}
									if (apidx < (pAd->ApCfg.BssidNum - 1))
										snprintf(pTempStr, 512, "%s;", pTempStr);
								}
								snprintf(pTempStr, 512, "%s\n", pTempStr);
								offset=strlen(pTempStr);
								NdisMoveMemory(pDatStr+datoffset,pTempStr,offset);
								datoffset += offset;
							}

							snprintf(WepKeyName, sizeof(WepKeyName), "Key%dStr=", index);
							/*if (rtstrstr(pTempStr, WepKeyName)) */
							{
								NdisZeroMemory(pTempStr, 512);
								offset=0;
								NdisMoveMemory(pTempStr, WepKeyName, strlen(WepKeyName));
								tempStrLen = strlen(pTempStr);
								for (apidx = 0; apidx < pAd->ApCfg.BssidNum; apidx++)
								{
									pCredentail = &pAd->ApCfg.MBSSID[apidx].WscControl.WscProfile.Profile[0];
									if (pCredentail->KeyLength)
									{
										NdisMoveMemory(pTempStr + tempStrLen, pCredentail->Key, pCredentail->KeyLength);
										tempStrLen = strlen(pTempStr);
									}
									if (apidx < (pAd->ApCfg.BssidNum - 1))
										NdisMoveMemory(pTempStr + tempStrLen, ";", 1);
									tempStrLen += 1;
								}
								snprintf(pTempStr, 512, "%s\n", pTempStr);
								offset=strlen(pTempStr);
								NdisMoveMemory(pDatStr+datoffset,pTempStr,offset);
								datoffset += offset;
							}
							
							for (apidx = 0; apidx < pAd->ApCfg.BssidNum; apidx++)
							{								
								snprintf(WepKeyName, sizeof(WepKeyName), "Key%dStr%d=", index, (apidx + 1));								
								if ((pAd->ApCfg.MBSSID[apidx].wdev.WepStatus == Ndis802_11WEPEnabled))
								{
									NdisZeroMemory(pTempStr, 512);
									NdisMoveMemory(pTempStr, WepKeyName, strlen(WepKeyName));
									tempStrLen = strlen(pTempStr);
									pCredentail = &pAd->ApCfg.MBSSID[apidx].WscControl.WscProfile.Profile[0];
									NdisMoveMemory(pTempStr + tempStrLen, pCredentail->Key, pCredentail->KeyLength);
									NdisMoveMemory(pTempStr + tempStrLen+pCredentail->KeyLength, "\n", 1);
								}
								offset=tempStrLen+pCredentail->KeyLength+1;
								NdisMoveMemory(pDatStr+datoffset,pTempStr,offset);
								datoffset += offset;
							}
						}
					}
#endif /* CONFIG_AP_SUPPORT */
				RtmpOSFileWrite(file_w, pDatStr, datoffset);
				/*RtmpOSFileWrite(file_w, "\n", 1); */
			}
		}
		RtmpOSFileClose(file_w);
	}

WriteErr:   
	if (pTempStr)
		os_free_mem(NULL, pTempStr);
	if (pDatStr)
		os_free_mem(NULL, pDatStr);

WriteFileOpenErr:    
#if 0
	if (cfgData)
		os_free_mem(NULL, cfgData);
#endif
	RtmpOSFSInfoChange(&osFSInfo, FALSE);


	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<----- WscWriteConfToAR9File\n"));
	return;
}
#endif/*CONFIG_AP_SUPPORT*/

static INT wsc_write_dat_file_thread (
    IN ULONG Context)
{
	RTMP_OS_TASK *pTask;
	RTMP_ADAPTER *pAd;
	int 	Status = 0;

	pTask = (RTMP_OS_TASK *)Context;
	pAd = (PRTMP_ADAPTER)RTMP_OS_TASK_DATA_GET(pTask);

	if (pAd == NULL)
		return 0;

	RtmpOSTaskCustomize(pTask);

	while (pTask && !RTMP_OS_TASK_IS_KILLED(pTask))
	{
		RtmpusecDelay(2000);

		if (RtmpOSTaskWait(pAd, pTask, &Status) == FALSE)
		{
			RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS);
			break;
		}

		if (Status != 0)
			break;

#ifdef RTMP_MAC_USB		
		/* device had been closed */
		if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST))
			break;
#endif /* RTMP_MAC_USB */

		if (pAd->pWscElme && (pAd->pWscElme->MsgLen != 0))
		{
			MLME_QUEUE_ELEM	*pElme;
			os_alloc_mem(pAd, (UCHAR **)&pElme, sizeof(MLME_QUEUE_ELEM));
			if (pElme)
			{
				NdisZeroMemory(pElme, sizeof(MLME_QUEUE_ELEM));
				RTMP_SEM_LOCK(&pAd->WscElmeLock);
				NdisMoveMemory(pElme, pAd->pWscElme, sizeof(MLME_QUEUE_ELEM));
				pAd->pWscElme->MsgLen = 0;
				NdisZeroMemory(pAd->pWscElme->Msg, MGMT_DMA_BUFFER_SIZE);
				RTMP_SEM_UNLOCK(&pAd->WscElmeLock);
				WpsSmProcess(pAd, pElme);
				os_free_mem(NULL, pElme);
			}
		}

		if (pAd->WriteWscCfgToDatFile != 0xFF)
		{
			UCHAR	CurOpMode = AP_MODE;

#ifdef CONFIG_AP_SUPPORT
			IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
				CurOpMode = AP_MODE;
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
			IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
				CurOpMode = STA_MODE;
			if (pAd->WriteWscCfgToDatFile != BSS0)
				CurOpMode = AP_MODE;
#endif /* CONFIG_STA_SUPPORT */

#ifndef DPA_T
			WscWriteConfToDatFile(pAd, CurOpMode);
#endif /* !DPA_T */

#ifdef CONFIG_AP_SUPPORT
#ifdef INF_AR9
#ifdef AR9_MAPI_SUPPORT
			WscWriteConfToAR9File(pAd, CurOpMode);
#endif /*AR9_MAPI_SUPPORT*/
#endif /* INF_AR9 */
#endif/*CONFIG_AP_SUPPORT*/
			pAd->WriteWscCfgToDatFile = 0xFF;
		}
	}

	if (pTask)
		RtmpOSTaskNotifyToExit(pTask);
	
	return 0;
}


/*
  * This kernel thread init in the probe fucntion, so we should kill it when do remove module.
  */
BOOLEAN WscThreadExit(RTMP_ADAPTER *pAd)
{	
	INT ret;
	
	/* 
		This kernel thread init in the probe fucntion, so kill it when do remove module. 
	*/
	ret = RtmpOSTaskKill(&pAd->wscTask);
	if (ret == NDIS_STATUS_FAILURE)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("kill wsc task failed!\n"));
	}
	
	if (pAd->pHmacData)
	{
		os_free_mem(NULL, pAd->pHmacData);
		pAd->pHmacData = NULL;
	}
	if (pAd->pWscElme)
	{
		os_free_mem(NULL, pAd->pWscElme);
		pAd->pWscElme = NULL;
	}
	NdisFreeSpinLock(&pAd->WscElmeLock);
#ifdef CONFIG_AP_SUPPORT
	if ((pAd->OpMode == OPMODE_AP)
#ifdef P2P_SUPPORT
		/* P2P will use ApCfg.MBSSID and ApCfg.ApCliTab also. */
		|| TRUE
#endif /* P2P_SUPPORT */
		)
	{
		INT ap_idx;
		UCHAR MaxBssidNum = MAX_MBSSID_NUM(pAd);

		for (ap_idx = 0; ap_idx < MaxBssidNum; ap_idx++)
		{
			PWSC_CTRL	pWpsCtrl = &pAd->ApCfg.MBSSID[ap_idx].WscControl;
			WscStop(pAd, FALSE, pWpsCtrl);
			pWpsCtrl->WscRxBufLen = 0;
			if (pWpsCtrl->pWscRxBuf)
			{
				os_free_mem(pAd, pWpsCtrl->pWscRxBuf);
				pWpsCtrl->pWscRxBuf = NULL;
			}
			pWpsCtrl->WscTxBufLen = 0;
			if (pWpsCtrl->pWscTxBuf)
			{
				os_free_mem(pAd, pWpsCtrl->pWscTxBuf);
				pWpsCtrl->pWscTxBuf = NULL;
			}
#ifdef WSC_V2_SUPPORT
			if (pWpsCtrl->WscSetupLockTimerRunning)
			{
				BOOLEAN Cancelled;
				pWpsCtrl->WscSetupLockTimerRunning = FALSE;
				RTMPCancelTimer(&pWpsCtrl->WscSetupLockTimer, &Cancelled);
			}
			if (pWpsCtrl->WscV2Info.ExtraTlv.pTlvData)
			{
				os_free_mem(NULL, pWpsCtrl->WscV2Info.ExtraTlv.pTlvData);
				pWpsCtrl->WscV2Info.ExtraTlv.pTlvData = NULL;
			}
#endif // WSC_V2_SUPPORT //
			WscClearPeerList(&pWpsCtrl->WscPeerList);
			NdisFreeSpinLock(&pWpsCtrl->WscPeerListSemLock);
#ifdef WAC_SUPPORT
			WscClearPeerList(&pAd->ApCfg.MBSSID[ap_idx].EasyConfigInfo.WAC_PeerList);
			NdisFreeSpinLock(&pAd->ApCfg.MBSSID[ap_idx].EasyConfigInfo.WAC_PeerListSemLock);
#ifdef WAC_QOS_PRIORITY
			WscClearPeerList(&pAd->ApCfg.MBSSID[ap_idx].EasyConfigInfo.WAC_ForcePriorityList);
			NdisFreeSpinLock(&pAd->ApCfg.MBSSID[ap_idx].EasyConfigInfo.WAC_ForcePriorityListSemLock);
#endif /* WAC_QOS_PRIORITY */
#endif /* WAC_SUPPORT */
		}		
#ifdef APCLI_SUPPORT
		{
			INT index;
			WscStop(pAd, TRUE, &pAd->ApCfg.ApCliTab[BSS0].WscControl);

			for(index = 0; index < MAX_APCLI_NUM; index++)
			{
				PWSC_CTRL       pWpsCtrl = &pAd->ApCfg.ApCliTab[index].WscControl;

				pWpsCtrl->WscTxBufLen = 0;
				if (pWpsCtrl->pWscTxBuf)
					os_free_mem(pAd, pWpsCtrl->pWscTxBuf);
				pWpsCtrl->WscRxBufLen = 0;
				if (pWpsCtrl->pWscRxBuf)
					os_free_mem(pAd, pWpsCtrl->pWscRxBuf);
				WscClearPeerList(&pWpsCtrl->WscPeerList);
				NdisFreeSpinLock(&pWpsCtrl->WscPeerListSemLock);
			}
		}
#endif // APCLI_SUPPORT //
	}	
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
	{
		WscStop(pAd,
#ifdef CONFIG_AP_SUPPORT
				FALSE,
#endif // CONFIG_AP_SUPPORT //
				&pAd->StaCfg.WscControl);
		pAd->StaCfg.WscControl.WscRxBufLen = 0;
		if (pAd->StaCfg.WscControl.pWscRxBuf)
		{
			os_free_mem(pAd, pAd->StaCfg.WscControl.pWscRxBuf);
			pAd->StaCfg.WscControl.pWscRxBuf = NULL;
		}
		pAd->StaCfg.WscControl.WscTxBufLen = 0;
		if (pAd->StaCfg.WscControl.pWscTxBuf)
		{
			os_free_mem(pAd, pAd->StaCfg.WscControl.pWscTxBuf);
			pAd->StaCfg.WscControl.pWscTxBuf = NULL;
		}
		WscClearPeerList(&pAd->StaCfg.WscControl.WscPeerList);
		NdisFreeSpinLock(&pAd->StaCfg.WscControl.WscPeerListSemLock);

#ifdef IWSC_SUPPORT
		WscClearPeerList(&pAd->StaCfg.WscControl.WscConfiguredPeerList);
		NdisFreeSpinLock(&pAd->StaCfg.WscControl.WscConfiguredPeerListSemLock);
#endif /* IWSC_SUPPORT */
	}
#endif /* CONFIG_STA_SUPPORT */

	/* WSC hardware push button function 0811 */
	WSC_HDR_BTN_Stop(pAd);	
	return TRUE;
}


/*
  * This kernel thread init in the probe function.
  */
NDIS_STATUS WscThreadInit(RTMP_ADAPTER *pAd)
{
	NDIS_STATUS status = NDIS_STATUS_FAILURE;
	RTMP_OS_TASK *pTask;

	
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("-->WscThreadInit()\n"));

	pTask = &pAd->wscTask;

#ifdef __ECOS
	pAd->WriteWscCfgToDatFile = 0xFF;
#endif /* __ECOS */

	RTMP_OS_TASK_INIT(pTask, "RtmpWscTask", pAd);
	status = RtmpOSTaskAttach(pTask, wsc_write_dat_file_thread, (ULONG)&pAd->wscTask);
	if (status == NDIS_STATUS_SUCCESS)
	{
		os_alloc_mem(NULL, &pAd->pHmacData, sizeof(CHAR)*(2048));
		if (pAd->pHmacData == NULL)
		{
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Wsc HmacData memory alloc failed!\n"));
			status = FALSE;
		}
		NdisAllocateSpinLock(pAd, &pAd->WscElmeLock);
		os_alloc_mem(NULL, (UCHAR **)&pAd->pWscElme, sizeof(MLME_QUEUE_ELEM));
	}
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<--WscThreadInit(), status=%d!\n", status));

	return status;
}


/* WSC hardware push button function 0811 */
/*
========================================================================
Routine Description:
	Initialize the PUSH PUTTION Check Module.

Arguments:
	ad_p			- WLAN control block pointer

Return Value:
	None

Note:
========================================================================
*/
VOID WSC_HDR_BTN_Init(
	IN	PRTMP_ADAPTER	pAd)
{
	pAd->CommonCfg.WscHdrPshBtnCheckCount = 0;
	
	//add mjh wsc key init gpio 45
	pAd_mjh = pAd;
	unsigned int result;
	printf("wsc key init\n");
	
	result = HAL_REG32(GPIOMODE);     //uart1  gpiomode1
	result &= ~0x3000000;    //clear 0    bit 25 bit 24
	result |= 0x1000000;     //set 1         gpio mode
	HAL_REG32(GPIOMODE) = result;
	
	result = HAL_REG32(RALINK_REG_PIO6332DIR); //45    input
	result &= ~0x1<<(45-32);          //clear 0 input                              
	HAL_REG32(RALINK_REG_PIO6332DIR) = result;
} /* End of WSC_HDR_BTN_Init */


/*
========================================================================
Routine Description:
	Stop the PUSH PUTTION Check Module.

Arguments:
	ad_p			- WLAN control block pointer

Return Value:
	None

Note:
========================================================================
*/
VOID WSC_HDR_BTN_Stop(
	IN	PRTMP_ADAPTER	pAd)
{
	pAd->CommonCfg.WscHdrPshBtnCheckCount = 0;
} /* End of WSC_HDR_BTN_Stop */


/*
========================================================================
Routine Description:
	Start the PUSH PUTTION Check thread.

Arguments:
	*Context		- WLAN control block pointer

Return Value:
	0			- terminate the thread successfully

Note:
========================================================================
*/
#ifdef CONFIG_AP_SUPPORT
extern INT	Set_AP_WscMode_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);
extern INT	Set_AP_WscGetConf_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);
#endif /* CONFIG_AP_SUPPORT */

/*#ifdef CONFIG_STA_SUPPORT */
#if 0
extern INT	Set_WscConfMode_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);
extern INT	Set_WscMode_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);
extern INT	Set_WscGetConf_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);
#endif /* CONFIG_STA_SUPPORT */

#ifndef RTMP_RESET_WPS_SHARE_PIN_SUPPORT
VOID WSC_HDR_BTN_CheckHandler(
	IN	PRTMP_ADAPTER	pAd)
{
 	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	BOOLEAN flg_pressed = 0;
	//********************************************mjh
	//WSC_HDR_BTN_MR_PRESS_FLG_GET(pAd, flg_pressed);
	unsigned int result;
	result = HAL_REG32(RALINK_REG_PIO6332DATA);
	result = result>>(45-32);
	result &= ~0xfffffffe;
	if(result == 0)
		flg_pressed = 1;
	else
		flg_pressed = 0;
	//********************************************mjh
	if (flg_pressed)
	{
		/* the button is pressed */
		if (pAd->CommonCfg.WscHdrPshBtnCheckCount == WSC_HDR_BTN_CONT_TIMES)
		{
			//mjh wsc key pressed flag
			mjh_wsc_link = 0;
			mjh_wsc_flag = 1;
			mjh_wsc_fail = 0;   //clear 
			mjh_2_out = 0;
			weak();

			/* we only handle once until the button is released */
			pAd->CommonCfg.WscHdrPshBtnCheckCount = 0;

			/* execute WSC PBC function */
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("wsc> execute WSC PBC...\n"));

#ifdef CONFIG_AP_SUPPORT
			IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
			{
				//diag_printf("connect >>>>>\n"); //mjh
				pObj->ioctl_if = 0;
				Set_ApCli_Enable_Proc(pAd, (PUCHAR)"0");//mjh
				Set_AP_WscConfMode_Proc(pAd, (PUCHAR)"1");//mjh
				Set_AP_WscMode_Proc(pAd, (PUCHAR)"2"); /* 2: PBC */
				Set_ApCli_Enable_Proc(pAd, (PUCHAR)"1");//mjh
				Set_AP_WscGetConf_Proc(pAd, (PUCHAR)"1"); /* 1: Trigger */
				//diag_printf("connect <<<<<\n");
			}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
			IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
			{
				/*			
				pObj->ioctl_if = 0;
				Set_WscConfMode_Proc(pAd, (PUCHAR)"1"); // 1:
				Set_WscMode_Proc(pAd, (PUCHAR)"2"); // 2: PBC
				Set_WscGetConf_Proc(pAd, (PUCHAR)"1"); // 1: Trigger  			*/
				//diag_printf("connect >>>>>\n"); //mjh
				pObj->ioctl_if = 0;
				Set_ApCli_Enable_Proc(pAd, (PUCHAR)"0");//mjh
				Set_AP_WscConfMode_Proc(pAd, (PUCHAR)"1");//mjh
				Set_AP_WscMode_Proc(pAd, (PUCHAR)"2"); /* 2: PBC */
				Set_ApCli_Enable_Proc(pAd, (PUCHAR)"1");//mjh
				Set_AP_WscGetConf_Proc(pAd, (PUCHAR)"1"); /* 1: Trigger */
				//diag_printf("connect <<<<<\n");
			}
#endif /* CONFIG_STA_SUPPORT */
			return;
		}

		pAd->CommonCfg.WscHdrPshBtnCheckCount ++;
	}
	else
	{
		/* the button is released */
		pAd->CommonCfg.WscHdrPshBtnCheckCount = 0;		
	}
}
#else

VOID WSC_HDR_BTN_CheckHandler(
	IN	PRTMP_ADAPTER	pAd)
{
 	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	BOOLEAN flg_pressed = 0;
	static char pbc_button_press_start =0;
	
	WSC_HDR_BTN_MR_PRESS_FLG_GET(pAd, flg_pressed);
	//diag_printf("check button\n");
	if (flg_pressed)
	{	//diag_printf("check button pressed\n");
		if(pAd->CommonCfg.WscHdrPshBtnCheckCount == 0)
			pbc_button_press_start = 1;
		if(pbc_button_press_start == 1)
			pAd->CommonCfg.WscHdrPshBtnCheckCount++;
	}
	else
	{
		pbc_button_press_start = 0;
	}
		
	if(pbc_button_press_start == 0)
	{	
		/* the button is pressed */
		if (pAd->CommonCfg.WscHdrPshBtnCheckCount <= WSC_HDR_BTN_CONT_TIMES && pAd->CommonCfg.WscHdrPshBtnCheckCount >= 1)
		{	
			//diag_printf("check button wps\n");
			/* we only handle once until the button is released */
			pAd->CommonCfg.WscHdrPshBtnCheckCount = 0;

			/* execute WSC PBC function */
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("wsc> execute WSC PBC...\n"));

#ifdef CONFIG_AP_SUPPORT
			IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
			{
				pObj->ioctl_if = 0;
				Set_AP_WscMode_Proc(pAd, (PUCHAR)"2"); /* 2: PBC */
				Set_AP_WscGetConf_Proc(pAd, (PUCHAR)"1"); /* 1: Trigger */
			}
#endif /* CONFIG_AP_SUPPORT */

#ifdef CONFIG_STA_SUPPORT
			IF_DEV_CONFIG_OPMODE_ON_STA(pAd)
			{
				pObj->ioctl_if = 0;
				Set_WscConfMode_Proc(pAd, (PUCHAR)"1"); /* 1:  */
				Set_WscMode_Proc(pAd, (PUCHAR)"2"); /* 2: PBC */
				Set_WscGetConf_Proc(pAd, (PUCHAR)"1"); /* 1: Trigger */
			}
#endif /* CONFIG_STA_SUPPORT */
		}
		else if(pAd->CommonCfg.WscHdrPshBtnCheckCount > WSC_HDR_BTN_CONT_TIMES)
		{	diag_printf("check button reset\n");
			pAd->CommonCfg.WscHdrPshBtnCheckCount = 0;
		    /* execute Reset function */
        	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("Restore to factory default settings...\n"));			
			CGI_reset_default();
		}
		else
		{	//diag_printf("check button null\n");
			pAd->CommonCfg.WscHdrPshBtnCheckCount = 0;
		}

	}

}

#endif

#ifdef WSC_LED_SUPPORT
/* */
/* Support WPS LED mode (mode 7, mode 8 and mode 9). */
/* Ref: User Feedback (page 80, WPS specification 1.0) */
/* */
BOOLEAN WscSupportWPSLEDMode(
	IN PRTMP_ADAPTER pAd)
{
	if ((LED_MODE(pAd) == WPS_LED_MODE_7) || 
	    (LED_MODE(pAd) == WPS_LED_MODE_8) || 
	    (LED_MODE(pAd) == WPS_LED_MODE_9) || 
	    (LED_MODE(pAd) == WPS_LED_MODE_11) ||
	    (LED_MODE(pAd) == WPS_LED_MODE_12)
#ifdef CONFIG_WIFI_LED_SUPPORT
		||(LED_MODE(pAd) == WPS_LED_MODE_SHARE)
#endif /* CONFIG_WIFI_LED_SUPPORT */
		)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: Support WPS LED mode (The WPS LED mode = %d).\n", 
			__FUNCTION__, LED_MODE(pAd)));
		return TRUE; /* Support WPS LED mode. */
	}
	else
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: Not support WPS LED mode (The WPS LED mode = %d).\n", 
			__FUNCTION__, LED_MODE(pAd)));
		return FALSE; /* Not support WPS LED mode. */
	}
}

BOOLEAN WscSupportWPSLEDMode10(
	IN PRTMP_ADAPTER pAd)
{
	if ((LED_MODE(pAd) == WPS_LED_MODE_10)){
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: Support WPS LED mode (The WPS LED mode = %d).\n", 
			__FUNCTION__, LED_MODE(pAd)));	
		return TRUE; /*Support WPS LED mode 10. */
	} 
	else
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: Not support WPS LED mode (The WPS LED mode = %d).\n", 
			__FUNCTION__, LED_MODE(pAd)));
		return FALSE; /* Not support WPS LED mode 10. */
	}
}

/* */
/* Whether the WPS AP has security setting or not. */
/* Note that this function is valid only after the WPS handshaking. */
/* */
BOOLEAN WscAPHasSecuritySetting(
	IN PRTMP_ADAPTER pAdapter,
	IN PWSC_CTRL     pWscControl)
{
	BOOLEAN bAPHasSecuritySetting = FALSE;
	UCHAR	currentIdx = MAIN_MBSSID;
	
#ifdef CONFIG_AP_SUPPORT
	currentIdx = (pWscControl->EntryIfIdx & 0x0F);
#endif /* CONFIG_AP_SUPPORT */
#ifdef CONFIG_STA_SUPPORT
	currentIdx = pWscControl->WscProfile.ApplyProfileIdx;
#endif /* CONFIG_STA_SUPPORT */

	switch (pWscControl->WscProfile.Profile[currentIdx].EncrType)
	{
		case WSC_ENCRTYPE_NONE:
		{
			bAPHasSecuritySetting = FALSE;
			break;
		}

		case WSC_ENCRTYPE_WEP:
		case WSC_ENCRTYPE_TKIP:
		case (WSC_ENCRTYPE_TKIP | WSC_ENCRTYPE_AES):
		case WSC_ENCRTYPE_AES:
		{
			bAPHasSecuritySetting = TRUE;
			break;
		}

		default:
		{
			MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: Incorrect encryption types (%d)\n", 
				__FUNCTION__, pWscControl->WscProfile.Profile[currentIdx].EncrType));
			ASSERT(FALSE);
			break;
		}
	}

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: WSC Entryption Type = %d\n", 
		__FUNCTION__, pWscControl->WscProfile.Profile[currentIdx].EncrType));

	return bAPHasSecuritySetting;
}


/* */
/* After the NIC connects with a WPS AP or not, */
/* the WscLEDTimer timer controls the LED behavior according to LED mode. */
/* */
VOID WscLEDTimer(
	IN PVOID	SystemSpecific1, 
	IN PVOID	FunctionContext, 
	IN PVOID	SystemSpecific2, 
	IN PVOID	SystemSpecific3) 
{
	PWSC_CTRL pWscControl = (PWSC_CTRL)FunctionContext;
	PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pWscControl->pAd;
	UCHAR WPSLEDStatus = 0;

	/* WPS LED mode 7, 8, 11 and 12. */
	if ((LED_MODE(pAd) == WPS_LED_MODE_7) || 
	    (LED_MODE(pAd) == WPS_LED_MODE_8) || 
	    (LED_MODE(pAd) == WPS_LED_MODE_11) ||
	    (LED_MODE(pAd) == WPS_LED_MODE_12))
	{
		WPSLEDStatus = LED_WPS_TURN_LED_OFF;
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: Turn off the WPS successful LED pattern.\n", __FUNCTION__));
	}
	else if ((LED_MODE(pAd) == WPS_LED_MODE_9) /* WPS LED mode 9. */
#ifdef CONFIG_WIFI_LED_SUPPORT
			|| (LED_MODE(pAd) == WPS_LED_MODE_SHARE)
#endif /* CONFIG_WIFI_LED_SUPPORT */
			)
	{
		switch (pWscControl->WscLEDMode) /* Last WPS LED state. */
		{

#ifdef CONFIG_WIFI_LED_SHARE
			case LED_WPS_PRE_STAGE:
				WPSLEDStatus = LED_WPS_IN_PROCESS;
				break;
			case LED_WPS_POST_STAGE:
				WPSLEDStatus = LED_WPS_TURN_LED_OFF;
				break;
#endif /* CONFIG_WIFI_LED_SHARE */

			/* Turn off the blue LED after 300 seconds. */
			case LED_WPS_SUCCESS:
#ifdef CONFIG_WIFI_LED_SHARE
				if (LED_MODE(pAd) == WPS_LED_MODE_SHARE)
				{
					WPSLEDStatus = LED_WPS_POST_STAGE;
					RTMPSetTimer(&pWscControl->WscLEDTimer, WSC_WPS_PREPOST_WIFI_LED_TIMEOUT);
				}
				else
#endif /* CONFIG_WIFI_LED_SHARE */
				WPSLEDStatus = LED_WPS_TURN_LED_OFF;

				/* Turn on/off the WPS success LED according to AP's encryption algorithm after one second. */
				RTMPSetTimer(&pWscControl->WscLEDTimer, WSC_WPS_TURN_OFF_LED_TIMEOUT);
				pWscControl->WscLEDTimerRunning = TRUE;

				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: LED_WPS_SUCCESS => LED_WPS_TURN_LED_OFF\n", __FUNCTION__));
				break;

			/* After turn off the blue LED for one second. */
			/* AP uses an encryption algorithm: */
			/* a) YES: Turn on the blue LED. */
			/* b) NO: Turn off the blue LED. */
			case LED_WPS_TURN_LED_OFF:
				if ((pWscControl->WscState == WSC_STATE_OFF) && 
				     (pWscControl->WscStatus == STATUS_WSC_CONFIGURED))
				{
					if (WscAPHasSecuritySetting(pAd, pWscControl) == TRUE) /* The NIC connects with an AP using an encryption algorithm. */
					{
						/* Turn WPS success LED. */
						WPSLEDStatus = LED_WPS_TURN_ON_BLUE_LED;
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: LED_WPS_TURN_LED_OFF => LED_WPS_TURN_ON_BLUE_LED\n", __FUNCTION__));
					}
					else /* The NIC connects with an AP using OPEN-NONE. */
					{
						/* Turn off the WPS LED. */
						WPSLEDStatus = LED_WPS_TURN_LED_OFF;
						MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: LED_WPS_TURN_LED_OFF => LED_WPS_TURN_LED_OFF\n", __FUNCTION__));
					}
				}
				break;
				
			/* Turn off the amber LED after 15 seconds. */
			case LED_WPS_ERROR:
#ifdef CONFIG_WIFI_LED_SHARE
				if (LED_MODE(pAd) == WPS_LED_MODE_SHARE)
				{
					WPSLEDStatus = LED_WPS_POST_STAGE;
					RTMPSetTimer(&pWscControl->WscLEDTimer, WSC_WPS_PREPOST_WIFI_LED_TIMEOUT);
				}
				else
#endif /* CONFIG_WIFI_LED_SHARE */
					WPSLEDStatus = LED_WPS_TURN_LED_OFF; /* Turn off the WPS LED. */
					
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: LED_WPS_ERROR/LED_WPS_SESSION_OVERLAP_DETECTED => LED_WPS_TURN_LED_OFF\n", __FUNCTION__));
				break;
			/* Turn off the amber LED after ~3 seconds. */
			case LED_WPS_SESSION_OVERLAP_DETECTED:
#ifdef CONFIG_WIFI_LED_SHARE
				if (LED_MODE(pAd) == WPS_LED_MODE_SHARE)
				{
					WPSLEDStatus = LED_WPS_POST_STAGE;
					RTMPSetTimer(&pWscControl->WscLEDTimer, WSC_WPS_PREPOST_WIFI_LED_TIMEOUT);
				}else
#endif /* CONFIG_WIFI_LED_SHARE */
					WPSLEDStatus = LED_WPS_TURN_LED_OFF; /* Turn off the WPS LED. */

				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: LED_WPS_SESSION_OVERLAP_DETECTED => LED_WPS_TURN_LED_OFF\n", __FUNCTION__));
				break;

			default:
				/* do nothing. */
				break;
		}

		if (WPSLEDStatus)
			RTMPSetLED(pAd, WPSLEDStatus);
	}
	else
	{
		/* do nothing. */
	}	
}


VOID WscSkipTurnOffLEDTimer(
	IN PVOID	SystemSpecific1, 
	IN PVOID	FunctionContext, 
	IN PVOID	SystemSpecific2, 
	IN PVOID	SystemSpecific3) 
{
	PWSC_CTRL pWscControl = (PWSC_CTRL)FunctionContext;

	/* Allow the NIC to turn off the WPS LED again. */
	pWscControl->bSkipWPSTurnOffLED = FALSE;
	
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: Allow the NIC to turn off the WPS LED again.\n", __FUNCTION__));
}

#endif /* WSC_LED_SUPPORT */

#ifdef CONFIG_AP_SUPPORT
VOID WscUpdatePortCfgTimeout(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3)
{
	WSC_CTRL *pWscControl = (WSC_CTRL *)FunctionContext;
	RTMP_ADAPTER *pAd = NULL;
	BOOLEAN	 bEnrollee = TRUE;
	WSC_CREDENTIAL *pCredential = NULL;
	BSS_STRUCT *pMbss = NULL;

	if (pWscControl == NULL)
		return;

	pCredential = (PWSC_CREDENTIAL) &pWscControl->WscProfile.Profile[0];
	pAd = (PRTMP_ADAPTER)pWscControl->pAd;

	if (pAd == NULL)
		return;
	
	pMbss = &pAd->ApCfg.MBSSID[pWscControl->EntryIfIdx & 0x0F];
	if (WscGetAuthMode(pCredential->AuthType) == pMbss->wdev.AuthMode &&
		WscGetWepStatus(pCredential->EncrType) == pMbss->wdev.WepStatus &&
		NdisEqualMemory(pMbss->Ssid, pCredential->SSID.Ssid, pCredential->SSID.SsidLength) &&
		NdisEqualMemory(pWscControl->WpaPsk, pCredential->Key, pCredential->KeyLength))
	{
		return;
	}

	if (pWscControl->WscProfile.ApplyProfileIdx & 0x8000)
		bEnrollee = FALSE;
	WscWriteConfToPortCfg(pAd,
						  pWscControl,
						  &pWscControl->WscProfile.Profile[0],
						  bEnrollee);
	pWscControl->WscProfile.ApplyProfileIdx &= 0x7FFF;
#ifdef P2P_SUPPORT
	if (pWscControl->EntryIfIdx & MIN_NET_DEVICE_FOR_P2P_GO)
	{
#ifdef RTMP_MAC_PCI
		OS_WAIT(1000);
#endif /* RTMP_MAC_PCI */
		P2P_GoStop(pAd);
		P2P_GoStartUp(pAd, MAIN_MBSSID);
	}
	else
#endif /* P2P_SUPPORT */
	{
		pAd->WriteWscCfgToDatFile = (pWscControl->EntryIfIdx & 0x0F);
		APStop(pAd);
		APStartUp(pAd);
	}

/*#ifdef KTHREAD_SUPPORT */
/*	WAKE_UP(&(pAd->wscTask)); */
/*#else */
/*	RTMP_SEM_EVENT_UP(&(pAd->wscTask.taskSema)); */
/*#endif */
	RtmpOsTaskWakeUp(&(pAd->wscTask));

	return;
}
#endif /* CONFIG_AP_SUPPORT */

VOID WscCheckPeerDPID(
	IN	PRTMP_ADAPTER	pAd,
	IN  PFRAME_802_11 	Fr,
	IN  PUCHAR			eid_data,
	IN  INT				eid_len)
{	
	WSC_IE		*pWscIE;
	PUCHAR		pData = NULL;
	INT			Len = 0;
	USHORT		DevicePasswordID;
	PWSC_CTRL	pWscCtrl = NULL;

	pData = eid_data + 4;
	Len = eid_len - 4;
	while (Len > 0)
	{
		WSC_IE	WscIE;
		NdisMoveMemory(&WscIE, pData, sizeof(WSC_IE));
		/* Check for WSC IEs*/
		pWscIE = &WscIE;

		/* Check for device password ID, PBC = 0x0004*/
		if (be2cpu16(pWscIE->Type) == WSC_ID_DEVICE_PWD_ID)
		{
			/* Found device password ID*/
			NdisMoveMemory(&DevicePasswordID, pData + 4, sizeof(DevicePasswordID));
			DevicePasswordID = be2cpu16(DevicePasswordID);
			if (DevicePasswordID == DEV_PASS_ID_PBC)	/* Check for PBC value*/
			{
				WscPBC_DPID_FromSTA(pAd, Fr->Hdr.Addr2);
				hex_dump("PBC STA:", Fr->Hdr.Addr2, MAC_ADDR_LEN);
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("\n"));
			}
			else if (DevicePasswordID == DEV_PASS_ID_PIN)
			{
#ifdef CONFIG_AP_SUPPORT
				if ((pAd->OpMode == OPMODE_AP)
#ifdef P2P_SUPPORT
					|| P2P_GO_ON(pAd)
#endif /* P2P_SUPPORT */
					)
				{
					UCHAR	ap_idx = 0;
					for (ap_idx = 0; ap_idx < pAd->ApCfg.BssidNum; ap_idx++)
					{
						if (NdisEqualMemory(Fr->Hdr.Addr1, pAd->ApCfg.MBSSID[ap_idx].wdev.bssid, MAC_ADDR_LEN))
							break;
					}

					if (ap_idx >= pAd->ApCfg.BssidNum)
					{
						break;
					}
		
					pWscCtrl = &pAd->ApCfg.MBSSID[ap_idx].WscControl;
				}
#endif /* CONFIG_AP_SUPPORT */

				/*
					WSC 2.0 STA will send probe request with WPS IE anyway.
					Do NOT add this STA to WscPeerList after AP is triggered to do PBC.
				*/
				if (pWscCtrl && 
					(!pWscCtrl->bWscTrigger || (pWscCtrl->WscMode != WSC_PBC_MODE)))
				{
					RTMP_SEM_LOCK(&pWscCtrl->WscPeerListSemLock);
					WscInsertPeerEntryByMAC(&pWscCtrl->WscPeerList, Fr->Hdr.Addr2);
					RTMP_SEM_UNLOCK(&pWscCtrl->WscPeerListSemLock);
				}
			}
#ifdef IWSC_SUPPORT
			else if (DevicePasswordID == DEV_PASS_ID_SMPBC)
			{
				IWSC_AddSmpbcEnrollee(pAd, Fr->Hdr.Addr2);
			}
#endif /* IWSC_SUPPORT */
			else {
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s : DevicePasswordID = 0x%04x\n",
							__FUNCTION__, DevicePasswordID));
			}
			break;
		}
		
#ifdef IWSC_SUPPORT
		if (pAd->StaCfg.BssType == BSS_ADHOC)
		{
			pWscCtrl = &pAd->StaCfg.WscControl;

			if ((be2cpu16(pWscIE->Type) == WSC_ID_CONFIG_METHODS) &&
				(pWscCtrl->WscConfMode == WSC_REGISTRAR) &&
				(pWscCtrl->bWscTrigger == TRUE))
			{
				USHORT PeerConfigMethod = 0;
				PWSC_PEER_ENTRY pWscPeerEntry = NULL;
				RTMP_SEM_LOCK(&pWscCtrl->WscConfiguredPeerListSemLock);
				pWscPeerEntry = WscFindPeerEntry(&pWscCtrl->WscConfiguredPeerList, Fr->Hdr.Addr2);
				if (pWscPeerEntry == NULL)
				{
					NdisMoveMemory(&PeerConfigMethod, pData + 4, sizeof(PeerConfigMethod));
					PeerConfigMethod = be2cpu16(PeerConfigMethod);
					if (pWscCtrl->WscMode == WSC_PIN_MODE)
					{
						NdisMoveMemory(pWscCtrl->WscPeerMAC, Fr->Hdr.Addr2, MAC_ADDR_LEN);
						NdisMoveMemory(pWscCtrl->EntryAddr, Fr->Hdr.Addr2, MAC_ADDR_LEN);
					}
					MlmeEnqueue(pAd, IWSC_STATE_MACHINE, IWSC_MT2_PEER_PROBE_REQ, sizeof(USHORT), &PeerConfigMethod, 0);
					RTMP_MLME_HANDLER(pAd);
					MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s(): Add this peer: %02x:%02x:%02x:%02x:%02x:%02x\n",
						__FUNCTION__, PRINT_MAC(Fr->Hdr.Addr2)));	
				}
				RTMP_SEM_UNLOCK(&pWscCtrl->WscConfiguredPeerListSemLock);
				MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s() : PeerConfigMethod = 0x%04x\n", 
							__FUNCTION__, PeerConfigMethod));
			}
		}
#endif /* IWSC_SUPPORT */
		
		/* Set the offset and look for PBC information*/
		/* Since Type and Length are both short type, we need to offset 4, not 2*/
		pData += (be2cpu16(pWscIE->Length) + 4);
		Len   -= (be2cpu16(pWscIE->Length) + 4);
	}
}

VOID	WscClearPeerList(
	IN  PLIST_HEADER	pWscEnList)
{
	RT_LIST_ENTRY *pEntry = NULL;

	pEntry = pWscEnList->pHead;

	while (pEntry != NULL)
	{		
		removeHeadList(pWscEnList);
		os_free_mem(NULL, pEntry);
		pEntry = pWscEnList->pHead;
	}
	
	return;
}

PWSC_PEER_ENTRY	WscFindPeerEntry(
	PLIST_HEADER		pWscEnList,
	IN	PUCHAR			pMacAddr)
{
	PWSC_PEER_ENTRY 	pPeerEntry = NULL;
	RT_LIST_ENTRY *pListEntry = NULL;

	pListEntry = pWscEnList->pHead;
	pPeerEntry = (PWSC_PEER_ENTRY)pListEntry;
	while (pPeerEntry != NULL)
	{
		if (NdisEqualMemory(pPeerEntry->mac_addr, pMacAddr, MAC_ADDR_LEN))
			return pPeerEntry;
		pListEntry = pListEntry->pNext;
		pPeerEntry = (PWSC_PEER_ENTRY)pListEntry;
	}
	
	return NULL;
}

VOID	WscInsertPeerEntryByMAC(
	PLIST_HEADER		pWscEnList,
	IN	PUCHAR			pMacAddr)
{
	PWSC_PEER_ENTRY		pWscPeer = NULL;

	pWscPeer = WscFindPeerEntry(pWscEnList, pMacAddr);
	if (pWscPeer)
	{
		NdisGetSystemUpTime(&pWscPeer->receive_time);
	}
	else
	{
		os_alloc_mem(NULL, (UCHAR **)&pWscPeer, sizeof(WSC_PEER_ENTRY));
		if (pWscPeer)
		{
			NdisZeroMemory(pWscPeer, sizeof(WSC_PEER_ENTRY));
			pWscPeer->pNext = NULL;
			NdisMoveMemory(pWscPeer->mac_addr, pMacAddr, MAC_ADDR_LEN);
			NdisGetSystemUpTime(&pWscPeer->receive_time);
			insertTailList(pWscEnList, (RT_LIST_ENTRY *)pWscPeer);
		}
		ASSERT(pWscPeer != NULL);
	}
}

#ifdef CONFIG_AP_SUPPORT
INT WscApShowPeerList(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	UCHAR				ApIdx = 0;
	PWSC_CTRL 			pWscControl = NULL;
	PWSC_PEER_ENTRY 	pPeerEntry = NULL;
	RT_LIST_ENTRY *pListEntry = NULL;
	PLIST_HEADER		pWscEnList = NULL;

	for (ApIdx = 0; ApIdx < pAd->ApCfg.BssidNum; ApIdx++)
	{
		pWscControl = &pAd->ApCfg.MBSSID[ApIdx].WscControl;
		pWscEnList = &pWscControl->WscPeerList;

		if (pWscEnList->size != 0)
		{
			WscMaintainPeerList(pAd, pWscControl);
			RTMP_SEM_LOCK(&pWscControl->WscPeerListSemLock);
			pListEntry = pWscEnList->pHead;
			pPeerEntry = (PWSC_PEER_ENTRY)pListEntry;
			while (pPeerEntry != NULL)
			{
				printk("MAC:%02x:%02x:%02x:%02x:%02x:%02x\tReveive Time:%lu\n", 					
							pPeerEntry->mac_addr[0],
							pPeerEntry->mac_addr[1],
							pPeerEntry->mac_addr[2],
							pPeerEntry->mac_addr[3],
							pPeerEntry->mac_addr[4],
							pPeerEntry->mac_addr[5],
							pPeerEntry->receive_time);
				pListEntry = pListEntry->pNext;
				pPeerEntry = (PWSC_PEER_ENTRY)pListEntry;
			}
			RTMP_SEM_UNLOCK(&pWscControl->WscPeerListSemLock);
		}
		printk("\n");
	}
	
	return TRUE;
}
#endif // CONFIG_AP_SUPPORT //

#ifdef CONFIG_STA_SUPPORT
INT WscStaShowPeerList(RTMP_ADAPTER *pAd, RTMP_STRING *arg)
{
	PWSC_CTRL 			pWscControl = NULL;
	PWSC_PEER_ENTRY 	pPeerEntry = NULL;
	RT_LIST_ENTRY *pListEntry = NULL;
	PLIST_HEADER		pWscEnList = NULL;

	pWscControl = &pAd->StaCfg.WscControl;
	pWscEnList = &pWscControl->WscPeerList;

	RTMP_SEM_LOCK(&pWscControl->WscPeerListSemLock);
	if (pWscEnList->size != 0)
	{
		RTMP_SEM_UNLOCK(&pWscControl->WscPeerListSemLock);
		WscMaintainPeerList(pAd, pWscControl);
		RTMP_SEM_LOCK(&pWscControl->WscPeerListSemLock);
		pListEntry = pWscEnList->pHead;
		pPeerEntry = (PWSC_PEER_ENTRY)pListEntry;
		while (pPeerEntry != NULL)
		{
			printk("MAC:%02x:%02x:%02x:%02x:%02x:%02x\tReveive Time:%lu\n", 					
						pPeerEntry->mac_addr[0],
						pPeerEntry->mac_addr[1],
						pPeerEntry->mac_addr[2],
						pPeerEntry->mac_addr[3],
						pPeerEntry->mac_addr[4],
						pPeerEntry->mac_addr[5],
						pPeerEntry->receive_time);
			pListEntry = pListEntry->pNext;
			pPeerEntry = (PWSC_PEER_ENTRY)pListEntry;
		}
	}
	RTMP_SEM_UNLOCK(&pWscControl->WscPeerListSemLock);
	printk("\n");
	
	return TRUE;
}
#endif // CONFIG_STA_SUPPORT //

VOID	WscMaintainPeerList(
	IN  PRTMP_ADAPTER	pAd,
	IN  PWSC_CTRL		pWpsCtrl)
{
	PWSC_PEER_ENTRY 	pPeerEntry = NULL;
	RT_LIST_ENTRY *pListEntry = NULL, *pTempListEntry = NULL;
	PLIST_HEADER		pWscEnList = NULL;
	ULONG				now_time = 0;

	RTMP_SEM_LOCK(&pWpsCtrl->WscPeerListSemLock);
	pWscEnList = &pWpsCtrl->WscPeerList;


	NdisGetSystemUpTime(&now_time);
	pListEntry = pWscEnList->pHead;
	pPeerEntry = (PWSC_PEER_ENTRY)pListEntry;

	while (pPeerEntry != NULL)
	{
		if (RTMP_TIME_AFTER(now_time, pPeerEntry->receive_time + (30 * OS_HZ)))
		{
			pTempListEntry = pListEntry->pNext;
			delEntryList(pWscEnList, pListEntry);
			os_free_mem(pAd, pPeerEntry);
			pListEntry = pTempListEntry;
		}
		else
			pListEntry = pListEntry->pNext;
		pPeerEntry = (PWSC_PEER_ENTRY)pListEntry;
	}
	
	RTMP_SEM_UNLOCK(&pWpsCtrl->WscPeerListSemLock);
	return;
}

VOID	WscDelListEntryByMAC(
	PLIST_HEADER		pWscEnList,
	IN  PUCHAR			pMacAddr)
{
	RT_LIST_ENTRY *pListEntry = NULL;
	pListEntry = (RT_LIST_ENTRY *)WscFindPeerEntry(pWscEnList, pMacAddr);
	if (pListEntry)
	{
		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("WscDelListEntryByMAC : pMacAddr = %02X:%02X:%02X:%02X:%02X:%02X\n", PRINT_MAC(pMacAddr)));
		delEntryList(pWscEnList, pListEntry);		
		os_free_mem(NULL, pListEntry);
	}
}

VOID	WscAssignEntryMAC(
	IN  PRTMP_ADAPTER	pAd,
	IN  PWSC_CTRL		pWpsCtrl)
{
	PWSC_PEER_ENTRY pPeerEntry = NULL;

	WscMaintainPeerList(pAd, pWpsCtrl);

	RTMP_SEM_LOCK(&pWpsCtrl->WscPeerListSemLock);
	pPeerEntry = (PWSC_PEER_ENTRY)pWpsCtrl->WscPeerList.pHead;

	NdisZeroMemory(pWpsCtrl->EntryAddr, MAC_ADDR_LEN);
	if (pPeerEntry)
	{
		NdisMoveMemory(pWpsCtrl->EntryAddr, pPeerEntry->mac_addr, MAC_ADDR_LEN);
	}
	RTMP_SEM_UNLOCK(&pWpsCtrl->WscPeerListSemLock);
}

#ifdef DPA_T
INT WscProfilePriorityVal(
	IN	USHORT	auth,
	IN  USHORT	encr)
{
	if ((auth & WSC_AUTHTYPE_WPA2PSK) &&
		(encr & WSC_ENCRTYPE_AES))
		return 4;
	else if ((auth & WSC_AUTHTYPE_WPAPSK) &&
		(encr & WSC_ENCRTYPE_AES))
		return 3;
	else if ((auth & WSC_AUTHTYPE_WPAPSK) &&
		(encr & WSC_ENCRTYPE_TKIP))
		return 2;
	else if ((auth & WSC_AUTHTYPE_WPA2PSK) &&
		(encr & WSC_ENCRTYPE_TKIP))
		return 1;
	else
		return 0;
}

VOID WscSortProfile(
	IN	PRTMP_ADAPTER	pAd,
	IN  PWSC_CTRL       pWpsCtrl)
{
	PWSC_PROFILE	pProfile = NULL;
	WSC_CREDENTIAL	TmpCredential;
	INT				i, j, v1, v2;
	pProfile = &pWpsCtrl->WscProfile;
	if (pProfile->ProfileCnt > 1)
	{
		for (i = 0; i < pProfile->ProfileCnt; i++)
		{
			for (j = (i+1); j < pProfile->ProfileCnt; j++)
			{
				v1 = WscProfilePriorityVal(pProfile->Profile[i].AuthType, pProfile->Profile[i].EncrType);
				v2 = WscProfilePriorityVal(pProfile->Profile[j].AuthType, pProfile->Profile[j].EncrType);
				if (v2 > v1)
				{
					RTMPMoveMemory(&TmpCredential, &pProfile->Profile[i], sizeof(WSC_CREDENTIAL));
					RTMPMoveMemory(&pProfile->Profile[i], &pProfile->Profile[j], sizeof(WSC_CREDENTIAL));
					RTMPMoveMemory(&pProfile->Profile[j], &TmpCredential, sizeof(WSC_CREDENTIAL));
				}
			}
		}
	}
}
#endif /* DPA_T */

/*
	Get WSC IE data from WSC Peer by Tag.
*/
BOOLEAN WscGetDataFromPeerByTag(
    IN  PRTMP_ADAPTER 	pAd, 
    IN  PUCHAR			pIeData,
    IN  INT				IeDataLen,
    IN  USHORT			WscTag,
    OUT PUCHAR			pWscBuf,
    OUT PUSHORT			pWscBufLen)
{
	PUCHAR				pData = pIeData;
	INT					Len = 0;
	USHORT				DataLen = 0;
	PWSC_IE				pWscIE;

	Len = IeDataLen;

	while (Len > 0)
	{
		WSC_IE	WscIE;
		NdisMoveMemory(&WscIE, pData, sizeof(WSC_IE));
		// Check for WSC IEs
		pWscIE = &WscIE;
		
		if (be2cpu16(pWscIE->Type) == WscTag)
		{
			DataLen = be2cpu16(pWscIE->Length);
			if (pWscBufLen)
				*pWscBufLen = DataLen;
			NdisMoveMemory(pWscBuf, pData + 4, DataLen);
			return TRUE;
		}
		
		// Set the offset and look for next WSC Tag information
		// Since Type and Length are both short type, we need to offset 4, not 2
		pData += (be2cpu16(pWscIE->Length) + 4);
		Len   -= (be2cpu16(pWscIE->Length) + 4);
	}

    return FALSE;
}

#ifdef MT_MAC
VOID WscEapReqIdRetryTimeout(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3)
{
	MAC_TABLE_ENTRY *pEntry = (MAC_TABLE_ENTRY *)FunctionContext;

    if ((pEntry) 
		&& IS_ENTRY_CLIENT(pEntry) 
		&& pEntry->bEapReqIdRetryTimerRunning)
    {
        PRTMP_ADAPTER pAd = (PRTMP_ADAPTER)pEntry->pAd;

		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: re-send EapReqId again.\n", __FUNCTION__));
		WscSendEapReqId(pAd, pEntry, AP_MODE);
    }
	else
	{

		MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("%s: cancel this timer.\n", __FUNCTION__));
		pEntry->EapReqIdRetryTimer.PeriodicType = FALSE;
		pEntry->EapReqIdRetryTimer.Repeat = FALSE;
	}
	
	return;
}

#endif /* MT_MAC */

//add by mjh 
void mjh_wsc_handler(void)
{
	WSC_HDR_BTN_CheckHandler(pAd_mjh);	
}

int is_connect(void)
{	
	char signal;
	POS_COOKIE pObj = (POS_COOKIE)pAd_mjh->OS_Cookie;
	UCHAR ifIdx;
	ifIdx = pObj->ioctl_if;
	if(pAd_mjh->ApCfg.ApCliTab[ifIdx].CtrlCurrState == APCLI_CTRL_CONNECTED)
	{
		printf("connect : %s\n",pAd_mjh->ApCfg.ApCliTab[0].CfgSsid);
		//signal=get_singal(pAd_mjh->ApCfg.ApCliTab[0].CfgSsid);
		signal=get_singal(pAd_mjh->ApCfg.ApCliTab[ifIdx].CfgSsid);
		//led_mode();

		//led_control(signal);
		//printf("signal = %d",signal);
		return 1;
	}
	else
	{
		extern int CmdIwpriv(int argc, char *argv[]);
		{	char *string[]={
			"apcli0",
			"set",
			"ApCliAutoConnect=1"
			};
			CmdIwpriv(3,string);
			cyg_thread_delay(10);
		}
		disconnect();
		//printf("not connect\n");
		return 0;
	}	
}

extern char mjh_wsc_flag;
extern char mjh_wsc_link;  //ap_apcli.c
extern char mjh_wsc_fail;
extern char mjh_2_out;
//wsc led
extern int mjh_i;

//wsc key
void wsc_do(void)
{
	printf("wsc thread\n");
	while(1)
	{
	if(mjh_wsc_flag)
	{
		led_flash();
		if(mjh_wsc_link == 1)
		{
			mjh_i = 0;
			mjh_wsc_flag = 0; //clear falsh flag
				
		}
		if(mjh_wsc_fail == 1)
		{
			mjh_i = 0;
			mjh_wsc_flag = 0; //clear falsh flag
			mjh_wsc_link = 0;  
			led_wps(1);  //led oFF
		}
		/*
		else if( (pWscControl->WscStatus == STATUS_WSC_CONFIGURED) ||
		(pWscControl->WscStatus == STATUS_WSC_FAIL) ||
		(pWscControl->WscStatus == STATUS_WSC_PBC_TOO_MANY_AP)  ||
		(pWscControl->WscStatus == STATUS_WSC_NOTUSED)    ||
		(pWscControl->WscState == WSC_STATE_OFF))
			led_wps(1);	
		*/	
	}
	else
	{	
			
		mjh_i = 0;
		mjh_wsc_flag = 0;
		if(mjh_wsc_link == 1 && pAd_mjh->ApCfg.ApCliTab[0].CtrlCurrState == APCLI_CTRL_CONNECTED) //first link up //apcli0 connect //wsc ssid  web yao rebbot tiaojian fail
			led_wps(0);  //led on
		else if(mjh_wsc_link == 1 && mjh_2_out == 0)
			led_flash();
		else if(mjh_wsc_fail == 1 || mjh_2_out == 1)
			led_wps(1);
		else
			led_wps(1);  //led off	
	}
	mjh_wsc_handler();
	}
		
}

#endif /* WSC_INCLUDED */
