/*
 * @file TransferAgent.cpp 网络文件传输代理定义文件
 * @version 0.1
 * @date 2017-10-29
 */
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/bind/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "globaldef.h"
#include "GLog.h"
#include "TransferAgent.h"

using namespace boost::filesystem;
using namespace boost::posix_time;
using namespace boost::placeholders;

TransferAgent::TransferAgent() {
	param_.LoadFile(gConfigPath);
}

TransferAgent::~TransferAgent() {
}

bool TransferAgent::StartService() {
	/* 创建文件存储接口 */
	fwptr_ = make_filewritter();
	fwptr_->SetDatabase(param_.bDB, param_.urlDB.c_str());
	fwptr_->UpdateStorage(param_.pathStorage.c_str());
	/* 启动服务器 */
	const TCPServer::CBSlot &slot = boost::bind(&TransferAgent::network_accept, this, _1, _2);
	tcps_fs_ = maketcp_server();
	tcps_fs_->RegisterAccespt(slot);
	if (tcps_fs_->CreateServer(param_.portFS)) {
		_gLog.Write(LOG_FAULT, NULL, "failed to create server on port<%d>", param_.portFS);
		return false;
	}

	tcps_dp_ = maketcp_server();
	tcps_dp_->RegisterAccespt(slot);
	if (tcps_dp_->CreateServer(param_.portDP)) {
		_gLog.Write(LOG_FAULT, NULL, "failed to create server on port<%d>", param_.portDP);
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
	TCPServer *s = (TCPServer*) server;
	if (s == tcps_fs_.get()) {
		mutex_lock lck(mtx_filercv_);
		FileRcvPtr receiver = make_filercv(fwptr_);
		if (receiver->CoupleNetwork(client)) filercv_.push_back(receiver);
	}
	else {// s == tcps_dp_.get
		_gLog.Write("accepted connection from data-process");
		const TCPClient::CBSlot &slot = boost::bind(&TransferAgent::receive_dp, this, _1, _2);
		client->RegisterRead(slot);
		client->UseBuffer();
		tcpc_dp_ = client;
		fwptr_->CoupleNetwork(client);
	}
}

void TransferAgent::receive_dp(const long client, const long ec) {
	TCPClient *tcpc = (TCPClient *)client;
	if (ec && tcpc->IsOpen()) {
		_gLog.Write(LOG_WARN, NULL, "data-process had broken connection");
		fwptr_->DecoupleNetowrk();
		tcpc_dp_->Close();
	}
}

const char *TransferAgent::find_storage() {
	return param_.pathStorage.c_str();
}

void TransferAgent::free_storage() {
	path filepath(param_.pathStorage);
	space_info nfspace;
	nfspace = space(filepath);
	if ((nfspace.available >> 30) < param_.minDiskStorage) {
		directory_iterator itend = directory_iterator();
		ptime now = second_clock::local_time(), tmlast;
		ptime tmdir;
		int mjd_now = now.date().modjulian_day();
		int days4 = 4 * 86400; // 删除4日前所有数据
		int pos;
		int year, month, day, ymd;
		string filename;

		_gLog.Write("Cleaning <%s> for LocalStorage", filepath.c_str());
		for (directory_iterator x = directory_iterator(filepath); x != itend; ++x) {
			get_filetime(x->path(), tmlast);
			filename = x->path().filename().string();
			if (filename.front() != 'G') continue; // 仅清除G开头文件/目录 -- GWAC
			pos = filename.rfind('_');
			if (pos == string::npos) continue;
			ymd = stoi(filename.substr(pos + 1));
			day = ymd % 100;
			year = ymd / 10000;
			month = (ymd - year * 10000 - day) / 100;
			tmdir = ptime(ptime::date_type(year + 2000, month, day));
			if ((mjd_now - tmdir.date().modjulian_day()) > days4) remove_all(x->path());
		}

		nfspace = space(filepath);
		_gLog.Write("free capacity of <%s> is %d GB", filepath.c_str(), nfspace.available >> 30);
		fwptr_->UpdateStorage(filepath.c_str());
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
