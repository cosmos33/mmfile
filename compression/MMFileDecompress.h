//
// Created by chenwangwang on 2018/9/14.
//

#ifndef MMLOG_MMFILEDECOMPRESS_H
#define MMLOG_MMFILEDECOMPRESS_H

#include <string>
#include "ITraverseResolver.h"
#include "../base/autobuffer.h"

namespace mmlog {

#define VERSION_ONE 1
#define HEAD_LENGTH_OLD 9
#define HEAD_LENGTH_VERSION_ONE 14

    class DataBridge {
    public:
        AutoBuffer outBuffer;
        size_t writeLength;
        ErrorType errorType = NoneError;
    public:
        DataBridge();
    };

    class Compression {
    private:
        ITraverseResolver* resolver = nullptr;
        char kMagic_crypt_no_compress = 5;          // 加密不压缩，老
        char kMagic_crypt_compress = 7;             // 加密又压缩，老
        char kMagic_no_crypt_compress = 9;          // 压缩不加密，老

        char kMagic_end = 0;                          // 尾标识

        char kMagic_crypt_no_compress_new = 18;          // 加密不压缩，新
        char kMagic_crypt_compress_new = 19;             // 加密又压缩，新
        char kMagic_no_crypt_compress_new = 20;          // 压缩不加密，新

    private:
        off_t GetLogStartPos(char *data, off_t size, off_t offset, int32_t i);

        off_t DecodeBuffer(char *data, off_t size, off_t startpos, DataBridge *dataBridge);

        bool IsGoodLogBuffer(char *buffer, off_t size, off_t offset, int32_t count);

        void decrypt(AutoBuffer &buffer, int key);
    public:

        Compression(ITraverseResolver *resolver);

        /**
         * 解析一个目录下的日志，或者是一个日志文件
         * @param path 目录/文件路径
         */
        void decode(std::string path);
        /**
         * 解析一个解析一个目录下的日志，或者是一个日志文件
         * @param path 目录/文件路径
         * @param out_to_file 是否需要自动输出到文件
         */
        void decode(std::string path, bool out_to_file);
        /**
         * 解析一个日志文件
         * @param file_path 日志文件路径
         * @param out_path 输出文件的路径
         */
        void decodeFile(std::string file_path, std::string out_path);

        /**
         * 解析一个日志存放目录
         * @param src_dir 源目录
         * @param dst_dir 目标目录
         */
        void decodeDirectory(std::string src_dir, std::string dst_dir);

        /**
         * 解析一个二进制数据，结果不保留到文件
         * @param data 数据开始指针
         * @param length 数据长度
         */
        void decode(char *data, off_t length);
    };
}

#endif //MMLOG_MMFILEDECOMPRESS_H
