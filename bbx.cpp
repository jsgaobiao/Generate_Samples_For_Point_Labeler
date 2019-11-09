#include "bbx.h"

//void GetRectPts(point2d *cp, point2d *v1, double olen1, double olen2, point2d *p, double margin);

std::vector<GROUP> groups;
MASTER mapInfo;

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
