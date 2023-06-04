#ifndef __PCM_H__
#define __PCM_H__

/* Audio Sample Format */
#define	AFMT_U8			8
#define AFMT_S16_LE		16

/* PCM ioctl command */
#define PCM_SET_SAMPLE_RATE	0
#define PCM_SET_CHANNEL		1
#define PCM_SET_FORMAT		2
#define PCM_SET_VOL		3
#define PCM_GET_VOL		4

#endif /* __PCM_H__ */

