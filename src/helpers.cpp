#include "helpers.h"


float horarioToDec(RtcDateTime tempo){
    return (float)tempo.Hour() + (float)tempo.Minute()/60 + (float)tempo.Second()/3600;
}


void incTime(Horario *c_time){
	int hora = c_time->hora;
	int minuto = c_time->minuto;
	int segundo = c_time->segundo;
	segundo++;
	if(segundo >= 60){
		segundo = 0;
		minuto++;
		if(minuto >= 60){
			minuto = 0;
			hora++;
			if(hora >= 23){
				hora = 0;
			}
		}
	}
	c_time->hora = hora;
	c_time->minuto = minuto;
	c_time->segundo = segundo;
}

void timeToString(char *str, RtcDateTime tempo){
	int hour = tempo.Hour();
	int minute = tempo.Minute();
	int second = tempo.Second();
	if(hour < 10){
		str[0] = '0';
		str[1] = hour + 48;
		}else{
		str[0] = hour/10 + 48;
		str[1] = hour%10 + 48;
	}
	str[2] = ':';
	if(minute < 10){
		str[3] = '0';
		str[4] = minute + 48;
		}else{
		str[3] = minute/10 + 48;
		str[4] = minute%10 + 48;
	}
	str[5] = ':';
	if(second < 10){
		str[6] = '0';
		str[7] = second + 48;
		}else{
		str[6] = second/10 + 48;
		str[7] = second%10 + 48;
	}
	str[8] = 0;
	
}

float calcC(float diff, float max, float limit){
    return -(diff*diff)/(8.0*(float)log(limit/max));
}

int getLightValue(RtcDateTime tempo, LightData params){
    float x = horarioToDec(tempo);
    if(params.start > x || x > params.end) return 0;
    float diff = params.end - params.start;
    float mean = params.start + diff/2;
    float c = calcC(diff, params.max, params.limit);
    return (params.max*(float)exp(-((x-mean)*(x-mean))/(2.0*c)))*1023/100;
}

void fillLightData(LightData *params,float start, float end, float max, float limit){
    if(start > end){
        params->start = -1.0;
    }
    params->start = start;
    params->end = end;
    params->max = max;
    params->limit = limit;
}