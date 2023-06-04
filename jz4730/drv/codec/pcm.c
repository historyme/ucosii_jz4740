#if CODECTYPE == 2
//#error "AAA"
#include <./i2s/i2s_ak4642en.c>
#include <./i2s/i2s_codec_ak4642en.c>
#endif
#if CODECTYPE == 3
#include <./ac97/ac97.c>
#endif
