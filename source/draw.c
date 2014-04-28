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

#include "main.h"

//{ Global vars
img_t box[BOX_NUM];
img_t icon[ICON_NUM];
//}


int Xp(panel_t *p)
{
    int idofs=PAN_ENT*p->index+PAN_ENT;
    int x=D(PANEL_OFSX+idofs);

    return x>=0?x:(main1x_c+x);
}
int Yp(panel_t *p)
{
    int idofs=PAN_ENT*p->index+PAN_ENT;
    int y=D(PANEL_OFSY+idofs);

    return y>=0?y:(main1y_c+y);
}
int XP(panel_t *p)
{
    int idofs=PAN_ENT*p->index+PAN_ENT;
    int x=D(PANEL_WX+idofs),o=D(PANEL_OFSX+idofs);

    return x>=0?x:(main1x_c+x-o);
}
int YP(panel_t *p)
{
    int idofs=PAN_ENT*p->index+PAN_ENT;
    int y=D(PANEL_WY+idofs),o=D(PANEL_OFSY+idofs);

    return y>=0?y:(main1y_c+y-o);
}

int Xm(int x){return x>=0?x:(main1x_c+x);}
int Ym(int y){return y>=0?y:(main1y_c+y);}
int XM(int x,int o){return x>=0?x:(main1x_c+x-o);}
int YM(int y,int o){return y>=0?y:(main1y_c+y-o);}

int Xg(int x){return x>=0?x:(mainx_c+x);}
int Yg(int y){return y>=0?y:(mainy_c+y);}
int XG(int x,int o){return x>=0?x:(mainx_c+x-o);}
int YG(int y,int o){return y>=0?y:(mainy_c+y-o);}

//{ Image
void box_init(img_t *img,int i)
{
    WCHAR *filename;
    int j;

    if(img->big&&!img->iscopy)free(img->big);
    memset(img,0,sizeof(img_t));

    img->index=boxindex[i];
    filename=(WCHAR *)D(img->index+4);
    if(!*filename)return;

    for(j=0;j<i;j++)
        if(box[j].index&&j!=i)
            if(!wcscmp(filename,(WCHAR *)D(box[j].index+4)))
    {
        //printf("Match %d,'%ws'\n",j,D(box[j].index+4));
        img->big=box[j].big;
        img->bitmap=box[j].bitmap;
        img->hasalpha=box[j].hasalpha;
        img->dc=box[j].dc;
        img->sx=box[j].sx;
        img->sy=box[j].sy;
        img->iscopy=1;
        return;
    }

    if(wcsstr(filename,L"RES_"))
        image_loadRes(img,_wtoi(filename+4));
    else
        image_loadFile(img,filename);
}

void box_free(img_t *img)
{
    if(img->big&&!img->iscopy)free(img->big);
}

void icon_init(img_t *img,int i)
{
    WCHAR *filename;

    if(img->big&&!img->iscopy)free(img->big);
    memset(img,0,sizeof(img_t));

    filename=(WCHAR *)D(i);
    if(wcsstr(filename,L"RES_"))
        image_loadRes(img,_wtoi(filename+4));
    else
        image_loadFile(img,filename);
}

void icon_free(img_t *img)
{
    if(img->big&&!img->iscopy)free(img->big);
}
//}

//{ Draw
void image_load(img_t *img,BYTE *data,int sz)
{
    BYTE *bits,*p1,*p2;
    BITMAPINFO bmi;
    int ret;
    int i;

    img->hasalpha=img->sx=img->sy=0;

    ret=WebPGetInfo((PBYTE)data,sz,&img->sx,&img->sy);
    if(!ret)
    {
        log_err("ERROR in image_load(): failed WebPGetInfo\n");
        return;
    }
    img->big=WebPDecodeBGRA((PBYTE)data,sz,&img->sx,&img->sy);
    if(!img->big)
    {
        log_err("ERROR in image_load(): failed WebPDecodeBGRA\n");
        return;
    }

    ZeroMemory(&bmi,sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth=img->sx;
    bmi.bmiHeader.biHeight=-img->sy;
    bmi.bmiHeader.biPlanes=1;
    bmi.bmiHeader.biBitCount=32;
    bmi.bmiHeader.biCompression=BI_RGB;
    bmi.bmiHeader.biSizeImage=img->sx*img->sy*4;

    img->dc=CreateCompatibleDC(0);
    img->bitmap=CreateDIBSection(img->dc,&bmi,DIB_RGB_COLORS,(void *)&bits,0,0);
    SelectObject(img->dc,img->bitmap);

    p1=bits;p2=img->big;
    for(i=0;i<img->sx*img->sy;i++)
    {
        BYTE B,G,R,A;
        B=*p2++;
        G=*p2++;
        R=*p2++;
        A=*p2++;
        double dA=A/255.;
        if(A!=255)img->hasalpha=1;

        *p1++=(BYTE)(B*dA);
        *p1++=(BYTE)(G*dA);
        *p1++=(BYTE)(R*dA);
        *p1++=A;
    }
    //log_con("%dx%d:%d,%d\n",img->sx,img->sy,img->hasalpha,img->index);
}

void image_loadFile(img_t *img,WCHAR *filename)
{
    WCHAR buf[BUFLEN];
    FILE *f;
    int sz;
    BYTE *imgbuf;

    if(!filename||!*filename)return;
    wsprintf(buf,L"%s\\themes\\%s",data_dir,filename);
    //printf("Loading '%ws'\n",buf);
    f=_wfopen(buf,L"rb");
    if(!f)
    {
        log_err("ERROR in image_loadFile(): file '%ws' not found\n",buf);
        return;
    }
    fseek(f,0,SEEK_END);
    sz=ftell(f);
    fseek(f,0,SEEK_SET);
    imgbuf=malloc(sz);

    sz=fread(imgbuf,1,sz,f);
    if(!sz)
    {
        log_err("ERROR in image_loadFile(): cannnot read from file '%ws'\n",buf);
        return;
    }
    fclose(f);
    image_load(img,imgbuf,sz);
    free(imgbuf);
}

void image_loadRes(img_t *img,int id)
{
    int sz;
    HGLOBAL myResourceData;

    get_resource(id,&myResourceData,&sz);
    if(!sz)
    {
        log_err("ERROR in image_loadRes(): failed get_resource\n");
        return;
    }
    image_load(img,myResourceData,sz);
}

void drawrect(HDC hdc,int x1,int y1,int x2,int y2,int color1,int color2,int w,int rn)
{
    HPEN newpen,oldpen;
    HBRUSH oldbrush;
    unsigned r;

    oldbrush=(HBRUSH)SelectObject(hdc,GetStockObject(color1&0xFF000000?NULL_BRUSH:DC_BRUSH));
    if(!oldbrush)log_err("ERROR in drawrect(): failed SelectObject(GetStockObject)\n");
    r=SetDCBrushColor(hdc,color1);
    if(r==CLR_INVALID)log_err("ERROR in drawrect(): failed SetDCBrushColor\n");

    newpen=CreatePen(w?PS_SOLID:PS_NULL,w,color2);
    if(!newpen)log_err("ERROR in drawrect(): failed CreatePen\n");
    oldpen=(HPEN)SelectObject(hdc,newpen);
    if(!oldpen)log_err("ERROR in drawrect(): failed SelectObject(newpen)\n");

    if(rn)
        RoundRect(hdc,x1,y1,x2,y2,rn,rn);
    else
        Rectangle(hdc,x1,y1,x2,y2);

    r=(int)SelectObject(hdc,oldpen);
    if(!r)log_err("ERROR in drawrect(): failed SelectObject(oldpen)\n");
    r=(int)SelectObject(hdc,oldbrush);
    if(!r)log_err("ERROR in drawrect(): failed SelectObject(oldbrush)\n");
    r=DeleteObject(newpen);
    if(!r)log_err("ERROR in drawrect(): failed SelectObject(newpen)\n");
}

void box_draw(HDC hdc,int x1,int y1,int x2,int y2,int id)
{
    int i=box[id].index;

    if(id<0||id>=BOX_NUM)
    {
        log_err("ERROR in box_draw(): invalid id=%d\n",id);
        return;
    }
    if(i<0||i>=THEME_NM)
    {
        log_err("ERROR in box_draw(): invalid index=%d\n",i);
        return;
    }
    drawrect(hdc,x1,y1,x2,y2,D(i),D(i+1),D(i+2),D(i+3));
    if(box[id].big)
        image_draw(hdc,&box[id],x1,y1,x2,y2,D(i+5),D(i+6));
/*    else
        drawrect(hdc,x1,y1,x2,y2,0xFF,D(i+2),D(i+2),D(i+3));*/
}

void image_draw(HDC dc,img_t *img,int x1,int y1,int x2,int y2,int anchor,int fill)
{
    BLENDFUNCTION blend={AC_SRC_OVER,0,255,AC_SRC_ALPHA};
    int xi,yi,wx,wy;

    if(!img)return;

    wx=(fill&HSTR)?x2-x1:img->sx;
    wy=(fill&VSTR)?y2-y1:img->sy;

    for(xi=0;xi<x2;xi+=wx)
    {
        for(yi=0;yi<y2;yi+=wy)
        {
            int x=x1+xi,y=y1+yi;
            if(anchor&ALIGN_RIGHT)  x=x2-xi-wx;
            if(anchor&ALIGN_BOTTOM) y=y2-yi-wy;
            if(anchor&ALIGN_HCENTER)x=(x2-x1-wx)/2;
            if(anchor&ALIGN_VCENTER)y=(y2-y1-wy)/2;
            if(img->hasalpha)
                AlphaBlend(dc,x,y,wx,wy,img->dc,0,0,img->sx,img->sy,blend);
            else if(wx==img->sx&&wy==img->sy)
                BitBlt(dc,x,y,wx,wy,img->dc,0,0,SRCCOPY);
            else
                StretchBlt(dc,x,y,wx,wy,img->dc,0,0,img->sx,img->sy,SRCCOPY);

            if((fill&VTILE)==0)break;
        }
        if((fill&HTILE)==0)break;
    }
    //drawrect(dc,x1,y1,x2,y2,0xFF000000,0xFF00,1,0);
}

void drawcheckbox(HDC hdc,int x,int y,int wx,int wy,int checked,int active)
{
    RECT rect;
    int i=4+(active?1:0)+(checked?2:0);

    rect.left=x;
    rect.top=y;
    rect.right=x+wx;
    rect.bottom=y+wy;

    if(icon[i].bitmap)
        image_draw(hdc,&icon[i],x,y,x+wx,y+wy,0,HSTR|VSTR);
    else
        DrawFrameControl(hdc,&rect,DFC_BUTTON,DFCS_BUTTONCHECK|(checked?DFCS_CHECKED:0));
}

void drawpopup(int itembar,int type,int x,int y,HWND hwnd)
{
    POINT p={x,y};
    HMONITOR hMonitor;
    MONITORINFO mi;
    int needupdate;

    if((type==FLOATING_CMPDRIVER||type==FLOATING_DRIVERLST)&&itembar<0)type=FLOATING_NONE;
    if(type==FLOATING_TOOLTIP&&(itembar<=1||!*STR(itembar)))type=FLOATING_NONE;

    ClientToScreen(hwnd,&p);
    needupdate=floating_itembar!=itembar||floating_type!=type;
    floating_itembar=itembar;
    floating_type=type;

    if(type!=FLOATING_NONE)
    {
        if(type==FLOATING_ABOUT)p.y=p.y-floating_y-30;
        //if(type==FLOATING_CMPDRIVER||type==FLOATING_DRIVERLST)
        {
            hMonitor=MonitorFromPoint(p,MONITOR_DEFAULTTONEAREST);
            mi.cbSize=sizeof(MONITORINFO);
            GetMonitorInfo(hMonitor,&mi);

            mi.rcWork.right-=15;
            if(p.x+floating_x>mi.rcWork.right)p.x=mi.rcWork.right-floating_x;
            if(p.x<5)p.x=5;
            if(p.y+floating_y>mi.rcWork.bottom-20)p.y=p.y-floating_y-30;
            if(p.y<5)p.y=5;
        }

        MoveWindow(hPopup,p.x+10,p.y+20,floating_x,floating_y,1);
        if(needupdate)InvalidateRect(hPopup,0,0);

        TRACKMOUSEEVENT tme;
        tme.cbSize=sizeof(tme);
        tme.hwndTrack=hwnd;
        tme.dwFlags=TME_LEAVE;
        TrackMouseEvent(&tme);
    }
    ShowWindow(hPopup,type==FLOATING_NONE?SW_HIDE:SW_SHOWNOACTIVATE);
}
//}

//{ Canvas
void canvas_init(canvas_t *canvas)
{
    int r;

    canvas->hdcMem=CreateCompatibleDC(0);
    if(!canvas->hdcMem)log_err("ERROR in canvas_init(): failed CreateCompatibleDC\n");
    r=SetBkMode(canvas->hdcMem,TRANSPARENT);
    if(!r)log_err("ERROR in canvas_init(): failed SetBkMode\n");
    canvas->bitmap=0;
    canvas->x=0;
    canvas->y=0;
}

void canvas_free(canvas_t *canvas)
{
    int r;

    if(canvas->hdcMem)
    {
        r=DeleteDC(canvas->hdcMem);
        if(!r)log_err("ERROR in canvas_free(): failed DeleteDC\n");
    }

    if(canvas->bitmap)
    {
        //r=(int)SelectObject(canvas->hdcMem,canvas->oldbitmap);
        //if(!r)log_err("ERROR in canvas_free(): failed SelectObject\n");
        r=DeleteObject(canvas->bitmap);
        if(!r)log_err("ERROR in canvas_free(): failed DeleteObject\n");
    }
}

void canvas_begin(canvas_t *canvas,HWND hwnd,int x,int y)
{
    int r;

    canvas->hwnd=hwnd;
    canvas->localDC=BeginPaint(hwnd,&canvas->ps);
    if(!canvas->localDC)log_err("ERROR in canvas_begin(): failed BeginPaint\n");

    if(canvas->x!=x||canvas->y!=y)
    {
        canvas->x=x;
        canvas->y=y;
        if(canvas->bitmap)
        {
            r=(int)SelectObject(canvas->hdcMem,canvas->oldbitmap);
            if(!r)log_err("ERROR in canvas_begin(): failed SelectObject(oldbitmap)\n");
            r=DeleteObject(canvas->bitmap);
            if(!r)log_err("ERROR in canvas_begin(): failed DeleteObject\n");
        }
        canvas->bitmap=CreateCompatibleBitmap(canvas->localDC,x,y);
        if(!canvas->bitmap)log_err("ERROR in canvas_begin(): failed CreateCompatibleBitmap\n");
        canvas->oldbitmap=SelectObject(canvas->hdcMem,canvas->bitmap);
        if(!canvas->oldbitmap)log_err("ERROR in canvas_begin(): failed SelectObject(bitmap)\n");
    }
    canvas->clipping=CreateRectRgnIndirect(&canvas->ps.rcPaint);
    if(!canvas->clipping)log_err("ERROR in canvas_begin(): failed BeginPaint\n");
    SetStretchBltMode(canvas->hdcMem,HALFTONE);
    //r=SelectClipRgn(canvas->hdcMem,canvas->clipping);
    //if(!r)log_err("ERROR in canvas_begin(): failed SelectClipRgn\n");
}

void canvas_end(canvas_t *canvas)
{
    int r;

    r=BitBlt(canvas->localDC,
            canvas->ps.rcPaint.left,canvas->ps.rcPaint.top,canvas->ps.rcPaint.right,canvas->ps.rcPaint.bottom,
            canvas->hdcMem,
            canvas->ps.rcPaint.left,canvas->ps.rcPaint.top,
            SRCCOPY);
    SelectClipRgn(canvas->hdcMem,0);
    if(!r)log_err("ERROR in canvas_end(): failed BitBlt\n");
    r=DeleteObject(canvas->clipping);
    if(!r)log_err("ERROR in canvas_end(): failed DeleteObject\n");
    EndPaint(canvas->hwnd,&canvas->ps);
}
//}
