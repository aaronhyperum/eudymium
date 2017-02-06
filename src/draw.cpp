#include "draw.h"

#include <stdio.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DrawBuffer::DrawBuffer()
{
    textPoolSize = 0;
    gfxCmdQueueSize = 0;
};

const DrawCommand* DrawBuffer::getPrimitives()
{
        return gfxCmdQueue;
}

unsigned DrawBuffer::getNumPrimitives()
{
        return gfxCmdQueueSize;
}

void DrawBuffer::reset()
{
    gfxCmdQueueSize = 0;
    textPoolSize = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void DrawBuffer::addScissor(int x, int y, int w, int h)
{
        if (gfxCmdQueueSize >= GFXCMD_QUEUE_SIZE)
                return;
        DrawCommand& cmd = gfxCmdQueue[gfxCmdQueueSize++];
        cmd.type = IMGUI_GFXCMD_SCISSOR;
        cmd.flags = x < 0 ? 0 : 1;      // on/off flag.
        cmd.col = 0;
        cmd.rect.x = (short)x;
        cmd.rect.y = (short)y;
        cmd.rect.w = (short)w;
        cmd.rect.h = (short)h;
}

void DrawBuffer::addRect(float x, float y, float w, float h, unsigned int color)
{
        if (gfxCmdQueueSize >= GFXCMD_QUEUE_SIZE)
                return;
        DrawCommand& cmd = gfxCmdQueue[gfxCmdQueueSize++];
        cmd.type = IMGUI_GFXCMD_RECT;
        cmd.flags = 0;
        cmd.col = color;
        cmd.rect.x = (short)(x);
        cmd.rect.y = (short)(y);
        cmd.rect.w = (short)(w);
        cmd.rect.h = (short)(h);
        cmd.rect.r = 0;
}

void DrawBuffer::addLine(float x0, float y0, float x1, float y1, float r, unsigned int color)
{
        if (gfxCmdQueueSize >= GFXCMD_QUEUE_SIZE)
                return;
        DrawCommand& cmd = gfxCmdQueue[gfxCmdQueueSize++];
        cmd.type = IMGUI_GFXCMD_LINE;
        cmd.flags = 0;
        cmd.col = color;
        cmd.line.x0 = (short)(x0);
        cmd.line.y0 = (short)(y0);
        cmd.line.x1 = (short)(x1);
        cmd.line.y1 = (short)(y1);
        cmd.line.r = (short)(r);
}

void DrawBuffer::addRoundedRect(float x, float y, float w, float h, float r, unsigned int color)
{
        if (gfxCmdQueueSize >= GFXCMD_QUEUE_SIZE)
                return;
        DrawCommand& cmd = gfxCmdQueue[gfxCmdQueueSize++];
        cmd.type = IMGUI_GFXCMD_RECT;
        cmd.flags = 0;
        cmd.col = color;
        cmd.rect.x = (short)(x);
        cmd.rect.y = (short)(y);
        cmd.rect.w = (short)(w);
        cmd.rect.h = (short)(h);
        cmd.rect.r = (short)(r);
}

void DrawBuffer::addTriangle(int x, int y, int w, int h, int flags, unsigned int color)
{
        if (gfxCmdQueueSize >= GFXCMD_QUEUE_SIZE)
                return;
        DrawCommand& cmd = gfxCmdQueue[gfxCmdQueueSize++];
        cmd.type = IMGUI_GFXCMD_TRIANGLE;
        cmd.flags = (char)flags;
        cmd.col = color;
        cmd.rect.x = (short)(x);
        cmd.rect.y = (short)(y);
        cmd.rect.w = (short)(w);
        cmd.rect.h = (short)(h);
}

void DrawBuffer::addText(int x, int y, int align, const char* text, unsigned int color)
{
        if (gfxCmdQueueSize >= GFXCMD_QUEUE_SIZE)
                return;
        DrawCommand& cmd = gfxCmdQueue[gfxCmdQueueSize++];
        cmd.type = IMGUI_GFXCMD_TEXT;
        cmd.flags = 0;
        cmd.col = color;
        cmd.text.x = (short)x;
        cmd.text.y = (short)y;
        cmd.text.align = (short)align;

        //allocate text
        unsigned len = strlen(text)+1;
        char* dst;
        if ( + len >= TEXT_POOL_SIZE) dst = 0;
        else dst = &textPool[textPoolSize];
        memcpy(dst, text, len);
        textPoolSize += len;

        cmd.text.text = dst;
}
