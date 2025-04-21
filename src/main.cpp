#include <Arduino.h>
#include <algorithm>
#include <pins_arduino.h>
#include <microops.h>
#include <basic_ssd1306.h>
#include "lvgui.h"

__attribute__((always_inline)) inline
float recipsf2(float input) {
    float result, temp;
    asm(
        "recip0.s %0, %2\n"
        "const.s %1, 1\n"
        "msub.s %1, %2, %0\n"
        "madd.s %0, %0, %1\n"
        "const.s %1, 1\n"
        "msub.s %1, %2, %0\n"
        "maddn.s %0, %0, %1\n"
        :"=&f"(result),"=&f"(temp):"f"(input)
    );
    return result;
}

#define DIV(a, b) (a)*recipsf2(b)

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    // #define UI16_TO_BBE(v) (((v << 8) & 0xFF00) | ((v >> 8) & 0x00FF))
    #define UI16_TO_BBE(v) (((v << 8) | (v >> 8)) & 0xFFFF)
#else
    #define UI16_TO_BBE(v) v 
#endif

void testfill(void* dst, size_t count, uint16_t color){
    color = UI16_TO_BBE(color);
    uint16_t* beg = (uint16_t*)dst;
    std::fill_n(beg, count, color);
}

namespace PLANT {
    void VextEnable(bool state){
        pinMode(Vext, OUTPUT);
        digitalWrite(Vext, state ? 0 : 1);
    }

    constexpr uint8_t USER_SWITCH = 0;
    LEDBeacon beacon{LED_BUILTIN};
    EpochsButton<3> buttonP{USER_SWITCH, {500, 1000, 2000}};

    void intialize(){
        VextEnable(true);
        beacon.init();
        Wire.begin(SDA_OLED, SCL_OLED, 500'000);
        beacon.double_strobe(300, 1700);
        // delay(3000);
    }

    timestamp_t update(){
        timestamp_t msNow = get_millis();
        beacon.update(msNow);
        buttonP.update(msNow);
        return msNow;
    }

}

void setup() {
    Serial.begin(115200);
    PLANT::intialize();
    LVGUI::initialize();
}

void loop() {
    timestamp_t msNow = PLANT::update();
    LVGUI::update(msNow);
    delay(4);
}


