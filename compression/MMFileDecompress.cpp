//
// Created by chenwangwang on 2018/9/14.
//

#include "MMFileDecompress.h"
#include "../base/MMLogUtils.h"
#include "ZLibCompress.h"
#include <fstream>
#include <iostream>
#include "../base/Log.h"

using namespace std;

namespace mmlog {

#define OUT_BUFFER_LENGTH 1024 * 1024 * 13

    /**
     * 解析一段带头和尾标记符的字段，并返回下一个待解密的头下标
     */
    off_t Compression::DecodeBuffer(char *_buffer, off_t size, off_t _offset, DataBridge *dataBridge) {
        // 到文件尾巴上了
        if (_offset >= size) {
            return -1;
        }

        //判断一下是否是合法
        bool ret = IsGoodLogBuffer(_buffer, size, _offset, 1);

        // 如果不合法，找下一个合法的头下标
        if (!ret) {
            _MOMO_FILE_LOGI_("invalid index");
            off_t fixpos = GetLogStartPos(_buffer, size, _offset + 1, 1);
            if (-1 == fixpos) {
                return -1;
            } else {
                _MOMO_FILE_LOGI_("[F]decode_log_file.py decode error");
                _offset = fixpos;
            }
        }

        // 检验头字符是否合规范
        char magic_start = _buffer[_offset];
        bool isOldVersion = true;
        if (kMagic_crypt_compress != magic_start && kMagic_crypt_no_compress != magic_start &&
            kMagic_no_crypt_compress != magic_start) {
            if (kMagic_crypt_compress_new != magic_start && kMagic_crypt_no_compress_new != magic_start &&
                    kMagic_no_crypt_compress_new != magic_start) {
                return -1;
                _MOMO_FILE_LOGI_("in DecodeBuffer magic start != MAGIC_NUM_START");
            } else {
                isOldVersion = false;
            }
        }

        // 取本段日志长度
        uint32_t headerLen = 0;
        char version = 0;
        if (isOldVersion) {
            headerLen = HEAD_LENGTH_OLD;
        } else {
            // 新版本头
            version = _buffer[_offset + 1];
            if (version == VERSION_ONE) {
                // 第一版
                headerLen = HEAD_LENGTH_VERSION_ONE;     // 14个字节
            } else {
                // 识别不到的版本号
                return -1;
            }
        }

        // 取压缩后的日志长度
        uint32_t encodedLength = 0;
        if (version == VERSION_ONE) {
            memcpy(&encodedLength, _buffer + _offset + sizeof(char) * 2, sizeof(encodedLength));
        } else {
            memcpy(&encodedLength, _buffer + _offset + headerLen - sizeof(uint32_t), sizeof(encodedLength));
        }

        // 取压缩前的日志长度
        uint32_t originLength = 0;
        if (version == VERSION_ONE) {
            memcpy(&originLength, _buffer + _offset + sizeof(char) * 2 + sizeof(uint32_t), sizeof(originLength));
        }

        // 创建一个bytearray来存储未解密/解压缩之前的数据
        AutoBuffer tmpbuffer;
        tmpbuffer.AllocWrite(encodedLength);

        // 本段未解密/解压缩之前的数据
        tmpbuffer.Write(_buffer + _offset + headerLen, encodedLength);

        if (kMagic_crypt_no_compress == _buffer[_offset] || kMagic_crypt_no_compress_new == _buffer[_offset]) {              // 加密但是没压缩的情况
            decrypt(tmpbuffer, 13);                                     // 这里的13是加密用到的key
            dataBridge->outBuffer.Write(tmpbuffer);
            dataBridge->writeLength = encodedLength;
        } else if (kMagic_no_crypt_compress == _buffer[_offset] || kMagic_no_crypt_compress_new == _buffer[_offset]) {       // 压缩但不加密
            if (originLength != 0 && dataBridge->outBuffer.Length() < originLength) {
                dataBridge->outBuffer.AllocWrite(originLength, true);
                if (resolver) {
                    resolver->onMemoryExtension(originLength);
                }
            }
            ZLibCompress zLibCompress(false);
            zLibCompress.reset();
            size_t write_length = 0;
            bool decodeResult = zLibCompress.deCompress(tmpbuffer.Ptr(), tmpbuffer.Length(), (Bytef *) dataBridge->outBuffer.PosPtr(),
                                                        dataBridge->outBuffer.Length() - dataBridge->outBuffer.Pos(), write_length);
            if (decodeResult && (originLength == 0 || originLength == write_length)) {
                dataBridge->writeLength = write_length;
                dataBridge->outBuffer.Length(write_length + dataBridge->outBuffer.Pos(), dataBridge->outBuffer.Length());
            } else {
                // 解压缩失败
                dataBridge->writeLength = 0;
                dataBridge->outBuffer.Length(0, dataBridge->outBuffer.Length());
                dataBridge->errorType = decodeResult ? LogLengthError : ZLibError;
            }
        } else if (kMagic_crypt_compress == _buffer[_offset] || kMagic_crypt_compress_new == _buffer[_offset]) {          //即压缩也加密
            if (originLength != 0 && dataBridge->outBuffer.Length() < originLength) {
                dataBridge->outBuffer.AllocWrite(originLength - dataBridge->outBuffer.Length(), true);
                if (resolver) {
                    resolver->onMemoryExtension(originLength);
                }
            }
            decrypt(tmpbuffer, 13);                                     // 这里的13是加密用到的key
            ZLibCompress zLibCompress(false);
            zLibCompress.reset();
            size_t write_length = 0;
            bool decodeResult = zLibCompress.deCompress(tmpbuffer.Ptr(), tmpbuffer.Length(), (Bytef *) dataBridge->outBuffer.PosPtr(),
                                                        dataBridge->outBuffer.Length() - dataBridge->outBuffer.Pos(), write_length);
            if (decodeResult && (originLength == 0 || originLength == write_length)) {
                dataBridge->writeLength = write_length;
                dataBridge->outBuffer.Length(write_length + dataBridge->outBuffer.Pos(), dataBridge->outBuffer.Length());
            } else {
                // 解压缩失败
                dataBridge->writeLength = 0;
                dataBridge->outBuffer.Length(0, dataBridge->outBuffer.Length());
                dataBridge->errorType = decodeResult ? LogLengthError : ZLibError;
            }
        }

        return _offset + headerLen + encodedLength + 1;
    }

    void Compression::decrypt(AutoBuffer &buffer, int key) {
        AutoBuffer tmp;
        tmp.AllocWrite(buffer.Length());
        for (int i = 0; i < buffer.Pos(); ++i) {
            char b = (char) (((char *) buffer.Ptr())[i] ^ key);
            tmp.Write(b);
        }
        buffer.Length(0, buffer.Length());
        buffer.Write(tmp);
    }

    off_t Compression::GetLogStartPos(char *_buffer, off_t size, off_t offset, int32_t _count) {
        while (true) {
            if (offset >= size) {
                break;
            }

            if (kMagic_crypt_no_compress == _buffer[offset] ||
                    kMagic_crypt_compress == _buffer[offset] ||
                    kMagic_no_crypt_compress == _buffer[offset] ||
                    kMagic_crypt_no_compress_new == _buffer[offset] ||
                    kMagic_crypt_compress_new == _buffer[offset] ||
                    kMagic_no_crypt_compress_new == _buffer[offset]) {
                if (IsGoodLogBuffer(_buffer, size, offset, _count)) {
                    return offset;
                }
            }
            offset += 1;
        }

        return -1;
    }

    /**
     * 判断文件是否是符合规范的日志
     */
    bool Compression::IsGoodLogBuffer(char *_buffer, off_t size, off_t _offset, int32_t count) {
        // 已经到达buffer尾部
        if (_offset == size) {
            return true;
        }

        // 检验头是否合法
        char magic_start = _buffer[_offset];

        bool isOldVersion = true;
        if (kMagic_crypt_compress != magic_start &&
                kMagic_crypt_no_compress != magic_start &&
                kMagic_no_crypt_compress != magic_start) {
            // 不是老版本，开始校验新版本
            if (kMagic_crypt_compress_new != magic_start &&
                kMagic_crypt_no_compress_new != magic_start &&
                kMagic_no_crypt_compress_new != magic_start) {
                // 新老版本都不是，return false
                return false;
            } else {
                // 新版本
                isOldVersion = false;
            }
        }

        // 头长度
        uint32_t headerLen = 0;
        char version = 0;
        if (isOldVersion) {
            // 老版本头
            headerLen = HEAD_LENGTH_OLD;          // 9个字节
        } else {
            // 新版本头
            if (_offset + 1 >= size) {
                return false;
            }
            version = _buffer[_offset + 1];
            if (version == VERSION_ONE) {
                // 第一版
                headerLen = HEAD_LENGTH_VERSION_ONE;     // 14个字节
            } else {
                // 识别不到的版本号
                return false;
            }
        }

        // offset+头长度超过buffer长度，失败
        if (_offset + headerLen + 1 >= size) {
            return false;
        }

        // 取压缩后的日志长度
        uint32_t encodedLength = 0;
        if (version == VERSION_ONE) {
            memcpy(&encodedLength, _buffer + _offset + sizeof(char) * 2, sizeof(encodedLength));
        } else if (isOldVersion) {
            memcpy(&encodedLength, _buffer + _offset + headerLen - sizeof(uint32_t), sizeof(encodedLength));
        }

        // 如果offset + 头 + 日志长度超过buffer长度，失败
        if (_offset + headerLen + encodedLength + 1 > size) {
            return false;
        }

        // 如果结束标记符对不上，失败
        if (kMagic_end != _buffer[_offset + headerLen + encodedLength]) {
            return false;
        }

        // count 应该是用来控制检验个数的，比如，为了检验文件是否有效，我得多看几个，但是在读取中，就只需要确认一个就好
        if (1 >= count) {
            return true;
        } else {
            return IsGoodLogBuffer(_buffer, size, _offset + headerLen + encodedLength + 1, count - 1);
        }
    }

    void Compression::decode(std::string path) {
        decode(path, true);
    }

    void Compression::decode(std::string path, bool out_to_file) {
        if (!MMLogUtils::isFileExit(path.c_str())) {
            return;
        }
        if (MMLogUtils::isDir(path.c_str())) {
            MMLogUtils::traverseFolder(path.c_str(), [this, out_to_file](std::string filePath, std::string name) {
                std::string filenameStr;
                if (out_to_file) {
                    filenameStr = std::string(filePath);
                    filenameStr.append(".log");
                }
                decodeFile(filePath, filenameStr);
                return false;
            });
        } else {
            std::string str;
            if (out_to_file) {
                str = std::string(path);
                str.append(".log");
            }
            decodeFile(path, str);
        }
    }

    void Compression::decodeFile(std::string file_path, std::string out_path) {
        if (!MMLogUtils::isFileExit(file_path.c_str())) {
            return;
        }

        std::string suffix(".xlog");
        size_t index = file_path.rfind(suffix);
        if (index == string::npos || index != file_path.length() - suffix.length()) {
            return;
        }

        struct stat s;
        lstat(file_path.c_str(), &s);
        char *data = static_cast<char *>(malloc(s.st_size));

        // 以读模式打开文件
        ifstream infile;
        infile.open(file_path);
        // 读取数据
        infile.read(data, s.st_size);
        // 关闭打开的文件
        infile.close();

        // 校验文件的有效性
        off_t startpos = GetLogStartPos(data, s.st_size, 0, 2);
        if (-1 == startpos) {
            return;
        }

        // 解析文件
        DataBridge dataBridge;
        ofstream *outfile = nullptr;
        if (!out_path.empty()) {
            outfile = new ofstream();
            outfile->open(out_path, ios::out | ios::trunc);
        }

        while (true) {
            dataBridge.outBuffer.Length(0, OUT_BUFFER_LENGTH > dataBridge.outBuffer.Length() ? OUT_BUFFER_LENGTH: dataBridge.outBuffer.Length());
            dataBridge.errorType = NoneError;
            startpos = DecodeBuffer(data, s.st_size, startpos, &dataBridge);
            if (-1 == startpos) {
                break;
            } else if (dataBridge.writeLength > 0) {
                if (outfile) {
                    outfile->write((const char *) dataBridge.outBuffer.Ptr(), dataBridge.outBuffer.Pos());
                }
                if (resolver) {
                    resolver->traverse((const char *) dataBridge.outBuffer.Ptr(), dataBridge.outBuffer.Pos());
                }
            } else {
                _MOMO_FILE_LOGI_("decompress error");
                if (resolver && dataBridge.errorType != NoneError) {
                    resolver->onDecompressError(dataBridge.errorType);
                }
            }
        }

        if (outfile) {
            outfile->close();
        }

        free(data);
    }

    void Compression::decodeDirectory(std::string src_dir, std::string dst_dir) {
        if (!MMLogUtils::isDir(src_dir.c_str())) {
            return;
        }
        MMLogUtils::traverseFolder(src_dir.c_str(), [this, dst_dir](std::string filePath, std::string name) -> bool {
            std::string outFile(dst_dir);
            std::string sep = "/";
#ifdef _WIN32
            sep = "\\";
#endif
            size_t lastIndex = dst_dir.rfind(sep);
            if (lastIndex == dst_dir.length() - 1) {
                outFile.append(name);
            } else {
                outFile.append(sep);
                outFile.append(name);
            }
            decodeFile(filePath, outFile);
            return false;
        });
    }

    Compression::Compression(ITraverseResolver *resolver)
            : resolver(resolver) {}

    void Compression::decode(char *data, off_t length) {
        // 校验文件的有效性
        off_t startpos = GetLogStartPos(data, length, 0, 2);
        if (-1 == startpos) {
            return;
        }

        // 解析文件
        DataBridge dataBridge;
        while (true) {
            dataBridge.outBuffer.Length(0, OUT_BUFFER_LENGTH > dataBridge.outBuffer.Length() ? OUT_BUFFER_LENGTH: dataBridge.outBuffer.Length());
            dataBridge.errorType = NoneError;
            startpos = DecodeBuffer(data, length, startpos, &dataBridge);
            if (-1 == startpos) {
                break;
            } else if (dataBridge.writeLength > 0){
                if (resolver) {
                    resolver->traverse((const char *) dataBridge.outBuffer.Ptr(), dataBridge.outBuffer.Pos());
                }
            } else {
                _MOMO_FILE_LOGI_("decompress error");
                if (resolver && dataBridge.errorType != NoneError) {
                    resolver->onDecompressError(dataBridge.errorType);
                }
            }
        }
    }


    DataBridge::DataBridge() {
        outBuffer.AllocWrite(OUT_BUFFER_LENGTH);
        outBuffer.Length(0, OUT_BUFFER_LENGTH);
        writeLength = 0;
    }
}
