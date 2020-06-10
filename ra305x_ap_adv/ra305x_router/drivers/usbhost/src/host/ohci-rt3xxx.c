/*
 * RT3883 OHCI HCD (Host Controller Driver) for USB.
 *
 * (C) Copyright 2009 Ralink Tech Company
 *
 * Bus Glue for Ralink OHCI controller.
 *
 * Written by YYHuang <yy_huang@ralinktech.com.tw>
 * Based on fragments of previous driver by Russell King et al.
 *
 * This file is licenced under the GPL.
 */
/*
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/signal.h>
#include <linux/platform_device.h>
*/

#include "ralink_usb.h"
#include "hcd.h"
//#include "ohci_hcd.c"
#if 0 //Jody
static struct hc_driver ohci_rt3xxx_hc_driver = {
	.description		= hcd_name,
	.product_desc		= "RT3xxx OHCI Controller",
	.hcd_priv_size		= sizeof(struct ohci_hcd),
	.irq			= ohci_irq,
	.flags			= HCD_USB11 | HCD_MEMORY,
	.start			= ohci_rt3xxx_start,
	.stop			= ohci_stop,
	.shutdown		= ohci_shutdown,
	.urb_enqueue		= ohci_urb_enqueue,
	.urb_dequeue		= ohci_urb_dequeue,
	.endpoint_disable	= ohci_endpoint_disable,
	.get_frame_number	= ohci_get_frame,
	.hub_status_data	= ohci_hub_status_data,
	.hub_control		= ohci_hub_control,
//	.hub_irq_enable		= ohci_rhsc_enable,
#ifdef CONFIG_PM
	.bus_suspend		= ohci_bus_suspend,
	.bus_resume		= ohci_bus_resume,
#endif
	.start_port_reset	= ohci_start_port_reset,
};


int usb_hcd_rt3xxx_probe(void)
{
	int retval;
	struct usb_hcd *hcd;
	struct hc_driver *driver = &ohci_rt3xxx_hc_driver;

	/*
	if (pdev->resource[1].flags != IORESOURCE_IRQ) {
		pr_debug("resource[1] is not IORESOURCE_IRQ");
		return -ENOMEM;
	}

	hcd = usb_create_hcd(driver, &pdev->dev, "rt3xxx-ohci");
	if (hcd == NULL)
		return -ENOMEM;
       
	hcd->rsrc_start = pdev->resource[0].start;
	hcd->rsrc_len = pdev->resource[0].end - pdev->resource[0].start + 1;
	if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len, hcd_name)) {
		usb_put_hcd(hcd);
		retval = -EBUSY;
		goto err1;
	}

	hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
	if (hcd->regs == NULL) {
		pr_debug("ioremap failed");
		retval = -ENOMEM;
		goto err2;
	}
      */

//	usb_host_clock = clk_get(&pdev->dev, "usb_host");
//	ep93xx_start_hc(&pdev->dev);

	try_wake_up();

	//ohci_hcd_init(hcd_to_ohci(hcd));

	if (driver->reset && (retval = driver->reset(hcd)) < 0) {
		diag_printf("setup error %d\n", retval);
		return retval;
	}

	if ((retval = driver->start(hcd)) < 0) {
		diag_printf("startup error %d\n", retval);
		return retval;
	}

    /*
	retval = usb_add_hcd(hcd, pdev->resource[1].start, IRQF_DISABLED | IRQF_SHARED);
	if (retval == 0)
		return retval;

	iounmap(hcd->regs);
err2:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
err1:
	usb_put_hcd(hcd);
       */
       
	return retval;
}

static void usb_hcd_rt3xxx_remove(struct usb_hcd *hcd, struct platform_device *pdev)
{
#if 0
	usb_remove_hcd(hcd);
//	ep93xx_stop_hc(&pdev->dev);
//	clk_put(usb_host_clock);
	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);
#endif
}

static int __devinit ohci_rt3xxx_start(struct usb_hcd *hcd)
{
	struct ohci_hcd *ohci = hcd_to_ohci(hcd);
	int ret;

	if ((ret = ohci_init(ohci)) < 0)
		return ret;

	if ((ret = ohci_run(ohci)) < 0) {
		err("can't start %s", hcd->self.bus_name);
		ohci_stop(hcd);
		return ret;
	}

	return 0;
}

extern int usb_disabled(void);


static int ohci_hcd_rt3xxx_drv_probe(struct platform_device *pdev)
{
	int ret;

	ret = -ENODEV;

	if (!usb_disabled())
		ret = usb_hcd_rt3xxx_probe(&ohci_rt3xxx_hc_driver, pdev);

	return ret;
}

static int ohci_hcd_rt3xxx_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	usb_hcd_rt3xxx_remove(hcd, pdev);

	if(!usb_find_device(0x0, 0x0)) // No any other USB host controller.
		try_sleep();

	return 0;
}

/*
#ifdef CONFIG_PM
static int ohci_hcd_ep93xx_drv_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct ohci_hcd *ohci = hcd_to_ohci(hcd);

	if (time_before(jiffies, ohci->next_statechange))
		msleep(5);
	ohci->next_statechange = jiffies;

	ep93xx_stop_hc(&pdev->dev);
	hcd->state = HC_STATE_SUSPENDED;
	pdev->dev.power.power_state = PMSG_SUSPEND;

	return 0;
}

static int ohci_hcd_ep93xx_drv_resume(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct ohci_hcd *ohci = hcd_to_ohci(hcd);
	int status;

	if (time_before(jiffies, ohci->next_statechange))
		msleep(5);
	ohci->next_statechange = jiffies;

	ep93xx_start_hc(&pdev->dev);
	pdev->dev.power.power_state = PMSG_ON;
	usb_hcd_resume_root_hub(hcd);

	return 0;
}
#endif
*/

#if 0//Jody
static struct platform_driver ohci_hcd_rt3xxx_driver = {
	.probe		= ohci_hcd_rt3xxx_drv_probe,
	.remove		= ohci_hcd_rt3xxx_drv_remove,
	.shutdown	= usb_hcd_platform_shutdown,
/*
#ifdef CONFIG_PM
	.suspend	= ohci_hcd_rt3xxx_drv_suspend,
	.resume		= ohci_hcd_rt3xxx_drv_resume,
#endif
*/
	.driver		= {
		.name	= "rt3xxx-ohci",
	},
};
#endif
#endif
