#pragma once
#include <Arduino.h>

constexpr uint8_t ID8_NULL = 0xFF;
constexpr uint8_t ID8_OVER = 0xF0;
#define ID8_VALID(id) (id < ID8_OVER)

inline void IO_CONFIG(uint8_t pin, uint8_t mode, uint8_t state = ID8_NULL){
    if(ID8_VALID(pin)){
        pinMode(pin, mode);
        if(ID8_VALID(state))
            digitalWrite(pin, state == 0 ? 0 : 1);
    }
}

inline void IO_SET(uint8_t pin, uint8_t state){
    if(ID8_VALID(pin) && ID8_VALID(state)) digitalWrite(pin, state == 0 ? 0 : 1);
}

inline bool IO_GET(uint8_t pin, uint8_t* pval){
    if(ID8_VALID(pin)) {*pval = digitalRead(pin); return true;}
    return false;
}

inline uint8_t IO_CHECK(uint8_t pin){
    return ID8_VALID(pin) ? digitalRead(pin) : ID8_NULL;
}

inline uint32_t IO_PULSE_MILLIS(uint8_t pin, uint8_t pol, uint32_t pre, uint32_t hold, uint32_t post){
    auto set_hold = [](uint8_t pin, uint8_t state, uint32_t ms) { digitalWrite(pin, state); delay(ms); return ms;};
    uint32_t rslt = 0;
    if(ID8_VALID(pin)){
        pinMode(pin, OUTPUT);
        rslt += set_hold(pin, (pol == 0) ? 1 : 0, std::clamp<uint32_t>(pre, 1U, 10'000U));
        rslt += set_hold(pin, (pol == 0) ? 0 : 1, std::clamp<uint32_t>(hold, 1U, 20'000U));
        rslt += set_hold(pin, (pol == 0) ? 1 : 0, std::clamp<uint32_t>(post, 1U, 10'000U));

    }
    return rslt;
}

inline uint32_t IO_PULSE_MICROS(uint8_t pin, uint8_t pol, uint32_t pre, uint32_t hold, uint32_t post){
    auto set_hold = [](uint8_t pin, uint8_t state, uint32_t us) { digitalWrite(pin, state); delayMicroseconds(us); return us;};
    uint32_t rslt = 0;
    if(ID8_VALID(pin)){
        pinMode(pin, OUTPUT);
        rslt += set_hold(pin, (pol == 0) ? 1 : 0, std::clamp<uint32_t>(pre, 1U, 5'000U));
        rslt += set_hold(pin, (pol == 0) ? 0 : 1, std::clamp<uint32_t>(hold, 1U, 10'000U));
        rslt += set_hold(pin, (pol == 0) ? 1 : 0, std::clamp<uint32_t>(post, 1U, 5'000U));

    }
    return rslt;
}





