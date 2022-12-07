#pragma once
#include <memory>
#include <string>

class StringBuilder {
    public:
    const char* data = nullptr;
    std::size_t offset = 0;

    StringBuilder() {}

    template<bool isFirst = true, typename... Strings>
    StringBuilder* append(const char* const string, Strings... strings) {
        if (isFirst) {
            if (this->data) {
                if constexpr (sizeof...(strings) >= 2) {
                    return this->append(this->data, string)->append(strings...);
                } else {
                    return this->append(this->data, string, strings...);
                }
            }
            std::size_t length = strings_length(string, strings...);
            this->data = (const char*)::operator new[](length + 1);
            const_cast<char*>(this->data)[length] = (const char)0;
        }
        std::size_t length = std::char_traits<char>::length(string);
        memcpy((void*)(this->data + this->offset), string, length);
        this->offset += length;
        if constexpr (sizeof...(Strings) > 0) {
            return this->append<false>(strings...);
        }
        return this;
    }

    template<bool isFirst = true, typename... Strings>
    StringBuilder* append(std::string string, Strings... strings) {
        if (isFirst) {
            if (this->data) {
                if constexpr (sizeof...(strings) >= 2) {
                    return this->append(this->data, string)->append(strings...);
                } else {
                    return this->append(this->data, string, strings...);
                }
            }
            std::size_t length = strings_length(string, strings...);
            this->data = (const char*)::operator new[](length + 1);
            const_cast<char*>(this->data)[length] = (const char)0;
        }
        std::size_t length = string.length();
        memcpy((void*)(this->data + this->offset), string.c_str(), length);
        this->offset += length;
        if constexpr (sizeof...(Strings) > 0) {
            return this->append<false>(strings...);
        }
        return this;
    }

    const char* result() {
        const char* result = this->data;
        this->data = nullptr;
        return result;
    }

    ~StringBuilder() {
        if (this->data) {
            delete this->data;
        }
    }

    private:
    template<typename... Strings>
    static inline std::size_t strings_length(const char* const string, Strings... strings) {
        std::size_t length = std::char_traits<char>::length(string);
        if constexpr (sizeof...(Strings) > 0) {
            length += strings_length(strings...);
        }
        return length;
    }

    template<typename... Strings>
    static inline std::size_t strings_length(std::string string, Strings... strings) {
        std::size_t length = string.length();
        if constexpr (sizeof...(Strings) > 0) {
            length += strings_length(strings...);
        }
        return length;
    }
};
