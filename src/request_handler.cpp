#include "request_handler.h"

namespace http_handler {

std::string DecodeURL(const std::string str) {
    std::string ret;
    char ch;
    int i, ii, len = str.length();

    for (i=0; i < len; i++){
        if(str[i] != '%'){
            if(str[i] == '+')
                ret += ' ';
            else
                ret += str[i];
        }else{
            std::stringstream s_str;
            s_str << str.substr(i + 1, 2);
            s_str >> std::hex >> ii;
            ch = static_cast<char>(ii);
            ret += ch;
            i = i + 2;
        }
    }
    return ret;
}

std::string PrintErrorResponce(std::string code, std::string messege) {
    pt::ptree data;

    data.put("code", code);
    data.put("message", messege);

    return model::PtreeToString(data);
}

fs::path StringToPath(std::string str) {
    return fs::path(str);
}

bool IsSubPath(fs::path path, fs::path base) {
    // Приводим оба пути к каноничному виду (без . и ..)
    path = fs::weakly_canonical(path);
    base = fs::weakly_canonical(base);

    // Проверяем, что все компоненты base содержатся внутри path
    for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
        if (p == path.end() || *p != *b) {
            return false;
        }
    }
    return true;
}

}  // namespace http_handler
