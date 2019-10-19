/* 
 * File:   DataTransfer.h
 * Author: xy
 *
 * Created on May 24, 2016, 10:12 PM
 */

#ifndef DataTransfer_H
#define	DataTransfer_H

#include <map> 
#include <string> 
#include <vector>
#include "AllHeader.h"

using namespace std;

#define DEBUG1

#define URL_MAX_LENGTH 10240
#define CURL_ERROR_BUFFER 10240
#define ROOT_URL "http://127.0.0.1/"

#define SEND_OT1_LIST_URL				((char*)"commonFileUpload.action")
#define SEND_OT2_CUT_IMAGE_LIST_URL		((char*)"commonFileUpload.action")
#define SEND_IMAGE_QUALITY_URL			((char*)"commonFileUpload.action")
#define SEND_LOOK_BACK_URL				((char*)"otLookBack.action")
#define SEND_LOG_MSG_URL				((char*)"commonLog.action")
#define SEND_MAG_CALIBRATION_URL		((char*)"commonFileUpload.action")
#define SEND_FITS_PREVIEW_URL			((char*)"commonFileUpload.action")
#define REG_ORIG_IMAGE_URL				((char*)"regOrigImg.action")
#define GET_OT2_CUT_IMAGE_LIST_URL		((char*)"commonFileUpload.action")
#define GET_OT2_CUT_IMAGE_REF_LIST_URL	((char*)"commonFileUpload.action")

struct CurlCache {
  char *memory;
  size_t size;
};

class DataTransfer {
public:

  /**mkdir /mnt/gwacMem && sudo mount -osize=100m tmpfs /mnt/gwacMem -t tmpfs*/
  char *tmpPath; //temporary file store path, Linux Memory/Temporary File System
  char *rootUrl; //web server root url http://190.168.1.25
  char *ot1ListUrl;
  char *getOt2CutImageListUrl;
  char *imageQualityFileUrl;
  char *lookBackResultUrl;
  char *sendOt2CutImageListUrl;
  char *ot2TmplCutImageListUrl;
  char *sendLogMsgUrl;
  char *sendMagCalibrationUrl;
  char *sendFitsPreviewUrl;
  char *regOrigImgUrl;

  /**
   * 
   * @param rootUrl Web服务器网址，如http://190.168.1.25
   */
  DataTransfer(char *url);
  virtual ~DataTransfer();

  /**
   * 
   * @param groupId 组ID
   * @param unitId 单元ID
   * @param camId 相机ID
   * @param gridId 天区划分模式ID
   * @param fieldId 天区编号
   * @param imgName原始图像的名称
   * @param imgPath原始图像的绝对路径
   * @param genTime 格式为CCYY-MM-DDThh:mm:ss.sss
   * @param statusstr 若程序调用出错，则返回错误字符串
   * @return 正常返回GWAC_SUCCESS（0）
   */
  int regOrigImage(const char *groupId, const char *unitId, const char *camId, const char *gridId,
		  const char *fieldId, const char *imgName, const char *imgPath, const char *genTime, char statusstr[]);

  /**
   * 向服务器发送OT1列表文件
   * @param path OT1列表文件路径
   * @param fName OT1列表文件名，命名规范：原始图像名.crsot1
   * @param statusstr 返回结果
   * @return 成功返回GWAC_SUCCESS
   */
  int sendOT1ListFile(char *path, char *fName, char statusstr[]);

  /**
   * 向服务器发送OT2切图文件
   * @param path OT2切图文件路径
   * @param fNames OT2切图文件名数组，OT2切图命名规范：按照切图文件列表中给出的名称命名，如OT2切图名称.fit，OT2切图名称.jpg
   * @param statusstr 返回结果
   * @return 成功返回GWAC_SUCCESS
   */
  int sendOT2CutImage(char *path, vector<char *> &fNames, char statusstr[]);

  /**
   * 向服务器发送OT2模板切图文件
   * @param path OT2模板切图文件路径
   * @param fNames OT2模板切图文件名数组,OT2模板切图命名规范：按照切图文件列表中给出的名称+模板制作日期命名，OT2名称_ref_YYYYmmddThhMMss.fit，OT2名称_ref_YYYYmmddThhMMss.jpg
   * @param statusstr 返回结果
   * @return 成功返回GWAC_SUCCESS
   */
  int sendOT2CutImageRef(char *path, vector<char *> &fNames, char statusstr[]);

  /**
   * 向服务器发送图像参数文件
   * @param path 图像参数文件路径
   * @param fName 图像参数文件名，图像参数文件命名规范：原始图像名.imqty
   * @param statusstr 返回结果
   * @return 成功返回GWAC_SUCCESS
   */
  int sendImageQualityFile(char *path, char *fName, char statusstr[]);

  /**
   * 向服务器发送OT2回看结果，
   * @param ot2Name OT2名
   * @param flag OT2回看结果：0结果未知，1回看结果为OT，2回看结果为FOT
   * @param statusstr 返回结果
   * @return 成功返回GWAC_SUCCESS
   */
  int sendLookBackResult(char *ot2Name, int flag, char statusstr[]);

  /**
   * 向服务器发送FITS预览图像文件
   * @param path FITS预览图像文件路径
   * @param fName FITS预览图像文件名, 命名规范:原始图像名.jpg
   * @param statusstr 返回结果
   * @return 成功返回GWAC_SUCCESS
   */
  int sendFitsPreview(char *path, char *fName, char statusstr[]);

  /**
   * 向服务器发送星等标定文件
   * @param path 星等标定文件路径
   * @param fName 星等标定文件名, 命名规范:原始图像名.magclb
   * @param statusstr 返回结果
   * @return 成功返回GWAC_SUCCESS
   */
  int sendMagCalibrationFile(char *path, char *fName, char statusstr[]);

  /**
   * 向服务器发送日志消息
   * @param msg 日志消息结构体
   * @param statusstr 返回结果
   * @return 成功返回GWAC_SUCCESS
   */
  int sendLogMsg(ST_MSGBUF *msg, char statusstr[]);

  /**
   * 从服务器获取OT2切图列表文件
   * @param path OT2切图文件存储路径
   * @param fName OT2切图文件存储名
   * @param statusstr 返回结果
   * @return 成功返回GWAC_SUCCESS
   */
  int getOT2CutImageList(char *path, char *fName, char statusstr[]);

  /**
   * 从服务器获取OT2模板切图列表文件
   * @param path OT2模板切图文件存储路径
   * @param fName OT2模板切图文件存储名
   * @param statusstr 返回结果
   * @return 成功返回GWAC_SUCCESS
   */
  int getOT2TmplCutImageList(char *path, char *fName, char statusstr[]);

  /**
   * 将参数或文件上次到服务器
   * @param url 服务器地址
   * @param path 文件所在独立
   * @param params 参数键值对<参数名，参数值>
   * @param files 文件键值对<上次文件名，实际文件名>，“path+实际文件名”组成待上传文件路径
   * @param statusstr 函数返回状态字符串
   * @return 函数返回状态值
   */
  int uploadDatas(const char url[], const char path[], multimap<string, string> params, multimap<string, string> files, char statusstr[]);
  int sendParameters(const char url[], multimap<string, string> params, char statusstr[]);
  int sendFiles(const char url[], const char path[], multimap<string, string> files, char statusstr[]);

private:
  struct CurlCache *tmpChunk;
  int initGwacMem(char *path);
  void initParameter(char *url);

};

#endif	/* DataTransfer_H */

