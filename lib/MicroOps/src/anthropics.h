#pragma once
#include <Arduino.h>
#include <initializer_list>
#include "io_controls.h"
#include "chrono_gear.h"

class DwellButton{
public:
    using iopin_t = uint8_t;
    static constexpr iopin_t IOPIN_NONE = (iopin_t)~0;

protected:
    uint32_t    _msLastReleased = 0U;
    uint32_t    _msLastPressed = 0U;
    uint32_t    _msDepletion = 10U;
    uint32_t    _msLastChange = 0U;
    bool        _inited = false;
    iopin_t     _pin = IOPIN_NONE;
    uint8_t     _input_mode = INPUT_PULLUP;
    uint8_t     _state = 0;

    inline uint8_t _get_pressed(){
        if(!_inited){
            if(_pin == IOPIN_NONE) return 0;
            pinMode(_pin, INPUT_PULLUP); 
            delayMicroseconds(100); 
            _inited = true;
        }
        return (digitalRead(_pin) == 0) ? 1 : 0;
    }

public:
    DwellButton(uint8_t pin, uint32_t debounce = 23)  : _pin{pin}, _msDepletion{debounce} {}

    /*** returns: 0 if unchanged ; neg dwell if just pressed; pos dwell if just released  ***/
    virtual int32_t update(uint32_t msNow);
};

class LEDBeacon{
    public:
        static constexpr uint8_t LED_ON = 1;
        static constexpr uint8_t LED_OFF = 0;
        static constexpr timestamp_t TIME_TBD = ~((timestamp_t)0);
    
    protected:
        enum beacon_state { BEACON_ON, BEACON_OFF, STROBE_SINGLE, STROBE_MULTI, STROBE_DTAP};
        const uint8_t                       _iopin;
        bool                                _enlit = false;
        beacon_state                        _state;
        bool                                _inited = false;
        timestamp_t                         _msPrev, _msPeriodHigh, _msPeriodLow;
    
        void set_show(bool ison){ 
            _enlit = ison;
            show(); 
        }
        void set_mode(beacon_state state);
    
    public:
        LEDBeacon(uint8_t sigpin);
        virtual bool show();
        virtual void init();
        virtual void update(timestamp_t msNow);
    
        void strobe(timestamp_t msHigh, timestamp_t msLow);
        void double_strobe(timestamp_t msHigh, timestamp_t msLow) { strobe(msHigh, msLow); if(_state == STROBE_MULTI) { _state = STROBE_DTAP; } }
        void solid(uint8_t state) { if(state == 0) { _state = BEACON_OFF; set_show(false); } else { _state = BEACON_ON; set_show(true); } }
    };
    

#pragma region EpochsButton
template<size_t N>
class EpochsButton : public DwellButton{
public:
    static constexpr size_t MAX_BINS = N+1;

protected:
    uint32_t _bounds[MAX_BINS];
    uint32_t _counts[MAX_BINS];

public:
    EpochsButton(uint8_t pin, std::initializer_list<uint32_t> epochs);

    virtual int32_t update(uint32_t msNow) override;
    uint32_t peek(size_t index){ return (index <= N) ? _counts[index] : 0; }
    uint32_t take(size_t index, uint32_t max = ~0U);
};

template <size_t N>
inline EpochsButton<N>::EpochsButton(uint8_t pin, std::initializer_list<uint32_t> epochs) : DwellButton{pin}
{
    const uint32_t* epoch = epochs.begin();
    for(int ix = 0; ix < N; ++ix){
        _counts[ix] = 0;
        _bounds[ix] = (epoch < epochs.end()) ? *(epoch++) : ~0U;
    }
    _counts[N] = 0;
    _bounds[N] = ~0U;

}

template <size_t N>
inline int32_t EpochsButton<N>::update(uint32_t msNow)
{
    int32_t dwell = DwellButton::update(msNow);
    if((0 < dwell) && (dwell < _bounds[N])){
        size_t n = 0;
        while(_bounds[n] < dwell) n++;
        ++_counts[n];
    }
    return dwell;
}

template <size_t N>
inline uint32_t EpochsButton<N>::take(size_t index, uint32_t max)
{
    uint32_t rslt = 0;
    if(index <= N){
        rslt = _counts[index];
        if(rslt > max) rslt = max;
        _counts[index] -= rslt;
    }
    return rslt;
}

#pragma endregion



    