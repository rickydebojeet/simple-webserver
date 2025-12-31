#include "http_server.hh"

/**
 * @brief split a string into a vector of strings
 *
 * @param s input String
 * @param delim delimiter for splitting
 * @return vector<string>
 */
vector<string> split(const string& s, char delim) {
    vector<string> elems;

    stringstream ss(s);
    string item;

    while (getline(ss, item, delim)) {
        if (!item.empty()) elems.push_back(item);
    }

    return elems;
}

/**
 * @brief construct a new HTTP_Request structure
 *
 * @param request request string from the client socket
 * @return HTTP_Request* structure
 */
HTTP_Request::HTTP_Request(string request) : error_code(0) {
    vector<string> lines = split(request, '\n');
    if (lines.empty()) {
        this->error_code = 400;
        return;
    }

    for (auto& line : lines) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
    }

    if (lines[0].empty()) {
        this->error_code = 400;
        return;
    }
    vector<string> first_line = split(lines[0], ' ');
    if (first_line.size() < 3) {
        this->error_code = 400;
        return;
    }

    this->method = first_line[0];
    this->url = first_line[1];
    string ver = first_line[2];

    if (ver.rfind("HTTP/", 0) == 0) {
        this->HTTP_version = ver.substr(5);
    } else {
        this->error_code = 400;
        return;
    }

    if (this->method != "GET") {
        this->error_code = 405;  // Method Not Allowed
        return;
    }

    for (size_t i = 1; i < lines.size(); ++i) {
        this->header_lines.push_back(lines[i]);
        if (lines[i].compare(0, 6, "Range:") == 0) {
#if SANITY_CHECK
            cout << "Found Range header: " << lines[i] << endl;
#endif
            this->range_header = lines[i];
        }
    }
}

/**
 * @brief construct a new HTTP_Response structure
 *
 * @return HTTP_Response* structure
 */
HTTP_Response::HTTP_Response() {
    this->use_sendfile = false;
    this->file_fd = -1;
    this->offset = 0;
    this->length = 0;
}

// Helper: Content-Type map
string get_content_type(const string& url) {
    size_t pos = url.find_last_of(".");
    if (pos == string::npos) return "text/html";

    string ext = url.substr(pos);
    if (ext == ".html") return "text/html";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "text/javascript";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".png") return "image/png";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".ts") return "video/mp2t";
    if (ext == ".m3u8") return "application/vnd.apple.mpegurl";
    if (ext == ".mp4") return "video/mp4";
    return "text/plain";
}

void make_error_response(HTTP_Response* r, int code, const string& version) {
    if (!r) return;
    r->HTTP_version = version.empty() ? "1.0" : version;
    r->content_type = "text/html";
    r->connection = "close";

    switch (code) {
        case 400:
            r->status_code = "400";
            r->status_text = "Bad Request";
            break;
        case 404:
            r->status_code = "404";
            r->status_text = "Not Found";
            break;
        case 405:
            r->status_code = "405";
            r->status_text = "Method Not Allowed";
            break;
        case 416:
            r->status_code = "416";
            r->status_text = "Range Not Satisfiable";
            break;
        case 500:
            r->status_code = "500";
            r->status_text = "Internal Server Error";
            break;
        default:
            r->status_code = to_string(code);
            r->status_text = "Error";
            break;
    }

    r->body = "<html><body><h1>" + r->status_code + " " + r->status_text +
              "</h1></body></html>";
    r->content_length = to_string(r->body.length());
}

FILE_range parse_range_header(const string& header, off_t file_size) {
    if (header.empty()) return {0, 0, false};

    size_t eq_pos = header.find("=");
    if (eq_pos == string::npos) return {0, 0, false};

    string bytes_range = header.substr(eq_pos + 1);
    size_t dash_pos = bytes_range.find("-");
    if (dash_pos == string::npos) return {0, 0, false};

    string start_str = bytes_range.substr(0, dash_pos);
    string end_str = bytes_range.substr(dash_pos + 1);

    off_t start = 0;
    off_t end = file_size - 1;

    try {
        if (!start_str.empty()) start = stoll(start_str);
        if (!end_str.empty())
            end = stoll(end_str);
        else
            end = file_size - 1;
    } catch (...) {
        return {0, 0, false};
    }

    if (start_str.empty() && !end_str.empty()) {
        start = file_size - end;
        end = file_size - 1;
    }

    if (start < 0) start = 0;
    if (end >= file_size) end = file_size - 1;

    if (start > end) return {0, 0, false};

    return {start, end, true};
}

/**
 * @brief parses a HTTP request and returns a HTTP_Response structure
 *
 * @param req request string from the client socket
 * @return HTTP_Response*
 */
HTTP_Response* handle_request(string req) {
    auto* request = new HTTP_Request(req);
    auto* response = new HTTP_Response();
    response->HTTP_version = request->HTTP_version;

    if (request->error_code != 0) {
        make_error_response(response, request->error_code,
                            request->HTTP_version);
        delete request;
        return response;
    }

    bool connection_close = false;
    bool host_found = false;
    for (const auto& line : request->header_lines) {
        if (line.compare(0, 5, "Host:") == 0) host_found = true;
        if (line.compare(0, 11, "Connection:") == 0 &&
            line.find("close") != string::npos) {
            connection_close = true;
        }
    }

    if (request->HTTP_version == "1.1" && !host_found) {
        make_error_response(response, 400, request->HTTP_version);
        delete request;
        return response;
    }

    response->connection = (connection_close || request->HTTP_version != "1.1")
                               ? "close"
                               : "keep-alive";

    string file_path = "www" + request->url;
    struct stat sb;

    if (stat(file_path.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
        file_path += "/index.html";
        if (stat(file_path.c_str(), &sb) != 0) {
            make_error_response(response, 404, request->HTTP_version);
            delete request;
            return response;
        }
    } else if (stat(file_path.c_str(), &sb) != 0) {
        make_error_response(response, 404, request->HTTP_version);
        delete request;
        return response;
    }

    response->content_type = get_content_type(file_path);

    FILE_range range = {0, sb.st_size - 1, false};
    bool partial = false;
    if (!request->range_header.empty()) {
        FILE_range parsed =
            parse_range_header(request->range_header, sb.st_size);
        if (parsed.valid) {
            range = parsed;
            partial = true;
        } else if (request->range_header.find("bytes=") != string::npos) {
            make_error_response(response, 416, request->HTTP_version);
            delete request;
            return response;
        }
    }

    if (partial) {
        response->status_code = "206";
        response->status_text = "Partial Content";
        response->content_range = "bytes " + to_string(range.start) + "-" +
                                  to_string(range.end) + "/" +
                                  to_string(sb.st_size);
    } else {
        response->status_code = "200";
        response->status_text = "OK";
    }

    response->offset = range.start;
    response->length = range.end - range.start + 1;
    response->content_length = to_string(response->length);

#if USE_SENDFILE
    int fd = open(file_path.c_str(), O_RDONLY);
    if (fd != -1) {
        response->file_fd = fd;
        response->use_sendfile = true;
        delete request;
        return response;
    }
#endif

    int file_fd = open(file_path.c_str(), O_RDONLY);
    if (file_fd != -1) {
        if (lseek(file_fd, range.start, SEEK_SET) != -1) {
            response->body.resize(response->length);
            ssize_t bytes_read =
                read(file_fd, &response->body[0], response->length);
            if (bytes_read >= 0 && (size_t)bytes_read != response->length) {
                response->body.resize(bytes_read);
            } else if (bytes_read < 0) {
                response->body.clear();
            }
        }
        close(file_fd);
    }

    delete request;
    return response;
}

string HTTP_Response::get_string() {
    time_t now = time(0);
    struct tm tm;
    gmtime_r(&now, &tm);
    char buf[128];
    strftime(buf, sizeof(buf), "%a %b %d %H:%M:%S %Y", &tm);
    string str_time = buf;

    string response_str;
    response_str.reserve(512);

    response_str += "HTTP/" + this->HTTP_version + " " + this->status_code +
                    " " + this->status_text + "\r\n";
    response_str += "Date: " + str_time + " GMT\r\n";
    response_str += "Content-Type: " + this->content_type + "\r\n";
    response_str += "Content-Length: " + this->content_length + "\r\n";
    response_str += "Accept-Ranges: bytes\r\n";
    if (!this->content_range.empty())
        response_str += "Content-Range: " + this->content_range + "\r\n";
    if (!this->connection.empty())
        response_str += "Connection: " + this->connection + "\r\n";
    response_str += "\r\n";

    return response_str;
}