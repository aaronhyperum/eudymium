#pragma once
#include <functional>
#include <vector>

struct DrawBuffer;

inline unsigned int imguiRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a=255)
{
        return (r) | (g << 8) | (b << 16) | (a << 24);
}

enum imguiMouseButton
{
        IMGUI_MBUT_LEFT = 0x01,
        IMGUI_MBUT_RIGHT = 0x02,
};

struct Panel{
    int x;
    int y;
    int w;
    int h;

    unsigned id;
};
//TODO: minimize
struct GuiState
{
        GuiState() : scroll(0),
                active(0), hot(0),
                 widgetY(0),
                areaId(0), widgetId(0)
        {
            cursor.is_active = false;
            cursor.x = -1;
            cursor.y = -1;

            panel.x = 0;
            panel.y = 0;
            panel.w = 0;
            panel.h = 0;

            indents = 0;
        }

        int indents;

        struct {
            bool is_active;
            int x;
            int y;
        } cursor;

        Panel panel;

        //TODO: reorganize into panel
        int scroll;

        unsigned int active;
        unsigned int hot;
        int widgetY;

        unsigned int areaId;
        unsigned int widgetId;
};

struct component
{
    int x, y, w, h;
};

enum anchor_at
{
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT
};

struct anchor
{
    int from_x, from_y;
    int to_x, to_y;
    int offset_x, offset_y;

    void from(anchor_at a)
    {
        if (a == TOP_LEFT) { from_x = from_y = 0; }
        if (a == TOP_RIGHT) { from_x = 1; from_y = 0; }
        if (a == BOTTOM_LEFT) { from_x = 0; from_y = 1; }
        if (a == BOTTOM_RIGHT) { from_x = from_y = 1; }
    }

    void to(anchor_at a)
    {
        if (a == TOP_LEFT) { to_x = to_y = 0; }
        if (a == TOP_RIGHT) { to_x = 1; to_y = 0; }
        if (a == BOTTOM_LEFT) { to_x = 0; to_y = 1; }
        if (a == BOTTOM_RIGHT) { to_x = to_y = 1; }
    }

    void off(int x, int y)
    {
        offset_x = x; offset_y = y;
    }
};

struct imgui
{
private:
    DrawBuffer* gfx;
    //TODO: Inject only on demand.
    GuiState state;

    int scrollTop = 0;
    int scrollBottom = 0;
    int scrollRight = 0;
    int scrollAreaTop = 0;
    int* scrollVal = 0;
    int focusTop = 0;
    int focusBottom = 0;
    unsigned int scrollId = 0;

    //Move to source file
    bool isHovered(int x, int y, int w, int h);
    bool isHoveringPanel(unsigned id);

    //TODO: minimal vectors.
    std::vector<Panel> panels;
    std::vector<Panel> next_panels;

public:
    void setDrawBuffer(DrawBuffer* db);

    void update(int mx, int my, bool leftmbut, int scroll);

    //TODO: arrange by x/y coordinates.
    //TODO: separate out of imgui class.

    void component(component c, anchor a);

    void panel(const char* name, int x, int y, int w, int h, int* scroll, std::function<void()> contents);
    bool button(const char* text, bool enabled = true);
    bool item(const char* text, bool enabled = true);
    bool check(const char* text, bool checked, bool enabled = true);
    bool collapse(const char* text, const char* subtext, bool checked, bool enabled = true);
    void label(const char* text);
    void value(const char* text);
    bool slider(const char* text, float* val, float vmin, float vmax, float vinc, bool enabled = true);

    void indent();
    void unindent();
    void separator();
    void separatorLine();
};
