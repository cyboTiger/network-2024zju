#include "utils.hpp"
#define MAX_CLIENT_QUEUE 50
#define USERNAME "aaa"
#define PASSWORD "bbb"
std::map<std::string, std::string> url2path =
{
    std::make_pair("/index.html","/html/test.html"),
    std::make_pair("/index_noimg.html","/html/noimg.html"),
    std::make_pair("/info/server","/txt/test.txt"),
    std::make_pair("/assets/logo.jpg","/img/logo.jpg")
};
std::string case_conversion(std::string str) {
    std::string lower_case_str = str;
    for (int i = 0 ; i < lower_case_str.size(); i++) {
        if (lower_case_str[i] >= 'A' && lower_case_str[i] <= 'Z') {
            lower_case_str[i] += 'a' - 'A';
        }
    }
    return lower_case_str;
}

class HTTPRequest {
private: // Necessary data elements
    
    // request header
    std::map<std::string, std::string> field_map;
    size_t content_length;
    std::string content_type;
    std::string content;
public:
    // request line
    std::string method;
    std::string url;
    std::string http_version;
    HTTPRequest(std::string& is) : content_length(0) {
        std::stringstream ss(is);
        std::string line;
        
        std::getline(ss, method, ' ');
        std::getline(ss, url, ' ');
        std::getline(ss, http_version);

        std::getline(ss, line);
        while(line != "\r") {
            std::stringstream ss2(line);
            std::string key, value;
            std::getline(ss2, key, ':');
            std::getline(ss2, value);
            
            std::string lower_case_key = case_conversion(key);
            if(lower_case_key == "content-length" 
            || lower_case_key == "content-type") {
                field_map[lower_case_key] = value;
            } else {
                field_map[key] = value;
            }
            std::getline(ss, line);
        }
        update_content_attr();

        char buf[content_length+1];
        ss.read(buf, content_length);
        buf[content_length] = '\0';
        content = buf;
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
    std::string get_req_line() {
        return method + " " + url + " " + http_version;
    }
    std::string get_req_header() {
        std::stringstream ss;
        for(auto& pair : field_map) {
            ss << pair.first << ": " << pair.second << "\r\n";
        }
        ss << "\r\n";
        return ss.str();
    }

    void update_content_attr() {
        if(field_map.find("content-length") != field_map.end())
            content_length = std::stoi(field_map["content-length"]);

        if(field_map.find("content-type") != field_map.end())
            content_type = field_map["content-type"];
    }
        
    std::string operator[](const std::string& key) const {
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
    // test3: web echo
    HTTPResponse(std::string& version, int code, std::string& reasonPhrase, 
    std::string content_type, size_t content_length, std::string& content) 
    : version(version), code(code), reasonPhrase(reasonPhrase) {
        field_map["Content-Type"] = content_type;
        field_map["Content-Length"] = std::to_string(content_length);
        content = content;
    }

    // test4: path echo
    HTTPResponse(std::string& version, HTTPRequest& req) 
    : version(version), reasonPhrase(reasonPhrase) {
        field_map["Content-Type"] = "text/plain";
        if(req.method == "GET") {
            if(url2path.find(req.url) != url2path.end()) {
                content = url2path[req.url];
                code = 200;
                reasonPhrase = "OK";
            } else {
                code = 404;
                reasonPhrase = "Not Found";
            }
        } else if(req.method == "POST"){
            if(req.url == "/dopost") {
                code = 200;
                reasonPhrase = "OK";
            } else {
                code = 404;
                reasonPhrase = "Not Found";
            }
        }
        field_map["Content-Length"] = std::to_string(content.size()); 
    }

    // test5/6: normal response
    HTTPResponse(std::string& version, int code, std::string& reasonPhrase, 
    HTTPRequest& req) 
    : version(version), code(code), reasonPhrase(reasonPhrase) {

        if(req.method == "GET") {
            if(url2path.find(req.url) != url2path.end()) {
                std::string path = url2path[req.url];
                std::string resource_type = path.substr(1, path.find_first_of('/', 1));
                std::string relative_path = path.substr(1);

                if(resource_type == "txt/")
                    field_map["Content-Type"] = "text/plain";
                else if(resource_type == "html/")
                    field_map["Content-Type"] = "text/html";
                else if(resource_type == "img/")
                    field_map["Content-Type"] = "image/jpeg";
                else 
                    field_map["Content-Type"] = "text/plain";
            
                std::ifstream file;
                file.open(relative_path, std::ios::in);
                if(file.is_open()) {
                    std::stringstream ss;
                    ss << file.rdbuf();
                    content = ss.str();
                }
            } else {
                code = 404;
                reasonPhrase = "Not Found";
            }

            field_map["Content-Length"] = std::to_string(content.size());

            if(field_map["Content-Type"] != "image/jpeg")
                std::cout << "[Serialize]" << std::endl << serialize() << std::endl;
        } else if(req.method == "POST") {
            // login=aaa&pass=bbb
            if(req.url == "/dopost") {
                field_map["Content-Type"] = "text/html";
                std::string req_content = req.get_content_in_string();
                std::string username = req_content.substr(req_content.find("login=") + 6, req_content.find("&") - req_content.find("login=") - 6);
                std::string password = req_content.substr(req_content.find("pass=") + 5);

                if(username == USERNAME && password == PASSWORD) {
                    content = "<html><body>login success</body></html>";
                } else {
                    content = "<html><body>login failed</body></html>";
                    code = 401;
                    reasonPhrase = "Unauthorized";
                }
                field_map["Content-Length"] = std::to_string(content.size());
            }
        }
        
    }

    // empty content
    HTTPResponse(std::string& version, int code, std::string& reasonPhrase) 
    : version(version), code(code), reasonPhrase(reasonPhrase)
    {}
    std::string serialize() const {
        std::stringstream ss;
        ss << version << " " << code << " " << reasonPhrase << "\r\n";
        for(auto& pair : field_map) {
            ss << pair.first << ": " << pair.second << "\r\n";
        }
        ss << "\r\n";
        ss << content << "\r\n";
        return ss.str();
    }
    std::string operator[](const std::string& key) { 
        if(field_map.find(key) != field_map.end()) {
            return field_map[key];
        } else {
            std::string null_str;
            return null_str;
        }
    }  // Optional - Reload [] for easier header fields access & modification ?

};