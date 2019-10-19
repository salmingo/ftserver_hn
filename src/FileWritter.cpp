/*
 * @file FileWritter.cpp 文件写盘管理器定义文件
 * @version 0.1
 * @date 2017-10-28
 */

#include <boost/make_shared.hpp>
#include <boost/filesystem.hpp>
#include <stdio.h>
#include "FileWritter.h"
#include "GLog.h"

FileWritePtr make_filewritter() {
	return boost::make_shared<FileWritter>();
}

FileWritter::FileWritter() {
	running_ = false;
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
	if (!enabled) db_.reset();
	else if(url) db_.reset(new DataTransfer((char*) url));
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
	if (!fs::is_directory(filepath) && !fs::create_directory(filepath)) {
		_gLog.Write(LOG_FAULT, "FileWritter::OnNewFile", "failed to create directory<%s>", filepath.c_str());
	}
	else {
		FILE *fp;

		filepath /= ptr->filename;
		if (NULL != (fp = fopen(filepath.c_str(), "wb"))) {
			fwrite(ptr->filedata.get(), 1, ptr->filesize, fp);
			fclose(fp);

			if (db_.unique()) {
				char status[200];
				db_->regOrigImage(ptr->gid.c_str(), ptr->uid.c_str(), ptr->cid.c_str(),
						ptr->grid.c_str(), ptr->field.c_str(), ptr->filename.c_str(),
						filepath.c_str(), ptr->tmobs.c_str(), status);
			}
			quenf_.pop_front();
			rslt = true;

			_gLog.Write("Received: %s", ptr->filename.c_str());
		}
		else {
			_gLog.Write(LOG_FAULT, "FileWritter::save_first", "failed to create file<%s>. %s",
					filepath.c_str(), strerror(errno));
		}
	}

	return rslt;
}
