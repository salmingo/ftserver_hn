/*!
 * @file TransferAgent.h 网络文件传输代理声明文件
 * @version 0.1
 * @date 2017-10-29
 * @note
 * - 处理网络连接, 为其创建对象TransferClient
 * - 维持线程, 检查、切换与清除原始数据磁盘空间
 * - 维持线程, 检查与清除模板数据磁盘空间
 */

#ifndef TRANSFERAGENT_H_
#define TRANSFERAGENT_H_

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/container/stable_vector.hpp>
#include "MessageQueue.h"
#include "FileReceiver.h"
#include "FileWritter.h"
#include "parameter.h"
#include "tcpasio.h"
#include "NTPClient.h"

class TransferAgent {
public:
	TransferAgent();
	virtual ~TransferAgent();

public:
	// 数据结构
	typedef boost::container::stable_vector<FileRcvPtr> FileRcvVec;
	typedef boost::shared_ptr<boost::thread> threadptr;
	typedef boost::unique_lock<boost::mutex> mutex_lock;

protected:
	// 成员变量
	param_config param_;		//< 配置参数
	FileWritePtr fwptr_;		//< 文件写盘接口
	TcpSPtr tcpsrv_;			//< 网络服务器
	NTPPtr ntp_;				//< NTP接口
	threadptr thrdIdle_;			//< 线程: 空闲检查文件接收器有效性
	threadptr thrdAutoFree_;		//< 线程: 定时磁盘清除线程
	boost::mutex mtx_filercv_;	//< 互斥锁, 文件接收器
	FileRcvVec filercv_;			//< 文件接收接口

public:
	// 接口
	/*!
	 * @brief 尝试启动服务
	 * @return
	 * 服务启动结果
	 */
	bool StartService();
	/*!
	 * @brief 停止服务
	 */
	void StopService();

protected:
	// 功能
	/*!
	 * @brief 网络连接请求
	 * @param 为客户端分配的网络资源
	 * @param 服务器地址
	 */
	void network_accept(const TcpCPtr&, const long);
	/*!
	 * @brief 查找可用的本地存储盘区
	 * @return
	 * 可用盘区地址
	 */
	const char *find_storage();
	/*!
	 * @brief 清除本地存储空间
	 */
	void free_storage();
	/*!
	 * @brief 清除模板存储空间
	 */
	void free_template();
	/*!
	 * @brief 清除模板目录内大容量文件
	 * @param dirname 目录名称
	 */
	void free_template_directory(const char *dirname);
	/*!
	 * @brief 线程, 检查FileReceiver的有效性
	 */
	void thread_idle();
	/*!
	 * @brief 线程, 定时检查/清除磁盘空间
	 */
	void thread_autofree();
	/*!
	 * @brief 计算下一个正午与当前时间之间的秒数
	 * @return
	 * 秒数
	 */
	long next_noon();
	/*!
	 * @brief 中止线程
	 * @param thrd 线程指针
	 */
	void interrupt_thread(threadptr& thrd);
	/*!
	 * @brief 查看文件/目录的最后修改时间
	 * @param path     路径
	 * @param tmlast   最后修改时间
	 */
	void get_filetime(const boost::filesystem::path &path, boost::posix_time::ptime &tmlast);
};

#endif /* TRANSFERAGENT_H_ */
