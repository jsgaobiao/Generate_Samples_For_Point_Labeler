#include "samplegenerator.h"
#include "seglogloader.h"
#include "dsvlprocessor.h"
#include "bbx.h"
#include <QDir>
#include <algorithm>
#include <map>
#include <unordered_set>
#include <fstream>
#include <string>
#include <cmath>

extern std::vector<GROUP> groups;
extern MASTER mapInfo;

extern std::string tag_path;
extern point3d calib_shv, calib_ang;
extern string poseFileName;
extern ofstream fout;

SampleGenerator::SampleGenerator(RMAP *prm_)
{
    prm = prm_;
    dataimg = cv::Mat::zeros(prm->len * 1.0/*4.5*/, prm->wid / 2, CV_8UC3);
}

// 查找是否有Bbox包围了点pnt，返回pnt的标签和instance id
void findBbox(int miliSec, cv::Point3d pnt, int &ptLabel, int &ptInstance)
{
    // 枚举Group，一个物体的多个轨迹Track构成Group
    for (int ng = 0; ng < groups.size(); ng++) {
        GROUP	*gr = &groups[ng];
        if (!gr->tracks.size() || !gr->val) continue;
        // 枚举Track，同一物体不同帧的Bbox构成Track
        for (int nt = 0; nt < gr->tracks.size(); nt++) {
            TRACK	*tr = &gr->tracks[nt];
            if (!tr->bbxs.size() || !tr->val) continue;
            // 枚举Bbox
            for (int nb = 0; nb < tr->bbxs.size(); nb++) {
                BBX	*bx = &tr->bbxs[nb];
                if (!bx->val) continue;

                point2d	p[4];
                GetRectPts(&bx->cp, &bx->hv, gr->bbx.w1, gr->bbx.w2, p, 0);

                if (IsPtInRect(&pnt, &p[0], &p[1], &p[3]) && pnt.z < gr->bbx.h1) {
                    ptLabel = gr->label; ptInstance = tr->prid;
                }
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
//    cv::Mat cshv = (cv::Mat_<double>(4,4)<<0,0,0,calib_shv.x,0,0,0,calib_shv.y,0,0,0,calib_shv.z,0,0,0,0);
    cv::Mat cshv = (cv::Mat_<double>(4,4)<<1,0,0,calib_shv.x,0,1,0,calib_shv.y,0,0,1,calib_shv.z,0,0,0,1);
//    cv::Mat crot = crot_z*crot_y*crot_x + cshv;
    cv::Mat crot = cshv*crot_z*crot_y*crot_x;

    double shv_x = shv_now.x - shv_init.x;
    double shv_y = shv_now.y - shv_init.y;
    double shv_z = shv_now.z - shv_init.z;
    cv::Mat rot_x = (cv::Mat_<double>(4,4)<<1,0,0,0,0,cos(ang_now.x),-sin(ang_now.x),0,0,sin(ang_now.x),cos(ang_now.x),0,0,0,0,1);
    cv::Mat rot_y = (cv::Mat_<double>(4,4)<<cos(ang_now.y),0,sin(ang_now.y),0,0,1,0,0,-sin(ang_now.y),0,cos(ang_now.y),0,0,0,0,1);
    cv::Mat rot_z = (cv::Mat_<double>(4,4)<<cos(ang_now.z),-sin(ang_now.z),0,0,sin(ang_now.z),cos(ang_now.z),0,0,0,0,1,0,0,0,0,1);
//    cv::Mat shv = (cv::Mat_<double>(4,4)<<0,0,0,shv_x,0,0,0,shv_y,0,0,0,shv_z,0,0,0,0);
//    cv::Mat rot = rot_z*rot_y*rot_x + shv;
    cv::Mat shv = (cv::Mat_<double>(4,4)<<1,0,0,shv_x,0,1,0,shv_y,0,0,1,shv_z,0,0,0,1);
    cv::Mat rot = shv*rot_z*rot_y*rot_x;
    cv::Mat trans = rot*crot;
    cv::Matx34d transMat;
    for(int i=0;i<3;i++)
    {
        for(int j=0;j<4;j++) {
            transMat(i, j) = trans.at<double>(i,j);
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
    printf("Total region id number : %d\n", idx_set.size());

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
            int ptInstance = 0;
//            cv::Point3d pnt = transMat * cv::Point3d(prm->pts[y*prm->wid+x].x, prm->pts[y*prm->wid+x].y, prm->pts[y*prm->wid+x].z);
//            pnt.x = (pnt.x - mapInfo.ox);
//            pnt.y = (pnt.y - mapInfo.oy);
//            findBbox(prm->millsec, pnt, ptLabel, ptInstance);

            if (tmpRegId > 0) {
                if (regionIdMapLabel.find(tmpRegId) != regionIdMapLabel.end()) {
                    int l = seglog->colorTable[regionIdMapLabel[tmpRegId]][3];
                    if(l>0 && l<8)
                        l += (tmpRegId<<16);
                    lab_fp.write((char*) &l, sizeof(int));
                }
                else if (tmpRegId < 21){
                    int l = seglog->colorTable[tmpRegId%21][3];
                    lab_fp.write((char*) &l, sizeof(int));
                }
                else {
                    int l = 0;
                    lab_fp.write((char*) &l, sizeof(int));
                }
            }
            else if (tmpRegId == GROUND) {
                int l = 22;
                lab_fp.write((char*) &l, sizeof(int));
            }
            else {
                int l = 0;
                lab_fp.write((char*) &l, sizeof(int));
            }
        }
    }
    pts_fp.close();
    lab_fp.close();
    tag_fp.close();

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
