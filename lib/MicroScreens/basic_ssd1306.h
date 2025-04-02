#pragma once
#include <Arduino.h>
#include <microops.h>
#include <initializer_list>

#include "imgab1.h"
#include "imgab1_packed.h"

// #if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
//     #if OLED_DEBUG_ON > 0
//         #define log_oled(format, ...) log_printf(ARDUHAL_LOG_FORMAT(I, format), ##__VA_ARGS__)
//     #else
//         #define log_oled(format, ...) do {} while(0)
//     #endif
// #else
//     #define log_oled(format, ...) do {} while(0)
// #endif

#if OLED_DEBUG_ON > 0
    #define log_oled log_i
#else
    #define log_oled(format, ...) do {} while(0)
#endif

namespace PACKBITS {
    int strip_pack_le(uint8_t* dst, const uint8_t *src, int count, int aside, int step, uint8_t thresh);
    int strip_pack_be(uint8_t* dst, const uint8_t *src, int count, int aside, int step, uint8_t thresh);

}

namespace SSD1306 {
    constexpr uint8_t ADDRESS_0 =       0x3C;
    constexpr uint8_t ADDRESS_1 =       0x3D;

	namespace REGS{
        constexpr uint8_t LOCOL_ADDR_BASE =   	0x00;
        constexpr uint8_t HICOL_ADDR_BASE =   	0x10;
        constexpr uint8_t SET_ADDR_MODE =     	0x20;
        constexpr uint8_t SET_COL_ADDR =      	0x21;
        constexpr uint8_t SET_PAGE_ADDR =     	0x22;
        constexpr uint8_t DEACTIVATE_SCROLL = 	0x2E;
	
        constexpr uint8_t STARTLINE_BASE =    	0x40;
	
        constexpr uint8_t BANK0_CONTRAST =    	0x81;
        constexpr uint8_t CHARGE_PUMP =       	0x8D;
	
        constexpr uint8_t COL_FLIP_NO =       	0xA0;
        constexpr uint8_t COL_FLIP_YES =      	0xA1;
	
        constexpr uint8_t DISPLAYALLON_RESUME = 0xA4;
        constexpr uint8_t DISPLAYALLON_RAM =    0xA5;
        constexpr uint8_t PIXEL_REV_NO =      	0xA6;
        constexpr uint8_t PIXEL_REV_YES =     	0xA7;
        constexpr uint8_t SET_MUX_RATIO =     	0xA8;
        constexpr uint8_t DISPLAY_SLEEP =     	0xAE;
        constexpr uint8_t DISPLAY_WAKE =      	0xAF;

        constexpr uint8_t ROW_FLIP_NO =       	0xC0;
        constexpr uint8_t ROW_FLIP_YES =      	0xC8;
        constexpr uint8_t SET_DISP_OFFSET =   	0xD3;
        constexpr uint8_t SET_CLOCK_DIV =     	0xD5;
        constexpr uint8_t SET_PRECHARGE =     	0xD9;
        constexpr uint8_t SET_COM_SCAN =      	0xDA;
        constexpr uint8_t SET_VCOM_DESELECT = 	0xDB;

        constexpr uint8_t Init1306_128x64_IntVcc[] = {
            26,
            DISPLAY_SLEEP,
            // SET_CLOCK_DIV, 0x80,
            SET_CLOCK_DIV, 0xF0,
            SET_MUX_RATIO, 0x3F,
            SET_DISP_OFFSET, 0x00, STARTLINE_BASE | 0x00,
            CHARGE_PUMP, 0x14,
            SET_ADDR_MODE, 0x00,
            COL_FLIP_NO, ROW_FLIP_NO,
            SET_COM_SCAN, 0x12,
            BANK0_CONTRAST, 0xCF,
            SET_PRECHARGE, 0xF1,
            SET_VCOM_DESELECT, 0x40,
            DISPLAYALLON_RESUME,
            PIXEL_REV_NO,
            DEACTIVATE_SCROLL,
            DISPLAY_WAKE
        };	
	}
	
	namespace HERALD {
        constexpr uint8_t COMMANDS =   			0x00;
        constexpr uint8_t COL_RANGE =  			0x21; 
        constexpr uint8_t PAGE_RANGE = 			0x22;
        constexpr uint8_t DATA =       			0x40;
        constexpr uint8_t COMMAND1 =            0x80;
        constexpr uint8_t HERALDNONE = 			0xFF;
	}


} // namespace SSD1306

template<size_t W, size_t H>
class TransverseScreenTile{
public:
    static constexpr uint8_t xlim = (uint8_t)(W-1);
    static constexpr uint8_t ylim = (uint8_t)(H-1);

protected:
    uint8_t _x0, _y0, _xm, _ym;

public:
    TransverseScreenTile(int x, int y, int w, int h){
        _x0 = std::clamp<int>(x, 0, xlim);
        _y0 = std::clamp<int>(y, 0, ylim);
        _xm = (w > 0) ? std::clamp<int>(x+w-1, _x0, xlim) : xlim;
        _ym = (h > 0) ? std::clamp<int>(y+h-1, _y0, ylim) : ylim;
    }

    inline uint8_t s0() const { return _y0 >> 3; }
    inline uint8_t sm() const { return _ym >> 3; }
    inline uint8_t x0() const { return _x0; }
    inline uint8_t xm() const { return _xm; }
    inline bool isAligned() const { return (_y0 & 0x07) == 0; }

    inline size_t osBegin() const { return W*(_y0 >> 3) + _x0; }
    inline size_t osEnd() const { return W*(_ym >> 3) + _xm + 1; }
    inline size_t getWidth() const { return _xm - _x0 + 1; }

};

class SSD1306_BASIC{
public:
    static int pack_gray_stripe(const uint8_t* gsrc, uint8_t* bdst, int count, int step, int stride, uint8_t thresh);

protected:
    int                         _block_size = 32;
    uint8_t                     _address = ID8_NULL;
    uint8_t                     _pin_rst = ID8_NULL;

    void _hw_reset(){
        uint32_t dt = IO_PULSE_MILLIS(_pin_rst, 0, 2, 1, 1);
        log_oled("OLED: Hardware Reset on pin #%u for ~ %u milliseconds", _pin_rst, dt);
    }
    void _send_data(const iic_bus_context*pctx, const uint8_t* data, int count);
    void _send_data_rept(const iic_bus_context* pctx, uint8_t val, int count);

    int _set_window(const iic_bus_context* pctx, uint8_t x0, uint8_t x1, uint8_t s0, uint8_t s1);
    void _refresh(const iic_bus_context* pctx, uint8_t x0, uint8_t x1, uint8_t s0, uint8_t s1, const uint8_t* imgdata);
    void _refresh(const iic_bus_context* pctx, const uint8_t* imgdata) { _refresh(pctx, (uint8_t)0, (uint8_t)127, (uint8_t)0, (uint8_t)7, imgdata);}
    void _flip_xy(const iic_bus_context* pctx, bool flipcols, bool fliprows);

public:
    SSD1306_BASIC(uint8_t address, uint8_t rst) : _address{address} , _pin_rst{rst} {}


};

class SSD1306_128x64 : public SSD1306_BASIC {
public:
    static constexpr uint32_t WIDTH = 128;
    static constexpr uint32_t HEIGHT = 64;
    static constexpr uint32_t STRIPES = HEIGHT/8;
    static constexpr uint32_t BUFSIZE = WIDTH*STRIPES;
    static constexpr uint32_t PIXCOUNT = WIDTH*HEIGHT;
    using Tile = TransverseScreenTile<WIDTH, HEIGHT>;

protected:
    uint8_t                 _bit_img[BUFSIZE];
    iic_context_producer    _ctxgen = nullptr;

    void _preconfigure( const iic_bus_context* pctx );
    using SSD1306_BASIC::_set_window;
    int _set_window(const iic_bus_context* pctx, const Tile& tile);

    int _display(const iic_bus_context* pctx, int x0, int y0, int wd, int ht);

public:
    SSD1306_128x64(uint8_t address, uint8_t rst = 0xFF) : SSD1306_BASIC{address, rst} {}
    SSD1306_128x64(const SSD1306_128x64&) = delete;
    SSD1306_128x64(SSD1306_128x64&&) = delete;
    SSD1306_128x64& operator=(const SSD1306_128x64&) = delete;
    SSD1306_128x64& operator=(SSD1306_128x64&&) = delete;

    void initialize(iic_context_producer ctx_producer, bool flip180 = false);

    void flood(uint8_t val);

    int drawImageGray(const uint8_t* img, uint16_t x0, uint16_t y0, uint16_t wd, uint16_t ht);
    bool show();
    bool show(uint32_t x0, uint32_t x1, uint32_t y0, uint32_t y1);

    void flipOrient(bool cols, bool rows){
        if((bool)_ctxgen){ auto ctx = _ctxgen(); _flip_xy(&ctx, cols, rows); }
    }

    void test(timestamp_t msNow);

};

    
    
