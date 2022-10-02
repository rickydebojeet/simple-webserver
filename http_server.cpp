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

    /*
     TODO : extract the request method and URL from first_line here
    */

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
HTTP_Response *handle_request(string req)
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

        string body;

        if (S_ISDIR(sb.st_mode))
        {
            /*
            In this case, requested path is a directory.
            TODO : find the index.html file in that directory (modify the url
            accordingly)
            */
        }

        /*
        TODO : open the file and read its contents
        */

        /*
        TODO : set the remaining fields of response appropriately
        */
    }

    else
    {
        response->status_code = "404";

        /*
        TODO : set the remaining fields of response appropriately
        */
    }

    delete request;

    return response;
}

string HTTP_Response::get_string()
{
    /*
    TODO : implement this function
    */

    return "";
}
