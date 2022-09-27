#include <obs-module.h>
#include <media-io/video-scaler.h>
#include <opencv2/opencv.hpp>
//#include <opencv2/imgproc.hpp>

#include <numeric>
#include <memory>
#include <exception>
#include <fstream>

#include "plugin-macros.generated.h"
#include "wx_vseg.h"
#include <time.h>
//#include <QObject>
#include <iostream>
#include <stdio.h>
#include <util/platform.h>


#define licfile "obs.lic"
#define keys    "7dfeff9f01e8d86545ee4d495bf5d723"


static wx_vseg_handle_t handle = nullptr;
static cv::Mat frame, alpha, bg,setbgpic, setbgcolor,blend;
static cv::Mat blurt;
static bool setbginit = false;

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

typedef struct wx_seg_filter
{
	// Use the media-io converter to both scale and convert the colorspace
	video_scaler_t* scalerToBGR;
	video_scaler_t* scalerFromBGR;
	bool disseg = false;
	bool blurbg = true;
	bool usesetbg = false;
	char* bgpath;
	bool usesetcolor = false;
	cv::Scalar backgroundColor{ 0, 0, 0 };

	//CriticalSection mutex;
	//vector<Action> actions;

	//inline void QueueAction(Action action){
		//CriticalScope scope(mutex);
		//actions.push_back(action);
		//ReleaseSemaphore(semaphore, 1, nullptr);}

} wx_seg_filter;

static const char *filter_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "AI_remove_bg";
}

static bool chblur=true, chbg = false, chcolor = false;

static bool tracking_props_modified(obs_properties_t* props, obs_property_t* property, obs_data_t* settings)
{
	bool disableseg = obs_data_get_bool(settings, "seg_disen");
	obs_property_t* blurbg = obs_properties_get(props, "blurbgbool");
	obs_property_t* setbg = obs_properties_get(props, "setbg");
	obs_property_t* bgpic = obs_properties_get(props, "bgpic");

	//obs_property_t* act = obs_properties_get(props, "activate");
	//obs_property_t* act2 = obs_properties_get(props, "activate2");

	if (disableseg == true)
	{
		//obs_property_set_enabled(blurbg, false);
		//obs_property_set_visible(act, false);
		//obs_property_set_visible(act2, false);
		//obs_data_set_bool(settings, "blurbgbool", false);
		//obs_property_set_visible(blurbg, false);
		//obs_property_set_visible(setbg, false);
		//obs_property_set_visible(bgpic, false);
		chblur   = obs_data_get_bool(settings, "blurbgbool");
		chbg = obs_data_get_bool(settings, "setbg");
		chcolor = obs_data_get_bool(settings, "setcolor");

		obs_data_set_bool(settings, "blurbgbool", false);
		obs_data_set_bool(settings, "setbg", false);
		obs_data_set_bool(settings, "setcolor", false);
	}
	else
	{
		//obs_property_set_enabled(blurbg, true);
		//obs_property_set_visible(blurbg, true);
		//obs_property_set_visible(setbg, true);
		//obs_property_set_visible(bgpic, true);

		obs_data_set_bool(settings, "blurbgbool", chblur);
		obs_data_set_bool(settings, "setbg", chbg);
		obs_data_set_bool(settings, "setcolor", chcolor);
	}

	return true;
}


static bool tracking_blur_modified(obs_properties_t* props, obs_property_t* property, obs_data_t* settings)
{
	bool blurbg = obs_data_get_bool(settings, "blurbgbool");
	//obs_property_t* setbg = obs_properties_get(props, "setbg");

	if (blurbg == true)
	{
		//obs_property_set_enabled(setbg, false);
		//obs_data_set_default_bool(settings, "setbg", false);
		bool disseg = obs_data_get_bool(settings, "seg_disen");
		if(disseg == true)
			obs_data_set_bool(settings, "seg_disen", false);

		obs_data_set_bool(settings, "setbg", false);
		obs_data_set_bool(settings, "setcolor", false);
		
	}
	else
	{
		//obs_property_set_enabled(setbg, true);
		//obs_data_set_default_bool(settings, "setbg", true);
		obs_data_set_bool(settings, "setbg", true);
		obs_data_set_bool(settings, "setcolor", false);


	}

	return true;
}
static bool tracking_setbg_modified(obs_properties_t* props, obs_property_t* property, obs_data_t* settings)
{
	bool setbg = obs_data_get_bool(settings, "setbg");
	//obs_property_t* blurbg = obs_properties_get(props, "blurbgbool");
	//obs_property_t* setcolor = obs_properties_get(props, "setcolor");

	if (setbg == true)
	{
		bool disseg = obs_data_get_bool(settings, "seg_disen");
		if (disseg == true)
			obs_data_set_bool(settings, "seg_disen", false);
		//obs_property_set_enabled(setbg, false);
		//obs_data_set_default_bool(settings, "blurbgbool", false);
		obs_data_set_bool(settings, "blurbgbool", false);
		obs_data_set_bool(settings, "setcolor", false);

	}
	else
	{
		//obs_property_set_enabled(setbg, true);
		//obs_data_set_default_bool(settings, "blurbgbool", true);
		obs_data_set_bool(settings, "blurbgbool",true);
		obs_data_set_bool(settings, "setcolor", false);

	}
	setbginit = false;

	return true;
}

static bool tracking_secolor_modified(obs_properties_t* props, obs_property_t* property, obs_data_t* settings)
{
	bool setcolor = obs_data_get_bool(settings, "setcolor");
	//obs_property_t* blurbg = obs_properties_get(props, "blurbgbool");
	//obs_property_t* setbg = obs_properties_get(props, "setbg");

	if (setcolor == true)
	{

		//obs_property_set_enabled(setbg, false);
		//obs_data_set_default_bool(settings, "blurbgbool", false);
		bool disseg = obs_data_get_bool(settings, "seg_disen");
		if (disseg == true)
			obs_data_set_bool(settings, "seg_disen", false);
		obs_data_set_bool(settings, "blurbgbool", false);
		obs_data_set_bool(settings, "setbg", false);

	}
	else
	{
		//obs_property_set_enabled(setbg, true);
		//obs_data_set_default_bool(settings, "blurbgbool", true);
		obs_data_set_bool(settings, "blurbgbool", true);
		obs_data_set_bool(settings, "setbg", false);

	}
	setbginit = false;

	return true;
}



static bool ActivateClicked(obs_properties_t* props, obs_property_t* p,void* data)
{
	struct wx_seg_filter* wsf = reinterpret_cast<wx_seg_filter*>(data);
	//gstreamer_filter_update(data, ((data_t*)data)->settings);
	if (1) {
		//SetActive(false);
		obs_property_set_description(p, "");
	}
	else {
		//SetActive(true);
		obs_property_set_description(p, "");
	}

	return false;
}

 
static obs_properties_t *filter_properties(void *data)
{

	obs_properties_t* props = obs_properties_create();
	obs_property_t* enableseg = obs_properties_add_bool(props, "seg_disen", obs_module_text("EnableSeg"));
	//取值 放在update
	obs_property_set_modified_callback(enableseg, tracking_props_modified);
	//obs_property_set_enabled(enableseg, false);


	//obs_properties_add_button2(props, "activate", "ac", ActivateClicked,data);
	//obs_properties_add_button2(props, "activate2", "22sssssssssssssssssend", ActivateClicked, data);

	/*
	obs_properties_t* track_matte_group = obs_properties_create();
	obs_property_t* p = obs_properties_add_group(
		props, "track_matte_enabled",
		obs_module_text("TrackMatteEnabled"),
		OBS_GROUP_CHECKABLE, track_matte_group);
	*/


	obs_property_t* blurraw   = obs_properties_add_bool(props, "blurbgbool",      obs_module_text("bgblur"));
	obs_property_set_modified_callback(blurraw, tracking_blur_modified);

	obs_property_t* setbg = obs_properties_add_bool(props, "setbg", obs_module_text("setbg"));
	obs_property_set_modified_callback(setbg, tracking_setbg_modified);

	static const char* image_filter =
		"All formats (*.bmp *.tga *.png *.jpeg *.jpg *.gif *.psd *.webp);;"
		"BMP Files (*.bmp);;"
		"Targa Files (*.tga);;"
		"PNG Files (*.png);;"
		"JPEG Files (*.jpeg *.jpg);;"
		"GIF Files (*.gif);;"
		"PSD Files (*.psd);;"
		"WebP Files (*.webp);;"
		"All Files (*.*)";


	obs_property_t* bgpic =obs_properties_add_path(props, "bgpic", nullptr,///obs_module_text("choosebg")
		OBS_PATH_FILE, image_filter, obs_get_module_data_path(obs_current_module()) );
	//obs_property_set_modified_callback(bgpic, tracking_props_modified);

	obs_property_t* setcolor = obs_properties_add_bool(props, "setcolor", obs_module_text("setcolor"));
	obs_property_set_modified_callback(setcolor, tracking_secolor_modified);

	obs_property_t* p_color = obs_properties_add_color(props, "bgColor", nullptr);//,// obs_module_text("Background Color"));

	//obs_properties_add_text(props, "contact", obs_module_text("contact"), OBS_TEXT_DEFAULT);
	obs_properties_add_button2(props, "", obs_module_text("contact"), ActivateClicked,data);


	wx_seg_filter* wsf = reinterpret_cast<wx_seg_filter*>(data);

	UNUSED_PARAMETER(data);
	return props;
}

static void filter_defaults(obs_data_t *settings)
{
	//暂时没有属性页
	obs_data_set_default_bool(settings, "seg_disen", false);
	obs_data_set_default_bool(settings, "blurbgbool", true);
	obs_data_set_default_bool(settings, "setbg", false);
	obs_data_set_default_string(settings, "bgpic", obs_module_file("bkg.jpg"));
	obs_data_set_default_bool(settings, "setcolor", false);
	obs_data_set_default_int(settings, "bgColor", 0x000000);
}



//没有保存
static void filter_update(void *data, obs_data_t *settings)
{
	struct wx_seg_filter* wsf = reinterpret_cast<wx_seg_filter*>(data);

	wsf->disseg = obs_data_get_bool(settings, "seg_disen");
	wsf->blurbg = obs_data_get_bool(settings, "blurbgbool");
	wsf->usesetbg = obs_data_get_bool(settings, "setbg");
	wsf->bgpath = (char*)obs_data_get_string(settings, "bgpic");
	wsf->usesetcolor = obs_data_get_bool(settings, "setcolor");
	uint64_t color = obs_data_get_int(settings, "bgColor");
	wsf->backgroundColor.val[0] = (double)((color >> 16) & 0x0000ff);
	wsf->backgroundColor.val[1] = (double)((color >> 8) & 0x0000ff);
	wsf->backgroundColor.val[2] = (double)(color & 0x0000ff);
	setbginit = false;
}


static void destroyScalers(struct wx_seg_filter* wsf)
{
	blog(LOG_INFO, "Destroy scalers.");
	if (wsf->scalerToBGR != nullptr) {
		video_scaler_destroy(wsf->scalerToBGR);
		wsf->scalerToBGR = nullptr;
	}
	if (wsf->scalerFromBGR != nullptr) {
		video_scaler_destroy(wsf->scalerFromBGR);
		wsf->scalerFromBGR = nullptr;
	}
}
static void * filter_create(obs_data_t *settings, obs_source_t *source)
{
	struct wx_seg_filter* wsf =reinterpret_cast<wx_seg_filter*>(bzalloc(sizeof(struct wx_seg_filter)));
	setbginit = false;
	int res;



	if (handle) {
		wx_vseg_handle_release(handle);
		handle = nullptr;
	}

	// 设置license文件,
	// 第一个参数设置为您的app_key
	char* licFilepath_rawPtr = obs_module_file(licfile);
	if ((res = wx_vseg_lic_set_key_and_file(keys, licFilepath_rawPtr)) != 0) {
		const char* errmsg = wx_vseg_get_last_error_msg();
		printf("set license fail: %d, %s\n", res, errmsg);
		return nullptr;
	}

	// 创建抠图handle
	res = wx_vseg_handle_create(0, 0, 0, nullptr, &handle);
	if (res != 0) {
		printf("wx_vseg_handle_create fail: %d\n", res);
		return nullptr;
	}
	//虽然还未create	 但是必须这call  update ，否则影响控件状态
	filter_update(wsf, settings);
	return wsf;
}

static void initializeScalers(cv::Size frameSize, enum video_format frameFormat,struct wx_seg_filter* wsf)
{
	struct video_scale_info dst {VIDEO_FORMAT_BGR3, (uint32_t)frameSize.width,(uint32_t)frameSize.height, VIDEO_RANGE_DEFAULT,VIDEO_CS_DEFAULT};
	struct video_scale_info src {frameFormat, (uint32_t)frameSize.width,(uint32_t)frameSize.height, VIDEO_RANGE_DEFAULT,VIDEO_CS_DEFAULT};
	// Check if scalers already defined and release them
	destroyScalers(wsf);
	blog(LOG_INFO, "Initialize scalers. Size %d x %d", frameSize.width,frameSize.height);
	// Create new scalers
	video_scaler_create(&wsf->scalerToBGR, &dst, &src, VIDEO_SCALE_DEFAULT);
	video_scaler_create(&wsf->scalerFromBGR, &src, &dst,VIDEO_SCALE_DEFAULT);
}

static cv::Mat convertFrameToBGR(struct obs_source_frame *frame,struct wx_seg_filter* wsf)
{
	const cv::Size frameSize(frame->width, frame->height);

	if (wsf->scalerToBGR == nullptr) {
		// Lazy initialize the frame scale & color converter
		initializeScalers(frameSize, frame->format, wsf);
	}

	cv::Mat imageBGR(frameSize, CV_8UC3);
	const uint32_t bgrLinesize =(uint32_t)(imageBGR.cols * imageBGR.elemSize());
	video_scaler_scale(wsf->scalerToBGR, &(imageBGR.data), &(bgrLinesize), frame->data, frame->linesize);

	return imageBGR;
}

static void convertBGRToFrame(const cv::Mat &imageBGR,struct obs_source_frame *frame,struct wx_seg_filter* wsf)
{
	if (wsf->scalerFromBGR == nullptr) {
		// Lazy initialize the frame scale & color converter
		initializeScalers(cv::Size(frame->width, frame->height),frame->format, wsf);
	}

	const uint32_t rgbLinesize =(uint32_t)(imageBGR.cols * imageBGR.elemSize());
	video_scaler_scale(wsf->scalerFromBGR, frame->data, frame->linesize,   &(imageBGR.data), &(rgbLinesize));
}
void blurbg(cv::Mat& bg, cv::Mat& blurbg){
	cv::Mat rsbg,rsbgblur;
	int width = bg.cols;
	int height = bg.rows;

	cv::resize(bg, rsbg, cv::Size(80,80), 0, 0, cv::INTER_LINEAR);

	//int ksize = 15;
	//if (width < height) {
	//	if (width < ksize)
	//		ksize = width;
	//	if (ksize % 2 == 0)
	//		ksize = ksize - 1;
	//}
	//else
	//{
	//	if (height < ksize)
	//		ksize = height;
	//	if (ksize % 2 == 0)
	//		ksize = ksize - 1;
	//}
	cv::GaussianBlur(rsbg, rsbgblur, cv::Size(15, 15), 0, 0, 4);
	cv::resize(rsbgblur, blurbg, cv::Size(width,height), 0, 0, cv::INTER_LINEAR);

}
static struct obs_source_frame *filter_render(void *data,struct obs_source_frame *frame)
{
	struct wx_seg_filter* wsf =reinterpret_cast<wx_seg_filter*>(data);
	if (wsf->disseg == true)
		return frame;

	// Convert to BGR
	cv::Mat imageBGR = convertFrameToBGR(frame, wsf);

	wx_image_t img_frame = { imageBGR.cols, imageBGR.rows,WX_IMG_BGR24,(int)imageBGR.step[0],imageBGR.data };
	if (alpha.empty()) {
		alpha.create(imageBGR.rows, imageBGR.cols, CV_32FC1);
	}

	wx_image_t img_alpha;
	// img_alpha作为输出参数，只需要分配并设置data变量即可
	img_alpha.data = alpha.data;

	// 对本帧进行抠图，返回alpha
	int res = wx_vseg_sync(handle, &img_frame, &img_alpha);
	if (res != 0) {
		printf("wx_vseg_sync fail: %d\n", res);
	}
	else {
		if (blend.empty())
			blend.create(imageBGR.rows, imageBGR.cols, CV_8UC3);


		if (wsf->blurbg == true)
		{
			//clock_t t0 = clock();
			//cv::blur(imageBGR, bg, cv::Size(20, 20), cv::Point(-1, -1), 4);
			//cv::GaussianBlur(imageBGR, bg, cv::Size(15, 15), 15, 0, 4);

			cv::resize(imageBGR, bg, cv::Size(), 0.1, 0.1, cv::INTER_LINEAR);

			//cv::blur(bg, blurt, cv::Size(20, 20), cv::Point(-1, -1), 4);
			//cv::blur(bg, blurt, cv::Size(20, 20), cv::Point(-1, -1), 4);
			int ksize = 15;
			int width = bg.cols;
			int height = bg.rows;
			if (width < height) {
				if (width < ksize)
					ksize = width;
				if (ksize % 2 == 0)
					ksize = ksize - 1;
			}
			else
			{
				if (height < ksize)
					ksize = height;
				if (ksize % 2 == 0)
					ksize = ksize - 1;
			}
			cv::GaussianBlur(bg, blurt, cv::Size(15, 15), 0, 0, 4);

			cv::resize(blurt, bg, cv::Size(), 10, 10, cv::INTER_LINEAR);

			//clock_t t1 = clock();
			//double tpost = ((double)(t1 - t0) / CLOCKS_PER_SEC) * 1000;
			//printf("tpost = %f \n", tpost);
		}



		if (setbginit == false)
		{
			//#ifdef   _WIN32
			//std::string mFilepath_s(path);
			//bfree(mFilepath_s);

			//std::wstring mFilepath_ws(mFilepath_s.size(), L' ');
			//std::copy(mFilepath_s.begin(), mFilepath_s.end(), mFilepath_ws.begin());

			//QString strPath;
			//std::string path = strPath.toLocal8Bit().toStdString(); //关键是这个
			
			//std::string path_s(wsf->bgpath);
			//std::wstring path_ws(path_s.size(), L' ');
			//std::copy(path_s.begin(), path_s.end(), path_ws.begin());

			//FILE* pFile = _wfopen(path_ws.c_str(), L"rb");
			//setlocale(LC_ALL, "Chinese-simplified");//设置中文环境
			FILE* pFile = os_fopen(wsf->bgpath,"rb"); //fopen
			fseek(pFile, 0, SEEK_END);
			long lSize = ftell(pFile);
			rewind(pFile);
			char* pData = new char[lSize];
			fread(pData, sizeof(char), lSize, pFile);
			fclose(pFile);

			std::vector<char> cvdata(pData, pData + lSize);
			//std::vector<uchar> cvdata; 
			//for (int i = 0; i < lSize; ++i) 
			//	cvdata.push_back(pData[i]);

			setbgpic = cv::imdecode(cvdata, cv::IMREAD_COLOR);
			
			//setbgpic = cv::imread(wsf->bgpath);

			if (setbgpic.empty())
			{
				setbgpic.create(imageBGR.rows, imageBGR.cols, CV_8UC3);
				setbgpic = cv::Scalar(30, 150, 30); // 设置为绿色背景
			}
			if (setbgpic.rows != imageBGR.rows || setbgpic.cols != imageBGR.cols)
				resizeAndCrop(setbgpic, imageBGR.cols, imageBGR.rows, setbgpic);

			setbginit = true;


			setbgcolor.create(imageBGR.rows, imageBGR.cols, CV_8UC3);
			setbgcolor = wsf->backgroundColor;
		}
		



		wx_image_t img_blend;
		// img_blend作为输出参数，只需要分配并设置data变量即可
		img_blend.data = blend.data;
		wx_image_t img_bg;
		if(wsf->blurbg)
			img_bg = { bg.cols,bg.rows, WX_IMG_BGR24,(int)bg.step[0],bg.data };
		else if(wsf->usesetbg)
			img_bg = { setbgpic.cols,bg.rows, WX_IMG_BGR24,(int)setbgpic.step[0],setbgpic.data };
		else
			img_bg = { setbgcolor.cols,bg.rows, WX_IMG_BGR24,(int)setbgcolor.step[0],setbgcolor.data };


		// 图像融合
		res = wx_vseg_blend(&img_frame, &img_bg, &img_alpha, &img_blend);
		if (res != 0) {
			printf("failed in wx_vseg_blend, res: %d\n", res);
		}

		//cv::imshow("blend", blend);
		//Put masked image back on frame,


		convertBGRToFrame(blend, frame, wsf);
	}

	return frame;
}

static void filter_destroy(void *data)
{
	struct wx_seg_filter* wsf =	reinterpret_cast<wx_seg_filter*>(data);

	if (wsf) {
		destroyScalers(wsf);
		bfree(wsf);
	}
	if (!alpha.empty())
		alpha.release();
	if (!blend.empty())
		blend.release();
	if (handle) {
		wx_vseg_handle_release(handle);
		handle = nullptr;
	}
}

struct obs_source_info wx_seg_filter_info = {
	.id = "AI_remove_bg",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_ASYNC,
	.get_name = filter_getname,
	.create = filter_create,
	.destroy = filter_destroy,
	.get_defaults = filter_defaults,
	.get_properties = filter_properties,
	.update = filter_update,
	.filter_video = filter_render,
};
