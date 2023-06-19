#include "videoOperate.h"


#pragma comment(lib, "opencv_world460.lib")

//custom loger
static FileWriter loger_vdooperate("videoOperate.log");

bool getimage_fromvideo(std::string videofilepath, std::string& imagefilepath)
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
    int width = 480,height = 270;
    resize(frame, frame, Size(width, height));

    //����ΪͼƬ�ļ�
    imwrite(imagefilepath, frame);

    return true;
}
