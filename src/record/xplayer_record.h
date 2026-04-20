#ifndef __XPLAYER_RECORD_H__
#define __XPLAYER_RECORD_H__

#include <cstdbool>
#include <string>
#include <vector>

#include "xplayer_definitions.h"

class CXPlayerRecordInfo
{
public:
	CXPlayerRecordInfo() = default;

	// 文件路径
	std::string _path;

	// 文件名
	std::string _name;

	// 类型
	XPLAYER_RECORD_MODE _mode = XPLAYER_RECORD_NONE;
};

class CXPlayerRecord
{
public:
	CXPlayerRecord() = default;
	~CXPlayerRecord() = default;

	// 加载播放记录文件
	bool loadRecordFile(const std::string & path);
	// 卸载播放记录文件
	void unloadRecordFile();

	// 获取播放列表
	bool getRecordList(std::vector<CXPlayerRecordInfo> & pl);
	// 添加播放记录
	bool addRecord(const CXPlayerRecordInfo & pri);
	// 删除播放记录
	bool delRecord(const CXPlayerRecordInfo & pri);
	// 修改播放记录
	bool updateRecord(const CXPlayerRecordInfo & pri);

	// 获取类型
	XPLAYER_RECORD_MODE getMode() const;

	// 获取错误信息
	const char * err()const;

private:
	// 记录类型
	XPLAYER_RECORD_MODE _mode = XPLAYER_RECORD_NONE;
	// 文件路径
	std::string _path;
	// 错误信息
	std::string _err;

	//
	std::vector<CXPlayerRecordInfo> _lst;
};

#endif
