/*
 * @file GLog.cpp 类GLog的定义文件
 * @version      2.0
 * @date    2016年10月28日
 */

#include <sys/stat.h>
#include <sys/types.h>	// Linux需要
#include <unistd.h>
#include <stdarg.h>
#include <string>
#include <boost/filesystem/path.hpp>
#include <boost/format.hpp>
#include "GLog.h"
#include "globaldef.h"

using std::string;
using namespace boost::posix_time;

GLog::GLog(FILE *out) {
	day_ = -1;
	fd_  = out;
	bytecache_ = 0;
}

GLog::~GLog() {
	if (thrdCycle_.unique()) {
		thrdCycle_->interrupt();
		thrdCycle_->join();
	}
	if (fd_ && fd_ != stdout && fd_ != stderr) fclose(fd_);
}

bool GLog::valid_file(ptime &t) {
	if (fd_ == stdout || fd_ == stderr) return true;
	ptime::date_type date = t.date();
	if (day_ != date.day()) {// 日期变更
		day_ = date.day();
		if (fd_) {// 关闭已打开的日志文件
			fprintf(fd_, "%s continue\n", string(69, '>').c_str());
			fclose(fd_);
			fd_ = NULL;
		}
	}

	if (fd_ == NULL) {
		if (access(gLogDir, F_OK)) mkdir(gLogDir, 0755);	// 创建目录
		if (!access(gLogDir, W_OK | X_OK)) {
			boost::filesystem::path path = gLogDir;
			boost::format fmt("%1%%2%.log");
			fmt % gLogPrefix % to_iso_string(date);
			path /= fmt.str();
			fd_ = fopen(path.string().c_str(), "a+");
			tmlast_ = t;
			bytecache_ = fprintf(fd_, "%s\n", string(79, '-').c_str());

			if (!thrdCycle_.unique()) {
				thrdCycle_.reset(new boost::thread(boost::bind(&GLog::thread_cycle, this)));
			}
		}
	}

	return (fd_ != NULL);
}

void GLog::thread_cycle() {
	boost::chrono::seconds period(1);
	time_duration td;

	while(1) {
		boost::this_thread::sleep_for(period);

		mutex_lock lock(mtx_);
		if (bytecache_ > 0 && (second_clock::local_time() - tmlast_).total_seconds() > 1) {
			fflush(fd_);
			bytecache_ = 0;
		}
	}
}

void GLog::Write(const char* format, ...) {
	if (format == NULL) return;

	mutex_lock lock(mtx_);
	ptime t(second_clock::local_time());

	if (valid_file(t)) {
		// 时间标签
		bytecache_ += fprintf(fd_, "%s >> ", to_simple_string(t.time_of_day()).c_str());
		// 日志描述的格式与内容
		va_list vl;
		va_start(vl, format);
		bytecache_ += vfprintf(fd_, format, vl);
		va_end(vl);
		bytecache_ += fprintf(fd_, "\n");
		tmlast_ = t;
	}
}

void GLog::Write(const LOG_TYPE type, const char* where, const char* format, ...) {
	if (format == NULL) return;

	mutex_lock lock(mtx_);
	ptime t(second_clock::local_time());

	if (valid_file(t)) {
		// 时间标签
		bytecache_ += fprintf(fd_, "%s >> ", to_simple_string(t.time_of_day()).c_str());
		// 日志类型
		if (type == LOG_WARN)       bytecache_ += fprintf(fd_, "WARN: ");
		else if (type == LOG_FAULT) bytecache_ += fprintf(fd_, "ERROR: ");
		// 事件位置
		if (where) bytecache_ += fprintf(fd_, "%s, ", where);
		// 日志描述的格式与内容
		va_list vl;
		va_start(vl, format);
		bytecache_ += vfprintf(fd_, format, vl);
		va_end(vl);
		bytecache_ += fprintf(fd_, "\n");
		tmlast_ = t;
	}
}
