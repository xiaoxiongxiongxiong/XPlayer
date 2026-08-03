#ifndef __XPLAYER_RECORD_H__
#define __XPLAYER_RECORD_H__

#include <cstdbool>
#include <string>
#include <vector>

#include "xplayer_definitions.h"

struct xplayer_record_info_t
{
	std::string name;
	std::string path;
};

struct xplayer_record_config_t
{
	XPLAYER_RECORD_MODE mode;
    std::vector<xplayer_record_info_t> ris;
};

class CXPlayerRecord
{
public:
	CXPlayerRecord() = default;
	~CXPlayerRecord() = default;

	// 加载播放记录文件
	bool load(const std::string & path);
	// 卸载播放记录文件
	void unload();

	// 添加播放记录
	bool addRecord(const xplayer_record_info_t & ri);
	// 删除播放记录
	bool delRecord(const xplayer_record_info_t & ri);
	// 修改播放记录
	bool updateRecord(const xplayer_record_info_t & ri);
    // 获取播放列表
    bool getRecord(std::vector<xplayer_record_info_t> & ris);

	// 设置类型
	bool setMode(XPLAYER_RECORD_MODE mode);
	// 获取类型
	XPLAYER_RECORD_MODE getMode() const;

	// 获取错误信息
	const char * err()const;

private:
	// 文件路径
	std::string _path;
	// 错误信息
	std::string _err;

	xplayer_record_config_t _ctx{};
};

#endif
