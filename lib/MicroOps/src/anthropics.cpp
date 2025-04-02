#include "anthropics.h"

int32_t DwellButton::update(uint32_t msNow)
{
    /*** returns: 0 if unchanged ; neg dwell if just pressed; pos dwell if just released  ***/
    int32_t dt = 0;
    if((msNow - _msLastChange) > _msDepletion){
        uint8_t pressed = _get_pressed();
        if(pressed != _state){
            if(pressed){
                dt = -(int32_t)((msNow - _msLastReleased) & 0x7FFFFFFFU);
                _msLastPressed = msNow;
            }
            else {
                dt = (int32_t)((msNow - _msLastPressed) & 0x7FFFFFFFU);
                _msLastReleased = msNow;
            }
            _msLastChange = msNow;
            _state = pressed;
        }
    }
    return dt;
}

LEDBeacon::LEDBeacon(uint8_t sigpin) : _iopin{sigpin} { 
    _inited = false; 
    _state = BEACON_OFF; 
}

bool LEDBeacon::show()
{
    if(_inited){
        digitalWrite(_iopin, _enlit ? LED_ON : LED_OFF);
    }
    return _inited;
}

void LEDBeacon::set_mode(beacon_state state){
    if((state == BEACON_ON) || (state == STROBE_SINGLE) || (state == STROBE_MULTI)){
        _state = state; set_show(true);
    }
    else {
        _state = BEACON_OFF; set_show(false);
    }
}

void LEDBeacon::update(timestamp_t msNow)
{
    if(_msPrev == TIME_TBD) _msPrev = msNow;
    switch(_state){
        case BEACON_OFF:
            if (_enlit) { set_show(false); }
            break;
        case BEACON_ON:
            if(!_enlit) { set_show(true); }
            break;
        case STROBE_SINGLE:
            if((msNow - _msPrev) > _msPeriodHigh){
                set_show(false);
                _state = BEACON_OFF;
            }
            break;
        case STROBE_MULTI:
            {
                timestamp_t et = msNow - _msPrev;
                if((!_enlit) && (et > _msPeriodLow)){
                    _msPrev = msNow;
                    set_show(true);
                }
                else if((_enlit) && (et > _msPeriodHigh)){
                    _msPrev = msNow;
                    set_show(false);
                }
            }
            break;
        case STROBE_DTAP:
            {
                timestamp_t et = msNow - _msPrev;
                bool state;
                if(et <= _msPeriodHigh){
                    int perc = (100*et)/_msPeriodHigh;
                    state = ((perc < 33) || (perc > 67));
                }
                else if(et <= (_msPeriodHigh + _msPeriodLow)){
                    state = false;
                }
                else {
                    state = true;
                    _msPrev = msNow;
                }
                if(_enlit != state) set_show(state);
            }
            break;
        default:
            set_show(false);
            break;
    }
}

void LEDBeacon::init()
{
        _msPeriodHigh = _msPeriodLow = 0;
        _msPrev = TIME_TBD;
        if(ID8_VALID(_iopin)){
            IO_CONFIG(_iopin, OUTPUT, 0);
            set_show(false);
            _inited = true;
        }
}

void LEDBeacon::strobe(timestamp_t msHigh, timestamp_t msLow)
{
    _msPeriodHigh = msHigh; _msPeriodLow = msLow;
    _msPrev = TIME_TBD;
    if(msLow == 0){
        set_mode((msHigh == 0) ? BEACON_OFF : STROBE_SINGLE);
    }
    else {
        set_mode(STROBE_MULTI);
    }
}



