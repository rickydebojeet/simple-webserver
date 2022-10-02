#include "http_server.hh"

/**
 * @brief Split a string into a vector of strings
 *
 * @param s Input String
 * @param delim Delimiter for splitting
 * @return vector<string>
 */
vector<string> split(const string &s, char delim)
{
    vector<string> elems;

    stringstream ss(s);
    string item;

    while (getline(ss, item, delim))
    {
        if (!item.empty())
            elems.push_back(item);
    }

    return elems;
}

/**
 * @brief Construct a new HTTP_Request structure
 *
 * @param request Request string from the client socket
 */
HTTP_Request::HTTP_Request(string request)
{
    vector<string> lines = split(request, '\n');      // Split the request into lines
    vector<string> first_line = split(lines[0], ' '); // Split the first line into words

    this->HTTP_version = "1.0"; // We'll be using 1.0 irrespective of the request

    // Extraction of the request method and URL from first_line
    this->method = first_line[0];
    this->url = first_line[1];

    // Supports only GET requests
    if (this->method != "GET")
    {
        cerr << "Method '" << this->method << "' not supported" << endl;
        exit(1);
    }
}

/**
 * @brief Parses a HTTP request and returns a HTTP_Response structure
 *
 * @param req Request string from the client socket
 * @return HTTP_Response*
 */
string handle_request(string req)
{
    HTTP_Request *request = new HTTP_Request(req);

    HTTP_Response *response = new HTTP_Response();

    string url = string("html_files") + request->url;

    response->HTTP_version = "1.0";

    struct stat sb;
    if (stat(url.c_str(), &sb) == 0) // requested path exists
    {
        response->status_code = "200";
        response->status_text = "OK";
        response->content_type = "text/html";

        // If the requested path is a directory, open index.html
        if (S_ISDIR(sb.st_mode))
        {
            url += "/index.html";
        }

        // Opening the file and reading its contents
        struct stat filestat;
        stat(url.c_str(), &filestat);
        response->content_length = to_string(filestat.st_size);
        char char_array[filestat.st_size + 1];
        int fd = open(url.c_str(), O_RDONLY);
        read(fd, char_array, filestat.st_size);
        char_array[filestat.st_size] = '\0';
        response->body = string(char_array);

        /*
        TODO : set the remaining fields of response appropriately
        */
    }

    else
    {
        response->status_code = "404";
        response->status_text = "Not Found";
        response->content_type = "text/html";

        string errorOutput = string("html_files/404.html");
        struct stat filestat;
        stat(errorOutput.c_str(), &filestat);
        response->content_length = to_string(filestat.st_size);
        char char_array[filestat.st_size + 1];
        int fd = open(errorOutput.c_str(), O_RDONLY);
        read(fd, char_array, filestat.st_size);
        char_array[filestat.st_size] = '\0';
        response->body = string(char_array);
    }

    string response_string = response->get_string();

    delete request;
    delete response;

    return response_string;
}

/**
 * @brief Returns the string representation of the HTTP Response
 *
 * @return string
 */
string HTTP_Response::get_string()
{
    string response = "";
    time_t ltime;
    ltime = time(0);
    string str_time = asctime(gmtime(&ltime));
    str_time.erase(remove(str_time.begin(), str_time.end(), '\n'), str_time.cend());
    this->date = "Date: " + str_time + " GMT";

    response += "HTTP/" + this->HTTP_version + " " + this->status_code + " " + this->status_text + "\r\n";
    response += this->date + "\r\n";
    response += "Content-Type: " + this->content_type + "\r\n";
    response += "Content-Length: " + this->content_length + "\r\n";
    response += "\r\n";
    response += this->body;

    return response;
}
