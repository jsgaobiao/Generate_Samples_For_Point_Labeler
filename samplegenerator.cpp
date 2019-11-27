#include "samplegenerator.h"
#include "seglogloader.h"
#include "dsvlprocessor.h"
#include "bbx.h"
#include <QDir>
#include <algorithm>
#include <map>
#include <unordered_set>
#include <fstream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <cmath>
#include <opencv2/imgproc.hpp>

extern std::vector<GROUP> groups;
extern MASTER mapInfo;

extern std::string tag_path;
extern point3d calib_shv, calib_ang;
extern string poseFileName;
extern ofstream fout;
extern bool flagBbox;
extern bool bbox_overwrite_flag;
extern std::string outputDir;

double pixelSize = 0.1;
double curH = 100;  // m
double curW = 100;  // m
cv::Mat curLidar(int(curH/pixelSize), int(curW/pixelSize), CV_8UC3);
cv::Mat bboxMap(int(curH/pixelSize), int(curW/pixelSize), CV_32FC3);

SampleGenerator::SampleGenerator(RMAP *prm_)
{
    prm = prm_;
    dataimg = cv::Mat::zeros(prm->len * 1.0/*4.5*/, prm->wid / 2, CV_8UC3);
}

void setPixel(cv::Mat &curLidar, cv::Point2d pntPixel, int b, int g, int r)
{
    if (pntPixel.x < 0 || pntPixel.y < 0 || pntPixel.x >= curLidar.rows || pntPixel.y >= curLidar.cols)
        return;
    curLidar.at<cv::Vec3b>(pntPixel.x, pntPixel.y)[0] = b;
    curLidar.at<cv::Vec3b>(pntPixel.x, pntPixel.y)[1] = g;
    curLidar.at<cv::Vec3b>(pntPixel.x, pntPixel.y)[2] = r;
}

// 在局部二维平面画出所有bbox
void drawBbox(int milliSec, cv::Mat &bboxMap, cv::Mat &curLidar, point3d shv_now)
{
    int cntBbox = 0;
    // 枚举Group，一个物体的多个轨迹Track构成Group
    for (int ng = 0; ng < groups.size(); ng++) {
        GROUP	*gr = &groups[ng];
        if (!gr->tracks.size() || !gr->val) continue;
        // 动态物体只绘制同一时间戳的包围框，静态物体始终绘制包围框
        if ((milliSec < gr->smilli || milliSec > gr->emilli) && (!gr->isstatic)) continue;
        // 枚举Track，同一物体不同帧的Bbox构成Track
        for (int nt = 0; nt < gr->tracks.size(); nt++) {
            TRACK	*tr = &gr->tracks[nt];
            if (!tr->bbxs.size() || !tr->val) continue;
            if ((milliSec < tr->smilli || milliSec > tr->emilli) && (!tr->isstatic)) continue;
            // 枚举Bbox
            for (int nb = 0; nb < tr->bbxs.size(); nb++) {
                BBX	*bx = &tr->bbxs[nb];
                if (!bx->val) continue;
                // 动态物体只保留附近几帧的包围框
                if (!tr->isstatic) {
                    if (fabs(milliSec - bx->milli) > 200) continue;
                }
                point2d	p[4];
                GetRectPts(&bx->cp, &bx->hv, gr->bbx.w1, gr->bbx.w2, p, 0);
                cv::Point _points[1][4];
                // 绘制当前的包围框
                for (int i = 0; i < 4; i ++) {
                    p[i].x -= shv_now.x; p[i].y -= shv_now.y;
                    p[i].x /= pixelSize; p[i].y /= pixelSize;
                    p[i].x += bboxMap.rows/2; p[i].y += bboxMap.cols/2;
                    _points[0][i] = Point(p[i].y, p[i].x);
                }
                const Point* ppt[1] = {_points[0]};
                const int num_pnt = 4;
                // bboxMap标记了实心bbox框和bbox的高度\类别标签\Instance标签（一个Group用第一个track的prid作为instance的标签）
                cv::fillPoly(bboxMap, ppt, &num_pnt, 1, cv::Scalar(gr->bbx.h1, gr->label, gr->tracks[0].prid));
                cv::putText(curLidar, std::to_string(gr->tracks[0].prid), cv::Point(p[0].y, p[0].x), cv::FONT_HERSHEY_COMPLEX, 0.3, cv::Scalar(0,0,255));
                // curLidar用于可视化，画的是空心框
                // 静态物体黄色框，动态物体红色框
                if (tr->isstatic)
                    cv::polylines(curLidar, ppt, &num_pnt, 1, 1, cv::Scalar(0,255,255));
                else
                    cv::polylines(curLidar, ppt, &num_pnt, 1, 1, cv::Scalar(0,0,255));
            }
        }
    }
}

void SampleGenerator::GenerateAllSamplesInRangeImage(RMAP *prm_, RMAP *first_prm, SegLogLoader *seglog, cv::VideoWriter &out)
{
    prm = prm_;

    // write pose (error on dsvl now)
    point3d shv_now = prm->shv;
    point3d ang_now = prm->ang;
    ang_now.x = prm->ang.y;
    ang_now.y = prm->ang.x;
    point3d shv_init = first_prm->shv;
    cv::Mat crot_x = (cv::Mat_<double>(4,4)<<1,0,0,0,0,cos(calib_ang.x),-sin(calib_ang.x),0,0,sin(calib_ang.x),cos(calib_ang.x),0,0,0,0,1);
    cv::Mat crot_y = (cv::Mat_<double>(4,4)<<cos(calib_ang.y),0,sin(calib_ang.y),0,0,1,0,0,-sin(calib_ang.y),0,cos(calib_ang.y),0,0,0,0,1);
    cv::Mat crot_z = (cv::Mat_<double>(4,4)<<cos(calib_ang.z),-sin(calib_ang.z),0,0,sin(calib_ang.z),cos(calib_ang.z),0,0,0,0,1,0,0,0,0,1);
    cv::Mat cshv = (cv::Mat_<double>(4,4)<<0,0,0,calib_shv.x,0,0,0,calib_shv.y,0,0,0,calib_shv.z,0,0,0,0);
    cv::Mat crot = crot_z*crot_y*crot_x + cshv;
//    cv::Mat cshv = (cv::Mat_<double>(4,4)<<1,0,0,calib_shv.x,0,1,0,calib_shv.y,0,0,1,calib_shv.z,0,0,0,1);
//    cv::Mat crot = cshv*crot_z*crot_y*crot_x;

    double shv_x = shv_now.x - shv_init.x;
    double shv_y = shv_now.y - shv_init.y;
    double shv_z = shv_now.z - shv_init.z;
    cv::Mat rot_x = (cv::Mat_<double>(4,4)<<1,0,0,0,0,cos(ang_now.x),-sin(ang_now.x),0,0,sin(ang_now.x),cos(ang_now.x),0,0,0,0,1);
    cv::Mat rot_y = (cv::Mat_<double>(4,4)<<cos(ang_now.y),0,sin(ang_now.y),0,0,1,0,0,-sin(ang_now.y),0,cos(ang_now.y),0,0,0,0,1);
    cv::Mat rot_z = (cv::Mat_<double>(4,4)<<cos(ang_now.z),-sin(ang_now.z),0,0,sin(ang_now.z),cos(ang_now.z),0,0,0,0,1,0,0,0,0,1);
    cv::Mat shv = (cv::Mat_<double>(4,4)<<0,0,0,shv_x,0,0,0,shv_y,0,0,0,shv_z,0,0,0,0);
    cv::Mat rot = rot_z*rot_y*rot_x + shv;
//    cv::Mat shv = (cv::Mat_<double>(4,4)<<1,0,0,shv_x,0,1,0,shv_y,0,0,1,shv_z,0,0,0,1);
//    cv::Mat rot = shv*rot_z*rot_y*rot_x;
    cv::Mat trans = rot*crot;
    cv::Matx33d cTransMat;
    cv::Matx33d rTransMat;
    for(int i=0;i<3;i++)
    {
        for(int j=0;j<4;j++) {
            if (j < 3) {
                cTransMat(i, j) = crot.at<double>(i,j);
                rTransMat(i, j) = rot.at<double>(i,j);
            }
            fout << trans.at<double>(i,j) << ' ';
        }
    }
    fout<<'\n';

    QDir dir1(QString(bin_path.c_str()));
    if (!dir1.exists()){
        if (!dir1.mkpath(dir1.absolutePath()))
            return ;
    }
    QDir dir2(QString(label_path.c_str()));
    if (!dir2.exists()){
        if (!dir2.mkpath(dir2.absolutePath()))
            return ;
    }
    QDir dir3(QString(tag_path.c_str()));
    if (!dir3.exists()){
        if (!dir3.mkpath(dir3.absolutePath()))
            return ;
    }
    string filename;
    string str_fno = to_string(prm->fno);
    if(prm->fno < 10) filename = "00000"+str_fno;
    else if(prm->fno < 100) filename = "0000"+str_fno;
    else if(prm->fno < 1000) filename = "000"+str_fno;
    else if(prm->fno < 10000) filename = "00"+str_fno;
    else if(prm->fno < 100000) filename = "0"+str_fno;
    else filename = str_fno;
    ofstream pts_fp(bin_path+filename+".bin",ios::binary);
    ofstream lab_fp(label_path+filename+".label",ios::binary);
    ifstream lab_bak_fp(label_bak_path+filename+".label",ios::binary);
    ofstream tag_fp(tag_path+filename+".tag",ios::binary);

    //determine all region id, prm->regnum is not enough
    // 将所有region id放进set中，用于查找
    std::unordered_set<int> idx_set;
    for (int y = 0; y < prm->len; y++) {
        for (int x = 0; x < prm->wid; x++) {
            if (prm->regionID[y*prm->wid+x] > 0){
                idx_set.insert(prm->regionID[y*prm->wid+x]);
            }
        }
    }
//    printf("Total region id number : %d\n", idx_set.size());

    // outsample : GT输出图像
    cv::Mat outsample(cv::Size(WIDTH, HEIGHT), CV_8UC1, cv::Scalar(0));
    std::map<int, int> regionIdMapLabel;
    std::unordered_set<int>::iterator iter;
    std::multimap<int,int> seedsMap;

    // 在seedMap中seed按照时间戳排序
    for (int i = 0; i < seglog->seednum; i ++) {
        seedsMap.insert(std::pair<int,int>(seglog->seeds[i].fno, i));
    }
    for (iter=idx_set.begin(); iter!=idx_set.end(); iter++){
        int regionid = *iter;
        int count = 0;
        int label = 0;
        typedef std::multimap<int, int>::iterator MITER;
        std::pair<MITER, MITER> range;
        // 寻找时间戳与当前帧相同的所有seeds
        range = seedsMap.equal_range(prm->fno);

        for (MITER i = range.first; i != range.second; i ++) {
            // 确认是当前帧的seed
            if (seglog->seeds[i->second].prid == regionid) {
                label = seglog->seeds[i->second].lab;
                // 标注时，label标记在seed上。所以，要找到regionid对应的seed，并且把标签赋值给regionid
                regionIdMapLabel.insert(std::pair<int, int>(regionid, label));
                count++;
            }
        }
    }

    // 如果有bbox文件用来初始化点云标签
    if (flagBbox) {
        // 可视化当前帧的激光点和包围框
            curLidar.setTo(0);
            bboxMap.setTo(0);

        // 在局部二维平面画出所有bbox
        drawBbox(prm->millsec, bboxMap, curLidar, shv_now);
    }

    for (int y = 0; y <prm->len; y++) {
        for (int x = 0; x < prm->wid; x++) {
            if(!prm->pts[y*prm->wid+x].i)
            {
                char t = 0;
                tag_fp.write(&t, sizeof(char));
                continue;
            }
            char t = 1;
            tag_fp.write(&t, sizeof(char));
            pts_fp.write((char*) &(prm->pts[y*prm->wid+x].x), sizeof(float));
            pts_fp.write((char*) &(prm->pts[y*prm->wid+x].y), sizeof(float));
            pts_fp.write((char*) &(prm->pts[y*prm->wid+x].z), sizeof(float));
            float intensity = (float) prm->pts[y*prm->wid+x].i;
            pts_fp.write((char*) &intensity, sizeof(float));
            int tmpRegId = prm->regionID[y*prm->wid+x];

            // 判断激光点是不是在某个boudingbox里
            int ptLabel = -1;
            int ptInstance = -1;
            // 如果有bbox文件用来初始化点云标签
            if (flagBbox) {
                cv::Point3d pnt = cTransMat * cv::Point3d(prm->pts[y*prm->wid+x].x, prm->pts[y*prm->wid+x].y, prm->pts[y*prm->wid+x].z);
                pnt += cv::Point3d(calib_shv.x, calib_shv.y, calib_shv.z);
                pnt = rTransMat * pnt;
                cv::Point2d pntPixel(int(pnt.x / pixelSize)+curLidar.rows/2, int(pnt.y / pixelSize)+curLidar.cols/2);
                // 查找是否有Bbox包围了点pnt，返回pnt的标签和instance id
                if (pntPixel.x >= 0 && pntPixel.y >= 0 && pntPixel.x < curLidar.rows && pntPixel.y < curLidar.cols) {
                    if (bboxMap.at<cv::Vec3f>(pntPixel.x, pntPixel.y)[0] != 0
                        && bboxMap.at<cv::Vec3f>(pntPixel.x, pntPixel.y)[0]+0.2 >= pnt.z) {
                        // bbox内的激光点
                        setPixel(curLidar, pntPixel, 0, 255, 0);
                        ptLabel = bboxMap.at<cv::Vec3f>(pntPixel.x, pntPixel.y)[1];
                        ptInstance = int(bboxMap.at<cv::Vec3f>(pntPixel.x, pntPixel.y)[2]) % ((1<<17)-1);
                    }
                    else {
                        setPixel(curLidar, pntPixel, 255, 255, 255);
                    }
                }
            }
            if (ptLabel == -1) ptLabel = tmpRegId;


            int l_bak;
            lab_bak_fp.read((char*)&l_bak, sizeof(int));
            l_bak = l_bak & 0xffff;
            if (ptLabel > 0) {
                if (ptLabel < 22){ // 包围框中的点
                    int l = ptLabel;
                    // 找一下激光点是不是已经有标注了
                    if (regionIdMapLabel.find(tmpRegId) != regionIdMapLabel.end()) {
                        l = seglog->colorTable[regionIdMapLabel[tmpRegId]][3];
                        // 如果包围框内的点是非地面点，那么用包围框标签赋值
                        if (l != 22) {
                            l = ptLabel;
                        }
                        else if (l != ptLabel) { // 已经有了地面点标注，那么不归为这个Instance
                            ptInstance = -1;
                        }
                    }
                    if (tmpRegId == GROUND) {l = 22; ptInstance = -1;}
                    // 如果有instance标签(高16位记录instance id，低16位记录类别)
                    if (ptInstance > 0) {l += (ptInstance<<16);}
                    lab_fp.write((char*) &l, sizeof(int));
                }
                else
                if (regionIdMapLabel.find(ptLabel) != regionIdMapLabel.end()) {
                    int l = seglog->colorTable[regionIdMapLabel[ptLabel]][3];
                    // 如果有instance标签(高16位记录instance id，低16位记录类别)
                    if (ptInstance > 0) l += (ptInstance<<16);
                    if (bbox_overwrite_flag)
                        lab_fp.write((char*) &l, sizeof(int));
                    else
                        lab_fp.write((char*) &l_bak, sizeof(int));
                }
                else {
                    int l = 0; // unlabelled
                    if (bbox_overwrite_flag)
                        lab_fp.write((char*) &l, sizeof(int));
                    else
                        lab_fp.write((char*) &l_bak, sizeof(int));
                }
            }
            else if (ptLabel == GROUND) {
                int l = 22;
                if (bbox_overwrite_flag)
                    lab_fp.write((char*) &l, sizeof(int));
                else
                    lab_fp.write((char*) &l_bak, sizeof(int));
            }
            else {
                int l = 0;
                if (bbox_overwrite_flag)
                    lab_fp.write((char*) &l, sizeof(int));
                else
                    lab_fp.write((char*) &l_bak, sizeof(int));
            }
        }
    }
    pts_fp.close();
    lab_bak_fp.close();
    lab_fp.close();
    tag_fp.close();
    // 可视化当前帧的激光和包围框
    cv::imshow("currentFrameLidar", curLidar);

    cv::Mat rangeImg, mergeImg;
    cv::resize(cv::cvarrToMat(prm->rMap), rangeImg, cv::Size(WIDTH, HEIGHT), cv::INTER_NEAREST);
    cv::cvtColor(rangeImg, rangeImg, CV_GRAY2BGR);
    mergeImg = rangeImg.clone();

    double yRatio = double(prm->len) / (double)HEIGHT;
    double xRatio = double(prm->wid) / (double)WIDTH;
    int maxid = 0;
    int minz = 100000;
    int maxz = -minz;

    // Build ground truth image
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int yy = y * yRatio;
            int xx = x * xRatio;
            int tmpRegId = prm->regionID[yy*prm->wid+xx];
            // 高度（z值）的归一化处理    [-3, 7]m
//            int height = (prm->pts[yy*prm->wid+xx].z + 3.0) * 60;
            int height = (prm->pts[yy*prm->wid+xx].z + 3.0) * 25.5;
            rangeImg.at<cv::Vec3b>(y, x)[1] = prm->pts[yy*prm->wid+xx].i;
            rangeImg.at<cv::Vec3b>(y, x)[2] = std::max(0, std::min(height, 255));

             // regId > 0 表示背景的region
            if (tmpRegId > 0) {
                if (tmpRegId > maxid)
                    maxid = tmpRegId;
                // 有标注的region
                if (regionIdMapLabel.find(tmpRegId) != regionIdMapLabel.end()) {
                    int r = seglog->colorTable[regionIdMapLabel[tmpRegId]][0];
                    int g = seglog->colorTable[regionIdMapLabel[tmpRegId]][1];
                    int b = seglog->colorTable[regionIdMapLabel[tmpRegId]][2];
                    int l = seglog->colorTable[regionIdMapLabel[tmpRegId]][3];
                    // GT
                    outsample.at<uchar>(y, x) = l;
                    mergeImg.at<cv::Vec3b>(y, x)[0] = b;
                    mergeImg.at<cv::Vec3b>(y, x)[1] = g;
                    mergeImg.at<cv::Vec3b>(y, x)[2] = r;
                }
                // 前景的region
                else if (tmpRegId < 21){     // 最详细的类别数量为21
                    int r = seglog->colorTable[tmpRegId%21][0];
                    int g = seglog->colorTable[tmpRegId%21][1];
                    int b = seglog->colorTable[tmpRegId%21][2];
                    int l = seglog->colorTable[tmpRegId%21][3];
                    // GT
                    outsample.at<uchar>(y, x) = l;
                    mergeImg.at<cv::Vec3b>(y, x)[0] = b;
                    mergeImg.at<cv::Vec3b>(y, x)[1] = g;
                    mergeImg.at<cv::Vec3b>(y, x)[2] = r;
                }
            }
            else if (tmpRegId == GROUND) {      // 地面的类别标签
                outsample.at<uchar>(y, x) = 22;
                mergeImg.at<cv::Vec3b>(y, x)[0] = 208;
                mergeImg.at<cv::Vec3b>(y, x)[1] = 149;
                mergeImg.at<cv::Vec3b>(y, x)[2] = 117;
            }
            else if (tmpRegId <= 0 && prm->pts[yy*prm->wid+xx].i == 0) {
                outsample.at<uchar>(y, x) = 0;
            }
        }
    }
    // Write to image file
//    std::string _imgname = imgname;
//    QDir dir(QString(_imgname.c_str()));
//    if (!dir.exists()){
//        if (!dir.mkpath(dir.absolutePath()))
//            return ;
//    }
    // 把生成的样本写入图片
//    _imgname += std::to_string(prm->millsec);
    // 要生成数据的话，把注释去掉
//    cv::imwrite(_imgname + "_gt.png", outsample);
//    cv::imwrite(_imgname + "_img.png", rangeImg);
//    cv::imwrite(_imgname + "_merge.png", mergeImg);

    // Write to video
    cv::resize(mergeImg, mergeImg, cv::Size(VIS_WIDTH, VIS_HEIGHT), cv::INTER_NEAREST);
    cv::resize(rangeImg, rangeImg, cv::Size(VIS_WIDTH, VIS_HEIGHT), cv::INTER_NEAREST);
//    cv::putText(mergeImg, std::to_string(prm->millsec), cv::Point(20,10), CV_FONT_BLACK, 0.6, cv::Scalar(0, 0, 255), 2);
    cv::imshow("gt_vis", mergeImg);
    cv::imshow("input_vis", rangeImg);
    cv::waitKey(1);
    out << mergeImg;
    return ;
}

bool SampleGenerator::IsInBbox(LABELLEDPONTS pt, cv::Point3f minBoxPt, cv::Point3f maxBoxPt)
{
    if (pt.loc.x>minBoxPt.x && pt.loc.x<maxBoxPt.x)
        if (pt.loc.y>minBoxPt.y && pt.loc.y<maxBoxPt.y)
            if (pt.loc.z > minBoxPt.z && pt.loc.z < maxBoxPt.z) {
                return 1;
            }
    return 0;
}

void SampleGenerator::setRangeMapPointer(RMAP *value)
{
    prm = value;
}

void SampleGenerator::OnMouse(int event, int x, int y)
{
    if (event == CV_EVENT_LBUTTONDOWN)
    {
        cv::Point p = cv::Point(x,y);
        p.x = x * 2;
        p.y = y / 1.0/*4.5*/;

        if (!prm)
            return;

        int regionid = prm->regionID[p.y*prm->wid + p.x];
        if (regionid == GROUND)
            return;

        IDTYPE rid(regionid, 0);

//        cv::Mat sample_mat;
//        ExtractSampleById(rid, sample_mat);

        printf("time: %d ID: %d point_num: %d\n", prm->millsec, regionid, rid.point_num);
       // cv::imshow("sample", sample_mat);
    }
}
