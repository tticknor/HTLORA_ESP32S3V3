#pragma once
#include <Arduino.h>

typedef decltype(millis()) timestamp_t;

inline timestamp_t get_millis() { return millis(); }
inline timestamp_t get_micros() { return micros(); }
inline void delay_millis(timestamp_t ms) { delay(ms); }
inline void delay_micros(timestamp_t us) {
    if(us > 5000){
        uint32_t usFinal =  us + micros();
        delay(us/1000 - 2);
        us = usFinal - micros();  // should now be ~ 2000 < us < 3000, even if rolled over
        if(us > 5000) us = 1; // just in case delay() overshot and us is now a large (e.g. negative) number
    }
    delayMicroseconds(us);
}

struct interval_t {
    timestamp_t msDelay;
    timestamp_t msPrev;

    interval_t(timestamp_t delay, timestamp_t start = 0) : msDelay{delay}, msPrev{start} {}
    inline void restart(timestamp_t msNow) { msPrev = msNow; }
    inline bool check(timestamp_t msNow) {bool rslt = ((msNow - msPrev) >= msDelay); if(rslt) { msPrev = msNow; } return rslt; }
    inline bool peek(timestamp_t msNow) { return ((msNow - msPrev) >= msDelay); }
};

struct oneshot_t {
    static constexpr timestamp_t TSNULL = ~((timestamp_t)0);
    timestamp_t msDelay;
    timestamp_t msOrig;

    oneshot_t(timestamp_t delay, timestamp_t start = TSNULL) : msDelay{delay}, msOrig{start} {}
    inline void restart(timestamp_t msNow, int delay = -1) { msOrig = msNow; if(delay > 0) { msDelay = (timestamp_t)delay; } }
    inline void kill() { msOrig = TSNULL; }
    inline bool is_active() { return (msOrig != TSNULL); }
    inline bool elapsed(timestamp_t msNow, bool kill_on_true = true){
        bool rslt = false;
        if((msOrig != TSNULL) && ((msNow - msOrig) >= msDelay)){
            rslt = true;
            if(kill_on_true) { kill(); }
        }
        return rslt;
    }
};

struct procedure_monitor_o{
    uint32_t sampling = 64;
    uint32_t usTotal = 0;
    uint32_t nReps = 0;
    uint32_t msHead = 0;

    procedure_monitor_o(uint32_t sampling_ = 64, uint32_t msNow = 0) : sampling{sampling_}, usTotal{0}, nReps{0}, msHead{msNow} {}

    uint32_t log(uint32_t usec0, uint32_t usec1){
        usTotal += (usec1 - usec0);
        if(++nReps >= sampling) report(millis());
        return nReps;
    }

    float report(uint32_t msNow){
        float msDwell = 0.001f*(float)usTotal;
        float msAvg = msDwell/(float)(nReps);
        float occupancy = msDwell/(float)(msNow - msHead);
        usTotal = nReps = 0;
        msHead = msNow;
        Serial.printf("Average procedure takes %0.02f millseconds and is running ~ %0.01f percent of time", msAvg, 100.0f*occupancy);
        return msAvg;
    }
};
    
