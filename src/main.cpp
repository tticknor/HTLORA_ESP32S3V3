#include <Arduino.h>
#include <pins_arduino.h>
#include <microops.h>
#include <basic_ssd1306.h>
#include "lvgui.h"

//#include "dsps_fft2r_platform.h"

// extern "C" {
// int s3_add16x8(int16_t *pA, int16_t *pB, int16_t *pC);
// }
// // 128-bit (16-byte) loads and stores need to be 16-byte aligned
// int16_t __attribute__((aligned (16))) u16_A[8] = {0x00, -0x100, 0x00, 0x1111, 0x00, 0x1234, 0x00, 0x7fff};
// int16_t __attribute__((aligned (16))) u16_B[8] = {0x00, 0x3000, 0x00, 0x2222, 0x00, 0x4321, 0x00, 0x4000};
// int16_t __attribute__((aligned (16))) u16_C[8] = {0};

// void test_simd() {
//     Serial.println("About to call Asm code");
//     s3_add16x8(u16_A, u16_B, u16_C);
//     Serial.println("Returned from Asm code");
//     for (int i=0; i<8; i++) {
//         Serial.printf("value %d = 0x%04x\n", i, u16_C[i]);
//     }
// }

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


