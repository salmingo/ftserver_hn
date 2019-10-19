/*!
 * @file parameter.h 使用XML文件格式管理配置参数
 */

#ifndef PARAMETER_H_
#define PARAMETER_H_

#include <string>
#include <vector>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>

using std::string;
using std::vector;

struct param_config {// 软件配置参数
	uint16_t port;	//< 网络服务端口
	bool bNTP;		//< 是否启用NTP
	string ipNTP;	//< NTP主机地址
	int diffNTP;	//< 时钟最大偏差
	bool bDB;		//< 是否启用数据库
	string urlDB;	//< 数据库链接地址
	/* 原始数据存储路径 */
	bool bFreeStorage;	//< 自动清除磁盘空间
	int minDiskStorage;	//< 最小磁盘容量, 量纲: GB. 当小于该值时更换盘区或删除历史数据
	int nStorage;	//< 文件存储盘区数量
	int iStorage;	//< 当前使用盘区索引
	int iOld;		//< 上次使用盘区索引
	vector<string> pathStorage;	//< 文件存储盘区名称列表
	bool bNotifyStorage;			//< 通知已变更磁盘存储空间
	string pathNotifyStorage;	//< 通知文件路径
	/* 模板数据存储路径 */
	bool bFreeTemplate;	//< 自动清除模板空间
	int minDiskTemplate;	//< 最小磁盘容量, 量纲: GB
	int nTemplate;	//< 模板存储盘区数量
	vector<string> pathTemplate;	//< 模板存储盘区名称列表

private:
	string pathxml;	//< 配置文件路径

public:
	virtual ~param_config() {
		if (iStorage != iOld) {
			using boost::property_tree::ptree;

			ptree pt;
			read_xml(pathxml, pt, boost::property_tree::xml_parser::trim_whitespace);

			for (ptree::iterator it = pt.begin(); it != pt.end(); ++it) {
				if (boost::iequals((*it).first, "LocalStorage")) {
					(*it).second.put("DiskIndex.<xmlattr>.Value", iStorage);
					break;
				}
			}
			boost::property_tree::xml_writer_settings<std::string> settings(' ', 4);
			write_xml(pathxml, pt, std::locale(), settings);
		}
	}
	/*!
	 * @brief 初始化文件filepath, 存储缺省配置参数
	 * @param filepath 文件路径
	 */
	void InitFile(const std::string& filepath) {
		pathxml = filepath;

		using namespace boost::posix_time;
		using boost::property_tree::ptree;

		ptree pt;

		pathStorage.clear();
		pt.add("version", "0.2");
		pt.add("date", to_iso_string(second_clock::universal_time()));
		pt.add("Server.<xmlattr>.Port",     port = 4020);
		pt.add("NTP.<xmlattr>.Enable",      bNTP = true);
		pt.add("NTP.<xmlattr>.IP",          ipNTP = "172.28.1.3");
		pt.add("NTP.<xmlattr>.Difference",  diffNTP = 100);
		pt.add("Database.<xmlattr>.Enable", bDB = true);
		pt.add("Database.<xmlattr>.URL",    urlDB = "http://172.28.8.8:8080/gwebend/");

		ptree& node1 = pt.add("LocalStorage", "");
		string path11 = "/data1";
		string path12 = "/data2";
		string path13 = "/data3";
		node1.add("AutoFree.<xmlattr>.Enable", bFreeStorage = true);
		node1.add("AutoFree.<xmlattr>.MinimumCapacity", minDiskStorage = 100);
		node1.add("DiskNumber.<xmlattr>.Value", nStorage = 3);
		node1.add("DiskIndex.<xmlattr>.Value",  iStorage = 0);
		node1.add("PathRoot#1.<xmlattr>.Name", path11);
		node1.add("PathRoot#2.<xmlattr>.Name", path12);
		node1.add("PathRoot#3.<xmlattr>.Name", path13);
		node1.add("Notify.<xmlattr>.Enable", bNotifyStorage = true);
		node1.add("Notify.<xmlattr>.FilePath", pathNotifyStorage = "/data/GWAC/xMatch_Dir/LocalStorage.txt");
		pathStorage.push_back(path11);
		pathStorage.push_back(path12);
		pathStorage.push_back(path13);
		iOld = iStorage;

		ptree& node2 = pt.add("Template", "");
		string path21 = "/data/GWAC/output";
		string path22 = "/data/GWAC/gwacsub";
		node2.add("AutoFree.<xmlattr>.Enable", bFreeTemplate = true);
		node2.add("AutoFree.<xmlattr>.MinimumCapacity", minDiskTemplate = 1000);
		node2.add("Directory.<xmlattr>.Number", nTemplate = 2);
		node2.add("PathRoot#1.<xmlattr>.Name", path21);
		node2.add("PathRoot#2.<xmlattr>.Name", path22);
		pathTemplate.push_back(path21);
		pathTemplate.push_back(path22);

		boost::property_tree::xml_writer_settings<std::string> settings(' ', 4);
		write_xml(filepath, pt, std::locale(), settings);
	}

	/*!
	 * @brief 从文件filepath加载配置参数
	 * @param filepath 文件路径
	 */
	void LoadFile(const std::string& filepath) {
		pathxml = filepath;

		try {
			using boost::property_tree::ptree;

			ptree pt;
			pathStorage.clear();
			read_xml(filepath, pt, boost::property_tree::xml_parser::trim_whitespace);

			BOOST_FOREACH(ptree::value_type const &child, pt.get_child("")) {
				if (boost::iequals(child.first, "Server")) {
					port = child.second.get("Server.<xmlattr>.Port", 4020);
				}
				else if (boost::iequals(child.first, "NTP")) {
					bNTP    = child.second.get("NTP.<xmlattr>.Enable",     true);
					ipNTP   = child.second.get("NTP.<xmlattr>.IP",         "172.28.1.3");
					diffNTP = child.second.get("NTP.<xmlattr>.Difference", 100);
				}
				else if (boost::iequals(child.first, "Database")) {
					bDB   = child.second.get("Database.<xmlattr>.Enable", true);
					urlDB = child.second.get("Database.<xmlattr>.URL",    "http://172.28.8.8:8080/gwebend/");
				}
				else if (boost::iequals(child.first, "LocalStorage")) {
					boost::format fmt("PathRoot#%d.<xmlattr>.Name");
					bFreeStorage   = child.second.get("AutoFree.<xmlattr>.Enable", true);
					minDiskStorage = child.second.get("AutoFree.<xmlattr>.MinimumCapacity", 100);
					nStorage = child.second.get("DiskNumber.<xmlattr>.Value", 1);
					iStorage = child.second.get("DiskIndex.<xmlattr>.Value",  0);
					iOld     = iStorage;
					for (int i = 1; i <= nStorage; ++i) {
						fmt % i;
						string path = child.second.get(fmt.str(), "/data");
						pathStorage.push_back(path);
					}
					bNotifyStorage = child.second.get("Notify.<xmlattr>.Enable", true);
					pathNotifyStorage = child.second.get("Notify.<xmlattr>.FilePath", "/data/GWAC/xMatch_Dir/LocalStorage.txt");
				}
				else if (boost::iequals(child.first, "Template")) {
					boost::format fmt("PathRoot#%d.<xmlattr>.Name");
					bFreeTemplate   = child.second.get("AutoFree.<xmlattr>.Enable", true);
					minDiskTemplate = child.second.get("AutoFree.<xmlattr>.MinimumCapacity", 1000);
					nTemplate = child.second.get("Directory.<xmlattr>.Number", 1);
					for (int i = 1; i <= nTemplate; ++i) {
						fmt % i;
						string path = child.second.get(fmt.str(), "/data");
						pathTemplate.push_back(path);
					}
				}
			}
		}
		catch(boost::property_tree::xml_parser_error& ex) {
			InitFile(filepath);
		}
	}
};

#endif // PARAMETER_H_
