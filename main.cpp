#include <iostream>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
#include <QString>
#include <QDir>
#include <QStringList>
#include "dsvlprocessor.h"
#include "seglogloader.h"
#include "bbx.h"

using namespace std;

std::string imgname;
std::string videoName;
std::string videoNameSeg;
std::string bin_path;
std::string label_path;
std::string poseFileName;
std::string outputDir;
std::string tag_path;
ofstream fout;


int HEIGHT;
int WIDTH;
int VIS_HEIGHT;
int VIS_WIDTH;
double FAC_HEIGHT;

point3d calib_shv, calib_ang;
bool flagBbox;

void loadCalibFile(std::string calibFileName)
{
    FILE* fCalib = std::fopen(calibFileName.c_str(), "r");
    fscanf(fCalib, "rot %lf %lf %lf\n", &calib_ang.y, &calib_ang.x, &calib_ang.z);
    fscanf(fCalib, "shv %lf %lf %lf\n", &calib_shv.x, &calib_shv.y, &calib_shv.z);
    calib_ang.x *= M_PI/180.0;
    calib_ang.y *= M_PI/180.0;
    calib_ang.z *= M_PI/180.0;
    std::fclose(fCalib);
}

int main(int argc, char* argv[])
{
    std::printf("[Usage] ./GenerateSamplesForPointLabeler [dsvl] [transproc.log] [calib] [colortable] [output_dir]\n");
    std::printf("For example:\n");
    std::printf("[dsvl](required): 20190331133302_4-seg.dsvl-timefix\n");
    std::printf("[transproc.log](required): 20190331133302_4-transproc.log\n");
    std::printf("[calib](default): P40n.calib\n");
    std::printf("[colortable](default): colortable.txt\n");
    std::printf("[output_dir](default): data_for_point_labeler\n");
    std::printf("[bbox](optional): bbx-all.log\n");

    if (argc < 3) {
        std::fprintf(stderr, "Args not enough !\n");
        return 0;
    }
    // LiDAR data (dsv) and human labeling data (l)
    std::string dsvlfilename(argv[1]);     // "20190331133302_4-seg.dsvl";
    // Segmentation of range images (记录了相邻帧分割块之间的关联信息)
    std::string seglogfilename(argv[2]);   // "20190331133302_4-transproc.log";
    // 如果修改类别数量，需要修改Ground对应的类别编号（samplegenerator.cpp 359行）

    // 激光雷达到GPS标定文件
    std::string calibFileName = "P40n.calib";
    if (argc > 3) calibFileName = argv[3];
    std::string colortablefilename = "colortable.txt";
    if (argc > 4) colortablefilename = argv[4];
    // 存放输出结果的文件夹
    outputDir = "data_for_point_labeler/";
    if (argc > 5) {
        outputDir = argv[5];
        if (outputDir[outputDir.length() - 1] != '/')
            outputDir = outputDir + '/';
    }
    QDir dir1(QString(outputDir.c_str()));
    if (!dir1.exists()){
        if (!dir1.mkpath(dir1.absolutePath())) {
            std::fprintf(stderr, "Making output dir failed!\n");
            return 0;
        }
    }
    // 读入包围框文件，包围框用于初始点云标签和instance信息
    flagBbox = false;
    if (argc > 6) {
        if (LoadBbx(argv[6])) {
            flagBbox = true;
        }
        else {
            std::fprintf(stderr, "Loading bbox file failed!\n");
        }
    }

    // 输出的点云位姿
    poseFileName = outputDir + "poses.txt";
    fout.open(poseFileName.c_str());

    videoName = outputDir + "gt.avi";
    videoNameSeg = outputDir + "seg.avi";
    imgname = outputDir + "images/";
    bin_path = outputDir + "velodyne/";
    label_path = outputDir + "labels/";
    tag_path = outputDir + "tag/";

    // 32线激光雷达，投影到距离图像的尺寸
    HEIGHT = 32;
    WIDTH  = 1080;
    // 用于可视化的尺寸
    VIS_HEIGHT = 144;
    VIS_WIDTH  = 1080;
    FAC_HEIGHT = (double)VIS_HEIGHT / (double)HEIGHT;

    loadCalibFile(calibFileName);

    SegLogLoader segloader;
    segloader.loadSegLog(const_cast<char*>(seglogfilename.c_str()));
    segloader.loadColorTabel(const_cast<char*>(colortablefilename.c_str()));

    DsvlProcessor dsvl(dsvlfilename);
    dsvl.setSeglog(&segloader);
    dsvl.Processing();

    cout << "Over!" << endl;
    fout.close();
    return 0;
}
