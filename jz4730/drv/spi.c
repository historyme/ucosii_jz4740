#include <regs.h>
#include <ops.h>

#define DEVCLK  12000000

#define USCS1

#define CE_CE_LL	0
#define CE_CE_LH	1
#define CE_CE_HL	2
#define CE_CE_HH	3

#define MSB	0
#define LSB	1

void spi_init(void)
{
	__gpio_as_ssi();
	__cpm_start_ssi();
	__ssi_disable();
	__ssi_flush_fifo();
	__ssi_clear_errors();

	__ssi_disable_tx_intr();
	__ssi_disable_rx_intr();
	__ssi_disable_recvfinish();
	__ssi_finish_transmit();
	__ssi_enable_receive();
	__ssi_spi_format();
	__ssi_normal_mode();
	__ssi_disable_loopback();
	__ssi_set_rx_trigger(1);

	REG_SSI_CR1 &= ~(SSI_CR1_FRMHL_MASK | SSI_CR1_LFST |
			 SSI_CR1_MCOM_MASK | SSI_CR1_FLEN_MASK |
			 SSI_CR1_PHA | SSI_CR1_POL);
	REG_SSI_CR1 |= ((CE_CE_LL << SSI_CR1_FRMHL_BIT) |
			(MSB << 25) |	/* SSI_CR1_LFST bit indicat LSB */
			((16-2) << SSI_CR1_FLEN_BIT) |
			(1 << 1));	/* SSI_CR1_PHA */

#ifdef USCS1
	__ssi_select_ce();
#else
	__ssi_select_ce2();
#endif
	__ssi_set_clk(DEVCLK, 500000);	/* clk freq is 500KHz */
	__ssi_enable();
	__ssi_enable_receive();
}

unsigned int spi_read(void)
{
	__ssi_transmit_data(0);	/* dummy operation */
	__ssi_flush_rxfifo();
	while (__ssi_rxfifo_empty());
	__ssi_receive_data();	/* not a valid data. */
	while (__ssi_rxfifo_empty());
	return __ssi_receive_data();
}

void spi_write(unsigned int data)	/* max is 16 bit. */
{
	__ssi_transmit_data(data);
}

