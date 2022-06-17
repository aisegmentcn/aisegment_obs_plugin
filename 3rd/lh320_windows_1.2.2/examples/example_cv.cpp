#include <opencv2/opencv.hpp>
#include "wx_vseg.h"

static void resizeAndCrop(const cv::Mat& input, int width, int height, cv::Mat& output)
{
    if (input.rows == height && input.cols == width) {
        output = input;
    }
    else {
        int dw, dh;
        float scale = (float)width / (float)input.cols;
        if (scale < (float)height / (float)input.rows) {
            scale = (float)height / (float)input.rows;
            dw = (int)(scale * (float)input.cols);
            dh = height;
        }
        else {
            dw = width;
            dh = (int)(scale * (float)input.rows);
        }

        cv::resize(input, output, cv::Size(dw, dh));
        if (width < dw) {
            int x = (dw - width) / 2;
            output = input(cv::Range::all(), cv::Range(x, x + width));
        }
        else if (height < dh) {
            int y = (dh - height) / 2;
            output = input(cv::Range(y, y + height), cv::Range::all());
        }
    }
    if (!output.isContinuous()) {
        output = output.clone();
    }
}

int main(int argc, char *argv[])
{
    wx_vseg_handle_t handle = nullptr;
    int res;

    // 设置license文件,
    // 第一个参数设置为您的app_key
    if ((res = wx_vseg_lic_set_key_and_file("your_app_key", "my.lic")) != 0) {
        const char* errmsg = wx_vseg_get_last_error_msg();
        printf("set license fail: %d, %s\n", res, errmsg);
        return -1;
    }

    // 创建抠图handle
    res = wx_vseg_handle_create(0, 0, 0, nullptr, &handle);
    if (res != 0) {
        printf("wx_vseg_handle_create fail: %d\n", res);
        return -1;
    }

    // 使用opencv打开摄像头
    cv::VideoCapture cap(0);
    cv::Mat frame, alpha, bg, blend;
    bg = cv::imread("./bkg.jpg");
    cv::setWindowTitle("blend", "press 'q' to exit");
    int key = 0;
    while(key != 'q') {
        // 从摄像头读取一帧
        if (cap.read(frame)) {
            wx_image_t img_frame = {frame.cols,
                                    frame.rows,
                                    WX_IMG_BGR24,
                                    (int)frame.step[0],
                                    frame.data};
            if (alpha.empty()) {
                alpha.create(frame.rows, frame.cols, CV_32FC1);
            }
            wx_image_t img_alpha;
            // img_alpha作为输出参数，只需要分配并设置data变量即可
            img_alpha.data = alpha.data;

            // 对本帧进行抠图，返回alpha
            res = wx_vseg_sync(handle, &img_frame, &img_alpha);
            if (res != 0) {
                printf("wx_vseg_sync fail: %d\n", res);
            }
            else {
                if (blend.empty()) {
                    blend.create(frame.rows, frame.cols, CV_8UC3);
                }
                if (bg.empty()) {
                    bg.create(frame.rows, frame.cols, CV_8UC3);
                    bg = cv::Scalar(30, 150, 30); // 设置为绿色背景
                }
                if (bg.rows != frame.rows || bg.cols != frame.cols) {
                    resizeAndCrop(bg, frame.cols, frame.rows, bg);
                }
                wx_image_t img_blend;
                // img_blend作为输出参数，只需要分配并设置data变量即可
                img_blend.data = blend.data;
                wx_image_t img_bg = {bg.cols,
                                     bg.rows,
                                     WX_IMG_BGR24,
                                     (int)bg.step[0],
                                     bg.data};
                // 图像融合
                res = wx_vseg_blend(&img_frame, &img_bg, &img_alpha, &img_blend);
                if (res != 0) {
                    printf("failed in wx_vseg_blend, res: %d\n", res);
                }
                else {
                    cv::imshow("blend", blend);
                }
            }
        }
        key = cv::waitKey(30);
    }
    return 0;
}
