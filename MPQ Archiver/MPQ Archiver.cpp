#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include <coroutine>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <locale>
#include <string>
#include <vector>

#include "../StormLib-master/src/StormLib.h"
#include "StringBuilder.h"
#include "windows.h"

#define as_wchar(string) (const TCHAR*)(converter.from_bytes(string).c_str())
#define concat$(...) string_builder.append(__VA_ARGS__)->result()
#define assert$(expression, ...)                               \
    {                                                          \
        if (!(expression)) {                                   \
            const char* result = concat$(__VA_ARGS__);         \
            throw std::exception(result, GetLastError() || 1); \
            ::operator delete[]((void*)result);                \
        };                                                     \
    }

const std::allocator<std::string> alloc;
std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>> converter;
std::vector<std::string> arguments(alloc);
std::string archive_path;
std::string folder_path;
std::wstring _;
HANDLE archive_handle;
bool unpack;

/*
inline const TCHAR* as_wchar(std::string string) noexcept {
    auto instance = converter.from_bytes(string);
    return (const TCHAR*)(instance.c_str());
}
*/
inline std::string concat_string(const char* first) noexcept {
    return std::string(first);
}

inline std::string concat_string(std::string first) noexcept {
    return first;
}

inline std::string concat_string(std::string first, std::string second) noexcept {
    return first.append(second);
}

inline std::string concat_string(std::string first, const char* second) noexcept {
    return first.append(second);
}

inline std::string concat_string(const char* first, std::string second) noexcept {
    return std::string(first).append(second);
}

inline std::string concat_string(const char* first, const char* second) noexcept {
    return std::string(first).append(second);
}

template<typename... Strings>
inline std::string concat_string(const char* first, const char* second, Strings... values) noexcept {
    return concat_string(std::string(first).append((const char*)second).c_str(), values...);
}

template<typename... Strings>
inline std::string concat_string(std::string first, const char* second, Strings... values) noexcept {
    return concat_string(first.append(second), values...);
}

template<typename... Strings>
inline std::string concat_string(std::string first, std::string second, Strings... values) noexcept {
    return concat_string(first.append(second), values...);
}

/*
struct read_file$ {
    const char* data;
    std::size_t length;

    read_file$(const char* data, std::size_t length) {
        this->data = data;
        this->length = length;
    }
};
*/

void write_file(SFILE_FIND_DATA*);
//read_file$* read_file(std::filesystem::path);

StringBuilder string_builder;

int main(int argc, const char* args[]) {
    try {
        arguments.resize(argc);
        for (int index = 0; index < argc; index++) {
            arguments[index] = args[index];
            if (arguments[index] == "--pack" || arguments[index] == "--unpack") {
                unpack = arguments[index] == "--unpack";
                if (++index >= argc) {
                    throw std::out_of_range("Archive file path must be specified correctly");
                }
                archive_path = args[index];
            } else if (arguments[index] == "--out") {
                if (++index >= argc) {
                    throw std::out_of_range("Output file / folder path must be specified correctly");
                }
                folder_path = args[index];
            } else if (arguments[index] == "--help") {
                std::cout << "The MPQ Archiver (Copyright @MadProbe 2022 - present, all right reseved)" << std::endl
                          << "With this program you can easily unpack MPQ into folder or pack a folder into an MPQ archive" << std::endl
                          << "CL Options:" << std::endl
                          << "--help - print this help message" << std::endl
                          << "--unpack (file path) - unpack archive %filename%" << std::endl
                          << "--pack (file path) - pack folder %filename%" << std::endl
                          << "--out (file path) - pack folder into %filename% or unpack archive into folder %filename%" << std::endl;
                return 0;
            }
        };
        if (!&folder_path) {
            throw std::exception("Output archive / folder must be specified", 1);
        }
        if (!&archive_path) {
            throw std::exception("Input archive / folder must be specified", 1);
        }
        std::cout << "folder_path = " << std::filesystem::absolute(folder_path) << std::endl
                  << "archive_path = " << std::filesystem::absolute(archive_path) << std::endl;
        if (unpack) {
            const std::filesystem::path folder_out_directory_path = std::filesystem::absolute(folder_path);
            folder_path = folder_out_directory_path.generic_string();
            std::filesystem::create_directory(folder_out_directory_path);
            assert$(SFileOpenArchive(as_wchar(archive_path), 0x0, STREAM_FLAG_READ_ONLY, &archive_handle),
                    "Could not open archive ", archive_path);
            SFILE_FIND_DATA* found_data = new SFILE_FIND_DATA;
            HANDLE handle = SFileFindFirstFile(archive_handle, "*", found_data, nullptr);
            for (bool done = false; &found_data && handle && !done;) {
                write_file(found_data);
                if (done = !SFileFindNextFile(handle, found_data)) {
                    delete found_data;
                }
            }
            if (handle) {
                SFileFindClose(handle);
            }
            assert$(SFileCloseArchive(archive_handle),
                    "Could not close archive ", archive_path);
        } else {
            if (std::filesystem::exists(std::filesystem::absolute(folder_path))) {
                std::filesystem::remove(std::filesystem::absolute(folder_path));
            }
            assert$(SFileCreateArchive(std::filesystem::absolute(folder_path).generic_wstring().data(), 0x0UL, 0x80000UL, &archive_handle),
                    "Could not create archive ", folder_path);
            std::filesystem::recursive_directory_iterator rec_dir_iter(std::filesystem::absolute(archive_path));
            std::size_t file_count = 0;
            for (const std::filesystem::directory_entry& entry : rec_dir_iter) {
                if (entry.is_regular_file()) {
                    const std::string file_name = std::filesystem::proximate(entry, archive_path).generic_string();
                    if (file_name != "(attributes)" && file_name != "(listfile)") {
                        /*
                         file_count++;
                         auto _ = read_file(entry);
                         auto [data, length] = *_;
                         void* file_handle = nullptr;
                         assert$(SFileCreateFile(archive_handle, file_name.c_str(), 0, length,
                                                 0, MPQ_FILE_COMPRESS, &file_handle),
                                 "Could not create file ", file_name);
                         assert$(SFileWriteFile(file_handle, data, length, MPQ_COMPRESSION_LZMA),
                                 "Could not write file ", file_name);
                         assert$(SFileCloseFile(file_handle), "Could not close file ", file_name);*/

                        SFileAddFileEx(archive_handle, entry.path().generic_wstring().data(),
                                       file_name.c_str(), MPQ_FILE_COMPRESS,
                                       MPQ_COMPRESSION_ZLIB, MPQ_COMPRESSION_NEXT_SAME);

                        const DWORD error_code = GetLastError();
                        assert$(error_code == 0 || error_code == 2 || error_code == 10003,
                                "Could add file ", entry.path().generic_string(), " to archive ", folder_path);
                        if (error_code == 10003) {
                            std::cerr << "Cannot add file " << file_name << " into archive as it is for internal use." << std::endl;
                        }
                        /*
                        if (error_code == 2) {
                            std::cout << file_name << std::endl;
                        }*/
                        //delete _;
                    }
                }
            }
            /*assert$(SFileSetMaxFileCount(archive_handle, file_count < HASH_TABLE_SIZE_MIN ? HASH_TABLE_SIZE_MIN : file_count),
                    "Could not compact archive ", folder_path);*/
            assert$(SFileCompactArchive(archive_handle, nullptr, false),
                    "Could not compact archive ", folder_path);
            assert$(SFileCloseArchive(archive_handle),
                    "Could not close archive ", folder_path);
        }
        std::cout << "Everything went ok, closing!" << std::endl;
    } catch (const std::exception& error) {
        std::cerr << error.what() << std::endl
                  << "Error Code: " << GetLastError() << std::endl;
        return 1;
    }
    return 0;
}

inline void write_file(SFILE_FIND_DATA* found_data) {
    void* file_data = operator new[](found_data->dwFileSize);
    std::string file_name_normalized(found_data->cFileName);
    HANDLE archive_file_handle = nullptr;
    assert$(SFileOpenFileEx(archive_handle, found_data->cFileName, 0, &archive_file_handle),
            "Could not open file ", found_data->cFileName, " from archive ", archive_path);
    assert$(SFileReadFile(archive_file_handle, file_data, found_data->dwFileSize, nullptr, nullptr),
            "Could not read file ", found_data->cFileName, " from archive ", archive_path);
    assert$(SFileCloseFile(archive_file_handle),
            "Could not close file ", found_data->cFileName, " from archive ", archive_path);
    std::filesystem::path file_path = folder_path;
    file_path /= file_name_normalized;
    std::filesystem::create_directories(file_path.parent_path());
    std::ofstream out_file_stream(file_path, std::ios::binary);
    out_file_stream.write((const char*)file_data, found_data->dwFileSize);
    delete[] file_data;
}

/*
inline read_file$* read_file(std::filesystem::path file_path) {
    std::ifstream file_stream(file_path);
    const std::size_t file_size = std::filesystem::file_size(file_path);
    const char* file_data = static_cast<char*>(operator new[](file_size));
    file_stream.get(const_cast<char*>(file_data), (std::streamsize)file_size);
    return new read_file$(file_data, file_size);
}
*/
// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started:
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add
//   Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project
//   and select the .sln file
