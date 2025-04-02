#include "bus_guards.h"

namespace{

}

SemaphoreHandle_t GetSPI0Mutex(){
    static StaticSemaphore_t _mutex_buffer_;
    static SemaphoreHandle_t _hMutex_ = NULL;
    if(!(bool)_hMutex_) { _hMutex_ = xSemaphoreCreateMutexStatic(&_mutex_buffer_); }
    return _hMutex_;
}

SemaphoreHandle_t GetSPI1Mutex(){
    static StaticSemaphore_t _mutex_buffer_;
    static SemaphoreHandle_t _hMutex_ = NULL;
    if(!(bool)_hMutex_) { _hMutex_ = xSemaphoreCreateMutexStatic(&_mutex_buffer_); }
    return _hMutex_;
}

SemaphoreHandle_t GetWire0Mutex(){
    static StaticSemaphore_t _mutex_buffer_;
    static SemaphoreHandle_t _hMutex_ = NULL;
    if(!(bool)_hMutex_) { _hMutex_ = xSemaphoreCreateMutexStatic(&_mutex_buffer_); }
    return _hMutex_;
}

SemaphoreHandle_t GetWire1Mutex(){
    static StaticSemaphore_t _mutex_buffer_;
    static SemaphoreHandle_t _hMutex_ = NULL;
    if(!(bool)_hMutex_) { _hMutex_ = xSemaphoreCreateMutexStatic(&_mutex_buffer_); }
    return _hMutex_;
}

iic_bus_context GetWire0Context(){
    return iic_bus_context{&Wire, GetWire0Mutex()};
}

iic_bus_context GetWire1Context(){
    return iic_bus_context{&Wire1, GetWire1Mutex()};
}

#pragma region spi_bus_context

spi_bus_context::spi_bus_context(spi_halc_t *spi, SemaphoreHandle_t hmux)
    : _spi{spi}, _hMutex{hmux}, _presiding{false}, _connected{false}
{
    if((bool)_hMutex){
        BaseType_t sxs = xSemaphoreTake(_hMutex, portMAX_DELAY);
        _presiding = (sxs == pdTRUE);
    }

}

bool spi_bus_context::connect(uint8_t cs_pin, SPISettings settings)
{
    bool rslt = _presiding && !_connected && (bool)_spi && (cs_pin < 0x80);
    if(rslt){
        _spi->beginTransaction(settings);
        _cs.select(true, cs_pin);
        delayMicroseconds(2);
        _connected = true;
    }
    return rslt;
}

void spi_bus_context::disconnect()
{
    if(_connected){
        _connected = false;
        _spi->endTransaction();
    }
    _cs.select(false);
}

void spi_bus_context::release()
{
    if(_presiding){
        _presiding = false;
        disconnect();
        xSemaphoreGive(_hMutex);
        _hMutex = NULL;
    }
}

#pragma endregion


#pragma region iic_bus_context

iic_bus_context::iic_bus_context(iic_halc_t *i2c, SemaphoreHandle_t hmux)
    : _iic{i2c}, _hMutex{hmux}, _presiding{false}, _address{0xFF}, _connected{false}
{
    if((bool)_hMutex){
        BaseType_t sxs = xSemaphoreTake(_hMutex, portMAX_DELAY);
        _presiding = (sxs == pdTRUE);
    }
}

bool iic_bus_context::connect(uint8_t address) const
{
    bool sxs = _presiding;
    if(sxs && (address < 0x80)){
        _iic->beginTransmission(address);
        _address = address;
        _connected = true;
    }
    return sxs;
}

int iic_bus_context::write(uint8_t val) const
{
    int n = _connected ? 1 : 0;
    if(n > 0){
        _iic->write(val);
    }
    return n;
}

int iic_bus_context::write(const uint8_t *vals, size_t count) const
{
    return 0;
}

uint8_t iic_bus_context::disconnect() const
{
    uint8_t sxs = 0xFF;
    if(_connected){
        sxs = _iic->endTransmission(true);
        _address = 0xFF;
        _connected = false;
    }
    return sxs;
}

void iic_bus_context::release()
{
    if(_presiding){
        disconnect();
        _presiding = false;
        BaseType_t sxs = xSemaphoreGive(_hMutex);
        if(sxs == pdTRUE){
            _hMutex = NULL;
        }
        else{
            log_e("I2C Semaphore release failure");
        }
    }
}

#pragma endregion

