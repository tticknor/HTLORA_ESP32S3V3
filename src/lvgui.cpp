#include "lvgui.h"
#include <microops.h>

namespace{
    uint32_t lvgl_get_millis() { return millis(); }

    procedure_monitor_o ScreenMon{64};

}

namespace LVGUI_HT8K{
    constexpr bool FLIP180 = true;
    HT8KP_Display Display{SSD1306::ADDRESS_0, RST_OLED, FLIP180};
    bool _inited_ = false;

    void initialize()
    {
        if(!_inited_){
            lv_init();
            lv_tick_set_cb(::lvgl_get_millis);
            Display.initialize(GetWire0Context);
            // gui_construct();
            _inited_ = true;
        }
    }

    uint32_t update(uint32_t msNow)
    {
        return lv_timer_handler();
    }

}

void HT8KP_Display::flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
    HT8KP_Display* self = (HT8KP_Display*)lv_display_get_user_data(display);
    uint16_t y0 = area->y1 & 0xFFF8;
    uint16_t y1 = (area->y2 & 0xFFF8) | 0x07;
    self->drawImageGray(self->_lvframebuf_ + (self->WIDTH * y0), 0, y0, 128, y1-y0+1);
    self->show(area->x1, area->x2, area->y1, area->y2);
    lv_display_flush_ready(display);
    
}


void HT8KP_Display::initialize(iic_context_producer ctx_producer)
{
    SSD1306_128x64::initialize(ctx_producer, _flipped);
    flood(0x55);

    _lvdisp_ = lv_display_create(WIDTH, HEIGHT);
    lv_display_set_buffers(_lvdisp_, _lvframebuf_, NULL, GFRAMEBUFFER_SIZE, LV_DISP_RENDER_MODE_DIRECT);
    lv_display_set_user_data(_lvdisp_, this);
    lv_display_set_flush_cb(_lvdisp_, HT8KP_Display::flush_cb);

    lv_obj_t* mscreen = lv_screen_active();
    lv_obj_set_style_bg_color(mscreen, lv_color_hex(0x00), LV_PART_MAIN);
    lv_obj_set_style_text_color(mscreen, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    crawler.forge(mscreen);
    crawler.set_text("O, si vile, si ergo, Fortibus es inero! O nobile, demis trux. Vadis indem? Causem dux.");

}

bool Marquee::forge(lv_obj_t *pa)
{
    if(!(bool)core){
        parent = pa;
        core = lv_label_create(pa);
        lv_label_set_long_mode(core, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_style_anim_duration(core, 30000, LV_PART_MAIN);
        lv_label_set_text(core, "INFO");
        lv_obj_set_width(core, width);
        lv_obj_set_style_text_align(core, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_align(core, align, 0, 0);
        return true;
    }
    return false;
}
