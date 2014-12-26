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

panelitem_t panel1[]=
{
    {TYPE_GROUP,0,3,0},
    {TYPE_TEXT,STR_SHOW_SYSINFO,0,0},
    {TYPE_TEXT,0,0,0},
    {TYPE_TEXT,0,0,0},
};

panelitem_t panel2[]=
{
    {TYPE_GROUP,0,3,0},
    {0,STR_INSTALL,               ID_INSTALL,0},
    {0,STR_SELECT_ALL,            ID_SELECT_ALL,0},
    {0,STR_SELECT_NONE,           ID_SELECT_NONE,0},
};

panelitem_t panel3[]=
{
    {TYPE_GROUP,0,5,0},
    {TYPE_TEXT,STR_LANG,0,0},
    {TYPE_TEXT,0,0,0},
    {TYPE_TEXT,STR_THEME,0,0},
    {TYPE_TEXT,0,0,0},
    {TYPE_CHECKBOX,STR_EXPERT,              ID_EXPERT_MODE,0},
};

panelitem_t panel3_w[]=
{
    {TYPE_GROUP,0,3,0},
    {TYPE_TEXT,STR_LANG,0,0},
    {TYPE_TEXT,STR_THEME,0,0},
    {TYPE_CHECKBOX,STR_EXPERT,              ID_EXPERT_MODE,0},
};

panelitem_t panel4[]=
{
    {TYPE_GROUP_BREAK,0,4,0},
    {TYPE_BUTTON,STR_OPENLOGS,              ID_OPENLOGS,0},
    {TYPE_BUTTON,STR_SNAPSHOT,              ID_SNAPSHOT,0},
    {TYPE_BUTTON,STR_EXTRACT,               ID_EXTRACT,0},
    {TYPE_BUTTON,STR_DRVDIR,                ID_DRVDIR,0},
};

panelitem_t panel5[]=
{
    {TYPE_GROUP_BREAK,0,7,0},
    {TYPE_TEXT,STR_SHOW_FOUND,0,0},
    {TYPE_CHECKBOX, STR_SHOW_MISSING,       ID_SHOW_MISSING,0},
    {TYPE_CHECKBOX, STR_SHOW_NEWER,         ID_SHOW_NEWER,0},
    {TYPE_CHECKBOX, STR_SHOW_CURRENT,       ID_SHOW_CURRENT,0},
    {TYPE_CHECKBOX, STR_SHOW_OLD,           ID_SHOW_OLD,0},
    {TYPE_CHECKBOX, STR_SHOW_BETTER,        ID_SHOW_BETTER,0},
    {TYPE_CHECKBOX, STR_SHOW_WORSE_RANK,    ID_SHOW_WORSE_RANK,0},
};

panelitem_t panel6[]=
{
    {TYPE_GROUP_BREAK,0,4,0},
    {TYPE_TEXT,STR_SHOW_NOTFOUND,0,0},
    {TYPE_CHECKBOX, STR_SHOW_NF_MISSING,    ID_SHOW_NF_MISSING,0},
    {TYPE_CHECKBOX, STR_SHOW_NF_UNKNOWN,    ID_SHOW_NF_UNKNOWN,0},
    {TYPE_CHECKBOX, STR_SHOW_NF_STANDARD,   ID_SHOW_NF_STANDARD,0},
};

panelitem_t panel7[]=
{
    {TYPE_GROUP_BREAK,0,3,0},
    {TYPE_CHECKBOX, STR_SHOW_ONE,           ID_SHOW_ONE,0},
    {TYPE_CHECKBOX, STR_SHOW_DUP,           ID_SHOW_DUP,0},
    {TYPE_CHECKBOX, STR_SHOW_INVALID,       ID_SHOW_INVALID,0},

};

panelitem_t panel8[]=
{
    {TYPE_GROUP,0,1,0},
    {TYPE_TEXT,0,0,0},
};

panelitem_t panel9[]=
{
    {TYPE_GROUP,0,1,0},
    {TYPE_BUTTON,STR_INSTALL,               ID_INSTALL,0},
};

panelitem_t panel10[]=
{
    {TYPE_GROUP,0,1,0},
    {TYPE_BUTTON,STR_SELECT_ALL,            ID_SELECT_ALL,0},
};

panelitem_t panel11[]=
{
    {TYPE_GROUP,0,1,0},
    {TYPE_BUTTON,STR_SELECT_NONE,           ID_SELECT_NONE,0},
};

panelitem_t panel12[]=
{
    {TYPE_GROUP,0,3,0},
    {TYPE_TEXT,STR_OPTIONS,0,0},
    {TYPE_CHECKBOX,STR_RESTOREPOINT,        ID_RESTPNT,0},
    {TYPE_CHECKBOX,STR_REBOOT,              ID_REBOOT,0},
};

panelitem_t panel13[]=
{
    {TYPE_GROUP,0,1,0},
    {TYPE_TEXT,0,0,0},
};

panel_t panels[NUM_PANELS]=
{
    {panel1,  0},
    {panel2,  1},
    {panel3,  2},
    {panel4,  3},
    {panel5,  4},
    {panel6,  5},
    {panel7,  6},
    {panel8,  7},
    {panel9,  8},
    {panel10, 9},
    {panel11,10},
    {panel12,11},
    {panel13,12},
};
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
    int j,r;

    if(img->big&&!img->iscopy)
    {
        r=DeleteObject(img->bitmap);
            if(!r)log_err("ERROR in box_init(): failed DeleteObject\n");
        r=DeleteDC(img->dc);
            if(!r)log_err("ERROR in box_init(): failed DeleteDC\n");
        free(img->big);
    }
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

    if(img->big&&!img->iscopy)
    {
        int r;
        r=DeleteObject(img->bitmap);
            if(!r)log_err("ERROR in icon_init(): failed DeleteObject\n");
        free(img->big);

    }
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
#ifdef CONSOLE_MODE
    return;
#else
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
#endif

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
    uintptr_t *r;
    unsigned r32;

    oldbrush=(HBRUSH)SelectObject(hdc,GetStockObject(color1&0xFF000000?NULL_BRUSH:DC_BRUSH));
    if(!oldbrush)log_err("ERROR in drawrect(): failed SelectObject(GetStockObject)\n");
    r32=SetDCBrushColor(hdc,color1);
    if(r32==CLR_INVALID)log_err("ERROR in drawrect(): failed SetDCBrushColor\n");

    newpen=CreatePen(w?PS_SOLID:PS_NULL,w,color2);
    if(!newpen)log_err("ERROR in drawrect(): failed CreatePen\n");
    oldpen=(HPEN)SelectObject(hdc,newpen);
    if(!oldpen)log_err("ERROR in drawrect(): failed SelectObject(newpen)\n");

    if(rn)
        RoundRect(hdc,x1,y1,x2,y2,rn,rn);
    else
        Rectangle(hdc,x1,y1,x2,y2);

    r=SelectObject(hdc,oldpen);
    if(!r)log_err("ERROR in drawrect(): failed SelectObject(oldpen)\n");
    r=SelectObject(hdc,oldbrush);
    if(!r)log_err("ERROR in drawrect(): failed SelectObject(oldbrush)\n");
    r32=DeleteObject(newpen);
    if(!r32)log_err("ERROR in drawrect(): failed SelectObject(newpen)\n");
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
    int xi,yi,wx,wy,wx1,wy1,wx2,wy2;

    if(!img)return;

    wx=(fill&HSTR)?x2-x1:img->sx;
    wy=(fill&VSTR)?y2-y1:img->sy;
    if(fill&ASPECT)
    {
        if(fill&HSTR)wy=img->sy*((double)wx/img->sx);
        if(fill&VSTR)wx=img->sx*((double)wy/img->sy);
    }


    for(xi=0;xi<x2;xi+=wx)
    {
        for(yi=0;yi<y2;yi+=wy)
        {
            int x=x1+xi,y=y1+yi;
            if(anchor&ALIGN_RIGHT)  x=x2-xi-wx;
            if(anchor&ALIGN_BOTTOM) y=y2-yi-wy;
            if(anchor&ALIGN_HCENTER)x=(x2-x1-wx)/2;
            if(anchor&ALIGN_VCENTER)y=(y2-y1-wy)/2;

            wx1=(x+wx>x2)?x2-x:wx;
            wy1=(y+wy>y2)?y2-y:wy;
            wx2=(x+wx>x2)?wx1:img->sx;
            wy2=(y+wy>y2)?wy1:img->sy;

            if(img->hasalpha)
                AlphaBlend(dc,x,y,wx1,wy1,img->dc,0,0,wx2,wy2,blend);
            else if(wx==wx2&&wy==wy2)
                BitBlt(dc,x,y,wx1,wy1,img->dc,0,0,SRCCOPY);
            else
                StretchBlt(dc,x,y,wx1,wy1,img->dc,0,0,wx2,wy2,SRCCOPY);

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
        //if(type==FLOATING_ABOUT)p.y=p.y-floating_y-30;
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
        tme.dwFlags=TME_LEAVE|TME_HOVER;
        tme.dwHoverTime=(ctrl_down||space_down)?1:hintdelay;
        TrackMouseEvent(&tme);
    }
    //ShowWindow(hPopup,type==FLOATING_NONE?SW_HIDE:SW_SHOWNOACTIVATE);
    if(type==FLOATING_NONE)ShowWindow(hPopup,SW_HIDE);
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
    uintptr_t *r;
    unsigned r32;

    canvas->hwnd=hwnd;
    canvas->localDC=BeginPaint(hwnd,&canvas->ps);
    if(!canvas->localDC)log_err("ERROR in canvas_begin(): failed BeginPaint\n");

    if(canvas->x!=x||canvas->y!=y)
    {
        canvas->x=x;
        canvas->y=y;
        if(canvas->bitmap)
        {
            r=SelectObject(canvas->hdcMem,canvas->oldbitmap);
            if(!r)log_err("ERROR in canvas_begin(): failed SelectObject(oldbitmap)\n");
            r32=DeleteObject(canvas->bitmap);
            if(!r32)log_err("ERROR in canvas_begin(): failed DeleteObject\n");
        }
        canvas->bitmap=CreateCompatibleBitmap(canvas->localDC,x,y);
        if(!canvas->bitmap)log_err("ERROR in canvas_begin(): failed CreateCompatibleBitmap\n");
        canvas->oldbitmap=SelectObject(canvas->hdcMem,canvas->bitmap);
        if(!canvas->oldbitmap)log_err("ERROR in canvas_begin(): failed SelectObject(bitmap)\n");
    }
    canvas->clipping=CreateRectRgnIndirect(&canvas->ps.rcPaint);
    if(!canvas->clipping)log_err("ERROR in canvas_begin(): failed BeginPaint\n");
    SetStretchBltMode(canvas->hdcMem,HALFTONE);
    r32=SelectClipRgn(canvas->hdcMem,canvas->clipping);
    if(!r32)log_err("ERROR in canvas_begin(): failed SelectClipRgn\n");
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

//{ Panel
int panel_hitscan(panel_t *panel,int hx,int hy)
{
    int idofs=PAN_ENT*panel->index+PAN_ENT;
    int wy=D(PANEL_WY+idofs);

    if(!wy)return -1;
    hx-=Xp(panel)+D(PNLITEM_OFSX);
    hy-=Yp(panel)+D(PNLITEM_OFSY);

    if(!expertmode&&panel->items[0].type==TYPE_GROUP_BREAK)return -2;
    if(hx<0||hy<0||hx>XP(panel)-D(PNLITEM_OFSX)*2)return -3;
    if(hy/wy>=panel->items[0].action_id)return -4;
    return hy/wy+1;
}

int panels_hitscan(int hx,int hy,int *ii)
{
    int i,r=-1;

    *ii=-1;
    for(i=0;i<NUM_PANELS;i++)
    {
        r=panel_hitscan(&panels[i],hx,hy);
        if(r>=0&&panels[i].items[r].type)
        {
            *ii=i;
            return r;
        }
    }
    return -1;
}

void panel_draw_inv(panel_t *panel)
{
    int x=Xp(panel),y=Yp(panel);
    int idofs=PAN_ENT*panel->index+PAN_ENT;
    int wy=D(PANEL_WY+idofs);
    int ofsy=D(PNLITEM_OFSY);
    RECT rect;

    if(!panel)return;
    rect.left=x;
    rect.top=y;
    rect.right=x+XP(panel);
    rect.bottom=y+(wy+1)*panel->items[0].action_id+ofsy*2;
    InvalidateRect(hMain,&rect,0);
}

void panel_draw(HDC hdc,panel_t *panel)
{
    WCHAR buf[BUFLEN];
    POINT p;
    HRGN rgn=0;
    int cur_i;
    int i;
    int idofs=PAN_ENT*panel->index+PAN_ENT;
    int x=Xp(panel),y=Yp(panel);
    int ofsx=D(PNLITEM_OFSX),ofsy=D(PNLITEM_OFSY);
    int wy=D(PANEL_WY+idofs);

    if(XP(panel)<0)return;
    //if(panel_lasti/256!=panel->index)return;

    GetCursorPos(&p);
    ScreenToClient(hMain,&p);
    cur_i=panel_hitscan(panel,p.x,p.y);

    if(!D(PANEL_WY+idofs))return;
    for(i=0;i<panel->items[0].action_id+1;i++)
    {
        if(i==1&&panel->index==0)
        {
            wsprintf(buf,L"%s",STR(STR_SYSINF_MOTHERBOARD));
            SetTextColor(hdc,D(CHKBOX_TEXT_COLOR));
            TextOut(hdc,x+ofsx+SYSINFO_COL1,y+ofsy,buf,wcslen(buf));

            wsprintf(buf,L"%s",STR(STR_SYSINF_ENVIRONMENT));
            SetTextColor(hdc,D(CHKBOX_TEXT_COLOR));
            TextOut(hdc,x+ofsx+SYSINFO_COL2,y+ofsy,buf,wcslen(buf));
        }
        if(i==2&&panel->index==0)
        {
            wsprintf(buf,L"%s (%d-bit)",get_winverstr(manager_g),manager_g->matcher->state->architecture?64:32);
            SetTextColor(hdc,D(CHKBOX_TEXT_COLOR));
            TextOut(hdc,x+ofsx+10+SYSINFO_COL0,y+ofsy,buf,wcslen(buf));

            wsprintf(buf,L"%s",state_getproduct(manager_g->matcher->state));
            SetTextColor(hdc,D(CHKBOX_TEXT_COLOR));
            TextOut(hdc,x+ofsx+10+SYSINFO_COL1,y+ofsy,buf,wcslen(buf));

            wsprintf(buf,L"%s",STR(STR_SYSINF_WINDIR));
            SetTextColor(hdc,D(CHKBOX_TEXT_COLOR));
            TextOut(hdc,x+ofsx+10+SYSINFO_COL2,y+ofsy,buf,wcslen(buf));

            wsprintf(buf,L"%s",manager_g->matcher->state->text+manager_g->matcher->state->windir);
            SetTextColor(hdc,D(CHKBOX_TEXT_COLOR));
            TextOut(hdc,x+ofsx+10+SYSINFO_COL3,y+ofsy,buf,wcslen(buf));
        }
        if(i==3&&panel->index==0)
        {
            if(XP(panel)<10+SYSINFO_COL1)
                wsprintf(buf,L"%s",state_getproduct(manager_g->matcher->state));
            else
                wsprintf(buf,L"%s",manager_g->matcher->state->platform.szCSDVersion);

            SetTextColor(hdc,D(CHKBOX_TEXT_COLOR));
            TextOut(hdc,x+ofsx+10+SYSINFO_COL0,y+ofsy,buf,wcslen(buf));

            wsprintf(buf,L"%s: %s",STR(STR_SYSINF_TYPE),STR(isLaptop?STR_SYSINF_LAPTOP:STR_SYSINF_DESKTOP));
            SetTextColor(hdc,D(CHKBOX_TEXT_COLOR));
            TextOut(hdc,x+ofsx+10+SYSINFO_COL1,y+ofsy,buf,wcslen(buf));

            wsprintf(buf,L"%s",STR(STR_SYSINF_TEMP));
            SetTextColor(hdc,D(CHKBOX_TEXT_COLOR));
            TextOut(hdc,x+ofsx+10+SYSINFO_COL2,y+ofsy,buf,wcslen(buf));

            wsprintf(buf,L"%s",manager_g->matcher->state->text+manager_g->matcher->state->temp);
            SetTextColor(hdc,D(CHKBOX_TEXT_COLOR));
            TextOut(hdc,x+ofsx+10+SYSINFO_COL3,y+ofsy,buf,wcslen(buf));
        }
        if(panel->items[i].type==TYPE_GROUP_BREAK&&!expertmode)break;
        switch(panel->items[i].type)
        {
            case TYPE_CHECKBOX:
                drawcheckbox(hdc,x+ofsx,y+ofsy,D(CHKBOX_SIZE)-2,D(CHKBOX_SIZE)-2,panel->items[i].checked,i==cur_i);
                SetTextColor(hdc,D(i==cur_i?CHKBOX_TEXT_COLOR_H:CHKBOX_TEXT_COLOR));
                TextOut(hdc,x+D(CHKBOX_TEXT_OFSX)+ofsx,y+ofsy,STR(panel->items[i].str_id),wcslen(STR(panel->items[i].str_id)));
                y+=D(PNLITEM_WY);
                break;

            case TYPE_BUTTON:
                if(panel->index>=8&&panel->index<=10&&D(PANEL_OUTLINE_WIDTH+idofs)<0)
                    box_draw(hdc,x+ofsx,y+ofsy,x+XP(panel)-ofsx,y+ofsy+wy,i==cur_i?BOX_PANEL_H+panel->index*2+2:BOX_PANEL+panel->index*2+2);
                else
                    box_draw(hdc,x+ofsx,y+ofsy,x+XP(panel)-ofsx,y+ofsy+wy-1,i==cur_i?BOX_BUTTON_H:BOX_BUTTON);

                SetTextColor(hdc,D(CHKBOX_TEXT_COLOR));
                if(i==1&&panel->index==8)
                {
                    int j,cnt=0;
                    itembar_t *itembar;

                    itembar=&manager_g->items_list[RES_SLOTS];
                    for(j=RES_SLOTS;j<manager_g->items_handle.items;j++,itembar++)
                    if(itembar->checked)cnt++;

                    wsprintf(buf,L"%s (%d)",STR(panel->items[i].str_id),cnt);
                    TextOut(hdc,x+ofsx+wy/2,y+ofsy+(wy-D(FONT_SIZE)-2)/2,buf,wcslen(buf));
                }
                else
                    TextOut(hdc,x+ofsx+wy/2,y+ofsy+(wy-D(FONT_SIZE)-2)/2,STR(panel->items[i].str_id),wcslen(STR(panel->items[i].str_id)));

                y+=D(PNLITEM_WY);
                break;

            case TYPE_TEXT:
                if(i==1&&panel->index==7)
                {
                    version_t v;

                    v.d=atoi(SVN_REV_D);
                    v.m=atoi(SVN_REV_M);
                    v.y=SVN_REV_Y;

                    wsprintf(buf,L"%s (",TEXT(SVN_REV2));
                    str_date(&v,buf+wcslen(buf));
                    wcscat(buf,L")");
                    SetTextColor(hdc,D(CHKBOX_TEXT_COLOR));
                    TextOut(hdc,x+ofsx,y+ofsy,buf,wcslen(buf));
                }
                SetTextColor(hdc,D(i==cur_i&&i>11?CHKBOX_TEXT_COLOR_H:CHKBOX_TEXT_COLOR));
                TextOut(hdc,x+ofsx,y+ofsy,STR(panel->items[i].str_id),wcslen(STR(panel->items[i].str_id)));
                y+=D(PNLITEM_WY);
                break;

            case TYPE_GROUP_BREAK:
            case TYPE_GROUP:
                if(panel->index>=8&&panel->index<=10)break;
                if(i)y+=D(PNLITEM_WY);
                box_draw(hdc,x,y,x+XP(panel),y+(wy)*panel->items[i].action_id+ofsy*2,
                         BOX_PANEL+panel->index*2+2);
                rgn=CreateRectRgn(x,y,x+XP(panel),y+(wy)*panel->items[i].action_id+ofsy*2);
                SelectClipRgn(hdc,rgn);
                break;

            default:
                break;
        }

    }
    if(rgn)
    {
        SelectClipRgn(hdc,0);
        DeleteObject(rgn);
    }
}
//}
