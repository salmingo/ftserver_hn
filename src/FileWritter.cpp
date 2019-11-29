/*
 * @file FileWritter.cpp 文件写盘管理器定义文件
 * @version 0.1
 * @date 2017-10-28
 */

#include <boost/make_shared.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <stdio.h>
#include "FileWritter.h"
#include "GLog.h"

using namespace boost;
using namespace boost::posix_time;

FileWritePtr make_filewritter() {
	return boost::make_shared<FileWritter>();
}

FileWritter::FileWritter() {
	running_ = false;
	ascproto_ = boost::make_shared<AsciiProtocol>();
	thrdmntr_.reset(new boost::thread(boost::bind(&FileWritter::thread_monitor, this)));
}

FileWritter::~FileWritter() {
	if (thrdmntr_.unique()) {
		thrdmntr_->interrupt();
		thrdmntr_->join();
		running_ = false;
	}
	if (quenf_.size()) {
		_gLog.Write(LOG_WARN, "", "%d unsaved files will be lost", quenf_.size());
		quenf_.clear();
	}
}

void FileWritter::UpdateStorage(const char* path) {
	pathRoot_ = path;
	if (!pathNotify_.empty()) {
		FILE *fp = fopen(pathNotify_.c_str(), "wt");
		boost::posix_time::ptime t(boost::posix_time::second_clock::local_time());
		fprintf(fp, "%s     %s\n", path, boost::posix_time::to_iso_extended_string(t).c_str());
		fclose(fp);
	}
	_gLog.Write("LocalStorage use <%s>", path);
}

void FileWritter::SetDatabase(bool enabled, const char* url) {
	if (!enabled) dbt_.reset();
	else if(url) dbt_.reset(new DBCurl(url));
}

void FileWritter::SetNotifyPath(bool enabled, const char* filepath) {
	pathNotify_ = enabled ? filepath : "";
}

void FileWritter::NewFile(nfileptr nfptr) {
	if (running_) {
		quenf_.push_back(nfptr);
		cvfile_.notify_one();
	}
	else {
		_gLog.Write(LOG_WARN, NULL, "rejects new file for FileWritter terminated");
	}
}

void FileWritter::CoupleNetwork(const TcpCPtr tcpc) {
	tcpc_dp_ = tcpc;
}

void FileWritter::DecoupleNetowrk() {
	tcpc_dp_.reset();
}

void FileWritter::thread_monitor() {
	boost::mutex dummy;
	mutex_lock lck(dummy);
	bool rslt(true);
	int errcnt(0);
	boost::chrono::minutes period(1); // 异常等待延时1分钟

	running_ = true;
	while(errcnt < 5) {
		if (!quenf_.size()) // 队列空时, 等待新的文件
			cvfile_.wait(lck);
		else if (!rslt) { // 文件存储失败, 计数加1, 延时等待1分钟
			++errcnt;
			boost::this_thread::sleep_for(period);
		}
		else if (errcnt) // 存储成功, 清除计数
			errcnt = 0;
		rslt = save_first();
	}
	running_ = false;
	_gLog.Write(LOG_FAULT, NULL, "FileWritter terminated due to too much error");
}

bool FileWritter::save_first() {
	namespace fs = boost::filesystem;
	fs::path filepath = pathRoot_;	// 文件路径
	nfileptr ptr = quenf_.front();
	bool rslt(false);

	filepath /= ptr->subpath;
	ptr->subpath = filepath.string();
	if (!fs::is_directory(filepath) && !fs::create_directory(filepath)) {
		_gLog.Write(LOG_FAULT, "FileWritter::OnNewFile", "failed to create directory<%s>", filepath.c_str());
	}
	else {
		FILE *fp;

		filepath /= ptr->filename;
		if (NULL != (fp = fopen(filepath.c_str(), "wb"))) {
			fwrite(ptr->filedata.get(), 1, ptr->filesize, fp);
			fclose(fp);

			if (dbt_.unique()) {
				ptime tmobs = from_iso_extended_string(ptr->tmobs) + hours(8);
				ptime::time_duration_type tdt = tmobs.time_of_day();
				ptime tmutc(tmobs.date(), hours(tdt.hours()) + minutes(tdt.minutes()) + seconds(tdt.seconds()));

				dbt_->RegImageFile(ptr->cid, ptr->filename, filepath.parent_path().string(),
						to_iso_string(tmutc), tdt.fractional_seconds());
			}
			quenf_.pop_front();
			rslt = true;

			_gLog.Write("Received: %s", ptr->filename.c_str());

			if (tcpc_dp_.use_count()) {// 通知数据处理已经接收到新的文件
				int n;
				apfileinfo proto = boost::make_shared<ascii_proto_fileinfo>();
				proto->gid = ptr->gid;
				proto->uid = ptr->uid;
				proto->cid = ptr->cid;
				proto->subpath = ptr->subpath;
				proto->filename = ptr->filename;
				const char *s = ascproto_->CompactFileInfo(proto, n);
				tcpc_dp_->Write(s, n);
			}
		}
		else {
			_gLog.Write(LOG_FAULT, "FileWritter::save_first", "failed to create file<%s>. %s",
					filepath.c_str(), strerror(errno));
		}
	}

	return rslt;
}
