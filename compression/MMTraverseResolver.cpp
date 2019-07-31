//
// Created by chenwangwang on 2018/9/15.
//

#include <iostream>
#include "MMTraverseResolver.h"
#include <cstring>

namespace mmlog {

    void MMTraverseResolver::traverse(const char *data, off_t length) {
        if (!data) {
            return;
        }
        if (head_wrapper_start == -1 || head_wrapper_end == -1 || body_wrapper_start == -1 || body_wrapper_end == -1) {
            return;
        }
        bool startHead = false;
        bool startBody = false;
        MMLogInfo *info = nullptr;
        off_t index = 0;
        for (off_t i = 0; i < length; ++i) {
            char ascii = data[i];
            if (ascii == head_wrapper_start) {
                if (!info) {
                    info = new MMLogInfo();
                }
                startHead = true;
                index = i;
            } else if (ascii == head_wrapper_end) {
                if (startHead && info) {
                    char *char_str = static_cast<char *>(malloc(static_cast<size_t>(i - index)));
                    memset(char_str, 0, (size_t) (i - index));
                    memcpy(char_str, data + index + 1, (size_t) (i - index - 1));
                    info->heads.push_back(std::string(char_str));
                    free(char_str);
                    char_str = nullptr;
                }
                startHead = false;
            } else if (ascii == body_wrapper_start) {
                if (!info) {
                    info = new MMLogInfo();
                }
                startBody = true;
                index = i;
            } else if (ascii == body_wrapper_end) {
                if (startBody && info) {
                    char *char_str = static_cast<char *>(malloc(static_cast<size_t>(i - index)));
                    memset(char_str, 0, (size_t) (i - index));
                    memcpy(char_str, data + index + 1, (size_t) (i - index - 1));
                    info->body = std::string(char_str);
                    itemResolver(info);
                    free(char_str);
                    char_str = nullptr;
                }
                if (info) {
                    delete(info);
                    info = nullptr;
                }
                startBody = false;
            }
        }
    }

    MMTraverseResolver::MMTraverseResolver(std::function<void(const MMLogInfo * _info)> item_resolver,
                                           int8_t _head_wrapper_start,
                                           int8_t _head_wrapper_end, int8_t _body_wrapper_start, int8_t _body_wrapper_end)
            : itemResolver(item_resolver), head_wrapper_start(_head_wrapper_start), head_wrapper_end(_head_wrapper_end),
                body_wrapper_start(_body_wrapper_start), body_wrapper_end(_body_wrapper_end) {
    }

    void MMTraverseResolver::onDecompressError(ErrorType errorType) {

    }

    void MMTraverseResolver::onMemoryExtension(uint64_t newLength) {

    }

}