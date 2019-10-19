/*
 * @file TransferAgent.cpp 网络文件传输代理定义文件
 * @version 0.1
 * @date 2017-10-29
 */
#include <algorithm>
#include "globaldef.h"
#include "GLog.h"
#include "TransferAgent.h"

TransferAgent::TransferAgent() {
	param_.LoadFile(gConfigPath);
}

TransferAgent::~TransferAgent() {
}

bool TransferAgent::StartService() {
	if (param_.pathStorage.size() != param_.nStorage
			|| param_.pathTemplate.size() != param_.nTemplate
			|| param_.nStorage <= 0 || param_.nTemplate <= 0) {// 安全检查
		_gLog.Write(LOG_FAULT, NULL, "storage or template path number doesn't math settings");
		return false;
	}
	/* 创建文件存储接口 */
	fwptr_ = make_filewritter();
	fwptr_->SetDatabase(param_.bDB, param_.urlDB.c_str());
	fwptr_->SetNotifyPath(param_.bNotifyStorage, param_.pathNotifyStorage.c_str());
	/* 查找可用磁盘存储空间 */
	const char *path = find_storage();
	if (path) {
		fwptr_->UpdateStorage(path);
	}
	else {
		_gLog.Write(LOG_FAULT, NULL, "no disk has enough free capacity for raw data");
		return false;
	}
	/* 启动服务器 */
	const TCPServer::CBSlot &slot = boost::bind(&TransferAgent::network_accept, this, _1, _2);
	tcpsrv_ = maketcp_server();
	tcpsrv_->RegisterAccespt(slot);
	if (tcpsrv_->CreateServer(param_.port)) {
		_gLog.Write(LOG_FAULT, NULL, "failed to create server on port<%d>", param_.port);
		return false;
	}
	/* 启动时钟同步 */
	if (param_.bNTP) {
		ntp_ = make_ntp(param_.ipNTP.c_str(), 123, param_.diffNTP);
		ntp_->EnableAutoSynch();
	}
	/* 启动线程 */
	thrdIdle_.reset(new boost::thread(boost::bind(&TransferAgent::thread_idle, this)));
	thrdAutoFree_.reset(new boost::thread(boost::bind(&TransferAgent::thread_autofree, this)));

	return true;
}

void TransferAgent::StopService() {
	interrupt_thread(thrdIdle_);
	interrupt_thread(thrdAutoFree_);
	filercv_.clear();
}

void TransferAgent::network_accept(const TcpCPtr&client, const long server) {
	mutex_lock lck(mtx_filercv_);
	FileRcvPtr receiver = make_filercv(fwptr_);
	if (receiver->CoupleNetwork(client)) filercv_.push_back(receiver);
}

const char *TransferAgent::find_storage() {
	namespace fs = boost::filesystem;
	int n = param_.nStorage, iNow = param_.iStorage, iNew, i;
	fs::path path;
	fs::space_info space;
	for (i = 0, iNew = iNow; i < n; ++i, ++iNew) {
		if (iNew >= n) iNew -= n;
		path = param_.pathStorage[iNew];
		space = fs::space(path);
		if ((space.available >> 30) >= param_.minDiskStorage) break;
	}
	if (i < n && iNow != iNew) param_.iStorage = iNew;
	return i == n ? NULL : param_.pathStorage[iNew].c_str();
}

void TransferAgent::free_storage() {
	int iNow = param_.iStorage;
	const char *path = find_storage();
	if (path) {// 更新存储空间
		if (iNow != param_.iStorage) fwptr_->UpdateStorage(path);
	}
	else {// 启动清理流程
		if (++iNow >= param_.nStorage) iNow = 0;
		namespace fs = boost::filesystem;
		namespace pt = boost::posix_time;

		fs::path path(param_.pathStorage[iNow]);
		fs::space_info space;
		fs::directory_iterator itend = fs::directory_iterator();
		pt::ptime now = pt::second_clock::local_time(), tmlast;
		int days3 = 3 * 86400; // 删除3日前所有数据

		_gLog.Write("Cleaning <%s> for LocalStorage", path.c_str());
		for (fs::directory_iterator x = fs::directory_iterator(path); x != itend; ++x) {
			get_filetime(x->path(), tmlast);
			if (x->path().filename().c_str()[0] != 'G') continue; // 仅清除G开头文件/目录 -- GWAC
			if ((now - tmlast).total_seconds() > days3) fs::remove_all(x->path());
		}

		space = fs::space(path);
		_gLog.Write("free capacity of <%s> is %d GB", path.c_str(), space.available >> 30);
		param_.iStorage = iNow;
		fwptr_->UpdateStorage(path.c_str());
	}
}

void TransferAgent::free_template_directory(const char *dirname) {
	namespace fs = boost::filesystem;
	fs::path path = dirname;
	fs::directory_iterator itend = fs::directory_iterator();

	for (fs::directory_iterator x = fs::directory_iterator(path); x != itend; ++x) {
		if ((fs::file_size(x->path()) >> 10) > 100) fs::remove(x->path());
	}
}

void TransferAgent::free_template() {
	namespace fs = boost::filesystem;
	namespace pt = boost::posix_time;

	fs::path path = param_.pathTemplate[0];
	fs::space_info space;

	space = fs::space(path);
	if ((space.available >> 30) <= param_.minDiskTemplate) {// 启动清理流程
		fs::directory_iterator itend = fs::directory_iterator();
		pt::ptime now = pt::second_clock::local_time(), tmlast;
		int days;
		int days3 = 3 * 86400; // 删除3日前数据
		int days10= 10 * 86400; // 删除10日前数据

		for (int i = 0; i < param_.nTemplate; ++i) {
			path = param_.pathTemplate[i];

			_gLog.Write("Cleaning <%s> for Template", path.c_str());
			for (fs::directory_iterator x = fs::directory_iterator(path); x != itend; ++x) {
				get_filetime(x->path(), tmlast);
				if (x->path().filename().c_str()[0] != 'G') continue; // 仅清除G开头文件/目录 -- GWAC
				days = (now - tmlast).total_seconds();
				if (days > days10 || (fs::is_regular_file(x->path()) && days > days3)) {
					fs::remove_all(x->path());
				}
				else if (days > days3 && fs::is_directory(x->path())) {// 3-7天间目录, 清除目录内大容量文件
					free_template_directory(x->path().c_str());
				}
			}
		}
		space = fs::space(path);
		_gLog.Write("free capacity for template is %d GB", space.available >> 30);
	}
}

void TransferAgent::thread_idle() {
	boost::chrono::minutes period(1);
	FileRcvVec::iterator it;

	while(1) {
		boost::this_thread::sleep_for(period);

		mutex_lock lck(mtx_filercv_);
		for (it = filercv_.begin(); it != filercv_.end(); ) {
			if ((*it)->IsAlive()) ++it;
			else it = filercv_.erase(it);
		}
	}
}

void TransferAgent::thread_autofree() {
	// 服务启动5秒后检查一次磁盘空间
	boost::this_thread::sleep_for(boost::chrono::seconds(5));

	while(1) {
		if (param_.bFreeStorage)  free_storage();
		if (param_.bFreeTemplate) free_template();
		// 之后每天中午检查一次磁盘空间
		boost::this_thread::sleep_for(boost::chrono::seconds(next_noon()));
	}
}

long TransferAgent::next_noon() {
	namespace pt = boost::posix_time;
	pt::ptime now(pt::second_clock::local_time());
	pt::ptime noon(now.date(), pt::hours(12));
	long secs = (noon - now).total_seconds();
	return secs < 10 ? secs + 86400 : secs;
}

void TransferAgent::interrupt_thread(threadptr& thrd) {
	if (thrd.unique()) {
		thrd->interrupt();
		thrd->join();
		thrd.reset();
	}
}

void TransferAgent::get_filetime(const boost::filesystem::path &path, boost::posix_time::ptime &tmlast) {
	tmlast = boost::posix_time::from_time_t(boost::filesystem::last_write_time(path));
}
