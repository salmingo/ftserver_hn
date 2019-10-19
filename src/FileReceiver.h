/*
 * @file FileReceiver.h 文件传输客户端声明文件, 接收文件
 * @version 0.1
 * @date 2017-10-29
 * @note
 * - 维持网络连接
 * - 接收客户端信息和文件数据
 * - 向客户端反馈状态
 */

#ifndef FILERECEIVER_H_
#define FILERECEIVER_H_

#include <boost/asio.hpp>
#include "MessageQueue.h"
#include "FileWritter.h"
#include "tcpasio.h"
#include "AsciiProtocol.h"

class FileReceiver : public MessageQueue {
public:
	FileReceiver(FileWritePtr fwptr);
	virtual ~FileReceiver();

protected:
	// 数据结构
	enum MSG_TC {// 消息
		MSG_NETWORK_RECEIVE = MSG_USER, //< 处理文件描述信息
		MSG_NETWORK_CLOSE,		//< 处理远程主机断开连接
		MSG_RECEIVE_COMPLETE		//< 完成数据接收
	};

	enum STATE {// 文件传输过程
		WAITING,		//< 等待新的文件
		READY,		//< 完成接收前准备
		COMPLETE	,	//< 文件传输完毕
		FAILURE		//< 文件接收错误
	};

	typedef boost::shared_ptr<boost::thread> threadptr;
	typedef boost::shared_array<char> charray;

protected:
	// 成员变量
	FileWritePtr fwptr_;		//< 文件写盘接口
	nfileptr fileptr_;		//< 待接收数据
	TcpCPtr tcpptr_;			//< 网络接口
	AscProtoPtr ascproto_;	//< 通信协议封装接口
	int state_;				//< 文件传输过程
	charray bufrcv_;			//< 数据接收缓存区

public:
	// 接口
	/*!
	 * @brief 关联网络接口
	 * @param client 网络接口
	 * @return
	 * 关联结果
	 */
	bool CoupleNetwork(TcpCPtr client);
	/*!
	 * @brief 检查网络连接是否有效
	 * @return
	 * 网络连接有效性
	 */
	bool IsAlive();

protected:
	// 功能
	/*!
	 * @brief 注册消息
	 */
	void register_messages();
	/*!
	 * @brief 处理收到的网络信息
	 * @param client 网络接口地址
	 * @param ec     错误代码. 0: 正确
	 */
	void network_receive(long client, long ec);
	/*!
	 * @brief 处理收到的文件描述信息
	 * @param param1 保留
	 * @param param2 保留
	 */
	void on_network_receive(long param1, long param2);
	/*!
	 * @brief 远程主机断开连接
	 * @param param1 保留
	 * @param param2 保留
	 */
	void on_network_close(long param1, long param2);
	/*!
	 * @brief 完成文件接收
	 * @param param1 保留
	 * @param param2 保留
	 */
	void on_receive_complete(long param1, long param2);
	/*!
	 * @brief 通知远程主机数据接收状况
	 * @param status 接收状态. 1: 可以发送数据; 2: 完成数据接收
	 */
	void notify_status(int status);
};
typedef boost::shared_ptr<FileReceiver> FileRcvPtr;
/*!
 * @brief 工厂函数, 创建TransferClient指针
 * @param ptr 文件写盘接口
 * @return
 * 文件接收接口
 */
extern FileRcvPtr make_filercv(FileWritePtr ptr);

#endif /* FILERECEIVER_H_ */
