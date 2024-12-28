#include "utils.hpp"
#define MAX_CLIENT_QUEUE 20
void case_conversion(std::string& str) {
    for (auto& c : str) {
        if (c >= 'A' && c <= 'Z') {
            c += 'a' - 'A';
        }
    }
}

class HTTPRequest {
private: // Necessary data elements
    // request line
    std::string method;
    std::string req_file;
    std::string http_version;
    // request header
    std::map<std::string, std::string> field_map;
    size_t content_length;
    std::string content_type;
    std::string content;
public:
    HTTPRequest(std::string& is) : content_length(0) {
        std::stringstream ss(is);
        std::string line;
        std::getline(ss, line);
        std::getline(ss, method, ' ');
        std::getline(ss, req_file, ' ');
        std::getline(ss, http_version);

        std::getline(ss, line);
        while(line != "\r") {
            std::stringstream ss2(line);
            std::string key, value;
            std::getline(ss2, key, ':');
            std::getline(ss2, value);
            case_conversion(key);
            if(key == "content-length" || key == "content-type") {
                field_map[key] = value;
            }
            std::getline(ss, line);
        }
        update_content_attr();
        std::getline(ss, content); 
    }
    const std::string& get_content_in_string() const {
        return content;
    }
    const size_t get_content_length() const {
        return content_length;
    }
    const std::string& get_content_type() const {
        return content_type;
    }
    void update_content_attr() {
        if(field_map.find("content-length") != field_map.end())
            content_length = std::stoi(field_map["content-length"]);

        if(field_map.find("content-type") != field_map.end())
            content_type = field_map["content-type"];
    }
        
    const std::string& operator[](const std::string& key) const {
        if(field_map.find(key) != field_map.end()) {
            return field_map.at(key);
        } else {
            std::string null_str("");
            return null_str;
        }
    }
    
};
class HTTPResponse {
private: // Necessary data elements
    std::string version;
    int code;
    std::string reasonPhrase;
    std::map<std::string, std::string> field_map;
    std::string content;
public:
    HTTPResponse(std::string& version, int code, std::string& reasonPhrase, HTTPRequest& req) 
    : version(version), code(code), reasonPhrase(reasonPhrase) {
        field_map["Content-Type"] = req.get_content_type();
        field_map["Content-Length"] = std::to_string(req.get_content_length());
        content = req.get_content_in_string();
    }
    std::string serialize() const {
        std::stringstream ss;
        ss << version << " " << code << " " << reasonPhrase << "\r\n";
        for(auto& pair : field_map) {
            ss << pair.first << ": " << pair.second << "\r\n";
        }
        ss << "\r\n";
        ss << content;
        return ss.str();
    }
    std::string& operator[](const std::string& key) { 
        if(field_map.find(key) != field_map.end()) {
            return field_map[key];
        } else {
            std::string null_str;
            return null_str;
        }
    }  // Optional - Reload [] for easier header fields access & modification ?

};