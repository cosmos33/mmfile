//
// Created by chenwangwang on 2018/9/15.
//

#ifndef MMLOG_MMTRAVERSEINFLATER_H
#define MMLOG_MMTRAVERSEINFLATER_H

#include <functional>
#include "ITraverseResolver.h"
#include "../base/MMFileInfo.h"

namespace mmlog {

    class MMTraverseResolver : public ITraverseResolver {
    private:
        std::function<void(const MMLogInfo *_info)> itemResolver;
        int8_t head_wrapper_start;                     // 头tag的前置分隔符
        int8_t head_wrapper_end;                       // 头tag的后置分隔符
        int8_t body_wrapper_start;                     // 内容区域tag的前置分隔符
        int8_t body_wrapper_end;                       // 内容区域tag的后置分隔符
    public:
        MMTraverseResolver(std::function<void(const MMLogInfo * _info)> itemResolver,
                           int8_t _head_wrapper_start = '\x01',
                           int8_t _head_wrapper_end = '\x04',
                           int8_t _body_wrapper_start = '\x02',
                           int8_t _body_wrapper_end = '\x03');

        void traverse(const char *data, off_t length) override;

        void onDecompressError(ErrorType errorType) override;

        void onMemoryExtension(uint64_t newLength) override;

    };
}


#endif //MMLOG_MMTRAVERSEINFLATER_H
