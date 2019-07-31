//
// Created by chenwangwang on 2018/9/15.
//

#ifndef MMLOG_ITRAVERSEINFLATER_H
#define MMLOG_ITRAVERSEINFLATER_H

/**
 * 该接口定义了每个完整数据的回调接口
 */
namespace mmlog {

    enum ErrorType {
        ZLibError, LogLengthError, NoneError
    };

  class ITraverseResolver {
    public:
        /**
         * 解析完完整的数据之后的回调，一个文件会有多个完整的数据
         * @param data 数据
         * @param length 数据长度
         */
        virtual void traverse(const char *data, off_t length) = 0;

        /**
         * 解析错误事件回调
         * @param errorType 错误类型
         */
        virtual void onDecompressError(ErrorType errorType) = 0;

        /**
         * 内存不足进行拓展
         * @param newLength 新的内存长度
         */
        virtual void onMemoryExtension(uint64_t newLength) = 0;
    };
}


#endif //MMLOG_ITRAVERSEINFLATER_H
