#include "xplayer_record.h"
#include "jansson.h"
#include <filesystem>
#include "xplayer_utils.h"

bool CXPlayerRecord::loadRecordFile(const std::string & path)
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

    json_error_t jso_err = {};
    json_t * jso_root = json_load_file(path.c_str(), 0, &jso_err);
    if (nullptr == jso_root)
    {
        xpu_format_string(_err, "json_loads %s failed, pos: %d, source: %s",
                          path.c_str(), jso_err.position, jso_err.source);
        return false;
    }

    json_t * jso_mode = json_object_get(jso_root, "mode");
    if (nullptr == jso_mode)
    {
        xpu_format_string(_err, "Get key word 'mode' failed");
        json_decref(jso_root);
        return false;
    }

    const auto * mode = json_string_value(jso_mode);
    if (nullptr == mode)
    {
        xpu_format_string(_err, "Get key word 'mode' value failed");
        json_decref(jso_root);
        return false;
    }

    if (0 == strcmp(mode, "vod"))
        _mode = XPLAYER_MODE_VOD;
    else if (0 == strcmp(mode, "live"))
        _mode = XPLAYER_MODE_LIVE;
    else
    {
        xpu_format_string(_err, "Unsupported mode '%s'", mode);
        json_decref(jso_root);
        return false;
    }

    json_t * jso_lst = json_object_get(jso_root, "playlist");
    if (nullptr == jso_lst)
    {
        xpu_format_string(_err, "Get key word 'playlist' failed");
        json_decref(jso_root);
        return false;
    }

    const auto cnt = json_array_size(jso_lst);
    json_t * jso_record = nullptr;
    size_t index = 0ul;
    json_array_foreach(jso_lst, index, jso_record)
    {
        json_t * jso_name = json_object_get(jso_record, "name");
        if (nullptr == jso_name)
        {
            xpu_format_string(_err, "Get key word 'name' failed");
            json_decref(jso_root);
            return false;
        }

        json_t * jso_path = json_object_get(jso_record, "path");
        if (nullptr == jso_path)
        {
            xpu_format_string(_err, "Get key word 'path' failed");
            json_decref(jso_root);
            return false;
        }

        CXPlayerRecordInfo tmp;
        tmp._name = json_string_value(jso_name);
        tmp._path = json_string_value(jso_path);
        tmp._mode = _mode;
        _lst.emplace_back(std::move(tmp));
    }

    json_decref(jso_root);
    _path = path;

    return true;
}

void CXPlayerRecord::unloadRecordFile()
{
    if (_path.empty())
        return;

    std::string mode_str;
    if (XPLAYER_MODE_VOD == _mode)
        mode_str = "vod";
    else if (XPLAYER_MODE_LIVE == _mode)
        mode_str = "live";
    else
        return;

    json_t * jso_root = json_pack("{s: s, s: o}", "mode", mode_str.c_str(), "playlist", json_array());
    if (nullptr == jso_root)
    {
        xpu_format_string(_err, "json_pack failed");
        return;
    }

    json_t * jso_lst = json_object_get(jso_root, "playlist");
    for (const auto & elem : _lst)
    {
        json_t * jso_record = json_pack("{s: s, s: s}", "name", elem._name.c_str(), "path", elem._path.c_str());
        if (nullptr == jso_record)
        {
            json_decref(jso_root);
            return;
        }
        json_array_append_new(jso_lst, jso_record);
    }

    json_dump_file(jso_root, _path.c_str(), JSON_INDENT(4) | JSON_ENSURE_ASCII);
    json_decref(jso_root);
}

bool CXPlayerRecord::getRecordList(std::vector<CXPlayerRecordInfo> & pl)
{
    if (_path.empty())
    {
        xpu_format_string(_err, "No opened file");
        return false;
    }

    pl = _lst;

    return true;
}

bool CXPlayerRecord::addRecord(const CXPlayerRecordInfo & pri)
{
    if (_path.empty())
    {
        xpu_format_string(_err, "No opened file");
        return false;
    }

    if (XPLAYER_MODE_NONE == _mode)
        _mode = pri._mode;
    if (_mode != pri._mode)
    {
        xpu_format_string(_err, "Record mode mismatch for %s", _path.c_str());
        return false;
    }

    const auto & path = pri._path;
    auto found = std::find_if(_lst.begin(), _lst.end(),
                              [&path](const CXPlayerRecordInfo & tmp)
    {
        return path == tmp._path;
    });
    if (_lst.end() != found)
    {
        xpu_format_string(_err, "Repeated path %s", path.c_str());
        return false;
    }

    _lst.emplace_back(pri);

    return true;
}

bool CXPlayerRecord::delRecord(const CXPlayerRecordInfo & pri)
{
    if (_path.empty())
    {
        xpu_format_string(_err, "No opened file");
        return false;
    }

    const auto & path = pri._path;
    auto found = std::find_if(_lst.begin(), _lst.end(),
                              [&path](const CXPlayerRecordInfo & tmp)
    {
        return path == tmp._path;
    });
    if (_lst.end() == found)
    {
        xpu_format_string(_err, "Found record '%s' failed", path.c_str());
        return false;
    }

    _lst.erase(found);

    return true;
}

bool CXPlayerRecord::updateRecord(const CXPlayerRecordInfo & pri)
{
    if (_path.empty())
    {
        xpu_format_string(_err, "No opened file");
        return false;
    }

    const auto & path = pri._path;
    auto found = std::find_if(_lst.begin(), _lst.end(),
                              [&path](const CXPlayerRecordInfo & tmp)
    {
        return path == tmp._path;
    });
    if (_lst.end() == found)
    {
        xpu_format_string(_err, "Found record '%s' failed", path.c_str());
        return false;
    }

    if (XPLAYER_MODE_NONE != pri._mode && (*found)._mode != pri._mode)
    {
        xpu_format_string(_err, "Record '%s' mode changed", path.c_str());
        return false;
    }

    (*found)._name = pri._name;
    (*found)._path = pri._path;

    return true;
}

XPLAYER_MODE CXPlayerRecord::getMode() const
{
    return _mode;
}

const char * CXPlayerRecord::err()const
{
    return _err.c_str();
}
