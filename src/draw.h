#pragma once

static const unsigned TEXT_POOL_SIZE = 8000;
static const unsigned GFXCMD_QUEUE_SIZE = 5000;

enum imguiTextAlign
{
        IMGUI_ALIGN_LEFT,
        IMGUI_ALIGN_CENTER,
        IMGUI_ALIGN_RIGHT,
};

enum DrawCommandType
{
        IMGUI_GFXCMD_RECT,
        IMGUI_GFXCMD_TRIANGLE,
        IMGUI_GFXCMD_LINE,
        IMGUI_GFXCMD_TEXT,
        IMGUI_GFXCMD_SCISSOR,
};

struct imguiGfxRect
{
        short x,y,w,h,r;
};

struct imguiGfxText
{
        short x,y,align;
        const char* text;
};

struct imguiGfxLine
{
        short x0,y0,x1,y1,r;
};

struct DrawCommand
{
        char type;
        char flags;
        char pad[2];
        unsigned int col;
        union
        {
                imguiGfxLine line;
                imguiGfxRect rect;
                imguiGfxText text;
        };
};

struct DrawBuffer
{
private:
    char textPool[TEXT_POOL_SIZE];
    DrawCommand gfxCmdQueue[GFXCMD_QUEUE_SIZE];
    unsigned textPoolSize;
    unsigned gfxCmdQueueSize;

public:
    DrawBuffer();
    void addScissor(int x, int y, int w, int h);
    void addRect(float x, float y, float w, float h, unsigned int color);
    void addLine(float x0, float y0, float x1, float y1, float r, unsigned int color);
    void addRoundedRect(float x, float y, float w, float h, float r, unsigned int color);
    void addTriangle(int x, int y, int w, int h, int flags, unsigned int color);
    void addText(int x, int y, int align, const char* text, unsigned int color);

    void reset();

    const DrawCommand* getPrimitives();
    unsigned getNumPrimitives();
};
