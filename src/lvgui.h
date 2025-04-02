#pragma once
#include <Arduino.h>
#include <basic_ssd1306.h>
#include <lvgl.h>

class HT8KP_Display;
class Marquee;

namespace LVGUI_HT8K{
    
    extern HT8KP_Display Display;

    void initialize();

    uint32_t update(uint32_t msNow);

}

namespace LVGUI = LVGUI_HT8K;

class Marquee{
public:
    lv_obj_t* parent = nullptr;
    lv_obj_t* core = nullptr;
    int32_t width;
    lv_align_t align;

    Marquee(int32_t width_, lv_align_t align_) : width{width_}, align{align_} {}

    bool forge(lv_obj_t* pa);
    void set_text(const char* txt) { if((bool)core) { lv_label_set_text(core, txt); } }

};

class HT8KP_Display : public SSD1306_128x64{
public:
    static constexpr size_t GFRAMEBUFFER_SIZE = WIDTH*HEIGHT*LV_COLOR_DEPTH/8;
    static void flush_cb(lv_display_t * display, const lv_area_t * area, uint8_t * px_map);

protected:
    uint8_t _lvframebuf_[GFRAMEBUFFER_SIZE + 32];
    lv_display_t* _lvdisp_ = nullptr;
    bool _flipped = false;
    Marquee crawler{WIDTH, LV_ALIGN_BOTTOM_LEFT};

public:
    HT8KP_Display(uint8_t address, uint8_t pinRst, bool flipped): SSD1306_128x64{address, pinRst}, _flipped{flipped}
    {
    }

    void initialize(iic_context_producer ctx_producer);
};

