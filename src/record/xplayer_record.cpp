#include "xplayer_record.h"
#include <filesystem>
#include <fstream>
#include <iostream>

#include "nlohmann/json.hpp"

#include "xplayer_utils.h"

static inline void from_json(const nlohmann::ordered_json & j, xplayer_record_info_t & p)
{
    j.at("name").get_to(p.name);
    j.at("path").get_to(p.path);
}

static inline void to_json(nlohmann::ordered_json & j, const xplayer_record_info_t & p)
{
    j = nlohmann::ordered_json{ {"name", p.name}, {"path", p.path} };
}

static inline void from_json(const nlohmann::ordered_json & j, xplayer_record_config_t & c)
{
    std::string mode;
    j.at("mode").get_to(mode);
    if ("vod" == mode)
        c.mode = XPLAYER_RECORD_VOD;
    else if ("live" == mode)
        c.mode = XPLAYER_RECORD_LIVE;
    else
        c.mode = XPLAYER_RECORD_NONE;
    j.at("playlist").get_to(c.ris);
}

static inline void to_json(nlohmann::ordered_json & j, const xplayer_record_config_t & c)
{
    std::string mode;
    if (XPLAYER_RECORD_VOD == c.mode)
        mode = "vod";
    else if (XPLAYER_RECORD_LIVE == c.mode)
        mode = "live";
    else
        mode = "none";
    j = nlohmann::ordered_json{ {"mode", mode}, {"playlist", c.ris} };
}

bool CXPlayerRecord::load(const std::string & path)
{
    if (!std::filesystem::exists(path))
    {
        std::filesystem::path src = path;
        auto dir_path = src.parent_path();
        auto tmp = std::filesystem::u8path(dir_path.u8string());
        if (!std::filesystem::exists(tmp) && !std::filesystem::create_directory(tmp))
        {
            xpu_format_string(_err, "Create directory '%s' failed", dir_path.c_str());
            return false;
        }

        _path = path;
        return true;
    }

    std::ifstream ifs(path, std::ios::in | std::ios::binary);
    if (!ifs.is_open())
    {
        xpu_format_string(_err, "Open %s failed", path.c_str());
        return false;
    }

    try
    {
        nlohmann::ordered_json body = nlohmann::ordered_json::parse(ifs);
        _ctx = body.get<xplayer_record_config_t>();
    }
    catch (const nlohmann::ordered_json::parse_error & e)
    {
        xpu_format_string(_err, "JSON 格式错误: %s", e.what());
        return false;
    }
    catch (const nlohmann::ordered_json::type_error & e)
    {
        xpu_format_string(_err, "JSON 类型错误: %s", e.what());
        return false;
    }
    catch (const nlohmann::ordered_json::out_of_range & e)
    {
        xpu_format_string(_err, "JSON 字段缺失: %s", e.what());
        return false;
    }

    _path = path;

    return true;
}

void CXPlayerRecord::unload()
{
    if (_path.empty())
        return;

    nlohmann::ordered_json body = _ctx;

    std::ofstream ofs(_path, std::ios::out | std::ios::binary);
    if (!ofs.is_open())
    {
        xpu_format_string(_err, "Open file %s failed", _path.c_str());
        return;
    }

    ofs << body.dump(4, ' ', false, nlohmann::ordered_json::error_handler_t::replace);

    if (!ofs.good())
    {
        xpu_format_string(_err, "Write to file %s failed", _path.c_str());
        return;
    }

    ofs.close();
}

bool CXPlayerRecord::addRecord(const xplayer_record_info_t & ri)
{
    if (_path.empty())
    {
        xpu_format_string(_err, "No opened file");
        return false;
    }

    const auto & path = ri.path;
    auto found = std::find_if(_ctx.ris.begin(), _ctx.ris.end(),
                              [&path](const xplayer_record_info_t & tmp)
    {
        return path == tmp.path;
    });
    if (_ctx.ris.end() != found)
    {
        xpu_format_string(_err, "Repeated path %s", path.c_str());
        return false;
    }

    _ctx.ris.emplace_back(ri);

    return true;
}

bool CXPlayerRecord::delRecord(const xplayer_record_info_t & ri)
{
    if (_path.empty())
    {
        xpu_format_string(_err, "No opened file");
        return false;
    }

    const auto & path = ri.path;
    auto found = std::find_if(_ctx.ris.begin(), _ctx.ris.end(),
                              [&path](const xplayer_record_info_t & tmp)
    {
        return path == tmp.path;
    });
    if (_ctx.ris.end() == found)
    {
        xpu_format_string(_err, "Found record '%s' failed", path.c_str());
        return false;
    }

    _ctx.ris.erase(found);

    return true;
}

bool CXPlayerRecord::updateRecord(const xplayer_record_info_t & ri)
{
    if (_path.empty())
    {
        xpu_format_string(_err, "No opened file");
        return false;
    }

    const auto & path = ri.path;
    auto found = std::find_if(_ctx.ris.begin(), _ctx.ris.end(),
                              [&path](const xplayer_record_info_t & tmp)
    {
        return path == tmp.path;
    });
    if (_ctx.ris.end() == found)
    {
        xpu_format_string(_err, "Found record '%s' failed", path.c_str());
        return false;
    }

    (*found).name = ri.name;
    (*found).path = ri.path;

    return true;
}

bool CXPlayerRecord::getRecord(std::vector<xplayer_record_info_t> & ris)
{
    if (_path.empty())
    {
        xpu_format_string(_err, "No opened file");
        return false;
    }

    ris = _ctx.ris;

    return true;
}

bool CXPlayerRecord::setMode(XPLAYER_RECORD_MODE mode)
{
    if (mode <= XPLAYER_RECORD_NONE || mode >= XPLAYER_RECORD_MAX)
    {
        xpu_format_string(_err, "Unsupported mode: %d", mode);
        return false;
    }

    _ctx.mode = mode;

    return true;
}

XPLAYER_RECORD_MODE CXPlayerRecord::getMode() const
{
    return _ctx.mode;
}

const char * CXPlayerRecord::err()const
{
    return _err.c_str();
}
