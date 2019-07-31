#!/usr/bin/python
# coding=UTF-8

import sys
import os
import glob
import zlib
import struct
import binascii
import traceback


MAGIC_CRYPT_NO_COMPRESS = 0x05          # 加密不压缩
MAGIC_CRYPT_COMPRESS = 0x07             # 加密又压缩
MAGIC_NO_CRYPT_COMPRESS = 0x09          # 压缩不加密

MAGIC_CRYPT_NO_COMPRESS_NEW = 18          # 加密不压缩，新
MAGIC_CRYPT_COMPRESS_NEW = 19             # 加密又压缩，新
MAGIC_NO_CRYPT_COMPRESS_NEW = 20          # 压缩不加密，新

MAGIC_END = 0x00

HEAD_LENGTH_OLD = 9
HEAD_LENGTH_NEW_VERSION_ONE = 14

VERSION_ONE = 1

# 解密
def decrypt(tmpbuffer, key):
    ret = bytearray(len(tmpbuffer))
    index = 0
    for i in tmpbuffer:
        ret[index] = (i ^ key)
        index += 1
    return ret

# 判断文件是否是符合规范的日志
def IsGoodLogBuffer(_buffer, _offset, count):

    # 已经到达buffer尾部
    if _offset == len(_buffer): return (True, '')

    # 检验头是否合法
    magic_start = _buffer[_offset]

    isOldVersion = 1
    if MAGIC_CRYPT_COMPRESS != magic_start and MAGIC_CRYPT_NO_COMPRESS != magic_start and MAGIC_NO_CRYPT_COMPRESS != magic_start:
        if MAGIC_CRYPT_COMPRESS_NEW != magic_start and MAGIC_CRYPT_NO_COMPRESS_NEW != magic_start and MAGIC_NO_CRYPT_COMPRESS_NEW != magic_start:
            return (False, '_buffer[%d]:%d != MAGIC_NUM_START'%(_offset, _buffer[_offset]))
        else:
            isOldVersion = 0;

    # 头长度
    version = 0
    if isOldVersion == 1:
        headerLen = HEAD_LENGTH_OLD
    else:
        if _offset + 1 >= len(_buffer):
            return (False, 'no version')
        version = _buffer[_offset + 1]
        if version == VERSION_ONE:
            headerLen = HEAD_LENGTH_NEW_VERSION_ONE
        else:
            return (False, 'version not match')

    # offset+头长度超过buffer长度，失败
    if _offset + headerLen + 1 + 1 > len(_buffer):
        return (False, 'offset:%d > len(buffer):%d'%(_offset, len(_buffer)))

    # 取日志长度
    if version == VERSION_ONE:
        length = struct.unpack_from("I", buffer(_buffer, _offset + 2, 4))[0]
    else:
        length = struct.unpack_from("I", buffer(_buffer, _offset + headerLen - 4, 4))[0]

    # 如果offset + 头 + 日志长度超过buffer长度，失败
    if _offset + headerLen + length + 1 > len(_buffer):
        return (False, 'log length:%d, end pos %d > len(buffer):%d'%(length, _offset + headerLen + length + 1, len(_buffer)))

    # 如果结束标记符对不上，失败
    if MAGIC_END!=_buffer[_offset + headerLen + length]:
        return (False, 'log length:%d, buffer[%d]:%d != MAGIC_END'%(length, _offset + headerLen + length, _buffer[_offset + headerLen + length]))

    # count 应该是用来控制检验个数的
    if (1 >= count):
        return (True, '')
    else:
        return IsGoodLogBuffer(_buffer, _offset+headerLen+length+1, count-1)

# 获取第一个合法的位置
def GetLogStartPos(_buffer, _count):
    offset = 0
    while True:
        if offset >= len(_buffer): break

        if MAGIC_CRYPT_NO_COMPRESS==_buffer[offset] or MAGIC_CRYPT_COMPRESS==_buffer[offset] \
                or MAGIC_NO_CRYPT_COMPRESS==_buffer[offset] or MAGIC_CRYPT_NO_COMPRESS_NEW==_buffer[offset] \
                or MAGIC_CRYPT_COMPRESS_NEW==_buffer[offset] or MAGIC_NO_CRYPT_COMPRESS_NEW==_buffer[offset]:
            if IsGoodLogBuffer(_buffer, offset, _count)[0]: return offset
        offset+=1

    return -1

# 解析一段带头和尾标记符的字段，并返回下一个待解密的头下标
def DecodeBuffer(_buffer, _offset, _outbuffer):

    # 到文件尾巴上了
    if _offset >= len(_buffer):
        return -1

    # 判断一下是否是合法
    ret = IsGoodLogBuffer(_buffer, _offset, 1)

    # 如果不合法，找下一个合法的头下标
    if not ret[0]:
        fixpos = GetLogStartPos(_buffer[_offset:], 1)
        if -1==fixpos:
            return -1
        else:
            #_outbuffer.extend("[F]decode_log_file.py decode error len=%d, result:%s \n"%(fixpos, ret[1]))
            _offset += fixpos

    # 检验头字符是否合规范
    isOldVersion = 1
    magic_start = _buffer[_offset]
    if MAGIC_CRYPT_COMPRESS != magic_start and MAGIC_CRYPT_NO_COMPRESS != magic_start and MAGIC_NO_CRYPT_COMPRESS != magic_start:
        # _outbuffer.extend('in DecodeBuffer _buffer[%d]:%d != MAGIC_NUM_START'%(_offset, magic_start))
        if MAGIC_CRYPT_COMPRESS_NEW != magic_start and MAGIC_CRYPT_NO_COMPRESS_NEW != magic_start and MAGIC_NO_CRYPT_COMPRESS_NEW != magic_start:
            return -1
        else:
            isOldVersion = 0;

    # 取本段日志长度
    # 头长度
    version = 0
    if isOldVersion == 1:
        headerLen = HEAD_LENGTH_OLD
    else:
        version = _buffer[_offset + 1]
        if version == VERSION_ONE:
            headerLen = HEAD_LENGTH_NEW_VERSION_ONE
        else:
            return -1


    if version == VERSION_ONE:
        encodeLength = struct.unpack_from("I", buffer(_buffer, _offset + 2, 4))[0]
    else:
        encodeLength = struct.unpack_from("I", buffer(_buffer, _offset + headerLen - 4, 4))[0]
    # 创建一个bytearray来存储未解密/解压缩之前的数据
    tmpbuffer = bytearray(encodeLength)

    # 本段未解密/解压缩之前的数据
    tmpbuffer[:] = _buffer[_offset+headerLen:_offset+headerLen+encodeLength]

    try:
        if MAGIC_CRYPT_NO_COMPRESS ==_buffer[_offset] or MAGIC_CRYPT_NO_COMPRESS_NEW ==_buffer[_offset] :     # 加密但是没压缩的情况
            tmpbuffer = decrypt(tmpbuffer, 13)      # 这里的13是加密用到的key
        elif MAGIC_NO_CRYPT_COMPRESS ==_buffer[_offset] or MAGIC_NO_CRYPT_COMPRESS_NEW ==_buffer[_offset]:   # 压缩但不加密
            decompressor = zlib.decompressobj(-zlib.MAX_WBITS)
            tmpbuffer = decompressor.decompress(str(tmpbuffer))
        elif MAGIC_CRYPT_COMPRESS ==_buffer[_offset] or MAGIC_CRYPT_COMPRESS_NEW ==_buffer[_offset]:      # 即压缩也加密
            tmpbuffer = decrypt(tmpbuffer, 13)  # 这里的13是加密用到的key
            decompressor = zlib.decompressobj(-zlib.MAX_WBITS)
            tmpbuffer = decompressor.decompress(str(tmpbuffer))
        else:
            pass
    except Exception, e:
        traceback.print_exc()
        # _outbuffer.extend("[F]decode_log_file.py decompress err, " + str(e) + "\n")
        return _offset+headerLen+encodeLength+1

    _outbuffer.extend(tmpbuffer)

    return _offset+headerLen+encodeLength+1


def ParseFile(_file, _outfile):
    fp = open(_file, "rb")
    _buffer = bytearray(os.path.getsize(_file))
    fp.readinto(_buffer)
    fp.close()
    startpos = GetLogStartPos(_buffer, 2)
    if -1==startpos:
        return

    outbuffer = bytearray()

    while True:
        startpos = DecodeBuffer(_buffer, startpos, outbuffer)
        if -1==startpos: break;

    if 0==len(outbuffer): return

    fpout = open(_outfile, "wb")
    fpout.write(outbuffer)
    fpout.close()

def main(args):
    global lastseq

    if 1==len(args):
        if os.path.isdir(args[0]):
            filelist = glob.glob(args[0] + "/*.xlog")
            for filepath in filelist:
                lastseq = 0
                ParseFile(filepath, filepath+".log")
        else: ParseFile(args[0], args[0]+".log")
    elif 2==len(args):
        ParseFile(args[0], args[1])
    else:
        filelist = glob.glob("*.xlog")
        for filepath in filelist:
            lastseq = 0
            ParseFile(filepath, filepath+".log")

if __name__ == "__main__":
    main(sys.argv[1:])
