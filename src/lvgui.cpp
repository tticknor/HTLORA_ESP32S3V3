#include "lvgui.h"
#include <microops.h>

#include <draw/lv_draw_buf.h>

namespace{
    uint32_t lvgl_get_millis() { return millis(); }

    procedure_monitor_o ScreenMon{64};

    void canvas_test(){
        LV_DRAW_BUF_DEFINE_STATIC(draw_buf, 32, 24, LV_COLOR_FORMAT_L8);
        LV_DRAW_BUF_INIT_STATIC(draw_buf);
        lv_color_t pxwhite = lv_color_white();
        lv_color_t pxblack = lv_color_black();
        lv_opa_t pxopa = LV_OPA_100;

        lv_obj_t * canvas = lv_canvas_create(lv_screen_active());
        lv_canvas_set_draw_buf(canvas, &draw_buf);
        lv_obj_align(canvas, LV_ALIGN_TOP_MID, 0, 0);
        lv_canvas_fill_bg(canvas, pxblack, pxopa);

        uint32_t x=0, y=0;
        for(x = 0; x < 32; ++x){
            y = (2*x)/3 + 1;
            lv_canvas_set_px(canvas, x, 0, pxwhite, pxopa);
            lv_canvas_set_px(canvas, x, 23, pxwhite, pxopa);
            lv_canvas_set_px(canvas, x, y, pxwhite, pxopa);
            lv_canvas_set_px(canvas, 32-x, y, pxwhite, pxopa);
        }
    }

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

    ::canvas_test();

    crawler.forge(mscreen);
    // crawler.set_text("O, si vile, si ergo, Fortibus es inero! O nobile, demis trux. Vadis indem? Causem dux.");
    crawler.set_text("The monster in his consternation demonstrates defenestration.");

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
