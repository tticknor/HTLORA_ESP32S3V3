#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <FreeRTOS.h>

typedef decltype(SPI) spi_halc_t;
typedef decltype(Wire) iic_halc_t;

class spi_bus_context;
class iic_bus_context;

SemaphoreHandle_t GetSPI0Mutex();
SemaphoreHandle_t GetSPI1Mutex();
SemaphoreHandle_t GetWire0Mutex();
SemaphoreHandle_t GetWire1Mutex();

iic_bus_context GetWire0Context();
iic_bus_context GetWire1Context();

typedef spi_bus_context (*spi_context_producer)();
typedef iic_bus_context (*iic_context_producer)();

struct cs_signal_t{
    static constexpr uint8_t LEVEL_TRUE = 0;
    static constexpr uint8_t LEVEL_FALSE = 1;
    uint8_t         _pin_;
    bool            _state_;

    void _active(bool state){
        if((_pin_ < 128) && (state != _state_)){
            if(state){ pinMode(_pin_, OUTPUT); }
            digitalWrite(_pin_, state ? LEVEL_TRUE : LEVEL_FALSE);
            _state_ = state;
        }
    }

    cs_signal_t(uint8_t pin) : _pin_{pin}, _state_{false} {}
    cs_signal_t() : cs_signal_t{255} {}

    void select(bool state, uint8_t pin = 255){
        if((pin < 128) && (pin != _pin_)){
            if((_pin_ < 128) && _state_) { _active(false); delayMicroseconds(2); }
            _pin_ = pin;
        }
        _active(state);
    }

};


class spi_bus_context {
public:
    inline static SPISettings DefaultSettings{4'000'000, SPI_MSBFIRST, SPI_MODE0};
public: //protected:
    spi_halc_t*         _spi;
    SemaphoreHandle_t   _hMutex;
    cs_signal_t         _cs;
    bool                _presiding;
    bool                _connected = false;

public:
    spi_bus_context(spi_halc_t* spi, SemaphoreHandle_t hmux);
    spi_bus_context() : spi_bus_context{nullptr, NULL} {}
    spi_bus_context(const spi_bus_context&) = delete;
    spi_bus_context(spi_bus_context&&) = delete;
    spi_bus_context& operator=(const spi_bus_context&) = delete;
    spi_bus_context& operator=(spi_bus_context&&) = delete;
    void release();
    ~spi_bus_context() { release(); }

    bool connect(uint8_t cs_pin, SPISettings settings = spi_bus_context::DefaultSettings);
    void disconnect();

    inline spi_halc_t* get_spi() { return _presiding ?  _spi : nullptr; }
    inline bool has_spi() const { return _presiding; }

};

class iic_bus_context {
public:
public: //protected:
    iic_halc_t*         _iic;
    SemaphoreHandle_t   _hMutex;
    bool                _presiding;
    mutable bool        _connected = false;     
    mutable uint8_t     _address = 0xFF;

    void _write() const {}
    void _write(uint8_t v1) const;
    
    template<typename T2, typename... Ts>
    void _write(uint8_t v1, T2 v2, Ts... rest) const;

    template<typename T2, typename... Ts>
    void _write(const uint8_t* vec, T2 v2, Ts... rest) const;

    template<typename T2, typename... Ts>
    void _write(double dval, T2 v2, Ts... rest) const;  // repeat (uint8_t)(dval) v2 times

public:
    iic_bus_context(iic_halc_t* i2c, SemaphoreHandle_t hmux);
    iic_bus_context() : iic_bus_context{nullptr, NULL} {}
    iic_bus_context(const iic_bus_context&) = delete;
    iic_bus_context(iic_bus_context&&) = delete;
    iic_bus_context& operator=(const iic_bus_context&) = delete;
    iic_bus_context& operator=(iic_bus_context&&) = delete;
    void release();
    ~iic_bus_context() { release(); }

    bool connect( uint8_t address ) const;
    int write(uint8_t val) const;
    int write(const uint8_t* vals, size_t count) const;
    uint8_t disconnect() const;

    template<typename T1, typename... Ts>
    int transmit(uint8_t address, T1 v1, Ts... rest) const;

    inline iic_halc_t* get_i2c() { return _presiding ? _iic : nullptr; }
    inline bool has_i2c() { return _presiding; }

};

inline void iic_bus_context::_write(uint8_t v1) const
{
    if(_connected) { 
        _iic->write(v1); 
    }
}

template <typename T2, typename... Ts>
inline void iic_bus_context::_write(const uint8_t *vec, T2 v2, Ts... rest) const
{
    if(_connected){
        _iic->write(vec, static_cast<size_t>(v2));
    }
    _write(rest...);
}

template <typename T2, typename... Ts>
inline void iic_bus_context::_write(double dval, T2 v2, Ts... rest) const
{
    if(_connected){
        int count = (int)v2;
        if((0.0 <= dval) && (dval < 256.0) && (count > 0)){
            uint8_t v1 = (uint8_t)dval;
            while((count--) > 0){
                _iic->write(v1);
            }
        }
    }
    _write(rest...);
}

template <typename T2, typename... Ts>
inline void iic_bus_context::_write(uint8_t v1, T2 v2, Ts... rest) const
{
    if(_connected){
        _write(v1);
    }
    _write(v2, rest...);
}

template <typename T1, typename... Ts>
inline int iic_bus_context::transmit(uint8_t address, T1 v1, Ts... rest) const
{
    if(_connected) { disconnect(); delayMicroseconds(20); }
    int rslt = -1;
    if(connect(address)){
        rslt = 0;
        _write(v1, rest...);
    }
    return rslt;
}
