#include "imguiRenderGL3.h"
#include "imgui.h"
#include "draw.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>

#include <GL/gl3w.h>

// Some math headers don't have PI defined.
static const float PI = 3.14159265f;
static const unsigned TEMP_COORD_COUNT = 100;
static const int CIRCLE_VERTS = 8*4;
static const int CIRCLE_DIVS = 10;
static const float g_tabStops[4] = {150, 210, 270, 330};

static float g_tempCoords[TEMP_COORD_COUNT*2];
static float g_tempNormals[TEMP_COORD_COUNT*2];
static float g_tempVertices[TEMP_COORD_COUNT * 12 + (TEMP_COORD_COUNT - 2) * 6];
static float g_tempTextureCoords[TEMP_COORD_COUNT * 12 + (TEMP_COORD_COUNT - 2) * 6];
static float g_tempColors[TEMP_COORD_COUNT * 24 + (TEMP_COORD_COUNT - 2) * 12];


static float g_circleVerts[CIRCLE_VERTS*2];

static stbtt_bakedchar g_cdata[96]; // ASCII 32..126 is 95 glyphs
static GLuint g_ftex = 0;
static GLuint g_whitetex = 0;
static GLuint g_vao = 0;
static GLuint g_vbos[3] = {0, 0, 0};
static GLuint g_program = 0;
static GLuint g_programViewportLocation = 0;
static GLuint g_programTextureLocation = 0;


void imguiRenderer::drawPolygon(const float* coords, unsigned numCoords, float r, unsigned int col)
{
        if (numCoords > TEMP_COORD_COUNT) numCoords = TEMP_COORD_COUNT;

        for (unsigned i = 0, j = numCoords-1; i < numCoords; j=i++)
        {
                const float* v0 = &coords[j*2];
                const float* v1 = &coords[i*2];
                float dx = v1[0] - v0[0];
                float dy = v1[1] - v0[1];
                float d = sqrtf(dx*dx+dy*dy);
                if (d > 0)
                {
                        d = 1.0f/d;
                        dx *= d;
                        dy *= d;
                }
                g_tempNormals[j*2+0] = dy;
                g_tempNormals[j*2+1] = -dx;
        }

        float colf[4] = {
            (float) (col & 0xff) / 255.f,
            (float) ((col>>8) & 0xff) / 255.f,
            (float) ((col>>16) & 0xff) / 255.f,
            (float) ((col>>24) & 0xff) / 255.f
        };
        float colTransf[4] = {
            (float) (col&0xff) / 255.f,
            (float) ((col>>8)&0xff) / 255.f,
            (float) ((col>>16)&0xff) / 255.f,
            0
        };

        for (unsigned i = 0, j = numCoords-1; i < numCoords; j=i++)
        {
                float dlx0 = g_tempNormals[j*2+0];
                float dly0 = g_tempNormals[j*2+1];
                float dlx1 = g_tempNormals[i*2+0];
                float dly1 = g_tempNormals[i*2+1];
                float dmx = (dlx0 + dlx1) * 0.5f;
                float dmy = (dly0 + dly1) * 0.5f;
                float   dmr2 = dmx*dmx + dmy*dmy;
                if (dmr2 > 0.000001f)
                {
                        float   scale = 1.0f / dmr2;
                        if (scale > 10.0f) scale = 10.0f;
                        dmx *= scale;
                        dmy *= scale;
                }
                g_tempCoords[i*2+0] = coords[i*2+0]+dmx*r;
                g_tempCoords[i*2+1] = coords[i*2+1]+dmy*r;
        }

        int vSize = numCoords * 12 + (numCoords - 2) * 6;
        int uvSize = numCoords * 2 * 6 + (numCoords - 2) * 2 * 3;
        int cSize = numCoords * 4 * 6 + (numCoords - 2) * 4 * 3;
        float * v = g_tempVertices;
        float * uv = g_tempTextureCoords;
        memset(uv, 0, uvSize * sizeof(float));
        float * c = g_tempColors;
        memset(c, 1, cSize * sizeof(float));

        float * ptrV = v;
        float * ptrC = c;

        int index_v = 0, index_c = 0;

        //TODO: make this more readable.
        //from i = 0 ... numcoords-1, from j == numcoords-1 -> 0...numcoords-2
        for (unsigned i = 0, j = numCoords-1; i < numCoords; j=i++)
        {
            v[0 + index_v] = coords[i*2];
            v[1 + index_v] = coords[i*2 + 1];

            v[2 + index_v] = coords[j*2];
            v[3 + index_v] = coords[j*2 + 1];

            v[4 + index_v] = g_tempCoords[j*2];
            v[5 + index_v] = g_tempCoords[j*2 + 1];

            v[6 + index_v] = g_tempCoords[j*2];
            v[7 + index_v] = g_tempCoords[j*2 + 1];

            v[8 + index_v] = g_tempCoords[i*2];
            v[9 + index_v] = g_tempCoords[i*2 + 1];

            v[10 + index_v] = coords[i*2];
            v[11 + index_v] = coords[i*2 + 1];

            c[0 + index_c] = colf[0];
            c[1 + index_c] = colf[1];
            c[2 + index_c] = colf[2];
            c[3 + index_c] = colf[3];

            c[4 + index_c] = colf[0];
            c[5 + index_c] = colf[1];
            c[6 + index_c] = colf[2];
            c[7 + index_c] = colf[3];

            c[8 + index_c] = colTransf[0];
            c[9 + index_c] = colTransf[1];
            c[10 + index_c] = colTransf[2];
            c[11 + index_c] = colTransf[3];

            c[12 + index_c]= colTransf[0];
            c[13 + index_c] = colTransf[1];
            c[14 + index_c] = colTransf[2];
            c[15 + index_c] = colTransf[3];

            c[16 + index_c] = colTransf[0];
            c[17 + index_c] = colTransf[1];
            c[18 + index_c] = colTransf[2];
            c[19 + index_c] = colTransf[3];

            c[20 + index_c] = colf[0];
            c[21 + index_c] = colf[1];
            c[22 + index_c] = colf[2];
            c[23 + index_c] = colf[3];

            index_v += 12;
            index_c += 24;
        }

        for (unsigned i = 2; i < numCoords; ++i)
        {
            v[0 + index_v] = coords[0];
            v[1 + index_v] = coords[1];

            v[2 + index_v] = coords[(i-1)*2];
            v[3 + index_v] = coords[(i-1)*2+1];

            v[4 + index_v]= coords[i*2];
            v[5 + index_v] = coords[i*2 + 1];

            c[0 + index_c] = colf[0];
            c[1 + index_c] = colf[1];
            c[2 + index_c] = colf[2];
            c[3 + index_c] = colf[3];

            c[4 + index_c] = colf[0];
            c[5 + index_c] = colf[1];
            c[6 + index_c] = colf[2];
            c[7 + index_c] = colf[3];

            c[8 + index_c] = colf[0];
            c[9 + index_c] = colf[1];
            c[10 + index_c] = colf[2];
            c[11 + index_c] = colf[3];

            index_v += 6;
            index_c += 12;
        }
        glBindTexture(GL_TEXTURE_2D, g_whitetex);

        glBindVertexArray(g_vao);
        glBindBuffer(GL_ARRAY_BUFFER, g_vbos[0]);
        glBufferData(GL_ARRAY_BUFFER, vSize * sizeof(float), v, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, g_vbos[1]);
        glBufferData(GL_ARRAY_BUFFER, uvSize * sizeof(float), uv, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, g_vbos[2]);
        glBufferData(GL_ARRAY_BUFFER, cSize * sizeof(float), c, GL_STATIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, (numCoords * 2 + numCoords - 2)*3);

}

void imguiRenderer::drawRect(float x, float y, float w, float h, float fth, unsigned int col)
{
    int triangles = 2;

        float verts[] = {
            x,  y, // Top-left
            x+w,  y, // Top-right
            x+w, y+h, // Bottom-right

            x+w, y+h, // Bottom-right
            x, y+h, // Bottom-left
            x,  y,   // Top-left
        };

        float uv[] = {
             0,  0, // Top-left
             0,  0, // Top-right
             0, 0, // Bottom-right

            0, 0, // Bottom-right
            0, 0, // Bottom-left
            0,  0,   // Top-left
        };

        float r = (col & 0xff) / 255.f, g = ((col>>8) & 0xff) / 255.f, b = ((col>>16) & 0xff) / 255.f, a = ((col>>24) & 0xff) / 255.f;

        float colors[] = {
            r, g, b, a, // Top-left
            r, g, b, a, // Top-right
            r, g, b, a, // Bottom-right

            r, g, b, a, // Bottom-right
            r, g, b, a, // Bottom-left
            r, g, b, a, // Top-left
        };

        glBindTexture(GL_TEXTURE_2D, g_whitetex);

        glBindVertexArray(g_vao);
        glBindBuffer(GL_ARRAY_BUFFER, g_vbos[0]);
        glBufferData(GL_ARRAY_BUFFER, 6*2 * sizeof(float), verts, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, g_vbos[1]);
        glBufferData(GL_ARRAY_BUFFER, 6*2 * sizeof(float), uv, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, g_vbos[2]);
        glBufferData(GL_ARRAY_BUFFER, 6*4 * sizeof(float), colors, GL_STATIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, 6);
}

void rect (float* verts, int& c, int x, int y, int w, int h)
{
    verts[c+0] = x;
    verts[c+1] = y;

    verts[c+2] = x+w;
    verts[c+3] = y;

    verts[c+4] = x+w;
    verts[c+5] = y+h;


    verts[c+6] = x+w;
    verts[c+7] = y+h;

    verts[c+8] = x;
    verts[c+9] = y+h;

    verts[c+10] = x;
    verts[c+11] = y;

}

//TODO: Remove drawPolygon();
void imguiRenderer::drawRoundedRect(float x, float y, float w, float h, float r, float fth, unsigned int col)
{
        /*int triangles =  5 * 2 +  CIRCLE_DIVS * 4;
        float circle[CIRCLE_DIVS];
        for (int i = 0; i <= CIRCLE_DIVS; i++)
        {
            circle[i*2+0] = cosf((i*1.0/CIRCLE_DIVS)*(PI/2));
            circle[i*2+0] = sinf((i*1.0/CIRCLE_DIVS)*(PI/2));
        }

        float *verts = new float[triangles*3*2];

        float *uv = new float[triangles*3*2];

                memset(uv, 0, triangles*3*2);
        float *colors = new float[triangles*3*4];

        float red = (col & 0xff) / 255.f, grn = ((col>>8) & 0xff) / 255.f, blu = ((col>>16) & 0xff) / 255.f, alp = ((col>>24) & 0xff) / 255.f;
        for (int i =0; i < triangles*3; i++)
        {
            colors[i*4+0] = red;

            colors[i*4+1] = grn;

            colors[i*4+2] = blu;

            colors[i*4+3] = alp;
        }

        int c = 0;
        for (int i = 0; i <= CIRCLE_DIVS; i++) //upper right
        {
            verts[c*6+0] = x+w-r + circle[i*2+0];
            verts[c*6+1] = y+r - circle[i*2+1];
            verts[c*6+2] = x+w-r + circle[i*2+2];
            verts[c*6+3] = y+r - circle[i*2+3];
            verts[c*6+4] = x+w-r;
            verts[c*6+4] = y+r;
            c++;
        }

        for (int i = 0; i <= CIRCLE_DIVS; i++) //bottom right
        {
            verts[c*6+0] = x+w-r + circle[i*2+0];
            verts[c*6+1] = y+h-r + circle[i*2+1];
            verts[c*6+2] = x+w-r + circle[i*2+2];
            verts[c*6+3] = y+h-r + circle[i*2+3];
            verts[c*6+4] = x+w-r;
            verts[c*6+4] = y+h-r;
            c++;
        }

        for (int i = 0; i <= CIRCLE_DIVS; i++) //bottom left
        {
            verts[c*6+0] = x+r - circle[i*2+0];
            verts[c*6+1] = y+h-r + circle[i*2+1];
            verts[c*6+2] = x+r - circle[i*2+2];
            verts[c*6+3] = y+h-r + circle[i*2+3];
            verts[c*6+4] = x+r;
            verts[c*6+4] = y+h-r;
            c++;
        }

        for (int i = 0; i <= CIRCLE_DIVS; i++) //bottom left
        {
            verts[c*6+0] = x+r - circle[i*2+0];
            verts[c*6+1] = y+r - circle[i*2+1];
            verts[c*6+2] = x+r - circle[i*2+2];
            verts[c*6+3] = y+r - circle[i*2+3];
            verts[c*6+4] = x+r;
            verts[c*6+4] = y+r;
            c++;
        }



        //center
        rect(verts, c, x+r, y+r, w-r-r, h-r-r);

        //up
        rect(verts, c, x+r, y, w-r-r, r);

        //down
        rect(verts, c, x+r, y+w-r, w-r-r, r);

        //left
        rect(verts, c, x, y+r, r, h-r-r);

        //right
        rect(verts, c, x+w-r, y+r, r, h-r-r);

        glBindTexture(GL_TEXTURE_2D, g_whitetex);

        glBindVertexArray(g_vao);
        glBindBuffer(GL_ARRAY_BUFFER, g_vbos[0]);
        glBufferData(GL_ARRAY_BUFFER, triangles*3*2 * sizeof(float), verts, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, g_vbos[1]);
        glBufferData(GL_ARRAY_BUFFER, triangles*3*2 * sizeof(float), uv, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, g_vbos[2]);
        glBufferData(GL_ARRAY_BUFFER, triangles*3*4 * sizeof(float), colors, GL_STATIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, triangles*3*2);

        delete[] verts;
        delete[] uv;
        delete[] colors;*/

        const unsigned n = CIRCLE_VERTS/4;
        float verts[(n+1)*4*2];
        const float* cverts = g_circleVerts;
        float* v = verts;

        for (unsigned i = 0; i <= n; ++i)
        {
                *v++ = x+w-r + cverts[i*2]*r;
                *v++ = y+h-r + cverts[i*2+1]*r;
        }

        for (unsigned i = n; i <= n*2; ++i)
        {
                *v++ = x+r + cverts[i*2]*r;
                *v++ = y+h-r + cverts[i*2+1]*r;
        }

        for (unsigned i = n*2; i <= n*3; ++i)
        {
                *v++ = x+r + cverts[i*2]*r;
                *v++ = y+r + cverts[i*2+1]*r;
        }

        for (unsigned i = n*3; i < n*4; ++i)
        {
                *v++ = x+w-r + cverts[i*2]*r;
                *v++ = y+r + cverts[i*2+1]*r;
        }
        *v++ = x+w-r + cverts[0]*r;
        *v++ = y+r + cverts[1]*r;

        drawPolygon(verts, (n+1)*4, fth, col);
}

//TODO: Remove drawPolygon();
void imguiRenderer::drawLine(float x0, float y0, float x1, float y1, float r, float fth, unsigned int col)
{
        float dx = x1-x0;
        float dy = y1-y0;
        float d = sqrtf(dx*dx+dy*dy);
        if (d > 0.0001f)
        {
                d = 1.0f/d;
                dx *= d;
                dy *= d;
        }
        float nx = dy;
        float ny = -dx;
        float verts[4*2];
        r -= fth;
        r *= 0.5f;
        if (r < 0.01f) r = 0.01f;
        dx *= r;
        dy *= r;
        nx *= r;
        ny *= r;

        verts[0] = x0-dx-nx;
        verts[1] = y0-dy-ny;

        verts[2] = x0-dx+nx;
        verts[3] = y0-dy+ny;

        verts[4] = x1+dx+nx;
        verts[5] = y1+dy+ny;

        verts[6] = x1+dx-nx;
        verts[7] = y1+dy-ny;

        drawPolygon(verts, 4, fth, col);
}



// Fix to use standard stb quad baking.
void imguiRenderer::getBakedQuad(stbtt_bakedchar *chardata, int pw, int ph, int char_index,
                                                 float *xpos, float *ypos, stbtt_aligned_quad *q)
{
        stbtt_bakedchar *b = chardata + char_index;
        int round_x = STBTT_ifloor(*xpos + b->xoff);
        int round_y = STBTT_ifloor(*ypos - b->yoff);

        q->x0 = (float)round_x;
        q->y0 = (float)round_y;
        q->x1 = (float)round_x + b->x1 - b->x0;
        q->y1 = (float)round_y - b->y1 + b->y0;

        q->s0 = b->x0 / (float)pw;
        q->t0 = b->y0 / (float)pw;
        q->s1 = b->x1 / (float)ph;
        q->t1 = b->y1 / (float)ph;

        *xpos += b->xadvance;
}

float imguiRenderer::getTextLength(stbtt_bakedchar *chardata, const char* text)
{
        float xpos = 0;
        float len = 0;
        while (*text)
        {
                int c = (unsigned char)*text;
                if (c == '\t')
                {
                        for (int i = 0; i < 4; ++i)
                        {
                                if (xpos < g_tabStops[i])
                                {
                                        xpos = g_tabStops[i];
                                        break;
                                }
                        }
                }
                else if (c >= 32 && c < 128)
                {
                        stbtt_bakedchar *b = chardata + c-32;
                        int round_x = STBTT_ifloor((xpos + b->xoff) + 0.5);
                        len = round_x + b->x1 - b->x0 + 0.5f;
                        xpos += b->xadvance;
                }
                ++text;
        }
        return len;
}

void imguiRenderer::drawText(float x, float y, const char *text, int align, unsigned int col)
{
        if (!g_ftex) return;
        if (!text) return;

        if (align == IMGUI_ALIGN_CENTER)
                x -= getTextLength(g_cdata, text)/2;
        else if (align == IMGUI_ALIGN_RIGHT)
                x -= getTextLength(g_cdata, text);

        float r = (float) (col&0xff) / 255.f;
        float g = (float) ((col>>8)&0xff) / 255.f;
        float b = (float) ((col>>16)&0xff) / 255.f;
        float a = (float) ((col>>24)&0xff) / 255.f;

        // assume orthographic projection with units = screen pixels, origin at top left
        glBindTexture(GL_TEXTURE_2D, g_ftex);

        const float ox = x;

        while (*text)
        {
                int c = (unsigned char)*text;
                if (c == '\t')
                {
                        for (int i = 0; i < 4; ++i)
                        {
                                if (x < g_tabStops[i]+ox)
                                {
                                        x = g_tabStops[i]+ox;
                                        break;
                                }
                        }
                }
                else if (c >= 32 && c < 128)
                {
                        stbtt_aligned_quad q;
                        getBakedQuad(g_cdata, 512,512, c-32, &x,&y,&q);

                        float v[12] = {
                                        q.x0, q.y0,
                                        q.x1, q.y1,
                                        q.x1, q.y0,
                                        q.x0, q.y0,
                                        q.x0, q.y1,
                                        q.x1, q.y1,
                                      };
                        float uv[12] = {
                                        q.s0, q.t0,
                                        q.s1, q.t1,
                                        q.s1, q.t0,
                                        q.s0, q.t0,
                                        q.s0, q.t1,
                                        q.s1, q.t1,
                                      };
                        float c[24] = {
                                        r, g, b, a,
                                        r, g, b, a,
                                        r, g, b, a,
                                        r, g, b, a,
                                        r, g, b, a,
                                        r, g, b, a,
                                      };
                        glBindVertexArray(g_vao);
                        glBindBuffer(GL_ARRAY_BUFFER, g_vbos[0]);
                        glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), v, GL_STATIC_DRAW);
                        glBindBuffer(GL_ARRAY_BUFFER, g_vbos[1]);
                        glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), uv, GL_STATIC_DRAW);
                        glBindBuffer(GL_ARRAY_BUFFER, g_vbos[2]);
                        glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(float), c, GL_STATIC_DRAW);
                        glDrawArrays(GL_TRIANGLES, 0, 6);

                }
                ++text;
        }

        //glEnd();
        //glDisable(GL_TEXTURE_2D);
}

bool imguiRenderer::init(const char* fontpath)
{
        for (int i = 0; i < CIRCLE_VERTS; ++i)
        {
                float a = (float)i/(float)CIRCLE_VERTS * PI*2;
                g_circleVerts[i*2+0] = cosf(a); //evens = x
                g_circleVerts[i*2+1] = sinf(a); //odds = y
        }

        // Load font.
        //TODO: use stbi font loading.
        FILE* fp = fopen(fontpath, "rb");
        if (!fp) return false;
        fseek(fp, 0, SEEK_END);
        int size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        unsigned char* ttfBuffer = (unsigned char*)malloc(size);
        if (!ttfBuffer)
        {
                fclose(fp);
                return false;
        }

        fread(ttfBuffer, 1, size, fp);
        fclose(fp);
        fp = 0;

        unsigned char* bmap = (unsigned char*)malloc(512*512);
        if (!bmap)
        {
                free(ttfBuffer);
                return false;
        }

        stbtt_BakeFontBitmap(ttfBuffer,0, 15.0f, bmap,512,512, 32,96, g_cdata);

        // can free ttf_buffer at this point
        glGenTextures(1, &g_ftex);
        glBindTexture(GL_TEXTURE_2D, g_ftex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 512,512, 0, GL_RED, GL_UNSIGNED_BYTE, bmap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // can free ttf_buffer at this point
        unsigned char white_alpha = 255;
        glGenTextures(1, &g_whitetex);
        glBindTexture(GL_TEXTURE_2D, g_whitetex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, &white_alpha);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glGenVertexArrays(1, &g_vao);
        glGenBuffers(3, g_vbos);

        glBindVertexArray(g_vao);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ARRAY_BUFFER, g_vbos[0]);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
        glBufferData(GL_ARRAY_BUFFER, 0, 0, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, g_vbos[1]);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
        glBufferData(GL_ARRAY_BUFFER, 0, 0, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, g_vbos[2]);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*4, (void*)0);
        glBufferData(GL_ARRAY_BUFFER, 0, 0, GL_STATIC_DRAW);
        g_program = glCreateProgram();

        const char * vs =
        "#version 150\n"
        "uniform vec2 Viewport;\n"
        "in vec2 VertexPosition;\n"
        "in vec2 VertexTexCoord;\n"
        "in vec4 VertexColor;\n"
        "out vec2 texCoord;\n"
        "out vec4 vertexColor;\n"
        "void main(void)\n"
        "{\n"
        "    vertexColor = VertexColor;\n"
        "    texCoord = VertexTexCoord;\n"
        "    gl_Position = vec4(VertexPosition * 2.0 / Viewport - 1.0, 0.f, 1.0);\n"
        "}\n";
        GLuint vso = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vso, 1, (const char **)  &vs, NULL);
        glCompileShader(vso);
        glAttachShader(g_program, vso);

        const char * fs =
        "#version 150\n"
        "in vec2 texCoord;\n"
        "in vec4 vertexColor;\n"
        "uniform sampler2D Texture;\n"
        "out vec4  Color;\n"
        "void main(void)\n"
        "{\n"
        "    float alpha = texture(Texture, texCoord).r;\n"
        "    Color = vec4(vertexColor.rgb, vertexColor.a * alpha);\n"
        "}\n";
        GLuint fso = glCreateShader(GL_FRAGMENT_SHADER);

        glShaderSource(fso, 1, (const char **) &fs, NULL);
        glCompileShader(fso);
        glAttachShader(g_program, fso);

        glBindAttribLocation(g_program,  0,  "VertexPosition");
        glBindAttribLocation(g_program,  1,  "VertexTexCoord");
        glBindAttribLocation(g_program,  2,  "VertexColor");
        glBindFragDataLocation(g_program, 0, "Color");
        glLinkProgram(g_program);
        glDeleteShader(vso);
        glDeleteShader(fso);

        glUseProgram(g_program);
        g_programViewportLocation = glGetUniformLocation(g_program, "Viewport");
        g_programTextureLocation = glGetUniformLocation(g_program, "Texture");

        glUseProgram(0);


        free(ttfBuffer);
        free(bmap);

        return true;
}

void imguiRenderer::quit()
{
        if (g_ftex)
        {
                glDeleteTextures(1, &g_ftex);
                g_ftex = 0;
        }

        if (g_vao)
        {
            glDeleteVertexArrays(1, &g_vao);
            glDeleteBuffers(3, g_vbos);
            g_vao = 0;
        }

        if (g_program)
        {
            glDeleteProgram(g_program);
            g_program = 0;
        }

}

void imguiRenderer::draw(int width, int height, DrawBuffer* db)
{
        const DrawCommand* q = db->getPrimitives();
        int nq = db->getNumPrimitives();

        glViewport(0, 0, width, height);
        glUseProgram(g_program);
	    glActiveTexture(GL_TEXTURE0);
        glUniform2f(g_programViewportLocation, (float) width, (float) height);
        glUniform1i(g_programTextureLocation, 0);


        glDisable(GL_SCISSOR_TEST);
        for (int i = 0; i < nq; ++i)
        {
                const DrawCommand& cmd = q[i];
                if (cmd.type == IMGUI_GFXCMD_RECT)
                {
                        if (cmd.rect.r == 0)
                        {
                                drawRect((float)cmd.rect.x, (float)cmd.rect.y,
                                                 (float)cmd.rect.w, (float)cmd.rect.h,
                                                 1.0f, cmd.col);
                        }
                        else
                        {
                                drawRoundedRect((float)cmd.rect.x+0.5f, (float)cmd.rect.y+0.5f,
                                                                (float)cmd.rect.w, (float)cmd.rect.h,
                                                                (float)cmd.rect.r, 1.0f, cmd.col);
                        }
                }
                else if (cmd.type == IMGUI_GFXCMD_LINE)
                {
                        drawLine(cmd.line.x0, cmd.line.y0, cmd.line.x1, cmd.line.y1, cmd.line.r, 1.0f, cmd.col);
                }
                //TODO: Remove drawPolygon();
                else if (cmd.type == IMGUI_GFXCMD_TRIANGLE)
                {
                        if (cmd.flags == 1)
                        {
                                const float verts[3*2] =
                                {
                                        (float)cmd.rect.x+0.5f, (float)cmd.rect.y+0.5f,
                                        (float)cmd.rect.x+0.5f+(float)cmd.rect.w-1, (float)cmd.rect.y+0.5f+(float)cmd.rect.h/2-0.5f,
                                        (float)cmd.rect.x+0.5f, (float)cmd.rect.y+0.5f+(float)cmd.rect.h-1,
                                };
                                drawPolygon(verts, 3, 1.0f, cmd.col);
                        }
                        if (cmd.flags == 2)
                        {
                                const float verts[3*2] =
                                {
                                        (float)cmd.rect.x+0.5f, (float)cmd.rect.y+0.5f+(float)cmd.rect.h-1,
                                        (float)cmd.rect.x+0.5f+(float)cmd.rect.w/2-0.5f, (float)cmd.rect.y+0.5f,
                                        (float)cmd.rect.x+0.5f+(float)cmd.rect.w-1, (float)cmd.rect.y+0.5f+(float)cmd.rect.h-1,
                                };
                                drawPolygon(verts, 3, 1.0f, cmd.col);
                        }
                }
                else if (cmd.type == IMGUI_GFXCMD_TEXT)
                {
                        drawText(cmd.text.x, cmd.text.y, cmd.text.text, cmd.text.align, cmd.col);
                }
                else if (cmd.type == IMGUI_GFXCMD_SCISSOR)
                {
                        if (cmd.flags)
                        {
                                glEnable(GL_SCISSOR_TEST);
                                glScissor(cmd.rect.x, cmd.rect.y, cmd.rect.w, cmd.rect.h);
                        }
                        else
                        {
                                glDisable(GL_SCISSOR_TEST);
                        }
                }
        }
        glDisable(GL_SCISSOR_TEST);
}
