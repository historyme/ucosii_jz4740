#ifndef __JZTOUCH_H__
#define __JZTOUCH_H__

typedef void (*PFN_SADC)(unsigned short * dat);

int  TS_init(void);
void initTouch(PFN_SADC touchfn);
void initBattery(int countpersec,PFN_SADC batfn);

#endif
