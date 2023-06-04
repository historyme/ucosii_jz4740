#ifndef __JZTOUCH_H__
#define __JZTOUCH_H__
typedef  void (*PFN_CALIBRATE)(unsigned short dat);
void TS_SetxCalibration(PFN_CALIBRATE xCal,PFN_CALIBRATE yCal);
int TS_init(void);
#endif
