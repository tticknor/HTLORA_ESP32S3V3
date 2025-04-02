#pragma once
#include <Arduino.h>
#include "chrono_gear.h"
#include "bus_guards.h"
#include "io_controls.h"
#include "anthropics.h"


#define GetSPI0Context() spi_bus_context{&SPI, GetSPI0Mutex()}
#define GetWire0Context() iic_bus_context{&Wire, GetWire0Mutex()}


