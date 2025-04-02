#include "basic_ssd1306.h"

namespace{
    uint8_t IconA[1024];

    void ns_init(){
        for(int irow = 0; irow < 32; ++irow){
            for(int icol = 0; icol < 32; ++icol){
                IconA[32*irow + icol] = imgab1[128*irow + icol];
            }
        }
    }

}

namespace PACKBITS {
    int strip_pack_le(uint8_t* dst, const uint8_t *src, int count, int aside, int step, uint8_t thresh)
    {
		if(count > 0) {
			const uint8_t *col = src;
			const uint8_t *pxl;
			uint8_t *tgt = dst;
			uint8_t *fin = dst+count;
			while(tgt < fin){
				pxl = src;
				*tgt = 0;
				for(int ix{8}; ix > 0; ix--){
					*tgt = (*tgt >> 1) | ((*pxl > thresh) ? (uint8_t)0x80 : (uint8_t)0x00);
					pxl += aside;
				}
				src += step;
				++tgt;
			}
		}
        return count;
    }

    int strip_pack_be(uint8_t* dst, const uint8_t *src, int count, int aside, int step, uint8_t thresh)
    {
		if(count > 0) {
			const uint8_t *col = src;
			const uint8_t *pxl;
			uint8_t *tgt = dst;
			uint8_t *fin = dst+count;
			while(tgt < fin){
				pxl = src;
				*tgt = 0;
				for(int ix{8}; ix > 0; ix--){
					*tgt = (*tgt << 1) | ((*pxl > thresh) ? (uint8_t)0x01 : (uint8_t)0x00);
					pxl += aside;
				}
				src += step;
				++tgt;
			}
		}
        return count;

    }

} // namespace PACKBITS

int SSD1306_BASIC::pack_gray_stripe(const uint8_t *gsrc, uint8_t *bdst, int count, int step, int stride, uint8_t thresh)
{
    const uint8_t* src = gsrc;
    const uint8_t* spur;
    uint8_t* dst = bdst;
    uint8_t* dend = bdst + count;
    while(dst < dend){
        spur = src;
        *dst = (*spur > thresh) ? (uint8_t)0x01 : (uint8_t)0x00; spur += stride;
        *dst |= (*spur > thresh) ? (uint8_t)0x02 : (uint8_t)0x00; spur += stride;
        *dst |= (*spur > thresh) ? (uint8_t)0x04 : (uint8_t)0x00; spur += stride;
        *dst |= (*spur > thresh) ? (uint8_t)0x08 : (uint8_t)0x00; spur += stride;
        *dst |= (*spur > thresh) ? (uint8_t)0x10 : (uint8_t)0x00; spur += stride;
        *dst |= (*spur > thresh) ? (uint8_t)0x20 : (uint8_t)0x00; spur += stride;
        *dst |= (*spur > thresh) ? (uint8_t)0x40 : (uint8_t)0x00; spur += stride;
        *dst |= (*spur > thresh) ? (uint8_t)0x80 : (uint8_t)0x00; spur += stride;
        src += step;
        ++dst;
    }
    return dst - bdst;
}

void SSD1306_BASIC::_send_data(const iic_bus_context *pctx, const uint8_t *data, int count)
{
    if((bool)pctx && count > 0){
        const uint8_t* src = data;
        const uint8_t* end = data+count;
        int chunk;
        while(src < end){
            chunk = std::clamp<int>((end-src), 1, _block_size);
            pctx->transmit(_address, SSD1306::HERALD::DATA, src, chunk);
            src += chunk;
        }
    }
}

int SSD1306_BASIC::_set_window(const iic_bus_context *pctx, uint8_t x0, uint8_t x1, uint8_t s0, uint8_t s1)
{
    uint8_t cmdbuf[8];
    if((bool)pctx){
        uint8_t* dst = cmdbuf;
        *(dst++) = SSD1306::HERALD::COMMANDS;
        *(dst++) = SSD1306::REGS::SET_COL_ADDR; *(dst++) = x0; *(dst++) = x1;
        *(dst++) = SSD1306::REGS::SET_PAGE_ADDR; *(dst++) = s0; *(dst++) = s1;
        pctx->transmit(_address, cmdbuf, dst - cmdbuf);
        return (x1 - x0 + 1)*(s1 - s0 + 1);
    }
    return 0;
}

void SSD1306_BASIC::_refresh(const iic_bus_context *pctx, uint8_t x0, uint8_t x1, uint8_t s0, uint8_t s1, const uint8_t *imgdata)
{
    int count = ((x1 >= x0) && (s1 >= s0)) ? (x1-x0+1)*(s1-s0+1) : 0;
    if((bool) pctx && (count > 0)){
        _set_window(pctx, x0, x1, s0, s1);
        _send_data(pctx, imgdata, count);
    }
}

void SSD1306_BASIC::_flip_xy(const iic_bus_context* pctx, bool flipcols, bool fliprows)
{
    uint8_t cmdbuf[4];
    if((bool)pctx){
        uint8_t* dst = cmdbuf;
        *(dst++) = SSD1306::HERALD::COMMANDS;
        *(dst++) = flipcols ? SSD1306::REGS::COL_FLIP_YES : SSD1306::REGS::COL_FLIP_NO;
        *(dst++) = fliprows ? SSD1306::REGS::ROW_FLIP_YES : SSD1306::REGS::ROW_FLIP_NO;
        pctx->transmit(_address, cmdbuf, dst - cmdbuf);
    }
}

void SSD1306_128x64::_preconfigure(const iic_bus_context *pctx)
{
    if((bool)pctx){
        const uint8_t* cmdblock = SSD1306::REGS::Init1306_128x64_IntVcc;
        pctx->transmit(_address, SSD1306::HERALD::COMMANDS, cmdblock + 1, *cmdblock);
    }
    // ::oled_init();
}

int SSD1306_128x64::_set_window(const iic_bus_context *pctx, const Tile &tile)
{
    return _set_window(pctx, tile.x0(), tile.xm(), tile.s0(), tile.sm());
}

int SSD1306_128x64::_display(const iic_bus_context* pctx, int x0, int y0, int wd, int ht)
{
    Tile tile{x0, y0, wd, ht};
    int sxs = _set_window(pctx, tile);
    if(sxs > 0){
        const uint8_t* src = _bit_img + tile.osBegin();
        const uint8_t* end = _bit_img + tile.osEnd();
        size_t ncols = tile.getWidth();
        // _send_data(pctx, _bit_img, 1024);
        while(src < end){
            _send_data(pctx, src, ncols);
            src += WIDTH;
        }
    }
    return sxs;
}

void SSD1306_BASIC::_send_data_rept(const iic_bus_context *pctx, uint8_t val, int count)
{
    int chunk;
    if((bool)pctx){
        double dval = (double)val;
        while(count > 0){
            chunk = std::clamp<int>(count, 1, _block_size);
            pctx->transmit(_address, SSD1306::HERALD::DATA, dval, chunk);
            count -= chunk;
        }
    }
}

void SSD1306_128x64::initialize(iic_context_producer ctx_producer, bool flip180)
{
    _ctxgen = ctx_producer;
    memset(_bit_img, 0, sizeof(_bit_img));
    if((bool)_ctxgen)
    {
        auto ctx = _ctxgen();
        _hw_reset();
        _preconfigure(&ctx);
        if(flip180)
            _flip_xy(&ctx, true, true);
    }

    ::ns_init();
}

void SSD1306_128x64::flood(uint8_t val)
{
    if((bool)_ctxgen){
        auto ctx = _ctxgen();
        _set_window(&ctx, 0, 127, 0, 7);
        _send_data_rept(&ctx, val, 1024);
    }
}

int SSD1306_128x64::drawImageGray(const uint8_t *img,  uint16_t x0, uint16_t y0, uint16_t wd, uint16_t ht)
{
    /*** To Do: Handle mis-aligned tiles ... ***/
    /*** For now, assume the caller is sane [ y is 8-row aligned ] ***/
    bool handled = ((x0 < 128) && (y0 < 64) && (wd > 0) && (ht > 0) && ((y0 & 0x07) == 0));
    if(handled){
        int ncols = (int)(std::min<uint16_t>(x0+wd-1, 127)-x0)+1;
        int nrows = (int)(std::min<uint16_t>(y0+ht-1, 63)-y0)+1;
        const uint8_t* src = img;
        uint8_t* dst = _bit_img + (WIDTH*(y0 >> 3) + x0);
        uint8_t* dend = dst + (WIDTH*((nrows+7)/8));
        while(dst < dend){
            pack_gray_stripe(src, dst, ncols, 1, (int)wd, 0x7F);
            src += (wd*8);
            dst += WIDTH;
        }
    }
    return handled;
}

bool SSD1306_128x64::show()
{
    if((bool)_ctxgen){
        auto ctx = _ctxgen();
        _display(&ctx, 0, 0, 128, 64);
    }
    return true;
}
bool SSD1306_128x64::show(uint32_t x0, uint32_t x1, uint32_t y0, uint32_t y1)
{
    bool sxs = (bool)_ctxgen;
    if(sxs){
        int z0 = (int)(y0 & 0xFFF8);
        int z1 = (int)(y1 & 0xFFF8) + 7;
        auto ctx = _ctxgen();
        _display(&ctx, (int)x0, z0, (int)(x1-x0+1), z1-z0+1);
    }
    return sxs;
}

void SSD1306_128x64::test(timestamp_t msNow)
{
    static int count = -1;
    ++count;
    auto ctx = _ctxgen();
    // memcpy(_bit_img, imgab1_packed, 1024);
    // ::test_pack_A(_bit_img);
    memset(_bit_img, count, 1024);
    switch(count & 0x07){
        case 0:
            drawImageGray(imgab1, 0, 0, 128, 64);
            _display(&ctx, 0, 0, 0, 0);
            break;
        case 2:
            drawImageGray(IconA, 25, 16, 32, 32);
            // _display(&ctx, 64, 0, 64, 32);
            _display(&ctx, 0, 0, 0, 0);
            break;
        case 4:
            drawImageGray(IconA, 50, 32, 32, 32);
            // _display(&ctx, 32, 16, 64, 32);
            _display(&ctx, 0, 0, 0, 0);
            break;
        case 6:
            drawImageGray(IconA, 75, 40, 32, 24);
            // _display(&ctx, 0, 32, 64, 32);
            _display(&ctx, 0, 0, 0, 0);
            break;
        default:
            _display(&ctx, 0, 0, 0, 0);
            break;
    }
}

namespace PACKFNS{
    typedef int16_t locus_t;

    struct PixelRect{
        locus_t     x, y;
        locus_t     w, h;
    };

    class MonoFramebufBase {
    public:
        PixelRect           _rect;
    public:
        
        const PixelRect& peek_rect() const { return _rect; }
        const uint8_t* peek_pixel_from_ul(locus_t x, locus_t y, bool oob_null = true) const { return nullptr; }
        const uint8_t* peek_pixel_from_ll(locus_t x, locus_t y, bool oob_null = true) const { return nullptr; }
        const uint8_t* peek_pixel_from_ur(locus_t x, locus_t y, bool oob_null = true) const { return nullptr; }
        const uint8_t* peek_pixel_from_lr(locus_t x, locus_t y, bool oob_null = true) const { return nullptr; }
    };

    locus_t _packer_phi000(MonoFramebufBase *pfb, uint8_t *dst, locus_t u0, locus_t v0, locus_t max_length)
    {
        auto& rc = pfb->peek_rect();
        const uint8_t* src = pfb->peek_pixel_from_ul((locus_t)u0, (locus_t)v0, true);
        if(!src || ((v0+8) > rc.h)) return 0;
        locus_t count = std::min((locus_t)(rc.w - u0), max_length);
        int stride = rc.w, step = 1;
        return PACKBITS::strip_pack_le(dst, src, count, stride, step, 0x7F);
    }

    locus_t _packer_phi090(MonoFramebufBase *pfb, uint8_t *dst, locus_t u0, locus_t v0, locus_t max_length)
    {
        auto& rc = pfb->peek_rect();
        const uint8_t* src = pfb->peek_pixel_from_ll(v0, u0);
        if(!src || ((v0+8) > rc.w)) return 0;
        locus_t count = std::min((locus_t)(rc.h - u0), max_length);
        int stride = 1, step = -rc.w;
        return PACKBITS::strip_pack_le(dst, src, count, stride, step, 0x7F);
    }

    locus_t _packer_phi180(MonoFramebufBase *pfb, uint8_t *dst, locus_t u0, locus_t v0, locus_t max_length)
    {
        auto& rc = pfb->peek_rect();
        const uint8_t* src = pfb->peek_pixel_from_lr(u0, v0);
        if(!src || ((v0+8) > rc.h)) return 0;
        locus_t count = std::min((locus_t)(rc.w - u0), max_length);
        int stride = -rc.w, step = -1;
        return PACKBITS::strip_pack_le(dst, src, count, stride, step, 0x7F);
    }

    locus_t _packer_phi270(MonoFramebufBase *pfb, uint8_t *dst, locus_t u0, locus_t v0, locus_t max_length)
    {
        auto& rc = pfb->peek_rect();
        const uint8_t* src = pfb->peek_pixel_from_ur(v0, u0);
        if(!src || ((v0+8) > rc.w)) return 0;
        locus_t count = std::min((locus_t)(rc.h - u0), max_length);
        int stride = -1, step = rc.w;
        return PACKBITS::strip_pack_le(dst, src, count, stride, step, 0x7F);
    }


}


