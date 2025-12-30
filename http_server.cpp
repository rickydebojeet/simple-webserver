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
HTTP_Request::HTTP_Request(string request) {
    vector<string> lines = split(request, '\n');
    if (lines.empty()) {
        this->error_code = 400;
        return;
    }

    vector<string> first_line = split(lines[0], ' ');
    if (first_line.size() < 2) {
        this->error_code = 400;
        return;
    }

    if (first_line.size() == 3) {
        string ver = first_line[2];
        if (!ver.empty() && ver.back() == '\r') ver.pop_back();

        if (ver.rfind("HTTP/", 0) == 0) {
            this->HTTP_version = ver.substr(5);
        } else {
            this->error_code = 400;
            return;
        }
    } else {
        this->error_code = 400;
        return;
    }

    // extraction of the request method and URL from first_line
    this->method = first_line[0];
    this->url = first_line[1];

    // Store headers
    for (size_t i = 1; i < lines.size(); ++i) {
        if (!lines[i].empty()) {
            this->header_lines.push_back(lines[i]);
            if (lines[i].find("Range:") != string::npos) {
                this->range_header = lines[i];
            }
        }
    }

    if (this->method != "GET") this->error_code = 405;  // Method Not Allowed
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

bool should_use_sendfile(const string& path) {
    if (path.find("www/media/") == 0) return true;
    return false;
}

/**
 * @brief create a simple HTTP error response
 *
 * @param code numeric HTTP status code (e.g., 400)
 * @param version HTTP version string (e.g., "1.1")
 * @return HTTP_Response* newly allocated response (caller owns)
 */
void make_error_response(HTTP_Response* r, int code, const string& version) {
    if (!r) return;
    r->HTTP_version = version.empty() ? string("1.0") : version;
    r->content_type = "text/html";
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
        case 505:
            r->status_code = "505";
            r->status_text = "HTTP Version Not Supported";
            break;
        default:
            r->status_code = to_string(code);
            r->status_text = "Error";
            break;
    }
    string body =
        "<html><body><h1>" + r->status_code + " " + r->status_text + "</h1>";
    body += "</body></html>";
    r->body = body;
    r->content_length = to_string(r->body.length());
    r->connection = "close";
}

/**
 * @brief parses a HTTP request and returns a HTTP_Response structure
 *
 * @param req request string from the client socket
 * @return HTTP_Response*
 */
HTTP_Response* handle_request(string req) {
    HTTP_Request* request = new HTTP_Request(req);

    HTTP_Response* response = new HTTP_Response();

    if (request->error_code != 0) {
        make_error_response(response, request->error_code,
                            request->HTTP_version);
        delete request;
        return response;
    }

    string url = string("www") + request->url;

    response->HTTP_version = request->HTTP_version;

    bool host_found = false;
    bool connection_close = false;

    for (const string& line : request->header_lines) {
        if (line.compare(0, 5, "Host:") == 0) {
            host_found = true;
        }
        if (line.compare(0, 11, "Connection:") == 0) {
            if (line.find("close") != string::npos) {
                connection_close = true;
            }
        }
    }

    if (request->HTTP_version == "1.1" && !host_found) {
        make_error_response(response, 400, request->HTTP_version);
        delete request;
        return response;
    }

    if (request->HTTP_version == "1.1") {
        response->connection = connection_close ? "close" : "keep-alive";
    } else {
        response->connection = "close";
    }

    struct stat sb;
    if (stat(url.c_str(), &sb) == 0)  // requested path exists
    {
        if (S_ISDIR(sb.st_mode)) {
            url += "/index.html";
            stat(url.c_str(), &sb);  // Stat the index file
        }

        // Only use sendfile if path matches criteria
        if (should_use_sendfile(url)) {
            response->file_fd = open(url.c_str(), O_RDONLY);
            if (response->file_fd != -1) {
                response->use_sendfile = true;

                // Range Support
                off_t start = 0;
                off_t end = sb.st_size - 1;
                bool partial = false;

                if (!request->range_header.empty()) {
                    size_t eq_pos = request->range_header.find("=");
                    if (eq_pos != string::npos) {
                        string bytes_range =
                            request->range_header.substr(eq_pos + 1);
                        size_t dash_pos = bytes_range.find("-");
                        if (dash_pos != string::npos) {
                            string start_str = bytes_range.substr(0, dash_pos);
                            string end_str = bytes_range.substr(dash_pos + 1);
                            bool parsed_ok = true;
                            try {
                                if (!start_str.empty())
                                    start = stoll(start_str);
                                if (!end_str.empty()) end = stoll(end_str);
                            } catch (const std::invalid_argument&) {
                                parsed_ok = false;
                            } catch (const std::out_of_range&) {
                                parsed_ok = false;
                            }
                            if (parsed_ok) partial = true;
                        }
                    }
                }

                if (end >= sb.st_size) end = sb.st_size - 1;
                if (start > end) {
                    // 416 Range Not Satisfiable
                    response->status_code = "416";
                    response->status_text = "Range Not Satisfiable";
                    partial = false;
                } else {
                    response->offset = start;
                    response->length = end - start + 1;
                    if (partial) {
                        response->status_code = "206";
                        response->status_text = "Partial Content";
                        response->content_range = "bytes " + to_string(start) +
                                                  "-" + to_string(end) + "/" +
                                                  to_string(sb.st_size);
                    } else {
                        response->status_code = "200";
                        response->status_text = "OK";
                    }
                }
                response->content_length = to_string(response->length);
            }
        }

        // Fallback: If sendfile not used (or not enabled for this file), use
        // standard read
        if (!response->use_sendfile) {
            response->status_code = "200";
            response->status_text = "OK";
            struct stat filestat;
            stat(url.c_str(), &filestat);
            response->content_length = to_string(filestat.st_size);
            // Warning: Reading large files into memory here is bad, but this
            // path is now for small non-media files
            char* char_array = new char[filestat.st_size + 1];
            int fd = open(url.c_str(), O_RDONLY);
            read(fd, char_array, filestat.st_size);
            char_array[filestat.st_size] = '\0';
            response->body = string(char_array);
            delete[] char_array;
            close(fd);
        }

        // Content Type logic (shared)
        // Determine content type
        string extension = "";
        size_t pos = url.find_last_of(".");
        if (pos != string::npos) {
            extension = url.substr(pos);
        }

        if (extension == ".css") {
            response->content_type = "text/css";
        } else if (extension == ".js") {
            response->content_type = "text/javascript";
        } else if (extension == ".jpg" || extension == ".jpeg") {
            response->content_type = "image/jpeg";
        } else if (extension == ".png") {
            response->content_type = "image/png";
        } else if (extension == ".svg") {
            response->content_type = "image/svg+xml";
        } else if (extension == ".ts") {
            response->content_type = "video/mp2t";
        } else if (extension == ".m3u8") {
            response->content_type = "application/vnd.apple.mpegurl";
        } else {
            response->content_type = "text/html";
        }

    } else {
        make_error_response(response, 404, request->HTTP_version);
    }

    // NOTE: Logic for non-sendfile (legacy) buffering needed if USE_SENDFILE is
    // off or file open failed For brevity in this diff, assuming SUCCESSFUL
    // SENDFILE or 404. If USE_SENDFILE is 1, and Open succeeded, we are good.
    // We need to handle directory case where index.html is used.

    delete request;
    return response;
}

/**
 * @brief returns the string representation of the HTTP Response
 *
 * @return string
 */
string HTTP_Response::get_string() {
    string response = "";
    time_t ltime;
    ltime = time(0);
    string str_time = asctime(gmtime(&ltime));
    str_time.erase(remove(str_time.begin(), str_time.end(), '\n'),
                   str_time.cend());
    this->date = "Date: " + str_time + " GMT";

    response += "HTTP/" + this->HTTP_version + " " + this->status_code + " " +
                this->status_text + "\r\n";
    response += this->date + "\r\n";
    response += "Content-Type: " + this->content_type + "\r\n";
    response += "Content-Length: " + this->content_length + "\r\n";
    // Advertise byte-range support which HLS players often expect
    response += "Accept-Ranges: bytes\r\n";
    if (!this->content_range.empty()) {
        response += "Content-Range: " + this->content_range + "\r\n";
    }
    if (!this->connection.empty()) {
        response += "Connection: " + this->connection + "\r\n";
    }
    response += "\r\n";
    response += this->body;

    return response;
}