#pragma once

#include "stb_truetype.h"

struct imgui;
struct DrawBuffer;

class imguiRenderer
{
	void drawPolygon(const float* coords, unsigned numCoords, float r, unsigned int col);
	void drawRect(float x, float y, float w, float h, float fth, unsigned int col);
	void drawRoundedRect(float x, float y, float w, float h, float r, float fth, unsigned int col);
	void drawLine(float x0, float y0, float x1, float y1, float r, float fth, unsigned int col);

	void getBakedQuad(stbtt_bakedchar *chardata, int pw, int ph, int char_index, float *xpos, float *ypos, stbtt_aligned_quad *q);
	float getTextLength(stbtt_bakedchar *chardata, const char* text);

	void drawText(float x, float y, const char *text, int align, unsigned int col);

public:
	bool init(const char* fontpath);
	void quit();
	void draw(int width, int height, DrawBuffer* g);
};

static imguiRenderer implgui;
