/*!
 * Name        : ftserver.cpp
 * Author      : Xiaomeng Lu
 * Version     : 0.2
 * Copyright   : SVOM Group, NAOC
 * Description : 文件服务器, 接收并存储GWAC相机上传的FITS文件, 并在数据库中注册该文件
 * - 创建网络服务
 * - 接收FITS文件
 * - 存储FITS文件
 * - 注册FITS文件
 * - 记录日志文件
 * - 同步本地时钟
 **/

#include "GLog.h"
#include "parameter.h"
#include "daemon.h"
#include "globaldef.h"
#include "TransferAgent.h"

GLog _gLog;

int main(int argc, char** argv) {
	if (argc >= 2) {
		if (strcmp(argv[1], "-d") == 0) {
			param_config param;
			param.InitFile("ftserver.xml");
		}
		else {
			printf("Usage: ftserver <-d>\n");
		}
	}
	else {
		boost::asio::io_service ios;
		boost::asio::signal_set signals(ios, SIGINT, SIGTERM);  // interrupt signal
		signals.async_wait(boost::bind(&boost::asio::io_service::stop, &ios));

		if (!MakeItDaemon(ios)) return 1;
		if (!isProcSingleton(gPIDPath)) {
			_gLog.Write("%s is already running or failed to access PID file", DAEMON_NAME);
			return 2;
		}

		_gLog.Write("Try to launch %s %s %s as daemon", DAEMON_NAME, DAEMON_VERSION, DAEMON_AUTHORITY);
		// 主程序入口
		TransferAgent agent;
		if (agent.StartService()) {
			_gLog.Write("Daemon goes running");
			ios.run();
			agent.StopService();
		}
		else {
			_gLog.Write(LOG_FAULT, NULL, "Fail to launch %s", DAEMON_NAME);
		}
		_gLog.Write("Daemon stopped");
	}

	return 0;
}
