#ifndef _WX_VSEG_H_
#define _WX_VSEG_H_

#include <stddef.h>
#include <time.h>
#include <stdint.h>

#if WIN32
#ifdef WX_VSEG_STATIC
#define WX_API_
#else
#ifdef WX_EXPORT
#define WX_API_ __declspec(dllexport)
#else
#define WX_API_ __declspec(dllimport)
#endif
#endif
#else
#ifdef WX_EXPORT
#define WX_API_ __attribute__((visibility("default")))
#else
#define WX_API_
#endif
#endif

#ifdef __cplusplus
#define WX_API extern "C" WX_API_
#else
#define WX_API WX_API_
#endif

/**********************************************************
 ** SDK结构定义
 **********************************************************/

typedef enum wx_img_format_e {
    WX_IMG_UNK = 0,
    WX_IMG_RGB24,
    WX_IMG_BGR24,
    WX_IMG_RGBA,
    WX_IMG_BGRA,
    WX_IMG_F32
} wx_img_format_e;

typedef struct wx_image_t {
    int width;
    int height;
    wx_img_format_e format;
    /**
     * 行宽（单位为：bytes），
     * 填0的情况下，为width * element_size，
     * 对于RGB24/BGR24/RGBA/BGRA/F32等格式的的图片
     * 比如，100x200的RGB24的图片，line_size为300,
     * 100x200的RGBA的图片，line_size为400
     */
    int line_size;
    void *data;
} wx_image_t;

#ifdef ANDROID
#include <jni.h>
WX_API int wx_vseg_android_init(JNIEnv *jni_env, jobject application_context);
#endif

/**
 * @brief 获取版本
 * @return SDK 版本信息
 */
WX_API const char* wx_vseg_get_version();

/**********************************************************
 ** SDK授权API
 **********************************************************/

/**
 * @brief 通过文件方式，为SDK设置license，
 * @note 调用其他api前，必须先调用wx_vseg_lic_set_file或者wx_vseg_lic_set_data两者之一,
 *        license文件可以使用auth_tools下载或者向顺势兄弟科技有限公司索要，
 *        一个进程只需要调用一次，不需要反复调用
 * @param[in] appkey appkey
 * @param[in] license_path license文件路径，
 * @return：
 * @deprecated: 请使用wx_vseg_lic_set_key_and_file
 *  成功 - 0
 *  失败 - 错误码
 */
WX_API int wx_vseg_lic_set_key_and_file(const char *appkey, const char *license_path);

/**
 * @deprecated: 请使用wx_vseg_lic_set_key_and_file
 */
WX_API int wx_vseg_lic_set_file(const char *license_path);

/**
 * @brief 通过内存的方式，为SDK设置license，
 * @note 调用其他api前，必须先调用wx_vseg_lic_set_file或者wx_vseg_lic_set_data两者之一,
 *        license文件可以使用auth_tools下载或者向顺势兄弟科技有限公司索要，
 *        一个进程只需要调用一次，不需要反复调用
 * @param[in] appkey appkey
 * @param[in] buffer license数据，
 * @param[in] size buffer参数长度，
 * @return：
 *      成功 - 0
 *      失败 - 错误码
 * @deprecated: 请使用wx_vseg_lic_set_key_and_data
 */
WX_API int wx_vseg_lic_set_key_and_data(const char *appkey, void *buffer, size_t size);

/**
 * @deprecated: 请使用wx_vseg_lic_set_key_and_data
 */
WX_API int wx_vseg_lic_set_data(void *buffer, size_t size);



/**
 * @brief 获取当前license文件的有效期起始时间戳，单位为秒
 * @note 需要在wx_vseg_lic_set_file或者wx_vseg_lic_set_data函数调用后使用，
 * @param[in] valid_from license有效期起始时间戳
 * @return：
 *      成功 - 0
 *      失败 - 错误码
 */
WX_API int wx_vseg_lic_valid_from(time_t *valid_from);

/**
 * @brief 获取当前license文件的有效期过期时间戳，单位为秒
 * @note 需要在wx_vseg_lic_set_file或者wx_vseg_lic_set_data函数调用后使用，
 * @param[in] expired_at license有效期过期时间戳
 * @return：
 *      成功 - 0
 *      失败 - 错误码
 */
WX_API int wx_vseg_lic_expired_at(time_t *expired_at);

/**
 * @brief 获取当前license文件的licenseId
 * @note 需要在wx_vseg_lic_set_file或者wx_vseg_lic_set_data函数调用后使用，
 * @return：
 *      成功 - 当前设置的license的id
 *      失败 - NULL
 */
WX_API const char * wx_vseg_lic_license_id();

/**
 * @brief 获取当前设备的device_id
 * @note
 * @return：
 *      成功 - device_id
 *      失败 - NULL
 */
WX_API const char * wx_vseg_lic_device_id();


/**********************************************************
 ** 图像分割API
 **********************************************************/

typedef struct wx_vseg_handle_t_ wx_vseg_handle_t_;
typedef wx_vseg_handle_t_* wx_vseg_handle_t;

/**
 * @brief 创建抠图操作句柄
 * @note 该句柄在后续函数调用中作为输入，需要调用 wx_vseg_handle_release() 函数来释放
 *       如果一开始不确定图像尺寸，可以输入width和height可以输入0，
 *       当输入0时，处理图像的宽高由第一次调用wx_vseg_push的输入frame的尺寸决定，
 *       一旦图像的尺寸确定，后续的wx_vseg_push输入的图像，尺寸必须保持一致，不能修改
 * @param[in] width 目标图像的宽度，输入0时，由第一次wx_vseg_push调用指定宽高
 * @param[in] height 目标图像的高度，输入0时，由第一次wx_vseg_push调用指定宽高
 * @param[in] async_mode 是否是异步模式，false - 同步模式， true - 异步模式
 * @param[in] config 保留参数，传入NULL
 * @param[out] handle 处理句柄
 * @return 0：成功，其他：失败
 */
WX_API int wx_vseg_handle_create(int width,  /* in */
                                 int height,  /* in */
                                 int async_mode, /* in */
                                 const char* config,  /* in */
                                 wx_vseg_handle_t *handle  /* out */);

/**
 * @brief 同步模式下对某一帧进行抠图处理
 * @note 本函数的handle，必须为以同步模式创建（函数wx_vseg_handle_create的async_mode参数为false）
 *       抠图结果是随着函数直接返回的，只有付费版可调用本函数，免费版调用会报错。
 * @param[in] handle 句柄
 * @param[in] frame 帧的原图，连续的图像输入，图像尺寸必须保持一致
 * @param[in/out] alpha 抠图结果，WX_IMG_F32格式，[0-1.0]之间的二维矩阵，宽高和输入的frame一致，
 *                alpha.data的存储空间需要调用者分配，并且保证最小空间为frame.width * frame.height * 4
 * @return 0：成功，其他：失败
 */
WX_API int wx_vseg_sync(wx_vseg_handle_t handle, /* in */
                        const wx_image_t *frame, /* in */
                        wx_image_t *alpha /* in/out */);


/**
 * @brief 异步模式下，传入一帧待抠图的帧
 * @note 本函数的handle，必须为以异步模式创建（函数wx_vseg_handle_create的async_mode参数为true）
 *       抠图结果通过函数wx_vseg_async_pull()获取，
 *       如果系统负载过高，可能会导致丢帧的情况，
 *       是否丢帧可以pts来对应，
 *       只有付费版可调用本函数，免费版调用会报错。
 * @param[in] handle 句柄
 * @param[in] frame 帧的原图，连续的图像输入，图像尺寸必须保持一致
 * @param[in] pts pts，用来标识某一帧，wx_vseg_async_pull会返回
 * @return 0：成功，其他：失败
 */
WX_API int wx_vseg_async_push(wx_vseg_handle_t handle, /* in */
                              const wx_image_t *frame, /* in */
                              uint64_t pts /* in */);

/**
 * @brief 获取之前某一帧的抠图结果，
 * @note 本函数的handle，必须为以异步模式创建（函数wx_vseg_handle_create的async_mode参数为true）
 *       由于采用异步模式，所以本函数返回的结果，并不是最后一次wx_vseg_push的帧的结果，
 *       有可能是数帧之前的某帧的结果，
 *       另外，如果系统负载过高，可能会导致丢帧的情况，
 *       是否丢帧可以pts来对应，
 *       只有付费版可调用本函数，免费版调用会报错。
 * @param[in] handle 句柄
 * @param[in] wait_in_microsec 等待时间（单位微秒：1/1000000秒），
 *            如果有已经处理完成的帧，本函数会立即返回，否则将会等待wait_in_microsec指定的时间
 *            == 0 : 为不等待，即使没有结果，也立即返回1；
 *             < 0 : 一直等待，直到有可返回的结果，或者抠图结束；
 *             > 0 : 等待指定的时间，如果超时后，返回1
 * @param[in/out] alpha 返回的抠图结果，WX_IMG_F32格式，0-1.0之间的浮点数矩阵，
 *                   alpha.data的存储空间需要调用者分配，并且保证最小空间为frame.width * frame.height * 4
 * @param[out] pts alpha对应的输入帧的pts（wx_vseg_async_push函数的输入）
 * @return 0 - 成功获取一帧；
 *         1 - 暂时没有结果帧可以返回，等待后再次获取；
 *         2 - 抠图结束，wx_vseg_handle_release被调用；
 *        -1 - 出错
 */
WX_API int wx_vseg_async_pull(wx_vseg_handle_t handle, /* in */
                              int64_t wait_in_microsec, /* in */
                              wx_image_t *alpha, /* in/out */
                              uint64_t *pts /* out */);

/**
 * @brief 释放生成的句柄
 * @param[in] handle 输入背景图像
 */
WX_API void wx_vseg_handle_release(wx_vseg_handle_t handle);

/**
 * @brief 根据传入的前景图、背景图图、alpha，做图像融合
 * 必须保证前景图，背景图和alpha图尺寸相同，
 * @param[in] foreground 前景图，格式必须为RGB24/BGR24
 * @param[in] background 背景图像，格式必须为RGB24/BGR24
 * @param[in] alpha 输入alpha图，WX_IMG_F32格式，值为[0 - 1.0]之间
 * @param[in/out] blend 融合后的图像,
 *                   blend.data的存储空间需要调用者分配，并且保证最小空间为foreground.width * foreground.height * 3
 * @return 0：成功，其他：失败
 */
WX_API int wx_vseg_blend(const wx_image_t *foreground,  /* in */
                         const wx_image_t *background,  /* in */
                         const wx_image_t *alpha,  /* in */
                         wx_image_t *blend /* in/out */);

/**
 * @brief 同步模式下对某一帧进行抠图并换背景操作，并输出新背景的合成图
 * @note 本函数的handle，必须为以同步模式创建（函数wx_vseg_handle_create的async_mode参数为false），
 *       必须保证frame，background和blend图尺寸相同，
 *       结果是随着函数直接返回的，
 *       免费版和付费版均可调用本函数，
 *       但是对于windows免费版和iOS/Android非实时免费版，只能输出低于25万像素的合成图。
 * @param[in] handle 句柄
 * @param[in] frame 帧的原图，格式必须为RGB24/BGR24，连续的图像输入，图像尺寸必须保持一致
 * @param[in] background 背景图像，格式必须为RGB24/BGR24
 * @param[in/out] blend 融合后的图像,
 *                blend.data的存储空间需要调用者分配，并且保证最小空间为frame.width * frame.height * 3
 * @return 0：成功，其他：失败
 */
WX_API int wx_vseg_change_background_sync(wx_vseg_handle_t handle, /* in */
                                          const wx_image_t *frame, /* in */
                                          const wx_image_t *background,  /* in */
                                          wx_image_t *blend /* in/out */);

/**
 * @brief 获取本线程调用最后的错误描述，当前面某个函数返回值不为0时有效
 */
WX_API const char *wx_vseg_get_last_error_msg();

#endif // !_WX_VSEG_H_
