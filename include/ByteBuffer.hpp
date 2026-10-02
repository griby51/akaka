#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

class ByteWriter{
public:
    void u8(uint8_t v){ mData.push_back(v); }

    void u16(uint16_t v){
        u8((uint8_t)(v & 0xFF));
        u8((uint8_t)((v >> 8) & 0xFF));
    }

    void u32(uint32_t v){
        u16((uint16_t)(v & 0xFFFF));
        u16((uint16_t)((v >> 16) & 0xFFFF));
    }

    void i16(int16_t v){ u16((uint16_t)v); }
    void i32(int32_t v){ u32((uint32_t)v); }

    void f32(float v){
        uint32_t bits;
        std::memcpy(&bits, &v, sizeof(bits));
        u32(bits);
    }

    const std::vector<uint8_t>& data() const { return mData; }
    void clear(){ mData.clear(); }

private:
    std::vector<uint8_t> mData;
};

class ByteReader{
public:
    ByteReader(const uint8_t* data, size_t size) : mData(data), mSize(size) {}

    uint8_t u8(){
        if(mPos + 1 > mSize){ mOk = false; return 0; }
        return mData[mPos++];
    }

    uint16_t u16(){
        uint16_t low = u8();
        uint16_t high = u8();
        return (uint16_t)(low | (high << 8));
    }

    uint32_t u32(){
        uint32_t low = u16();
        uint32_t high = u16();
        return low | (high << 16);
    }

    int16_t i16(){ return (int16_t)u16(); }
    int32_t i32(){ return (int32_t)u32(); }

    float f32(){
        uint32_t bits = u32();
        float v = 0.f;
        std::memcpy(&v, &bits, sizeof(v));
        return v;
    }

    bool ok() const { return mOk; }

private:
    const uint8_t* mData;
    size_t mSize;
    size_t mPos = 0;
    bool mOk = true;
};
