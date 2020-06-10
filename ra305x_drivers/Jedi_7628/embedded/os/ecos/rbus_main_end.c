/****************************************************************************
 * Ralink Tech Inc.
 * Taiwan, R.O.C.
 *
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

    Module Name:
    rbus_main_end.c

    Abstract:
    Create and register network interface for RBUS based chipsets in eCos platform.

    Revision History:
    Who         When            What
    --------    ----------      ------------------------------------------
*/

#include "rt_config.h"

#define MAX_ETH_FRAME_SIZE 1520
#define PRIORITY_HIGH   0 /* the highest irq priorit */


static cyg_interrupt rtmp_wlan_interrupt;
static cyg_handle_t  rtmp_wlan_interrupt_handle;

struct mt_dev_priv *rt_ecos_priv_data;

ETH_DRV_SC(devive_wireless_sc0,
           &rt_ecos_priv_data,  /* Driver specific data */
           INF_MAIN_DEV_NAME "0",
           rt_ecos_start,
           rt_ecos_stop,
           rt_ecos_control,
           rt_ecos_can_send,
           rt_ecos_send,
           rt_ecos_recv,
           rt_ecos_deliver,
           rt_ecos_poll,
           rt_ecos_int_vector
           );

NETDEVTAB_ENTRY(devive_wireless_netdev0, 
                INF_MAIN_DEV_NAME "0",
                rt_ecos_init,
                &devive_wireless_sc0);

/*
 *  Interrupt handling - ISR and DSR
 */
static cyg_uint32 rt_wlan_isr(cyg_vector_t vector, cyg_addrword_t data)
{
    rt2860_interrupt((void *) data);
   	cyg_interrupt_acknowledge(vector);
   	return CYG_ISR_CALL_DSR;
}

static void rt_wlan_dsr(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
	/*  schecdule deliver routine  */
	if (data != 0)
		eth_drv_dsr(vector, count, (cyg_addrword_t) data);
    else
    	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("rt_wlan_dsr: data is NULL\n"));
}


static bool rt_ecos_init(struct cyg_netdevtab_entry *tab)
{
	PRTMP_ADAPTER		pAd = NULL;
	PNET_DEV 			pNetDev = NULL;
	POS_COOKIE			pOSCookie = NULL;
	ULONG				csr_addr;
    UINT8				MacAddr[6];
	unsigned int			dev_irq;
    //USHORT 				value;	
    UINT32 Value;
	int					status;
	struct mt_dev_priv 	*priv_info = NULL;
	

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("===> rt_ecos_init()\n"));

    /* Important: Check Kernel Configure */
    if (sizeof(ECOS_PKT_BUFFER) > MLEN)
    {
		DBGPRINT_ERR(("%s: The size of structure ECOS_PKT_BUFFER > MLEN\n", __FUNCTION__));
        goto err_out;
    }
    if (MCLBYTES < RX_BUFFER_AGGRESIZE)
    {
		DBGPRINT_ERR(("%s: The size of m_buf clusters (%d) is too small\n", __FUNCTION__, MCLBYTES));
        goto err_out;
    }

	/*RtmpRaBusInit============================================ */
	/* map physical address to virtual address for accessing register */
	csr_addr = (unsigned long)RTMP_MAC_CSR_ADDR;
	dev_irq = RTMP_MAC_IRQ_NUM;

	pNetDev = (struct eth_drv_sc *)tab->device_instance;

	/* Allocate RTMP_ADAPTER adapter structure */
	pOSCookie = (POS_COOKIE) kmalloc(sizeof(struct os_cookie), GFP_ATOMIC);
	if (!pOSCookie)
	{
		DBGPRINT_ERR(("%s: allocate memory for os_cookie failed!\n", __FUNCTION__));
		goto err_out;
	}
	NdisZeroMemory((PUCHAR) pOSCookie, sizeof(struct os_cookie));
    
	status = RTMPAllocAdapterBlock((PVOID) pOSCookie, (VOID **)&pAd);
	if (status != NDIS_STATUS_SUCCESS)
	{
		kfree(pOSCookie);
		goto err_out;
	}
	/* Here are the RTMP_ADAPTER structure with rbus-bus specific parameters. */
	pAd->CSRBaseAddress = (PUCHAR) csr_addr;
	
#ifdef MT7628
		pAd->MACVersion = 0x76280000;
		pAd->chipCap.hif_type = HIF_MT;
		pAd->infType = RTMP_DEV_INF_RBUS;
	
		RTMP_IO_READ32(pAd, TOP_HVR, &Value);
		pAd->HWVersion = Value;
	
		RTMP_IO_READ32(pAd, TOP_FVR, &Value);
		pAd->FWVersion = Value;
	
		RTMP_IO_READ32(pAd, TOP_HCR, &Value);
		pAd->ChipID = Value;
	
	
		if (IS_MT7628(pAd))
		{
			RTMP_IO_READ32(pAd, STRAP_STA, &Value);
			pAd->AntMode = (Value >> 24) & 0x1;
		}
#endif
	diag_printf("%s:%d\n",__FUNCTION__,__LINE__);
	RtmpRaDevCtrlInit(pAd, RTMP_DEV_INF_RBUS);

	/*NetDevInit============================================== */
	priv_info = (struct mt_dev_priv *) kmalloc(sizeof(struct mt_dev_priv), GFP_ATOMIC);
	pAd->net_dev = pNetDev;
	pNetDev->driver_private = priv_info;
	pOSCookie->pci_dev = pNetDev;
	
	/* put private data structure */
	RTMP_OS_NETDEV_SET_PRIV(pNetDev, pAd);
	RTMP_DRIVER_NET_DEV_SET(pAd, pNetDev);

	RTMP_DRIVER_MAIN_INF_CREATE(pAd, &pNetDev);
	//comment:has not assign pAd->chipOps.eeinit here. pAd->chipOps.eeinit assign in rt28xx_init call RtmpChipOpsEepromHook()
	if (pAd->chipOps.eeinit)
	{
		pAd->chipOps.eeinit(pAd);
		diag_printf("%s()->pAd->chipOps.eeinit",__func__);
	}	

        /* get MAC address */
		//get_mac_from_eeprom(pAd, &MacAddr[0]);//before eeprom write hook init,so driver can't read mac
		CFG_get_mac(2,&MacAddr[0]);

	/* Set up to handle interrupts */
	cyg_interrupt_create(RTMP_INTERRUPT_INIC,
                        PRIORITY_HIGH,
                        (cyg_addrword_t)pAd->net_dev,
                        (cyg_ISR_t *)rt_wlan_isr,
                        (cyg_DSR_t *)rt_wlan_dsr,
                        &rtmp_wlan_interrupt_handle,
                        &rtmp_wlan_interrupt);
	cyg_interrupt_attach(rtmp_wlan_interrupt_handle);
	cyg_interrupt_configure(RTMP_INTERRUPT_INIC, 1, 0);
	cyg_interrupt_unmask(RTMP_INTERRUPT_INIC);
        
	/* Initialize upper level driver */
	(pNetDev->funs->eth_drv->init)(pNetDev, MacAddr);

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<=== rt_ecos_init()\n"));

	return true;

err_out:
	return false;
}

static void rt_ecos_start(struct eth_drv_sc *sc, unsigned char *enaddr, int flags)
{
        PNET_DEV		    pNetDev = NULL;
	POS_COOKIE			pOSCookie = NULL;    
	PRTMP_ADAPTER	    pAd = NULL;
        char                devName[IFNAMSIZ];
        int                 i, value;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("===> rt_ecos_start()\n"));

        /* if the device is runnung, do nothing */
        if (sc->state & ETH_DRV_STATE_ACTIVE)
                goto exit;

	pNetDev = sc;
	pAd = (PRTMP_ADAPTER) RtmpOsGetNetDevPriv(sc);	

	if (pAd == NULL) {
		DBGPRINT_ERR(("%s: pAd is NULL.\n", __FUNCTION__));
                goto exit;
        }
	
	pOSCookie = (POS_COOKIE) pAd->OS_Cookie;

        i = 0;
#ifdef APCLI_SUPPORT
        for (i = 0; i < MAX_APCLI_NUM; i++)
        {
                sprintf(devName, "%s%d", INF_APCLI_DEV_NAME, i);
                if (strcmp(pNetDev->dev_name, devName) == 0)
                {
                        ApCli_VirtualIF_Open(sc);
                        return;
                }
        }
#endif /* APCLI_SUPPORT */
    
#ifdef WDS_SUPPORT
        for (i = 0; i < MAX_WDS_ENTRY; i++)
        {
                sprintf(devName, "%s%d", INF_WDS_DEV_NAME, i);
                if (strcmp(pNetDev->dev_name, devName) == 0)
                {
                        WdsVirtualIF_open(sc);
                        return;
                }
        }
#endif /* WDS_SUPPORT */

#ifdef MBSS_SUPPORT
        for (i = 1; i < pAd->ApCfg.BssidNum; i++)
        {
                sprintf(devName, "%s%d", INF_MBSSID_DEV_NAME, i);
                if (strcmp(pNetDev->dev_name, devName) == 0)
                {
                        MBSS_VirtualIF_Open(sc);
                        return;
                }
        }
#endif /* MBSS_SUPPORT */

        sprintf(devName, "%s0", INF_MAIN_DEV_NAME);
        if (strcmp(pNetDev->dev_name, devName) != 0)
        {
		DBGPRINT_ERR(("%s: device name(%s) is not found\n", __FUNCTION__, pNetDev->dev_name));
                goto exit;    
        }
		
#ifdef CONFIG_AP_SUPPORT
	RTMP_DRIVER_AP_MAIN_OPEN(pAd);
#endif
//SHT To Do for STA

	if (VIRTUAL_IF_UP(pAd) != 0)
		return;

	/* increase MODULE use count */
	RT_MOD_INC_USE_COUNT();

#ifdef RTMP_RESET_WPS_SHARE_PIN_SUPPORT
#ifdef MT7628
/*Set WPS and Reset to default pin to GPIO Mode*/  
	value = HAL_REG32(RTMP_SYS_CTL_ADDR+60);
	value &= ~(3<<24);
	value |=(1<<24);
	HAL_REG32(RTMP_SYS_CTL_ADDR+60)= value;		
	//pAd->CommonCfg.RestoreHdrBtnFlag = TRUE;
/*For WIFI LED*/
	value = HAL_SYSCTL_REG(0x64) ;
	value &= ~0x00010001;
	HAL_SYSCTL_REG(0X64)= value;
#endif


#ifdef WSC_INCLUDED
        /* Polling WPS button in APSOC */
	WSC_HDR_BTN_MR_HDR_SUPPORT_SET(pAd, 1);
#endif /* WSC_INCLUDED */

#else /*RTMP_RESET_WPS_SHARE_PIN_SUPPORT*/

#ifdef PLATFORM_BUTTON_SUPPORT
    /* Polling reset button in APSOC */
#ifdef RT5350
	value = HAL_REG32(RTMP_SYS_CTL_ADDR + RTMP_SYS_GPIOMODE_OFFSET);
	value |= 0x001c;
	HAL_REG32(RTMP_SYS_CTL_ADDR + RTMP_SYS_GPIOMODE_OFFSET) = value;
	value = HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO2100_POL_OFFSET);
	value &= 0xfffe;
	HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO2100_POL_OFFSET) = value;
	pAd->CommonCfg.RestoreHdrBtnFlag = TRUE;
#endif

#ifdef RT6352
	value = HAL_REG32(RTMP_SYS_CTL_ADDR + RTMP_SYS_GPIOMODE_OFFSET);
	value |= 0x0001;
	value &= 0xffffdfff; //For WIFI LED
        HAL_REG32(RTMP_SYS_CTL_ADDR + RTMP_SYS_GPIOMODE_OFFSET) = value;
        value = HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO2100_POL_OFFSET);
	value &= 0xfdf9;
	HAL_REG32(RTMP_PIO_CTL_ADDR + RTMP_PIO2100_POL_OFFSET) = value;
	pAd->CommonCfg.RestoreHdrBtnFlag = TRUE;
#endif

#ifdef MT7628
/*Set WPS and Reset to default pin to GPIO Mode*/  
	value = HAL_REG32(RTMP_SYS_CTL_ADDR+60);
	value &= ~(3<<24);
	value |=(1<<24);
	HAL_REG32(RTMP_SYS_CTL_ADDR+60)= value;	
	pAd->CommonCfg.RestoreHdrBtnFlag = TRUE;
/*For WIFI LED*/
	value = HAL_SYSCTL_REG(0x64) ;
	value &= ~0x00010001;
	HAL_SYSCTL_REG(0X64)= value;
#endif


#ifdef WSC_INCLUDED
        /* Polling WPS button in APSOC */
	WSC_HDR_BTN_MR_HDR_SUPPORT_SET(pAd, 1);
#endif /* WSC_INCLUDED */
#endif /* PLATFORM_BUTTON_SUPPORT */
#endif /*RTMP_RESET_WPS_SHARE_PIN_SUPPORT*/

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<=== rt_ecos_start()\n"));

exit:
    return;
}

static void  rt_ecos_stop(struct eth_drv_sc *sc)
{
	INT BssId, i;
	PNET_DEV		pNetDev = NULL;
	PRTMP_ADAPTER   pAd = NULL;
	POS_COOKIE		pOSCookie = NULL;
	char devName[IFNAMSIZ];
    
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("===> rt_ecos_stop()\n"));

    /* if the device is not runnung, do nothing */
    if ((sc->state & ETH_DRV_STATE_ACTIVE) == 0)
        return;
    
	pNetDev = sc;
	pAd = (PRTMP_ADAPTER) RtmpOsGetNetDevPriv(pNetDev);
	if (pAd == NULL)
		return;
	
	pOSCookie = (POS_COOKIE) pAd->OS_Cookie;

	MlmeRadioOn(pAd);

#ifdef APCLI_SUPPORT
    sprintf(devName, "%s0", INF_APCLI_DEV_NAME);
    if (strcmp(pNetDev->dev_name, devName) == 0)
    {
        ApCli_VirtualIF_Close(sc);
        return;
    }
#endif /* APCLI_SUPPORT */

#ifdef MBSS_SUPPORT
	for (i = 1; i < pAd->ApCfg.BssidNum; i++)
	{
		sprintf(devName, "%s%d", INF_MBSSID_DEV_NAME, i);
		if (strcmp(pNetDev->dev_name, devName) == 0)
		{
			MBSS_VirtualIF_Close(sc);
			return;
		}
	}
	pAd->FlgMbssInit = FALSE;
#endif /* MBSS_SUPPORT */

	RTMPInfClose(pAd);
	VIRTUAL_IF_DOWN(pAd);

	RT_MOD_DEC_USE_COUNT();
    
	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<=== rt_ecos_stop()\n"));  
}

static int rt_ecos_control(struct eth_drv_sc *sc, 
                          unsigned long cmd, 
                          void *data, 
                          int len)
{
	POS_COOKIE		 pOSCookie = NULL;    
	PRTMP_ADAPTER	 pAd = NULL;
	char devName[IFNAMSIZ];

	DBGPRINT(RT_DEBUG_INFO, ("rt_ecos_control()\n"));	
	pAd = (PRTMP_ADAPTER) RtmpOsGetNetDevPriv(sc);	
	if (pAd == NULL)
	    return;

	pOSCookie = (POS_COOKIE) pAd->OS_Cookie;
	switch(cmd)
	{
		case SIOCGIFPHY:
		{			
#ifdef APCLI_SUPPORT			
			INT i=0;
			UCHAR ifIndex;
			struct ifreq *ifr = (struct ifreq *)data;
			unsigned int *p = (unsigned int *)ifr->ifr_ifru.ifru_data;
			*p = 0;

			sprintf(devName, "%s0", INF_APCLI_DEV_NAME);
			if (strcmp(sc->dev_name, devName) == 0)
			{
				pOSCookie->ioctl_if_type = INT_APCLI;
				pOSCookie->ioctl_if = 0;
				ifIndex = pOSCookie->ioctl_if;
				
				if((pAd->ApCfg.ApCliTab[ifIndex].CtrlCurrState == APCLI_CTRL_CONNECTED)
					&& (pAd->ApCfg.ApCliTab[ifIndex].SsidLen != 0))
				{
					for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
					{
						PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];
				
						if ( IS_ENTRY_APCLI(pEntry)
							&& (pEntry->Sst == SST_ASSOC)
							&& (pEntry->wdev->PortSecured == WPA_802_1X_PORT_SECURED))
							{	
								*p = 1;
							}
					}
				}
			}				
#endif /* APCLI_SUPPORT */
			break;
		}

	}
	
	return 0;
}

static int rt_ecos_can_send(struct eth_drv_sc *sc)
{
    return 1;
}

//Copy from eCos kernel, remove the eth_drv_send(ifp);
static void rt_eth_drv_tx_done(struct eth_drv_sc *sc, CYG_ADDRESS key, int status)
{
    struct ifnet *ifp = &sc->sc_arpcom.ac_if;
    struct mbuf *m0 = (struct mbuf *)key;
    CYGARC_HAL_SAVE_GP();

    // Check for errors here (via 'status')
    ifp->if_opackets++;
    // Done with packet

    // Guard against a NULL return - can be caused by race conditions in
    // the driver, this is the neatest fixup:
    if (m0) { 
//        mbuf_key = m0;
        m_freem(m0);
    }
    // Start another if possible
//    eth_drv_send(ifp); 
    CYGARC_HAL_RESTORE_GP();
}

static char rt_convert_sglist_to_mbuf_check_point_counter = 0;
static PECOS_PKT_BUFFER rt_convert_sglist_to_mbuf(struct eth_drv_sc *sc,
				struct eth_drv_sg *sg_list,
				int sg_len,
				int total_len,
				unsigned long key)
{
	PRTMP_ADAPTER		pAd = NULL;
	PECOS_PKT_BUFFER	pPacket = NULL;
	NDIS_STATUS			Status;
	INT					packetLength = 0;
	BOOLEAN				isFreeSG = TRUE;
        struct mbuf         *pMBuf = NULL;

        MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("==>Convert_sglist_to_mbuf()\n"));

	if (total_len >= MAX_ETH_FRAME_SIZE) {
                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s:total_len >= MAX_ETH_FRAME_SIZE\n", __FUNCTION__));
                isFreeSG = TRUE;
                goto FREE_SG;
        }

	pAd = (PRTMP_ADAPTER) RtmpOsGetNetDevPriv(sc);
        pMBuf = (struct mbuf *) key;
        if (sg_len == 2)
        {
                struct mbuf *pMBuf2 = pMBuf->m_next;

                if ((pMBuf2->m_flags & M_EXT) && ((pMBuf2->m_data - pMBuf2->m_ext.ext_buf) > sg_list[0].len))
                {
                        Status = RTMP_AllocateNdisPacket_AppandMbuf(pAd, (PNDIS_PACKET *)&pPacket, pMBuf2);
                        if (Status != NDIS_STATUS_SUCCESS)
                        {
                                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s:can't allocate NDIS PACKET AppandMbuf\n", __FUNCTION__));
                                goto FREE_SG;
                        }
                        pPacket->pDataPtr -= sg_list[0].len;
                        NdisCopyMemory(pPacket->pDataPtr, (void *) sg_list[0].buf, sg_list[0].len);
                        pPacket->pktLen += sg_list[0].len;                
                        pMBuf->m_next = NULL;
                        goto XMIT;
                }
                goto COPY_MBUF;
        }
        else if (sg_len == 1)
        {
                if ((pMBuf->m_flags & M_EXT) /*&& ((pMBuf->m_data - pMBuf->m_ext.ext_buf) <= pMBuf->m_ext.ext_size)*/)
                {
                        Status = RTMP_AllocateNdisPacket_AppandMbuf(pAd, (PNDIS_PACKET *)&pPacket, pMBuf);
                        if (Status != NDIS_STATUS_SUCCESS)
                        {
                                MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s:can't allocate NDIS PACKET AppandMbuf\n", __FUNCTION__));
                                goto FREE_SG;
                        }
                        isFreeSG = FALSE;
                        goto XMIT;
                }
                goto COPY_MBUF;
        }
        else if (sg_len == 0)
                goto FREE_SG;


COPY_MBUF: /*when sg_len > 2 */
        Status = RTMPAllocateNdisPacket(pAd, (PNDIS_PACKET *)&pPacket, NULL, 0, NULL, MAX_ETH_FRAME_SIZE);
        if (Status != NDIS_STATUS_SUCCESS)
        {		rt_convert_sglist_to_mbuf_check_point_counter++;
				if (rt_convert_sglist_to_mbuf_check_point_counter > 20) 
				{
					rt_convert_sglist_to_mbuf_check_point_counter = 0;
                	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, ("%s:can't allocate NDIS PACKET\n", __FUNCTION__));
        		}
				goto FREE_SG;
        }

	pPacket->pDataPtr = (PUCHAR)ROUND_UP(pPacket->pDataPtr, 4) - 2;
        while (sg_len > 0)
        {
                NdisCopyMemory(GET_OS_PKT_DATAPTR(pPacket) + packetLength, (void *) sg_list->buf, sg_list->len);
                packetLength += sg_list->len;
                sg_len--;
                sg_list++;
        }

        pPacket->pDataMBuf->m_len = packetLength;
XMIT:
        GET_OS_PKT_LEN(pPacket) = total_len;
   
FREE_SG:
        if (isFreeSG)
		rt_eth_drv_tx_done(sc,key,0);

        return pPacket;
}

static void rt_ecos_send(struct eth_drv_sc *sc,
				struct eth_drv_sg *sg_list,
				int sg_len,
				int total_len,
				unsigned long key)
{
	PECOS_PKT_BUFFER	pPacket = NULL;

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("==>rt_ecos_send()\n"));

    pPacket = rt_convert_sglist_to_mbuf(sc, sg_list, sg_len, total_len, key);
    if (pPacket != NULL)
		rt28xx_send_packets(pPacket, sc);

    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("<==rt_ecos_send()\n"));
    return;
}

static void rt_ecos_recv(struct eth_drv_sc *sc,
                        struct eth_drv_sg *sg_list,
                        int sg_len)
{
    /* Not Use */
	return;
} 

#ifdef ECOS_NETTASK_SCHDULE_NEW

volatile UINT32 nettask_schedule=0;

static void rt_ecos_deliver(struct eth_drv_sc *sc)
{
	POS_COOKIE			    pOSCookie = NULL;    
	PRTMP_ADAPTER	        pAd = NULL;
	RTMP_NET_TASK_STRUCT   NetTask;
	unsigned long flags;
	UINT32 IntSource = 0;
	unsigned long flag=0;
	struct MCU_CTRL *ctl=NULL;
	DBGPRINT(RT_DEBUG_INFO, ("rt_ecos_deliver()\n"));	
	pAd = (PRTMP_ADAPTER) RtmpOsGetNetDevPriv(sc);
	if (pAd == NULL)
	    return;

	pOSCookie = (POS_COOKIE) pAd->OS_Cookie;

	
	ctl = &pAd->MCUCtrl;	
	HAL_DISABLE_INTERRUPTS(flag);
	IntSource = nettask_schedule;
	nettask_schedule = 0;
	HAL_RESTORE_INTERRUPTS(flag);

//#ifdef MT_MAC

	if (IntSource & WF_MAC_INT_3)	//27
	{
		NetTask = pOSCookie->mt_mac_int_3_task;
		//diag_printf("get:%s\n",(char *)(pNetTask->taskName));
		NetTask.funcPtr(NetTask.data); 
	}	

	if (IntSource & MT_INT_BCN_DLY)	//11
	{
		NetTask = pOSCookie->bcn_dma_done_task;

		//diag_printf("get:%s\n",(char *)(pNetTask->taskName));
		NetTask.funcPtr(NetTask.data); 
	}



	if (IntSource & MT_INT_BMC_DLY)	//12
	{
		NetTask = pOSCookie->bmc_dma_done_task;

		//diag_printf("get:%s\n",(char *)(pNetTask->taskName));
		NetTask.funcPtr(NetTask.data); 
	}
	
//#endif

	if (IntSource & 0)		//no use
	{
		NetTask = pOSCookie->fifo_statistic_full_task;

		//diag_printf("get:%s\n",(char *)(pNetTask->taskName));
		NetTask.funcPtr(NetTask.data); 
	}
	
	if (IntSource & MT_INT_MGMT_DLY)	//8
	{
		NetTask = pOSCookie->mgmt_dma_done_task;

		//diag_printf("get:%s\n",(char *)(pNetTask->taskName));
		NetTask.funcPtr(NetTask.data); 
	}

	if (IntSource & MT_INT_RX_DATA)	//0
	{
		NetTask = pOSCookie->rx_done_task;
		//log_trace(5);
		//diag_printf("get:%s\n",(char *)(pNetTask->taskName));
		NetTask.funcPtr(NetTask.data); 
	}
	
//#ifdef CONFIG_ANDES_SUPPORT
	if (IntSource & MT_INT_RX_CMD)	//1// 
	{
		NetTask = pOSCookie->rx1_done_task;

		//diag_printf("get:%s\n",(char *)(pNetTask->taskName));
		NetTask.funcPtr(NetTask.data); 
	}	
//#endif

	if (IntSource & MT_INT_CMD)	//9
	{
		NetTask = pOSCookie->hcca_dma_done_task;

		//diag_printf("get:%s\n",(char *)(pNetTask->taskName));
		NetTask.funcPtr(NetTask.data); 
	}

	if (IntSource & MT_INT_AC3_DLY)	//7
	{
		NetTask = pOSCookie->ac3_dma_done_task;

		//diag_printf("get:%s\n",(char *)(pNetTask->taskName));
		NetTask.funcPtr(NetTask.data); 
	}

	if (IntSource & MT_INT_AC2_DLY)	//6
	{
		NetTask = pOSCookie->ac2_dma_done_task;

		//diag_printf("get:%s\n",(char *)(pNetTask->taskName));
		NetTask.funcPtr(NetTask.data); 
	}

	if (IntSource & MT_INT_AC1_DLY)	//5
	{
		NetTask = pOSCookie->ac1_dma_done_task;

		//diag_printf("get:%s\n",(char *)(pNetTask->taskName));
		NetTask.funcPtr(NetTask.data); 
	}

	if (IntSource & MT_INT_AC0_DLY)	//4 //4
	{
		NetTask = pOSCookie->ac0_dma_done_task;

		//diag_printf("get:%s\n",(char *)(pNetTask->taskName));
		NetTask.funcPtr(NetTask.data); 
	}

	if (IntSource & MT_INT_MCU_CMD)	//4 //30
	{
		NetTask = ctl->cmd_msg_task;

		//diag_printf("cmd_msg_task:%s\n",(char *)(NetTask.taskName));
		NetTask.funcPtr(NetTask.data); 
	}

	if (IntSource & MT_INT_RESVD)	//4 //18
	{
		NetTask = pOSCookie->tbtt_task;

		//diag_printf("get:%s\n",(char *)(pNetTask->taskName));
		NetTask.funcPtr(NetTask.data); 
	}	

	if (IntSource & MT_INT_RESVD_2)	//4 //31
	{
		NetTask = pOSCookie->uapsd_eosp_sent_task;

		//diag_printf("get:%s\n",(char *)(pNetTask->taskName));
		NetTask.funcPtr(NetTask.data); 
	}
	/*
	if (IntSource & MT_INT_ERR_DET0)	//4 //2
	{
		NetTask = pOSCookie->dfs_task;

		//diag_printf("get:%s\n",(char *)(pNetTask->taskName));
		NetTask.funcPtr(NetTask.data); 
	}	
	*/
	DBGPRINT(RT_DEBUG_INFO, ("<=== rt_ecos_deliver()\n"));	
}

#else

static void rt_ecos_deliver(struct eth_drv_sc *sc)
{
	POS_COOKIE			    pOSCookie = NULL;    
	PRTMP_ADAPTER	        pAd = NULL;
	PRTMP_NET_TASK_STRUCT   pNetTask = NULL;
	unsigned long flags;
	int i = 0;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("rt_ecos_deliver()\n"));	
	pAd = (PRTMP_ADAPTER) RtmpOsGetNetDevPriv(sc);
	if (pAd == NULL)
	    return;

	pOSCookie = (POS_COOKIE) pAd->OS_Cookie;

	while(1)
	{
		if (pOSCookie->nettask_handle == 0)
			break;
	
        /* Note: here must call cyg_mbox_tryget, not cyg_mbox_get */
		RTMP_INT_LOCK(&pAd->irq_lock, flags);
		pNetTask = (PRTMP_NET_TASK_STRUCT) cyg_mbox_tryget(pOSCookie->nettask_handle);
		RTMP_INT_UNLOCK(&pAd->irq_lock, flags);
		
		if (pNetTask == NULL)
			break;
		//diag_printf("01%s\n",(char *)(pNetTask->taskName));
		pNetTask->funcPtr(pNetTask->data); 
		//diag_printf("02\n");
		i++;
		if (i > 60)break;
	}

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO, ("<=== rt_ecos_deliver()\n"));	
}

#endif //#endif MT7628 
static void rt_ecos_poll(struct eth_drv_sc *sc)
{
	return;
}

static int  rt_ecos_int_vector(struct eth_drv_sc *sc)
{
	return RTMP_INTERRUPT_INIC;
}

#ifdef MBSS_SUPPORT
/* Interface: ra1 */
ETH_DRV_SC(devive_wireless_mbss_sc1,
           &rt_ecos_priv_data,
           INF_MBSSID_DEV_NAME "1",
           rt_ecos_start,
           rt_ecos_stop,
           rt_ecos_control,
           rt_ecos_can_send,
           rt_ecos_send,
           rt_ecos_recv,
           rt_ecos_deliver,
           rt_ecos_poll,
           rt_ecos_int_vector
           );

NETDEVTAB_ENTRY(devive_wireless_mbss_netdev1,
                INF_MBSSID_DEV_NAME "1",
                rt_ecos_mbss_init,
                &devive_wireless_mbss_sc1);
/* Interface: ra2 */
ETH_DRV_SC(devive_wireless_mbss_sc2,
           &rt_ecos_priv_data,
           INF_MBSSID_DEV_NAME "2",
           rt_ecos_start,
           rt_ecos_stop,
           rt_ecos_control,
           rt_ecos_can_send,
           rt_ecos_send,
           rt_ecos_recv,
           rt_ecos_deliver,
           rt_ecos_poll,
           rt_ecos_int_vector
           );

NETDEVTAB_ENTRY(devive_wireless_mbss_netdev2,
                INF_MBSSID_DEV_NAME "2",
                rt_ecos_mbss_init,
                &devive_wireless_mbss_sc2);

/* Interface: ra3 */
ETH_DRV_SC(devive_wireless_mbss_sc3,
           &rt_ecos_priv_data,
           INF_MBSSID_DEV_NAME "3",
           rt_ecos_start,
           rt_ecos_stop,
           rt_ecos_control,
           rt_ecos_can_send,
           rt_ecos_send,
           rt_ecos_recv,
           rt_ecos_deliver,
           rt_ecos_poll,
           rt_ecos_int_vector
           );

NETDEVTAB_ENTRY(devive_wireless_mbss_netdev3,
                INF_MBSSID_DEV_NAME "3",
                rt_ecos_mbss_init,
                &devive_wireless_mbss_sc3);

/* Interface: ra4 */
ETH_DRV_SC(devive_wireless_mbss_sc4,
           &rt_ecos_priv_data,
           INF_MBSSID_DEV_NAME "4",
           rt_ecos_start,
           rt_ecos_stop,
           rt_ecos_control,
           rt_ecos_can_send,
           rt_ecos_send,
           rt_ecos_recv,
           rt_ecos_deliver,
           rt_ecos_poll,
           rt_ecos_int_vector
           );

NETDEVTAB_ENTRY(devive_wireless_mbss_netdev4,
                INF_MBSSID_DEV_NAME "4",
                rt_ecos_mbss_init,
                &devive_wireless_mbss_sc4);

/* Interface: ra5 */
ETH_DRV_SC(devive_wireless_mbss_sc5,
           &rt_ecos_priv_data,
           INF_MBSSID_DEV_NAME "5",
           rt_ecos_start,
           rt_ecos_stop,
           rt_ecos_control,
           rt_ecos_can_send,
           rt_ecos_send,
           rt_ecos_recv,
           rt_ecos_deliver,
           rt_ecos_poll,
           rt_ecos_int_vector
           );

NETDEVTAB_ENTRY(devive_wireless_mbss_netdev5,
                INF_MBSSID_DEV_NAME "5",
                rt_ecos_mbss_init,
                &devive_wireless_mbss_sc5);

/* Interface: ra6 */
ETH_DRV_SC(devive_wireless_mbss_sc6,
           &rt_ecos_priv_data,
           INF_MBSSID_DEV_NAME "6",
           rt_ecos_start,
           rt_ecos_stop,
           rt_ecos_control,
           rt_ecos_can_send,
           rt_ecos_send,
           rt_ecos_recv,
           rt_ecos_deliver,
           rt_ecos_poll,
           rt_ecos_int_vector
           );

NETDEVTAB_ENTRY(devive_wireless_mbss_netdev6,
                INF_MBSSID_DEV_NAME "6",
                rt_ecos_mbss_init,
                &devive_wireless_mbss_sc6);

ETH_DRV_SC(devive_wireless_mbss_sc7,
           &rt_ecos_priv_data,
           INF_MBSSID_DEV_NAME "7",
           rt_ecos_start,
           rt_ecos_stop,
           rt_ecos_control,
           rt_ecos_can_send,
           rt_ecos_send,
           rt_ecos_recv,
           rt_ecos_deliver,
           rt_ecos_poll,
           rt_ecos_int_vector
           );

NETDEVTAB_ENTRY(devive_wireless_mbss_netdev7,
                INF_MBSSID_DEV_NAME "7",
                rt_ecos_mbss_init,
                &devive_wireless_mbss_sc7);

static bool rt_ecos_mbss_init(struct cyg_netdevtab_entry *tab)
{
	PRTMP_ADAPTER		pAd = NULL;
	PNET_DEV 			pNetDev = NULL;
    UINT8				    MacAddr[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    cyg_netdevtab_entry_t   *pNetdevEntry;
    struct                  eth_drv_sc *sc;
	char                    devName[IFNAMSIZ];
	struct mt_dev_priv 	*priv_info = NULL;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("===> rt_ecos_mbss_init()\n"));
	pNetDev = (struct eth_drv_sc *)tab->device_instance;

    sprintf(devName, "%s0", INF_MAIN_DEV_NAME);
    for (pNetdevEntry = &__NETDEVTAB__[0]; pNetdevEntry != &__NETDEVTAB_END__; pNetdevEntry++)
    {
        sc = (struct eth_drv_sc *)pNetdevEntry->device_instance;
        if (strcmp(sc->dev_name, devName) == 0)
        {
			pAd = (PRTMP_ADAPTER) RtmpOsGetNetDevPriv(sc);
            break;
        }
	}
    
	if (pAd == NULL)
    {
		DBGPRINT_ERR(("%s: pAd is NULL.\n", __FUNCTION__));
		goto err_out;
    }

	/*NetDevInit============================================== */
	priv_info = (struct mt_dev_priv *) kmalloc(sizeof(struct mt_dev_priv), GFP_ATOMIC);
	pNetDev->driver_private = priv_info;
	
	/* put private data structure */
	RTMP_OS_NETDEV_SET_PRIV(pNetDev, pAd);

        	/* Initialize upper level driver */
        	(pNetDev->funs->eth_drv->init)(pNetDev, MacAddr);

        	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<=== rt_ecos_mbss_init()\n"));
        	return true;
/*        } */
/*    } */
err_out:
	return false;
}

#endif /* MBSS_SUPPORT */

#ifdef APCLI_SUPPORT
/* Interface: apcli0 */
ETH_DRV_SC(devive_wireless_apcli_sc0,
           &rt_ecos_priv_data,
           INF_APCLI_DEV_NAME "0",
           rt_ecos_start,
           rt_ecos_stop,
           rt_ecos_control,
           rt_ecos_can_send,
           rt_ecos_send,
           rt_ecos_recv,
           rt_ecos_deliver,
           rt_ecos_poll,
           rt_ecos_int_vector
           );

NETDEVTAB_ENTRY(devive_wireless_apcli_netdev0,
                INF_APCLI_DEV_NAME "0",
                rt_ecos_apcli_init,
                &devive_wireless_apcli_sc0);

static bool rt_ecos_apcli_init(struct cyg_netdevtab_entry *tab)
{
	PRTMP_ADAPTER		pAd = NULL;
	PNET_DEV 			pNetDev = NULL;
    UINT8				    MacAddr[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    cyg_netdevtab_entry_t   *pNetdevEntry;
    struct                  eth_drv_sc *sc;
	char                    devName[IFNAMSIZ];
	struct mt_dev_priv	*priv_info = NULL;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("===> rt_ecos_apcli_init()\n"));
	pNetDev = (struct eth_drv_sc *)tab->device_instance;

    sprintf(devName, "%s0", INF_MAIN_DEV_NAME);
    for (pNetdevEntry = &__NETDEVTAB__[0]; pNetdevEntry != &__NETDEVTAB_END__; pNetdevEntry++)
    {
        sc = (struct eth_drv_sc *)pNetdevEntry->device_instance;
        if (strcmp(sc->dev_name, devName) == 0)
        {
			pAd = (PRTMP_ADAPTER) RtmpOsGetNetDevPriv(sc);
            break;
        }
	}
    
	if (pAd == NULL)
    {
		DBGPRINT_ERR(("%s: pAd is NULL.\n", __FUNCTION__));
		goto err_out;
    }
	
	/*NetDevInit============================================== */
	priv_info = (struct mt_dev_priv *) kmalloc(sizeof(struct mt_dev_priv), GFP_ATOMIC);
	pNetDev->driver_private = priv_info;
		
	/* put private data structure */
	RTMP_OS_NETDEV_SET_PRIV(pNetDev, pAd);
	
        	/* Initialize upper level driver */
        	(pNetDev->funs->eth_drv->init)(pNetDev, MacAddr);

    	    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<=== rt_ecos_apcli_init()\n"));
        	return true;
/*        } */
/*    } */

err_out:
	return false;
}

#endif /* APCLI_SUPPORT */

#ifdef WDS_SUPPORT
/* Interface: wds0 */
ETH_DRV_SC(devive_wireless_wds_sc0,
           &rt_ecos_priv_data,
           INF_WDS_DEV_NAME "0",
           rt_ecos_start,
           rt_ecos_stop,
           rt_ecos_control,
           rt_ecos_can_send,
           rt_ecos_send,
           rt_ecos_recv,
           rt_ecos_deliver,
           rt_ecos_poll,
           rt_ecos_int_vector
           );
NETDEVTAB_ENTRY(devive_wireless_wds_netdev0,
                INF_WDS_DEV_NAME "0",
                rt_ecos_wds_init,
                &devive_wireless_wds_sc0);
/* Interface: wds1 */
ETH_DRV_SC(devive_wireless_wds_sc1,
           &rt_ecos_priv_data,
           INF_WDS_DEV_NAME "1",
           rt_ecos_start,
           rt_ecos_stop,
           rt_ecos_control,
           rt_ecos_can_send,
           rt_ecos_send,
           rt_ecos_recv,
           rt_ecos_deliver,
           rt_ecos_poll,
           rt_ecos_int_vector
           );
NETDEVTAB_ENTRY(devive_wireless_wds_netdev1,
                INF_WDS_DEV_NAME "1",
                rt_ecos_wds_init,
                &devive_wireless_wds_sc1);
/* Interface: wds2 */
ETH_DRV_SC(devive_wireless_wds_sc2,
           &rt_ecos_priv_data,
           INF_WDS_DEV_NAME "2",
           rt_ecos_start,
           rt_ecos_stop,
           rt_ecos_control,
           rt_ecos_can_send,
           rt_ecos_send,
           rt_ecos_recv,
           rt_ecos_deliver,
           rt_ecos_poll,
           rt_ecos_int_vector
           );
NETDEVTAB_ENTRY(devive_wireless_wds_netdev2,
                INF_WDS_DEV_NAME "2",
                rt_ecos_wds_init,
                &devive_wireless_wds_sc2);
/* Interface: wds3 */
ETH_DRV_SC(devive_wireless_wds_sc3,
           &rt_ecos_priv_data,
           INF_WDS_DEV_NAME "3",
           rt_ecos_start,
           rt_ecos_stop,
           rt_ecos_control,
           rt_ecos_can_send,
           rt_ecos_send,
           rt_ecos_recv,
           rt_ecos_deliver,
           rt_ecos_poll,
           rt_ecos_int_vector
           );
NETDEVTAB_ENTRY(devive_wireless_wds_netdev3,
                INF_WDS_DEV_NAME "3",
                rt_ecos_wds_init,
                &devive_wireless_wds_sc3);

static bool rt_ecos_wds_init(struct cyg_netdevtab_entry *tab)
{
	PRTMP_ADAPTER		    pAd = NULL;
	PNET_DEV 			    pNetDev = NULL;
    UINT8				    MacAddr[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    cyg_netdevtab_entry_t   *pNetdevEntry;
    struct                  eth_drv_sc *sc;
	char                    devName[IFNAMSIZ];
	struct mt_dev_priv	*priv_info = NULL;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("===> rt_ecos_wds_init()\n"));
	pNetDev = (struct eth_drv_sc *)tab->device_instance;

    sprintf(devName, "%s0", INF_MAIN_DEV_NAME);
    for (pNetdevEntry = &__NETDEVTAB__[0]; pNetdevEntry != &__NETDEVTAB_END__; pNetdevEntry++)
    {
        sc = (struct eth_drv_sc *)pNetdevEntry->device_instance;
        if (strcmp(sc->dev_name, devName) == 0)
        {
			pAd = (PRTMP_ADAPTER) RtmpOsGetNetDevPriv(sc);
            break;
        }
	}
    
	if (pAd == NULL)
    {
		DBGPRINT_ERR(("%s: pAd is NULL.\n", __FUNCTION__));
		goto err_out;
    }

	/*NetDevInit============================================== */
	priv_info = (struct mt_dev_priv *) kmalloc(sizeof(struct mt_dev_priv), GFP_ATOMIC);
	pNetDev->driver_private = priv_info;
			
	/* put private data structure */
	RTMP_OS_NETDEV_SET_PRIV(pNetDev, pAd);
		
        	/* Initialize upper level driver */
        	(pNetDev->funs->eth_drv->init)(pNetDev, MacAddr);

    	    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<=== rt_ecos_wds_init()\n"));
        	return true;
/*        } */
/*    } */

err_out:
	return false;
}

#endif /* WDS_SUPPORT */

#ifdef CONFIG_SNIFFER_SUPPORT
/* Interface: mon0 */
ETH_DRV_SC(devive_wireless_mon_sc0,
           &rt_ecos_priv_data,
           INF_MONITOR_DEV_NAME "0",
           rt_ecos_start,
           rt_ecos_stop,
           rt_ecos_control,
           rt_ecos_can_send,
           rt_ecos_send,
           rt_ecos_recv,
           rt_ecos_deliver,
           rt_ecos_poll,
           rt_ecos_int_vector
           );

NETDEVTAB_ENTRY(devive_wireless_mon_netdev0,
                INF_MONITOR_DEV_NAME "0",
                rt_ecos_mon_init,
                &devive_wireless_mon_sc0);

static bool rt_ecos_mon_init(struct cyg_netdevtab_entry *tab)
{
	PRTMP_ADAPTER		pAd = NULL;
	PNET_DEV 			pNetDev = NULL;
    UINT8				    MacAddr[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    cyg_netdevtab_entry_t   *pNetdevEntry;
    struct                  eth_drv_sc *sc;
	char                    devName[IFNAMSIZ];
	struct mt_dev_priv	*priv_info = NULL;

	MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("===> rt_ecos_mon_init()\n"));
	pNetDev = (struct eth_drv_sc *)tab->device_instance;

    sprintf(devName, "%s0", INF_MAIN_DEV_NAME);
    for (pNetdevEntry = &__NETDEVTAB__[0]; pNetdevEntry != &__NETDEVTAB_END__; pNetdevEntry++)
    {
        sc = (struct eth_drv_sc *)pNetdevEntry->device_instance;
        if (strcmp(sc->dev_name, devName) == 0)
        {
			pAd = (PRTMP_ADAPTER) RtmpOsGetNetDevPriv(sc);
            break;
        }
	}
    
	if (pAd == NULL)
    {
		DBGPRINT_ERR(("%s: pAd is NULL.\n", __FUNCTION__));
		goto err_out;
    }
	
	/*NetDevInit============================================== */
	priv_info = (struct mt_dev_priv *) kmalloc(sizeof(struct mt_dev_priv), GFP_ATOMIC);
	pNetDev->driver_private = priv_info;
		
	/* put private data structure */
	RTMP_OS_NETDEV_SET_PRIV(pNetDev, pAd);
	
        	/* Initialize upper level driver */
        	(pNetDev->funs->eth_drv->init)(pNetDev, MacAddr);

    	    MTWF_LOG(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_TRACE, ("<=== rt_ecos_mon_init()\n"));
        	return true;
/*        } */
/*    } */

err_out:
	return false;
}

#endif /* CONFIG_SNIFFER_SUPPORT */


