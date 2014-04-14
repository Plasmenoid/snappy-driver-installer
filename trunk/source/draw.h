/*
This file is part of Snappy Driver Installer.

Snappy Driver Installer is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Snappy Driver Installer is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Snappy Driver Installer.  If not, see <http://www.gnu.org/licenses/>.
*/

//{ Structs
typedef struct _img_t
{
    BYTE *big;
    HBITMAP bitmap;
    HDC dc;
    int sx,sy;
    int index;
    int iscopy;
    int hasalpha;
}img_t;

typedef struct _canvas_t
{
    int x,y;
    HDC hdcMem,localDC;
    HBITMAP bitmap,oldbitmap;
    HRGN clipping;
    PAINTSTRUCT ps;
    HWND hwnd;
}canvas_t;

typedef struct _panelitem_t
{
    int type;
    int str_id;
    int action_id;
    int checked;
}panelitem_t;

typedef struct _panel_t
{
    panelitem_t *items;
    int x,y,wx,wy;
}panel_t;

//}

//{ Global vars
extern img_t box[BOX_NUM];
extern img_t icon[ICON_NUM];
//}


int Xb(int x);
int Yb(int y);
int Xt(int x,int o);
int Yt(int y,int o);

// Image
void box_init(img_t *img,int i);
void box_free(img_t *img);
void icon_init(img_t *img,int i);
void icon_free(img_t *img);
void image_load(img_t *img,BYTE *data,int sz);
void image_loadFile(img_t *img,WCHAR *filename);
void image_loadRes(img_t *img,int id);
void image_draw(HDC dc,img_t *img,int x1,int y1,int x2,int y2,int anchor,int fill);

// Draw
void box_draw(HDC hdc,int x1,int y1,int x2,int y2,int i);
void drawcheckbox(HDC hdc,int x,int y,int wx,int wy,int checked,int active);
void drawrect(HDC hdc,int x1,int y1,int x2,int y2,int color1,int color2,int w,int r);
void drawrevision(HDC hdcMem,int y);
void drawpopup(int itembar,int type,int x,int y,HWND hwnd);

// Canvas
void canvas_init(canvas_t *canvas);
void canvas_free(canvas_t *canvas);
void canvas_begin(canvas_t *canvas,HWND hwnd,int x,int y);
void canvas_end(canvas_t *canvas);
