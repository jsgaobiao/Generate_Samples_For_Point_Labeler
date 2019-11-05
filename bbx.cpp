#include "bbx.h"

//void GetRectPts(point2d *cp, point2d *v1, double olen1, double olen2, point2d *p, double margin);

std::vector<GROUP> groups;
MASTER mapInfo;

//int	select_grpid = -1;
//int	select_trkid = -1;
//int select_bbxid = -1;

bool LoadMaster(char *szFile)
{
    FILE *fp;
    fp = fopen(szFile, "r");
    if (!fp) return false;
    fscanf(fp, "width %d\n", &mapInfo.width);
    fscanf(fp, "length %d\n", &mapInfo.length);
    fscanf(fp, "ox %lf\n", &mapInfo.ox);
    fscanf(fp, "oy %lf\n", &mapInfo.oy);
    fscanf(fp, "pixelsize %lf\n", &mapInfo.pixelsize);
    return true;
}

bool LoadBbx(char *szFile)
{
    char			i_line[200];
    FILE			*fp;

    groups.clear();

    fp = fopen(szFile, "r");
    if (!fp)
        return false;

    GROUP	onegrp;
    TRACK	onetrk;
    BBX		onebbx;
    int		gid=0;

    onegrp.tracks.clear();
    onetrk.bbxs.clear();

    while (1) {
        if (fgets(i_line, 200, fp) == NULL)
            break;

        if (strlen(i_line) < 3)
            continue;

        if (strncmp(i_line, "GROUP", 5) == 0) {

            if (onetrk.bbxs.size()) {
                onetrk.val = true;
                onegrp.tracks.push_back(onetrk);
                onetrk.bbxs.clear();
            }
            if (onegrp.tracks.size()) {
                onegrp.val = true;
                groups.push_back(onegrp);
                onegrp.tracks.clear();
            }
            strtok(i_line, "=,\t\n");
            onegrp.smilli = atoi(strtok(NULL, "=,\t\n"));
            onegrp.emilli = atoi(strtok(NULL, "=,\t\n"));
            onegrp.sfn = atoi(strtok(NULL, "=,\t\n"));
            onegrp.efn = atoi(strtok(NULL, "=,\t\n"));
            onegrp.isstatic = atoi(strtok(NULL, "=,\t\n"));
            onegrp.label = atoi(strtok(NULL, "=,\t\n"));
            onegrp.bbx.w1 = atof(strtok(NULL, "=,\t\n"));
            onegrp.bbx.w2 = atof(strtok(NULL, "=,\t\n"));
            onegrp.bbx.h1 = atof(strtok(NULL, "=,\t\n"));
            continue;
        }
        else if (strncmp(i_line, "TRACK", 5) == 0) {
            if (onetrk.bbxs.size()) {
                onetrk.val = true;
                onegrp.tracks.push_back(onetrk);
                onetrk.bbxs.clear();
            }
            strtok(i_line, "=,\t\n");
            onetrk.prid = atoi(strtok(NULL, "=,\t\n"));
            onetrk.smilli = atoi(strtok(NULL, "=,\t\n"));
            onetrk.emilli = atoi(strtok(NULL, "=,\t\n"));
            onetrk.sfn = atoi(strtok(NULL, "=,\t\n"));
            onetrk.efn = atoi(strtok(NULL, "=,\t\n"));
            onetrk.isstatic = atoi(strtok(NULL, "=,\t\n"));
            onetrk.SVrat = atof(strtok(NULL, "=,\t\n"));
            onetrk.mptnum = atof(strtok(NULL, "=,\t\n"));
            continue;
        }
        else if (strncmp(i_line, "MBBX", 4) == 0) {
            strtok(i_line, "=,\t\n");
            onebbx.milli = atoi(strtok(NULL, "=,\t\n"));
            onebbx.cp.x = atof(strtok(NULL, "=,\t\n"));
            onebbx.cp.y = atof(strtok(NULL, "=,\t\n"));
            onebbx.cornerpt.x = atof(strtok(NULL, "=,\t\n"));
            onebbx.cornerpt.y = atof(strtok(NULL, "=,\t\n"));
            onebbx.hv.x = atof(strtok(NULL, "=,\t\n"));
            onebbx.hv.y = atof(strtok(NULL, "=,\t\n"));
            onebbx.h0 = atof(strtok(NULL, "=,\t\n"));
            onebbx.var = atof(strtok(NULL, "=,\t\n"));
            onebbx.val = true;
            onetrk.bbxs.push_back(onebbx);
        }
        else continue;

    }
    if (onetrk.bbxs.size()) {
        onetrk.val = true;
        onegrp.tracks.push_back(onetrk);
        onetrk.bbxs.clear();
    }
    if (onegrp.tracks.size()) {
        onegrp.val = true;
        groups.push_back(onegrp);
        onegrp.tracks.clear();
    }

    fclose(fp);

    return groups.size() ? true : false;
}

//void OutputBbx (char *filename)
//{
//    FILE	*bbxfp = NULL;
//    bbxfp = fopen(filename, "w");
//    if (!bbxfp) {
//        printf("Failed in opening file %s\n", filename);
//        return;
//    }
//    for (int j = 0; j < groups.size(); j++) {
//        GROUP	*gr = &groups[j];
//        if (!gr->tracks.size() || !gr->val)
//            continue;
//        //GROUP,smilli,emilli,sfn,efn,isstatic,label, width, length, height
//        //label:与标注软件的定义一致
//        fprintf(bbxfp, "GROUP,%d,%d,%d,%d,%d,%d,%.3f,%.3f,%.3f\n", gr->smilli, gr->emilli, gr->sfn, gr->efn, gr->isstatic, gr->label, gr->bbx.w1, gr->bbx.w2, gr->bbx.h1);
//        for (int k = 0; k < gr->tracks.size(); k++) {
//            TRACK *tr = &gr->tracks[k];
//            if (!tr->bbxs.size() || !tr->val)
//                continue;
//            fprintf(bbxfp, "TRACK,%d,%d,%d,%d,%d,%d,%.3f,%d\n", tr->prid, tr->smilli, tr->emilli, tr->sfn, tr->efn, tr->isstatic, tr->SVrat, tr->mptnum);
//            for (int i = 0; i < tr->bbxs.size(); i++) {
//                if (!tr->bbxs[i].val)
//                    continue;
//                fprintf(bbxfp, "MBBX,");
//                fprintf(bbxfp, "%d,", tr->bbxs[i].milli);
//                fprintf(bbxfp, "%.3f,%.3f,", tr->bbxs[i].cp.x, tr->bbxs[i].cp.y);
//                fprintf(bbxfp, "%.3f,%.3f,", tr->bbxs[i].cornerpt.x, tr->bbxs[i].cornerpt.y);
//                fprintf(bbxfp, "%.3f,%.3f,", tr->bbxs[i].hv.x, tr->bbxs[i].hv.y);
//                fprintf(bbxfp, "%.3f,%.3f\n", tr->bbxs[i].h0, tr->bbxs[i].var);
//            }
//        }
//    }
//    fclose(bbxfp);
//}

void rotatePoint90(point2d *p, point2d *p90)
{
    p90->x = -p->y;
    p90->y = p->x;
}

// 得到中心点和边长计算四个角点位置
void GetRectPts(point2d *cp, point2d *v1, double olen1, double olen2, point2d *p, double margin)
{
    double	len1 = olen1 / 2 + margin;
    double	len2 = olen2 / 2 + margin;

    point2d	v2;
    rotatePoint90(v1, &v2);

    // 1,-1
    p[0].x = cp->x + v1->x*len1 - v2.x*len2;
    p[0].y = cp->y + v1->y*len1 - v2.y*len2;

    // 1,1
    p[1].x = cp->x + v1->x*len1 + v2.x*len2;
    p[1].y = cp->y + v1->y*len1 + v2.y*len2;

    // -1,1
    p[2].x = cp->x - v1->x*len1 + v2.x*len2;
    p[2].y = cp->y - v1->y*len1 + v2.y*len2;

    // -1,-1
    p[3].x = cp->x - v1->x*len1 - v2.x*len2;
    p[3].y = cp->y - v1->y*len1 - v2.y*len2;

}
//void DrawSelected_Bbx(HDC hDC, double iZoom)
//{
//    if (select_grpid < 0 || select_trkid < 0 || select_bbxid < 0)
//        return;
//    GROUP	*gr = &groups[select_grpid];
//    if (!gr->tracks.size() || !gr->val)
//        return;
//    TRACK	*tr = &gr->tracks[select_trkid];
//    if (!tr->bbxs.size() || !tr->val)
//        return;
//    BBX		*bx = &tr->bbxs[select_bbxid];
//    if (!bx->val)
//        return;
//    SelectObject(hDC, hPen[GREEN]);
//    point2d	p[4], ip[4];
//    POINT	dispt[4];
//    GetRectPts(&bx->cp, &bx->hv, gr->bbx.w1, gr->bbx.w2, p, 0);
//    for (int i = 0; i < 4; i++) {
//        WC2BG(&p[i], &ip[i]);
//        dispt[i].x = ip[i].x*iZoom;
//        dispt[i].y = ip[i].y*iZoom;
//    }
//    MoveToEx(hDC, dispt[3].x, dispt[3].y, NULL);
//    for (int i = 0; i < 4; i++)
//        LineTo(hDC, dispt[i].x, dispt[i].y);
//}

//void DrawSelected_Trk(HDC hDC, double iZoom)
//{
//    if (select_grpid < 0 || select_trkid < 0)
//        return;
//    GROUP	*gr = &groups[select_grpid];
//    if (!gr->tracks.size() || !gr->val)
//        return;
//    TRACK	*tr = &gr->tracks[select_trkid];
//    if (!tr->bbxs.size() || !tr->val)
//        return;
//    SelectObject(hDC, hPen[GREEN]);
//    for (int nb = 0; nb < tr->bbxs.size(); nb++) {
//        BBX		*bx = &tr->bbxs[nb];
//        if (!bx->val)
//            continue;
//        point2d	p[4], ip[4];
//        POINT	dispt[4];
//        GetRectPts(&bx->cp, &bx->hv, gr->bbx.w1, gr->bbx.w2, p, 0);
//        for (int i = 0; i < 4; i++) {
//            WC2BG(&p[i], &ip[i]);
//            dispt[i].x = ip[i].x*iZoom;
//            dispt[i].y = ip[i].y*iZoom;
//        }
//        MoveToEx(hDC, dispt[3].x, dispt[3].y, NULL);
//        for (int i = 0; i < 4; i++)
//            LineTo(hDC, dispt[i].x, dispt[i].y);
//    }
//}

//void DrawSelected_Grp(HDC hDC, double iZoom)
//{
//    if (select_grpid < 0)
//        return;
//    GROUP	*gr = &groups[select_grpid];
//    if (!gr->tracks.size() || !gr->val)
//        return;
//    SelectObject(hDC, hPen[GREEN]);
//    for (int nt = 0; nt < gr->tracks.size(); nt++) {
//        TRACK	*tr = &gr->tracks[nt];
//        if (!tr->bbxs.size() || !tr->val)
//            continue;
//        for (int nb = 0; nb < tr->bbxs.size(); nb++) {
//            BBX		*bx = &tr->bbxs[nb];
//            if (!bx->val)
//                continue;
//            point2d	p[4], ip[4];
//            POINT	dispt[4];
//            GetRectPts(&bx->cp, &bx->hv, gr->bbx.w1, gr->bbx.w2, p, 0);
//            for (int i = 0; i < 4; i++) {
//                WC2BG(&p[i], &ip[i]);
//                dispt[i].x = ip[i].x*iZoom;
//                dispt[i].y = ip[i].y*iZoom;
//            }
//            MoveToEx(hDC, dispt[3].x, dispt[3].y, NULL);
//            for (int i = 0; i < 4; i++)
//                LineTo(hDC, dispt[i].x, dispt[i].y);
//        }
//    }
//}

//void DrawBbx(HDC hDC, double iZoom, double x1, double y1, double x2, double y2)
//{
//    for (int ng = 0; ng < groups.size(); ng++) {
//        GROUP	*gr = &groups[ng];
//        if (!gr->tracks.size() || !gr->val)
//            continue;

//        for (int nt = 0; nt < gr->tracks.size(); nt++) {
//            if (gr->isstatic)
//                SelectObject(hDC, hPen[RED]);
//            else
//                SelectObject(hDC, hPen[ng%PENNUM]);
//            TRACK	*tr = &gr->tracks[nt];
//            if (!tr->bbxs.size() || !tr->val)
//                continue;
//            BBX		*bx;
//            point2d	p[4], ip[4];
//            POINT	dispt[4];
//            for (int nb = 0; nb < tr->bbxs.size(); nb++) {
//                bx = &tr->bbxs[nb];
//                if (!bx->val)
//                    continue;
//                GetRectPts(&bx->cp, &bx->hv, gr->bbx.w1, gr->bbx.w2, p, 0);
//                for (int i = 0; i < 4; i++) {
//                    WC2BG(&p[i], &ip[i]);
//                    dispt[i].x = ip[i].x*iZoom;
//                    dispt[i].y = ip[i].y*iZoom;
//                }
//                MoveToEx(hDC, dispt[3].x, dispt[3].y, NULL);
//                for (int i = 0; i < 4; i++)
//                    LineTo(hDC, dispt[i].x, dispt[i].y);
//                if (gr->isstatic)
//                    break;
//            }
//            if (gr->isstatic)
//                continue;
//            SelectObject(hDC, hPen[GREEN]);
//            for (int nb = 0; nb < tr->bbxs.size(); nb++) {
//                bx = &tr->bbxs[nb];
//                WC2BG(&bx->cp, &ip[0]);
//                dispt[0].x = ip[0].x*iZoom;
//                dispt[0].y = ip[0].y*iZoom;
//                if (!nb)
//                    MoveToEx(hDC, dispt[0].x, dispt[0].y, NULL);
//                else
//                    LineTo(hDC, dispt[0].x, dispt[0].y);
//            }
//        }
//    }

//    if (op != _OP_EDIT_ONEBBX && op != _OP_EDIT_TRKBBX && op != _OP_EDIT_GRPBBX)
//        return;

//    switch (op) {
//    case  _OP_EDIT_GRPBBX:
//        DrawSelected_Grp(hDC, iZoom);
//        break;
//    case  _OP_EDIT_TRKBBX:
//        DrawSelected_Trk(hDC, iZoom);
//        break;
//    case _OP_EDIT_ONEBBX:
//        DrawSelected_Bbx(hDC, iZoom);
//        break;
//    }
//    return;

//}

double Dist2D(double x, double y)
{
    return sqrt(x * x + y * y);
}

bool IsPtInRect(cv::Point3d *pt, point2d *prt1, point2d *prt2, point2d *prt3)
{
    const double RECT_EPS = 0.001;

    double	u, v, tx, ty, bx1, bx2, by1, by2;
    double	x0, y0, cos0, sin0;

    if ((pt == NULL) || (prt1 == NULL) || (prt2 == NULL) || (prt3 == NULL))
        return false;

    x0 = Dist2D(prt2->x - prt1->x, prt2->y - prt1->y);

    if (x0 < RECT_EPS)		// Rectangle too small
        return false;

    // Rotate, centered at (x1,y1)
    cos0 = (prt2->x - prt1->x) / x0;
    sin0 = (prt2->y - prt1->y) / x0;

    tx = prt3->x - prt1->x;
    ty = prt3->y - prt1->y;
    u = tx * cos0 + ty * sin0;
    v = tx * -sin0 + ty * cos0;
    if (fabs(u) > RECT_EPS)
    {
        // The two input vectors --- (x2 - x1, y2 - y1) and (x3 - x1, y3 - y1) --- are not orthogonal
        return false;
    }
    y0 = v;

    bx1 = std::min(0.0, x0);
    bx2 = std::max(0.0, x0);
    by1 = std::min(0.0, y0);
    by2 = std::max(0.0, y0);

    tx = pt->x - prt1->x;
    ty = pt->y - prt1->y;
    u = tx * cos0 + ty * sin0;
    v = tx * -sin0 + ty * cos0;

    return (u >= bx1) && (u <= bx2) && (v >= by1) && (v <= by2);
}

//bool SelectBbx(point2d pt)
//{
//    double	dis, mindis = INVALIDDOUBLE;
//    int		id, minid = -1;

//    for (int ng = 0; ng < groups.size(); ng++) {
//        GROUP	*gr = &groups[ng];
//        if (!gr->tracks.size() || !gr->val)
//            continue;
//        for (int nt = 0; nt < gr->tracks.size(); nt++) {
//            TRACK	*tr = &gr->tracks[nt];
//            if (!tr->bbxs.size() || !tr->val)
//                continue;
//            for (int nb = 0; nb < tr->bbxs.size(); nb++) {
//                BBX		*bx = &tr->bbxs[nb];
//                if (!bx->val)
//                    continue;

//                point2d	p[4];
//                GetRectPts(&bx->cp, &bx->hv, gr->bbx.w1, gr->bbx.w2, p, 0);

//                if (IsPtInRect(&pt, &p[0], &p[1], &p[3])) {
//                    select_grpid = ng;
//                    select_trkid = nt;
//                    select_bbxid = nb;
//                    return true;
//                }
//            }
//        }
//    }
//    return false;
//}

//bool DblBbx_Bbx()
//{
//    if (select_grpid < 0 || select_trkid < 0 || select_bbxid < 0 || !groups[select_grpid].tracks[select_trkid].bbxs[select_bbxid].val)
//        return false;
//    BBX	onebbx = groups[select_grpid].tracks[select_trkid].bbxs[select_bbxid];
//    groups[select_grpid].tracks[select_trkid].bbxs.push_back(onebbx);
//    return true;
//}

//bool DblBbx_Trk()
//{
//    if (select_grpid < 0 || select_trkid < 0 || !groups[select_grpid].tracks[select_trkid].val)
//        return false;
//    TRACK	onetrk = groups[select_grpid].tracks[select_trkid];
//    groups[select_grpid].tracks.push_back(onetrk);
//    return true;
//}

//bool DblBbx_Grp()
//{
//    if (select_grpid < 0 || !groups[select_grpid].val)
//        return false;
//    GROUP	onegrp = groups[select_grpid];
//    groups.push_back(onegrp);
//    return true;
//}

//bool DblBbx()
//{
//    if (select_grpid < 0 || !groups[select_grpid].val)
//        return false;

//    //can only double a static object
//    if (groups[select_grpid].isstatic) {
//        //double the group
//        GROUP	onegrp = groups[select_grpid];
//        groups.push_back(onegrp);
//    }

//    return false;
//}

//#define	SIZESTEP	0.1
//#define	MOVESTEP	1.0
//#define	ROTATESTEP	(10.0/180.0*M_PI)

//bool EditBbx_Grp(WPARAM wParam)
//{
//    if (select_grpid < 0 || select_trkid < 0 || select_bbxid < 0)
//        return false;
//    GROUP	*mgrp = &groups[select_grpid];

//    switch (wParam) {
//    case VK_DELETE:
//        mgrp->val = false;
//        select_grpid = select_trkid = select_bbxid = -1;
//        break;
//    case 'v':
//    case 'V':
//        if (GetKeyState(VK_CONTROL) & 0x8000)
//            DblBbx();
//        break;
//    }
//    return true;
//}

//bool EditBbx_Trk(WPARAM wParam)
//{
//    if (select_grpid < 0 || select_trkid < 0 || select_bbxid < 0)
//        return false;
//    GROUP	*mgrp = &groups[select_grpid];
//    TRACK	*mtrk = &groups[select_grpid].tracks[select_trkid];

//    switch (wParam) {
//    case VK_DELETE:
//    {
//        mtrk->val = false;
//        int i;
//        for (i = 0; i < mgrp->tracks.size(); i++) {
//            if (mgrp->tracks[i].val)
//                break;
//        }
//        if (i >= mgrp->tracks.size())
//            mgrp->val = false;
//        select_grpid = select_trkid = select_bbxid = -1;
//        break;
//    }
//    case 'v':
//    case 'V':
//        if (GetKeyState(VK_CONTROL) & 0x8000)
//            DblBbx();
//        break;
//    }
//    return true;
//}

//bool EditBbx_Bbx(WPARAM wParam)
//{
//    if (select_grpid < 0 || select_trkid < 0 || select_bbxid < 0)
//        return false;

//    GROUP	*mgrp = &groups[select_grpid];
//    TRACK	*mtrk = &groups[select_grpid].tracks[select_trkid];
//    BBX	*mbbx = &groups[select_grpid].tracks[select_trkid].bbxs[select_bbxid];
//    bool sign=false;
//    double	scl=10.0;
//    if (GetKeyState(VK_SHIFT) & 0x8000) {
//        sign = true;
//        scl = 1.0;
//    }

//    double ang;
//    switch (wParam) {
//    case VK_LEFT:
//        mbbx->cp.x -= MOVESTEP / scl;
//        break;
//    case VK_RIGHT:
//        mbbx->cp.x += MOVESTEP / scl;
//        break;
//    case VK_UP:
//        mbbx->cp.y += MOVESTEP / scl;
//        break;
//    case VK_DOWN:
//        mbbx->cp.y -= MOVESTEP / scl;
//        break;
//    case VK_HOME:
//    case VK_END:
//        ang = atan2(mbbx->hv.y, mbbx->hv.x);
//        if (wParam == VK_HOME)
//            ang -= ROTATESTEP / scl;
//        else
//            ang += ROTATESTEP / scl;
//        mbbx->hv.y = sin(ang);
//        mbbx->hv.x = cos(ang);
//        break;
//    case 'w':
//    case 'W':
//        if (sign)
//            mgrp->bbx.w1 = max(mgrp->bbx.w1 - SIZESTEP, 0.5);
//        else
//            mgrp->bbx.w1 += SIZESTEP;
//        break;
//    case 'l':
//    case 'L':
//        if (sign)
//            mgrp->bbx.w2 = max(mgrp->bbx.w2 - SIZESTEP, 0.5);
//        else
//            mgrp->bbx.w2 += SIZESTEP;
//        break;
//    case VK_DELETE:
//    {
//        select_grpid = select_trkid = select_bbxid = -1;
//        int i;
//        mbbx->val = false;
//        for (i = 0; i < mtrk->bbxs.size(); i++) {
//            if (mtrk->bbxs[i].val)
//                break;
//        }
//        if (i >= mtrk->bbxs.size())
//            break;
//        mtrk->val = false;
//        for (i = 0; i < mgrp->tracks.size(); i++) {
//            if (mgrp->tracks[i].val)
//                break;
//        }
//        if (i >= mgrp->tracks.size())
//            mgrp->val = false;
//        break;
//    }
//    case 'v':
//    case 'V':
//        if (GetKeyState(VK_CONTROL) & 0x8000)
//            DblBbx();
//        break;
//    }
//    return true;
//}


//bool EditBbx(WPARAM wParam)
//{
//    if (select_grpid < 0 || !groups[select_grpid].val)
//        return false;
//    /*
//    switch (op) {
//    case  _OP_EDIT_GRPBBX:
//        return EditBbx_Grp(wParam);
//    case  _OP_EDIT_TRKBBX:
//        return EditBbx_Trk(wParam);
//    case _OP_EDIT_ONEBBX:
//        return EditBbx_Bbx(wParam);
//    }
//    */
//    switch (op) {
//    case  _OP_EDIT_GRPBBX:
//    case  _OP_EDIT_TRKBBX:
//    case _OP_EDIT_ONEBBX:
//        //can only edit bbx
//        return EditBbx_Bbx(wParam);
//    }
//    return false;
//}
