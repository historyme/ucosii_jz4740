#ifndef __ASM_JZ47XX_CLOCK_H__
#define __ASM_JZ47XX_CLOCK_H__

/*
 * System clock structure
 */
struct sys_clock {
	unsigned int iclk;
	unsigned int sclk;
	unsigned int mclk;
	unsigned int pclk;
	unsigned int devclk;
	unsigned int rtcclk;
	unsigned int uartclk;
	unsigned int lcdclk;
	unsigned int pixclk;
	unsigned int usbclk;
	unsigned int i2sclk;
	unsigned int mscclk;
};

extern struct sys_clock jz_clocks;

#define JZ_EXTAL		12000000
#define JZ_EXTAL2		32768

static __inline__ unsigned int __cpm_get_pllout(void)
{
	unsigned int nf, nr, no, pllout;
	unsigned long plcr = REG_CPM_PLCR1;
	unsigned long od[4] = {1, 2, 2, 4};
	if (plcr & CPM_PLCR1_PLL1EN) {
		nf = (plcr & CPM_PLCR1_PLL1FD_MASK) >> CPM_PLCR1_PLL1FD_BIT;
		nr = (plcr & CPM_PLCR1_PLL1RD_MASK) >> CPM_PLCR1_PLL1RD_BIT;
		no = od[((plcr & CPM_PLCR1_PLL1OD_MASK) >> CPM_PLCR1_PLL1OD_BIT)];
		pllout = (JZ_EXTAL) / ((nr+2) * no) * (nf+2);
	} else
		pllout = JZ_EXTAL;
	return pllout;
}

static __inline__ unsigned int __cpm_get_iclk(void)
{
	unsigned int iclk;
	int div[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};
	unsigned long cfcr = REG_CPM_CFCR;
	unsigned long plcr = REG_CPM_PLCR1;
	if (plcr & CPM_PLCR1_PLL1EN)
		iclk = __cpm_get_pllout() /
		       div[(cfcr & CPM_CFCR_IFR_MASK) >> CPM_CFCR_IFR_BIT];
	else
		iclk = JZ_EXTAL;
	return iclk;
}

static __inline__ unsigned int __cpm_get_sclk(void)
{
	unsigned int sclk;
	int div[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};
	unsigned long cfcr = REG_CPM_CFCR;
	unsigned long plcr = REG_CPM_PLCR1;
	if (plcr & CPM_PLCR1_PLL1EN)
		sclk = __cpm_get_pllout() /
		       div[(cfcr & CPM_CFCR_SFR_MASK) >> CPM_CFCR_SFR_BIT];
	else
		sclk = JZ_EXTAL;
	return sclk;
}

static __inline__ unsigned int __cpm_get_mclk(void)
{
	unsigned int mclk;
	int div[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};
	unsigned long cfcr = REG_CPM_CFCR;
	unsigned long plcr = REG_CPM_PLCR1;
	if (plcr & CPM_PLCR1_PLL1EN)
		mclk = __cpm_get_pllout() /
		       div[(cfcr & CPM_CFCR_MFR_MASK) >> CPM_CFCR_MFR_BIT];
	else
		mclk = JZ_EXTAL;
	return mclk;
}

static __inline__ unsigned int __cpm_get_pclk(void)
{
	unsigned int devclk;
	int div[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};
	unsigned long cfcr = REG_CPM_CFCR;
	unsigned long plcr = REG_CPM_PLCR1;
	if (plcr & CPM_PLCR1_PLL1EN)
		devclk = __cpm_get_pllout() /
			 div[(cfcr & CPM_CFCR_PFR_MASK) >> CPM_CFCR_PFR_BIT];
	else
		devclk = JZ_EXTAL;
	return devclk;
}

static __inline__ unsigned int __cpm_get_lcdclk(void)
{
	unsigned int lcdclk;
	int div[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};
	unsigned long cfcr = REG_CPM_CFCR;
	unsigned long plcr = REG_CPM_PLCR1;
	if (plcr & CPM_PLCR1_PLL1EN)
		lcdclk = __cpm_get_pllout() /
			 div[(cfcr & CPM_CFCR_LFR_MASK) >> CPM_CFCR_LFR_BIT];
	else
		lcdclk = JZ_EXTAL;
	return lcdclk;
}

static __inline__ unsigned int __cpm_get_pixclk(void)
{
	unsigned int pixclk;
	unsigned long cfcr2 = REG_CPM_CFCR2;
	pixclk = __cpm_get_pllout() / (cfcr2 + 1);
	return pixclk;
}

static __inline__ unsigned int __cpm_get_devclk(void)
{
	return 12000000;
}

static __inline__ unsigned int __cpm_get_rtcclk(void)
{
	return 32768;
}

static __inline__ unsigned int __cpm_get_uartclk(void)
{
	return 12000000;
}

static __inline__ unsigned int __cpm_get_usbclk(void)
{
	unsigned int usbclk;
	unsigned long cfcr = REG_CPM_CFCR;
	if (cfcr & CPM_CFCR_MSC)
		usbclk = 48000000;
	else
		usbclk = __cpm_get_pllout() /
			(((cfcr &CPM_CFCR_UFR_MASK) >> CPM_CFCR_UFR_BIT) + 1);
	return usbclk;
}

static __inline__ unsigned int __cpm_get_i2sclk(void)
{
	unsigned int i2sclk;
	unsigned long cfcr = REG_CPM_CFCR;
	i2sclk = __cpm_get_pllout() /
		((cfcr & CPM_CFCR_I2S) ? 2: 1);
	return i2sclk;
}

static __inline__ unsigned int __cpm_get_mscclk(void)
{
	if (REG_CPM_CFCR & CPM_CFCR_I2S)
		return 24576000;
	else
		return 19169200;
}

#endif /* __ASM_JZ47XX_CLOCK_H__ */
