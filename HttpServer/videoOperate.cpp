#include "videoOperate.h"


#pragma comment(lib, "opencv_world460.lib")

//custom loger
static FileWriter loger_vdooperate("videoOperate.log");

bool getimage_fromvideo(std::string videofilepath, std::string& imagefilepath, unsigned int& videowidth, unsigned int& videoheight)
{
    if (videofilepath.empty() || imagefilepath.empty())
    {
        _debug_to(loger_vdooperate, 0, ("[getimage_fromvideo] input videopath/imagepath is empty...\n"));
        return false;
    }

    // ������Ƶ�ļ�
    VideoCapture cap(videofilepath);
    if (!cap.isOpened())
    {
        _debug_to(loger_vdooperate, 0, ("[getimage_fromvideo]Error opening video file...\n"));
        return false;
    }

    // ��ȡ֡����֡��
    int num_frames = cap.get(CAP_PROP_FRAME_COUNT);
    double fps = cap.get(CAP_PROP_FPS);

    // ����֡��
    int frame_number = num_frames / 2;
    cap.set(CAP_PROP_POS_FRAMES, frame_number);

    // ��ȡ��֡��ͼ������
    Mat frame;
    bool success = cap.read(frame);
    if (!success)
    {
        _debug_to(loger_vdooperate, 0, ("[getimage_fromvideo]Error reading frame...\n"));
        return false;
    }

    //ָ��ͼƬ���
    int width = 480, height = 270;
    videowidth = cap.get(CAP_PROP_FRAME_WIDTH);
    videoheight = cap.get(CAP_PROP_FRAME_HEIGHT);
    if (videowidth > videoheight)
    {
        width = 480;
        double dbscale = (double)width / (double)videowidth;
        height = (int)((double)videoheight * dbscale);//�̶����,���Ÿ߶�
      
    }
    else
    {
        height = 480;
        double dbscale = (double)height / (double)videoheight;
        width = (int)((double)videowidth * dbscale);//�̶��߶�,���ſ��
    }
    resize(frame, frame, Size(width, height));

    //����ΪͼƬ�ļ�
    imwrite(imagefilepath, frame);

    return true;
}

bool getmp4_fromAvi(std::string aviFilePath, std::string& mp4FilePath)
{
    if (aviFilePath.empty() || mp4FilePath.empty())
    {
        _debug_to(loger_vdooperate, 0, ("[getmp4_fromAvi] input videopath/imagepath is empty...\n"));
        return false;
    }

    // ��AVI��Ƶ�ļ�
    cv::VideoCapture inputVideo(aviFilePath);
    if (!inputVideo.isOpened()) 
    {
        _debug_to(loger_vdooperate, 0, ("[getmp4_fromAvi]Error opening video file...\n"));
        return false;
    }

    // ����MP4��Ƶ������
    cv::VideoWriter outputVideo(mp4FilePath, cv::VideoWriter::fourcc('X', '2', '6', '4'), inputVideo.get(cv::CAP_PROP_FPS), cv::Size(inputVideo.get(cv::CAP_PROP_FRAME_WIDTH), inputVideo.get(cv::CAP_PROP_FRAME_HEIGHT)));

    // ��֡��ȡAVI��Ƶ��д��MP4��Ƶ
    cv::Mat frame;
    while (inputVideo.read(frame)) 
    {
        outputVideo.write(frame);
    }

    // �ͷ���Ƶ�ļ�
    inputVideo.release();
    outputVideo.release();

    return true;
}
