#include <fstream>
#include <sstream>
#include <cstring>
#include <string>
#include <cstdio>
#include <string.h>
#include <vector>
#include <map>
#include <cstdlib>
#include "types.h"

typedef struct {
	int				fno;
	long			milli;
	point2d			cp, cornerpt;
	point2d			hv;
	double			var;
	double			w1, w2, h0, h1, rng;
	bool			val;
	int				ptnum;
} BBX;

typedef struct {
	int				prid;
	int				sfn;
	int				efn;
	long			smilli;
	long			emilli;
	bool			isstatic;
	bool			val;
	BBX				bbx;
	double			SVrat;
	int				mptnum;
	std::vector<BBX> bbxs;
} TRACK;

typedef struct {
	BBX				bbx;
	int				sfn;
	int				efn;
	long			smilli;
	long			emilli;
	bool			isstatic;
	int				label;
	bool			val;
	std::vector<TRACK> tracks;
} GROUP;

typedef struct
{
    int width, length;
    double ox,oy,pixelsize;
}MASTER;

bool LoadMaster(char *szFile);
bool LoadBbx(char *szFile);
void GetRectPts(point2d *cp, point2d *v1, double olen1, double olen2, point2d *p, double margin);
bool IsPtInRect(cv::Point3d *pt, point2d *prt1, point2d *prt2, point2d *prt3);
