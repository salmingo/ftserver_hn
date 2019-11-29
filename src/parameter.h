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
	uint16_t portFS;	//< 网络服务端口: 文件服务
	uint16_t portDP;	//< 网络服务端口: 数据处理
	bool bRepeat;		//< 启用转发
	string hostRepeat;	//< 转发服务器IP地址
	bool bNTP;		//< 是否启用NTP
	string ipNTP;	//< NTP主机地址
	int diffNTP;	//< 时钟最大偏差
	bool bDB;		//< 是否启用数据库
	string urlDB;	//< 数据库链接地址
	/* 原始数据存储路径 */
	bool bFreeStorage;	//< 自动清除磁盘空间
	int minDiskStorage;	//< 最小磁盘容量, 量纲: GB. 当小于该值时更换盘区或删除历史数据
	string pathStorage;	//< 文件存储盘区名称列表

private:
	string pathxml;	//< 配置文件路径

public:
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

		pt.add("Server.<xmlattr>.PortFS",    4020);
		pt.add("Server.<xmlattr>.PortDP",    4021);
		pt.add("Repeat.<xmlattr>.Enable",    false);
		pt.add("Repeat.Server.<xmlattr>.IP", "172.28.9.14");
		pt.add("NTP.<xmlattr>.Enable",       true);
		pt.add("NTP.<xmlattr>.IP",           "172.28.9.251");
		pt.add("NTP.<xmlattr>.Difference",   100);
		pt.add("Database.<xmlattr>.Enable",  false);
		pt.add("Database.<xmlattr>.URL",     "http://172.28.9.14:8080/gwebend/");

		ptree& node1 = pt.add("LocalStorage", "");
		node1.add("AutoFree.<xmlattr>.Enable",          true);
		node1.add("AutoFree.<xmlattr>.MinimumCapacity", 500);
		node1.add("PathRoot.<xmlattr>.Name",            "/data");

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
					portFS = child.second.get("<xmlattr>.PortFS", 4020);
					portDP = child.second.get("<xmlattr>.PortDP", 4021);
				}
				else if (boost::iequals(child.first, "Repeat")) {
					bRepeat    = child.second.get("<xmlattr>.Enable",    false);
					hostRepeat = child.second.get("Server.<xmlattr>.IP", "172.28.9.14");
				}
				else if (boost::iequals(child.first, "NTP")) {
					bNTP    = child.second.get("<xmlattr>.Enable",     true);
					ipNTP   = child.second.get("<xmlattr>.IP",         "172.28.9.251");
					diffNTP = child.second.get("<xmlattr>.Difference", 500);
				}
				else if (boost::iequals(child.first, "Database")) {
					bDB   = child.second.get("<xmlattr>.Enable", false);
					urlDB = child.second.get("<xmlattr>.URL",    "http://172.28.8.8:8080/gwebend/");
				}
				else if (boost::iequals(child.first, "LocalStorage")) {
					bFreeStorage   = child.second.get("AutoFree.<xmlattr>.Enable", true);
					minDiskStorage = child.second.get("AutoFree.<xmlattr>.MinimumCapacity", 500);
					pathStorage    = child.second.get("PathRoot.<xmlattr>.Name", "/data");
				}
			}
		}
		catch(boost::property_tree::xml_parser_error& ex) {
			InitFile(filepath);
		}
	}
};

#endif // PARAMETER_H_
