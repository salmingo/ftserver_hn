/*
 * @file FileReceiver.cpp 文件传输客户端定义文件, 接收网络文件
 * @version 0.1
 * @date 2017-10-29
 */
#include <boost/make_shared.hpp>
#include <boost/format.hpp>
#include <boost/bind/bind.hpp>
#include "FileReceiver.h"
#include "GLog.h"

using namespace boost::placeholders;

FileRcvPtr make_filercv(FileWritePtr ptr) {
	return boost::make_shared<FileReceiver>(ptr);
}

FileReceiver::FileReceiver(FileWritePtr fwptr) {
	fwptr_ = fwptr;
	state_ = WAITING;
	ascproto_ = boost::make_shared<AsciiProtocol>();
	bufrcv_.reset(new char[TCP_PACK_SIZE]);
}

FileReceiver::~FileReceiver() {
	Stop();
}

bool FileReceiver::CoupleNetwork(TcpCPtr client) {
	/* 启动消息机制 */
	boost::format fmt("msgque_%1%-%2%");
	const tcp::endpoint &endpoint = client->GetSocket().remote_endpoint();
	string ip;
	uint16_t port;

	ip = endpoint.address().to_string();
	port = endpoint.port();
	fmt % ip % port;
	register_messages();
	if (!Start(fmt.str().c_str())) {
		_gLog.Write(LOG_FAULT, "FileReceiver::CoupleNetwork",
				"failed to create message queue<%s>", fmt.str().c_str());
		return false;
	}
	/* 关联网络接口 */
	const TCPClient::CBSlot &slot = boost::bind(&FileReceiver::network_receive, this, _1, _2);
	tcpptr_ = client;
	tcpptr_->RegisterRead(slot);

	return true;
}

bool FileReceiver::IsAlive() {
	return (tcpptr_.unique() && tcpptr_->IsOpen());
}

void FileReceiver::network_receive(long client, long ec) {
	if (ec) PostMessage(MSG_NETWORK_CLOSE);
	else if(state_ == READY) {// 接收文件数据
		int n = tcpptr_->Read(bufrcv_.get(), TCP_PACK_SIZE, 0);
		if (fileptr_->DataArrive(bufrcv_.get(), n)) PostMessage(MSG_RECEIVE_COMPLETE);
	}
	else if(state_ == WAITING) PostMessage(MSG_NETWORK_RECEIVE);
}

void FileReceiver::register_messages() {
	const CBSlot &slot1 = boost::bind(&FileReceiver::on_network_receive,  this, _1, _2);
	const CBSlot &slot2 = boost::bind(&FileReceiver::on_network_close,    this, _1, _2);
	const CBSlot &slot3 = boost::bind(&FileReceiver::on_receive_complete, this, _1, _2);

	RegisterMessage(MSG_NETWORK_RECEIVE,  slot1);
	RegisterMessage(MSG_NETWORK_CLOSE,    slot2);
	RegisterMessage(MSG_RECEIVE_COMPLETE, slot3);
}

void FileReceiver::on_network_receive(long param1, long param2) {
	char term[] = "\n";
	int len = strlen(term);
	int pos = tcpptr_->Lookup(term, len, 0);
	apbase base;

	tcpptr_->Read(bufrcv_.get(), pos + len, 0);
	bufrcv_[pos] = 0;
	base = ascproto_->Resolve(bufrcv_.get());
	if (base.unique()) {
		if (base->type == "fileinfo") {
			// 缓存文件信息
			apfileinfo fileinfo = from_apbase<ascii_proto_fileinfo>(base);
			const long n = fileptr_.use_count();
			if (n == 0 || n > 1 || (n == 1 && fileptr_->filesize != fileinfo->filesize))
				fileptr_ = boost::make_shared<FileInfo>(fileinfo->filesize);
			fileptr_->gid      = fileinfo->gid;
			fileptr_->uid      = fileinfo->uid;
			fileptr_->cid      = fileinfo->cid;
			fileptr_->tmobs    = fileinfo->tmobs;
			fileptr_->subpath  = fileinfo->subpath;
			fileptr_->filename = fileinfo->filename;
			fileptr_->rcvsize  = 0;
			// 通知可以接收数据
			notify_status(READY);
		}
		else if (base->type == "filestat") {
			//...心跳机制, 不处理
		}
	}
	else {
		_gLog.Write(LOG_FAULT, NULL, "wrong communication");
		tcpptr_->Close();
	}
}

void FileReceiver::on_network_close(long param1, long param2) {
	tcpptr_.reset();
}

void FileReceiver::on_receive_complete(long param1, long param2) {
	if (fileptr_->filesize == fileptr_->rcvsize) {
		fwptr_->NewFile(fileptr_);
		notify_status(COMPLETE);
	}
	else {
		_gLog.Write(LOG_FAULT, NULL, "bytes received<%d> is greater than file size<%d>",
				fileptr_->rcvsize, fileptr_->filesize);
		notify_status(FAILURE);
	}
	state_ = WAITING;
}

void FileReceiver::notify_status(int status) {
	apfilestat filestat = boost::make_shared<ascii_proto_filestat>();
	const char *tosend;
	int n;

	filestat->status = state_ = status;
	tosend = ascproto_->CompactFileStat(filestat, n);
	tcpptr_->Write(tosend, n);
}
