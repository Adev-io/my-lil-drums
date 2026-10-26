// Widgets.hpp: the piano roll, page tabs, velocity lane and 4x4 pad grid. Drawn with NanoVG, edit the module's data directly.
// Author: Ari Klopfer / A_Development · MIT
#pragma once
#include "Drums.hpp"

namespace ui_colors
{
inline NVGcolor cell() { return nvgRGB (0x20, 0x20, 0x27); }
inline NVGcolor cellBeat() { return nvgRGB (0x2c, 0x2c, 0x36); }
inline NVGcolor cellOff() { return nvgRGB (0x14, 0x14, 0x18); }
inline NVGcolor lime (int a = 255) { return nvgRGBA (0xc8, 0xff, 0x3a, a); }
inline NVGcolor magenta (int a = 255) { return nvgRGBA (0xff, 0x3d, 0x9a, a); }
inline NVGcolor dim() { return nvgRGB (0x8d, 0x8d, 0x96); }
}

inline void drawText (const Widget::DrawArgs& a, const std::string& s, float x, float y, float size, NVGcolor c, int align = NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE)
{
    std::shared_ptr<window::Font> font = APP->window->loadFont (asset::system ("res/fonts/ShareTechMono-Regular.ttf"));
    if (! font) return;
    nvgFontFaceId (a.vg, font->handle);
    nvgFontSize (a.vg, size);
    nvgFillColor (a.vg, c);
    nvgTextAlign (a.vg, align);
    nvgText (a.vg, x, y, s.c_str(), nullptr);
}

// Mouse position that follows a drag, in this widget's pixels (the rack may be zoomed).
struct DragWidget : OpaqueWidget
{
    Vec dragPos;
    void onButton (const ButtonEvent& e) override
    {
        if (e.action == GLFW_PRESS) { dragPos = e.pos; e.consume (this); }
    }
    void onDragMove (const DragMoveEvent& e) override { dragPos = dragPos.plus (e.mouseDelta.div (APP->scene->rackScroll->getZoom())); }
};

// 16 rows (pad 16 on top, pad 1 at the bottom) x 16 columns of the current page. Left click paints or erases, right click erases.
struct Roll : DragWidget
{
    Drums* m = nullptr;
    int paint = 0;   // value a drag writes: 0 erase, 100 write

    float gutter() const { return mm2px (6.f); }
    bool cellAt (Vec pos, int& pad, int& step) const
    {
        const float cw = (box.size.x - gutter()) / 16, ch = box.size.y / 16;
        if (pos.x < gutter() || pos.y < 0) return false;
        const int c = (int) ((pos.x - gutter()) / cw), row = (int) (pos.y / ch);
        if (c < 0 || c > 15 || row < 0 || row > 15) return false;
        pad = 15 - row;
        step = m->page.load() * 16 + c;
        return step < MAXSTEPS;
    }
    void apply (Vec pos)
    {
        int pad, step;
        if (! m || ! cellAt (pos, pad, step)) return;
        m->seq.pat[m->seq.cur].vel[pad][step] = (uint8_t) paint;
        m->sel = pad;
    }
    void onButton (const ButtonEvent& e) override
    {
        DragWidget::onButton (e);
        if (! m || e.action != GLFW_PRESS || (e.button != GLFW_MOUSE_BUTTON_LEFT && e.button != GLFW_MOUSE_BUTTON_RIGHT)) return;
        int pad, step;
        if (cellAt (e.pos, pad, step))
            paint = (e.button == GLFW_MOUSE_BUTTON_RIGHT || m->seq.pat[m->seq.cur].vel[pad][step]) ? 0 : 100;
        else if (e.pos.x < gutter() && e.pos.y >= 0)   // row number: select that pad and hear it
        {
            const int p = 15 - (int) (e.pos.y / (box.size.y / 16));
            if (p >= 0 && p < PADS) { m->sel = p; m->audition[p] = true; }
            return;
        }
        apply (e.pos);
    }
    void onDragMove (const DragMoveEvent& e) override { DragWidget::onDragMove (e); apply (dragPos); }

    void draw (const DrawArgs& a) override
    {
        namespace C = ui_colors;
        const float g = gutter(), cw = (box.size.x - g) / 16, ch = box.size.y / 16;
        const int pg = m ? m->page.load() : 0, sel = m ? m->sel.load() : 0;
        const Pattern* pt = m ? &m->seq.pat[m->seq.cur] : nullptr;
        const int play = m ? m->seq.clk.step : -1;
        for (int row = 0; row < 16; ++row)
        {
            const int pad = 15 - row;
            drawText (a, std::to_string (pad + 1), g / 2, row * ch + ch / 2, 9.f, pad == sel ? C::magenta() : C::dim());
            for (int c = 0; c < 16; ++c)
            {
                const int s = pg * 16 + c;
                const bool inLen = ! pt || s < pt->length;
                nvgBeginPath (a.vg);
                nvgRect (a.vg, g + c * cw + 0.5f, row * ch + 0.5f, cw - 1.f, ch - 1.f);
                nvgFillColor (a.vg, ! inLen ? C::cellOff() : ((c / 4) & 1) ? C::cellBeat() : C::cell());
                nvgFill (a.vg);
                if (pt && pt->vel[pad][s])
                {
                    nvgFillColor (a.vg, C::lime (90 + pt->vel[pad][s] * 165 / 127));
                    nvgFill (a.vg);
                }
                if (s == play)
                {
                    nvgStrokeColor (a.vg, C::magenta());
                    nvgStrokeWidth (a.vg, 1.f);
                    nvgStroke (a.vg);
                }
            }
        }
    }
};

// Four page tabs: steps 1-16, 17-32, 33-48, 49-64. A dot marks the page that is playing.
struct Tabs : OpaqueWidget
{
    Drums* m = nullptr;
    void onButton (const ButtonEvent& e) override
    {
        if (m && e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT)
        {
            m->page = clamp ((int) (e.pos.x / (box.size.x / 4)), 0, 3);
            e.consume (this);
        }
    }
    void draw (const DrawArgs& a) override
    {
        namespace C = ui_colors;
        const float w = box.size.x / 4;
        const int pg = m ? m->page.load() : 0, len = m ? m->seq.pat[m->seq.cur].length : 16, play = m ? m->seq.clk.step : -1;
        for (int i = 0; i < 4; ++i)
        {
            nvgBeginPath (a.vg);
            nvgRect (a.vg, i * w + 1.f, 0.f, w - 2.f, box.size.y);
            nvgFillColor (a.vg, i == pg ? C::lime (200) : C::cell());
            nvgFill (a.vg);
            const NVGcolor ink = i == pg ? nvgRGB (0x16, 0x16, 0x1a) : (i * 16 < len ? nvgRGB (0xf2, 0xf2, 0xf4) : C::dim());
            drawText (a, std::to_string (i * 16 + 1) + "-" + std::to_string (i * 16 + 16), i * w + w / 2, box.size.y / 2, 9.f, ink);
            if (play >= 0 && play / 16 == i)
            {
                nvgBeginPath (a.vg);
                nvgCircle (a.vg, i * w + 5.f, box.size.y / 2, 2.f);
                nvgFillColor (a.vg, C::magenta());
                nvgFill (a.vg);
            }
        }
    }
};

// Velocity bars of the selected pad for the current page. Drag over a lit step to set its velocity.
struct VelLane : DragWidget
{
    Drums* m = nullptr;
    void apply (Vec pos)
    {
        if (! m) return;
        const int c = (int) (pos.x / (box.size.x / 16)), s = m->page.load() * 16 + c;
        if (c < 0 || c > 15 || s >= MAXSTEPS) return;
        uint8_t& v = m->seq.pat[m->seq.cur].vel[m->sel.load()][s];
        if (v) v = (uint8_t) clamp ((int) (1 + (1.f - pos.y / box.size.y) * 126), 1, 127);
    }
    void onButton (const ButtonEvent& e) override { DragWidget::onButton (e); if (e.action == GLFW_PRESS) apply (e.pos); }
    void onDragMove (const DragMoveEvent& e) override { DragWidget::onDragMove (e); apply (dragPos); }
    void draw (const DrawArgs& a) override
    {
        const float cw = box.size.x / 16;
        for (int c = 0; c < 16; ++c)
        {
            const int s = (m ? m->page.load() : 0) * 16 + c;
            const int v = m ? m->seq.pat[m->seq.cur].vel[m->sel.load()][s] : 0;
            if (! v) continue;
            const float h = box.size.y * v / 127.f;
            nvgBeginPath (a.vg);
            nvgRect (a.vg, c * cw + 1.5f, box.size.y - h, cw - 3.f, h);
            nvgFillColor (a.vg, ui_colors::lime());
            nvgFill (a.vg);
        }
    }
};

// 4x4 pads, pad 1 bottom-left. A click selects the pad and plays it. The caption names the selected pad.
struct PadGrid : OpaqueWidget
{
    Drums* m = nullptr;
    static constexpr float GAP = 1.5f, CAP = 4.f;   // mm
    float pw() const { return (box.size.x - 3 * mm2px (GAP)) / 4; }
    float ph() const { return (box.size.y - mm2px (CAP) - 3 * mm2px (GAP)) / 4; }
    Vec origin (int p) const { return Vec ((p % 4) * (pw() + mm2px (GAP)), (3 - p / 4) * (ph() + mm2px (GAP))); }

    void onButton (const ButtonEvent& e) override
    {
        if (! m || e.action != GLFW_PRESS || e.button != GLFW_MOUSE_BUTTON_LEFT) return;
        for (int p = 0; p < PADS; ++p)
        {
            const Vec o = origin (p);
            if (e.pos.x >= o.x && e.pos.x < o.x + pw() && e.pos.y >= o.y && e.pos.y < o.y + ph())
            {
                m->sel = p;
                m->audition[p] = true;
                e.consume (this);
                return;
            }
        }
    }
    void draw (const DrawArgs& a) override
    {
        namespace C = ui_colors;
        const int sel = m ? m->sel.load() : 0;
        for (int p = 0; p < PADS; ++p)
        {
            const Vec o = origin (p);
            const float f = m ? m->flash[p] : 0.f;
            nvgBeginPath (a.vg);
            nvgRoundedRect (a.vg, o.x, o.y, pw(), ph(), 3.f);
            nvgFillColor (a.vg, nvgRGB (0x3a, 0x3a, 0x44));
            nvgFill (a.vg);
            if (f > 0.f) { nvgFillColor (a.vg, C::lime ((int) (f * 220))); nvgFill (a.vg); }
            nvgStrokeColor (a.vg, p == sel ? C::magenta() : nvgRGB (0x55, 0x55, 0x5f));
            nvgStrokeWidth (a.vg, p == sel ? 2.f : 1.f);
            nvgStroke (a.vg);
            drawText (a, std::to_string (p + 1), o.x + 4.f, o.y + 6.f, 8.f, C::dim(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            if (m) drawText (a, m->padName (p).substr (0, 8), o.x + pw() / 2, o.y + ph() / 2 + 3.f, 10.f, nvgRGB (0xf2, 0xf2, 0xf4));
        }
        if (m) drawText (a, "PAD " + std::to_string (sel + 1) + "  " + m->padName (sel), 2.f, box.size.y - mm2px (CAP) / 2, 10.f, C::lime(), NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    }
};
