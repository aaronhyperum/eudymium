#include <stdio.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "imgui.h"
#include "draw.h"

#ifdef _MSC_VER
#       define snprintf _snprintf
#endif

static const int BUTTON_HEIGHT = 20;
static const int SLIDER_HEIGHT = 20;
static const int SLIDER_MARKER_WIDTH = 10;
static const int CHECK_SIZE = 8;
static const int DEFAULT_SPACING = 4;
static const int TEXT_HEIGHT = 8;
static const int SCROLL_AREA_PADDING = 6;
static const int INDENT_SIZE = 16;
static const int AREA_HEADER = 28;

bool imgui::isHovered(int x, int y, int w, int h) //privately used only
{
   return state.cursor.x >= x && state.cursor.x <= x+w && state.cursor.y >= y && state.cursor.y <= y+h;
}

bool imgui::isHoveringPanel(unsigned id)
{
    for (int i = panels.size()-1; i >=0; i--)
    {
        Panel p = panels[i];
        if (isHovered(p.x, p.y, p.w, p.h))
        {
            return id == p.id;
        }
    }
    return false;
}

enum ButtonState {BUTTON_NO_EVENT, BUTTON_WENT_ACTIVE, BUTTON_WENT_INACTIVE};
ButtonState buttonLogic(unsigned int id, GuiState* state)
{
        // if button is not active, then make it active on click
        if (state->active == 0)
        {
                if (state->hot == id && state->cursor.is_active)
                {
                    state->active = id;
                    return BUTTON_WENT_ACTIVE;
                }
        }

        // if button is active, then deactivate on click release
        if (state->active == id)
        {
                if (!state->cursor.is_active)
                {
                    state->active = 0;
                    state->scroll = 0;
                    if (state->hot == id) return BUTTON_WENT_INACTIVE;
                }
        }
        //return if deactivated on the button. (button pressed)
        return BUTTON_NO_EVENT;
}

void imgui::update(int mx, int my, bool leftmbut, int scroll)
{
        // Update Inputs
        state.cursor.x = mx;
        state.cursor.y = my;
        state.cursor.is_active = leftmbut;
        state.scroll = scroll;

        state.widgetY = 0;

        state.areaId = 1;
        state.widgetId = 1;
        state.hot = 0;

        panels = next_panels;
        next_panels.clear();

        gfx->reset();
}

void imgui::panel(const char* name, int x, int y, int w, int h, int* scroll, std::function<void()> contents )
{
    state.areaId++;

    state.widgetId = 0;
    scrollId = (state.areaId<<16) | state.widgetId;

    state.widgetY = y+h-AREA_HEADER + (*scroll); //change widget beginning widget height to the scroll offset + the area height.
    scrollTop = y-AREA_HEADER+h;
    scrollBottom = y+SCROLL_AREA_PADDING;
    scrollRight = x+w - SCROLL_AREA_PADDING*3;
    scrollVal = scroll;

    scrollAreaTop = state.widgetY;

    focusTop = y-AREA_HEADER;
    focusBottom = y-AREA_HEADER+h;

    state.panel.x = x;
    state.panel.y = y;
    state.panel.w = w;
    state.panel.h = h;



    gfx->addRoundedRect((float)x, (float)y, (float)w, (float)h, 6, imguiRGBA(0,0,0,192));


    gfx->addText(x+AREA_HEADER/2, y+h-AREA_HEADER/2-TEXT_HEIGHT/2-5, IMGUI_ALIGN_LEFT, name, imguiRGBA(255,255,255,128));

    gfx->addScissor(x+SCROLL_AREA_PADDING, y+SCROLL_AREA_PADDING, w-SCROLL_AREA_PADDING*4, h-AREA_HEADER-SCROLL_AREA_PADDING);


    contents();

    // Disable scissoring.
    gfx->addScissor(-1,-1,-1,-1);

    // Draw scroll bar
    int nx = scrollRight+SCROLL_AREA_PADDING/2;
    int ny = scrollBottom;
    int nw = SCROLL_AREA_PADDING*2;
    int nh = scrollTop - scrollBottom;

    int stop = scrollAreaTop;
    int sbot = state.widgetY;
    int sh = stop - sbot; // The scrollable area height.

    float barHeight = (float)nh/(float)sh;

    if (barHeight < 1)
    {
        float barY = (float)(ny - sbot)/(float)sh;
        if (barY < 0) barY = 0;
        if (barY > 1) barY = 1;

        // Handle scroll bar logic.
        unsigned int hid = scrollId;
        int hx = nx;
        int hy = ny + (int)(barY*nh);
        int hw = nw;
        int hh = (int)(barHeight*nh);

        const int range = nh - (hh-1);
        //run hovering panel later to optimize - most of the time, panels won't overlap.

        if(isHovered(hx, ny, hw, nh) && isHoveringPanel(state.areaId)) state.hot = hid;
        ButtonState bst = buttonLogic(hid, &state);
        if (state.active == hid)
        {
            float u = (state.cursor.y - (ny) - (hh/2)) / (float)range; //set u to the distance from cursor to bottom of scroll (or top? who knows)
            if (u < 0) u = 0;
            if (u > 1) u = 1;
            *scrollVal = (int)((1-u) * (sh - nh));
        }

        // BG
        gfx->addRoundedRect((float)nx, (float)ny, (float)nw, (float)nh, (float)nw/2-1, imguiRGBA(0,0,0,196));
        // Bar
        if (state.active == hid) gfx->addRoundedRect((float)hx, (float)hy, (float)hw, (float)hh, (float)nw/2-1, imguiRGBA(255,196,0,196));
        else gfx->addRoundedRect((float)hx, (float)hy, (float)hw, (float)hh, (float)nw/2-1, state.hot == hid ? imguiRGBA(255,196,0,96) : imguiRGBA(255,255,255,64));
        if (isHovered(state.panel.x, state.panel.y, state.panel.w, state.panel.h))
        {
            *scrollVal += 20*state.scroll;
            if (*scrollVal < 0) *scrollVal = 0;
            if (*scrollVal > (sh - nh)) *scrollVal = (sh - nh);
        }
    }
    next_panels.push_back({x, y, w, h, state.areaId});
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool imgui::button(const char* text, bool enabled)
{
        state.widgetId++;
        unsigned int id = (state.areaId<<16) | state.widgetId;

        int x = state.panel.x + SCROLL_AREA_PADDING + (state.indents * INDENT_SIZE);
        int w = state.panel.w - (SCROLL_AREA_PADDING*4) - (state.indents * INDENT_SIZE);

        int y = state.widgetY - BUTTON_HEIGHT;
        int h = BUTTON_HEIGHT;
        state.widgetY -= BUTTON_HEIGHT + DEFAULT_SPACING;

        if(enabled && isHovered(x, y, w, h) && isHoveringPanel(state.areaId)) state.hot = id;
        bool res = buttonLogic(id, &state) == BUTTON_WENT_INACTIVE;

        gfx->addRoundedRect((float)x, (float)y, (float)w, (float)h, (float)BUTTON_HEIGHT/2-1, imguiRGBA(128,128,128, state.active == id?196:96));
        if (enabled)
                gfx->addText(x+BUTTON_HEIGHT/2, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_LEFT, text, state.hot == id ? imguiRGBA(255,196,0,255) : imguiRGBA(255,255,255,200));
        else
                gfx->addText(x+BUTTON_HEIGHT/2, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_LEFT, text, imguiRGBA(128,128,128,200));

        return res;
}

bool imgui::item(const char* text, bool enabled)
{
        state.widgetId++;
        unsigned int id = (state.areaId<<16) | state.widgetId;

        int x = state.panel.x + SCROLL_AREA_PADDING + (state.indents * INDENT_SIZE);
        int w = state.panel.w - (SCROLL_AREA_PADDING*4) - (state.indents * INDENT_SIZE);

        int y = state.widgetY - BUTTON_HEIGHT;
        int h = BUTTON_HEIGHT;
        state.widgetY -= BUTTON_HEIGHT + DEFAULT_SPACING;

        if(enabled && isHovered(x, y, w, h) && isHoveringPanel(state.areaId)) state.hot = id;
        bool res = buttonLogic(id, &state) == BUTTON_WENT_INACTIVE;

        if (state.hot == id) gfx->addRoundedRect((float)x, (float)y, (float)w, (float)h, 2.0f, imguiRGBA(255,196,0,state.active == id?196:96));
        gfx->addText(x+BUTTON_HEIGHT/2, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_LEFT, text, enabled? imguiRGBA(255,255,255,200):imguiRGBA(128,128,128,200));

        return res;
}

bool imgui::check(const char* text, bool checked, bool enabled)
{
        state.widgetId++;
        unsigned int id = (state.areaId<<16) | state.widgetId;

        int x = state.panel.x + SCROLL_AREA_PADDING + (state.indents * INDENT_SIZE);
        int w = state.panel.w - (SCROLL_AREA_PADDING*4) - (state.indents * INDENT_SIZE);

        int y = state.widgetY - BUTTON_HEIGHT;
        int h = BUTTON_HEIGHT;
        state.widgetY -= BUTTON_HEIGHT + DEFAULT_SPACING;

        if(enabled && isHovered(x, y, w, h) && isHoveringPanel(state.areaId)) state.hot = id;
        bool res = buttonLogic(id, &state) == BUTTON_WENT_INACTIVE;

        const int cx = x+BUTTON_HEIGHT/2-CHECK_SIZE/2;
        const int cy = y+BUTTON_HEIGHT/2-CHECK_SIZE/2;

        // Add the box of the checkbox.
        gfx->addRoundedRect((float)cx-3, (float)cy-3, (float)CHECK_SIZE+6, (float)CHECK_SIZE+6, 4, imguiRGBA(128,128,128, state.active == id?196:96));

        // Add 'tick' in the checkbox.
        if (checked) gfx->addRoundedRect((float)cx, (float)cy, (float)CHECK_SIZE, (float)CHECK_SIZE, (float)CHECK_SIZE/2-1, enabled? imguiRGBA(255,255,255,255) : imguiRGBA(128,128,128,200));

        // Add label to the right of the checkbox.
        gfx->addText(x+BUTTON_HEIGHT, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_LEFT, text, enabled? state.hot == id ? imguiRGBA(255,196,0,255) : imguiRGBA(255,255,255,200) : imguiRGBA(128,128,128,200));

        return res;
}

bool imgui::collapse(const char* text, const char* subtext, bool checked, bool enabled)
{
        state.widgetId++;
        unsigned int id = (state.areaId<<16) | state.widgetId;

        int x = state.panel.x + SCROLL_AREA_PADDING + (state.indents * INDENT_SIZE);
        int w = state.panel.w - (SCROLL_AREA_PADDING*4) - (state.indents * INDENT_SIZE);

        int y = state.widgetY - BUTTON_HEIGHT;
        int h = BUTTON_HEIGHT;
        state.widgetY -= BUTTON_HEIGHT; // + DEFAULT_SPACING;

        const int cx = x+BUTTON_HEIGHT/2-CHECK_SIZE/2;
        const int cy = y+BUTTON_HEIGHT/2-CHECK_SIZE/2;

        if(enabled && isHovered(x, y, w, h) && isHoveringPanel(state.areaId)) state.hot = id;
        bool res = buttonLogic(id, &state) == BUTTON_WENT_INACTIVE;

        gfx->addTriangle(cx, cy, CHECK_SIZE, CHECK_SIZE, checked? 2 : 1, imguiRGBA(255,255,255,state.active == id?255:200));

        gfx->addText(x+BUTTON_HEIGHT, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_LEFT, text, enabled ? state.hot == id ? imguiRGBA(255,196,0,255) : imguiRGBA(255,255,255,200) : imguiRGBA(128,128,128,200));

        if (subtext) gfx->addText(x+w-BUTTON_HEIGHT/2, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_RIGHT, subtext, imguiRGBA(255,255,255,128));

        return res;
}

void imgui::label(const char* text)
{
        int x = state.panel.x + SCROLL_AREA_PADDING + (state.indents * INDENT_SIZE);
        int y = state.widgetY - BUTTON_HEIGHT;
        state.widgetY -= BUTTON_HEIGHT;

        gfx->addText(x, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_LEFT, text, imguiRGBA(255,255,255,255));
}

void imgui::value(const char* text)
{
        const int x = state.panel.x + SCROLL_AREA_PADDING + (state.indents * INDENT_SIZE);
        const int w = state.panel.w - (SCROLL_AREA_PADDING*4) - (state.indents * INDENT_SIZE);

        const int y = state.widgetY - BUTTON_HEIGHT;
        state.widgetY -= BUTTON_HEIGHT;

        gfx->addText(x+w-BUTTON_HEIGHT/2, y+BUTTON_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_RIGHT, text, imguiRGBA(255,255,255,200));
}

bool imgui::slider(const char* text, float* val, float vmin, float vmax, float vinc, bool enabled)
{
        state.widgetId++;
        unsigned int id = (state.areaId<<16) | state.widgetId;

        int x = state.panel.x + SCROLL_AREA_PADDING + (state.indents * INDENT_SIZE);
        int w = state.panel.w - (SCROLL_AREA_PADDING*4) - (state.indents * INDENT_SIZE);

        int y = state.widgetY - BUTTON_HEIGHT;
        int h = SLIDER_HEIGHT;
        state.widgetY -= SLIDER_HEIGHT + DEFAULT_SPACING;

        gfx->addRoundedRect((float)x, (float)y, (float)w, (float)h, 4.0f, imguiRGBA(0,0,0,128));

        const int range = w - SLIDER_MARKER_WIDTH;

        float u = (*val - vmin) / (vmax-vmin); // u = the percent of the slider the dragger covers (0-1)
        if (u < 0) u = 0; // limit u
        if (u > 1) u = 1;
        int m = (int)(u * range);

        if(enabled && isHovered(x, y, w, h) && isHoveringPanel(state.areaId)) state.hot = id; //allow hover on bar.
        ButtonState bst = buttonLogic(id, &state);
        bool res = bst == BUTTON_WENT_INACTIVE;
        bool valChanged = false;

        if (state.active == id) {
            u =  (float)(state.cursor.x - x - (SLIDER_MARKER_WIDTH/2)) / (float)range; //set u to the amount the cursor is offset from the left of the slider widget
            if (u < 0) u = 0;// limit u
            if (u > 1) u = 1;
            *val = vmin + u*(vmax-vmin);
            *val = floorf(*val/vinc+0.5f)*vinc; // Snap to vinc
            m = (int)(u * range);
            valChanged = true;
        }

        gfx->addRoundedRect((float)(x+m), (float)y, (float)SLIDER_MARKER_WIDTH, (float)SLIDER_HEIGHT, 4.0f, state.active == id? imguiRGBA(255,255,255,255) : state.hot == id ? imguiRGBA(255,196,0,128) : imguiRGBA(255,255,255,64));

        char msg[128];
        snprintf(msg, 128, "%0.0f", *val);

        gfx->addText(x+SLIDER_HEIGHT/2, y+SLIDER_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_LEFT, text, enabled? state.hot == id ? imguiRGBA(255,196,0,255) : imguiRGBA(255,255,255,200) : imguiRGBA(128,128,128,200));
        gfx->addText(x+w-SLIDER_HEIGHT/2, y+SLIDER_HEIGHT/2-TEXT_HEIGHT/2, IMGUI_ALIGN_RIGHT, msg, enabled? state.hot == id ? imguiRGBA(255,196,0,255) : imguiRGBA(255,255,255,200) : imguiRGBA(128,128,128,200));

        return res || valChanged;
}


void imgui::indent()
{
        state.indents += 1;
}

void imgui::unindent()
{
        state.indents -= 1;
}

void imgui::separator()
{
        state.widgetY -= DEFAULT_SPACING*3;
}

void imgui::separatorLine()
{
        int x = state.panel.x + SCROLL_AREA_PADDING + (state.indents * INDENT_SIZE);
        int w = state.panel.w - (SCROLL_AREA_PADDING*4) - (state.indents * INDENT_SIZE);

        int y = state.widgetY - DEFAULT_SPACING*2;
        int h = 1;

        state.widgetY -= DEFAULT_SPACING*4;

        gfx->addRect((float)x, (float)y, (float)w, (float)h, imguiRGBA(255,255,255,32));
}

void imgui::setDrawBuffer(DrawBuffer* db)
{
    gfx = db;
}
