/**********************************************************************************
File Name	:	faceutil.cpp 
Description	:	TelpoFace人脸识别终端底层控制接口jni库
Revision	:	v1.0
Author		:	jiangxh
Date		:	2018/10/11
***********************************************************************************/

#define LOG_TAG "faceutil"
#define LOG_NDDEBUG 0

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <jni.h>

#include <cassert>
#include <cstdlib>

#include <cutils/log.h>
#include <cutils/properties.h>
#include <cutils/str_parms.h>

#include "jni.h"
#include "JNIHelp.h"

#include "wiegand.h"

using namespace std;

void RecvCallback(WiegandDev *dev, unsigned char* data, int len);

class WiegandRd
{
private:

public:
	WiegandRd();
	~WiegandRd();

	WiegandDev *wiegand_dev;

	JavaVM *g_VM;
	jobject g_obj;

};

WiegandRd::WiegandRd()
{
	wiegand_dev = new WiegandDev();
	//配置回调函数
	wiegand_dev->recv_event = RecvCallback;
}

WiegandRd::~WiegandRd()
{
	delete wiegand_dev;
	wiegand_dev = NULL;
}

static WiegandRd *WiegandRead = NULL;

//----------------------------------------------------------------------
// Name:			RecvCallback
// Parameters:		
// Returns:			
// Description:		韦根接收回调函数
//----------------------------------------------------------------------
void RecvCallback(WiegandDev *dev, unsigned char* data, int len)
{
	JNIEnv *env;
	int mNeedDetach;
	int res, i;
	char str[64], *p_str;
	jstring dataStr;
	
	ALOGI("Got wiegand(%d):%02X%02X%02X%02X\n",
		len, data[0], data[1], data[2], data[3]);

	//获取当前native线程是否有没有被附加到jvm环境中
	int getEnvStat = WiegandRead->g_VM->GetEnv((void **)&env, JNI_VERSION_1_6);
	if (getEnvStat == JNI_EDETACHED) {
		//如果没有， 主动附加到jvm环境中，获取到env
		if (WiegandRead->g_VM->AttachCurrentThread(&env, NULL) != 0) {
			return;
		}
		mNeedDetach = JNI_TRUE;
	}

	//通过全局变量g_obj 获取到要回调的类
	jclass javaClass = env->GetObjectClass(WiegandRead->g_obj);
	if (javaClass == 0) {
		ALOGE("%s:Unable to find class", __func__);
		WiegandRead->g_VM->DetachCurrentThread();
		return;
	}

	//获取要回调的方法ID                                            
	jmethodID javaCallbackId = env->GetMethodID(javaClass, "WiegandRdEventCallback", "(Ljava/lang/String;)I");
	if (javaCallbackId == NULL) {
		ALOGE("%s:Unable to find method : WiegandRdEventCallback", __func__);
		WiegandRead->g_VM->DetachCurrentThread();
		return;
	}

	//将wiegand data转换为字符串
	p_str = str;
	for (i = 0; i < len; i++)
	{
		//sprintf(p_str, "%02X", data[i]);		
		//p_str += 2;
		sprintf(p_str, "%d", data[i]);
		p_str += 1;
	}
	*p_str = 0;

	dataStr = env->NewStringUTF(str);
	//执行回调
	env->CallIntMethod(WiegandRead->g_obj, javaCallbackId, dataStr);

	//释放当前线程
	if (mNeedDetach) {
		WiegandRead->g_VM->DetachCurrentThread();
	}
	env = NULL;
}


static int write_device(const char *dev, const char *parm)
{
	int fd, len;

	fd = open(dev, O_WRONLY);
	if (fd == -1)
	{
		ALOGE("%s:  %s open error", __func__, dev);
		return -1;
	}
	
	len = strlen(parm);
	write(fd, parm, len);

	close(fd);

	return 0;
}

static int read_device(const char *dev, char *buf, int size)
{
	int fd, len;

	fd = open(dev, O_RDONLY);
	if (fd == -1)
	{
		ALOGE("%s:  %s open error", __func__, dev);
		return -1;
	}

	len = read(fd, buf, size);
	
	//ALOGE("%s: read data %d", __func__, len);
	
	close(fd);

	if(len > 0)
		return len;
	else
		return -2;
}


char* jstringToChar(JNIEnv* env, jstring jstr) {
    char* rtn = NULL;
    jclass clsstring = env->FindClass("java/lang/String");
    jstring strencode = env->NewStringUTF("GB2312");
    jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
    jbyteArray barr = (jbyteArray) env->CallObjectMethod(jstr, mid, strencode);
    jsize alen = env->GetArrayLength(barr);
    jbyte* ba = env->GetByteArrayElements(barr, JNI_FALSE);
    if (alen > 0) {
        rtn = (char*) malloc(alen + 1);
        memcpy(rtn, ba, alen);
        rtn[alen] = 0;
    }
    env->ReleaseByteArrayElements(barr, ba, 0);
    return rtn;
}

//----------------------------------------------------------------------
// Name:			RelaySet
// Parameters:		
//		on			1==>开启，0==>关闭
// Returns:			=0 :成功;
//					<0 :错误;
// Description:		控制继电器开关
//----------------------------------------------------------------------
extern "C" JNIEXPORT jint
JNICALL Java_com_common_face_api_FaceUtil_RelaySet(JNIEnv *env, jobject thiz, jint on)
{
	int ret = -1;
	
	ALOGD("%s: %d", __func__, on);

	if (on)
		ret = write_device("/sys/class/gpio-ctrl/relay/ctrl", "on");
	else
		ret = write_device("/sys/class/gpio-ctrl/relay/ctrl", "off");

	return ret;
}

//----------------------------------------------------------------------
// Name:			RelaysSet
// Parameters:	
//		index			
//		on			1==>开启，0==>关闭
// Returns:			=0 :成功;
//					<0 :错误;
// Description:		控制继电器开关
//----------------------------------------------------------------------
extern "C" JNIEXPORT jint
JNICALL Java_com_common_face_api_FaceUtil_RelaysSet(JNIEnv *env, jobject thiz, jint num, jint on)
{
	int ret = -1;
	char file[100];
	
	if(num < 1) {
		ALOGE("%s: Relay num error", __func__, on);
		return -2;
	}
	
	ALOGD("%s: %d", __func__, on);
	if(num == 1)
		strcpy(file, "/sys/class/gpio-ctrl/relay/ctrl");
	else
		sprintf(file, "/sys/class/gpio-ctrl/relay%d/ctrl", num);
	
	if (on)
		ret = write_device(file, "on");
	else
		ret = write_device(file, "off");
	
	return ret;
}

//----------------------------------------------------------------------
// Name:			LedSet
// Parameters:	
//		name        led节点名称			
//		on			1==>开启，0==>关闭
// Returns:			=0 :成功;
//					<0 :错误;
// Description:		LED灯开关
//----------------------------------------------------------------------
extern "C" JNIEXPORT jint
JNICALL Java_com_common_face_api_FaceUtil_LedSet(JNIEnv *env, jobject thiz, jstring name, jint on)
{
	int ret = -1;
	char file[100];
	
	char* chardata = jstringToChar(env, name);
	
	ALOGD("%s: %d", __func__, on);
	sprintf(file, "/sys/class/leds/%s/brightness", chardata);
	
	if (on)
		ret = write_device(file, "255");
	else
		ret = write_device(file, "0");
	
	return ret;
}

//----------------------------------------------------------------------
// Name:			GPIOSet
// Parameters:	
//		name        gpio节点名称			
//		on			1==>开启，0==>关闭
// Returns:			=0 :成功;
//					<0 :错误;
// Description:		GPIO控制
//----------------------------------------------------------------------
extern "C" JNIEXPORT jint
JNICALL Java_com_common_face_api_FaceUtil_GPIOSet(JNIEnv *env, jobject thiz, jstring name, jint on)
{
	int ret = -1;
	char file[100];
	
	char* chardata = jstringToChar(env, name);
	
	ALOGD("%s: %d", __func__, on);
	sprintf(file, "/sys/class/gpio-ctrl/%s/ctrl", chardata);
	
	if (on)
		ret = write_device(file, "on");
	else
		ret = write_device(file, "off");
	
	return ret;
}
//----------------------------------------------------------------------
// Name:			RS485SetMode
// Parameters:		
//		mode		1==>发送模式，0==>接收模式
// Returns:			=0 :成功;
//					<0 :错误;
// Description:		设置RS485发送接收状态
//----------------------------------------------------------------------
extern "C" JNIEXPORT jint
JNICALL Java_com_common_face_api_FaceUtil_RS485SetMode(JNIEnv *env, jobject thiz, jint mode)
{
	int ret = -1;
	
	ALOGD("%s: %d", __func__, mode);

	if (mode)
		ret = write_device("/sys/class/gpio-ctrl/rs485/ctrl", "send");
	else
		ret = write_device("/sys/class/gpio-ctrl/rs485/ctrl", "receive");

	return ret;
}

//----------------------------------------------------------------------
// Name:			ExtInputGetState
// Parameters:		none
// Returns:			>=0 :状态值
//					<0  :获取失败
// Description:		获取外部输入信号状态
//----------------------------------------------------------------------
extern "C" JNIEXPORT jint
JNICALL Java_com_common_face_api_FaceUtil_ExtInputGetState(JNIEnv *env, jobject thiz, jstring name)
{
	char file[100];
	char str[32];
	int ret;
	
	char* chardata = jstringToChar(env, name);

	sprintf(file, "/sys/class/switch-gpios/%s/state", chardata);
	ret = read_device(file, str, 32);
	if (ret > 0)
	{
		ret = atoi(str);
		ALOGD("%s: %d", __func__, ret);		
	}

	return ret;
}

//----------------------------------------------------------------------
// Name:			WiegandSend
// Parameters:		
//		data		发送数据
// Returns:			=0 :成功;
//					<0 :错误;
// Description:		韦根数据发送
//----------------------------------------------------------------------
extern "C" JNIEXPORT jint
JNICALL Java_com_common_face_api_FaceUtil_WiegandSend(JNIEnv *env, jobject thiz, jlong data)
{
	char str[32];

	sprintf(str, "%lld", data);
	ALOGD("%s: %s", __func__, str);

	return write_device("/sys/class/wiegand/wiegand-send/send", str);
}

//----------------------------------------------------------------------
// Name:			WiegandSetBitLength
// Parameters:		
//		length		位长，1~64
// Returns:			=0 :成功;
//					<0 :错误;
// Description:		设置韦根发送数据的位长度
//----------------------------------------------------------------------
extern "C" JNIEXPORT jint
JNICALL Java_com_common_face_api_FaceUtil_WiegandSetBitLength(JNIEnv *env, jobject thiz, jint length)
{
	char str[32];

	if (length < 1)
	{
		ALOGE("%s: bit length not valid", __func__);
		return -1;
	}

	sprintf(str, "%d", length);
	ALOGD("%s: %s", __func__, str);

	return write_device("/sys/class/wiegand/wiegand-send/bit_length", str);
}

//----------------------------------------------------------------------
// Name:			WiegandGetBitLength
// Parameters:		none
// Returns:			>=0 :数据位长度
//					<0  :获取失败
// Description:		获取韦根发送数据的位长度
//----------------------------------------------------------------------
extern "C" JNIEXPORT jint
JNICALL Java_com_common_face_api_FaceUtil_WiegandGetBitLength(JNIEnv *env, jobject thiz)
{
	char str[32];
	int ret;

	ret = read_device("/sys/class/wiegand/wiegand-send/bit_length", str, 32);
	if (ret > 0)
	{
		ret = atoi(str);
		ALOGD("%s: %d", __func__, ret);
	}

	return ret;
}

//----------------------------------------------------------------------
// Name:			WiegandSetBitWidth
// Parameters:		
//		width		位宽，1~1000000
// Returns:			=0 :成功;
//					<0 :错误;
// Description:		设置韦根总线发送数据的位宽，单位为us
//----------------------------------------------------------------------
extern "C" JNIEXPORT jint
JNICALL Java_com_common_face_api_FaceUtil_WiegandSetBitWidth(JNIEnv *env, jobject thiz, jint width)
{
	char str[32];

	if (width < 1 && width > 1000000)
	{
		ALOGE("%s: bit width not valid", __func__);
		return -1;
	}

	sprintf(str, "%d", width);
	ALOGD("%s: %s", __func__, str);

	return write_device("/sys/class/wiegand/wiegand-send/bit_width", str);
}

//----------------------------------------------------------------------
// Name:			WiegandGetBitWidth
// Parameters:		none
// Returns:			>=0 :数据位宽，单位为us;
//					<0  :获取失败
// Description:		获取韦根发送数据的位宽
//----------------------------------------------------------------------
extern "C" JNIEXPORT jint
JNICALL Java_com_common_face_api_FaceUtil_WiegandGetBitWidth(JNIEnv *env, jobject thiz)
{
	char str[32];
	int ret;

	ret = read_device("/sys/class/wiegand/wiegand-send/bit_width", str, 32);
	if (ret > 0)
	{
		ret = atoi(str);
		ALOGD("%s: %d", __func__, ret);
	}

	return ret;
}

//----------------------------------------------------------------------
// Name:			WiegandSetBitInvalid
// Parameters:		
//		width		位间隔，1~1000000
// Returns:			=0 :成功;
//					<0 :错误;
// Description:		设置韦根总线发送数据的位间隔时间，单位为us
//----------------------------------------------------------------------
extern "C" JNIEXPORT jint
JNICALL Java_com_common_face_api_FaceUtil_WiegandSetBitInvalid(JNIEnv *env, jobject thiz, jint invalid)
{
	char str[32];

	if (invalid < 1 && invalid > 1000000)
	{
		ALOGE("%s: bit width not valid", __func__);
		return -1;
	}

	sprintf(str, "%d", invalid);
	ALOGD("%s: %s", __func__, str);

	return write_device("/sys/class/wiegand/wiegand-send/bit_interval", str);
}

//----------------------------------------------------------------------
// Name:			WiegandGetBitInvalid
// Parameters:		none
// Returns:			>=0 :数据位间隔时间，单位为us;
//					<0  :获取失败
// Description:		获取韦根总线发送数据的位间隔时间
//----------------------------------------------------------------------
extern "C" JNIEXPORT jint
JNICALL Java_com_common_face_api_FaceUtil_WiegandGetBitInvalid(JNIEnv *env, jobject thiz)
{
	char str[32];
	int ret;

	ret = read_device("/sys/class/wiegand/wiegand-send/bit_interval", str, 32);
	if (ret > 0)
	{
		ret = atoi(str);
		ALOGD("%s: %d", __func__, ret);
	}

	return ret;
}
//----------------------------------------------------------------------
// Name:			Open
// Parameters:		none
// Returns:			=0 :成功;
//					<0 :失败;
// Description:		开启韦根读数据
//----------------------------------------------------------------------
extern "C" JNIEXPORT jint
//			   com.common.face.api.FaceUtil.WiegandRdFactory.WiegandRd_Open
JNICALL Java_com_common_face_api_FaceUtil_WiegandRdFactory_WiegandRd_1Open(JNIEnv *env, jobject thiz)
{
	int ret;

	if (WiegandRead != NULL)
	{
		ALOGE("%s:WiegandRd already opened\n", __func__);
		return -1;
	}

	ALOGI("%s:Open the WiegandRd function\n", __func__);

	WiegandRead = new WiegandRd();


	//JavaVM是虚拟机在JNI中的表示，等下再其他线程回调java层需要用到
	env->GetJavaVM(&WiegandRead->g_VM);
	// 生成一个全局引用保留下来，以便回调
	WiegandRead->g_obj = env->NewGlobalRef(thiz);
	
	//启动线程
	WiegandRead->wiegand_dev->start();

	return 0;
}
//----------------------------------------------------------------------
// Name:			Close
// Parameters:		none
// Returns:			=0 :成功;
//					<0 :失败;
// Description:		关闭韦根读数据
//----------------------------------------------------------------------
extern "C" JNIEXPORT jint
JNICALL Java_com_common_face_api_FaceUtil_WiegandRdFactory_WiegandRd_1Close(JNIEnv *env, jobject thiz)
{
	if (WiegandRead == NULL)
	{
		ALOGE("%s:WiegandRead already close\n", __func__);
		return 0;
	}

	ALOGI("%s:Close the WiegandRead function\n", __func__);

	//结束线程
	WiegandRead->wiegand_dev->stop();

	/*
	//释放当前线程
	if (NfcRead->ic_dev->javaCallbackId != NULL) {
		NfcRead->ic_dev->g_VM->DetachCurrentThread();
	}
	NfcRead->ic_dev->env = NULL;
	if (NfcRead->id_dev->javaCallbackId != NULL) {
		NfcRead->id_dev->g_VM->DetachCurrentThread();
	}
	NfcRead->id_dev->env = NULL;
	*/

	WiegandRead->~WiegandRd();
	delete WiegandRead;
	WiegandRead = NULL;

	return 0;
}

/*
JNINativeMethod gMethods[] = {
	{ "Relay_Set", "(I)I", (void *)Relay_Set },
	{ "RS485_SetMode", "(I)I", (void *)RS485_SetMode },
 	{ "Wiegand_Send", "(J)I", (void *)Wiegand_Send },
 	{ "Wiegand_Set_BitLength", "(I)I", (void *)Wiegand_Set_BitLength },
 	{ "Wiegand_Get_BitLength", "()I", (void *)Wiegand_Get_BitLength },	
	{ "Wiegand_Set_BitWidth", "(I)I", (void *)Wiegand_Set_BitWidth },
	{ "Wiegand_Get_BitWidth", "()I", (void *)Wiegand_Get_BitWidth },
	{ "Wiegand_Set_BitInvalid", "(I)I", (void *)Wiegand_Set_BitInvalid },
	{ "Wiegand_Get_BitInvalid", "()I", (void *)Wiegand_Get_BitInvalid },
};


//此函数通过调用RegisterNatives方法来注册我们的函数
static int registerNativeMethods(JNIEnv* env, const char* className, JNINativeMethod* getMethods, int methodsNum) {
	jclass clazz;
	//找到声明native方法的类
	clazz = env->FindClass(className);
	if (clazz == NULL) {
		return JNI_FALSE;
	}
	//注册函数 参数：java类 所要注册的函数数组 注册函数的个数
	if (env->RegisterNatives(clazz, getMethods, methodsNum) < 0) {
		return JNI_FALSE;
	}
	return JNI_TRUE;
}

static int registerNatives(JNIEnv* env) {
	//指定类的路径，通过FindClass 方法来找到对应的类
	const char* className = "com/common/face/api/FaceUtil";
	return registerNativeMethods(env, className, gMethods, sizeof(gMethods) / sizeof(gMethods[0]));
}
//回调函数
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
	JNIEnv* env = NULL;
	//获取JNIEnv
	if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
		return -1;
	}
	assert(env != NULL);
	//注册函数 registerNatives ->registerNativeMethods ->env->RegisterNatives
	if (!registerNatives(env)) {
		return -1;
	}
	//返回jni 的版本 
	return JNI_VERSION_1_6;
}
*/


