#include <math.h>
#include "NTPtimeESP.h"
#include <RtcDS3231.h>

#define NTP_SERVER "a.st1.ntp.br"
#define TIME_ZONE -3

typedef struct Horario{
    int hora;
    int minuto;
    int segundo;
} Horario;

typedef struct LightData{
    float start;
    float end;
    float max;
    float limit;
} LightData;


float calcC(float diff, float max, float limit);
float horarioToDec(RtcDateTime tempo);
void timeToString(char *str, RtcDateTime tempo);
int getLightValue(RtcDateTime tempo, LightData params);
void fillLightData(LightData *params,float start, float end, float max, float limit);
void setTime(Horario *tempo);
