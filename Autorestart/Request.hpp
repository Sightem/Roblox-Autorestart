#pragma once
#include <string>
#include <map>
#include <sstream>
#define CURL_STATICLIB
#include "curl/curl.h"

typedef std::map<std::string, std::string> headers_t;
typedef headers_t cookies_t;

static size_t _m_writeFunction(void* ptr, size_t size, size_t nmemb, std::string* data)
{
    data->append((char*)ptr, size * nmemb);
    return size * nmemb;
}

struct Response
{
    CURLcode code;
    std::string data;
    headers_t headers;
};

class Request
{

public:
    Request(std::string url) : url(url), data(""), headers() {}

    Request(std::string url, std::string data) : url(url), data(data), headers() {}

    Request(std::string url, std::string data, headers_t headers) : url(url), data(data), headers(headers) {}

    ~Request()
    {
        if (curl_headers)
            curl_slist_free_all(curl_headers);
        if (curl)
            curl_easy_cleanup(curl);
    }

    /**
     * @brief initalize the curl backend, must be called before any other method
     */
    int initalize()
    {
        CURL* localCurl = curl_easy_init();
        this->curl = localCurl;
        if (!this->curl)
        {
            return 1;
        }
        return 0;
    }
    /**
     * @brief execute the request with the method POST
     * @return the response of the request
     */
    Response post()
    {
        this->prepare();
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        return this->execute();
    }
    /**
     * @brief execute the request with the method GET
     * @return the response of the request
     */
    Response get()
    {
        this->prepare();
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
        return this->execute();
    }
    /**
     * @brief execute the request with the method defined (must be a valid HTTP 1.1 method)
     * @param method the method to use
     * @return the response of the request
     */
    Response request(std::string method)
    {
        this->prepare();
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
        return this->execute();
    }
    /**
     * @brief set the url of the request
     * @param url the url of the request
     */
    void set_url(std::string url)
    {
        this->url = url;
    }
    /**
     * @brief set the data of the request
     * @param data the data of the request
     */
    void set_data(std::string data)
    {
        this->data = data;
    }
    /**
     * @brief set a header in the request
     * @param key the key of the header
     * @param value the value of the header
     */
    void set_header(std::string key, std::string value)
    {
        this->headers[key] = value;
    }
    /**
     * @brief set a cookie in the request
     * @param key the key of the cookie
     * @param value the value of the cookie
     */
    void set_cookie(std::string key, std::string value)
    {
        this->cookies[key] = value;
    }
    /**
     * @brief remove a header from the request
     * @param key the key of the header
     */
    void remove_header(std::string key)
    {
        this->headers.erase(key);
    }
    /**
     * @brief remove a cookie from the request
     * @param key the key of the cookie
     */
    void remove_cookie(std::string key)
    {
        this->cookies.erase(key);
    }
    /**
     * @brief return a modifiable map of the headers of the request
     * @return the map of the headers
     */
    const headers_t& get_headers() const
    {
        return headers;
    }
    /**
     * @brief return a modifiable map of the cookies of the request
     * @return the map of the cookies
     */
    const cookies_t& get_cookies() const
    {
        return cookies;
    }
    /**
     * @brief return the url of the request
     * @return the url
     */
    std::string get_url() const
    {
        return url;
    }
    /**
     * @brief return the data of the request
     * @return the data
     */
    std::string get_data() const
    {
        return data;
    }

private:
    std::string url;
    std::string data;
    headers_t headers;
    cookies_t cookies;
    curl_slist* curl_headers;

    void prepare()
    {
#ifdef _DEBUG
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        // set headers
        prepareHeaders();
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, this->curl_headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    }
    void prepareHeaders()
    {
        // cleanup old headers in case this gets reused
        if (curl_headers)
            curl_slist_free_all(curl_headers);

        curl_slist* curl_headers = nullptr;
        // prepare cookies
        std::string cookie_header = "Cookie: ";
        for (auto [key, value] : cookies)
        {
            cookie_header += key + "=" + value + ";";
        }

        curl_headers = curl_slist_append(curl_headers, cookie_header.c_str());
        // define headers
        for (auto [key, value] : headers)
        {
            std::string header = key + ": " + value;
            curl_headers = curl_slist_append(curl_headers, header.c_str());
        }

        this->curl_headers = curl_headers; // save for later cleanup
    }
    Response execute()
    {
        Response response{};
        std::string headers;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _m_writeFunction);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.data);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headers);

        CURLcode code = curl_easy_perform(curl);

        // parse header string to vector of headers
        std::stringstream ss(headers);
        std::string line;
        while (std::getline(ss, line))
        {
            std::string key;
            std::string value;
            std::stringstream lineStream(line);
            std::getline(lineStream, key, ':');
            // consume 1 space
            std::getline(lineStream, value, ' ');
            // get actual value
            std::getline(lineStream, value, '\r');

            response.headers[key] = value;
        }

        response.code = code;
        return response;
    }

    CURL* curl;
};