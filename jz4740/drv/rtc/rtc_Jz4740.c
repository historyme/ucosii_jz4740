/*
 * Jz OnChip Real Time Clock interface for Linux
 *
 */

#include <jz4740.h>


#define RTC_ALM_READ 0
#define RTC_ALM_SET 1
#define RTC_RD_TIME 2
#define RTC_SET_TIME 3
#define RTC_EPOCH_READ 4
#define RTC_EPOCH_SET 5

#define EINVAL 1
#define EACCES 2
#define EFAULT 3

#include <rtc.h>
//extern spinlock_t rtc_lock;


static unsigned int rtc_status = 0;
static unsigned int epoch = 1900;
static const unsigned char days_in_mo[] = {
	0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static const unsigned int yearday[5] = {0, 366, 366+365, 366+365*2, 366+365*3};
static const unsigned int sweekday = 6;
static const unsigned int sum_monthday[13] = {
	0,
	31,
	31+28,
	31+28+31,
	31+28+31+30,
	31+28+31+30+31,
	31+28+31+30+31+30,
	31+28+31+30+31+30+31,
	31+28+31+30+31+30+31+31,
	31+28+31+30+31+30+31+31+30,
	31+28+31+30+31+30+31+31+30+31,
	31+28+31+30+31+30+31+31+30+31+30,
	365
};

unsigned int jz_mktime(int year, int mon, int day, int hour, int min, int sec)
{
	unsigned int seccounter;

	if (year < 2000)
		year = 2000;
	year -= 2000;
	seccounter = (year/4)*(365*3+366);
	seccounter += yearday[year%4];
	if (year%4)
		seccounter += sum_monthday[mon-1];
	else
		if (mon >= 3)
			seccounter += sum_monthday[mon-1]+1;
		else
			seccounter += sum_monthday[mon-1];
	seccounter += day-1;
	seccounter *= 24;
	seccounter += hour;
	seccounter *= 60;
	seccounter += min;
	seccounter *= 60;
	seccounter += sec;

	return seccounter;
}

void jz_gettime(unsigned int rtc, int *year, int *mon, int *day, int *hour,
	     int *min, int *sec, int *weekday)
{
	unsigned int tday, tsec, i, tmp;

	tday = rtc/(24*3600);
	*weekday = ((tday % 7) + sweekday) % 7;
	*year = (tday/(366+365*3)) * 4;
	tday = tday%(366+365*3);
	for (i=0;i<5;i++)
		if (tday<yearday[i]) {
			*year += i-1;
			tday -= yearday[i-1];
			break;
		}
	for (i=0;i<13;i++) {
		tmp = sum_monthday[i];
		if (((*year%4) == 0) && (i>=2))
			tmp += 1;
		if (tday<tmp) {
			*mon = i;
			tmp = sum_monthday[i-1];
			if (((*year%4) == 0) && ((i-1)>=2))
				tmp += 1;
			*day = tday - tmp + 1;
			break;
		}
	}
	tsec = rtc % (24 * 3600);
	*hour = tsec / 3600;
	*min = (tsec / 60) % 60;
	*sec = tsec - *hour*3600 - *min*60;
	*year += 2000;
}

void get_rtc_time(struct rtc_time *rtc_tm)
{
	unsigned int sec,mon,mday,wday,year,hour,min;
	unsigned int lval;
	unsigned long flags;
	/*
	 * Only the values that we read from the RTC are set. We leave
	 * tm_wday, tm_yday and tm_isdst untouched. Even though the
	 * RTC has RTC_DAY_OF_WEEK, we ignore it, as it is only updated
	 * by the RTC when initially set to a non-zero value.
	 */
	flags = spin_lock_irqsave();
	lval = REG_RTC_RSR;
	jz_gettime(lval, &year, &mon, &mday, &hour, &min, &sec, &wday);

	year -= 2000;

	rtc_tm->tm_sec = sec;
	rtc_tm->tm_min = min;
	rtc_tm->tm_hour = hour;
	rtc_tm->tm_mday = mday;
	rtc_tm->tm_wday = wday;
	/* Don't use centry, but start from year 1970 */
	rtc_tm->tm_mon = mon;
	if ((year += (epoch - 1900)) <= 69)
		year += 100;
	rtc_tm->tm_year = year + 1900;
	spin_unlock_irqrestore(flags);



	/*
	 * Account for differences between how the RTC uses the values
	 * and how they are defined in a struct rtc_time;
	 */
	rtc_tm->tm_mon--;
}

void get_rtc_alm_time(struct rtc_time *alm_tm)
{ 
	unsigned int sec,mon,mday,wday,year,hour,min;
	unsigned int lval;
	unsigned long flags;
	/*
	 * Only the values that we read from the RTC are set. That
	 * means only tm_hour, tm_min, and tm_sec.
	 */
	flags = spin_lock_irqsave();
	lval = REG_RTC_RSAR;
	jz_gettime(lval, &year, &mon, &mday, &hour, &min, &sec, &wday);

	alm_tm->tm_sec = sec;
	alm_tm->tm_min = min;
	alm_tm->tm_hour = hour;

	spin_unlock_irqrestore(flags);
}


int rtc_ioctl(unsigned int cmd,struct rtc_time *val,unsigned int epo)
{
	struct rtc_time wtime; 
	switch (cmd) {
	case RTC_ALM_READ:	/* Read the present alarm time */
		/*
		 * This returns a struct rtc_time. Reading >= 0xc0
		 * means "don't care" or "match all". Only the tm_hour,
		 * tm_min, and tm_sec values are filled in.
		 */
		get_rtc_alm_time(val);
		break; 
	case RTC_ALM_SET:	/* Store a time into the alarm */
	{
		unsigned char ahrs, amin, asec;
		unsigned int sec,mon,mday,wday,year,hour,min;
		unsigned int lval;
		unsigned long flags;
		struct rtc_time alm_tm;

		alm_tm = *val;
		ahrs = alm_tm.tm_hour;
		amin = alm_tm.tm_min;
		asec = alm_tm.tm_sec;

	

		if (ahrs >= 24)
			return -1;

		if (amin >= 60)
			return -1;

		if (asec >= 60)
			return -1;

		flags = spin_lock_irqsave();
		lval = REG_RTC_RSR;
		jz_gettime(lval, &year, &mon, &mday, &hour, &min, &sec, &wday);
		hour = ahrs;
		min = amin;
		sec = asec;
		lval = jz_mktime(year, mon, mday, hour, min, sec);
		REG_RTC_RSAR = lval;
		spin_unlock_irqrestore(flags);
		break;
	}
	case RTC_RD_TIME:	/* Read the time/date from RTC	*/
		get_rtc_time(val);
		break;
	case RTC_SET_TIME:	/* Set the RTC */
	{
		struct rtc_time rtc_tm;
		unsigned int mon, day, hrs, min, sec, leap_yr, date;
		unsigned int yrs;
		unsigned int lval;
		unsigned long flags;

		rtc_tm = *val;
		yrs = rtc_tm.tm_year;// + 1900;
		mon = rtc_tm.tm_mon + 1;   /* tm_mon starts at zero */
		day = rtc_tm.tm_wday;
		date = rtc_tm.tm_mday;
		hrs = rtc_tm.tm_hour;
		min = rtc_tm.tm_min;
		sec = rtc_tm.tm_sec;


		if (yrs < 1970)
			return -EINVAL;
		leap_yr = ((!(yrs % 4) && (yrs % 100)) || !(yrs % 400));

		if ((mon > 12) || (date == 0))
			return -EINVAL;

		if (date > (days_in_mo[mon] + ((mon == 2) && leap_yr)))
			return -EINVAL;

		if ((hrs >= 24) || (min >= 60) || (sec >= 60))
			return -EINVAL;

		if ((yrs -= epoch) > 255)    /* They are unsigned */
			return -EINVAL;

		flags = spin_lock_irqsave();
		/* These limits and adjustments are independant of
		 * whether the chip is in binary mode or not.
		 */

		if (yrs > 169) {
			spin_unlock_irqrestore(flags);
			return -EINVAL;
		}

		yrs += epoch;
		lval = jz_mktime(yrs, mon, date, hrs, min, sec);
		REG_RTC_RSR = lval;
		/* FIXME: maybe we need to write alarm register here. */
		spin_unlock_irqrestore(flags);

		return 0;
	}
	break;
	case RTC_EPOCH_READ:	/* Read the epoch.	*/
		epo = epoch;
		return 0;
	case RTC_EPOCH_SET:	/* Set the epoch.	*/
		/* 
		 * There were no RTC clocks before 1900.
		 */
		if (epo < 1900)
			return -EINVAL;

		epoch = epo;
		return 0;
	default:
		return -EINVAL;
	}
	return -EINVAL;
}

void Jz_rtc_init(void)
{
	REG_RTC_RSR = 0;
	while(!(REG_RTC_RCR & 0x80))
              ;
	REG_RTC_RCR = 1;
	udelay(70);
	while(!(REG_RTC_RCR & 0x80))
              ;
	REG_RTC_RGR=0x7fff; 
	udelay(70);

}



